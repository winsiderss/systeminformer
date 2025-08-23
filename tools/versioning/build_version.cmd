@echo off
@setlocal enableextensions

for /f "usebackq tokens=*" %%A in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
    set VSINSTALLPATH=%%A
)

if not defined VSINSTALLPATH (
    echo [-] Visual Studio not found
    goto end
)

if exist "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" (
   call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" amd64_arm64
) else (
    echo [-] Failed to set up build environment
    goto end
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
