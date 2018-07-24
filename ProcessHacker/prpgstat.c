/*
 * Process Hacker -
 *   Process properties: Statistics page
 *
 * Copyright (C) 2009-2016 wj32
 * Copyright (C) 2017-2018 dmex
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
#include <phplug.h>
#include <procprp.h>
#include <procprpp.h>
#include <procprv.h>
#include <uxtheme.h>

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
    PH_PROCESS_STATISTICS_INDEX_KERNELTIME,
    PH_PROCESS_STATISTICS_INDEX_USERTIME,
    PH_PROCESS_STATISTICS_INDEX_TOTALTIME,

    PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTES,
    PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATEBYTES,
    PH_PROCESS_STATISTICS_INDEX_VIRTUALSIZE,
    PH_PROCESS_STATISTICS_INDEX_PEAKVIRTUALSIZE,
    PH_PROCESS_STATISTICS_INDEX_PAGEFAULTS,
    PH_PROCESS_STATISTICS_INDEX_WORKINGSET,
    PH_PROCESS_STATISTICS_INDEX_PEAKWORKINGSET,
    PH_PROCESS_STATISTICS_INDEX_PRIVATEWS,
    PH_PROCESS_STATISTICS_INDEX_SHAREABLEWS,
    PH_PROCESS_STATISTICS_INDEX_SHAREDWS,
    PH_PROCESS_STATISTICS_INDEX_PAGEPRIORITY,

    PH_PROCESS_STATISTICS_INDEX_READS,
    PH_PROCESS_STATISTICS_INDEX_READBYTES,
    PH_PROCESS_STATISTICS_INDEX_WRITES,
    PH_PROCESS_STATISTICS_INDEX_WRITEBYTES,
    PH_PROCESS_STATISTICS_INDEX_OTHER,
    PH_PROCESS_STATISTICS_INDEX_OTHERBYTES,
    PH_PROCESS_STATISTICS_INDEX_IOPRIORITY,

    PH_PROCESS_STATISTICS_INDEX_HANDLES,
    PH_PROCESS_STATISTICS_INDEX_PEAKHANDLES,
    PH_PROCESS_STATISTICS_INDEX_GDIHANDLES,
    PH_PROCESS_STATISTICS_INDEX_USERHANDLES,

    PH_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHES,
    PH_PROCESS_STATISTICS_INDEX_DISKREAD,
    PH_PROCESS_STATISTICS_INDEX_DISKWRITE,
    PH_PROCESS_STATISTICS_INDEX_NETWORKTXRXBYTES,
    PH_PROCESS_STATISTICS_INDEX_MBBTXRXBYTES
} PH_PROCESS_STATISTICS_INDEX;

VOID PhpUpdateStatisticsAddListViewGroups(
    _In_ HWND ListViewHandle
    )
{
    ListView_EnableGroupView(ListViewHandle, TRUE);

    PhAddListViewGroup(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, L"CPU");
    PhAddListViewGroup(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, L"Memory");
    PhAddListViewGroup(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, L"I/O");
    PhAddListViewGroup(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_OTHER, L"Other");
    PhAddListViewGroup(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_EXTENSION, L"Extension");

    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_PRIORITY, L"Priority", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_CYCLES, L"Cycles", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_KERNELTIME, L"Kernel time", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_USERTIME, L"User time", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_CPU, PH_PROCESS_STATISTICS_INDEX_TOTALTIME, L"Total time", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTES, L"Private bytes", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATEBYTES, L"Peak private bytes", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_VIRTUALSIZE, L"Virtual size", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PEAKVIRTUALSIZE, L"Peak virtual size", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PAGEFAULTS, L"Page faults", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_WORKINGSET, L"Working set", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PEAKWORKINGSET, L"Peak working set", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PRIVATEWS, L"Private WS", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_SHAREABLEWS, L"Shareable WS", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_SHAREDWS, L"Shared WS", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_MEMORY, PH_PROCESS_STATISTICS_INDEX_PAGEPRIORITY, L"Page priority", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_READS, L"Reads", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_READBYTES, L"Read bytes", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_WRITES, L"Writes", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_WRITEBYTES, L"Write bytes", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_OTHER, L"Other", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_OTHERBYTES, L"Other bytes", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_IO, PH_PROCESS_STATISTICS_INDEX_IOPRIORITY, L"I/O priority", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_HANDLES, L"Handles", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_PEAKHANDLES, L"Peak handles", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_GDIHANDLES, L"GDI handles", NULL);
    PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_OTHER, PH_PROCESS_STATISTICS_INDEX_USERHANDLES, L"USER handles", NULL);

    if (WindowsVersion >= WINDOWS_10_RS3 && !PhIsExecutingInWow64())
    {
        PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_EXTENSION, PH_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHES, L"ContextSwitches", NULL);
        PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_EXTENSION, PH_PROCESS_STATISTICS_INDEX_DISKREAD, L"BytesRead", NULL);
        PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_EXTENSION, PH_PROCESS_STATISTICS_INDEX_DISKWRITE, L"BytesWritten", NULL);
        PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_EXTENSION, PH_PROCESS_STATISTICS_INDEX_NETWORKTXRXBYTES, L"NetworkTxRxBytes", NULL);
        PhAddListViewGroupItem(ListViewHandle, PH_PROCESS_STATISTICS_CATEGORY_EXTENSION, PH_PROCESS_STATISTICS_INDEX_MBBTXRXBYTES, L"MBBTxRxBytes", NULL);
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
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_PRIVATEBYTES, 1, PhaFormatSize(ProcessItem->VmCounters.PagefileUsage, -1)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_PEAKPRIVATEBYTES, 1, PhaFormatSize(ProcessItem->VmCounters.PeakPagefileUsage, -1)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_VIRTUALSIZE, 1, PhaFormatSize(ProcessItem->VmCounters.VirtualSize, -1)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_PEAKVIRTUALSIZE, 1, PhaFormatSize(ProcessItem->VmCounters.PeakVirtualSize, -1)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_PAGEFAULTS, 1, PhaFormatUInt64(ProcessItem->VmCounters.PageFaultCount, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_WORKINGSET, 1, PhaFormatSize(ProcessItem->VmCounters.WorkingSetSize, -1)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_PEAKWORKINGSET, 1, PhaFormatSize(ProcessItem->VmCounters.PeakWorkingSetSize, -1)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_READS, 1, PhaFormatUInt64(ProcessItem->IoCounters.ReadOperationCount, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_READBYTES, 1, PhaFormatSize(ProcessItem->IoCounters.ReadTransferCount, -1)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_WRITES, 1, PhaFormatUInt64(ProcessItem->IoCounters.WriteOperationCount, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_WRITEBYTES, 1, PhaFormatSize(ProcessItem->IoCounters.WriteTransferCount, -1)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_OTHER, 1, PhaFormatUInt64(ProcessItem->IoCounters.OtherOperationCount, TRUE)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_OTHERBYTES, 1, PhaFormatSize(ProcessItem->IoCounters.OtherTransferCount, -1)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_HANDLES, 1, PhaFormatUInt64(ProcessItem->NumberOfHandles, TRUE)->Buffer);

    // Optional information
    if (!PH_IS_FAKE_PROCESS_ID(ProcessItem->ProcessId))
    {
        PPH_STRING peakHandles = NULL;
        PPH_STRING gdiHandles = NULL;
        PPH_STRING userHandles = NULL;
        PPH_STRING cycles = NULL;
        ULONG pagePriority = -1;
        IO_PRIORITY_HINT ioPriority = -1;
        PPH_STRING privateWs = NULL;
        PPH_STRING shareableWs = NULL;
        PPH_STRING sharedWs = NULL;
        BOOLEAN gotCycles = FALSE;
        BOOLEAN gotWsCounters = FALSE;

        if (ProcessItem->QueryHandle)
        {
            ULONG64 cycleTime;
            PROCESS_HANDLE_INFORMATION handleInfo;

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
        }

        if (Context->ProcessHandle)
        {
            PH_PROCESS_WS_COUNTERS wsCounters;

            if (NT_SUCCESS(PhGetProcessWsCounters(Context->ProcessHandle, &wsCounters)))
            {
                privateWs = PhaFormatSize((ULONG64)wsCounters.NumberOfPrivatePages * PAGE_SIZE, -1);
                shareableWs = PhaFormatSize((ULONG64)wsCounters.NumberOfShareablePages * PAGE_SIZE, -1);
                sharedWs = PhaFormatSize((ULONG64)wsCounters.NumberOfSharedPages * PAGE_SIZE, -1);
                gotWsCounters = TRUE;
            }
        }

        if (!gotCycles)
            cycles = PhaFormatUInt64(ProcessItem->CycleTimeDelta.Value, TRUE);
        if (!gotWsCounters)
            privateWs = PhaFormatSize(ProcessItem->WorkingSetPrivateSize, -1);

        PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_CYCLES, 1, PhGetStringOrEmpty(cycles));

        if (pagePriority != -1 && pagePriority <= MEMORY_PRIORITY_NORMAL)
            PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_PAGEPRIORITY, 1, PhPagePriorityNames[pagePriority]);
        else
            PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_PAGEPRIORITY, 1, L"");

        if (ioPriority != -1 && ioPriority < MaxIoPriorityTypes)
            PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_IOPRIORITY, 1, PhIoPriorityHintNames[ioPriority]);
        else
            PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_IOPRIORITY, 1, L"");

        PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_PRIVATEWS, 1, PhGetStringOrEmpty(privateWs));
        PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_SHAREABLEWS, 1, PhGetStringOrEmpty(shareableWs));
        PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_SHAREDWS, 1, PhGetStringOrEmpty(sharedWs));

        PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_PEAKHANDLES, 1, PhGetStringOrEmpty(peakHandles));
        PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_GDIHANDLES, 1, PhGetStringOrEmpty(gdiHandles));
        PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_USERHANDLES, 1, PhGetStringOrEmpty(userHandles));
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
                PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHES, 1, PhaFormatUInt64(processExtension->ContextSwitches, TRUE)->Buffer);
                PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_DISKREAD, 1, PhaFormatSize(processExtension->DiskCounters.BytesRead, -1)->Buffer);
                PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_DISKWRITE, 1, PhaFormatSize(processExtension->DiskCounters.BytesWritten, -1)->Buffer);
                PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_NETWORKTXRXBYTES, 1, PhaFormatSize(processExtension->EnergyValues.NetworkTxRxBytes, -1)->Buffer);
                PhSetListViewSubItem(Context->ListViewHandle, PH_PROCESS_STATISTICS_INDEX_MBBTXRXBYTES, 1, PhaFormatSize(processExtension->EnergyValues.MBBTxRxBytes, -1)->Buffer);
            }
        
            PhFree(processes);
        }
    }
}

static VOID NTAPI StatisticsUpdateHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_STATISTICS_CONTEXT statisticsContext = (PPH_STATISTICS_CONTEXT)Context;

    if (statisticsContext->Enabled)
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

    if (PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext, &processItem))
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
            statisticsContext = propPageContext->Context = PhAllocate(sizeof(PH_STATISTICS_CONTEXT));

            statisticsContext->WindowHandle = hwndDlg;
            statisticsContext->ListViewHandle = GetDlgItem(hwndDlg, IDC_STATISTICS_LIST);
            statisticsContext->Enabled = TRUE;
            statisticsContext->ProcessHandle = NULL;

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
            PhRegisterWindowCallback(statisticsContext->ListViewHandle, PH_PLUGIN_WINDOW_EVENT_TYPE_FONT, NULL);

            PhpUpdateStatisticsAddListViewGroups(statisticsContext->ListViewHandle);

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                StatisticsUpdateHandler,
                statisticsContext,
                &statisticsContext->ProcessesUpdatedRegistration
                );

            PhpUpdateProcessStatistics(processItem, statisticsContext);

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterWindowCallback(statisticsContext->ListViewHandle);

            PhUnregisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                &statisticsContext->ProcessesUpdatedRegistration
                );

            if (statisticsContext->ProcessHandle)
                NtClose(statisticsContext->ProcessHandle);

            PhpPropPageDlgProcDestroy(hwndDlg);
            PhFree(statisticsContext);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PhAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PhAddPropPageLayoutItem(hwndDlg, statisticsContext->ListViewHandle, dialogItem, PH_ANCHOR_ALL);

                PhDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }

            ExtendedListView_SetColumnWidth(statisticsContext->ListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

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
    }

    return FALSE;
}
