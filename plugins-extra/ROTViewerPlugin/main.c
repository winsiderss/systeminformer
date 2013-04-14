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

static HWND ListViewWndHandle;
static PH_LAYOUT_MANAGER LayoutManager;
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
            info->Description = L"Plugin for viewing the RunningObjectTable via the Tools menu";
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
    PhPluginAddMenuItem(PluginInstance, PH_MENU_ITEM_LOCATION_TOOLS, L"$", ROT_TABLE_MENUITEM, L"Running Object Table", NULL);
}

static VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = (PPH_PLUGIN_MENU_ITEM)Parameter;

    switch (menuItem->Id)
    {
    case ROT_TABLE_MENUITEM:
        {
            DialogBox(
                (HINSTANCE)PluginInstance->DllBase,
                MAKEINTRESOURCE(IDD_ROTVIEW),
                NULL,
                RotViewDlgProc
                );
        }
        break;
    }
}

static VOID EnumRunningObjectTable(
    __in HWND hwndDlg
    )
{
    IRunningObjectTable* table = NULL;
    IEnumMoniker* moniker = NULL;
    IMoniker* pmkObjectNames = NULL;
    IBindCtx* ctx = NULL;
    IMalloc* iMalloc = NULL;
    ULONG count = 0;

    CoGetMalloc(1, &iMalloc);

    // Query the running object table address
    if (SUCCEEDED(GetRunningObjectTable(0, &table)))
    {
        // Enum the objects registered
        if (SUCCEEDED(IRunningObjectTable_EnumRunning(table, &moniker)))
        {
            while (IEnumMoniker_Next(moniker, 1, &pmkObjectNames, &count) == S_OK)
            {
                if (SUCCEEDED(CreateBindCtx(0, &ctx)))
                {
                    OLECHAR* name = NULL;

                    // Query the object name
                    if (SUCCEEDED(IMoniker_GetDisplayName(pmkObjectNames, ctx, NULL, &name)))
                    {
                        // Set the items name column
                        PhAddListViewItem(hwndDlg, MAXINT, name, NULL);

                        // Free the object name
                        IMalloc_Free(iMalloc, name);
                    }

                    IBindCtx_Release(ctx);
                    ctx = NULL;
                }

                IEnumMoniker_Release(pmkObjectNames);
                pmkObjectNames = NULL;
            }

            IEnumMoniker_Release(moniker);
            moniker = NULL;
        }

        IRunningObjectTable_Release(table);
        table = NULL;
    }

    IMalloc_Release(iMalloc);
    iMalloc = NULL;
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
            ListViewWndHandle = GetDlgItem(hwndDlg, IDC_LIST1);

            PhCenterWindow(hwndDlg, PhMainWndHandle);

            PhSetListViewStyle(ListViewWndHandle, FALSE, TRUE);
            PhSetControlTheme(ListViewWndHandle, L"explorer");
            PhAddListViewColumn(ListViewWndHandle, 0, 0, 0, LVCFMT_LEFT, 420, L"Display Name");
            PhSetExtendedListView(ListViewWndHandle);
         
            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, ListViewWndHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_ROTREFRESH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
                       
            EnumRunningObjectTable(ListViewWndHandle);
        }
        break;    
    case WM_SIZE:
        PhLayoutManagerLayout(&LayoutManager);
        break;
    case WM_DESTROY:
        PhDeleteLayoutManager(&LayoutManager);
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_ROTREFRESH:
                {
                    ListView_DeleteAllItems(ListViewWndHandle);
                    EnumRunningObjectTable(ListViewWndHandle);
                }
                break;
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    }

    return FALSE;
}