/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2022-2023
 *
 */

#include <peview.h>
#include <apiimport.h>
#include <mapldr.h>
#include <thirdparty.h>
#include <vsstyle.h>
#include <uxtheme.h>
#include <Vssym32.h>

#include "settings.h"

// https://learn.microsoft.com/en-us/windows/win32/winmsg/about-window-procedures#window-procedure-superclassing
static WNDPROC PhDefaultMenuWindowProcedure = NULL;
static WNDPROC PhDefaultDialogWindowProcedure = NULL;
static WNDPROC PhDefaultRebarWindowProcedure = NULL;
static WNDPROC PhDefaultComboBoxWindowProcedure = NULL;
static WNDPROC PhDefaultStaticWindowProcedure = NULL;
static WNDPROC PhDefaultStatusbarWindowProcedure = NULL;
static WNDPROC PhDefaultEditWindowProcedure = NULL;
static WNDPROC PhDefaultHeaderWindowProcedure = NULL;
static BOOLEAN PhDefaultEnableStreamerMode = FALSE;
static BOOLEAN PhDefaultEnableThemeAcrylicWindowSupport = FALSE;
static BOOLEAN PhDefaultEnableThemeAnimation = FALSE;

BOOL(WINAPI* PhIsDarkModeAllowedForWindow_I)(
    _In_ HWND WindowHandle
    ) = NULL;

LRESULT CALLBACK PhMenuWindowHookProcedure(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (WindowMessage)
    {
    case WM_NCCREATE:
        {
            //CREATESTRUCT* createStruct = (CREATESTRUCT*)lParam;

            if (PhEnableThemeSupport)
            {
                HFONT fontHandle;
                LONG windowDpi = PhGetWindowDpi(WindowHandle);

                if (fontHandle = PhCreateMessageFont(windowDpi))
                {
                    PhSetWindowContext(WindowHandle, (ULONG)'font', fontHandle);
                    SetWindowFont(WindowHandle, fontHandle, TRUE);
                }
            }
        }
        break;
    case WM_CREATE:
        {
            //CREATESTRUCT* createStruct = (CREATESTRUCT*)lParam;

            if (PhDefaultEnableStreamerMode)
            {
                if (SetWindowDisplayAffinity_Import())
                    SetWindowDisplayAffinity_Import()(WindowHandle, WDA_EXCLUDEFROMCAPTURE);
            }

            if (PhEnableThemeSupport)
            {
                if (PhEnableThemeAcrylicSupport)
                {
                    // Note: DWM crashes if called from WM_NCCREATE (dmex)
                    PhSetWindowAcrylicCompositionColor(WindowHandle, MakeABGRFromCOLORREF(0, RGB(10, 10, 10)));
                }
            }
        }
        break;
    case WM_NCDESTROY:
        {
            if (PhEnableThemeSupport)
            {
                HFONT fontHandle;

                fontHandle = PhGetWindowContext(WindowHandle, (ULONG)'font');
                PhRemoveWindowContext(WindowHandle, (ULONG)'font');

                if (fontHandle)
                {
                    DeleteFont(fontHandle);
                }
            }
        }
        break;
    }

    return CallWindowProc(PhDefaultMenuWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);
}

LRESULT CALLBACK PhDialogWindowHookProcedure(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (WindowMessage)
    {
    case WM_CREATE:
        {
            //CREATESTRUCT* createStruct = (CREATESTRUCT*)lParam;
            //IsTopLevelWindow(createStruct->hwndParent)

            if (WindowHandle == GetAncestor(WindowHandle, GA_ROOT))
            {
                if (PhDefaultEnableStreamerMode)
                {
                    if (SetWindowDisplayAffinity_Import())
                        SetWindowDisplayAffinity_Import()(WindowHandle, WDA_EXCLUDEFROMCAPTURE);
                }

                if (PhEnableThemeSupport && PhDefaultEnableThemeAcrylicWindowSupport)
                {
                    // Note: DWM crashes if called from WM_NCCREATE (dmex)
                    PhSetWindowAcrylicCompositionColor(WindowHandle, MakeABGRFromCOLORREF(0, RGB(10, 10, 10)));
                }
            }
        }
        break;
    }

    return CallWindowProc(PhDefaultDialogWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);
}

LRESULT CALLBACK PhRebarWindowHookProcedure(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (WindowMessage)
    {
    case WM_CTLCOLOREDIT:
        {
            HDC hdc = (HDC)wParam;

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, PhThemeWindowTextColor);
            SetDCBrushColor(hdc, PhThemeWindowBackground2Color);
            return (INT_PTR)PhGetStockBrush(DC_BRUSH);
        }
        break;
    }

    return CallWindowProc(PhDefaultRebarWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);
}

LRESULT CALLBACK PhComboBoxWindowHookProcedure(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LRESULT result = CallWindowProc(PhDefaultComboBoxWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);

    switch (WindowMessage)
    {
    case WM_NCCREATE:
        {
            //CREATESTRUCT* createStruct = (CREATESTRUCT*)lParam;
            COMBOBOXINFO info = { sizeof(COMBOBOXINFO) };

            if (SendMessage(WindowHandle, CB_GETCOMBOBOXINFO, 0, (LPARAM)&info))
            {
                if (PhDefaultEnableStreamerMode)
                {
                    if (SetWindowDisplayAffinity_Import())
                        SetWindowDisplayAffinity_Import()(info.hwndList, WDA_EXCLUDEFROMCAPTURE);
                }
            }
        }
        break;
    }

    return result;
}

LRESULT CALLBACK PhStaticWindowHookProcedure(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (WindowMessage)
    {
    case WM_NCCREATE:
        {
            LONG_PTR style = PhGetWindowStyle(WindowHandle);

            if ((style & SS_ICON) == SS_ICON)
            {
                PhSetWindowContext(WindowHandle, SCHAR_MAX, (PVOID)TRUE);
            }
        }
        break;
    case WM_NCDESTROY:
        {
            if (!PhGetWindowContext(WindowHandle, SCHAR_MAX))
                break;

            PhRemoveWindowContext(WindowHandle, SCHAR_MAX);
        }
        break;
    case WM_ERASEBKGND:
        {
            if (!PhGetWindowContext(WindowHandle, SCHAR_MAX))
                break;

            return TRUE;
        }
        break;
    case WM_KILLFOCUS:
        {
            WCHAR windowClassName[MAX_PATH];
            HWND ParentHandle = GetParent(WindowHandle);
            if (!GetClassName(ParentHandle, windowClassName, RTL_NUMBER_OF(windowClassName)))
                windowClassName[0] = UNICODE_NULL;
            if (PhEqualStringZ(windowClassName, L"CHECKLIST_ACLUI", FALSE))
            {
                RECT rectClient;
                GetClientRect(WindowHandle, &rectClient);
                InflateRect(&rectClient, 2, 2);
                MapWindowRect(WindowHandle, ParentHandle, &rectClient);
                InvalidateRect(ParentHandle, &rectClient, TRUE);     // fix the annoying white border left by the previous active control
            }
        }
        break;
    case WM_PAINT:
        {
            HDC hdc;
            HICON iconHandle;
            RECT clientRect;
            WCHAR windowClassName[MAX_PATH];

            if (!PhGetWindowContext(WindowHandle, SCHAR_MAX))
                break;

            if (!GetClassName(GetParent(WindowHandle), windowClassName, RTL_NUMBER_OF(windowClassName)))
                windowClassName[0] = UNICODE_NULL;
            if (PhEqualStringZ(windowClassName, L"CHECKLIST_ACLUI", FALSE))
            {
                if (iconHandle = (HICON)(UINT_PTR)CallWindowProc(PhDefaultStaticWindowProcedure, WindowHandle, STM_GETICON, 0, 0))
                {
                    static PH_INITONCE initOnce = PH_INITONCE_INIT;
                    static HFONT hCheckFont = NULL;

                    LRESULT retval = CallWindowProc(PhDefaultStaticWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);
                    hdc = GetDC(WindowHandle);
                    GetClientRect(WindowHandle, &clientRect);
                    INT buttonCenterX = clientRect.left + (clientRect.right - clientRect.left) / 2 + 1;
                    INT buttonCenterY = clientRect.top + (clientRect.bottom - clientRect.top) / 2;
                    COLORREF checkCenter = GetPixel(hdc, buttonCenterX, buttonCenterY);
                    if (checkCenter == RGB(0, 0, 0) || checkCenter == RGB(0xB4, 0xB4, 0xB4))   // right is checked or special permission checked
                    {
                        if (PhBeginInitOnce(&initOnce)) // cache font  
                        {
                            hCheckFont = CreateFont(
                                clientRect.bottom - clientRect.top - 1,
                                clientRect.right - clientRect.left - 3,
                                0, 0,
                                FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                                CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                VARIABLE_PITCH, L"Segoe UI");
                            PhEndInitOnce(&initOnce);
                        }
                        SetBkMode(hdc, TRANSPARENT);
                        SetTextColor(hdc, checkCenter == RGB(0, 0, 0) ? PhThemeWindowTextColor : RGB(0xB4, 0xB4, 0xB4));
                        SelectFont(hdc, hCheckFont);
                        //HFONT hFontOriginal = SelectFont(hdc, hCheckFont);
                        FillRect(hdc, &clientRect, PhThemeWindowBackgroundBrush);
                        DrawText(hdc, L"✓", 1, &clientRect, DT_CENTER | DT_VCENTER);
                        //SelectFont(hdc, hFontOriginal);
                    }
                    ReleaseDC(WindowHandle, hdc);
                    return retval;
                }
            }
            //else if (iconHandle = (HICON)(UINT_PTR)CallWindowProc(PhDefaultStaticWindowProcedure, WindowHandle, STM_GETICON, 0, 0)) // Static_GetIcon(WindowHandle, 0)
            //{
            //    PAINTSTRUCT ps;
            //    if (PhGetWindowContext(GetParent(WindowHandle), LONG_MAX) &&
            //        BeginPaint(WindowHandle, &ps))
            //    {
            //        // Fix artefacts when window moving back from off-screen (Dart Vanya)
            //        hdc = GetDC(WindowHandle);
            //        GetClientRect(WindowHandle, &clientRect);

            //        FillRect(hdc, &clientRect, PhThemeWindowBackgroundBrush);

            //        DrawIconEx(
            //            hdc,
            //            clientRect.left,
            //            clientRect.top,
            //            iconHandle,
            //            clientRect.right - clientRect.left,
            //            clientRect.bottom - clientRect.top,
            //            0,
            //            NULL,
            //            DI_NORMAL
            //            );

            //        ReleaseDC(WindowHandle, hdc);
            //        EndPaint(WindowHandle, &ps);
            //    }
            //}
        }
    }

    return CallWindowProc(PhDefaultStaticWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);
}

