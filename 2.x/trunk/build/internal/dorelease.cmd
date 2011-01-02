@echo off

rem The first parameter specifies the ProcessHacker2 
rem base directory.
rem The second parameter specifies the output directory.

if exist %2\bin rmdir /S /Q %2\bin
mkdir %2\bin

for %%a in (
    CHANGELOG.txt
    LICENSE.txt
    README.txt
    ) do copy %1\%%a %2\bin\%%a

copy %1\doc\Help.htm %2\bin\

mkdir %2\bin\x86
copy %1\bin\Release32\ProcessHacker.exe %2\bin\x86\
copy %1\bin\Release32\kprocesshacker.sys %2\bin\x86\
copy %1\tools\peview\bin\Release32\peview.exe %2\bin\x86\

mkdir %2\bin\x64
copy %1\bin\Release64\ProcessHacker.exe %2\bin\x64\
copy %1\tools\peview\bin\Release64\peview.exe %2\bin\x64\

mkdir %2\bin\plugins\x86
for %%a in (
    ExtendedNotifications
    ExtendedServices
    ExtendedTools
    NetworkTools
    OnlineChecks
    SbieSupport
    ToolStatus
    ) do copy %1\plugins\%%a\bin\Release32\%%a.dll %2\bin\plugins\x86\%%a.dll

mkdir %2\bin\plugins\x64
for %%a in (
    ExtendedNotifications
    ExtendedServices
    ExtendedTools
    NetworkTools
    OnlineChecks
    SbieSupport
    ToolStatus
    ) do copy %1\plugins\%%a\bin\Release64\%%a.dll %2\bin\plugins\x64\%%a.dll
