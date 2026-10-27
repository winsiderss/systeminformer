/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2022-2026
 *
 */

#include <phapp.h>
#include <apiimport.h>
#include <mapldr.h>
#include <thirdparty.h>
#include <dwmapi.h>
#include <vsstyle.h>
#include <uxtheme.h>
#include <Vssym32.h>

#include "settings.h"
#include <phsettings.h>

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

/**
 * Window hook procedure for superclassed menu windows (#32768).
 *
 * \param WindowHandle A handle to the menu window.
 * \param WindowMessage The window message.
 * \param wParam Additional message-dependent information.
 * \param lParam Additional message-dependent information.
 * \return LRESULT The result of the message processing.
 */
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

            if (WindowsVersion >= WINDOWS_11)
            {
                // CS_DROPSHADOW is no longer honored for the menu class on Windows 11
                // and DWM non-client rendering doesn't apply to these windows, so
                // composite the shadow ourselves. The shadow is positioned and shown
                // from WM_WINDOWPOSCHANGED once the menu has been sized. (dmex)
                PhCreateWindowShadow(WindowHandle);
            }

            if (PhDefaultEnableStreamerMode)
            {
                SetWindowDisplayAffinity(WindowHandle, WDA_EXCLUDEFROMCAPTURE);
                PhSetWindowShadowDisplayAffinity(WindowHandle, WDA_EXCLUDEFROMCAPTURE);
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
    case WM_DESTROY:
        {
            PhDestroyWindowShadow(WindowHandle);
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
    case WM_WINDOWPOSCHANGED:
        {
            LRESULT result = CallWindowProc(PhDefaultMenuWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);

            // Track the menu after the default procedure has sized and placed it. (dmex)
            PhUpdateWindowShadow(WindowHandle, PhWindowShadowSideAll);

            return result;
        }
    }

    return CallWindowProc(PhDefaultMenuWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);
}

/**
 * Window hook procedure for superclassed dialog windows (#32770).
 *
 * \param WindowHandle A handle to the dialog window.
 * \param WindowMessage The window message.
 * \param wParam Additional message-dependent information.
 * \param lParam Additional message-dependent information.
 * \return LRESULT The result of the message processing.
 */
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
                    SetWindowDisplayAffinity(WindowHandle, WDA_EXCLUDEFROMCAPTURE);
                }

                if (PhEnableThemeSupport && PhDefaultEnableThemeAcrylicWindowSupport)
                {
                    // Note: DWM crashes if called from WM_NCCREATE (dmex)
                    PhSetWindowAcrylicCompositionColor(WindowHandle, MakeABGRFromCOLORREF(0, RGB(10, 10, 10)));
                }
            }
        }
        break;
    case WM_ERASEBKGND:
        {
            RECT clipRect;
            HDC hdc = (HDC)wParam;

            // The clipped parent cannot erase this child window. Clear the
            // status bar with the same brush used by WM_PAINT so an exposed
            // area does not show stale or white pixels before the buffered paint.
            if (GetClipBox(hdc, &clipRect) <= NULLREGION)
                return TRUE;

            FillRect(hdc, &clipRect, PhThemeWindowBackgroundBrush);

            return TRUE;
        }
    }

    return CallWindowProc(PhDefaultDialogWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);
}

/**
 * Window hook procedure for superclassed rebar controls (ReBarWindow32).
 *
 * \param WindowHandle A handle to the rebar control window.
 * \param WindowMessage The window message.
 * \param wParam Additional message-dependent information.
 * \param lParam Additional message-dependent information.
 * \return LRESULT The result of the message processing.
 */
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
    case WM_ERASEBKGND:
        {
            if (!PhEnableThemeSupport)
                break;
        }
        return TRUE;
    case WM_PAINT:
        {
            PAINTSTRUCT paintStruct;
            PH_BUFFERED_PAINT paintBuffer;
            HDC bufferDc;
            HDC hdc;

            if (!PhEnableThemeSupport)
                break;

            if (!(hdc = BeginPaint(WindowHandle, &paintStruct)))
                break;

            if (PhRectEmpty(&paintStruct.rcPaint))
            {
                EndPaint(WindowHandle, &paintStruct);
                return 0;
            }

            if (PhBeginBufferedPaint(hdc, &paintStruct.rcPaint, PHBF_TOPDOWNDIB, NULL, &paintBuffer, &bufferDc))
            {
                FillRect(bufferDc, &paintStruct.rcPaint, PhThemeWindowBackgroundBrush);
                CallWindowProc(PhDefaultRebarWindowProcedure, WindowHandle, WM_PRINTCLIENT, (WPARAM)bufferDc, PRF_CLIENT);
                PhEndBufferedPaint(&paintBuffer, TRUE);
            }
            else
            {
                FillRect(hdc, &paintStruct.rcPaint, PhThemeWindowBackgroundBrush);
                CallWindowProc(PhDefaultRebarWindowProcedure, WindowHandle, WM_PRINTCLIENT, (WPARAM)hdc, PRF_CLIENT);
            }

            EndPaint(WindowHandle, &paintStruct);
        }
        return 0;
    }

    return CallWindowProc(PhDefaultRebarWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);
}

/**
 * Window hook procedure for superclassed combo box controls (ComboBox).
 *
 * \param WindowHandle A handle to the combo box window.
 * \param WindowMessage The window message.
 * \param wParam Additional message-dependent information.
 * \param lParam Additional message-dependent information.
 * \return LRESULT The result of the message processing.
 */
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
                    SetWindowDisplayAffinity(info.hwndList, WDA_EXCLUDEFROMCAPTURE);
                }
            }
        }
        break;
    }

    return result;
}

/**
 * Window hook procedure for superclassed static controls (Static).
 *
 * \param WindowHandle A handle to the static control window.
 * \param WindowMessage The window message.
 * \param wParam Additional message-dependent information.
 * \param lParam Additional message-dependent information.
 * \return LRESULT The result of the message processing.
 */
