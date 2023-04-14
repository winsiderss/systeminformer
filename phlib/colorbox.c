/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2017-2021
 *
 */

#include <ph.h>
#include <colorbox.h>
#include <guisup.h>

#include <commdlg.h>

typedef struct _PHP_COLORBOX_CONTEXT
{
    COLORREF SelectedColor;
    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN Hot : 1;
            BOOLEAN HasFocus : 1;
            BOOLEAN EnableThemeSupport : 1;
            BOOLEAN Spare : 5;
        };
    };
} PHP_COLORBOX_CONTEXT, *PPHP_COLORBOX_CONTEXT;

LRESULT CALLBACK PhpColorBoxWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

BOOLEAN PhColorBoxInitialization(
    VOID
    )
{
    WNDCLASSEX c = { sizeof(c) };

    c.style = CS_PARENTDC | CS_GLOBALCLASS;
    c.lpfnWndProc = PhpColorBoxWndProc;
    c.cbClsExtra = 0;
    c.cbWndExtra = sizeof(PVOID);
    c.hInstance = PhInstanceHandle;
    c.hIcon = NULL;
    c.hCursor = LoadCursor(NULL, IDC_ARROW);
    c.hbrBackground = NULL;
    c.lpszMenuName = NULL;
    c.lpszClassName = PH_COLORBOX_CLASSNAME;
    c.hIconSm = NULL;

    if (!RegisterClassEx(&c))
        return FALSE;

    return TRUE;
}

VOID PhpCreateColorBoxContext(
    _Out_ PPHP_COLORBOX_CONTEXT *Context
    )
{
    PPHP_COLORBOX_CONTEXT context;

    context = PhAllocate(sizeof(PHP_COLORBOX_CONTEXT));
    memset(context, 0, sizeof(PHP_COLORBOX_CONTEXT));
    *Context = context;
}

VOID PhpFreeColorBoxContext(
    _In_ _Post_invalid_ PPHP_COLORBOX_CONTEXT Context
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
            PPHP_COLORBOX_CONTEXT context = (PPHP_COLORBOX_CONTEXT)chooseColor->lCustData;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhInitializeWindowTheme(hwndDlg, context->EnableThemeSupport);
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
    _In_ PPHP_COLORBOX_CONTEXT Context
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
    }
}

LRESULT CALLBACK PhpColorBoxWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_COLORBOX_CONTEXT context;

    context = PhGetWindowContext(hwnd, MAXCHAR);

    if (uMsg == WM_CREATE)
    {
        PhpCreateColorBoxContext(&context);
        PhSetWindowContext(hwnd, MAXCHAR, context);
    }

    if (!context)
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_CREATE:
        {
            // Nothing
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwnd, MAXCHAR);
            PhpFreeColorBoxContext(context);
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT paintStruct;
            RECT clientRect;
            HDC hdc;

            if (hdc = BeginPaint(hwnd, &paintStruct))
            {
                GetClientRect(hwnd, &clientRect);

                // Border color
                SetDCPenColor(hdc, RGB(0x44, 0x44, 0x44));

                // Fill color
                if (!context->Hot && !context->HasFocus)
                    SetDCBrushColor(hdc, context->SelectedColor);
                else
                    SetDCBrushColor(hdc, PhMakeColorBrighter(context->SelectedColor, 64));

                // Draw the rectangle.
                SelectPen(hdc, GetStockPen(DC_PEN));
                SelectBrush(hdc, GetStockBrush(DC_BRUSH));
                Rectangle(hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);

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
