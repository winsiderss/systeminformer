/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2021-2022
 *
 */

#include "exttools.h"

typedef struct _ET_FRAMES_CONTEXT
{
    HWND WindowHandle;

    PET_PROCESS_BLOCK Block;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;
    BOOLEAN Enabled;

    HWND FramesPerSecondGroupBox;
    HWND FramesLatencyGroupBox;
    HWND PresentIntervalGroupBox;
    HWND PresentDurationGroupBox;
    HWND FramesRenderTimeGroupBox;
    HWND FramesDisplayTimeGroupBox;
    HWND FramesDisplayLatencyGroupBox;

    HWND FramesPerSecondGraphHandle;
    HWND FramesLatencyGraphHandle;
    HWND PresentIntervalGraphHandle;
    HWND PresentDurationGraphHandle;
    HWND FramesRenderTimeGraphHandle;
    HWND FramesDisplayTimeGraphHandle;
    HWND FramesDisplayLatencyGraphHandle;

    PH_GRAPH_STATE FramesPerSecondGraphState;
    PH_GRAPH_STATE FramesLatencyGraphState;
    PH_GRAPH_STATE PresentIntervalGraphState;
    PH_GRAPH_STATE PresentDurationGraphState;
    PH_GRAPH_STATE FramesRenderTimeGraphState;
    PH_GRAPH_STATE FramesDisplayTimeGraphState;
    PH_GRAPH_STATE FramesDisplayLatencyGraphState;
} ET_FRAMES_CONTEXT, *PET_FRAMES_CONTEXT;

static RECT NormalGraphTextMargin = { 5, 5, 5, 5 };
static RECT NormalGraphTextPadding = { 3, 3, 3, 3 };

PPH_STRING FramesLabelYFunction(
    _In_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ ULONG DataIndex,
    _In_ FLOAT Value,
    _In_ FLOAT Parameter
    )
{
    DOUBLE value;

    value = (DOUBLE)((DOUBLE)Value * Parameter);

    if (value != 0)
    {
        PH_FORMAT format[1];

        PhInitFormatF(&format[0], value, 2);

        return PhFormat(format, RTL_NUMBER_OF(format), 0);
    }
    else
    {
        return PhReferenceEmptyString();
    }
}

