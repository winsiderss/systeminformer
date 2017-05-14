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

INT_PTR CALLBACK PvpPeCgfDlgProc(
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
            PH_MAPPED_IMAGE_CFG cfgConfig = { 0 };

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");

            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_RIGHT, 80, L"VA");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 250, L"Name");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"Flags");
            PhSetExtendedListView(lvHandle);
            PhLoadListViewColumnsFromSetting(L"ImageCfgListViewColumns", lvHandle);

            // Retrieve Cfg Table entry and characteristics
            if (NT_SUCCESS(PhGetMappedImageCfg(&cfgConfig, &PvMappedImage)))
            {
                for (ULONGLONG i = 0; i < cfgConfig.NumberOfGuardFunctionEntries; i++)
                {
                    INT lvItemIndex;
                    ULONG64 displacement;
                    PPH_STRING symbol;
                    PPH_STRING symbolName = NULL;
                    PH_SYMBOL_RESOLVE_LEVEL symbolResolveLevel = PhsrlInvalid;
                    IMAGE_CFG_ENTRY cfgFunctionEntry = { 0 };
                    WCHAR number[PH_INT64_STR_LEN_1];
                    WCHAR pointer[PH_PTR_STR_LEN_1];

                    // Parse cfg entry : if it fails, just skip it ?
                    if (!NT_SUCCESS(PhGetMappedImageCfgEntry(&cfgConfig, i, ControlFlowGuardFunction, &cfgFunctionEntry)))
                        continue;

                    PhPrintUInt64(number, i + 1);
                    lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, number, NULL);

                    PhPrintPointer(pointer, UlongToPtr(cfgFunctionEntry.Rva));
                    PhSetListViewSubItem(lvHandle, lvItemIndex, 1, pointer);

                    // Resolve name based on public symbols
                    if (!(symbol = PhGetSymbolFromAddress(
                        PvSymbolProvider,
                        (ULONG64)PTR_ADD_OFFSET(PvMappedImage.NtHeaders->OptionalHeader.ImageBase, cfgFunctionEntry.Rva),
                        &symbolResolveLevel,
                        NULL,
                        &symbolName,
                        &displacement
                        )))
                    {
                        continue;
                    }

                    switch (symbolResolveLevel)
                    {
                    case PhsrlFunction:
                        {
                            if (displacement)
                            {
                                PhSetListViewSubItem(
                                    lvHandle, 
                                    lvItemIndex, 
                                    2,
                                    PhaFormatString(L"%s+0x%x", symbolName->Buffer, displacement)->Buffer
                                    );
                            }
                            else
                            {
                                PhSetListViewSubItem(lvHandle, lvItemIndex, 2, symbolName->Buffer);
                            }
                        }
                        break;
                    case PhsrlModule:
                    case PhsrlAddress: 
                        {
                            PhSetListViewSubItem(lvHandle, lvItemIndex, 2, symbol->Buffer);
                        }
                        break;
                    default:
                    case PhsrlInvalid:
                        {
                            PhSetListViewSubItem(lvHandle, lvItemIndex, 2, L"(unnamed)");
                        }
                        break;
                    }

                    // Add additional flags
                    if (cfgFunctionEntry.SuppressedCall)
                        PhSetListViewSubItem(lvHandle, lvItemIndex, 3, L"SuppressedCall");
                    else
                        PhSetListViewSubItem(lvHandle, lvItemIndex, 3, L"");

                    if (symbolName)
                        PhDereferenceObject(symbolName);
                    PhDereferenceObject(symbol);
                }
            }

            ExtendedListView_SortItems(lvHandle);
            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageCfgListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));
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
