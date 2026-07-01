/*
 * Copyright (c) 2026 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2025-2026
 *
 */

#include "exttools.h"

#define ET_CACHE_LATENCY_SERIALIZE 1
#define ET_CACHE_LATENCY_TRIALS 10
#define ET_CACHE_LATENCY_REPEAT_DELAY 1000
#define ET_CACHE_LATENCY_OUTER_ITERS 1000
#define ET_CACHE_LATENCY_DEREF_UNROLL 100
#define ET_CACHE_LATENCY_TOTAL_DEREFS ((DOUBLE)(ET_CACHE_LATENCY_OUTER_ITERS * ET_CACHE_LATENCY_DEREF_UNROLL))

#define ET_CACHE_LATENCY_MSIZE_MIN 0x400
#define ET_CACHE_LATENCY_MSIZE_MAX 0x2000000
#define ET_CACHE_LATENCY_MSIZE_ROWS 16
#define ET_CACHE_LATENCY_MAX_STRIDE_COLS 8
#define ET_CACHE_LATENCY_MAX_CACHE_LEVELS 8
#define ET_WM_CACHE_LATENCY_UPDATE (WM_APP + 1)
#define ET_CACHE_LATENCY_LISTVIEW_CONTEXT 0xE

typedef struct _ET_CACHE_LATENCY_INFO
{
    ULONG Level;
    ULONG Type;
    ULONG SizeKb;
    ULONG Ways;
    ULONG LineSize;
    ULONG Sets;
    ULONG Partitions;
} ET_CACHE_LATENCY_INFO, *PET_CACHE_LATENCY_INFO;

typedef struct _ET_CACHE_LATENCY_RESULT
{
    ET_CACHE_LATENCY_INFO CpuidCacheInfo[ET_CACHE_LATENCY_MAX_CACHE_LEVELS];
    ULONG CpuidCacheCount;
    ET_CACHE_LATENCY_INFO NtCacheInfo[ET_CACHE_LATENCY_MAX_CACHE_LEVELS];
    ULONG NtCacheCount;
    ULONG Results[ET_CACHE_LATENCY_MSIZE_ROWS][ET_CACHE_LATENCY_MAX_STRIDE_COLS];
    DOUBLE Trials[ET_CACHE_LATENCY_MSIZE_ROWS][ET_CACHE_LATENCY_MAX_STRIDE_COLS][ET_CACHE_LATENCY_TRIALS];
    BOOLEAN Succeeded;
} ET_CACHE_LATENCY_RESULT, *PET_CACHE_LATENCY_RESULT;

typedef struct _ET_CACHE_LATENCY_ROW
{
    ULONG Index;
    WCHAR SizeText[PH_INT32_STR_LEN_1];
    WCHAR Text[ET_CACHE_LATENCY_MAX_STRIDE_COLS][64];
    DOUBLE Median[ET_CACHE_LATENCY_MAX_STRIDE_COLS];
} ET_CACHE_LATENCY_ROW, *PET_CACHE_LATENCY_ROW;

typedef struct _ET_CACHE_LATENCY_CONTEXT
{
    HWND WindowHandle;
    HWND ParentWindowHandle;
    HWND ListViewHandle;
    WNDPROC ListViewWindowProcedure;
    PPH_LISTVIEW_CONTEXT ListViewContext;
    HWND SummaryLabelHandles[4];
    PH_LAYOUT_MANAGER LayoutManager;
    ET_CACHE_LATENCY_ROW RowData[ET_CACHE_LATENCY_MSIZE_ROWS];
    PET_CACHE_LATENCY_ROW Rows[ET_CACHE_LATENCY_MSIZE_ROWS];
    BOOLEAN RefreshRunning;
} ET_CACHE_LATENCY_CONTEXT, *PET_CACHE_LATENCY_CONTEXT;

typedef struct _ET_CACHE_LATENCY_WORKER_CONTEXT
{
    HWND WindowHandle;
} ET_CACHE_LATENCY_WORKER_CONTEXT, *PET_CACHE_LATENCY_WORKER_CONTEXT;

#define ET_CACHE_LATENCY_CHASE_STEP(p) ((p) = *(volatile ULONG_PTR *)(ULONG_PTR)(p))

#define ET_CACHE_LATENCY_CHASE10(p) \
    ET_CACHE_LATENCY_CHASE_STEP(p); ET_CACHE_LATENCY_CHASE_STEP(p); ET_CACHE_LATENCY_CHASE_STEP(p); ET_CACHE_LATENCY_CHASE_STEP(p); ET_CACHE_LATENCY_CHASE_STEP(p); \
    ET_CACHE_LATENCY_CHASE_STEP(p); ET_CACHE_LATENCY_CHASE_STEP(p); ET_CACHE_LATENCY_CHASE_STEP(p); ET_CACHE_LATENCY_CHASE_STEP(p); ET_CACHE_LATENCY_CHASE_STEP(p)

#define ET_CACHE_LATENCY_CHASE100(p) \
    ET_CACHE_LATENCY_CHASE10(p); ET_CACHE_LATENCY_CHASE10(p); ET_CACHE_LATENCY_CHASE10(p); ET_CACHE_LATENCY_CHASE10(p); ET_CACHE_LATENCY_CHASE10(p); \
    ET_CACHE_LATENCY_CHASE10(p); ET_CACHE_LATENCY_CHASE10(p); ET_CACHE_LATENCY_CHASE10(p); ET_CACHE_LATENCY_CHASE10(p); ET_CACHE_LATENCY_CHASE10(p)