LRESULT CALLBACK PhStaticWindowHookProcedure(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    if (WindowMessage == WM_NCCREATE)
    {
        ULONG style = PhGetWindowStyle(WindowHandle);

        if ((style & SS_TYPEMASK) == SS_ICON)
        {
            PhSetWindowContext(WindowHandle, SCHAR_MAX, UlongToPtr(TRUE));
        }
    }

    if (WindowMessage != WM_KILLFOCUS && !PhGetWindowContext(WindowHandle, SCHAR_MAX))
        return CallWindowProc(PhDefaultStaticWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);

    switch (WindowMessage)
    {
    case WM_NCDESTROY:
        PhRemoveWindowContext(WindowHandle, SCHAR_MAX);
        break;
    case WM_ERASEBKGND:
        {
            RECT clipRect;
            HDC hdc = (HDC)wParam;

            // The clipped parent cannot erase this child window. Clear the
            // status bar with the same brush used by WM_PAINT so an exposed
            // area does not show stale or white pixels before the buffered paint.
            if (GetClipBox(hdc, &clipRect) <= NULLREGION)
                return TRUE;

            FillRect(hdc, &clipRect, PhThemeWindowBackgroundBrush);

            return TRUE;
        }
    case WM_KILLFOCUS:
        {
            WCHAR windowClassName[MAX_PATH];
            HWND ParentHandle = GetParent(WindowHandle);
            if (!NT_SUCCESS(PhGetClassName(ParentHandle, windowClassName, RTL_NUMBER_OF(windowClassName), NULL)))
                windowClassName[0] = UNICODE_NULL;
            if (PhEqualStringZ(windowClassName, L"CHECKLIST_ACLUI", FALSE))
            {
                RECT rectClient;

                if (PhGetClientRect(WindowHandle, &rectClient))
                {
                    PhInflateRect(&rectClient, 2, 2);
                    MapWindowRect(WindowHandle, ParentHandle, &rectClient);
                    InvalidateRect(ParentHandle, &rectClient, TRUE);     // fix the annoying white border left by the previous active control
                }
            }
        }
        break;
    case WM_PAINT:
        {
            HICON iconHandle;
            PAINTSTRUCT ps;
            RECT clientRect;
            WCHAR windowClassName[MAX_PATH];

            if (!NT_SUCCESS(PhGetClassName(GetParent(WindowHandle), windowClassName, RTL_NUMBER_OF(windowClassName), NULL)))
                windowClassName[0] = UNICODE_NULL;
            if (PhEqualStringZ(windowClassName, L"CHECKLIST_ACLUI", FALSE))
            {
                if (iconHandle = (HICON)(UINT_PTR)CallWindowProc(PhDefaultStaticWindowProcedure, WindowHandle, STM_GETICON, 0, 0))
                {
                    static PH_INITONCE initOnce = PH_INITONCE_INIT;
                    static HFONT hCheckFont = NULL;

                    HDC hdc = BeginPaint(WindowHandle, &ps);
                    PH_BUFFERED_PAINT bufferedPaint;
                    BOOLEAN buffered;
                    HDC bufferDc;

                    if (!hdc)
                        return 0;

                    GetClientRect(WindowHandle, &clientRect);
                    buffered = PhBeginBufferedPaint(hdc, &clientRect, PHBF_TOPDOWNDIB, NULL, &bufferedPaint, &bufferDc);

                    if (!buffered)
                        bufferDc = hdc;

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
                                VARIABLE_PITCH, L"Segoe UI");
                            PhEndInitOnce(&initOnce);
                        }

                        HFONT oldFont;
                        INT oldBkMode;
                        COLORREF oldTextColor;

                        // Draw into the surface that will be committed. In the
                        // fallback path bufferDc is hdc, so both paths match.
                        oldBkMode = SetBkMode(bufferDc, TRANSPARENT);
                        oldTextColor = SetTextColor(bufferDc, checkType == check ? PhThemeWindowTextColor : RGB(0xB4, 0xB4, 0xB4));
                        oldFont = hCheckFont ? SelectFont(bufferDc, hCheckFont) : NULL;
                        FillRect(bufferDc, &clientRect, PhThemeWindowBackgroundBrush);
                        DrawText(bufferDc, L"V", 1, &clientRect, DT_CENTER | DT_VCENTER);
                        if (oldFont)
                            SelectFont(bufferDc, oldFont);
                        SetTextColor(bufferDc, oldTextColor);
                        SetBkMode(bufferDc, oldBkMode);
                    }

                    if (buffered)
                        PhEndBufferedPaint(&bufferedPaint, TRUE);

                    EndPaint(WindowHandle, &ps);
                    return 0;
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
    LONG WindowDpi;
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
    LONG HotIndex;
} PHP_THEME_WINDOW_STATUSBAR_CONTEXT, *PPHP_THEME_WINDOW_STATUSBAR_CONTEXT;

/**
 * Finds the status bar part index matching an update rectangle.
 *
 * \param WindowHandle A handle to the status bar window.
 * \param WindowProcedure The status bar window procedure.
 * \param UpdateRect A pointer to the update rectangle.
 * \param Count The number of status bar parts.
 * \return LONG The matching part index, or INT_ERROR if not found.
 */
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

/**
 * Renders a single status bar part with theme support.
 *
 * \param Context A pointer to the status bar theme context.
 * \param WindowHandle A handle to the status bar window.
 * \param bufferDc The device context used for drawing.
 * \param clientRect A pointer to the client area rectangle.
 * \param Index The index of the status bar part to draw.
 */
VOID ThemeWindowStatusBarDrawPart(
    _In_ PPHP_THEME_WINDOW_STATUSBAR_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC bufferDc,
    _In_ PRECT clientRect,
    _In_ LONG Index
    )
{
    RECT blockRect = { 0 };
    WCHAR textBuffer[0x80] = { 0 };
    PWSTR text = textBuffer;
    ULONG_PTR result;
    ULONG textLength;
    ULONG textStyle;
    HICON iconHandle;

    if (!CallWindowProc(PhDefaultStatusbarWindowProcedure, WindowHandle, SB_GETRECT, (WPARAM)Index, (WPARAM)&blockRect))
    if (!CallWindowProc(PhDefaultStatusbarWindowProcedure, WindowHandle, SB_GETRECT, (WPARAM)Index, (LPARAM)&blockRect))
        return;
    if (!RectVisible(bufferDc, &blockRect))
        return;

    // SB_GETTEXTLENGTH returns the length in the LOWORD and the SBT_* drawing type in the HIWORD.
    result = (ULONG_PTR)CallWindowProc(PhDefaultStatusbarWindowProcedure, WindowHandle, SB_GETTEXTLENGTH, (WPARAM)Index, 0);
    textLength = LOWORD(result);
    textStyle = HIWORD(result);

    if (Index == Context->HotIndex)
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

    if (textStyle & SBT_OWNERDRAW)
    {
        DRAWITEMSTRUCT drawItem;

        // Owner-drawn parts store application data instead of text and are painted by the parent.
        memset(&drawItem, 0, sizeof(DRAWITEMSTRUCT));
        drawItem.CtlID = GetDlgCtrlID(WindowHandle);
        drawItem.itemID = Index;
        drawItem.itemAction = ODA_DRAWENTIRE;
        drawItem.hwndItem = WindowHandle;
        drawItem.hDC = bufferDc;
        drawItem.rcItem = blockRect;
        drawItem.itemData = (ULONG_PTR)CallWindowProc(PhDefaultStatusbarWindowProcedure, WindowHandle, SB_GETTEXT, (WPARAM)Index, (LPARAM)textBuffer);

        SendMessage(GetParent(WindowHandle), WM_DRAWITEM, drawItem.CtlID, (LPARAM)&drawItem);
        return;
    }

    if (iconHandle = (HICON)CallWindowProc(PhDefaultStatusbarWindowProcedure, WindowHandle, SB_GETICON, (WPARAM)Index, 0))
    {
        LONG iconSize = blockRect.bottom - blockRect.top;

        DrawIconEx(bufferDc, blockRect.left + 2, blockRect.top, iconHandle, iconSize, iconSize, 0, NULL, DI_NORMAL);
        blockRect.left += iconSize + 2;
    }

    if (textLength == 0)
        return;

    // SB_GETTEXT has no buffer length; allocate when the text exceeds the stack buffer.
    if (textLength >= RTL_NUMBER_OF(textBuffer))
        text = PhAllocateZero((textLength + 1) * sizeof(WCHAR));

    CallWindowProc(PhDefaultStatusbarWindowProcedure, WindowHandle, SB_GETTEXT, (WPARAM)Index, (LPARAM)text);

    blockRect.left += 2, blockRect.bottom -= 1;
    DrawText(
        bufferDc,
        text,
        (UINT)PhCountStringZ(text),
        &blockRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX | ((textStyle & SBT_RTLREADING) ? DT_RTLREADING : 0)
        );
    blockRect.left -= 2, blockRect.bottom += 1;

    if (text != textBuffer)
        PhFree(text);
}

/**
 * Returns the number of drawable status bar parts.
 *
 * \param WindowHandle A handle to the status bar window.
 * \return LONG The part count; 1 when the status bar is in simple mode.
 */
LONG ThemeWindowStatusBarGetPartCount(
    _In_ HWND WindowHandle
    )
{
    // In simple mode index 0 maps to the simple part for SB_GETRECT and SB_GETTEXT.
    if (CallWindowProc(PhDefaultStatusbarWindowProcedure, WindowHandle, SB_ISSIMPLE, 0, 0))
        return 1;

    return (LONG)CallWindowProc(PhDefaultStatusbarWindowProcedure, WindowHandle, SB_GETPARTS, 0, 0);
}

/**
 * Refreshes the cached DPI and theme handle of a status bar.
 *
 * \param Context A pointer to the status bar theme context.
 * \param WindowHandle A handle to the status bar window.
 */
VOID ThemeWindowStatusBarUpdateTheme(
    _In_ PPHP_THEME_WINDOW_STATUSBAR_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    if (Context->ThemeHandle)
    {
        PhCloseThemeData(Context->ThemeHandle);
        Context->ThemeHandle = NULL;
    }

    Context->WindowDpi = PhGetWindowDpi(WindowHandle);
    Context->ThemeHandle = PhOpenThemeData(WindowHandle, VSCLASS_STATUS, Context->WindowDpi);
}

/**
 * Finds the status bar part under a client point.
 *
 * \param WindowHandle A handle to the status bar window.
 * \param Point The point in client coordinates.
 * \return LONG The part index, or INT_ERROR if no part contains the point.
 */
LONG ThemeWindowStatusBarHitTest(
    _In_ HWND WindowHandle,
    _In_ PPOINT Point
    )
{
    LONG count = ThemeWindowStatusBarGetPartCount(WindowHandle);

    for (LONG i = 0; i < count; i++)
    {
        RECT blockRect = { 0 };

        if (!CallWindowProc(PhDefaultStatusbarWindowProcedure, WindowHandle, SB_GETRECT, (WPARAM)i, (LPARAM)&blockRect))
            continue;

        if (PhPtInRect(&blockRect, Point))
            return i;
    }

    return INT_ERROR;
}

/**
 * Invalidates a single status bar part, including the hot highlight margin.
 *
 * \param WindowHandle A handle to the status bar window.
 * \param Index The part index, or INT_ERROR for none.
 */
VOID ThemeWindowStatusBarInvalidatePart(
    _In_ HWND WindowHandle,
    _In_ LONG Index
    )
{
    RECT blockRect = { 0 };

    if (Index == INT_ERROR)
        return;
    if (!CallWindowProc(PhDefaultStatusbarWindowProcedure, WindowHandle, SB_GETRECT, (WPARAM)Index, (LPARAM)&blockRect))
        return;

    // ThemeWindowStatusBarDrawPart extends the hot fill 3px left and 1px up.
    blockRect.left -= 3, blockRect.top -= 1;
    InvalidateRect(WindowHandle, &blockRect, FALSE);
}

/**
 * Renders all status bar parts and background with theme support.
 *
 * \param Context A pointer to the status bar theme context.
 * \param WindowHandle A handle to the status bar window.
 * \param bufferDc The device context used for drawing.
 * \param clientRect A pointer to the client area rectangle.
 */
VOID ThemeWindowRenderStatusBar(
    _In_ PPHP_THEME_WINDOW_STATUSBAR_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC bufferDc,
    _In_ PRECT clientRect
    )
{
    SetBkMode(bufferDc, TRANSPARENT);
    //SelectFont(bufferDc, GetWindowFont(WindowHandle));

    FillRect(bufferDc, clientRect, PhThemeWindowBackgroundBrush);

    LONG blockCount = ThemeWindowStatusBarGetPartCount(WindowHandle);

    //INT index = ThemeWindowStatusBarUpdateRectToIndex( // used with BeginBufferedPaint (dmex)
    //    WindowHandle,
    //    WindowProcedure,
    //    clientRect,
    //    blockCount
    //    );
    //
    //if (index == UINT_MAX)
    {
        // Top statusbar border will be drawn by bottom tabcontrol border

        for (LONG i = 0; i < blockCount; i++)
        {
            ThemeWindowStatusBarDrawPart(Context, WindowHandle, bufferDc, clientRect, i);
        }

        // Draw the gripper last: the final part extends underneath it.
        if ((PhGetWindowStyle(WindowHandle) & SBARS_SIZEGRIP) && !IsZoomed(GetParent(WindowHandle)))
        {
            RECT sizeGripRect;
            LONG dpi;

            dpi = Context->WindowDpi;
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
        }
    }
    //else
    //{
    //    ThemeWindowStatusBarDrawPart(Context, WindowHandle, bufferDc, clientRect, WindowProcedure, index);
    //}
}

/**
 * Window hook procedure for superclassed status bar controls (msctls_statusbar32).
 *
 * \param WindowHandle A handle to the status bar control window.
 * \param WindowMessage The window message.
 * \param wParam Additional message-dependent information.
 * \param lParam Additional message-dependent information.
 * \return LRESULT The result of the message processing.
 */
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
        context->HotIndex = INT_ERROR;
        ThemeWindowStatusBarUpdateTheme(context, WindowHandle);
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

                if (context->ThemeHandle)
                {
                    PhCloseThemeData(context->ThemeHandle);
                }

                PhFree(context);
                context = NULL;
            }
            break;
        case WM_THEMECHANGED:
            {
                ThemeWindowStatusBarUpdateTheme(context, WindowHandle);
            }
            break;
        case WM_DPICHANGED_AFTERPARENT:
            {
                ThemeWindowStatusBarUpdateTheme(context, WindowHandle);
                InvalidateRect(WindowHandle, NULL, FALSE);
            }
            break;
        case WM_ERASEBKGND:
            {
                // WM_PAINT fills the entire update region (buffered), so erasing here
                // only causes a visible background flash before the buffered blit.
                return TRUE;
            }
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

                POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                LONG hotIndex = ThemeWindowStatusBarHitTest(WindowHandle, &point);

                // Repaint only the parts whose hot state changed.
                if (context->HotIndex != hotIndex)
                {
                    ThemeWindowStatusBarInvalidatePart(WindowHandle, context->HotIndex);
                    ThemeWindowStatusBarInvalidatePart(WindowHandle, hotIndex);
                    context->HotIndex = hotIndex;
                }
            }
            break;
        case WM_MOUSELEAVE:
            {
                context->MouseActive = FALSE;

                ThemeWindowStatusBarInvalidatePart(WindowHandle, context->HotIndex);
                context->HotIndex = INT_ERROR;
            }
            break;
        case WM_PAINT:
            {
                PAINTSTRUCT paintStruct;
                RECT clientRect;
                PH_BUFFERED_PAINT paintBuffer;
                HDC bufferDc;
                HDC hdc;

                if (!(hdc = BeginPaint(WindowHandle, &paintStruct)))
                    break;

                if (PhRectEmpty(&paintStruct.rcPaint) || !PhGetClientRect(WindowHandle, &clientRect))
                {
                    EndPaint(WindowHandle, &paintStruct);
                    return 0;
                }

                // Buffer only the invalidated region; ThemeWindowRenderStatusBar still
                // lays out using the full client rect and is clipped to the rcPaint buffer.
                if (PhBeginBufferedPaint(hdc, &paintStruct.rcPaint, PHBF_TOPDOWNDIB, NULL, &paintBuffer, &bufferDc))
                {
                    ThemeWindowRenderStatusBar(context, WindowHandle, bufferDc, &clientRect);
                    PhEndBufferedPaint(&paintBuffer, TRUE);
                }
                else
                {
                    ThemeWindowRenderStatusBar(context, WindowHandle, hdc, &clientRect);
                }

                EndPaint(WindowHandle, &paintStruct);
            }
            return 0;
        }
    }

    return CallWindowProc(PhDefaultStatusbarWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);
}

