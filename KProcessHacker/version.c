/*
 * Process Hacker Driver - 
 *   Windows version-specific data
 * 
 * Copyright (C) 2009 wj32
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

#define _VERSION_PRIVATE
#include "include/version.h"
#include "include/debug.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KvInit)
#pragma alloc_text(PAGE, KvScanProc)
#pragma alloc_text(PAGE, KvVerifyPrologue)
#endif

/*
 * mov      edi, edi
 * push     ebp
 * mov      ebp, esp
 */
static char StandardPrologue[] = { 0x8b, 0xff, 0x55, 0x8b, 0xec };

/* KiFastCallEntry */
/*
 * Note that this scan will get the address of 
 * mov      esi, edx
 * within KiFastCallEntry, not the start of KiFastCallEntry. 
 * We will then subtract 7 to get the address of 
 * inc      dword ptr fs:PbSystemCalls
 * See sysservice.c for more details.
 */
static char KiFastCallEntry51[] =
{
    0x8b, 0xf2, 0x8b, 0x5f, 0x0c, 0x33, 0xc9, 0x8a,
    0x0c, 0x18, 0x8b, 0x3f, 0x8b, 0x1c, 0x87, 0x2b
};
static char KiFastCallEntry52[] =
{
    0x8b, 0xf2, 0x8b, 0x5f, 0x0c, 0x33, 0xc9, 0x8a,
    0x0c, 0x18, 0x8b, 0x3f, 0x8b, 0x1c, 0x87, 0x2b
}; /* same as 5.1 */
static char KiFastCallEntry60[] =
{
    0x8b, 0xf2, 0x33, 0xc9, 0x8b, 0x57, 0x0c, 0x8b,
    0x3f, 0x8a, 0x0c, 0x10, 0x8b, 0x14, 0x87, 0x2b
};
static char KiFastCallEntry61[] =
{
    0x8b, 0xf2, 0x33, 0xc9, 0x8b, 0x57, 0x0c, 0x8b,
    0x3f, 0x8a, 0x0c, 0x10, 0x8b, 0x14, 0x87, 0x2b
}; /* same as 6.0 */
/* Below is the scan to find the start of KiFastCallEntry. */
/* static char KiFastCallEntry[] =
{
    0xb9, 0x23, 0x00, 0x00, 0x00, 0x6a, 0x30, 0x0f,
    0xa1, 0x8e, 0xd9, 0x8e, 0xc1, 0x64, 0x8b, 0x0d
}; */

/* PsExitSpecialApc */
static char PsExitSpecialApc51[] =
{
    0x8b, 0xff, 0x55, 0x8b, 0xec, 0x64, 0xa1, 0x24,
    0x01, 0x00, 0x00, 0x8b, 0x45, 0x08, 0xf6, 0x40
};
static char PsExitSpecialApc60[] =
{
    0x8b, 0xff, 0x55, 0x8b, 0xec, 0x83, 0xe4, 0xf8,
    0x51, 0x8b, 0x45, 0x08, 0xf6, 0x40, 0x28, 0x01
};
static char PsExitSpecialApc61[] =
{
    0x8b, 0xff, 0x55, 0x8b, 0xec, 0x83, 0xe4, 0xf8,
    0x51, 0x8b, 0x45, 0x08, 0xf6, 0x40, 0x28, 0x01
}; /* same as 6.0 */

