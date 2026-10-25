# Step 5: ci-cache-split

## 참조 문서

- `docs/ARCHITECTURE.md`
- `docs/ADR.md`

## 근거 ADR

- ADR-003: GitHub Actions(windows-latest) · 공식 action 만 · 커밋 SHA 고정 — gcc 캐시는 복원 · 저장 분리

## 읽어야 할 파일

먼저 아래 파일들을 읽고 프로젝트의 아키텍처와 설계 의도를 파악하라:

- `docs/ADR.md` 의 ADR-003 — 캐시를 복원 · 저장으로 나누는 이유와 action 4개의 커밋 SHA
- `.github/workflows/test.yml` — step 4 산출물. 지금은 `actions/cache@<sha>` 한 step 으로 캐시한다
- `tests/test_ci_workflow.py` — step 4 산출물. 워크플로 구조 검사 9건
- `setup/setup_gcc.ps1` — `$Sha256`, `.setup-ok` 표시가 있으면 아무것도 하지 않는 동작

이전 step에서 만들어진 코드를 꼼꼼히 읽고, 설계 의도를 이해한 뒤 작업하라.

## 작업

### 배경 (결함)

`actions/cache@55cc834…`(v6.1.0) 의 `action.yml` 은 `post-if: "success()"` 다. job 이 성공해야만 캐시를 저장한다. master 코드는 교육용 결함판이라 PR 테스트가 늘 실패하므로, 지금 워크플로로는 gcc 캐시가 한 번도 저장되지 않고 매 실행 268MB 를 새로 받는다. 같은 저장소 · 같은 커밋의 하위 action `actions/cache/restore` 와 `actions/cache/save` 는 post 단계가 없어 각 step 이 놓인 자리에서 돈다.

### 1. `.github/workflows/test.yml`

기존 `gcc 캐시` step(`actions/cache@...`)을 지우고 아래처럼 바꾼다. 나머지 step 은 그대로 둔다.

| 순서 | step | 내용 |
|---|---|---|
| 1 | checkout | 그대로 |
| 2 | gcc 캐시 복원 | `id: gcc-cache`, `uses: actions/cache/restore@55cc8345863c7cc4c66a329aec7e433d2d1c52a9  # v6.1.0`, `path: C:\Users\Public\vsm-gcc`, `key` 는 지금과 같은 값(`vsm-gcc-<setup_gcc.ps1 $Sha256>`) |
| 3 | gcc 받기 | 그대로 |
| 4 | gcc 캐시 저장 | `if: steps.gcc-cache.outputs.cache-hit != 'true'`, `uses: actions/cache/save@55cc8345863c7cc4c66a329aec7e433d2d1c52a9  # v6.1.0`, 같은 `path` · `key` |
| 5 ~ | C 테스트 · 요약 표 · 아티팩트 | 그대로 |

반드시 지킬 규칙:
- 저장 step 은 **C 테스트 step 보다 앞**에 둔다. 이유: 테스트가 실패해도 캐시는 이미 저장돼 있어야 한다.
- 저장 step 의 `if` 는 캐시가 없었을 때만이다. 이유: 같은 키가 이미 있으면 저장이 실패(이미 존재)로 끝나 job 이 빨갛게 된다.
- `actions/cache@<sha>`(하위 경로 없는 형태)를 남기지 않는다. 이유: 그 형태는 job 성공 때만 저장한다.

### 2. `tests/test_ci_workflow.py`

- 허용하는 `uses` 형태를 `actions/(checkout|cache/restore|cache/save|upload-artifact)@<40자 hex>` 로 바꾼다. `uses` 집합 = `docs/ADR.md` 의 같은 형태 집합(지금처럼 ADR 이 원천이다).
- 캐시 키 검사를 복원 · 저장 두 step 모두에 건다(`setup/setup_gcc.ps1` 의 `$Sha256` 이 키에 들어 있다).
- 새 검사 2건:
  - 저장 step 이 C 테스트 step(`.\make.cmd test`) **앞**에 있다.
  - 저장 step 의 `if` 에 `steps.<복원 step id>.outputs.cache-hit` 와 `'true'` 가 있다(복원 step 의 `id` 를 읽어 대조한다).
