@echo off
@setlocal enableextensions

call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64_arm64

echo;

rc.exe /v /c 65001 /i "%INCLUDE%" /i "%~dp0..\..\systeminformer" /D "_UNICODE" /D "UNICODE" /fo version.res version.rc

echo;
cvtres.exe /MACHINE:x64 /VERBOSE version.res

echo;

cl.exe version.obj /link /DLL /MACHINE:X64 /SUBSYSTEM:WINDOWS /NOENTRY /out:version.dll

echo;

link.exe /SUBSYSTEM:CONSOLE /MACHINE:x64 /ENTRY:__ImageBase /RELEASE /Brepro /VERBOSE /OUT:version_link.exe version.res

pause
