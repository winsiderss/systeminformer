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

// History over the provider's circular buffers: what a process has been doing for the last N
// seconds, without the agent having to poll and diff. Index 0 is the most recent sample, and
// PhGetStatisticsTime maps an index to its wall clock time (and refuses one that is older than the
// buffer holds, which is what bounds the walk).

#define AT_HISTORY_DEFAULT_WINDOW_SECONDS 60

typedef struct _AT_HISTORY_STATS
{
    DOUBLE Total;
    DOUBLE Maximum;
    DOUBLE Last;
    ULONG Count;
} AT_HISTORY_STATS, *PAT_HISTORY_STATS;

VOID AtpAccumulate(
    _Inout_ PAT_HISTORY_STATS Stats,
    _In_ DOUBLE Value
    )
{
    if (Stats->Count == 0 || Value > Stats->Maximum)
        Stats->Maximum = Value;

    if (Stats->Count == 0)
        Stats->Last = Value;

    Stats->Total += Value;
    Stats->Count++;
}

// Aggregates for one series. The samples are per-tick values, so the average of a rate series is a
// rate and the total of a byte series is the bytes moved in the window; both are given rather than
// making the agent guess which one the field is.
VOID AtpAddStats(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ PAT_HISTORY_STATS Stats,
    _In_ BOOLEAN Cumulative
    )
{
    PVOID entry;

    entry = PhCreateJsonObject();
    PhAddJsonObjectDouble(entry, "average", Stats->Count ? Stats->Total / Stats->Count : 0.0);
    PhAddJsonObjectDouble(entry, "maximum", Stats->Count ? Stats->Maximum : 0.0);
    PhAddJsonObjectDouble(entry, "last", Stats->Count ? Stats->Last : 0.0);

    if (Cumulative)
        PhAddJsonObjectDouble(entry, "total", Stats->Total);

    PhAddJsonObjectValue(Object, Key, entry);
}

