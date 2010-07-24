/*
 * Process Hacker - 
 *   process provider
 * 
 * Copyright (C) 2009-2010 wj32
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

#define PROCPRV_PRIVATE
#include <phapp.h>
#include <kph.h>
#include <wtsapi32.h>

VOID PhpQueueProcessQueryStage1(
    __in PPH_PROCESS_ITEM ProcessItem
    );

VOID PhpQueueProcessQueryStage2(
    __in PPH_PROCESS_ITEM ProcessItem
    );

PPH_PROCESS_RECORD PhpCreateProcessRecord(
    __in PPH_PROCESS_ITEM ProcessItem
    );

VOID PhpAddProcessRecord(
    __inout PPH_PROCESS_RECORD ProcessRecord
    );

VOID PhpRemoveProcessRecord(
    __inout PPH_PROCESS_RECORD ProcessRecord
    );

typedef struct _PH_PROCESS_QUERY_DATA
{
    SLIST_ENTRY ListEntry;
    ULONG Stage;
    PPH_PROCESS_ITEM ProcessItem;
} PH_PROCESS_QUERY_DATA, *PPH_PROCESS_QUERY_DATA;

typedef struct _PH_PROCESS_QUERY_S1_DATA
{
    PH_PROCESS_QUERY_DATA Header;

    PPH_STRING CommandLine;

    HICON SmallIcon;
    HICON LargeIcon;
    PH_IMAGE_VERSION_INFO VersionInfo;

    TOKEN_ELEVATION_TYPE ElevationType;
    BOOLEAN IsElevated;
    PH_INTEGRITY IntegrityLevel;
    PPH_STRING IntegrityString;

    PPH_STRING JobName;
    BOOLEAN IsInJob;
    BOOLEAN IsInSignificantJob;

    HANDLE ConsoleHostProcessId;

    BOOLEAN IsPosix;
    BOOLEAN IsWow64;
} PH_PROCESS_QUERY_S1_DATA, *PPH_PROCESS_QUERY_S1_DATA;

typedef struct _PH_PROCESS_QUERY_S2_DATA
{
    PH_PROCESS_QUERY_DATA Header;

    BOOLEAN IsDotNet;

    VERIFY_RESULT VerifyResult;
    PPH_STRING VerifySignerName;

    BOOLEAN IsPacked;
    ULONG ImportFunctions;
    ULONG ImportModules;
} PH_PROCESS_QUERY_S2_DATA, *PPH_PROCESS_QUERY_S2_DATA;

VOID NTAPI PhpProcessItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

BOOLEAN NTAPI PhpProcessHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    );

ULONG NTAPI PhpProcessHashtableHashFunction(
    __in PVOID Entry
    );

VOID PhpUpdatePerfInformation();

VOID PhpUpdateCpuInformation(
    __out PULONG64 TotalTime
    );

PPH_OBJECT_TYPE PhProcessItemType;

PPH_HASHTABLE PhProcessHashtable;
PH_QUEUED_LOCK PhProcessHashtableLock = PH_QUEUED_LOCK_INIT;

SLIST_HEADER PhProcessQueryDataListHead;

PHAPPAPI PH_CALLBACK PhProcessAddedEvent;
PHAPPAPI PH_CALLBACK PhProcessModifiedEvent;
PHAPPAPI PH_CALLBACK PhProcessRemovedEvent;
PHAPPAPI PH_CALLBACK PhProcessesUpdatedEvent;

PPH_LIST PhProcessRecordList;
PH_QUEUED_LOCK PhProcessRecordListLock = PH_QUEUED_LOCK_INIT;

ULONG PhStatisticsSampleCount = 512;
BOOLEAN PhEnableProcessQueryStage2 = FALSE;

SYSTEM_PERFORMANCE_INFORMATION PhPerfInformation;
PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PhCpuInformation;
SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PhCpuTotals;
ULONG PhTotalProcesses;
ULONG PhTotalThreads;
ULONG PhTotalHandles;

SYSTEM_PROCESS_INFORMATION PhDpcsProcessInformation;
SYSTEM_PROCESS_INFORMATION PhInterruptsProcessInformation;

FLOAT PhCpuKernelUsage;
FLOAT PhCpuUserUsage;
PFLOAT PhCpusKernelUsage;
PFLOAT PhCpusUserUsage;

PH_UINT64_DELTA PhCpuKernelDelta;
PH_UINT64_DELTA PhCpuUserDelta;
PH_UINT64_DELTA PhCpuOtherDelta;

PPH_UINT64_DELTA PhCpusKernelDelta;
PPH_UINT64_DELTA PhCpusUserDelta;
PPH_UINT64_DELTA PhCpusOtherDelta;

PH_UINT64_DELTA PhIoReadDelta;
PH_UINT64_DELTA PhIoWriteDelta;
PH_UINT64_DELTA PhIoOtherDelta;

static BOOLEAN PhProcessStatisticsInitialized = FALSE;
static ULONG PhTimeSequenceNumber = 0;
static PH_CIRCULAR_BUFFER_ULONG PhTimeHistory;

PH_CIRCULAR_BUFFER_FLOAT PhCpuKernelHistory;
PH_CIRCULAR_BUFFER_FLOAT PhCpuUserHistory;
//PH_CIRCULAR_BUFFER_FLOAT PhCpuOtherHistory;

PPH_CIRCULAR_BUFFER_FLOAT PhCpusKernelHistory;
PPH_CIRCULAR_BUFFER_FLOAT PhCpusUserHistory;
//PPH_CIRCULAR_BUFFER_FLOAT PhCpusOtherHistory;

PH_CIRCULAR_BUFFER_ULONG64 PhIoReadHistory;
PH_CIRCULAR_BUFFER_ULONG64 PhIoWriteHistory;
PH_CIRCULAR_BUFFER_ULONG64 PhIoOtherHistory;

PH_CIRCULAR_BUFFER_ULONG PhCommitHistory;
PH_CIRCULAR_BUFFER_ULONG PhPhysicalHistory;

PH_CIRCULAR_BUFFER_ULONG PhMaxCpuHistory; // ID of max. CPU process
PH_CIRCULAR_BUFFER_ULONG PhMaxIoHistory; // ID of max. I/O process
#ifdef PH_RECORD_MAX_USAGE
PH_CIRCULAR_BUFFER_FLOAT PhMaxCpuUsageHistory;
PH_CIRCULAR_BUFFER_ULONG64 PhMaxIoReadOtherHistory;
PH_CIRCULAR_BUFFER_ULONG64 PhMaxIoWriteHistory;
#endif

static PWTS_PROCESS_INFO PhpWtsProcesses = NULL;
static ULONG PhpWtsNumberOfProcesses;

BOOLEAN PhProcessProviderInitialization()
{
    PFLOAT usageBuffer;
    PPH_UINT64_DELTA deltaBuffer;
    PPH_CIRCULAR_BUFFER_FLOAT historyBuffer;
    ULONG i;

    if (!NT_SUCCESS(PhCreateObjectType(
        &PhProcessItemType,
        L"ProcessItem",
        0,
        PhpProcessItemDeleteProcedure
        )))
        return FALSE;

    PhProcessHashtable = PhCreateHashtable(
        sizeof(PPH_PROCESS_ITEM),
        PhpProcessHashtableCompareFunction,
        PhpProcessHashtableHashFunction,
        40
        );

    RtlInitializeSListHead(&PhProcessQueryDataListHead);

    PhInitializeCallback(&PhProcessAddedEvent);
    PhInitializeCallback(&PhProcessModifiedEvent);
    PhInitializeCallback(&PhProcessRemovedEvent);
    PhInitializeCallback(&PhProcessesUpdatedEvent);

    PhProcessRecordList = PhCreateList(40);

    RtlInitUnicodeString(
        &PhDpcsProcessInformation.ImageName,
        L"DPCs"
        );
    PhDpcsProcessInformation.UniqueProcessId = DPCS_PROCESS_ID;
    PhDpcsProcessInformation.InheritedFromUniqueProcessId = SYSTEM_IDLE_PROCESS_ID;

    RtlInitUnicodeString(
        &PhInterruptsProcessInformation.ImageName,
        L"Interrupts"
        );
    PhInterruptsProcessInformation.UniqueProcessId = INTERRUPTS_PROCESS_ID;
    PhInterruptsProcessInformation.InheritedFromUniqueProcessId = SYSTEM_IDLE_PROCESS_ID;

    PhCpuInformation = PhAllocate(
        sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) *
        (ULONG)PhSystemBasicInformation.NumberOfProcessors
        );
    usageBuffer = PhAllocate(
        sizeof(FLOAT) *
        (ULONG)PhSystemBasicInformation.NumberOfProcessors *
        2
        );
    deltaBuffer = PhAllocate(
        sizeof(PH_UINT64_DELTA) *
        (ULONG)PhSystemBasicInformation.NumberOfProcessors *
        3
        );
    historyBuffer = PhAllocate(
        sizeof(PH_CIRCULAR_BUFFER_FLOAT) *
        (ULONG)PhSystemBasicInformation.NumberOfProcessors *
        2
        );

    PhCpusKernelUsage = usageBuffer;
    PhCpusUserUsage = PhCpusKernelUsage + (ULONG)PhSystemBasicInformation.NumberOfProcessors;

    PhCpusKernelDelta = deltaBuffer;
    PhCpusUserDelta = PhCpusKernelDelta + (ULONG)PhSystemBasicInformation.NumberOfProcessors;
    PhCpusOtherDelta = PhCpusUserDelta + (ULONG)PhSystemBasicInformation.NumberOfProcessors;

    PhCpusKernelHistory = historyBuffer;
    PhCpusUserHistory = PhCpusKernelHistory + (ULONG)PhSystemBasicInformation.NumberOfProcessors;

    PhInitializeDelta(&PhCpuKernelDelta);
    PhInitializeDelta(&PhCpuUserDelta);
    PhInitializeDelta(&PhCpuOtherDelta);

    for (i = 0; i < (ULONG)PhSystemBasicInformation.NumberOfProcessors; i++)
    {
        PhInitializeDelta(&PhCpusKernelDelta[i]);
        PhInitializeDelta(&PhCpusUserDelta[i]);
        PhInitializeDelta(&PhCpusOtherDelta[i]);
    }

    return TRUE;
}

PPH_STRING PhGetClientIdName(
    __in PCLIENT_ID ClientId
    )
{
    PPH_STRING name;
    PPH_STRING processName = NULL;
    PPH_PROCESS_ITEM processItem;

    processItem = PhReferenceProcessItem(ClientId->UniqueProcess);

    if (processItem)
    {
        processName = processItem->ProcessName;
        PhReferenceObject(processName);
        PhDereferenceObject(processItem);
    }

    if (ClientId->UniqueThread)
    {
        if (processName)
        {
            name = PhFormatString(L"%s (%u): %u", processName->Buffer,
                (ULONG)ClientId->UniqueProcess, (ULONG)ClientId->UniqueThread);
        }
        else
        {
            name = PhFormatString(L"Non-existent process (%u): %u",
                (ULONG)ClientId->UniqueProcess, (ULONG)ClientId->UniqueThread);
        }
    }
    else
    {
        if (processName)
            name = PhFormatString(L"%s (%u)", processName->Buffer, (ULONG)ClientId->UniqueProcess);
        else
            name = PhFormatString(L"Non-existent process (%u)", (ULONG)ClientId->UniqueProcess);
    }

    if (processName)
        PhDereferenceObject(processName);

    return name;
}

PWSTR PhGetProcessPriorityClassWin32String(
    __in ULONG PriorityClassWin32
    )
{
    switch (PriorityClassWin32)
    {
    case REALTIME_PRIORITY_CLASS:
        return L"Real Time";
    case HIGH_PRIORITY_CLASS:
        return L"High";
    case ABOVE_NORMAL_PRIORITY_CLASS:
        return L"Above Normal";
    case NORMAL_PRIORITY_CLASS:
        return L"Normal";
    case BELOW_NORMAL_PRIORITY_CLASS:
        return L"Below Normal";
    case IDLE_PRIORITY_CLASS:
        return L"Idle";
    default:
        return L"Unknown";
    }
}

PPH_PROCESS_ITEM PhCreateProcessItem(
    __in HANDLE ProcessId
    )
{
    PPH_PROCESS_ITEM processItem;

    if (!NT_SUCCESS(PhCreateObject(
        &processItem,
        sizeof(PH_PROCESS_ITEM),
        0,
        PhProcessItemType,
        0
        )))
        return NULL;

    memset(processItem, 0, sizeof(PH_PROCESS_ITEM));
    PhInitializeEvent(&processItem->Stage1Event);
    processItem->ServiceList = PhCreatePointerList(1);
    PhInitializeQueuedLock(&processItem->ServiceListLock);

    processItem->ProcessId = ProcessId;

    if (!PH_IS_FAKE_PROCESS_ID(ProcessId))
        PhPrintUInt32(processItem->ProcessIdString, (ULONG)ProcessId);

    // Create the statistics buffers.
    PhInitializeCircularBuffer_FLOAT(&processItem->CpuKernelHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_FLOAT(&processItem->CpuUserHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_ULONG64(&processItem->IoReadHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_ULONG64(&processItem->IoWriteHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_ULONG64(&processItem->IoOtherHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_SIZE_T(&processItem->PrivateBytesHistory, PhStatisticsSampleCount);
    //PhInitializeCircularBuffer_SIZE_T(&processItem->WorkingSetHistory, PhStatisticsSampleCount);

    return processItem;
}

VOID PhpProcessItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Object;
    ULONG i;

    PhDeleteCircularBuffer_FLOAT(&processItem->CpuKernelHistory);
    PhDeleteCircularBuffer_FLOAT(&processItem->CpuUserHistory);
    PhDeleteCircularBuffer_ULONG64(&processItem->IoReadHistory);
    PhDeleteCircularBuffer_ULONG64(&processItem->IoWriteHistory);
    PhDeleteCircularBuffer_ULONG64(&processItem->IoOtherHistory);
    PhDeleteCircularBuffer_SIZE_T(&processItem->PrivateBytesHistory);
    //PhDeleteCircularBuffer_SIZE_T(&processItem->WorkingSetHistory);

    for (i = 0; i < processItem->ServiceList->Count; i++)
        PhDereferenceObject(processItem->ServiceList->Items[i]);

    PhDereferenceObject(processItem->ServiceList);

    if (processItem->ProcessName) PhDereferenceObject(processItem->ProcessName);
    if (processItem->FileName) PhDereferenceObject(processItem->FileName);
    if (processItem->CommandLine) PhDereferenceObject(processItem->CommandLine);
    if (processItem->SmallIcon) DestroyIcon(processItem->SmallIcon);
    if (processItem->LargeIcon) DestroyIcon(processItem->LargeIcon);
    PhDeleteImageVersionInfo(&processItem->VersionInfo);
    if (processItem->UserName) PhDereferenceObject(processItem->UserName);
    if (processItem->VerifySignerName) PhDereferenceObject(processItem->VerifySignerName);

    if (processItem->QueryHandle) NtClose(processItem->QueryHandle);

    if (processItem->Record) PhDereferenceProcessRecord(processItem->Record);
}

BOOLEAN PhpProcessHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    return
        (*(PPH_PROCESS_ITEM *)Entry1)->ProcessId ==
        (*(PPH_PROCESS_ITEM *)Entry2)->ProcessId;
}

ULONG PhpProcessHashtableHashFunction(
    __in PVOID Entry
    )
{
    return (ULONG)(*(PPH_PROCESS_ITEM *)Entry)->ProcessId / 4;
}

__assumeLocked PPH_PROCESS_ITEM PhpLookupProcessItem(
    __in HANDLE ProcessId
    )
{
    PH_PROCESS_ITEM lookupProcessItem;
    PPH_PROCESS_ITEM lookupProcessItemPtr = &lookupProcessItem;
    PPH_PROCESS_ITEM *processItemPtr;

    // Construct a temporary process item for the lookup.
    lookupProcessItem.ProcessId = ProcessId;

    processItemPtr = (PPH_PROCESS_ITEM *)PhGetHashtableEntry(
        PhProcessHashtable,
        &lookupProcessItemPtr
        );

    if (processItemPtr)
        return *processItemPtr;
    else
        return NULL;
}

PPH_PROCESS_ITEM PhReferenceProcessItem(
    __in HANDLE ProcessId
    )
{
    PPH_PROCESS_ITEM processItem;

    PhAcquireQueuedLockShared(&PhProcessHashtableLock);

    processItem = PhpLookupProcessItem(ProcessId);

    if (processItem)
        PhReferenceObject(processItem);

    PhReleaseQueuedLockShared(&PhProcessHashtableLock);

    return processItem;
}

VOID PhEnumProcessItems(
    __out_opt PPH_PROCESS_ITEM **ProcessItems,
    __out PULONG NumberOfProcessItems
    )
{
    PPH_PROCESS_ITEM *processItems;
    PPH_PROCESS_ITEM *processItem;
    ULONG numberOfProcessItems;
    ULONG enumerationKey = 0;
    ULONG i = 0;

    if (!ProcessItems)
    {
        *NumberOfProcessItems = PhProcessHashtable->Count;
        return;
    }

    PhAcquireQueuedLockShared(&PhProcessHashtableLock);

    numberOfProcessItems = PhProcessHashtable->Count;
    processItems = PhAllocate(sizeof(PPH_PROCESS_ITEM) * numberOfProcessItems);

    while (PhEnumHashtable(PhProcessHashtable, (PPVOID)&processItem, &enumerationKey))
    {
        PhReferenceObject(*processItem);
        processItems[i++] = *processItem;
    }

    PhReleaseQueuedLockShared(&PhProcessHashtableLock);

    *ProcessItems = processItems;
    *NumberOfProcessItems = numberOfProcessItems;
}

__assumeLocked VOID PhpRemoveProcessItem(
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    PhRemoveHashtableEntry(PhProcessHashtable, &ProcessItem);
    PhDereferenceObject(ProcessItem);
}

VOID PhpProcessQueryStage1(
    __inout PPH_PROCESS_QUERY_S1_DATA Data
    )
{
    NTSTATUS status;
    PPH_PROCESS_ITEM processItem = Data->Header.ProcessItem;
    HANDLE processId = processItem->ProcessId;
    HANDLE processHandleLimited = NULL;

    PhOpenProcess(&processHandleLimited, ProcessQueryAccess, processId);

    if (processItem->FileName)
    {
        // Small icon, large icon.
        {
            SHFILEINFO fileInfo;

            if (SHGetFileInfo(
                processItem->FileName->Buffer,
                0,
                &fileInfo,
                sizeof(SHFILEINFO),
                SHGFI_ICON | SHGFI_SMALLICON
                ))
                Data->SmallIcon = fileInfo.hIcon;

            if (SHGetFileInfo(
                processItem->FileName->Buffer,
                0,
                &fileInfo,
                sizeof(SHFILEINFO),
                SHGFI_ICON | SHGFI_LARGEICON
                ))
                Data->LargeIcon = fileInfo.hIcon;
        }

        // Version info.
        PhInitializeImageVersionInfo(&Data->VersionInfo, processItem->FileName->Buffer);
    }

    // Use the default EXE icon if we didn't get the file's icon.
    {
        SHFILEINFO fileInfo;

        if (!Data->SmallIcon)
        {
            if (SHGetFileInfo(
                L".exe",
                FILE_ATTRIBUTE_NORMAL,
                &fileInfo,
                sizeof(SHFILEINFO),
                SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES
                ))
                Data->SmallIcon = fileInfo.hIcon;
        }

        if (!Data->LargeIcon)
        {
            if (SHGetFileInfo(
                L".exe",
                FILE_ATTRIBUTE_NORMAL,
                &fileInfo,
                sizeof(SHFILEINFO),
                SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES
                ))
                Data->LargeIcon = fileInfo.hIcon;
        }
    }

    // POSIX, command line
    {
        HANDLE processHandle;

        status = PhOpenProcess(
            &processHandle,
            ProcessQueryAccess | PROCESS_VM_READ,
            processId
            );

        if (NT_SUCCESS(status))
        {
            BOOLEAN isPosix = FALSE;
            PPH_STRING commandLine;
            ULONG i;

            status = PhGetProcessIsPosix(processHandle, &isPosix);
            Data->IsPosix = isPosix;

            if (!NT_SUCCESS(status) || !isPosix)
            {
                status = PhGetProcessCommandLine(processHandle, &commandLine);

                if (NT_SUCCESS(status))
                {
                    // Some command lines (e.g. from taskeng.exe) have nulls in them. 
                    // Since Windows can't display them, we'll replace them with 
                    // spaces.
                    for (i = 0; i < (ULONG)commandLine->Length / 2; i++)
                    {
                        if (commandLine->Buffer[i] == 0)
                            commandLine->Buffer[i] = ' ';
                    }
                }
            }
            else
            {
                // Get the POSIX command line.
                status = PhGetProcessPosixCommandLine(processHandle, &commandLine);
            }

            if (NT_SUCCESS(status))
            {
                Data->CommandLine = commandLine;
            }

            NtClose(processHandle);
        }
    }

    // Token information
    if (processHandleLimited)
    {
        if (WINDOWS_HAS_UAC)
        {
            HANDLE tokenHandle;

            status = PhOpenProcessToken(&tokenHandle, TOKEN_QUERY, processHandleLimited);

            if (NT_SUCCESS(status))
            {
                // Elevation
                if (NT_SUCCESS(PhGetTokenElevationType(
                    tokenHandle,
                    &Data->ElevationType
                    )))
                {
                    Data->IsElevated = Data->ElevationType == TokenElevationTypeFull;
                }

                // Integrity
                PhGetTokenIntegrityLevel(
                    tokenHandle,
                    &Data->IntegrityLevel,
                    &Data->IntegrityString
                    );

                NtClose(tokenHandle);
            }
        }

#ifdef _M_X64
        // WOW64
        PhGetProcessIsWow64(processHandleLimited, &Data->IsWow64);
#endif
    }

    // Job
    if (processHandleLimited)
    {
        if (PhKphHandle)
        {
            HANDLE jobHandle = NULL;

            status = KphOpenProcessJob(
                PhKphHandle,
                &jobHandle,
                processHandleLimited,
                JOB_OBJECT_QUERY
                );

            if (NT_SUCCESS(status) && status != STATUS_PROCESS_NOT_IN_JOB && jobHandle)
            {
                JOBOBJECT_BASIC_LIMIT_INFORMATION basicLimits;

                Data->IsInJob = TRUE;

                PhGetHandleInformation(
                    NtCurrentProcess(),
                    jobHandle,
                    -1,
                    NULL,
                    NULL,
                    NULL,
                    &Data->JobName
                    );

                // Process Explorer only recognizes processes as being in jobs if they 
                // don't have the silent-breakaway-OK limit as their only limit.
                // Emulate this behaviour.
                if (NT_SUCCESS(PhGetJobBasicLimits(jobHandle, &basicLimits)))
                {
                    Data->IsInSignificantJob =
                        basicLimits.LimitFlags != JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
                }

                NtClose(jobHandle);
            }
        }
        else
        {
            // KProcessHacker not available. We can determine if the process is 
            // in a job, but we can't get a handle to the job.

            status = NtIsProcessInJob(processHandleLimited, NULL);

            if (NT_SUCCESS(status))
                Data->IsInJob = status == STATUS_PROCESS_IN_JOB;
        }
    }

    // Console host process
    if (WINDOWS_HAS_CONSOLE_HOST)
    {
        PhGetProcessConsoleHostProcessId(processHandleLimited, &Data->ConsoleHostProcessId);
    }

    if (processHandleLimited)
        NtClose(processHandleLimited);

    PhpQueueProcessQueryStage2(processItem);
}

VOID PhpProcessQueryStage2(
    __inout PPH_PROCESS_QUERY_S2_DATA Data
    )
{
    NTSTATUS status;
    PPH_PROCESS_ITEM processItem = Data->Header.ProcessItem;
    HANDLE processId = processItem->ProcessId;

    PhGetProcessIsDotNet(processId, &Data->IsDotNet);

    // Despite its name PhEnableProcessQueryStage2 doesn't disable stage 2; 
    // it disables checks for signatures and packing.

    if (PhEnableProcessQueryStage2 && processItem->FileName)
    {
        Data->VerifyResult = PhVerifyFile(
            processItem->FileName->Buffer,
            &Data->VerifySignerName
            );

        status = PhIsExecutablePacked(
            processItem->FileName->Buffer,
            &Data->IsPacked,
            &Data->ImportModules,
            &Data->ImportFunctions
            );

        // If we got an image-related error, the image is packed.
        if (
            status == STATUS_INVALID_IMAGE_NOT_MZ ||
            status == STATUS_INVALID_IMAGE_FORMAT ||
            status == STATUS_ACCESS_VIOLATION
            )
        {
            Data->IsPacked = TRUE;
            Data->ImportModules = -1;
            Data->ImportFunctions = -1;
        }
    }
}

NTSTATUS PhpProcessQueryStage1Worker(
    __in PVOID Parameter
    )
{
    PPH_PROCESS_QUERY_S1_DATA data;
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;

    data = PhAllocate(sizeof(PH_PROCESS_QUERY_S1_DATA));
    memset(data, 0, sizeof(PH_PROCESS_QUERY_S1_DATA));
    data->Header.Stage = 1;
    data->Header.ProcessItem = processItem;

    PhpProcessQueryStage1(data);

    RtlInterlockedPushEntrySList(&PhProcessQueryDataListHead, &data->Header.ListEntry);

    return STATUS_SUCCESS;
}

NTSTATUS PhpProcessQueryStage2Worker(
    __in PVOID Parameter
    )
{
    PPH_PROCESS_QUERY_S2_DATA data;
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;

    data = PhAllocate(sizeof(PH_PROCESS_QUERY_S2_DATA));
    memset(data, 0, sizeof(PH_PROCESS_QUERY_S2_DATA));
    data->Header.Stage = 2;
    data->Header.ProcessItem = processItem;

    PhpProcessQueryStage2(data);

    RtlInterlockedPushEntrySList(&PhProcessQueryDataListHead, &data->Header.ListEntry);

    return STATUS_SUCCESS;
}

VOID PhpQueueProcessQueryStage1(
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    // Ref: dereferenced when the provider update function removes the item from 
    // the queue.
    PhReferenceObject(ProcessItem);
    PhQueueGlobalWorkQueueItem(PhpProcessQueryStage1Worker, ProcessItem);
}

VOID PhpQueueProcessQueryStage2(
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    PhReferenceObject(ProcessItem);
    PhQueueGlobalWorkQueueItem(PhpProcessQueryStage2Worker, ProcessItem);
}

VOID PhpFillProcessItemStage1(
    __in PPH_PROCESS_QUERY_S1_DATA Data
    )
{
    PPH_PROCESS_ITEM processItem = Data->Header.ProcessItem;

    processItem->CommandLine = Data->CommandLine;
    processItem->SmallIcon = Data->SmallIcon;
    processItem->LargeIcon = Data->LargeIcon;
    memcpy(&processItem->VersionInfo, &Data->VersionInfo, sizeof(PH_IMAGE_VERSION_INFO));
    processItem->ElevationType = Data->ElevationType;
    processItem->IntegrityLevel = Data->IntegrityLevel;
    processItem->JobName = Data->JobName;
    processItem->ConsoleHostProcessId = Data->ConsoleHostProcessId;
    processItem->IsElevated = Data->IsElevated;
    processItem->IsInJob = Data->IsInJob;
    processItem->IsInSignificantJob = Data->IsInSignificantJob;
    processItem->IsPosix = Data->IsPosix;
    processItem->IsWow64 = Data->IsWow64;

    if (Data->IntegrityString)
    {
        wcsncpy(processItem->IntegrityString, Data->IntegrityString->Buffer, PH_INTEGRITY_STR_LEN);
        PhDereferenceObject(Data->IntegrityString);
    }
}

VOID PhpFillProcessItemStage2(
    __in PPH_PROCESS_QUERY_S2_DATA Data
    )
{
    PPH_PROCESS_ITEM processItem = Data->Header.ProcessItem;

    processItem->IsDotNet = Data->IsDotNet;
    processItem->VerifyResult = Data->VerifyResult;
    processItem->VerifySignerName = Data->VerifySignerName;
    processItem->IsPacked = Data->IsPacked;
    processItem->ImportFunctions = Data->ImportFunctions;
    processItem->ImportModules = Data->ImportModules;
}

VOID PhpFillProcessItem(
    __inout PPH_PROCESS_ITEM ProcessItem,
    __in PSYSTEM_PROCESS_INFORMATION Process
    )
{
    NTSTATUS status;
    HANDLE processHandle = NULL;

    ProcessItem->ParentProcessId = Process->InheritedFromUniqueProcessId;
    ProcessItem->SessionId = Process->SessionId;
    ProcessItem->CreateTime = Process->CreateTime;

    if (ProcessItem->ProcessId != SYSTEM_IDLE_PROCESS_ID)
    {
        ProcessItem->ProcessName = PhCreateStringEx(Process->ImageName.Buffer, Process->ImageName.Length);
    }
    else
    {
        ProcessItem->ProcessName = PhCreateString(L"System Idle Process");
    }

    PhPrintUInt32(ProcessItem->ParentProcessIdString, (ULONG)ProcessItem->ParentProcessId);
    PhPrintUInt32(ProcessItem->SessionIdString, ProcessItem->SessionId);

    PhOpenProcess(&processHandle, ProcessQueryAccess, ProcessItem->ProcessId);

    // Process information
    {
        // If we're dealing with System (PID 4), we need to get the 
        // kernel file name. Otherwise, get the image file name.

        if (ProcessItem->ProcessId != SYSTEM_PROCESS_ID)
        {
            PPH_STRING fileName;

            if (WINDOWS_HAS_IMAGE_FILE_NAME_BY_PROCESS_ID)
                status = PhGetProcessImageFileNameByProcessId(ProcessItem->ProcessId, &fileName);
            else
                status = PhGetProcessImageFileName(processHandle, &fileName);

            if (NT_SUCCESS(status))
            {
                PPH_STRING newFileName;

                newFileName = PhGetFileName(fileName);
                ProcessItem->FileName = newFileName;

                PhDereferenceObject(fileName);
            }
        }
        else
        {
            PPH_STRING fileName;

            fileName = PhGetKernelFileName();

            if (fileName)
            {
                PPH_STRING newFileName;

                newFileName = PhGetFileName(fileName);
                ProcessItem->FileName = newFileName;

                PhDereferenceObject(fileName);
            }
        }
    }

    // Token-related information
    if (
        processHandle &&
        !(WindowsVersion <= WINDOWS_XP && ProcessItem->ProcessId == SYSTEM_PROCESS_ID)
        )
    {
        HANDLE tokenHandle;

        status = PhOpenProcessToken(&tokenHandle, TOKEN_QUERY, processHandle);

        if (NT_SUCCESS(status))
        {
            // User name
            {
                PTOKEN_USER user;

                status = PhGetTokenUser(tokenHandle, &user);

                if (NT_SUCCESS(status))
                {
                    ProcessItem->UserName = PhGetSidFullName(user->User.Sid, TRUE, NULL);
                    PhFree(user);
                }
            }

            NtClose(tokenHandle);
        }
    }
    else
    {
        if (ProcessItem->ProcessId == SYSTEM_IDLE_PROCESS_ID)
            ProcessItem->UserName = PhDuplicateString(PhLocalSystemName);
        else if (ProcessItem->ProcessId == SYSTEM_PROCESS_ID) // System token can't be opened on XP
            ProcessItem->UserName = PhDuplicateString(PhLocalSystemName);
    }

    if (!ProcessItem->UserName && WindowsVersion <= WINDOWS_XP)
    {
        // In some cases we can get the user SID using WTS (only works on XP and below).

        if (!PhpWtsProcesses)
        {
            WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, 0, 1, &PhpWtsProcesses, &PhpWtsNumberOfProcesses);
        }

        if (PhpWtsProcesses)
        {
            ULONG i;

            for (i = 0; i < PhpWtsNumberOfProcesses; i++)
            {
                if (PhpWtsProcesses[i].ProcessId == (ULONG)ProcessItem->ProcessId)
                    ProcessItem->UserName = PhGetSidFullName(PhpWtsProcesses[i].pUserSid, TRUE, NULL);
            }
        }
    }

    NtClose(processHandle);
}

FORCEINLINE VOID PhpUpdateDynamicInfoProcessItem(
    __inout PPH_PROCESS_ITEM ProcessItem,
    __in PSYSTEM_PROCESS_INFORMATION Process
    )
{
    ProcessItem->BasePriority = Process->BasePriority;
    ProcessItem->PriorityClassWin32 = GetPriorityClass(ProcessItem->QueryHandle);
    ProcessItem->KernelTime = Process->KernelTime;
    ProcessItem->UserTime = Process->UserTime;
    ProcessItem->NumberOfHandles = Process->HandleCount;
    ProcessItem->NumberOfThreads = Process->NumberOfThreads;
}

VOID PhpUpdatePerfInformation()
{
    NtQuerySystemInformation(
        SystemPerformanceInformation,
        &PhPerfInformation,
        sizeof(SYSTEM_PERFORMANCE_INFORMATION),
        NULL
        );

    PhUpdateDelta(&PhIoReadDelta, PhPerfInformation.IoReadTransferCount.QuadPart);
    PhUpdateDelta(&PhIoWriteDelta, PhPerfInformation.IoWriteTransferCount.QuadPart);
    PhUpdateDelta(&PhIoOtherDelta, PhPerfInformation.IoOtherTransferCount.QuadPart);
}

VOID PhpUpdateCpuInformation(
    __out PULONG64 TotalTime
    )
{
    ULONG i;
    ULONG64 totalTime;

    NtQuerySystemInformation(
        SystemProcessorPerformanceInformation,
        PhCpuInformation,
        sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) *
        (ULONG)PhSystemBasicInformation.NumberOfProcessors,
        NULL
        );

    // Zero the CPU totals.
    memset(&PhCpuTotals, 0, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION));

    for (i = 0; i < (ULONG)PhSystemBasicInformation.NumberOfProcessors; i++)
    {
        PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION cpuInfo =
            &PhCpuInformation[i];

        // KernelTime includes kernel + idle + DPC + interrupt time.
        cpuInfo->KernelTime.QuadPart -=
            cpuInfo->IdleTime.QuadPart +
            cpuInfo->DpcTime.QuadPart +
            cpuInfo->InterruptTime.QuadPart;

        PhCpuTotals.DpcTime.QuadPart += cpuInfo->DpcTime.QuadPart;
        PhCpuTotals.IdleTime.QuadPart += cpuInfo->IdleTime.QuadPart;
        PhCpuTotals.InterruptCount += cpuInfo->InterruptCount;
        PhCpuTotals.InterruptTime.QuadPart += cpuInfo->InterruptTime.QuadPart;
        PhCpuTotals.KernelTime.QuadPart += cpuInfo->KernelTime.QuadPart;
        PhCpuTotals.UserTime.QuadPart += cpuInfo->UserTime.QuadPart;

        PhUpdateDelta(&PhCpusKernelDelta[i], cpuInfo->KernelTime.QuadPart);
        PhUpdateDelta(&PhCpusUserDelta[i], cpuInfo->UserTime.QuadPart);
        PhUpdateDelta(&PhCpusOtherDelta[i],
            cpuInfo->IdleTime.QuadPart +
            cpuInfo->DpcTime.QuadPart +
            cpuInfo->InterruptTime.QuadPart
            );

        totalTime = PhCpusKernelDelta[i].Delta + PhCpusUserDelta[i].Delta + PhCpusOtherDelta[i].Delta;

        if (totalTime != 0)
        {
            PhCpusKernelUsage[i] = (FLOAT)PhCpusKernelDelta[i].Delta / totalTime;
            PhCpusUserUsage[i] = (FLOAT)PhCpusUserDelta[i].Delta / totalTime;
        }
        else
        {
            PhCpusKernelUsage[i] = 0;
            PhCpusUserUsage[i] = 0;
        }
    }

    PhUpdateDelta(&PhCpuKernelDelta, PhCpuTotals.KernelTime.QuadPart);
    PhUpdateDelta(&PhCpuUserDelta, PhCpuTotals.UserTime.QuadPart);
    PhUpdateDelta(&PhCpuOtherDelta,
        PhCpuTotals.IdleTime.QuadPart +
        PhCpuTotals.DpcTime.QuadPart +
        PhCpuTotals.InterruptTime.QuadPart
        );

    totalTime = PhCpuKernelDelta.Delta + PhCpuUserDelta.Delta + PhCpuOtherDelta.Delta;

    if (totalTime != 0)
    {
        PhCpuKernelUsage = (FLOAT)PhCpuKernelDelta.Delta / totalTime;
        PhCpuUserUsage = (FLOAT)PhCpuUserDelta.Delta / totalTime;
    }
    else
    {
        PhCpuKernelUsage = 0;
        PhCpuUserUsage = 0;
    }

    *TotalTime = totalTime;
}

VOID PhpInitializeProcessStatistics()
{
    ULONG i;

    PhInitializeCircularBuffer_ULONG(&PhTimeHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_FLOAT(&PhCpuKernelHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_FLOAT(&PhCpuUserHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_ULONG64(&PhIoReadHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_ULONG64(&PhIoWriteHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_ULONG64(&PhIoOtherHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_ULONG(&PhCommitHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_ULONG(&PhPhysicalHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_ULONG(&PhMaxCpuHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_ULONG(&PhMaxIoHistory, PhStatisticsSampleCount);
#ifdef PH_RECORD_MAX_USAGE
    PhInitializeCircularBuffer_FLOAT(&PhMaxCpuUsageHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_ULONG64(&PhMaxIoReadOtherHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_ULONG64(&PhMaxIoWriteHistory, PhStatisticsSampleCount);
#endif

    for (i = 0; i < (ULONG)PhSystemBasicInformation.NumberOfProcessors; i++)
    {
        PhInitializeCircularBuffer_FLOAT(&PhCpusKernelHistory[i], PhStatisticsSampleCount);
        PhInitializeCircularBuffer_FLOAT(&PhCpusUserHistory[i], PhStatisticsSampleCount);
    }
}

VOID PhpUpdateSystemHistory()
{
    ULONG i;
    LARGE_INTEGER systemTime;
    ULONG secondsSince1980;

    // CPU
    PhCircularBufferAdd_FLOAT(&PhCpuKernelHistory, PhCpuKernelUsage);
    PhCircularBufferAdd_FLOAT(&PhCpuUserHistory, PhCpuUserUsage);

    // CPUs
    for (i = 0; i < (ULONG)PhSystemBasicInformation.NumberOfProcessors; i++)
    {
        PhCircularBufferAdd_FLOAT(&PhCpusKernelHistory[i], PhCpusKernelUsage[i]);
        PhCircularBufferAdd_FLOAT(&PhCpusUserHistory[i], PhCpusUserUsage[i]);
    }

    // I/O
    PhCircularBufferAdd_ULONG64(&PhIoReadHistory, PhIoReadDelta.Delta);
    PhCircularBufferAdd_ULONG64(&PhIoWriteHistory, PhIoWriteDelta.Delta);
    PhCircularBufferAdd_ULONG64(&PhIoOtherHistory, PhIoOtherDelta.Delta);

    // Memory
    PhCircularBufferAdd_ULONG(&PhCommitHistory, PhPerfInformation.CommittedPages);
    PhCircularBufferAdd_ULONG(&PhPhysicalHistory,
        PhSystemBasicInformation.NumberOfPhysicalPages - PhPerfInformation.AvailablePages
        );

    // Time
    PhQuerySystemTime(&systemTime);
    RtlTimeToSecondsSince1980(&systemTime, &secondsSince1980);
    PhCircularBufferAdd_ULONG(&PhTimeHistory, secondsSince1980);
    PhTimeSequenceNumber++;
}

BOOLEAN PhGetStatisticsTime(
    __in_opt PPH_PROCESS_ITEM ProcessItem,
    __in ULONG Index,
    __out PLARGE_INTEGER Time
    )
{
    ULONG secondsSince1980;
    ULONG index;
    LARGE_INTEGER time;

    if (ProcessItem)
    {
        // The sequence number is used to synchronize statistics when a process exits, since 
        // that process' history is not updated anymore.
        index = PhTimeSequenceNumber - ProcessItem->SequenceNumber + Index;

        if (index >= PhTimeHistory.Count)
        {
            // The data point is too far into the past.
            return FALSE;
        }
    }
    else
    {
        // Assume the index is valid.
        index = Index;
    }

    secondsSince1980 = PhCircularBufferGet_ULONG(&PhTimeHistory, index);
    RtlSecondsSince1980ToTime(secondsSince1980, &time);

    *Time = time;

    return TRUE;
}

PPH_STRING PhGetStatisticsTimeString(
    __in_opt PPH_PROCESS_ITEM ProcessItem,
    __in ULONG Index
    )
{
    LARGE_INTEGER time;
    SYSTEMTIME systemTime;
    PPH_STRING dateString;
    PPH_STRING timeString;
    PPH_STRING dateAndTimeString;

    if (PhGetStatisticsTime(ProcessItem, Index, &time))
    {
        PhLargeIntegerToLocalSystemTime(&systemTime, &time);

        dateString = PhFormatDate(&systemTime, NULL);
        timeString = PhFormatTime(&systemTime, NULL);
        dateAndTimeString = PhConcatStrings(3, dateString->Buffer, L" ", timeString->Buffer);
        PhDereferenceObject(dateString);
        PhDereferenceObject(timeString);

        return dateAndTimeString;
    }
    else
    {
        return PhCreateString(L"Unknown time");
    }
}

VOID PhProcessProviderUpdate(
    __in PVOID Object
    )
{
    static ULONG runCount = 0;
    static PPH_LIST pids = NULL;

    // Note about locking:
    // Since this is the only function that is allowed to 
    // modify the process hashtable, locking is not needed 
    // for shared accesses. However, exclusive accesses 
    // need locking.

    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;

    ULONG64 sysTotalTime;
    FLOAT maxCpuValue = 0;
    PPH_PROCESS_ITEM maxCpuProcessItem = NULL;
    ULONG64 maxIoValue = 0;
    PPH_PROCESS_ITEM maxIoProcessItem = NULL;

    // Pre-update tasks

    if (runCount % 4 == 0)
        PhRefreshDosDevicePrefixes();
    if (runCount % 512 == 0) // yes, a very long time
        PhPurgeProcessRecords();

    if (!PhProcessStatisticsInitialized)
    {
        PhpInitializeProcessStatistics();
        PhProcessStatisticsInitialized = TRUE;
    }

    PhpUpdatePerfInformation();
    PhpUpdateCpuInformation(&sysTotalTime);

    // History cannot be updated on the first run because the deltas are invalid.
    // For example, the I/O "deltas" will be huge because they are currently the 
    // raw accumulated values.
    if (runCount != 0)
    {
        PhpUpdateSystemHistory();
    }

    // Get the process list.

    PhTotalProcesses = 0;
    PhTotalThreads = 0;
    PhTotalHandles = 0;

    if (!NT_SUCCESS(PhEnumProcesses(&processes)))
        return;

    // Create a PID list.

    if (!pids)
        pids = PhCreateList(40);

    process = PH_FIRST_PROCESS(processes);

    do
    {
        PhTotalProcesses++;
        PhTotalThreads += process->NumberOfThreads;
        PhTotalHandles += process->HandleCount;

        if (process->UniqueProcessId == SYSTEM_IDLE_PROCESS_ID)
        {
            process->KernelTime = PhCpuTotals.IdleTime;
        }

        PhAddListItem(pids, process->UniqueProcessId);
    } while (process = PH_NEXT_PROCESS(process));

    // Add the fake processes to the PID list.
    PhAddListItem(pids, DPCS_PROCESS_ID);
    PhAddListItem(pids, INTERRUPTS_PROCESS_ID);

    PhDpcsProcessInformation.KernelTime = PhCpuTotals.DpcTime;
    PhInterruptsProcessInformation.KernelTime = PhCpuTotals.InterruptTime;

    // Look for dead processes.
    {
        PPH_LIST processesToRemove = NULL;
        ULONG enumerationKey = 0;
        PPH_PROCESS_ITEM *processItem;
        ULONG i;

        while (PhEnumHashtable(PhProcessHashtable, (PPVOID)&processItem, &enumerationKey))
        {
            // Check if the process still exists.
            // TODO: Check CreateTime as well; between updates a process may terminate and 
            // get replaced by another one with the same PID.
            if (PhIndexOfListItem(pids, (*processItem)->ProcessId) == -1)
            {
                PPH_PROCESS_ITEM processItem2;

                processItem2 = *processItem;
                processItem2->State |= PH_PROCESS_ITEM_REMOVED;

                if (processItem2->QueryHandle)
                {
                    KERNEL_USER_TIMES times;

                    if (NT_SUCCESS(PhGetProcessTimes(processItem2->QueryHandle, &times)))
                    {
                        processItem2->Record->Flags |= PH_PROCESS_RECORD_DEAD;
                        processItem2->Record->ExitTime = times.ExitTime;
                    }
                }

                // Raise the process removed event.
                PhInvokeCallback(&PhProcessRemovedEvent, *processItem);

                if (!processesToRemove)
                    processesToRemove = PhCreateList(2);

                PhAddListItem(processesToRemove, *processItem);
            }
        }

        // Lock only if we have something to do.
        if (processesToRemove)
        {
            PhAcquireQueuedLockExclusive(&PhProcessHashtableLock);

            for (i = 0; i < processesToRemove->Count; i++)
            {
                PhpRemoveProcessItem((PPH_PROCESS_ITEM)processesToRemove->Items[i]);
            }

            PhReleaseQueuedLockExclusive(&PhProcessHashtableLock);
            PhDereferenceObject(processesToRemove);
        }
    }

    // Go through the queued process query data.
    if (RtlQueryDepthSList(&PhProcessQueryDataListHead) != 0)
    {
        PSLIST_ENTRY entry;
        PPH_PROCESS_QUERY_DATA data;

        while (entry = RtlInterlockedPopEntrySList(&PhProcessQueryDataListHead))
        {
            data = CONTAINING_RECORD(entry, PH_PROCESS_QUERY_DATA, ListEntry);

            if (data->Stage == 1)
            {
                PhpFillProcessItemStage1((PPH_PROCESS_QUERY_S1_DATA)data);
                PhSetEvent(&data->ProcessItem->Stage1Event);
                data->ProcessItem->JustProcessed = TRUE;
            }
            else if (data->Stage == 2)
            {
                PhpFillProcessItemStage2((PPH_PROCESS_QUERY_S2_DATA)data);
                data->ProcessItem->JustProcessed = TRUE;
            }

            PhDereferenceObject(data->ProcessItem);
            PhFree(data);
        }
    }

    // Look for new processes and update existing ones.
    process = PH_FIRST_PROCESS(processes);

    while (process)
    {
        PPH_PROCESS_ITEM processItem;

        processItem = PhpLookupProcessItem(process->UniqueProcessId);

        if (!processItem)
        {
            PPH_PROCESS_RECORD processRecord;

            // Create the process item and fill in basic information.
            processItem = PhCreateProcessItem(process->UniqueProcessId);
            PhpFillProcessItem(processItem, process);
            processItem->SequenceNumber = PhTimeSequenceNumber;

            // Create and add a process record. (This will have to be moved when the CommandLine/UserName 
            // fields are enabled.)
            processRecord = PhpCreateProcessRecord(processItem);
            PhpAddProcessRecord(processRecord);
            processItem->Record = processRecord;

            // Open a handle to the process for later usage.
            PhOpenProcess(&processItem->QueryHandle, PROCESS_QUERY_INFORMATION, processItem->ProcessId);

            // Check if process actually has a parent.
            {
                PSYSTEM_PROCESS_INFORMATION parentProcess;

                processItem->HasParent = TRUE;
                parentProcess = PhFindProcessInformation(processes, processItem->ParentProcessId);

                if (!parentProcess || processItem->ParentProcessId == processItem->ProcessId)
                {
                    processItem->HasParent = FALSE;
                }
                else if (parentProcess)
                {
                    // Check the parent's creation time to see if it is 
                    // actually the parent.
                    if (parentProcess->CreateTime.QuadPart > processItem->CreateTime.QuadPart)
                        processItem->HasParent = FALSE;
                }
            }

            PhpUpdateDynamicInfoProcessItem(processItem, process);

            // Initialize the deltas.
            PhUpdateDelta(&processItem->CpuKernelDelta, process->KernelTime.QuadPart);
            PhUpdateDelta(&processItem->CpuUserDelta, process->UserTime.QuadPart);
            PhUpdateDelta(&processItem->IoReadDelta, process->ReadTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoWriteDelta, process->WriteTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoOtherDelta, process->OtherTransferCount.QuadPart);

            // Update VM and I/O statistics.
            processItem->VmCounters = *(PVM_COUNTERS_EX)&process->PeakVirtualSize;
            processItem->IoCounters = *(PIO_COUNTERS)&process->ReadOperationCount;

            // If this is the first run of the provider, queue the 
            // process query tasks. Otherwise, perform stage 1 
            // processing now and queue stage 2 processing.
            if (runCount > 0)
            {
                PH_PROCESS_QUERY_S1_DATA data;

                memset(&data, 0, sizeof(PH_PROCESS_QUERY_S1_DATA));
                data.Header.Stage = 1;
                data.Header.ProcessItem = processItem;
                PhpProcessQueryStage1(&data);
                PhpFillProcessItemStage1(&data);
                PhSetEvent(&processItem->Stage1Event);
            }
            else
            {
                PhpQueueProcessQueryStage1(processItem);
            }

            // Add pending service items to the process item.
            PhUpdateProcessItemServices(processItem);

            // Add the process item to the hashtable.
            PhAcquireQueuedLockExclusive(&PhProcessHashtableLock);
            PhAddHashtableEntry(PhProcessHashtable, &processItem);
            PhReleaseQueuedLockExclusive(&PhProcessHashtableLock);

            // Raise the process added event.
            PhInvokeCallback(&PhProcessAddedEvent, processItem);

            // (Ref: for the process item being in the hashtable.)
            // Instead of referencing then dereferencing we simply don't 
            // do anything.
            // Dereferenced in PhpRemoveProcessItem.
        }
        else
        {
            BOOLEAN modified = FALSE;
            FLOAT kernelCpuUsage;
            FLOAT userCpuUsage;
            FLOAT newCpuUsage;

            // Update the deltas.
            PhUpdateDelta(&processItem->CpuKernelDelta, process->KernelTime.QuadPart);
            PhUpdateDelta(&processItem->CpuUserDelta, process->UserTime.QuadPart);
            PhUpdateDelta(&processItem->IoReadDelta, process->ReadTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoWriteDelta, process->WriteTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoOtherDelta, process->OtherTransferCount.QuadPart);

            processItem->SequenceNumber++;
            PhCircularBufferAdd_ULONG64(&processItem->IoReadHistory, processItem->IoReadDelta.Delta);
            PhCircularBufferAdd_ULONG64(&processItem->IoWriteHistory, processItem->IoWriteDelta.Delta);
            PhCircularBufferAdd_ULONG64(&processItem->IoOtherHistory, processItem->IoOtherDelta.Delta);

            PhCircularBufferAdd_SIZE_T(&processItem->PrivateBytesHistory, processItem->VmCounters.PagefileUsage);
            //PhCircularBufferAdd_SIZE_T(&processItem->WorkingSetHistory, processItem->VmCounters.WorkingSetSize);

            PhpUpdateDynamicInfoProcessItem(processItem, process);

            // Update VM and I/O statistics.
            processItem->VmCounters = *(PVM_COUNTERS_EX)&process->PeakVirtualSize;
            processItem->IoCounters = *(PIO_COUNTERS)&process->ReadOperationCount;

            if (processItem->JustProcessed)
            {
                processItem->JustProcessed = FALSE;
                modified = TRUE;
            }

            if (sysTotalTime != 0)
            {
                kernelCpuUsage = (FLOAT)processItem->CpuKernelDelta.Delta / sysTotalTime;
                userCpuUsage = (FLOAT)processItem->CpuUserDelta.Delta / sysTotalTime;
            }
            else
            {
                kernelCpuUsage = 0;
                userCpuUsage = 0;
            }

            processItem->CpuKernelUsage = kernelCpuUsage;
            processItem->CpuUserUsage = userCpuUsage;
            PhCircularBufferAdd_FLOAT(&processItem->CpuKernelHistory, kernelCpuUsage);
            PhCircularBufferAdd_FLOAT(&processItem->CpuUserHistory, userCpuUsage);

            newCpuUsage = kernelCpuUsage + userCpuUsage;

            if (processItem->CpuUsage != newCpuUsage)
            {
                processItem->CpuUsage = newCpuUsage;

                if ((newCpuUsage * 100) >= 0.01)
                {
                    _snwprintf(processItem->CpuUsageString, PH_INT32_STR_LEN,
                        L"%.2f", (DOUBLE)newCpuUsage * 100);
                }
                else
                {
                    processItem->CpuUsageString[0] = 0;
                }
            }

            // Max. values

            if (processItem->ProcessId != NULL)
            {
                if (maxCpuValue < newCpuUsage)
                {
                    maxCpuValue = newCpuUsage;
                    maxCpuProcessItem = processItem;
                }

                // I/O for Other is not included because it is too generic.
                if (maxIoValue < processItem->IoReadDelta.Delta + processItem->IoWriteDelta.Delta)
                {
                    maxIoValue = processItem->IoReadDelta.Delta + processItem->IoWriteDelta.Delta;
                    maxIoProcessItem = processItem;
                }
            }

            // Debugged
            {
                BOOLEAN isBeingDebugged;

                if (NT_SUCCESS(PhGetProcessIsBeingDebugged(
                    processItem->QueryHandle,
                    &isBeingDebugged
                    )) && processItem->IsBeingDebugged != isBeingDebugged)
                {
                    processItem->IsBeingDebugged = isBeingDebugged;
                    modified = TRUE;
                }
            }

            // Suspended
            {
                BOOLEAN isSuspended;

                isSuspended = PhGetProcessIsSuspended(process);

                if (processItem->IsSuspended != isSuspended)
                {
                    processItem->IsSuspended = isSuspended;
                    modified = TRUE;
                }
            }

            if (modified)
            {
                PhInvokeCallback(&PhProcessModifiedEvent, processItem);
            }

            // No reference added by PhpLookupProcessItem.
        }

        // Trick ourselves into thinking that DPCs and Interrupts 
        // are on the list.
        if (process == &PhInterruptsProcessInformation)
        {
            process = NULL;
        }
        else if (process == &PhDpcsProcessInformation)
        {
            process = &PhInterruptsProcessInformation;
        }
        else
        {
            process = PH_NEXT_PROCESS(process);

            if (process == NULL)
                process = &PhDpcsProcessInformation;
        }
    }

    PhClearList(pids);
    PhFree(processes);

    if (PhpWtsProcesses)
    {
        WTSFreeMemory(PhpWtsProcesses);
        PhpWtsProcesses = NULL;
    }

    if (runCount != 0)
    {
        // Post-update history

        // Note that we need to add a reference to the records of these processes, to 
        // make it possible for others to get the name of a max. CPU or I/O process.

        if (maxCpuProcessItem)
        {
            PhCircularBufferAdd_ULONG(&PhMaxCpuHistory, (ULONG)maxCpuProcessItem->ProcessId);
#ifdef PH_RECORD_MAX_USAGE
            PhCircularBufferAdd_FLOAT(&PhMaxCpuUsageHistory, maxCpuProcessItem->CpuUsage);
#endif

            if (!(maxCpuProcessItem->State & PH_PROCESS_ITEM_RECORD_STAT_REF))
            {
                PhReferenceProcessRecord(maxCpuProcessItem->Record);
                maxCpuProcessItem->State |= PH_PROCESS_ITEM_RECORD_STAT_REF;
                maxCpuProcessItem->Record->Flags |= PH_PROCESS_RECORD_STAT_REF;
            }
        }
        else
        {
            PhCircularBufferAdd_ULONG(&PhMaxCpuHistory, (ULONG)NULL);
#ifdef PH_RECORD_MAX_USAGE
            PhCircularBufferAdd_FLOAT(&PhMaxCpuUsageHistory, 0);
#endif
        }

        if (maxIoProcessItem)
        {
            PhCircularBufferAdd_ULONG(&PhMaxIoHistory, (ULONG)maxIoProcessItem->ProcessId);
#ifdef PH_RECORD_MAX_USAGE
            PhCircularBufferAdd_ULONG64(&PhMaxIoReadOtherHistory,
                maxIoProcessItem->IoReadDelta.Delta + maxIoProcessItem->IoOtherDelta.Delta);
            PhCircularBufferAdd_ULONG64(&PhMaxIoWriteHistory, maxIoProcessItem->IoWriteDelta.Delta);
#endif

            if (!(maxIoProcessItem->State & PH_PROCESS_ITEM_RECORD_STAT_REF))
            {
                PhReferenceProcessRecord(maxIoProcessItem->Record);
                maxIoProcessItem->State |= PH_PROCESS_ITEM_RECORD_STAT_REF;
                maxIoProcessItem->Record->Flags |= PH_PROCESS_RECORD_STAT_REF;
            }
        }
        else
        {
            PhCircularBufferAdd_ULONG(&PhMaxIoHistory, (ULONG)NULL);
#ifdef PH_RECORD_MAX_USAGE
            PhCircularBufferAdd_ULONG64(&PhMaxIoReadOtherHistory, 0);
            PhCircularBufferAdd_ULONG64(&PhMaxIoWriteHistory, 0);
#endif
        }
    }

    PhInvokeCallback(&PhProcessesUpdatedEvent, NULL);
    runCount++;
}

PPH_PROCESS_RECORD PhpCreateProcessRecord(
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    PPH_PROCESS_RECORD processRecord;

    processRecord = PhAllocate(sizeof(PH_PROCESS_RECORD));
    memset(processRecord, 0, sizeof(PH_PROCESS_RECORD));

    InitializeListHead(&processRecord->ListEntry);
    processRecord->RefCount = 1;

    processRecord->ProcessId = ProcessItem->ProcessId;
    processRecord->ParentProcessId = ProcessItem->ParentProcessId;
    processRecord->SessionId = ProcessItem->SessionId;
    processRecord->CreateTime = ProcessItem->CreateTime;

    PhReferenceObject(ProcessItem->ProcessName);
    processRecord->ProcessName = ProcessItem->ProcessName;

    if (ProcessItem->FileName)
    {
        PhReferenceObject(ProcessItem->FileName);
        processRecord->FileName = ProcessItem->FileName;
    }

    /*if (ProcessItem->CommandLine)
    {
        PhReferenceObject(ProcessItem->CommandLine);
        processRecord->CommandLine = ProcessItem->CommandLine;
    }

    if (ProcessItem->UserName)
    {
        PhReferenceObject(ProcessItem->UserName);
        processRecord->UserName = ProcessItem->UserName;
    }*/

    return processRecord;
}

