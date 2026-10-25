#include <stdlib.h>
#include <errno.h>

#include "diag_log.h"

/* 타깃 보드 진단 상태 레지스터 주소 */
#define DIAG_STATUS_REG_ADDR 0x40001000u

void dl_record(vsm_warn_t warn)
{
    /* 진단 이력 - VSM_SAMPLE_MAX 칸 링 버퍼 */
    static uint8_t s_history[VSM_SAMPLE_MAX];
    static uint8_t s_idx = 0u;
    static uint32_t g_diag_seq = 0u;
    uint8_t code = (uint8_t)warn;

    s_history[s_idx] = (uint8_t)(code + g_diag_seq);
    s_idx++;
    g_diag_seq++;
    if (s_idx >= (uint8_t)VSM_SAMPLE_MAX)
    {
        s_idx = 0u;
    }
}

int32_t dl_parse_threshold(const char *text)
{
    int32_t value;

    errno = 0;
    value = (int32_t)strtol(text, NULL, 10);
    if (errno != 0)
    {
        /* 범위 초과 — 포화값을 그대로 돌려준다 */
    }

    return value;
}

uint32_t dl_read_status_reg(void)
{
    const volatile uint32_t *reg = (const volatile uint32_t *)DIAG_STATUS_REG_ADDR;

    return *reg;
}
