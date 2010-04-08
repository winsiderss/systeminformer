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

#ifndef _VERSION_H
#define _VERSION_H

#include "kph.h"

#define WINDOWS_XP 51
#define WINDOWS_SERVER_2003 52
#define WINDOWS_VISTA 60
#define WINDOWS_7 61

#define KVOFF(object, offset) ((PCHAR)(object) + offset)
#define SCAN_LENGTH 0x100000
#define INIT_SCAN(scan, bytes, length, address, scanLength, displacement) \
    ( \
    ((scan).Initialized = TRUE), \
    ((scan).Bytes = (bytes)), \
    ((scan).Length = (length)), \
    ((scan).StartAddress = (address)), \
    ((scan).ScanLength = (scanLength)), \
    ((scan).Displacement = (displacement)), \
    bytes \
    )

typedef struct _KV_SCANPROC
{
    BOOLEAN Initialized;
    PUCHAR Bytes;
    ULONG Length;
    ULONG_PTR StartAddress;
    ULONG ScanLength;
    LONG Displacement;
} KV_SCANPROC, *PKV_SCANPROC;

NTSTATUS KvInit();

PVOID KvScanProc(
    PKV_SCANPROC ScanProc
    );

PVOID KvVerifyPrologue(
    PVOID Address
    );

#ifdef EXT
#undef EXT
#endif

#ifdef _VERSION_PRIVATE
#define EXT
#define SCANNULL = { FALSE, NULL, 0, 0, 0, 0 }
#else
#define EXT extern
#define SCANNULL
#endif

EXT ULONG WindowsVersion;
EXT RTL_OSVERSIONINFOEXW RtlWindowsVersion;
EXT ACCESS_MASK ProcessAllAccess;
EXT ACCESS_MASK ThreadAllAccess;

/* Offsets */
/* Structures
 * Et: ETHREAD
 * Ep: EPROCESS
 * Ot: OBJECT_TYPE
 * Oti: OBJECT_TYPE_INITIALIZER, offset measured from an OBJECT_TYPE
 */
EXT ULONG OffEtClientId;
EXT ULONG OffEtSpareByteForSs;
EXT ULONG OffEtStartAddress;
EXT ULONG OffEtWin32StartAddress;
EXT ULONG OffEpJob;
EXT ULONG OffEpObjectTable;
EXT ULONG OffEpProtectedProcessOff;
EXT ULONG OffEpProtectedProcessBit;
EXT ULONG OffEpRundownProtect;
EXT ULONG OffOhBody;
EXT ULONG OffOtName;
EXT ULONG OffOtiGenericMapping;
EXT ULONG OffOtiOpenProcedure;

/* Functions
 */
EXT KV_SCANPROC KiFastCallEntryScan SCANNULL;
EXT KV_SCANPROC PsExitSpecialApcScan SCANNULL;
EXT KV_SCANPROC PsTerminateProcessScan SCANNULL;
EXT KV_SCANPROC PspTerminateThreadByPointerScan SCANNULL;

/* System Call Numbers
 */
