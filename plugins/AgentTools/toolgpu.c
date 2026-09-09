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

VOID AtpAddEngineType(
    _In_ PVOID Engine,
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ ULONG NodeOrdinal
    )
{
    D3DKMT_NODEMETADATA metaData;

    memset(&metaData, 0, sizeof(D3DKMT_NODEMETADATA));
    metaData.NodeOrdinalAndAdapterIndex = MAKEWORD(NodeOrdinal, 0);

    if (NT_SUCCESS(AtpQueryAdapterInformation(
        AdapterHandle,
        KMTQAITYPE_NODEMETADATA,
        &metaData,
        sizeof(D3DKMT_NODEMETADATA)
        )))
    {
        PhAddJsonObject(Engine, "engine_type", AtpGpuEngineTypeString(metaData.NodeData.EngineType));
    }
    else
    {
        AtJsonAddNull(Engine, "engine_type");
    }
}

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
        PVOID engine;

        engine = PhCreateJsonObject();
        PhAddJsonObjectUInt64(engine, "engine_id", i);

        AtpAddEngineType(engine, AdapterHandle, i);

        if (Interface)
            PhAddJsonObjectDouble(engine, "gpu_usage", Interface->GetGpuAdapterEngineUtilization(AdapterLuid, i));
        else
            AtJsonAddNull(engine, "gpu_usage");

        PhAddJsonArrayObject(engines, engine);
    }

    PhAddJsonObjectValue(Row, "engines", engines);
}

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

VOID AtpAddAdapterIdentity(
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
        registryInfo.ChipType[RTL_NUMBER_OF(registryInfo.ChipType) - 1] = UNICODE_NULL;
        registryInfo.BiosString[RTL_NUMBER_OF(registryInfo.BiosString) - 1] = UNICODE_NULL;
        registryInfo.DacType[RTL_NUMBER_OF(registryInfo.DacType) - 1] = UNICODE_NULL;

        AtJsonAddStringZ(Row, "description", registryInfo.AdapterString);
        AtJsonAddStringZ(Row, "chip_type", registryInfo.ChipType);
        AtJsonAddStringZ(Row, "bios_string", registryInfo.BiosString);
        AtJsonAddStringZ(Row, "dac_type", registryInfo.DacType);
    }
    else
    {
        AtJsonAddNull(Row, "description");
        AtJsonAddNull(Row, "chip_type");
        AtJsonAddNull(Row, "bios_string");
        AtJsonAddNull(Row, "dac_type");
    }
}

VOID AtpAddAdapterDeviceIds(
    _In_ PVOID Row,
    _In_ D3DKMT_HANDLE AdapterHandle
    )
{
    D3DKMT_QUERY_DEVICE_IDS deviceIds;

    memset(&deviceIds, 0, sizeof(D3DKMT_QUERY_DEVICE_IDS));

    if (NT_SUCCESS(AtpQueryAdapterInformation(
        AdapterHandle,
        KMTQAITYPE_PHYSICALADAPTERDEVICEIDS,
        &deviceIds,
        sizeof(D3DKMT_QUERY_DEVICE_IDS)
        )))
    {
        AtJsonAddHex(Row, "vendor_id", deviceIds.DeviceIds.VendorID);
        AtJsonAddHex(Row, "device_id", deviceIds.DeviceIds.DeviceID);
        AtJsonAddHex(Row, "subsystem_id", deviceIds.DeviceIds.SubSystemID);
        AtJsonAddHex(Row, "sub_vendor_id", deviceIds.DeviceIds.SubVendorID);
        PhAddJsonObjectUInt64(Row, "revision_id", deviceIds.DeviceIds.RevisionID);
        PhAddJsonObjectUInt64(Row, "physical_adapter_index", deviceIds.PhysicalAdapterIndex);
    }
    else
    {
        AtJsonAddNull(Row, "vendor_id");
        AtJsonAddNull(Row, "device_id");
        AtJsonAddNull(Row, "subsystem_id");
        AtJsonAddNull(Row, "sub_vendor_id");
        AtJsonAddNull(Row, "revision_id");
        AtJsonAddNull(Row, "physical_adapter_index");
    }
}

