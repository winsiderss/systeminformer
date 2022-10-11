/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2015-2022
 *
 */

#include "exttools.h"
#include "etwsys.h"

static PPH_SYSINFO_SECTION DiskSection;
static HWND DiskDialog;
static PH_LAYOUT_MANAGER DiskLayoutManager;
static RECT DiskGraphMargin;
static HWND DiskGraphHandle;
static PH_GRAPH_STATE DiskGraphState;
static HWND DiskPanel;
static HWND DiskPanelReadsDeltaLabel;
static HWND DiskPanelReadBytesDeltaLabel;
static HWND DiskPanelWritesDeltaLabel;
static HWND DiskPanelWriteBytesDeltaLabel;

static PPH_SYSINFO_SECTION NetworkSection;
static HWND NetworkDialog;
static PH_LAYOUT_MANAGER NetworkLayoutManager;
static RECT NetworkGraphMargin;
static HWND NetworkGraphHandle;
static PH_GRAPH_STATE NetworkGraphState;
static HWND NetworkPanel;
static HWND NetworkPanelReceiveDeltaLabel;
static HWND NetworkPanelReceiveBytesDeltaLabel;
static HWND NetworkPanelSendsDeltaLabel;
static HWND NetworkPanelSendBytesDeltaLabel;

VOID EtEtwSystemInformationInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers
    )
{
    PH_SYSINFO_SECTION section;

    memset(&section, 0, sizeof(PH_SYSINFO_SECTION));
    PhInitializeStringRef(&section.Name, L"Disk");
    section.Flags = 0;
    section.Callback = EtpDiskSysInfoSectionCallback;

    DiskSection = Pointers->CreateSection(&section);

    memset(&section, 0, sizeof(PH_SYSINFO_SECTION));
    PhInitializeStringRef(&section.Name, L"Network");
    section.Flags = 0;
    section.Callback = EtpNetworkSysInfoSectionCallback;

    NetworkSection = Pointers->CreateSection(&section);
}

BOOLEAN EtpDiskSysInfoSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    switch (Message)
    {
    case SysInfoDestroy:
        {
            if (DiskDialog)
            {
                PhDeleteGraphState(&DiskGraphState);
                DiskDialog = NULL;
            }
        }
        return TRUE;
    case SysInfoTick:
        {
            if (DiskDialog)
            {
                EtpUpdateDiskGraph();
                EtpUpdateDiskPanel();
            }
        }
        return TRUE;
    case SysInfoCreateDialog:
        {
            PPH_SYSINFO_CREATE_DIALOG createDialog = Parameter1;

            if (!createDialog)
                break;

            createDialog->Instance = PluginInstance->DllBase;
            createDialog->Template = MAKEINTRESOURCE(IDD_SYSINFO_DISK);
            createDialog->DialogProc = EtpDiskDialogProc;
        }
        return TRUE;
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = Parameter1;
            LONG dpiValue;

            if (!drawInfo)
                break;

            dpiValue = PhGetWindowDpi(Section->GraphHandle);
            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y | PH_GRAPH_USE_LINE_2;
            Section->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), PhGetIntegerSetting(L"ColorIoWrite"), dpiValue);
            PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, EtDiskReadHistory.Count);

            if (!Section->GraphState.Valid)
            {
                ULONG i;
                FLOAT max = 1024 * 1024; // Minimum scaling of 1 MB

                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data1;
                    FLOAT data2;

                    Section->GraphState.Data1[i] = data1 =
                        (FLOAT)PhGetItemCircularBuffer_ULONG(&EtDiskReadHistory, i);
                    Section->GraphState.Data2[i] = data2 =
                        (FLOAT)PhGetItemCircularBuffer_ULONG(&EtDiskWriteHistory, i);

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
            ULONG64 diskRead;
            ULONG64 diskWrite;
            PH_FORMAT format[7];

            if (!getTooltipText)
                break;

            diskRead = PhGetItemCircularBuffer_ULONG(&EtDiskReadHistory, getTooltipText->Index);
            diskWrite = PhGetItemCircularBuffer_ULONG(&EtDiskWriteHistory, getTooltipText->Index);

            // R: %s\nW: %s%s\n%s
            PhInitFormatS(&format[0], L"R: ");
            PhInitFormatSize(&format[1], diskRead);
            PhInitFormatS(&format[2], L"\nW: ");
            PhInitFormatSize(&format[3], diskWrite);
            PhInitFormatSR(&format[4], PH_AUTO_T(PH_STRING, EtpGetMaxDiskString(getTooltipText->Index))->sr);
            PhInitFormatC(&format[5], L'\n');
            PhInitFormatSR(&format[6], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

            PhMoveReference(&Section->GraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 128));
            getTooltipText->Text = PhGetStringRef(Section->GraphState.TooltipText);
        }
        return TRUE;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = Parameter1;
            PH_FORMAT format[4];

            if (!drawPanel)
                break;

            drawPanel->Title = PhCreateString(L"Disk");

            // R: %s\nW: %s
            PhInitFormatS(&format[0], L"R: ");
            PhInitFormatSize(&format[1], EtDiskReadDelta.Delta);
            PhInitFormatS(&format[2], L"\nW: ");
            PhInitFormatSize(&format[3], EtDiskWriteDelta.Delta);

            drawPanel->SubTitle = PhFormat(format, RTL_NUMBER_OF(format), 0);
        }
        return TRUE;
    }

    return FALSE;
}