VOID AtpGetProcessHistory(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    AT_TARGET target;
    AT_ROWS rows;
    PVOID structured;
    PPH_PROCESS_ITEM processItem;
    AT_HISTORY_STATS cpuKernel;
    AT_HISTORY_STATS cpuUser;
    AT_HISTORY_STATS cpu;
    AT_HISTORY_STATS ioRead;
    AT_HISTORY_STATS ioWrite;
    AT_HISTORY_STATS ioOther;
    AT_HISTORY_STATS privateBytes;
    BOOLEAN includeSamples;
    ULONG interval;
    ULONG64 windowSeconds;
    ULONG windowSamples;
    ULONG available;
    ULONG i;

    if (!NT_SUCCESS(AtResolveProcessTarget(Call->Arguments, FALSE, 0, &target, Result)))
        return;

    processItem = target.ProcessItem;
    interval = AtGetUpdateInterval();

    if (!AtGetArgumentUInt64(Call->Arguments, "window_seconds", &windowSeconds) || windowSeconds == 0)
        windowSeconds = AT_HISTORY_DEFAULT_WINDOW_SECONDS;

    // One sample per provider run: a window in seconds is that many runs back.
    windowSamples = (ULONG)min(windowSeconds * 1000 / interval, MAXLONG);

    if (windowSamples == 0)
        windowSamples = 1;

    includeSamples = AtJsonGetObjectBoolean(Call->Arguments, "include_samples");

    memset(&cpuKernel, 0, sizeof(AT_HISTORY_STATS));
    memset(&cpuUser, 0, sizeof(AT_HISTORY_STATS));
    memset(&cpu, 0, sizeof(AT_HISTORY_STATS));
    memset(&ioRead, 0, sizeof(AT_HISTORY_STATS));
    memset(&ioWrite, 0, sizeof(AT_HISTORY_STATS));
    memset(&ioOther, 0, sizeof(AT_HISTORY_STATS));
    memset(&privateBytes, 0, sizeof(AT_HISTORY_STATS));

    AtInitializeRows(&rows, Call->Arguments);

    // Every series is pushed once per run, so the shortest one bounds the walk.
    available = processItem->CpuKernelHistory.Count;
    available = min(available, processItem->CpuUserHistory.Count);
    available = min(available, processItem->IoReadHistory.Count);
    available = min(available, processItem->IoWriteHistory.Count);
    available = min(available, processItem->IoOtherHistory.Count);
    available = min(available, processItem->PrivateBytesHistory.Count);
    available = min(available, windowSamples);

    for (i = 0; i < available; i++)
    {
        FLOAT kernel;
        FLOAT user;
        ULONG64 read;
        ULONG64 write;
        ULONG64 other;
        SIZE_T bytes;
        LARGE_INTEGER time;
        BOOLEAN haveTime;

        kernel = PhGetItemCircularBuffer_FLOAT(&processItem->CpuKernelHistory, i);
        user = PhGetItemCircularBuffer_FLOAT(&processItem->CpuUserHistory, i);
        read = PhGetItemCircularBuffer_ULONG64(&processItem->IoReadHistory, i);
        write = PhGetItemCircularBuffer_ULONG64(&processItem->IoWriteHistory, i);
        other = PhGetItemCircularBuffer_ULONG64(&processItem->IoOtherHistory, i);
        bytes = PhGetItemCircularBuffer_SIZE_T(&processItem->PrivateBytesHistory, i);

        // False once the index reaches past what the shared time history still holds.
        haveTime = PhGetStatisticsTime(processItem, i, &time);

        if (!haveTime)
            break;

        AtpAccumulate(&cpuKernel, kernel);
        AtpAccumulate(&cpuUser, user);
        AtpAccumulate(&cpu, (DOUBLE)kernel + user);
        AtpAccumulate(&ioRead, (DOUBLE)read);
        AtpAccumulate(&ioWrite, (DOUBLE)write);
        AtpAccumulate(&ioOther, (DOUBLE)other);
        AtpAccumulate(&privateBytes, (DOUBLE)bytes);

        if (includeSamples)
        {
            PVOID row = PhCreateJsonObject();

            AtJsonAddTime(row, "time", &time);
            PhAddJsonObjectDouble(row, "cpu_usage", (DOUBLE)kernel + user);
            PhAddJsonObjectDouble(row, "cpu_kernel_usage", kernel);
            PhAddJsonObjectDouble(row, "cpu_user_usage", user);
            PhAddJsonObjectUInt64(row, "io_read_bytes", read);
            PhAddJsonObjectUInt64(row, "io_write_bytes", write);
            PhAddJsonObjectUInt64(row, "io_other_bytes", other);
            PhAddJsonObjectUInt64(row, "private_bytes", bytes);
            AtAddRow(&rows, row);
        }
    }

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, processItem);
    PhAddJsonObjectUInt64(structured, "update_interval_ms", interval);
    PhAddJsonObjectUInt64(structured, "window_seconds", (ULONG64)cpu.Count * interval / 1000);
    PhAddJsonObjectUInt64(structured, "sample_count", cpu.Count);

    AtpAddStats(structured, "cpu_usage", &cpu, FALSE);
    AtpAddStats(structured, "cpu_kernel_usage", &cpuKernel, FALSE);
    AtpAddStats(structured, "cpu_user_usage", &cpuUser, FALSE);
    AtpAddStats(structured, "io_read_bytes", &ioRead, TRUE);
    AtpAddStats(structured, "io_write_bytes", &ioWrite, TRUE);
    AtpAddStats(structured, "io_other_bytes", &ioOther, TRUE);
    AtpAddStats(structured, "private_bytes", &privateBytes, FALSE);

    if (includeSamples)
    {
        AtAddRows(structured, "samples", &rows);
    }
    else
    {
        AtDeleteRows(&rows);
        AtJsonAddNull(structured, "samples");
    }

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteTarget(&target);
}

