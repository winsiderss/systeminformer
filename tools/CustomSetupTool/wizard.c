/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex
 *
 */

#include "setup.h"

typedef struct _SETUP_WIZARD_FONT_CONTEXT
{
    HFONT PageFont;
    HFONT TitleFont;
    HFONT LargeTitleFont;
    HFONT ButtonFont;
} SETUP_WIZARD_FONT_CONTEXT, *PSETUP_WIZARD_FONT_CONTEXT;

 // Win32 PropertySheet Control IDs
#define IDD_PROPSHEET_ID            1006  // ID of the propsheet dialog template in comctl32.dll
#define IDC_PROPSHEET_CANCEL        0x0002
#define IDC_PROPSHEET_APPLYNOW      0x3021
#define IDC_PROPSHEET_DLGFRAME      0x3022
#define IDC_PROPSHEET_BACK          0x3023
#define IDC_PROPSHEET_NEXT          0x3024
#define IDC_PROPSHEET_FINISH        0x3025
#define IDC_PROPSHEET_DIVIDER       0x3026
#define IDC_PROPSHEET_TOPDIVIDER    0x3027

#define SETUP_WIZARD_WELCOME_PAGE_INDEX 0
#define SETUP_WIZARD_SHORTCUTS_PAGE_INDEX 2
#define SETUP_WIZARD_UNINSTALL_PAGE_INDEX 3
#define SETUP_WIZARD_INSTALL_PAGE_INDEX 4
#define SETUP_WIZARD_COMPLETED_PAGE_INDEX 5
#define SETUP_WIZARD_ERROR_PAGE_INDEX 6

#define SETUP_DARKMODE_SUBCLASS_ID 1
#define SETUP_DWM_USE_IMMERSIVE_DARK_MODE 20
#define SETUP_DWM_BORDER_COLOR 34
#define SETUP_WINDOW_CONTEXT_LARGE_TITLE UCHAR_MAX

static BOOLEAN SetupWizardDarkMode = FALSE;
static HBRUSH SetupWizardBrushWindow = NULL;
static HBRUSH SetupWizardBrushControl = NULL;

static COLORREF SetupWizardColorWindow = RGB(32, 32, 32);
static COLORREF SetupWizardColorControl = RGB(45, 45, 45);
static COLORREF SetupWizardColorBorder = RGB(96, 96, 96);
static COLORREF SetupWizardColorText = RGB(242, 242, 242);
static COLORREF SetupWizardColorDisabledText = RGB(128, 128, 128);
static COLORREF SetupWizardColorHot = RGB(62, 62, 62);
static COLORREF SetupWizardColorPressed = RGB(72, 72, 72);
static COLORREF SetupWizardColorEditActiveBorder = RGB(91, 157, 217);
static COLORREF SetupWizardColorWindowInactiveBorder = RGB(90, 90, 90);

typedef HRESULT (WINAPI* SETUP_DWMSETWINDOWATTRIBUTE)(
    _In_ HWND WindowHandle,
    _In_ ULONG Attribute,
    _In_reads_bytes_(AttributeLength) PVOID AttributeData,
    _In_ ULONG AttributeLength
    );

typedef BOOL (WINAPI* SETUP_SHOULDAPPSUSEDARKMODE)(
    VOID
    );

typedef BOOL (WINAPI* SETUP_ALLOWDARKMODEFORWINDOW)(
    _In_ HWND WindowHandle,
    _In_ BOOL Allow
    );

static SETUP_DWMSETWINDOWATTRIBUTE SetupDwmSetWindowAttribute_I = NULL;
static SETUP_SHOULDAPPSUSEDARKMODE ShouldAppsUseDarkMode_I = NULL;
static SETUP_ALLOWDARKMODEFORWINDOW AllowDarkModeForWindow_I = NULL;

/**
 * Queries whether Windows is using dark app mode.
 *
 * \return TRUE if app dark mode is enabled, otherwise FALSE.
 */
BOOLEAN SetupQueryDarkMode(
    VOID
    )
{
    HMODULE uxthemeModule;

    if (!(uxthemeModule = PhGetDllHandle(L"uxtheme.dll")))
        uxthemeModule = PhLoadLibrary(L"uxtheme.dll");

    if (!uxthemeModule)
        return FALSE;

    ShouldAppsUseDarkMode_I = (SETUP_SHOULDAPPSUSEDARKMODE)PhGetProcedureAddress(uxthemeModule, NULL, 132);

    if (ShouldAppsUseDarkMode_I && ShouldAppsUseDarkMode_I())
        return TRUE;

    return FALSE;
}

/**
 * Enables dark non-client rendering for a window when supported.
 *
 * \param WindowHandle The window handle.
 */
VOID SetupAllowDarkModeForWindow(
    _In_ HWND WindowHandle
    )
{
    HMODULE uxthemeModule;
    HMODULE dwmapiModule;
    BOOL value;

    if (!SetupWizardDarkMode)
        return;

    if (uxthemeModule = PhGetDllHandle(L"uxtheme.dll"))
    {
        AllowDarkModeForWindow_I = (SETUP_ALLOWDARKMODEFORWINDOW)PhGetProcedureAddress(uxthemeModule, NULL, 133);

        if (AllowDarkModeForWindow_I)
            AllowDarkModeForWindow_I(WindowHandle, TRUE);
    }

    if (dwmapiModule = PhLoadLibrary(L"dwmapi.dll"))
    {
        SetupDwmSetWindowAttribute_I = (SETUP_DWMSETWINDOWATTRIBUTE)PhGetProcedureAddress(dwmapiModule, "DwmSetWindowAttribute", 0);

        if (SetupDwmSetWindowAttribute_I)
        {
            value = TRUE;
            SetupDwmSetWindowAttribute_I(WindowHandle, SETUP_DWM_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
        }
    }
}

/**
 * Updates the top-level wizard border color.
 *
 * \param WindowHandle The window handle.
 * \param Active TRUE to apply the active border color.
 */
VOID SetupUpdateWindowBorderColor(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN Active
    )
{
    COLORREF borderColor;

    if (!SetupWizardDarkMode || !SetupDwmSetWindowAttribute_I)
        return;

    borderColor = Active ? GetSysColor(COLOR_HOTLIGHT) : SetupWizardColorWindowInactiveBorder;
    SetupDwmSetWindowAttribute_I(WindowHandle, SETUP_DWM_BORDER_COLOR, &borderColor, sizeof(borderColor));
}

/**
 * Initializes local setup wizard dark mode resources.
 */
VOID SetupInitializeDarkMode(
    VOID
    )
{
    SetupWizardDarkMode = SetupQueryDarkMode();

    if (!SetupWizardDarkMode)
        return;

    if (!SetupWizardBrushWindow)
        SetupWizardBrushWindow = CreateSolidBrush(SetupWizardColorWindow);

    if (!SetupWizardBrushControl)
        SetupWizardBrushControl = CreateSolidBrush(SetupWizardColorControl);
}

/**
 * Frees local setup wizard dark mode resources.
 */
VOID SetupDeleteDarkMode(
    VOID
    )
{
    if (SetupWizardBrushWindow)
    {
        DeleteBrush(SetupWizardBrushWindow);
        SetupWizardBrushWindow = NULL;
    }

    if (SetupWizardBrushControl)
    {
        DeleteBrush(SetupWizardBrushControl);
        SetupWizardBrushControl = NULL;
    }
}

/**
 * Handles control color messages for local dark mode.
 *
 * \param wParam The control device context.
 * \param lParam The control window handle.
 * \return The brush used to paint the control background.
 */
INT_PTR SetupHandleDarkControlColor(
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    HWND controlHandle;
    WCHAR className[16];

    if (!SetupWizardDarkMode)
        return 0;

    controlHandle = (HWND)lParam;

    SetTextColor((HDC)wParam, SetupWizardColorText);
    SetBkColor((HDC)wParam, SetupWizardColorWindow);

    if (controlHandle && NT_SUCCESS(PhGetClassName(controlHandle, className, ARRAYSIZE(className), NULL)) && PhEqualStringZ(className, L"Edit", TRUE))
    {
        SetBkColor((HDC)wParam, SetupWizardColorControl);
        return (INT_PTR)SetupWizardBrushControl;
    }

    return (INT_PTR)SetupWizardBrushWindow;
}

/**
 * Paints a button control using local dark colors.
 *
 * \param WindowHandle The button window handle.
 * \param DcHandle The device context.
 */
VOID SetupPaintDarkButton(
    _In_ LPNMCUSTOMDRAW CustomDraw
    )
{
    HWND windowHandle = CustomDraw->hdr.hwndFrom;
    HDC hdc = CustomDraw->hdc;
    RECT textRect;
    WCHAR text[256];
    ULONG state;
    ULONG style;
    BOOLEAN pushed;
    BOOLEAN hot;
    BOOLEAN disabled;
    BOOLEAN focused;
    BOOLEAN checkbox;
    COLORREF oldTextColor;
    INT oldBkMode;
    HBRUSH brushHandle;

    state = (ULONG)SendMessage(windowHandle, BM_GETSTATE, 0, 0);
    style = PhGetWindowStyle(windowHandle);

    pushed = !!(state & BST_PUSHED);
    hot = !!(state & BST_HOT);
    disabled = !IsWindowEnabled(windowHandle);
    focused = (GetFocus() == windowHandle);
    checkbox = ((style & BS_TYPEMASK) == BS_AUTOCHECKBOX) || ((style & BS_TYPEMASK) == BS_CHECKBOX);

    if (checkbox)
    {
        LONG dpiValue;
        LONG checkSize;
        LONG checkPadding;
        LONG checkPenWidth;
        RECT checkRect;

        dpiValue = PhGetWindowDpi(windowHandle);
        checkSize = PhScaleToDisplay(13, dpiValue);
        checkPadding = PhScaleToDisplay(18, dpiValue);
        checkPenWidth = PhScaleToDisplay(2, dpiValue);

        FillRect(hdc, &CustomDraw->rc, SetupWizardBrushWindow);

        checkRect.left = CustomDraw->rc.left;
        checkRect.top = CustomDraw->rc.top + ((CustomDraw->rc.bottom - CustomDraw->rc.top) - checkSize) / 2;
        checkRect.right = CustomDraw->rc.left + checkSize;
        checkRect.bottom = checkRect.top + checkSize;

        FillRect(hdc, &checkRect, SetupWizardBrushControl);
        SetDCBrushColor(hdc, RGB(0, 130, 135));
        FrameRect(hdc, &checkRect, GetStockBrush(DC_BRUSH));

        if (SendMessage(windowHandle, BM_GETCHECK, 0, 0) == BST_CHECKED)
        {
            HPEN penHandle;
            HPEN oldPenHandle;

            penHandle = CreatePen(PS_SOLID, checkPenWidth, RGB(0, 120, 215));
            oldPenHandle = SelectPen(hdc, penHandle);
            MoveToEx(hdc, checkRect.left + PhScaleToDisplay(3, dpiValue), checkRect.top + PhScaleToDisplay(7, dpiValue), NULL);
            LineTo(hdc, checkRect.left + PhScaleToDisplay(6, dpiValue), checkRect.top + PhScaleToDisplay(10, dpiValue));
            LineTo(hdc, checkRect.left + PhScaleToDisplay(11, dpiValue), checkRect.top + PhScaleToDisplay(3, dpiValue));
            SelectPen(hdc, oldPenHandle);
            DeletePen(penHandle);
        }

        textRect = CustomDraw->rc;
        textRect.left += checkPadding;
    }
    else
    {
        if (CustomDraw->uItemState & CDIS_SELECTED)
        {
            brushHandle = CreateSolidBrush(SetupWizardColorPressed);
        }
        else
        {
            if (CustomDraw->uItemState & CDIS_HOT)
            {
                brushHandle = CreateSolidBrush(SetupWizardColorHot);
            }
            else
            {
                brushHandle = CreateSolidBrush(SetupWizardColorControl);
            }
        }

        FillRect(hdc, &CustomDraw->rc, brushHandle);
        DeleteBrush(brushHandle);

        SetDCBrushColor(hdc, RGB(55, 55, 55));
        FrameRect(hdc, &CustomDraw->rc, GetStockBrush(DC_BRUSH));

        if (focused)
        {
            RECT focusRect = CustomDraw->rc;
            PhInflateRect(&focusRect, -3, -3);
            //DrawFocusRect(DcHandle, &focusRect);
        }

        textRect = CustomDraw->rc;
    }

    GetWindowText(windowHandle, text, ARRAYSIZE(text));

    oldBkMode = SetBkMode(hdc, TRANSPARENT);
    oldTextColor = SetTextColor(hdc, disabled ? SetupWizardColorDisabledText : SetupWizardColorText);
    HFONT font = SelectFont(hdc, GetWindowFont(windowHandle));

    // Draw the button image list (if any) before the text so the text is painted
    // on top. Guard against a NULL himl: Button_GetImageList can succeed while
    // leaving himl NULL when no image list has been assigned to the button.
    BUTTON_IMAGELIST biml = { 0 };

    if (Button_GetImageList(windowHandle, &biml) && biml.himl)
    {
        LONG cx = 0;
        LONG cy = 0;

        if (ImageList_GetIconSize(biml.himl, &cx, &cy) && cx > 0 && cy > 0)
        {
            LONG imageX = textRect.left + biml.margin.left;
            LONG imageY = CustomDraw->rc.top + (CustomDraw->rc.bottom - CustomDraw->rc.top - cy) / 2;
            ULONG drawStyle = disabled ? (ILD_TRANSPARENT | ILD_BLEND50) : ILD_TRANSPARENT;

            ImageList_Draw(biml.himl, 0, hdc, imageX, imageY, drawStyle);

            // Shrink the text rect so the label does not overlap the image.
            textRect.left += biml.margin.left + cx + biml.margin.right;
        }
    }

    DrawText(
        hdc,
        text,
        -1,
        &textRect,
        (checkbox ? (DT_SINGLELINE | DT_VCENTER | DT_LEFT) : (DT_SINGLELINE | DT_VCENTER | DT_CENTER)) | DT_NOCLIP | DT_HIDEPREFIX
        );

    SelectFont(hdc, font);
    SetTextColor(hdc, oldTextColor);
    SetBkMode(hdc, oldBkMode);
}

/**
 * Subclass procedure for property sheet divider controls.
 */
LRESULT CALLBACK SetupDividerSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    WNDPROC previousProcedure;

    previousProcedure = (WNDPROC)PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);

    switch (uMsg)
    {
    case WM_NCDESTROY:
        PhSetWindowProcedure(WindowHandle, previousProcedure);
        PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT:
        {
            PAINTSTRUCT paint;
            RECT clientRect;
            COLORREF dividerColor;
            LONG dpi = PhGetWindowDpi(WindowHandle);

            BeginPaint(WindowHandle, &paint);
            GetClientRect(WindowHandle, &clientRect);
            //PhInflateRect(&clientRect, -1, -1);
            PhInflateRect(&clientRect, 1, 1);

            dividerColor = SetupWizardDarkMode ? RGB(55, 55, 55) : GetSysColor(COLOR_3DSHADOW);
            SetDCBrushColor(paint.hdc, dividerColor);
            FillRect(paint.hdc, &clientRect, GetStockBrush(DC_BRUSH));

            EndPaint(WindowHandle, &paint);
        }
        return 0;
    }

    return CallWindowProc(previousProcedure, WindowHandle, uMsg, wParam, lParam);
}

