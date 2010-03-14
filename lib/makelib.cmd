@echo off

lib /machine:x86 /def:ntdll32/ntdll.def /out:ntdll32/ntdll.lib
"../tools/fixlib/bin/Release/fixlib.exe" ntdll32/ntdll.lib
lib /machine:x64 /def:ntdll64/ntdll.def /out:ntdll64/ntdll.lib
