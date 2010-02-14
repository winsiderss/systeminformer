/*
 * Process Hacker - 
 *   hidden processes detection
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

#include <phgui.h>
#include <windowsx.h>

INT_PTR CALLBACK PhpHiddenProcessesDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

HWND PhHiddenProcessesWindowHandle = NULL;
HWND PhHiddenProcessesListViewHandle = NULL;
static PH_LAYOUT_MANAGER WindowLayoutManager;

VOID PhShowHiddenProcessesDialog()
{
    if (!PhHiddenProcessesWindowHandle)
    {
        PhHiddenProcessesWindowHandle = CreateDialog(
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_HIDDENPROCESSES),
            PhMainWndHandle,
            PhpHiddenProcessesDlgProc
            );
    }

    if (!IsWindowVisible(PhHiddenProcessesWindowHandle))
        ShowWindow(PhHiddenProcessesWindowHandle, SW_SHOW);
    else
        BringWindowToTop(PhHiddenProcessesWindowHandle);
}

static INT_PTR CALLBACK PhpHiddenProcessesDlgProc(      
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
            HWND lvHandle;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));
            PhHiddenProcessesListViewHandle = lvHandle = GetDlgItem(hwndDlg, IDC_PROCESSES);

            PhInitializeLayoutManager(&WindowLayoutManager, hwndDlg);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_INTRO),
                NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_FORCE_INVALIDATE);
            PhAddLayoutItem(&WindowLayoutManager, lvHandle,
                NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_DESCRIPTION),
                NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM | PH_FORCE_INVALIDATE);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_METHOD),
                NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_TERMINATE),
                NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_SAVE),
                NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_SCAN),
                NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDOK),
                NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            PhRegisterDialog(hwndDlg);

            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 320, L"Process");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 60, L"PID");

            PhSetExtendedListView(lvHandle);

            ComboBox_AddString(GetDlgItem(hwndDlg, IDC_METHOD), L"Brute Force");
            ComboBox_AddString(GetDlgItem(hwndDlg, IDC_METHOD), L"CSR Handles");
            ComboBox_SelectString(GetDlgItem(hwndDlg, IDC_METHOD), -1, L"CSR Handles");
        }
        break;
    case WM_CLOSE:
        {
            // Hide, don't close.
            ShowWindow(hwndDlg, SW_HIDE);
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, 0);
        }
        return TRUE;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                {
                    SendMessage(hwndDlg, WM_CLOSE, 0, 0);
                }
                break;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&WindowLayoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, 476, 380);
        }
        break;
    }

    return FALSE;
}
