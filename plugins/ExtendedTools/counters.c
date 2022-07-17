/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2019-2022
 *
 */

#include "exttools.h"
#include <perflib.h>

typedef struct _ET_GPU_ENGINE_COUNTER
{
    ULONG ProcessId;
    ULONG EngineId;
    ULONG EngineLuid;

    ULONG64 GpuTime;
    union
    {
        ULONG64 Value64;
        FLOAT ValueF;
    };
} ET_GPU_ENGINE_COUNTER, *PET_GPU_ENGINE_COUNTER;

typedef struct _ET_GPU_ADAPTER_COUNTER
{
    ULONG EngineLuid;

    //ULONGLONG SharedUsage;
    ULONGLONG DedicatedUsage;
    //ULONGLONG CommitUsage;
} ET_GPU_ADAPTER_COUNTER, *PET_GPU_ADAPTER_COUNTER;

typedef struct _ET_GPU_PROCESS_COUNTER
{
    ULONG ProcessId;
    ULONG EngineId;
    ULONG EngineLuid;

    ULONGLONG SharedUsage;
    ULONGLONG DedicatedUsage;
    ULONGLONG CommitUsage;
} ET_GPU_PROCESS_COUNTER, *PET_GPU_PROCESS_COUNTER;

//{978C167D-4764-4D9C-9824-14747351DC81} - "GPU Engine"
// [1] Running Time
// [2] Utilization Percentage
DEFINE_GUID(GUID_GPU_ENGINE, 0x978C167D, 0x4764, 0x4D9C, 0x98, 0x24, 0x14, 0x74, 0x73, 0x51, 0xDC, 0x81);

//{BE2139C7-AB81-424D-B107-D87F7C9322AC} - "GPU Adapter Memory"
// [1] Total Committed
// [2] Dedicated Usage
// [3] Shared Usage
DEFINE_GUID(GUID_GPU_ADAPTERMEMORY, 0xBE2139C7, 0xAB81, 0x424D, 0xB1, 0x07, 0xD8, 0x7F, 0x7C, 0x93, 0x22, 0xAC);
#define ET_GPU_ADAPTERMEMORY_TOTALCOMMITTED_INDEX 1
#define ET_GPU_ADAPTERMEMORY_DEDICATEDUSAGE_INDEX 2
#define ET_GPU_ADAPTERMEMORY_SHAREDUSAGE_INDEX 3

//{F802502B-77B4-4713-81B3-3BE05759DA5D} - "GPU Process Memory"
// [1] Total Committed
// [2] Local Usage
// [3] Non Local Usage
// [4] Dedicated Usage
// [5] Shared Usage
DEFINE_GUID(GUID_GPU_PROCESSMEMORY, 0xF802502B, 0x77B4, 0x4713, 0x81, 0xB3, 0x3B, 0xE0, 0x57, 0x59, 0xDA, 0x5D);
#define ET_GPU_PROCESSMEMORY_TOTALCOMMITTED_INDEX 1
#define ET_GPU_PROCESSMEMORY_DEDICATEDUSAGE_INDEX 4
#define ET_GPU_PROCESSMEMORY_SHAREDUSAGE_INDEX 5

//{F9ED01F5-8F3E-4956-973F-9F05BC96F489} - "GPU Non Local Adapter Memory"
//{227419D5-F6D8-4FB7-85D6-2CAC1725E4A9} - "GPU Local Adapter Memory"
#define ET_GPU_PROCESSMEMORY_COUNTER_INDEX 0
#define ET_GPU_ADAPTERMEMORY_COUNTER_INDEX 1
#define ET_GPU_ENGINE_COUNTER_INDEX 2

typedef struct _ET_GPU_COUNTER_DATA
{
    ULONG DataSize; // Size of the counter data, in bytes.
    ULONG Size; // sizeof(PERF_COUNTER_DATA) + sizeof(Data) + sizeof(Padding)
    ULONGLONG Value;
} ET_GPU_COUNTER_DATA, *PET_GPU_COUNTER_DATA;

typedef struct _ET_GPU_ENGINE_PERFCOUNTER
{
    ULONG InstanceId;
    ULONG ProcessId;
    ULONG EngineId;
    ULONG EngineLuid;

    ULONGLONG InstanceValue;
    ULONGLONG InstanceTime;
    DOUBLE CounterValue;
} ET_GPU_ENGINE_PERFCOUNTER, *PET_GPU_ENGINE_PERFCOUNTER;

typedef struct _ET_GPU_PROCESS_PERFCOUNTER
{
    ULONG InstanceId;
    ULONG ProcessId;
    ULONG EngineLuid;

    ULONGLONG SharedUsage;
    ULONGLONG DedicatedUsage;
    ULONGLONG CommitUsage;
} ET_GPU_PROCESS_PERFCOUNTER, *PET_GPU_PROCESS_PERFCOUNTER;

typedef struct _ET_GPU_ADAPTER_PERFCOUNTER
{
    ULONG InstanceId;
    ULONG EngineLuid;
    //ULONGLONG SharedUsage;
    ULONGLONG DedicatedUsage;
    //ULONGLONG CommitUsage;
} ET_GPU_ADAPTER_PERFCOUNTER, *PET_GPU_ADAPTER_PERFCOUNTER;

typedef struct _ET_GPU_ENGINE_PERF_COUNTER
{
    ULONG InstanceId;
    PWSTR InstanceName;
    ULONGLONG InstanceTime;
    ULONGLONG InstanceValue;
} ET_GPU_ENGINE_PERF_COUNTER, *PET_GPU_ENGINE_PERF_COUNTER;

typedef struct _ET_GPU_PROCESSMEMORY_PERF_COUNTER
{
    ULONG InstanceId;
    PWSTR InstanceName;
    ULONGLONG SharedUsage;
    ULONGLONG DedicatedUsage;
    ULONGLONG CommitUsage;
} ET_GPU_PROCESSMEMORY_PERF_COUNTER, *PET_GPU_PROCESSMEMORY_PERF_COUNTER;

typedef struct _ET_GPU_ADAPTER_PERF_COUNTER
{
    ULONG InstanceId;
    PWSTR InstanceName;
    //ULONGLONG SharedUsage;
    ULONGLONG DedicatedUsage;
    //ULONGLONG CommitUsage;
} ET_GPU_ADAPTER_PERF_COUNTER, *PET_GPU_ADAPTER_PERF_COUNTER;

PPH_HASHTABLE EtPerfCounterEngineInstanceHashTable = NULL;
PPH_HASHTABLE EtPerfCounterProcessInstanceHashTable = NULL;
PPH_HASHTABLE EtPerfCounterAdapterInstanceHashTable = NULL;
PPH_HASHTABLE EtGpuRunningTimeHashTable = NULL;
PPH_HASHTABLE EtGpuProcessCounterHashTable = NULL;
PPH_HASHTABLE EtGpuAdapterDedicatedHashTable = NULL;

static BOOLEAN NTAPI EtpRunningTimeEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PET_GPU_ENGINE_COUNTER entry1 = Entry1;
    PET_GPU_ENGINE_COUNTER entry2 = Entry2;

    return entry1->ProcessId == entry2->ProcessId && entry1->EngineId == entry2->EngineId && entry1->EngineLuid == entry2->EngineLuid;
}

static ULONG NTAPI EtpEtpRunningTimeHashFunction(
    _In_ PVOID Entry
    )
{
    PET_GPU_ENGINE_COUNTER entry = Entry;

    return (entry->ProcessId / 4) ^ entry->EngineId ^ entry->EngineLuid;
}

static BOOLEAN NTAPI EtpGpuProcessEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PET_GPU_PROCESS_COUNTER entry1 = Entry1;
    PET_GPU_PROCESS_COUNTER entry2 = Entry2;

    return entry1->ProcessId == entry2->ProcessId && entry1->EngineLuid == entry2->EngineLuid;
}

static ULONG NTAPI EtpGpuProcessHashFunction(
    _In_ PVOID Entry
    )
{
    PET_GPU_PROCESS_COUNTER entry = Entry;

    return (entry->ProcessId / 4) ^ entry->EngineLuid;
}

static ULONG NTAPI EtAdapterDedicatedHashFunction(
    _In_ PVOID Entry
    )
{
    PET_GPU_ADAPTER_COUNTER entry = Entry;

    return entry->EngineLuid;
}

static BOOLEAN NTAPI EtAdapterDedicatedEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PET_GPU_ADAPTER_COUNTER entry1 = Entry1;
    PET_GPU_ADAPTER_COUNTER entry2 = Entry2;

    return entry1->EngineLuid == entry2->EngineLuid;
}

VOID EtGpuCreatePerfCounterHashTables(
    VOID
    )
{
    EtGpuRunningTimeHashTable = PhCreateHashtable(
        sizeof(ET_GPU_ENGINE_COUNTER),
        EtpRunningTimeEqualFunction,
        EtpEtpRunningTimeHashFunction,
        10
        );

    EtGpuProcessCounterHashTable = PhCreateHashtable(
        sizeof(ET_GPU_PROCESS_COUNTER),
        EtpGpuProcessEqualFunction,
        EtpGpuProcessHashFunction,
        10
        );

    EtGpuAdapterDedicatedHashTable = PhCreateHashtable(
        sizeof(ET_GPU_ADAPTER_COUNTER),
        EtAdapterDedicatedEqualFunction,
        EtAdapterDedicatedHashFunction,
        2
        );
}

VOID EtGpuDeletePerfCounterHashTables(
    VOID
    )
{
    PhDereferenceObject(EtGpuRunningTimeHashTable);
    PhDereferenceObject(EtGpuProcessCounterHashTable);
    PhDereferenceObject(EtGpuAdapterDedicatedHashTable);
}

BOOLEAN NTAPI EtPerfCounterInstanceEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2)
{
    PET_GPU_ENGINE_PERFCOUNTER entry1 = Entry1;
    PET_GPU_ENGINE_PERFCOUNTER entry2 = Entry2;

    return entry1->InstanceId == entry2->InstanceId;
}

ULONG NTAPI EtPerfCounterInstanceHashFunction(
    _In_ PVOID Entry)
{
    PET_GPU_ENGINE_PERFCOUNTER entry = Entry;

    return entry->InstanceId;
}

BOOLEAN NTAPI EtGpuPerfCounterProcessEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2)
{
    PET_GPU_PROCESS_PERFCOUNTER entry1 = Entry1;
    PET_GPU_ADAPTER_PERFCOUNTER entry2 = Entry2;

    return entry1->InstanceId == entry2->InstanceId;
}

