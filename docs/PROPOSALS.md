# Proposals (범위 밖 제안 백로그)

> 규칙
> - Claude는 `[ ]` 항목만 추가한다. 기존 항목의 수정, 삭제, 승인 표시는 하지 않는다.
> - Human이 `[x]`로 승인하면, 다음 phase 설계(/harness C단계)에서 PRD의 MVP 범위로 옮긴 뒤 구현한다.
> - 형식: `- [ ] P{번호} | {출처 phase/step} | {제안 한 줄} | {이유}`

- [ ] P1 | example/step0 | {예: 실패 시 스크린샷 자동 첨부} | {예: 디버깅 시간 단축}
- [ ] P2 | 0-test-ci/설계 | 진단 이력을 읽는 함수(예: `dl_last()`) 추가 | 지금은 기록을 읽을 수단이 없어 SR-006 을 테스트할 수 없다
- [ ] P3 | 0-test-ci/설계 | 과속 카운터 시작값을 정하는 테스트 전용 수단 | SR-010 포화 경계를 42억 회 호출 없이 검증
- [ ] P4 | 0-test-ci/설계 | 요구사항 보강 — 제동(brake_on) 반영 규칙 · `sm_reset(ecu_id)` 대상 구분 · `sf_average` count 범위 밖 반환값 · SR-011 부분 숫자("12abc") · 범위 초과 처리 | 코드에 동작이 있거나 경계가 모호한데 요구사항이 없다
- [ ] P5 | 0-test-ci/설계 | `scripts/execute.py` 의 claude 호출에 `encoding="utf-8"` 지정 | cp949 기본 환경에서 한글 출력 디코딩 실패 위험 (지금은 `py -X utf8` 로 우회)
- [ ] P6 | 0-test-ci/설계 | 데모 평균을 링 버퍼 값으로 · `dl_read_status_reg()` PC 빌드에서 차단 | 데모 avg 가 항상 45 · PC 에서 부르면 접근 위반
- [ ] P7 | 0-test-ci/step2 | 링 버퍼의 최근 N 개를 읽는 함수(예: `sf_recent(i)`) 추가 | `sf_latest()` 만으로는 버퍼 크기를 판별할 수 없어 SR-004 「최근 8개」를 검증하지 못한다 (임시 복사본에서 링 크기를 7 로 줄인 변이가 통과함)
- [ ] P8 | 0-test-ci/step4 | `.github/workflows/test.yml` checkout 에 `persist-credentials: false` 추가 | 이후 step 이 쓰지 않는 GITHUB_TOKEN 이 `.git/config` 에 남지 않게 한다 (지금은 `contents: read` 라 영향이 작다)
- [ ] P9 | 0-test-ci/review | `.github/workflows/test.yml` 의 C 테스트 step 에 step 단위 `timeout-minutes`(예: 5) | 테스트가 멈추면 job 전체가 20분 뒤 취소(cancelled)되어 `!cancelled()` 인 요약 표 · 아티팩트 step 까지 건너뛴다 — step 시간 초과는 실패로 끝나 결과가 남는다
- [ ] P10 | 0-test-ci/review | `tests/check_junit.py --summary` 가 결과 파일 오류가 있어도 읽힌 스위트의 표는 함께 쓰기 | 지금은 한 스위트 XML 이 없으면 요약에 오류 목록만 나와 나머지 모듈 결과를 PR 에서 볼 수 없다 (exit 2 판정은 그대로)