/**
 * Gets the display string for a cache type identifier.
 *
 * \param Type Cache type value.
 * \return Cache type display text.
 */
PCWSTR EtCacheLatencyTypeString(
    _In_ ULONG Type
    )
{
    switch (Type)
    {
    case 1:
        return L"Data";
    case 2:
        return L"Instruction";
    case 3:
        return L"Unified";
    default:
        return L"Unknown";
    }
}

/**
 * Converts an NT cache type to the internal cache type value.
 *
 * \param Type Processor cache type value.
 * \return Internal cache type value.
 */
ULONG EtCacheLatencyNtType(
    _In_ PROCESSOR_CACHE_TYPE Type
    )
{
    switch (Type)
    {
    case CacheData:
        return 1;
    case CacheInstruction:
        return 2;
    case CacheUnified:
        return 3;
    default:
        return 0;
    }
}

/**
 * Builds the stride list used by the benchmark.
 *
 * \param Strides Receives stride values in bytes.
 * \param Count Receives the number of stride values.
 */
VOID EtCacheLatencyGetStrideList(
    _Out_writes_(ET_CACHE_LATENCY_MAX_STRIDE_COLS) PULONG Strides,
    _Out_ PULONG Count
    )
{
    ULONG stride;
    ULONG count = 0;

    for (stride = (ULONG)sizeof(PVOID); stride <= 0x200 && count < ET_CACHE_LATENCY_MAX_STRIDE_COLS; stride <<= 1)
    {
        Strides[count++] = stride;
    }

    *Count = count;
}

/**
 * Compares two benchmark rows by size index.
 *
 * \param Item1 First row.
 * \param Item2 Second row.
 * \param Context Unused compare context.
 * \return Comparison result.
 */
LONG EtCacheLatencyCompareSize(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PET_CACHE_LATENCY_ROW row1 = Item1;
    PET_CACHE_LATENCY_ROW row2 = Item2;

    if (!row1 || !row2)
        return 0;

    return uintptrcmp(row1->Index, row2->Index);
}

/**
 * Compares two benchmark rows by a stride column median.
 *
 * \param Item1 First row.
 * \param Item2 Second row.
 * \param Context Unused compare context.
 * \param Column Zero-based stride column index.
 * \return Comparison result.
 */
LONG EtCacheLatencyCompareStrideColumn(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context,
    _In_ ULONG Column
    )
{
    PET_CACHE_LATENCY_ROW row1 = Item1;
    PET_CACHE_LATENCY_ROW row2 = Item2;
    DOUBLE value1;
    DOUBLE value2;

    if (!row1 || !row2 || Column >= ET_CACHE_LATENCY_MAX_STRIDE_COLS)
        return 0;

    value1 = row1->Median[Column];
    value2 = row2->Median[Column];

    if (value1 < value2)
        return -1;
    if (value1 > value2)
        return 1;

    return uintptrcmp(row1->Index, row2->Index);
}

/**
 * Compares rows using the 8-byte stride column.
 *
 * \param Item1 First row.
 * \param Item2 Second row.
 * \param Context Compare context.
 * \return Comparison result.
 */
LONG EtCacheLatencyCompareStride8(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    return EtCacheLatencyCompareStrideColumn(Item1, Item2, Context, 0);
}

/**
 * Compares rows using the 16-byte stride column.
 *
 * \param Item1 First row.
 * \param Item2 Second row.
 * \param Context Compare context.
 * \return Comparison result.
 */
LONG EtCacheLatencyCompareStride16(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    return EtCacheLatencyCompareStrideColumn(Item1, Item2, Context, 1);
}

/**
 * Compares rows using the 32-byte stride column.
 *
 * \param Item1 First row.
 * \param Item2 Second row.
 * \param Context Compare context.
 * \return Comparison result.
 */
LONG EtCacheLatencyCompareStride32(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    return EtCacheLatencyCompareStrideColumn(Item1, Item2, Context, 2);
}

/**
 * Compares rows using the 64-byte stride column.
 *
 * \param Item1 First row.
 * \param Item2 Second row.
 * \param Context Compare context.
 * \return Comparison result.
 */
LONG EtCacheLatencyCompareStride64(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    return EtCacheLatencyCompareStrideColumn(Item1, Item2, Context, 3);
}

/**
 * Compares rows using the 128-byte stride column.
 *
 * \param Item1 First row.
 * \param Item2 Second row.
 * \param Context Compare context.
 * \return Comparison result.
 */
LONG EtCacheLatencyCompareStride128(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    return EtCacheLatencyCompareStrideColumn(Item1, Item2, Context, 4);
}

/**
 * Compares rows using the 256-byte stride column.
 *
 * \param Item1 First row.
 * \param Item2 Second row.
 * \param Context Compare context.
 * \return Comparison result.
 */
LONG EtCacheLatencyCompareStride256(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    return EtCacheLatencyCompareStrideColumn(Item1, Item2, Context, 5);
}

/**
 * Compares rows using the 512-byte stride column.
 *
 * \param Item1 First row.
 * \param Item2 Second row.
 * \param Context Compare context.
 * \return Comparison result.
 */