/* PsTerminateProcess/PspTerminateProcess */
static char PspTerminateProcess51[] =
{
    0x8b, 0xff, 0x55, 0x8b, 0xec, 0x56, 0x64, 0xa1,
    0x24, 0x01, 0x00, 0x00, 0x8b, 0x75, 0x08, 0x3b
};
static char PspTerminateProcess52[] =
{
    0x8b, 0xff, 0x55, 0x8b, 0xec, 0x56, 0x8b, 0x75,
    0x08, 0x57, 0x8d, 0xbe, 0x40, 0x02, 0x00, 0x00
};
static char PsTerminateProcess60[] =
{
    0x8b, 0xff, 0x55, 0x8b, 0xec, 0x53, 0x56, 0x57,
    0x33, 0xd2, 0x6a, 0x08, 0x42, 0x5e, 0x8d, 0xb9
};
static char PsTerminateProcess61[] =
{
    0x8b, 0xff, 0x55, 0x8b, 0xec, 0x51, 0x51, 0x53,
    0x56, 0x64, 0x8b, 0x35, 0x24, 0x01, 0x00, 0x00,
    0x66, 0xff, 0x8e, 0x84, 0x00, 0x00, 0x00, 0x57,
    0xc7, 0x45, 0xfc
}; /* a lot of functions seem to share the first 
    * 16 bytes of the Windows 7 PsTerminateProcess, 
    * and a few even share the first 24 bytes.
    */

/* PspTerminateThreadByPointer */
static char PspTerminateThreadByPointer51[] =
{
    0x8b, 0xff, 0x55, 0x8b, 0xec, 0x83, 0xec, 0x0c,
    0x83, 0x4d, 0xf8, 0xff, 0x56, 0x57, 0x8b, 0x7d
};
static char PspTerminateThreadByPointer52[] =
{
    0x8b, 0xff, 0x55, 0x8b, 0xec, 0x53, 0x56, 0x57,
    0x8b, 0x7d, 0x08, 0x8d, 0xb7, 0x40, 0x02, 0x00
};
static char PspTerminateThreadByPointer60[] =
{
    0x8b, 0xff, 0x55, 0x8b, 0xec, 0x83, 0xe4, 0xf8,
    0x51, 0x53, 0x56, 0x8b, 0x75, 0x08, 0x57, 0x8d,
    0xbe, 0x60, 0x02, 0x00, 0x00, 0xf6, 0x07, 0x40
};
static char PspTerminateThreadByPointer61[] =
{
    0x8b, 0xff, 0x55, 0x8b, 0xec, 0x83, 0xe4, 0xf8,
    0x51, 0x53, 0x56, 0x8b, 0x75, 0x08, 0x57, 0x8d,
    0xbe, 0x80, 0x02, 0x00, 0x00, 0xf6, 0x07, 0x40
};

/* The following offsets took me a long time to work out, so 
   please do not steal them. If you want to use them, please 
   license your project under the GNU GPL (although you are 
   not legally required to).
 */
