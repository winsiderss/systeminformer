@echo off

PowerShell.exe -ExecutionPolicy Bypass -Command "& .\nightly.ps1 -debug $true"

pause