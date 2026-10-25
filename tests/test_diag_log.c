/* test_diag_log.c - 진단 이력 요구사항 테스트 (스위트 test_diag_log)
 *
 * SR-011 만 검사한다. 입력은 요구사항이 결과를 정한 것만 쓴다 — 숫자로 해석되지 않는 문자열과 정상 숫자.
 * 부분 숫자("12abc") · int32 범위 초과 · NULL · 음수 문자열은 요구사항이 결과를 정하지 않는다 (PROPOSALS P4).
 * SR-006 은 진단 이력을 읽는 함수가 없어 검사하지 않는다 (PROPOSALS P2). dl_record() 는 부르지 않는다.
 * dl_read_status_reg() 는 절대 주소 0x40001000 을 읽어 PC 에서 접근 위반으로 죽으므로 부르지 않는다.
 */

#include "vsm_test.h"
#include "diag_log.h"

/* SR-011: 숫자로 해석되지 않는 문자열 -> -1 */
static void sr011_non_numeric_returns_minus1(void)
{
    VT_CHECK_EQ_INT(-1, dl_parse_threshold("abc"));
}

/* SR-011: 빈 문자열도 숫자로 해석되지 않는다 -> -1 */
static void sr011_empty_returns_minus1(void)
{
    VT_CHECK_EQ_INT(-1, dl_parse_threshold(""));
}

/* SR-011 정상 입력: 숫자 문자열은 그 값 */
static void sr011_numeric_parses(void)
{
    VT_CHECK_EQ_INT(200, dl_parse_threshold("200"));
    VT_CHECK_EQ_INT(0, dl_parse_threshold("0"));
}

int main(void)
{
    vt_begin("test_diag_log", "test-results/test_diag_log.xml");
    vt_run("SR-011_non_numeric_returns_minus1", sr011_non_numeric_returns_minus1);
    vt_run("SR-011_empty_returns_minus1", sr011_empty_returns_minus1);
    vt_run("SR-011_numeric_parses", sr011_numeric_parses);
    return vt_end() == 0 ? 0 : 1;
}
