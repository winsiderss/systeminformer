@ECHO OFF

SETLOCAL

PUSHD %~dp0

SET COVDIR=H:\progs\thirdparty\cov-analysis-win64

CALL "%VS140COMNTOOLS%\vsvars32.bat"

"%COVDIR%\bin\cov-build.exe" --dir cov-int coverity-build.bat

IF EXIST "ProcessHacker.lzma" DEL "ProcessHacker.lzma"
IF EXIST "ProcessHacker.tar"  DEL "ProcessHacker.tar"
IF EXIST "ProcessHacker.tgz"  DEL "ProcessHacker.tgz"


:tar
tar --version 1>&2 2>NUL || (ECHO. & ECHO ERROR: tar not found & GOTO SevenZip)
tar caf "ProcessHacker.lzma" "cov-int"
GOTO End

:SevenZip
IF NOT EXIST "%PROGRAMFILES%\7-Zip\7z.exe" (
  ECHO.
  ECHO ERROR: "%PROGRAMFILES%\7-Zip\7z.exe" not found
  GOTO End
)
"%PROGRAMFILES%\7-Zip\7z.exe" a -ttar "ProcessHacker.tar" "cov-int"
"%PROGRAMFILES%\7-Zip\7z.exe" a -tgzip "ProcessHacker.tgz" "ProcessHacker.tar"
IF EXIST "ProcessHacker.tar" DEL "ProcessHacker.tar"


:End
POPD
ECHO. & ECHO Press any key to close this window...
PAUSE >NUL
ENDLOCAL
EXIT /B
