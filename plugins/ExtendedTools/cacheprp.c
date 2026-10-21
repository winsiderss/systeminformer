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
#define ET_CACHE_LATENCY_MIN_ITERS 50
#define ET_CACHE_LATENCY_CALIBRATE_ITERS 20
#define ET_CACHE_LATENCY_DEREF_UNROLL 100

// Time budget for a single trial, in time stamp counter ticks (roughly a millisecond on
// current parts). Working sets differ in latency by two orders of magnitude, so a fixed
// iteration count lets the slowest rows dominate the run. Each cell is calibrated to this
// budget instead; the fast rows saturate the iteration clamp and behave as they did before.
#define ET_CACHE_LATENCY_TRIAL_BUDGET_CYCLES 3000000

// A row counts as memory when it costs at least this much more than the last cache level
// and the curve is within this much of the final row, which together mean the curve has
// left the cache and settled.
#define ET_CACHE_LATENCY_DRAM_RISE 1.5
#define ET_CACHE_LATENCY_DRAM_FLAT 0.15

#define ET_CACHE_LATENCY_MSIZE_MIN 0x400
#define ET_CACHE_LATENCY_MSIZE_MAX 0x4000000
#define ET_CACHE_LATENCY_MSIZE_ROWS 17
#define ET_CACHE_LATENCY_MAX_STRIDE_COLS 8
#define ET_CACHE_LATENCY_MAX_CACHE_LEVELS 8
#define ET_WM_CACHE_LATENCY_UPDATE (WM_APP + 1)
#define ET_CACHE_LATENCY_LISTVIEW_CONTEXT 0xE
#define ET_CACHE_LATENCY_MENU_COLUMNS_1 1
#define ET_CACHE_LATENCY_MENU_COLUMNS_2 2

// Overview tab: one graph per memory hierarchy level (L1, L2, L3, DRAM).
#define ET_CACHE_LATENCY_LEVELS 4
#define ET_CACHE_LATENCY_TAB_OVERVIEW 0
#define ET_CACHE_LATENCY_TAB_TABLE 1

// ET_CACHEGRAPH: line chart of latency against working set size. The x-axis is
// linear in the size row index, which makes it logarithmic in the working set size
// because every row is a doubling.
#define ET_CACHE_GRAPH_CLASSNAME L"EtCacheGraph"
#define ET_CGM_SETDATA (WM_USER + 0x10)
#define ET_CGM_SETCOLOR (WM_USER + 0x11)
#define ET_CGM_SETDPI (WM_USER + 0x12)

typedef struct _ET_CACHE_GRAPH_DATA
{
    DOUBLE Values[ET_CACHE_LATENCY_MSIZE_ROWS];
    ULONG Count;
    DOUBLE Maximum;
    ULONG HighlightStart;
    ULONG HighlightEnd;
    DOUBLE CurrentValue;
    DOUBLE CyclesToNanoseconds;
    BOOLEAN HighlightValid;
    BOOLEAN CurrentValid;
} ET_CACHE_GRAPH_DATA, *PET_CACHE_GRAPH_DATA;

typedef struct _ET_CACHE_GRAPH_CONTEXT
{
    HWND WindowHandle;
    HWND TooltipHandle;
    LONG WindowDpi;
    ET_CACHE_GRAPH_DATA Data;
    COLORREF LineColor;
    COLORREF BackColor;
    COLORREF GridColor;
    COLORREF AxisColor;
    COLORREF TextColor;
    HDC BufferedDc;
    HBITMAP BufferedBitmap;
    HBITMAP BufferedOldBitmap;
    LONG BufferedWidth;
    LONG BufferedHeight;
    LONG HoverRow;
    BOOLEAN HasHover;
} ET_CACHE_GRAPH_CONTEXT, *PET_CACHE_GRAPH_CONTEXT;

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
    DOUBLE TscFrequency;
    ULONG ProcessorNumber;
    BOOLEAN Succeeded;
    BOOLEAN LargePages;
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
    HWND TabControlHandle;
    HWND StatusLabelHandle;
    HWND PanelHandle;
    HWND GroupBoxHandles[ET_CACHE_LATENCY_LEVELS];
    HWND GraphHandles[ET_CACHE_LATENCY_LEVELS];
    HWND InfoLabelHandles[ET_CACHE_LATENCY_LEVELS];
    HWND ValueLabelHandles[ET_CACHE_LATENCY_LEVELS];
    PH_LAYOUT_MANAGER LayoutManager;
    ET_CACHE_LATENCY_ROW RowData[ET_CACHE_LATENCY_MSIZE_ROWS];
    PET_CACHE_LATENCY_ROW Rows[ET_CACHE_LATENCY_MSIZE_ROWS];

    // Overview tab state. The curve is the per-size-row latency averaged over the
    // largest strides. It is resampled into the graph state buffers at draw time
    // because the graph control draws one data point every two pixels; each level
    // owns the span of size rows between RegionStart and RegionEnd, which is drawn
    // in the level color while the remainder of the curve is drawn muted.
    DOUBLE Curve[ET_CACHE_LATENCY_MSIZE_ROWS];
    DOUBLE CurveMaximum;
    DOUBLE CyclesToNanoseconds;
    DOUBLE LevelLatency[ET_CACHE_LATENCY_LEVELS];
    ULONG LevelRow[ET_CACHE_LATENCY_LEVELS];
    ULONG RegionStart[ET_CACHE_LATENCY_LEVELS];
    ULONG RegionEnd[ET_CACHE_LATENCY_LEVELS];
    BOOLEAN LevelValid[ET_CACHE_LATENCY_LEVELS];
    // The last slot is only DRAM when the curve actually plateaus above the last cache
    // level. When it does not, the slot still shows the largest working set tested, but
    // it must not be labelled as memory latency.
    BOOLEAN DramDetected;

    LONG WindowDpi;
    LONG TabIndex;
    LONG GraphColumns;
    ULONG PassCount;
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

    context = PhGetWindowContext(WindowHandle, ET_CACHE_LATENCY_LISTVIEW_CONTEXT);

    if (context)
    {
        oldWndProc = context->ListViewWindowProcedure;

        switch (WindowMessage)
        {
        case WM_NCDESTROY:
            {
                PhSetWindowProcedure(WindowHandle, oldWndProc);
                PhRemoveWindowContext(WindowHandle, ET_CACHE_LATENCY_LISTVIEW_CONTEXT);
            }
            break;
        case WM_NOTIFY:
            {
                LPNMHDR header = (LPNMHDR)lParam;

                if (header->code == HDN_ITEMCLICK)
                {
                    EtCacheLatencySortRows(context);
                    InvalidateRect(WindowHandle, NULL, FALSE);
                }
            }
            break;
        }

        return CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);
    }

    return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);;
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
    DOUBLE values[ET_CACHE_LATENCY_TRIALS];

    if (!Count)
        return 0.0;

    for (ULONG i = 0; i < Count; i++)
    {
        values[i] = Values[i];
    }

    for (ULONG i = 1; i < Count; i++)
    {
        DOUBLE value = values[i];
        LONG j = (LONG)i - 1;

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
    DOUBLE mean = 0.0;
    DOUBLE variance = 0.0;

    if (Count <= 1)
        return 0.0;

    for (ULONG i = 0; i < Count; i++)
    {
        mean += Values[i];
    }

    mean /= (DOUBLE)Count;

    for (ULONG i = 0; i < Count; i++)
    {
        DOUBLE delta = Values[i] - mean;
        variance += delta * delta;
    }

    variance /= (DOUBLE)Count;

    return sqrt(variance);
}

/**
 * Initializes a reverse pointer ring for pointer-chasing.
 *
 * \param Base Base address of the buffer.
 * \param Size Buffer size in bytes.
 * \param Stride Stride size in bytes.
 */
