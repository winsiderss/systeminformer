/*
 * Process Hacker Extended Tools -
 *   ETW process properties page
 *
 * Copyright (C) 2010-2011 wj32
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

#include "exttools.h"
#include "resource.h"

#define MSG_UPDATE (WM_APP + 1)

static RECT NormalGraphTextMargin = { 5, 5, 5, 5 };
static RECT NormalGraphTextPadding = { 3, 3, 3, 3 };

typedef struct _ET_DISKNET_CONTEXT
{
    HWND WindowHandle;
    PET_PROCESS_BLOCK Block;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;
    BOOLEAN Enabled;

    PH_LAYOUT_MANAGER LayoutManager;

    HWND DiskGroupBox;
    HWND NetworkGroupBox;

    HWND DiskGraphHandle;
    HWND NetworkGraphHandle;
    HWND PanelHandle;

    ULONG64 CurrentDiskRead;
    ULONG64 CurrentDiskWrite;
    ULONG64 CurrentNetworkSend;
    ULONG64 CurrentNetworkReceive;

    PH_GRAPH_STATE DiskGraphState;
    PH_GRAPH_STATE NetworkGraphState;

    PH_CIRCULAR_BUFFER_ULONG64 DiskReadHistory;
    PH_CIRCULAR_BUFFER_ULONG64 DiskWriteHistory;
    PH_CIRCULAR_BUFFER_ULONG64 NetworkSendHistory;
    PH_CIRCULAR_BUFFER_ULONG64 NetworkReceiveHistory;
} ET_DISKNET_CONTEXT, *PET_DISKNET_CONTEXT;

static INT_PTR CALLBACK EtwDiskNetworkPanelDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    return FALSE;
}

static VOID EtwDiskNetworkCreateGraphs(
    _In_ PET_DISKNET_CONTEXT Context
    )
{
    Context->DiskGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        Context->WindowHandle,
        NULL,
        NULL,
        NULL
        );
    Graph_SetTooltip(Context->DiskGraphHandle, TRUE);

    Context->NetworkGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        Context->WindowHandle,
        NULL,
        NULL,
        NULL
        );
    Graph_SetTooltip(Context->NetworkGraphHandle, TRUE);
}

static VOID EtwDiskNetworkCreatePanel(
    _In_ PET_DISKNET_CONTEXT Context
    )
{
    RECT margin;

    Context->PanelHandle = CreateDialogParam(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_PROCDISKNET_PANEL),
        Context->WindowHandle,
        EtwDiskNetworkPanelDialogProc,
        (LPARAM)Context
        );

    SetWindowPos(
        Context->PanelHandle,
        NULL,
        10, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER
        );

    ShowWindow(Context->PanelHandle, SW_SHOW);

    margin.left = 0;
    margin.top = 0;
    margin.right = 0;
    margin.bottom = 10;
    MapDialogRect(Context->WindowHandle, &margin);

    PhAddLayoutItemEx(
        &Context->LayoutManager,
        Context->PanelHandle,
        NULL,
        PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT,
        margin
        );

    SendMessage(Context->WindowHandle, WM_SIZE, 0, 0);
}

static VOID EtwDiskNetworkLayoutGraphs(
    _In_ PET_DISKNET_CONTEXT Context
    )
{
    HDWP deferHandle;
    RECT clientRect;
    RECT panelRect;
    RECT margin = { 13, 13, 13, 13 };
    RECT innerMargin = { 10, 20, 10, 10 };
    LONG between = 3;
    ULONG graphWidth;
    ULONG graphHeight;

    PhLayoutManagerLayout(&Context->LayoutManager);

    Context->DiskGraphState.Valid = FALSE;
    Context->NetworkGraphState.Valid = FALSE;

    GetClientRect(Context->WindowHandle, &clientRect);

    // Limit the rectangle bottom to the top of the panel.
    GetWindowRect(Context->PanelHandle, &panelRect);
    MapWindowPoints(NULL, Context->WindowHandle, (PPOINT)&panelRect, 2);
    clientRect.bottom = panelRect.top + 10; // +10 removing extra spacing

    graphWidth = clientRect.right - margin.left - margin.right;
    graphHeight = (clientRect.bottom - margin.top - margin.bottom - between * 2) / 2;

    deferHandle = BeginDeferWindowPos(4);

    deferHandle = DeferWindowPos(deferHandle, Context->DiskGroupBox, NULL, margin.left, margin.top, graphWidth, graphHeight, SWP_NOACTIVATE | SWP_NOZORDER);
    deferHandle = DeferWindowPos(
        deferHandle,
        Context->DiskGraphHandle,
        NULL,
        margin.left + innerMargin.left,
        margin.top + innerMargin.top,
        graphWidth - innerMargin.left - innerMargin.right,
        graphHeight - innerMargin.top - innerMargin.bottom,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    deferHandle = DeferWindowPos(deferHandle, Context->NetworkGroupBox, NULL, margin.left, margin.top + graphHeight + between, graphWidth, graphHeight, SWP_NOACTIVATE | SWP_NOZORDER);
    deferHandle = DeferWindowPos(
        deferHandle,
        Context->NetworkGraphHandle,
        NULL,
        margin.left + innerMargin.left,
        margin.top + graphHeight + between + innerMargin.top,
        graphWidth - innerMargin.left - innerMargin.right,
        graphHeight - innerMargin.top - innerMargin.bottom,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    EndDeferWindowPos(deferHandle);
}

static VOID EtwDiskNetworkUpdateGraphs(
    _In_ PET_DISKNET_CONTEXT Context
    )
{
    Context->DiskGraphState.Valid = FALSE;
    Context->DiskGraphState.TooltipIndex = -1;
    Graph_MoveGrid(Context->DiskGraphHandle, 1);
    Graph_Draw(Context->DiskGraphHandle);
    Graph_UpdateTooltip(Context->DiskGraphHandle);
    InvalidateRect(Context->DiskGraphHandle, NULL, FALSE);

    Context->NetworkGraphState.Valid = FALSE;
    Context->NetworkGraphState.TooltipIndex = -1;
    Graph_MoveGrid(Context->NetworkGraphHandle, 1);
    Graph_Draw(Context->NetworkGraphHandle);
    Graph_UpdateTooltip(Context->NetworkGraphHandle);
    InvalidateRect(Context->NetworkGraphHandle, NULL, FALSE);
}

static VOID EtwDiskNetworkUpdatePanel(
    _Inout_ PET_DISKNET_CONTEXT Context
    )
{
    PET_PROCESS_BLOCK block = Context->Block;

    SetDlgItemText(Context->PanelHandle, IDC_ZREADS_V, PhaFormatUInt64(block->DiskReadCount, TRUE)->Buffer);
    SetDlgItemText(Context->PanelHandle, IDC_ZREADBYTES_V, PhaFormatSize(block->DiskReadRawDelta.Value, -1)->Buffer);
    SetDlgItemText(Context->PanelHandle, IDC_ZREADBYTESDELTA_V, PhaFormatSize(block->DiskReadRawDelta.Delta, -1)->Buffer);
    SetDlgItemText(Context->PanelHandle, IDC_ZWRITES_V, PhaFormatUInt64(block->DiskWriteCount, TRUE)->Buffer);
    SetDlgItemText(Context->PanelHandle, IDC_ZWRITEBYTES_V, PhaFormatSize(block->DiskWriteRawDelta.Value, -1)->Buffer);
    SetDlgItemText(Context->PanelHandle, IDC_ZWRITEBYTESDELTA_V, PhaFormatSize(block->DiskWriteRawDelta.Delta, -1)->Buffer);

    SetDlgItemText(Context->PanelHandle, IDC_ZRECEIVES_V, PhaFormatUInt64(block->NetworkReceiveCount, TRUE)->Buffer);
    SetDlgItemText(Context->PanelHandle, IDC_ZRECEIVEBYTES_V, PhaFormatSize(block->NetworkReceiveRawDelta.Value, -1)->Buffer);
    SetDlgItemText(Context->PanelHandle, IDC_ZRECEIVEBYTESDELTA_V, PhaFormatSize(block->NetworkReceiveRawDelta.Delta, -1)->Buffer);
    SetDlgItemText(Context->PanelHandle, IDC_ZSENDS_V, PhaFormatUInt64(block->NetworkSendCount, TRUE)->Buffer);
    SetDlgItemText(Context->PanelHandle, IDC_ZSENDBYTES_V, PhaFormatSize(block->NetworkSendRawDelta.Value, -1)->Buffer);
    SetDlgItemText(Context->PanelHandle, IDC_ZSENDBYTESDELTA_V, PhaFormatSize(block->NetworkSendRawDelta.Delta, -1)->Buffer);
}

static VOID EtwDiskNetworkUpdateInfo(
    _In_ PET_DISKNET_CONTEXT Context
    )
{
    PET_PROCESS_BLOCK block = Context->Block;

    Context->CurrentDiskRead = block->DiskReadRawDelta.Delta;
    Context->CurrentDiskWrite = block->DiskWriteRawDelta.Delta;
    Context->CurrentNetworkSend = block->NetworkSendRawDelta.Delta;
    Context->CurrentNetworkReceive = block->NetworkReceiveRawDelta.Delta;

    PhAddItemCircularBuffer_ULONG64(&Context->DiskReadHistory, Context->CurrentDiskRead);
    PhAddItemCircularBuffer_ULONG64(&Context->DiskWriteHistory, Context->CurrentDiskWrite);
    PhAddItemCircularBuffer_ULONG64(&Context->NetworkSendHistory, Context->CurrentNetworkSend);
    PhAddItemCircularBuffer_ULONG64(&Context->NetworkReceiveHistory, Context->CurrentNetworkReceive);
}

static VOID NTAPI EtwDiskNetworkUpdateHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PET_DISKNET_CONTEXT context = Context;

    if (!context->Enabled)
        return;

    if (context->WindowHandle)
    {
        PostMessage(context->WindowHandle, MSG_UPDATE, 0, 0);
    }
}

static INT_PTR CALLBACK EtwDiskNetworkPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PET_DISKNET_CONTEXT context;

    if (PhPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext, &processItem))
    {
        context = propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            ULONG sampleCount;

            sampleCount = PhGetIntegerSetting(L"SampleCount");

            context = PhAllocate(sizeof(ET_DISKNET_CONTEXT));
            memset(context, 0, sizeof(ET_DISKNET_CONTEXT));

            context->WindowHandle = hwndDlg;
            context->Block = EtGetProcessBlock(processItem);
            context->Enabled = TRUE;
            context->DiskGroupBox = GetDlgItem(hwndDlg, IDC_GROUPDISK);
            context->NetworkGroupBox = GetDlgItem(hwndDlg, IDC_GROUPNETWORK);
            propPageContext->Context = context;

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);

            PhInitializeGraphState(&context->DiskGraphState);
            PhInitializeGraphState(&context->NetworkGraphState);

            PhInitializeCircularBuffer_ULONG64(&context->DiskReadHistory, sampleCount);
            PhInitializeCircularBuffer_ULONG64(&context->DiskWriteHistory, sampleCount);
            PhInitializeCircularBuffer_ULONG64(&context->NetworkSendHistory, sampleCount);
            PhInitializeCircularBuffer_ULONG64(&context->NetworkReceiveHistory, sampleCount);

            EtwDiskNetworkCreateGraphs(context);
            EtwDiskNetworkCreatePanel(context);
            EtwDiskNetworkUpdateInfo(context);
            EtwDiskNetworkUpdatePanel(context);

            PhRegisterCallback(
                &PhProcessesUpdatedEvent,
                EtwDiskNetworkUpdateHandler,
                context,
                &context->ProcessesUpdatedRegistration
                );
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);

            PhDeleteGraphState(&context->DiskGraphState);
            PhDeleteGraphState(&context->NetworkGraphState);

            PhDeleteCircularBuffer_ULONG64(&context->DiskReadHistory);
            PhDeleteCircularBuffer_ULONG64(&context->DiskWriteHistory);
            PhDeleteCircularBuffer_ULONG64(&context->NetworkSendHistory);
            PhDeleteCircularBuffer_ULONG64(&context->NetworkReceiveHistory);

            if (context->DiskGraphHandle)
                DestroyWindow(context->DiskGraphHandle);
            if (context->NetworkGraphHandle)
                DestroyWindow(context->NetworkGraphHandle);
            if (context->PanelHandle)
                DestroyWindow(context->PanelHandle);

            PhUnregisterCallback(&PhProcessesUpdatedEvent, &context->ProcessesUpdatedRegistration);
            PhFree(context);

            PhPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (PhBeginPropPageLayout(hwndDlg, propPageContext))
                PhEndPropPageLayout(hwndDlg, propPageContext);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_SETACTIVE:
                context->Enabled = TRUE;
                break;
            case PSN_KILLACTIVE:
                context->Enabled = FALSE;
                break;
            case GCN_GETDRAWINFO:
                {
                    PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)header;
                    PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

                    if (header->hwndFrom == context->DiskGraphHandle)
                    {
                        if (PhGetIntegerSetting(L"GraphShowText"))
                        {
                            HDC hdc;

                            PhMoveReference(&context->DiskGraphState.Text, PhFormatString(
                                L"R: %s, W: %s",
                                PhaFormatSize(context->CurrentDiskRead, -1)->Buffer,
                                PhaFormatSize(context->CurrentDiskWrite, -1)->Buffer
                                ));

                            hdc = Graph_GetBufferedContext(context->DiskGraphHandle);
                            SelectObject(hdc, PhApplicationFont);
                            PhSetGraphText(hdc, drawInfo, &context->DiskGraphState.Text->sr,
                                &NormalGraphTextMargin, &NormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }

                        drawInfo->Flags = PH_GRAPH_USE_GRID | PH_GRAPH_USE_LINE_2;
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), PhGetIntegerSetting(L"ColorIoWrite"));
                        PhGraphStateGetDrawInfo(&context->DiskGraphState, getDrawInfo, context->DiskReadHistory.Count);

                        if (!context->DiskGraphState.Valid)
                        {
                            FLOAT max = 0;

                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data1;
                                FLOAT data2;

                                context->DiskGraphState.Data1[i] = data1 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->DiskReadHistory, i);
                                context->DiskGraphState.Data2[i] = data2 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->DiskWriteHistory, i);

                                if (max < data1 + data2)
                                    max = data1 + data2;
                            }

                            // Minimum scaling of 1 MB.
                            //if (max < 1024 * 1024)
                            //    max = 1024 * 1024;

                            // Scale the data.
                            PhDivideSinglesBySingle(
                                context->DiskGraphState.Data1,
                                max,
                                drawInfo->LineDataCount
                                );

                            // Scale the data.
                            PhDivideSinglesBySingle(
                                context->DiskGraphState.Data2,
                                max,
                                drawInfo->LineDataCount
                                );

                            context->DiskGraphState.Valid = TRUE;
                        }
                    }
                    else if (header->hwndFrom == context->NetworkGraphHandle)
                    {
                        if (PhGetIntegerSetting(L"GraphShowText"))
                        {
                            HDC hdc;

                            PhMoveReference(&context->NetworkGraphState.Text, PhFormatString(
                                L"R: %s, S: %s",
                                PhaFormatSize(context->CurrentNetworkReceive, -1)->Buffer,
                                PhaFormatSize(context->CurrentNetworkSend, -1)->Buffer
                                ));

                            hdc = Graph_GetBufferedContext(context->NetworkGraphHandle);
                            SelectObject(hdc, PhApplicationFont);
                            PhSetGraphText(hdc, drawInfo, &context->NetworkGraphState.Text->sr,
                                &NormalGraphTextMargin, &NormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }

                        drawInfo->Flags = PH_GRAPH_USE_GRID | PH_GRAPH_USE_LINE_2;
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), PhGetIntegerSetting(L"ColorIoWrite"));
                        PhGraphStateGetDrawInfo(&context->NetworkGraphState, getDrawInfo, context->NetworkSendHistory.Count);

                        if (!context->NetworkGraphState.Valid)
                        {
                            FLOAT max = 0;

                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data1;
                                FLOAT data2;

                                context->NetworkGraphState.Data1[i] = data1 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->NetworkReceiveHistory, i);
                                context->NetworkGraphState.Data2[i] = data2 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->NetworkSendHistory, i);

                                if (max < data1 + data2)
                                    max = data1 + data2;
                            }

                            // Minimum scaling of 1 MB.
                            //if (max < 1024 * 1024)
                            //    max = 1024 * 1024;

                            // Scale the data.
                            PhDivideSinglesBySingle(
                                context->NetworkGraphState.Data1,
                                max,
                                drawInfo->LineDataCount
                                );

                            // Scale the data.
                            PhDivideSinglesBySingle(
                                context->NetworkGraphState.Data2,
                                max,
                                drawInfo->LineDataCount
                                );

                            context->NetworkGraphState.Valid = TRUE;
                        }
                    }
                }
                break;
            case GCN_GETTOOLTIPTEXT:
                {
                    PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)lParam;

                    if (getTooltipText->Index < getTooltipText->TotalCount)
                    {
                        if (header->hwndFrom == context->DiskGraphHandle)
                        {
                            if (context->DiskGraphState.TooltipIndex != getTooltipText->Index)
                            {
                                ULONG64 diskRead = PhGetItemCircularBuffer_ULONG64(
                                    &context->DiskReadHistory,
                                    getTooltipText->Index
                                    );

                                ULONG64 diskWrite = PhGetItemCircularBuffer_ULONG64(
                                    &context->DiskWriteHistory,
                                    getTooltipText->Index
                                    );

                                PhMoveReference(&context->DiskGraphState.TooltipText, PhFormatString(
                                    L"R: %s\nW: %s\n%s",
                                    PhaFormatSize(diskRead, -1)->Buffer,
                                    PhaFormatSize(diskWrite, -1)->Buffer,
                                    ((PPH_STRING)PhAutoDereferenceObject(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                                    ));
                            }

                            getTooltipText->Text = context->DiskGraphState.TooltipText->sr;
                        }
                        else if (header->hwndFrom == context->NetworkGraphHandle)
                        {
                            if (context->NetworkGraphState.TooltipIndex != getTooltipText->Index)
                            {
                                ULONG64 networkSend = PhGetItemCircularBuffer_ULONG64(
                                    &context->NetworkSendHistory,
                                    getTooltipText->Index
                                    );

                                ULONG64 networkReceive = PhGetItemCircularBuffer_ULONG64(
                                    &context->NetworkReceiveHistory,
                                    getTooltipText->Index
                                    );

                                PhMoveReference(&context->NetworkGraphState.TooltipText, PhFormatString(
                                    L"S: %s\nR: %s\n%s",
                                    PhaFormatSize(networkSend, -1)->Buffer,
                                    PhaFormatSize(networkReceive, -1)->Buffer,
                                    ((PPH_STRING)PhAutoDereferenceObject(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                                    ));
                            }

                            getTooltipText->Text = context->NetworkGraphState.TooltipText->sr;
                        }
                    }
                }
                break;
            }
        }
        break;
    case MSG_UPDATE:
        {
            EtwDiskNetworkUpdateInfo(context);
            EtwDiskNetworkUpdateGraphs(context);
            EtwDiskNetworkUpdatePanel(context);
        }
        break;
    case WM_SIZE:
        {
            EtwDiskNetworkLayoutGraphs(context);
        }
        break;
    }

    return FALSE;
}

VOID EtProcessEtwPropertiesInitializing(
    _In_ PVOID Parameter
    )
{
    PPH_PLUGIN_PROCESS_PROPCONTEXT propContext = Parameter;

    if (EtEtwEnabled)
    {
        PhAddProcessPropPage(
            propContext->PropContext,
            PhCreateProcessPropPageContextEx(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_PROCDISKNET), EtwDiskNetworkPageDlgProc, NULL)
            );
    }
}