@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0\.."

REM -----------------------------------------------------------------------------
REM Script: build_cmake.cmd
REM Description: Configures, builds, or cleans CMake out-of-tree build directories.
REM -----------------------------------------------------------------------------

REM Initialize script state, incoming arguments, and resolved build settings.
set "ExitCode=0"
set "IsCI=false"
set "BuildTerminalLogger=auto"
set "GENERATOR=%~1"
set "CONFIG=%~2"
set "TOOLCHAIN=%~3"
set "ACTION=%~4"
set "VSINSTALLPATH="
set "PLATFORM="
set "VCVARS_ARCH="
set "TOOLCHAIN_FILE="
set "BUILD_DIR="
set "BUILD_DIR_NAME="
set "SOURCE_DIR=%CD%"
set "CMAKE_GEN_OPTS="
set "CMAKE_BUILD_OPTS="

REM Run the main script flow and capture the final exit code.
call :DetectCi
call :ConfigureBuildLogger
call :Main
if errorlevel 1 set "ExitCode=%errorlevel%"

:end
REM Pause only for interactive, non-CI invocations before returning.
if /i "%IsCI%"=="false" call :PauseIfInteractive
endlocal & exit /b %ExitCode%

REM -----------------------------------------------------------------------------
REM Function: Main
REM Description: Validates inputs, resolves toolchain settings, and dispatches the action.
REM -----------------------------------------------------------------------------
:Main
call :ValidateArgs
if errorlevel 1 exit /b %errorlevel%

call :ResolveConfig
if errorlevel 1 exit /b %errorlevel%

call :ResolveToolchain
if errorlevel 1 exit /b %errorlevel%

call :ConfigureGeneratorOptions
if errorlevel 1 exit /b %errorlevel%

call :PrintBanner
if errorlevel 1 exit /b %errorlevel%

if /i "%ACTION%"=="clean" (
    call :RunClean
    exit /b !errorlevel!
)

call :FindVisualStudio
if errorlevel 1 exit /b %errorlevel%

call :SetupVcVars "%VCVARS_ARCH%"
if errorlevel 1 exit /b %errorlevel%

if /i "%ACTION%"=="generate" (
    call :RunGenerate
    exit /b !errorlevel!
)

if /i "%ACTION%"=="build" (
    if "%NINJA_STATUS%"=="" set "NINJA_STATUS=[%%p][%%w][%%f/%%t][%%o/s] "
    call :RunBuild
    exit /b !errorlevel!
)

echo Error: Invalid action "%ACTION%". Use "generate", "build", or "clean".
exit /b 1

REM -----------------------------------------------------------------------------
REM Function: ValidateArgs
REM Description: Ensures all required positional arguments were provided.
REM -----------------------------------------------------------------------------
:ValidateArgs
if not "%GENERATOR%"=="" if not "%CONFIG%"=="" if not "%TOOLCHAIN%"=="" if not "%ACTION%"=="" exit /b 0
echo Usage: build_cmake.cmd [GENERATOR] [CONFIG] [TOOLCHAIN] [ACTION]
echo Example: build_cmake.cmd "Ninja" "Debug" "msvc-amd64" "generate"
exit /b 1

REM -----------------------------------------------------------------------------
REM Function: ResolveConfig
REM Description: Resolves the requested configuration to a build directory prefix.
REM -----------------------------------------------------------------------------
:ResolveConfig
if /i "%CONFIG%"=="Debug" (
    set "BUILD_DIR_NAME=build-debug"
    exit /b 0
)
if /i "%CONFIG%"=="Release" (
    set "BUILD_DIR_NAME=build-release"
    exit /b 0
)
echo Error: Unsupported configuration: %CONFIG%
echo Supported configurations: Debug, Release
exit /b 1

REM -----------------------------------------------------------------------------
REM Function: ResolveToolchain
REM Description: Resolves the requested toolchain to platform, vcvars, and CMake paths.
REM -----------------------------------------------------------------------------
:ResolveToolchain
if /i "%TOOLCHAIN%"=="msvc-x86" (
    set "PLATFORM=Win32"
    set "VCVARS_ARCH=amd64_x86"
    set "BUILD_DIR=%CD%\%BUILD_DIR_NAME%-msvc-32"
    set "TOOLCHAIN_FILE=%CD%\cmake\toolchain\msvc-x86.cmake"
    exit /b 0
)
if /i "%TOOLCHAIN%"=="msvc-amd64" (
    set "PLATFORM=x64"
    set "VCVARS_ARCH=amd64"
    set "BUILD_DIR=%CD%\%BUILD_DIR_NAME%-msvc-64"
    set "TOOLCHAIN_FILE=%CD%\cmake\toolchain\msvc-amd64.cmake"
    exit /b 0
)
if /i "%TOOLCHAIN%"=="msvc-arm64" (
    set "PLATFORM=ARM64"
    set "VCVARS_ARCH=amd64_arm64"
    set "BUILD_DIR=%CD%\%BUILD_DIR_NAME%-msvc-arm64"
    set "TOOLCHAIN_FILE=%CD%\cmake\toolchain\msvc-arm64.cmake"
    exit /b 0
)
if /i "%TOOLCHAIN%"=="clang-msvc-x86" (
    set "PLATFORM=Win32"
    set "VCVARS_ARCH=amd64_x86"
    set "BUILD_DIR=%CD%\%BUILD_DIR_NAME%-clang-msvc-32"
    set "TOOLCHAIN_FILE=%CD%\cmake\toolchain\clang-msvc-x86.cmake"
    exit /b 0
)
if /i "%TOOLCHAIN%"=="clang-msvc-amd64" (
    set "PLATFORM=x64"
    set "VCVARS_ARCH=amd64"
    set "BUILD_DIR=%CD%\%BUILD_DIR_NAME%-clang-msvc-64"
    set "TOOLCHAIN_FILE=%CD%\cmake\toolchain\clang-msvc-amd64.cmake"
    exit /b 0
)
if /i "%TOOLCHAIN%"=="clang-msvc-arm64" (
    set "PLATFORM=ARM64"
    set "VCVARS_ARCH=amd64_arm64"
    set "BUILD_DIR=%CD%\%BUILD_DIR_NAME%-clang-msvc-arm64"
    set "TOOLCHAIN_FILE=%CD%\cmake\toolchain\clang-msvc-arm64.cmake"
    exit /b 0
)
echo Error: Unsupported toolchain: %TOOLCHAIN%
echo Supported toolchains: msvc-x86, msvc-amd64, msvc-arm64, clang-msvc-x86, clang-msvc-amd64, clang-msvc-arm64
exit /b 1

