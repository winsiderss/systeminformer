/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2019-2022
 *
 */

#include <peview.h>

typedef struct _PV_PE_PREVIEW_CONTEXT
{
    HWND WindowHandle;
    HWND EditWindow;
    HIMAGELIST ListViewImageList;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PV_PE_PREVIEW_CONTEXT, *PPV_PE_PREVIEW_CONTEXT;

VOID PvpSetRichEditText(
    _In_ HWND WindowHandle,
    _In_ PWSTR Text
    )
{
    //SetFocus(WindowHandle);
    SendMessage(WindowHandle, WM_SETREDRAW, FALSE, 0);
    //SendMessage(WindowHandle, EM_SETSEL, 0, -1); // -2
    SendMessage(WindowHandle, WM_SETTEXT, FALSE, (LPARAM)Text);
    //SendMessage(WindowHandle, WM_VSCROLL, SB_TOP, 0); // requires SetFocus()    
    SendMessage(WindowHandle, WM_SETREDRAW, TRUE, 0);
    //PostMessage(WindowHandle, EM_SETSEL, -1, 0);
    //InvalidateRect(WindowHandle, NULL, FALSE);
}

VOID PvpShowFilePreview(
    _In_ HWND WindowHandle
    )
{
    PPH_STRING fileText;
    PH_STRING_BUILDER sb;

    if (fileText = PhFileReadAllText(PvFileName->Buffer, TRUE))
    {
        PhInitializeStringBuilder(&sb, 0x1000);

        for (SIZE_T i = 0; i < fileText->Length / sizeof(WCHAR); i++)
        {
            if (iswprint(fileText->Buffer[i]))
            {
                PhAppendCharStringBuilder(&sb, fileText->Buffer[i]);
            }
            else
            {
                PhAppendCharStringBuilder(&sb, L' ');
            }
        }

        PhMoveReference(&fileText, PhFinalStringBuilderString(&sb));

        PvpSetRichEditText(GetDlgItem(WindowHandle, IDC_PREVIEW), fileText->Buffer);

        PhDereferenceObject(fileText);
    }
}

INT_PTR CALLBACK PvpPePreviewDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_PE_PREVIEW_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PV_PE_PREVIEW_CONTEXT));
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

            PvpShowFilePreview(hwndDlg);
            
            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);

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
            return (INT_PTR)GetStockBrush(DC_BRUSH);
        }
        break;
    }

    return FALSE;
}
