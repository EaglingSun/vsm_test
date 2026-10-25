# Step 6: sr004-rename

## 참조 문서

- `docs/ARCHITECTURE.md`
- `docs/ADR.md`

## 근거 ADR

- ADR-002: 외부 프레임워크 없는 C 테스트 + C 쪽 JUnit XML 출력 + 알려진 실패 목록

## 읽어야 할 파일

먼저 아래 파일들을 읽고 프로젝트의 아키텍처와 설계 의도를 파악하라:

- `docs/PRD.md` — R3 에서 SR-004 가 빠졌고, MVP 제외 사항에 SR-004(검사 불가) 가 있다
- `tests/test_sensor_filter.c` — step 2 산출물. 케이스 `SR-004_latest_after_wraparound`
- `tests/expected_failures.txt` — `test_sensor_filter::SR-004_latest_after_wraparound` 항목
- `test_ref/요구사항.md` — SR-004 · SR-009

이전 step에서 만들어진 코드를 꼼꼼히 읽고, 설계 의도를 이해한 뒤 작업하라.

## 작업

### 배경 (결함)

케이스 `SR-004_latest_after_wraparound` 는 8칸을 한 바퀴 넘긴 뒤 `sf_latest()` 가 마지막 값인지만 본다. 링 버퍼 크기를 7 로 줄여도 통과하므로 SR-004(「최근 8개를 유지」)를 검증하지 못한다. 그런데 이름이 SR-004 라 결함을 고친 뒤 판정표에 「SR-004 통과」로 과대 표시된다. 실제로 검증하는 것은 SR-009(가장 최근 값)다.

### 1. `tests/test_sensor_filter.c`

- 케이스 이름 `SR-004_latest_after_wraparound` → `SR-009_latest_after_wraparound`. `vt_run` 의 이름 문자열, 케이스 함수 이름, 바로 위 주석을 함께 바꾼다.
- 주석에 「링 크기(SR-004 8칸)는 이 케이스로 검증되지 않는다 — 버퍼 내용을 읽는 함수가 없다(PROPOSALS P7)」를 적는다.
- 파일 머리 주석에 SR-004 를 검사하지 않는다는 한 줄을 넣는다.
- 케이스의 입력 · 기대값 · 순서는 바꾸지 않는다.

### 2. `tests/expected_failures.txt`

- `test_sensor_filter::SR-004_latest_after_wraparound` → `test_sensor_filter::SR-009_latest_after_wraparound`. 다른 항목은 건드리지 않는다.

## 요구사항 ID

R3

## 변경 허용 범위

이 목록 밖의 파일은 수정하지 마라. scope_check.py가 검사한다.

- `tests/test_sensor_filter.c`
- `tests/expected_failures.txt`

## Acceptance Criteria

```bash
./make.cmd test; python tests/check_junit.py test-results --expect tests/expected_failures.txt                 # sh 경로 — 통과 13 · 알려진 실패 8
./make.cmd test SHELL=cmd.exe; python tests/check_junit.py test-results --expect tests/expected_failures.txt   # cmd.exe 경로
python -m pytest -q
ruff check scripts tests
python scripts/scope_check.py phases/0-test-ci/step6.md
```

## 검증 절차

1. 위 AC 커맨드를 실행한다. 통과 · 알려진 실패 수는 이름만 바뀌므로 13 · 8 그대로여야 한다.
2. `test-results/test_sensor_filter.xml` 에 `SR-004` 로 시작하는 testcase 가 없고 `SR-009_latest_after_wraparound` 가 실패(실제 3)로 있는지 본다.
3. 아키텍처 체크리스트를 확인한다:
   - ARCHITECTURE.md 디렉토리 구조를 따르는가?
   - ADR 기술 스택을 벗어나지 않았는가?
   - CLAUDE.md CRITICAL 규칙을 위반하지 않았는가?
   - "근거 ADR"의 결정을 따랐는가? 새 기술 결정이 필요했다면 구현하지 말고 blocked 처리하라.
4. 결과에 따라 `phases/0-test-ci/index.json`의 해당 step을 업데이트한다:
   - 성공 → `"status": "completed"`, `"summary": "산출물 한 줄 요약"` (바뀐 케이스 이름 · 통과/알려진 실패 수)
   - 수정 3회 시도 후에도 실패 → `"status": "error"`, `"error_message": "구체적 에러 내용"`
   - 사용자 개입 필요 → `"status": "blocked"`, `"blocked_reason": "구체적 사유"` 후 즉시 중단

## 금지사항

- `src/` · `include/` 를 고치지 마라. 이유: master 코드는 교육용 결함판이다. 버퍼를 읽는 함수 추가(P7)도 하지 않는다.
- SR-004 를 검증하는 새 케이스를 만들지 마라. 이유: 공개 API 로는 검증할 수 없고, 이름만 SR-004 인 케이스는 이 결함을 되풀이한다.
- 다른 케이스의 이름 · 입력 · 기대값을 바꾸지 마라. 이유: 이 step 범위는 이름 정정 하나다.
- 기존 테스트를 깨뜨리지 마라
- AC에 없는 기능, 리팩터링, 의존성을 추가하지 마라. 이유: MVP 범위 고정. 아이디어는 `docs/PROPOSALS.md`에 `[ ]` 항목으로만 기록하라.