/**
 * Subclasses a property sheet divider control.
 *
 * \param ParentWindowHandle The property sheet window handle.
 * \param ControlId The divider control identifier.
 */
VOID SetupSubclassPropSheetDivider(
    _In_ HWND ParentWindowHandle,
    _In_ INT ControlId
    )
{
    HWND dividerHandle;

    if (dividerHandle = GetDlgItem(ParentWindowHandle, ControlId))
    {
        if (!PhGetWindowContext(dividerHandle, PH_WINDOW_CONTEXT_DEFAULT))
        {
            WNDPROC previousProcedure;

            previousProcedure = PhGetWindowProcedure(dividerHandle);
            PhSetWindowProcedure(dividerHandle, SetupDividerSubclassProc);
            PhSetWindowContext(
                dividerHandle,
                PH_WINDOW_CONTEXT_DEFAULT,
                previousProcedure
                );
        }

        InvalidateRect(dividerHandle, NULL, TRUE);
    }
}

VOID SetupRedrawPropSheetDivider(
    _In_ HWND ParentWindowHandle,
    _In_ INT ControlId
    )
{
    HWND dividerHandle;

    if (dividerHandle = GetDlgItem(ParentWindowHandle, ControlId))
    {
        InvalidateRect(dividerHandle, NULL, TRUE);
        UpdateWindow(dividerHandle);
    }
}

VOID SetupRedrawPropSheetDividers(
    _In_ HWND ParentWindowHandle
    )
{
    SetupRedrawPropSheetDivider(ParentWindowHandle, IDC_PROPSHEET_DIVIDER);
    SetupRedrawPropSheetDivider(ParentWindowHandle, IDC_PROPSHEET_TOPDIVIDER);

    RedrawWindow(
        ParentWindowHandle,
        NULL,
        NULL,
        RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW
        );
}

/**
 * Redraws the non-client border of a control.
 *
 * \param WindowHandle The control window handle.
 */
VOID SetupRedrawControlBorder(
    _In_ HWND WindowHandle
    )
{
    RedrawWindow(
        WindowHandle,
        NULL,
        NULL,
        RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW
        );
}

/**
 * Draws a dark control border without painting the client area.
 *
 * \param WindowHandle The control window handle.
 */
VOID SetupPaintControlBorder(
    _In_ HWND WindowHandle,
    _In_ WPARAM wParam
    )
{
    ULONG flags;
    HRGN updateRegion;
    HDC dcHandle;
    RECT windowRect;
    RECT clientRect;
    COLORREF borderColor;

    updateRegion = (HRGN)wParam;

    if (updateRegion == HRGN_FULL)
        updateRegion = NULL;

    flags = DCX_WINDOW | DCX_CACHE | DCX_USESTYLE;

    if (updateRegion)
        flags |= DCX_INTERSECTRGN | DCX_NODELETERGN;

    if (!(dcHandle = GetDCEx(WindowHandle, updateRegion, flags)))
        return;

    GetWindowRect(WindowHandle, &windowRect);
    GetClientRect(WindowHandle, &clientRect);

    PhOffsetRect(&windowRect, -windowRect.left, -windowRect.top);

    ExcludeClipRect(
        dcHandle,
        clientRect.left + 2,
        clientRect.top + 2,
        clientRect.right + 2,
        clientRect.bottom + 2
        );

    if (!IsWindowEnabled(WindowHandle))
        borderColor = RGB(128, 128, 128);
    else if (GetFocus() == WindowHandle)
        borderColor = SetupWizardColorEditActiveBorder;
    else
        borderColor = RGB(122, 122, 122);

    SetDCBrushColor(dcHandle, borderColor);
    FrameRect(dcHandle, &windowRect, GetStockBrush(DC_BRUSH));

    PhInflateRect(&windowRect, -1, -1);
    FrameRect(dcHandle, &windowRect, SetupWizardBrushWindow);

    ReleaseDC(WindowHandle, dcHandle);
}

static LRESULT CALLBACK SetupProgressSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT Message,
    _In_ WPARAM WParam,
    _In_ LPARAM LParam
    );

/**
 * Subclass procedure for dark control border painting.
 */
LRESULT CALLBACK SetupDarkControlBorderSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LRESULT result;
    WNDPROC previousProcedure;

    previousProcedure = (WNDPROC)PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);

    switch (uMsg)
    {
    case WM_NCDESTROY:
        PhSetWindowProcedure(WindowHandle, previousProcedure);
        PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
        break;
    case WM_NCPAINT:
        result = CallWindowProc(previousProcedure, WindowHandle, uMsg, wParam, lParam);
        SetupPaintControlBorder(WindowHandle, wParam);
        return result;
    case WM_NCACTIVATE:
        result = CallWindowProc(previousProcedure, WindowHandle, uMsg, wParam, lParam);
        SetupPaintControlBorder(WindowHandle, wParam);
        return result;
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    case WM_ENABLE:
        result = CallWindowProc(previousProcedure, WindowHandle, uMsg, wParam, lParam);
        SetupRedrawControlBorder(WindowHandle);
        return result;
    }

    return CallWindowProc(previousProcedure, WindowHandle, uMsg, wParam, lParam);
}

/**
 * Applies local dark-mode handling to a child window.
 *
 * \param WindowHandle The child window handle.
 */
VOID SetupApplyDarkModeToControl(
    _In_ HWND WindowHandle
    )
{
    WCHAR className[16];

    if (!SetupWizardDarkMode)
        return;

    if (NT_SUCCESS(PhGetClassName(WindowHandle, className, ARRAYSIZE(className), NULL)))
    {
        if (PhEqualStringZ(className, L"Edit", TRUE))
        {
            if (!PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT))
            {
                WNDPROC previousProcedure;

                previousProcedure = PhGetWindowProcedure(WindowHandle);
                PhSetWindowProcedure(WindowHandle, SetupDarkControlBorderSubclassProc);
                PhSetWindowContext(
                    WindowHandle,
                    PH_WINDOW_CONTEXT_DEFAULT,
                    previousProcedure
                    );
            }

            SetupRedrawControlBorder(WindowHandle);
        }
        else if (PhEqualStringZ(className, L"ListBox", TRUE))
        {
            PhSetControlTheme(WindowHandle, L"DarkMode_Explorer");
            if (!PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT))
            {
                WNDPROC previousProcedure;

                previousProcedure = PhGetWindowProcedure(WindowHandle);
                PhSetWindowProcedure(WindowHandle, SetupDarkControlBorderSubclassProc);
                PhSetWindowContext(
                    WindowHandle,
                    PH_WINDOW_CONTEXT_DEFAULT,
                    previousProcedure
                    );
            }

            SetupRedrawControlBorder(WindowHandle);
        }
        else if (PhEqualStringZ(className, L"msctls_progress32", TRUE))
        {
            SetWindowTheme(WindowHandle, L"", L"");
            if (!PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT))
            {
                WNDPROC previousProcedure = PhGetWindowProcedure(WindowHandle);
                PhSetWindowProcedure(WindowHandle, SetupProgressSubclassProc);
                PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, previousProcedure);
            }
            SendMessage(WindowHandle, PBM_SETBKCOLOR, 0, SetupWizardColorControl);
            SendMessage(WindowHandle, PBM_SETBARCOLOR, 0, RGB(0, 120, 215));
            InvalidateRect(WindowHandle, NULL, TRUE);
        }
    }
}