ULONG NTAPI EtGpuPerfCounterProcessHashFunction(
    _In_ PVOID Entry)
{
    PET_GPU_ADAPTER_PERFCOUNTER entry = Entry;

    return entry->InstanceId;
}

BOOLEAN NTAPI EtGpuPerfCounterAdapterEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2)
{
    PET_GPU_PROCESS_PERFCOUNTER entry1 = Entry1;
    PET_GPU_PROCESS_PERFCOUNTER entry2 = Entry2;

    return entry1->InstanceId == entry2->InstanceId;
}

ULONG NTAPI EtGpuPerfCounterAdapterHashFunction(
    _In_ PVOID Entry)
{
    PET_GPU_PROCESS_PERFCOUNTER entry = Entry;

    return entry->InstanceId;
}

VOID EtGpuCreatePerfCounterHashTable(
    VOID)
{
    EtPerfCounterEngineInstanceHashTable = PhCreateHashtable(
        sizeof(ET_GPU_ENGINE_PERFCOUNTER),
        EtPerfCounterInstanceEqualFunction,
        EtPerfCounterInstanceHashFunction,
        64
        );

    EtPerfCounterProcessInstanceHashTable = PhCreateHashtable(
        sizeof(ET_GPU_PROCESS_PERFCOUNTER),
        EtGpuPerfCounterProcessEqualFunction,
        EtGpuPerfCounterProcessHashFunction,
        64
        );

    EtPerfCounterAdapterInstanceHashTable = PhCreateHashtable(
        sizeof(ET_GPU_ADAPTER_PERFCOUNTER),
        EtGpuPerfCounterAdapterEqualFunction,
        EtGpuPerfCounterAdapterHashFunction,
        2
        );
}

VOID EtGpuResetHashtables(
    VOID
    )
{
    static ULONG64 lastTickCount = 0;
    ULONG64 tickCount = NtGetTickCount64();

    if (lastTickCount == 0)
        lastTickCount = tickCount;

    // Reset hashtable once in a while.
    if (tickCount - lastTickCount >= 600 * 1000)
    {
        EtGpuDeletePerfCounterHashTables();
        EtGpuCreatePerfCounterHashTables();

        lastTickCount = tickCount;
    }
    else
    {
        PhClearHashtable(EtGpuRunningTimeHashTable);
        PhClearHashtable(EtGpuProcessCounterHashTable);
        PhClearHashtable(EtGpuAdapterDedicatedHashTable);
    }
}

BOOLEAN EtGpuCleanupCounters(
    VOID
    )
{
    static ULONG64 lastTickCount = 0;
    ULONG64 tickCount = NtGetTickCount64();

    if (lastTickCount == 0)
        lastTickCount = tickCount;

    if (tickCount - lastTickCount >= 600 * 1000)
    {
        lastTickCount = tickCount;
        return TRUE;
    }

    return FALSE;
}

_Success_(return)
BOOLEAN EtPerfCounterParseGpuPerfCounterInstance(
    _In_ PWSTR InstanceName,
    _Out_ PULONG ProcessId,
    _Out_ PULONG EngineId,
    _Out_ PULONG EngineLuid
    )
{
    PH_STRINGREF pidPartSr;
    PH_STRINGREF luidHighPartSr;
    PH_STRINGREF luidLowPartSr;
    PH_STRINGREF physPartSr;
    PH_STRINGREF engPartSr;
    PH_STRINGREF remainingPart;
    ULONG64 processId;
    ULONG64 engineId;
    LONG64 engineLuidLow;

    // pid_12220_luid_0x00000000_0x0000C85A_phys_0_eng_5_engtype_Copy
    PhInitializeStringRefLongHint(&remainingPart, InstanceName);
    PhSkipStringRef(&remainingPart, 4 * sizeof(WCHAR));

    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &pidPartSr, &remainingPart))
        return FALSE;

    PhSkipStringRef(&remainingPart, 5 * sizeof(WCHAR));

    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &luidHighPartSr, &remainingPart))
        return FALSE;
    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &luidLowPartSr, &remainingPart))
        return FALSE;

    PhSkipStringRef(&remainingPart, 5 * sizeof(WCHAR));

    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &physPartSr, &remainingPart))
        return FALSE;

    PhSkipStringRef(&remainingPart, 4 * sizeof(WCHAR));

    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &engPartSr, &remainingPart))
        return FALSE;

    PhSkipStringRef(&luidLowPartSr, 2 * sizeof(WCHAR));

    if (
        PhStringToInteger64(&pidPartSr, 10, &processId) &&
        PhStringToInteger64(&luidLowPartSr, 16, &engineLuidLow) &&
        PhStringToInteger64(&engPartSr, 10, &engineId)
        )
    {
        *ProcessId = (ULONG)processId;
        *EngineId = (ULONG)engineId;
        *EngineLuid = (ULONG)engineLuidLow;
        return TRUE;
    }

    return FALSE;
}

_Success_(return)
BOOLEAN EtPerfCounterParseGpuProcessMemoryPerfCounterInstance(
    _In_ PWSTR InstanceName,
    _Out_ PULONG ProcessId,
    _Out_ PULONG EngineLuid
    )
{
    PH_STRINGREF pidPartSr;
    PH_STRINGREF luidHighPartSr;
    PH_STRINGREF luidLowPartSr;
    PH_STRINGREF remainingPart;
    ULONG64 processId;
    LONG64 engineLuidLow;

    // pid_1116_luid_0x00000000_0x0000D3EC_phys_0
    PhInitializeStringRefLongHint(&remainingPart, InstanceName);
    PhSkipStringRef(&remainingPart, 4 * sizeof(WCHAR));

    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &pidPartSr, &remainingPart))
        return FALSE;

    PhSkipStringRef(&remainingPart, 5 * sizeof(WCHAR));

    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &luidHighPartSr, &remainingPart))
        return FALSE;
    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &luidLowPartSr, &remainingPart))
        return FALSE;

    PhSkipStringRef(&luidLowPartSr, 2 * sizeof(WCHAR));

    if (
        PhStringToInteger64(&pidPartSr, 10, &processId) &&
        PhStringToInteger64(&luidLowPartSr, 16, &engineLuidLow)
        )
    {
        *ProcessId = (ULONG)processId;
        *EngineLuid = (ULONG)engineLuidLow;
        return TRUE;
    }

    return FALSE;
}

_Success_(return)
BOOLEAN EtPerfCounterParseGpuAdapterDedicatedPerfCounterInstance(
    _In_ PWSTR InstanceName,
    _Out_ PULONG EngineLuid
    )
{
    PH_STRINGREF luidHighPartSr;
    PH_STRINGREF luidLowPartSr;
    PH_STRINGREF remainingPart;
    LONG64 engineLuidLow;

    // luid_0x00000000_0x0000C4CF_phys_0
    PhInitializeStringRefLongHint(&remainingPart, InstanceName);
    PhSkipStringRef(&remainingPart, 5 * sizeof(WCHAR));

    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &luidHighPartSr, &remainingPart))
        return FALSE;
    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &luidLowPartSr, &remainingPart))
        return FALSE;

    PhSkipStringRef(&luidLowPartSr, 2 * sizeof(WCHAR));

    if (PhStringToInteger64(&luidLowPartSr, 16, &engineLuidLow))
    {
        *EngineLuid = (ULONG)engineLuidLow;
        return TRUE;
    }

    return FALSE;
}

PET_GPU_ENGINE_PERFCOUNTER EtPerfCounterAddOrUpdateGpuEngineCounters(
    _In_ ET_GPU_ENGINE_PERF_COUNTER CounterInstance
    )
{
    ET_GPU_ENGINE_PERFCOUNTER lookupEntry;
    PET_GPU_ENGINE_PERFCOUNTER entry;

    if (!EtPerfCounterEngineInstanceHashTable)
        return NULL;

    lookupEntry.InstanceId = CounterInstance.InstanceId;

    if (entry = PhFindEntryHashtable(EtPerfCounterEngineInstanceHashTable, &lookupEntry))
    {
        DOUBLE numerator;
        DOUBLE denomenator;

        numerator = (DOUBLE)entry->InstanceValue - (DOUBLE)CounterInstance.InstanceValue;

        if (numerator)
        {
            denomenator = (DOUBLE)entry->InstanceTime - (DOUBLE)CounterInstance.InstanceTime;
            entry->CounterValue = (numerator / denomenator) * 100.0;

            if (entry->CounterValue < 0.0)
                entry->CounterValue = 0.0;
            if (entry->CounterValue > 100.0)
                entry->CounterValue = 0.0; // Taskmgr zeros values above 100 (dmex) 
        }
        else
        {
            entry->CounterValue = 0.0;
        }

        entry->InstanceValue = CounterInstance.InstanceValue;
        entry->InstanceTime = CounterInstance.InstanceTime;

        return entry;
    }
    else
    {
        ULONG processId;
        ULONG engineId;
        ULONG engineLuid;

        if (CounterInstance.InstanceValue == 0)
            return NULL;

        if (EtPerfCounterParseGpuPerfCounterInstance(
            CounterInstance.InstanceName,
            &processId,
            &engineId,
            &engineLuid
            ))
        {
            lookupEntry.InstanceId = CounterInstance.InstanceId;
            lookupEntry.ProcessId = processId;
            lookupEntry.EngineId = engineId;
            lookupEntry.EngineLuid = engineLuid;
            lookupEntry.InstanceValue = CounterInstance.InstanceValue;
            lookupEntry.InstanceTime = CounterInstance.InstanceTime;
            lookupEntry.CounterValue = 0.0;

            PhAddEntryHashtable(EtPerfCounterEngineInstanceHashTable, &lookupEntry);
        }
    }

    return NULL;
}

