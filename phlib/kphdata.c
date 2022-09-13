/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2022
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
        if (versionInfo = PhGetFileVersionInfoEx(&kernelFileName->sr))
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

NTSTATUS KphInitializeDynamicConfiguration(
    _Out_ PKPH_DYN_CONFIGURATION Configuration
    )
{
    ULONG majorVersion = PhOsVersion.dwMajorVersion;
    ULONG minorVersion = PhOsVersion.dwMinorVersion;
    ULONG servicePack = PhOsVersion.wServicePackMajor;
    ULONG buildNumber = PhOsVersion.dwBuildNumber;

    memset(Configuration, -1, sizeof(KPH_DYN_CONFIGURATION));
    Configuration->Version = KPH_DYN_CONFIGURATION_VERSION;
    Configuration->MajorVersion = (USHORT)majorVersion;
    Configuration->MinorVersion = (USHORT)minorVersion;
    Configuration->ServicePackMajor = (USHORT)servicePack;
    Configuration->BuildNumber = USHRT_MAX;

    // Windows 7, Windows Server 2008 R2
    if (majorVersion == 6 && minorVersion == 1)
    {
        ULONG revisionNumber = KphpGetKernelRevisionNumber();

        Configuration->ResultingNtVersion = PHNT_WIN7;

        if (servicePack == 0)
        {
            NOTHING;
        }
        else if (servicePack == 1)
        {
            Configuration->CiVersion = KPH_DYN_CI_V1;
            if ((revisionNumber == 18519) ||
                (revisionNumber == 22730))
            {
                Configuration->CiVersion = KPH_DYN_CI_V2;
            }
        }
        else
        {
            return STATUS_NOT_SUPPORTED;
        }

        Configuration->EgeGuid = 0x14;
        Configuration->EpObjectTable = 0x200;
        Configuration->EreGuidEntry = revisionNumber >= 19160 ? 0x20 : 0x10;
        Configuration->OtName = 0x10;
        Configuration->OtIndex = 0x28; // now only a UCHAR, not a ULONG
    }
    // Windows 8, Windows Server 2012
    else if (majorVersion == 6 && minorVersion == 2 && buildNumber == 9200)
    {
        Configuration->BuildNumber = 9200;
        Configuration->ResultingNtVersion = PHNT_WIN8;

        Configuration->EgeGuid = 0x14;
        Configuration->EpObjectTable = 0x408;
        Configuration->EreGuidEntry = 0x10;
        Configuration->HtHandleContentionEvent = 0x30;
        Configuration->OtName = 0x10;
        Configuration->OtIndex = 0x28;
        Configuration->ObDecodeShift = 19;
        Configuration->ObAttributesShift = 20;
        Configuration->CiVersion = KPH_DYN_CI_V2;
    }
    // Windows 8.1, Windows Server 2012 R2
    else if (majorVersion == 6 && minorVersion == 3 && buildNumber == 9600)
    {
        ULONG revisionNumber = KphpGetKernelRevisionNumber();

        Configuration->BuildNumber = 9600;
        Configuration->ResultingNtVersion = PHNT_WINBLUE;

        Configuration->EgeGuid = 0x18;
        Configuration->EpObjectTable = 0x408;
        Configuration->EreGuidEntry = revisionNumber >= 17736 ? 0x20 : 0x10;
        Configuration->HtHandleContentionEvent = 0x30;
        Configuration->OtName = 0x10;
        Configuration->OtIndex = 0x28;
        Configuration->ObDecodeShift = 16;
        Configuration->ObAttributesShift = 17;
        Configuration->CiVersion = KPH_DYN_CI_V2;
    }
    // Windows 10, Windows Server 2016, Windows 11, Windows Server 2022
    else if (majorVersion == 10 && minorVersion == 0)
    {
        ULONG revisionNumber = KphpGetKernelRevisionNumber();

        switch (buildNumber)
        {
        case 10240:
            Configuration->BuildNumber = 10240;
            Configuration->ResultingNtVersion = PHNT_THRESHOLD;
            break;
        case 10586:
            Configuration->BuildNumber = 10586;
            Configuration->ResultingNtVersion = PHNT_THRESHOLD2;
            break;
        case 14393:
            Configuration->BuildNumber = 14393;
            Configuration->ResultingNtVersion = PHNT_REDSTONE;
            break;
        case 15063:
            Configuration->BuildNumber = 15063;
            Configuration->ResultingNtVersion = PHNT_REDSTONE2;
            break;
        case 16299:
            Configuration->BuildNumber = 16299;
            Configuration->ResultingNtVersion = PHNT_REDSTONE3;
            break;
        case 17134:
            Configuration->BuildNumber = 17134;
            Configuration->ResultingNtVersion = PHNT_REDSTONE4;
            break;
        case 17763:
            Configuration->BuildNumber = 17763;
            Configuration->ResultingNtVersion = PHNT_REDSTONE5;
            break;
        case 18362:
            Configuration->BuildNumber = 18362;
            Configuration->ResultingNtVersion = PHNT_19H1;
            break;
        case 18363:
            Configuration->BuildNumber = 18363;
            Configuration->ResultingNtVersion = PHNT_19H2;
            break;
        case 19041:
            Configuration->BuildNumber = 19041;
            Configuration->ResultingNtVersion = PHNT_20H1;
            break;
        case 19042:
            Configuration->BuildNumber = 19042;
            Configuration->ResultingNtVersion = PHNT_20H2;
            break;
        case 19043:
            Configuration->BuildNumber = 19043;
            Configuration->ResultingNtVersion = PHNT_21H1;
            break;
        case 19044:
            Configuration->BuildNumber = 19044;
            Configuration->ResultingNtVersion = PHNT_21H2;
            break;
        case 22000:
            Configuration->BuildNumber = 22000;
            Configuration->ResultingNtVersion = PHNT_WIN11;
            break;
        default:
            return STATUS_NOT_SUPPORTED;
        }

        if (buildNumber >= 19041)
            Configuration->EgeGuid = 0x28;
        else if (buildNumber >= 18363)
            Configuration->EgeGuid = revisionNumber >= 693 ? 0x28 : 0x18;
        else
            Configuration->EgeGuid = 0x18;

        if (buildNumber >= 19042)
            Configuration->EpObjectTable = 0x570;
        else
            Configuration->EpObjectTable = 0x418;

        Configuration->EreGuidEntry = 0x20;
        Configuration->HtHandleContentionEvent = 0x30;
        Configuration->OtName = 0x10;
        Configuration->OtIndex = 0x28;
        Configuration->ObDecodeShift = 16;
        Configuration->ObAttributesShift = 17;
        Configuration->CiVersion = KPH_DYN_CI_V2;
    }
    else
    {
        return STATUS_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS;
}

#else

NTSTATUS KphInitializeDynamicConfiguration(
    _Out_ PKPH_DYN_CONFIGURATION Configuration
    )
{
    ULONG majorVersion, minorVersion, servicePack, buildNumber;

    majorVersion = PhOsVersion.dwMajorVersion;
    minorVersion = PhOsVersion.dwMinorVersion;
    servicePack = PhOsVersion.wServicePackMajor;
    buildNumber = PhOsVersion.dwBuildNumber;

    memset(Configuration, -1, sizeof(KPH_DYN_CONFIGURATION));
    Configuration->Version = KPH_DYN_CONFIGURATION_VERSION;
    Configuration->MajorVersion = (USHORT)majorVersion;
    Configuration->MinorVersion = (USHORT)minorVersion;
    Configuration->ServicePackMajor = (USHORT)servicePack;
    Configuration->BuildNumber = -1;

    // Windows 7, Windows Server 2008 R2
    if (majorVersion == 6 && minorVersion == 1)
    {
        ULONG revisionNumber = KphpGetKernelRevisionNumber();

        Configuration->ResultingNtVersion = PHNT_WIN7;

        if (servicePack == 0)
        {
            NOTHING;
        }
        else if (servicePack == 1)
        {
            Configuration->CiVersion = KPH_DYN_CI_V1;
            if ((revisionNumber == 18519) ||
                (revisionNumber == 22730))
            {
                Configuration->CiVersion = KPH_DYN_CI_V2;
            }
        }
        else
        {
            return STATUS_NOT_SUPPORTED;
        }

        Configuration->EgeGuid = 0xc;
        Configuration->EpObjectTable = 0xf4;
        Configuration->EreGuidEntry = 0x8;
        Configuration->OtName = 0x8;
        Configuration->OtIndex = 0x14; // now only a UCHAR, not a ULONG
    }
    // Windows 8, Windows Server 2012
    else if (majorVersion == 6 && minorVersion == 2)
    {
        Configuration->ResultingNtVersion = PHNT_WIN8;

        if (servicePack == 0)
        {
            NOTHING;
        }
        else
        {
            return STATUS_NOT_SUPPORTED;
        }

        Configuration->EgeGuid = 0xc;
        Configuration->EpObjectTable = 0x150;
        Configuration->HtHandleContentionEvent = 0x20;
        Configuration->EreGuidEntry = 0x8;
        Configuration->OtName = 0x8;
        Configuration->OtIndex = 0x14;
        Configuration->CiVersion = KPH_DYN_CI_V2;
    }
    // Windows 8.1, Windows Server 2012 R2
    else if (majorVersion == 6 && minorVersion == 3)
    {
        Configuration->ResultingNtVersion = PHNT_WINBLUE;

        if (servicePack == 0)
        {
            NOTHING;
        }
        else
        {
            return STATUS_NOT_SUPPORTED;
        }

        Configuration->EgeGuid = 0xc;
        Configuration->EpObjectTable = 0x150;
        Configuration->HtHandleContentionEvent = 0x20;
        Configuration->EreGuidEntry = 0x8;
        Configuration->OtName = 0x8;
        Configuration->OtIndex = 0x14;
        Configuration->CiVersion = KPH_DYN_CI_V2;
    }
    // Windows 10
    else if (majorVersion == 10 && minorVersion == 0)
    {
        ULONG revisionNumber = KphpGetKernelRevisionNumber();

        switch (buildNumber)
        {
        case 10240:
            Configuration->BuildNumber = 10240;
            Configuration->ResultingNtVersion = PHNT_THRESHOLD;
            break;
        case 10586:
            Configuration->BuildNumber = 10586;
            Configuration->ResultingNtVersion = PHNT_THRESHOLD2;
            break;
        case 14393:
            Configuration->BuildNumber = 14393;
            Configuration->ResultingNtVersion = PHNT_REDSTONE;
            break;
        case 15063:
            Configuration->BuildNumber = 15063;
            Configuration->ResultingNtVersion = PHNT_REDSTONE2;
            break;
        case 16299:
            Configuration->BuildNumber = 16299;
            Configuration->ResultingNtVersion = PHNT_REDSTONE3;
            break;
        case 17134:
            Configuration->BuildNumber = 17134;
            Configuration->ResultingNtVersion = PHNT_REDSTONE4;
            break;
        case 17763:
            Configuration->BuildNumber = 17763;
            Configuration->ResultingNtVersion = PHNT_REDSTONE5;
            break;
        case 18362:
            Configuration->BuildNumber = 18362;
            Configuration->ResultingNtVersion = PHNT_19H1;
            break;
        case 18363:
            Configuration->BuildNumber = 18363;
            Configuration->ResultingNtVersion = PHNT_19H2;
            break;
        case 19041:
            Configuration->BuildNumber = 19041;
            Configuration->ResultingNtVersion = PHNT_20H1;
            break;
        case 19042:
            Configuration->BuildNumber = 19042;
            Configuration->ResultingNtVersion = PHNT_20H2;
            break;
        case 19043:
            Configuration->BuildNumber = 19043;
            Configuration->ResultingNtVersion = PHNT_21H1;
            break;
        case 19044:
            Configuration->BuildNumber = 19044;
            Configuration->ResultingNtVersion = PHNT_21H2;
            break;
        default:
            return STATUS_NOT_SUPPORTED;
        }

        if (buildNumber >= 19041)
            Configuration->EgeGuid = 0x14;
        else if (buildNumber >= 18363)
            Configuration->EgeGuid = revisionNumber >= 693 ? 0x14 : 0xC;
        else
            Configuration->EgeGuid = 0xC;

        if (buildNumber >= 19041)
            Configuration->EpObjectTable = 0x18c;
        else if (buildNumber >= 15063)
            Configuration->EpObjectTable = 0x15c;
        else
            Configuration->EpObjectTable = 0x154;

        Configuration->EreGuidEntry = 0x10;
        Configuration->HtHandleContentionEvent = 0x20;
        Configuration->OtName = 0x8;
        Configuration->OtIndex = 0x14;
        Configuration->CiVersion = KPH_DYN_CI_V2;
    }
    else
    {
        return STATUS_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS;
}

#endif
