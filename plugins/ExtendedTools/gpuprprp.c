/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011
 *     dmex    2015-2021
 *
 */

#include "exttools.h"

typedef struct _ET_GPU_CONTEXT
{
    HWND WindowHandle;
    //HWND PanelHandle;
    //HWND DetailsHandle;

    PET_PROCESS_BLOCK Block;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;
    BOOLEAN Enabled;
    //PH_LAYOUT_MANAGER LayoutManager;

    HWND GpuGroupBox;
    HWND MemGroupBox;
    HWND SharedGroupBox;
    HWND CommittedGroupBox;

    HWND GpuGraphHandle;
    HWND MemGraphHandle;
    HWND SharedGraphHandle;
    HWND CommittedGraphHandle;

    PH_GRAPH_STATE GpuGraphState;
    PH_GRAPH_STATE MemoryGraphState;
    PH_GRAPH_STATE MemorySharedGraphState;
    PH_GRAPH_STATE GpuCommittedGraphState;
} ET_GPU_CONTEXT, *PET_GPU_CONTEXT;

static RECT NormalGraphTextMargin = { 5, 5, 5, 5 };
static RECT NormalGraphTextPadding = { 3, 3, 3, 3 };

VOID GpuPropCreateGraphs(
    _In_ PET_GPU_CONTEXT Context
    )
{
    Context->GpuGraphHandle = CreateWindow(
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
    Graph_SetTooltip(Context->GpuGraphHandle, TRUE);

    Context->MemGraphHandle = CreateWindow(
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
    Graph_SetTooltip(Context->MemGraphHandle, TRUE);

    Context->SharedGraphHandle = CreateWindow(
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
    Graph_SetTooltip(Context->SharedGraphHandle, TRUE);

    Context->CommittedGraphHandle = CreateWindow(
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
    Graph_SetTooltip(Context->CommittedGraphHandle, TRUE);
}

VOID GpuPropCreatePanel(
    _In_ PET_GPU_CONTEXT Context
    )
{
    RECT margin;

    //Context->PanelHandle = PhCreateDialog(
    //    PluginInstance->DllBase,
    //    MAKEINTRESOURCE(IDD_PROCGPU_PANEL),
    //    Context->WindowHandle,
    //    GpuPanelDialogProc,
    //    Context
    //    );
    //
    //SetWindowPos(
    //    Context->PanelHandle,
    //    NULL,
    //    10, 0, 0, 0,
    //    SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER
    //    );
    //
    //ShowWindow(Context->PanelHandle, SW_SHOW);

    margin.left = 0;
    margin.top = 0;
    margin.right = 0;
    margin.bottom = 10;
    MapDialogRect(Context->WindowHandle, &margin);

    //PhAddLayoutItemEx(
    //    &Context->LayoutManager,
    //    Context->PanelHandle,
    //    NULL,
    //    PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT,
    //    margin
    //    );

    SendMessage(Context->WindowHandle, WM_SIZE, 0, 0);
}

VOID GpuPropLayoutGraphs(
    _In_ PET_GPU_CONTEXT Context
    )
{
    HDWP deferHandle;
    RECT clientRect;
    //RECT panelRect;
    RECT margin = { PH_SCALE_DPI(13), PH_SCALE_DPI(13), PH_SCALE_DPI(13), PH_SCALE_DPI(13) };
    RECT innerMargin = { PH_SCALE_DPI(10), PH_SCALE_DPI(20), PH_SCALE_DPI(10), PH_SCALE_DPI(10) };
    LONG between = PH_SCALE_DPI(3);
    ULONG graphWidth;
    ULONG graphHeight;

    Context->GpuGraphState.Valid = FALSE;
    Context->GpuGraphState.TooltipIndex = ULONG_MAX;
    Context->MemoryGraphState.Valid = FALSE;
    Context->MemoryGraphState.TooltipIndex = ULONG_MAX;
    Context->MemorySharedGraphState.Valid = FALSE;
    Context->MemorySharedGraphState.TooltipIndex = ULONG_MAX;
    Context->GpuCommittedGraphState.Valid = FALSE;
    Context->GpuCommittedGraphState.TooltipIndex = ULONG_MAX;

    //PhLayoutManagerLayout(&Context->LayoutManager);
    GetClientRect(Context->WindowHandle, &clientRect);

    // Limit the rectangle bottom to the top of the panel.
    //GetWindowRect(Context->PanelHandle, &panelRect);
    //MapWindowPoints(NULL, Context->WindowHandle, (PPOINT)&panelRect, 2);
    //clientRect.bottom = panelRect.top + 10; // +10 removing extra spacing

    graphWidth = clientRect.right - margin.left - margin.right;
    graphHeight = (clientRect.bottom - margin.top - margin.bottom - between * 3) / 4;

    deferHandle = BeginDeferWindowPos(8);

    deferHandle = DeferWindowPos(deferHandle, Context->GpuGroupBox, NULL, margin.left, margin.top, graphWidth, graphHeight, SWP_NOACTIVATE | SWP_NOZORDER);
    deferHandle = DeferWindowPos(
        deferHandle,
        Context->GpuGraphHandle,
        NULL,
        margin.left + innerMargin.left,
        margin.top + innerMargin.top,
        graphWidth - innerMargin.left - innerMargin.right,
        graphHeight - innerMargin.top - innerMargin.bottom,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    deferHandle = DeferWindowPos(deferHandle, Context->MemGroupBox, NULL, margin.left, margin.top + graphHeight + between, graphWidth, graphHeight, SWP_NOACTIVATE | SWP_NOZORDER);
    deferHandle = DeferWindowPos(
        deferHandle,
        Context->MemGraphHandle,
        NULL,
        margin.left + innerMargin.left,
        margin.top + graphHeight + between + innerMargin.top,
        graphWidth - innerMargin.left - innerMargin.right,
        graphHeight - innerMargin.top - innerMargin.bottom,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    deferHandle = DeferWindowPos(deferHandle, Context->SharedGroupBox, NULL, margin.left, margin.top + (graphHeight + between) * 2, graphWidth, graphHeight, SWP_NOACTIVATE | SWP_NOZORDER);
    deferHandle = DeferWindowPos(
        deferHandle,
        Context->SharedGraphHandle,
        NULL,
        margin.left + innerMargin.left,
        margin.top + (graphHeight + between) * 2 + innerMargin.top,
        graphWidth - innerMargin.left - innerMargin.right,
        graphHeight - innerMargin.top - innerMargin.bottom,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    deferHandle = DeferWindowPos(deferHandle, Context->CommittedGroupBox, NULL, margin.left, margin.top + (graphHeight + between) * 3, graphWidth, graphHeight, SWP_NOACTIVATE | SWP_NOZORDER);
    deferHandle = DeferWindowPos(
        deferHandle,
        Context->CommittedGraphHandle,
        NULL,
        margin.left + innerMargin.left,
        margin.top + (graphHeight + between) * 3 + innerMargin.top,
        graphWidth - innerMargin.left - innerMargin.right,
        graphHeight - innerMargin.top - innerMargin.bottom,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    EndDeferWindowPos(deferHandle);
}

VOID GpuPropUpdateGraphs(
    _In_ PET_GPU_CONTEXT Context
    )
{
    Context->GpuGraphState.Valid = FALSE;
    Context->GpuGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->GpuGraphHandle, 1);
    Graph_Draw(Context->GpuGraphHandle);
    Graph_UpdateTooltip(Context->GpuGraphHandle);
    InvalidateRect(Context->GpuGraphHandle, NULL, FALSE);

    Context->MemoryGraphState.Valid = FALSE;
    Context->MemoryGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->MemGraphHandle, 1);
    Graph_Draw(Context->MemGraphHandle);
    Graph_UpdateTooltip(Context->MemGraphHandle);
    InvalidateRect(Context->MemGraphHandle, NULL, FALSE);

    Context->MemorySharedGraphState.Valid = FALSE;
    Context->MemorySharedGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->SharedGraphHandle, 1);
    Graph_Draw(Context->SharedGraphHandle);
    Graph_UpdateTooltip(Context->SharedGraphHandle);
    InvalidateRect(Context->SharedGraphHandle, NULL, FALSE);

    Context->GpuCommittedGraphState.Valid = FALSE;
    Context->GpuCommittedGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->CommittedGraphHandle, 1);
    Graph_Draw(Context->CommittedGraphHandle);
    Graph_UpdateTooltip(Context->CommittedGraphHandle);
    InvalidateRect(Context->CommittedGraphHandle, NULL, FALSE);
}

//VOID GpuPropUpdatePanel(
//    _Inout_ PET_GPU_CONTEXT Context
//    )
//{
//    PET_PROCESS_BLOCK block = Context->Block;
//    WCHAR runningTimeString[PH_TIMESPAN_STR_LEN_1] = L"N/A";
//
//    PhPrintTimeSpan(runningTimeString, block->GpuRunningTimeDelta.Value * 10, PH_TIMESPAN_HMSM);
//    PhSetDialogItemText(Context->PanelHandle, IDC_ZRUNNINGTIME_V, runningTimeString);
//    PhSetDialogItemText(Context->PanelHandle, IDC_ZCONTEXTSWITCHES_V, PhaFormatUInt64(block->GpuContextSwitches, TRUE)->Buffer);
//
//    if (Context->DetailsHandle)
//    {
//        ET_PROCESS_GPU_STATISTICS processGpuStatistics;
//    
//        if (Context->Block->ProcessItem->QueryHandle)
//            EtQueryProcessGpuStatistics(Context->Block->ProcessItem->QueryHandle, &processGpuStatistics);
//        else
//            memset(&processGpuStatistics, 0, sizeof(ET_PROCESS_GPU_STATISTICS));
//    
//        PhSetDialogItemText(Context->DetailsHandle, IDC_ZDEDICATEDCOMMITTED_V, PhaFormatSize(processGpuStatistics.DedicatedCommitted, ULONG_MAX)->Buffer);
//        PhSetDialogItemText(Context->DetailsHandle, IDC_ZSHAREDCOMMITTED_V, PhaFormatSize(processGpuStatistics.SharedCommitted, ULONG_MAX)->Buffer);
//        PhSetDialogItemText(Context->DetailsHandle, IDC_ZTOTALALLOCATED_V, PhaFormatSize(processGpuStatistics.BytesAllocated, ULONG_MAX)->Buffer);
//        PhSetDialogItemText(Context->DetailsHandle, IDC_ZTOTALRESERVED_V, PhaFormatSize(processGpuStatistics.BytesReserved, ULONG_MAX)->Buffer);
//        PhSetDialogItemText(Context->DetailsHandle, IDC_ZWRITECOMBINEDALLOCATED_V, PhaFormatSize(processGpuStatistics.WriteCombinedBytesAllocated, ULONG_MAX)->Buffer);
//        PhSetDialogItemText(Context->DetailsHandle, IDC_ZWRITECOMBINEDRESERVED_V, PhaFormatSize(processGpuStatistics.WriteCombinedBytesReserved, ULONG_MAX)->Buffer);
//        PhSetDialogItemText(Context->DetailsHandle, IDC_ZCACHEDALLOCATED_V, PhaFormatSize(processGpuStatistics.CachedBytesAllocated, ULONG_MAX)->Buffer);
//        PhSetDialogItemText(Context->DetailsHandle, IDC_ZCACHEDRESERVED_V, PhaFormatSize(processGpuStatistics.CachedBytesReserved, ULONG_MAX)->Buffer);
//        PhSetDialogItemText(Context->DetailsHandle, IDC_ZSECTIONALLOCATED_V, PhaFormatSize(processGpuStatistics.SectionBytesAllocated, ULONG_MAX)->Buffer);
//        PhSetDialogItemText(Context->DetailsHandle, IDC_ZSECTIONRESERVED_V, PhaFormatSize(processGpuStatistics.SectionBytesReserved, ULONG_MAX)->Buffer);
//    }
//}

VOID NTAPI ProcessesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PET_GPU_CONTEXT context = Context;

    if (context && context->WindowHandle && context->Enabled)
    {
        PostMessage(context->WindowHandle, WM_PH_UPDATE_DIALOG, 0, 0);
    }
}

INT_PTR CALLBACK EtpGpuPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PET_GPU_CONTEXT context;

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

            context = PhAllocateZero(sizeof(ET_GPU_CONTEXT));
            context->WindowHandle = hwndDlg;
            context->Block = EtGetProcessBlock(processItem);
            context->Enabled = TRUE;
            context->GpuGroupBox = GetDlgItem(hwndDlg, IDC_GROUPGPU);
            context->MemGroupBox = GetDlgItem(hwndDlg, IDC_GROUPMEM);
            context->SharedGroupBox = GetDlgItem(hwndDlg, IDC_GROUPSHARED);
            context->CommittedGroupBox = GetDlgItem(hwndDlg, IDC_GROUPCOMMIT);
            propPageContext->Context = context;

            //PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);

            PhInitializeGraphState(&context->GpuGraphState);
            PhInitializeGraphState(&context->MemoryGraphState);
            PhInitializeGraphState(&context->MemorySharedGraphState);
            PhInitializeGraphState(&context->GpuCommittedGraphState);

            GpuPropCreateGraphs(context);
            GpuPropCreatePanel(context);

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                ProcessesUpdatedHandler,
                context,
                &context->ProcessesUpdatedRegistration
                );

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            //PhDeleteLayoutManager(&context->LayoutManager);

            PhDeleteGraphState(&context->GpuGraphState);
            PhDeleteGraphState(&context->MemoryGraphState);
            PhDeleteGraphState(&context->MemorySharedGraphState);
            PhDeleteGraphState(&context->GpuCommittedGraphState);

            if (context->GpuGraphHandle)
                DestroyWindow(context->GpuGraphHandle);
            if (context->MemGraphHandle)
                DestroyWindow(context->MemGraphHandle);
            if (context->SharedGraphHandle)
                DestroyWindow(context->SharedGraphHandle);
            if (context->CommittedGraphHandle)
                DestroyWindow(context->CommittedGraphHandle);

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
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_SETACTIVE:
                {
                    context->Enabled = TRUE;

                    context->GpuGraphState.Valid = FALSE;
                    context->GpuGraphState.TooltipIndex = ULONG_MAX;
                    context->MemoryGraphState.Valid = FALSE;
                    context->MemoryGraphState.TooltipIndex = ULONG_MAX;
                    context->MemorySharedGraphState.Valid = FALSE;
                    context->MemorySharedGraphState.TooltipIndex = ULONG_MAX;
                    context->GpuCommittedGraphState.Valid = FALSE;
                    context->GpuCommittedGraphState.TooltipIndex = ULONG_MAX;

                    if (context->GpuGraphHandle)
                        Graph_Draw(context->GpuGraphHandle);
                    if (context->MemGraphHandle)
                        Graph_Draw(context->MemGraphHandle);
                    if (context->SharedGraphHandle)
                        Graph_Draw(context->SharedGraphHandle);
                    if (context->CommittedGraphHandle)
                        Graph_Draw(context->CommittedGraphHandle);
                }
                break;
            case PSN_KILLACTIVE:
                context->Enabled = FALSE;
                break;
            case GCN_GETDRAWINFO:
                {
                    PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)header;
                    PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

                    if (header->hwndFrom == context->GpuGraphHandle)
                    {
                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (EtEnableScaleGraph ? PH_GRAPH_LABEL_MAX_Y : 0);
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), 0);
                        PhGraphStateGetDrawInfo(&context->GpuGraphState, getDrawInfo, context->Block->GpuHistory.Count);

                        if (!context->GpuGraphState.Valid)
                        {
                            PhCopyCircularBuffer_FLOAT(&context->Block->GpuHistory, context->GpuGraphState.Data1, drawInfo->LineDataCount);

                            if (EtEnableScaleGraph)
                            {
                                FLOAT max = 0;

                                for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                                {
                                    FLOAT data = context->GpuGraphState.Data1[i]; // HACK

                                    if (max < data)
                                        max = data;
                                }

                                if (max != 0)
                                {
                                    PhDivideSinglesBySingle(context->GpuGraphState.Data1, max, drawInfo->LineDataCount);
                                }

                                drawInfo->LabelYFunction = PhSiDoubleLabelYFunction;
                                drawInfo->LabelYFunctionParameter = max;
                            }

                            context->GpuGraphState.Valid = TRUE;
                        }

                        if (EtGraphShowText)
                        {
                            HDC hdc;
                            PH_FORMAT format[2];

                            // %.2f%%
                            PhInitFormatF(&format[0], context->Block->CurrentGpuUsage * 100, 2);
                            PhInitFormatC(&format[1], L'%');

                            PhMoveReference(&context->GpuGraphState.Text, PhFormat(format, RTL_NUMBER_OF(format), 0));

                            hdc = Graph_GetBufferedContext(context->GpuGraphHandle);
                            PhSetGraphText(
                                hdc,
                                drawInfo,
                                &context->GpuGraphState.Text->sr,
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
                    else if (header->hwndFrom == context->MemGraphHandle)
                    {
                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (EtEnableScaleGraph ? PH_GRAPH_LABEL_MAX_Y : 0);
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorPhysical"), 0);
                        PhGraphStateGetDrawInfo(&context->MemoryGraphState, getDrawInfo, context->Block->MemoryHistory.Count);

                        if (!context->MemoryGraphState.Valid)
                        {
                            FLOAT max = 0;

                            if (EtGpuDedicatedLimit != 0 && !EtEnableScaleGraph)
                            {
                                max = (FLOAT)EtGpuDedicatedLimit / PAGE_SIZE;
                            }

                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data1;

                                context->MemoryGraphState.Data1[i] = data1 = (FLOAT)PhGetItemCircularBuffer_ULONG(&context->Block->MemoryHistory, i);

                                if (max < data1)
                                    max = data1;
                            }

                            if (max != 0)
                            {
                                PhDivideSinglesBySingle(context->MemoryGraphState.Data1, max, drawInfo->LineDataCount);
                            }

                            if (EtEnableScaleGraph)
                            {
                                drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                                drawInfo->LabelYFunctionParameter = max * PAGE_SIZE;
                            }

                            context->MemoryGraphState.Valid = TRUE;
                        }
                        
                        if (EtGraphShowText)
                        {
                            HDC hdc;

                            PhMoveReference(&context->MemoryGraphState.Text, PhFormatSize(
                                UInt32x32To64(context->Block->CurrentMemUsage, PAGE_SIZE), ULONG_MAX));

                            hdc = Graph_GetBufferedContext(context->MemGraphHandle);
                            PhSetGraphText(
                                hdc,
                                drawInfo,
                                &context->MemoryGraphState.Text->sr,
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
                    else if (header->hwndFrom == context->SharedGraphHandle)
                    {
                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (EtEnableScaleGraph ? PH_GRAPH_LABEL_MAX_Y : 0);
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorIoWrite"), 0);
                        PhGraphStateGetDrawInfo(&context->MemorySharedGraphState, getDrawInfo, context->Block->MemorySharedHistory.Count);

                        if (!context->MemorySharedGraphState.Valid)
                        {
                            FLOAT max = 0;

                            if (EtGpuSharedLimit != 0 && !EtEnableScaleGraph)
                            {
                                max = (FLOAT)EtGpuSharedLimit / PAGE_SIZE;
                            }

                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data;

                                context->MemorySharedGraphState.Data1[i] = data = (FLOAT)PhGetItemCircularBuffer_ULONG(&context->Block->MemorySharedHistory, i);

                                if (max < data)
                                    max = data;
                            }

                            if (max != 0)
                            {
                                PhDivideSinglesBySingle(context->MemorySharedGraphState.Data1, max, drawInfo->LineDataCount);
                            }

                            if (EtEnableScaleGraph)
                            {
                                drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                                drawInfo->LabelYFunctionParameter = max * PAGE_SIZE;
                            }

                            context->MemorySharedGraphState.Valid = TRUE;
                        }
                        
                        if (EtGraphShowText)
                        {
                            HDC hdc;

                            PhMoveReference(&context->MemorySharedGraphState.Text, PhFormatSize(
                                UInt32x32To64(context->Block->CurrentMemSharedUsage, PAGE_SIZE), ULONG_MAX));

                            hdc = Graph_GetBufferedContext(context->SharedGraphHandle);
                            PhSetGraphText(
                                hdc,
                                drawInfo,
                                &context->MemorySharedGraphState.Text->sr,
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
                    else if (header->hwndFrom == context->CommittedGraphHandle)
                    {
                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (EtEnableScaleGraph ? PH_GRAPH_LABEL_MAX_Y : 0);
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorPrivate"), 0);
                        PhGraphStateGetDrawInfo(&context->GpuCommittedGraphState, getDrawInfo, context->Block->GpuCommittedHistory.Count);

                        if (!context->GpuCommittedGraphState.Valid)
                        {
                            ULONG i;
                            FLOAT max = 0;

                            if (!EtEnableScaleGraph)
                            {
                                max = 1024 * 1024; // minimum scaling
                            }

                            for (i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data1;

                                context->GpuCommittedGraphState.Data1[i] = data1 = (FLOAT)PhGetItemCircularBuffer_ULONG(&context->Block->GpuCommittedHistory, i);

                                if (max < data1)
                                    max = data1;
                            }

                            if (max != 0)
                            {
                                PhDivideSinglesBySingle(context->GpuCommittedGraphState.Data1, max, drawInfo->LineDataCount);
                            }

                            if (EtEnableScaleGraph)
                            {
                                drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                                drawInfo->LabelYFunctionParameter = max * PAGE_SIZE;
                            }

                            context->GpuCommittedGraphState.Valid = TRUE;
                        }

                        if (EtGraphShowText)
                        {
                            HDC hdc;

                            PhMoveReference(&context->GpuCommittedGraphState.Text, PhFormatSize(
                                UInt32x32To64(context->Block->CurrentCommitUsage, PAGE_SIZE), ULONG_MAX));

                            hdc = Graph_GetBufferedContext(context->CommittedGraphHandle);
                            PhSetGraphText(
                                hdc,
                                drawInfo,
                                &context->GpuCommittedGraphState.Text->sr,
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
                        if (header->hwndFrom == context->GpuGraphHandle)
                        {
                            if (context->GpuGraphState.TooltipIndex != getTooltipText->Index)
                            {
                                FLOAT gpuUsage;
                                PH_FORMAT format[3];

                                gpuUsage = PhGetItemCircularBuffer_FLOAT(
                                    &context->Block->GpuHistory,
                                    getTooltipText->Index
                                    );

                                // %.2f%%\n%s
                                PhInitFormatF(&format[0], gpuUsage * 100, 2);
                                PhInitFormatS(&format[1], L"%\n");
                                PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                                PhMoveReference(&context->GpuGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                            }

                            getTooltipText->Text = PhGetStringRef(context->GpuGraphState.TooltipText);
                        }
                        else if (header->hwndFrom == context->MemGraphHandle)
                        {
                            if (context->MemoryGraphState.TooltipIndex != getTooltipText->Index)
                            {
                                ULONG gpuMemory;
                                PH_FORMAT format[3];

                                gpuMemory = PhGetItemCircularBuffer_ULONG(
                                    &context->Block->MemoryHistory,
                                    getTooltipText->Index
                                    );

                                // %s\n%s
                                PhInitFormatSize(&format[0], UInt32x32To64(gpuMemory, PAGE_SIZE));
                                PhInitFormatC(&format[1], L'\n');
                                PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                                PhMoveReference(&context->MemoryGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                            }

                            getTooltipText->Text = PhGetStringRef(context->MemoryGraphState.TooltipText);
                        }
                        else if (header->hwndFrom == context->SharedGraphHandle)
                        {
                            if (context->MemorySharedGraphState.TooltipIndex != getTooltipText->Index)
                            {
                                ULONG gpuSharedMemory;
                                PH_FORMAT format[3];

                                gpuSharedMemory = PhGetItemCircularBuffer_ULONG(
                                    &context->Block->MemorySharedHistory,
                                    getTooltipText->Index
                                    );

                                // %s\n%s
                                PhInitFormatSize(&format[0], UInt32x32To64(gpuSharedMemory, PAGE_SIZE));
                                PhInitFormatC(&format[1], L'\n');
                                PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                                PhMoveReference(&context->MemorySharedGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                            }

                            getTooltipText->Text = PhGetStringRef(context->MemorySharedGraphState.TooltipText);
                        }
                        else if (header->hwndFrom == context->CommittedGraphHandle)
                        {
                            if (context->GpuCommittedGraphState.TooltipIndex != getTooltipText->Index)
                            {
                                ULONG gpuCommitMemory;
                                PH_FORMAT format[3];

                                gpuCommitMemory = PhGetItemCircularBuffer_ULONG(
                                    &context->Block->GpuCommittedHistory,
                                    getTooltipText->Index
                                    );

                                // %s\n%s
                                PhInitFormatSize(&format[0], UInt32x32To64(gpuCommitMemory, PAGE_SIZE));
                                PhInitFormatC(&format[1], L'\n');
                                PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                                PhMoveReference(&context->GpuCommittedGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                            }

                            getTooltipText->Text = PhGetStringRef(context->GpuCommittedGraphState.TooltipText);
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
                GpuPropUpdateGraphs(context);
                //GpuPropUpdatePanel(context);
            }
        }
        break;
    case WM_SIZE:
        {
            GpuPropLayoutGraphs(context);
        }
        break;
    }

    return FALSE;
}

VOID EtProcessGpuPropertiesInitializing(
    _In_ PVOID Parameter
    )
{
    PPH_PLUGIN_PROCESS_PROPCONTEXT propContext = Parameter;

    if (EtGpuEnabled)
    {
        PhAddProcessPropPage(
            propContext->PropContext,
            PhCreateProcessPropPageContextEx(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_PROCGPU), EtpGpuPageDlgProc, NULL)
            );
    }
}
