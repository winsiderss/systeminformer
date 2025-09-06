@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

if "%1"=="" (
   echo:
   echo Usage: build_zdriver_sdk.cmd [FILE_PATH] [CONFIGURATION] [PLATFORM]
   echo:
   pause
   goto end
)
if "%2"=="" (
   echo:
   echo Usage: build_zdriver_sdk.cmd [FILE_PATH] [CONFIGURATION] [PLATFORM]
   echo:
   pause
   goto end
)
if "%3"=="" (
   echo:
   echo Usage: build_zdriver_sdk.cmd [FILE_PATH] [CONFIGURATION] [PLATFORM]
   echo:
   pause
   goto end
)

if exist "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" (
    start /B /W "" "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" "-kphsign" %1
    if %ERRORLEVEL% neq 0 (
        echo "[CustomBuildTool] Failed to sign %1"
    )  
    start /B /W "" "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" "-sdk" "-verbose" %2 %3
    if %ERRORLEVEL% neq 0 (
        echo "[CustomBuildTool] SDK Failed %2 %3"
    )
) else (
   echo CustomBuildTool.exe not found in tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE% folder.
)

:end
