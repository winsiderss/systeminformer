/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2022
 *     jxy-s   2020-2021
 *
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

#include <hndlinfo.h>
#include <kphuser.h>
#include <lsasup.h>
#include <workqueue.h>

#include <extmgri.h>
#include <phplug.h>
#include <srvprv.h>

#include <mapimg.h>

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

    PPH_IMAGELIST_ITEM IconEntry;
    PH_IMAGE_VERSION_INFO VersionInfo;

    HANDLE ConsoleHostProcessId;
    PPH_STRING UserName;

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
            ULONG Architecture : 16; /*!< Process Machine Architecture (IMAGE_FILE_MACHINE_...) */
            ULONG Spare : 9;
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
    PH_IMAGE_VERSION_INFO VersionInfo; // LXSS only

    NTSTATUS ImageCoherencyStatus;
    FLOAT ImageCoherency;

    USHORT Architecture;
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
BOOLEAN PhEnableProcessExtension = TRUE;
BOOLEAN PhEnablePurgeProcessRecords = TRUE;
BOOLEAN PhEnableCycleCpuUsage = TRUE;
ULONG PhProcessProviderFlagsMask = 0;

PVOID PhProcessInformation = NULL; // only can be used if running on same thread as process provider
SYSTEM_PERFORMANCE_INFORMATION PhPerfInformation;
PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PhCpuInformation = NULL;
SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PhCpuTotals;
ULONG PhTotalProcesses = 0;
ULONG PhTotalThreads = 0;
ULONG PhTotalHandles = 0;
ULONG PhTotalCpuQueueLength = 0;

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

    PhProcessRecordList = PhCreateList(40);

    PhEnableProcessExtension = WindowsVersion >= WINDOWS_10_RS3 && !PhIsExecutingInWow64();

    RtlInitializeSListHead(&PhProcessQueryDataListHead);

    PhDpcsProcessInformation = PhAllocateZero(sizeof(SYSTEM_PROCESS_INFORMATION) + sizeof(SYSTEM_PROCESS_INFORMATION_EXTENSION));
    RtlInitUnicodeString(&PhDpcsProcessInformation->ImageName, L"DPCs");
    PhDpcsProcessInformation->UniqueProcessId = DPCS_PROCESS_ID;
    PhDpcsProcessInformation->InheritedFromUniqueProcessId = SYSTEM_IDLE_PROCESS_ID;

    PhInterruptsProcessInformation = PhAllocateZero(sizeof(SYSTEM_PROCESS_INFORMATION) + sizeof(SYSTEM_PROCESS_INFORMATION_EXTENSION));
    RtlInitUnicodeString(&PhInterruptsProcessInformation->ImageName, L"Interrupts");
    PhInterruptsProcessInformation->UniqueProcessId = INTERRUPTS_PROCESS_ID;
    PhInterruptsProcessInformation->InheritedFromUniqueProcessId = SYSTEM_IDLE_PROCESS_ID;

    PhCpuInformation = PhAllocateZero(
        sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) *
        PhSystemProcessorInformation.NumberOfProcessors
        );

    PhCpuIdleCycleTime = PhAllocateZero(
        sizeof(LARGE_INTEGER) *
        PhSystemProcessorInformation.NumberOfProcessors
        );
    PhCpuSystemCycleTime = PhAllocateZero(
        sizeof(LARGE_INTEGER) *
        PhSystemProcessorInformation.NumberOfProcessors
        );

    usageBuffer = PhAllocateZero(
        sizeof(FLOAT) *
        PhSystemProcessorInformation.NumberOfProcessors *
        2
        );
    deltaBuffer = PhAllocateZero(
        sizeof(PH_UINT64_DELTA) *
        PhSystemProcessorInformation.NumberOfProcessors *
        3 // 4 for PhCpusIdleCycleDelta
        );
    historyBuffer = PhAllocateZero(
        sizeof(PH_CIRCULAR_BUFFER_FLOAT) *
        PhSystemProcessorInformation.NumberOfProcessors *
        2
        );

    PhCpusKernelUsage = usageBuffer;
    PhCpusUserUsage = PhCpusKernelUsage + PhSystemProcessorInformation.NumberOfProcessors;

    PhCpusKernelDelta = deltaBuffer;
    PhCpusUserDelta = PhCpusKernelDelta + PhSystemProcessorInformation.NumberOfProcessors;
    PhCpusIdleDelta = PhCpusUserDelta + PhSystemProcessorInformation.NumberOfProcessors;
    //PhCpusIdleCycleDelta = PhCpusIdleDelta + PhGetSystemInformation.NumberOfProcessors;

    PhCpusKernelHistory = historyBuffer;
    PhCpusUserHistory = PhCpusKernelHistory + PhSystemProcessorInformation.NumberOfProcessors;

    return TRUE;
}

PPH_STRING PhGetClientIdName(
    _In_ PCLIENT_ID ClientId
    )
{
    return PhGetClientIdNameEx(ClientId, NULL);
}