VOID AtpAddAdapterDriverModel(
    _In_ PVOID Row,
    _In_ D3DKMT_HANDLE AdapterHandle
    )
{
    D3DKMT_DRIVERVERSION driverVersion = 0;

    if (NT_SUCCESS(AtpQueryAdapterInformation(
        AdapterHandle,
        KMTQAITYPE_DRIVERVERSION,
        &driverVersion,
        sizeof(D3DKMT_DRIVERVERSION)
        )) && driverVersion >= 1000)
    {
        PPH_STRING version = PhFormatString(L"%lu.%lu", (ULONG)driverVersion / 1000, ((ULONG)driverVersion % 1000) / 100);

        AtJsonAddString(Row, "wddm_version", version);
        PhClearReference(&version);
    }
    else
    {
        AtJsonAddNull(Row, "wddm_version");
    }
}

VOID AtpAddAdapterStatistics(
    _In_ PVOID Row,
    _In_ LUID AdapterLuid
    )
{
    D3DKMT_QUERYSTATISTICS queryStatistics;

    memset(&queryStatistics, 0, sizeof(D3DKMT_QUERYSTATISTICS));
    queryStatistics.Type = D3DKMT_QUERYSTATISTICS_ADAPTER;
    queryStatistics.AdapterLuid = AdapterLuid;

    if (NT_SUCCESS(D3DKMTQueryStatistics(&queryStatistics)))
    {
        PhAddJsonObjectUInt64(Row, "node_count", queryStatistics.QueryResult.AdapterInformation.NodeCount);
        PhAddJsonObjectUInt64(Row, "segment_count", queryStatistics.QueryResult.AdapterInformation.NbSegments);
        PhAddJsonObjectUInt64(Row, "display_source_count", queryStatistics.QueryResult.AdapterInformation.VidPnSourceCount);
        // How many times this adapter has been reset out from under its clients.
        PhAddJsonObjectUInt64(Row, "tdr_count", queryStatistics.QueryResult.AdapterInformation.TdrDetectedCount);
    }
    else
    {
        AtJsonAddNull(Row, "node_count");
        AtJsonAddNull(Row, "segment_count");
        AtJsonAddNull(Row, "display_source_count");
        AtJsonAddNull(Row, "tdr_count");
    }
}

VOID AtpAddAdapterSensors(
    _In_ PVOID Row,
    _In_ D3DKMT_HANDLE AdapterHandle
    )
{
    D3DKMT_ADAPTER_PERFDATA perfData;

    memset(&perfData, 0, sizeof(D3DKMT_ADAPTER_PERFDATA));

    if (NT_SUCCESS(AtpQueryAdapterInformation(
        AdapterHandle,
        KMTQAITYPE_ADAPTERPERFDATA,
        &perfData,
        sizeof(D3DKMT_ADAPTER_PERFDATA)
        )))
    {
        PhAddJsonObjectDouble(Row, "power_usage_percent", (DOUBLE)perfData.Power / 10);
        PhAddJsonObjectDouble(Row, "temperature_celsius", (DOUBLE)perfData.Temperature / 10);
        PhAddJsonObjectUInt64(Row, "fan_rpm", perfData.FanRPM);
        PhAddJsonObjectUInt64(Row, "memory_frequency_hz", perfData.MemoryFrequency);
        PhAddJsonObjectUInt64(Row, "memory_frequency_max_hz", perfData.MaxMemoryFrequency);
    }
    else
    {
        AtJsonAddNull(Row, "power_usage_percent");
        AtJsonAddNull(Row, "temperature_celsius");
        AtJsonAddNull(Row, "fan_rpm");
        AtJsonAddNull(Row, "memory_frequency_hz");
        AtJsonAddNull(Row, "memory_frequency_max_hz");
    }
}

