/*
 * Process Hacker -
 *   KProcessHacker dynamic data definitions
 *
 * Copyright (C) 2011 wj32
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

#ifdef _M_X64

NTSTATUS KphInitializeDynamicPackage(
    __out PKPH_DYN_PACKAGE Package
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

    // Windows Vista, Windows Server 2008
    if (majorVersion == 6 && minorVersion == 0)
    {
        Package->ResultingNtVersion = PHNT_VISTA;

        if (servicePack == 0)
        {
            Package->StructData.OtName = 0x78;
            Package->StructData.OtIndex = 0x90;
        }
        else if (servicePack == 1)
        {
            Package->StructData.OtName = 0x10;
            Package->StructData.OtIndex = 0x28;
        }
        else if (servicePack == 2)
        {
            Package->StructData.OtName = 0x10;
            Package->StructData.OtIndex = 0x28;
        }
        else
        {
            return STATUS_NOT_SUPPORTED;
        }

        Package->StructData.EgeGuid = 0x14;
        Package->StructData.EpObjectTable = 0x160;
        Package->StructData.EpProtectedProcessOff = 0x36c;
        Package->StructData.EpProtectedProcessBit = 0xb;
        Package->StructData.EpRundownProtect = 0xd8;
        Package->StructData.EreGuidEntry = 0x10;
    }
    // Windows 7, Windows Server 2008 R2
    else if (majorVersion == 6 && minorVersion == 1)
    {
        Package->ResultingNtVersion = PHNT_WIN7;

        if (servicePack == 0)
        {
        }
        else if (servicePack == 1)
        {
        }
        else
        {
            return STATUS_NOT_SUPPORTED;
        }

        Package->StructData.EgeGuid = 0x14;
        Package->StructData.EpObjectTable = 0x200;
        Package->StructData.EpProtectedProcessOff = 0x43c;
        Package->StructData.EpProtectedProcessBit = 0xb;
        Package->StructData.EpRundownProtect = 0x178;
        Package->StructData.EreGuidEntry = 0x10;
        Package->StructData.OtName = 0x10;
        Package->StructData.OtIndex = 0x28; // now only a UCHAR, not a ULONG
    }
    // Windows Developer Preview
    else if (majorVersion == 6 && minorVersion == 2 && buildNumber == 8102)
    {
        Package->BuildNumber = 8102;
        Package->ResultingNtVersion = PHNT_WIN8;

        Package->StructData.EgeGuid = 0x14;
        Package->StructData.EpObjectTable = 0x2f0;
        Package->StructData.EpProtectedProcessOff = -1; // now SE_SIGNING_LEVEL, no longer relevant
        Package->StructData.EpProtectedProcessBit = -1;
        Package->StructData.EpRundownProtect = 0x1c8;
        Package->StructData.EreGuidEntry = 0x10;
        Package->StructData.HtHandleContentionEvent = 0x30;
        Package->StructData.OtName = 0x10;
        Package->StructData.OtIndex = 0x28;
    }
    else
    {
        return STATUS_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS;
}

#else

NTSTATUS KphInitializeDynamicPackage(
    __out PKPH_DYN_PACKAGE Package
    )
{
    return STATUS_NOT_SUPPORTED;
}

#endif
