@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0\.."

REM -----------------------------------------------------------------------------
REM Script: build_tools.cmd
REM Description: Publishes CustomBuildTool and records the current tools identifier.
REM -----------------------------------------------------------------------------

REM Initialize script state, tool discovery values, and mode flags.
set "ExitCode=0"
set "IsCI=false"
set "SuppressPause=false"
set "TIB=false"
set "DOTNET_CLI_TELEMETRY_OPTOUT=1"
set "VSINSTALLPATH="
set "PublishProfile="
set "VCVARS_ARCH=amd64"
set "CustomBuildTool=tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe"

if /i "%~1"=="INIT" set "SuppressPause=true"

REM Run the main script flow and capture the final exit code.
call :DetectCi
call :Main
if errorlevel 1 set "ExitCode=%errorlevel%"

:end
REM Pause only for interactive, non-CI, non-nested invocations before returning.
if /i "%SuppressPause%"=="false" if /i "%IsCI%"=="false" call :PauseIfInteractive
endlocal & exit /b %ExitCode%

REM -----------------------------------------------------------------------------
REM Function: Main
REM Description: Cleans old outputs, publishes CustomBuildTool, and writes the tools id.
REM -----------------------------------------------------------------------------
:Main
call :FindVisualStudio
if errorlevel 1 exit /b %errorlevel%

call :CleanupPath "tools\CustomBuildTool\bin"
if errorlevel 1 exit /b %errorlevel%
call :CleanupPath "tools\CustomBuildTool\obj"
if errorlevel 1 exit /b %errorlevel%
call :CleanupPath "tools\CustomBuildTool\.vs"
if errorlevel 1 exit /b %errorlevel%

if /i "%PROCESSOR_ARCHITECTURE%"=="ARM64" (
    set "VCVARS_ARCH=arm64"
    set "PublishProfile=Properties\PublishProfiles\arm64.pubxml"
)
if not defined PublishProfile set "PublishProfile=Properties\PublishProfiles\amd64.pubxml"

call :SetupVcVars "%VCVARS_ARCH%"
if errorlevel 1 exit /b %errorlevel%

call :RunDotnetPublish "%PublishProfile%"
if errorlevel 1 exit /b %errorlevel%

call :CleanupPath "tools\CustomBuildTool\bin\ARM64"
if errorlevel 1 exit /b %errorlevel%
call :CleanupPath "tools\CustomBuildTool\bin\x64"
if errorlevel 1 exit /b %errorlevel%
call :CleanupPath "tools\CustomBuildTool\bin\Release\net9.0-windows-arm64"
if errorlevel 1 exit /b %errorlevel%
call :CleanupPath "tools\CustomBuildTool\bin\Release\net9.0-windows-x64"
if errorlevel 1 exit /b %errorlevel%
call :CleanupPath "tools\CustomBuildTool\obj"
if errorlevel 1 exit /b %errorlevel%
call :CleanupPath "tools\CustomBuildTool\.vs"
if errorlevel 1 exit /b %errorlevel%

if not exist "%CustomBuildTool%" (
    echo:
    echo CustomBuildTool build failed.
    echo:
    exit /b 1
)

echo:
call :RunCustomBuildTool "-write-tools-id"
if errorlevel 1 exit /b %errorlevel%
echo:

exit /b 0

REM -----------------------------------------------------------------------------
REM Function: RunDotnetPublish
REM Description: Publishes the CustomBuildTool project for the active architecture.
REM Parameters:
REM   %~1 - Publish profile path.
REM -----------------------------------------------------------------------------
:RunDotnetPublish
dotnet publish tools\CustomBuildTool\CustomBuildTool.sln -c Release /p:PublishProfile=%~1 /p:ContinuousIntegrationBuild=%TIB%
exit /b %errorlevel%

REM -----------------------------------------------------------------------------
REM Function: RunCustomBuildTool
REM Description: Executes CustomBuildTool with the supplied arguments.
REM Parameters:
REM   %* - Arguments forwarded to CustomBuildTool.
REM -----------------------------------------------------------------------------
:RunCustomBuildTool
start /B /W "" "%CustomBuildTool%" %*
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
REM Description: Detects whether the script is running under CI and enables CI publish mode.
REM -----------------------------------------------------------------------------
:DetectCi
if /i "%GITHUB_ACTIONS%"=="true" set "IsCI=true"
if /i "%TF_BUILD%"=="true" set "IsCI=true"
if /i "%IsCI%"=="true" set "TIB=true"
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
