/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2018 dmex
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

typedef struct _PV_EA_CALLBACK
{
    HWND ListViewHandle;
    ULONG Count;
} PV_EA_CALLBACK, *PPV_EA_CALLBACK;

BOOLEAN NTAPI PvpEnumFileAttributesCallback(
    _In_ PFILE_FULL_EA_INFORMATION Information,
    _In_opt_ PVOID Context
    )
{
    PPV_EA_CALLBACK context = Context;
    PPH_STRING attributeName;
    INT lvItemIndex;
    WCHAR number[PH_INT32_STR_LEN_1];

    if (Information->EaNameLength == 0)
        return TRUE;

    PhPrintUInt32(number, ++context->Count);
    lvItemIndex = PhAddListViewItem(context->ListViewHandle, MAXINT, number, NULL);

    attributeName = PhZeroExtendToUtf16Ex(Information->EaName, Information->EaNameLength);
    PhSetListViewSubItem(
        context->ListViewHandle,
        lvItemIndex,
        1,
        attributeName->Buffer
        );
    PhDereferenceObject(attributeName);

    PhSetListViewSubItem(
        context->ListViewHandle,
        lvItemIndex,
        2,
        PhaFormatSize(Information->EaValueLength, ULONG_MAX)->Buffer
        );

    return TRUE;
}

INT_PTR CALLBACK PvpPeExtendedAttributesDlgProc(
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
            HANDLE fileHandle;
            HWND lvHandle;

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 150, L"Name");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 200, L"Value");
            PhSetExtendedListView(lvHandle);
            PhLoadListViewColumnsFromSetting(L"ImageAttributesListViewColumns", lvHandle);

            if (NT_SUCCESS(PhCreateFileWin32(
                &fileHandle,
                PhGetString(PvFileName),
                FILE_READ_ATTRIBUTES | FILE_READ_DATA | FILE_READ_EA | SYNCHRONIZE,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                )))
            {
                PV_EA_CALLBACK context;

                context.ListViewHandle = lvHandle;
                context.Count = 0;

                PhEnumFileExtendedAttributes(fileHandle, PvpEnumFileAttributesCallback, &context);

                NtClose(fileHandle);
            }

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageAttributesListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));
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
