/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include "agenttools.h"
#include <mapldr.h>

// GPU utilization is not System Informer's own data: ExtendedTools collects it from the graphics
// performance counters and publishes it on its plugin interface, keyed by adapter LUID. So this
// tool enumerates the adapters itself (D3DKMT, which also gives their names, engines and memory
// limits) and asks ExtendedTools what each one is doing.
//
// The interface answers 0 for an adapter it has no counters for, which is exactly what an idle
// GPU looks like. Everything that comes from it is therefore reported as null, not zero, unless
// the collector is actually running, and `collector` says which of its two settings is off.

// Mirrors EtGpuMonitorInitialization: the counters that back the interface are only collected when
// the GPU monitor is on and its performance-counter mode is enabled.
BOOLEAN AtpGpuCountersEnabled(
    VOID
    )
{
    if (!PhFindPlugin(EXTENDEDTOOLS_PLUGIN_NAME))
        return FALSE;

    return !!PhGetIntegerSetting(L"ExtendedTools.EnableGpuMonitor") &&
        !!PhGetIntegerSetting(L"ExtendedTools.EnableGpuPerformanceCounters");
}

PCSTR AtpGpuEngineTypeString(
    _In_ DXGK_ENGINE_TYPE EngineType
    )
{
    switch (EngineType)
    {
    case DXGK_ENGINE_TYPE_OTHER:
        return "other";
    case DXGK_ENGINE_TYPE_3D:
        return "3d";
    case DXGK_ENGINE_TYPE_VIDEO_DECODE:
        return "video_decode";
    case DXGK_ENGINE_TYPE_VIDEO_ENCODE:
        return "video_encode";
    case DXGK_ENGINE_TYPE_VIDEO_PROCESSING:
        return "video_processing";
    case DXGK_ENGINE_TYPE_SCENE_ASSEMBLY:
        return "scene_assembly";
    case DXGK_ENGINE_TYPE_COPY:
        return "copy";
    case DXGK_ENGINE_TYPE_OVERLAY:
        return "overlay";
    case DXGK_ENGINE_TYPE_CRYPTO:
        return "crypto";
    case DXGK_ENGINE_TYPE_VIDEO_CODEC:
        return "video_codec";
    }

    return "unknown";
}

NTSTATUS AtpQueryAdapterInformation(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ KMTQUERYADAPTERINFOTYPE InformationClass,
    _Out_writes_bytes_opt_(InformationLength) PVOID Information,
    _In_ UINT32 InformationLength
    )
{
    D3DKMT_QUERYADAPTERINFO queryAdapterInfo;

    memset(&queryAdapterInfo, 0, sizeof(D3DKMT_QUERYADAPTERINFO));
    queryAdapterInfo.hAdapter = AdapterHandle;
    queryAdapterInfo.Type = InformationClass;
    queryAdapterInfo.pPrivateDriverData = Information;
    queryAdapterInfo.PrivateDriverDataSize = InformationLength;

    return D3DKMTQueryAdapterInfo(&queryAdapterInfo);
}

VOID AtpCloseAdapterHandle(
    _In_ D3DKMT_HANDLE AdapterHandle
    )
{
    D3DKMT_CLOSEADAPTER closeAdapter;

    memset(&closeAdapter, 0, sizeof(D3DKMT_CLOSEADAPTER));
    closeAdapter.hAdapter = AdapterHandle;

    D3DKMTCloseAdapter(&closeAdapter);
}

