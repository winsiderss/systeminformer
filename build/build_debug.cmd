@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

if not exist "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" (
    echo CustomBuildTool.exe not found. Run build\build_init.cmd first.
    exit /b 1
)

start /B /W "" "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" "-debug"

pause
