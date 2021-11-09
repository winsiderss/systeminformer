@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"

cd tools\CustomBuildTool\

dotnet publish -c Release /p:PublishProfile=Properties\PublishProfiles\32bit.pubxml

pause
