@echo off

build -cZ
if not %errorlevel%==0 goto end
copy i386\kprocesshacker.sys ..\bin\Release32\
copy i386\kprocesshacker.pdb ..\bin\Release32\
copy i386\kprocesshacker.sys ..\bin\Release64\
copy i386\kprocesshacker.pdb ..\bin\Release64\
:end