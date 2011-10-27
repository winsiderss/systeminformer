/*
 * Process Hacker Extended Tools -
 *   GPU system information dialog
 *
 * Copyright (C) 2011 wj32
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

#define WM_ET_GPUSYS_ACTIVATE (WM_APP + 150)
#define WM_ET_GPUSYS_UPDATE (WM_APP + 151)
#define WM_ET_GPUSYS_PANEL_UPDATE (WM_APP + 160)

NTSTATUS EtpGpuSysThreadStart(
    __in PVOID Parameter
    );

INT_PTR CALLBACK EtpGpuSysDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK EtpGpuSysPanelDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

HANDLE EtpGpuSysThreadHandle = NULL;
HWND EtpGpuSysWindowHandle = NULL;
HWND EtpGpuSysPanelWindowHandle = NULL;
static PH_EVENT InitializedEvent = PH_EVENT_INIT;
static PH_LAYOUT_MANAGER WindowLayoutManager;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

static BOOLEAN AlwaysOnTop;

static HWND GpuGraphHandle;
static HWND DedicatedGraphHandle;
static HWND SharedGraphHandle;

static PH_GRAPH_STATE GpuGraphState;
static PH_GRAPH_STATE DedicatedGraphState;
static PH_GRAPH_STATE SharedGraphState;

static RECT EtpNormalGraphTextMargin = { 5, 5, 5, 5 };
static RECT EtpNormalGraphTextPadding = { 3, 3, 3, 3 };

VOID EtShowGpuSystemDialog(
    VOID
    )
{
    if (!EtpGpuSysWindowHandle)
    {
        if (!(EtpGpuSysThreadHandle = PhCreateThread(0, EtpGpuSysThreadStart, NULL)))
        {
            PhShowStatus(PhMainWndHandle, L"Unable to create the GPU information window", 0, GetLastError());
            return;
        }

        PhWaitForEvent(&InitializedEvent, NULL);
    }

    SendMessage(EtpGpuSysWindowHandle, WM_ET_GPUSYS_ACTIVATE, 0, 0);
}

static NTSTATUS EtpGpuSysThreadStart(
    __in PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    BOOL result;
    MSG message;

    PhInitializeAutoPool(&autoPool);

    EtpGpuSysWindowHandle = CreateDialog(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_SYSGPUINFO),
        NULL,
        EtpGpuSysDlgProc
        );

    PhSetEvent(&InitializedEvent);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!IsDialogMessage(EtpGpuSysWindowHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);
    PhResetEvent(&InitializedEvent);
    NtClose(EtpGpuSysThreadHandle);

    EtpGpuSysWindowHandle = NULL;
    EtpGpuSysPanelWindowHandle = NULL;
    EtpGpuSysThreadHandle = NULL;

    return STATUS_SUCCESS;
}

static VOID EtpSetAlwaysOnTop(
    VOID
    )
{
    SetWindowPos(EtpGpuSysWindowHandle, AlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

static VOID NTAPI GpuSysUpdateHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PostMessage(EtpGpuSysWindowHandle, WM_ET_GPUSYS_UPDATE, 0, 0);
}

static PPH_PROCESS_RECORD EtpReferenceMaxNodeRecord(
    __in LONG Index
    )
{
    LARGE_INTEGER time;
    ULONG maxProcessId;

    maxProcessId = PhGetItemCircularBuffer_ULONG(&EtMaxGpuNodeHistory, Index);

    if (!maxProcessId)
        return NULL;

    PhGetStatisticsTime(NULL, Index, &time);
    time.QuadPart += PH_TICKS_PER_SEC - 1;

    return PhFindProcessRecord((HANDLE)maxProcessId, &time);
}

static PPH_STRING EtpGetMaxNodeString(
    __in LONG Index
    )
{
    PPH_PROCESS_RECORD maxProcessRecord;
    FLOAT maxGpuUsage;
    PPH_STRING maxUsageString = NULL;

    if (maxProcessRecord = EtpReferenceMaxNodeRecord(Index))
    {
        maxGpuUsage = PhGetItemCircularBuffer_FLOAT(&EtMaxGpuNodeUsageHistory, Index);

        maxUsageString = PhaFormatString(
            L"\n%s (%u): %.2f%%",
            maxProcessRecord->ProcessName->Buffer,
            (ULONG)maxProcessRecord->ProcessId,
            maxGpuUsage * 100
            );

        PhDereferenceProcessRecord(maxProcessRecord);
    }

    return maxUsageString;
}

INT_PTR CALLBACK EtpGpuSysDlgProc(
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

            PhLoadWindowPlacementFromSetting(SETTING_NAME_GPUSYS_WINDOW_POSITION, SETTING_NAME_GPUSYS_WINDOW_SIZE, hwndDlg);

            PhInitializeGraphState(&GpuGraphState);
            PhInitializeGraphState(&DedicatedGraphState);
            PhInitializeGraphState(&SharedGraphState);

            GpuGraphHandle = CreateWindow(
                PH_GRAPH_CLASSNAME,
                L"",
                WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
                0,
                0,
                3,
                3,
                hwndDlg,
                (HMENU)IDC_GPU,
                PluginInstance->DllBase,
                NULL
                );
            Graph_SetTooltip(GpuGraphHandle, TRUE);
            BringWindowToTop(GpuGraphHandle);

            DedicatedGraphHandle = CreateWindow(
                PH_GRAPH_CLASSNAME,
                L"",
                WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
                0,
                0,
                3,
                3,
                hwndDlg,
                (HMENU)IDC_DEDICATED,
                PluginInstance->DllBase,
                NULL
                );
            Graph_SetTooltip(DedicatedGraphHandle, TRUE);
            BringWindowToTop(DedicatedGraphHandle);

            SharedGraphHandle = CreateWindow(
                PH_GRAPH_CLASSNAME,
                L"",
                WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
                0,
                0,
                3,
                3,
                hwndDlg,
                (HMENU)IDC_SHARED,
                PluginInstance->DllBase,
                NULL
                );
            Graph_SetTooltip(SharedGraphHandle, TRUE);
            BringWindowToTop(SharedGraphHandle);

            PhRegisterCallback(
                &PhProcessesUpdatedEvent,
                GpuSysUpdateHandler,
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

            PhDeleteGraphState(&GpuGraphState);
            PhDeleteGraphState(&DedicatedGraphState);
            PhDeleteGraphState(&SharedGraphState);

            PhSetIntegerSetting(SETTING_NAME_GPUSYS_ALWAYS_ON_TOP, AlwaysOnTop);
            PhSaveWindowPlacementToSetting(SETTING_NAME_GPUSYS_WINDOW_POSITION, SETTING_NAME_GPUSYS_WINDOW_SIZE, hwndDlg);

            PhDeleteLayoutManager(&WindowLayoutManager);
            PostQuitMessage(0);
        }
        break;
    case WM_SHOWWINDOW:
        {
            RECT margin;

            EtpGpuSysPanelWindowHandle = CreateDialog(
                PluginInstance->DllBase,
                MAKEINTRESOURCE(IDD_SYSGPUINFO_PANEL),
                EtpGpuSysWindowHandle,
                EtpGpuSysPanelDlgProc
                );

            SetWindowPos(EtpGpuSysPanelWindowHandle, NULL, 10, 0, 0, 0,
                SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER);
            ShowWindow(EtpGpuSysPanelWindowHandle, SW_SHOW);

            AlwaysOnTop = (BOOLEAN)PhGetIntegerSetting(SETTING_NAME_GPUSYS_ALWAYS_ON_TOP);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ALWAYSONTOP), AlwaysOnTop ? BST_CHECKED : BST_UNCHECKED);
            EtpSetAlwaysOnTop();

            margin.left = 0;
            margin.top = 0;
            margin.right = 0;
            margin.bottom = 25;
            MapDialogRect(hwndDlg, &margin);

            PhAddLayoutItemEx(&WindowLayoutManager, EtpGpuSysPanelWindowHandle, NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT, margin);

            SendMessage(hwndDlg, WM_SIZE, 0, 0);
            SendMessage(hwndDlg, WM_ET_GPUSYS_UPDATE, 0, 0);
        }
        break;
    case WM_SIZE:
        {
            HDWP deferHandle;
            HWND gpuGroupBox = GetDlgItem(hwndDlg, IDC_GROUPGPU);
            HWND dedicatedGroupBox = GetDlgItem(hwndDlg, IDC_GROUPDEDICATED);
            HWND sharedGroupBox = GetDlgItem(hwndDlg, IDC_GROUPSHARED);
            RECT clientRect;
            RECT panelRect;
            RECT margin = { 13, 13, 13, 13 };
            RECT innerMargin = { 10, 20, 10, 10 };
            LONG between = 3;
            LONG width;
            LONG height;

            PhLayoutManagerLayout(&WindowLayoutManager);

            GpuGraphState.Valid = FALSE;
            DedicatedGraphState.Valid = FALSE;
            SharedGraphState.Valid = FALSE;

            GetClientRect(hwndDlg, &clientRect);
            // Limit the rectangle bottom to the top of the panel.
            GetWindowRect(EtpGpuSysPanelWindowHandle, &panelRect);
            MapWindowPoints(NULL, hwndDlg, (POINT *)&panelRect, 2);
            clientRect.bottom = panelRect.top;

            width = clientRect.right - margin.left - margin.right;
            height = (clientRect.bottom - margin.top - margin.bottom - between * 2) / 3;

            deferHandle = BeginDeferWindowPos(4);

            deferHandle = DeferWindowPos(deferHandle, gpuGroupBox, NULL, margin.left, margin.top,
                width, height, SWP_NOACTIVATE | SWP_NOZORDER);
            deferHandle = DeferWindowPos(
                deferHandle,
                GpuGraphHandle,
                NULL,
                margin.left + innerMargin.left,
                margin.top + innerMargin.top,
                width - innerMargin.left - innerMargin.right,
                height - innerMargin.top - innerMargin.bottom,
                SWP_NOACTIVATE | SWP_NOZORDER
                );

            deferHandle = DeferWindowPos(deferHandle, dedicatedGroupBox, NULL, margin.left, margin.top + height + between,
                width, height, SWP_NOACTIVATE | SWP_NOZORDER);
            deferHandle = DeferWindowPos(
                deferHandle,
                DedicatedGraphHandle,
                NULL,
                margin.left + innerMargin.left,
                margin.top + height + between + innerMargin.top,
                width - innerMargin.left - innerMargin.right,
                height - innerMargin.top - innerMargin.bottom,
                SWP_NOACTIVATE | SWP_NOZORDER
                );

            deferHandle = DeferWindowPos(deferHandle, sharedGroupBox, NULL, margin.left, margin.top + (height + between) * 2,
                width, height, SWP_NOACTIVATE | SWP_NOZORDER);
            deferHandle = DeferWindowPos(
                deferHandle,
                SharedGraphHandle,
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
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, 420, 400);
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

                    if (header->hwndFrom == GpuGraphHandle)
                    {
                        if (PhGetIntegerSetting(L"GraphShowText"))
                        {
                            HDC hdc;

                            PhSwapReference2(&GpuGraphState.TooltipText,
                                PhFormatString(
                                L"%.2f%%",
                                EtGpuNodeUsage * 100
                                ));

                            hdc = Graph_GetBufferedContext(GpuGraphHandle);
                            SelectObject(hdc, PhApplicationFont);
                            PhSetGraphText(hdc, drawInfo, &GpuGraphState.TooltipText->sr,
                                &EtpNormalGraphTextMargin, &EtpNormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }

                        drawInfo->Flags = PH_GRAPH_USE_GRID;
                        drawInfo->LineColor1 = PhGetIntegerSetting(L"ColorCpuKernel");
                        drawInfo->LineBackColor1 = PhHalveColorBrightness(drawInfo->LineColor1);

                        PhGraphStateGetDrawInfo(
                            &GpuGraphState,
                            getDrawInfo,
                            EtGpuNodeHistory.Count
                            );

                        if (!GpuGraphState.Valid)
                        {
                            PhCopyCircularBuffer_FLOAT(&EtGpuNodeHistory, GpuGraphState.Data1, drawInfo->LineDataCount);
                            GpuGraphState.Valid = TRUE;
                        }
                    }
                    else if (header->hwndFrom == DedicatedGraphHandle)
                    {
                        if (PhGetIntegerSetting(L"GraphShowText"))
                        {
                            HDC hdc;

                            PhSwapReference2(&DedicatedGraphState.TooltipText,
                                PhFormatString(
                                L"%s / %s (%.2f%%)",
                                PhaFormatSize(EtGpuDedicatedUsage, -1)->Buffer,
                                PhaFormatSize(EtGpuDedicatedLimit, -1)->Buffer,
                                (FLOAT)EtGpuDedicatedUsage * 100 / EtGpuDedicatedLimit
                                ));

                            hdc = Graph_GetBufferedContext(DedicatedGraphHandle);
                            SelectObject(hdc, PhApplicationFont);
                            PhSetGraphText(hdc, drawInfo, &DedicatedGraphState.TooltipText->sr,
                                &EtpNormalGraphTextMargin, &EtpNormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }

                        drawInfo->Flags = PH_GRAPH_USE_GRID;
                        drawInfo->LineColor1 = PhGetIntegerSetting(L"ColorPrivate");
                        drawInfo->LineBackColor1 = PhHalveColorBrightness(drawInfo->LineColor1);

                        PhGraphStateGetDrawInfo(
                            &DedicatedGraphState,
                            getDrawInfo,
                            EtGpuDedicatedHistory.Count
                            );

                        if (!DedicatedGraphState.Valid)
                        {
                            ULONG i;

                            for (i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                DedicatedGraphState.Data1[i] =
                                    (FLOAT)PhGetItemCircularBuffer_ULONG(&EtGpuDedicatedHistory, i);
                            }

                            if (EtGpuDedicatedLimit != 0)
                            {
                                // Scale the data.
                                PhxfDivideSingle2U(
                                    DedicatedGraphState.Data1,
                                    (FLOAT)EtGpuDedicatedLimit / PAGE_SIZE,
                                    drawInfo->LineDataCount
                                    );
                            }

                            DedicatedGraphState.Valid = TRUE;
                        }
                    }
                    else if (header->hwndFrom == SharedGraphHandle)
                    {
                        if (PhGetIntegerSetting(L"GraphShowText"))
                        {
                            HDC hdc;

                            PhSwapReference2(&SharedGraphState.TooltipText,
                                PhFormatString(
                                L"%s / %s (%.2f%%)",
                                PhaFormatSize(EtGpuSharedUsage, -1)->Buffer,
                                PhaFormatSize(EtGpuSharedLimit, -1)->Buffer,
                                (FLOAT)EtGpuSharedUsage * 100 / EtGpuSharedLimit
                                ));

                            hdc = Graph_GetBufferedContext(SharedGraphHandle);
                            SelectObject(hdc, PhApplicationFont);
                            PhSetGraphText(hdc, drawInfo, &SharedGraphState.TooltipText->sr,
                                &EtpNormalGraphTextMargin, &EtpNormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }

                        drawInfo->Flags = PH_GRAPH_USE_GRID;
                        drawInfo->LineColor1 = PhGetIntegerSetting(L"ColorPrivate");
                        drawInfo->LineBackColor1 = PhHalveColorBrightness(drawInfo->LineColor1);

                        PhGraphStateGetDrawInfo(
                            &SharedGraphState,
                            getDrawInfo,
                            EtGpuSharedHistory.Count
                            );

                        if (!SharedGraphState.Valid)
                        {
                            ULONG i;

                            for (i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                SharedGraphState.Data1[i] =
                                    (FLOAT)PhGetItemCircularBuffer_ULONG(&EtGpuSharedHistory, i);
                            }

                            if (EtGpuSharedLimit != 0)
                            {
                                // Scale the data.
                                PhxfDivideSingle2U(
                                    SharedGraphState.Data1,
                                    (FLOAT)EtGpuSharedLimit / PAGE_SIZE,
                                    drawInfo->LineDataCount
                                    );
                            }

                            SharedGraphState.Valid = TRUE;
                        }
                    }
                }
                break;
            case GCN_GETTOOLTIPTEXT:
                {
                    PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)lParam;

                    if (
                        header->hwndFrom == GpuGraphHandle &&
                        getTooltipText->Index < getTooltipText->TotalCount
                        )
                    {
                        if (GpuGraphState.TooltipIndex != getTooltipText->Index)
                        {
                            FLOAT gpu;

                            gpu = PhGetItemCircularBuffer_FLOAT(&EtGpuNodeHistory, getTooltipText->Index);

                            PhSwapReference2(&GpuGraphState.TooltipText, PhFormatString(
                                L"%.2f%%%s\n%s",
                                gpu * 100,
                                PhGetStringOrEmpty(EtpGetMaxNodeString(getTooltipText->Index)),
                                ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                                ));
                        }

                        getTooltipText->Text = GpuGraphState.TooltipText->sr;
                    }
                    else if (
                        header->hwndFrom == DedicatedGraphHandle &&
                        getTooltipText->Index < getTooltipText->TotalCount
                        )
                    {
                        if (DedicatedGraphState.TooltipIndex != getTooltipText->Index)
                        {
                            ULONG usedPages;

                            usedPages = PhGetItemCircularBuffer_ULONG(&EtGpuDedicatedHistory, getTooltipText->Index);

                            PhSwapReference2(&DedicatedGraphState.TooltipText, PhFormatString(
                                L"Dedicated Memory: %s\n%s",
                                PhaFormatSize(UInt32x32To64(usedPages, PAGE_SIZE), -1)->Buffer,
                                ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                                ));
                        }

                        getTooltipText->Text = DedicatedGraphState.TooltipText->sr;
                    }
                    else if (
                        header->hwndFrom == SharedGraphHandle &&
                        getTooltipText->Index < getTooltipText->TotalCount
                        )
                    {
                        if (SharedGraphState.TooltipIndex != getTooltipText->Index)
                        {
                            ULONG usedPages;

                            usedPages = PhGetItemCircularBuffer_ULONG(&EtGpuSharedHistory, getTooltipText->Index);

                            PhSwapReference2(&SharedGraphState.TooltipText, PhFormatString(
                                L"Shared Memory: %s\n%s",
                                PhaFormatSize(UInt32x32To64(usedPages, PAGE_SIZE), -1)->Buffer,
                                ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                                ));
                        }

                        getTooltipText->Text = SharedGraphState.TooltipText->sr;
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
                            header->hwndFrom == GpuGraphHandle &&
                            mouseEvent->Index < mouseEvent->TotalCount
                            )
                        {
                            record = EtpReferenceMaxNodeRecord(mouseEvent->Index);
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
    case WM_ET_GPUSYS_ACTIVATE:
        {
            if (IsIconic(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
        }
        break;
    case WM_ET_GPUSYS_UPDATE:
        {
            GpuGraphState.Valid = FALSE;
            GpuGraphState.TooltipIndex = -1;
            Graph_MoveGrid(GpuGraphHandle, 1);
            Graph_Draw(GpuGraphHandle);
            Graph_UpdateTooltip(GpuGraphHandle);
            InvalidateRect(GpuGraphHandle, NULL, FALSE);

            DedicatedGraphState.Valid = FALSE;
            DedicatedGraphState.TooltipIndex = -1;
            Graph_MoveGrid(DedicatedGraphHandle, 1);
            Graph_Draw(DedicatedGraphHandle);
            Graph_UpdateTooltip(DedicatedGraphHandle);
            InvalidateRect(DedicatedGraphHandle, NULL, FALSE);

            SharedGraphState.Valid = FALSE;
            SharedGraphState.TooltipIndex = -1;
            Graph_MoveGrid(SharedGraphHandle, 1);
            Graph_Draw(SharedGraphHandle);
            Graph_UpdateTooltip(SharedGraphHandle);
            InvalidateRect(SharedGraphHandle, NULL, FALSE);

            SendMessage(EtpGpuSysPanelWindowHandle, WM_ET_GPUSYS_PANEL_UPDATE, 0, 0);
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK EtpGpuSysPanelDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_ET_GPUSYS_PANEL_UPDATE:
        {
            SetDlgItemText(hwndDlg, IDC_ZDEDICATEDCURRENT_V, PhaFormatSize(EtGpuDedicatedUsage, -1)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZDEDICATEDLIMIT_V, PhaFormatSize(EtGpuDedicatedLimit, -1)->Buffer);

            SetDlgItemText(hwndDlg, IDC_ZSHAREDCURRENT_V, PhaFormatSize(EtGpuSharedUsage, -1)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZSHAREDLIMIT_V, PhaFormatSize(EtGpuSharedLimit, -1)->Buffer);
        }
        break;
    }

    return FALSE;
}
