/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2025
 *
 */

#include <peview.h>

typedef struct _PV_APP_MANIFEST_CONTEXT
{
    HWND WindowHandle;
    HWND EditWindow;
    HIMAGELIST ListViewImageList;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PV_APP_MANIFEST_CONTEXT, *PPV_APP_MANIFEST_CONTEXT;

VOID PvpShowAppManifest(
    _In_ PPV_APP_MANIFEST_CONTEXT Context
    )
{
    ULONG manifestLength;
    PVOID manifestBuffer;

    if (NT_SUCCESS(PhGetMappedImageResource(
        &PvMappedImage,
        MAKEINTRESOURCEW(1),
        RT_MANIFEST,
        0,
        &manifestLength,
        &manifestBuffer
        )))
    {
        PPH_STRING manifestString;

        manifestString = PhConvertUtf8ToUtf16Ex(manifestBuffer, manifestLength);

        SendMessage(Context->EditWindow, WM_SETREDRAW, FALSE, 0);
        SendMessage(Context->EditWindow, WM_SETTEXT, FALSE, (LPARAM)manifestString->Buffer);
        SendMessage(Context->EditWindow, WM_SETREDRAW, TRUE, 0);

        PhDereferenceObject(manifestString);
    }
}

INT_PTR CALLBACK PvPeAppManifestDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_APP_MANIFEST_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PV_APP_MANIFEST_CONTEXT));
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
            context->EditWindow = GetDlgItem(hwndDlg, IDC_PREVIEW);

            SendMessage(context->EditWindow, EM_SETLIMITTEXT, ULONG_MAX, 0);
            PvConfigTreeBorders(context->EditWindow);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->EditWindow, NULL, PH_ANCHOR_ALL);

            PvpShowAppManifest(context);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
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
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)context->EditWindow);
                return TRUE;
            }
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
            return (INT_PTR)PhGetStockBrush(DC_BRUSH);
        }
        break;
    }

    return FALSE;
}
