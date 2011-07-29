/*
 * Process Hacker - 
 *   process provider
 * 
 * Copyright (C) 2009-2011 wj32
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
 * This provider module handles the collection of process information and 
 * system-wide statistics. A list of all running processes is kept and 
 * periodically scanned to detect new and terminated processes.
 *
 * The retrieval of certain information is delayed in order to improve 
 * performance. This includes things such as file icons, version information, 
 * digital signature verification, and packed executable detection. These 
 * requests are handed to worker threads which then post back information 
 * to a S-list.
 *
 * Also contained in this module is the storage of process records, which 
 * contain static information about processes. Unlike process items which are 
 * removed as soon as their corresponding process exits, process records 
 * remain as long as they are needed by the statistics system, and more 
 * specifically the max. CPU and I/O history buffers. In PH 1.x, a new 
 * formatted string was created at each update containing information about 
 * the maximum-usage process within that interval. Here we use a much more 
 * storage-efficient method, where the raw maximum-usage PIDs are stored for 
 * each interval, and the process record list is searched when the name of a 
 * process is needed.
 *
 * The process record list is stored as a list of records sorted by process 
 * creation time. If two or more processes have the same creation time, they 
 * are added to a doubly-linked list. This structure allows for fast searching 
 * in the typical scenario where we know the PID of a process and a specific 
 * time in which the process was running. In this case a binary search is used 
 * and then the list is traversed backwards until the process is found. Binary 
 * search is similarly used for insertion and removal.
 *
 * On Windows 7 and above, CPU usage can be calculated from cycle time. However, 
 * cycle time cannot be split into kernel/user components, and cycle time is not 
 * available for DPCs and Interrupts separately (only a "system" cycle time). 
 * Currently, cycle-based CPU usage is only used for the CpuUsage field of the 
 * process item, not for overall system CPU usage.
 */

#define PH_PROCPRV_PRIVATE
#include <phapp.h>
#include <kphuser.h>
#include <extmgri.h>
#include <phplug.h>
#include <winsta.h>

#define PROCESS_ID_BUCKETS 64
#define PROCESS_ID_TO_BUCKET_INDEX(ProcessId) (((ULONG)(ProcessId) / 4) & (PROCESS_ID_BUCKETS - 1))

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
    PWSTR IntegrityString;

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

    VERIFY_RESULT VerifyResult;
    PPH_STRING VerifySignerName;

    BOOLEAN IsPacked;
    ULONG ImportFunctions;
    ULONG ImportModules;
} PH_PROCESS_QUERY_S2_DATA, *PPH_PROCESS_QUERY_S2_DATA;

typedef struct _PH_VERIFY_CACHE_ENTRY
{
    PH_AVL_LINKS Links;

    PPH_STRING FileName;
    VERIFY_RESULT VerifyResult;
    PPH_STRING VerifySignerName;
} PH_VERIFY_CACHE_ENTRY, *PPH_VERIFY_CACHE_ENTRY;

VOID NTAPI PhpProcessItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

INT NTAPI PhpVerifyCacheCompareFunction(
    __in PPH_AVL_LINKS Links1,
    __in PPH_AVL_LINKS Links2
    );

VOID PhpQueueProcessQueryStage1(
    __in PPH_PROCESS_ITEM ProcessItem
    );

VOID PhpQueueProcessQueryStage2(
    __in PPH_PROCESS_ITEM ProcessItem
    );

VOID PhpUpdatePerfInformation();