VOID EtpCreateDiskGraph(
    VOID
    )
{
    DiskGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        DiskDialog,
        NULL,
        NULL,
        NULL
        );
    Graph_SetTooltip(DiskGraphHandle, TRUE);

    PhAddLayoutItemEx(&DiskLayoutManager, DiskGraphHandle, NULL, PH_ANCHOR_ALL, DiskGraphMargin);
}

INT_PTR CALLBACK EtpDiskDialogProc(
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
            LONG dpiValue;

            dpiValue = PhGetWindowDpi(hwndDlg);

            PhInitializeGraphState(&DiskGraphState);

            DiskDialog = hwndDlg;
            PhInitializeLayoutManager(&DiskLayoutManager, hwndDlg);
            graphItem = PhAddLayoutItem(&DiskLayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            panelItem = PhAddLayoutItem(&DiskLayoutManager, GetDlgItem(hwndDlg, IDC_PANEL_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            DiskGraphMargin = graphItem->Margin;
            PhGetSizeDpiValue(&DiskGraphMargin, dpiValue, TRUE);

            SetWindowFont(GetDlgItem(hwndDlg, IDC_TITLE), DiskSection->Parameters->LargeFont, FALSE);

            DiskPanel = PhCreateDialog(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_SYSINFO_DISKPANEL), hwndDlg, EtpDiskPanelDialogProc, NULL);
            ShowWindow(DiskPanel, SW_SHOW);
            PhAddLayoutItemEx(&DiskLayoutManager, DiskPanel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, panelItem->Margin);

            EtpCreateDiskGraph();
            EtpUpdateDiskGraph();
            EtpUpdateDiskPanel();
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&DiskLayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&DiskLayoutManager);

            DiskGraphState.Valid = FALSE;
            DiskGraphState.TooltipIndex = ULONG_MAX;
            if (DiskGraphHandle)
                Graph_Draw(DiskGraphHandle);
        }
        break;
    case WM_NOTIFY:
        {
            NMHDR *header = (NMHDR *)lParam;

            if (header->hwndFrom == DiskGraphHandle)
            {
                EtpNotifyDiskGraph(header);
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

INT_PTR CALLBACK EtpDiskPanelDialogProc(
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
            DiskPanelReadsDeltaLabel = GetDlgItem(hwndDlg, IDC_ZREADSDELTA_V);
            DiskPanelReadBytesDeltaLabel = GetDlgItem(hwndDlg, IDC_ZREADBYTESDELTA_V);
            DiskPanelWritesDeltaLabel = GetDlgItem(hwndDlg, IDC_ZWRITESDELTA_V);
            DiskPanelWriteBytesDeltaLabel = GetDlgItem(hwndDlg, IDC_ZWRITEBYTESDELTA_V);
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

VOID EtpNotifyDiskGraph(
    _In_ NMHDR *Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;
            LONG dpiValue;

            dpiValue = PhGetWindowDpi(Header->hwndFrom);

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y | PH_GRAPH_USE_LINE_2;
            DiskSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), PhGetIntegerSetting(L"ColorIoWrite"), dpiValue);

            PhGraphStateGetDrawInfo(
                &DiskGraphState,
                getDrawInfo,
                EtDiskReadHistory.Count
                );

            if (!DiskGraphState.Valid)
            {
                ULONG i;
                FLOAT max = 1024 * 1024; // Minimum scaling of 1 MB

                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data1;
                    FLOAT data2;

                    DiskGraphState.Data1[i] = data1 =
                        (FLOAT)PhGetItemCircularBuffer_ULONG(&EtDiskReadHistory, i);
                    DiskGraphState.Data2[i] = data2 =
                        (FLOAT)PhGetItemCircularBuffer_ULONG(&EtDiskWriteHistory, i);

                    if (max < data1 + data2)
                        max = data1 + data2;
                }

                if (max != 0)
                {
                    // Scale the data.

                    PhDivideSinglesBySingle(
                        DiskGraphState.Data1,
                        max,
                        drawInfo->LineDataCount
                        );
                    PhDivideSinglesBySingle(
                        DiskGraphState.Data2,
                        max,
                        drawInfo->LineDataCount
                        );
                }

                drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                drawInfo->LabelYFunctionParameter = max;

                DiskGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (DiskGraphState.TooltipIndex != getTooltipText->Index)
                {
                    ULONG64 diskRead;
                    ULONG64 diskWrite;
                    PH_FORMAT format[7];

                    diskRead = PhGetItemCircularBuffer_ULONG(&EtDiskReadHistory, getTooltipText->Index);
                    diskWrite = PhGetItemCircularBuffer_ULONG(&EtDiskWriteHistory, getTooltipText->Index);

                    // R: %s\nW: %s%s\n%s
                    PhInitFormatS(&format[0], L"R: ");
                    PhInitFormatSize(&format[1], diskRead);
                    PhInitFormatS(&format[2], L"\nW: ");
                    PhInitFormatSize(&format[3], diskWrite);
                    PhInitFormatSR(&format[4], PH_AUTO_T(PH_STRING, EtpGetMaxDiskString(getTooltipText->Index))->sr);
                    PhInitFormatC(&format[5], L'\n');
                    PhInitFormatSR(&format[6], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&DiskGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 128));
                }

                getTooltipText->Text = PhGetStringRef(DiskGraphState.TooltipText);
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
                record = EtpReferenceMaxDiskRecord(mouseEvent->Index);
            }

            if (record)
            {
                PhShowProcessRecordDialog(DiskDialog, record);
                PhDereferenceProcessRecord(record);
            }
        }
        break;
    }
}

