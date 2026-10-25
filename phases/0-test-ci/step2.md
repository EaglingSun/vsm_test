# Step 2: sensor-filter-tests

## 참조 문서

- `docs/ARCHITECTURE.md`
- `docs/ADR.md`

## 근거 ADR

- ADR-002: 외부 프레임워크 없는 C 테스트 + C 쪽 JUnit XML 출력 + 알려진 실패 목록

## 읽어야 할 파일

먼저 아래 파일들을 읽고 프로젝트의 아키텍처와 설계 의도를 파악하라:

- `test_ref/요구사항.md` — SR-001 · SR-004 · SR-007 · SR-008 · SR-009, 1-1 용어(센서 값의 범위 = 차속 범위)
- `include/vsm_types.h` · `include/sensor_filter.h` · `src/sensor_filter.c`
- `tests/vsm_test.h` · `tests/test_speed_monitor.c`(케이스 작성 방식 참고) · `tests/expected_failures.txt` — 이전 step 산출물
- `Makefile` 의 `test` 타깃 — `tests/test_*.c` 를 자동으로 찾아 `src/<모듈>.c` 와 링크한다

이전 step에서 만들어진 코드를 꼼꼼히 읽고, 설계 의도를 이해한 뒤 작업하라.

## 작업

`tests/test_sensor_filter.c` 를 새로 만든다. 스위트 이름 `test_sensor_filter`, XML 경로 `test-results/test_sensor_filter.xml`.

### 넣을 케이스 (이름 그대로 · 기대값은 요구사항에서 온다)

| 케이스 | 근거 | 입력 → 기대 |
|---|---|---|
| `SR-009_latest_after_push` | SR-009 | `sf_push(11)` → `sf_latest() == 11`, 이어서 `sf_push(22)` → `sf_latest() == 22` |
| `SR-004_latest_after_wraparound` | SR-004 · SR-009 | 1 ~ 10 을 차례로 `sf_push` (8칸을 한 바퀴 넘김) → `sf_latest() == 10` |
| `SR-001_sensor_value_40000` | SR-001 · 1-1 용어 | `uint16_t v = 40000u; sf_push(v);` → `(long long)sf_latest() == 40000` |
| `SR-007_average_truncates` | SR-007 | `{1, 2}` count 2 → 1, `{1, 1, 2}` count 3 → 1 |
| `SR-007_average_exact` | SR-007 | `{10, 20, 30, 40}` count 4 → 25 |
| `SR-008_average_count_1` | SR-008 하한 | `{7}` count 1 → 7 |
| `SR-008_average_count_8` | SR-008 상한 | `{10, 20, 30, 40, 50, 60, 70, 80}` count 8 → 45 |

규칙:
- 샘플 값은 0 ~ 32767 안에서만 쓴다(`SR-001_sensor_value_40000` 제외). 이유: 센서 값의 요구 범위는 0 ~ 65535 이고, 음수 입력에서의 동작은 요구사항 밖이다. 지금 헤더 타입(`int16_t`)으로 표현되는 값만 쓰면 SR-007 · SR-008 케이스가 타입 결함과 섞이지 않는다.
- 샘플 배열 타입은 지금 헤더 시그니처(`const int16_t *`)를 따른다.
- `SR-001_sensor_value_40000` 은 값을 `uint16_t` 변수에 담아 넘긴다. 이유: 상수 40000 을 바로 넘기면 gcc 가 -Woverflow 경고를 낸다. 이 케이스는 SR-009 결함과도 함께 걸린다 — 둘 다 고쳐져야 통과하는 것이 맞다.
- 링 버퍼는 초기화 함수가 없어 케이스 사이에 상태가 남는다(ADR-002 트레이드오프). 최근 값을 보는 케이스는 자기가 넣은 값만으로 기대값이 정해지게 쓴다.
- count 0 이나 9 이상, 음수 샘플은 넣지 않는다. 이유: 요구사항이 그 경우의 결과를 정하지 않는다(PROPOSALS P4).