LONG EtCacheLatencyCompareStride512(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    return EtCacheLatencyCompareStrideColumn(Item1, Item2, Context, 6);
}

/**
 * Compares two rows according to the active list-view sort column.
 *
 * \param Context Cache latency dialog context.
 * \param Row1 First row.
 * \param Row2 Second row.
 * \param SortColumn Sort column index.
 * \return Comparison result.
 */
LONG EtCacheLatencyCompareRows(
    _In_ PET_CACHE_LATENCY_CONTEXT Context,
    _In_ PET_CACHE_LATENCY_ROW Row1,
    _In_ PET_CACHE_LATENCY_ROW Row2,
    _In_ ULONG SortColumn
    )
{
    switch (SortColumn)
    {
    case 0:
        return EtCacheLatencyCompareSize(Row1, Row2, Context);
    case 1:
        return EtCacheLatencyCompareStride8(Row1, Row2, Context);
    case 2:
        return EtCacheLatencyCompareStride16(Row1, Row2, Context);
    case 3:
        return EtCacheLatencyCompareStride32(Row1, Row2, Context);
    case 4:
        return EtCacheLatencyCompareStride64(Row1, Row2, Context);
    case 5:
        return EtCacheLatencyCompareStride128(Row1, Row2, Context);
    case 6:
        return EtCacheLatencyCompareStride256(Row1, Row2, Context);
    case 7:
        return EtCacheLatencyCompareStride512(Row1, Row2, Context);
    default:
        return 0;
    }
}

/**
 * Sorts benchmark rows using the list-view sort state.
 *
 * \param Context Cache latency dialog context.
 */
VOID EtCacheLatencySortRows(
    _Inout_ PET_CACHE_LATENCY_CONTEXT Context
    )
{
    ULONG sortColumn;
    PH_SORT_ORDER sortOrder;
    ULONG i;

    ExtendedListView_GetSort(Context->ListViewHandle, &sortColumn, &sortOrder);

    if (sortOrder == NoSortOrder)
        return;

    for (i = 1; i < ET_CACHE_LATENCY_MSIZE_ROWS; i++)
    {
        LONG j;
        PET_CACHE_LATENCY_ROW row = Context->Rows[i];

        j = (LONG)i - 1;

        while (j >= 0)
        {
            LONG sortResult = EtCacheLatencyCompareRows(Context, Context->Rows[j], row, sortColumn);

            if (sortOrder == DescendingSortOrder)
                sortResult = -sortResult;

            if (sortResult <= 0)
                break;

            Context->Rows[j + 1] = Context->Rows[j];
            j--;
        }

        Context->Rows[j + 1] = row;
    }
}

/**
 * Subclass procedure for the benchmark list-view control.
 *
 * \param WindowHandle List-view handle.
 * \param WindowMessage Window message.
 * \param wParam Message WPARAM.
 * \param lParam Message LPARAM.
 * \return Message handling result.
 */
LRESULT CALLBACK EtCacheLatencyListViewWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PET_CACHE_LATENCY_CONTEXT context;
    WNDPROC oldWndProc;
    LRESULT result;

    context = PhGetWindowContext(WindowHandle, ET_CACHE_LATENCY_LISTVIEW_CONTEXT);

    if (!context)
        return 0;

    oldWndProc = context->ListViewWindowProcedure;

    if (WindowMessage == WM_NCDESTROY)
    {
        PhSetWindowProcedure(WindowHandle, oldWndProc);
        PhRemoveWindowContext(WindowHandle, ET_CACHE_LATENCY_LISTVIEW_CONTEXT);
        return CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);
    }

    result = CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);

    if (WindowMessage == WM_NOTIFY)
    {
        LPNMHDR header = (LPNMHDR)lParam;

        if (header->code == HDN_ITEMCLICK)
        {
            EtCacheLatencySortRows(context);
            InvalidateRect(WindowHandle, NULL, FALSE);
        }
    }

    return result;
}

/**
 * Computes the median of a sample set.
 *
 * \param Values Sample values.
 * \param Count Number of samples.
 * \return Median value.
 */
DOUBLE EtCacheLatencyMedian(
    _In_reads_(Count) const DOUBLE* Values,
    _In_ ULONG Count
    )
{
    ULONG i;
    DOUBLE values[ET_CACHE_LATENCY_TRIALS];

    if (!Count)
        return 0.0;

    for (i = 0; i < Count; i++)
        values[i] = Values[i];

    for (i = 1; i < Count; i++)
    {
        LONG j;
        DOUBLE value = values[i];

        j = (LONG)i - 1;

        while (j >= 0 && values[j] > value)
        {
            values[j + 1] = values[j];
            j--;
        }

        values[j + 1] = value;
    }

    if (Count & 1)
        return values[Count / 2];

    return (values[Count / 2 - 1] + values[Count / 2]) * 0.5;
}

/**
 * Computes the standard deviation of a sample set.
 *
 * \param Values Sample values.
 * \param Count Number of samples.
 * \return Standard deviation value.
 */
