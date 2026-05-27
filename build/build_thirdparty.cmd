@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0\.."

REM -----------------------------------------------------------------------------
REM Script: build_thirdparty.cmd
REM Description: Rebuilds third-party dependencies for all supported platforms.
REM -----------------------------------------------------------------------------

REM Initialize script state, tool discovery values, and mode flags.
set "ExitCode=0"
set "IsCI=false"
set "SuppressPause=false"
set "TLG=auto"
set "VerboseLevel=normal"
set "VSINSTALLPATH="
set "VS_ARM64_SUPPORT=false"
set "VCVARS_ARCH=amd64"

if /i "%~1"=="INIT" set "SuppressPause=true"

REM Run the main script flow and capture the final exit code.
call :DetectCi
call :ConfigureTerminalLogger
call :Main
if errorlevel 1 set "ExitCode=%errorlevel%"

:end
REM Pause only for interactive, non-CI, non-nested invocations before returning.
if /i "%SuppressPause%"=="false" if /i "%IsCI%"=="false" call :PauseIfInteractive
endlocal & exit /b %ExitCode%

REM -----------------------------------------------------------------------------
REM Function: Main
REM Description: Discovers the toolchain, cleans outputs, and rebuilds thirdparty.sln.
REM -----------------------------------------------------------------------------
:Main
call :FindVisualStudio
if errorlevel 1 exit /b %errorlevel%

REM Detect if ARM64 toolchain is available.
call :DetectArm64Support
if errorlevel 1 exit /b %errorlevel%

REM If ARM64 support is available, set up cross-compiling (amd64_arm64).
if /i "%VS_ARM64_SUPPORT%"=="true" set "VCVARS_ARCH=amd64_arm64"
REM Initialize the Visual Studio build environment.
call :SetupVcVars "%VCVARS_ARCH%"
if errorlevel 1 exit /b %errorlevel%

REM Clean up previous build.
call :CleanupPath "tools\thirdparty\bin"
if errorlevel 1 exit /b %errorlevel%

call :CleanupPath "tools\thirdparty\obj"
if errorlevel 1 exit /b %errorlevel%

REM Set the list of target platforms to build.
set "TP_PLATFORMS=x86;x64"
if /i "%VS_ARM64_SUPPORT%"=="true" set "TP_PLATFORMS=x86;x64;ARM64"
REM Build the third-party dependencies for all selected platforms and configurations.
call :RunThirdPartyBuild "%TP_PLATFORMS%" "%TP_PLATFORMS%"
if errorlevel 1 exit /b %errorlevel%

echo:
exit /b 0

REM -----------------------------------------------------------------------------
REM Function: RunThirdPartyBuild
REM Description: Builds thirdparty.sln across the Debug+Release x platform matrix in a single MSBuild call.
REM Parameters:
REM   %~1 - Semicolon-separated platform list (e.g. "x86;x64;ARM64").
REM   %~2 - Friendly label shown in output.
REM -----------------------------------------------------------------------------
:RunThirdPartyBuild
echo:
echo Building thirdparty.sln [%~2]
msbuild /m /graph tools\thirdparty\thirdparty.sln -t:All -p:TargetConfigurations="Debug;Release" -p:TargetPlatforms="%~1" -p:RestoreUseStaticGraphEvaluation=true -p:CopyRetryCount=10 -p:CopyRetryDelayMilliseconds=200 -verbosity:%VerboseLevel% -terminalLogger:%TLG%
exit /b %errorlevel%

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
echo vcvarsall.bat not found under %VSINSTALLPATH%.
exit /b 1

REM -----------------------------------------------------------------------------
REM Function: CleanupPath
REM Description: Removes a directory tree if it exists.
REM Parameters:
REM   %~1 - Directory path to remove.
REM -----------------------------------------------------------------------------
:CleanupPath
if exist "%~1" rmdir /S /Q "%~1"
exit /b 0

REM -----------------------------------------------------------------------------
REM Function: DetectCi
REM Description: Detects whether the script is running under CI.
REM -----------------------------------------------------------------------------
:DetectCi
if /i "%GITHUB_ACTIONS%"=="true" set "IsCI=true"
if /i "%GITHUB_ACTIONS%"=="true" set "VerboseLevel=minimal"
if /i "%TF_BUILD%"=="true" set "IsCI=true"
if /i "%TF_BUILD%"=="true" set "VerboseLevel=minimal"
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
