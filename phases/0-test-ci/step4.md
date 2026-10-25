# Step 4: ci-workflow

## 참조 문서

- `docs/ARCHITECTURE.md`
- `docs/ADR.md`

## 근거 ADR

- ADR-001: C11 + WinLibs gcc 15.2.0 + mingw32-make (make.cmd 경로로만 빌드)
- ADR-003: GitHub Actions(windows-latest) · 공식 action 만 · 커밋 SHA 고정

## 읽어야 할 파일

먼저 아래 파일들을 읽고 프로젝트의 아키텍처와 설계 의도를 파악하라:

- `docs/ADR.md` 의 ADR-003 — action 3개의 커밋 SHA · 트리거 · 권한
- `setup/setup_gcc.ps1` — gcc 받는 곳 · `$Sha256` · `.setup-ok` 표시가 있으면 아무것도 하지 않는 동작
- `make.cmd` · `config.bat` · `Makefile` 의 `test` 타깃
- `tests/check_junit.py` — `--summary` 인자 (step 0 산출물)
- `tests/expected_failures.txt` — 결함판의 알려진 실패 (step 1 ~ 3 산출물)

이전 step에서 만들어진 코드를 꼼꼼히 읽고, 설계 의도를 이해한 뒤 작업하라.

## 작업

### 1. `.github/workflows/test.yml`

| 항목 | 값 |
|---|---|
| `name` | `test` |
| 트리거 | `pull_request`(branches: `master`) + `workflow_dispatch` |
| `permissions` | 최상위에 `contents: read` 하나만 |
| job | `test` · `runs-on: windows-latest` · `timeout-minutes: 20` |

step 순서:
1. checkout — `actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1  # v7.0.1`
2. gcc 캐시 — `actions/cache@55cc8345863c7cc4c66a329aec7e433d2d1c52a9  # v6.1.0`, `path: C:\Users\Public\vsm-gcc`, `key` 에 `setup/setup_gcc.ps1` 의 SHA256 64자를 그대로 넣는다(예: `vsm-gcc-<SHA256>`). 이유: 받는 파일이 바뀌면 캐시도 바뀌어야 한다.
3. gcc 받기 — `powershell -NoProfile -ExecutionPolicy Bypass -File setup\setup_gcc.ps1`. 캐시가 있으면 스크립트가 `.setup-ok` 를 보고 건너뛴다.
4. C 테스트 — `id: test`, `shell: cmd`, `run: .\make.cmd test`. 테스트가 실패하면 이 step 이 실패하고 job 도 실패한다(그대로 둔다 — PR 에 빨간 표시가 나는 것이 맞다).
5. 요약 표 — `if: ${{ !cancelled() }}`, `python tests/check_junit.py test-results --summary <$GITHUB_STEP_SUMMARY 경로>`. 셸에 맞게 환경변수를 넘긴다(pwsh 면 `"$env:GITHUB_STEP_SUMMARY"`).
6. 아티팩트 — `if: ${{ !cancelled() }}`, `actions/upload-artifact@cf430e030ddbb5b0abf93d22962f4752f3646cd9  # v7.0.2`, `name: junit-xml`, `path: test-results/*.xml`, `if-no-files-found: error`.

반드시 지킬 규칙:
- 5 · 6 은 4 가 실패해도 돈다. 이유: 실패한 PR 이야말로 결과 표와 XML 이 필요하다.
- `if-no-files-found: error`. 이유: XML 이 하나도 없으면 빌드가 깨진 것이다. 빈 아티팩트로 통과시키지 않는다.
- CI 에서는 `--expect` 를 쓰지 않는다. 이유: CI 는 "실제 결과"를 보여 주는 곳이다. 알려진 실패 대조는 하네스(로컬) 검증용이다.
- `uses:` 는 위 3개만, 반드시 40자 커밋 SHA 로 쓴다. 태그(`@v7`)로 쓰지 않는다. 이유: 공개 저장소의 공급망 위험(ADR-003).
- secrets · `pull_request_target` 을 쓰지 마라. 이유: 포크 PR 코드가 쓰기 권한 토큰과 함께 도는 경로를 만들지 않는다.

### 2. `tests/test_ci_workflow.py` — 워크플로 구조 검사 (pytest + PyYAML)

