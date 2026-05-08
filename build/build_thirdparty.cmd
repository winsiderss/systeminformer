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

call :DetectArm64Support
if errorlevel 1 exit /b %errorlevel%

if /i "%PROCESSOR_ARCHITECTURE%"=="ARM64" set "VCVARS_ARCH=arm64"

call :SetupVcVars "%VCVARS_ARCH%"
if errorlevel 1 exit /b %errorlevel%

call :CleanupPath "tools\thirdparty\bin"
if errorlevel 1 exit /b %errorlevel%
call :CleanupPath "tools\thirdparty\obj"
if errorlevel 1 exit /b %errorlevel%

call :RunThirdPartyBuild "Debug" "x86" "Debug32"
if errorlevel 1 exit /b %errorlevel%
call :RunThirdPartyBuild "Release" "x86" "Release32"
if errorlevel 1 exit /b %errorlevel%
call :RunThirdPartyBuild "Debug" "x64" "Debug64"
if errorlevel 1 exit /b %errorlevel%
call :RunThirdPartyBuild "Release" "x64" "Release64"
if errorlevel 1 exit /b %errorlevel%

if /i "%VS_ARM64_SUPPORT%"=="true" (
    call :RunThirdPartyBuild "Debug" "ARM64" "DebugARM64"
    if errorlevel 1 exit /b !errorlevel!
    call :RunThirdPartyBuild "Release" "ARM64" "ReleaseARM64"
    if errorlevel 1 exit /b !errorlevel!
)

exit /b 0

REM -----------------------------------------------------------------------------
REM Function: RunThirdPartyBuild
REM Description: Builds one third-party solution configuration/platform pair.
REM Parameters:
REM   %~1 - Build configuration.
REM   %~2 - Build platform.
REM   %~3 - Friendly label shown in output.
REM -----------------------------------------------------------------------------
:RunThirdPartyBuild
echo:
echo Building thirdparty.sln [%~3]
msbuild /m tools\thirdparty\thirdparty.sln -property:Configuration=%~1 -property:Platform=%~2 -terminalLogger:%TLG%
exit /b %errorlevel%

REM -----------------------------------------------------------------------------
REM Function: FindVisualStudio
REM Description: Locates a Visual Studio or SDK installation with MSBuild.
REM -----------------------------------------------------------------------------
:FindVisualStudio
for /f "usebackq tokens=*" %%a in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
   set "VSINSTALLPATH=%%a"
)
if not defined VSINSTALLPATH if defined WindowsSdkDir set "VSINSTALLPATH=%WindowsSdkDir%"
if not defined VSINSTALLPATH if defined EWDK_ROOT set "VSINSTALLPATH=%EWDK_ROOT%"
if defined VSINSTALLPATH exit /b 0
echo No Visual Studio installation detected.
exit /b 1

REM -----------------------------------------------------------------------------
REM Function: DetectArm64Support
REM Description: Checks whether the detected Visual Studio install has ARM64 VC tools.
REM -----------------------------------------------------------------------------
:DetectArm64Support
for /f "usebackq tokens=*" %%a in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -products * -requires "Microsoft.VisualStudio.Component.VC.Tools.ARM64" -property installationPath`) do (
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