typedef struct _PHP_THEME_WINDOW_STATUSBAR_CONTEXT
{
    struct
    {
       BOOLEAN Flags;
       union
       {
            BOOLEAN NonMouseActive : 1;
            BOOLEAN MouseActive : 1;
            BOOLEAN HotTrack : 1;
            BOOLEAN Hot : 1;
            BOOLEAN Spare : 4;
       };
    };

    HTHEME ThemeHandle;
    POINT CursorPos;

    HDC BufferedDc;
    HBITMAP BufferedOldBitmap;
    HBITMAP BufferedBitmap;
    RECT BufferedContextRect;
} PHP_THEME_WINDOW_STATUSBAR_CONTEXT, *PPHP_THEME_WINDOW_STATUSBAR_CONTEXT;

VOID ThemeWindowStatusBarCreateBufferedContext(
    _In_ PPHP_THEME_WINDOW_STATUSBAR_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ RECT BufferRect
    )
{
    Context->BufferedDc = CreateCompatibleDC(Hdc);

    if (!Context->BufferedDc)
        return;

    Context->BufferedContextRect = BufferRect;
    Context->BufferedBitmap = CreateCompatibleBitmap(
        Hdc,
        Context->BufferedContextRect.right,
        Context->BufferedContextRect.bottom
        );

    Context->BufferedOldBitmap = SelectBitmap(Context->BufferedDc, Context->BufferedBitmap);
}

VOID ThemeWindowStatusBarDestroyBufferedContext(
    _In_ PPHP_THEME_WINDOW_STATUSBAR_CONTEXT Context
    )
{
    if (Context->BufferedDc && Context->BufferedOldBitmap)
    {
        SelectBitmap(Context->BufferedDc, Context->BufferedOldBitmap);
    }

    if (Context->BufferedBitmap)
    {
        DeleteBitmap(Context->BufferedBitmap);
        Context->BufferedBitmap = NULL;
    }

    if (Context->BufferedDc)
    {
        DeleteDC(Context->BufferedDc);
        Context->BufferedDc = NULL;
    }
}

INT ThemeWindowStatusBarUpdateRectToIndex(
    _In_ HWND WindowHandle,
    _In_ WNDPROC WindowProcedure,
    _In_ PRECT UpdateRect,
    _In_ INT Count
    )
{
    for (INT i = 0; i < Count; i++)
    {
        RECT blockRect = { 0 };

        if (!CallWindowProc(WindowProcedure, WindowHandle, SB_GETRECT, (WPARAM)i, (WPARAM)&blockRect))
            continue;

        if (
            UpdateRect->bottom == blockRect.bottom &&
            //UpdateRect->left == blockRect.left &&
            UpdateRect->right == blockRect.right
            //UpdateRect->top == blockRect.top
            )
        {
            return i;
        }
    }

    return INT_ERROR;
}

VOID ThemeWindowStatusBarDrawPart(
    _In_ PPHP_THEME_WINDOW_STATUSBAR_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC bufferDc,
    _In_ PRECT clientRect,
    _In_ INT Index
    )
{
    RECT blockRect = { 0 };
    WCHAR text[0x80] = { 0 };

    if (!CallWindowProc(PhDefaultStatusbarWindowProcedure, WindowHandle, SB_GETRECT, (WPARAM)Index, (WPARAM)&blockRect))
        return;
    if (!RectVisible(bufferDc, &blockRect))
        return;
    if (CallWindowProc(PhDefaultStatusbarWindowProcedure, WindowHandle, SB_GETTEXTLENGTH, (WPARAM)Index, 0) >= RTL_NUMBER_OF(text))
        return;
    if (!CallWindowProc(PhDefaultStatusbarWindowProcedure, WindowHandle, SB_GETTEXT, (WPARAM)Index, (LPARAM)text))
        return;

    if (PhPtInRect(&blockRect, Context->CursorPos))
    {
        SetTextColor(bufferDc, RGB(0xff, 0xff, 0xff));
        SetDCBrushColor(bufferDc, PhThemeWindowHighlightColor);
        blockRect.left -= 3, blockRect.top -= 1;
        FillRect(bufferDc, &blockRect, PhGetStockBrush(DC_BRUSH));
        blockRect.left += 3, blockRect.top += 1;
    }
    else
    {
        RECT separator;
        SetTextColor(bufferDc, PhThemeWindowTextColor);
        FillRect(bufferDc, &blockRect, PhThemeWindowBackgroundBrush);

        separator = blockRect;
        separator.left = separator.right - 1;
        PhInflateRect(&separator, 0, -1);
        SetDCBrushColor(bufferDc, PhThemeWindowHighlightColor);
        FillRect(bufferDc, &separator, PhGetStockBrush(DC_BRUSH));
    }

    blockRect.left += 2, blockRect.bottom -= 1;
    DrawText(
        bufferDc,
        text,
        (UINT)PhCountStringZ(text),
        &blockRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX
        );
    blockRect.left -= 2, blockRect.bottom += 1;
}

