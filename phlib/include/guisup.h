/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2023
 *
 */

#ifndef _PH_PHGUI_H
#define _PH_PHGUI_H

#include <commctrl.h>
#include <guisupview.h>

EXTERN_C_START

// guisup

#define RFF_NOBROWSE 0x0001
#define RFF_NODEFAULT 0x0002
#define RFF_CALCDIRECTORY 0x0004
#define RFF_NOLABEL 0x0008
#define RFF_NOSEPARATEMEM 0x0020
#define RFF_OPTRUNAS 0x0040

#define RFN_VALIDATE (-510)
#define RFN_LIMITEDRUNAS (-511)

typedef struct _NMRUNFILEDLGW
{
    NMHDR hdr;
    PWSTR lpszFile;
    PWSTR lpszDirectory;
    UINT ShowCmd;
} NMRUNFILEDLGW, *LPNMRUNFILEDLGW, *PNMRUNFILEDLGW;

typedef NMRUNFILEDLGW NMRUNFILEDLG;
typedef PNMRUNFILEDLGW PNMRUNFILEDLG;
typedef LPNMRUNFILEDLGW LPNMRUNFILEDLG;

#define RF_OK 0x0000
#define RF_CANCEL 0x0001
#define RF_RETRY 0x0002

typedef HANDLE HTHEME;

#define DCX_USESTYLE 0x00010000
#define DCX_NODELETERGN 0x00040000

#define HRGN_FULL ((HRGN)1) // passed by WM_NCPAINT even though it's completely undocumented (wj32)

extern LONG PhFontQuality;
extern LONG PhSystemDpi;
extern PH_INTEGER_PAIR PhSmallIconSize;
extern PH_INTEGER_PAIR PhLargeIconSize;

PHLIBAPI
VOID
NTAPI
PhGuiSupportInitialization(
    VOID
    );

PHLIBAPI
VOID
NTAPI
PhGuiSupportUpdateSystemMetrics(
    _In_opt_ HWND WindowHandle,
    _In_opt_ LONG WindowDpi
    );

PHLIBAPI
LONG
NTAPI
PhGetFontQualitySetting(
    _In_ LONG FontQuality
    );

PHLIBAPI
HFONT
NTAPI
PhInitializeFont(
    _In_ LONG WindowDpi
    );

PHLIBAPI
HFONT
NTAPI
PhInitializeMonospaceFont(
    _In_ LONG WindowDpi
    );

PHLIBAPI
HDC
NTAPI
PhGetDC(
    _In_opt_ HWND WindowHandle
    );

PHLIBAPI
VOID
NTAPI
PhReleaseDC(
    _In_opt_ HWND WindowHandle,
    _In_ _Frees_ptr_ HDC Hdc
    );

PHLIBAPI
HGDIOBJ
NTAPI
PhGetStockObject(
    _In_ LONG Index
    );

#define PhGetStockBrush(i) ((HBRUSH)PhGetStockObject(i))
#define PhGetStockPen(i) ((HPEN)PhGetStockObject(i))

PHLIBAPI
HTHEME
NTAPI
PhOpenThemeData(
    _In_opt_ HWND WindowHandle,
    _In_ PCWSTR ClassList,
    _In_ LONG WindowDpi
    );

PHLIBAPI
VOID
NTAPI
PhCloseThemeData(
    _In_ HTHEME ThemeHandle
    );

