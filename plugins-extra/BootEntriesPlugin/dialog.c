/*
 * Process Hacker Extra Plugins -
 *   Boot Entries Plugin
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

static PPH_STRING PhGetSelectedListViewItemText(
    _In_ HWND hWnd
    )
{
    INT index = PhFindListViewItemByFlags(
        hWnd,
        -1,
        LVNI_SELECTED
        );

    if (index != -1)
    {
        WCHAR textBuffer[MAX_PATH] = L"";

        LVITEM item;
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 0;
        item.pszText = textBuffer;
        item.cchTextMax = _countof(textBuffer);

        if (ListView_GetItem(hWnd, &item))
            return PhCreateString(textBuffer);
    }

    return NULL;
}

static VOID ShowBootEntryMenu(
    _In_ PBOOT_WINDOW_CONTEXT Context,
    _In_ HWND hwndDlg
    )
{
    HMENU menu;
    HMENU subMenu;
    ULONG id;
    POINT cursorPos = { 0, 0 };

    PPH_STRING bootEntryName = PhGetSelectedListViewItemText(Context->ListViewHandle);

    if (bootEntryName)
    {
        GetCursorPos(&cursorPos);

        menu = LoadMenu(
            PluginInstance->DllBase,
            MAKEINTRESOURCE(IDR_BOOT_ENTRY_MENU)
            );

        subMenu = GetSubMenu(menu, 0);

        id = (ULONG)TrackPopupMenu(
            subMenu,
            TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
            cursorPos.x,
            cursorPos.y,
            0,
            hwndDlg,
            NULL
            );

        DestroyMenu(menu);

        switch (id)
        {
        case ID_MENU_MOVEUP:
        case ID_MENU_MOVEDOWN:
            break;
        }

        PhDereferenceObject(bootEntryName);
    }
}

INT_PTR CALLBACK BootEntriesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PBOOT_WINDOW_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PBOOT_WINDOW_CONTEXT)PhAllocate(sizeof(BOOT_WINDOW_CONTEXT));
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PBOOT_WINDOW_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
        {
            PhSaveWindowPlacementToSetting(SETTING_NAME_WINDOW_POSITION, SETTING_NAME_WINDOW_SIZE, hwndDlg);
            PhDeleteLayoutManager(&context->LayoutManager);
            PhUnregisterDialog(hwndDlg);
            RemoveProp(hwndDlg, L"Context");
            PhFree(context);
        }
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HANDLE threadHandle;

            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_BOOT_LIST);

            PhRegisterDialog(hwndDlg);
            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 30, L"ID");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 140, L"Type");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 340, L"Name");
            PhSetExtendedListView(context->ListViewHandle);

            ExtendedListView_SetSortFast(context->ListViewHandle, TRUE);
            //ExtendedListView_SetCompareFunction(context->ListViewHandle, 0, PhpObjectProcessCompareFunction);
            //ExtendedListView_SetCompareFunction(context->ListViewHandle, 1, PhpObjectProcessCompareFunction);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_LISTVIEW_COLUMNS, context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_BOOT_REFRESH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhLoadWindowPlacementFromSetting(SETTING_NAME_WINDOW_POSITION, SETTING_NAME_WINDOW_SIZE, hwndDlg);

            if (threadHandle = PhCreateThread(0, EnumerateBootEntriesThread, context->ListViewHandle))
            {
                NtClose(threadHandle);
            }
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_BOOT_REFRESH:
                {
                    HANDLE threadHandle;

                    ListView_DeleteAllItems(context->ListViewHandle);

                    if (threadHandle = PhCreateThread(0, EnumerateBootEntriesThread, context->ListViewHandle))
                    {
                        NtClose(threadHandle);
                    }
                }
                break;
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR)lParam;

            switch (hdr->code)
            {
            case NM_RCLICK:
                {
                    if (hdr->hwndFrom == context->ListViewHandle)
                        ShowBootEntryMenu(context, hwndDlg);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}