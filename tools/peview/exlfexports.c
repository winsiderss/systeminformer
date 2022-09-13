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

VOID PvpProcessElfExports(
    _In_ HWND ListViewHandle
    )
{
    PPH_LIST exports;
    ULONG count = 0;

    PhGetMappedWslImageSymbols(&PvMappedImage, &exports);

    for (ULONG i = 0; i < exports->Count; i++)
    {
        PPH_ELF_IMAGE_SYMBOL_ENTRY export = exports->Items[i];
        INT lvItemIndex;
        WCHAR value[PH_PTR_STR_LEN_1];

        if (!export->ExportSymbol)
            continue;

        PhPrintUInt32(value, ++count);
        lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, value, NULL);

        PhPrintPointer(value, (PVOID)export->Address);
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, value);
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, export->Name);
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, PhaFormatSize(export->Size, ULONG_MAX)->Buffer);
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 4, PvpGetSymbolTypeName(export->TypeInfo));
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 5, PvpGetSymbolBindingName(export->TypeInfo));
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 6, PvpGetSymbolVisibility(export->OtherInfo));
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 7, PvpGetSymbolSectionName(export->SectionIndex)->Buffer);
    }

    PhFreeMappedWslImageSymbols(exports);
}

INT_PTR CALLBACK PvpExlfExportsDlgProc(
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
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_RIGHT, 80, L"RVA");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 250, L"Name");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 80, L"Size");
            PhAddListViewColumn(lvHandle, 4, 4, 4, LVCFMT_LEFT, 80, L"Type");
            PhAddListViewColumn(lvHandle, 5, 5, 5, LVCFMT_LEFT, 80, L"Binding");
            PhAddListViewColumn(lvHandle, 6, 6, 6, LVCFMT_LEFT, 80, L"Visibility");
            PhAddListViewColumn(lvHandle, 7, 7, 7, LVCFMT_LEFT, 80, L"Section");
            PhSetExtendedListView(lvHandle);
            PhLoadListViewColumnsFromSetting(L"ExportsWslListViewColumns", lvHandle);

            PvpProcessElfExports(lvHandle);
            ExtendedListView_SortItems(lvHandle);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ExportsWslListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));
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
