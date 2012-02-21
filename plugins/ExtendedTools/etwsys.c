/*
 * Process Hacker Extended Tools -
 *   ETW system information section
 *
 * Copyright (C) 2010-2011 wj32
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
#include "etwsys.h"

static PPH_SYSINFO_SECTION DiskSection;
static HWND DiskDialog;
static PH_LAYOUT_MANAGER DiskLayoutManager;
static HWND DiskGraphHandle;
static PH_GRAPH_STATE DiskGraphState;
static HWND DiskPanel;

static PPH_SYSINFO_SECTION NetworkSection;
static HWND NetworkDialog;
static PH_LAYOUT_MANAGER NetworkLayoutManager;
static HWND NetworkGraphHandle;
static PH_GRAPH_STATE NetworkGraphState;
static HWND NetworkPanel;

VOID EtEtwSystemInformationInitializing(
    __in PPH_PLUGIN_SYSINFO_POINTERS Pointers
    )
{
    PH_SYSINFO_SECTION section;

    memset(&section, 0, sizeof(PH_SYSINFO_SECTION));
    PhInitializeStringRef(&section.Name, L"Disk");
    section.Flags = 0;
    section.Callback = EtpDiskSectionCallback;

    DiskSection = Pointers->CreateSection(&section);

    memset(&section, 0, sizeof(PH_SYSINFO_SECTION));
    PhInitializeStringRef(&section.Name, L"Network");
    section.Flags = 0;
    section.Callback = EtpNetworkSectionCallback;

    NetworkSection = Pointers->CreateSection(&section);
}

BOOLEAN EtpDiskSectionCallback(
    __in PPH_SYSINFO_SECTION Section,
    __in PH_SYSINFO_SECTION_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2
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

            createDialog->Instance = PluginInstance->DllBase;
            createDialog->Template = MAKEINTRESOURCE(IDD_SYSINFO_DISK);
            createDialog->DialogProc = EtpDiskDialogProc;
        }
        return TRUE;
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = Parameter1;

            drawInfo->Flags = PH_GRAPH_USE_GRID | PH_GRAPH_USE_LINE_2;
            Section->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), PhGetIntegerSetting(L"ColorIoWrite"));
            PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, EtDiskReadHistory.Count);

            if (!Section->GraphState.Valid)
            {
                ULONG i;
                FLOAT max = 0;

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

                // Minimum scaling of 1 MB.
                if (max < 1024 * 1024)
                    max = 1024 * 1024;

                // Scale the data.

                PhxfDivideSingle2U(
                    Section->GraphState.Data1,
                    max,
                    drawInfo->LineDataCount
                    );
                PhxfDivideSingle2U(
                    Section->GraphState.Data2,
                    max,
                    drawInfo->LineDataCount
                    );

                Section->GraphState.Valid = TRUE;
            }
        }
        return TRUE;
    case SysInfoGraphGetTooltipText:
        {
            PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT getTooltipText = Parameter1;
            ULONG64 diskRead;
            ULONG64 diskWrite;

            diskRead = PhGetItemCircularBuffer_ULONG(&EtDiskReadHistory, getTooltipText->Index);
            diskWrite = PhGetItemCircularBuffer_ULONG(&EtDiskWriteHistory, getTooltipText->Index);

            PhSwapReference2(&Section->GraphState.TooltipText, PhFormatString(
                L"R: %s\nW: %s%s\n%s",
                PhaFormatSize(diskRead, -1)->Buffer,
                PhaFormatSize(diskWrite, -1)->Buffer,
                PhGetStringOrEmpty(EtpGetMaxDiskString(getTooltipText->Index)),
                ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                ));
            getTooltipText->Text = Section->GraphState.TooltipText->sr;
        }
        return TRUE;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = Parameter1;

            drawPanel->Title = PhCreateString(L"Disk");
            drawPanel->SubTitle = PhFormatString(
                L"R: %s\nW: %s",
                PhaFormatSize(EtDiskReadDelta.Delta, -1)->Buffer,
                PhaFormatSize(EtDiskWriteDelta.Delta, -1)->Buffer
                );
        }
        return TRUE;
    }

    return FALSE;
}

INT_PTR CALLBACK EtpDiskDialogProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PPH_LAYOUT_ITEM graphItem;
            PPH_LAYOUT_ITEM panelItem;

            PhInitializeGraphState(&DiskGraphState);

            DiskDialog = hwndDlg;
            PhInitializeLayoutManager(&DiskLayoutManager, hwndDlg);
            graphItem = PhAddLayoutItem(&DiskLayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            panelItem = PhAddLayoutItem(&DiskLayoutManager, GetDlgItem(hwndDlg, IDC_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            SendMessage(GetDlgItem(hwndDlg, IDC_TITLE), WM_SETFONT, (WPARAM)DiskSection->Parameters->LargeFont, FALSE);

            DiskPanel = CreateDialog(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_SYSINFO_DISKPANEL), hwndDlg, EtpDiskPanelDialogProc);
            ShowWindow(DiskPanel, SW_SHOW);
            PhAddLayoutItemEx(&DiskLayoutManager, DiskPanel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, panelItem->Margin);

            DiskGraphHandle = CreateWindow(
                PH_GRAPH_CLASSNAME,
                NULL,
                WS_VISIBLE | WS_CHILD | WS_BORDER,
                0,
                0,
                3,
                3,
                hwndDlg,
                (HMENU)IDC_DISK,
                PluginInstance->DllBase,
                NULL
                );
            Graph_SetTooltip(DiskGraphHandle, TRUE);

            PhAddLayoutItemEx(&DiskLayoutManager, DiskGraphHandle, NULL, PH_ANCHOR_ALL, graphItem->Margin);

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
    }

    return FALSE;
}

INT_PTR CALLBACK EtpDiskPanelDialogProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    return FALSE;
}

VOID EtpNotifyDiskGraph(
    __in NMHDR *Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            drawInfo->Flags = PH_GRAPH_USE_GRID | PH_GRAPH_USE_LINE_2;
            DiskSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), PhGetIntegerSetting(L"ColorIoWrite"));

            PhGraphStateGetDrawInfo(
                &DiskGraphState,
                getDrawInfo,
                EtDiskReadHistory.Count
                );

            if (!DiskGraphState.Valid)
            {
                ULONG i;
                FLOAT max = 0;

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

                // Minimum scaling of 1 MB.
                if (max < 1024 * 1024)
                    max = 1024 * 1024;

                // Scale the data.

                PhxfDivideSingle2U(
                    DiskGraphState.Data1,
                    max,
                    drawInfo->LineDataCount
                    );
                PhxfDivideSingle2U(
                    DiskGraphState.Data2,
                    max,
                    drawInfo->LineDataCount
                    );

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

                    diskRead = PhGetItemCircularBuffer_ULONG(&EtDiskReadHistory, getTooltipText->Index);
                    diskWrite = PhGetItemCircularBuffer_ULONG(&EtDiskWriteHistory, getTooltipText->Index);

                    PhSwapReference2(&DiskGraphState.TooltipText, PhFormatString(
                        L"R: %s\nW: %s%s\n%s",
                        PhaFormatSize(diskRead, -1)->Buffer,
                        PhaFormatSize(diskWrite, -1)->Buffer,
                        PhGetStringOrEmpty(EtpGetMaxDiskString(getTooltipText->Index)),
                        ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                        ));
                }

                getTooltipText->Text = DiskGraphState.TooltipText->sr;
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
    DiskGraphState.TooltipIndex = -1;
    Graph_MoveGrid(DiskGraphHandle, 1);
    Graph_Draw(DiskGraphHandle);
    Graph_UpdateTooltip(DiskGraphHandle);
    InvalidateRect(DiskGraphHandle, NULL, FALSE);
}

VOID EtpUpdateDiskPanel(
    VOID
    )
{
    SetDlgItemText(DiskPanel, IDC_ZREADSDELTA_V, PhaFormatUInt64(EtDiskReadCountDelta.Delta, TRUE)->Buffer);
    SetDlgItemText(DiskPanel, IDC_ZREADBYTESDELTA_V, PhaFormatSize(EtDiskReadDelta.Delta, -1)->Buffer);
    SetDlgItemText(DiskPanel, IDC_ZWRITESDELTA_V, PhaFormatUInt64(EtDiskWriteCountDelta.Delta, TRUE)->Buffer);
    SetDlgItemText(DiskPanel, IDC_ZWRITEBYTESDELTA_V, PhaFormatSize(EtDiskWriteDelta.Delta, -1)->Buffer);
}

PPH_PROCESS_RECORD EtpReferenceMaxDiskRecord(
    __in LONG Index
    )
{
    LARGE_INTEGER time;
    ULONG maxProcessId;

    maxProcessId = PhGetItemCircularBuffer_ULONG(&EtMaxDiskHistory, Index);

    if (!maxProcessId)
        return NULL;

    PhGetStatisticsTime(NULL, Index, &time);
    time.QuadPart += PH_TICKS_PER_SEC - 1;

    return PhFindProcessRecord((HANDLE)maxProcessId, &time);
}

PPH_STRING EtpGetMaxDiskString(
    __in LONG Index
    )
{
    PPH_PROCESS_RECORD maxProcessRecord;
    PPH_STRING maxUsageString = NULL;

    if (maxProcessRecord = EtpReferenceMaxDiskRecord(Index))
    {
        maxUsageString = PhaFormatString(
            L"\n%s (%u)",
            maxProcessRecord->ProcessName->Buffer,
            (ULONG)maxProcessRecord->ProcessId
            );
        PhDereferenceProcessRecord(maxProcessRecord);
    }

    return maxUsageString;
}

BOOLEAN EtpNetworkSectionCallback(
    __in PPH_SYSINFO_SECTION Section,
    __in PH_SYSINFO_SECTION_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2
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

            createDialog->Instance = PluginInstance->DllBase;
            createDialog->Template = MAKEINTRESOURCE(IDD_SYSINFO_NET);
            createDialog->DialogProc = EtpNetworkDialogProc;
        }
        return TRUE;
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = Parameter1;

            drawInfo->Flags = PH_GRAPH_USE_GRID | PH_GRAPH_USE_LINE_2;
            Section->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), PhGetIntegerSetting(L"ColorIoWrite"));
            PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, EtNetworkReceiveHistory.Count);

            if (!Section->GraphState.Valid)
            {
                ULONG i;
                FLOAT max = 0;

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

                // Minimum scaling of 1 MB.
                if (max < 1024 * 1024)
                    max = 1024 * 1024;

                // Scale the data.

                PhxfDivideSingle2U(
                    Section->GraphState.Data1,
                    max,
                    drawInfo->LineDataCount
                    );
                PhxfDivideSingle2U(
                    Section->GraphState.Data2,
                    max,
                    drawInfo->LineDataCount
                    );

                Section->GraphState.Valid = TRUE;
            }
        }
        return TRUE;
    case SysInfoGraphGetTooltipText:
        {
            PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT getTooltipText = Parameter1;
            ULONG64 networkReceive;
            ULONG64 networkSend;

            networkReceive = PhGetItemCircularBuffer_ULONG(&EtNetworkReceiveHistory, getTooltipText->Index);
            networkSend = PhGetItemCircularBuffer_ULONG(&EtNetworkSendHistory, getTooltipText->Index);

            PhSwapReference2(&Section->GraphState.TooltipText, PhFormatString(
                L"R: %s\nS: %s%s\n%s",
                PhaFormatSize(networkReceive, -1)->Buffer,
                PhaFormatSize(networkSend, -1)->Buffer,
                PhGetStringOrEmpty(EtpGetMaxNetworkString(getTooltipText->Index)),
                ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                ));
            getTooltipText->Text = Section->GraphState.TooltipText->sr;
        }
        return TRUE;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = Parameter1;

            drawPanel->Title = PhCreateString(L"Network");
            drawPanel->SubTitle = PhFormatString(
                L"R: %s\nS: %s",
                PhaFormatSize(EtNetworkReceiveDelta.Delta, -1)->Buffer,
                PhaFormatSize(EtNetworkSendDelta.Delta, -1)->Buffer
                );
        }
        return TRUE;
    }

    return FALSE;
}

INT_PTR CALLBACK EtpNetworkDialogProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PPH_LAYOUT_ITEM graphItem;
            PPH_LAYOUT_ITEM panelItem;

            PhInitializeGraphState(&NetworkGraphState);

            NetworkDialog = hwndDlg;
            PhInitializeLayoutManager(&NetworkLayoutManager, hwndDlg);
            graphItem = PhAddLayoutItem(&NetworkLayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            panelItem = PhAddLayoutItem(&NetworkLayoutManager, GetDlgItem(hwndDlg, IDC_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            SendMessage(GetDlgItem(hwndDlg, IDC_TITLE), WM_SETFONT, (WPARAM)NetworkSection->Parameters->LargeFont, FALSE);

            NetworkPanel = CreateDialog(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_SYSINFO_NETPANEL), hwndDlg, EtpNetworkPanelDialogProc);
            ShowWindow(NetworkPanel, SW_SHOW);
            PhAddLayoutItemEx(&NetworkLayoutManager, NetworkPanel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, panelItem->Margin);

            NetworkGraphHandle = CreateWindow(
                PH_GRAPH_CLASSNAME,
                NULL,
                WS_VISIBLE | WS_CHILD | WS_BORDER,
                0,
                0,
                3,
                3,
                hwndDlg,
                (HMENU)IDC_NETWORK,
                PluginInstance->DllBase,
                NULL
                );
            Graph_SetTooltip(NetworkGraphHandle, TRUE);

            PhAddLayoutItemEx(&NetworkLayoutManager, NetworkGraphHandle, NULL, PH_ANCHOR_ALL, graphItem->Margin);

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
    }

    return FALSE;
}

INT_PTR CALLBACK EtpNetworkPanelDialogProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    return FALSE;
}

VOID EtpNotifyNetworkGraph(
    __in NMHDR *Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            drawInfo->Flags = PH_GRAPH_USE_GRID | PH_GRAPH_USE_LINE_2;
            NetworkSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), PhGetIntegerSetting(L"ColorIoWrite"));

            PhGraphStateGetDrawInfo(
                &NetworkGraphState,
                getDrawInfo,
                EtNetworkReceiveHistory.Count
                );

            if (!NetworkGraphState.Valid)
            {
                ULONG i;
                FLOAT max = 0;

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

                // Minimum scaling of 1 MB.
                if (max < 1024 * 1024)
                    max = 1024 * 1024;

                // Scale the data.

                PhxfDivideSingle2U(
                    NetworkGraphState.Data1,
                    max,
                    drawInfo->LineDataCount
                    );
                PhxfDivideSingle2U(
                    NetworkGraphState.Data2,
                    max,
                    drawInfo->LineDataCount
                    );

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

                    networkReceive = PhGetItemCircularBuffer_ULONG(&EtNetworkReceiveHistory, getTooltipText->Index);
                    networkSend = PhGetItemCircularBuffer_ULONG(&EtNetworkSendHistory, getTooltipText->Index);

                    PhSwapReference2(&NetworkGraphState.TooltipText, PhFormatString(
                        L"R: %s\nS: %s%s\n%s",
                        PhaFormatSize(networkReceive, -1)->Buffer,
                        PhaFormatSize(networkSend, -1)->Buffer,
                        PhGetStringOrEmpty(EtpGetMaxNetworkString(getTooltipText->Index)),
                        ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                        ));
                }

                getTooltipText->Text = NetworkGraphState.TooltipText->sr;
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
    NetworkGraphState.TooltipIndex = -1;
    Graph_MoveGrid(NetworkGraphHandle, 1);
    Graph_Draw(NetworkGraphHandle);
    Graph_UpdateTooltip(NetworkGraphHandle);
    InvalidateRect(NetworkGraphHandle, NULL, FALSE);
}

VOID EtpUpdateNetworkPanel(
    VOID
    )
{
    SetDlgItemText(NetworkPanel, IDC_ZRECEIVESDELTA_V, PhaFormatUInt64(EtNetworkReceiveCountDelta.Delta, TRUE)->Buffer);
    SetDlgItemText(NetworkPanel, IDC_ZRECEIVEBYTESDELTA_V, PhaFormatSize(EtNetworkReceiveDelta.Delta, -1)->Buffer);
    SetDlgItemText(NetworkPanel, IDC_ZSENDSDELTA_V, PhaFormatUInt64(EtNetworkSendCountDelta.Delta, TRUE)->Buffer);
    SetDlgItemText(NetworkPanel, IDC_ZSENDBYTESDELTA_V, PhaFormatSize(EtNetworkSendDelta.Delta, -1)->Buffer);
}

PPH_PROCESS_RECORD EtpReferenceMaxNetworkRecord(
    __in LONG Index
    )
{
    LARGE_INTEGER time;
    ULONG maxProcessId;

    maxProcessId = PhGetItemCircularBuffer_ULONG(&EtMaxNetworkHistory, Index);

    if (!maxProcessId)
        return NULL;

    PhGetStatisticsTime(NULL, Index, &time);
    time.QuadPart += PH_TICKS_PER_SEC - 1;

    return PhFindProcessRecord((HANDLE)maxProcessId, &time);
}

PPH_STRING EtpGetMaxNetworkString(
    __in LONG Index
    )
{
    PPH_PROCESS_RECORD maxProcessRecord;
    PPH_STRING maxUsageString = NULL;

    if (maxProcessRecord = EtpReferenceMaxNetworkRecord(Index))
    {
        maxUsageString = PhaFormatString(
            L"\n%s (%u)",
            maxProcessRecord->ProcessName->Buffer,
            (ULONG)maxProcessRecord->ProcessId
            );
        PhDereferenceProcessRecord(maxProcessRecord);
    }

    return maxUsageString;
}
