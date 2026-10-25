# VSM(Vehicle Speed Monitor) 예제 코드

차속 센서 값을 걸러 경고 등급을 판정하고 진단 이력에 기록하는 C 예제입니다.
요구사항은 `test_ref/요구사항.md`(SR-001 ~ SR-011)에 있고, 빌드 환경은 `setup_guide.md` 로 맞춥니다.

## 구성

```
(프로젝트 폴더)/
  Makefile           빌드 — CC 는 gcc(C:\Users\Public\vsm-gcc\mingw32\bin) · 산출물 vsm_demo.exe
  config.bat         C:\Users\Public\vsm-gcc\mingw32\bin 을 PATH 앞에 · chcp 65001 — gcc 가 없으면 setup_guide.md 안내
  make.cmd           config.bat 을 부른 뒤 mingw32-make 호출 (PATH 는 그 호출 안에서만 바뀐다)
  setup_guide.md     gcc 받기 · 빌드 확인 순서 — Claude 에게 「setup_guide 읽고 셋업해줘」
  setup/
    setup_gcc.ps1    WinLibs gcc 15.2.0 을 C:\Users\Public\vsm-gcc 에 받는다 (SHA256 확인 · 한글 경로를 피해 프로젝트 밖)
    verify_build.ps1 환경 세팅 · 빌드 성공 확인
  test_ref/
    요구사항.md      요구사항 SR-001 ~ SR-011
    요구사항.xlsx    같은 내용의 엑셀판
  include/
    vsm_types.h      경고 등급 enum(vsm_warn_t) · 차속 한계값 · 샘플 수
    speed_monitor.h  차속 판정
    sensor_filter.h  센서 필터
    diag_log.h       진단 이력
  src/
    main.c           데모 진입점 — 차속 0 ~ 220 을 20 씩 올리며 판정 결과를 출력
    speed_monitor.c  sm_evaluate() · sm_reset() · sm_over_count()
    sensor_filter.c  sf_push() · sf_latest() · sf_average() — 8칸 링 버퍼
    diag_log.c       dl_record() · dl_parse_threshold() · dl_read_status_reg()
```

## 함수

| 함수 | 하는 일 |
|---|---|
| `sm_evaluate(speed_kph, brake_on)` | 차속으로 경고 등급(NONE · LOW · MID · HIGH)을 판정하고 과속 횟수를 센다 |
| `sm_reset(ecu_id)` | 과속 카운터 초기화 |
| `sm_over_count()` | 과속 횟수 |
| `sf_push(value)` · `sf_latest()` | 링 버퍼에 센서 값을 넣고 · 최근 값을 꺼낸다 |
| `sf_average(samples, count)` | 샘플 평균 |
| `dl_record(warn)` | 경고 등급을 진단 이력에 기록 |
| `dl_parse_threshold(text)` | 임계값 문자열을 숫자로 — 해석 못 하면 -1 |
| `dl_read_status_reg()` | 하드웨어 상태 레지스터(`0x40001000`) 읽기 — PC 에서는 호출하지 않는다 |

## 빌드 · 실행

처음 한 번 gcc 를 받는다 — 순서와 문제 해결은 `setup_guide.md`.

```
powershell -NoProfile -ExecutionPolicy Bypass -File setup\setup_gcc.ps1
```

그다음 cmd 창에서 — `make` 가 `config.bat` 을 먼저 부르므로 따로 실행하지 않아도 된다:

```
make clean
make
vsm_demo.exe
```