VOID ThemeWindowRenderStatusBar(
    _In_ PPHP_THEME_WINDOW_STATUSBAR_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC bufferDc,
    _In_ PRECT clientRect
    )
{
    SetBkMode(bufferDc, TRANSPARENT);
    SelectFont(bufferDc, GetWindowFont(WindowHandle));

    FillRect(bufferDc, clientRect, PhThemeWindowBackgroundBrush);

    INT blockCount = (INT)CallWindowProc(
        PhDefaultStatusbarWindowProcedure,
        WindowHandle,
        SB_GETPARTS,
        0, 0
        );

    //INT index = ThemeWindowStatusBarUpdateRectToIndex( // used with BeginBufferedPaint (dmex)
    //    WindowHandle,
    //    WindowProcedure,
    //    clientRect,
    //    blockCount
    //    );
    //
    //if (index == UINT_MAX)
    {
        RECT sizeGripRect;
        LONG dpi;

        dpi = PhGetWindowDpi(WindowHandle);
        sizeGripRect.left = clientRect->right - PhGetSystemMetrics(SM_CXHSCROLL, dpi);
        sizeGripRect.top = clientRect->bottom - PhGetSystemMetrics(SM_CYVSCROLL, dpi);
        sizeGripRect.right = clientRect->right;
        sizeGripRect.bottom = clientRect->bottom;

        if (Context->ThemeHandle)
        {
            //if (IsThemeBackgroundPartiallyTransparent(Context->ThemeHandle, SP_GRIPPER, 0))
            //    DrawThemeParentBackground(WindowHandle, bufferDc, NULL);

            PhDrawThemeBackground(Context->ThemeHandle, bufferDc, SP_GRIPPER, 0, &sizeGripRect, &sizeGripRect);
        }
        else
        {
            DrawFrameControl(bufferDc, &sizeGripRect, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
        }

        // Top statusbar border will be drawn by bottom tabcontrol border

        for (INT i = 0; i < blockCount; i++)
        {
            ThemeWindowStatusBarDrawPart(Context, WindowHandle, bufferDc, clientRect, i);
        }
    }
    //else
    //{
    //    ThemeWindowStatusBarDrawPart(Context, WindowHandle, bufferDc, clientRect, WindowProcedure, index);
    //}
}

LRESULT CALLBACK PhStatusBarWindowHookProcedure(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_THEME_WINDOW_STATUSBAR_CONTEXT context = NULL;

    if (WindowMessage == WM_NCCREATE)
    {
        context = PhAllocateZero(sizeof(PHP_THEME_WINDOW_STATUSBAR_CONTEXT));
        context->ThemeHandle = PhOpenThemeData(WindowHandle, VSCLASS_STATUS, PhGetWindowDpi(WindowHandle));
        context->CursorPos.x = LONG_MIN;
        context->CursorPos.y = LONG_MIN;
        PhSetWindowContext(WindowHandle, LONG_MAX, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, LONG_MAX);
    }

    if (context)
    {
        switch (WindowMessage)
        {
        case WM_NCDESTROY:
            {
                PhRemoveWindowContext(WindowHandle, LONG_MAX);

                ThemeWindowStatusBarDestroyBufferedContext(context);

                if (context->ThemeHandle)
                {
                    PhCloseThemeData(context->ThemeHandle);
                }

                PhFree(context);
            }
            break;
        case WM_THEMECHANGED:
            {
                if (context->ThemeHandle)
                {
                    PhCloseThemeData(context->ThemeHandle);
                    context->ThemeHandle = NULL;
                }

                context->ThemeHandle = PhOpenThemeData(WindowHandle, VSCLASS_STATUS, PhGetWindowDpi(WindowHandle));
            }
            break;
        case WM_ERASEBKGND:
            return TRUE;
        case WM_MOUSEMOVE:
            {
                if (!context->MouseActive)
                {
                    TRACKMOUSEEVENT trackEvent =
                    {
                        sizeof(TRACKMOUSEEVENT),
                        TME_LEAVE,
                        WindowHandle,
                        0
                    };

                    TrackMouseEvent(&trackEvent);
                    context->MouseActive = TRUE;
                }

                context->CursorPos.x = GET_X_LPARAM(lParam);
                context->CursorPos.y = GET_Y_LPARAM(lParam);

                InvalidateRect(WindowHandle, NULL, FALSE);
            }
            break;
        case WM_MOUSELEAVE:
            {
                context->MouseActive = FALSE;
                context->CursorPos.x = LONG_MIN;
                context->CursorPos.y = LONG_MIN;

                InvalidateRect(WindowHandle, NULL, FALSE);
            }
            break;
        case WM_PAINT:
            {
                //PAINTSTRUCT ps;
                //HDC BufferedHDC;
                //HPAINTBUFFER BufferedPaint;
                //
                //if (!BeginPaint(WindowHandle, &ps))
                //    break;
                //
                //if (BufferedPaint = BeginBufferedPaint(ps.hdc, &ps.rcPaint, BPBF_COMPATIBLEBITMAP, NULL, &BufferedHDC))
                //{
                //    ThemeWindowRenderStatusBar(context, WindowHandle, BufferedHDC, &ps.rcPaint, oldWndProc);
                //    EndBufferedPaint(BufferedPaint, TRUE);
                //}
                //else
                {
                    RECT clientRect;
                    RECT bufferRect;
                    HDC hdc;

                    GetClientRect(WindowHandle, &clientRect);
                    bufferRect.left = 0;
                    bufferRect.top = 0;
                    bufferRect.right = clientRect.right - clientRect.left;
                    bufferRect.bottom = clientRect.bottom - clientRect.top;

                    hdc = GetDC(WindowHandle);

                    if (context->BufferedDc && (
                        context->BufferedContextRect.right < bufferRect.right ||
                        context->BufferedContextRect.bottom < bufferRect.bottom))
                    {
                        ThemeWindowStatusBarDestroyBufferedContext(context);
                    }

                    if (!context->BufferedDc)
                    {
                        ThemeWindowStatusBarCreateBufferedContext(context, hdc, bufferRect);
                    }

                    if (context->BufferedDc)
                    {
                        ThemeWindowRenderStatusBar(
                            context,
                            WindowHandle,
                            context->BufferedDc,
                            &clientRect
                            );

                        BitBlt(hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom, context->BufferedDc, 0, 0, SRCCOPY);
                    }

                    ReleaseDC(WindowHandle, hdc);
                }

                //EndPaint(WindowHandle, &ps);
            }
            goto DefaultWndProc;
        }
    }

    return CallWindowProc(PhDefaultStatusbarWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);

DefaultWndProc:
    return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);
}

LRESULT CALLBACK PhEditWindowHookProcedure(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (WindowMessage)
    {
    case WM_NCPAINT:
        {
            HDC hdc;
            ULONG flags;
            RECT windowRect;
            HRGN updateRegion;

            // The searchbox control does its own theme drawing.
            if (PhGetWindowContext(WindowHandle, SHRT_MAX))
                break;

            updateRegion = (HRGN)wParam;

            if (updateRegion == HRGN_FULL)
                updateRegion = NULL;

            flags = DCX_WINDOW | DCX_LOCKWINDOWUPDATE | DCX_USESTYLE;

            if (updateRegion)
                flags |= DCX_INTERSECTRGN | DCX_NODELETERGN;

            if (hdc = GetDCEx(WindowHandle, updateRegion, flags))
            {
                GetWindowRect(WindowHandle, &windowRect);
                PhOffsetRect(&windowRect, -windowRect.left, -windowRect.top);

                if (GetFocus() == WindowHandle)
                {
                    SetDCBrushColor(hdc, GetSysColor(COLOR_HOTLIGHT)); // PhThemeWindowHighlightColor
                    FrameRect(hdc, &windowRect, PhGetStockBrush(DC_BRUSH));
                }
                else
                {
                    SetDCBrushColor(hdc, PhThemeWindowBackground2Color);
                    FrameRect(hdc, &windowRect, PhGetStockBrush(DC_BRUSH));
                }

                ReleaseDC(WindowHandle, hdc);
                return 0;
            }
        }
        break;
    }

    return CallWindowProc(PhDefaultEditWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);
}

typedef struct _PHP_THEME_WINDOW_HEADER_CONTEXT
{
    HTHEME ThemeHandle;
    BOOLEAN MouseActive;
    POINT CursorPos;
} PHP_THEME_WINDOW_HEADER_CONTEXT, *PPHP_THEME_WINDOW_HEADER_CONTEXT;

VOID ThemeWindowRenderHeaderControl(
    _In_ PPHP_THEME_WINDOW_HEADER_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC bufferDc,
    _In_ PRECT clientRect
    )
{
    SetBkMode(bufferDc, TRANSPARENT);
    SelectFont(bufferDc, GetWindowFont(WindowHandle));

    FillRect(bufferDc, clientRect, PhThemeWindowBackgroundBrush);

    //INT headerItemCount = Header_GetItemCount(WindowHandle);
    INT headerItemCount = (INT)CallWindowProc(
        PhDefaultHeaderWindowProcedure,
        WindowHandle,
        HDM_GETITEMCOUNT,
        0, 0
        );

    for (INT i = 0; i < headerItemCount; i++)
    {
        RECT headerRect = { 0 };

        //Header_GetItemRect(WindowHandle, i, &headerRect);
        if (!(BOOL)CallWindowProc(
            PhDefaultHeaderWindowProcedure,
            WindowHandle,
            HDM_GETITEMRECT,
            (WPARAM)i,
            (LPARAM)&headerRect
            ))
        {
            continue;
        }

        if (PhPtInRect(&headerRect, Context->CursorPos))
        {
            SetTextColor(bufferDc, PhThemeWindowTextColor);
            SetDCBrushColor(bufferDc, PhThemeWindowBackground2Color); // PhThemeWindowHighlightColor);
            FillRect(bufferDc, &headerRect, PhGetStockBrush(DC_BRUSH));
            //FrameRect(bufferDc, &headerRect, GetSysColorBrush(COLOR_HIGHLIGHT));
        }
        else
        {
            SetTextColor(bufferDc, PhThemeWindowTextColor);
            FillRect(bufferDc, &headerRect, PhThemeWindowBackgroundBrush);

            //FrameRect(hdc, &headerRect, GetSysColorBrush(COLOR_HIGHLIGHT));
            //SetDCPenColor(hdc, RGB(0, 255, 0));
            //SetDCBrushColor(hdc, RGB(0, 255, 0));
            //DrawEdge(hdc, &headerRect, BDR_RAISEDOUTER | BF_FLAT, BF_RIGHT);

            //RECT frameRect;
            //frameRect.bottom = headerRect.bottom - 2;
            //frameRect.left = headerRect.right - 1;
            //frameRect.right = headerRect.right;
            //frameRect.top = headerRect.top;
            //SetDCBrushColor(hdc, RGB(68, 68, 68)); // RGB(0x77, 0x77, 0x77));
            //FrameRect(hdc, &headerRect, PhGetStockBrush(DC_BRUSH));

            //PatBlt(DrawInfo->hDC, DrawInfo->rcItem.right - 1, DrawInfo->rcItem.top, 1, DrawInfo->rcItem.bottom - DrawInfo->rcItem.top, PATCOPY);
            //PatBlt(DrawInfo->hDC, DrawInfo->rcItem.left, DrawInfo->rcItem.bottom - 1, DrawInfo->rcItem.right - DrawInfo->rcItem.left, 1, PATCOPY);
            DrawEdge(bufferDc, &headerRect, BDR_RAISEDOUTER, BF_RIGHT);
        }

        INT drawTextFlags = DT_SINGLELINE | DT_HIDEPREFIX | DT_WORD_ELLIPSIS;
        WCHAR headerText[0x80] = { 0 };
        HDITEM headerItem;

        ZeroMemory(&headerItem, sizeof(HDITEM));
        headerItem.mask = HDI_TEXT | HDI_FORMAT;
        headerItem.cchTextMax = MAX_PATH;
        headerItem.pszText = headerText;

        //Header_GetItem(WindowHandle, i, &headerItem);
        if (!(BOOL)CallWindowProc(
            PhDefaultHeaderWindowProcedure,
            WindowHandle,
            HDM_GETITEM,
            (WPARAM)i,
            (LPARAM)&headerItem
            ))
        {
            break;
        }

        if (headerItem.fmt & HDF_SORTUP)
        {
            if (Context->ThemeHandle)
            {
                RECT sortArrowRect = headerRect;
                SIZE sortArrowSize;

                if (PhGetThemePartSize(
                    Context->ThemeHandle,
                    bufferDc,
                    HP_HEADERSORTARROW,
                    HSAS_SORTEDUP,
                    NULL,
                    TS_TRUE,
                    &sortArrowSize
                    ))
                {
                    sortArrowRect.bottom = sortArrowSize.cy;
                }

                PhDrawThemeBackground(
                    Context->ThemeHandle,
                    bufferDc,
                    HP_HEADERSORTARROW,
                    HSAS_SORTEDUP,
                    &sortArrowRect,
                    NULL
                    );
            }
        }
        else if (headerItem.fmt & HDF_SORTDOWN)
        {
            if (Context->ThemeHandle)
            {
                RECT sortArrowRect = headerRect;
                SIZE sortArrowSize;

                if (PhGetThemePartSize(
                    Context->ThemeHandle,
                    bufferDc,
                    HP_HEADERSORTARROW,
                    HSAS_SORTEDDOWN,
                    NULL,
                    TS_TRUE,
                    &sortArrowSize
                    ))
                {
                    sortArrowRect.bottom = sortArrowSize.cy;
                }

                PhDrawThemeBackground(
                    Context->ThemeHandle,
                    bufferDc,
                    HP_HEADERSORTARROW,
                    HSAS_SORTEDDOWN,
                    &sortArrowRect,
                    NULL
                    );
            }
        }

        if (headerItem.fmt & HDF_RIGHT)
            drawTextFlags |= DT_VCENTER | DT_RIGHT;
        else
            drawTextFlags |= DT_VCENTER | DT_LEFT;

        headerRect.left += 4;
        headerRect.right -= 8;
        DrawText(
            bufferDc,
            headerText,
            (UINT)PhCountStringZ(headerText),
            &headerRect,
            drawTextFlags
            );
    }
}