PET_GPU_PROCESS_PERFCOUNTER EtPerfCounterAddOrUpdateGpuProcessCounters(
    _In_ ET_GPU_PROCESSMEMORY_PERF_COUNTER CounterInstance
    )
{
    ET_GPU_PROCESS_PERFCOUNTER lookupEntry;
    PET_GPU_PROCESS_PERFCOUNTER entry;

    if (!EtPerfCounterProcessInstanceHashTable)
        return NULL;

    lookupEntry.InstanceId = CounterInstance.InstanceId;

    if (entry = PhFindEntryHashtable(EtPerfCounterProcessInstanceHashTable, &lookupEntry))
    {
        entry->SharedUsage = CounterInstance.SharedUsage;
        entry->DedicatedUsage = CounterInstance.DedicatedUsage;
        entry->CommitUsage = CounterInstance.CommitUsage;
        return entry;
    }
    else
    {
        ULONG processId;
        ULONG engineLuid;

        if (CounterInstance.SharedUsage == 0 && CounterInstance.DedicatedUsage == 0 && CounterInstance.CommitUsage == 0)
            return NULL;

        if (EtPerfCounterParseGpuProcessMemoryPerfCounterInstance(
            CounterInstance.InstanceName,
            &processId,
            &engineLuid
            ))
        {
            lookupEntry.InstanceId = CounterInstance.InstanceId;
            lookupEntry.ProcessId = processId;
            lookupEntry.EngineLuid = engineLuid;
            lookupEntry.SharedUsage = CounterInstance.SharedUsage;
            lookupEntry.DedicatedUsage = CounterInstance.DedicatedUsage;
            lookupEntry.CommitUsage = CounterInstance.CommitUsage;

            PhAddEntryHashtable(EtPerfCounterProcessInstanceHashTable, &lookupEntry);
        }
    }

    return NULL;
}

PET_GPU_ADAPTER_PERFCOUNTER EtPerfCounterAddOrUpdateGpuAdapterCounters(
    _In_ ET_GPU_ADAPTER_PERF_COUNTER CounterInstance
    )
{
    ET_GPU_ADAPTER_PERFCOUNTER lookupEntry;
    PET_GPU_ADAPTER_PERFCOUNTER entry;

    if (!EtPerfCounterAdapterInstanceHashTable)
        return NULL;

    lookupEntry.InstanceId = CounterInstance.InstanceId;

    if (entry = PhFindEntryHashtable(EtPerfCounterAdapterInstanceHashTable, &lookupEntry))
    {
        //entry->SharedUsage = CounterInstance.SharedUsage;
        entry->DedicatedUsage = CounterInstance.DedicatedUsage;
        //entry->CommitUsage = CounterInstance.CommitUsage;
        return entry;
    }
    else
    {
        ULONG engineLuid;

        if (CounterInstance.DedicatedUsage == 0)
            return NULL;
        //if (CounterInstance.SharedUsage == 0 && CounterInstance.DedicatedUsage == 0 && CounterInstance.CommitUsage == 0)
        //    return NULL;

        if (EtPerfCounterParseGpuAdapterDedicatedPerfCounterInstance(
            CounterInstance.InstanceName,
            &engineLuid
            ))
        {
            lookupEntry.InstanceId = CounterInstance.InstanceId;
            lookupEntry.EngineLuid = engineLuid;
            //lookupEntry.SharedUsage = CounterInstance.SharedUsage;
            lookupEntry.DedicatedUsage = CounterInstance.DedicatedUsage;
            //lookupEntry.CommitUsage = CounterInstance.CommitUsage;

            PhAddEntryHashtable(EtPerfCounterAdapterInstanceHashTable, &lookupEntry);
        }
    }

    return NULL;
}

VOID EtPerfCounterCleanupDeletedGpuEngineCounters(
    _In_ PET_GPU_ENGINE_PERF_COUNTER Counters,
    _In_ ULONG NumberOfCounters
    )
{
    PPH_LIST countersToAdd = NULL;
    PPH_HASHTABLE newHashTable;
    PPH_HASHTABLE oldHashTable;
    PET_GPU_PROCESS_PERFCOUNTER counter;
    PH_HASHTABLE_ENUM_CONTEXT enumContext;

    newHashTable = PhCreateHashtable(
        sizeof(ET_GPU_ENGINE_PERFCOUNTER),
        EtPerfCounterInstanceEqualFunction,
        EtPerfCounterInstanceHashFunction,
        64
        );

    PhBeginEnumHashtable(EtPerfCounterEngineInstanceHashTable, &enumContext);

    while (counter = PhNextEnumHashtable(&enumContext))
    {
        for (ULONG i = 0; i < NumberOfCounters; i++)
        {
            if (counter->InstanceId == Counters[i].InstanceId)
            {
                PhAddEntryHashtable(newHashTable, counter);
                break;
            }
        }
    }

    oldHashTable = EtPerfCounterEngineInstanceHashTable;
    EtPerfCounterEngineInstanceHashTable = newHashTable;
    PhDereferenceObject(oldHashTable);

    //PPH_LIST countersToRemove = NULL;
    //PH_HASHTABLE_ENUM_CONTEXT enumContext;
    //PET_GPU_ENGINE_PERFCOUNTER counter;
    //
    //PhBeginEnumHashtable(EtPerfCounterEngineInstanceHashTable, &enumContext);
    //
    //while (counter = PhNextEnumHashtable(&enumContext))
    //{
    //    BOOLEAN found = FALSE;
    //
    //    for (ULONG i = 0; i < NumberOfCounters; i++)
    //    {
    //        if (counter->InstanceId == Counters[i].InstanceId)
    //        {
    //            found = TRUE;
    //            break;
    //        }
    //    }
    //
    //    if (!found)
    //    {
    //        if (!countersToRemove)
    //            countersToRemove = PhCreateList(2);
    //
    //        PhAddItemList(countersToRemove, counter);
    //    }
    //}
    //
    //if (countersToRemove)
    //{
    //    for (ULONG i = 0; i < countersToRemove->Count; i++)
    //    {
    //        PhRemoveEntryHashtable(EtPerfCounterEngineInstanceHashTable, countersToRemove->Items[i]);
    //    }
    //
    //    PhDereferenceObject(countersToRemove);
    //}
}

VOID EtPerfCounterCleanupDeletedGpuProcessCounters(
    _In_ PET_GPU_PROCESSMEMORY_PERF_COUNTER Counters,
    _In_ ULONG NumberOfCounters
    )
{
    PPH_HASHTABLE newHashTable;
    PPH_HASHTABLE oldHashTable;
    PET_GPU_PROCESS_PERFCOUNTER counter;
    PH_HASHTABLE_ENUM_CONTEXT enumContext;

    newHashTable = PhCreateHashtable(
        sizeof(ET_GPU_PROCESS_PERFCOUNTER),
        EtGpuPerfCounterProcessEqualFunction,
        EtGpuPerfCounterProcessHashFunction,
        64
        );

    PhBeginEnumHashtable(EtPerfCounterProcessInstanceHashTable, &enumContext);

    while (counter = PhNextEnumHashtable(&enumContext))
    {
        for (ULONG i = 0; i < NumberOfCounters; i++)
        {
            if (counter->InstanceId == Counters[i].InstanceId)
            {
                PhAddEntryHashtable(newHashTable, counter);
                break;
            }
        }
    }

    oldHashTable = EtPerfCounterProcessInstanceHashTable;
    EtPerfCounterProcessInstanceHashTable = newHashTable;
    PhDereferenceObject(oldHashTable);

    //PPH_LIST countersToRemove = NULL;
    //PH_HASHTABLE_ENUM_CONTEXT enumContext;
    //PET_GPU_PROCESS_PERFCOUNTER counter;
    //
    //PhBeginEnumHashtable(EtPerfCounterProcessInstanceHashTable, &enumContext);
    //
    //while (counter = PhNextEnumHashtable(&enumContext))
    //{
    //    BOOLEAN found = FALSE;
    //
    //    for (ULONG i = 0; i < NumberOfCounters; i++)
    //    {
    //        if (counter->InstanceId == Counters[i].InstanceId)
    //        {
    //            found = TRUE;
    //            break;
    //        }
    //    }
    //
    //    if (!found)
    //    {
    //        if (!countersToRemove)
    //            countersToRemove = PhCreateList(2);
    //
    //        PhAddItemList(countersToRemove, counter);
    //    }
    //}
    //
    //if (countersToRemove)
    //{
    //    for (ULONG i = 0; i < countersToRemove->Count; i++)
    //    {
    //        PhRemoveEntryHashtable(EtPerfCounterProcessInstanceHashTable, countersToRemove->Items[i]);
    //    }
    //
    //    PhDereferenceObject(countersToRemove);
    //}
}

VOID EtPerfCounterCleanupDeletedGpuAdapterCounters(
    _In_ PET_GPU_ADAPTER_PERF_COUNTER Counters,
    _In_ ULONG NumberOfCounters
    )
{
    PPH_HASHTABLE newHashTable;
    PPH_HASHTABLE oldHashTable;
    PET_GPU_PROCESS_PERFCOUNTER counter;
    PH_HASHTABLE_ENUM_CONTEXT enumContext;

    newHashTable = PhCreateHashtable(
        sizeof(ET_GPU_ADAPTER_PERFCOUNTER),
        EtGpuPerfCounterAdapterEqualFunction,
        EtGpuPerfCounterAdapterHashFunction,
        2
        );

    PhBeginEnumHashtable(EtPerfCounterAdapterInstanceHashTable, &enumContext);

    while (counter = PhNextEnumHashtable(&enumContext))
    {
        for (ULONG i = 0; i < NumberOfCounters; i++)
        {
            if (counter->InstanceId == Counters[i].InstanceId)
            {
                PhAddEntryHashtable(newHashTable, counter);
                break;
            }
        }
    }

    oldHashTable = EtPerfCounterAdapterInstanceHashTable;
    EtPerfCounterAdapterInstanceHashTable = newHashTable;
    PhDereferenceObject(oldHashTable);

    //PPH_LIST countersToRemove = NULL;
    //PH_HASHTABLE_ENUM_CONTEXT enumContext;
    //PET_GPU_ADAPTER_PERFCOUNTER counter;
    //
    //PhBeginEnumHashtable(EtPerfCounterAdapterInstanceHashTable, &enumContext);
    //
    //while (counter = PhNextEnumHashtable(&enumContext))
    //{
    //    BOOLEAN found = FALSE;
    //
    //    for (ULONG i = 0; i < NumberOfCounters; i++)
    //    {
    //        if (counter->InstanceId == Counters[i].InstanceId)
    //        {
    //            found = TRUE;
    //            break;
    //        }
    //    }
    //
    //    if (!found)
    //    {
    //        if (!countersToRemove)
    //            countersToRemove = PhCreateList(2);
    //
    //        PhAddItemList(countersToRemove, counter);
    //    }
    //}
    //
    //if (countersToRemove)
    //{
    //    for (ULONG i = 0; i < countersToRemove->Count; i++)
    //    {
    //        PhRemoveEntryHashtable(EtPerfCounterAdapterInstanceHashTable, countersToRemove->Items[i]);
    //    }
    //
    //    PhDereferenceObject(countersToRemove);
    //}
}

