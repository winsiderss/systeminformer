/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex         2022-2023
 *     Dart Vanya   2024
 *
 */

#include <ph.h>
#include <guisup.h>
#include <treenew.h>
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

            if (PhEnableStreamerMode)
            {
                SetWindowDisplayAffinity(WindowHandle, WDA_EXCLUDEFROMCAPTURE);
            }

            if (PhEnableThemeSupport)
            {
                if (PhEnableThemeAcrylicSupport)
                {
                    // Note: DWM crashes if called from WM_NCCREATE (dmex)
                    PhSetWindowAcrylicCompositionColor(WindowHandle, MakeABGRFromCOLORREF(0, RGB(10, 10, 10)), TRUE);
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
                if (PhEnableStreamerMode)
                {
                    SetWindowDisplayAffinity(WindowHandle, WDA_EXCLUDEFROMCAPTURE);
                }

                if (PhEnableThemeSupport && PhEnableThemeAcrylicWindowSupport)
                {
                    // Note: DWM crashes if called from WM_NCCREATE (dmex)
                    PhSetWindowAcrylicCompositionColor(WindowHandle, MakeABGRFromCOLORREF(0, RGB(10, 10, 10)), TRUE);
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
        if (PhEnableThemeSupport)
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
        if (PhEnableStreamerMode)
        {
            //CREATESTRUCT* createStruct = (CREATESTRUCT*)lParam;
            COMBOBOXINFO info = { sizeof(COMBOBOXINFO) };

            if (SendMessage(WindowHandle, CB_GETCOMBOBOXINFO, 0, (LPARAM)&info) && info.hwndList)
            {
                if (SetWindowDisplayAffinity_Import())
                    SetWindowDisplayAffinity_Import()(info.hwndList, WDA_EXCLUDEFROMCAPTURE);
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
    if (WindowMessage == WM_NCCREATE)
    {
        CREATESTRUCT* createStruct = (CREATESTRUCT*)lParam;
        WCHAR windowClassName[MAX_PATH];
        LONG_PTR style = PhGetWindowStyle(WindowHandle);

        if (createStruct->hwndParent)
        {
            GETCLASSNAME_OR_NULL(createStruct->hwndParent, windowClassName);

            if (PhEqualStringZ(windowClassName, PH_TREENEW_CLASSNAME, FALSE))
            {
                PhSetWindowContext(WindowHandle, SHRT_MAX, (PVOID)'TNSB');
            }
            else if (PhEqualStringZ(windowClassName, L"CHECKLIST_ACLUI", FALSE))
            {
                PhSetWindowContext(WindowHandle, SHRT_MAX, (PVOID)'ACLC');
            }
        }
    }

    LONG contextTag = HandleToLong(PhGetWindowContext(WindowHandle, SHRT_MAX));

    if (!(contextTag))
        return CallWindowProc(PhDefaultStaticWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);

    switch (WindowMessage)
    {
    case WM_NCDESTROY:
        PhRemoveWindowContext(WindowHandle, SCHAR_MAX);
        break;
    case WM_ERASEBKGND:
        if (contextTag == 'TNSB')
        {
            HDC hdc = (HDC)wParam;
            RECT rect;

            GetClipBox(hdc, &rect);
            SetDCBrushColor(hdc, PhEnableThemeSupport ? RGB(23, 23, 23) : GetSysColor(COLOR_BTNFACE));
            FillRect(hdc, &rect, PhGetStockBrush(DC_BRUSH));
            return TRUE;
        }
        break;
    case WM_KILLFOCUS:
        if (contextTag == 'ACLC')
        {
            RECT rectClient;
            HWND ParentHandle = GetParent(WindowHandle);

            GetClientRect(WindowHandle, &rectClient);
            PhInflateRect(&rectClient, 2, 2);
            MapWindowRect(WindowHandle, ParentHandle, &rectClient);
            InvalidateRect(ParentHandle, &rectClient, TRUE);     // fix the annoying white border left by the previous active control
        }
        break;
    case WM_PAINT:
        {
            HICON iconHandle;
            PAINTSTRUCT ps;
            RECT clientRect;

            switch (contextTag)
            {
            case 'TNSB':
                return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);
            case 'ACLC':
                if (PhEnableThemeSupport &&
                    (iconHandle = (HICON)(UINT_PTR)CallWindowProc(PhDefaultStaticWindowProcedure, WindowHandle, STM_GETICON, 0, 0)))
                {
                    static PH_INITONCE initOnce = PH_INITONCE_INIT;
                    static HFONT hCheckFont = NULL;

                    HDC hdc = BeginPaint(WindowHandle, &ps);
                    GetClientRect(WindowHandle, &clientRect);

                    HDC bufferDc = CreateCompatibleDC(hdc);
                    HBITMAP bufferBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
                    HBITMAP oldBufferBitmap = SelectBitmap(bufferDc, bufferBitmap);

                    enum { nocheck, check, graycheck } checkType = nocheck;
                    INT startX = clientRect.left + (clientRect.right - clientRect.bottom) / 2 + 1;
                    INT startY = clientRect.top + (clientRect.bottom - clientRect.top) / 2;

                    DrawIconEx(bufferDc, clientRect.left, clientRect.top, iconHandle, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top,
                        0, NULL, DI_NORMAL);

                    for (INT x = startX, y = startY; x < clientRect.right; x++)
                    {
                        COLORREF pixel = GetPixel(bufferDc, x, y);
                        if (pixel == RGB(0xB4, 0xB4, 0xB4))
                        {
                            checkType = graycheck;
                            goto draw_acl_check;
                        }
                    }
                    for (INT x = startX, y = startY; x < clientRect.right; x++)
                    {
                        COLORREF pixel = GetPixel(bufferDc, x, y);
                        if (pixel == RGB(0, 0, 0) || pixel == PhThemeWindowTextColor)
                        {
                            checkType = check;
                            goto draw_acl_check;
                        }
                    }

                draw_acl_check:
                    if (checkType == check || checkType == graycheck)   // right is checked or special permission checked
                    {
                        if (PhBeginInitOnce(&initOnce)) // cache font
                        {
                            hCheckFont = CreateFont(
                                clientRect.bottom - clientRect.top - 1,
                                clientRect.right - clientRect.left - 3,
                                0, 0,
                                FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                                CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                VARIABLE_PITCH, L"Segoe UI"
                            );
                            PhEndInitOnce(&initOnce);
                        }

                        SetBkMode(hdc, TRANSPARENT);
                        SetTextColor(hdc, checkType == check ? PhThemeWindowTextColor : RGB(0xB4, 0xB4, 0xB4));
                        SelectFont(hdc, hCheckFont);
                        //HFONT hFontOriginal = SelectFont(hdc, hCheckFont);
                        FillRect(hdc, &clientRect, PhThemeWindowBackgroundBrush);
                        DrawText(hdc, L"✓", 1, &clientRect, DT_CENTER | DT_VCENTER);
                        //SelectFont(hdc, hFontOriginal);
                    }

                    SelectBitmap(bufferDc, oldBufferBitmap);
                    DeleteBitmap(bufferBitmap);
                    DeleteDC(bufferDc);
                    EndPaint(WindowHandle, &ps);
                    return 0;
                }
            }
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

LONG ThemeWindowStatusBarUpdateRectToIndex(
    _In_ HWND WindowHandle,
    _In_ WNDPROC WindowProcedure,
    _In_ PRECT UpdateRect,
    _In_ LONG Count
    )
{
    for (LONG i = 0; i < Count; i++)
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
    _In_ LONG Index
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
        SetTextColor(bufferDc, PhThemeWindowTextColor);
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

    LONG blockCount = (LONG)CallWindowProc(
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

        for (LONG i = 0; i < blockCount; i++)
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
    else if (PhEnableThemeSupport && WindowMessage != WM_NCDESTROY)
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

            if (!PhEnableThemeSupport ||
                PhGetWindowContext(WindowHandle, SHRT_MAX) ||   // The searchbox control does its own theme drawing.
                !PhIsDarkModeAllowedForWindow(WindowHandle))    // Unsupported system dialogs does its own drawing
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
                    // A little bit nicer and contrast color (Dart Vanya)
                    SetDCBrushColor(hdc, PhMakeColorBrighter(GetSysColor(COLOR_HOTLIGHT), 85)); // PhThemeWindowHighlightColor
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
    HWND ParentWindow;
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

            GETCLASSNAME_OR_NULL(createStruct->hwndParent, windowClassName);

            if (PhEqualStringZ(windowClassName, PH_TREENEW_CLASSNAME, FALSE))
            {
                LONG_PTR windowStyle = PhGetWindowStyle(createStruct->hwndParent);

                if (BooleanFlagOn(windowStyle, TN_STYLE_CUSTOM_HEADERDRAW))
                {
                    PhSetControlTheme(WindowHandle, PhEnableThemeSupport ? L"DarkMode_ItemsView" : L"Explorer");

                    return CallWindowProc(PhDefaultHeaderWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);
                }
            }
        }

		context = PhAllocateZero(sizeof(PHP_THEME_WINDOW_HEADER_CONTEXT));
		context->ThemeHandle = PhOpenThemeData(WindowHandle, VSCLASS_HEADER, PhGetWindowDpi(WindowHandle));
		context->CursorPos.x = LONG_MIN;
		context->CursorPos.y = LONG_MIN;
		context->ParentWindow = createStruct->hwndParent;
		PhSetWindowContext(WindowHandle, LONG_MAX, context);

        PhSetControlTheme(WindowHandle, PhEnableThemeSupport ? L"DarkMode_ItemsView" : L"Explorer");
    }
    else if (PhEnableThemeSupport || WindowMessage == WM_NCDESTROY)
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
                if (!PhIsDarkModeAllowedForWindow(context->ParentWindow))
                {
                    PhRemoveWindowContext(WindowHandle, LONG_MAX);
                    if (context->ThemeHandle)
                        PhCloseThemeData(context->ThemeHandle);
                    PhSetControlTheme(WindowHandle, L"Explorer");
                    PhFree(context);
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

static HRESULT (WINAPI* DefaultDrawThemeBackground)(
    _In_ HTHEME Theme,
    _In_ HDC Hdc,
    _In_ INT PartId,
    _In_ INT StateId,
    _In_ LPCRECT Rect,
    _In_ LPCRECT ClipRect
    ) = NULL;

static HRESULT (WINAPI* DefaultDrawThemeBackgroundEx)(
    _In_ HTHEME         hTheme,
    _In_ HDC            hdc,
    _In_ int            iPartId,
    _In_ int            iStateId,
    _In_ LPCRECT        pRect,
    _In_ const DTBGOPTS* pOptions
    ) = NULL;

static HRESULT(WINAPI* DefaultDrawThemeText)(
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

static HRESULT(WINAPI* DefaultDrawThemeTextEx)(
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

int (WINAPI* DefaultComCtl32DrawTextW)(
    _In_      HDC     hdc,
    _Inout_   LPCWSTR lpchText,
    _In_      int     cchText,
    _Inout_   LPRECT  lprc,
    _In_      UINT    format
    ) = NULL;

static HRESULT(WINAPI* DefaultTaskDialogIndirect)(
    _In_      const TASKDIALOGCONFIG* pTaskConfig,
    _Out_opt_ int* pnButton,
    _Out_opt_ int* pnRadioButton,
    _Out_opt_ BOOL* pfVerificationFlagChecked
    ) = NULL;

static HRESULT(WINAPI* DefaultGetThemeColor)(
    _In_  HTHEME   hTheme,
    _In_  int      iPartId,
    _In_  int      iStateId,
    _In_  int      iPropId,
    _Out_ COLORREF* pColor
    ) = NULL;

// uxtheme.dll ordinal 49
static HTHEME(WINAPI* DefaultOpenNcThemeData)(
    _In_ HWND    hwnd,
    _In_ LPCWSTR pszClassList
    ) = NULL;

static BOOL (WINAPI* DefaultSystemParametersInfo)(
    _In_ UINT uiAction,
    _In_ UINT uiParam,
    _Pre_maybenull_ _Post_valid_ PVOID pvParam,
    _In_ UINT fWinIni
    ) = NULL;

static HWND (WINAPI* DefaultCreateWindowEx)(
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
} TASKDIALOG_CALLBACK_WRAP, *PTASKDIALOG_CALLBACK_WRAP;

typedef struct _TASKDIALOG_COMMON_CONTEXT
{
    WNDPROC DefaultWindowProc;
    ULONG Painting;
} TASKDIALOG_COMMON_CONTEXT, *PTASKDIALOG_COMMON_CONTEXT;

typedef struct _TASKDIALOG_WINDOW_CONTEXT
{
    WNDPROC DefaultWindowProc;
    ULONG Painting;
    PTASKDIALOG_CALLBACK_WRAP CallbackData;
} TASKDIALOG_WINDOW_CONTEXT, *PTASKDIALOG_WINDOW_CONTEXT;

#define TASKDIALOG_CONTEXT_TAG (ULONG)'TDLG'

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
    _In_ LONG PartId,
    _In_ LONG StateId,
    _In_ LPCRECT Rect,
    _In_ LPCRECT ClipRect
    )
{
    WCHAR className[MAX_PATH];

    if (PhEnableThemeSupport &&
        PhGetThemeClass(Theme, className, RTL_NUMBER_OF(className)))
    {
        if (WindowsVersion >= WINDOWS_11)
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

        if (PhEqualStringZ(className, VSCLASS_PROGRESS, TRUE)
            /*|| WindowsVersion < WINDOWS_11 && WindowFromDC(Hdc) == NULL*/)
        {
            if (PartId == PP_TRANSPARENTBAR || PartId == PP_TRANSPARENTBARVERT) // Progress bar background
            {
                FillRect(Hdc, Rect, PhThemeWindowBackgroundBrush);
                SetDCBrushColor(Hdc, RGB(0x60, 0x60, 0x60));
                FrameRect(Hdc, Rect, PhGetStockBrush(DC_BRUSH));
                return S_OK;
            }
        }
    }

    return DefaultDrawThemeBackground(Theme, Hdc, PartId, StateId, Rect, ClipRect);
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

    if (PhEnableThemeSupport)
    {
        // Apply theme to ListView checkboxes
        if (iPartId == BP_CHECKBOX /*|| iPartId == BP_RADIOBUTTON*/)
        {
            if (PhGetThemeClass(hTheme, className, RTL_NUMBER_OF(className)) &&
                PhEqualStringZ(className, VSCLASS_BUTTON, TRUE))
            {
                HTHEME darkButtonTheme = PhOpenThemeData(NULL, L"DarkMode_Explorer::Button", 0);
                HRESULT retVal = DefaultDrawThemeBackgroundEx(darkButtonTheme ? darkButtonTheme : hTheme, hdc, iPartId, iStateId, pRect, pOptions);
                if (darkButtonTheme)
                    PhCloseThemeData(darkButtonTheme);
                return retVal;
            }
        }

        // Micro optimization
        if ((iPartId == TDLG_PRIMARYPANEL || iPartId == TDLG_FOOTNOTEPANE || iPartId == TDLG_SECONDARYPANEL ||
            iPartId == TDLG_FOOTNOTESEPARATOR ||iPartId == TDLG_EXPANDOBUTTON) &&
            PhGetThemeClass(hTheme, className, RTL_NUMBER_OF(className)) && PhEqualStringZ(className, VSCLASS_TASKDIALOG, TRUE)
            /*|| WindowsVersion < WINDOWS_11 && WindowFromDC(hdc) == NULL*/)
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
                if (WindowsVersion >= WINDOWS_11)
                {
                    // In Windows 11, buttons lack background, making them indistinguishable on dark backgrounds.
                    // To address this, we invert the button. This technique isn't applicable to Windows 10 as it causes the button's border to appear chipped.
                    static enum { yes, no, unknown } mustInvertButton = unknown;
                    if (mustInvertButton == unknown)
                    {
                        DefaultDrawThemeBackgroundEx(hTheme, hdc, iPartId, iStateId, pRect, pOptions);
                        int buttonCenterX = pOptions->rcClip.left + (pOptions->rcClip.right - pOptions->rcClip.left) / 2;
                        int buttonCenterY = pOptions->rcClip.top + (pOptions->rcClip.bottom - pOptions->rcClip.top) / 2;
                        COLORREF centerPixel = GetPixel(hdc, buttonCenterX, buttonCenterY);
                        mustInvertButton = centerPixel == PhThemeWindowTextColor ? no : yes;
                    }
                    FillRect(hdc, pRect, PhThemeWindowBackgroundBrush);
                    if (mustInvertButton == yes) InvertRect(hdc, pRect);
                    HRESULT retVal = DefaultDrawThemeBackgroundEx(hTheme, hdc, iPartId, iStateId, pRect, pOptions);
                    if (mustInvertButton == yes) InvertRect(hdc, pRect);
                    return retVal;
                }
            }
        }
    }

    return DefaultDrawThemeBackgroundEx(hTheme, hdc, iPartId, iStateId, pRect, pOptions);
}

HWND PhCreateWindowExHook(
    _In_ ULONG ExStyle,
    _In_opt_ PCWSTR ClassName,
    _In_opt_ PCWSTR WindowName,
    _In_ ULONG Style,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_opt_ HWND Parent,
    _In_opt_ HMENU Menu,
    _In_opt_ PVOID Instance,
    _In_opt_ PVOID Param
    )
{
    HWND windowHandle = DefaultCreateWindowEx(
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
        if (PhEnableStreamerMode)
        {
            SetWindowDisplayAffinity(windowHandle, WDA_EXCLUDEFROMCAPTURE);
        }

        if (PhEnableThemeSupport && PhEnableThemeAcrylicWindowSupport)
        {
            PhSetWindowAcrylicCompositionColor(windowHandle, MakeABGRFromCOLORREF(0, RGB(10, 10, 10)), TRUE);
        }
    }
    else
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
    if (PhEnableThemeSupport && !PhEnableThemeAnimation)
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
    }

    return DefaultSystemParametersInfo(uiAction, uiParam, pvParam, fWinIni);
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
    WCHAR className[MAX_PATH];

    if (PhEnableThemeSupport)
    {
        if ((iPartId == BP_COMMANDLINK /*|| iPartId == BP_RADIOBUTTON*/) && iStateId != PBS_DISABLED)
        {
            if (PhGetThemeClass(hTheme, className, RTL_NUMBER_OF(className)) && PhEqualStringZ(className, VSCLASS_BUTTON, TRUE)
                /*|| WindowsVersion < WINDOWS_11 && WindowFromDC(hdc) == NULL*/)
            {
                DTTOPTS options = { sizeof(DTTOPTS), DTT_TEXTCOLOR, PhThemeWindowTextColor };
                return DefaultDrawThemeTextEx(hTheme, hdc, iPartId, iStateId, pszText, cchText, dwTextFlags, (LPRECT)pRect, &options);
            }
        }
    }

    return DefaultDrawThemeText(hTheme, hdc, iPartId, iStateId, pszText, cchText, dwTextFlags, dwTextFlags2, pRect);
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
    WCHAR className[MAX_PATH];

    if (PhEnableThemeSupport)
    {
        if (iPartId == BP_COMMANDLINK)
        {
            if (PhGetThemeClass(hTheme, className, RTL_NUMBER_OF(className)) && PhEqualStringZ(className, VSCLASS_BUTTON, TRUE)
                /*|| WindowsVersion < WINDOWS_11 && WindowFromDC(hdc) == NULL*/)
            {
                DTTOPTS options = { sizeof(DTTOPTS) };
                if (pOptions)
                    options = *pOptions;
                options.dwFlags |= DTT_TEXTCOLOR;
                DefaultGetThemeColor(hTheme, iPartId, iStateId, TMT_TEXTCOLOR, &options.crText);
                options.crText = PhMakeColorBrighter(options.crText, 90);
                return DefaultDrawThemeTextEx(hTheme, hdc, iPartId, iStateId, pszText, cchText, dwTextFlags, pRect, &options);
            }
        }
    }

    return DefaultDrawThemeTextEx(hTheme, hdc, iPartId, iStateId, pszText, cchText, dwTextFlags, pRect, pOptions);
}

int PhDetoursComCtl32DrawTextW(
    _In_      HDC     hdc,
    _Inout_   LPCWSTR lpchText,
    _In_      int     cchText,
    _Inout_   LPRECT  lprc,
    _In_      UINT    format
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static COLORREF colLinkNormal = RGB(0, 0 ,0);
    static COLORREF colLinkHot = RGB(0, 0, 0);
    static COLORREF colLinkPressed = RGB(0, 0, 0);
    HWND WindowHandle;
    WCHAR windowClassName[MAX_PATH];

    if (PhEnableThemeSupport)
    {
        if ((WindowHandle = WindowFromDC(hdc)) && PhIsDarkModeAllowedForWindow(WindowHandle))    // HACK
        {
            GETCLASSNAME_OR_NULL(WindowHandle, windowClassName);
            if (PhEqualStringZ(windowClassName, WC_LINK, FALSE))
            {
                if (PhBeginInitOnce(&initOnce))
                {
                    HTHEME hTextTheme = PhOpenThemeData(WindowHandle, VSCLASS_TEXTSTYLE, PhGetWindowDpi(WindowHandle));
                    if (hTextTheme)
                    {
                        PhGetThemeColor(hTextTheme, TEXT_HYPERLINKTEXT, TS_HYPERLINK_NORMAL, TMT_TEXTCOLOR, &colLinkNormal);
                        PhGetThemeColor(hTextTheme, TEXT_HYPERLINKTEXT, TS_HYPERLINK_HOT, TMT_TEXTCOLOR, &colLinkHot);
                        PhGetThemeColor(hTextTheme, TEXT_HYPERLINKTEXT, TS_HYPERLINK_PRESSED, TMT_TEXTCOLOR, &colLinkPressed);
                        PhCloseThemeData(hTextTheme);
                    }
                    PhEndInitOnce(&initOnce);
                }

                COLORREF color = GetTextColor(hdc);

                if (color == colLinkNormal || color == colLinkHot || color == colLinkPressed ||
                    WindowsVersion < WINDOWS_11 && color == RGB(0x00, 0x66, 0xCC))  // on Windows 10 PhGetThemeColor returns 0xFFFFFF for any StateId
                {
                    SetTextColor(hdc, PhMakeColorBrighter(color, 95));
                }
            }
        }
    }

    return DefaultComCtl32DrawTextW(hdc, lpchText, cchText, lprc, format);
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

    HRESULT retVal = DefaultGetThemeColor(hTheme, iPartId, iStateId, iPropId, pColor);

    if (PhEnableThemeSupport)
    {
        if (iPropId == TMT_TEXTCOLOR && iPartId == TDLG_MAININSTRUCTIONPANE)
        {
            if (PhGetThemeClass(hTheme, className, RTL_NUMBER_OF(className)) && PhEqualStringZ(className, VSCLASS_TASKDIALOGSTYLE, TRUE)
                /*|| WindowsVersion < WINDOWS_11*/)
            {
                *pColor = PhMakeColorBrighter(*pColor, 150); // Main header.
            }
        }
        else if (iPropId == TMT_TEXTCOLOR)
        {
            if (PhGetThemeClass(hTheme, className, RTL_NUMBER_OF(className)) && PhEqualStringZ(className, VSCLASS_TASKDIALOGSTYLE, TRUE)
                /*|| WindowsVersion < WINDOWS_11*/)
            {
                *pColor = PhThemeWindowTextColor; // Text color for check boxes, expanded text, and expander button text.
            }
        }
    }

    return retVal;
}

HTHEME PhOpenNcThemeDataHook(
    _In_ HWND    hwnd,
    _In_ LPCWSTR pszClassList
    )
{
    if (PhEnableThemeSupport)
    {
        if (PhEqualStringZ(pszClassList, VSCLASS_SCROLLBAR, TRUE) &&
            PhIsDarkModeAllowedForWindow(hwnd))
        {
            return DefaultOpenNcThemeData(NULL, L"Explorer::ScrollBar");
        }
    }

    return DefaultOpenNcThemeData(hwnd, pszClassList);
}

BOOLEAN CALLBACK PhInitializeTaskDialogTheme(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID CallbackData
    )
{
    WCHAR windowClassName[MAX_PATH];
    PTASKDIALOG_COMMON_CONTEXT context;
    BOOLEAN windowHasContext = !!PhGetWindowContext(WindowHandle, TASKDIALOG_CONTEXT_TAG);

    if (CallbackData && !windowHasContext)
    {
        if (PhEnableStreamerMode)
        {
            SetWindowDisplayAffinity(WindowHandle, WDA_EXCLUDEFROMCAPTURE);
        }

        if (PhEnableThemeSupport)
        {
            PhInitializeThemeWindowFrame(WindowHandle);
        }

        PTASKDIALOG_WINDOW_CONTEXT context = PhAllocateZero(sizeof(TASKDIALOG_WINDOW_CONTEXT));
        context->DefaultWindowProc = PhSetWindowProcedure(WindowHandle, ThemeTaskDialogMasterSubclass);
        context->CallbackData = CallbackData;
        PhSetWindowContext(WindowHandle, TASKDIALOG_CONTEXT_TAG, context);
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

    context = PhAllocateZero(sizeof(TASKDIALOG_COMMON_CONTEXT));
    context->DefaultWindowProc = PhSetWindowProcedure(WindowHandle, ThemeTaskDialogMasterSubclass);
    PhSetWindowContext(WindowHandle, TASKDIALOG_CONTEXT_TAG, context);

    if (PhEnableThemeSupport)
    {
        GETCLASSNAME_OR_NULL(WindowHandle, windowClassName);

        if (PhEqualStringZ(windowClassName, WC_BUTTON, FALSE) ||
            PhEqualStringZ(windowClassName, WC_SCROLLBAR, FALSE))
        {
            PhWindowThemeSetDarkMode(WindowHandle, TRUE);
        }
        else if (PhEqualStringZ(windowClassName, WC_LINK, FALSE))
        {
            PhAllowDarkModeForWindow(WindowHandle, TRUE);
        }
        else if (PhEqualStringZ(windowClassName, L"DirectUIHWND", FALSE))
        {
            //WINDOWPLACEMENT pos = { 0 };
            //GetWindowPlacement(GetParent(WindowHandle), &pos);
            PhWindowThemeSetDarkMode(WindowHandle, TRUE);
            //SetWindowPlacement(GetParent(WindowHandle), &pos);
        }
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

    if (!(context = PhGetWindowContext(hwnd, TASKDIALOG_CONTEXT_TAG)))
        return 0;

    OldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_ERASEBKGND:
        {
            HDC hdc = (HDC)wParam;
            RECT rect;
            WCHAR windowClassName[MAX_PATH];

            if (!PhEnableThemeSupport)
                return CallWindowProc(OldWndProc, hwnd, uMsg, wParam, lParam);

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
            }
        }
        return TRUE;
    case WM_NOTIFY:
        {
            LPNMHDR data = (LPNMHDR)lParam;

            if (!PhEnableThemeSupport)
                return CallWindowProc(OldWndProc, hwnd, uMsg, wParam, lParam);

            if (data->code == NM_CUSTOMDRAW)
            {
                LPNMCUSTOMDRAW customDraw = (LPNMCUSTOMDRAW)lParam;
                WCHAR className[MAX_PATH];

                GETCLASSNAME_OR_NULL(customDraw->hdr.hwndFrom, className);
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
    case WM_NCDESTROY:
        {
            PhSetWindowProcedure(hwnd, OldWndProc);
            PhRemoveWindowContext(hwnd, TASKDIALOG_CONTEXT_TAG);
            PhFree(context);
        }
        return CallWindowProc(OldWndProc, hwnd, uMsg, wParam, lParam);
    case WM_CTLCOLORDLG:
        // Window background color when the extender resizes upward (Windows 10 only).
        return PhEnableThemeSupport ? (LRESULT)PhThemeWindowBackgroundBrush : CallWindowProc(OldWndProc, hwnd, uMsg, wParam, lParam);
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

    return DefaultTaskDialogIndirect(&myConfig, pnButton, pnRadioButton, pfVerificationFlagChecked);
}

VOID PhRegisterDetoursHooks(
    VOID
    )
{
    NTSTATUS status;
    PVOID baseAddress;

    if (baseAddress = PhGetLoaderEntryDllBaseZ(L"user32.dll"))
    {
        DefaultCreateWindowEx = PhGetDllBaseProcedureAddress(baseAddress, "CreateWindowExW", 0);
        DefaultSystemParametersInfo = PhGetDllBaseProcedureAddress(baseAddress, "SystemParametersInfoW", 0);
    }

    if (baseAddress = PhGetLoaderEntryDllBaseZ(L"uxtheme.dll"))
    {
        DefaultDrawThemeBackground = PhGetDllBaseProcedureAddress(baseAddress, "DrawThemeBackground", 0);
        DefaultDrawThemeBackgroundEx = PhGetDllBaseProcedureAddress(baseAddress, "DrawThemeBackgroundEx", 0);
        DefaultDrawThemeText = PhGetDllBaseProcedureAddress(baseAddress, "DrawThemeText", 0);
        DefaultDrawThemeTextEx = PhGetDllBaseProcedureAddress(baseAddress, "DrawThemeTextEx", 0);
        DefaultGetThemeColor = PhGetDllBaseProcedureAddress(baseAddress, "GetThemeColor", 0);
        DefaultOpenNcThemeData = PhGetDllBaseProcedureAddress(baseAddress, NULL, 49);
    }

    if (baseAddress = PhGetLoaderEntryDllBaseZ(L"Comctl32.dll"))
    {
        if (WindowsVersion >= WINDOWS_11)   // TaskDialog theme on Windows 10 currently unsupported...
            DefaultTaskDialogIndirect = PhGetDllBaseProcedureAddress(baseAddress, "TaskDialogIndirect", 0);
        PhLoaderEntryDetourImportProcedure(baseAddress, "User32.dll", "DrawTextW", PhDetoursComCtl32DrawTextW, (PVOID*)&DefaultComCtl32DrawTextW);
    }

    if (!NT_SUCCESS(status = DetourTransactionBegin()))
        goto CleanupExit;

    {
        if (!NT_SUCCESS(status = DetourAttach((PVOID)&DefaultDrawThemeBackground, (PVOID)PhDrawThemeBackgroundHook)))
            goto CleanupExit;
        if (!NT_SUCCESS(status = DetourAttach((PVOID)&DefaultDrawThemeBackgroundEx, (PVOID)PhDrawThemeBackgroundExHook)))
            goto CleanupExit;
        if (!PhEnableThemeAnimation)
            if (!NT_SUCCESS(status = DetourAttach((PVOID)&DefaultSystemParametersInfo, (PVOID)PhSystemParametersInfoHook)))
                goto CleanupExit;

        if (!NT_SUCCESS(status = DetourAttach((PVOID)&DefaultDrawThemeText, (PVOID)PhDrawThemeTextHook)))
            goto CleanupExit;
        if (!NT_SUCCESS(status = DetourAttach((PVOID)&DefaultDrawThemeTextEx, (PVOID)PhDrawThemeTextExHook)))
            goto CleanupExit;
        if (!NT_SUCCESS(status = DetourAttach((PVOID)&DefaultGetThemeColor, (PVOID)PhGetThemeColorHook)))
            goto CleanupExit;
        if (!NT_SUCCESS(status = DetourAttach((PVOID)&DefaultOpenNcThemeData, (PVOID)PhOpenNcThemeDataHook)))
            goto CleanupExit;
        if (WindowsVersion >= WINDOWS_11)
            if (!NT_SUCCESS(status = DetourAttach((PVOID)&DefaultTaskDialogIndirect, (PVOID)PhTaskDialogIndirectHook)))
                goto CleanupExit;
    }

    if (!NT_SUCCESS(status = DetourAttach((PVOID)&DefaultCreateWindowEx, (PVOID)PhCreateWindowExHook)))
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
        ULONG enableTransparency = PhQueryRegistryUlongZ(keyHandle, L"EnableTransparency");
        themesEnableTransparency = enableTransparency != ULONG_MAX ? !!enableTransparency : FALSE;
        NtClose(keyHandle);
    }

    return themesEnableTransparency;
}

VOID PhInitializeSuperclassControls(
    VOID
    )
{
    if (PhEnableThemeAcrylicSupport && !PhEnableThemeSupport)
        PhEnableThemeAcrylicSupport = FALSE;
    //if (PhEnableThemeAcrylicSupport)
        //PhEnableThemeAcrylicSupport = PhIsThemeTransparencyEnabled();

    // For early TaskDialog with PhStartupParameters.ShowOptions
    if (!PhThemeWindowBackgroundBrush)
        PhThemeWindowBackgroundBrush = CreateSolidBrush(PhThemeWindowBackgroundColor);

    // Superclasses and detours now always installing that gives opportunity to flawlessly runtime theme switching.
    // If theme is disabled they are just trampolined to original hooked procedures.
    {
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