LRESULT CALLBACK PhHeaderWindowHookProcedure(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_THEME_WINDOW_HEADER_CONTEXT context = NULL;

    if (WindowMessage == WM_NCCREATE)
    {
        CREATESTRUCT* createStruct = (CREATESTRUCT*)lParam;

        if (createStruct->hwndParent)
        {
            WCHAR windowClassName[MAX_PATH];

            if (!GetClassName(createStruct->hwndParent, windowClassName, RTL_NUMBER_OF(windowClassName)))
                windowClassName[0] = UNICODE_NULL;

            if (PhEqualStringZ(windowClassName, L"PhTreeNew", FALSE))
            {
                LONG_PTR windowStyle = PhGetWindowStyle(createStruct->hwndParent);

                if (BooleanFlagOn(windowStyle, TN_STYLE_CUSTOM_HEADERDRAW))
                {
                    PhSetControlTheme(WindowHandle, L"DarkMode_ItemsView");

                    return CallWindowProc(PhDefaultHeaderWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);
                }
            }
        }
			
		context = PhAllocateZero(sizeof(PHP_THEME_WINDOW_HEADER_CONTEXT));
		context->ThemeHandle = PhOpenThemeData(WindowHandle, VSCLASS_HEADER, PhGetWindowDpi(WindowHandle));
		context->CursorPos.x = LONG_MIN;
		context->CursorPos.y = LONG_MIN;
		PhSetWindowContext(WindowHandle, LONG_MAX, context);

		PhSetControlTheme(WindowHandle, L"DarkMode_ItemsView");

		InvalidateRect(WindowHandle, NULL, FALSE);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, LONG_MAX);
    }

    if (context)
    {
        switch (WindowMessage)
        {
        case WM_NCDESTROY:
            {
                PhRemoveWindowContext(WindowHandle, LONG_MAX);

                if (context->ThemeHandle)
                {
                    PhCloseThemeData(context->ThemeHandle);
                }

                PhFree(context);
            }
            break;
        case WM_THEMECHANGED:
            {
                if (context->ThemeHandle)
                {
                    PhCloseThemeData(context->ThemeHandle);
                    context->ThemeHandle = NULL;
                }

                context->ThemeHandle = PhOpenThemeData(WindowHandle, VSCLASS_HEADER, PhGetWindowDpi(WindowHandle));
            }
            break;
        case WM_ERASEBKGND:
            return TRUE;
        case WM_MOUSEMOVE:
            {
                if (GetCapture() == WindowHandle)
                    break;

                if (!context->MouseActive)
                {
                    TRACKMOUSEEVENT trackEvent =
                    {
                        sizeof(TRACKMOUSEEVENT),
                        TME_LEAVE,
                        WindowHandle,
                        0
                    };

                    TrackMouseEvent(&trackEvent);
                    context->MouseActive = TRUE;
                }

                context->CursorPos.x = GET_X_LPARAM(lParam);
                context->CursorPos.y = GET_Y_LPARAM(lParam);

                InvalidateRect(WindowHandle, NULL, FALSE);
            }
            break;
        case WM_CONTEXTMENU:
            {
                LRESULT result = CallWindowProc(PhDefaultHeaderWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);

                InvalidateRect(WindowHandle, NULL, TRUE);

                return result;
            }
            break;
        case WM_MOUSELEAVE:
            {
                LRESULT result = CallWindowProc(PhDefaultHeaderWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);

                context->MouseActive = FALSE;
                context->CursorPos.x = LONG_MIN;
                context->CursorPos.y = LONG_MIN;

                InvalidateRect(WindowHandle, NULL, TRUE);

                return result;
            }
            break;
        case WM_PAINT:
            {
                // Don't apply header theme for unsupported dialogs: Advanced Security, Digital Signature Details, etc. (Dart Vanya)
                if (!PhIsDarkModeAllowedForWindow_I(GetParent(WindowHandle)))
                {
                    PhRemoveWindowContext(WindowHandle, LONG_MAX);
                    if (context->ThemeHandle)
                        PhCloseThemeData(context->ThemeHandle);
                    PhFree(context);
                    PhSetControlTheme(WindowHandle, L"Explorer");
                    break;
                }

                //PAINTSTRUCT ps;
                //HDC BufferedHDC;
                //HPAINTBUFFER BufferedPaint;
                //
                //if (!BeginPaint(WindowHandle, &ps))
                //    break;
                //
                //DEBUG_BEGINPAINT_RECT(WindowHandle, ps.rcPaint);
                //
                //if (BufferedPaint = BeginBufferedPaint(ps.hdc, &ps.rcPaint, BPBF_COMPATIBLEBITMAP, NULL, &BufferedHDC))
                //{
                //    ThemeWindowRenderHeaderControl(context, WindowHandle, BufferedHDC, &ps.rcPaint, oldWndProc);
                //    EndBufferedPaint(BufferedPaint, TRUE);
                //}
                //else
                {
                    RECT clientRect;
                    HDC hdc;
                    HDC bufferDc;
                    HBITMAP bufferBitmap;
                    HBITMAP oldBufferBitmap;

                    GetClientRect(WindowHandle, &clientRect);

                    hdc = GetDC(WindowHandle);
                    bufferDc = CreateCompatibleDC(hdc);
                    bufferBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
                    oldBufferBitmap = SelectBitmap(bufferDc, bufferBitmap);

                    ThemeWindowRenderHeaderControl(context, WindowHandle, bufferDc, &clientRect);

                    BitBlt(hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom, bufferDc, 0, 0, SRCCOPY);
                    SelectBitmap(bufferDc, oldBufferBitmap);
                    DeleteBitmap(bufferBitmap);
                    DeleteDC(bufferDc);
                    ReleaseDC(WindowHandle, hdc);
                }

                //EndPaint(WindowHandle, &ps);
            }
            goto DefaultWndProc;
        }
    }

    return CallWindowProc(PhDefaultHeaderWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);

DefaultWndProc:
    return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);
}

VOID PhRegisterDialogSuperClass(
    VOID
    )
{
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

    if (!GetClassInfoEx(NULL, L"#32770", &wcex))
        return;

    PhDefaultDialogWindowProcedure = wcex.lpfnWndProc;
    wcex.lpfnWndProc = PhDialogWindowHookProcedure;
    wcex.style = wcex.style | CS_PARENTDC | CS_GLOBALCLASS;

    UnregisterClass(L"#32770", NULL);
    if (RegisterClassEx(&wcex) == INVALID_ATOM)
    {
        PhShowStatus(NULL, L"Unable to register window class.", 0, GetLastError());
    }
}

VOID PhRegisterMenuSuperClass(
    VOID
    )
{
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

    if (!GetClassInfoEx(NULL, L"#32768", &wcex))
        return;

    PhDefaultMenuWindowProcedure = wcex.lpfnWndProc;
    wcex.lpfnWndProc = PhMenuWindowHookProcedure;
    wcex.style = wcex.style | CS_PARENTDC | CS_GLOBALCLASS;

    UnregisterClass(L"#32768", NULL);
    if (RegisterClassEx(&wcex) == INVALID_ATOM)
    {
        PhShowStatus(NULL, L"Unable to register window class.", 0, GetLastError());
    }
}

VOID PhRegisterRebarSuperClass(
    VOID
    )
{
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

    if (!GetClassInfoEx(NULL, REBARCLASSNAME, &wcex))
        return;

    PhDefaultRebarWindowProcedure = wcex.lpfnWndProc;
    wcex.lpfnWndProc = PhRebarWindowHookProcedure;
    wcex.style = wcex.style | CS_PARENTDC | CS_GLOBALCLASS;

    UnregisterClass(REBARCLASSNAME, NULL);
    if (RegisterClassEx(&wcex) == INVALID_ATOM)
    {
        PhShowStatus(NULL, L"Unable to register window class.", 0, GetLastError());
    }
}

