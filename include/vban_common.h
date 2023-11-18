
#pragma once

#ifndef __VBAN_COMMON_H__
#define __VBAN_COMMON_H__

extern "C"
{
#include "vban.h"
}

#include <stdint.h>
#include <RingBuf.h>
#include <AsyncUDP.h>

#define CONFIG_RING_BUFFER_COUNT 8

/** Return VBanHeader pointer from buffer */
#define CPP_PACKET_HEADER_PTR(_buffer) ((struct VBanHeader *)_buffer)

/** Return payload pointer of a Vban packet from buffer pointer */
#define CPP_PACKET_PAYLOAD_PTR(_buffer) ((u_int8_t *)(CPP_PACKET_HEADER_PTR(_buffer) + 1))

/** Return paylod size from total packet size */
#define CPP_PACKET_PAYLOAD_SIZE(_size) (_size - sizeof(struct VBanHeader))

/*
 * RING BUFFER CONFIGURATION
 *
 * If we wait for the I2S operations in the same thread as the packet thread, things start getting laggy.
 * Luckily the ESP32 has 2 cores, one is mostly unused, so we can shedule the heavy lifting between the two cores. For this we need a ring buffer.
 * Populate the buffer from one thread and let the other thread take items out of the buffer.
 * We pre-create the buffer objects in order to save on alloc/dealloc calls.
 */
struct PcmSamples
{
  uint8_t data[VBAN_PROTOCOL_MAX_SIZE];
  size_t len;
  u_int32_t sample_rate;
  u_int8_t bits_per_sample;
};

class AudioRingBuffer
{
public:
  AudioRingBuffer();
  ~AudioRingBuffer();

  PcmSamples* getNextBuffer();
  bool popRing(PcmSamples* &pcm_samples);
  bool pushRing(PcmSamples* pcm_samples);
  void reset();

private:
  
  RingBuf<PcmSamples*, CONFIG_RING_BUFFER_COUNT> ring_buffer;
  PcmSamples buffered_samples[CONFIG_RING_BUFFER_COUNT];
  uint32_t ring_buffer_position;
};

#endif
