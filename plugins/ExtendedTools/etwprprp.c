/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2015-2023
 *
 */

#include "exttools.h"

static RECT NormalGraphTextMargin = { 5, 5, 5, 5 };
static RECT NormalGraphTextPadding = { 3, 3, 3, 3 };

typedef struct _ET_DISK_CONTEXT
{
    HWND WindowHandle;
    PET_PROCESS_BLOCK Block;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;
    BOOLEAN Enabled;
    LONG WindowDpi;

    PH_LAYOUT_MANAGER LayoutManager;

    HWND DiskReadGroupBox;
    HWND DiskWriteGroupBox;
    HWND DiskReadGraphHandle;
    HWND DiskWriteGraphHandle;
    HWND PanelHandle;

    PH_GRAPH_STATE DiskReadGraphState;
    PH_GRAPH_STATE DiskWriteGraphState;
} ET_DISK_CONTEXT, *PET_DISK_CONTEXT;

typedef struct _ET_NET_CONTEXT
{
    HWND WindowHandle;
    PET_PROCESS_BLOCK Block;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;
    BOOLEAN Enabled;
    LONG WindowDpi;

    PH_LAYOUT_MANAGER LayoutManager;

    HWND NetworkSendGroupBox;
    HWND NetworkReceiveGroupBox;
    HWND NetworkSendGraphHandle;
    HWND NetworkReceiveGraphHandle;
    HWND PanelHandle;

    PH_GRAPH_STATE NetworkSendGraphState;
    PH_GRAPH_STATE NetworkReceiveGraphState;
} ET_NET_CONTEXT, *PET_NET_CONTEXT;

INT_PTR CALLBACK EtwDiskNetworkPanelDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    return FALSE;
}

VOID EtwDiskUpdateWindowDpi(
    _In_ PET_DISK_CONTEXT Context
    )
{
    Context->WindowDpi = PhGetWindowDpi(Context->WindowHandle);
}

VOID EtwNetworkUpdateWindowDpi(
    _In_ PET_NET_CONTEXT Context
    )
{
    Context->WindowDpi = PhGetWindowDpi(Context->WindowHandle);
}

VOID EtwDiskCreateGraphs(
    _In_ PET_DISK_CONTEXT Context
    )
{
    Context->DiskReadGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER | WS_CLIPSIBLINGS,
        0,
        0,
        3,
        3,
        Context->WindowHandle,
        NULL,
        NULL,
        NULL
        );
    Graph_SetTooltip(Context->DiskReadGraphHandle, TRUE);

    Context->DiskWriteGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER | WS_CLIPSIBLINGS,
        0,
        0,
        3,
        3,
        Context->WindowHandle,
        NULL,
        NULL,
        NULL
        );
    Graph_SetTooltip(Context->DiskWriteGraphHandle, TRUE);
}

VOID EtwNetworkCreateGraphs(
    _In_ PET_NET_CONTEXT Context
    )
{
    Context->NetworkSendGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER | WS_CLIPSIBLINGS,
        0,
        0,
        3,
        3,
        Context->WindowHandle,
        NULL,
        NULL,
        NULL
        );
    Graph_SetTooltip(Context->NetworkSendGraphHandle, TRUE);

    Context->NetworkReceiveGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER | WS_CLIPSIBLINGS,
        0,
        0,
        3,
        3,
        Context->WindowHandle,
        NULL,
        NULL,
        NULL
        );
    Graph_SetTooltip(Context->NetworkReceiveGraphHandle, TRUE);
}

