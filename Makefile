# 차속 감시(VSM) 예제 - 빌드
#
# CC 는 C:\Users\Public\vsm-gcc\mingw32\bin 의 gcc 다 (setup_guide.md 대로 setup\setup_gcc.ps1 이 받는다).
# make.cmd 가 config.bat 으로 그 폴더를 PATH 앞에 붙인 뒤 같은 폴더의 mingw32-make.exe 를 부른다.
#
CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Iinclude

TARGET  = vsm_demo.exe
SRCS    = src/main.c src/speed_monitor.c src/sensor_filter.c src/diag_log.c
OBJS    = $(SRCS:.c=.o)

# 단위 테스트: tests/test_<모듈>.c + src/<모듈>.c -> tests/test_<모듈>.exe (main.c 는 링크하지 않는다)
TEST_SRCS = $(wildcard tests/test_*.c)
TEST_EXES = $(TEST_SRCS:.c=.exe)
TEST_RUNS = $(TEST_EXES:tests/%.exe=run-%)
TEST_HDRS = $(wildcard include/*.h) tests/vsm_test.h

# 레시피가 sh(Git) 로 돌 때 MSYS 가 cmd.exe 의 /C · /S · /Q 를 C:/ 같은 경로로 바꾸지 않게 한다
# (바뀌면 cmd 가 명령 없이 대화형으로 떠서 멈춘다). 레시피가 cmd.exe 로 돌 때는 영향이 없다.
export MSYS_NO_PATHCONV := 1
export MSYS2_ARG_CONV_EXCL := *

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# make test: ① test-results/ 를 지우고 새로 만든다 ② 테스트 실행 파일을 전부 빌드한다
#            ③ 앞의 것이 실패해도 모든 실행 파일을 돌린다(-k) ④ 하나라도 실패하면 make 도 0 이 아닌 값으로 끝난다.
# ①을 빌드보다 먼저 하는 이유: 빌드가 깨졌을 때 지난 실행의 XML 이 남아 통과처럼 보이면 안 된다.
# 지우지 못해 폴더가 남아 있으면 mkdir 이 실패해 make 가 멈춘다. 판정은 tests/check_junit.py 가 XML 로 한다.
test:
	cmd.exe /C "if exist test-results rmdir /S /Q test-results"
	cmd.exe /C mkdir test-results
	$(MAKE) --no-print-directory $(TEST_EXES)
	$(MAKE) --no-print-directory -k $(TEST_RUNS)

$(TEST_EXES): tests/test_%.exe: tests/test_%.c src/%.c $(TEST_HDRS)
	$(CC) $(CFLAGS) $(filter %.c,$^) -o $@

$(TEST_RUNS): run-%: tests/%.exe
	$<

# Windows cmd / PowerShell 기준
clean:
	-cmd.exe /C del /Q $(subst /,\,$(OBJS) $(TEST_EXES)) $(TARGET)
	-cmd.exe /C "if exist test-results rmdir /S /Q test-results"

# Git Bash 등 POSIX 셸용
clean-sh:
	-rm -f $(OBJS) $(TARGET) $(TEST_EXES)
	-rm -rf test-results

.PHONY: all clean clean-sh test $(TEST_RUNS)