VOID EtPerfCounterProcessGpuEngineUtilizationCounter(
    _In_ PET_GPU_ENGINE_PERFCOUNTER CounterInstance
    )
{
    PET_GPU_ENGINE_COUNTER entry;
    ET_GPU_ENGINE_COUNTER lookupEntry;

    if (!EtGpuRunningTimeHashTable)
        return;

    lookupEntry.ProcessId = CounterInstance->ProcessId;
    lookupEntry.EngineId = CounterInstance->EngineId;
    lookupEntry.EngineLuid = CounterInstance->EngineLuid;

    if (entry = PhFindEntryHashtable(EtGpuRunningTimeHashTable, &lookupEntry))
    {
        entry->ValueF = (FLOAT)CounterInstance->CounterValue;
        entry->GpuTime = CounterInstance->InstanceTime;
    }
    else
    {
        if (CounterInstance->CounterValue == 0)
            return;

        lookupEntry.ProcessId = CounterInstance->ProcessId;
        lookupEntry.EngineId = CounterInstance->EngineId;
        lookupEntry.EngineLuid = CounterInstance->EngineLuid;
        lookupEntry.ValueF = (FLOAT)CounterInstance->CounterValue;
        lookupEntry.GpuTime = CounterInstance->InstanceTime;

        PhAddEntryHashtable(EtGpuRunningTimeHashTable, &lookupEntry);
    }
}

VOID EtPerfCounterGpuProcessUtilizationCounter(
    _In_ PET_GPU_PROCESS_PERFCOUNTER CounterInstance
    )
{
    PET_GPU_PROCESS_COUNTER entry;
    ET_GPU_PROCESS_COUNTER lookupEntry;

    if (!EtGpuProcessCounterHashTable)
        return;

    lookupEntry.ProcessId = CounterInstance->ProcessId;
    lookupEntry.EngineLuid = CounterInstance->EngineLuid;

    if (entry = PhFindEntryHashtable(EtGpuProcessCounterHashTable, &lookupEntry))
    {
        entry->SharedUsage = CounterInstance->SharedUsage;
        entry->DedicatedUsage = CounterInstance->DedicatedUsage;
        entry->CommitUsage = CounterInstance->CommitUsage;
    }
    else
    {
        if (CounterInstance->SharedUsage == 0 && CounterInstance->DedicatedUsage == 0 && CounterInstance->CommitUsage == 0)
            return;

        lookupEntry.ProcessId = CounterInstance->ProcessId;
        lookupEntry.EngineLuid = CounterInstance->EngineLuid;
        lookupEntry.SharedUsage = CounterInstance->SharedUsage;
        lookupEntry.DedicatedUsage = CounterInstance->DedicatedUsage;
        lookupEntry.CommitUsage = CounterInstance->CommitUsage;

        PhAddEntryHashtable(EtGpuProcessCounterHashTable, &lookupEntry);
    }
}

VOID EtPerfCounterGpuAdapterDedicatedCounter(
    _In_ PET_GPU_ADAPTER_PERFCOUNTER CounterInstance
    )
{
    PET_GPU_ADAPTER_COUNTER entry;
    ET_GPU_ADAPTER_COUNTER lookupEntry;

    if (!EtGpuAdapterDedicatedHashTable)
        return;

    lookupEntry.EngineLuid = CounterInstance->EngineLuid;

    if (entry = PhFindEntryHashtable(EtGpuAdapterDedicatedHashTable, &lookupEntry))
    {
        //entry->SharedUsage = CounterInstance->SharedUsage;
        entry->DedicatedUsage = CounterInstance->DedicatedUsage;
        //entry->CommitUsage = CounterInstance->CommitUsage;
    }
    else
    {
        if (CounterInstance->DedicatedUsage == 0)
            return;
        //if (CounterInstance->SharedUsage == 0 && CounterInstance->DedicatedUsage == 0 && CounterInstance->CommitUsage == 0)
        //    return;

        lookupEntry.EngineLuid = CounterInstance->EngineLuid;
        //lookupEntry.SharedUsage = CounterInstance->SharedUsage;
        lookupEntry.DedicatedUsage = CounterInstance->DedicatedUsage;
        //lookupEntry.CommitUsage = CounterInstance->CommitUsage;

        PhAddEntryHashtable(EtGpuAdapterDedicatedHashTable, &lookupEntry);
    }
}

ULONG EtPerfCounterAddCounters(
    _In_ HANDLE CounterHandle,
    _In_ GUID CounterGuid,
    _In_ ULONG CounterId
    )
{
    ULONG status;
    ULONG counterIdentifierLength;
    PPERF_COUNTER_IDENTIFIER counterIdentifierBuffer;

    counterIdentifierLength = ALIGN_UP_BY(sizeof(PERF_COUNTER_IDENTIFIER) + sizeof(PERF_WILDCARD_INSTANCE), 8);
    counterIdentifierBuffer = PhAllocateZero(counterIdentifierLength);
    counterIdentifierBuffer->Size = counterIdentifierLength;
    counterIdentifierBuffer->CounterSetGuid = CounterGuid;
    counterIdentifierBuffer->InstanceId = ULONG_MAX;
    counterIdentifierBuffer->CounterId = CounterId;
    memcpy(
        PTR_ADD_OFFSET(counterIdentifierBuffer, RTL_SIZEOF_THROUGH_FIELD(PERF_COUNTER_IDENTIFIER, Reserved)),
        PERF_WILDCARD_INSTANCE,
        sizeof(PERF_WILDCARD_INSTANCE) // - sizeof(UNICODE_NULL)
        );

    status = PerfAddCounters(
        CounterHandle,
        counterIdentifierBuffer,
        counterIdentifierLength
        );

    if (status == ERROR_SUCCESS)
    {
        status = counterIdentifierBuffer->Status;
    }

    return status;
}

_Success_(return)
BOOLEAN EtPerfCounterGetCounterData(
    _In_ HANDLE CounterHandle,
    _Out_ PPERF_DATA_HEADER *CounterBuffer
    )
{
    static ULONG initialBufferSize = 0x4000;
    ULONG status;
    ULONG bufferSize;
    PPERF_DATA_HEADER buffer;

    {
        static ULONG64 lastTickCount = 0;
        ULONG64 tickCount = NtGetTickCount64();

        if (lastTickCount == 0)
            lastTickCount = tickCount;

        if (tickCount - lastTickCount >= 600 * 1000)
        {
            lastTickCount = tickCount;
            // Reset initial buffer size.
            initialBufferSize = 0x4000;
        }
    }

    bufferSize = initialBufferSize;
    buffer = PhAllocate(bufferSize);

    status = PerfQueryCounterData(
        CounterHandle,
        buffer,
        bufferSize,
        &bufferSize
        );

    if (status == ERROR_NOT_ENOUGH_MEMORY)
    {
        if (initialBufferSize < bufferSize)
            initialBufferSize = bufferSize;

        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = PerfQueryCounterData(
            CounterHandle,
            buffer,
            bufferSize,
            &bufferSize
            );
    }

    if (status == ERROR_SUCCESS)
    {
        *CounterBuffer = buffer;
        return TRUE;
    }

    return FALSE;
}

//ULONG EtPerfCounterDumpCounterInfo(
//    _In_ GUID CounterGuid
//    )
//{
//    ULONG status;
//    ULONG stringBufferHeaderLength = 0;
//    PVOID stringBufferHeader = NULL;
//
//    status = PerfQueryCounterSetRegistrationInfo(
//        NULL,
//        &CounterGuid,
//        PERF_REG_COUNTER_NAME_STRINGS,
//        0,
//        NULL,
//        0,
//        &stringBufferHeaderLength
//        );
//
//    stringBufferHeader = PhAllocate(stringBufferHeaderLength);
//
//    status = PerfQueryCounterSetRegistrationInfo(
//        NULL,
//        &CounterGuid,
//        PERF_REG_COUNTER_NAME_STRINGS,
//        0,
//        (PBYTE)stringBufferHeader,
//        stringBufferHeaderLength,
//        &stringBufferHeaderLength
//        );
//
//    if (status == ERROR_SUCCESS)
//    {
//        PPERF_STRING_BUFFER_HEADER stringHeaders = stringBufferHeader;
//        PPERF_STRING_COUNTER_HEADER stringHeaderCounterArray = PTR_ADD_OFFSET(stringHeaders, RTL_SIZEOF_THROUGH_FIELD(PERF_STRING_BUFFER_HEADER, dwCounters));
//
//        for (ULONG i = 0; i < stringHeaders->dwCounters; i++)
//        {
//            PERF_STRING_COUNTER_HEADER currentStringHeader = stringHeaderCounterArray[i];
//            PWSTR instanceName = PTR_ADD_OFFSET(stringHeaders, currentStringHeader.dwOffset);
//
//            PERF_COUNTER_REG_INFO info = { 0 };
//            status = PerfQueryCounterSetRegistrationInfo(
//                NULL,
//                &CounterGuid,
//                PERF_REG_COUNTER_STRUCT,
//                currentStringHeader.dwCounterId,
//                (PBYTE)&info,
//                sizeof(PERF_COUNTER_REG_INFO),
//                &stringBufferHeaderLength
//                );
//
//            dprintf("[%lu] [%lu] %S\n", currentStringHeader.dwCounterId, info.Type, instanceName);
//        }
//    }
//
//    return status;
//}
//
//ULONG EtPerfCounterQueryCounterInfo(
//    _In_ HANDLE QueryHandle
//    )
//{
//    ULONG status;
//    ULONG perfCounterInfoLength = 0;
//    PVOID perfCounterInfoBuffer = NULL;
//
//    status = PerfQueryCounterInfo(
//        QueryHandle,
//        NULL,
//        0,
//        &perfCounterInfoLength
//        );
//
//    perfCounterInfoBuffer = PhAllocateZero(perfCounterInfoLength);
//
//    status = PerfQueryCounterInfo(
//        QueryHandle,
//        perfCounterInfoBuffer,
//        perfCounterInfoLength,
//        &perfCounterInfoLength
//        );
//
//    if (status == ERROR_SUCCESS)
//    {
//        PPERF_COUNTER_IDENTIFIER perfCounterId = perfCounterInfoBuffer;
//        ULONG perfCounterIdLength = perfCounterId->Size;
//
//        while (TRUE)
//        {
//            if (IsEqualGUID(&perfCounterId->CounterSetGuid, &GUID_GPU_ENGINE))
//            {
//                dprintf("GPU_ENGINE index: %lu\n", perfCounterId->Index);
//            }
//
//            if (IsEqualGUID(&perfCounterId->CounterSetGuid, &GUID_GPU_ADAPTERMEMORY))
//            {
//                dprintf("GPU_ADAPTERMEMORY index: %lu\n", perfCounterId->Index);
//            }
//
//            if (IsEqualGUID(&perfCounterId->CounterSetGuid, &GUID_GPU_PROCESSMEMORY))
//            {
//                dprintf("GPU_PROCESSMEMORY index: %lu\n", perfCounterId->Index);
//            }
//
//            if (perfCounterIdLength >= perfCounterInfoLength)
//                break;
//
//            perfCounterIdLength += perfCounterId->Size;
//            perfCounterId = PTR_ADD_OFFSET(perfCounterId, perfCounterId->Size);
//        }
//    }
//
//    PhFree(perfCounterInfoBuffer);
//
//    return status;
//}

