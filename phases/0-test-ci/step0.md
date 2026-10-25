# Step 0: test-infra

## 참조 문서

- `docs/ARCHITECTURE.md`
- `docs/ADR.md`

## 근거 ADR

- ADR-001: C11 + WinLibs gcc 15.2.0 + mingw32-make (make.cmd 경로로만 빌드)
- ADR-002: 외부 프레임워크 없는 C 테스트 + C 쪽 JUnit XML 출력 + 알려진 실패 목록

## 읽어야 할 파일

먼저 아래 파일들을 읽고 프로젝트의 아키텍처와 설계 의도를 파악하라:

- `Makefile` · `make.cmd` · `config.bat` — 빌드 경로. `clean` 이 `cmd.exe /C del` 로 sh · cmd 양쪽에서 도는 방식을 본다
- `include/vsm_types.h` · `include/speed_monitor.h` · `src/speed_monitor.c`
- `test_ref/요구사항.md` — SR-001 ~ SR-011, §3 경고 등급 정의
- `.claude/hooks/stop-verify.sh` — 작업 종료 검증 hook
- `.gitignore`

이 step 은 phase 의 첫 step 이다. 이전 step 산출물은 없다.

## 작업

C 테스트를 돌려 JUnit XML 을 쓰고 그 XML 을 판정하는 최소 end-to-end 를 만든다. 테스트 케이스는 스모크 1개만 넣는다(나머지는 다음 step).

### 1. `tests/vsm_test.h` — 검사 매크로 + JUnit XML 쓰기 (header-only)

인터페이스(이름은 그대로 쓴다. 내부 구현은 재량):

```c
void vt_begin(const char *suite, const char *xml_path);  /* 스위트 시작 */
void vt_run(const char *name, void (*fn)(void));         /* 케이스 하나 실행 */
#define VT_CHECK(cond)                     /* 거짓이면 현재 케이스 실패 + "file:line: cond" 메시지 */
#define VT_CHECK_EQ_INT(expected, actual)  /* long long 으로 비교, 메시지에 기대값 · 실제값 */
int vt_end(void);  /* XML 을 쓰고 stdout 에 요약 1줄. 반환 = 실패 케이스 수 */
```

각 테스트 실행 파일의 `main` 은 `vt_begin` → `vt_run` 여러 번 → `return vt_end() == 0 ? 0 : 1;` 형태다.

반드시 지킬 규칙:
- XML 형식: `<?xml version="1.0" encoding="UTF-8"?>` → `<testsuites>` → `<testsuite name=".." tests=".." failures=".." errors="0">` → 케이스마다 `<testcase classname="<suite>" name="<case>">`, 실패면 그 안에 `<failure message="...">본문</failure>` 하나.
- 한 케이스에서 검사가 여러 번 실패해도 `<failure>` 는 하나다(첫 실패 메시지 + 실패한 검사 수). 실패한 뒤에도 그 케이스의 나머지 검사는 계속 돈다.
- 속성 값과 본문에 들어가는 `& < > " '` 를 escape 한다. 이유: 메시지에 `<` · `&` 가 든 조건식이 들어간다.
- XML 은 `vt_end()` 에서 한 번에 쓴다. 파일을 열지 못하거나 쓰기에 실패하면 stderr 에 이유를 쓰고 `exit(2)` 한다. 이유: 결과 파일이 없는데 통과로 보이면 안 된다.
- 케이스 이름과 메시지는 ASCII 만 쓴다. 이유: Windows 콘솔(cp949)에서 깨지지 않게.

### 2. `tests/test_speed_monitor.c` — 스모크 케이스 1개

- 케이스 `SR-003_speed_151_is_mid`: `sm_evaluate(151u, false) == VSM_WARN_MID`. 결함판에서도 통과하는 케이스다.
- 스위트 이름 `test_speed_monitor`, XML 경로 `test-results/test_speed_monitor.xml`.

### 3. `Makefile` — `test` 타깃

