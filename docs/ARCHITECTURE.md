# 아키텍처

## 디렉토리 구조
```
include/               # 공개 헤더 — vsm_types.h · speed_monitor.h · sensor_filter.h · diag_log.h
src/                   # 모듈 구현(speed_monitor · sensor_filter · diag_log) + main.c(데모)
tests/                 # C 단위 테스트 + 판정 스크립트
├── vsm_test.h         # 검사 매크로 + JUnit XML 쓰기
├── test_speed_monitor.c
├── test_sensor_filter.c
├── test_diag_log.c
├── expected_failures.txt
├── check_junit.py     # XML 판정 · 알려진 실패 대조 · 요약 표
├── test_check_junit.py
└── test_ci_workflow.py
test-results/          # make test 산출물(JUnit XML) — git 제외
setup/                 # gcc 받기 · 빌드 확인 (PowerShell)
test_ref/              # 요구사항 SR-001 ~ SR-011 (md · xlsx)
scripts/               # 하네스 (execute.py · scope_check.py · harness_map.py + 테스트)
docs/                  # PRD · ARCHITECTURE · ADR · PROPOSALS
phases/                # phase 계획 · 상태
.github/
└── workflows/         # CI — test.yml
.claude/
├── commands/          # /harness · /run-phase · /review · /harness-map
└── hooks/             # stop-verify.sh
```

루트 파일: `Makefile` · `make.cmd`(config.bat 을 부른 뒤 mingw32-make) · `config.bat`(gcc 를 PATH 앞에) · `README.md` · `setup_guide.md` · `CLAUDE.md` · `.gitignore`

## 패턴
- 모듈 = 헤더 1개 + 구현 1개. 상태는 모듈 안 `static` 변수(과속 카운터 · 링 버퍼 · 진단 이력)에 둔다.
- 테스트 실행 파일은 모듈마다 하나다: `tests/test_<모듈>.c` + `src/<모듈>.c` 만 링크한다(`main.c` 제외). 모듈의 static 상태가 다른 모듈 테스트와 섞이지 않는다.
- 테스트 케이스 이름은 ASCII 이며 요구사항 ID 로 시작한다(`SR-002_...` · `SEC3_...`). 결과 식별자는 `<스위트>::<케이스>` (스위트 = 실행 파일 이름, 예: `test_speed_monitor`).

## 데이터 흐름
```
데모:    차속 → sf_push() → sm_evaluate() → dl_record() → printf
테스트:  make test → tests/test_<모듈>.exe → test-results/test_<모듈>.xml (JUnit)
         → tests/check_junit.py ─ 로컬 · 하네스: expected_failures.txt 와 대조
                                 └ CI: $GITHUB_STEP_SUMMARY 요약 표 + 아티팩트 업로드
```

## 상태 관리
- 실행 중 상태는 각 모듈의 static 변수뿐이다. `sm_reset()` 은 과속 카운터만 초기화하고, 센서 링 버퍼와 진단 이력에는 초기화 함수가 없다.
- 테스트 결과는 `test-results/` 에만 쓴다. `make test` 는 시작할 때 이 폴더를 비운다.