DOUBLE EtCacheLatencyStdDev(
    _In_reads_(Count) const DOUBLE* Values,
    _In_ ULONG Count
    )
{
    ULONG i;
    DOUBLE mean = 0.0;
    DOUBLE variance = 0.0;

    if (Count <= 1)
        return 0.0;

    for (i = 0; i < Count; i++)
        mean += Values[i];

    mean /= (DOUBLE)Count;

    for (i = 0; i < Count; i++)
    {
        DOUBLE delta = Values[i] - mean;
        variance += delta * delta;
    }

    variance /= (DOUBLE)Count;

    return sqrt(variance);
}

/**
 * Reads the current timestamp counter value.
 *
 * \return Timestamp counter value.
 */
ULONG64 EtCacheLatencyReadTimeStampCounter(
    VOID
    )
{
    return PhReadTimeStampCounter();
}

/**
 * Initializes a reverse pointer ring for pointer-chasing.
 *
 * \param Base Base address of the buffer.
 * \param Size Buffer size in bytes.
 * \param Stride Stride size in bytes.
 */
VOID EtCacheLatencyBuildRing(
    _In_ PBYTE Base,
    _In_ ULONG Size,
    _In_ ULONG Stride
    )
{
    ULONG offset;
    ULONG lastOffset;

    if (Stride >= Size)
    {
        *(PULONG_PTR)Base = (ULONG_PTR)Base;
        return;
    }

    for (offset = Stride; offset < Size; offset += Stride)
    {
        *(PULONG_PTR)(Base + offset) = (ULONG_PTR)(Base + offset - Stride);
    }

    lastOffset = ((Size - 1) / Stride) * Stride;
    *(PULONG_PTR)Base = (ULONG_PTR)(Base + lastOffset);
}

/**
 * Warms up the pointer ring before measurement.
 *
 * \param Base Base address of the ring.
 */
VOID EtCacheLatencyWarmupRing(
    _In_ PBYTE Base
    )
{
    ULONG_PTR pointer = (ULONG_PTR)Base;

    for (ULONG i = 0; i < 32; i++)
    {
        ET_CACHE_LATENCY_CHASE100(pointer);
    }
}

/**
 * Measures cache latency for one size/stride configuration.
 *
 * \param Result Benchmark result storage.
 * \param Base Base address of the ring buffer.
 * \param Size Working-set size in bytes.
 * \param Stride Stride size in bytes.
 * \param Row Result row index.
 * \param Column Result column index.
 * \return Median latency in cycles per load.
 */
DOUBLE EtCacheLatencyMeasure(
    _Inout_ PET_CACHE_LATENCY_RESULT Result,
    _In_ PBYTE Base,
    _In_ ULONG Size,
    _In_ ULONG Stride,
    _In_ ULONG Row,
    _In_ ULONG Column
    )
{
    ULONG trial;

    EtCacheLatencyBuildRing(Base, Size, Stride);
    EtCacheLatencyWarmupRing(Base);

    for (trial = 0; trial < ET_CACHE_LATENCY_TRIALS; trial++)
    {
        ULONG i;
        ULONG_PTR pointer = (ULONG_PTR)Base;
        ULONG64 start;
        ULONG64 end;

        start = EtCacheLatencyReadTimeStampCounter();

        for (i = 0; i < ET_CACHE_LATENCY_OUTER_ITERS; i++)
        {
            ET_CACHE_LATENCY_CHASE100(pointer);
        }

        end = EtCacheLatencyReadTimeStampCounter();
        Result->Trials[Row][Column][trial] = (DOUBLE)(end - start) / ET_CACHE_LATENCY_TOTAL_DEREFS;
    }

    return EtCacheLatencyMedian(Result->Trials[Row][Column], ET_CACHE_LATENCY_TRIALS);
}

/**
 * Finds the nearest benchmark row for the specified size.
 *
 * \param SizeKb Cache size in kilobytes.
 * \return Matching row index.
 */
ULONG EtCacheLatencyFindSizeRow(
    _In_ ULONG SizeKb
    )
{
    for (ULONG row = 0; row < ET_CACHE_LATENCY_MSIZE_ROWS; row++)
    {
        if ((1u << row) >= SizeKb)
            return row;
    }

    return ET_CACHE_LATENCY_MSIZE_ROWS - 1;
}

/**
 * Detects cache topology using CPUID deterministic cache parameters.
 *
 * \param Result Benchmark result storage.
 */
VOID EtCacheLatencyDetectCpuid(
    _Inout_ PET_CACHE_LATENCY_RESULT Result
    )
{
#if ET_HAS_CPUID
    int cpuInfo[4];
    ULONG i;

    Result->CpuidCacheCount = 0;

    __cpuid(cpuInfo, 0);

    if ((ULONG)cpuInfo[0] < 4)
        return;

    for (i = 0; i < 32 && Result->CpuidCacheCount < ET_CACHE_LATENCY_MAX_CACHE_LEVELS; i++)
    {
        ULONG type;
        ULONG level;
        ULONG lineSize;
        ULONG partitions;
        ULONG ways;
        ULONG sets;
        ULONG sizeBytes;
        PET_CACHE_LATENCY_INFO cache;

        CpuIdEx(cpuInfo, 0x04, (int)i);

        type = (ULONG)(cpuInfo[0] & 0x1f);

        if (!type)
            break;

        level = (ULONG)((cpuInfo[0] >> 5) & 0x7);
        lineSize = (ULONG)((cpuInfo[1] & 0xfff) + 1);
        partitions = (ULONG)(((cpuInfo[1] >> 12) & 0x3ff) + 1);
        ways = (ULONG)(((cpuInfo[1] >> 22) & 0x3ff) + 1);
        sets = (ULONG)(cpuInfo[2] + 1);
        sizeBytes = ways * partitions * lineSize * sets;

        cache = &Result->CpuidCacheInfo[Result->CpuidCacheCount++];
        cache->Level = level;
        cache->Type = type;
        cache->SizeKb = sizeBytes / 1024;
        cache->Ways = ways;
        cache->LineSize = lineSize;
        cache->Sets = sets;
        cache->Partitions = partitions;
    }
#else
    Result->CpuidCacheCount = 0;
#endif
}

