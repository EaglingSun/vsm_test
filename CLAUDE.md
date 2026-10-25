# 프로젝트: VSM(Vehicle Speed Monitor) 예제

> 공통 응답 스타일과 MVP-First Guard 원칙은 글로벌 CLAUDE.md를 따른다.
> 이 파일에는 프로젝트 고유 규칙과, 팀원도 지켜야 하는 Scope Lock만 둔다.

## 기술 스택
- C11 · WinLibs gcc 15.2.0(i686, `C:\Users\Public\vsm-gcc\mingw32\bin`) · mingw32-make — ADR-001
- C 단위 테스트: 외부 프레임워크 없이 `tests/vsm_test.h` + 모듈별 실행 파일 · JUnit XML 출력 — ADR-002
- 판정 · 요약 스크립트(`tests/check_junit.py`)와 하네스 스크립트: Python 3.13 + pytest + ruff
- CI: GitHub Actions `windows-latest` — ADR-003

## 아키텍처 규칙
- CRITICAL: master 의 `src/` · `include/` 는 교육용 결함판이다(요구사항 `test_ref/요구사항.md` 와 일부러 어긋난다). PRD 에 결함 수정 요구사항(R-ID)이 없는 phase 에서는 `src/` · `include/` 를 고치지 않는다.
- CRITICAL: `setup/setup_gcc.ps1` 의 내려받기 주소 · SHA256 과 gcc 위치(`config.bat` · `setup_gcc.ps1` · `verify_build.ps1` 세 곳이 같은 값)를 바꾸지 않는다.
- CRITICAL: 테스트는 요구사항(SR-xxx · §3)이 정한 입력과 동작만 기대값으로 쓴다. 요구사항이 정하지 않은 동작(제동 가산, count 범위 밖 반환값, `ecu_id` 대상 구분 등)을 기대값으로 박지 않는다.
- 결함판에서 실패하는 테스트는 `tests/expected_failures.txt` 에 `<스위트>::<케이스>` 로 올린다. 대조는 `tests/check_junit.py` 가 한다.
- 파일 I/O 는 UTF-8. Makefile 명령은 sh 와 cmd.exe 양쪽에서 돌아야 한다(mingw32-make 는 PATH 에 sh.exe 가 있으면 sh 를 쓴다).

## Scope Lock (MVP-First Guard 적용)
- CRITICAL: 구현 범위는 `docs/PRD.md`의 "MVP 범위"와 현재 step 파일의 "변경 허용 범위"로 한정한다.
- CRITICAL: 리뷰와 검증은 승인된 AC만 기준으로 한다. 결함 수정은 기존 범위 안에서만 한다.
- CRITICAL: 범위 밖 아이디어는 코드로 구현하지 말고 `docs/PROPOSALS.md`에 한 줄씩 추가한다. 승인 표시(`[x]`)는 Human만 한다.
- 범위 검증: `python3 scripts/scope_check.py phases/{task}/step{N}.md`

## Phase 실행
- CRITICAL: 승인된 phase 계획의 구현은 `/run-phase` 절차(execute.py)로만 한다. "계획 실행", "구현 시작", "계획대로 구현해" 같은 요청도 메인 세션에서 직접 코드를 고치지 말고 `/run-phase`를 따른다.
- CRITICAL: execute.py 실행 중(`phases/.running` 있음)에는 메인 세션에서 코드와 문서를 수정하지 않는다. 진행 확인은 `phases/<phase>/index.json`으로 한다.
- Windows 에서는 `py -X utf8 scripts/execute.py <phase>` 로 실행한다. 이유: 이 PC 의 Python 기본 인코딩이 cp949 라 Claude 출력의 한글을 읽다 깨질 수 있다.

## 개발 프로세스
- 테스트를 먼저 쓰고, 테스트를 통과시키는 구현을 쓴다 (TDD).
- 커밋 메시지는 conventional commits 형식을 따른다.

## 명령어
```bash
./make.cmd                  # 데모 빌드 (Git Bash). cmd 창에서는 .\make.cmd
./make.cmd test; python tests/check_junit.py test-results --expect tests/expected_failures.txt   # C 테스트 + 판정
python -m pytest -q         # 하네스 · 판정 스크립트 테스트
ruff check scripts tests    # lint
```

## 하네스 현황
```bash
python3 scripts/harness_map.py            # harness_map.html 생성 (execute.py가 자동 갱신)
python3 scripts/harness_map.py --strict   # Health error 시 exit 1 (CI용)
```
