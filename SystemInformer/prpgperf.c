/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2019-2023
 *
 */

#include <phapp.h>
#include <phplug.h>
#include <procprp.h>
#include <procprpp.h>
#include <procprv.h>
#include <phsettings.h>

static VOID NTAPI PerformanceUpdateHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PERFORMANCE_CONTEXT performanceContext = (PPH_PERFORMANCE_CONTEXT)Context;

    if (performanceContext && performanceContext->Enabled)
        PostMessage(performanceContext->WindowHandle, WM_PH_PERFORMANCE_UPDATE, 0, 0);
}

INT_PTR CALLBACK PhpProcessPerformanceDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_PERFORMANCE_CONTEXT performanceContext;

    if (PhPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext, &processItem))
    {
        performanceContext = (PPH_PERFORMANCE_CONTEXT)propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            performanceContext = propPageContext->Context = PhAllocateZero(sizeof(PH_PERFORMANCE_CONTEXT));
            performanceContext->WindowHandle = hwndDlg;
            performanceContext->Enabled = TRUE;
            performanceContext->WindowDpi = PhGetWindowDpi(hwndDlg);

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                PerformanceUpdateHandler,
                performanceContext,
                &performanceContext->ProcessesUpdatedRegistration
                );

            // We have already set the group boxes to have WS_EX_TRANSPARENT to fix
            // the drawing issue that arises when using WS_CLIPCHILDREN. However
            // in removing the flicker from the graphs the group boxes will now flicker.
            // It's a good tradeoff since no one stares at the group boxes.
            PhSetWindowStyle(hwndDlg, WS_CLIPCHILDREN, WS_CLIPCHILDREN);

            performanceContext->CpuGroupBox = GetDlgItem(hwndDlg, IDC_GROUPCPU);
            performanceContext->PrivateBytesGroupBox = GetDlgItem(hwndDlg, IDC_GROUPPRIVATEBYTES);
            performanceContext->IoGroupBox = GetDlgItem(hwndDlg, IDC_GROUPIO);

            PhInitializeGraphState(&performanceContext->CpuGraphState);
            PhInitializeGraphState(&performanceContext->PrivateGraphState);
            PhInitializeGraphState(&performanceContext->IoGraphState);

            performanceContext->CpuGraphHandle = GetDlgItem(hwndDlg, IDC_CPU);
            PhSetWindowStyle(performanceContext->CpuGraphHandle, WS_BORDER, WS_BORDER);
            Graph_SetTooltip(performanceContext->CpuGraphHandle, TRUE);
            BringWindowToTop(performanceContext->CpuGraphHandle);

            performanceContext->PrivateGraphHandle = GetDlgItem(hwndDlg, IDC_PRIVATEBYTES);
            PhSetWindowStyle(performanceContext->PrivateGraphHandle, WS_BORDER, WS_BORDER);
            Graph_SetTooltip(performanceContext->PrivateGraphHandle, TRUE);
            BringWindowToTop(performanceContext->PrivateGraphHandle);

            performanceContext->IoGraphHandle = GetDlgItem(hwndDlg, IDC_IO);
            PhSetWindowStyle(performanceContext->IoGraphHandle, WS_BORDER, WS_BORDER);
            Graph_SetTooltip(performanceContext->IoGraphHandle, TRUE);
            BringWindowToTop(performanceContext->IoGraphHandle);

            PhInitializeWindowTheme(hwndDlg);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteGraphState(&performanceContext->CpuGraphState);
            PhDeleteGraphState(&performanceContext->PrivateGraphState);
            PhDeleteGraphState(&performanceContext->IoGraphState);

            PhUnregisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                &performanceContext->ProcessesUpdatedRegistration
                );
            PhFree(performanceContext);
        }
        break;
    case WM_SHOWWINDOW:
        {
            PPH_LAYOUT_ITEM dialogItem;

            if (dialogItem = PhBeginPropPageLayout(hwndDlg, propPageContext))
            {
                PhEndPropPageLayout(hwndDlg, propPageContext);
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_SETACTIVE:
                {
                    performanceContext->Enabled = TRUE;

                    performanceContext->CpuGraphState.Valid = FALSE;
                    performanceContext->CpuGraphState.TooltipIndex = ULONG_MAX;
                    performanceContext->PrivateGraphState.Valid = FALSE;
                    performanceContext->PrivateGraphState.TooltipIndex = ULONG_MAX;
                    performanceContext->IoGraphState.Valid = FALSE;
                    performanceContext->IoGraphState.TooltipIndex = ULONG_MAX;

                    if (performanceContext->CpuGraphHandle)
                        Graph_Draw(performanceContext->CpuGraphHandle);
                    if (performanceContext->PrivateGraphHandle)
                        Graph_Draw(performanceContext->PrivateGraphHandle);
                    if (performanceContext->IoGraphHandle)
                        Graph_Draw(performanceContext->IoGraphHandle);
                }
                break;
            case PSN_KILLACTIVE:
                performanceContext->Enabled = FALSE;
                break;
            case GCN_GETDRAWINFO:
                {
                    PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)header;
                    PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

                    if (header->hwndFrom == performanceContext->CpuGraphHandle)
                    {
                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_USE_LINE_2 | (PhCsEnableGraphMaxText ? PH_GRAPH_LABEL_MAX_Y : 0);
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhCsColorCpuKernel, PhCsColorCpuUser, performanceContext->WindowDpi);

                        PhGraphStateGetDrawInfo(
                            &performanceContext->CpuGraphState,
                            getDrawInfo,
                            processItem->CpuKernelHistory.Count
                            );

                        if (!performanceContext->CpuGraphState.Valid)
                        {
                            PhCopyCircularBuffer_FLOAT(&processItem->CpuKernelHistory,
                                performanceContext->CpuGraphState.Data1, drawInfo->LineDataCount);
                            PhCopyCircularBuffer_FLOAT(&processItem->CpuUserHistory,
                                performanceContext->CpuGraphState.Data2, drawInfo->LineDataCount);

                            if (PhCsEnableGraphMaxScale)
                            {
                                FLOAT max = 0;

                                if (PhCsEnableAvxSupport && drawInfo->LineDataCount > 128)
                                {
                                    max = PhAddPlusMaxMemorySingles(
                                        performanceContext->CpuGraphState.Data1,
                                        performanceContext->CpuGraphState.Data2,
                                        drawInfo->LineDataCount
                                        );
                                }
                                else
                                {
                                    for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                                    {
                                        FLOAT data = performanceContext->CpuGraphState.Data1[i] +
                                            performanceContext->CpuGraphState.Data2[i];

                                        if (max < data)
                                            max = data;
                                    }
                                }

                                if (max != 0)
                                {
                                    PhDivideSinglesBySingle(
                                        performanceContext->CpuGraphState.Data1,
                                        max,
                                        drawInfo->LineDataCount
                                        );
                                    PhDivideSinglesBySingle(
                                        performanceContext->CpuGraphState.Data2,
                                        max,
                                        drawInfo->LineDataCount
                                        );
                                }

                                drawInfo->LabelYFunction = PhSiDoubleLabelYFunction;
                                drawInfo->LabelYFunctionParameter = max;
                            }
                            else
                            {
                                drawInfo->LabelYFunction = PhSiDoubleLabelYFunction;
                                drawInfo->LabelYFunctionParameter = 1.0f;
                            }

                            performanceContext->CpuGraphState.Valid = TRUE;
                        }

                        if (PhCsGraphShowText)
                        {
                            HDC hdc;
                            PH_FORMAT format[6];

                            // %.2f%% (K: %.2f%%, U: %.2f%%)
                            PhInitFormatF(&format[0], (processItem->CpuKernelUsage + processItem->CpuUserUsage) * 100.f, PhMaxPrecisionUnit);
                            PhInitFormatS(&format[1], L"% (K: ");
                            PhInitFormatF(&format[2], processItem->CpuKernelUsage * 100.f, PhMaxPrecisionUnit);
                            PhInitFormatS(&format[3], L"%, U: ");
                            PhInitFormatF(&format[4], processItem->CpuUserUsage * 100.f, PhMaxPrecisionUnit);
                            PhInitFormatS(&format[5], L"%)");

                            PhMoveReference(&performanceContext->CpuGraphState.Text, PhFormat(format, RTL_NUMBER_OF(format), 16));

                            hdc = Graph_GetBufferedContext(performanceContext->CpuGraphHandle);
                            PhSetGraphText(hdc, drawInfo, &performanceContext->CpuGraphState.Text->sr,
                                &PhNormalGraphTextMargin, &PhNormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }
                    }
                    else if (header->hwndFrom == performanceContext->PrivateGraphHandle)
                    {
                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (PhCsEnableGraphMaxText ? PH_GRAPH_LABEL_MAX_Y : 0);
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhCsColorPrivate, 0, performanceContext->WindowDpi);

                        PhGraphStateGetDrawInfo(
                            &performanceContext->PrivateGraphState,
                            getDrawInfo,
                            processItem->PrivateBytesHistory.Count
                            );

                        if (!performanceContext->PrivateGraphState.Valid)
                        {
                            FLOAT max = PhCsEnableGraphMaxScale ? 0.f : (FLOAT)processItem->VmCounters.PeakPagefileUsage;

                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data = performanceContext->PrivateGraphState.Data1[i] =
                                    (FLOAT)PhGetItemCircularBuffer_SIZE_T(&processItem->PrivateBytesHistory, i);

                                if (max < data)
                                    max = data;
                            }

                            if (max != 0)
                            {
                                // Scale the data.
                                PhDivideSinglesBySingle(
                                    performanceContext->PrivateGraphState.Data1,
                                    max,
                                    drawInfo->LineDataCount
                                    );
                            }

                            drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                            drawInfo->LabelYFunctionParameter = max;

                            performanceContext->PrivateGraphState.Valid = TRUE;
                        }

                        if (PhCsGraphShowText)
                        {
                            HDC hdc;
                            PH_FORMAT format[1];

                            PhInitFormatSize(&format[0], processItem->VmCounters.PagefileUsage);

                            PhMoveReference(&performanceContext->PrivateGraphState.Text,
                                PhFormat(format, RTL_NUMBER_OF(format), 0));

                            hdc = Graph_GetBufferedContext(performanceContext->PrivateGraphHandle);
                            PhSetGraphText(hdc, drawInfo, &performanceContext->PrivateGraphState.Text->sr,
                                &PhNormalGraphTextMargin, &PhNormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }
                    }
                    else if (header->hwndFrom == performanceContext->IoGraphHandle)
                    {
                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y | PH_GRAPH_USE_LINE_2;
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhCsColorIoReadOther, PhCsColorIoWrite, performanceContext->WindowDpi);

                        PhGraphStateGetDrawInfo(
                            &performanceContext->IoGraphState,
                            getDrawInfo,
                            processItem->IoReadHistory.Count
                            );

                        if (!performanceContext->IoGraphState.Valid)
                        {
                            ULONG i;
                            FLOAT max = 0;

                            for (i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data1;
                                FLOAT data2;

                                performanceContext->IoGraphState.Data1[i] = data1 =
                                    (FLOAT)PhGetItemCircularBuffer_ULONG64(&processItem->IoReadHistory, i) +
                                    (FLOAT)PhGetItemCircularBuffer_ULONG64(&processItem->IoOtherHistory, i);
                                performanceContext->IoGraphState.Data2[i] = data2 =
                                    (FLOAT)PhGetItemCircularBuffer_ULONG64(&processItem->IoWriteHistory, i);

                                if (max < data1 + data2)
                                    max = data1 + data2;
                            }

                            if (max != 0)
                            {
                                // Scale the data.

                                PhDivideSinglesBySingle(
                                    performanceContext->IoGraphState.Data1,
                                    max,
                                    drawInfo->LineDataCount
                                    );
                                PhDivideSinglesBySingle(
                                    performanceContext->IoGraphState.Data2,
                                    max,
                                    drawInfo->LineDataCount
                                    );
                            }

                            drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                            drawInfo->LabelYFunctionParameter = max;

                            performanceContext->IoGraphState.Valid = TRUE;
                        }

                        if (PhCsGraphShowText)
                        {
                            HDC hdc;
                            PH_FORMAT format[4];

                            // R+O: %s, W: %s
                            PhInitFormatS(&format[0], L"R+O: ");
                            PhInitFormatSize(&format[1], processItem->IoReadDelta.Delta + processItem->IoOtherDelta.Delta);
                            PhInitFormatS(&format[2], L", W: ");
                            PhInitFormatSize(&format[3], processItem->IoWriteDelta.Delta);

                            PhMoveReference(&performanceContext->IoGraphState.Text,
                                PhFormat(format, RTL_NUMBER_OF(format), 64));

                            hdc = Graph_GetBufferedContext(performanceContext->IoGraphHandle);
                            PhSetGraphText(hdc, drawInfo, &performanceContext->IoGraphState.Text->sr,
                                &PhNormalGraphTextMargin, &PhNormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
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

                    if (
                        header->hwndFrom == performanceContext->CpuGraphHandle &&
                        getTooltipText->Index < getTooltipText->TotalCount
                        )
                    {
                        if (performanceContext->CpuGraphState.TooltipIndex != getTooltipText->Index)
                        {
                            FLOAT cpuKernel;
                            FLOAT cpuUser;
                            PH_FORMAT format[7];

                            cpuKernel = PhGetItemCircularBuffer_FLOAT(&processItem->CpuKernelHistory, getTooltipText->Index);
                            cpuUser = PhGetItemCircularBuffer_FLOAT(&processItem->CpuUserHistory, getTooltipText->Index);

                            // %.2f%% (K: %.2f%%, U: %.2f%%)%s\n%s
                            PhInitFormatF(&format[0], (cpuKernel + cpuUser) * 100.f, PhMaxPrecisionUnit);
                            PhInitFormatS(&format[1], L"% (K: ");
                            PhInitFormatF(&format[2], cpuKernel * 100.f, PhMaxPrecisionUnit);
                            PhInitFormatS(&format[3], L"%, U: ");
                            PhInitFormatF(&format[4], cpuUser * 100.f, PhMaxPrecisionUnit);
                            PhInitFormatS(&format[5], L"%)\n");
                            PhInitFormatSR(&format[6], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(processItem, getTooltipText->Index))->sr);

                            PhMoveReference(&performanceContext->CpuGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 160));
                        }

                        getTooltipText->Text = performanceContext->CpuGraphState.TooltipText->sr;
                    }
                    else if (
                        header->hwndFrom == performanceContext->PrivateGraphHandle &&
                        getTooltipText->Index < getTooltipText->TotalCount
                        )
                    {
                        if (performanceContext->PrivateGraphState.TooltipIndex != getTooltipText->Index)
                        {
                            SIZE_T privateBytes;
                            PH_FORMAT format[3];

                            privateBytes = PhGetItemCircularBuffer_SIZE_T(&processItem->PrivateBytesHistory, getTooltipText->Index);

                            // %s\n%s
                            PhInitFormatSize(&format[0], privateBytes);
                            PhInitFormatC(&format[1], L'\n');
                            PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(processItem, getTooltipText->Index))->sr);

                            PhMoveReference(&performanceContext->PrivateGraphState.TooltipText,
                                PhFormat(format, RTL_NUMBER_OF(format), 64));
                        }

                        getTooltipText->Text = performanceContext->PrivateGraphState.TooltipText->sr;
                    }
                    else if (
                        header->hwndFrom == performanceContext->IoGraphHandle &&
                        getTooltipText->Index < getTooltipText->TotalCount
                        )
                    {
                        if (performanceContext->IoGraphState.TooltipIndex != getTooltipText->Index)
                        {
                            ULONG64 ioRead;
                            ULONG64 ioWrite;
                            ULONG64 ioOther;
                            PH_FORMAT format[8];

                            ioRead = PhGetItemCircularBuffer_ULONG64(&processItem->IoReadHistory, getTooltipText->Index);
                            ioWrite = PhGetItemCircularBuffer_ULONG64(&processItem->IoWriteHistory, getTooltipText->Index);
                            ioOther = PhGetItemCircularBuffer_ULONG64(&processItem->IoOtherHistory, getTooltipText->Index);

                            // R: %s\nW: %s\nO: %s\n%s
                            PhInitFormatS(&format[0], L"R: ");
                            PhInitFormatSize(&format[1], ioRead);
                            PhInitFormatS(&format[2], L"\nW: ");
                            PhInitFormatSize(&format[3], ioWrite);
                            PhInitFormatS(&format[4], L"\nO: ");
                            PhInitFormatSize(&format[5], ioOther);
                            PhInitFormatC(&format[6], L'\n');
                            PhInitFormatSR(&format[7], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(processItem, getTooltipText->Index))->sr);

                            PhMoveReference(&performanceContext->IoGraphState.TooltipText,
                                PhFormat(format, RTL_NUMBER_OF(format), 64));
                        }

                        getTooltipText->Text = performanceContext->IoGraphState.TooltipText->sr;
                    }
                }
                break;
            }
        }
        break;
    case WM_SIZE:
        {
            HDWP deferHandle;
            RECT clientRect;
            RECT margin;
            RECT innerMargin;
            LONG between;
            LONG width;
            LONG height;

            margin.left = margin.top = margin.right = margin.bottom = PhGetDpi(13, performanceContext->WindowDpi);

            innerMargin.top = PhGetDpi(20, performanceContext->WindowDpi);
            innerMargin.left = innerMargin.right = innerMargin.bottom = PhGetDpi(10, performanceContext->WindowDpi);

            between = PhGetDpi(3, performanceContext->WindowDpi);

            performanceContext->CpuGraphState.Valid = FALSE;
            performanceContext->CpuGraphState.TooltipIndex = ULONG_MAX;
            performanceContext->PrivateGraphState.Valid = FALSE;
            performanceContext->PrivateGraphState.TooltipIndex = ULONG_MAX;
            performanceContext->IoGraphState.Valid = FALSE;
            performanceContext->IoGraphState.TooltipIndex = ULONG_MAX;

            GetClientRect(hwndDlg, &clientRect);
            width = clientRect.right - margin.left - margin.right;
            height = (clientRect.bottom - margin.top - margin.bottom - between * 2) / 3;

            deferHandle = BeginDeferWindowPos(6);

            deferHandle = DeferWindowPos(deferHandle, performanceContext->CpuGroupBox, NULL, margin.left, margin.top,
                width, height, SWP_NOACTIVATE | SWP_NOZORDER);
            deferHandle = DeferWindowPos(
                deferHandle,
                performanceContext->CpuGraphHandle,
                NULL,
                margin.left + innerMargin.left,
                margin.top + innerMargin.top,
                width - innerMargin.left - innerMargin.right,
                height - innerMargin.top - innerMargin.bottom,
                SWP_NOACTIVATE | SWP_NOZORDER
                );

            deferHandle = DeferWindowPos(deferHandle, performanceContext->PrivateBytesGroupBox, NULL, margin.left, margin.top + height + between,
                width, height, SWP_NOACTIVATE | SWP_NOZORDER);
            deferHandle = DeferWindowPos(
                deferHandle,
                performanceContext->PrivateGraphHandle,
                NULL,
                margin.left + innerMargin.left,
                margin.top + height + between + innerMargin.top,
                width - innerMargin.left - innerMargin.right,
                height - innerMargin.top - innerMargin.bottom,
                SWP_NOACTIVATE | SWP_NOZORDER
                );

            deferHandle = DeferWindowPos(deferHandle, performanceContext->IoGroupBox, NULL, margin.left, margin.top + (height + between) * 2,
                width, height, SWP_NOACTIVATE | SWP_NOZORDER);
            deferHandle = DeferWindowPos(
                deferHandle,
                performanceContext->IoGraphHandle,
                NULL,
                margin.left + innerMargin.left,
                margin.top + (height + between) * 2 + innerMargin.top,
                width - innerMargin.left - innerMargin.right,
                height - innerMargin.top - innerMargin.bottom,
                SWP_NOACTIVATE | SWP_NOZORDER
                );

            EndDeferWindowPos(deferHandle);
        }
        break;
    case WM_PH_PERFORMANCE_UPDATE:
        {
            if (!(processItem->State & PH_PROCESS_ITEM_REMOVED))
            {
                performanceContext->CpuGraphState.Valid = FALSE;
                Graph_Update(performanceContext->CpuGraphHandle);

                performanceContext->PrivateGraphState.Valid = FALSE;
                Graph_Update(performanceContext->PrivateGraphHandle);

                performanceContext->IoGraphState.Valid = FALSE;
                Graph_Update(performanceContext->IoGraphHandle);
            }
        }
        break;
    case WM_DPICHANGED_BEFOREPARENT:
        {
            performanceContext->WindowDpi = PhGetWindowDpi(hwndDlg);
        }
        break;
    }

    return FALSE;
}