VOID PhRegisterComboBoxSuperClass(
    VOID
    )
{
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

    if (!GetClassInfoEx(NULL, WC_COMBOBOX, &wcex))
        return;

    PhDefaultComboBoxWindowProcedure = wcex.lpfnWndProc;
    wcex.lpfnWndProc = PhComboBoxWindowHookProcedure;
    wcex.style = wcex.style | CS_PARENTDC | CS_GLOBALCLASS;

    UnregisterClass(WC_COMBOBOX, NULL);
    if (RegisterClassEx(&wcex) == INVALID_ATOM)
    {
        PhShowStatus(NULL, L"Unable to register window class.", 0, GetLastError());
    }
}

VOID PhRegisterStaticSuperClass(
    VOID
    )
{
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

    if (!GetClassInfoEx(NULL, WC_STATIC, &wcex))
        return;

    PhDefaultStaticWindowProcedure = wcex.lpfnWndProc;
    wcex.lpfnWndProc = PhStaticWindowHookProcedure;
    wcex.style = wcex.style | CS_PARENTDC | CS_GLOBALCLASS;

    UnregisterClass(WC_STATIC, NULL);
    if (RegisterClassEx(&wcex) == INVALID_ATOM)
    {
        PhShowStatus(NULL, L"Unable to register window class.", 0, GetLastError());
    }
}

VOID PhRegisterStatusBarSuperClass(
    VOID
    )
{
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

    if (!GetClassInfoEx(NULL, STATUSCLASSNAME, &wcex))
        return;

    PhDefaultStatusbarWindowProcedure = wcex.lpfnWndProc;
    wcex.lpfnWndProc = PhStatusBarWindowHookProcedure;
    wcex.style = wcex.style | CS_PARENTDC | CS_GLOBALCLASS;

    UnregisterClass(STATUSCLASSNAME, NULL);
    if (RegisterClassEx(&wcex) == INVALID_ATOM)
    {
        PhShowStatus(NULL, L"Unable to register window class.", 0, GetLastError());
    }
}

VOID PhRegisterEditSuperClass(
    VOID
    )
{
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

    if (!GetClassInfoEx(NULL, WC_EDIT, &wcex))
        return;

    PhDefaultEditWindowProcedure = wcex.lpfnWndProc;
    wcex.lpfnWndProc = PhEditWindowHookProcedure;
    wcex.style = wcex.style | CS_PARENTDC | CS_GLOBALCLASS;

    UnregisterClass(WC_EDIT, NULL);
    if (RegisterClassEx(&wcex) == INVALID_ATOM)
    {
        PhShowStatus(NULL, L"Unable to register window class.", 0, GetLastError());
    }
}

VOID PhRegisterHeaderSuperClass(
    VOID
    )
{
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

    if (!GetClassInfoEx(NULL, WC_HEADER, &wcex))
        return;

    PhDefaultHeaderWindowProcedure = wcex.lpfnWndProc;
    wcex.lpfnWndProc = PhHeaderWindowHookProcedure;
    wcex.style = wcex.style | CS_PARENTDC | CS_GLOBALCLASS;

    UnregisterClass(WC_HEADER, NULL);
    if (RegisterClassEx(&wcex) == INVALID_ATOM)
    {
        PhShowStatus(NULL, L"Unable to register window class.", 0, GetLastError());
    }
}

// Detours export procedure hooks

static HRESULT (WINAPI* PhDefaultDrawThemeBackground)(
    _In_ HTHEME Theme,
    _In_ HDC Hdc,
    _In_ INT PartId,
    _In_ INT StateId,
    _In_ LPCRECT Rect,
    _In_ LPCRECT ClipRect
    ) = NULL;

static HRESULT (WINAPI* PhDefaultDrawThemeBackgroundEx)(
    _In_ HTHEME         hTheme,
    _In_ HDC            hdc,
    _In_ int            iPartId,
    _In_ int            iStateId,
    _In_ LPCRECT        pRect,
    _In_ const DTBGOPTS* pOptions
    ) = NULL;

static HRESULT(WINAPI* PhDefaultDrawThemeText)(
    _In_ HTHEME  hTheme,
    _In_ HDC     hdc,
    _In_ int     iPartId,
    _In_ int     iStateId,
    _In_ LPCWSTR pszText,
    _In_ int     cchText,
    _In_ DWORD   dwTextFlags,
    _In_ DWORD   dwTextFlags2,
    _In_ LPCRECT pRect
    ) = NULL;

static HRESULT(WINAPI* PhDefaultDrawThemeTextEx)(
    _In_      HTHEME        hTheme,
    _In_      HDC           hdc,
    _In_      int           iPartId,
    _In_      int           iStateId,
    _In_      LPCWSTR       pszText,
    _In_      int           cchText,
    _In_      DWORD         dwTextFlags,
    _Inout_   LPRECT        pRect,
    _In_      const DTTOPTS* pOptions
    ) = NULL;

static HRESULT(WINAPI* PhDefaultTaskDialogIndirect)(
    _In_      const TASKDIALOGCONFIG* pTaskConfig,
    _Out_opt_ int* pnButton,
    _Out_opt_ int* pnRadioButton,
    _Out_opt_ BOOL* pfVerificationFlagChecked
    ) = NULL;

static HRESULT(WINAPI* PhDefaultGetThemeColor)(
    _In_  HTHEME   hTheme,
    _In_  int      iPartId,
    _In_  int      iStateId,
    _In_  int      iPropId,
    _Out_ COLORREF* pColor
    ) = NULL;

// uxtheme.dll ordinal 49
static HTHEME(WINAPI* PhDefaultOpenNcThemeData)(
    _In_ HWND    hwnd,
    _In_ LPCWSTR pszClassList
    ) = NULL;

static BOOL (WINAPI* PhDefaultSystemParametersInfo)(
    _In_ UINT uiAction,
    _In_ UINT uiParam,
    _Pre_maybenull_ _Post_valid_ PVOID pvParam,
    _In_ UINT fWinIni
    ) = NULL;

static HWND (WINAPI* PhDefaultCreateWindowEx)(
    _In_ ULONG ExStyle,
    _In_opt_ PCWSTR ClassName,
    _In_opt_ PCWSTR WindowName,
    _In_ ULONG Style,
    _In_ INT X,
    _In_ INT Y,
    _In_ INT Width,
    _In_ INT Height,
    _In_opt_ HWND Parent,
    _In_opt_ HMENU Menu,
    _In_opt_ PVOID Instance,
    _In_opt_ PVOID Param
    ) = NULL;

typedef struct _TASKDIALOG_CALLBACK_WRAP
{
    PFTASKDIALOGCALLBACK pfCallback;
    LONG_PTR lpCallbackData;
} TASKDIALOG_CALLBACK_WRAP, * PTASKDIALOG_CALLBACK_WRAP;

typedef struct _TASKDIALOG_COMMON_CONTEXT
{
    WNDPROC DefaultWindowProc;
    ULONG Painting;
} TASKDIALOG_COMMON_CONTEXT, * PTASKDIALOG_COMMON_CONTEXT;

typedef struct _TASKDIALOG_WINDOW_CONTEXT
{
    WNDPROC DefaultWindowProc;
    ULONG Painting;
    PTASKDIALOG_CALLBACK_WRAP CallbackData;
} TASKDIALOG_WINDOW_CONTEXT, * PTASKDIALOG_WINDOW_CONTEXT;

#define GETCLASSNAME_OR_NULL(WindowHandle, ClassName) if (!GetClassName(WindowHandle, ClassName, RTL_NUMBER_OF(ClassName))) ClassName[0] = UNICODE_NULL

HRESULT CALLBACK ThemeTaskDialogCallbackHook(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
);

LRESULT CALLBACK ThemeTaskDialogMasterSubclass(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
);

BOOLEAN CALLBACK PhInitializeTaskDialogTheme(
    _In_ HWND hwndDlg,
    _In_opt_ PVOID Context
);

HRESULT PhDrawThemeBackgroundHook(
    _In_ HTHEME Theme,
    _In_ HDC Hdc,
    _In_ INT PartId,
    _In_ INT StateId,
    _In_ LPCRECT Rect,
    _In_ LPCRECT ClipRect
    )
{
    WCHAR className[MAX_PATH];
    BOOLEAN hasThemeClass = PhGetThemeClass(Theme, className, RTL_NUMBER_OF(className));

    if (WindowsVersion >= WINDOWS_11)
    {
        if (hasThemeClass)
        {
            if (PhEqualStringZ(className, VSCLASS_MENU, TRUE))
            {
                if (PartId == MENU_POPUPGUTTER || PartId == MENU_POPUPBORDERS)
                {
                    FillRect(Hdc, Rect, PhThemeWindowBackgroundBrush);
                    return S_OK;
                }
            }
        }
    }

    if (hasThemeClass)
    {
        if (PhEqualStringZ(className, VSCLASS_PROGRESS, TRUE))
        {
            if (PartId == PP_TRANSPARENTBAR) // Progress bar background.
            {
                FillRect(Hdc, Rect, PhThemeWindowBackgroundBrush);
                SetDCBrushColor(Hdc, RGB(0x60, 0x60, 0x60));
                FrameRect(Hdc, Rect, PhGetStockBrush(DC_BRUSH));
                return S_OK;
            }
        }
    }

    return PhDefaultDrawThemeBackground(Theme, Hdc, PartId, StateId, Rect, ClipRect);
}

