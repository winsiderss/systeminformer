/*
 * Process Hacker -
 *   process provider
 *
 * Copyright (C) 2009-2016 wj32
 * Copyright (C) 2017-2018 dmex
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

/*
 * This provider module handles the collection of process information and system-wide statistics. A
 * list of all running processes is kept and periodically scanned to detect new and terminated
 * processes.
 *
 * The retrieval of certain information is delayed in order to improve performance. This includes
 * things such as file icons, version information, digital signature verification, and packed
 * executable detection. These requests are handed to worker threads which then post back
 * information to a S-list.
 *
 * Also contained in this module is the storage of process records, which contain static information
 * about processes. Unlike process items which are removed as soon as their corresponding process
 * exits, process records remain as long as they are needed by the statistics system, and more
 * specifically the max. CPU and I/O history buffers. In PH 1.x, a new formatted string was created
 * at each update containing information about the maximum-usage process within that interval. Here
 * we use a much more storage-efficient method, where the raw maximum-usage PIDs are stored for each
 * interval, and the process record list is searched when the name of a process is needed.
 *
 * The process record list is stored as a list of records sorted by process creation time. If two or
 * more processes have the same creation time, they are added to a doubly-linked list. This
 * structure allows for fast searching in the typical scenario where we know the PID of a process
 * and a specific time in which the process was running. In this case a binary search is used and
 * then the list is traversed backwards until the process is found. Binary search is similarly used
 * for insertion and removal.
 *
 * On Windows 7 and above, CPU usage can be calculated from cycle time. However, cycle time cannot
 * be split into kernel/user components, and cycle time is not available for DPCs and Interrupts
 * separately (only a "system" cycle time).
 */

#include <phapp.h>
#include <phsettings.h>
#include <procprv.h>
#include <appresolver.h>

#include <shellapi.h>
#include <winsta.h>

#include <hndlinfo.h>
#include <kphuser.h>
#include <lsasup.h>
#include <verify.h>
#include <workqueue.h>

#include <extmgri.h>
#include <phplug.h>
#include <srvprv.h>

#define PROCESS_ID_BUCKETS 64
#define PROCESS_ID_TO_BUCKET_INDEX(ProcessId) ((HandleToUlong(ProcessId) / 4) & (PROCESS_ID_BUCKETS - 1))

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

    PPH_STRING JobName;
    HANDLE ConsoleHostProcessId;
    PPH_STRING PackageFullName;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG IsDotNet : 1;
            ULONG Spare1 : 1;
            ULONG IsInJob : 1;
            ULONG IsInSignificantJob : 1;
            ULONG IsBeingDebugged : 1;
            ULONG IsImmersive : 1;
            ULONG IsFilteredHandle : 1;
            ULONG Spare : 25;
        };
    };
} PH_PROCESS_QUERY_S1_DATA, *PPH_PROCESS_QUERY_S1_DATA;

typedef struct _PH_PROCESS_QUERY_S2_DATA
{
    PH_PROCESS_QUERY_DATA Header;

    VERIFY_RESULT VerifyResult;
    PPH_STRING VerifySignerName;

    BOOLEAN IsPacked;
    ULONG ImportFunctions;
    ULONG ImportModules;
} PH_PROCESS_QUERY_S2_DATA, *PPH_PROCESS_QUERY_S2_DATA;

typedef struct _PH_SID_FULL_NAME_CACHE_ENTRY
{
    PSID Sid;
    PPH_STRING FullName;
} PH_SID_FULL_NAME_CACHE_ENTRY, *PPH_SID_FULL_NAME_CACHE_ENTRY;

VOID NTAPI PhpProcessItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

