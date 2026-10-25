# PRD: VSM 요구사항 테스트 · PR 테스트 리포트

## 목표
VSM 예제 코드가 요구사항(`test_ref/요구사항.md` SR-001 ~ SR-011 · §3)을 지키는지 SR 별 C 단위 테스트로 판정하고, 그 결과(JUnit XML)를 PR 마다 GitHub Actions 아티팩트와 요약 표로 남긴다.

## 사용자
- SQA실장 — 실습 판정표 · 교육 자료
- 실습 수강생 — PR 에서 자기 수정이 어떤 SR 을 통과했는지 확인

## MVP 범위 (승인됨)
| ID | 요구사항 | 완료 기준 (실행 가능한 명령) |
|----|---------|---------------------------|
| R1 | `make test` 가 모듈별 C 테스트를 돌려 `test-results/` 에 JUnit XML 을 쓰고, `tests/check_junit.py` 가 결과를 알려진 실패 목록과 대조하고 요약 표를 만든다 | `./make.cmd test; python tests/check_junit.py test-results --expect tests/expected_failures.txt` |
| R2 | 차속 판정 테스트 — SR-002 · SR-003 · SR-005 · SR-010 · §3 경고 등급 정의 | `./make.cmd test; python tests/check_junit.py test-results --expect tests/expected_failures.txt` |
| R3 | 센서 필터 테스트 — SR-001 · SR-007 · SR-008 · SR-009 | `./make.cmd test; python tests/check_junit.py test-results --expect tests/expected_failures.txt` |
| R4 | 진단 이력 테스트 — SR-011 | `./make.cmd test; python tests/check_junit.py test-results --expect tests/expected_failures.txt` |
| R5 | PR CI — master 대상 pull_request 마다 Windows 러너에서 `make test`, JUnit XML 아티팩트 업로드, SR 별 요약 표 | `python -m pytest -q tests/test_ci_workflow.py` (구조) + PR 실행 후 `gh run download` (실동작) |

## MVP 제외 사항
- `src/` · `include/` 결함 수정 — 다음 phase(로컬 브랜치 전용, 공개 저장소에 올리지 않음)
- SR-006 테스트 — 진단 이력을 읽는 함수가 없어 검사할 수 없다 (PROPOSALS P2)
- SR-004 테스트 — 링 버퍼 내용을 읽는 함수가 없어 「최근 8개」를 검사할 수 없다. `sf_latest()` 만으로는 링 크기를 7 로 줄여도 통과한다 (PROPOSALS P7)
- SR-010 포화(4294967295) 테스트 — 42억 회 호출이 필요하다 (PROPOSALS P3)
- 요구사항이 정하지 않은 동작의 테스트 — 제동 가산, `sf_average` count 범위 밖, `sm_reset` 의 ECU 구분, SR-011 부분 숫자 · 범위 초과 (PROPOSALS P4)
- PR 코멘트 · diff 실패 줄 표시(외부 action)

## 변경 이력
- 2026-10-25: 초안 승인 (phase 0-test-ci)
- 2026-10-25: 0-test-ci 리뷰 반영 — R3 에서 SR-004 제외(검사 불가), R5 gcc 캐시를 복원 · 저장 분리 (ADR-003). step 5 · 6 추가
