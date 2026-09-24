@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0\.."

REM -----------------------------------------------------------------------------
REM Script: build_zsign.cmd
REM Description: Signs build outputs through CustomBuildTool.
REM -----------------------------------------------------------------------------

REM Initialize script state and tool paths.
set "ExitCode=0"
set "IsCI=false"
set "CustomBuildTool=tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe"

REM Run the main script flow and capture the final exit code.
call :DetectCi
call :Main "%~1"
if errorlevel 1 set "ExitCode=%errorlevel%"

:end
REM Pause only for interactive, non-CI invocations before returning.
if /i "%IsCI%"=="false" call :PauseIfInteractive
endlocal & exit /b %ExitCode%

REM -----------------------------------------------------------------------------
REM Function: Main
REM Description: Signs a file, or all executable files in a directory tree.
REM Parameters:
REM   %~1 - Optional file or directory path. Defaults to the bin directory.
REM -----------------------------------------------------------------------------
:Main
set "TargetPath=%~1"
if not defined TargetPath set "TargetPath=bin"

if not exist "%TargetPath%" (
    echo Path not found: %TargetPath%
    exit /b 1
)

call :CheckCustomBuildTool
if errorlevel 1 exit /b %errorlevel%

echo:
if exist "%TargetPath%\." (
    call :SignDirectory "%TargetPath%"
) else (
    call :SignFile "%TargetPath%"
)
if errorlevel 1 exit /b !errorlevel!
echo:

exit /b 0

REM -----------------------------------------------------------------------------
REM Function: SignDirectory
REM Description: Recursively signs executable and library files in a directory.
REM Parameters:
REM   %~1 - Directory containing the files that should be signed.
REM -----------------------------------------------------------------------------
:SignDirectory
set "SignedFiles=0"
for /r "%~1" %%F in (*.exe *.dll) do (
    call :SignFile "%%~fF"
    if errorlevel 1 exit /b !errorlevel!
    set /a SignedFiles+=1 >nul
)

if !SignedFiles! equ 0 (
    echo No executable or library files found in %~1
    exit /b 1
)

echo Signed !SignedFiles! files.
exit /b 0

REM -----------------------------------------------------------------------------
REM Function: SignFile
REM Description: Creates a KPH signature for one file.
REM Parameters:
REM   %~1 - File to sign.
REM -----------------------------------------------------------------------------
:SignFile
echo Signing %~1
call :RunCustomBuildTool "-kphsign" "%~1"
exit /b %errorlevel%

REM -----------------------------------------------------------------------------
REM Function: CheckCustomBuildTool
REM Description: Ensures the CustomBuildTool executable is available.
REM -----------------------------------------------------------------------------
:CheckCustomBuildTool
if exist "%CustomBuildTool%" exit /b 0
echo CustomBuildTool.exe not found in tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE% folder.
exit /b 1

REM -----------------------------------------------------------------------------
REM Function: RunCustomBuildTool
REM Description: Executes CustomBuildTool with the supplied arguments.
REM Parameters:
REM   %* - Arguments forwarded to CustomBuildTool.
REM -----------------------------------------------------------------------------
:RunCustomBuildTool
start /B /W "" "%CustomBuildTool%" %*
exit /b %errorlevel%

REM -----------------------------------------------------------------------------
REM Function: DetectCi
REM Description: Detects whether the script is running under CI.
REM -----------------------------------------------------------------------------
:DetectCi
if /i "%GITHUB_ACTIONS%"=="true" set "IsCI=true"
if /i "%TF_BUILD%"=="true" set "IsCI=true"
exit /b 0

REM -----------------------------------------------------------------------------
REM Function: PauseIfInteractive
REM Description: Pauses only when stdin is attached to an interactive console.
REM -----------------------------------------------------------------------------
:PauseIfInteractive
set "STDIN_REDIRECTED=False"
for /f %%i in ('powershell -NoProfile -Command "[Console]::IsInputRedirected"') do set "STDIN_REDIRECTED=%%i"
if /i not "%STDIN_REDIRECTED%"=="True" pause
exit /b 0