PPH_STRING PhGetClientIdNameEx(
    _In_ PCLIENT_ID ClientId,
    _In_opt_ PPH_STRING ProcessName
    )
{
    PPH_STRING name;
    PPH_PROCESS_ITEM processItem = NULL;

    // Lookup the name in the process snapshot if necessary
    if (!ProcessName)
    {
        processItem = PhReferenceProcessItem(ClientId->UniqueProcess);

        if (processItem)
            ProcessName = processItem->ProcessName;
    }

    name = PhStdGetClientIdNameEx(ClientId, ProcessName);

    if (processItem)
        PhDereferenceObject(processItem);

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
    case PROCESS_PRIORITY_CLASS_UNKNOWN:
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

    //
    // Initialize ImageCoherencyStatus to STATUS_PENDING this notes that the
    // image coherency hasn't been done yet. This prevents the process items
    // from being noted as "Low Image Coherency" or being highlighted until
    // the analysis runs. See: PhpShouldShowImageCoherency
    //
    processItem->ImageCoherencyStatus = STATUS_PENDING;

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
    if (processItem->FileNameWin32) PhDereferenceObject(processItem->FileNameWin32);
    if (processItem->FileName) PhDereferenceObject(processItem->FileName);
    if (processItem->CommandLine) PhDereferenceObject(processItem->CommandLine);
    PhDeleteImageVersionInfo(&processItem->VersionInfo);
    if (processItem->Sid) PhFree(processItem->Sid);
    if (processItem->VerifySignerName) PhDereferenceObject(processItem->VerifySignerName);
    if (processItem->PackageFullName) PhDereferenceObject(processItem->PackageFullName);
    if (processItem->UserName) PhDereferenceObject(processItem->UserName);

    if (processItem->QueryHandle) NtClose(processItem->QueryHandle);

    if (processItem->Record) PhDereferenceProcessRecord(processItem->Record);

    if (processItem->IconEntry) PhDereferenceObject(processItem->IconEntry);
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

PPH_STRING PhpGetSidFullNameCachedSlow(
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

PPH_STRING PhpGetSidFullNameCached(
    _In_ PSID Sid
    )
{
    //if (!PhpSidFullNameCacheHashtable)
    //{
    //    PhpSidFullNameCacheHashtable = PhCreateHashtable(
    //        sizeof(PH_SID_FULL_NAME_CACHE_ENTRY),
    //        PhpSidFullNameCacheHashtableEqualFunction,
    //        PhpSidFullNameCacheHashtableHashFunction,
    //        16
    //        );
    //    // HACK pre-cache local SIDs (dmex)
    //    PhpGetSidFullNameCachedSlow(&PhSeLocalSystemSid);
    //    PhpGetSidFullNameCachedSlow(&PhSeLocalServiceSid);
    //    PhpGetSidFullNameCachedSlow(&PhSeNetworkServiceSid);
    //}

    if (PhpSidFullNameCacheHashtable)
    {
        PPH_SID_FULL_NAME_CACHE_ENTRY entry;
        PH_SID_FULL_NAME_CACHE_ENTRY lookupEntry;

        lookupEntry.Sid = Sid;
        entry = PhFindEntryHashtable(PhpSidFullNameCacheHashtable, &lookupEntry);

        if (entry)
            return PhReferenceObject(entry->FullName);
    }

    return NULL;
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

    // Version info
    if (!processItem->IsSubsystemProcess)
    {
        if (!PhIsNullOrEmptyString(processItem->FileName) && PhDoesFileExist(&processItem->FileName->sr))
        {
            LONG dpiValue = PhGetSystemDpi();

            Data->IconEntry = PhImageListExtractIcon(processItem->FileName, TRUE, dpiValue);

            PhInitializeImageVersionInfoCached(
                &Data->VersionInfo,
                processItem->FileName,
                FALSE,
                PhEnableVersionShortText
                );
        }

        if (PhEnableCycleCpuUsage && processId == INTERRUPTS_PROCESS_ID)
        {
            static PH_STRINGREF descriptionText = PH_STRINGREF_INIT(L"Interrupts and DPCs");
            PhMoveReference(&Data->VersionInfo.FileDescription, PhCreateString2(&descriptionText));
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
                    if (commandLine->Buffer[i] == UNICODE_NULL)
                        commandLine->Buffer[i] = L' ';
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
        if (KphLevel() >= KphLevelMed)
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
            // KSystemInformer is not available. We can determine if the process is in a job, but we
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
    if (processHandleLimited && WindowsVersion >= WINDOWS_8 && processItem->IsPackagedProcess && !processItem->IsSubsystemProcess)
    {
        Data->IsImmersive = !!PhIsImmersiveProcess(processHandleLimited);
    }

    // Package full name
    //if (processHandleLimited && ((WindowsVersion >= WINDOWS_8 && Data->IsImmersive) || WindowsVersion >= WINDOWS_10))
    //{
    //    Data->PackageFullName = PhGetProcessPackageFullName(processHandleLimited);
    //}

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
            if (!RtlAreAllAccessesGranted(basicInfo.GrantedAccess, PROCESS_QUERY_INFORMATION))
                Data->IsFilteredHandle = TRUE;
        }
        else
        {
            Data->IsFilteredHandle = TRUE;
        }
    }

    // Debugged
    if (
        processHandleLimited &&
        !processItem->IsSubsystemProcess &&
        !Data->IsFilteredHandle && // Don't query the debug object if the handle was filtered (dmex)
        processItem->ProcessId != SYSTEM_PROCESS_ID // Ignore the system process on 20H2 (dmex)
        )
    {
        BOOLEAN isBeingDebugged;

        if (NT_SUCCESS(PhGetProcessIsBeingDebugged(processHandleLimited, &isBeingDebugged)))
        {
            Data->IsBeingDebugged = isBeingDebugged;
        }
    }

    // Architecture
    if (PH_IS_REAL_PROCESS_ID(processItem->ProcessId))
    {
        status = STATUS_NOT_FOUND;

        //
        // First try to use the new API if it makes sense to. (jxy-s)
        //
        if (WindowsVersion >= WINDOWS_11 && processItem->QueryHandle)
        {
            USHORT processArchitecture;

            status = PhGetProcessArchitecture(
                processItem->QueryHandle,
                &processArchitecture
                );

            if (NT_SUCCESS(status))
            {
                Data->Architecture = processArchitecture;
            }
        }

        //
        // Check if we succeeded above. (jxy-s)
        //
        if (!NT_SUCCESS(status))
        {
            // Set the special value for a delayed stage2 query. (dmex)
            Data->Architecture = USHRT_MAX;
        }
    }

    if (processItem->Sid && FlagOn(PhProcessProviderFlagsMask, PH_PROCESS_PROVIDER_FLAG_USERNAME))
    {
        // Note: We delay resolving the SID name because the local LSA cache might still be
        // initializing for users on domain networks with slow links (e.g. VPNs). This can block
        // for a very long time depending on server/network conditions. (dmex)
        // TODO: This might need to be moved to Stage2...
        PhMoveReference(&Data->UserName, PhpGetSidFullNameCachedSlow(processItem->Sid));
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
            TRUE,
            FALSE
            );

        status = PhIsExecutablePacked(
            processItem->FileName,
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
            Data->ImportModules = ULONG_MAX;
            Data->ImportFunctions = ULONG_MAX;
        }

        if (processItem->Architecture == USHRT_MAX)
        {
            PH_MAPPED_IMAGE mappedImage;

            // For backward compatibility we'll read the Machine from the file.
            // If we fail to access the file we could go read from the remote
            // process memory, but for now we only read from the file. (jxy-s)

            if (NT_SUCCESS(PhLoadMappedImageEx(&processItem->FileName->sr, NULL, &mappedImage)))
            {
                Data->Architecture = (USHORT)mappedImage.NtHeaders->FileHeader.Machine;
                PhUnloadMappedImage(&mappedImage);
            }
        }
    }

    if (PhEnableImageCoherencySupport && processItem->FileName && !processItem->IsSubsystemProcess)
    {
        if (PhCsImageCoherencyScanLevel == 0)
        {
            //
            // If the user changes the configuration from 0 to >0 they will
            // need to re-run stage 2 analysis otherwise all images will show
            // low coherency.
            // [ENHANCEMENT] re-run stage 2 when this advanced option changes
            //
            processItem->ImageCoherencyStatus = STATUS_SUCCESS;
        }
        else
        {
            PH_IMAGE_COHERENCY_SCAN_TYPE type;

            switch (PhCsImageCoherencyScanLevel)
            {
            case 1:
                type = PhImageCoherencyQuick;
                break;
            case 2:
                type = PhImageCoherencyNormal;
                break;
            case 3:
            default:
                type = PhImageCoherencyFull;
                break;
            }

            Data->ImageCoherencyStatus = PhGetProcessImageCoherency(
                processItem->FileName,
                processItem->ProcessId,
                type,
                &Data->ImageCoherency
                );
        }
    }

    if (PhEnableLinuxSubsystemSupport && processItem->FileNameWin32 && processItem->IsSubsystemProcess)
    {
        PhInitializeImageVersionInfoCached(&Data->VersionInfo, processItem->FileNameWin32, TRUE, PhEnableVersionShortText);
    }
}

NTSTATUS PhpProcessQueryStage1Worker(
    _In_ PVOID Parameter
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;
    PPH_PROCESS_QUERY_S1_DATA data;

    data = PhAllocateZero(sizeof(PH_PROCESS_QUERY_S1_DATA));
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
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;
    PPH_PROCESS_QUERY_S2_DATA data;

    data = PhAllocateZero(sizeof(PH_PROCESS_QUERY_S2_DATA));
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

    PhQueueItemWorkQueueEx(PhGetGlobalWorkQueue(), PhpProcessQueryStage1Worker, ProcessItem, NULL, &environment);
}

VOID PhpQueueProcessQueryStage2(
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PH_WORK_QUEUE_ENVIRONMENT environment;

    PhReferenceObject(ProcessItem);

    PhInitializeWorkQueueEnvironment(&environment);
    environment.BasePriority = THREAD_PRIORITY_BELOW_NORMAL;
    environment.IoPriority = IoPriorityVeryLow;
    environment.PagePriority = MEMORY_PRIORITY_VERY_LOW;

    PhQueueItemWorkQueueEx(PhGetGlobalWorkQueue(), PhpProcessQueryStage2Worker, ProcessItem, NULL, &environment);
}

VOID PhpFillProcessItemStage1(
    _In_ PPH_PROCESS_QUERY_S1_DATA Data
    )
{
    PPH_PROCESS_ITEM processItem = Data->Header.ProcessItem;

    processItem->CommandLine = Data->CommandLine;
    if (Data->IconEntry)
    {
        processItem->SmallIconIndex = Data->IconEntry->SmallIconIndex;
        processItem->LargeIconIndex = Data->IconEntry->LargeIconIndex;
    }
    processItem->IconEntry = Data->IconEntry;
    memcpy(&processItem->VersionInfo, &Data->VersionInfo, sizeof(PH_IMAGE_VERSION_INFO));
    processItem->ConsoleHostProcessId = Data->ConsoleHostProcessId;
    processItem->IsDotNet = Data->IsDotNet;
    processItem->IsInJob = Data->IsInJob;
    processItem->IsInSignificantJob = Data->IsInSignificantJob;
    processItem->IsBeingDebugged = Data->IsBeingDebugged;
    processItem->IsImmersive = Data->IsImmersive;
    processItem->IsProtectedHandle = Data->IsFilteredHandle;
    processItem->Architecture = (USHORT)Data->Architecture;

    PhSwapReference(&processItem->Record->CommandLine, processItem->CommandLine);

    // Note: We might have referenced the cached username so don't overwrite the previous data. (dmex)
    if (!processItem->UserName)
        processItem->UserName = Data->UserName;
    else if (Data->UserName)
        PhDereferenceObject(Data->UserName);

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

    processItem->ImageCoherencyStatus = Data->ImageCoherencyStatus;
    processItem->ImageCoherency = Data->ImageCoherency;

    // Note: We query Win32 processes in stage1 so don't overwrite the previous data. (dmex)
    if (processItem->IsSubsystemProcess)
    {
        memcpy(&processItem->VersionInfo, &Data->VersionInfo, sizeof(PH_IMAGE_VERSION_INFO));
    }

    // Note: We query the architecture in stage1 so don't overwrite the previous data. (dmex)
    if (processItem->Architecture == USHRT_MAX)
    {
        processItem->Architecture = Data->Architecture;
    }
}

VOID PhpFillProcessItemExtension(
    _Inout_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PSYSTEM_PROCESS_INFORMATION Process
    )
{
    PSYSTEM_PROCESS_INFORMATION_EXTENSION processExtension;

    if (!PhEnableProcessExtension)
        return;

    processExtension = PH_PROCESS_EXTENSION(Process);

    ProcessItem->DiskCounters = processExtension->DiskCounters;
    ProcessItem->ContextSwitches = processExtension->ContextSwitches;
    ProcessItem->JobObjectId = processExtension->JobObjectId;
    ProcessItem->SharedCommitCharge = processExtension->SharedCommitCharge;
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
        ProcessItem->ProcessName = PhCreateStringFromUnicodeString(&SYSTEM_IDLE_PROCESS_NAME);

    if (PH_IS_REAL_PROCESS_ID(ProcessItem->ProcessId))
    {
        PhPrintUInt32(ProcessItem->ProcessIdString, HandleToUlong(ProcessItem->ProcessId));
        //PhPrintUInt32(ProcessItem->ParentProcessIdString, HandleToUlong(ProcessItem->ParentProcessId));
        //PhPrintUInt32(ProcessItem->SessionIdString, ProcessItem->SessionId);
    }

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
            ProcessItem->IsPackagedProcess = basicInfo.IsStronglyNamed;
            ProcessItem->IsWow64Valid = TRUE;
        }
    }

    // Process information
    {
        // If we're dealing with System (PID 4), we need to get the
        // kernel file name. Otherwise, get the image file name. (wj32)

        if (ProcessItem->ProcessId != SYSTEM_PROCESS_ID)
        {
            if (PH_IS_REAL_PROCESS_ID(ProcessItem->ProcessId))
            {
                PPH_STRING fileName;

                if (NT_SUCCESS(PhGetProcessImageFileNameByProcessId(ProcessItem->ProcessId, &fileName)))
                {
                    ProcessItem->FileName = fileName;
                }

                if (ProcessItem->QueryHandle)
                {
                    //if (NT_SUCCESS(PhGetProcessImageFileName(ProcessItem->QueryHandle, &fileName)))
                    //    ProcessItem->FileName = fileName;
                    if (NT_SUCCESS(PhGetProcessImageFileNameWin32(ProcessItem->QueryHandle, &fileName)))
                        ProcessItem->FileNameWin32 = fileName;
                }

                if (ProcessItem->FileName && PhIsNullOrEmptyString(ProcessItem->FileNameWin32))
                {
                    PhMoveReference(&ProcessItem->FileNameWin32, PhGetFileName(ProcessItem->FileName));
                }
            }
        }
        else
        {
            PPH_STRING fileName;

            if (fileName = PhGetKernelFileName())
            {
                ProcessItem->FileName = fileName;
                ProcessItem->FileNameWin32 = PhGetFileName(fileName);
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
            BOOLEAN isElevated;
            MANDATORY_LEVEL integrityLevel;
            PWSTR integrityString;

            // User
            if (NT_SUCCESS(PhGetTokenUser(tokenHandle, &tokenUser)))
            {
                ProcessItem->Sid = PhAllocateCopy(tokenUser->User.Sid, RtlLengthSid(tokenUser->User.Sid));
                ProcessItem->UserName = PhpGetSidFullNameCached(tokenUser->User.Sid);

                PhFree(tokenUser);
            }

            // Elevation
            if (NT_SUCCESS(PhGetTokenElevationType(tokenHandle, &elevationType)))
            {
                ProcessItem->ElevationType = elevationType;
            }

            if (NT_SUCCESS(PhGetTokenIsElevated(tokenHandle, &isElevated)))
            {
                ProcessItem->IsElevated = isElevated;
            }

            // Integrity
            if (NT_SUCCESS(PhGetTokenIntegrityLevel(tokenHandle, &integrityLevel, &integrityString)))
            {
                ProcessItem->IntegrityLevel = integrityLevel;
                ProcessItem->IntegrityString = integrityString;
            }

            // Package name
            if (WindowsVersion >= WINDOWS_8 && ProcessItem->IsPackagedProcess)
            {
                ProcessItem->PackageFullName = PhGetTokenPackageFullName(tokenHandle);
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
            ProcessItem->UserName = PhpGetSidFullNameCached(&PhSeLocalSystemSid);
        }
    }

    // Known Process Type
    {
        ProcessItem->KnownProcessType = PhGetProcessKnownTypeEx(
            ProcessItem->ProcessId,
            ProcessItem->FileNameWin32
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
    if (WindowsVersion >= WINDOWS_8_1 && ProcessItem->QueryHandle && FlagOn(PhProcessProviderFlagsMask, PH_PROCESS_PROVIDER_FLAG_CFGUARD))
    {
        BOOLEAN cfguardEnabled;

        if (NT_SUCCESS(PhGetProcessIsCFGuardEnabled(ProcessItem->QueryHandle, &cfguardEnabled)))
        {
            ProcessItem->IsControlFlowGuardEnabled = cfguardEnabled;
        }

        if (WindowsVersion >= WINDOWS_11)
        {
            BOOLEAN xfguardEnabled;
            BOOLEAN xfguardAuditEnabled;

            if (NT_SUCCESS(PhGetProcessIsXFGuardEnabled(ProcessItem->QueryHandle, &xfguardEnabled, &xfguardAuditEnabled)))
            {
                ProcessItem->IsXfgEnabled = xfguardEnabled;
                ProcessItem->IsXfgAuditEnabled = xfguardAuditEnabled;
            }
        }
    }

    // CET
    if (WindowsVersion >= WINDOWS_10_20H1 && FlagOn(PhProcessProviderFlagsMask, PH_PROCESS_PROVIDER_FLAG_CET))
    {
        if (ProcessItem->ProcessId == SYSTEM_PROCESS_ID)
        {
            SYSTEM_SHADOW_STACK_INFORMATION shadowStackInformation;

            if (NT_SUCCESS(PhGetSystemShadowStackInformation(&shadowStackInformation)))
            {
                ProcessItem->IsCetEnabled = shadowStackInformation.KernelCetEnabled; // Kernel CET is always strict (TheEragon)
            }
        }
        else
        {
            if (ProcessItem->QueryHandle)
            {
                BOOLEAN cetEnabled;
                BOOLEAN cetStrictModeEnabled;

                if (NT_SUCCESS(PhGetProcessIsCetEnabled(ProcessItem->QueryHandle, &cetEnabled, &cetStrictModeEnabled)))
                {
                    ProcessItem->IsCetEnabled = cetEnabled;
                }
            }
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
        UCHAR priorityClass;

        if (NT_SUCCESS(PhGetProcessPriority(ProcessItem->QueryHandle, &priorityClass)))
        {
            ProcessItem->PriorityClass = priorityClass;
        }
    }
    else
    {
        ProcessItem->PriorityClass = PROCESS_PRIORITY_CLASS_UNKNOWN;
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
    if (!NT_SUCCESS(NtQuerySystemInformation(
        SystemPerformanceInformation,
        &PhPerfInformation,
        sizeof(SYSTEM_PERFORMANCE_INFORMATION),
        NULL
        )))
    {
        memset(&PhPerfInformation, 0, sizeof(SYSTEM_PERFORMANCE_INFORMATION));
    }

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

    if (PhSystemProcessorInformation.SingleProcessorGroup)
    {
        if (!NT_SUCCESS(NtQuerySystemInformation(
            SystemProcessorPerformanceInformation,
            PhCpuInformation,
            sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * PhSystemProcessorInformation.NumberOfProcessors,
            NULL
            )))
        {
            memset(
                PhCpuInformation,
                0,
                sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * PhSystemProcessorInformation.NumberOfProcessors
                );
        }
    }
    else
    {
        USHORT processorCount = 0;

        for (USHORT processorGroup = 0; processorGroup < PhSystemProcessorInformation.NumberOfProcessorGroups; processorGroup++)
        {
            USHORT activeProcessorCount = PhGetActiveProcessorCount(processorGroup);

            if (!NT_SUCCESS(NtQuerySystemInformationEx(
                SystemProcessorPerformanceInformation,
                &processorGroup,
                sizeof(USHORT),
                PTR_ADD_OFFSET(PhCpuInformation, sizeof(SYSTEM_PROCESSOR_IDLE_INFORMATION) * processorCount),
                sizeof(SYSTEM_PROCESSOR_IDLE_INFORMATION) * activeProcessorCount,
                NULL
                )))
            {
                memset(
                    PTR_ADD_OFFSET(PhCpuInformation, sizeof(SYSTEM_PROCESSOR_IDLE_INFORMATION) * processorCount),
                    0,
                    sizeof(SYSTEM_PROCESSOR_IDLE_INFORMATION) * activeProcessorCount
                    );
            }

            processorCount += activeProcessorCount;
        }
    }

    // Zero the CPU totals.
    memset(&PhCpuTotals, 0, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION));

    for (i = 0; i < PhSystemProcessorInformation.NumberOfProcessors; i++)
    {
        PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION cpuInfo = &PhCpuInformation[i];

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
                PhCpusKernelUsage[i] = 0.0f;
                PhCpusUserUsage[i] = 0.0f;
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
            PhCpuKernelUsage = 0.0f;
            PhCpuUserUsage = 0.0f;
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

    if (PhSystemProcessorInformation.SingleProcessorGroup)
    {
        if (!NT_SUCCESS(NtQuerySystemInformation(
            SystemProcessorIdleCycleTimeInformation,
            PhCpuIdleCycleTime,
            sizeof(LARGE_INTEGER) * PhSystemProcessorInformation.NumberOfProcessors,
            NULL
            )))
        {
            memset(
                PhCpuIdleCycleTime,
                0,
                sizeof(LARGE_INTEGER) * PhSystemProcessorInformation.NumberOfProcessors
                );
        }
    }
    else
    {
        USHORT processorCount = 0;

        for (USHORT processorGroup = 0; processorGroup < PhSystemProcessorInformation.NumberOfProcessorGroups; processorGroup++)
        {
            USHORT activeProcessorCount = PhGetActiveProcessorCount(processorGroup);

            if (!NT_SUCCESS(NtQuerySystemInformationEx(
                SystemProcessorIdleCycleTimeInformation,
                &processorGroup,
                sizeof(USHORT),
                PTR_ADD_OFFSET(PhCpuIdleCycleTime, sizeof(LARGE_INTEGER) * processorCount),
                sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * activeProcessorCount,
                NULL
                )))
            {
                memset(
                    PTR_ADD_OFFSET(PhCpuIdleCycleTime, sizeof(LARGE_INTEGER) * processorCount),
                    0,
                    sizeof(LARGE_INTEGER) * activeProcessorCount
                    );
            }

            processorCount += activeProcessorCount;
        }
    }

    total = 0;

    for (i = 0; i < PhSystemProcessorInformation.NumberOfProcessors; i++)
    {
        //PhUpdateDelta(&PhCpusIdleCycleDelta[i], PhCpuIdleCycleTime[i].QuadPart);
        total += PhCpuIdleCycleTime[i].QuadPart;
    }

    PhUpdateDelta(&PhCpuIdleCycleDelta, total);
    *IdleCycleTime = PhCpuIdleCycleDelta.Delta;

    // System

    if (PhSystemProcessorInformation.SingleProcessorGroup)
    {
        if (!NT_SUCCESS(NtQuerySystemInformation(
            SystemProcessorCycleTimeInformation,
            PhCpuSystemCycleTime,
            sizeof(LARGE_INTEGER) * PhSystemProcessorInformation.NumberOfProcessors,
            NULL
            )))
        {
            memset(
                PhCpuSystemCycleTime,
                0,
                sizeof(LARGE_INTEGER) * PhSystemProcessorInformation.NumberOfProcessors
                );
        }
    }
    else
    {
        USHORT processorCount = 0;

        for (USHORT processorGroup = 0; processorGroup < PhSystemProcessorInformation.NumberOfProcessorGroups; processorGroup++)
        {
            USHORT activeProcessorCount = PhGetActiveProcessorCount(processorGroup);

            if (!NT_SUCCESS(NtQuerySystemInformationEx(
                SystemProcessorCycleTimeInformation,
                &processorGroup,
                sizeof(USHORT),
                PTR_ADD_OFFSET(PhCpuSystemCycleTime, sizeof(LARGE_INTEGER) * processorCount),
                sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * activeProcessorCount,
                NULL
                )))
            {
                memset(
                    PTR_ADD_OFFSET(PhCpuSystemCycleTime, sizeof(LARGE_INTEGER) * processorCount),
                    0,
                    sizeof(LARGE_INTEGER) * activeProcessorCount
                    );
            }

            processorCount += activeProcessorCount;
        }
    }

    total = 0;

    for (i = 0; i < PhSystemProcessorInformation.NumberOfProcessors; i++)
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

    for (i = 0; i < PhSystemProcessorInformation.NumberOfProcessors; i++)
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

    for (i = 0; i < PhSystemProcessorInformation.NumberOfProcessors; i++)
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
    for (i = 0; i < PhSystemProcessorInformation.NumberOfProcessors; i++)
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
    _Out_opt_ PULONG64 ContextSwitches,
    _Out_opt_ PULONG ProcessorQueueLength
    )
{
    ULONG i;
    BOOLEAN isSuspended;
    BOOLEAN isPartiallySuspended;
    ULONG64 contextSwitches;
    ULONG processorQueueLength;

    isSuspended = PH_IS_REAL_PROCESS_ID(Process->UniqueProcessId);
    isPartiallySuspended = FALSE;
    contextSwitches = 0;
    processorQueueLength = 0;

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

        if (Process->Threads[i].ThreadState == Ready)
        {
            processorQueueLength++;
        }

        contextSwitches += Process->Threads[i].ContextSwitches;
    }

    // HACK: Minimal/Reflected processes don't have threads. (dmex)
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
    if (ProcessorQueueLength)
        *ProcessorQueueLength = processorQueueLength;
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

        PhpFlushSidFullNameCache();

        PhImageListFlushCache();

        PhFlushImageVersionInfoCache();

        //PhFlushVerifyCache();
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
    PhTotalCpuQueueLength = 0;

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

            if (PhEnableProcessExtension)
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

                if (PhEnableProcessExtension)
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
            ULONG64 contextSwitches;
            ULONG processorQueueLength;

            // Create the process item and fill in basic information.
            processItem = PhCreateProcessItem(process->UniqueProcessId);
            PhpFillProcessItem(processItem, process);
            PhpFillProcessItemExtension(processItem, process);
            processItem->TimeSequenceNumber = PhTimeSequenceNumber;

            processRecord = PhpCreateProcessRecord(processItem);
            PhpAddProcessRecord(processRecord);
            processItem->Record = processRecord;

            PhpGetProcessThreadInformation(process, &isSuspended, &isPartiallySuspended, &contextSwitches, &processorQueueLength);
            PhpUpdateDynamicInfoProcessItem(processItem, process);
            PhTotalCpuQueueLength += processorQueueLength;

            // Initialize the deltas.
            PhUpdateDelta(&processItem->CpuKernelDelta, process->KernelTime.QuadPart);
            PhUpdateDelta(&processItem->CpuUserDelta, process->UserTime.QuadPart);
            PhUpdateDelta(&processItem->IoReadDelta, process->ReadTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoWriteDelta, process->WriteTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoOtherDelta, process->OtherTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoReadCountDelta, process->ReadOperationCount.QuadPart);
            PhUpdateDelta(&processItem->IoWriteCountDelta, process->WriteOperationCount.QuadPart);
            PhUpdateDelta(&processItem->IoOtherCountDelta, process->OtherOperationCount.QuadPart);
            PhUpdateDelta(&processItem->ContextSwitchesDelta, PhEnableProcessExtension ? processItem->ContextSwitches : contextSwitches);
            PhUpdateDelta(&processItem->PageFaultsDelta, process->PageFaultCount);
            PhUpdateDelta(&processItem->HardFaultsDelta, process->HardFaultCount);
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
            ULONG64 contextSwitches;
            ULONG readyThreads;
            FLOAT newCpuUsage;
            FLOAT kernelCpuUsage;
            FLOAT userCpuUsage;

            PhpGetProcessThreadInformation(process, &isSuspended, &isPartiallySuspended, &contextSwitches, &readyThreads);
            PhpUpdateDynamicInfoProcessItem(processItem, process);
            PhpFillProcessItemExtension(processItem, process);
            PhTotalCpuQueueLength += readyThreads;

            // Update the deltas.
            PhUpdateDelta(&processItem->CpuKernelDelta, process->KernelTime.QuadPart);
            PhUpdateDelta(&processItem->CpuUserDelta, process->UserTime.QuadPart);
            PhUpdateDelta(&processItem->IoReadDelta, process->ReadTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoWriteDelta, process->WriteTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoOtherDelta, process->OtherTransferCount.QuadPart);
            PhUpdateDelta(&processItem->IoReadCountDelta, process->ReadOperationCount.QuadPart);
            PhUpdateDelta(&processItem->IoWriteCountDelta, process->WriteOperationCount.QuadPart);
            PhUpdateDelta(&processItem->IoOtherCountDelta, process->OtherOperationCount.QuadPart);
            PhUpdateDelta(&processItem->ContextSwitchesDelta, PhEnableProcessExtension ? processItem->ContextSwitches : contextSwitches);
            PhUpdateDelta(&processItem->PageFaultsDelta, process->PageFaultCount);
            PhUpdateDelta(&processItem->HardFaultsDelta, process->HardFaultCount);
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

            // Average
            if (FlagOn(PhProcessProviderFlagsMask, PH_PROCESS_PROVIDER_FLAG_AVERAGE))
            {
                FLOAT value = 0.0f;

                for (ULONG i = 0; i < processItem->CpuKernelHistory.Count; i++)
                {
                    value += PhGetItemCircularBuffer_FLOAT(&processItem->CpuKernelHistory, i) +
                        PhGetItemCircularBuffer_FLOAT(&processItem->CpuUserHistory, i);
                }

                if (processItem->CpuKernelHistory.Count)
                    processItem->CpuAverageUsage = (FLOAT)value / processItem->CpuKernelHistory.Count;
                else
                    processItem->CpuAverageUsage = 0.0f;
            }

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
                processItem->ProcessId != SYSTEM_PROCESS_ID // System token can't be opened on XP (wj32)
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
                    //BOOLEAN isElevated;
                    //TOKEN_ELEVATION_TYPE elevationType;
                    MANDATORY_LEVEL integrityLevel;
                    PWSTR integrityString;

                    if (FlagOn(PhProcessProviderFlagsMask, PH_PROCESS_PROVIDER_FLAG_USERNAME))
                    {
                        // User
                        if (NT_SUCCESS(PhGetTokenUser(tokenHandle, &tokenUser)))
                        {
                            if (!processItem->Sid || !RtlEqualSid(processItem->Sid, tokenUser->User.Sid))
                            {
                                PSID processSid;

                                // HACK (dmex)
                                processSid = processItem->Sid;
                                processItem->Sid = PhAllocateCopy(tokenUser->User.Sid, RtlLengthSid(tokenUser->User.Sid));
                                if (processSid) PhFree(processSid);
                                modified = TRUE;
                            }

                            PhFree(tokenUser);
                        }

                        if (PhIsNullOrEmptyString(processItem->UserName) && processItem->Sid)
                        {
                            PhMoveReference(&processItem->UserName, PhpGetSidFullNameCachedSlow(processItem->Sid));
                            modified = TRUE;
                        }
                    }

                    // Elevation

                    //if (NT_SUCCESS(PhGetTokenIsElevated(tokenHandle, &isElevated)))
                    //{
                    //    if (processItem->IsElevated != isElevated)
                    //    {
                    //        processItem->IsElevated = isElevated;
                    //        modified = TRUE;
                    //    }
                    //}
                    //
                    //if (NT_SUCCESS(PhGetTokenElevationType(tokenHandle, &elevationType)))
                    //{
                    //    if (processItem->ElevationType != elevationType)
                    //    {
                    //        processItem->ElevationType = elevationType;
                    //        modified = TRUE;
                    //    }
                    //}

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
            else
            {
                if (FlagOn(PhProcessProviderFlagsMask, PH_PROCESS_PROVIDER_FLAG_USERNAME))
                {
                    if (processItem->ProcessId == SYSTEM_IDLE_PROCESS_ID ||
                        processItem->ProcessId == SYSTEM_PROCESS_ID)
                    {
                        if (PhIsNullOrEmptyString(processItem->UserName))
                        {
                            PhMoveReference(&processItem->UserName, PhpGetSidFullNameCachedSlow(&PhSeLocalSystemSid));
                            modified = TRUE;
                        }
                    }
                }
            }

            // Control Flow Guard
            if (WindowsVersion >= WINDOWS_8_1 && processItem->QueryHandle && FlagOn(PhProcessProviderFlagsMask, PH_PROCESS_PROVIDER_FLAG_CFGUARD))
            {
                BOOLEAN cfguardEnabled;

                if (NT_SUCCESS(PhGetProcessIsCFGuardEnabled(processItem->QueryHandle, &cfguardEnabled)))
                {
                    if (processItem->IsControlFlowGuardEnabled != cfguardEnabled)
                    {
                        processItem->IsControlFlowGuardEnabled = cfguardEnabled;
                        modified = TRUE;
                    }
                }

                if (WindowsVersion >= WINDOWS_11)
                {
                    BOOLEAN xfguardEnabled;
                    BOOLEAN xfguardAuditEnabled;

                    if (NT_SUCCESS(PhGetProcessIsXFGuardEnabled(processItem->QueryHandle, &xfguardEnabled, &xfguardAuditEnabled)))
                    {
                        if (processItem->IsXfgEnabled != xfguardEnabled)
                        {
                            processItem->IsXfgEnabled = xfguardEnabled;
                            modified = TRUE;
                        }

                        if (processItem->IsXfgAuditEnabled != xfguardAuditEnabled)
                        {
                            processItem->IsXfgAuditEnabled = xfguardAuditEnabled;
                            modified = TRUE;
                        }
                    }
                }
            }

            // CET
            if (WindowsVersion >= WINDOWS_10_20H1 && FlagOn(PhProcessProviderFlagsMask, PH_PROCESS_PROVIDER_FLAG_CET))
            {
                if (processItem->ProcessId == SYSTEM_PROCESS_ID)
                {
                    SYSTEM_SHADOW_STACK_INFORMATION shadowStackInformation;

                    if (NT_SUCCESS(PhGetSystemShadowStackInformation(&shadowStackInformation)))
                    {
                        if (processItem->IsCetEnabled != shadowStackInformation.KernelCetEnabled)
                        {
                            processItem->IsCetEnabled = shadowStackInformation.KernelCetEnabled;
                            modified = TRUE;
                        }
                    }
                }
                else
                {
                    if (processItem->QueryHandle)
                    {
                        BOOLEAN cetEnabled;
                        BOOLEAN cetStrictModeEnabled;

                        if (NT_SUCCESS(PhGetProcessIsCetEnabled(processItem->QueryHandle, &cetEnabled, &cetStrictModeEnabled)))
                        {
                            if (processItem->IsCetEnabled != cetEnabled)
                            {
                                processItem->IsCetEnabled = cetEnabled;
                                modified = TRUE;
                            }
                        }
                    }
                }
            }

            // Job
            if (processItem->QueryHandle)
            {
                NTSTATUS status;
                BOOLEAN isInSignificantJob = FALSE;
                BOOLEAN isInJob = FALSE;

                if (KphLevel() >= KphLevelMed)
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

                        isInJob = TRUE;

                        if (NT_SUCCESS(PhGetJobBasicLimits(jobHandle, &basicLimits)))
                        {
                            isInSignificantJob = basicLimits.LimitFlags != JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
                        }
                    }

                    if (jobHandle)
                        NtClose(jobHandle);
                }
                else
                {
                    status = NtIsProcessInJob(processItem->QueryHandle, NULL);

                    if (NT_SUCCESS(status))
                        isInJob = status == STATUS_PROCESS_IN_JOB;
                }

                if (processItem->IsInSignificantJob != isInSignificantJob)
                {
                    processItem->IsInSignificantJob = isInSignificantJob;
                    modified = TRUE;
                }

                if (processItem->IsInJob != isInJob)
                {
                    processItem->IsInJob = isInJob;
                    modified = TRUE;
                }
            }

            // Debugged
            if (
                processItem->QueryHandle &&
                !processItem->IsSubsystemProcess &&
                !processItem->IsProtectedHandle && // Don't query the debug object if the handle was filtered (dmex)
                processItem->ProcessId != SYSTEM_PROCESS_ID // Ignore the system process on 20H2 (dmex)
                )
            {
                BOOLEAN isBeingDebugged = FALSE;

                PhGetProcessIsBeingDebugged(processItem->QueryHandle, &isBeingDebugged);

                if (processItem->IsBeingDebugged != isBeingDebugged)
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

            if (PhCsUseColorPartiallySuspended) // HACK // Don't invalidate for partially suspended unless enabled (dmex)
            {
                if (processItem->IsPartiallySuspended != isPartiallySuspended)
                {
                    processItem->IsPartiallySuspended = isPartiallySuspended;
                    modified = TRUE;
                }
            }
            else
            {
                processItem->IsPartiallySuspended = isPartiallySuspended;
            }

            // .NET
            if (processItem->UpdateIsDotNet)
            {
                BOOLEAN isDotNet;
                ULONG flags = 0;

                if (NT_SUCCESS(PhGetProcessIsDotNetEx(processItem->ProcessId, NULL, 0, &isDotNet, &flags)))
                {
                    processItem->IsDotNet = isDotNet;
                    modified = TRUE;
                }

                processItem->UpdateIsDotNet = FALSE;
            }

            // Immersive
            // if (processItem->QueryHandle && WindowsVersion >= WINDOWS_8 && !processItem->IsSubsystemProcess)
            // {
            //     BOOLEAN isImmersive;
            //
            //     isImmersive = PhIsImmersiveProcess(processItem->QueryHandle);
            //
            //     if (processItem->IsImmersive != isImmersive)
            //     {
            //         processItem->IsImmersive = isImmersive;
            //         modified = TRUE;
            //     }
            // }

            if (processItem->QueryHandle && processItem->IsHandleValid)
            {
                OBJECT_BASIC_INFORMATION basicInfo;
                BOOLEAN filteredHandle = FALSE;

                if (NT_SUCCESS(PhGetHandleInformationEx(
                    NtCurrentProcess(),
                    processItem->QueryHandle,
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
                    if (!RtlAreAllAccessesGranted(basicInfo.GrantedAccess, PROCESS_QUERY_INFORMATION))
                    {
                        filteredHandle = TRUE;
                    }
                }
                else
                {
                    filteredHandle = TRUE;
                }

                if (processItem->IsProtectedHandle != filteredHandle)
                {
                    processItem->IsProtectedHandle = filteredHandle;
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
    PhSetReference(&processRecord->FileName, ProcessItem->FileNameWin32);
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

                    if (derefList)
                    {
                        PhAddItemList(derefList, processRecord);
                    }
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

    if (PhEnableProcessExtension)
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

    if (PhEnableProcessExtension)
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

PPH_HASHTABLE PhImageListCacheHashtable = NULL;
PH_QUEUED_LOCK PhImageListCacheHashtableLock = PH_QUEUED_LOCK_INIT;
HIMAGELIST PhProcessLargeImageList = NULL;
HIMAGELIST PhProcessSmallImageList = NULL;
PPH_OBJECT_TYPE PhImageListItemType = NULL;

VOID PhpImageListItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_IMAGELIST_ITEM entry = (PPH_IMAGELIST_ITEM)Object;
    // TODO: Remove TN_CACHE from TreeNewGetNodeIcon so we're
    // able to sync the updated image index after ImageList_Remove. (dmex)
    //ULONG LargeIconIndex = entry->LargeIconIndex;
    //ULONG SmallIconIndex = entry->SmallIconIndex;
    //PPH_PROCESS_ITEM* processes;
    //ULONG numberOfProcesses;
    //
    //PhEnumProcessItems(&processes, &numberOfProcesses);
    //
    //for (ULONG i = 0; i < numberOfProcesses; i++)
    //{
    //    PPH_PROCESS_ITEM process = processes[i];
    //
    //    if (
    //        process->LargeIconIndex > LargeIconIndex &&
    //        process->SmallIconIndex > SmallIconIndex
    //        )
    //    {
    //        process->LargeIconIndex -= 1;
    //        process->SmallIconIndex -= 1;
    //    }
    //}
    //
    //PhFree(processes);
    //
    //ImageList_Remove(PhProcessLargeImageList, LargeIconIndex);
    //ImageList_Remove(PhProcessSmallImageList, SmallIconIndex);
    //
    PhDereferenceObject(entry->FileName);
}

VOID PhProcessImageListInitialization(
    _In_ HWND hwnd
    )
{
    HICON iconSmall;
    HICON iconLarge;
    LONG dpiValue;

    if (!PhImageListItemType)
        PhImageListItemType = PhCreateObjectType(L"ImageListItem", 0, PhpImageListItemDeleteProcedure);

    dpiValue = PhGetWindowDpi(hwnd);

    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PPH_IMAGELIST_ITEM* entry;
    PPH_LIST fileNames = NULL;

    if (PhImageListCacheHashtable)
    {
        fileNames = PhCreateList(PhImageListCacheHashtable->Count);

        PhAcquireQueuedLockExclusive(&PhImageListCacheHashtableLock);
        PhBeginEnumHashtable(PhImageListCacheHashtable, &enumContext);

        while (entry = PhNextEnumHashtable(&enumContext))
        {
            PPH_IMAGELIST_ITEM item = *entry;
            PhAddItemList(fileNames, PhReferenceObject(item->FileName));
            PhDereferenceObject(item);
        }

        PhDereferenceObject(PhImageListCacheHashtable);
        PhImageListCacheHashtable = NULL;

        PhReleaseQueuedLockExclusive(&PhImageListCacheHashtableLock);
    }

    if (PhProcessLargeImageList) PhImageListDestroy(PhProcessLargeImageList);
    if (PhProcessSmallImageList) PhImageListDestroy(PhProcessSmallImageList);
    PhProcessLargeImageList = PhImageListCreate(
        PhGetSystemMetrics(SM_CXICON, dpiValue),
        PhGetSystemMetrics(SM_CYICON, dpiValue),
        ILC_MASK | ILC_COLOR32,
        100,
        100);
    PhProcessSmallImageList = PhImageListCreate(
        PhGetSystemMetrics(SM_CXSMICON, dpiValue),
        PhGetSystemMetrics(SM_CYSMICON, dpiValue),
        ILC_MASK | ILC_COLOR32,
        100,
        100);

    PhImageListSetBkColor(PhProcessLargeImageList, CLR_NONE);
    PhImageListSetBkColor(PhProcessSmallImageList, CLR_NONE);

    PhGetStockApplicationIcon(&iconSmall, &iconLarge);
    PhImageListAddIcon(PhProcessLargeImageList, iconLarge);
    PhImageListAddIcon(PhProcessSmallImageList, iconSmall);

    iconLarge = PhLoadIcon(PhInstanceHandle, MAKEINTRESOURCE(IDI_COG), PH_LOAD_ICON_SIZE_LARGE, 0, 0, dpiValue);
    iconSmall = PhLoadIcon(PhInstanceHandle, MAKEINTRESOURCE(IDI_COG), PH_LOAD_ICON_SIZE_SMALL, 0, 0, dpiValue);
    PhImageListAddIcon(PhProcessLargeImageList, iconLarge);
    PhImageListAddIcon(PhProcessSmallImageList, iconSmall);
    DestroyIcon(iconLarge);
    DestroyIcon(iconSmall);

    if (fileNames)
    {
        for (ULONG i = 0; i < fileNames->Count; i++)
        {
            PPH_STRING filename = fileNames->Items[i];
            PPH_IMAGELIST_ITEM iconEntry;

            dpiValue = PhGetSystemDpi();
            iconEntry = PhImageListExtractIcon(filename, TRUE, dpiValue);

            if (iconEntry)
            {
                PPH_PROCESS_ITEM* processes;
                ULONG numberOfProcesses;

                PhEnumProcessItems(&processes, &numberOfProcesses);

                for (ULONG i = 0; i < numberOfProcesses; i++)
                {
                    PPH_PROCESS_ITEM process = processes[i];

                    if (process->FileName && PhEqualString(process->FileName, filename, TRUE))
                    {
                        process->IconEntry = PhReferenceObject(iconEntry);
                        process->SmallIconIndex = iconEntry->SmallIconIndex;
                        process->LargeIconIndex = iconEntry->LargeIconIndex;
                    }
                }

                PhDereferenceObjects(processes, numberOfProcesses);
                PhFree(processes);

                PhDereferenceObject(iconEntry);
            }
        }

        PhDereferenceObjects(fileNames->Items, fileNames->Count);
        PhDereferenceObject(fileNames);
    }
}

BOOLEAN PhImageListCacheHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_IMAGELIST_ITEM entry1 = *(PPH_IMAGELIST_ITEM*)Entry1;
    PPH_IMAGELIST_ITEM entry2 = *(PPH_IMAGELIST_ITEM*)Entry2;

    return PhEqualStringRef(&entry1->FileName->sr, &entry2->FileName->sr, TRUE);
}

ULONG PhImageListCacheHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PPH_IMAGELIST_ITEM entry = *(PPH_IMAGELIST_ITEM*)Entry;

    return PhHashStringRefEx(&entry->FileName->sr, TRUE, PH_STRING_HASH_X65599);
}

PPH_IMAGELIST_ITEM PhImageListExtractIcon(
    _In_ PPH_STRING FileName,
    _In_ BOOLEAN NativeFileName,
    _In_ LONG dpiValue
    )
{
    HICON largeIcon = NULL;
    HICON smallIcon = NULL;
    PPH_IMAGELIST_ITEM newentry;

    PhAcquireQueuedLockShared(&PhImageListCacheHashtableLock);

    if (PhImageListCacheHashtable)
    {
        PH_IMAGELIST_ITEM lookupEntry;
        PPH_IMAGELIST_ITEM lookupEntryPtr = &lookupEntry;
        PPH_IMAGELIST_ITEM* entry;

        lookupEntry.FileName = FileName;

        entry = (PPH_IMAGELIST_ITEM*)PhFindEntryHashtable(PhImageListCacheHashtable, &lookupEntryPtr);

        if (entry)
        {
            PPH_IMAGELIST_ITEM foundEntry = *entry;

            PhReferenceObject(foundEntry);

            PhReleaseQueuedLockShared(&PhImageListCacheHashtableLock);

            return foundEntry;
        }
    }

    PhReleaseQueuedLockShared(&PhImageListCacheHashtableLock);

    PhExtractIconEx(
        FileName,
        NativeFileName,
        0,
        &largeIcon,
        &smallIcon,
        dpiValue
        );

    PhAcquireQueuedLockExclusive(&PhImageListCacheHashtableLock);

    if (!PhImageListCacheHashtable)
    {
        PhImageListCacheHashtable = PhCreateHashtable(
            sizeof(PPH_IMAGELIST_ITEM),
            PhImageListCacheHashtableEqualFunction,
            PhImageListCacheHashtableHashFunction,
            32
            );
    }

    newentry = PhCreateObjectZero(sizeof(PH_IMAGELIST_ITEM), PhImageListItemType);
    newentry->FileName = PhReferenceObject(FileName);

    if (largeIcon && smallIcon)
    {
        newentry->LargeIconIndex = PhImageListAddIcon(PhProcessLargeImageList, largeIcon);
        newentry->SmallIconIndex = PhImageListAddIcon(PhProcessSmallImageList, smallIcon);
        DestroyIcon(smallIcon);
        DestroyIcon(largeIcon);
    }
    else
    {
        newentry->LargeIconIndex = 0;
        newentry->SmallIconIndex = 0;
    }

    PhReferenceObject(newentry);
    PhAddEntryHashtable(PhImageListCacheHashtable, &newentry);

    PhReleaseQueuedLockExclusive(&PhImageListCacheHashtableLock);

    return newentry;
}

VOID PhImageListFlushCache(
    VOID
    )
{
    //PH_HASHTABLE_ENUM_CONTEXT enumContext;
    //PPH_IMAGELIST_ITEM* entry;
    //
    //if (!PhImageListCacheHashtable)
    //    return;
    //
    //PhAcquireQueuedLockExclusive(&PhImageListCacheHashtableLock);
    //
    //PhBeginEnumHashtable(PhImageListCacheHashtable, &enumContext);
    //
    //while (entry = PhNextEnumHashtable(&enumContext))
    //{
    //    PPH_IMAGELIST_ITEM item = *entry;
    //
    //    PhDereferenceObject(item);
    //}
    //
    //PhClearReference(&PhImageListCacheHashtable);
    //PhImageListCacheHashtable = PhCreateHashtable(
    //    sizeof(PPH_IMAGELIST_ITEM),
    //    PhImageListCacheHashtableEqualFunction,
    //    PhImageListCacheHashtableHashFunction,
    //    32
    //    );
    //
    //PhReleaseQueuedLockExclusive(&PhImageListCacheHashtableLock);
}

VOID PhDrawProcessIcon(
    _In_ HDC hdc,
    _In_ RECT rect,
    _In_ ULONG Index,
    _In_ BOOLEAN Large)
{
    if (Large)
    {
        if (PhProcessLargeImageList)
        {
            PhImageListDrawIcon(
                PhProcessLargeImageList,
                Index,
                hdc,
                rect.left,
                rect.top,
                ILD_NORMAL | ILD_TRANSPARENT,
                FALSE
                );
        }
    }
    else
    {
        if (PhProcessSmallImageList)
        {
            PhImageListDrawIcon(
                PhProcessSmallImageList,
                Index,
                hdc,
                rect.left,
                rect.top,
                ILD_NORMAL | ILD_TRANSPARENT,
                FALSE
                );
        }
    }
}

HICON PhGetImageListIcon(
    _In_ ULONG_PTR Index,
    _In_ BOOLEAN Large
    )
{
    if (Large)
    {
        if (PhProcessLargeImageList)
        {
            return PhImageListGetIcon(
                PhProcessLargeImageList,
                (ULONG)Index,
                ILD_NORMAL | ILD_TRANSPARENT
                );
        }
    }
    else
    {
        if (PhProcessSmallImageList)
        {
            return PhImageListGetIcon(
                PhProcessSmallImageList,
                (ULONG)Index,
                ILD_NORMAL | ILD_TRANSPARENT
                );
        }
    }

    return NULL;
}

HIMAGELIST PhGetProcessSmallImageList(
    VOID)
{
    return PhProcessSmallImageList;
}
