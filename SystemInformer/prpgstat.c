/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2023
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
} PH_PROCESS_STATISTICS_CATEGORY;

typedef enum _PH_PROCESS_STATISTICS_INDEX
{
    PH_PROCESS_STATISTICS_INDEX_CPU,
    PH_PROCESS_STATISTICS_INDEX_CPUUSER,
    PH_PROCESS_STATISTICS_INDEX_CPUKERNEL,
    PH_PROCESS_STATISTICS_INDEX_CPUAVERAGE,
    PH_PROCESS_STATISTICS_INDEX_CPURELATIVE,
    PH_PROCESS_STATISTICS_INDEX_CYCLES,
    PH_PROCESS_STATISTICS_INDEX_CYCLESDELTA,
    PH_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHES,
    PH_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHESDELTA,
    PH_PROCESS_STATISTICS_INDEX_KERNELTIME,
    PH_PROCESS_STATISTICS_INDEX_KERNELDELTA,
    PH_PROCESS_STATISTICS_INDEX_USERTIME,
    PH_PROCESS_STATISTICS_INDEX_USERDELTA,
    PH_PROCESS_STATISTICS_INDEX_TOTALTIME,
    PH_PROCESS_STATISTICS_INDEX_TOTALDELTA,
    PH_PROCESS_STATISTICS_INDEX_PRIORITY,

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
    PH_PROCESS_STATISTICS_INDEX_IOTOTAL,
    PH_PROCESS_STATISTICS_INDEX_IOTOTALDELTA,
    PH_PROCESS_STATISTICS_INDEX_IOAVERAGE,
    PH_PROCESS_STATISTICS_INDEX_IOPRIORITY,

    PH_PROCESS_STATISTICS_INDEX_HANDLES,
    PH_PROCESS_STATISTICS_INDEX_PEAKHANDLES,
    PH_PROCESS_STATISTICS_INDEX_GDIHANDLES,
    PH_PROCESS_STATISTICS_INDEX_PEAKGDIHANDLES,
    PH_PROCESS_STATISTICS_INDEX_USERHANDLES,
    PH_PROCESS_STATISTICS_INDEX_PEAKUSERHANDLES,

    PH_PROCESS_STATISTICS_INDEX_PAGEDPOOL,
    PH_PROCESS_STATISTICS_INDEX_PEAKPAGEDPOOL,
    PH_PROCESS_STATISTICS_INDEX_NONPAGED,
    PH_PROCESS_STATISTICS_INDEX_PEAKNONPAGED,

    PH_PROCESS_STATISTICS_INDEX_RUNNINGTIME,
    PH_PROCESS_STATISTICS_INDEX_SUSPENDEDTIME,
    PH_PROCESS_STATISTICS_INDEX_HANGCOUNT,
    PH_PROCESS_STATISTICS_INDEX_GHOSTCOUNT,
    PH_PROCESS_STATISTICS_INDEX_NETWORKTXRXBYTES,
    PH_PROCESS_STATISTICS_INDEX_MOUSE,
    PH_PROCESS_STATISTICS_INDEX_KEYBOARD,

    PH_PROCESS_STATISTICS_INDEX_MAX,
} PH_PROCESS_STATISTICS_INDEX;

VOID PhpUpdateStatisticsAddListViewGroups(
    _In_ PPH_STATISTICS_CONTEXT Context
    )
{
    PhListView_EnableGroupView(Context->ListViewContext, TRUE);

    PhListView_AddGroup(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_CPU, L"CPU");
    PhListView_AddGroup(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, L"Memory");
    PhListView_AddGroup(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_IO, L"I/O");
    PhListView_AddGroup(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_OTHER, L"Other");

    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_CPU, L"CPU", (PVOID)PH_PROCESS_STATISTICS_INDEX_CPU);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_CPUUSER, L"CPU (user)", (PVOID)PH_PROCESS_STATISTICS_INDEX_CPUUSER);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_CPUKERNEL, L"CPU (kernel)", (PVOID)PH_PROCESS_STATISTICS_INDEX_CPUKERNEL);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_CPUAVERAGE, L"CPU (average)", (PVOID)PH_PROCESS_STATISTICS_INDEX_CPUAVERAGE);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_CPURELATIVE, L"CPU (relative)", (PVOID)PH_PROCESS_STATISTICS_INDEX_CPURELATIVE);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_CYCLES, L"Cycles", (PVOID)PH_PROCESS_STATISTICS_INDEX_CYCLES);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_CYCLESDELTA, L"Cycles delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_CYCLESDELTA);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHES, L"Context switches", (PVOID)PH_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHES);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHESDELTA, L"Context switches delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHESDELTA);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_KERNELTIME, L"Kernel time", (PVOID)PH_PROCESS_STATISTICS_INDEX_KERNELTIME);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_KERNELDELTA, L"Kernel delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_KERNELDELTA);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_USERTIME, L"User time", (PVOID)PH_PROCESS_STATISTICS_INDEX_USERTIME);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_USERDELTA, L"User delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_USERDELTA);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_TOTALTIME, L"Total time", (PVOID)PH_PROCESS_STATISTICS_INDEX_TOTALTIME);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_TOTALDELTA, L"Total delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_TOTALDELTA);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_PRIORITY, L"Priority", (PVOID)PH_PROCESS_STATISTICS_INDEX_PRIORITY);

    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTES, L"Private bytes", (PVOID)PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTES);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTESDELTA, L"Private bytes delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTESDELTA);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATEBYTES, L"Peak private bytes", (PVOID)PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATEBYTES);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_VIRTUALSIZE, L"Virtual size", (PVOID)PH_PROCESS_STATISTICS_INDEX_VIRTUALSIZE);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PEAKVIRTUALSIZE, L"Peak virtual size", (PVOID)PH_PROCESS_STATISTICS_INDEX_PEAKVIRTUALSIZE);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PAGEFAULTS, L"Page faults", (PVOID)PH_PROCESS_STATISTICS_INDEX_PAGEFAULTS);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PAGEFAULTSDELTA, L"Page faults delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_PAGEFAULTSDELTA);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_HARDFAULTS, L"Hard faults", (PVOID)PH_PROCESS_STATISTICS_INDEX_HARDFAULTS);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_HARDFAULTSDELTA, L"Hard faults delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_HARDFAULTSDELTA);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_WORKINGSET, L"Working set", (PVOID)PH_PROCESS_STATISTICS_INDEX_WORKINGSET);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PEAKWORKINGSET, L"Peak working set", (PVOID)PH_PROCESS_STATISTICS_INDEX_PEAKWORKINGSET);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PRIVATEWS, L"Private WS", (PVOID)PH_PROCESS_STATISTICS_INDEX_PRIVATEWS);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_SHAREABLEWS, L"Shareable WS", (PVOID)PH_PROCESS_STATISTICS_INDEX_SHAREABLEWS);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_SHAREDWS, L"Shared WS", (PVOID)PH_PROCESS_STATISTICS_INDEX_SHAREDWS);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PAGEDPOOL, L"Paged pool bytes", (PVOID)PH_PROCESS_STATISTICS_INDEX_PAGEDPOOL);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PEAKPAGEDPOOL, L"Peak paged pool bytes", (PVOID)PH_PROCESS_STATISTICS_INDEX_PEAKPAGEDPOOL);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_NONPAGED, L"Nonpaged pool bytes", (PVOID)PH_PROCESS_STATISTICS_INDEX_NONPAGED);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PEAKNONPAGED, L"Peak nonpaged pool bytes", (PVOID)PH_PROCESS_STATISTICS_INDEX_PEAKNONPAGED);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_SHAREDCOMMIT, L"Shared commit", (PVOID)PH_PROCESS_STATISTICS_INDEX_SHAREDCOMMIT);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PRIVATECOMMIT, L"Private commit", (PVOID)PH_PROCESS_STATISTICS_INDEX_PRIVATECOMMIT);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATECOMMIT, L"Peak private commit", (PVOID)PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATECOMMIT);
    //PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PRIVATECOMMITLIMIT, L"Private commit limit", (PVOID)PH_PROCESS_STATISTICS_INDEX_PRIVATECOMMITLIMIT);
    //PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_TOTALCOMMITLIMIT, L"Total commit limit", (PVOID)PH_PROCESS_STATISTICS_INDEX_TOTALCOMMITLIMIT);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PAGEPRIORITY, L"Page priority", (PVOID)PH_PROCESS_STATISTICS_INDEX_PAGEPRIORITY);

    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_READS, L"Reads", (PVOID)PH_PROCESS_STATISTICS_INDEX_READS);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_READSDELTA, L"Reads delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_READSDELTA);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_READBYTES, L"Read bytes", (PVOID)PH_PROCESS_STATISTICS_INDEX_READBYTES);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_READBYTESDELTA, L"Read bytes delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_READBYTESDELTA);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_WRITES, L"Writes", (PVOID)PH_PROCESS_STATISTICS_INDEX_WRITES);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_WRITESDELTA, L"Writes delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_WRITESDELTA);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_WRITEBYTES, L"Write bytes", (PVOID)PH_PROCESS_STATISTICS_INDEX_WRITEBYTES);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_WRITEBYTESDELTA, L"Write bytes delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_WRITEBYTESDELTA);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_OTHER, L"Other", (PVOID)PH_PROCESS_STATISTICS_INDEX_OTHER);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_OTHERDELTA, L"Other delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_OTHERDELTA);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_OTHERBYTES, L"Other bytes", (PVOID)PH_PROCESS_STATISTICS_INDEX_OTHERBYTES);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_OTHERBYTESDELTA, L"Other bytes delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_OTHERBYTESDELTA);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_IOTOTAL, L"Total bytes", (PVOID)PH_PROCESS_STATISTICS_INDEX_IOTOTAL);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_IOTOTALDELTA, L"Total bytes delta", (PVOID)PH_PROCESS_STATISTICS_INDEX_IOTOTALDELTA);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_IOAVERAGE, L"Total bytes (average)", (PVOID)PH_PROCESS_STATISTICS_INDEX_IOAVERAGE);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_IOPRIORITY, L"I/O priority", (PVOID)PH_PROCESS_STATISTICS_INDEX_IOPRIORITY);

    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_HANDLES, L"Handles", (PVOID)PH_PROCESS_STATISTICS_INDEX_HANDLES);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_PEAKHANDLES, L"Peak handles", (PVOID)PH_PROCESS_STATISTICS_INDEX_PEAKHANDLES);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_GDIHANDLES, L"GDI handles", (PVOID)PH_PROCESS_STATISTICS_INDEX_GDIHANDLES);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_PEAKGDIHANDLES, L"Peak GDI handles", (PVOID)PH_PROCESS_STATISTICS_INDEX_PEAKGDIHANDLES);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_USERHANDLES, L"USER handles", (PVOID)PH_PROCESS_STATISTICS_INDEX_USERHANDLES);
    PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_PEAKUSERHANDLES, L"Peak USER handles", (PVOID)PH_PROCESS_STATISTICS_INDEX_PEAKUSERHANDLES);

    if (WindowsVersion >= WINDOWS_10_RS3)
    {
        PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_RUNNINGTIME, L"Running time", (PVOID)PH_PROCESS_STATISTICS_INDEX_RUNNINGTIME);
        PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_SUSPENDEDTIME, L"Suspended time", (PVOID)PH_PROCESS_STATISTICS_INDEX_SUSPENDEDTIME);
        PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_HANGCOUNT, L"Hang count", (PVOID)PH_PROCESS_STATISTICS_INDEX_HANGCOUNT);
        PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_GHOSTCOUNT, L"Ghost count", (PVOID)PH_PROCESS_STATISTICS_INDEX_GHOSTCOUNT);
        //PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_DISKREAD, L"BytesRead", NULL);
        //PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_DISKWRITE, L"BytesWritten", NULL);
        PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_NETWORKTXRXBYTES, L"NetworkTxRxBytes", (PVOID)PH_PROCESS_STATISTICS_INDEX_NETWORKTXRXBYTES);
        //PhListView_AddGroupItem(Context->ListViewContext, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_MBBTXRXBYTES, L"MBBTxRxBytes", NULL);
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

