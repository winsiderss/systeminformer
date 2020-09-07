/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2010-2011 wj32
 * Copyright (C) 2017-2019 dmex
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
            PhAddListViewColumn(lvHandle, 4, 4, 4, LVCFMT_LEFT, 50, L"Hint");
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
                        WCHAR value[PH_PTR_STR_LEN_1];

                        PhPrintUInt32(value, i + 1);
                        lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, value, NULL);

                        if (exportFunction.ForwardedName)
                        {
                            PPH_STRING forwardName;

                            forwardName = PhZeroExtendToUtf16(exportFunction.ForwardedName);

                            if (forwardName->Buffer[0] == '?')
                            {
                                PPH_STRING undecoratedName;

                                if (undecoratedName = PhUndecorateSymbolName(PvSymbolProvider, forwardName->Buffer))
                                    PhMoveReference(&forwardName, undecoratedName);
                            }

                            PhSetListViewSubItem(lvHandle, lvItemIndex, 1, forwardName->Buffer);
                            PhDereferenceObject(forwardName);
                        }
                        else
                        {
                            PhPrintPointer(value, exportFunction.Function);
                            PhSetListViewSubItem(lvHandle, lvItemIndex, 1, value);
                        }

                        if (exportEntry.Name)
                        {
                            PPH_STRING exportName;

                            exportName = PhZeroExtendToUtf16(exportEntry.Name);

                            if (exportName->Buffer[0] == '?')
                            {
                                PPH_STRING undecoratedName;

                                if (undecoratedName = PhUndecorateSymbolName(PvSymbolProvider, exportName->Buffer))
                                    PhMoveReference(&exportName, undecoratedName);
                            }

                            PhSetListViewSubItem(lvHandle, lvItemIndex, 2, exportName->Buffer);
                            PhDereferenceObject(exportName);
                        }
                        else
                        {
                            if (exportFunction.Function)
                            {
                                PPH_STRING exportSymbol = NULL;
                                PPH_STRING exportSymbolName = NULL;

                                // Try find the export name using symbols.
                                if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
                                {
                                    exportSymbol = PhGetSymbolFromAddress(
                                        PvSymbolProvider,
                                        (ULONG64)PTR_ADD_OFFSET(PvMappedImage.NtHeaders32->OptionalHeader.ImageBase, exportFunction.Function),
                                        NULL,
                                        NULL,
                                        &exportSymbolName,
                                        NULL
                                        );
                                }
                                else
                                {
                                    exportSymbol = PhGetSymbolFromAddress(
                                        PvSymbolProvider,
                                        (ULONG64)PTR_ADD_OFFSET(PvMappedImage.NtHeaders->OptionalHeader.ImageBase, exportFunction.Function),
                                        NULL,
                                        NULL,
                                        &exportSymbolName,
                                        NULL
                                        );
                                }

                                if (exportSymbolName)
                                {
                                    PhSetListViewSubItem(lvHandle, lvItemIndex, 2, PH_AUTO_T(PH_STRING, PhConcatStringRefZ(&exportSymbolName->sr, L" (unnamed)"))->Buffer);
                                    PhDereferenceObject(exportSymbolName);
                                }
                                else
                                {
                                    PhSetListViewSubItem(lvHandle, lvItemIndex, 2, L"(unnamed)");
                                }

                                if (exportSymbol)
                                    PhDereferenceObject(exportSymbol);
                            }
                            else
                            {
                                PhSetListViewSubItem(lvHandle, lvItemIndex, 2, L"(unnamed)");
                            }
                        }

                        PhPrintUInt32(value, exportEntry.Ordinal);
                        PhSetListViewSubItem(lvHandle, lvItemIndex, 3, value);

                        if (exportEntry.Name) // Note: The 'Hint' is only valid for named exports. (dmex)
                        {
                            PhPrintUInt32(value, exportEntry.Hint);
                            PhSetListViewSubItem(lvHandle, lvItemIndex, 4, value);
                        }
                    }
                }
            }

            ExtendedListView_SortItems(lvHandle);
            
            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
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