EXT ULONG SsNtAddAtom;
EXT ULONG SsNtAlertResumeThread;
EXT ULONG SsNtAlertThread;
EXT ULONG SsNtAllocateLocallyUniqueId;
EXT ULONG SsNtAllocateUserPhysicalPages;
EXT ULONG SsNtAllocateUuids;
EXT ULONG SsNtAllocateVirtualMemory;
EXT ULONG SsNtApphelpCacheControl;
EXT ULONG SsNtAreMappedFilesTheSame;
EXT ULONG SsNtAssignProcessToJobObject;
EXT ULONG SsNtCallbackReturn;
EXT ULONG SsNtCancelDeviceWakeupRequest;
EXT ULONG SsNtCancelIoFile;
EXT ULONG SsNtCancelTimer;
EXT ULONG SsNtClearEvent;
EXT ULONG SsNtClose;
EXT ULONG SsNtContinue;
EXT ULONG SsNtCreateDebugObject;
EXT ULONG SsNtCreateDirectoryObject;
EXT ULONG SsNtCreateEvent;
EXT ULONG SsNtCreateEventPair;
EXT ULONG SsNtCreateFile;
EXT ULONG SsNtCreateIoCompletion;
EXT ULONG SsNtCreateJobObject;
EXT ULONG SsNtCreateJobSet;
EXT ULONG SsNtCreateKey;
EXT ULONG SsNtCreateKeyedEvent;
EXT ULONG SsNtCreateMailslotFile;
EXT ULONG SsNtCreateMutant;
EXT ULONG SsNtCreateNamedPipeFile;
EXT ULONG SsNtCreatePagingFile;
EXT ULONG SsNtCreatePort;
EXT ULONG SsNtCreatePrivateNamespace;
EXT ULONG SsNtCreateProcess;
EXT ULONG SsNtCreateProcessEx;
EXT ULONG SsNtCreateProfile;
EXT ULONG SsNtCreateSection;
EXT ULONG SsNtCreateSemaphore;
EXT ULONG SsNtCreateSymbolicLinkObject;
EXT ULONG SsNtCreateThread;
EXT ULONG SsNtCreateTimer;
EXT ULONG SsNtCreateToken;
EXT ULONG SsNtCreateUserProcess;
EXT ULONG SsNtCreateWaitablePort;
EXT ULONG SsNtDebugActiveProcess;
EXT ULONG SsNtDebugContinue;
EXT ULONG SsNtDelayExecution;
EXT ULONG SsNtDeleteAtom;
EXT ULONG SsNtDeleteBootEntry;
EXT ULONG SsNtDeleteDriverEntry;
EXT ULONG SsNtDeleteFile;
EXT ULONG SsNtDeleteKey;
EXT ULONG SsNtDeleteObjectAuditAlarm;
EXT ULONG SsNtDeletePrivateNamespace;
EXT ULONG SsNtDeleteValueKey;
EXT ULONG SsNtDeviceIoControlFile;
EXT ULONG SsNtDisplayString;
EXT ULONG SsNtDuplicateObject;
EXT ULONG SsNtDuplicateToken;
EXT ULONG SsNtEnumerateBootEntries;
EXT ULONG SsNtEnumerateDriverEntries;
EXT ULONG SsNtEnumerateKey;
EXT ULONG SsNtEnumerateSystemEnvironmentValuesEx;
EXT ULONG SsNtEnumerateValueKey;
EXT ULONG SsNtExtendSection;
EXT ULONG SsNtFilterToken;
EXT ULONG SsNtFindAtom;
EXT ULONG SsNtFlushBuffersFile;
EXT ULONG SsNtFlushInstructionCache;
EXT ULONG SsNtFlushKey;
EXT ULONG SsNtFlushProcessWriteBuffers;
EXT ULONG SsNtFlushVirtualMemory;
EXT ULONG SsNtFlushWriteBuffer;
EXT ULONG SsNtFreeUserPhysicalPages;
EXT ULONG SsNtFreeVirtualMemory;
EXT ULONG SsNtFsControlFile;
EXT ULONG SsNtGetContextThread;
EXT ULONG SsNtGetCurrentProcessorNumber;
EXT ULONG SsNtGetDevicePowerState;
EXT ULONG SsNtGetNextProcess;
EXT ULONG SsNtGetNextThread;
EXT ULONG SsNtGetPlugPlayEvent;
EXT ULONG SsNtGetWriteWatch;
EXT ULONG SsNtImpersonateAnonymousToken;
EXT ULONG SsNtImpersonateClientOfPort;
EXT ULONG SsNtImpersonateThread;
EXT ULONG SsNtInitiatePowerAction;
EXT ULONG SsNtIsProcessInJob;
EXT ULONG SsNtIsSystemResumeAutomatic;
EXT ULONG SsNtListenPort;
EXT ULONG SsNtLoadDriver;
EXT ULONG SsNtLoadKey;
EXT ULONG SsNtLoadKey2;
EXT ULONG SsNtLockFile;
EXT ULONG SsNtLockVirtualMemory;
EXT ULONG SsNtMakePermanentObject;
EXT ULONG SsNtMakeTemporaryObject;
EXT ULONG SsNtMapUserPhysicalPages;
EXT ULONG SsNtMapUserPhysicalPagesScatter;
EXT ULONG SsNtMapViewOfSection;
EXT ULONG SsNtModifyBootEntry;
EXT ULONG SsNtModifyDriverEntry;
EXT ULONG SsNtNotifyChangeDirectoryFile;
EXT ULONG SsNtNotifyChangeKey;
EXT ULONG SsNtNotifyChangeMultipleKeys;
EXT ULONG SsNtOpenDirectoryObject;
EXT ULONG SsNtOpenEvent;
EXT ULONG SsNtOpenEventPair;
EXT ULONG SsNtOpenFile;
EXT ULONG SsNtOpenIoCompletion;
EXT ULONG SsNtOpenJobObject;
EXT ULONG SsNtOpenKey;
EXT ULONG SsNtOpenKeyedEvent;
EXT ULONG SsNtOpenMutant;
EXT ULONG SsNtOpenObjectAuditAlarm;
EXT ULONG SsNtOpenProcess;
EXT ULONG SsNtOpenProcessToken;
EXT ULONG SsNtOpenProcessTokenEx;
EXT ULONG SsNtOpenSection;
EXT ULONG SsNtOpenSemaphore;
EXT ULONG SsNtOpenSymbolicLinkObject;
EXT ULONG SsNtOpenThread;
EXT ULONG SsNtOpenThreadToken;
EXT ULONG SsNtOpenThreadTokenEx;
EXT ULONG SsNtOpenTimer;
EXT ULONG SsNtReadFile;
EXT ULONG SsNtWriteFile;

#endif
