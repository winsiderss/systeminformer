@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0\.."

REM -----------------------------------------------------------------------------
REM Script: build_analyze.cmd
REM Description: Invokes a build with MSVC code analysis (/analyze) enabled through
REM              CustomBuildTool. SARIF logs are written to build\output\logs\analyze.
REM Usage: build_analyze.cmd [debug|release]  (defaults to debug)
REM -----------------------------------------------------------------------------

REM Initialize script state and tool paths.
set "ExitCode=0"
set "IsCI=false"
set "BuildAction=-debug"
set "AnalyzeLogPath=build\output\logs\analyze"
set "CustomBuildTool=tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe"

REM Run the main script flow and capture the final exit code.
call :DetectCi
call :ConfigureAction "%~1"
if errorlevel 1 set "ExitCode=%errorlevel%" & goto :end
call :Main
if errorlevel 1 set "ExitCode=%errorlevel%"

:end
REM Pause only for interactive, non-CI invocations before returning.
if /i "%IsCI%"=="false" call :PauseIfInteractive
endlocal & exit /b %ExitCode%

REM -----------------------------------------------------------------------------
REM Function: Main
REM Description: Validates prerequisites and runs the code analysis build action.
REM -----------------------------------------------------------------------------
:Main
call :CheckCustomBuildTool
if errorlevel 1 exit /b %errorlevel%

call :RunCustomBuildTool "%BuildAction%" "-analyze"
if errorlevel 1 exit /b %errorlevel%

echo:
echo Code analysis SARIF logs written to %AnalyzeLogPath%
exit /b 0

REM -----------------------------------------------------------------------------
REM Function: ConfigureAction
REM Description: Selects the build configuration to analyze.
REM Parameters:
REM   %~1 - Optional configuration argument (debug or release).
REM -----------------------------------------------------------------------------
:ConfigureAction
if "%~1"=="" exit /b 0
if /i "%~1"=="debug" set "BuildAction=-debug" & exit /b 0
if /i "%~1"=="-debug" set "BuildAction=-debug" & exit /b 0
if /i "%~1"=="release" set "BuildAction=-release" & exit /b 0
if /i "%~1"=="-release" set "BuildAction=-release" & exit /b 0
echo Unknown configuration '%~1'. Usage: build_analyze.cmd [debug^|release]
exit /b 1

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
start /B /W "" "%CustomBuildTool%" %~1 %~2
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
