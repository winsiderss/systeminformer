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
#pragma comment(lib, "uxtheme.lib")

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

#define SETUP_WIZARD_COMPLETED_PAGE_INDEX 3
#define SETUP_WIZARD_ERROR_PAGE_INDEX 4

typedef BOOL (WINAPI* SETUP_SHOULDAPPSUSEDARKMODE)(VOID);
typedef BOOL (WINAPI* SETUP_ALLOWDARKMODEFORWINDOW)(_In_ HWND WindowHandle, _In_ BOOL Allow);
typedef HRESULT (WINAPI* SETUP_DWMSETWINDOWATTRIBUTE)(_In_ HWND hwnd, _In_ DWORD dwAttribute, _In_reads_bytes_(cbAttribute) LPCVOID pvAttribute, _In_ DWORD cbAttribute);

#define SETUP_DARKMODE_SUBCLASS_ID 1
#define SETUP_DWM_USE_IMMERSIVE_DARK_MODE 20
#define SETUP_DWM_BORDER_COLOR 34
#define SETUP_WINDOW_CONTEXT_LARGE_TITLE UCHAR_MAX

static BOOLEAN SetupWizardDarkMode = FALSE;
static SETUP_DWMSETWINDOWATTRIBUTE SetupDwmSetWindowAttribute_I = NULL;
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

