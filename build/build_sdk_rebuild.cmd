@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

if %PROCESSOR_ARCHITECTURE% EQU ARM64 (
   set "_BUILD_TOOL_ARCH=arm64"
) else (
   set "_BUILD_TOOL_ARCH=x64"
)

start /B /W "" "tools\CustomBuildTool\bin\Release\%_BUILD_TOOL_ARCH%\CustomBuildTool.exe" "-cleansdk"

pause