HRESULT WINAPI PhDrawThemeBackgroundExHook(
    _In_ HTHEME         hTheme,
    _In_ HDC            hdc,
    _In_ int            iPartId,
    _In_ int            iStateId,
    _In_ LPCRECT        pRect,
    _In_ const DTBGOPTS* pOptions
)
{
    WCHAR className[MAX_PATH];

    // Apply theme to ListView checkboxes
    if (iPartId == BP_CHECKBOX /*|| iPartId == BP_RADIOBUTTON*/)
    {
        if (PhGetThemeClass(hTheme, className, RTL_NUMBER_OF(className)) &&
            PhEqualStringZ(className, VSCLASS_BUTTON, TRUE))
        {
            HTHEME darkButtonTheme = PhOpenThemeData(NULL, L"DarkMode_Explorer::Button", 0);
            HRESULT retVal = PhDefaultDrawThemeBackgroundEx(darkButtonTheme ? darkButtonTheme : hTheme, hdc, iPartId, iStateId, pRect, pOptions);
            if (darkButtonTheme)
                PhCloseThemeData(darkButtonTheme);
            return retVal;
        }
    }

    // Micro optimization
    if ((iPartId == TDLG_PRIMARYPANEL || iPartId == TDLG_FOOTNOTEPANE || iPartId == TDLG_SECONDARYPANEL || iPartId == TDLG_FOOTNOTESEPARATOR || iPartId == TDLG_EXPANDOBUTTON) &&
        PhGetThemeClass(hTheme, className, RTL_NUMBER_OF(className)) &&
        PhEqualStringZ(className, VSCLASS_TASKDIALOG, TRUE))
    {
        switch (iPartId)
        {
        case TDLG_PRIMARYPANEL:
            SetDCBrushColor(hdc, PhThemeWindowBackground2Color);
            FillRect(hdc, pRect, PhGetStockBrush(DC_BRUSH));
            return S_OK;
        case TDLG_FOOTNOTEPANE:
            FillRect(hdc, pRect, PhThemeWindowBackgroundBrush);
            return S_OK;
        case TDLG_SECONDARYPANEL:
        {
            FillRect(hdc, pRect, PhThemeWindowBackgroundBrush);
            RECT rect = *pRect;
            rect.bottom = rect.top + 1;
            SetDCBrushColor(hdc, PhThemeWindowForegroundColor);
            FillRect(hdc, &rect, PhGetStockBrush(DC_BRUSH));
            PhOffsetRect(&rect, 0, 1);
            SetDCBrushColor(hdc, PhThemeWindowBackground2Color);
            FillRect(hdc, &rect, PhGetStockBrush(DC_BRUSH));
            return S_OK;
        }
        case TDLG_FOOTNOTESEPARATOR:
        {
            SetDCBrushColor(hdc, PhThemeWindowForegroundColor);
            FillRect(hdc, pRect, PhGetStockBrush(DC_BRUSH));
            RECT rect = *pRect;
            rect.top += 1;
            SetDCBrushColor(hdc, PhThemeWindowBackground2Color);
            FillRect(hdc, &rect, PhGetStockBrush(DC_BRUSH));
            return S_OK;
        }
        case TDLG_EXPANDOBUTTON:
            {
                // In Windows 11, buttons lack background, making them indistinguishable on dark backgrounds.
                // To address this, we invert the button. This technique isn't applicable to Windows 10 as it causes the button's border to appear chipped.
                static enum { yes, no, unknown } mustInvertButton = unknown;
                if (mustInvertButton == unknown)
                {
                    PhDefaultDrawThemeBackgroundEx(hTheme, hdc, iPartId, iStateId, pRect, pOptions);
                    int buttonCenterX = pOptions->rcClip.left + (pOptions->rcClip.right - pOptions->rcClip.left) / 2;
                    int buttonCenterY = pOptions->rcClip.top + (pOptions->rcClip.bottom - pOptions->rcClip.top) / 2;
                    COLORREF centerPixel = GetPixel(hdc, buttonCenterX, buttonCenterY);
                    mustInvertButton = centerPixel == PhThemeWindowTextColor ? no : yes;
                }
                FillRect(hdc, pRect, PhThemeWindowBackgroundBrush);
                if (mustInvertButton == yes) InvertRect(hdc, pRect);
                HRESULT retVal = PhDefaultDrawThemeBackgroundEx(hTheme, hdc, iPartId, iStateId, pRect, pOptions);
                if (mustInvertButton == yes) InvertRect(hdc, pRect);
                return retVal;
            }
        }
    }

    return PhDefaultDrawThemeBackgroundEx(hTheme, hdc, iPartId, iStateId, pRect, pOptions);
}

HWND PhCreateWindowExHook(
    _In_ ULONG ExStyle,
    _In_opt_ PCWSTR ClassName,
    _In_opt_ PCWSTR WindowName,
    _In_ ULONG Style,
    _In_ INT X,
    _In_ INT Y,
    _In_ INT Width,
    _In_ INT Height,
    _In_opt_ HWND Parent,
    _In_opt_ HMENU Menu,
    _In_opt_ PVOID Instance,
    _In_opt_ PVOID Param
    )
{
    HWND windowHandle = PhDefaultCreateWindowEx(
        ExStyle,
        ClassName,
        WindowName,
        Style,
        X,
        Y,
        Width,
        Height,
        Parent,
        Menu,
        Instance,
        Param
        );

    if (Parent == NULL)
    {
        if (PhDefaultEnableStreamerMode)
        {
            if (SetWindowDisplayAffinity_Import())
                SetWindowDisplayAffinity_Import()(windowHandle, WDA_EXCLUDEFROMCAPTURE);
        }

        if (PhEnableThemeSupport && PhDefaultEnableThemeAcrylicWindowSupport)
        {
            PhSetWindowAcrylicCompositionColor(windowHandle, MakeABGRFromCOLORREF(0, RGB(10, 10, 10)));
        }
    }
    else if (PhEnableThemeSupport)
    {
        // Early subclassing of the SysLink control to eliminate blinking during page switches.
        if (!IS_INTRESOURCE(ClassName) && PhEqualStringZ((PWSTR)ClassName, WC_LINK, TRUE))
        {
            PhInitializeTaskDialogTheme(windowHandle, 0);
        }
        else if (!IS_INTRESOURCE(ClassName) && PhEqualStringZ((PWSTR)ClassName, WC_BUTTON, TRUE) &&
            PhGetWindowContext(GetAncestor(Parent, GA_ROOT), LONG_MAX))
        {
            PhSetControlTheme(windowHandle, L"DarkMode_Explorer");
        }
    }

    return windowHandle;
}

BOOL WINAPI PhSystemParametersInfoHook(
    _In_ UINT uiAction,
    _In_ UINT uiParam,
    _Pre_maybenull_ _Post_valid_ PVOID pvParam,
    _In_ UINT fWinIni
    )
{
    if (uiAction == SPI_GETMENUFADE && pvParam)
    {
        *((PBOOL)pvParam) = FALSE;
        return TRUE;
    }

    if (uiAction == SPI_GETCLIENTAREAANIMATION && pvParam)
    {
        *((PBOOL)pvParam) = FALSE;
        return TRUE;
    }

    if (uiAction == SPI_GETCOMBOBOXANIMATION && pvParam)
    {
        *((PBOOL)pvParam) = FALSE;
        return TRUE;
    }

    if (uiAction == SPI_GETTOOLTIPANIMATION && pvParam)
    {
        *((PBOOL)pvParam) = FALSE;
        return TRUE;
    }

    if (uiAction == SPI_GETMENUANIMATION && pvParam)
    {
        *((PBOOL)pvParam) = FALSE;
        return TRUE;
    }

    if (uiAction == SPI_GETTOOLTIPFADE && pvParam)
    {
        *((PBOOL)pvParam) = FALSE;
        return TRUE;
    }

    if (uiAction == SPI_GETMOUSEVANISH && pvParam)
    {
        *((PBOOL)pvParam) = FALSE;
        return TRUE;
    }

    return PhDefaultSystemParametersInfo(uiAction, uiParam, pvParam, fWinIni);
}

//ULONG WINAPI GetSysColorHook(int nIndex)
//{
//    if (nIndex == COLOR_WINDOW)
//        return PhThemeWindowTextColor;
//    if (nIndex == COLOR_MENUTEXT)
//        return PhThemeWindowTextColor;
//    if (nIndex == COLOR_WINDOWTEXT)
//        return PhThemeWindowTextColor;
//    if (nIndex == COLOR_CAPTIONTEXT)
//        return PhThemeWindowTextColor;
//    if (nIndex == COLOR_HIGHLIGHTTEXT)
//        return PhThemeWindowTextColor;
//    if (nIndex == COLOR_GRAYTEXT)
//        return PhThemeWindowTextColor;
//    if (nIndex == COLOR_BTNTEXT)
//        return PhThemeWindowTextColor;
//    if (nIndex == COLOR_INACTIVECAPTIONTEXT)
//        return PhThemeWindowTextColor;
//    if (nIndex == COLOR_BTNFACE)
//        return PhThemeWindowBackgroundColor;
//    if (nIndex == COLOR_BTNTEXT)
//        return PhThemeWindowTextColor;
//    if (nIndex == COLOR_BTNHIGHLIGHT)
//        return PhThemeWindowBackgroundColor;
//    if (nIndex == COLOR_INFOTEXT)
//        return PhThemeWindowTextColor;
//    if (nIndex == COLOR_INFOBK)
//        return PhThemeWindowBackgroundColor;
//    if (nIndex == COLOR_MENU)
//        return PhThemeWindowBackgroundColor;
//    if (nIndex == COLOR_HIGHLIGHT)
//        return PhThemeWindowForegroundColor;
//    return originalGetSysColor(nIndex);
//}
//
//HBRUSH WINAPI GetSysColorBrushHook(_In_ int nIndex)
//{
//    //if (nIndex == COLOR_WINDOW)
//    //   return originalCreateSolidBrush(PhThemeWindowBackgroundColor);
//    if (nIndex == COLOR_BTNFACE)
//        return originalCreateSolidBrush(PhThemeWindowBackgroundColor);
//    return originalHook(nIndex);
//}
//
// RGB(GetBValue(color), GetGValue(color), GetRValue(color));
//#define RGB_FROM_COLOREF(cref) \
//    ((((cref) & 0x000000FF) << 16) | (((cref) & 0x0000FF00)) | (((cref) & 0x00FF0000) >> 16))