/**
 * Window hook procedure for superclassed edit controls (Edit).
 *
 * \param WindowHandle A handle to the edit control window.
 * \param WindowMessage The window message.
 * \param wParam Additional message-dependent information.
 * \param lParam Additional message-dependent information.
 * \return LRESULT The result of the message processing.
 */
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

            if (!PhEnableThemeSupport)
                break;

            // The searchbox control does its own theme drawing.
            if (PhGetWindowContext(WindowHandle, SHRT_MAX))
                break;
            if (!PhGetWindowRect(WindowHandle, &windowRect))
                break;

            updateRegion = (HRGN)wParam;

            if (updateRegion != HRGN_FULL && updateRegion != NULL)
            {
                if (!RectInRegion(updateRegion, &windowRect))
                    return FALSE;   // frame area isn't dirty at all — skip GetDCEx entirely
            }

            if (updateRegion == HRGN_FULL)
                updateRegion = NULL;

            flags = DCX_WINDOW | DCX_CACHE | DCX_USESTYLE;

            if (updateRegion)
                flags |= DCX_INTERSECTRGN | DCX_NODELETERGN;

            if (hdc = GetDCEx(WindowHandle, updateRegion, flags))
            {
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
    LONG WindowDpi;
    HTHEME ThemeHandle;
    BOOLEAN MouseActive;
    POINT CursorPos;
} PHP_THEME_WINDOW_HEADER_CONTEXT, *PPHP_THEME_WINDOW_HEADER_CONTEXT;

