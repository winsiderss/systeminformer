@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0\.."

REM -----------------------------------------------------------------------------
REM Script: build_init.cmd
REM Description: Bootstraps third-party components and the local build tools.
REM -----------------------------------------------------------------------------

REM Initialize script state and INIT-mode pause suppression.
set "ExitCode=0"
set "IsCI=false"
set "SuppressPause=false"

if /i "%~1"=="INIT" set "SuppressPause=true"

REM Run the main script flow and capture the final exit code.
call :DetectCi
call :Main
if errorlevel 1 set "ExitCode=%errorlevel%"

:end
REM Pause only for interactive, non-CI, non-nested invocations before returning.
if /i "%SuppressPause%"=="false" if /i "%IsCI%"=="false" call :PauseIfInteractive
endlocal & exit /b %ExitCode%

REM -----------------------------------------------------------------------------
REM Function: Main
REM Description: Builds third-party dependencies first, then build tools.
REM -----------------------------------------------------------------------------
:Main
call "%~dp0build_thirdparty.cmd" INIT
if errorlevel 1 exit /b %errorlevel%

call "%~dp0build_tools.cmd" INIT
if errorlevel 1 exit /b %errorlevel%

exit /b 0

REM -----------------------------------------------------------------------------
REM Function: DetectCi
REM Description: Detects whether the script is running under CI.
REM -----------------------------------------------------------------------------
:DetectCi
if /i "%GITHUB_ACTIONS%"=="true" set "IsCI=true"
if /i "%TF_BUILD%"=="true" set "IsCI=true"
exit /b 0

REM -----------------------------------------------------------------------------
REM Function: PauseIfInteractive
REM Description: Pauses only when stdin is attached to an interactive console.
REM -----------------------------------------------------------------------------
:PauseIfInteractive
set "STDIN_REDIRECTED=False"
for /f %%i in ('powershell -NoProfile -Command "[Console]::IsInputRedirected"') do set "STDIN_REDIRECTED=%%i"
if /i not "%STDIN_REDIRECTED%"=="True" pause
exit /b 0
