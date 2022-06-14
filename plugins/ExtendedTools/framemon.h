/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2021
 *
 */

#ifndef FRAMECACHE_H
#define FRAMECACHE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ET_FPS_COUNTER
{
    HANDLE ProcessId;
    FLOAT FrameMs;
    FLOAT FramesPerSecond;
    FLOAT MsBetweenPresents;
    FLOAT MsInPresentApi;
    FLOAT MsUntilRenderComplete;
    FLOAT MsUntilDisplayed;
    FLOAT DisplayLatency;
    //FLOAT DisplayFramesPerSecond;   
    USHORT Runtime;
    USHORT PresentMode;
} ET_FPS_COUNTER, *PET_FPS_COUNTER;

VOID EtFramesMonitorInitialization(
    VOID
    );

VOID EtFramesMonitorUninitialization(
    VOID
    );

VOID EtFramesMonitorStart(
    VOID
    );

PET_FPS_COUNTER EtLookupProcessGpuFrameEntry(
    _In_ HANDLE ProcessId
    );

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
    );

VOID EtLockGpuFrameHashTable(
    VOID
    );

VOID EtUnlockGpuFrameHashTable(
    VOID
    );

VOID EtClearGpuFrameHashTable(
    VOID
    );

VOID EtProcessFramesUpdateProcessBlock(
    _In_ struct _ET_PROCESS_BLOCK* ProcessBlock
    );

PCWSTR EtPresentModeToString(
    _In_ USHORT PresentMode
    );

PCWSTR EtRuntimeToString(
    _In_ USHORT Runtime
    );

// PresentMon.hpp

BOOLEAN StartFpsTraceSession(
    VOID
    );
VOID StopFpsTraceSession(
    VOID
    );

#ifdef __cplusplus
}
#endif

#endif

