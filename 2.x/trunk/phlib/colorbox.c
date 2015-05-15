/*
 * Process Hacker -
 *   color picker
 *
 * Copyright (C) 2010 wj32
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

#include <phgui.h>
#include <colorbox.h>

typedef struct _PHP_COLORBOX_CONTEXT
{
    COLORREF SelectedColor;
    BOOLEAN Hot;
    BOOLEAN HasFocus;
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

    c.style = CS_GLOBALCLASS;
    c.lpfnWndProc = PhpColorBoxWndProc;
    c.cbClsExtra = 0;
    c.cbWndExtra = sizeof(PVOID);
    c.hInstance = PhLibImageBase;
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
    chooseColor.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT;

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

    context = (PPHP_COLORBOX_CONTEXT)GetWindowLongPtr(hwnd, 0);

    if (uMsg == WM_CREATE)
    {
        PhpCreateColorBoxContext(&context);
        SetWindowLongPtr(hwnd, 0, (LONG_PTR)context);
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
            PhpFreeColorBoxContext(context);
            SetWindowLongPtr(hwnd, 0, (LONG_PTR)NULL);
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
                SelectObject(hdc, GetStockObject(DC_PEN));
                SelectObject(hdc, GetStockObject(DC_BRUSH));
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
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