/**
 * Detects cache topology using system logical processor information.
 *
 * \param Result Benchmark result storage.
 */
VOID EtCacheLatencyDetectNt(
    _Inout_ PET_CACHE_LATENCY_RESULT Result
    )
{
    NTSTATUS status;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX information;
    ULONG informationLength;
    PBYTE current;
    PBYTE end;

    Result->NtCacheCount = 0;

    status = PhGetSystemLogicalProcessorInformation(
        RelationCache,
        &information,
        &informationLength
        );

    if (!NT_SUCCESS(status))
        return;

    current = (PBYTE)information;
    end = current + informationLength;

    while (current < end && Result->NtCacheCount < ET_CACHE_LATENCY_MAX_CACHE_LEVELS)
    {
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX entry = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)current;

        if (entry->Relationship == RelationCache)
        {
            PET_CACHE_LATENCY_INFO cache = &Result->NtCacheInfo[Result->NtCacheCount++];

            cache->Level = entry->Cache.Level;
            cache->Type = EtCacheLatencyNtType(entry->Cache.Type);
            cache->SizeKb = entry->Cache.CacheSize / 1024;
            cache->Ways = entry->Cache.Associativity;
            cache->LineSize = entry->Cache.LineSize;
            cache->Sets = 0;
            cache->Partitions = 0;
        }

        if (!entry->Size)
            break;

        current += entry->Size;
    }

    PhFree(information);
}

/**
 * Runs the pointer-chasing cache latency benchmark.
 *
 * \param Result Benchmark result storage.
 * \return TRUE if the benchmark completed successfully, otherwise FALSE.
 */
BOOLEAN EtCacheLatencyRunBenchmark(
    _Inout_ PET_CACHE_LATENCY_RESULT Result
    )
{
    PVOID allocation = NULL;
    SIZE_T allocationSize;
    ULONG_PTR processMask = 0;
    ULONG_PTR targetMask;
    KAFFINITY oldAffinity = 0;
    KPRIORITY oldPriority = 0;
    ULONG strides[ET_CACHE_LATENCY_MAX_STRIDE_COLS];
    ULONG strideCount;
    ULONG row;
    ULONG size;
    PBYTE aligned;

    allocationSize = ET_CACHE_LATENCY_MSIZE_MAX + 64;

    if (!NT_SUCCESS(PhAllocateVirtualMemory(
        NtCurrentProcess(),
        &allocation,
        allocationSize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
        )))
    {
        return FALSE;
    }

    EtCacheLatencyGetStrideList(strides, &strideCount);

    if (NT_SUCCESS(PhGetProcessAffinityMask(NtCurrentProcess(), &processMask)))
    {
        targetMask = processMask & (0 - processMask);

        if (targetMask)
        {
            PhGetThreadAffinityMask(NtCurrentThread(), &oldAffinity);
            PhSetThreadAffinityMask(NtCurrentThread(), targetMask);
        }
    }

    if (NT_SUCCESS(PhGetThreadBasePriority(NtCurrentThread(), &oldPriority)))
    {
        PhSetThreadBasePriority(NtCurrentThread(), THREAD_PRIORITY_HIGHEST);
    }

    aligned = (PBYTE)(((ULONG_PTR)allocation + 63u) & ~(ULONG_PTR)63u);

    for (size = ET_CACHE_LATENCY_MSIZE_MIN, row = 0; size <= ET_CACHE_LATENCY_MSIZE_MAX && row < ET_CACHE_LATENCY_MSIZE_ROWS; size <<= 1, row++)
    {
        for (ULONG column = 0; column < strideCount; column++)
        {
            DOUBLE latency = EtCacheLatencyMeasure(
                Result,
                aligned,
                size,
                strides[column],
                row,
                column
                );

            Result->Results[row][column] = (ULONG)(latency + 0.5);
        }
    }

    if (oldAffinity)
    {
        PhSetThreadAffinityMask(NtCurrentThread(), oldAffinity);
    }

    if (oldPriority)
    {
        PhSetThreadBasePriority(NtCurrentThread(), oldPriority);
    }

    PhFreeVirtualMemory(
        NtCurrentProcess(),
        allocation,
        MEM_RELEASE
        );

    return TRUE;
}

/**
 * Updates list-view row buffers from benchmark data.
 *
 * \param Context Cache latency dialog context.
 * \param Result Benchmark result data.
 */
