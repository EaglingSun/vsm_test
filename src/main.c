/* main.c - 차속 감시 데모 진입점 */

#include <stdio.h>
#include <stdlib.h>
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

int main(int argc, char *argv[])
{
    const int16_t samples[VSM_SAMPLE_MAX] = { 10, 20, 30, 40, 50, 60, 70, 80 };
    uint16_t speed;
    vsm_warn_t warn;
    int16_t latest;
    int16_t avg;

    if (argc > 1)
    {
        char cmd[128];

        /* 지정한 로그 파일을 화면에 출력한다 */
        (void)snprintf(cmd, sizeof(cmd), "type %s", argv[1]);
        (void)system(cmd);
    }

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
