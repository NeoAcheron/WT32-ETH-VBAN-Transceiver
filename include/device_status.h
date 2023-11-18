
#ifndef __DEVICE_STATUS_H__
#define __DEVICE_STATUS_H__

#include <ArduinoJson.h>
#include <IPAddress.h>
#include <SPIFFS.h>

class DeviceStatus
{
public:
    uint8_t vban_enable;
    struct
    {
        uint8_t active : 1;
        uint8_t outgoing_stream : 1;

        int32_t last_packet_transmitted_timestamp;
    } vban_transmitter;

    struct
    {
        IPAddress ip;
        uint8_t mask;
        IPAddress gateway;
        IPAddress dns;
    } network;

    struct
    {
        uint8_t active;
        uint8_t incoming_stream;
        uint32_t sample_rate;
        uint8_t channels;
        uint8_t bits_per_sample;

        int32_t last_packet_received_timestamp;
    } vban_receiver;

    struct
    {
        uint8_t overrun : 1;
        uint8_t corrupt : 1;
        uint8_t disorder : 1;
        uint8_t missing : 1;
        uint8_t underrun : 1;
    } errors;

public:
    DeviceStatus();
    DeviceStatus(DeviceStatus &config);

    ~DeviceStatus();

    void get(JsonDocument &json);
};

#endif