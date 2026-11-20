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

    // System Informer allocates per-CPU histories across every group, so this count never runs past
    // that array; per_cpu covers this group only.
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
        // pointers: the buffer is (*CpusKernelHistory)[j]. Indexing the field directly reads
        // whatever globals follow it.
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

#define AT_RANK_DEFAULT_LIMIT 10

typedef struct _AT_RANK_METRIC
{
    PCWSTR Name;
    PCSTR Field;
} AT_RANK_METRIC, *PAT_RANK_METRIC;

static CONST AT_RANK_METRIC AtpRankMetrics[] =
{
    { L"cpu", "cpu_usage_average" },
    { L"io", "io_bytes_total" },
    { L"io_read", "io_read_bytes_total" },
    { L"io_write", "io_write_bytes_total" },
    { L"private_bytes_growth", "private_bytes_growth" },
    { L"private_bytes", "private_bytes" },
};

PCSTR AtpRankField(
    _In_opt_ PPH_STRING RankBy
    )
{
    ULONG i;

    if (!RankBy)
        return AtpRankMetrics[0].Field;

    for (i = 0; i < RTL_NUMBER_OF(AtpRankMetrics); i++)
    {
        if (PhEqualStringZ(RankBy->Buffer, AtpRankMetrics[i].Name, TRUE))
            return AtpRankMetrics[i].Field;
    }

    return NULL;
}

VOID AtpRankProcesses(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    AT_ROWS rows;
    PVOID structured;
    PPH_PROCESS_ITEM* processItems;
    ULONG numberOfProcessItems;
    PPH_STRING rankBy;
    PPH_STRING nameContains;
    PCSTR field;
    ULONG interval;
    ULONG64 windowSeconds;
    ULONG64 limit;
    ULONG windowSamples;
    ULONG i;
    ULONG j;

    rankBy = AtGetArgumentString(Call->Arguments, "rank_by");
    field = AtpRankField(rankBy);
    PhClearReference(&rankBy);

    if (!field)
    {
        AtSetToolError(
            Result,
            "invalid_arguments",
            STATUS_INVALID_PARAMETER,
            L"rank_by must be cpu, io, io_read, io_write, private_bytes_growth or private_bytes."
            );
        return;
    }

    interval = AtGetUpdateInterval();

    if (!AtGetArgumentUInt64(Call->Arguments, "window_seconds", &windowSeconds) || windowSeconds == 0)
        windowSeconds = AT_HISTORY_DEFAULT_WINDOW_SECONDS;

    windowSamples = (ULONG)min(windowSeconds * 1000 / interval, MAXLONG);

    if (windowSamples == 0)
        windowSamples = 1;

    nameContains = AtGetArgumentString(Call->Arguments, "name_contains");

    AtInitializeRows(&rows, Call->Arguments);

    // Ranking is the whole point of the tool, so the order is the metric's, not the caller's, and
    // a top-N default is more useful here than the list default.
    PhMoveReference(&rows.SortBy, PhZeroExtendToUtf16(field));
    rows.Descending = TRUE;

    if (!AtGetArgumentUInt64(Call->Arguments, "limit", &limit))
        rows.Limit = AT_RANK_DEFAULT_LIMIT;

    PhEnumProcessItems(&processItems, &numberOfProcessItems);

    for (i = 0; i < numberOfProcessItems; i++)
    {
        PPH_PROCESS_ITEM processItem = processItems[i];
        AT_HISTORY_STATS cpu;
        AT_HISTORY_STATS io;
        AT_HISTORY_STATS ioRead;
        AT_HISTORY_STATS ioWrite;
        ULONG available;
        SIZE_T newestPrivate = 0;
        SIZE_T oldestPrivate = 0;
        PVOID row;

        if (!AtContainsString(processItem->ProcessName, nameContains))
            continue;

        memset(&cpu, 0, sizeof(AT_HISTORY_STATS));
        memset(&io, 0, sizeof(AT_HISTORY_STATS));
        memset(&ioRead, 0, sizeof(AT_HISTORY_STATS));
        memset(&ioWrite, 0, sizeof(AT_HISTORY_STATS));

        available = processItem->CpuKernelHistory.Count;
        available = min(available, processItem->CpuUserHistory.Count);
        available = min(available, processItem->IoReadHistory.Count);
        available = min(available, processItem->IoWriteHistory.Count);
        available = min(available, processItem->IoOtherHistory.Count);
        available = min(available, processItem->PrivateBytesHistory.Count);
        available = min(available, windowSamples);

        for (j = 0; j < available; j++)
        {
            FLOAT kernel;
            FLOAT user;
            ULONG64 read;
            ULONG64 write;
            ULONG64 other;
            SIZE_T bytes;
            LARGE_INTEGER time;

            if (!PhGetStatisticsTime(processItem, j, &time))
                break;

            kernel = PhGetItemCircularBuffer_FLOAT(&processItem->CpuKernelHistory, j);
            user = PhGetItemCircularBuffer_FLOAT(&processItem->CpuUserHistory, j);
            read = PhGetItemCircularBuffer_ULONG64(&processItem->IoReadHistory, j);
            write = PhGetItemCircularBuffer_ULONG64(&processItem->IoWriteHistory, j);
            other = PhGetItemCircularBuffer_ULONG64(&processItem->IoOtherHistory, j);
            bytes = PhGetItemCircularBuffer_SIZE_T(&processItem->PrivateBytesHistory, j);

            AtpAccumulate(&cpu, (DOUBLE)kernel + user);
            AtpAccumulate(&ioRead, (DOUBLE)read);
            AtpAccumulate(&ioWrite, (DOUBLE)write);
            AtpAccumulate(&io, (DOUBLE)read + write + other);

            if (cpu.Count == 1)
                newestPrivate = bytes;

            oldestPrivate = bytes;
        }

        row = PhCreateJsonObject();
        AtFillProcessIdentity(row, processItem);
        AtJsonAddString(row, "user", processItem->UserName);
        PhAddJsonObjectUInt64(row, "sample_count", cpu.Count);
        PhAddJsonObjectDouble(row, "cpu_usage_average", cpu.Count ? cpu.Total / cpu.Count : 0.0);
        PhAddJsonObjectDouble(row, "cpu_usage_maximum", cpu.Count ? cpu.Maximum : 0.0);
        PhAddJsonObjectDouble(row, "io_bytes_total", io.Total);
        PhAddJsonObjectDouble(row, "io_read_bytes_total", ioRead.Total);
        PhAddJsonObjectDouble(row, "io_write_bytes_total", ioWrite.Total);
        PhAddJsonObjectUInt64(row, "private_bytes", processItem->VmCounters.PagefileUsage);

        // Newest minus oldest in the window: negative when the process gave memory back.
        PhAddJsonObjectInt64(row, "private_bytes_growth", (LONG64)newestPrivate - (LONG64)oldestPrivate);

        PhAddJsonObjectDouble(row, "cpu_usage", processItem->CpuUsage);
        AtAddRow(&rows, row);
    }

    for (i = 0; i < numberOfProcessItems; i++)
        PhDereferenceObject(processItems[i]);

    PhFree(processItems);
    PhClearReference(&nameContains);

    structured = PhCreateJsonObject();
    PhAddJsonObjectUInt64(structured, "update_interval_ms", interval);
    PhAddJsonObjectUInt64(structured, "window_seconds", (ULONG64)windowSamples * interval / 1000);
    AtJsonAddStringZ(structured, "ranked_by", PhGetStringOrEmpty(rows.SortBy));
    AtAddRows(structured, "processes", &rows);
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
    case AtActionRankProcesses:
        AtpRankProcesses(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
