@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64_arm64

msbuild ProcessHacker.sln -property:Configuration=Debug -property:Platform=Win32 -verbosity:normal
if %ERRORLEVEL% neq 0 goto end

msbuild ProcessHacker.sln -property:Configuration=Debug -property:Platform=x64 -verbosity:normal
if %ERRORLEVEL% neq 0 goto end

tools\CustomBuildTool\bin\Release\CustomBuildTool.exe -sdk

msbuild Plugins\Plugins.sln -property:Configuration=Debug -property:Platform=Win32 -verbosity:normal
if %ERRORLEVEL% neq 0 goto end

msbuild Plugins\Plugins.sln -property:Configuration=Debug -property:Platform=x64 -verbosity:normal
if %ERRORLEVEL% neq 0 goto end

:end
pause