VOID PH_PROCESS_STATISTICS_FORMAT_F(
    _In_ NMLVDISPINFO* Entry,
    _In_ FLOAT Value
    )
{
    PH_FORMAT format[2];
    WCHAR buffer[PH_INT64_STR_LEN_1];

    // %.2f%%
    PhInitFormatF(&format[0], (Value), PhMaxPrecisionUnit);
    PhInitFormatC(&format[1], L'%');

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
    {
        wcsncpy_s(Entry->item.pszText, Entry->item.cchTextMax, buffer, _TRUNCATE);
    }
}

VOID PH_PROCESS_STATISTICS_FORMAT_I64U(
    _In_ NMLVDISPINFO* Entry,
    _In_ ULONG64 Value
    )
{
    PH_FORMAT format[1];
    WCHAR buffer[PH_INT64_STR_LEN_1];

    PhInitFormatI64UGroupDigits(&format[0], Value);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
    {
        wcsncpy_s(Entry->item.pszText, Entry->item.cchTextMax, buffer, _TRUNCATE);
    }
}

VOID PH_PROCESS_STATISTICS_FORMAT_SIZE(
    _In_ NMLVDISPINFO* Entry,
    _In_ ULONG64 Value
    )
{
    PH_FORMAT format[1];
    WCHAR buffer[PH_INT64_STR_LEN_1];

    PhInitFormatSize(&format[0], Value);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
    {
        wcsncpy_s(Entry->item.pszText, Entry->item.cchTextMax, buffer, _TRUNCATE);
    }
}

VOID PH_PROCESS_STATISTICS_FORMAT_TIME(
    _In_ NMLVDISPINFO* Entry,
    _In_ ULONG64 Value
    )
{
    WCHAR buffer[PH_TIMESPAN_STR_LEN_1] = L"";

    if (PhPrintTimeSpanToBuffer(Value, PH_TIMESPAN_DHMSM, buffer, sizeof(buffer), NULL))
    {
        wcsncpy_s(Entry->item.pszText, Entry->item.cchTextMax, buffer, _TRUNCATE);
    }
}

#define PH_PROCESS_STATISTICS_UPDATE_MINMAX(Minimum, Maximum, Difference, Value) \
    if ((Value) != 0 && ((Minimum) == 0 || (Value) < (Minimum))) \
    (Minimum) = (Value); \
    if ((Value) != 0 && ((Maximum) == 0 || (Value) > (Maximum))) \
        (Maximum) = (Value); \
    (Difference) = (Maximum)-(Minimum);

#define PH_PROCESS_STATISTICS_UPDATE_INCREMENTALMINMAX(Context, Type, Last, Minimum, Maximum, NewValue) \
    if ((Last).Value) \
    { \
        Type delta = (NewValue) - (Last).Value; \
        if (delta != 0 && ((Minimum) == 0 || delta < (Minimum))) \
            (Minimum) = delta; \
        if (delta != 0 && ((Maximum) == 0 || delta > (Maximum))) \
            (Maximum) = delta; \
    } PhUpdateDelta(&(Last), (NewValue));

