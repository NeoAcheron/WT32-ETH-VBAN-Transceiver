#include "vban_transmitter.h"

/** should better be in vban.h header ?*/
size_t sr_from_value(unsigned int value)
{
    size_t index = 0;
    while ((index < VBAN_SR_MAXNUMBER) && (value != VBanSRList[index]))
    {
        ++index;
    }

    return index;
}

VbanTransmitter::VbanTransmitter(DeviceConfig *device_config, DeviceStatus *device_status, AudioRingBuffer *ring_buffer) : AudioProcessor(device_config, device_status, ring_buffer)
{
    sample_size = 0;
    packet_payload = VBAN_PROTOCOL_PACKET_PAYLOAD_PTR(packet_buffer);
    packet_header = VBAN_PROTOCOL_PACKET_HEADER_PTR(packet_buffer);
}

VbanTransmitter::~VbanTransmitter()
{
    udp.stop();
}

TaskDef VbanTransmitter::taskConfig()
{
    TaskDef task_config = {
        .pcName = "vban_transmitter",
        .usStackDepth = 2048,
        .uxPriority = 1,
        .xCoreID = 0,
        .task_delay_ms = 1};
    return task_config;
}

bool VbanTransmitter::init()
{
    packet_header->vban = VBAN_HEADER_FOURC;
    packet_header->format_nbc = device_config->vban_transmitter.channels - 1;
    packet_header->format_SR = sr_from_value(device_config->vban_transmitter.sample_rate);
    packet_header->format_bit = (VBanBitResolution)((device_config->vban_transmitter.bits_per_sample / 8) - 1);
    packet_header->nuFrame = 0;
    strncpy(packet_header->streamname, device_config->vban_transmitter.stream_name.c_str(), VBAN_STREAM_NAME_SIZE - 1);

    sample_size = ((device_config->vban_transmitter.channels) * VBanBitResolutionSize[(packet_header->format_bit & VBAN_BIT_RESOLUTION_MASK)]);

    return udp.begin(device_config->vban_port);
}

bool VbanTransmitter::handle()
{
    PcmSamples *buffer;
    while (ring_buffer->popRing(buffer))
    {
        memcpy(packet_payload, buffer->data, buffer->len);

        packet_header->format_nbs = (buffer->len / sample_size) - 1;
        if (packet_header->nuFrame == 0xFFFFFFFF)
        {
            packet_header->nuFrame = 0;
        }

        udp.beginPacket(device_config->vban_transmitter.ip_address_to, device_config->vban_port);
        udp.write((const uint8_t *)packet_buffer, buffer->len + VBAN_HEADER_SIZE);
        udp.endPacket();

        device_status->vban_transmitter.last_packet_transmitted_timestamp = esp_timer_get_time();
        device_status->vban_transmitter.active = 1;

        ++packet_header->nuFrame;
    }
    return true;
}