VOID PhpUpdateCpuInformation(
    __out PULONG64 TotalTime
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

PPH_OBJECT_TYPE PhProcessItemType;

PPH_HASH_ENTRY PhProcessHashSet[256] = PH_HASH_SET_INIT;
ULONG PhProcessHashSetCount = 0;
PH_QUEUED_LOCK PhProcessHashSetLock = PH_QUEUED_LOCK_INIT;

SLIST_HEADER PhProcessQueryDataListHead;

PHAPPAPI PH_CALLBACK_DECLARE(PhProcessAddedEvent);
PHAPPAPI PH_CALLBACK_DECLARE(PhProcessModifiedEvent);
PHAPPAPI PH_CALLBACK_DECLARE(PhProcessRemovedEvent);
PHAPPAPI PH_CALLBACK_DECLARE(PhProcessesUpdatedEvent);

PPH_LIST PhProcessRecordList;
PH_QUEUED_LOCK PhProcessRecordListLock = PH_QUEUED_LOCK_INIT;

ULONG PhStatisticsSampleCount = 512;
BOOLEAN PhEnableProcessQueryStage2 = FALSE;
BOOLEAN PhEnablePurgeProcessRecords = TRUE;
BOOLEAN PhEnableCycleCpuUsage = TRUE;

SYSTEM_PERFORMANCE_INFORMATION PhPerfInformation;
PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PhCpuInformation;
SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PhCpuTotals;
ULONG PhTotalProcesses;
ULONG PhTotalThreads;
ULONG PhTotalHandles;

SYSTEM_PROCESS_INFORMATION PhDpcsProcessInformation;
SYSTEM_PROCESS_INFORMATION PhInterruptsProcessInformation;
PLARGE_INTEGER PhCpuSystemCycleTime; // cycle time for DPCs and Interrupts
LARGE_INTEGER PhCpuTotalSystemCycleTime;
PH_UINT64_DELTA PhCpuSystemCycleDelta;

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

static PPH_IS_DOT_NET_CONTEXT PhpIsDotNetContext = NULL;

static PTS_ALL_PROCESSES_INFO PhpTsProcesses = NULL;
static ULONG PhpTsNumberOfProcesses;

#ifdef PH_ENABLE_VERIFY_CACHE
static PH_AVL_TREE PhpVerifyCacheSet = PH_AVL_TREE_INIT(PhpVerifyCacheCompareFunction);
static PH_QUEUED_LOCK PhpVerifyCacheLock = PH_QUEUED_LOCK_INIT;
#endif

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

    RtlInitializeSListHead(&PhProcessQueryDataListHead);

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

    if (WindowsVersion >= WINDOWS_7)
    {
        PhCpuSystemCycleTime = PhAllocate(
            sizeof(LARGE_INTEGER) *
            (ULONG)PhSystemBasicInformation.NumberOfProcessors
            );
    }

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

    PhInitializeDelta(&PhCpuSystemCycleDelta);
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
    PPH_PROCESS_ITEM processItem;

    processItem = PhReferenceProcessItem(ClientId->UniqueProcess);

    if (processItem)
    {
        name = PhGetClientIdNameEx(ClientId, processItem->ProcessName);
        PhDereferenceObject(processItem);
    }
    else
    {
        name = PhGetClientIdNameEx(ClientId, NULL);
    }

    return name;
}

PPH_STRING PhGetClientIdNameEx(
    __in PCLIENT_ID ClientId,
    __in_opt PPH_STRING ProcessName
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
    __in ULONG PriorityClass
    )
{
    switch (PriorityClass)
    {
    case PROCESS_PRIORITY_CLASS_REALTIME:
        return L"Real Time";
    case PROCESS_PRIORITY_CLASS_HIGH:
        return L"High";
    case PROCESS_PRIORITY_CLASS_ABOVE_NORMAL:
        return L"Above Normal";
    case PROCESS_PRIORITY_CLASS_NORMAL:
        return L"Normal";
    case PROCESS_PRIORITY_CLASS_BELOW_NORMAL:
        return L"Below Normal";
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
    __in HANDLE ProcessId
    )
{
    PPH_PROCESS_ITEM processItem;

    if (!NT_SUCCESS(PhCreateObject(
        &processItem,
        PhEmGetObjectSize(EmProcessItemType, sizeof(PH_PROCESS_ITEM)),
        0,
        PhProcessItemType
        )))
        return NULL;

    memset(processItem, 0, sizeof(PH_PROCESS_ITEM));
    PhInitializeEvent(&processItem->Stage1Event);
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

    PhEmCallObjectOperation(EmProcessItemType, processItem, EmObjectCreate);

    return processItem;
}

VOID PhpProcessItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
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
    if (processItem->UserName) PhDereferenceObject(processItem->UserName);
    if (processItem->JobName) PhDereferenceObject(processItem->JobName);
    if (processItem->VerifySignerName) PhDereferenceObject(processItem->VerifySignerName);

    if (processItem->QueryHandle) NtClose(processItem->QueryHandle);

    if (processItem->Record) PhDereferenceProcessRecord(processItem->Record);
}

FORCEINLINE BOOLEAN PhCompareProcessItem(
    __in PPH_PROCESS_ITEM Value1,
    __in PPH_PROCESS_ITEM Value2
    )
{
    return Value1->ProcessId == Value2->ProcessId;
}

FORCEINLINE ULONG PhHashProcessItem(
    __in PPH_PROCESS_ITEM Value
    )
{
    return (ULONG)Value->ProcessId / 4;
}

/**
 * Finds a process item in the hash set.
 *
 * \param ProcessId The process ID of the process item.
 *
 * \remarks The hash set must be locked before calling this 
 * function. The reference count of the found process item is 
 * not incremented.
 */
