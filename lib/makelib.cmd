@echo off

lib /machine:x86 /def:lib32/%1.def /out:lib32/%1.lib
"../tools/fixlib/bin/Release/fixlib.exe" lib32/%1.lib
lib /machine:x64 /def:lib64/%1.def /out:lib64/%1.lib
