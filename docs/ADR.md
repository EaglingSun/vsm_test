# Architecture Decision Records

> 규칙
> - 각 ADR은 `**Status**: Proposed | Accepted | Superseded by ADR-NNN` 중 하나를 가진다.
> - step은 Accepted ADR만 `## 근거 ADR`에 참조한다. 결정을 바꿀 때는 기존 ADR을 수정하지 말고 새 ADR을 추가한 뒤 기존 ADR을 Superseded로 표시한다.
> - ADR 추가와 Status 변경은 phase 설계 시점에 Human 승인으로만 한다.

## 철학
요구사항 → 테스트 → 증적. 의존성은 최소로 두고, 로컬 결과와 CI 결과가 같아야 한다. 실패는 조용히 넘기지 않는다(결과 파일이 없으면 통과가 아니라 오류다).

---

### ADR-001: C11 + WinLibs gcc 15.2.0(i686) + mingw32-make
**Status**: Accepted
**결정**: 컴파일러는 `setup/setup_gcc.ps1` 이 `C:\Users\Public\vsm-gcc` 에 받는 WinLibs gcc 15.2.0(i686, MSVCRT, SHA256 고정)을 쓴다. 빌드는 `make.cmd`(config.bat → mingw32-make) 경로로만 한다. 플래그는 `-std=c11 -Wall -Wextra -Iinclude`.
**이유**: 실습 환경 그대로다. 한글 경로를 피해 프로젝트 밖에 두고, 같은 PC 의 복사본이 같은 gcc 를 쓴다.
**트레이드오프**: Windows 전용 빌드 스크립트(make.cmd · config.bat)라 Linux 러너를 쓸 수 없다.

### ADR-002: 외부 프레임워크 없는 C 테스트 + C 쪽 JUnit XML 출력 + 알려진 실패 목록
**Status**: Accepted
**결정**:
- 테스트 헤더 `tests/vsm_test.h` 하나(검사 매크로 + JUnit XML 쓰기)만 쓴다. Unity · CMocka 등 프레임워크를 들이지 않는다.
- 모듈마다 실행 파일 하나: `tests/test_<모듈>.c` + `src/<모듈>.c`. 각 실행 파일이 `test-results/test_<모듈>.xml` 을 직접 쓴다.
- XML 판정 · 대조 · 요약 표는 Python `tests/check_junit.py`(표준 라이브러리만)가 한다.
- 결함판(master)에서 실패해야 하는 테스트는 `tests/expected_failures.txt` 에 고정한다. 하네스 검증은 "실제 실패 집합 = 목록" 일 때만 통과한다. 결함 수정 phase 에서는 고친 만큼 목록에서 지운다.
**이유**: 수강생 PC 는 gcc 만으로 `make test` 를 돌릴 수 있다(Python 불필요). 모듈의 static 상태가 섞이지 않는다. 알려진 실패 목록은 "결함판에서 실패해야 할 테스트가 실제로 실패한다"(RED 실증)를 기계 판정으로 남긴다.
**트레이드오프**: XML escape · 파일 쓰기를 직접 구현해야 한다. 한 실행 파일 안 케이스들은 static 상태를 공유하므로 케이스마다 시작 상태를 스스로 맞춰야 한다(센서 링 버퍼는 초기화 함수가 없다).

### ADR-003: GitHub Actions(windows-latest) · 공식 action 만 · 커밋 SHA 고정
**Status**: Accepted
**결정**:
- 워크플로 `.github/workflows/test.yml`. 트리거 `pull_request`(대상 master) + `workflow_dispatch`. `permissions: contents: read`.
- 러너 `windows-latest`. gcc 는 `setup\setup_gcc.ps1` 로 받고 `C:\Users\Public\vsm-gcc` 를 `actions/cache` 로 캐시한다(키에 setup_gcc.ps1 의 SHA256 을 넣는다).
- action 은 GitHub 공식 것만, 커밋 SHA 로 고정한다:
  - `actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1` (v7.0.1)
  - `actions/cache@55cc8345863c7cc4c66a329aec7e433d2d1c52a9` (v6.1.0)
  - `actions/upload-artifact@cf430e030ddbb5b0abf93d22962f4752f3646cd9` (v7.0.2)
- 요약 표는 `python tests/check_junit.py test-results --summary $GITHUB_STEP_SUMMARY` 로 쓴다. 테스트가 실패해도 요약 · 아티팩트 단계는 돈다(`if: ${{ !cancelled() }}`).
- 워크플로 구조는 로컬 pytest(`tests/test_ci_workflow.py`, PyYAML)로 검사한다. 실동작은 PR 실행으로만 확인된다.
**이유**: 로컬과 같은 컴파일러 · 같은 make.cmd 경로라 로컬 결과 = CI 결과다. 공개 저장소라 서드파티 action 공급망 위험을 공식 action + SHA 고정으로 줄인다. 포크 PR(읽기 전용 토큰)에서도 아티팩트와 요약 표는 동작한다.
**트레이드오프**: Windows 러너는 느리고 첫 실행에 gcc 268MB 를 받는다. PR diff 에 실패 줄이 표시되지 않는다.
