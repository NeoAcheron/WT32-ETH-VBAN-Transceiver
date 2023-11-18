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

VbanTransmitter::VbanTransmitter(DeviceConfig *device_config, DeviceStatus *device_status, AudioRingBuffer *ring_buffer) : device_config(device_config),
                                                                                                                           device_status(device_status),
                                                                                                                           ring_buffer(ring_buffer),
                                                                                                                           stream_name(device_config->vban_receiver.stream_name.c_str())
{
}

VbanTransmitter::~VbanTransmitter() {}

bool VbanTransmitter::begin()
{
    packet_payload = CPP_PACKET_PAYLOAD_PTR(packet_buffer);
    packet_header = CPP_PACKET_HEADER_PTR(packet_buffer);

    packet_header->vban = VBAN_HEADER_FOURC;
    packet_header->format_nbc = device_config->vban_transmitter.channels - 1;
    packet_header->format_SR = sr_from_value(device_config->vban_transmitter.sample_rate);
    packet_header->format_bit = (VBanBitResolution)((device_config->vban_transmitter.bits_per_sample / 8) - 1);
    strncpy(packet_header->streamname, stream_name, VBAN_STREAM_NAME_SIZE - 1);

    packet_header->nuFrame = 0;

    sample_size = ((device_config->vban_transmitter.channels) * VBanBitResolutionSize[(packet_header->format_bit & VBAN_BIT_RESOLUTION_MASK)]);


    if (udp.connect(device_config->vban_transmitter.ip_address_to, device_config->vban_port))
    {
        xTaskCreatePinnedToCore(
            this->loop,                       /* Function to implement the task */
            "vban_transmit",                  /* Name of the task */
            10000,                            /* Stack size in words */
            this,                             /* Task input parameter */
            CONFIG_ARDUINO_UDP_TASK_PRIORITY, /* Priority of the task */
            &task,                            /* Task handle. */
            0);                               /* Core where the task should run */

        return true;
    }
    return false;
}

void VbanTransmitter::stop()
{
    task = 0;
}

void VbanTransmitter::loop(void *parameter)
{
    VbanTransmitter *self = (VbanTransmitter *)parameter;
    while (true)
    {
        if (self->device_status->vban_enable)
        {
            self->handleAudio();
        }
        delay(1);
    }
}

void VbanTransmitter::handleAudio()
{
    PcmSamples *buffer;
    while (ring_buffer->popRing(buffer))
    {
        memcpy(packet_payload, buffer->data, buffer->len);

        packet_header->format_nbs = (buffer->len / sample_size) - 1;
        packet_header->nuFrame++;

        udp.write((const uint8_t *)packet_buffer, buffer->len + VBAN_HEADER_SIZE);
        device_status->vban_transmitter.last_packet_transmitted_timestamp = millis();
        device_status->vban_transmitter.active = 1;
    }
}