PPH_PROCESS_RECORD PhpSearchProcessRecordList(
    __in PLARGE_INTEGER Time,
    __out_opt PULONG Index,
    __out_opt PULONG InsertIndex
    )
{
    PPH_PROCESS_RECORD processRecord;
    LONG low;
    LONG high;
    LONG i;
    BOOLEAN found = FALSE;
    INT comparison;

    if (PhProcessRecordList->Count == 0)
    {
        if (Index)
            *Index = 0;
        if (InsertIndex)
            *InsertIndex = 0;

        return NULL;
    }

    low = 0;
    high = PhProcessRecordList->Count - 1;

    // Do a binary search.
    do
    {
        i = (low + high) / 2;
        processRecord = (PPH_PROCESS_RECORD)PhProcessRecordList->Items[i];

        comparison = uint64cmp(Time->QuadPart, processRecord->CreateTime.QuadPart);

        if (comparison == 0)
        {
            found = TRUE;
            break;
        }
        else if (comparison < 0)
        {
            high = i - 1;
        }
        else
        {
            low = i + 1;
        }
    } while (low <= high);

    if (Index)
        *Index = i;

    if (found)
    {
        return processRecord;
    }
    else
    {
        if (InsertIndex)
            *InsertIndex = i + (comparison > 0);

        return NULL;
    }
}

