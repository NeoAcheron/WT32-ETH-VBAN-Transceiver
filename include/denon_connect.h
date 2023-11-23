
#ifndef __DENON_CONNECT_H__
#define __DENON_CONNECT_H__

#include <Arduino.h>
#include <WiFi.h>
#include "common.h"

#define DENON_TCP_PORT 22

#define DENON_CONNECT_QUERY_POWER_STATUS "PW?\r"
#define DENON_CONNECT_RESPONSE_POWER_STANDBY "PWSTANDBY"
#define DENON_CONNECT_RESPONSE_POWER_ON "PWON"

class DenonConnect : public AudioProcessor
{
private: 

    WiFiClient denon_connection;
    u_int8_t buf[100];

public:
    DenonConnect(DeviceConfig *device_config, DeviceStatus *device_status, AudioRingBuffer *ring_buffer);

    ~DenonConnect();

private:

    TaskDef taskConfig() override;
    bool init() override;
    bool handle() override;
};


#endif