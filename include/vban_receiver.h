
#pragma once

#ifndef __VBAN_RECEIVER_H__
#define __VBAN_RECEIVER_H__

#include <WiFiUdp.h>

#include "vban.h"
#include "common.h"

class VbanReceiver : public AudioProcessor
{
private:
    uint32_t nu_frame;
    char stream_name[16];

    WiFiUDP udp;

    uint8_t packet_buffer[VBAN_PROTOCOL_MAX_SIZE];
    const VBanHeader* packet_header;
    const uint8_t* packet_payload;

public:
    VbanReceiver(DeviceConfig *device_config, DeviceStatus *device_status, AudioRingBuffer *ring_buffer);
    ~VbanReceiver();

    TaskDef taskConfig() override;
    bool init() override;
    bool handle() override;

private:
    void handleAudio(size_t payload_size);

    bool checkHeader(size_t packet_size);
    bool checkAudio(size_t packet_size);
};

#endif