- `tests/test_*.c` 를 `$(wildcard ...)` 로 찾는다. `tests/test_<모듈>.c` 는 `src/<모듈>.c` 와만 링크한다(`src/main.c` 는 링크하지 않는다). 실행 파일은 `tests/test_<모듈>.exe`. 이유: 다음 step 이 테스트 파일만 추가해도 Makefile 을 고치지 않게.
- `make test` 순서: ① `test-results/` 를 지우고 새로 만든다 ② 테스트 실행 파일을 전부 빌드한다 ③ **앞의 것이 실패해도** 모든 실행 파일을 돌린다 ④ 하나라도 0 이 아닌 값으로 끝났으면 make 도 0 이 아닌 값으로 끝난다.
- ①은 반드시 한다. 이유: 빌드가 깨졌을 때 지난 실행의 XML 이 남아 통과처럼 보이면 안 된다.
- 명령은 sh 와 cmd.exe 양쪽에서 돌아야 한다. mingw32-make 는 PATH 에 sh.exe 가 있으면 sh 를, 없으면 cmd.exe 를 쓴다(이 PC 는 cmd 창에서 불러도 Git 의 sh 가 잡힌다). 기존 `clean` 처럼 `cmd.exe /C` 를 명시하는 방식 등으로 양쪽을 맞춘다. AC 의 `SHELL=cmd.exe` 줄이 cmd 쪽을 검사한다.
- `clean` · `clean-sh` 는 테스트 실행 파일과 `test-results/` 도 지운다. `all`(인자 없는 `make`)은 지금처럼 `vsm_demo.exe` 만 만든다.

### 4. `tests/check_junit.py` — 판정 · 대조 · 요약

```
python tests/check_junit.py RESULTS_DIR [--expect FILE] [--summary FILE]
```

- 기대 스위트 = 이 스크립트가 있는 폴더의 `test_*.c` 파일 이름들. 각각 `RESULTS_DIR/<이름>.xml` 이 있어야 하고, 파싱되고, testcase 가 1개 이상이어야 한다. 하나라도 어긋나면 이유를 출력하고 **exit 2**. 이유: 결과 파일 누락 · 손상을 "실패 0건" 으로 읽으면 안 된다.
- 결과 식별자 = `<testsuite name>::<testcase name>`. `<failure>` 또는 `<error>` 가 있으면 실패.
- `--expect` 가 없으면: 실패가 있으면 exit 1, 없으면 exit 0.
- `--expect FILE` 이 있으면: 빈 줄과 `#` 로 시작하는 줄을 빼고 한 줄에 식별자 하나. ⓐ 실패했는데 목록에 없음 ⓑ 목록에 있는데 통과함 ⓒ 목록에 있는데 결과에 그런 케이스가 없음 — 하나라도 있으면 각각 나열하고 exit 1. 셋 다 없으면 "통과 N · 알려진 실패 M" 을 출력하고 exit 0.
- `--summary FILE` 이 있으면 마크다운 표(스위트 · 케이스 · 결과 · 메시지)와 합계를 그 파일에 **덧붙인다**(UTF-8). 판정 exit 코드와 관계없이 쓴다.
- 표준 라이브러리만 쓴다(`xml.etree.ElementTree` · `argparse` · `pathlib`). 파일은 `encoding="utf-8"` 로 읽고 쓴다. 한글을 출력하면 시작할 때 `sys.stdout.reconfigure(encoding="utf-8")`.
- 테스트에서 기대 스위트 폴더를 바꿀 수 있게 `--sources DIR`(기본값 = 스크립트 폴더)을 두어도 된다.

### 5. `tests/test_check_junit.py` — pytest

`tmp_path` 에 실제 JUnit 구조의 XML 과 `test_*.c` 더미 파일을 만들어 `check_junit.py` 를 돌린다. 최소 케이스: 목록 일치(exit 0) · ⓐ · ⓑ · ⓒ(각 exit 1) · XML 누락(exit 2) · XML 파싱 실패(exit 2) · testcase 0개(exit 2) · `--summary` 덧붙이기 · escape 된 메시지(`&lt;` 등) 파싱.

### 6. `tests/expected_failures.txt`

주석 머리말만 둔다(형식 `<스위트>::<케이스>` · "결함판(master)에서 실패해야 하는 테스트"). 항목은 0개다 — 스모크 케이스는 통과한다.

### 7. `.gitignore` 에 `test-results/` 한 줄 추가

