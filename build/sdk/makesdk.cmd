@echo off

rmdir /S /Q ..\..\sdk
mkdir ..\..\sdk\include
mkdir ..\..\sdk\lib\amd64
mkdir ..\..\sdk\lib\i386

rem Header files

for %%a in (
    circbuf.h
    circbuf_h.h
    dltmgr.h
    fastlock.h
    graph.h
    hexedit.h
    kph.h
    ntbasic.h
    ntcm.h
    ntdbg.h
    ntexapi.h
    ntgdi.h
    ntimport.h
    ntioapi.h
    ntkeapi.h
    ntldr.h
    ntlpcapi.h
    ntlsa.h
    ntmisc.h
    ntmmapi.h
    ntobapi.h
    ntpebteb.h
    ntpnpapi.h
    ntpoapi.h
    ntpsapi.h
    ntregapi.h
    ntrtl.h
    ntsam.h
    ntseapi.h
    nttmapi.h
    nttp.h
    ntwin.h
    ntxcapi.h
    ph.h
    phbase.h
    phgui.h
    phnet.h
    phnt.h
    phsup.h
    queuedlock.h
    ref.h
    rev.h
    symprv.h
    templ.h
    winsta.h
    ) do copy ..\..\phlib\include\%%a ..\..\sdk\include\

for %%a in (
    hidnproc.h
    phplug.h
    providers.h
    ) do copy ..\..\ProcessHacker\include\%%a ..\..\sdk\include\

for %%a in (
    phapppub.h
    phdk.h
    ) do copy ..\..\ProcessHacker\sdk\%%a ..\..\sdk\include\

rem Libraries

copy ..\..\lib\lib32\ntdll.lib ..\..\sdk\lib\i386\
copy ..\..\lib\lib64\ntdll.lib ..\..\sdk\lib\amd64\
copy ..\..\lib\lib32\samlib.lib ..\..\sdk\lib\i386\
copy ..\..\lib\lib64\samlib.lib ..\..\sdk\lib\amd64\

copy ..\..\bin\Release32\ProcessHacker.lib ..\..\sdk\lib\i386\
copy ..\..\bin\Release64\ProcessHacker.lib ..\..\sdk\lib\amd64\
