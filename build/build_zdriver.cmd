@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

set "msvc2019="

if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" ( set "msvc2019=1" )

if defined msvc2019 (
   call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64_arm64

   msbuild KProcessHacker\KProcessHacker.sln -property:Configuration=Debug -property:Platform=Win32 -verbosity:minimal
   if %ERRORLEVEL% neq 0 goto end
)
else (
   call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64_arm64
)

msbuild KProcessHacker\KProcessHacker.sln -property:Configuration=Debug -property:Platform=x64 -verbosity:minimal
if %ERRORLEVEL% neq 0 goto end

msbuild KProcessHacker\KProcessHacker.sln -property:Configuration=Debug -property:Platform=ARM64 -verbosity:minimal
if %ERRORLEVEL% neq 0 goto end

:end
pause
