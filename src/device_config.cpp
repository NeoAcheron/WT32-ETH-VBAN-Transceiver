#include "device_config.h"

DeviceConfig::DeviceConfig(){};

DeviceConfig::DeviceConfig(DeviceConfig &config)
{
    this->vban_port = config.vban_port;
    this->net_quality = config.net_quality;
    this->host_name = config.host_name;
    this->user_name = config.user_name;

    this->operation_mode.receiver = config.operation_mode.receiver;
    this->operation_mode.transmitter = config.operation_mode.transmitter;

    this->vban_transmitter.ip_address_to = config.vban_transmitter.ip_address_to;
    this->vban_transmitter.stream_name = config.vban_transmitter.stream_name;
    this->vban_transmitter.sample_rate = config.vban_transmitter.sample_rate;
    this->vban_transmitter.channels = config.vban_transmitter.channels;
    this->vban_transmitter.bits_per_sample = config.vban_transmitter.bits_per_sample;

    this->vban_receiver.ip_address_from = config.vban_receiver.ip_address_from;
    this->vban_receiver.stream_name = config.vban_receiver.stream_name;
};

DeviceConfig::~DeviceConfig(){};

void DeviceConfig::load()
{
    pinMode(STORED_CONFIG_GPIO_RESET, INPUT_PULLUP);
    bool factory_reset = digitalRead(STORED_CONFIG_GPIO_RESET) == 0;

    File config_file = SPIFFS.open(STORED_CONFIG_FILENAME, "r", false);
    if (factory_reset || (config_file.available() == 0))
    {
        config_file.close();
        config_file = SPIFFS.open(STORED_CONFIG_DEFAULTS_FILENAME, "r", false);

        factory_reset = 1;
    }
    DynamicJsonDocument json(4096);
    deserializeJson(json, config_file);

    this->set(json);

    config_file.close();

    if(factory_reset){
        this->save();
    }
};

void DeviceConfig::save()
{
    DynamicJsonDocument json(4096);

    this->get(json);

    File config_file = SPIFFS.open(STORED_CONFIG_FILENAME, "w", true);
    serializeJsonPretty(json, config_file);
    config_file.close();
};

void DeviceConfig::reset()
{
    bool success = SPIFFS.remove(STORED_CONFIG_FILENAME);
    this->load();
};

void DeviceConfig::get(JsonDocument &json)
{
    json["vban_port"] = this->vban_port;
    json["net_quality"] = this->net_quality;
    json["host_name"] = this->host_name;
    json["user_name"] = this->user_name;

    if (this->operation_mode.receiver)
    {
        json["operation_mode"] = "receiver";
    }
    else
    {
        json["operation_mode"] = "transmitter";
    }

    json["vban_transmitter"]["ip_address_to"] = this->vban_transmitter.ip_address_to.toString();
    json["vban_transmitter"]["stream_name"] = this->vban_transmitter.stream_name;
    json["vban_transmitter"]["sample_rate"] = this->vban_transmitter.sample_rate;
    json["vban_transmitter"]["channels"] = this->vban_transmitter.channels;
    json["vban_transmitter"]["bits_per_sample"] = this->vban_transmitter.bits_per_sample;

    json["vban_receiver"]["ip_address_from"] = this->vban_receiver.ip_address_from.toString();
    json["vban_receiver"]["stream_name"] = this->vban_receiver.stream_name;
};

void DeviceConfig::set(JsonDocument &json)
{
    this->vban_port = json["vban_port"];
    this->net_quality = json["net_quality"];
    this->host_name = json["host_name"].as<String>();
    this->user_name = json["user_name"].as<String>();

    String operation_mode_string = json["operation_mode"];
    if (operation_mode_string.equals("receiver"))
    {
        this->operation_mode.receiver = 1;
    }
    else if(operation_mode_string.equals("transmitter"))
    {
        this->operation_mode.transmitter = 1;
    }

    this->vban_transmitter.ip_address_to.fromString(json["vban_transmitter"]["ip_address_to"].as<String>());
    this->vban_transmitter.stream_name = json["vban_transmitter"]["stream_name"].as<String>();
    this->vban_transmitter.sample_rate = json["vban_transmitter"]["sample_rate"];
    this->vban_transmitter.channels = json["vban_transmitter"]["channels"];
    this->vban_transmitter.bits_per_sample = json["vban_transmitter"]["bits_per_sample"];

    this->vban_receiver.ip_address_from.fromString(json["vban_receiver"]["ip_address_from"].as<String>());
    this->vban_receiver.stream_name = json["vban_receiver"]["stream_name"].as<String>();
};
