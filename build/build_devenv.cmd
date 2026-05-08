@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0\.."

REM -----------------------------------------------------------------------------
REM Script: build_devenv.cmd
REM Description: Rebuilds third-party, core, and plugin solutions via devenv mode.
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
REM Description: Validates prerequisites and runs the full devenv rebuild matrix.
REM -----------------------------------------------------------------------------
:Main
call :CheckCustomBuildTool
if errorlevel 1 exit /b %errorlevel%

call :RunDevenvBuild "thirdparty [Debug|Win32]" "tools\thirdparty\thirdparty.sln /Rebuild Debug /Project thirdparty /projectconfig ""Debug|Win32"""
if errorlevel 1 exit /b %errorlevel%
call :RunDevenvBuild "thirdparty [Debug|x64]" "tools\thirdparty\thirdparty.sln /Rebuild Debug /Project thirdparty /projectconfig ""Debug|x64"""
if errorlevel 1 exit /b %errorlevel%
call :RunDevenvBuild "thirdparty [Release|Win32]" "tools\thirdparty\thirdparty.sln /Rebuild Release /Project thirdparty /projectconfig ""Release|Win32"""
if errorlevel 1 exit /b %errorlevel%
call :RunDevenvBuild "thirdparty [Release|x64]" "tools\thirdparty\thirdparty.sln /Rebuild Release /Project thirdparty /projectconfig ""Release|x64"""
if errorlevel 1 exit /b %errorlevel%

call :RunDevenvBuild "SystemInformer.sln [Debug|Win32]" "SystemInformer.sln /Rebuild ""Debug|Win32"""
if errorlevel 1 exit /b %errorlevel%
call :RunDevenvBuild "SystemInformer.sln [Debug|x64]" "SystemInformer.sln /Rebuild ""Debug|x64"""
if errorlevel 1 exit /b %errorlevel%
call :RunDevenvBuild "SystemInformer.sln [Release|Win32]" "SystemInformer.sln /Rebuild ""Release|Win32"""
if errorlevel 1 exit /b %errorlevel%
call :RunDevenvBuild "SystemInformer.sln [Release|x64]" "SystemInformer.sln /Rebuild ""Release|x64"""
if errorlevel 1 exit /b %errorlevel%

call :RunDevenvBuild "Plugins.sln [Debug|Win32]" "Plugins\Plugins.sln /Rebuild ""Debug|Win32"""
if errorlevel 1 exit /b %errorlevel%
call :RunDevenvBuild "Plugins.sln [Debug|x64]" "Plugins\Plugins.sln /Rebuild ""Debug|x64"""
if errorlevel 1 exit /b %errorlevel%
call :RunDevenvBuild "Plugins.sln [Release|Win32]" "Plugins\Plugins.sln /Rebuild ""Release|Win32"""
if errorlevel 1 exit /b %errorlevel%
call :RunDevenvBuild "Plugins.sln [Release|x64]" "Plugins\Plugins.sln /Rebuild ""Release|x64"""
if errorlevel 1 exit /b %errorlevel%

exit /b 0

REM -----------------------------------------------------------------------------
REM Function: RunDevenvBuild
REM Description: Runs a single devenv-backed build through CustomBuildTool.
REM Parameters:
REM   %~1 - Friendly display label for the build.
REM   %~2 - Devenv command line passed to CustomBuildTool.
REM -----------------------------------------------------------------------------
:RunDevenvBuild
set "BuildLabel=%~1"
set "BuildCommand=%~2"
echo:
echo Building !BuildLabel!
start /B /W "" "%CustomBuildTool%" "-devenv-build" "!BuildCommand!"
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
