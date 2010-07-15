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
} PHP_COLORBOX_CONTEXT, *PPHP_COLORBOX_CONTEXT;

LRESULT CALLBACK PhpColorBoxWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

BOOLEAN PhColorBoxInitialization()
{
    WNDCLASSEX c = { sizeof(c) };

    c.style = 0;
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

HWND PhCreateColorBoxControl(
    __in HWND ParentHandle,
    __in INT_PTR Id
    )
{
    return CreateWindow(
        PH_COLORBOX_CLASSNAME,
        L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_BORDER,
        0,
        0,
        3,
        3,
        ParentHandle,
        (HMENU)Id,
        PhInstanceHandle,
        NULL
        );
}

VOID PhpCreateColorBoxContext(
    __out PPHP_COLORBOX_CONTEXT *Context
    )
{
    PPHP_COLORBOX_CONTEXT context;

    context = PhAllocate(sizeof(PHP_COLORBOX_CONTEXT));

    context->SelectedColor = RGB(0x00, 0x00, 0x00);
    context->Hot = FALSE;

    *Context = context;
}

VOID PhpFreeColorBoxContext(
    __in __post_invalid PPHP_COLORBOX_CONTEXT Context
    )
{
    PhFree(Context);
}

LRESULT CALLBACK PhpColorBoxWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
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

                if (!context->Hot)
                {
                    SetDCBrushColor(hdc, context->SelectedColor);
                    FillRect(hdc, &clientRect, GetStockObject(DC_BRUSH));
                }
                else
                {
                    SetDCBrushColor(hdc, PhMakeColorBrighter(context->SelectedColor, 64));
                    FillRect(hdc, &clientRect, GetStockObject(DC_BRUSH));
                }

                EndPaint(hwnd, &paintStruct);
            }
        }
        break;
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
            CHOOSECOLOR chooseColor = { sizeof(chooseColor) };
            COLORREF customColors[16] = { 0 };

            chooseColor.hwndOwner = hwnd;
            chooseColor.rgbResult = context->SelectedColor;
            chooseColor.lpCustColors = customColors;
            chooseColor.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT;

            if (ChooseColor(&chooseColor))
            {
                context->SelectedColor = chooseColor.rgbResult;
                InvalidateRect(hwnd, NULL, TRUE);
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
