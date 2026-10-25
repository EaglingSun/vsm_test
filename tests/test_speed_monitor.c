/* test_speed_monitor.c - 차속 판정 요구사항 테스트 (스위트 test_speed_monitor)
 *
 * 판정 케이스는 모두 brake_on = false 로 부른다 (제동이 등급에 주는 영향은 요구사항에 없다).
 * 카운터를 보는 케이스는 sm_reset(1u) 로 시작한다 (케이스들이 static 카운터를 공유한다, ADR-002).
 */

#include "vsm_test.h"
#include "speed_monitor.h"

/* §3: 차속 0 km/h -> NONE */
static void sec3_speed_0_is_none(void)
{
    VT_CHECK_EQ_INT(VSM_WARN_NONE, sm_evaluate(0u, false));
}

/* §3: 0 < 차속 <= 150 km/h -> LOW (하한) */
static void sec3_speed_1_is_low(void)
{
    VT_CHECK_EQ_INT(VSM_WARN_LOW, sm_evaluate(1u, false));
}

/* §3 · SR-003 경계: 150 km/h 는 MID 가 아니라 LOW */
static void sec3_speed_150_is_low(void)
{
    VT_CHECK_EQ_INT(VSM_WARN_LOW, sm_evaluate(150u, false));
}

/* SR-003: 150 km/h 초과 200 km/h 미만 -> MID */
static void sr003_speed_151_is_mid(void)
{
    VT_CHECK_EQ_INT(VSM_WARN_MID, sm_evaluate(151u, false));
}

/* SR-003: 200 km/h 미만 상한 -> MID */
static void sr003_speed_199_is_mid(void)
{
    VT_CHECK_EQ_INT(VSM_WARN_MID, sm_evaluate(199u, false));
}

/* SR-002 경계: 200 km/h 이상 -> HIGH */
static void sr002_speed_200_is_high(void)
{
    VT_CHECK_EQ_INT(VSM_WARN_HIGH, sm_evaluate(200u, false));
}

/* SR-002: 200 km/h 초과 -> HIGH */
static void sr002_speed_201_is_high(void)
{
    VT_CHECK_EQ_INT(VSM_WARN_HIGH, sm_evaluate(201u, false));
}

/* SR-002 · SR-001 상한: 65535 km/h -> HIGH */
static void sr002_speed_65535_is_high(void)
{
    VT_CHECK_EQ_INT(VSM_WARN_HIGH, sm_evaluate(65535u, false));
}

/* SR-010: HIGH 로 판정할 때마다 카운터 1 증가 */
static void sr010_count_per_high(void)
{
    sm_reset(1u);
    (void)sm_evaluate(201u, false);
    (void)sm_evaluate(201u, false);
    (void)sm_evaluate(201u, false);
    VT_CHECK_EQ_INT(3, sm_over_count());
}

/* SR-010 불변식: HIGH 를 반환했으면 카운터가 1, 아니면 0 (제동 효과 자체는 검사하지 않는다) */
static void sr010_count_follows_high_with_brake(void)
{
    vsm_warn_t w;

    sm_reset(1u);
    w = sm_evaluate(199u, true);
    VT_CHECK_EQ_INT((w == VSM_WARN_HIGH ? 1 : 0), sm_over_count());
}

/* SR-005: sm_reset() 이 과속 카운터를 초기화 */
static void sr005_reset_clears_counter(void)
{
    sm_reset(1u);
    (void)sm_evaluate(201u, false);
    (void)sm_evaluate(201u, false);
    sm_reset(1u);
    VT_CHECK_EQ_INT(0, sm_over_count());
}

int main(void)
{
    vt_begin("test_speed_monitor", "test-results/test_speed_monitor.xml");
    vt_run("SEC3_speed_0_is_none", sec3_speed_0_is_none);
    vt_run("SEC3_speed_1_is_low", sec3_speed_1_is_low);
    vt_run("SEC3_speed_150_is_low", sec3_speed_150_is_low);
    vt_run("SR-003_speed_151_is_mid", sr003_speed_151_is_mid);
    vt_run("SR-003_speed_199_is_mid", sr003_speed_199_is_mid);
    vt_run("SR-002_speed_200_is_high", sr002_speed_200_is_high);
    vt_run("SR-002_speed_201_is_high", sr002_speed_201_is_high);
    vt_run("SR-002_speed_65535_is_high", sr002_speed_65535_is_high);
    vt_run("SR-010_count_per_high", sr010_count_per_high);
    vt_run("SR-010_count_follows_high_with_brake", sr010_count_follows_high_with_brake);
    vt_run("SR-005_reset_clears_counter", sr005_reset_clears_counter);
    return vt_end() == 0 ? 0 : 1;
}
