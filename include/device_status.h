
#ifndef __DEVICE_STATUS_H__
#define __DEVICE_STATUS_H__

#include <ArduinoJson.h>
#include <IPAddress.h>
#include <SPIFFS.h>

#pragma pack (push, 1)
class DeviceStatus
{
public:
    bool processors_active;
    bool vban_enable;
    
    struct
    {
        IPAddress ip;
        uint8_t mask;
        IPAddress gateway;
        IPAddress dns;
    } network;

    struct
    {
        bool active;
        bool outgoing_stream;

        int64_t last_packet_transmitted_timestamp;
    } vban_transmitter;


    struct
    {
        bool active;
        bool incoming_stream;
        uint8_t channels;
        uint8_t bits_per_sample;        
        uint32_t sample_rate;
        int64_t last_packet_received_timestamp;
    } vban_receiver;

    struct
    {
        bool client_connected;
        bool dac_mute;
    } sigma_connect;

    struct
    {
        bool client_connected;
        char power_state[16];
    } denon_connect;

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
    ~DeviceStatus();

    void get(JsonDocument &json);
};
#pragma pack(pop)

#endif