@echo off

set /p choice="Enable debug output? (Y/N): "

if "%choice%" == "y" (
  PowerShell.exe -ExecutionPolicy Bypass -Command "& .\nightly.ps1 -debug $true"
)

if "%choice%" == "n" ( 
  PowerShell.exe -ExecutionPolicy Bypass -File .\nightly.ps1
)

pause