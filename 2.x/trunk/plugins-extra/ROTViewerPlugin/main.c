/*
 * Running Object Table Plugin
 *
 * Copyright (C) 2012 dmex
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

VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );
VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );
INT_PTR CALLBACK RotViewDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );
VOID ShowStatusMenu(
    __in HWND hwndDlg,
    __in PPOINT Point,
    __in LPNMITEMACTIVATE lpnmitem
    );

static PPH_PLUGIN PluginInstance;
static PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
static PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;

LOGICAL DllMain(
    __in HINSTANCE Instance,
    __in ULONG Reason,
    __reserved PVOID Reserved
    )
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            PPH_PLUGIN_INFORMATION info;

            PluginInstance = PhRegisterPlugin(L"ProcessHacker.RunningObjectTable", Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"RunningObjectTable Viewer";
            info->Author = L"dmex";
            info->Description = L"Plugin for viewing the ROT (RunningObjectTable) via the Tools > Running Object Table";
            info->HasOptions = FALSE;

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainWindowShowing),
                MainWindowShowingCallback,
                NULL,
                &MainWindowShowingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackMenuItem),
                MenuItemCallback,
                NULL,
                &PluginMenuItemCallbackRegistration
                );
        }
        break;
    }

    return TRUE;
}

static VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    // Add our menu item, 4 = Help menu.
    PhPluginAddMenuItem(PluginInstance, PH_MENU_ITEM_LOCATION_TOOLS, L"$", UPDATE_MENUITEM, L"Running Object Table", NULL);
}

static VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = (PPH_PLUGIN_MENU_ITEM)Parameter;

    if (menuItem != NULL)
    {
        switch (menuItem->Id)
        {
        case UPDATE_MENUITEM:
            {
                DialogBox(
                    (HINSTANCE)PluginInstance->DllBase,
                    MAKEINTRESOURCE(IDD_ROT),
                    PhMainWndHandle,
                    RotViewDlgProc
                    );
            }
            break;
        }
    }
}

static PVOID ListViewGetlParam(
    __in HWND hwndDlg, 
    __in INT Index
    )
{
    INT itemIndex;
    LVITEM item;

    if (Index == -1)
    {
        itemIndex = ListView_GetNextItem(hwndDlg, -1, LVNI_VISIBLEONLY);
        if (itemIndex == -1)
            return NULL;
    }
    else
    {
        itemIndex = Index;
    }

    memset(&item, 0, sizeof(LVITEM));

    item.mask = LVIF_PARAM;
    item.iItem = itemIndex;

    if (!ListView_GetItem(hwndDlg, &item))
        return NULL;

    return (PVOID)item.lParam;
}

static VOID EnumRunningObjectTable(
    __in HWND hwndDlg
    )
{
    IRunningObjectTable* table;
    IEnumMoniker* moniker;
    IMoniker* pmkObjectNames;
    IBindCtx* ctx;
    IMalloc* iMalloc;

    ULONG count = 0;
    ULONG index = 0;

    CoGetMalloc(1, &iMalloc);

    // Query the running object table address
    if (SUCCEEDED(GetRunningObjectTable(0, &table)))
    {
        // Enum the objects registered
        if (SUCCEEDED(IRunningObjectTable_EnumRunning(table, &moniker)))
        {
            while (IEnumMoniker_Next(moniker, 1, &pmkObjectNames, &count) == S_OK)
            {
                INT itemIndex = 0;
                PPH_STRING indexString = NULL;

                indexString = PhIntegerToString64(index, 0, FALSE);

                // Add the item to the listview (it might be nameless) 
                //  - pass the object to the listview so we never have to enumerate the ROT to find it again...
                itemIndex = PhAddListViewItem(hwndDlg, MAXINT, indexString->Buffer, pmkObjectNames);

                if (SUCCEEDED(CreateBindCtx(0, &ctx)))
                {
                    OLECHAR* name = NULL;

                    // Query the object name
                    if (SUCCEEDED(IMoniker_GetDisplayName(pmkObjectNames, ctx, NULL, &name)))
                    {
                        // Set the items name column
                        PhSetListViewSubItem(hwndDlg, itemIndex, 1, name);

                        // Free the object name
                        IMalloc_Free(iMalloc, name);
                    }
                }    

                PhDereferenceObject(indexString);

                index++;
            }

            IEnumMoniker_Release(moniker);
            moniker = NULL;
        }

        IRunningObjectTable_Release(table);
        table = NULL;
    }
}

INT_PTR CALLBACK RotViewDlgProc(
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
            HWND listView = GetDlgItem(hwndDlg, IDC_LIST1);

            PhCenterWindow(hwndDlg, PhMainWndHandle);

            PhSetListViewStyle(listView, FALSE, TRUE);
            PhSetControlTheme(listView, L"explorer");
            PhAddListViewColumn(listView, 0, 0, 0, LVCFMT_LEFT, 40, L"Index");
            PhAddListViewColumn(listView, 1, 1, 1, LVCFMT_LEFT, 400, L"Display Name");
            PhSetExtendedListView(listView);

            EnumRunningObjectTable(listView);
        }
        break;
    case WM_DESTROY:
        {
            // We added a param for each listview item - we must free them
            HWND listView = GetDlgItem(hwndDlg, IDC_LIST1);
            INT itemCount = ListView_GetItemCount(listView) - 1;
            IMoniker* pmkObjectNames;

            while (itemCount >= 0)
            {
                pmkObjectNames = (IMoniker*)ListViewGetlParam(listView, itemCount);

                if (pmkObjectNames)
                    IEnumMoniker_Release(pmkObjectNames);

                itemCount--;
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
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
                    POINT cursorPos = { 0 };

                    GetCursorPos(&cursorPos);

                    ShowStatusMenu(hwndDlg, &cursorPos, (LPNMITEMACTIVATE)lParam);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

VOID ShowStatusMenu(
    __in HWND hwndDlg,
    __in PPOINT Point,
    __in LPNMITEMACTIVATE lpnmitem
    )
{
    HMENU menu;
    HMENU subMenu;
    ULONG id;

    menu = LoadMenu(
        (HINSTANCE)PluginInstance->DllBase,
        MAKEINTRESOURCE(IDR_CONTEXTMENU)
        );

    subMenu = GetSubMenu(menu, 0);

    id = (ULONG)TrackPopupMenu(
        subMenu,
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
        Point->x,
        Point->y,
        0,
        hwndDlg,
        NULL
        );

    DestroyMenu(menu);

    switch (id)
    {
    case ID_MENU_PROPERTIES:
        {
            IMoniker* pmkObjectName;
            INT lvItemIndex;

            lvItemIndex = ListView_GetNextItem(GetDlgItem(hwndDlg, IDC_LIST1), -1, LVNI_SELECTED);

            if (lvItemIndex != -1 && PhGetListViewItemParam(GetDlgItem(hwndDlg, IDC_LIST1), lvItemIndex, &pmkObjectName))
            {
                DialogBoxParam(
                    (HINSTANCE)PluginInstance->DllBase,
                    MAKEINTRESOURCE(IDD_DIALOG1),
                    hwndDlg,
                    PropDialogProc,
                    (LPARAM)pmkObjectName
                    );
            }
        }
        break;
    }
}