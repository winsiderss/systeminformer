/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2016
 *     dmex    2017-2023
 *
 */

#include <phapp.h>
#include <sysinfo.h>
#include <sysinfop.h>

#include <procprv.h>
#include <phsettings.h>

static PPH_SYSINFO_SECTION IoSection;
static HWND IoDialog;
static PH_LAYOUT_MANAGER IoLayoutManager;
static RECT IoGraphMargin;
static HWND IoGraphHandle;
static PH_GRAPH_STATE IoGraphState;
static HWND IoPanel;
static ULONG IoTicked;
static PH_UINT64_DELTA IoReadDelta;
static PH_UINT64_DELTA IoWriteDelta;
static PH_UINT64_DELTA IoOtherDelta;
static HWND IoPanelReadsDeltaLabel;
static HWND IoPanelWritesDeltaLabel;
static HWND IoPanelOtherDeltaLabel;
static HWND IoPanelReadBytesDeltaLabel;
static HWND IoPanelWriteBytesDeltaLabel;
static HWND IoPanelOtherBytesDeltaLabel;
static HWND IoPanelReadsLabel;
static HWND IoPanelReadBytesLabel;
static HWND IoPanelWritesLabel;
static HWND IoPanelWriteBytesLabel;
static HWND IoPanelOtherLabel;
static HWND IoPanelOtherBytesLabel;

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
    case SysInfoViewChanging:
        {
            PH_SYSINFO_VIEW_TYPE view = (PH_SYSINFO_VIEW_TYPE)Parameter1;
            PPH_SYSINFO_SECTION section = (PPH_SYSINFO_SECTION)Parameter2;

            if (view == SysInfoSummaryView || section != Section)
                return TRUE;

            if (IoGraphHandle)
            {
                IoGraphState.Valid = FALSE;
                IoGraphState.TooltipIndex = ULONG_MAX;
                Graph_Draw(IoGraphHandle);
            }
        }
        return TRUE;
    case SysInfoCreateDialog:
        {
            PPH_SYSINFO_CREATE_DIALOG createDialog = Parameter1;

            if (!createDialog)
                break;

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

            if (!drawInfo)
                break;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y | PH_GRAPH_USE_LINE_2;
            Section->Parameters->ColorSetupFunction(drawInfo, PhCsColorIoReadOther, PhCsColorIoWrite, Section->Parameters->WindowDpi);
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
            PH_FORMAT format[9];

            if (!getTooltipText)
                break;

            ioRead = PhGetItemCircularBuffer_ULONG64(&PhIoReadHistory, getTooltipText->Index);
            ioWrite = PhGetItemCircularBuffer_ULONG64(&PhIoWriteHistory, getTooltipText->Index);
            ioOther = PhGetItemCircularBuffer_ULONG64(&PhIoOtherHistory, getTooltipText->Index);

            // R: %s\nW: %s\nO: %s%s\n%s
            PhInitFormatS(&format[0], L"R: ");
            PhInitFormatSize(&format[1], ioRead);
            PhInitFormatS(&format[2], L"\nW: ");
            PhInitFormatSize(&format[3], ioWrite);
            PhInitFormatS(&format[4], L"\nO: ");
            PhInitFormatSize(&format[5], ioOther);
            PhInitFormatSR(&format[6], PH_AUTO_T(PH_STRING, PhSipGetMaxIoString(getTooltipText->Index))->sr);
            PhInitFormatC(&format[7], L'\n');
            PhInitFormatSR(&format[8], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

            PhMoveReference(&Section->GraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 160));
            getTooltipText->Text = Section->GraphState.TooltipText->sr;
        }
        return TRUE;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = Parameter1;
            PH_FORMAT format[4];

            if (!drawPanel)
                break;

            drawPanel->Title = PhCreateString(L"I/O");

            // R+O: %s\nW: %s
            PhInitFormatS(&format[0], L"R+O: ");
            PhInitFormatSizeWithPrecision(&format[1], PhIoReadDelta.Delta + PhIoOtherDelta.Delta, 1);
            PhInitFormatS(&format[2], L"\nW: ");
            PhInitFormatSizeWithPrecision(&format[3], PhIoWriteDelta.Delta, 1);

            drawPanel->SubTitle = PhFormat(format, RTL_NUMBER_OF(format), 64);
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

    // Note: Required for SysInfoViewChanging (dmex)
    IoGraphHandle = NULL;
}

