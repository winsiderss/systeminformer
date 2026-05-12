@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

for /f "usebackq tokens=*" %%a in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
   set "VSINSTALLPATH=%%a"
)

if not defined VSINSTALLPATH if defined VSINSTALLDIR (
   set "VSINSTALLPATH=%VSINSTALLDIR%"
)

if not defined VSINSTALLPATH if defined VCINSTALLDIR (
   for %%I in ("%VCINSTALLDIR%\..\..") do set "VSINSTALLPATH=%%~fI"
)

if not defined VSINSTALLPATH if defined WindowsSdkDir (
   set "VSINSTALLPATH=%WindowsSdkDir%"
)

if not defined VSINSTALLPATH (
   echo No Visual Studio installation detected.
   goto end
)

if exist "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" (
   call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" amd64_arm64
)

MakeAppx.exe pack /o /f msix\PackageManifest64.msix.map /p msix\systeminformer-build-package.msix

:end
pause