VOID PhpQueueProcessQueryStage1(
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

VOID PhpQueueProcessQueryStage2(
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

PPH_PROCESS_RECORD PhpCreateProcessRecord(
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

VOID PhpAddProcessRecord(
    _Inout_ PPH_PROCESS_RECORD ProcessRecord
    );

VOID PhpRemoveProcessRecord(
    _Inout_ PPH_PROCESS_RECORD ProcessRecord
    );

PPH_OBJECT_TYPE PhProcessItemType = NULL;

PPH_HASH_ENTRY PhProcessHashSet[256] = PH_HASH_SET_INIT;
ULONG PhProcessHashSetCount = 0;
PH_QUEUED_LOCK PhProcessHashSetLock = PH_QUEUED_LOCK_INIT;

SLIST_HEADER PhProcessQueryDataListHead;

PPH_LIST PhProcessRecordList = NULL;
PH_QUEUED_LOCK PhProcessRecordListLock = PH_QUEUED_LOCK_INIT;

ULONG PhStatisticsSampleCount = 512;
BOOLEAN PhEnablePurgeProcessRecords = TRUE;
BOOLEAN PhEnableCycleCpuUsage = TRUE;

PVOID PhProcessInformation = NULL; // only can be used if running on same thread as process provider
SYSTEM_PERFORMANCE_INFORMATION PhPerfInformation;
PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PhCpuInformation = NULL;
SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PhCpuTotals;
ULONG PhTotalProcesses = 0;
ULONG PhTotalThreads = 0;
ULONG PhTotalHandles = 0;

PSYSTEM_PROCESS_INFORMATION PhDpcsProcessInformation = NULL;
PSYSTEM_PROCESS_INFORMATION PhInterruptsProcessInformation = NULL;

ULONG64 PhCpuTotalCycleDelta = 0; // real cycle time delta for this period
PLARGE_INTEGER PhCpuIdleCycleTime = NULL; // cycle time for Idle
PLARGE_INTEGER PhCpuSystemCycleTime = NULL; // cycle time for DPCs and Interrupts
PH_UINT64_DELTA PhCpuIdleCycleDelta;
PH_UINT64_DELTA PhCpuSystemCycleDelta;
//PPH_UINT64_DELTA PhCpusIdleCycleDelta;

FLOAT PhCpuKernelUsage = 0.0f;
FLOAT PhCpuUserUsage = 0.0f;
PFLOAT PhCpusKernelUsage = NULL;
PFLOAT PhCpusUserUsage = NULL;

PH_UINT64_DELTA PhCpuKernelDelta;
PH_UINT64_DELTA PhCpuUserDelta;
PH_UINT64_DELTA PhCpuIdleDelta;

PPH_UINT64_DELTA PhCpusKernelDelta = NULL;
PPH_UINT64_DELTA PhCpusUserDelta = NULL;
PPH_UINT64_DELTA PhCpusIdleDelta = NULL;

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

static PPH_HASHTABLE PhpSidFullNameCacheHashtable = NULL;

BOOLEAN PhProcessProviderInitialization(
    VOID
    )
{
    PFLOAT usageBuffer;
    PPH_UINT64_DELTA deltaBuffer;
    PPH_CIRCULAR_BUFFER_FLOAT historyBuffer;

    PhProcessItemType = PhCreateObjectType(L"ProcessItem", 0, PhpProcessItemDeleteProcedure);

    RtlInitializeSListHead(&PhProcessQueryDataListHead);

    PhProcessRecordList = PhCreateList(40);

    PhDpcsProcessInformation = PhAllocateZero(sizeof(SYSTEM_PROCESS_INFORMATION) + sizeof(SYSTEM_PROCESS_INFORMATION_EXTENSION));
    RtlInitUnicodeString(&PhDpcsProcessInformation->ImageName, L"DPCs");
    PhDpcsProcessInformation->UniqueProcessId = DPCS_PROCESS_ID;
    PhDpcsProcessInformation->InheritedFromUniqueProcessId = SYSTEM_IDLE_PROCESS_ID;

    PhInterruptsProcessInformation = PhAllocateZero(sizeof(SYSTEM_PROCESS_INFORMATION) + sizeof(SYSTEM_PROCESS_INFORMATION_EXTENSION));
    RtlInitUnicodeString(&PhInterruptsProcessInformation->ImageName, L"Interrupts");
    PhInterruptsProcessInformation->UniqueProcessId = INTERRUPTS_PROCESS_ID;
    PhInterruptsProcessInformation->InheritedFromUniqueProcessId = SYSTEM_IDLE_PROCESS_ID;

    PhCpuInformation = PhAllocate(
        sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) *
        (ULONG)PhSystemBasicInformation.NumberOfProcessors
        );

    PhCpuIdleCycleTime = PhAllocate(
        sizeof(LARGE_INTEGER) *
        (ULONG)PhSystemBasicInformation.NumberOfProcessors
        );
    PhCpuSystemCycleTime = PhAllocate(
        sizeof(LARGE_INTEGER) *
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
        3 // 4 for PhCpusIdleCycleDelta
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
    PhCpusIdleDelta = PhCpusUserDelta + (ULONG)PhSystemBasicInformation.NumberOfProcessors;
    //PhCpusIdleCycleDelta = PhCpusIdleDelta + (ULONG)PhSystemBasicInformation.NumberOfProcessors;

    PhCpusKernelHistory = historyBuffer;
    PhCpusUserHistory = PhCpusKernelHistory + (ULONG)PhSystemBasicInformation.NumberOfProcessors;

    memset(deltaBuffer, 0, sizeof(PH_UINT64_DELTA) * (ULONG)PhSystemBasicInformation.NumberOfProcessors);

    return TRUE;
}

PPH_STRING PhGetClientIdName(
    _In_ PCLIENT_ID ClientId
    )
{
    PPH_STRING name;
    PPH_PROCESS_ITEM processItem;

    processItem = PhReferenceProcessItem(ClientId->UniqueProcess);

    if (processItem)
    {
        name = PhGetClientIdNameEx(ClientId, processItem->ProcessName);
        PhDereferenceObject(processItem);
    }
    else
    {
        // HACK
        // Workaround race condition with newly created process handles showing as 'Non-existent process' -dmex
        name = PhStdGetClientIdName(ClientId);
    }

    return name;
}

PPH_STRING PhGetClientIdNameEx(
    _In_ PCLIENT_ID ClientId,
    _In_opt_ PPH_STRING ProcessName
    )
{
    PPH_STRING name;
    PH_FORMAT format[5];

    if (ClientId->UniqueThread)
    {
        if (ProcessName)
        {
            PhInitFormatSR(&format[0], ProcessName->sr);
            PhInitFormatS(&format[1], L" (");
            PhInitFormatIU(&format[2], (ULONG_PTR)ClientId->UniqueProcess);
            PhInitFormatS(&format[3], L"): ");
            PhInitFormatIU(&format[4], (ULONG_PTR)ClientId->UniqueThread);

            name = PhFormat(format, 5, ProcessName->Length + 16 * sizeof(WCHAR));
        }
        else
        {
            PhInitFormatS(&format[0], L"Non-existent process (");
            PhInitFormatIU(&format[1], (ULONG_PTR)ClientId->UniqueProcess);
            PhInitFormatS(&format[2], L"): ");
            PhInitFormatIU(&format[3], (ULONG_PTR)ClientId->UniqueThread);

            name = PhFormat(format, 4, 0);
        }
    }
    else
    {
        if (ProcessName)
        {
            PhInitFormatSR(&format[0], ProcessName->sr);
            PhInitFormatS(&format[1], L" (");
            PhInitFormatIU(&format[2], (ULONG_PTR)ClientId->UniqueProcess);
            PhInitFormatC(&format[3], ')');

            name = PhFormat(format, 4, 0);
        }
        else
        {
            PhInitFormatS(&format[0], L"Non-existent process (");
            PhInitFormatIU(&format[1], (ULONG_PTR)ClientId->UniqueProcess);
            PhInitFormatC(&format[2], ')');

            name = PhFormat(format, 3, 0);
        }
    }

    return name;
}

PWSTR PhGetProcessPriorityClassString(
    _In_ ULONG PriorityClass
    )
{
    switch (PriorityClass)
    {
    case PROCESS_PRIORITY_CLASS_REALTIME:
        return L"Real time";
    case PROCESS_PRIORITY_CLASS_HIGH:
        return L"High";
    case PROCESS_PRIORITY_CLASS_ABOVE_NORMAL:
        return L"Above normal";
    case PROCESS_PRIORITY_CLASS_NORMAL:
        return L"Normal";
    case PROCESS_PRIORITY_CLASS_BELOW_NORMAL:
        return L"Below normal";
    case PROCESS_PRIORITY_CLASS_IDLE:
        return L"Idle";
    default:
        return L"Unknown";
    }
}

/**
 * Creates a process item.
 */
PPH_PROCESS_ITEM PhCreateProcessItem(
    _In_ HANDLE ProcessId
    )
{
    PPH_PROCESS_ITEM processItem;

    processItem = PhCreateObject(
        PhEmGetObjectSize(EmProcessItemType, sizeof(PH_PROCESS_ITEM)),
        PhProcessItemType
        );
    memset(processItem, 0, sizeof(PH_PROCESS_ITEM));
    PhInitializeEvent(&processItem->Stage1Event);
    PhInitializeQueuedLock(&processItem->ServiceListLock);

    processItem->ProcessId = ProcessId;

    // Create the statistics buffers.
    PhInitializeCircularBuffer_FLOAT(&processItem->CpuKernelHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_FLOAT(&processItem->CpuUserHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_ULONG64(&processItem->IoReadHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_ULONG64(&processItem->IoWriteHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_ULONG64(&processItem->IoOtherHistory, PhStatisticsSampleCount);
    PhInitializeCircularBuffer_SIZE_T(&processItem->PrivateBytesHistory, PhStatisticsSampleCount);
    //PhInitializeCircularBuffer_SIZE_T(&processItem->WorkingSetHistory, PhStatisticsSampleCount);

    PhEmCallObjectOperation(EmProcessItemType, processItem, EmObjectCreate);

    return processItem;
}

VOID PhpProcessItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Object;
    ULONG i;

    PhEmCallObjectOperation(EmProcessItemType, processItem, EmObjectDelete);

    PhDeleteCircularBuffer_FLOAT(&processItem->CpuKernelHistory);
    PhDeleteCircularBuffer_FLOAT(&processItem->CpuUserHistory);
    PhDeleteCircularBuffer_ULONG64(&processItem->IoReadHistory);
    PhDeleteCircularBuffer_ULONG64(&processItem->IoWriteHistory);
    PhDeleteCircularBuffer_ULONG64(&processItem->IoOtherHistory);
    PhDeleteCircularBuffer_SIZE_T(&processItem->PrivateBytesHistory);
    //PhDeleteCircularBuffer_SIZE_T(&processItem->WorkingSetHistory);

    if (processItem->ServiceList)
    {
        PPH_SERVICE_ITEM serviceItem;

        i = 0;

        while (PhEnumPointerList(processItem->ServiceList, &i, &serviceItem))
            PhDereferenceObject(serviceItem);

        PhDereferenceObject(processItem->ServiceList);
    }

    if (processItem->ProcessName) PhDereferenceObject(processItem->ProcessName);
    if (processItem->FileName) PhDereferenceObject(processItem->FileName);
    if (processItem->CommandLine) PhDereferenceObject(processItem->CommandLine);
    if (processItem->SmallIcon) DestroyIcon(processItem->SmallIcon);
    if (processItem->LargeIcon) DestroyIcon(processItem->LargeIcon);
    PhDeleteImageVersionInfo(&processItem->VersionInfo);
    if (processItem->Sid) PhFree(processItem->Sid);
    if (processItem->JobName) PhDereferenceObject(processItem->JobName);
    if (processItem->VerifySignerName) PhDereferenceObject(processItem->VerifySignerName);
    if (processItem->PackageFullName) PhDereferenceObject(processItem->PackageFullName);

    if (processItem->QueryHandle) NtClose(processItem->QueryHandle);

    if (processItem->Record) PhDereferenceProcessRecord(processItem->Record);
}

FORCEINLINE BOOLEAN PhCompareProcessItem(
    _In_ PPH_PROCESS_ITEM Value1,
    _In_ PPH_PROCESS_ITEM Value2
    )
{
    return Value1->ProcessId == Value2->ProcessId;
}

FORCEINLINE ULONG PhHashProcessItem(
    _In_ PPH_PROCESS_ITEM Value
    )
{
    return HandleToUlong(Value->ProcessId) / 4;
}

/**
 * Finds a process item in the hash set.
 *
 * \param ProcessId The process ID of the process item.
 *
 * \remarks The hash set must be locked before calling this function. The reference count of the
 * found process item is not incremented.
 */
PPH_PROCESS_ITEM PhpLookupProcessItem(
    _In_ HANDLE ProcessId
    )
{
    PH_PROCESS_ITEM lookupProcessItem;
    PPH_HASH_ENTRY entry;
    PPH_PROCESS_ITEM processItem;

    lookupProcessItem.ProcessId = ProcessId;
    entry = PhFindEntryHashSet(
        PhProcessHashSet,
        PH_HASH_SET_SIZE(PhProcessHashSet),
        PhHashProcessItem(&lookupProcessItem)
        );

    for (; entry; entry = entry->Next)
    {
        processItem = CONTAINING_RECORD(entry, PH_PROCESS_ITEM, HashEntry);

        if (PhCompareProcessItem(&lookupProcessItem, processItem))
            return processItem;
    }

    return NULL;
}

/**
 * Finds and references a process item.
 *
 * \param ProcessId The process ID of the process item.
 *
 * \return The found process item.
 */
PPH_PROCESS_ITEM PhReferenceProcessItem(
    _In_ HANDLE ProcessId
    )
{
    PPH_PROCESS_ITEM processItem;

    PhAcquireQueuedLockShared(&PhProcessHashSetLock);

    processItem = PhpLookupProcessItem(ProcessId);

    if (processItem)
        PhReferenceObject(processItem);

    PhReleaseQueuedLockShared(&PhProcessHashSetLock);

    return processItem;
}

/**
 * Enumerates the process items.
 *
 * \param ProcessItems A variable which receives an array of pointers to process items. You must
 * free the buffer with PhFree() when you no longer need it.
 * \param NumberOfProcessItems A variable which receives the number of process items returned in
 * \a ProcessItems.
 */
VOID PhEnumProcessItems(
    _Out_opt_ PPH_PROCESS_ITEM **ProcessItems,
    _Out_ PULONG NumberOfProcessItems
    )
{
    PPH_PROCESS_ITEM *processItems;
    ULONG numberOfProcessItems;
    ULONG count = 0;
    ULONG i;
    PPH_HASH_ENTRY entry;
    PPH_PROCESS_ITEM processItem;

    if (!ProcessItems)
    {
        *NumberOfProcessItems = PhProcessHashSetCount;
        return;
    }

    PhAcquireQueuedLockShared(&PhProcessHashSetLock);

    numberOfProcessItems = PhProcessHashSetCount;
    processItems = PhAllocate(sizeof(PPH_PROCESS_ITEM) * numberOfProcessItems);

    for (i = 0; i < PH_HASH_SET_SIZE(PhProcessHashSet); i++)
    {
        for (entry = PhProcessHashSet[i]; entry; entry = entry->Next)
        {
            processItem = CONTAINING_RECORD(entry, PH_PROCESS_ITEM, HashEntry);
            PhReferenceObject(processItem);
            processItems[count++] = processItem;
        }
    }

    PhReleaseQueuedLockShared(&PhProcessHashSetLock);

    *ProcessItems = processItems;
    *NumberOfProcessItems = numberOfProcessItems;
}

VOID PhpAddProcessItem(
    _In_ _Assume_refs_(1) PPH_PROCESS_ITEM ProcessItem
    )
{
    PhAddEntryHashSet(
        PhProcessHashSet,
        PH_HASH_SET_SIZE(PhProcessHashSet),
        &ProcessItem->HashEntry,
        PhHashProcessItem(ProcessItem)
        );
    PhProcessHashSetCount++;
}

VOID PhpRemoveProcessItem(
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PhRemoveEntryHashSet(PhProcessHashSet, PH_HASH_SET_SIZE(PhProcessHashSet), &ProcessItem->HashEntry);
    PhProcessHashSetCount--;
    PhDereferenceObject(ProcessItem);
}

BOOLEAN PhpSidFullNameCacheHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_SID_FULL_NAME_CACHE_ENTRY entry1 = Entry1;
    PPH_SID_FULL_NAME_CACHE_ENTRY entry2 = Entry2;

    return RtlEqualSid(entry1->Sid, entry2->Sid);
}

ULONG PhpSidFullNameCacheHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PPH_SID_FULL_NAME_CACHE_ENTRY entry = Entry;

    return PhHashBytes(entry->Sid, RtlLengthSid(entry->Sid));
}

PPH_STRING PhpGetSidFullNameCached(
    _In_ PSID Sid
    )
{
    PPH_STRING fullName;
    PH_SID_FULL_NAME_CACHE_ENTRY newEntry;

    if (PhpSidFullNameCacheHashtable)
    {
        PPH_SID_FULL_NAME_CACHE_ENTRY entry;
        PH_SID_FULL_NAME_CACHE_ENTRY lookupEntry;

        lookupEntry.Sid = Sid;
        entry = PhFindEntryHashtable(PhpSidFullNameCacheHashtable, &lookupEntry);

        if (entry)
            return PhReferenceObject(entry->FullName);
    }

    fullName = PhGetSidFullName(Sid, TRUE, NULL);

    if (!fullName)
        return NULL;

    if (!PhpSidFullNameCacheHashtable)
    {
        PhpSidFullNameCacheHashtable = PhCreateHashtable(
            sizeof(PH_SID_FULL_NAME_CACHE_ENTRY),
            PhpSidFullNameCacheHashtableEqualFunction,
            PhpSidFullNameCacheHashtableHashFunction,
            16
            );
    }

    newEntry.Sid = PhAllocateCopy(Sid, RtlLengthSid(Sid));
    newEntry.FullName = PhReferenceObject(fullName);
    PhAddEntryHashtable(PhpSidFullNameCacheHashtable, &newEntry);

    return fullName;
}

VOID PhpFlushSidFullNameCache(
    VOID
    )
{
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PPH_SID_FULL_NAME_CACHE_ENTRY entry;

    if (!PhpSidFullNameCacheHashtable)
        return;

    PhBeginEnumHashtable(PhpSidFullNameCacheHashtable, &enumContext);

    while (entry = PhNextEnumHashtable(&enumContext))
    {
        PhFree(entry->Sid);
        PhDereferenceObject(entry->FullName);
    }

    PhClearReference(&PhpSidFullNameCacheHashtable);
}

VOID PhpProcessQueryStage1(
    _Inout_ PPH_PROCESS_QUERY_S1_DATA Data
    )
{
    NTSTATUS status;
    PPH_PROCESS_ITEM processItem = Data->Header.ProcessItem;
    HANDLE processId = processItem->ProcessId;
    HANDLE processHandleLimited = processItem->QueryHandle;

    if (processItem->FileName && !processItem->IsSubsystemProcess)
    {
        if (!PhExtractIcon(
            processItem->FileName->Buffer,
            &Data->LargeIcon,
            &Data->SmallIcon
            ))
        {
            Data->LargeIcon = NULL;
            Data->SmallIcon = NULL;
        }

        // Version info.
        PhInitializeImageVersionInfo(&Data->VersionInfo, processItem->FileName->Buffer);
    }

    // Debugged
    if (processHandleLimited && !processItem->IsSubsystemProcess)
    {
        BOOLEAN isBeingDebugged;

        if (NT_SUCCESS(PhGetProcessIsBeingDebugged(processHandleLimited, &isBeingDebugged)))
        {
            Data->IsBeingDebugged = isBeingDebugged;
        }
    }

    // Command line, .NET
    if (processHandleLimited && !processItem->IsSubsystemProcess)
    {
        BOOLEAN isDotNet = FALSE;
        HANDLE processHandle = NULL;
        ULONG processQueryFlags = 0;

        if (WindowsVersion >= WINDOWS_8_1)
        {
            processHandle = processHandleLimited;
            processQueryFlags |= PH_CLR_USE_SECTION_CHECK;
            status = STATUS_SUCCESS;
        }
        else
        {
            status = PhOpenProcess(
                &processHandle,
                PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
                processId
                );
        }

        if (NT_SUCCESS(status))
        {
            PPH_STRING commandLine;

            if (NT_SUCCESS(PhGetProcessCommandLine(processHandle, &commandLine)))
            {
                // Some command lines (e.g. from taskeng.exe) have nulls in them. Since Windows
                // can't display them, we'll replace them with spaces.
                for (ULONG i = 0; i < (ULONG)commandLine->Length / sizeof(WCHAR); i++)
                {
                    if (commandLine->Buffer[i] == 0)
                        commandLine->Buffer[i] = ' ';
                }

                Data->CommandLine = commandLine;
            }
        }

        if (NT_SUCCESS(status))
        {
            PhGetProcessIsDotNetEx(
                processId,
                processHandle,
#ifdef _WIN64
                processQueryFlags | PH_CLR_NO_WOW64_CHECK | (processItem->IsWow64 ? PH_CLR_KNOWN_IS_WOW64 : 0),
#else
                processQueryFlags,
#endif
                &isDotNet,
                NULL
                );
            Data->IsDotNet = isDotNet;
        }

        if (!(processQueryFlags & PH_CLR_USE_SECTION_CHECK) && processHandle)
            NtClose(processHandle);
    }

    // Job
    if (processHandleLimited)
    {
        if (KphIsConnected())
        {
            HANDLE jobHandle = NULL;

            status = KphOpenProcessJob(
                processHandleLimited,
                JOB_OBJECT_QUERY,
                &jobHandle
                );

            if (NT_SUCCESS(status) && status != STATUS_PROCESS_NOT_IN_JOB)
            {
                JOBOBJECT_BASIC_LIMIT_INFORMATION basicLimits;

                Data->IsInJob = TRUE;

                PhGetHandleInformation(
                    NtCurrentProcess(),
                    jobHandle,
                    ULONG_MAX,
                    NULL,
                    NULL,
                    NULL,
                    &Data->JobName
                    );

                // Process Explorer only recognizes processes as being in jobs if they don't have
                // the silent-breakaway-OK limit as their only limit. Emulate this behaviour.
                if (NT_SUCCESS(PhGetJobBasicLimits(jobHandle, &basicLimits)))
                {
                    Data->IsInSignificantJob =
                        basicLimits.LimitFlags != JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
                }
            }

            if (jobHandle)
                NtClose(jobHandle);
        }
        else
        {
            // KProcessHacker is not available. We can determine if the process is in a job, but we
            // can't get a handle to the job.

            status = NtIsProcessInJob(processHandleLimited, NULL);

            if (NT_SUCCESS(status))
                Data->IsInJob = status == STATUS_PROCESS_IN_JOB;
        }
    }

    // Console host process
    if (processHandleLimited)
    {
        PhGetProcessConsoleHostProcessId(processHandleLimited, &Data->ConsoleHostProcessId);
    }

    // Immersive
    if (processHandleLimited && IsImmersiveProcess_I && !processItem->IsSubsystemProcess)
    {
        Data->IsImmersive = !!IsImmersiveProcess_I(processHandleLimited);
    }

    // Package full name
    if (processHandleLimited && WINDOWS_HAS_IMMERSIVE && Data->IsImmersive)
    {
        Data->PackageFullName = PhGetProcessPackageFullName(processHandleLimited);
    }

    if (processHandleLimited && processItem->IsHandleValid)
    {
        OBJECT_BASIC_INFORMATION basicInfo;

        if (NT_SUCCESS(PhGetHandleInformationEx(
            NtCurrentProcess(),
            processHandleLimited,
            ULONG_MAX,
            0,
            NULL,
            &basicInfo,
            NULL,
            NULL,
            NULL,
            NULL
            )))
        {
            if ((basicInfo.GrantedAccess & PROCESS_QUERY_INFORMATION) != PROCESS_QUERY_INFORMATION)
                Data->IsFilteredHandle = TRUE;
        }
        else
        {
            Data->IsFilteredHandle = TRUE;
        }
    }
}

VOID PhpProcessQueryStage2(
    _Inout_ PPH_PROCESS_QUERY_S2_DATA Data
    )
{
    PPH_PROCESS_ITEM processItem = Data->Header.ProcessItem;

    if (PhEnableProcessQueryStage2 && processItem->FileName && !processItem->IsSubsystemProcess)
    {
        NTSTATUS status;

        Data->VerifyResult = PhVerifyFileCached(
            processItem->FileName,
            processItem->PackageFullName,
            &Data->VerifySignerName,
            FALSE
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
    _In_ PVOID Parameter
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
    _In_ PVOID Parameter
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
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PH_WORK_QUEUE_ENVIRONMENT environment;

    // Ref: dereferenced when the provider update function removes the item from the queue.
    PhReferenceObject(ProcessItem);

    PhInitializeWorkQueueEnvironment(&environment);
    environment.BasePriority = THREAD_PRIORITY_BELOW_NORMAL;

    PhQueueItemWorkQueueEx(PhGetGlobalWorkQueue(), PhpProcessQueryStage1Worker, ProcessItem,
        NULL, &environment);
}

VOID PhpQueueProcessQueryStage2(
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PH_WORK_QUEUE_ENVIRONMENT environment;

    if (!PhEnableProcessQueryStage2)
        return;

    PhReferenceObject(ProcessItem);

    PhInitializeWorkQueueEnvironment(&environment);
    environment.BasePriority = THREAD_PRIORITY_BELOW_NORMAL;
    environment.IoPriority = IoPriorityVeryLow;
    environment.PagePriority = MEMORY_PRIORITY_VERY_LOW;

    PhQueueItemWorkQueueEx(PhGetGlobalWorkQueue(), PhpProcessQueryStage2Worker, ProcessItem,
        NULL, &environment);
}

VOID PhpFillProcessItemStage1(
    _In_ PPH_PROCESS_QUERY_S1_DATA Data
    )
{
    PPH_PROCESS_ITEM processItem = Data->Header.ProcessItem;

    processItem->CommandLine = Data->CommandLine;
    processItem->SmallIcon = Data->SmallIcon;
    processItem->LargeIcon = Data->LargeIcon;
    memcpy(&processItem->VersionInfo, &Data->VersionInfo, sizeof(PH_IMAGE_VERSION_INFO));
    processItem->JobName = Data->JobName;
    processItem->ConsoleHostProcessId = Data->ConsoleHostProcessId;
    processItem->PackageFullName = Data->PackageFullName;
    processItem->IsDotNet = Data->IsDotNet;
    processItem->IsInJob = Data->IsInJob;
    processItem->IsInSignificantJob = Data->IsInSignificantJob;
    processItem->IsBeingDebugged = Data->IsBeingDebugged;
    processItem->IsImmersive = Data->IsImmersive;
    processItem->IsProtectedHandle = Data->IsFilteredHandle;

    PhSwapReference(&processItem->Record->CommandLine, processItem->CommandLine);

    // Note: Queue stage 2 processing after filling stage1 process data. 
    PhpQueueProcessQueryStage2(processItem);
}

VOID PhpFillProcessItemStage2(
    _In_ PPH_PROCESS_QUERY_S2_DATA Data
    )
{
    PPH_PROCESS_ITEM processItem = Data->Header.ProcessItem;

    processItem->VerifyResult = Data->VerifyResult;
    processItem->VerifySignerName = Data->VerifySignerName;
    processItem->IsPacked = Data->IsPacked;
    processItem->ImportFunctions = Data->ImportFunctions;
    processItem->ImportModules = Data->ImportModules;
}

VOID PhpFillProcessItemExtension(
    _Inout_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PSYSTEM_PROCESS_INFORMATION Process
    )
{
    PSYSTEM_PROCESS_INFORMATION_EXTENSION processExtension;

    if (WindowsVersion < WINDOWS_10_RS3 || PhIsExecutingInWow64())
        return;
    
    processExtension = PH_PROCESS_EXTENSION(Process);

    ProcessItem->JobObjectId = processExtension->JobObjectId;
    ProcessItem->ProcessSequenceNumber = processExtension->ProcessSequenceNumber;
}

VOID PhpFillProcessItem(
    _Inout_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PSYSTEM_PROCESS_INFORMATION Process
    )
{
    ProcessItem->ParentProcessId = Process->InheritedFromUniqueProcessId;
    ProcessItem->SessionId = Process->SessionId;
    ProcessItem->CreateTime = Process->CreateTime;

    if (ProcessItem->ProcessId != SYSTEM_IDLE_PROCESS_ID)
        ProcessItem->ProcessName = PhCreateStringFromUnicodeString(&Process->ImageName);
    else
        ProcessItem->ProcessName = PhCreateString(SYSTEM_IDLE_PROCESS_NAME);

    if (PH_IS_REAL_PROCESS_ID(ProcessItem->ProcessId))
    {
        if (PhEnableHexId)
            PhPrintUInt32Hex(ProcessItem->ProcessIdString, HandleToUlong(ProcessItem->ProcessId));
        else
            PhPrintUInt32(ProcessItem->ProcessIdString, HandleToUlong(ProcessItem->ProcessId));
    }

    PhPrintUInt32(ProcessItem->ParentProcessIdString, HandleToUlong(ProcessItem->ParentProcessId));
    PhPrintUInt32(ProcessItem->SessionIdString, ProcessItem->SessionId);

    // Open a handle to the process for later usage.
    if (PH_IS_REAL_PROCESS_ID(ProcessItem->ProcessId))
    {
        if (NT_SUCCESS(PhOpenProcess(&ProcessItem->QueryHandle, PROCESS_QUERY_INFORMATION, ProcessItem->ProcessId)))
        {
            ProcessItem->IsHandleValid = TRUE;
        }

        if (!ProcessItem->QueryHandle)
        {
            PhOpenProcess(&ProcessItem->QueryHandle, PROCESS_QUERY_LIMITED_INFORMATION, ProcessItem->ProcessId);
        }
    }

    // Process flags
    if (ProcessItem->QueryHandle)
    {
        PROCESS_EXTENDED_BASIC_INFORMATION basicInfo;

        if (NT_SUCCESS(PhGetProcessExtendedBasicInformation(ProcessItem->QueryHandle, &basicInfo)))
        {
            ProcessItem->IsProtectedProcess = basicInfo.IsProtectedProcess;
            ProcessItem->IsSecureProcess = basicInfo.IsSecureProcess;
            ProcessItem->IsSubsystemProcess = basicInfo.IsSubsystemProcess;
            ProcessItem->IsWow64 = basicInfo.IsWow64Process;
            ProcessItem->IsWow64Valid = TRUE;
        }
    }

    // Process information
    {
        // If we're dealing with System (PID 4), we need to get the
        // kernel file name. Otherwise, get the image file name. (wj32)

        if (ProcessItem->ProcessId != SYSTEM_PROCESS_ID)
        {
            PPH_STRING fileName;

            if (ProcessItem->QueryHandle && !ProcessItem->IsSubsystemProcess)
            {
                PhGetProcessImageFileNameWin32(ProcessItem->QueryHandle, &ProcessItem->FileName);
            }
            else
            {
                if (NT_SUCCESS(PhGetProcessImageFileNameByProcessId(ProcessItem->ProcessId, &fileName)))
                {
                    ProcessItem->FileName = PhGetFileName(fileName);
                    PhDereferenceObject(fileName);
                }
            }
        }
        else
        {
            PPH_STRING fileName;

            fileName = PhGetKernelFileName();

            if (fileName)
            {
                ProcessItem->FileName = PhGetFileName(fileName);
                PhDereferenceObject(fileName);
            }
        }
    }

    // Token information
    if (
        ProcessItem->QueryHandle &&
        ProcessItem->ProcessId != SYSTEM_PROCESS_ID // System token can't be opened (dmex)
        )
    {
        HANDLE tokenHandle;

        if (NT_SUCCESS(PhOpenProcessToken(
            ProcessItem->QueryHandle, 
            TOKEN_QUERY, 
            &tokenHandle
            )))
        {
            PTOKEN_USER tokenUser;
            TOKEN_ELEVATION_TYPE elevationType;
            MANDATORY_LEVEL integrityLevel;
            PWSTR integrityString;

            // User
            if (NT_SUCCESS(PhGetTokenUser(tokenHandle, &tokenUser)))
            {
                ProcessItem->Sid = PhAllocateCopy(tokenUser->User.Sid, RtlLengthSid(tokenUser->User.Sid));
                PhFree(tokenUser);
            }

            // Elevation
            if (NT_SUCCESS(PhGetTokenElevationType(tokenHandle, &elevationType)))
            {
                ProcessItem->ElevationType = elevationType;
                ProcessItem->IsElevated = elevationType == TokenElevationTypeFull;
            }

            // Integrity
            if (NT_SUCCESS(PhGetTokenIntegrityLevel(tokenHandle, &integrityLevel, &integrityString)))
            {
                ProcessItem->IntegrityLevel = integrityLevel;
                ProcessItem->IntegrityString = integrityString;
            }

            NtClose(tokenHandle);
        }
    }
    else
    {
        if (ProcessItem->ProcessId == SYSTEM_IDLE_PROCESS_ID ||
            ProcessItem->ProcessId == SYSTEM_PROCESS_ID) // System token can't be opened on XP (wj32)
        {
            ProcessItem->Sid = PhAllocateCopy(&PhSeLocalSystemSid, RtlLengthSid(&PhSeLocalSystemSid));
        }
    }

    // Known Process Type
    {
        ProcessItem->KnownProcessType = PhGetProcessKnownTypeEx(
            ProcessItem->ProcessId,
            ProcessItem->FileName
            );
    }

    // Protection
    if (ProcessItem->QueryHandle)
    {
        if (WindowsVersion >= WINDOWS_8_1)
        {
            PS_PROTECTION protection;

            if (NT_SUCCESS(PhGetProcessProtection(ProcessItem->QueryHandle, &protection)))
            {
                ProcessItem->Protection.Level = protection.Level;
            }
        }
        else
        {
            // HACK: 'emulate' the PS_PROTECTION info for older OSes. (ge0rdi)
            if (ProcessItem->IsProtectedProcess)
                ProcessItem->Protection.Type = PsProtectedTypeProtected;
        }
    }
    else
    {
        // Signalize that we weren't able to get protection info with a special value.
        // Note: We use this value to determine if we should show protection information. (ge0rdi)
        ProcessItem->Protection.Level = UCHAR_MAX;
    }

    // Control Flow Guard
    if (WindowsVersion >= WINDOWS_8_1 && ProcessItem->QueryHandle)
    {
        BOOLEAN cfguardEnabled;

        if (NT_SUCCESS(PhGetProcessIsCFGuardEnabled(ProcessItem->QueryHandle, &cfguardEnabled)))
        {
            ProcessItem->IsControlFlowGuardEnabled = cfguardEnabled;
        }
    }

    // On Windows 8.1 and above, processes without threads are reflected processes 
    // which will not terminate if we have a handle open. (wj32)
    if (Process->NumberOfThreads == 0 && ProcessItem->QueryHandle)
    {
        NtClose(ProcessItem->QueryHandle);
        ProcessItem->QueryHandle = NULL;
    }
}

FORCEINLINE VOID PhpUpdateDynamicInfoProcessItem(
    _Inout_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PSYSTEM_PROCESS_INFORMATION Process
    )
{
    ProcessItem->BasePriority = Process->BasePriority;

    if (ProcessItem->QueryHandle)
    {
        PROCESS_PRIORITY_CLASS priorityClass;

        if (NT_SUCCESS(PhGetProcessPriority(ProcessItem->QueryHandle, &priorityClass)))
        {
            ProcessItem->PriorityClass = priorityClass.PriorityClass;
        }
    }
    else
    {
        ProcessItem->PriorityClass = 0;
    }

    ProcessItem->KernelTime = Process->KernelTime;
    ProcessItem->UserTime = Process->UserTime;
    ProcessItem->NumberOfHandles = Process->HandleCount;
    ProcessItem->NumberOfThreads = Process->NumberOfThreads;
    ProcessItem->WorkingSetPrivateSize = (SIZE_T)Process->WorkingSetPrivateSize.QuadPart;
    ProcessItem->PeakNumberOfThreads = Process->NumberOfThreadsHighWatermark;
    ProcessItem->HardFaultCount = Process->HardFaultCount;

    // Update VM and I/O counters.
    ProcessItem->VmCounters = *(PVM_COUNTERS_EX)&Process->PeakVirtualSize;
    ProcessItem->IoCounters = *(PIO_COUNTERS)&Process->ReadOperationCount;
}

VOID PhpUpdatePerfInformation(
    VOID
    )
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
    _In_ BOOLEAN SetCpuUsage,
    _Out_ PULONG64 TotalTime
    )
{
    ULONG i;
    ULONG64 totalTime;

    NtQuerySystemInformation(
        SystemProcessorPerformanceInformation,
        PhCpuInformation,
        sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * (ULONG)PhSystemBasicInformation.NumberOfProcessors,
        NULL
        );

    // Zero the CPU totals.
    memset(&PhCpuTotals, 0, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION));

    for (i = 0; i < (ULONG)PhSystemBasicInformation.NumberOfProcessors; i++)
    {
        PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION cpuInfo =
            &PhCpuInformation[i];

        // KernelTime includes IdleTime.
        cpuInfo->KernelTime.QuadPart -= cpuInfo->IdleTime.QuadPart;

        PhCpuTotals.DpcTime.QuadPart += cpuInfo->DpcTime.QuadPart;
        PhCpuTotals.IdleTime.QuadPart += cpuInfo->IdleTime.QuadPart;
        PhCpuTotals.InterruptCount += cpuInfo->InterruptCount;
        PhCpuTotals.InterruptTime.QuadPart += cpuInfo->InterruptTime.QuadPart;
        PhCpuTotals.KernelTime.QuadPart += cpuInfo->KernelTime.QuadPart;
        PhCpuTotals.UserTime.QuadPart += cpuInfo->UserTime.QuadPart;

        PhUpdateDelta(&PhCpusKernelDelta[i], cpuInfo->KernelTime.QuadPart);
        PhUpdateDelta(&PhCpusUserDelta[i], cpuInfo->UserTime.QuadPart);
        PhUpdateDelta(&PhCpusIdleDelta[i], cpuInfo->IdleTime.QuadPart);

        if (SetCpuUsage)
        {
            totalTime = PhCpusKernelDelta[i].Delta + PhCpusUserDelta[i].Delta + PhCpusIdleDelta[i].Delta;

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
    }

    PhUpdateDelta(&PhCpuKernelDelta, PhCpuTotals.KernelTime.QuadPart);
    PhUpdateDelta(&PhCpuUserDelta, PhCpuTotals.UserTime.QuadPart);
    PhUpdateDelta(&PhCpuIdleDelta, PhCpuTotals.IdleTime.QuadPart);

    totalTime = PhCpuKernelDelta.Delta + PhCpuUserDelta.Delta + PhCpuIdleDelta.Delta;

    if (SetCpuUsage)
    {
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
    }

    *TotalTime = totalTime;
}

VOID PhpUpdateCpuCycleInformation(
    _Out_ PULONG64 IdleCycleTime
    )
{
    ULONG i;
    ULONG64 total;

    // Idle

    // We need to query this separately because the idle cycle time in SYSTEM_PROCESS_INFORMATION
    // doesn't give us data for individual processors.

    NtQuerySystemInformation(
        SystemProcessorIdleCycleTimeInformation,
        PhCpuIdleCycleTime,
        sizeof(LARGE_INTEGER) * (ULONG)PhSystemBasicInformation.NumberOfProcessors,
        NULL
        );

    total = 0;

    for (i = 0; i < (ULONG)PhSystemBasicInformation.NumberOfProcessors; i++)
    {
        //PhUpdateDelta(&PhCpusIdleCycleDelta[i], PhCpuIdleCycleTime[i].QuadPart);
        total += PhCpuIdleCycleTime[i].QuadPart;
    }

    PhUpdateDelta(&PhCpuIdleCycleDelta, total);
    *IdleCycleTime = PhCpuIdleCycleDelta.Delta;

    // System

    NtQuerySystemInformation(
        SystemProcessorCycleTimeInformation,
        PhCpuSystemCycleTime,
        sizeof(LARGE_INTEGER) * (ULONG)PhSystemBasicInformation.NumberOfProcessors,
        NULL
        );

    total = 0;

    for (i = 0; i < (ULONG)PhSystemBasicInformation.NumberOfProcessors; i++)
    {
        total += PhCpuSystemCycleTime[i].QuadPart;
    }

    PhUpdateDelta(&PhCpuSystemCycleDelta, total);
}

VOID PhpUpdateCpuCycleUsageInformation(
    _In_ ULONG64 TotalCycleTime,
    _In_ ULONG64 IdleCycleTime
    )
{
    ULONG i;
    FLOAT baseCpuUsage;
    FLOAT totalTimeDelta;
    ULONG64 totalTime;

    // Cycle time is not only lacking for kernel/user components, but also for individual
    // processors. We can get the total idle cycle time for individual processors but
    // without knowing the total cycle time for individual processors, this information
    // is useless.
    //
    // We'll start by calculating the total CPU usage, then we'll calculate the kernel/user
    // components. In the event that the corresponding CPU time deltas are zero, we'll split
    // the CPU usage evenly across the kernel/user components. CPU usage for individual
    // processors is left untouched, because it's too difficult to provide an estimate.
    //
    // Let I_1, I_2, ..., I_n be the idle cycle times and T_1, T_2, ..., T_n be the
    // total cycle times. Let I'_1, I'_2, ..., I'_n be the idle CPU times and T'_1, T'_2, ...,
    // T'_n be the total CPU times.
    // We know all I'_n, T'_n and I_n, but we only know sigma(T). The "real" total CPU usage is
    // sigma(I)/sigma(T), and the "real" individual CPU usage is I_n/T_n. The problem is that
    // we don't know T_n; we only know sigma(T). Hence we need to find values i_1, i_2, ..., i_n
    // and t_1, t_2, ..., t_n such that:
    // sigma(i)/sigma(t) ~= sigma(I)/sigma(T), and
    // i_n/t_n ~= I_n/T_n
    //
    // Solution 1: Set i_n = I_n and t_n = sigma(T)*T'_n/sigma(T'). Then:
    // sigma(i)/sigma(t) = sigma(I)/(sigma(T)*sigma(T')/sigma(T')) = sigma(I)/sigma(T), and
    // i_n/t_n = I_n/T'_n*sigma(T')/sigma(T) ~= I_n/T_n since I_n/T'_n ~= I_n/T_n and sigma(T')/sigma(T) ~= 1.
    // However, it is not guaranteed that i_n/t_n <= 1, which may lead to CPU usages over 100% being displayed.
    //
    // Solution 2: Set i_n = I'_n and t_n = T'_n. Then:
    // sigma(i)/sigma(t) = sigma(I')/sigma(T') ~= sigma(I)/sigma(T) since I'_n ~= I_n and T'_n ~= T_n.
    // i_n/t_n = I'_n/T'_n ~= I_n/T_n as above.
    // Not scaling at all is currently the best solution, since it's fast, simple and guarantees that i_n/t_n <= 1.

    baseCpuUsage = 1 - (FLOAT)IdleCycleTime / TotalCycleTime;
    totalTimeDelta = (FLOAT)(PhCpuKernelDelta.Delta + PhCpuUserDelta.Delta);

    if (totalTimeDelta != 0)
    {
        PhCpuKernelUsage = baseCpuUsage * ((FLOAT)PhCpuKernelDelta.Delta / totalTimeDelta);
        PhCpuUserUsage = baseCpuUsage * ((FLOAT)PhCpuUserDelta.Delta / totalTimeDelta);
    }
    else
    {
        PhCpuKernelUsage = baseCpuUsage / 2;
        PhCpuUserUsage = baseCpuUsage / 2;
    }

    for (i = 0; i < (ULONG)PhSystemBasicInformation.NumberOfProcessors; i++)
    {
        totalTime = PhCpusKernelDelta[i].Delta + PhCpusUserDelta[i].Delta + PhCpusIdleDelta[i].Delta;

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
}

VOID PhpInitializeProcessStatistics(
    VOID
    )
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

VOID PhpUpdateSystemHistory(
    VOID
    )
{
    ULONG i;
    LARGE_INTEGER systemTime;
    ULONG secondsSince1980;

    // CPU
    PhAddItemCircularBuffer_FLOAT(&PhCpuKernelHistory, PhCpuKernelUsage);
    PhAddItemCircularBuffer_FLOAT(&PhCpuUserHistory, PhCpuUserUsage);

    // CPUs
    for (i = 0; i < (ULONG)PhSystemBasicInformation.NumberOfProcessors; i++)
    {
        PhAddItemCircularBuffer_FLOAT(&PhCpusKernelHistory[i], PhCpusKernelUsage[i]);
        PhAddItemCircularBuffer_FLOAT(&PhCpusUserHistory[i], PhCpusUserUsage[i]);
    }

    // I/O
    PhAddItemCircularBuffer_ULONG64(&PhIoReadHistory, PhIoReadDelta.Delta);
    PhAddItemCircularBuffer_ULONG64(&PhIoWriteHistory, PhIoWriteDelta.Delta);
    PhAddItemCircularBuffer_ULONG64(&PhIoOtherHistory, PhIoOtherDelta.Delta);

    // Memory
    PhAddItemCircularBuffer_ULONG(&PhCommitHistory, PhPerfInformation.CommittedPages);
    PhAddItemCircularBuffer_ULONG(&PhPhysicalHistory,
        PhSystemBasicInformation.NumberOfPhysicalPages - PhPerfInformation.AvailablePages
        );

    // Time
    PhQuerySystemTime(&systemTime);
    RtlTimeToSecondsSince1980(&systemTime, &secondsSince1980);
    PhAddItemCircularBuffer_ULONG(&PhTimeHistory, secondsSince1980);
}

/**
 * Retrieves a time value recorded by the statistics system.
 *
 * \param ProcessItem A process item to synchronize with, or NULL if no synchronization is
 * necessary.
 * \param Index The history index.
 * \param Time A variable which receives the time at \a Index.
 *
 * \return TRUE if the function succeeded, otherwise FALSE if \a ProcessItem was specified and
 * \a Index is too far into the past for that process item.
 */
BOOLEAN PhGetStatisticsTime(
    _In_opt_ PPH_PROCESS_ITEM ProcessItem,
    _In_ ULONG Index,
    _Out_ PLARGE_INTEGER Time
    )
{
    ULONG secondsSince1980;
    ULONG index;
    LARGE_INTEGER time;

    if (ProcessItem)
    {
        // The sequence number is used to synchronize statistics when a process exits, since that
        // process' history is not updated anymore.
        index = PhTimeSequenceNumber - ProcessItem->TimeSequenceNumber + Index;

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

    secondsSince1980 = PhGetItemCircularBuffer_ULONG(&PhTimeHistory, index);
    RtlSecondsSince1980ToTime(secondsSince1980, &time);

    *Time = time;

    return TRUE;
}

PPH_STRING PhGetStatisticsTimeString(
    _In_opt_ PPH_PROCESS_ITEM ProcessItem,
    _In_ ULONG Index
    )
{
    LARGE_INTEGER time;
    SYSTEMTIME systemTime;

    if (PhGetStatisticsTime(ProcessItem, Index, &time))
    {
        PhLargeIntegerToLocalSystemTime(&systemTime, &time);

        return PhFormatDateTime(&systemTime);
    }
    else
    {
        return PhCreateString(L"Unknown time");
    }
}

VOID PhFlushProcessQueryData(
    VOID
    )
{
    PSLIST_ENTRY entry;
    PPH_PROCESS_QUERY_DATA data;

    if (!RtlFirstEntrySList(&PhProcessQueryDataListHead))
        return;

    entry = RtlInterlockedFlushSList(&PhProcessQueryDataListHead);

    while (entry)
    {
        data = CONTAINING_RECORD(entry, PH_PROCESS_QUERY_DATA, ListEntry);
        entry = entry->Next;

        if (data->Stage == 1)
        {
            PhpFillProcessItemStage1((PPH_PROCESS_QUERY_S1_DATA)data);
            PhSetEvent(&data->ProcessItem->Stage1Event);
        }
        else if (data->Stage == 2)
        {
            PhpFillProcessItemStage2((PPH_PROCESS_QUERY_S2_DATA)data);
        }

        data->ProcessItem->JustProcessed = TRUE;

        PhDereferenceObject(data->ProcessItem);
        PhFree(data);
    }
}

VOID PhpGetProcessThreadInformation(
    _In_ PSYSTEM_PROCESS_INFORMATION Process,
    _Out_opt_ PBOOLEAN IsSuspended,
    _Out_opt_ PBOOLEAN IsPartiallySuspended,
    _Out_opt_ PULONG ContextSwitches
    )
{
    ULONG i;
    BOOLEAN isSuspended;
    BOOLEAN isPartiallySuspended;
    ULONG contextSwitches;

    isSuspended = PH_IS_REAL_PROCESS_ID(Process->UniqueProcessId);
    isPartiallySuspended = FALSE;
    contextSwitches = 0;

    for (i = 0; i < Process->NumberOfThreads; i++)
    {
        if (Process->Threads[i].ThreadState != Waiting ||
            Process->Threads[i].WaitReason != Suspended)
        {
            isSuspended = FALSE;
        }
        else
        {
            isPartiallySuspended = TRUE;
        }

        contextSwitches += Process->Threads[i].ContextSwitches;
    }

    // HACK: Minimal/Reflected processes don't have threads (TODO: Use PhGetProcessIsSuspended instead).
    if (Process->NumberOfThreads == 0)
    {
        isSuspended = FALSE;
        isPartiallySuspended = FALSE;
    }

    if (IsSuspended)
        *IsSuspended = isSuspended;
    if (IsPartiallySuspended)
        *IsPartiallySuspended = isPartiallySuspended;
    if (ContextSwitches)
        *ContextSwitches = contextSwitches;
}

VOID PhProcessProviderUpdate(
    _In_ PVOID Object
    )
{
    static ULONG runCount = 0;
    static PSYSTEM_PROCESS_INFORMATION pidBuckets[PROCESS_ID_BUCKETS];

    // Note about locking:
    //
    // Since this is the only function that is allowed to modify the process hashtable, locking is
    // not needed for shared accesses. However, exclusive accesses need locking.

    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;
    ULONG bucketIndex;

    ULONG64 sysTotalTime; // total time for this update period
    ULONG64 sysTotalCycleTime = 0; // total cycle time for this update period
    ULONG64 sysIdleCycleTime = 0; // total idle cycle time for this update period
    FLOAT maxCpuValue = 0;
    PPH_PROCESS_ITEM maxCpuProcessItem = NULL;
    ULONG64 maxIoValue = 0;
    PPH_PROCESS_ITEM maxIoProcessItem = NULL;

    // Pre-update tasks

    if (runCount % 512 == 0) // yes, a very long time
    {
        if (PhEnablePurgeProcessRecords)
            PhPurgeProcessRecords();
    }

    if (!PhProcessStatisticsInitialized)
    {
        PhpInitializeProcessStatistics();
        PhProcessStatisticsInitialized = TRUE;
    }

    PhpUpdatePerfInformation();

    if (PhEnableCycleCpuUsage)
    {
        PhpUpdateCpuInformation(FALSE, &sysTotalTime);
        PhpUpdateCpuCycleInformation(&sysIdleCycleTime);
    }
    else
    {
        PhpUpdateCpuInformation(TRUE, &sysTotalTime);
    }

    if (runCount != 0)
    {
        PhTimeSequenceNumber++;
    }

    // Get the process list.

    PhTotalProcesses = 0;
    PhTotalThreads = 0;
    PhTotalHandles = 0;

    if (!NT_SUCCESS(PhEnumProcesses(&processes)))
        return;

    // Notes on cycle-based CPU usage:
    //
    // Cycle-based CPU usage is a bit tricky to calculate because we cannot get the total number of
    // cycles consumed by all processes since system startup - we can only get total number of
    // cycles per process. This means there are two ways to calculate the system-wide cycle time
    // delta:
    //
    // 1. Each update, sum the cycle times of all processes, and calculate the system-wide delta
    //    from this. Process Explorer seems to do this.
    // 2. Each update, calculate the cycle time delta for each individual process, and sum these
    //    deltas to create the system-wide delta. We use this here.
    //
    // The first method is simpler but has a problem when a process exits and its cycle time is no
    // longer counted in the system-wide total. This may cause the delta to be negative and all
    // other calculations to become invalid. Process Explorer simply ignored this fact and treated
    // the system-wide delta as unsigned (and therefore huge when negative), leading to all CPU
    // usages being displayed as "< 0.01".
    //
    // The second method is used here, but the adjustments must be done before the main new/modified
    // pass. We need take into account new, existing and terminated processes.

    // Create the PID hash set. This contains the process information structures returned by
    // PhEnumProcesses, distinct from the process item hash set. Note that we use the
    // UniqueProcessKey field as the next node pointer to avoid having to allocate extra memory.

    memset(pidBuckets, 0, sizeof(pidBuckets));

    process = PH_FIRST_PROCESS(processes);

    do
    {
        PhTotalProcesses++;
        PhTotalThreads += process->NumberOfThreads;
        PhTotalHandles += process->HandleCount;

        if (process->UniqueProcessId == SYSTEM_IDLE_PROCESS_ID)
        {
            process->CycleTime = PhCpuIdleCycleDelta.Value;
            process->KernelTime = PhCpuTotals.IdleTime;
        }

        bucketIndex = PROCESS_ID_TO_BUCKET_INDEX(process->UniqueProcessId);
        process->UniqueProcessKey = (ULONG_PTR)pidBuckets[bucketIndex];
        pidBuckets[bucketIndex] = process;

        if (PhEnableCycleCpuUsage)
        {
            PPH_PROCESS_ITEM processItem;

            if (WindowsVersion >= WINDOWS_10_RS3 && !PhIsExecutingInWow64())
            {
                if ((processItem = PhpLookupProcessItem(process->UniqueProcessId)) && processItem->ProcessSequenceNumber == PH_PROCESS_EXTENSION(process)->ProcessSequenceNumber)
                    sysTotalCycleTime += process->CycleTime - processItem->CycleTimeDelta.Value; // existing process
                else
                    sysTotalCycleTime += process->CycleTime; // new process
            }
            else
            {
                if ((processItem = PhpLookupProcessItem(process->UniqueProcessId)) && processItem->CreateTime.QuadPart == process->CreateTime.QuadPart)
                    sysTotalCycleTime += process->CycleTime - processItem->CycleTimeDelta.Value; // existing process
                else
                    sysTotalCycleTime += process->CycleTime; // new process
            }
        }
    } while (process = PH_NEXT_PROCESS(process));

    // Add the fake processes to the PID list.
    //
    // On Windows 7 the two fake processes are merged into "Interrupts" since we can only get cycle
    // time information both DPCs and Interrupts combined.

    if (PhEnableCycleCpuUsage)
    {
        PhInterruptsProcessInformation->KernelTime.QuadPart = PhCpuTotals.DpcTime.QuadPart + PhCpuTotals.InterruptTime.QuadPart;
        PhInterruptsProcessInformation->CycleTime = PhCpuSystemCycleDelta.Value;
        sysTotalCycleTime += PhCpuSystemCycleDelta.Delta;
    }
    else
    {
        PhDpcsProcessInformation->KernelTime = PhCpuTotals.DpcTime;
        PhInterruptsProcessInformation->KernelTime = PhCpuTotals.InterruptTime;
    }

    // Look for dead processes.
    {
        PPH_LIST processesToRemove = NULL;
        ULONG i;
        PPH_HASH_ENTRY entry;
        PPH_PROCESS_ITEM processItem;
        PSYSTEM_PROCESS_INFORMATION processEntry;

        for (i = 0; i < PH_HASH_SET_SIZE(PhProcessHashSet); i++)
        {
            for (entry = PhProcessHashSet[i]; entry; entry = entry->Next)
            {
                BOOLEAN processRemoved = FALSE;

                processItem = CONTAINING_RECORD(entry, PH_PROCESS_ITEM, HashEntry);

                // Check if the process still exists. Note that we take into account PID re-use by
                // checking CreateTime as well.

                if (processItem->ProcessId == DPCS_PROCESS_ID)
                {
                    processEntry = PhDpcsProcessInformation;
                }
                else if (processItem->ProcessId == INTERRUPTS_PROCESS_ID)
                {
                    processEntry = PhInterruptsProcessInformation;
                }
                else
                {
                    processEntry = pidBuckets[PROCESS_ID_TO_BUCKET_INDEX(processItem->ProcessId)];

                    while (processEntry && processEntry->UniqueProcessId != processItem->ProcessId)
                        processEntry = (PSYSTEM_PROCESS_INFORMATION)processEntry->UniqueProcessKey;
                }

                if (WindowsVersion >= WINDOWS_10_RS3 && !PhIsExecutingInWow64())
                {
                    if (!processEntry || PH_PROCESS_EXTENSION(processEntry)->ProcessSequenceNumber != processItem->ProcessSequenceNumber)
                        processRemoved = TRUE;
                }
                else
                {
                    if (!processEntry || processEntry->CreateTime.QuadPart != processItem->CreateTime.QuadPart)
                        processRemoved = TRUE;
                }

                if (processRemoved)
                {
                    LARGE_INTEGER exitTime;

                    processItem->State |= PH_PROCESS_ITEM_REMOVED;
                    exitTime.QuadPart = 0;

                    if (processItem->QueryHandle)
                    {
                        KERNEL_USER_TIMES times;
                        ULONG64 finalCycleTime;

                        if (NT_SUCCESS(PhGetProcessTimes(processItem->QueryHandle, &times)))
                        {
                            exitTime = times.ExitTime;
                        }

                        if (PhEnableCycleCpuUsage)
                        {
                            if (NT_SUCCESS(PhGetProcessCycleTime(processItem->QueryHandle, &finalCycleTime)))
                            {
                                // Adjust deltas for the terminated process because this doesn't get
                                // picked up anywhere else.
                                //
                                // Note that if we don't have sufficient access to the process, the
                                // worst that will happen is that the CPU usages of other processes
                                // will get inflated. (See above; if we were using the first
                                // technique, we could get negative deltas, which is much worse.)
                                sysTotalCycleTime += finalCycleTime - processItem->CycleTimeDelta.Value;
                            }
                        }
                    }

                    // If we don't have a valid exit time, use the current time.
                    if (exitTime.QuadPart == 0)
                        PhQuerySystemTime(&exitTime);

                    processItem->Record->Flags |= PH_PROCESS_RECORD_DEAD;
                    processItem->Record->ExitTime = exitTime;

                    // Raise the process removed event.
                    PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderRemovedEvent), processItem);

                    if (!processesToRemove)
                        processesToRemove = PhCreateList(2);

                    PhAddItemList(processesToRemove, processItem);
                }
            }
        }

        // Lock only if we have something to do.
        if (processesToRemove)
        {
            PhAcquireQueuedLockExclusive(&PhProcessHashSetLock);

            for (i = 0; i < processesToRemove->Count; i++)
            {
                PhpRemoveProcessItem((PPH_PROCESS_ITEM)processesToRemove->Items[i]);
            }

            PhReleaseQueuedLockExclusive(&PhProcessHashSetLock);
            PhDereferenceObject(processesToRemove);
        }
    }

    // Go through the queued process query data.
    PhFlushProcessQueryData();

    if (sysTotalTime == 0)
        sysTotalTime = -1; // max. value
    if (sysTotalCycleTime == 0)
        sysTotalCycleTime = -1;

    PhCpuTotalCycleDelta = sysTotalCycleTime;

    // Look for new processes and update existing ones.
    process = PH_FIRST_PROCESS(processes);

    while (process)
    {
        PPH_PROCESS_ITEM processItem;

        processItem = PhpLookupProcessItem(process->UniqueProcessId);

        if (!processItem)
        {
            PPH_PROCESS_RECORD processRecord;
            BOOLEAN isSuspended;
            BOOLEAN isPartiallySuspended;
            ULONG contextSwitches;

            // Create the process item and fill in basic information.
            processItem = PhCreateProcessItem(process->UniqueProcessId);
            PhpFillProcessItem(processItem, process);
            PhpFillProcessItemExtension(processItem, process);
            processItem->TimeSequenceNumber = PhTimeSequenceNumber;

            processRecord = PhpCreateProcessRecord(processItem);
            PhpAddProcessRecord(processRecord);
            processItem->Record = processRecord;

            PhpGetProcessThreadInformation(process, &isSuspended, &isPartiallySuspended, &contextSwitches);
            PhpUpdateDynamicInfoProcessItem(processItem, process);

            // Initialize the deltas.
            PhUpdateDelta(&processItem->CpuKernelDelta, process->KernelTime.QuadPart);
            PhUpdateDelta(&processItem->CpuUserDelta, process->UserTime.QuadPart);
            PhUpdateDelta(&processItem->IoReadDelta, process->ReadTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoWriteDelta, process->WriteTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoOtherDelta, process->OtherTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoReadCountDelta, process->ReadOperationCount.QuadPart);
            PhUpdateDelta(&processItem->IoWriteCountDelta, process->WriteOperationCount.QuadPart);
            PhUpdateDelta(&processItem->IoOtherCountDelta, process->OtherOperationCount.QuadPart);
            PhUpdateDelta(&processItem->ContextSwitchesDelta, contextSwitches);
            PhUpdateDelta(&processItem->PageFaultsDelta, process->PageFaultCount);
            PhUpdateDelta(&processItem->CycleTimeDelta, process->CycleTime);
            PhUpdateDelta(&processItem->PrivateBytesDelta, process->PagefileUsage);

            processItem->IsSuspended = isSuspended;
            processItem->IsPartiallySuspended = isPartiallySuspended;

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
            PhAcquireQueuedLockExclusive(&PhProcessHashSetLock);
            PhpAddProcessItem(processItem);
            PhReleaseQueuedLockExclusive(&PhProcessHashSetLock);

            // Raise the process added event.
            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderAddedEvent), processItem);

            // (Ref: for the process item being in the hashtable.)
            // Instead of referencing then dereferencing we simply don't do anything.
            // Dereferenced in PhpRemoveProcessItem.
        }
        else
        {
            BOOLEAN modified = FALSE;
            BOOLEAN isSuspended;
            BOOLEAN isPartiallySuspended;
            ULONG contextSwitches;
            FLOAT newCpuUsage;
            FLOAT kernelCpuUsage;
            FLOAT userCpuUsage;

            PhpGetProcessThreadInformation(process, &isSuspended, &isPartiallySuspended, &contextSwitches);
            PhpUpdateDynamicInfoProcessItem(processItem, process);
            PhpFillProcessItemExtension(processItem, process);

            // Update the deltas.
            PhUpdateDelta(&processItem->CpuKernelDelta, process->KernelTime.QuadPart);
            PhUpdateDelta(&processItem->CpuUserDelta, process->UserTime.QuadPart);
            PhUpdateDelta(&processItem->IoReadDelta, process->ReadTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoWriteDelta, process->WriteTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoOtherDelta, process->OtherTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoReadCountDelta, process->ReadOperationCount.QuadPart);
            PhUpdateDelta(&processItem->IoWriteCountDelta, process->WriteOperationCount.QuadPart);
            PhUpdateDelta(&processItem->IoOtherCountDelta, process->OtherOperationCount.QuadPart);
            PhUpdateDelta(&processItem->ContextSwitchesDelta, contextSwitches);
            PhUpdateDelta(&processItem->PageFaultsDelta, process->PageFaultCount);
            PhUpdateDelta(&processItem->CycleTimeDelta, process->CycleTime);
            PhUpdateDelta(&processItem->PrivateBytesDelta, process->PagefileUsage);

            processItem->TimeSequenceNumber++;
            PhAddItemCircularBuffer_ULONG64(&processItem->IoReadHistory, processItem->IoReadDelta.Delta);
            PhAddItemCircularBuffer_ULONG64(&processItem->IoWriteHistory, processItem->IoWriteDelta.Delta);
            PhAddItemCircularBuffer_ULONG64(&processItem->IoOtherHistory, processItem->IoOtherDelta.Delta);

            PhAddItemCircularBuffer_SIZE_T(&processItem->PrivateBytesHistory, processItem->VmCounters.PagefileUsage);
            //PhAddItemCircularBuffer_SIZE_T(&processItem->WorkingSetHistory, processItem->VmCounters.WorkingSetSize);

            if (InterlockedExchange(&processItem->JustProcessed, 0) != 0)
                modified = TRUE;

            if (PhEnableCycleCpuUsage)
            {
                FLOAT totalDelta;

                newCpuUsage = (FLOAT)processItem->CycleTimeDelta.Delta / sysTotalCycleTime;

                // Calculate the kernel/user CPU usage based on the kernel/user time. If the kernel
                // and user deltas are both zero, we'll just have to use an estimate. Currently, we
                // split the CPU usage evenly across the kernel and user components, except when the
                // total user time is zero, in which case we assign it all to the kernel component.

                totalDelta = (FLOAT)(processItem->CpuKernelDelta.Delta + processItem->CpuUserDelta.Delta);

                if (totalDelta != 0)
                {
                    kernelCpuUsage = newCpuUsage * ((FLOAT)processItem->CpuKernelDelta.Delta / totalDelta);
                    userCpuUsage = newCpuUsage * ((FLOAT)processItem->CpuUserDelta.Delta / totalDelta);
                }
                else
                {
                    if (processItem->UserTime.QuadPart != 0)
                    {
                        kernelCpuUsage = newCpuUsage / 2;
                        userCpuUsage = newCpuUsage / 2;
                    }
                    else
                    {
                        kernelCpuUsage = newCpuUsage;
                        userCpuUsage = 0;
                    }
                }
            }
            else
            {
                kernelCpuUsage = (FLOAT)processItem->CpuKernelDelta.Delta / sysTotalTime;
                userCpuUsage = (FLOAT)processItem->CpuUserDelta.Delta / sysTotalTime;
                newCpuUsage = kernelCpuUsage + userCpuUsage;
            }

            processItem->CpuUsage = newCpuUsage;
            processItem->CpuKernelUsage = kernelCpuUsage;
            processItem->CpuUserUsage = userCpuUsage;

            PhAddItemCircularBuffer_FLOAT(&processItem->CpuKernelHistory, kernelCpuUsage);
            PhAddItemCircularBuffer_FLOAT(&processItem->CpuUserHistory, userCpuUsage);

            // Max. values

            if (processItem->ProcessId)
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

            // Token information
            if (
                processItem->QueryHandle &&
                processItem->ProcessId != SYSTEM_PROCESS_ID // System token can't be opened (dmex)
                )
            {
                HANDLE tokenHandle;

                if (NT_SUCCESS(PhOpenProcessToken(
                    processItem->QueryHandle,
                    TOKEN_QUERY,
                    &tokenHandle
                    )))
                {
                    PTOKEN_USER tokenUser;
                    TOKEN_ELEVATION_TYPE elevationType;
                    MANDATORY_LEVEL integrityLevel;
                    PWSTR integrityString;

                    // User
                    if (NT_SUCCESS(PhGetTokenUser(tokenHandle, &tokenUser)))
                    {
                        PSID processSid = processItem->Sid;

                        processItem->Sid = PhAllocateCopy(tokenUser->User.Sid, RtlLengthSid(tokenUser->User.Sid));

                        PhFree(processSid);
                        PhFree(tokenUser);
                        modified = TRUE;
                    }

                    // Elevation
                    if (NT_SUCCESS(PhGetTokenElevationType(tokenHandle, &elevationType)))
                    {
                        if (processItem->ElevationType != elevationType)
                        {
                            processItem->ElevationType = elevationType;
                            processItem->IsElevated = elevationType == TokenElevationTypeFull;
                            modified = TRUE;
                        }
                    }

                    // Integrity
                    if (NT_SUCCESS(PhGetTokenIntegrityLevel(tokenHandle, &integrityLevel, &integrityString)))
                    {
                        if (processItem->IntegrityLevel != integrityLevel)
                        {
                            processItem->IntegrityLevel = integrityLevel;
                            processItem->IntegrityString = integrityString;
                            modified = TRUE;
                        }
                    }

                    NtClose(tokenHandle);
                }
            }

            // Job
            if (processItem->QueryHandle)
            {
                NTSTATUS status;

                if (KphIsConnected())
                {
                    HANDLE jobHandle = NULL;

                    status = KphOpenProcessJob(
                        processItem->QueryHandle,
                        JOB_OBJECT_QUERY,
                        &jobHandle
                        );

                    if (NT_SUCCESS(status) && status != STATUS_PROCESS_NOT_IN_JOB)
                    {
                        JOBOBJECT_BASIC_LIMIT_INFORMATION basicLimits;
                        PPH_STRING jobName = NULL;

                        processItem->IsInJob = TRUE;

                        PhGetHandleInformation(
                            NtCurrentProcess(),
                            jobHandle,
                            ULONG_MAX,
                            NULL,
                            NULL,
                            NULL,
                            &jobName
                            );

                        if (jobName)
                            PhMoveReference(&processItem->JobName, jobName);

                        // Process Explorer only recognizes processes as being in jobs if they don't have
                        // the silent-breakaway-OK limit as their only limit. Emulate this behaviour. (wj32)
                        if (NT_SUCCESS(PhGetJobBasicLimits(jobHandle, &basicLimits)))
                        {
                            processItem->IsInSignificantJob =
                                basicLimits.LimitFlags != JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
                        }
                    }

                    if (jobHandle)
                        NtClose(jobHandle);
                }
                else
                {
                    // KProcessHacker is not available. We can determine if the process is in a job, but we
                    // can't get a handle to the job. (wj32)

                    status = NtIsProcessInJob(processItem->QueryHandle, NULL);

                    if (NT_SUCCESS(status))
                        processItem->IsInJob = status == STATUS_PROCESS_IN_JOB;
                }
            }

            // Debugged
            if (processItem->QueryHandle)
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
            if (processItem->IsSuspended != isSuspended)
            {
                processItem->IsSuspended = isSuspended;
                modified = TRUE;
            }

            processItem->IsPartiallySuspended = isPartiallySuspended;

            // .NET
            if (processItem->UpdateIsDotNet)
            {
                BOOLEAN isDotNet;
                ULONG flags = 0;

                if (NT_SUCCESS(PhGetProcessIsDotNetEx(processItem->ProcessId, NULL, 0, &isDotNet, &flags)))
                {
                    processItem->IsDotNet = isDotNet;

                    // This check is needed for the DotNetTools plugin. (dmex)
                    if (!isDotNet && (flags & PH_CLR_JIT_PRESENT))
                        processItem->IsDotNet = TRUE;

                    modified = TRUE;
                }

                processItem->UpdateIsDotNet = FALSE;
            }

            // Immersive
            if (processItem->QueryHandle && IsImmersiveProcess_I)
            {
                BOOLEAN isImmersive;

                isImmersive = !!IsImmersiveProcess_I(processItem->QueryHandle);

                if (processItem->IsImmersive != isImmersive)
                {
                    processItem->IsImmersive = isImmersive;
                    modified = TRUE;
                }
            }

            if (modified)
            {
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderModifiedEvent), processItem);
            }

            // No reference added by PhpLookupProcessItem.
        }

        // Trick ourselves into thinking that the fake processes
        // are on the list.
        if (process == PhInterruptsProcessInformation)
        {
            process = NULL;
        }
        else if (process == PhDpcsProcessInformation)
        {
            process = PhInterruptsProcessInformation;
        }
        else
        {
            process = PH_NEXT_PROCESS(process);

            if (process == NULL)
            {
                if (PhEnableCycleCpuUsage)
                    process = PhInterruptsProcessInformation;
                else
                    process = PhDpcsProcessInformation;
            }
        }
    }

    if (PhProcessInformation)
        PhFree(PhProcessInformation);

    PhProcessInformation = processes;

    PhpFlushSidFullNameCache();

    // History cannot be updated on the first run because the deltas are invalid. For example, the
    // I/O "deltas" will be huge because they are currently the raw accumulated values.
    if (runCount != 0)
    {
        if (PhEnableCycleCpuUsage)
            PhpUpdateCpuCycleUsageInformation(sysTotalCycleTime, sysIdleCycleTime);

        PhpUpdateSystemHistory();

        // Note that we need to add a reference to the records of these processes, to make it
        // possible for others to get the name of a max. CPU or I/O process.

        if (maxCpuProcessItem)
        {
            PhAddItemCircularBuffer_ULONG(&PhMaxCpuHistory, HandleToUlong(maxCpuProcessItem->ProcessId));
#ifdef PH_RECORD_MAX_USAGE
            PhAddItemCircularBuffer_FLOAT(&PhMaxCpuUsageHistory, maxCpuProcessItem->CpuUsage);
#endif

            if (!(maxCpuProcessItem->Record->Flags & PH_PROCESS_RECORD_STAT_REF))
            {
                PhReferenceProcessRecord(maxCpuProcessItem->Record);
                maxCpuProcessItem->Record->Flags |= PH_PROCESS_RECORD_STAT_REF;
            }
        }
        else
        {
            PhAddItemCircularBuffer_ULONG(&PhMaxCpuHistory, PtrToUlong(NULL));
#ifdef PH_RECORD_MAX_USAGE
            PhAddItemCircularBuffer_FLOAT(&PhMaxCpuUsageHistory, 0);
#endif
        }

        if (maxIoProcessItem)
        {
            PhAddItemCircularBuffer_ULONG(&PhMaxIoHistory, HandleToUlong(maxIoProcessItem->ProcessId));
#ifdef PH_RECORD_MAX_USAGE
            PhAddItemCircularBuffer_ULONG64(&PhMaxIoReadOtherHistory,
                maxIoProcessItem->IoReadDelta.Delta + maxIoProcessItem->IoOtherDelta.Delta);
            PhAddItemCircularBuffer_ULONG64(&PhMaxIoWriteHistory, maxIoProcessItem->IoWriteDelta.Delta);
#endif

            if (!(maxIoProcessItem->Record->Flags & PH_PROCESS_RECORD_STAT_REF))
            {
                PhReferenceProcessRecord(maxIoProcessItem->Record);
                maxIoProcessItem->Record->Flags |= PH_PROCESS_RECORD_STAT_REF;
            }
        }
        else
        {
            PhAddItemCircularBuffer_ULONG(&PhMaxIoHistory, PtrToUlong(NULL));
#ifdef PH_RECORD_MAX_USAGE
            PhAddItemCircularBuffer_ULONG64(&PhMaxIoReadOtherHistory, 0);
            PhAddItemCircularBuffer_ULONG64(&PhMaxIoWriteHistory, 0);
#endif
        }
    }

    PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent), NULL);
    runCount++;
}

