/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2021-2022
 *
 */

#include <peview.h>

#ifdef __has_include
#if __has_include (<metahost.h>)
#include <metahost.h>
#else
#include "metahost/metahost.h"
#endif
#else
#include "metahost/metahost.h"
#endif

typedef struct _PVP_PE_CLR_IMPORTS_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PVP_PE_CLR_IMPORTS_CONTEXT, *PPVP_PE_CLR_IMPORTS_CONTEXT;

VOID PvpEnumerateClrImports(
    _In_ HWND ListViewHandle
    )
{
    PPH_LIST clrImportsList = NULL;

    if (SUCCEEDED(PvGetClrImageImports(&clrImportsList)))
    {
        ULONG count = 0;
        ULONG i;
        ULONG j;

        for (i = 0; i < clrImportsList->Count; i++)
        {
            PPV_CLR_IMAGE_IMPORT_DLL importDll = clrImportsList->Items[i];

            if (importDll->Functions)
            {
                if (importDll->ImportName)
                {
                    PPH_STRING importDllName;

                    if (importDllName = PhApiSetResolveToHost(&importDll->ImportName->sr))
                    {
                        PhMoveReference(&importDll->ImportName, PhFormatString(
                            L"%s (%s)",
                            PhGetString(importDll->ImportName),
                            PhGetString(importDllName))
                            );
                        PhDereferenceObject(importDllName);
                    }
                }

                for (j = 0; j < importDll->Functions->Count; j++)
                {
                    PPV_CLR_IMAGE_IMPORT_FUNCTION importFunction = importDll->Functions->Items[j];
                    INT lvItemIndex;
                    WCHAR value[PH_INT64_STR_LEN_1];

                    PhPrintUInt32(value, ++count);
                    lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, value, NULL);

                    PhPrintPointer(value, importFunction->Offset);
                    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, value);
                    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetString(importDll->ImportName));
                    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, PhGetString(importFunction->FunctionName));
                    if (importFunction->Flags)
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 4, PH_AUTO_T(PH_STRING, PvClrImportFlagsToString(importFunction->Flags))->Buffer);

                    PhClearReference(&importFunction->FunctionName);
                    PhFree(importFunction);
                }

                PhDereferenceObject(importDll->Functions);
            }

            PhClearReference(&importDll->ImportName);
            PhFree(importDll);
        }

        PhDereferenceObject(clrImportsList);
    }
}

INT_PTR CALLBACK PvpPeClrImportsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_CLR_IMPORTS_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_CLR_IMPORTS_CONTEXT));
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
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 250, L"RVA");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"DLL");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 250, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 80, L"Flags");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImageClrImportsListViewColumns", context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PvpEnumerateClrImports(context->ListViewHandle);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageClrImportsListViewColumns", context->ListViewHandle);

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
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)context->ListViewHandle);
                return TRUE;
            }

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