static VOID SetupPaintDarkProgress(
    _In_ HWND WindowHandle,
    _In_ HDC DcHandle,
    _In_ PRECT ProgressRect
    )
{
    RECT trackRect = *ProgressRect;
    RECT barRect = trackRect;
    PBRANGE range = { 0 };
    INT position = (INT)SendMessage(WindowHandle, PBM_GETPOS, 0, 0);
    LONG width;

    SetDCBrushColor(DcHandle, SetupWizardColorControl);
    FillRect(DcHandle, &trackRect, GetStockBrush(DC_BRUSH));

    SendMessage(WindowHandle, PBM_GETRANGE, 0, (LPARAM)&range);

    if (range.iHigh > range.iLow && position > range.iLow)
    {
        width = (trackRect.right - trackRect.left) * (position - range.iLow) / (range.iHigh - range.iLow);
        barRect.right = barRect.left + width;
        SetDCBrushColor(DcHandle, RGB(0, 120, 215));
        FillRect(DcHandle, &barRect, GetStockBrush(DC_BRUSH));
    }

    SetDCBrushColor(DcHandle, RGB(70, 70, 70));
    FrameRect(DcHandle, &trackRect, GetStockBrush(DC_BRUSH));
}

static LRESULT CALLBACK SetupProgressSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT Message,
    _In_ WPARAM WParam,
    _In_ LPARAM LParam
    )
{
    WNDPROC previousProcedure = (WNDPROC)PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);

    if (Message == WM_NCDESTROY)
    {
        PhSetWindowProcedure(WindowHandle, previousProcedure);
        PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }
    else if (Message == WM_ERASEBKGND)
    {
        return TRUE;
    }
    else if (Message == WM_PAINT)
    {
        PAINTSTRUCT paint;
        PH_BUFFERED_PAINT bufferedPaint;
        HDC bufferDc;
        RECT clientRect;

        BeginPaint(WindowHandle, &paint);
        GetClientRect(WindowHandle, &clientRect);
        if (PhBeginBufferedPaint(paint.hdc, &paint.rcPaint, BPBF_TOPDOWNDIB, NULL, &bufferedPaint, &bufferDc))
        {
            SetupPaintDarkProgress(WindowHandle, bufferDc, &clientRect);
            PhEndBufferedPaint(&bufferedPaint, TRUE);
        }
        else
        {
            SetupPaintDarkProgress(WindowHandle, paint.hdc, &clientRect);
        }
        EndPaint(WindowHandle, &paint);
        return 0;
    }

    return CallWindowProc(previousProcedure, WindowHandle, Message, WParam, LParam);
}

LRESULT SetupHandleControlCustomDraw(
    _In_ PVOID CustomDraw
    )
{
    LPNMCUSTOMDRAW customDraw = CustomDraw;

    if (!SetupWizardDarkMode)
        return CDRF_DODEFAULT;

    if (customDraw->hdr.code == NM_CUSTOMDRAW)
    {
        WCHAR className[16];

        if (NT_SUCCESS(PhGetClassName(customDraw->hdr.hwndFrom, className, ARRAYSIZE(className), NULL)))
        {
            if (PhEqualStringZ(className, L"Button", TRUE))
            {
                switch (customDraw->dwDrawStage)
                {
                case CDDS_PREPAINT:
                case CDDS_ITEMPREPAINT:
                    SetupPaintDarkButton(customDraw);
                    return CDRF_SKIPDEFAULT;
                }
            }
        }
    }

    return CDRF_DODEFAULT;
}

/**
 * Applies local dark-mode handling to all child controls.
 */
_Function_class_(PH_WINDOW_ENUM_CALLBACK)
BOOLEAN NTAPI SetupApplyDarkModeToChildControl(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    SetupApplyDarkModeToControl(WindowHandle);
    return TRUE;
}

/**
 * Applies local dark-mode rendering to a wizard page.
 *
 * \param WindowHandle The wizard page window handle.
 */
VOID SetupApplyDarkModeToPage(
    _In_ HWND WindowHandle
    )
{
    if (!SetupWizardDarkMode)
        return;

    SetupAllowDarkModeForWindow(WindowHandle);
    PhEnumChildWindows(WindowHandle, SetupApplyDarkModeToChildControl, NULL);
    InvalidateRect(WindowHandle, NULL, TRUE);
}

/**
 * Paints a dark background for a wizard window.
 *
 * \param WindowHandle The window handle.
 * \param DcHandle The device context.
 */
VOID SetupPaintDarkBackground(
    _In_ HWND WindowHandle,
    _In_ HDC DcHandle
    )
{
    RECT clipRect;

    UNREFERENCED_PARAMETER(WindowHandle);

    if (GetClipBox(DcHandle, &clipRect) <= NULLREGION)
        return;

    FillRect(DcHandle, &clipRect, SetupWizardBrushWindow);
}

/**
 * Creates a wizard font with an explicit face and logical height.
 *
 * \param WindowHandle The window handle used for DPI scaling.
 * \param FaceName The font face name.
 * \param Height The logical font height at 96 DPI.
 * \param Weight The font weight.
 * \return The created font handle, or NULL on failure.
 */
static HFONT SetupCreateWizardFont(
    _In_ HWND WindowHandle,
    _In_ LONG WindowDpi,
    _In_ PCWSTR FaceName,
    _In_ LONG Height,
    _In_ LONG Weight
    )
{
    return CreateFont(
        -PhMultiplyDivideSigned(Height, WindowDpi, 72),
        0,
        0,
        0,
        Weight,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH,
        FaceName
        );
}

typedef struct _SETUP_PROPSHEETCONTEXT
{
    BOOLEAN LayoutInitialized;
    WNDPROC DefaultWindowProc;
    PH_LAYOUT_MANAGER LayoutManager;
    PPH_LAYOUT_ITEM TabPageItem;
    SETUP_WIZARD_FONT_CONTEXT Fonts;
    LONG DialogDpi;
} PV_PROPSHEETCONTEXT, * PPV_PROPSHEETCONTEXT;

VOID SetupDeleteWizardFonts(
    _Inout_ PSETUP_WIZARD_FONT_CONTEXT FontContext
    )
{
    if (FontContext->PageFont)
    {
        DeleteFont(FontContext->PageFont);
        FontContext->PageFont = NULL;
    }

    if (FontContext->TitleFont)
    {
        DeleteFont(FontContext->TitleFont);
        FontContext->TitleFont = NULL;
    }

    if (FontContext->LargeTitleFont)
    {
        DeleteFont(FontContext->LargeTitleFont);
        FontContext->LargeTitleFont = NULL;
    }

    if (FontContext->ButtonFont)
    {
        DeleteFont(FontContext->ButtonFont);
        FontContext->ButtonFont = NULL;
    }
}

BOOLEAN SetupCreateWizardFonts(
    _In_ HWND WindowHandle,
    _In_ LONG WindowDpi,
    _Out_ PSETUP_WIZARD_FONT_CONTEXT FontContext
    )
{
    memset(FontContext, 0, sizeof(SETUP_WIZARD_FONT_CONTEXT));

    FontContext->PageFont = SetupCreateWizardFont(WindowHandle, WindowDpi, L"Segoe UI", -12, FW_NORMAL);
    FontContext->TitleFont = SetupCreateWizardFont(WindowHandle, WindowDpi, L"Tahoma", -11, FW_BOLD);
    FontContext->LargeTitleFont = SetupCreateWizardFont(WindowHandle, WindowDpi, L"Verdana", -16, FW_BOLD);
    FontContext->ButtonFont = SetupCreateWizardFont(WindowHandle, WindowDpi, L"MS Shell Dlg", -14, FW_NORMAL);

    if (
        FontContext->PageFont &&
        FontContext->TitleFont &&
        FontContext->LargeTitleFont &&
        FontContext->ButtonFont
        )
    {
        return TRUE;
    }

    SetupDeleteWizardFonts(FontContext);
    return FALSE;
}

/**
 * Sets the wizard page font for a child window.
 *
 * \param WindowHandle The child window handle.
 * \param lParam The font handle.
 * \return TRUE to continue enumeration.
 */
_Function_class_(PH_WINDOW_ENUM_CALLBACK)
BOOLEAN NTAPI SetupSetWizardChildFont(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    PSETUP_WIZARD_FONT_CONTEXT fontContext;
    WCHAR className[16];

    fontContext = (PSETUP_WIZARD_FONT_CONTEXT)Context;

    if (NT_SUCCESS(PhGetClassName(WindowHandle, className, ARRAYSIZE(className), NULL)) && PhEqualStringZ(className, L"Button", TRUE))
    {
        ULONG style = PhGetWindowStyle(WindowHandle);

        if ((style & BS_TYPEMASK) == BS_PUSHBUTTON || (style & BS_TYPEMASK) == BS_DEFPUSHBUTTON)
        {
            if (fontContext->ButtonFont)
            {
                SetWindowFont(WindowHandle, fontContext->ButtonFont, TRUE);
                return TRUE;
            }
        }
    }

    if (fontContext->PageFont)
        SetWindowFont(WindowHandle, fontContext->PageFont, TRUE);

    return TRUE;
}

/**
 * Applies the wizard fonts to a page.
 *
 * \param WindowHandle The wizard page window handle.
 * \param LargeTitle TRUE to use the welcome/completed page title font.
 */
VOID SetupApplyWizardPageFonts(
    _In_ HWND WindowHandle,
    _In_ PPV_PROPSHEETCONTEXT PropSheetContext,
    _In_ BOOLEAN LargeTitle
    )
{
    PSETUP_WIZARD_FONT_CONTEXT fontContext;

    SetupInitializeDarkMode();
    fontContext = &PropSheetContext->Fonts;

    if (fontContext->PageFont)
        SetWindowFont(WindowHandle, fontContext->PageFont, TRUE);

    if (fontContext->PageFont)
        PhEnumChildWindows(WindowHandle, SetupSetWizardChildFont, fontContext);

    if (LargeTitle)
    {
        if (fontContext->LargeTitleFont)
            SetWindowFont(GetDlgItem(WindowHandle, IDC_TITLE), fontContext->LargeTitleFont, TRUE);
    }
    else
    {
        if (fontContext->TitleFont)
            SetWindowFont(GetDlgItem(WindowHandle, IDC_TITLE), fontContext->TitleFont, TRUE);
    }
}

_Function_class_(PH_WINDOW_ENUM_CALLBACK)
BOOLEAN NTAPI SetupInvalidateWizardChildWindow(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    InvalidateRect(WindowHandle, NULL, TRUE);
    UpdateWindow(WindowHandle);

    return TRUE;
}

/**
 * Initializes the wizard page title font.
 *
 * \param WindowHandle The wizard page window handle.
 * \param LargeTitle TRUE to use the welcome/completed page title font.
 */
VOID SetupInitializeWizardTitleFont(
    _In_ PPH_SETUP_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ BOOLEAN LargeTitle
    )
{
    PPV_PROPSHEETCONTEXT propSheetContext;

    PhSetWindowContext(WindowHandle, SETUP_WINDOW_CONTEXT_LARGE_TITLE, (PVOID)(ULONG_PTR)LargeTitle);

    if (propSheetContext = PhGetWindowContext(Context->ParentWindowHandle, UCHAR_MAX))
    {
        SetupApplyWizardPageFonts(WindowHandle, propSheetContext, LargeTitle);
    }
}

VOID SetupDestroyWizardPage(
    _In_ HWND WindowHandle
    )
{
    PhRemoveWindowContext(WindowHandle, SETUP_WINDOW_CONTEXT_LARGE_TITLE);
    PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
}

/**
 * Sets fonts for property sheet command buttons.
 *
 * \param PropSheetContext The property sheet context.
 * \param ParentWindowHandle The property sheet window handle.
 */
