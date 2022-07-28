/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2021
 *
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
    PH_PROCESS_STATISTICS_INDEX_HARDFAULTS,
    PH_PROCESS_STATISTICS_INDEX_HARDFAULTSDELTA,
    PH_PROCESS_STATISTICS_INDEX_WORKINGSET,
    PH_PROCESS_STATISTICS_INDEX_PEAKWORKINGSET,
    PH_PROCESS_STATISTICS_INDEX_PRIVATEWS,
    PH_PROCESS_STATISTICS_INDEX_SHAREABLEWS,
    PH_PROCESS_STATISTICS_INDEX_SHAREDWS,
    PH_PROCESS_STATISTICS_INDEX_SHAREDCOMMIT,
    PH_PROCESS_STATISTICS_INDEX_PRIVATECOMMIT,
    PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATECOMMIT,
    //PH_PROCESS_STATISTICS_INDEX_PRIVATECOMMITLIMIT,
    //PH_PROCESS_STATISTICS_INDEX_TOTALCOMMITLIMIT,
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
    PH_PROCESS_STATISTICS_INDEX_NETWORKTXRXBYTES,
    PH_PROCESS_STATISTICS_INDEX_MAX,
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

    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_PRIORITY, L"Priority", (PVOID)PH_PROCESS_STATISTICS_INDEX_PRIORITY);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_CYCLES, L"Cycles", (PVOID)PH_PROCESS_STATISTICS_INDEX_CYCLES);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_CYCLESDELTA, L"Cycles delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_CYCLESDELTA);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_KERNELTIME, L"Kernel time", (PVOID)PH_PROCESS_STATISTICS_INDEX_KERNELTIME);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_KERNELDELTA, L"Kernel delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_KERNELDELTA);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_USERTIME, L"User time", (PVOID)PH_PROCESS_STATISTICS_INDEX_USERTIME);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_USERDELTA, L"User delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_USERDELTA);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_TOTALTIME, L"Total time", (PVOID)PH_PROCESS_STATISTICS_INDEX_TOTALTIME);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_TOTALDELTA, L"Total delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_TOTALDELTA);

    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTES, L"Private bytes", (PVOID)PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTES);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTESDELTA, L"Private bytes delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTESDELTA);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATEBYTES, L"Peak private bytes", (PVOID)PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATEBYTES);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_VIRTUALSIZE, L"Virtual size", (PVOID)PH_PROCESS_STATISTICS_INDEX_VIRTUALSIZE);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PEAKVIRTUALSIZE, L"Peak virtual size", (PVOID)PH_PROCESS_STATISTICS_INDEX_PEAKVIRTUALSIZE);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PAGEFAULTS, L"Page faults", (PVOID)PH_PROCESS_STATISTICS_INDEX_PAGEFAULTS);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PAGEFAULTSDELTA, L"Page faults delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_PAGEFAULTSDELTA);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_HARDFAULTS, L"Hard faults", (PVOID)PH_PROCESS_STATISTICS_INDEX_HARDFAULTS);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_HARDFAULTSDELTA, L"Hard faults delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_HARDFAULTSDELTA);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_WORKINGSET, L"Working set", (PVOID)PH_PROCESS_STATISTICS_INDEX_WORKINGSET);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PEAKWORKINGSET, L"Peak working set", (PVOID)PH_PROCESS_STATISTICS_INDEX_PEAKWORKINGSET);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PRIVATEWS, L"Private WS", (PVOID)PH_PROCESS_STATISTICS_INDEX_PRIVATEWS);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_SHAREABLEWS, L"Shareable WS", (PVOID)PH_PROCESS_STATISTICS_INDEX_SHAREABLEWS);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_SHAREDWS, L"Shared WS", (PVOID)PH_PROCESS_STATISTICS_INDEX_SHAREDWS);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PAGEDPOOL, L"Paged pool bytes", (PVOID)PH_PROCESS_STATISTICS_INDEX_PAGEDPOOL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PEAKPAGEDPOOL, L"Peak paged pool bytes", (PVOID)PH_PROCESS_STATISTICS_INDEX_PEAKPAGEDPOOL);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_NONPAGED, L"Nonpaged pool bytes", (PVOID)PH_PROCESS_STATISTICS_INDEX_NONPAGED);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PEAKNONPAGED, L"Peak nonpaged pool bytes", (PVOID)PH_PROCESS_STATISTICS_INDEX_PEAKNONPAGED);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_SHAREDCOMMIT, L"Shared commit", (PVOID)PH_PROCESS_STATISTICS_INDEX_SHAREDCOMMIT);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PRIVATECOMMIT, L"Private commit", (PVOID)PH_PROCESS_STATISTICS_INDEX_PRIVATECOMMIT);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATECOMMIT, L"Peak private commit", (PVOID)PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATECOMMIT);
    //PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PRIVATECOMMITLIMIT, L"Private commit limit", (PVOID)PH_PROCESS_STATISTICS_INDEX_PRIVATECOMMITLIMIT);
    //PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_TOTALCOMMITLIMIT, L"Total commit limit", (PVOID)PH_PROCESS_STATISTICS_INDEX_TOTALCOMMITLIMIT);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PAGEPRIORITY, L"Page priority", (PVOID)PH_PROCESS_STATISTICS_INDEX_PAGEPRIORITY);

    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_READS, L"Reads", (PVOID)PH_PROCESS_STATISTICS_INDEX_READS);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_READSDELTA, L"Reads delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_READSDELTA);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_READBYTES, L"Read bytes", (PVOID)PH_PROCESS_STATISTICS_INDEX_READBYTES);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_READBYTESDELTA, L"Read bytes delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_READBYTESDELTA);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_WRITES, L"Writes", (PVOID)PH_PROCESS_STATISTICS_INDEX_WRITES);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_WRITESDELTA, L"Writes delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_WRITESDELTA);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_WRITEBYTES, L"Write bytes", (PVOID)PH_PROCESS_STATISTICS_INDEX_WRITEBYTES);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_WRITEBYTESDELTA, L"Write bytes delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_WRITEBYTESDELTA);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_OTHER, L"Other", (PVOID)PH_PROCESS_STATISTICS_INDEX_OTHER);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_OTHERDELTA, L"Other delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_OTHERDELTA);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_OTHERBYTES, L"Other bytes", (PVOID)PH_PROCESS_STATISTICS_INDEX_OTHERBYTES);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_OTHERBYTESDELTA, L"Other bytes delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_OTHERBYTESDELTA);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_IOPRIORITY, L"I/O priority", (PVOID)PH_PROCESS_STATISTICS_INDEX_IOPRIORITY);

    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_HANDLES, L"Handles", (PVOID)PH_PROCESS_STATISTICS_INDEX_HANDLES);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_PEAKHANDLES, L"Peak handles", (PVOID)PH_PROCESS_STATISTICS_INDEX_PEAKHANDLES);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_GDIHANDLES, L"GDI handles", (PVOID)PH_PROCESS_STATISTICS_INDEX_GDIHANDLES);
    PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_USERHANDLES, L"USER handles", (PVOID)PH_PROCESS_STATISTICS_INDEX_USERHANDLES);

    if (WindowsVersion >= WINDOWS_10_RS3)
    {
        PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_RUNNINGTIME, L"Running time", (PVOID)PH_PROCESS_STATISTICS_INDEX_RUNNINGTIME);
        PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_SUSPENDEDTIME, L"Suspended time", (PVOID)PH_PROCESS_STATISTICS_INDEX_SUSPENDEDTIME);
        PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_HANGCOUNT, L"Hang count", (PVOID)PH_PROCESS_STATISTICS_INDEX_HANGCOUNT);
        PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_GHOSTCOUNT, L"Ghost count", (PVOID)PH_PROCESS_STATISTICS_INDEX_GHOSTCOUNT);
    }

    if (WindowsVersion >= WINDOWS_10_RS3 && !PhIsExecutingInWow64())
    {
        PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_EXTENSION, PH_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHES, L"ContextSwitches", (PVOID)PH_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHES);
        //PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_EXTENSION, PH_PROCESS_STATISTICS_INDEX_DISKREAD, L"BytesRead", NULL);
        //PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_EXTENSION, PH_PROCESS_STATISTICS_INDEX_DISKWRITE, L"BytesWritten", NULL);
        PhAddListViewGroupItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_EXTENSION, PH_PROCESS_STATISTICS_INDEX_NETWORKTXRXBYTES, L"NetworkTxRxBytes", (PVOID)PH_PROCESS_STATISTICS_INDEX_NETWORKTXRXBYTES);
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
    _In_ NMLVDISPINFO* Entry,
    _In_ ULONG_PTR Delta
    )
{
    LONG_PTR delta = (LONG_PTR)Delta;

    if (delta != 0)
    {
        PPH_STRING value;
        PH_FORMAT format[2];

        if (delta > 0)
        {
            PhInitFormatC(&format[0], L'+');
        }
        else
        {
            PhInitFormatC(&format[0], L'-');
            delta = -delta;
        }

        format[1].Type = SizeFormatType | FormatUseRadix;
        format[1].Radix = (UCHAR)PhMaxSizeUnit;
        format[1].u.Size = delta;

        value = PhFormat(format, 2, 0);
        wcsncpy_s(Entry->item.pszText, Entry->item.cchTextMax, value->Buffer, _TRUNCATE);
        PhDereferenceObject(value);
    }
    else
    {
        wcsncpy_s(Entry->item.pszText, Entry->item.cchTextMax, L"0", _TRUNCATE);
    }
}

