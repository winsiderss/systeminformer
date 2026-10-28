@echo off
setlocal

REM Locate MSBuild for callers outside a Visual Studio developer shell.
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
set "MSBUILD_PATH="
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%I in (`call "%VSWHERE%" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -find MSBuild\Current\Bin\MSBuild.exe`) do set "MSBUILD_PATH=%%I"
)
if defined MSBUILD_PATH goto Run
for /f "tokens=*" %%I in ('where msbuild.exe 2^>nul') do if not defined MSBUILD_PATH set "MSBUILD_PATH=%%I"
if defined MSBUILD_PATH goto Run
echo MSBuild was not found. Install the Visual Studio C++ build tools. 1>&2
exit /b 1

:Run
"%MSBUILD_PATH%" %*
exit /b %errorlevel%
