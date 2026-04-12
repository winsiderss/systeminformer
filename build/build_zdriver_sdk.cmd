@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0\.."

REM -----------------------------------------------------------------------------
REM Script: build_zdriver_sdk.cmd
REM Description: Signs a driver binary and then builds the SDK package for it.
REM -----------------------------------------------------------------------------

REM Initialize script state and tool paths.
set "ExitCode=0"
set "IsCI=false"
set "CustomBuildTool=tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe"

REM Run the main script flow and capture the final exit code.
call :DetectCi
call :Main "%~1" "%~2" "%~3"
if errorlevel 1 set "ExitCode=%errorlevel%"

:end
REM Pause only for interactive, non-CI invocations before returning.
if /i "%IsCI%"=="false" call :PauseIfInteractive
endlocal & exit /b %ExitCode%

REM -----------------------------------------------------------------------------
REM Function: Main
REM Description: Validates arguments, signs the input file, and builds the SDK output.
REM Parameters:
REM   %~1 - File path to sign.
REM   %~2 - SDK build configuration.
REM   %~3 - SDK target platform.
REM -----------------------------------------------------------------------------
:Main
if "%~1"=="" goto usage
if "%~2"=="" goto usage
if "%~3"=="" goto usage

call :CheckCustomBuildTool
if errorlevel 1 exit /b %errorlevel%

call :RunCustomBuildTool "-kphsign" "%~1"
if errorlevel 1 (
    echo [CustomBuildTool] Failed to sign %~1
    exit /b !errorlevel!
)

call :RunCustomBuildTool "-sdk" "-verbose" "%~2" "%~3"
if errorlevel 1 (
    echo [CustomBuildTool] SDK Failed %~2 %~3
    exit /b !errorlevel!
)

exit /b 0

REM -----------------------------------------------------------------------------
REM Function: Usage
REM Description: Prints command usage information.
REM -----------------------------------------------------------------------------
:Usage
echo:
echo Usage: build_zdriver_sdk.cmd [FILE_PATH] [CONFIGURATION] [PLATFORM]
echo:
exit /b 1

REM -----------------------------------------------------------------------------
REM Function: CheckCustomBuildTool
REM Description: Ensures the CustomBuildTool executable is available.
REM -----------------------------------------------------------------------------
:CheckCustomBuildTool
if exist "%CustomBuildTool%" exit /b 0
echo CustomBuildTool.exe not found in tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE% folder.
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
