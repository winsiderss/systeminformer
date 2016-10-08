#
# Process Hacker Toolchain - 
#   Plugin SDK build script
#
# Copyright (C) 2016 dmex
#
# This file is part of Process Hacker.
#
# Process Hacker is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Process Hacker is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
#

# Script entry-point arguments:
Param([bool] $rebuild);

function InitializeScriptEnvironment()
{
    # Get script start time
    $global:TimeStart = (Get-Date);

    # Stop script execution after any errors.
    $global:ErrorActionPreference = "Stop";

    # Clear the console
    Clear-Host;
}

function CheckBaseDirectory()
{
    # Check if the current directory contains the main solution
    if (!(Test-Path "ProcessHacker.sln"))
    {
        # Change root directory to the \build\internal\ directory (where this script is located).
        Set-Location $PSScriptRoot;

        # Set the current location to the base repository directory.
        Set-Location "..\..\";

        # Re-check if the current directory
        if (!(Test-Path "ProcessHacker.sln"))
        {
            Write-Host "Unable to find project directory... Exiting." -ForegroundColor Red
            exit 5
        }
    }
}

function CleanSdk()
{
    if (Test-Path "sdk")
    {
        Remove-Item "sdk" -Recurse -Force
    }

    if (Test-Path "ProcessHacker\sdk\phapppub.h")
    {
        Remove-Item "ProcessHacker\sdk\phapppub.h" -Force
    } 
}

function BuildSolution([string] $FileName)
{
    # Set msbuild path (TODO: Do we need the 32bit version of MSBuild for x86 builds?)
    $msBuild = "${env:ProgramFiles(x86)}\MSBuild\14.0\bin\amd64\MSBuild.exe"
    # Get base file name
    $baseName = [System.IO.Path]::GetFileNameWithoutExtension($FileName);

    Write-Host "Building $baseName" -NoNewline -ForegroundColor Cyan

    # Debug builds
    & $msBuild  "/m",
                "/nologo",
                "/nodeReuse:true",
                "/verbosity:quiet",
                "/p:Configuration=Debug",
                "/p:Platform=Win32",
                "/target:Rebuild",
                "$FileName"

    & $msBuild  "/m",
                "/nologo",
                "/nodeReuse:true",
                "/verbosity:quiet",
                "/p:Configuration=Debug",
                "/p:Platform=x64",
                "/target:Rebuild",
                "$FileName"
    
    # Release builds
    & $msBuild  "/m",
                "/nologo",
                "/nodeReuse:true",
                "/verbosity:quiet",
                "/p:Configuration=Release",
                "/p:Platform=Win32",
                "/target:Rebuild",
                "$FileName"

    & $msBuild  "/m",
                "/nologo",
                "/nodeReuse:true",
                "/verbosity:quiet",
                "/p:Configuration=Release",
                "/p:Platform=x64",
                "/target:Rebuild",
                "$FileName"

    if ($LASTEXITCODE -eq 0)
    {
        Write-Host "`t`t[SUCCESS]" -ForegroundColor Green
    }
    else
    {
        Write-Host "`t`t[ERROR]" -ForegroundColor Red
        exit 1
    }
}

function SetupSdkHeaders()
{
    Write-Host "Building SDK headers" -NoNewline -ForegroundColor Cyan

    # Create the SDK directories
    New-Item "sdk\include\" -type directory | Out-Null
    New-Item "sdk\dbg\amd64\" -type directory | Out-Null
    New-Item "sdk\dbg\i386\" -type directory | Out-Null
    New-Item "sdk\lib\amd64\" -type directory | Out-Null
    New-Item "sdk\lib\i386\" -type directory | Out-Null
    New-Item "sdk\samples\SamplePlugin\" -type directory | Out-Null
    New-Item "sdk\samples\SamplePlugin\bin\Release32\" -type directory | Out-Null

    # Copy SDK readme.txt
    Copy-Item "ProcessHacker\sdk\readme.txt"                         "sdk\"
    # Copy symbols
    Copy-Item "bin\Release32\ProcessHacker.pdb"                      "sdk\dbg\i386\"
    Copy-Item "bin\Release64\ProcessHacker.pdb"                      "sdk\dbg\amd64\"
    Copy-Item "KProcessHacker\bin\i386\kprocesshacker.pdb"           "sdk\dbg\i386\"
    Copy-Item "KProcessHacker\bin\amd64\kprocesshacker.pdb"          "sdk\dbg\amd64\"
    # Copy lib objects
    Copy-Item "bin\Release32\ProcessHacker.lib"                      "sdk\lib\i386\"
    Copy-Item "bin\Release64\ProcessHacker.lib"                      "sdk\lib\amd64\"
    # Copy sample plugin files
    Copy-Item "plugins\SamplePlugin\main.c"                          "sdk\samples\SamplePlugin\"
    Copy-Item "plugins\SamplePlugin\SamplePlugin.sln"                "sdk\samples\SamplePlugin\"
    Copy-Item "plugins\SamplePlugin\SamplePlugin.vcxproj"            "sdk\samples\SamplePlugin\"
    Copy-Item "plugins\SamplePlugin\SamplePlugin.vcxproj.filters"    "sdk\samples\SamplePlugin\"
    Copy-Item "plugins\SamplePlugin\bin\Release32\SamplePlugin.dll"  "sdk\samples\SamplePlugin\bin\Release32\"

    # The array of PHNT headers we need to copy.
    $phnt_headers = 
        "ntdbg.h",
        "ntexapi.h",
        "ntgdi.h",
        "ntioapi.h",
        "ntkeapi.h",
        "ntldr.h",
        "ntlpcapi.h",
        "ntmisc.h",
        "ntmmapi.h",
        "ntnls.h",
        "ntobapi.h",
        "ntpebteb.h",
        "ntpfapi.h",
        "ntpnpapi.h",
        "ntpoapi.h",
        "ntpsapi.h",
        "ntregapi.h",
        "ntrtl.h",
        "ntsam.h",
        "ntseapi.h",
        "nttmapi.h",
        "nttp.h",
        "ntwow64.h",
        "ntxcapi.h",
        "ntzwapi.h",
        "phnt.h",
        "phnt_ntdef.h",
        "phnt_windows.h",
        "subprocesstag.h",
        "winsta.h";

    # The array of PHLIB headers we need to copy.
    $phlib_headers = 
        "circbuf.h",
        "circbuf_h.h",
        "cpysave.h",
        "dltmgr.h",
        "dspick.h",
        "emenu.h",
        "fastlock.h",
        "filestream.h",
        "graph.h",
        "guisup.h",
        "hexedit.h",
        "hndlinfo.h",
        "kphapi.h",
        "kphuser.h",
        "lsasup.h",
        "mapimg.h",
        "ph.h",
        "phbase.h",
        "phbasesup.h",
        "phconfig.h",
        "phdata.h",
        "phnative.h",
        "phnativeinl.h",
        "phnet.h",
        "phsup.h",
        "phutil.h",
        "provider.h",
        "queuedlock.h",
        "ref.h",
        "secedit.h",
        "svcsup.h",
        "symprv.h",
        "templ.h",
        "treenew.h",
        "verify.h",
        "workqueue.h";

    foreach ($file in $phnt_headers)
    {
        Copy-Item "phnt\include\$file" "sdk\include\"
    }

    foreach ($file in $phlib_headers)
    {
        Copy-Item "phlib\include\$file" "sdk\include\"
    }

    Write-Host "`t`t[SUCCESS]" -ForegroundColor Green
}

