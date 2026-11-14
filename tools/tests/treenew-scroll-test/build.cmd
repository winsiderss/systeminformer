@echo off
setlocal
rem Run from an x64 Native Tools Command Prompt after building Debug x64.
pushd "%~dp0"
cl /nologo /utf-8 /MTd /Zi /DUNICODE /D_UNICODE /DWIN64 /D_DEBUG /DDEBUG /D_PHLIB_ /I..\..\..\phlib\include /I..\..\..\phnt\include /I..\..\..\kphlib\include /I..\..\..\kphlib\bin\Debug64 main.c /link /LIBPATH:..\..\..\phlib\bin\Debug64 phlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib setupapi.lib cfgmgr32.lib aclui.lib comctl32.lib dnsapi.lib gdiplus.lib ntdll.lib samlib.lib userenv.lib wbemuuid.lib windowscodecs.lib winhttp.lib winsta.lib delayimp.lib /DELAYLOAD:setupapi.dll /DELAYLOAD:cfgmgr32.dll /DELAYLOAD:aclui.dll /DELAYLOAD:comdlg32.dll /DELAYLOAD:gdiplus.dll /DELAYLOAD:oleaut32.dll /DELAYLOAD:winhttp.dll /DELAYLOAD:winsta.dll /DELAYLOAD:samlib.dll /out:treenew-scroll-test.exe
if not "%errorlevel%"=="0" goto failed
treenew-scroll-test.exe
if not "%errorlevel%"=="0" goto failed
popd
exit /b 0
:failed
popd
exit /b 1
