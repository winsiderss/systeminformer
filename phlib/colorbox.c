/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2017-2024
 *
 */

#include <ph.h>
#include <colorbox.h>
#include <guisup.h>
#include <commdlg.h>

typedef struct _PH_COLORBOX_CONTEXT
{
    COLORREF SelectedColor;
    union
    {
        ULONG Flags;
        struct
        {
            ULONG Hot : 1;
            ULONG HasFocus : 1;
            ULONG EnableThemeSupport : 1;
            ULONG Spare : 29;
        };
    };
} PH_COLORBOX_CONTEXT, *PPH_COLORBOX_CONTEXT;

LRESULT CALLBACK PhColorBoxWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

BOOLEAN PhColorBoxInitialization(
    VOID
    )
{
    WNDCLASSEX wcex;

    memset(&wcex, 0, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_GLOBALCLASS | CS_PARENTDC;
    wcex.lpfnWndProc = PhColorBoxWndProc;
    wcex.cbWndExtra = sizeof(PVOID);
    wcex.hInstance = PhInstanceHandle;
    wcex.hCursor = PhLoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = PH_COLORBOX_CLASSNAME;

    if (RegisterClassEx(&wcex) == INVALID_ATOM)
        return FALSE;

    return TRUE;
}

PPH_COLORBOX_CONTEXT PhCreateColorBoxContext(
    VOID
    )
{
    PPH_COLORBOX_CONTEXT context;

    context = PhAllocate(sizeof(PH_COLORBOX_CONTEXT));
    memset(context, 0, sizeof(PH_COLORBOX_CONTEXT));

    return context;
}

VOID PhFreeColorBoxContext(
    _In_ _Post_invalid_ PPH_COLORBOX_CONTEXT Context
    )
{
    PhFree(Context);
}

UINT_PTR CALLBACK PhpColorBoxDlgHookProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            LPCHOOSECOLOR chooseColor = (LPCHOOSECOLOR)lParam;
            PPH_COLORBOX_CONTEXT context = (PPH_COLORBOX_CONTEXT)chooseColor->lCustData;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhInitializeWindowTheme(hwndDlg, !!context->EnableThemeSupport);
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

VOID PhpChooseColor(
    _In_ HWND hwnd,
    _In_ PPH_COLORBOX_CONTEXT Context
    )
{
    CHOOSECOLOR chooseColor = { sizeof(chooseColor) };
    COLORREF customColors[16] = { 0 };

    chooseColor.hwndOwner = hwnd;
    chooseColor.rgbResult = Context->SelectedColor;
    chooseColor.lpCustColors = customColors;
    chooseColor.lCustData = (LPARAM)Context;
    chooseColor.lpfnHook = PhpColorBoxDlgHookProc;
    chooseColor.Flags = CC_ANYCOLOR | CC_ENABLEHOOK | CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColor(&chooseColor))
    {
        Context->SelectedColor = chooseColor.rgbResult;
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
    }
}

LRESULT CALLBACK PhColorBoxWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_COLORBOX_CONTEXT context = PhGetWindowContextEx(hwnd);

    switch (uMsg)
    {
    case WM_NCCREATE:
        {
            context = PhCreateColorBoxContext();
            PhSetWindowContextEx(hwnd, context);
        }
        break;
    case WM_NCDESTROY:
        {
            PhRemoveWindowContextEx(hwnd);
            PhFreeColorBoxContext(context);
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT paintStruct;
            RECT updateRect;
            HBRUSH oldBrush;
            HPEN oldPen;
            HDC hdc;

            if (hdc = BeginPaint(hwnd, &paintStruct))
            {
                updateRect = paintStruct.rcPaint;

                // Border color
                SetDCPenColor(hdc, RGB(0x44, 0x44, 0x44));

                // Fill color
                if (!context->Hot && !context->HasFocus)
                    SetDCBrushColor(hdc, context->SelectedColor);
                else
                    SetDCBrushColor(hdc, PhMakeColorBrighter(context->SelectedColor, 64));

                // Select the border and fill.
                oldBrush = SelectBrush(hdc, PhGetStockBrush(DC_BRUSH));
                oldPen = SelectPen(hdc, PhGetStockPen(DC_PEN));

                // Draw the border and fill.
                Rectangle(hdc, updateRect.left, updateRect.top, updateRect.right, updateRect.bottom);

                // Restore the original border and fill.
                SelectPen(hdc, oldPen);
                SelectBrush(hdc, oldBrush);

                EndPaint(hwnd, &paintStruct);
            }
        }
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_MOUSEMOVE:
        {
            if (!context->Hot)
            {
                TRACKMOUSEEVENT trackMouseEvent = { sizeof(trackMouseEvent) };

                context->Hot = TRUE;
                InvalidateRect(hwnd, NULL, TRUE);

                trackMouseEvent.dwFlags = TME_LEAVE;
                trackMouseEvent.hwndTrack = hwnd;
                TrackMouseEvent(&trackMouseEvent);
            }
        }
        break;
    case WM_MOUSELEAVE:
        {
            context->Hot = FALSE;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;
    case WM_LBUTTONDOWN:
        {
            PhpChooseColor(hwnd, context);
        }
        break;
    case WM_SETFOCUS:
        {
            context->HasFocus = TRUE;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    case WM_KILLFOCUS:
        {
            context->HasFocus = FALSE;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    case WM_GETDLGCODE:
        if (wParam == VK_RETURN)
            return DLGC_WANTMESSAGE;
        return 0;
    case WM_KEYDOWN:
        {
            switch (wParam)
            {
            case VK_SPACE:
            case VK_RETURN:
                PhpChooseColor(hwnd, context);
                break;
            }
        }
        break;
    case CBCM_SETCOLOR:
        context->SelectedColor = (COLORREF)wParam;
        return TRUE;
    case CBCM_GETCOLOR:
        return (LRESULT)context->SelectedColor;
    case CBCM_THEMESUPPORT:
        context->EnableThemeSupport = (BOOLEAN)wParam;
        return TRUE;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