### 8. `.claude/hooks/stop-verify.sh` 의 VERIFY 기본값 교체

```
VERIFY="${VERIFY:-./make.cmd test; python tests/check_junit.py test-results --expect tests/expected_failures.txt && ruff check scripts tests && python -m pytest -q}"
```

`VERIFY=` 줄과 바로 위 주석 한 줄만 바꾼다. 나머지(stop_hook_active · phases/.running · 변경 없음 건너뛰기 · exit 2)는 건드리지 않는다.

## 요구사항 ID

R1

## 변경 허용 범위

이 목록 밖의 파일은 수정하지 마라. scope_check.py가 검사한다.

- `Makefile`
- `tests/*`
- `.gitignore`
- `.claude/hooks/stop-verify.sh`

## Acceptance Criteria

```bash
./make.cmd test; python tests/check_junit.py test-results --expect tests/expected_failures.txt                 # sh 경로
./make.cmd test SHELL=cmd.exe; python tests/check_junit.py test-results --expect tests/expected_failures.txt   # cmd.exe 경로
./make.cmd                       # 데모 빌드는 그대로 성공
python -m pytest -q              # check_junit 테스트 + 기존 하네스 테스트
ruff check scripts tests
python scripts/scope_check.py phases/0-test-ci/step0.md
```

## 검증 절차

1. 위 AC 커맨드를 실행한다.
2. RED 를 눈으로 확인한다 — 판정 장치가 실패를 실제로 잡는지:
   - `tests/test_speed_monitor.c` 의 기대값을 잠깐 `VSM_WARN_HIGH` 로 바꾸고 AC 첫 줄을 돌려 check_junit 이 ⓐ(목록에 없는 실패)로 exit 1 하는지 본다.
   - `test-results/test_speed_monitor.xml` 을 지운 뒤 `python tests/check_junit.py test-results --expect tests/expected_failures.txt` 가 exit 2 인지 본다.
   - 원래대로 되돌리고 AC 를 다시 통과시킨다. 두 RED 의 exit 코드를 summary 에 적는다.
3. 아키텍처 체크리스트를 확인한다:
   - ARCHITECTURE.md 디렉토리 구조를 따르는가?
   - ADR 기술 스택을 벗어나지 않았는가?
   - CLAUDE.md CRITICAL 규칙을 위반하지 않았는가?
   - "근거 ADR"의 결정을 따랐는가? 새 기술 결정이 필요했다면 구현하지 말고 blocked 처리하라.
4. 결과에 따라 `phases/0-test-ci/index.json`의 해당 step을 업데이트한다:
   - 성공 → `"status": "completed"`, `"summary": "산출물 한 줄 요약"` (만든 파일 · `vsm_test.h` 인터페이스 이름 · check_junit 인자 · RED 확인 exit 코드)
   - 수정 3회 시도 후에도 실패 → `"status": "error"`, `"error_message": "구체적 에러 내용"`
   - 사용자 개입 필요 (gcc 없음 등) → `"status": "blocked"`, `"blocked_reason": "구체적 사유"` 후 즉시 중단

## 금지사항

- `src/` · `include/` 를 고치지 마라. 이유: master 코드는 교육용 결함판이고, 이 phase 는 결함을 테스트로 드러내는 데까지만 한다.
- Unity · CMocka 등 테스트 프레임워크나 Python 서드파티 패키지를 들이지 마라. 이유: ADR-002 (수강생 PC 에는 gcc 만 있다).
- `check_junit.py` 에서 XML 누락 · 파싱 실패를 빈 결과나 통과로 처리하지 마라. 이유: 결과가 없는 것을 "실패 0건" 으로 읽으면 판정표가 거짓이 된다.
- `config.bat` · `make.cmd` · `setup/` 을 고치지 마라. 이유: gcc 위치 · 해시는 세 파일이 맞물려 있다(CLAUDE.md CRITICAL).
- 기존 테스트를 깨뜨리지 마라
- AC에 없는 기능, 리팩터링, 의존성을 추가하지 마라. 이유: MVP 범위 고정. 아이디어는 `docs/PROPOSALS.md`에 `[ ]` 항목으로만 기록하라.