VOID EtpUpdateDiskGraph(
    VOID
    )
{
    DiskGraphState.Valid = FALSE;
    DiskGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(DiskGraphHandle, 1);
    Graph_Draw(DiskGraphHandle);
    Graph_UpdateTooltip(DiskGraphHandle);
    InvalidateRect(DiskGraphHandle, NULL, FALSE);
}

VOID EtpUpdateDiskPanel(
    VOID
    )
{
    PH_FORMAT format[1];
    WCHAR formatBuffer[256];

    PhInitFormatI64UGroupDigits(&format[0], EtDiskReadCountDelta.Delta);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(DiskPanelReadsDeltaLabel, formatBuffer);
    else
        PhSetWindowText(DiskPanelReadsDeltaLabel, PhaFormatUInt64(EtDiskReadCountDelta.Delta, TRUE)->Buffer);

    PhInitFormatSize(&format[0], EtDiskReadDelta.Delta);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(DiskPanelReadBytesDeltaLabel, formatBuffer);
    else
        PhSetWindowText(DiskPanelReadBytesDeltaLabel, PhaFormatSize(EtDiskReadDelta.Delta, ULONG_MAX)->Buffer);

    PhInitFormatI64UGroupDigits(&format[0], EtDiskWriteCountDelta.Delta);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(DiskPanelWritesDeltaLabel, formatBuffer);
    else
        PhSetWindowText(DiskPanelWritesDeltaLabel, PhaFormatUInt64(EtDiskWriteCountDelta.Delta, TRUE)->Buffer);

    PhInitFormatSize(&format[0], EtDiskWriteDelta.Delta);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(DiskPanelWriteBytesDeltaLabel, formatBuffer);
    else
        PhSetWindowText(DiskPanelWriteBytesDeltaLabel, PhaFormatSize(EtDiskWriteDelta.Delta, ULONG_MAX)->Buffer);
}

