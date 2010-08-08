@echo off

rem Doxygen

pushd ..\..\
if exist doc\doxygen rmdir /S /Q doc\doxygen
doxygen
popd

call makesdk.cmd
