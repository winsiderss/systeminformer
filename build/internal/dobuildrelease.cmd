@echo off

rem The first parameter specifies the ProcessHacker2 base directory.
rem The second parameter specifies the output directory.
rem Variables:
rem INNOBIN - Specify the path to Inno Setup.
rem     e.g. C:\Program Files\Inno Setup 5
rem SVNBIN - Specify the path to the SVN client.
rem SEVENZIPBIN - Specify the path to 7-Zip.
rem     e.g. C:\Program Files\7-Zip
rem SIGN - Specify 1 to sign executable files.
rem SIGN_TIMESTAMP - Specify 1 to timestamp executable files.
rem MAJORVERSION - Specify the major version of the Process Hacker release being built.
rem     e.g. 2
rem MINORVERSION - Specify the minor version of the Process Hacker release being built.
rem     e.g. 8

rem Build the main projects

set PHBASE=%1

devenv %1\ProcessHacker.sln /build "Release|Win32"
devenv %1\ProcessHacker.sln /build "Release|x64"
call %1\build\internal\wait.cmd 2

pushd %1\build\sdk\
call makesdk.cmd
popd

devenv %1\plugins\Plugins.sln /build "Release|Win32"
devenv %1\plugins\Plugins.sln /build "Release|x64"
call %1\build\internal\wait.cmd 2

call %1\build\internal\dorelease.cmd %1 %2
