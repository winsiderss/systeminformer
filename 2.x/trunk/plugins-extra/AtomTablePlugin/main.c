/*
 * NT Atom Table Plugin
 *
 * Copyright (C) 2013 dmex
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

#define CINTERFACE
#define COBJMACROS
#include "phdk.h"
#include "phappresource.h"
#include "resource.h"

#define ATOM_TABLE_MENUITEM 1000

VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );
VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );
INT_PTR CALLBACK MainWindowDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

static PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
static PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
static HWND ListViewWndHandle;
static PH_LAYOUT_MANAGER LayoutManager;
static PPH_PLUGIN PluginInstance;

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

            PluginInstance = PhRegisterPlugin(L"dmex.AtomTablePlugin", Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->Author = L"dmex";
            info->DisplayName = L"Atom Table Plugin";
            info->Description = L"Plugin for viewing the NT Atom Table via the Tools menu";
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
                static PH_SETTING_CREATE settings[] =
                {
                    { IntegerPairSettingType, L"AtomTableWindowPosition", L"350,350" },
                    { IntegerPairSettingType, L"AtomTableWindowSize", L"510,380" },
                    { StringSettingType, L"AtomTableListViewColumns", L"" }
                };

                PhAddSettings(settings, _countof(settings));
            }
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
    PhPluginAddMenuItem(PluginInstance, PH_MENU_ITEM_LOCATION_TOOLS, L"$", ATOM_TABLE_MENUITEM, L"Atom Table", NULL);
}

static VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = (PPH_PLUGIN_MENU_ITEM)Parameter;

    switch (menuItem->Id)
    {
    case ATOM_TABLE_MENUITEM:
        {
            DialogBox(
                (HINSTANCE)PluginInstance->DllBase,
                MAKEINTRESOURCE(IDD_ATOMDIALOG),
                NULL,
                MainWindowDlgProc
                );
        }
        break;
    }
}

static NTSTATUS PhEnumAtomTable(
    __out PATOM_TABLE_INFORMATION* AtomTable
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 0x1000;

    buffer = PhAllocate(bufferSize);
    status = NtQueryInformationAtom(
        RTL_ATOM_INVALID_ATOM,
        AtomTableInformation,
        buffer,
        bufferSize,
        &bufferSize // Not used...
        );

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *AtomTable = buffer;

    return status;
}

static NTSTATUS PhQueryAtomTableEntry(
    __in RTL_ATOM Atom,
    __out PATOM_BASIC_INFORMATION* AtomInfo
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 0x1000;

    buffer = PhAllocate(bufferSize);
    status = NtQueryInformationAtom(
        Atom,
        AtomBasicInformation,
        buffer,
        bufferSize,
        &bufferSize // Not used...
        );

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *AtomInfo = buffer;

    return status;
}

static VOID LoadAtomTable(
    VOID
    )
{
    ULONG i = 0;
    PATOM_TABLE_INFORMATION atomTable = NULL;

    if (!NT_SUCCESS(PhEnumAtomTable(&atomTable)))
        return;

    for (i = 0; i < atomTable->NumberOfAtoms; i++)
    {
        PATOM_BASIC_INFORMATION atomInfo = NULL;

        if (!NT_SUCCESS(PhQueryAtomTableEntry(atomTable->Atoms[i], &atomInfo)))
        {
            PhAddListViewItem(ListViewWndHandle, MAXINT, PhaFormatString(L"(Error) #%u", i)->Buffer, NULL);
            continue;
        }

        if ((atomInfo->Flags & RTL_ATOM_PINNED) == RTL_ATOM_PINNED)
        {
            INT index = PhAddListViewItem(
                ListViewWndHandle,
                MAXINT,
                PhaFormatString(L"#%u (Pinned)", atomInfo->Name)->Buffer,
                NULL
                );
            PhSetListViewSubItem(
                ListViewWndHandle,
                index,
                1,
                PhaFormatString(L"%u", atomInfo->UsageCount)->Buffer
                );
        }
        else
        {
            INT index = PhAddListViewItem(
                ListViewWndHandle,
                MAXINT,
                atomInfo->Name,
                NULL
                );
            PhSetListViewSubItem(
                ListViewWndHandle,
                index,
                1,
                PhaFormatString(L"%u", atomInfo->UsageCount)->Buffer
                );
        }

        PhFree(atomInfo);
    }

    PhFree(atomTable);
}

static PPH_STRING PhGetSelectedListViewItemText(
    __in HWND hWnd
    )
{
    INT index = PhFindListViewItemByFlags(
        hWnd,
        -1,
        LVNI_SELECTED
        );

    if (index != -1)
    {
        WCHAR textBuffer[MAX_PATH + 1];

        LVITEM item;
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 0;
        item.pszText = textBuffer;
        item.cchTextMax = MAX_PATH;

        if (ListView_GetItem(hWnd, &item))
            return PhCreateString(textBuffer);
    }

    return NULL;
}

static VOID ShowStatusMenu(
    __in HWND hwndDlg
    )
{
    HMENU menu;
    HMENU subMenu;
    ULONG id;
    POINT cursorPos = { 0, 0 };

    PPH_STRING cacheEntryName = PhGetSelectedListViewItemText(ListViewWndHandle);

    if (cacheEntryName)
    {
        GetCursorPos(&cursorPos);

        menu = LoadMenu(
            (HINSTANCE)PluginInstance->DllBase,
            MAKEINTRESOURCE(IDR_MAIN_MENU)
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
        case ID_ATOMMENU_REMOVE:
            {
                INT lvItemIndex = PhFindListViewItemByFlags(
                    ListViewWndHandle,
                    -1,
                    LVNI_SELECTED
                    );

                if (lvItemIndex != -1)
                {
                    if (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
                        hwndDlg,
                        L"remove",
                        cacheEntryName->Buffer,
                        NULL,
                        FALSE
                        ))
                    {
                        ULONG i = 0;
                        PATOM_TABLE_INFORMATION atomTable = NULL;

                        if (!NT_SUCCESS(PhEnumAtomTable(&atomTable)))
                            return;

                        for (i = 0; i < atomTable->NumberOfAtoms; i++)
                        {
                            PATOM_BASIC_INFORMATION atomInfo = NULL;

                            if (!NT_SUCCESS(PhQueryAtomTableEntry(atomTable->Atoms[i], &atomInfo)))
                                continue;

                            if (_wcsicmp(atomInfo->Name, cacheEntryName->Buffer))
                                continue;

                            do
                            {
                                if (!NT_SUCCESS(NtDeleteAtom(atomTable->Atoms[i])))
                                {
                                    break;
                                }

                                PhFree(atomInfo);
                                atomInfo = NULL;

                                if (!NT_SUCCESS(PhQueryAtomTableEntry(atomTable->Atoms[i], &atomInfo)))
                                    break;

                            } while (atomInfo->UsageCount >= 1);

                            ListView_DeleteItem(ListViewWndHandle, lvItemIndex);

                            if (atomInfo)
                            {
                                PhFree(atomInfo);
                            }
                        }

                        PhFree(atomTable);
                    }
                }
            }
            break;
        }

        PhDereferenceObject(cacheEntryName);
    }
}

INT_PTR CALLBACK MainWindowDlgProc(
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
            PhCenterWindow(hwndDlg, PhMainWndHandle);
            ListViewWndHandle = GetDlgItem(hwndDlg, IDC_ATOMLIST);

            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, ListViewWndHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            PhRegisterDialog(hwndDlg);
            PhLoadWindowPlacementFromSetting(L"AtomTableWindowPosition", L"AtomTableWindowSize", hwndDlg);

            PhSetListViewStyle(ListViewWndHandle, FALSE, TRUE);
            PhSetControlTheme(ListViewWndHandle, L"explorer");
            PhAddListViewColumn(ListViewWndHandle, 0, 0, 0, LVCFMT_LEFT, 370, L"Atom Name");
            PhAddListViewColumn(ListViewWndHandle, 1, 1, 1, LVCFMT_LEFT, 70, L"Ref Count");
            PhSetExtendedListView(ListViewWndHandle);
            PhLoadListViewColumnsFromSetting(L"AtomTableListViewColumns", ListViewWndHandle);

            LoadAtomTable();
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&LayoutManager);
        break;
    case WM_DESTROY:
        PhSaveWindowPlacementToSetting(L"AtomTableWindowPosition", L"AtomTableWindowSize", hwndDlg);
        PhSaveListViewColumnsToSetting(L"AtomTableListViewColumns", ListViewWndHandle);
        PhDeleteLayoutManager(&LayoutManager);
        PhUnregisterDialog(hwndDlg);
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
                    if (hdr->hwndFrom == ListViewWndHandle)
                        ShowStatusMenu(hwndDlg);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}