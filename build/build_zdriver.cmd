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
set "VS_ARM64_SUPPORT=false"
set "VCVARS_ARCH=amd64"
set "CustomBuildTool=tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe"
set "BUILD_CONFIGURATION=Debug;Release"
set "BUILD_PLATFORMS=x64;ARM64"
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

REM Detect if ARM64 toolchain is available in the current Visual Studio installation.
call :DetectArm64Support
if errorlevel 1 exit /b %errorlevel%

REM If ARM64 support is available, set up cross-compiling (amd64_arm64).
if /i "%VS_ARM64_SUPPORT%"=="true" set "VCVARS_ARCH=amd64_arm64"

REM Set up the Visual C++ compiler environment for the target architecture.
REM Configures PATH, INCLUDE, and LIB variables needed by the compiler and MSBuild.
call :SetupVcVars "%VCVARS_ARCH%"
if errorlevel 1 exit /b %errorlevel%

REM Execute the kernel driver build across all configured platforms and configurations.
REM Logs build parameters, invokes MSBuild with multiprocessor support, and validates success.
echo [+] Building... [%BUILD_PLATFORMS%] (%BUILD_CONFIGURATION%) %BUILD_TARGET% %PREFAST_ANALYSIS%
call :RunDriverBuild "%BUILD_PLATFORMS%" %BUILD_TARGET%
if errorlevel 1 exit /b %errorlevel%

REM Report successful completion and return success code.
echo [+] Build Complete! %BUILD_CONFIGURATION% %BUILD_TARGET% %PREFAST_ANALYSIS%
exit /b 0

REM -----------------------------------------------------------------------------
REM Function: RunDriverBuild
REM Description: Builds KSystemInformer across the selected configuration/platform matrix.
REM Parameters:
REM   %~1 - Semicolon-separated platform list (e.g. "x64" or "x64;ARM64").
REM   %~2 - BuildAll target such as Clean, Build, or Rebuild.
REM -----------------------------------------------------------------------------
:RunDriverBuild
msbuild /m /graph KSystemInformer\KSystemInformer.sln -t:All -p:BuildAllTarget=%~2 -p:TargetConfigurations="%BUILD_CONFIGURATION%" -p:TargetPlatforms="%~1" -p:RestoreUseStaticGraphEvaluation=true -p:ContinueOnError=False -p:CopyRetryCount=10 -p:CopyRetryDelayMilliseconds=500 -terminalLogger:%TLG% -consoleLoggerParameters:Summary;Verbosity=minimal %PREFAST_ANALYSIS%
exit /b %errorlevel%

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
if /i "%~1"=="prefast" set "BUILD_CONFIGURATION=Debug" & set "PREFAST_ANALYSIS=-p:RunCodeAnalysis=true -p:CodeAnalysisTreatWarningsAsErrors=true" & shift & goto parseArgs
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
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%A in (`call "%VSWHERE%" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
        set "VSINSTALLPATH=%%A"
    )
)
if not defined VSINSTALLPATH if defined VSINSTALLDIR set "VSINSTALLPATH=%VSINSTALLDIR%"
if not defined VSINSTALLPATH if defined VCINSTALLDIR for %%I in ("%VCINSTALLDIR%\..\..") do set "VSINSTALLPATH=%%~fI"
if not defined VSINSTALLPATH if defined WindowsSdkDir set "VSINSTALLPATH=%WindowsSdkDir%"
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
if /i "%EnterpriseWDK%"=="true" (
    REM EWDK has already configured INCLUDE/LIB/PATH via LaunchBuildEnv.cmd.
    exit /b 0
)
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
REM Function: DetectArm64Support
REM Description: Checks whether the detected Visual Studio install has ARM64 VC tools.
REM -----------------------------------------------------------------------------
:DetectArm64Support
if /i "%EnterpriseWDK%"=="true" (
    REM EWDK shells are single-arch; the launched arch is exposed via Platform.
    if /i "%Platform%"=="arm64" set "VS_ARM64_SUPPORT=true"
    exit /b 0
)
if not exist "%VSWHERE%" exit /b 0
for /f "usebackq tokens=*" %%a in (`call "%VSWHERE%" -latest -prerelease -products * -requires "Microsoft.VisualStudio.Component.VC.Tools.ARM64" -property installationPath`) do (
   set "VS_ARM64_SUPPORT=true"
)
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