VOID SetupSetPropSheetButtonFonts(
    _In_ PPV_PROPSHEETCONTEXT PropSheetContext,
    _In_ HWND ParentWindowHandle
    )
{
    HWND buttonHandle;

    if (!PropSheetContext->Fonts.ButtonFont)
        return;

    if (buttonHandle = GetDlgItem(ParentWindowHandle, IDC_PROPSHEET_BACK))
    {
        SetWindowFont(buttonHandle, PropSheetContext->Fonts.ButtonFont, TRUE);
        InvalidateRect(buttonHandle, NULL, TRUE);
        UpdateWindow(buttonHandle);
    }

    if (buttonHandle = GetDlgItem(ParentWindowHandle, IDC_PROPSHEET_NEXT))
    {
        SetWindowFont(buttonHandle, PropSheetContext->Fonts.ButtonFont, TRUE);
        InvalidateRect(buttonHandle, NULL, TRUE);
        UpdateWindow(buttonHandle);
    }

    if (buttonHandle = GetDlgItem(ParentWindowHandle, IDC_PROPSHEET_FINISH))
    {
        SetWindowFont(buttonHandle, PropSheetContext->Fonts.ButtonFont, TRUE);
        InvalidateRect(buttonHandle, NULL, TRUE);
        UpdateWindow(buttonHandle);
    }

    if (buttonHandle = GetDlgItem(ParentWindowHandle, IDC_PROPSHEET_CANCEL))
    {
        SetWindowFont(buttonHandle, PropSheetContext->Fonts.ButtonFont, TRUE);
        InvalidateRect(buttonHandle, NULL, TRUE);
        UpdateWindow(buttonHandle);
    }
}

VOID SetupApplyWizardFonts(
    _In_ HWND ParentWindowHandle,
    _In_ PPV_PROPSHEETCONTEXT PropSheetContext
    )
{
    ULONG i;

    SetupSetPropSheetButtonFonts(PropSheetContext, ParentWindowHandle);

    for (i = 0; i <= SETUP_WIZARD_ERROR_PAGE_INDEX; i++)
    {
        HWND pageWindowHandle;

        if (pageWindowHandle = PropSheet_IndexToHwnd(ParentWindowHandle, i))
        {
            SetupApplyWizardPageFonts(
                pageWindowHandle,
                PropSheetContext,
                !!PhGetWindowContext(pageWindowHandle, SETUP_WINDOW_CONTEXT_LARGE_TITLE)
                );
        }
    }
}

VOID SetupUpdateWizardFonts(
    _In_ HWND ParentWindowHandle,
    _Inout_ PPV_PROPSHEETCONTEXT PropSheetContext,
    _In_ LONG WindowDpi
    )
{
    SETUP_WIZARD_FONT_CONTEXT oldFonts;

    oldFonts = PropSheetContext->Fonts;
    PropSheetContext->DialogDpi = WindowDpi;

    if (!SetupCreateWizardFonts(ParentWindowHandle, WindowDpi, &PropSheetContext->Fonts))
    {
        PropSheetContext->Fonts = oldFonts;
        return;
    }

    SetupApplyWizardFonts(ParentWindowHandle, PropSheetContext);
    PhEnumChildWindows(ParentWindowHandle, SetupInvalidateWizardChildWindow, NULL);
    InvalidateRect(ParentWindowHandle, NULL, TRUE);
    UpdateWindow(ParentWindowHandle);
    SetupDeleteWizardFonts(&oldFonts);
}

/**
 * Loads the welcome page bitmap from the embedded PNG resource.
 *
 * \param WindowHandle The wizard page window handle.
 */
VOID SetupLoadWelcomeBitmap(
    _In_ HWND WindowHandle
    )
{
    LONG dpiValue;
    LONG bitmapSize;
    HBITMAP bitmapHandle;

    dpiValue = PhGetWindowDpi(WindowHandle);
    bitmapSize = PhMultiplyDivideSigned(144, dpiValue, USER_DEFAULT_SCREEN_DPI);

    bitmapHandle = PhLoadImageFormatFromResource(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDB_PNG1),
        L"PNG",
        PH_IMAGE_FORMAT_TYPE_PNG,
        bitmapSize,
        bitmapSize
        );

    if (bitmapHandle)
    {
        SendMessage(GetDlgItem(WindowHandle, IDC_SIDEBAR), STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmapHandle);
    }
}

/**
 * Shows or hides a wizard property sheet button.
 *
 * \param ParentWindowHandle The property sheet window handle.
 * \param ControlId The property sheet button control identifier.
 * \param ShowButton TRUE to show the button, FALSE to hide the button.
 */
VOID SetupShowWizardButton(
    _In_ HWND ParentWindowHandle,
    _In_ INT ControlId,
    _In_ BOOLEAN ShowButton
    )
{
    HWND buttonHandle;

    if (buttonHandle = GetDlgItem(ParentWindowHandle, ControlId))
    {
        ShowWindow(buttonHandle, ShowButton ? SW_SHOW : SW_HIDE);
    }
}

VOID SetupEnableWizardButton(
    _In_ HWND ParentWindowHandle,
    _In_ INT ControlId,
    _In_ BOOLEAN EnableButton
    )
{
    HWND buttonHandle;

    if (buttonHandle = GetDlgItem(ParentWindowHandle, ControlId))
    {
        EnableWindow(buttonHandle, EnableButton);
    }
}

VOID SetupSetWizardButtonText(
    _In_ HWND ParentWindowHandle,
    _In_ INT ControlId,
    _In_ PCWSTR Text
    )
{
    HWND buttonHandle;

    if (buttonHandle = GetDlgItem(ParentWindowHandle, ControlId))
    {
        SetWindowText(buttonHandle, Text);
    }
}

BOOLEAN SetupCancelWizard(
    _In_ HWND WindowHandle,
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    if (!Context->SetupProgressActive)
        return TRUE;

    if (PhShowMessage2(
        Context->ParentWindowHandle,
        TD_YES_BUTTON | TD_NO_BUTTON,
        TD_WARNING_ICON,
        L"Cancel Setup?",
        L"Setup is currently in progress. Cancelling now may leave System Informer partially installed or updated.\r\n\r\nAre you sure you want to cancel?"
        ) == IDYES)
    {
        return TRUE;
    }

    SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, TRUE);
    return FALSE;
}

/**
 * Updates the visible wizard buttons for the active page.
 *
 * \param WindowHandle The wizard page window handle.
 * \param Buttons The property sheet wizard button mask.
 *        Enables the Back, Next and Finish buttons.
 * \param ShowFinish TRUE to show the Finish button.
 * \param ShowCancel TRUE to show and enable the Cancel button.
 */
VOID SetupSetWizardButtons(
    _In_ HWND WindowHandle,
    _In_ DWORD Buttons,
    _In_ BOOLEAN ShowFinish,
    _In_ BOOLEAN ShowCancel
    )
{
    HWND parentWindowHandle;

    parentWindowHandle = GetParent(WindowHandle);
    PropSheet_SetWizButtons(parentWindowHandle, Buttons);
    SetupSetWizardButtonText(parentWindowHandle, IDC_PROPSHEET_BACK, L"< &Back");
    SetupSetWizardButtonText(parentWindowHandle, IDC_PROPSHEET_NEXT, L"&Next >");
    SetupEnableWizardButton(parentWindowHandle, IDC_PROPSHEET_CANCEL, ShowCancel);
    SetupShowWizardButton(parentWindowHandle, IDC_PROPSHEET_FINISH, ShowFinish);
    SetupShowWizardButton(parentWindowHandle, IDC_PROPSHEET_CANCEL, ShowCancel);

    SetupApplyDarkModeToControl(GetDlgItem(parentWindowHandle, IDC_PROPSHEET_BACK));
    SetupApplyDarkModeToControl(GetDlgItem(parentWindowHandle, IDC_PROPSHEET_NEXT));
    SetupApplyDarkModeToControl(GetDlgItem(parentWindowHandle, IDC_PROPSHEET_FINISH));
    SetupApplyDarkModeToControl(GetDlgItem(parentWindowHandle, IDC_PROPSHEET_CANCEL));
}

/**
 * Dialog procedure for the wizard welcome page.
 *
 * \param WindowHandle The dialog window handle.
 * \param uMsg The window message.
 * \param wParam Additional message information.
 * \param lParam Additional message information.
 * \return TRUE if the message was handled, otherwise FALSE.
 */
INT_PTR CALLBACK SetupWelcomePageDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_SETUP_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        LPPROPSHEETPAGE page = (LPPROPSHEETPAGE)lParam;
        context = (PPH_SETUP_CONTEXT)page->lParam;

        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->DialogHandle = WindowHandle;
            context->ParentWindowHandle = GetParent(WindowHandle);

            PhCenterWindow(context->ParentWindowHandle, NULL);

            ShowWindow(context->ParentWindowHandle, SW_SHOW);
            UpdateWindow(context->ParentWindowHandle);

            if (!SetForegroundWindow(context->ParentWindowHandle))
            {
                PhBringWindowToTop(context->ParentWindowHandle);
            }

            SetupInitializeWizardTitleFont(context, WindowHandle, TRUE);
            SetupLoadWelcomeBitmap(WindowHandle);
            SetupApplyDarkModeToPage(WindowHandle);

            SetupSetWizardButtons(WindowHandle, PSWIZB_NEXT, FALSE, TRUE);

            if (!PhGetOwnTokenAttributes().Elevated)
            {
                Button_SetElevationRequiredState(GetDlgItem(context->ParentWindowHandle, IDC_PROPSHEET_NEXT), TRUE);
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;
            LRESULT result;

            result = SetupHandleControlCustomDraw((LPNMCUSTOMDRAW)lParam);

            if (result != CDRF_DODEFAULT)
            {
                SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, result);
                return TRUE;
            }

            switch (header->code)
            {
            case PSN_SETACTIVE:
                SetupSetWizardButtons(WindowHandle, PSWIZB_NEXT, FALSE, TRUE);
                break;
            case PSN_QUERYCANCEL:
                return !SetupCancelWizard(WindowHandle, context);
            case PSN_WIZNEXT:
                {
#ifndef FORCE_TEST_UPDATE_LOCAL_INSTALL
                    if (PhGetOwnTokenAttributes().Elevated)
                    {
                        if (context->SetupMode == SetupCommandUninstall)
                        {
                            PropSheet_SetCurSel(context->ParentWindowHandle, NULL, SETUP_WIZARD_UNINSTALL_PAGE_INDEX);
                            SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, -1);
                            return TRUE;
                        }

                        if (!context->SetupIsLegacyUpdate && NT_SUCCESS(SetupLegacySetupInstalled()))
                        {
                            //SetupShowMessagePromptForLegacyVersion();
                        }
                    }
                    else
                    {
                        NTSTATUS status;
                        PPH_STRING applicationFileName;
                        PH_STRINGREF applicationCommandLine;

                        if (NT_SUCCESS(status = PhGetProcessCommandLineStringRef(&applicationCommandLine)))
                        {
                            if (applicationFileName = PhGetApplicationFileNameWin32())
                            {
                                status = PhShellExecuteEx(
                                    WindowHandle,
                                    PhGetString(applicationFileName),
                                    PhGetStringRefZ(&applicationCommandLine),
                                    NULL,
                                    SW_SHOW,
                                    PH_SHELL_EXECUTE_ADMIN,
                                    0,
                                    NULL
                                    );

                                if (NT_SUCCESS(status))
                                {
                                    PhExitApplication(status);
                                }
                                else if (status != STATUS_CANCELLED)
                                {
                                    PhShowStatus(WindowHandle, L"Unable to restart the application.", status, 0);
                                }

                                PhDereferenceObject(applicationFileName);
                            }
                        }

                        SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, -1);
                        return TRUE;
                    }
