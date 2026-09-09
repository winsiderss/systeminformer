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

#ifndef _EXTENDEDTOOLSINTF_H
#define _EXTENDEDTOOLSINTF_H

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

typedef struct _EXTENDEDTOOLS_PROCESS_IO
{
    BOOLEAN EtwEnabled;
    BOOLEAN DiskCountersEnabled;
    BOOLEAN HaveSample;

    ULONG64 DiskReadBytes;
    ULONG64 DiskWriteBytes;
    ULONG64 NetworkReceiveBytes;
    ULONG64 NetworkSendBytes;

    ULONG64 DiskReadCount;
    ULONG64 DiskWriteCount;
    ULONG64 NetworkReceiveCount;
    ULONG64 NetworkSendCount;

    ULONG64 DiskReadBytesDelta;
    ULONG64 DiskWriteBytesDelta;
    ULONG64 NetworkReceiveBytesDelta;
    ULONG64 NetworkSendBytesDelta;

    ULONG64 DiskReadCountDelta;
    ULONG64 DiskWriteCountDelta;
    ULONG64 NetworkReceiveCountDelta;
    ULONG64 NetworkSendCountDelta;

    ULONG64 DiskTotalBytesDeltaPeak;
    ULONG64 NetworkTotalBytesDeltaPeak;
} EXTENDEDTOOLS_PROCESS_IO, *PEXTENDEDTOOLS_PROCESS_IO;

_Success_(return)
typedef BOOLEAN (NTAPI* PEXTENDEDTOOLS_GET_PROCESSIO)(
    _In_ HANDLE ProcessId,
    _Out_ PEXTENDEDTOOLS_PROCESS_IO Statistics
    );

typedef struct _EXTENDEDTOOLS_PROCESS_GPU
{
    BOOLEAN GpuEnabled;
    BOOLEAN PerformanceCountersEnabled;

    FLOAT Utilization;
    ULONG64 DedicatedBytes;
    ULONG64 SharedBytes;
    ULONG64 CommitBytes;
    ULONG64 DedicatedCommittedBytes;
    ULONG64 SharedCommittedBytes;
} EXTENDEDTOOLS_PROCESS_GPU, *PEXTENDEDTOOLS_PROCESS_GPU;

_Success_(return)
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

#endif
