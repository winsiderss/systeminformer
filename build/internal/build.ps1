# Process Hacker Toolchain - 
#   Build script
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

    # This directory is excluded by .gitignore and handy for local builds.
    $env:BUILD_OUTPUT_FOLDER = "ClientBin";

    if (Test-Path Env:\APPVEYOR)
    {
        $global:buildbot = $true;

        # AppVeyor preseves the directory structure during deployment.
        # So, we need to output into the current directory to upload into the correct FTP directory.
        #$env:BUILD_OUTPUT_FOLDER = ".";
    }
    else
    {
        #$global:buildbot = $false;

        # Set the default branch for the build-src.zip.
        # We only set this when doing a local build, the buildbot sets this variable based on our appveyor.yml.
        $env:APPVEYOR_REPO_BRANCH = "master";
    }

    if ($global:buildbot)
    {
        # intentionally empty
        Write-Host
    }
    else
    {
        # Clear the console (if we're not running on the appveyor buildbot)
        #Clear-Host;
    }

    if ($debug)
    {
        # Enable debug if we have a valid argument.
        $global:debug_enabled = $true;
    }

    if (Test-Path Env:\APPVEYOR_RE_BUILD)
    {
        # Always enable debug when the build was executed by the "RE-BUILD COMMIT" button on the AppVeyor web interface.
        # This way we can get debug output without having to commit changes to our yml on github after a build fails.
        #$global:debug_enabled = $true;
    }

    if ($global:debug_enabled)
    {
        Write-Host "[DEBUG MODE]" -ForegroundColor Red
    }
}

