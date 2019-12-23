/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2019 dmex
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

#include <peview.h>

VOID PvpPeEnumerateHeaderDirectory(
    _In_ HWND ListViewHandle,
    _In_ ULONG Index,
    _In_ PWSTR Name
    )
{
    INT lvItemIndex;
    PIMAGE_DATA_DIRECTORY directory;
    WCHAR value[PH_INT64_STR_LEN_1];

    PhPrintUInt32(value, Index);
    lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, value, NULL);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, Name);

    if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, Index, &directory)))
    {
        if (directory->VirtualAddress)
        {
            PhPrintPointer(value, UlongToPtr(directory->VirtualAddress));
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, value);
        }

        if (directory->Size)
        {
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, PhaFormatSize(directory->Size, ULONG_MAX)->Buffer);
        }
    }
}

INT_PTR CALLBACK PvpPeDirectoryDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext))
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND lvHandle;

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 130, L"Name");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"VA");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"Size");
            PhSetExtendedListView(lvHandle);
            PhLoadListViewColumnsFromSetting(L"ImageDirectoryListViewColumns", lvHandle);

            // for (ULONG i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; i++) // TODO (dmex)
            PvpPeEnumerateHeaderDirectory(lvHandle, IMAGE_DIRECTORY_ENTRY_EXPORT, L"Export");
            PvpPeEnumerateHeaderDirectory(lvHandle, IMAGE_DIRECTORY_ENTRY_IMPORT, L"Import");
            PvpPeEnumerateHeaderDirectory(lvHandle, IMAGE_DIRECTORY_ENTRY_RESOURCE, L"Resource");
            PvpPeEnumerateHeaderDirectory(lvHandle, IMAGE_DIRECTORY_ENTRY_EXCEPTION, L"Exception");
            PvpPeEnumerateHeaderDirectory(lvHandle, IMAGE_DIRECTORY_ENTRY_SECURITY, L"Security");
            PvpPeEnumerateHeaderDirectory(lvHandle, IMAGE_DIRECTORY_ENTRY_BASERELOC, L"Base relocation");
            PvpPeEnumerateHeaderDirectory(lvHandle, IMAGE_DIRECTORY_ENTRY_DEBUG, L"Debug");
            PvpPeEnumerateHeaderDirectory(lvHandle, IMAGE_DIRECTORY_ENTRY_ARCHITECTURE, L"Architecture");
            PvpPeEnumerateHeaderDirectory(lvHandle, IMAGE_DIRECTORY_ENTRY_GLOBALPTR, L"Global PTR");
            PvpPeEnumerateHeaderDirectory(lvHandle, IMAGE_DIRECTORY_ENTRY_TLS, L"TLS");
            PvpPeEnumerateHeaderDirectory(lvHandle, IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, L"Load configuration");
            PvpPeEnumerateHeaderDirectory(lvHandle, IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT, L"Bound imports");
            PvpPeEnumerateHeaderDirectory(lvHandle, IMAGE_DIRECTORY_ENTRY_IAT, L"IAT");
            PvpPeEnumerateHeaderDirectory(lvHandle, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT, L"Delay load imports");
            PvpPeEnumerateHeaderDirectory(lvHandle, IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR, L"CLR");

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageDirectoryListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_LIST), dialogItem, PH_ANCHOR_ALL);

                PvDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PvHandleListViewNotifyForCopy(lParam, GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    case WM_CONTEXTMENU:
        {
            PvHandleListViewCommandCopy(hwndDlg, lParam, wParam, GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    }

    return FALSE;
}
