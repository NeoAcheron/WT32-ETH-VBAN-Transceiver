#include <Arduino.h>

#include <driver/i2s.h>

#include <SPIFFS.h>

#include <ArduinoJson.h>
#include <ElegantOTA.h>

#include <ESPmDNS.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

#include "common.h"

#include "device_config.h"
#include "device_status.h"

#include "vban_receiver.h"
#include "vban_transmitter.h"
#include "i2s_processor.h"

#include "sigma_connect.h"
#include "denon_connect.h"

#define ETH_PHY_ADDR 1
#define ETH_PHY_TYPE ETH_PHY_LAN8720
#define ETH_PHY_POWER 16
#define ETH_PHY_MDC 23
#define ETH_PHY_MDIO 18
#define ETH_CLK_MODE ETH_CLOCK_GPIO0_IN
#define SHIELD_TYPE "ETH_PHY_LAN8720"
#include <ETH.h>

#define WEB_SERVER_LISTEN_PORT 80
#define WEB_SOCKET_SERVER_LISTEN_PORT 81
#define WEB_SERVER_MIME_TYPE_JSON "application/json"

WebServer web_server(WEB_SERVER_LISTEN_PORT);
WebSocketsServer web_socket_server(WEB_SOCKET_SERVER_LISTEN_PORT);

DeviceConfig device_config;
DeviceStatus device_status;
AudioRingBuffer ring_buffer;

static StaticJsonDocument<1024> system_status;
std::vector<AudioProcessor *> audio_processors;

static uint32_t start_all()
{
  uint32_t result;

  if (!audio_processors.empty())
  {
    return false;
  }
  ring_buffer.reset();

  if (device_config.operation_mode.receiver)
  {
    AudioProcessor *i2s_writer = new I2sWriter(&device_config, &device_status, &ring_buffer);
    AudioProcessor *vban_receiver = new VbanReceiver(&device_config, &device_status, &ring_buffer);

    audio_processors.push_back(i2s_writer);
    audio_processors.push_back(vban_receiver);
  }
  else if (device_config.operation_mode.transmitter)
  {
    AudioProcessor *vban_transmitter = new VbanTransmitter(&device_config, &device_status, &ring_buffer);
    AudioProcessor *i2s_reader = new I2sReader(&device_config, &device_status, &ring_buffer);

    audio_processors.push_back(vban_transmitter);
    audio_processors.push_back(i2s_reader);
  }
  device_status.vban_enable = 1;

  if (device_config.sigma_connect.enable_dsp)
  {
    AudioProcessor *sigma_tcp_server = new SigmaConnect(&device_config, &device_status, &ring_buffer);
    audio_processors.push_back(sigma_tcp_server);
  }

  if (device_config.denon_connect.enable_sync)
  {
    AudioProcessor *denon_connect = new DenonConnect(&device_config, &device_status, &ring_buffer);
    audio_processors.push_back(denon_connect);
  }

  boolean started_success = !audio_processors.empty();
  for (AudioProcessor *processor : audio_processors)
  {
    uint32_t result = processor->begin();
    TaskDef taskdef = processor->taskConfig();
    system_status[taskdef.pcName]["start_result"] = result;
    if (result != pdPASS)
    {
      started_success = false;
    }
  }
  device_status.processors_active = !audio_processors.empty();

  return result;
}

static uint32_t stop_all()
{
  device_status.vban_enable = 0;

  if (audio_processors.empty())
  {
    return 0;
  }
  ring_buffer.reset();

  for (AudioProcessor *processor : audio_processors)
  {
    processor->stop();
    delete processor;
  }
  audio_processors.clear();

  device_status.processors_active = !audio_processors.empty();

  return audio_processors.empty();
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

  system_status["available_heap_bytes"] = esp_get_free_heap_size();
  for (AudioProcessor *processor : audio_processors)
  {
    TaskDef taskdef = processor->taskConfig();
    if (processor->debug.size())
    {
      json["debug"][taskdef.pcName] = processor->debug;
    }
    system_status[taskdef.pcName]["allocated_stack_bytes"] = taskdef.usStackDepth;
    system_status[taskdef.pcName]["available_stack_bytes"] = uxTaskGetStackHighWaterMark(processor->task_handle);
  }

  if (system_status.size())
  {
    json["system_status"] = system_status;
  }

  char body[2048];
  serializeJson(json, body);
  json.clear();

  web_server.send_P(200, WEB_SERVER_MIME_TYPE_JSON, body);
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
    device_config.set(json);

    start_all();
    device_config.save();
    json.clear();

    device_config.get(json);

    String body;
    serializeJson(json, body);
    json.clear();

    web_server.send(200, WEB_SERVER_MIME_TYPE_JSON, (const char *)body.c_str());
  }
}

