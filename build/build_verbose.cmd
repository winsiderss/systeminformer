@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

REM Check if terminal logging is supported
set "TLG=auto"
if "%GITHUB_ACTIONS%"=="true" (
    set "TLG=off"
) else if "%TF_BUILD%"=="true" (
    set "TLG=off"
)

REM Check if diagnostic is supported
set "TLV=normal"
if not "%~1"=="" (
    set "TLV=diagnostic"
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

msbuild /m SystemInformer.sln -t:rebuild -p:Configuration=Debug -p:Platform=Win32 -verbosity:%TLV% -terminalLogger:%TLG%
if %ERRORLEVEL% neq 0 goto end
echo:

msbuild /m SystemInformer.sln -t:rebuild -p:Configuration=Debug -p:Platform=x64 -verbosity:%TLV% -terminalLogger:%TLG%
if %ERRORLEVEL% neq 0 goto end
echo:

msbuild /m SystemInformer.sln -t:rebuild -p:Configuration=Debug -p:Platform=ARM64 -verbosity:%TLV% -terminalLogger:%TLG%
if %ERRORLEVEL% neq 0 goto end
echo:

msbuild /m Plugins\Plugins.sln -t:rebuild -p:Configuration=Debug -p:Platform=Win32 -verbosity:%TLV% -terminalLogger:%TLG%
if %ERRORLEVEL% neq 0 goto end
echo:

msbuild /m Plugins\Plugins.sln -t:rebuild -p:Configuration=Debug -p:Platform=x64 -verbosity:%TLV% -terminalLogger:%TLG%
if %ERRORLEVEL% neq 0 goto end
echo:

msbuild /m Plugins\Plugins.sln -t:rebuild -p:Configuration=Debug -p:Platform=ARM64 -verbosity:%TLV% -terminalLogger:%TLG%
if %ERRORLEVEL% neq 0 goto end
echo:

:end
pause
