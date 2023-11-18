#include "vban_common.h"

AudioRingBuffer::AudioRingBuffer()
{
    ring_buffer_position = 0;
}

AudioRingBuffer::~AudioRingBuffer()
{
    ring_buffer.clear();
}

PcmSamples *AudioRingBuffer::getNextBuffer()
{
    return &(buffered_samples[ring_buffer_position++ % CONFIG_RING_BUFFER_COUNT]);
}

bool AudioRingBuffer::popRing(PcmSamples *&pcm_samples)
{
    return this->ring_buffer.pop(pcm_samples);
}

bool AudioRingBuffer::pushRing(PcmSamples *pcm_samples)
{
    return this->ring_buffer.pushOverwrite(pcm_samples);
}

void AudioRingBuffer::reset()
{
    ring_buffer_position = 0;
    ring_buffer.clear();
}
