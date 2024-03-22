@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

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

for /f "usebackq tokens=*" %%A in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
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

echo [+] Building... %BUILD_CONFIGURATION% %BUILD_TARGET% %PREFAST_ANALYSIS% %LEGACY_BUILD%

msbuild KSystemInformer\KSystemInformer.sln -t:%BUILD_TARGET% -p:Configuration=%BUILD_CONFIGURATION%;Platform=x64 -maxCpuCount -consoleLoggerParameters:Summary;Verbosity=minimal %PREFAST_ANALYSIS%
if %ERRORLEVEL% neq 0 goto end

msbuild KSystemInformer\KSystemInformer.sln -t:%BUILD_TARGET% -p:Configuration=%BUILD_CONFIGURATION%;Platform=ARM64 -maxCpuCount -consoleLoggerParameters:Summary;Verbosity=minimal %PREFAST_ANALYSIS%
if %ERRORLEVEL% neq 0 goto end

echo [+] Build Complete! %BUILD_CONFIGURATION% %BUILD_TARGET% %PREFAST_ANALYSIS% %LEGACY_BUILD%

:end
pause
