/*
 * Process Hacker -
 *   System Information I/O section
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
#include <sysinfop.h>

static PPH_SYSINFO_SECTION IoSection;
static HWND IoDialog;
static PH_LAYOUT_MANAGER IoLayoutManager;
static HWND IoGraphHandle;
static PH_GRAPH_STATE IoGraphState;
static HWND IoPanel;
static ULONG IoTicked;
static PH_UINT64_DELTA IoReadDelta;
static PH_UINT64_DELTA IoWriteDelta;
static PH_UINT64_DELTA IoOtherDelta;

BOOLEAN PhSipIoSectionCallback(
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
            IoSection = Section;
        }
        return TRUE;
    case SysInfoDestroy:
        {
            if (IoDialog)
            {
                PhSipUninitializeIoDialog();
                IoDialog = NULL;
            }
        }
        break;
    case SysInfoTick:
        {
            if (IoDialog)
            {
                PhSipTickIoDialog();
            }
        }
        break;
    case SysInfoCreateDialog:
        {
            PPH_SYSINFO_CREATE_DIALOG createDialog = Parameter1;

            createDialog->Instance = PhInstanceHandle;
            createDialog->Template = MAKEINTRESOURCE(IDD_SYSINFO_IO);
            createDialog->DialogProc = PhSipIoDialogProc;
        }
        return TRUE;
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = Parameter1;
            ULONG i;
            FLOAT max;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y | PH_GRAPH_USE_LINE_2;
            Section->Parameters->ColorSetupFunction(drawInfo, PhCsColorIoReadOther, PhCsColorIoWrite);
            PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, PhIoReadHistory.Count);

            if (!Section->GraphState.Valid)
            {
                max = 1024 * 1024; // Minimum scaling of 1 MB

                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data1;
                    FLOAT data2;

                    Section->GraphState.Data1[i] = data1 =
                        (FLOAT)PhGetItemCircularBuffer_ULONG64(&PhIoReadHistory, i) +
                        (FLOAT)PhGetItemCircularBuffer_ULONG64(&PhIoOtherHistory, i);
                    Section->GraphState.Data2[i] = data2 =
                        (FLOAT)PhGetItemCircularBuffer_ULONG64(&PhIoWriteHistory, i);

                    if (max < data1 + data2)
                        max = data1 + data2;
                }

                if (max != 0)
                {
                    // Scale the data.

                    PhDivideSinglesBySingle(
                        Section->GraphState.Data1,
                        max,
                        drawInfo->LineDataCount
                        );
                    PhDivideSinglesBySingle(
                        Section->GraphState.Data2,
                        max,
                        drawInfo->LineDataCount
                        );
                }

                drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                drawInfo->LabelYFunctionParameter = max;

                Section->GraphState.Valid = TRUE;
            }
        }
        return TRUE;
    case SysInfoGraphGetTooltipText:
        {
            PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT getTooltipText = Parameter1;
            ULONG64 ioRead;
            ULONG64 ioWrite;
            ULONG64 ioOther;

            ioRead = PhGetItemCircularBuffer_ULONG64(&PhIoReadHistory, getTooltipText->Index);
            ioWrite = PhGetItemCircularBuffer_ULONG64(&PhIoWriteHistory, getTooltipText->Index);
            ioOther = PhGetItemCircularBuffer_ULONG64(&PhIoOtherHistory, getTooltipText->Index);

            PhMoveReference(&Section->GraphState.TooltipText, PhFormatString(
                L"R: %s\nW: %s\nO: %s%s\n%s",
                PhaFormatSize(ioRead, -1)->Buffer,
                PhaFormatSize(ioWrite, -1)->Buffer,
                PhaFormatSize(ioOther, -1)->Buffer,
                PhGetStringOrEmpty(PhSipGetMaxIoString(getTooltipText->Index)),
                PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->Buffer
                ));
            getTooltipText->Text = Section->GraphState.TooltipText->sr;
        }
        return TRUE;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = Parameter1;

            drawPanel->Title = PhCreateString(L"I/O");
            drawPanel->SubTitle = PhFormatString(
                L"R+O: %s\nW: %s",
                PhSipFormatSizeWithPrecision(PhIoReadDelta.Delta + PhIoOtherDelta.Delta, 1)->Buffer,
                PhSipFormatSizeWithPrecision(PhIoWriteDelta.Delta, 1)->Buffer
                );
        }
        return TRUE;
    }

    return FALSE;
}

VOID PhSipInitializeIoDialog(
    VOID
    )
{
    PhInitializeDelta(&IoReadDelta);
    PhInitializeDelta(&IoWriteDelta);
    PhInitializeDelta(&IoOtherDelta);

    PhInitializeGraphState(&IoGraphState);

    IoTicked = 0;
}

VOID PhSipUninitializeIoDialog(
    VOID
    )
{
    PhDeleteGraphState(&IoGraphState);
}

VOID PhSipTickIoDialog(
    VOID
    )
{
    PhUpdateDelta(&IoReadDelta, PhPerfInformation.IoReadOperationCount);
    PhUpdateDelta(&IoWriteDelta, PhPerfInformation.IoWriteOperationCount);
    PhUpdateDelta(&IoOtherDelta, PhPerfInformation.IoOtherOperationCount);

    IoTicked++;

    if (IoTicked > 2)
        IoTicked = 2;

    PhSipUpdateIoGraph();
    PhSipUpdateIoPanel();
}

INT_PTR CALLBACK PhSipIoDialogProc(
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

            PhSipInitializeIoDialog();

            IoDialog = hwndDlg;
            PhInitializeLayoutManager(&IoLayoutManager, hwndDlg);
            graphItem = PhAddLayoutItem(&IoLayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            panelItem = PhAddLayoutItem(&IoLayoutManager, GetDlgItem(hwndDlg, IDC_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            SendMessage(GetDlgItem(hwndDlg, IDC_TITLE), WM_SETFONT, (WPARAM)IoSection->Parameters->LargeFont, FALSE);

            IoPanel = CreateDialog(PhInstanceHandle, MAKEINTRESOURCE(IDD_SYSINFO_IOPANEL), hwndDlg, PhSipIoPanelDialogProc);
            ShowWindow(IoPanel, SW_SHOW);
            PhAddLayoutItemEx(&IoLayoutManager, IoPanel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, panelItem->Margin);

            IoGraphHandle = CreateWindow(
                PH_GRAPH_CLASSNAME,
                NULL,
                WS_VISIBLE | WS_CHILD | WS_BORDER,
                0,
                0,
                3,
                3,
                IoDialog,
                (HMENU)IDC_IO,
                PhInstanceHandle,
                NULL
                );
            Graph_SetTooltip(IoGraphHandle, TRUE);

            PhAddLayoutItemEx(&IoLayoutManager, IoGraphHandle, NULL, PH_ANCHOR_ALL, graphItem->Margin);

            PhSipUpdateIoGraph();
            PhSipUpdateIoPanel();
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&IoLayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&IoLayoutManager);
        }
        break;
    case WM_NOTIFY:
        {
            NMHDR *header = (NMHDR *)lParam;

            if (header->hwndFrom == IoGraphHandle)
            {
                PhSipNotifyIoGraph(header);
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PhSipIoPanelDialogProc(
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
            NOTHING;
        }
        break;
    }

    return FALSE;
}

VOID PhSipNotifyIoGraph(
    _In_ NMHDR *Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;
            ULONG i;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y | PH_GRAPH_USE_LINE_2;
            PhSiSetColorsGraphDrawInfo(drawInfo, PhCsColorIoReadOther, PhCsColorIoWrite);

            PhGraphStateGetDrawInfo(
                &IoGraphState,
                getDrawInfo,
                PhIoReadHistory.Count
                );

            if (!IoGraphState.Valid)
            {
                FLOAT max = 1024 * 1024; // Minimum scaling of 1 MB

                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data1;
                    FLOAT data2;

                    IoGraphState.Data1[i] = data1 =
                        (FLOAT)PhGetItemCircularBuffer_ULONG64(&PhIoReadHistory, i) +
                        (FLOAT)PhGetItemCircularBuffer_ULONG64(&PhIoOtherHistory, i);
                    IoGraphState.Data2[i] = data2 =
                        (FLOAT)PhGetItemCircularBuffer_ULONG64(&PhIoWriteHistory, i);

                    if (max < data1 + data2)
                        max = data1 + data2;
                }

                if (max != 0)
                {
                    // Scale the data.

                    PhDivideSinglesBySingle(
                        IoGraphState.Data1,
                        max,
                        drawInfo->LineDataCount
                        );
                    PhDivideSinglesBySingle(
                        IoGraphState.Data2,
                        max,
                        drawInfo->LineDataCount
                        );
                }

                drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                drawInfo->LabelYFunctionParameter = max;

                IoGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (IoGraphState.TooltipIndex != getTooltipText->Index)
                {
                    ULONG64 ioRead;
                    ULONG64 ioWrite;
                    ULONG64 ioOther;

                    ioRead = PhGetItemCircularBuffer_ULONG64(&PhIoReadHistory, getTooltipText->Index);
                    ioWrite = PhGetItemCircularBuffer_ULONG64(&PhIoWriteHistory, getTooltipText->Index);
                    ioOther = PhGetItemCircularBuffer_ULONG64(&PhIoOtherHistory, getTooltipText->Index);

                    PhMoveReference(&IoGraphState.TooltipText, PhFormatString(
                        L"R: %s\nW: %s\nO: %s%s\n%s",
                        PhaFormatSize(ioRead, -1)->Buffer,
                        PhaFormatSize(ioWrite, -1)->Buffer,
                        PhaFormatSize(ioOther, -1)->Buffer,
                        PhGetStringOrEmpty(PhSipGetMaxIoString(getTooltipText->Index)),
                        PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->Buffer
                        ));
                }

                getTooltipText->Text = IoGraphState.TooltipText->sr;
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
                record = PhSipReferenceMaxIoRecord(mouseEvent->Index);
            }

            if (record)
            {
                PhShowProcessRecordDialog(IoDialog, record);
                PhDereferenceProcessRecord(record);
            }
        }
        break;
    }
}

VOID PhSipUpdateIoGraph(
    VOID
    )
{
    IoGraphState.Valid = FALSE;
    IoGraphState.TooltipIndex = -1;
    Graph_MoveGrid(IoGraphHandle, 1);
    Graph_Draw(IoGraphHandle);
    Graph_UpdateTooltip(IoGraphHandle);
    InvalidateRect(IoGraphHandle, NULL, FALSE);
}

VOID PhSipUpdateIoPanel(
    VOID
    )
{
    // I/O Deltas

    if (IoTicked > 1)
    {
        SetDlgItemText(IoPanel, IDC_ZREADSDELTA_V, PhaFormatUInt64(IoReadDelta.Delta, TRUE)->Buffer);
        SetDlgItemText(IoPanel, IDC_ZWRITESDELTA_V, PhaFormatUInt64(IoWriteDelta.Delta, TRUE)->Buffer);
        SetDlgItemText(IoPanel, IDC_ZOTHERDELTA_V, PhaFormatUInt64(IoOtherDelta.Delta, TRUE)->Buffer);
    }
    else
    {
        SetDlgItemText(IoPanel, IDC_ZREADSDELTA_V, L"-");
        SetDlgItemText(IoPanel, IDC_ZWRITESDELTA_V, L"-");
        SetDlgItemText(IoPanel, IDC_ZOTHERDELTA_V, L"-");
    }

    if (PhIoReadHistory.Count != 0)
    {
        SetDlgItemText(IoPanel, IDC_ZREADBYTESDELTA_V, PhaFormatSize(PhIoReadDelta.Delta, -1)->Buffer);
        SetDlgItemText(IoPanel, IDC_ZWRITEBYTESDELTA_V, PhaFormatSize(PhIoWriteDelta.Delta, -1)->Buffer);
        SetDlgItemText(IoPanel, IDC_ZOTHERBYTESDELTA_V, PhaFormatSize(PhIoOtherDelta.Delta, -1)->Buffer);
    }
    else
    {
        SetDlgItemText(IoPanel, IDC_ZREADBYTESDELTA_V, L"-");
        SetDlgItemText(IoPanel, IDC_ZWRITEBYTESDELTA_V, L"-");
        SetDlgItemText(IoPanel, IDC_ZOTHERBYTESDELTA_V, L"-");
    }

    // I/O Totals

    SetDlgItemText(IoPanel, IDC_ZREADS_V, PhaFormatUInt64(PhPerfInformation.IoReadOperationCount, TRUE)->Buffer);
    SetDlgItemText(IoPanel, IDC_ZREADBYTES_V, PhaFormatSize(PhPerfInformation.IoReadTransferCount.QuadPart, -1)->Buffer);
    SetDlgItemText(IoPanel, IDC_ZWRITES_V, PhaFormatUInt64(PhPerfInformation.IoWriteOperationCount, TRUE)->Buffer);
    SetDlgItemText(IoPanel, IDC_ZWRITEBYTES_V, PhaFormatSize(PhPerfInformation.IoWriteTransferCount.QuadPart, -1)->Buffer);
    SetDlgItemText(IoPanel, IDC_ZOTHER_V, PhaFormatUInt64(PhPerfInformation.IoOtherOperationCount, TRUE)->Buffer);
    SetDlgItemText(IoPanel, IDC_ZOTHERBYTES_V, PhaFormatSize(PhPerfInformation.IoOtherTransferCount.QuadPart, -1)->Buffer);
}

PPH_PROCESS_RECORD PhSipReferenceMaxIoRecord(
    _In_ LONG Index
    )
{
    LARGE_INTEGER time;
    ULONG maxProcessId;

    // Find the process record for the max. I/O process for the particular time.

    maxProcessId = PhGetItemCircularBuffer_ULONG(&PhMaxIoHistory, Index);

    if (!maxProcessId)
        return NULL;

    // See above for the explanation.
    PhGetStatisticsTime(NULL, Index, &time);
    time.QuadPart += PH_TICKS_PER_SEC - 1;

    return PhFindProcessRecord(UlongToHandle(maxProcessId), &time);
}

PPH_STRING PhSipGetMaxIoString(
    _In_ LONG Index
    )
{
    PPH_PROCESS_RECORD maxProcessRecord;
#ifdef PH_RECORD_MAX_USAGE
    ULONG64 maxIoReadOther;
    ULONG64 maxIoWrite;
#endif
    PPH_STRING maxUsageString = NULL;

    if (maxProcessRecord = PhSipReferenceMaxIoRecord(Index))
    {
        // We found the process record, so now we construct the max. usage string.
#ifdef PH_RECORD_MAX_USAGE
        maxIoReadOther = PhGetItemCircularBuffer_ULONG64(&PhMaxIoReadOtherHistory, Index);
        maxIoWrite = PhGetItemCircularBuffer_ULONG64(&PhMaxIoWriteHistory, Index);

        if (!PH_IS_FAKE_PROCESS_ID(maxProcessRecord->ProcessId))
        {
            maxUsageString = PhaFormatString(
                L"\n%s (%u): R+O: %s, W: %s",
                maxProcessRecord->ProcessName->Buffer,
                HandleToUlong(maxProcessRecord->ProcessId),
                PhaFormatSize(maxIoReadOther, -1)->Buffer,
                PhaFormatSize(maxIoWrite, -1)->Buffer
                );
        }
        else
        {
            maxUsageString = PhaFormatString(
                L"\n%s: R+O: %s, W: %s",
                maxProcessRecord->ProcessName->Buffer,
                PhaFormatSize(maxIoReadOther, -1)->Buffer,
                PhaFormatSize(maxIoWrite, -1)->Buffer
                );
        }
#else
        maxUsageString = PhaConcatStrings2(L"\n", maxProcessRecord->ProcessName->Buffer);
#endif

        PhDereferenceProcessRecord(maxProcessRecord);
    }

    return maxUsageString;
}
