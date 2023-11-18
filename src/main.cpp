#include <Arduino.h>

#include <driver/i2s.h>
#include <AsyncUDP.h>

#include <SPIFFS.h>
#include <ElegantOTA.h>

#include <ArduinoJson.h>
#include "sigma_connect.h"

#include "device_config.h"
#include "device_status.h"

#include "vban_common.h"
#include "vban_receiver.h"
#include "vban_transmitter.h"

extern "C"
{
#include "vban.h"
#include "stream.h"
#include "packet.h"
}

#define ETH_PHY_ADDR 1
#define ETH_PHY_TYPE ETH_PHY_LAN8720
#define ETH_PHY_POWER 16
#define ETH_PHY_MDC 23
#define ETH_PHY_MDIO 18
#define ETH_CLK_MODE ETH_CLOCK_GPIO0_IN
#define SHIELD_TYPE "ETH_PHY_LAN8720"
#include <ETH.h>

#define CONFIG_I2S_MCK_PIN GPIO_NUM_3
#define CONFIG_I2S_LRCK_PIN GPIO_NUM_12
#define CONFIG_I2S_BCK_PIN GPIO_NUM_14
#define CONFIG_I2S_DATA_OUT_PIN GPIO_NUM_15
#define CONFIG_I2S_DATA_IN_PIN GPIO_NUM_15
#define CONFIG_I2S_BUFFER_COUNT 8
#define CONFIG_I2S_PORT I2S_NUM_1

#define WEB_SERVER_LISTEN_PORT 80
#define WEB_SERVER_MIME_TYPE_JSON "application/json"

#define VBAN_PORT 6980

AsyncUDP udp;
WebServer web_server(WEB_SERVER_LISTEN_PORT);

DeviceConfig device_config;
DeviceStatus device_status;
AudioRingBuffer ring_buffer;

VbanReceiver *vban_receiver;
VbanTransmitter *vban_transmitter;

/*
 * I2S CONFIGURATION
 */
i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
    .sample_rate = 48000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL3 | ESP_INTR_FLAG_IRAM,
    .dma_buf_count = CONFIG_I2S_BUFFER_COUNT,
    .dma_buf_len = VBAN_SAMPLES_MAX_NB,
    .use_apll = true,
    .tx_desc_auto_clear = true,
};

i2s_pin_config_t i2s_pin_config = {
    .mck_io_num = CONFIG_I2S_MCK_PIN,
    .bck_io_num = CONFIG_I2S_BCK_PIN,
    .ws_io_num = CONFIG_I2S_LRCK_PIN,
    .data_out_num = CONFIG_I2S_DATA_OUT_PIN,
    .data_in_num = CONFIG_I2S_DATA_IN_PIN};
int i2s_enabled = 0;

void init_i2s(const i2s_mode_t &mode, const u_int32_t &sample_rate, const i2s_bits_per_sample_t &bits_per_sample)
{
  if (i2s_enabled == 0 ||
      mode != i2s_config.mode ||
      sample_rate != i2s_config.sample_rate ||
      bits_per_sample != i2s_config.bits_per_sample)
  {

    esp_err_t err;
    if (i2s_enabled)
    {
      err = i2s_driver_uninstall(CONFIG_I2S_PORT);
      i2s_enabled = 0;
    }

    if (mode & I2S_MODE_RX)
    {
      i2s_pin_config.data_out_num = -1;
      i2s_pin_config.data_in_num = CONFIG_I2S_DATA_IN_PIN;
      i2s_pin_config.mck_io_num = CONFIG_I2S_MCK_PIN;
    }
    if (mode & I2S_MODE_TX)
    {
      i2s_pin_config.data_out_num = CONFIG_I2S_DATA_OUT_PIN;
      i2s_pin_config.data_in_num = -1;
      i2s_pin_config.mck_io_num = CONFIG_I2S_MCK_PIN;
    }

    i2s_config.mode = mode;
    i2s_config.sample_rate = sample_rate;
    i2s_config.bits_per_sample = bits_per_sample;
    i2s_config.dma_buf_len = VBanNetQualitySampleSize[device_config.net_quality];

    err = i2s_driver_install(CONFIG_I2S_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK)
    {
      return;
    }
    err = i2s_set_pin(CONFIG_I2S_PORT, &i2s_pin_config);
    if (err != ESP_OK)
    {
      return;
    }
    i2s_enabled = 1;
  }
}

/*
 * VBAN CONFIGURATION
 */
StaticJsonDocument<2048> json;
/*
 * WEB SERVER CONFIGURATION
 */
void web_server_stats()
{
  device_status.get(json);

  String body;
  serializeJson(json, body);
  json.clear();

  web_server.send_P(200, WEB_SERVER_MIME_TYPE_JSON, body.c_str());
}

void (*resetFunc)(void) = 0;

void web_server_config_upload()
{
  if (web_server.args() == 0)
  {
    return;
  }
  String body = web_server.arg(0);

  deserializeJson(json, body.c_str());
  if (json.size() > 0)
  {
    web_server.send(200, WEB_SERVER_MIME_TYPE_JSON, (const char *)body.c_str());

    File file = SPIFFS.open(STORED_CONFIG_FILENAME, "w", true);
    file.write((const uint8_t *)body.c_str(), (size_t)body.length());

    file.flush();
    file.close();

    delay(200);
  }

  resetFunc();
}

void start_vban_receive()
{
  if (device_status.vban_enable)
  {
    web_server.send(200, "text/plain", "ALREADY ON");
    return;
  }
  device_status.vban_enable = 1;
  web_server.send(200, "text/plain", "OK");
}

