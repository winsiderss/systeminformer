@echo off

rem The first parameter specifies the ProcessHacker2 
rem base directory.
rem The second parameter specifies the output directory.

rem Build the main projects

devenv %1\ProcessHacker.sln /build "Release|Win32"
devenv %1\ProcessHacker.sln /build "Release|x64"
call %1\build\internal\wait.cmd 2

pushd %1\build\sdk\
call makesdk.cmd
popd

devenv %1\plugins\Plugins.sln /build "Release|Win32"
devenv %1\plugins\Plugins.sln /build "Release|x64"
call %1\build\internal\wait.cmd 2

rem Build the installer (HARDCODED PATH)

if exist "C:\Program Files\Inno Setup 5\iscc.exe". (
pushd %1\build\Installer\
del *.exe
"C:\Program Files\Inno Setup 5\iscc.exe" Process_Hacker2_installer.iss
popd
)

call %1\build\internal\dorelease.cmd %1 %2
