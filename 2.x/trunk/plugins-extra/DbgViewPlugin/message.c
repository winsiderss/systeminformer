/*
 * Process Hacker Extra Plugins -
 *   Debug View Plugin
 *
 * Copyright (C) 2015 dmex
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

#include "main.h"

static PH_LAYOUT_MANAGER LayoutManager;

INT_PTR CALLBACK DbgPropDlgProc(
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
            PDEBUG_LOG_ENTRY entry = (PDEBUG_LOG_ENTRY)lParam;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_MESSAGE), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDCLOSE), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            Edit_SetText(GetDlgItem(hwndDlg, IDC_MESSAGE), entry->Message->Buffer);
            //SendMessage(GetDlgItem(hwndDlg, IDC_MESSAGE), WM_SETFONT, (WPARAM)PhApplicationFont, FALSE);
        }
        return TRUE;
    case WM_DESTROY:
        PhDeleteLayoutManager(&LayoutManager);
        break;    
    case WM_SIZE:
        PhLayoutManagerLayout(&LayoutManager);
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDCLOSE:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        {
            HDC hDC = (HDC)wParam;
            HWND hwndChild = (HWND)lParam;

            // Check for our static label and change the color.
            if (GetDlgCtrlID(hwndChild) == IDC_MESSAGE)
            {
                SetBkMode(hDC, TRANSPARENT);

                // set window background color.
                return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
            }
        }       
        break;
    }

    return FALSE;
}