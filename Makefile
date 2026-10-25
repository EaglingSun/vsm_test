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

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Windows cmd / PowerShell 기준
clean:
	-cmd.exe /C del /Q $(subst /,\,$(OBJS)) $(TARGET)

# Git Bash 등 POSIX 셸용
clean-sh:
	-rm -f $(OBJS) $(TARGET)

.PHONY: all clean clean-sh
