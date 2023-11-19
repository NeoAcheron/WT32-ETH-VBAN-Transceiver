
#pragma once

#ifndef __I2S_PROCESSOR_H__
#define __I2S_PROCESSOR_H__

#include <stdint.h>
#include <driver/i2s.h>

#include "common.h"

#define CONFIG_I2S_MCK_PIN GPIO_NUM_3
#define CONFIG_I2S_LRCK_PIN GPIO_NUM_12
#define CONFIG_I2S_BCK_PIN GPIO_NUM_14
#define CONFIG_I2S_DATA_OUT_PIN GPIO_NUM_15
#define CONFIG_I2S_DATA_IN_PIN GPIO_NUM_15
#define CONFIG_I2S_BUFFER_COUNT 8
#define CONFIG_I2S_PORT I2S_NUM_0

class I2sCommon
{
public:
    i2s_port_t i2s_port;
    i2s_config_t i2s_config;
    i2s_pin_config_t i2s_pin_config;
    bool i2s_enabled;
    xQueueHandle i2s_event_queue;

public:
    I2sCommon();
    ~I2sCommon();

    void init_i2s(const i2s_mode_t &mode, const u_int32_t &sample_rate, const i2s_bits_per_sample_t &bits_per_sample, const vban_net_quality_t &vban_net_quality);
};

class I2sWriter : public I2sCommon, public AudioProcessor
{
private:
public:
    I2sWriter(DeviceConfig *device_config, DeviceStatus *device_status, AudioRingBuffer *ring_buffer);
    ~I2sWriter();

    TaskDef taskConfig() override;
    bool init() override;
    bool handle() override;
};

class I2sReader : public I2sCommon, public AudioProcessor
{
private:
    size_t expected_read;
    
public:
    I2sReader(DeviceConfig *device_config, DeviceStatus *device_status, AudioRingBuffer *ring_buffer);
    ~I2sReader();

    TaskDef taskConfig() override;
    bool init() override;
    bool handle() override;
};

#endif