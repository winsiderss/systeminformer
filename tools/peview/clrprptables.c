/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2023
 *
 */

#include <peview.h>

typedef struct _PVP_PE_CLR_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
    PVOID PdbMetadataAddress;
} PVP_PE_CLR_CONTEXT, *PPVP_PE_CLR_CONTEXT;

BOOLEAN NTAPI PvClrEnumTableCallback(
    _In_ ULONG TableIndex,
    _In_ ULONG TableSize,
    _In_ ULONG TableCount,
    _In_ PPH_STRING TableName,
    _In_ PVOID TableOffset,
    _In_ PVOID Context
    )
{
    PPVP_PE_CLR_CONTEXT context = Context;
    INT lvItemIndex;
    WCHAR value[PH_INT64_STR_LEN_1];

    PhPrintUInt32(value, TableIndex);
    lvItemIndex = PhAddListViewItem(context->ListViewHandle, MAXINT, value, NULL);

    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 1, PhGetStringOrEmpty(TableName));

    if (TableCount)
    {
        PhPrintUInt32(value, TableCount);
        PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 2, value);
    }

    if (TableCount && TableSize)
    {
        PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 3, PhaFormatSize(UInt32Mul32To64(TableCount, TableSize), ULONG_MAX)->Buffer);
    }

    if (TableOffset)
    {
        PhPrintPointer(value, PTR_SUB_OFFSET(TableOffset, PvMappedImage.ViewBase));
        PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 4, value);

        PhPrintPointer(value, PTR_ADD_OFFSET(PTR_SUB_OFFSET(TableOffset, PvMappedImage.ViewBase), UInt32Mul32To64(TableCount, TableSize)));
        PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 5, value);
    }

    if (TableCount && TableSize && TableOffset)
    {
        __try
        {
            PH_HASH_CONTEXT hashContext;
            PPH_STRING hashString;
            UCHAR hash[32];

            PhInitializeHash(&hashContext, Md5HashAlgorithm); // PhGetIntegerSetting(L"HashAlgorithm")
            PhUpdateHash(&hashContext, TableOffset, UInt32Mul32To64(TableCount, TableSize));

            if (PhFinalHash(&hashContext, hash, 16, NULL))
            {
                hashString = PhBufferToHexString(hash, 16);
                PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 6, hashString->Buffer);
                PhDereferenceObject(hashString);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            PPH_STRING message;

            //message = PH_AUTO(PhGetNtMessage(GetExceptionCode()));
            message = PH_AUTO(PhGetWin32Message(PhNtStatusToDosError(GetExceptionCode()))); // WIN32_FROM_NTSTATUS

            PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 6, PhGetStringOrEmpty(message));
        }
    }

    return TRUE;
}

VOID PvClrEnumerateTables(
    _In_ PPVP_PE_CLR_CONTEXT Context
    )
{
    HRESULT status;

    status = PvClrImageEnumTables( 
        PvClrEnumTableCallback, 
        Context
        );

    if (SUCCEEDED(status))
    {
        // TODO: PhShowStatus doesn't handle HRESULT (dmex)
        //PhShowStatus(Context->WindowHandle, L"Unable to enumerate CLR tables", 0, status);
    }
}

INT_PTR CALLBACK PvpPeClrTablesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_CLR_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_CLR_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);

        if (lParam)
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            context->PropSheetContext = (PPV_PROPPAGECONTEXT)propSheetPage->lParam;
            context->PdbMetadataAddress = context->PropSheetContext->Context;
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
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 80, L"Count");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 80, L"Size");
            PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 80, L"RVA (start)");
            PhAddListViewColumn(context->ListViewHandle, 5, 5, 5, LVCFMT_LEFT, 80, L"RVA (end)");
            PhAddListViewColumn(context->ListViewHandle, 6, 6, 6, LVCFMT_LEFT, 220, L"Hash");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImageClrTablesListViewColumns", context->ListViewHandle);
            PvSetListViewImageList(context->WindowHandle, context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PvClrEnumerateTables(context);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageClrTablesListViewColumns", context->ListViewHandle);
            PhDeleteLayoutManager(&context->LayoutManager);
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_DPICHANGED:
        {
            PvSetListViewImageList(context->WindowHandle, context->ListViewHandle);
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
