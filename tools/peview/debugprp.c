/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2020 dmex
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

PWSTR PvpGetDebugTypeString(
    _In_ ULONG Type
    )
{
    switch (Type)
    {
    case IMAGE_DEBUG_TYPE_COFF:
        return L"COFF";
    case IMAGE_DEBUG_TYPE_CODEVIEW:
        return L"CODEVIEW";
    case IMAGE_DEBUG_TYPE_FPO:
        return L"FPO";
    case IMAGE_DEBUG_TYPE_MISC:
        return L"MISC";
    case IMAGE_DEBUG_TYPE_EXCEPTION:
        return L"EXCEPTION";
    case IMAGE_DEBUG_TYPE_FIXUP:
        return L"FIXUP";
    case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:
        return L"OMAP_TO_SRC";
    case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC:
        return L"OMAP_FROM_SRC";
    case IMAGE_DEBUG_TYPE_BORLAND:
        return L"BORLAND";
    case IMAGE_DEBUG_TYPE_RESERVED10:
        return L"RESERVED10";
    case IMAGE_DEBUG_TYPE_CLSID:
        return L"CLSID";
    case IMAGE_DEBUG_TYPE_VC_FEATURE:
        return L"VC_FEATURE";
    case IMAGE_DEBUG_TYPE_POGO:
        return L"POGO";
    case IMAGE_DEBUG_TYPE_ILTCG:
        return L"ILTCG";
    case IMAGE_DEBUG_TYPE_MPX:
        return L"MPX";
    case IMAGE_DEBUG_TYPE_REPRO:
        return L"REPRO";
    case IMAGE_DEBUG_TYPE_EMBEDDEDPORTABLEPDB:
        return L"EMBEDDED_PDB";
    // Note: missing 18.
    case IMAGE_DEBUG_TYPE_PDBCHECKSUM:
        return L"PDB_CHECKSUM";
    case IMAGE_DEBUG_TYPE_EX_DLLCHARACTERISTICS:
        return L"EX_DLLCHARACTERISTICS";
    }

    return PhaFormatString(L"%lu", Type)->Buffer;
}

INT_PTR CALLBACK PvpPeDebugDlgProc(
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
            PH_MAPPED_IMAGE_DEBUG debug;
            PH_IMAGE_DEBUG_ENTRY entry;
            HWND lvHandle;
            ULONG count = 0;
            ULONG i;
            INT lvItemIndex;

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Type");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"RVA");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"Size");
            PhSetExtendedListView(lvHandle);
            PhLoadListViewColumnsFromSetting(L"ImageDebugListViewColumns", lvHandle);

            if (NT_SUCCESS(PhGetMappedImageDebug(&PvMappedImage, &debug)))
            {
                for (i = 0; i < debug.NumberOfEntries; i++)
                {
                    WCHAR value[PH_INT64_STR_LEN_1];

                    entry = debug.DebugEntries[i];

                    PhPrintUInt32(value, ++count);
                    lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, value, NULL);

                    PhSetListViewSubItem(lvHandle, lvItemIndex, 1, PvpGetDebugTypeString(entry.Type));

                    PhPrintPointer(value, UlongToPtr(entry.AddressOfRawData));
                    PhSetListViewSubItem(lvHandle, lvItemIndex, 2, value);

                    PhSetListViewSubItem(lvHandle, lvItemIndex, 3, PhaFormatSize(entry.SizeOfData, ULONG_MAX)->Buffer);
                }

                PhFree(debug.DebugEntries);
            }

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageDebugListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));
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
