@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

for /f "usebackq tokens=*" %%a in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
   set "VSINSTALLPATH=%%a"
)

if not defined VSINSTALLPATH (
   echo No Visual Studio installation detected.
   goto end
)

if exist "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" (
   call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" x86
) else (
   goto end
)

:: Pre-cleanup (required since dotnet doesn't cleanup)
if exist "tools\CustomBuildTool\bin\Release\net6.0-x86" (
   rmdir /S /Q "tools\CustomBuildTool\bin\Release\net6.0-x86"
)
if exist "tools\CustomBuildTool\bin\Debug" (
   rmdir /S /Q "tools\CustomBuildTool\bin\Debug"
)
if exist "tools\CustomBuildTool\bin\obj" (
   rmdir /S /Q "tools\CustomBuildTool\bin\obj"
)

dotnet publish tools\CustomBuildTool\CustomBuildTool.sln -c Release /p:PublishProfile=Properties\PublishProfiles\32bit.pubxml

:: Post-cleanup (optional)
if exist "tools\CustomBuildTool\bin\Release\net6.0-x86" (
   rmdir /S /Q "tools\CustomBuildTool\bin\Release\net6.0-x86"
)
if exist "tools\CustomBuildTool\bin\Debug" (
   rmdir /S /Q "tools\CustomBuildTool\bin\Debug"
)
if exist "tools\CustomBuildTool\bin\obj" (
   rmdir /S /Q "tools\CustomBuildTool\bin\obj"
)

:end
pause
