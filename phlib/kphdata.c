/*
 * Process Hacker -
 *   KProcessHacker dynamic data definitions
 *
 * Copyright (C) 2011-2016 wj32
 * Copyright (C) 2017 dmex
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ph.h>
#include <kphuser.h>

#ifdef _WIN64

ULONG KphpGetKernelRevisionNumber(
    VOID
    )
{
    ULONG result;
    PPH_STRING kernelFileName;
    PVOID versionInfo;
    VS_FIXEDFILEINFO *rootBlock;
    ULONG rootBlockLength;

    result = 0;
    kernelFileName = PhGetKernelFileName();
    PhMoveReference(&kernelFileName, PhGetFileName(kernelFileName));
    versionInfo = PhGetFileVersionInfo(kernelFileName->Buffer);
    PhDereferenceObject(kernelFileName);

    if (versionInfo && VerQueryValue(versionInfo, L"\\", &rootBlock, &rootBlockLength) && rootBlockLength != 0)
        result = rootBlock->dwFileVersionLS & 0xffff;

    PhFree(versionInfo);

    return result;
}

NTSTATUS KphInitializeDynamicPackage(
    _Out_ PKPH_DYN_PACKAGE Package
    )
{
    ULONG majorVersion, minorVersion, servicePack, buildNumber;

    majorVersion = PhOsVersion.dwMajorVersion;
    minorVersion = PhOsVersion.dwMinorVersion;
    servicePack = PhOsVersion.wServicePackMajor;
    buildNumber = PhOsVersion.dwBuildNumber;

    memset(&Package->StructData, -1, sizeof(KPH_DYN_STRUCT_DATA));

    Package->MajorVersion = (USHORT)majorVersion;
    Package->MinorVersion = (USHORT)minorVersion;
    Package->ServicePackMajor = (USHORT)servicePack;
    Package->BuildNumber = -1;

    // Windows 7, Windows Server 2008 R2
    if (majorVersion == 6 && minorVersion == 1)
    {
        ULONG revisionNumber = KphpGetKernelRevisionNumber();

        Package->ResultingNtVersion = PHNT_WIN7;

        if (servicePack == 0)
        {
            NOTHING;
        }
        else if (servicePack == 1)
        {
            NOTHING;
        }
        else
        {
            return STATUS_NOT_SUPPORTED;
        }

        Package->StructData.EgeGuid = 0x14;
        Package->StructData.EpObjectTable = 0x200;
        Package->StructData.EreGuidEntry = revisionNumber >= 19160 ? 0x20 : 0x10;
        Package->StructData.OtName = 0x10;
        Package->StructData.OtIndex = 0x28; // now only a UCHAR, not a ULONG
    }
    // Windows 8, Windows Server 2012
    else if (majorVersion == 6 && minorVersion == 2 && buildNumber == 9200)
    {
        Package->BuildNumber = 9200;
        Package->ResultingNtVersion = PHNT_WIN8;

        Package->StructData.EgeGuid = 0x14;
        Package->StructData.EpObjectTable = 0x408;
        Package->StructData.EreGuidEntry = 0x10;
        Package->StructData.HtHandleContentionEvent = 0x30;
        Package->StructData.OtName = 0x10;
        Package->StructData.OtIndex = 0x28;
        Package->StructData.ObDecodeShift = 19;
        Package->StructData.ObAttributesShift = 20;
    }
    // Windows 8.1, Windows Server 2012 R2
    else if (majorVersion == 6 && minorVersion == 3 && buildNumber == 9600)
    {
        ULONG revisionNumber = KphpGetKernelRevisionNumber();

        Package->BuildNumber = 9600;
        Package->ResultingNtVersion = PHNT_WINBLUE;

        Package->StructData.EgeGuid = 0x18;
        Package->StructData.EpObjectTable = 0x408;
        Package->StructData.EreGuidEntry = revisionNumber >= 17736 ? 0x20 : 0x10;
        Package->StructData.HtHandleContentionEvent = 0x30;
        Package->StructData.OtName = 0x10;
        Package->StructData.OtIndex = 0x28;
        Package->StructData.ObDecodeShift = 16;
        Package->StructData.ObAttributesShift = 17;
    }
    // Windows 10, Windows Server 2016
    else if (majorVersion == 10 && minorVersion == 0)
    {
        switch (buildNumber)
        {
        case 10240:
            Package->BuildNumber = 10240;
            Package->ResultingNtVersion = PHNT_THRESHOLD;
            break;
        case 10586:
            Package->BuildNumber = 10586;
            Package->ResultingNtVersion = PHNT_THRESHOLD2;
            break;
        case 14393:
            Package->BuildNumber = 14393;
            Package->ResultingNtVersion = PHNT_REDSTONE;
            break;
        case 15063:
            Package->BuildNumber = 15063;
            Package->ResultingNtVersion = PHNT_REDSTONE2;
            break;
        case 16299:
            Package->BuildNumber = 16299;
            Package->ResultingNtVersion = PHNT_REDSTONE3;
            break;
        case 17134:
            Package->BuildNumber = 17134;
            Package->ResultingNtVersion = PHNT_REDSTONE4;
            break;
        case 17763:
            Package->BuildNumber = 17763;
            Package->ResultingNtVersion = PHNT_REDSTONE5;
            break;
        default:
            return STATUS_NOT_SUPPORTED;
        }

        Package->StructData.EgeGuid = 0x18;
        Package->StructData.EpObjectTable = 0x418;
        Package->StructData.EreGuidEntry = 0x20;
        Package->StructData.HtHandleContentionEvent = 0x30;
        Package->StructData.OtName = 0x10;
        Package->StructData.OtIndex = 0x28;
        Package->StructData.ObDecodeShift = 16;
        Package->StructData.ObAttributesShift = 17;
    }
    else
    {
        return STATUS_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS;
}

#else

NTSTATUS KphInitializeDynamicPackage(
    _Out_ PKPH_DYN_PACKAGE Package
    )
{
    ULONG majorVersion, minorVersion, servicePack, buildNumber;

    majorVersion = PhOsVersion.dwMajorVersion;
    minorVersion = PhOsVersion.dwMinorVersion;
    servicePack = PhOsVersion.wServicePackMajor;
    buildNumber = PhOsVersion.dwBuildNumber;

    memset(&Package->StructData, -1, sizeof(KPH_DYN_STRUCT_DATA));

    Package->MajorVersion = (USHORT)majorVersion;
    Package->MinorVersion = (USHORT)minorVersion;
    Package->ServicePackMajor = (USHORT)servicePack;
    Package->BuildNumber = -1;

    // Windows 7, Windows Server 2008 R2
    if (majorVersion == 6 && minorVersion == 1)
    {
        Package->ResultingNtVersion = PHNT_WIN7;

        if (servicePack == 0)
        {
            NOTHING;
        }
        else if (servicePack == 1)
        {
            NOTHING;
        }
        else
        {
            return STATUS_NOT_SUPPORTED;
        }

        Package->StructData.EgeGuid = 0xc;
        Package->StructData.EpObjectTable = 0xf4;
        Package->StructData.EreGuidEntry = 0x8;
        Package->StructData.OtName = 0x8;
        Package->StructData.OtIndex = 0x14; // now only a UCHAR, not a ULONG
    }
    // Windows 8, Windows Server 2012
    else if (majorVersion == 6 && minorVersion == 2)
    {
        Package->ResultingNtVersion = PHNT_WIN8;

        if (servicePack == 0)
        {
            NOTHING;
        }
        else
        {
            return STATUS_NOT_SUPPORTED;
        }

        Package->StructData.EgeGuid = 0xc;
        Package->StructData.EpObjectTable = 0x150;
        Package->StructData.EreGuidEntry = 0x8;
        Package->StructData.OtName = 0x8;
        Package->StructData.OtIndex = 0x14;
    }
    // Windows 8.1, Windows Server 2012 R2
    else if (majorVersion == 6 && minorVersion == 3)
    {
        Package->ResultingNtVersion = PHNT_WINBLUE;

        if (servicePack == 0)
        {
            NOTHING;
        }
        else
        {
            return STATUS_NOT_SUPPORTED;
        }

        Package->StructData.EgeGuid = 0xc;
        Package->StructData.EpObjectTable = 0x150;
        Package->StructData.EreGuidEntry = 0x8;
        Package->StructData.OtName = 0x8;
        Package->StructData.OtIndex = 0x14;
    }
    // Windows 10
    else if (majorVersion == 10 && minorVersion == 0)
    {
        switch (buildNumber)
        {
        case 10240:
            Package->BuildNumber = 10240;
            Package->ResultingNtVersion = PHNT_THRESHOLD;
            break;
        case 10586:
            Package->BuildNumber = 10586;
            Package->ResultingNtVersion = PHNT_THRESHOLD2;
            break;
        case 14393:
            Package->BuildNumber = 14393;
            Package->ResultingNtVersion = PHNT_REDSTONE;
            break;
        case 15063:
            Package->BuildNumber = 15063;
            Package->ResultingNtVersion = PHNT_REDSTONE2;
            break;
        case 16299:
            Package->BuildNumber = 16299;
            Package->ResultingNtVersion = PHNT_REDSTONE3;
            break;
        case 17134:
            Package->BuildNumber = 17134;
            Package->ResultingNtVersion = PHNT_REDSTONE4;
            break;
        case 17763:
            Package->BuildNumber = 17763;
            Package->ResultingNtVersion = PHNT_REDSTONE5;
            break;
        default:
            return STATUS_NOT_SUPPORTED;
        }

        Package->StructData.EgeGuid = 0xc;
        Package->StructData.EpObjectTable = 0x154;
        Package->StructData.EreGuidEntry = 0x10;
        Package->StructData.OtName = 0x8;
        Package->StructData.OtIndex = 0x14;
    }
    else
    {
        return STATUS_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS;
}

#endif
