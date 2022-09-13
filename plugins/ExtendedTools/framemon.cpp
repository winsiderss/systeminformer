/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2022
 *
 */

#include "exttools.h"
#include "framemon.h"

BOOLEAN EtFramesEnabled = FALSE;
static PPH_HASHTABLE EtFramesHashTable = nullptr;
static PH_QUEUED_LOCK EtFramesHashTableLock = PH_QUEUED_LOCK_INIT;

static BOOLEAN NTAPI EtFramesEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PET_FPS_COUNTER entry1 = static_cast<PET_FPS_COUNTER>(Entry1);
    PET_FPS_COUNTER entry2 = static_cast<PET_FPS_COUNTER>(Entry2);

    return entry1->ProcessId == entry2->ProcessId;
}

static ULONG NTAPI EtFramesHashFunction(
    _In_ PVOID Entry
    )
{
    PET_FPS_COUNTER entry = static_cast<PET_FPS_COUNTER>(Entry);

    return HandleToUlong(entry->ProcessId) / 4;
}

static NTSTATUS NTAPI EtStartFpsTraceSession(
    _In_ PVOID ThreadParameter
    )
{
    StartFpsTraceSession();
    return STATUS_SUCCESS;
}

VOID EtFramesMonitorInitialization(
    VOID
    )
{
    if (PhWindowsVersion < WINDOWS_8)
    {
        // TODO: PresentMon supports Windows 7 but doesn't generate events. Disable support until resolved. (dmex)
        return;
    }

    EtFramesEnabled = !!PhGetIntegerSetting(const_cast<PWSTR>(SETTING_NAME_ENABLE_FPS_MONITOR));

    if (!EtFramesEnabled)
        return;

    EtFramesHashTable = PhCreateHashtable(
        sizeof(ET_FPS_COUNTER),
        EtFramesEqualFunction,
        EtFramesHashFunction,
        10
        );
}

VOID EtFramesMonitorUninitialization(
    VOID
    )
{
    if (EtFramesEnabled)
    {
        StopFpsTraceSession();
    }
}

VOID EtFramesMonitorStart(
    VOID
    )
{
    if (EtFramesEnabled)
    {
        PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), EtStartFpsTraceSession, nullptr);
    }
}

PET_FPS_COUNTER EtLookupProcessGpuFrameEntry(
    _In_ HANDLE ProcessId
    )
{
    ET_FPS_COUNTER lookupEntry;
    PET_FPS_COUNTER entry;

    lookupEntry.ProcessId = ProcessId;

    PhAcquireQueuedLockShared(&EtFramesHashTableLock);
    entry = static_cast<PET_FPS_COUNTER>(PhFindEntryHashtable(EtFramesHashTable, &lookupEntry));
    PhReleaseQueuedLockShared(&EtFramesHashTableLock);

    return entry;
}

VOID EtLockGpuFrameHashTable(
    VOID
    )
{
    PhAcquireQueuedLockExclusive(&EtFramesHashTableLock);
}

VOID EtUnlockGpuFrameHashTable(
    VOID
    )
{
#pragma warning(suppress: 26110)

    PhReleaseQueuedLockExclusive(&EtFramesHashTableLock);
}