VOID EtwDiskCreatePanel(
    _In_ PET_DISK_CONTEXT Context
    )
{
    RECT margin;

    Context->PanelHandle = PhCreateDialog(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_PROCDISK_PANEL),
        Context->WindowHandle,
        EtwDiskNetworkPanelDialogProc,
        Context
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

VOID EtwNetworkCreatePanel(
    _In_ PET_NET_CONTEXT Context
    )
{
    RECT margin;

    Context->PanelHandle = PhCreateDialog(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_PROCNET_PANEL),
        Context->WindowHandle,
        EtwDiskNetworkPanelDialogProc,
        Context
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

VOID EtwDiskLayoutGraphs(
    _In_ PET_DISK_CONTEXT Context
    )
{
    HDWP deferHandle;
    RECT clientRect;
    RECT panelRect;
    RECT margin;
    RECT innerMargin;
    LONG between;
    ULONG graphWidth;
    ULONG graphHeight;

    margin.left = margin.top = margin.right = margin.bottom = PhGetDpi(13, Context->WindowDpi);

    innerMargin.left = innerMargin.right = innerMargin.bottom = PhGetDpi(10, Context->WindowDpi);
    innerMargin.top = PhGetDpi(20, Context->WindowDpi);

    between = PhGetDpi(3, Context->WindowDpi);

    PhLayoutManagerLayout(&Context->LayoutManager);

    Context->DiskReadGraphState.Valid = FALSE;
    Context->DiskReadGraphState.TooltipIndex = ULONG_MAX;
    Context->DiskWriteGraphState.Valid = FALSE;
    Context->DiskWriteGraphState.TooltipIndex = ULONG_MAX;

    GetClientRect(Context->WindowHandle, &clientRect);

    // Limit the rectangle bottom to the top of the panel.
    GetWindowRect(Context->PanelHandle, &panelRect);
    MapWindowPoints(NULL, Context->WindowHandle, (PPOINT)&panelRect, 2);
    clientRect.bottom = panelRect.top + 10; // +10 removing extra spacing

    graphWidth = clientRect.right - margin.left - margin.right;
    graphHeight = (clientRect.bottom - margin.top - margin.bottom - between * 2) / 2;

    deferHandle = BeginDeferWindowPos(4);

    deferHandle = DeferWindowPos(deferHandle, Context->DiskReadGroupBox, NULL, margin.left, margin.top, graphWidth, graphHeight, SWP_NOACTIVATE | SWP_NOZORDER);
    deferHandle = DeferWindowPos(
        deferHandle,
        Context->DiskReadGraphHandle,
        NULL,
        margin.left + innerMargin.left,
        margin.top + innerMargin.top,
        graphWidth - innerMargin.left - innerMargin.right,
        graphHeight - innerMargin.top - innerMargin.bottom,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    deferHandle = DeferWindowPos(deferHandle, Context->DiskWriteGroupBox, NULL, margin.left, margin.top + graphHeight + between, graphWidth, graphHeight, SWP_NOACTIVATE | SWP_NOZORDER);
    deferHandle = DeferWindowPos(
        deferHandle,
        Context->DiskWriteGraphHandle,
        NULL,
        margin.left + innerMargin.left,
        margin.top + graphHeight + between + innerMargin.top,
        graphWidth - innerMargin.left - innerMargin.right,
        graphHeight - innerMargin.top - innerMargin.bottom,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    EndDeferWindowPos(deferHandle);
}

VOID EtwNetworkLayoutGraphs(
    _In_ PET_NET_CONTEXT Context
    )
{
    HDWP deferHandle;
    RECT clientRect;
    RECT panelRect;
    RECT margin;
    RECT innerMargin;
    LONG between;
    ULONG graphWidth;
    ULONG graphHeight;

    margin.left = margin.top = margin.right = margin.bottom = PhGetDpi(13, Context->WindowDpi);

    innerMargin.left = innerMargin.right = innerMargin.bottom = PhGetDpi(10, Context->WindowDpi);
    innerMargin.top = PhGetDpi(20, Context->WindowDpi);

    between = PhGetDpi(3, Context->WindowDpi);

    PhLayoutManagerLayout(&Context->LayoutManager);

    Context->NetworkSendGraphState.Valid = FALSE;
    Context->NetworkSendGraphState.TooltipIndex = ULONG_MAX;
    Context->NetworkReceiveGraphState.Valid = FALSE;
    Context->NetworkReceiveGraphState.TooltipIndex = ULONG_MAX;

    GetClientRect(Context->WindowHandle, &clientRect);

    // Limit the rectangle bottom to the top of the panel.
    GetWindowRect(Context->PanelHandle, &panelRect);
    MapWindowPoints(NULL, Context->WindowHandle, (PPOINT)&panelRect, 2);
    clientRect.bottom = panelRect.top + 10; // +10 removing extra spacing

    graphWidth = clientRect.right - margin.left - margin.right;
    graphHeight = (clientRect.bottom - margin.top - margin.bottom - between * 2) / 2;

    deferHandle = BeginDeferWindowPos(4);

    deferHandle = DeferWindowPos(deferHandle, Context->NetworkReceiveGroupBox, NULL, margin.left, margin.top, graphWidth, graphHeight, SWP_NOACTIVATE | SWP_NOZORDER);
    deferHandle = DeferWindowPos(
        deferHandle,
        Context->NetworkReceiveGraphHandle,
        NULL,
        margin.left + innerMargin.left,
        margin.top + innerMargin.top,
        graphWidth - innerMargin.left - innerMargin.right,
        graphHeight - innerMargin.top - innerMargin.bottom,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    deferHandle = DeferWindowPos(deferHandle, Context->NetworkSendGroupBox, NULL, margin.left, margin.top + graphHeight + between, graphWidth, graphHeight, SWP_NOACTIVATE | SWP_NOZORDER);
    deferHandle = DeferWindowPos(
        deferHandle,
        Context->NetworkSendGraphHandle,
        NULL,
        margin.left + innerMargin.left,
        margin.top + graphHeight + between + innerMargin.top,
        graphWidth - innerMargin.left - innerMargin.right,
        graphHeight - innerMargin.top - innerMargin.bottom,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    EndDeferWindowPos(deferHandle);
}

VOID EtwDiskUpdateGraphs(
    _In_ PET_DISK_CONTEXT Context
    )
{
    Context->DiskReadGraphState.Valid = FALSE;
    Context->DiskReadGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->DiskReadGraphHandle, 1);
    Graph_Draw(Context->DiskReadGraphHandle);
    Graph_UpdateTooltip(Context->DiskReadGraphHandle);
    InvalidateRect(Context->DiskReadGraphHandle, NULL, FALSE);

    Context->DiskWriteGraphState.Valid = FALSE;
    Context->DiskWriteGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->DiskWriteGraphHandle, 1);
    Graph_Draw(Context->DiskWriteGraphHandle);
    Graph_UpdateTooltip(Context->DiskWriteGraphHandle);
    InvalidateRect(Context->DiskWriteGraphHandle, NULL, FALSE);
}

VOID EtwNetworkUpdateGraphs(
    _In_ PET_NET_CONTEXT Context
    )
{
    Context->NetworkReceiveGraphState.Valid = FALSE;
    Context->NetworkReceiveGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->NetworkReceiveGraphHandle, 1);
    Graph_Draw(Context->NetworkReceiveGraphHandle);
    Graph_UpdateTooltip(Context->NetworkReceiveGraphHandle);
    InvalidateRect(Context->NetworkReceiveGraphHandle, NULL, FALSE);

    Context->NetworkSendGraphState.Valid = FALSE;
    Context->NetworkSendGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->NetworkSendGraphHandle, 1);
    Graph_Draw(Context->NetworkSendGraphHandle);
    Graph_UpdateTooltip(Context->NetworkSendGraphHandle);
    InvalidateRect(Context->NetworkSendGraphHandle, NULL, FALSE);
}

VOID EtwDiskUpdatePanel(
    _Inout_ PET_DISK_CONTEXT Context
    )
{
    PET_PROCESS_BLOCK block = Context->Block;

    PhSetDialogItemText(Context->PanelHandle, IDC_ZREADS_V, PhaFormatUInt64(block->DiskReadCount, TRUE)->Buffer);
    PhSetDialogItemText(Context->PanelHandle, IDC_ZREADBYTES_V, PhaFormatSize(block->DiskReadRawDelta.Value, ULONG_MAX)->Buffer);
    PhSetDialogItemText(Context->PanelHandle, IDC_ZREADBYTESDELTA_V, PhaFormatSize(block->DiskReadRawDelta.Delta, ULONG_MAX)->Buffer);
    PhSetDialogItemText(Context->PanelHandle, IDC_ZWRITES_V, PhaFormatUInt64(block->DiskWriteCount, TRUE)->Buffer);
    PhSetDialogItemText(Context->PanelHandle, IDC_ZWRITEBYTES_V, PhaFormatSize(block->DiskWriteRawDelta.Value, ULONG_MAX)->Buffer);
    PhSetDialogItemText(Context->PanelHandle, IDC_ZWRITEBYTESDELTA_V, PhaFormatSize(block->DiskWriteRawDelta.Delta, ULONG_MAX)->Buffer);
}

VOID EtwNetworkUpdatePanel(
    _Inout_ PET_NET_CONTEXT Context
    )
{
    PET_PROCESS_BLOCK block = Context->Block;

    PhSetDialogItemText(Context->PanelHandle, IDC_ZRECEIVES_V, PhaFormatUInt64(block->NetworkReceiveCount, TRUE)->Buffer);
    PhSetDialogItemText(Context->PanelHandle, IDC_ZRECEIVEBYTES_V, PhaFormatSize(block->NetworkReceiveRawDelta.Value, ULONG_MAX)->Buffer);
    PhSetDialogItemText(Context->PanelHandle, IDC_ZRECEIVEBYTESDELTA_V, PhaFormatSize(block->NetworkReceiveRawDelta.Delta, ULONG_MAX)->Buffer);
    PhSetDialogItemText(Context->PanelHandle, IDC_ZSENDS_V, PhaFormatUInt64(block->NetworkSendCount, TRUE)->Buffer);
    PhSetDialogItemText(Context->PanelHandle, IDC_ZSENDBYTES_V, PhaFormatSize(block->NetworkSendRawDelta.Value, ULONG_MAX)->Buffer);
    PhSetDialogItemText(Context->PanelHandle, IDC_ZSENDBYTESDELTA_V, PhaFormatSize(block->NetworkSendRawDelta.Delta, ULONG_MAX)->Buffer);
}

VOID NTAPI EtwDiskUpdateHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PET_DISK_CONTEXT context = Context;

    if (context && context->WindowHandle && context->Enabled)
    {
        PostMessage(context->WindowHandle, WM_PH_UPDATE_DIALOG, 0, 0);
    }
}

VOID NTAPI EtwNetworkUpdateHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PET_NET_CONTEXT context = Context;

    if (context && context->WindowHandle && context->Enabled)
    {
        PostMessage(context->WindowHandle, WM_PH_UPDATE_DIALOG, 0, 0);
    }
}

