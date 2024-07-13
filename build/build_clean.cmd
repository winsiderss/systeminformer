@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

start /B /W "" "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" "-cleanup"

pause