void stop_vban_receive()
{
  if (!device_status.vban_enable)
  {
    web_server.send(200, "text/plain", "ALREADY OFF");
    return;
  }
  device_status.vban_enable = 0;
  device_status.errors.overrun = 0;
  device_status.errors.corrupt = 0;
  device_status.errors.disorder = 0;
  device_status.errors.missing = 0;
  device_status.errors.underrun = 0;
  web_server.send(200, "text/plain", "OK");
}

void begin_ota()
{
  device_status.vban_enable = 0;
}

/*
 * I2S FUNCTIONS
 */
void i2s_writer(void *parameter)
{
  const i2s_mode_t mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);

  while (true)
  {
    PcmSamples *buffer;
    while (ring_buffer.popRing(buffer))
    {
      device_status.errors.underrun = 0;

      init_i2s(mode, buffer->sample_rate, (i2s_bits_per_sample_t)buffer->bits_per_sample);

      size_t written_len = 0;
      esp_err_t err = i2s_write(CONFIG_I2S_PORT, buffer->data, buffer->len, &written_len, 100);
    }
    delay(1);
  }
}

void i2s_reader(void *parameter)
{
  const i2s_mode_t mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
  const uint32_t sample_rate = device_config.vban_transmitter.sample_rate;
  const i2s_bits_per_sample_t bits_per_sample = (i2s_bits_per_sample_t)device_config.vban_transmitter.bits_per_sample;

  init_i2s(mode, sample_rate, bits_per_sample);

  size_t expected_read = VBanNetQualitySampleSize[device_config.net_quality] * device_config.vban_transmitter.channels * (device_config.vban_transmitter.bits_per_sample / 8);
  size_t read_len = 0;

  while (true)
  {
    if (!device_status.vban_enable)
    {
      delay(1);
      continue;
    }

    if (i2s_enabled)
    {
      PcmSamples *buffer = ring_buffer.getNextBuffer();

      device_status.errors.corrupt = 0;

      esp_err_t err = i2s_read(CONFIG_I2S_PORT, buffer->data, expected_read, &read_len, 25);

      buffer->len = read_len;

      if (read_len > 0)
      {
        if (ring_buffer.pushRing(buffer))
        {
          device_status.errors.overrun = 1;
        }
      }
    }
    else
    {
      device_status.errors.corrupt = 1;
    }
  }
}

TaskHandle_t i2s_task;

bool ethernet_connected = false;

void EthEvent(WiFiEvent_t event)
{
  switch (event)
  {
  case ARDUINO_EVENT_ETH_START:
    // set eth hostname here
    ETH.setHostname("esp-vban");
    break;
  case ARDUINO_EVENT_ETH_CONNECTED:
    break;
  case ARDUINO_EVENT_ETH_GOT_IP:
    ethernet_connected = true;
    break;
  case ARDUINO_EVENT_ETH_DISCONNECTED:
    ethernet_connected = false;
    break;
  case ARDUINO_EVENT_ETH_STOP:
    ethernet_connected = false;
    break;
  default:
    break;
  }
}

void setup()
{
  if (SPIFFS.begin(true))
  {
    web_server.serveStatic("/", SPIFFS, "/index.html");
    web_server.serveStatic("/knockout.js", SPIFFS, "/knockout.js");
    web_server.serveStatic("/knockout.mapping.js", SPIFFS, "/knockout.mapping.js");

    web_server.serveStatic("/config", SPIFFS, "/config.json");

    web_server.on("/stats", web_server_stats);
    web_server.on("/config", HTTP_POST, web_server_config_upload);
    web_server.on("/start_vban_receive", start_vban_receive);
    web_server.on("/stop_vban_receive", stop_vban_receive);
  }

  device_config.load();

  WiFi.onEvent(EthEvent);

  ETH.setHostname("esp-vban");
  ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER);

  while (!ethernet_connected)
  {
    delay(10);
  }

  device_status.network.ip = ETH.localIP();
  device_status.network.mask = ETH.subnetCIDR();
  device_status.network.gateway = ETH.gatewayIP();
  device_status.network.dns = ETH.dnsIP();

  ElegantOTA.begin(&web_server);
  ElegantOTA.onStart(begin_ota);

  web_server.begin();

  if (device_config.operation_mode.receiver)
  {
    vban_receiver = new VbanReceiver(&device_config, &device_status, &ring_buffer);

    device_status.vban_enable = 1;
    if (vban_receiver->begin())
    {
      xTaskCreatePinnedToCore(
          i2s_writer,   /* Function to implement the task */
          "i2s_writer", /* Name of the task */
          1024,         /* Stack size in words */
          NULL,         /* Task input parameter */
          0,            /* Priority of the task */
          &i2s_task,    /* Task handle. */
          1);           /* Core where the task should run */
    }
  }
  else if (device_config.operation_mode.transmitter)
  {
    vban_transmitter = new VbanTransmitter(&device_config, &device_status, &ring_buffer);
    device_status.vban_enable = 1;
    if (vban_transmitter->begin())
    {
      xTaskCreatePinnedToCore(
          i2s_reader,   /* Function to implement the task */
          "i2s_reader", /* Name of the task */
          10000,        /* Stack size in words */
          NULL,         /* Task input parameter */
          0,            /* Priority of the task */
          &i2s_task,    /* Task handle. */
          1);           /* Core where the task should run */
    };
  }
}

void loop()
{
  delay(100);
  web_server.handleClient();
  ElegantOTA.loop();
}
