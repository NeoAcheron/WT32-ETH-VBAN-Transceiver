#include "common.h"

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

AudioProcessor::AudioProcessor(DeviceConfig *device_config, DeviceStatus *device_status, AudioRingBuffer *ring_buffer) : device_config(device_config),
                                                                                                                         device_status(device_status),
                                                                                                                         ring_buffer(ring_buffer)

{
    task_handle = NULL;
    task_running = false;
    task_delay_ms = 1;
}

AudioProcessor::~AudioProcessor()
{
}

void AudioProcessor::loop(void *self_ref)
{
    if (self_ref == 0)
    {
        return;
    }

    AudioProcessor *self = (AudioProcessor *)self_ref;

    if (!self->init())
        return;

    self->task_running = true;
    TickType_t task_delay_ticks = self->task_delay_ms / portTICK_PERIOD_MS;

    while (true)
    {
        self->handle();
        if(task_delay_ticks){
            vTaskDelay(task_delay_ticks);
        }
    }
}

int AudioProcessor::begin()
{
    if (task_handle)
        return pdFREERTOS_ERRNO_EINPROGRESS;

    TaskDef task_def = this->taskConfig();
    task_delay_ms = task_def.task_delay_ms;

    int result = xTaskCreatePinnedToCore(
        this->loop,            /* Function to implement the task */
        task_def.pcName,       /* Name of the task */
        task_def.usStackDepth, /* Stack size in words */
        this,                  /* Task input parameter */
        task_def.uxPriority,   /* Priority of the task */
        &task_handle,          /* Task handle. */
        task_def.xCoreID);     /* Core where the task should run */

    return result;
}

bool AudioProcessor::stop()
{
    if (this->task_handle != NULL)
    {
        vTaskDelete(this->task_handle);
        this->task_handle = NULL;
        this->task_running = false;
    }
    return true;
}
