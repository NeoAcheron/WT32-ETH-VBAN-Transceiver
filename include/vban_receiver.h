
#pragma once

#ifndef __VBAN_RECEIVER_H__
#define __VBAN_RECEIVER_H__

#include "vban_common.h"

#include "device_config.h"
#include "device_status.h"

class VbanReceiver
{
private:
    DeviceConfig *device_config;
    DeviceStatus *device_status;
    AudioRingBuffer *ring_buffer;

    uint32_t nu_frame;
    const char *stream_name;

    AsyncUDP udp;

public:
    VbanReceiver(DeviceConfig *device_config, DeviceStatus *device_status, AudioRingBuffer *ring_buffer);

    ~VbanReceiver();

    bool begin();
    bool stop();

private:

    static void handlePacket(void* self, AsyncUDPPacket packet);
    void handlePacketData(u_int8_t* packet_date, size_t packet_size);
    void handleAudio(const VBanHeader *header_ptr, uint8_t *payload_ptr, size_t payload_size);

    bool checkHeader(const VBanHeader *header_ptr, size_t packet_size);
    bool checkAudio(const VBanHeader *header_ptr, size_t packet_size);
};

#endif
