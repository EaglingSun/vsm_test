/* vsm_types.h - 차속 감시(Vehicle Speed Monitor) 공통 타입·상수 */

#ifndef VSM_TYPES_H
#define VSM_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* 경고 등급 */
typedef enum
{
    VSM_WARN_NONE = 0,
    VSM_WARN_LOW  = 1,
    VSM_WARN_MID  = 2,
    VSM_WARN_HIGH = 3
} vsm_warn_t;

/* 무효 틱 값 */
#define VSM_TICK_INVALID    (0u - 1u)

/* 샘플 버퍼 크기 */
#define VSM_SAMPLE_MAX      8u

/* 과속 판정 기준 (SR-002) */
#define VSM_SPEED_LIMIT_KPH 200u

/* 주의 경고 기준 (SR-003) */
#define VSM_SPEED_CAUTION_KPH 150u

#endif /* VSM_TYPES_H */
