/*
 * Process Hacker -
 *   Splitter control
 *
 * Copyright (C) 2017 dmex
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
 *
 */

#include <phapp.h>
#include <splitter.h>
#include <settings.h>
#include <windowsx.h>

#define SPLITTER_PADDING 6

typedef struct _PH_HSPLITTER_CONTEXT
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG Hot : 1;
            ULONG HasFocus : 1;
            ULONG Spare : 30;
        };
    };

    LONG SplitterOffset;

    HWND WindowHandle;
    HWND ParentWindow;
    HWND TopWindow;
    HWND BottomWindow;
    WNDPROC DefaultWindowProc;

    HBRUSH FocusBrush;
    HBRUSH HotBrush;
    HBRUSH NormalBrush;

    PPH_STRING SettingName;
} PH_HSPLITTER_CONTEXT, *PPH_HSPLITTER_CONTEXT;

VOID PhDeleteHSplitter(
    _Inout_ PPH_HSPLITTER_CONTEXT Context
    );

VOID PhHSplitterHandleWmSize(
    _Inout_ PPH_HSPLITTER_CONTEXT Context,
    _In_ INT Width,
    _In_ INT Height
    );

LONG GetWindowWidth(HWND hwnd)
{
    RECT Rect;

    GetWindowRect(hwnd, &Rect);
    return (Rect.right - Rect.left);
}

LONG GetWindowHeight(HWND hwnd)
{
    RECT Rect;

    GetWindowRect(hwnd, &Rect);
    return (Rect.bottom - Rect.top);
}

LONG GetClientWindowWidth(HWND hwnd)
{
    RECT Rect;

    GetClientRect(hwnd, &Rect);
    return (Rect.right - Rect.left);
}

LONG GetClientWindowHeight(HWND hwnd)
{
    RECT Rect;

    GetClientRect(hwnd, &Rect);
    return (Rect.bottom - Rect.top);
}

LRESULT CALLBACK HSplitterWindowProc(
    _In_ HWND hwnd, 
    _In_ UINT uMsg, 
    _In_ WPARAM wParam, 
    _In_ LPARAM lParam
    )
{
    PPH_HSPLITTER_CONTEXT context = NULL;

    if (uMsg == WM_CREATE)
    {
        LPCREATESTRUCT cs = (LPCREATESTRUCT)lParam;
        context = cs->lpCreateParams;
        PhSetWindowContext(hwnd, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwnd, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_DESTROY)
        {
            PhRemoveWindowContext(hwnd, PH_WINDOW_CONTEXT_DEFAULT);
        }
    }

    if (!context)
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT paintStruct;
            RECT clientRect;
            HDC hdc;

            if (hdc = BeginPaint(hwnd, &paintStruct))
            {
                GetClientRect(hwnd, &clientRect);

                if (context->HasFocus)
                    FillRect(hdc, &clientRect, context->FocusBrush);
                else if (context->Hot)
                    FillRect(hdc, &clientRect, context->HotBrush);
                else
                    FillRect(hdc, &clientRect, context->NormalBrush);

                EndPaint(hwnd, &paintStruct);
            }
        }
        return 1;
    case WM_ERASEBKGND:
        return 1;
    case WM_LBUTTONDOWN:
        {
            context->HasFocus = TRUE;

            SetCapture(hwnd);

            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
        {
            if (GetCapture() == hwnd)
            {
                ReleaseCapture();

                context->HasFocus = FALSE;

                InvalidateRect(hwnd, NULL, TRUE);
            }
        }
        break;
    case WM_MOUSELEAVE:
        {
            context->Hot = FALSE;
            InvalidateRect(hwnd, NULL, TRUE);
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
    case WM_MOUSEMOVE:
        {
            INT width;
            INT height;
            INT NewPos;
            HDWP deferHandle;
            POINT cursorPos;

            if (!context->Hot)
            {
                TRACKMOUSEEVENT trackMouseEvent = { sizeof(trackMouseEvent) };
                context->Hot = TRUE;
                InvalidateRect(hwnd, NULL, TRUE);
                trackMouseEvent.dwFlags = TME_LEAVE;
                trackMouseEvent.hwndTrack = hwnd;
                TrackMouseEvent(&trackMouseEvent);
            }

            if (!context->HasFocus)
                break;

            GetCursorPos(&cursorPos);
            ScreenToClient(context->ParentWindow, &cursorPos);
            NewPos = cursorPos.y;
            width = GetClientWindowWidth(context->ParentWindow);
            height = GetClientWindowHeight(context->ParentWindow);

            if (NewPos < 200)
                break;
            if (NewPos > height - 80)
                break;

            context->SplitterOffset = NewPos;

            deferHandle = BeginDeferWindowPos(3);
            DeferWindowPos(
                deferHandle,
                context->TopWindow,
                NULL,
                SPLITTER_PADDING,
                90,
                width - SPLITTER_PADDING * 2,
                cursorPos.y - 90,
                SWP_NOZORDER | SWP_NOACTIVATE
                );
            DeferWindowPos(
                deferHandle,
                context->WindowHandle,
                NULL,
                0,
                cursorPos.y,
                width,
                SPLITTER_PADDING,
                SWP_NOZORDER | SWP_NOACTIVATE
                );
            DeferWindowPos(
                deferHandle,
                context->BottomWindow,
                NULL,
                SPLITTER_PADDING,
                cursorPos.y + SPLITTER_PADDING,
                width - SPLITTER_PADDING * 2,
                height - (cursorPos.y + SPLITTER_PADDING) - 65,
                SWP_NOZORDER | SWP_NOACTIVATE
                );

            EndDeferWindowPos(deferHandle);
        }
        break;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK HSplitterParentWindowProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_HSPLITTER_CONTEXT context = PhGetWindowContext(hwnd, 0x200);

    if (!context)
        return 0;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)context->DefaultWindowProc);
            PhDeleteHSplitter(context);
            PhRemoveWindowContext(hwnd, 0x200);
        }
        break;
    case WM_SIZE:
        {
            PhHSplitterHandleWmSize(context, LOWORD(lParam), HIWORD(lParam));
        }
        break;
    }

    return CallWindowProc(context->DefaultWindowProc, hwnd, uMsg, wParam, lParam);
}