__assumeLocked PPH_PROCESS_ITEM PhpLookupProcessItem(
    __in HANDLE ProcessId
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
    __in HANDLE ProcessId
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
 * \param ProcessItems A variable which receives an array of 
 * pointers to process items. You must free the buffer with 
 * PhFree() when you no longer need it.
 * \param NumberOfProcessItems A variable which receives the 
 * number of process items returned in \a ProcessItems.
 */
VOID PhEnumProcessItems(
    __out_opt PPH_PROCESS_ITEM **ProcessItems,
    __out PULONG NumberOfProcessItems
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

__assumeLocked VOID PhpAddProcessItem(
    __in __assumeRefs(1) PPH_PROCESS_ITEM ProcessItem
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

__assumeLocked VOID PhpRemoveProcessItem(
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    PhRemoveEntryHashSet(PhProcessHashSet, PH_HASH_SET_SIZE(PhProcessHashSet), &ProcessItem->HashEntry);
    PhProcessHashSetCount--;
    PhDereferenceObject(ProcessItem);
}

INT NTAPI PhpVerifyCacheCompareFunction(
    __in PPH_AVL_LINKS Links1,
    __in PPH_AVL_LINKS Links2
    )
{
    PPH_VERIFY_CACHE_ENTRY entry1 = CONTAINING_RECORD(Links1, PH_VERIFY_CACHE_ENTRY, Links);
    PPH_VERIFY_CACHE_ENTRY entry2 = CONTAINING_RECORD(Links2, PH_VERIFY_CACHE_ENTRY, Links);

    return PhCompareString(entry1->FileName, entry2->FileName, TRUE);
}

/**
 * Verifies a file's digital signature, using a cached 
 * result if possible.
 *
 * \param FileName A file name.
 * \param SignerName A variable which receives a pointer 
 * to a string containing the signer name. You must free 
 * the string using PhDereferenceObject() when you no 
 * longer need it. Note that the signer name may be NULL 
 * if it is not valid.
 * \param CachedOnly Specify TRUE to fail the function when 
 * no cached result exists.
 *
 * \return A VERIFY_RESULT value.
 */
VERIFY_RESULT PhVerifyFileCached(
    __in PPH_STRING FileName,
    __out_opt PPH_STRING *SignerName,
    __in BOOLEAN CachedOnly
    )
{
#ifdef PH_ENABLE_VERIFY_CACHE
    PPH_AVL_LINKS links;
    PPH_VERIFY_CACHE_ENTRY entry;
    PH_VERIFY_CACHE_ENTRY lookupEntry;

    lookupEntry.FileName = FileName;

    PhAcquireQueuedLockShared(&PhpVerifyCacheLock);
    links = PhFindElementAvlTree(&PhpVerifyCacheSet, &lookupEntry.Links);
    PhReleaseQueuedLockShared(&PhpVerifyCacheLock);

    if (links)
    {
        entry = CONTAINING_RECORD(links, PH_VERIFY_CACHE_ENTRY, Links);

        if (SignerName)
        {
            if (entry->VerifySignerName)
                PhReferenceObject(entry->VerifySignerName);

            *SignerName = entry->VerifySignerName;
        }

        return entry->VerifyResult;
    }
    else
    {
        VERIFY_RESULT result;
        PPH_STRING signerName;

        if (!CachedOnly)
        {
            result = PhVerifyFile(FileName->Buffer, &signerName);
        }
        else
        {
            result = VrUnknown;
            signerName = NULL;
        }

        if (result != VrUnknown)
        {
            entry = PhAllocate(sizeof(PH_VERIFY_CACHE_ENTRY));
            entry->FileName = FileName;
            entry->VerifyResult = result;
            entry->VerifySignerName = signerName;

            PhAcquireQueuedLockExclusive(&PhpVerifyCacheLock);
            links = PhAddElementAvlTree(&PhpVerifyCacheSet, &entry->Links);
            PhReleaseQueuedLockExclusive(&PhpVerifyCacheLock);

            if (!links)
            {
                // We successfully added the cache entry. Add references.

                PhReferenceObject(entry->FileName);

                if (entry->VerifySignerName)
                    PhReferenceObject(entry->VerifySignerName);
            }
            else
            {
                // Entry already exists.
                PhFree(entry);
            }
        }

        if (SignerName)
        {
            *SignerName = signerName;
        }
        else
        {
            if (signerName)
                PhDereferenceObject(signerName);
        }

        return result;
    }
#else
    return PhVerifyFile(
        FileName->Buffer,
        SignerName
        );
#endif
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
        if (ExtractIconEx(
            processItem->FileName->Buffer,
            0,
            &Data->LargeIcon,
            &Data->SmallIcon,
            1
            ) == 0)
        {
            Data->LargeIcon = NULL;
            Data->SmallIcon = NULL;
        }

        // Version info.
        PhInitializeImageVersionInfo(&Data->VersionInfo, processItem->FileName->Buffer);
    }

    // Use the default EXE icon if we didn't get the file's icon.
    {
        if (!Data->SmallIcon || !Data->LargeIcon)
        {
            if (Data->SmallIcon)
            {
                DestroyIcon(Data->SmallIcon);
                Data->SmallIcon = NULL;
            }
            else if (Data->LargeIcon)
            {
                DestroyIcon(Data->LargeIcon);
                Data->LargeIcon = NULL;
            }

            PhGetStockApplicationIcon(&Data->SmallIcon, &Data->LargeIcon);
            Data->SmallIcon = DuplicateIcon(NULL, Data->SmallIcon);
            Data->LargeIcon = DuplicateIcon(NULL, Data->LargeIcon);
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
                processHandleLimited,
                JOB_OBJECT_QUERY,
                &jobHandle
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
    if (processHandleLimited && WINDOWS_HAS_CONSOLE_HOST)
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

    if (PhEnableProcessQueryStage2 && processItem->FileName)
    {
        Data->VerifyResult = PhVerifyFileCached(
            processItem->FileName,
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
    PhQueueItemGlobalWorkQueue(PhpProcessQueryStage1Worker, ProcessItem);
}

VOID PhpQueueProcessQueryStage2(
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    if (PhEnableProcessQueryStage2)
    {
        PhReferenceObject(ProcessItem);
        PhQueueItemGlobalWorkQueue(PhpProcessQueryStage2Worker, ProcessItem);
    }
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
    processItem->IntegrityString = Data->IntegrityString;
    processItem->JobName = Data->JobName;
    processItem->ConsoleHostProcessId = Data->ConsoleHostProcessId;
    processItem->IsElevated = Data->IsElevated;
    processItem->IsInJob = Data->IsInJob;
    processItem->IsInSignificantJob = Data->IsInSignificantJob;
    processItem->IsPosix = Data->IsPosix;
    processItem->IsWow64 = Data->IsWow64;

    PhSwapReference(&processItem->Record->CommandLine, processItem->CommandLine);
}

VOID PhpFillProcessItemStage2(
    __in PPH_PROCESS_QUERY_S2_DATA Data
    )
{
    PPH_PROCESS_ITEM processItem = Data->Header.ProcessItem;

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
        ProcessItem->ProcessName = PhCreateString(SYSTEM_IDLE_PROCESS_NAME);
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
            else if (processHandle)
                status = PhGetProcessImageFileName(processHandle, &fileName);
            else
                status = STATUS_UNSUCCESSFUL;

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
        ProcessItem->ProcessId != SYSTEM_PROCESS_ID // Token of System process can't be opened sometimes
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
        if (
            ProcessItem->ProcessId == SYSTEM_IDLE_PROCESS_ID ||
            ProcessItem->ProcessId == SYSTEM_PROCESS_ID // System token can't be opened on XP
            )
        {
            PhReferenceObject(PhLocalSystemName);
            ProcessItem->UserName = PhLocalSystemName;
        }
    }

    if (!ProcessItem->UserName && WindowsVersion <= WINDOWS_XP)
    {
        // In some cases we can get the user SID using WTS (only works on XP and below).

        if (!PhpTsProcesses)
        {
            WinStationGetAllProcesses(
                NULL,
                0,
                &PhpTsNumberOfProcesses,
                &PhpTsProcesses
                );
        }

        if (PhpTsProcesses)
        {
            ULONG i;

            for (i = 0; i < PhpTsNumberOfProcesses; i++)
            {
                if (UlongToHandle(PhpTsProcesses[i].pTsProcessInfo->UniqueProcessId) == ProcessItem->ProcessId)
                {
                    ProcessItem->UserName = PhGetSidFullName(PhpTsProcesses[i].pSid, TRUE, NULL);
                    break;
                }
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

    if (ProcessItem->QueryHandle)
    {
        PROCESS_PRIORITY_CLASS priorityClass;

        if (NT_SUCCESS(NtQueryInformationProcess(
            ProcessItem->QueryHandle,
            ProcessPriorityClass,
            &priorityClass,
            sizeof(PROCESS_PRIORITY_CLASS),
            NULL
            )))
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
        sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * (ULONG)PhSystemBasicInformation.NumberOfProcessors,
        NULL
        );

    // Zero the CPU totals.
    memset(&PhCpuTotals, 0, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION));

    for (i = 0; i < (ULONG)PhSystemBasicInformation.NumberOfProcessors; i++)
    {
        PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION cpuInfo =
            &PhCpuInformation[i];

        // KernelTime includes idle time.
        cpuInfo->KernelTime.QuadPart -= cpuInfo->IdleTime.QuadPart;

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

VOID PhpUpdateSystemCpuCycleInformation()
{
    ULONG i;

    NtQuerySystemInformation(
        SystemProcessorCycleTimeInformation,
        PhCpuSystemCycleTime,
        sizeof(LARGE_INTEGER) * (ULONG)PhSystemBasicInformation.NumberOfProcessors,
        NULL
        );

    PhCpuTotalSystemCycleTime.QuadPart = 0;

    for (i = 0; i < (ULONG)PhSystemBasicInformation.NumberOfProcessors; i++)
    {
        PhCpuTotalSystemCycleTime.QuadPart += PhCpuSystemCycleTime[i].QuadPart;
    }

    PhUpdateDelta(&PhCpuSystemCycleDelta, PhCpuTotalSystemCycleTime.QuadPart);
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
    PhTimeSequenceNumber++;
}

/**
 * Retrieves a time value recorded by the statistics system.
 *
 * \param ProcessItem A process item to synchronize with, or NULL if 
 * no synchronization is necessary.
 * \param Index The history index.
 * \param Time A variable which receives the time at \a Index.
 *
 * \return TRUE if the function succeeded, otherwise FALSE if 
 * \a ProcessItem was specified and \a Index is too far into the 
 * past for that process item.
 */
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

    secondsSince1980 = PhGetItemCircularBuffer_ULONG(&PhTimeHistory, index);
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

VOID PhpGetProcessThreadInformation(
    __in PSYSTEM_PROCESS_INFORMATION Process,
    __out_opt PBOOLEAN IsSuspended,
    __out_opt PULONG ContextSwitches
    )
{
    ULONG i;
    BOOLEAN isSuspended;
    ULONG contextSwitches;

    isSuspended = Process->NumberOfThreads != 0;
    contextSwitches = 0;

    for (i = 0; i < Process->NumberOfThreads; i++)
    {
        if (
            Process->Threads[i].ThreadState != Waiting ||
            Process->Threads[i].WaitReason != Suspended
            )
        {
            isSuspended = FALSE;
        }

        contextSwitches += Process->Threads[i].ContextSwitches;
    }

    if (IsSuspended)
        *IsSuspended = isSuspended;
    if (ContextSwitches)
        *ContextSwitches = contextSwitches;
}

VOID PhProcessProviderUpdate(
    __in PVOID Object
    )
{
    static ULONG runCount = 0;
    static PSYSTEM_PROCESS_INFORMATION pidBuckets[PROCESS_ID_BUCKETS];

    // Note about locking:
    // Since this is the only function that is allowed to 
    // modify the process hashtable, locking is not needed 
    // for shared accesses. However, exclusive accesses 
    // need locking.

    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;
    ULONG bucketIndex;

    BOOLEAN isDotNetContextUpdated = FALSE;
    BOOLEAN isCycleCpuUsageEnabled = FALSE;

    ULONG64 sysTotalTime; // total time for this update period
    ULONG64 sysTotalCycleTime = 0; // total cycle time for this update period
    FLOAT maxCpuValue = 0;
    PPH_PROCESS_ITEM maxCpuProcessItem = NULL;
    ULONG64 maxIoValue = 0;
    PPH_PROCESS_ITEM maxIoProcessItem = NULL;

    // Pre-update tasks

    if (runCount % 8 == 0)
    {
        PhRefreshDosDevicePrefixes();
    }

    if (runCount % 16 == 1)
    {
        if (PhpIsDotNetContext)
        {
            PhFreeIsDotNetContext(PhpIsDotNetContext);
            PhpIsDotNetContext = NULL;
        }

        if (PhPluginsEnabled)
        {
            PH_PLUGIN_IS_DOT_NET_DIRECTORY_NAMES directoryNames;

            PhInitializeStringRef(&directoryNames.DirectoryNames[0], L"\\BaseNamedObjects");
            directoryNames.NumberOfDirectoryNames = 1;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackGetIsDotNetDirectoryNames), &directoryNames);

            PhCreateIsDotNetContext(&PhpIsDotNetContext, directoryNames.DirectoryNames, directoryNames.NumberOfDirectoryNames);
            isDotNetContextUpdated = TRUE;
        }
        else
        {
            PhCreateIsDotNetContext(&PhpIsDotNetContext, NULL, 0);
            isDotNetContextUpdated = TRUE;
        }
    }

    if (runCount % 512 == 0) // yes, a very long time
    {
        if (PhEnablePurgeProcessRecords)
            PhPurgeProcessRecords();
    }

    isCycleCpuUsageEnabled = WindowsVersion >= WINDOWS_7 && PhEnableCycleCpuUsage;

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

    // Notes on cycle-based CPU usage:
    //
    // Cycle-based CPU usage is a bit tricky to calculate because we cannot get 
    // the total number of cycles consumed by all processes since system startup - we 
    // can only get total number of cycles per process. This means there are two ways 
    // to calculate the system-wide cycle time delta:
    //
    // 1. Each update, sum the cycle times of all processes, and calculate the system-wide 
    //    delta from this. Process Explorer seems to do this.
    // 2. Each update, calculate the cycle time delta for each individual process, and 
    //    sum these deltas to create the system-wide delta. We use this here.
    //
    // The first method is simpler but has a problem when a process exits and its cycle time 
    // is no longer counted in the system-wide total. This may cause the delta to be 
    // negative and all other calculations to become invalid. Process Explorer simply ignores 
    // this fact and treats the system-wide delta as unsigned (and therefore huge when negative), 
    // leading to all CPU usages being displayed as "< 0.01".
    //
    // The second method is used here, but the adjustments must be done before the main new/modified 
    // pass. We need take into account new, existing and terminated processes.

    // Create the PID hash set. This contains the process information structures returned by 
    // PhEnumProcesses, distinct from the process item hash set.
    // Note that we use the UniqueProcessKey field as the next node pointer to avoid having to 
    // allocate extra memory.

    memset(pidBuckets, 0, sizeof(pidBuckets));

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

        bucketIndex = PROCESS_ID_TO_BUCKET_INDEX(process->UniqueProcessId);
        process->UniqueProcessKey = (ULONG_PTR)pidBuckets[bucketIndex];
        pidBuckets[bucketIndex] = process;

        if (isCycleCpuUsageEnabled)
        {
            PPH_PROCESS_ITEM processItem;

            if ((processItem = PhpLookupProcessItem(process->UniqueProcessId)) && processItem->CreateTime.QuadPart == process->CreateTime.QuadPart)
                sysTotalCycleTime += process->CycleTime - processItem->CycleTimeDelta.Value; // existing process
            else
                sysTotalCycleTime += process->CycleTime; // new process
        }
    } while (process = PH_NEXT_PROCESS(process));

    // Add the fake processes to the PID list.
    // On Windows 7 the two fake processes are merged into "Interrupts" since we can only get 
    // cycle time information both DPCs and Interrupts combined.

    if (isCycleCpuUsageEnabled)
    {
        PhpUpdateSystemCpuCycleInformation();
        PhInterruptsProcessInformation.KernelTime.QuadPart = PhCpuTotals.DpcTime.QuadPart + PhCpuTotals.InterruptTime.QuadPart;
        PhInterruptsProcessInformation.CycleTime = PhCpuTotalSystemCycleTime.QuadPart;
        sysTotalCycleTime += PhCpuSystemCycleDelta.Delta;
    }
    else
    {
        PhDpcsProcessInformation.KernelTime = PhCpuTotals.DpcTime;
        PhInterruptsProcessInformation.KernelTime = PhCpuTotals.InterruptTime;
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
                processItem = CONTAINING_RECORD(entry, PH_PROCESS_ITEM, HashEntry);

                // Check if the process still exists. Note that we take into account PID re-use by 
                // checking CreateTime as well.

                if (processItem->ProcessId == DPCS_PROCESS_ID)
                {
                    processEntry = &PhDpcsProcessInformation;
                }
                else if (processItem->ProcessId == INTERRUPTS_PROCESS_ID)
                {
                    processEntry = &PhInterruptsProcessInformation;
                }
                else
                {
                    processEntry = pidBuckets[PROCESS_ID_TO_BUCKET_INDEX(processItem->ProcessId)];

                    while (processEntry && processEntry->UniqueProcessId != processItem->ProcessId)
                        processEntry = (PSYSTEM_PROCESS_INFORMATION)processEntry->UniqueProcessKey;
                }

                if (!processEntry || processEntry->CreateTime.QuadPart != processItem->CreateTime.QuadPart)
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

                        if (isCycleCpuUsageEnabled)
                        {
                            if (NT_SUCCESS(PhGetProcessCycleTime(processItem->QueryHandle, &finalCycleTime)))
                            {
                                // Adjust deltas for the terminated process because this doesn't get 
                                // picked up anywhere else.
                                //
                                // Note that if we don't have sufficient access to the process, the worst 
                                // that will happen is that the CPU usages of other processes will get 
                                // inflated. (See above; if we were using the first technique, we could 
                                // get negative deltas, which is much worse.)
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
                    PhInvokeCallback(&PhProcessRemovedEvent, processItem);

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
    if (RtlQueryDepthSList(&PhProcessQueryDataListHead) != 0)
    {
        PSLIST_ENTRY entry;
        PPH_PROCESS_QUERY_DATA data;

        entry = RtlInterlockedFlushSList(&PhProcessQueryDataListHead);

        while (entry)
        {
            data = CONTAINING_RECORD(entry, PH_PROCESS_QUERY_DATA, ListEntry);
            entry = entry->Next;

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
            BOOLEAN isSuspended;
            ULONG contextSwitches;

            // Create the process item and fill in basic information.
            processItem = PhCreateProcessItem(process->UniqueProcessId);
            PhpFillProcessItem(processItem, process);
            processItem->SequenceNumber = PhTimeSequenceNumber;

            processRecord = PhpCreateProcessRecord(processItem);
            PhpAddProcessRecord(processRecord);
            processItem->Record = processRecord;

            // Open a handle to the process for later usage.
            PhOpenProcess(&processItem->QueryHandle, PROCESS_QUERY_INFORMATION, processItem->ProcessId);

            PhpGetProcessThreadInformation(process, &isSuspended, &contextSwitches);
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

            // Update VM and I/O statistics.
            processItem->VmCounters = *(PVM_COUNTERS_EX)&process->PeakVirtualSize;
            processItem->IoCounters = *(PIO_COUNTERS)&process->ReadOperationCount;
            processItem->WorkingSetPrivateSize = (SIZE_T)process->WorkingSetPrivateSize.QuadPart;
            processItem->PeakNumberOfThreads = process->NumberOfThreadsHighWatermark;
            processItem->HardFaultCount = process->HardFaultCount;

            processItem->IsSuspended = isSuspended;

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
            PhInvokeCallback(&PhProcessAddedEvent, processItem);

            // (Ref: for the process item being in the hashtable.)
            // Instead of referencing then dereferencing we simply don't 
            // do anything.
            // Dereferenced in PhpRemoveProcessItem.
        }
        else
        {
            BOOLEAN modified = FALSE;
            BOOLEAN isSuspended;
            ULONG contextSwitches;
            FLOAT kernelCpuUsage;
            FLOAT userCpuUsage;
            FLOAT newCpuUsage;

            PhpGetProcessThreadInformation(process, &isSuspended, &contextSwitches);

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

            processItem->SequenceNumber++;
            PhAddItemCircularBuffer_ULONG64(&processItem->IoReadHistory, processItem->IoReadDelta.Delta);
            PhAddItemCircularBuffer_ULONG64(&processItem->IoWriteHistory, processItem->IoWriteDelta.Delta);
            PhAddItemCircularBuffer_ULONG64(&processItem->IoOtherHistory, processItem->IoOtherDelta.Delta);

            PhAddItemCircularBuffer_SIZE_T(&processItem->PrivateBytesHistory, processItem->VmCounters.PagefileUsage);
            //PhAddItemCircularBuffer_SIZE_T(&processItem->WorkingSetHistory, processItem->VmCounters.WorkingSetSize);

            PhpUpdateDynamicInfoProcessItem(processItem, process);

            // Update VM and I/O statistics.
            processItem->VmCounters = *(PVM_COUNTERS_EX)&process->PeakVirtualSize;
            processItem->IoCounters = *(PIO_COUNTERS)&process->ReadOperationCount;
            processItem->WorkingSetPrivateSize = (SIZE_T)process->WorkingSetPrivateSize.QuadPart;
            processItem->PeakNumberOfThreads = process->NumberOfThreadsHighWatermark;
            processItem->HardFaultCount = process->HardFaultCount;

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
            PhAddItemCircularBuffer_FLOAT(&processItem->CpuKernelHistory, kernelCpuUsage);
            PhAddItemCircularBuffer_FLOAT(&processItem->CpuUserHistory, userCpuUsage);

            if (isCycleCpuUsageEnabled)
            {
                newCpuUsage = (FLOAT)processItem->CycleTimeDelta.Delta / sysTotalCycleTime;
                processItem->CpuUsage = newCpuUsage;
            }
            else
            {
                newCpuUsage = kernelCpuUsage + userCpuUsage;
                processItem->CpuUsage = newCpuUsage;
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

            // .NET
            if (isDotNetContextUpdated && PhpIsDotNetContext && !processItem->IsDotNet)
            {
                if (PhGetProcessIsDotNetFromContext(PhpIsDotNetContext, processItem->ProcessId, NULL))
                {
                    processItem->IsDotNet = TRUE;
                    modified = TRUE;
                }
            }

            if (modified)
            {
                PhInvokeCallback(&PhProcessModifiedEvent, processItem);
            }

            // No reference added by PhpLookupProcessItem.
        }

        // Trick ourselves into thinking that the fake processes 
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
            {
                if (isCycleCpuUsageEnabled)
                    process = &PhInterruptsProcessInformation;
                else
                    process = &PhDpcsProcessInformation;
            }
        }
    }

    PhFree(processes);

    if (PhpTsProcesses)
    {
        WinStationFreeGAPMemory(0, PhpTsProcesses, PhpTsNumberOfProcesses);
        PhpTsProcesses = NULL;
    }

    if (runCount != 0)
    {
        // Post-update history

        // Note that we need to add a reference to the records of these processes, to 
        // make it possible for others to get the name of a max. CPU or I/O process.

        if (maxCpuProcessItem)
        {
            PhAddItemCircularBuffer_ULONG(&PhMaxCpuHistory, (ULONG)maxCpuProcessItem->ProcessId);
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
            PhAddItemCircularBuffer_ULONG(&PhMaxCpuHistory, (ULONG)NULL);
#ifdef PH_RECORD_MAX_USAGE
            PhAddItemCircularBuffer_FLOAT(&PhMaxCpuUsageHistory, 0);
#endif
        }

        if (maxIoProcessItem)
        {
            PhAddItemCircularBuffer_ULONG(&PhMaxIoHistory, (ULONG)maxIoProcessItem->ProcessId);
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
            PhAddItemCircularBuffer_ULONG(&PhMaxIoHistory, (ULONG)NULL);
#ifdef PH_RECORD_MAX_USAGE
            PhAddItemCircularBuffer_ULONG64(&PhMaxIoReadOtherHistory, 0);
            PhAddItemCircularBuffer_ULONG64(&PhMaxIoWriteHistory, 0);
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

    if (ProcessItem->CommandLine)
    {
        PhReferenceObject(ProcessItem->CommandLine);
        processRecord->CommandLine = ProcessItem->CommandLine;
    }

    /*if (ProcessItem->UserName)
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
            PhRemoveItemList(PhProcessRecordList, i);
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

VOID PhReferenceProcessRecordForStatistics(
    __in PPH_PROCESS_RECORD ProcessRecord
    )
{
    if (!(ProcessRecord->Flags & PH_PROCESS_RECORD_STAT_REF))
    {
        PhReferenceProcessRecord(ProcessRecord);
        ProcessRecord->Flags |= PH_PROCESS_RECORD_STAT_REF;
    }
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
        if (ProcessRecord->CommandLine) PhDereferenceObject(ProcessRecord->CommandLine);
        /*if (ProcessRecord->UserName) PhDereferenceObject(ProcessRecord->UserName);*/
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

/**
 * Deletes unused process records.
 */
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
    __in HANDLE ParentProcessId,
    __in HANDLE ProcessId,
    __in PLARGE_INTEGER CreateTime
    )
{
    PPH_PROCESS_ITEM processItem;

    if (ParentProcessId == ProcessId) // for cases where the parent PID = PID (e.g. System Idle Process)
        return NULL;

    PhAcquireQueuedLockShared(&PhProcessHashSetLock);

    processItem = PhpLookupProcessItem(ParentProcessId);

    // We make sure that the process item we found is actually the parent 
    // process - its start time must not be larger than the supplied 
    // time.
    if (processItem && processItem->CreateTime.QuadPart <= CreateTime->QuadPart)
        PhReferenceObject(processItem);
    else
        processItem = NULL;

    PhReleaseQueuedLockShared(&PhProcessHashSetLock);

    return processItem;
}

PPH_PROCESS_ITEM PhReferenceProcessItemForRecord(
    __in PPH_PROCESS_RECORD Record
    )
{
    PPH_PROCESS_ITEM processItem;

    PhAcquireQueuedLockShared(&PhProcessHashSetLock);

    processItem = PhpLookupProcessItem(Record->ProcessId);

    if (processItem && processItem->CreateTime.QuadPart == Record->CreateTime.QuadPart)
        PhReferenceObject(processItem);
    else
        processItem = NULL;

    PhReleaseQueuedLockShared(&PhProcessHashSetLock);

    return processItem;
}
