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

#define EXTENDEDTOOLS_PLUGIN_NAME L"ProcessHacker.ExtendedTools"
#define EXTENDEDTOOLS_INTERFACE_VERSION 1

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

typedef struct _EXTENDEDTOOLS_INTERFACE
{
    ULONG Version;
    PEXTENDEDTOOLS_GET_GPUADAPTERUTILIZATION GetGpuAdapterUtilization;
    PEXTENDEDTOOLS_GET_GPUADAPTERDEDICATED GetGpuAdapterDedicated;
    PEXTENDEDTOOLS_GET_GPUADAPTERSHARED GetGpuAdapterShared;
    PEXTENDEDTOOLS_GET_GPUADAPTERENGINEUTILIZATION GetGpuAdapterEngineUtilization;
} EXTENDEDTOOLS_INTERFACE, *PEXTENDEDTOOLS_INTERFACE;

extern EXTENDEDTOOLS_INTERFACE PluginInterface;

#endif
