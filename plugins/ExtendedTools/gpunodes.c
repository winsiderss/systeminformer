/*
 * Process Hacker Extended Tools -
 *   GPU nodes window
 *
 * Copyright (C) 2011-2015 wj32
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
#include <windowsx.h>
#include <math.h>

#define UPDATE_MSG (WM_APP + 1)
#define GRAPH_PADDING 5
#define CHECKBOX_PADDING 3

INT_PTR CALLBACK EtpGpuNodesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

static HWND WindowHandle;
static RECT MinimumSize;
static PH_LAYOUT_MANAGER LayoutManager;
static RECT LayoutMargin;
static HWND *GraphHandle;
static HWND *CheckBoxHandle;
static PPH_GRAPH_STATE GraphState;
static PPH_SYSINFO_PARAMETERS SysInfoParameters;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;

VOID EtShowGpuNodesDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_SYSINFO_PARAMETERS Parameters
    )
{
    SysInfoParameters = Parameters;
    DialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_GPUNODES),
        ParentWindowHandle,
        EtpGpuNodesDlgProc
        );
}

static VOID ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PostMessage(WindowHandle, UPDATE_MSG, 0, 0);
}

VOID EtpLoadNodeBitMap(
    VOID
    )
{
    ULONG i;

    for (i = 0; i < EtGpuTotalNodeCount; i++)
    {
        Button_SetCheck(
            CheckBoxHandle[i],
            RtlCheckBit(&EtGpuNodeBitMap, i) ? BST_CHECKED : BST_UNCHECKED
            );
    }
}

VOID EtpSaveNodeBitMap(
    VOID
    )
{
    RTL_BITMAP newBitMap;
    ULONG i;

    EtAllocateGpuNodeBitMap(&newBitMap);

    for (i = 0; i < EtGpuTotalNodeCount; i++)
    {
        if (Button_GetCheck(CheckBoxHandle[i]) == BST_CHECKED)
            RtlSetBits(&newBitMap, i, 1);
    }

    if (RtlNumberOfSetBits(&newBitMap) == 0)
        RtlSetBits(&newBitMap, 0, 1);

    EtUpdateGpuNodeBitMap(&newBitMap);
}

INT_PTR CALLBACK EtpGpuNodesDlgProc(
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
            ULONG i;
            HFONT font;
            PPH_STRING nodeString;
            RECT labelRect;
            RECT tempRect;
            ULONG numberOfRows;
            ULONG numberOfColumns;

            WindowHandle = hwndDlg;
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            LayoutMargin = PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_LAYOUT), NULL, PH_ANCHOR_ALL)->Margin;

            PhRegisterCallback(&PhProcessesUpdatedEvent, ProcessesUpdatedCallback, NULL, &ProcessesUpdatedCallbackRegistration);

            GraphHandle = PhAllocate(sizeof(HWND) * EtGpuTotalNodeCount);
            CheckBoxHandle = PhAllocate(sizeof(HWND) * EtGpuTotalNodeCount);
            GraphState = PhAllocate(sizeof(PH_GRAPH_STATE) * EtGpuTotalNodeCount);

            font = (HFONT)SendMessage(hwndDlg, WM_GETFONT, 0, 0);

            for (i = 0; i < EtGpuTotalNodeCount; i++)
            {
                nodeString = PhFormatString(L"Node %lu", i);

                GraphHandle[i] = CreateWindow(
                    PH_GRAPH_CLASSNAME,
                    NULL,
                    WS_VISIBLE | WS_CHILD | WS_BORDER,
                    0,
                    0,
                    3,
                    3,
                    hwndDlg,
                    NULL,
                    NULL,
                    NULL
                    );
                Graph_SetTooltip(GraphHandle[i], TRUE);
                CheckBoxHandle[i] = CreateWindow(
                    WC_BUTTON,
                    nodeString->Buffer,
                    WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                    0,
                    0,
                    3,
                    3,
                    hwndDlg,
                    NULL,
                    NULL,
                    NULL
                    );
                SendMessage(CheckBoxHandle[i], WM_SETFONT, (WPARAM)font, FALSE);
                PhInitializeGraphState(&GraphState[i]);

                PhDereferenceObject(nodeString);
            }

            // Calculate the minimum size.

            numberOfRows = (ULONG)sqrt(EtGpuTotalNodeCount);
            numberOfColumns = (EtGpuTotalNodeCount + numberOfRows - 1) / numberOfRows;

            MinimumSize.left = 0;
            MinimumSize.top = 0;
            MinimumSize.right = 45;
            MinimumSize.bottom = 60;
            MapDialogRect(hwndDlg, &MinimumSize);
            MinimumSize.right += (MinimumSize.right + GRAPH_PADDING) * numberOfColumns;
            MinimumSize.bottom += (MinimumSize.bottom + GRAPH_PADDING) * numberOfRows;

            GetWindowRect(GetDlgItem(hwndDlg, IDC_INSTRUCTION), &labelRect);
            MapWindowPoints(NULL, hwndDlg, (POINT *)&labelRect, 2);
            labelRect.right += GetSystemMetrics(SM_CXFRAME) * 2;

            tempRect.left = 0;
            tempRect.top = 0;
            tempRect.right = 7;
            tempRect.bottom = 0;
            MapDialogRect(hwndDlg, &tempRect);
            labelRect.right += tempRect.right;

            if (MinimumSize.right < labelRect.right)
                MinimumSize.right = labelRect.right;

            SetWindowPos(hwndDlg, NULL, 0, 0, MinimumSize.right, MinimumSize.bottom, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);

            EtpLoadNodeBitMap();
        }
        break;
    case WM_DESTROY:
        {
            ULONG i;

            EtpSaveNodeBitMap();

            PhUnregisterCallback(&PhProcessesUpdatedEvent, &ProcessesUpdatedCallbackRegistration);

            for (i = 0; i < EtGpuTotalNodeCount; i++)
            {
                PhDeleteGraphState(&GraphState[i]);
            }

            PhFree(GraphHandle);
            PhFree(CheckBoxHandle);
            PhFree(GraphState);

            PhDeleteLayoutManager(&LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            HDWP deferHandle;
            RECT clientRect;
            RECT checkBoxRect;
            ULONG numberOfRows = (ULONG)sqrt(EtGpuTotalNodeCount);
            ULONG numberOfColumns = (EtGpuTotalNodeCount + numberOfRows - 1) / numberOfRows;
            ULONG numberOfYPaddings = numberOfRows - 1;
            ULONG numberOfXPaddings = numberOfColumns - 1;
            ULONG cellHeight;
            ULONG y;
            ULONG cellWidth;
            ULONG x;
            ULONG i;

            PhLayoutManagerLayout(&LayoutManager);

            deferHandle = BeginDeferWindowPos(EtGpuTotalNodeCount * 2);

            GetClientRect(hwndDlg, &clientRect);
            GetClientRect(GetDlgItem(hwndDlg, IDC_EXAMPLE), &checkBoxRect);
            cellHeight = (clientRect.bottom - LayoutMargin.top - LayoutMargin.bottom - GRAPH_PADDING * numberOfYPaddings) / numberOfRows;
            y = LayoutMargin.top;
            i = 0;

            for (ULONG row = 0; row < numberOfRows; ++row)
            {
                // Give the last row the remaining space; the height we calculated might be off by a few
                // pixels due to integer division.
                if (row == numberOfRows - 1)
                    cellHeight = clientRect.bottom - LayoutMargin.bottom - y;

                cellWidth = (clientRect.right - LayoutMargin.left - LayoutMargin.right - GRAPH_PADDING * numberOfXPaddings) / numberOfColumns;
                x = LayoutMargin.left;

                for (ULONG column = 0; column < numberOfColumns; column++)
                {
                    // Give the last cell the remaining space; the width we calculated might be off by a few
                    // pixels due to integer division.
                    if (column == numberOfColumns - 1)
                        cellWidth = clientRect.right - LayoutMargin.right - x;

                    if (i < EtGpuTotalNodeCount)
                    {
                        deferHandle = DeferWindowPos(
                            deferHandle,
                            GraphHandle[i],
                            NULL,
                            x,
                            y,
                            cellWidth,
                            cellHeight - checkBoxRect.bottom - CHECKBOX_PADDING,
                            SWP_NOACTIVATE | SWP_NOZORDER
                            );
                        deferHandle = DeferWindowPos(
                            deferHandle,
                            CheckBoxHandle[i],
                            NULL,
                            x,
                            y + cellHeight - checkBoxRect.bottom,
                            cellWidth,
                            checkBoxRect.bottom,
                            SWP_NOACTIVATE | SWP_NOZORDER
                            );
                        i++;
                    }
                    x += cellWidth + GRAPH_PADDING;
                }

                y += cellHeight + GRAPH_PADDING;
            }

            EndDeferWindowPos(deferHandle);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, MinimumSize.right, MinimumSize.bottom);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                {
                    EndDialog(hwndDlg, IDOK);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            NMHDR *header = (NMHDR *)lParam;
            ULONG i;

            switch (header->code)
            {
            case GCN_GETDRAWINFO:
                {
                    PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)header;
                    PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

                    drawInfo->Flags = PH_GRAPH_USE_GRID;
                    SysInfoParameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), 0);

                    for (i = 0; i < EtGpuTotalNodeCount; i++)
                    {
                        if (header->hwndFrom == GraphHandle[i])
                        {
                            PhGraphStateGetDrawInfo(
                                &GraphState[i],
                                getDrawInfo,
                                EtGpuNodesHistory[i].Count
                                );

                            if (!GraphState[i].Valid)
                            {
                                PhCopyCircularBuffer_FLOAT(&EtGpuNodesHistory[i], GraphState[i].Data1, drawInfo->LineDataCount);
                                GraphState[i].Valid = TRUE;
                            }

                            break;
                        }
                    }
                }
                break;
            case GCN_GETTOOLTIPTEXT:
                {
                    PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)header;

                    if (getTooltipText->Index < getTooltipText->TotalCount)
                    {
                        for (i = 0; i < EtGpuTotalNodeCount; i++)
                        {
                            if (header->hwndFrom == GraphHandle[i])
                            {
                                if (GraphState[i].TooltipIndex != getTooltipText->Index)
                                {
                                    FLOAT gpu;
                                    ULONG adapterIndex;
                                    PPH_STRING adapterDescription;

                                    gpu = PhGetItemCircularBuffer_FLOAT(&EtGpuNodesHistory[i], getTooltipText->Index);
                                    adapterIndex = EtGetGpuAdapterIndexFromNodeIndex(i);

                                    if (adapterIndex != -1)
                                    {
                                        adapterDescription = EtGetGpuAdapterDescription(adapterIndex);

                                        if (adapterDescription && adapterDescription->Length == 0)
                                            PhClearReference(&adapterDescription);

                                        if (!adapterDescription)
                                            adapterDescription = PhFormatString(L"Adapter %lu", adapterIndex);
                                    }
                                    else
                                    {
                                        adapterDescription = PhCreateString(L"Unknown Adapter");
                                    }

                                    PhMoveReference(&GraphState[i].TooltipText, PhFormatString(
                                        L"Node %lu on %s\n%.2f%%\n%s",
                                        i,
                                        adapterDescription->Buffer,
                                        gpu * 100,
                                        ((PPH_STRING)PhAutoDereferenceObject(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                                        ));
                                    PhDereferenceObject(adapterDescription);
                                }

                                getTooltipText->Text = GraphState[i].TooltipText->sr;

                                break;
                            }
                        }
                    }
                }
                break;
            }
        }
        break;
    case UPDATE_MSG:
        {
            ULONG i;

            for (i = 0; i < EtGpuTotalNodeCount; i++)
            {
                GraphState[i].Valid = FALSE;
                GraphState[i].TooltipIndex = -1;
                Graph_MoveGrid(GraphHandle[i], 1);
                Graph_Draw(GraphHandle[i]);
                Graph_UpdateTooltip(GraphHandle[i]);
                InvalidateRect(GraphHandle[i], NULL, FALSE);
            }
        }
        break;
    }

    return FALSE;
}