// The system-wide twin of get_process_history. The per-CPU series are summarised per processor
// rather than returned in full: on a large machine the full set is thousands of samples that say
// less than "core 12 was pinned".
VOID AtpGetSystemHistory(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PH_PLUGIN_SYSTEM_STATISTICS statistics;
    SYSTEM_BASIC_INFORMATION basicInfo;
    AT_ROWS rows;
    PVOID structured;
    AT_HISTORY_STATS cpuKernel;
    AT_HISTORY_STATS cpuUser;
    AT_HISTORY_STATS cpu;
    AT_HISTORY_STATS ioRead;
    AT_HISTORY_STATS ioWrite;
    AT_HISTORY_STATS ioOther;
    AT_HISTORY_STATS commit;
    AT_HISTORY_STATS physical;
    PAT_HISTORY_STATS perCpu = NULL;
    BOOLEAN includeSamples;
    BOOLEAN includePerCpu;
    ULONG processorCount;
    ULONG pageSize;
    ULONG interval;
    ULONG64 windowSeconds;
    ULONG windowSamples;
    ULONG available;
    ULONG i;
    ULONG j;

    if (!NT_SUCCESS(NtQuerySystemInformation(SystemBasicInformation, &basicInfo, sizeof(basicInfo), NULL)))
    {
        AtSetToolError(Result, "failed", STATUS_UNSUCCESSFUL, L"The system page size could not be read.");
        return;
    }

    pageSize = basicInfo.PageSize;

    // The processors of this group. System Informer allocates its per-CPU histories for every
    // processor across every group, so this count never runs past that array; on a machine with
    // more than one group the per_cpu summary covers this group only.
    processorCount = basicInfo.NumberOfProcessors;

    PhPluginGetSystemStatistics(&statistics);

    interval = AtGetUpdateInterval();

    if (!AtGetArgumentUInt64(Call->Arguments, "window_seconds", &windowSeconds) || windowSeconds == 0)
        windowSeconds = AT_HISTORY_DEFAULT_WINDOW_SECONDS;

    windowSamples = (ULONG)min(windowSeconds * 1000 / interval, MAXLONG);

    if (windowSamples == 0)
        windowSamples = 1;

    includeSamples = AtJsonGetObjectBoolean(Call->Arguments, "include_samples");
    includePerCpu = AtJsonGetObjectBoolean(Call->Arguments, "include_per_cpu");

    memset(&cpuKernel, 0, sizeof(AT_HISTORY_STATS));
    memset(&cpuUser, 0, sizeof(AT_HISTORY_STATS));
    memset(&cpu, 0, sizeof(AT_HISTORY_STATS));
    memset(&ioRead, 0, sizeof(AT_HISTORY_STATS));
    memset(&ioWrite, 0, sizeof(AT_HISTORY_STATS));
    memset(&ioOther, 0, sizeof(AT_HISTORY_STATS));
    memset(&commit, 0, sizeof(AT_HISTORY_STATS));
    memset(&physical, 0, sizeof(AT_HISTORY_STATS));

    AtInitializeRows(&rows, Call->Arguments);

    available = statistics.CpuKernelHistory->Count;
    available = min(available, statistics.CpuUserHistory->Count);
    available = min(available, statistics.IoReadHistory->Count);
    available = min(available, statistics.IoWriteHistory->Count);
    available = min(available, statistics.IoOtherHistory->Count);
    available = min(available, statistics.CommitHistory->Count);
    available = min(available, statistics.PhysicalHistory->Count);
    available = min(available, statistics.MaxCpuHistory->Count);
    available = min(available, statistics.MaxIoHistory->Count);
    available = min(available, windowSamples);

    if (includePerCpu && processorCount)
    {
        perCpu = PhAllocateZero(processorCount * sizeof(AT_HISTORY_STATS));

        // CpusKernelHistory is the address of the provider's array pointer, not an array of
        // pointers: the buffer for a processor is (*CpusKernelHistory)[j]. Indexing the field
        // directly reads whatever globals follow it and yields a garbage sample count.
        for (j = 0; j < processorCount; j++)
        {
            PPH_CIRCULAR_BUFFER_FLOAT kernelHistory = &(*statistics.CpusKernelHistory)[j];
            PPH_CIRCULAR_BUFFER_FLOAT userHistory = &(*statistics.CpusUserHistory)[j];
            ULONG count;

            count = min(kernelHistory->Count, userHistory->Count);
            count = min(count, available);

            for (i = 0; i < count; i++)
            {
                AtpAccumulate(
                    &perCpu[j],
                    (DOUBLE)PhGetItemCircularBuffer_FLOAT(kernelHistory, i) +
                    PhGetItemCircularBuffer_FLOAT(userHistory, i)
                    );
            }
        }
    }

    for (i = 0; i < available; i++)
    {
        FLOAT kernel;
        FLOAT user;
        ULONG64 read;
        ULONG64 write;
        ULONG64 other;
        ULONG64 commitBytes;
        ULONG64 physicalBytes;
        ULONG maxCpuPid;
        ULONG maxIoPid;
        LARGE_INTEGER time;

        kernel = PhGetItemCircularBuffer_FLOAT(statistics.CpuKernelHistory, i);
        user = PhGetItemCircularBuffer_FLOAT(statistics.CpuUserHistory, i);
        read = PhGetItemCircularBuffer_ULONG64(statistics.IoReadHistory, i);
        write = PhGetItemCircularBuffer_ULONG64(statistics.IoWriteHistory, i);
        other = PhGetItemCircularBuffer_ULONG64(statistics.IoOtherHistory, i);
        commitBytes = (ULONG64)PhGetItemCircularBuffer_ULONG(statistics.CommitHistory, i) * pageSize;
        physicalBytes = (ULONG64)PhGetItemCircularBuffer_ULONG(statistics.PhysicalHistory, i) * pageSize;
        maxCpuPid = PhGetItemCircularBuffer_ULONG(statistics.MaxCpuHistory, i);
        maxIoPid = PhGetItemCircularBuffer_ULONG(statistics.MaxIoHistory, i);

        if (!PhGetStatisticsTime(NULL, i, &time))
            break;

        AtpAccumulate(&cpuKernel, kernel);
        AtpAccumulate(&cpuUser, user);
        AtpAccumulate(&cpu, (DOUBLE)kernel + user);
        AtpAccumulate(&ioRead, (DOUBLE)read);
        AtpAccumulate(&ioWrite, (DOUBLE)write);
        AtpAccumulate(&ioOther, (DOUBLE)other);
        AtpAccumulate(&commit, (DOUBLE)commitBytes);
        AtpAccumulate(&physical, (DOUBLE)physicalBytes);

        if (includeSamples)
        {
            PVOID row = PhCreateJsonObject();

            AtJsonAddTime(row, "time", &time);
            PhAddJsonObjectDouble(row, "cpu_usage", (DOUBLE)kernel + user);
            PhAddJsonObjectDouble(row, "cpu_kernel_usage", kernel);
            PhAddJsonObjectDouble(row, "cpu_user_usage", user);
            PhAddJsonObjectUInt64(row, "io_read_bytes", read);
            PhAddJsonObjectUInt64(row, "io_write_bytes", write);
            PhAddJsonObjectUInt64(row, "io_other_bytes", other);
            PhAddJsonObjectUInt64(row, "commit_bytes", commitBytes);
            PhAddJsonObjectUInt64(row, "physical_in_use_bytes", physicalBytes);

            // The heaviest process of that tick, which is the point of asking about a spike after
            // the fact. It may since have exited, so this is a pid and nothing more.
            if (maxCpuPid)
                PhAddJsonObjectUInt64(row, "max_cpu_pid", maxCpuPid);
            else
                AtJsonAddNull(row, "max_cpu_pid");

            if (maxIoPid)
                PhAddJsonObjectUInt64(row, "max_io_pid", maxIoPid);
            else
                AtJsonAddNull(row, "max_io_pid");

            AtAddRow(&rows, row);
        }
    }

    structured = PhCreateJsonObject();
    PhAddJsonObjectUInt64(structured, "update_interval_ms", interval);
    PhAddJsonObjectUInt64(structured, "window_seconds", (ULONG64)cpu.Count * interval / 1000);
    PhAddJsonObjectUInt64(structured, "sample_count", cpu.Count);
    PhAddJsonObjectUInt64(structured, "processor_count", processorCount);

    AtpAddStats(structured, "cpu_usage", &cpu, FALSE);
    AtpAddStats(structured, "cpu_kernel_usage", &cpuKernel, FALSE);
    AtpAddStats(structured, "cpu_user_usage", &cpuUser, FALSE);
    AtpAddStats(structured, "io_read_bytes", &ioRead, TRUE);
    AtpAddStats(structured, "io_write_bytes", &ioWrite, TRUE);
    AtpAddStats(structured, "io_other_bytes", &ioOther, TRUE);
    AtpAddStats(structured, "commit_bytes", &commit, FALSE);
    AtpAddStats(structured, "physical_in_use_bytes", &physical, FALSE);

    if (perCpu)
    {
        PVOID array = PhCreateJsonArray();

        for (j = 0; j < processorCount; j++)
        {
            PVOID entry = PhCreateJsonObject();

            PhAddJsonObjectUInt64(entry, "index", j);
            AtpAddStats(entry, "cpu_usage", &perCpu[j], FALSE);
            PhAddJsonArrayObject(array, entry);
        }

        PhAddJsonObjectValue(structured, "per_cpu", array);
        PhFree(perCpu);
    }
    else
    {
        AtJsonAddNull(structured, "per_cpu");
    }

    if (includeSamples)
    {
        AtAddRows(structured, "samples", &rows);
    }
    else
    {
        AtDeleteRows(&rows);
        AtJsonAddNull(structured, "samples");
    }

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

VOID AtHistoryInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    UNREFERENCED_PARAMETER(Target);

    switch (Tool->Action)
    {
    case AtActionGetProcessHistory:
        AtpGetProcessHistory(Call, Result);
        break;
    case AtActionGetSystemHistory:
        AtpGetSystemHistory(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