REM -----------------------------------------------------------------------------
REM Function: ConfigureGeneratorOptions
REM Description: Sets generator-specific CMake generate/build options.
REM -----------------------------------------------------------------------------
:ConfigureGeneratorOptions
set "CMAKE_GEN_OPTS="
set "CMAKE_BUILD_OPTS="
if /i "%GENERATOR%"=="Ninja" set "CMAKE_GEN_OPTS=-DCMAKE_BUILD_TYPE=%CONFIG%"
if not "%GENERATOR:Visual Studio=%"=="%GENERATOR%" set "CMAKE_BUILD_OPTS=-- /m /p:Platform=%PLATFORM% -terminalLogger:%BuildTerminalLogger%"
exit /b 0

REM -----------------------------------------------------------------------------
REM Function: PrintBanner
REM Description: Prints the resolved CMake build configuration before execution.
REM -----------------------------------------------------------------------------
:PrintBanner
echo ===============================================
echo System Informer CMake Build Script
echo ===============================================
echo Action:        %ACTION%
echo Generator:     %GENERATOR%
echo Configuration: %CONFIG%
echo Platform:      %PLATFORM%
echo Toolchain:     %TOOLCHAIN%
echo VCVARS:        %VCVARS_ARCH%
echo Source:        %SOURCE_DIR%
echo Build:         %BUILD_DIR%
echo ===============================================
exit /b 0

REM -----------------------------------------------------------------------------
REM Function: RunClean
REM Description: Removes the resolved CMake build directory if it exists.
REM -----------------------------------------------------------------------------
:RunClean
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
exit /b 0

REM -----------------------------------------------------------------------------
REM Function: RunGenerate
REM Description: Creates the build directory and runs CMake generate.
REM -----------------------------------------------------------------------------
:RunGenerate
echo Setting up Visual Studio environment for %VCVARS_ARCH%...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cmake -G "%GENERATOR%" -S "%SOURCE_DIR%" -B "%BUILD_DIR%" --toolchain "%TOOLCHAIN_FILE%" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DSI_WITH_CORE=ON -DSI_WITH_PLUGINS=ON %CMAKE_GEN_OPTS%
exit /b %errorlevel%

REM -----------------------------------------------------------------------------
REM Function: RunBuild
REM Description: Invokes CMake build for the resolved build directory.
REM -----------------------------------------------------------------------------
:RunBuild
echo Setting up Visual Studio environment for %VCVARS_ARCH%...
cmake --build "%BUILD_DIR%" --config "%CONFIG%" %CMAKE_BUILD_OPTS%
exit /b %errorlevel%

REM -----------------------------------------------------------------------------
REM Function: FindVisualStudio
REM Description: Locates a Visual Studio or SDK installation with MSBuild.
REM -----------------------------------------------------------------------------
:FindVisualStudio
for /f "usebackq tokens=*" %%a in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
   set "VSINSTALLPATH=%%a"
)
if not defined VSINSTALLPATH if defined WindowsSdkDir set "VSINSTALLPATH=%WindowsSdkDir%"
if not defined VSINSTALLPATH if defined EWDK_ROOT set "VSINSTALLPATH=%EWDK_ROOT%"
if defined VSINSTALLPATH exit /b 0
echo No Visual Studio installation detected.
exit /b 1

REM -----------------------------------------------------------------------------
REM Function: SetupVcVars
REM Description: Initializes the Visual C++ build environment for the requested arch.
REM Parameters:
REM   %~1 - vcvarsall architecture argument.
REM -----------------------------------------------------------------------------
:SetupVcVars
if exist "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" (
    call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" %~1
    exit /b !errorlevel!
)
echo vcvarsall.bat not found under %VSINSTALLPATH%.
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
REM Function: ConfigureBuildLogger
REM Description: Disables the Visual Studio terminal logger when running under CI.
REM -----------------------------------------------------------------------------
:ConfigureBuildLogger
if /i "%IsCI%"=="true" set "BuildTerminalLogger=off"
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
