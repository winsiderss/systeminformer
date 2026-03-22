@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

REM Check if terminal logging is supported
set "TLG=auto"
if "%GITHUB_ACTIONS%"=="true" (
    set "TLG=off"
) else if "%TF_BUILD%"=="true" (
    set "TLG=off"
)

REM Check if diagnostic is supported
set "TLV=normal"
if not "%~1"=="" (
    set "TLV=diagnostic"
)

for /f "usebackq tokens=*" %%a in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
   set "VSINSTALLPATH=%%a"
)

if not defined VSINSTALLPATH (
   echo No Visual Studio installation detected.
   goto end
)

set "VS_ARM64_SUPPORT=false"
for /f "usebackq tokens=*" %%a in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -products * -requires "Microsoft.VisualStudio.Component.VC.Tools.ARM64" -property installationPath`) do (
   set "VS_ARM64_SUPPORT=true"
)

if exist "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" (
   if "%VS_ARM64_SUPPORT%"=="true" (
      call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" amd64_arm64
   ) else (
      call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" amd64
   )
   if %ERRORLEVEL% neq 0 goto end
) else (
   echo Warning: vcvarsall.bat not found and compilation environment variables are not defined.
   echo The build may fail if the environment is not properly initialized.
   goto end
)

echo:

echo Building SystemInformer.sln [Debug32]
msbuild /m SystemInformer.sln -t:rebuild -p:Configuration=Debug -p:Platform=Win32 -verbosity:%TLV% -terminalLogger:%TLG%
if %ERRORLEVEL% neq 0 goto end
echo:

echo Building SystemInformer.sln [Debug64]
msbuild /m SystemInformer.sln -t:rebuild -p:Configuration=Debug -p:Platform=x64 -verbosity:%TLV% -terminalLogger:%TLG%
if %ERRORLEVEL% neq 0 goto end
echo:

echo Building Plugins.sln [Release32]
msbuild /m Plugins\Plugins.sln -t:rebuild -p:Configuration=Debug -p:Platform=Win32 -verbosity:%TLV% -terminalLogger:%TLG%
if %ERRORLEVEL% neq 0 goto end
echo:

echo Building Plugins.sln [Release64]
msbuild /m Plugins\Plugins.sln -t:rebuild -p:Configuration=Debug -p:Platform=x64 -verbosity:%TLV% -terminalLogger:%TLG%
if %ERRORLEVEL% neq 0 goto end
echo:

if "%VS_ARM64_SUPPORT%"=="true" (
   echo Building SystemInformer.sln [DebugARM64]
   msbuild /m SystemInformer.sln -t:rebuild -p:Configuration=Debug -p:Platform=ARM64 -verbosity:%TLV% -terminalLogger:%TLG%
   if %ERRORLEVEL% neq 0 goto end
   echo:

   echo Building Plugins.sln [ReleaseARM64]
   msbuild /m Plugins\Plugins.sln -t:rebuild -p:Configuration=Debug -p:Platform=ARM64 -verbosity:%TLV% -terminalLogger:%TLG%
   if %ERRORLEVEL% neq 0 goto end
   echo:
)

:end
pause