INT_PTR CALLBACK EtwDiskPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PET_DISK_CONTEXT context;

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
            // We have already set the group boxes to have WS_EX_TRANSPARENT to fix
            // the drawing issue that arises when using WS_CLIPCHILDREN. However
            // in removing the flicker from the graphs the group boxes will now flicker.
            // It's a good tradeoff since no one stares at the group boxes.

            PhSetWindowStyle(hwndDlg, WS_CLIPCHILDREN, WS_CLIPCHILDREN);

            context = PhAllocateZero(sizeof(ET_DISK_CONTEXT));
            context->WindowHandle = hwndDlg;
            context->Block = EtGetProcessBlock(processItem);
            context->Enabled = TRUE;
            context->DiskReadGroupBox = GetDlgItem(hwndDlg, IDC_GROUPDISKREAD);
            context->DiskWriteGroupBox = GetDlgItem(hwndDlg, IDC_GROUPDISKWRITE);
            propPageContext->Context = context;

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);

            PhInitializeGraphState(&context->DiskReadGraphState);
            PhInitializeGraphState(&context->DiskWriteGraphState);

            EtwDiskUpdateWindowDpi(context);
            EtwDiskCreateGraphs(context);
            EtwDiskCreatePanel(context);
            EtwDiskUpdatePanel(context);

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                EtwDiskUpdateHandler,
                context,
                &context->ProcessesUpdatedRegistration
                );

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);

            PhDeleteGraphState(&context->DiskReadGraphState);
            PhDeleteGraphState(&context->DiskWriteGraphState);

            if (context->DiskReadGraphHandle)
                DestroyWindow(context->DiskReadGraphHandle);
            if (context->DiskWriteGraphHandle)
                DestroyWindow(context->DiskWriteGraphHandle);
            if (context->PanelHandle)
                DestroyWindow(context->PanelHandle);

            PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent), &context->ProcessesUpdatedRegistration);
            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (PhBeginPropPageLayout(hwndDlg, propPageContext))
                PhEndPropPageLayout(hwndDlg, propPageContext);
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
            EtwDiskUpdateWindowDpi(context);

            EtwDiskLayoutGraphs(context);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_SETACTIVE:
                {
                    context->Enabled = TRUE;

                    context->DiskReadGraphState.Valid = FALSE;
                    context->DiskReadGraphState.TooltipIndex = ULONG_MAX;
                    context->DiskWriteGraphState.Valid = FALSE;
                    context->DiskWriteGraphState.TooltipIndex = ULONG_MAX;

                    if (context->DiskReadGraphHandle)
                        Graph_Draw(context->DiskReadGraphHandle);
                    if (context->DiskWriteGraphHandle)
                        Graph_Draw(context->DiskWriteGraphHandle);
                }
                break;
            case PSN_KILLACTIVE:
                context->Enabled = FALSE;
                break;
            case GCN_GETDRAWINFO:
                {
                    PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)header;
                    PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

                    if (header->hwndFrom == context->DiskReadGraphHandle)
                    {
                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y;
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), 0, context->WindowDpi);
                        PhGraphStateGetDrawInfo(&context->DiskReadGraphState, getDrawInfo, context->Block->DiskReadHistory.Count);

                        if (!context->DiskReadGraphState.Valid)
                        {
                            FLOAT max = 0;

                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data;

                                context->DiskReadGraphState.Data1[i] = data = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->Block->DiskReadHistory, i);

                                if (max < data)
                                    max = data;
                            }

                            // Minimum scaling of 1 MB.
                            //if (max < 1024 * 1024)
                            //    max = 1024 * 1024;

                            if (max != 0)
                            {
                                // Scale the data.

                                PhDivideSinglesBySingle(
                                    context->DiskReadGraphState.Data1,
                                    max,
                                    drawInfo->LineDataCount
                                    );
                            }

                            drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                            drawInfo->LabelYFunctionParameter = max;

                            context->DiskReadGraphState.Valid = TRUE;
                        }

                        if (EtGraphShowText)
                        {
                            HDC hdc;
                            PH_FORMAT format[2];

                            // R: %s, W: %s
                            PhInitFormatS(&format[0], L"R: ");
                            PhInitFormatSize(&format[1], context->Block->CurrentDiskRead);

                            PhMoveReference(&context->DiskReadGraphState.Text, PhFormat(format, RTL_NUMBER_OF(format), 0));

                            hdc = Graph_GetBufferedContext(context->DiskReadGraphHandle);
                            PhSetGraphText(
                                hdc,
                                drawInfo,
                                &context->DiskReadGraphState.Text->sr,
                                &NormalGraphTextMargin,
                                &NormalGraphTextPadding,
                                PH_ALIGN_TOP | PH_ALIGN_LEFT
                                );
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }
                    }
                    else if (header->hwndFrom == context->DiskWriteGraphHandle)
                    {
                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y;
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorIoWrite"), 0, context->WindowDpi);
                        PhGraphStateGetDrawInfo(&context->DiskWriteGraphState, getDrawInfo, context->Block->DiskWriteHistory.Count);

                        if (!context->DiskWriteGraphState.Valid)
                        {
                            FLOAT max = 0;

                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data;

                                context->DiskWriteGraphState.Data1[i] = data = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->Block->DiskWriteHistory, i);

                                if (max < data)
                                    max = data;
                            }

                            // Minimum scaling of 1 MB.
                            //if (max < 1024 * 1024)
                            //    max = 1024 * 1024;

                            if (max != 0)
                            {
                                // Scale the data.

                                PhDivideSinglesBySingle(
                                    context->DiskWriteGraphState.Data1,
                                    max,
                                    drawInfo->LineDataCount
                                    );
                            }

                            drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                            drawInfo->LabelYFunctionParameter = max;

                            context->DiskWriteGraphState.Valid = TRUE;
                        }

                        if (EtGraphShowText)
                        {
                            HDC hdc;
                            PH_FORMAT format[2];

                            // R: %s, W: %s
                            PhInitFormatS(&format[0], L"W: ");
                            PhInitFormatSize(&format[1], context->Block->CurrentDiskWrite);

                            PhMoveReference(&context->DiskWriteGraphState.Text, PhFormat(format, RTL_NUMBER_OF(format), 0));

                            hdc = Graph_GetBufferedContext(context->DiskWriteGraphHandle);
                            PhSetGraphText(
                                hdc,
                                drawInfo,
                                &context->DiskWriteGraphState.Text->sr,
                                &NormalGraphTextMargin,
                                &NormalGraphTextPadding,
                                PH_ALIGN_TOP | PH_ALIGN_LEFT
                                );
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }
                    }
                }
                break;
            case GCN_GETTOOLTIPTEXT:
                {
                    PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)lParam;

                    if (getTooltipText->Index < getTooltipText->TotalCount)
                    {
                        if (header->hwndFrom == context->DiskReadGraphHandle)
                        {
                            if (context->DiskReadGraphState.TooltipIndex != getTooltipText->Index)
                            {
                                ULONG64 diskRead;
                                PH_FORMAT format[4];

                                diskRead = PhGetItemCircularBuffer_ULONG64(
                                    &context->Block->DiskReadHistory,
                                    getTooltipText->Index
                                    );

                                // R: %s\nW: %s\n%s
                                PhInitFormatS(&format[0], L"R: ");
                                PhInitFormatSize(&format[1], diskRead);
                                PhInitFormatC(&format[2], L'\n');
                                PhInitFormatSR(&format[3], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                                PhMoveReference(&context->DiskReadGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                            }

                            getTooltipText->Text = PhGetStringRef(context->DiskReadGraphState.TooltipText);
                        }
                        else if (header->hwndFrom == context->DiskWriteGraphHandle)
                        {
                            if (context->DiskWriteGraphState.TooltipIndex != getTooltipText->Index)
                            {
                                ULONG64 diskWrite;
                                PH_FORMAT format[4];

                                diskWrite = PhGetItemCircularBuffer_ULONG64(
                                    &context->Block->DiskWriteHistory,
                                    getTooltipText->Index
                                    );

                                // R: %s\nW: %s\n%s
                                PhInitFormatS(&format[0], L"W: ");
                                PhInitFormatSize(&format[1], diskWrite);
                                PhInitFormatC(&format[2], L'\n');
                                PhInitFormatSR(&format[3], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                                PhMoveReference(&context->DiskWriteGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                            }

                            getTooltipText->Text = PhGetStringRef(context->DiskWriteGraphState.TooltipText);
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_PH_UPDATE_DIALOG:
        {
            if (!(processItem->State & PH_PROCESS_ITEM_REMOVED) && context->Enabled)
            {
                EtwDiskUpdateGraphs(context);
                EtwDiskUpdatePanel(context);
            }
        }
        break;
    case WM_SIZE:
        {
            EtwDiskLayoutGraphs(context);
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK EtwNetworkPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PET_NET_CONTEXT context;

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
            // We have already set the group boxes to have WS_EX_TRANSPARENT to fix
            // the drawing issue that arises when using WS_CLIPCHILDREN. However
            // in removing the flicker from the graphs the group boxes will now flicker.
            // It's a good tradeoff since no one stares at the group boxes.

            PhSetWindowStyle(hwndDlg, WS_CLIPCHILDREN, WS_CLIPCHILDREN);

            context = PhAllocateZero(sizeof(ET_NET_CONTEXT));
            context->WindowHandle = hwndDlg;
            context->Block = EtGetProcessBlock(processItem);
            context->Enabled = TRUE;
            context->NetworkReceiveGroupBox  = GetDlgItem(hwndDlg, IDC_GROUPNETRECEIVE);
            context->NetworkSendGroupBox = GetDlgItem(hwndDlg, IDC_GROUPNETSEND);
            propPageContext->Context = context;

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);

            PhInitializeGraphState(&context->NetworkReceiveGraphState);
            PhInitializeGraphState(&context->NetworkSendGraphState);

            EtwNetworkUpdateWindowDpi(context);
            EtwNetworkCreateGraphs(context);
            EtwNetworkCreatePanel(context);
            EtwNetworkUpdatePanel(context);

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                EtwNetworkUpdateHandler,
                context,
                &context->ProcessesUpdatedRegistration
                );

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);

            PhDeleteGraphState(&context->NetworkReceiveGraphState);
            PhDeleteGraphState(&context->NetworkSendGraphState);

            if (context->NetworkReceiveGraphHandle)
                DestroyWindow(context->NetworkReceiveGraphHandle);
            if (context->NetworkSendGraphHandle)
                DestroyWindow(context->NetworkSendGraphHandle);
            if (context->PanelHandle)
                DestroyWindow(context->PanelHandle);

            PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent), &context->ProcessesUpdatedRegistration);
            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (PhBeginPropPageLayout(hwndDlg, propPageContext))
                PhEndPropPageLayout(hwndDlg, propPageContext);
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
            EtwNetworkUpdateWindowDpi(context);

            EtwNetworkLayoutGraphs(context);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_SETACTIVE:
                {
                    context->Enabled = TRUE;

                    context->NetworkReceiveGraphState.Valid = FALSE;
                    context->NetworkReceiveGraphState.TooltipIndex = ULONG_MAX;
                    context->NetworkSendGraphState.Valid = FALSE;
                    context->NetworkSendGraphState.TooltipIndex = ULONG_MAX;

                    if (context->NetworkReceiveGraphHandle)
                        Graph_Draw(context->NetworkReceiveGraphHandle);
                    if (context->NetworkSendGraphHandle)
                        Graph_Draw(context->NetworkSendGraphHandle);
                }
                break;
            case PSN_KILLACTIVE:
                context->Enabled = FALSE;
                break;
            case GCN_GETDRAWINFO:
                {
                    PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)header;
                    PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

                    if (header->hwndFrom == context->NetworkReceiveGraphHandle)
                    {
                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y;
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), 0, context->WindowDpi);
                        PhGraphStateGetDrawInfo(&context->NetworkReceiveGraphState, getDrawInfo, context->Block->NetworkReceiveHistory.Count);

                        if (!context->NetworkReceiveGraphState.Valid)
                        {
                            FLOAT max = 0;

                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data;

                                context->NetworkReceiveGraphState.Data1[i] = data = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->Block->NetworkReceiveHistory, i);

                                if (max < data)
                                    max = data;
                            }

                            // Minimum scaling of 1 MB.
                            //if (max < 1024 * 1024)
                            //    max = 1024 * 1024;

                            if (max != 0)
                            {
                                // Scale the data.

                                PhDivideSinglesBySingle(
                                    context->NetworkReceiveGraphState.Data1,
                                    max,
                                    drawInfo->LineDataCount
                                    );
                            }

                            drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                            drawInfo->LabelYFunctionParameter = max;

                            context->NetworkReceiveGraphState.Valid = TRUE;
                        }

                        if (EtGraphShowText)
                        {
                            HDC hdc;
                            PH_FORMAT format[2];

                            // R: %s, W: %s
                            PhInitFormatS(&format[0], L"R: ");
                            PhInitFormatSize(&format[1], context->Block->CurrentNetworkReceive);

                            PhMoveReference(&context->NetworkReceiveGraphState.Text, PhFormat(format, RTL_NUMBER_OF(format), 0));

                            hdc = Graph_GetBufferedContext(context->NetworkReceiveGraphHandle);
                            PhSetGraphText(
                                hdc,
                                drawInfo,
                                &context->NetworkReceiveGraphState.Text->sr,
                                &NormalGraphTextMargin,
                                &NormalGraphTextPadding,
                                PH_ALIGN_TOP | PH_ALIGN_LEFT
                                );
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }
                    }
                    else if (header->hwndFrom == context->NetworkSendGraphHandle)
                    {
                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y | PH_GRAPH_USE_LINE_2;
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorIoWrite"), 0, context->WindowDpi);
                        PhGraphStateGetDrawInfo(&context->NetworkSendGraphState, getDrawInfo, context->Block->NetworkSendHistory.Count);

                        if (!context->NetworkSendGraphState.Valid)
                        {
                            FLOAT max = 0;

                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data;

                                context->NetworkSendGraphState.Data1[i] = data = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->Block->NetworkSendHistory, i);

                                if (max < data)
                                    max = data;
                            }

                            // Minimum scaling of 1 MB.
                            //if (max < 1024 * 1024)
                            //    max = 1024 * 1024;

                            if (max != 0)
                            {
                                // Scale the data.

                                PhDivideSinglesBySingle(
                                    context->NetworkSendGraphState.Data1,
                                    max,
                                    drawInfo->LineDataCount
                                    );
                            }

                            drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                            drawInfo->LabelYFunctionParameter = max;

                            context->NetworkSendGraphState.Valid = TRUE;
                        }

                        if (EtGraphShowText)
                        {
                            HDC hdc;
                            PH_FORMAT format[2];

                            // R: %s, S: %s
                            PhInitFormatS(&format[0], L"S: ");
                            PhInitFormatSize(&format[1], context->Block->CurrentNetworkSend);

                            PhMoveReference(&context->NetworkSendGraphState.Text, PhFormat(format, RTL_NUMBER_OF(format), 0));

                            hdc = Graph_GetBufferedContext(context->NetworkSendGraphHandle);
                            PhSetGraphText(
                                hdc,
                                drawInfo,
                                &context->NetworkSendGraphState.Text->sr,
                                &NormalGraphTextMargin,
                                &NormalGraphTextPadding,
                                PH_ALIGN_TOP | PH_ALIGN_LEFT
                                );
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }
                    }
                }
                break;
            case GCN_GETTOOLTIPTEXT:
                {
                    PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)lParam;

                    if (getTooltipText->Index < getTooltipText->TotalCount)
                    {
                        if (header->hwndFrom == context->NetworkReceiveGraphHandle)
                        {
                            if (context->NetworkReceiveGraphState.TooltipIndex != getTooltipText->Index)
                            {
                                ULONG64 networkReceive;
                                PH_FORMAT format[4];

                                networkReceive = PhGetItemCircularBuffer_ULONG64(
                                    &context->Block->NetworkReceiveHistory,
                                    getTooltipText->Index
                                    );

                                // R: %s\nS: %s\n%s
                                PhInitFormatS(&format[0], L"R: ");
                                PhInitFormatSize(&format[1], networkReceive);
                                PhInitFormatC(&format[2], L'\n');
                                PhInitFormatSR(&format[3], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                                PhMoveReference(&context->NetworkReceiveGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                            }

                            getTooltipText->Text = PhGetStringRef(context->NetworkReceiveGraphState.TooltipText);
                        }
                        else if (header->hwndFrom == context->NetworkSendGraphHandle)
                        {
                            if (context->NetworkSendGraphState.TooltipIndex != getTooltipText->Index)
                            {
                                ULONG64 networkSend;
                                PH_FORMAT format[4];

                                networkSend = PhGetItemCircularBuffer_ULONG64(
                                    &context->Block->NetworkSendHistory,
                                    getTooltipText->Index
                                    );

                                // R: %s\nS: %s\n%s
                                PhInitFormatS(&format[0], L"S: ");
                                PhInitFormatSize(&format[1], networkSend);
                                PhInitFormatC(&format[2], L'\n');
                                PhInitFormatSR(&format[3], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                                PhMoveReference(&context->NetworkSendGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                            }

                            getTooltipText->Text = PhGetStringRef(context->NetworkSendGraphState.TooltipText);
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_PH_UPDATE_DIALOG:
        {
            if (!(processItem->State & PH_PROCESS_ITEM_REMOVED) && context->Enabled)
            {
                EtwNetworkUpdateGraphs(context);
                EtwNetworkUpdatePanel(context);
            }
        }
        break;
    case WM_SIZE:
        {
            EtwNetworkLayoutGraphs(context);
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

    PhAddProcessPropPage(
        propContext->PropContext,
        PhCreateProcessPropPageContextEx(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_PROCDISK), EtwDiskPageDlgProc, NULL)
        );

    PhAddProcessPropPage(
        propContext->PropContext,
        PhCreateProcessPropPageContextEx(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_PROCNET), EtwNetworkPageDlgProc, NULL)
        );
}
