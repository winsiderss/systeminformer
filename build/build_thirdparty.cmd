@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

REM Check if an argument is provided and if it is "INIT"
set "IS_INIT=false"
if "%1"=="INIT" (
    set "IS_INIT=true"
)

REM Check if terminal logging is supported
set "TLG=auto"
if "%GITHUB_ACTIONS%"=="true" (
    set "TLG=off"
) else if "%TF_BUILD%"=="true" (
    set "TLG=off"
)

for /f "usebackq tokens=*" %%a in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
   set "VSINSTALLPATH=%%a"
)

if not defined VSINSTALLPATH if defined WindowsSdkDir (
   set "VSINSTALLPATH=%WindowsSdkDir%"
)

if not defined VSINSTALLPATH if defined EWDK_ROOT (
   set "VSINSTALLPATH=%EWDK_ROOT%"
)

if not defined VSINSTALLPATH (
   echo No Visual Studio installation detected.
   goto end
)

set "VS_ARM64_SUPPORT=false"
for /f "usebackq tokens=*" %%a in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -products * -requires "Microsoft.VisualStudio.Component.VC.Tools.ARM64" -property installationPath`) do (
   set "VS_ARM64_SUPPORT=true"
)

if "%PROCESSOR_ARCHITECTURE%"=="ARM64" (
   call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" arm64
) else (
   call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" amd64
)

if exist "tools\thirdparty\bin" (
   rmdir /S /Q "tools\thirdparty\bin"
)
if exist "tools\thirdparty\obj" (
   rmdir /S /Q "tools\thirdparty\obj"
)

echo:

echo Building thirdparty.sln [Debug32]
msbuild /m tools\thirdparty\thirdparty.sln -property:Configuration=Debug -property:Platform=x86 -terminalLogger:%TLG%
if %ERRORLEVEL% neq 0 goto end
echo:

echo Building thirdparty.sln [Release32]
msbuild /m tools\thirdparty\thirdparty.sln -property:Configuration=Release -property:Platform=x86 -terminalLogger:%TLG%
if %ERRORLEVEL% neq 0 goto end
echo:

echo Building thirdparty.sln [Debug64]
msbuild /m tools\thirdparty\thirdparty.sln -property:Configuration=Debug -property:Platform=x64 -terminalLogger:%TLG%
if %ERRORLEVEL% neq 0 goto end
echo:

echo Building thirdparty.sln [Release64]
msbuild /m tools\thirdparty\thirdparty.sln -property:Configuration=Release -property:Platform=x64 -terminalLogger:%TLG%
if %ERRORLEVEL% neq 0 goto end
echo:

if "%VS_ARM64_SUPPORT%"=="true" (
   echo Building thirdparty.sln [DebugARM64]
   msbuild /m tools\thirdparty\thirdparty.sln -property:Configuration=Debug -property:Platform=ARM64 -terminalLogger:%TLG%
   if %ERRORLEVEL% neq 0 goto end
   echo:

   echo Building thirdparty.sln [ReleaseARM64]
   msbuild /m tools\thirdparty\thirdparty.sln -property:Configuration=Release -property:Platform=ARM64 -terminalLogger:%TLG%
   if %ERRORLEVEL% neq 0 goto end
   echo:
)

:end

REM If IS_INIT is not provided, print a message and exit
if /i "%IS_INIT%"=="false" (
   REM Avoid pause in non-interactive runs (stdin redirected).
   set "STDIN_REDIRECTED=False"
   for /f %%i in ('powershell -NoProfile -Command "[Console]::IsInputRedirected"') do set "STDIN_REDIRECTED=%%i"
   REM Pause only for interactive console sessions.
   if /i not "%STDIN_REDIRECTED%"=="True" pause
) else (
   REM INIT mode is used by scripts, so return immediately.
   exit /b 0
)
