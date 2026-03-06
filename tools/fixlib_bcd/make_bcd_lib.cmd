@echo off
@setlocal enableextensions

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

if exist "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" (
   call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" amd64_arm64
)

lib /machine:x86 /def:lib32/bcd.def /out:lib32/bcd.lib
"../fixlib/bin/Release/fixlib.exe" lib32/bcd.lib
lib /machine:x64 /def:lib64/bcd.def /out:lib64/bcd.lib

pause
