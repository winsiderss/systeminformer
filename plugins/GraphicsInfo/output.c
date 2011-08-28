/*
* Process Hacker Update Checker -
*   main window
*
* Copyright (C) 2011 wj32
* Copyright (C) 2011 dmex
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

#include "gfxinfo.h"

static NTSTATUS WindowThreadStart(
    __in PVOID Parameter
    );
static VOID GfxSetAlwaysOnTop(
    VOID
    );

static BOOLEAN AlwaysOnTop;
static PH_EVENT InitializedEvent = PH_EVENT_INIT;

static HWND GpuGraphHandle;
static HWND CoreGraphHandle;
static HWND MemGraphHandle;

HANDLE GfxThreadHandle = NULL;
HWND GfxWindowHandle = NULL;
HWND GfxPanelWindowHandle = NULL;

static PH_GRAPH_STATE GpuGraphState;
static PH_GRAPH_STATE MemGraphState;
static PH_GRAPH_STATE CoreGraphState;

static VOID NTAPI GfxUpdateHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

PH_CIRCULAR_BUFFER_FLOAT GpuHistory;
PH_CIRCULAR_BUFFER_ULONG MemHistory;
PH_CIRCULAR_BUFFER_FLOAT CoreHistory;

FLOAT CurrentGpuUsage;
FLOAT CurrentMemUsage;
FLOAT CurrentCoreUsage;
ULONG MaxMemUsage;

ULONG GfxCoreTempCount;
ULONG GfxBoardTempCount;

FLOAT GfxCoreClockCount;
FLOAT GfxMemoryClockCount;
FLOAT GfxShaderClockCount;

static RECT NormalGraphTextMargin = { 5, 5, 5, 5 };
static RECT NormalGraphTextPadding = { 3, 3, 3, 3 };

INT_PTR CALLBACK MainWndProc(
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
            // Add the Graphics card name to the Window Title.
            PPH_STRING gpuname = GetDriverName();
            PPH_STRING title = PhFormatString(L"Graphics Information (%s)", gpuname->Buffer);

            SetWindowText(hwndDlg, title->Buffer);  

            PhDereferenceObject(gpuname);
            PhDereferenceObject(title);

            // We have already set the group boxes to have WS_EX_TRANSPARENT to fix
            // the drawing issue that arises when using WS_CLIPCHILDREN. However
            // in removing the flicker from the graphs the group boxes will now flicker.
            // It's a good tradeoff since no one stares at the group boxes.
            PhSetWindowStyle(hwndDlg, WS_CLIPCHILDREN, WS_CLIPCHILDREN);
         
            PhCenterWindow(hwndDlg, PhMainWndHandle);
            
            PhInitializeLayoutManager(&WindowLayoutManager, hwndDlg);
  
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_ALWAYSONTOP), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            PhLoadWindowPlacementFromSetting(SETTING_NAME_GFX_WINDOW_POSITION, SETTING_NAME_GFX_WINDOW_SIZE, hwndDlg);

            PhInitializeGraphState(&GpuGraphState);
            PhInitializeGraphState(&CoreGraphState);
            PhInitializeGraphState(&MemGraphState);

            // TEMP
            if (GpuHistory.Count == 0)
            {
                PhInitializeCircularBuffer_FLOAT(&GpuHistory, PhGetIntegerSetting(L"SampleCount"));
                PhInitializeCircularBuffer_FLOAT(&CoreHistory, PhGetIntegerSetting(L"SampleCount"));
                PhInitializeCircularBuffer_ULONG(&MemHistory, PhGetIntegerSetting(L"SampleCount"));
            }

            GpuGraphHandle = CreateWindow(
                PH_GRAPH_CLASSNAME,
                NULL,
                WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
                0,
                0,
                3,
                3,
                hwndDlg,
                (HMENU)IDC_GPUGRAPH,
                PluginInstance->DllBase,
                NULL
                );
            Graph_SetTooltip(GpuGraphHandle, TRUE);
            BringWindowToTop(GpuGraphHandle);
     
            CoreGraphHandle = CreateWindow(
                PH_GRAPH_CLASSNAME,
                NULL,
                WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
                0,
                0,
                3,
                3,
                hwndDlg,
                (HMENU)IDC_CONTGRAPH,
                PluginInstance->DllBase,
                NULL
                );
            Graph_SetTooltip(CoreGraphHandle, TRUE);
            BringWindowToTop(CoreGraphHandle);

            MemGraphHandle = CreateWindow(
                PH_GRAPH_CLASSNAME,
                NULL,
                WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
                0,
                0,
                3,
                3,
                hwndDlg,
                (HMENU)IDC_MEMGRAPH,
                PluginInstance->DllBase,
                NULL
                );
            Graph_SetTooltip(MemGraphHandle, TRUE);
            BringWindowToTop(MemGraphHandle);
      
            PhRegisterCallback(
				PhGetGeneralCallback(GeneralCallbackProcessesUpdated),
				GfxUpdateHandler,
				NULL,
				&ProcessesUpdatedRegistration
				);
        }
        break;
    case WM_DESTROY:
        {     
            // Unregister our callbacks.
            PhUnregisterCallback(&PhProcessesUpdatedEvent, &ProcessesUpdatedRegistration);

            // Save our settings.
            PhSetIntegerSetting(SETTING_NAME_GFX_ALWAYS_ON_TOP, AlwaysOnTop);
            PhSaveWindowPlacementToSetting(SETTING_NAME_GFX_WINDOW_POSITION, SETTING_NAME_GFX_WINDOW_SIZE, hwndDlg);

            // Reset our Window Management.
            PhDeleteLayoutManager(&WindowLayoutManager);

            // TEMP commented out.
            // Clear our buffers.
            //PhDeleteCircularBuffer_FLOAT(&GpuHistory);
            //PhDeleteCircularBuffer_ULONG(&MemHistory);

            // Clear our state.
            PhDeleteGraphState(&GpuGraphState);
            PhDeleteGraphState(&MemGraphState);

            // Quit.
            PostQuitMessage(0);
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

                            PhSwapReference2(
                                &GpuGraphState.TooltipText,
                                PhFormatString(
                                L"%.0f%%",
                                CurrentGpuUsage * 100
                                ));

                            hdc = Graph_GetBufferedContext(GpuGraphHandle);
                            SelectObject(hdc, PhApplicationFont);
                            PhSetGraphText(hdc, drawInfo, &GpuGraphState.TooltipText->sr,
                                &NormalGraphTextMargin, &NormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }

                        drawInfo->Flags = PH_GRAPH_USE_GRID;
                        drawInfo->LineColor1 = PhGetIntegerSetting(L"ColorCpuKernel");
                        //drawInfo->LineColor2 = PhGetIntegerSetting(L"ColorCpuUser");
                        drawInfo->LineBackColor1 = PhHalveColorBrightness(drawInfo->LineColor1);
                        //drawInfo->LineBackColor2 = PhHalveColorBrightness(drawInfo->LineColor2);

                        PhGraphStateGetDrawInfo(
                            &GpuGraphState,
                            getDrawInfo,
                            GpuHistory.Count
                            );

                        if (!GpuGraphState.Valid)
                        {
                            PhCopyCircularBuffer_FLOAT(
                                &GpuHistory, 
                                getDrawInfo->DrawInfo->LineData1, 
                                getDrawInfo->DrawInfo->LineDataCount
                                );

                            GpuGraphState.Valid = TRUE;
                        }
                    }
                    else if (header->hwndFrom == MemGraphHandle)
                    {
                        if (PhGetIntegerSetting(L"GraphShowText"))
                        {
                            HDC hdc;

                            PhSwapReference2(&MemGraphState.TooltipText,
                                PhFormatString(
                                L"%s / %s (%.2f%%)",
                                PhaFormatSize(UInt32x32To64(CurrentMemUsage, 1024), -1)->Buffer,
                                PhaFormatSize(UInt32x32To64(MaxMemUsage, 1024), -1)->Buffer,
                                (FLOAT)CurrentMemUsage / MaxMemUsage * 100
                                ));

                            hdc = Graph_GetBufferedContext(MemGraphHandle);
                            SelectObject(hdc, PhApplicationFont);
                            PhSetGraphText(
                                hdc, 
                                drawInfo, 
                                &MemGraphState.TooltipText->sr,  
                                &NormalGraphTextMargin, 
                                &NormalGraphTextPadding, 
                                PH_ALIGN_TOP | PH_ALIGN_LEFT
                                );
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }

                        drawInfo->Flags = PH_GRAPH_USE_GRID;
                        drawInfo->LineColor1 = PhGetIntegerSetting(L"ColorCpuKernel");
                        //drawInfo->LineColor2 = PhGetIntegerSetting(L"ColorCpuUser");
                        drawInfo->LineBackColor1 = PhHalveColorBrightness(drawInfo->LineColor1);
                        //drawInfo->LineBackColor2 = PhHalveColorBrightness(drawInfo->LineColor2);

                        PhGraphStateGetDrawInfo(
                            &MemGraphState,
                            getDrawInfo,
                            MemHistory.Count
                            );

                        if (!MemGraphState.Valid)
                        {
                            ULONG i = 0;

                            for (i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                MemGraphState.Data1[i] =
                                    (FLOAT)PhGetItemCircularBuffer_ULONG(&MemHistory, i);
                            }

                            // Scale the data.
                            PhxfDivideSingle2U(
                                MemGraphState.Data1,
                                (FLOAT)MaxMemUsage,
                                drawInfo->LineDataCount
                                );

                            MemGraphState.Valid = TRUE;
                        }
                    }
                    else if (header->hwndFrom == CoreGraphHandle)
                    {
                        if (PhGetIntegerSetting(L"GraphShowText"))
                        {
                            HDC hdc;

                            PhSwapReference2(
                                &CoreGraphState.TooltipText,
                                PhFormatString(
                                L"%.0f%%",
                                CurrentCoreUsage * 100
                                ));

                            hdc = Graph_GetBufferedContext(CoreGraphHandle);
                            SelectObject(hdc, PhApplicationFont);
                            PhSetGraphText(hdc, drawInfo, &CoreGraphState.TooltipText->sr,
                                &NormalGraphTextMargin, &NormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }

                        drawInfo->Flags = PH_GRAPH_USE_GRID;
                        drawInfo->LineColor1 = PhGetIntegerSetting(L"ColorCpuKernel");
                        //drawInfo->LineColor2 = PhGetIntegerSetting(L"ColorCpuUser");
                        drawInfo->LineBackColor1 = PhHalveColorBrightness(drawInfo->LineColor1);
                        //drawInfo->LineBackColor2 = PhHalveColorBrightness(drawInfo->LineColor2);

                        PhGraphStateGetDrawInfo(
                            &CoreGraphState,
                            getDrawInfo,
                            CoreHistory.Count
                            );

                        if (!CoreGraphState.Valid)
                        {
                            PhCopyCircularBuffer_FLOAT(
                                &CoreHistory, 
                                getDrawInfo->DrawInfo->LineData1, 
                                getDrawInfo->DrawInfo->LineDataCount
                                );

                            CoreGraphState.Valid = TRUE;
                        }
                    }
                }
                break;
            case GCN_GETTOOLTIPTEXT:
                {
                    PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)lParam;

                    if (getTooltipText->Index < getTooltipText->TotalCount)
                    {
                        if (header->hwndFrom == GpuGraphHandle)
                        {
                            if (GpuGraphState.TooltipIndex != getTooltipText->Index)
                            {
                                FLOAT usage;

                                usage = PhGetItemCircularBuffer_FLOAT(&GpuHistory, getTooltipText->Index);

                                PhSwapReference2(&GpuGraphState.TooltipText, PhFormatString(
                                    L"%.0f%%",
                                    usage * 100
                                    ));
                            }

                            getTooltipText->Text = GpuGraphState.TooltipText->sr;
                        }
                        else if (header->hwndFrom == MemGraphHandle)
                        {
                            if (MemGraphState.TooltipIndex != getTooltipText->Index)
                            {
                                ULONG usage;

                                usage = PhGetItemCircularBuffer_ULONG(&MemHistory, getTooltipText->Index);

                                PhSwapReference2(&MemGraphState.TooltipText,
                                    PhFormatString(
                                    L"%s / %s (%.2f%%)",
                                    PhaFormatSize(UInt32x32To64(usage, 1024), -1)->Buffer,
                                    PhaFormatSize(UInt32x32To64(MaxMemUsage, 1024), -1)->Buffer,
                                    (FLOAT)usage / MaxMemUsage * 100
                                    ));
                            }

                            getTooltipText->Text = MemGraphState.TooltipText->sr;
                        }
                        else if (header->hwndFrom == CoreGraphHandle)
                        {
                            if (CoreGraphState.TooltipIndex != getTooltipText->Index)
                            {
                                FLOAT usage;

                                usage = PhGetItemCircularBuffer_FLOAT(&CoreHistory, getTooltipText->Index);

                                PhSwapReference2(&CoreGraphState.TooltipText, 
                                    PhFormatString(
                                    L"%.0f%%",
                                    usage * 100
                                    ));
                            }

                            getTooltipText->Text = CoreGraphState.TooltipText->sr;
                        }
                    }
                }
                break;
            case GCN_MOUSEEVENT:
                {
                    PPH_GRAPH_MOUSEEVENT mouseEvent = (PPH_GRAPH_MOUSEEVENT)lParam;

                    if (mouseEvent->Message == WM_LBUTTONDBLCLK)
                    {
                        if (header->hwndFrom == GpuGraphHandle)
                        {
                            PhShowInformation(hwndDlg, L"Double clicked!");
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_SHOWWINDOW:
        {
            RECT margin;

            GfxPanelWindowHandle = CreateDialog(
                PluginInstance->DllBase,
                MAKEINTRESOURCE(IDD_SYSGFX_PANEL),
                hwndDlg,
                EtpEtwSysPanelDlgProc
                );

            SetWindowPos(
                GfxPanelWindowHandle, 
                NULL, 
                10, 0, 0, 0,
                SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER
                );

            ShowWindow(GfxPanelWindowHandle, SW_SHOW);

            AlwaysOnTop = (BOOLEAN)PhGetIntegerSetting(SETTING_NAME_GFX_ALWAYS_ON_TOP);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ALWAYSONTOP), AlwaysOnTop ? BST_CHECKED : BST_UNCHECKED);
            GfxSetAlwaysOnTop();

            margin.left = 0;
            margin.top = 0;
            margin.right = 0;
            margin.bottom = 25;
            MapDialogRect(hwndDlg, &margin);

            PhAddLayoutItemEx(
                &WindowLayoutManager, 
                GfxPanelWindowHandle, 
                NULL, 
                PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT, 
                margin
                );

            SendMessage(hwndDlg, WM_SIZE, 0, 0);
            SendMessage(hwndDlg, WM_GFX_UPDATE, 0, 0);
        }
        break;
    case WM_SIZE:
        {                      
            HDWP deferHandle;
            HWND cpuGroupBox = GetDlgItem(hwndDlg, IDC_GROUPCONTROLLER);
            HWND diskGroupBox = GetDlgItem(hwndDlg, IDC_GROUPGPU);
            HWND networkGroupBox = GetDlgItem(hwndDlg, IDC_GROUPMEM);
            RECT clientRect;
            RECT panelRect;
            RECT margin = { 13, 13, 13, 13 };
            RECT innerMargin = { 10, 20, 10, 10 };
            LONG between = 3;
            LONG width;
            LONG height;

            PhLayoutManagerLayout(&WindowLayoutManager);

            GpuGraphState.Valid = FALSE;
            MemGraphState.Valid = FALSE;

            GetClientRect(hwndDlg, &clientRect);
            // Limit the rectangle bottom to the top of the panel.
            GetWindowRect(GfxPanelWindowHandle, &panelRect);
            MapWindowPoints(NULL, hwndDlg, (POINT *)&panelRect, 2);
            clientRect.bottom = panelRect.top;

            width = clientRect.right - margin.left - margin.right;
            height = (clientRect.bottom - margin.top - margin.bottom - between * 2) / 3;

            deferHandle = BeginDeferWindowPos(6);

            deferHandle = DeferWindowPos(deferHandle, diskGroupBox, NULL, margin.left, margin.top,  width, height, SWP_NOACTIVATE | SWP_NOZORDER);
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

            deferHandle = DeferWindowPos(deferHandle, networkGroupBox, NULL, margin.left, margin.top + height + between, width, height, SWP_NOACTIVATE | SWP_NOZORDER);
            deferHandle = DeferWindowPos(
                deferHandle,
                MemGraphHandle,
                NULL,
                margin.left + innerMargin.left,
                margin.top + height + between + innerMargin.top,
                width - innerMargin.left - innerMargin.right,
                height - innerMargin.top - innerMargin.bottom,
                SWP_NOACTIVATE | SWP_NOZORDER
                );

            deferHandle = DeferWindowPos(deferHandle, cpuGroupBox, NULL, margin.left, margin.top + (height + between) * 2, width, height, SWP_NOACTIVATE | SWP_NOZORDER);
            deferHandle = DeferWindowPos(
                deferHandle,
                CoreGraphHandle,
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
                    GfxSetAlwaysOnTop();
                }
                break;
            }
        }
        break;
    case WM_GFX_ACTIVATE:
        {
            if (IsIconic(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
        }
        break;
    case WM_GFX_UPDATE:
        {
            GetGfxUsages();
            GetGfxTemp();
            GetGfxClockSpeeds();

            GpuGraphState.Valid = FALSE;
            GpuGraphState.TooltipIndex = -1;
            Graph_MoveGrid(GpuGraphHandle, 1);
            Graph_Draw(GpuGraphHandle);
            Graph_UpdateTooltip(GpuGraphHandle);
            InvalidateRect(GpuGraphHandle, NULL, FALSE);

            CoreGraphState.Valid = FALSE;
            CoreGraphState.TooltipIndex = -1;
            Graph_MoveGrid(CoreGraphHandle, 1);
            Graph_Draw(CoreGraphHandle);
            Graph_UpdateTooltip(CoreGraphHandle);
            InvalidateRect(CoreGraphHandle, NULL, FALSE);

            MemGraphState.Valid = FALSE;
            MemGraphState.TooltipIndex = -1;
            Graph_MoveGrid(MemGraphHandle, 1);
            Graph_Draw(MemGraphHandle);
            Graph_UpdateTooltip(MemGraphHandle);
            InvalidateRect(MemGraphHandle, NULL, FALSE);

            SendMessage(GfxPanelWindowHandle, WM_GFX_PANEL_UPDATE, 0, 0);
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
    case WM_GFX_PANEL_UPDATE:
        {
            SetDlgItemText(hwndDlg, IDC_ZREADS_V, PhFormatString(L"%s\u00b0", PhaFormatUInt64(GfxCoreTempCount, TRUE)->Buffer)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZREADBYTES_V, PhFormatString(L"%s\u00b0", PhaFormatUInt64(GfxBoardTempCount, TRUE)->Buffer)->Buffer);
            //SetDlgItemText(hwndDlg, IDC_ZWRITES_V, PhaFormatUInt64(EtDiskWriteCount, TRUE)->Buffer);
            //SetDlgItemText(hwndDlg, IDC_ZWRITEBYTES_V, PhaFormatSize(EtDiskWriteDelta.Value, -1)->Buffer);
            
            SetDlgItemText(hwndDlg, IDC_ZRECEIVES_V, PhFormatString(L"%.2f MHz", GfxCoreClockCount)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZRECEIVEBYTES_V, PhFormatString(L"%.2f MHz", GfxMemoryClockCount)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZSENDS_V, PhFormatString(L"%.2f MHz", GfxShaderClockCount)->Buffer);
            //SetDlgItemText(hwndDlg, IDC_ZSENDS_V, PhaFormatUInt64(EtNetworkSendCount, TRUE)->Buffer);
            //SetDlgItemText(hwndDlg, IDC_ZSENDBYTES_V, PhaFormatSize(EtNetworkSendDelta.Value, -1)->Buffer);
        }
        break;
    }

    return FALSE;
}

static VOID NTAPI GfxUpdateHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PostMessage(GfxWindowHandle, WM_GFX_UPDATE, 0, 0);
}


static VOID GfxSetAlwaysOnTop(
    VOID
    )
{
    SetWindowPos(GfxWindowHandle, AlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

VOID ShowDialog(VOID)
{
    if (!GfxWindowHandle)
    {
        if (!(GfxThreadHandle = PhCreateThread(0, WindowThreadStart, NULL)))
        {
            PhShowStatus(PhMainWndHandle, L"Unable to create the Graphics information window.", 0, GetLastError());
            return;
        }

        PhWaitForEvent(&InitializedEvent, NULL);
    }

    SendMessage(GfxWindowHandle, WM_GFX_ACTIVATE, 0, 0);
}

static NTSTATUS WindowThreadStart(
    __in PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    BOOL result;
    MSG message;

    PhInitializeAutoPool(&autoPool);

    GfxWindowHandle = CreateDialog(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_SYSGFX),
        NULL,
        MainWndProc
        );

    PhSetEvent(&InitializedEvent);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!IsDialogMessage(GfxWindowHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);
    PhResetEvent(&InitializedEvent);
    NtClose(GfxThreadHandle);

    GfxWindowHandle = NULL;
    GfxPanelWindowHandle = NULL;
    GfxThreadHandle = NULL;

    return STATUS_SUCCESS;
}

VOID LogEvent(__in PWSTR str, __in NvStatus status)
{
    if (NvAPI_GetErrorMessage != NULL)
    {
        PPH_STRING nvPhString = NULL;
        PPH_STRING statusString = NULL;
        NvAPI_ShortString nvString = { 0 };

        NvAPI_GetErrorMessage(status, nvString);

        nvPhString = PhCreateStringFromAnsi(nvString);
        statusString = PhFormatString(str, nvPhString->Buffer);

        PhLogMessageEntry(PH_LOG_ENTRY_MESSAGE, statusString);

        PhDereferenceObject(statusString);
        PhDereferenceObject(nvPhString);
    }
    else
    {
        PPH_STRING string = PhCreateString(L"gfxinfo: (LogEvent) NvAPI_GetErrorMessage was not initialized.");

        PhLogMessageEntry(PH_LOG_ENTRY_MESSAGE, string);

        PhDereferenceObject(string);
    }
}

VOID NvInit(VOID)
{
    NvStatus status = NvAPI_Initialize();

    if (NV_SUCCESS(status))
    {
        //EnumNvidiaGpuHandles();
    }
    else
    {
        LogEvent(L"gfxinfo: (NvInit) NvAPI_Initialize failed (%s)", status);
    }
}

NvPhysicalGpuHandle EnumNvidiaGpuHandles()
{
    NvPhysicalGpuHandle szGPUHandle[NVAPI_MAX_PHYSICAL_GPUS] = { 0 };     
    NvU32 gpuCount = 0;
    ULONG i = 0;

    NvStatus status = NvAPI_EnumPhysicalGPUs(szGPUHandle, &gpuCount);

    if (NV_SUCCESS(status))
    {
        for (i = 0; i < gpuCount; i++)
        {
            NvDisplayHandle zero = 0;

            if (NV_SUCCESS(NvAPI_EnumNvidiaDisplayHandle(i, &zero)))
            {
                NvU32 num3;

                NvPhysicalGpuHandle gpuHandles2[0x40];

                if (NV_SUCCESS(NvAPI_GetPhysicalGPUsFromDisplay(zero, gpuHandles2, &num3)))
                {
                    ULONG gpuCount2;

                    for (gpuCount2 = 0; gpuCount2 < num3; gpuCount2++)
                    {
                        //if (!NVidiaGPUs.ContainsKey(gpuCount2))
                           // NVidiaGPUs.Add(gpuCount2, new NvidiaGPU(i, gpuHandles2[gpuCount2], zero));
                        return gpuHandles2[gpuCount2];
                    }
                }
            }
        }
    }
    else
    {
        LogEvent(L"gfxinfo: (EnumNvidiaGpuHandles) NvAPI_EnumPhysicalGPUs failed (%s)", status);
    }

    return szGPUHandle[0];
}

NvDisplayHandle EnumNvidiaDisplayHandles(VOID)
{
    NvPhysicalGpuHandle szGPUHandle[NVAPI_MAX_PHYSICAL_GPUS] = { 0 };     
    NvU32 gpuCount = 0;
    ULONG i = 0;

    NvStatus status = NvAPI_EnumPhysicalGPUs(szGPUHandle, &gpuCount);

    if (NV_SUCCESS(status))
    {
        for (i = 0; i < gpuCount; i++)
        {
            NvDisplayHandle zero = 0;

            if (NV_SUCCESS(NvAPI_EnumNvidiaDisplayHandle(i, &zero)))
            {
                return zero;
            }
        }
    }
    else
    {
        LogEvent(L"gfxinfo: (EnumNvidiaDisplayHandles) NvAPI_EnumPhysicalGPUs failed (%s)", status);
    }

    return NULL;
}


VOID GetGfxUsages(VOID)
{
    NvStatus status = NVAPI_ERROR;
    NvPhysicalGpuHandle physHandle = NULL;
    NvDisplayHandle dispHandle = NULL;

    NV_USAGES_INFO_V1 gpuInfo = { 0 };
    NV_MEMORY_INFO_V2 memInfo = { 0 };
    gpuInfo.Version = NV_USAGES_INFO_VER;
    memInfo.Version = NV_MEMORY_INFO_VER;

    physHandle = EnumNvidiaGpuHandles();
    dispHandle = EnumNvidiaDisplayHandles();
    
    status = NvAPI_GetUsages(physHandle, &gpuInfo);

    if (NV_SUCCESS(status))
    {
        UINT gfxCoreLoad = gpuInfo.Values[2];
        //UINT gfxCoreUsage = gpuInfo.Values[3];
        UINT gfxMemControllerLoad = gpuInfo.Values[6]; 
        //UINT gfxVideoEngineLoad = gpuInfo.Values[10];

        CurrentGpuUsage = (FLOAT)gfxCoreLoad / 100;
        CurrentCoreUsage = (FLOAT)gfxMemControllerLoad / 100;

        PhAddItemCircularBuffer_FLOAT(&GpuHistory, CurrentGpuUsage);
        PhAddItemCircularBuffer_FLOAT(&CoreHistory, CurrentCoreUsage);
    }
    else
    {
        LogEvent(L"gfxinfo: (GetNvidiaGpuUsages) NvAPI_GetUsages failed (%s)", status);
    }

    status = NvAPI_GetMemoryInfo(dispHandle, &memInfo);
     
    if (NV_SUCCESS(status))
    {
        UINT totalMemory = memInfo.Values[0];
        UINT freeMemory = memInfo.Values[4];
        
        ULONG usedMemory = max(totalMemory - freeMemory, 0);
        
        MaxMemUsage = totalMemory;
        CurrentMemUsage = (FLOAT)usedMemory;
        
        PhAddItemCircularBuffer_ULONG(&MemHistory, usedMemory);
    }
    else
    {
        LogEvent(L"gfxinfo: (GetNvidiaGpuUsages) NvAPI_GetMemoryInfo failed (%s)", status);
    }
}

PPH_STRING GetDriverName(VOID)
{
    NvStatus status = NVAPI_ERROR;
    NvPhysicalGpuHandle physHandle = NULL;

    NvAPI_ShortString str = { 0 };
    physHandle = EnumNvidiaGpuHandles();
    status = NvAPI_GetFullName(physHandle, str);

    if (NV_SUCCESS(status))
    {
        return PhCreateStringFromAnsi(str);
    }
    else
    {
        LogEvent(L"gfxinfo: (GetFullName) NvAPI_GetFullName failed (%s)", status);
    }

    return NULL;
}

VOID GetDriverVersion(VOID)
{
    NvStatus status = NVAPI_ERROR;
    NvDisplayHandle dispHandle = NULL;

    NV_DISPLAY_DRIVER_VERSION versionInfo = { 0 };
    versionInfo.Version = NV_DISPLAY_DRIVER_VERSION_VER;

    dispHandle = EnumNvidiaDisplayHandles();
    status = NvAPI_GetDisplayDriverVersion(dispHandle, &versionInfo);

    if (NV_SUCCESS(status))
    {
        PH_STRING_BUILDER strb = { 0 };

        PhInitializeStringBuilder(&strb, 30);

        PhAppendFormatStringBuilder(&strb, L"Driver Version: %d", versionInfo.drvVersion / 100);
        PhAppendFormatStringBuilder(&strb, L".%d", versionInfo.drvVersion % 100);
        //PhAppendFormatStringBuilder(&strb, L" Driver Branch: %ls", versionInfo.szBuildBranchString);

        PhDeleteStringBuilder(&strb);
    }
    else
    {
        LogEvent(L"gfxinfo: (GetDriverVersion) NvAPI_GetDisplayDriverVersion failed (%s)", status);
    }
}

VOID GetGfxTemp(VOID)
{
    NvStatus status = NVAPI_ERROR;
    NvPhysicalGpuHandle dispHandle = NULL;

    NV_GPU_THERMAL_SETTINGS_V2 thermalInfo = { 0 };
    thermalInfo.Version = NV_GPU_THERMAL_SETTINGS_VER;

    dispHandle = EnumNvidiaGpuHandles();

    status = NvAPI_GetThermalSettings(dispHandle, NVAPI_THERMAL_TARGET_ALL, &thermalInfo);

    if (NV_SUCCESS(status))
    {
        GfxCoreTempCount = thermalInfo.Sensor[0].CurrentTemp;
        GfxBoardTempCount = thermalInfo.Sensor[1].CurrentTemp;
    }
    else
    {
        LogEvent(L"gfxinfo: (GetGfxTemp) NvAPI_GetThermalSettings failed (%s)", status);
    }
}

VOID GetGfxFanSpeed(VOID)
{
    NvStatus status = NVAPI_ERROR;
    NvPhysicalGpuHandle dispHandle = NULL;

    NV_COOLER_INFO_V2 coolInfo = { 0 };
    coolInfo.Version = NV_COOLER_INFO_VER;

    dispHandle = EnumNvidiaGpuHandles();

    status = NvAPI_GetCoolerSettings(dispHandle, 0, &coolInfo);

    if (NV_SUCCESS(status))
    {

    }
    else
    {
        LogEvent(L"gfxinfo: (GetGfxFanSpeed) NvAPI_GetCoolerSettings failed (%s)", status);
    }
}

VOID GetGfxClockSpeeds(VOID)
{
    NvStatus status = NVAPI_ERROR;
    NvPhysicalGpuHandle dispHandle = NULL;

    NV_CLOCKS_INFO_V2 clockInfo = { 0 };
    clockInfo.Version = NV_CLOCKS_INFO_VER;

    dispHandle = EnumNvidiaGpuHandles();

    status = NvAPI_GetAllClocks(dispHandle, &clockInfo);

    if (NV_SUCCESS(status))
    {
        //clocks[0] = "GPU Core"
        //clocks[1] = "GPU Memory"
        //clocks[2] = "GPU Shader"

        //clocks[0] = 0.001f * clockInfo.Values[0];
        //clocks[1] = 0.001f * clockInfo.Values[8];
        //clocks[2] = 0.001f * clockInfo.Values[14];

        //if (clocks[30] != 0) 
        //{
        //    clocks[0] = 0.0005f * clockInfo.Values[30];
        //    clocks[2] = 0.001f * clockInfo.Values[30];
        //}

        GfxCoreClockCount = 0.001f * clockInfo.Values[0];
        GfxMemoryClockCount = 0.001f * clockInfo.Values[8];
        GfxShaderClockCount = 0.001f * clockInfo.Values[14];
    }
    else
    {
        LogEvent(L"gfxinfo: (GetGfxClockSpeeds) NvAPI_GetAllClocks failed (%s)", status);
    }
}