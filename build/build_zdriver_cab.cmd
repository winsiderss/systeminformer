@echo off
@setlocal enableextensions

rmdir /q /s %~dp0\output\cab
del %~dp0\output\KSystemInformer.cab

call %~dp0\build_zdriver.cmd release rebuild
if %ERRORLEVEL% neq 0 (
    echo [-] Build failed, CAB was not generated. 
    goto end
)

mkdir %~dp0\output\cab 

(robocopy %~dp0\..\KSystemInformer\bin %~dp0\output\cab *.sys *.dll *.pdb /mir) ^& if %ERRORLEVEL% lss 8 set ERRORLEVEL = 0
if %ERRORLEVEL% neq 0 (
    echo [-] Failed to copy build artifacts to CAB directory.
    goto end
)

(robocopy %~dp0\..\KSystemInformer %~dp0\output\cab KSystemInformer.inf) ^& if %ERRORLEVEL% lss 8 set ERRORLEVEL = 0
if %ERRORLEVEL% neq 0 (
    echo [-] Failed to copy INF to CAB directory.
    goto end
)

pushd %~dp0\output\cab
makecab /f %~dp0\KSystemInformer.ddf
if %ERRORLEVEL% neq 0 (
    echo [-] Failed to generate CAB. 
    popd
    goto end
)
popd

(robocopy %~dp0\output\cab\disk1 %~dp0\output KSystemInformer.cab) ^& if %ERRORLEVEL% lss 8 set ERRORLEVEL = 0
if %ERRORLEVEL% neq 0 (
    echo [-] Failed to copy CAB to output folder.
    goto end
)

rmdir /q /s %~dp0\output\cab

echo [+] CAB Complete!

echo [.] Preparing to sign CAB...

for /f "usebackq tokens=*" %%A in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
    set VSINSTALLPATH=%%A
)

if not defined VSINSTALLPATH (
    echo [-] Visual Studio not found
    goto end
)

if exist "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" (
   call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" amd64_arm64
) else (
    echo [-] Failed to set up build environment
    goto end
)

echo [.] Signing: %~dp0\output\KSystemInformer.cab
signtool sign /fd sha256 /n "Winsider" %~dp0\output\KSystemInformer.cab
if %ERRORLEVEL% neq 0 goto end

echo [+] CAB Signed!

:end
pause