PPH_PROCESS_RECORD EtpReferenceMaxDiskRecord(
    _In_ LONG Index
    )
{
    LARGE_INTEGER time;
    ULONG maxProcessId;

    maxProcessId = PhGetItemCircularBuffer_ULONG(&EtMaxDiskHistory, Index);

    if (!maxProcessId)
        return NULL;

    PhGetStatisticsTime(NULL, Index, &time);
    time.QuadPart += PH_TICKS_PER_SEC - 1;

    return PhFindProcessRecord(UlongToHandle(maxProcessId), &time);
}

PPH_STRING EtpGetMaxDiskString(
    _In_ LONG Index
    )
{
    PPH_PROCESS_RECORD maxProcessRecord;
#ifdef PH_RECORD_MAX_USAGE
    ULONG64 maxDiskUsage;
#endif

    if (maxProcessRecord = EtpReferenceMaxDiskRecord(Index))
    {
        PPH_STRING maxUsageString;
#ifdef PH_RECORD_MAX_USAGE
        PH_FORMAT format[6];

        maxDiskUsage = PhGetItemCircularBuffer_ULONG64(&PhMaxDiskUsageHistory, Index);

        // \n%s (%lu): %s
        PhInitFormatC(&format[0], L'\n');
        PhInitFormatSR(&format[1], maxProcessRecord->ProcessName->sr);
        PhInitFormatS(&format[2], L" (");
        PhInitFormatU(&format[3], HandleToUlong(maxProcessRecord->ProcessId));
        PhInitFormatS(&format[4], L"): ");
        PhInitFormatSize(&format[5], maxDiskUsage);
#else
        PH_FORMAT format[5];

        // \n%s (%lu)
        PhInitFormatC(&format[0], L'\n');
        PhInitFormatSR(&format[1], maxProcessRecord->ProcessName->sr);
        PhInitFormatS(&format[2], L" (");
        PhInitFormatU(&format[3], HandleToUlong(maxProcessRecord->ProcessId));
        PhInitFormatC(&format[4], L')');
#endif

        maxUsageString = PhFormat(format, RTL_NUMBER_OF(format), 128);

        PhDereferenceProcessRecord(maxProcessRecord);

        return maxUsageString;
    }

    return PhReferenceEmptyString();
}

BOOLEAN EtpNetworkSysInfoSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    switch (Message)
    {
    case SysInfoDestroy:
        {
            if (NetworkDialog)
            {
                PhDeleteGraphState(&NetworkGraphState);
                NetworkDialog = NULL;
            }
        }
        return TRUE;
    case SysInfoTick:
        {
            if (NetworkDialog)
            {
                EtpUpdateNetworkGraph();
                EtpUpdateNetworkPanel();
            }
        }
        return TRUE;
    case SysInfoCreateDialog:
        {
            PPH_SYSINFO_CREATE_DIALOG createDialog = Parameter1;

            if (!createDialog)
                break;

            createDialog->Instance = PluginInstance->DllBase;
            createDialog->Template = MAKEINTRESOURCE(IDD_SYSINFO_NET);
            createDialog->DialogProc = EtpNetworkDialogProc;
        }
        return TRUE;
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = Parameter1;
            LONG dpiValue;

            if (!drawInfo)
                break;

            dpiValue = PhGetWindowDpi(Section->GraphHandle);
            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y | PH_GRAPH_USE_LINE_2;
            Section->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), PhGetIntegerSetting(L"ColorIoWrite"), dpiValue);
            PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, EtNetworkReceiveHistory.Count);

            if (!Section->GraphState.Valid)
            {
                ULONG i;
                FLOAT max = 1024 * 1024; // Minimum scaling of 1 MB

                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data1;
                    FLOAT data2;

                    Section->GraphState.Data1[i] = data1 =
                        (FLOAT)PhGetItemCircularBuffer_ULONG(&EtNetworkReceiveHistory, i);
                    Section->GraphState.Data2[i] = data2 =
                        (FLOAT)PhGetItemCircularBuffer_ULONG(&EtNetworkSendHistory, i);

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
            ULONG64 networkReceive;
            ULONG64 networkSend;
            PH_FORMAT format[7];

            if (!getTooltipText)
                break;

            networkReceive = PhGetItemCircularBuffer_ULONG(&EtNetworkReceiveHistory, getTooltipText->Index);
            networkSend = PhGetItemCircularBuffer_ULONG(&EtNetworkSendHistory, getTooltipText->Index);

            // R: %s\nS: %s%s\n%s
            PhInitFormatS(&format[0], L"R: ");
            PhInitFormatSize(&format[1], networkReceive);
            PhInitFormatS(&format[2], L"\nS: ");
            PhInitFormatSize(&format[3], networkSend);
            PhInitFormatSR(&format[4], PH_AUTO_T(PH_STRING, EtpGetMaxNetworkString(getTooltipText->Index))->sr);
            PhInitFormatC(&format[5], L'\n');
            PhInitFormatSR(&format[6], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

            PhMoveReference(&Section->GraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 128));
            getTooltipText->Text = PhGetStringRef(Section->GraphState.TooltipText);
        }
        return TRUE;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = Parameter1;
            PH_FORMAT format[4];

            if (!drawPanel)
                break;

            drawPanel->Title = PhCreateString(L"Network");

            // R: %s\nS: %s
            PhInitFormatS(&format[0], L"R: ");
            PhInitFormatSize(&format[1], EtNetworkReceiveDelta.Delta);
            PhInitFormatS(&format[2], L"\nS: ");
            PhInitFormatSize(&format[3], EtNetworkSendDelta.Delta);

            drawPanel->SubTitle = PhFormat(format, RTL_NUMBER_OF(format), 0);
        }
        return TRUE;
    }

    return FALSE;
}

VOID EtpCreateNetworkGraph(
    VOID
    )
{
    NetworkGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        NetworkDialog,
        NULL,
        NULL,
        NULL
        );
    Graph_SetTooltip(NetworkGraphHandle, TRUE);

    PhAddLayoutItemEx(&NetworkLayoutManager, NetworkGraphHandle, NULL, PH_ANCHOR_ALL, NetworkGraphMargin);
}