HRESULT WINAPI PhDrawThemeTextHook(
    _In_ HTHEME  hTheme,
    _In_ HDC     hdc,
    _In_ int     iPartId,
    _In_ int     iStateId,
    _In_ LPCWSTR pszText,
    _In_ int     cchText,
    _In_ DWORD   dwTextFlags,
    _In_ DWORD   dwTextFlags2,
    _In_ LPCRECT pRect
    )
{
    if (iPartId == BP_COMMANDLINK && iStateId != PBS_DISABLED)
    {
        WCHAR className[MAX_PATH];
        if (PhGetThemeClass(hTheme, className, RTL_NUMBER_OF(className)) &&
            PhEqualStringZ(className, VSCLASS_BUTTON, TRUE))
        {
            DTTOPTS options = { sizeof(DTTOPTS), DTT_TEXTCOLOR, PhThemeWindowTextColor };
            return PhDefaultDrawThemeTextEx(hTheme, hdc, iPartId, iStateId, pszText, cchText, dwTextFlags, (LPRECT)pRect, &options);
        }
    }

    return PhDefaultDrawThemeText(hTheme, hdc, iPartId, iStateId, pszText, cchText, dwTextFlags, dwTextFlags2, pRect);
}

HRESULT WINAPI PhDrawThemeTextExHook(
    _In_      HTHEME        hTheme,
    _In_      HDC           hdc,
    _In_      int           iPartId,
    _In_      int           iStateId,
    _In_      LPCWSTR       pszText,
    _In_      int           cchText,
    _In_      DWORD         dwTextFlags,
    _Inout_   LPRECT        pRect,
    _In_      const DTTOPTS* pOptions
    )
{
    if (iPartId == BP_COMMANDLINK)
    {
        WCHAR className[MAX_PATH];
        if (PhGetThemeClass(hTheme, className, RTL_NUMBER_OF(className)) &&
            PhEqualStringZ(className, VSCLASS_BUTTON, TRUE))
        {
            DTTOPTS options = { sizeof(DTTOPTS) };
            if (pOptions)
                options = *pOptions;
            options.dwFlags |= DTT_TEXTCOLOR;
            PhDefaultGetThemeColor(hTheme, iPartId, iStateId, TMT_TEXTCOLOR, &options.crText);
            options.crText = PhMakeColorBrighter(options.crText, 90);
            return PhDefaultDrawThemeTextEx(hTheme, hdc, iPartId, iStateId, pszText, cchText, dwTextFlags, pRect, &options);
        }
    }

    return PhDefaultDrawThemeTextEx(hTheme, hdc, iPartId, iStateId, pszText, cchText, dwTextFlags, pRect, pOptions);
}

HRESULT PhGetThemeColorHook(
    _In_  HTHEME   hTheme,
    _In_  int      iPartId,
    _In_  int      iStateId,
    _In_  int      iPropId,
    _Out_ COLORREF* pColor
    )
{
    WCHAR className[MAX_PATH];

    HRESULT retVal = PhDefaultGetThemeColor(hTheme, iPartId, iStateId, iPropId, pColor);

    if (iPropId == TMT_TEXTCOLOR && iPartId == TDLG_MAININSTRUCTIONPANE)
    {
        if (PhGetThemeClass(hTheme, className, RTL_NUMBER_OF(className)) &&
            PhEqualStringZ(className, VSCLASS_TASKDIALOGSTYLE, TRUE))
        {
            *pColor = PhMakeColorBrighter(*pColor, 150); // Main header.
        }
    }
    else if (iPropId == TMT_TEXTCOLOR)
    {
        if (PhGetThemeClass(hTheme, className, RTL_NUMBER_OF(className)) &&
            PhEqualStringZ(className, VSCLASS_TASKDIALOGSTYLE, TRUE))
        {
            *pColor = PhThemeWindowTextColor; // Text color for check boxes, expanded text, and expander button text.
        }  
    }

    return retVal;
}

HTHEME PhOpenNcThemeDataHook(
    _In_ HWND    hwnd,
    _In_ LPCWSTR pszClassList
)
{
    if (PhEqualStringZ((PWSTR)pszClassList, VSCLASS_SCROLLBAR, TRUE) &&
        PhIsDarkModeAllowedForWindow_I(hwnd))
    {
        return PhDefaultOpenNcThemeData(NULL, L"Explorer::ScrollBar");
    }

    return PhDefaultOpenNcThemeData(hwnd, pszClassList);
}

BOOLEAN CALLBACK PhInitializeTaskDialogTheme(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID CallbackData
)
{
    WCHAR windowClassName[MAX_PATH];
    PTASKDIALOG_COMMON_CONTEXT context;
    BOOLEAN windowHasContext = !!PhGetWindowContext(WindowHandle, (ULONG)'TDLG');

    if (CallbackData && !windowHasContext)
    {
        if (PhDefaultEnableStreamerMode)
        {
            if (SetWindowDisplayAffinity_Import())
                SetWindowDisplayAffinity_Import()(WindowHandle, WDA_EXCLUDEFROMCAPTURE);
        }

        PhInitializeThemeWindowFrame(WindowHandle);

        PTASKDIALOG_WINDOW_CONTEXT context = PhAllocateZero(sizeof(TASKDIALOG_WINDOW_CONTEXT));
        context->DefaultWindowProc = PhSetWindowProcedure(WindowHandle, ThemeTaskDialogMasterSubclass);
        context->CallbackData = CallbackData;
        PhSetWindowContext(WindowHandle, (ULONG)'TDLG', context);
        windowHasContext = TRUE;
    }

    PhEnumChildWindows(
        WindowHandle,
        0x1000,
        PhInitializeTaskDialogTheme,
        NULL
    );

    if (windowHasContext)    // HACK
        return TRUE;

    GETCLASSNAME_OR_NULL(WindowHandle, windowClassName);

    context = PhAllocateZero(sizeof(TASKDIALOG_COMMON_CONTEXT));
    context->DefaultWindowProc = PhSetWindowProcedure(WindowHandle, ThemeTaskDialogMasterSubclass);
    PhSetWindowContext(WindowHandle, (ULONG)'TDLG', context);

    if (PhEqualStringZ(windowClassName, WC_BUTTON, FALSE) ||
        PhEqualStringZ(windowClassName, WC_SCROLLBAR, FALSE))
    {
        PhSetControlTheme(WindowHandle, L"DarkMode_Explorer");
    }
    else if (PhEqualStringZ(windowClassName, L"DirectUIHWND", FALSE))
    {
        //WINDOWPLACEMENT pos = { 0 };
        //GetWindowPlacement(GetParent(WindowHandle), &pos);
        PhSetControlTheme(WindowHandle, L"DarkMode_Explorer");
        //SetWindowPlacement(GetParent(WindowHandle), &pos);
    }

    return TRUE;
}

