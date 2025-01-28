/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2023
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
            ULONG PowerThrottling : 1;
            ULONG Spare : 24;
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
BOOLEAN PhEnablePackageIconSupport = FALSE;
ULONG PhProcessProviderFlagsMask = 0;
LONG PhProcessImageListWindowDpi = 96;

PVOID PhProcessInformation = NULL; // only can be used if running on same thread as process provider
SYSTEM_PERFORMANCE_INFORMATION PhPerfInformation;
PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PhCpuInformation = NULL;
SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PhCpuTotals;
ULONG PhTotalProcesses = 0;
ULONG PhTotalThreads = 0;
ULONG PhTotalHandles = 0;
ULONG PhTotalCpuQueueLength = 0;

UCHAR PhDpcsProcessInformationBuffer[sizeof(SYSTEM_PROCESS_INFORMATION) + sizeof(SYSTEM_PROCESS_INFORMATION_EXTENSION)] = { 0 }; // HACK (dmex)
UCHAR PhInterruptsProcessInformationBuffer[sizeof(SYSTEM_PROCESS_INFORMATION) + sizeof(SYSTEM_PROCESS_INFORMATION_EXTENSION)] = { 0 };
PSYSTEM_PROCESS_INFORMATION PhDpcsProcessInformation = (PSYSTEM_PROCESS_INFORMATION)PhDpcsProcessInformationBuffer;
PSYSTEM_PROCESS_INFORMATION PhInterruptsProcessInformation = (PSYSTEM_PROCESS_INFORMATION)PhInterruptsProcessInformationBuffer;

ULONG64 PhCpuTotalCycleDelta = 0; // real cycle time delta for this period
PSYSTEM_PROCESSOR_IDLE_CYCLE_TIME_INFORMATION PhCpuIdleCycleTime = NULL; // cycle time for Idle
PSYSTEM_PROCESSOR_CYCLE_TIME_INFORMATION PhCpuSystemCycleTime = NULL; // cycle time for DPCs and Interrupts
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

BOOLEAN PhProcessStatisticsInitialized = FALSE;
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

static PPH_STRING PhpProtectionUnknownString = NULL;
static PPH_STRING PhpProtectionYesString = NULL;
static PPH_STRING PhpProtectionNoneString = NULL;
static PPH_STRING PhpProtectionSecureIUMString = NULL;

static CONST PH_KEY_VALUE_PAIR PhProtectedTypeStrings[] =
{
    SIP(L"None", PsProtectedTypeNone),
    SIP(L"Light", PsProtectedTypeProtectedLight),
    SIP(L"Full", PsProtectedTypeProtected),
};

static CONST PH_KEY_VALUE_PAIR PhProtectedSignerStrings[] =
{
    SIP(L"Authenticode", PsProtectedSignerAuthenticode),
    SIP(L"CodeGen", PsProtectedSignerCodeGen),
    SIP(L"Antimalware", PsProtectedSignerAntimalware),
    SIP(L"Lsa", PsProtectedSignerLsa),
    SIP(L"Windows", PsProtectedSignerWindows),
    SIP(L"WinTcb", PsProtectedSignerWinTcb),
    SIP(L"WinSystem", PsProtectedSignerWinSystem),
    SIP(L"StoreApp", PsProtectedSignerApp),
};

PPH_STRING PhpGetProtectionString(
    _In_ PS_PROTECTION Protection,
    _In_ BOOLEAN IsSecureProcess
    )
{
    PH_FORMAT format[5];
    ULONG count = 0;
    PWSTR type = L"Unknown";
    PWSTR signer = L"";

    if (Protection.Level == 0)
    {
        if (IsSecureProcess)
            return PhReferenceObject(PhpProtectionSecureIUMString);
        else
            return PhReferenceObject(PhpProtectionNoneString);
    }

    PhFindStringSiKeyValuePairs(PhProtectedTypeStrings, sizeof(PhProtectedTypeStrings), Protection.Type, &type);
    PhFindStringSiKeyValuePairs(PhProtectedSignerStrings, sizeof(PhProtectedSignerStrings), Protection.Signer, &signer);

    if (IsSecureProcess)
        PhInitFormatS(&format[count++], L"Secure ");

    PhInitFormatS(&format[count++], type);

    if (signer[0] != UNICODE_NULL)
    {
        PhInitFormatS(&format[count++], L" (");
        PhInitFormatS(&format[count++], signer);
        PhInitFormatS(&format[count++], Protection.Audit ? L", Audit)" : L")");
    }
    else if (Protection.Audit)
    {
        PhInitFormatS(&format[count++], L" (Audit)");
    }

    return PhFormat(format, count, 10);
}

BOOLEAN PhProcessProviderInitialization(
    VOID
    )
{
    PFLOAT usageBuffer;
    PPH_UINT64_DELTA deltaBuffer;
    PPH_CIRCULAR_BUFFER_FLOAT historyBuffer;

    PhProcessItemType = PhCreateObjectType(L"ProcessItem", 0, PhpProcessItemDeleteProcedure);

    PhProcessRecordList = PhCreateList(512);

    PhEnableProcessExtension = WindowsVersion >= WINDOWS_10_RS3 && !PhIsExecutingInWow64();

    PhInitializeSListHead(&PhProcessQueryDataListHead);

    RtlInitUnicodeString(&PhDpcsProcessInformation->ImageName, L"DPCs");
    PhDpcsProcessInformation->UniqueProcessId = DPCS_PROCESS_ID;
    PhDpcsProcessInformation->InheritedFromUniqueProcessId = SYSTEM_IDLE_PROCESS_ID;

    RtlInitUnicodeString(&PhInterruptsProcessInformation->ImageName, L"Interrupts");
    PhInterruptsProcessInformation->UniqueProcessId = INTERRUPTS_PROCESS_ID;
    PhInterruptsProcessInformation->InheritedFromUniqueProcessId = SYSTEM_IDLE_PROCESS_ID;

    PhCpuInformation = PhAllocateZero(
        sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) *
        PhSystemProcessorInformation.NumberOfProcessors
        );
    PhCpuIdleCycleTime = PhAllocateZero(
        sizeof(SYSTEM_PROCESSOR_IDLE_CYCLE_TIME_INFORMATION) *
        PhSystemProcessorInformation.NumberOfProcessors
        );
    PhCpuSystemCycleTime = PhAllocateZero(
        sizeof(SYSTEM_PROCESSOR_CYCLE_TIME_INFORMATION) *
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

    PhpProtectionUnknownString = PhCreateString(L"Unknown");
    PhpProtectionYesString = PhCreateString(L"Yes");
    PhpProtectionNoneString = PhCreateString(L"None");
    PhpProtectionSecureIUMString = PhCreateString(L"Secure (IUM)");

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
    if (processItem->ProtectionString) PhDereferenceObject(processItem->ProtectionString);
    if (processItem->VerifySignerName) PhDereferenceObject(processItem->VerifySignerName);
    if (processItem->PackageFullName) PhDereferenceObject(processItem->PackageFullName);
    if (processItem->UserName) PhDereferenceObject(processItem->UserName);

    if (processItem->QueryHandle) NtClose(processItem->QueryHandle);
    if (processItem->FreezeHandle) NtClose(processItem->FreezeHandle);

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
    _In_opt_ HANDLE ProcessId
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
    _In_opt_ HANDLE ProcessId
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

    return PhEqualSid(entry1->Sid, entry2->Sid);
}

ULONG PhpSidFullNameCacheHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PPH_SID_FULL_NAME_CACHE_ENTRY entry = Entry;

    return PhHashBytes(entry->Sid, PhLengthSid(entry->Sid));
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

    newEntry.Sid = PhAllocateCopy(Sid, PhLengthSid(Sid));
    newEntry.FullName = PhReferenceObject(fullName);
    PhAddEntryHashtable(PhpSidFullNameCacheHashtable, &newEntry);

    return fullName;
}

PPH_STRING PhpGetSidFullNameCached(
    _In_ PSID Sid
    )
{
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
        if (!PhIsNullOrEmptyString(processItem->FileName))
        {
            Data->IconEntry = PhImageListExtractIcon(
                processItem->FileName,
                TRUE,
                processItem->ProcessId,
                processItem->PackageFullName,
                PhProcessImageListWindowDpi
                );

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
                for (SIZE_T i = 0; i < commandLine->Length / sizeof(WCHAR); i++)
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
                processQueryFlags | PH_CLR_NO_WOW64_CHECK | (processItem->IsWow64Process ? PH_CLR_KNOWN_IS_WOW64 : 0),
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
        if (KsiLevel() >= KphLevelMed)
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
    if (processHandleLimited && WindowsVersion >= WINDOWS_8 && processItem->IsPackagedProcess)
    {
        Data->IsImmersive = !!PhIsImmersiveProcess(processHandleLimited);
    }

    // Filtered
    if (processHandleLimited && processItem->IsHandleValid)
    {
        OBJECT_BASIC_INFORMATION basicInfo;

        if (NT_SUCCESS(PhQueryObjectBasicInformation(processHandleLimited, &basicInfo)))
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

    // Process Throttling State
    {
        if (processHandleLimited && !processItem->IsSubsystemProcess)
        {
            POWER_THROTTLING_PROCESS_STATE powerThrottlingState;

            if (NT_SUCCESS(PhGetProcessPowerThrottlingState(processHandleLimited, &powerThrottlingState)))
            {
                if (FlagOn(powerThrottlingState.ControlMask, POWER_THROTTLING_PROCESS_EXECUTION_SPEED) &&
                    FlagOn(powerThrottlingState.StateMask, POWER_THROTTLING_PROCESS_EXECUTION_SPEED))
                {
                    Data->PowerThrottling = TRUE;
                }

                if (FlagOn(powerThrottlingState.ControlMask, POWER_THROTTLING_PROCESS_DELAYTIMERS) &&
                    FlagOn(powerThrottlingState.StateMask, POWER_THROTTLING_PROCESS_DELAYTIMERS))
                {
                    Data->PowerThrottling = TRUE;
                }

                if (FlagOn(powerThrottlingState.ControlMask, POWER_THROTTLING_PROCESS_IGNORE_TIMER_RESOLUTION) &&
                    FlagOn(powerThrottlingState.StateMask, POWER_THROTTLING_PROCESS_IGNORE_TIMER_RESOLUTION))
                {
                    Data->PowerThrottling = TRUE;
                }
            }
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
            case 4:
                type = PhImageCoherencySharedOriginal;
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

    if (PhEnableLinuxSubsystemSupport && processItem->FileName && processItem->IsSubsystemProcess)
    {
        PhInitializeImageVersionInfoCached(&Data->VersionInfo, processItem->FileName, TRUE, PhEnableVersionShortText);
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
    environment.IoPriority = IoPriorityLow;
    environment.PagePriority = MEMORY_PRIORITY_LOW;

    PhQueueItemWorkQueueEx(PhGetGlobalWorkQueue(), PhpProcessQueryStage1Worker, ProcessItem, NULL, &environment);
}

VOID PhpQueueProcessQueryStage2(
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PH_WORK_QUEUE_ENVIRONMENT environment;

    PhReferenceObject(ProcessItem);

    PhInitializeWorkQueueEnvironment(&environment);
    environment.BasePriority = THREAD_PRIORITY_LOWEST;
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
    processItem->IsPowerThrottling = Data->PowerThrottling;

    PhSwapReference(&processItem->Record->CommandLine, processItem->CommandLine);

    // Note: We might have referenced the cached username so don't overwrite the previous data. (dmex)
    if (!processItem->UserName)
        processItem->UserName = Data->UserName;
    else if (Data->UserName)
        PhDereferenceObject(Data->UserName);

    // Note: Queue stage 2 processing after filling stage1 process data.
    if (
        PhEnableProcessQueryStage2 ||
        PhEnableImageCoherencySupport ||
        PhEnableLinuxSubsystemSupport
        )
    {
        PhpQueueProcessQueryStage2(processItem);
    }
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
    ProcessItem->IsSystemProcess = processExtension->Classification != SystemProcessClassificationNormal;
    ProcessItem->IsSecureSystem = processExtension->Classification == SystemProcessClassificationSecureSystem;
}

VOID PhpFillProcessItem(
    _Inout_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PSYSTEM_PROCESS_INFORMATION Process
    )
{
    ProcessItem->ParentProcessId = Process->InheritedFromUniqueProcessId;
    ProcessItem->SessionId = Process->SessionId;
    ProcessItem->CreateTime = Process->CreateTime;
    ProcessItem->IntegrityLevel.Level = MAXUSHORT;

    if (ProcessItem->ProcessId != SYSTEM_IDLE_PROCESS_ID)
        ProcessItem->ProcessName = PhCreateStringFromUnicodeString(&Process->ImageName);
    else
        ProcessItem->ProcessName = PhCreateStringFromUnicodeString(&SYSTEM_IDLE_PROCESS_NAME);

    if (PH_IS_REAL_PROCESS_ID(ProcessItem->ProcessId))
    {
        PhPrintUInt32(ProcessItem->ProcessIdString, HandleToUlong(ProcessItem->ProcessId));
        PhPrintUInt32IX(ProcessItem->ProcessIdHexString, HandleToUlong(ProcessItem->ProcessId));
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

    // Process basic information
    {
        PROCESS_EXTENDED_BASIC_INFORMATION basicInfo;

        if (ProcessItem->QueryHandle &&
            NT_SUCCESS(PhGetProcessExtendedBasicInformation(ProcessItem->QueryHandle, &basicInfo)))
        {
            ProcessItem->IsProtectedProcess = basicInfo.IsProtectedProcess;
            ProcessItem->IsSecureProcess = basicInfo.IsSecureProcess;
            ProcessItem->IsSubsystemProcess = basicInfo.IsSubsystemProcess;
            ProcessItem->IsWow64Process = basicInfo.IsWow64Process;
            ProcessItem->IsPackagedProcess = basicInfo.IsStronglyNamed;
            ProcessItem->IsCrossSessionProcess = basicInfo.IsCrossSessionCreate;
            ProcessItem->IsBackgroundProcess = basicInfo.IsBackground;
            ProcessItem->AffinityMask = basicInfo.BasicInfo.AffinityMask;
        }
        else
        {
            ProcessItem->AffinityMask = PhSystemBasicInformation.ActiveProcessorsAffinityMask;
        }
    }

    // Process information
    {
        // If we're dealing with System (PID 4), we need to get the
        // kernel file name. Otherwise, get the image file name. (wj32)

        if (ProcessItem->ProcessId == SYSTEM_PROCESS_ID)
        {
            PPH_STRING fileName;

            if (fileName = PhGetKernelFileName())
            {
                ProcessItem->FileName = fileName;
                ProcessItem->FileNameWin32 = PhGetFileName(fileName);
            }
        }
        else if (ProcessItem->IsSecureSystem)
        {
            PPH_STRING fileName;

            if (fileName = PhGetSecureKernelFileName())
            {
                ProcessItem->FileName = fileName;
                ProcessItem->FileNameWin32 = PhGetFileName(fileName);
            }
        }
        else
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
                    {
                        ProcessItem->FileNameWin32 = fileName;
                    }
                }
            }
        }
    }

    // Token information
    if (ProcessItem->QueryHandle)
    {
        HANDLE tokenHandle;

        if (NT_SUCCESS(PhOpenProcessToken(
            ProcessItem->QueryHandle,
            TOKEN_QUERY,
            &tokenHandle
            )))
        {
            PH_TOKEN_USER tokenUser;
            TOKEN_ELEVATION_TYPE elevationType;
            BOOLEAN isElevated;
            BOOLEAN tokenIsUIAccessEnabled;
            PH_INTEGRITY_LEVEL integrityLevel;
            PPH_STRINGREF integrityString;

            // User
            if (NT_SUCCESS(PhGetTokenUser(tokenHandle, &tokenUser)))
            {
                ProcessItem->Sid = PhAllocateCopy(tokenUser.User.Sid, PhLengthSid(tokenUser.User.Sid));
                ProcessItem->UserName = PhpGetSidFullNameCached(tokenUser.User.Sid);
            }

            // Elevation
            if (NT_SUCCESS(PhGetTokenElevationType(tokenHandle, &elevationType)))
            {
                ProcessItem->ElevationType = elevationType;
            }

            if (NT_SUCCESS(PhGetTokenElevation(tokenHandle, &isElevated)))
            {
                ProcessItem->IsElevated = isElevated;
            }

            // Integrity
            if (NT_SUCCESS(PhGetTokenIntegrityLevelEx(tokenHandle, &integrityLevel, &integrityString)))
            {
                ProcessItem->IntegrityLevel = integrityLevel;
                ProcessItem->IntegrityString = integrityString;
            }

            // UIAccess
            if (NT_SUCCESS(PhGetTokenUIAccess(tokenHandle, &tokenIsUIAccessEnabled)))
            {
                ProcessItem->IsUIAccessEnabled = !!tokenIsUIAccessEnabled;
            }

            // Package name
            if (WindowsVersion >= WINDOWS_8 && ProcessItem->IsPackagedProcess)
            {
                ProcessItem->PackageFullName = PhGetTokenPackageFullName(tokenHandle);
            }

            NtClose(tokenHandle);
        }
    }
    // Token information
    {
        if (ProcessItem->ProcessId == SYSTEM_IDLE_PROCESS_ID ||
            ProcessItem->ProcessId == SYSTEM_PROCESS_ID)
        {
            if (!ProcessItem->Sid)
                ProcessItem->Sid = PhAllocateCopy((PSID)&PhSeLocalSystemSid, PhLengthSid((PSID)&PhSeLocalSystemSid));
            if (!ProcessItem->UserName)
                ProcessItem->UserName = PhpGetSidFullNameCached((PSID)&PhSeLocalSystemSid);

            ProcessItem->IsSystemProcess = TRUE;
        }
    }

    // Known Process Type
    if (ProcessItem->FileName)
    {
        ProcessItem->KnownProcessType = PhGetProcessKnownTypeEx(
            ProcessItem->ProcessId,
            ProcessItem->FileName
            );
    }

    // Protection
    {
        BOOLEAN haveProtection = FALSE;

        if (ProcessItem->QueryHandle)
        {
            if (WindowsVersion >= WINDOWS_8_1)
            {
                PS_PROTECTION protection;

                if (NT_SUCCESS(PhGetProcessProtection(ProcessItem->QueryHandle, &protection)))
                {
                    ProcessItem->Protection.Level = protection.Level;
                    haveProtection = TRUE;
                }
            }
        }

        if (WindowsVersion >= WINDOWS_8_1)
        {
            if (haveProtection)
                ProcessItem->ProtectionString = PhpGetProtectionString(ProcessItem->Protection, (BOOLEAN)ProcessItem->IsSecureProcess);
            else
                ProcessItem->ProtectionString = PhReferenceObject(PhpProtectionUnknownString);
        }
        else if (ProcessItem->IsProtectedProcess)
        {
            ProcessItem->ProtectionString = PhReferenceObject(PhpProtectionYesString);
        }
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

    // WSL
    if (WindowsVersion >= WINDOWS_10_22H2 && ProcessItem->QueryHandle)
    {
        if (ProcessItem->IsSubsystemProcess && KsiLevel() >= KphLevelMed)
        {
            ULONG lxssProcessId;

            if (NT_SUCCESS(KphQueryInformationProcess(
                ProcessItem->QueryHandle,
                KphProcessWSLProcessId,
                &lxssProcessId,
                sizeof(ULONG),
                NULL
                )))
            {
                ProcessItem->LxssProcessId = lxssProcessId;
            }
        }
    }

    // Extended Process Information
    if (WindowsVersion >= WINDOWS_10 && ProcessItem->QueryHandle)
    {
        PPROCESS_TELEMETRY_ID_INFORMATION telemetryInfo;
        ULONG telemetryInfoLength;

        if (NT_SUCCESS(PhGetProcessTelemetryIdInformation(ProcessItem->QueryHandle, &telemetryInfo, &telemetryInfoLength)))
        {
            SIZE_T UserSidLength = telemetryInfo->ImagePathOffset - telemetryInfo->UserSidOffset;
            PWSTR UserSidBuffer = PTR_ADD_OFFSET(telemetryInfo, telemetryInfo->UserSidOffset);
            SIZE_T ImagePathLength = telemetryInfo->PackageNameOffset - telemetryInfo->ImagePathOffset;
            PWSTR ImagePathBuffer = PTR_ADD_OFFSET(telemetryInfo, telemetryInfo->ImagePathOffset);
            SIZE_T PackageNameLength = telemetryInfo->RelativeAppNameOffset - telemetryInfo->PackageNameOffset;
            PWSTR PackageNameBuffer = PTR_ADD_OFFSET(telemetryInfo, telemetryInfo->PackageNameOffset);
            SIZE_T RelativeAppNameLength = telemetryInfo->CommandLineOffset - telemetryInfo->RelativeAppNameOffset;
            PWSTR RelativeAppNameBuffer = PTR_ADD_OFFSET(telemetryInfo, telemetryInfo->RelativeAppNameOffset);
            SIZE_T CommandLineLength = telemetryInfoLength - telemetryInfo->CommandLineOffset;
            PWSTR CommandLineBuffer = PTR_ADD_OFFSET(telemetryInfo, telemetryInfo->CommandLineOffset);

            ProcessItem->ProcessStartKey = telemetryInfo->ProcessStartKey;
            ProcessItem->CreateInterruptTime = telemetryInfo->CreateInterruptTime;
            ProcessItem->ProcessSequenceNumber = telemetryInfo->ProcessSequenceNumber;
            ProcessItem->SessionCreateTime = telemetryInfo->SessionCreateTime;
            ProcessItem->ImageChecksum = telemetryInfo->ImageChecksum;
            ProcessItem->ImageTimeStamp = telemetryInfo->ImageTimeDateStamp;

            if (!ProcessItem->Sid && RtlValidSid(UserSidBuffer))
            {
                ProcessItem->Sid = PhAllocateCopy(UserSidBuffer, UserSidLength);
            }

            if (PhIsNullOrEmptyString(ProcessItem->FileName) && ImagePathLength > sizeof(UNICODE_NULL))
            {
                ProcessItem->FileName = PhCreateStringEx(ImagePathBuffer, ImagePathLength - sizeof(UNICODE_NULL));
            }

            if (PhIsNullOrEmptyString(ProcessItem->PackageFullName) && PackageNameLength > sizeof(UNICODE_NULL))
            {
                ProcessItem->PackageFullName = PhCreateStringEx(PackageNameBuffer, PackageNameLength - sizeof(UNICODE_NULL));
            }

            //if (PhIsNullOrEmptyString(ProcessItem->CommandLine) && CommandLineLength > sizeof(UNICODE_NULL))
            //{
            //    ProcessItem->CommandLine = PhCreateString(CommandLineBuffer); // CommandLineLength - sizeof(UNICODE_NULL));
            //}
        }
    }

    // On Windows 8.1 and above, processes without threads are reflected processes
    // which will not terminate if we have a handle open. (wj32)
    if (Process->UserTime.QuadPart + Process->KernelTime.QuadPart == 0 && Process->NumberOfThreads == 0 && ProcessItem->QueryHandle)
    {
        ProcessItem->IsReflectedProcess = TRUE;

        NtClose(ProcessItem->QueryHandle);
        ProcessItem->QueryHandle = NULL;
    }
}

VOID PhpUpdateDynamicInfoProcessItem(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PSYSTEM_PROCESS_INFORMATION Process
    )
{
    ProcessItem->BasePriority = Process->BasePriority;

    if (ProcessItem->QueryHandle)
    {
        UCHAR priorityClass;

        if (NT_SUCCESS(PhGetProcessPriorityClass(ProcessItem->QueryHandle, &priorityClass)))
        {
            ProcessItem->PriorityClass = priorityClass;
        }

        if (WindowsVersion >= WINDOWS_11_24H2)
        {
            PROCESS_NETWORK_COUNTERS networkCounters;

            if (NT_SUCCESS(PhGetProcesNetworkIoCounters(ProcessItem->QueryHandle, &networkCounters)))
            {
                ProcessItem->NetworkCounters = networkCounters;
            }
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
                PTR_ADD_OFFSET(PhCpuInformation, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * processorCount),
                sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * activeProcessorCount,
                NULL
                )))
            {
                memset(
                    PTR_ADD_OFFSET(PhCpuInformation, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * processorCount),
                    0,
                    sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * activeProcessorCount
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
            sizeof(SYSTEM_PROCESSOR_IDLE_CYCLE_TIME_INFORMATION) * PhSystemProcessorInformation.NumberOfProcessors,
            NULL
            )))
        {
            memset(
                PhCpuIdleCycleTime,
                0,
                sizeof(SYSTEM_PROCESSOR_IDLE_CYCLE_TIME_INFORMATION) * PhSystemProcessorInformation.NumberOfProcessors
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
                PTR_ADD_OFFSET(PhCpuIdleCycleTime, sizeof(SYSTEM_PROCESSOR_IDLE_CYCLE_TIME_INFORMATION) * processorCount),
                sizeof(SYSTEM_PROCESSOR_IDLE_CYCLE_TIME_INFORMATION) * activeProcessorCount,
                NULL
                )))
            {
                memset(
                    PTR_ADD_OFFSET(PhCpuIdleCycleTime, sizeof(SYSTEM_PROCESSOR_IDLE_CYCLE_TIME_INFORMATION) * processorCount),
                    0,
                    sizeof(SYSTEM_PROCESSOR_IDLE_CYCLE_TIME_INFORMATION) * activeProcessorCount
                    );
            }

            processorCount += activeProcessorCount;
        }
    }

    total = 0;

    for (i = 0; i < PhSystemProcessorInformation.NumberOfProcessors; i++)
    {
        //PhUpdateDelta(&PhCpusIdleCycleDelta[i], PhCpuIdleCycleTime[i].CycleTime);
        total += PhCpuIdleCycleTime[i].CycleTime;
    }

    PhUpdateDelta(&PhCpuIdleCycleDelta, total);
    *IdleCycleTime = PhCpuIdleCycleDelta.Delta;

    // System

    if (PhSystemProcessorInformation.SingleProcessorGroup)
    {
        if (!NT_SUCCESS(NtQuerySystemInformation(
            SystemProcessorCycleTimeInformation,
            PhCpuSystemCycleTime,
            sizeof(SYSTEM_PROCESSOR_CYCLE_TIME_INFORMATION) * PhSystemProcessorInformation.NumberOfProcessors,
            NULL
            )))
        {
            memset(
                PhCpuSystemCycleTime,
                0,
                sizeof(SYSTEM_PROCESSOR_CYCLE_TIME_INFORMATION) * PhSystemProcessorInformation.NumberOfProcessors
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
                PTR_ADD_OFFSET(PhCpuSystemCycleTime, sizeof(SYSTEM_PROCESSOR_CYCLE_TIME_INFORMATION) * processorCount),
                sizeof(SYSTEM_PROCESSOR_CYCLE_TIME_INFORMATION) * activeProcessorCount,
                NULL
                )))
            {
                memset(
                    PTR_ADD_OFFSET(PhCpuSystemCycleTime, sizeof(SYSTEM_PROCESSOR_CYCLE_TIME_INFORMATION) * processorCount),
                    0,
                    sizeof(SYSTEM_PROCESSOR_CYCLE_TIME_INFORMATION) * activeProcessorCount
                    );
            }

            processorCount += activeProcessorCount;
        }
    }

    total = 0;

    for (i = 0; i < PhSystemProcessorInformation.NumberOfProcessors; i++)
    {
        total += PhCpuSystemCycleTime[i].CycleTime;
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
    ULONG64 totalTimeDelta;
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
    totalTimeDelta = PhCpuKernelDelta.Delta + PhCpuUserDelta.Delta;

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
    PhTimeToSecondsSince1980(&systemTime, &secondsSince1980);
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
_Success_(return)
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
    PhSecondsSince1980ToTime(secondsSince1980, &time);

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

    //if (!RtlFirstEntrySList(&PhProcessQueryDataListHead))
    //    return;

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

        InterlockedExchange(&data->ProcessItem->JustProcessed, TRUE);

        PhDereferenceObject(data->ProcessItem);
        PhFree(data);
    }
}

