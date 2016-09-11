#
# Process Hacker Toolchain - 
#   Nightly release build script
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
Param([bool] $debug);

function InitializeScriptEnvironment()
{
    # Get script start time
    $global:TimeStart = (Get-Date);

    # Stop script execution after any errors.
    $global:ErrorActionPreference = "Stop";

    if (Test-Path Env:\APPVEYOR)
    {
        $global:buildbot = $true;

        # AppVeyor preseves the directory structure during deployment.
        # So, we need to output into the current directory to upload into the correct FTP directory.
        $env:BUILD_OUTPUT_FOLDER = ".";
    }
    else
    {
        $global:buildbot = $false;

        # This directory is excluded by .gitignore and handy for local builds.
        $env:BUILD_OUTPUT_FOLDER = "ClientBin";

        # Set the default branch for the nightly-src.zip.
        # We only set this when doing a local build, the buildbot sets this variable based on our appveyor.yml.
        $env:APPVEYOR_REPO_BRANCH = "master";
    }

    if ($global:buildbot)
    {
        # intentionally empty
        Write-Host;
    }
    else
    {
        # Clear the console (if we're not running on the appveyor buildbot)
        Clear-Host;
    }
}

function EnableDebug()
{
    if ($debug)
    {
        # Enable debug if we have a valid argument.
        $global:debug_enabled = $true;
    }

    if (Test-Path Env:\APPVEYOR_RE_BUILD)
    {
        # Always enable debug when the build was executed by the "RE-BUILD COMMIT" button on the AppVeyor web interface.
        # This way we can get debug output without having to commit changes to our yml on github after a build fails.
        $global:debug_enabled = $true;
    }

    if ($global:debug_enabled)
    {
        Write-Host "[DEBUG MODE]" -ForegroundColor Red
    }
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

function CleanBuild([bool] $final)
{
    if ($final)
    {
        if (Test-Path "bin")
        {
            Remove-Item "bin" -Recurse -Force
        }
    }
    else
    {
        if (Test-Path "bin")
        {
            Remove-Item "bin" -Recurse -Force
        }

        if (Test-Path "sdk")
        {
            Remove-Item "sdk" -Recurse -Force
        }

        if ((!$global:buildbot) -and (Test-Path "${env:BUILD_OUTPUT_FOLDER}"))
        {
            Remove-Item "${env:BUILD_OUTPUT_FOLDER}" -Recurse -Force
        }

        if (Test-Path "ProcessHacker\sdk\phapppub.h")
        {
            Remove-Item "ProcessHacker\sdk\phapppub.h" -Force
        }
    }
}

function BuildSolution([string] $FileName)
{
    # Set msbuild path (TODO: Do we need the 32bit version of MSBuild for x86 builds?)
    $msBuild = "${env:ProgramFiles(x86)}\MSBuild\14.0\bin\amd64\MSBuild.exe"
    # Get base file name
    $baseName = [System.IO.Path]::GetFileNameWithoutExtension($FileName);

    Write-Host "Building $baseName" -NoNewline -ForegroundColor Cyan

    if ($global:debug_enabled)
    {
        $verbose = "normal";
    }
    else
    {
        $verbose = "quiet";
    }

    & $msBuild  "/m",
                "/nologo",
                "/verbosity:$verbose",
                "/p:Configuration=Release",
                "/p:Platform=Win32",
                "/maxcpucount:${env:NUMBER_OF_PROCESSORS}",
                "/target:Rebuild",
                "/nodeReuse:true",
                "$FileName"

    & $msBuild  "/m",
                "/nologo",
                "/verbosity:$verbose",
                "/p:Configuration=Release",
                "/p:Platform=x64",
                "/maxcpucount:${env:NUMBER_OF_PROCESSORS}",
                "/target:Rebuild",
                "/nodeReuse:true",
                "$FileName"

    if ($LASTEXITCODE -eq 0)
    {
        Write-Host "`t`t[SUCCESS]" -ForegroundColor Green
    }
    else
    {
        Write-Host "`t`t[ERROR]" -ForegroundColor Red
    }
}

function CopyTextFiles()
{
    Write-Host "Copying text files" -NoNewline -ForegroundColor Cyan 

    Copy-Item "README.md"     "bin\README.txt"
    Copy-Item "CHANGELOG.txt" "bin\"
    Copy-Item "COPYRIGHT.txt" "bin\"
    Copy-Item "LICENSE.txt"   "bin\"

    Write-Host "`t`t[SUCCESS]" -ForegroundColor Green
}

function CopyKProcessHacker()
{
    Write-Host "Copying signed KPH driver" -NoNewline -ForegroundColor Cyan

    Copy-Item "KProcessHacker\bin-signed\i386\kprocesshacker.sys"  "bin\Release32\"
    Copy-Item "KProcessHacker\bin-signed\amd64\kprocesshacker.sys" "bin\Release64\"

    Write-Host "`t[SUCCESS]" -ForegroundColor Green
}

function SetupSdkHeaders()
{
    Write-Host "Building SDK headers" -NoNewline -ForegroundColor Cyan

    if (Test-Path "sdk")
    {
        # Remove the existing SDK directory
        Remove-Item "sdk" -Recurse -Force
    }

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

function SetupProcessHackerWow64()
{
    Write-Host "Setting up Wow64 files" -NoNewline -ForegroundColor Cyan

    New-Item "bin\Release64\x86" -type directory | Out-Null
    New-Item "bin\Release64\x86\plugins" -type directory | Out-Null

    Copy-Item "bin\Release32\ProcessHacker.exe"        "bin\Release64\x86\"
    Copy-Item "bin\Release32\ProcessHacker.pdb"        "bin\Release64\x86\"
    Copy-Item "bin\Release32\plugins\DotNetTools.dll"  "bin\Release64\x86\plugins\"
    Copy-Item "bin\Release32\plugins\DotNetTools.pdb"  "bin\Release64\x86\plugins\"

    Write-Host "`t`t[SUCCESS]" -ForegroundColor Green
}

function SetupSignatures()
{
    Write-Host "Setting up signature files" -NoNewline -ForegroundColor Cyan

    New-Item "bin\Release32\ProcessHacker.sig" -ItemType file | Out-Null
    New-Item "bin\Release64\ProcessHacker.sig" -ItemType file | Out-Null

    Write-Host "`t[SUCCESS]" -ForegroundColor Green
}

function BuildSetup()
{
    Write-Host "Building nightly-setup.exe" -NoNewline -ForegroundColor Cyan

    if ((!$global:buildbot) -and (!(Test-Path "${env:BUILD_OUTPUT_FOLDER}")))
    {
        New-Item "$env:BUILD_OUTPUT_FOLDER" -type directory  -ErrorAction SilentlyContinue | Out-Null
    }

    # Set innosetup path
    $innobuild = "${env:ProgramFiles(x86)}\Inno Setup 5\ISCC.exe"

    if (!(Test-Path $innobuild))
    {
        Write-Host "`t[SKIPPED] (Inno Setup not installed)" -ForegroundColor Yellow
        return;
    }

    # Execute the Inno setup
    if ($global:debug_enabled)
    {
        & $innobuild "build\installer\Process_Hacker_installer.iss"
    }
    else
    {
        & $innobuild "build\installer\Process_Hacker_installer.iss" | Out-Null
    }

    # Move and rename the setup into the bin directory
    Move-Item "build\installer\processhacker-3.0-setup.exe" "${env:BUILD_OUTPUT_FOLDER}\processhacker-nightly-setup.exe"

    Write-Host "`t[SUCCESS]" -ForegroundColor Green
}

function BuildSdkZip()
{
    Write-Host "Building nightly-sdk.zip" -NoNewline -ForegroundColor Cyan
    
    if ((!$global:buildbot) -and (!(Test-Path "${env:BUILD_OUTPUT_FOLDER}")))
    {
        New-Item "${env:BUILD_OUTPUT_FOLDER}" -type directory -ErrorAction SilentlyContinue | Out-Null
    }

    # Set 7-Zip executable path
    $7zip = "${env:ProgramFiles}\7-Zip\7z.exe"

    if (!(Test-Path $7zip))
    {
        Write-Host "`t[SKIPPED] (7-Zip not installed)" -ForegroundColor Yellow
        return;
    }

    # Execute the Inno setup
    if ($global:debug_enabled)
    {
        & $7zip "a",
                "-tzip",
                "-mx9",
                "${env:BUILD_OUTPUT_FOLDER}\processhacker-nightly-sdk.zip",
                ".\sdk\*",
                "-r"
    }
    else
    {
        & $7zip "a",
                "-tzip",
                "-mx9",
                "${env:BUILD_OUTPUT_FOLDER}\processhacker-nightly-sdk.zip",
                ".\sdk\*",
                "-r" | Out-Null
    }

    Write-Host "`t[SUCCESS]" -ForegroundColor Green
}

function BuildZip()
{
    Write-Host "Building nightly-bin.zip" -NoNewline -ForegroundColor Cyan
    
    if ((!$global:buildbot) -and (!(Test-Path "${env:BUILD_OUTPUT_FOLDER}")))
    {
        New-Item "${env:BUILD_OUTPUT_FOLDER}" -type directory -ErrorAction SilentlyContinue | Out-Null
    }

    # Remove junk files
    Remove-Item "bin\*" -recurse -include "*.iobj", "*.ipdb", "*.exp", "*.lib"

    # Rename the directories
    Rename-Item "bin\Release32" "x86"
    Rename-Item "bin\Release64" "x64"

    # Set 7-Zip path
    $7zip = "${env:ProgramFiles}\7-Zip\7z.exe"
    
    if (!(Test-Path $7zip))
    {
        Write-Host "`t[SKIPPED] (7-Zip not installed)" -ForegroundColor Yellow
        return;
    }

    # Execute 7-Zip
    if ($global:debug_enabled)
    {
        & "$7zip" "a",
                  "-tzip",
                  "-mx9",
                  "${env:BUILD_OUTPUT_FOLDER}\processhacker-nightly-bin.zip",
                  ".\bin\*",
                  "-r"
    }
    else
    {
        & "$7zip" "a", 
                  "-tzip",
                  "-mx9",
                  "${env:BUILD_OUTPUT_FOLDER}\processhacker-nightly-bin.zip", 
                  ".\bin\*", 
                  "-r" | Out-Null
    }

    Write-Host "`t[SUCCESS]" -ForegroundColor Green
}

function BuildSourceZip()
{
    Write-Host "Building nightly-src.zip" -NoNewline -ForegroundColor Cyan
    
    if ((!$global:buildbot) -and (!(Test-Path "${env:BUILD_OUTPUT_FOLDER}")))
    {
        New-Item "${env:BUILD_OUTPUT_FOLDER}" -type directory -ErrorAction SilentlyContinue | Out-Null
    }
    
    # Set git executable path
    $git = "${env:ProgramFiles}\Git\cmd\git.exe"
    
    if (!(Test-Path $git))
    {
        Write-Host "`t[SKIPPED] (Git not installed)" -ForegroundColor Yellow
        return;
    }

    & $git "--git-dir=.git", 
           "--work-tree=.\", 
           "archive",
           "--format", 
           "zip", 
           "--output", 
           "${env:BUILD_OUTPUT_FOLDER}\processhacker-nightly-src.zip", 
           "${env:APPVEYOR_REPO_BRANCH}"

    Write-Host "`t[SUCCESS]" -ForegroundColor Green
}

function GenerateHashes()
{
    Write-Host "Generating checksums" -NoNewline -ForegroundColor Cyan

    # Query the full path from the relative path
    $file_path = Resolve-Path "${env:BUILD_OUTPUT_FOLDER}\";

    # Create Arraylist
    $array = $();

    $file_names = 
        "processhacker-nightly-setup.exe",
        "processhacker-nightly-sdk.zip",
        "processhacker-nightly-bin.zip",
        "processhacker-nightly-src.zip";

    foreach ($file in $file_names)
    {
        if (!(Test-Path "${env:BUILD_OUTPUT_FOLDER}\$file"))
        {
            if ($global:debug_enabled)
            {
                Write-Host "`t(SKIPPED: $file)" -NoNewline -ForegroundColor Yellow
            }
            continue;
        }

        Get-FileHash "${env:BUILD_OUTPUT_FOLDER}\$file" -Algorithm SHA256 -OutVariable +array | Out-Null
    }

    # Convert the Arraylist to a string
    $data = $array | Out-String
    # Cleanup the string
    $data = $data.Remove(0, 2).Replace($file_path, "").Replace("Path", "File").Replace("\\", "\").Replace("\", "");   
    # Save the string
    $data | Out-File "${env:BUILD_OUTPUT_FOLDER}\processhacker-nightly-checksums.txt"

    if ($global:buildbot)
    {
        # TODO: This code is disabled while the website API is worked out.
        # Convert the Arraylist to a json string
        #$json = $array | ConvertTo-Json | Out-String
        # Cleanup the string
        #$json = $json.Replace("\\", "\").Replace($file_path, "").Replace("Path", "File");
        # Send the json to our internal API and update the website hashes
        #Invoke-RestMethod -Method Post -Uri "xyz.php" -Body $json -Header @{"X-ApiKey"="123ABC"} -ErrorAction SilentlyContinue
    }

    Write-Host "`t`t[SUCCESS]" -ForegroundColor Green
}

function ShowBuildTime()
{
    $TimeEnd = New-TimeSpan -Start $global:TimeStart -End $(Get-Date)

    Write-Host "";
    Write-Host "Elapsed Time: $($TimeEnd.Minutes) minute(s), $($TimeEnd.Seconds) second(s)";
    Write-Host "";
}

function main()
{
    InitializeScriptEnvironment;

    EnableDebug;

    # Check if the current directory contains the main solution
    CheckBaseDirectory;

    # Cleanup any previous builds
    CleanBuild($false);

    # Build the main solution
    BuildSolution("ProcessHacker.sln");

    # Copy the text files
    CopyTextFiles;

    # Copy the signed KPH drivers
    CopyKProcessHacker;

    # Copy the SDK headers for the plugins
    SetupSdkHeaders;

    # Build the public headers
    BuildPublicHeaders;

    # 
    SetupPublicHeaders;

    # 
    SetupSdkResourceHeaders;

    # Copy the import library files
    SetupPluginLibFiles;

    # Build the plugins
    BuildSolution("plugins\Plugins.sln");

    # 
    SetupProcessHackerWow64;

    # 
    SetupSignatures;

    #
    BuildSetup;

    #
    BuildSdkZip;

    #
    BuildZip;

    #
    BuildSourceZip;
    
    #
    GenerateHashes;

    # Preform final clean-up
    CleanBuild($true);

    #
    ShowBuildTime;

    # Sleep for 10 seconds before exiting (if we're not running on the appveyor buildbot)
    if (!$global:buildbot)
    {
        Start-Sleep 10;
    }
}

# Entry point
main;