
#pragma once

#ifndef __VBAN_TRANSMITTER_H__
#define __VBAN_TRANSMITTER_H__

#include <WiFiUdp.h>

#include "vban.h"
#include "common.h"

class VbanTransmitter : public AudioProcessor
{
private:
    uint32_t sample_size;

    WiFiUDP udp;

    uint8_t packet_buffer[VBAN_PROTOCOL_MAX_SIZE];
    VBanHeader *packet_header;
    uint8_t *packet_payload;

public:
    VbanTransmitter(DeviceConfig *device_config, DeviceStatus *device_status, AudioRingBuffer *ring_buffer);
    ~VbanTransmitter() override;

    TaskDef taskConfig() override;
    bool init() override;
    bool handle() override;
};

#endif