/*
 * Process Hacker Extended Tools -
 *   GPU frame monitoring
 *
 * Copyright (C) 2021 dmex
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

