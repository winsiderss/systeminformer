@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

if "%1"=="" (
   echo:
   echo Usage: build_zsign.cmd [FILE_PATH] [CONFIGURATION] [PLATFORM]
   echo:
   pause
   goto end
)

if exist "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" (
   echo:
   start /B /W "" "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" "-kphsign" "%1"
   echo:
) else (
   echo CustomBuildTool.exe not found in tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE% folder.
)

:end