PPH_PROCESS_RECORD PhpCreateProcessRecord(
    _In_ PPH_PROCESS_ITEM ProcessItem
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
    processRecord->ProcessSequenceNumber = ProcessItem->ProcessSequenceNumber;
    processRecord->CreateTime = ProcessItem->CreateTime;

    PhSetReference(&processRecord->ProcessName, ProcessItem->ProcessName);
    PhSetReference(&processRecord->FileName, ProcessItem->FileName);
    PhSetReference(&processRecord->CommandLine, ProcessItem->CommandLine);
    //PhSetReference(&processRecord->UserName, ProcessItem->UserName);

    return processRecord;
}

PPH_PROCESS_RECORD PhpSearchProcessRecordList(
    _In_ PLARGE_INTEGER Time,
    _Out_opt_ PULONG Index,
    _Out_opt_ PULONG InsertIndex
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
    _Inout_ PPH_PROCESS_RECORD ProcessRecord
    )
{
    PPH_PROCESS_RECORD processRecord;
    ULONG insertIndex;

    PhAcquireQueuedLockExclusive(&PhProcessRecordListLock);

    processRecord = PhpSearchProcessRecordList(&ProcessRecord->CreateTime, NULL, &insertIndex);

    if (!processRecord)
    {
        // Insert the process record, keeping the list sorted.
        PhInsertItemList(PhProcessRecordList, insertIndex, ProcessRecord);
    }
    else
    {
        // Link the process record with the existing one that we found.
        InsertTailList(&processRecord->ListEntry, &ProcessRecord->ListEntry);
    }

    PhReleaseQueuedLockExclusive(&PhProcessRecordListLock);
}