VOID AtpAddAdapterEngineList(
    _In_ PVOID Row,
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ LUID AdapterLuid
    )
{
    PVOID engines;
    ULONG nodeCount;
    ULONG i;

    nodeCount = AtpQueryAdapterNodeCount(AdapterLuid);
    engines = PhCreateJsonArray();

    for (i = 0; i < nodeCount; i++)
    {
        PVOID engine = PhCreateJsonObject();

        PhAddJsonObjectUInt64(engine, "engine_id", i);
        AtpAddEngineType(engine, AdapterHandle, i);
        PhAddJsonArrayObject(engines, engine);
    }

    PhAddJsonObjectValue(Row, "engines", engines);
}

VOID AtpListGpuAdapters(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    D3DKMT_ADAPTERINFO* adapters;
    ULONG adapterCount;
    AT_ROWS rows;
    PVOID structured;
    ULONG i;

    if (!AtpEnumerateGraphicsAdapters(&adapters, &adapterCount))
    {
        AtSetToolStatusError(Result, STATUS_UNSUCCESSFUL, L"Enumerating the graphics adapters");
        return;
    }

    structured = PhCreateJsonObject();
    AtInitializeRows(&rows, Call->Arguments);

    for (i = 0; i < adapterCount; i++)
    {
        PVOID row;

        row = PhCreateJsonObject();
        AtJsonAddHex(row, "luid", ((ULONG64)(ULONG)adapters[i].AdapterLuid.HighPart << 32) | adapters[i].AdapterLuid.LowPart);
        AtpAddAdapterIdentity(row, adapters[i].hAdapter);
        AtpAddAdapterDeviceIds(row, adapters[i].hAdapter);
        AtpAddAdapterDriverModel(row, adapters[i].hAdapter);
        AtpAddAdapterType(row, adapters[i].hAdapter);
        AtpAddAdapterMemoryLimits(row, adapters[i].hAdapter);
        AtpAddAdapterStatistics(row, adapters[i].AdapterLuid);
        AtpAddAdapterSensors(row, adapters[i].hAdapter);
        AtpAddAdapterEngineList(row, adapters[i].hAdapter, adapters[i].AdapterLuid);

        AtAddRow(&rows, row);
    }

    AtAddRows(structured, "adapters", &rows);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    for (i = 0; i < adapterCount; i++)
        AtpCloseAdapterHandle(adapters[i].hAdapter);

    PhFree(adapters);
    AtDeleteRows(&rows);
}

VOID AtpAddProcessAdapterEngines(
    _In_ PVOID Object,
    _In_ PEXTENDEDTOOLS_INTERFACE Interface,
    _In_ HANDLE ProcessId
    )
{
    D3DKMT_ADAPTERINFO* adapters;
    ULONG adapterCount;
    PVOID array;
    ULONG i;

    if (!AtpEnumerateGraphicsAdapters(&adapters, &adapterCount))
    {
        AtJsonAddNull(Object, "adapters");
        return;
    }

    array = PhCreateJsonArray();

    for (i = 0; i < adapterCount; i++)
    {
        PVOID entry;
        PVOID engines;
        ULONG nodeCount;
        ULONG j;

        entry = PhCreateJsonObject();
        AtJsonAddHex(entry, "luid", ((ULONG64)(ULONG)adapters[i].AdapterLuid.HighPart << 32) | adapters[i].AdapterLuid.LowPart);
        AtpAddAdapterDescription(entry, adapters[i].hAdapter);

        nodeCount = AtpQueryAdapterNodeCount(adapters[i].AdapterLuid);
        engines = PhCreateJsonArray();

        for (j = 0; j < nodeCount; j++)
        {
            PVOID engine = PhCreateJsonObject();

            PhAddJsonObjectUInt64(engine, "engine_id", j);
            AtpAddEngineType(engine, adapters[i].hAdapter, j);
            PhAddJsonObjectDouble(
                engine,
                "gpu_usage",
                Interface->GetProcessGpuEngineUtilization(ProcessId, adapters[i].AdapterLuid, j)
                );

            PhAddJsonArrayObject(engines, engine);
        }

        PhAddJsonObjectValue(entry, "engines", engines);
        PhAddJsonArrayObject(array, entry);

        AtpCloseAdapterHandle(adapters[i].hAdapter);
    }

    PhAddJsonObjectValue(Object, "adapters", array);
    PhFree(adapters);
}

