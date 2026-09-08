/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2018-2023
 *
 */

#ifndef ETPLUGINEXT_H
#define ETPLUGINEXT_H

#define EXTENDEDTOOLS_PLUGIN_NAME L"ExtendedTools"
#define EXTENDEDTOOLS_INTERFACE_VERSION 3

typedef FLOAT (NTAPI* PEXTENDEDTOOLS_GET_GPUADAPTERUTILIZATION)(
    _In_ LUID AdapterLuid
    );
typedef ULONG64 (NTAPI* PEXTENDEDTOOLS_GET_GPUADAPTERDEDICATED)(
    _In_ LUID AdapterLuid
    );
typedef ULONG64 (NTAPI* PEXTENDEDTOOLS_GET_GPUADAPTERSHARED)(
    _In_ LUID AdapterLuid
    );
typedef FLOAT (NTAPI* PEXTENDEDTOOLS_GET_GPUADAPTERENGINEUTILIZATION)(
    _In_ LUID AdapterLuid,
    _In_ ULONG EngineId
    );

/**
 * Per-process disk and network I/O as ExtendedTools accumulates it.
 *
 * 
emarks The counters have two independent sources and each field is only as good as the source
 * that fills it. The kernel trace session attributes disk and network events to processes and is
 * the only source of the operation counts; the disk and network counters on the process item fill
 * in the byte totals without it. EtwEnabled and DiskCountersEnabled say which of the two were
 * running, so a reader can tell "nothing happened" from "nobody was watching".
 *
 * The block is written by the process provider on its own thread and is copied out without
 * synchronisation, so a reader can see a total from one run beside a delta from the next.
 */
typedef struct _EXTENDEDTOOLS_PROCESS_IO
{
    BOOLEAN EtwEnabled;         // The kernel trace session is running (elevation + EnableEtwMonitor).
    BOOLEAN DiskCountersEnabled;// Disk byte totals are being taken from the process item.
    BOOLEAN HaveSample;         // A provider run has completed, so the deltas mean something.

    ULONG64 DiskReadBytes;
    ULONG64 DiskWriteBytes;
    ULONG64 NetworkReceiveBytes;
    ULONG64 NetworkSendBytes;

    ULONG64 DiskReadCount;      // Operations, not bytes.
    ULONG64 DiskWriteCount;
    ULONG64 NetworkReceiveCount;
    ULONG64 NetworkSendCount;

    ULONG64 DiskReadBytesDelta; // In the last provider run.
    ULONG64 DiskWriteBytesDelta;
    ULONG64 NetworkReceiveBytesDelta;
    ULONG64 NetworkSendBytesDelta;

    ULONG64 DiskReadCountDelta;
    ULONG64 DiskWriteCountDelta;
    ULONG64 NetworkReceiveCountDelta;
    ULONG64 NetworkSendCountDelta;

    ULONG64 DiskTotalBytesDeltaPeak;    // Busiest run seen since the process was first observed.
    ULONG64 NetworkTotalBytesDeltaPeak;
} EXTENDEDTOOLS_PROCESS_IO, *PEXTENDEDTOOLS_PROCESS_IO;

typedef BOOLEAN (NTAPI* PEXTENDEDTOOLS_GET_PROCESSIO)(
    _In_ HANDLE ProcessId,
    _Out_ PEXTENDEDTOOLS_PROCESS_IO Statistics
    );

/**
 * The graphics work and video memory attributed to one process.
 *
 * emarks These come from the graphics performance counters, which ExtendedTools only collects
 * when its GPU monitor and performance-counter mode are both on. PerformanceCountersEnabled says
 * whether they were: with the collector off every field here is zero, and a process that is not
 * using the GPU is zero too.
 */
typedef struct _EXTENDEDTOOLS_PROCESS_GPU
{
    BOOLEAN GpuEnabled;                 // GPU monitoring found adapters and started.
    BOOLEAN PerformanceCountersEnabled; // The counters these values are read from are being collected.

    FLOAT Utilization;                  // 0..1, this process's engine shares added up and capped.
    ULONG64 DedicatedBytes;
    ULONG64 SharedBytes;
    ULONG64 CommitBytes;
    ULONG64 DedicatedCommittedBytes;
    ULONG64 SharedCommittedBytes;
} EXTENDEDTOOLS_PROCESS_GPU, *PEXTENDEDTOOLS_PROCESS_GPU;

typedef BOOLEAN (NTAPI* PEXTENDEDTOOLS_GET_PROCESSGPU)(
    _In_ HANDLE ProcessId,
    _Out_ PEXTENDEDTOOLS_PROCESS_GPU Statistics
    );

typedef FLOAT (NTAPI* PEXTENDEDTOOLS_GET_PROCESSGPUENGINE)(
    _In_ HANDLE ProcessId,
    _In_ LUID AdapterLuid,
    _In_ ULONG EngineId
    );

typedef struct _EXTENDEDTOOLS_INTERFACE
{
    ULONG Version;
    PEXTENDEDTOOLS_GET_GPUADAPTERUTILIZATION GetGpuAdapterUtilization;
    PEXTENDEDTOOLS_GET_GPUADAPTERDEDICATED GetGpuAdapterDedicated;
    PEXTENDEDTOOLS_GET_GPUADAPTERSHARED GetGpuAdapterShared;
    PEXTENDEDTOOLS_GET_GPUADAPTERENGINEUTILIZATION GetGpuAdapterEngineUtilization;
    PEXTENDEDTOOLS_GET_PROCESSIO GetProcessIoStatistics; // Version 2
    PEXTENDEDTOOLS_GET_PROCESSGPU GetProcessGpuStatistics; // Version 3
    PEXTENDEDTOOLS_GET_PROCESSGPUENGINE GetProcessGpuEngineUtilization; // Version 3
} EXTENDEDTOOLS_INTERFACE, *PEXTENDEDTOOLS_INTERFACE;

extern EXTENDEDTOOLS_INTERFACE PluginInterface;

#endif
