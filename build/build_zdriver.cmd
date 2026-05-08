@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0\.."

REM -----------------------------------------------------------------------------
REM Script: build_zdriver.cmd
REM Description: Builds the kernel driver solution for x64 and ARM64 targets.
REM -----------------------------------------------------------------------------

REM Initialize script state, tool discovery values, and argument-controlled modes.
set "ExitCode=0"
set "IsCI=false"
set "SuppressPause=false"
set "TLG=auto"
set "VSINSTALLPATH="
set "CustomBuildTool=tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe"
set "BUILD_CONFIGURATION=Debug"
set "BUILD_TARGET=Build"
set "PREFAST_ANALYSIS="

REM Parse environment and command-line options before running the build.
call :DetectCi
call :ConfigureTerminalLogger
call :ParseArgs %*
if errorlevel 1 (
    set "ExitCode=%errorlevel%"
    goto end
)

call :Main
if errorlevel 1 set "ExitCode=%errorlevel%"

:end
REM Pause only for interactive, non-CI, non-nested invocations before returning.
if /i "%SuppressPause%"=="false" if /i "%IsCI%"=="false" call :PauseIfInteractive
endlocal & exit /b %ExitCode%

REM -----------------------------------------------------------------------------
REM Function: Main
REM Description: Validates prerequisites and builds both supported driver targets.
REM -----------------------------------------------------------------------------
:Main
call :CheckCustomBuildTool
if errorlevel 1 exit /b %errorlevel%

call :FindVisualStudio
if errorlevel 1 exit /b %errorlevel%

call :SetupVcVars "amd64_arm64"
if errorlevel 1 exit /b %errorlevel%

echo [+] Building... [WIN64] %BUILD_CONFIGURATION% %BUILD_TARGET% %PREFAST_ANALYSIS%
msbuild KSystemInformer\KSystemInformer.sln -t:%BUILD_TARGET% -p:Configuration=%BUILD_CONFIGURATION%;Platform=x64 -maxCpuCount -terminalLogger:%TLG% -consoleLoggerParameters:Summary;Verbosity=minimal %PREFAST_ANALYSIS%
if errorlevel 1 exit /b %errorlevel%

echo [+] Building... [ARM64] %BUILD_CONFIGURATION% %BUILD_TARGET% %PREFAST_ANALYSIS%
msbuild KSystemInformer\KSystemInformer.sln -t:%BUILD_TARGET% -p:Configuration=%BUILD_CONFIGURATION%;Platform=ARM64 -maxCpuCount -terminalLogger:%TLG% -consoleLoggerParameters:Summary;Verbosity=minimal %PREFAST_ANALYSIS%
if errorlevel 1 exit /b %errorlevel%

echo [+] Build Complete! %BUILD_CONFIGURATION% %BUILD_TARGET% %PREFAST_ANALYSIS%
exit /b 0

REM -----------------------------------------------------------------------------
REM Function: ParseArgs
REM Description: Parses optional build mode arguments and updates script state.
REM Parameters:
REM   %* - Optional build mode tokens such as release, rebuild, prefast, or nopause.
REM -----------------------------------------------------------------------------
:ParseArgs
if "%~1"=="" exit /b 0
if /i "%~1"=="debug" set "BUILD_CONFIGURATION=Debug" & shift & goto parseArgs
if /i "%~1"=="release" set "BUILD_CONFIGURATION=Release" & shift & goto parseArgs
if /i "%~1"=="build" set "BUILD_TARGET=Build" & shift & goto parseArgs
if /i "%~1"=="rebuild" set "BUILD_TARGET=Rebuild" & shift & goto parseArgs
if /i "%~1"=="clean" set "BUILD_TARGET=Clean" & shift & goto parseArgs
if /i "%~1"=="prefast" set "PREFAST_ANALYSIS=-p:RunCodeAnalysis=true -p:CodeAnalysisTreatWarningsAsErrors=true" & shift & goto parseArgs
if /i "%~1"=="nopause" set "SuppressPause=true" & shift & goto parseArgs
shift
goto parseArgs

REM -----------------------------------------------------------------------------
REM Function: CheckCustomBuildTool
REM Description: Ensures the CustomBuildTool executable is available.
REM -----------------------------------------------------------------------------
:CheckCustomBuildTool
if exist "%CustomBuildTool%" exit /b 0
echo Build tool not found. Run build\build_init.cmd first.
exit /b 1

REM -----------------------------------------------------------------------------
REM Function: FindVisualStudio
REM Description: Locates a Visual Studio or SDK installation with MSBuild.
REM -----------------------------------------------------------------------------
:FindVisualStudio
for /f "usebackq tokens=*" %%A in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
    set "VSINSTALLPATH=%%A"
)
if not defined VSINSTALLPATH if defined WindowsSdkDir set "VSINSTALLPATH=%WindowsSdkDir%"
if not defined VSINSTALLPATH if defined EWDK_ROOT set "VSINSTALLPATH=%EWDK_ROOT%"
if defined VSINSTALLPATH exit /b 0
echo [-] Visual Studio not found
exit /b 1

REM -----------------------------------------------------------------------------
REM Function: SetupVcVars
REM Description: Initializes the Visual C++ build environment for the requested arch.
REM Parameters:
REM   %~1 - vcvarsall architecture argument.
REM -----------------------------------------------------------------------------
:SetupVcVars
if exist "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" (
    call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" %~1
    exit /b !errorlevel!
)
if not defined INCLUDE (
    if not defined LIB (
        echo Warning: vcvarsall.bat not found and compilation environment variables ^(INCLUDE/LIB^) are not defined.
        echo The build may fail if the environment is not properly initialized.
        exit /b 1
    )
)
exit /b 0

REM -----------------------------------------------------------------------------
REM Function: DetectCi
REM Description: Detects whether the script is running under CI.
REM -----------------------------------------------------------------------------
:DetectCi
if /i "%GITHUB_ACTIONS%"=="true" set "IsCI=true"
if /i "%TF_BUILD%"=="true" set "IsCI=true"
exit /b 0

REM -----------------------------------------------------------------------------
REM Function: ConfigureTerminalLogger
REM Description: Disables the terminal logger when running under CI.
REM -----------------------------------------------------------------------------
:ConfigureTerminalLogger
if /i "%IsCI%"=="true" set "TLG=off"
exit /b 0

REM -----------------------------------------------------------------------------
REM Function: PauseIfInteractive
REM Description: Pauses only when stdin is attached to an interactive console.
REM -----------------------------------------------------------------------------
:PauseIfInteractive
set "STDIN_REDIRECTED=False"
for /f %%i in ('powershell -NoProfile -Command "[Console]::IsInputRedirected"') do set "STDIN_REDIRECTED=%%i"
if /i not "%STDIN_REDIRECTED%"=="True" pause
exit /b 0
