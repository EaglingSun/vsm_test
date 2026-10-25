# VSM 예제 셋업 가이드

> 이 문서는 Claude 가 그대로 실행하는 지시서다. 사용자가 「setup_guide 읽고 셋업해줘」라고 하면
> 아래 「단계」 1 ~ 3 을 순서대로 실행하고 「보고」 형식으로 알린다. 사람이 따라 해도 같다.

## 무엇을 하나

gcc 를 `C:\Users\Public\vsm-gcc` 에 받고, `make` 로 `vsm_demo.exe` 가 빌드되는지 확인한다(`make` 가 `config.bat` 을 먼저 부른다).
압축을 풀기만 한다 — 설치 프로그램 · 레지스트리 · PATH 영구 변경 없음 · 관리자 권한 불필요.

gcc 를 프로젝트 밖 영문 경로에 두는 이유: gcc 를 한글이 든 경로(예: `C:\Users\홍길동\…` · OneDrive `바탕 화면`)에 풀면
링크 단계에서 `ld.exe: cannot find …crt2.o` 로 깨진다. 프로젝트 폴더는 한글 경로에 있어도 된다.

| 항목 | 값 |
|---|---|
| 컴파일러 | WinLibs GCC 15.2.0 + MinGW-w64 14.0.0 · 32비트(i686) · MSVCRT · release 7 |
| 받는 파일 | `winlibs-i686-posix-dwarf-gcc-15.2.0-mingw-w64msvcrt-14.0.0-r7.zip` (268MB · 풀면 약 0.9GB · 받고 풀기까지 실측 24초) |
| 받는 곳 | github.com/brechtsanders/winlibs_mingw 릴리스 `15.2.0posix-14.0.0-msvcrt-r7` |
| SHA256 | `ed7fb2db387834849af00216a609d36b3d65730ba28ca73c7d759360edaeb913` — `setup\setup_gcc.ps1` 에 고정 |
| 설치 위치 | `C:\Users\Public\vsm-gcc\mingw32\bin` — `gcc.exe` · `mingw32-make.exe`. 이 PC 의 모든 복사본이 같이 쓴다 |

## 전제

- Windows 10 1803 이상 또는 Windows 11 — `C:\Windows\System32` 의 `curl.exe` · `tar.exe` 를 쓴다
- 인터넷 — github.com 접속. 사내망에서 막히면 「문제 해결」 첫 줄
- 디스크 여유 2GB
- 이 파일이 있는 폴더가 프로젝트 루트다. 모든 명령은 이 폴더에서 실행한다

## 단계

### 1. gcc 받기

```
powershell -NoProfile -ExecutionPolicy Bypass -File setup\setup_gcc.ps1
```

- `[setup] 완료` 또는 `[setup] 이미 받아 두었습니다` 다음에 gcc · make 버전 두 줄이 나오면 다음 단계로 간다.
- 내려받기와 압축 풀기에 몇 분 걸린다. Claude 는 이 명령의 시간 제한을 10분으로 잡는다.
- `[setup] 실패:` 가 나오면 멈추고 「문제 해결」을 본다.

### 2. 빌드 확인

```
powershell -NoProfile -ExecutionPolicy Bypass -File setup\verify_build.ps1
```

네 항목이 모두 `[PASS]` 이고 마지막 줄이 `결과: 통과` 면 셋업 끝이다.

| 항목 | 보는 것 |
|---|---|
| 1 | `config.bat` 이 성공한다 |
| 2 | gcc 가 `C:\Users\Public\vsm-gcc\mingw32\bin` 의 것으로 잡힌다 |
| 3 | `config.bat` 을 따로 부르지 않아도 `make clean` · `make` 가 성공하고, 빌드 로그에 옛 실습 도구(Suresoft · CodeScroll) 경로가 없다 |
| 4 | `vsm_demo.exe` 가 만들어진다 |

경고 2건(`ecu_id` · `s_history` 를 쓰지 않음)은 예제 코드에 원래 있는 것이다. 고치지 않는다.

### 3. 실행해 보기

```
cmd /d /c ".\vsm_demo.exe"
```

