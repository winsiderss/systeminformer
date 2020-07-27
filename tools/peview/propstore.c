/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2018-2019 dmex
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

VOID PvpPeEnumerateFilePropStore(
    _In_ HWND ListViewHandle
    )
{
    HRESULT status;
    IPropertyStore *propstore;
    ULONG count;
    ULONG i;

    status = SHGetPropertyStoreFromParsingName(
        PvFileName->Buffer,
        NULL,
        GPS_DEFAULT | GPS_EXTRINSICPROPERTIES | GPS_VOLATILEPROPERTIES,
        &IID_IPropertyStore,
        &propstore
        );

    if (status == HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND))
    {
        status = SHGetPropertyStoreFromParsingName(
            PvFileName->Buffer,
            NULL,
            GPS_FASTPROPERTIESONLY,
            &IID_IPropertyStore,
            &propstore
            );
    }

    if (FAILED(status))
    {
        status = SHGetPropertyStoreFromParsingName(
            PvFileName->Buffer,
            NULL,
            GPS_DEFAULT, // required for Windows 7 (dmex)
            &IID_IPropertyStore,
            &propstore
            );
    }

    if (SUCCEEDED(status))
    {
        if (SUCCEEDED(IPropertyStore_GetCount(propstore, &count)))
        {
            for (i = 0; i < count; i++)
            {
                PROPERTYKEY propkey;

                if (SUCCEEDED(IPropertyStore_GetAt(propstore, i, &propkey)))
                {
                    INT lvItemIndex;
                    PROPVARIANT propKeyVariant = { 0 };
                    PWSTR propKeyName;
                    WCHAR number[PH_INT32_STR_LEN_1];

                    PhPrintUInt32(number, i + 1);
                    lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, number, NULL);

                    if (SUCCEEDED(PSGetNameFromPropertyKey(&propkey, &propKeyName)))
                    {
                        IPropertyDescription* propertyDescriptionPtr = NULL;

                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, propKeyName);

                        if (SUCCEEDED(PSGetPropertyDescriptionByName(propKeyName, &IID_IPropertyDescription, &propertyDescriptionPtr)))
                        {
                            PWSTR propertyLabel = NULL;

                            if (SUCCEEDED(IPropertyDescription_GetDisplayName(propertyDescriptionPtr, &propertyLabel)))
                            {
                                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, propertyLabel);
                                CoTaskMemFree(propertyLabel);
                            }

                            IPropertyDescription_Release(propertyDescriptionPtr);
                        }
       
                        CoTaskMemFree(propKeyName);
                    }
                    else
                    {
                        WCHAR propKeyString[PKEYSTR_MAX];

                        if (SUCCEEDED(PSStringFromPropertyKey(&propkey, propKeyString, RTL_NUMBER_OF(propKeyString))))
                            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, propKeyString);
                        else
                            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, L"Unknown");
                    }

                    if (SUCCEEDED(IPropertyStore_GetValue(propstore, &propkey, &propKeyVariant)))
                    {
                        if (SUCCEEDED(PSFormatForDisplayAlloc(&propkey, &propKeyVariant, PDFF_DEFAULT, &propKeyName)))
                        {
                            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, propKeyName);
                            CoTaskMemFree(propKeyName);
                        }

                        PropVariantClear(&propKeyVariant);
                    }
                }
            }
        }

        IPropertyStore_Release(propstore);
    }
}

INT_PTR CALLBACK PvpPePropStoreDlgProc(
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
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 150, L"Name");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 250, L"Value");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 150, L"Description");
            PhSetExtendedListView(lvHandle);
            PhLoadListViewColumnsFromSetting(L"ImagePropertiesListViewColumns", lvHandle);

            PvpPeEnumerateFilePropStore(lvHandle);
            //ExtendedListView_SortItems(lvHandle);
            
            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImagePropertiesListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));
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
