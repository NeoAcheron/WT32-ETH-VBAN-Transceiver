#include "vban_receiver.h"

VbanReceiver::VbanReceiver(DeviceConfig *device_config, DeviceStatus *device_status, AudioRingBuffer *ring_buffer) : AudioProcessor(device_config, device_status, ring_buffer)
{
    nu_frame = 0;

    // Set up the fixed pointers to the incoming packet buffer
    packet_payload = VBAN_PROTOCOL_PACKET_PAYLOAD_PTR(packet_buffer);
    packet_header = VBAN_PROTOCOL_PACKET_HEADER_PTR(packet_buffer);
};

VbanReceiver::~VbanReceiver()
{
    udp.stop();
};

TaskDef VbanReceiver::taskConfig()
{
    TaskDef task_config = {
        .pcName = "VBanReceiver",
        .usStackDepth = 4096,
        .uxPriority = 1,
        .xCoreID = 0};
    return task_config;
}

bool VbanReceiver::init()
{
    strncpy(stream_name, device_config->vban_receiver.stream_name.c_str(), 16);
    bool success = udp.begin(device_config->vban_port);

    return success;
}

bool VbanReceiver::handle()
{
    size_t packet_size = udp.parsePacket();

    if (packet_size == 0 || packet_size > VBAN_PROTOCOL_MAX_SIZE)
    {
        return false;
    }

    udp.read(packet_buffer, packet_size);

    if (!this->checkHeader(packet_size))
        return false;

    const VBanProtocol protocol = (VBanProtocol)(packet_header->format_SR & VBAN_PROTOCOL_MASK);

    switch (protocol)
    {
    case VBAN_PROTOCOL_AUDIO:
        if (this->checkAudio(packet_size))
        {
            handleAudio(VBAN_PROTOCOL_PACKET_PAYLOAD_SIZE(packet_size));
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
    return true;
}

bool VbanReceiver::checkHeader(size_t packet_size)
{
    if (packet_header->vban != VBAN_HEADER_FOURC)
    {
        return false;
    }

    if (packet_size <= VBAN_HEADER_SIZE)
    {
        return false;
    }

    /** check the reserved bit : it must be 0 */
    if (packet_header->format_bit & VBAN_RESERVED_MASK)
    {
        return false;
    }
    return true;
}

bool VbanReceiver::checkAudio(size_t packet_size)
{
    const VBanCodec codec = (VBanCodec)(packet_header->format_bit & VBAN_CODEC_MASK);
    if (codec != VBAN_CODEC_PCM)
    {
        device_status->errors.corrupt = 1;
        return false;
    }

    const VBanBitResolution bit_resolution = (VBanBitResolution)(packet_header->format_bit & VBAN_BIT_RESOLUTION_MASK);
    int const sample_rate = packet_header->format_SR & VBAN_SR_MASK;
    int const nb_samples = packet_header->format_nbs + 1;
    int const nb_channels = packet_header->format_nbc + 1;

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

    if (strncmp(stream_name, packet_header->streamname, 16) != 0)
    {
        return false;
    };

    if (packet_header->nuFrame != 0 && nu_frame >= packet_header->nuFrame)
    {
        device_status->errors.disorder = 1;
        return false;
    }
    else if ((packet_header->nuFrame - nu_frame) > 1)
    {
        device_status->errors.missing = 1;
    }
    else if (packet_header->nuFrame != 0)
    {
        device_status->errors.disorder = 0;
        device_status->errors.missing = 0;
    }
    nu_frame = packet_header->nuFrame;
    device_status->errors.corrupt = 0;

    return true;
}

void VbanReceiver::handleAudio(size_t payload_size)
{
    PcmSamples *buffer = ring_buffer->getNextBuffer();
    const uint32_t sample_rate = VBanSRList[packet_header->format_SR & VBAN_SR_MASK];
    const uint8_t bits_per_sample = VBanBitResolutionSize[packet_header->format_bit & VBAN_BIT_RESOLUTION_MASK] * 8;

    buffer->len = payload_size;
    buffer->sample_rate = sample_rate;
    buffer->bits_per_sample = bits_per_sample;
    memcpy(buffer->data, packet_payload, payload_size);

    if (ring_buffer->pushRing(buffer))
    {
        device_status->vban_receiver.active = 1;
        device_status->errors.overrun = 0;
    }
    else
    {
        device_status->errors.overrun = 1;
    }
    device_status->vban_receiver.sample_rate = sample_rate;
    device_status->vban_receiver.bits_per_sample = bits_per_sample;
    device_status->vban_receiver.channels = packet_header->format_nbc + 1;
}