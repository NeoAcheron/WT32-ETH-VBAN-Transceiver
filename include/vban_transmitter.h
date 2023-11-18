
#pragma once

#ifndef __VBAN_TRANSMITTER_H__
#define __VBAN_TRANSMITTER_H__

#include "vban_common.h"

#include "device_config.h"
#include "device_status.h"

class VbanTransmitter
{
private:
    DeviceConfig *device_config;
    DeviceStatus *device_status;
    AudioRingBuffer *ring_buffer;

    const char* stream_name;

    TaskHandle_t task;

    uint8_t* packet_buffer[VBAN_PROTOCOL_MAX_SIZE];
    VBanHeader* packet_header;
    uint8_t* packet_payload;

    uint32_t sample_size;

    AsyncUDP udp;

public:
    VbanTransmitter(DeviceConfig *device_config, DeviceStatus *device_status, AudioRingBuffer *ring_buffer);

    ~VbanTransmitter();

    bool begin();
    void stop();

private:
    static void loop(void *parameter);
    void handleAudio();
};

#endif