typedef struct _SETUP_WIZARD_FONT_CONTEXT
{
    HFONT PageFont;
    HFONT TitleFont;
    HFONT LargeTitleFont;
    HFONT ButtonFont;
} SETUP_WIZARD_FONT_CONTEXT, *PSETUP_WIZARD_FONT_CONTEXT;

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
    SETUP_SHOULDAPPSUSEDARKMODE shouldAppsUseDarkMode;

    if (!(uxthemeModule = PhGetDllHandle(L"uxtheme.dll")))
        uxthemeModule = PhLoadLibrary(L"uxtheme.dll");

    if (!uxthemeModule)
        return FALSE;

    shouldAppsUseDarkMode = (SETUP_SHOULDAPPSUSEDARKMODE)PhGetProcedureAddress(uxthemeModule, NULL, 132);

    if (shouldAppsUseDarkMode && shouldAppsUseDarkMode())
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
    SETUP_ALLOWDARKMODEFORWINDOW allowDarkModeForWindow;
    BOOL value;

    if (!SetupWizardDarkMode)
        return;

    if (uxthemeModule = PhGetDllHandle(L"uxtheme.dll"))
    {
        allowDarkModeForWindow = (SETUP_ALLOWDARKMODEFORWINDOW)PhGetProcedureAddress(uxthemeModule, NULL, 133);

        if (allowDarkModeForWindow)
            allowDarkModeForWindow(WindowHandle, TRUE);
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

    if (controlHandle && GetClassName(controlHandle, className, ARRAYSIZE(className)) && PhEqualStringZ(className, L"Edit", TRUE))
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
    style = (ULONG)GetWindowLongPtr(windowHandle, GWL_STYLE);

    pushed = !!(state & BST_PUSHED);
    hot = !!(state & BST_HOT);
    disabled = !IsWindowEnabled(windowHandle);
    focused = (GetFocus() == windowHandle);
    checkbox = ((style & BS_TYPEMASK) == BS_AUTOCHECKBOX) || ((style & BS_TYPEMASK) == BS_CHECKBOX);

    if (checkbox)
    {
        RECT checkRect;

        FillRect(hdc, &CustomDraw->rc, SetupWizardBrushWindow);

        checkRect.left = CustomDraw->rc.left;
        checkRect.top = CustomDraw->rc.top + ((CustomDraw->rc.bottom - CustomDraw->rc.top) - 13) / 2;
        checkRect.right = CustomDraw->rc.left + 13;
        checkRect.bottom = CustomDraw->rc.top + 13;

        FillRect(hdc, &checkRect, SetupWizardBrushControl);

        SetDCBrushColor(hdc, RGB(0, 130, 135));
        FrameRect(hdc, &checkRect, GetStockBrush(DC_BRUSH));

        if (SendMessage(windowHandle, BM_GETCHECK, 0, 0) == BST_CHECKED)
        {
            HPEN penHandle;
            HPEN oldPenHandle;

            penHandle = CreatePen(PS_SOLID, 2, RGB(0, 120, 215));
            oldPenHandle = SelectPen(hdc, penHandle);
            MoveToEx(hdc, checkRect.left + 3, checkRect.top + 7, NULL);
            LineTo(hdc, checkRect.left + 6, checkRect.top + 10);
            LineTo(hdc, checkRect.left + 11, checkRect.top + 3);
            SelectPen(hdc, oldPenHandle);
            DeletePen(penHandle);
        }

        textRect = CustomDraw->rc;
        textRect.left += 18;
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
            InflateRect(&focusRect, -3, -3);
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
    _In_ LPARAM lParam,
    _In_ UINT_PTR SubclassId,
    _In_ DWORD_PTR RefData
    )
{
    switch (uMsg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(WindowHandle, SetupDividerSubclassProc, SubclassId);
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

    return DefSubclassProc(WindowHandle, uMsg, wParam, lParam);
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
        SetWindowSubclass(dividerHandle, SetupDividerSubclassProc, SETUP_DARKMODE_SUBCLASS_ID, 0);
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
 * Redraws the non-client border of an edit control.
 *
 * \param WindowHandle The edit control window handle.
 */
VOID SetupRedrawEditBorder(
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
 * Draws a Windows 7 style edit border without painting the client area.
 *
 * \param WindowHandle The edit control window handle.
 */
VOID SetupPaintEditBorder(
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

/**
 * Subclass procedure for edit border painting.
 */
LRESULT CALLBACK SetupDarkEditSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ UINT_PTR SubclassId,
    _In_ DWORD_PTR RefData
    )
{
    LRESULT result;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(WindowHandle, SetupDarkEditSubclassProc, SubclassId);
        break;
    case WM_NCPAINT:
        result = DefSubclassProc(WindowHandle, uMsg, wParam, lParam);
        SetupPaintEditBorder(WindowHandle, wParam);
        return result;
    case WM_NCACTIVATE:
        result = DefSubclassProc(WindowHandle, uMsg, wParam, lParam);
        SetupPaintEditBorder(WindowHandle, wParam);
        return result;
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    case WM_ENABLE:
        result = DefSubclassProc(WindowHandle, uMsg, wParam, lParam);
        SetupRedrawEditBorder(WindowHandle);
        return result;
    }

    return DefSubclassProc(WindowHandle, uMsg, wParam, lParam);
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

    if (GetClassName(WindowHandle, className, ARRAYSIZE(className)))
    {
        if (PhEqualStringZ(className, L"Edit", TRUE))
        {
            SetWindowSubclass(WindowHandle, SetupDarkEditSubclassProc, SETUP_DARKMODE_SUBCLASS_ID, 0);
            SetupRedrawEditBorder(WindowHandle);
        }
        else if (PhEqualStringZ(className, L"msctls_progress32", TRUE))
        {
            //SetWindowTheme(WindowHandle, L"", L"");
            SendMessage(WindowHandle, PBM_SETBKCOLOR, 0, SetupWizardColorControl);
            SendMessage(WindowHandle, PBM_SETBARCOLOR, 0, RGB(0, 120, 215));
        }
    }
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

        if (GetClassName(customDraw->hdr.hwndFrom, className, ARRAYSIZE(className)))
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
BOOL CALLBACK SetupApplyDarkModeToChildControl(
    _In_ HWND WindowHandle,
    _In_ LPARAM lParam
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
    EnumChildWindows(WindowHandle, SetupApplyDarkModeToChildControl, 0);
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
    RECT clientRect;

    GetClientRect(WindowHandle, &clientRect);
    FillRect(DcHandle, &clientRect, SetupWizardBrushWindow);
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

static BOOL CALLBACK SetupSetWizardChildFont(
    _In_ HWND WindowHandle,
    _In_ LPARAM lParam
    );

static BOOL CALLBACK SetupInvalidateWizardChildWindow(
    _In_ HWND WindowHandle,
    _In_ LPARAM lParam
    );

typedef struct _SETUP_PROPSHEETCONTEXT
{
    BOOLEAN LayoutInitialized;
    WNDPROC DefaultWindowProc;
    PH_LAYOUT_MANAGER LayoutManager;
    PPH_LAYOUT_ITEM TabPageItem;
    SETUP_WIZARD_FONT_CONTEXT Fonts;
    LONG DialogDpi;
} PV_PROPSHEETCONTEXT, * PPV_PROPSHEETCONTEXT;

static VOID SetupDeleteWizardFonts(
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

static BOOLEAN SetupCreateWizardFonts(
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
 * Applies the wizard fonts to a page.
 *
 * \param WindowHandle The wizard page window handle.
 * \param LargeTitle TRUE to use the welcome/completed page title font.
 */
static VOID SetupApplyWizardPageFonts(
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
        EnumChildWindows(WindowHandle, SetupSetWizardChildFont, (LPARAM)fontContext);

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

/**
 * Sets the wizard page font for a child window.
 *
 * \param WindowHandle The child window handle.
 * \param lParam The font handle.
 * \return TRUE to continue enumeration.
 */
static BOOL CALLBACK SetupSetWizardChildFont(
    _In_ HWND WindowHandle,
    _In_ LPARAM lParam
    )
{
    PSETUP_WIZARD_FONT_CONTEXT fontContext;
    WCHAR className[16];

    fontContext = (PSETUP_WIZARD_FONT_CONTEXT)lParam;

    if (GetClassName(WindowHandle, className, ARRAYSIZE(className)) && PhEqualStringZ(className, L"Button", TRUE))
    {
        ULONG style = (ULONG)GetWindowLongPtr(WindowHandle, GWL_STYLE);

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

static BOOL CALLBACK SetupInvalidateWizardChildWindow(
    _In_ HWND WindowHandle,
    _In_ LPARAM lParam
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
static VOID SetupInitializeWizardTitleFont(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN LargeTitle
    )
{
    PPV_PROPSHEETCONTEXT propSheetContext;

    PhSetWindowContext(WindowHandle, SETUP_WINDOW_CONTEXT_LARGE_TITLE, (PVOID)(ULONG_PTR)LargeTitle);

    if (propSheetContext = PhGetWindowContext(GetParent(WindowHandle), UCHAR_MAX))
        SetupApplyWizardPageFonts(WindowHandle, propSheetContext, LargeTitle);
}

static VOID SetupDestroyWizardPage(
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
static VOID SetupSetPropSheetButtonFonts(
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

static VOID SetupApplyWizardFonts(
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

static VOID SetupUpdateWizardFonts(
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
    EnumChildWindows(ParentWindowHandle, SetupInvalidateWizardChildWindow, 0);
    InvalidateRect(ParentWindowHandle, NULL, TRUE);
    UpdateWindow(ParentWindowHandle);
    SetupDeleteWizardFonts(&oldFonts);
}

/**
 * Loads the welcome page bitmap from the embedded PNG resource.
 *
 * \param WindowHandle The wizard page window handle.
 */
static VOID SetupLoadWelcomeBitmap(
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
static VOID SetupShowWizardButton(
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

static VOID SetupEnableizardButton(
    _In_ HWND ParentWindowHandle,
    _In_ INT ControlId,
    _In_ BOOLEAN ShowButton
)
{
    HWND buttonHandle;

    if (buttonHandle = GetDlgItem(ParentWindowHandle, ControlId))
    {
        EnableWindow(buttonHandle, ShowButton );
    }
}

static VOID SetupSetWizardButtonText(
    _In_ HWND ParentWindowHandle,
    _In_ INT ControlId,
    _In_ PCWSTR Text
    )
{
    HWND buttonHandle;

    if (buttonHandle = GetDlgItem(ParentWindowHandle, ControlId))
        SetWindowText(buttonHandle, Text);
}

static BOOLEAN SetupCancelWizard(
    _In_ HWND WindowHandle,
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    if (!Context->SetupProgressActive)
        return TRUE;

    if (PhShowMessage2(
        GetParent(WindowHandle),
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
 * \param ShowBack TRUE to show the Back button.
 * \param ShowNext TRUE to show the Next button.
 * \param ShowFinish TRUE to show the Finish button.
 * \param ShowCancel TRUE to show the Cancel button.
 */
static VOID SetupSetWizardButtons(
    _In_ HWND WindowHandle,
    _In_ DWORD Buttons,
    _In_ BOOLEAN ShowBack,
    _In_ BOOLEAN ShowNext,
    _In_ BOOLEAN ShowFinish,
    _In_ BOOLEAN ShowCancel
    )
{
    HWND parentWindowHandle;

    parentWindowHandle = GetParent(WindowHandle);
    PropSheet_SetWizButtons(parentWindowHandle, Buttons);
    SetupSetWizardButtonText(parentWindowHandle, IDC_PROPSHEET_BACK, L"< &Back");
    SetupEnableizardButton(parentWindowHandle, IDC_PROPSHEET_BACK, ShowBack);
    SetupEnableizardButton(parentWindowHandle, IDC_PROPSHEET_NEXT, ShowNext);
    SetupEnableizardButton(parentWindowHandle, IDC_PROPSHEET_FINISH, ShowFinish);
    SetupEnableizardButton(parentWindowHandle, IDC_PROPSHEET_CANCEL, ShowCancel);
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
            PhCenterWindow(GetParent(WindowHandle), NULL);
            SetupInitializeWizardTitleFont(WindowHandle, TRUE);
            SetupLoadWelcomeBitmap(WindowHandle);
            SetupApplyDarkModeToPage(WindowHandle);

            SetupSetWizardButtons(WindowHandle, PSWIZB_NEXT, FALSE, TRUE, FALSE, TRUE);

            if (!PhGetOwnTokenAttributes().Elevated)
            {
                Button_SetElevationRequiredState(GetDlgItem(GetParent(WindowHandle), IDC_PROPSHEET_NEXT), TRUE);
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
                SetupSetWizardButtons(WindowHandle, PSWIZB_NEXT, FALSE, TRUE, FALSE, TRUE);
                break;
            case PSN_QUERYCANCEL:
                return !SetupCancelWizard(WindowHandle, context);
            case PSN_WIZNEXT:
                {
                    if (PhGetOwnTokenAttributes().Elevated)
                    {
                        if (!context->SetupIsLegacyUpdate && NT_SUCCESS(SetupLegacySetupInstalled()))
                        {
                            SetupShowMessagePromptForLegacyVersion();
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
                        SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, TRUE);
                        return TRUE;
                    }
                }
                break;
            }
        }
        break;
    case WM_DPICHANGED:
    case WM_DPICHANGED_AFTERPARENT:
        SetupRedrawEditBorder(WindowHandle);
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

        SetupInitializeWizardTitleFont(WindowHandle, FALSE);
        SetupApplyDarkModeToPage(WindowHandle);
        SetDlgItemText(WindowHandle, IDC_PATH, PhGetStringOrEmpty(context->SetupInstallPath));
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

                            fileDialogFolderPath = PhGetFileDialogFileName(fileDialog);
                            PhTrimToNullTerminatorString(fileDialogFolderPath);
                            PhSwapReference(&context->SetupInstallPath, fileDialogFolderPath);
                            SetDlgItemText(WindowHandle, IDC_PATH, PhGetStringOrEmpty(context->SetupInstallPath));
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
                SetupSetWizardButtons(WindowHandle, PSWIZB_BACK | PSWIZB_NEXT, TRUE, TRUE, FALSE, TRUE);
                break;
            case PSN_QUERYCANCEL:
                return !SetupCancelWizard(WindowHandle, context);
            case PSN_WIZNEXT:
                {
                    WCHAR path[MAX_PATH];

                    GetDlgItemText(WindowHandle, IDC_PATH, path, MAX_PATH);

                    PhSwapReference(&context->SetupInstallPath, PhCreateString(path));

                    if (PhIsNullOrEmptyString(context->SetupInstallPath))
                    {
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
        SetupRedrawEditBorder(WindowHandle);
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

        SetupInitializeWizardTitleFont(WindowHandle, FALSE);
        SetupApplyDarkModeToPage(WindowHandle);
        SendMessage(GetDlgItem(WindowHandle, IDC_PROGRESS), PBM_SETMARQUEE, TRUE, 0);
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
                    SetupSetWizardButtons(WindowHandle, 0, FALSE, FALSE, FALSE, TRUE);
                    context->DialogHandle = WindowHandle;
                    context->SetupProgressActive = TRUE;
                    PhCreateThread2(SetupProgressThread, context);
                }
                break;
            case PSN_QUERYCANCEL:
                return !SetupCancelWizard(WindowHandle, context);
            }
        }
        break;
    case WM_DPICHANGED:
    case WM_DPICHANGED_AFTERPARENT:
        SetupRedrawEditBorder(WindowHandle);
        break;
    case SETUP_SHOWFINAL:
        {
            PropSheet_SetCurSel(GetParent(WindowHandle), NULL, SETUP_WIZARD_COMPLETED_PAGE_INDEX);
        }
        break;
    case SETUP_SHOWERROR:
        {
            PropSheet_SetCurSel(GetParent(WindowHandle), NULL, SETUP_WIZARD_ERROR_PAGE_INDEX);
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

        SetupInitializeWizardTitleFont(WindowHandle, TRUE);
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
                SetupSetWizardButtons(WindowHandle, PSWIZB_FINISH, FALSE, FALSE, TRUE, FALSE);
                break;
            case PSN_QUERYCANCEL:
                return !SetupCancelWizard(WindowHandle, context);
            case PSN_WIZFINISH:
                {
                    if (IsDlgButtonChecked(WindowHandle, IDC_STARTAPP) == BST_CHECKED)
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
        SetupRedrawEditBorder(WindowHandle);
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

        SetupInitializeWizardTitleFont(WindowHandle, TRUE);
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
                        SetDlgItemText(WindowHandle, IDC_STATUS, PhGetString(statusMessage));
                    }
                    else
                    {
                        SetDlgItemText(WindowHandle, IDC_STATUS, L"An unknown error occurred.");
                    }

                    SetupSetWizardButtons(WindowHandle, PSWIZB_BACK, TRUE, FALSE, FALSE, TRUE);
                    SetupSetWizardButtonText(GetParent(WindowHandle), IDC_PROPSHEET_BACK, L"Retry");
                }
                break;
            case PSN_QUERYCANCEL:
                return !SetupCancelWizard(WindowHandle, context);
            case PSN_WIZBACK:
                PropSheet_SetCurSel(GetParent(WindowHandle), NULL, 0);
                SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, -1);
                return TRUE;
            }
        }
        break;
    case WM_DPICHANGED:
    case WM_DPICHANGED_AFTERPARENT:
        SetupRedrawEditBorder(WindowHandle);
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
            RECT *suggestedRect = (RECT *)lParam;

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
            SetupRedrawEditBorder(WindowHandle);

            return result;
        }
    case WM_SETFONT:
        return 0;
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
        if (SetupWizardDarkMode)
            return SetupHandleDarkControlColor(wParam, lParam);
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

            PhCenterWindow(WindowHandle, NULL);
            SetupInitializeDarkMode();
            SetupAllowDarkModeForWindow(WindowHandle);
            SetupUpdateWindowBorderColor(WindowHandle, TRUE);
            SetupApplyDarkModeToPage(WindowHandle);
            SetupApplyDarkModeToControl(GetDlgItem(WindowHandle, IDC_PROPSHEET_BACK));
            SetupApplyDarkModeToControl(GetDlgItem(WindowHandle, IDC_PROPSHEET_NEXT));
            SetupApplyDarkModeToControl(GetDlgItem(WindowHandle, IDC_PROPSHEET_FINISH));
            SetupApplyDarkModeToControl(GetDlgItem(WindowHandle, IDC_PROPSHEET_CANCEL));
            context = PhAllocateZero(sizeof(PV_PROPSHEETCONTEXT));
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
    PH_AUTO_POOL autoPool;
    PROPSHEETPAGE pages[5];
    PROPSHEETHEADER header;

    PhInitializeAutoPool(&autoPool);

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
    pages[2].pszTemplate = MAKEINTRESOURCE(IDD_INSTALL);
    pages[2].pfnDlgProc = SetupInstallPageDlgProc;
    pages[2].lParam = (LPARAM)Context;

    pages[3].dwSize = sizeof(PROPSHEETPAGE);
    pages[3].dwFlags = PSP_DEFAULT | PSP_PREMATURE;
    pages[3].hInstance = PhInstanceHandle;
    pages[3].pszTemplate = MAKEINTRESOURCE(IDD_COMPLETED);
    pages[3].pfnDlgProc = SetupCompletedPageDlgProc;
    pages[3].lParam = (LPARAM)Context;

    pages[4].dwSize = sizeof(PROPSHEETPAGE);
    pages[4].dwFlags = PSP_DEFAULT | PSP_PREMATURE;
    pages[4].hInstance = PhInstanceHandle;
    pages[4].pszTemplate = MAKEINTRESOURCE(IDD_ERROR);
    pages[4].pfnDlgProc = SetupErrorPageDlgProc;
    pages[4].lParam = (LPARAM)Context;

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

    PhModalPropertySheet(&header);
}
