/*
 * Process Hacker - 
 *   system information
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

#include <phapp.h>
#include <settings.h>
#include <windowsx.h>

#define WM_PH_SYSINFO_ACTIVATE (WM_APP + 150)

NTSTATUS PhpSysInfoThreadStart(
    __in PVOID Parameter
    );

INT_PTR CALLBACK PhpSysInfoDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhpSysInfoPanelDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

HANDLE PhSysInfoThreadHandle = NULL;
HWND PhSysInfoWindowHandle = NULL;
HWND PhSysInfoPanelWindowHandle = NULL;
static PH_EVENT InitializedEvent = PH_EVENT_INIT;
static PH_LAYOUT_MANAGER WindowLayoutManager;

static BOOLEAN OneGraphPerCpu;
static BOOLEAN AlwaysOnTop;

VOID PhShowSystemInformationDialog()
{
    if (!PhSysInfoWindowHandle)
    {
        if (!(PhSysInfoThreadHandle = PhCreateThread(0, PhpSysInfoThreadStart, NULL)))
        {
            PhShowStatus(PhMainWndHandle, L"Unable to create the system information window", 0, GetLastError());
            return;
        }

        PhWaitForEvent(&InitializedEvent, NULL);
    }

    SendMessage(PhSysInfoWindowHandle, WM_PH_SYSINFO_ACTIVATE, 0, 0);
}

static NTSTATUS PhpSysInfoThreadStart(
    __in PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    BOOL result;
    MSG message;

    PhInitializeAutoPool(&autoPool);

    PhSysInfoWindowHandle = CreateDialog(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_SYSINFO),
        PhMainWndHandle,
        PhpSysInfoDlgProc
        );

    PhSetEvent(&InitializedEvent);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!IsDialogMessage(PhSysInfoWindowHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);
    PhResetEvent(&InitializedEvent);
    NtClose(PhSysInfoThreadHandle);

    PhSysInfoWindowHandle = NULL;
    PhSysInfoPanelWindowHandle = NULL;
    PhSysInfoThreadHandle = NULL;

    return STATUS_SUCCESS;
}

static VOID PhpSetAlwaysOnTop()
{
    SetWindowPos(PhSysInfoWindowHandle, AlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

static VOID PhpSetOneGraphPerCpu()
{
    PhLayoutManagerLayout(&WindowLayoutManager);
}

INT_PTR CALLBACK PhpSysInfoDlgProc(      
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
            PH_RECTANGLE windowRectangle;

            PhInitializeLayoutManager(&WindowLayoutManager, hwndDlg);

            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_ONEGRAPHPERCPU), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_ALWAYSONTOP), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            windowRectangle.Position = PhGetIntegerPairSetting(L"SysInfoWindowPosition");
            windowRectangle.Size = PhGetIntegerPairSetting(L"SysInfoWindowSize");

            PhAdjustRectangleToWorkingArea(
                hwndDlg,
                &windowRectangle
                );
            MoveWindow(hwndDlg, windowRectangle.Left, windowRectangle.Top,
                windowRectangle.Width, windowRectangle.Height, FALSE);
        }
        break;
    case WM_DESTROY:
        {
            WINDOWPLACEMENT placement = { sizeof(placement) };
            PH_RECTANGLE windowRectangle;

            PhSetIntegerSetting(L"SysInfoWindowAlwaysOnTop", AlwaysOnTop);
            PhSetIntegerSetting(L"SysInfoWindowOneGraphPerCpu", OneGraphPerCpu);

            GetWindowPlacement(hwndDlg, &placement);
            windowRectangle = PhRectToRectangle(placement.rcNormalPosition);

            PhSetIntegerPairSetting(L"SysInfoWindowPosition", windowRectangle.Position);
            PhSetIntegerPairSetting(L"SysInfoWindowSize", windowRectangle.Size);

            PhDeleteLayoutManager(&WindowLayoutManager);
            PostQuitMessage(0);
        }
        break;
    case WM_SHOWWINDOW:
        {
            RECT margin;

            PhSysInfoPanelWindowHandle = CreateDialog(
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_SYSINFO_PANEL),
                PhSysInfoWindowHandle,
                PhpSysInfoPanelDlgProc
                );

            SetWindowPos(PhSysInfoPanelWindowHandle, NULL, 10, 0, 0, 0,
                SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER);
            SetParent(PhSysInfoPanelWindowHandle, hwndDlg);
            ShowWindow(PhSysInfoPanelWindowHandle, SW_SHOW);

            AlwaysOnTop = (BOOLEAN)PhGetIntegerSetting(L"SysInfoWindowAlwaysOnTop");
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ALWAYSONTOP), AlwaysOnTop ? BST_CHECKED : BST_UNCHECKED);
            PhpSetAlwaysOnTop();

            OneGraphPerCpu = (BOOLEAN)PhGetIntegerSetting(L"SysInfoWindowOneGraphPerCpu");
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ONEGRAPHPERCPU), OneGraphPerCpu ? BST_CHECKED : BST_UNCHECKED);
            PhpSetOneGraphPerCpu();

            margin.left = 0;
            margin.top = 0;
            margin.right = 0;
            margin.bottom = 25;
            MapDialogRect(hwndDlg, &margin);

            PhAddLayoutItemEx(&WindowLayoutManager, PhSysInfoPanelWindowHandle, NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT, margin);

            PhLayoutManagerLayout(&WindowLayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&WindowLayoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, 620, 590);
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
                    PhpSetAlwaysOnTop();
                }
                break;
            case IDC_ONEGRAPHPERCPU:
                {
                    OneGraphPerCpu = Button_GetCheck(GetDlgItem(hwndDlg, IDC_ONEGRAPHPERCPU)) == BST_CHECKED;
                    PhpSetOneGraphPerCpu();
                }
                break;
            }
        }
        break;
    case WM_PH_SYSINFO_ACTIVATE:
        {
            if (IsIconic(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            BringWindowToTop(hwndDlg);
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PhpSysInfoPanelDlgProc(      
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

        }
        break;
    case WM_DESTROY:
        {

        }
        break;
    }

    return FALSE;
}
