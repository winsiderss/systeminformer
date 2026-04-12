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
set "TLG=auto"
set "TLV=normal"
set "VSINSTALLPATH="
set "VS_ARM64_SUPPORT=false"
set "VCVARS_ARCH=amd64"

REM Run the main script flow and capture the final exit code.
call :DetectCi
call :ConfigureTerminalLogger
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

call :DetectArm64Support
if errorlevel 1 exit /b %errorlevel%

if /i "%VS_ARM64_SUPPORT%"=="true" (
    set "VCVARS_ARCH=amd64_arm64"
)

call :SetupVcVars "%VCVARS_ARCH%"
if errorlevel 1 exit /b %errorlevel%

call :RunMsBuild "SystemInformer.sln" "Debug" "Win32" "SystemInformer.sln [Debug32]"
if errorlevel 1 exit /b %errorlevel%
call :RunMsBuild "SystemInformer.sln" "Debug" "x64" "SystemInformer.sln [Debug64]"
if errorlevel 1 exit /b %errorlevel%
call :RunMsBuild "Plugins\Plugins.sln" "Debug" "Win32" "Plugins.sln [Debug32]"
if errorlevel 1 exit /b %errorlevel%
call :RunMsBuild "Plugins\Plugins.sln" "Debug" "x64" "Plugins.sln [Debug64]"
if errorlevel 1 exit /b %errorlevel%

if /i "%VS_ARM64_SUPPORT%"=="true" (
    call :RunMsBuild "SystemInformer.sln" "Debug" "ARM64" "SystemInformer.sln [DebugARM64]"
    if errorlevel 1 exit /b !errorlevel!
    call :RunMsBuild "Plugins\Plugins.sln" "Debug" "ARM64" "Plugins.sln [DebugARM64]"
    if errorlevel 1 exit /b !errorlevel!
)

exit /b 0

REM -----------------------------------------------------------------------------
REM Function: RunMsBuild
REM Description: Rebuilds one solution/configuration/platform combination.
REM Parameters:
REM   %~1 - Solution path.
REM   %~2 - Build configuration.
REM   %~3 - Build platform.
REM   %~4 - Friendly label shown in output.
REM -----------------------------------------------------------------------------
:RunMsBuild
echo:
echo Building %~4
msbuild /m %~1 -t:rebuild -p:Configuration=%~2 -p:Platform=%~3 -verbosity:%TLV% -terminalLogger:%TLG%
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