VOID AtpGetProcessGpuStats(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PEXTENDEDTOOLS_INTERFACE pluginInterface;
    EXTENDEDTOOLS_PROCESS_GPU statistics;
    AT_TARGET target;
    PVOID structured;
    PVOID collector;
    BOOLEAN available;

    pluginInterface = AtGetExtendedToolsInterface();

    if (!pluginInterface)
    {
        AtSetToolHint(Result, AT_HINT_PLUGIN_MISSING);
        AtSetToolError(
            Result,
            "plugin_missing",
            STATUS_NOT_FOUND,
            L"The ExtendedTools plugin is not loaded, so nothing is collecting per-process GPU usage."
            );
        return;
    }

    if (!NT_SUCCESS(AtResolveProcessTarget(Call->Arguments, FALSE, 0, &target, Result)))
        return;

    if (!pluginInterface->GetProcessGpuStatistics(target.ProcessItem->ProcessId, &statistics))
    {
        AtSetToolError(
            Result,
            "unavailable",
            STATUS_NOT_SUPPORTED,
            L"GPU monitoring is not running in this System Informer instance."
            );
        AtDeleteTarget(&target);
        return;
    }

    // Same rule as get_gpu_usage: with the counters off every process reads as idle, so say
    // nothing rather than say zero.
    available = statistics.PerformanceCountersEnabled;

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, target.ProcessItem);

    if (available)
    {
        PhAddJsonObjectDouble(structured, "gpu_usage", statistics.Utilization);
        PhAddJsonObjectUInt64(structured, "dedicated_memory_bytes", statistics.DedicatedBytes);
        PhAddJsonObjectUInt64(structured, "shared_memory_bytes", statistics.SharedBytes);
        PhAddJsonObjectUInt64(structured, "commit_bytes", statistics.CommitBytes);
        PhAddJsonObjectUInt64(structured, "dedicated_committed_bytes", statistics.DedicatedCommittedBytes);
        PhAddJsonObjectUInt64(structured, "shared_committed_bytes", statistics.SharedCommittedBytes);
    }
    else
    {
        AtJsonAddNull(structured, "gpu_usage");
        AtJsonAddNull(structured, "dedicated_memory_bytes");
        AtJsonAddNull(structured, "shared_memory_bytes");
        AtJsonAddNull(structured, "commit_bytes");
        AtJsonAddNull(structured, "dedicated_committed_bytes");
        AtJsonAddNull(structured, "shared_committed_bytes");
    }

    if (available && AtJsonGetObjectBoolean(Call->Arguments, "include_engines"))
        AtpAddProcessAdapterEngines(structured, pluginInterface, target.ProcessItem->ProcessId);
    else
        AtJsonAddNull(structured, "adapters");

    collector = PhCreateJsonObject();
    PhAddJsonObjectBoolean(collector, "gpu_monitor_enabled", statistics.GpuEnabled);
    PhAddJsonObjectBoolean(collector, "performance_counters_enabled", statistics.PerformanceCountersEnabled);
    PhAddJsonObjectBoolean(collector, "usage_available", available);
    PhAddJsonObjectValue(structured, "collector", collector);

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteTarget(&target);
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
    case AtActionListGpuAdapters:
        AtpListGpuAdapters(Call, Result);
        break;
    case AtActionGetProcessGpuStats:
        AtpGetProcessGpuStats(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