VOID PhpAddProcessRecord(
    __inout PPH_PROCESS_RECORD ProcessRecord
    )
{
    PPH_PROCESS_RECORD processRecord;
    ULONG insertIndex;

    PhAcquireQueuedLockExclusive(&PhProcessRecordListLock);

    processRecord = PhpSearchProcessRecordList(&ProcessRecord->CreateTime, NULL, &insertIndex);

    if (!processRecord)
    {
        // Insert the process record, keeping the list sorted.
        PhInsertListItem(PhProcessRecordList, insertIndex, ProcessRecord);
    }
    else
    {
        // Link the process record with the existing one that we found.
        InsertTailList(&processRecord->ListEntry, &ProcessRecord->ListEntry);
    }

    PhReleaseQueuedLockExclusive(&PhProcessRecordListLock);
}

VOID PhpRemoveProcessRecord(
    __inout PPH_PROCESS_RECORD ProcessRecord
    )
{
    ULONG i;
    PPH_PROCESS_RECORD headProcessRecord;

    PhAcquireQueuedLockExclusive(&PhProcessRecordListLock);

    headProcessRecord = PhpSearchProcessRecordList(&ProcessRecord->CreateTime, &i, NULL);
    assert(headProcessRecord);

    // Unlink the record from the list.
    RemoveEntryList(&ProcessRecord->ListEntry);

    if (ProcessRecord == headProcessRecord)
    {
        // Remove the slot completely, or choose a new head record.
        if (IsListEmpty(&headProcessRecord->ListEntry))
            PhRemoveListItem(PhProcessRecordList, i);
        else
            PhProcessRecordList->Items[i] = CONTAINING_RECORD(headProcessRecord->ListEntry.Flink, PH_PROCESS_RECORD, ListEntry);
    }

    PhReleaseQueuedLockExclusive(&PhProcessRecordListLock);
}

