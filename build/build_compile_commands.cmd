@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

for /f "usebackq tokens=*" %%a in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
   set "VSINSTALLPATH=%%a"
)

if exist "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" (
    if "%PROCESSOR_ARCHITECTURE%"=="ARM64" (
       call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" arm64
       dotnet publish tools\CompileCommandsJson\CompileCommandsJson.sln -c Release /p:PublishProfile=Properties\PublishProfiles\arm64.pubxml
       set COMPILE_COMMANDS_LOGGER=tools\CompileCommandsJson\bin\Release\arm64\CompileCommandsJson.dll
       set BUILD_PLATFORM=ARM64
    ) else (
       call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" amd64
       dotnet publish tools\CompileCommandsJson\CompileCommandsJson.sln -c Release /p:PublishProfile=Properties\PublishProfiles\amd64.pubxml
       set COMPILE_COMMANDS_LOGGER=tools\CompileCommandsJson\bin\Release\amd64\CompileCommandsJson.dll
       set BUILD_PLATFORM=x64
    )
) else (
   goto end
)

if not exist %COMPILE_COMMANDS_LOGGER% (
    echo CompileCommandsJson.dll not found.
    goto end
)

set BUILD_ARGS=-t:rebuild -p:Configuration=Debug;Platform=%BUILD_PLATFORM% -logger:%COMPILE_COMMANDS_LOGGER% --terminalLogger:auto

msbuild /m KSystemInformer\KSystemInformer.sln %BUILD_ARGS%
msbuild /m SystemInformer.sln %BUILD_ARGS%
msbuild /m Plugins\Plugins.sln %BUILD_ARGS%

:end
pause
