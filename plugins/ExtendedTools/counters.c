/*
 * Process Hacker .NET Tools -
 *   GPU performance counters
 *
 * Copyright (C) 2019 dmex
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
PH_QUEUED_LOCK EtGpuRunningTimeHashTableLock = PH_QUEUED_LOCK_INIT;
PPH_HASHTABLE EtGpuDedicatedHashTable = NULL;
PH_QUEUED_LOCK EtGpuDedicatedHashTableLock = PH_QUEUED_LOCK_INIT;
PPH_HASHTABLE EtGpuSharedHashTable = NULL;
PH_QUEUED_LOCK EtGpuSharedHashTableLock = PH_QUEUED_LOCK_INIT;
PPH_HASHTABLE EtGpuCommitHashTable = NULL;
PH_QUEUED_LOCK EtGpuCommitHashTableLock = PH_QUEUED_LOCK_INIT;

HANDLE GpuCounterTimerHandle = NULL;
PDH_HQUERY GpuPerfCounterQueryHandle = NULL;
PDH_HCOUNTER GpuPerfCounterRunningTimeHandle = NULL;
PDH_HCOUNTER GpuPerfCounterDedicatedUsageHandle = NULL;
PDH_HCOUNTER GpuPerfCounterSharedUsageHandle = NULL;
PDH_HCOUNTER GpuPerfCounterNonLocalUsageHandle = NULL;
PPDH_FMT_COUNTERVALUE_ITEM buffer = NULL;
ULONG bufferCount = 0;

typedef struct _ET_GPU_COUNTER
{
    HANDLE ProcessId;
    ULONG64 Value;
    ULONG64 Node;
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

ULONG64 EtLookupProcessGpuEngineUtilization(
    _In_opt_ HANDLE ProcessId
    )
{
    ET_GPU_COUNTER lookupEntry;
    PET_GPU_COUNTER entry;
    ULONG64 value = 0;

    if (!EtGpuRunningTimeHashTable)
    {
        EtGpuCountersInitialization();
        return 0;
    }

    lookupEntry.ProcessId = ProcessId;

    PhAcquireQueuedLockShared(&EtGpuRunningTimeHashTableLock);

    if (entry = PhFindEntryHashtable(EtGpuRunningTimeHashTable, &lookupEntry))
    {
        value = entry->Value;
    }

    PhReleaseQueuedLockShared(&EtGpuRunningTimeHashTableLock);

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

    lookupEntry.ProcessId = ProcessId;

    PhAcquireQueuedLockShared(&EtGpuDedicatedHashTableLock);

    if (entry = PhFindEntryHashtable(EtGpuDedicatedHashTable, &lookupEntry))
    {
        value = entry->Value;
    }

    PhReleaseQueuedLockShared(&EtGpuDedicatedHashTableLock);

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

    PhAcquireQueuedLockShared(&EtGpuSharedHashTableLock);

    if (entry = PhFindEntryHashtable(EtGpuSharedHashTable, &lookupEntry))
    {
        value = entry->Value;
    }

    PhReleaseQueuedLockShared(&EtGpuSharedHashTableLock);

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

    PhAcquireQueuedLockShared(&EtGpuCommitHashTableLock);

    if (entry = PhFindEntryHashtable(EtGpuCommitHashTable, &lookupEntry))
    {
        value = entry->Value;
    }

    PhReleaseQueuedLockShared(&EtGpuCommitHashTableLock);

    return value;
}

VOID ParseGpuEngineUtilizationCounter(
    _In_ PWSTR InstanceName,
    _In_ ULONG64 InstanceValue
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
        PET_GPU_COUNTER entry;
        ET_GPU_COUNTER lookupEntry;

        if (PhStringToInteger64(&pidPartSr, 10, &processId))
        {
            lookupEntry.ProcessId = (HANDLE)processId;

            if (entry = PhFindEntryHashtable(EtGpuRunningTimeHashTable, &lookupEntry))
            {
                entry->Value = entry->Value + InstanceValue;
            }
            else
            {
                lookupEntry.ProcessId = (HANDLE)processId;
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
                entry->Value = entry->Value + InstanceValue;
            }
            else
            {
                lookupEntry.ProcessId = (HANDLE)processId;
                lookupEntry.Value = InstanceValue;

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
                if (entry->Value < InstanceValue)
                {
                    entry->Value = InstanceValue;
                }
            }
            else
            {
                lookupEntry.ProcessId = (HANDLE)processId;
                lookupEntry.Value = InstanceValue;

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
                if (entry->Value < InstanceValue)
                {
                    entry->Value = InstanceValue;
                }
            }
            else
            {
                lookupEntry.ProcessId = (HANDLE)processId;
                lookupEntry.Value = InstanceValue;

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

static VOID CALLBACK EtGpuCounterTimerQueueCallback(
    _In_ PVOID TimerParameter,
    _In_ BOOLEAN TimerOrWaitFired
    )
{
    if (GpuPerfCounterQueryHandle)
    {
        PdhCollectQueryData(GpuPerfCounterQueryHandle);
    }

    if (EtGpuRunningTimeHashTable)
    {
        if (GetCounterArrayBuffer(
            GpuPerfCounterRunningTimeHandle,
            PDH_FMT_LARGE,
            &bufferCount,
            &buffer
            ))
        {
            PhAcquireQueuedLockExclusive(&EtGpuRunningTimeHashTableLock);

            PhClearHashtable(EtGpuRunningTimeHashTable);

            for (ULONG i = 0; i < bufferCount; i++)
            {
                PPDH_FMT_COUNTERVALUE_ITEM entry = PTR_ADD_OFFSET(buffer, sizeof(PDH_FMT_COUNTERVALUE_ITEM) * i);

                if (entry->FmtValue.CStatus)
                    continue;
                if (entry->FmtValue.largeValue == 0)
                    continue;

                ParseGpuEngineUtilizationCounter(entry->szName, entry->FmtValue.largeValue);
            }

            PhReleaseQueuedLockExclusive(&EtGpuRunningTimeHashTableLock);

            PhFree(buffer);
        }
    }

    if (EtGpuDedicatedHashTable)
    {
        if (GetCounterArrayBuffer(
            GpuPerfCounterDedicatedUsageHandle,
            PDH_FMT_LARGE,
            &bufferCount,
            &buffer
            ))
        {
            PhAcquireQueuedLockExclusive(&EtGpuDedicatedHashTableLock);

            PhClearHashtable(EtGpuDedicatedHashTable);

            for (ULONG i = 0; i < bufferCount; i++)
            {
                PPDH_FMT_COUNTERVALUE_ITEM entry = PTR_ADD_OFFSET(buffer, sizeof(PDH_FMT_COUNTERVALUE_ITEM) * i);

                if (entry->FmtValue.CStatus)
                    continue;
                if (entry->FmtValue.largeValue == 0)
                    continue;

                ParseGpuProcessMemoryDedicatedUsageCounter(entry->szName, entry->FmtValue.largeValue);
            }

            PhReleaseQueuedLockExclusive(&EtGpuDedicatedHashTableLock);

            PhFree(buffer);
        }
    }

    if (EtGpuSharedHashTable)
    {
        if (GetCounterArrayBuffer(
            GpuPerfCounterSharedUsageHandle,
            PDH_FMT_LARGE,
            &bufferCount,
            &buffer
            ))
        {
            PhAcquireQueuedLockExclusive(&EtGpuSharedHashTableLock);

            PhClearHashtable(EtGpuSharedHashTable);

            for (ULONG i = 0; i < bufferCount; i++)
            {
                PPDH_FMT_COUNTERVALUE_ITEM entry = PTR_ADD_OFFSET(buffer, sizeof(PDH_FMT_COUNTERVALUE_ITEM) * i);

                if (entry->FmtValue.CStatus)
                    continue;
                if (entry->FmtValue.largeValue == 0)
                    continue;

                ParseGpuProcessMemorySharedUsageCounter(entry->szName, entry->FmtValue.largeValue);
            }

            PhReleaseQueuedLockExclusive(&EtGpuSharedHashTableLock);

            PhFree(buffer);
        }
    }

    if (GetCounterArrayBuffer(
        GpuPerfCounterNonLocalUsageHandle,
        PDH_FMT_LARGE,
        &bufferCount,
        &buffer
        ))
    {
        PhAcquireQueuedLockExclusive(&EtGpuCommitHashTableLock);

        PhClearHashtable(EtGpuCommitHashTable);

        for (ULONG i = 0; i < bufferCount; i++)
        {
            PPDH_FMT_COUNTERVALUE_ITEM entry = PTR_ADD_OFFSET(buffer, sizeof(PDH_FMT_COUNTERVALUE_ITEM) * i);

            if (entry->FmtValue.CStatus)
                continue;
            if (entry->FmtValue.largeValue == 0)
                continue;

            ParseGpuProcessMemoryCommitUsageCounter(entry->szName, entry->FmtValue.largeValue);
        }

        PhReleaseQueuedLockExclusive(&EtGpuCommitHashTableLock);

        PhFree(buffer);
    }

    RtlUpdateTimer(PhGetGlobalTimerQueue(), GpuCounterTimerHandle, 1000, INFINITE);
}

NTSTATUS NTAPI EtGpuCounterQueryThread(
    _In_ PVOID ThreadParameter
    )
{
    if (PdhOpenQuery(NULL, 0, &GpuPerfCounterQueryHandle) != ERROR_SUCCESS)
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

    if (PdhAddCounter(GpuPerfCounterQueryHandle, L"\\GPU Engine(*)\\Running Time", 0, &GpuPerfCounterRunningTimeHandle) != ERROR_SUCCESS)
        goto CleanupExit;
    if (PdhAddCounter(GpuPerfCounterQueryHandle, L"\\GPU Process Memory(*)\\Shared Usage", 0, &GpuPerfCounterSharedUsageHandle) != ERROR_SUCCESS)
        goto CleanupExit;
    if (PdhAddCounter(GpuPerfCounterQueryHandle, L"\\GPU Process Memory(*)\\Dedicated Usage", 0, &GpuPerfCounterDedicatedUsageHandle) != ERROR_SUCCESS)
        goto CleanupExit;
    if (PdhAddCounter(GpuPerfCounterQueryHandle, L"\\GPU Process Memory(*)\\Local Usage", 0, &GpuPerfCounterNonLocalUsageHandle) != ERROR_SUCCESS)
        goto CleanupExit;

    //if (!NT_SUCCESS(NtCreateEvent(&perfQueryEvent, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE)))
    //    return STATUS_SUCCESS;
    //if (PdhCollectQueryDataEx(perfQueryHandle, 1, perfQueryEvent) != ERROR_SUCCESS)
    //    return STATUS_SUCCESS;

    if (PdhCollectQueryData(GpuPerfCounterQueryHandle) != ERROR_SUCCESS)
        goto CleanupExit;

    if (!NT_SUCCESS(RtlCreateTimer(
        PhGetGlobalTimerQueue(),
        &GpuCounterTimerHandle,
        EtGpuCounterTimerQueueCallback,
        NULL,
        0,
        1000,
        0
        )))
    {
        goto CleanupExit;
    }

CleanupExit:
    //PdhCloseQuery(perfQueryHandle);
    //DeleteTimer();

    return STATUS_SUCCESS;
}
