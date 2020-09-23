@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64_arm64

msbuild ProcessHacker.sln -property:Configuration=Debug -property:Platform=Win32 -verbosity:normal
msbuild ProcessHacker.sln -property:Configuration=Debug -property:Platform=x64 -verbosity:normal

tools\CustomBuildTool\bin\Release\CustomBuildTool.exe -sdk

msbuild Plugins\Plugins.sln -property:Configuration=Debug -property:Platform=Win32 -verbosity:normal
msbuild Plugins\Plugins.sln -property:Configuration=Debug -property:Platform=x64 -verbosity:normal

pause
