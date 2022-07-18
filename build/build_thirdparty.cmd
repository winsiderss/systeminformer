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

msbuild tools\thirdparty\thirdparty.sln -property:Configuration=Debug -property:Platform=x86 -verbosity:normal
if %ERRORLEVEL% neq 0 goto end

msbuild tools\thirdparty\thirdparty.sln -property:Configuration=Release -property:Platform=x86 -verbosity:normal
if %ERRORLEVEL% neq 0 goto end

msbuild tools\thirdparty\thirdparty.sln -property:Configuration=Debug -property:Platform=x64 -verbosity:normal
if %ERRORLEVEL% neq 0 goto end

msbuild tools\thirdparty\thirdparty.sln -property:Configuration=Release -property:Platform=x64 -verbosity:normal
if %ERRORLEVEL% neq 0 goto end

msbuild tools\thirdparty\thirdparty.sln -property:Configuration=Debug -property:Platform=ARM64 -verbosity:normal
if %ERRORLEVEL% neq 0 goto end

msbuild tools\thirdparty\thirdparty.sln -property:Configuration=Release -property:Platform=ARM64 -verbosity:normal
if %ERRORLEVEL% neq 0 goto end

:end
pause