#endif
                }
                break;
            }
        }
        break;
    case WM_DPICHANGED:
    case WM_DPICHANGED_AFTERPARENT:
        SetupRedrawControlBorder(WindowHandle);
        break;
    case WM_ERASEBKGND:
        {
            if (SetupWizardDarkMode)
            {
                SetupPaintDarkBackground(WindowHandle, (HDC)wParam);
                return TRUE;
            }
        }
        break;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
        {
            if (SetupWizardDarkMode)
            {
                return SetupHandleDarkControlColor(wParam, lParam);
            }
        }
        break;
    case WM_NCDESTROY:
        SetupDestroyWizardPage(WindowHandle);
        break;
    }

    return FALSE;
}

/**
 * Callback that stops at the first directory entry, used to test whether a directory is empty.
 */
_Function_class_(PH_ENUM_DIRECTORY_FILE)
static BOOLEAN SetupIsDirectoryEmptyCallback(
    _In_ HANDLE RootDirectory,
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_ PVOID Context
    )
{
    PH_STRINGREF fileName;

    fileName.Buffer = Information->FileName;
    fileName.Length = Information->FileNameLength;

    if (PATH_IS_WIN32_RELATIVE_PREFIX(&fileName))
        return TRUE;

    *(PBOOLEAN)Context = FALSE;
    return FALSE;
}

/**
 * Determines whether a directory is empty.
 *
 * \param DirectoryPath The directory path.
 * \return TRUE if the directory has no entries, otherwise FALSE.
 */
static BOOLEAN SetupIsDirectoryEmpty(
    _In_ PPH_STRING DirectoryPath
    )
{
    HANDLE directoryHandle;
    BOOLEAN empty = TRUE;

    if (NT_SUCCESS(PhCreateFileWin32(
        &directoryHandle,
        PhGetString(DirectoryPath),
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        FILE_ATTRIBUTE_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
        )))
    {
        PhEnumDirectoryFile(directoryHandle, NULL, SetupIsDirectoryEmptyCallback, &empty);
        NtClose(directoryHandle);
    }

    return empty;
}

/**
 * Determines whether a path is a volume root or a protected shell folder
 * (or inside the Windows directory). Uninstall deletes the install directory
 * recursively, so these must never be used as the install path.
 *
 * \param Path A full path without a trailing separator.
 * \return TRUE if the path is protected, otherwise FALSE.
 */
static BOOLEAN SetupIsProtectedInstallPath(
    _In_ PPH_STRING Path
    )
{
    static CONST GUID protectedFolders[] =
    {
        { 0xF38BF404, 0x1D43, 0x42F2, { 0x93, 0x05, 0x67, 0xDE, 0x0B, 0x28, 0xFC, 0x23 } }, // Windows
        { 0x1AC14E77, 0x02E7, 0x4E5D, { 0xB7, 0x44, 0x2E, 0xB1, 0xAE, 0x51, 0x98, 0xB7 } }, // System
        { 0x905E63B6, 0xC1BF, 0x494E, { 0xB2, 0x9C, 0x65, 0xB7, 0x32, 0xD3, 0xD2, 0x1A } }, // ProgramFiles
        { 0x7C5A40EF, 0xA0FB, 0x4BFC, { 0x87, 0x4A, 0xC0, 0xF2, 0xE0, 0xB9, 0xFA, 0x8E } }, // ProgramFilesX86
        { 0xF7F1ED05, 0x9F6D, 0x47A2, { 0xAA, 0xAE, 0x29, 0xD3, 0x17, 0xC6, 0xF0, 0x66 } }, // ProgramFilesCommon
        { 0x62AB5D82, 0xFDC1, 0x4DC3, { 0xA9, 0xDD, 0x07, 0x0D, 0x1D, 0x49, 0x5D, 0x97 } }, // ProgramData
        { 0x0762D272, 0xC50A, 0x4BB0, { 0xA3, 0x82, 0x69, 0x7D, 0xCD, 0x72, 0x9B, 0x80 } }, // UserProfiles
        { 0x5E6C858F, 0x0E22, 0x4760, { 0x9A, 0xFE, 0xEA, 0x33, 0x17, 0xB6, 0x71, 0x73 } }, // Profile
        { 0xB4BFCC3A, 0xDB2C, 0x424C, { 0xB0, 0x29, 0x7F, 0xE9, 0x9A, 0x87, 0xC6, 0x41 } }, // Desktop
        { 0xFDD39AD0, 0x238F, 0x46AF, { 0xAD, 0xB4, 0x6C, 0x85, 0x48, 0x03, 0x69, 0xC7 } }, // Documents
        { 0x374DE290, 0x123F, 0x4565, { 0x91, 0x64, 0x39, 0xC4, 0x92, 0x5E, 0x46, 0x7B } }, // Downloads
    };
    BOOLEAN result = FALSE;

    // Volume root (e.g. "C:").
    if (Path->Length <= 2 * sizeof(WCHAR))
        return TRUE;

    for (ULONG i = 0; i < RTL_NUMBER_OF(protectedFolders) && !result; i++)
    {
        PPH_STRING folderPath;

        if (!(folderPath = PhGetKnownFolderPath(&protectedFolders[i], NULL)))
            continue;

        if (PhEndsWithStringRef(&folderPath->sr, &PhNtPathSeparatorString, FALSE))
            folderPath->Length -= sizeof(WCHAR);

        if (PhEqualStringRef(&Path->sr, &folderPath->sr, TRUE))
        {
            result = TRUE;
        }
        else if (i == 0) // Anything inside the Windows directory.
        {
            PH_STRINGREF remaining;

            if (PhStartsWithStringRef(&Path->sr, &folderPath->sr, TRUE))
            {
                remaining.Buffer = Path->Buffer + folderPath->Length / sizeof(WCHAR);
                remaining.Length = Path->Length - folderPath->Length;

                if (PhStartsWithStringRef(&remaining, &PhNtPathSeparatorString, FALSE))
                    result = TRUE;
            }
        }

        PhDereferenceObject(folderPath);
    }

    return result;
}

/**
 * Validates the install path chosen on the configuration page. Uninstall deletes
 * the install directory recursively, so the path must be absolute, must not be a
 * volume root or shell folder, and links are resolved and confirmed with the user.
 *
 * \param WindowHandle The configuration page window handle.
 * \param InputPath The path entered by the user.
 * \param InstallPath Receives the normalized install path.
 * \return TRUE if the path can be used, otherwise FALSE.
 */
static BOOLEAN SetupValidateInstallPath(
    _In_ HWND WindowHandle,
    _In_ PPH_STRING InputPath,
    _Out_ PPH_STRING *InstallPath
    )
{
    static CONST PH_STRINGREF whitespace = PH_STRINGREF_INIT(L" \t\"");
    PH_STRINGREF inputPath;
    RTL_PATH_TYPE pathType;
    PPH_STRING fullPath;
    PPH_STRING inputString;

    inputPath = InputPath->sr;
    PhTrimStringRef(&inputPath, &whitespace, 0);

    if (inputPath.Length == 0)
        return FALSE;

    inputString = PhCreateString2(&inputPath);
    pathType = RtlDetermineDosPathNameType_U(PhGetString(inputString));

    if (pathType != RtlPathTypeDriveAbsolute && pathType != RtlPathTypeUncAbsolute)
    {
        PhShowError2(WindowHandle, L"Invalid install location.", L"%s", L"Enter a full path, for example C:\\Program Files\\SystemInformer.");
        PhDereferenceObject(inputString);
        return FALSE;
    }

    if (!NT_SUCCESS(PhGetFullPath(PhGetString(inputString), &fullPath, NULL)))
    {
        PhShowError2(WindowHandle, L"Invalid install location.", L"%s", PhGetString(inputString));
        PhDereferenceObject(inputString);
        return FALSE;
    }

    PhDereferenceObject(inputString);

    if (PhEndsWithStringRef(&fullPath->sr, &PhNtPathSeparatorString, FALSE))
    {
        fullPath->Length -= sizeof(WCHAR);
        fullPath->Buffer[fullPath->Length / sizeof(WCHAR)] = UNICODE_NULL;
    }

    if (SetupIsProtectedInstallPath(fullPath))
    {
        PhShowError2(
            WindowHandle,
            L"This location can't be used.",
            L"%s\r\n\r\nUninstalling removes the entire install folder. Choose a dedicated folder, for example C:\\Program Files\\SystemInformer.",
            PhGetString(fullPath)
            );
        PhDereferenceObject(fullPath);
        return FALSE;
    }

    if (PhDoesDirectoryExistWin32(PhGetString(fullPath)))
    {
        PPH_STRING realPath;

        // Resolve junctions/symlinks; uninstall deletes the real target directory.
        if (!(realPath = SetupResolveInstallPath(fullPath)))
        {
            PhShowError2(WindowHandle, L"This location can't be used.", L"%s\r\n\r\nThe folder could not be resolved, or it points to the root of a drive.", PhGetString(fullPath));
            PhDereferenceObject(fullPath);
            return FALSE;
        }

        if (!PhEqualString(realPath, fullPath, TRUE))
        {
            if (SetupIsProtectedInstallPath(realPath))
            {
                PhShowError2(WindowHandle, L"This location can't be used.", L"%s\r\n\r\nThis folder is a link to %s, which is a protected folder.", PhGetString(fullPath), PhGetString(realPath));
                PhDereferenceObject(realPath);
                PhDereferenceObject(fullPath);
                return FALSE;
            }

            if (PhShowMessage2(
                WindowHandle,
                TD_YES_BUTTON | TD_NO_BUTTON,
                TD_WARNING_ICON,
                L"The install folder is a link.",
                L"%s\r\n\r\npoints to\r\n\r\n%s\r\n\r\nSystem Informer will be installed into the target folder, and uninstalling will delete the target folder and everything in it. Continue?",
                PhGetString(fullPath),
                PhGetString(realPath)
                ) != IDYES)
            {
                PhDereferenceObject(realPath);
                PhDereferenceObject(fullPath);
                return FALSE;
            }
        }

        // Existing non-empty folder that isn't a previous install: everything in it is removed on uninstall.
        if (!SetupIsDirectoryEmpty(realPath))
        {
            PPH_STRING exePath = PhConcatStringRefZ(&realPath->sr, L"\\SystemInformer.exe");
            BOOLEAN previousInstall = PhDoesFileExistWin32(PhGetString(exePath));

            PhDereferenceObject(exePath);

            if (!previousInstall && PhShowMessage2(
                WindowHandle,
                TD_YES_BUTTON | TD_NO_BUTTON,
                TD_WARNING_ICON,
                L"The install folder is not empty.",
                L"%s\r\n\r\nUninstalling System Informer will delete this folder and everything in it, including files that were not installed by setup. Continue?",
                PhGetString(realPath)
                ) != IDYES)
            {
                PhDereferenceObject(realPath);
                PhDereferenceObject(fullPath);
                return FALSE;
            }
        }

        PhDereferenceObject(realPath);
    }

    *InstallPath = fullPath;
    return TRUE;
}

/**
 * Dialog procedure for the wizard configuration page.
 *
 * \param WindowHandle The dialog window handle.
 * \param uMsg The window message.
 * \param wParam Additional message information.
 * \param lParam Additional message information.
 * \return TRUE if the message was handled, otherwise FALSE.
 */