/**
 * Renders a themed header control including columns, dividers, and sort indicators.
 *
 * \param Context A pointer to the header theme context.
 * \param WindowHandle A handle to the header control window.
 * \param bufferDc The device context used for drawing.
 * \param clientRect A pointer to the client area rectangle.
 */
VOID ThemeWindowRenderHeaderControl(
    _In_ PPHP_THEME_WINDOW_HEADER_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC bufferDc,
    _In_ PRECT clientRect
    )
{
    HIMAGELIST imageList;

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

    imageList = (HIMAGELIST)CallWindowProc(
        PhDefaultHeaderWindowProcedure,
        WindowHandle,
        HDM_GETIMAGELIST,
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

        if (!RectVisible(bufferDc, &headerRect))
            continue;

        if (PhPtInRect(&headerRect, &Context->CursorPos))
        {
            SetTextColor(bufferDc, PhThemeWindowTextColor);
            SetDCBrushColor(bufferDc, PhThemeWindowBackground2Color); // PhThemeWindowHighlightColor);
            FillRect(bufferDc, &headerRect, PhGetStockBrush(DC_BRUSH));
        }
        else
        {
            SetTextColor(bufferDc, PhThemeWindowTextColor);
            FillRect(bufferDc, &headerRect, PhThemeWindowBackgroundBrush);
            DrawEdge(bufferDc, &headerRect, BDR_RAISEDOUTER, BF_RIGHT);
        }

        INT drawTextFlags = DT_SINGLELINE | DT_HIDEPREFIX | DT_WORD_ELLIPSIS | DT_VCENTER;
        WCHAR headerText[0x80] = { 0 };
        HDITEM headerItem;

        ZeroMemory(&headerItem, sizeof(HDITEM));
        headerItem.mask = HDI_TEXT | HDI_FORMAT | HDI_IMAGE | HDI_LPARAM;
        headerItem.cchTextMax = RTL_NUMBER_OF(headerText);
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
            continue;
        }

        if (headerItem.fmt & HDF_OWNERDRAW)
        {
            DRAWITEMSTRUCT drawItem;

            // Owner-drawn items are painted by the parent over our background.
            memset(&drawItem, 0, sizeof(DRAWITEMSTRUCT));
            drawItem.CtlType = ODT_HEADER;
            drawItem.CtlID = GetDlgCtrlID(WindowHandle);
            drawItem.itemID = i;
            drawItem.itemAction = ODA_DRAWENTIRE;
            drawItem.hwndItem = WindowHandle;
            drawItem.hDC = bufferDc;
            drawItem.rcItem = headerRect;
            drawItem.itemData = (ULONG_PTR)headerItem.lParam;

            SendMessage(GetParent(WindowHandle), WM_DRAWITEM, drawItem.CtlID, (LPARAM)&drawItem);
            continue;
        }

        if (headerItem.fmt & (HDF_SORTUP | HDF_SORTDOWN))
        {
            if (Context->ThemeHandle)
            {
                INT sortArrowState = (headerItem.fmt & HDF_SORTUP) ? HSAS_SORTEDUP : HSAS_SORTEDDOWN;
                RECT sortArrowRect = headerRect;
                SIZE sortArrowSize;

                if (PhGetThemePartSize(
                    Context->ThemeHandle,
                    bufferDc,
                    HP_HEADERSORTARROW,
                    sortArrowState,
                    NULL,
                    THEMEPARTSIZE_TRUE,
                    &sortArrowSize
                    ))
                {
                    sortArrowRect.bottom = sortArrowSize.cy;
                }

                PhDrawThemeBackground(
                    Context->ThemeHandle,
                    bufferDc,
                    HP_HEADERSORTARROW,
                    sortArrowState,
                    &sortArrowRect,
                    NULL
                    );
            }
        }

        headerRect.left += 4;
        headerRect.right -= 8;

        if ((headerItem.fmt & HDF_IMAGE) && imageList && headerItem.iImage >= 0)
        {
            LONG iconWidth;
            LONG iconHeight;

            if (PhImageListGetIconSize(imageList, &iconWidth, &iconHeight))
            {
                PhImageListDrawIcon(
                    imageList,
                    headerItem.iImage,
                    bufferDc,
                    headerRect.left,
                    headerRect.top + ((headerRect.bottom - headerRect.top) - iconHeight) / 2,
                    ILD_NORMAL,
                    FALSE
                    );

                headerRect.left += iconWidth + 4;
            }
        }

        switch (headerItem.fmt & HDF_JUSTIFYMASK)
        {
        case HDF_RIGHT:
            drawTextFlags |= DT_RIGHT;
            break;
        case HDF_CENTER:
            drawTextFlags |= DT_CENTER;
            break;
        default:
            drawTextFlags |= DT_LEFT;
            break;
        }

        if (headerItem.fmt & HDF_RTLREADING)
            drawTextFlags |= DT_RTLREADING;

        DrawText(
            bufferDc,
            headerText,
            (UINT)PhCountStringZ(headerText),
            &headerRect,
            drawTextFlags
            );
    }
}

/**
 * Invalidates only header items whose hover state changed.
 */
static VOID PhpHeaderInvalidateHoverChange(
    _In_ HWND WindowHandle,
    _In_ POINT OldPoint,
    _In_ POINT NewPoint
    )
{
    INT count = (INT)CallWindowProc(PhDefaultHeaderWindowProcedure,
        WindowHandle, HDM_GETITEMCOUNT, 0, 0);

    // Use the same rectangles as the painter, including divider pixels; native
    // hit-test flags do not exactly match its point-in-rectangle hover rule.
    for (INT i = 0; i < count; i++)
    {
        RECT rect;
        if (!CallWindowProc(PhDefaultHeaderWindowProcedure,
            WindowHandle, HDM_GETITEMRECT, (WPARAM)i, (LPARAM)&rect))
            continue;

        if (!!PhPtInRect(&rect, &OldPoint) != !!PhPtInRect(&rect, &NewPoint))
            InvalidateRect(WindowHandle, &rect, FALSE);
    }
}

/**
 * Refreshes the cached DPI and theme handle of a header control.
 *
 * \param Context A pointer to the header theme context.
 * \param WindowHandle A handle to the header control window.
 */
