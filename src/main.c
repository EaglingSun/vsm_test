/* main.c - 차속 감시 데모 진입점 */

#include <stdio.h>
#include <inttypes.h>

#include "speed_monitor.h"
#include "sensor_filter.h"
#include "diag_log.h"

static void fatal_halt(void)
{
    while (1u)
    {
        /* 워치독 리셋 대기 */
    }
}

int main(void)
{
    const int16_t samples[VSM_SAMPLE_MAX] = { 10, 20, 30, 40, 50, 60, 70, 80 };
    uint16_t speed;
    vsm_warn_t warn;
    int16_t latest;
    int16_t avg;

    sm_reset(1u);

    for (speed = 0u; speed <= 220u; speed = (uint16_t)(speed + 20u))
    {
        sf_push((int16_t)speed);
        warn = sm_evaluate(speed, false);
        dl_record(warn);

        latest = sf_latest();
        avg = sf_average(samples, (uint8_t)VSM_SAMPLE_MAX);

        (void)printf("speed=%3" PRIu16 "  warn=%" PRId32 "  latest=%" PRId16 "  avg=%" PRId16 "\n",
                     speed,
                     (int32_t)warn,
                     latest,
                     avg);
    }

    (void)printf("over_count=%" PRIu32 "\n", sm_over_count());

    if (dl_parse_threshold("abc") < 0)
    {
        fatal_halt();
    }

    return 0;
}