`.github/workflows/test.yml` 을 `yaml.safe_load` 로 읽어 아래를 검사한다:
- 트리거: `pull_request.branches` 에 `master`, `workflow_dispatch` 있음. **주의**: PyYAML 은 키 `on` 을 불리언 `True` 로 읽는다 — `wf.get("on", wf.get(True))` 처럼 둘 다 본다.
- `permissions == {"contents": "read"}`, `runs-on == "windows-latest"`.
- 모든 `uses:` 가 `actions/(checkout|cache|upload-artifact)@<40자 hex>` 이고, 그 SHA 집합이 `docs/ADR.md` 에 적힌 `actions/...@<sha>` 집합과 같다. 이유: SHA 의 원천은 ADR 이다. 테스트에 SHA 를 다시 적지 않는다.
- cache 의 `key` 에 `setup/setup_gcc.ps1` 의 `$Sha256` 값이 들어 있다(대소문자 무시). 값은 테스트가 그 파일에서 읽는다.
- `.\make.cmd test` 를 실행하는 step 이 있다.
- 요약 표 step 의 run 에 `check_junit.py test-results --summary` 가 있고, 요약 · 아티팩트 step 의 `if` 에 `!cancelled()` 가 있다.
- 아티팩트 step: `path` 가 `test-results/*.xml`, `if-no-files-found` 가 `error`.

## 요구사항 ID

R5

## 변경 허용 범위

이 목록 밖의 파일은 수정하지 마라. scope_check.py가 검사한다.

- `.github/workflows/*`
- `tests/test_ci_workflow.py`

## Acceptance Criteria

```bash
python -m pytest -q tests/test_ci_workflow.py
./make.cmd test; python tests/check_junit.py test-results --expect tests/expected_failures.txt --summary test-results/summary.md   # 로컬 판정 + 요약 표 생성
test -s test-results/summary.md   # 요약 표가 비어 있지 않음
python -m pytest -q
ruff check scripts tests
python scripts/scope_check.py phases/0-test-ci/step4.md
```

## 검증 절차

1. 위 AC 커맨드를 실행한다.
2. RED 확인: `test.yml` 에서 checkout 의 SHA 를 잠깐 `@v7` 로 바꾸고 `python -m pytest -q tests/test_ci_workflow.py` 가 실패하는지 본다. 요약 step 의 `if` 를 잠깐 지우고도 실패하는지 본다. 둘 다 되돌리고 AC 를 다시 통과시킨다.
3. `test-results/summary.md` 를 열어 표가 스위트 · 케이스 · 결과 · 메시지로 읽히는지 눈으로 본다.
4. 아키텍처 체크리스트를 확인한다:
   - ARCHITECTURE.md 디렉토리 구조를 따르는가?
   - ADR 기술 스택을 벗어나지 않았는가?
   - CLAUDE.md CRITICAL 규칙을 위반하지 않았는가?
   - "근거 ADR"의 결정을 따랐는가? 새 기술 결정이 필요했다면 구현하지 말고 blocked 처리하라.
5. 결과에 따라 `phases/0-test-ci/index.json`의 해당 step을 업데이트한다:
   - 성공 → `"status": "completed"`, `"summary": "산출물 한 줄 요약"` (워크플로 step 구성 · 검사 항목 수 · RED 확인 결과). GitHub 에서의 실제 실행은 이 step 에서 확인할 수 없다는 점도 적는다.
   - 수정 3회 시도 후에도 실패 → `"status": "error"`, `"error_message": "구체적 에러 내용"`
   - 사용자 개입 필요 → `"status": "blocked"`, `"blocked_reason": "구체적 사유"` 후 즉시 중단

## 금지사항

- `git push` · PR 생성 · `gh workflow run` 을 하지 마라. 이유: 공개 저장소에 올리는 일은 phase 가 끝난 뒤 사람 확인을 받고 메인 세션이 한다(이 PC 의 기본 GitHub 계정은 쓰기 권한도 없다).
- 서드파티 action(예: action-junit-report · test-reporter) · PR 코멘트 기능을 넣지 마라. 이유: ADR-003, PRD 제외 사항.
- `setup/` · `make.cmd` · `config.bat` · `Makefile` · `tests/check_junit.py` 를 고치지 마라. 이유: 이 step 범위 밖이다. 워크플로가 그 파일들의 동작을 바꿔야만 돌 것 같으면 blocked 처리한다.
- 기존 테스트를 깨뜨리지 마라
- AC에 없는 기능, 리팩터링, 의존성을 추가하지 마라. 이유: MVP 범위 고정. 아이디어는 `docs/PROPOSALS.md`에 `[ ]` 항목으로만 기록하라.
