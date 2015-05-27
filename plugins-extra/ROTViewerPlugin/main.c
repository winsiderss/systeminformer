/*
 * Process Hacker Extra Plugins -
 *   Running Object Table Plugin
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

static PPH_PLUGIN PluginInstance;
static PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
static PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;

static NTSTATUS EnumRunningObjectTable(
    _In_ PVOID ThreadParam
    )
{
    IRunningObjectTable* iRunningObjectTable = NULL;
    IEnumMoniker* iEnumMoniker = NULL;
    IMoniker* iMoniker = NULL;
    IBindCtx* iBindCtx = NULL;
    IMalloc* iMalloc = NULL;
    ULONG count = 0;

    HWND listViewHandle = (HWND)ThreadParam;

    if (!SUCCEEDED(CoGetMalloc(1, &iMalloc)))
        return STATUS_INSUFFICIENT_RESOURCES;

    // Query the running object table address
    if (SUCCEEDED(GetRunningObjectTable(0, &iRunningObjectTable)))
    {
        // Enum the objects registered
        if (SUCCEEDED(IRunningObjectTable_EnumRunning(iRunningObjectTable, &iEnumMoniker)))
        {
            while (IEnumMoniker_Next(iEnumMoniker, 1, &iMoniker, &count) == S_OK)
            {
                if (SUCCEEDED(CreateBindCtx(0, &iBindCtx)))
                {
                    OLECHAR* displayName = NULL;

                    // Query the object name
                    if (SUCCEEDED(IMoniker_GetDisplayName(iMoniker, iBindCtx, NULL, &displayName)))
                    {
                        // Set the items name column
                        PhAddListViewItem(listViewHandle, MAXINT, displayName, NULL);

                        // Free the object name
                        IMalloc_Free(iMalloc, displayName);
                    }

                    IBindCtx_Release(iBindCtx);
                }

                IEnumMoniker_Release(iMoniker);
            }

            IEnumMoniker_Release(iEnumMoniker);
        }

        IRunningObjectTable_Release(iRunningObjectTable);
    }

    IMalloc_Release(iMalloc);

    return STATUS_SUCCESS;
}

static INT_PTR CALLBACK RotViewDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PROT_WINDOW_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PROT_WINDOW_CONTEXT)PhAllocate(sizeof(ROT_WINDOW_CONTEXT));
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PROT_WINDOW_CONTEXT)GetProp(hwndDlg, L"Context");

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

            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST1);

            PhRegisterDialog(hwndDlg);
            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 420, L"Display Name");
            PhSetExtendedListView(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_ROTREFRESH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhLoadWindowPlacementFromSetting(SETTING_NAME_WINDOW_POSITION, SETTING_NAME_WINDOW_SIZE, hwndDlg);

            if (threadHandle = PhCreateThread(0, EnumRunningObjectTable, context->ListViewHandle))
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
            case IDC_ROTREFRESH:
                {
                    ListView_DeleteAllItems(context->ListViewHandle);

                    HANDLE threadHandle;

                    if (threadHandle = PhCreateThread(0, EnumRunningObjectTable, context->ListViewHandle))
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
    }

    return FALSE;
}

static VOID NTAPI MenuItemCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = (PPH_PLUGIN_MENU_ITEM)Parameter;

    switch (menuItem->Id)
    {
        case ROT_TABLE_MENUITEM:
        {
            DialogBox(
                PluginInstance->DllBase,
                MAKEINTRESOURCE(IDD_ROTVIEW),
                NULL,
                RotViewDlgProc
                );
        }
            break;
    }
}

static VOID NTAPI MainWindowShowingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PhPluginAddMenuItem(PluginInstance, PH_MENU_ITEM_LOCATION_TOOLS, L"$", ROT_TABLE_MENUITEM, L"Running Object Table", NULL);
}

LOGICAL DllMain(
    _In_ HINSTANCE Instance,
    _In_ ULONG Reason,
    _Reserved_ PVOID Reserved
    )
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            PPH_PLUGIN_INFORMATION info;

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Running Object Table";
            info->Author = L"dmex";
            info->Description = L"Plugin for viewing the Running Object Table via the Tools menu.";
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
            {
                PH_SETTING_CREATE settings[] =
                {
                    { IntegerPairSettingType, SETTING_NAME_WINDOW_POSITION, L"100,100" },
                    { IntegerPairSettingType, SETTING_NAME_WINDOW_SIZE, L"490,340" }
                };

                PhAddSettings(settings, _countof(settings));
            }
        }
        break;
    }

    return TRUE;
}