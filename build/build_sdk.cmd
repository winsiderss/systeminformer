@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0\.."

REM -----------------------------------------------------------------------------
REM Script: build_sdk.cmd
REM Description: Builds SDK packages for all supported configurations and platforms.
REM -----------------------------------------------------------------------------

REM Initialize script state and tool paths.
set "ExitCode=0"
set "IsCI=false"
set "CustomBuildTool=tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe"

REM Run the main script flow and capture the final exit code.
call :DetectCi
call :Main
if errorlevel 1 set "ExitCode=%errorlevel%"

:end
REM Pause only for interactive, non-CI invocations before returning.
if /i "%IsCI%"=="false" call :PauseIfInteractive
endlocal & exit /b %ExitCode%

REM -----------------------------------------------------------------------------
REM Function: Main
REM Description: Validates prerequisites and builds each SDK variant.
REM -----------------------------------------------------------------------------
:Main
call :CheckCustomBuildTool
if errorlevel 1 exit /b %errorlevel%

call :RunSdkBuild Debug Win32
if errorlevel 1 exit /b %errorlevel%
call :RunSdkBuild Debug x64
if errorlevel 1 exit /b %errorlevel%
call :RunSdkBuild Debug arm64
if errorlevel 1 exit /b %errorlevel%
call :RunSdkBuild Release Win32
if errorlevel 1 exit /b %errorlevel%
call :RunSdkBuild Release x64
if errorlevel 1 exit /b %errorlevel%
call :RunSdkBuild Release arm64
if errorlevel 1 exit /b %errorlevel%

exit /b 0

REM -----------------------------------------------------------------------------
REM Function: RunSdkBuild
REM Description: Invokes the SDK build for a single configuration and platform.
REM Parameters:
REM   %~1 - Build configuration name.
REM   %~2 - Build platform name.
REM -----------------------------------------------------------------------------
:RunSdkBuild
call :RunCustomBuildTool "-sdk" "-%~1" "-%~2" "-verbose"
exit /b %errorlevel%

REM -----------------------------------------------------------------------------
REM Function: CheckCustomBuildTool
REM Description: Ensures the CustomBuildTool executable is available.
REM -----------------------------------------------------------------------------
:CheckCustomBuildTool
if exist "%CustomBuildTool%" exit /b 0
echo CustomBuildTool.exe not found. Run build\build_init.cmd first.
exit /b 1

REM -----------------------------------------------------------------------------
REM Function: RunCustomBuildTool
REM Description: Executes CustomBuildTool with the supplied arguments.
REM Parameters:
REM   %* - Arguments forwarded to CustomBuildTool.
REM -----------------------------------------------------------------------------
:RunCustomBuildTool
start /B /W "" "%CustomBuildTool%" %*
exit /b %errorlevel%

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
