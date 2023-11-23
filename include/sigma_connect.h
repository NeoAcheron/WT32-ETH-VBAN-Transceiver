
#ifndef __SIGMA_CONNECT_H__
#define __SIGMA_CONNECT_H__

#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <SigmaDSP.h>
#include "common.h"

#define SIGMA_TCP_PORT 8086
#define SIGMA_TCP_BUFFER_SIZE 1500
#define SIGMA_TCP_COMMAND_READ 0x0a
#define SIGMA_TCP_COMMAND_READRESPONSE 0x0b
#define SIGMA_TCP_COMMAND_WRITE 0x09

#define DSP_I2C_ADDRESS (0x68 >> 1) & 0xFE
#define EEPROM_I2C_ADDRESS (0xa0 >> 1) & 0xFE

class SigmaConnect : public AudioProcessor
{
private:
    WiFiServer sigma_tcp_server;
    SigmaDSP *dsp;
    u_int8_t buf[1500];

    bool mute;

public:
    SigmaConnect(DeviceConfig *device_config, DeviceStatus *device_status, AudioRingBuffer *ring_buffer);

    ~SigmaConnect();

private:

    TaskDef taskConfig() override;
    bool init() override;
    bool handle() override;
};

#endif