VOID PhpRemoveProcessRecord(
    _Inout_ PPH_PROCESS_RECORD ProcessRecord
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
            PhRemoveItemList(PhProcessRecordList, i);
        else
            PhProcessRecordList->Items[i] = CONTAINING_RECORD(headProcessRecord->ListEntry.Flink, PH_PROCESS_RECORD, ListEntry);
    }

    PhReleaseQueuedLockExclusive(&PhProcessRecordListLock);
}

VOID PhReferenceProcessRecord(
    _In_ PPH_PROCESS_RECORD ProcessRecord
    )
{
    _InterlockedIncrement(&ProcessRecord->RefCount);
}

BOOLEAN PhReferenceProcessRecordSafe(
    _In_ PPH_PROCESS_RECORD ProcessRecord
    )
{
    return _InterlockedIncrementNoZero(&ProcessRecord->RefCount);
}

VOID PhReferenceProcessRecordForStatistics(
    _In_ PPH_PROCESS_RECORD ProcessRecord
    )
{
    if (!(ProcessRecord->Flags & PH_PROCESS_RECORD_STAT_REF))
    {
        PhReferenceProcessRecord(ProcessRecord);
        ProcessRecord->Flags |= PH_PROCESS_RECORD_STAT_REF;
    }
}

VOID PhDereferenceProcessRecord(
    _In_ PPH_PROCESS_RECORD ProcessRecord
    )
{
    if (_InterlockedDecrement(&ProcessRecord->RefCount) == 0)
    {
        PhpRemoveProcessRecord(ProcessRecord);

        PhDereferenceObject(ProcessRecord->ProcessName);
        if (ProcessRecord->FileName) PhDereferenceObject(ProcessRecord->FileName);
        if (ProcessRecord->CommandLine) PhDereferenceObject(ProcessRecord->CommandLine);
        /*if (ProcessRecord->UserName) PhDereferenceObject(ProcessRecord->UserName);*/
        PhFree(ProcessRecord);
    }
}

