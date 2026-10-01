@echo off
setlocal enabledelayedexpansion

REM -----------------------------------------------------------------------------
REM Script: build_zdriver_cab.cmd
REM Description: Builds, packages, and signs the KSystemInformer CAB output.
REM Usage:       build_zdriver_cab.cmd [-sbom]
REM   -sbom      Generate the WHCP SBOM/VEX files and include them in the CAB.
REM -----------------------------------------------------------------------------

REM Initialize script state, working paths, and tool discovery values.
set "ExitCode=0"
set "IsCI=false"
set "ScriptDir=%~dp0"
set "OutputDir=%~dp0output"
set "CabWorkDir=%~dp0output\cab"
set "CabPath=%~dp0output\KSystemInformer.cab"
set "VSINSTALLPATH="
set "IncludeSbom=false"

REM Parse command line options.
for %%A in (%*) do (
    if /i "%%~A"=="-sbom" set "IncludeSbom=true"
)

REM Run the main script flow and capture the final exit code.
call :DetectCi
call :Main
if errorlevel 1 set "ExitCode=%errorlevel%"

:end
REM Pause only for interactive, non-CI invocations before returning.
if /i "%IsCI%"=="false" call :PauseIfInteractive
endlocal & exit /b %ExitCode%

REM -----------------------------------------------------------------------------
REM Function: Main
REM Description: Builds driver binaries, packages them into a CAB, and signs the CAB.
REM -----------------------------------------------------------------------------
:Main
call :CleanupPath "%CabWorkDir%"
if errorlevel 1 exit /b %errorlevel%
call :DeleteFile "%CabPath%"
if errorlevel 1 exit /b %errorlevel%

call "%~dp0build_zdriver.cmd" release rebuild nopause
if errorlevel 1 (
    echo [-] Build failed, CAB was not generated.
    exit /b !errorlevel!
)

call :EnsureDirectory "%CabWorkDir%"
if errorlevel 1 exit /b %errorlevel%

call :RunRobocopy "%~dp0..\KSystemInformer\bin" "%CabWorkDir%" "*.sys" "*.dll" "*.pdb" /mir
if errorlevel 1 (
    echo [-] Failed to copy build artifacts to CAB directory.
    exit /b !errorlevel!
)

call :RunRobocopy "%~dp0..\KSystemInformer" "%CabWorkDir%" "KSystemInformer.inf"
if errorlevel 1 (
    echo [-] Failed to copy INF to CAB directory.
    exit /b !errorlevel!
)

call :FindVisualStudio
if errorlevel 1 exit /b %errorlevel%
call :SetupVcVars "amd64_arm64"
if errorlevel 1 exit /b %errorlevel%

set "CabDirectives=/f "%~dp0KSystemInformer.ddf""
if /i "%IncludeSbom%"=="true" (
    call :GenerateDriverSbom "x64" "Release64"
    if errorlevel 1 exit /b !errorlevel!
    call :GenerateDriverSbom "arm64" "ReleaseARM64"
    if errorlevel 1 exit /b !errorlevel!
    set "CabDirectives=!CabDirectives! /f "%~dp0KSystemInformer.sbom.ddf""
)

pushd "%CabWorkDir%"
makecab !CabDirectives!
set "ExitCode=%errorlevel%"
popd
if not "%ExitCode%"=="0" (
    echo [-] Failed to generate CAB.
    exit /b %ExitCode%
)

call :RunRobocopy "%CabWorkDir%\disk1" "%OutputDir%" "KSystemInformer.cab"
if errorlevel 1 (
    echo [-] Failed to copy CAB to output folder.
    exit /b !errorlevel!
)

call :CleanupPath "%CabWorkDir%"
if errorlevel 1 exit /b %errorlevel%

echo [+] CAB Complete!
echo [.] Preparing to sign CAB...

echo [.] Signing: %CabPath%
signtool sign /fd sha256 /n "Winsider" "%CabPath%"
if errorlevel 1 exit /b %errorlevel%

echo [+] CAB Signed!
exit /b 0

REM -----------------------------------------------------------------------------
REM Function: GenerateDriverSbom
REM Description: Generates and COSE signs the WHCP SPDX 3.0 SBOM and VEX for one
REM              driver folder (required for 25H2/26H1 submissions from 03/2027).
REM Parameters:
REM   %~1 - Driver package folder / architecture (x64, arm64).
REM   %~2 - Build output folder containing the driver binaries.
REM Environment:
REM   KphEnablePqc         - "true" when the driver was built with SymCrypt (PQC).
REM   SbomSignThumbprint   - SHA-1 thumbprint of a signing cert in a local store
REM                          (uses CustomBuildTool -sbom-driver-sign / CoseSignTool).
REM   SbomSignStoreName    - Optional store name for the thumbprint (default: My).
REM   SbomSignStoreLocation- Optional store location for the thumbprint
REM                          (default: CurrentUser).
REM   SbomSignPfx          - Path to a signing certificate (.pfx). Alternative to
REM                          SbomSignThumbprint.
REM   SbomSignPassword     - Optional password for SbomSignPfx.
REM   SbomSignCommand      - Explicit per-file signer override, invoked as
REM                          %SbomSignCommand% "<file>". Takes precedence over the
REM                          thumbprint/pfx configuration above.
REM -----------------------------------------------------------------------------
:GenerateDriverSbom
set "CustomBuildTool=%~dp0..\tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe"
if not exist "%CustomBuildTool%" (
    echo [-] CustomBuildTool.exe not found. Run build\build_init.cmd first.
    exit /b 1
)
set "SbomPqc="
if /i "%KphEnablePqc%"=="true" set "SbomPqc=--pqc"
echo [.] Generating SBOM/VEX: %~1
"%CustomBuildTool%" -sbom-driver "%~dp0.." "%~1" "%CabWorkDir%\%~2" "%CabWorkDir%\%~1" %SbomPqc%
if errorlevel 1 (
    echo [-] Failed to generate SBOM/VEX for %~1.
    exit /b 1
)
set "SbomDir=%CabWorkDir%\%~1\sbom"

