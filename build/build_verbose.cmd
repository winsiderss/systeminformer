@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0\.."

REM -----------------------------------------------------------------------------
REM Script: build_verbose.cmd
REM Description: Rebuilds core and plugin solutions with configurable MSBuild verbosity.
REM -----------------------------------------------------------------------------

REM Initialize script state, tool discovery values, and logger settings.
set "ExitCode=0"
set "IsCI=false"
set "TLV=normal"
set "VSINSTALLPATH="
set "VS_ARM64_SUPPORT=false"
set "VCVARS_ARCH=amd64"

REM Run the main script flow and capture the final exit code.
call :DetectCi
call :ConfigureVerbosity "%~1"
call :Main
if errorlevel 1 set "ExitCode=%errorlevel%"

:end
REM Pause only for interactive, non-CI invocations before returning.
if /i "%IsCI%"=="false" call :PauseIfInteractive
endlocal & exit /b %ExitCode%

REM -----------------------------------------------------------------------------
REM Function: Main
REM Description: Discovers the toolchain and rebuilds the selected solution matrix.
REM -----------------------------------------------------------------------------
:Main
call :FindVisualStudio
if errorlevel 1 exit /b %errorlevel%

REM Detect if ARM64 toolchain is available in the current Visual Studio installation.
call :DetectArm64Support
if errorlevel 1 exit /b %errorlevel%

REM Target ARM64 when supported; the host toolset is picked from the machine arch.
if /i "%VS_ARM64_SUPPORT%"=="true" (call :ResolveVcVarsArch arm64) else (call :ResolveVcVarsArch x64)

REM Initialize the Visual Studio build environment for the selected architecture.
call :SetupVcVars "%VCVARS_ARCH%"
if errorlevel 1 exit /b %errorlevel%

REM Set the list of platforms to build for. Add ARM64 if supported.
set "PLATFORMS=Win32;x64"
if /i "%VS_ARM64_SUPPORT%"=="true" set "PLATFORMS=Win32;x64;ARM64"

REM Build the core and plugin solutions for all selected platforms in Debug configuration.
call :RunMsBuild "SystemInformer.sln" "%PLATFORMS%" "SystemInformer.sln [%PLATFORMS%]"
if errorlevel 1 exit /b %errorlevel%
call :RunMsBuild "Plugins\Plugins.sln" "%PLATFORMS%" "Plugins.sln [%PLATFORMS%]"
if errorlevel 1 exit /b %errorlevel%

exit /b 0

REM -----------------------------------------------------------------------------
REM Function: RunMsBuild
REM Description: Rebuilds a solution across the configured platform matrix in a single MSBuild call.
REM Parameters:
REM   %~1 - Solution path.
REM   %~2 - Semicolon-separated platform list (e.g. "Win32;x64;ARM64").
REM   %~3 - Friendly label shown in output.
REM -----------------------------------------------------------------------------
:RunMsBuild
echo:
echo Building %~3
msbuild -mt -p:UseClStructuredOutput=false -t:All -p:TargetConfigurations="Debug;Release" -p:TargetPlatforms="%~2" -p:RestoreUseStaticGraphEvaluation=true -p:CopyRetryCount=10 -p:CopyRetryDelayMilliseconds=200 -verbosity:%TLV% /m /graph %~1
exit /b %errorlevel%

REM -----------------------------------------------------------------------------
REM Function: ConfigureVerbosity
REM Description: Switches to diagnostic verbosity when any argument is supplied.
REM Parameters:
REM   %~1 - Optional verbosity toggle argument.
REM -----------------------------------------------------------------------------
:ConfigureVerbosity
if not "%~1"=="" set "TLV=diagnostic"
exit /b 0

REM -----------------------------------------------------------------------------
REM Function: FindVisualStudio
REM Description: Locates a Visual Studio or SDK installation with MSBuild.
REM -----------------------------------------------------------------------------
:FindVisualStudio
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%a in (`call "%VSWHERE%" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
       set "VSINSTALLPATH=%%a"
    )
)
if not defined VSINSTALLPATH if defined VSINSTALLDIR set "VSINSTALLPATH=%VSINSTALLDIR%"
if not defined VSINSTALLPATH if defined VCINSTALLDIR for %%I in ("%VCINSTALLDIR%\..\..") do set "VSINSTALLPATH=%%~fI"
if not defined VSINSTALLPATH if defined WindowsSdkDir set "VSINSTALLPATH=%WindowsSdkDir%"
if defined VSINSTALLPATH exit /b 0
echo No Visual Studio installation detected.
exit /b 1

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
REM Function: ResolveVcVarsArch
REM Description: Resolves the vcvarsall argument for the requested target, using
REM              the native host toolset (amd64 or arm64) when it is installed.
REM Parameters:
REM   %~1 - Target architecture (x86, x64, arm64).
REM Sets:
REM   VCVARS_HOST - Host toolset selected for vcvarsall (amd64 or arm64).
REM   VCVARS_ARCH - vcvarsall architecture argument (e.g. amd64, arm64_x64).
REM -----------------------------------------------------------------------------
:ResolveVcVarsArch
set "VCVARS_HOST=amd64"
set "VCVARS_TARGET=%~1"
if /i "%VCVARS_TARGET%"=="amd64" set "VCVARS_TARGET=x64"
if /i "%VCVARS_TARGET%"=="win32" set "VCVARS_TARGET=x86"
REM PROCESSOR_ARCHITEW6432 reports the real machine arch when the shell is emulated.
set "VCVARS_MACHINE=%PROCESSOR_ARCHITECTURE%"
if defined PROCESSOR_ARCHITEW6432 set "VCVARS_MACHINE=%PROCESSOR_ARCHITEW6432%"
if /i "%VCVARS_MACHINE%"=="ARM64" (
    REM Only select the ARM64-hosted toolset when Visual Studio has it installed.
    for /d %%D in ("%VSINSTALLPATH%\VC\Tools\MSVC\*") do (
        if exist "%%~D\bin\Hostarm64\" set "VCVARS_HOST=arm64"
    )
)
if /i "%VCVARS_HOST%"=="arm64" (
    if /i "%VCVARS_TARGET%"=="arm64" (set "VCVARS_ARCH=arm64") else (set "VCVARS_ARCH=arm64_%VCVARS_TARGET%")
) else (
    if /i "%VCVARS_TARGET%"=="x64" (set "VCVARS_ARCH=amd64") else (set "VCVARS_ARCH=amd64_%VCVARS_TARGET%")
)
exit /b 0

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
echo Warning: vcvarsall.bat not found and compilation environment variables are not defined.
echo The build may fail if the environment is not properly initialized.
exit /b 1

REM -----------------------------------------------------------------------------
REM Function: DetectCi
REM Description: Detects whether the script is running under CI.
REM -----------------------------------------------------------------------------
:DetectCi
if /i "%GITHUB_ACTIONS%"=="true" set "IsCI=true"
if /i "%TF_BUILD%"=="true" set "IsCI=true"
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
