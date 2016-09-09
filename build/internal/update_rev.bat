@ECHO OFF
SETLOCAL

SET "UPDATEGITREVISION=..\..\tools\UpdateGitRevision\UpdateGitRevision\bin\Release\UpdateGitRevision.exe"

PUSHD %~dp0

"%UPDATEGITREVISION%" "..\..\ProcessHacker\include\phapprev_in.h" "..\..\ProcessHacker\include\phapprev.h"
IF %ERRORLEVEL% NEQ 0 GOTO NoUpdateGitRevision

POPD
ENDLOCAL
EXIT /B


:NoUpdateGitRevision
ECHO. & ECHO UpdateGitRevision.exe wasn't found!
ECHO You need to build it.
ECHO I'll use PHAPP_VERSION_REVISION=0 for now.

ECHO #ifndef PHAPPREV_H > "..\..\ProcessHacker\include\phapprev.h"
ECHO #define PHAPPREV_H >> "..\..\ProcessHacker\include\phapprev.h"
ECHO. >> "..\..\ProcessHacker\include\phapprev.h"
ECHO #define PHAPP_VERSION_REVISION 0 >> "..\..\ProcessHacker\include\phapprev.h"
ECHO. >> "..\..\ProcessHacker\include\phapprev.h"
ECHO #endif // PHAPPREV_H >> "..\..\ProcessHacker\include\phapprev.h"

POPD
ENDLOCAL
EXIT /B