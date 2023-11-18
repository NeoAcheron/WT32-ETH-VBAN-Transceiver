#include "vban_receiver.h"

VbanReceiver::VbanReceiver(DeviceConfig *device_config, DeviceStatus *device_status, AudioRingBuffer *ring_buffer) : device_config(device_config),
                                                                                                                     device_status(device_status),
                                                                                                                     ring_buffer(ring_buffer),
                                                                                                                     stream_name(device_config->vban_receiver.stream_name.c_str()),
                                                                                                                     nu_frame(0){};

VbanReceiver::~VbanReceiver(){};

bool VbanReceiver::begin()
{    
    udp.onPacket(this->handlePacket, this);
    return udp.listen(device_status->network.ip, device_config->vban_port);
}

bool VbanReceiver::stop()
{
    udp.close();
    return false;
}

void VbanReceiver::handlePacket(void* self, AsyncUDPPacket packet)
{
    ((VbanReceiver* )self)->handlePacketData(packet.data(), packet.length());
}

void VbanReceiver::handlePacketData(uint8_t* packet_data, size_t packet_size)
{
    if (!device_status->vban_enable)
    {
        return;
    }

    const VBanHeader *header_ptr = CPP_PACKET_HEADER_PTR(packet_data);

    if (!this->checkHeader(header_ptr, packet_size))
        return;

    const VBanProtocol protocol = (VBanProtocol)(header_ptr->format_SR & VBAN_PROTOCOL_MASK);

    switch (protocol)
    {
    case VBAN_PROTOCOL_AUDIO:
        if (this->checkAudio(header_ptr, packet_size))
        {
            handleAudio(header_ptr, CPP_PACKET_PAYLOAD_PTR(packet_data), CPP_PACKET_PAYLOAD_SIZE(packet_size));
        }
        break;
    case VBAN_PROTOCOL_SERVICE:
    case VBAN_PROTOCOL_SERIAL:
    case VBAN_PROTOCOL_TXT:
    case VBAN_PROTOCOL_USER:
    case VBAN_PROTOCOL_UNDEFINED_1:
    case VBAN_PROTOCOL_UNDEFINED_2:
    case VBAN_PROTOCOL_UNDEFINED_3:
    default:
        break;
    }
}

bool VbanReceiver::checkHeader(const VBanHeader *header_ptr, size_t packet_size)
{
    if (header_ptr->vban != VBAN_HEADER_FOURC)
    {
        return false;
    }

    if (packet_size <= VBAN_HEADER_SIZE)
    {
        return false;
    }

    /** check the reserved bit : it must be 0 */
    if (header_ptr->format_bit & VBAN_RESERVED_MASK)
    {
        return false;
    }
    return true;
}

bool VbanReceiver::checkAudio(const VBanHeader *header_ptr, size_t packet_size)
{
    const VBanCodec codec = (VBanCodec)(header_ptr->format_bit & VBAN_CODEC_MASK);
    if (codec != VBAN_CODEC_PCM)
    {
        device_status->errors.corrupt = 1;
        return false;
    }

    const VBanBitResolution bit_resolution = (VBanBitResolution)(header_ptr->format_bit & VBAN_BIT_RESOLUTION_MASK);
    int const sample_rate = header_ptr->format_SR & VBAN_SR_MASK;
    int const nb_samples = header_ptr->format_nbs + 1;
    int const nb_channels = header_ptr->format_nbc + 1;

    if (bit_resolution >= VBAN_BIT_RESOLUTION_MAX)
    {
        device_status->errors.corrupt = 1;
        return false;
    }

    if (sample_rate >= VBAN_SR_MAXNUMBER)
    {
        device_status->errors.corrupt = 1;
        return false;
    }

    device_status->vban_receiver.last_packet_received_timestamp = millis();
    device_status->vban_receiver.incoming_stream = 1;

    size_t sample_size = VBanBitResolutionSize[bit_resolution];
    size_t payload_size = nb_samples * sample_size * nb_channels;

    if (payload_size != (packet_size - VBAN_HEADER_SIZE))
    {
        device_status->errors.corrupt = 1;
        return false;
    }

    if (strncmp(stream_name, header_ptr->streamname, 16))
    {
        return false;
    };

    if (header_ptr->nuFrame != 0 && nu_frame >= header_ptr->nuFrame)
    {
        device_status->errors.disorder = 1;
        return false;
    }
    else if ((header_ptr->nuFrame - nu_frame) > 1)
    {
        device_status->errors.missing = 1;
    }
    else if (header_ptr->nuFrame != 0)
    {
        device_status->errors.disorder = 0;
        device_status->errors.missing = 0;
    }
    nu_frame = header_ptr->nuFrame;
    device_status->errors.corrupt = 0;

    return true;
}

void VbanReceiver::handleAudio(const VBanHeader *header_ptr, uint8_t *payload_ptr, size_t payload_size)
{
    device_status->vban_receiver.sample_rate = VBanSRList[header_ptr->format_SR];
    device_status->vban_receiver.bits_per_sample = VBanBitResolutionSize[header_ptr->format_bit] * 8;
    device_status->vban_receiver.channels = header_ptr->format_nbc + 1;

    PcmSamples *buffer = ring_buffer->getNextBuffer();

    buffer->len = payload_size;

    memcpy(buffer->data, payload_ptr, payload_size);
    buffer->sample_rate = device_status->vban_receiver.sample_rate;
    buffer->bits_per_sample = device_status->vban_receiver.bits_per_sample;

    if (ring_buffer->pushRing(buffer))
    {
        device_status->vban_receiver.active = 1;
        device_status->errors.overrun = 0;
    }
    else
    {
        device_status->errors.overrun = 1;
    }
}
