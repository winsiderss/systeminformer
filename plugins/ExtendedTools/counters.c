/*
 * Process Hacker .NET Tools -
 *   GPU performance counters
 *
 * Copyright (C) 2019-2020 dmex
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

#include "exttools.h"
#include <winperf.h>
#include <pdh.h>
#include <pdhmsg.h>

NTSTATUS NTAPI EtGpuCounterQueryThread(
    _In_ PVOID ThreadParameter
    );

PPH_HASHTABLE EtGpuRunningTimeHashTable = NULL;
PH_FAST_LOCK EtGpuRunningTimeHashTableLock = PH_FAST_LOCK_INIT;
PPH_HASHTABLE EtGpuDedicatedHashTable = NULL;
PH_FAST_LOCK EtGpuDedicatedHashTableLock = PH_FAST_LOCK_INIT;
PPH_HASHTABLE EtGpuSharedHashTable = NULL;
PH_FAST_LOCK EtGpuSharedHashTableLock = PH_FAST_LOCK_INIT;
PPH_HASHTABLE EtGpuCommitHashTable = NULL;
PH_FAST_LOCK EtGpuCommitHashTableLock = PH_FAST_LOCK_INIT;

typedef struct _ET_GPU_COUNTER
{
    ULONG64 Node;
    HANDLE ProcessId;
    ULONG64 EngineId;

    union
    {
        DOUBLE Value;
        struct
        {
            ULONG64 Value64;
            FLOAT ValueF;
        };
    };
} ET_GPU_COUNTER, * PET_GPU_COUNTER;

//typedef struct _ET_COUNTER_CONFIG
//{
//    PWSTR Path;
//    PDH_HCOUNTER Handle;
//} ET_COUNTER_CONFIG, * PET_COUNTER_CONFIG;
//
//static ET_COUNTER_CONFIG CounterConfigArray[] =
//{
//    { L"\\GPU Engine(*)\\Running Time", NULL },
//    { L"\\GPU Process Memory(*)\\Dedicated Usage", NULL },
//    { L"\\GPU Process Memory(*)\\Shared Usage", NULL },
//};

static BOOLEAN NTAPI EtpDedicatedEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PET_GPU_COUNTER entry1 = Entry1;
    PET_GPU_COUNTER entry2 = Entry2;

    return entry1->ProcessId == entry2->ProcessId;
}

static ULONG NTAPI EtpDedicatedHashFunction(
    _In_ PVOID Entry
    )
{
    PET_GPU_COUNTER entry = Entry;

    return PhHashIntPtr((ULONG_PTR)entry->ProcessId);
}

VOID EtGpuCountersInitialization(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        EtGpuRunningTimeHashTable = PhCreateHashtable(
            sizeof(ET_GPU_COUNTER),
            EtpDedicatedEqualFunction,
            EtpDedicatedHashFunction,
            10
            );
        EtGpuDedicatedHashTable = PhCreateHashtable(
            sizeof(ET_GPU_COUNTER),
            EtpDedicatedEqualFunction,
            EtpDedicatedHashFunction,
            10
            );
        EtGpuSharedHashTable = PhCreateHashtable(
            sizeof(ET_GPU_COUNTER),
            EtpDedicatedEqualFunction,
            EtpDedicatedHashFunction,
            10
            );
        EtGpuCommitHashTable = PhCreateHashtable(
            sizeof(ET_GPU_COUNTER),
            EtpDedicatedEqualFunction,
            EtpDedicatedHashFunction,
            10
            );

        PhCreateThread2(EtGpuCounterQueryThread, NULL);

        PhEndInitOnce(&initOnce);
    }
}

FLOAT EtLookupProcessGpuUtilization(
    _In_ HANDLE ProcessId
    )
{
    ET_GPU_COUNTER lookupEntry;
    PET_GPU_COUNTER entry;
    FLOAT value = 0;

    if (!EtGpuRunningTimeHashTable)
    {
        EtGpuCountersInitialization();
        return 0;
    }

    PhAcquireFastLockShared(&EtGpuRunningTimeHashTableLock);

    lookupEntry.ProcessId = ProcessId;

    if (entry = PhFindEntryHashtable(EtGpuRunningTimeHashTable, &lookupEntry))
    {
        FLOAT usage = (FLOAT)entry->Value;

        if (usage > value)
            value = usage;
    }

    PhReleaseFastLockShared(&EtGpuRunningTimeHashTableLock);

    if (value > 0)
        value = value / 100;

    return value;
}

FLOAT EtLookupTotalGpuUtilization(
    VOID
    )
{
    FLOAT value = 0;
    ULONG enumerationKey;
    PET_GPU_COUNTER entry;

    if (!EtGpuRunningTimeHashTable)
    {
        EtGpuCountersInitialization();
        return 0;
    }

    PhAcquireFastLockShared(&EtGpuRunningTimeHashTableLock);

    enumerationKey = 0;

    while (PhEnumHashtable(EtGpuRunningTimeHashTable, (PVOID*)&entry, &enumerationKey))
    {
        FLOAT usage = (FLOAT)entry->Value;

        if (usage > value)
            value = usage;
    }

    PhReleaseFastLockShared(&EtGpuRunningTimeHashTableLock);

    if (value > 0)
        value = value / 100;

    return value;
}

FLOAT EtLookupTotalGpuEngineUtilization(
    _In_ ULONG64 EngineId
    )
{
    FLOAT value = 0;
    ULONG enumerationKey;
    PET_GPU_COUNTER entry;

    if (!EtGpuRunningTimeHashTable)
    {
        EtGpuCountersInitialization();
        return 0;
    }

    PhAcquireFastLockShared(&EtGpuRunningTimeHashTableLock);

    enumerationKey = 0;

    while (PhEnumHashtable(EtGpuRunningTimeHashTable, (PVOID*)&entry, &enumerationKey))
    {
        if (entry->EngineId != EngineId)
            continue;

        FLOAT usage = (FLOAT)entry->Value;

        if (usage > value)
            value = usage;
    }

    PhReleaseFastLockShared(&EtGpuRunningTimeHashTableLock);

    if (value > 0)
        value = value / 100;

    return value;
}

ULONG64 EtLookupProcessGpuDedicated(
    _In_opt_ HANDLE ProcessId
    )
{
    ET_GPU_COUNTER lookupEntry;
    PET_GPU_COUNTER entry;
    ULONG64 value = 0;

    if (!EtGpuDedicatedHashTable)
    {
        EtGpuCountersInitialization();
        return 0;
    }

    PhAcquireFastLockShared(&EtGpuDedicatedHashTableLock);

    lookupEntry.ProcessId = ProcessId;

    if (entry = PhFindEntryHashtable(EtGpuDedicatedHashTable, &lookupEntry))
    {
        value = entry->Value64;
    }

    PhReleaseFastLockShared(&EtGpuDedicatedHashTableLock);

    return value;
}

ULONG64 EtLookupTotalGpuDedicated(
    VOID
    )
{
    ULONG64 value = 0;
    ULONG enumerationKey;
    PET_GPU_COUNTER entry;

    if (!EtGpuDedicatedHashTable)
    {
        EtGpuCountersInitialization();
        return 0;
    }

    PhAcquireFastLockShared(&EtGpuDedicatedHashTableLock);

    enumerationKey = 0;

    while (PhEnumHashtable(EtGpuDedicatedHashTable, (PVOID*)&entry, &enumerationKey))
    {
        value += entry->Value64;
    }

    PhReleaseFastLockShared(&EtGpuDedicatedHashTableLock);

    return value;
}

ULONG64 EtLookupProcessGpuSharedUsage(
    _In_opt_ HANDLE ProcessId
    )
{
    ET_GPU_COUNTER lookupEntry;
    PET_GPU_COUNTER entry;
    ULONG64 value = 0;

    if (!EtGpuSharedHashTable)
    {
        EtGpuCountersInitialization();
        return 0;
    }

    lookupEntry.ProcessId = ProcessId;

    PhAcquireFastLockShared(&EtGpuSharedHashTableLock);

    if (entry = PhFindEntryHashtable(EtGpuSharedHashTable, &lookupEntry))
    {
        value = entry->Value64;
    }

    PhReleaseFastLockShared(&EtGpuSharedHashTableLock);

    return value;
}

ULONG64 EtLookupTotalGpuShared(
    VOID
    )
{
    ULONG64 value = 0;
    ULONG enumerationKey;
    PET_GPU_COUNTER entry;

    if (!EtGpuSharedHashTable)
    {
        EtGpuCountersInitialization();
        return 0;
    }

    PhAcquireFastLockShared(&EtGpuSharedHashTableLock);

    enumerationKey = 0;

    while (PhEnumHashtable(EtGpuSharedHashTable, (PVOID*)&entry, &enumerationKey))
    {
        value += entry->Value64;
    }

    PhReleaseFastLockShared(&EtGpuSharedHashTableLock);

    return value;
}

ULONG64 EtLookupProcessGpuCommitUsage(
    _In_opt_ HANDLE ProcessId
    )
{  
    ET_GPU_COUNTER lookupEntry;
    PET_GPU_COUNTER entry;
    ULONG64 value = 0;

    if (!EtGpuCommitHashTable)
    {
        EtGpuCountersInitialization();
        return 0;
    }

    lookupEntry.ProcessId = ProcessId;

    PhAcquireFastLockShared(&EtGpuCommitHashTableLock);

    if (entry = PhFindEntryHashtable(EtGpuCommitHashTable, &lookupEntry))
    {
        value = entry->Value64;
    }

    PhReleaseFastLockShared(&EtGpuCommitHashTableLock);

    return value;
}

ULONG64 EtLookupTotalGpuCommit(
    VOID
    )
{
    ULONG64 value = 0;
    ULONG enumerationKey;
    PET_GPU_COUNTER entry;

    if (!EtGpuCommitHashTable)
    {
        EtGpuCountersInitialization();
        return 0;
    }

    PhAcquireFastLockShared(&EtGpuCommitHashTableLock);

    enumerationKey = 0;

    while (PhEnumHashtable(EtGpuCommitHashTable, (PVOID*)&entry, &enumerationKey))
    {
        value += entry->Value64;
    }

    PhReleaseFastLockShared(&EtGpuCommitHashTableLock);

    return value;
}

VOID ParseGpuEngineUtilizationCounter(
    _In_ PWSTR InstanceName,
    _In_ DOUBLE InstanceValue
    )
{
    PH_STRINGREF partSr;
    PH_STRINGREF pidPartSr;
    PH_STRINGREF luidLowPartSr;
    PH_STRINGREF luidHighPartSr;
    PH_STRINGREF physPartSr;
    PH_STRINGREF engPartSr;
    PH_STRINGREF engTypePartSr;
    PH_STRINGREF remainingPart;

    if (!EtGpuRunningTimeHashTable)
        return;

    // pid_12704_luid_0x00000000_0x0000D503_phys_0_eng_3_engtype_VideoDecode
    PhInitializeStringRefLongHint(&remainingPart, InstanceName);

    if (remainingPart.Length == 0)
        return;

    PhSplitStringRefAtChar(&remainingPart, L'_', &partSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &pidPartSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &partSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &luidLowPartSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &luidHighPartSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &partSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &physPartSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &partSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &engPartSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &partSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &engTypePartSr, &remainingPart);

    if (pidPartSr.Length)
    {
        ULONG64 processId;
        ULONG64 engineId;
        PET_GPU_COUNTER entry;
        ET_GPU_COUNTER lookupEntry;

        if (
            PhStringToInteger64(&pidPartSr, 10, &processId) &&
            PhStringToInteger64(&engPartSr, 10, &engineId)
            )
        {
            lookupEntry.ProcessId = (HANDLE)processId;

            if (entry = PhFindEntryHashtable(EtGpuRunningTimeHashTable, &lookupEntry))
            {
                if (entry->Value < InstanceValue)
                    entry->Value = InstanceValue;

                entry->EngineId = engineId;
            }
            else
            {
                lookupEntry.ProcessId = (HANDLE)processId;
                lookupEntry.EngineId = engineId;
                lookupEntry.Value = InstanceValue;

                PhAddEntryHashtable(EtGpuRunningTimeHashTable, &lookupEntry);
            }
        }
    }
}

VOID ParseGpuProcessMemoryDedicatedUsageCounter(
    _In_ PWSTR InstanceName,
    _In_ ULONG64 InstanceValue
    )
{
    PH_STRINGREF partSr;
    PH_STRINGREF pidPartSr;
    PH_STRINGREF luidLowPartSr;
    PH_STRINGREF luidHighPartSr;
    PH_STRINGREF physPartSr;
    PH_STRINGREF remainingPart;

    if (!EtGpuDedicatedHashTable)
        return;

    // pid_1116_luid_0x00000000_0x0000D3EC_phys_0
    PhInitializeStringRefLongHint(&remainingPart, InstanceName);
    PhSplitStringRefAtChar(&remainingPart, L'_', &partSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &pidPartSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &partSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &luidLowPartSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &luidHighPartSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &partSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &physPartSr, &remainingPart);

    if (pidPartSr.Length)
    {
        ULONG64 processId;
        PET_GPU_COUNTER entry;
        ET_GPU_COUNTER lookupEntry;

        if (PhStringToInteger64(&pidPartSr, 10, &processId))
        {
            lookupEntry.ProcessId = (HANDLE)processId;

            if (entry = PhFindEntryHashtable(EtGpuDedicatedHashTable, &lookupEntry))
            {
                if (entry->Value64 < InstanceValue)
                    entry->Value64 = InstanceValue;
            }
            else
            {
                lookupEntry.ProcessId = (HANDLE)processId;
                lookupEntry.Value64 = InstanceValue;

                PhAddEntryHashtable(EtGpuDedicatedHashTable, &lookupEntry);
            }
        }
    }
}

VOID ParseGpuProcessMemorySharedUsageCounter(
    _In_ PWSTR InstanceName,
    _In_ ULONG64 InstanceValue
    )
{
    PH_STRINGREF partSr;
    PH_STRINGREF pidPartSr;
    PH_STRINGREF luidLowPartSr;
    PH_STRINGREF luidHighPartSr;
    PH_STRINGREF physPartSr;
    PH_STRINGREF remainingPart;

    if (!EtGpuSharedHashTable)
        return;

    // pid_1116_luid_0x00000000_0x0000D3EC_phys_0
    PhInitializeStringRefLongHint(&remainingPart, InstanceName);
    PhSplitStringRefAtChar(&remainingPart, L'_', &partSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &pidPartSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &partSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &luidLowPartSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &luidHighPartSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &partSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &physPartSr, &remainingPart);

    if (pidPartSr.Length)
    {
        ULONG64 processId;
        PET_GPU_COUNTER entry;
        ET_GPU_COUNTER lookupEntry;

        if (PhStringToInteger64(&pidPartSr, 10, &processId))
        {
            lookupEntry.ProcessId = (HANDLE)processId;

            if (entry = PhFindEntryHashtable(EtGpuSharedHashTable, &lookupEntry))
            {
                if (entry->Value64 < InstanceValue)
                    entry->Value64 = InstanceValue;
            }
            else
            {
                lookupEntry.ProcessId = (HANDLE)processId;
                lookupEntry.Value64 = InstanceValue;

                PhAddEntryHashtable(EtGpuSharedHashTable, &lookupEntry);
            }
        }
    }
}

VOID ParseGpuProcessMemoryCommitUsageCounter(
    _In_ PWSTR InstanceName,
    _In_ ULONG64 InstanceValue
    )
{
    PH_STRINGREF partSr;
    PH_STRINGREF pidPartSr;
    PH_STRINGREF luidLowPartSr;
    PH_STRINGREF luidHighPartSr;
    PH_STRINGREF physPartSr;
    PH_STRINGREF remainingPart;

    if (!EtGpuCommitHashTable)
        return;

    // pid_1116_luid_0x00000000_0x0000D3EC_phys_0
    PhInitializeStringRefLongHint(&remainingPart, InstanceName);
    PhSplitStringRefAtChar(&remainingPart, L'_', &partSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &pidPartSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &partSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &luidLowPartSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &luidHighPartSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &partSr, &remainingPart);
    PhSplitStringRefAtChar(&remainingPart, L'_', &physPartSr, &remainingPart);

    if (pidPartSr.Length)
    {
        ULONG64 processId;
        PET_GPU_COUNTER entry;
        ET_GPU_COUNTER lookupEntry;

        if (PhStringToInteger64(&pidPartSr, 10, &processId))
        {
            lookupEntry.ProcessId = (HANDLE)processId;

            if (entry = PhFindEntryHashtable(EtGpuCommitHashTable, &lookupEntry))
            {
                if (entry->Value64 < InstanceValue)
                    entry->Value64 = InstanceValue;
            }
            else
            {
                lookupEntry.ProcessId = (HANDLE)processId;
                lookupEntry.Value64 = InstanceValue;

                PhAddEntryHashtable(EtGpuCommitHashTable, &lookupEntry);
            }
        }
    }
}

_Success_(return)
BOOLEAN GetCounterArrayBuffer(
    _In_ PDH_HCOUNTER CounterHandle,
    _In_ ULONG Format,
    _Out_ ULONG *ArrayCount,
    _Out_ PPDH_FMT_COUNTERVALUE_ITEM *Array
    )
{
    PDH_STATUS status;
    ULONG bufferLength = 0;
    ULONG bufferCount = 0;
    PPDH_FMT_COUNTERVALUE_ITEM buffer = NULL;

    status = PdhGetFormattedCounterArray(
        CounterHandle,
        Format,
        &bufferLength,
        &bufferCount,
        NULL
        );

    if (status == PDH_MORE_DATA)
    {
        buffer = PhAllocate(bufferLength);

        status = PdhGetFormattedCounterArray(
            CounterHandle,
            Format,
            &bufferLength,
            &bufferCount,
            buffer
            );
    }

    if (status == ERROR_SUCCESS)
    {
        if (ArrayCount)
        {
            *ArrayCount = bufferCount;
        }

        if (Array)
        {
            *Array = buffer;
        }

        return TRUE;
    }

    if (buffer)
        PhFree(buffer);

    return FALSE;
}

NTSTATUS NTAPI EtGpuCounterQueryThread(
    _In_ PVOID ThreadParameter
    )
{
    HANDLE gpuCounterQueryEvent = NULL;
    PDH_HQUERY gpuPerfCounterQueryHandle = NULL;
    PDH_HCOUNTER gpuPerfCounterRunningTimeHandle = NULL;
    PDH_HCOUNTER gpuPerfCounterDedicatedUsageHandle = NULL;
    PDH_HCOUNTER gpuPerfCounterSharedUsageHandle = NULL;
    PDH_HCOUNTER gpuPerfCounterCommittedUsageHandle = NULL;
    PPDH_FMT_COUNTERVALUE_ITEM buffer;
    ULONG bufferCount;

    if (!NT_SUCCESS(NtCreateEvent(&gpuCounterQueryEvent, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE)))
        goto CleanupExit;

    if (PdhOpenQuery(NULL, 0, &gpuPerfCounterQueryHandle) != ERROR_SUCCESS)
        goto CleanupExit;

    // \GPU Engine(*)\Running Time
    // \GPU Engine(*)\Utilization Percentage                                                           
    // \GPU Process Memory(*)\Shared Usage
    // \GPU Process Memory(*)\Dedicated Usage
    // \GPU Process Memory(*)\Non Local Usage
    // \GPU Process Memory(*)\Local Usage
    // \GPU Adapter Memory(*)\Shared Usage
    // \GPU Adapter Memory(*)\Dedicated Usage
    // \GPU Adapter Memory(*)\Total Committed
    // \GPU Local Adapter Memory(*)\Local Usage
    // \GPU Non Local Adapter Memory(*)\Non Local Usage

    if (PdhAddCounter(gpuPerfCounterQueryHandle, L"\\GPU Engine(*)\\Utilization Percentage", 0, &gpuPerfCounterRunningTimeHandle) != ERROR_SUCCESS)
        goto CleanupExit;
    if (PdhAddCounter(gpuPerfCounterQueryHandle, L"\\GPU Process Memory(*)\\Shared Usage", 0, &gpuPerfCounterSharedUsageHandle) != ERROR_SUCCESS)
        goto CleanupExit;
    if (PdhAddCounter(gpuPerfCounterQueryHandle, L"\\GPU Process Memory(*)\\Dedicated Usage", 0, &gpuPerfCounterDedicatedUsageHandle) != ERROR_SUCCESS)
        goto CleanupExit;
    if (PdhAddCounter(gpuPerfCounterQueryHandle, L"\\GPU Process Memory(*)\\Total Committed", 0, &gpuPerfCounterCommittedUsageHandle) != ERROR_SUCCESS)
        goto CleanupExit;

    if (PdhCollectQueryDataEx(gpuPerfCounterQueryHandle, 1, gpuCounterQueryEvent) != ERROR_SUCCESS)
        goto CleanupExit;

    for (;;)
    {
        if (NtWaitForSingleObject(gpuCounterQueryEvent, FALSE, NULL) != WAIT_OBJECT_0)
            break;

        if (EtGpuRunningTimeHashTable)
        {
            if (GetCounterArrayBuffer(
                gpuPerfCounterRunningTimeHandle,
                PDH_FMT_DOUBLE,
                &bufferCount,
                &buffer
                ))
            {
                PhAcquireFastLockExclusive(&EtGpuRunningTimeHashTableLock);

                PhClearHashtable(EtGpuRunningTimeHashTable);

                for (ULONG i = 0; i < bufferCount; i++)
                {
                    PPDH_FMT_COUNTERVALUE_ITEM entry = PTR_ADD_OFFSET(buffer, sizeof(PDH_FMT_COUNTERVALUE_ITEM) * i);

                    if (entry->FmtValue.CStatus)
                        continue;
                    if (entry->FmtValue.doubleValue == 0)
                        continue;

                    ParseGpuEngineUtilizationCounter(entry->szName, entry->FmtValue.doubleValue);
                }

                PhReleaseFastLockExclusive(&EtGpuRunningTimeHashTableLock);

                PhFree(buffer);
            }
        }

        if (EtGpuDedicatedHashTable)
        {
            if (GetCounterArrayBuffer(
                gpuPerfCounterDedicatedUsageHandle,
                PDH_FMT_LARGE,
                &bufferCount,
                &buffer
                ))
            {
                PhAcquireFastLockExclusive(&EtGpuDedicatedHashTableLock);

                // Reset hashtable once in a while.
                {
                    static ULONG64 lastTickCount = 0;
                    ULONG64 tickCount;

                    tickCount = NtGetTickCount64();

                    if (tickCount - lastTickCount >= 10 * CLOCKS_PER_SEC)
                    {
                        PhClearHashtable(EtGpuDedicatedHashTable);
                        lastTickCount = tickCount;
                    }
                }

                for (ULONG i = 0; i < bufferCount; i++)
                {
                    PPDH_FMT_COUNTERVALUE_ITEM entry = PTR_ADD_OFFSET(buffer, sizeof(PDH_FMT_COUNTERVALUE_ITEM) * i);

                    if (entry->FmtValue.CStatus)
                        continue;
                    if (entry->FmtValue.largeValue == 0)
                        continue;

                    ParseGpuProcessMemoryDedicatedUsageCounter(entry->szName, entry->FmtValue.largeValue);
                }

                PhReleaseFastLockExclusive(&EtGpuDedicatedHashTableLock);

                PhFree(buffer);
            }
        }

        if (EtGpuSharedHashTable)
        {
            if (GetCounterArrayBuffer(
                gpuPerfCounterSharedUsageHandle,
                PDH_FMT_LARGE,
                &bufferCount,
                &buffer
                ))
            {
                PhAcquireFastLockExclusive(&EtGpuSharedHashTableLock);

                // Reset hashtable once in a while.
                {
                    static ULONG64 lastTickCount = 0;
                    ULONG64 tickCount;

                    tickCount = NtGetTickCount64();

                    if (tickCount - lastTickCount >= 10 * CLOCKS_PER_SEC)
                    {
                        PhClearHashtable(EtGpuSharedHashTable);
                        lastTickCount = tickCount;
                    }
                }

                for (ULONG i = 0; i < bufferCount; i++)
                {
                    PPDH_FMT_COUNTERVALUE_ITEM entry = PTR_ADD_OFFSET(buffer, sizeof(PDH_FMT_COUNTERVALUE_ITEM) * i);

                    if (entry->FmtValue.CStatus)
                        continue;
                    if (entry->FmtValue.largeValue == 0)
                        continue;

                    ParseGpuProcessMemorySharedUsageCounter(entry->szName, entry->FmtValue.largeValue);
                }

                PhReleaseFastLockExclusive(&EtGpuSharedHashTableLock);

                PhFree(buffer);
            }
        }

        if (EtGpuCommitHashTable)
        {
            if (GetCounterArrayBuffer(
                gpuPerfCounterCommittedUsageHandle,
                PDH_FMT_LARGE,
                &bufferCount,
                &buffer
                ))
            {
                PhAcquireFastLockExclusive(&EtGpuCommitHashTableLock);

                // Reset hashtable once in a while.
                {
                    static ULONG64 lastTickCount = 0;
                    ULONG64 tickCount;

                    tickCount = NtGetTickCount64();

                    if (tickCount - lastTickCount >= 10 * 1000)
                    {
                        PhClearHashtable(EtGpuCommitHashTable);
                        lastTickCount = tickCount;
                    }
                }

                for (ULONG i = 0; i < bufferCount; i++)
                {
                    PPDH_FMT_COUNTERVALUE_ITEM entry = PTR_ADD_OFFSET(buffer, sizeof(PDH_FMT_COUNTERVALUE_ITEM)* i);

                    if (entry->FmtValue.CStatus)
                        continue;
                    if (entry->FmtValue.largeValue == 0)
                        continue;

                    ParseGpuProcessMemoryCommitUsageCounter(entry->szName, entry->FmtValue.largeValue);
                }

                PhReleaseFastLockExclusive(&EtGpuCommitHashTableLock);

                PhFree(buffer);
            }
        }
    }

CleanupExit:

    if (gpuPerfCounterQueryHandle)
        PdhCloseQuery(gpuPerfCounterQueryHandle);
    if (gpuCounterQueryEvent)
        NtClose(gpuCounterQueryEvent);

    return STATUS_SUCCESS;
}
