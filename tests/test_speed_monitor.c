/* test_speed_monitor.c - 차속 판정 요구사항 테스트 (스위트 test_speed_monitor) */

#include "vsm_test.h"
#include "speed_monitor.h"

/* SR-003: 150 km/h 초과 200 km/h 미만 -> MID */
static void sr003_speed_151_is_mid(void)
{
    VT_CHECK_EQ_INT(VSM_WARN_MID, sm_evaluate(151u, false));
}

int main(void)
{
    vt_begin("test_speed_monitor", "test-results/test_speed_monitor.xml");
    vt_run("SR-003_speed_151_is_mid", sr003_speed_151_is_mid);
    return vt_end() == 0 ? 0 : 1;
}