INT_PTR CALLBACK SetupConfigPageDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_SETUP_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        LPPROPSHEETPAGE page = (LPPROPSHEETPAGE)lParam;
        context = (PPH_SETUP_CONTEXT)page->lParam;
        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);

        SetupInitializeWizardTitleFont(context, WindowHandle, FALSE);
        SetupApplyDarkModeToPage(WindowHandle);
        PhSetDialogItemText(WindowHandle, IDC_PATH, PhGetStringOrEmpty(context->SetupInstallPath));
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_BROWSE:
                {
                    PVOID fileDialog;

                    if (fileDialog = PhCreateOpenFileDialog())
                    {
                        PhSetFileDialogOptions(fileDialog, PH_FILEDIALOG_PICKFOLDERS);

                        if (PhShowFileDialog(WindowHandle, fileDialog))
                        {
                            PPH_STRING fileDialogFolderPath;

                            if (fileDialogFolderPath = PhGetFileDialogFileName(fileDialog))
                            {
                                PhTrimToNullTerminatorString(fileDialogFolderPath);
                                PhMoveReference(&context->SetupInstallPath, fileDialogFolderPath);
                                PhSetDialogItemText(WindowHandle, IDC_PATH, PhGetStringOrEmpty(context->SetupInstallPath));
                            }
                        }

                        PhFreeFileDialog(fileDialog);
                    }
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;
            LRESULT result;

            result = SetupHandleControlCustomDraw((LPNMCUSTOMDRAW)lParam);

            if (result != CDRF_DODEFAULT)
            {
                SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, result);
                return TRUE;
            }

            switch (header->code)
            {
            case PSN_SETACTIVE:
                SetupSetWizardButtons(WindowHandle, PSWIZB_BACK | PSWIZB_NEXT, FALSE, TRUE);
                break;
            case PSN_QUERYCANCEL:
                return !SetupCancelWizard(WindowHandle, context);
            case PSN_WIZNEXT:
                {
                    PPH_STRING inputPath;
                    PPH_STRING installPath;
                    BOOLEAN valid;

                    valid = FALSE;

                    if (inputPath = PhGetWindowText(GetDlgItem(WindowHandle, IDC_PATH)))
                    {
                        valid = SetupValidateInstallPath(WindowHandle, inputPath, &installPath);
                        PhDereferenceObject(inputPath);
                    }

                    if (!valid)
                    {
                        SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, -1);
                        return TRUE;
                    }

                    //PhMoveReference(&context->SetupInstallPath, installPath);
                    PhSetDialogItemText(WindowHandle, IDC_PATH, PhGetString(context->SetupInstallPath));

                    if (context->SetupMode == SetupCommandInstall)
                    {
                        PropSheet_SetCurSel(context->ParentWindowHandle, NULL, SETUP_WIZARD_SHORTCUTS_PAGE_INDEX);
                        SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, -1);
                        return TRUE;
                    }
                }
                break;
            }
        }
        break;
    case WM_DPICHANGED:
    case WM_DPICHANGED_AFTERPARENT:
        SetupRedrawControlBorder(WindowHandle);
        break;
    case WM_ERASEBKGND:
        if (SetupWizardDarkMode)
        {
            SetupPaintDarkBackground(WindowHandle, (HDC)wParam);
            return TRUE;
        }
        break;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
        if (SetupWizardDarkMode)
            return SetupHandleDarkControlColor(wParam, lParam);
        break;
    case WM_NCDESTROY:
        SetupDestroyWizardPage(WindowHandle);
        break;
    }

    return FALSE;
}

_Function_class_(PH_ENUM_DIRECTORY_FILE)
static BOOLEAN CALLBACK SetupEnumStartMenuFoldersCallback(
    _In_ HANDLE RootDirectory,
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_ PVOID Context
    )
{
    PH_STRINGREF folderName;
    PPH_STRING folderNameString;

    folderName.Buffer = Information->FileName;
    folderName.Length = Information->FileNameLength;

    if (!FlagOn(Information->FileAttributes, FILE_ATTRIBUTE_DIRECTORY) ||
        PhEqualStringRef2(&folderName, L".", FALSE) ||
        PhEqualStringRef2(&folderName, L"..", FALSE))
    {
        return TRUE;
    }

    folderNameString = PhCreateString2(&folderName);
    ListBox_AddString((HWND)Context, PhGetString(folderNameString));
    PhDereferenceObject(folderNameString);

    return TRUE;
}

static VOID SetupPopulateStartMenuFolders(
    _In_ HWND ListBoxHandle
    )
{
    PPH_STRING programsPath;
    HANDLE directoryHandle;

    if (!(programsPath = PhGetKnownFolderPath(&FOLDERID_CommonPrograms, NULL)))
        return;

    if (NT_SUCCESS(PhCreateFileWin32(
        &directoryHandle,
        PhGetString(programsPath),
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        FILE_ATTRIBUTE_DIRECTORY,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        PhEnumDirectoryFile(directoryHandle, NULL, SetupEnumStartMenuFoldersCallback, ListBoxHandle);
        NtClose(directoryHandle);
    }

    PhDereferenceObject(programsPath);
}

INT_PTR CALLBACK SetupShortcutsPageDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_SETUP_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        LPPROPSHEETPAGE page = (LPPROPSHEETPAGE)lParam;
        context = (PPH_SETUP_CONTEXT)page->lParam;
        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);

        SetupInitializeWizardTitleFont(context, WindowHandle, FALSE);
        SetupApplyDarkModeToPage(WindowHandle);
        PhSetDialogItemText(WindowHandle, IDC_STARTMENUFOLDER, PhGetStringOrEmpty(context->SetupStartMenuFolderName));
        SendDlgItemMessage(
            WindowHandle,
            IDC_STARTMENUFOLDER,
            EM_SETCUEBANNER,
            TRUE,
            (LPARAM)L"No folder will be created"
            );
        SetupPopulateStartMenuFolders(GetDlgItem(WindowHandle, IDC_STARTMENUFOLDERLIST));
        CheckDlgButton(
            WindowHandle,
            IDC_NOSTARTMENUSHORTCUTS,
            context->SetupCreateStartMenuShortcuts ? BST_UNCHECKED : BST_CHECKED
            );
        CheckDlgButton(
            WindowHandle,
            IDC_DESKTOPSHORTCUT,
            context->SetupCreateDesktopShortcut ? BST_CHECKED : BST_UNCHECKED
            );
        EnableWindow(GetDlgItem(WindowHandle, IDC_STARTMENUFOLDER), context->SetupCreateStartMenuShortcuts);
        EnableWindow(GetDlgItem(WindowHandle, IDC_STARTMENUFOLDERLIST), context->SetupCreateStartMenuShortcuts);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_STARTMENUFOLDERLIST:
            if (HIWORD(wParam) == LBN_SELCHANGE)
            {
                LONG index = ListBox_GetCurSel((HWND)lParam);

                if (index != LB_ERR)
                {
                    PPH_STRING folderName;

                    if (folderName = PhGetListBoxString((HWND)lParam, index))
                    {
                        PhSetDialogItemText(WindowHandle, IDC_STARTMENUFOLDER, folderName->Buffer);
                        PhDereferenceObject(folderName);
                    }
                }
            }
            break;
        case IDC_NOSTARTMENUSHORTCUTS:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                BOOLEAN enable = Button_GetCheck(GetDlgItem(WindowHandle, IDC_NOSTARTMENUSHORTCUTS)) != BST_CHECKED;
                EnableWindow(GetDlgItem(WindowHandle, IDC_STARTMENUFOLDER), enable);
                EnableWindow(GetDlgItem(WindowHandle, IDC_STARTMENUFOLDERLIST), enable);
            }
            break;
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;
            LRESULT result = SetupHandleControlCustomDraw((LPNMCUSTOMDRAW)lParam);

            if (result != CDRF_DODEFAULT)
            {
                SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, result);
                return TRUE;
            }

            switch (header->code)
            {
            case PSN_SETACTIVE:
                SetupSetWizardButtons(WindowHandle, PSWIZB_BACK | PSWIZB_NEXT, FALSE, TRUE);
                break;
            case PSN_QUERYCANCEL:
                return !SetupCancelWizard(WindowHandle, context);
            case PSN_WIZNEXT:
                {
                    WCHAR folderName[MAX_PATH];
                    PH_STRINGREF folderNameSr;
                    ULONG returnLength;

                    context->SetupCreateStartMenuShortcuts = Button_GetCheck(GetDlgItem(WindowHandle, IDC_NOSTARTMENUSHORTCUTS)) != BST_CHECKED;
                    context->SetupCreateDesktopShortcut = Button_GetCheck(GetDlgItem(WindowHandle, IDC_DESKTOPSHORTCUT)) == BST_CHECKED;

                    if (!NT_SUCCESS(PhGetWindowTextToBuffer(
                        GetDlgItem(WindowHandle, IDC_STARTMENUFOLDER),
                        0,
                        folderName,
                        RTL_NUMBER_OF(folderName),
                        &returnLength
                        )))
                    {
                        return FALSE;
                    }

                    folderNameSr.Buffer = folderName;
                    folderNameSr.Length = returnLength * sizeof(WCHAR);

                    if (context->SetupCreateStartMenuShortcuts &&
                        (PhFindCharInStringRef(&folderNameSr, L'\\', FALSE) != SIZE_MAX ||
                        PhFindCharInStringRef(&folderNameSr, L'/', FALSE) != SIZE_MAX ||
                        PhFindCharInStringRef(&folderNameSr, L':', FALSE) != SIZE_MAX ||
                        PhFindCharInStringRef(&folderNameSr, L'*', FALSE) != SIZE_MAX ||
                        PhFindCharInStringRef(&folderNameSr, L'?', FALSE) != SIZE_MAX ||
                        PhFindCharInStringRef(&folderNameSr, L'\"', FALSE) != SIZE_MAX ||
                        PhFindCharInStringRef(&folderNameSr, L'<', FALSE) != SIZE_MAX ||
                        PhFindCharInStringRef(&folderNameSr, L'>', FALSE) != SIZE_MAX ||
                        PhFindCharInStringRef(&folderNameSr, L'|', FALSE) != SIZE_MAX))
                    {
                        PhShowMessage2(
                            WindowHandle,
                            TD_OK_BUTTON,
                            TD_WARNING_ICON,
                            L"Invalid Start Menu folder",
                            L"Enter a valid Start Menu folder name."
                            );
                        SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, -1);
                        return TRUE;
                    }

                    PhMoveReference(&context->SetupStartMenuFolderName, PhCreateString(folderName));
                    PropSheet_SetCurSel(context->ParentWindowHandle, NULL, SETUP_WIZARD_INSTALL_PAGE_INDEX);
                    SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, -1);
                    return TRUE;
                }
            }
        }
        break;
    case WM_ERASEBKGND:
        if (SetupWizardDarkMode)
        {
            SetupPaintDarkBackground(WindowHandle, (HDC)wParam);
            return TRUE;
        }
        break;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
        if (SetupWizardDarkMode)
            return SetupHandleDarkControlColor(wParam, lParam);
        break;
    case WM_NCDESTROY:
        SetupDestroyWizardPage(WindowHandle);
        break;
    }

    return FALSE;
}

/**
 * Dialog procedure for the wizard uninstall page.
 *
 * \param WindowHandle The dialog window handle.
 * \param uMsg The window message.
 * \param wParam Additional message information.
 * \param lParam Additional message information.
 * \return TRUE if the message was handled, otherwise FALSE.
 */
