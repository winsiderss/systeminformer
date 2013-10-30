@echo off
set PHBASE=..
set SIGN_TIMESTAMP=1
copy bin\i386\kprocesshacker.sys bin-signed\i386\kprocesshacker.sys
copy bin\amd64\kprocesshacker.sys bin-signed\amd64\kprocesshacker.sys
call ..\build\internal\sign.cmd bin-signed\i386\kprocesshacker.sys kmcs
call ..\build\internal\sign.cmd bin-signed\amd64\kprocesshacker.sys kmcs
