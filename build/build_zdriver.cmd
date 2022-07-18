@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

for /f "usebackq tokens=*" %%A in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -version "[16.0,17.0)" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
   set VSINSTALLPATH2019=%%A
)

for /f "usebackq tokens=*" %%A in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
   set VSINSTALLPATH=%%A
)

if defined VSINSTALLPATH2019 (

   if exist "%VSINSTALLPATH2019%\VC\Auxiliary\Build\vcvarsall.bat" (
      call "%VSINSTALLPATH2019%\VC\Auxiliary\Build\vcvarsall.bat" amd64_arm64
   ) else (
      goto end
   )

   msbuild KSystemInformer\KSystemInformer.sln -property:Configuration=Debug -property:Platform=Win32 -verbosity:minimal
   if %ERRORLEVEL% neq 0 goto end

   msbuild KSystemInformer\KSystemInformer.sln -property:Configuration=Debug -property:Platform=x64 -verbosity:minimal
   if %ERRORLEVEL% neq 0 goto end

   msbuild KSystemInformer\KSystemInformer.sln -property:Configuration=Debug -property:Platform=ARM64 -verbosity:minimal
   if %ERRORLEVEL% neq 0 goto end

) else (
 
   if not defined VSINSTALLPATH (
      goto end
   )

   if exist "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" (
      call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" amd64_arm64
   ) else (
      goto end
   )

   msbuild KSystemInformer\KSystemInformer.sln -property:Configuration=Debug -property:Platform=x64 /p:TargetVersion="Windows10" -verbosity:minimal
   if %ERRORLEVEL% neq 0 goto end

   msbuild KSystemInformer\KSystemInformer.sln -property:Configuration=Debug -property:Platform=ARM64 /p:TargetVersion="Windows10" -verbosity:minimal
   if %ERRORLEVEL% neq 0 goto end
)

:end
pause