- `actions/cache@`(하위 경로 없음)가 워크플로 어디에도 없다는 검사를 넣는다.

## 요구사항 ID

R5

## 변경 허용 범위

이 목록 밖의 파일은 수정하지 마라. scope_check.py가 검사한다.

- `.github/workflows/*`
- `tests/test_ci_workflow.py`

## Acceptance Criteria

```bash
python -m pytest -q tests/test_ci_workflow.py
python -m pytest -q
ruff check scripts tests
./make.cmd test; python tests/check_junit.py test-results --expect tests/expected_failures.txt
python scripts/scope_check.py phases/0-test-ci/step5.md
```

## 검증 절차

1. 위 AC 커맨드를 실행한다. 시작 시점에는 `docs/ADR.md` 가 이미 복원 · 저장 형태로 바뀌어 있어 `test_uses_pinned_to_adr_sha` 가 실패하는 상태다 — 이 step 이 끝나면 통과해야 한다.
2. RED 를 눈으로 확인한다 — 아래 셋을 하나씩 잠깐 바꿔 `python -m pytest -q tests/test_ci_workflow.py` 가 실패하는지 보고 되돌린다:
   - 저장 step 을 C 테스트 step 뒤로 옮긴다
   - 저장 step 의 `if` 를 지운다
   - 복원 · 저장 두 step 을 예전 `actions/cache@<sha>` 한 step 으로 되돌린다
   되돌린 뒤 AC 를 다시 통과시키고, 세 RED 결과를 summary 에 적는다.
3. 아키텍처 체크리스트를 확인한다:
   - ARCHITECTURE.md 디렉토리 구조를 따르는가?
   - ADR 기술 스택을 벗어나지 않았는가?
   - CLAUDE.md CRITICAL 규칙을 위반하지 않았는가?
   - "근거 ADR"의 결정을 따랐는가? 새 기술 결정이 필요했다면 구현하지 말고 blocked 처리하라.
4. 결과에 따라 `phases/0-test-ci/index.json`의 해당 step을 업데이트한다:
   - 성공 → `"status": "completed"`, `"summary": "산출물 한 줄 요약"` (바뀐 step 구성 · 검사 수 · RED 결과). GitHub 실제 실행은 이 step 에서 확인할 수 없다는 점도 적는다.
   - 수정 3회 시도 후에도 실패 → `"status": "error"`, `"error_message": "구체적 에러 내용"`
   - 사용자 개입 필요 → `"status": "blocked"`, `"blocked_reason": "구체적 사유"` 후 즉시 중단

## 금지사항

- `git push` · PR 생성 · `gh workflow run` 을 하지 마라. 이유: 공개 저장소에 올리는 일은 phase 가 끝난 뒤 사람 확인을 받고 메인 세션이 한다.
- `docs/ADR.md` 를 고치지 마라. 이유: ADR 은 사람이 승인한 원천이고 이미 이 step 에 맞게 바뀌어 있다. 맞지 않는 점이 보이면 blocked 처리한다.
- `save-always` 입력을 쓰지 마라. 이유: v6.1.0 action.yml 에 "does not work as intended and will be removed" 로 폐기 예고돼 있다.
- 요약 표 · 아티팩트 · C 테스트 step 과 트리거 · 권한 · 러너를 바꾸지 마라. 이유: 이 step 범위는 캐시뿐이다.
- 기존 테스트를 깨뜨리지 마라
- AC에 없는 기능, 리팩터링, 의존성을 추가하지 마라. 이유: MVP 범위 고정. 아이디어는 `docs/PROPOSALS.md`에 `[ ]` 항목으로만 기록하라.