VOID PhReferenceProcessRecord(
    __in PPH_PROCESS_RECORD ProcessRecord
    )
{
    _InterlockedIncrement(&ProcessRecord->RefCount);
}

BOOLEAN PhReferenceProcessRecordSafe(
    __in PPH_PROCESS_RECORD ProcessRecord
    )
{
    return _InterlockedIncrementNoZero(&ProcessRecord->RefCount);
}

VOID PhDereferenceProcessRecord(
    __in PPH_PROCESS_RECORD ProcessRecord
    )
{
    if (_InterlockedDecrement(&ProcessRecord->RefCount) == 0)
    {
        PhpRemoveProcessRecord(ProcessRecord);

        PhDereferenceObject(ProcessRecord->ProcessName);
        if (ProcessRecord->FileName) PhDereferenceObject(ProcessRecord->FileName);
        /*if (ProcessRecord->CommandLine) PhDereferenceObject(ProcessRecord->CommandLine);
        if (ProcessRecord->UserName) PhDereferenceObject(ProcessRecord->UserName);*/
        PhFree(ProcessRecord);
    }
}

PPH_PROCESS_RECORD PhpFindProcessRecord(
    __in PPH_PROCESS_RECORD ProcessRecord,
    __in HANDLE ProcessId
    )
{
    PPH_PROCESS_RECORD startProcessRecord;

    startProcessRecord = ProcessRecord;

    do
    {
        if (ProcessRecord->ProcessId == ProcessId)
            return ProcessRecord;

        ProcessRecord = CONTAINING_RECORD(ProcessRecord->ListEntry.Flink, PH_PROCESS_RECORD, ListEntry);
    } while (ProcessRecord != startProcessRecord);

    return NULL;
}