VOID PhSipTickIoDialog(
    VOID
    )
{
    PhUpdateDelta(&IoReadDelta, PhPerfInformation.IoReadOperationCount);
    PhUpdateDelta(&IoWriteDelta, PhPerfInformation.IoWriteOperationCount);
    PhUpdateDelta(&IoOtherDelta, PhPerfInformation.IoOtherOperationCount);

    if (IoTicked < 2)
        IoTicked++;

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
            RECT margin;

            PhSipInitializeIoDialog();

            IoDialog = hwndDlg;
            PhInitializeLayoutManager(&IoLayoutManager, hwndDlg);
            graphItem = PhAddLayoutItem(&IoLayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            panelItem = PhAddLayoutItem(&IoLayoutManager, GetDlgItem(hwndDlg, IDC_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            IoGraphMargin = graphItem->Margin;

            SetWindowFont(GetDlgItem(hwndDlg, IDC_TITLE), IoSection->Parameters->LargeFont, FALSE);

            IoPanel = PhCreateDialog(PhInstanceHandle, MAKEINTRESOURCE(IDD_SYSINFO_IOPANEL), hwndDlg, PhSipIoPanelDialogProc, NULL);
            ShowWindow(IoPanel, SW_SHOW);

            margin = panelItem->Margin;
            PhGetSizeDpiValue(&margin, IoSection->Parameters->WindowDpi, TRUE);
            PhAddLayoutItemEx(&IoLayoutManager, IoPanel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, margin);

            PhSipCreateIoGraph();
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
            IoGraphState.Valid = FALSE;
            IoGraphState.TooltipIndex = ULONG_MAX;

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
    case WM_DPICHANGED_AFTERPARENT:
        {
            if (IoSection->Parameters->LargeFont)
            {
                SetWindowFont(GetDlgItem(hwndDlg, IDC_TITLE), IoSection->Parameters->LargeFont, FALSE);
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
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
            IoPanelReadsDeltaLabel = GetDlgItem(hwndDlg, IDC_ZREADSDELTA_V);
            IoPanelWritesDeltaLabel = GetDlgItem(hwndDlg, IDC_ZWRITESDELTA_V);
            IoPanelOtherDeltaLabel = GetDlgItem(hwndDlg, IDC_ZOTHERDELTA_V);
            IoPanelReadBytesDeltaLabel = GetDlgItem(hwndDlg, IDC_ZREADBYTESDELTA_V);
            IoPanelWriteBytesDeltaLabel = GetDlgItem(hwndDlg, IDC_ZWRITEBYTESDELTA_V);
            IoPanelOtherBytesDeltaLabel = GetDlgItem(hwndDlg, IDC_ZOTHERBYTESDELTA_V);
            IoPanelReadsLabel = GetDlgItem(hwndDlg, IDC_ZREADS_V);
            IoPanelReadBytesLabel = GetDlgItem(hwndDlg, IDC_ZREADBYTES_V);
            IoPanelWritesLabel = GetDlgItem(hwndDlg, IDC_ZWRITES_V);
            IoPanelWriteBytesLabel = GetDlgItem(hwndDlg, IDC_ZWRITEBYTES_V);
            IoPanelOtherLabel = GetDlgItem(hwndDlg, IDC_ZOTHER_V);
            IoPanelOtherBytesLabel = GetDlgItem(hwndDlg, IDC_ZOTHERBYTES_V);
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

VOID PhSipCreateIoGraph(
    VOID
    )
{
    IoGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        IoDialog,
        NULL,
        PhInstanceHandle,
        NULL
        );
    Graph_SetTooltip(IoGraphHandle, TRUE);

    PhGetSizeDpiValue(&IoGraphMargin, IoSection->Parameters->WindowDpi, TRUE);
    PhAddLayoutItemEx(&IoLayoutManager, IoGraphHandle, NULL, PH_ANCHOR_ALL, IoGraphMargin);
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
            PhSiSetColorsGraphDrawInfo(drawInfo, PhCsColorIoReadOther, PhCsColorIoWrite, IoSection->Parameters->WindowDpi);

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
                    PH_FORMAT format[9];

                    ioRead = PhGetItemCircularBuffer_ULONG64(&PhIoReadHistory, getTooltipText->Index);
                    ioWrite = PhGetItemCircularBuffer_ULONG64(&PhIoWriteHistory, getTooltipText->Index);
                    ioOther = PhGetItemCircularBuffer_ULONG64(&PhIoOtherHistory, getTooltipText->Index);

                    // R: %s\nW: %s\nO: %s%s\n%s
                    PhInitFormatS(&format[0], L"R: ");
                    PhInitFormatSize(&format[1], ioRead);
                    PhInitFormatS(&format[2], L"\nW: ");
                    PhInitFormatSize(&format[3], ioWrite);
                    PhInitFormatS(&format[4], L"\nO: ");
                    PhInitFormatSize(&format[5], ioOther);
                    PhInitFormatSR(&format[6], PH_AUTO_T(PH_STRING, PhSipGetMaxIoString(getTooltipText->Index))->sr);
                    PhInitFormatC(&format[7], L'\n');
                    PhInitFormatSR(&format[8], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&IoGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 160));
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
    IoGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(IoGraphHandle, 1);
    Graph_Draw(IoGraphHandle);
    Graph_UpdateTooltip(IoGraphHandle);
    InvalidateRect(IoGraphHandle, NULL, FALSE);
}

VOID PhSipUpdateIoPanel(
    VOID
    )
{
    PH_FORMAT format[1];
    WCHAR formatBuffer[256];

    // I/O Deltas

    if (IoTicked > 1)
    {
        PhInitFormatI64UGroupDigits(&format[0], IoReadDelta.Delta);

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
            PhSetWindowText(IoPanelReadsDeltaLabel, formatBuffer);
        else
            PhSetWindowText(IoPanelReadsDeltaLabel, PhaFormatUInt64(IoReadDelta.Delta, TRUE)->Buffer);

        PhInitFormatI64UGroupDigits(&format[0], IoWriteDelta.Delta);

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
            PhSetWindowText(IoPanelWritesDeltaLabel, formatBuffer);
        else
            PhSetWindowText(IoPanelWritesDeltaLabel, PhaFormatUInt64(IoWriteDelta.Delta, TRUE)->Buffer);

        PhInitFormatI64UGroupDigits(&format[0], IoOtherDelta.Delta);

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
            PhSetWindowText(IoPanelOtherDeltaLabel, formatBuffer);
        else
            PhSetWindowText(IoPanelOtherDeltaLabel, PhaFormatUInt64(IoOtherDelta.Delta, TRUE)->Buffer);
    }
    else
    {
        PhSetWindowText(IoPanelReadsDeltaLabel, L"-");
        PhSetWindowText(IoPanelWritesDeltaLabel, L"-");
        PhSetWindowText(IoPanelOtherDeltaLabel, L"-");
    }

    if (PhIoReadHistory.Count != 0)
    {
        PhInitFormatSize(&format[0], PhIoReadDelta.Delta);

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
            PhSetWindowText(IoPanelReadBytesDeltaLabel, formatBuffer);
        else
            PhSetWindowText(IoPanelReadBytesDeltaLabel, PhaFormatSize(PhIoReadDelta.Delta, ULONG_MAX)->Buffer);

        PhInitFormatSize(&format[0], PhIoWriteDelta.Delta);

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
            PhSetWindowText(IoPanelWriteBytesDeltaLabel, formatBuffer);
        else
            PhSetWindowText(IoPanelWriteBytesDeltaLabel, PhaFormatSize(PhIoWriteDelta.Delta, ULONG_MAX)->Buffer);

        PhInitFormatSize(&format[0], PhIoOtherDelta.Delta);

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
            PhSetWindowText(IoPanelOtherBytesDeltaLabel, formatBuffer);
        else
            PhSetWindowText(IoPanelOtherBytesDeltaLabel, PhaFormatSize(PhIoOtherDelta.Delta, ULONG_MAX)->Buffer);
    }
    else
    {
        PhSetWindowText(IoPanelReadBytesDeltaLabel, L"-");
        PhSetWindowText(IoPanelWriteBytesDeltaLabel, L"-");
        PhSetWindowText(IoPanelOtherBytesDeltaLabel, L"-");
    }

    // I/O Totals

    PhInitFormatI64UGroupDigits(&format[0], PhPerfInformation.IoReadOperationCount);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(IoPanelReadsLabel, formatBuffer);
    else
        PhSetWindowText(IoPanelReadsLabel, PhaFormatUInt64(PhPerfInformation.IoReadOperationCount, TRUE)->Buffer);

    PhInitFormatSize(&format[0], PhPerfInformation.IoReadTransferCount.QuadPart);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(IoPanelReadBytesLabel, formatBuffer);
    else
        PhSetWindowText(IoPanelReadBytesLabel, PhaFormatSize(PhPerfInformation.IoReadTransferCount.QuadPart, ULONG_MAX)->Buffer);

    PhInitFormatI64UGroupDigits(&format[0], PhPerfInformation.IoWriteOperationCount);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(IoPanelWritesLabel, formatBuffer);
    else
        PhSetWindowText(IoPanelWritesLabel, PhaFormatUInt64(PhPerfInformation.IoWriteOperationCount, TRUE)->Buffer);

    PhInitFormatSize(&format[0], PhPerfInformation.IoWriteTransferCount.QuadPart);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(IoPanelWriteBytesLabel, formatBuffer);
    else
        PhSetWindowText(IoPanelWriteBytesLabel, PhaFormatSize(PhPerfInformation.IoWriteTransferCount.QuadPart, ULONG_MAX)->Buffer);

    PhInitFormatI64UGroupDigits(&format[0], PhPerfInformation.IoOtherOperationCount);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(IoPanelOtherLabel, formatBuffer);
    else
        PhSetWindowText(IoPanelOtherLabel, PhaFormatUInt64(PhPerfInformation.IoOtherOperationCount, TRUE)->Buffer);

    PhInitFormatSize(&format[0], PhPerfInformation.IoOtherTransferCount.QuadPart);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(IoPanelOtherBytesLabel, formatBuffer);
    else
        PhSetWindowText(IoPanelOtherBytesLabel, PhaFormatSize(PhPerfInformation.IoOtherTransferCount.QuadPart, ULONG_MAX)->Buffer);
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

    if (maxProcessRecord = PhSipReferenceMaxIoRecord(Index))
    {
        PPH_STRING maxUsageString;

        // We found the process record, so now we construct the max. usage string.
#ifdef PH_RECORD_MAX_USAGE
        maxIoReadOther = PhGetItemCircularBuffer_ULONG64(&PhMaxIoReadOtherHistory, Index);
        maxIoWrite = PhGetItemCircularBuffer_ULONG64(&PhMaxIoWriteHistory, Index);

        if (!PH_IS_FAKE_PROCESS_ID(maxProcessRecord->ProcessId))
        {
            PH_FORMAT format[8];

            // \n%s (%u): R+O: %s, W: %s
            PhInitFormatC(&format[0], L'\n');
            PhInitFormatSR(&format[1], maxProcessRecord->ProcessName->sr);
            PhInitFormatS(&format[2], L" (");
            PhInitFormatU(&format[3], HandleToUlong(maxProcessRecord->ProcessId));
            PhInitFormatS(&format[4], L"): R+O: ");
            PhInitFormatSize(&format[5], maxIoReadOther);
            PhInitFormatS(&format[6], L", W: ");
            PhInitFormatSize(&format[7], maxIoWrite);

            maxUsageString = PhFormat(format, RTL_NUMBER_OF(format), 128);
        }
        else
        {
            PH_FORMAT format[6];

            // \n%s: R+O: %s, W: %s
            PhInitFormatC(&format[0], L'\n');
            PhInitFormatSR(&format[1], maxProcessRecord->ProcessName->sr);
            PhInitFormatS(&format[2], L": R+O: ");
            PhInitFormatSize(&format[3], maxIoReadOther);
            PhInitFormatS(&format[4], L", W: ");
            PhInitFormatSize(&format[5], maxIoWrite);

            maxUsageString = PhFormat(format, RTL_NUMBER_OF(format), 128);
        }
#else
        PH_FORMAT format[2];

        PhInitFormatC(&format[0], L'\n');
        PhInitFormatSR(&format[1], maxProcessRecord->ProcessName->sr);

        maxUsageString = PhFormat(format, RTL_NUMBER_OF(format), 128);
#endif

        PhDereferenceProcessRecord(maxProcessRecord);

        return maxUsageString;
    }

    return PhReferenceEmptyString();
}
