@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

set "TIB=false"
if "%GITHUB_ACTIONS%"=="true" (
    set "TIB=true"
) else if "%TF_BUILD%"=="true" (
    set "TIB=true"
)

set GENERATOR=%~1
set CONFIG=%~2
set TOOLCHAIN=%~3
set ACTION=%~4
set TOOLCHAIN_FILE=
set BUILD_DIR=

for /f "usebackq tokens=*" %%a in (`call "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
   set "VSINSTALLPATH=%%a"
)

if not defined VSINSTALLPATH (
   echo No Visual Studio installation detected.
   goto end
)

if /i "%CONFIG%"=="Debug" (
    set BUILD_DIR=build-debug
) else if /i "%CONFIG%"=="Release" (
    set BUILD_DIR=build-release
) else (
    echo Error: Unsupported configuration: %CONFIG%
    echo Supported configurations: Debug, Release
    set ERRORLEVEL=1
    goto end
)

if /i "%TOOLCHAIN%"=="msvc-x86" (
    set PLATFORM=Win32
    set VCVARS_ARCH=amd64_x86
    set BUILD_DIR="%CD%\%BUILD_DIR%-msvc-32"
    set TOOLCHAIN_FILE="%CD%\cmake\toolchain\msvc-x86.cmake"
) else if /i "%TOOLCHAIN%"=="msvc-amd64" (
    set PLATFORM=x64
    set VCVARS_ARCH=amd64
    set BUILD_DIR="%CD%\%BUILD_DIR%-msvc-64"
    set TOOLCHAIN_FILE="%CD%\cmake\toolchain\msvc-amd64.cmake"
) else if /i "%TOOLCHAIN%"=="msvc-arm64" (
    set PLATFORM=ARM64
    set VCVARS_ARCH=amd64_arm64
    set BUILD_DIR="%CD%\%BUILD_DIR%-msvc-arm64"
    set TOOLCHAIN_FILE="%CD%\cmake\toolchain\msvc-arm64.cmake"
) else if /i "%TOOLCHAIN%"=="clang-msvc-x86" (
    set PLATFORM=Win32
    set VCVARS_ARCH=amd64_x86
    set BUILD_DIR="%CD%\%BUILD_DIR%-clang-msvc-32"
    set TOOLCHAIN_FILE="%CD%\cmake\toolchain\clang-msvc-x86.cmake"
) else if /i "%TOOLCHAIN%"=="clang-msvc-amd64" (
    set PLATFORM=x64
    set VCVARS_ARCH=amd64
    set BUILD_DIR="%CD%\%BUILD_DIR%-clang-msvc-64"
    set TOOLCHAIN_FILE="%CD%\cmake\toolchain\clang-msvc-amd64.cmake"
) else if /i "%TOOLCHAIN%"=="clang-msvc-arm64" (
    set PLATFORM=ARM64
    set VCVARS_ARCH=amd64_arm64
    set BUILD_DIR="%CD%\%BUILD_DIR%-clang-msvc-arm64"
    set TOOLCHAIN_FILE="%CD%\cmake\toolchain\clang-msvc-arm64.cmake"
) else (
    echo Error: Unsupported toolchain: %TOOLCHAIN%
    echo Supported toolchains: msvc-x86, msvc-amd64, msvc-arm64, clang-msvc-x86, clang-msvc-amd64, clang-msvc-arm64
    set ERRORLEVEL=1
    goto end
)

set SOURCE_DIR="%CD%"

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

set CMAKE_GEN_OPTS=
if /i "%GENERATOR%"=="Ninja" (
    set CMAKE_GEN_OPTS=-DCMAKE_BUILD_TYPE=%CONFIG%
)

set CMAKE_BUILD_OPTS=
if not "%GENERATOR:Visual Studio=%"=="%GENERATOR%" (
    set CMAKE_BUILD_OPTS=-- /m /p:Platform=%PLATFORM% -terminalLogger:auto
)

if /i "%ACTION%"=="clean" (
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
) else if /i "%ACTION%"=="generate" (
    echo Setting up Visual Studio environment for %VCVARS_ARCH%...
    call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" %VCVARS_ARCH%
    if %ERRORLEVEL% neq 0 goto end
    if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
    cmake -G "%GENERATOR%" -S %SOURCE_DIR% -B %BUILD_DIR% ^
        --toolchain %TOOLCHAIN_FILE% ^
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ^
        -DSI_WITH_CORE=ON ^
        -DSI_WITH_PLUGINS=ON ^
        %CMAKE_GEN_OPTS%
    if %ERRORLEVEL% neq 0 goto end
) else if /i "%ACTION%"=="build" (
    echo Setting up Visual Studio environment for %VCVARS_ARCH%...
    call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" %VCVARS_ARCH%
    if %ERRORLEVEL% neq 0 goto end
    cmake --build %BUILD_DIR% --config %CONFIG% %CMAKE_BUILD_OPTS%
    if %ERRORLEVEL% neq 0 goto end
) else (
    echo Error: Invalid action "%ACTION%". Use "generate", "build", or "clean".
    set ERRORLEVEL=1
)

:end
if "%TIB%"=="false" pause
