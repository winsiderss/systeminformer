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

VOID PvpProcessImports(
    _In_ HWND ListViewHandle,
    _In_ PPH_MAPPED_IMAGE_IMPORTS Imports,
    _In_ BOOLEAN DelayImports,
    _Inout_ ULONG *Count
    )
{
    PH_MAPPED_IMAGE_IMPORT_DLL importDll;
    PH_MAPPED_IMAGE_IMPORT_ENTRY importEntry;
    ULONG i;
    ULONG j;

    for (i = 0; i < Imports->NumberOfDlls; i++)
    {
        if (NT_SUCCESS(PhGetMappedImageImportDll(Imports, i, &importDll)))
        {
            for (j = 0; j < importDll.NumberOfEntries; j++)
            {
                if (NT_SUCCESS(PhGetMappedImageImportEntry(&importDll, j, &importEntry)))
                {
                    INT lvItemIndex;
                    PPH_STRING name;
                    WCHAR number[PH_INT32_STR_LEN_1];

                    if (DelayImports)
                        name = PhFormatString(L"%S (Delay)", importDll.Name);
                    else
                        name = PhZeroExtendToUtf16(importDll.Name);

                    PhPrintUInt64(number, ++(*Count)); // HACK
                    lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, number, NULL);

                    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, name->Buffer);
                    PhDereferenceObject(name);

                    if (importEntry.Name)
                    {
                        PPH_STRING importName = NULL;

                        if (importEntry.Name[0] == '?')
                            importName = PhUndecorateName(PvSymbolProvider->ProcessHandle, importEntry.Name);
                        else
                            importName = PhZeroExtendToUtf16(importEntry.Name);

                        if (!importName)
                            importName = PhZeroExtendToUtf16(importEntry.Name);

                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, importName->Buffer);
                        PhDereferenceObject(importName);

                        PhPrintUInt32(number, importEntry.NameHint);
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, number);
                    }
                    else
                    {
                        name = PhFormatString(L"(Ordinal %u)", importEntry.Ordinal);
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, name->Buffer);
                        PhDereferenceObject(name);
                    }
                }
            }
        }
    }
}

INT_PTR CALLBACK PvpPeImportsDlgProc(
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
            ULONG count = 0;
            ULONG fallbackColumns[] = { 0, 1, 2 };
            HWND lvHandle;
            PH_MAPPED_IMAGE_IMPORTS imports;

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 130, L"DLL");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 210, L"Name");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 50, L"Hint");
            PhSetExtendedListView(lvHandle);
            ExtendedListView_AddFallbackColumns(lvHandle, 3, fallbackColumns);
            PhLoadListViewColumnsFromSetting(L"ImageImportsListViewColumns", lvHandle);

            if (NT_SUCCESS(PhGetMappedImageImports(&imports, &PvMappedImage)))
            {
                PvpProcessImports(lvHandle, &imports, FALSE, &count);
            }

            if (NT_SUCCESS(PhGetMappedImageDelayImports(&imports, &PvMappedImage)))
            {
                PvpProcessImports(lvHandle, &imports, TRUE, &count);
            }

            ExtendedListView_SortItems(lvHandle);

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageImportsListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));
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
