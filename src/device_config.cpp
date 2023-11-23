#include "device_config.h"

DeviceConfig::DeviceConfig(){};
DeviceConfig::~DeviceConfig(){};

void convertFromJson(JsonVariantConst src, IPAddress &dst)
{
    dst.fromString(src.as<const char *>());
}

bool convertToJson(const IPAddress &src, JsonVariant dst)
{
    return dst.set(src.toString());
}

bool DeviceConfig::load()
{
    pinMode(STORED_CONFIG_GPIO_RESET, INPUT_PULLUP);
    bool factory_reset = digitalRead(STORED_CONFIG_GPIO_RESET) == 0;

    File config_file = SPIFFS.open(STORED_CONFIG_FILENAME, "r", false);
    if (factory_reset || (config_file.available() == 0))
    {
        config_file.close();
        config_file = SPIFFS.open(STORED_CONFIG_DEFAULTS_FILENAME, "r", false);

        factory_reset = true;
    }
    StaticJsonDocument<2048> json;
    deserializeJson(json, config_file);

    this->set(json);

    config_file.close();

    if (factory_reset)
    {
        this->save();
    }
    return !factory_reset;
}

void DeviceConfig::save()
{
    StaticJsonDocument<2048> json;

    this->get(json);

    File config_file = SPIFFS.open(STORED_CONFIG_FILENAME, "w", true);
    serializeJsonPretty(json, config_file);
    config_file.close();
}

void DeviceConfig::reset()
{
    bool success = SPIFFS.remove(STORED_CONFIG_FILENAME);
    this->load();
}

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

    JsonVariant config = json.createNestedObject("vban_transmitter");
    config["ip_address_to"] = this->vban_transmitter.ip_address_to.toString();
    config["stream_name"] = this->vban_transmitter.stream_name;
    config["sample_rate"] = this->vban_transmitter.sample_rate;
    config["channels"] = this->vban_transmitter.channels;
    config["bits_per_sample"] = this->vban_transmitter.bits_per_sample;

    config = json.createNestedObject("vban_receiver");
    config["ip_address_from"] = this->vban_receiver.ip_address_from.toString();
    config["stream_name"] = this->vban_receiver.stream_name;

    config = json.createNestedObject("sigma_connect");
    config["enable_dsp"] = this->sigma_connect.enable_dsp;
    config["sigma_tcpi_server_enable"] = this->sigma_connect.sigma_tcpi_server_enable;
    config["dsp_auto_power_on_vban"] = this->sigma_connect.dsp_auto_power_on_vban;
    config["dsp_i2c_sda_pin"] = this->sigma_connect.dsp_i2c_sda_pin;
    config["dsp_i2c_scl_pin"] = this->sigma_connect.dsp_i2c_scl_pin;
    config["dsp_reset_pin"] = this->sigma_connect.dsp_reset_pin;

    config = json.createNestedObject("denon_connect");
    config["enable_sync"] = this->denon_connect.enable_sync;
    config["receiver_ip_address"] = this->denon_connect.receiver_ip_address.toString();
    config["receiver_port"] = this->denon_connect.receiver_port;
}

void DeviceConfig::set(JsonDocument &json)
{
    this->vban_port = json["vban_port"];
    this->net_quality = json["net_quality"];
    this->host_name = json["host_name"].as<String>();
    this->user_name = json["user_name"].as<String>();

    String operation_mode_string = json["operation_mode"].as<String>();
    this->operation_mode.receiver = 0;
    this->operation_mode.transmitter = 0;
    if (operation_mode_string.equals("receiver"))
    {
        this->operation_mode.receiver = 1;
    }
    else if (operation_mode_string.equals("transmitter"))
    {
        this->operation_mode.transmitter = 1;
    }

    JsonObject config = json["vban_transmitter"];
    this->vban_transmitter = {
        .ip_address_to = config["ip_address_to"].as<IPAddress>(),
        .stream_name = config["stream_name"].as<String>(),
        .sample_rate = config["sample_rate"],
        .channels = config["channels"],
        .bits_per_sample = config["bits_per_sample"],
    };

    config = json["vban_receiver"];
    this->vban_receiver = {
        .ip_address_from = config["ip_address_from"].as<IPAddress>(),
        .stream_name = config["stream_name"].as<String>(),
    };

    config = json["sigma_connect"];
    this->sigma_connect = {
        .enable_dsp = config["enable_dsp"],
        .sigma_tcpi_server_enable = config["sigma_tcpi_server_enable"],
        .dsp_auto_power_on_vban = config["dsp_auto_power_on_vban"],
        .dsp_i2c_sda_pin = config["dsp_i2c_sda_pin"],
        .dsp_i2c_scl_pin = config["dsp_i2c_scl_pin"],
        .dsp_reset_pin = config["dsp_reset_pin"],
    };

    config = json["denon_connect"];
    this->denon_connect = {
        .enable_sync = config["enable_sync"],
        .receiver_ip_address = config["receiver_ip_address"].as<IPAddress>(),
        .receiver_port = config["receiver_port"],
    };
}