VOID PhpUpdateProcessStatisticDeltaBytes(
    _In_ PPH_STATISTICS_CONTEXT Context,
    _In_ NMLVDISPINFO* Entry,
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
        PPH_STRING value;
        PH_FORMAT format[2];

        PhInitFormatSize(&format[0], number);
        PhInitFormatS(&format[1], L"/s");

        value = PhFormat(format, 2, 0);
        wcsncpy_s(Entry->item.pszText, Entry->item.cchTextMax, value->Buffer, _TRUNCATE);
        PhDereferenceObject(value);
    }
    else
    {
        wcsncpy_s(Entry->item.pszText, Entry->item.cchTextMax, L"0", _TRUNCATE);
    }
}

VOID PhpUpdateProcessStatistics(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_STATISTICS_CONTEXT Context
    )
{
    if (!PH_IS_FAKE_PROCESS_ID(ProcessItem->ProcessId))
    {
        if (ProcessItem->QueryHandle)
        {
            ULONG64 cycleTime;
            PROCESS_HANDLE_INFORMATION handleInfo;
            PROCESS_UPTIME_INFORMATION uptimeInfo;

            if (NT_SUCCESS(PhGetProcessHandleCount(ProcessItem->QueryHandle, &handleInfo)))
            {
                // The HighWatermark changed on Windows 10 and doesn't track the peak handle count,
                // instead the value is now the maximum count of the handle freelist which decreases
                // when deallocating. So we'll track and cache the highest value where possible. (dmex)
                if (handleInfo.HandleCountHighWatermark > Context->PeakHandleCount)
                    Context->PeakHandleCount = handleInfo.HandleCountHighWatermark;

                PhMoveReference(&Context->PeakHandles, PhFormatUInt64(Context->PeakHandleCount, TRUE));
            }

            PhMoveReference(&Context->GdiHandles, PhFormatUInt64(GetGuiResources(ProcessItem->QueryHandle, GR_GDIOBJECTS), TRUE)); // GDI handles
            PhMoveReference(&Context->UserHandles, PhFormatUInt64(GetGuiResources(ProcessItem->QueryHandle, GR_USEROBJECTS), TRUE)); // USER handles

            if (NT_SUCCESS(PhGetProcessCycleTime(ProcessItem->QueryHandle, &cycleTime)))
            {
                PhMoveReference(&Context->Cycles, PhFormatUInt64(cycleTime, TRUE));
                Context->GotCycles = TRUE;
            }

            PhGetProcessPagePriority(ProcessItem->QueryHandle, &Context->PagePriority);
            PhGetProcessIoPriority(ProcessItem->QueryHandle, &Context->IoPriority);

            if (WindowsVersion >= WINDOWS_10_RS3 && NT_SUCCESS(PhGetProcessUptime(ProcessItem->QueryHandle, &uptimeInfo)))
            {
                Context->RunningTime = uptimeInfo.Uptime;
                Context->SuspendedTime = uptimeInfo.SuspendedTime;
                Context->HangCount = uptimeInfo.HangCount;
                Context->GhostCount = uptimeInfo.GhostCount;
                Context->GotUptime = TRUE;
            }
        }

        if (Context->ProcessHandle)
        {
            PH_PROCESS_WS_COUNTERS wsCounters;
            PH_APP_MEMORY_INFORMATION appMemoryInfo;

            if (NT_SUCCESS(PhGetProcessWsCounters(Context->ProcessHandle, &wsCounters)))
            {
                PhMoveReference(&Context->PrivateWs, PhFormatSize((ULONG64)wsCounters.NumberOfPrivatePages * PAGE_SIZE, ULONG_MAX));
                PhMoveReference(&Context->ShareableWs, PhFormatSize((ULONG64)wsCounters.NumberOfShareablePages * PAGE_SIZE, ULONG_MAX));
                PhMoveReference(&Context->SharedWs, PhFormatSize((ULONG64)wsCounters.NumberOfSharedPages * PAGE_SIZE, ULONG_MAX));
                Context->GotWsCounters = TRUE;
            }
            if (NT_SUCCESS(PhGetProcessAppMemoryInformation(Context->ProcessHandle, &appMemoryInfo)))
            {
                PhMoveReference(&Context->SharedCommitUsage, PhFormatSize(appMemoryInfo.TotalCommitUsage - appMemoryInfo.PrivateCommitUsage, ULONG_MAX));
                PhMoveReference(&Context->PrivateCommitUsage, PhFormatSize(appMemoryInfo.PrivateCommitUsage, ULONG_MAX));
                PhMoveReference(&Context->PeakPrivateCommitUsage, PhFormatSize(appMemoryInfo.PeakPrivateCommitUsage, ULONG_MAX));
                //if (appMemoryInfo.PrivateCommitLimit)
                //    PhMoveReference(&Context->PrivateCommitLimit, PhFormatSize(appMemoryInfo.PrivateCommitLimit, ULONG_MAX));
                //if (appMemoryInfo.TotalCommitLimit)
                //    PhMoveReference(&Context->TotalCommitLimit, PhFormatSize(appMemoryInfo.TotalCommitLimit, ULONG_MAX));
            }
        }

        if (!Context->GotCycles)
        {
            PhMoveReference(&Context->Cycles, PhFormatUInt64(ProcessItem->CycleTimeDelta.Value, TRUE));
        }

        if (!Context->GotWsCounters)
        {
            PhMoveReference(&Context->PrivateWs, PhFormatSize(ProcessItem->WorkingSetPrivateSize, ULONG_MAX));
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
                PVOID extension = Context->ProcessExtension;

                Context->ProcessExtension = PhAllocateCopy(processExtension, sizeof(SYSTEM_PROCESS_INFORMATION_EXTENSION));

                if (extension)
                {
                    PhFree(extension);
                }
            }

            PhFree(processes);
        }
    }

    // The ListView doesn't send LVN_GETDISPINFO (or redraw properly) when not focused so force the redraw. (dmex)
    ListView_RedrawItems(Context->ListViewHandle, 0, INT_MAX);
    UpdateWindow(Context->ListViewHandle);
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
            statisticsContext->PagePriority = ULONG_MAX;
            statisticsContext->IoPriority = LONG_MAX;

            // Try to open a process handle with PROCESS_QUERY_INFORMATION access for WS information.
            if (PH_IS_REAL_PROCESS_ID(processItem->ProcessId))
            {
                PhOpenProcess(
                    &statisticsContext->ProcessHandle,
                    PROCESS_QUERY_INFORMATION,
                    processItem->ProcessId
                    );
            }

            PhSetListViewStyle(statisticsContext->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(statisticsContext->ListViewHandle, L"explorer");
            PhAddListViewColumn(statisticsContext->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 135, L"Property");
            PhAddListViewColumn(statisticsContext->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 150, L"Value");
            PhSetExtendedListView(statisticsContext->ListViewHandle);
            ExtendedListView_SetTriState(statisticsContext->ListViewHandle, TRUE);
            PhpUpdateStatisticsAddListViewGroups(statisticsContext);
            PhLoadListViewColumnsFromSetting(L"ProcStatPropPageGroupListViewColumns", statisticsContext->ListViewHandle);
            PhLoadListViewSortColumnsFromSetting(L"ProcStatPropPageGroupListViewSort", statisticsContext->ListViewHandle);
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
            PhSaveListViewSortColumnsToSetting(L"ProcStatPropPageGroupListViewSort", statisticsContext->ListViewHandle);
            PhSaveListViewColumnsToSetting(L"ProcStatPropPageGroupListViewColumns", statisticsContext->ListViewHandle);
            PhSaveListViewGroupStatesToSetting(L"ProcStatPropPageGroupStates", statisticsContext->ListViewHandle);

            PhUnregisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                &statisticsContext->ProcessesUpdatedRegistration
                );

            if (statisticsContext->ProcessHandle)
                NtClose(statisticsContext->ProcessHandle);

            if (statisticsContext->ProcessExtension)
                PhFree(statisticsContext->ProcessExtension);

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

            //ExtendedListView_SetColumnWidth(statisticsContext->ListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
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
            case LVN_GETDISPINFO:
                {
                    NMLVDISPINFO* dispInfo = (NMLVDISPINFO*)header;

                    if (dispInfo->item.iSubItem == 1)
                    {
                        if (dispInfo->item.mask & LVIF_TEXT)
                        {
                            switch (PtrToUlong((PVOID)dispInfo->item.lParam))
                            {
                            case PH_PROCESS_STATISTICS_INDEX_PRIORITY:
                                {
                                    WCHAR priority[PH_INT32_STR_LEN_1] = L"";

                                    PhPrintInt32(priority, statisticsContext->ProcessItem->BasePriority);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, priority, _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CYCLES:
                                {
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhGetStringOrDefault(statisticsContext->Cycles, L"N/A"), _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_KERNELTIME:
                                {
                                    WCHAR timeSpan[PH_TIMESPAN_STR_LEN_1] = L"";

                                    PhPrintTimeSpan(timeSpan, statisticsContext->ProcessItem->KernelTime.QuadPart, PH_TIMESPAN_DHMSM);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, timeSpan, _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_USERTIME:
                                {
                                    WCHAR timeSpan[PH_TIMESPAN_STR_LEN_1] = L"";

                                    PhPrintTimeSpan(timeSpan, statisticsContext->ProcessItem->UserTime.QuadPart, PH_TIMESPAN_DHMSM);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, timeSpan, _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_TOTALTIME:
                                {
                                    WCHAR timeSpan[PH_TIMESPAN_STR_LEN_1] = L"";

                                    PhPrintTimeSpan(timeSpan, statisticsContext->ProcessItem->KernelTime.QuadPart + statisticsContext->ProcessItem->UserTime.QuadPart, PH_TIMESPAN_DHMSM);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, timeSpan, _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CYCLESDELTA:
                                {
                                    PPH_STRING value;

                                    //PhPrintUInt64(dispInfo->item.pszText, statisticsContext->ProcessItem->CycleTimeDelta.Delta);
                                    value = PhFormatUInt64(statisticsContext->ProcessItem->CycleTimeDelta.Delta, TRUE);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_KERNELDELTA:
                                {
                                    PPH_STRING value;

                                    //PhPrintUInt64(dispInfo->item.pszText, statisticsContext->ProcessItem->CpuKernelDelta.Delta);
                                    value = PhFormatUInt64(statisticsContext->ProcessItem->CpuKernelDelta.Delta, TRUE);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_USERDELTA:
                                {
                                    PPH_STRING value;

                                    //PhPrintUInt64(dispInfo->item.pszText, statisticsContext->ProcessItem->CpuUserDelta.Delta);
                                    value = PhFormatUInt64(statisticsContext->ProcessItem->CpuUserDelta.Delta, TRUE);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_TOTALDELTA:
                                {
                                    PPH_STRING value;

                                    //PhPrintUInt64(dispInfo->item.pszText, statisticsContext->ProcessItem->CpuKernelDelta.Delta + statisticsContext->ProcessItem->CpuUserDelta.Delta);
                                    value = PhFormatUInt64(statisticsContext->ProcessItem->CpuKernelDelta.Delta + statisticsContext->ProcessItem->CpuUserDelta.Delta, TRUE);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTES:
                                {
                                    PPH_STRING value;

                                    value = PhFormatSize(statisticsContext->ProcessItem->VmCounters.PagefileUsage, ULONG_MAX);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTESDELTA:
                                {
                                    PhpUpdateProcessStatisticDelta(statisticsContext, dispInfo, statisticsContext->ProcessItem->PrivateBytesDelta.Delta);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATEBYTES:
                                {
                                    PPH_STRING value;

                                    value = PhFormatSize(statisticsContext->ProcessItem->VmCounters.PeakPagefileUsage, ULONG_MAX);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_VIRTUALSIZE:
                                {
                                    PPH_STRING value;

                                    value = PhFormatSize(statisticsContext->ProcessItem->VmCounters.VirtualSize, ULONG_MAX);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKVIRTUALSIZE:
                                {
                                    PPH_STRING value;

                                    value = PhFormatSize(statisticsContext->ProcessItem->VmCounters.PeakVirtualSize, ULONG_MAX);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PAGEFAULTS:
                                {
                                    PPH_STRING value;

                                    value = PhFormatUInt64(statisticsContext->ProcessItem->VmCounters.PageFaultCount, TRUE);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PAGEFAULTSDELTA:
                                {
                                    PPH_STRING value;

                                    value = PhFormatUInt64(statisticsContext->ProcessItem->PageFaultsDelta.Delta, TRUE);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_HARDFAULTS:
                                {
                                    PPH_STRING value;

                                    value = PhFormatUInt64(statisticsContext->ProcessItem->HardFaultCount, TRUE);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_HARDFAULTSDELTA:
                                {
                                    PPH_STRING value;
                             
                                    value = PhFormatUInt64(statisticsContext->ProcessItem->HardFaultsDelta.Delta, TRUE);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WORKINGSET:
                                {
                                    PPH_STRING value;

                                    value = PhFormatSize(statisticsContext->ProcessItem->VmCounters.WorkingSetSize, ULONG_MAX);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKWORKINGSET:
                                {
                                    PPH_STRING value;

                                    value = PhFormatSize(statisticsContext->ProcessItem->VmCounters.PeakWorkingSetSize, ULONG_MAX);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIVATEWS:
                                {
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhGetStringOrDefault(statisticsContext->PrivateWs, L"N/A"), _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_SHAREABLEWS:
                                {
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhGetStringOrDefault(statisticsContext->ShareableWs, L"N/A"), _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_SHAREDWS:
                                {
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhGetStringOrDefault(statisticsContext->SharedWs, L"N/A"), _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PAGEPRIORITY:
                                {
                                    if (statisticsContext->PagePriority != ULONG_MAX && statisticsContext->PagePriority <= MEMORY_PRIORITY_NORMAL)
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhPagePriorityNames[statisticsContext->PagePriority], _TRUNCATE);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_SHAREDCOMMIT:
                                {
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhGetStringOrDefault(statisticsContext->SharedCommitUsage, L"N/A"), _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIVATECOMMIT:
                                {
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhGetStringOrDefault(statisticsContext->PrivateCommitUsage, L"N/A"), _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATECOMMIT:
                                {
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhGetStringOrDefault(statisticsContext->PeakPrivateCommitUsage, L"N/A"), _TRUNCATE);
                                }
                                break;
                            //case PH_PROCESS_STATISTICS_INDEX_PRIVATECOMMITLIMIT:
                            //    {
                            //        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhGetStringOrDefault(statisticsContext->PrivateCommitLimit, L"N/A"), _TRUNCATE);
                            //    }
                            //    break;
                            //case PH_PROCESS_STATISTICS_INDEX_TOTALCOMMITLIMIT:
                            //    {
                            //        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhGetStringOrDefault(statisticsContext->TotalCommitLimit, L"N/A"), _TRUNCATE);
                            //    }
                            //    break;
                            case PH_PROCESS_STATISTICS_INDEX_READS:
                                {
                                    PPH_STRING value;

                                    value = PhFormatUInt64(statisticsContext->ProcessItem->IoCounters.ReadOperationCount, TRUE);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_READSDELTA:
                                {
                                    PPH_STRING value;

                                    value = PhFormatUInt64(statisticsContext->ProcessItem->IoReadCountDelta.Delta, TRUE);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_READBYTES:
                                {
                                    PPH_STRING value;

                                    value = PhFormatSize(statisticsContext->ProcessItem->IoCounters.ReadTransferCount, ULONG_MAX);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_READBYTESDELTA:
                                {
                                    PhpUpdateProcessStatisticDeltaBytes(statisticsContext, dispInfo, statisticsContext->ProcessItem->IoReadDelta);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WRITES:
                                {
                                    PPH_STRING value;

                                    value = PhFormatUInt64(statisticsContext->ProcessItem->IoCounters.WriteOperationCount, TRUE);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WRITESDELTA:
                                {
                                    PPH_STRING value;

                                    value = PhFormatUInt64(statisticsContext->ProcessItem->IoWriteCountDelta.Delta, TRUE);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WRITEBYTES:
                                {
                                    PPH_STRING value;

                                    value = PhFormatSize(statisticsContext->ProcessItem->IoCounters.WriteTransferCount, ULONG_MAX);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WRITEBYTESDELTA:
                                {
                                    PhpUpdateProcessStatisticDeltaBytes(statisticsContext, dispInfo, statisticsContext->ProcessItem->IoWriteDelta);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_OTHER:
                                {
                                    PPH_STRING value;

                                    value = PhFormatUInt64(statisticsContext->ProcessItem->IoCounters.OtherOperationCount, TRUE);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_OTHERDELTA:
                                {
                                    PPH_STRING value;

                                    value = PhFormatUInt64(statisticsContext->ProcessItem->IoOtherCountDelta.Delta, TRUE);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_OTHERBYTES:
                                {
                                    PPH_STRING value;

                                    value = PhFormatSize(statisticsContext->ProcessItem->IoCounters.OtherTransferCount, ULONG_MAX);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_OTHERBYTESDELTA:
                                {
                                    PhpUpdateProcessStatisticDeltaBytes(statisticsContext, dispInfo, statisticsContext->ProcessItem->IoOtherDelta);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_IOPRIORITY:
                                {
                                    if (statisticsContext->IoPriority != LONG_MAX && statisticsContext->IoPriority >= IoPriorityVeryLow && statisticsContext->IoPriority <= MaxIoPriorityTypes)
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhIoPriorityHintNames[statisticsContext->IoPriority], _TRUNCATE);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_HANDLES:
                                {
                                    PPH_STRING value;

                                    value = PhFormatUInt64(statisticsContext->ProcessItem->NumberOfHandles, TRUE);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKHANDLES:
                                {
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhGetStringOrDefault(statisticsContext->PeakHandles, L"N/A"), _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_GDIHANDLES:
                                {
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhGetStringOrDefault(statisticsContext->GdiHandles, L"N/A"), _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_USERHANDLES:
                                {
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhGetStringOrDefault(statisticsContext->UserHandles, L"N/A"), _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PAGEDPOOL:
                                {
                                    PPH_STRING value;

                                    value = PhFormatSize(statisticsContext->ProcessItem->VmCounters.QuotaPagedPoolUsage, ULONG_MAX);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKPAGEDPOOL:
                                {
                                    PPH_STRING value;

                                    value = PhFormatSize(statisticsContext->ProcessItem->VmCounters.QuotaPeakPagedPoolUsage, ULONG_MAX);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_NONPAGED:
                                {
                                    PPH_STRING value;

                                    value = PhFormatSize(statisticsContext->ProcessItem->VmCounters.QuotaNonPagedPoolUsage, ULONG_MAX);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKNONPAGED:
                                {
                                    PPH_STRING value;

                                    value = PhFormatSize(statisticsContext->ProcessItem->VmCounters.QuotaPeakNonPagedPoolUsage, ULONG_MAX);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_RUNNINGTIME:
                                {
                                    WCHAR timeSpan[PH_TIMESPAN_STR_LEN_1] = L"";

                                    PhPrintTimeSpan(timeSpan, statisticsContext->RunningTime, PH_TIMESPAN_HMSM);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, timeSpan, _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_SUSPENDEDTIME:
                                {
                                    WCHAR timeSpan[PH_TIMESPAN_STR_LEN_1] = L"";

                                    PhPrintTimeSpan(timeSpan, statisticsContext->SuspendedTime, PH_TIMESPAN_HMSM);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, timeSpan, _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_HANGCOUNT:
                                {
                                    PPH_STRING value;

                                    value = PhFormatUInt64(statisticsContext->HangCount, TRUE);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_GHOSTCOUNT:
                                {
                                    PPH_STRING value;

                                    value = PhFormatUInt64(statisticsContext->GhostCount, TRUE);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;

                            case PH_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHES:
                                {
                                    PPH_STRING value;

                                    if (!statisticsContext->ProcessExtension)
                                        break;

                                    // TODO: ContextSwitchesDelta
                                    value = PhFormatUInt64(statisticsContext->ProcessExtension->ContextSwitches, TRUE);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            //case PH_PROCESS_STATISTICS_INDEX_DISKREAD:
                            //    {
                            //        PPH_STRING value;
                            //
                            //        if (!statisticsContext->ProcessExtension)
                            //            break;
                            //
                            //        value = PhFormatUInt64(statisticsContext->ProcessExtension->DiskCounters.BytesRead, TRUE);
                            //        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                            //        PhDereferenceObject(value);
                            //    }
                            //    break;
                            //case PH_PROCESS_STATISTICS_INDEX_DISKWRITE:
                            //    {
                            //        PPH_STRING value;
                            //
                            //        if (!statisticsContext->ProcessExtension)
                            //            break;
                            //
                            //        value = PhFormatSize(statisticsContext->ProcessExtension->DiskCounters.BytesWritten, ULONG_MAX);
                            //        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                            //        PhDereferenceObject(value);
                            //    }
                            //    break;
                            case PH_PROCESS_STATISTICS_INDEX_NETWORKTXRXBYTES:
                                {
                                    PPH_STRING value;

                                    if (!statisticsContext->ProcessExtension)
                                        break;

                                    value = PhFormatSize(statisticsContext->ProcessExtension->EnergyValues.NetworkTxRxBytes, ULONG_MAX);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                                    PhDereferenceObject(value);
                                }
                                break;
                            //case PH_PROCESS_STATISTICS_INDEX_MBBTXRXBYTES:
                            //    {
                            //        PPH_STRING value;
                            //
                            //        if (!statisticsContext->ProcessExtension)
                            //            break;
                            //
                            //        value = PhFormatSize(statisticsContext->ProcessExtension->EnergyValues.MBBTxRxBytes, ULONG_MAX);
                            //        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, value->Buffer, _TRUNCATE);
                            //        PhDereferenceObject(value);
                            //    }
                            //    break;
                            default:
                                {
                                    if (PhPluginsEnabled)
                                    {
                                        PH_PLUGIN_PROCESS_STATS_EVENT notifyEvent;

                                        notifyEvent.Version = 0;
                                        notifyEvent.Type = 2;
                                        notifyEvent.ProcessItem = statisticsContext->ProcessItem;
                                        notifyEvent.Parameter = dispInfo;

                                        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackProcessStatsNotifyEvent), &notifyEvent);
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
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
            //ExtendedListView_SetColumnWidth(statisticsContext->ListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
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
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}
