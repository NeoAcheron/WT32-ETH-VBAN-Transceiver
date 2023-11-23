#include "denon_connect.h"

DenonConnect::DenonConnect(DeviceConfig *device_config, DeviceStatus *device_status, AudioRingBuffer *ring_buffer) : AudioProcessor(device_config, device_status, ring_buffer)
{
}

DenonConnect::~DenonConnect()
{
    denon_connection.stop();
}

bool DenonConnect::init()
{
    denon_connection.stop();
    return device_config->denon_connect.enable_sync;
}

bool DenonConnect::handle()
{
    if (!denon_connection.connected())
    {
        if (denon_connection.connect(device_config->denon_connect.receiver_ip_address, device_config->denon_connect.receiver_port))
        {
            denon_connection.setNoDelay(true);
            denon_connection.write(DENON_CONNECT_QUERY_POWER_STATUS);
        }
    }

    device_status->denon_connect.client_connected = denon_connection.connected();
    vTaskDelay(100);

    while (denon_connection.available())
    {
        String data = denon_connection.readStringUntil('\r');

        if (data.equals(DENON_CONNECT_RESPONSE_POWER_STANDBY))
        {
            strcpy(device_status->denon_connect.power_state, DENON_CONNECT_RESPONSE_POWER_STANDBY);
            device_status->vban_enable = 0;
        }
        else if (data.equals(DENON_CONNECT_RESPONSE_POWER_ON))
        {
            strcpy(device_status->denon_connect.power_state, DENON_CONNECT_RESPONSE_POWER_ON);
            device_status->vban_enable = 1;
        }
    }

    return true;
}

TaskDef DenonConnect::taskConfig()
{
    TaskDef task_config = {
        .pcName = "denon_connect",
        .usStackDepth = 2048,
        .uxPriority = 0,
        .xCoreID = 0,
        .task_delay_ms = 100};
    return task_config;
}