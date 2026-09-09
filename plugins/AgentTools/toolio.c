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

VOID AtpAddIoValue(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ ULONG64 Value,
    _In_ BOOLEAN Available
    )
{
    if (Available)
        PhAddJsonObjectUInt64(Object, Key, Value);
    else
        AtJsonAddNull(Object, Key);
}

VOID AtpAddIoRate(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ ULONG64 Delta,
    _In_ ULONG IntervalMs,
    _In_ BOOLEAN Available
    )
{
    if (Available)
        AtAddRate(Object, Key, Delta, IntervalMs);
    else
        AtJsonAddNull(Object, Key);
}

VOID AtpGetProcessIoRates(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PEXTENDEDTOOLS_INTERFACE pluginInterface;
    EXTENDEDTOOLS_PROCESS_IO statistics;
    AT_TARGET target;
    PVOID structured;
    PVOID disk;
    PVOID network;
    PVOID collector;
    ULONG interval;
    BOOLEAN diskAvailable;
    BOOLEAN networkBytesAvailable;
    BOOLEAN networkCountsAvailable;

    pluginInterface = AtGetExtendedToolsInterface();

    if (!pluginInterface)
    {
        AtSetToolHint(Result, AT_HINT_PLUGIN_MISSING);
        AtSetToolError(
            Result,
            "plugin_missing",
            STATUS_NOT_FOUND,
            L"The ExtendedTools plugin is not loaded, so nothing is accumulating per-process I/O."
            );
        return;
    }

    if (!NT_SUCCESS(AtResolveProcessTarget(Call->Arguments, FALSE, 0, &target, Result)))
        return;

    if (!pluginInterface->GetProcessIoStatistics(target.ProcessItem->ProcessId, &statistics))
    {
        AtSetToolError(
            Result,
            "not_found",
            STATUS_NOT_FOUND,
            L"ExtendedTools has no I/O counters for this process."
            );
        AtDeleteTarget(&target);
        return;
    }

    // The trace session attributes both kinds of event; the process item's own counters cover disk
    // on their own, and network only from the release that added them.
    diskAvailable = statistics.EtwEnabled || statistics.DiskCountersEnabled;
    networkCountsAvailable = statistics.EtwEnabled;
    networkBytesAvailable = statistics.EtwEnabled || PhWindowsVersion >= WINDOWS_11_24H2;

    interval = AtGetUpdateInterval();

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, target.ProcessItem);
    PhAddJsonObjectUInt64(structured, "update_interval_ms", interval);

    disk = PhCreateJsonObject();
    AtpAddIoValue(disk, "read_bytes", statistics.DiskReadBytes, diskAvailable);
    AtpAddIoValue(disk, "write_bytes", statistics.DiskWriteBytes, diskAvailable);
    AtpAddIoValue(disk, "read_operations", statistics.DiskReadCount, diskAvailable);
    AtpAddIoValue(disk, "write_operations", statistics.DiskWriteCount, diskAvailable);
    AtpAddIoValue(disk, "read_bytes_delta", statistics.DiskReadBytesDelta, diskAvailable);
    AtpAddIoValue(disk, "write_bytes_delta", statistics.DiskWriteBytesDelta, diskAvailable);
    AtpAddIoValue(disk, "read_operations_delta", statistics.DiskReadCountDelta, diskAvailable);
    AtpAddIoValue(disk, "write_operations_delta", statistics.DiskWriteCountDelta, diskAvailable);
    AtpAddIoRate(disk, "read_rate", statistics.DiskReadBytesDelta, interval, diskAvailable);
    AtpAddIoRate(disk, "write_rate", statistics.DiskWriteBytesDelta, interval, diskAvailable);
    AtpAddIoValue(disk, "peak_bytes_delta", statistics.DiskTotalBytesDeltaPeak, diskAvailable);
    PhAddJsonObjectValue(structured, "disk", disk);

    network = PhCreateJsonObject();
    AtpAddIoValue(network, "receive_bytes", statistics.NetworkReceiveBytes, networkBytesAvailable);
    AtpAddIoValue(network, "send_bytes", statistics.NetworkSendBytes, networkBytesAvailable);
    AtpAddIoValue(network, "receive_operations", statistics.NetworkReceiveCount, networkCountsAvailable);
    AtpAddIoValue(network, "send_operations", statistics.NetworkSendCount, networkCountsAvailable);
    AtpAddIoValue(network, "receive_bytes_delta", statistics.NetworkReceiveBytesDelta, networkBytesAvailable);
    AtpAddIoValue(network, "send_bytes_delta", statistics.NetworkSendBytesDelta, networkBytesAvailable);
    AtpAddIoValue(network, "receive_operations_delta", statistics.NetworkReceiveCountDelta, networkCountsAvailable);
    AtpAddIoValue(network, "send_operations_delta", statistics.NetworkSendCountDelta, networkCountsAvailable);
    AtpAddIoRate(network, "receive_rate", statistics.NetworkReceiveBytesDelta, interval, networkBytesAvailable);
    AtpAddIoRate(network, "send_rate", statistics.NetworkSendBytesDelta, interval, networkBytesAvailable);
    AtpAddIoValue(network, "peak_bytes_delta", statistics.NetworkTotalBytesDeltaPeak, networkBytesAvailable);
    PhAddJsonObjectValue(structured, "network", network);

    collector = PhCreateJsonObject();
    PhAddJsonObjectBoolean(collector, "etw_enabled", statistics.EtwEnabled);
    PhAddJsonObjectBoolean(collector, "disk_counters_enabled", statistics.DiskCountersEnabled);
    PhAddJsonObjectBoolean(collector, "have_sample", statistics.HaveSample);
    PhAddJsonObjectValue(structured, "collector", collector);

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteTarget(&target);
}

VOID AtIoInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    UNREFERENCED_PARAMETER(Target);

    switch (Tool->Action)
    {
    case AtActionGetProcessIoRates:
        AtpGetProcessIoRates(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