static VOID PhpHeaderUpdateTheme(
    _In_ PPHP_THEME_WINDOW_HEADER_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    if (Context->ThemeHandle)
    {
        PhCloseThemeData(Context->ThemeHandle);
        Context->ThemeHandle = NULL;
    }

    Context->WindowDpi = PhGetWindowDpi(WindowHandle);
    Context->ThemeHandle = PhOpenThemeData(WindowHandle, VSCLASS_HEADER, Context->WindowDpi);
}

/**
 * Window hook procedure for superclassed header controls (SysHeader32).
 *
 * \param WindowHandle A handle to the header control window.
 * \param WindowMessage The window message.
 * \param wParam Additional message-dependent information.
 * \param lParam Additional message-dependent information.
 * \return LRESULT The result of the message processing.
 */
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

            if (!NT_SUCCESS(PhGetClassName(createStruct->hwndParent, windowClassName, RTL_NUMBER_OF(windowClassName), NULL)))
                windowClassName[0] = UNICODE_NULL;

            if (PhEqualStringZ(windowClassName, L"PhTreeNew", FALSE))
            {
                ULONG windowStyle = PhGetWindowStyle(createStruct->hwndParent);

                if (BooleanFlagOn(windowStyle, TN_STYLE_CUSTOM_HEADERDRAW))
                {
                    PhSetControlTheme(WindowHandle, L"DarkMode_ItemsView");

                    return CallWindowProc(PhDefaultHeaderWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);
                }
            }
        }

        context = PhAllocateZero(sizeof(PHP_THEME_WINDOW_HEADER_CONTEXT));
        context->CursorPos.x = LONG_MIN;
        context->CursorPos.y = LONG_MIN;
        PhpHeaderUpdateTheme(context, WindowHandle);
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
                context = NULL;
            }
            break;
        case WM_THEMECHANGED:
            {
                PhpHeaderUpdateTheme(context, WindowHandle);
            }
            break;
        case WM_DPICHANGED_AFTERPARENT:
            {
                PhpHeaderUpdateTheme(context, WindowHandle);
                InvalidateRect(WindowHandle, NULL, FALSE);
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

                POINT oldPoint = context->CursorPos;
                context->CursorPos.x = GET_X_LPARAM(lParam);
                context->CursorPos.y = GET_Y_LPARAM(lParam);

                PhpHeaderInvalidateHoverChange(WindowHandle, oldPoint, context->CursorPos);
            }
            break;
        case WM_CAPTURECHANGED:
            {
                POINT oldPoint = context->CursorPos;

                // Hover updates are suspended during a resize/drag capture; drop the
                // stale hot item so it does not linger until the next mouse move.
                context->CursorPos.x = LONG_MIN;
                context->CursorPos.y = LONG_MIN;

                PhpHeaderInvalidateHoverChange(WindowHandle, oldPoint, context->CursorPos);
            }
            break;
        case WM_CONTEXTMENU:
            {
                LRESULT result = CallWindowProc(PhDefaultHeaderWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);

                InvalidateRect(WindowHandle, NULL, FALSE);

                return result;
            }
            break;
        case WM_MOUSELEAVE:
            {
                LRESULT result = CallWindowProc(PhDefaultHeaderWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);

                POINT oldPoint = context->CursorPos;
                context->MouseActive = FALSE;
                context->CursorPos.x = LONG_MIN;
                context->CursorPos.y = LONG_MIN;

                PhpHeaderInvalidateHoverChange(WindowHandle, oldPoint, context->CursorPos);

                return result;
            }
            break;
        case WM_PAINT:
            {
                // Don't apply header theme for unsupported dialogs: Advanced Security, Digital Signature Details, etc. (Dart Vanya)
                if (!PhIsDarkModeAllowedForWindow(GetParent(WindowHandle)))
                {
                    PhRemoveWindowContext(WindowHandle, LONG_MAX);
                    if (context->ThemeHandle)
                        PhCloseThemeData(context->ThemeHandle);
                    PhFree(context);
                    context = NULL;
                    PhSetControlTheme(WindowHandle, L"Explorer");
                    break;
                }

                {
                    PAINTSTRUCT ps;
                    RECT clientRect;
                    PH_BUFFERED_PAINT paintBuffer;
                    HDC bufferDc;
                    HDC hdc;

                    if (!(hdc = BeginPaint(WindowHandle, &ps)))
                        break;

                    if (PhRectEmpty(&ps.rcPaint) || !PhGetClientRect(WindowHandle, &clientRect))
                    {
                        EndPaint(WindowHandle, &ps);
                        return 0;
                    }

                    // Buffer only the invalidated region; ThemeWindowRenderHeaderControl
                    // lays out using the full client rect and is clipped to the rcPaint buffer.
                    if (PhBeginBufferedPaint(hdc, &ps.rcPaint, PHBF_TOPDOWNDIB, NULL, &paintBuffer, &bufferDc))
                    {
                        ThemeWindowRenderHeaderControl(context, WindowHandle, bufferDc, &clientRect);
                        PhEndBufferedPaint(&paintBuffer, TRUE);
                    }
                    else
                    {
                        ThemeWindowRenderHeaderControl(context, WindowHandle, hdc, &clientRect);
                    }

                    EndPaint(WindowHandle, &ps);
                }
            }
            return 0;
        }
    }

    return CallWindowProc(PhDefaultHeaderWindowProcedure, WindowHandle, WindowMessage, wParam, lParam);
}

/**
 * Superclasses the standard Windows dialog class (#32770).
 */
VOID PhRegisterDialogSuperClass(
    VOID
    )
{
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

    if (!GetClassInfoEx(NULL, L"#32770", &wcex))
        return;

    PhDefaultDialogWindowProcedure = wcex.lpfnWndProc;
    wcex.lpfnWndProc = PhDialogWindowHookProcedure;
    wcex.style = wcex.style | CS_GLOBALCLASS;

    UnregisterClass(L"#32770", NULL);
    if (RegisterClassEx(&wcex) == INVALID_ATOM)
    {
        PhShowStatus(NULL, L"Unable to register window class.", 0, GetLastError());
    }
}

/**
 * Superclasses the standard Windows menu class (#32768).
 */
VOID PhRegisterMenuSuperClass(
    VOID
    )
{
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

    if (!GetClassInfoEx(NULL, L"#32768", &wcex))
        return;

    PhDefaultMenuWindowProcedure = wcex.lpfnWndProc;
    wcex.lpfnWndProc = PhMenuWindowHookProcedure;
    wcex.style |= CS_GLOBALCLASS;

    // Windows 11 ignores CS_DROPSHADOW for this class and uses the layered
    // shadow window created from WM_CREATE instead. (dmex)
    if (WindowsVersion < WINDOWS_11)
        wcex.style |= CS_DROPSHADOW;

    UnregisterClass(L"#32768", NULL);
    if (RegisterClassEx(&wcex) == INVALID_ATOM)
    {
        PhShowStatus(NULL, L"Unable to register window class.", 0, GetLastError());
    }
}

/**
 * Superclasses the standard rebar control class (ReBarWindow32).
 */
VOID PhRegisterRebarSuperClass(
    VOID
    )
{
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

    if (!GetClassInfoEx(NULL, REBARCLASSNAME, &wcex))
        return;

    PhDefaultRebarWindowProcedure = wcex.lpfnWndProc;
    wcex.lpfnWndProc = PhRebarWindowHookProcedure;
    wcex.style = wcex.style | CS_GLOBALCLASS;

    UnregisterClass(REBARCLASSNAME, NULL);
    if (RegisterClassEx(&wcex) == INVALID_ATOM)
    {
        PhShowStatus(NULL, L"Unable to register window class.", 0, GetLastError());
    }
}

