@echo off

rem Doxygen

pushd ..\..\
doxygen
popd

call makesdk.cmd
