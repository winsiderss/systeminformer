@echo off
@setlocal enableextensions
@setlocal enabledelayedexpansion
@cd /d "%~dp0\..\"

set localerror=1

if "%1"=="" (
   echo Usage: build_dyndata.cmd [BUILD_OUTPUT_PATH]
   goto end
)

if exist "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" (
   "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" "-dyndata" "%1"
   set localerror=!errorlevel!
) else (
   echo CustomBuildTool.exe not found in tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE% folder.
)

:end
exit /B %localerror%