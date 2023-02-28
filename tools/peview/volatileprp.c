/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2022
 *
 */

#include <peview.h>

typedef struct _PVP_PE_VOLATILE_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HIMAGELIST ListViewImageList;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PVP_PE_VOLATILE_CONTEXT, *PPVP_PE_VOLATILE_CONTEXT;

VOID PvEnumerateVolatileEntries(
    _In_ HWND ListViewHandle
    )
{
    PH_MAPPED_IMAGE_VOLATILE_METADATA volatileConfig = { 0 };

    ExtendedListView_SetRedraw(ListViewHandle, FALSE);
    ListView_DeleteAllItems(ListViewHandle);

    if (NT_SUCCESS(PhGetMappedImageVolatileMetadata(&PvMappedImage, &volatileConfig)))
    {
        for (ULONG i = 0; i < volatileConfig.NumberOfRangeEntries; i++)
        {
            PH_IMAGE_VOLATILE_ENTRY entry;
            INT lvItemIndex;
            WCHAR value[PH_INT64_STR_LEN_1];
            WCHAR end[PH_INT64_STR_LEN_1];

            PhPrintUInt64(value, i + 1);
            lvItemIndex = PhAddListViewGroupItem(ListViewHandle, 2, MAXINT, value, NULL);

            entry = volatileConfig.RangeEntries[i];
            PhPrintPointer(value, UlongToPtr(entry.Rva));
            PhPrintPointer(end, UlongToPtr(entry.Rva + entry.Size));
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, PhaFormatString(L"%s - %s", value, end)->Buffer);
        }

        for (ULONG i = 0; i < volatileConfig.NumberOfAccessEntries; i++)
        {
            PH_IMAGE_VOLATILE_ENTRY entry;
            INT lvItemIndex;
            ULONG64 displacement;
            PPH_STRING symbol;
            PPH_STRING symbolName = NULL;
            PH_SYMBOL_RESOLVE_LEVEL symbolResolveLevel = PhsrlInvalid;
            WCHAR value[PH_INT64_STR_LEN_1];

            PhPrintUInt64(value, i + 1);
            lvItemIndex = PhAddListViewGroupItem(ListViewHandle, 1, MAXINT, value, NULL);

            entry = volatileConfig.AccessEntries[i];
            PhPrintPointer(value, UlongToPtr(entry.Rva));
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, value);

            if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            {
                if (!(symbol = PhGetSymbolFromAddress(
                    PvSymbolProvider,
                    (ULONG64)PTR_ADD_OFFSET(PvMappedImage.NtHeaders32->OptionalHeader.ImageBase, entry.Rva),
                    &symbolResolveLevel,
                    NULL,
                    &symbolName,
                    &displacement
                    )))
                {
                    continue;
                }
            }
            else
            {
                if (!(symbol = PhGetSymbolFromAddress(
                    PvSymbolProvider,
                    (ULONG64)PTR_ADD_OFFSET(PvMappedImage.NtHeaders->OptionalHeader.ImageBase, entry.Rva),
                    &symbolResolveLevel,
                    NULL,
                    &symbolName,
                    &displacement
                    )))
                {
                    continue;
                }
            }

            switch (symbolResolveLevel)
            {
            case PhsrlFunction:
                {
                    if (displacement)
                    {
                        PhSetListViewSubItem(
                            ListViewHandle,
                            lvItemIndex,
                            2,
                            PhaFormatString(L"%s+0x%llx", symbolName->Buffer, displacement)->Buffer
                            );
                    }
                    else
                    {
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, symbolName->Buffer);
                    }
                }
                break;
            case PhsrlModule:
            case PhsrlAddress:
                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, symbol->Buffer);
                break;
            default:
            case PhsrlInvalid:
                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"(unnamed)");
                break;
            }

            if (symbolName)
                PhDereferenceObject(symbolName);
            PhDereferenceObject(symbol);
        }

        if (volatileConfig.AccessEntries)
        {
            PhFree(volatileConfig.AccessEntries);
        }

        if (volatileConfig.RangeEntries)
        {
            PhFree(volatileConfig.RangeEntries);
        }
    }

    ExtendedListView_SortItems(ListViewHandle);
    ExtendedListView_SetRedraw(ListViewHandle, TRUE);
}

INT_PTR CALLBACK PvpPeVolatileDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_VOLATILE_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_VOLATILE_CONTEXT));
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
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 150, L"RVA");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 250, L"Symbol");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImageVolatileListViewColumns", context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            ListView_EnableGroupView(context->ListViewHandle, TRUE);
            PhAddListViewGroup(context->ListViewHandle, 2, L"Volatile Range Table");
            PhAddListViewGroup(context->ListViewHandle, 1, L"Volatile RVA Table");
            PvEnumerateVolatileEntries(context->ListViewHandle);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageVolatileListViewColumns", context->ListViewHandle);

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
