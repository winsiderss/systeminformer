@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

for /f "usebackq tokens=*" %%a in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
   set "VSINSTALLPATH=%%a"
)

if not defined VSINSTALLPATH (
   goto end
)

if exist "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" (
   call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" amd64_arm64
) else (
   goto end
)

msbuild KProcessHacker\KProcessHacker.sln -property:Configuration=Debug -property:Platform=ARM64 -verbosity:minimal
if %ERRORLEVEL% neq 0 goto end

msbuild ProcessHacker.sln -property:Configuration=Debug -property:Platform=ARM64 -verbosity:minimal
if %ERRORLEVEL% neq 0 goto end

mkdir sdk\lib\arm64
copy bin\DebugARM64\ProcessHacker.lib sdk\lib\arm64

msbuild plugins\Plugins.sln -property:Configuration=Debug -property:Platform=ARM64 -verbosity:minimal

:end
pause
