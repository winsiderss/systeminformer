@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

for /f "usebackq tokens=*" %%A in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -version "[16.0,17.0)" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
   set VSINSTALLPATH2019=%%A
)
 
if not defined VSINSTALLPATH2019 (
   goto end
)

if exist "%VSINSTALLPATH2019%\VC\Auxiliary\Build\vcvarsall.bat" (
   call "%VSINSTALLPATH2019%\VC\Auxiliary\Build\vcvarsall.bat" amd64_arm64
) else (
   goto end
)

msbuild /m KSystemInformer\KSystemInformer.sln -property:Configuration=Debug -property:Platform=Win32 /p:TargetVersion="Windows7" -verbosity:minimal
if %ERRORLEVEL% neq 0 goto end

msbuild /m KSystemInformer\KSystemInformer.sln -property:Configuration=Debug -property:Platform=x64 /p:TargetVersion="Windows7" -verbosity:minimal
if %ERRORLEVEL% neq 0 goto end

:end
pause