INT_PTR CALLBACK EtpNetworkDialogProc(
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
            LONG dpiValue;

            PhInitializeGraphState(&NetworkGraphState);

            dpiValue = PhGetWindowDpi(hwndDlg);

            NetworkDialog = hwndDlg;
            PhInitializeLayoutManager(&NetworkLayoutManager, hwndDlg);
            graphItem = PhAddLayoutItem(&NetworkLayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            panelItem = PhAddLayoutItem(&NetworkLayoutManager, GetDlgItem(hwndDlg, IDC_PANEL_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            NetworkGraphMargin = graphItem->Margin;
            PhGetSizeDpiValue(&NetworkGraphMargin, dpiValue, TRUE);

            SetWindowFont(GetDlgItem(hwndDlg, IDC_TITLE), NetworkSection->Parameters->LargeFont, FALSE);

            NetworkPanel = PhCreateDialog(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_SYSINFO_NETPANEL), hwndDlg, EtpNetworkPanelDialogProc, NULL);
            ShowWindow(NetworkPanel, SW_SHOW);

            margin = panelItem->Margin;
            PhGetSizeDpiValue(&margin, dpiValue, TRUE);
            PhAddLayoutItemEx(&NetworkLayoutManager, NetworkPanel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, margin);

            EtpCreateNetworkGraph();
            EtpUpdateNetworkGraph();
            EtpUpdateNetworkPanel();
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&NetworkLayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&NetworkLayoutManager);

            NetworkGraphState.Valid = FALSE;
            NetworkGraphState.TooltipIndex = ULONG_MAX;
            if (NetworkGraphHandle)
                Graph_Draw(NetworkGraphHandle);
        }
        break;
    case WM_NOTIFY:
        {
            NMHDR *header = (NMHDR *)lParam;

            if (header->hwndFrom == NetworkGraphHandle)
            {
                EtpNotifyNetworkGraph(header);
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

INT_PTR CALLBACK EtpNetworkPanelDialogProc(
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
            NetworkPanelReceiveDeltaLabel = GetDlgItem(hwndDlg, IDC_ZRECEIVESDELTA_V);
            NetworkPanelReceiveBytesDeltaLabel = GetDlgItem(hwndDlg, IDC_ZRECEIVEBYTESDELTA_V);
            NetworkPanelSendsDeltaLabel = GetDlgItem(hwndDlg, IDC_ZSENDSDELTA_V);
            NetworkPanelSendBytesDeltaLabel = GetDlgItem(hwndDlg, IDC_ZSENDBYTESDELTA_V);
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

VOID EtpNotifyNetworkGraph(
    _In_ NMHDR *Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;
            LONG dpiValue;

            dpiValue = PhGetWindowDpi(Header->hwndFrom);

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y | PH_GRAPH_USE_LINE_2;
            NetworkSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), PhGetIntegerSetting(L"ColorIoWrite"), dpiValue);

            PhGraphStateGetDrawInfo(
                &NetworkGraphState,
                getDrawInfo,
                EtNetworkReceiveHistory.Count
                );

            if (!NetworkGraphState.Valid)
            {
                ULONG i;
                FLOAT max = 1024 * 1024; // Minimum scaling of 1 MB

                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data1;
                    FLOAT data2;

                    NetworkGraphState.Data1[i] = data1 =
                        (FLOAT)PhGetItemCircularBuffer_ULONG(&EtNetworkReceiveHistory, i);
                    NetworkGraphState.Data2[i] = data2 =
                        (FLOAT)PhGetItemCircularBuffer_ULONG(&EtNetworkSendHistory, i);

                    if (max < data1 + data2)
                        max = data1 + data2;
                }

                if (max != 0)
                {
                    // Scale the data.

                    PhDivideSinglesBySingle(
                        NetworkGraphState.Data1,
                        max,
                        drawInfo->LineDataCount
                        );
                    PhDivideSinglesBySingle(
                        NetworkGraphState.Data2,
                        max,
                        drawInfo->LineDataCount
                        );
                }

                drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                drawInfo->LabelYFunctionParameter = max;

                NetworkGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (NetworkGraphState.TooltipIndex != getTooltipText->Index)
                {
                    ULONG64 networkReceive;
                    ULONG64 networkSend;
                    PH_FORMAT format[7];

                    networkReceive = PhGetItemCircularBuffer_ULONG(&EtNetworkReceiveHistory, getTooltipText->Index);
                    networkSend = PhGetItemCircularBuffer_ULONG(&EtNetworkSendHistory, getTooltipText->Index);

                    // R: %s\nS: %s%s\n%s
                    PhInitFormatS(&format[0], L"R: ");
                    PhInitFormatSize(&format[1], networkReceive);
                    PhInitFormatS(&format[2], L"\nS: ");
                    PhInitFormatSize(&format[3], networkSend);
                    PhInitFormatSR(&format[4], PH_AUTO_T(PH_STRING, EtpGetMaxNetworkString(getTooltipText->Index))->sr);
                    PhInitFormatC(&format[5], L'\n');
                    PhInitFormatSR(&format[6], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&NetworkGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 128));
                }

                getTooltipText->Text = PhGetStringRef(NetworkGraphState.TooltipText);
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
                record = EtpReferenceMaxNetworkRecord(mouseEvent->Index);
            }

            if (record)
            {
                PhShowProcessRecordDialog(NetworkDialog, record);
                PhDereferenceProcessRecord(record);
            }
        }
        break;
    }
}

