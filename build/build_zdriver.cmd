@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

REM Check if terminal logging is supported
set "TLG=auto"
if "%GITHUB_ACTIONS%"=="true" (
    set "TLG=off"
) else if "%TF_BUILD%"=="true" (
    set "TLG=off"
)

if not exist "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" (
    echo Build tool not found. Run build\build_init.cmd first.
    exit /b 1
)

set BUILD_CONFIGURATION=Debug
set BUILD_TARGET=Build
set PREFAST_ANALYSIS=

:argloop
if not "%1"=="" (
    if "%1"=="debug" (
        set BUILD_CONFIGURATION=Debug
        shift
        goto :argloop
    )
    if "%1"=="release" (
        set BUILD_CONFIGURATION=Release
        shift
        goto :argloop
    )
    if "%1"=="build" (
        set BUILD_TARGET=Build
        shift
        goto :argloop
    )
    if "%1"=="rebuild" (
        set BUILD_TARGET=Rebuild
        shift
        goto :argloop
    )
    if "%1"=="clean" (
        set BUILD_TARGET=Clean
        shift
        goto :argloop
    )
    if "%1"=="prefast" (
        set PREFAST_ANALYSIS=-p:RunCodeAnalysis=true -p:CodeAnalysisTreatWarningsAsErrors=true
        shift
        goto :argloop
    )
    shift
    goto :argloop
)

for /f "usebackq tokens=*" %%A in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
    set VSINSTALLPATH=%%A
)

if not defined VSINSTALLPATH (
    echo [-] Visual Studio not found
    goto end
)

if exist "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" (
   call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" amd64_arm64
) else (
    echo [-] Failed to set up build environment
    goto end
)

echo [+] Building... [WIN64] %BUILD_CONFIGURATION% %BUILD_TARGET% %PREFAST_ANALYSIS% %LEGACY_BUILD%

msbuild KSystemInformer\KSystemInformer.sln -t:%BUILD_TARGET% -p:Configuration=%BUILD_CONFIGURATION%;Platform=x64 -maxCpuCount -terminalLogger:%TLG% -consoleLoggerParameters:Summary;Verbosity=minimal %PREFAST_ANALYSIS%
if %ERRORLEVEL% neq 0 goto end

echo [+] Building... [ARM64] %BUILD_CONFIGURATION% %BUILD_TARGET% %PREFAST_ANALYSIS% %LEGACY_BUILD%

msbuild KSystemInformer\KSystemInformer.sln -t:%BUILD_TARGET% -p:Configuration=%BUILD_CONFIGURATION%;Platform=ARM64 -maxCpuCount -terminalLogger:%TLG% -consoleLoggerParameters:Summary;Verbosity=minimal %PREFAST_ANALYSIS%
if %ERRORLEVEL% neq 0 goto end

echo [+] Build Complete! %BUILD_CONFIGURATION% %BUILD_TARGET% %PREFAST_ANALYSIS% %LEGACY_BUILD%

:end
pause
