/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2021
 *
 */

#include <ph.h>
#include <kphuser.h>

ULONG KphpGetKernelRevisionNumber(
    VOID
    )
{
    ULONG result = 0;
    PPH_STRING kernelFileName;
    PVOID versionInfo;

    if (kernelFileName = PhGetKernelFileName())
    {
        if (versionInfo = PhGetFileVersionInfoEx(kernelFileName))
        {
            VS_FIXEDFILEINFO* rootBlock;

            if (rootBlock = PhGetFileVersionFixedInfo(versionInfo))
            {
                result = LOWORD(rootBlock->dwFileVersionLS);
            }

            PhFree(versionInfo);
        }

        PhDereferenceObject(kernelFileName);
    }

    return result;
}

#ifdef _WIN64

NTSTATUS KphInitializeDynamicPackage(
    _Out_ PKPH_DYN_PACKAGE Package
    )
{
    ULONG majorVersion = PhOsVersion.dwMajorVersion;
    ULONG minorVersion = PhOsVersion.dwMinorVersion;
    ULONG servicePack = PhOsVersion.wServicePackMajor;
    ULONG buildNumber = PhOsVersion.dwBuildNumber;

    memset(&Package->StructData, -1, sizeof(KPH_DYN_STRUCT_DATA));
    Package->MajorVersion = (USHORT)majorVersion;
    Package->MinorVersion = (USHORT)minorVersion;
    Package->ServicePackMajor = (USHORT)servicePack;
    Package->BuildNumber = USHRT_MAX;

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
    // Windows 10, Windows Server 2016, Windows 11, Windows Server 2022
    else if (majorVersion == 10 && minorVersion == 0)
    {
        ULONG revisionNumber = KphpGetKernelRevisionNumber();

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
        case 18362:
            Package->BuildNumber = 18362;
            Package->ResultingNtVersion = PHNT_19H1;
            break;
        case 18363:
            Package->BuildNumber = 18363;
            Package->ResultingNtVersion = PHNT_19H2;
            break;
        case 19041:
            Package->BuildNumber = 19041;
            Package->ResultingNtVersion = PHNT_20H1;
            break;
        case 19042:
            Package->BuildNumber = 19042;
            Package->ResultingNtVersion = PHNT_20H2;
            break;
        case 19043:
            Package->BuildNumber = 19043;
            Package->ResultingNtVersion = PHNT_21H1;
            break;
        case 19044:
            Package->BuildNumber = 19044;
            Package->ResultingNtVersion = PHNT_21H2;
            break;
        case 22000:
            Package->BuildNumber = 22000;
            Package->ResultingNtVersion = PHNT_WIN11;
            break;
        default:
            return STATUS_NOT_SUPPORTED;
        }

        if (buildNumber >= 19041)
            Package->StructData.EgeGuid = 0x28;
        else if (buildNumber >= 18363)
            Package->StructData.EgeGuid = revisionNumber >= 693 ? 0x28 : 0x18;
        else
            Package->StructData.EgeGuid = 0x18;

        if (buildNumber >= 19042)
            Package->StructData.EpObjectTable = 0x570;
        else
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
        ULONG revisionNumber = KphpGetKernelRevisionNumber();

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
        case 18362:
            Package->BuildNumber = 18362;
            Package->ResultingNtVersion = PHNT_19H1;
            break;
        case 18363:
            Package->BuildNumber = 18363;
            Package->ResultingNtVersion = PHNT_19H2;
            break;
        case 19041:
            Package->BuildNumber = 19041;
            Package->ResultingNtVersion = PHNT_20H1;
            break;
        case 19042:
            Package->BuildNumber = 19042;
            Package->ResultingNtVersion = PHNT_20H2;
            break;
        case 19043:
            Package->BuildNumber = 19043;
            Package->ResultingNtVersion = PHNT_21H1;
            break;
        case 19044:
            Package->BuildNumber = 19044;
            Package->ResultingNtVersion = PHNT_21H2;
            break;
        default:
            return STATUS_NOT_SUPPORTED;
        }

        if (buildNumber >= 19041)
            Package->StructData.EgeGuid = 0x14;
        else if (buildNumber >= 18363)
            Package->StructData.EgeGuid = revisionNumber >= 693 ? 0x14 : 0xC;
        else
            Package->StructData.EgeGuid = 0xC;

        if (buildNumber >= 19041)
            Package->StructData.EpObjectTable = 0x18c;
        else if (buildNumber >= 15063)
            Package->StructData.EpObjectTable = 0x15c;
        else
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