/**
 * Finds a process record.
 *
 * \param ProcessId The ID of the process.
 * \param Time A time in which the process was active.
 *
 * \return The newest record older than \a Time, or NULL 
 * if the record could not be found. You must call 
 * PhDereferenceProcessRecord() when you no longer need 
 * the record.
 */
PPH_PROCESS_RECORD PhFindProcessRecord(
    __in_opt HANDLE ProcessId,
    __in PLARGE_INTEGER Time
    )
{
    PPH_PROCESS_RECORD processRecord;
    ULONG i;
    BOOLEAN found;

    if (PhProcessRecordList->Count == 0)
        return NULL;

    PhAcquireQueuedLockShared(&PhProcessRecordListLock);

    processRecord = PhpSearchProcessRecordList(Time, &i, NULL);

    if (!processRecord)
    {
        // This is expected. Now we search backwards to find the newest matching element 
        // older than the given time.

        found = FALSE;

        while (TRUE)
        {
            processRecord = (PPH_PROCESS_RECORD)PhProcessRecordList->Items[i];

            if (processRecord->CreateTime.QuadPart < Time->QuadPart)
            {
                if (ProcessId)
                {
                    processRecord = PhpFindProcessRecord(processRecord, ProcessId);

                    if (processRecord)
                        found = TRUE;
                }
                else
                {
                    found = TRUE;
                }

                if (found)
                    break;
            }

            if (i == 0)
                break;

            i--;
        };
    }
    else
    {
        if (ProcessId)
        {
            processRecord = PhpFindProcessRecord(processRecord, ProcessId);

            if (processRecord)
                found = TRUE;
            else
                found = FALSE;
        }
        else
        {
            found = TRUE;
        }
    }

    if (found)
    {
        // The record might have had its last reference just cleared but it hasn't 
        // been removed from the list yet.
        if (!PhReferenceProcessRecordSafe(processRecord))
            found = FALSE;
    }

    PhReleaseQueuedLockShared(&PhProcessRecordListLock);

    if (found)
        return processRecord;
    else
        return NULL;
}