VOID PhUpdateProcessStatisticsValue(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_STATISTICS_CONTEXT Context
    )
{
    Context->CpuUsage = ProcessItem->CpuUsage * 100;
    Context->CpuUsageUser = ProcessItem->CpuUserUsage * 100;
    Context->CpuUsageKernel = ProcessItem->CpuKernelUsage * 100;

    if (FlagOn(PhProcessProviderFlagsMask, PH_PROCESS_PROVIDER_FLAG_AVERAGE))
    {
        Context->CpuUsageAverage = ProcessItem->CpuAverageUsage * 100;
    }
    else
    {
        FLOAT cpuSumValue = 0;
        FLOAT cpuAverageValue = 0;

        for (ULONG i = 0; i < ProcessItem->CpuKernelHistory.Count; i++)
        {
            cpuSumValue += PhGetItemCircularBuffer_FLOAT(&ProcessItem->CpuKernelHistory, i) +
                PhGetItemCircularBuffer_FLOAT(&ProcessItem->CpuUserHistory, i);
        }

        if (ProcessItem->CpuKernelHistory.Count)
        {
            cpuAverageValue = (FLOAT)cpuSumValue / ProcessItem->CpuKernelHistory.Count;
        }

        Context->CpuUsageAverage = cpuAverageValue * 100;
    }

    Context->CpuUsageRelative = (FLOAT)(ProcessItem->CpuUsage * 100) * ProcessItem->AffinityPopulationCount;

    {
        ULONG64 cycleTime;

        if (ProcessItem->QueryHandle && NT_SUCCESS(PhGetProcessCycleTime(ProcessItem->QueryHandle, &cycleTime)))
        {
            Context->CycleTime = cycleTime;
            Context->GotCycles = TRUE;
        }
        else
        {
            Context->CycleTime = 0;
            Context->GotCycles = FALSE;
        }
    }

    Context->CycleTimeDelta = ProcessItem->CycleTimeDelta.Delta;
    Context->ContextSwitches = ProcessItem->ContextSwitches;
    Context->ContextSwitchesDelta = ProcessItem->ContextSwitchesDelta.Delta;
    Context->KernelTime = ProcessItem->KernelTime.QuadPart;
    Context->KernelTimeDelta = ProcessItem->CpuKernelDelta.Delta;
    Context->UserTime = ProcessItem->UserTime.QuadPart;
    Context->UserTimeDelta = ProcessItem->CpuUserDelta.Delta;
    Context->BasePriority = ProcessItem->BasePriority;

    Context->PagefileUsage = ProcessItem->PrivateBytesDelta.Value; // ProcessItem->VmCounters.PagefileUsage
    Context->PagefileDelta = ProcessItem->PrivateBytesDelta.Delta;
    Context->PeakPagefileUsage = ProcessItem->VmCounters.PeakPagefileUsage;
    Context->VirtualSize = ProcessItem->VmCounters.VirtualSize;
    Context->PeakVirtualSize = ProcessItem->VmCounters.PeakVirtualSize;
    Context->PageFaultCount = ProcessItem->PageFaultsDelta.Value; // ProcessItem->VmCounters.PageFaultCount
    Context->PageFaultsDelta = ProcessItem->PageFaultsDelta.Delta;
    Context->HardFaultCount = ProcessItem->HardFaultsDelta.Value; // ProcessItem->HardFaultCount
    Context->HardFaultsDelta = ProcessItem->HardFaultsDelta.Delta;
    Context->WorkingSetSize = ProcessItem->VmCounters.WorkingSetSize;
    Context->PeakWorkingSetSize = ProcessItem->VmCounters.PeakWorkingSetSize;

    {
        PH_PROCESS_WS_COUNTERS wsCounters;
        PROCESS_JOB_MEMORY_INFO appMemoryInfo;
        PROCESS_EXTENDED_ENERGY_VALUES processExtendedValues;

        if (Context->ProcessHandle && NT_SUCCESS(PhGetProcessWsCounters(Context->ProcessHandle, &wsCounters)))
        {
            Context->NumberOfPrivatePages = wsCounters.NumberOfPrivatePages * PAGE_SIZE;
            Context->NumberOfShareablePages = wsCounters.NumberOfShareablePages * PAGE_SIZE;
            Context->NumberOfSharedPages = wsCounters.NumberOfSharedPages * PAGE_SIZE;
        }
        else
        {
            Context->NumberOfPrivatePages = ProcessItem->WorkingSetPrivateSize;
            Context->NumberOfShareablePages = 0;
            Context->NumberOfSharedPages = 0;
        }

        if (Context->ProcessHandle && NT_SUCCESS(PhGetProcessAppMemoryInformation(Context->ProcessHandle, &appMemoryInfo)))
        {
            Context->SharedCommitUsage = appMemoryInfo.SharedCommitUsage;
            Context->PrivateCommitUsage = appMemoryInfo.PrivateCommitUsage;
            Context->PeakPrivateCommitUsage = appMemoryInfo.PeakPrivateCommitUsage;
        }
        else
        {
            Context->SharedCommitUsage = 0;
            Context->PrivateCommitUsage = 0;
            Context->PeakPrivateCommitUsage = 0;
        }

        if (Context->ProcessHandle && NT_SUCCESS(PhGetProcessEnergyValues(Context->ProcessHandle, &processExtendedValues)))
        {
            PhUpdateDelta(&Context->MouseDelta, processExtendedValues.Extension.MouseInput);
            PhUpdateDelta(&Context->KeyboardDelta, processExtendedValues.Extension.KeyboardInput);
        }
        else
        {
            //PhUpdateDelta(&Context->MouseDelta, 0);
            //PhUpdateDelta(&Context->KeyboardDelta, 0);
        }
    }

    Context->QuotaPagedPoolUsage = ProcessItem->VmCounters.QuotaPagedPoolUsage;
    Context->QuotaPeakPagedPoolUsage = ProcessItem->VmCounters.QuotaPeakPagedPoolUsage;
    Context->QuotaNonPagedPoolUsage = ProcessItem->VmCounters.QuotaNonPagedPoolUsage;
    Context->QuotaPeakNonPagedPoolUsage = ProcessItem->VmCounters.QuotaPeakNonPagedPoolUsage;

    //wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhGetStringOrDefault(statisticsContext->PrivateWs, L"N/A"), _TRUNCATE);
    //wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhGetStringOrDefault(statisticsContext->ShareableWs, L"N/A"), _TRUNCATE);
    //wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhGetStringOrDefault(statisticsContext->SharedWs, L"N/A"), _TRUNCATE);
    //wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhGetStringOrDefault(statisticsContext->SharedCommitUsage, L"N/A"), _TRUNCATE);
    //wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhGetStringOrDefault(statisticsContext->PrivateCommitUsage, L"N/A"), _TRUNCATE);
    //wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhGetStringOrDefault(statisticsContext->PeakPrivateCommitUsage, L"N/A"), _TRUNCATE);

    Context->ReadOperationCount = ProcessItem->IoCounters.ReadOperationCount;
    Context->IoReadCountDelta = ProcessItem->IoReadCountDelta.Delta;
    Context->ReadTransferCount = ProcessItem->IoCounters.ReadTransferCount;
    Context->IoReadDelta = ProcessItem->IoReadDelta.Delta;

    Context->WriteOperationCount = ProcessItem->IoCounters.WriteOperationCount;
    Context->IoWriteCountDelta = ProcessItem->IoWriteCountDelta.Delta;
    Context->WriteTransferCount = ProcessItem->IoCounters.WriteTransferCount;
    Context->IoWriteDelta = ProcessItem->IoWriteDelta.Delta;

    Context->OtherOperationCount = ProcessItem->IoCounters.OtherOperationCount;
    Context->IoOtherCountDelta = ProcessItem->IoOtherCountDelta.Delta;
    Context->OtherTransferCount = ProcessItem->IoCounters.OtherTransferCount;
    Context->IoOtherDelta = ProcessItem->IoOtherDelta.Delta;

    Context->IoTotalDelta = ProcessItem->IoReadDelta.Value + ProcessItem->IoWriteDelta.Value + ProcessItem->IoOtherDelta.Value;
}

