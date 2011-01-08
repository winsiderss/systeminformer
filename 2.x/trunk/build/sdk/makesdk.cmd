@echo off

if exist ..\..\sdk rmdir /S /Q ..\..\sdk
if exist ..\..\doc\doxygen mkdir ..\..\sdk\doc\doxygen
mkdir ..\..\sdk\include
mkdir ..\..\sdk\lib\amd64
mkdir ..\..\sdk\lib\i386
mkdir ..\..\sdk\samples

rem Documentation

if exist ..\..\doc\doxygen echo d | xcopy /Q ..\..\doc\doxygen\html ..\..\sdk\doc\doxygen\html
copy ..\..\ProcessHacker\sdk\readme.txt ..\..\sdk\

rem Header files

for %%a in (
    circbuf.h
    circbuf_h.h
    dltmgr.h
    dspick.h
    emenu.h
    fastlock.h
    graph.h
    hexedit.h
    kphapi.h
    kphuser.h
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
    ntnls.h
    ntobapi.h
    ntpebteb.h
    ntpfapi.h
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
    ntwow64.h
    ntxcapi.h
    ntzwapi.h
    ph.h
    phbase.h
    phgui.h
    phnatinl.h
    phnet.h
    phnt.h
    phsup.h
    phsync.h
    queuedlock.h
    ref.h
    rev.h
    symprv.h
    templ.h
    treelist.h
    winsta.h
    ) do copy ..\..\phlib\include\%%a ..\..\sdk\include\%%a

for %%a in (
    hidnproc.h
    phplug.h
    providers.h
    ) do copy ..\..\ProcessHacker\include\%%a ..\..\sdk\include\%%a

for %%a in (
    phapppub.h
    phdk.h
    ) do copy ..\..\ProcessHacker\sdk\%%a ..\..\sdk\include\%%a

copy ..\..\ProcessHacker\mxml\mxml.h ..\..\sdk\include\mxml.h

copy ..\..\ProcessHacker\resource.h ..\..\sdk\include\phappresource.h
cscript replace.vbs ..\..\sdk\include\phappresource.h "#define ID" "#define PHAPP_ID"
cscript replace.vbs ..\..\sdk\include\phappresource.h "#ifdef APSTUDIO_INVOKED" "#if 0"

rem Libraries

copy ..\..\lib\lib32\ntdll.lib ..\..\sdk\lib\i386\
copy ..\..\lib\lib64\ntdll.lib ..\..\sdk\lib\amd64\
copy ..\..\lib\lib32\samlib.lib ..\..\sdk\lib\i386\
copy ..\..\lib\lib64\samlib.lib ..\..\sdk\lib\amd64\
copy ..\..\lib\lib32\winsta.lib ..\..\sdk\lib\i386\
copy ..\..\lib\lib64\winsta.lib ..\..\sdk\lib\amd64\

copy ..\..\bin\Release32\ProcessHacker.lib ..\..\sdk\lib\i386\
copy ..\..\bin\Release64\ProcessHacker.lib ..\..\sdk\lib\amd64\ > NUL

rem Samples

rem SamplePlugin
mkdir ..\..\sdk\samples\SamplePlugin
for %%a in (
    bin\Release32\SamplePlugin.dll
    main.c
    SamplePlugin.sln
    SamplePlugin.vcproj
    ) do echo f | xcopy /Q ..\..\plugins\SamplePlugin\%%a ..\..\sdk\samples\SamplePlugin\%%a