function BuildPublicHeaders()
{
    Write-Host "Building public headers" -NoNewline -ForegroundColor Cyan

    # Set GenerateHeader path
    $genheader = "tools\GenerateHeader\GenerateHeader\bin\Release\GenerateHeader.exe"

    if ($global:debug_enabled)
    {
        # call the executable
        & $genheader "build\sdk\phapppub_options.txt"
    }
    else
    {
        # call the executable (no output)
        & $genheader "build\sdk\phapppub_options.txt" | Out-Null
    }    

    if ($LASTEXITCODE -eq 0)
    {
        Write-Host "`t`t[SUCCESS]" -ForegroundColor Green
    }
    else
    {
        Write-Host "`t`t[ERROR]" -ForegroundColor Red
        exit 1
    }
}

function SetupPublicHeaders()
{
    Write-Host "Setting up public headers" -NoNewline -ForegroundColor Cyan

    Copy-Item "ProcessHacker\sdk\phapppub.h"  "sdk\include\"
    Copy-Item "ProcessHacker\sdk\phdk.h"      "sdk\include\"
    Copy-Item "ProcessHacker\mxml\mxml.h"     "sdk\include\"
    Copy-Item "ProcessHacker\resource.h"      "sdk\include\phappresource.h"
    
    Write-Host "`t[SUCCESS]" -ForegroundColor Green
}

function SetupSdkResourceHeaders()
{
    Write-Host "Setting up resource headers" -NoNewline -ForegroundColor Cyan

    # Load phappresource
    $phappresource = Get-Content "sdk\include\phappresource.h"

    # Replace strings and pipe/save the output
    $phappresource.Replace("#define ID", "#define PHAPP_ID") | Set-Content "sdk\include\phappresource.h"

    Write-Host "`t[SUCCESS]" -ForegroundColor Green
}

function SetupPluginLibFiles()
{
    Write-Host "Setting up plugin lib files" -NoNewline -ForegroundColor Cyan

    Copy-Item "bin\Release32\ProcessHacker.lib" "sdk\lib\i386\"
    Copy-Item "bin\Release64\ProcessHacker.lib" "sdk\lib\amd64\"

    Write-Host "`t[SUCCESS]" -ForegroundColor Green
}

function ShowBuildTime()
{
    $TimeEnd = New-TimeSpan -Start $global:TimeStart -End $(Get-Date)

    Write-Host "";
    Write-Host "Build Time: $($TimeEnd.Minutes) min, $($TimeEnd.Seconds) sec, $($TimeEnd.Milliseconds) ms";
    Write-Host "";
}

function main()
{
    #
    InitializeScriptEnvironment;

    #
    CheckBaseDirectory;

    # Cleanup old builds
    CleanSdk;

    if ($rebuild)
    {
        # Build the main solution
        BuildSolution("ProcessHacker.sln");
    }

    # Setup the SDK files required by the plugins
    SetupSdkHeaders;

    # Build the public headers
    BuildPublicHeaders;

    #
    SetupPublicHeaders;

    #
    SetupSdkResourceHeaders;
    
    # Copy the import library files
    SetupPluginLibFiles;

    #
    ShowBuildTime;
}

# Entry point
main;