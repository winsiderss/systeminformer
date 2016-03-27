/*
 * Process Hacker Extra Plugins -
 *   Nvidia GPU Plugin
 *
 * Copyright (C) 2015 dmex
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

#include "devices.h"

#ifdef _NV_GPU_BUILD

#define ET_GPU_PADDING 3
static RECT NormalGraphTextMargin = { 5, 5, 5, 5 };
static RECT NormalGraphTextPadding = { 3, 3, 3, 3 };

static PPH_STRING GpuName;
static HWND WindowHandle;
static HWND DetailsHandle;
static PPH_SYSINFO_SECTION Section;
static PH_LAYOUT_MANAGER LayoutManager;

static RECT GpuGraphMargin;
static HWND GpuPanel;
static HWND GpuLabelHandle;
static HWND MemLabelHandle;
static HWND SharedLabelHandle;
static HWND BusLabelHandle;
static HWND GpuGraphHandle;
static HWND MemGraphHandle;
static HWND SharedGraphHandle;
static HWND BusGraphHandle;

static PH_GRAPH_STATE GpuGraphState;
static PH_GRAPH_STATE MemGraphState;
static PH_GRAPH_STATE SharedGraphState;
static PH_GRAPH_STATE BusGraphState;

PH_CIRCULAR_BUFFER_FLOAT GpuUtilizationHistory;
PH_CIRCULAR_BUFFER_ULONG GpuMemoryHistory;
PH_CIRCULAR_BUFFER_FLOAT GpuBoardHistory;
PH_CIRCULAR_BUFFER_FLOAT GpuBusHistory;

INT_PTR CALLBACK NvGpuPanelDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    return FALSE;
}

VOID NvGpuCreateGraphs(
    VOID
    )
{
    GpuGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        WindowHandle,
        NULL,
        NULL,
        NULL
        );
    Graph_SetTooltip(GpuGraphHandle, TRUE);

    MemGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        WindowHandle,
        NULL,
        NULL,
        NULL
        );
    Graph_SetTooltip(MemGraphHandle, TRUE);

    SharedGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        WindowHandle,
        NULL,
        NULL,
        NULL
        );
    Graph_SetTooltip(SharedGraphHandle, TRUE);

    BusGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        WindowHandle,
        NULL,
        NULL,
        NULL
        );
    Graph_SetTooltip(BusGraphHandle, TRUE);
}

VOID NvGpuLayoutGraphs(
    VOID
    )
{
    RECT clientRect;
    RECT labelRect;
    ULONG graphWidth;
    ULONG graphHeight;
    HDWP deferHandle;
    ULONG y;

    PhLayoutManagerLayout(&LayoutManager);

    GetClientRect(WindowHandle, &clientRect);
    GetClientRect(GpuLabelHandle, &labelRect);

    graphWidth = clientRect.right - GpuGraphMargin.left - GpuGraphMargin.right;
    graphHeight = (clientRect.bottom - GpuGraphMargin.top - GpuGraphMargin.bottom - labelRect.bottom * 4 - ET_GPU_PADDING * 5) / 4;

    deferHandle = BeginDeferWindowPos(8);
    y = GpuGraphMargin.top;

    deferHandle = DeferWindowPos(
        deferHandle,
        GpuLabelHandle,
        NULL,
        GpuGraphMargin.left,
        y,
        0,
        0,
        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += labelRect.bottom + ET_GPU_PADDING;

    deferHandle = DeferWindowPos(
        deferHandle,
        GpuGraphHandle,
        NULL,
        GpuGraphMargin.left,
        y,
        graphWidth,
        graphHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += graphHeight + ET_GPU_PADDING;

    deferHandle = DeferWindowPos(
        deferHandle,
        MemLabelHandle,
        NULL,
        GpuGraphMargin.left,
        y,
        0,
        0,
        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += labelRect.bottom + ET_GPU_PADDING;

    deferHandle = DeferWindowPos(
        deferHandle,
        MemGraphHandle,
        NULL,
        GpuGraphMargin.left,
        y,
        graphWidth,
        graphHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += graphHeight + ET_GPU_PADDING;

    deferHandle = DeferWindowPos(
        deferHandle,
        SharedLabelHandle,
        NULL,
        GpuGraphMargin.left,
        y,
        0,
        0,
        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += labelRect.bottom + ET_GPU_PADDING;

    deferHandle = DeferWindowPos(
        deferHandle,
        SharedGraphHandle,
        NULL,
        GpuGraphMargin.left,
        y,
        graphWidth,
        graphHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += graphHeight + ET_GPU_PADDING;

    deferHandle = DeferWindowPos(
        deferHandle,
        BusLabelHandle,
        NULL,
        GpuGraphMargin.left,
        y,
        0,
        0,
        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += labelRect.bottom + ET_GPU_PADDING;

    deferHandle = DeferWindowPos(
        deferHandle,
        BusGraphHandle,
        NULL,
        GpuGraphMargin.left,
        y,
        graphWidth,
        clientRect.bottom - GpuGraphMargin.bottom - y,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    EndDeferWindowPos(deferHandle);
}

VOID NvGpuNotifyUsageGraph(
    _In_ NMHDR *Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y;
            Section->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), 0);
            PhGraphStateGetDrawInfo(&GpuGraphState, getDrawInfo, GpuUtilizationHistory.Count);

            if (PhGetIntegerSetting(L"GraphShowText"))
            {
                HDC hdc = Graph_GetBufferedContext(GpuGraphHandle);

                PhMoveReference(&GpuGraphState.Text,
                    PhFormatString(L"%.0f%%", GpuCurrentGpuUsage * 100)
                    );

                SelectObject(hdc, PhApplicationFont);
                PhSetGraphText(hdc, drawInfo, &GpuGraphState.Text->sr,
                    &NormalGraphTextMargin, &NormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
            }
            else
            {
                drawInfo->Text.Buffer = NULL;
            }

            if (!GpuGraphState.Valid)
            {
                PhCopyCircularBuffer_FLOAT(&GpuUtilizationHistory, GpuGraphState.Data1, drawInfo->LineDataCount);
                GpuGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (GpuGraphState.TooltipIndex != getTooltipText->Index)
                {
                    FLOAT gpuUsageValue;

                    gpuUsageValue = PhGetItemCircularBuffer_FLOAT(&GpuUtilizationHistory, getTooltipText->Index);

                    PhMoveReference(&GpuGraphState.TooltipText, PhFormatString(
                        L"%.0f%%\n%s",
                        gpuUsageValue * 100,
                        ((PPH_STRING)PhAutoDereferenceObject(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                        ));
                }

                getTooltipText->Text = GpuGraphState.TooltipText->sr;
            }
        }
        break;
    }
}

VOID NvGpuNotifyMemoryGraph(
    _In_ NMHDR *Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y;
            Section->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorPhysical"), 0);
            PhGraphStateGetDrawInfo(&MemGraphState, getDrawInfo, GpuMemoryHistory.Count);

            if (PhGetIntegerSetting(L"GraphShowText"))
            {
                HDC hdc = Graph_GetBufferedContext(MemGraphHandle);

                PhMoveReference(&MemGraphState.Text, PhFormatString(
                    L"%s / %s (%.2f%%)", 
                    PhaFormatSize(UInt32x32To64(GpuCurrentMemUsage, 1024), -1)->Buffer,
                    PhaFormatSize(UInt32x32To64(GpuMemoryLimit, 1024), -1)->Buffer,
                    (FLOAT)GpuCurrentMemUsage / GpuMemoryLimit * 100
                    ));

                SelectObject(hdc, PhApplicationFont);
                PhSetGraphText(hdc, drawInfo, &MemGraphState.Text->sr,
                    &NormalGraphTextMargin, &NormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
            }
            else
            {
                drawInfo->Text.Buffer = NULL;
            }

            if (!MemGraphState.Valid)
            {
                for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                {
                    MemGraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&GpuMemoryHistory, i);
                }

                if (GpuMemoryLimit != 0)
                {
                    // Scale the data.
                    PhDivideSinglesBySingle(
                        MemGraphState.Data1,
                        (FLOAT)GpuMemoryLimit,
                        drawInfo->LineDataCount
                        );
                }

                MemGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (MemGraphState.TooltipIndex != getTooltipText->Index)
                {
                    ULONG usedPages;

                    usedPages = PhGetItemCircularBuffer_ULONG(&GpuMemoryHistory, getTooltipText->Index);

                    PhMoveReference(&MemGraphState.TooltipText, PhFormatString(
                        L"%s / %s (%.2f%%)\n%s",
                        PhaFormatSize(UInt32x32To64(usedPages, 1024), -1)->Buffer,
                        PhaFormatSize(UInt32x32To64(GpuMemoryLimit, 1024), -1)->Buffer,
                        (FLOAT)usedPages / GpuMemoryLimit * 100,
                        ((PPH_STRING)PhAutoDereferenceObject(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                        ));
                }

                getTooltipText->Text = MemGraphState.TooltipText->sr;
            }
        }
        break;
    }
}

VOID NvGpuNotifySharedGraph(
    _In_ NMHDR *Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y;
            Section->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), 0);
            PhGraphStateGetDrawInfo(&SharedGraphState, getDrawInfo, GpuBoardHistory.Count);

            if (PhGetIntegerSetting(L"GraphShowText"))
            {
                HDC hdc = Graph_GetBufferedContext(SharedGraphHandle);

                PhMoveReference(&SharedGraphState.Text, PhFormatString(
                    L"%.0f%%",
                    (FLOAT)GpuCurrentCoreUsage * 100
                    ));

                SelectObject(hdc, PhApplicationFont);
                PhSetGraphText(hdc, drawInfo, &SharedGraphState.Text->sr,
                    &NormalGraphTextMargin, &NormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
            }
            else
            {
                drawInfo->Text.Buffer = NULL;
            }

            if (!SharedGraphState.Valid)
            {
                PhCopyCircularBuffer_FLOAT(&GpuBoardHistory, SharedGraphState.Data1, drawInfo->LineDataCount);
                SharedGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (SharedGraphState.TooltipIndex != getTooltipText->Index)
                {
                    FLOAT usedPages;

                    usedPages = PhGetItemCircularBuffer_FLOAT(&GpuBoardHistory, getTooltipText->Index);

                    PhMoveReference(&SharedGraphState.TooltipText, PhFormatString(
                        L"%.0f%%\n%s",
                        usedPages * 100,
                        ((PPH_STRING)PhAutoDereferenceObject(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                        ));
                }

                getTooltipText->Text = SharedGraphState.TooltipText->sr;
            }
        }
        break;
    }
}

VOID NvGpuNotifyBusGraph(
    _In_ NMHDR *Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y;
            Section->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), 0);
            PhGraphStateGetDrawInfo(&BusGraphState, getDrawInfo, GpuBusHistory.Count);

            if (PhGetIntegerSetting(L"GraphShowText"))
            {
                HDC hdc = Graph_GetBufferedContext(BusGraphHandle);

                PhMoveReference(&BusGraphState.Text, PhFormatString(
                    L"%.0f%%",
                    (FLOAT)GpuCurrentBusUsage * 100
                    ));

                SelectObject(hdc, PhApplicationFont);
                PhSetGraphText(hdc, drawInfo, &BusGraphState.Text->sr,
                    &NormalGraphTextMargin, &NormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
            }
            else
            {
                drawInfo->Text.Buffer = NULL;
            }

            if (!BusGraphState.Valid)
            {
                PhCopyCircularBuffer_FLOAT(&GpuBusHistory, BusGraphState.Data1, drawInfo->LineDataCount);
                BusGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (BusGraphState.TooltipIndex != getTooltipText->Index)
                {
                    FLOAT busUsage;

                    busUsage = PhGetItemCircularBuffer_FLOAT(&GpuBusHistory, getTooltipText->Index);

                    PhMoveReference(&BusGraphState.TooltipText, PhFormatString(
                        L"%.0f%%\n%s",
                        busUsage * 100,
                        ((PPH_STRING)PhAutoDereferenceObject(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                        ));
                }

                getTooltipText->Text = BusGraphState.TooltipText->sr;
            }
        }
        break;
    }
}

VOID NvGpuUpdateGraphs(
    VOID
    )
{
    GpuGraphState.Valid = FALSE;
    GpuGraphState.TooltipIndex = -1;
    Graph_MoveGrid(GpuGraphHandle, 1);
    Graph_Draw(GpuGraphHandle);
    Graph_UpdateTooltip(GpuGraphHandle);
    InvalidateRect(GpuGraphHandle, NULL, FALSE);

    MemGraphState.Valid = FALSE;
    MemGraphState.TooltipIndex = -1;
    Graph_MoveGrid(MemGraphHandle, 1);
    Graph_Draw(MemGraphHandle);
    Graph_UpdateTooltip(MemGraphHandle);
    InvalidateRect(MemGraphHandle, NULL, FALSE);

    SharedGraphState.Valid = FALSE;
    SharedGraphState.TooltipIndex = -1;
    Graph_MoveGrid(SharedGraphHandle, 1);
    Graph_Draw(SharedGraphHandle);
    Graph_UpdateTooltip(SharedGraphHandle);
    InvalidateRect(SharedGraphHandle, NULL, FALSE);

    BusGraphState.Valid = FALSE;
    BusGraphState.TooltipIndex = -1;
    Graph_MoveGrid(BusGraphHandle, 1);
    Graph_Draw(BusGraphHandle);
    Graph_UpdateTooltip(BusGraphHandle);
    InvalidateRect(BusGraphHandle, NULL, FALSE);
}

VOID NvGpuUpdatePanel(
    VOID
    )
{
    SetDlgItemText(GpuPanel, IDC_CLOCK_CORE, PhaFormatString(L"%lu MHz", GpuCurrentCoreClock)->Buffer);
    SetDlgItemText(GpuPanel, IDC_CLOCK_MEMORY, PhaFormatString(L"%lu MHz", GpuCurrentMemoryClock)->Buffer);
    SetDlgItemText(GpuPanel, IDC_CLOCK_SHADER, PhaFormatString(L"%lu MHz", GpuCurrentShaderClock)->Buffer);
    SetDlgItemText(GpuPanel, IDC_FAN_PERCENT, ((PPH_STRING)PhAutoDereferenceObject(NvGpuQueryFanSpeed()))->Buffer);

    if (PhGetIntegerSetting(SETTING_NAME_ENABLE_FAHRENHEIT))
    {
        FLOAT fahrenheit = (FLOAT)(GpuCurrentCoreTemp * 1.8 + 32);

        SetDlgItemText(GpuPanel, IDC_TEMP_VALUE, PhaFormatString(L"%.1f\u00b0F", fahrenheit)->Buffer);
    }
    else
    {
        SetDlgItemText(GpuPanel, IDC_TEMP_VALUE, PhaFormatString(L"%lu\u00b0C", GpuCurrentCoreTemp)->Buffer);
    }

    //SetDlgItemText(GpuPanel, IDC_TEMP_VALUE, PhaFormatString(L"%s\u00b0C", PhaFormatUInt64(GpuCurrentBoardTemp, TRUE)->Buffer)->Buffer);
    SetDlgItemText(GpuPanel, IDC_VOLTAGE, PhaFormatString(L"%lu mV", GpuCurrentVoltage)->Buffer);
}

static INT_PTR CALLBACK NvGpuDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    //PhDeleteLayoutManager(&LayoutManager);

    //PhDeleteGraphState(&GpuGraphState);
    //PhDeleteGraphState(&MemGraphState);
    //PhDeleteGraphState(&SharedGraphState);
    //PhDeleteGraphState(&BusGraphState);

    //if (GpuGraphHandle)
    //    DestroyWindow(GpuGraphHandle);
    //if (MemGraphHandle)
    //    DestroyWindow(MemGraphHandle);
    //if (SharedGraphHandle)
    //    DestroyWindow(SharedGraphHandle);
    //if (BusGraphHandle)
    //    DestroyWindow(BusGraphHandle);
    //if (GpuPanel)
    //    DestroyWindow(GpuPanel);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PPH_LAYOUT_ITEM graphItem;
            PPH_LAYOUT_ITEM panelItem;

            WindowHandle = hwndDlg;

            GpuLabelHandle = GetDlgItem(hwndDlg, IDC_GPU_L);
            MemLabelHandle = GetDlgItem(hwndDlg, IDC_MEMORY_L);
            SharedLabelHandle = GetDlgItem(hwndDlg, IDC_SHARED_L);
            BusLabelHandle = GetDlgItem(hwndDlg, IDC_BUS_L);

            PhInitializeGraphState(&GpuGraphState);
            PhInitializeGraphState(&MemGraphState);
            PhInitializeGraphState(&SharedGraphState);
            PhInitializeGraphState(&BusGraphState);

            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_GPUNAME), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            graphItem = PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            GpuGraphMargin = graphItem->Margin;
            panelItem = PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            SendMessage(GetDlgItem(hwndDlg, IDC_TITLE), WM_SETFONT, (WPARAM)Section->Parameters->LargeFont, FALSE);
            SendMessage(GetDlgItem(hwndDlg, IDC_GPUNAME), WM_SETFONT, (WPARAM)Section->Parameters->MediumFont, FALSE);
            SetDlgItemText(hwndDlg, IDC_GPUNAME, GpuName->Buffer);

            GpuPanel = CreateDialog(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_GPU_PANEL), hwndDlg, NvGpuPanelDialogProc);
            ShowWindow(GpuPanel, SW_SHOW);
            PhAddLayoutItemEx(&LayoutManager, GpuPanel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, panelItem->Margin);

            NvGpuCreateGraphs();

            NvGpuUpdate();
            NvGpuUpdateGraphs();
            NvGpuUpdatePanel();
        }
        break;
    case WM_SIZE:
        NvGpuLayoutGraphs();
        break;
    case WM_NOTIFY:
        {
            NMHDR* header = (NMHDR*)lParam;

            if (header->hwndFrom == GpuGraphHandle)
            {
                NvGpuNotifyUsageGraph(header);
            }
            else if (header->hwndFrom == MemGraphHandle)
            {
                NvGpuNotifyMemoryGraph(header);
            }
            else if (header->hwndFrom == SharedGraphHandle)
            {
                NvGpuNotifySharedGraph(header);
            }
            else if (header->hwndFrom == BusGraphHandle)
            {
                NvGpuNotifyBusGraph(header);
            }
        }
        break;
    case UPDATE_MSG:
        {
            NvGpuUpdateGraphs();
            NvGpuUpdatePanel();
        }
        break;
    }

    return FALSE;
}

static BOOLEAN NvGpuSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    switch (Message)
    {
    case SysInfoCreate:
        return TRUE;
    case SysInfoDestroy:
        return TRUE;
    case SysInfoTick:
        {
            if (WindowHandle)
                PostMessage(WindowHandle, UPDATE_MSG, 0, 0);

            if (DetailsHandle)
                PostMessage(DetailsHandle, UPDATE_MSG, 0, 0);
        }
        return TRUE;
    case SysInfoCreateDialog:
        {
            PPH_SYSINFO_CREATE_DIALOG createDialog = (PPH_SYSINFO_CREATE_DIALOG)Parameter1;

            createDialog->Instance = PluginInstance->DllBase;
            createDialog->Template = MAKEINTRESOURCE(IDD_GPU_DIALOG);
            createDialog->DialogProc = NvGpuDialogProc;
            //createDialog->Parameter = context;
        }
        return TRUE;
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = (PPH_GRAPH_DRAW_INFO)Parameter1;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y;
            Section->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), 0);
            PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, GpuUtilizationHistory.Count);

            if (!Section->GraphState.Valid)
            {
                PhCopyCircularBuffer_FLOAT(&GpuUtilizationHistory, Section->GraphState.Data1, drawInfo->LineDataCount);
                Section->GraphState.Valid = TRUE;
            }
        }
        return TRUE;
    case SysInfoGraphGetTooltipText:
        {
            FLOAT gpuUsageValue;
            PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT getTooltipText = (PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT)Parameter1;

            gpuUsageValue = PhGetItemCircularBuffer_FLOAT(&GpuUtilizationHistory, getTooltipText->Index);

            PhMoveReference(&Section->GraphState.TooltipText, PhFormatString(
                L"%.0f%%\n%s",
                gpuUsageValue * 100,
                ((PPH_STRING)PhAutoDereferenceObject(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                ));

            getTooltipText->Text = Section->GraphState.TooltipText->sr;
        }
        return TRUE;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = (PPH_SYSINFO_DRAW_PANEL)Parameter1;

            drawPanel->Title = PhCreateString(Section->Name.Buffer);
            drawPanel->SubTitle = PhFormatString(
                L"%.0f%%",
                GpuCurrentGpuUsage * 100
                );
        }
        return TRUE;
    }

    return FALSE;
}

VOID NvGpuSysInfoInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers
    )
{
    PH_SYSINFO_SECTION section;

    if (!PhGetIntegerSetting(SETTING_NAME_ENABLE_GPU))
        return;

    if (!NvApiInitialized)
        return;

    memset(&section, 0, sizeof(PH_SYSINFO_SECTION));

    section.Callback = NvGpuSectionCallback;

    GpuName = NvGpuQueryName();
    PhInitializeStringRef(&section.Name, GpuName->Buffer);

    Section = Pointers->CreateSection(&section);
}

#endif