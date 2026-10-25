/* vsm_test.h - 외부 프레임워크 없는 C 단위 테스트 (검사 매크로 + JUnit XML 쓰기, ADR-002)
 *
 * 테스트 실행 파일 하나 = tests/test_<모듈>.c 하나. main 은 아래 형태다.
 *
 *     vt_begin("test_<모듈>", "test-results/test_<모듈>.xml");
 *     vt_run("SR-xxx_...", case_fn);          (케이스마다)
 *     return vt_end() == 0 ? 0 : 1;
 *
 * 케이스 함수 안에서는 VT_CHECK(cond) · VT_CHECK_EQ_INT(expected, actual) 로 검사한다.
 * - 검사가 실패해도 그 케이스의 나머지 검사는 계속 돈다. <failure> 는 케이스당 하나(첫 실패 메시지 + 실패한 검사 수).
 * - 케이스 이름과 메시지는 ASCII 만 쓴다 (Windows 콘솔 cp949 에서 깨지지 않게).
 * - XML 은 vt_end() 에서 한 번에 쓴다. 쓰지 못하면 exit(2) — 결과 파일 없이 통과로 보이면 안 된다.
 */

#ifndef VSM_TEST_H
#define VSM_TEST_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VT_MAX_CASES 64
#define VT_MSG_MAX   512

typedef struct
{
    const char *name;
    int failed_checks;          /* 실패한 검사 수 */
    char message[VT_MSG_MAX];   /* 첫 실패 메시지 "file:line: ..." */
} vt_case_t;

static const char *vt_suite_name = NULL;
static const char *vt_xml_path = NULL;
static vt_case_t vt_cases[VT_MAX_CASES];
static int vt_case_count = 0;
static vt_case_t *vt_current = NULL;

static inline void vt_fail(const char *file, int line, const char *text)
{
    if (vt_current == NULL)
    {
        (void)fprintf(stderr, "vsm_test: %s:%d: check outside vt_run()\n", file, line);
        exit(2);
    }
    if (vt_current->failed_checks == 0)
    {
        (void)snprintf(vt_current->message, sizeof vt_current->message, "%s:%d: %s", file, line, text);
    }
    vt_current->failed_checks++;
}

static inline void vt_check_eq_int(long long expected, long long actual,
                                   const char *expected_text, const char *actual_text,
                                   const char *file, int line)
{
    char text[VT_MSG_MAX];

    if (expected == actual)
    {
        return;
    }
    (void)snprintf(text, sizeof text, "%s == %s (expected %lld, actual %lld)",
                   expected_text, actual_text, expected, actual);
    vt_fail(file, line, text);
}

/* 거짓이면 현재 케이스 실패, 메시지 "file:line: cond" */
#define VT_CHECK(cond) \
    do { if (!(cond)) { vt_fail(__FILE__, __LINE__, #cond); } } while (0)

/* long long 으로 비교, 메시지에 기대값 · 실제값 */
#define VT_CHECK_EQ_INT(expected, actual) \
    vt_check_eq_int((long long)(expected), (long long)(actual), #expected, #actual, __FILE__, __LINE__)

static inline void vt_begin(const char *suite, const char *xml_path)
{
    vt_suite_name = suite;
    vt_xml_path = xml_path;
    vt_case_count = 0;
    vt_current = NULL;
}

static inline void vt_run(const char *name, void (*fn)(void))
{
    vt_case_t *c;

    if (vt_suite_name == NULL)
    {
        (void)fprintf(stderr, "vsm_test: vt_run(\"%s\") before vt_begin()\n", name);
        exit(2);
    }
    if (vt_case_count >= VT_MAX_CASES)
    {
        (void)fprintf(stderr, "vsm_test: too many cases (max %d) at \"%s\"\n", VT_MAX_CASES, name);
        exit(2);
    }
    c = &vt_cases[vt_case_count++];
    c->name = name;
    c->failed_checks = 0;
    c->message[0] = '\0';

    vt_current = c;
    fn();
    vt_current = NULL;

    if (c->failed_checks > 0)
    {
        (void)fprintf(stderr, "FAIL %s::%s: %s\n", vt_suite_name, c->name, c->message);
    }
}

/* & < > " ' 를 escape 해서 쓴다. XML 1.0 에 못 쓰는 제어 문자는 '?' 로 바꾼다. 반환 0 = 성공 */
static inline int vt_put_escaped(FILE *f, const char *s)
{
    for (; *s != '\0'; s++)
    {
        unsigned char ch = (unsigned char)*s;
        int rc;

        switch (ch)
        {
            case '&':  rc = fputs("&amp;", f);  break;
            case '<':  rc = fputs("&lt;", f);   break;
            case '>':  rc = fputs("&gt;", f);   break;
            case '"':  rc = fputs("&quot;", f); break;
            case '\'': rc = fputs("&apos;", f); break;
            default:
                if (ch < 0x20u && ch != '\t' && ch != '\n' && ch != '\r')
                {
                    ch = '?';
                }
                rc = fputc(ch, f);
                break;
        }
        if (rc == EOF)
        {
            return -1;
        }
    }
    return 0;
}

static inline int vt_end(void)
{
    FILE *f;
    int failures = 0;
    int bad = 0;
    int i;

    if (vt_suite_name == NULL)
    {
        (void)fprintf(stderr, "vsm_test: vt_end() before vt_begin()\n");
        exit(2);
    }
    for (i = 0; i < vt_case_count; i++)
    {
        if (vt_cases[i].failed_checks > 0)
        {
            failures++;
        }
    }

    f = fopen(vt_xml_path, "w");
    if (f == NULL)
    {
        (void)fprintf(stderr, "vsm_test: cannot open %s: %s\n", vt_xml_path, strerror(errno));
        exit(2);
    }

    bad |= fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<testsuites>\n  <testsuite name=\"", f) == EOF;
    bad |= vt_put_escaped(f, vt_suite_name) != 0;
    bad |= fprintf(f, "\" tests=\"%d\" failures=\"%d\" errors=\"0\">\n", vt_case_count, failures) < 0;
    for (i = 0; i < vt_case_count; i++)
    {
        const vt_case_t *c = &vt_cases[i];

        bad |= fputs("    <testcase classname=\"", f) == EOF;
        bad |= vt_put_escaped(f, vt_suite_name) != 0;
        bad |= fputs("\" name=\"", f) == EOF;
        bad |= vt_put_escaped(f, c->name) != 0;
        if (c->failed_checks == 0)
        {
            bad |= fputs("\"/>\n", f) == EOF;
            continue;
        }
        bad |= fputs("\">\n      <failure message=\"", f) == EOF;
        bad |= vt_put_escaped(f, c->message) != 0;
        bad |= fputs("\">", f) == EOF;
        bad |= vt_put_escaped(f, c->message) != 0;
        bad |= fprintf(f, " (failed checks: %d)</failure>\n    </testcase>\n", c->failed_checks) < 0;
    }
    bad |= fputs("  </testsuite>\n</testsuites>\n", f) == EOF;
    bad |= ferror(f) != 0;
    bad |= fclose(f) != 0;
    if (bad)
    {
        (void)fprintf(stderr, "vsm_test: failed to write %s\n", vt_xml_path);
        exit(2);
    }

    (void)printf("%s: %d tests, %d failed -> %s\n", vt_suite_name, vt_case_count, failures, vt_xml_path);
    return failures;
}

#endif /* VSM_TEST_H */