// EnumAdapters3 is preferred because compute-only and display-only adapters are left out of the
// older enumeration by design; it only exists from Windows 10 20H1, so EnumAdapters2 is the
// fallback. Both hand back opened adapter handles that the caller has to close.
_Success_(return)
BOOLEAN AtpEnumerateGraphicsAdapters(
    _Outptr_result_maybenull_ D3DKMT_ADAPTERINFO** Adapters,
    _Out_ PULONG Count
    )
{
    static PFND3DKMT_ENUMADAPTERS3 enumAdapters3 = NULL;
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    D3DKMT_ADAPTERINFO* adapters;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID gdi32;

        if (gdi32 = PhGetDllHandle(L"gdi32.dll"))
            enumAdapters3 = PhGetProcedureAddressT(gdi32, D3DKMTEnumAdapters3);

        PhEndInitOnce(&initOnce);
    }

    if (enumAdapters3)
    {
        D3DKMT_ENUMADAPTERS3 enumAdapters;

        memset(&enumAdapters, 0, sizeof(D3DKMT_ENUMADAPTERS3));
        enumAdapters.Filter.IncludeComputeOnly = 1;
        enumAdapters.Filter.IncludeDisplayOnly = 1;

        if (NT_SUCCESS(enumAdapters3(&enumAdapters)) && enumAdapters.NumAdapters)
        {
            adapters = PhAllocateZero(sizeof(D3DKMT_ADAPTERINFO) * enumAdapters.NumAdapters);
            enumAdapters.pAdapters = adapters;

            if (NT_SUCCESS(enumAdapters3(&enumAdapters)))
            {
                *Adapters = adapters;
                *Count = enumAdapters.NumAdapters;
                return TRUE;
            }

            PhFree(adapters);
        }
    }

    {
        D3DKMT_ENUMADAPTERS2 enumAdapters;

        memset(&enumAdapters, 0, sizeof(D3DKMT_ENUMADAPTERS2));

        if (NT_SUCCESS(D3DKMTEnumAdapters2(&enumAdapters)) && enumAdapters.NumAdapters)
        {
            adapters = PhAllocateZero(sizeof(D3DKMT_ADAPTERINFO) * enumAdapters.NumAdapters);
            enumAdapters.pAdapters = adapters;

            if (NT_SUCCESS(D3DKMTEnumAdapters2(&enumAdapters)))
            {
                *Adapters = adapters;
                *Count = enumAdapters.NumAdapters;
                return TRUE;
            }

            PhFree(adapters);
        }
    }

    return FALSE;
}

ULONG AtpQueryAdapterNodeCount(
    _In_ LUID AdapterLuid
    )
{
    D3DKMT_QUERYSTATISTICS queryStatistics;

    memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
    queryStatistics.Type = D3DKMT_QUERYSTATISTICS_ADAPTER;
    queryStatistics.AdapterLuid = AdapterLuid;

    if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
        return queryStatistics.QueryResult.AdapterInformation.NodeCount;

    return 0;
}

// One entry per engine the adapter reports, named by what it does, so "the GPU is busy" can be
// told apart from "something is decoding video".
VOID AtpAddAdapterEngines(
    _In_ PVOID Row,
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ LUID AdapterLuid,
    _In_opt_ PEXTENDEDTOOLS_INTERFACE Interface
    )
{
    PVOID engines;
    ULONG nodeCount;
    ULONG i;

    nodeCount = AtpQueryAdapterNodeCount(AdapterLuid);
    engines = PhCreateJsonArray();

    for (i = 0; i < nodeCount; i++)
    {
        D3DKMT_NODEMETADATA metaData;
        PVOID engine;

        engine = PhCreateJsonObject();
        PhAddJsonObjectUInt64(engine, "engine_id", i);

        memset(&metaData, 0, sizeof(D3DKMT_NODEMETADATA));
        metaData.NodeOrdinalAndAdapterIndex = MAKEWORD(i, 0);

        if (NT_SUCCESS(AtpQueryAdapterInformation(
            AdapterHandle,
            KMTQAITYPE_NODEMETADATA,
            &metaData,
            sizeof(D3DKMT_NODEMETADATA)
            )))
        {
            PhAddJsonObject(engine, "engine_type", AtpGpuEngineTypeString(metaData.NodeData.EngineType));
        }
        else
        {
            AtJsonAddNull(engine, "engine_type");
        }

        if (Interface)
            PhAddJsonObjectDouble(engine, "gpu_usage", Interface->GetGpuAdapterEngineUtilization(AdapterLuid, i));
        else
            AtJsonAddNull(engine, "gpu_usage");

        PhAddJsonArrayObject(engines, engine);
    }

    PhAddJsonObjectValue(Row, "engines", engines);
}