VOID EtCacheLatencyUpdateBenchmarkRows(
    _Inout_ PET_CACHE_LATENCY_CONTEXT Context,
    _In_ PET_CACHE_LATENCY_RESULT Result
    )
{
    ULONG strides[ET_CACHE_LATENCY_MAX_STRIDE_COLS];
    ULONG strideCount;
    ULONG row;

    EtCacheLatencyGetStrideList(strides, &strideCount);

    for (row = 0; row < ET_CACHE_LATENCY_MSIZE_ROWS; row++)
    {
        PET_CACHE_LATENCY_ROW cacheRow = &Context->RowData[row];
        ULONG column;

        cacheRow->Index = row;
        PhPrintUInt32(cacheRow->SizeText, 1u << row);

        for (column = 0; column < strideCount; column++)
        {
            cacheRow->Median[column] = EtCacheLatencyMedian(Result->Trials[row][column], ET_CACHE_LATENCY_TRIALS);

            wcsncpy_s(cacheRow->Text[column], RTL_NUMBER_OF(cacheRow->Text[column]), PhaFormatString(
                L"%.2f (%.2f)",
                cacheRow->Median[column],
                EtCacheLatencyStdDev(Result->Trials[row][column], ET_CACHE_LATENCY_TRIALS)
                )->Buffer,
                _TRUNCATE
                );
        }
    }
}

/**
 * Supplies virtual list-view text and item data.
 *
 * \param Context Cache latency dialog context.
 * \param DispInfo List-view display info request.
 */
VOID EtCacheLatencyGetDispInfo(
    _Inout_ PET_CACHE_LATENCY_CONTEXT Context,
    _Inout_ NMLVDISPINFO* DispInfo
    )
{
    PET_CACHE_LATENCY_ROW cacheRow;

    if (DispInfo->item.iItem < 0 || DispInfo->item.iItem >= ET_CACHE_LATENCY_MSIZE_ROWS)
        return;

    cacheRow = Context->Rows[DispInfo->item.iItem];

    if (!cacheRow)
        return;

    if (FlagOn(DispInfo->item.mask, LVIF_PARAM))
    {
        DispInfo->item.lParam = (LPARAM)cacheRow;
    }

    if (!FlagOn(DispInfo->item.mask, LVIF_TEXT))
        return;

    if (DispInfo->item.iSubItem == 0)
    {
        wcsncpy_s(DispInfo->item.pszText, DispInfo->item.cchTextMax, cacheRow->SizeText, _TRUNCATE);
    }
    else if ((ULONG)DispInfo->item.iSubItem <= ET_CACHE_LATENCY_MAX_STRIDE_COLS)
    {
        wcsncpy_s(DispInfo->item.pszText, DispInfo->item.cchTextMax, cacheRow->Text[DispInfo->item.iSubItem - 1], _TRUNCATE);
    }
}

/**
 * Builds summary label text for detected cache levels and DRAM.
 *
 * \param Context Cache latency dialog context.
 * \param Result Benchmark result data.
 */
VOID EtCacheLatencyAddSummaryRows(
    _Inout_ PET_CACHE_LATENCY_CONTEXT Context,
    _In_ PET_CACHE_LATENCY_RESULT Result
    )
{
    ULONG strides[ET_CACHE_LATENCY_MAX_STRIDE_COLS];
    ULONG strideCount;
    ULONG analysisColumnStart;
    ULONG i;
    ULONG dramRow;
    ULONG64 dramSum = 0;
    ULONG dramSamples = 0;
    PET_CACHE_LATENCY_INFO caches;
    ULONG cacheCount;
    BOOLEAN levelWritten[3] = { FALSE, FALSE, FALSE };

    EtCacheLatencyGetStrideList(strides, &strideCount);

    if (!strideCount)
        return;

    analysisColumnStart = strideCount >= 3 ? strideCount - 3 : 0;

    // Prefer CPUID topology; fall back to the NT logical processor information when CPUID is unavailable.
    if (Result->CpuidCacheCount)
    {
        caches = Result->CpuidCacheInfo;
        cacheCount = Result->CpuidCacheCount;
    }
    else
    {
        caches = Result->NtCacheInfo;
        cacheCount = Result->NtCacheCount;
    }

    // Each level maps to a fixed label slot: L1 -> 0, L2 -> 1, L3 -> 2, DRAM -> 3.
    for (i = 0; i < cacheCount; i++)
    {
        PET_CACHE_LATENCY_INFO cache = &caches[i];
        ULONG slot;
        ULONG row;
        ULONG column;
        ULONG64 sum = 0;
        ULONG samples = 0;

        if (cache->Type == 2)
            continue;

        if (cache->Level < 1 || cache->Level > 3)
            continue;

        slot = cache->Level - 1;

        // Keep the first cache reported for a level (data/unified); ignore duplicates.
        if (levelWritten[slot])
            continue;

        row = EtCacheLatencyFindSizeRow(cache->SizeKb);

        for (column = analysisColumnStart; column < strideCount; column++)
        {
            sum += Result->Results[row][column];
            samples++;
        }

        PhSetWindowText(Context->SummaryLabelHandles[slot], PhaFormatString(
            L"L%lu %s latency: %lu cycles (%lu KB)",
            cache->Level,
            EtCacheLatencyTypeString(cache->Type),
            samples ? (ULONG)(sum / samples) : 0,
            1u << row
            )->Buffer);

        levelWritten[slot] = TRUE;
    }

    // Clear any level slot that had no matching cache so stale text does not linger.
    for (i = 0; i < RTL_NUMBER_OF(levelWritten); i++)
    {
        if (!levelWritten[i])
            PhSetWindowText(Context->SummaryLabelHandles[i], L"");
    }

    dramRow = ET_CACHE_LATENCY_MSIZE_ROWS - 1;

    for (i = analysisColumnStart; i < strideCount; i++)
    {
        dramSum += Result->Results[dramRow][i];
        dramSamples++;
    }

    PhSetWindowText(Context->SummaryLabelHandles[3], PhaFormatString(
        L"DRAM latency: %lu cycles (%lu KB)",
        dramSamples ? (ULONG)(dramSum / dramSamples) : 0,
        1u << dramRow
        )->Buffer);
}

