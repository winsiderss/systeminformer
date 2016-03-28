/*
 * Process Hacker -
 *   System Information CPU section
 *
 * Copyright (C) 2011-2016 wj32
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
#include <procprv.h>
#include <settings.h>
#include <sysinfo.h>
#include <windowsx.h>
#include <math.h>
#include <sysinfop.h>

static PPH_SYSINFO_SECTION CpuSection;
static HWND CpuDialog;
static PH_LAYOUT_MANAGER CpuLayoutManager;
static RECT CpuGraphMargin;
static HWND CpuGraphHandle;
static PH_GRAPH_STATE CpuGraphState;
static HWND *CpusGraphHandle;
static PPH_GRAPH_STATE CpusGraphState;
static BOOLEAN OneGraphPerCpu;
static HWND CpuPanel;
static ULONG CpuTicked;
static ULONG NumberOfProcessors;
static PSYSTEM_INTERRUPT_INFORMATION InterruptInformation;
static PPROCESSOR_POWER_INFORMATION PowerInformation;
static PSYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION CurrentPerformanceDistribution;
static PSYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION PreviousPerformanceDistribution;
static PH_UINT32_DELTA ContextSwitchesDelta;
static PH_UINT32_DELTA InterruptsDelta;
static PH_UINT64_DELTA DpcsDelta;
static PH_UINT32_DELTA SystemCallsDelta;

BOOLEAN PhSipCpuSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    switch (Message)
    {
    case SysInfoCreate:
        {
            CpuSection = Section;
        }
        return TRUE;
    case SysInfoDestroy:
        {
            if (CpuDialog)
            {
                PhSipUninitializeCpuDialog();
                CpuDialog = NULL;
            }
        }
        return TRUE;
    case SysInfoTick:
        {
            if (CpuDialog)
            {
                PhSipTickCpuDialog();
            }
        }
        return TRUE;
    case SysInfoCreateDialog:
        {
            PPH_SYSINFO_CREATE_DIALOG createDialog = Parameter1;

            createDialog->Instance = PhInstanceHandle;
            createDialog->Template = MAKEINTRESOURCE(IDD_SYSINFO_CPU);
            createDialog->DialogProc = PhSipCpuDialogProc;
        }
        return TRUE;
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = Parameter1;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_USE_LINE_2;
            Section->Parameters->ColorSetupFunction(drawInfo, PhCsColorCpuKernel, PhCsColorCpuUser);
            PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, PhCpuKernelHistory.Count);

            if (!Section->GraphState.Valid)
            {
                PhCopyCircularBuffer_FLOAT(&PhCpuKernelHistory, Section->GraphState.Data1, drawInfo->LineDataCount);
                PhCopyCircularBuffer_FLOAT(&PhCpuUserHistory, Section->GraphState.Data2, drawInfo->LineDataCount);
                Section->GraphState.Valid = TRUE;
            }
        }
        return TRUE;
    case SysInfoGraphGetTooltipText:
        {
            PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT getTooltipText = Parameter1;
            FLOAT cpuKernel;
            FLOAT cpuUser;

            cpuKernel = PhGetItemCircularBuffer_FLOAT(&PhCpuKernelHistory, getTooltipText->Index);
            cpuUser = PhGetItemCircularBuffer_FLOAT(&PhCpuUserHistory, getTooltipText->Index);

            PhMoveReference(&Section->GraphState.TooltipText, PhFormatString(
                L"%.2f%%%s\n%s",
                (cpuKernel + cpuUser) * 100,
                PhGetStringOrEmpty(PhSipGetMaxCpuString(getTooltipText->Index)),
                PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->Buffer
                ));
            getTooltipText->Text = Section->GraphState.TooltipText->sr;
        }
        return TRUE;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = Parameter1;

            drawPanel->Title = PhCreateString(L"CPU");
            drawPanel->SubTitle = PhFormatString(L"%.2f%%", (PhCpuKernelUsage + PhCpuUserUsage) * 100);
        }
        return TRUE;
    }

    return FALSE;
}

VOID PhSipInitializeCpuDialog(
    VOID
    )
{
    ULONG i;

    PhInitializeDelta(&ContextSwitchesDelta);
    PhInitializeDelta(&InterruptsDelta);
    PhInitializeDelta(&DpcsDelta);
    PhInitializeDelta(&SystemCallsDelta);

    NumberOfProcessors = (ULONG)PhSystemBasicInformation.NumberOfProcessors;
    CpusGraphHandle = PhAllocate(sizeof(HWND) * NumberOfProcessors);
    CpusGraphState = PhAllocate(sizeof(PH_GRAPH_STATE) * NumberOfProcessors);
    InterruptInformation = PhAllocate(sizeof(SYSTEM_INTERRUPT_INFORMATION) * NumberOfProcessors);
    PowerInformation = PhAllocate(sizeof(PROCESSOR_POWER_INFORMATION) * NumberOfProcessors);

    PhInitializeGraphState(&CpuGraphState);

    for (i = 0; i < NumberOfProcessors; i++)
        PhInitializeGraphState(&CpusGraphState[i]);

    CpuTicked = 0;

    if (!NT_SUCCESS(NtPowerInformation(
        ProcessorInformation,
        NULL,
        0,
        PowerInformation,
        sizeof(PROCESSOR_POWER_INFORMATION) * NumberOfProcessors
        )))
    {
        memset(PowerInformation, 0, sizeof(PROCESSOR_POWER_INFORMATION) * NumberOfProcessors);
    }

    CurrentPerformanceDistribution = NULL;
    PreviousPerformanceDistribution = NULL;

    if (WindowsVersion >= WINDOWS_7)
        PhSipQueryProcessorPerformanceDistribution(&CurrentPerformanceDistribution);
}

VOID PhSipUninitializeCpuDialog(
    VOID
    )
{
    ULONG i;

    PhDeleteGraphState(&CpuGraphState);

    for (i = 0; i < NumberOfProcessors; i++)
        PhDeleteGraphState(&CpusGraphState[i]);

    PhFree(CpusGraphHandle);
    PhFree(CpusGraphState);
    PhFree(InterruptInformation);
    PhFree(PowerInformation);

    if (CurrentPerformanceDistribution)
        PhFree(CurrentPerformanceDistribution);
    if (PreviousPerformanceDistribution)
        PhFree(PreviousPerformanceDistribution);

    PhSetIntegerSetting(L"SysInfoWindowOneGraphPerCpu", OneGraphPerCpu);
}

VOID PhSipTickCpuDialog(
    VOID
    )
{
    ULONG64 dpcCount;
    ULONG i;

    dpcCount = 0;

    if (NT_SUCCESS(NtQuerySystemInformation(
        SystemInterruptInformation,
        InterruptInformation,
        sizeof(SYSTEM_INTERRUPT_INFORMATION) * NumberOfProcessors,
        NULL
        )))
    {
        for (i = 0; i < NumberOfProcessors; i++)
            dpcCount += InterruptInformation[i].DpcCount;
    }

    PhUpdateDelta(&ContextSwitchesDelta, PhPerfInformation.ContextSwitches);
    PhUpdateDelta(&InterruptsDelta, PhCpuTotals.InterruptCount);
    PhUpdateDelta(&DpcsDelta, dpcCount);
    PhUpdateDelta(&SystemCallsDelta, PhPerfInformation.SystemCalls);

    if (!NT_SUCCESS(NtPowerInformation(
        ProcessorInformation,
        NULL,
        0,
        PowerInformation,
        sizeof(PROCESSOR_POWER_INFORMATION) * NumberOfProcessors
        )))
    {
        memset(PowerInformation, 0, sizeof(PROCESSOR_POWER_INFORMATION) * NumberOfProcessors);
    }

    if (WindowsVersion >= WINDOWS_7)
    {
        if (PreviousPerformanceDistribution)
            PhFree(PreviousPerformanceDistribution);

        PreviousPerformanceDistribution = CurrentPerformanceDistribution;
        CurrentPerformanceDistribution = NULL;
        PhSipQueryProcessorPerformanceDistribution(&CurrentPerformanceDistribution);
    }

    CpuTicked++;

    if (CpuTicked > 2)
        CpuTicked = 2;

    PhSipUpdateCpuGraphs();
    PhSipUpdateCpuPanel();
}

INT_PTR CALLBACK PhSipCpuDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PPH_LAYOUT_ITEM graphItem;
            PPH_LAYOUT_ITEM panelItem;
            WCHAR brandString[49];

            PhSipInitializeCpuDialog();

            CpuDialog = hwndDlg;
            PhInitializeLayoutManager(&CpuLayoutManager, hwndDlg);
            PhAddLayoutItem(&CpuLayoutManager, GetDlgItem(hwndDlg, IDC_CPUNAME), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            graphItem = PhAddLayoutItem(&CpuLayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            CpuGraphMargin = graphItem->Margin;
            panelItem = PhAddLayoutItem(&CpuLayoutManager, GetDlgItem(hwndDlg, IDC_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            SendMessage(GetDlgItem(hwndDlg, IDC_TITLE), WM_SETFONT, (WPARAM)CpuSection->Parameters->LargeFont, FALSE);
            SendMessage(GetDlgItem(hwndDlg, IDC_CPUNAME), WM_SETFONT, (WPARAM)CpuSection->Parameters->MediumFont, FALSE);

            PhSipGetCpuBrandString(brandString);
            SetDlgItemText(hwndDlg, IDC_CPUNAME, brandString);

            CpuPanel = CreateDialog(PhInstanceHandle, MAKEINTRESOURCE(IDD_SYSINFO_CPUPANEL), hwndDlg, PhSipCpuPanelDialogProc);
            ShowWindow(CpuPanel, SW_SHOW);
            PhAddLayoutItemEx(&CpuLayoutManager, CpuPanel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, panelItem->Margin);

            PhSipCreateCpuGraphs();

            if (NumberOfProcessors != 1)
            {
                OneGraphPerCpu = (BOOLEAN)PhGetIntegerSetting(L"SysInfoWindowOneGraphPerCpu");
                Button_SetCheck(GetDlgItem(CpuPanel, IDC_ONEGRAPHPERCPU), OneGraphPerCpu ? BST_CHECKED : BST_UNCHECKED);
                PhSipSetOneGraphPerCpu();
            }
            else
            {
                OneGraphPerCpu = FALSE;
                EnableWindow(GetDlgItem(CpuPanel, IDC_ONEGRAPHPERCPU), FALSE);
                PhSipSetOneGraphPerCpu();
            }

            PhSipUpdateCpuGraphs();
            PhSipUpdateCpuPanel();
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&CpuLayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&CpuLayoutManager);
            PhSipLayoutCpuGraphs();
        }
        break;
    case WM_NOTIFY:
        {
            NMHDR *header = (NMHDR *)lParam;
            ULONG i;

            if (header->hwndFrom == CpuGraphHandle)
            {
                PhSipNotifyCpuGraph(-1, header);
            }
            else
            {
                for (i = 0; i < NumberOfProcessors; i++)
                {
                    if (header->hwndFrom == CpusGraphHandle[i])
                    {
                        PhSipNotifyCpuGraph(i, header);
                        break;
                    }
                }
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PhSipCpuPanelDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            SendMessage(GetDlgItem(hwndDlg, IDC_UTILIZATION), WM_SETFONT, (WPARAM)CpuSection->Parameters->MediumFont, FALSE);
            SendMessage(GetDlgItem(hwndDlg, IDC_SPEED), WM_SETFONT, (WPARAM)CpuSection->Parameters->MediumFont, FALSE);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_ONEGRAPHPERCPU:
                {
                    OneGraphPerCpu = Button_GetCheck(GetDlgItem(hwndDlg, IDC_ONEGRAPHPERCPU)) == BST_CHECKED;
                    PhSipLayoutCpuGraphs();
                    PhSipSetOneGraphPerCpu();
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

VOID PhSipCreateCpuGraphs(
    VOID
    )
{
    ULONG i;

    CpuGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        CpuDialog,
        NULL,
        PhInstanceHandle,
        NULL
        );
    Graph_SetTooltip(CpuGraphHandle, TRUE);

    for (i = 0; i < NumberOfProcessors; i++)
    {
        CpusGraphHandle[i] = CreateWindow(
            PH_GRAPH_CLASSNAME,
            NULL,
            WS_CHILD | WS_BORDER,
            0,
            0,
            3,
            3,
            CpuDialog,
            NULL,
            PhInstanceHandle,
            NULL
            );
        Graph_SetTooltip(CpusGraphHandle[i], TRUE);
    }
}

VOID PhSipLayoutCpuGraphs(
    VOID
    )
{
    RECT clientRect;
    HDWP deferHandle;

    GetClientRect(CpuDialog, &clientRect);
    deferHandle = BeginDeferWindowPos(OneGraphPerCpu ? NumberOfProcessors : 1);

    if (!OneGraphPerCpu)
    {
        deferHandle = DeferWindowPos(
            deferHandle,
            CpuGraphHandle,
            NULL,
            CpuGraphMargin.left,
            CpuGraphMargin.top,
            clientRect.right - CpuGraphMargin.left - CpuGraphMargin.right,
            clientRect.bottom - CpuGraphMargin.top - CpuGraphMargin.bottom,
            SWP_NOACTIVATE | SWP_NOZORDER
            );
    }
    else
    {
        ULONG numberOfRows = 1;
        ULONG numberOfColumns = NumberOfProcessors;

        for (ULONG rows = 2; rows <= NumberOfProcessors / rows; rows++)
        {
            if (NumberOfProcessors % rows != 0)
                continue;

            numberOfRows = rows;
            numberOfColumns = NumberOfProcessors / rows;
        }

        if (numberOfRows == 1)
        {
            numberOfRows = (ULONG)sqrt(NumberOfProcessors);
            numberOfColumns = (NumberOfProcessors + numberOfRows - 1) / numberOfRows;
        }

        ULONG numberOfYPaddings = numberOfRows - 1;
        ULONG numberOfXPaddings = numberOfColumns - 1;

        ULONG cellHeight = (clientRect.bottom - CpuGraphMargin.top - CpuGraphMargin.bottom - CpuSection->Parameters->CpuPadding * numberOfYPaddings) / numberOfRows;
        ULONG y = CpuGraphMargin.top;
        ULONG cellWidth;
        ULONG x;
        ULONG i = 0;

        for (ULONG row = 0; row < numberOfRows; row++)
        {
            // Give the last row the remaining space; the height we calculated might be off by a few
            // pixels due to integer division.
            if (row == numberOfRows - 1)
                cellHeight = clientRect.bottom - CpuGraphMargin.bottom - y;

            cellWidth = (clientRect.right - CpuGraphMargin.left - CpuGraphMargin.right - CpuSection->Parameters->CpuPadding * numberOfXPaddings) / numberOfColumns;
            x = CpuGraphMargin.left;

            for (ULONG column = 0; column < numberOfColumns; column++)
            {
                // Give the last cell the remaining space; the width we calculated might be off by a few
                // pixels due to integer division.
                if (column == numberOfColumns - 1)
                    cellWidth = clientRect.right - CpuGraphMargin.right - x;

                if (i < NumberOfProcessors)
                {
                    deferHandle = DeferWindowPos(
                        deferHandle,
                        CpusGraphHandle[i],
                        NULL,
                        x,
                        y,
                        cellWidth,
                        cellHeight,
                        SWP_NOACTIVATE | SWP_NOZORDER
                        );
                    i++;
                }

                x += cellWidth + CpuSection->Parameters->CpuPadding;
            }

            y += cellHeight + CpuSection->Parameters->CpuPadding;
        }
    }

    EndDeferWindowPos(deferHandle);
}

VOID PhSipSetOneGraphPerCpu(
    VOID
    )
{
    ULONG i;

    ShowWindow(CpuGraphHandle, !OneGraphPerCpu ? SW_SHOW : SW_HIDE);

    for (i = 0; i < NumberOfProcessors; i++)
    {
        ShowWindow(CpusGraphHandle[i], OneGraphPerCpu ? SW_SHOW : SW_HIDE);
    }
}

VOID PhSipNotifyCpuGraph(
    _In_ ULONG Index,
    _In_ NMHDR *Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_USE_LINE_2;
            PhSiSetColorsGraphDrawInfo(drawInfo, PhCsColorCpuKernel, PhCsColorCpuUser);

            if (Index == -1)
            {
                PhGraphStateGetDrawInfo(
                    &CpuGraphState,
                    getDrawInfo,
                    PhCpuKernelHistory.Count
                    );

                if (!CpuGraphState.Valid)
                {
                    PhCopyCircularBuffer_FLOAT(&PhCpuKernelHistory, CpuGraphState.Data1, drawInfo->LineDataCount);
                    PhCopyCircularBuffer_FLOAT(&PhCpuUserHistory, CpuGraphState.Data2, drawInfo->LineDataCount);
                    CpuGraphState.Valid = TRUE;
                }
            }
            else
            {
                PhGraphStateGetDrawInfo(
                    &CpusGraphState[Index],
                    getDrawInfo,
                    PhCpuKernelHistory.Count
                    );

                if (!CpusGraphState[Index].Valid)
                {
                    PhCopyCircularBuffer_FLOAT(&PhCpusKernelHistory[Index], CpusGraphState[Index].Data1, drawInfo->LineDataCount);
                    PhCopyCircularBuffer_FLOAT(&PhCpusUserHistory[Index], CpusGraphState[Index].Data2, drawInfo->LineDataCount);
                    CpusGraphState[Index].Valid = TRUE;
                }
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (Index == -1)
                {
                    if (CpuGraphState.TooltipIndex != getTooltipText->Index)
                    {
                        FLOAT cpuKernel;
                        FLOAT cpuUser;

                        cpuKernel = PhGetItemCircularBuffer_FLOAT(&PhCpuKernelHistory, getTooltipText->Index);
                        cpuUser = PhGetItemCircularBuffer_FLOAT(&PhCpuUserHistory, getTooltipText->Index);

                        PhMoveReference(&CpuGraphState.TooltipText, PhFormatString(
                            L"%.2f%%%s\n%s",
                            (cpuKernel + cpuUser) * 100,
                            PhGetStringOrEmpty(PhSipGetMaxCpuString(getTooltipText->Index)),
                            PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->Buffer
                            ));
                    }

                    getTooltipText->Text = CpuGraphState.TooltipText->sr;
                }
                else
                {
                    if (CpusGraphState[Index].TooltipIndex != getTooltipText->Index)
                    {
                        FLOAT cpuKernel;
                        FLOAT cpuUser;

                        cpuKernel = PhGetItemCircularBuffer_FLOAT(&PhCpusKernelHistory[Index], getTooltipText->Index);
                        cpuUser = PhGetItemCircularBuffer_FLOAT(&PhCpusUserHistory[Index], getTooltipText->Index);

                        PhMoveReference(&CpusGraphState[Index].TooltipText, PhFormatString(
                            L"%.2f%% (K: %.2f%%, U: %.2f%%)%s\n%s",
                            (cpuKernel + cpuUser) * 100,
                            cpuKernel * 100,
                            cpuUser * 100,
                            PhGetStringOrEmpty(PhSipGetMaxCpuString(getTooltipText->Index)),
                            PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->Buffer
                            ));
                    }

                    getTooltipText->Text = CpusGraphState[Index].TooltipText->sr;
                }
            }
        }
        break;
    case GCN_MOUSEEVENT:
        {
            PPH_GRAPH_MOUSEEVENT mouseEvent = (PPH_GRAPH_MOUSEEVENT)Header;
            PPH_PROCESS_RECORD record;

            record = NULL;

            if (mouseEvent->Message == WM_LBUTTONDBLCLK && mouseEvent->Index < mouseEvent->TotalCount)
            {
                record = PhSipReferenceMaxCpuRecord(mouseEvent->Index);
            }

            if (record)
            {
                PhShowProcessRecordDialog(CpuDialog, record);
                PhDereferenceProcessRecord(record);
            }
        }
        break;
    }
}

VOID PhSipUpdateCpuGraphs(
    VOID
    )
{
    ULONG i;

    CpuGraphState.Valid = FALSE;
    CpuGraphState.TooltipIndex = -1;
    Graph_MoveGrid(CpuGraphHandle, 1);
    Graph_Draw(CpuGraphHandle);
    Graph_UpdateTooltip(CpuGraphHandle);
    InvalidateRect(CpuGraphHandle, NULL, FALSE);

    for (i = 0; i < NumberOfProcessors; i++)
    {
        CpusGraphState[i].Valid = FALSE;
        CpusGraphState[i].TooltipIndex = -1;
        Graph_MoveGrid(CpusGraphHandle[i], 1);
        Graph_Draw(CpusGraphHandle[i]);
        Graph_UpdateTooltip(CpusGraphHandle[i]);
        InvalidateRect(CpusGraphHandle[i], NULL, FALSE);
    }
}

VOID PhSipUpdateCpuPanel(
    VOID
    )
{
    HWND hwnd = CpuPanel;
    DOUBLE cpuFraction;
    DOUBLE cpuGhz;
    BOOLEAN distributionSucceeded;
    SYSTEM_TIMEOFDAY_INFORMATION timeOfDayInfo;
    WCHAR uptimeString[PH_TIMESPAN_STR_LEN_1] = L"Unknown";

    SetDlgItemText(hwnd, IDC_UTILIZATION, PhaFormatString(L"%.2f%%", (PhCpuUserUsage + PhCpuKernelUsage) * 100)->Buffer);

    cpuGhz = 0;
    distributionSucceeded = FALSE;

    if (WindowsVersion >= WINDOWS_7 && CurrentPerformanceDistribution && PreviousPerformanceDistribution)
    {
        if (PhSipGetCpuFrequencyFromDistribution(&cpuFraction))
        {
            cpuGhz = (DOUBLE)PowerInformation[0].MaxMhz * cpuFraction / 1000;
            distributionSucceeded = TRUE;
        }
    }

    if (!distributionSucceeded)
        cpuGhz = (DOUBLE)PowerInformation[0].CurrentMhz / 1000;

    SetDlgItemText(hwnd, IDC_SPEED, PhaFormatString(L"%.2f / %.2f GHz", cpuGhz, (DOUBLE)PowerInformation[0].MaxMhz / 1000)->Buffer);

    SetDlgItemText(hwnd, IDC_ZPROCESSES_V, PhaFormatUInt64(PhTotalProcesses, TRUE)->Buffer);
    SetDlgItemText(hwnd, IDC_ZTHREADS_V, PhaFormatUInt64(PhTotalThreads, TRUE)->Buffer);
    SetDlgItemText(hwnd, IDC_ZHANDLES_V, PhaFormatUInt64(PhTotalHandles, TRUE)->Buffer);

    if (NT_SUCCESS(NtQuerySystemInformation(
        SystemTimeOfDayInformation,
        &timeOfDayInfo,
        sizeof(SYSTEM_TIMEOFDAY_INFORMATION),
        NULL
        )))
    {
        PhPrintTimeSpan(uptimeString, timeOfDayInfo.CurrentTime.QuadPart - timeOfDayInfo.BootTime.QuadPart, PH_TIMESPAN_DHMS);
    }

    SetDlgItemText(hwnd, IDC_ZUPTIME_V, uptimeString);

    if (CpuTicked > 1)
        SetDlgItemText(hwnd, IDC_ZCONTEXTSWITCHESDELTA_V, PhaFormatUInt64(ContextSwitchesDelta.Delta, TRUE)->Buffer);
    else
        SetDlgItemText(hwnd, IDC_ZCONTEXTSWITCHESDELTA_V, L"-");

    if (CpuTicked > 1)
        SetDlgItemText(hwnd, IDC_ZINTERRUPTSDELTA_V, PhaFormatUInt64(InterruptsDelta.Delta, TRUE)->Buffer);
    else
        SetDlgItemText(hwnd, IDC_ZINTERRUPTSDELTA_V, L"-");

    if (CpuTicked > 1)
        SetDlgItemText(hwnd, IDC_ZDPCSDELTA_V, PhaFormatUInt64(DpcsDelta.Delta, TRUE)->Buffer);
    else
        SetDlgItemText(hwnd, IDC_ZDPCSDELTA_V, L"-");

    if (CpuTicked > 1)
        SetDlgItemText(hwnd, IDC_ZSYSTEMCALLSDELTA_V, PhaFormatUInt64(SystemCallsDelta.Delta, TRUE)->Buffer);
    else
        SetDlgItemText(hwnd, IDC_ZSYSTEMCALLSDELTA_V, L"-");
}

PPH_PROCESS_RECORD PhSipReferenceMaxCpuRecord(
    _In_ LONG Index
    )
{
    LARGE_INTEGER time;
    LONG maxProcessIdLong;
    HANDLE maxProcessId;

    // Find the process record for the max. CPU process for the particular time.

    maxProcessIdLong = PhGetItemCircularBuffer_ULONG(&PhMaxCpuHistory, Index);

    if (!maxProcessIdLong)
        return NULL;

    // This must be treated as a signed integer to handle Interrupts correctly.
    maxProcessId = LongToHandle(maxProcessIdLong);

    // Note that the time we get has its components beyond seconds cleared.
    // For example:
    // * At 2.5 seconds a process is started.
    // * At 2.75 seconds our process provider is fired, and the process is determined
    //   to have 75% CPU usage, which happens to be the maximum CPU usage.
    // * However the 2.75 seconds is recorded as 2 seconds due to
    //   RtlTimeToSecondsSince1980.
    // * If we call PhFindProcessRecord, it cannot find the process because it was
    //   started at 2.5 seconds, not 2 seconds or older.
    //
    // This mean we must add one second minus one tick (100ns) to the time, giving us
    // 2.9999999 seconds. This will then make sure we find the process.
    PhGetStatisticsTime(NULL, Index, &time);
    time.QuadPart += PH_TICKS_PER_SEC - 1;

    return PhFindProcessRecord(maxProcessId, &time);
}

PPH_STRING PhSipGetMaxCpuString(
    _In_ LONG Index
    )
{
    PPH_PROCESS_RECORD maxProcessRecord;
#ifdef PH_RECORD_MAX_USAGE
    FLOAT maxCpuUsage;
#endif
    PPH_STRING maxUsageString = NULL;

    if (maxProcessRecord = PhSipReferenceMaxCpuRecord(Index))
    {
        // We found the process record, so now we construct the max. usage string.
#ifdef PH_RECORD_MAX_USAGE
        maxCpuUsage = PhGetItemCircularBuffer_FLOAT(&PhMaxCpuUsageHistory, Index);

        // Make sure we don't try to display the PID of DPCs or Interrupts.
        if (!PH_IS_FAKE_PROCESS_ID(maxProcessRecord->ProcessId))
        {
            maxUsageString = PhaFormatString(
                L"\n%s (%u): %.2f%%",
                maxProcessRecord->ProcessName->Buffer,
                HandleToUlong(maxProcessRecord->ProcessId),
                maxCpuUsage * 100
                );
        }
        else
        {
            maxUsageString = PhaFormatString(
                L"\n%s: %.2f%%",
                maxProcessRecord->ProcessName->Buffer,
                maxCpuUsage * 100
                );
        }
#else
        maxUsageString = PhaConcatStrings2(L"\n", maxProcessRecord->ProcessName->Buffer);
#endif

        PhDereferenceProcessRecord(maxProcessRecord);
    }

    return maxUsageString;
}

VOID PhSipGetCpuBrandString(
    _Out_writes_(49) PWSTR BrandString
    )
{
    ULONG brandString[4 * 3];

    __cpuid(&brandString[0], 0x80000002);
    __cpuid(&brandString[4], 0x80000003);
    __cpuid(&brandString[8], 0x80000004);

    PhZeroExtendToUtf16Buffer((PSTR)brandString, 48, BrandString);
    BrandString[48] = 0;
}

BOOLEAN PhSipGetCpuFrequencyFromDistribution(
    _Out_ DOUBLE *Fraction
    )
{
    ULONG stateSize;
    PVOID differences;
    PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION stateDistribution;
    PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION stateDifference;
    PSYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8 hitcountOld;
    ULONG i;
    ULONG j;
    DOUBLE count;
    DOUBLE total;

    // Calculate the differences from the last performance distribution.

    if (CurrentPerformanceDistribution->ProcessorCount != NumberOfProcessors || PreviousPerformanceDistribution->ProcessorCount != NumberOfProcessors)
        return FALSE;

    stateSize = FIELD_OFFSET(SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION, States) + sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT) * 2;
    differences = PhAllocate(stateSize * NumberOfProcessors);

    for (i = 0; i < NumberOfProcessors; i++)
    {
        stateDistribution = (PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION)((PCHAR)CurrentPerformanceDistribution + CurrentPerformanceDistribution->Offsets[i]);
        stateDifference = (PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION)((PCHAR)differences + stateSize * i);

        if (stateDistribution->StateCount != 2)
        {
            PhFree(differences);
            return FALSE;
        }

        for (j = 0; j < stateDistribution->StateCount; j++)
        {
            if (WindowsVersion >= WINDOWS_8_1)
            {
                stateDifference->States[j] = stateDistribution->States[j];
            }
            else
            {
                hitcountOld = (PSYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8)((PCHAR)stateDistribution->States + sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8) * j);
                stateDifference->States[j].Hits.QuadPart = hitcountOld->Hits;
                stateDifference->States[j].PercentFrequency = hitcountOld->PercentFrequency;
            }
        }
    }

    for (i = 0; i < NumberOfProcessors; i++)
    {
        stateDistribution = (PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION)((PCHAR)PreviousPerformanceDistribution + PreviousPerformanceDistribution->Offsets[i]);
        stateDifference = (PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION)((PCHAR)differences + stateSize * i);

        if (stateDistribution->StateCount != 2)
        {
            PhFree(differences);
            return FALSE;
        }

        for (j = 0; j < stateDistribution->StateCount; j++)
        {
            if (WindowsVersion >= WINDOWS_8_1)
            {
                stateDifference->States[j].Hits.QuadPart -= stateDistribution->States[j].Hits.QuadPart;
            }
            else
            {
                hitcountOld = (PSYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8)((PCHAR)stateDistribution->States + sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8) * j);
                stateDifference->States[j].Hits.QuadPart -= hitcountOld->Hits;
            }
        }
    }

    // Calculate the frequency.

    count = 0;
    total = 0;

    for (i = 0; i < NumberOfProcessors; i++)
    {
        stateDifference = (PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION)((PCHAR)differences + stateSize * i);

        for (j = 0; j < 2; j++)
        {
            count += (ULONGLONG)stateDifference->States[j].Hits.QuadPart;
            total += (ULONGLONG)stateDifference->States[j].Hits.QuadPart * stateDifference->States[j].PercentFrequency;
        }
    }

    PhFree(differences);

    if (count == 0)
        return FALSE;

    total /= count;
    total /= 100;
    *Fraction = total;

    return TRUE;
}

NTSTATUS PhSipQueryProcessorPerformanceDistribution(
    _Out_ PVOID *Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG attempts;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    status = NtQuerySystemInformation(
        SystemProcessorPerformanceDistribution,
        buffer,
        bufferSize,
        &bufferSize
        );
    attempts = 0;

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 8)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = NtQuerySystemInformation(
            SystemProcessorPerformanceDistribution,
            buffer,
            bufferSize,
            &bufferSize
            );
        attempts++;
    }

    if (NT_SUCCESS(status))
        *Buffer = buffer;
    else
        PhFree(buffer);

    return status;
}
