@echo off

if "%1" == "" goto :notset
if "%2" == "" goto :notset

if exist %2\processhacker-*-*.* del %2\processhacker-*-*.*

rem Source distribution

if exist "%SVNBIN%\svn.exe". (
    if exist %2\ProcessHacker2 rmdir /S /Q %2\ProcessHacker2
    "%SVNBIN%\svn.exe" export %1 %2\ProcessHacker2
    if exist "%SEVENZIPBIN%\7z.exe" "%SEVENZIPBIN%\7z.exe" a -mx9 %2\processhacker-2.%MINORVERSION%-src.zip %2\ProcessHacker2\*
    )

rem SDK distribution

if exist "%SEVENZIPBIN%\7z.exe" "%SEVENZIPBIN%\7z.exe" a -mx9 %2\processhacker-2.%MINORVERSION%-sdk.zip %1\sdk\*

rem Binary distribution

if exist %2\bin rmdir /S /Q %2\bin
mkdir %2\bin

for %%a in (
    CHANGELOG.txt
    COPYRIGHT.txt
    LICENSE.txt
    README.txt
    ) do copy %1\%%a %2\bin\%%a

mkdir %2\bin\x86
copy %1\bin\Release32\ProcessHacker.exe %2\bin\x86\
copy %1\KProcessHacker\bin-signed\i386\kprocesshacker.sys %2\bin\x86\
copy %1\bin\Release32\peview.exe %2\bin\x86\

mkdir %2\bin\x64
copy %1\bin\Release64\ProcessHacker.exe %2\bin\x64\
copy %1\KProcessHacker\bin-signed\amd64\kprocesshacker.sys %2\bin\x64\
copy %1\bin\Release64\peview.exe %2\bin\x64\

mkdir %2\bin\plugins\x86
for %%a in (
    DotNetTools
    ExtendedNotifications
    ExtendedServices
    ExtendedTools
    NetworkTools
    OnlineChecks
    SbieSupport
    ToolStatus
    Updater
    UserNotes
    WindowExplorer
    ) do copy %1\bin\Release32\plugins\%%a.dll %2\bin\plugins\x86\%%a.dll

mkdir %2\bin\plugins\x64
for %%a in (
    DotNetTools
    ExtendedNotifications
    ExtendedServices
    ExtendedTools
    NetworkTools
    OnlineChecks
    SbieSupport
    ToolStatus
    Updater
    UserNotes
    WindowExplorer
    ) do copy %1\bin\Release64\plugins\%%a.dll %2\bin\plugins\x64\%%a.dll

if exist "%SEVENZIPBIN%\7z.exe" "%SEVENZIPBIN%\7z.exe" a -mx9 %2\processhacker-2.%MINORVERSION%-bin.zip %2\bin\*
if exist %1\build\Installer\processhacker-*-setup.exe copy %1\build\Installer\processhacker-*-setup.exe %2\

goto :end

:notset
echo Parameters not set.
pause

:end
