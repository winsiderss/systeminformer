@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0\.."

REM -----------------------------------------------------------------------------
REM Script: build_compile_commands.cmd
REM Description: Publishes the compile-commands logger and rebuilds solutions with it.
REM -----------------------------------------------------------------------------

REM Initialize script state, tool discovery values, and output locations.
set "ExitCode=0"
set "IsCI=false"
set "TLG=auto"
set "VSINSTALLPATH="
set "VCVARS_ARCH=amd64"
set "PublishProfile="
set "CompileCommandsLogger="
set "BuildPlatform=x64"

REM Run the main script flow and capture the final exit code.
call :DetectCi
call :ConfigureTerminalLogger
call :Main
if errorlevel 1 set "ExitCode=%errorlevel%"

:end
REM Pause only for interactive, non-CI invocations before returning.
if /i "%IsCI%"=="false" call :PauseIfInteractive
endlocal & exit /b %ExitCode%

REM -----------------------------------------------------------------------------
REM Function: Main
REM Description: Sets up the toolchain, publishes the logger, and rebuilds solutions.
REM -----------------------------------------------------------------------------
:Main
call :FindVisualStudio
if errorlevel 1 exit /b %errorlevel%

if /i "%PROCESSOR_ARCHITECTURE%"=="ARM64" (
    set "VCVARS_ARCH=arm64"
    set "PublishProfile=Properties\PublishProfiles\arm64.pubxml"
    set "CompileCommandsLogger=tools\CompileCommandsJson\bin\Release\arm64\CompileCommandsJson.dll"
    set "BuildPlatform=ARM64"
) else (
    set "PublishProfile=Properties\PublishProfiles\amd64.pubxml"
    set "CompileCommandsLogger=tools\CompileCommandsJson\bin\Release\amd64\CompileCommandsJson.dll"
)

call :SetupVcVars "%VCVARS_ARCH%"
if errorlevel 1 exit /b %errorlevel%

call :RunDotnetPublish "%PublishProfile%"
if errorlevel 1 exit /b %errorlevel%

if not exist "%CompileCommandsLogger%" (
    echo CompileCommandsJson.dll not found.
    exit /b 1
)

call :RunMsBuild "KSystemInformer\KSystemInformer.sln" "%BuildPlatform%"
if errorlevel 1 exit /b %errorlevel%
call :RunMsBuild "SystemInformer.sln" "%BuildPlatform%"
if errorlevel 1 exit /b %errorlevel%
call :RunMsBuild "Plugins\Plugins.sln" "%BuildPlatform%"
if errorlevel 1 exit /b %errorlevel%

exit /b 0

REM -----------------------------------------------------------------------------
REM Function: RunDotnetPublish
REM Description: Publishes the CompileCommandsJson project for the active architecture.
REM Parameters:
REM   %~1 - Publish profile path.
REM -----------------------------------------------------------------------------
:RunDotnetPublish
dotnet publish tools\CompileCommandsJson\CompileCommandsJson.sln -c Release /p:PublishProfile=%~1
exit /b %errorlevel%

REM -----------------------------------------------------------------------------
REM Function: RunMsBuild
REM Description: Rebuilds a solution while emitting compile_commands.json data.
REM Parameters:
REM   %~1 - Solution path.
REM   %~2 - Platform name.
REM -----------------------------------------------------------------------------
:RunMsBuild
echo:
msbuild /m %~1 -t:rebuild -p:Configuration=Debug;Platform=%~2 -logger:%CompileCommandsLogger% -terminalLogger:%TLG%
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