VOID EtPerfCounterInitialization(
    VOID
    )
{
    EtGpuCreatePerfCounterHashTables();
    EtGpuCreatePerfCounterHashTable();
}

_Success_(return)
BOOLEAN EtPerfCounterOpenHandle(
    _Out_ PHANDLE PerfQueryHandle
    )
{
    ULONG status;
    HANDLE perfQueryHandle;

    status = PerfOpenQueryHandle(NULL, &perfQueryHandle);
    if (status != ERROR_SUCCESS)
        return FALSE;

    status = EtPerfCounterAddCounters(perfQueryHandle, GUID_GPU_ENGINE, 1);
    if (status != ERROR_SUCCESS)
        goto CleanupExit;

    status = EtPerfCounterAddCounters(perfQueryHandle, GUID_GPU_ADAPTERMEMORY, 2);
    if (status != ERROR_SUCCESS)
        goto CleanupExit;

    status = EtPerfCounterAddCounters(perfQueryHandle, GUID_GPU_PROCESSMEMORY, PERF_WILDCARD_COUNTER);
    if (status != ERROR_SUCCESS)
        goto CleanupExit;

    *PerfQueryHandle = perfQueryHandle;
    return TRUE;

    //EtPerfCounterQueryCounterInfo(perfQueryHandle);
    //EtPerfCounterDumpCounterInfo(GUID_GPU_ENGINE);
    //EtPerfCounterDumpCounterInfo(GUID_GPU_ADAPTERMEMORY);
    //EtPerfCounterDumpCounterInfo(GUID_GPU_PROCESSMEMORY);

CleanupExit:
    PerfCloseQueryHandle(perfQueryHandle);
    return FALSE;
}

NTSTATUS EtUpdatePerfCounterData(
    VOID
    )
{
    static HANDLE perfQueryHandle = NULL;
    PPERF_DATA_HEADER perfQueryBuffer;
    PPERF_COUNTER_HEADER perfCounterHeader;
    PET_GPU_ENGINE_PERF_COUNTER gpuEngineCounters = NULL;
    PET_GPU_PROCESSMEMORY_PERF_COUNTER gpuProcessCounters = NULL;
    PET_GPU_ADAPTER_PERF_COUNTER gpuAdapterCounters = NULL;
    ULONG numberOfGpuEngineCounters = 0;
    ULONG numberOfGpuProcessCounters = 0;
    ULONG numberOfGpuAdapterCounters = 0;
    ULONG i;
    ULONG j;
    BOOLEAN cleanupCounters;

    if (!perfQueryHandle)
    {
        if (!EtPerfCounterOpenHandle(&perfQueryHandle))
            return STATUS_UNSUCCESSFUL;
    }

    if (!EtPerfCounterGetCounterData(perfQueryHandle, &perfQueryBuffer))
        return STATUS_UNSUCCESSFUL;

    if (perfQueryBuffer->dwNumCounters != 3)
    {
        PhFree(perfQueryBuffer);
        return STATUS_UNSUCCESSFUL;
    }

    cleanupCounters = EtGpuCleanupCounters();
    EtGpuResetHashtables();

    perfCounterHeader = PTR_ADD_OFFSET(perfQueryBuffer, sizeof(PERF_DATA_HEADER));

    for (i = 0; i < perfQueryBuffer->dwNumCounters; i++)
    {
        if (perfCounterHeader->dwStatus != ERROR_SUCCESS)
            continue;

        switch (perfCounterHeader->dwType)
        {
        case PERF_MULTIPLE_INSTANCES:
            {
                PPERF_MULTI_INSTANCES perfMultipleInstance = PTR_ADD_OFFSET(perfCounterHeader, sizeof(PERF_COUNTER_HEADER));
                PPERF_INSTANCE_HEADER perfCurrentInstance = PTR_ADD_OFFSET(perfMultipleInstance, sizeof(PERF_MULTI_INSTANCES));
                ULONG count = 0;
                ULONG index = 0;

                // Do a scan and determine how many instances have values.
                for (j = 0; j < perfMultipleInstance->dwInstances; j++)
                {
                    PET_GPU_COUNTER_DATA perfCounterData = PTR_ADD_OFFSET(perfCurrentInstance, perfCurrentInstance->Size);

                    if (perfCounterData->Value)
                    {
                        switch (i)
                        {
                        case ET_GPU_ADAPTERMEMORY_COUNTER_INDEX:
                            count++;
                            break;
                        case ET_GPU_ENGINE_COUNTER_INDEX:
                            count++;
                            break;
                        }
                    }

                    perfCurrentInstance = PTR_ADD_OFFSET(perfCounterData, perfCounterData->Size);
                }

                if (count)
                {
                    switch (i)
                    {
                    case ET_GPU_ADAPTERMEMORY_COUNTER_INDEX:
                        {
                            numberOfGpuAdapterCounters = count;
                            gpuAdapterCounters = PhAllocate(sizeof(ET_GPU_ADAPTER_PERF_COUNTER) * numberOfGpuAdapterCounters);
                            memset(gpuAdapterCounters, 0, sizeof(ET_GPU_ADAPTER_PERF_COUNTER) * numberOfGpuAdapterCounters);
                        }
                        break;
                    case ET_GPU_ENGINE_COUNTER_INDEX:
                        {
                            numberOfGpuEngineCounters = count;
                            gpuEngineCounters = PhAllocate(sizeof(ET_GPU_ENGINE_PERF_COUNTER) * numberOfGpuEngineCounters);
                            memset(gpuEngineCounters, 0, sizeof(ET_GPU_ENGINE_PERF_COUNTER) * numberOfGpuEngineCounters);
                        }
                        break;
                    }
                }

                perfCurrentInstance = PTR_ADD_OFFSET(perfMultipleInstance, sizeof(PERF_MULTI_INSTANCES));

                for (j = 0; j < perfMultipleInstance->dwInstances; j++)
                {
                    PET_GPU_COUNTER_DATA perfCounterData = PTR_ADD_OFFSET(perfCurrentInstance, perfCurrentInstance->Size);

                    if (perfCounterData->Value)
                    {
                        PWSTR perfCounterInstanceName = PTR_ADD_OFFSET(perfCurrentInstance, sizeof(PERF_INSTANCE_HEADER));

                        switch (i)
                        {
                        case ET_GPU_ADAPTERMEMORY_COUNTER_INDEX:
                            {
                                if (gpuAdapterCounters)
                                {
                                    gpuAdapterCounters[index].InstanceId = perfCurrentInstance->InstanceId;
                                    gpuAdapterCounters[index].InstanceName = perfCounterInstanceName;
                                    gpuAdapterCounters[index].DedicatedUsage = perfCounterData->Value;
                                    index++;
                                }
                            }
                            break;
                        case ET_GPU_ENGINE_COUNTER_INDEX:
                            {
                                if (gpuEngineCounters)
                                {
                                    gpuEngineCounters[index].InstanceId = perfCurrentInstance->InstanceId;
                                    gpuEngineCounters[index].InstanceName = perfCounterInstanceName;
                                    gpuEngineCounters[index].InstanceValue = perfCounterData->Value;
                                    gpuEngineCounters[index].InstanceTime = perfQueryBuffer->PerfTime100NSec;
                                    index++;
                                }
                            }
                            break;
                        }
                    }

                    perfCurrentInstance = PTR_ADD_OFFSET(perfCounterData, perfCounterData->Size);
                }
            }
            break;
        case PERF_COUNTERSET:
            {
                PPERF_MULTI_COUNTERS perfMultipleCounters = PTR_ADD_OFFSET(perfCounterHeader, sizeof(PERF_COUNTER_HEADER));
                PPERF_MULTI_INSTANCES perfMultipleInstance = PTR_ADD_OFFSET(perfMultipleCounters, perfMultipleCounters->dwSize);
                PPERF_INSTANCE_HEADER perfCurrentInstance = PTR_ADD_OFFSET(perfMultipleInstance, sizeof(PERF_MULTI_INSTANCES));
                PULONG perfCounterIdArray = PTR_ADD_OFFSET(perfMultipleCounters, sizeof(PERF_MULTI_COUNTERS));

                if (perfMultipleInstance->dwInstances)
                {
                    switch (i)
                    {
                    //case ET_GPU_ADAPTERMEMORY_COUNTER_INDEX:
                    //    {
                    //        numberOfGpuAdapterCounters = perfMultipleInstance->dwInstances;
                    //        gpuAdapterCounters = PhAllocate(sizeof(ET_GPU_ADAPTER_PERF_COUNTER) * numberOfGpuAdapterCounters);
                    //        memset(gpuAdapterCounters, 0, sizeof(ET_GPU_ADAPTER_PERF_COUNTER)* numberOfGpuAdapterCounters);
                    //    }
                    //    break;
                    case ET_GPU_PROCESSMEMORY_COUNTER_INDEX:
                        {
                            numberOfGpuProcessCounters = perfMultipleInstance->dwInstances;
                            gpuProcessCounters = PhAllocate(sizeof(ET_GPU_PROCESSMEMORY_PERF_COUNTER) * numberOfGpuProcessCounters);
                            memset(gpuProcessCounters, 0, sizeof(ET_GPU_PROCESSMEMORY_PERF_COUNTER) * numberOfGpuProcessCounters);
                        }
                        break;
                    }
                }

                for (j = 0; j < perfMultipleInstance->dwInstances; j++)
                {
                    PET_GPU_COUNTER_DATA currentCounterData = PTR_ADD_OFFSET(perfCurrentInstance, perfCurrentInstance->Size);
                    PWSTR instanceName = PTR_ADD_OFFSET(perfCurrentInstance, sizeof(PERF_INSTANCE_HEADER));

                    if (gpuProcessCounters)
                    {
                        switch (i)
                        {
                        //case ET_GPU_ADAPTERMEMORY_COUNTER_INDEX:
                        //    gpuAdapterCounters[j].InstanceId = perfCurrentInstance->InstanceId;
                        //    gpuAdapterCounters[j].InstanceName = instanceName;
                        //    break;
                        case ET_GPU_PROCESSMEMORY_COUNTER_INDEX:
                            gpuProcessCounters[j].InstanceId = perfCurrentInstance->InstanceId;
                            gpuProcessCounters[j].InstanceName = instanceName;
                            break;
                        }
                    }

                    for (ULONG k = 0; k < perfMultipleCounters->dwCounters; k++)
                    {
                        ULONG currentCounterId = perfCounterIdArray[k];

                        switch (i)
                        {
                        //case ET_GPU_ADAPTERMEMORY_COUNTER_INDEX:
                        //    {
                        //        if (gpuAdapterCounters)
                        //        {
                        //            switch (currentCounterId)
                        //            {
                        //            case ET_GPU_ADAPTERMEMORY_TOTALCOMMITTED_INDEX:
                        //                gpuAdapterCounters[j].CommitUsage = currentCounterData->Value;
                        //                break;
                        //            case ET_GPU_ADAPTERMEMORY_DEDICATEDUSAGE_INDEX:
                        //                gpuAdapterCounters[j].DedicatedUsage = currentCounterData->Value;
                        //                break;
                        //            case ET_GPU_ADAPTERMEMORY_SHAREDUSAGE_INDEX:
                        //                gpuAdapterCounters[j].SharedUsage = currentCounterData->Value;
                        //                break;
                        //            }
                        //        }
                        //    }
                        //    break;
                        case ET_GPU_PROCESSMEMORY_COUNTER_INDEX:
                            {
                                if (gpuProcessCounters)
                                {
                                    switch (currentCounterId)
                                    {
                                    case ET_GPU_PROCESSMEMORY_TOTALCOMMITTED_INDEX:
                                        gpuProcessCounters[j].CommitUsage = currentCounterData->Value;
                                        break;
                                    case ET_GPU_PROCESSMEMORY_DEDICATEDUSAGE_INDEX:
                                        gpuProcessCounters[j].DedicatedUsage = currentCounterData->Value;
                                        break;
                                    case ET_GPU_PROCESSMEMORY_SHAREDUSAGE_INDEX:
                                        gpuProcessCounters[j].SharedUsage = currentCounterData->Value;
                                        break;
                                    }
                                }
                            }
                            break;
                        }

                        currentCounterData = PTR_ADD_OFFSET(currentCounterData, currentCounterData->Size);
                    }

                    perfCurrentInstance = PTR_ADD_OFFSET(currentCounterData, 0);
                }
            }
            break;
        }

        perfCounterHeader = PTR_ADD_OFFSET(perfCounterHeader, perfCounterHeader->dwSize);
    }

    if (gpuEngineCounters)
    {
        PET_GPU_ENGINE_PERFCOUNTER counter;

        if (cleanupCounters)
        {
            EtPerfCounterCleanupDeletedGpuEngineCounters(gpuEngineCounters, numberOfGpuEngineCounters);
        }

        for (i = 0; i < numberOfGpuEngineCounters; i++)
        {
            if (counter = EtPerfCounterAddOrUpdateGpuEngineCounters(gpuEngineCounters[i]))
            {
                EtPerfCounterProcessGpuEngineUtilizationCounter(counter);
            }
        }

        PhFree(gpuEngineCounters);
    }

    if (gpuProcessCounters)
    {
        PET_GPU_PROCESS_PERFCOUNTER counter;

        if (cleanupCounters)
        {
            EtPerfCounterCleanupDeletedGpuProcessCounters(gpuProcessCounters, numberOfGpuProcessCounters);
        }

        for (i = 0; i < numberOfGpuProcessCounters; i++)
        {
            if (counter = EtPerfCounterAddOrUpdateGpuProcessCounters(gpuProcessCounters[i]))
            {
                EtPerfCounterGpuProcessUtilizationCounter(counter);
            }
        }

        PhFree(gpuProcessCounters);
    }

    if (gpuAdapterCounters)
    {
        PET_GPU_ADAPTER_PERFCOUNTER counter;

        if (cleanupCounters)
        {
            EtPerfCounterCleanupDeletedGpuAdapterCounters(gpuAdapterCounters, numberOfGpuAdapterCounters);
        }

        for (i = 0; i < numberOfGpuAdapterCounters; i++)
        {
            if (counter = EtPerfCounterAddOrUpdateGpuAdapterCounters(gpuAdapterCounters[i]))
            {
                EtPerfCounterGpuAdapterDedicatedCounter(counter);
            }
        }

        PhFree(gpuAdapterCounters);
    }

    PhFree(perfQueryBuffer);

    return STATUS_SUCCESS;
}

