#include "sigma_connect.h"

SigmaConnect::SigmaConnect(DeviceConfig *device_config, DeviceStatus *device_status, AudioRingBuffer *ring_buffer) : AudioProcessor(device_config, device_status, ring_buffer)
{
    dsp = new SigmaDSP(Wire, DSP_I2C_ADDRESS, 48000.0f, device_config->sigma_connect.dsp_reset_pin);
    u_int16_t buf_size = SIGMA_TCP_BUFFER_SIZE;
    mute = true; // Assume the DSP is muted at the start, in case it is, and we don't know
}

SigmaConnect::~SigmaConnect()
{
    if (device_config->sigma_connect.dsp_auto_power_on_vban)
    {
        dsp->muteDAC(true);
    }
    delete dsp;

    Wire.end();
}

bool SigmaConnect::init()
{
    sigma_tcp_server.begin(SIGMA_TCP_PORT);
    sigma_tcp_server.setNoDelay(true);
    Wire.begin(device_config->sigma_connect.dsp_i2c_sda_pin, device_config->sigma_connect.dsp_i2c_scl_pin);
    dsp->begin();

    return true;
}

bool SigmaConnect::handle()
{
    if (device_config->sigma_connect.dsp_auto_power_on_vban)
    {
        bool inactive = !(device_status->vban_receiver.active || device_status->vban_transmitter.active);
        if (mute != inactive)
        {
            mute = inactive;
            dsp->muteDAC(mute);

            device_status->sigma_connect.dac_mute = mute;
        }
    }

    if (!device_config->sigma_connect.sigma_tcpi_server_enable)
    {
        return false;
    }

    if (!sigma_tcp_server.hasClient())
    {
        return false;
    }

    WiFiClient client = sigma_tcp_server.available();
    size_t count, ret = 0;

    int command;
    u_int16_t addr, len;
    u_int32_t payload_len;

    u_int8_t read_value;

    while (client.connected())
    {
        device_status->sigma_connect.client_connected = true;
        command = client.read();
        ret = 1;

        if (command == SIGMA_TCP_COMMAND_READ)
        {
            ret += client.readBytes(buf + ret, 7);

            payload_len = (buf[1] << 8) | buf[2];
            len = (buf[4] << 8) | buf[5];
            addr = (buf[6] << 8) | buf[7];

            buf[0] = SIGMA_TCP_COMMAND_READRESPONSE;
            buf[1] = (0x4 + len) >> 8;
            buf[2] = (0x4 + len) & 0xff;
            buf[3] = 0; // 0 indicates success, anything else is failure

            uint32_t data = dsp->readRegister((dspRegister)addr, len);
            int p = 4;
            if (len == 4)
                buf[p++] = (data > 24) & 0xFF;
            if (len >= 3)
                buf[p++] = (data > 16) & 0xFF;
            if (len >= 2)
                buf[p++] = (data > 8) & 0xFF;
            if (len >= 1)
                buf[p++] = (data > 0) & 0xFF;

            client.write(buf, 4 + len);
            client.flush();
        }
        else if (command == SIGMA_TCP_COMMAND_WRITE)
        {
            ret += client.readBytes(buf + ret, 9);

            payload_len = (buf[1] << 24) | (buf[2] << 16) | (buf[3] << 8) | buf[4];
            len = (buf[6] << 8) | buf[7];
            addr = (buf[8] << 8) | buf[9];

            ret += client.readBytes(buf + ret, len);

            dsp->writeRegister(addr, len, buf + 10);
        } else {
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }
    }
    device_status->sigma_connect.client_connected = false;
    return true;
}

TaskDef SigmaConnect::taskConfig()
{
    TaskDef task_config = {
        .pcName = "sigma_connect",
        .usStackDepth = 2048,
        .uxPriority = 0,
        .xCoreID = 1,
        .task_delay_ms = 500};
    return task_config;
}
