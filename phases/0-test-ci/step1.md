# Step 1: speed-monitor-tests

## 참조 문서

- `docs/ARCHITECTURE.md`
- `docs/ADR.md`

## 근거 ADR

- ADR-002: 외부 프레임워크 없는 C 테스트 + C 쪽 JUnit XML 출력 + 알려진 실패 목록

## 읽어야 할 파일

먼저 아래 파일들을 읽고 프로젝트의 아키텍처와 설계 의도를 파악하라:

- `test_ref/요구사항.md` — SR-002 · SR-003 · SR-005 · SR-010, §3 경고 등급 정의
- `include/vsm_types.h` · `include/speed_monitor.h` · `src/speed_monitor.c`
- `tests/vsm_test.h` · `tests/test_speed_monitor.c` · `tests/check_junit.py` · `tests/expected_failures.txt` — step 0 산출물
- `Makefile` 의 `test` 타깃 — step 0 산출물

이전 step에서 만들어진 코드를 꼼꼼히 읽고, 설계 의도를 이해한 뒤 작업하라.

## 작업

`tests/test_speed_monitor.c` 에 차속 판정 요구사항 테스트를 채운다. step 0 의 스모크 케이스 `SR-003_speed_151_is_mid` 는 그대로 둔다.

### 넣을 케이스 (이름 그대로 · 기대값은 요구사항에서 온다)

| 케이스 | 근거 | 입력 → 기대 |
|---|---|---|
| `SEC3_speed_0_is_none` | §3 | `sm_evaluate(0u, false)` → `VSM_WARN_NONE` |
| `SEC3_speed_1_is_low` | §3 | `1` → `VSM_WARN_LOW` |
| `SEC3_speed_150_is_low` | §3 · SR-003 경계 | `150` → `VSM_WARN_LOW` |
| `SR-003_speed_199_is_mid` | SR-003 | `199` → `VSM_WARN_MID` |
| `SR-002_speed_200_is_high` | SR-002 경계(200 이상) | `200` → `VSM_WARN_HIGH` |
| `SR-002_speed_201_is_high` | SR-002 | `201` → `VSM_WARN_HIGH` |
| `SR-002_speed_65535_is_high` | SR-002 · SR-001 상한 | `65535` → `VSM_WARN_HIGH` |
| `SR-010_count_per_high` | SR-010 | `sm_reset(1u)` 뒤 `sm_evaluate(201u, false)` 3회 → `sm_over_count() == 3` |
| `SR-010_count_follows_high_with_brake` | SR-010 | `sm_reset(1u)` 뒤 `w = sm_evaluate(199u, true)` → `sm_over_count() == (w == VSM_WARN_HIGH ? 1 : 0)` |
| `SR-005_reset_clears_counter` | SR-005 | `sm_evaluate(201u, false)` 2회 → `sm_reset(1u)` → `sm_over_count() == 0` |

규칙:
- 판정 케이스는 모두 `brake_on = false` 로 부른다. 이유: 제동이 등급에 주는 영향은 요구사항에 없다.
- `SR-010_count_follows_high_with_brake` 만 제동을 켠다. 기대값은 제동 효과가 아니라 "HIGH 를 반환했으면 카운터가 1 늘어야 한다"는 SR-010 불변식이다. 결과 등급이 무엇이든 그 불변식만 검사한다.
- 카운터를 보는 케이스는 반드시 `sm_reset(1u)` 로 시작한다. 이유: 한 실행 파일 안 케이스들이 static 카운터를 공유한다(ADR-002).
- SR-010 포화(4294967295) 케이스는 넣지 않는다. 이유: 42억 회 호출이 필요하다(PRD 제외 · PROPOSALS P3).

### `tests/expected_failures.txt`

결함판에서 **실제로 실패한** 케이스를 `test_speed_monitor::<케이스>` 로 올린다. 코드 분석상 실패가 예상되는 것은 아래 셋이다:

- `test_speed_monitor::SEC3_speed_0_is_none` — `src/speed_monitor.c` 가 0 km/h 도 LOW(1)로 판정
- `test_speed_monitor::SR-002_speed_200_is_high` — `>` 비교라 200 이 MID(2)
- `test_speed_monitor::SR-010_count_follows_high_with_brake` — 199 + 제동이 HIGH 를 반환하는데 카운터는 0

실제 결과가 이와 다르면 먼저 테스트가 요구사항대로 쓰였는지 다시 본다. 그래도 다르면 실제 결과대로 목록을 두고 차이를 summary 에 적는다.

## 요구사항 ID

R2

## 변경 허용 범위

이 목록 밖의 파일은 수정하지 마라. scope_check.py가 검사한다.

- `tests/test_speed_monitor.c`
- `tests/expected_failures.txt`

## Acceptance Criteria

```bash
./make.cmd test; python tests/check_junit.py test-results --expect tests/expected_failures.txt                 # sh 경로
./make.cmd test SHELL=cmd.exe; python tests/check_junit.py test-results --expect tests/expected_failures.txt   # cmd.exe 경로
python -m pytest -q
ruff check scripts tests
python scripts/scope_check.py phases/0-test-ci/step1.md
```

## 검증 절차

1. 위 AC 커맨드를 실행한다.
2. RED 확인: 목록에 올린 각 케이스의 실패 메시지(기대값 · 실제값)를 `test-results/test_speed_monitor.xml` 에서 읽고, 그 실패가 위에 적은 결함 때문인지 확인한다(예: 200 → 실제 2). 엉뚱한 이유(링크 · 정적 상태 오염)로 실패한 케이스는 테스트를 고친다.
3. 아키텍처 체크리스트를 확인한다:
   - ARCHITECTURE.md 디렉토리 구조를 따르는가?
   - ADR 기술 스택을 벗어나지 않았는가?
   - CLAUDE.md CRITICAL 규칙을 위반하지 않았는가?
   - "근거 ADR"의 결정을 따랐는가? 새 기술 결정이 필요했다면 구현하지 말고 blocked 처리하라.
4. 결과에 따라 `phases/0-test-ci/index.json`의 해당 step을 업데이트한다:
   - 성공 → `"status": "completed"`, `"summary": "산출물 한 줄 요약"` (케이스 수 · 알려진 실패 목록 · 예상과 다른 점)
   - 수정 3회 시도 후에도 실패 → `"status": "error"`, `"error_message": "구체적 에러 내용"`
   - 사용자 개입 필요 → `"status": "blocked"`, `"blocked_reason": "구체적 사유"` 후 즉시 중단

## 금지사항

- `src/` · `include/` 를 고치지 마라. 이유: master 코드는 교육용 결함판이다. 실패하는 테스트는 고치는 대상이 아니라 `expected_failures.txt` 에 올리는 대상이다.
- 결함판에서 통과하도록 기대값을 바꾸지 마라. 이유: 기대값의 출처는 요구사항이지 현재 코드가 아니다.
- 제동 가산 · `ecu_id` 대상 구분처럼 요구사항에 없는 동작을 기대값으로 쓰지 마라. 이유: CLAUDE.md CRITICAL. 필요하면 PROPOSALS 에 기록한다.
- `tests/vsm_test.h` · `Makefile` · `tests/check_junit.py` 를 고치지 마라. 이유: step 0 산출물이고 이 step 범위 밖이다. 고칠 필요가 보이면 blocked 처리한다.
- 기존 테스트를 깨뜨리지 마라
- AC에 없는 기능, 리팩터링, 의존성을 추가하지 마라. 이유: MVP 범위 고정. 아이디어는 `docs/PROPOSALS.md`에 `[ ]` 항목으로만 기록하라.
