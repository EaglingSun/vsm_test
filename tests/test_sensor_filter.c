/* test_sensor_filter.c - 센서 필터 요구사항 테스트 (스위트 test_sensor_filter)
 *
 * 샘플 값은 0 ~ 32767 만 쓴다 (SR-001_sensor_value_40000 제외). 음수 입력의 동작은 요구사항 밖이고,
 * 지금 헤더 타입(int16_t)으로 표현되는 값만 써야 SR-007 · SR-008 케이스가 타입 결함과 섞이지 않는다.
 * count 는 1 ~ 8 만 쓴다 (SR-008 범위 밖의 결과는 요구사항이 정하지 않는다).
 * 링 버퍼는 초기화 함수가 없어 케이스 사이에 상태가 남는다 (ADR-002). 최근 값을 보는 케이스는
 * 자기가 넣은 값만으로 기대값이 정해지게 쓴다.
 * SR-004(링 버퍼에 최근 8개 유지)는 검사하지 않는다 — 버퍼 내용을 읽는 함수가 없다 (PROPOSALS P7).
 */

#include "vsm_test.h"
#include "sensor_filter.h"

/* SR-009: sf_latest() 는 가장 최근에 넣은 값 */
static void sr009_latest_after_push(void)
{
    sf_push(11);
    VT_CHECK_EQ_INT(11, sf_latest());
    sf_push(22);
    VT_CHECK_EQ_INT(22, sf_latest());
}

/* SR-009: 8칸을 한 바퀴 넘겨 넣어도 최근 값은 마지막에 넣은 값
 * 링 크기(SR-004 8칸)는 이 케이스로 검증되지 않는다 — 버퍼 내용을 읽는 함수가 없다 (PROPOSALS P7).
 */
static void sr009_latest_after_wraparound(void)
{
    int16_t v;

    for (v = 1; v <= 10; v++)
    {
        sf_push(v);
    }
    VT_CHECK_EQ_INT(10, sf_latest());
}

/* SR-001 · 1-1 용어: 센서 값 범위는 차속과 같은 0 ~ 65535 (상수를 바로 넘기면 -Woverflow 경고라 변수에 담는다) */
static void sr001_sensor_value_40000(void)
{
    uint16_t v = 40000u;

    sf_push(v);
    VT_CHECK_EQ_INT(40000, sf_latest());
}

/* SR-007: 산술 평균의 소수점 이하는 버린다 */
static void sr007_average_truncates(void)
{
    const int16_t two[] = { 1, 2 };
    const int16_t three[] = { 1, 1, 2 };

    VT_CHECK_EQ_INT(1, sf_average(two, 2u));
    VT_CHECK_EQ_INT(1, sf_average(three, 3u));
}

/* SR-007: 나누어떨어지는 산술 평균 */
static void sr007_average_exact(void)
{
    const int16_t samples[] = { 10, 20, 30, 40 };

    VT_CHECK_EQ_INT(25, sf_average(samples, 4u));
}

/* SR-008 하한: 샘플 1 개 */
static void sr008_average_count_1(void)
{
    const int16_t samples[] = { 7 };

    VT_CHECK_EQ_INT(7, sf_average(samples, 1u));
}

/* SR-008 상한: 샘플 8 개 */
static void sr008_average_count_8(void)
{
    const int16_t samples[] = { 10, 20, 30, 40, 50, 60, 70, 80 };

    VT_CHECK_EQ_INT(45, sf_average(samples, 8u));
}

int main(void)
{
    vt_begin("test_sensor_filter", "test-results/test_sensor_filter.xml");
    vt_run("SR-009_latest_after_push", sr009_latest_after_push);
    vt_run("SR-009_latest_after_wraparound", sr009_latest_after_wraparound);
    vt_run("SR-001_sensor_value_40000", sr001_sensor_value_40000);
    vt_run("SR-007_average_truncates", sr007_average_truncates);
    vt_run("SR-007_average_exact", sr007_average_exact);
    vt_run("SR-008_average_count_1", sr008_average_count_1);
    vt_run("SR-008_average_count_8", sr008_average_count_8);
    return vt_end() == 0 ? 0 : 1;
}