NTSTATUS KvInit()
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG majorVersion, minorVersion, servicePack, buildNumber;
    
    /* Get Windows version information. */
    
    RtlWindowsVersion.dwOSVersionInfoSize = sizeof(RtlWindowsVersion);
    status = RtlGetVersion((PRTL_OSVERSIONINFOW)&RtlWindowsVersion);
    
    if (!NT_SUCCESS(status))
        return status;
    
    majorVersion = RtlWindowsVersion.dwMajorVersion;
    minorVersion = RtlWindowsVersion.dwMinorVersion;
    servicePack = RtlWindowsVersion.wServicePackMajor;
    buildNumber = RtlWindowsVersion.dwBuildNumber;
    dfprintf("Windows %d.%d, SP%d.%d, build %d\n",
        majorVersion, minorVersion, servicePack,
        RtlWindowsVersion.wServicePackMinor, buildNumber
        );
    
    __NtClose = GetSystemRoutineAddress(L"NtClose");
    
    /* NtClose is used as a reference point for most addresses 
       dependent on where the kernel is loaded, so if we don't 
       have it, we can't proceed.
     */
    if (!__NtClose)
        return STATUS_NOT_SUPPORTED;
    
    /* We also need the address of ZwClose to get KiFastCallEntry. */
    __ZwClose = GetSystemRoutineAddress(L"ZwClose");
    
    if (!__ZwClose)
        return STATUS_NOT_SUPPORTED;
    
    /* Windows XP */
    if (majorVersion == 5 && minorVersion == 1)
    {
        ULONG_PTR searchOffset = (ULONG_PTR)__NtClose;
        
        WindowsVersion = WINDOWS_XP;
        ProcessAllAccess = STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xfff;
        ThreadAllAccess = STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x3ff;
        
        OffEtClientId = 0x1ec;
        OffEtSpareByteForSs = 0x256; /* Padding, last */
        OffEtStartAddress = 0x224;
        OffEtWin32StartAddress = 0x228;
        OffEpJob = 0x134;
        OffEpObjectTable = 0xc4;
        OffEpProtectedProcessOff = 0;
        OffEpProtectedProcessBit = 0;
        OffEpRundownProtect = 0x80;
        OffOhBody = 0x18;
        OffOtName = 0x40;
        OffOtiGenericMapping = 0x60 + 0x8;
        OffOtiOpenProcedure = 0x60 + 0x30;
        
        SsNtContinue = 0x20;
        
        /* KiFastCallEntry isn't hooked properly yet. Disabled for now. */
        /* INIT_SCAN(
            KiFastCallEntryScan,
            KiFastCallEntry51,
            sizeof(KiFastCallEntry51),
            (ULONG_PTR)__ZwClose, SCAN_LENGTH, -6
            ); */
        /* We are scanning for PspTerminateProcess which has 
           the same signature as PsTerminateProcess because 
           PsTerminateProcess is simply a wrapper on XP.
         */
        INIT_SCAN(
            PsTerminateProcessScan,
            PspTerminateProcess51,
            sizeof(PspTerminateProcess51),
            searchOffset, SCAN_LENGTH, 0
            );
        INIT_SCAN(
            PspTerminateThreadByPointerScan,
            PspTerminateThreadByPointer51,
            sizeof(PspTerminateThreadByPointer51),
            searchOffset, SCAN_LENGTH, 0
            );
        
        /* Windows XP SP0 and 1 are not supported */
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
    /* Windows Server 2003 */
    else if (majorVersion == 5 && minorVersion == 2)
    {
        ULONG_PTR psSearchOffset = (ULONG_PTR)GetSystemRoutineAddress(L"RtlCreateHeap");
        
        WindowsVersion = WINDOWS_SERVER_2003;
        ProcessAllAccess = STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xfff;
        ThreadAllAccess = STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x3ff;
        
        OffEtClientId = 0x1e4;
        OffEtSpareByteForSs = 0x24f; /* Padding, last */
        OffEtStartAddress = 0x21c;
        OffEtWin32StartAddress = 0x220;
        OffEpJob = 0x120;
        OffEpObjectTable = 0xd4;
        OffEpProtectedProcessOff = 0;
        OffEpProtectedProcessBit = 0;
        OffEpRundownProtect = 0x90;
        OffOhBody = 0x18;
        OffOtName = 0x40;
        OffOtiGenericMapping = 0x60 + 0x8;
        OffOtiOpenProcedure = 0x60 + 0x30;
        
        SsNtContinue = 0x22;
        
        /* Can't find on ntoskrnl *and* ntkrnlpa. Disabled for now. */
        /* INIT_SCAN(
            KiFastCallEntryScan,
            KiFastCallEntry52,
            sizeof(KiFastCallEntry52),
            (ULONG_PTR)__ZwClose, SCAN_LENGTH, -7
            ); */
        /* We are scanning for PspTerminateProcess which has 
           the same signature as PsTerminateProcess because 
           PsTerminateProcess is simply a wrapper on Server 2003.
         */
        INIT_SCAN(
            PsTerminateProcessScan,
            PspTerminateProcess52,
            sizeof(PspTerminateProcess52),
            psSearchOffset - 0x50000, SCAN_LENGTH, 0
            );
        INIT_SCAN(
            PspTerminateThreadByPointerScan,
            PspTerminateThreadByPointer52,
            sizeof(PspTerminateThreadByPointer52),
            psSearchOffset - 0x20000, SCAN_LENGTH, 0
            );
        
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
    /* Windows Vista, Windows Server 2008 */
    else if (majorVersion == 6 && minorVersion == 0)
    {
        ULONG_PTR searchOffset = (ULONG_PTR)__NtClose;
        
        WindowsVersion = WINDOWS_VISTA;
        ProcessAllAccess = STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1fff;
        ThreadAllAccess = STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xfff;
        
        OffEtClientId = 0x20c;
        OffEtSpareByteForSs = 0x26f; /* Padding, second-last */
        OffEtStartAddress = 0x1f8;
        OffEtWin32StartAddress = 0x240;
        OffEpJob = 0x10c;
        OffEpObjectTable = 0xdc;
        OffEpProtectedProcessOff = 0x224;
        OffEpProtectedProcessBit = 0xb;
        OffEpRundownProtect = 0x98;
        OffOhBody = 0x18;
        
        INIT_SCAN(
            KiFastCallEntryScan,
            KiFastCallEntry60,
            sizeof(KiFastCallEntry60),
            (ULONG_PTR)__ZwClose, SCAN_LENGTH, -7
            );
        INIT_SCAN(
            PsTerminateProcessScan,
            PsTerminateProcess60,
            sizeof(PsTerminateProcess60),
            searchOffset, SCAN_LENGTH, 0
            );
        INIT_SCAN(
            PspTerminateThreadByPointerScan,
            PspTerminateThreadByPointer60,
            sizeof(PspTerminateThreadByPointer60),
            searchOffset - 0x50000, SCAN_LENGTH, 0
            );
        
        /* SP0 */
        if (servicePack == 0)
        {
            OffOtName = 0x40;
            OffOtiGenericMapping = 0x60 + 0xc;
            OffOtiOpenProcedure = 0x60 + 0x30;
            
            SsNtContinue = 0x36;
        }
        /* SP1 */
        else if (servicePack == 1)
        {
            OffOtName = 0x8;
            OffOtiGenericMapping = 0x28 + 0xc; /* They got rid of the Mutex (an ERESOURCE) */
            OffOtiOpenProcedure = 0x28 + 0x34;
            
            SsNtContinue = 0x37;
        }
        /* SP2 */
        else if (servicePack == 2)
        {
            OffOtName = 0x8;
            OffOtiGenericMapping = 0x28 + 0xc;
            OffOtiOpenProcedure = 0x28 + 0x34;
            
            SsNtAddAtom = 0x8;
            SsNtAlertResumeThread = 0xd;
            SsNtAlertThread = 0xe;
            SsNtAllocateLocallyUniqueId = 0xf;
            SsNtAllocateUserPhysicalPages = 0x10;
            SsNtAllocateUuids = 0x11;
            SsNtAllocateVirtualMemory = 0x12;
            SsNtApphelpCacheControl = 0x28;
            SsNtAreMappedFilesTheSame = 0x29;
            SsNtAssignProcessToJobObject = 0x2a;
            SsNtCallbackReturn = 0x2b;
            SsNtCancelDeviceWakeupRequest = 0x2c;
            SsNtCancelIoFile = 0x2d;
            SsNtCancelTimer = 0x2e;
            SsNtClearEvent = 0x2f;
            SsNtClose = 0x30;
            SsNtContinue = 0x37;
            SsNtCreateDebugObject = 0x38;
            SsNtCreateDirectoryObject = 0x39;
            SsNtCreateEvent = 0x3a;
            SsNtCreateEventPair = 0x3b;
            SsNtCreateFile = 0x3c;
            SsNtCreateIoCompletion = 0x3d;
            SsNtCreateJobObject = 0x3e;
            SsNtCreateJobSet = 0x3f;
            SsNtCreateKey = 0x40;
            SsNtCreateKeyedEvent = 0x168;
            SsNtCreateMailslotFile = 0x42;
            SsNtCreateMutant = 0x43;
            SsNtCreateNamedPipeFile = 0x44;
            SsNtCreatePagingFile = 0x46;
            SsNtCreatePort = 0x47;
            SsNtCreatePrivateNamespace = 0x45;
            SsNtCreateProcess = 0x48;
            SsNtCreateProcessEx = 0x49;
            SsNtCreateProfile = 0x4a;
            SsNtCreateSection = 0x4b;
            SsNtCreateSemaphore = 0x4c;
            SsNtCreateSymbolicLinkObject = 0x4d;
            SsNtCreateThread = 0x4e;
            SsNtCreateTimer = 0x4f;
            SsNtCreateToken = 0x50;
            SsNtCreateUserProcess = 0x17f;
            SsNtCreateWaitablePort = 0x73;
            SsNtDebugActiveProcess = 0x74;
            SsNtDebugContinue = 0x75;
            SsNtDelayExecution = 0x76;
            SsNtDeleteAtom = 0x77;
            SsNtDeleteBootEntry = 0x78;
            SsNtDeleteDriverEntry = 0x79;
            SsNtDeleteFile = 0x7a;
            SsNtDeleteKey = 0x7b;
            SsNtDeletePrivateNamespace = 0x7c;
            SsNtDeleteObjectAuditAlarm = 0x7d;
            SsNtDeleteValueKey = 0x7e;
            SsNtDeviceIoControlFile = 0x7f;
            SsNtDisplayString = 0x80;
            SsNtDuplicateObject = 0x81;
            SsNtDuplicateToken = 0x82;
            SsNtEnumerateBootEntries = 0x83;
            SsNtEnumerateDriverEntries = 0x84;
            SsNtEnumerateKey = 0x85;
            SsNtEnumerateSystemEnvironmentValuesEx = 0x86;
            SsNtEnumerateValueKey = 0x88;
            SsNtExtendSection = 0x89;
            SsNtFilterToken = 0x8a;
            SsNtFindAtom = 0x8b;
            SsNtFlushBuffersFile = 0x8c;
            SsNtFlushInstructionCache = 0x8d;
            SsNtFlushKey = 0x8e;
            SsNtFlushProcessWriteBuffers = 0x8f;
            SsNtFlushVirtualMemory = 0x90;
            SsNtFlushWriteBuffer = 0x91;
            SsNtFreeUserPhysicalPages = 0x92;
            SsNtFreeVirtualMemory = 0x93;
            SsNtFsControlFile = 0x96;
            SsNtGetContextThread = 0x97;
            SsNtGetDevicePowerState = 0x98;
            SsNtGetPlugPlayEvent = 0x9a;
            SsNtGetWriteWatch = 0x9b;
            SsNtImpersonateAnonymousToken = 0x9c;
            SsNtImpersonateClientOfPort = 0x9d;
            SsNtImpersonateThread = 0x9e;
            SsNtInitiatePowerAction = 0xa1;
            SsNtIsProcessInJob = 0xa2;
            SsNtIsSystemResumeAutomatic = 0xa3;
            SsNtListenPort = 0xa4;
            SsNtLoadDriver = 0xa5;
            SsNtLoadKey = 0xa6;
            SsNtLoadKey2 = 0xa7;
            SsNtLockFile = 0xa9;
            SsNtLockVirtualMemory = 0xac;
            SsNtMakePermanentObject = 0xad;
            SsNtMakeTemporaryObject = 0xae;
            SsNtMapUserPhysicalPages = 0xaf;
            SsNtMapUserPhysicalPagesScatter = 0xb0;
            SsNtMapViewOfSection = 0xb1;
            SsNtModifyBootEntry = 0xb2;
            SsNtModifyDriverEntry = 0xb3;
            SsNtNotifyChangeDirectoryFile = 0xb4;
            SsNtNotifyChangeKey = 0xb5;
            SsNtNotifyChangeMultipleKeys = 0xb6;
            SsNtOpenDirectoryObject = 0xb7;
            SsNtOpenEvent = 0xb8;
            SsNtOpenEventPair = 0xb9;
            SsNtOpenFile = 0xba;
            SsNtOpenIoCompletion = 0xbb;
            SsNtOpenJobObject = 0xbc;
            SsNtOpenKey = 0xbd;
            SsNtOpenKeyedEvent = 0x169;
            SsNtOpenMutant = 0xbf;
            SsNtOpenObjectAuditAlarm = 0xc1;
            SsNtOpenProcess = 0xc2;
            SsNtOpenProcessToken = 0xc3;
            SsNtOpenProcessTokenEx = 0xc4;
            SsNtOpenSection = 0xc5;
            SsNtOpenSemaphore = 0xc6;
            SsNtOpenSymbolicLinkObject = 0xc8;
            SsNtOpenThread = 0xc9;
            SsNtOpenThreadToken = 0xca;
            SsNtOpenThreadTokenEx = 0xcb;
            SsNtOpenTimer = 0xcc;
            SsNtReadFile = 0x102;
            SsNtWriteFile = 0x163;
        }
        else
        {
            return STATUS_NOT_SUPPORTED;
        }
        
        dprintf("Initialized version-specific data for Windows Vista SP%d/Windows Server 2008\n", servicePack);
    }
    /* Windows 7, Windows Server 2008 R2 */
    else if (majorVersion == 6 && minorVersion == 1)
    {
        ULONG_PTR psSearchOffset = (ULONG_PTR)GetSystemRoutineAddress(L"PsSetCreateProcessNotifyRoutine");
        ULONG psScanLength = 0x200000;
        
        if (!psSearchOffset)
            return STATUS_NOT_SUPPORTED;
        
        WindowsVersion = WINDOWS_7;
        ProcessAllAccess = STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1fff;
        ThreadAllAccess = STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xfff;
        
        OffEtClientId = 0x22c;
        OffEtSpareByteForSs = 0x2b4; /* Padding, last */
        OffEtStartAddress = 0x218;
        OffEtWin32StartAddress = 0x260;
        OffEpJob = 0x124;
        OffEpObjectTable = 0xf4;
        OffEpProtectedProcessOff = 0x26c;
        OffEpProtectedProcessBit = 0xb;
        OffEpRundownProtect = 0xb0;
        OffOhBody = 0x18;
        OffOtName = 0x8;
        OffOtiGenericMapping = 0x28 + 0xc;
        OffOtiOpenProcedure = 0x28 + 0x34;
        
        SsNtContinue = 0x3c;
        
        INIT_SCAN(
            KiFastCallEntryScan,
            KiFastCallEntry61,
            sizeof(KiFastCallEntry61),
            (ULONG_PTR)__ZwClose, SCAN_LENGTH, -7
            );
        INIT_SCAN(
            PsTerminateProcessScan,
            PsTerminateProcess61,
            sizeof(PsTerminateProcess61),
            psSearchOffset, psScanLength, 0
            );
        INIT_SCAN(
            PspTerminateThreadByPointerScan,
            PspTerminateThreadByPointer61,
            sizeof(PspTerminateThreadByPointer61),
            psSearchOffset, psScanLength, 0
            );
        
        /* SP0 */
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
    
    return status;
}

PVOID KvScanProc(
    PKV_SCANPROC ScanProc
    )
{
    PUCHAR bytes = ScanProc->Bytes;
    ULONG length = ScanProc->Length;
    ULONG_PTR endAddress = ScanProc->StartAddress + ScanProc->ScanLength;
    ULONG_PTR i;
    
    for (i = ScanProc->StartAddress; i < endAddress; i++)
    {
        if (memcmp((PVOID)i, bytes, length) == 0)
            return (PVOID)(i + ScanProc->Displacement);
    }
    
    return NULL;
}

PVOID KvVerifyPrologue(
    PVOID Address
    )
{
    if (memcmp(Address, StandardPrologue, 5) == 0)
        return Address;
    else
        return NULL;
}