// What sort of adapter this is, straight from its own type flags. A machine reports more adapters
// than it has cards: software renderers, compute-only devices and paravirtualized adapters all
// enumerate alongside the real ones, and only these flags tell them apart.
VOID AtpAddAdapterType(
    _In_ PVOID Row,
    _In_ D3DKMT_HANDLE AdapterHandle
    )
{
    D3DKMT_ADAPTERTYPE adapterType;

    memset(&adapterType, 0, sizeof(D3DKMT_ADAPTERTYPE));

    if (NT_SUCCESS(AtpQueryAdapterInformation(
        AdapterHandle,
        KMTQAITYPE_ADAPTERTYPE,
        &adapterType,
        sizeof(D3DKMT_ADAPTERTYPE)
        )))
    {
        PhAddJsonObjectBoolean(Row, "render_supported", !!adapterType.RenderSupported);
        PhAddJsonObjectBoolean(Row, "display_supported", !!adapterType.DisplaySupported);
        PhAddJsonObjectBoolean(Row, "software_device", !!adapterType.SoftwareDevice);
        PhAddJsonObjectBoolean(Row, "compute_only", !!adapterType.ComputeOnly);
    }
    else
    {
        AtJsonAddNull(Row, "render_supported");
        AtJsonAddNull(Row, "display_supported");
        AtJsonAddNull(Row, "software_device");
        AtJsonAddNull(Row, "compute_only");
    }
}

VOID AtpAddAdapterMemoryLimits(
    _In_ PVOID Row,
    _In_ D3DKMT_HANDLE AdapterHandle
    )
{
    D3DKMT_SEGMENTSIZEINFO segmentInfo;

    memset(&segmentInfo, 0, sizeof(D3DKMT_SEGMENTSIZEINFO));

    if (NT_SUCCESS(AtpQueryAdapterInformation(
        AdapterHandle,
        KMTQAITYPE_GETSEGMENTSIZE,
        &segmentInfo,
        sizeof(D3DKMT_SEGMENTSIZEINFO)
        )))
    {
        PhAddJsonObjectUInt64(Row, "dedicated_memory_limit_bytes", segmentInfo.DedicatedVideoMemorySize);
        PhAddJsonObjectUInt64(Row, "shared_memory_limit_bytes", segmentInfo.SharedSystemMemorySize);
    }
    else
    {
        AtJsonAddNull(Row, "dedicated_memory_limit_bytes");
        AtJsonAddNull(Row, "shared_memory_limit_bytes");
    }
}

VOID AtpAddAdapterDescription(
    _In_ PVOID Row,
    _In_ D3DKMT_HANDLE AdapterHandle
    )
{
    D3DKMT_ADAPTERREGISTRYINFO registryInfo;

    memset(&registryInfo, 0, sizeof(D3DKMT_ADAPTERREGISTRYINFO));

    if (NT_SUCCESS(AtpQueryAdapterInformation(
        AdapterHandle,
        KMTQAITYPE_ADAPTERREGISTRYINFO,
        &registryInfo,
        sizeof(D3DKMT_ADAPTERREGISTRYINFO)
        )))
    {
        registryInfo.AdapterString[RTL_NUMBER_OF(registryInfo.AdapterString) - 1] = UNICODE_NULL;
        AtJsonAddStringZ(Row, "description", registryInfo.AdapterString);
    }
    else
    {
        AtJsonAddNull(Row, "description");
    }
}