/**
 * Applies benchmark results to dialog controls.
 *
 * \param Context Cache latency dialog context.
 * \param Result Benchmark result data.
 */
VOID EtCacheLatencyApplyResult(
    _Inout_ PET_CACHE_LATENCY_CONTEXT Context,
    _In_ PET_CACHE_LATENCY_RESULT Result
    )
{
    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    PhSetWindowText(Context->SummaryLabelHandles[0], L"");
    PhSetWindowText(Context->SummaryLabelHandles[1], L"");
    PhSetWindowText(Context->SummaryLabelHandles[2], L"");
    PhSetWindowText(Context->SummaryLabelHandles[3], L"");

    if (Result->Succeeded)
    {
        EtCacheLatencyUpdateBenchmarkRows(Context, Result);
        PhListView_SetItemCount(Context->ListViewContext, ET_CACHE_LATENCY_MSIZE_ROWS, LVSICF_NOINVALIDATEALL);
        EtCacheLatencyAddSummaryRows(Context, Result);
        EtCacheLatencySortRows(Context);
        InvalidateRect(Context->ListViewHandle, NULL, FALSE);
    }
    else
    {
        PhListView_SetItemCount(Context->ListViewContext, 0, LVSICF_NOINVALIDATEALL);
    }

    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

/**
 * Worker thread entry point for cache detection and benchmarking.
 *
 * \param Parameter Worker context.
 * \return Thread exit status.
 */
NTSTATUS EtCacheLatencyRefreshThreadStart(
    _In_ PVOID Parameter
    )
{
    PET_CACHE_LATENCY_WORKER_CONTEXT workerContext = Parameter;
    PET_CACHE_LATENCY_RESULT result;

    result = PhAllocateZero(sizeof(ET_CACHE_LATENCY_RESULT));

    EtCacheLatencyDetectCpuid(result);
    EtCacheLatencyDetectNt(result);
    result->Succeeded = EtCacheLatencyRunBenchmark(result);

    if (!PostMessage(workerContext->WindowHandle, ET_WM_CACHE_LATENCY_UPDATE, 0, (LPARAM)result))
        PhFree(result);

    PhFree(workerContext);

    return STATUS_SUCCESS;
}

/**
 * Starts asynchronous benchmark refresh if one is not already running.
 *
 * \param Context Cache latency dialog context.
 */
VOID EtCacheLatencyStartRefresh(
    _Inout_ PET_CACHE_LATENCY_CONTEXT Context
    )
{
    PET_CACHE_LATENCY_WORKER_CONTEXT workerContext;

    if (Context->RefreshRunning)
        return;

    Context->RefreshRunning = TRUE;

    workerContext = PhAllocateZero(sizeof(ET_CACHE_LATENCY_WORKER_CONTEXT));
    workerContext->WindowHandle = Context->WindowHandle;

    PhCreateThread2(EtCacheLatencyRefreshThreadStart, workerContext);
}

/**
 * Dialog procedure for the cache latency window.
 *
 * \param WindowHandle Dialog window handle.
 * \param WindowMessage Window message.
 * \param wParam Message WPARAM.
 * \param lParam Message LPARAM.
 * \return Dialog procedure result.
 */
INT_PTR CALLBACK EtCacheLatencyDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PET_CACHE_LATENCY_CONTEXT context;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = (PET_CACHE_LATENCY_CONTEXT)lParam;
        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = WindowHandle;
            context->ListViewHandle = GetDlgItem(WindowHandle, IDC_CACHE_LATENCY_LIST);
            context->SummaryLabelHandles[0] = GetDlgItem(WindowHandle, IDC_CACHE_LATENCY_L1);
            context->SummaryLabelHandles[1] = GetDlgItem(WindowHandle, IDC_CACHE_LATENCY_L2);
            context->SummaryLabelHandles[2] = GetDlgItem(WindowHandle, IDC_CACHE_LATENCY_L3);
            context->SummaryLabelHandles[3] = GetDlgItem(WindowHandle, IDC_CACHE_LATENCY_DRAM);

            PhSetApplicationWindowIcon(WindowHandle);
            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhSetExtendedListView(context->ListViewHandle);
            context->ListViewContext = PhListView_Initialize(context->ListViewHandle);
            context->ListViewWindowProcedure = PhGetWindowProcedure(context->ListViewHandle);
            PhSetWindowContext(context->ListViewHandle, ET_CACHE_LATENCY_LISTVIEW_CONTEXT, context);
            PhSetWindowProcedure(context->ListViewHandle, EtCacheLatencyListViewWndProc);
            ExtendedListView_SetContext(context->ListViewHandle, context);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 0, EtCacheLatencyCompareSize);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 1, EtCacheLatencyCompareStride8);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 2, EtCacheLatencyCompareStride16);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 3, EtCacheLatencyCompareStride32);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 4, EtCacheLatencyCompareStride64);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 5, EtCacheLatencyCompareStride128);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 6, EtCacheLatencyCompareStride256);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 7, EtCacheLatencyCompareStride512);

            {
                ULONG strides[ET_CACHE_LATENCY_MAX_STRIDE_COLS];
                ULONG strideCount;
                ULONG column;
                ULONG row;

                EtCacheLatencyGetStrideList(strides, &strideCount);

                for (row = 0; row < ET_CACHE_LATENCY_MSIZE_ROWS; row++)
                {
                    context->RowData[row].Index = row;
                    context->Rows[row] = &context->RowData[row];
                    PhPrintUInt32(context->RowData[row].SizeText, 1u << row);
                }

                PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 135, L"size (KB) [cycles]");

                for (column = 0; column < strideCount; column++)
                {
                    PhAddListViewColumn(
                        context->ListViewHandle,
                        column + 1,
                        column + 1,
                        column + 1,
                        LVCFMT_LEFT,
                        105,
                        PhaFormatString(L"%lu", strides[column])->Buffer
                        );
                }
            }

            PhLoadListViewColumnsFromSetting(SETTING_NAME_CACHE_LATENCY_LISTVIEW_COLUMNS, context->ListViewHandle);
            PhListView_SetItemCount(context->ListViewContext, 0, LVSICF_NOINVALIDATEALL);

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, context->SummaryLabelHandles[0], NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, context->SummaryLabelHandles[1], NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, context->SummaryLabelHandles[2], NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, context->SummaryLabelHandles[3], NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            if (PhValidWindowPlacementFromSetting(SETTING_NAME_CACHE_LATENCY_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_NAME_CACHE_LATENCY_WINDOW_POSITION, SETTING_NAME_CACHE_LATENCY_WINDOW_SIZE, WindowHandle);
            else
                PhCenterWindow(WindowHandle, context->ParentWindowHandle);

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT));
            EtCacheLatencyStartRefresh(context);
        }
        break;
    case WM_DESTROY:
        {
            KillTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT);
            PhSaveListViewColumnsToSetting(SETTING_NAME_CACHE_LATENCY_LISTVIEW_COLUMNS, context->ListViewHandle);
            PhSaveWindowPlacementToSetting(SETTING_NAME_CACHE_LATENCY_WINDOW_POSITION, SETTING_NAME_CACHE_LATENCY_WINDOW_SIZE, WindowHandle);
            PhDeleteLayoutManager(&context->LayoutManager);

            if (context->ListViewWindowProcedure)
            {
                PhSetWindowProcedure(context->ListViewHandle, context->ListViewWindowProcedure);
                PhRemoveWindowContext(context->ListViewHandle, ET_CACHE_LATENCY_LISTVIEW_CONTEXT);
            }

            if (context->ListViewContext)
                PhListView_Destroy(context->ListViewContext);

            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_TIMER:
        {
            switch (wParam)
            {
            case PH_WINDOW_TIMER_DEFAULT:
                KillTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT);
                EtCacheLatencyStartRefresh(context);
                break;
            }
        }
        break;
    case ET_WM_CACHE_LATENCY_UPDATE:
        {
            PET_CACHE_LATENCY_RESULT result = (PET_CACHE_LATENCY_RESULT)lParam;

            context->RefreshRunning = FALSE;

            if (result)
            {
                EtCacheLatencyApplyResult(context, result);
                PhFree(result);
            }

            PhSetTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT, ET_CACHE_LATENCY_REPEAT_DELAY, NULL);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            PhHandleListViewNotifyForCopy(lParam, context->ListViewHandle);

            if (header->hwndFrom == context->ListViewHandle)
            {
                switch (header->code)
                {
                case LVN_GETDISPINFO:
                    EtCacheLatencyGetDispInfo(context, (NMLVDISPINFO*)header);
                    break;
                }
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(WindowHandle, IDOK);
                break;
            }
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                menu = PhCreateEMenu();
                PhInsertCopyListViewEMenuItem(menu, PHAPP_IDC_COPY, context->ListViewHandle);

                item = PhShowEMenu(
                    menu,
                    WindowHandle,
                    PH_EMENU_SHOW_LEFTRIGHT,
                    PH_ALIGN_LEFT | PH_ALIGN_TOP,
                    point.x,
                    point.y
                    );

                if (item)
                    PhHandleCopyListViewEMenuItem(item);

                PhDestroyEMenu(menu);
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

/**
 * Opens the cache latency dialog.
 *
 * \param ParentWindowHandle Parent window handle.
 */
VOID EtShowCacheLatencyDialog(
    _In_ HWND ParentWindowHandle
    )
{
    PET_CACHE_LATENCY_CONTEXT context;

    context = PhAllocateZero(sizeof(ET_CACHE_LATENCY_CONTEXT));
    context->ParentWindowHandle = ParentWindowHandle;

    PhDialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_CACHE_LATENCY),
        NULL,
        EtCacheLatencyDlgProc,
        context
        );
}
