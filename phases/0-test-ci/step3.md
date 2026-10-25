# Step 3: diag-log-tests

## 참조 문서

- `docs/ARCHITECTURE.md`
- `docs/ADR.md`

## 근거 ADR

- ADR-002: 외부 프레임워크 없는 C 테스트 + C 쪽 JUnit XML 출력 + 알려진 실패 목록

## 읽어야 할 파일

먼저 아래 파일들을 읽고 프로젝트의 아키텍처와 설계 의도를 파악하라:

- `test_ref/요구사항.md` — SR-006 · SR-011
- `include/diag_log.h` · `src/diag_log.c`
- `tests/vsm_test.h` · `tests/test_speed_monitor.c`(케이스 작성 방식 참고) · `tests/expected_failures.txt` — 이전 step 산출물
- `Makefile` 의 `test` 타깃 — `tests/test_*.c` 를 자동으로 찾아 `src/<모듈>.c` 와 링크한다

이전 step에서 만들어진 코드를 꼼꼼히 읽고, 설계 의도를 이해한 뒤 작업하라.

## 작업

`tests/test_diag_log.c` 를 새로 만든다. 스위트 이름 `test_diag_log`, XML 경로 `test-results/test_diag_log.xml`.

### 넣을 케이스 (이름 그대로 · 기대값은 요구사항에서 온다)

| 케이스 | 근거 | 입력 → 기대 |
|---|---|---|
| `SR-011_non_numeric_returns_minus1` | SR-011 | `dl_parse_threshold("abc")` → `-1` |
| `SR-011_empty_returns_minus1` | SR-011 | `dl_parse_threshold("")` → `-1` |
| `SR-011_numeric_parses` | SR-011 (정상 입력) | `"200"` → `200`, `"0"` → `0` |

규칙:
- `"12abc"` · int32 범위를 넘는 숫자 · `NULL` · 음수 문자열은 넣지 않는다. 이유: 요구사항이 그 결과를 정하지 않는다(PROPOSALS P4).
- `dl_read_status_reg()` 를 부르지 마라. 이유: 절대 주소 `0x40001000` 을 읽어 PC 에서는 접근 위반으로 죽고, XML 이 생기지 않는다.
- SR-006 케이스는 넣지 않는다. 이유: 진단 이력을 읽는 함수가 없어 기록 내용을 확인할 수 없다(PRD 제외 · PROPOSALS P2). `dl_record()` 를 부르기만 하는 "안 죽는지" 케이스도 만들지 않는다 — SR-006 을 검증한 것처럼 보이게 된다.

### `tests/expected_failures.txt`

결함판에서 **실제로 실패한** 케이스를 `test_diag_log::<케이스>` 로 덧붙인다(기존 항목은 그대로 둔다). 코드 분석상 실패가 예상되는 것은 아래 둘이다:

- `test_diag_log::SR-011_non_numeric_returns_minus1` — `strtol` 의 끝 위치를 보지 않아 `"abc"` 가 0
- `test_diag_log::SR-011_empty_returns_minus1` — 같은 원인으로 `""` 가 0

실제 결과가 이와 다르면 먼저 테스트가 요구사항대로 쓰였는지 다시 본다. 그래도 다르면 실제 결과대로 목록을 두고 차이를 summary 에 적는다.

## 요구사항 ID

R4

## 변경 허용 범위

이 목록 밖의 파일은 수정하지 마라. scope_check.py가 검사한다.

- `tests/test_diag_log.c`
- `tests/expected_failures.txt`

## Acceptance Criteria

```bash
./make.cmd test; python tests/check_junit.py test-results --expect tests/expected_failures.txt                 # sh 경로
./make.cmd test SHELL=cmd.exe; python tests/check_junit.py test-results --expect tests/expected_failures.txt   # cmd.exe 경로
python -m pytest -q
ruff check scripts tests
python scripts/scope_check.py phases/0-test-ci/step3.md
```

## 검증 절차

1. 위 AC 커맨드를 실행한다.
2. RED 확인: 목록에 올린 각 케이스의 실패 메시지(기대값 · 실제값)를 `test-results/test_diag_log.xml` 에서 읽고, 그 실패가 위에 적은 결함 때문인지 확인한다(예: `"abc"` → 실제 0).
3. 아키텍처 체크리스트를 확인한다:
   - ARCHITECTURE.md 디렉토리 구조를 따르는가?
   - ADR 기술 스택을 벗어나지 않았는가?
   - CLAUDE.md CRITICAL 규칙을 위반하지 않았는가?
   - "근거 ADR"의 결정을 따랐는가? 새 기술 결정이 필요했다면 구현하지 말고 blocked 처리하라.
4. 결과에 따라 `phases/0-test-ci/index.json`의 해당 step을 업데이트한다:
   - 성공 → `"status": "completed"`, `"summary": "산출물 한 줄 요약"` (케이스 수 · 이번에 추가한 알려진 실패 · 전체 알려진 실패 수)
   - 수정 3회 시도 후에도 실패 → `"status": "error"`, `"error_message": "구체적 에러 내용"`
   - 사용자 개입 필요 → `"status": "blocked"`, `"blocked_reason": "구체적 사유"` 후 즉시 중단

## 금지사항

- `src/` · `include/` 를 고치지 마라. 이유: master 코드는 교육용 결함판이다. 읽기 함수 추가(SR-006)도 하지 않는다.
- 결함판에서 통과하도록 기대값을 바꾸지 마라. 이유: 기대값의 출처는 요구사항이지 현재 코드가 아니다.
- `tests/vsm_test.h` · `Makefile` · `tests/check_junit.py` · 다른 `tests/test_*.c` 를 고치지 마라. 이유: 이전 step 산출물이고 이 step 범위 밖이다. 고칠 필요가 보이면 blocked 처리한다.
- 기존 테스트를 깨뜨리지 마라
- AC에 없는 기능, 리팩터링, 의존성을 추가하지 마라. 이유: MVP 범위 고정. 아이디어는 `docs/PROPOSALS.md`에 `[ ]` 항목으로만 기록하라.
