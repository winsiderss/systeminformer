@ECHO OFF

SET MSBUILD_SWITCHES=/nologo /consoleloggerparameters:Verbosity=minimal /maxcpucount^
 /nodeReuse:true /target:Rebuild /property:Configuration="Release"

MSBuild "..\..\ProcessHacker.sln" %MSBUILD_SWITCHES%;Platform=Win32
MSBuild "..\..\ProcessHacker.sln" %MSBUILD_SWITCHES%;Platform=x64

PUSHD ..\sdk
CALL "makesdk.cmd"
POPD

MSBuild "..\..\plugins\Plugins.sln" %MSBUILD_SWITCHES%;Platform=Win32
MSBuild "..\..\plugins\Plugins.sln" %MSBUILD_SWITCHES%;Platform=x64