/**
 * Superclasses the standard combo box control class (ComboBox).
 */
VOID PhRegisterComboBoxSuperClass(
    VOID
    )
{
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

    if (!GetClassInfoEx(NULL, WC_COMBOBOX, &wcex))
        return;

    PhDefaultComboBoxWindowProcedure = wcex.lpfnWndProc;
    wcex.lpfnWndProc = PhComboBoxWindowHookProcedure;
    wcex.style = wcex.style | CS_GLOBALCLASS;

    UnregisterClass(WC_COMBOBOX, NULL);
    if (RegisterClassEx(&wcex) == INVALID_ATOM)
    {
        PhShowStatus(NULL, L"Unable to register window class.", 0, GetLastError());
    }
}

/**
 * Superclasses the standard static control class (Static).
 */
VOID PhRegisterStaticSuperClass(
    VOID
    )
{
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

    if (!GetClassInfoEx(NULL, WC_STATIC, &wcex))
        return;

    PhDefaultStaticWindowProcedure = wcex.lpfnWndProc;
    wcex.lpfnWndProc = PhStaticWindowHookProcedure;
    wcex.style = wcex.style | CS_GLOBALCLASS;

    UnregisterClass(WC_STATIC, NULL);
    if (RegisterClassEx(&wcex) == INVALID_ATOM)
    {
        PhShowStatus(NULL, L"Unable to register window class.", 0, GetLastError());
    }
}

/**
 * Superclasses the standard status bar control class (msctls_statusbar32).
 */
VOID PhRegisterStatusBarSuperClass(
    VOID
    )
{
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

    if (!GetClassInfoEx(NULL, STATUSCLASSNAME, &wcex))
        return;

    PhDefaultStatusbarWindowProcedure = wcex.lpfnWndProc;
    wcex.lpfnWndProc = PhStatusBarWindowHookProcedure;
    wcex.style = wcex.style | CS_GLOBALCLASS;

    UnregisterClass(STATUSCLASSNAME, NULL);
    if (RegisterClassEx(&wcex) == INVALID_ATOM)
    {
        PhShowStatus(NULL, L"Unable to register window class.", 0, GetLastError());
    }
}

/**
 * Superclasses the standard edit control class (Edit).
 */
VOID PhRegisterEditSuperClass(
    VOID
    )
{
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

    if (!GetClassInfoEx(NULL, WC_EDIT, &wcex))
        return;

    PhDefaultEditWindowProcedure = wcex.lpfnWndProc;
    wcex.lpfnWndProc = PhEditWindowHookProcedure;
    wcex.style = wcex.style | CS_GLOBALCLASS;

    UnregisterClass(WC_EDIT, NULL);
    if (RegisterClassEx(&wcex) == INVALID_ATOM)
    {
        PhShowStatus(NULL, L"Unable to register window class.", 0, GetLastError());
    }
}

/**
 * Superclasses the standard header control class (SysHeader32).
 */
VOID PhRegisterHeaderSuperClass(
    VOID
    )
{
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

    if (!GetClassInfoEx(NULL, WC_HEADER, &wcex))
        return;

    PhDefaultHeaderWindowProcedure = wcex.lpfnWndProc;
    wcex.lpfnWndProc = PhHeaderWindowHookProcedure;
    wcex.style = wcex.style | CS_GLOBALCLASS;

    UnregisterClass(WC_HEADER, NULL);
    if (RegisterClassEx(&wcex) == INVALID_ATOM)
    {
        PhShowStatus(NULL, L"Unable to register window class.", 0, GetLastError());
    }
}

// Detours export procedure hooks

static typeof(&DrawThemeBackground) DefaultDrawThemeBackground = NULL;
static typeof(&DrawThemeBackgroundEx) DefaultDrawThemeBackgroundEx = NULL;
static typeof(&DrawThemeText) DefaultDrawThemeText = NULL;
static typeof(&DrawThemeTextEx) DefaultDrawThemeTextEx = NULL;
static typeof(&DrawTextW) DefaultComCtl32DrawTextW = NULL;
static typeof(&TaskDialogIndirect) DefaultTaskDialogIndirect = NULL;
static typeof(&GetThemeColor) DefaultGetThemeColor = NULL;
static typeof(&SystemParametersInfoW) DefaultSystemParametersInfo = NULL;
static typeof(&CreateWindowExW) DefaultCreateWindowEx = NULL;

// uxtheme.dll ordinal 49
static HTHEME(WINAPI* DefaultOpenNcThemeData)(
    _In_ HWND    hwnd,
    _In_ LPCWSTR pszClassList
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

#define GETCLASSNAME_OR_NULL(WindowHandle, ClassName) if (!NT_SUCCESS(PhGetClassName(WindowHandle, ClassName, RTL_NUMBER_OF(ClassName), NULL))) ClassName[0] = UNICODE_NULL

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

_Function_class_(PH_WINDOW_ENUM_CALLBACK)
BOOLEAN CALLBACK PhInitializeTaskDialogTheme(
    _In_ HWND hwndDlg,
    _In_opt_ PVOID Context
    );

/**
 * Detour hook for DrawThemeBackground to provide custom theme rendering.
 *
 * \param Theme Theme data handle.
 * \param Hdc Device context used for drawing.
 * \param PartId The theme part to draw.
 * \param StateId The state of the theme part.
 * \param Rect A pointer to the destination drawing rectangle.
 * \param ClipRect A pointer to the clipping rectangle.
 * \return HRESULT Successful or errant status.
 */
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
    BOOLEAN hasThemeClass = PhGetThemeClass(Theme, className, RTL_NUMBER_OF(className));

    if (WindowsVersion >= WINDOWS_11 && hasThemeClass)
    {
        if (PhEqualStringZ(className, VSCLASS_MENU, TRUE))
        {
            if (PartId == MENU_POPUPGUTTER || PartId == MENU_POPUPBORDERS)
            {
                FillRect(Hdc, Rect, PhThemeWindowBackgroundBrush);

                if (PartId == MENU_POPUPBORDERS)
                {
                    COLORREF oldBrushColor;

                    oldBrushColor = SetDCBrushColor(Hdc, PhThemeWindowBorderColor);
                    FrameRect(Hdc, Rect, PhGetStockBrush(DC_BRUSH));
                    SetDCBrushColor(Hdc, oldBrushColor);
                }

                return S_OK;
            }
        }
    }

    if (hasThemeClass && PhEqualStringZ(className, VSCLASS_PROGRESS, TRUE)
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

    return DefaultDrawThemeBackground(Theme, Hdc, PartId, StateId, Rect, ClipRect);
}

/**
 * Detour hook for DrawThemeBackgroundEx to provide custom theme rendering.
 *
 * \param hTheme Theme data handle.
 * \param hdc Device context used for drawing.
 * \param iPartId The theme part to draw.
 * \param iStateId The state of the theme part.
 * \param pRect A pointer to the destination drawing rectangle.
 * \param pOptions A pointer to drawing options.
 * \return HRESULT Successful or errant status.
 */
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
            HRESULT retVal = DefaultDrawThemeBackgroundEx(darkButtonTheme ? darkButtonTheme : hTheme, hdc, iPartId, iStateId, pRect, pOptions);
            if (darkButtonTheme)
                PhCloseThemeData(darkButtonTheme);
            return retVal;
        }
    }

    // Micro optimization
    if ((iPartId == TDLG_PRIMARYPANEL || iPartId == TDLG_FOOTNOTEPANE || iPartId == TDLG_SECONDARYPANEL || iPartId == TDLG_FOOTNOTESEPARATOR || iPartId == TDLG_EXPANDOBUTTON) &&
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

    return DefaultDrawThemeBackgroundEx(hTheme, hdc, iPartId, iStateId, pRect, pOptions);
}

