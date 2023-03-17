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

#include <vsstyle.h>
#include <uxtheme.h>

#include "settings.h"
#include "../thirdparty/detours/detours.h"

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
    case WM_NCCREATE:
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
            return (INT_PTR)GetStockBrush(DC_BRUSH);
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
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            HICON iconHandle;

            if (!PhGetWindowContext(WindowHandle, SCHAR_MAX))
                break;

            if (iconHandle = (HICON)(UINT_PTR)CallWindowProc(PhDefaultStaticWindowProcedure, WindowHandle, STM_GETICON, 0, 0)) // Static_GetIcon(WindowHandle, 0)
            {
                if (hdc = BeginPaint(WindowHandle, &ps))
                {
                    FillRect(hdc, &ps.rcPaint, PhThemeWindowBackgroundBrush);

                    DrawIconEx(
                        hdc,
                        ps.rcPaint.left,
                        ps.rcPaint.top,
                        iconHandle,
                        ps.rcPaint.right - ps.rcPaint.left,
                        ps.rcPaint.bottom - ps.rcPaint.top,
                        0,
                        NULL,
                        DI_NORMAL
                        );

                    EndPaint(WindowHandle, &ps);
                }
            }
        }
        return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);
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
        FillRect(bufferDc, &blockRect, GetStockBrush(DC_BRUSH));
        //FrameRect(bufferDc, &blockRect, GetSysColorBrush(COLOR_HIGHLIGHT));
    }
    else
    {
        SetTextColor(bufferDc, PhThemeWindowTextColor);
        FillRect(bufferDc, &blockRect, PhThemeWindowBackgroundBrush);
        //FrameRect(bufferDc, &blockRect, GetSysColorBrush(COLOR_HIGHLIGHT));
    }

    blockRect.left += 2;
    DrawText(
        bufferDc,
        text,
        (UINT)PhCountStringZ(text),
        &blockRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX
        );
    blockRect.left -= 2;
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
                    FrameRect(hdc, &windowRect, GetStockBrush(DC_BRUSH));
                }
                else
                {
                    SetDCBrushColor(hdc, PhThemeWindowBackground2Color);
                    FrameRect(hdc, &windowRect, GetStockBrush(DC_BRUSH));
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
            FillRect(bufferDc, &headerRect, GetStockBrush(DC_BRUSH));
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
            //FrameRect(hdc, &headerRect, GetStockBrush(DC_BRUSH));

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
                    ) == S_OK)
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
                    ) == S_OK)
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

    UnregisterClass(WC_COMBOBOX, NULL); // Must be unregistered first? (dmex)

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

    UnregisterClass(WC_EDIT, NULL); // Must be unregistered first? (dmex)

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

    UnregisterClass(WC_HEADER, NULL); // Must be unregistered first? (dmex)

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

//static HRESULT (WINAPI* PhDefaultDrawThemeBackgroundEx)(
//    _In_ HTHEME Theme,
//    _In_ HDC hdc,
//    _In_ INT PartId,
//    _In_ INT StateId,
//    _In_ LPCRECT Rect,
//    _In_ PVOID Options
//    ) = NULL;

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

//static COLORREF (WINAPI* PhDefaultSetTextColor)(
//    _In_ HDC hdc,
//    _In_ COLORREF color
//    ) = NULL;