PPH_PROCESS_RECORD PhpFindProcessRecord(
    _In_ PPH_PROCESS_RECORD ProcessRecord,
    _In_ HANDLE ProcessId
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
    _In_opt_ HANDLE ProcessId,
    _In_ PLARGE_INTEGER Time
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
        // This is expected. Now we search backwards to find the newest matching element older than
        // the given time.

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
        }
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
        // The record might have had its last reference just cleared but it hasn't been removed from
        // the list yet.
        if (!PhReferenceProcessRecordSafe(processRecord))
            found = FALSE;
    }

    PhReleaseQueuedLockShared(&PhProcessRecordListLock);

    if (found)
        return processRecord;
    else
        return NULL;
}

/**
 * Deletes unused process records.
 */
VOID PhPurgeProcessRecords(
    VOID
    )
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
                // Check if the process exit time is before the oldest statistics time. If so we can
                // dereference the process record.
                if (processRecord->ExitTime.QuadPart < threshold.QuadPart)
                {
                    // Clear the stat ref bit; this is to make sure we don't try to dereference the
                    // record twice (e.g. if someone else currently holds a reference to the record
                    // and it doesn't get removed immediately).
                    processRecord->Flags &= ~PH_PROCESS_RECORD_STAT_REF;

                    if (!derefList)
                        derefList = PhCreateList(2);

                    PhAddItemList(derefList, processRecord);
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

PPH_PROCESS_ITEM PhReferenceProcessItemForParent(
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PPH_PROCESS_ITEM parentProcessItem;

    if (ProcessItem->ParentProcessId == ProcessItem->ProcessId) // for cases where the parent PID = PID (e.g. System Idle Process)
        return NULL;

    PhAcquireQueuedLockShared(&PhProcessHashSetLock);

    parentProcessItem = PhpLookupProcessItem(ProcessItem->ParentProcessId);

    if (WindowsVersion >= WINDOWS_10_RS3 && !PhIsExecutingInWow64())
    {
        // We make sure that the process item we found is actually the parent process - its sequence number
        // must not be higher than the supplied sequence.
        if (parentProcessItem && parentProcessItem->ProcessSequenceNumber <= ProcessItem->ProcessSequenceNumber)
            PhReferenceObject(parentProcessItem);
        else
            parentProcessItem = NULL;
    }
    else
    {
        // We make sure that the process item we found is actually the parent process - its start time
        // must not be larger than the supplied time.
        if (parentProcessItem && parentProcessItem->CreateTime.QuadPart <= ProcessItem->CreateTime.QuadPart)
            PhReferenceObject(parentProcessItem);
        else
            parentProcessItem = NULL;
    }

    PhReleaseQueuedLockShared(&PhProcessHashSetLock);

    return parentProcessItem;
}

PPH_PROCESS_ITEM PhReferenceProcessItemForRecord(
    _In_ PPH_PROCESS_RECORD Record
    )
{
    PPH_PROCESS_ITEM processItem;

    PhAcquireQueuedLockShared(&PhProcessHashSetLock);

    processItem = PhpLookupProcessItem(Record->ProcessId);

    if (WindowsVersion >= WINDOWS_10_RS3 && !PhIsExecutingInWow64())
    {
        if (processItem && processItem->ProcessSequenceNumber == Record->ProcessSequenceNumber)
            PhReferenceObject(processItem);
        else
            processItem = NULL;
    }
    else
    {
        if (processItem && processItem->CreateTime.QuadPart == Record->CreateTime.QuadPart)
            PhReferenceObject(processItem);
        else
            processItem = NULL;
    }

    PhReleaseQueuedLockShared(&PhProcessHashSetLock);

    return processItem;
}

