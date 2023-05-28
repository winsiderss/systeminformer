@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

call build\build_tools.cmd
if %ERRORLEVEL% neq 0 goto end

start /B /W "" "tools\CustomBuildTool\bin\Release\CustomBuildTool.exe" "-release"

end:
pause