_Use_decl_annotations_
NTSTATUS EtCacheLatencyBuildRing(
    _In_ PVOID Base,
    _In_ ULONG Size,
    _In_ ULONG Stride
    )
{
    ULONG offset;
    ULONG last;

    if (!Base || Stride == 0 || Size == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Handle edge case where stride covers or exceeds the entire buffer
    //

    if (Stride >= Size)
    {
        *(PVOID*)Base = Base;
        return STATUS_SUCCESS;
    }

    //
    // Build the latency ring chain (backward-linked)
    //

    last = 0;
    offset = Stride;

    while (offset < Size)
    {
        *(PULONG_PTR)PTR_ADD_OFFSET(Base, offset) = (ULONG_PTR)PTR_ADD_OFFSET(Base, last);
        last = offset;

        if (ULONG_MAX - offset < Stride)
            break;

        offset += Stride;
    }

    //
    // Link the head back to the last offset securely to close the ring
    //

    *(PVOID*)Base = PTR_ADD_OFFSET(Base, last);

    return STATUS_SUCCESS;
}

/**
 * Warms up the pointer ring before measurement.
 *
 * \param Base Base address of the ring.
 */
VOID EtCacheLatencyWarmupRing(
    _In_ PVOID Base
    )
{
    ULONG_PTR pointer = (ULONG_PTR)Base;

    for (ULONG i = 0; i < 32; i++)
    {
        ET_CACHE_LATENCY_CHASE100(pointer);
    }
}

/**
 * Calibrates the iteration count for one ring so that a trial costs roughly a fixed
 * amount of time regardless of where in the hierarchy the working set lands.
 *
 * The ring must already be warmed up. A short chase measures the cost of one unrolled
 * iteration and the count is scaled so the trial fits the budget. Rows that hit in the
 * near caches are far faster than the budget allows for and saturate the upper clamp,
 * which leaves their sampling identical to the previous fixed count. The chase loads are
 * volatile, so the calibration cannot be optimized away.
 *
 * \param Base Base address of the ring.
 * \return Outer iteration count to use for each trial.
 */
ULONG EtCacheLatencyCalibrateIterations(
    _In_ PVOID Base
    )
{
    ULONG_PTR pointer = (ULONG_PTR)Base;
    ULONG64 start;
    ULONG64 stop;
    ULONG64 elapsed;
    ULONG64 iterations;

    start = PhReadTimeStampCounter();

    for (ULONG i = 0; i < ET_CACHE_LATENCY_CALIBRATE_ITERS; i++)
    {
        ET_CACHE_LATENCY_CHASE100(pointer);
    }

    stop = PhReadTimeStampCounter();

    if (stop <= start)
        return ET_CACHE_LATENCY_OUTER_ITERS;

    elapsed = stop - start;
    iterations = ((ULONG64)ET_CACHE_LATENCY_TRIAL_BUDGET_CYCLES * ET_CACHE_LATENCY_CALIBRATE_ITERS) / elapsed;

    if (iterations > ET_CACHE_LATENCY_OUTER_ITERS)
        return ET_CACHE_LATENCY_OUTER_ITERS;

    if (iterations < ET_CACHE_LATENCY_MIN_ITERS)
        return ET_CACHE_LATENCY_MIN_ITERS;

    return (ULONG)iterations;
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
    _In_ PVOID Base,
    _In_ ULONG Size,
    _In_ ULONG Stride,
    _In_ ULONG Row,
    _In_ ULONG Column
    )
{
    ULONG outerIterations;
    DOUBLE totalDereferences;

    EtCacheLatencyBuildRing(Base, Size, Stride);
    EtCacheLatencyWarmupRing(Base);

    outerIterations = EtCacheLatencyCalibrateIterations(Base);
    totalDereferences = (DOUBLE)outerIterations * ET_CACHE_LATENCY_DEREF_UNROLL;

    for (ULONG trial = 0; trial < ET_CACHE_LATENCY_TRIALS; trial++)
    {
        ULONG_PTR pointer = (ULONG_PTR)Base;
        ULONG64 start;
        ULONG64 stop;

        start = PhReadTimeStampCounter();

        for (ULONG i = 0; i < outerIterations; i++)
        {
            ET_CACHE_LATENCY_CHASE100(pointer);
        }

        stop = PhReadTimeStampCounter();

        Result->Trials[Row][Column][trial] = (DOUBLE)(stop - start) / totalDereferences;
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
    NTSTATUS status;
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
    PVOID aligned;
    ULONG64 start;
    ULONG64 stop;
    LARGE_INTEGER performanceCounterFrequency;
    LARGE_INTEGER performanceCounterStart;
    LARGE_INTEGER performanceCounterStop;

    allocationSize = ET_CACHE_LATENCY_MSIZE_MAX + 64;

    // 1. Attempt Large Page Allocation
    if (USER_SHARED_DATA->LargePageMinimum != 0)
    {
        SIZE_T largePageMinimum = USER_SHARED_DATA->LargePageMinimum;
        SIZE_T regionSize;

        // Align size to the system large page minimum
        regionSize = ALIGN_UP(allocationSize, largePageMinimum);

        // Large pages require the size to be a multiple of large page minimum
        if (regionSize < largePageMinimum)
            regionSize = largePageMinimum;

        // Ensure we hold the required privilege (best effort)
        status = PhAdjustPrivilege(NULL, SE_LOCK_MEMORY_PRIVILEGE, TRUE);

        if (NT_SUCCESS(status))
        {
            PVOID baseAddress = NULL;

            status = NtAllocateVirtualMemory(
                NtCurrentProcess(),
                &baseAddress,
                0,
                &regionSize,
                MEM_COMMIT | MEM_RESERVE | MEM_LARGE_PAGES,
                PAGE_READWRITE
                );

            if (NT_SUCCESS(status))
            {
                allocationSize = regionSize;
                allocation = baseAddress;
                Result->LargePages = TRUE;
            }
        }
    }

    // 2. Fallback to Standard Pages
    if (!allocation)
    {
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
    }

    EtCacheLatencyGetStrideList(strides, &strideCount);

    if (NT_SUCCESS(PhGetProcessAffinityMask(NtCurrentProcess(), &processMask)))
    {
        targetMask = processMask & (0 - processMask);

        if (targetMask)
        {
            PhGetThreadAffinityMask(NtCurrentThread(), &oldAffinity);
            PhSetThreadAffinityMask(NtCurrentThread(), targetMask);

            // Report the processor the benchmark is pinned to.
            for (ULONG bit = 0; bit < sizeof(KAFFINITY) * 8; bit++)
            {
                if (targetMask & ((KAFFINITY)1 << bit))
                {
                    Result->ProcessorNumber = bit;
                    break;
                }
            }
        }
    }

    if (NT_SUCCESS(PhGetThreadBasePriority(NtCurrentThread(), &oldPriority)))
    {
        PhSetThreadBasePriority(NtCurrentThread(), THREAD_PRIORITY_HIGHEST);
    }

    aligned = ALIGN_UP_POINTER_BY(allocation, ULONG64_C(64));

    // The benchmark counts time stamp counter ticks, so the tick rate is calibrated
    // against the performance counter over the whole run to convert them to time.
    PhQueryPerformanceFrequency(&performanceCounterFrequency);
    PhQueryPerformanceCounter(&performanceCounterStart);
    start = PhReadTimeStampCounter();

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

    stop = PhReadTimeStampCounter();
    PhQueryPerformanceCounter(&performanceCounterStop);

    if (stop > start && performanceCounterStop.QuadPart > performanceCounterStart.QuadPart && performanceCounterFrequency.QuadPart > 0)
    {
        Result->TscFrequency = (DOUBLE)(stop - start) * (DOUBLE)performanceCounterFrequency.QuadPart /
            (DOUBLE)(performanceCounterStop.QuadPart - performanceCounterStart.QuadPart);
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
 * Averages the benchmark result for a size row over the largest stride columns.
 *
 * The largest strides defeat the hardware prefetcher, so they are the columns that
 * most closely reflect the true access latency of the level being probed.
 *
 * \param Result Benchmark result data.
 * \param Row Size row index.
 * \return Average cycles per load for the row.
 */
DOUBLE EtCacheLatencyAverageTailColumns(
    _In_ PET_CACHE_LATENCY_RESULT Result,
    _In_ ULONG Row
    )
{
    ULONG strides[ET_CACHE_LATENCY_MAX_STRIDE_COLS];
    ULONG strideCount;
    ULONG column;
    ULONG64 sum = 0;
    ULONG samples = 0;

    EtCacheLatencyGetStrideList(strides, &strideCount);

    if (!strideCount || Row >= ET_CACHE_LATENCY_MSIZE_ROWS)
        return 0;

    for (column = strideCount >= 3 ? strideCount - 3 : 0; column < strideCount; column++)
    {
        sum += Result->Results[Row][column];
        samples++;
    }

    return samples ? (DOUBLE)sum / samples : 0;
}

/**
 * Sets a group box caption, but only when it actually changes.
 *
 * A button paints itself directly while handling WM_SETTEXT, which overdraws the
 * themed frame drawn by the group box subclass. The topology does not change between
 * benchmark passes, so the caption is left alone unless it differs, and the control is
 * invalidated afterwards to force a themed repaint.
 *
 * \param WindowHandle Group box window handle.
 * \param Text Caption text.
 */
VOID EtCacheLatencySetGroupBoxText(
    _In_ HWND WindowHandle,
    _In_ PCWSTR Text
    )
{
    PPH_STRING text;

    text = PH_AUTO(PhGetWindowText(WindowHandle));

    if (text && PhEqualStringZ(text->Buffer, Text, FALSE))
        return;

    PhSetWindowText(WindowHandle, Text);
    InvalidateRect(WindowHandle, NULL, TRUE);
}

/**
 * Returns the latency of a level sampled where its working set is actually resident.
 *
 * A working set exactly at a level's capacity already spills through conflict misses and
 * page walks, so the curve at the capacity row is biased high and is not what that level
 * costs when it hits. The rows below capacity are fully resident, and the lowest of them
 * is the cleanest estimate of the plateau because noise only ever pushes a sample up.
 *
 * \param Context Cache latency dialog context.
 * \param Start First size row owned by the level.
 * \param CapacityRow Size row matching the level capacity.
 * \return Cycles per load on the level plateau.
 */
DOUBLE EtCacheLatencyPlateauLatency(
    _In_ PET_CACHE_LATENCY_CONTEXT Context,
    _In_ ULONG Start,
    _In_ ULONG CapacityRow
    )
{
    DOUBLE latency = 0;
    ULONG row;

    if (CapacityRow >= ET_CACHE_LATENCY_MSIZE_ROWS)
        return 0;

    // Rows are powers of two, so the row below capacity is at most half the capacity and
    // is therefore resident. A span too narrow to hold one leaves nothing but the
    // capacity row itself to report.
    if (CapacityRow == 0 || Start > CapacityRow - 1)
        return Context->Curve[CapacityRow];

    for (row = Start; row <= CapacityRow - 1; row++)
    {
        if (Context->Curve[row] <= 0)
            continue;

        if (latency <= 0 || Context->Curve[row] < latency)
            latency = Context->Curve[row];
    }

    return latency > 0 ? latency : Context->Curve[CapacityRow];
}

/**
 * Averages the curve over an inclusive span of size rows.
 *
 * Used for the memory plateau, where every row is the same access cost and averaging
 * across them is simply a longer sample.
 *
 * \param Context Cache latency dialog context.
 * \param Start First size row.
 * \param End Last size row.
 * \return Average cycles per load over the span.
 */
DOUBLE EtCacheLatencyAverageRows(
    _In_ PET_CACHE_LATENCY_CONTEXT Context,
    _In_ ULONG Start,
    _In_ ULONG End
    )
{
    DOUBLE sum = 0;
    ULONG samples = 0;
    ULONG row;

    if (Start >= ET_CACHE_LATENCY_MSIZE_ROWS || End >= ET_CACHE_LATENCY_MSIZE_ROWS || Start > End)
        return 0;

    for (row = Start; row <= End; row++)
    {
        sum += Context->Curve[row];
        samples++;
    }

    return samples ? sum / samples : 0;
}

/**
 * Locates the size row at which the curve reaches main memory.
 *
 * The largest tested working set is not memory latency by itself. On a part with a large
 * last level cache the sweep can end while still substantially cache resident, which is
 * why this looks for an actual transition instead of assuming one: the curve has to rise
 * well clear of the last cache level and then flatten out, which is what a level change
 * followed by a plateau looks like.
 *
 * \param Context Cache latency dialog context.
 * \param LastCacheRow Capacity row of the last cache level.
 * \param DramRow Receives the first row of the memory plateau.
 * \return TRUE when memory latency was reached, otherwise FALSE.
 */
BOOLEAN EtCacheLatencyFindDramRow(
    _In_ PET_CACHE_LATENCY_CONTEXT Context,
    _In_ ULONG LastCacheRow,
    _Out_ PULONG DramRow
    )
{
    DOUBLE reference;
    DOUBLE final;
    ULONG row;

    *DramRow = ET_CACHE_LATENCY_MSIZE_ROWS - 1;

    // Compare against the last cache level where it still hits, not at its capacity.
    reference = Context->Curve[LastCacheRow > 0 ? LastCacheRow - 1 : 0];
    final = Context->Curve[ET_CACHE_LATENCY_MSIZE_ROWS - 1];

    if (reference <= 0 || final <= 0)
        return FALSE;

    for (row = LastCacheRow + 1; row < ET_CACHE_LATENCY_MSIZE_ROWS; row++)
    {
        DOUBLE delta;

        if (Context->Curve[row] <= 0)
            continue;

        // A real level change, not drift.
        if (Context->Curve[row] < reference * ET_CACHE_LATENCY_DRAM_RISE)
            continue;

        // Flat from here to the end of the sweep, which is what a plateau means. At the
        // final row this is trivially true and the rise above is the whole test.
        delta = final > Context->Curve[row] ? final - Context->Curve[row] : Context->Curve[row] - final;

        if (delta / final <= ET_CACHE_LATENCY_DRAM_FLAT)
        {
            *DramRow = row;
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Builds the overview graph curves and panel text from benchmark data.
 *
 * Each level owns a contiguous span of size rows ending at the row matching its
 * reported cache size; the last slot owns everything above the last cache level. The span
 * is recorded in \a RegionStart and \a RegionEnd, which the draw callback uses to split
 * the resampled curve between the highlighted and muted lines.
 *
 * Spans are for drawing. The value reported for a level is sampled where that level is
 * resident rather than at its capacity row, and the last slot is only labelled DRAM when
 * the curve is found to actually reach memory.
 *
 * \param Context Cache latency dialog context.
 * \param Result Benchmark result data.
 */
VOID EtCacheLatencyBuildOverview(
    _Inout_ PET_CACHE_LATENCY_CONTEXT Context,
    _In_ PET_CACHE_LATENCY_RESULT Result
    )
{
    PET_CACHE_LATENCY_INFO caches;
    ULONG cacheCount;
    ULONG level;
    ULONG row;
    ULONG lastCacheRow;
    ULONG dramRow = ET_CACHE_LATENCY_MSIZE_ROWS - 1;
    LONG previousRow;
    ET_CACHE_LATENCY_INFO levelInfo[ET_CACHE_LATENCY_LEVELS] = { 0 };

    Context->DramDetected = FALSE;
    memset(Context->LevelValid, 0, sizeof(Context->LevelValid));
    memset(Context->LevelRow, 0, sizeof(Context->LevelRow));
    memset(Context->LevelLatency, 0, sizeof(Context->LevelLatency));
    memset(Context->RegionStart, 0, sizeof(Context->RegionStart));
    memset(Context->RegionEnd, 0, sizeof(Context->RegionEnd));

    Context->CurveMaximum = 0;

    // A wildly out of range calibration means the run was descheduled; drop the
    // nanosecond figure rather than reporting nonsense.
    if (Result->TscFrequency >= 1e8 && Result->TscFrequency <= 1e11)
        Context->CyclesToNanoseconds = 1e9 / Result->TscFrequency;
    else
        Context->CyclesToNanoseconds = 0;

    for (row = 0; row < ET_CACHE_LATENCY_MSIZE_ROWS; row++)
    {
        Context->Curve[row] = EtCacheLatencyAverageTailColumns(Result, row);

        if (Context->Curve[row] > Context->CurveMaximum)
            Context->CurveMaximum = Context->Curve[row];
    }

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

    // Each level maps to a fixed slot: L1 -> 0, L2 -> 1, L3 -> 2, DRAM -> 3.
    for (ULONG i = 0; i < cacheCount; i++)
    {
        PET_CACHE_LATENCY_INFO cache = &caches[i];
        ULONG slot;

        if (cache->Type == 2)
            continue;

        if (cache->Level < 1 || cache->Level > 3)
            continue;

        slot = cache->Level - 1;

        // Keep the first cache reported for a level (data/unified); ignore duplicates.
        if (Context->LevelValid[slot])
            continue;

        levelInfo[slot] = *cache;
        Context->LevelRow[slot] = EtCacheLatencyFindSizeRow(cache->SizeKb);
        Context->LevelValid[slot] = TRUE;
    }

    // The last slot owns the largest working sets, but it is only memory if the curve
    // actually got there. Probe for the transition above the last cache level; when there
    // is none the slot still reports the largest working set tested and says so.
    lastCacheRow = 0;

    for (level = 0; level < ET_CACHE_LATENCY_LEVELS - 1; level++)
    {
        if (Context->LevelValid[level] && Context->LevelRow[level] > lastCacheRow)
            lastCacheRow = Context->LevelRow[level];
    }

    Context->DramDetected = EtCacheLatencyFindDramRow(Context, lastCacheRow, &dramRow);
    Context->LevelRow[3] = Context->DramDetected ? dramRow : ET_CACHE_LATENCY_MSIZE_ROWS - 1;
    Context->LevelValid[3] = TRUE;

    // Assign each level a monotonically increasing span of size rows. A level's regime
    // is the stretch of the curve between the previous level's capacity and its own, so
    // the spans meet at that boundary row rather than skipping it; otherwise a level only
    // one row wider than its predecessor (DRAM, usually) would have no span to draw.
    previousRow = -1;

    for (level = 0; level < ET_CACHE_LATENCY_LEVELS; level++)
    {
        ULONG start;
        ULONG end;

        if (!Context->LevelValid[level])
            continue;

        start = previousRow < 0 ? 0 : (ULONG)previousRow;

        if (start >= ET_CACHE_LATENCY_MSIZE_ROWS)
            start = ET_CACHE_LATENCY_MSIZE_ROWS - 1;

        end = Context->LevelRow[level];

        if (end < start)
            end = start;

        if (level == ET_CACHE_LATENCY_LEVELS - 1)
            end = ET_CACHE_LATENCY_MSIZE_ROWS - 1;

        Context->RegionStart[level] = start;
        Context->RegionEnd[level] = end;

        if (level == ET_CACHE_LATENCY_LEVELS - 1)
        {
            // The memory plateau runs from where it was detected to the end of the sweep.
            // LevelRow holds that first plateau row, which is the working set the panel
            // reports; the span still covers the whole tail so the graph is unbroken.
            Context->LevelLatency[level] = EtCacheLatencyAverageRows(
                Context,
                Context->LevelRow[level],
                end
                );
        }
        else
        {
            Context->LevelRow[level] = end;
            Context->LevelLatency[level] = EtCacheLatencyPlateauLatency(Context, start, end);
        }

        previousRow = (LONG)end;
    }

    if (Context->CurveMaximum <= 0)
        Context->CurveMaximum = 1;

    // Group box captions and panel text.
    for (level = 0; level < ET_CACHE_LATENCY_LEVELS; level++)
    {
        if (!Context->LevelValid[level])
        {
            PhSetWindowText(Context->InfoLabelHandles[level], L"");
            PhSetWindowText(Context->ValueLabelHandles[level], L"");
            continue;
        }

        if (level == ET_CACHE_LATENCY_LEVELS - 1)
        {
            EtCacheLatencySetGroupBoxText(
                Context->GroupBoxHandles[level],
                Context->DramDetected ? L"DRAM" : L"Memory"
                );

            if (Context->DramDetected)
            {
                PhSetWindowText(Context->InfoLabelHandles[level], PhaFormatString(
                    L"DRAM \x2022 %lu KB working set",
                    1u << Context->LevelRow[level]
                    )->Buffer);
            }
            else
            {
                PhSetWindowText(Context->InfoLabelHandles[level], PhaFormatString(
                    L"%lu KB working set \x2022 DRAM not reached",
                    1u << Context->LevelRow[level]
                    )->Buffer);
            }
        }
        else
        {
            EtCacheLatencySetGroupBoxText(Context->GroupBoxHandles[level], PhaFormatString(
                L"L%lu %s",
                levelInfo[level].Level,
                EtCacheLatencyTypeString(levelInfo[level].Type)
                )->Buffer);

            PhSetWindowText(Context->InfoLabelHandles[level], PhaFormatString(
                L"L%lu %s \x2022 %lu KB \x2022 %lu-way \x2022 %lu B line",
                levelInfo[level].Level,
                EtCacheLatencyTypeString(levelInfo[level].Type),
                levelInfo[level].SizeKb,
                levelInfo[level].Ways,
                levelInfo[level].LineSize
                )->Buffer);
        }

        if (Context->CyclesToNanoseconds > 0)
        {
            PhSetWindowText(Context->ValueLabelHandles[level], PhaFormatString(
                L"%.2f cycles (%.1f ns)",
                Context->LevelLatency[level],
                Context->LevelLatency[level] * Context->CyclesToNanoseconds
                )->Buffer);
        }
        else
        {
            PhSetWindowText(Context->ValueLabelHandles[level], PhaFormatString(
                L"%.2f cycles",
                Context->LevelLatency[level]
                )->Buffer);
        }
    }
}

/**
 * Clears the overview graph curves and panel text.
 *
 * \param Context Cache latency dialog context.
 */
VOID EtCacheLatencyClearOverview(
    _Inout_ PET_CACHE_LATENCY_CONTEXT Context
    )
{
    memset(Context->Curve, 0, sizeof(Context->Curve));
    memset(Context->RegionStart, 0, sizeof(Context->RegionStart));
    memset(Context->RegionEnd, 0, sizeof(Context->RegionEnd));
    memset(Context->LevelValid, 0, sizeof(Context->LevelValid));
    memset(Context->LevelLatency, 0, sizeof(Context->LevelLatency));
    Context->DramDetected = FALSE;
    Context->CurveMaximum = 1;

    for (ULONG level = 0; level < ET_CACHE_LATENCY_LEVELS; level++)
    {
        PhSetWindowText(Context->InfoLabelHandles[level], L"");
        PhSetWindowText(Context->ValueLabelHandles[level], L"");
    }
}

/**
 * Gets the graph color setting name for a memory hierarchy level.
 *
 * \param Level Level index (0 = L1, 3 = DRAM).
 * \return Color setting name.
 */
PCWSTR EtCacheLatencyLevelColorSetting(
    _In_ ULONG Level
    )
{
    switch (Level)
    {
    case 0:
        return SETTING_COLOR_CPU_KERNEL;
    case 1:
        return SETTING_COLOR_PRIVATE;
    case 2:
        return SETTING_COLOR_IO_WRITE;
    default:
        return SETTING_COLOR_PHYSICAL;
    }
}

/**
 * Blends two colors.
 *
 * \param Color1 First color.
 * \param Color2 Second color.
 * \param Percent Weight of the second color, 0 to 100.
 * \return Blended color.
 */
COLORREF EtCacheGraphBlendColor(
    _In_ COLORREF Color1,
    _In_ COLORREF Color2,
    _In_ ULONG Percent
    )
{
    ULONG red = (GetRValue(Color1) * (100 - Percent) + GetRValue(Color2) * Percent) / 100;
    ULONG green = (GetGValue(Color1) * (100 - Percent) + GetGValue(Color2) * Percent) / 100;
    ULONG blue = (GetBValue(Color1) * (100 - Percent) + GetBValue(Color2) * Percent) / 100;

    return RGB(red, green, blue);
}

/**
 * Frees the double buffer of a cache graph control.
 *
 * \param Context Cache graph control context.
 */
static VOID EtpCgFreeBuffer(
    _Inout_ PET_CACHE_GRAPH_CONTEXT Context
    )
{
    if (Context->BufferedDc)
    {
        SelectBitmap(Context->BufferedDc, Context->BufferedOldBitmap);
        DeleteDC(Context->BufferedDc);
        Context->BufferedDc = NULL;
    }

    if (Context->BufferedBitmap)
    {
        DeleteBitmap(Context->BufferedBitmap);
        Context->BufferedBitmap = NULL;
    }

    Context->BufferedWidth = 0;
    Context->BufferedHeight = 0;
}

/**
 * Creates or resizes the double buffer of a cache graph control.
 *
 * \param Context Cache graph control context.
 * \param Width Buffer width.
 * \param Height Buffer height.
 */
static VOID EtpCgEnsureBuffer(
    _Inout_ PET_CACHE_GRAPH_CONTEXT Context,
    _In_ LONG Width,
    _In_ LONG Height
    )
{
    HDC hdc;

    if (Width <= 0 || Height <= 0)
        return;
    if (Context->BufferedDc && Context->BufferedWidth == Width && Context->BufferedHeight == Height)
        return;

    EtpCgFreeBuffer(Context);

    hdc = GetDC(Context->WindowHandle);
    Context->BufferedDc = CreateCompatibleDC(hdc);
    Context->BufferedBitmap = PhCreateDIBSection(hdc, PHBF_TOPDOWNDIB, Width, Height, NULL);
    ReleaseDC(Context->WindowHandle, hdc);

    Context->BufferedOldBitmap = SelectBitmap(Context->BufferedDc, Context->BufferedBitmap);
    Context->BufferedWidth = Width;
    Context->BufferedHeight = Height;
}

/**
 * Applies the theme colors to a cache graph control.
 *
 * \param Context Cache graph control context.
 */
static HBRUSH EtpCgUpdateColors(
    _Inout_ PET_CACHE_GRAPH_CONTEXT Context,
    _In_ HDC Hdc
    )
{
    LOGBRUSH logBrush;
    HBRUSH brush;

    // The parent answers with the same brush and text color it uses for the dialog,
    // so the chart matches the window in both the themed and unthemed cases.
    brush = (HBRUSH)SendMessage(
        GetParent(Context->WindowHandle),
        WM_CTLCOLORSTATIC,
        (WPARAM)Hdc,
        (LPARAM)Context->WindowHandle
        );

    Context->TextColor = GetTextColor(Hdc);
    Context->BackColor = GetSysColor(COLOR_3DFACE);

    if (brush && GetObject(brush, sizeof(LOGBRUSH), &logBrush) && logBrush.lbStyle == BS_SOLID)
        Context->BackColor = logBrush.lbColor;

    Context->GridColor = EtCacheGraphBlendColor(Context->BackColor, Context->TextColor, 14);
    Context->AxisColor = EtCacheGraphBlendColor(Context->BackColor, Context->TextColor, 35);

    return brush;
}

/**
 * Rounds a value up to one, two or five times a power of ten.
 *
 * The y-axis is labelled in quarters of this value so the tick labels stay round.
 *
 * \param Value Value to round.
 * \return Rounded value, never zero.
 */
static DOUBLE EtpCgNiceMaximum(
    _In_ DOUBLE Value
    )
{
    DOUBLE magnitude;
    DOUBLE normalized;

    if (Value <= 0)
        return 1;

    magnitude = pow(10, floor(log10(Value)));
    normalized = Value / magnitude;

    if (normalized <= 1)
        normalized = 1;
    else if (normalized <= 2)
        normalized = 2;
    else if (normalized <= 5)
        normalized = 5;
    else
        normalized = 10;

    return normalized * magnitude;
}

/**
 * Formats a latency value, appending the nanosecond equivalent when known.
 *
 * \param Context Cache graph control context.
 * \param Cycles Latency in cycles.
 * \param Buffer Receives the text.
 * \param Count Buffer length in characters.
 */
static VOID EtpCgFormatLatency(
    _In_ PET_CACHE_GRAPH_CONTEXT Context,
    _In_ DOUBLE Cycles,
    _Out_writes_(Count) PWSTR Buffer,
    _In_ SIZE_T Count
    )
{
    if (Context->Data.CyclesToNanoseconds > 0)
    {
        _snwprintf_s(Buffer, Count, _TRUNCATE, L"%.2f cycles (%.1f ns)",
            Cycles, Cycles * Context->Data.CyclesToNanoseconds);
    }
    else
    {
        _snwprintf_s(Buffer, Count, _TRUNCATE, L"%.2f cycles", Cycles);
    }
}

/**
 * Computes the plot rectangle of a cache graph control.
 *
 * The axis gutters are measured from the current font so the labels always fit.
 *
 * \param Context Cache graph control context.
 * \param Hdc Device context with the label font selected.
 * \param Rect Receives the plot rectangle.
 * \param TextHeight Receives the label line height.
 */
static VOID EtpCgGetPlotRect(
    _In_ PET_CACHE_GRAPH_CONTEXT Context,
    _In_ HDC Hdc,
    _Out_ PRECT Rect,
    _Out_ PLONG TextHeight
    )
{
    TEXTMETRIC textMetric;
    SIZE size;
    LONG pad;
    LONG height = 12;
    LONG labelWidth = 24;
    WCHAR label[32];

    if (GetTextMetrics(Hdc, &textMetric))
        height = textMetric.tmHeight;

    pad = PhScaleToDisplay(4, Context->WindowDpi);

    _snwprintf_s(label, RTL_NUMBER_OF(label), _TRUNCATE, L"%.0f", EtpCgNiceMaximum(Context->Data.Maximum));

    if (GetTextExtentPoint32(Hdc, label, (int)wcslen(label), &size))
        labelWidth = size.cx;

    // The top gutter carries the current value and the bottom gutter the size labels.
    Rect->left = labelWidth + pad * 2;
    Rect->top = height + pad;
    Rect->right = Context->BufferedWidth - pad * 2;
    Rect->bottom = Context->BufferedHeight - height - pad;

    *TextHeight = height;
}

/**
 * Maps a size row to a horizontal plot position.
 *
 * The x-axis is linear in the row index, which makes it logarithmic in the working
 * set size because each row is a doubling.
 *
 * \param Plot Plot rectangle.
 * \param Row Size row index.
 * \param Count Number of size rows.
 * \return Horizontal position.
 */
static LONG EtpCgRowToX(
    _In_ PRECT Plot,
    _In_ ULONG Row,
    _In_ ULONG Count
    )
{
    if (Count <= 1)
        return Plot->left;

    return Plot->left + (LONG)(((LONG64)(Plot->right - Plot->left) * Row) / (Count - 1));
}

/**
 * Maps a horizontal plot position to the nearest size row.
 *
 * \param Plot Plot rectangle.
 * \param X Horizontal position.
 * \param Count Number of size rows.
 * \return Size row index.
 */
static ULONG EtpCgXToRow(
    _In_ PRECT Plot,
    _In_ LONG X,
    _In_ ULONG Count
    )
{
    LONG width = Plot->right - Plot->left;
    LONG row;

    if (Count <= 1 || width <= 0)
        return 0;

    row = (LONG)((((LONG64)(X - Plot->left) * (Count - 1)) + width / 2) / width);

    if (row < 0)
        row = 0;
    if (row > (LONG)(Count - 1))
        row = (LONG)(Count - 1);

    return (ULONG)row;
}

/**
 * Draws the size labels along the bottom of the plot.
 *
 * The largest size is placed first so it always appears; the remaining labels are
 * drawn left to right and skipped whenever they would collide.
 *
 * \param Context Cache graph control context.
 * \param Hdc Target device context.
 * \param Plot Plot rectangle.
 * \param TextHeight Label line height.
 */
static VOID EtpCgDrawSizeLabels(
    _In_ PET_CACHE_GRAPH_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PRECT Plot,
    _In_ LONG TextHeight
    )
{
    ULONG count = Context->Data.Count;
    LONG gap = PhScaleToDisplay(8, Context->WindowDpi);
    LONG limit;
    LONG previousRight;
    RECT labelRect;
    SIZE size;
    PH_FORMAT format;
    SIZE_T returnLength;
    WCHAR text[64];

    if (!count)
        return;

    labelRect.top = Plot->bottom + PhScaleToDisplay(2, Context->WindowDpi);
    labelRect.bottom = labelRect.top + TextHeight;

    // The largest size is anchored to the right edge and reserves its space.
    PhInitFormatSize(&format, (ULONG64)1024 << (count - 1));

    if (!PhFormatToBuffer(&format, 1, text, sizeof(text), &returnLength))
        return;

    if (!GetTextExtentPoint32(Hdc, text, (int)((returnLength - sizeof(UNICODE_NULL)) / sizeof(WCHAR)), &size))
        size.cx = PhScaleToDisplay(40, Context->WindowDpi);

    labelRect.left = Plot->right - size.cx;
    labelRect.right = Plot->right;
    DrawText(Hdc, text, -1, &labelRect, DT_RIGHT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);

    limit = labelRect.left - gap;
    previousRight = LONG_MIN;

    for (ULONG row = 0; row < count - 1; row++)
    {
        LONG x = EtpCgRowToX(Plot, row, count);
        LONG left;

        PhInitFormatSize(&format, (ULONG64)1024 << row);

        if (!PhFormatToBuffer(&format, 1, text, sizeof(text), &returnLength))
            continue;

        if (!GetTextExtentPoint32(Hdc, text, (int)((returnLength - sizeof(UNICODE_NULL)) / sizeof(WCHAR)), &size))
            continue;

        left = x - size.cx / 2;

        if (left < Plot->left)
            left = Plot->left;
        if (left < previousRight + gap)
            continue;
        if (left + size.cx > limit)
            continue;

        labelRect.left = left;
        labelRect.right = left + size.cx;
        DrawText(Hdc, text, -1, &labelRect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
        previousRight = labelRect.right;
    }
}

/**
 * Paints a cache graph control into its double buffer.
 *
 * \param Context Cache graph control context.
 * \param Hdc Target device context.
 */
static VOID EtpCgPaint(
    _In_ PET_CACHE_GRAPH_CONTEXT Context,
    _In_ HDC Hdc
    )
{
    RECT clientRect = { 0, 0, Context->BufferedWidth, Context->BufferedHeight };
    RECT plot;
    LONG textHeight;
    LONG plotHeight;
    LONG penWidth;
    COLORREF lineColor;
    DOUBLE maximum;
    ULONG count = Context->Data.Count;
    HFONT font;
    HFONT oldFont = NULL;
    HBRUSH brush;
    HPEN pen;
    HPEN oldPen;
    POINT points[ET_CACHE_LATENCY_MSIZE_ROWS];

    // Fills with the parent's own brush so the chart, the group box and the dialog
    // all share one background.
    brush = EtpCgUpdateColors(Context, Hdc);

    if (brush)
        FillRect(Hdc, &clientRect, brush);

    lineColor = Context->LineColor;

    // In light mode, darken the line color so it stands out against the light background.
    // The color settings are tuned for dark backgrounds.
    if (PhGetColorBrightness(Context->BackColor) > 128)
        lineColor = PhHalveColorBrightness(lineColor);

    font = (HFONT)SendMessage(GetParent(Context->WindowHandle), WM_GETFONT, 0, 0);

    if (font)
        oldFont = SelectFont(Hdc, font);

    SetBkMode(Hdc, TRANSPARENT);
    SetTextColor(Hdc, Context->TextColor);

    EtpCgGetPlotRect(Context, Hdc, &plot, &textHeight);
    plotHeight = plot.bottom - plot.top;

    if (plotHeight <= 0 || plot.right <= plot.left || !count)
    {
        if (oldFont)
            SelectFont(Hdc, oldFont);

        return;
    }

    if (count > RTL_NUMBER_OF(points))
        count = RTL_NUMBER_OF(points);

    maximum = EtpCgNiceMaximum(Context->Data.Maximum);

    // Highlight band for the span of sizes this level is responsible for.
    if (Context->Data.HighlightValid && Context->Data.HighlightEnd < count)
    {
        RECT band;
        LONG minimumWidth = PhScaleToDisplay(6, Context->WindowDpi);

        band.left = EtpCgRowToX(&plot, Context->Data.HighlightStart, count);
        band.right = EtpCgRowToX(&plot, Context->Data.HighlightEnd, count);
        band.top = plot.top;
        band.bottom = plot.bottom;

        // A single-row span would be invisible, so give it a minimum width.
        if (band.right - band.left < minimumWidth)
            band.right = band.left + minimumWidth;

        brush = CreateSolidBrush(EtCacheGraphBlendColor(Context->BackColor, lineColor, 18));
        FillRect(Hdc, &band, brush);
        DeleteBrush(brush);
    }

    // Horizontal grid and value labels.
    pen = CreatePen(PS_SOLID, 1, Context->GridColor);
    oldPen = SelectPen(Hdc, pen);

    for (ULONG i = 0; i <= 4; i++)
    {
        LONG y = plot.bottom - (LONG)((LONG64)plotHeight * i / 4);
        RECT labelRect;
        WCHAR label[32];

        MoveToEx(Hdc, plot.left, y, NULL);
        LineTo(Hdc, plot.right, y);

        _snwprintf_s(label, RTL_NUMBER_OF(label), _TRUNCATE, L"%.0f", maximum * i / 4);

        labelRect.left = 0;
        labelRect.right = plot.left - PhScaleToDisplay(4, Context->WindowDpi);
        labelRect.top = y - textHeight / 2;
        labelRect.bottom = labelRect.top + textHeight;
        DrawText(Hdc, label, -1, &labelRect, DT_RIGHT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
    }

    SelectPen(Hdc, oldPen);
    DeletePen(pen);

    EtpCgDrawSizeLabels(Context, Hdc, &plot, textHeight);

    // Axes.
    pen = CreatePen(PS_SOLID, 1, Context->AxisColor);
    oldPen = SelectPen(Hdc, pen);
    MoveToEx(Hdc, plot.left, plot.top, NULL);
    LineTo(Hdc, plot.left, plot.bottom);
    LineTo(Hdc, plot.right, plot.bottom);
    SelectPen(Hdc, oldPen);
    DeletePen(pen);

    // Curve.
    for (ULONG row = 0; row < count; row++)
    {
        DOUBLE value = Context->Data.Values[row] / maximum;

        if (value < 0)
            value = 0;
        if (value > 1)
            value = 1;

        points[row].x = EtpCgRowToX(&plot, row, count);
        points[row].y = plot.bottom - (LONG)(plotHeight * value);
    }

    penWidth = max(1, PhScaleToDisplay(1, Context->WindowDpi));
    pen = CreatePen(PS_SOLID, penWidth, lineColor);
    oldPen = SelectPen(Hdc, pen);
    Polyline(Hdc, points, (int)count);

    // Markers on the points this level owns.
    if (Context->Data.HighlightValid && Context->Data.HighlightEnd < count)
    {
        LONG radius = max(2, PhScaleToDisplay(2, Context->WindowDpi));
        HBRUSH markerBrush = CreateSolidBrush(lineColor);
        HBRUSH oldBrush = SelectBrush(Hdc, markerBrush);

        for (ULONG row = Context->Data.HighlightStart; row <= Context->Data.HighlightEnd; row++)
        {
            Ellipse(
                Hdc,
                points[row].x - radius,
                points[row].y - radius,
                points[row].x + radius,
                points[row].y + radius
                );
        }

        SelectBrush(Hdc, oldBrush);
        DeleteBrush(markerBrush);
    }

    SelectPen(Hdc, oldPen);
    DeletePen(pen);

    // Current value, aligned with the group box caption above it.
    if (Context->Data.CurrentValid)
    {
        RECT valueRect;
        WCHAR value[64];

        EtpCgFormatLatency(Context, Context->Data.CurrentValue, value, RTL_NUMBER_OF(value));

        valueRect.left = plot.left;
        valueRect.right = plot.right;
        valueRect.top = 0;
        valueRect.bottom = textHeight;

        SetTextColor(Hdc, lineColor);
        DrawText(Hdc, value, -1, &valueRect, DT_RIGHT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
    }

    if (oldFont)
        SelectFont(Hdc, oldFont);
}

/**
 * Updates the hover tooltip of a cache graph control.
 *
 * \param Context Cache graph control context.
 * \param X Cursor x position.
 * \param Y Cursor y position.
 */
static VOID EtpCgUpdateHover(
    _Inout_ PET_CACHE_GRAPH_CONTEXT Context,
    _In_ LONG X,
    _In_ LONG Y
    )
{
    TOOLINFO toolInfo = { sizeof(TOOLINFO) };
    RECT plot;
    LONG textHeight;
    ULONG row;
    HFONT font;
    HFONT oldFont = NULL;

    if (!Context->TooltipHandle || !Context->Data.Count || !Context->BufferedDc)
        return;

    // The plot rect is measured from the label font, so the same font must be selected
    // as when painting or the hit test would not line up with what is on screen.
    font = (HFONT)SendMessage(GetParent(Context->WindowHandle), WM_GETFONT, 0, 0);

    if (font)
        oldFont = SelectFont(Context->BufferedDc, font);

    EtpCgGetPlotRect(Context, Context->BufferedDc, &plot, &textHeight);

    if (oldFont)
        SelectFont(Context->BufferedDc, oldFont);

    toolInfo.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
    toolInfo.hwnd = GetParent(Context->WindowHandle);
    toolInfo.uId = (UINT_PTR)Context->WindowHandle;

    if (X < plot.left || X > plot.right || Y < plot.top || Y > plot.bottom)
    {
        if (Context->HasHover)
        {
            Context->HasHover = FALSE;
            Context->HoverRow = -1;
            SendMessage(Context->TooltipHandle, TTM_TRACKACTIVATE, FALSE, (LPARAM)&toolInfo);
        }

        return;
    }

    row = EtpCgXToRow(&plot, X, Context->Data.Count);

    if (!Context->HasHover || Context->HoverRow != (LONG)row)
    {
        PH_FORMAT format[3];
        WCHAR latency[64];
        WCHAR tip[128];

        EtpCgFormatLatency(Context, Context->Data.Values[row], latency, RTL_NUMBER_OF(latency));
        PhInitFormatSize(&format[0], (ULONG64)1024 << row);
        PhInitFormatC(&format[1], L'\n');
        PhInitFormatS(&format[2], latency);

        if (!PhFormatToBuffer(format, RTL_NUMBER_OF(format), tip, sizeof(tip), NULL))
            return;

        toolInfo.lpszText = tip;
        SendMessage(Context->TooltipHandle, TTM_UPDATETIPTEXT, 0, (LPARAM)&toolInfo);
        Context->HoverRow = (LONG)row;
    }

    {
        POINT point = { X, Y };

        ClientToScreen(Context->WindowHandle, &point);
        SendMessage(Context->TooltipHandle, TTM_TRACKPOSITION, 0, MAKELONG(point.x + 12, point.y + 12));
    }

    if (!Context->HasHover)
    {
        Context->HasHover = TRUE;
        SendMessage(Context->TooltipHandle, TTM_TRACKACTIVATE, TRUE, (LPARAM)&toolInfo);
    }
}

/**
 * Window procedure for the cache graph control.
 *
 * \param WindowHandle Control window handle.
 * \param WindowMessage Window message.
 * \param wParam Message WPARAM.
 * \param lParam Message LPARAM.
 * \return Message result.
 */
static LRESULT CALLBACK EtpCgWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PET_CACHE_GRAPH_CONTEXT context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);

    switch (WindowMessage)
    {
    case WM_NCCREATE:
        {
            context = PhAllocateZero(sizeof(ET_CACHE_GRAPH_CONTEXT));
            context->WindowHandle = WindowHandle;
            context->WindowDpi = PhGetWindowDpi(WindowHandle);
            context->LineColor = RGB(0x3b, 0x82, 0xf6);
            context->HoverRow = -1;

            PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
        }
        break;
    case WM_CREATE:
        {
            context->TooltipHandle = CreateWindowEx(
                WS_EX_TOPMOST,
                TOOLTIPS_CLASS,
                NULL,
                WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                WindowHandle,
                NULL,
                NULL,
                NULL
                );

            if (context->TooltipHandle)
            {
                TOOLINFO toolInfo = { sizeof(TOOLINFO) };

                toolInfo.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
                toolInfo.hwnd = GetParent(WindowHandle);
                toolInfo.uId = (UINT_PTR)WindowHandle;
                toolInfo.lpszText = L"";
                SendMessage(context->TooltipHandle, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
            }
        }
        break;
    case WM_NCDESTROY:
        {
            if (context)
            {
                if (context->TooltipHandle)
                    DestroyWindow(context->TooltipHandle);

                EtpCgFreeBuffer(context);
                PhFree(context);
                PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
            }
        }
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT:
        {
            PAINTSTRUCT paintStruct;
            RECT clientRect;
            HDC hdc;

            hdc = BeginPaint(WindowHandle, &paintStruct);

            if (context && GetClientRect(WindowHandle, &clientRect))
            {
                EtpCgEnsureBuffer(context, clientRect.right, clientRect.bottom);

                if (context->BufferedDc)
                {
                    EtpCgPaint(context, context->BufferedDc);
                    RECT paintRect = paintStruct.rcPaint;

                    BitBlt(
                        hdc,
                        paintRect.left,
                        paintRect.top,
                        paintRect.right - paintRect.left,
                        paintRect.bottom - paintRect.top,
                        context->BufferedDc,
                        paintRect.left,
                        paintRect.top,
                        SRCCOPY
                        );
                }
            }

            EndPaint(WindowHandle, &paintStruct);
        }
        return 0;
    case WM_SIZE:
        {
            InvalidateRect(WindowHandle, NULL, FALSE);
        }
        return 0;
    case WM_MOUSEMOVE:
        {
            if (context)
            {
                TRACKMOUSEEVENT trackMouseEvent = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, WindowHandle, 0 };

                EtpCgUpdateHover(context, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                TrackMouseEvent(&trackMouseEvent);
            }
        }
        return 0;
    case WM_MOUSELEAVE:
        {
            if (context && context->HasHover && context->TooltipHandle)
            {
                TOOLINFO toolInfo = { sizeof(TOOLINFO) };

                toolInfo.uFlags = TTF_IDISHWND;
                toolInfo.hwnd = GetParent(WindowHandle);
                toolInfo.uId = (UINT_PTR)WindowHandle;

                context->HasHover = FALSE;
                context->HoverRow = -1;
                SendMessage(context->TooltipHandle, TTM_TRACKACTIVATE, FALSE, (LPARAM)&toolInfo);
            }
        }
        return 0;
    case ET_CGM_SETDATA:
        {
            if (context && lParam)
            {
                memcpy(&context->Data, (PVOID)lParam, sizeof(ET_CACHE_GRAPH_DATA));
                context->HoverRow = -1;
                InvalidateRect(WindowHandle, NULL, FALSE);
            }
        }
        return TRUE;
    case ET_CGM_SETCOLOR:
        {
            if (context)
            {
                context->LineColor = (COLORREF)wParam;
                InvalidateRect(WindowHandle, NULL, FALSE);
            }
        }
        return TRUE;
    case ET_CGM_SETDPI:
        {
            if (context)
            {
                context->WindowDpi = (LONG)wParam;
                InvalidateRect(WindowHandle, NULL, FALSE);
            }
        }
        return TRUE;
    }

    return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);
}

/**
 * Registers the cache graph control window class.
 *
 * \return Class atom.
 */
RTL_ATOM EtCacheGraphInitialization(
    VOID
    )
{
    WNDCLASSEX windowClass;
    static RTL_ATOM atom = 0;

    if (atom)
        return atom;

    memset(&windowClass, 0, sizeof(WNDCLASSEX));
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_GLOBALCLASS;
    windowClass.lpfnWndProc = EtpCgWndProc;
    windowClass.cbWndExtra = sizeof(PVOID);
    windowClass.hInstance = NULL;
    windowClass.hCursor = PhLoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = ET_CACHE_GRAPH_CLASSNAME;

    atom = RegisterClassEx(&windowClass);

    return atom;
}

/**
 * Pushes the current curve and level span into a cache graph control.
 *
 * \param Context Cache latency dialog context.
 * \param Level Level index.
 */
VOID EtCacheLatencyUpdateGraph(
    _Inout_ PET_CACHE_LATENCY_CONTEXT Context,
    _In_ ULONG Level
    )
{
    ET_CACHE_GRAPH_DATA data;

    if (!Context->GraphHandles[Level])
        return;

    memset(&data, 0, sizeof(ET_CACHE_GRAPH_DATA));

    for (ULONG row = 0; row < ET_CACHE_LATENCY_MSIZE_ROWS; row++)
        data.Values[row] = Context->Curve[row];

    data.Count = ET_CACHE_LATENCY_MSIZE_ROWS;
    data.Maximum = Context->CurveMaximum;
    data.HighlightStart = Context->RegionStart[Level];
    data.HighlightEnd = Context->RegionEnd[Level];
    data.HighlightValid = Context->LevelValid[Level];
    data.CurrentValue = Context->LevelLatency[Level];
    data.CurrentValid = Context->LevelValid[Level];
    data.CyclesToNanoseconds = Context->CyclesToNanoseconds;

    SendMessage(Context->GraphHandles[Level], ET_CGM_SETDATA, 0, (LPARAM)&data);
}

static VOID EtCacheLatencyRedrawGraphs(
    _Inout_ PET_CACHE_LATENCY_CONTEXT Context
    )
{
    for (ULONG level = 0; level < ET_CACHE_LATENCY_LEVELS; level++)
        EtCacheLatencyUpdateGraph(Context, level);
}

/**
 * Retrieves the tab page rectangle in dialog coordinates.
 *
 * \param Context Cache latency dialog context.
 * \param Rect Receives the page rectangle.
 * \return TRUE when the page rectangle is available.
 */
static BOOLEAN EtpCacheLatencyGetPageRect(
    _In_ PET_CACHE_LATENCY_CONTEXT Context,
    _Out_ PRECT Rect
    )
{
    if (!Context->TabControlHandle)
        return FALSE;

    if (!PhTabNew_GetPageRect(Context->TabControlHandle, Rect))
        return FALSE;

    MapWindowRect(Context->TabControlHandle, Context->WindowHandle, Rect);

    return Rect->right > Rect->left && Rect->bottom > Rect->top;
}

/**
 * Creates the overview tab graph controls.
 *
 * \param Context Cache latency dialog context.
 */
VOID EtCacheLatencyCreateGraphs(
    _Inout_ PET_CACHE_LATENCY_CONTEXT Context
    )
{
    for (ULONG level = 0; level < ET_CACHE_LATENCY_LEVELS; level++)
    {
        // Visibility is owned by the tab selection, so WS_VISIBLE is not set here.
        Context->GraphHandles[level] = PhCreateWindow(
            ET_CACHE_GRAPH_CLASSNAME,
            NULL,
            WS_CHILD | WS_CLIPSIBLINGS,
            0,
            0,
            0,
            0,
            Context->WindowHandle,
            NULL,
            NULL,
            NULL
            );

        SendMessage(
            Context->GraphHandles[level],
            ET_CGM_SETCOLOR,
            PhGetIntegerSetting(EtCacheLatencyLevelColorSetting(level)),
            0
            );
        SendMessage(Context->GraphHandles[level], ET_CGM_SETDPI, Context->WindowDpi, 0);
        PhBringWindowToTop(Context->GraphHandles[level]);
    }
}

/**
 * Gets the line height of a window's font.
 *
 * \param WindowHandle Window handle.
 * \return Text height in pixels.
 */
LONG EtCacheLatencyGetTextHeight(
    _In_ HWND WindowHandle
    )
{
    HDC hdc;
    HFONT font;
    HFONT oldFont;
    TEXTMETRIC textMetric;
    LONG height = 0;

    if (!(hdc = GetDC(WindowHandle)))
        return 16;

    font = (HFONT)SendMessage(WindowHandle, WM_GETFONT, 0, 0);
    oldFont = font ? SelectFont(hdc, font) : NULL;

    if (GetTextMetrics(hdc, &textMetric))
        height = textMetric.tmHeight;

    if (oldFont)
        SelectFont(hdc, oldFont);

    ReleaseDC(WindowHandle, hdc);

    return height > 0 ? height : 16;
}

/**
 * Positions the overview tab controls inside the tab display area.
 *
 * \param Context Cache latency dialog context.
 */
VOID EtCacheLatencyLayoutOverview(
    _Inout_ PET_CACHE_LATENCY_CONTEXT Context
    )
{
    RECT clientRect;
    LONG margin;
    LONG cellWidth;
    LONG cellHeight;
    LONG captionHeight;
    LONG textHeight;
    LONG columns;
    LONG rows;
    HDWP deferHandle;

    if (!EtpCacheLatencyGetPageRect(Context, &clientRect))
        return;

    margin = PhScaleToDisplay(6, Context->WindowDpi);

    // The group box caption must not be covered by the chart, so the inset is measured
    // from the dialog font rather than assumed.
    textHeight = EtCacheLatencyGetTextHeight(Context->WindowHandle);
    captionHeight = textHeight + margin;

    columns = Context->GraphColumns;

    if (columns < 1 || columns > ET_CACHE_LATENCY_LEVELS)
        columns = 2;

    rows = (ET_CACHE_LATENCY_LEVELS + columns - 1) / columns;

    cellWidth = (clientRect.right - clientRect.left - margin * (columns - 1)) / columns;
    cellHeight = (clientRect.bottom - clientRect.top - margin * (rows - 1)) / rows;

    if (cellWidth <= 0 || cellHeight <= 0)
        return;

    if (cellHeight - captionHeight - margin <= 0)
        return;

    deferHandle = BeginDeferWindowPos(ET_CACHE_LATENCY_LEVELS * 2);

    if (!deferHandle)
        return;

    for (LONG level = 0; level < ET_CACHE_LATENCY_LEVELS; level++)
    {
        LONG x = clientRect.left + (level % columns) * (cellWidth + margin);
        LONG y = clientRect.top + (level / columns) * (cellHeight + margin);
        HDWP nextDefer;

        nextDefer = DeferWindowPos(
            deferHandle,
            Context->GroupBoxHandles[level],
            NULL,
            x,
            y,
            cellWidth,
            cellHeight,
            SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOCOPYBITS
            );

        if (!nextDefer)
            return;

        deferHandle = nextDefer;
        nextDefer = DeferWindowPos(
            deferHandle,
            Context->GraphHandles[level],
            NULL,
            x + margin,
            y + captionHeight,
            cellWidth - margin * 2,
            cellHeight - captionHeight - margin,
            SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOCOPYBITS
            );

        if (!nextDefer)
            return;

        deferHandle = nextDefer;
    }

    EndDeferWindowPos(deferHandle);
}

/**
 * Positions the table control inside the tab display area.
 *
 * \param Context Cache latency dialog context.
 */
static VOID EtpCacheLatencyLayoutTable(
    _Inout_ PET_CACHE_LATENCY_CONTEXT Context
    )
{
    RECT pageRect;

    if (!Context->ListViewHandle)
        return;

    if (!EtpCacheLatencyGetPageRect(Context, &pageRect))
        return;

    SetWindowPos(
        Context->ListViewHandle,
        NULL,
        pageRect.left,
        pageRect.top,
        pageRect.right - pageRect.left,
        pageRect.bottom - pageRect.top,
        SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOCOPYBITS
        );
}

/**
 * Positions the shared details panel labels.
 *
 * The panel sits below the tab control and stays visible on both tabs, so it is
 * laid out independently of the tab page.
 *
 * \param Context Cache latency dialog context.
 */
VOID EtCacheLatencyLayoutPanel(
    _Inout_ PET_CACHE_LATENCY_CONTEXT Context
    )
{
    RECT panelRect;
    LONG margin;
    LONG captionHeight;
    LONG labelWidth;
    LONG labelHeight;
    LONG labelTop;
    HDWP deferHandle;

    if (!Context->PanelHandle)
        return;

    GetWindowRect(Context->PanelHandle, &panelRect);
    PhScreenToClientRect(Context->WindowHandle, &panelRect);

    margin = PhScaleToDisplay(6, Context->WindowDpi);
    captionHeight = PhScaleToDisplay(14, Context->WindowDpi);
    labelHeight = PhScaleToDisplay(11, Context->WindowDpi);
    labelWidth = (panelRect.right - panelRect.left - margin * 2) / ET_CACHE_LATENCY_LEVELS;
    labelTop = panelRect.top + captionHeight;

    if (labelWidth <= 0)
        return;

    deferHandle = BeginDeferWindowPos(ET_CACHE_LATENCY_LEVELS * 2);

    if (!deferHandle)
        return;

    for (LONG level = 0; level < ET_CACHE_LATENCY_LEVELS; level++)
    {
        LONG x = panelRect.left + margin + level * labelWidth;
        HDWP nextDefer;

        nextDefer = DeferWindowPos(
            deferHandle,
            Context->InfoLabelHandles[level],
            NULL,
            x,
            labelTop,
            labelWidth - margin,
            labelHeight,
            SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOCOPYBITS
            );

        if (!nextDefer)
            return;

        deferHandle = nextDefer;
        nextDefer = DeferWindowPos(
            deferHandle,
            Context->ValueLabelHandles[level],
            NULL,
            x,
            labelTop + labelHeight,
            labelWidth - margin,
            labelHeight,
            SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOCOPYBITS
            );

        if (!nextDefer)
            return;

        deferHandle = nextDefer;
    }

    EndDeferWindowPos(deferHandle);
}

/**
 * Shows the selected tab page controls as one deferred operation.
 *
 * \param Context Cache latency dialog context.
 * \param Overview TRUE to show the overview controls; FALSE to show the table.
 */
static VOID EtpCacheLatencySetPageVisibility(
    _Inout_ PET_CACHE_LATENCY_CONTEXT Context,
    _In_ BOOLEAN Overview
    )
{
    HDWP deferHandle;
    BOOLEAN deferFailed = FALSE;

    deferHandle = BeginDeferWindowPos(ET_CACHE_LATENCY_LEVELS * 2 + 1);

    if (!deferHandle)
        deferFailed = TRUE;

    for (ULONG level = 0; level < ET_CACHE_LATENCY_LEVELS && deferHandle; level++)
    {
        deferHandle = DeferWindowPos(
            deferHandle,
            Context->GroupBoxHandles[level],
            NULL,
            0,
            0,
            0,
            0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER |
            (Overview ? SWP_SHOWWINDOW : SWP_HIDEWINDOW)
            );

        if (!deferHandle)
        {
            deferFailed = TRUE;
            break;
        }

        deferHandle = DeferWindowPos(
            deferHandle,
            Context->GraphHandles[level],
            NULL,
            0,
            0,
            0,
            0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER |
            (Overview ? SWP_SHOWWINDOW : SWP_HIDEWINDOW)
            );

        if (!deferHandle)
        {
            deferFailed = TRUE;
            break;
        }
    }

    if (deferHandle)
    {
        deferHandle = DeferWindowPos(
            deferHandle,
            Context->ListViewHandle,
            NULL,
            0,
            0,
            0,
            0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER |
            (Overview ? SWP_HIDEWINDOW : SWP_SHOWWINDOW)
            );

        if (!deferHandle || !EndDeferWindowPos(deferHandle))
            deferFailed = TRUE;
    }

    if (deferFailed)
    {
        for (ULONG level = 0; level < ET_CACHE_LATENCY_LEVELS; level++)
        {
            ShowWindow(Context->GroupBoxHandles[level], Overview ? SW_SHOW : SW_HIDE);
            ShowWindow(Context->GraphHandles[level], Overview ? SW_SHOW : SW_HIDE);
        }

        ShowWindow(Context->ListViewHandle, Overview ? SW_HIDE : SW_SHOW);
    }
}

/**
 * Shows the controls belonging to the selected tab and hides the others.
 *
 * \param Context Cache latency dialog context.
 * \param TabIndex Selected tab index.
 */
VOID EtCacheLatencySelectTab(
    _Inout_ PET_CACHE_LATENCY_CONTEXT Context,
    _In_ LONG TabIndex
    )
{
    BOOLEAN overview = TabIndex == ET_CACHE_LATENCY_TAB_OVERVIEW;
    RECT tabRect;

    Context->TabIndex = TabIndex;

    if (overview)
        EtCacheLatencyLayoutOverview(Context);
    else
        EtpCacheLatencyLayoutTable(Context);

    if (overview)
    {
        for (ULONG level = 0; level < ET_CACHE_LATENCY_LEVELS; level++)
        {
            SendMessage(Context->GroupBoxHandles[level], WM_SETREDRAW, FALSE, 0);
            SendMessage(Context->GraphHandles[level], WM_SETREDRAW, FALSE, 0);
        }
    }
    else
    {
        SendMessage(Context->ListViewHandle, WM_SETREDRAW, FALSE, 0);
    }

    SendMessage(Context->WindowHandle, WM_SETREDRAW, FALSE, 0);
    EtpCacheLatencySetPageVisibility(Context, overview);

    SendMessage(Context->WindowHandle, WM_SETREDRAW, TRUE, 0);

    if (overview)
    {
        for (ULONG level = 0; level < ET_CACHE_LATENCY_LEVELS; level++)
        {
            SendMessage(Context->GroupBoxHandles[level], WM_SETREDRAW, TRUE, 0);
            SendMessage(Context->GraphHandles[level], WM_SETREDRAW, TRUE, 0);
        }
    }
    else
    {
        SendMessage(Context->ListViewHandle, WM_SETREDRAW, TRUE, 0);
    }

    if (GetWindowRect(Context->TabControlHandle, &tabRect))
    {
        PhScreenToClientRect(Context->WindowHandle, &tabRect);
        RedrawWindow(
            Context->WindowHandle,
            &tabRect,
            NULL,
            RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW
            );
    }
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

    if (Result->Succeeded)
    {
        EtCacheLatencyUpdateBenchmarkRows(Context, Result);
        PhListView_SetItemCount(Context->ListViewContext, ET_CACHE_LATENCY_MSIZE_ROWS, LVSICF_NOINVALIDATEALL);
        EtCacheLatencyBuildOverview(Context, Result);
        EtCacheLatencySortRows(Context);
        InvalidateRect(Context->ListViewHandle, NULL, FALSE);
    }
    else
    {
        PhListView_SetItemCount(Context->ListViewContext, 0, LVSICF_NOINVALIDATEALL);
        EtCacheLatencyClearOverview(Context);
    }

    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);

    EtCacheLatencyRedrawGraphs(Context);

    Context->PassCount++;

    // The page size matters to the reader: without large pages the largest working sets
    // walk enough page table entries that translation cost lands in the tail rows and
    // inflates what looks like memory latency.
    PhSetWindowText(Context->StatusLabelHandle, PhaFormatString(
        L"Pass %lu \x2022 CPU %lu \x2022 %lu sizes \x00d7 %lu strides \x00d7 %lu trials \x2022 %s%s",
        Context->PassCount,
        Result->ProcessorNumber,
        (ULONG)ET_CACHE_LATENCY_MSIZE_ROWS,
        (ULONG)ET_CACHE_LATENCY_MAX_STRIDE_COLS,
        (ULONG)ET_CACHE_LATENCY_TRIALS,
        Result->LargePages ? L"large pages" : L"small pages",
        Result->Succeeded ? L"" : L" \x2014 benchmark failed"
        )->Buffer);
}

/**
 * Worker thread entry point for cache detection and benchmarking.
 *
 * \param Parameter Worker context.
 * \return Thread exit status.
 */
_Function_class_(USER_THREAD_START_ROUTINE)
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
            context->WindowDpi = PhGetWindowDpi(WindowHandle);
            context->ListViewHandle = GetDlgItem(WindowHandle, IDC_CACHE_LATENCY_LIST);
            context->TabControlHandle = GetDlgItem(WindowHandle, IDC_CACHE_LATENCY_TAB);
            context->StatusLabelHandle = GetDlgItem(WindowHandle, IDC_CACHE_LATENCY_STATUS);
            context->PanelHandle = GetDlgItem(WindowHandle, IDC_CACHE_LATENCY_PANEL);
            context->GroupBoxHandles[0] = GetDlgItem(WindowHandle, IDC_CACHE_LATENCY_L1_GROUP);
            context->GroupBoxHandles[1] = GetDlgItem(WindowHandle, IDC_CACHE_LATENCY_L2_GROUP);
            context->GroupBoxHandles[2] = GetDlgItem(WindowHandle, IDC_CACHE_LATENCY_L3_GROUP);
            context->GroupBoxHandles[3] = GetDlgItem(WindowHandle, IDC_CACHE_LATENCY_DRAM_GROUP);
            context->InfoLabelHandles[0] = GetDlgItem(WindowHandle, IDC_CACHE_LATENCY_L1_INFO);
            context->InfoLabelHandles[1] = GetDlgItem(WindowHandle, IDC_CACHE_LATENCY_L2_INFO);
            context->InfoLabelHandles[2] = GetDlgItem(WindowHandle, IDC_CACHE_LATENCY_L3_INFO);
            context->InfoLabelHandles[3] = GetDlgItem(WindowHandle, IDC_CACHE_LATENCY_DRAM_INFO);
            context->ValueLabelHandles[0] = GetDlgItem(WindowHandle, IDC_CACHE_LATENCY_L1_VALUE);
            context->ValueLabelHandles[1] = GetDlgItem(WindowHandle, IDC_CACHE_LATENCY_L2_VALUE);
            context->ValueLabelHandles[2] = GetDlgItem(WindowHandle, IDC_CACHE_LATENCY_L3_VALUE);
            context->ValueLabelHandles[3] = GetDlgItem(WindowHandle, IDC_CACHE_LATENCY_DRAM_VALUE);
            context->CurveMaximum = 1;

            PhSetApplicationWindowIcon(WindowHandle);
            PhAddTabControlTab(context->TabControlHandle, ET_CACHE_LATENCY_TAB_OVERVIEW, L"Overview");
            PhAddTabControlTab(context->TabControlHandle, ET_CACHE_LATENCY_TAB_TABLE, L"Table");
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

            EtCacheLatencyCreateGraphs(context);

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            PhAddLayoutItem(&context->LayoutManager, context->TabControlHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, context->PanelHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, context->StatusLabelHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            if (PhValidWindowPlacementFromSetting(SETTING_NAME_CACHE_LATENCY_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_NAME_CACHE_LATENCY_WINDOW_POSITION, SETTING_NAME_CACHE_LATENCY_WINDOW_SIZE, WindowHandle);
            else
                PhCenterWindow(WindowHandle, context->ParentWindowHandle);

            context->GraphColumns = (LONG)PhGetIntegerSetting(SETTING_NAME_CACHE_LATENCY_GRAPH_COLUMNS);

            if (context->GraphColumns < 1 || context->GraphColumns > ET_CACHE_LATENCY_LEVELS)
                context->GraphColumns = 2;

            context->TabIndex = (LONG)PhGetIntegerSetting(SETTING_NAME_CACHE_LATENCY_WINDOW_TAB);

            if (context->TabIndex != ET_CACHE_LATENCY_TAB_TABLE)
                context->TabIndex = ET_CACHE_LATENCY_TAB_OVERVIEW;

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT));

            // The double-buffered group box painter avoids caption artifacts while
            // the overview cells are moved during a resize.
            for (ULONG level = 0; level < ET_CACHE_LATENCY_LEVELS; level++)
                PhInitializeThemeWindowGroupBoxEx(context->GroupBoxHandles[level]);
            PhInitializeThemeWindowGroupBoxEx(context->PanelHandle);

            PhTabNew_SetThemeDark(context->TabControlHandle, !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT));
            SetWindowPos(
                context->TabControlHandle,
                HWND_BOTTOM,
                0,
                0,
                0,
                0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
                );

            TabCtrl_SetCurSel(context->TabControlHandle, context->TabIndex);
            EtCacheLatencySelectTab(context, context->TabIndex);
            EtCacheLatencyLayoutPanel(context);
            EtCacheLatencyRedrawGraphs(context);

            EtCacheLatencyStartRefresh(context);
        }
        break;
    case WM_DESTROY:
        {
            KillTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT);
            PhSaveListViewColumnsToSetting(SETTING_NAME_CACHE_LATENCY_LISTVIEW_COLUMNS, context->ListViewHandle);
            PhSaveWindowPlacementToSetting(SETTING_NAME_CACHE_LATENCY_WINDOW_POSITION, SETTING_NAME_CACHE_LATENCY_WINDOW_SIZE, WindowHandle);
            PhSetIntegerSetting(SETTING_NAME_CACHE_LATENCY_WINDOW_TAB, (ULONG)context->TabIndex);
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
            EtCacheLatencyLayoutOverview(context);
            EtpCacheLatencyLayoutTable(context);
            EtCacheLatencyLayoutPanel(context);

            // The dialog clips its children, so this erases only newly exposed parent
            // background without forcing every child through a separate erase pass.
            InvalidateRect(WindowHandle, NULL, TRUE);
        }
        break;
    case WM_DPICHANGED:
        {
            context->WindowDpi = LOWORD(wParam);
            PhLayoutManagerUpdate(&context->LayoutManager, context->WindowDpi);
            PhLayoutManagerLayout(&context->LayoutManager);
            EtCacheLatencyLayoutOverview(context);
            EtpCacheLatencyLayoutTable(context);
            EtCacheLatencyLayoutPanel(context);

            for (ULONG level = 0; level < ET_CACHE_LATENCY_LEVELS; level++)
                SendMessage(context->GraphHandles[level], ET_CGM_SETDPI, context->WindowDpi, 0);

            InvalidateRect(context->TabControlHandle, NULL, FALSE);
            InvalidateRect(WindowHandle, NULL, TRUE);
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
    case WM_THEMECHANGED:
        {
            PhTabNew_SetThemeDark(context->TabControlHandle, !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT));

            for (ULONG level = 0; level < ET_CACHE_LATENCY_LEVELS; level++)
                InvalidateRect(context->GraphHandles[level], NULL, FALSE);

            InvalidateRect(WindowHandle, NULL, TRUE);
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

            if (header->hwndFrom == context->TabControlHandle && header->code == PHTNN_SELCHANGED)
            {
                EtCacheLatencySelectTab(context, PhTabNew_GetCurSel(context->TabControlHandle));
                break;
            }

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
            else if (context->TabIndex == ET_CACHE_LATENCY_TAB_OVERVIEW)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                // The menu key reports no position, so fall back to the cursor.
                if (point.x == -1 && point.y == -1)
                    GetCursorPos(&point);

                menu = PhCreateEMenu();
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ET_CACHE_LATENCY_MENU_COLUMNS_1, L"&One graph per row", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ET_CACHE_LATENCY_MENU_COLUMNS_2, L"&Two graphs per row", NULL, NULL), ULONG_MAX);
                PhSetFlagsEMenuItem(
                    menu,
                    context->GraphColumns == 1 ? ET_CACHE_LATENCY_MENU_COLUMNS_1 : ET_CACHE_LATENCY_MENU_COLUMNS_2,
                    PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK,
                    PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK
                    );

                item = PhShowEMenu(
                    menu,
                    WindowHandle,
                    PH_EMENU_SHOW_LEFTRIGHT,
                    PH_ALIGN_LEFT | PH_ALIGN_TOP,
                    point.x,
                    point.y
                    );

                if (item)
                {
                    LONG columns = item->Id == ET_CACHE_LATENCY_MENU_COLUMNS_1 ? 1 : 2;

                    if (columns != context->GraphColumns)
                    {
                        context->GraphColumns = columns;
                        PhSetIntegerSetting(SETTING_NAME_CACHE_LATENCY_GRAPH_COLUMNS, (ULONG)columns);
                        EtCacheLatencyLayoutOverview(context);
                        InvalidateRect(WindowHandle, NULL, TRUE);
                    }
                }

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
