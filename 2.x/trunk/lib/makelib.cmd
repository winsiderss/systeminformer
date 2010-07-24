@echo off

lib /machine:x86 /def:lib32/ntdll.def /out:lib32/ntdll.lib
"../tools/fixlib/bin/Release/fixlib.exe" lib32/ntdll.lib
lib /machine:x64 /def:lib64/ntdll.def /out:lib64/ntdll.lib