VOID EtAddGpuFrameToHashTable(
    _In_ ULONG ProcessId,
    _In_ FLOAT FrameMs,
    _In_ FLOAT FramesPerSecond,
    _In_ FLOAT DisplayLatency,
    _In_ FLOAT DisplayFramesPerSecond,
    _In_ FLOAT MsBetweenPresents,
    _In_ FLOAT MsInPresentApi,
    _In_ FLOAT MsUntilRenderComplete,
    _In_ FLOAT MsUntilDisplayed,
    _In_ USHORT Runtime,
    _In_ USHORT PresentMode
    )
{
    ET_FPS_COUNTER lookupEntry;
    PET_FPS_COUNTER entry;

    lookupEntry.ProcessId = UlongToHandle(ProcessId);
    entry = static_cast<PET_FPS_COUNTER>(PhFindEntryHashtable(EtFramesHashTable, &lookupEntry));

    if (entry)
    {
        if (entry->FrameMs < FrameMs)
            entry->FrameMs = FrameMs;
        if (entry->FramesPerSecond < FramesPerSecond)
            entry->FramesPerSecond = FramesPerSecond;
        if (entry->MsBetweenPresents < MsBetweenPresents)
            entry->MsBetweenPresents = MsBetweenPresents;
        if (entry->MsInPresentApi < MsInPresentApi)
            entry->MsInPresentApi = MsInPresentApi;
        if (entry->MsUntilRenderComplete < MsUntilRenderComplete)
            entry->MsUntilRenderComplete = MsUntilRenderComplete;
        if (entry->MsUntilDisplayed < MsUntilDisplayed)
            entry->MsUntilDisplayed = MsUntilDisplayed;
        if (entry->DisplayLatency < DisplayLatency)
            entry->DisplayLatency = DisplayLatency;
        //if (entry->DisplayFramesPerSecond < DisplayFramesPerSecond)
        //    entry->DisplayFramesPerSecond = DisplayFramesPerSecond;
        if (entry->Runtime != Runtime)
            entry->Runtime = Runtime;
        if (entry->PresentMode != PresentMode)
            entry->PresentMode = PresentMode;
    }
    else
    {
        lookupEntry.ProcessId = UlongToHandle(ProcessId);
        lookupEntry.FrameMs = FrameMs;
        lookupEntry.FramesPerSecond = FramesPerSecond;
        lookupEntry.MsBetweenPresents = MsBetweenPresents;
        lookupEntry.MsInPresentApi = MsInPresentApi;
        lookupEntry.MsUntilRenderComplete = MsUntilRenderComplete;
        lookupEntry.MsUntilDisplayed = MsUntilDisplayed;
        lookupEntry.DisplayLatency = DisplayLatency;
        //lookupEntry.DisplayFramesPerSecond = DisplayFramesPerSecond;
        lookupEntry.Runtime = Runtime;
        lookupEntry.PresentMode = PresentMode;

        PhAddEntryHashtable(EtFramesHashTable, &lookupEntry);
    }
}

VOID EtClearGpuFrameHashTable(
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
        PhDereferenceObject(EtFramesHashTable);

        EtFramesHashTable = PhCreateHashtable(
            sizeof(ET_FPS_COUNTER),
            EtFramesEqualFunction,
            EtFramesHashFunction,
            10
            );

        lastTickCount = tickCount;
    }
    else
    {
        PhClearHashtable(EtFramesHashTable);
    }
}