FLOAT EtLookupProcessGpuUtilization(
    _In_ HANDLE ProcessId
    )
{
    FLOAT value = 0;
    ULONG enumerationKey;
    PET_GPU_ENGINE_COUNTER entry;

    if (!EtGpuRunningTimeHashTable)
        return 0;

    enumerationKey = 0;

    while (PhEnumHashtable(EtGpuRunningTimeHashTable, (PVOID*)&entry, &enumerationKey))
    {
        if (UlongToHandle(entry->ProcessId) == ProcessId)
        {
            value += entry->ValueF;
        }
    }

    if (value > 0) // HACK
        value = value / 100;

    if (value > 1) // HACK
        value = 1;

    return value;
}

_Success_(return)
BOOLEAN EtLookupProcessGpuMemoryCounters(
    _In_opt_ HANDLE ProcessId,
    _Out_ PULONG64 SharedUsage,
    _Out_ PULONG64 DedicatedUsage,
    _Out_ PULONG64 CommitUsage
    )
{
    ULONG64 sharedUsage = 0;
    ULONG64 dedicatedUsage = 0;
    ULONG64 commitUsage = 0;
    ULONG enumerationKey;
    PET_GPU_PROCESS_COUNTER entry;

    if (!EtGpuProcessCounterHashTable)
        return 0;

    enumerationKey = 0;

    while (PhEnumHashtable(EtGpuProcessCounterHashTable, (PVOID*)&entry, &enumerationKey))
    {
        if (UlongToHandle(entry->ProcessId) == ProcessId)
        {
            sharedUsage += entry->SharedUsage;
            dedicatedUsage += entry->DedicatedUsage;
            commitUsage += entry->CommitUsage;
        }
    }

    if (sharedUsage && dedicatedUsage && commitUsage)
    {
        *SharedUsage = sharedUsage;
        *DedicatedUsage = dedicatedUsage;
        *CommitUsage = commitUsage;
        return TRUE;
    }

    return FALSE;
}

FLOAT EtLookupTotalGpuUtilization(
    VOID
    )
{
    FLOAT value = 0;
    ULONG enumerationKey;
    PET_GPU_ENGINE_COUNTER entry;

    if (!EtGpuRunningTimeHashTable)
        return 0;

    enumerationKey = 0;

    while (PhEnumHashtable(EtGpuRunningTimeHashTable, (PVOID*)&entry, &enumerationKey))
    {
        FLOAT usage = entry->ValueF;

        if (usage > value)
            value = usage;
    }

    if (value > 0) // HACK
        value = value / 100;

    return value;
}

FLOAT EtLookupTotalGpuEngineUtilization(
    _In_ ULONG EngineId
    )
{
    FLOAT value = 0;
    ULONG enumerationKey;
    PET_GPU_ENGINE_COUNTER entry;

    if (!EtGpuRunningTimeHashTable)
        return 0;

    enumerationKey = 0;

    while (PhEnumHashtable(EtGpuRunningTimeHashTable, (PVOID*)&entry, &enumerationKey))
    {
        if (entry->EngineId == EngineId)
        {
            FLOAT usage = entry->ValueF;

            if (usage > value)
                value = usage;
        }
    }

    if (value > 0) // HACK
        value = value / 100;

    return value;
}

ULONG64 EtLookupTotalGpuDedicated(
    VOID
    )
{
    ULONG64 value = 0;
    ULONG enumerationKey;
    PET_GPU_ADAPTER_COUNTER entry;

    if (!EtGpuAdapterDedicatedHashTable)
        return 0;

    enumerationKey = 0;

    while (PhEnumHashtable(EtGpuAdapterDedicatedHashTable, (PVOID*)&entry, &enumerationKey))
    {
        value += entry->DedicatedUsage;
    }

    return value;
}

ULONG64 EtLookupTotalGpuShared(
    VOID
    )
{
    ULONG64 value = 0;
    ULONG enumerationKey;
    PET_GPU_PROCESS_COUNTER entry;

    if (!EtGpuProcessCounterHashTable)
        return 0;

    enumerationKey = 0;

    while (PhEnumHashtable(EtGpuProcessCounterHashTable, (PVOID*)&entry, &enumerationKey))
    {
        value += entry->SharedUsage;
    }

    return value;
}

ULONG64 EtLookupTotalGpuCommit(
    VOID
    )
{
    ULONG64 value = 0;
    ULONG enumerationKey;
    PET_GPU_PROCESS_COUNTER entry;

    if (!EtGpuProcessCounterHashTable)
        return 0;

    enumerationKey = 0;

    while (PhEnumHashtable(EtGpuProcessCounterHashTable, (PVOID*)&entry, &enumerationKey))
    {
        value += entry->CommitUsage;
    }

    return value;
}

