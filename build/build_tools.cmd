@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

dotnet publish tools\CustomBuildTool\CustomBuildTool.sln -c Release /p:PublishProfile=Properties\PublishProfiles\64bit.pubxml
