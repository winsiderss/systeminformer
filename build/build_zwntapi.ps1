
function InitializeScriptEnvironment()
{
    # Get script start time
    $global:TimeStart = (Get-Date);
    # Stop script execution after any errors.
    $global:ErrorActionPreference = "Stop";
}

function CheckSolutionDirectory()
{
    # Check if the current directory contains the main solution
    if (!(Test-Path "SystemInformer.sln"))
    {
        # Change root directory to the \build\internal\ directory (where this script is located).
        Set-Location $PSScriptRoot;

        # Set the current location to the base repository directory.
        Set-Location "..\";

        # Re-check if the current directory
        if (!(Test-Path "SystemInformer.sln"))
        {
            Write-Host "Unable to find project directory... Exiting." -ForegroundColor Red
            exit 5
        }
    }
}

function GenerateZw()
{
    $BaseDirectory = "phnt\include"
    $OutputFile = "ntzwapi.h"
    $Header = "#ifndef _NTZWAPI_H`r`n#define _NTZWAPI_H`r`n`r`n// This file was automatically generated. Do not edit.`r`n`r`n"
    $Footer = "#endif`r`n"

    $directoryInfo = Get-Item -Path ".."
    while ($null -ne $directoryInfo.Parent -and $null -ne $directoryInfo.Parent.Parent) {
        $directoryInfo = $directoryInfo.Parent
        if (Test-Path -Path "$($directoryInfo.FullName)\$BaseDirectory") {
            Set-Location -Path $directoryInfo.FullName
            break
        }
    }

    $regex = New-Object System.Text.RegularExpressions.RegEx("NTSYSCALLAPI[\w\s_]*NTAPI\s*(Nt(\w)*)\(.*?\);", [System.Text.RegularExpressions.RegexOptions]::Singleline)
    $definitions = @()
    
    $files = Get-ChildItem -Path $BaseDirectory -Filter "*.h" -Exclude "ntuser.h" -Recurse
    foreach ($file in $files) {
        $text = Get-Content -Path $file.FullName -Raw

        if ([String]::IsNullOrEmpty($text)) {
            continue
        }

        $textmatches = $regex.Matches($text)
    
        foreach ($match in $textmatches)
        {
            $definitions += [PSCustomObject]@{
                Name = $match.Groups[1].Value
                Text = $match.Value
                NameIndex = $match.Groups[1].Index - $match.Index
            }
        }
    }
    
    $defs = $definitions | Sort-Object -Property Name -CaseSensitive -Unique

    $fileName = New-Item -Name ($BaseDirectory + '\\' + $OutputFile) -ItemType File -Force
    $fileStream = $fileName.OpenWrite()
    $sw = [System.IO.StreamWriter]::new($fileStream)
    $sw.Write($Header)

    Write-Host "Updating $($fileName)"

    foreach ($d in $defs) 
    {
        Write-Host "System service: $($d.Name)"

        $sw.Write($d.Text.Substring(0, $d.NameIndex) + "Zw" + $d.Text.Substring($d.NameIndex + 2) + "`r`n`r`n")
    }

    $sw.Write($Footer)
    $sw.Dispose()
    $fileStream.Dispose();
}

# Entry point
InitializeScriptEnvironment;

# Check the current directory
CheckSolutionDirectory;

# Update the ntzwapi.h exports
GenerateZw;
