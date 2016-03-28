/*
 * Process Hacker -
 *   Process properties: Performance page
 *
 * Copyright (C) 2009-2016 wj32
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
#include <graph.h>
#include <settings.h>
#include <procprv.h>
#include <sysinfo.h>
#include <procprpp.h>

static VOID NTAPI PerformanceUpdateHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PERFORMANCE_CONTEXT performanceContext = (PPH_PERFORMANCE_CONTEXT)Context;

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

    if (PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
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
            performanceContext = propPageContext->Context =
                PhAllocate(sizeof(PH_PERFORMANCE_CONTEXT));

            performanceContext->WindowHandle = hwndDlg;

            PhRegisterCallback(
                &PhProcessesUpdatedEvent,
                PerformanceUpdateHandler,
                performanceContext,
                &performanceContext->ProcessesUpdatedRegistration
                );

            // We have already set the group boxes to have WS_EX_TRANSPARENT to fix
            // the drawing issue that arises when using WS_CLIPCHILDREN. However
            // in removing the flicker from the graphs the group boxes will now flicker.
            // It's a good tradeoff since no one stares at the group boxes.
            PhSetWindowStyle(hwndDlg, WS_CLIPCHILDREN, WS_CLIPCHILDREN);

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
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteGraphState(&performanceContext->CpuGraphState);
            PhDeleteGraphState(&performanceContext->PrivateGraphState);
            PhDeleteGraphState(&performanceContext->IoGraphState);

            PhUnregisterCallback(
                &PhProcessesUpdatedEvent,
                &performanceContext->ProcessesUpdatedRegistration
                );
            PhFree(performanceContext);

            PhpPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PhAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);

                PhDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case GCN_GETDRAWINFO:
                {
                    PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)header;
                    PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

                    if (header->hwndFrom == performanceContext->CpuGraphHandle)
                    {
                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_USE_LINE_2;
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhCsColorCpuKernel, PhCsColorCpuUser);

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
                            performanceContext->CpuGraphState.Valid = TRUE;
                        }

                        if (PhCsGraphShowText)
                        {
                            HDC hdc;

                            PhMoveReference(&performanceContext->CpuGraphState.Text,
                                PhFormatString(L"%.2f%%",
                                (processItem->CpuKernelUsage + processItem->CpuUserUsage) * 100
                                ));

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
                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y;
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhCsColorPrivate, 0);

                        PhGraphStateGetDrawInfo(
                            &performanceContext->PrivateGraphState,
                            getDrawInfo,
                            processItem->PrivateBytesHistory.Count
                            );

                        if (!performanceContext->PrivateGraphState.Valid)
                        {
                            ULONG i;

                            for (i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                performanceContext->PrivateGraphState.Data1[i] =
                                    (FLOAT)PhGetItemCircularBuffer_SIZE_T(&processItem->PrivateBytesHistory, i);
                            }

                            if (processItem->VmCounters.PeakPagefileUsage != 0)
                            {
                                // Scale the data.
                                PhDivideSinglesBySingle(
                                    performanceContext->PrivateGraphState.Data1,
                                    (FLOAT)processItem->VmCounters.PeakPagefileUsage,
                                    drawInfo->LineDataCount
                                    );
                            }

                            performanceContext->PrivateGraphState.Valid = TRUE;
                        }

                        if (PhCsGraphShowText)
                        {
                            HDC hdc;

                            PhMoveReference(&performanceContext->PrivateGraphState.Text,
                                PhConcatStrings2(
                                L"Private bytes: ",
                                PhaFormatSize(processItem->VmCounters.PagefileUsage, -1)->Buffer
                                ));

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
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhCsColorIoReadOther, PhCsColorIoWrite);

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

                            PhMoveReference(&performanceContext->IoGraphState.Text,
                                PhFormatString(
                                L"R+O: %s, W: %s",
                                PhaFormatSize(processItem->IoReadDelta.Delta + processItem->IoOtherDelta.Delta, -1)->Buffer,
                                PhaFormatSize(processItem->IoWriteDelta.Delta, -1)->Buffer
                                ));

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

                            cpuKernel = PhGetItemCircularBuffer_FLOAT(&processItem->CpuKernelHistory, getTooltipText->Index);
                            cpuUser = PhGetItemCircularBuffer_FLOAT(&processItem->CpuUserHistory, getTooltipText->Index);

                            PhMoveReference(&performanceContext->CpuGraphState.TooltipText, PhFormatString(
                                L"%.2f%%\n%s",
                                (cpuKernel + cpuUser) * 100,
                                PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(processItem, getTooltipText->Index))->Buffer
                                ));
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

                            privateBytes = PhGetItemCircularBuffer_SIZE_T(&processItem->PrivateBytesHistory, getTooltipText->Index);

                            PhMoveReference(&performanceContext->PrivateGraphState.TooltipText, PhFormatString(
                                L"Private bytes: %s\n%s",
                                PhaFormatSize(privateBytes, -1)->Buffer,
                                PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(processItem, getTooltipText->Index))->Buffer
                                ));
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

                            ioRead = PhGetItemCircularBuffer_ULONG64(&processItem->IoReadHistory, getTooltipText->Index);
                            ioWrite = PhGetItemCircularBuffer_ULONG64(&processItem->IoWriteHistory, getTooltipText->Index);
                            ioOther = PhGetItemCircularBuffer_ULONG64(&processItem->IoOtherHistory, getTooltipText->Index);

                            PhMoveReference(&performanceContext->IoGraphState.TooltipText, PhFormatString(
                                L"R: %s\nW: %s\nO: %s\n%s",
                                PhaFormatSize(ioRead, -1)->Buffer,
                                PhaFormatSize(ioWrite, -1)->Buffer,
                                PhaFormatSize(ioOther, -1)->Buffer,
                                PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(processItem, getTooltipText->Index))->Buffer
                                ));
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
            HWND cpuGroupBox = GetDlgItem(hwndDlg, IDC_GROUPCPU);
            HWND privateBytesGroupBox = GetDlgItem(hwndDlg, IDC_GROUPPRIVATEBYTES);
            HWND ioGroupBox = GetDlgItem(hwndDlg, IDC_GROUPIO);
            RECT clientRect;
            RECT margin = { PH_SCALE_DPI(13), PH_SCALE_DPI(13), PH_SCALE_DPI(13), PH_SCALE_DPI(13) };
            RECT innerMargin = { PH_SCALE_DPI(10), PH_SCALE_DPI(20), PH_SCALE_DPI(10), PH_SCALE_DPI(10) };
            LONG between = PH_SCALE_DPI(3);
            LONG width;
            LONG height;

            performanceContext->CpuGraphState.Valid = FALSE;
            performanceContext->CpuGraphState.TooltipIndex = -1;
            performanceContext->PrivateGraphState.Valid = FALSE;
            performanceContext->PrivateGraphState.TooltipIndex = -1;
            performanceContext->IoGraphState.Valid = FALSE;
            performanceContext->IoGraphState.TooltipIndex = -1;

            GetClientRect(hwndDlg, &clientRect);
            width = clientRect.right - margin.left - margin.right;
            height = (clientRect.bottom - margin.top - margin.bottom - between * 2) / 3;

            deferHandle = BeginDeferWindowPos(6);

            deferHandle = DeferWindowPos(deferHandle, cpuGroupBox, NULL, margin.left, margin.top,
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

            deferHandle = DeferWindowPos(deferHandle, privateBytesGroupBox, NULL, margin.left, margin.top + height + between,
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

            deferHandle = DeferWindowPos(deferHandle, ioGroupBox, NULL, margin.left, margin.top + (height + between) * 2,
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
                Graph_MoveGrid(performanceContext->CpuGraphHandle, 1);
                Graph_Draw(performanceContext->CpuGraphHandle);
                Graph_UpdateTooltip(performanceContext->CpuGraphHandle);
                InvalidateRect(performanceContext->CpuGraphHandle, NULL, FALSE);

                performanceContext->PrivateGraphState.Valid = FALSE;
                Graph_MoveGrid(performanceContext->PrivateGraphHandle, 1);
                Graph_Draw(performanceContext->PrivateGraphHandle);
                Graph_UpdateTooltip(performanceContext->PrivateGraphHandle);
                InvalidateRect(performanceContext->PrivateGraphHandle, NULL, FALSE);

                performanceContext->IoGraphState.Valid = FALSE;
                Graph_MoveGrid(performanceContext->IoGraphHandle, 1);
                Graph_Draw(performanceContext->IoGraphHandle);
                Graph_UpdateTooltip(performanceContext->IoGraphHandle);
                InvalidateRect(performanceContext->IoGraphHandle, NULL, FALSE);
            }
        }
        break;
    }

    return FALSE;
}