function InitializeBuildEnvironment()
{
    $git = "${env:ProgramFiles}\Git\cmd\git.exe"

    if (Test-Path $git)
    {
        Write-Host "Getting build information...  " -NoNewline -ForegroundColor Cyan

        $global:latestGitMessage =  (& "$git" "log", "-n", "5", "--pretty=%B") | Out-String
        $global:latestGitTag =      (& "$git" "describe", "--abbrev=0", "--tags", "--always") | Out-String
        #$global:latestGitCount =   (& "$git" "rev-list", "--count", "master") | Out-String
        $global:latestGitRevision = (& "$git" "rev-list", "--count", ($global:latestGitTag.Trim() + "..master")) | Out-String
        
        $global:buildMessage = $global:latestGitMessage -Replace "`r`n`r`n", "`r`n"
        $global:buildMessage = ($global:buildMessage -split '\r\n')[0]
        $global:fileVersion = "3.0." + $global:latestGitRevision.Trim() #${env:APPVEYOR_BUILD_VERSION}

        Write-Host "$global:fileVersion ($global:buildMessage)" -ForegroundColor White
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

    if (($global:buildbot) -and (Test-Path Env:\VIRUSTOTAL_BUILD_KEY))
    {
       $msbuild_output += & "$msBuild"  "/m",
                    "/nologo",
                    "/verbosity:$verbose",
                    "/p:Configuration=Release",
                    "/p:Platform=Win32",
                    "/p:ExternalCompilerOptions=VIRUSTOTAL_API",
                    "/t:Rebuild",        
                    "/nodeReuse:true",
                    "$FileName";

       $msbuild_output += & "$msBuild"  "/m",
                    "/nologo",
                    "/verbosity:$verbose",
                    "/p:Configuration=Release",
                    "/p:Platform=x64",
                    "/p:ExternalCompilerOptions=VIRUSTOTAL_API",
                    "/t:Rebuild",
                    "/nodeReuse:true",
                    "$FileName";
    }
    else
    {
        if ($global:debug_enabled)
        {
           $msbuild_output += & "$msBuild"  "/m",
                        "/nologo",
                        "/verbosity:$verbose",
                        "/p:Configuration=Release",
                        "/p:Platform=Win32",
                        "/t:Rebuild",        
                        "/nodeReuse:true",
                        "$FileName";

           $msbuild_output += & $msBuild  "/m",
                        "/nologo",
                        "/verbosity:$verbose",
                        "/p:Configuration=Release",
                        "/p:Platform=x64",
                        "/t:Rebuild",
                        "/nodeReuse:true",
                        "$FileName";
        }
        else
        {
            $msbuild_output += & "$msBuild"  "/m",
                        "/nologo",
                        "/verbosity:$verbose",
                        "/p:Configuration=Release",
                        "/p:Platform=Win32",
                        "/t:Rebuild",        
                        "/nodeReuse:true",
                        "$FileName" | Out-String;

           $msbuild_output += & "$msBuild"  "/m",
                        "/nologo",
                        "/verbosity:$verbose",
                        "/p:Configuration=Release",
                        "/p:Platform=x64",
                        "/t:Rebuild",
                        "/nodeReuse:true",
                        "$FileName" | Out-String;
        }

    }

    if ($LASTEXITCODE -eq 0)
    {
        Write-Host "        [SUCCESS]" -ForegroundColor Green
    }
    else
    {
        Write-Host "        [ERROR] $msbuild_output" -ForegroundColor Red
        exit 2
    }
}

function CopyTextFiles()
{
    Write-Host "Copying text files" -NoNewline -ForegroundColor Cyan 

    Copy-Item "README.md"     "bin\README.txt"
    Copy-Item "CHANGELOG.txt" "bin\"
    Copy-Item "COPYRIGHT.txt" "bin\"
    Copy-Item "LICENSE.txt"   "bin\"

    Write-Host "            [SUCCESS]" -ForegroundColor Green
}

function CopyKProcessHacker()
{
    Write-Host "Copying signed KPH driver" -NoNewline -ForegroundColor Cyan

    Copy-Item "KProcessHacker\bin-signed\i386\kprocesshacker.sys"  "bin\Release32\"
    Copy-Item "KProcessHacker\bin-signed\amd64\kprocesshacker.sys" "bin\Release64\"
    
    New-Item "bin\Release32\ProcessHacker.sig" -ItemType File -ErrorAction SilentlyContinue | Out-Null
    New-Item "bin\Release64\ProcessHacker.sig" -ItemType File -ErrorAction SilentlyContinue | Out-Null

    Write-Host "     [SUCCESS]" -ForegroundColor Green
}

function SetupSdkHeaders()
{
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

    foreach ($file in $phnt_headers)
    {
        Copy-Item "phnt\include\$file" "sdk\include\"
    }

    foreach ($file in $phlib_headers)
    {
        Copy-Item "phlib\include\$file" "sdk\include\"
    }

    Write-Host "          [SUCCESS]" -ForegroundColor Green
}

function SetupPublicHeaders()
{
    # Set GenerateHeader path
    $genheader = "tools\GenerateHeader\GenerateHeader\bin\Release\GenerateHeader.exe"

    Write-Host "Setting up public headers" -NoNewline -ForegroundColor Cyan

    if ($global:debug_enabled)
    {
        # Call the executable
        & "$genheader" "build\sdk\phapppub_options.txt"
    }
    else
    {
        # Call the executable (no output)
        & "$genheader" "build\sdk\phapppub_options.txt" | Out-Null
    }

    Copy-Item "ProcessHacker\sdk\phapppub.h"  "sdk\include\"
    Copy-Item "ProcessHacker\sdk\phdk.h"      "sdk\include\"
    Copy-Item "ProcessHacker\mxml\mxml.h"     "sdk\include\"
    Copy-Item "ProcessHacker\resource.h"      "sdk\include\phappresource.h"
    
    Write-Host "     [SUCCESS]" -ForegroundColor Green
}

function SetupSdkResourceHeaders()
{
    Write-Host "Setting up resource headers" -NoNewline -ForegroundColor Cyan

    # Load phappresource
    $phappresource = Get-Content "sdk\include\phappresource.h"

    # Replace strings and pipe/save the output
    $phappresource.Replace("#define ID", "#define PHAPP_ID") | Set-Content "sdk\include\phappresource.h"

    Write-Host "   [SUCCESS]" -ForegroundColor Green
}

function SetupPluginLibFiles()
{
    Write-Host "Setting up plugin lib files" -NoNewline -ForegroundColor Cyan

    Copy-Item "bin\Release32\ProcessHacker.lib" "sdk\lib\i386\"
    Copy-Item "bin\Release64\ProcessHacker.lib" "sdk\lib\amd64\"

    Write-Host "   [SUCCESS]" -ForegroundColor Green
}

function SetupProcessHackerWow64()
{
    Write-Host "Setting up Wow64 files" -NoNewline -ForegroundColor Cyan

    New-Item "bin\Release64\x86" -type Directory -ErrorAction SilentlyContinue | Out-Null
    New-Item "bin\Release64\x86\plugins" -type Directory -ErrorAction SilentlyContinue | Out-Null

    Copy-Item "bin\Release32\ProcessHacker.exe"        "bin\Release64\x86\"
    Copy-Item "bin\Release32\ProcessHacker.pdb"        "bin\Release64\x86\"
    Copy-Item "bin\Release32\plugins\DotNetTools.dll"  "bin\Release64\x86\plugins\"
    Copy-Item "bin\Release32\plugins\DotNetTools.pdb"  "bin\Release64\x86\plugins\"

    Write-Host "       [SUCCESS]" -ForegroundColor Green
}

function BuildSetupExe()
{  
    $innoBuild = "${env:ProgramFiles(x86)}\Inno Setup 5\ISCC.exe"
    $setupPath = "${env:BUILD_OUTPUT_FOLDER}\processhacker-build-setup.exe"

    Write-Host "Building build-setup.exe" -NoNewline -ForegroundColor Cyan

    if (!(Test-Path $innoBuild))
    {
        Write-Host "`t[SKIPPED] (Inno Setup not installed)" -ForegroundColor Yellow
        return
    }

    if ((!$global:buildbot) -and (!(Test-Path Env:\BUILD_OUTPUT_FOLDER)))
    {
        New-Item "$env:BUILD_OUTPUT_FOLDER" -type Directory -ErrorAction SilentlyContinue | Out-Null
    }

    if ($global:debug_enabled)
    {
        # Call the executable
        & "$innoBuild" "build\installer\Process_Hacker_installer.iss"
    }
    else
    {
        # Call the executable (no output)
        & "$innoBuild" "build\installer\Process_Hacker_installer.iss" | Out-Null
    }

    Remove-Item "$setupPath" -ErrorAction SilentlyContinue

    Move-Item "build\installer\processhacker-3.0-setup.exe" "$setupPath" -ErrorAction SilentlyContinue

    Write-Host "    [SUCCESS]" -ForegroundColor Green
}

function BuildSdkZip()
{
    $7zip =     "${env:ProgramFiles}\7-Zip\7z.exe"
    $zip_path = "${env:BUILD_OUTPUT_FOLDER}\processhacker-build-sdk.zip";

    Write-Host "Building build-sdk.zip" -NoNewline -ForegroundColor Cyan

    if (!(Test-Path "$7zip"))
    {
        Write-Host "`t[SKIPPED] (7-Zip not installed)" -ForegroundColor Yellow
        return
    }

    if ((!$global:buildbot) -and (!(Test-Path Env:\BUILD_OUTPUT_FOLDER)))
    {
        New-Item "${env:BUILD_OUTPUT_FOLDER}" -type Directory -ErrorAction SilentlyContinue | Out-Null
    }

    if ($global:debug_enabled)
    {
        # Call the executable
        & "$7zip" "a",
                "-tzip",
                "-mx9",
                "$zip_path",
                ".\sdk\*",
                "-r"
    }
    else
    {
        # Call the executable (no output)
        & "$7zip" "a",
                "-tzip",
                "-mx9",
                "$zip_path",
                ".\sdk\*",
                "-r" | Out-Null
    }

    Write-Host "      [SUCCESS]" -ForegroundColor Green
}

function BuildBinZip()
{
    $7zip =     "${env:ProgramFiles}\7-Zip\7z.exe";
    $zip_path = "${env:BUILD_OUTPUT_FOLDER}\processhacker-build-bin.zip";

    Write-Host "Building build-bin.zip" -NoNewline -ForegroundColor Cyan
    
    if (!(Test-Path "$7zip"))
    {
        Write-Host "`t[SKIPPED] (7-Zip not installed)" -ForegroundColor Yellow
        return
    }

    if (Test-Path "$zip_path")
    {
        Remove-Item $zip_path -Force -ErrorAction SilentlyContinue
    }

    if ((!$global:buildbot) -and (!(Test-Path Env:\BUILD_OUTPUT_FOLDER)))
    {
        New-Item "${env:BUILD_OUTPUT_FOLDER}" -type Directory -ErrorAction SilentlyContinue | Out-Null
    }

    if ($global:debug_enabled)
    {
        # Call the executable
        & "$7zip" "a",
                  "-tzip",
                  "-mx9",
                  "$zip_path",
                  ".\bin\*",
                  "-r",
                  "-xr!Debug32", # Ignore junk directories
                  "-xr!Debug64",
                  "-x!*.pdb", # Ignore junk files
                  "-x!*.iobj",
                  "-x!*.ipdb",
                  "-x!*.exp",
                  "-x!*.lib"
    }
    else
    {
        # Call the executable (no output)
        & "$7zip" "a", 
                  "-tzip",
                  "-mx9",
                  "$zip_path", 
                  ".\bin\*", 
                  "-r",
                  "-xr!Debug32", # Ignore junk directories
                  "-xr!Debug64",
                  "-x!*.pdb", # Ignore junk files
                  "-x!*.iobj",
                  "-x!*.ipdb",
                  "-x!*.exp",
                  "-x!*.lib" | Out-Null
    }

    Write-Host "      [SUCCESS]" -ForegroundColor Green
}

function BuildSourceZip()
{
    $git = "${env:ProgramFiles}\Git\cmd\git.exe"

    Write-Host "Building build-src.zip" -NoNewline -ForegroundColor Cyan

    if (!(Test-Path "$git"))
    {
        Write-Host "`t[SKIPPED] (Git not installed)" -ForegroundColor Yellow
        return
    }

    if ((!$global:buildbot) -and (!(Test-Path Env:\BUILD_OUTPUT_FOLDER)))
    {
        New-Item "${env:BUILD_OUTPUT_FOLDER}" -type Directory -ErrorAction SilentlyContinue | Out-Null
    }

    # Call the executable
    & "$git" "--git-dir=.git", 
           "--work-tree=.\", 
           "archive",
           "--format", 
           "zip", 
           "--output", 
           "${env:BUILD_OUTPUT_FOLDER}\processhacker-build-src.zip", 
           "${env:APPVEYOR_REPO_BRANCH}"

    Write-Host "      [SUCCESS]" -ForegroundColor Green
}

function BuildPdbZip()
{
    $7zip =     "${env:ProgramFiles}\7-Zip\7z.exe";
    $zip_path = "${env:BUILD_OUTPUT_FOLDER}\processhacker-build-pdb.zip";

    Write-Host "Building build-pdb.zip" -NoNewline -ForegroundColor Cyan
    
    if (!(Test-Path "$7zip"))
    {
        Write-Host "`t[SKIPPED] (7-Zip not installed)" -ForegroundColor Yellow
        return
    }

    if (Test-Path "$zip_path")
    {
        Remove-Item $zip_path -Force -ErrorAction SilentlyContinue
    }

    if ((!$global:buildbot) -and (!(Test-Path Env:\BUILD_OUTPUT_FOLDER)))
    {
        New-Item "${env:BUILD_OUTPUT_FOLDER}" -type Directory -ErrorAction SilentlyContinue | Out-Null
    }

    if ($global:debug_enabled)
    {
        & "$7zip" "a",
                  "-tzip",
                  "-mx9",
                  "$zip_path",
                  "-r",
                  "-xr!Debug32", # Ignore junk directories
                  "-xr!Debug64",
                  "-xr!Obj",
                  "-ir!*.pdb" # Include only PDB files
    }
    else
    {
        & "$7zip" "a",
                  "-tzip",
                  "-mx9",
                  "$zip_path",
                  "-r",
                  "-xr!Debug32", # Ignore junk directories
                  "-xr!Debug64",
                  "-xr!Obj",
                  "-ir!*.pdb" | Out-Null # Include only PDB files
    }

    Write-Host "      [SUCCESS]" -ForegroundColor Green
}

function BuildChecksumsFile()
{
    $fileHashes = "`r`n";
    $file_names = 
        "processhacker-build-setup.exe",
        "processhacker-build-sdk.zip",
        "processhacker-build-bin.zip",
        "processhacker-build-src.zip",
        "processhacker-build-pdb.zip";  

    Write-Host "Generating checksums" -NoNewline -ForegroundColor Cyan

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

        $fileHashes += $file + ": (SHA256)`r`n" + (Get-FileHash "${env:BUILD_OUTPUT_FOLDER}\$file" -Algorithm SHA256).Hash + "`r`n`r`n";
    }

    if (Test-Path "${env:BUILD_OUTPUT_FOLDER}\processhacker-build-checksums.txt")
    {
        Remove-Item "${env:BUILD_OUTPUT_FOLDER}\processhacker-build-checksums.txt" -Force -ErrorAction SilentlyContinue
    }

    # Convert the Arraylist to a string and save the string
    $fileHashes | Out-String | Out-File "${env:BUILD_OUTPUT_FOLDER}\processhacker-build-checksums.txt"

    Write-Host "        [SUCCESS]" -ForegroundColor Green
}

