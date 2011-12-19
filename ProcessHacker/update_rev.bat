@ECHO OFF
SETLOCAL

SET "SUBWCREV=SubWCRev.exe"

PUSHD %~dp0

"%SUBWCREV%" ".." "include\phapprev_in.h" "include\phapprev.h" -f
IF %ERRORLEVEL% NEQ 0 GOTO NoSubWCRev

POPD
ENDLOCAL
EXIT /B


:NoSubWCRev
ECHO. & ECHO SubWCRev, which is part of TortoiseSVN, wasn't found!
ECHO You should (re)install TortoiseSVN.
ECHO I'll use PHAPP_VERSION_REVISION=0 for now.

ECHO #ifndef PHAPPREV_H> "include\phapprev.h"
ECHO #define PHAPPREV_H>> "include\phapprev.h"
ECHO.>> "include\phapprev.h"
ECHO #define PHAPP_VERSION_REVISION 0 >> "include\phapprev.h"
ECHO.>> "include\phapprev.h"
ECHO #endif // PHAPPREV_H>> "include\phapprev.h"

POPD
ENDLOCAL
EXIT /B