REM Explicit per-file signer override.
if defined SbomSignCommand (
    for %%F in ("%SbomDir%\KSystemInformer.spdx.json" "%SbomDir%\KSystemInformer.vex.json") do (
        call %SbomSignCommand% "%%~F"
        if errorlevel 1 (
            echo [-] Failed to COSE sign %%~F.
            exit /b 1
        )
    )
    exit /b 0
)

REM Build the -sbom-driver-sign certificate arguments from the environment.
set "SbomSignArgs="
if defined SbomSignThumbprint (
    set "SbomSignArgs=--thumbprint "%SbomSignThumbprint%""
    if defined SbomSignStoreName set "SbomSignArgs=!SbomSignArgs! --store-name "%SbomSignStoreName%""
    if defined SbomSignStoreLocation set "SbomSignArgs=!SbomSignArgs! --store-location "%SbomSignStoreLocation%""
) else if defined SbomSignPfx (
    set "SbomSignArgs=--pfx "%SbomSignPfx%""
    if defined SbomSignPassword set "SbomSignArgs=!SbomSignArgs! --pw "%SbomSignPassword%""
)

if not defined SbomSignArgs (
    if /i "%IsCI%"=="true" (
        echo [-] No SBOM signing configuration set ^(SbomSignThumbprint, SbomSignPfx or SbomSignCommand^); WHCP requires COSE signed SBOM/VEX files.
        exit /b 1
    )
    echo [!] No SBOM signing configuration set; SBOM/VEX for %~1 are unsigned.
    exit /b 0
)

echo [.] COSE signing SBOM/VEX: %~1
"%CustomBuildTool%" -sbom-driver-sign "%SbomDir%" !SbomSignArgs! --validate
if errorlevel 1 (
    echo [-] Failed to COSE sign SBOM/VEX for %~1.
    exit /b 1
)
exit /b 0

REM -----------------------------------------------------------------------------
REM Function: RunRobocopy
REM Description: Copies files with robocopy and normalizes its success exit codes.
REM Parameters:
REM   %~1 - Source directory.
REM   %~2 - Destination directory.
REM   %~3..%~6 - Robocopy file patterns and options.
REM -----------------------------------------------------------------------------
:RunRobocopy
robocopy "%~1" "%~2" %~3 %~4 %~5 %~6
set "ROBOCOPY_EXIT=%errorlevel%"
if %ROBOCOPY_EXIT% lss 8 exit /b 0
exit /b %ROBOCOPY_EXIT%

REM -----------------------------------------------------------------------------
REM Function: EnsureDirectory
REM Description: Creates a directory if it does not already exist.
REM Parameters:
REM   %~1 - Directory path to create.
REM -----------------------------------------------------------------------------
:EnsureDirectory
if not exist "%~1" mkdir "%~1"
exit /b %errorlevel%

REM -----------------------------------------------------------------------------
REM Function: CleanupPath
REM Description: Removes a directory tree if it exists.
REM Parameters:
REM   %~1 - Directory path to remove.
REM -----------------------------------------------------------------------------
:CleanupPath
if exist "%~1" rmdir /Q /S "%~1"
exit /b 0

REM -----------------------------------------------------------------------------
REM Function: DeleteFile
REM Description: Deletes a file if it exists.
REM Parameters:
REM   %~1 - File path to remove.
REM -----------------------------------------------------------------------------
:DeleteFile
if exist "%~1" del "%~1"
exit /b 0

REM -----------------------------------------------------------------------------
REM Function: FindVisualStudio
REM Description: Locates a Visual Studio or SDK installation with signing tools.
REM -----------------------------------------------------------------------------
:FindVisualStudio
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%A in (`call "%VSWHERE%" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
        set "VSINSTALLPATH=%%A"
    )
)
if not defined VSINSTALLPATH if defined VSINSTALLDIR set "VSINSTALLPATH=%VSINSTALLDIR%"
if not defined VSINSTALLPATH if defined VCINSTALLDIR for %%I in ("%VCINSTALLDIR%\..\..") do set "VSINSTALLPATH=%%~fI"
if not defined VSINSTALLPATH if defined WindowsSdkDir set "VSINSTALLPATH=%WindowsSdkDir%"
if defined VSINSTALLPATH exit /b 0
echo [-] Visual Studio not found
exit /b 1

REM -----------------------------------------------------------------------------
REM Function: SetupVcVars
REM Description: Initializes the Visual C++ build environment for the requested arch.
REM Parameters:
REM   %~1 - vcvarsall architecture argument.
REM -----------------------------------------------------------------------------
:SetupVcVars
if /i "%EnterpriseWDK%"=="true" (
    REM EWDK has already configured INCLUDE/LIB/PATH via LaunchBuildEnv.cmd.
    exit /b 0
)
if exist "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" (
    call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" %~1
    exit /b !errorlevel!
)
echo [-] Failed to set up build environment
exit /b 1

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
