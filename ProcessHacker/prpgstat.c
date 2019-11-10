/*
 * Process Hacker -
 *   Process properties: Statistics page
 *
 * Copyright (C) 2009-2016 wj32
 * Copyright (C) 2017-2019 dmex
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <phapp.h>
#include <phsettings.h>
#include <phplug.h>
#include <procprp.h>
#include <procprpp.h>
#include <procprv.h>

#include <emenu.h>
#include <settings.h>

typedef enum _PH_PROCESS_STATISTICS_CATEGORY
{
    PH_PROCESS_STATISTICS_CATEGORY_CPU,
    PH_PROCESS_STATISTICS_CATEGORY_MEMORY,
    PH_PROCESS_STATISTICS_CATEGORY_IO,
    PH_PROCESS_STATISTICS_CATEGORY_OTHER,
    PH_PROCESS_STATISTICS_CATEGORY_EXTENSION
} PH_PROCESS_STATISTICS_CATEGORY;

typedef enum _PH_PROCESS_STATISTICS_INDEX
{
    PH_PROCESS_STATISTICS_INDEX_PRIORITY,
    PH_PROCESS_STATISTICS_INDEX_CYCLES,
    PH_PROCESS_STATISTICS_INDEX_CYCLESDELTA,
    PH_PROCESS_STATISTICS_INDEX_KERNELTIME,
    PH_PROCESS_STATISTICS_INDEX_KERNELDELTA,
    PH_PROCESS_STATISTICS_INDEX_USERTIME,
    PH_PROCESS_STATISTICS_INDEX_USERDELTA,
    PH_PROCESS_STATISTICS_INDEX_TOTALTIME,
    PH_PROCESS_STATISTICS_INDEX_TOTALDELTA,
    PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTES,
    PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTESDELTA,

    PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATEBYTES,
    PH_PROCESS_STATISTICS_INDEX_VIRTUALSIZE,
    PH_PROCESS_STATISTICS_INDEX_PEAKVIRTUALSIZE,
    PH_PROCESS_STATISTICS_INDEX_PAGEFAULTS,
    PH_PROCESS_STATISTICS_INDEX_PAGEFAULTSDELTA,
    PH_PROCESS_STATISTICS_INDEX_WORKINGSET,
    PH_PROCESS_STATISTICS_INDEX_PEAKWORKINGSET,
    PH_PROCESS_STATISTICS_INDEX_PRIVATEWS,
    PH_PROCESS_STATISTICS_INDEX_SHAREABLEWS,
    PH_PROCESS_STATISTICS_INDEX_SHAREDWS,
    PH_PROCESS_STATISTICS_INDEX_PAGEPRIORITY,

    PH_PROCESS_STATISTICS_INDEX_READS,
    PH_PROCESS_STATISTICS_INDEX_READSDELTA,
    PH_PROCESS_STATISTICS_INDEX_READBYTES,
    PH_PROCESS_STATISTICS_INDEX_READBYTESDELTA,
    PH_PROCESS_STATISTICS_INDEX_WRITES,
    PH_PROCESS_STATISTICS_INDEX_WRITESDELTA,
    PH_PROCESS_STATISTICS_INDEX_WRITEBYTES,
    PH_PROCESS_STATISTICS_INDEX_WRITEBYTESDELTA,
    PH_PROCESS_STATISTICS_INDEX_OTHER,
    PH_PROCESS_STATISTICS_INDEX_OTHERDELTA,
    PH_PROCESS_STATISTICS_INDEX_OTHERBYTES,
    PH_PROCESS_STATISTICS_INDEX_OTHERBYTESDELTA,
    PH_PROCESS_STATISTICS_INDEX_IOPRIORITY,

    PH_PROCESS_STATISTICS_INDEX_HANDLES,
    PH_PROCESS_STATISTICS_INDEX_PEAKHANDLES,
    PH_PROCESS_STATISTICS_INDEX_GDIHANDLES,
    PH_PROCESS_STATISTICS_INDEX_USERHANDLES,

    PH_PROCESS_STATISTICS_INDEX_PAGEDPOOL,
    PH_PROCESS_STATISTICS_INDEX_PEAKPAGEDPOOL,
    PH_PROCESS_STATISTICS_INDEX_NONPAGED,
    PH_PROCESS_STATISTICS_INDEX_PEAKNONPAGED,

    PH_PROCESS_STATISTICS_INDEX_RUNNINGTIME,
    PH_PROCESS_STATISTICS_INDEX_SUSPENDEDTIME,
    PH_PROCESS_STATISTICS_INDEX_HANGCOUNT,
    PH_PROCESS_STATISTICS_INDEX_GHOSTCOUNT,

    PH_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHES,
    PH_PROCESS_STATISTICS_INDEX_NETWORKTXRXBYTES
} PH_PROCESS_STATISTICS_INDEX;

VOID PhpUpdateStatisticsAddListViewGroups(
    _In_ PPH_STATISTICS_CONTEXT Context
    )
{
    ListView_EnableGroupView(Context->ListViewHandle, TRUE);

    PhAddListViewGroup(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, L"CPU");
    PhAddListViewGroup(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, L"Memory");
    PhAddListViewGroup(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, L"I/O");
    PhAddListViewGroup(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_OTHER, L"Other");
    PhAddListViewGroup(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_EXTENSION, L"Extension");

    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_PRIORITY, L"Priority", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_CYCLES, L"Cycles", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_CYCLESDELTA, L"Cycles delta", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_KERNELTIME, L"Kernel time", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_KERNELDELTA, L"Kernel delta", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_USERTIME, L"User time", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_USERDELTA, L"User delta", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_TOTALTIME, L"Total time", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_TOTALDELTA, L"Total delta", NULL);

    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTES, L"Private bytes", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTESDELTA, L"Private bytes delta", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATEBYTES, L"Peak private bytes", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_VIRTUALSIZE, L"Virtual size", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PEAKVIRTUALSIZE, L"Peak virtual size", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PAGEFAULTS, L"Page faults", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PAGEFAULTSDELTA, L"Page faults delta", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_WORKINGSET, L"Working set", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PEAKWORKINGSET, L"Peak working set", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PRIVATEWS, L"Private WS", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_SHAREABLEWS, L"Shareable WS", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_SHAREDWS, L"Shared WS", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PAGEDPOOL, L"Paged pool bytes", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PEAKPAGEDPOOL, L"Peak paged pool bytes", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_NONPAGED, L"Nonpaged pool bytes", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PEAKNONPAGED, L"Peak nonpaged pool bytes", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PAGEPRIORITY, L"Page priority", NULL);

    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_READS, L"Reads", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_READSDELTA, L"Reads delta", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_READBYTES, L"Read bytes", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_READBYTESDELTA, L"Read bytes delta", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_WRITES, L"Writes", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_WRITESDELTA, L"Writes delta", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_WRITEBYTES, L"Write bytes", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_WRITEBYTESDELTA, L"Write bytes delta", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_OTHER, L"Other", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_OTHERDELTA, L"Other delta", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_OTHERBYTES, L"Other bytes", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_OTHERBYTESDELTA, L"Other bytes delta", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_IOPRIORITY, L"I/O priority", NULL);

    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_HANDLES, L"Handles", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_PEAKHANDLES, L"Peak handles", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_GDIHANDLES, L"GDI handles", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_USERHANDLES, L"USER handles", NULL);

    if (WindowsVersion >= WINDOWS_10_RS3)
    {
        PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_RUNNINGTIME, L"Running time", NULL);
        PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_SUSPENDEDTIME, L"Suspended time", NULL);
        PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_HANGCOUNT, L"Hang count", NULL);
        PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_GHOSTCOUNT, L"Ghost count", NULL);
    }

    if (WindowsVersion >= WINDOWS_10_RS3 && !PhIsExecutingInWow64())
    {
        PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_EXTENSION, PH_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHES, L"ContextSwitches", NULL);
        //PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_EXTENSION, PH_PROCESS_STATISTICS_INDEX_DISKREAD, L"BytesRead", NULL);
        //PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_EXTENSION, PH_PROCESS_STATISTICS_INDEX_DISKWRITE, L"BytesWritten", NULL);
        PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_EXTENSION, PH_PROCESS_STATISTICS_INDEX_NETWORKTXRXBYTES, L"NetworkTxRxBytes", NULL);
        //PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_EXTENSION, PH_PROCESS_STATISTICS_INDEX_MBBTXRXBYTES, L"MBBTxRxBytes", NULL);
    }

    if (PhPluginsEnabled)
    {
        PH_PLUGIN_PROCESS_STATS_EVENT notifyEvent;

        notifyEvent.Version = 0;
        notifyEvent.Type = 1;
        notifyEvent.ProcessItem = Context->ProcessItem;
        notifyEvent.Parameter = Context->ListViewHandle;

        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackProcessStatsNotifyEvent), &notifyEvent);
    }
}

VOID PhpUpdateProcessStatisticDelta(
    _In_ PPH_STATISTICS_CONTEXT Context,
    _In_ INT Index,
    _In_ ULONG_PTR Delta
    )
{
    LONG_PTR delta = (LONG_PTR)Delta;

    if (delta != 0)
    {
        PH_FORMAT format[2];

        if (delta > 0)
        {
            PhInitFormatC(&format[0], '+');
        }
        else
        {
            PhInitFormatC(&format[0], '-');
            delta = -delta;
        }

        format[1].Type = SizeFormatType | FormatUseRadix;
        format[1].Radix = (UCHAR)PhMaxSizeUnit;
        format[1].u.Size = delta;

        PhSetListViewSubItem(Context->ListViewHandle, Index, 1, PH_AUTO_T(PH_STRING, PhFormat(format, 2, 0))->Buffer);
    }
    else
    {
        PhSetListViewSubItem(Context->ListViewHandle, Index, 1, L"0");
    }
}

VOID PhpUpdateProcessStatisticDeltaBytes(
    _In_ PPH_STATISTICS_CONTEXT Context,
    _In_ INT Index,
    _In_ PH_UINT64_DELTA DeltaBuffer
    )
{
    ULONG64 number = 0;

    if (DeltaBuffer.Delta != DeltaBuffer.Value)
    {
        number = DeltaBuffer.Delta;
        number *= 1000;
        number /= PhCsUpdateInterval;
    }

    if (number != 0)
    {
        PH_FORMAT format[2];

        PhInitFormatSize(&format[0], number);
        PhInitFormatS(&format[1], L"/s");
        PhSetListViewSubItem(Context->ListViewHandle, Index, 1, PH_AUTO_T(PH_STRING, PhFormat(format, 2, 0))->Buffer);
    }
    else
    {
        PhSetListViewSubItem(Context->ListViewHandle, Index, 1, L"0");
    }
}

VOID PhpUpdateProcessStatistics(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_STATISTICS_CONTEXT Context
    )
{
    WCHAR priority[PH_INT32_STR_LEN_1] = L"";
    WCHAR timeSpan[PH_TIMESPAN_STR_LEN_1] = L"";

    PhPrintInt32(priority, ProcessItem->BasePriority);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_PRIORITY, 1, priority);
    PhPrintTimeSpan(timeSpan, ProcessItem->KernelTime.QuadPart, PH_TIMESPAN_HMSM);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_KERNELTIME, 1, timeSpan);
    PhPrintTimeSpan(timeSpan, ProcessItem->UserTime.QuadPart, PH_TIMESPAN_HMSM);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_USERTIME, 1, timeSpan);
    PhPrintTimeSpan(timeSpan, ProcessItem->KernelTime.QuadPart + ProcessItem->UserTime.QuadPart, PH_TIMESPAN_HMSM);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_TOTALTIME, 1, timeSpan);

    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_CYCLESDELTA, 1, PhaFormatUInt64(ProcessItem->CycleTimeDelta.Delta, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_KERNELDELTA, 1, PhaFormatUInt64(ProcessItem->CpuKernelDelta.Delta, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_USERDELTA, 1, PhaFormatUInt64(ProcessItem->CpuUserDelta.Delta, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_TOTALDELTA, 1, PhaFormatUInt64(ProcessItem->CpuKernelDelta.Delta + ProcessItem->CpuUserDelta.Delta, TRUE)->Buffer);

    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTES, 1, PhaFormatSize(ProcessItem->VmCounters.PagefileUsage, ULONG_MAX)->Buffer);
    PhpUpdateProcessStatisticDelta(Context, PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTESDELTA, ProcessItem->PrivateBytesDelta.Delta);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATEBYTES, 1, PhaFormatSize(ProcessItem->VmCounters.PeakPagefileUsage, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_VIRTUALSIZE, 1, PhaFormatSize(ProcessItem->VmCounters.VirtualSize, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_PEAKVIRTUALSIZE, 1, PhaFormatSize(ProcessItem->VmCounters.PeakVirtualSize, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_PAGEFAULTS, 1, PhaFormatUInt64(ProcessItem->VmCounters.PageFaultCount, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_PAGEFAULTSDELTA, 1, PhaFormatUInt64(ProcessItem->PageFaultsDelta.Delta, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_WORKINGSET, 1, PhaFormatSize(ProcessItem->VmCounters.WorkingSetSize, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_PEAKWORKINGSET, 1, PhaFormatSize(ProcessItem->VmCounters.PeakWorkingSetSize, ULONG_MAX)->Buffer);

    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_PAGEDPOOL, 1, PhaFormatSize(ProcessItem->VmCounters.QuotaPagedPoolUsage, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_PEAKPAGEDPOOL, 1, PhaFormatSize(ProcessItem->VmCounters.QuotaPeakPagedPoolUsage, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_NONPAGED, 1, PhaFormatSize(ProcessItem->VmCounters.QuotaNonPagedPoolUsage, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_PEAKNONPAGED, 1, PhaFormatSize(ProcessItem->VmCounters.QuotaPeakNonPagedPoolUsage, ULONG_MAX)->Buffer);

    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_READS, 1, PhaFormatUInt64(ProcessItem->IoCounters.ReadOperationCount, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_READSDELTA, 1, PhaFormatUInt64(ProcessItem->IoReadCountDelta.Delta, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_READBYTES, 1, PhaFormatSize(ProcessItem->IoCounters.ReadTransferCount, ULONG_MAX)->Buffer);
    PhpUpdateProcessStatisticDeltaBytes(Context, PH_PROCESS_STATISTICS_INDEX_READBYTESDELTA, ProcessItem->IoReadDelta);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_WRITES, 1, PhaFormatUInt64(ProcessItem->IoCounters.WriteOperationCount, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_WRITESDELTA, 1, PhaFormatUInt64(ProcessItem->IoWriteCountDelta.Delta, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_WRITEBYTES, 1, PhaFormatSize(ProcessItem->IoCounters.WriteTransferCount, ULONG_MAX)->Buffer);
    PhpUpdateProcessStatisticDeltaBytes(Context, PH_PROCESS_STATISTICS_INDEX_WRITEBYTESDELTA, ProcessItem->IoWriteDelta);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_OTHER, 1, PhaFormatUInt64(ProcessItem->IoCounters.OtherOperationCount, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_OTHERDELTA, 1, PhaFormatUInt64(ProcessItem->IoOtherCountDelta.Delta, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_OTHERBYTES, 1, PhaFormatSize(ProcessItem->IoCounters.OtherTransferCount, ULONG_MAX)->Buffer);
    PhpUpdateProcessStatisticDeltaBytes(Context, PH_PROCESS_STATISTICS_INDEX_OTHERBYTESDELTA, ProcessItem->IoOtherDelta);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_HANDLES, 1, PhaFormatUInt64(ProcessItem->NumberOfHandles, TRUE)->Buffer);

    // Optional information
    if (!PH_IS_FAKE_PROCESS_ID(ProcessItem->ProcessId))
    {
        PPH_STRING peakHandles = NULL;
        PPH_STRING gdiHandles = NULL;
        PPH_STRING userHandles = NULL;
        PPH_STRING cycles = NULL;
        ULONG pagePriority = ULONG_MAX;
        IO_PRIORITY_HINT ioPriority = ULONG_MAX;
        PPH_STRING privateWs = NULL;
        PPH_STRING shareableWs = NULL;
        PPH_STRING sharedWs = NULL;
        BOOLEAN gotCycles = FALSE;
        BOOLEAN gotWsCounters = FALSE;
        BOOLEAN gotUptime = FALSE;
        ULONG hangCount = 0;
        ULONG ghostCount = 0;
        ULONGLONG runningTime = 0;
        ULONGLONG suspendedTime = 0;
        WCHAR timeSpan[PH_TIMESPAN_STR_LEN_1] = L"";

        if (ProcessItem->QueryHandle)
        {
            ULONG64 cycleTime;
            PROCESS_HANDLE_INFORMATION handleInfo;
            PROCESS_UPTIME_INFORMATION uptimeInfo;

            if (NT_SUCCESS(PhGetProcessHandleCount(ProcessItem->QueryHandle, &handleInfo)))
            {
                peakHandles = PhaFormatUInt64(handleInfo.HandleCountHighWatermark, TRUE);
            }

            gdiHandles = PhaFormatUInt64(GetGuiResources(ProcessItem->QueryHandle, GR_GDIOBJECTS), TRUE); // GDI handles
            userHandles = PhaFormatUInt64(GetGuiResources(ProcessItem->QueryHandle, GR_USEROBJECTS), TRUE); // USER handles

            if (NT_SUCCESS(PhGetProcessCycleTime(ProcessItem->QueryHandle, &cycleTime)))
            {
                cycles = PhaFormatUInt64(cycleTime, TRUE);
                gotCycles = TRUE;
            }

            PhGetProcessPagePriority(ProcessItem->QueryHandle, &pagePriority);
            PhGetProcessIoPriority(ProcessItem->QueryHandle, &ioPriority);

            if (WindowsVersion >= WINDOWS_10_RS3 && NT_SUCCESS(PhGetProcessUptime(ProcessItem->QueryHandle, &uptimeInfo)))
            {
                runningTime = uptimeInfo.Uptime;
                suspendedTime = uptimeInfo.SuspendedTime;
                hangCount = uptimeInfo.HangCount;
                ghostCount = uptimeInfo.GhostCount;
                gotUptime = TRUE;
            }
        }

        if (Context->ProcessHandle)
        {
            PH_PROCESS_WS_COUNTERS wsCounters;

            if (NT_SUCCESS(PhGetProcessWsCounters(Context->ProcessHandle, &wsCounters)))
            {
                privateWs = PhaFormatSize((ULONG64)wsCounters.NumberOfPrivatePages * PAGE_SIZE, ULONG_MAX);
                shareableWs = PhaFormatSize((ULONG64)wsCounters.NumberOfShareablePages * PAGE_SIZE, ULONG_MAX);
                sharedWs = PhaFormatSize((ULONG64)wsCounters.NumberOfSharedPages * PAGE_SIZE, ULONG_MAX);
                gotWsCounters = TRUE;
            }
        }

        if (!gotCycles)
            cycles = PhaFormatUInt64(ProcessItem->CycleTimeDelta.Value, TRUE);
        if (!gotWsCounters)
            privateWs = PhaFormatSize(ProcessItem->WorkingSetPrivateSize, ULONG_MAX);

        PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_CYCLES, 1, PhGetStringOrDefault(cycles, L"N/A"));

        if (pagePriority != ULONG_MAX && pagePriority <= MEMORY_PRIORITY_NORMAL)
            PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_PAGEPRIORITY, 1, PhPagePriorityNames[pagePriority]);
        else
            PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_PAGEPRIORITY, 1, L"N/A");

        if (ioPriority != ULONG_MAX && ioPriority < MaxIoPriorityTypes)
            PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_IOPRIORITY, 1, PhIoPriorityHintNames[ioPriority]);
        else
            PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_IOPRIORITY, 1, L"N/A");

        PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_PRIVATEWS, 1, PhGetStringOrDefault(privateWs, L"N/A"));
        PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_SHAREABLEWS, 1, PhGetStringOrDefault(shareableWs, L"N/A"));
        PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_SHAREDWS, 1, PhGetStringOrDefault(sharedWs, L"N/A"));

        PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_PEAKHANDLES, 1, PhGetStringOrDefault(peakHandles, L"N/A"));
        PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_GDIHANDLES, 1, PhGetStringOrDefault(gdiHandles, L"N/A"));
        PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_USERHANDLES, 1, PhGetStringOrDefault(userHandles, L"N/A"));

        if (WindowsVersion >= WINDOWS_10_RS3)
        {
            PhPrintTimeSpan(timeSpan, runningTime, PH_TIMESPAN_HMSM);
            PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_RUNNINGTIME, 1, timeSpan);
            PhPrintTimeSpan(timeSpan, suspendedTime, PH_TIMESPAN_HMSM);
            PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_SUSPENDEDTIME, 1, timeSpan);
            PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_HANGCOUNT, 1, PhaFormatUInt64(hangCount, TRUE)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_GHOSTCOUNT, 1, PhaFormatUInt64(ghostCount, TRUE)->Buffer);
        }
    }

    if (WindowsVersion >= WINDOWS_10_RS3 && !PhIsExecutingInWow64())
    {
        PVOID processes;
        PSYSTEM_PROCESS_INFORMATION processInfo;
        PSYSTEM_PROCESS_INFORMATION_EXTENSION processExtension;

        if (NT_SUCCESS(PhEnumProcesses(&processes)))
        {
            processInfo = PhFindProcessInformation(processes, ProcessItem->ProcessId);

            if (processInfo && (processExtension = PH_PROCESS_EXTENSION(processInfo)))
            {
                PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHES, 1, PhaFormatUInt64(processExtension->ContextSwitches, TRUE)->Buffer); // TODO: ContextSwitchesDelta
                //PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_DISKREAD, 1, PhaFormatSize(processExtension->DiskCounters.BytesRead, ULONG_MAX)->Buffer);
                //PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_DISKWRITE, 1, PhaFormatSize(processExtension->DiskCounters.BytesWritten, ULONG_MAX)->Buffer);
                PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_NETWORKTXRXBYTES, 1, PhaFormatSize(processExtension->EnergyValues.NetworkTxRxBytes, ULONG_MAX)->Buffer);
                //PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_MBBTXRXBYTES, 1, PhaFormatSize(processExtension->EnergyValues.MBBTxRxBytes, ULONG_MAX)->Buffer);
            }

            PhFree(processes);
        }
    }

    if (PhPluginsEnabled)
    {
        PH_PLUGIN_PROCESS_STATS_EVENT notifyEvent;

        notifyEvent.Version = 0;
        notifyEvent.Type = 2;
        notifyEvent.ProcessItem = Context->ProcessItem;
        notifyEvent.Parameter = Context->ListViewHandle;

        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackProcessStatsNotifyEvent), &notifyEvent);
    }
}

static VOID NTAPI PhpStatisticsUpdateHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_STATISTICS_CONTEXT statisticsContext = (PPH_STATISTICS_CONTEXT)Context;

    if (statisticsContext && statisticsContext->Enabled)
        PostMessage(statisticsContext->WindowHandle, WM_PH_STATISTICS_UPDATE, 0, 0);
}

INT_PTR CALLBACK PhpProcessStatisticsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_STATISTICS_CONTEXT statisticsContext;

    if (PhPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext, &processItem))
    {
        statisticsContext = (PPH_STATISTICS_CONTEXT)propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            statisticsContext = propPageContext->Context = PhAllocateZero(sizeof(PH_STATISTICS_CONTEXT));
            statisticsContext->WindowHandle = hwndDlg;
            statisticsContext->ListViewHandle = GetDlgItem(hwndDlg, IDC_STATISTICS_LIST);
            statisticsContext->ProcessItem = processItem;
            statisticsContext->Enabled = TRUE;

            // Try to open a process handle with PROCESS_QUERY_INFORMATION access for WS information.
            if (PH_IS_REAL_PROCESS_ID(processItem->ProcessId))
            {
                PhOpenProcess(
                    &statisticsContext->ProcessHandle,
                    PROCESS_QUERY_INFORMATION,
                    processItem->ProcessId
                    );
            }

            PhSetListViewStyle(statisticsContext->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(statisticsContext->ListViewHandle, L"explorer");
            PhAddListViewColumn(statisticsContext->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 135, L"Property");
            PhAddListViewColumn(statisticsContext->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 150, L"Value");
            PhSetExtendedListView(statisticsContext->ListViewHandle);

            PhpUpdateStatisticsAddListViewGroups(statisticsContext);
            PhLoadListViewGroupStatesFromSetting(L"ProcStatPropPageGroupStates", statisticsContext->ListViewHandle);

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                PhpStatisticsUpdateHandler,
                statisticsContext,
                &statisticsContext->ProcessesUpdatedRegistration
                );

            PhpUpdateProcessStatistics(processItem, statisticsContext);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewGroupStatesToSetting(L"ProcStatPropPageGroupStates", statisticsContext->ListViewHandle);

            PhUnregisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                &statisticsContext->ProcessesUpdatedRegistration
                );

            if (statisticsContext->ProcessHandle)
                NtClose(statisticsContext->ProcessHandle);

            PhFree(statisticsContext);
        }
        break;
    case WM_SHOWWINDOW:
        {
            PPH_LAYOUT_ITEM dialogItem;

            if (dialogItem = PhBeginPropPageLayout(hwndDlg, propPageContext))
            {
                PhAddPropPageLayoutItem(hwndDlg, statisticsContext->ListViewHandle, dialogItem, PH_ANCHOR_ALL);
                PhEndPropPageLayout(hwndDlg, propPageContext);
            }

            ExtendedListView_SetColumnWidth(statisticsContext->ListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            PhHandleListViewNotifyBehaviors(lParam, statisticsContext->ListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);

            switch (header->code)
            {
            case PSN_SETACTIVE:
                statisticsContext->Enabled = TRUE;
                break;
            case PSN_KILLACTIVE:
                statisticsContext->Enabled = FALSE;
                break;
            }
        }
        break;
    case WM_PH_STATISTICS_UPDATE:
        {
            PhpUpdateProcessStatistics(processItem, statisticsContext);
        }
        break;
    case WM_SIZE:
        {
            ExtendedListView_SetColumnWidth(statisticsContext->ListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == statisticsContext->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID *listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint((HWND)wParam, &point);

                PhGetSelectedListViewItemParams(statisticsContext->ListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();

                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, IDC_COPY, statisticsContext->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        BOOLEAN handled = FALSE;

                        handled = PhHandleCopyListViewEMenuItem(item);

                        //if (!handled && PhPluginsEnabled)
                        //    handled = PhPluginTriggerEMenuItem(&menuInfo, item);

                        if (!handled)
                        {
                            switch (item->Id)
                            {
                            case IDC_COPY:
                                {
                                    PhCopyListView(statisticsContext->ListViewHandle);
                                }
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }

                PhFree(listviewItems);
            }
        }
        break;
    }

    return FALSE;
}
