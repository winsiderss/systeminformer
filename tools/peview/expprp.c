/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2010-2011 wj32
 * Copyright (C) 2017 dmex
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

INT_PTR CALLBACK PvpPeExportsDlgProc(
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
            PH_MAPPED_IMAGE_EXPORTS exports;
            PH_MAPPED_IMAGE_EXPORT_ENTRY exportEntry;
            PH_MAPPED_IMAGE_EXPORT_FUNCTION exportFunction;
            ULONG i;

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_RIGHT, 80, L"RVA");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 250, L"Name");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 50, L"Ordinal");
            PhSetExtendedListView(lvHandle);
            PhLoadListViewColumnsFromSetting(L"ImageExportsListViewColumns", lvHandle);

            if (NT_SUCCESS(PhGetMappedImageExports(&exports, &PvMappedImage)))
            {
                for (i = 0; i < exports.NumberOfEntries; i++)
                {
                    if (
                        NT_SUCCESS(PhGetMappedImageExportEntry(&exports, i, &exportEntry)) &&
                        NT_SUCCESS(PhGetMappedImageExportFunction(&exports, NULL, exportEntry.Ordinal, &exportFunction))
                        )
                    {
                        INT lvItemIndex;
                        WCHAR number[PH_INT32_STR_LEN_1];
                        WCHAR pointer[PH_PTR_STR_LEN_1];

                        PhPrintUInt64(number, i + 1);
                        lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, number, NULL);

                        if (exportFunction.ForwardedName)
                        {
                            PPH_STRING forwardName = NULL;

                            if (exportFunction.ForwardedName[0] == '?')
                                forwardName = PhUndecorateName(PvSymbolProvider->ProcessHandle, exportFunction.ForwardedName);
                            else
                                forwardName = PhZeroExtendToUtf16(exportFunction.ForwardedName);

                            if (!forwardName)
                                forwardName = PhZeroExtendToUtf16(exportFunction.ForwardedName);

                            PhSetListViewSubItem(lvHandle, lvItemIndex, 1, forwardName->Buffer);
                            PhDereferenceObject(forwardName);
                        }
                        else
                        {
                            PhPrintPointer(pointer, exportFunction.Function);
                            PhSetListViewSubItem(lvHandle, lvItemIndex, 1, pointer);
                        }

                        if (exportEntry.Name)
                        {
                            PPH_STRING exportName = NULL;

                            if (exportEntry.Name[0] == '?')
                                exportName = PhUndecorateName(PvSymbolProvider->ProcessHandle, exportEntry.Name);
                            else
                                exportName = PhZeroExtendToUtf16(exportEntry.Name);

                            if (!exportName)
                                exportName = PhZeroExtendToUtf16(exportEntry.Name);

                            PhSetListViewSubItem(lvHandle, lvItemIndex, 2, exportName->Buffer);
                            PhDereferenceObject(exportName);
                        }
                        else
                        {
                            PhSetListViewSubItem(lvHandle, lvItemIndex, 2, L"(unnamed)");
                        }

                        PhPrintUInt32(number, exportEntry.Ordinal);
                        PhSetListViewSubItem(lvHandle, lvItemIndex, 3, number);
                    }
                }
            }

            ExtendedListView_SortItems(lvHandle);
            
            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageExportsListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_LIST),
                    dialogItem, PH_ANCHOR_ALL);

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
    }

    return FALSE;
}
