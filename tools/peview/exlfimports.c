/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017
 *
 */

#include <peview.h>

VOID PvpProcessElfImports(
    _In_ HWND ListViewHandle
    )
{
    PPH_LIST imports;
    ULONG count = 0;

    PhGetMappedWslImageSymbols(&PvMappedImage, &imports);

    for (ULONG i = 0; i < imports->Count; i++)
    {
        PPH_ELF_IMAGE_SYMBOL_ENTRY import = imports->Items[i];
        INT lvItemIndex;
        WCHAR number[PH_INT64_STR_LEN_1];

        if (!import->ImportSymbol)
            continue;

        PhPrintUInt64(number, ++count);
        lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, number, NULL);

        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, import->Module);
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, import->Name);
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, PvpGetSymbolTypeName(import->TypeInfo));
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 4, PvpGetSymbolBindingName(import->TypeInfo));
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 5, PvpGetSymbolVisibility(import->OtherInfo));
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 6, PvpGetSymbolSectionName(import->SectionIndex)->Buffer);
    }

    PhFreeMappedWslImageSymbols(imports);
}

INT_PTR CALLBACK PvpExlfImportsDlgProc(
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
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 130, L"Module");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 210, L"Name");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"Type");
            PhAddListViewColumn(lvHandle, 4, 4, 4, LVCFMT_LEFT, 80, L"Binding");
            PhAddListViewColumn(lvHandle, 5, 5, 5, LVCFMT_LEFT, 80, L"Visibility");
            PhAddListViewColumn(lvHandle, 6, 6, 6, LVCFMT_LEFT, 80, L"Section");
            PhSetExtendedListView(lvHandle);
            PhLoadListViewColumnsFromSetting(L"ImportsWslListViewColumns", lvHandle);

            PvpProcessElfImports(lvHandle);
            ExtendedListView_SortItems(lvHandle);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImportsWslListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));
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