void web_server_toggle_vban()
{
  if (device_status.processors_active)
  {
    device_status.vban_enable = !device_status.vban_enable;
  }
  web_server.send(200, "text/plain", "OK");
}

void web_server_start_all()
{
  start_all();
  web_server.send(200, "text/plain", "OK");
}

void web_server_stop_all()
{
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

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{

  switch (type)
  {
  case WStype_DISCONNECTED:
    break;
  case WStype_CONNECTED:
  {
    IPAddress ip = web_socket_server.remoteIP(num);
  }
  break;
  case WStype_TEXT:
    // send message to client
    // webSocket.sendTXT(num, "message here");

    // send data to all connected clients
    // webSocket.broadcastTXT("message here");
    break;
  case WStype_BIN:
  case WStype_ERROR:
  case WStype_FRAGMENT_TEXT_START:
  case WStype_FRAGMENT_BIN_START:
  case WStype_FRAGMENT:
  case WStype_FRAGMENT_FIN:
    break;
  }
}

void setup()
{
  if (SPIFFS.begin(true))
  {
    web_server.serveStatic("/", SPIFFS, "/index.html");
    web_server.serveStatic("/knockout.js", SPIFFS, "/knockout.js"); // Knockout Base
    web_server.serveStatic("/knockout.mapping.js", SPIFFS, "/knockout.mapping.js"); // Knockout Mapping Plugin
    web_server.serveStatic("/struct.js", SPIFFS, "/struct.js"); // Struct unpacker

    web_server.serveStatic("/config", SPIFFS, "/config.json");
    web_server.on("/config", HTTP_POST, web_server_config_upload);

    web_server.on("/stats", web_server_stats);
    web_server.on("/toggle_vban", web_server_toggle_vban);

    web_server.on("/start", web_server_start_all);
    web_server.on("/stop", web_server_stop_all);
  }

  bool safe_to_start = device_config.load();

  WiFi.onEvent(EthEvent);

  char *hostname = (char *)malloc(device_config.host_name.length() + 1);
  strncpy(hostname, device_config.host_name.c_str(), device_config.host_name.length());

  ETH.setHostname(hostname);
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
  web_socket_server.begin();

  web_socket_server.onEvent(webSocketEvent);

  if (MDNS.begin(hostname))
  {
    MDNS.addService("http", "tcp", WEB_SERVER_LISTEN_PORT);
    MDNS.addService("ws", "tcp", WEB_SOCKET_SERVER_LISTEN_PORT);
    MDNS.addService("vban", "udp", device_config.vban_port);
  }

  if (safe_to_start)
  {
    start_all();
  }
}

uint32_t counter = 0;

void loop()
{
  delay(100);
  web_server.handleClient();
  ElegantOTA.loop();
  web_socket_server.loop();
  if (web_socket_server.connectedClients() && (counter++ % 10 == 0))
  {    
    int64_t time = esp_timer_get_time();
    if((time - device_status.vban_transmitter.last_packet_transmitted_timestamp) > 250000){
        device_status.vban_transmitter.active = 0;
        device_status.vban_transmitter.outgoing_stream = 0;
    }
    if((time - device_status.vban_receiver.last_packet_received_timestamp) > 250000){
        device_status.vban_receiver.active = 0;
        device_status.vban_receiver.incoming_stream = 0;
    }

    web_socket_server.broadcastBIN((const uint8_t *)&device_status, sizeof(DeviceStatus));
  }
}
