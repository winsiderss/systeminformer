@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

REM Check if terminal logging is supported
set "TLG=auto"
if "%GITHUB_ACTIONS%"=="true"  (
    set "TLG=off"
) else if "%BUILD_BUILDID%" NEQ "" (
    set "TLG=off"
)

for /f "usebackq tokens=*" %%a in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
   set "VSINSTALLPATH=%%a"
)

if not defined VSINSTALLPATH (
   echo No Visual Studio installation detected.
   goto end
)

if exist "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" (
   call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" amd64_arm64
) else (
   goto end
)

echo:

msbuild /m SystemInformer.sln -property:Configuration=Debug -property:Platform=Win32 -verbosity:normal -terminalLogger:"%TLG%"
if %ERRORLEVEL% neq 0 goto end
echo:

msbuild /m SystemInformer.sln -property:Configuration=Debug -property:Platform=x64 -verbosity:normal -terminalLogger:"%TLG%"
if %ERRORLEVEL% neq 0 goto end
echo:

msbuild /m SystemInformer.sln -property:Configuration=Debug -property:Platform=ARM64 -verbosity:normal -terminalLogger:"%TLG%"
if %ERRORLEVEL% neq 0 goto end
echo:

msbuild /m Plugins\Plugins.sln -property:Configuration=Debug -property:Platform=Win32 -verbosity:normal -terminalLogger:"%TLG%"
if %ERRORLEVEL% neq 0 goto end
echo:

msbuild /m Plugins\Plugins.sln -property:Configuration=Debug -property:Platform=x64 -verbosity:normal -terminalLogger:"%TLG%"
if %ERRORLEVEL% neq 0 goto end
echo:

msbuild /m Plugins\Plugins.sln -property:Configuration=Debug -property:Platform=ARM64 -verbosity:normal -terminalLogger:"%TLG%"
if %ERRORLEVEL% neq 0 goto end
echo:

:end
pause
