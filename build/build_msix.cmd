@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

start /B /W "" "tools\CustomBuildTool\bin\Release\CustomBuildTool.exe" "-msix"

pause