LRESULT CALLBACK ThemeTaskDialogMasterSubclass(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LRESULT result;
    PTASKDIALOG_COMMON_CONTEXT context;
    WNDPROC OldWndProc;

    if (!(context = PhGetWindowContext(hwnd, (ULONG)'TDLG')))
        return 0;

    OldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_ERASEBKGND:
        {
            HDC hdc = (HDC)wParam;
            RECT rect;
            WCHAR windowClassName[MAX_PATH];

            SetTextColor(hdc, PhThemeWindowTextColor); // Color for SysLink, which must be set in its parent.

            if (!context->Painting)
            {
                GETCLASSNAME_OR_NULL(hwnd, windowClassName);
                // Avoid erasing the background for links, as they will blink white on the extender and during page switches.
                if (!PhEqualStringZ(windowClassName, WC_LINK, FALSE))
                {
                    GetClipBox(hdc, &rect);
                    SetDCBrushColor(hdc, PhThemeWindowBackground2Color);
                    FillRect(hdc, &rect, PhGetStockBrush(DC_BRUSH));
                }
                else
                {
                    PhInitializeSysLinkTheme(hwnd);     // this doesn't work in EnumWindows callback, I don't know why
                }
            }
        }
        return TRUE;
    case WM_NOTIFY:
        {
            LPNMHDR data = (LPNMHDR)lParam;

            if (data->code == NM_CUSTOMDRAW)
            {
                LPNMCUSTOMDRAW customDraw = (LPNMCUSTOMDRAW)lParam;
                WCHAR className[MAX_PATH];

                if (!GetClassName(customDraw->hdr.hwndFrom, className, RTL_NUMBER_OF(className)))
                    className[0] = UNICODE_NULL;
                if (PhEqualStringZ(className, WC_BUTTON, FALSE))
                {
                    return PhThemeWindowDrawButton(customDraw);
                }
            }
        }
        break;
    case TDM_NAVIGATE_PAGE:
        {
            PTASKDIALOG_WINDOW_CONTEXT WindowContext = (PTASKDIALOG_WINDOW_CONTEXT)context;
            PTASKDIALOGCONFIG trueConfig = (PTASKDIALOGCONFIG)lParam;
            PTASKDIALOGCONFIG myConfig;
            TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };

            WindowContext->CallbackData->pfCallback = trueConfig ? trueConfig->pfCallback : NULL;
            WindowContext->CallbackData->lpCallbackData = trueConfig ? trueConfig->lpCallbackData : 0;
            myConfig = trueConfig ? trueConfig : &config;
            myConfig->pfCallback = ThemeTaskDialogCallbackHook;
            myConfig->lpCallbackData = (LONG_PTR)WindowContext->CallbackData;

            return CallWindowProc(OldWndProc, hwnd, uMsg, wParam, (LPARAM)myConfig);
        }
    case WM_DESTROY:
        {
            PhSetWindowProcedure(hwnd, OldWndProc);
            PhRemoveWindowContext(hwnd, (ULONG)'TDLG');
            PhFree(context);
        }
        return CallWindowProc(OldWndProc, hwnd, uMsg, wParam, lParam);
    case WM_CTLCOLORDLG:
        return (LRESULT)PhThemeWindowBackgroundBrush; // Window background color when the extender resizes upward (Windows 10 only).
    }

    context->Painting++;
    result = CallWindowProc(OldWndProc, hwnd, uMsg, wParam, lParam);
    context->Painting--;
    return result;
}

HRESULT CALLBACK ThemeTaskDialogCallbackHook(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    HRESULT result = S_OK;

    PTASKDIALOG_CALLBACK_WRAP CallbackData = (PTASKDIALOG_CALLBACK_WRAP)dwRefData;

    if (uMsg == TDN_DIALOG_CONSTRUCTED) // Called on each new page, including the first one.
    {
        PhInitializeTaskDialogTheme(hwndDlg, CallbackData);
    }

    if (CallbackData->pfCallback)
        result = CallbackData->pfCallback(hwndDlg, uMsg, wParam, lParam, CallbackData->lpCallbackData);

    return result;
}

// https://github.com/SFTRS/DarkTaskDialog
HRESULT PhTaskDialogIndirectHook(
    _In_      const TASKDIALOGCONFIG* pTaskConfig,
    _Out_opt_ int* pnButton,
    _Out_opt_ int* pnRadioButton,
    _Out_opt_ BOOL* pfVerificationFlagChecked
    )
{
    TASKDIALOG_CALLBACK_WRAP CallbackData;
    CallbackData.pfCallback = pTaskConfig->pfCallback;
    CallbackData.lpCallbackData = pTaskConfig->lpCallbackData;
    TASKDIALOGCONFIG myConfig = *pTaskConfig;
    myConfig.pfCallback = ThemeTaskDialogCallbackHook;
    myConfig.lpCallbackData = (LONG_PTR)&CallbackData;

    return PhDefaultTaskDialogIndirect(&myConfig, pnButton, pnRadioButton, pfVerificationFlagChecked);
}

VOID PhRegisterDetoursHooks(
    VOID
    )
{
    NTSTATUS status;
    PVOID baseAddress;

    // For early TaskDialog with PhStartupParameters.ShowOptions
    if (!PhThemeWindowBackgroundBrush)
    {
        PhThemeWindowBackgroundBrush = CreateSolidBrush(PhThemeWindowBackgroundColor);
    }

    if (baseAddress = PhGetLoaderEntryDllBaseZ(L"user32.dll"))
    {
        PhDefaultCreateWindowEx = PhGetDllBaseProcedureAddress(baseAddress, "CreateWindowExW", 0);
        PhDefaultSystemParametersInfo = PhGetDllBaseProcedureAddress(baseAddress, "SystemParametersInfoW", 0);
    }

    if (baseAddress = PhGetLoaderEntryDllBaseZ(L"uxtheme.dll"))
    {
        PhDefaultDrawThemeBackground = PhGetDllBaseProcedureAddress(baseAddress, "DrawThemeBackground", 0);
        PhDefaultDrawThemeBackgroundEx = PhGetDllBaseProcedureAddress(baseAddress, "DrawThemeBackgroundEx", 0);
        PhDefaultDrawThemeText = PhGetDllBaseProcedureAddress(baseAddress, "DrawThemeText", 0);
        PhDefaultDrawThemeTextEx = PhGetDllBaseProcedureAddress(baseAddress, "DrawThemeTextEx", 0);
        PhDefaultGetThemeColor = PhGetDllBaseProcedureAddress(baseAddress, "GetThemeColor", 0);
        PhIsDarkModeAllowedForWindow_I = PhGetDllBaseProcedureAddress(baseAddress, NULL, 137);
        PhDefaultOpenNcThemeData = PhGetDllBaseProcedureAddress(baseAddress, NULL, 49);
    }

    if (baseAddress = PhGetLoaderEntryDllBaseZ(L"Comctl32.dll"))
    {
        PhDefaultTaskDialogIndirect = PhGetDllBaseProcedureAddress(baseAddress, "TaskDialogIndirect", 0);
    }

    if (!NT_SUCCESS(status = DetourTransactionBegin()))
        goto CleanupExit;

    if (PhEnableThemeSupport || PhEnableThemeAcrylicSupport)
    {
        if (!NT_SUCCESS(status = DetourAttach((PVOID)&PhDefaultDrawThemeBackground, (PVOID)PhDrawThemeBackgroundHook)))
            goto CleanupExit;
        if (!NT_SUCCESS(status = DetourAttach((PVOID)&PhDefaultDrawThemeBackgroundEx, (PVOID)PhDrawThemeBackgroundExHook)))
            goto CleanupExit;
        if (!PhDefaultEnableThemeAnimation)
        {
            if (!NT_SUCCESS(status = DetourAttach((PVOID)&PhDefaultSystemParametersInfo, (PVOID)PhSystemParametersInfoHook)))
                goto CleanupExit;
        }
        if (!NT_SUCCESS(status = DetourAttach((PVOID)&PhDefaultDrawThemeText, (PVOID)PhDrawThemeTextHook)))
            goto CleanupExit;
        if (!NT_SUCCESS(status = DetourAttach((PVOID)&PhDefaultDrawThemeTextEx, (PVOID)PhDrawThemeTextExHook)))
            goto CleanupExit;
        if (!NT_SUCCESS(status = DetourAttach((PVOID)&PhDefaultGetThemeColor, (PVOID)PhGetThemeColorHook)))
            goto CleanupExit;
        if (!NT_SUCCESS(status = DetourAttach((PVOID)&PhDefaultOpenNcThemeData, (PVOID)PhOpenNcThemeDataHook)))
            goto CleanupExit;
        if (!NT_SUCCESS(status = DetourAttach((PVOID)&PhDefaultTaskDialogIndirect, (PVOID)PhTaskDialogIndirectHook)))
            goto CleanupExit;
    }

    if (!NT_SUCCESS(status = DetourAttach((PVOID)&PhDefaultCreateWindowEx, (PVOID)PhCreateWindowExHook)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = DetourTransactionCommit()))
        goto CleanupExit;

CleanupExit:

    if (!NT_SUCCESS(status))
    {
        PhShowStatus(NULL, L"Unable to commit detours transaction.", status, 0);
    }
}

BOOLEAN PhIsThemeTransparencyEnabled(
    VOID
    )
{
    static PH_STRINGREF themesKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize");
    BOOLEAN themesEnableTransparency = FALSE;
    HANDLE keyHandle;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_QUERY_VALUE,
        PH_KEY_CURRENT_USER,
        &themesKeyName,
        0
        )))
    {
        themesEnableTransparency = !!PhQueryRegistryUlongZ(keyHandle, L"EnableTransparency");
        NtClose(keyHandle);
    }

    return themesEnableTransparency;
}

VOID PvInitializeSuperclassControls(
    VOID
    )
{
    PhDefaultEnableStreamerMode = !!PhGetIntegerSetting(L"EnableStreamerMode");

    if (PhEnableThemeAcrylicSupport && !PhEnableThemeSupport)
        PhEnableThemeAcrylicSupport = FALSE;
    if (PhEnableThemeAcrylicSupport)
        PhEnableThemeAcrylicSupport = PhIsThemeTransparencyEnabled();

    if (PhEnableThemeSupport || PhDefaultEnableStreamerMode)
    {
        if (WindowsVersion >= WINDOWS_11)
        {
            PhDefaultEnableThemeAcrylicWindowSupport = !!PhGetIntegerSetting(L"EnableThemeAcrylicWindowSupport");
        }

        PhDefaultEnableThemeAnimation = !!PhGetIntegerSetting(L"EnableThemeAnimation");
        PhEnableThemeNativeButtons = !!PhGetIntegerSetting(L"EnableThemeNativeButtons");

        PhRegisterDialogSuperClass();
        PhRegisterMenuSuperClass();
        PhRegisterRebarSuperClass();
        PhRegisterComboBoxSuperClass();
        PhRegisterStaticSuperClass();
        PhRegisterStatusBarSuperClass();
        PhRegisterEditSuperClass();
        PhRegisterHeaderSuperClass();

        PhRegisterDetoursHooks();
    }
}
