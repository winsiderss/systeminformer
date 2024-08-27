@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

set BUILD_TOOL="tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe"

if not exist %BUILD_TOOL% (
    echo Build tool not found. Run build\build_init.cmd first.
    exit /b 1
)

start /B /W "" %BUILD_TOOL% "-cleanup"

pause
