@echo off

rem The first parameter specifies the ProcessHacker2
rem base directory.
rem The second parameter specifies the output directory.
rem Variables:
rem INNOBIN - Specify the path to Inno Setup.
rem     e.g. C:\Program Files\Inno Setup 5
rem SVNBIN - Specify the path to the SVN client.
rem SEVENZIPBIN - Specify the path to 7-Zip.
rem     e.g. C:\Program Files\7-Zip
rem MINORVERSION - Specify the minor version of
rem     the Process Hacker release being built.
rem     e.g. 8

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

if exist "%INNOBIN%\iscc.exe". (
    pushd %1\build\Installer\
    del *.exe
    "%INNOBIN%\iscc.exe" Process_Hacker2_installer.iss
    popd
)

call %1\build\internal\dorelease.cmd %1 %2
