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

static HWND GpuGraphHandle;
static HWND MemGraphHandle;

HWND EtpEtwSysWindowHandle = NULL;
HWND EtpEtwSysPanelWindowHandle = NULL;

static PH_GRAPH_STATE GpuGraphState;
static PH_GRAPH_STATE MemGraphState;

static VOID NTAPI EtwSysUpdateHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

PH_CIRCULAR_BUFFER_ULONG GpuHistory;
PH_CIRCULAR_BUFFER_ULONG MemHistory;

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
            PhSetWindowStyle(hwndDlg, WS_CLIPCHILDREN, WS_CLIPCHILDREN);
          
            PhCenterWindow(hwndDlg, PhMainWndHandle);
            
            PhInitializeLayoutManager(&WindowLayoutManager, hwndDlg);
  
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_ALWAYSONTOP), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            PhInitializeGraphState(&GpuGraphState);
            PhInitializeGraphState(&MemGraphState);

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
      
            NvInit();

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
             PhDeleteLayoutManager(&WindowLayoutManager);

             PhUnregisterCallback(&PhProcessesUpdatedEvent, &ProcessesUpdatedRegistration);

             PhDeleteGraphState(&GpuGraphState);
             PhDeleteGraphState(&MemGraphState);
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
    case WM_SHOWWINDOW:
        {
            RECT margin;

            EtpEtwSysPanelWindowHandle = CreateDialog(
                PluginInstance->DllBase,
                MAKEINTRESOURCE(IDD_SYSGFX_PANEL),
                hwndDlg,
                EtpEtwSysPanelDlgProc
                );

            SetWindowPos(EtpEtwSysPanelWindowHandle, NULL, 10, 0, 0, 0,
                SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER);
            ShowWindow(EtpEtwSysPanelWindowHandle, SW_SHOW);

            //AlwaysOnTop = (BOOLEAN)PhGetIntegerSetting(SETTING_NAME_ETWSYS_ALWAYS_ON_TOP);
            //Button_SetCheck(GetDlgItem(hwndDlg, IDC_ALWAYSONTOP), AlwaysOnTop ? BST_CHECKED : BST_UNCHECKED);
            //EtpSetAlwaysOnTop();

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

            //DiskGraphState.Valid = FALSE;
            //NetworkGraphState.Valid = FALSE;

            GetClientRect(hwndDlg, &clientRect);
            // Limit the rectangle bottom to the top of the panel.
            GetWindowRect(EtpEtwSysPanelWindowHandle, &panelRect);
            MapWindowPoints(NULL, hwndDlg, (POINT *)&panelRect, 2);
            clientRect.bottom = panelRect.top;

            width = clientRect.right - margin.left - margin.right;
            height = (clientRect.bottom - margin.top - margin.bottom - between * 2) / 2;

            deferHandle = BeginDeferWindowPos(4);

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

            EndDeferWindowPos(deferHandle);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, 500, 400);
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
            //DiskGraphState.Valid = FALSE;
            //DiskGraphState.TooltipIndex = -1;
            Graph_MoveGrid(GpuGraphHandle, 1);
            Graph_Draw(GpuGraphHandle);
            Graph_UpdateTooltip(GpuGraphHandle);
            InvalidateRect(GpuGraphHandle, NULL, FALSE);

            //NetworkGraphState.Valid = FALSE;
            //NetworkGraphState.TooltipIndex = -1;
            Graph_MoveGrid(MemGraphHandle, 1);
            Graph_Draw(MemGraphHandle);
            Graph_UpdateTooltip(MemGraphHandle);
            InvalidateRect(MemGraphHandle, NULL, FALSE);

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
            //SetDlgItemText(hwndDlg, IDC_ZREADS_V, PhaFormatUInt64(EtDiskReadCount, TRUE)->Buffer);
            //SetDlgItemText(hwndDlg, IDC_ZREADBYTES_V, PhaFormatSize(EtDiskReadDelta.Value, -1)->Buffer);
            //SetDlgItemText(hwndDlg, IDC_ZWRITES_V, PhaFormatUInt64(EtDiskWriteCount, TRUE)->Buffer);
            //SetDlgItemText(hwndDlg, IDC_ZWRITEBYTES_V, PhaFormatSize(EtDiskWriteDelta.Value, -1)->Buffer);

            //SetDlgItemText(hwndDlg, IDC_ZRECEIVES_V, PhaFormatUInt64(EtNetworkReceiveCount, TRUE)->Buffer);
            //SetDlgItemText(hwndDlg, IDC_ZRECEIVEBYTES_V, PhaFormatSize(EtNetworkReceiveDelta.Value, -1)->Buffer);
            //SetDlgItemText(hwndDlg, IDC_ZSENDS_V, PhaFormatUInt64(EtNetworkSendCount, TRUE)->Buffer);
            //SetDlgItemText(hwndDlg, IDC_ZSENDBYTES_V, PhaFormatSize(EtNetworkSendDelta.Value, -1)->Buffer);
        }
        break;
    }

    return FALSE;
}

