@echo off
@setlocal enableextensions

call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64_arm64

lib /machine:x86 /def:lib32/bcd.def /out:lib32/bcd.lib
"../fixlib/bin/Release/fixlib.exe" lib32/bcd.lib
lib /machine:x64 /def:lib64/bcd.def /out:lib64/bcd.lib

pause
