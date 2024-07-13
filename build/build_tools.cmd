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


:: Pre-cleanup (required since dotnet doesn't cleanup)
if exist "tools\CustomBuildTool\bin\Release" (
   rmdir /S /Q "tools\CustomBuildTool\bin\Release"
)
if exist "tools\CustomBuildTool\bin\" (
   rmdir /S /Q "tools\CustomBuildTool\bin\"
)
if exist "tools\CustomBuildTool\obj" (
   rmdir /S /Q "tools\CustomBuildTool\obj"
)

if exist "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" (
   call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" amd64
   dotnet publish tools\CustomBuildTool\CustomBuildTool.sln -c Release /p:PublishProfile=Properties\PublishProfiles\amd64.pubxml /p:ContinuousIntegrationBuild=true
   call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" arm64
   dotnet publish tools\CustomBuildTool\CustomBuildTool.sln -c Release /p:PublishProfile=Properties\PublishProfiles\arm64.pubxml /p:ContinuousIntegrationBuild=true
) else (
   goto end
)

:end
pause
