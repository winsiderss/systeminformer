# 
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
# 
# This file is part of System Informer.
# 
# Authors:
# 
#     dmex    2024
# 
# 

$Export_Header = @"
/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2008-2016
 *     dmex    2017-2024
 *
 * 
 * This file was automatically generated.
 * 
 * Do not link at runtime. Use the SystemInformer.def.h header file instead.
 * 
 */
"@;

function UpdateExportDefs()
{
    $ordinal = 1000
    $content = New-Object System.Text.StringBuilder
    $exports = Get-Content -Path "SystemInformer\SystemInformer.def"

    foreach ($line in $exports)
    {
        $span = $line.Trim();

        if (
            [string]::IsNullOrWhiteSpace($span) -or 
            $span.StartsWith("EXPORTS", [StringComparison]::OrdinalIgnoreCase) -or 
            $span.StartsWith(";", [StringComparison]::OrdinalIgnoreCase)
            )
        {
            [void]$content.AppendLine($span);
        }
        else
        {
            $length = $span.IndexOf("@", [StringComparison]::OrdinalIgnoreCase);

            if ($length -ne -1)
            {
                $export = ("    " + $span.Substring(0, $length).Trim());
                $offset = (" @" + (++$ordinal));

                if ($span.EndsWith("DATA", [StringComparison]::OrdinalIgnoreCase)) 
                {
                    $format = "{0,-80} {1,-10} NONAME DATA" -f $export, $offset
                    [void]$content.AppendLine($format.TrimEnd());
                }
                else
                {
                    $format = "{0,-80} {1,-10} NONAME" -f $export, $offset
                    [void]$content.AppendLine($format.TrimEnd());
                }
            }
            else 
            {
                $firstWord = $span.Split(" ")[0];  # Extract the first word
                $export = ("    " + $firstWord.Trim());
                $offset = (" @" + (++$ordinal));
                
                if ($span.EndsWith("DATA", [StringComparison]::OrdinalIgnoreCase)) 
                {
                    $format = "{0,-80} {1,-10} NONAME DATA" -f $export, $offset
                    [void]$content.AppendLine($format.TrimEnd());
                }
                else
                {
                    $format = "{0,-80} {1,-10} NONAME" -f $export, $offset,
                    [void]$content.AppendLine($format.TrimEnd());
                }
            }
        }
    }

    $export_content = $content.ToString().TrimEnd()

    $export_content | Out-File -FilePath "SystemInformer\SystemInformer.def" -Force
}

function UpdateExportHeader()
{
    $content = New-Object System.Text.StringBuilder
    $exports = Get-Content -Path "SystemInformer\SystemInformer.def"

    [void]$content.AppendLine($Export_Header);
    [void]$content.AppendLine("`r`n#pragma once`r`n"); 
    [void]$content.AppendLine("#ifndef _PH_EXPORT_DEF_H"); 
    [void]$content.AppendLine("#define _PH_EXPORT_DEF_H`r`n"); 

    foreach ($line in $exports)
    {
        $span = $line.Trim();

        if ([string]::IsNullOrWhiteSpace($span) -eq $false)
        {
            $ordinal = $null
            $reader = [System.IO.StringReader]::new($span)
            $line = $reader.ReadLine()

            while (![string]::IsNullOrWhiteSpace($line))
            {
                $line = $line.Trim()

                if (![string]::IsNullOrWhiteSpace($line))
                {
                    $offset = $line.IndexOf("@", [StringComparison]::OrdinalIgnoreCase)

                    if ($offset -ne -1)
                    {
                        $ordinal = $line.Substring($offset + 1)             
                        $length = $ordinal.IndexOf(" NONAME", [StringComparison]::OrdinalIgnoreCase)     
            
                        [void]$content.Append('#define EXPORT_');
                        [void]$content.Append($line.Substring(0, $offset).ToUpper());
                        [void]$content.Append(' ');
                        [void]$content.Append($ordinal.Substring(0, $length));
                        [void]$content.AppendLine();
                    }
                }
                
                $line = $reader.ReadLine()
            }
        }
    }

    [void]$content.AppendLine("`r`n#endif _PH_EXPORT_DEF_H`r`n"); 

    $export_content = $content.ToString().TrimEnd();

    $export_content | Out-File -FilePath "SystemInformer\SystemInformer.def.h" -Force
}

# Get script start time
$global:TimeStart = (Get-Date);
# Stop script execution after any errors.
$global:ErrorActionPreference = "Stop";

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

# Update the solution exports
UpdateExportDefs;
UpdateExportHeader;