@echo off
rem Put the project gcc on PATH via config.bat (this call only - setlocal), then run make.
setlocal
call "%~dp0config.bat" || exit /b 1
mingw32-make.exe %*
exit /b %errorlevel%