function SetupSignatureFiles()
{
    $secure_file = "tools\SecureBuildTool\secure-file.exe";
    
    Write-Host "Setting up signature files" -NoNewline -ForegroundColor Cyan
    
    if ((!$global:buildbot) -and !(Test-Path Env:\VIRUSTOTAL_BUILD_KEY))
    {
        Write-Host " [SKIPPED] (VirusTotal build key)" -ForegroundColor Yellow
        return
    }
    
    if (!(Test-Path "$secure_file"))
    {
        Write-Host " [SKIPPED] (secure-file not installed)" -ForegroundColor Yellow
        return
    }

    if ($global:debug_enabled)
    {
        & "$secure_file" "-decrypt",
                "plugins\OnlineChecks\virustotal.s",
                "-secret",
                "${env:VIRUSTOTAL_BUILD_KEY}",
                "-out",
                "plugins\OnlineChecks\virustotal.h"
    }
    else
    {
        & "$secure_file" "-decrypt",
                "plugins\OnlineChecks\virustotal.s",
                "-secret",
                "${env:VIRUSTOTAL_BUILD_KEY}",
                "-out",
                "plugins\OnlineChecks\virustotal.h"
    }

    if (!($LASTEXITCODE -eq 0))
    {
        Write-Host "     [ERROR] (virustotal)" -ForegroundColor Red
        exit 5
    }

    if ($global:debug_enabled)
    {
        & "$secure_file" "-decrypt",
                "build\internal\private.s",
                "-secret",
                "${env:VIRUSTOTAL_BUILD_KEY}",
                "-out",
                "build\internal\private.key"
    }
    else
    {
        & "$secure_file" "-decrypt",
                "build\internal\private.s",
                "-secret",
                "${env:VIRUSTOTAL_BUILD_KEY}",
                "-out",
                "build\internal\private.key"
    }

    if ($LASTEXITCODE -eq 0)
    {
        Write-Host "     [SUCCESS]" -ForegroundColor Green
    }
    else
    {
        Write-Host "     [ERROR] (private)" -ForegroundColor Red
        exit 5
    }
}