/**
 * Detour hook for CreateWindowExW to apply streamer mode, acrylic backdrop, and theme settings.
 *
 * \param ExStyle The extended window style.
 * \param ClassName The window class name.
 * \param WindowName The window title text.
 * \param Style The window style flags.
 * \param X The initial horizontal position.
 * \param Y The initial vertical position.
 * \param Width The initial window width.
 * \param Height The initial window height.
 * \param Parent A handle to the parent or owner window.
 * \param Menu A handle to a menu, or child-window identifier.
 * \param Instance A handle to the module instance.
 * \param Param Pointer to window creation data.
 * \return HWND Handle to the created window, or NULL if creation failed.
 */
HWND WINAPI PhCreateWindowExHook(
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

    // Preserve the creation error and never pass a failed HWND to helpers.
    if (!windowHandle)
        return NULL;

    // An owned popup is top-level too. A message-only window is not visual.
    if (!(Style & WS_CHILD) && Parent != HWND_MESSAGE)
    {
        if (PhDefaultEnableStreamerMode)
        {
            SetWindowDisplayAffinity(windowHandle, WDA_EXCLUDEFROMCAPTURE);
        }

        // Preserve the prior acrylic policy for unowned top-level windows;
        // owned menus/tooltips/popups must not acquire a generic backdrop.
        if (!Parent && !(ExStyle & (WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP)) &&
            PhEnableThemeSupport && PhDefaultEnableThemeAcrylicWindowSupport)
        {
            PhSetWindowAcrylicCompositionColor(windowHandle, MakeABGRFromCOLORREF(0, RGB(10, 10, 10)));
        }
    }
    else if ((Style & WS_CHILD) && PhEnableThemeSupport)
    {
        // Early subclassing of the SysLink control to eliminate blinking during page switches.
        if (!IS_INTRESOURCE(ClassName) && PhEqualStringZ(ClassName, WC_LINK, TRUE))
        {
            PhInitializeTaskDialogTheme(windowHandle, 0);
        }
        else if (!IS_INTRESOURCE(ClassName) && PhEqualStringZ(ClassName, WC_BUTTON, TRUE) &&
                 PhGetWindowContext(GetAncestor(Parent, GA_ROOT), LONG_MAX))
        {
            PhSetControlTheme(windowHandle, L"DarkMode_Explorer");
        }
    }

    return windowHandle;
}

/**
 * Detour hook for SystemParametersInfoW to disable animations when theme animation is disabled.
 *
 * \param uiAction The system parameter action to query or set.
 * \param uiParam Parameter dependent on the action.
 * \param pvParam Parameter dependent on the action.
 * \param fWinIni User profile update flags.
 * \return BOOL TRUE if successful, FALSE otherwise.
 */
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

/**
 * Detour hook for DrawThemeText to provide custom text coloring for theme controls.
 *
 * \param hTheme Theme data handle.
 * \param hdc Device context used for drawing.
 * \param iPartId The theme part.
 * \param iStateId The state of the theme part.
 * \param pszText The string to draw.
 * \param cchText The character count in the string.
 * \param dwTextFlags Text formatting flags.
 * \param dwTextFlags2 Additional text formatting flags.
 * \param pRect A pointer to the text layout rectangle.
 * \return HRESULT Successful or errant status.
 */
HRESULT WINAPI PhDrawThemeTextHook(
    _In_ HTHEME  hTheme,
    _In_ HDC     hdc,
    _In_ LONG    iPartId,
    _In_ LONG    iStateId,
    _In_ LPCWSTR pszText,
    _In_ LONG    cchText,
    _In_ ULONG   dwTextFlags,
    _In_ ULONG   dwTextFlags2,
    _In_ LPCRECT pRect
    )
{
    if ((iPartId == BP_COMMANDLINK /*|| iPartId == BP_RADIOBUTTON*/) && iStateId != PBS_DISABLED)
    {
        WCHAR className[MAX_PATH];
        if (PhGetThemeClass(hTheme, className, RTL_NUMBER_OF(className)) && PhEqualStringZ(className, VSCLASS_BUTTON, TRUE)
            /*|| WindowsVersion < WINDOWS_11 && WindowFromDC(hdc) == NULL*/)
        {
            DTTOPTS options = { sizeof(DTTOPTS), DTT_TEXTCOLOR, PhThemeWindowTextColor };
            return DefaultDrawThemeTextEx(hTheme, hdc, iPartId, iStateId, pszText, cchText, dwTextFlags, (LPRECT)pRect, &options);
        }
    }

    return DefaultDrawThemeText(hTheme, hdc, iPartId, iStateId, pszText, cchText, dwTextFlags, dwTextFlags2, pRect);
}

/**
 * Detour hook for DrawThemeTextEx to provide custom text coloring for themed buttons and command links.
 *
 * \param hTheme Theme data handle.
 * \param hdc Device context used for drawing.
 * \param iPartId The theme part.
 * \param iStateId The state of the theme part.
 * \param pszText The string to draw.
 * \param cchText The character count in the string.
 * \param dwTextFlags Text formatting flags.
 * \param pRect A pointer to the text layout rectangle.
 * \param pOptions A pointer to text drawing options.
 * \return HRESULT Successful or errant status.
 */
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

    return DefaultDrawThemeTextEx(hTheme, hdc, iPartId, iStateId, pszText, cchText, dwTextFlags, pRect, pOptions);
}