VOID PhInitializeHSplitter(
    _In_ PWSTR SettingName,
    _In_ HWND ParentWindow,
    _In_ HWND TopWindow,
    _In_ HWND BottomWindow
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    PPH_HSPLITTER_CONTEXT context;

    context = PhAllocate(sizeof(PH_HSPLITTER_CONTEXT));
    memset(context, 0, sizeof(PH_HSPLITTER_CONTEXT));

    context->SettingName = PhCreateString(SettingName);
    context->ParentWindow = ParentWindow;
    context->TopWindow = TopWindow;
    context->BottomWindow = BottomWindow;
    context->SplitterOffset = PhGetIntegerSetting(SettingName);
    context->FocusBrush = CreateSolidBrush(RGB(0x0, 0x0, 0x0));
    context->HotBrush = CreateSolidBrush(RGB(0x44, 0x44, 0x44));
    context->NormalBrush = GetSysColorBrush(COLOR_WINDOW);

    if (PhBeginInitOnce(&initOnce))
    {
        WNDCLASSEX c = { sizeof(c) };

        c.style = CS_GLOBALCLASS;
        c.lpfnWndProc = HSplitterWindowProc;
        c.cbClsExtra = 0;
        c.cbWndExtra = sizeof(PVOID);
        c.hInstance = PhInstanceHandle;
        c.hIcon = NULL;
        c.hCursor = LoadCursor(NULL, IDC_SIZENS);
        c.hbrBackground = NULL;
        c.lpszMenuName = NULL;
        c.lpszClassName = L"PhHSplitter";
        c.hIconSm = NULL;

        RegisterClassEx(&c);

        PhEndInitOnce(&initOnce);
    }

    context->WindowHandle = CreateWindowEx(
        WS_EX_TRANSPARENT,
        L"PhHSplitter",
        NULL,
        WS_CHILD | WS_VISIBLE,
        5,
        5,
        5,
        5,
        ParentWindow,
        NULL,
        PhInstanceHandle,
        context
        );

    ShowWindow(context->WindowHandle, SW_SHOW);
    UpdateWindow(context->WindowHandle);

    context->DefaultWindowProc = (WNDPROC)GetWindowLongPtr(ParentWindow, GWLP_WNDPROC);
    PhSetWindowContext(ParentWindow, 0x200, context);
    SetWindowLongPtr(ParentWindow, GWLP_WNDPROC, (LONG_PTR)HSplitterParentWindowProc);
}

VOID PhDeleteHSplitter(
    _Inout_ PPH_HSPLITTER_CONTEXT Context
    )
{
    PhSetIntegerSetting(PhGetString(Context->SettingName), Context->SplitterOffset);
    PhDereferenceObject(Context->SettingName);

    if (Context->FocusBrush)
        DeleteObject(Context->FocusBrush);
    if (Context->HotBrush)
        DeleteObject(Context->HotBrush);
}

VOID PhHSplitterHandleWmSize(
    _Inout_ PPH_HSPLITTER_CONTEXT Context,
    _In_ INT Width,
    _In_ INT Height
    )
{
    HDWP deferHandle;

    deferHandle = BeginDeferWindowPos(3);
    DeferWindowPos(
        deferHandle,
        Context->TopWindow,
        NULL,
        SPLITTER_PADDING,
        90,
        Width - SPLITTER_PADDING * 2,
        Context->SplitterOffset - 90 - 65,
        SWP_NOZORDER | SWP_NOACTIVATE
        );

    DeferWindowPos(
        deferHandle,
        Context->WindowHandle,
        NULL,
        0,
        Context->SplitterOffset - 65,
        Width,
        SPLITTER_PADDING,
        SWP_NOZORDER | SWP_NOACTIVATE
        );

    DeferWindowPos(
        deferHandle,
        Context->BottomWindow,
        NULL,
        SPLITTER_PADDING,
        Context->SplitterOffset + SPLITTER_PADDING - 65,
        Width - SPLITTER_PADDING * 2,
        GetClientWindowHeight(Context->ParentWindow) - (Context->SplitterOffset + SPLITTER_PADDING),
        SWP_NOZORDER | SWP_NOACTIVATE
        );
    EndDeferWindowPos(deferHandle);
}