PHLIBAPI
VOID
NTAPI
PhSetControlTheme(
    _In_ HWND Handle,
    _In_opt_ PCWSTR Theme
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIsThemeActive(
    VOID
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIsThemePartDefined(
    _In_ HTHEME ThemeHandle,
    _In_ LONG PartId,
    _In_ LONG StateId
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetThemeClass(
    _In_ HTHEME ThemeHandle,
    _Out_writes_z_(ClassLength) PWSTR Class,
    _In_ ULONG ClassLength
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetThemeColor(
    _In_ HTHEME ThemeHandle,
    _In_ LONG PartId,
    _In_ LONG StateId,
    _In_ LONG PropId,
    _Out_ COLORREF* Color
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetThemeInt(
    _In_ HTHEME ThemeHandle,
    _In_ LONG PartId,
    _In_ LONG StateId,
    _In_ LONG PropId,
    _Out_ PLONG Value
    );

typedef enum _THEMEPARTSIZE
{
    THEMEPARTSIZE_MIN, // minimum size
    THEMEPARTSIZE_TRUE, // size without stretching
    THEMEPARTSIZE_DRAW // size that theme mgr will use to draw part
} THEMEPARTSIZE;

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetThemePartSize(
    _In_ HTHEME ThemeHandle,
    _In_opt_ HDC hdc,
    _In_ LONG PartId,
    _In_ LONG StateId,
    _In_opt_ LPCRECT Rect,
    _In_ THEMEPARTSIZE Flags,
    _Out_ PSIZE Size
    );

typedef struct _THEMEMARGINS
{
    LONG cxLeftWidth;      // width of left border that retains its size
    LONG cxRightWidth;     // width of right border that retains its size
    LONG cyTopHeight;      // height of top border that retains its size
    LONG cyBottomHeight;   // height of bottom border that retains its size
} THEMEMARGINS, *PTHEMEMARGINS;

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetThemeMargins(
    _In_ HTHEME ThemeHandle,
    _In_opt_ HDC hdc,
    _In_ LONG PartId,
    _In_ LONG StateId,
    _In_ LONG PropId,
    _In_opt_ LPCRECT Rect,
    _Out_ PTHEMEMARGINS Margins
    );

PHLIBAPI
BOOLEAN
NTAPI
PhDrawThemeBackground(
    _In_ HTHEME ThemeHandle,
    _In_ HDC hdc,
    _In_ LONG PartId,
    _In_ LONG StateId,
    _In_ LPCRECT Rect,
    _In_opt_ LPCRECT ClipRect
    );

PHLIBAPI
BOOLEAN
NTAPI
PhDrawThemeText(
    _In_ HTHEME ThemeHandle,
    _In_ HDC hdc,
    _In_ LONG PartId,
    _In_ LONG StateId,
    _In_reads_(cchText) PCWSTR Text,
    _In_ LONG cchText,
    _In_ ULONG TextFlags,
    _In_ LPCRECT Rect
    );

PHLIBAPI
BOOLEAN
NTAPI
PhDrawThemeTextEx(
    _In_ HTHEME ThemeHandle,
    _In_ HDC hdc,
    _In_ LONG PartId,
    _In_ LONG StateId,
    _In_reads_(cchText) PCWSTR Text,
    _In_ LONG cchText,
    _In_ ULONG TextFlags,
    _Inout_ LPRECT Rect,
    _In_opt_ const PVOID Options // DTTOPTS*
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIsThemeBackgroundPartiallyTransparent(
    _In_ HTHEME ThemeHandle,
    _In_ LONG PartId,
    _In_ LONG StateId
    );

PHLIBAPI
BOOLEAN
NTAPI
PhDrawThemeParentBackground(
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_opt_ const PRECT Rect
    );

PHLIBAPI
BOOLEAN
NTAPI
PhAllowDarkModeForWindow(
    _In_ HWND WindowHandle,
    _In_ BOOL Enabled
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIsDarkModeAllowedForWindow(
    _In_ HWND WindowHandle
    );

FORCEINLINE
BOOLEAN
NTAPI
PhRectEmpty(
    _In_ PRECT Rect
    )
{
#if defined(PHNT_NATIVE_RECT)
    return !!IsRectEmpty(Rect);
#else
    if (Rect)
    {
        if (Rect->left >= Rect->right || Rect->top >= Rect->bottom)
            return TRUE;
    }

    return FALSE;
#endif
}

FORCEINLINE
BOOLEAN
NTAPI
PhInflateRect(
    _In_ PRECT Rect,
    _In_ LONG dx,
    _In_ LONG dy
    )
{
#if defined(PHNT_NATIVE_RECT)
    return !!InflateRect(Rect, dx, dy);
#else
    Rect->left -= dx;
    Rect->top -= dy;
    Rect->right += dx;
    Rect->bottom += dy;
    return TRUE;
#endif
}

FORCEINLINE
BOOLEAN
NTAPI
PhOffsetRect(
    _In_ PRECT Rect,
    _In_ LONG dx,
    _In_ LONG dy
    )
{
#if defined(PHNT_NATIVE_RECT)
    return !!OffsetRect(Rect, dx, dy);
#else
    Rect->left += dx;
    Rect->top += dy;
    Rect->right += dx;
    Rect->bottom += dy;
    return TRUE;
#endif
}

FORCEINLINE
BOOLEAN
NTAPI
PhPtInRect(
    _In_ PRECT Rect,
    _In_ PPOINT Point
    )
{
#if defined(PHNT_NATIVE_RECT)
    return !!PtInRect(Rect, Point);
#else
    return Point->x >= Rect->left && Point->x < Rect->right &&
        Point->y >= Rect->top && Point->y < Rect->bottom;
#endif
}

_Success_(return)
FORCEINLINE
BOOLEAN
NTAPI
PhGetWindowRect(
    _In_ HWND WindowHandle,
    _Out_ PRECT WindowRect
    )
{
    // Note: GetWindowRect can return success with either invalid (0,0) or empty rects (40,40) and in some cases
    // this results in unwanted clipping, performance issues with the CreateCompatibleBitmap double buffering and
    // issues with MonitorFromRect layout and DPI queries, so ignore the return status and check the rect (dmex)

    if (!GetWindowRect(WindowHandle, WindowRect))
        return FALSE;

    if (PhRectEmpty(WindowRect))
        return FALSE;

    return TRUE;
}

_Success_(return)
FORCEINLINE
BOOLEAN
NTAPI
PhGetClientRect(
    _In_ HWND WindowHandle,
    _Out_ PRECT ClientRect
    )
{
    if (!GetClientRect(WindowHandle, ClientRect))
        return FALSE;

    if (!(ClientRect->right && ClientRect->bottom))
        return FALSE;

    return TRUE;
}

_Success_(return)
FORCEINLINE
BOOLEAN
NTAPI
PhGetCursorPos(
    _Out_ PPOINT Point
    )
{
    if (GetCursorPos(Point))
        return TRUE;

    return FALSE;
}

_Success_(return)
FORCEINLINE
BOOLEAN
NTAPI
PhGetMessagePos(
    _Out_ PPOINT MessagePoint
    )
{
    ULONG position;
    POINT point;

    position = GetMessagePos();
    point.x = GET_X_LPARAM(position);
    point.y = GET_Y_LPARAM(position);
    memcpy(MessagePoint, &point, sizeof(POINT));
    return TRUE;
}


_Success_(return)
FORCEINLINE
BOOLEAN
NTAPI
PhGetClientPos(
    _In_ HWND WindowHandle,
    _Out_ PPOINT ClientPoint
    )
{
    ULONG position;
    POINT point;

    position = GetMessagePos();
    point.x = GET_X_LPARAM(position);
    point.y = GET_Y_LPARAM(position);

    if (ScreenToClient(WindowHandle, &point))
    {
        memcpy(ClientPoint, &point, sizeof(POINT));
        return TRUE;
    }

    return FALSE;
}

_Success_(return)
FORCEINLINE
BOOLEAN
NTAPI
PhGetScreenPos(
    _In_ HWND WindowHandle,
    _Out_ PPOINT ClientPoint
    )
{
    ULONG position;
    POINT point;

    position = GetMessagePos();
    point.x = GET_X_LPARAM(position);
    point.y = GET_Y_LPARAM(position);

    if (ClientToScreen(WindowHandle, &point))
    {
        memcpy(ClientPoint, &point, sizeof(POINT));
        return TRUE;
    }

    return FALSE;
}

FORCEINLINE
BOOLEAN
NTAPI
PhClientToScreen(
    _In_ HWND WindowHandle,
    _Inout_ PPOINT Point
    )
{
    if (ClientToScreen(WindowHandle, Point))
        return TRUE;
    return FALSE;
}

FORCEINLINE
BOOLEAN
NTAPI
PhScreenToClient(
    _In_ HWND WindowHandle,
    _Inout_ PPOINT Point
    )
{
    if (ScreenToClient(WindowHandle, Point))
        return TRUE;
    return FALSE;
}

FORCEINLINE
BOOLEAN
NTAPI
PhClientToScreenRect(
    _In_ HWND WindowHandle,
    _Inout_ PRECT Rect
    )
{
    //if (!ClientToScreen(WindowHandle, (PPOINT)Rect))
    //    return FALSE;
    //if (!ClientToScreen(WindowHandle, ((PPOINT)Rect) + 1))
    //    return FALSE;

    if (MapWindowRect(WindowHandle, HWND_DESKTOP, Rect) == 0)
        return FALSE;

    return TRUE;
}

FORCEINLINE
BOOLEAN
NTAPI
PhScreenToClientRect(
    _In_ HWND WindowHandle,
    _Inout_ PRECT Rect
    )
{
    if (MapWindowRect(HWND_DESKTOP, WindowHandle, Rect) == 0)
        return FALSE;

    return TRUE;

    //if (!ScreenToClient(WindowHandle, (PPOINT)Rect))
    //    return FALSE;
    //if (!ScreenToClient(WindowHandle, ((PPOINT)Rect) + 1))
    //    return FALSE;
    //return TRUE;
}

PHLIBAPI
BOOLEAN
NTAPI
PhIsHungAppWindow(
    _In_ HWND WindowHandle
    );

PHLIBAPI
HWND
NTAPI
PhGetShellWindow(
    VOID
    );

/**
 * Converts default logical units (based on 96 DPI) to physical units appropriate for the current current monitor's display DPI.
 * \param Value The value to scale.
 * \param Scale The target DPI scale.
 * \return The scaled value.
 */
FORCEINLINE
LONG
NTAPI
PhScaleToDisplay(
    _In_ LONG Value,
    _In_ LONG Scale
    )
{
    return PhMultiplyDivideSigned(Value, Scale, USER_DEFAULT_SCREEN_DPI);
}

/**
 * Converts a value from physical units (current DPI) to default logical units (based on 96 DPI).
 *
 * \param Value The value to convert from physical units.
 * \param Scale The current DPI scale.
 * \return The value converted to default logical units (96 DPI).
 */
FORCEINLINE
LONG
NTAPI
PhScaleToDefault(
    _In_ LONG Value,
    _In_ LONG Scale
    )
{
    return PhMultiplyDivideSigned(Value, USER_DEFAULT_SCREEN_DPI, Scale);
}

FORCEINLINE
LONG
NTAPI
PhGetDpi(
    _In_ LONG Value,
    _In_ LONG Scale
    )
{
    return PhMultiplyDivideSigned(Value, Scale, USER_DEFAULT_SCREEN_DPI);
}

PHLIBAPI
LONG
NTAPI
PhGetMonitorDpi(
    _In_opt_ HWND WindowHandle,
    _In_opt_ PRECT WindowRect
    );

FORCEINLINE
LONG
PhGetMonitorDpiFromRect(
    _In_ PPH_RECTANGLE Rectangle
    )
{
    RECT rect = { 0 };

    PhRectangleToRect(&rect, Rectangle);

    return PhGetMonitorDpi(NULL, &rect);
}

PHLIBAPI
LONG
NTAPI
PhGetSystemDpi(
    VOID
    );

PHLIBAPI
LONG
NTAPI
PhGetTaskbarDpi(
    VOID
    );

PHLIBAPI
LONG
NTAPI
PhGetWindowDpi(
    _In_ HWND WindowHandle
    );

PHLIBAPI
LONG
NTAPI
PhGetDpiValue(
    _In_opt_ HWND WindowHandle,
    _In_opt_ PRECT WindowRect
    );

FORCEINLINE
VOID
PhGetSizeDpiValue(
    _Inout_ PRECT Rect,
    _In_ LONG Dpi,
    _In_ BOOLEAN ScaleToDisplay
    )
{
    PH_RECTANGLE rect;

    if (Dpi == USER_DEFAULT_SCREEN_DPI)
        return;

    PhRectToRectangle(&rect, Rect);

    if (ScaleToDisplay)
    {
        if (rect.Left)
            rect.Left = PhScaleToDisplay(rect.Left, Dpi);
        if (rect.Top)
            rect.Top = PhScaleToDisplay(rect.Top, Dpi);
        if (rect.Width)
            rect.Width = PhScaleToDisplay(rect.Width, Dpi);
        if (rect.Height)
            rect.Height = PhScaleToDisplay(rect.Height, Dpi);
    }
    else
    {
        if (rect.Left)
            rect.Left = PhScaleToDefault(rect.Left, Dpi);
        if (rect.Top)
            rect.Top = PhScaleToDefault(rect.Top, Dpi);
        if (rect.Width)
            rect.Width = PhScaleToDefault(rect.Width, Dpi);
        if (rect.Height)
            rect.Height = PhScaleToDefault(rect.Height, Dpi);
    }

    PhRectangleToRect(Rect, &rect);
}

PHLIBAPI
LONG
NTAPI
PhGetSystemMetrics(
    _In_ LONG Index,
    _In_opt_ LONG DpiValue
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetSystemSafeBootMode(
    VOID
    );

PHLIBAPI
BOOL
NTAPI
PhGetSystemParametersInfo(
    _In_ LONG Action,
    _In_ ULONG Param1,
    _Pre_maybenull_ _Post_valid_ PVOID Param2,
    _In_opt_ LONG DpiValue
    );

FORCEINLINE
ULONG
PhGetClassStyle(
    _In_ HWND WindowHandle
    )
{
    return (ULONG)GetClassLongPtr(WindowHandle, GCL_STYLE);
}

FORCEINLINE
VOID
PhSetClassStyle(
    _In_ HWND Handle,
    _In_ ULONG Mask,
    _In_ ULONG Value
    )
{
    ULONG style;

    style = (ULONG)GetClassLongPtr(Handle, GCL_STYLE);
    style = (style & ~Mask) | (Value & Mask);
    SetClassLongPtr(Handle, GCL_STYLE, style);
}

FORCEINLINE
ULONG
PhGetWindowStyle(
    _In_ HWND WindowHandle
    )
{
    return (ULONG)GetWindowLongPtr(WindowHandle, GWL_STYLE);
}

FORCEINLINE
ULONG
PhGetWindowStyleEx(
    _In_ HWND WindowHandle
    )
{
    return (ULONG)GetWindowLongPtr(WindowHandle, GWL_EXSTYLE);
}

FORCEINLINE VOID PhSetWindowStyle(
    _In_ HWND Handle,
    _In_ ULONG Mask,
    _In_ ULONG Value
    )
{
    ULONG style;

    style = (ULONG)GetWindowLongPtr(Handle, GWL_STYLE);
    style = (style & ~Mask) | (Value & Mask);
    SetWindowLongPtr(Handle, GWL_STYLE, style);
}

FORCEINLINE VOID PhSetWindowExStyle(
    _In_ HWND Handle,
    _In_ ULONG Mask,
    _In_ ULONG Value
    )
{
    ULONG style;

    style = (ULONG)GetWindowLongPtr(Handle, GWL_EXSTYLE);
    style = (style & ~Mask) | (Value & Mask);
    SetWindowLongPtr(Handle, GWL_EXSTYLE, style);
}

FORCEINLINE WNDPROC PhGetWindowProcedure(
    _In_ HWND WindowHandle
    )
{
    return (WNDPROC)GetWindowLongPtr(WindowHandle, GWLP_WNDPROC);
}

FORCEINLINE WNDPROC PhSetWindowProcedure(
    _In_ HWND WindowHandle,
    _In_ WNDPROC SubclassProcedure
    )
{
    return (WNDPROC)SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)SubclassProcedure);
}

#define PH_WINDOW_TIMER_DEFAULT 0xF

FORCEINLINE ULONG_PTR PhSetTimer(
    _In_ HWND WindowHandle,
    _In_ ULONG_PTR TimerID,
    _In_ ULONG Elapse,
    _In_opt_ TIMERPROC TimerProcedure
    )
{
    assert(WindowHandle);
    return SetTimer(WindowHandle, TimerID, Elapse, TimerProcedure);
}

FORCEINLINE BOOL PhKillTimer(
    _In_ HWND WindowHandle,
    _In_ ULONG_PTR TimerID
    )
{
    assert(WindowHandle);
    return KillTimer(WindowHandle, TimerID);
}

FORCEINLINE VOID PhBringWindowToTop(
    _In_ HWND WindowHandle
    )
{
    // Move the window to the top of the Z-order. (dmex)
    // This is a workaround for a Windows or third party issue breaking
    // SetForegroundWindow and displaying the main window behind other programs.
    // https://github.com/winsiderss/systeminformer/issues/639

    SetWindowPos(
        WindowHandle,
        HWND_TOP,
        0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE
        );
}

#define IDC_DIVIDER MAKEINTRESOURCE(106) // comctl32.dll

FORCEINLINE
HCURSOR
NTAPI
PhLoadCursor(
    _In_opt_ PVOID BaseAddress,
    _In_ PCWSTR CursorName
    )
{
    return (HCURSOR)LoadImage((HINSTANCE)BaseAddress, CursorName, IMAGE_CURSOR, 0, 0, LR_SHARED);
    //return LoadCursor((HINSTANCE)BaseAddress, CursorName);
}

FORCEINLINE
HCURSOR
NTAPI
PhGetCursor(
    VOID
    )
{
    return GetCursor();
}

FORCEINLINE
HCURSOR
NTAPI
PhSetCursor(
    _In_opt_ HCURSOR CursorHandle
    )
{
    return SetCursor(CursorHandle);
}

FORCEINLINE
BOOLEAN
NTAPI
PhGetKeyState(
    _In_ LONG VirtualKey
    )
{
    return GetKeyState(VirtualKey) < 0;
}

#ifndef WM_REFLECT
#define WM_REFLECT 0x2000
#endif

FORCEINLINE LRESULT PhReflectMessage(
    _In_ HWND Handle,
    _In_ ULONG Message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    if (Message == WM_NOTIFY)
    {
        LPNMHDR header = (LPNMHDR)lParam;

        if (header->hwndFrom == Handle)
            return SendMessage(Handle, WM_REFLECT + Message, wParam, lParam);
    }

    return 0;
}

#define REFLECT_MESSAGE(hwnd, msg, wParam, lParam) \
    { \
        LRESULT result_ = PhReflectMessage(hwnd, msg, wParam, lParam); \
        \
        if (result_) \
            return result_; \
    }

#define REFLECT_MESSAGE_DLG(WindowHandle, hwnd, msg, wParam, lParam) \
    { \
        LRESULT result_ = PhReflectMessage(hwnd, msg, wParam, lParam); \
        \
        if (result_) \
        { \
            SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, result_); \
            return TRUE; \
        } \
    }

FORCEINLINE VOID PhSetListViewStyle(
    _In_ HWND Handle,
    _In_ BOOLEAN AllowDragDrop,
    _In_ BOOLEAN ShowLabelTips
    )
{
    ULONG style;

    style = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP;

    if (AllowDragDrop)
        style |= LVS_EX_HEADERDRAGDROP;
    if (ShowLabelTips)
        style |= LVS_EX_LABELTIP;

    ListView_SetExtendedListViewStyleEx(
        Handle,
        style,
        INT_ERROR
        );
}

PHLIBAPI
LONG
NTAPI
PhAddListViewColumnDpi(
    _In_ HWND ListViewHandle,
    _In_ LONG ListViewDpi,
    _In_ LONG Index,
    _In_ LONG DisplayIndex,
    _In_ LONG SubItemIndex,
    _In_ LONG Format,
    _In_ LONG Width,
    _In_ PCWSTR Text
    );

PHLIBAPI
LONG
NTAPI
PhAddListViewColumn(
    _In_ HWND ListViewHandle,
    _In_ LONG Index,
    _In_ LONG DisplayIndex,
    _In_ LONG SubItemIndex,
    _In_ LONG Format,
    _In_ LONG Width,
    _In_ PCWSTR Text
    );

PHLIBAPI
LONG
NTAPI
PhAddListViewItem(
    _In_ HWND ListViewHandle,
    _In_ LONG Index,
    _In_ PCWSTR Text,
    _In_opt_ PVOID Param
    );

PHLIBAPI
LONG
NTAPI
PhFindListViewItemByFlags(
    _In_ HWND ListViewHandle,
    _In_ LONG StartIndex,
    _In_ ULONG Flags
    );

PHLIBAPI
LONG
NTAPI
PhFindListViewItemByParam(
    _In_ HWND ListViewHandle,
    _In_ LONG StartIndex,
    _In_opt_ PVOID Param
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetListViewItemImageIndex(
    _In_ HWND ListViewHandle,
    _In_ LONG Index,
    _Out_ PLONG ImageIndex
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetListViewItemParam(
    _In_ HWND ListViewHandle,
    _In_ LONG Index,
    _Outptr_ PVOID *Param
    );

PHLIBAPI
BOOLEAN
NTAPI
PhSetListViewItemParam(
    _In_ HWND ListViewHandle,
    _In_ LONG Index,
    _In_ PVOID Param
    );

PHLIBAPI
VOID
NTAPI
PhRemoveListViewItem(
    _In_ HWND ListViewHandle,
    _In_ LONG Index
    );

PHLIBAPI
VOID
NTAPI
PhSetListViewItemImageIndex(
    _In_ HWND ListViewHandle,
    _In_ LONG Index,
    _In_ LONG ImageIndex
    );

PHLIBAPI
VOID
NTAPI
PhSetListViewSubItem(
    _In_ HWND ListViewHandle,
    _In_ LONG Index,
    _In_ LONG SubItemIndex,
    _In_ PCWSTR Text
    );

PHLIBAPI
VOID
NTAPI
PhRedrawListViewItems(
    _In_ HWND ListViewHandle
    );

PHLIBAPI
LONG
NTAPI
PhAddListViewGroup(
    _In_ HWND ListViewHandle,
    _In_ LONG GroupId,
    _In_ PCWSTR Text
    );

PHLIBAPI
LONG
NTAPI
PhAddListViewGroupItem(
    _In_ HWND ListViewHandle,
    _In_ LONG GroupId,
    _In_ LONG Index,
    _In_ PCWSTR Text,
    _In_opt_ PVOID Param
    );

PHLIBAPI
LONG
NTAPI
PhAddTabControlTab(
    _In_ HWND TabControlHandle,
    _In_ LONG Index,
    _In_ PCWSTR Text
    );

#define PhaGetDlgItemText(WindowHandle, id) \
    PH_AUTO_T(PH_STRING, PhGetWindowText(GetDlgItem(WindowHandle, id)))

PHLIBAPI
PPH_STRING
NTAPI
PhGetWindowText(
    _In_ HWND WindowHandle
    );

#define PhaGetWindowText(WindowHandle) \
    PH_AUTO_T(PH_STRING, PhGetWindowText(WindowHandle))

#define PH_GET_WINDOW_TEXT_INTERNAL 0x1
#define PH_GET_WINDOW_TEXT_LENGTH_ONLY 0x2

PHLIBAPI
ULONG
NTAPI
PhGetWindowTextEx(
    _In_ HWND WindowHandle,
    _In_ ULONG Flags,
    _Out_opt_ PPH_STRING *Text
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetWindowTextToBuffer(
    _In_ HWND WindowHandle,
    _In_ ULONG Flags,
    _Out_writes_bytes_(BufferLength) PWSTR Buffer,
    _In_opt_ ULONG BufferLength,
    _Out_opt_ PULONG ReturnLength
    );

FORCEINLINE
VOID
NTAPI
PhAddComboBoxStrings(
    _In_ HWND WindowHandle,
    _In_ PCWSTR* Strings,
    _In_ ULONG NumberOfStrings
    )
{
    for (ULONG i = 0; i < NumberOfStrings; i++)
        ComboBox_AddString(WindowHandle, Strings[i]);
}

FORCEINLINE
VOID
NTAPI
PhAddComboBoxStringRefs(
    _In_ HWND WindowHandle,
    _In_ CONST PCPH_STRINGREF* Strings,
    _In_ ULONG NumberOfStrings
    )
{
    for (ULONG i = 0; i < NumberOfStrings; i++)
        ComboBox_AddString(WindowHandle, Strings[i]->Buffer);
}

PHLIBAPI
NTSTATUS
NTAPI
PhGetClassName(
    _In_ HWND WindowHandle,
    _Out_writes_bytes_(BufferLength) PWSTR Buffer,
    _In_ ULONG BufferLength,
    _Out_opt_ PULONG ReturnLength
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetComboBoxString(
    _In_ HWND WindowHandle,
    _In_ LONG Index
    );

PHLIBAPI
LONG
NTAPI
PhSelectComboBoxString(
    _In_ HWND WindowHandle,
    _In_ PCWSTR String,
    _In_ BOOLEAN Partial
    );

PHLIBAPI
VOID
NTAPI
PhDeleteComboBoxStrings(
    _In_ HWND ComboBoxHandle,
    _In_ BOOLEAN ResetContent
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetListBoxString(
    _In_ HWND WindowHandle,
    _In_ LONG Index
    );

PHLIBAPI
VOID
NTAPI
PhSetStateAllListViewItems(
    _In_ HWND WindowHandle,
    _In_ ULONG State,
    _In_ ULONG Mask
    );

PHLIBAPI
PVOID
NTAPI
PhGetSelectedListViewItemParam(
    _In_ HWND WindowHandle
    );

PHLIBAPI
VOID
NTAPI
PhGetSelectedListViewItemParams(
    _In_ HWND WindowHandle,
    _Out_ PVOID **Items,
    _Out_ PULONG NumberOfItems
    );

FORCEINLINE
IListView*
NTAPI
PhGetListViewInterface(
    _In_ HWND ListViewHandle
    )
{
    IListView* listviewInterface = NULL;

    SendMessage(
        ListViewHandle,
        LVM_QUERYINTERFACE,
        (WPARAM)&IID_IListView,
        (LPARAM)&listviewInterface
        );

    return listviewInterface;
}

#define PhDestroyListViewInterface(ListViewClass) \
    if (ListViewClass) IListView_Release((struct IListView*)(ListViewClass));

PHLIBAPI
VOID
NTAPI
PhSetImageListBitmap(
    _In_ HIMAGELIST ImageList,
    _In_ LONG Index,
    _In_ HINSTANCE InstanceHandle,
    _In_ PCWSTR BitmapName
    );

#define PH_LOAD_ICON_SHARED 0x1
#define PH_LOAD_ICON_SIZE_SMALL 0x2
#define PH_LOAD_ICON_SIZE_LARGE 0x4
#define PH_LOAD_ICON_STRICT 0x8

PHLIBAPI
HICON
NTAPI
PhLoadIcon(
    _In_opt_ PVOID ImageBaseAddress,
    _In_ PCWSTR Name,
    _In_ ULONG Flags,
    _In_opt_ LONG Width,
    _In_opt_ LONG Height,
    _In_opt_ LONG SystemDpi
    );

PHLIBAPI
VOID
NTAPI
PhGetStockApplicationIcon(
    _Out_opt_ HICON *SmallIcon,
    _Out_opt_ HICON *LargeIcon
    );

//PHLIBAPI
//HICON PhGetFileShellIcon(
//    _In_opt_ PWSTR FileName,
//    _In_opt_ PWSTR DefaultExtension,
//    _In_ BOOLEAN LargeIcon
//    );

PHLIBAPI
VOID
NTAPI
PhSetClipboardString(
    _In_ HWND WindowHandle,
    _In_ PCPH_STRINGREF String
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetClipboardString(
    _In_ HWND WindowHandle
    );

#include <pshpack1.h>
typedef struct _DLGTEMPLATEEX
{
    USHORT dlgVer;
    USHORT signature;
    ULONG helpID;
    ULONG exStyle;
    ULONG style;
    USHORT cDlgItems;
    SHORT x;
    SHORT y;
    SHORT cx;
    SHORT cy;
    //sz_Or_Ord menu;
    //sz_Or_Ord windowClass;
    //WCHAR title[titleLen];
    //USHORT pointsize;
    //USHORT weight;
    //BYTE italic;
    //BYTE charset;
    //WCHAR typeface[stringLen];
} DLGTEMPLATEEX, *PDLGTEMPLATEEX;
#include <poppack.h>

PHLIBAPI
HWND
NTAPI
PhCreateDialogFromTemplate(
    _In_ HWND Parent,
    _In_ ULONG Style,
    _In_ PVOID Instance,
    _In_ PCWSTR Template,
    _In_ DLGPROC DialogProc,
    _In_ PVOID Parameter
    );

PHLIBAPI
HWND
NTAPI
PhCreateDialog(
    _In_ PVOID Instance,
    _In_ PCWSTR Template,
    _In_opt_ HWND ParentWindow,
    _In_ DLGPROC DialogProc,
    _In_opt_ PVOID Parameter
    );

PHLIBAPI
HWND
NTAPI
PhCreateWindowEx(
    _In_ PCWSTR ClassName,
    _In_opt_ PCWSTR WindowName,
    _In_ ULONG Style,
    _In_ ULONG ExStyle,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_opt_ HWND ParentWindow,
    _In_opt_ HMENU MenuHandle,
    _In_opt_ PVOID InstanceHandle,
    _In_opt_ PVOID Parameter
    );

#define PhCreateWindow(ClassName, WindowName, Style, X, Y, Width, Height, ParentWindow, MenuHandle, InstanceHandle, Parameter) \
    PhCreateWindowEx(ClassName, WindowName, Style, 0, X, Y, Width, Height, ParentWindow, MenuHandle, InstanceHandle, Parameter)

//FORCEINLINE
//HWND
//PhCreateWindow(
//    _In_ PCWSTR ClassName,
//    _In_opt_ PCWSTR WindowName,
//    _In_ ULONG Style,
//    _In_ LONG X,
//    _In_ LONG Y,
//    _In_ LONG Width,
//    _In_ LONG Height,
//    _In_opt_ HWND ParentWindow,
//    _In_opt_ HMENU MenuHandle,
//    _In_opt_ PVOID InstanceHandle,
//    _In_opt_ PVOID Parameter
//    )
//{
//    return PhCreateWindowEx(
//        ClassName,
//        WindowName,
//        Style,
//        0,
//        X,
//        Y,
//        Width,
//        Height,
//        ParentWindow,
//        MenuHandle,
//        InstanceHandle,
//        Parameter
//        );
//}

PHLIBAPI
INT_PTR
NTAPI
PhDialogBox(
    _In_ PVOID Instance,
    _In_ PCWSTR Template,
    _In_opt_ HWND ParentWindow,
    _In_ DLGPROC DialogProc,
    _In_opt_ PVOID Parameter
    );

PHLIBAPI
HMENU
NTAPI
PhLoadMenu(
    _In_ PVOID DllBase,
    _In_ PCWSTR MenuName
    );

PHLIBAPI
BOOLEAN
NTAPI
PhModalPropertySheet(
    _Inout_ PROPSHEETHEADER *Header
    );

#define PH_ANCHOR_LEFT 0x1
#define PH_ANCHOR_TOP 0x2
#define PH_ANCHOR_RIGHT 0x4
#define PH_ANCHOR_BOTTOM 0x8
#define PH_ANCHOR_ALL 0xf

// This interface is horrible and should be rewritten, but it works for now.

#define PH_LAYOUT_FORCE_INVALIDATE 0x1000 // invalidate the control when it is resized
#define PH_LAYOUT_TAB_CONTROL 0x2000 // this is a dummy item, a hack for the tab control
#define PH_LAYOUT_IMMEDIATE_RESIZE 0x4000 // needed for the tab control hack

#define PH_LAYOUT_DUMMY_MASK (PH_LAYOUT_TAB_CONTROL) // items that don't have a window handle, or don't actually get their window resized

typedef struct _PH_LAYOUT_ITEM
{
    HWND Handle;
    struct _PH_LAYOUT_ITEM *ParentItem; // for rectangle calculation
    struct _PH_LAYOUT_ITEM *LayoutParentItem; // for actual resizing
    ULONG LayoutNumber;
    ULONG NumberOfChildren;
    HDWP DeferHandle;

    RECT Rect;
    RECT Margin;
    ULONG Anchor;
} PH_LAYOUT_ITEM, *PPH_LAYOUT_ITEM;

typedef struct _PH_LAYOUT_MANAGER
{
    PPH_LIST List;
    PH_LAYOUT_ITEM RootItem;

    ULONG LayoutNumber;

    LONG WindowDpi;
} PH_LAYOUT_MANAGER, *PPH_LAYOUT_MANAGER;

PHLIBAPI
BOOLEAN
NTAPI
PhInitializeLayoutManager(
    _Out_ PPH_LAYOUT_MANAGER Manager,
    _In_ HWND RootWindowHandle
    );

PHLIBAPI
VOID
NTAPI
PhDeleteLayoutManager(
    _Inout_ PPH_LAYOUT_MANAGER Manager
    );

PHLIBAPI
PPH_LAYOUT_ITEM
NTAPI
PhAddLayoutItem(
    _Inout_ PPH_LAYOUT_MANAGER Manager,
    _In_ HWND Handle,
    _In_opt_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor
    );

PHLIBAPI
PPH_LAYOUT_ITEM
NTAPI
PhAddLayoutItemEx(
    _Inout_ PPH_LAYOUT_MANAGER Manager,
    _In_ HWND Handle,
    _In_opt_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor,
    _In_ PRECT Margin
    );

PHLIBAPI
VOID
NTAPI
PhLayoutManagerLayout(
    _Inout_ PPH_LAYOUT_MANAGER Manager
    );

PHLIBAPI
VOID
NTAPI
PhLayoutManagerUpdate(
    _Inout_ PPH_LAYOUT_MANAGER Manager,
    _In_ LONG WindowDpi
    );

#define PH_WINDOW_CONTEXT_DEFAULT 0xFFFF

PHLIBAPI
PVOID
NTAPI
PhGetWindowContext(
    _In_ HWND WindowHandle,
    _In_ ULONG PropertyHash
    );

PHLIBAPI
VOID
NTAPI
PhSetWindowContext(
    _In_ HWND WindowHandle,
    _In_ ULONG PropertyHash,
    _In_ PVOID Context
    );

PHLIBAPI
VOID
NTAPI
PhRemoveWindowContext(
    _In_ HWND WindowHandle,
    _In_ ULONG PropertyHash
    );

/**
 * Retrieves the window context pointer associated with a window handle.
 *
 * \param[in] WindowHandle A handle to the window from which to retrieve the context.
 * \return A pointer to the window context, or NULL if no context has been set. * *
 */
FORCEINLINE
PVOID
NTAPI
PhGetWindowContextEx(
    _In_ HWND WindowHandle
    )
{
#if defined(PHNT_WINDOW_CLASS_CONTEXT)
    return PhGetWindowContext(WindowHandle, MAXCHAR);
#else
    //assert(GetClassLongPtr(WindowHandle, GCL_CBWNDEXTRA) == sizeof(PVOID));
    return (PVOID)GetWindowLongPtr(WindowHandle, 0);
#endif
}

/**
 * Sets the extended window context for a window handle.
 * 
 * \param[in] WindowHandle The handle to the window for which to set the context.
 * \param[in] Context A pointer to the context data to associate with the window.
 * \return This function does not return a value.
 * \remarks The window must have sufficient extra bytes allocated to store a PVOID
 * if PHNT_WINDOW_CLASS_CONTEXT is not defined.
 */
FORCEINLINE
VOID
NTAPI
PhSetWindowContextEx(
    _In_ HWND WindowHandle,
    _In_ PVOID Context
    )
{
#if defined(PHNT_WINDOW_CLASS_CONTEXT)
    PhSetWindowContext(WindowHandle, MAXCHAR, Context);
#else
    //assert(GetClassLongPtr(WindowHandle, GCL_CBWNDEXTRA) == sizeof(PVOID));
    SetWindowLongPtr(WindowHandle, 0, (LONG_PTR)Context);
#endif
}

/**
 * Removes the window context from a window handle.
 *
 * \param[in] WindowHandle The handle to the window from which to remove the context.
 * \remarks
 * If PHNT_WINDOW_CLASS_CONTEXT is defined, this function delegates to PhRemoveWindowContext
 * with MAXCHAR as the context identifier. Otherwise, it clears the window's extra data by
 * setting the window long pointer at offset 0 to NULL.
 */
FORCEINLINE
VOID
NTAPI
PhRemoveWindowContextEx(
    _In_ HWND WindowHandle
    )
{
#if defined(PHNT_WINDOW_CLASS_CONTEXT)
    PhRemoveWindowContext(WindowHandle, MAXCHAR);
#else
    //assert(GetClassLongPtr(WindowHandle, GCL_CBWNDEXTRA) == sizeof(PVOID));
    SetWindowLongPtr(WindowHandle, 0, (LONG_PTR)NULL);
#endif
}

FORCEINLINE
PVOID
NTAPI
PhGetDialogContext(
    _In_ HWND WindowHandle
    )
{
#if defined(PHNT_WINDOW_CLASS_CONTEXT)
    return PhGetWindowContext(WindowHandle, MAXCHAR);
#else
    return (PVOID)GetWindowLongPtr(WindowHandle, DWLP_USER);
#endif
}

FORCEINLINE
VOID
NTAPI
PhSetDialogContext(
    _In_ HWND WindowHandle,
    _In_ PVOID Context
    )
{
#if defined(PHNT_WINDOW_CLASS_CONTEXT)
    PhSetWindowContext(WindowHandle, MAXCHAR, Context);
#else
    SetWindowLongPtr(WindowHandle, DWLP_USER, (LONG_PTR)Context);
#endif
}

FORCEINLINE
VOID
NTAPI
PhRemoveDialogContext(
    _In_ HWND WindowHandle
    )
{
#if defined(PHNT_WINDOW_CLASS_CONTEXT)
    PhRemoveWindowContext(WindowHandle, MAXCHAR);
#else
    SetWindowLongPtr(WindowHandle, DWLP_USER, (LONG_PTR)NULL);
#endif
}

FORCEINLINE
VOID
NTAPI
PhSetWindowRedraw(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN Enable
    )
{
    SendMessage(WindowHandle, WM_SETREDRAW, Enable, 0);
}

FORCEINLINE
BOOL
NTAPI
PhInvalidateRect(
    _In_ HWND WindowHandle,
    _In_opt_ CONST RECT* Rect,
    _In_ BOOL Erase
    )
{
    return InvalidateRect(WindowHandle, Rect, Erase);
}

FORCEINLINE
VOID
NTAPI
PhUpdateWindow(
    _In_ HWND WindowHandle
    )
{
    UpdateWindow(WindowHandle);
}

FORCEINLINE
VOID
NTAPI
PhInvalidateUpdateWindow(
    _In_ HWND WindowHandle
    )
{
    PhInvalidateRect(WindowHandle, NULL, TRUE);
    PhUpdateWindow(WindowHandle);
}

FORCEINLINE
VOID
NTAPI
PhRedrawWindow(
    _In_ HWND WindowHandle
    )
{
    // You should use RedrawWindow with the specified flags, instead of InvalidateRect,
    // because the former is necessary for some controls that have nonclient area of their own,
    // or have window styles that cause them to be given a nonclient area (such as WS_THICKFRAME, WS_BORDER, or WS_EX_CLIENTEDGE).
    // If the control does not have a nonclient area, then RedrawWindow with these flags
    // will do only as much invalidation as InvalidateRect would.
    // https://learn.microsoft.com/en-us/windows/win32/gdi/wm-setredraw

    RedrawWindow(WindowHandle, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

typedef _Function_class_(PH_WINDOW_ENUM_CALLBACK)
BOOLEAN NTAPI PH_WINDOW_ENUM_CALLBACK(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    );
typedef PH_WINDOW_ENUM_CALLBACK* PPH_WINDOW_ENUM_CALLBACK;

PHLIBAPI
NTSTATUS
NTAPI
PhEnumWindows(
    _In_ PH_WINDOW_ENUM_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumWindowsEx(
    _In_opt_ HWND ParentWindow,
    _In_ PH_WINDOW_ENUM_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumGetWindow(
    _In_opt_ HWND StartWindow,
    _In_ ULONG Command,
    _In_ PH_WINDOW_ENUM_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumWindowsZOrder(
    _In_ PH_WINDOW_ENUM_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhEnumChildWindows(
    _In_opt_ HWND WindowHandle,
    _In_ PH_WINDOW_ENUM_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhBuildHwndList(
    _In_opt_ HANDLE DesktopHandle,
    _In_opt_ HWND ParentWindowHandle,
    _In_ BOOLEAN IncludeChildren,
    _In_ BOOLEAN ExcludeImmersive,
    _In_opt_ HANDLE ThreadId,
    _Out_ PULONG NumberOfHandles,
    _Outptr_result_buffer_(*NumberOfHandles) HWND** Handles
    );

HWND
NTAPI
PhGetProcessMainWindow(
    _In_opt_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle
    );

HWND
NTAPI
PhGetProcessMainWindowEx(
    _In_opt_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle,
    _In_ BOOLEAN SkipInvisible
    );

PHLIBAPI
ULONG
NTAPI
PhGetDialogItemValue(
    _In_ HWND WindowHandle,
    _In_ LONG ControlID
    );

PHLIBAPI
VOID
NTAPI
PhSetDialogItemValue(
    _In_ HWND WindowHandle,
    _In_ LONG ControlID,
    _In_ ULONG Value,
    _In_ BOOLEAN Signed
    );

PHLIBAPI
BOOLEAN
NTAPI
PhSetDialogItemText(
    _In_ HWND WindowHandle,
    _In_ LONG ControlID,
    _In_ PCWSTR WindowText
    );

PHLIBAPI
BOOLEAN
NTAPI
PhSetWindowText(
    _In_ HWND WindowHandle,
    _In_ PCWSTR WindowText
    );

PHLIBAPI
VOID
NTAPI
PhSetGroupBoxText(
    _In_ HWND WindowHandle,
    _In_ PCWSTR WindowText
    );

PHLIBAPI
VOID
NTAPI
PhSetWindowAlwaysOnTop(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN AlwaysOnTop
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhSendMessageTimeout(
    _In_ HWND WindowHandle,
    _In_ ULONG WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ ULONG Timeout,
    _Out_opt_ PULONG_PTR Result
    );

FORCEINLINE ULONG PhGetWindowTextLength(
    _In_ HWND WindowHandle
    )
{
    return (ULONG)SendMessage(WindowHandle, WM_GETTEXTLENGTH, 0, 0); // DefWindowProc
}

FORCEINLINE VOID PhSetDialogFocus(
    _In_ HWND WindowHandle,
    _In_ HWND FocusHandle
    )
{
    // Do not use the SendMessage function to send a WM_NEXTDLGCTL message if your application will
    // concurrently process other messages that set the focus. Use the PostMessage function instead.
    SendMessage(WindowHandle, WM_NEXTDLGCTL, (WPARAM)FocusHandle, MAKELPARAM(TRUE, 0));
}

FORCEINLINE VOID PhResizingMinimumSize(
    _Inout_ PRECT Rect,
    _In_ WPARAM Edge,
    _In_ LONG MinimumWidth,
    _In_ LONG MinimumHeight
    )
{
    if (Edge == WMSZ_BOTTOMRIGHT || Edge == WMSZ_RIGHT || Edge == WMSZ_TOPRIGHT)
    {
        if (Rect->right - Rect->left < MinimumWidth)
            Rect->right = Rect->left + MinimumWidth;
    }
    else if (Edge == WMSZ_BOTTOMLEFT || Edge == WMSZ_LEFT || Edge == WMSZ_TOPLEFT)
    {
        if (Rect->right - Rect->left < MinimumWidth)
            Rect->left = Rect->right - MinimumWidth;
    }

    if (Edge == WMSZ_BOTTOMRIGHT || Edge == WMSZ_BOTTOM || Edge == WMSZ_BOTTOMLEFT)
    {
        if (Rect->bottom - Rect->top < MinimumHeight)
            Rect->bottom = Rect->top + MinimumHeight;
    }
    else if (Edge == WMSZ_TOPRIGHT || Edge == WMSZ_TOP || Edge == WMSZ_TOPLEFT)
    {
        if (Rect->bottom - Rect->top < MinimumHeight)
            Rect->top = Rect->bottom - MinimumHeight;
    }
}

FORCEINLINE VOID PhCopyControlRectangle(
    _In_ HWND ParentWindowHandle,
    _In_ HWND FromControlHandle,
    _In_ HWND ToControlHandle
    )
{
    RECT windowRect;

    if (!PhGetWindowRect(FromControlHandle, &windowRect))
        return;

    MapWindowRect(NULL, ParentWindowHandle, &windowRect);
    MoveWindow(ToControlHandle, windowRect.left, windowRect.top,
        windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, FALSE);
}

// icotobmp

PHLIBAPI
HBITMAP
NTAPI
PhIconToBitmap(
    _In_ HICON Icon,
    _In_ LONG Width,
    _In_ LONG Height
    );

PHLIBAPI
VOID
NTAPI
PhBitmapSetAlpha(
    _In_ PVOID Bits,
    _In_ LONG Width,
    _In_ LONG Height
    );

//
// Extended ListView
//

#define PH_ALIGN_CENTER 0x0
#define PH_ALIGN_LEFT 0x1
#define PH_ALIGN_RIGHT 0x2
#define PH_ALIGN_TOP 0x4
#define PH_ALIGN_BOTTOM 0x8

#define PH_ALIGN_MONOSPACE_FONT 0x80000000

typedef enum _PH_ITEM_STATE
{
    // The item is normal. Use the ItemColorFunction to determine the color of the item.
    NormalItemState = 0,
    // The item is new. On the next tick, change the state to NormalItemState. When an item is in
    // this state, highlight it in NewColor.
    NewItemState,
    // The item is being removed. On the next tick, delete the item. When an item is in this state,
    // highlight it in RemovingColor.
    RemovingItemState
} PH_ITEM_STATE;

typedef _Function_class_(PH_EXTLV_GET_ITEM_COLOR)
COLORREF NTAPI PH_EXTLV_GET_ITEM_COLOR(
    _In_ LONG Index,
    _In_ PVOID Param,
    _In_opt_ PVOID Context
    );
typedef PH_EXTLV_GET_ITEM_COLOR* PPH_EXTLV_GET_ITEM_COLOR;

typedef _Function_class_(PH_EXTLV_GET_ITEM_FONT)
HFONT NTAPI PH_EXTLV_GET_ITEM_FONT(
    _In_ LONG Index,
    _In_ PVOID Param,
    _In_opt_ PVOID Context
    );
typedef PH_EXTLV_GET_ITEM_FONT* PPH_EXTLV_GET_ITEM_FONT;

PHLIBAPI
VOID
NTAPI
PhSetExtendedListView(
    _In_ HWND WindowHandle
    );

PHLIBAPI
VOID
NTAPI
PhSetExtendedListViewEx(
    _In_ HWND WindowHandle,
    _In_ ULONG SortColumn,
    _In_ ULONG SortOrder
    );

PHLIBAPI
VOID
NTAPI
PhSetHeaderSortIcon(
    _In_ HWND hwnd,
    _In_ LONG Index,
    _In_ PH_SORT_ORDER Order
    );

// next 1122

#define ELVM_ADDFALLBACKCOLUMN (WM_APP + 1106)
#define ELVM_ADDFALLBACKCOLUMNS (WM_APP + 1109)
#define ELVM_RESERVED5 (WM_APP + 1120)
#define ELVM_INIT (WM_APP + 1102)
#define ELVM_SETCOLUMNWIDTH (WM_APP + 1121)
#define ELVM_SETCOMPAREFUNCTION (WM_APP + 1104)
#define ELVM_SETCONTEXT (WM_APP + 1103)
#define ELVM_SETCURSOR (WM_APP + 1114)
#define ELVM_RESERVED4 (WM_APP + 1118)
#define ELVM_SETITEMCOLORFUNCTION (WM_APP + 1111)
#define ELVM_SETITEMFONTFUNCTION (WM_APP + 1117)
#define ELVM_RESERVED1 (WM_APP + 1112)
#define ELVM_SETREDRAW (WM_APP + 1116)
#define ELVM_GETSORT (WM_APP + 1113)
#define ELVM_SETSORT (WM_APP + 1108)
#define ELVM_SETSORTFAST (WM_APP + 1119)
#define ELVM_RESERVED0 (WM_APP + 1110)
#define ELVM_SETTRISTATE (WM_APP + 1107)
#define ELVM_SETTRISTATECOMPAREFUNCTION (WM_APP + 1105)
#define ELVM_SORTITEMS (WM_APP + 1101)
#define ELVM_RESERVED3 (WM_APP + 1115)

#define ExtendedListView_AddFallbackColumn(hWnd, Column) \
    SendMessage((hWnd), ELVM_ADDFALLBACKCOLUMN, (WPARAM)(Column), 0)
#define ExtendedListView_AddFallbackColumns(hWnd, NumberOfColumns, Columns) \
    SendMessage((hWnd), ELVM_ADDFALLBACKCOLUMNS, (WPARAM)(NumberOfColumns), (LPARAM)(Columns))
#define ExtendedListView_Init(hWnd) \
    SendMessage((hWnd), ELVM_INIT, 0, 0)
#define ExtendedListView_SetColumnWidth(hWnd, Column, Width) \
    SendMessage((hWnd), ELVM_SETCOLUMNWIDTH, (WPARAM)(Column), (LPARAM)(Width))
#define ExtendedListView_SetCompareFunction(hWnd, Column, CompareFunction) \
    SendMessage((hWnd), ELVM_SETCOMPAREFUNCTION, (WPARAM)(Column), (LPARAM)(CompareFunction))
#define ExtendedListView_SetContext(hWnd, Context) \
    SendMessage((hWnd), ELVM_SETCONTEXT, 0, (LPARAM)(Context))
#define ExtendedListView_SetCursor(hWnd, Cursor) \
    SendMessage((hWnd), ELVM_SETCURSOR, 0, (LPARAM)(Cursor))
#define ExtendedListView_SetItemColorFunction(hWnd, ItemColorFunction) \
    SendMessage((hWnd), ELVM_SETITEMCOLORFUNCTION, 0, (LPARAM)(ItemColorFunction))
#define ExtendedListView_SetItemFontFunction(hWnd, ItemFontFunction) \
    SendMessage((hWnd), ELVM_SETITEMFONTFUNCTION, 0, (LPARAM)(ItemFontFunction))
#define ExtendedListView_SetRedraw(hWnd, Redraw) \
    SendMessage((hWnd), ELVM_SETREDRAW, (WPARAM)(Redraw), 0)
#define ExtendedListView_GetSort(hWnd, Column, Order) \
    SendMessage((hWnd), ELVM_GETSORT, (WPARAM)(Column), (LPARAM)(Order))
#define ExtendedListView_SetSort(hWnd, Column, Order) \
    SendMessage((hWnd), ELVM_SETSORT, (WPARAM)(Column), (LPARAM)(Order))
#define ExtendedListView_SetSortFast(hWnd, Fast) \
    SendMessage((hWnd), ELVM_SETSORTFAST, (WPARAM)(Fast), 0)
#define ExtendedListView_SetTriState(hWnd, TriState) \
    SendMessage((hWnd), ELVM_SETTRISTATE, (WPARAM)(TriState), 0)
#define ExtendedListView_SetTriStateCompareFunction(hWnd, CompareFunction) \
    SendMessage((hWnd), ELVM_SETTRISTATECOMPAREFUNCTION, 0, (LPARAM)(CompareFunction))
#define ExtendedListView_SortItems(hWnd) \
    SendMessage((hWnd), ELVM_SORTITEMS, 0, 0)

#define ELVSCW_AUTOSIZE (-1)
#define ELVSCW_AUTOSIZE_USEHEADER (-2)
#define ELVSCW_AUTOSIZE_REMAININGSPACE (-3)

//
// Listview Wrappers
//

typedef struct _PH_LISTVIEW_CONTEXT
{
    HWND ListViewHandle;
    IListView* ListViewInterface;
    HANDLE ThreadId;
} PH_LISTVIEW_CONTEXT, *PPH_LISTVIEW_CONTEXT;

PHLIBAPI
PPH_LISTVIEW_CONTEXT
NTAPI
PhListView_Initialize(
    _In_ HWND ListViewHandle
    );

PHLIBAPI
VOID
NTAPI
PhListView_Destroy(
    _In_ PPH_LISTVIEW_CONTEXT Context
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhListView_GetItemCount(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _Out_ PLONG ItemCount
    );

PHLIBAPI
BOOLEAN
NTAPI
PhListView_SetItemCount(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG ItemCount,
    _In_ ULONG Flags
    );

PHLIBAPI
BOOLEAN
NTAPI
PhListView_GetItem(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _Inout_ LVITEM* Item
    );

PHLIBAPI
BOOLEAN
NTAPI
PhListView_SetItem(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LVITEM* Item
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhListView_GetItemText(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG ItemIndex,
    _In_ LONG SubItemIndex,
    _Out_writes_(BufferSize) PWSTR Buffer,
    _In_ LONG BufferSize
    );

PHLIBAPI
BOOLEAN
NTAPI
PhListView_SetItemText(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG ItemIndex,
    _In_ LONG SubItemIndex,
    _In_ PWSTR Text
    );

PHLIBAPI
BOOLEAN
NTAPI
PhListView_DeleteItem(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG ItemIndex
    );

PHLIBAPI
BOOLEAN
NTAPI
PhListView_DeleteAllItems(
    _In_ PPH_LISTVIEW_CONTEXT Context
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhListView_InsertItem(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LVITEMW* Item,
    _Out_opt_ PLONG ItemIndex
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhListView_InsertGroup(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG InsertAt,
    _In_ LVGROUP* Group,
    _Out_opt_ PLONG GroupId
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhListView_GetItemState(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG ItemIndex,
    _In_ ULONG Mask,
    _Out_ PULONG State
    );

PHLIBAPI
BOOLEAN
NTAPI
PhListView_SetItemState(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG ItemIndex,
    _In_ ULONG State,
    _In_ ULONG Mask
    );

PHLIBAPI
BOOLEAN
NTAPI
PhListView_SortItems(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ BOOL SortingByIndex,
    _In_ PFNLVCOMPARE Compare,
    _In_ PVOID CompareContext
    );

PHLIBAPI
BOOLEAN
NTAPI
PhListView_GetColumn(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ ULONG ColumnIndex,
    _Inout_ LV_COLUMN* Column
    );

PHLIBAPI
BOOLEAN
NTAPI
PhListView_SetColumn(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ ULONG ColumnIndex,
    _In_ LV_COLUMN* Column
    );

PHLIBAPI
BOOLEAN
NTAPI
PhListView_SetColumnWidth(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ ULONG ColumnIndex,
    _In_ ULONG Width
    );

PHLIBAPI
BOOLEAN
NTAPI
PhListView_GetHeader(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _Out_ HWND* WindowHandle
    );

PHLIBAPI
BOOLEAN
NTAPI
PhListView_GetToolTip(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _Out_ HWND* WindowHandle
    );

PHLIBAPI
LONG
NTAPI
PhListView_AddColumn(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG Index,
    _In_ LONG DisplayIndex,
    _In_ LONG SubItemIndex,
    _In_ LONG Format,
    _In_ LONG Width,
    _In_ PCWSTR Text
    );

PHLIBAPI
LONG
NTAPI
PhListView_AddItem(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG Index,
    _In_ PCWSTR Text,
    _In_opt_ PVOID Param
    );

PHLIBAPI
LONG
NTAPI
PhListView_FindItemByFlags(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG StartIndex,
    _In_ ULONG Flags
    );

PHLIBAPI
LONG
NTAPI
PhListView_FindItemByParam(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG StartIndex,
    _In_opt_ PVOID Param
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhListView_GetItemParam(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG Index,
    _Outptr_ PVOID* Param
    );

PHLIBAPI
VOID
NTAPI
PhListView_SetSubItem(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG Index,
    _In_ LONG SubItemIndex,
    _In_ PCWSTR Text
    );

PHLIBAPI
VOID
NTAPI
PhListView_RedrawItems(
    _In_ PPH_LISTVIEW_CONTEXT Context
    );

PHLIBAPI
LONG
NTAPI
PhListView_AddGroup(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG GroupId,
    _In_ PCWSTR Text
    );

PHLIBAPI
LONG
NTAPI
PhListView_AddGroupItem(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG GroupId,
    _In_ LONG Index,
    _In_ PCWSTR Text,
    _In_opt_ PVOID Param
    );

PHLIBAPI
VOID
NTAPI
PhListView_SetStateAllItems(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ ULONG State,
    _In_ ULONG Mask
    );

PHLIBAPI
PVOID
NTAPI
PhListView_GetSelectedItemParam(
    _In_ PPH_LISTVIEW_CONTEXT Context
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhListView_GetSelectedCount(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _Out_ PLONG SelectedCount
    );

PHLIBAPI
VOID
NTAPI
PhListView_GetSelectedItemParams(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _Out_ PVOID** Items,
    _Out_ PULONG NumberOfItems
    );

PHLIBAPI
BOOLEAN
NTAPI
PhListView_GetClientRect(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _Inout_ PRECT ClientRect
    );

PHLIBAPI
BOOLEAN
NTAPI
PhListView_GetItemRect(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG StartIndex,
    _In_ ULONG Flags,
    _Inout_ PRECT ItemRect
    );

PHLIBAPI
BOOLEAN
NTAPI
PhListView_EnableGroupView(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ BOOLEAN Enable
    );

PHLIBAPI
BOOLEAN
NTAPI
PhListView_EnsureItemVisible(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG ItemIndex,
    _In_ BOOLEAN PartialOk
    );

PHLIBAPI
BOOLEAN
NTAPI
PhListView_IsItemVisible(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _In_ LONG ItemIndex,
    _Out_ PBOOLEAN Visible
    );

PHLIBAPI
BOOLEAN
NTAPI
PhListView_HitTestSubItem(
    _In_ PPH_LISTVIEW_CONTEXT Context,
    _Inout_ LVHITTESTINFO * HitTestInfo
    );

/**
 * Gets the brightness of a color.
 *
 * \param Color The color.
 *
 * \return A value ranging from 0 to 255, indicating the brightness of the color.
 */
FORCEINLINE
ULONG
PhGetColorBrightness(
    _In_ COLORREF Color
    )
{
    ULONG r = Color & 0xff;
    ULONG g = (Color >> 8) & 0xff;
    ULONG b = (Color >> 16) & 0xff;
    ULONG min;
    ULONG max;

    min = r;
    if (g < min) min = g;
    if (b < min) min = b;

    max = r;
    if (g > max) max = g;
    if (b > max) max = b;

    return (min + max) / 2;
}

FORCEINLINE
COLORREF
PhHalveColorBrightness(
    _In_ COLORREF Color
    )
{
    /*return RGB(
        (UCHAR)Color / 2,
        (UCHAR)(Color >> 8) / 2,
        (UCHAR)(Color >> 16) / 2
        );*/
    // Since all targets are little-endian, we can use the following method.
    *((PUCHAR)&Color) /= 2;
    *((PUCHAR)&Color + 1) /= 2;
    *((PUCHAR)&Color + 2) /= 2;

    return Color;
}

FORCEINLINE
COLORREF
PhMakeColorBrighter(
    _In_ COLORREF Color,
    _In_ UCHAR Increment
    )
{
    UCHAR r;
    UCHAR g;
    UCHAR b;

    r = (UCHAR)Color;
    g = (UCHAR)(Color >> 8);
    b = (UCHAR)(Color >> 16);

    if (r <= 255 - Increment)
        r += Increment;
    else
        r = 255;

    if (g <= 255 - Increment)
        g += Increment;
    else
        g = 255;

    if (b <= 255 - Increment)
        b += Increment;
    else
        b = 255;

    return RGB(r, g, b);
}

// Window support

typedef enum _PH_PLUGIN_WINDOW_EVENT_TYPE
{
    PH_PLUGIN_WINDOW_EVENT_TYPE_NONE,
    PH_PLUGIN_WINDOW_EVENT_TYPE_TOPMOST,
    PH_PLUGIN_WINDOW_EVENT_TYPE_MAX
} PH_PLUGIN_WINDOW_EVENT_TYPE;

typedef struct _PH_PLUGIN_WINDOW_CALLBACK_REGISTRATION
{
    HWND WindowHandle;
    PH_PLUGIN_WINDOW_EVENT_TYPE Type;
} PH_PLUGIN_WINDOW_CALLBACK_REGISTRATION, *PPH_PLUGIN_WINDOW_CALLBACK_REGISTRATION;

typedef struct _PH_PLUGIN_WINDOW_NOTIFY_EVENT
{
    PH_PLUGIN_WINDOW_EVENT_TYPE Type;
    union
    {
        BOOLEAN TopMost;
        //HFONT FontHandle;
    };
} PH_PLUGIN_WINDOW_NOTIFY_EVENT, *PPH_PLUGIN_WINDOW_NOTIFY_EVENT;

typedef struct _PH_PLUGIN_MAINWINDOW_NOTIFY_EVENT
{
    PPH_PLUGIN_WINDOW_NOTIFY_EVENT Event;
    PPH_PLUGIN_WINDOW_CALLBACK_REGISTRATION Callback;
} PH_PLUGIN_MAINWINDOW_NOTIFY_EVENT, *PPH_PLUGIN_MAINWINDOW_NOTIFY_EVENT;

PHLIBAPI
VOID
NTAPI
PhRegisterWindowCallback(
    _In_ HWND WindowHandle,
    _In_ PH_PLUGIN_WINDOW_EVENT_TYPE Type,
    _In_opt_ PVOID Context
    );

PHLIBAPI
VOID
NTAPI
PhUnregisterWindowCallback(
    _In_ HWND WindowHandle
    );

PHLIBAPI
VOID
NTAPI
PhWindowNotifyTopMostEvent(
    _In_ BOOLEAN TopMost
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateEnvironmentBlock(
    _Out_ PVOID* Environment,
    _In_opt_ HANDLE TokenHandle,
    _In_ BOOLEAN Inherit
    );

PHLIBAPI
VOID
NTAPI
PhDestroyEnvironmentBlock(
    _In_ _Post_invalid_ PVOID Environment
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhRegenerateUserEnvironment(
    _Out_opt_ PVOID* NewEnvironment,
    _In_ BOOLEAN UpdateCurrentEnvironment
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIsImmersiveProcess(
    _In_ HANDLE ProcessHandle
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetProcessUIContextInformation(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_UICONTEXT_INFORMATION UIContext
    );

typedef enum _PH_PROCESS_DPI_AWARENESS
{
    PH_PROCESS_DPI_AWARENESS_UNAWARE = 0,
    PH_PROCESS_DPI_AWARENESS_SYSTEM_DPI_AWARE = 1,
    PH_PROCESS_DPI_AWARENESS_PER_MONITOR_DPI_AWARE = 2,
    PH_PROCESS_DPI_AWARENESS_PER_MONITOR_AWARE_V2 = 3,
    PH_PROCESS_DPI_AWARENESS_UNAWARE_GDISCALED = 4,
} PH_PROCESS_DPI_AWARENESS, *PPH_PROCESS_DPI_AWARENESS;

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetProcessDpiAwareness(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_PROCESS_DPI_AWARENESS ProcessDpiAwareness
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetPhysicallyInstalledSystemMemory(
    _Out_ PULONGLONG TotalMemory,
    _Out_ PULONGLONG ReservedMemory
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetSessionGuiResources(
    _In_ ULONG Flags,
    _Out_ PULONG Total
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetProcessGuiResources(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG Flags,
    _Out_ PULONG Total
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetThreadWin32Thread(
    _In_ HANDLE ThreadId
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetSendMessageReceiver(
    _In_ HANDLE ThreadId,
    _Out_ HWND *WindowHandle
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhExtractIcon(
    _In_ PCWSTR FileName,
    _Out_opt_ HICON *IconLarge,
    _Out_opt_ HICON *IconSmall
    );

PHLIBAPI
NTSTATUS
NTAPI
PhExtractIconEx(
    _In_ PCPH_STRINGREF FileName,
    _In_ BOOLEAN NativeFileName,
    _In_ LONG IconIndex,
    _In_ LONG IconLargeWidth,
    _In_ LONG IconLargeHeight,
    _In_ LONG IconSmallWidth,
    _In_ LONG IconSmallHeight,
    _Out_opt_ HICON *IconLarge,
    _Out_opt_ HICON *IconSmall
    );

// Imagelist support

PHLIBAPI
HIMAGELIST
NTAPI
PhImageListCreate(
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ LONG Flags,
    _In_ LONG InitialCount,
    _In_ LONG GrowCount
    );

PHLIBAPI
BOOLEAN
NTAPI
PhImageListDestroy(
    _In_opt_ HIMAGELIST ImageListHandle
    );

PHLIBAPI
BOOLEAN
NTAPI
PhImageListSetImageCount(
    _In_ HIMAGELIST ImageListHandle,
    _In_ ULONG Count
    );

PHLIBAPI
BOOLEAN
NTAPI
PhImageListGetImageCount(
    _In_ HIMAGELIST ImageListHandle,
    _Out_ PLONG Count
    );

PHLIBAPI
BOOLEAN
NTAPI
PhImageListSetBkColor(
    _In_ HIMAGELIST ImageListHandle,
    _In_ COLORREF BackgroundColor
    );

PHLIBAPI
BOOLEAN
NTAPI
PhImageListRemoveIcon(
    _In_ HIMAGELIST ImageListHandle,
    _In_ LONG Index
    );

#define PhImageListRemoveAll(ImageListHandle) \
    PhImageListRemoveIcon((ImageListHandle), INT_ERROR)

PHLIBAPI
LONG
NTAPI
PhImageListAddIcon(
    _In_ HIMAGELIST ImageListHandle,
    _In_ HICON IconHandle
    );

PHLIBAPI
LONG
NTAPI
PhImageListAddBitmap(
    _In_ HIMAGELIST ImageListHandle,
    _In_ HBITMAP BitmapImage,
    _In_opt_ HBITMAP BitmapMask
    );

PHLIBAPI
HICON
NTAPI
PhImageListGetIcon(
    _In_ HIMAGELIST ImageListHandle,
    _In_ LONG Index,
    _In_ ULONG Flags
    );

PHLIBAPI
BOOLEAN
NTAPI
PhImageListGetIconSize(
    _In_ HIMAGELIST ImageListHandle,
    _Out_ PLONG cx,
    _Out_ PLONG cy
    );

PHLIBAPI
BOOLEAN
NTAPI
PhImageListReplace(
    _In_ HIMAGELIST ImageListHandle,
    _In_ LONG Index,
    _In_ HBITMAP BitmapImage,
    _In_opt_ HBITMAP BitmapMask
    );

PHLIBAPI
BOOLEAN
NTAPI
PhImageListDrawIcon(
    _In_ HIMAGELIST ImageListHandle,
    _In_ LONG Index,
    _In_ HDC Hdc,
    _In_ LONG x,
    _In_ LONG y,
    _In_ UINT32 Style,
    _In_ BOOLEAN Disabled
    );

PHLIBAPI
BOOLEAN
NTAPI
PhImageListDrawEx(
    _In_ HIMAGELIST ImageListHandle,
    _In_ LONG Index,
    _In_ HDC Hdc,
    _In_ LONG x,
    _In_ LONG y,
    _In_ LONG dx,
    _In_ LONG dy,
    _In_ COLORREF BackColor,
    _In_ COLORREF ForeColor,
    _In_ UINT32 Style,
    _In_ ULONG State
    );

PHLIBAPI
BOOLEAN
NTAPI
PhImageListSetIconSize(
    _In_ HIMAGELIST ImageListHandle,
    _In_ LONG cx,
    _In_ LONG cy
    );

#define PH_SHUTDOWN_RESTART 0x1
#define PH_SHUTDOWN_POWEROFF 0x2
#define PH_SHUTDOWN_INSTALL_UPDATES 0x4
#define PH_SHUTDOWN_HYBRID 0x8
#define PH_SHUTDOWN_RESTART_BOOTOPTIONS 0x10

PHLIBAPI
ULONG
NTAPI
PhInitiateShutdown(
    _In_ ULONG Flags
    );

PHLIBAPI
BOOLEAN
NTAPI
PhSetProcessShutdownParameters(
    _In_ ULONG Level,
    _In_ ULONG Flags
    );

#define PH_DRAW_TIMELINE_OVERFLOW 0x1
#define PH_DRAW_TIMELINE_DARKTHEME 0x2

PHLIBAPI
VOID
NTAPI
PhCustomDrawTreeTimeLine(
    _In_ HDC Hdc,
    _In_ PRECT CellRect,
    _In_ ULONG Flags,
    _In_opt_ PLARGE_INTEGER StartTime,
    _In_ PLARGE_INTEGER CreateTime
    );

//
// Windows Imaging Component (WIC) bitmap support
//

DEFINE_GUID(IID_IWICBitmapSource, 0x00000120, 0xa8f2, 0x4877, 0xba, 0x0a, 0xfd, 0x2b, 0x66, 0x45, 0xfb, 0x94);
DEFINE_GUID(IID_IWICImagingFactory, 0xec5ec8a9, 0xc395, 0x4314, 0x9c, 0x77, 0x54, 0xd7, 0xa9, 0x35, 0xff, 0x70);

typedef enum _PH_BUFFERFORMAT
{
    PHBF_COMPATIBLEBITMAP,    // Compatible bitmap
    PHBF_DIB,                 // Device-independent bitmap
    PHBF_TOPDOWNDIB,          // Top-down device-independent bitmap
    PHBF_TOPDOWNMONODIB       // Top-down monochrome device-independent bitmap
} PH_BUFFERFORMAT;

HBITMAP PhCreateDIBSection(
    _In_ HDC Hdc,
    _In_ PH_BUFFERFORMAT Format,
    _In_ LONG Width,
    _In_ LONG Height,
    _Out_opt_ PVOID* Bits
    );

typedef enum _PH_IMAGE_FORMAT_TYPE
{
    PH_IMAGE_FORMAT_TYPE_NONE,
    PH_IMAGE_FORMAT_TYPE_ICO,
    PH_IMAGE_FORMAT_TYPE_BMP,
    PH_IMAGE_FORMAT_TYPE_JPG,
    PH_IMAGE_FORMAT_TYPE_PNG,
} PH_IMAGE_FORMAT_TYPE, *PPH_IMAGE_FORMAT_TYPE;

PHLIBAPI
HBITMAP
NTAPI
PhLoadImageFormatFromResource(
    _In_ PVOID DllBase,
    _In_ PCWSTR Name,
    _In_ PCWSTR Type,
    _In_ PH_IMAGE_FORMAT_TYPE Format,
    _In_ LONG Width,
    _In_ LONG Height
    );

PHLIBAPI
HBITMAP
NTAPI
PhLoadImageFromAddress(
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength,
    _In_ LONG Width,
    _In_ LONG Height
    );

PHLIBAPI
HBITMAP
NTAPI
PhLoadImageFromResource(
    _In_ PVOID DllBase,
    _In_ PCWSTR Name,
    _In_ PCWSTR Type,
    _In_ LONG Width,
    _In_ LONG Height
    );

PHLIBAPI
HBITMAP
NTAPI
PhLoadImageFromFile(
    _In_ PCWSTR FileName,
    _In_ LONG Width,
    _In_ LONG Height
    );

// Acrylic support

typedef enum _WINDOWCOMPOSITIONATTRIB
{
    WCA_UNDEFINED = 0,
    WCA_NCRENDERING_ENABLED = 1,                // q: BOOL
    WCA_NCRENDERING_POLICY = 2,                 // s: ULONG // DWMNCRENDERINGPOLICY
    WCA_TRANSITIONS_FORCEDISABLED = 3,          // s: BOOL
    WCA_ALLOW_NCPAINT = 4,                      // s: BOOL
    WCA_CAPTION_BUTTON_BOUNDS = 5,              // q: RECT
    WCA_NONCLIENT_RTL_LAYOUT = 6,               // s: BOOL
    WCA_FORCE_ICONIC_REPRESENTATION = 7,        // s: BOOL
    WCA_EXTENDED_FRAME_BOUNDS = 8,              // q: RECT
    WCA_HAS_ICONIC_BITMAP = 9,                  // q: BOOL
    WCA_THEME_ATTRIBUTES = 10,                  // s: ULONG
    WCA_NCRENDERING_EXILED = 11,                // q: BOOL
    WCA_NCADORNMENTINFO = 12,                   // q:
    WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,         // s: BOOL
    WCA_VIDEO_OVERLAY_ACTIVE = 14,              // q: BOOL
    WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,     // s: BOOL
    WCA_DISALLOW_PEEK = 16,                     // s: BOOL
    WCA_CLOAK = 17,                             // s: ULONG
    WCA_CLOAKED = 18,                           // q: ULONG // DWM_CLOAKED_*
    WCA_ACCENT_POLICY = 19,                     // s: ACCENT_POLICY // since WIN11
    WCA_FREEZE_REPRESENTATION = 20,             // s: BOOL
    WCA_EVER_UNCLOAKED = 21,                    // q: BOOL
    WCA_VISUAL_OWNER = 22,                      // s: HANDLE
    WCA_HOLOGRAPHIC = 23,                       // q: BOOL
    WCA_EXCLUDED_FROM_DDA = 24,                 // s: BOOL
    WCA_PASSIVEUPDATEMODE = 25,                 // s: BOOL
    WCA_USEDARKMODECOLORS = 26,                 // s: BOOL
    WCA_CORNER_STYLE = 27,                      // s: ULONG // DWM_WINDOW_CORNER_PREFERENCE
    WCA_PART_COLOR = 28,                        // s: ULONG // COLORREF
    WCA_DISABLE_MOVESIZE_FEEDBACK = 29,         // s: BOOL
    WCA_SYSTEMBACKDROP_TYPE = 30,               // s: ULONG // DWM_SYSTEMBACKDROP_TYPE
    WCA_SET_TAGGED_WINDOW_RECT = 31,            // s: RECT
    WCA_CLEAR_TAGGED_WINDOW_RECT = 32,          // s: RECT
    WCA_REMOTEAPP_POLICY = 33,                  // s: ULONG // since 24H2
    WCA_HAS_ACCENT_POLICY = 34,                 // q: BOOL
    WCA_REDIRECTIONBITMAP_FILL_COLOR = 35,      // s: ULONG // COLORREF
    WCA_REDIRECTIONBITMAP_ALPHA = 36,           // s: UCHAR
    WCA_BORDER_MARGINS = 37,                    // s: MARGINS
    WCA_LAST
} WINDOWCOMPOSITIONATTRIB;

typedef struct _WINDOWCOMPOSITIONATTRIBUTEDATA
{
    WINDOWCOMPOSITIONATTRIB Attribute;
    PVOID Data;
    SIZE_T Length;
} WINDOWCOMPOSITIONATTRIBUTEDATA, *PWINDOWCOMPOSITIONATTRIBUTEDATA;

PHLIBAPI
NTSTATUS
NTAPI
PhGetWindowCompositionAttribute(
    _In_ HWND WindowHandle,
    _Inout_ PWINDOWCOMPOSITIONATTRIBUTEDATA AttributeData
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetWindowCompositionAttribute(
    _In_ HWND WindowHandle,
    _In_ PWINDOWCOMPOSITIONATTRIBUTEDATA AttributeData
    );

// TODO: https://stackoverflow.com/questions/12304848/fast-algorithm-to-invert-an-argb-color-value-to-abgr/42133405#42133405
FORCEINLINE ULONG MakeARGB(
    _In_ BYTE a,
    _In_ BYTE r,
    _In_ BYTE g,
    _In_ BYTE b)
{
    return (((ULONG)(b) << 0) | ((ULONG)(g) << 8) | ((ULONG)(r) << 16) | ((ULONG)(a) << 24));
}

FORCEINLINE ULONG MakeABGR(
    _In_ BYTE a,
    _In_ BYTE b,
    _In_ BYTE g,
    _In_ BYTE r)
{
    return (((ULONG)(a) << 24) | ((ULONG)(b) << 16) | ((ULONG)(g) << 8) | ((ULONG)(r) << 0));
}

FORCEINLINE ULONG MakeARGBFromCOLORREF(_In_ BYTE Alpha, _In_ COLORREF rgb)
{
    return MakeARGB(Alpha, GetRValue(rgb), GetGValue(rgb), GetBValue(rgb));
}

FORCEINLINE ULONG MakeABGRFromCOLORREF(_In_ BYTE Alpha, _In_ COLORREF rgb)
{
    return MakeABGR(Alpha, GetBValue(rgb), GetGValue(rgb), GetRValue(rgb));
}

typedef enum _ACCENT_STATE
{
    ACCENT_DISABLED,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
    ACCENT_ENABLE_HOSTBACKDROP = 5,
    ACCENT_INVALID_STATE
} ACCENT_STATE;

typedef enum _ACCENT_FLAG
{
    ACCENT_NONE,
    ACCENT_WINDOWS11_LUMINOSITY = 0x2,
    ACCENT_BORDER_LEFT = 0x20,
    ACCENT_BORDER_TOP = 0x40,
    ACCENT_BORDER_RIGHT = 0x80,
    ACCENT_BORDER_BOTTOM = 0x100,
    ACCENT_BORDER_ALL = (ACCENT_BORDER_LEFT | ACCENT_BORDER_TOP | ACCENT_BORDER_RIGHT | ACCENT_BORDER_BOTTOM)
} ACCENT_FLAG;

typedef struct _ACCENT_POLICY
{
    ACCENT_STATE AccentState;
    ULONG AccentFlags;
    ULONG GradientColor;
    ULONG AnimationId;
} ACCENT_POLICY;

PHLIBAPI
NTSTATUS
NTAPI
PhSetWindowAcrylicCompositionColor(
    _In_ HWND WindowHandle,
    _In_ ULONG GradientColor
    );

PHLIBAPI
HCURSOR
NTAPI
PhLoadArrowCursor(
    VOID
    );

PHLIBAPI
HCURSOR
NTAPI
PhLoadDividerCursor(
    VOID
    );

PHLIBAPI
NTSTATUS
NTAPI
PhIsInteractiveUserSession(
    VOID
    );

typedef struct _PH_USER_OBJECT_SID
{
    union
    {
        SID Sid;
        BYTE Buffer[SECURITY_MAX_SID_SIZE];
    };
} PH_USER_OBJECT_SID, *PPH_USER_OBJECT_SID;

static_assert(sizeof(PH_USER_OBJECT_SID) == SECURITY_MAX_SID_SIZE, "sizeof(PH_USER_OBJECT_SID) is invalid.");

PHLIBAPI
NTSTATUS
NTAPI
PhGetUserObjectSidInformationToBuffer(
    _In_ HANDLE Handle,
    _Out_ PPH_USER_OBJECT_SID ObjectSid,
    _In_ ULONG ObjectSidLength,
    _Out_opt_ PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetUserObjectSidInformationCopy(
    _In_ HANDLE Handle,
    _Out_ PSID* ObjectSid
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetCurrentWindowStationName(
    VOID
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetCurrentThreadDesktopName(
    VOID
    );

PHLIBAPI
BOOLEAN
NTAPI
PhRecentListAddCommand(
    _In_ PPH_STRINGREF Command
    );

typedef _Function_class_(PH_ENUM_MRULIST_CALLBACK)
BOOLEAN NTAPI PH_ENUM_MRULIST_CALLBACK(
    _In_ PPH_STRINGREF Command,
    _In_opt_ PVOID Context
    );
typedef PH_ENUM_MRULIST_CALLBACK* PPH_ENUM_MRULIST_CALLBACK;

PHLIBAPI
VOID
NTAPI
PhEnumerateRecentList(
    _In_ PPH_ENUM_MRULIST_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhTerminateWindow(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN Force
    );

PHLIBAPI
NTSTATUS
NTAPI
PhOpenWindowProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HWND WindowHandle
    );

PHLIBAPI
ULONG_PTR
NTAPI
PhUserQueryWindow(
    _In_ HWND WindowHandle,
    _In_ WINDOWINFOCLASS WindowInfo
    );

FORCEINLINE
ULONG
PhQueryWindowRealProcess(
    _In_ HWND WindowHandle
    )
{
    return (ULONG)PhUserQueryWindow(
        WindowHandle,
        WindowRealProcess
        );
}

FORCEINLINE
BOOLEAN
PhWindowIsHung(
    _In_ HWND WindowHandle
    )
{
    return (BOOLEAN)(ULONG_PTR)PhUserQueryWindow(
        WindowHandle,
        WindowIsHung
        );
}

#ifndef DBT_DEVICEARRIVAL
#define DBT_DEVICEARRIVAL        0x8000  // system detected a new device
#define DBT_DEVICEREMOVECOMPLETE 0x8004  // device is gone
#define DBT_DEVTYP_VOLUME        0x00000002  // logical volume

typedef struct _DEV_BROADCAST_HDR
{
    ULONG dbch_size;
    ULONG dbch_devicetype;
    ULONG dbch_reserved;
} DEV_BROADCAST_HDR, *PDEV_BROADCAST_HDR;
#endif

// theme support (theme.c)

extern HFONT PhApplicationFont;
extern HFONT PhTreeWindowFont;
extern HFONT PhMonospaceFont;
extern HBRUSH PhThemeWindowBackgroundBrush;
extern BOOLEAN PhEnableThemeSupport;
extern BOOLEAN PhEnableThemeAcrylicSupport;
extern BOOLEAN PhEnableThemeAcrylicWindowSupport;
extern BOOLEAN PhEnableThemeNativeButtons;
extern BOOLEAN PhEnableThemeListviewBorder;
extern COLORREF PhThemeWindowForegroundColor;
extern COLORREF PhThemeWindowBackgroundColor;
extern COLORREF PhThemeWindowBackground2Color;
extern COLORREF PhThemeWindowHighlightColor;
extern COLORREF PhThemeWindowHighlight2Color;
extern COLORREF PhThemeWindowTextColor;

PHLIBAPI
VOID
NTAPI
PhInitializeWindowTheme(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN EnableThemeSupport
    );

PHLIBAPI
VOID
NTAPI
PhInitializeWindowThemeEx(
    _In_ HWND WindowHandle
    );

PHLIBAPI
VOID
NTAPI
PhReInitializeWindowTheme(
    _In_ HWND WindowHandle
    );

PHLIBAPI
VOID
NTAPI
PhInitializeThemeWindowFrame(
    _In_ HWND WindowHandle
    );

PHLIBAPI
HBRUSH
NTAPI
PhWindowThemeControlColor(
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ HWND ChildWindowHandle,
    _In_ LONG Type
    );

PHLIBAPI
BOOLEAN
NTAPI
PhThemeWindowDrawItem(
    _In_ HWND WindowHandle,
    _In_ PDRAWITEMSTRUCT DrawInfo
    );

PHLIBAPI
LRESULT
NTAPI
PhThemeWindowDrawButton(
    _In_ LPNMCUSTOMDRAW DrawInfo
    );

PHLIBAPI
BOOLEAN
NTAPI
PhThemeWindowMeasureItem(
    _In_ HWND WindowHandle,
    _In_ PMEASUREITEMSTRUCT DrawInfo
    );

PHLIBAPI
VOID
NTAPI
PhInitializeWindowThemeMainMenu(
    _In_ HMENU MenuHandle
    );

PHLIBAPI
LRESULT
CALLBACK
PhThemeWindowDrawRebar(
    _In_ LPNMCUSTOMDRAW DrawInfo
    );

PHLIBAPI
LRESULT
CALLBACK
PhThemeWindowDrawToolbar(
    _In_ LPNMTBCUSTOMDRAW DrawInfo
    );

// Font support

PHLIBAPI
HFONT
NTAPI
PhCreateFont(
    _In_opt_ PCWSTR Name,
    _In_ LONG Size,
    _In_ LONG Weight,
    _In_ LONG PitchAndFamily,
    _In_ LONG Dpi
    );

PHLIBAPI
HFONT
NTAPI
PhCreateCommonFont(
    _In_ LONG Size,
    _In_ LONG Weight,
    _In_opt_ HWND WindowHandle,
    _In_ LONG WindowDpi
    );

PHLIBAPI
HFONT
NTAPI
PhCreateIconTitleFont(
    _In_opt_ LONG WindowDpi
    );

PHLIBAPI
HFONT
NTAPI
PhCreateMessageFont(
    _In_opt_ LONG WindowDpi
    );

PHLIBAPI
HFONT
NTAPI
PhDuplicateFont(
    _In_ HFONT Font
    );

PHLIBAPI
HFONT
NTAPI
PhDuplicateFontWithNewWeight(
    _In_ HFONT Font,
    _In_ LONG NewWeight
    );

PHLIBAPI
HFONT
NTAPI
PhDuplicateFontWithNewHeight(
    _In_ HFONT Font,
    _In_ LONG NewHeight,
    _In_ LONG dpiValue
    );

VOID PhWindowThemeMainMenuBorder(
    _In_ HWND WindowHandle
    );

// directdraw.cpp

HICON PhGdiplusConvertBitmapToIcon(
    _In_ HBITMAP Bitmap,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ COLORREF Background
    );

HWND PhCreateBackgroundWindow(
    _In_ HWND ParentWindowHandle,
    _In_ BOOLEAN DesktopWindow
    );

HICON PhGdiplusConvertHBitmapToHIcon(
    _In_ HBITMAP BitmapHandle
    );

EXTERN_C_END

#endif