/**
 * Detour import hook for DrawTextW in Comctl32.dll to adjust hyperlink text colors in dark mode.
 *
 * \param hdc Device context used for drawing.
 * \param lpchText The string to draw.
 * \param cchText The character count in the string.
 * \param lprc A pointer to the text layout rectangle.
 * \param format Text formatting flags.
 * \return int The height of the text drawn.
 */
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

    if ((WindowHandle = WindowFromDC(hdc)) &&
        (PhIsDarkModeAllowedForWindow(WindowHandle) || PhGetWindowContext(WindowHandle, TASKDIALOG_CONTEXT_TAG)))    // HACK
    {
        WCHAR windowClassName[MAX_PATH];
        GETCLASSNAME_OR_NULL(WindowHandle, windowClassName);
        if (PhEqualStringZ(windowClassName, WC_LINK, FALSE))
        {
            if (PhBeginInitOnce(&initOnce))
            {
                LONG windowDpi = PhGetWindowDpi(WindowHandle);
                HTHEME hTextTheme = PhOpenThemeData(WindowHandle, VSCLASS_TEXTSTYLE, windowDpi);
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

    return DefaultComCtl32DrawTextW(hdc, lpchText, cchText, lprc, format);
}

/**
 * Detour hook for GetThemeColor to override theme colors for task dialog elements in dark mode.
 *
 * \param hTheme Theme data handle.
 * \param iPartId The theme part.
 * \param iStateId The state of the theme part.
 * \param iPropId The property identifier.
 * \param pColor Receives the color value.
 * \return HRESULT Successful or errant status.
 */
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

    return retVal;
}

/**
 * Detour hook for OpenNcThemeData to apply dark scrollbar themes.
 *
 * \param hwnd A handle to the window.
 * \param pszClassList The theme class list.
 * \return HTHEME Theme data handle.
 */
HTHEME PhOpenNcThemeDataHook(
    _In_ HWND    hwnd,
    _In_ LPCWSTR pszClassList
    )
{
    if (PhEqualStringZ((PWSTR)pszClassList, VSCLASS_SCROLLBAR, TRUE) &&
        PhIsDarkModeAllowedForWindow(hwnd))
    {
        return DefaultOpenNcThemeData(NULL, L"Explorer::ScrollBar");
    }

    return DefaultOpenNcThemeData(hwnd, pszClassList);
}

/**
 * Window enumeration callback to initialize theme subclasses and controls for task dialogs.
 *
 * \param WindowHandle A handle to the window being processed.
 * \param CallbackData Optional task dialog callback wrapper structure.
 * \return BOOLEAN TRUE to continue window enumeration.
 */
_Function_class_(PH_WINDOW_ENUM_CALLBACK)
BOOLEAN CALLBACK PhInitializeTaskDialogTheme(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID CallbackData
    )
{
    WCHAR windowClassName[MAX_PATH];
    BOOLEAN windowHasContext = !!PhGetWindowContext(WindowHandle, TASKDIALOG_CONTEXT_TAG);

    if (CallbackData && !windowHasContext)
    {
        if (PhDefaultEnableStreamerMode)
        {
            SetWindowDisplayAffinity(WindowHandle, WDA_EXCLUDEFROMCAPTURE);
        }

        PhInitializeThemeWindowFrame(WindowHandle);

        PTASKDIALOG_WINDOW_CONTEXT context = PhAllocateZero(sizeof(TASKDIALOG_WINDOW_CONTEXT));
        context->DefaultWindowProc = PhSetWindowProcedure(WindowHandle, ThemeTaskDialogMasterSubclass);
        context->CallbackData = CallbackData;
        PhSetWindowContext(WindowHandle, TASKDIALOG_CONTEXT_TAG, context);
        windowHasContext = TRUE;
    }

    PhEnumChildWindows(
        WindowHandle,
        PhInitializeTaskDialogTheme,
        NULL
        );

    if (windowHasContext)    // HACK
        return TRUE;

    GETCLASSNAME_OR_NULL(WindowHandle, windowClassName);

    {
        PTASKDIALOG_COMMON_CONTEXT context;

        context = PhAllocateZero(sizeof(TASKDIALOG_COMMON_CONTEXT));
        context->DefaultWindowProc = PhSetWindowProcedure(WindowHandle, ThemeTaskDialogMasterSubclass);
        PhSetWindowContext(WindowHandle, TASKDIALOG_CONTEXT_TAG, context);

        if (PhEqualStringZ(windowClassName, WC_BUTTON, FALSE) ||
            PhEqualStringZ(windowClassName, WC_SCROLLBAR, FALSE))
        {
            PhSetControlTheme(WindowHandle, L"DarkMode_Explorer");
        }
        //else if (PhEqualStringZ(windowClassName, WC_LINK, FALSE))
        //{
        //    PhAllowDarkModeForWindow(WindowHandle);   // this doesn't work, idk why
        //}
        else if (PhEqualStringZ(windowClassName, L"DirectUIHWND", FALSE))
        {
            //WINDOWPLACEMENT pos = { 0 };
            //GetWindowPlacement(GetParent(WindowHandle), &pos);
            PhSetControlTheme(WindowHandle, L"DarkMode_Explorer");
            //SetWindowPlacement(GetParent(WindowHandle), &pos);
        }
    }

    return TRUE;
}

/**
 * Subclass window procedure for task dialogs and child controls.
 *
 * \param hwnd A handle to the window.
 * \param uMsg The window message.
 * \param wParam Additional message-dependent information.
 * \param lParam Additional message-dependent information.
 * \return LRESULT The result of the message processing.
 */
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

            SetTextColor(hdc, PhThemeWindowTextColor); // Color for SysLink, which must be set in its parent.

            if (!context->Painting)
            {
                GETCLASSNAME_OR_NULL(hwnd, windowClassName);
                // Avoid erasing the background for links, as they will blink white on the extender and during page switches.
                if (!PhEqualStringZ(windowClassName, WC_LINK, FALSE))
                {
                    if (GetClipBox(hdc, &rect) > NULLREGION)
                    {
                        SetDCBrushColor(hdc, PhThemeWindowBackground2Color);
                        FillRect(hdc, &rect, PhGetStockBrush(DC_BRUSH));
                    }
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

                if (!NT_SUCCESS(PhGetClassName(customDraw->hdr.hwndFrom, className, RTL_NUMBER_OF(className), NULL)))
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
            PhRemoveWindowContext(hwnd, TASKDIALOG_CONTEXT_TAG);
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

/**
 * Task dialog callback hook used to apply theme initialization on page construction.
 *
 * \param hwndDlg A handle to the task dialog window.
 * \param uMsg The notification message.
 * \param wParam Notification parameter.
 * \param lParam Notification parameter.
 * \param dwRefData User reference data pointer.
 * \return HRESULT Callback result.
 */
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
/**
 * Detour hook for TaskDialogIndirect to enable theme support and custom styling.
 *
 * \param pTaskConfig A pointer to the task dialog configuration structure.
 * \param pnButton Optional receives the selected button ID.
 * \param pnRadioButton Optional receives the selected radio button ID.
 * \param pfVerificationFlagChecked Optional receives the verification checkbox state.
 * \return HRESULT Successful or errant status.
 */
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

/**
 * Installs Microsoft Detours API hooks for theming, transparency, and window creation.
 */
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

    if (PhEnableThemeSupport || PhEnableThemeAcrylicSupport)
    {
        if (!NT_SUCCESS(status = DetourAttach((PVOID)&DefaultDrawThemeBackground, (PVOID)PhDrawThemeBackgroundHook)))
            goto CleanupExit;
        if (!NT_SUCCESS(status = DetourAttach((PVOID)&DefaultDrawThemeBackgroundEx, (PVOID)PhDrawThemeBackgroundExHook)))
            goto CleanupExit;
        if (!PhDefaultEnableThemeAnimation)
        {
            if (!NT_SUCCESS(status = DetourAttach((PVOID)&DefaultSystemParametersInfo, (PVOID)PhSystemParametersInfoHook)))
                goto CleanupExit;
        }
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

/**
 * Checks if system-wide transparency effects are enabled in Windows Personalization settings.
 *
 * \return BOOLEAN TRUE if transparency effects are enabled, FALSE otherwise.
 */
BOOLEAN PhIsThemeTransparencyEnabled(
    VOID
    )
{
    static CONST PH_STRINGREF themesKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize");
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

/**
 * Initializes superclassed control window classes and registers detour hooks.
 */
VOID PhInitializeSuperclassControls(
    VOID
    )
{
    PhDefaultEnableStreamerMode = !!PhGetIntegerSetting(SETTING_ENABLE_STREAMER_MODE);

    if (PhEnableThemeAcrylicSupport && !PhEnableThemeSupport)
        PhEnableThemeAcrylicSupport = FALSE;
    if (PhEnableThemeAcrylicSupport)
        PhEnableThemeAcrylicSupport = PhIsThemeTransparencyEnabled();

    PhRegisterMenuSuperClass();

    if (PhEnableThemeSupport || PhDefaultEnableStreamerMode)
    {
        if (WindowsVersion >= WINDOWS_11)
        {
            PhDefaultEnableThemeAcrylicWindowSupport = !!PhGetIntegerSetting(SETTING_ENABLE_THEME_ACRYLIC_WINDOW_SUPPORT);
        }

        PhDefaultEnableThemeAnimation = !!PhGetIntegerSetting(SETTING_ENABLE_THEME_ANIMATION);

        PhRegisterDialogSuperClass();
        PhRegisterRebarSuperClass();
        PhRegisterComboBoxSuperClass();
        PhRegisterStaticSuperClass();
        PhRegisterStatusBarSuperClass();
        PhRegisterEditSuperClass();
        PhRegisterHeaderSuperClass();

        PhRegisterDetoursHooks();
    }
}