VOID FramesPropCreateGraphs(
    _In_ PET_FRAMES_CONTEXT Context
    )
{
    Context->FramesPerSecondGraphHandle = CreateWindow(
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
    Graph_SetTooltip(Context->FramesPerSecondGraphHandle, TRUE);

    Context->FramesLatencyGraphHandle = CreateWindow(
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
    Graph_SetTooltip(Context->FramesLatencyGraphHandle, TRUE);

    Context->PresentIntervalGraphHandle = CreateWindow(
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
    Graph_SetTooltip(Context->PresentIntervalGraphHandle, TRUE);

    Context->PresentDurationGraphHandle = CreateWindow(
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
    Graph_SetTooltip(Context->PresentDurationGraphHandle, TRUE);

    Context->FramesRenderTimeGraphHandle = CreateWindow(
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
    Graph_SetTooltip(Context->FramesRenderTimeGraphHandle, TRUE);

    Context->FramesDisplayTimeGraphHandle = CreateWindow(
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
    Graph_SetTooltip(Context->FramesDisplayTimeGraphHandle, TRUE);

    Context->FramesDisplayLatencyGraphHandle = CreateWindow(
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
    Graph_SetTooltip(Context->FramesDisplayLatencyGraphHandle, TRUE);
}

VOID FramesPropCreatePanel(
    _In_ PET_FRAMES_CONTEXT Context
    )
{
    RECT margin;

    margin.left = 0;
    margin.top = 0;
    margin.right = 0;
    margin.bottom = 10;
    MapDialogRect(Context->WindowHandle, &margin);

    SendMessage(Context->WindowHandle, WM_SIZE, 0, 0);
}

VOID FramesPropLayoutGraphs(
    _In_ PET_FRAMES_CONTEXT Context
    )
{
    HDWP deferHandle;
    RECT clientRect;
    //RECT panelRect;
    RECT margin;
    RECT innerMargin;
    LONG between;
    ULONG graphWidth;
    ULONG graphHeight;
    LONG dpiValue;

    Context->FramesPerSecondGraphState.Valid = FALSE;
    Context->FramesPerSecondGraphState.TooltipIndex = ULONG_MAX;
    Context->FramesLatencyGraphState.Valid = FALSE;
    Context->FramesLatencyGraphState.TooltipIndex = ULONG_MAX;
    Context->PresentIntervalGraphState.Valid = FALSE;
    Context->PresentIntervalGraphState.TooltipIndex = ULONG_MAX;
    Context->PresentDurationGraphState.Valid = FALSE;
    Context->PresentDurationGraphState.TooltipIndex = ULONG_MAX;
    Context->FramesRenderTimeGraphState.Valid = FALSE;
    Context->FramesRenderTimeGraphState.TooltipIndex = ULONG_MAX;
    Context->FramesDisplayTimeGraphState.Valid = FALSE;
    Context->FramesDisplayTimeGraphState.TooltipIndex = ULONG_MAX;
    Context->FramesDisplayLatencyGraphState.Valid = FALSE;
    Context->FramesDisplayLatencyGraphState.TooltipIndex = ULONG_MAX;

    dpiValue = PhGetWindowDpi(Context->WindowHandle);

    margin.left = margin.top = margin.right = margin.bottom = PhGetDpi(13, dpiValue);

    innerMargin.left = innerMargin.right = innerMargin.bottom = PhGetDpi(10, dpiValue);
    innerMargin.top = PhGetDpi(20, dpiValue);

    between = PhGetDpi(3, dpiValue);

    GetClientRect(Context->WindowHandle, &clientRect);
    graphWidth = clientRect.right - margin.left - margin.right;
    graphHeight = (clientRect.bottom - margin.top - margin.bottom - between * 6) / 7;

    deferHandle = BeginDeferWindowPos(14);

    deferHandle = DeferWindowPos(deferHandle, Context->FramesPerSecondGroupBox, NULL, margin.left, margin.top, graphWidth, graphHeight, SWP_NOACTIVATE | SWP_NOZORDER);
    deferHandle = DeferWindowPos(
        deferHandle,
        Context->FramesPerSecondGraphHandle,
        NULL,
        margin.left + innerMargin.left,
        margin.top + innerMargin.top,
        graphWidth - innerMargin.left - innerMargin.right,
        graphHeight - innerMargin.top - innerMargin.bottom,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    deferHandle = DeferWindowPos(deferHandle, Context->FramesLatencyGroupBox, NULL, margin.left, margin.top + graphHeight + between, graphWidth, graphHeight, SWP_NOACTIVATE | SWP_NOZORDER);
    deferHandle = DeferWindowPos(
        deferHandle,
        Context->FramesLatencyGraphHandle,
        NULL,
        margin.left + innerMargin.left,
        margin.top + graphHeight + between + innerMargin.top,
        graphWidth - innerMargin.left - innerMargin.right,
        graphHeight - innerMargin.top - innerMargin.bottom,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    deferHandle = DeferWindowPos(deferHandle, Context->PresentIntervalGroupBox, NULL, margin.left, margin.top + (graphHeight + between) * 2, graphWidth, graphHeight, SWP_NOACTIVATE | SWP_NOZORDER);
    deferHandle = DeferWindowPos(
        deferHandle,
        Context->PresentIntervalGraphHandle,
        NULL,
        margin.left + innerMargin.left,
        margin.top + (graphHeight + between) * 2 + innerMargin.top,
        graphWidth - innerMargin.left - innerMargin.right,
        graphHeight - innerMargin.top - innerMargin.bottom,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    deferHandle = DeferWindowPos(deferHandle, Context->PresentDurationGroupBox, NULL, margin.left, margin.top + (graphHeight + between) * 3, graphWidth, graphHeight, SWP_NOACTIVATE | SWP_NOZORDER);
    deferHandle = DeferWindowPos(
        deferHandle,
        Context->PresentDurationGraphHandle,
        NULL,
        margin.left + innerMargin.left,
        margin.top + (graphHeight + between) * 3 + innerMargin.top,
        graphWidth - innerMargin.left - innerMargin.right,
        graphHeight - innerMargin.top - innerMargin.bottom,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    deferHandle = DeferWindowPos(deferHandle, Context->FramesRenderTimeGroupBox, NULL, margin.left, margin.top + (graphHeight + between) * 4, graphWidth, graphHeight, SWP_NOACTIVATE | SWP_NOZORDER);
    deferHandle = DeferWindowPos(
        deferHandle,
        Context->FramesRenderTimeGraphHandle,
        NULL,
        margin.left + innerMargin.left,
        margin.top + (graphHeight + between) * 4 + innerMargin.top,
        graphWidth - innerMargin.left - innerMargin.right,
        graphHeight - innerMargin.top - innerMargin.bottom,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    deferHandle = DeferWindowPos(deferHandle, Context->FramesDisplayTimeGroupBox, NULL, margin.left, margin.top + (graphHeight + between) * 5, graphWidth, graphHeight, SWP_NOACTIVATE | SWP_NOZORDER);
    deferHandle = DeferWindowPos(
        deferHandle,
        Context->FramesDisplayTimeGraphHandle,
        NULL,
        margin.left + innerMargin.left,
        margin.top + (graphHeight + between) * 5 + innerMargin.top,
        graphWidth - innerMargin.left - innerMargin.right,
        graphHeight - innerMargin.top - innerMargin.bottom,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    deferHandle = DeferWindowPos(deferHandle, Context->FramesDisplayLatencyGroupBox, NULL, margin.left, margin.top + (graphHeight + between) * 6, graphWidth, graphHeight, SWP_NOACTIVATE | SWP_NOZORDER);
    deferHandle = DeferWindowPos(
        deferHandle,
        Context->FramesDisplayLatencyGraphHandle,
        NULL,
        margin.left + innerMargin.left,
        margin.top + (graphHeight + between) * 6 + innerMargin.top,
        graphWidth - innerMargin.left - innerMargin.right,
        graphHeight - innerMargin.top - innerMargin.bottom,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    EndDeferWindowPos(deferHandle);
}

VOID FramesPropUpdateGraphs(
    _In_ PET_FRAMES_CONTEXT Context
    )
{
    Context->FramesPerSecondGraphState.Valid = FALSE;
    Context->FramesPerSecondGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->FramesPerSecondGraphHandle, 1);
    Graph_Draw(Context->FramesPerSecondGraphHandle);
    Graph_UpdateTooltip(Context->FramesPerSecondGraphHandle);
    InvalidateRect(Context->FramesPerSecondGraphHandle, NULL, FALSE);

    Context->FramesLatencyGraphState.Valid = FALSE;
    Context->FramesLatencyGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->FramesLatencyGraphHandle, 1);
    Graph_Draw(Context->FramesLatencyGraphHandle);
    Graph_UpdateTooltip(Context->FramesLatencyGraphHandle);
    InvalidateRect(Context->FramesLatencyGraphHandle, NULL, FALSE);

    Context->PresentIntervalGraphState.Valid = FALSE;
    Context->PresentIntervalGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->PresentIntervalGraphHandle, 1);
    Graph_Draw(Context->PresentIntervalGraphHandle);
    Graph_UpdateTooltip(Context->PresentIntervalGraphHandle);
    InvalidateRect(Context->PresentIntervalGraphHandle, NULL, FALSE);

    Context->PresentDurationGraphState.Valid = FALSE;
    Context->PresentDurationGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->PresentDurationGraphHandle, 1);
    Graph_Draw(Context->PresentDurationGraphHandle);
    Graph_UpdateTooltip(Context->PresentDurationGraphHandle);
    InvalidateRect(Context->PresentDurationGraphHandle, NULL, FALSE);

    Context->FramesRenderTimeGraphState.Valid = FALSE;
    Context->FramesRenderTimeGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->FramesRenderTimeGraphHandle, 1);
    Graph_Draw(Context->FramesRenderTimeGraphHandle);
    Graph_UpdateTooltip(Context->FramesRenderTimeGraphHandle);
    InvalidateRect(Context->FramesRenderTimeGraphHandle, NULL, FALSE);

    Context->FramesDisplayTimeGraphState.Valid = FALSE;
    Context->FramesDisplayTimeGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->FramesDisplayTimeGraphHandle, 1);
    Graph_Draw(Context->FramesDisplayTimeGraphHandle);
    Graph_UpdateTooltip(Context->FramesDisplayTimeGraphHandle);
    InvalidateRect(Context->FramesDisplayTimeGraphHandle, NULL, FALSE);

    Context->FramesDisplayLatencyGraphState.Valid = FALSE;
    Context->FramesDisplayLatencyGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->FramesDisplayLatencyGraphHandle, 1);
    Graph_Draw(Context->FramesDisplayLatencyGraphHandle);
    Graph_UpdateTooltip(Context->FramesDisplayLatencyGraphHandle);
    InvalidateRect(Context->FramesDisplayLatencyGraphHandle, NULL, FALSE);
}

VOID FramesPropUpdatePanel(
    _In_ PET_FRAMES_CONTEXT Context
    )
{
    PCWSTR runtime;
    PCWSTR presentMode;
    PPH_STRING string;
    PH_FORMAT format[5];

    runtime = EtRuntimeToString(Context->Block->FramesRuntime);
    presentMode = EtPresentModeToString(Context->Block->FramesPresentMode);

    PhInitFormatS(&format[0], L"FPS: ");
    PhInitFormatS(&format[1], (PWSTR)presentMode);
    PhInitFormatS(&format[2], L" (");
    PhInitFormatS(&format[3], (PWSTR)runtime);
    PhInitFormatC(&format[4], L')');

    string = PhFormat(format, RTL_NUMBER_OF(format), 0);
    PhSetWindowText(Context->FramesPerSecondGroupBox, PhGetString(string));
    PhDereferenceObject(string);
}

VOID NTAPI FramesProcessesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PET_FRAMES_CONTEXT context = Context;

    if (context && context->WindowHandle && context->Enabled)
    {
        PostMessage(context->WindowHandle, WM_PH_UPDATE_DIALOG, 0, 0);
    }
}

INT_PTR CALLBACK EtpFramesPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PET_FRAMES_CONTEXT context;

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
            PhSetWindowStyle(hwndDlg, WS_CLIPCHILDREN, WS_CLIPCHILDREN);

            context = PhAllocateZero(sizeof(ET_FRAMES_CONTEXT));
            context->WindowHandle = hwndDlg;
            context->Block = EtGetProcessBlock(processItem);
            context->Enabled = TRUE;
            context->FramesPerSecondGroupBox = GetDlgItem(hwndDlg, IDC_GROUPFPS);
            context->FramesLatencyGroupBox = GetDlgItem(hwndDlg, IDC_GROUPFPSLATENCY);
            context->PresentIntervalGroupBox = GetDlgItem(hwndDlg, IDC_GROUPFPSPRESENT);
            context->PresentDurationGroupBox = GetDlgItem(hwndDlg, IDC_GROUPFPSPRESENTINTERVAL);
            context->FramesRenderTimeGroupBox = GetDlgItem(hwndDlg, IDC_GROUPFPSFRAMERENDER);
            context->FramesDisplayTimeGroupBox = GetDlgItem(hwndDlg, IDC_GROUPFPSFRAMEDISPLAY);
            context->FramesDisplayLatencyGroupBox = GetDlgItem(hwndDlg, IDC_GROUPFPSDISPLAYLATENCY);
            propPageContext->Context = context;

            PhInitializeGraphState(&context->FramesPerSecondGraphState);
            PhInitializeGraphState(&context->FramesLatencyGraphState);
            PhInitializeGraphState(&context->PresentIntervalGraphState);
            PhInitializeGraphState(&context->PresentDurationGraphState);
            PhInitializeGraphState(&context->FramesRenderTimeGraphState);
            PhInitializeGraphState(&context->FramesDisplayTimeGraphState);
            PhInitializeGraphState(&context->FramesDisplayLatencyGraphState);

            FramesPropCreateGraphs(context);
            FramesPropCreatePanel(context);

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                FramesProcessesUpdatedHandler,
                context,
                &context->ProcessesUpdatedRegistration
                );

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent), &context->ProcessesUpdatedRegistration);

            PhDeleteGraphState(&context->FramesPerSecondGraphState);
            PhDeleteGraphState(&context->FramesLatencyGraphState);
            PhDeleteGraphState(&context->PresentIntervalGraphState);
            PhDeleteGraphState(&context->PresentDurationGraphState);
            PhDeleteGraphState(&context->FramesRenderTimeGraphState);
            PhDeleteGraphState(&context->FramesDisplayTimeGraphState);
            PhDeleteGraphState(&context->FramesDisplayLatencyGraphState);

            if (context->FramesPerSecondGraphHandle)
                DestroyWindow(context->FramesPerSecondGraphHandle);
            if (context->FramesLatencyGraphHandle)
                DestroyWindow(context->FramesLatencyGraphHandle);
            if (context->PresentIntervalGraphHandle)
                DestroyWindow(context->PresentIntervalGraphHandle);
            if (context->PresentDurationGraphHandle)
                DestroyWindow(context->PresentDurationGraphHandle);
            if (context->FramesRenderTimeGraphHandle)
                DestroyWindow(context->FramesRenderTimeGraphHandle);
            if (context->FramesDisplayTimeGraphHandle)
                DestroyWindow(context->FramesDisplayTimeGraphHandle);
            if (context->FramesDisplayLatencyGraphHandle)
                DestroyWindow(context->FramesDisplayLatencyGraphHandle);

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

                    if (context->FramesPerSecondGraphHandle)
                    {
                        context->FramesPerSecondGraphState.Valid = FALSE;
                        context->FramesPerSecondGraphState.TooltipIndex = ULONG_MAX;
                        Graph_Draw(context->FramesPerSecondGraphHandle);
                    }

                    if (context->FramesLatencyGraphHandle)
                    {
                        context->FramesLatencyGraphState.Valid = FALSE;
                        context->FramesLatencyGraphState.TooltipIndex = ULONG_MAX;
                        Graph_Draw(context->FramesLatencyGraphHandle);
                    }

                    if (context->PresentIntervalGraphHandle)
                    {
                        context->PresentIntervalGraphState.Valid = FALSE;
                        context->PresentIntervalGraphState.TooltipIndex = ULONG_MAX;
                        Graph_Draw(context->PresentIntervalGraphHandle);
                    }

                    if (context->PresentDurationGraphHandle)
                    {
                        context->PresentDurationGraphState.Valid = FALSE;
                        context->PresentDurationGraphState.TooltipIndex = ULONG_MAX;
                        Graph_Draw(context->PresentDurationGraphHandle);
                    }

                    if (context->FramesRenderTimeGraphHandle)
                    {
                        context->FramesRenderTimeGraphState.Valid = FALSE;
                        context->FramesRenderTimeGraphState.TooltipIndex = ULONG_MAX;
                        Graph_Draw(context->FramesRenderTimeGraphHandle);
                    }

                    if (context->FramesDisplayTimeGraphHandle)
                    {
                        context->FramesDisplayTimeGraphState.Valid = FALSE;
                        context->FramesDisplayTimeGraphState.TooltipIndex = ULONG_MAX;
                        Graph_Draw(context->FramesDisplayTimeGraphHandle);
                    }

                    if (context->FramesDisplayLatencyGraphHandle)
                    {
                        context->FramesDisplayLatencyGraphState.Valid = FALSE;
                        context->FramesDisplayLatencyGraphState.TooltipIndex = ULONG_MAX;
                        Graph_Draw(context->FramesDisplayLatencyGraphHandle);
                    }
                }
                break;
            case PSN_KILLACTIVE:
                context->Enabled = FALSE;
                break;
            case GCN_GETDRAWINFO:
                {
                    PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)header;
                    PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;
                    LONG dpiValue;

                    dpiValue = PhGetWindowDpi(header->hwndFrom);

                    if (header->hwndFrom == context->FramesPerSecondGraphHandle)
                    {
                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (EtEnableScaleText ? PH_GRAPH_LABEL_MAX_Y : 0);
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), 0, dpiValue);
                        PhGraphStateGetDrawInfo(&context->FramesPerSecondGraphState, getDrawInfo, context->Block->FramesPerSecondHistory.Count);

                        if (!context->FramesPerSecondGraphState.Valid)
                        {
                            FLOAT max = 0;

                            PhCopyCircularBuffer_FLOAT(&context->Block->FramesPerSecondHistory, context->FramesPerSecondGraphState.Data1, drawInfo->LineDataCount);

                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data = context->FramesPerSecondGraphState.Data1[i]; // HACK

                                if (max < data)
                                    max = data;
                            }

                            if (max != 0)
                            {
                                PhDivideSinglesBySingle(context->FramesPerSecondGraphState.Data1, max, drawInfo->LineDataCount);
                            }

                            drawInfo->LabelYFunction = FramesLabelYFunction;
                            drawInfo->LabelYFunctionParameter = max;
                            context->FramesPerSecondGraphState.Valid = TRUE;
                        }

                        if (EtGraphShowText)
                        {
                            HDC hdc;
                            PH_FORMAT format[2];

                            PhInitFormatF(&format[0], context->Block->FramesPerSecond, 2);
                            PhInitFormatS(&format[1], L" FPS");

                            PhMoveReference(&context->FramesPerSecondGraphState.Text, PhFormat(format, RTL_NUMBER_OF(format), 0));

                            hdc = Graph_GetBufferedContext(context->FramesPerSecondGraphHandle);
                            PhSetGraphText(
                                hdc,
                                drawInfo,
                                &context->FramesPerSecondGraphState.Text->sr,
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
                    else if (header->hwndFrom == context->FramesLatencyGraphHandle)
                    {
                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (EtEnableScaleText ? PH_GRAPH_LABEL_MAX_Y : 0);
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorPhysical"), 0, dpiValue);
                        PhGraphStateGetDrawInfo(&context->FramesLatencyGraphState, getDrawInfo, context->Block->FramesLatencyHistory.Count);

                        if (!context->FramesLatencyGraphState.Valid)
                        {
                            FLOAT max = 0;

                            PhCopyCircularBuffer_FLOAT(&context->Block->FramesLatencyHistory, context->FramesLatencyGraphState.Data1, drawInfo->LineDataCount);

                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data = context->FramesLatencyGraphState.Data1[i]; // HACK

                                if (max < data)
                                    max = data;
                            }

                            if (max != 0)
                            {
                                PhDivideSinglesBySingle(context->FramesLatencyGraphState.Data1, max, drawInfo->LineDataCount);
                            }

                            drawInfo->LabelYFunction = FramesLabelYFunction;
                            drawInfo->LabelYFunctionParameter = max;
                            context->FramesLatencyGraphState.Valid = TRUE;
                        }
                        
                        if (EtGraphShowText)
                        {
                            HDC hdc;
                            PH_FORMAT format[2];

                            PhInitFormatF(&format[0], context->Block->FramesLatency, 2);
                            PhInitFormatS(&format[1], L" ms");

                            PhMoveReference(&context->FramesLatencyGraphState.Text, PhFormat(format, RTL_NUMBER_OF(format), 0));

                            hdc = Graph_GetBufferedContext(context->FramesLatencyGraphHandle);
                            PhSetGraphText(
                                hdc,
                                drawInfo,
                                &context->FramesLatencyGraphState.Text->sr,
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
                    else if (header->hwndFrom == context->PresentIntervalGraphHandle)
                    {
                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (EtEnableScaleText ? PH_GRAPH_LABEL_MAX_Y : 0);
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorIoWrite"), 0, dpiValue);
                        PhGraphStateGetDrawInfo(&context->PresentIntervalGraphState, getDrawInfo, context->Block->FramesMsBetweenPresentsHistory.Count);

                        if (!context->PresentIntervalGraphState.Valid)
                        {
                            FLOAT max = 0;

                            PhCopyCircularBuffer_FLOAT(&context->Block->FramesMsBetweenPresentsHistory, context->PresentIntervalGraphState.Data1, drawInfo->LineDataCount);

                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data = context->PresentIntervalGraphState.Data1[i]; // HACK

                                if (max < data)
                                    max = data;
                            }

                            if (max != 0)
                            {
                                PhDivideSinglesBySingle(context->PresentIntervalGraphState.Data1, max, drawInfo->LineDataCount);
                            }

                            drawInfo->LabelYFunction = FramesLabelYFunction;
                            drawInfo->LabelYFunctionParameter = max;
                            context->PresentIntervalGraphState.Valid = TRUE;
                        }

                        if (EtGraphShowText)
                        {
                            HDC hdc;
                            PH_FORMAT format[2];

                            PhInitFormatF(&format[0], context->Block->FramesMsBetweenPresents, 2);
                            PhInitFormatS(&format[1], L" ms");

                            PhMoveReference(&context->PresentIntervalGraphState.Text, PhFormat(format, RTL_NUMBER_OF(format), 0));

                            hdc = Graph_GetBufferedContext(context->PresentIntervalGraphHandle);
                            PhSetGraphText(
                                hdc,
                                drawInfo,
                                &context->PresentIntervalGraphState.Text->sr,
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
                    else if (header->hwndFrom == context->PresentDurationGraphHandle)
                    {
                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (EtEnableScaleText ? PH_GRAPH_LABEL_MAX_Y : 0);
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorPrivate"), 0, dpiValue);
                        PhGraphStateGetDrawInfo(&context->PresentDurationGraphState, getDrawInfo, context->Block->FramesMsInPresentApiHistory.Count);

                        if (!context->PresentDurationGraphState.Valid)
                        {
                            FLOAT max = 0;

                            PhCopyCircularBuffer_FLOAT(&context->Block->FramesMsInPresentApiHistory, context->PresentDurationGraphState.Data1, drawInfo->LineDataCount);

                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data = context->PresentDurationGraphState.Data1[i]; // HACK

                                if (max < data)
                                    max = data;
                            }

                            if (max != 0)
                            {
                                PhDivideSinglesBySingle(context->PresentDurationGraphState.Data1, max, drawInfo->LineDataCount);
                            }

                            drawInfo->LabelYFunction = FramesLabelYFunction;
                            drawInfo->LabelYFunctionParameter = max;
                            context->PresentDurationGraphState.Valid = TRUE;
                        }

                        if (EtGraphShowText)
                        {
                            HDC hdc;
                            PH_FORMAT format[2];

                            PhInitFormatF(&format[0], context->Block->FramesMsInPresentApi, 2);
                            PhInitFormatS(&format[1], L" ms");

                            PhMoveReference(&context->PresentDurationGraphState.Text, PhFormat(format, RTL_NUMBER_OF(format), 0));

                            hdc = Graph_GetBufferedContext(context->PresentDurationGraphHandle);
                            PhSetGraphText(
                                hdc,
                                drawInfo,
                                &context->PresentDurationGraphState.Text->sr,
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
                    else if (header->hwndFrom == context->FramesRenderTimeGraphHandle)
                    {
                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (EtEnableScaleText ? PH_GRAPH_LABEL_MAX_Y : 0);
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorPrivate"), 0, dpiValue);
                        PhGraphStateGetDrawInfo(&context->FramesRenderTimeGraphState, getDrawInfo, context->Block->FramesMsUntilRenderCompleteHistory.Count);

                        if (!context->FramesRenderTimeGraphState.Valid)
                        {
                            FLOAT max = 0;

                            PhCopyCircularBuffer_FLOAT(&context->Block->FramesMsUntilRenderCompleteHistory, context->FramesRenderTimeGraphState.Data1, drawInfo->LineDataCount);

                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data = context->FramesRenderTimeGraphState.Data1[i]; // HACK

                                if (max < data)
                                    max = data;
                            }

                            if (max != 0)
                            {
                                PhDivideSinglesBySingle(context->FramesRenderTimeGraphState.Data1, max, drawInfo->LineDataCount);
                            }

                            drawInfo->LabelYFunction = FramesLabelYFunction;
                            drawInfo->LabelYFunctionParameter = max;
                            context->FramesRenderTimeGraphState.Valid = TRUE;
                        }

                        if (EtGraphShowText)
                        {
                            HDC hdc;
                            PH_FORMAT format[2];

                            PhInitFormatF(&format[0], context->Block->FramesMsUntilRenderComplete, 2);
                            PhInitFormatS(&format[1], L" ms");

                            PhMoveReference(&context->FramesRenderTimeGraphState.Text, PhFormat(format, RTL_NUMBER_OF(format), 0));

                            hdc = Graph_GetBufferedContext(context->FramesRenderTimeGraphHandle);
                            PhSetGraphText(
                                hdc,
                                drawInfo,
                                &context->FramesRenderTimeGraphState.Text->sr,
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
                    else if (header->hwndFrom == context->FramesDisplayTimeGraphHandle)
                    {
                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (EtEnableScaleText ? PH_GRAPH_LABEL_MAX_Y : 0);
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), 0, dpiValue);
                        PhGraphStateGetDrawInfo(&context->FramesDisplayTimeGraphState, getDrawInfo, context->Block->FramesMsUntilDisplayedHistory.Count);

                        if (!context->FramesDisplayTimeGraphState.Valid)
                        {
                            FLOAT max = 0;

                            PhCopyCircularBuffer_FLOAT(&context->Block->FramesMsUntilDisplayedHistory, context->FramesDisplayTimeGraphState.Data1, drawInfo->LineDataCount);

                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data = context->FramesDisplayTimeGraphState.Data1[i]; // HACK

                                if (max < data)
                                    max = data;
                            }

                            if (max != 0)
                            {
                                PhDivideSinglesBySingle(context->FramesDisplayTimeGraphState.Data1, max, drawInfo->LineDataCount);
                            }

                            drawInfo->LabelYFunction = FramesLabelYFunction;
                            drawInfo->LabelYFunctionParameter = max;
                            context->FramesDisplayTimeGraphState.Valid = TRUE;
                        }

                        if (EtGraphShowText)
                        {
                            HDC hdc;
                            PH_FORMAT format[2];

                            PhInitFormatF(&format[0], context->Block->FramesMsUntilDisplayed, 2);
                            PhInitFormatS(&format[1], L" ms");

                            PhMoveReference(&context->FramesDisplayTimeGraphState.Text, PhFormat(format, RTL_NUMBER_OF(format), 0));

                            hdc = Graph_GetBufferedContext(context->FramesDisplayTimeGraphHandle);
                            PhSetGraphText(
                                hdc,
                                drawInfo,
                                &context->FramesDisplayTimeGraphState.Text->sr,
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
                    else if (header->hwndFrom == context->FramesDisplayLatencyGraphHandle)
                    {
                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (EtEnableScaleText ? PH_GRAPH_LABEL_MAX_Y : 0);
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorPhysical"), 0, dpiValue);
                        PhGraphStateGetDrawInfo(&context->FramesDisplayLatencyGraphState, getDrawInfo, context->Block->FramesDisplayLatencyHistory.Count);

                        if (!context->FramesDisplayLatencyGraphState.Valid)
                        {
                            FLOAT max = 0;

                            PhCopyCircularBuffer_FLOAT(&context->Block->FramesDisplayLatencyHistory, context->FramesDisplayLatencyGraphState.Data1, drawInfo->LineDataCount);

                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data = context->FramesDisplayLatencyGraphState.Data1[i]; // HACK

                                if (max < data)
                                    max = data;
                            }

                            if (max != 0)
                            {
                                PhDivideSinglesBySingle(context->FramesDisplayLatencyGraphState.Data1, max, drawInfo->LineDataCount);
                            }

                            drawInfo->LabelYFunction = FramesLabelYFunction;
                            drawInfo->LabelYFunctionParameter = max;
                            context->FramesDisplayLatencyGraphState.Valid = TRUE;
                        }

                        if (EtGraphShowText)
                        {
                            HDC hdc;
                            PH_FORMAT format[2];

                            PhInitFormatF(&format[0], context->Block->FramesDisplayLatency, 2);
                            PhInitFormatS(&format[1], L" ms");

                            PhMoveReference(&context->FramesDisplayLatencyGraphState.Text, PhFormat(format, RTL_NUMBER_OF(format), 0));

                            hdc = Graph_GetBufferedContext(context->FramesDisplayLatencyGraphHandle);
                            PhSetGraphText(
                                hdc,
                                drawInfo,
                                &context->FramesDisplayLatencyGraphState.Text->sr,
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
                        if (header->hwndFrom == context->FramesPerSecondGraphHandle)
                        {
                            if (context->FramesPerSecondGraphState.TooltipIndex != getTooltipText->Index)
                            {
                                FLOAT value;
                                PH_FORMAT format[3];

                                value = PhGetItemCircularBuffer_FLOAT(&context->Block->FramesPerSecondHistory, getTooltipText->Index);

                                PhInitFormatF(&format[0], value, 2);
                                PhInitFormatS(&format[1], L" FPS\n");
                                PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                                PhMoveReference(&context->FramesPerSecondGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                            }

                            getTooltipText->Text = PhGetStringRef(context->FramesPerSecondGraphState.TooltipText);
                        }
                        else if (header->hwndFrom == context->FramesLatencyGraphHandle)
                        {
                            if (context->FramesLatencyGraphState.TooltipIndex != getTooltipText->Index)
                            {
                                FLOAT value;
                                PH_FORMAT format[3];

                                value = PhGetItemCircularBuffer_FLOAT(&context->Block->FramesLatencyHistory, getTooltipText->Index);

                                PhInitFormatF(&format[0], value, 2);
                                PhInitFormatS(&format[1], L" ms\n");
                                PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                                PhMoveReference(&context->FramesLatencyGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                            }

                            getTooltipText->Text = PhGetStringRef(context->FramesLatencyGraphState.TooltipText);
                        }
                        else if (header->hwndFrom == context->PresentIntervalGraphHandle)
                        {
                            if (context->PresentIntervalGraphState.TooltipIndex != getTooltipText->Index)
                            {
                                FLOAT value;
                                PH_FORMAT format[3];

                                value = PhGetItemCircularBuffer_FLOAT(&context->Block->FramesMsBetweenPresentsHistory, getTooltipText->Index);

                                PhInitFormatF(&format[0], value, 2);
                                PhInitFormatS(&format[1], L" ms\n");
                                PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                                PhMoveReference(&context->PresentIntervalGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                            }

                            getTooltipText->Text = PhGetStringRef(context->PresentIntervalGraphState.TooltipText);
                        }
                        else if (header->hwndFrom == context->PresentDurationGraphHandle)
                        {
                            if (context->PresentDurationGraphState.TooltipIndex != getTooltipText->Index)
                            {
                                FLOAT value;
                                PH_FORMAT format[3];

                                value = PhGetItemCircularBuffer_FLOAT(&context->Block->FramesMsInPresentApiHistory, getTooltipText->Index);

                                PhInitFormatF(&format[0], value, 2);
                                PhInitFormatS(&format[1], L" ms\n");
                                PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                                PhMoveReference(&context->PresentDurationGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                            }

                            getTooltipText->Text = PhGetStringRef(context->PresentDurationGraphState.TooltipText);
                        }
                        else if (header->hwndFrom == context->FramesRenderTimeGraphHandle)
                        {
                            if (context->FramesRenderTimeGraphState.TooltipIndex != getTooltipText->Index)
                            {
                                FLOAT value;
                                PH_FORMAT format[3];

                                value = PhGetItemCircularBuffer_FLOAT(&context->Block->FramesMsUntilRenderCompleteHistory, getTooltipText->Index);

                                PhInitFormatF(&format[0], value, 2);
                                PhInitFormatS(&format[1], L" ms\n");
                                PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                                PhMoveReference(&context->FramesRenderTimeGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                            }

                            getTooltipText->Text = PhGetStringRef(context->FramesRenderTimeGraphState.TooltipText);
                        }
                        else if (header->hwndFrom == context->FramesDisplayTimeGraphHandle)
                        {
                            if (context->FramesDisplayTimeGraphState.TooltipIndex != getTooltipText->Index)
                            {
                                FLOAT value;
                                PH_FORMAT format[3];

                                value = PhGetItemCircularBuffer_FLOAT(&context->Block->FramesMsUntilDisplayedHistory, getTooltipText->Index);

                                PhInitFormatF(&format[0], value, 2);
                                PhInitFormatS(&format[1], L" ms\n");
                                PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                                PhMoveReference(&context->FramesDisplayTimeGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                            }

                            getTooltipText->Text = PhGetStringRef(context->FramesDisplayTimeGraphState.TooltipText);
                        }
                        else if (header->hwndFrom == context->FramesDisplayLatencyGraphHandle)
                        {
                            if (context->FramesDisplayLatencyGraphState.TooltipIndex != getTooltipText->Index)
                            {
                                FLOAT value;
                                PH_FORMAT format[3];

                                value = PhGetItemCircularBuffer_FLOAT(&context->Block->FramesDisplayLatencyHistory, getTooltipText->Index);

                                PhInitFormatF(&format[0], value, 2);
                                PhInitFormatS(&format[1], L" ms\n");
                                PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                                PhMoveReference(&context->FramesDisplayLatencyGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                            }

                            getTooltipText->Text = PhGetStringRef(context->FramesDisplayLatencyGraphState.TooltipText);
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
                FramesPropUpdateGraphs(context);
                FramesPropUpdatePanel(context);
            }
        }
        break;
    case WM_SIZE:
        {
            FramesPropLayoutGraphs(context);
        }
        break;
    }

    return FALSE;
}

VOID EtProcessFramesPropertiesInitializing(
    _In_ PVOID Parameter
    )
{
    PPH_PLUGIN_PROCESS_PROPCONTEXT propContext = Parameter;

    if (EtFramesEnabled)
    {
        PhAddProcessPropPage(
            propContext->PropContext,
            PhCreateProcessPropPageContextEx(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_PROCFRAMES), EtpFramesPageDlgProc, NULL)
            );
    }
}
