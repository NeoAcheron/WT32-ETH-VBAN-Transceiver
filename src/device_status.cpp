#include "device_status.h"

DeviceStatus::DeviceStatus(){};

DeviceStatus::DeviceStatus(DeviceStatus &config)
{
    this->vban_enable = config.vban_enable;

    this->network.ip = config.network.ip;
    this->network.mask = config.network.mask;
    this->network.gateway = config.network.gateway;
    this->network.dns = config.network.dns;

    this->vban_transmitter.active = config.vban_transmitter.active;
    this->vban_transmitter.outgoing_stream = config.vban_transmitter.outgoing_stream;

    this->vban_receiver.active = config.vban_receiver.active;
    this->vban_receiver.incoming_stream = config.vban_receiver.incoming_stream;
    this->vban_receiver.sample_rate = config.vban_receiver.sample_rate;
    this->vban_receiver.channels = config.vban_receiver.channels;
    this->vban_receiver.bits_per_sample = config.vban_receiver.bits_per_sample;

    this->errors.overrun = config.errors.overrun;
    this->errors.corrupt = config.errors.corrupt;
    this->errors.disorder = config.errors.disorder;
    this->errors.missing = config.errors.missing;
    this->errors.underrun = config.errors.underrun;
};

DeviceStatus::~DeviceStatus(){};

void DeviceStatus::get(JsonDocument &json)
{
    json["vban_enable"] = this->vban_enable;

    int32_t time = millis();

    json["network"]["ip"] = this->network.ip.toString();
    json["network"]["mask"] = this->network.mask;
    json["network"]["gateway"] = this->network.gateway.toString();
    json["network"]["dns"] = this->network.dns.toString();

    if((time - this->vban_transmitter.last_packet_transmitted_timestamp) > 250){
        this->vban_transmitter.active = 0;
        this->vban_transmitter.outgoing_stream = 0;
    }
    json["vban_transmitter"]["active"] = this->vban_transmitter.active;
    json["vban_transmitter"]["outgoing_stream"] = this->vban_transmitter.outgoing_stream;

    if((time - this->vban_receiver.last_packet_received_timestamp) > 250){
        this->vban_receiver.active = 0;
        this->vban_receiver.incoming_stream = 0;
    }
    json["vban_receiver"]["active"] = this->vban_receiver.active;
    json["vban_receiver"]["incoming_stream"] = this->vban_receiver.incoming_stream;
    json["vban_receiver"]["sample_rate"] = this->vban_receiver.sample_rate;
    json["vban_receiver"]["channels"] = this->vban_receiver.channels;
    json["vban_receiver"]["bits_per_sample"] = this->vban_receiver.bits_per_sample;

    json["errors"]["overrun"] = this->errors.overrun;
    json["errors"]["corrupt"] = this->errors.corrupt;
    json["errors"]["disorder"] = this->errors.disorder;
    json["errors"]["missing"] = this->errors.missing;
    json["errors"]["underrun"] = this->errors.underrun;
}