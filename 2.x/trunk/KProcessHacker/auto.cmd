@echo off

build -cZ
if not %errorlevel%==0 goto end
copy bin\i386\kprocesshacker.sys ..\bin\Release32\
copy bin\i386\kprocesshacker.pdb ..\bin\Release32\
copy bin\amd64\kprocesshacker.sys ..\bin\Release64\
copy bin\amd64\kprocesshacker.pdb ..\bin\Release64\
:end