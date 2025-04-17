@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

set "TIB=false"
if "%GITHUB_ACTIONS%"=="true" (
    set "TIB=true"
) else if "%TF_BUILD%"=="true" (
    set "TIB=true"
)

for /f "usebackq tokens=*" %%a in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
   set "VSINSTALLPATH=%%a"
)

if not defined VSINSTALLPATH (
   echo No Visual Studio installation detected.
   goto end
)

:: Pre-cleanup (required since dotnet doesn't cleanup)
if exist "tools\CustomBuildTool\bin\" (
   rmdir /S /Q "tools\CustomBuildTool\bin\"
)
if exist "tools\CustomBuildTool\obj" (
   rmdir /S /Q "tools\CustomBuildTool\obj"
)
if exist "tools\CustomBuildTool\.vs" (
   rmdir /S /Q "tools\CustomBuildTool\.vs"
)

if exist "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" (
   call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" amd64
   dotnet publish tools\CustomBuildTool\CustomBuildTool.sln -c Release /p:PublishProfile=Properties\PublishProfiles\amd64.pubxml /p:ContinuousIntegrationBuild=%TIB%
   call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" arm64
   dotnet publish tools\CustomBuildTool\CustomBuildTool.sln -c Release /p:PublishProfile=Properties\PublishProfiles\arm64.pubxml /p:ContinuousIntegrationBuild=%TIB%
) else (
   goto end
)

:: Post-cleanup (required since dotnet doesn't cleanup)
if exist "tools\CustomBuildTool\bin\ARM64" (
   rmdir /S /Q "tools\CustomBuildTool\bin\ARM64"
)
if exist "tools\CustomBuildTool\bin\x64" (
   rmdir /S /Q "tools\CustomBuildTool\bin\x64"
)
if exist "tools\CustomBuildTool\bin\Release\net9.0-windows-arm64" (
   rmdir /S /Q "tools\CustomBuildTool\bin\Release\net9.0-windows-arm64"
)
if exist "tools\CustomBuildTool\bin\Release\net9.0-windows-x64" (
   rmdir /S /Q "tools\CustomBuildTool\bin\Release\net9.0-windows-x64"
)
if exist "tools\CustomBuildTool\obj" (
   rmdir /S /Q "tools\CustomBuildTool\obj"
)
if exist "tools\CustomBuildTool\.vs" (
   rmdir /S /Q "tools\CustomBuildTool\.vs"
)

if exist "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" (
   echo:
   start /B /W "" "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" "-write-tools-id"
   echo:
) else (
   goto end
)

:end
pause
