@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0\.."

REM Build user-mode Debug binaries with Clang's undefined behavior sanitizer.
REM Usage: build_ubsan.cmd [generate|build] (defaults to generate and build)
REM Requires CMake, Ninja, and a Visual Studio installation with clang-cl.

set "SI_CMAKE_BUILD_SUFFIX=ubsan"
set "SI_CMAKE_EXTRA_ARGS=-DSI_OUTPUT_DIR=%CD%/bin-cmake-ubsan -DSI_WITH_UBSAN=ON"

if /i "%~1"=="generate" (
    call build\build_cmake.cmd Ninja Debug clang-msvc-amd64 generate --no-pause
    exit /b !errorlevel!
)
if /i "%~1"=="build" (
    call build\build_cmake.cmd Ninja Debug clang-msvc-amd64 build --no-pause
    exit /b !errorlevel!
)
if not "%~1"=="" (
    echo Usage: build_ubsan.cmd [generate^|build]
    exit /b 1
)

call build\build_cmake.cmd Ninja Debug clang-msvc-amd64 generate --no-pause
if errorlevel 1 exit /b %errorlevel%
call build\build_cmake.cmd Ninja Debug clang-msvc-amd64 build
exit /b %errorlevel%