VOID PhUpdateProcessStatisticsMinMax(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_STATISTICS_CONTEXT Context
    )
{
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->CpuUsageMin, Context->CpuUsageMax, Context->CpuUsageDiff, Context->CpuUsage);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->CpuUsageUserMin, Context->CpuUsageUserMax, Context->CpuUsageUserDiff, Context->CpuUsageUser);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->CpuUsageKernelMin, Context->CpuUsageKernelMax, Context->CpuUsageKernelDiff, Context->CpuUsageKernel);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->CpuUsageAverageMin, Context->CpuUsageAverageMax, Context->CpuUsageAverageDiff, Context->CpuUsageAverage);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->CpuUsageRelativeMin, Context->CpuUsageRelativeMax, Context->CpuUsageRelativeDiff, Context->CpuUsageRelative);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->CycleTimeMin, Context->CycleTimeMax, Context->CycleTimeDiff, Context->CycleTime);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->CycleTimeDeltaMin, Context->CycleTimeDeltaMax, Context->CycleTimeDeltaDiff, Context->CycleTimeDelta);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->ContextSwitchesMin, Context->ContextSwitchesMax, Context->ContextSwitchesDiff, Context->ContextSwitches);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->ContextSwitchesDeltaMin, Context->ContextSwitchesDeltaMax, Context->ContextSwitchesDeltaDiff, Context->ContextSwitchesDelta);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->KernelTimeMin, Context->KernelTimeMax, Context->KernelTimeDiff, Context->KernelTime);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->KernelTimeDeltaMin, Context->KernelTimeDeltaMax, Context->KernelTimeDeltaDiff, Context->KernelTimeDelta);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->UserTimeMin, Context->UserTimeMax, Context->UserTimeDiff, Context->UserTime);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->UserTimeDeltaMin, Context->UserTimeDeltaMax, Context->UserTimeDeltaDiff, Context->UserTimeDelta);

    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->QuotaPagedPoolUsageMin, Context->QuotaPagedPoolUsageMax, Context->QuotaPagedPoolUsageDiff, Context->QuotaPagedPoolUsage);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->QuotaPeakPagedPoolUsageMin, Context->QuotaPeakPagedPoolUsageMax, Context->QuotaPeakPagedPoolUsageDiff, Context->QuotaPeakPagedPoolUsage);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->QuotaNonPagedPoolUsageMin, Context->QuotaNonPagedPoolUsageMax, Context->QuotaNonPagedPoolUsageDiff, Context->QuotaNonPagedPoolUsage);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->QuotaPeakNonPagedPoolUsageMin, Context->QuotaPeakNonPagedPoolUsageMax, Context->QuotaPeakNonPagedPoolUsageDiff, Context->QuotaPeakNonPagedPoolUsage);

    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->PagefileUsageMin, Context->PagefileUsageMax, Context->PagefileUsageDiff, Context->PagefileUsage);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->PagefileDeltaMin, Context->PagefileDeltaMax, Context->PagefileDeltaDiff, Context->PagefileDelta);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->PeakPagefileUsageMin, Context->PeakPagefileUsageMax, Context->PeakPagefileUsageDiff, Context->PeakPagefileUsage);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->VirtualSizeMin, Context->VirtualSizeMax, Context->VirtualSizeDiff, Context->VirtualSize);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->PeakVirtualSizeMin, Context->PeakVirtualSizeMax, Context->PeakVirtualSizeDiff, Context->PeakVirtualSize);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->PageFaultCountMin, Context->PageFaultCountMax, Context->PageFaultCountDiff, Context->PageFaultCount);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->PageFaultsDeltaMin, Context->PageFaultsDeltaMax, Context->PageFaultsDeltaDiff, Context->PageFaultsDelta);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->HardFaultCountMin, Context->HardFaultCountMax, Context->HardFaultCountDiff, Context->HardFaultCount);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->HardFaultsDeltaMin, Context->HardFaultsDeltaMax, Context->HardFaultsDeltaDiff, Context->HardFaultsDelta);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->WorkingSetSizeMin, Context->WorkingSetSizeMax, Context->WorkingSetSizeDiff, Context->WorkingSetSize);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->PeakWorkingSetSizeMin, Context->PeakWorkingSetSizeMax, Context->PeakWorkingSetSizeDiff, Context->PeakWorkingSetSize);

    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->ReadOperationCountMin, Context->ReadOperationCountMax, Context->ReadOperationCountDiff, Context->ReadOperationCount);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->IoReadCountDeltaMin, Context->IoReadCountDeltaMax, Context->IoReadCountDeltaDiff, Context->IoReadCountDelta);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->ReadTransferCountMin, Context->ReadTransferCountMax, Context->ReadTransferCountDiff, Context->ReadTransferCount);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->IoReadDeltaMin, Context->IoReadDeltaMax, Context->IoReadDeltaDiff, Context->IoReadDelta);

    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->WriteOperationCountMin, Context->WriteOperationCountMax, Context->WriteOperationCountDiff, Context->WriteOperationCount);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->IoWriteCountDeltaMin, Context->IoWriteCountDeltaMax, Context->IoWriteCountDeltaDiff, Context->IoWriteCountDelta);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->WriteTransferCountMin, Context->WriteTransferCountMax, Context->WriteTransferCountDiff, Context->WriteTransferCount);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->IoWriteDeltaMin, Context->IoWriteDeltaMax, Context->IoWriteDeltaDiff, Context->IoWriteDelta);

    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->OtherOperationCountMin, Context->OtherOperationCountMax, Context->OtherOperationCountDiff, Context->OtherOperationCount);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->IoOtherCountDeltaMin, Context->IoOtherCountDeltaMax, Context->IoOtherCountDeltaDiff, Context->IoOtherCountDelta);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->OtherTransferCountMin, Context->OtherTransferCountMax, Context->OtherTransferCountDiff, Context->OtherTransferCount);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->IoOtherDeltaMin, Context->IoOtherDeltaMax, Context->IoOtherDeltaDiff, Context->IoOtherDelta);

    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->IoTotalMin, Context->IoTotalMax, Context->IoTotalDiff, Context->IoTotal);
    PH_PROCESS_STATISTICS_UPDATE_MINMAX(Context->IoTotalDeltaMin, Context->IoTotalDeltaMax, Context->IoTotalDeltaDiff, Context->IoTotalDelta);
}