INT_PTR CALLBACK SetupUninstallPageDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_SETUP_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        LPPROPSHEETPAGE page = (LPPROPSHEETPAGE)lParam;
        context = (PPH_SETUP_CONTEXT)page->lParam;
        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);

        SetupInitializeWizardTitleFont(context, WindowHandle, FALSE);
        SetupApplyDarkModeToPage(WindowHandle);

        if (PhGetOwnTokenAttributes().Elevated)
        {
            ShowWindow(GetDlgItem(WindowHandle, IDC_REMOVESETTINGS), SW_SHOW);
            EnableWindow(GetDlgItem(WindowHandle, IDC_REMOVESETTINGS), TRUE);
        }
        else
        {
            ShowWindow(GetDlgItem(WindowHandle, IDC_REMOVESETTINGS), SW_HIDE);
            EnableWindow(GetDlgItem(WindowHandle, IDC_REMOVESETTINGS), FALSE);
        }
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;
            LRESULT result;

            result = SetupHandleControlCustomDraw((LPNMCUSTOMDRAW)lParam);

            if (result != CDRF_DODEFAULT)
            {
                SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, result);
                return TRUE;
            }

            switch (header->code)
            {
            case PSN_SETACTIVE:
                {
                    SetupSetWizardButtons(WindowHandle, PSWIZB_BACK | PSWIZB_NEXT, FALSE, TRUE);
                    SetupSetWizardButtonText(context->ParentWindowHandle, IDC_PROPSHEET_NEXT, L"&Uninstall");

                    if (!PhGetOwnTokenAttributes().Elevated)
                    {
                        Button_SetElevationRequiredState(GetDlgItem(context->ParentWindowHandle, IDC_PROPSHEET_NEXT), TRUE);
                    }
                }
                break;
            case PSN_QUERYCANCEL:
                return !SetupCancelWizard(WindowHandle, context);
            case PSN_WIZBACK:
                {
                    PropSheet_SetCurSel(context->ParentWindowHandle, NULL, SETUP_WIZARD_WELCOME_PAGE_INDEX);
                    SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, -1);
                    return TRUE;
                }
                break;
            case PSN_WIZNEXT:
                {
#ifndef FORCE_TEST_UPDATE_LOCAL_INSTALL
                    if (!PhGetOwnTokenAttributes().Elevated)
                    {
                        NTSTATUS status;
                        PPH_STRING applicationFileName;
                        PH_STRINGREF applicationCommandLine;

                        if (NT_SUCCESS(status = PhGetProcessCommandLineStringRef(&applicationCommandLine)))
                        {
                            if (applicationFileName = PhGetApplicationFileNameWin32())
                            {
                                status = PhShellExecuteEx(
                                    NULL,
                                    PhGetString(applicationFileName),
                                    PhGetStringRefZ(&applicationCommandLine),
                                    NULL,
                                    SW_SHOW,
                                    PH_SHELL_EXECUTE_ADMIN,
                                    0,
                                    NULL
                                    );

                                if (NT_SUCCESS(status))
                                {
                                    PhExitApplication(status);
                                }
                                else
                                {
                                    PhShowStatus(NULL, L"Unable to restart the application.", status, 0);
                                }

                                PhDereferenceObject(applicationFileName);
                            }
                        }

                        SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, -1);
                        return TRUE;
                    }
#endif
                    context->SetupRemoveAppData = (Button_GetCheck(GetDlgItem(WindowHandle, IDC_REMOVESETTINGS)) == BST_CHECKED);
                }
                break;
            }
        }
        break;
    case WM_DPICHANGED:
    case WM_DPICHANGED_AFTERPARENT:
        SetupRedrawControlBorder(WindowHandle);
        break;
    case WM_ERASEBKGND:
        if (SetupWizardDarkMode)
        {
            SetupPaintDarkBackground(WindowHandle, (HDC)wParam);
            return TRUE;
        }
        break;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
        if (SetupWizardDarkMode)
            return SetupHandleDarkControlColor(wParam, lParam);
        break;
    case WM_NCDESTROY:
        SetupDestroyWizardPage(WindowHandle);
        break;
    }

    return FALSE;
}

/**
 * Starts the wizard progress page worker.
 *
 * \param WindowHandle The wizard page window handle.
 * \param Context The setup context.
 */
VOID SetupStartWizardProgress(
    _In_ HWND WindowHandle,
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    if (!Context->ParentWindowHandle)
        Context->ParentWindowHandle = GetParent(WindowHandle);

    SetupSetWizardButtons(WindowHandle, 0, FALSE, TRUE);

    Context->DialogHandle = WindowHandle;

    if (Context->SetupProgressActive)
        return;

    Context->SetupProgressActive = TRUE;

    switch (Context->SetupMode)
    {
    case SetupCommandUninstall:
        {
            PhCreateThread2(SetupUninstallBuild, Context);
        }
        break;
    case SetupCommandUpdate:
        {
            PhCreateThread2(SetupUpdateBuild, Context);
        }
        break;
    case SetupCommandInstall:
    default:
        {
            PhCreateThread2(SetupProgressThread, Context);
        }
        break;
    }
}

/**
 * Dialog procedure for the wizard installation page.
 *
 * \param WindowHandle The dialog window handle.
 * \param uMsg The window message.
 * \param wParam Additional message information.
 * \param lParam Additional message information.
 * \return TRUE if the message was handled, otherwise FALSE.
 */
INT_PTR CALLBACK SetupInstallPageDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_SETUP_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        LPPROPSHEETPAGE page = (LPPROPSHEETPAGE)lParam;
        context = (PPH_SETUP_CONTEXT)page->lParam;

        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);

        context->ParentWindowHandle = GetParent(WindowHandle);

        SetupInitializeWizardTitleFont(context, WindowHandle, FALSE);
        SetupApplyDarkModeToPage(WindowHandle);
        SendMessage(GetDlgItem(WindowHandle, IDC_PROGRESS), PBM_SETMARQUEE, TRUE, 0);

        if (context->SetupMode == SetupCommandUninstall)
        {
            PhSetDialogItemText(WindowHandle, IDC_TITLE, L"Uninstalling");
            PhSetDialogItemText(WindowHandle, IDC_SUBTITLE, L"Please wait while Setup removes System Informer from your computer.");
        }

        if (context->SetupMode == SetupCommandUpdate)
        {
            PostMessage(WindowHandle, SETUP_SHOWUPDATE, 0, 0);
        }
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;
            LRESULT result;

            result = SetupHandleControlCustomDraw((LPNMCUSTOMDRAW)lParam);

            if (result != CDRF_DODEFAULT)
            {
                SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, result);
                return TRUE;
            }

            switch (header->code)
            {
            case PSN_SETACTIVE:
                {
                    SetupStartWizardProgress(WindowHandle, context);
                }
                break;
            case PSN_QUERYCANCEL:
                return !SetupCancelWizard(WindowHandle, context);
            }
        }
        break;
    case WM_DPICHANGED:
    case WM_DPICHANGED_AFTERPARENT:
        SetupRedrawControlBorder(WindowHandle);
        break;
    case SETUP_SHOWFINAL:
        {
            PropSheet_SetCurSel(context->ParentWindowHandle, NULL, SETUP_WIZARD_COMPLETED_PAGE_INDEX);
        }
        break;
    case SETUP_SHOWUPDATEFINAL:
        {
            PropSheet_SetCurSel(context->ParentWindowHandle, NULL, SETUP_WIZARD_COMPLETED_PAGE_INDEX);
        }
        break;
    case SETUP_SHOWUNINSTALLFINAL:
        {
            PropSheet_PressButton(context->ParentWindowHandle, PSBTN_FINISH);
        }
        break;
    case SETUP_SHOWUPDATE:
        {
            SetupStartWizardProgress(WindowHandle, context);
        }
        break;
    case SETUP_SHOWERROR:
    case SETUP_SHOWUPDATEERROR:
    case SETUP_SHOWUNINSTALLERROR:
        {
            PropSheet_SetCurSel(context->ParentWindowHandle, NULL, SETUP_WIZARD_ERROR_PAGE_INDEX);
        }
        break;
    case WM_ERASEBKGND:
        if (SetupWizardDarkMode)
        {
            SetupPaintDarkBackground(WindowHandle, (HDC)wParam);
            return TRUE;
        }
        break;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
        {
            if (SetupWizardDarkMode)
            {
                return SetupHandleDarkControlColor(wParam, lParam);
            }
        }
        break;
    case WM_NCDESTROY:
        SetupDestroyWizardPage(WindowHandle);
        break;
    }

    return FALSE;
}

/**
 * Dialog procedure for the wizard completed page.
 *
 * \param WindowHandle The dialog window handle.
 * \param uMsg The window message.
 * \param wParam Additional message information.
 * \param lParam Additional message information.
 * \return TRUE if the message was handled, otherwise FALSE.
 */
INT_PTR CALLBACK SetupCompletedPageDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_SETUP_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        LPPROPSHEETPAGE page = (LPPROPSHEETPAGE)lParam;
        context = (PPH_SETUP_CONTEXT)page->lParam;
        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);

        SetupInitializeWizardTitleFont(context, WindowHandle, TRUE);
        SetupApplyDarkModeToPage(WindowHandle);
        CheckDlgButton(WindowHandle, IDC_STARTAPP, BST_CHECKED);
        SetupLoadWelcomeBitmap(WindowHandle);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;
            LRESULT result;

            result = SetupHandleControlCustomDraw((LPNMCUSTOMDRAW)lParam);

            if (result != CDRF_DODEFAULT)
            {
                SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, result);
                return TRUE;
            }

            switch (header->code)
            {
            case PSN_SETACTIVE:
                SetupSetWizardButtons(WindowHandle, PSWIZB_FINISH, TRUE, FALSE);
                break;
            case PSN_QUERYCANCEL:
                return !SetupCancelWizard(WindowHandle, context);
            case PSN_WIZFINISH:
                {
                    if (Button_GetCheck(GetDlgItem(WindowHandle, IDC_STARTAPP)) == BST_CHECKED)
                    {
                        SetupExecuteApplication(context);
                    }
                }
                break;
            }
        }
        break;
    case WM_DPICHANGED:
    case WM_DPICHANGED_AFTERPARENT:
        SetupRedrawControlBorder(WindowHandle);
        break;
    case WM_ERASEBKGND:
        {
            if (SetupWizardDarkMode)
            {
                SetupPaintDarkBackground(WindowHandle, (HDC)wParam);
                return TRUE;
            }
        }
        break;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
        {
            if (SetupWizardDarkMode)
            {
                return SetupHandleDarkControlColor(wParam, lParam);
            }
        }
        break;
    case WM_NCDESTROY:
        SetupDestroyWizardPage(WindowHandle);
        break;
    }

    return FALSE;
}

/**
 * Dialog procedure for the wizard error page.
 *
 * \param WindowHandle The dialog window handle.
 * \param uMsg The window message.
 * \param wParam Additional message information.
 * \param lParam Additional message information.
 * \return TRUE if the message was handled, otherwise FALSE.
 */