VOID AtpGetGpuUsage(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PEXTENDEDTOOLS_INTERFACE pluginInterface;
    PEXTENDEDTOOLS_INTERFACE counters;
    D3DKMT_ADAPTERINFO* adapters;
    ULONG adapterCount;
    AT_ROWS rows;
    PVOID structured;
    PVOID collector;
    FLOAT maximumUsage = 0;
    ULONG64 dedicated = 0;
    ULONG64 shared = 0;
    ULONG i;

    pluginInterface = AtGetExtendedToolsInterface();

    if (!pluginInterface)
    {
        AtSetToolHint(Result, AT_HINT_PLUGIN_MISSING);
        AtSetToolError(
            Result,
            "plugin_missing",
            STATUS_NOT_FOUND,
            L"The ExtendedTools plugin is not loaded, so nothing is collecting GPU utilization."
            );
        return;
    }

    if (!AtpEnumerateGraphicsAdapters(&adapters, &adapterCount))
    {
        AtSetToolStatusError(Result, STATUS_UNSUCCESSFUL, L"Enumerating the graphics adapters");
        return;
    }

    // Null rather than zero everywhere the collector would have answered, so an idle GPU and a
    // GPU nobody is watching do not read the same.
    counters = AtpGpuCountersEnabled() ? pluginInterface : NULL;

    structured = PhCreateJsonObject();
    AtInitializeRows(&rows, Call->Arguments);

    for (i = 0; i < adapterCount; i++)
    {
        PVOID row;

        row = PhCreateJsonObject();
        AtJsonAddHex(row, "luid", ((ULONG64)(ULONG)adapters[i].AdapterLuid.HighPart << 32) | adapters[i].AdapterLuid.LowPart);
        AtpAddAdapterDescription(row, adapters[i].hAdapter);
        AtpAddAdapterType(row, adapters[i].hAdapter);

        if (counters)
        {
            FLOAT usage = counters->GetGpuAdapterUtilization(adapters[i].AdapterLuid);
            ULONG64 adapterDedicated = counters->GetGpuAdapterDedicated(adapters[i].AdapterLuid);
            ULONG64 adapterShared = counters->GetGpuAdapterShared(adapters[i].AdapterLuid);

            PhAddJsonObjectDouble(row, "gpu_usage", usage);
            PhAddJsonObjectUInt64(row, "dedicated_memory_bytes", adapterDedicated);
            PhAddJsonObjectUInt64(row, "shared_memory_bytes", adapterShared);

            if (usage > maximumUsage)
                maximumUsage = usage;

            dedicated += adapterDedicated;
            shared += adapterShared;
        }
        else
        {
            AtJsonAddNull(row, "gpu_usage");
            AtJsonAddNull(row, "dedicated_memory_bytes");
            AtJsonAddNull(row, "shared_memory_bytes");
        }

        AtpAddAdapterMemoryLimits(row, adapters[i].hAdapter);
        AtpAddAdapterEngines(row, adapters[i].hAdapter, adapters[i].AdapterLuid, counters);

        AtAddRow(&rows, row);
    }

    AtAddRows(structured, "adapters", &rows);

    if (counters)
    {
        // The busiest adapter, not a sum: utilization of two adapters does not add up to anything.
        PhAddJsonObjectDouble(structured, "gpu_usage", maximumUsage);
        PhAddJsonObjectUInt64(structured, "dedicated_memory_bytes", dedicated);
        PhAddJsonObjectUInt64(structured, "shared_memory_bytes", shared);
    }
    else
    {
        AtJsonAddNull(structured, "gpu_usage");
        AtJsonAddNull(structured, "dedicated_memory_bytes");
        AtJsonAddNull(structured, "shared_memory_bytes");
    }

    collector = PhCreateJsonObject();
    PhAddJsonObjectBoolean(collector, "gpu_monitor_enabled", !!PhGetIntegerSetting(L"ExtendedTools.EnableGpuMonitor"));
    PhAddJsonObjectBoolean(collector, "performance_counters_enabled", !!PhGetIntegerSetting(L"ExtendedTools.EnableGpuPerformanceCounters"));
    PhAddJsonObjectBoolean(collector, "usage_available", !!counters);
    PhAddJsonObjectValue(structured, "collector", collector);

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    for (i = 0; i < adapterCount; i++)
        AtpCloseAdapterHandle(adapters[i].hAdapter);

    PhFree(adapters);
    AtDeleteRows(&rows);
}

VOID AtGpuInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    UNREFERENCED_PARAMETER(Target);

    switch (Tool->Action)
    {
    case AtActionGetGpuUsage:
        AtpGetGpuUsage(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
