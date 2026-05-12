@echo off
@setlocal enableextensions

for /f "usebackq tokens=*" %%A in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
    set VSINSTALLPATH=%%A
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
    echo [-] Visual Studio not found
    goto end
)

if exist "%VSINSTALLPATH%\Common7\Tools\VsDevCmd.bat" (
   call "%VSINSTALLPATH%\Common7\Tools\VsDevCmd.bat" -arch=arm64 -host_arch=amd64
)

echo;

rc.exe /v /c 65001 /i "%INCLUDE%" /i "%~dp0..\..\systeminformer" /D "_UNICODE" /D "UNICODE" /fo version.res version.rc

echo;
cvtres.exe /MACHINE:x64 /VERBOSE version.res

echo;

cl.exe version.obj /link /DLL /MACHINE:X64 /SUBSYSTEM:WINDOWS /NOENTRY /out:version.dll

echo;

link.exe /SUBSYSTEM:CONSOLE /MACHINE:x64 /ENTRY:__ImageBase /RELEASE /Brepro /VERBOSE /OUT:version_link.exe version.res

pause