VOID PhpUpdateProcessStatistics(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_STATISTICS_CONTEXT Context
    )
{
    PhUpdateProcessStatisticsValue(ProcessItem, Context);
    PhUpdateProcessStatisticsMinMax(ProcessItem, Context);

    if (ProcessItem->ProcessId == SYSTEM_PROCESS_ID)
    {
        PROCESS_HANDLE_INFORMATION handleInfo;
        PROCESS_UPTIME_INFORMATION uptimeInfo;
        ULONG objects;

        if (NT_SUCCESS(PhGetSessionGuiResources(GR_GDIOBJECTS, &objects))) // GDI handles
        {
            PhMoveReference(&Context->GdiHandles, PhFormatUInt64(objects, TRUE));
        }

        if (NT_SUCCESS(PhGetSessionGuiResources(GR_USEROBJECTS, &objects))) // USER handles
        {
            PhMoveReference(&Context->UserHandles, PhFormatUInt64(objects, TRUE));
        }

        if (NT_SUCCESS(PhGetSessionGuiResources(GR_GDIOBJECTS_PEAK, &objects))) // GDI handles (Peak)
        {
            PhMoveReference(&Context->PeakGdiHandles, PhFormatUInt64(objects, TRUE));
        }

        if (NT_SUCCESS(PhGetSessionGuiResources(GR_USEROBJECTS_PEAK, &objects))) // USER handles (Peak)
        {
            PhMoveReference(&Context->PeakUserHandles, PhFormatUInt64(objects, TRUE));
        }

        if (ProcessItem->QueryHandle)
        {
            if (NT_SUCCESS(PhGetProcessHandleCount(ProcessItem->QueryHandle, &handleInfo)))
            {
                // The HighWatermark changed on Windows 10 and doesn't track the peak handle count,
                // instead the value is now the maximum count of the handle freelist which decreases
                // when deallocating. So we'll track and cache the highest value where possible. (dmex)
                if (handleInfo.HandleCountHighWatermark > Context->PeakHandleCount)
                    Context->PeakHandleCount = handleInfo.HandleCountHighWatermark;

                PhMoveReference(&Context->PeakHandles, PhFormatUInt64(Context->PeakHandleCount, TRUE));
            }

            PhGetProcessPagePriority(ProcessItem->QueryHandle, &Context->PagePriority);
            PhGetProcessIoPriority(ProcessItem->QueryHandle, &Context->IoPriority);

            if (WindowsVersion >= WINDOWS_10_RS3 && NT_SUCCESS(PhGetProcessUptime(ProcessItem->QueryHandle, &uptimeInfo)))
            {
                Context->RunningTime = uptimeInfo.Uptime;
                Context->SuspendedTime = uptimeInfo.SuspendedTime;
                Context->HangCount = uptimeInfo.HangCount;
                Context->GhostCount = uptimeInfo.GhostCount;
            }
        }
    }
    else if (ProcessItem->QueryHandle)
    {
        PROCESS_HANDLE_INFORMATION handleInfo;
        PROCESS_UPTIME_INFORMATION uptimeInfo;
        ULONG objects;
        ULONG objectsTotal;

        if (NT_SUCCESS(PhGetProcessHandleCount(ProcessItem->QueryHandle, &handleInfo)))
        {
            // The HighWatermark changed on Windows 10 and doesn't track the peak handle count,
            // instead the value is now the maximum count of the handle freelist which decreases
            // when deallocating. So we'll track and cache the highest value where possible. (dmex)
            if (handleInfo.HandleCountHighWatermark > Context->PeakHandleCount)
                Context->PeakHandleCount = handleInfo.HandleCountHighWatermark;

            PhMoveReference(&Context->PeakHandles, PhFormatUInt64(Context->PeakHandleCount, TRUE));
        }

        if (NT_SUCCESS(PhGetProcessGuiResources(ProcessItem->QueryHandle, GR_GDIOBJECTS, &objects))) // GDI handles
        {
            if (!NT_SUCCESS(PhGetSessionGuiResources(GR_GDIOBJECTS, &objectsTotal)))
                objectsTotal = 1;

            PPH_STRING string = PhFormatUInt64(objects, TRUE);
            PhMoveReference(&string, PhFormatString(L"%s (%.2f%%)", PhGetString(string), (FLOAT)objects / (FLOAT)objectsTotal * 100.0f));
            PhMoveReference(&Context->GdiHandles, string);
        }

        if (NT_SUCCESS(PhGetProcessGuiResources(ProcessItem->QueryHandle, GR_USEROBJECTS, &objects))) // USER handles
        {
            if (!NT_SUCCESS(PhGetSessionGuiResources(GR_USEROBJECTS, &objectsTotal)))
                objectsTotal = 1;

            PPH_STRING string = PhFormatUInt64(objects, TRUE);
            PhMoveReference(&string, PhFormatString(L"%s (%.2f%%)", PhGetString(string), (FLOAT)objects / (FLOAT)objectsTotal * 100.0f));
            PhMoveReference(&Context->UserHandles, string);
        }

        if (NT_SUCCESS(PhGetProcessGuiResources(ProcessItem->QueryHandle, GR_GDIOBJECTS_PEAK, &objects))) // GDI handles (Peak)
        {
            if (!NT_SUCCESS(PhGetSessionGuiResources(GR_GDIOBJECTS_PEAK, &objectsTotal)))
                objectsTotal = 1;

            PPH_STRING string = PhFormatUInt64(objects, TRUE);
            PhMoveReference(&string, PhFormatString(L"%s (%.2f%%)", PhGetString(string), (FLOAT)objects / (FLOAT)objectsTotal * 100.0f));
            PhMoveReference(&Context->PeakGdiHandles, string);
        }

        if (NT_SUCCESS(PhGetProcessGuiResources(ProcessItem->QueryHandle, GR_USEROBJECTS_PEAK, &objects))) // USER handles (Peak)
        {
            if (!NT_SUCCESS(PhGetSessionGuiResources(GR_USEROBJECTS_PEAK, &objectsTotal)))
                objectsTotal = 1;

            PPH_STRING string = PhFormatUInt64(objects, TRUE);
            PhMoveReference(&string, PhFormatString(L"%s (%.2f%%)", PhGetString(string), (FLOAT)objects / (FLOAT)objectsTotal * 100.0f));
            PhMoveReference(&Context->PeakUserHandles, string);
        }

        PhGetProcessPagePriority(ProcessItem->QueryHandle, &Context->PagePriority);
        PhGetProcessIoPriority(ProcessItem->QueryHandle, &Context->IoPriority);

        if (WindowsVersion >= WINDOWS_10_RS3 && NT_SUCCESS(PhGetProcessUptime(ProcessItem->QueryHandle, &uptimeInfo)))
        {
            Context->RunningTime = uptimeInfo.Uptime;
            Context->SuspendedTime = uptimeInfo.SuspendedTime;
            Context->HangCount = uptimeInfo.HangCount;
            Context->GhostCount = uptimeInfo.GhostCount;
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
                //Context->ContextSwitches = processExtension->ContextSwitches;
                Context->NetworkTxRxBytes = processExtension->EnergyValues.NetworkTxRxBytes;
            }

            PhFree(processes);
        }
    }

    PhRedrawListViewItems(Context->ListViewHandle);
}

