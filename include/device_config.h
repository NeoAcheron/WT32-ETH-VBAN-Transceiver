
#ifndef __DEVICE_CONFIG_H__
#define __DEVICE_CONFIG_H__

#include <ArduinoJson.h>
#include <IPAddress.h>
#include <SPIFFS.h>

#define STORED_CONFIG_FILENAME "/config.json"
#define STORED_CONFIG_DEFAULTS_FILENAME "/defaults.json"
#define STORED_CONFIG_GPIO_RESET GPIO_NUM_32

typedef enum
{
    VBAN_NET_QUALITY_OPTIMAL,
    VBAN_NET_QUALITY_FAST,
    VBAN_NET_QUALITY_MEDIUM,
    VBAN_NET_QUALITY_SLOW,
    VBAN_NET_QUALITY_VERY_SLOW,
    VBAN_NET_QUALITY_MAX
} vban_net_quality_t;

static int const VBanNetQualitySampleSize[VBAN_BIT_RESOLUTION_MAX] =
{
    64, 32, 128, 192, 256
};

class DeviceConfig
{
public:
    uint16_t vban_port;
    vban_net_quality_t net_quality;
    String host_name;
    String user_name;

    struct
    {
        uint8_t transmitter : 1;
        uint8_t receiver : 1;
    } operation_mode;

    struct
    {
        IPAddress ip_address_to;
        String stream_name;
        uint32_t sample_rate;
        uint8_t channels;
        uint8_t bits_per_sample;
    } vban_transmitter;

    struct
    {
        IPAddress ip_address_from;
        String stream_name;
    } vban_receiver;


public:
    DeviceConfig();
    DeviceConfig(DeviceConfig &config);

    ~DeviceConfig();

    void load();
    void save();
    void reset();

    void get(JsonDocument& json);
    void set(JsonDocument& json);
};

#endif