VOID PhpGetProcessThreadInformation(
    _In_ PPH_PROCESS_ITEM ProcessItem,
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
    ULONG suspendedCount;
    ULONG workqueueCount;

    isSuspended = FALSE;
    isPartiallySuspended = FALSE;
    contextSwitches = 0;
    processorQueueLength = 0;
    suspendedCount = 0;
    workqueueCount = 0;

    for (i = 0; i < Process->NumberOfThreads; i++)
    {
        PSYSTEM_THREAD_INFORMATION thread = &Process->Threads[i];

        switch (thread->ThreadState)
        {
        case Ready:
            processorQueueLength++;
            break;
        case Waiting:
            {
                switch (thread->WaitReason)
                {
                case Suspended:
                    suspendedCount++;
                    break;
                case WrQueue:
                    workqueueCount++;
                    break;
                }
            }
            break;
        }

        contextSwitches += thread->ContextSwitches;
    }

    if (PH_IS_REAL_PROCESS_ID(Process->UniqueProcessId) && !ProcessItem->IsSystemProcess)
    {
        if (suspendedCount)
        {
            if (
                suspendedCount == Process->NumberOfThreads ||
                (suspendedCount + workqueueCount) == Process->NumberOfThreads  // (NumberOfThreads - workQueueLength)
                )
            {
                isSuspended = TRUE;
            }
            else
            {
                isPartiallySuspended = TRUE;
            }
        }
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

#ifdef _ARM64_
VOID PhpEstimateIdleCyclesForARM(
    _Inout_ PULONG64 TotalCycles,
    _Inout_ PULONG64 IdleCycles
    )
{
    // EXPERIMENTAL (jxy-s)
    //
    // Update (2024-09-29) - 24H2 is now estimating the cycle counts for idle threads in the kernel
    // making this routine obsolete. However, this means that the idle thread cycle counts returned
    // from the kernel is not a reflection of the actual CPU effort of the idle threads. In other
    // words the idle threads now represent the percentage of the CPU *not* being used by other
    // processes, as it does on other architectures. The kernel has also broadly changed the cycle
    // accounting across the entire system to no longer use PMCCNTR_EL0 and instead it uses
    // KeQueryPerformanceCounter. This means it is currently impossible to represent the cycle
    // accounting in a way best suited for the ARM architecture. The kernel appears to have opted
    // for more consistency between architectures instead of accuracy for ARM.
    //
    assert(WindowsVersion < WINDOWS_11_24H2);

    //
    // The kernel uses PMCCNTR_EL0 for CycleTime in threads and the processor control blocks.
    // Here is a snippet from ntoskrnl!KiIdleLoop:
    /*
00000001`404f45a0 d53b9d0b mrs         x11,PMCCNTR_EL0    // x11 gets current cycle count
00000001`404f45a4 f947a668 ldr         x8,[x19,#0xF48]    // x8 gets _KPRCB.StartCycles
00000001`404f45a8 cb08016a sub         x10,x11,x8         // x10 gets relative cycles from StartCycle to current cycle count
00000001`404f45ac f94026a9 ldr         x9,[x21,#0x48]     // x9 gets _KPRCB.IdleThread.CycleTime
00000001`404f45b0 8b0a0128 add         x8,x9,x10          // x8 gets idle thread cycle time (x9) plus cycle increment (x10)
00000001`404f45b4 f90026a8 str         x8,[x21,#0x48]     // stores new idle thread cycle time in _KPRCB.IdleThread.CycleTime
00000001`404f45b8 f907a66b str         x11,[x19,#0xF48]   // stores the last cycle time fetch in the _KPRCB.StartCycles
    */
    // The idle logic in the HAL is here:
    /*
ntoskrnl!HalProcessorIdle:
00000001`4048fbf0 d503237f pacibsp
00000001`4048fbf4 a9bf7bfd stp         fp,lr,[sp,#-0x10]!
00000001`4048fbf8 910003fd mov         fp,sp
00000001`4048fbfc 940013df bl          ntoskrnl!HalpTimerResetProfileAdjustment (00000001`40494b78)
00000001`4048fc00 d5033f9f dsb         sy
00000001`4048fc04 d503207f wfi
00000001`4048fc08 a8c17bfd ldp         fp,lr,[sp],#0x10
00000001`4048fc0c d50323ff autibsp
00000001`4048fc10 d65f03c0 ret
    */
    // This dictates that the CPU should enter WFI (Wait For Interrupt) mode. In this mode the
    // CPU clock is disabled and the PMCCNTR register is not being updated, from the docs:
    /*
All counters are subject to any changes in clock frequency, including clock stopping caused by
the WFI and WFE instructions. This means that it is CONSTRAINED UNPREDICTABLE regardless of whether
PMCCNTR_EL0 continues to increment when clocks are stopped by WFI and WFE instructions.
    */
    // Arguably, the kernel is doing the right thing here, the idle threads are taking less cycle
    // time and the KTHREAD reflects this accurately. However, due to the way cycle-based CPU
    // usage is implemented for other architectures it assumes on the idle thread cycles are
    // the cycles *not* spent using the CPU, this assumption is incorrect for ARM.
    //
    // The most accurate representation of the utilization of the CPU for ARM would show
    // the idle process CPU usage *below* what is "not being used" by other processes. Since
    // this would indicate the idle thread being in the WFI state and not using CPU cycles.
    // For cycle-based CPU to make sense for ARM, we need a way to get or estimate the amount
    // of cycles the CPU would have used by the idle threads if WFI was mode was not entered.
    //
    // This experimental estimate achieves that. It will show the idle process utilization
    // accurately (given the estimate and what the kernel tracks/reports), and generally retain
    // the cycle-based CPU benefits. Arguably, we are in some capacity reverting to the
    // time-based CPU usage calculation by relying on the time to generate the estimate, but
    // again it comes with the benefit of more realistically reporting the idle threads given
    // what is tracked/reported by the kernel.
    //
    // let x = Unknown Idle Cycles
    // let y = "Total" Cycles (missing idle cycles)
    // let a = Time Idle
    // let b = Time Kernel
    // let c = Time User
    //
    // x / (y + x) = CycleUsage
    // a / (a + b + c) = TimeUsage
    //
    // Assume: CycleUsage ~= TimeUsage
    // Then:  x / (y + x) ~= a / (a + b + c)
    //
    // We know y, a, b, and c. So we need to solve for x.
    //
    // x / (y + x) ~= a / (a + b + c)
    //
    // x ~= a * (y + x) / (a + b + c)
    // x * (a + b + c) ~= a * (y + x)
    // x * (a + b + c) ~= (a * y) + (a * x)
    // x * (a + b + c) - (a * x) ~= (a * y)
    // x * ((a + b + c) - a) ~= (a * y)
    // x ~= (a * y) / ((a + b + c) - a)
    //
    // x ~= (a * y) / (b + c)
    //
    ULONG64 delta = PhCpuKernelDelta.Delta + PhCpuUserDelta.Delta;
    if (delta != 0)
        *IdleCycles = (PhCpuIdleCycleDelta.Delta * *TotalCycles) / delta;
    else
        *IdleCycles = (PhCpuIdleCycleDelta.Delta * *TotalCycles);

    // We still need to add in the idle cycle time to the "total" since it wasn't complete yet.
    *TotalCycles += *IdleCycles;
}
#endif

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

        PhFlushVerifyCache();
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

#ifdef _ARM64_ // see: PhpEstimateIdleCyclesForARM (jxy-s)
        if (PhEnableCycleCpuUsage && (WindowsVersion >= WINDOWS_11_24H2 || process->UniqueProcessId != SYSTEM_IDLE_PROCESS_ID))
#else
        if (PhEnableCycleCpuUsage)
#endif
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

                    ASSUME_ASSERT(processesToRemove);
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

#ifdef _ARM64_
    if (PhEnableCycleCpuUsage && WindowsVersion < WINDOWS_11_24H2)
        PhpEstimateIdleCyclesForARM(&sysTotalCycleTime, &sysIdleCycleTime);
#endif

    // Go through the queued process query data.
    PhFlushProcessQueryData();

    if (sysTotalTime == 0)
        sysTotalTime = ULONG64_MAX; // max. value
    if (sysTotalCycleTime == 0)
        sysTotalCycleTime = ULONG64_MAX;

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
            PhpFillProcessItemExtension(processItem, process);
            PhpFillProcessItem(processItem, process);
            processItem->TimeSequenceNumber = PhTimeSequenceNumber;

            processRecord = PhpCreateProcessRecord(processItem);
            PhpAddProcessRecord(processRecord);
            processItem->Record = processRecord;

            PhpUpdateDynamicInfoProcessItem(processItem, process);
            PhpGetProcessThreadInformation(processItem, process, &isSuspended, &isPartiallySuspended, &contextSwitches, &processorQueueLength);
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
            ULONG processorQueueLength;
            FLOAT newCpuUsage;
            FLOAT kernelCpuUsage;
            FLOAT userCpuUsage;

            PhpUpdateDynamicInfoProcessItem(processItem, process);
            PhpGetProcessThreadInformation(processItem, process, &isSuspended, &isPartiallySuspended, &contextSwitches, &processorQueueLength);
            PhpFillProcessItemExtension(processItem, process);
            PhTotalCpuQueueLength += processorQueueLength;

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
                ULONG64 totalDelta;

                newCpuUsage = (FLOAT)processItem->CycleTimeDelta.Delta / sysTotalCycleTime;

                // Calculate the kernel/user CPU usage based on the kernel/user time. If the kernel
                // and user deltas are both zero, we'll just have to use an estimate. Currently, we
                // split the CPU usage evenly across the kernel and user components, except when the
                // total user time is zero, in which case we assign it all to the kernel component.

                totalDelta = processItem->CpuKernelDelta.Delta + processItem->CpuUserDelta.Delta;

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

            // Update the process affinity. Usually not frequently changed, do so lazily.
            if (runCount % 5 == 0)
            {
                KAFFINITY oldAffinityMask;
                KAFFINITY affinityMask;

                oldAffinityMask = processItem->AffinityMask;

                if (processItem->QueryHandle &&
                    NT_SUCCESS(PhGetProcessAffinityMask(processItem->QueryHandle, &affinityMask)))
                {
                    processItem->AffinityMask = affinityMask;
                }
                else
                {
                    processItem->AffinityMask = PhSystemBasicInformation.ActiveProcessorsAffinityMask;
                }

                if (processItem->AffinityMask != oldAffinityMask)
                {
                    modified = TRUE;
                }
            }

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
                    processItem->CpuAverageUsage = (FLOAT)value / (FLOAT)processItem->CpuKernelHistory.Count;
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
                    PH_TOKEN_USER tokenUser;
                    //BOOLEAN isElevated;
                    //TOKEN_ELEVATION_TYPE elevationType;
                    PH_INTEGRITY_LEVEL integrityLevel;
                    PPH_STRINGREF integrityString;

                    if (FlagOn(PhProcessProviderFlagsMask, PH_PROCESS_PROVIDER_FLAG_USERNAME))
                    {
                        // User
                        if (NT_SUCCESS(PhGetTokenUser(tokenHandle, &tokenUser)))
                        {
                            if (!processItem->Sid || !PhEqualSid(processItem->Sid, tokenUser.User.Sid))
                            {
                                PSID processSid;

                                // HACK (dmex)
                                processSid = processItem->Sid;
                                processItem->Sid = PhAllocateCopy(tokenUser.User.Sid, PhLengthSid(tokenUser.User.Sid));
                                if (processSid) PhFree(processSid);

                                if (processItem->Sid)
                                {
                                    PhMoveReference(&processItem->UserName, PhpGetSidFullNameCachedSlow(processItem->Sid));
                                }

                                modified = TRUE;
                            }
                        }
                    }

                    // Elevation

                    //if (NT_SUCCESS(PhGetTokenElevation(tokenHandle, &isElevated)))
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
                    if (NT_SUCCESS(PhGetTokenIntegrityLevelEx(tokenHandle, &integrityLevel, &integrityString)))
                    {
                        if (processItem->IntegrityLevel.Level != integrityLevel.Level)
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
                            PhMoveReference(&processItem->UserName, PhpGetSidFullNameCachedSlow((PSID)&PhSeLocalSystemSid));
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

                if (KsiLevel() >= KphLevelMed)
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

            // Process Throttling State
            {
                BOOLEAN isPowerThrottling = FALSE;

                if (processItem->QueryHandle && !processItem->IsSubsystemProcess)
                {
                    POWER_THROTTLING_PROCESS_STATE powerThrottlingState;

                    if (NT_SUCCESS(PhGetProcessPowerThrottlingState(processItem->QueryHandle, &powerThrottlingState)))
                    {
                        if (FlagOn(powerThrottlingState.ControlMask, POWER_THROTTLING_PROCESS_EXECUTION_SPEED) &&
                            FlagOn(powerThrottlingState.StateMask, POWER_THROTTLING_PROCESS_EXECUTION_SPEED))
                        {
                            isPowerThrottling = TRUE;
                        }

                        if (FlagOn(powerThrottlingState.ControlMask, POWER_THROTTLING_PROCESS_DELAYTIMERS) &&
                            FlagOn(powerThrottlingState.StateMask, POWER_THROTTLING_PROCESS_DELAYTIMERS))
                        {
                            isPowerThrottling = TRUE;
                        }

                        if (FlagOn(powerThrottlingState.ControlMask, POWER_THROTTLING_PROCESS_IGNORE_TIMER_RESOLUTION) &&
                            FlagOn(powerThrottlingState.StateMask, POWER_THROTTLING_PROCESS_IGNORE_TIMER_RESOLUTION))
                        {
                            isPowerThrottling = TRUE;
                        }
                    }
                }

                if (processItem->IsPowerThrottling != isPowerThrottling)
                {
                    processItem->IsPowerThrottling = isPowerThrottling;
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

                if (NT_SUCCESS(PhQueryObjectBasicInformation(processItem->QueryHandle, &basicInfo)))
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

    PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent), PH_PROCESS_PROVIDER_UPDATED_EVENT_PTR(runCount));
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
        if (InsertIndex)
            *InsertIndex = 0;

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
    _In_ HWND WindowHandle,
    _In_ LONG WindowDpi
    )
{
    HICON iconSmall;
    HICON iconLarge;
    PPH_LIST fileNames = NULL;

    PhAcquireQueuedLockExclusive(&PhImageListCacheHashtableLock);

    if (!PhImageListItemType)
        PhImageListItemType = PhCreateObjectType(L"ImageListItem", 0, PhpImageListItemDeleteProcedure);

    PhProcessImageListWindowDpi = WindowDpi;

    {
        PH_HASHTABLE_ENUM_CONTEXT enumContext;
        PPH_IMAGELIST_ITEM* entry;

        if (PhImageListCacheHashtable)
        {
            fileNames = PhCreateList(PhImageListCacheHashtable->Count);

            PhBeginEnumHashtable(PhImageListCacheHashtable, &enumContext);

            while (entry = PhNextEnumHashtable(&enumContext))
            {
                PPH_IMAGELIST_ITEM item = *entry;
                PhAddItemList(fileNames, PhReferenceObject(item->FileName));
                PhDereferenceObject(item);
            }

            PhDereferenceObject(PhImageListCacheHashtable);
            PhImageListCacheHashtable = NULL;
        }
    }

    if (PhProcessLargeImageList)
    {
        PhImageListSetIconSize(
            PhProcessLargeImageList,
            PhGetSystemMetrics(SM_CXICON, PhProcessImageListWindowDpi),
            PhGetSystemMetrics(SM_CYICON, PhProcessImageListWindowDpi)
            );
    }
    else
    {
        PhProcessLargeImageList = PhImageListCreate(
            PhGetSystemMetrics(SM_CXICON, PhProcessImageListWindowDpi),
            PhGetSystemMetrics(SM_CYICON, PhProcessImageListWindowDpi),
            ILC_MASK | ILC_COLOR32,
            100,
            100);
    }

    if (PhProcessSmallImageList)
    {
        PhImageListSetIconSize(
            PhProcessSmallImageList,
            PhGetSystemMetrics(SM_CXSMICON, PhProcessImageListWindowDpi),
            PhGetSystemMetrics(SM_CYSMICON, PhProcessImageListWindowDpi)
            );
    }
    else
    {
        PhProcessSmallImageList = PhImageListCreate(
            PhGetSystemMetrics(SM_CXSMICON, PhProcessImageListWindowDpi),
            PhGetSystemMetrics(SM_CYSMICON, PhProcessImageListWindowDpi),
            ILC_MASK | ILC_COLOR32,
            100,
            100);
    }

    //PhImageListSetBkColor(PhProcessLargeImageList, CLR_NONE);
    //PhImageListSetBkColor(PhProcessSmallImageList, CLR_NONE);

    PhGetStockApplicationIcon(&iconSmall, &iconLarge);
    PhImageListAddIcon(PhProcessLargeImageList, iconLarge);
    PhImageListAddIcon(PhProcessSmallImageList, iconSmall);

    iconLarge = PhLoadIcon(PhInstanceHandle, MAKEINTRESOURCE(IDI_COG), PH_LOAD_ICON_SIZE_LARGE, 0, 0, PhProcessImageListWindowDpi);
    iconSmall = PhLoadIcon(PhInstanceHandle, MAKEINTRESOURCE(IDI_COG), PH_LOAD_ICON_SIZE_SMALL, 0, 0, PhProcessImageListWindowDpi);
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

            PhReleaseQueuedLockExclusive(&PhImageListCacheHashtableLock);

            iconEntry = PhImageListExtractIcon(filename, TRUE, 0, NULL, PhProcessImageListWindowDpi);

            PhAcquireQueuedLockExclusive(&PhImageListCacheHashtableLock);

            if (iconEntry)
            {
                PPH_PROCESS_ITEM* processes;
                ULONG numberOfProcesses;

                PhEnumProcessItems(&processes, &numberOfProcesses);

                for (ULONG j = 0; j < numberOfProcesses; j++)
                {
                    PPH_PROCESS_ITEM process = processes[j];

                    if (process->FileName && PhEqualString(process->FileName, filename, FALSE))
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

    PhReleaseQueuedLockExclusive(&PhImageListCacheHashtableLock);
}

BOOLEAN PhImageListCacheHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_IMAGELIST_ITEM entry1 = *(PPH_IMAGELIST_ITEM*)Entry1;
    PPH_IMAGELIST_ITEM entry2 = *(PPH_IMAGELIST_ITEM*)Entry2;

    return PhEqualStringRef(&entry1->FileName->sr, &entry2->FileName->sr, FALSE);
}

ULONG PhImageListCacheHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PPH_IMAGELIST_ITEM entry = *(PPH_IMAGELIST_ITEM*)Entry;

    return PhHashStringRefEx(&entry->FileName->sr, FALSE, PH_STRING_HASH_X65599);
}

PPH_IMAGELIST_ITEM PhImageListExtractIcon(
    _In_ PPH_STRING FileName,
    _In_ BOOLEAN NativeFileName,
    _In_opt_ HANDLE ProcessId,
    _In_opt_ PPH_STRING PackageFullName,
    _In_ LONG SystemDpi
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

    if (PhEnablePackageIconSupport && ProcessId && PackageFullName)
    {
        if (!PhAppResolverGetPackageIcon(
            ProcessId,
            PackageFullName,
            &largeIcon,
            &smallIcon,
            SystemDpi
            ))
        {
            PhExtractIconEx(
                &FileName->sr,
                NativeFileName,
                0,
                &largeIcon,
                &smallIcon,
                SystemDpi
                );
        }
    }
    else
    {
        PhExtractIconEx(
            &FileName->sr,
            NativeFileName,
            0,
            &largeIcon,
            &smallIcon,
            SystemDpi
            );
    }

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

    newentry = PhCreateObject(sizeof(PH_IMAGELIST_ITEM), PhImageListItemType);
    newentry->FileName = PhReferenceObject(FileName);

    if (largeIcon && smallIcon)
    {
        newentry->LargeIconIndex = PhImageListAddIcon(PhProcessLargeImageList, largeIcon);
        newentry->SmallIconIndex = PhImageListAddIcon(PhProcessSmallImageList, smallIcon);
    }
    else
    {
        newentry->LargeIconIndex = 0;
        newentry->SmallIconIndex = 0;
    }

    PhReferenceObject(newentry);
    PhAddEntryHashtable(PhImageListCacheHashtable, &newentry);

    PhReleaseQueuedLockExclusive(&PhImageListCacheHashtableLock);

    if (smallIcon)
        DestroyIcon(smallIcon);
    if (largeIcon)
        DestroyIcon(smallIcon);

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

_Success_(return)
BOOLEAN PhDuplicateProcessInformation(
    _Outptr_ PPVOID ProcessInformation
    )
{
    SIZE_T infoLength;

    if (!PhProcessInformation)
        return FALSE;

    infoLength = PhSizeHeap(PhProcessInformation);

    if (!infoLength)
        return FALSE;

    *ProcessInformation = PhAllocateCopy(PhProcessInformation, infoLength);

    return TRUE;
}
