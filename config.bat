@echo off
chcp 65001 > nul
rem setup\setup_gcc.ps1 이 받은 gcc · mingw32-make 를 이 창의 PATH 앞에 붙인다 (setup_guide.md)
rem gcc 위치 - setup_gcc.ps1 · verify_build.ps1 과 같은 곳이어야 한다 (바꿀 때는 셋을 같이)
set "VSM_GCC_HOME=%PUBLIC%\vsm-gcc"
set "VSM_GCC=%VSM_GCC_HOME%\mingw32\bin"
if not exist "%VSM_GCC_HOME%\.setup-ok" goto missing
if exist "%VSM_GCC%\gcc.exe" goto ok

:missing
echo.
echo [config] gcc 가 없습니다: %VSM_GCC%
echo   setup_guide.md 순서대로 아래 명령으로 gcc 를 먼저 받으십시오.
echo.
echo   powershell -NoProfile -ExecutionPolicy Bypass -File setup\setup_gcc.ps1
echo.
exit /b 1

:ok
set "PATH=%VSM_GCC%;%PATH%"
for /f "delims=" %%v in ('gcc -dumpfullversion') do set "VSM_GCC_VER=%%v"
echo [config] gcc %VSM_GCC_VER% : %VSM_GCC%
exit /b 0