HRESULT PhDrawThemeBackgroundHook(
    _In_ HTHEME Theme,
    _In_ HDC Hdc,
    _In_ INT PartId,
    _In_ INT StateId,
    _In_ LPCRECT Rect,
    _In_ LPCRECT ClipRect
    )
{
    if (WindowsVersion >= WINDOWS_11)
    {
        WCHAR className[MAX_PATH];

        if (PhGetThemeClass(Theme, className, RTL_NUMBER_OF(className)))
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

    return PhDefaultDrawThemeBackground(Theme, Hdc, PartId, StateId, Rect, ClipRect);
}

//HRESULT WINAPI PhDrawThemeBackgroundExHook(
//    _In_ HTHEME Theme,
//    _In_ HDC hdc,
//    _In_ INT PartId,
//    _In_ INT StateId,
//    _In_ LPCRECT Rect,
//    _In_ PVOID Options // DTBGOPTS
//    )
//{
//    //HWND windowHandle = WindowFromDC(hdc);
//    WCHAR className[MAX_PATH];
//
//    if (PhGetThemeClass(Theme, className, RTL_NUMBER_OF(className)))
//    {
//        if (!PhEqualStringZ(className, VSCLASS_TASKDIALOG, TRUE))
//            goto CleanupExit;
//    }
//
//    if (PartId == TDLG_PRIMARYPANEL && StateId == 0)
//    {
//        SetDCBrushColor(hdc, RGB(65, 65, 65));
//        FillRect(hdc, Rect, GetStockBrush(DC_BRUSH));
//        return S_OK;
//    }
//
//    if (PartId == TDLG_SECONDARYPANEL && StateId == 0)
//    {
//        SetDCBrushColor(hdc, RGB(45, 45, 45));
//        FillRect(hdc, Rect, GetStockBrush(DC_BRUSH));
//        return S_OK;
//    }
//
//CleanupExit:
//    return PhDefaultDrawThemeBackgroundEx(Theme, hdc, PartId, StateId, Rect, Options);
//}

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
    else
    {
        //HWND parentHandle;
        //
        //if (parentHandle = GetAncestor(windowHandle, GA_ROOT))
        //{
        //    if (!IS_INTRESOURCE(ClassName) && PhEqualStringZ((PWSTR)ClassName, L"DirectUIHWND", TRUE))
        //    {
        //        PhInitializeTaskDialogTheme(windowHandle, PhDefaultThemeSupport); // don't initialize parentHandle themes
        //        PhInitializeThemeWindowFrame(parentHandle);
        //
        //        if (PhDefaultEnableStreamerMode)
        //        {
        //            if (SetWindowDisplayAffinity_Import())
        //                SetWindowDisplayAffinity_Import()(parentHandle, WDA_EXCLUDEFROMCAPTURE);
        //        }
        //    }
        //}
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
//
//COLORREF WINAPI PhSetTextColorHook(
//    _In_ HDC hdc,
//    _In_ COLORREF color
//    )
//{
//    //HWND windowHandle = WindowFromDC(hdc);
//
//    if (!(PhTaskDialogThemeHookIndex && TlsGetValue(PhTaskDialogThemeHookIndex)))
//        goto CleanupExit;
//
//    if (RGB_FROM_COLOREF(RGB_FROM_COLOREF(color)) == RGB(0, 51, 153)) // TMT_TEXTCOLOR (TaskDialog InstructionPane)
//    {
//        color = RGB(0, 102, 255);
//    }
//    else if (RGB_FROM_COLOREF(RGB_FROM_COLOREF(color)) == RGB(0, 102, 204)) // TaskDialog Link
//    {
//        color = RGB(0, 128, 0); // RGB(128, 255, 128);
//    }
//    else
//    {
//        color = RGB(255, 255, 255); // GetBkColor(hdc);
//    }
//
//CleanupExit:
//    return PhDefaultSetTextColor(hdc, color);
//}

VOID PhRegisterDetoursHooks(
    VOID
    )
{
    NTSTATUS status;
    PVOID baseAddress;

    //if (baseAddress = PhGetLoaderEntryDllBaseZ(L"gdi32.dll"))
    //{
    //    PhDefaultSetTextColor = PhGetDllBaseProcedureAddress(baseAddress, "SetTextColor", 0);
    //}

    if (baseAddress = PhGetLoaderEntryDllBaseZ(L"user32.dll"))
    {
        PhDefaultCreateWindowEx = PhGetDllBaseProcedureAddress(baseAddress, "CreateWindowExW", 0);
        PhDefaultSystemParametersInfo = PhGetDllBaseProcedureAddress(baseAddress, "SystemParametersInfoW", 0);
    }

    if (baseAddress = PhGetLoaderEntryDllBaseZ(L"uxtheme.dll"))
    {
        PhDefaultDrawThemeBackground = PhGetDllBaseProcedureAddress(baseAddress, "DrawThemeBackground", 0);
        //PhDefaultDrawThemeBackgroundEx = PhGetDllBaseProcedureAddress(baseAddress, "DrawThemeBackgroundEx", 0);
    }

    if (!NT_SUCCESS(status = DetourTransactionBegin()))
        goto CleanupExit;

    if (PhEnableThemeSupport || PhEnableThemeAcrylicSupport)
    {
        if (!NT_SUCCESS(status = DetourAttach((PVOID)&PhDefaultDrawThemeBackground, (PVOID)PhDrawThemeBackgroundHook)))
            goto CleanupExit;
        //if (!NT_SUCCESS(status = DetourAttach((PVOID)&PhDefaultDrawThemeBackgroundEx, (PVOID)PhDrawThemeBackgroundExHook)))
        //    goto CleanupExit;
        if (!NT_SUCCESS(status = DetourAttach((PVOID)&PhDefaultSystemParametersInfo, (PVOID)PhSystemParametersInfoHook)))
            goto CleanupExit;
    }

    if (!NT_SUCCESS(status = DetourAttach((PVOID)&PhDefaultCreateWindowEx, (PVOID)PhCreateWindowExHook)))
        goto CleanupExit;
    //if (!NT_SUCCESS(status = DetourAttach((PVOID)&PhDefaultSetTextColor, (PVOID)PhSetTextColorHook)))
    //    goto CleanupExit;
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
