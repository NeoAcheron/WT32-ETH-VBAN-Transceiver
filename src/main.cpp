#include <Arduino.h>

#include <driver/i2s.h>
#include <AsyncUDP.h>

#include <SPIFFS.h>
#include <ElegantOTA.h>

#include <ArduinoJson.h>
#include "sigma_connect.h"

#include "device_config.h"
#include "device_status.h"

#include "common.h"
#include "vban_receiver.h"
#include "vban_transmitter.h"
#include "i2s_processor.h"

#define ETH_PHY_ADDR 1
#define ETH_PHY_TYPE ETH_PHY_LAN8720
#define ETH_PHY_POWER 16
#define ETH_PHY_MDC 23
#define ETH_PHY_MDIO 18
#define ETH_CLK_MODE ETH_CLOCK_GPIO0_IN
#define SHIELD_TYPE "ETH_PHY_LAN8720"
#include <ETH.h>

#define WEB_SERVER_LISTEN_PORT 80
#define WEB_SERVER_MIME_TYPE_JSON "application/json"

#define VBAN_PORT 6980

AsyncUDP udp;
WebServer web_server(WEB_SERVER_LISTEN_PORT);

DeviceConfig device_config;
DeviceStatus device_status;
AudioRingBuffer ring_buffer;

VbanReceiver *vban_receiver = NULL;
VbanTransmitter *vban_transmitter = NULL;

I2sWriter *i2s_writer = NULL;
I2sReader *i2s_reader = NULL;

StaticJsonDocument<2048> system_errors;

static uint32_t start_all()
{
  uint32_t result;

  if (device_config.operation_mode.receiver)
  {
    if (i2s_writer == NULL)
    {
      i2s_writer = new I2sWriter(&device_config, &device_status, &ring_buffer);
    }
    result = i2s_writer->begin();
    if (result != pdPASS)
    {
      system_errors["i2s_writer"]["start_result"] = result;
    }

    if (vban_receiver == NULL)
    {
      vban_receiver = new VbanReceiver(&device_config, &device_status, &ring_buffer);
    }
    result = vban_receiver->begin();
    if (result != pdPASS)
    {
      system_errors["vban_receiver"]["start_result"] = result;
    }

    device_status.vban_enable = 1;
  }
  else if (device_config.operation_mode.transmitter)
  {
    if (vban_transmitter == NULL)
    {
      vban_transmitter = new VbanTransmitter(&device_config, &device_status, &ring_buffer);
    }
    result = vban_transmitter->begin();
    if (result != pdPASS)
    {
      system_errors["vban_transmitter"]["start_result"] = result;
    }

    if (i2s_reader == NULL)
    {
      i2s_reader = new I2sReader(&device_config, &device_status, &ring_buffer);
    }
    result = i2s_reader->begin();
    if (result != pdPASS)
    {
      system_errors["i2s_reader"]["start_result"] = result;
    }

    device_status.vban_enable = 1;
  }

  return result;
}

static uint32_t stop_all()
{
  device_status.vban_enable = 0;
  ring_buffer.reset();

  if (i2s_reader != NULL)
  {
    i2s_reader->stop();
  }

  if (vban_receiver != NULL)
  {
    vban_receiver->stop();
  }

  if (i2s_writer != NULL)
  {
    i2s_writer->stop();
  }

  if (vban_transmitter != NULL)
  {
    vban_transmitter->stop();
  }

  vTaskDelay(100 / portTICK_PERIOD_MS);

  delete vban_receiver;
  delete vban_transmitter;
  delete i2s_writer;
  delete i2s_reader;

  vban_receiver = NULL;
  vban_transmitter = NULL;
  i2s_writer = NULL;
  i2s_reader = NULL;

  return 0;
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
  if (system_errors.size())
  {
    json["system_errors"] = system_errors;
  }

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
  bool reboot_needed = false;

  deserializeJson(json, body.c_str());
  if (json.size() > 0)
  {
    stop_all();

    File file = SPIFFS.open(STORED_CONFIG_FILENAME, "w", true);
    file.write((const uint8_t *)body.c_str(), (size_t)body.length());

    file.flush();
    file.close();

    String operation_mode_string = json["operation_mode"];
    if (operation_mode_string.equals("receiver") && device_config.operation_mode.transmitter)
    {
      reboot_needed = true;
    }
    else if (operation_mode_string.equals("transmitter") && device_config.operation_mode.receiver)
    {
      reboot_needed = true;
    }

    device_config.set(json);

    web_server.send(200, WEB_SERVER_MIME_TYPE_JSON, (const char *)body.c_str());

    if (reboot_needed)
    {
      resetFunc();
    }
    else
    {
      start_all();
    }
  }
}

void start_vban_receive()
{
  if (device_status.vban_enable)
  {
    web_server.send(200, "text/plain", "ALREADY ON");
    return;
  }
  start_all();
  web_server.send(200, "text/plain", "OK");
}

void stop_vban_receive()
{
  if (!device_status.vban_enable)
  {
    web_server.send(200, "text/plain", "ALREADY OFF");
    return;
  }
  stop_all();
  web_server.send(200, "text/plain", "OK");
}

void begin_ota()
{
  device_status.vban_enable = 0;
}

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

  start_all();
}

void loop()
{
  delay(100);
  web_server.handleClient();
  ElegantOTA.loop();
}
