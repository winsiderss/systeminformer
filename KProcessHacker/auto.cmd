@echo off

build -cZ
if not %errorlevel%==0 goto end
copy i386\kprocesshacker.sys ..\ProcessHacker\bin\Release\
copy i386\kprocesshacker.pdb ..\ProcessHacker\bin\Release\
:end