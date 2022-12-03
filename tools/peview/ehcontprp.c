/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2020-2022
 *
 */

#include <peview.h>

typedef struct _PVP_PE_EHCONT_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HIMAGELIST ListViewImageList;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PVP_PE_EHCONT_CONTEXT, *PPVP_PE_EHCONT_CONTEXT;

VOID PvEnumerateEHContinuationEntries(
    _In_ HWND ListViewHandle
    )
{
    PH_MAPPED_IMAGE_EH_CONT ehContConfig = { 0 };

    ExtendedListView_SetRedraw(ListViewHandle, FALSE);
    ListView_DeleteAllItems(ListViewHandle);

    if (NT_SUCCESS(PhGetMappedImageEhCont(&ehContConfig, &PvMappedImage)))
    {
        for (ULONGLONG i = 0; i < ehContConfig.NumberOfEhContEntries; i++)
        {
            INT lvItemIndex;
            ULONG64 displacement;
            ULONG rva;
            PPH_STRING symbol;
            PPH_STRING symbolName = NULL;
            PH_SYMBOL_RESOLVE_LEVEL symbolResolveLevel = PhsrlInvalid;
            WCHAR value[PH_INT64_STR_LEN_1];

            PhPrintUInt64(value, i + 1);
            lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, value, NULL);

            rva = *(PULONG)PTR_ADD_OFFSET(ehContConfig.EhContTable, i * ehContConfig.EntrySize);
            PhPrintPointer(value, UlongToPtr(rva));
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, value);

            // Resolve name based on public symbols

            if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            {
                if (!(symbol = PhGetSymbolFromAddress(
                    PvSymbolProvider,
                    (ULONG64)PTR_ADD_OFFSET(PvMappedImage.NtHeaders32->OptionalHeader.ImageBase, rva),
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
                    (ULONG64)PTR_ADD_OFFSET(PvMappedImage.NtHeaders->OptionalHeader.ImageBase, rva),
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
    }

    ExtendedListView_SortItems(ListViewHandle);
    ExtendedListView_SetRedraw(ListViewHandle, TRUE);
}

INT_PTR CALLBACK PvpPeEhContDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_EHCONT_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_EHCONT_CONTEXT));
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
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 80, L"RVA");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 250, L"Symbol");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImageEhContListViewColumns", context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PvEnumerateEHContinuationEntries(context->ListViewHandle);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageEhContListViewColumns", context->ListViewHandle);

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