//VOID ParseGpuEngineUtilizationCounter(
//    _In_ PWSTR InstanceName,
//    _In_ DOUBLE InstanceValue
//    )
//{
//    PH_STRINGREF pidPartSr;
//    PH_STRINGREF luidHighPartSr;
//    PH_STRINGREF luidLowPartSr;
//    PH_STRINGREF physPartSr;
//    PH_STRINGREF engPartSr;
//    PH_STRINGREF remainingPart;
//    ULONG64 processId;
//    ULONG64 engineId;
//    LONG64 engineLuidLow;
//    PET_GPU_ENGINE_COUNTER entry;
//    ET_GPU_ENGINE_COUNTER lookupEntry;
//
//    if (!EtGpuRunningTimeHashTable)
//        return;
//
//    // pid_12704_luid_0x00000000_0x0000D503_phys_0_eng_3_engtype_VideoDecode
//    PhInitializeStringRefLongHint(&remainingPart, InstanceName);
//
//    PhSkipStringRef(&remainingPart, 4 * sizeof(WCHAR));
//
//    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &pidPartSr, &remainingPart))
//        return;
//
//    PhSkipStringRef(&remainingPart, 5 * sizeof(WCHAR));
//
//    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &luidHighPartSr, &remainingPart))
//        return;
//    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &luidLowPartSr, &remainingPart))
//        return;
//
//    PhSkipStringRef(&remainingPart, 5 * sizeof(WCHAR));
//
//    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &physPartSr, &remainingPart))
//        return;
//
//    PhSkipStringRef(&remainingPart, 4 * sizeof(WCHAR));
//
//    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &engPartSr, &remainingPart))
//        return;
//
//    //PhSkipStringRef(&remainingPart, 8 * sizeof(WCHAR));
//    //PhSplitStringRefAtChar(&remainingPart, L'_', &engTypePartSr, &remainingPart);
//    //PhSkipStringRef(&luidHighPartSr, 2 * sizeof(WCHAR));
//    PhSkipStringRef(&luidLowPartSr, 2 * sizeof(WCHAR));
//
//    if (
//        PhStringToInteger64(&pidPartSr, 10, &processId) &&
//        //PhStringToInteger64(&luidHighPartSr, 16, &engineLuidHigh) &&
//        PhStringToInteger64(&luidLowPartSr, 16, &engineLuidLow) &&
//        PhStringToInteger64(&engPartSr, 10, &engineId)
//        )
//    {
//        lookupEntry.ProcessId = (ULONG)processId;
//        lookupEntry.EngineId = (ULONG)engineId;
//        lookupEntry.EngineLuid = (ULONG)engineLuidLow;
//
//        if (entry = PhFindEntryHashtable(EtGpuRunningTimeHashTable, &lookupEntry))
//        {
//            //if (entry->ValueF < (FLOAT)InstanceValue)
//            entry->ValueF = (FLOAT)InstanceValue;
//        }
//        else
//        {
//            lookupEntry.ProcessId = (ULONG)processId;
//            lookupEntry.EngineId = (ULONG)engineId;
//            lookupEntry.EngineLuid = (ULONG)engineLuidLow;
//            lookupEntry.ValueF = (FLOAT)InstanceValue;
//
//            PhAddEntryHashtable(EtGpuRunningTimeHashTable, &lookupEntry);
//        }
//    }
//}
//
//VOID ParseGpuProcessMemoryDedicatedUsageCounter(
//    _In_ PWSTR InstanceName,
//    _In_ ULONG64 InstanceValue
//    )
//{
//    PH_STRINGREF pidPartSr;
//    PH_STRINGREF luidHighPartSr;
//    PH_STRINGREF luidLowPartSr;
//    PH_STRINGREF remainingPart;
//    ULONG64 processId;
//    LONG64 engineLuidLow;
//    PET_GPU_PROCESS_COUNTER entry;
//    ET_GPU_PROCESS_COUNTER lookupEntry;
//
//    if (!EtGpuProcessCounterHashTable)
//        return;
//
//    // pid_1116_luid_0x00000000_0x0000D3EC_phys_0
//    PhInitializeStringRefLongHint(&remainingPart, InstanceName);
//
//    PhSkipStringRef(&remainingPart, 4 * sizeof(WCHAR));
//
//    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &pidPartSr, &remainingPart))
//        return;
//
//    PhSkipStringRef(&remainingPart, 5 * sizeof(WCHAR));
//
//    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &luidHighPartSr, &remainingPart))
//        return;
//    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &luidLowPartSr, &remainingPart))
//        return;
//
//    //PhSkipStringRef(&remainingPart, 8 * sizeof(WCHAR));
//    //PhSplitStringRefAtChar(&remainingPart, L'_', &engTypePartSr, &remainingPart);
//    //PhSkipStringRef(&luidHighPartSr, 2 * sizeof(WCHAR));
//    PhSkipStringRef(&luidLowPartSr, 2 * sizeof(WCHAR));
//
//    if (
//        PhStringToInteger64(&pidPartSr, 10, &processId) &&
//        PhStringToInteger64(&luidLowPartSr, 16, &engineLuidLow)
//        )
//    {
//        lookupEntry.ProcessId = (ULONG)processId;
//        lookupEntry.EngineLuid = (ULONG)engineLuidLow;
//
//        if (entry = PhFindEntryHashtable(EtGpuProcessCounterHashTable, &lookupEntry))
//        {
//            //if (entry->Value64 < InstanceValue)
//            entry->DedicatedUsage = InstanceValue;
//        }
//        else
//        {
//            lookupEntry.ProcessId = (ULONG)processId;
//            lookupEntry.EngineLuid = (ULONG)engineLuidLow;
//            lookupEntry.DedicatedUsage = InstanceValue;
//
//            PhAddEntryHashtable(EtGpuProcessCounterHashTable, &lookupEntry);
//        }
//    }
//}
//
//VOID ParseGpuProcessMemorySharedUsageCounter(
//    _In_ PWSTR InstanceName,
//    _In_ ULONG64 InstanceValue
//    )
//{
//    PH_STRINGREF pidPartSr;
//    PH_STRINGREF luidHighPartSr;
//    PH_STRINGREF luidLowPartSr;
//    //PH_STRINGREF physPartSr;
//    PH_STRINGREF remainingPart;
//    ULONG64 processId;
//    LONG64 engineLuidLow;
//    //LONG64 engineLuidHigh;
//    PET_GPU_PROCESS_COUNTER entry;
//    ET_GPU_PROCESS_COUNTER lookupEntry;
//
//    if (!EtGpuProcessCounterHashTable)
//        return;
//
//    // pid_1116_luid_0x00000000_0x0000D3EC_phys_0
//    PhInitializeStringRefLongHint(&remainingPart, InstanceName);
//
//    PhSkipStringRef(&remainingPart, 4 * sizeof(WCHAR));
//
//    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &pidPartSr, &remainingPart))
//        return;
//
//    PhSkipStringRef(&remainingPart, 5 * sizeof(WCHAR));
//
//    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &luidHighPartSr, &remainingPart))
//        return;
//    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &luidLowPartSr, &remainingPart))
//        return;
//
//    //PhSkipStringRef(&remainingPart, 8 * sizeof(WCHAR));
//    //PhSplitStringRefAtChar(&remainingPart, L'_', &engTypePartSr, &remainingPart);
//    PhSkipStringRef(&luidHighPartSr, 2 * sizeof(WCHAR));
//    PhSkipStringRef(&luidLowPartSr, 2 * sizeof(WCHAR));
//
//    if (
//        PhStringToInteger64(&pidPartSr, 10, &processId) &&
//        PhStringToInteger64(&luidLowPartSr, 16, &engineLuidLow)
//        )
//    {
//        lookupEntry.ProcessId = (ULONG)processId;
//        lookupEntry.EngineLuid = (ULONG)engineLuidLow;
//
//        if (entry = PhFindEntryHashtable(EtGpuProcessCounterHashTable, &lookupEntry))
//        {
//            //if (entry->Value64 < InstanceValue)
//            entry->SharedUsage = InstanceValue;
//        }
//        else
//        {
//            lookupEntry.ProcessId = (ULONG)processId;
//            lookupEntry.EngineLuid = (ULONG)engineLuidLow;
//            lookupEntry.SharedUsage = InstanceValue;
//
//            PhAddEntryHashtable(EtGpuProcessCounterHashTable, &lookupEntry);
//        }
//    }
//}
//
//VOID ParseGpuProcessMemoryCommitUsageCounter(
//    _In_ PWSTR InstanceName,
//    _In_ ULONG64 InstanceValue
//    )
//{
//    PH_STRINGREF pidPartSr;
//    PH_STRINGREF luidHighPartSr;
//    PH_STRINGREF luidLowPartSr;
//    PH_STRINGREF remainingPart;
//    ULONG64 processId;
//    LONG64 engineLuidLow;
//    PET_GPU_ENGINE_COUNTER entry;
//    ET_GPU_ENGINE_COUNTER lookupEntry;
//
//    if (!EtGpuProcessCounterHashTable)
//        return;
//
//    // pid_1116_luid_0x00000000_0x0000D3EC_phys_0
//    PhInitializeStringRefLongHint(&remainingPart, InstanceName);
//
//    PhSkipStringRef(&remainingPart, 4 * sizeof(WCHAR));
//
//    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &pidPartSr, &remainingPart))
//        return;
//
//    PhSkipStringRef(&remainingPart, 5 * sizeof(WCHAR));
//
//    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &luidHighPartSr, &remainingPart))
//        return;
//    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &luidLowPartSr, &remainingPart))
//        return;
//
//    //PhSkipStringRef(&remainingPart, 8 * sizeof(WCHAR));
//    //PhSplitStringRefAtChar(&remainingPart, L'_', &engTypePartSr, &remainingPart);
//    //PhSkipStringRef(&luidHighPartSr, 2 * sizeof(WCHAR));
//    PhSkipStringRef(&luidLowPartSr, 2 * sizeof(WCHAR));
//
//    if (
//        PhStringToInteger64(&pidPartSr, 10, &processId) &&
//        PhStringToInteger64(&luidLowPartSr, 16, &engineLuidLow)
//        )
//    {
//        lookupEntry.ProcessId = (ULONG)processId;
//        lookupEntry.EngineLuid = (ULONG)engineLuidLow;
//
//        if (entry = PhFindEntryHashtable(EtGpuProcessCounterHashTable, &lookupEntry))
//        {
//            //if (entry->Value64 < InstanceValue)
//            entry->Value64 = InstanceValue;
//        }
//        else
//        {
//            lookupEntry.ProcessId = (ULONG)processId;
//            lookupEntry.EngineLuid = (ULONG)engineLuidLow;
//            lookupEntry.Value64 = InstanceValue;
//
//            PhAddEntryHashtable(EtGpuProcessCounterHashTable, &lookupEntry);
//        }
//    }
//}
//
//VOID ParseGpuAdapterDedicatedUsageCounter(
//    _In_ PWSTR InstanceName,
//    _In_ ULONG64 InstanceValue
//    )
//{
//    PH_STRINGREF luidHighPartSr;
//    PH_STRINGREF luidLowPartSr;
//    PH_STRINGREF remainingPart;
//    LONG64 engineLuidLow;
//    PET_GPU_ADAPTER_COUNTER entry;
//    ET_GPU_ADAPTER_COUNTER lookupEntry;
//
//    if (!EtGpuAdapterDedicatedHashTable)
//        return;
//
//    // luid_0x00000000_0x0000C4CF_phys_0
//    PhInitializeStringRefLongHint(&remainingPart, InstanceName);
//
//    PhSkipStringRef(&remainingPart, 5 * sizeof(WCHAR));
//
//    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &luidHighPartSr, &remainingPart))
//        return;
//    if (!PhSplitStringRefAtChar(&remainingPart, L'_', &luidLowPartSr, &remainingPart))
//        return;
//
//    //PhSkipStringRef(&remainingPart, 8 * sizeof(WCHAR));
//    //PhSplitStringRefAtChar(&remainingPart, L'_', &engTypePartSr, &remainingPart);
//    //PhSkipStringRef(&luidHighPartSr, 2 * sizeof(WCHAR));
//    PhSkipStringRef(&luidLowPartSr, 2 * sizeof(WCHAR));
//
//    if (PhStringToInteger64(&luidLowPartSr, 16, &engineLuidLow))
//    {
//        lookupEntry.EngineLuid = (ULONG)engineLuidLow;
//
//        if (entry = PhFindEntryHashtable(EtGpuAdapterDedicatedHashTable, &lookupEntry))
//        {
//            //if (entry->Value64 < InstanceValue)
//            entry->Value64 = InstanceValue;
//        }
//        else
//        {
//            lookupEntry.EngineLuid = (ULONG)engineLuidLow;
//            lookupEntry.Value64 = InstanceValue;
//
//            PhAddEntryHashtable(EtGpuAdapterDedicatedHashTable, &lookupEntry);
//        }
//    }
//}
//
//_Success_(return)
//BOOLEAN GetCounterArrayBuffer(
//    _In_ PDH_HCOUNTER CounterHandle,
//    _In_ ULONG Format,
//    _Out_ ULONG *ArrayCount,
//    _Out_ PPDH_FMT_COUNTERVALUE_ITEM *Array
//    )
//{
//    PDH_STATUS status;
//    ULONG bufferLength = 0;
//    ULONG bufferCount = 0;
//    PPDH_FMT_COUNTERVALUE_ITEM buffer = NULL;
//
//    status = PdhGetFormattedCounterArray(
//        CounterHandle,
//        Format,
//        &bufferLength,
//        &bufferCount,
//        NULL
//        );
//
//    if (status == PDH_MORE_DATA)
//    {
//        buffer = PhAllocate(bufferLength);
//
//        status = PdhGetFormattedCounterArray(
//            CounterHandle,
//            Format,
//            &bufferLength,
//            &bufferCount,
//            buffer
//            );
//    }
//
//    if (status == ERROR_SUCCESS)
//    {
//        if (ArrayCount)
//        {
//            *ArrayCount = bufferCount;
//        }
//
//        if (Array)
//        {
//            *Array = buffer;
//        }
//
//        return TRUE;
//    }
//
//    if (buffer)
//        PhFree(buffer);
//
//    return FALSE;
//}
//
//NTSTATUS NTAPI EtGpuCounterQueryThread(
//    _In_ PVOID ThreadParameter
//    )
//{
//    PDH_HQUERY gpuPerfCounterQueryHandle = NULL;
//    PDH_HCOUNTER gpuPerfCounterRunningTimeHandle = NULL;
//    PDH_HCOUNTER gpuPerfCounterDedicatedUsageHandle = NULL;
//    PDH_HCOUNTER gpuPerfCounterSharedUsageHandle = NULL;
//    PDH_HCOUNTER gpuPerfCounterCommittedUsageHandle = NULL;
//    PDH_HCOUNTER gpuPerfCounterAdapterDedicatedUsageHandle = NULL;
//    PPDH_FMT_COUNTERVALUE_ITEM buffer;
//    ULONG bufferCount;
//
//    if (PdhOpenQuery(NULL, 0, &gpuPerfCounterQueryHandle) != ERROR_SUCCESS)
//        goto CleanupExit;
//
//    // \GPU Engine(*)\Running Time
//    // \GPU Engine(*)\Utilization Percentage
//    // \GPU Process Memory(*)\Shared Usage
//    // \GPU Process Memory(*)\Dedicated Usage
//    // \GPU Process Memory(*)\Non Local Usage
//    // \GPU Process Memory(*)\Local Usage
//    // \GPU Adapter Memory(*)\Shared Usage
//    // \GPU Adapter Memory(*)\Dedicated Usage
//    // \GPU Adapter Memory(*)\Total Committed
//    // \GPU Local Adapter Memory(*)\Local Usage
//    // \GPU Non Local Adapter Memory(*)\Non Local Usage
//
//    if (PdhAddCounter(gpuPerfCounterQueryHandle, L"\\GPU Engine(*)\\Utilization Percentage", 0, &gpuPerfCounterRunningTimeHandle) != ERROR_SUCCESS)
//        goto CleanupExit;
//    if (PdhAddCounter(gpuPerfCounterQueryHandle, L"\\GPU Process Memory(*)\\Shared Usage", 0, &gpuPerfCounterSharedUsageHandle) != ERROR_SUCCESS)
//        goto CleanupExit;
//    if (PdhAddCounter(gpuPerfCounterQueryHandle, L"\\GPU Process Memory(*)\\Dedicated Usage", 0, &gpuPerfCounterDedicatedUsageHandle) != ERROR_SUCCESS)
//        goto CleanupExit;
//    if (PdhAddCounter(gpuPerfCounterQueryHandle, L"\\GPU Process Memory(*)\\Total Committed", 0, &gpuPerfCounterCommittedUsageHandle) != ERROR_SUCCESS)
//        goto CleanupExit;
//    if (PdhAddCounter(gpuPerfCounterQueryHandle, L"\\GPU Adapter Memory(*)\\Dedicated Usage", 0, &gpuPerfCounterAdapterDedicatedUsageHandle) != ERROR_SUCCESS)
//        goto CleanupExit;
//
//    for (;;)
//    {
//        if (PdhCollectQueryData(gpuPerfCounterQueryHandle) != ERROR_SUCCESS)
//            goto CleanupExit;
//
//        EtGpuResetHashtables();
//
//        EtGpuHashTableAcquireLockExclusive();
//
//        if (EtGpuRunningTimeHashTable)
//        {
//            if (GetCounterArrayBuffer(
//                gpuPerfCounterRunningTimeHandle,
//                PDH_FMT_DOUBLE,
//                &bufferCount,
//                &buffer
//                ))
//            {
//                for (ULONG i = 0; i < bufferCount; i++)
//                {
//                    PPDH_FMT_COUNTERVALUE_ITEM entry = PTR_ADD_OFFSET(buffer, sizeof(PDH_FMT_COUNTERVALUE_ITEM) * i);
//
//                    if (entry->FmtValue.CStatus)
//                        continue;
//                    if (entry->FmtValue.doubleValue == 0.0)
//                        continue;
//
//                    ParseGpuEngineUtilizationCounter(entry->szName, entry->FmtValue.doubleValue);
//                }
//
//                PhFree(buffer);
//            }
//        }
//
//        if (EtGpuProcessCounterHashTable)
//        {
//            if (GetCounterArrayBuffer(
//                gpuPerfCounterDedicatedUsageHandle,
//                PDH_FMT_LARGE,
//                &bufferCount,
//                &buffer
//                ))
//            {
//                for (ULONG i = 0; i < bufferCount; i++)
//                {
//                    PPDH_FMT_COUNTERVALUE_ITEM entry = PTR_ADD_OFFSET(buffer, sizeof(PDH_FMT_COUNTERVALUE_ITEM) * i);
//
//                    if (entry->FmtValue.CStatus)
//                        continue;
//                    if (entry->FmtValue.largeValue == 0)
//                        continue;
//
//                    ParseGpuProcessMemoryDedicatedUsageCounter(entry->szName, entry->FmtValue.largeValue);
//                }
//
//                PhFree(buffer);
//            }
//
//            if (GetCounterArrayBuffer(
//                gpuPerfCounterSharedUsageHandle,
//                PDH_FMT_LARGE,
//                &bufferCount,
//                &buffer
//                ))
//            {
//                for (ULONG i = 0; i < bufferCount; i++)
//                {
//                    PPDH_FMT_COUNTERVALUE_ITEM entry = PTR_ADD_OFFSET(buffer, sizeof(PDH_FMT_COUNTERVALUE_ITEM) * i);
//
//                    if (entry->FmtValue.CStatus)
//                        continue;
//                    if (entry->FmtValue.largeValue == 0)
//                        continue;
//
//                    ParseGpuProcessMemorySharedUsageCounter(entry->szName, entry->FmtValue.largeValue);
//                }
//
//                PhFree(buffer);
//            }
//
//            if (GetCounterArrayBuffer(
//                gpuPerfCounterCommittedUsageHandle,
//                PDH_FMT_LARGE,
//                &bufferCount,
//                &buffer
//                ))
//            {
//                for (ULONG i = 0; i < bufferCount; i++)
//                {
//                    PPDH_FMT_COUNTERVALUE_ITEM entry = PTR_ADD_OFFSET(buffer, sizeof(PDH_FMT_COUNTERVALUE_ITEM)* i);
//
//                    if (entry->FmtValue.CStatus)
//                        continue;
//                    if (entry->FmtValue.largeValue == 0)
//                        continue;
//
//                    ParseGpuProcessMemoryCommitUsageCounter(entry->szName, entry->FmtValue.largeValue);
//                }
//
//                PhFree(buffer);
//            }
//        }
//
//        if (EtGpuAdapterDedicatedHashTable)
//        {
//            if (GetCounterArrayBuffer(
//                gpuPerfCounterAdapterDedicatedUsageHandle,
//                PDH_FMT_LARGE,
//                &bufferCount,
//                &buffer
//                ))
//            {
//                for (ULONG i = 0; i < bufferCount; i++)
//                {
//                    PPDH_FMT_COUNTERVALUE_ITEM entry = PTR_ADD_OFFSET(buffer, sizeof(PDH_FMT_COUNTERVALUE_ITEM) * i);
//
//                    if (entry->FmtValue.CStatus)
//                        continue;
//                    if (entry->FmtValue.largeValue == 0)
//                        continue;
//
//                    ParseGpuAdapterDedicatedUsageCounter(entry->szName, entry->FmtValue.largeValue);
//                }
//
//                PhFree(buffer);
//            }
//        }
//
//        EtGpuHashTableReleaseLockExclusive();
//
//        PhDelayExecution(1000);
//    }
//
//CleanupExit:
//
//    if (gpuPerfCounterQueryHandle)
//        PdhCloseQuery(gpuPerfCounterQueryHandle);
//
//    return STATUS_SUCCESS;
//}