_Function_class_(PH_CALLBACK_FUNCTION)
static VOID NTAPI PhpStatisticsUpdateHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_STATISTICS_CONTEXT statisticsContext = (PPH_STATISTICS_CONTEXT)Context;

    if (statisticsContext->Enabled)
    {
        PostMessage(statisticsContext->WindowHandle, WM_PH_STATISTICS_UPDATE, 0, 0);
    }
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
            statisticsContext->ListViewContext = PhListView_Initialize(statisticsContext->ListViewHandle);
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
            PhSetExtendedListView(statisticsContext->ListViewHandle);
            PhListView_AddColumn(statisticsContext->ListViewContext, 0, 0, 0, LVCFMT_LEFT, 135, L"Property");
            PhListView_AddColumn(statisticsContext->ListViewContext, 1, 1, 1, LVCFMT_LEFT, 150, L"Value");
            PhListView_AddColumn(statisticsContext->ListViewContext, 2, 2, 2, LVCFMT_LEFT, 150, L"Min");
            PhListView_AddColumn(statisticsContext->ListViewContext, 3, 3, 3, LVCFMT_LEFT, 150, L"Max");
            PhListView_AddColumn(statisticsContext->ListViewContext, 4, 4, 4, LVCFMT_LEFT, 150, L"Difference");
            ExtendedListView_SetTriState(statisticsContext->ListViewHandle, TRUE);
            PhpUpdateStatisticsAddListViewGroups(statisticsContext);
            PhLoadListViewColumnsFromSetting(SETTING_PROC_STAT_PROP_PAGE_GROUP_LIST_VIEW_COLUMNS, statisticsContext->ListViewHandle);
            PhLoadListViewSortColumnsFromSetting(SETTING_PROC_STAT_PROP_PAGE_GROUP_LIST_VIEW_SORT, statisticsContext->ListViewHandle);
            PhLoadListViewGroupStatesFromSetting(SETTING_PROC_STAT_PROP_PAGE_GROUP_STATES, statisticsContext->ListViewHandle);

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
            PhSaveListViewSortColumnsToSetting(SETTING_PROC_STAT_PROP_PAGE_GROUP_LIST_VIEW_SORT, statisticsContext->ListViewHandle);
            PhSaveListViewColumnsToSetting(SETTING_PROC_STAT_PROP_PAGE_GROUP_LIST_VIEW_COLUMNS, statisticsContext->ListViewHandle);
            PhSaveListViewGroupStatesToSetting(SETTING_PROC_STAT_PROP_PAGE_GROUP_STATES, statisticsContext->ListViewHandle);

            PhUnregisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                &statisticsContext->ProcessesUpdatedRegistration
                );

            if (statisticsContext->ListViewContext)
            {
                PhListView_Destroy(statisticsContext->ListViewContext);
                statisticsContext->ListViewContext = NULL;
            }
        }
        break;
    case WM_NCDESTROY:
        {
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
                        if (FlagOn(dispInfo->item.mask, LVIF_TEXT))
                        {
                            switch (PtrToUlong((PVOID)dispInfo->item.lParam))
                            {
                            case PH_PROCESS_STATISTICS_INDEX_CPU:
                                PH_PROCESS_STATISTICS_FORMAT_F(dispInfo, statisticsContext->CpuUsage);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CPUUSER:
                                PH_PROCESS_STATISTICS_FORMAT_F(dispInfo, statisticsContext->CpuUsageUser);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CPUKERNEL:
                                PH_PROCESS_STATISTICS_FORMAT_F(dispInfo, statisticsContext->CpuUsageKernel);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CPUAVERAGE:
                                PH_PROCESS_STATISTICS_FORMAT_F(dispInfo, statisticsContext->CpuUsageAverage);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CPURELATIVE:
                                PH_PROCESS_STATISTICS_FORMAT_F(dispInfo, statisticsContext->CpuUsageRelative);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CYCLES:
                                {
                                    PH_FORMAT format[1];
                                    WCHAR buffer[PH_INT64_STR_LEN_1];

                                    if (statisticsContext->GotCycles)
                                    {
                                        PhInitFormatI64UGroupDigits(&format[0], statisticsContext->CycleTime);
                                    }
                                    //else if (statisticsContext->CycleTimeDelta)
                                    //{
                                    //    PhInitFormatI64UGroupDigits(&format[0], statisticsContext->CycleTimeDelta);
                                    //}
                                    else
                                    {
                                        PhInitFormatS(&format[0], L"N/A");
                                    }

                                    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                                    {
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                                    }
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_KERNELTIME:
                                PH_PROCESS_STATISTICS_FORMAT_TIME(dispInfo, statisticsContext->KernelTime);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_USERTIME:
                                PH_PROCESS_STATISTICS_FORMAT_TIME(dispInfo, statisticsContext->UserTime);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_TOTALTIME:
                                PH_PROCESS_STATISTICS_FORMAT_TIME(dispInfo, statisticsContext->KernelTime + statisticsContext->UserTime);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CYCLESDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->CycleTimeDelta);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHES:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->ContextSwitches);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHESDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->ContextSwitchesDelta);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_KERNELDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->KernelTimeDelta);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_USERDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->UserTimeDelta);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_TOTALDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->KernelTimeDelta + statisticsContext->UserTimeDelta);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIORITY:
                                {
                                    WCHAR priority[PH_INT32_STR_LEN_1] = L"";

                                    PhPrintInt32(priority, statisticsContext->ProcessItem->BasePriority);
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, priority, _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTES:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->ProcessItem->VmCounters.PagefileUsage);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTESDELTA:
                                PhpUpdateProcessStatisticDelta(dispInfo, statisticsContext->ProcessItem->PrivateBytesDelta.Delta);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATEBYTES:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->ProcessItem->VmCounters.PeakPagefileUsage);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_VIRTUALSIZE:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->ProcessItem->VmCounters.VirtualSize);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKVIRTUALSIZE:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->ProcessItem->VmCounters.PeakVirtualSize);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PAGEFAULTS:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->ProcessItem->VmCounters.PageFaultCount);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PAGEFAULTSDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->ProcessItem->PageFaultsDelta.Delta);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_HARDFAULTS:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->ProcessItem->HardFaultCount);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_HARDFAULTSDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->ProcessItem->HardFaultsDelta.Delta);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WORKINGSET:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->ProcessItem->VmCounters.WorkingSetSize);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKWORKINGSET:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->ProcessItem->VmCounters.PeakWorkingSetSize);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIVATEWS:
                                {
                                    if (statisticsContext->NumberOfPrivatePages)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->NumberOfPrivatePages);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_SHAREABLEWS:
                                {
                                    if (statisticsContext->NumberOfShareablePages)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->NumberOfShareablePages);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_SHAREDWS:
                                {
                                    if (statisticsContext->NumberOfSharedPages)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->NumberOfSharedPages);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PAGEDPOOL:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->QuotaPagedPoolUsage);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKPAGEDPOOL:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->QuotaPeakPagedPoolUsage);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_NONPAGED:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->QuotaNonPagedPoolUsage);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKNONPAGED:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->QuotaPeakNonPagedPoolUsage);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_SHAREDCOMMIT:
                                {
                                    if (statisticsContext->SharedCommitUsage)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->SharedCommitUsage);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIVATECOMMIT:
                                {
                                    if (statisticsContext->PrivateCommitUsage)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->PrivateCommitUsage);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATECOMMIT:
                                {
                                    if (statisticsContext->PeakPrivateCommitUsage)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->PeakPrivateCommitUsage);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PAGEPRIORITY:
                                {
                                    if (statisticsContext->PagePriority != ULONG_MAX && statisticsContext->PagePriority <= MEMORY_PRIORITY_NORMAL)
                                    {
                                        const PH_STRINGREF pagePriority = PhPagePriorityNames[statisticsContext->PagePriority];

                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, pagePriority.Buffer, _TRUNCATE);
                                    }
                                    else
                                    {
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                    }
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
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->ReadOperationCount);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_READSDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->IoReadCountDelta);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_READBYTES:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->ReadTransferCount);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_READBYTESDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->IoReadDelta);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WRITES:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->WriteOperationCount);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WRITESDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->IoWriteCountDelta);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WRITEBYTES:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->WriteTransferCount);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WRITEBYTESDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->IoWriteDelta);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_OTHER:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->OtherOperationCount);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_OTHERDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->IoOtherCountDelta);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_OTHERBYTES:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->OtherTransferCount);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_OTHERBYTESDELTA:
                                PhpUpdateProcessStatisticDeltaBytes(dispInfo, statisticsContext->ProcessItem->IoOtherDelta);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_IOTOTAL:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->IoTotalDelta);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_IOTOTALDELTA:
                                {
                                    PH_PROCESS_STATISTICS_FORMAT_SIZE(
                                        dispInfo,
                                        statisticsContext->ProcessItem->IoReadDelta.Delta +
                                        statisticsContext->ProcessItem->IoWriteDelta.Delta +
                                        statisticsContext->ProcessItem->IoOtherDelta.Delta
                                        );
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_IOAVERAGE:
                                {
                                    PH_FORMAT format[2];
                                    SIZE_T returnLength;
                                    FLOAT cpuSumValue = 0;
                                    FLOAT cpuAverageValue = 0;
                                    WCHAR buffer[PH_INT64_STR_LEN_1];

                                    for (ULONG i = 0; i < statisticsContext->ProcessItem->IoReadHistory.Count; i++)
                                    {
                                        cpuSumValue += (FLOAT)PhGetItemCircularBuffer_ULONG64(&statisticsContext->ProcessItem->IoReadHistory, i) +
                                            (FLOAT)PhGetItemCircularBuffer_ULONG64(&statisticsContext->ProcessItem->IoWriteHistory, i) +
                                            (FLOAT)PhGetItemCircularBuffer_ULONG64(&statisticsContext->ProcessItem->IoOtherHistory, i);
                                    }

                                    if (statisticsContext->ProcessItem->IoReadHistory.Count)
                                    {
                                        cpuAverageValue = (FLOAT)cpuSumValue / statisticsContext->ProcessItem->IoReadHistory.Count;
                                    }

                                    PhInitFormatSize(&format[0], (ULONG64)cpuAverageValue);
                                    PhInitFormatS(&format[1], L"/s");

                                    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), &returnLength))
                                    {
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                                    }
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_IOPRIORITY:
                                {
                                    if (statisticsContext->IoPriority != LONG_MAX && statisticsContext->IoPriority >= IoPriorityVeryLow && statisticsContext->IoPriority <= MaxIoPriorityTypes)
                                    {
                                        const PH_STRINGREF ioPriority = PhIoPriorityHintNames[statisticsContext->IoPriority];

                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, ioPriority.Buffer, _TRUNCATE);
                                    }
                                    else
                                    {
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                    }
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_HANDLES:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->ProcessItem->NumberOfHandles);
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
                            case PH_PROCESS_STATISTICS_INDEX_PEAKGDIHANDLES:
                                {
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhGetStringOrDefault(statisticsContext->PeakGdiHandles, L"N/A"), _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_USERHANDLES:
                                {
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhGetStringOrDefault(statisticsContext->UserHandles, L"N/A"), _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKUSERHANDLES:
                                {
                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, PhGetStringOrDefault(statisticsContext->PeakUserHandles, L"N/A"), _TRUNCATE);
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
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->HangCount);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_GHOSTCOUNT:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->GhostCount);
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
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->NetworkTxRxBytes);
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
                            case PH_PROCESS_STATISTICS_INDEX_MOUSE:
                                {
                                    PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->MouseDelta.Value);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_KEYBOARD:
                                {
                                    PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->KeyboardDelta.Value);
                                }
                                break;
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

                    if (FlagOn(dispInfo->item.mask, LVIF_TEXT))
                    {
                        if (dispInfo->item.iSubItem == 2)
                        {
                            switch (PtrToUlong((PVOID)dispInfo->item.lParam))
                            {
                            case PH_PROCESS_STATISTICS_INDEX_CPU:
                                PH_PROCESS_STATISTICS_FORMAT_F(dispInfo, statisticsContext->CpuUsageMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CPUUSER:
                                PH_PROCESS_STATISTICS_FORMAT_F(dispInfo, statisticsContext->CpuUsageUserMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CPUKERNEL:
                                PH_PROCESS_STATISTICS_FORMAT_F(dispInfo, statisticsContext->CpuUsageKernelMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CPUAVERAGE:
                                PH_PROCESS_STATISTICS_FORMAT_F(dispInfo, statisticsContext->CpuUsageAverageMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CPURELATIVE:
                                PH_PROCESS_STATISTICS_FORMAT_F(dispInfo, statisticsContext->CpuUsageRelativeMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CYCLES:
                                {
                                    PH_FORMAT format[1];
                                    WCHAR buffer[PH_INT64_STR_LEN_1];

                                    if (statisticsContext->GotCycles)
                                    {
                                        PhInitFormatI64UGroupDigits(&format[0], statisticsContext->CycleTimeMin);
                                    }
                                    //else if (statisticsContext->CycleTimeDeltaMin)
                                    //{
                                    //    PhInitFormatI64UGroupDigits(&format[0], statisticsContext->CycleTimeDeltaMin);
                                    //}
                                    else
                                    {
                                        PhInitFormatS(&format[0], L"N/A");
                                    }

                                    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                                    {
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                                    }
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CYCLESDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->CycleTimeDeltaMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHES:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->ContextSwitchesMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHESDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->ContextSwitchesDeltaMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_KERNELTIME:
                                PH_PROCESS_STATISTICS_FORMAT_TIME(dispInfo, statisticsContext->KernelTimeMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_USERTIME:
                                PH_PROCESS_STATISTICS_FORMAT_TIME(dispInfo, statisticsContext->UserTimeMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_TOTALTIME:
                                PH_PROCESS_STATISTICS_FORMAT_TIME(dispInfo, statisticsContext->KernelTimeMin + statisticsContext->UserTimeMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_KERNELDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->KernelTimeDeltaMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_USERDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->UserTimeDeltaMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_TOTALDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->KernelTimeDeltaMin + statisticsContext->UserTimeDeltaMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIORITY:
                                {
                                    WCHAR priority[PH_INT32_STR_LEN_1] = L"";

                                    PhPrintInt32(priority, statisticsContext->BasePriorityMin);

                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, priority, _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTES:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->PagefileUsageMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTESDELTA:
                                PhpUpdateProcessStatisticDelta(dispInfo, statisticsContext->PagefileDeltaMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATEBYTES:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->PeakPagefileUsageMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_VIRTUALSIZE:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->VirtualSizeMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKVIRTUALSIZE:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->PeakVirtualSizeMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PAGEFAULTS:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->PageFaultCountMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PAGEFAULTSDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->PageFaultsDeltaMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_HARDFAULTS:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->HardFaultCountMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_HARDFAULTSDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->HardFaultsDeltaMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WORKINGSET:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->WorkingSetSizeMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKWORKINGSET:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->PeakWorkingSetSizeMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIVATEWS:
                                {
                                    if (statisticsContext->NumberOfPrivatePagesMin)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->NumberOfPrivatePagesMin);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_SHAREABLEWS:
                                {
                                    if (statisticsContext->NumberOfShareablePagesMin)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->NumberOfShareablePagesMin);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_SHAREDWS:
                                {
                                    if (statisticsContext->NumberOfSharedPagesMin)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->NumberOfSharedPagesMin);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PAGEDPOOL:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->QuotaPagedPoolUsageMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKPAGEDPOOL:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->QuotaPeakPagedPoolUsageMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_NONPAGED:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->QuotaNonPagedPoolUsageMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKNONPAGED:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->QuotaPeakNonPagedPoolUsageMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_SHAREDCOMMIT:
                                {
                                    if (statisticsContext->SharedCommitUsageMin)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->SharedCommitUsageMin);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIVATECOMMIT:
                                {
                                    if (statisticsContext->PrivateCommitUsageMin)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->PrivateCommitUsageMin);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATECOMMIT:
                                {
                                    if (statisticsContext->PeakPrivateCommitUsageMin)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->PeakPrivateCommitUsageMin);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PAGEPRIORITY:
                                {

                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_READS:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->ReadOperationCountMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_READSDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->IoReadCountDeltaMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_READBYTES:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->ReadTransferCountMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_READBYTESDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->IoReadDeltaMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WRITES:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->WriteOperationCountMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WRITESDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->IoWriteCountDeltaMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WRITEBYTES:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->WriteTransferCountMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WRITEBYTESDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->IoWriteDeltaMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_OTHER:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->OtherOperationCountMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_OTHERDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->IoOtherCountDeltaMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_OTHERBYTES:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->OtherTransferCountMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_OTHERBYTESDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->IoOtherDeltaMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_IOTOTAL:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->IoTotalDeltaMin);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_IOTOTALDELTA:
                                {
                                    PH_PROCESS_STATISTICS_FORMAT_SIZE(
                                        dispInfo,
                                        statisticsContext->ProcessItem->IoReadDelta.Delta +
                                        statisticsContext->ProcessItem->IoWriteDelta.Delta +
                                        statisticsContext->ProcessItem->IoOtherDelta.Delta
                                    );
                                }
                                break;
                            }
                        }
                        else if (dispInfo->item.iSubItem == 3)
                        {
                            switch (PtrToUlong((PVOID)dispInfo->item.lParam))
                            {
                            case PH_PROCESS_STATISTICS_INDEX_CPU:
                                PH_PROCESS_STATISTICS_FORMAT_F(dispInfo, statisticsContext->CpuUsageMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CPUUSER:
                                PH_PROCESS_STATISTICS_FORMAT_F(dispInfo, statisticsContext->CpuUsageUserMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CPUKERNEL:
                                PH_PROCESS_STATISTICS_FORMAT_F(dispInfo, statisticsContext->CpuUsageKernelMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CPUAVERAGE:
                                PH_PROCESS_STATISTICS_FORMAT_F(dispInfo, statisticsContext->CpuUsageAverageMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CPURELATIVE:
                                PH_PROCESS_STATISTICS_FORMAT_F(dispInfo, statisticsContext->CpuUsageRelativeMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CYCLES:
                                {
                                    PH_FORMAT format[1];
                                    WCHAR buffer[PH_INT64_STR_LEN_1];

                                    if (statisticsContext->GotCycles)
                                    {
                                        PhInitFormatI64UGroupDigits(&format[0], statisticsContext->CycleTimeMax);
                                    }
                                    //else if (statisticsContext->CycleTimeDeltaMax)
                                    //{
                                    //    PhInitFormatI64UGroupDigits(&format[0], statisticsContext->CycleTimeDeltaMax);
                                    //}
                                    else
                                    {
                                        PhInitFormatS(&format[0], L"N/A");
                                    }

                                    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                                    {
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                                    }
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CYCLESDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->CycleTimeDeltaMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHES:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->ContextSwitchesMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHESDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->ContextSwitchesDeltaMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_KERNELTIME:
                                PH_PROCESS_STATISTICS_FORMAT_TIME(dispInfo, statisticsContext->KernelTimeMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_USERTIME:
                                PH_PROCESS_STATISTICS_FORMAT_TIME(dispInfo, statisticsContext->UserTimeMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_TOTALTIME:
                                PH_PROCESS_STATISTICS_FORMAT_TIME(dispInfo, statisticsContext->KernelTimeMax + statisticsContext->UserTimeMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_KERNELDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->KernelTimeDeltaMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_USERDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->UserTimeDeltaMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_TOTALDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->KernelTimeDeltaMax + statisticsContext->UserTimeDeltaMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIORITY:
                                {
                                    WCHAR priority[PH_INT32_STR_LEN_1] = L"";

                                    PhPrintInt32(priority, statisticsContext->BasePriorityMax);

                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, priority, _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTES:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->PagefileUsageMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTESDELTA:
                                PhpUpdateProcessStatisticDelta(dispInfo, statisticsContext->PagefileDeltaMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATEBYTES:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->PeakPagefileUsageMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_VIRTUALSIZE:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->VirtualSizeMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKVIRTUALSIZE:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->PeakVirtualSizeMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PAGEFAULTS:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->PageFaultCountMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PAGEFAULTSDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->PageFaultsDeltaMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_HARDFAULTS:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->HardFaultCountMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_HARDFAULTSDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->HardFaultsDeltaMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WORKINGSET:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->WorkingSetSizeMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKWORKINGSET:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->PeakWorkingSetSizeMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIVATEWS:
                                {
                                    if (statisticsContext->NumberOfPrivatePagesMax)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->NumberOfPrivatePagesMax);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_SHAREABLEWS:
                                {
                                    if (statisticsContext->NumberOfShareablePagesMax)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->NumberOfShareablePagesMax);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_SHAREDWS:
                                {
                                    if (statisticsContext->NumberOfSharedPagesMax)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->NumberOfSharedPagesMax);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PAGEDPOOL:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->QuotaPagedPoolUsageMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKPAGEDPOOL:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->QuotaPeakPagedPoolUsageMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_NONPAGED:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->QuotaNonPagedPoolUsageMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKNONPAGED:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->QuotaPeakNonPagedPoolUsageMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_SHAREDCOMMIT:
                                {
                                    if (statisticsContext->SharedCommitUsageMax)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->SharedCommitUsageMax);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIVATECOMMIT:
                                {
                                    if (statisticsContext->PrivateCommitUsageMax)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->PrivateCommitUsageMax);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATECOMMIT:
                                {
                                    if (statisticsContext->PeakPrivateCommitUsageMax)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->PeakPrivateCommitUsageMax);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PAGEPRIORITY:
                                {

                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_READS:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->ReadOperationCountMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_READSDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->IoReadCountDeltaMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_READBYTES:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->ReadTransferCountMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_READBYTESDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->IoReadDeltaMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WRITES:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->WriteOperationCountMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WRITESDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->IoWriteCountDeltaMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WRITEBYTES:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->WriteTransferCountMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WRITEBYTESDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->IoWriteDeltaMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_OTHER:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->OtherOperationCountMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_OTHERDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->IoOtherCountDeltaMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_OTHERBYTES:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->OtherTransferCountMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_OTHERBYTESDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->IoOtherDeltaMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_IOTOTAL:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->IoTotalDeltaMax);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_IOTOTALDELTA:
                                break;
                            }
                        }
                        else if (dispInfo->item.iSubItem == 4)
                        {
                            switch (PtrToUlong((PVOID)dispInfo->item.lParam))
                            {
                            case PH_PROCESS_STATISTICS_INDEX_CPU:
                                PH_PROCESS_STATISTICS_FORMAT_F(dispInfo, statisticsContext->CpuUsageDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CPUUSER:
                                PH_PROCESS_STATISTICS_FORMAT_F(dispInfo, statisticsContext->CpuUsageUserDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CPUKERNEL:
                                PH_PROCESS_STATISTICS_FORMAT_F(dispInfo, statisticsContext->CpuUsageKernelDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CPUAVERAGE:
                                PH_PROCESS_STATISTICS_FORMAT_F(dispInfo, statisticsContext->CpuUsageAverageDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CPURELATIVE:
                                PH_PROCESS_STATISTICS_FORMAT_F(dispInfo, statisticsContext->CpuUsageRelativeDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CYCLES:
                                {
                                    PH_FORMAT format[1];
                                    WCHAR buffer[PH_INT64_STR_LEN_1];

                                    if (statisticsContext->GotCycles)
                                    {
                                        PhInitFormatI64UGroupDigits(&format[0], statisticsContext->CycleTimeDiff);
                                    }
                                    //else if (statisticsContext->CycleTimeDeltaDiff)
                                    //{
                                    //    PhInitFormatI64UGroupDigits(&format[0], statisticsContext->CycleTimeDeltaDiff);
                                    //}
                                    else
                                    {
                                        PhInitFormatS(&format[0], L"N/A");
                                    }

                                    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), NULL))
                                    {
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, buffer, _TRUNCATE);
                                    }
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CYCLESDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->CycleTimeDeltaDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHES:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->ContextSwitchesDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHESDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->ContextSwitchesDeltaDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_KERNELTIME:
                                PH_PROCESS_STATISTICS_FORMAT_TIME(dispInfo, statisticsContext->KernelTimeDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_USERTIME:
                                PH_PROCESS_STATISTICS_FORMAT_TIME(dispInfo, statisticsContext->UserTimeDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_TOTALTIME:
                                PH_PROCESS_STATISTICS_FORMAT_TIME(dispInfo, statisticsContext->KernelTimeDiff + statisticsContext->UserTimeDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_KERNELDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->KernelTimeDeltaDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_USERDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->UserTimeDeltaDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_TOTALDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->KernelTimeDeltaDiff + statisticsContext->UserTimeDeltaDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIORITY:
                                {
                                    WCHAR priority[PH_INT32_STR_LEN_1] = L"";

                                    PhPrintInt32(priority, statisticsContext->BasePriorityDiff);

                                    wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, priority, _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTES:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->PagefileUsageDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTESDELTA:
                                PhpUpdateProcessStatisticDelta(dispInfo, statisticsContext->PagefileDeltaDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATEBYTES:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->PeakPagefileUsageDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_VIRTUALSIZE:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->VirtualSizeDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKVIRTUALSIZE:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->PeakVirtualSizeDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PAGEFAULTS:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->PageFaultCountDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PAGEFAULTSDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->PageFaultsDeltaDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_HARDFAULTS:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->HardFaultCountDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_HARDFAULTSDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->HardFaultsDeltaDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WORKINGSET:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->WorkingSetSizeDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKWORKINGSET:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->PeakWorkingSetSizeDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIVATEWS:
                                {
                                    if (statisticsContext->NumberOfPrivatePagesDiff)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->NumberOfPrivatePagesDiff);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_SHAREABLEWS:
                                {
                                    if (statisticsContext->NumberOfShareablePagesDiff)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->NumberOfShareablePagesDiff);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_SHAREDWS:
                                {
                                    if (statisticsContext->NumberOfSharedPagesDiff)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->NumberOfSharedPagesDiff);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PAGEDPOOL:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->QuotaPagedPoolUsageDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKPAGEDPOOL:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->QuotaPeakPagedPoolUsageDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_NONPAGED:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->QuotaNonPagedPoolUsageDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKNONPAGED:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->QuotaPeakNonPagedPoolUsageDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_SHAREDCOMMIT:
                                {
                                    if (statisticsContext->SharedCommitUsageDiff)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->SharedCommitUsageDiff);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PRIVATECOMMIT:
                                {
                                    if (statisticsContext->PrivateCommitUsageDiff)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->PrivateCommitUsageDiff);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATECOMMIT:
                                {
                                    if (statisticsContext->PeakPrivateCommitUsageDiff)
                                        PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->PeakPrivateCommitUsageDiff);
                                    else
                                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, L"N/A", _TRUNCATE);
                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_PAGEPRIORITY:
                                {

                                }
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_READS:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->ReadOperationCountDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_READSDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->IoReadCountDeltaDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_READBYTES:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->ReadTransferCountDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_READBYTESDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->IoReadDeltaDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WRITES:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->WriteOperationCountDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WRITESDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->IoWriteCountDeltaDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WRITEBYTES:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->WriteTransferCountDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_WRITEBYTESDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->IoWriteDeltaDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_OTHER:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->OtherOperationCountDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_OTHERDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->IoOtherCountDeltaDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_OTHERBYTES:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->OtherTransferCountDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_OTHERBYTESDELTA:
                                PH_PROCESS_STATISTICS_FORMAT_I64U(dispInfo, statisticsContext->IoOtherDeltaDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_IOTOTAL:
                                PH_PROCESS_STATISTICS_FORMAT_SIZE(dispInfo, statisticsContext->IoTotalDeltaDiff);
                                break;
                            case PH_PROCESS_STATISTICS_INDEX_IOTOTALDELTA:
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
