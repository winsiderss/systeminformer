/*
 * Process Hacker Extended Tools - 
 *   ETW system information dialog
 * 
 * Copyright (C) 2010 wj32
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
#include <graph.h>

#define WM_ET_ETWSYS_ACTIVATE (WM_APP + 150)
#define WM_ET_ETWSYS_UPDATE (WM_APP + 151)
#define WM_ET_ETWSYS_PANEL_UPDATE (WM_APP + 160)

NTSTATUS EtpEtwSysThreadStart(
    __in PVOID Parameter
    );

INT_PTR CALLBACK EtpEtwSysDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK EtpEtwSysPanelDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

HANDLE EtpEtwSysThreadHandle = NULL;
HWND EtpEtwSysWindowHandle = NULL;
HWND EtpEtwSysPanelWindowHandle = NULL;
static PH_EVENT InitializedEvent = PH_EVENT_INIT;
static PH_LAYOUT_MANAGER WindowLayoutManager;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

static BOOLEAN AlwaysOnTop;

static HWND DiskGraphHandle;
static HWND NetworkGraphHandle;

static PH_GRAPH_STATE DiskGraphState;
static PH_GRAPH_STATE NetworkGraphState;

static RECT EtpNormalGraphTextMargin = { 5, 5, 5, 5 };
static RECT EtpNormalGraphTextPadding = { 3, 3, 3, 3 };

VOID EtShowEtwSystemDialog(
    VOID
    )
{
    if (!EtpEtwSysWindowHandle)
    {
        if (!(EtpEtwSysThreadHandle = PhCreateThread(0, EtpEtwSysThreadStart, NULL)))
        {
            PhShowStatus(PhMainWndHandle, L"Unable to create the ETW information window", 0, GetLastError());
            return;
        }

        PhWaitForEvent(&InitializedEvent, NULL);
    }

    SendMessage(EtpEtwSysWindowHandle, WM_ET_ETWSYS_ACTIVATE, 0, 0);
}

static NTSTATUS EtpEtwSysThreadStart(
    __in PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    BOOL result;
    MSG message;

    PhInitializeAutoPool(&autoPool);

    EtpEtwSysWindowHandle = CreateDialog(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_SYSDISKNET),
        NULL,
        EtpEtwSysDlgProc
        );

    PhSetEvent(&InitializedEvent);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!IsDialogMessage(EtpEtwSysWindowHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);
    PhResetEvent(&InitializedEvent);
    NtClose(EtpEtwSysThreadHandle);

    EtpEtwSysWindowHandle = NULL;
    EtpEtwSysPanelWindowHandle = NULL;
    EtpEtwSysThreadHandle = NULL;

    return STATUS_SUCCESS;
}

static VOID EtpSetAlwaysOnTop(
    VOID
    )
{
    SetWindowPos(EtpEtwSysWindowHandle, AlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

static VOID NTAPI EtwSysUpdateHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PostMessage(EtpEtwSysWindowHandle, WM_ET_ETWSYS_UPDATE, 0, 0);
}

static PPH_PROCESS_RECORD EtpReferenceMaxDiskRecord(
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

static PPH_STRING EtpGetMaxDiskString(
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

static PPH_PROCESS_RECORD EtpReferenceMaxNetworkRecord(
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

static PPH_STRING EtpGetMaxNetworkString(
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

INT_PTR CALLBACK EtpEtwSysDlgProc(      
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
            PhSetWindowStyle(hwndDlg, WS_CLIPCHILDREN, WS_CLIPCHILDREN);

            PhInitializeLayoutManager(&WindowLayoutManager, hwndDlg);

            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_ALWAYSONTOP), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            PhLoadWindowPlacementFromSetting(SETTING_NAME_ETWSYS_WINDOW_POSITION, SETTING_NAME_ETWSYS_WINDOW_SIZE, hwndDlg);

            PhInitializeGraphState(&DiskGraphState);
            PhInitializeGraphState(&NetworkGraphState);

            DiskGraphHandle = CreateWindow(
                PH_GRAPH_CLASSNAME,
                L"",
                WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
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
            BringWindowToTop(DiskGraphHandle);

            NetworkGraphHandle = CreateWindow(
                PH_GRAPH_CLASSNAME,
                L"",
                WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
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
            BringWindowToTop(NetworkGraphHandle);

            PhRegisterCallback(
                &PhProcessesUpdatedEvent,
                EtwSysUpdateHandler,
                NULL,
                &ProcessesUpdatedRegistration
                );
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterCallback(
                &PhProcessesUpdatedEvent,
                &ProcessesUpdatedRegistration
                );

            PhDeleteGraphState(&DiskGraphState);
            PhDeleteGraphState(&NetworkGraphState);

            PhSetIntegerSetting(SETTING_NAME_ETWSYS_ALWAYS_ON_TOP, AlwaysOnTop);
            PhSaveWindowPlacementToSetting(SETTING_NAME_ETWSYS_WINDOW_POSITION, SETTING_NAME_ETWSYS_WINDOW_SIZE, hwndDlg);

            PhDeleteLayoutManager(&WindowLayoutManager);
            PostQuitMessage(0);
        }
        break;
    case WM_SHOWWINDOW:
        {
            RECT margin;

            EtpEtwSysPanelWindowHandle = CreateDialog(
                PluginInstance->DllBase,
                MAKEINTRESOURCE(IDD_SYSDISKNET_PANEL),
                EtpEtwSysWindowHandle,
                EtpEtwSysPanelDlgProc
                );

            SetWindowPos(EtpEtwSysPanelWindowHandle, NULL, 10, 0, 0, 0,
                SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER);
            ShowWindow(EtpEtwSysPanelWindowHandle, SW_SHOW);

            AlwaysOnTop = (BOOLEAN)PhGetIntegerSetting(SETTING_NAME_ETWSYS_ALWAYS_ON_TOP);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ALWAYSONTOP), AlwaysOnTop ? BST_CHECKED : BST_UNCHECKED);
            EtpSetAlwaysOnTop();

            margin.left = 0;
            margin.top = 0;
            margin.right = 0;
            margin.bottom = 25;
            MapDialogRect(hwndDlg, &margin);

            PhAddLayoutItemEx(&WindowLayoutManager, EtpEtwSysPanelWindowHandle, NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT, margin);

            SendMessage(hwndDlg, WM_SIZE, 0, 0);
            SendMessage(hwndDlg, WM_ET_ETWSYS_UPDATE, 0, 0);
        }
        break;
    case WM_SIZE:
        {                      
            HDWP deferHandle;
            HWND diskGroupBox = GetDlgItem(hwndDlg, IDC_GROUPDISK);
            HWND networkGroupBox = GetDlgItem(hwndDlg, IDC_GROUPNETWORK);
            RECT clientRect;
            RECT panelRect;
            RECT margin = { 13, 13, 13, 13 };
            RECT innerMargin = { 10, 20, 10, 10 };
            LONG between = 3;
            LONG width;
            LONG height;

            PhLayoutManagerLayout(&WindowLayoutManager);

            DiskGraphState.Valid = FALSE;
            NetworkGraphState.Valid = FALSE;

            GetClientRect(hwndDlg, &clientRect);
            // Limit the rectangle bottom to the top of the panel.
            GetWindowRect(EtpEtwSysPanelWindowHandle, &panelRect);
            MapWindowPoints(NULL, hwndDlg, (POINT *)&panelRect, 2);
            clientRect.bottom = panelRect.top;

            width = clientRect.right - margin.left - margin.right;
            height = (clientRect.bottom - margin.top - margin.bottom - between * 2) / 2;

            deferHandle = BeginDeferWindowPos(4);

            deferHandle = DeferWindowPos(deferHandle, diskGroupBox, NULL, margin.left, margin.top,
                width, height, SWP_NOACTIVATE | SWP_NOZORDER);
            deferHandle = DeferWindowPos(
                deferHandle,
                DiskGraphHandle,
                NULL,
                margin.left + innerMargin.left,
                margin.top + innerMargin.top,
                width - innerMargin.left - innerMargin.right,
                height - innerMargin.top - innerMargin.bottom,
                SWP_NOACTIVATE | SWP_NOZORDER
                );

            deferHandle = DeferWindowPos(deferHandle, networkGroupBox, NULL, margin.left, margin.top + height + between,
                width, height, SWP_NOACTIVATE | SWP_NOZORDER);
            deferHandle = DeferWindowPos(
                deferHandle,
                NetworkGraphHandle,
                NULL,
                margin.left + innerMargin.left,
                margin.top + height + between + innerMargin.top,
                width - innerMargin.left - innerMargin.right,
                height - innerMargin.top - innerMargin.bottom,
                SWP_NOACTIVATE | SWP_NOZORDER
                );

            EndDeferWindowPos(deferHandle);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, 500, 400);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(hwndDlg);
                break;
            case IDC_ALWAYSONTOP:
                {
                    AlwaysOnTop = Button_GetCheck(GetDlgItem(hwndDlg, IDC_ALWAYSONTOP)) == BST_CHECKED;
                    EtpSetAlwaysOnTop();
                }
                break;
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

                    if (header->hwndFrom == DiskGraphHandle)
                    {
                        if (PhGetIntegerSetting(L"GraphShowText"))
                        {
                            HDC hdc;

                            PhSwapReference2(&DiskGraphState.TooltipText,
                                PhFormatString(
                                L"R: %s, W: %s",
                                PhaFormatSize(EtDiskReadDelta.Delta, -1)->Buffer,
                                PhaFormatSize(EtDiskWriteDelta.Delta, -1)->Buffer
                                ));

                            hdc = Graph_GetBufferedContext(DiskGraphHandle);
                            SelectObject(hdc, PhApplicationFont);
                            PhSetGraphText(hdc, drawInfo, &DiskGraphState.TooltipText->sr,
                                &EtpNormalGraphTextMargin, &EtpNormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }

                        drawInfo->Flags = PH_GRAPH_USE_GRID | PH_GRAPH_USE_LINE_2;
                        drawInfo->LineColor1 = PhGetIntegerSetting(L"ColorIoReadOther");
                        drawInfo->LineColor2 = PhGetIntegerSetting(L"ColorIoWrite");
                        drawInfo->LineBackColor1 = PhHalveColorBrightness(drawInfo->LineColor1);
                        drawInfo->LineBackColor2 = PhHalveColorBrightness(drawInfo->LineColor2);

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

                            if (max != 0)
                            {
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
                            }

                            DiskGraphState.Valid = TRUE;
                        }
                    }
                    else if (header->hwndFrom == NetworkGraphHandle)
                    {
                        if (PhGetIntegerSetting(L"GraphShowText"))
                        {
                            HDC hdc;

                            PhSwapReference2(&NetworkGraphState.TooltipText,
                                PhFormatString(
                                L"R: %s, S: %s",
                                PhaFormatSize(EtNetworkReceiveDelta.Delta, -1)->Buffer,
                                PhaFormatSize(EtNetworkSendDelta.Delta, -1)->Buffer
                                ));

                            hdc = Graph_GetBufferedContext(NetworkGraphHandle);
                            SelectObject(hdc, PhApplicationFont);
                            PhSetGraphText(hdc, drawInfo, &NetworkGraphState.TooltipText->sr,
                                &EtpNormalGraphTextMargin, &EtpNormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }

                        drawInfo->Flags = PH_GRAPH_USE_GRID | PH_GRAPH_USE_LINE_2;
                        drawInfo->LineColor1 = PhGetIntegerSetting(L"ColorIoReadOther");
                        drawInfo->LineColor2 = PhGetIntegerSetting(L"ColorIoWrite");
                        drawInfo->LineBackColor1 = PhHalveColorBrightness(drawInfo->LineColor1);
                        drawInfo->LineBackColor2 = PhHalveColorBrightness(drawInfo->LineColor2);

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

                            if (max != 0)
                            {
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
                            }

                            NetworkGraphState.Valid = TRUE;
                        }
                    }
                }
                break;
            case GCN_GETTOOLTIPTEXT:
                {
                    PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)lParam;

                    if (
                        header->hwndFrom == DiskGraphHandle &&
                        getTooltipText->Index < getTooltipText->TotalCount
                        )
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
                    else if (
                        header->hwndFrom == NetworkGraphHandle &&
                        getTooltipText->Index < getTooltipText->TotalCount
                        )
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
                    PPH_GRAPH_MOUSEEVENT mouseEvent = (PPH_GRAPH_MOUSEEVENT)lParam;
                    PPH_PROCESS_RECORD record;

                    record = NULL;

                    if (mouseEvent->Message == WM_LBUTTONDBLCLK)
                    {
                        if (
                            header->hwndFrom == DiskGraphHandle &&
                            mouseEvent->Index < mouseEvent->TotalCount
                            )
                        {
                            record = EtpReferenceMaxDiskRecord(mouseEvent->Index);
                        }
                        else if (
                            header->hwndFrom == NetworkGraphHandle &&
                            mouseEvent->Index < mouseEvent->TotalCount
                            )
                        {
                            record = EtpReferenceMaxNetworkRecord(mouseEvent->Index);
                        }
                    }

                    if (record)
                    {
                        PhShowProcessRecordDialog(hwndDlg, record);
                        PhDereferenceProcessRecord(record);
                    }
                }
                break;
            }
        }
        break;
    case WM_ET_ETWSYS_ACTIVATE:
        {
            if (IsIconic(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
        }
        break;
    case WM_ET_ETWSYS_UPDATE:
        {
            DiskGraphState.Valid = FALSE;
            DiskGraphState.TooltipIndex = -1;
            Graph_MoveGrid(DiskGraphHandle, 1);
            Graph_Draw(DiskGraphHandle);
            Graph_UpdateTooltip(DiskGraphHandle);
            InvalidateRect(DiskGraphHandle, NULL, FALSE);

            NetworkGraphState.Valid = FALSE;
            NetworkGraphState.TooltipIndex = -1;
            Graph_MoveGrid(NetworkGraphHandle, 1);
            Graph_Draw(NetworkGraphHandle);
            Graph_UpdateTooltip(NetworkGraphHandle);
            InvalidateRect(NetworkGraphHandle, NULL, FALSE);

            SendMessage(EtpEtwSysPanelWindowHandle, WM_ET_ETWSYS_PANEL_UPDATE, 0, 0);
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK EtpEtwSysPanelDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_ET_ETWSYS_PANEL_UPDATE:
        {
            SetDlgItemText(hwndDlg, IDC_ZREADS_V, PhaFormatUInt64(EtDiskReadCount, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZREADBYTES_V, PhaFormatSize(EtDiskReadDelta.Value, -1)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZWRITES_V, PhaFormatUInt64(EtDiskWriteCount, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZWRITEBYTES_V, PhaFormatSize(EtDiskWriteDelta.Value, -1)->Buffer);

            SetDlgItemText(hwndDlg, IDC_ZRECEIVES_V, PhaFormatUInt64(EtNetworkReceiveCount, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZRECEIVEBYTES_V, PhaFormatSize(EtNetworkReceiveDelta.Value, -1)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZSENDS_V, PhaFormatUInt64(EtNetworkSendCount, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZSENDBYTES_V, PhaFormatSize(EtNetworkSendDelta.Value, -1)->Buffer);
        }
        break;
    }

    return FALSE;
}
