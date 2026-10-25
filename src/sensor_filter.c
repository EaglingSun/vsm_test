/* sensor_filter.c - 차속 센서 값 필터링 */

#include "sensor_filter.h"

/* 센서 값 링 버퍼 */
static int16_t s_buffer[VSM_SAMPLE_MAX] = { 0, 0, 0, 0, 0, 0, 0, 0 };

static uint8_t s_head = 0u;

void sf_push(int16_t value)
{
    s_buffer[s_head] = value;

    s_head = (uint8_t)((s_head + 1u) % VSM_SAMPLE_MAX);
}

int16_t sf_latest(void)
{
    return s_buffer[s_head];
}

int16_t sf_average(const int16_t *samples, uint8_t count)
{
    int32_t sum = 0;
    uint8_t i = 0u;
    int16_t avg;

    while (i < count)
    {
        sum += samples[i];
        if (samples[i] < 0)
        {
            i += 2u;
        }
        else
        {
            i++;
        }
    }

    avg = (int16_t)(sum / (int32_t)count);

    return avg;
}