실행 파일은 Windows 기본 DLL 만 쓰므로 gcc 가 PATH 에 없어도 된다. `speed=  0 …` 부터 `speed=220 …` 까지 12줄과 `over_count=1` 이 나오면 된다.
값이 요구사항(`test_ref\요구사항.md`)과 다른 곳이 있는데, 실습에서 다룰 내용이다. 고치지 않는다.

## 보고

끝나면 Claude 는 아래만 알린다.

- gcc 버전 — 1단계의 `[setup] gcc.exe (...) 15.2.0` 줄
- 2단계 결과 네 줄(`[PASS]` / `[FAIL]`)과 마지막 줄
- 실패했다면 실패 메시지 원문, 「문제 해결」에서 해 본 것

## Claude 가 지킬 것

- 명령은 위에 적힌 그대로 실행한다. `config.bat` · `make.cmd` · `vsm_demo.exe` 앞에는 `.\` 를 붙인다 —
  Claude Code 는 `NoDefaultCurrentDirectoryInExePath=1` 을 걸어 두어 cmd 가 현재 폴더에서 실행 파일을 찾지 않는다.
- 다른 gcc(MSYS2 · Chocolatey · winget · 이미 PATH 에 있는 gcc)를 설치하거나 대신 쓰지 않는다.
- `setup_gcc.ps1` 의 주소 · SHA256 을 고치지 않는다. 해시가 맞지 않으면 다른 파일로 바꾸지 말고 멈춰서 보고한다.
- `setx` 등으로 PATH 를 영구히 바꾸지 않는다. PATH 는 `config.bat` 이 그 창(`make.cmd` 가 부르면 그 호출) 안에서만 바꾼다.
- `src\` · `include\` · `Makefile` 은 고치지 않는다. 빌드가 안 되면 고치지 말고 보고한다.
- 2단계가 `결과: 통과` 를 내기 전에는 「셋업 완료」라고 보고하지 않는다.

## 문제 해결

| 증상 | 원인 | 조치 |
|---|---|---|
| `[setup] 실패: curl exit …` | 네트워크 · 프록시 · github 차단 | 브라우저로 위 「받는 곳」에서 zip 을 받아 `C:\Users\Public\vsm-gcc\` 에 두고 `setup_gcc.ps1 -Offline` 으로 다시 실행 |
| `SHA256 불일치` | 받다가 끊김 · 프록시가 내용을 바꿈 | 한 번 더 실행한다. 또 다르면 멈추고 보고 |
| `tar exit …` | 디스크 부족 · 백신이 압축 푸는 파일을 잡음 | 디스크를 확인하고 다시 실행한다. 풀다 만 폴더는 스크립트가 지우고 처음부터 푼다 |
| `[config] gcc 가 없습니다` | 1단계를 안 했거나 끝까지 안 됨 | 1단계를 다시 실행 |
| `'make' is not recognized …` · `'config.bat' is not recognized …` | `.\` 없이 실행 | `.\make.cmd` · `.\config.bat` 으로 실행 |
| `ld.exe: cannot find …crt2.o` · `cannot find -lmingw32` | gcc 가 한글이 든 경로에 있다 | 이 가이드대로면 생기지 않는다. gcc 폴더를 옮겼다면 `C:\Users\Public\vsm-gcc` 를 지우고 1단계부터 |
| 실행 정책 오류(`running scripts is disabled` 등) | 회사 정책(GPO)이 스크립트 실행을 막음 | 우회하지 말고 보고 |

## 사람이 손으로 빌드할 때

cmd 창에서:

```
make clean
make
vsm_demo.exe
```

PowerShell 창에서는 `cmd /c "make clean && make && vsm_demo.exe"`. 사람이 연 창에서는 `.\` 가 없어도 된다.

## 되돌리기 · 공유

- 되돌리기: `C:\Users\Public\vsm-gcc` 폴더를 지운다. 이 PC 의 다른 복사본도 그 gcc 를 쓰니 확인하고 지운다. 그 밖에 남는 것은 없다.
- 공유: 프로젝트 폴더만 보낸다. `*.o` · `vsm_demo.exe` 는 빼도 된다(`.gitignore` 에 있다). 받은 사람은 이 문서 1단계부터 한다.