VOID EtProcessFramesUpdateProcessBlock(
    _In_ struct _ET_PROCESS_BLOCK* ProcessBlock
    )
{
    PET_FPS_COUNTER entry;

    if (entry = EtLookupProcessGpuFrameEntry(ProcessBlock->ProcessItem->ProcessId))
    {
        ProcessBlock->FramesPerSecond = entry->FramesPerSecond;
        ProcessBlock->FramesLatency = entry->FrameMs;
        ProcessBlock->FramesMsBetweenPresents = entry->MsBetweenPresents;
        ProcessBlock->FramesMsInPresentApi = entry->MsInPresentApi;
        ProcessBlock->FramesMsUntilRenderComplete = entry->MsUntilRenderComplete;
        ProcessBlock->FramesMsUntilDisplayed = entry->MsUntilDisplayed;
        ProcessBlock->FramesDisplayLatency = entry->DisplayLatency;
        //ProcessBlock->FramesDisplayFramesPerSecond = entry->DisplayFramesPerSecond;
        ProcessBlock->FramesRuntime = entry->Runtime;
        ProcessBlock->FramesPresentMode = entry->PresentMode;

        PhAddItemCircularBuffer_FLOAT(&ProcessBlock->FramesPerSecondHistory, ProcessBlock->FramesPerSecond);
        PhAddItemCircularBuffer_FLOAT(&ProcessBlock->FramesLatencyHistory, ProcessBlock->FramesLatency);
        PhAddItemCircularBuffer_FLOAT(&ProcessBlock->FramesMsBetweenPresentsHistory, ProcessBlock->FramesMsBetweenPresents);
        PhAddItemCircularBuffer_FLOAT(&ProcessBlock->FramesMsInPresentApiHistory, ProcessBlock->FramesMsInPresentApi);
        PhAddItemCircularBuffer_FLOAT(&ProcessBlock->FramesMsUntilRenderCompleteHistory, ProcessBlock->FramesMsUntilRenderComplete);
        PhAddItemCircularBuffer_FLOAT(&ProcessBlock->FramesMsUntilDisplayedHistory, ProcessBlock->FramesMsUntilDisplayed);
        PhAddItemCircularBuffer_FLOAT(&ProcessBlock->FramesDisplayLatencyHistory, ProcessBlock->FramesDisplayLatency);
        //PhAddItemCircularBuffer_FLOAT(&ProcessBlock->FramesDisplayFramesPerSecondHistory, ProcessBlock->FramesDisplayFramesPerSecond);
    }
    else
    {
        ProcessBlock->FramesPerSecond = 0;
        ProcessBlock->FramesLatency = 0;
        ProcessBlock->FramesMsBetweenPresents = 0;
        ProcessBlock->FramesMsInPresentApi = 0;
        ProcessBlock->FramesMsUntilRenderComplete = 0;
        ProcessBlock->FramesMsUntilDisplayed = 0;
        ProcessBlock->FramesDisplayLatency = 0;
        //ProcessBlock->FramesDisplayFramesPerSecond = 0;
        ProcessBlock->FramesRuntime = 0;
        ProcessBlock->FramesPresentMode = 0;

        PhAddItemCircularBuffer_FLOAT(&ProcessBlock->FramesPerSecondHistory, 0);
        PhAddItemCircularBuffer_FLOAT(&ProcessBlock->FramesLatencyHistory, 0);
        PhAddItemCircularBuffer_FLOAT(&ProcessBlock->FramesMsBetweenPresentsHistory, 0);
        PhAddItemCircularBuffer_FLOAT(&ProcessBlock->FramesMsInPresentApiHistory, 0);
        PhAddItemCircularBuffer_FLOAT(&ProcessBlock->FramesMsUntilRenderCompleteHistory, 0);
        PhAddItemCircularBuffer_FLOAT(&ProcessBlock->FramesMsUntilDisplayedHistory, 0);
        PhAddItemCircularBuffer_FLOAT(&ProcessBlock->FramesDisplayLatencyHistory, 0);
        //PhAddItemCircularBuffer_FLOAT(&ProcessBlock->FramesDisplayFramesPerSecondHistory, 0);
    }
}

PCWSTR EtPresentModeToString(
    _In_ USHORT PresentMode
    )
{
    switch (PresentMode)
    {
    case 1:
        return L"Hardware: Legacy Flip";
    case 2:
        return L"Hardware: Legacy Copy to front buffer";
    case 3:
        return L"Hardware: Independent Flip";
    case 4:
        return L"Composed: Flip";
    case 5:
        return L"Composed: Copy with GPU GDI";
    case 6:
        return L"Composed: Copy with CPU GDI";
    case 7:
        return L"Composed: Composition Atlas";
    case 8:
        return L"Hardware Composed: Independent Flip";
    }

    return L"Other";
}

PCWSTR EtRuntimeToString(
    _In_ USHORT Runtime
    )
{
    switch (Runtime)
    {
    case 0:
        return L"DXGI";
    case 1:
        return L"D3D9";
    }

    return L"Other";
}