VOID PhPurgeProcessRecords()
{
    PPH_PROCESS_RECORD processRecord;
    PPH_PROCESS_RECORD startProcessRecord;
    ULONG i;
    LARGE_INTEGER threshold;
    PPH_LIST derefList = NULL;

    if (PhProcessRecordList->Count == 0)
        return;

    // Get the oldest statistics time.
    PhGetStatisticsTime(NULL, PhTimeHistory.Count - 1, &threshold);

    PhAcquireQueuedLockShared(&PhProcessRecordListLock);

    for (i = 0; i < PhProcessRecordList->Count; i++)
    {
        processRecord = PhProcessRecordList->Items[i];
        startProcessRecord = processRecord;

        do
        {
            ULONG requiredFlags;

            requiredFlags = PH_PROCESS_RECORD_DEAD | PH_PROCESS_RECORD_STAT_REF;

            if ((processRecord->Flags & requiredFlags) == requiredFlags)
            {
                // Check if the process exit time is before the oldest statistics time. 
                // If so we can dereference the process record.
                if (processRecord->ExitTime.QuadPart < threshold.QuadPart)
                {
                    // Clear the stat ref bit; this is to make sure we don't try to 
                    // dereference the record twice (e.g. if someone else currently holds 
                    // a reference to the record and it doesn't get removed immediately).
                    processRecord->Flags &= ~PH_PROCESS_RECORD_STAT_REF;

                    if (!derefList)
                        derefList = PhCreateList(2);

                    PhAddListItem(derefList, processRecord);
                }
            }

            processRecord = CONTAINING_RECORD(processRecord->ListEntry.Flink, PH_PROCESS_RECORD, ListEntry);
        } while (processRecord != startProcessRecord);
    }

    PhReleaseQueuedLockShared(&PhProcessRecordListLock);

    if (derefList)
    {
        for (i = 0; i < derefList->Count; i++)
        {
            PhDereferenceProcessRecord(derefList->Items[i]);
        }

        PhDereferenceObject(derefList);
    }
}