### `tests/expected_failures.txt`

결함판에서 **실제로 실패한** 케이스를 `test_sensor_filter::<케이스>` 로 덧붙인다(기존 항목은 그대로 둔다). 코드 분석상 실패가 예상되는 것은 아래 셋이다:

- `test_sensor_filter::SR-009_latest_after_push` — `sf_latest()` 가 다음에 쓸 칸(가장 오래된 값)을 돌려준다
- `test_sensor_filter::SR-004_latest_after_wraparound` — 같은 원인
- `test_sensor_filter::SR-001_sensor_value_40000` — 센서 값이 `int16_t` 라 32767 을 넘는 값을 담지 못한다(+ SR-009)

실제 결과가 이와 다르면 먼저 테스트가 요구사항대로 쓰였는지 다시 본다. 그래도 다르면 실제 결과대로 목록을 두고 차이를 summary 에 적는다.

## 요구사항 ID

R3

## 변경 허용 범위

이 목록 밖의 파일은 수정하지 마라. scope_check.py가 검사한다.

- `tests/test_sensor_filter.c`
- `tests/expected_failures.txt`

## Acceptance Criteria

```bash
./make.cmd test; python tests/check_junit.py test-results --expect tests/expected_failures.txt                 # sh 경로
./make.cmd test SHELL=cmd.exe; python tests/check_junit.py test-results --expect tests/expected_failures.txt   # cmd.exe 경로
python -m pytest -q
ruff check scripts tests
python scripts/scope_check.py phases/0-test-ci/step2.md
```

## 검증 절차

1. 위 AC 커맨드를 실행한다.
2. RED 확인: 목록에 올린 각 케이스의 실패 메시지(기대값 · 실제값)를 `test-results/test_sensor_filter.xml` 에서 읽고, 그 실패가 위에 적은 결함 때문인지 확인한다. 엉뚱한 이유(이전 케이스가 남긴 버퍼 상태 등)로 실패한 케이스는 테스트를 고친다.
3. 아키텍처 체크리스트를 확인한다:
   - ARCHITECTURE.md 디렉토리 구조를 따르는가?
   - ADR 기술 스택을 벗어나지 않았는가?
   - CLAUDE.md CRITICAL 규칙을 위반하지 않았는가?
   - "근거 ADR"의 결정을 따랐는가? 새 기술 결정이 필요했다면 구현하지 말고 blocked 처리하라.
4. 결과에 따라 `phases/0-test-ci/index.json`의 해당 step을 업데이트한다:
   - 성공 → `"status": "completed"`, `"summary": "산출물 한 줄 요약"` (케이스 수 · 이번에 추가한 알려진 실패 · 예상과 다른 점)
   - 수정 3회 시도 후에도 실패 → `"status": "error"`, `"error_message": "구체적 에러 내용"`
   - 사용자 개입 필요 → `"status": "blocked"`, `"blocked_reason": "구체적 사유"` 후 즉시 중단

## 금지사항

- `src/` · `include/` 를 고치지 마라. 이유: master 코드는 교육용 결함판이다. 실패하는 테스트는 `expected_failures.txt` 에 올리는 대상이다.
- 결함판에서 통과하도록 기대값을 바꾸지 마라. 이유: 기대값의 출처는 요구사항이지 현재 코드가 아니다.
- 음수 샘플 · count 범위 밖 케이스를 넣지 마라. 이유: 요구사항 밖 동작이다(CLAUDE.md CRITICAL).
- `tests/vsm_test.h` · `Makefile` · `tests/check_junit.py` · `tests/test_speed_monitor.c` 를 고치지 마라. 이유: 이전 step 산출물이고 이 step 범위 밖이다. 고칠 필요가 보이면 blocked 처리한다.
- 기존 테스트를 깨뜨리지 마라
- AC에 없는 기능, 리팩터링, 의존성을 추가하지 마라. 이유: MVP 범위 고정. 아이디어는 `docs/PROPOSALS.md`에 `[ ]` 항목으로만 기록하라.