INT_PTR CALLBACK SetupErrorPageDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_SETUP_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        LPPROPSHEETPAGE page = (LPPROPSHEETPAGE)lParam;
        context = (PPH_SETUP_CONTEXT)page->lParam;
        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);

        SetupInitializeWizardTitleFont(context, WindowHandle, TRUE);
        SetupApplyDarkModeToPage(WindowHandle);
        SetupLoadWelcomeBitmap(WindowHandle);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;
            LRESULT result;

            result = SetupHandleControlCustomDraw((LPNMCUSTOMDRAW)lParam);

            if (result != CDRF_DODEFAULT)
            {
                SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, result);
                return TRUE;
            }

            switch (header->code)
            {
            case PSN_SETACTIVE:
                {
                    PPH_STRING statusMessage;

                    if (statusMessage = PhGetStatusMessage(context->LastStatus, 0))
                    {
                        PhSetDialogItemText(WindowHandle, IDC_STATUS, PhGetString(statusMessage));
                    }
                    else
                    {
                        PhSetDialogItemText(WindowHandle, IDC_STATUS, L"An unknown error occurred.");
                    }

                    SetupSetWizardButtons(WindowHandle, PSWIZB_BACK, FALSE, TRUE);
                    SetupSetWizardButtonText(context->ParentWindowHandle, IDC_PROPSHEET_BACK, L"Retry");
                }
                break;
            case PSN_QUERYCANCEL:
                return !SetupCancelWizard(WindowHandle, context);
            case PSN_WIZBACK:
                if (context->SetupMode == SetupCommandUninstall)
                    PropSheet_SetCurSel(context->ParentWindowHandle, NULL, SETUP_WIZARD_UNINSTALL_PAGE_INDEX);
                else
                    PropSheet_SetCurSel(context->ParentWindowHandle, NULL, 0);
                SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, -1);
                return TRUE;
            }
        }
        break;
    case WM_DPICHANGED:
    case WM_DPICHANGED_AFTERPARENT:
        SetupRedrawControlBorder(WindowHandle);
        break;
    case WM_ERASEBKGND:
        if (SetupWizardDarkMode)
        {
            SetupPaintDarkBackground(WindowHandle, (HDC)wParam);
            return TRUE;
        }
        break;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
        if (SetupWizardDarkMode)
            return SetupHandleDarkControlColor(wParam, lParam);
        break;
    case WM_NCDESTROY:
        SetupDestroyWizardPage(WindowHandle);
        break;
    }

    return FALSE;
}

/**
 * Subclass procedure for the wizard property sheet.
 *
 * \param WindowHandle The property sheet window handle.
 * \param uMsg The window message.
 * \param wParam Additional message information.
 * \param lParam Additional message information.
 * \return The message result.
 */
LRESULT CALLBACK PvpPropSheetWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    PPV_PROPSHEETCONTEXT propSheetContext;
    WNDPROC oldWndProc;

    if (!(propSheetContext = PhGetWindowContext(WindowHandle, UCHAR_MAX)))
        return 0;

    oldWndProc = propSheetContext->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_ACTIVATE:
        SetupUpdateWindowBorderColor(WindowHandle, LOWORD(wParam) != WA_INACTIVE);
        break;
    case WM_DPICHANGED:
        {
            LRESULT result;
            PRECT suggestedRect = (RECT *)lParam;

            SetWindowPos(
                WindowHandle,
                NULL,
                suggestedRect->left,
                suggestedRect->top,
                suggestedRect->right - suggestedRect->left,
                suggestedRect->bottom - suggestedRect->top,
                SWP_NOZORDER | SWP_NOACTIVATE
                );

            result = CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);

            SetupUpdateWizardFonts(WindowHandle, propSheetContext, HIWORD(wParam));
            SetupUpdateWindowBorderColor(WindowHandle, TRUE);
            SetupRedrawPropSheetDividers(WindowHandle);
            SetupRedrawControlBorder(WindowHandle);

            return result;
        }
    case WM_DESTROY:
        {
            SetupDeleteDarkMode();
        }
        break;
    case WM_NCDESTROY:
        {
            PhSetWindowProcedure(WindowHandle, oldWndProc);
            PhRemoveWindowContext(WindowHandle, UCHAR_MAX);

            //PhDeleteLayoutManager(&propSheetContext->LayoutManager);
            SetupDeleteWizardFonts(&propSheetContext->Fonts);
            PhFree(propSheetContext);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDOK:
                // Prevent the OK button from working (even though
                // it's already hidden). This prevents the Enter
                // key from closing the dialog box.
                return 0;
            }
        }
        break;
    case WM_SIZE:
        {
            SetupRedrawPropSheetDividers(WindowHandle);

            //if (!IsMinimized(hWnd))
            //{
            //    PhLayoutManagerLayout(&propSheetContext->LayoutManager);
            //}
        }
        break;
    case WM_ERASEBKGND:
        if (SetupWizardDarkMode)
        {
            SetupPaintDarkBackground(WindowHandle, (HDC)wParam);
            return TRUE;
        }
        break;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
        {
            if (SetupWizardDarkMode)
                return SetupHandleDarkControlColor(wParam, lParam);
        }
        break;
    case WM_NOTIFY:
        {
            LRESULT result = SetupHandleControlCustomDraw((LPNMCUSTOMDRAW)lParam);

            if (result != CDRF_DODEFAULT)
                return result;
        }
        break;
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
}

/**
 * Callback for initializing the wizard property sheet.
 *
 * \param WindowHandle The property sheet window handle.
 * \param uMsg The property sheet callback message.
 * \param lParam Additional message information.
 * \return Zero.
 */
INT CALLBACK SetupPropSheetProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ LPARAM lParam
    )
{
#define PROPSHEET_ADD_STYLE WS_MINIMIZEBOX

    switch (uMsg)
    {
    case PSCB_PRECREATE:
        {
            if (lParam)
            {
                if (((DLGTEMPLATEEX*)lParam)->signature == USHRT_MAX)
                {
                    ((DLGTEMPLATEEX*)lParam)->style |= PROPSHEET_ADD_STYLE;
                }
                else
                {
                    ((DLGTEMPLATE*)lParam)->style |= PROPSHEET_ADD_STYLE;
                }
            }
        }
        break;
    case PSCB_INITIALIZED:
        {
            PPV_PROPSHEETCONTEXT context;

            SetupInitializeDarkMode();
            SetupAllowDarkModeForWindow(WindowHandle);
            SetupUpdateWindowBorderColor(WindowHandle, TRUE);
            SetupApplyDarkModeToPage(WindowHandle);
            SetupApplyDarkModeToControl(GetDlgItem(WindowHandle, IDC_PROPSHEET_BACK));
            SetupApplyDarkModeToControl(GetDlgItem(WindowHandle, IDC_PROPSHEET_NEXT));
            SetupApplyDarkModeToControl(GetDlgItem(WindowHandle, IDC_PROPSHEET_FINISH));
            SetupApplyDarkModeToControl(GetDlgItem(WindowHandle, IDC_PROPSHEET_CANCEL));
            context = PhAllocateZeroSafe(sizeof(PV_PROPSHEETCONTEXT));
            if (!context)
                break;
            //PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);

            context->DialogDpi = PhGetWindowDpi(WindowHandle);
            SetupCreateWizardFonts(WindowHandle, context->DialogDpi, &context->Fonts);

            context->DefaultWindowProc = PhGetWindowProcedure(WindowHandle);
            PhSetWindowContext(WindowHandle, UCHAR_MAX, context);
            PhSetWindowProcedure(WindowHandle, PvpPropSheetWndProc);

            SetupApplyWizardFonts(WindowHandle, context);
            SetupSubclassPropSheetDivider(WindowHandle, IDC_PROPSHEET_DIVIDER);
            SetupSubclassPropSheetDivider(WindowHandle, IDC_PROPSHEET_TOPDIVIDER);
        }
        break;
    }

    return 0;
}

/**
 * Shows the setup wizard.
 *
 * \param Context The setup context.
 */
VOID SetupShowWizard(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    LONG dpiValue;
    PROPSHEETPAGE pages[7];
    PROPSHEETHEADER header;

    dpiValue = PhGetMonitorDpi(NULL, NULL);

    Context->IconLargeHandle = PhLoadIcon(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDI_ICON),
        PH_LOAD_ICON_SIZE_LARGE,
        PhGetSystemMetrics(SM_CXICON, dpiValue),
        PhGetSystemMetrics(SM_CYICON, dpiValue),
        dpiValue
        );
    Context->IconSmallHandle = PhLoadIcon(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDI_ICON),
        PH_LOAD_ICON_SIZE_SMALL,
        PhGetSystemMetrics(SM_CXSMICON, dpiValue),
        PhGetSystemMetrics(SM_CYSMICON, dpiValue),
        dpiValue
        );

    pages[0].dwSize = sizeof(PROPSHEETPAGE);
    pages[0].dwFlags = PSP_DEFAULT | PSP_PREMATURE;
    pages[0].hInstance = PhInstanceHandle;
    pages[0].pszTemplate = MAKEINTRESOURCE(IDD_WELCOME);
    pages[0].pfnDlgProc = SetupWelcomePageDlgProc;
    pages[0].lParam = (LPARAM)Context;

    pages[1].dwSize = sizeof(PROPSHEETPAGE);
    pages[1].dwFlags = PSP_DEFAULT | PSP_PREMATURE;
    pages[1].hInstance = PhInstanceHandle;
    pages[1].pszTemplate = MAKEINTRESOURCE(IDD_CONFIG);
    pages[1].pfnDlgProc = SetupConfigPageDlgProc;
    pages[1].lParam = (LPARAM)Context;

    pages[2].dwSize = sizeof(PROPSHEETPAGE);
    pages[2].dwFlags = PSP_DEFAULT | PSP_PREMATURE;
    pages[2].hInstance = PhInstanceHandle;
    pages[2].pszTemplate = MAKEINTRESOURCE(IDD_SHORTCUTS);
    pages[2].pfnDlgProc = SetupShortcutsPageDlgProc;
    pages[2].lParam = (LPARAM)Context;

    pages[3].dwSize = sizeof(PROPSHEETPAGE);
    pages[3].dwFlags = PSP_DEFAULT | PSP_PREMATURE;
    pages[3].hInstance = PhInstanceHandle;
    pages[3].pszTemplate = MAKEINTRESOURCE(IDD_UNINSTALL);
    pages[3].pfnDlgProc = SetupUninstallPageDlgProc;
    pages[3].lParam = (LPARAM)Context;

    pages[4].dwSize = sizeof(PROPSHEETPAGE);
    pages[4].dwFlags = PSP_DEFAULT | PSP_PREMATURE;
    pages[4].hInstance = PhInstanceHandle;
    pages[4].pszTemplate = MAKEINTRESOURCE(IDD_INSTALL);
    pages[4].pfnDlgProc = SetupInstallPageDlgProc;
    pages[4].lParam = (LPARAM)Context;

    pages[5].dwSize = sizeof(PROPSHEETPAGE);
    pages[5].dwFlags = PSP_DEFAULT | PSP_PREMATURE;
    pages[5].hInstance = PhInstanceHandle;
    pages[5].pszTemplate = MAKEINTRESOURCE(IDD_COMPLETED);
    pages[5].pfnDlgProc = SetupCompletedPageDlgProc;
    pages[5].lParam = (LPARAM)Context;

    pages[6].dwSize = sizeof(PROPSHEETPAGE);
    pages[6].dwFlags = PSP_DEFAULT | PSP_PREMATURE;
    pages[6].hInstance = PhInstanceHandle;
    pages[6].pszTemplate = MAKEINTRESOURCE(IDD_ERROR);
    pages[6].pfnDlgProc = SetupErrorPageDlgProc;
    pages[6].lParam = (LPARAM)Context;

    memset(&header, 0, sizeof(PROPSHEETHEADER));
    header.dwSize = sizeof(PROPSHEETHEADER);
    header.dwFlags = PSH_MODELESS | PSH_PROPSHEETPAGE | PSH_WIZARD | PSH_USEHICON | PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP | PSH_PROPTITLE | PSH_USECALLBACK;
    header.hwndParent = NULL;
    header.hInstance = PhInstanceHandle;
    header.hIcon = Context->IconLargeHandle;
    header.pfnCallback = SetupPropSheetProc;
    header.pszCaption = L"System Informer Setup";
    header.nPages = ARRAYSIZE(pages);
    header.ppsp = pages;

    if (Context->SetupMode == SetupCommandUninstall)
    {
        header.nStartPage = SETUP_WIZARD_UNINSTALL_PAGE_INDEX;
    }
    else if (Context->SetupMode == SetupCommandUpdate)
    {
        header.nStartPage = SETUP_WIZARD_INSTALL_PAGE_INDEX;
    }

    PhModalPropertySheet(&header);
}
