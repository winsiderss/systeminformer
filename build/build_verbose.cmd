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

msbuild SystemInformer.sln -property:Configuration=Debug -property:Platform=Win32 -verbosity:normal
if %ERRORLEVEL% neq 0 goto end

msbuild SystemInformer.sln -property:Configuration=Debug -property:Platform=x64 -verbosity:normal
if %ERRORLEVEL% neq 0 goto end

tools\CustomBuildTool\bin\Release\CustomBuildTool.exe -sdk

msbuild Plugins\Plugins.sln -property:Configuration=Debug -property:Platform=Win32 -verbosity:normal
if %ERRORLEVEL% neq 0 goto end

msbuild Plugins\Plugins.sln -property:Configuration=Debug -property:Platform=x64 -verbosity:normal
if %ERRORLEVEL% neq 0 goto end

:end
pause
