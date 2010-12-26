/*
 * KProcessHacker
 * 
 * Copyright (C) 2010 wj32
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

#include <kph.h>
#define _DYNDATA_PRIVATE
#include <dyndata.h>

#define INIT_SCAN(scan, bytes, length, address, scanLength, displacement) \
    ( \
    ((scan)->Initialized = TRUE), \
    ((scan)->Scanned = FALSE), \
    ((scan)->Bytes = (bytes)), \
    ((scan)->Length = (length)), \
    ((scan)->StartAddress = (address)), \
    ((scan)->ScanLength = (scanLength)), \
    ((scan)->Displacement = (displacement)), \
    ((scan)->ProcedureAddress = NULL), \
    bytes \
    )

#ifdef _X86_

NTSTATUS KphpX86DataInitialization(
    VOID
    );

#else

NTSTATUS KphpAmd64DataInitialization(
    VOID
    );

#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KphDynamicDataInitialization)
#ifdef _X86_
#pragma alloc_text(PAGE, KphpX86DataInitialization)
#else
#pragma alloc_text(PAGE, KphpAmd64DataInitialization)
#endif
#endif

#ifdef _X86_

// PsTerminateProcess/PspTerminateProcess
static UCHAR PspTerminateProcess51Bytes[] =
{
    0x8b, 0xff, 0x55, 0x8b, 0xec, 0x56, 0x64, 0xa1,
    0x24, 0x01, 0x00, 0x00, 0x8b, 0x75, 0x08, 0x3b
};
static UCHAR PspTerminateProcess52Bytes[] =
{
    0x8b, 0xff, 0x55, 0x8b, 0xec, 0x56, 0x8b, 0x75,
    0x08, 0x57, 0x8d, 0xbe, 0x40, 0x02, 0x00, 0x00
};
static UCHAR PsTerminateProcess60Bytes[] =
{
    0x8b, 0xff, 0x55, 0x8b, 0xec, 0x53, 0x56, 0x57,
    0x33, 0xd2, 0x6a, 0x08, 0x42, 0x5e, 0x8d, 0xb9
};
static UCHAR PsTerminateProcess61Bytes[] =
{
    0x8b, 0xff, 0x55, 0x8b, 0xec, 0x51, 0x51, 0x53,
    0x56, 0x64, 0x8b, 0x35, 0x24, 0x01, 0x00, 0x00,
    0x66, 0xff, 0x8e, 0x84, 0x00, 0x00, 0x00, 0x57,
    0xc7, 0x45, 0xfc
}; // a lot of functions seem to share the first 
   // 16 bytes of the Windows 7 PsTerminateProcess, 
   // and a few even share the first 24 bytes.

// PspTerminateThreadByPointer
static UCHAR PspTerminateThreadByPointer51Bytes[] =
{
    0x8b, 0xff, 0x55, 0x8b, 0xec, 0x83, 0xec, 0x0c,
    0x83, 0x4d, 0xf8, 0xff, 0x56, 0x57, 0x8b, 0x7d
};
static UCHAR PspTerminateThreadByPointer52Bytes[] =
{
    0x8b, 0xff, 0x55, 0x8b, 0xec, 0x53, 0x56, 0x57,
    0x8b, 0x7d, 0x08, 0x8d, 0xb7, 0x40, 0x02, 0x00
};
static UCHAR PspTerminateThreadByPointer60Bytes[] =
{
    0x8b, 0xff, 0x55, 0x8b, 0xec, 0x83, 0xe4, 0xf8,
    0x51, 0x53, 0x56, 0x8b, 0x75, 0x08, 0x57, 0x8d,
    0xbe, 0x60, 0x02, 0x00, 0x00, 0xf6, 0x07, 0x40
};
static UCHAR PspTerminateThreadByPointer61Bytes[] =
{
    0x8b, 0xff, 0x55, 0x8b, 0xec, 0x83, 0xe4, 0xf8,
    0x51, 0x53, 0x56, 0x8b, 0x75, 0x08, 0x57, 0x8d,
    0xbe, 0x80, 0x02, 0x00, 0x00, 0xf6, 0x07, 0x40
};

#else

static UCHAR PsTerminateProcess61Bytes[] =
{
    0x48, 0x89, 0x5c, 0x24, 0x08, 0x48, 0x89, 0x74,
    0x24, 0x10, 0x57, 0x48, 0x83, 0xec, 0x20, 0x65,
    0x48, 0x8b, 0x1c, 0x25, 0x88, 0x01, 0x00, 0x00,
    0x4c, 0x8b, 0xd1, 0xbe, 0x01, 0x00, 0x00, 0x00
};
static UCHAR PspTerminateThreadByPointer61Bytes[] =
{
    0x48, 0x89, 0x5c, 0x24, 0x08, 0x48, 0x89, 0x6c,
    0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18, 0x57,
    0x48, 0x83, 0xec, 0x40, 0xf6, 0x81, 0x48, 0x04,
    0x00, 0x00, 0x40, 0x41, 0x8a, 0xf0, 0x8b, 0xea
};

#endif

NTSTATUS KphDynamicDataInitialization(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    // Get Windows version information.

    KphDynOsVersionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
    status = RtlGetVersion((PRTL_OSVERSIONINFOW)&KphDynOsVersionInfo);

    if (!NT_SUCCESS(status))
        return status;

#ifdef _X86_
    return KphpX86DataInitialization();
#else
    return KphpAmd64DataInitialization();
#endif
}

#ifdef _X86_

static NTSTATUS KphpX86DataInitialization(
    VOID
    )
{
    ULONG majorVersion, minorVersion, servicePack, buildNumber;

    majorVersion = KphDynOsVersionInfo.dwMajorVersion;
    minorVersion = KphDynOsVersionInfo.dwMinorVersion;
    servicePack = KphDynOsVersionInfo.wServicePackMajor;
    buildNumber = KphDynOsVersionInfo.dwBuildNumber;
    dprintf("Windows %d.%d, SP%d.%d, build %d\n",
        majorVersion, minorVersion, servicePack,
        KphDynOsVersionInfo.wServicePackMinor, buildNumber
        );

    // Windows XP
    if (majorVersion == 5 && minorVersion == 1)
    {
        ULONG_PTR searchOffset = (ULONG_PTR)KphGetSystemRoutineAddress(L"NtClose");
        ULONG scanLength = 0x100000;

        KphDynNtVersion = PHNT_WINXP;

        KphDynEpObjectTable = 0xc4;
        KphDynEpRundownProtect = 0x80;
        KphDynOtName = 0x40;
        KphDynOtIndex = 0x4c;

        if (searchOffset)
        {
            // We are scanning for PspTerminateProcess which has
            // the same signature as PsTerminateProcess because
            // PsTerminateProcess is simply a wrapper on XP.
            INIT_SCAN(
                &KphDynPsTerminateProcessScan,
                PspTerminateProcess51Bytes,
                sizeof(PspTerminateProcess51Bytes),
                searchOffset, scanLength, 0
                );
            INIT_SCAN(
                &KphDynPspTerminateThreadByPointerScan,
                PspTerminateThreadByPointer51Bytes,
                sizeof(PspTerminateThreadByPointer51Bytes),
                searchOffset, scanLength, 0
                );
        }

        // Windows XP SP0 and 1 are not supported
        if (servicePack == 0)
        {
            return STATUS_NOT_SUPPORTED;
        }
        else if (servicePack == 1)
        {
            return STATUS_NOT_SUPPORTED;
        }
        else if (servicePack == 2)
        {
        }
        else if (servicePack == 3)
        {
        }
        else
        {
            return STATUS_NOT_SUPPORTED;
        }

        dprintf("Initialized version-specific data for Windows XP SP%d\n", servicePack);
    }
    // Windows Server 2003
    else if (majorVersion == 5 && minorVersion == 2)
    {
        ULONG_PTR searchOffset = (ULONG_PTR)KphGetSystemRoutineAddress(L"RtlCreateHeap");
        ULONG scanLength = 0x100000;

        KphDynNtVersion = PHNT_WS03;

        KphDynEpObjectTable = 0xd4;
        KphDynEpRundownProtect = 0x90;
        KphDynOtName = 0x40;
        KphDynOtIndex = 0x4c;

        if (searchOffset)
        {
            // We are scanning for PspTerminateProcess which has
            // the same signature as PsTerminateProcess because
            // PsTerminateProcess is simply a wrapper on Server 2003.
            INIT_SCAN(
                &KphDynPsTerminateProcessScan,
                PspTerminateProcess52Bytes,
                sizeof(PspTerminateProcess52Bytes),
                searchOffset - 0x50000, scanLength, 0
                );
            INIT_SCAN(
                &KphDynPspTerminateThreadByPointerScan,
                PspTerminateThreadByPointer52Bytes,
                sizeof(PspTerminateThreadByPointer52Bytes),
                searchOffset - 0x20000, scanLength, 0
                );
        }

        if (servicePack == 0)
        {
        }
        else if (servicePack == 1)
        {
        }
        else if (servicePack == 2)
        {
        }
        else
        {
            return STATUS_NOT_SUPPORTED;
        }

        dprintf("Initialized version-specific data for Windows Server 2003 SP%d\n", servicePack);
    }
    // Windows Vista, Windows Server 2008
    else if (majorVersion == 6 && minorVersion == 0)
    {
        ULONG_PTR searchOffset = (ULONG_PTR)KphGetSystemRoutineAddress(L"NtClose");
        ULONG scanLength = 0x100000;

        KphDynNtVersion = PHNT_VISTA;

        KphDynEgeGuid = 0xc;
        KphDynEpObjectTable = 0xdc;
        KphDynEpProtectedProcessOff = 0x224;
        KphDynEpProtectedProcessBit = 0xb;
        KphDynEpRundownProtect = 0x98;
        KphDynEreGuidEntry = 0x8;

        if (searchOffset)
        {
            INIT_SCAN(
                &KphDynPsTerminateProcessScan,
                PsTerminateProcess60Bytes,
                sizeof(PsTerminateProcess60Bytes),
                searchOffset, scanLength, 0
                );
            INIT_SCAN(
                &KphDynPspTerminateThreadByPointerScan,
                PspTerminateThreadByPointer60Bytes,
                sizeof(PspTerminateThreadByPointer60Bytes),
                searchOffset - 0x50000, scanLength, 0
                );
        }

        if (servicePack == 0)
        {
            KphDynOtName = 0x40;
            KphDynOtIndex = 0x4c;
        }
        else if (servicePack == 1)
        {
            KphDynOtName = 0x8; // they got rid of Mutex (ERESOURCE)
            KphDynOtIndex = 0x14;
        }
        else if (servicePack == 2)
        {
            KphDynOtName = 0x8;
            KphDynOtIndex = 0x14;
        }
        else
        {
            return STATUS_NOT_SUPPORTED;
        }

        dprintf("Initialized version-specific data for Windows Vista SP%d/Windows Server 2008\n", servicePack);
    }
    // Windows 7, Windows Server 2008 R2
    else if (majorVersion == 6 && minorVersion == 1)
    {
        ULONG_PTR searchOffset = (ULONG_PTR)KphGetSystemRoutineAddress(L"PsSetCreateProcessNotifyRoutine");
        ULONG scanLength = 0x200000;

        KphDynNtVersion = PHNT_WIN7;

        KphDynEgeGuid = 0xc;
        KphDynEpObjectTable = 0xf4;
        KphDynEpProtectedProcessOff = 0x26c;
        KphDynEpProtectedProcessBit = 0xb;
        KphDynEpRundownProtect = 0xb0;
        KphDynEreGuidEntry = 0x8;
        KphDynOtName = 0x8;
        KphDynOtIndex = 0x14; // now only a UCHAR, not a ULONG

        if (searchOffset)
        {
            INIT_SCAN(
                &KphDynPsTerminateProcessScan,
                PsTerminateProcess61Bytes,
                sizeof(PsTerminateProcess61Bytes),
                searchOffset, scanLength, 0
                );
            INIT_SCAN(
                &KphDynPspTerminateThreadByPointerScan,
                PspTerminateThreadByPointer61Bytes,
                sizeof(PspTerminateThreadByPointer61Bytes),
                searchOffset, scanLength, 0
                );
        }

        if (servicePack == 0)
        {
        }
        else
        {
            return STATUS_NOT_SUPPORTED;
        }

        dprintf("Initialized version-specific data for Windows 7 SP%d\n", servicePack);
    }
    else
    {
        return STATUS_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS;
}

#else

static NTSTATUS KphpAmd64DataInitialization(
    VOID
    )
{
    ULONG majorVersion, minorVersion, servicePack, buildNumber;

    majorVersion = KphDynOsVersionInfo.dwMajorVersion;
    minorVersion = KphDynOsVersionInfo.dwMinorVersion;
    servicePack = KphDynOsVersionInfo.wServicePackMajor;
    buildNumber = KphDynOsVersionInfo.dwBuildNumber;
    dprintf("Windows %d.%d, SP%d.%d, build %d\n",
        majorVersion, minorVersion, servicePack,
        KphDynOsVersionInfo.wServicePackMinor, buildNumber
        );

    // Windows XP
    if (majorVersion == 5 && minorVersion == 1)
    {
        KphDynNtVersion = PHNT_WINXP;

        if (servicePack == 0)
        {
            return STATUS_NOT_SUPPORTED;
        }
        else if (servicePack == 1)
        {
            return STATUS_NOT_SUPPORTED;
        }
        else if (servicePack == 2)
        {
        }
        else if (servicePack == 3)
        {
        }
        else
        {
            return STATUS_NOT_SUPPORTED;
        }
    }
    // Windows Server 2003
    else if (majorVersion == 5 && minorVersion == 2)
    {
        KphDynNtVersion = PHNT_WS03;

        if (servicePack == 0)
        {
        }
        else if (servicePack == 1)
        {
        }
        else if (servicePack == 2)
        {
        }
        else
        {
            return STATUS_NOT_SUPPORTED;
        }
    }
    // Windows Vista, Windows Server 2008
    else if (majorVersion == 6 && minorVersion == 0)
    {
        KphDynNtVersion = PHNT_VISTA;

        KphDynEgeGuid = 0x14;
        KphDynEreGuidEntry = 0x10;
        KphDynOtName = 0x10;
        KphDynOtIndex = 0x28;

        if (servicePack == 0)
        {
        }
        else if (servicePack == 1)
        {
        }
        else if (servicePack == 2)
        {
        }
        else
        {
            return STATUS_NOT_SUPPORTED;
        }
    }
    // Windows 7, Windows Server 2008 R2
    else if (majorVersion == 6 && minorVersion == 1)
    {
        ULONG_PTR searchOffset = (ULONG_PTR)KphGetSystemRoutineAddress(L"ObQueryNameString");
        ULONG scanLength = 0x100000;

        KphDynNtVersion = PHNT_WIN7;

        KphDynEgeGuid = 0x14;
        KphDynEpObjectTable = 0x200;
        KphDynEpProtectedProcessOff = 0x43c;
        KphDynEpProtectedProcessBit = 0xb;
        KphDynEpRundownProtect = 0x178;
        KphDynEreGuidEntry = 0x10;
        KphDynOtName = 0x10;
        KphDynOtIndex = 0x28; // now only a UCHAR, not a ULONG

        if (searchOffset)
        {
            // Disabled for now because they aren't really necessary on x64.
            //INIT_SCAN(
            //    &KphDynPsTerminateProcessScan,
            //    PsTerminateProcess61Bytes,
            //    sizeof(PsTerminateProcess61Bytes),
            //    searchOffset, scanLength, 0
            //    );
            //INIT_SCAN(
            //    &KphDynPspTerminateThreadByPointerScan,
            //    PspTerminateThreadByPointer61Bytes,
            //    sizeof(PspTerminateThreadByPointer61Bytes),
            //    searchOffset, scanLength, 0
            //    );
        }

        if (servicePack == 0)
        {
        }
        else
        {
            return STATUS_NOT_SUPPORTED;
        }

        dprintf("Initialized version-specific data for Windows 7 SP%d\n", servicePack);
    }
    else
    {
        return STATUS_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS;
}

#endif

PVOID KphGetDynamicProcedureScan(
    __inout PKPH_PROCEDURE_SCAN ProcedureScan
    )
{
    PUCHAR bytes;
    ULONG length;
    ULONG_PTR endAddress;
    ULONG_PTR address;

    if (!ProcedureScan->Initialized)
        return NULL;

    if (!ProcedureScan->Scanned)
    {
        bytes = ProcedureScan->Bytes;
        length = ProcedureScan->Length;
        endAddress = ProcedureScan->StartAddress + ProcedureScan->ScanLength;

        for (address = ProcedureScan->StartAddress; address < endAddress; address++)
        {
            if (memcmp((PVOID)address, bytes, length) == 0)
            {
                ProcedureScan->ProcedureAddress = (PVOID)(address + ProcedureScan->Displacement);
                break;
            }
        }

        ProcedureScan->Scanned = TRUE;
    }

    return ProcedureScan->ProcedureAddress;
}
