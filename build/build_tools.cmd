@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"

cd tools\CustomBuildTool\

dotnet publish -c Release /p:PublishProfile=Properties\PublishProfiles\32bit.pubxml

pause