static VOID NTAPI EtwSysUpdateHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PostMessage(EtpEtwSysWindowHandle, WM_ET_ETWSYS_UPDATE, 0, 0);
}

VOID ShowDialog(VOID)
{
    DialogBox(
        (HINSTANCE)PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_SYSGFX),
        NULL,
        MainWndProc
        );
}

VOID LogEvent(__in PWSTR str, __in INT status)
{
    PPH_STRING nvPhString = NULL;
    PPH_STRING statusString = NULL;
    NvAPI_ShortString nvString = { 0 };

    if (NvAPI_GetErrorMessage != NULL)
    {
        NvAPI_GetErrorMessage((NvAPI_Status)status, nvString);

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

VOID NvInit()
{
    NvAPI_Status status = NvAPI_Initialize();

    if (NV_SUCCESS(status))
    {
        EnumNvidiaGpuHandles();
        GetNvidiaGpuUsages();
    }
    else
    {
        LogEvent(L"gfxinfo: (NvInit) NvAPI_Initialize failed (%s)", status);
    }
}

VOID EnumNvidiaGpuHandles()
{
    INT i = 0, gpuCount = 0;
    PINT gpuHandles[0x40];

    NvAPI_Status status = NvAPI_EnumPhysicalGPUs(gpuHandles, &gpuCount);

    if (NV_SUCCESS(status))
    {
        for (i = 0; i < gpuCount; i++)
        {
            //INT zero = 0;

            /*if (NV_SUCCESS(NvAPI_EnumNvidiaDisplayHandle(i, &zero)))
            {
                uint num3;

                IntPtr[] gpuHandles2 = new IntPtr[0x40];
                if (NvAPI_GetPhysicalGPUsFromDisplay(zero, gpuHandles2, out num3).IsSuccess())
                {
                    for (int gpuCount2 = 0; gpuCount2 < num3; gpuCount2++)
                    {
                        if (!NVidiaGPUs.ContainsKey(gpuCount2))
                        {
                            NVidiaGPUs.Add(gpuCount2, new NvidiaGPU(i, gpuHandles2[gpuCount2], zero));
                        }
                    }
                }
            }*/
        }
    }
    else
    {
        LogEvent(L"gfxinfo: (EnumNvidiaGpuHandles) NvAPI_EnumPhysicalGPUs failed (%s)", status);
    }
}


VOID GetNvidiaGpuUsages()
{
    unsigned int gpuUsages[NVAPI_MAX_USAGES_PER_GPU] = { 0 };

    int gpuCount = 0;
    int *gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = { NULL };
    // gpuUsages[0] must be this value, otherwise NvAPI_GPU_GetUsages won't work
    gpuUsages[0] = (NVAPI_MAX_USAGES_PER_GPU * 4) | 0x10000;
    (*NvAPI_EnumPhysicalGPUs)(gpuHandles, &gpuCount);

    NvAPI_GetUsages(gpuHandles[0], gpuUsages);
    {
        int coreLoad = gpuUsages[2];
        int usage = gpuUsages[3];
        int memLoad = gpuUsages[6]; 
        int engineLoad = gpuUsages[10];
    }
}
