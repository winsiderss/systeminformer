@echo off

if exist ..\..\sdk rmdir /S /Q ..\..\sdk
if exist ..\..\doc\doxygen mkdir ..\..\sdk\doc\doxygen
mkdir ..\..\sdk\dbg\amd64
mkdir ..\..\sdk\dbg\i386
mkdir ..\..\sdk\include
mkdir ..\..\sdk\lib\amd64
mkdir ..\..\sdk\lib\i386
mkdir ..\..\sdk\samples

rem Documentation

if exist ..\..\doc\doxygen echo d | xcopy /Q ..\..\doc\doxygen\html ..\..\sdk\doc\doxygen\html
copy ..\..\ProcessHacker\sdk\readme.txt ..\..\sdk\

rem Header files

for %%a in (
    ntdbg.h
    ntexapi.h
    ntgdi.h
    ntioapi.h
    ntkeapi.h
    ntldr.h
    ntlpcapi.h
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
    ntwow64.h
    ntxcapi.h
    ntzwapi.h
    phnt.h
    phnt_ntdef.h
    phnt_windows.h
    subprocesstag.h
    winsta.h
    ) do copy ..\..\phnt\include\%%a ..\..\sdk\include\%%a

for %%a in (
    circbuf.h
    circbuf_h.h
    cpysave.h
    dltmgr.h
    dspick.h
    emenu.h
    fastlock.h
    filestream.h
    graph.h
    guisup.h
    hexedit.h
    hndlinfo.h
    kphapi.h
    kphuser.h
    lsasup.h
    mapimg.h
    ph.h
    phbase.h
    phbasesup.h
    phconfig.h
    phdata.h
    phnative.h
    phnativeinl.h
    phnet.h
    phsup.h
    phutil.h
    provider.h
    queuedlock.h
    ref.h
    secedit.h
    svcsup.h
    symprv.h
    templ.h
    treenew.h
    verify.h
    workqueue.h
    ) do copy ..\..\phlib\include\%%a ..\..\sdk\include\%%a

call phapppub_gen.cmd

for %%a in (
    phapppub.h
    phdk.h
    ) do copy ..\..\ProcessHacker\sdk\%%a ..\..\sdk\include\%%a

copy ..\..\ProcessHacker\mxml\mxml.h ..\..\sdk\include\mxml.h

copy ..\..\ProcessHacker\resource.h ..\..\sdk\include\phappresource.h
cscript replace.vbs ..\..\sdk\include\phappresource.h "#define ID" "#define PHAPP_ID"
cscript replace.vbs ..\..\sdk\include\phappresource.h "#ifdef APSTUDIO_INVOKED" "#if 0"

rem Symbols

copy ..\..\bin\Release32\ProcessHacker.pdb ..\..\sdk\dbg\i386\
copy ..\..\bin\Release64\ProcessHacker.pdb ..\..\sdk\dbg\amd64\
copy ..\..\KProcessHacker\bin\i386\kprocesshacker.pdb ..\..\sdk\dbg\i386\
copy ..\..\KProcessHacker\bin\amd64\kprocesshacker.pdb ..\..\sdk\dbg\amd64\

rem Libraries

copy ..\..\bin\Release32\ProcessHacker.lib ..\..\sdk\lib\i386\
copy ..\..\bin\Release64\ProcessHacker.lib ..\..\sdk\lib\amd64\ > NUL

rem Samples

rem SamplePlugin
mkdir ..\..\sdk\samples\SamplePlugin
for %%a in (
    bin\Release32\SamplePlugin.dll
    main.c
    SamplePlugin.sln
    SamplePlugin.vcxproj
    SamplePlugin.vcxproj.filters
    ) do echo f | xcopy /Q ..\..\plugins\SamplePlugin\%%a ..\..\sdk\samples\SamplePlugin\%%a
