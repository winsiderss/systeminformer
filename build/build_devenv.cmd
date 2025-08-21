@echo off
@setlocal enableextensions
@cd /d "%~dp0\..\"

if not exist "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" (
    echo CustomBuildTool.exe not found. Run build\build_init.cmd first.
    exit /b 1
)

start /B /W "" "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" "-devenv-build" "tools\thirdparty\thirdparty.sln /Rebuild Debug /Project thirdparty /projectconfig ""Debug|Win32"" "
start /B /W "" "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" "-devenv-build" "tools\thirdparty\thirdparty.sln /rebuild Debug /Project thirdparty /projectconfig ""Debug|x64"" "
start /B /W "" "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" "-devenv-build" "tools\thirdparty\thirdparty.sln /Rebuild Release /Project thirdparty /projectconfig ""Release|Win32"" "
start /B /W "" "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" "-devenv-build" "tools\thirdparty\thirdparty.sln /rebuild Release /Project thirdparty /projectconfig ""Release|x64"" "

start /B /W "" "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" "-devenv-build" "SystemInformer.sln /Rebuild ""Debug|Win32"" "
start /B /W "" "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" "-devenv-build" "SystemInformer.sln /Rebuild ""Debug|x64"" "
start /B /W "" "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" "-devenv-build" "SystemInformer.sln /Rebuild ""Release|Win32"" "
start /B /W "" "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" "-devenv-build" "SystemInformer.sln /Rebuild ""Release|x64"" "

start /B /W "" "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" "-devenv-build" "Plugins\Plugins.sln /Rebuild ""Debug|Win32"" "
start /B /W "" "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" "-devenv-build" "Plugins\Plugins.sln /Rebuild ""Debug|x64"" "
start /B /W "" "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" "-devenv-build" "Plugins\Plugins.sln /Rebuild ""Release|Win32"" "
start /B /W "" "tools\CustomBuildTool\bin\Release\%PROCESSOR_ARCHITECTURE%\CustomBuildTool.exe" "-devenv-build" "Plugins\Plugins.sln /Rebuild ""Release|x64"" "

pause