function BuildSignatureFiles()
{
    $sign_file = "tools\CustomSignTool\bin\Release32\CustomSignTool.exe";
    $rootPath = (Get-Location).Path;

    Write-Host "Update build signatures" -NoNewline -ForegroundColor Cyan
    
    if ((!$global:buildbot) -and !(Test-Path Env:\VIRUSTOTAL_BUILD_KEY))
    {
        Write-Host " [SKIPPED] (VirusTotal build key)" -ForegroundColor Yellow
        return
    }

    $global:signature_output = ( & "$sign_file" "sign", 
        "-k", 
        "$rootPath\build\internal\private.key", 
        "$rootPath\ClientBin\processhacker-build-setup.exe", 
        "-h" | Out-String) -replace "`n|`r"

    if ($LASTEXITCODE -eq 0)
    {
        Write-Host "     [SUCCESS] ($global:signature_output)" -ForegroundColor Green
    }
    else
    {
        Write-Host "     [ERROR] ($global:signature_output)" -ForegroundColor Red
        exit 4
    }
}

function UpdateBuildService()
{
    $git = "${env:ProgramFiles}\Git\cmd\git.exe"
    $exeSetup = "${env:BUILD_OUTPUT_FOLDER}\processhacker-build-setup.exe"
    $sdkZip =   "${env:BUILD_OUTPUT_FOLDER}\processhacker-build-sdk.zip"
    $binZip =   "${env:BUILD_OUTPUT_FOLDER}\processhacker-build-bin.zip"
    $srcZip =   "${env:BUILD_OUTPUT_FOLDER}\processhacker-build-src.zip" 
    $pdbZip =   "${env:BUILD_OUTPUT_FOLDER}\processhacker-build-pdb.zip" 
    $checksums ="${env:BUILD_OUTPUT_FOLDER}\processhacker-build-checksums.txt"

    Write-Host "Updating build service" -NoNewline -ForegroundColor Cyan

    if ((!(Test-Path Env:\APPVEYOR_BUILD_API)) -and (!(Test-Path Env:\APPVEYOR_BUILD_KEY)))
    {
        Write-Host "      [SKIPPED] (Build service key)" -ForegroundColor Yellow
        return
    }

    if (Test-Path $binZip)
    {
        $fileInfo = (Get-Item "$binZip");
        $fileTime = $fileInfo.CreationTime.ToString("yyyy-MM-ddTHH:mm:sszzz");
        $fileSize = $fileInfo.Length;
    }

    if ($array) 
    {
        $exeHash = $array[0].Hash;
        $sdkHash = $array[1].Hash;
        $binHash = $array[2].Hash;
        $srcHash = $array[3].Hash;
        $pdbHash = $array[4].Hash;
    }
    else
    {
        if (Test-Path $exeSetup)
        {
            $exeHash = (Get-FileHash "$exeSetup" -Algorithm SHA256).Hash;
        }

        if (Test-Path "$sdkZip")
        {
            $sdkHash = (Get-FileHash "$sdkZip" -Algorithm SHA256).Hash;
        }

        if (Test-Path "$binZip")
        {
            $binHash = (Get-FileHash "$binZip" -Algorithm SHA256).Hash;
        }
        
        if (Test-Path "$srcZip")
        {
            $srcHash = (Get-FileHash "$srcZip" -Algorithm SHA256).Hash;
        }

        if (Test-Path "$pdbZip")
        {
            $pdbHash = (Get-FileHash "$pdbZip" -Algorithm SHA256).Hash;
        }
    }

    if ($global:signature_output -and $global:buildMessage -and $exeHash -and $sdkHash -and $binHash -and $srcHash -and $pdbHash -and $fileTime -and $fileSize -and $global:fileVersion)
    {
        $jsonString = @{
            "version"="$global:fileVersion"
            "size"="$fileSize"
            "updated"="$fileTime"
            "forum_url"="https://wj32.org/processhacker/forums/viewtopic.php?t=2315"
            "bin_url"="https://ci.appveyor.com/api/projects/processhacker/processhacker2/artifacts/processhacker-$global:fileVersion-bin.zip"
            "setup_url"="https://ci.appveyor.com/api/projects/processhacker/processhacker2/artifacts/processhacker-$global:fileVersion-setup.exe"
            "hash_setup"="$exeHash"
            "hash_sdk"="$sdkHash"
            "hash_bin"="$binHash"
            "hash_src"="$srcHash"
            "hash_pdb"="$pdbHash"
            "message"="$global:buildMessage"
            "sig"="$global:signature_output"
        } | ConvertTo-Json | Out-String;

        Rename-Item "$exeSetup"  "processhacker-$global:fileVersion-setup.exe" -Force
        Rename-Item "$sdkZip"    "processhacker-$global:fileVersion-sdk.zip" -Force
        Rename-Item "$binZip"    "processhacker-$global:fileVersion-bin.zip" -Force
        Rename-Item "$srcZip"    "processhacker-$global:fileVersion-src.zip" -Force
        Rename-Item "$pdbZip"    "processhacker-$global:fileVersion-pdb.zip" -Force
        Rename-Item "$checksums" "processhacker-$global:fileVersion-checksums.txt" -Force

        if (($global:buildbot) -and (Test-Path Env:\APPVEYOR_BUILD_API))
        {
            Push-AppveyorArtifact "${env:BUILD_OUTPUT_FOLDER}\processhacker-$global:fileVersion-setup.exe" -ErrorAction SilentlyContinue
            Push-AppveyorArtifact "${env:BUILD_OUTPUT_FOLDER}\processhacker-$global:fileVersion-sdk.zip" -ErrorAction SilentlyContinue
            Push-AppveyorArtifact "${env:BUILD_OUTPUT_FOLDER}\processhacker-$global:fileVersion-bin.zip" -ErrorAction SilentlyContinue
            Push-AppveyorArtifact "${env:BUILD_OUTPUT_FOLDER}\processhacker-$global:fileVersion-src.zip" -ErrorAction SilentlyContinue
            Push-AppveyorArtifact "${env:BUILD_OUTPUT_FOLDER}\processhacker-$global:fileVersion-pdb.zip" -ErrorAction SilentlyContinue
            Push-AppveyorArtifact "${env:BUILD_OUTPUT_FOLDER}\processhacker-$global:fileVersion-checksums.txt" -ErrorAction SilentlyContinue
        }

        if ((Test-Path Env:\APPVEYOR_BUILD_API) -and (Test-Path Env:\APPVEYOR_BUILD_KEY))
        {
            # Build the http headers
            $jsonHeaders = @{"X-ApiKey"="${env:APPVEYOR_BUILD_KEY}"};

            # Update the build service
            Invoke-RestMethod -Method Post -Uri ${env:APPVEYOR_BUILD_API} -Header $jsonHeaders -Body $jsonString -ErrorAction SilentlyContinue
        }
    }
    else
    {
        Write-Host "  [FAILED]" -ForegroundColor Red
    }
}

function ShowBuildTime()
{
    $timeEnd = New-TimeSpan -Start $global:TimeStart -End $(Get-Date)

    Write-Host "";
    Write-Host "Build Time: $($timeEnd.Minutes) minute(s), $($timeEnd.Seconds) second(s)";
}

# Setup the build script environment
InitializeScriptEnvironment;

# Setup the build version
InitializeBuildEnvironment;

# Decrypt build files
SetupSignatureFiles;

# Check if the current directory contains the main solution
CheckBaseDirectory;

# Build the main solution
BuildSolution("ProcessHacker.sln");

# Copy the text files
CopyTextFiles;
CopyKProcessHacker;

# Copy the sdk files
SetupSdkHeaders;
SetupPublicHeaders;
SetupSdkResourceHeaders;
SetupPluginLibFiles;

# Build the plugins
BuildSolution("plugins\Plugins.sln");

# Setup the x86 plugin files
SetupProcessHackerWow64;

# Build the release files
BuildSetupExe;
BuildSdkZip;
BuildBinZip;
BuildSourceZip;
BuildPdbZip;

# Build the checksums
BuildChecksumsFile;

# Create signature files
BuildSignatureFiles;

# Update the build service
UpdateBuildService;

# Show the total build time
ShowBuildTime;