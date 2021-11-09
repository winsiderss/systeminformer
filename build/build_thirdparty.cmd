@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64_arm64

msbuild tools\thirdparty\thirdparty.sln -property:Configuration=Debug -property:Platform=x86 -verbosity:normal
if %ERRORLEVEL% neq 0 goto end

msbuild tools\thirdparty\thirdparty.sln -property:Configuration=Release -property:Platform=x86 -verbosity:normal
if %ERRORLEVEL% neq 0 goto end

msbuild tools\thirdparty\thirdparty.sln -property:Configuration=Debug -property:Platform=x64 -verbosity:normal
if %ERRORLEVEL% neq 0 goto end

msbuild tools\thirdparty\thirdparty.sln -property:Configuration=Release -property:Platform=x64 -verbosity:normal
if %ERRORLEVEL% neq 0 goto end

msbuild tools\thirdparty\thirdparty.sln -property:Configuration=Debug -property:Platform=ARM64 -verbosity:normal
if %ERRORLEVEL% neq 0 goto end

msbuild tools\thirdparty\thirdparty.sln -property:Configuration=Release -property:Platform=ARM64 -verbosity:normal
if %ERRORLEVEL% neq 0 goto end

:end
pause
