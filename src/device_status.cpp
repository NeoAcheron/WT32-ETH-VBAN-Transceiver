#include "device_status.h"

DeviceStatus::DeviceStatus(){};
DeviceStatus::~DeviceStatus(){};

void DeviceStatus::get(JsonDocument &json)
{
    json["processors_active"] = this->processors_active;
    json["vban_enable"] = this->vban_enable;


    json["network"]["ip"] = this->network.ip.toString();
    json["network"]["mask"] = this->network.mask;
    json["network"]["gateway"] = this->network.gateway.toString();
    json["network"]["dns"] = this->network.dns.toString();

    json["vban_transmitter"]["active"] = this->vban_transmitter.active;
    json["vban_transmitter"]["outgoing_stream"] = this->vban_transmitter.outgoing_stream;

    json["vban_receiver"]["active"] = this->vban_receiver.active;
    json["vban_receiver"]["incoming_stream"] = this->vban_receiver.incoming_stream;
    json["vban_receiver"]["sample_rate"] = this->vban_receiver.sample_rate;
    json["vban_receiver"]["channels"] = this->vban_receiver.channels;
    json["vban_receiver"]["bits_per_sample"] = this->vban_receiver.bits_per_sample;
    
    json["sigma_connect"]["client_connected"] = this->sigma_connect.client_connected;
    json["sigma_connect"]["dac_mute"] = this->sigma_connect.dac_mute;
    
    json["denon_connect"]["client_connected"] = this->denon_connect.client_connected;
    json["denon_connect"]["power_state"] = this->denon_connect.power_state;

    json["errors"]["overrun"] = this->errors.overrun;
    json["errors"]["corrupt"] = this->errors.corrupt;
    json["errors"]["disorder"] = this->errors.disorder;
    json["errors"]["missing"] = this->errors.missing;
    json["errors"]["underrun"] = this->errors.underrun;
}