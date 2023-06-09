/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2018-2022
 *
 */

#include <peview.h>

typedef struct _PVP_PE_PROPSTORE_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PVP_PE_PROPSTORE_CONTEXT, *PPVP_PE_PROPSTORE_CONTEXT;

VOID PvpPeEnumerateFilePropStore(
    _In_ HWND ListViewHandle
    )
{
    HRESULT status;
    IPropertyStore *propstore;
    ULONG count;
    ULONG i;

    ExtendedListView_SetRedraw(ListViewHandle, FALSE);
    ListView_DeleteAllItems(ListViewHandle);

    status = SHGetPropertyStoreFromParsingName(
        PhGetString(PvFileName),
        NULL,
        GPS_DEFAULT | GPS_EXTRINSICPROPERTIES | GPS_VOLATILEPROPERTIES,
        &IID_IPropertyStore,
        &propstore
        );

    if (status == CO_E_NOTINITIALIZED)
    {
        CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

        status = SHGetPropertyStoreFromParsingName(
            PhGetString(PvFileName),
            NULL,
            GPS_DEFAULT | GPS_EXTRINSICPROPERTIES | GPS_VOLATILEPROPERTIES,
            &IID_IPropertyStore,
            &propstore
            );
    }

    if (status == HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND))
    {
        status = SHGetPropertyStoreFromParsingName(
            PhGetString(PvFileName),
            NULL,
            GPS_FASTPROPERTIESONLY,
            &IID_IPropertyStore,
            &propstore
            );
    }

    if (FAILED(status))
    {
        status = SHGetPropertyStoreFromParsingName(
            PhGetString(PvFileName),
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
                    IPropertyDescription* propertyDescriptionPtr = NULL;
                    PROPVARIANT propKeyVariant;
                    PWSTR propKeyName;
                    WCHAR number[PH_INT32_STR_LEN_1];

                    PhPrintUInt32(number, i + 1);
                    lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, number, NULL);

                    if (SUCCEEDED(PSGetNameFromPropertyKey(&propkey, &propKeyName)))
                    {
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, propKeyName);
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

                    PropVariantInit(&propKeyVariant);

                    if (SUCCEEDED(IPropertyStore_GetValue(propstore, &propkey, &propKeyVariant)))
                    {
                        if (SUCCEEDED(PSFormatForDisplayAlloc(&propkey, &propKeyVariant, PDFF_DEFAULT, &propKeyName)))
                        {
                            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, propKeyName);
                            CoTaskMemFree(propKeyName);
                        }

                        PropVariantClear(&propKeyVariant);
                    }

                    if (SUCCEEDED(PSGetPropertyDescription(&propkey, &IID_IPropertyDescription, &propertyDescriptionPtr)))
                    {
                        PWSTR propertyLabel = NULL;

                        if (SUCCEEDED(IPropertyDescription_GetDisplayName(propertyDescriptionPtr, &propertyLabel)))
                        {
                            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, propertyLabel);
                            CoTaskMemFree(propertyLabel);
                        }

                        IPropertyDescription_Release(propertyDescriptionPtr);
                    }
                }
            }
        }

        IPropertyStore_Release(propstore);
    }

    //ExtendedListView_SortItems(ListViewHandle);
    ExtendedListView_SetRedraw(ListViewHandle, TRUE);
}

INT_PTR CALLBACK PvpPePropStoreDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_PROPSTORE_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_PROPSTORE_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);

        if (lParam)
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            context->PropSheetContext = (PPV_PROPPAGECONTEXT)propSheetPage->lParam;
        }
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 150, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 250, L"Value");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 150, L"Description");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImagePropertiesListViewColumns", context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PvpPeEnumerateFilePropStore(context->ListViewHandle);
            //ExtendedListView_SortItems(context->ListViewHandle);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImagePropertiesListViewColumns", context->ListViewHandle);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (context->PropSheetContext && !context->PropSheetContext->LayoutInitialized)
            {
                PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                context->PropSheetContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_NOTIFY:
        {
            PvHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    case WM_CONTEXTMENU:
        {
            PvHandleListViewCommandCopy(hwndDlg, lParam, wParam, context->ListViewHandle);
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetTextColor((HDC)wParam, RGB(0, 0, 0));
            SetDCBrushColor((HDC)wParam, RGB(255, 255, 255));
            return (INT_PTR)GetStockBrush(DC_BRUSH);
        }
        break;
    }

    return FALSE;
}