VOID EtpUpdateNetworkGraph(
    VOID
    )
{
    NetworkGraphState.Valid = FALSE;
    NetworkGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(NetworkGraphHandle, 1);
    Graph_Draw(NetworkGraphHandle);
    Graph_UpdateTooltip(NetworkGraphHandle);
    InvalidateRect(NetworkGraphHandle, NULL, FALSE);
}

VOID EtpUpdateNetworkPanel(
    VOID
    )
{
    PH_FORMAT format[1];
    WCHAR formatBuffer[256];

    PhInitFormatI64UGroupDigits(&format[0], EtNetworkReceiveCountDelta.Delta);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(NetworkPanelReceiveDeltaLabel, formatBuffer);
    else
        PhSetWindowText(NetworkPanelReceiveDeltaLabel, PhaFormatUInt64(EtNetworkReceiveCountDelta.Delta, TRUE)->Buffer);

    PhInitFormatSize(&format[0], EtNetworkReceiveDelta.Delta);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(NetworkPanelReceiveBytesDeltaLabel, formatBuffer);
    else
        PhSetWindowText(NetworkPanelReceiveBytesDeltaLabel, PhaFormatSize(EtNetworkReceiveDelta.Delta, ULONG_MAX)->Buffer);

    PhInitFormatI64UGroupDigits(&format[0], EtNetworkSendCountDelta.Delta);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(NetworkPanelSendsDeltaLabel, formatBuffer);
    else
        PhSetWindowText(NetworkPanelSendsDeltaLabel, PhaFormatUInt64(EtNetworkSendCountDelta.Delta, TRUE)->Buffer);

    PhInitFormatSize(&format[0], EtNetworkSendDelta.Delta);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(NetworkPanelSendBytesDeltaLabel, formatBuffer);
    else
        PhSetWindowText(NetworkPanelSendBytesDeltaLabel, PhaFormatSize(EtNetworkSendDelta.Delta, ULONG_MAX)->Buffer);
}

PPH_PROCESS_RECORD EtpReferenceMaxNetworkRecord(
    _In_ LONG Index
    )
{
    LARGE_INTEGER time;
    ULONG maxProcessId;

    maxProcessId = PhGetItemCircularBuffer_ULONG(&EtMaxNetworkHistory, Index);

    if (!maxProcessId)
        return NULL;

    PhGetStatisticsTime(NULL, Index, &time);
    time.QuadPart += PH_TICKS_PER_SEC - 1;

    return PhFindProcessRecord(UlongToHandle(maxProcessId), &time);
}

PPH_STRING EtpGetMaxNetworkString(
    _In_ LONG Index
    )
{
    PPH_PROCESS_RECORD maxProcessRecord;
#ifdef PH_RECORD_MAX_USAGE
    ULONG64 maxNetworkUsage;
#endif

    if (maxProcessRecord = EtpReferenceMaxNetworkRecord(Index))
    {
        PPH_STRING maxUsageString;

#ifdef PH_RECORD_MAX_USAGE
        PH_FORMAT format[6];

        maxNetworkUsage = PhGetItemCircularBuffer_ULONG64(&PhMaxNetworkUsageHistory, Index);

        // \n%s (%lu): %s
        PhInitFormatC(&format[0], L'\n');
        PhInitFormatSR(&format[1], maxProcessRecord->ProcessName->sr);
        PhInitFormatS(&format[2], L" (");
        PhInitFormatU(&format[3], HandleToUlong(maxProcessRecord->ProcessId));
        PhInitFormatS(&format[4], L"): ");
        PhInitFormatSize(&format[5], maxNetworkUsage);
#else
        PH_FORMAT format[5];

        // \n%s (%lu)
        PhInitFormatC(&format[0], L'\n');
        PhInitFormatSR(&format[1], maxProcessRecord->ProcessName->sr);
        PhInitFormatS(&format[2], L" (");
        PhInitFormatU(&format[3], HandleToUlong(maxProcessRecord->ProcessId));
        PhInitFormatC(&format[4], L')');
#endif

        maxUsageString = PhFormat(format, RTL_NUMBER_OF(format), 128);

        PhDereferenceProcessRecord(maxProcessRecord);

        return maxUsageString;
    }

    return PhReferenceEmptyString();
}
