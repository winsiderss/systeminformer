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
set "TLG=auto"
set "DOTNET_CLI_TELEMETRY_OPTOUT=1"
set "VSINSTALLPATH="
set "VS_ARM64_SUPPORT=false"
set "CBT_PLATFORMS=x86;x64"
set "VCVARS_ARCH=amd64"
set "CustomBuildTool=tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe"

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
REM Description: Cleans old outputs, publishes CustomBuildTool, and writes the tools id.
REM -----------------------------------------------------------------------------
:Main
call :FindVisualStudio
if errorlevel 1 exit /b %errorlevel%

call :DetectArm64Support
if errorlevel 1 exit /b %errorlevel%

call :CleanupPath "tools\CustomBuildTool\bin"
if errorlevel 1 exit /b %errorlevel%
call :CleanupPath "tools\CustomBuildTool\obj"
if errorlevel 1 exit /b %errorlevel%
call :CleanupPath "tools\CustomBuildTool\.vs"
if errorlevel 1 exit /b %errorlevel%

REM Target ARM64 when supported; the host toolset is picked from the machine arch.
if /i "%VS_ARM64_SUPPORT%"=="true" (call :ResolveVcVarsArch arm64) else (call :ResolveVcVarsArch x64)
if /i "%VS_ARM64_SUPPORT%"=="true" set "CBT_PLATFORMS=x86;x64;ARM64"
call :SetupVcVars "%VCVARS_ARCH%"
if errorlevel 1 exit /b %errorlevel%

call :RunMsBuildPublish "%CBT_PLATFORMS%" "%CBT_PLATFORMS%"
if errorlevel 1 exit /b %errorlevel%

call :CleanupPath "tools\CustomBuildTool\bin\ARM64"
if errorlevel 1 exit /b %errorlevel%
call :CleanupPath "tools\CustomBuildTool\bin\x64"
if errorlevel 1 exit /b %errorlevel%
call :CleanupPath "tools\CustomBuildTool\bin\x86"
if errorlevel 1 exit /b %errorlevel%
call :CleanupPath "tools\CustomBuildTool\bin\Release\net9.0-windows-arm64"
if errorlevel 1 exit /b %errorlevel%
call :CleanupPath "tools\CustomBuildTool\bin\Release\net9.0-windows-x64"
if errorlevel 1 exit /b %errorlevel%
call :CleanupPath "tools\CustomBuildTool\bin\Release\net9.0-windows-x86"
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
REM Function: RunMsBuildPublish
REM Description: Publishes the CustomBuildTool solution for the active architecture.
REM Parameters:
REM   %~1 - Semicolon-separated platform list (e.g. "x86;x64;ARM64").
REM   %~2 - Friendly label shown in output.
REM -----------------------------------------------------------------------------
:RunMsBuildPublish
set "TargetPlatforms=%~1"
echo:
echo Building CustomBuildTool.sln [%~2]
dotnet msbuild -mt -p:UseClStructuredOutput=false -restore /m /graph tools\CustomBuildTool\CustomBuildTool.csproj -t:All -p:BuildAllTarget=Publish -p:Configuration=Release -p:Platform=x86 -p:BuildTargetConfigurations=Release -p:ContinuousIntegrationBuild=%TIB% -p:RestoreUseStaticGraphEvaluation=true -p:CopyRetryCount=10 -p:CopyRetryDelayMilliseconds=500 -terminalLogger:%TLG%
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
if /i "%GITLAB_CI%"=="true" set "IsCI=true"
if /i "%IsCI%"=="true" set "TIB=true"
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

