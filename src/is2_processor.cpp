#include "i2s_processor.h"

I2sCommon::I2sCommon()
{
    i2s_port = CONFIG_I2S_PORT;
    i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
        .sample_rate = 48000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL3 | ESP_INTR_FLAG_IRAM,
        .dma_buf_count = CONFIG_I2S_BUFFER_COUNT,
        .dma_buf_len = VBanNetQualitySampleSize[VBAN_NET_QUALITY_OPTIMAL],
        .use_apll = true,
        .tx_desc_auto_clear = true,
    };

    i2s_pin_config = {
        .mck_io_num = CONFIG_I2S_MCK_PIN,
        .bck_io_num = CONFIG_I2S_BCK_PIN,
        .ws_io_num = CONFIG_I2S_LRCK_PIN,
        .data_out_num = CONFIG_I2S_DATA_OUT_PIN,
        .data_in_num = CONFIG_I2S_DATA_IN_PIN};

    i2s_enabled = false;
    i2s_event_queue = NULL;
}

I2sCommon::~I2sCommon()
{
    if (i2s_enabled)
    {
        i2s_driver_uninstall(i2s_port);
    }
}

void I2sCommon::init_i2s(const i2s_mode_t &mode, const u_int32_t &sample_rate, const i2s_bits_per_sample_t &bits_per_sample, const vban_net_quality_t &vban_net_quality)
{
    if (i2s_enabled == false ||
        mode != i2s_config.mode ||
        sample_rate != i2s_config.sample_rate ||
        bits_per_sample != i2s_config.bits_per_sample)
    {

        esp_err_t err;
        if (i2s_enabled)
        {
            err = i2s_driver_uninstall(i2s_port);
            i2s_enabled = 0;
        }

        if (mode & I2S_MODE_RX)
        {
            i2s_pin_config.data_out_num = -1;
            i2s_pin_config.data_in_num = CONFIG_I2S_DATA_IN_PIN;
            i2s_pin_config.mck_io_num = CONFIG_I2S_MCK_PIN;
        }
        if (mode & I2S_MODE_TX)
        {
            i2s_pin_config.data_out_num = CONFIG_I2S_DATA_OUT_PIN;
            i2s_pin_config.data_in_num = -1;
            i2s_pin_config.mck_io_num = CONFIG_I2S_MCK_PIN;
        }

        i2s_config.mode = mode;
        i2s_config.sample_rate = sample_rate;
        i2s_config.bits_per_sample = bits_per_sample;
        i2s_config.dma_buf_len = VBanNetQualitySampleSize[vban_net_quality];

        err = i2s_driver_install(i2s_port, &i2s_config, 4, i2s_event_queue);
        if (err != ESP_OK)
        {
            return;
        }
        err = i2s_set_pin(i2s_port, &i2s_pin_config);
        if (err != ESP_OK)
        {
            return;
        }
        i2s_enabled = true;
    }
}

I2sWriter::I2sWriter(DeviceConfig *device_config, DeviceStatus *device_status, AudioRingBuffer *ring_buffer) : AudioProcessor(device_config, device_status, ring_buffer), I2sCommon()
{
}

I2sWriter::~I2sWriter()
{
}

TaskDef I2sWriter::taskConfig()
{
    TaskDef task_config = {
        .pcName = "i2s_writer",
        .usStackDepth = 2048,
        .uxPriority = 0,
        .xCoreID = 1};

    return task_config;
}

bool I2sWriter::init()
{
    i2s_port = CONFIG_I2S_PORT;

    return true;
}

bool I2sWriter::handle()
{
    PcmSamples *buffer;
    const i2s_mode_t mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    const vban_net_quality_t net_quality = device_config->net_quality;

    if (ring_buffer->popRing(buffer))
    {
        device_status->errors.underrun = 0;

        init_i2s(mode, buffer->sample_rate, (i2s_bits_per_sample_t)buffer->bits_per_sample, net_quality);

        do
        {
            size_t written_len = 0;
            esp_err_t err = i2s_write(i2s_port, buffer->data, buffer->len, &written_len, 100);
        } while (ring_buffer->popRing(buffer));
    }

    return true;
}

I2sReader::I2sReader(DeviceConfig *device_config, DeviceStatus *device_status, AudioRingBuffer *ring_buffer) : AudioProcessor(device_config, device_status, ring_buffer), I2sCommon()
{
}

I2sReader::~I2sReader()
{
}

TaskDef I2sReader::taskConfig()
{
    TaskDef task_config = {
        .pcName = "i2s_reader",
        .usStackDepth = 4096,
        .uxPriority = 0,
        .xCoreID = 1};

    return task_config;
}

bool I2sReader::init()
{
    const i2s_mode_t mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
    const uint32_t sample_rate = device_config->vban_transmitter.sample_rate;
    const i2s_bits_per_sample_t bits_per_sample = (i2s_bits_per_sample_t)device_config->vban_transmitter.bits_per_sample;
    const vban_net_quality_t net_quality = device_config->net_quality;
    const uint8_t channels = device_config->vban_transmitter.channels;

    i2s_port = CONFIG_I2S_PORT;

    init_i2s(mode, sample_rate, bits_per_sample, net_quality);

    expected_read = VBanNetQualitySampleSize[net_quality] * channels * (bits_per_sample / 8);

    return true;
}

bool I2sReader::handle()
{
    size_t read_len = 0;

    do
    {
        PcmSamples *buffer = ring_buffer->getNextBuffer();

        esp_err_t err = i2s_read(i2s_port, buffer->data, expected_read, &read_len, 25);

        if (expected_read > read_len)
        {
            device_status->errors.underrun = 1;
        }

        if (read_len == 0 || err != ESP_OK)
        {
            return false;
        }

        buffer->len = read_len;

        if (ring_buffer->pushRing(buffer))
        {
            device_status->errors.overrun = 0;
            device_status->errors.underrun = 0;
        }
        else
        {
            device_status->errors.overrun = 1;
        }
    } while (device_status->vban_enable);
    return true;
}