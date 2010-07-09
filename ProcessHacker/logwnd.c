/*
 * Process Hacker - 
 *   log window
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
#include <windowsx.h>

#define WM_PH_LOG_UPDATED (WM_APP + 300)

INT_PTR CALLBACK PhpLogDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

HWND PhLogWindowHandle = NULL;
static PH_LAYOUT_MANAGER WindowLayoutManager;
static HWND ListViewHandle;
static ULONG ListViewCount;
static PH_CALLBACK_REGISTRATION LoggedRegistration;

VOID PhShowLogDialog()
{
    if (!PhLogWindowHandle)
    {
        PhLogWindowHandle = CreateDialog(
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_LOG),
            NULL,
            PhpLogDlgProc
            );
        PhRegisterDialog(PhLogWindowHandle);
        ShowWindow(PhLogWindowHandle, SW_SHOW);
    }

    if (IsIconic(PhLogWindowHandle))
        ShowWindow(PhLogWindowHandle, SW_RESTORE);
    else
        SetForegroundWindow(PhLogWindowHandle);
}

static VOID NTAPI LoggedCallback(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PostMessage(PhLogWindowHandle, WM_PH_LOG_UPDATED, 0, 0); 
}

static VOID PhpUpdateLogList()
{
    ListViewCount = PhLogBuffer.Count;
    ListView_SetItemCount(ListViewHandle, ListViewCount);

    if (ListViewCount >= 2 && Button_GetCheck(GetDlgItem(PhLogWindowHandle, IDC_AUTOSCROLL)) == BST_CHECKED)
    {
        // This is a real WTF. EnsureVisible doesn't work if IsItemVisible is used and there is 
        // an item selected.
        //if (ListView_IsItemVisible(ListViewHandle, ListViewCount - 2))
        ListView_EnsureVisible(ListViewHandle, ListViewCount - 1, FALSE);
    }
}

INT_PTR CALLBACK PhpLogDlgProc(      
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
            ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(ListViewHandle, L"explorer");
            PhAddListViewColumn(ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 140, L"Time");
            PhAddListViewColumn(ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 260, L"Message");

            PhInitializeLayoutManager(&WindowLayoutManager, hwndDlg);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_LIST), NULL,
                PH_ANCHOR_ALL);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDOK), NULL,
                PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_COPY), NULL,
                PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_SAVE), NULL,
                PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_CLEAR), NULL,
                PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);

            Button_SetCheck(GetDlgItem(hwndDlg, IDC_AUTOSCROLL), BST_CHECKED);

            PhRegisterCallback(&PhLoggedCallback, LoggedCallback, NULL, &LoggedRegistration);
            PhpUpdateLogList();
            ListView_EnsureVisible(ListViewHandle, ListViewCount - 1, FALSE);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&WindowLayoutManager);

            PhUnregisterCallback(&PhLoggedCallback, &LoggedRegistration);
            PhUnregisterDialog(PhLogWindowHandle);
            PhLogWindowHandle = NULL;
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
            case IDC_CLEAR:
                {
                    PhClearLogEntries();
                    PhpUpdateLogList();
                }
                break;
            case IDC_COPY:
                {

                }
                break;
            case IDC_SAVE:
                {

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
            case LVN_GETDISPINFO:
                {
                    NMLVDISPINFO *dispInfo = (NMLVDISPINFO *)header;
                    PPH_LOG_ENTRY entry;

                    entry = PhCircularBufferGet_PVOID(&PhLogBuffer, ListViewCount - dispInfo->item.iItem - 1);

                    if (dispInfo->item.iSubItem == 0)
                    {
                        if (dispInfo->item.mask & LVIF_TEXT)
                        {
                            SYSTEMTIME systemTime;
                            PPH_STRING dateTime;

                            PhLargeIntegerToLocalSystemTime(&systemTime, &entry->Time);
                            dateTime = PhaFormatDateTime(&systemTime);
                            wcscpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, dateTime->Buffer);
                        }
                    }
                    else if (dispInfo->item.iSubItem == 1)
                    {
                        if (dispInfo->item.mask & LVIF_TEXT)
                        {
                            PPH_STRING string;

                            string = PhFormatLogEntry(entry);
                            wcscpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, string->Buffer);
                            PhDereferenceObject(string);
                        }
                    }
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
            PhResizingMinimumSize((PRECT)lParam, wParam, 400, 250);
        }
        break;
    case WM_PH_LOG_UPDATED:
        {
            PhpUpdateLogList();
        }
        break;
    }

    return FALSE;
}
