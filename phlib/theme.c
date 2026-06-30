/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2018-2026
 *
 */

#include <ph.h>
#include <guisup.h>
#include <emenu.h>
#include <treenew.h>
#include <mapldr.h>

#include <dwmapi.h>
#include <uxtheme.h>
#include <vsstyle.h>
#include <vssym32.h>

typedef struct _PHP_THEME_WINDOW_TAB_CONTEXT
{
    WNDPROC DefaultWindowProc;
    LONG WindowDpi;
    BOOLEAN MouseActive;
    POINT CursorPos;
} PHP_THEME_WINDOW_TAB_CONTEXT, *PPHP_THEME_WINDOW_TAB_CONTEXT;

typedef struct _PHP_THEME_WINDOW_STATUSBAR_CONTEXT
{
    WNDPROC DefaultWindowProc;
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
    POINT CursorPos;

    HDC BufferedDc;
    HBITMAP BufferedOldBitmap;
    HBITMAP BufferedBitmap;
    RECT BufferedContextRect;
} PHP_THEME_WINDOW_STATUSBAR_CONTEXT, *PPHP_THEME_WINDOW_STATUSBAR_CONTEXT;

typedef struct _PHP_THEME_WINDOW_COMBO_CONTEXT
{
    WNDPROC DefaultWindowProc;
    HTHEME ThemeHandle;
    LONG WindowDpi;
    BOOLEAN MouseActive;
    POINT CursorPos;
} PHP_THEME_WINDOW_COMBO_CONTEXT, *PPHP_THEME_WINDOW_COMBO_CONTEXT;

typedef struct _PHP_THEME_WINDOW_EDIT_CONTEXT
{
    WNDPROC DefaultWindowProc;
    HWND ParentWindowHandle;
    LONG WindowDpi;
    BOOLEAN WindowFocus;
    HWND PreviousFocusWindowHandle;

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

    HBRUSH WindowBrush;
    HBRUSH FrameBrush;
    LONG BorderSize;

    HDC BufferedDc;
    HBITMAP BufferedOldBitmap;
    HBITMAP BufferedBitmap;
    RECT BufferedContextRect;
} PHP_THEME_WINDOW_EDIT_CONTEXT, *PPHP_THEME_WINDOW_EDIT_CONTEXT;

typedef struct _PHP_THEME_PAINT_BUFFER
{
    HDC TargetDc;
    HDC PaintDc;
    HDC MemoryDc;
    HBITMAP Bitmap;
    HBITMAP OldBitmap;
    HPAINTBUFFER BufferedPaint;
    RECT Rect;
} PHP_THEME_PAINT_BUFFER, *PPHP_THEME_PAINT_BUFFER;

_Function_class_(PH_WINDOW_ENUM_CALLBACK)
BOOLEAN CALLBACK PhpThemeWindowEnumChildWindows(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    );

_Function_class_(PH_WINDOW_ENUM_CALLBACK)
BOOLEAN CALLBACK PhpReInitializeThemeWindowEnumChildWindows(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    );

LRESULT CALLBACK PhpThemeWindowSubclassProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

LRESULT CALLBACK PhpThemeWindowGroupBoxSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

LRESULT CALLBACK PhThemeWindowGroupBoxExSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

LRESULT CALLBACK PhpThemeWindowTabControlWndSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

LRESULT CALLBACK PhpThemeWindowListBoxControlSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

LRESULT CALLBACK PhpThemeWindowComboBoxControlSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

LRESULT CALLBACK PhpThemeWindowACLUISubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

LRESULT CALLBACK PhEditBorderWndSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhpThemeWindowEditThemeChanged(
    _In_ PPHP_THEME_WINDOW_EDIT_CONTEXT Context,
    _In_ HWND WindowHandle
    );

VOID PhWindowThemeSetDarkMode(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN EnableDarkMode
    );

// Win10-RS5 (uxtheme.dll ordinal 132)
BOOL (WINAPI *ShouldAppsUseDarkMode_I)(
    VOID
    ) = NULL;
// Win10-RS5 (uxtheme.dll ordinal 138)
BOOL (WINAPI *ShouldSystemUseDarkMode_I)(
    VOID
    ) = NULL;
// Win10-RS5 (uxtheme.dll ordinal 136)
BOOL (WINAPI* FlushMenuThemes_I)(
    VOID
    ) = NULL;

typedef enum _PreferredAppMode
{
    PreferredAppModeDisabled,
    PreferredAppModeDarkOnDark,
    PreferredAppModeDarkAlways
} PreferredAppMode;

// Win10-RS5 (uxtheme.dll ordinal 135)
// Win10 build 17763: AllowDarkModeForApp(BOOL)
// Win10 build 18334: SetPreferredAppMode(enum PreferredAppMode)
BOOL (WINAPI* SetPreferredAppMode_I)(
    _In_ PreferredAppMode AppMode
    ) = NULL;

// Win10-RS5 (uxtheme.dll ordinal 139)
BOOL (WINAPI *IsDarkModeAllowedForApp_I)(
    _In_ HWND WindowHandle
    ) = NULL;

//HRESULT (WINAPI* DwmGetColorizationColor_I)(
//    _Out_ PULONG Colorization,
//    _Out_ PBOOL OpaqueBlend
//    );

#ifdef DEBUG
#define DEBUG_BEGINPAINT_RECT(WindowHandle, RcPaint) \
{\
    RECT rect;\
    GetClientRect((WindowHandle), &rect);\
    assert(EqualRect(&rect, &(RcPaint)));\
}
#else
#define DEBUG_BEGINPAINT_RECT(RcPaint)
#endif

BOOLEAN PhEnableThemeSupport = FALSE;
BOOLEAN PhEnableThemeAcrylicSupport = FALSE;
BOOLEAN PhEnableThemeAcrylicWindowSupport = FALSE;
BOOLEAN PhEnableThemeNativeButtons = FALSE;
BOOLEAN PhEnableThemeListviewBorder = FALSE;
BOOLEAN PhEnableWindowBorderColor = TRUE;
HBRUSH PhThemeWindowBackgroundBrush = NULL;
COLORREF PhThemeWindowForegroundColor = RGB(28, 28, 28);
COLORREF PhThemeWindowBackgroundColor = RGB(43, 43, 43);
COLORREF PhThemeWindowBackground2Color = RGB(65, 65, 65);
COLORREF PhThemeWindowHighlightColor = RGB(128, 128, 128);
COLORREF PhThemeWindowHighlight2Color = RGB(143, 143, 143);
COLORREF PhThemeWindowTextColor = RGB(255, 255, 255);
COLORREF PhThemeWindowDisabledTextColor = RGB(155, 155, 155);
COLORREF PhThemeWindowBorderColor = RGB(95, 95, 95);
COLORREF PhThemeWindowEditColor = RGB(60, 60, 60);
COLORREF PhThemeWindowScrollbarColor = RGB(23, 23, 23);
COLORREF PhThemeWindowFilteredBorderColor = RGB(255, 0, 0);
COLORREF PhThemeWindowProtectedBorderColor = RGB(255, 128, 0);
COLORREF PhThemeWindowFocusBorderColor = RGB(0, 120, 215);
COLORREF PhThemeWindowGroupBoxFrameColor = RGB(95, 95, 95);
COLORREF PhThemeWindowWindowFrameColor = RGB(95, 95, 95);
COLORREF PhThemeWindowEditHotBorderColor = RGB(143, 143, 143);
COLORREF PhThemeWindowEditNormalBorderColor = RGB(208, 208, 208);
COLORREF PhThemeWindowMenuSelectedTextColor = RGB(255, 255, 255);
COLORREF PhThemeWindowMenuDisabledTextColor = RGB(155, 155, 155);

// Cached DC brush returned by PhGetStockBrush(DC_BRUSH)
static HBRUSH PhpStockDCBrush = NULL;

// Set once a palette has been explicitly selected (e.g. via PhApplyThemeMode at
// startup) so PhInitializeWindowTheme's default-palette InitOnce does not clobber
// the chosen theme with the hard-coded Dark/System default.
static BOOLEAN PhpWindowThemePaletteApplied = FALSE;

static CONST PH_WINDOW_THEME_PALETTE PhpWindowThemeLightPalette =
{
    RGB(255, 255, 255), // ForegroundColor
    RGB(255, 255, 255), // BackgroundColor
    RGB(245, 245, 245), // Background2Color
    RGB(204, 232, 255), // HighlightColor
    RGB(153, 209, 255), // Highlight2Color
    RGB(0, 0, 0),       // TextColor
    RGB(109, 109, 109), // DisabledTextColor
    RGB(160, 160, 160), // BorderColor
    RGB(229, 241, 251), // PressedColor
    RGB(255, 255, 255), // EditColor
    RGB(240, 240, 240), // ScrollbarColor
    RGB(60, 60, 60),    // DropdownGlyphColor
    RGB(0, 120, 215),   // WindowActiveBorderColor
    RGB(160, 160, 160), // WindowInactiveBorderColor
    RGB(255, 0, 0),     // FilteredBorderColor
    RGB(255, 128, 0),   // ProtectedBorderColor
    RGB(0, 120, 215),   // FocusBorderColor
    RGB(160, 160, 160), // GroupBoxFrameColor
    RGB(0, 0, 0),       // WindowFrameColor
    RGB(204, 232, 255), // EditHotBorderColor
    RGB(208, 208, 208), // EditNormalBorderColor
    RGB(255, 255, 255), // MenuSelectedTextColor
    RGB(109, 109, 109)  // MenuDisabledTextColor
};

static CONST PH_WINDOW_THEME_PALETTE PhpWindowThemeDarkPalette =
{
    RGB(28, 28, 28),    // ForegroundColor
    RGB(43, 43, 43),    // BackgroundColor
    RGB(65, 65, 65),    // Background2Color
    RGB(128, 128, 128), // HighlightColor
    RGB(143, 143, 143), // Highlight2Color
    RGB(255, 255, 255), // TextColor
    RGB(155, 155, 155), // DisabledTextColor
    RGB(95, 95, 95),    // BorderColor
    RGB(78, 78, 78),    // PressedColor
    RGB(60, 60, 60),    // EditColor
    RGB(23, 23, 23),    // ScrollbarColor
    RGB(222, 222, 222), // DropdownGlyphColor
    RGB(0, 120, 215),   // WindowActiveBorderColor
    RGB(90, 90, 90),    // WindowInactiveBorderColor
    RGB(255, 0, 0),     // FilteredBorderColor
    RGB(255, 128, 0),   // ProtectedBorderColor
    RGB(0, 120, 215),   // FocusBorderColor
    RGB(95, 95, 95),    // GroupBoxFrameColor
    RGB(95, 95, 95),    // WindowFrameColor
    RGB(143, 143, 143), // EditHotBorderColor
    RGB(60, 60, 60),    // EditNormalBorderColor
    RGB(255, 255, 255), // MenuSelectedTextColor
    RGB(155, 155, 155)  // MenuDisabledTextColor
};

static PH_WINDOW_THEME_PALETTE PhpWindowThemeCustom1Palette = { 0 };
static PH_WINDOW_THEME_PALETTE PhpWindowThemeCustom2Palette = { 0 };
static PH_WINDOW_THEME_PALETTE PhpWindowThemeSystemPalette = { 0 };
static PH_WINDOW_THEME_PALETTE PhpWindowThemeCurrentPalette =
{
    RGB(28, 28, 28),
    RGB(43, 43, 43),
    RGB(65, 65, 65),
    RGB(128, 128, 128),
    RGB(143, 143, 143),
    RGB(255, 255, 255),
    RGB(155, 155, 155),
    RGB(95, 95, 95),
    RGB(78, 78, 78),
    RGB(60, 60, 60),
    RGB(23, 23, 23),
    RGB(222, 222, 222),
    RGB(0, 120, 215),
    RGB(90, 90, 90),
    RGB(255, 0, 0),
    RGB(255, 128, 0),
    RGB(0, 120, 215),
    RGB(95, 95, 95),
    RGB(95, 95, 95),
    RGB(143, 143, 143),
    RGB(60, 60, 60),
    RGB(255, 255, 255),
    RGB(155, 155, 155)
};
static PH_WINDOW_THEME_ID PhpWindowThemeCurrentId = PhWindowThemeDark;

#define PHP_THEME_WINDOW_EDIT_COLOR PhpWindowThemeCurrentPalette.EditColor
#define PHP_THEME_WINDOW_SCROLLBAR_COLOR PhpWindowThemeCurrentPalette.ScrollbarColor
#define PHP_THEME_WINDOW_DISABLED_TEXT_COLOR PhpWindowThemeCurrentPalette.DisabledTextColor
#define PHP_THEME_WINDOW_BORDER_COLOR PhpWindowThemeCurrentPalette.BorderColor
#define PHP_THEME_WINDOW_PRESSED_COLOR PhpWindowThemeCurrentPalette.PressedColor
#define PHP_THEME_WINDOW_DROPDOWN_GLYPH_COLOR PhpWindowThemeCurrentPalette.DropdownGlyphColor

// The only place in this file where GetSysColor* is called. Populates a
// palette from current Windows system colors so that drawing code can read
// from PhpWindowThemeCurrentPalette unconditionally even when the user has
// disabled custom theming.
static VOID PhpResolveSystemPalette(
    _Out_ PPH_WINDOW_THEME_PALETTE Palette
    )
{
    COLORREF window = GetSysColor(COLOR_WINDOW);
    COLORREF windowText = GetSysColor(COLOR_WINDOWTEXT);
    COLORREF highlight = GetSysColor(COLOR_HIGHLIGHT);
    COLORREF highlightText = GetSysColor(COLOR_HIGHLIGHTTEXT);
    COLORREF grayText = GetSysColor(COLOR_GRAYTEXT);
    COLORREF hotlight = GetSysColor(COLOR_HOTLIGHT);
    COLORREF btnShadow = GetSysColor(COLOR_BTNSHADOW);
    COLORREF windowFrame = GetSysColor(COLOR_WINDOWFRAME);
    COLORREF scrollbar = GetSysColor(COLOR_SCROLLBAR);
    COLORREF btnFace = GetSysColor(COLOR_BTNFACE);

    Palette->ForegroundColor = window;
    Palette->BackgroundColor = window;
    Palette->Background2Color = btnFace;
    Palette->HighlightColor = highlight;
    Palette->Highlight2Color = highlight;
    Palette->TextColor = windowText;
    Palette->DisabledTextColor = grayText;
    Palette->BorderColor = btnShadow;
    Palette->PressedColor = btnFace;
    Palette->EditColor = window;
    Palette->ScrollbarColor = scrollbar;
    Palette->DropdownGlyphColor = windowText;
    Palette->WindowActiveBorderColor = hotlight;
    Palette->WindowInactiveBorderColor = btnShadow;
    Palette->FilteredBorderColor = RGB(255, 0, 0);
    Palette->ProtectedBorderColor = RGB(255, 128, 0);
    Palette->FocusBorderColor = hotlight;
    Palette->GroupBoxFrameColor = btnShadow;
    Palette->WindowFrameColor = windowFrame;
    Palette->EditHotBorderColor = highlight;
    Palette->EditNormalBorderColor = RGB(208, 208, 208);
    Palette->MenuSelectedTextColor = highlightText;
    Palette->MenuDisabledTextColor = grayText;
}

static VOID PhpApplyWindowThemePalette(
    _In_ const PH_WINDOW_THEME_PALETTE* Palette
    )
{
    HBRUSH previousBrush;

    PhpWindowThemeCurrentPalette = *Palette;

    PhThemeWindowForegroundColor = Palette->ForegroundColor;
    PhThemeWindowBackgroundColor = Palette->BackgroundColor;
    PhThemeWindowBackground2Color = Palette->Background2Color;
    PhThemeWindowHighlightColor = Palette->HighlightColor;
    PhThemeWindowHighlight2Color = Palette->Highlight2Color;
    PhThemeWindowTextColor = Palette->TextColor;
    PhThemeWindowDisabledTextColor = Palette->DisabledTextColor;
    PhThemeWindowBorderColor = Palette->BorderColor;
    PhThemeWindowEditColor = Palette->EditColor;
    PhThemeWindowScrollbarColor = Palette->ScrollbarColor;
    PhThemeWindowFilteredBorderColor = Palette->FilteredBorderColor;
    PhThemeWindowProtectedBorderColor = Palette->ProtectedBorderColor;
    PhThemeWindowFocusBorderColor = Palette->FocusBorderColor;
    PhThemeWindowGroupBoxFrameColor = Palette->GroupBoxFrameColor;
    PhThemeWindowWindowFrameColor = Palette->WindowFrameColor;
    PhThemeWindowEditHotBorderColor = Palette->EditHotBorderColor;
    PhThemeWindowEditNormalBorderColor = Palette->EditNormalBorderColor;
    PhThemeWindowMenuSelectedTextColor = Palette->MenuSelectedTextColor;
    PhThemeWindowMenuDisabledTextColor = Palette->MenuDisabledTextColor;

    previousBrush = PhThemeWindowBackgroundBrush;
    PhThemeWindowBackgroundBrush = CreateSolidBrush(PhThemeWindowBackgroundColor);

    if (previousBrush)
        DeleteBrush(previousBrush);
}

BOOLEAN PhSetWindowThemePalette(
    _In_ PH_WINDOW_THEME_ID ThemeId,
    _In_opt_ const PH_WINDOW_THEME_PALETTE* Palette
    )
{
    const PH_WINDOW_THEME_PALETTE* selectedPalette;

    switch (ThemeId)
    {
    case PhWindowThemeLight:
        selectedPalette = &PhpWindowThemeLightPalette;
        break;
    case PhWindowThemeDark:
        selectedPalette = &PhpWindowThemeDarkPalette;
        break;
    case PhWindowThemeCustom1:
        if (Palette)
            PhpWindowThemeCustom1Palette = *Palette;

        selectedPalette = &PhpWindowThemeCustom1Palette;
        break;
    case PhWindowThemeCustom2:
        if (Palette)
            PhpWindowThemeCustom2Palette = *Palette;

        selectedPalette = &PhpWindowThemeCustom2Palette;
        break;
    case PhWindowThemeSystem:
        PhpResolveSystemPalette(&PhpWindowThemeSystemPalette);
        selectedPalette = &PhpWindowThemeSystemPalette;
        break;
    default:
        return FALSE;
    }

    PhpWindowThemeCurrentId = ThemeId;
    PhpApplyWindowThemePalette(selectedPalette);
    PhpWindowThemePaletteApplied = TRUE;

    return TRUE;
}

BOOLEAN PhSetCurrentWindowTheme(
    _In_ PH_WINDOW_THEME_ID ThemeId,
    _In_opt_ HWND RootWindow
    )
{
    if (!PhSetWindowThemePalette(ThemeId, NULL))
        return FALSE;

    if (RootWindow)
        PhReInitializeWindowTheme(RootWindow);

    return TRUE;
}

const PH_WINDOW_THEME_PALETTE* PhGetWindowThemePalette(
    VOID
    )
{
    return &PhpWindowThemeCurrentPalette;
}

static VOID PhpThemeFillRect(
    _In_ HDC Hdc,
    _In_ PRECT Rect,
    _In_ COLORREF Color
    )
{
    SetDCBrushColor(Hdc, Color);
    FillRect(Hdc, Rect, PhpStockDCBrush);
}

static VOID PhpThemeFrameRect(
    _In_ HDC Hdc,
    _In_ PRECT Rect,
    _In_ COLORREF Color
    )
{
    SetDCBrushColor(Hdc, Color);
    FrameRect(Hdc, Rect, PhpStockDCBrush);
}

//static HDC PhpThemeBeginPaintBuffer(
//    _Out_ PPHP_THEME_PAINT_BUFFER PaintBuffer,
//    _In_ HDC TargetDc,
//    _In_ PRECT Rect
//    )
//{
//    static PH_INITONCE initOnce = PH_INITONCE_INIT;
//    LONG width;
//    LONG height;
//
//    memset(PaintBuffer, 0, sizeof(PHP_THEME_PAINT_BUFFER));
//    PaintBuffer->TargetDc = TargetDc;
//    PaintBuffer->Rect = *Rect;
//
//    if (PhBeginInitOnce(&initOnce))
//    {
//        PhBufferedPaintInit();
//        PhEndInitOnce(&initOnce);
//    }
//
//    PaintBuffer->BufferedPaint = PhBeginBufferedPaint(
//        TargetDc,
//        Rect,
//        BPBF_COMPATIBLEBITMAP,
//        NULL,
//        &PaintBuffer->PaintDc
//        );
//
//    if (PaintBuffer->BufferedPaint)
//        return PaintBuffer->PaintDc;
//
//    width = Rect->right - Rect->left;
//    height = Rect->bottom - Rect->top;
//
//    if (width <= 0 || height <= 0)
//        return NULL;
//
//    PaintBuffer->MemoryDc = CreateCompatibleDC(TargetDc);
//    PaintBuffer->Bitmap = CreateCompatibleBitmap(TargetDc, width, height);
//
//    if (PaintBuffer->MemoryDc && PaintBuffer->Bitmap)
//    {
//        PaintBuffer->OldBitmap = SelectBitmap(PaintBuffer->MemoryDc, PaintBuffer->Bitmap);
//        PaintBuffer->PaintDc = PaintBuffer->MemoryDc;
//        return PaintBuffer->PaintDc;
//    }
//
//    if (PaintBuffer->Bitmap)
//    {
//        DeleteBitmap(PaintBuffer->Bitmap);
//        PaintBuffer->Bitmap = NULL;
//    }
//
//    if (PaintBuffer->MemoryDc)
//    {
//        DeleteDC(PaintBuffer->MemoryDc);
//        PaintBuffer->MemoryDc = NULL;
//    }
//
//    return NULL;
//}
//
//static VOID PhpThemeEndPaintBuffer(
//    _Inout_ PPHP_THEME_PAINT_BUFFER PaintBuffer,
//    _In_ BOOLEAN UpdateTarget
//    )
//{
//    if (PaintBuffer->BufferedPaint)
//    {
//        PhEndBufferedPaint(PaintBuffer->BufferedPaint, UpdateTarget);
//        PaintBuffer->BufferedPaint = NULL;
//        return;
//    }
//
//    if (PaintBuffer->MemoryDc)
//    {
//        if (UpdateTarget)
//        {
//            BitBlt(
//                PaintBuffer->TargetDc,
//                PaintBuffer->Rect.left,
//                PaintBuffer->Rect.top,
//                PaintBuffer->Rect.right - PaintBuffer->Rect.left,
//                PaintBuffer->Rect.bottom - PaintBuffer->Rect.top,
//                PaintBuffer->MemoryDc,
//                0,
//                0,
//                SRCCOPY
//                );
//        }
//
//        if (PaintBuffer->OldBitmap)
//            SelectBitmap(PaintBuffer->MemoryDc, PaintBuffer->OldBitmap);
//    }
//
//    if (PaintBuffer->Bitmap)
//        DeleteBitmap(PaintBuffer->Bitmap);
//
//    if (PaintBuffer->MemoryDc)
//        DeleteDC(PaintBuffer->MemoryDc);
//}

VOID PhInitializeWindowTheme(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN EnableThemeSupport
    )
{
    static PH_INITONCE paletteInitOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&paletteInitOnce))
    {
        // Populate the palette + mirror globals before any paint code runs.
        // When custom theming is disabled, the System palette resolves from
        // GetSysColor so menus, backgrounds and borders inherit the user's
        // current Windows color scheme instead of the dark defaults. Skip this
        // when a palette was already selected (e.g. PhApplyThemeMode at startup)
        // so the chosen Light/Dark/Custom theme is not overwritten.
        if (!PhpWindowThemePaletteApplied)
            PhSetWindowThemePalette(EnableThemeSupport ? PhWindowThemeDark : PhWindowThemeSystem, NULL);
        PhEndInitOnce(&paletteInitOnce);
    }

    if (EnableThemeSupport && WindowsVersion >= WINDOWS_10_RS5)
    {
        static PH_INITONCE initOnce = PH_INITONCE_INIT;

        if (PhBeginInitOnce(&initOnce))
        {
            if (WindowsVersion >= WINDOWS_10_19H2)
            {
                PVOID baseAddress;

                if (!(baseAddress = PhGetLoaderEntryDllBaseZ(L"uxtheme.dll")))
                    baseAddress = PhLoadLibrary(L"uxtheme.dll");

                if (baseAddress)
                {
                    SetPreferredAppMode_I = PhGetDllBaseProcedureAddress(baseAddress, NULL, 135);
                    //FlushMenuThemes_I = PhGetDllBaseProcedureAddress(baseAddress, NULL, 136);
                }

                if (SetPreferredAppMode_I)
                {
                    SetPreferredAppMode_I(PreferredAppModeDarkAlways);
                }

                //if (FlushMenuThemes_I)
                //    FlushMenuThemes_I();
            }

            PhEndInitOnce(&initOnce);
        }
    }

    PhInitializeThemeWindowFrame(WindowHandle);

    if (!PhThemeWindowBackgroundBrush)
    {
        //HBRUSH brush = PhThemeWindowBackgroundBrush;
        PhThemeWindowBackgroundBrush = CreateSolidBrush(PhThemeWindowBackgroundColor);
        //if (brush) DeleteBrush(brush);
    }

    if (!PhpStockDCBrush)
        PhpStockDCBrush = PhGetStockBrush(DC_BRUSH);

    if (EnableThemeSupport)
    {
        WNDPROC defaultWindowProc;

        defaultWindowProc = (WNDPROC)GetWindowLongPtr(WindowHandle, GWLP_WNDPROC);

        if (defaultWindowProc != PhpThemeWindowSubclassProc)
        {
            PhSetWindowContext(WindowHandle, LONG_MAX, defaultWindowProc);
            PhSetWindowProcedure(WindowHandle, PhpThemeWindowSubclassProc);

            if (WindowsVersion >= WINDOWS_10_RS5)
            {
                WCHAR windowClassName[MAX_PATH];
                if (!NT_SUCCESS(PhGetClassName(WindowHandle, windowClassName, RTL_NUMBER_OF(windowClassName), NULL)))
                    windowClassName[0] = UNICODE_NULL;
                if (PhEqualStringZ(windowClassName, L"PhTreeNew", FALSE) || PhEqualStringZ(windowClassName, WC_LISTVIEW, FALSE))
                    PhAllowDarkModeForWindow(WindowHandle, TRUE);   // HACK for dynamically generated plugin tabs
            }
        }

        PhEnumChildWindows(
            WindowHandle,
            PhpThemeWindowEnumChildWindows,
            NULL
            );

        RedrawWindow(WindowHandle, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    }
    else
    {
        //EnableThemeDialogTexture(WindowHandle, ETDT_ENABLETAB);
    }
}

VOID PhUninitializeWindowTheme(
    _In_ HWND WindowHandle
    )
{
    HWND currentWindow = NULL;
    WCHAR windowClassName[MAX_PATH];

    if (!NT_SUCCESS(PhGetClassName(WindowHandle, windowClassName, RTL_NUMBER_OF(windowClassName), NULL)))
        windowClassName[0] = UNICODE_NULL;

    do
    {
        if (currentWindow = FindWindowEx(WindowHandle, currentWindow, NULL, NULL))
        {
            PhUninitializeWindowTheme(currentWindow);
        }
    } while (currentWindow);

    if (PhEqualStringZ(windowClassName, L"PhTreeNew", FALSE))
    {
        TreeNew_ThemeSupport(WindowHandle, FALSE);

        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            HWND tooltipWindow = TreeNew_GetTooltips(WindowHandle);

            PhWindowThemeSetDarkMode(tooltipWindow, FALSE);
            PhWindowThemeSetDarkMode(WindowHandle, FALSE);
            PhAllowDarkModeForWindow(WindowHandle, FALSE);
        }

        PhSetControlTheme(WindowHandle, L"");
    }
    else if (PhEqualStringZ(windowClassName, WC_TABCONTROL, FALSE))
    {
        PPHP_THEME_WINDOW_TAB_CONTEXT context;

        if (context = PhGetWindowContext(WindowHandle, LONG_MAX))
        {
            PhSetWindowProcedure(WindowHandle, context->DefaultWindowProc);
            PhRemoveWindowContext(WindowHandle, LONG_MAX);
            PhFree(context);
        }
    }
    else if (PhEqualStringZ(windowClassName, WC_SCROLLBAR, FALSE))
    {
        PhWindowThemeSetDarkMode(WindowHandle, FALSE);
    }
    else if (PhEqualStringZ(windowClassName, L"PhScrollNew", FALSE))
    {
        PhAllowDarkModeForWindow(WindowHandle, FALSE);
        SendMessage(WindowHandle, WM_THEMECHANGED, 0, 0);
    }

    if (PhGetWindowContext(WindowHandle, LONG_MAX) && GetWindowLongPtr(WindowHandle, GWLP_WNDPROC) == (LONG_PTR)PhpThemeWindowSubclassProc)
    {
        WNDPROC oldWndProc = (WNDPROC)PhGetWindowContext(WindowHandle, LONG_MAX);

        PhSetWindowProcedure(WindowHandle, oldWndProc);
        PhRemoveWindowContext(WindowHandle, LONG_MAX);
    }

    RedrawWindow(WindowHandle, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

// Reads the per-user "AppsUseLightTheme" preference from the registry. This is
// the low-level fallback used when the uxtheme dark-mode exports are missing
// (pre-Win10-RS5). Returns TRUE when apps should use the light theme.
static BOOLEAN PhpQueryWindowsAppsUseLightTheme(
    VOID
    )
{
    static CONST PH_STRINGREF keyPath = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize");
    HANDLE keyHandle;
    BOOLEAN appsUseLightTheme = TRUE;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_CURRENT_USER,
        &keyPath,
        0
        )))
    {
        appsUseLightTheme = !!PhQueryRegistryUlongZ(keyHandle, L"AppsUseLightTheme");
        NtClose(keyHandle);
    }

    return appsUseLightTheme;
}

// Resolves the undocumented uxtheme dark-mode query exports. Safe to call
// independently of PhInitializeWindowTheme (used by PhQueryWindowsUseDarkMode
// during early startup before the main window exists).
static VOID PhpInitializeUxThemeDarkModeExports(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            PVOID baseAddress;

            if (!(baseAddress = PhGetLoaderEntryDllBaseZ(L"uxtheme.dll")))
                baseAddress = PhLoadLibrary(L"uxtheme.dll");

            if (baseAddress)
            {
                ShouldAppsUseDarkMode_I = PhGetDllBaseProcedureAddress(baseAddress, NULL, 132);
                ShouldSystemUseDarkMode_I = PhGetDllBaseProcedureAddress(baseAddress, NULL, 138);
            }
        }

        PhEndInitOnce(&initOnce);
    }
}

// Determines whether application windows should use the dark theme. Prefers the
// uxtheme ShouldAppsUseDarkMode export (reflects the "Choose your default app
// mode" preference) and falls back to the AppsUseLightTheme registry value.
BOOLEAN PhQueryWindowsUseDarkMode(
    VOID
    )
{
    PhpInitializeUxThemeDarkModeExports();

    if (ShouldAppsUseDarkMode_I)
        return !!ShouldAppsUseDarkMode_I();

    return !PhpQueryWindowsAppsUseLightTheme();
}

// Applies a user-facing theme mode (see PH_THEME_MODE) by selecting the
// matching palette. Only meaningful when PhEnableThemeSupport is TRUE; callers
// must honour the master gate. When RootWindow is supplied the change is
// applied live (palette swap + re-theme); otherwise only the palette is
// selected (startup, before the window exists).
VOID PhApplyThemeMode(
    _In_ ULONG Mode,
    _In_opt_ HWND RootWindow
    )
{
    PH_WINDOW_THEME_ID themeId;

    switch (Mode)
    {
    case PhThemeModeLight:
        themeId = PhWindowThemeLight;
        break;
    case PhThemeModeDark:
        themeId = PhWindowThemeDark;
        break;
    case PhThemeModeCustom:
        // The custom palette is zero-initialized until configured; seed it from
        // the dark palette so selecting Custom never yields an unusable UI.
        if (PhpWindowThemeCustom1Palette.BackgroundColor == 0)
            PhpWindowThemeCustom1Palette = PhpWindowThemeDarkPalette;
        themeId = PhWindowThemeCustom1;
        break;
    case PhThemeModeAutomatic:
    default:
        themeId = PhQueryWindowsUseDarkMode() ? PhWindowThemeDark : PhWindowThemeLight;
        break;
    }

    if (RootWindow)
        PhSetCurrentWindowTheme(themeId, RootWindow);
    else
        PhSetWindowThemePalette(themeId, NULL);
}

VOID PhInitializeWindowThemeEx(
    _In_ HWND WindowHandle
    )
{
    PhInitializeWindowTheme(WindowHandle, PhQueryWindowsUseDarkMode());
}

VOID PhReInitializeWindowTheme(
    _In_ HWND WindowHandle
    )
{
    HWND currentWindow = NULL;

    PhInitializeThemeWindowFrame(WindowHandle);

    if (!PhEnableThemeSupport)
        return;

    if (!PhThemeWindowBackgroundBrush)
    {
        //HBRUSH brush = PhThemeWindowBackgroundBrush;
        PhThemeWindowBackgroundBrush = CreateSolidBrush(PhThemeWindowBackgroundColor);
        //if (brush) DeleteBrush(brush);
    }

    PhEnumChildWindows(
        WindowHandle,
        PhpReInitializeThemeWindowEnumChildWindows,
        NULL
        );

    do
    {
        if (currentWindow = FindWindowEx(NULL, currentWindow, NULL, NULL))
        {
            ULONG processID = 0;

            GetWindowThreadProcessId(currentWindow, &processID);

            if (UlongToHandle(processID) == NtCurrentProcessId())
            {
                WCHAR windowClassName[MAX_PATH];

                if (!NT_SUCCESS(PhGetClassName(currentWindow, windowClassName, RTL_NUMBER_OF(windowClassName), NULL)))
                    windowClassName[0] = UNICODE_NULL;

                //dprintf("PhReInitializeWindowTheme: %S\r\n", windowClassName);

                if (currentWindow != WindowHandle)
                {
                    if (PhEqualStringZ(windowClassName, L"#32770", FALSE))
                    {
                        PhEnumChildWindows(
                            currentWindow,
                            PhpReInitializeThemeWindowEnumChildWindows,
                            NULL
                            );
                        //PhReInitializeWindowTheme(currentWindow);
                    }

                    InvalidateRect(currentWindow, NULL, TRUE);
                }
            }
        }
    } while (currentWindow);

    InvalidateRect(WindowHandle, NULL, FALSE);
}

#define DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 19
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

HRESULT PhGetWindowThemeAttribute(
    _In_ HWND WindowHandle,
    _In_ ULONG AttributeId,
    _Out_writes_bytes_(AttributeLength) PVOID Attribute,
    _In_ ULONG AttributeLength
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static HRESULT (WINAPI* DwmGetWindowAttribute_I)(
        _In_ HWND WindowHandle,
        _In_ ULONG AttributeId,
        _Out_writes_bytes_(AttributeLength) PVOID Attribute,
        _In_ ULONG AttributeLength
        );

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"dwmapi.dll"))
        {
            DwmGetWindowAttribute_I = PhGetDllBaseProcedureAddress(baseAddress, "DwmGetWindowAttribute", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!DwmGetWindowAttribute_I)
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    return DwmGetWindowAttribute_I(WindowHandle, AttributeId, Attribute, AttributeLength);
}

HRESULT PhSetWindowThemeAttribute(
    _In_ HWND WindowHandle,
    _In_ ULONG AttributeId,
    _In_reads_bytes_(AttributeLength) PVOID Attribute,
    _In_ ULONG AttributeLength
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static HRESULT (WINAPI* DwmSetWindowAttribute_I)(
        _In_ HWND WindowHandle,
        _In_ ULONG AttributeId,
        _In_reads_bytes_(AttributeLength) PVOID Attribute,
        _In_ ULONG AttributeLength
        );

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"dwmapi.dll"))
        {
            DwmSetWindowAttribute_I = PhGetDllBaseProcedureAddress(baseAddress, "DwmSetWindowAttribute", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!DwmSetWindowAttribute_I)
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    return DwmSetWindowAttribute_I(WindowHandle, AttributeId, Attribute, AttributeLength);
}

HRESULT PhSetWindowBorderColor(
    _In_ HWND WindowHandle,
    _In_ COLORREF Color
    )
{
    return PhSetWindowThemeAttribute(WindowHandle, DWMWA_BORDER_COLOR, &Color, sizeof(COLORREF));
}

COLORREF PhGetWindowBorderColor(
    _In_ BOOLEAN IsActive,
    _In_ BOOLEAN IsHandleFiltered,
    _In_ BOOLEAN IsProtectedProcess,
    _In_ BOOLEAN IsIsolatedUserMode
    )
{
    if (!PhEnableWindowBorderColor)
        return 0;

    if (IsHandleFiltered)
        return PhpWindowThemeCurrentPalette.FilteredBorderColor;

    if (IsProtectedProcess || IsIsolatedUserMode)
        return PhpWindowThemeCurrentPalette.ProtectedBorderColor;

    return IsActive
        ? PhpWindowThemeCurrentPalette.WindowActiveBorderColor
        : PhpWindowThemeCurrentPalette.WindowInactiveBorderColor;
}

COLORREF PhGetWindowActiveBorderColor(
    _In_ BOOLEAN IsActive
    )
{
    return PhGetWindowBorderColor(IsActive, FALSE, FALSE, FALSE);
}

static VOID PhpUpdateThemeWindowBorderColor(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN Active
    )
{
    COLORREF borderColor = PhGetWindowActiveBorderColor(Active);
    if (borderColor)
        PhSetWindowBorderColor(WindowHandle, borderColor);
}

VOID PhInitializeThemeWindowFrame(
    _In_ HWND WindowHandle
    )
{
    if (WindowsVersion >= WINDOWS_10_RS5)
    {
        BOOL boolAttribute;
        ULONG ulongAttribute;

        if (PhEnableThemeSupport)
        {
            PhAllowDarkModeForWindow(WindowHandle, TRUE);
            PhSetControlTheme(WindowHandle, L"DarkMode_Explorer");

            boolAttribute = TRUE;

            if (FAILED(PhSetWindowThemeAttribute(WindowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE, &boolAttribute, sizeof(BOOL))))
            {
                PhSetWindowThemeAttribute(WindowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &boolAttribute, sizeof(BOOL));
            }

            if (WindowsVersion >= WINDOWS_11)
            {
                PhSetWindowThemeAttribute(WindowHandle, DWMWA_CAPTION_COLOR, &PhThemeWindowBackgroundColor, sizeof(COLORREF));
            }
        }

        if (WindowsVersion >= WINDOWS_11_22H2)
        {
            ulongAttribute = 1;
            PhSetWindowThemeAttribute(WindowHandle, DWMWA_SYSTEMBACKDROP_TYPE, &ulongAttribute, sizeof(ULONG));
        }
    }

    PhpUpdateThemeWindowBorderColor(WindowHandle, GetActiveWindow() == WindowHandle);
}

VOID PhWindowThemeSetDarkMode(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN EnableDarkMode
    )
{
    //BOOL boolAttribute;

    if (EnableDarkMode && PhEnableThemeSupport) // ShouldAppsUseDarkMode_I()
    {
        PhSetControlTheme(WindowHandle, L"DarkMode_Explorer");
        //PhSetControlTheme(WindowHandle, L"DarkMode_ItemsView");

        //if (WindowsVersion >= WINDOWS_11)
        //{
        //    boolAttribute = TRUE;
        //
        //    if (FAILED(PhSetWindowThemeAttribute(WindowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE, &boolAttribute, sizeof(BOOL))))
        //    {
        //        PhSetWindowThemeAttribute(WindowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &boolAttribute, sizeof(BOOL));
        //    }
        //}
    }
    else
    {
        PhSetControlTheme(WindowHandle, L"Explorer");
        //PhSetControlTheme(WindowHandle, L"ItemsView");

        //if (WindowsVersion >= WINDOWS_11)
        //{
        //    boolAttribute = FALSE;
        //
        //    if (FAILED(PhSetWindowThemeAttribute(WindowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE, &boolAttribute, sizeof(BOOL))))
        //    {
        //        PhSetWindowThemeAttribute(WindowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &boolAttribute, sizeof(BOOL));
        //    }
        //}
    }
}

HBRUSH PhWindowThemeControlColor(
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ HWND ChildWindowHandle,
    _In_ LONG Type
    )
{
    SetBkMode(Hdc, TRANSPARENT);

    switch (Type)
    {
    case CTLCOLOR_EDIT:
        {
            SetTextColor(Hdc, PhThemeWindowTextColor);
            SetDCBrushColor(Hdc, PHP_THEME_WINDOW_EDIT_COLOR);
            return PhpStockDCBrush;
        }
        break;
    case CTLCOLOR_SCROLLBAR:
        {
            SetDCBrushColor(Hdc, PHP_THEME_WINDOW_SCROLLBAR_COLOR);
            return PhpStockDCBrush;
        }
        break;
    case CTLCOLOR_MSGBOX:
    case CTLCOLOR_LISTBOX:
    case CTLCOLOR_BTN:
    case CTLCOLOR_DLG:
    case CTLCOLOR_STATIC:
        {
            SetTextColor(Hdc, PhThemeWindowTextColor);
            return PhThemeWindowBackgroundBrush;
        }
        break;
    }

    return PhThemeWindowBackgroundBrush;
}

VOID PhWindowThemeMainMenuBorder(
    _In_ HWND WindowHandle
    )
{
    if (GetMenu(WindowHandle))
    {
        RECT clientRect;
        RECT windowRect;
        HDC hdc;

        if (!PhGetClientRect(WindowHandle, &clientRect))
            return;
        if (!PhGetWindowRect(WindowHandle, &windowRect))
            return;

        MapWindowPoints(WindowHandle, NULL, (PPOINT)&clientRect, 2);
        PhOffsetRect(&clientRect, -windowRect.left, -windowRect.top);

        // the rcBar is offset by the window rect (thanks to adzm) (dmex)
        RECT rcAnnoyingLine = clientRect;
        rcAnnoyingLine.bottom = rcAnnoyingLine.top;
        rcAnnoyingLine.top--;

        if (hdc = GetWindowDC(WindowHandle))
        {
            FillRect(hdc, &rcAnnoyingLine, PhThemeWindowBackgroundBrush);

            ReleaseDC(WindowHandle, hdc);
        }
    }
}

VOID PhInitializeThemeWindowTabControl(
    _In_ HWND TabControlWindow
    )
{
    PPHP_THEME_WINDOW_TAB_CONTEXT context;

    context = PhAllocateZero(sizeof(PHP_THEME_WINDOW_TAB_CONTEXT));
    context->DefaultWindowProc = PhGetWindowProcedure(TabControlWindow);
    context->WindowDpi = PhGetWindowDpi(TabControlWindow);
    context->CursorPos.x = LONG_MIN;
    context->CursorPos.y = LONG_MIN;

    SetWindowFont(TabControlWindow, PhApplicationFont, FALSE);

    PhSetWindowContext(TabControlWindow, LONG_MAX, context);
    PhSetWindowProcedure(TabControlWindow, PhpThemeWindowTabControlWndSubclassProc);

    InvalidateRect(TabControlWindow, NULL, FALSE);
}

VOID PhInitializeThemeWindowGroupBox(
    _In_ HWND GroupBoxHandle
    )
{
    WNDPROC groupboxWindowProc;

    groupboxWindowProc = PhGetWindowProcedure(GroupBoxHandle);
    PhSetWindowContext(GroupBoxHandle, LONG_MAX, groupboxWindowProc);
    PhSetWindowProcedure(GroupBoxHandle, PhpThemeWindowGroupBoxSubclassProc);

    PhSetWindowStyle(GroupBoxHandle, WS_CLIPSIBLINGS, WS_CLIPSIBLINGS);

    InvalidateRect(GroupBoxHandle, NULL, FALSE);
}

VOID PhInitializeThemeWindowGroupBoxEx(
    _In_ HWND GroupBoxHandle
    )
{
    WNDPROC groupboxWindowProc;

    groupboxWindowProc = PhGetWindowProcedure(GroupBoxHandle);
    PhSetWindowContext(GroupBoxHandle, LONG_MAX, groupboxWindowProc);
    PhSetWindowProcedure(GroupBoxHandle, PhThemeWindowGroupBoxExSubclassProc);

    PhSetWindowStyle(GroupBoxHandle, WS_CLIPSIBLINGS, WS_CLIPSIBLINGS);

    InvalidateRect(GroupBoxHandle, NULL, FALSE);
}

VOID PhInitializeWindowThemeMainMenu(
    _In_ HMENU MenuHandle
    )
{
    MENUINFO menuInfo;

    memset(&menuInfo, 0, sizeof(MENUINFO));
    menuInfo.cbSize = sizeof(MENUINFO);
    menuInfo.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
    menuInfo.hbrBack = PhThemeWindowBackgroundBrush;

    SetMenuInfo(MenuHandle, &menuInfo);
}

VOID PhInitializeWindowThemeListboxControl(
    _In_ HWND ListBoxControl
    )
{
    PPHP_THEME_WINDOW_STATUSBAR_CONTEXT context;

    context = PhAllocateZero(sizeof(PHP_THEME_WINDOW_STATUSBAR_CONTEXT));
    context->DefaultWindowProc = (WNDPROC)GetWindowLongPtr(ListBoxControl, GWLP_WNDPROC);
    context->WindowDpi = PhGetWindowDpi(ListBoxControl);
    context->CursorPos.x = LONG_MIN;
    context->CursorPos.y = LONG_MIN;

    PhSetWindowContext(ListBoxControl, LONG_MAX, context);
    SetWindowLongPtr(ListBoxControl, GWLP_WNDPROC, (LONG_PTR)PhpThemeWindowListBoxControlSubclassProc);

    InvalidateRect(ListBoxControl, NULL, FALSE);
    SetWindowPos(ListBoxControl, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
}

VOID PhInitializeWindowThemeComboboxControl(
    _In_ HWND ComboBoxControl
    )
{
    PPHP_THEME_WINDOW_COMBO_CONTEXT context;

    context = PhAllocateZero(sizeof(PHP_THEME_WINDOW_COMBO_CONTEXT));
    context->DefaultWindowProc = (WNDPROC)GetWindowLongPtr(ComboBoxControl, GWLP_WNDPROC);
    context->WindowDpi = PhGetWindowDpi(ComboBoxControl);
    context->ThemeHandle = PhOpenThemeData(ComboBoxControl, VSCLASS_COMBOBOX, context->WindowDpi);
    context->CursorPos.x = LONG_MIN;
    context->CursorPos.y = LONG_MIN;

    PhSetWindowContext(ComboBoxControl, LONG_MAX, context);
    SetWindowLongPtr(ComboBoxControl, GWLP_WNDPROC, (LONG_PTR)PhpThemeWindowComboBoxControlSubclassProc);

    InvalidateRect(ComboBoxControl, NULL, FALSE);
}

VOID PhInitializeWindowThemeACLUI(
    _In_ HWND ACLUIControl
)
{
    PhSetWindowContext(ACLUIControl, LONG_MAX, PhGetWindowProcedure(ACLUIControl));
    PhSetWindowProcedure(ACLUIControl, PhpThemeWindowACLUISubclassProc);

    InvalidateRect(ACLUIControl, NULL, FALSE);
}

VOID PhInitializeWindowThemeEditControl(
    _In_ HWND EditControl
    )
{
    PPHP_THEME_WINDOW_EDIT_CONTEXT context;

    context = PhAllocateZero(sizeof(PHP_THEME_WINDOW_EDIT_CONTEXT));
    context->DefaultWindowProc = PhGetWindowProcedure(EditControl);
    context->ParentWindowHandle = GetParent(EditControl);
    context->WindowDpi = PhGetWindowDpi(EditControl);
    context->PreviousFocusWindowHandle = NULL;
    context->WindowFocus = FALSE;

    PhpThemeWindowEditThemeChanged(context, EditControl);

    PhSetWindowContext(EditControl, SHRT_MAX, context);
    PhSetWindowProcedure(EditControl, PhEditBorderWndSubclassProc);

    SetWindowPos(EditControl, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    InvalidateRect(EditControl, NULL, FALSE);
}

_Function_class_(PH_WINDOW_ENUM_CALLBACK)
BOOLEAN CALLBACK PhpThemeWindowEnumChildWindows(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    WCHAR windowClassName[MAX_PATH];

    PhEnumChildWindows(
        WindowHandle,
        PhpThemeWindowEnumChildWindows,
        NULL
        );

    if (PhGetWindowContext(WindowHandle, LONG_MAX)) // HACK
        return TRUE;

    if (!NT_SUCCESS(PhGetClassName(WindowHandle, windowClassName, RTL_NUMBER_OF(windowClassName), NULL)))
        windowClassName[0] = UNICODE_NULL;

    dprintf("PhpThemeWindowEnumChildWindows: %S\r\n", windowClassName);

    if (PhEqualStringZ(windowClassName, L"#32770", TRUE))
    {
        PhInitializeWindowTheme(WindowHandle, TRUE);
    }
    else if (PhEqualStringZ(windowClassName, WC_BUTTON, FALSE))
    {
        ULONG style = PhGetWindowStyle(WindowHandle);

        if ((style & BS_TYPEMASK) == BS_GROUPBOX)
        {
            PhInitializeThemeWindowGroupBox(WindowHandle);
        }
        else    // apply theme for CheckBox, Radio (Dart Vanya)
        {
            PhWindowThemeSetDarkMode(WindowHandle, TRUE);
        }
    }
    else if (PhEqualStringZ(windowClassName, WC_TABCONTROL, FALSE))
    {
        PhInitializeThemeWindowTabControl(WindowHandle);
    }
    else if (PhEqualStringZ(windowClassName, WC_SCROLLBAR, FALSE))
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            PhWindowThemeSetDarkMode(WindowHandle, TRUE);
        }
    }
    else if (PhEqualStringZ(windowClassName, L"PhScrollNew", FALSE))
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            PhAllowDarkModeForWindow(WindowHandle, TRUE);
            SendMessage(WindowHandle, WM_THEMECHANGED, 0, 0);
        }
    }
    else if (PhEqualStringZ(windowClassName, WC_LISTVIEW, FALSE))
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            HWND tooltipWindow = ListView_GetToolTips(WindowHandle);

            PhAllowDarkModeForWindow(WindowHandle, TRUE);
            PhSetControlTheme(WindowHandle, L"DarkMode_ItemsView");
            PhWindowThemeSetDarkMode(tooltipWindow, TRUE);
        }

        if (PhEnableThemeListviewBorder)
        {
            PhSetWindowStyle(WindowHandle, WS_BORDER, WS_BORDER);
            PhSetWindowExStyle(WindowHandle, WS_EX_CLIENTEDGE, WS_EX_CLIENTEDGE);
        }
        else
        {
            PhSetWindowStyle(WindowHandle, WS_BORDER, 0);
            PhSetWindowExStyle(WindowHandle, WS_EX_CLIENTEDGE, 0);
        }

        SetWindowPos(WindowHandle, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

        ListView_SetBkColor(WindowHandle, PhThemeWindowBackgroundColor);
        ListView_SetTextBkColor(WindowHandle, PhThemeWindowBackgroundColor);
        ListView_SetTextColor(WindowHandle, PhThemeWindowTextColor);
    }
    else if (PhEqualStringZ(windowClassName, WC_TREEVIEW, FALSE))
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            HWND tooltipWindow = TreeView_GetToolTips(WindowHandle);

            PhWindowThemeSetDarkMode(WindowHandle, TRUE);
            PhWindowThemeSetDarkMode(tooltipWindow, TRUE);
        }

        TreeView_SetBkColor(WindowHandle, PhThemeWindowBackgroundColor);// RGB(30, 30, 30));
        //TreeView_SetTextBkColor(WindowHandle, RGB(30, 30, 30));
        TreeView_SetTextColor(WindowHandle, PhThemeWindowTextColor);
        //InvalidateRect(WindowHandle, NULL, FALSE);
    }
    else if (PhEqualStringZ(windowClassName, L"RICHEDIT50W", FALSE))
    {
        if (PhEnableThemeListviewBorder)
            PhSetWindowStyle(WindowHandle, WS_BORDER, WS_BORDER);
        else
            PhSetWindowStyle(WindowHandle, WS_BORDER, 0);

        SetWindowPos(WindowHandle, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

        #define EM_SETBKGNDCOLOR (WM_USER + 67)
        SendMessage(WindowHandle, EM_SETBKGNDCOLOR, 0, PhThemeWindowBackgroundColor);
        PhWindowThemeSetDarkMode(WindowHandle, TRUE);
        //InvalidateRect(WindowHandle, NULL, FALSE);
    }
    else if (PhEqualStringZ(windowClassName, L"PhTreeNew", FALSE))
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            HWND tooltipWindow = TreeNew_GetTooltips(WindowHandle);

            PhWindowThemeSetDarkMode(tooltipWindow, TRUE);
            PhWindowThemeSetDarkMode(WindowHandle, TRUE);
            PhAllowDarkModeForWindow(WindowHandle, TRUE);
        }

        if (PhEnableThemeListviewBorder)
            PhSetWindowExStyle(WindowHandle, WS_EX_CLIENTEDGE, WS_EX_CLIENTEDGE);
        else
            PhSetWindowExStyle(WindowHandle, WS_EX_CLIENTEDGE, 0);

        SetWindowPos(WindowHandle, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

        TreeNew_ThemeSupport(WindowHandle, TRUE);

        //InvalidateRect(WindowHandle, NULL, TRUE);
    }
    else if (
        PhEqualStringZ(windowClassName, WC_LISTBOX, FALSE) ||
        PhEqualStringZ(windowClassName, L"ComboLBox", FALSE)
        )
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            PhWindowThemeSetDarkMode(WindowHandle, TRUE);
        }

        PhInitializeWindowThemeListboxControl(WindowHandle);
    }
    else if (PhEqualStringZ(windowClassName, WC_COMBOBOX, FALSE))
    {
        COMBOBOXINFO info = { sizeof(COMBOBOXINFO) };

        if (SendMessage(WindowHandle, CB_GETCOMBOBOXINFO, 0, (LPARAM)&info))
        {
            //if (info.hwndItem)
            //{
            //    SendMessage(info.hwndItem, EM_SETMARGINS, EC_LEFTMARGIN, MAKELPARAM(0, 0));
            //}

            if (info.hwndList)
            {
                PhWindowThemeSetDarkMode(info.hwndList, TRUE);
            }
        }

        //if ((PhGetWindowStyle(WindowHandle) & CBS_DROPDOWNLIST) != CBS_DROPDOWNLIST)
        {
            PhInitializeWindowThemeComboboxControl(WindowHandle);
        }
    }
    else if (PhEqualStringZ(windowClassName, L"CHECKLIST_ACLUI", FALSE))
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            PhWindowThemeSetDarkMode(WindowHandle, TRUE);
        }

        PhInitializeWindowThemeACLUI(WindowHandle);
    }
    else if (PhEqualStringZ(windowClassName, WC_EDIT, FALSE))
    {
        // Fix scrollbar on multiline edit (Dart Vanya)
        if (PhGetWindowStyle(WindowHandle) & ES_MULTILINE)
        {
            PhWindowThemeSetDarkMode(WindowHandle, TRUE);
        }
    }
    else if (PhEqualStringZ(windowClassName, WC_LINK, FALSE))
    {
        // SysLink theme support (Dart Vanya)
        PhAllowDarkModeForWindow(WindowHandle, TRUE);
    }

    return TRUE;
}

_Function_class_(PH_WINDOW_ENUM_CALLBACK)
BOOLEAN CALLBACK PhpReInitializeThemeWindowEnumChildWindows(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    WCHAR windowClassName[MAX_PATH];

    PhEnumChildWindows(
        WindowHandle,
        PhpReInitializeThemeWindowEnumChildWindows,
        NULL
        );

    if (!NT_SUCCESS(PhGetClassName(WindowHandle, windowClassName, RTL_NUMBER_OF(windowClassName), NULL)))
        windowClassName[0] = UNICODE_NULL;

    if (PhEqualStringZ(windowClassName, WC_LISTVIEW, FALSE))
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            PhWindowThemeSetDarkMode(WindowHandle, TRUE);
        }

        ListView_SetBkColor(WindowHandle, PhThemeWindowBackgroundColor);
        ListView_SetTextBkColor(WindowHandle, PhThemeWindowBackgroundColor);
        ListView_SetTextColor(WindowHandle, PhThemeWindowTextColor);
    }
    else if (PhEqualStringZ(windowClassName, WC_SCROLLBAR, FALSE))
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            PhWindowThemeSetDarkMode(WindowHandle, TRUE);
        }
    }
    else if (PhEqualStringZ(windowClassName, L"PhTreeNew", FALSE))
    {
        TreeNew_ThemeSupport(WindowHandle, TRUE);
        PhWindowThemeSetDarkMode(WindowHandle, TRUE);
    }
    else if (PhEqualStringZ(windowClassName, WC_EDIT, FALSE))
    {
        SendMessage(WindowHandle, WM_THEMECHANGED, 0, 0); // searchbox.c
    }

    InvalidateRect(WindowHandle, NULL, TRUE);

    return TRUE;
}

BOOLEAN PhThemeWindowDrawItem(
    _In_ HWND WindowHandle,
    _In_ PDRAWITEMSTRUCT DrawInfo
    )
{
    BOOLEAN isGrayed = (DrawInfo->itemState & CDIS_GRAYED) == CDIS_GRAYED;
    BOOLEAN isChecked = (DrawInfo->itemState & CDIS_CHECKED) == CDIS_CHECKED;
    BOOLEAN isDisabled = (DrawInfo->itemState & CDIS_DISABLED) == CDIS_DISABLED;
    BOOLEAN isSelected = (DrawInfo->itemState & CDIS_SELECTED) == CDIS_SELECTED;
    //BOOLEAN isHighlighted = (DrawInfo->itemState & CDIS_HOT) == CDIS_HOT;
    BOOLEAN isFocused = (DrawInfo->itemState & CDIS_FOCUS) == CDIS_FOCUS;
    //BOOLEAN isGrayed = (DrawInfo->itemState & ODS_GRAYED) == ODS_GRAYED;
    //BOOLEAN isChecked = (DrawInfo->itemState & ODS_CHECKED) == ODS_CHECKED;
    //BOOLEAN isDisabled = (DrawInfo->itemState & ODS_DISABLED) == ODS_DISABLED;
    //BOOLEAN isSelected = (DrawInfo->itemState & ODS_SELECTED) == ODS_SELECTED;
    BOOLEAN isHighlighted = (DrawInfo->itemState & ODS_HOTLIGHT) == ODS_HOTLIGHT;

    SetBkMode(DrawInfo->hDC, TRANSPARENT);

    switch (DrawInfo->CtlType)
    {
    case ODT_MENU:
        {
            PPH_EMENU_ITEM menuItemInfo = (PPH_EMENU_ITEM)DrawInfo->itemData;
            RECT rect = DrawInfo->rcItem;
            LONG dpiValue = PhGetWindowDpi(WindowHandle);
            ULONG drawTextFlags = DT_SINGLELINE | DT_NOCLIP;
            //HFONT fontHandle;
            //HFONT oldFont = NULL;

            if (DrawInfo->itemState & ODS_NOACCEL)
            {
                drawTextFlags |= DT_HIDEPREFIX;
            }

            //if (fontHandle = PhCreateMessageFont(dpiValue))
            //{
            //    oldFont = SelectFont(DrawInfo->hDC, fontHandle);
            //}
            //
            //FillRect(
            //    DrawInfo->hDC,
            //    &DrawInfo->rcItem,
            //    CreateSolidBrush(RGB(0, 0, 0))
            //    );
            //SetTextColor(DrawInfo->hDC, RGB(0xff, 0xff, 0xff));

            if (DrawInfo->itemState & ODS_HOTLIGHT)
            {
                SetTextColor(DrawInfo->hDC, PhThemeWindowTextColor);
                SetDCBrushColor(DrawInfo->hDC, PhThemeWindowHighlightColor);
                FillRect(DrawInfo->hDC, &DrawInfo->rcItem, PhpStockDCBrush);
            }
            else if (isDisabled)
            {
                SetTextColor(DrawInfo->hDC, PhThemeWindowMenuDisabledTextColor);
                FillRect(DrawInfo->hDC, &DrawInfo->rcItem, PhThemeWindowBackgroundBrush);
            }
            else if (isSelected)
            {
                SetTextColor(DrawInfo->hDC, PhThemeWindowTextColor);
                SetDCBrushColor(DrawInfo->hDC, PhThemeWindowHighlightColor);
                FillRect(DrawInfo->hDC, &DrawInfo->rcItem, PhpStockDCBrush);
            }
            else
            {
                SetTextColor(DrawInfo->hDC, PhThemeWindowMenuSelectedTextColor);
                FillRect(DrawInfo->hDC, &DrawInfo->rcItem, PhThemeWindowBackgroundBrush);
            }

            if (isChecked)
            {
                static CONST PH_STRINGREF menuCheckText = PH_STRINGREF_INIT(L"\u2713");
                COLORREF oldTextColor;

                //HFONT marlettFontHandle = CreateFont(
                //    0, 0, 0, 0,
                //    FW_DONTCARE,
                //    FALSE,
                //    FALSE,
                //    FALSE,
                //    DEFAULT_CHARSET,
                //    OUT_OUTLINE_PRECIS,
                //    CLIP_DEFAULT_PRECIS,
                //    CLEARTYPE_QUALITY,
                //    VARIABLE_PITCH,
                //    L"Arial Unicode MS"
                //    );

                oldTextColor = SetTextColor(DrawInfo->hDC, PhThemeWindowTextColor);

                DrawInfo->rcItem.left += PhScaleToDisplay(8, dpiValue);
                DrawInfo->rcItem.top += PhScaleToDisplay(3, dpiValue);
                DrawText(
                    DrawInfo->hDC,
                    menuCheckText.Buffer,
                    (UINT)menuCheckText.Length / sizeof(WCHAR),
                    &DrawInfo->rcItem,
                    DT_VCENTER | DT_NOCLIP
                    );
                DrawInfo->rcItem.left -= PhScaleToDisplay(8, dpiValue);
                DrawInfo->rcItem.top -= PhScaleToDisplay(3, dpiValue);

                SetTextColor(DrawInfo->hDC, oldTextColor);
            }

            if (menuItemInfo->Flags & PH_EMENU_SEPARATOR)
            {
                FillRect(DrawInfo->hDC, &DrawInfo->rcItem, PhThemeWindowBackgroundBrush);

                //DrawInfo->rcItem.top += PhScaleToDisplay(1, dpiValue);
                //DrawInfo->rcItem.bottom -= PhScaleToDisplay(2, dpiValue);
                //DrawFocusRect(drawInfo->hDC, &drawInfo->rcItem);

                // +5 font margin, +1 extra padding
                //INT cxMenuCheck = GetSystemMetrics(SM_CXMENUCHECK) + (GetSystemMetrics(SM_CXEDGE) * 2) + 5 + 1;
                INT cyEdge = PhGetSystemMetrics(SM_CYEDGE, dpiValue);
                //
                //SetRect(
                //    &DrawInfo->rcItem,
                //    DrawInfo->rcItem.left + cxMenuCheck, // 25
                //    DrawInfo->rcItem.top + cyEdge,
                //    DrawInfo->rcItem.right,
                //    DrawInfo->rcItem.bottom - cyEdge
                //    );

                SetDCBrushColor(DrawInfo->hDC, PHP_THEME_WINDOW_BORDER_COLOR);
                SelectBrush(DrawInfo->hDC, PhpStockDCBrush);
                PatBlt(DrawInfo->hDC, DrawInfo->rcItem.left, DrawInfo->rcItem.top + cyEdge, DrawInfo->rcItem.right - DrawInfo->rcItem.left, 1, PATCOPY);

                //DrawEdge(drawInfo->hDC, &drawInfo->rcItem, BDR_RAISEDINNER, BF_TOP);
            }
            else
            {
                PH_STRINGREF part = { 0 };
                PH_STRINGREF firstPart = { 0 };
                PH_STRINGREF secondPart = { 0 };

                PhInitializeStringRefLongHint(&part, menuItemInfo->Text);
                PhSplitStringRefAtLastChar(&part, L'\b', &firstPart, &secondPart);

                //SetDCBrushColor(DrawInfo->hDC, PhThemeWindowForegroundColor);
                //FillRect(DrawInfo->hDC, &DrawInfo->rcItem, PhGetStockBrush(DC_BRUSH));

                if (menuItemInfo->Bitmap)
                {
                    HDC bufferDc;
                    BITMAP bitmapInfo;
                    LONG bitmapWidth;
                    LONG bitmapHeight;
                    BLENDFUNCTION blendFunction;

                    blendFunction.BlendOp = AC_SRC_OVER;
                    blendFunction.BlendFlags = 0;
                    blendFunction.SourceConstantAlpha = 255;
                    blendFunction.AlphaFormat = AC_SRC_ALPHA;

                    if (GetObject(menuItemInfo->Bitmap, sizeof(bitmapInfo), &bitmapInfo))
                    {
                        bitmapWidth = bitmapInfo.bmWidth;
                        bitmapHeight = bitmapInfo.bmHeight;
                    }
                    else
                    {
                        bitmapWidth = PhGetSystemMetrics(SM_CXSMICON, dpiValue);
                        bitmapHeight = PhGetSystemMetrics(SM_CYSMICON, dpiValue);
                    }

                    bufferDc = CreateCompatibleDC(DrawInfo->hDC);
                    SelectBitmap(bufferDc, menuItemInfo->Bitmap);

                    GdiAlphaBlend(
                        DrawInfo->hDC,
                        DrawInfo->rcItem.left + 4,
                        DrawInfo->rcItem.top + 4,
                        bitmapWidth,
                        bitmapHeight,
                        bufferDc,
                        0,
                        0,
                        bitmapWidth,
                        bitmapHeight,
                        blendFunction
                        );

                    DeleteDC(bufferDc);
                }

                DrawInfo->rcItem.left += PhScaleToDisplay(25, dpiValue);
                DrawInfo->rcItem.right -= PhScaleToDisplay(25, dpiValue);

                if ((menuItemInfo->Flags & PH_EMENU_MAINMENU) == PH_EMENU_MAINMENU)
                {
                    if (firstPart.Length)
                    {
                        DrawText(
                            DrawInfo->hDC,
                            firstPart.Buffer,
                            (UINT)firstPart.Length / sizeof(WCHAR),
                            &DrawInfo->rcItem,
                            DT_LEFT | DT_SINGLELINE | DT_CENTER | DT_VCENTER | drawTextFlags
                            );
                    }
                }
                else
                {
                    if (firstPart.Length)
                    {
                        DrawText(
                            DrawInfo->hDC,
                            firstPart.Buffer,
                            (UINT)firstPart.Length / sizeof(WCHAR),
                            &DrawInfo->rcItem,
                            DT_LEFT | DT_VCENTER | drawTextFlags
                            );
                    }
                }

                if (secondPart.Length)
                {
                    DrawText(
                        DrawInfo->hDC,
                        secondPart.Buffer,
                        (UINT)secondPart.Length / sizeof(WCHAR),
                        &DrawInfo->rcItem,
                        DT_RIGHT | DT_VCENTER | drawTextFlags
                        );
                }
            }

            //if (oldFont)
            //{
            //    SelectFont(DrawInfo->hDC, oldFont);
            //}

            if (menuItemInfo->Items && menuItemInfo->Items->Count && (menuItemInfo->Flags & PH_EMENU_MAINMENU) != PH_EMENU_MAINMENU)
            {
                HTHEME themeHandle;

                if (themeHandle = PhOpenThemeData(DrawInfo->hwndItem, VSCLASS_MENU, dpiValue))
                {
                    //if (IsThemeBackgroundPartiallyTransparent(themeHandle, MENU_POPUPSUBMENU, isDisabled ? MSM_DISABLED : MSM_NORMAL))
                    //    DrawThemeParentBackground(DrawInfo->hwndItem, DrawInfo->hDC, NULL);

                    rect.left = rect.right - PhScaleToDisplay(25, dpiValue);

                    PhDrawThemeBackground(
                        themeHandle,
                        DrawInfo->hDC,
                        MENU_POPUPSUBMENU,
                        isDisabled ? MSM_DISABLED : MSM_NORMAL,
                        &rect,
                        NULL
                        );

                    PhCloseThemeData(themeHandle);
                }
            }

            ExcludeClipRect(DrawInfo->hDC, rect.left, rect.top, rect.right, rect.bottom); // exclude last

            //if (fontHandle)
            //{
            //    DeleteFont(fontHandle);
            //}

            return TRUE;
        }
    case ODT_COMBOBOX:
        {
            SetTextColor(DrawInfo->hDC, PhThemeWindowMenuSelectedTextColor);
            SetDCBrushColor(DrawInfo->hDC, PhThemeWindowForegroundColor);
            FillRect(DrawInfo->hDC, &DrawInfo->rcItem, PhpStockDCBrush);

            INT length = ComboBox_GetLBTextLen(DrawInfo->hwndItem, DrawInfo->itemID);

            if (length == CB_ERR)
                break;

            if (length < MAX_PATH)
            {
                WCHAR comboText[MAX_PATH] = L"";

                if (ComboBox_GetLBText(DrawInfo->hwndItem, DrawInfo->itemID, comboText) == CB_ERR)
                    break;

                DrawText(
                    DrawInfo->hDC,
                    comboText,
                    (UINT)PhCountStringZ(comboText),
                    &DrawInfo->rcItem,
                    DT_LEFT | DT_END_ELLIPSIS | DT_SINGLELINE
                    );
            }

            return TRUE;
        }
        break;
    }

    return FALSE;
}

BOOLEAN PhThemeWindowMeasureItem(
    _In_ HWND WindowHandle,
    _In_ PMEASUREITEMSTRUCT DrawInfo
    )
{
    if (DrawInfo->CtlType == ODT_MENU)
    {
        PPH_EMENU_ITEM menuItemInfo = (PPH_EMENU_ITEM)DrawInfo->itemData;
        LONG dpiValue = PhGetWindowDpi(WindowHandle);

        DrawInfo->itemWidth = PhScaleToDisplay(100, dpiValue);
        DrawInfo->itemHeight = PhScaleToDisplay(100, dpiValue);

        if ((menuItemInfo->Flags & PH_EMENU_SEPARATOR) == PH_EMENU_SEPARATOR)
        {
            DrawInfo->itemHeight = PhGetSystemMetrics(SM_CYMENU, dpiValue) >> 2;
        }
        else if (menuItemInfo->Text)
        {
            //HFONT fontHandle;
            HDC hdc;

            //fontHandle = PhCreateMessageFont(dpiValue);

            if (hdc = GetDC(WindowHandle))
            {
                PCWSTR text;
                SIZE_T textCount;
                SIZE textSize;
                //HFONT oldFont = NULL;
                INT cyborder = PhGetSystemMetrics(SM_CYBORDER, dpiValue);
                INT cymenu = PhGetSystemMetrics(SM_CYMENU, dpiValue);

                text = menuItemInfo->Text;
                textCount = PhCountStringZ(text);

                //if (fontHandle)
                //{
                //    oldFont = SelectFont(hdc, fontHandle);
                //}

                if ((menuItemInfo->Flags & PH_EMENU_MAINMENU) == PH_EMENU_MAINMENU)
                {
                    if (GetTextExtentPoint32(hdc, text, (ULONG)textCount, &textSize))
                    {
                        DrawInfo->itemWidth = textSize.cx + (cyborder * 2);
                        DrawInfo->itemHeight = cymenu + (cyborder * 2) + 1;
                    }
                }
                else
                {
                    if (GetTextExtentPoint32(hdc, text, (ULONG)textCount, &textSize))
                    {
                        DrawInfo->itemWidth = textSize.cx + (cyborder * 2) + PhScaleToDisplay(90, dpiValue); // HACK
                        DrawInfo->itemHeight = cymenu + (cyborder * 2) + PhScaleToDisplay(1, dpiValue);
                    }
                }

                //if (oldFont)
                //{
                //    SelectFont(hdc, oldFont);
                //}

                ReleaseDC(WindowHandle, hdc);
            }

            //if (fontHandle)
            //{
            //    DeleteFont(fontHandle);
            //}
        }

        return TRUE;
    }

    return FALSE;
}

// TODO: Use imagelist instead of loading images from uxtheme.
//HIMAGELIST CreateTreeViewCheckBoxes(HWND hwnd, int cx, int cy)
//{
//    const int frames = 6;
//
//    // Get a DC for our window.
//    HDC hdcScreen = GetDC(hwnd);
//
//    // Get a button theme for the window, if available.
//    HTHEME htheme = OpenThemeData(hwnd, L"button");
//
//    // If there is a theme, then ask it for the size
//    // of a checkbox and use that size.
//    if (htheme)
//    {
//        SIZE size;
//        PhGetThemePartSize(htheme, hdcScreen, BP_CHECKBOX, CBS_UNCHECKEDNORMAL, NULL, TS_DRAW, &size);
//        cx = size.cx;
//        cy = size.cy;
//    }
//
//    // Create a 32bpp bitmap that holds the desired number of frames.
//    BITMAPINFO bi = { sizeof(BITMAPINFOHEADER), cx * frames, cy, 1, 32 };
//    void* p;
//    HBITMAP hbmCheckboxes = CreateDIBSection(hdcScreen, &bi, DIB_RGB_COLORS, &p, NULL, 0);
//
//    // Create a compatible memory DC.
//    HDC hdcMem = CreateCompatibleDC(hdcScreen);
//
//    // Select our bitmap into it so we can draw to it.
//    HBITMAP hbmOld = SelectBitmap(hdcMem, hbmCheckboxes);
//
//    // Set up the rectangle into which we do our drawing.
//    RECT rc = { 0, 0, cx, cy };
//
//    // Frame 0 is not used. Draw nothing.
//    PhOffsetRect(&rc, cx, 0);
//
//    if (htheme)
//    {
//        // Frame 1: Unchecked.
//        PhDrawThemeBackground(htheme, hdcMem, BP_CHECKBOX, CBS_UNCHECKEDNORMAL, &rc, NULL);
//        PhOffsetRect(&rc, cx, 0);
//
//        // Frame 2: Checked.
//        PhDrawThemeBackground(htheme, hdcMem, BP_CHECKBOX, CBS_CHECKEDNORMAL, &rc, NULL);
//        PhOffsetRect(&rc, cx, 0);
//
//        // Frame 3: Indeterminate.
//        PhDrawThemeBackground(htheme, hdcMem, BP_CHECKBOX, CBS_MIXEDNORMAL, &rc, NULL);
//        PhOffsetRect(&rc, cx, 0);
//
//        // Frame 4: Disabled, unchecked.
//        PhDrawThemeBackground(htheme, hdcMem, BP_CHECKBOX, CBS_UNCHECKEDDISABLED, &rc, NULL);
//        PhOffsetRect(&rc, cx, 0);
//
//        // Frame 5: Disabled, checked.
//        PhDrawThemeBackground(htheme, hdcMem, BP_CHECKBOX, CBS_CHECKEDDISABLED, &rc, NULL);
//
//        // Done with the theme.
//        PhCloseThemeData(htheme);
//    }
//    else
//    {
//        // Flags common to all of our DrawFrameControl calls:
//        // Draw a flat checkbox.
//        UINT baseFlags = DFCS_FLAT | DFCS_BUTTONCHECK;
//
//        // Frame 1: Unchecked.
//        DrawFrameControl(hdcMem, &rc, DFC_BUTTON, baseFlags);
//        PhOffsetRect(&rc, cx, 0);
//
//        // Frame 2: Checked.
//        DrawFrameControl(hdcMem, &rc, DFC_BUTTON, baseFlags | DFCS_CHECKED);
//        PhOffsetRect(&rc, cx, 0);
//
//        // Frame 3: Indeterminate.
//        DrawFrameControl(hdcMem, &rc, DFC_BUTTON, baseFlags | DFCS_CHECKED | DFCS_BUTTON3STATE);
//        PhOffsetRect(&rc, cx, 0);
//
//        // Frame 4: Disabled, unchecked.
//        DrawFrameControl(hdcMem, &rc, DFC_BUTTON, baseFlags | DFCS_INACTIVE);
//        PhOffsetRect(&rc, cx, 0);
//
//        // Frame 5: Disabled, checked.
//        DrawFrameControl(hdcMem, &rc, DFC_BUTTON, baseFlags | DFCS_INACTIVE | DFCS_CHECKED);
//    }
//
//    // The bitmap is ready. Clean up.
//    SelectBitmap(hdcMem, hbmOld);
//    DeleteDC(hdcMem);
//    ReleaseDC(hwnd, hdcScreen);
//
//    // Create an imagelist from this bitmap.
//    HIMAGELIST himl = PhImageListCreate(cx, cy, ILC_COLOR, frames, frames);
//    PhImageListAddBitmap(himl, hbmCheckboxes, NULL);
//
//    // Don't need the bitmap any more.
//    DeleteObject(hbmCheckboxes);
//
//    return himl;
//}
//
//VOID OnThemeChange(HWND hwnd) // WM_THEMECHANGE
//{
//    // Rebuild the state images to match the new theme.
//    HIMAGELIST himl = CreateTreeViewCheckBoxes(g_hwndChild, 16, 16);
//    ImageList_Destroy(TreeView_SetImageList(g_hwndChild, himl, TVSIL_STATE));
//}

VOID PhThemeDrawButtonIcon(
    _In_ LPNMCUSTOMDRAW DrawInfo,
    _In_ HICON ButtonIcon,
    _In_ PRECT ButtonRect,
    _In_ LONG WindowDpi
    )
{
    BOOL result;
    ICONINFO iconInfo;
    BITMAP bmp;
    LONG width = PhGetSystemMetrics(SM_CXSMICON, WindowDpi);
    LONG height = PhGetSystemMetrics(SM_CYSMICON, WindowDpi);

    memset(&iconInfo, 0, sizeof(ICONINFO));
    memset(&bmp, 0, sizeof(BITMAP));

    result = GetIconInfo(ButtonIcon, &iconInfo);

    if (result)
    {
        if (iconInfo.hbmColor)
        {
            if (GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmp))
            {
                width = bmp.bmWidth;
                height = bmp.bmHeight;
            }
        }
        else if (iconInfo.hbmMask)
        {
            if (GetObject(iconInfo.hbmMask, sizeof(BITMAP), &bmp))
            {
                width = bmp.bmWidth;
                height = bmp.bmHeight / 2;
            }
        }

        if (iconInfo.hbmColor)
            DeleteBitmap(iconInfo.hbmColor);
        if (iconInfo.hbmMask)
            DeleteBitmap(iconInfo.hbmMask);
    }

    DrawIconEx(
        DrawInfo->hdc,
        ButtonRect->left + ((ButtonRect->right - ButtonRect->left) - width) / 2,
        ButtonRect->top + ((ButtonRect->bottom - ButtonRect->top) - height) / 2,
        ButtonIcon,
        width,
        height,
        0,
        NULL,
        DI_NORMAL
        );

    if (!result) // HACK
    {
        BUTTON_IMAGELIST imageList = { 0 };

        if (Button_GetImageList(DrawInfo->hdr.hwndFrom, &imageList) && imageList.himl)
        {
            ButtonRect->left += PhScaleToDisplay(1, WindowDpi);

            PhImageListDrawIcon(
                imageList.himl,
                0,
                DrawInfo->hdc,
                ButtonRect->left, // + ((ButtonRect->right - ButtonRect->left) - width) / 2,
                ButtonRect->top + ((ButtonRect->bottom - ButtonRect->top) - height) / 2,
                ILD_NORMAL,
                FALSE
                );

            ButtonRect->left += PhScaleToDisplay(5, WindowDpi);
        }
    }
}

LRESULT CALLBACK PhThemeWindowDrawButton(
    _In_ LPNMCUSTOMDRAW DrawInfo
    )
{
    ULONG buttonStyle;

    buttonStyle = PhGetWindowStyle(DrawInfo->hdr.hwndFrom);
    // COMMANDLINK unsupported
    if ((buttonStyle & BS_COMMANDLINK) == BS_COMMANDLINK || (buttonStyle & BS_DEFCOMMANDLINK) == BS_DEFCOMMANDLINK)
        return CDRF_DODEFAULT;

    BOOLEAN isGrayed = (DrawInfo->uItemState & CDIS_GRAYED) == CDIS_GRAYED;
    BOOLEAN isChecked = (DrawInfo->uItemState & CDIS_CHECKED) == CDIS_CHECKED;
    BOOLEAN isMixed = (DrawInfo->uItemState & CDIS_INDETERMINATE) == CDIS_INDETERMINATE;
    BOOLEAN isDisabled = (DrawInfo->uItemState & CDIS_DISABLED) == CDIS_DISABLED;
    BOOLEAN isSelected = (DrawInfo->uItemState & CDIS_SELECTED) == CDIS_SELECTED;
    BOOLEAN isHighlighted = (DrawInfo->uItemState & CDIS_HOT) == CDIS_HOT;
    BOOLEAN isFocused = (DrawInfo->uItemState & CDIS_FOCUS) == CDIS_FOCUS;
    BOOLEAN isKeyboardFocused = isFocused && (DrawInfo->uItemState & CDIS_SHOWKEYBOARDCUES) == CDIS_SHOWKEYBOARDCUES;
    RECT bufferRect =
    {
        0, 0,
        DrawInfo->rc.right - DrawInfo->rc.left,
        DrawInfo->rc.bottom - DrawInfo->rc.top
    };

    switch (DrawInfo->dwDrawStage)
    {
    case CDDS_PREPAINT:
        {
            PPH_STRING buttonText;
            HICON buttonIcon;
            LONG dpiValue;

            ULONG buttonType = buttonStyle & BS_TYPEMASK;
            BOOLEAN isCheckbox = buttonType == BS_AUTOCHECKBOX || buttonType == BS_CHECKBOX || buttonType == BS_AUTO3STATE || buttonType == BS_3STATE;
            BOOLEAN isRadio = buttonType == BS_AUTORADIOBUTTON || buttonType == BS_RADIOBUTTON;

            if (!isCheckbox && !isRadio && PhEnableThemeNativeButtons && !PhEnableThemeAcrylicWindowSupport)
                return CDRF_DODEFAULT;

            dpiValue = PhGetWindowDpi(DrawInfo->hdr.hwndFrom);
            buttonText = PhGetWindowText(DrawInfo->hdr.hwndFrom);

            if (!(buttonIcon = Static_GetIcon(DrawInfo->hdr.hwndFrom, 0)))
                buttonIcon = (HICON)SendMessage(DrawInfo->hdr.hwndFrom, BM_GETIMAGE, IMAGE_ICON, 0);

            // Add support for disabled and tristate checkboxes, support for radio with multiline (ex. TaskDialog) (Dart Vanya)
            if (isCheckbox || isRadio)
            {
                INT state = isCheckbox ? CBS_UNCHECKEDNORMAL : RBS_UNCHECKEDNORMAL;
                HTHEME themeHandle;

                isChecked = Button_GetCheck(DrawInfo->hdr.hwndFrom) & BST_CHECKED;
                isMixed =  Button_GetCheck(DrawInfo->hdr.hwndFrom) & BST_INDETERMINATE;

                if (isCheckbox)
                {
                    if (isDisabled)
                        state = isChecked ? CBS_CHECKEDDISABLED : isMixed ? CBS_MIXEDDISABLED : CBS_UNCHECKEDDISABLED;
                    else if (isSelected)
                        state = isChecked ? CBS_CHECKEDPRESSED : isMixed ? CBS_MIXEDPRESSED : CBS_UNCHECKEDPRESSED;
                    else if (isHighlighted)
                        state = isChecked ? CBS_CHECKEDHOT : isMixed ? CBS_MIXEDHOT : CBS_UNCHECKEDHOT;
                    else
                        state = isChecked ? CBS_CHECKEDNORMAL : isMixed ? CBS_MIXEDNORMAL : CBS_UNCHECKEDNORMAL;
                }
                else
                {
                    if (isDisabled)
                        state = isChecked ? RBS_CHECKEDDISABLED : RBS_UNCHECKEDDISABLED;
                    else if (isSelected)
                        state = isChecked ? RBS_CHECKEDPRESSED : RBS_UNCHECKEDPRESSED;
                    else if (isHighlighted)
                        state = isChecked ? RBS_CHECKEDHOT : RBS_UNCHECKEDHOT;
                    else
                        state = isChecked ? RBS_CHECKEDNORMAL : RBS_UNCHECKEDNORMAL;
                }

                if (buttonIcon)
                {
                    if (isSelected || isChecked)
                    {
                        SetTextColor(DrawInfo->hdc, PhThemeWindowTextColor);
                        PhpThemeFillRect(DrawInfo->hdc, &DrawInfo->rc, PHP_THEME_WINDOW_PRESSED_COLOR);
                    }
                    else if (isHighlighted)
                    {
                        SetTextColor(DrawInfo->hdc, PhThemeWindowTextColor);
                        PhpThemeFillRect(DrawInfo->hdc, &DrawInfo->rc, PhThemeWindowBackground2Color);
                    }
                    else
                    {
                        SetTextColor(DrawInfo->hdc, !isDisabled ? PhThemeWindowTextColor : PHP_THEME_WINDOW_DISABLED_TEXT_COLOR);
                        //SetDCBrushColor(DrawInfo->hdc, PhThemeWindowBackgroundColor); // WindowForegroundColor
                        FillRect(DrawInfo->hdc, &DrawInfo->rc, PhThemeWindowBackgroundBrush);
                    }

                    PhpThemeFrameRect(DrawInfo->hdc, &DrawInfo->rc, PhThemeWindowBackground2Color);

                    PhThemeDrawButtonIcon(DrawInfo, buttonIcon, &bufferRect, dpiValue);
                }
                else
                {
                    SetBkMode(DrawInfo->hdc, TRANSPARENT);
                    SetTextColor(DrawInfo->hdc, !isDisabled ? PhThemeWindowTextColor : PHP_THEME_WINDOW_DISABLED_TEXT_COLOR);

                    if (themeHandle = PhOpenThemeData(DrawInfo->hdr.hwndFrom, VSCLASS_BUTTON, dpiValue))
                    {
                        SIZE checkBoxSize = { 0 };
                        SIZE textSize = { 0 };
                        INT linesCount;

                        PhGetThemePartSize(
                            themeHandle,
                            DrawInfo->hdc,
                            isCheckbox ? BP_CHECKBOX : BP_RADIOBUTTON,
                            state,
                            &bufferRect,
                            THEMEPARTSIZE_TRUE,
                            &checkBoxSize
                            );
                        GetTextExtentPoint32W(DrawInfo->hdc, L"T", 1, &textSize);

                        bufferRect.left = 0;
                        bufferRect.right = checkBoxSize.cx;
                        linesCount = (bufferRect.bottom - bufferRect.top) / textSize.cy;
                        if (linesCount > 1)
                            bufferRect.bottom -= textSize.cy * (linesCount - 1);    // HACK (very sensitive value)

                        //if (IsThemeBackgroundPartiallyTransparent(themeHandle, isCheckbox ? BP_CHECKBOX : BP_RADIOBUTTON, state))
                        //    DrawThemeParentBackground(DrawInfo->hdr.hwndFrom, DrawInfo->hdc, NULL);

                        PhDrawThemeBackground(
                            themeHandle,
                            DrawInfo->hdc,
                            isCheckbox ? BP_CHECKBOX : BP_RADIOBUTTON,
                            state,
                            &bufferRect,
                            NULL
                            );

                        bufferRect = DrawInfo->rc;
                        bufferRect.left = checkBoxSize.cx + 4; // TNP_ICON_RIGHT_PADDING

                        if (linesCount == 1)
                        {
                            DrawText(
                                DrawInfo->hdc,
                                buttonText->Buffer,
                                (UINT)buttonText->Length / sizeof(WCHAR),
                                &bufferRect,
                                DT_LEFT | DT_SINGLELINE | DT_VCENTER | (!isKeyboardFocused ? DT_HIDEPREFIX : 0)
                                );
                        }
                        else
                        {
                            DrawText(
                                DrawInfo->hdc,
                                buttonText->Buffer,
                                (UINT)buttonText->Length / sizeof(WCHAR),
                                &bufferRect,
                                DT_LEFT | DT_TOP | DT_CALCRECT | (!isKeyboardFocused ? DT_HIDEPREFIX : 0)
                                );

                            bufferRect.top = (DrawInfo->rc.bottom - DrawInfo->rc.top) / 2 - (bufferRect.bottom - bufferRect.top) / 2 - 1;
                            bufferRect.bottom = DrawInfo->rc.bottom, bufferRect.right = DrawInfo->rc.right;

                            DrawText(
                                DrawInfo->hdc,
                                buttonText->Buffer,
                                (UINT)buttonText->Length / sizeof(WCHAR),
                                &bufferRect,
                                DT_LEFT | DT_TOP | (!isKeyboardFocused ? DT_HIDEPREFIX : 0)
                                );
                        }

                        if (isKeyboardFocused)
                        {
                            DrawText(
                                DrawInfo->hdc,
                                buttonText->Buffer,
                                (UINT)buttonText->Length / sizeof(WCHAR),
                                &bufferRect,
                                DT_LEFT | DT_TOP | DT_CALCRECT
                                );
                            PhInflateRect(&bufferRect, 1, 0);
                            bufferRect.top += 1, bufferRect.bottom += 2;
                            if (bufferRect.bottom > DrawInfo->rc.bottom - 1) bufferRect.bottom = DrawInfo->rc.bottom - 1;

                            for (INT i = 0; i < bufferRect.right - bufferRect.left - 1; i += 2)
                                SetPixel(DrawInfo->hdc, bufferRect.left + i + 1, bufferRect.bottom, PhThemeWindowHighlight2Color);
                            for (INT i = 0; i < bufferRect.bottom - bufferRect.top - 1; i += 2)
                                SetPixel(DrawInfo->hdc, bufferRect.right, bufferRect.bottom - i - 1, PhThemeWindowHighlight2Color);
                            for (INT i = 0; i < bufferRect.right - bufferRect.left - 1; i += 2)
                                SetPixel(DrawInfo->hdc, bufferRect.right - i - 1, bufferRect.top, PhThemeWindowHighlight2Color);
                            for (INT i = 0; i < bufferRect.bottom - bufferRect.top - 1; i += 2)
                                SetPixel(DrawInfo->hdc, bufferRect.left, bufferRect.top + i + 1, PhThemeWindowHighlight2Color);
                        }

                        PhCloseThemeData(themeHandle);
                    }
                    else
                    {
                        if (isChecked)
                        {
                            HFONT newFont = PhDuplicateFontWithNewHeight(PhApplicationFont, 16, dpiValue);
                            HFONT oldFont;

                            oldFont = SelectFont(DrawInfo->hdc, newFont);
                            DrawText(
                                DrawInfo->hdc,
                                L"\u2611",
                                1,
                                &DrawInfo->rc,
                                DT_LEFT | DT_SINGLELINE | DT_VCENTER
                                );
                            SelectFont(DrawInfo->hdc, oldFont);
                            DeleteFont(newFont);
                        }
                        else
                        {
                            HFONT newFont = PhDuplicateFontWithNewHeight(PhApplicationFont, 22, dpiValue);
                            HFONT oldFont;

                            oldFont = SelectFont(DrawInfo->hdc, newFont);
                            DrawText(
                                DrawInfo->hdc,
                                L"\u2610",
                                1,
                                &DrawInfo->rc,
                                DT_LEFT | DT_SINGLELINE | DT_VCENTER
                                );
                            SelectFont(DrawInfo->hdc, oldFont);
                            DeleteFont(newFont);
                        }

                        bufferRect.left = 17;
                        bufferRect.right = DrawInfo->rc.right;

                        DrawText(
                            DrawInfo->hdc,
                            buttonText->Buffer,
                            (UINT)buttonText->Length / sizeof(WCHAR),
                            &bufferRect,
                            DT_LEFT | DT_VCENTER | DT_SINGLELINE | (!isKeyboardFocused ? DT_HIDEPREFIX : 0)
                            );
                    }
                }
            }
            else
            {
                if (isSelected)
                {
                    SetTextColor(DrawInfo->hdc, PhThemeWindowTextColor);
                    PhpThemeFillRect(DrawInfo->hdc, &DrawInfo->rc, PHP_THEME_WINDOW_PRESSED_COLOR);
                }
                else if (isHighlighted)
                {
                    SetTextColor(DrawInfo->hdc, PhThemeWindowTextColor);
                    PhpThemeFillRect(DrawInfo->hdc, &DrawInfo->rc, PhThemeWindowBackground2Color);
                }
                else
                {
                    SetTextColor(DrawInfo->hdc, !isDisabled ? PhThemeWindowTextColor : PHP_THEME_WINDOW_DISABLED_TEXT_COLOR);
                    //SetDCBrushColor(DrawInfo->hdc, PhThemeWindowBackgroundColor); // WindowForegroundColor
                    FillRect(DrawInfo->hdc, &DrawInfo->rc, PhThemeWindowBackgroundBrush);
                }

                SetBkMode(DrawInfo->hdc, TRANSPARENT);
                PhpThemeFrameRect(DrawInfo->hdc, &DrawInfo->rc, !isFocused ? PhThemeWindowBackground2Color : PhThemeWindowHighlightColor);

                PhThemeDrawButtonIcon(DrawInfo, buttonIcon, &bufferRect, dpiValue);

                if ((buttonStyle & BS_ICON) != BS_ICON)
                {
                    DrawText(
                        DrawInfo->hdc,
                        buttonText->Buffer,
                        (UINT)buttonText->Length / sizeof(WCHAR),
                        &bufferRect,
                        DT_CENTER | DT_SINGLELINE | DT_VCENTER | (!isKeyboardFocused ? DT_HIDEPREFIX : 0)
                        );
                }
            }
            PhDereferenceObject(buttonText);
        }

        return CDRF_SKIPDEFAULT;
    }

    return CDRF_DODEFAULT;
}

LRESULT CALLBACK PhThemeWindowDrawRebar(
    _In_ LPNMCUSTOMDRAW DrawInfo
    )
{
    // temp chevron workaround until location/state supported.
    if (DrawInfo->dwDrawStage != CDDS_PREPAINT)
        return CDRF_DODEFAULT;

    switch (DrawInfo->dwDrawStage)
    {
    case CDDS_PREPAINT:
        {
            //REBARBANDINFO bandinfo = { sizeof(REBARBANDINFO), RBBIM_CHILD | RBBIM_STYLE | RBBIM_CHEVRONLOCATION | RBBIM_CHEVRONSTATE };
            //BOOL havebandinfo = (BOOL)SendMessage(DrawInfo->hdr.hwndFrom, RB_GETBANDINFO, DrawInfo->dwItemSpec, (LPARAM)&bandinfo);

            SetTextColor(DrawInfo->hdc, PhThemeWindowTextColor);
            FillRect(DrawInfo->hdc, &DrawInfo->rc, PhThemeWindowBackgroundBrush);
        }
        return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT:
        {

        }
        return CDRF_SKIPDEFAULT;
    }

    return CDRF_DODEFAULT;
}

LRESULT CALLBACK PhThemeWindowDrawToolbar(
    _In_ LPNMTBCUSTOMDRAW DrawInfo
    )
{
    switch (DrawInfo->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT;
    case CDDS_ITEMPREPAINT:
        {
            TBBUTTONINFO buttonInfo =
            {
                sizeof(TBBUTTONINFO),
                TBIF_STYLE | TBIF_COMMAND | TBIF_STATE | TBIF_IMAGE
            };

            SetBkMode(DrawInfo->nmcd.hdc, TRANSPARENT);

            LONG dpiValue;
            INT bitmapWidth = 0;
            INT bitmapHeight = 0;

            ULONG currentIndex = (ULONG)SendMessage(
                DrawInfo->nmcd.hdr.hwndFrom,
                TB_COMMANDTOINDEX,
                DrawInfo->nmcd.dwItemSpec,
                0
                );
            BOOLEAN isHighlighted = SendMessage(
                DrawInfo->nmcd.hdr.hwndFrom,
                TB_GETHOTITEM,
                0,
                0
                ) == currentIndex;
            BOOLEAN isMouseDown = SendMessage(
                DrawInfo->nmcd.hdr.hwndFrom,
                TB_ISBUTTONPRESSED,
                DrawInfo->nmcd.dwItemSpec,
                0
                ) == 0;
            BOOLEAN isEnabled = SendMessage(
                DrawInfo->nmcd.hdr.hwndFrom,
                TB_ISBUTTONENABLED,
                DrawInfo->nmcd.dwItemSpec,
                0
                ) != 0;

            if (SendMessage(
                DrawInfo->nmcd.hdr.hwndFrom,
                TB_GETBUTTONINFO,
                (ULONG)DrawInfo->nmcd.dwItemSpec,
                (LPARAM)&buttonInfo
                ) == INT_ERROR)
            {
                break;
            }

            BOOLEAN isDropDown = !!(buttonInfo.fsStyle & BTNS_WHOLEDROPDOWN);

            //if (isMouseDown)
            //{
            //    SetTextColor(DrawInfo->nmcd.hdc, PhThemeWindowTextColor);
            //    SetDCBrushColor(DrawInfo->nmcd.hdc, RGB(0xff, 0xff, 0xff));
            //    FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, PhGetStockBrush(DC_BRUSH));
            //}

            if (isHighlighted)
            {
                //INT stateId;
                //// Themed background
                //if (node->Selected)
                //{
                //    if (i == Context->HotNodeIndex)
                //        stateId = TREIS_HOTSELECTED;
                //    else if (!Context->HasFocus)
                //        stateId = TREIS_SELECTEDNOTFOCUS;
                //    else
                //        stateId = TREIS_SELECTED;
                //}
                //else
                //{
                //    if (i == Context->HotNodeIndex)
                //        stateId = TREIS_HOT;
                //    else
                //        stateId = INT_MAX;
                //}
                //// Themed background
                //if (stateId != INT_MAX)
                //if (!Context->FixedColumnVisible)
                //{
                //    rowRect.left = Context->NormalLeft - hScrollPosition;
                //}
                SetTextColor(DrawInfo->nmcd.hdc, PhThemeWindowTextColor);
                SetDCBrushColor(DrawInfo->nmcd.hdc, PhThemeWindowHighlightColor);
                FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, PhpStockDCBrush);
            }
            else
            {
                BOOLEAN isPressed = buttonInfo.fsState & TBSTATE_PRESSED;
                SetTextColor(DrawInfo->nmcd.hdc, PhThemeWindowTextColor);
                if (!isPressed)
                    FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, PhThemeWindowBackgroundBrush);
                else {
                    PhpThemeFillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, PHP_THEME_WINDOW_PRESSED_COLOR);
                }
            }

            dpiValue = PhGetWindowDpi(DrawInfo->nmcd.hdr.hwndFrom);

            SelectFont(DrawInfo->nmcd.hdc, GetWindowFont(DrawInfo->nmcd.hdr.hwndFrom));

            if (buttonInfo.iImage != I_IMAGECALLBACK)
            {
                HIMAGELIST toolbarImageList;

                if (toolbarImageList = (HIMAGELIST)SendMessage(
                    DrawInfo->nmcd.hdr.hwndFrom,
                    TB_GETIMAGELIST,
                    0,
                    0
                    ))
                {
                    LONG x;
                    LONG y;

                    PhImageListGetIconSize(toolbarImageList, &bitmapWidth, &bitmapHeight);

                    if (buttonInfo.fsStyle & BTNS_SHOWTEXT)
                    {
                        DrawInfo->nmcd.rc.left += PhGetSystemMetrics(SM_CXEDGE, dpiValue); // PhScaleToDisplay(5, dpiValue);
                        x = DrawInfo->nmcd.rc.left;
                        y = DrawInfo->nmcd.rc.top + ((DrawInfo->nmcd.rc.bottom - DrawInfo->nmcd.rc.top) - bitmapHeight) / 2;
                    }
                    else
                    {
                        x = DrawInfo->nmcd.rc.left + ((DrawInfo->nmcd.rc.right - DrawInfo->nmcd.rc.left) - bitmapWidth) / 2 - (isDropDown * 4);
                        y = DrawInfo->nmcd.rc.top + ((DrawInfo->nmcd.rc.bottom - DrawInfo->nmcd.rc.top) - bitmapHeight) / 2;
                    }

                    PhImageListDrawIcon(
                        toolbarImageList,
                        buttonInfo.iImage,
                        DrawInfo->nmcd.hdc,
                        x,
                        y,
                        ILD_NORMAL,
                        !isEnabled
                        );

                    if (isDropDown)
                    {
                        HDC hdc = DrawInfo->nmcd.hdc;
                        LPRECT rc = &DrawInfo->nmcd.rc;
                        int triangleLeft = rc->right - 11, triangleTop = (rc->bottom - rc->top) / 2 - 2;
                        POINT vertices[] = { {triangleLeft, triangleTop}, {triangleLeft + 6, triangleTop}, {triangleLeft + 3, triangleTop + 3} };
                        SetDCPenColor(hdc, PHP_THEME_WINDOW_DROPDOWN_GLYPH_COLOR);
                        SetDCBrushColor(hdc, PHP_THEME_WINDOW_DROPDOWN_GLYPH_COLOR);
                        SelectPen(hdc, PhGetStockPen(DC_PEN));
                        SelectBrush(hdc, PhpStockDCBrush);
                        Polygon(hdc, vertices, _countof(vertices));
                    }

                    //return CDRF_SKIPDEFAULT | CDRF_NOTIFYPOSTPAINT;
                }
            }
            else
            {
                return CDRF_DODEFAULT; // Required for I_IMAGECALLBACK (dmex)
            }

            if (buttonInfo.fsStyle & BTNS_SHOWTEXT)
            {
                RECT textRect = DrawInfo->nmcd.rc;
                WCHAR buttonText[MAX_PATH] = L"";

                SendMessage(
                    DrawInfo->nmcd.hdr.hwndFrom,
                    TB_GETBUTTONTEXT,
                    (ULONG)DrawInfo->nmcd.dwItemSpec,
                    (LPARAM)buttonText
                    );

                textRect.left += bitmapWidth - (isDropDown * 12); // PhScaleToDisplay(10, dpiValue);
                DrawText(
                    DrawInfo->nmcd.hdc,
                    buttonText,
                    (UINT)PhCountStringZ(buttonText),
                    &textRect,
                    DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX
                    );
            }

            //DrawInfo->clrText = RGB(0x0, 0xff, 0);
            //return TBCDRF_USECDCOLORS | CDRF_NEWFONT;
        }
        return CDRF_SKIPDEFAULT;
    }

    return CDRF_DODEFAULT;
}

LRESULT CALLBACK PhpThemeWindowDrawListViewGroup(
    _In_ LPNMLVCUSTOMDRAW DrawInfo
    )
{
    switch (DrawInfo->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        {
            LONG dpiValue = PhGetWindowDpi(DrawInfo->nmcd.hdr.hwndFrom);
            HFONT fontHandle = NULL;
            HFONT oldFontHandle = NULL;
            LVGROUP groupInfo;

            {
                NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };

                if (PhGetSystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, dpiValue))
                {
                    metrics.lfMessageFont.lfHeight = PhScaleToDisplay(-11, dpiValue);
                    metrics.lfMessageFont.lfWeight = FW_BOLD;

                    fontHandle = CreateFontIndirect(&metrics.lfMessageFont);
                }
            }

            SetBkMode(DrawInfo->nmcd.hdc, TRANSPARENT);
            //SelectFont(DrawInfo->nmcd.hdc, GetWindowFont(DrawInfo->nmcd.hdr.hwndFrom));
            if (fontHandle)
            {
                oldFontHandle = SelectFont(DrawInfo->nmcd.hdc, fontHandle);
            }

            memset(&groupInfo, 0, sizeof(LVGROUP));
            groupInfo.cbSize = sizeof(LVGROUP);
            groupInfo.mask = LVGF_HEADER;

            if (ListView_GetGroupInfo(DrawInfo->nmcd.hdr.hwndFrom, (ULONG)DrawInfo->nmcd.dwItemSpec, &groupInfo) != INT_ERROR)
            {
                SetTextColor(DrawInfo->nmcd.hdc, PhThemeWindowTextColor);
                SetDCBrushColor(DrawInfo->nmcd.hdc, PhThemeWindowBackground2Color);

                DrawInfo->rcText.top += PhScaleToDisplay(2, dpiValue);
                DrawInfo->rcText.bottom -= PhScaleToDisplay(2, dpiValue);
                FillRect(DrawInfo->nmcd.hdc, &DrawInfo->rcText, PhpStockDCBrush);
                DrawInfo->rcText.top -= PhScaleToDisplay(2, dpiValue);
                DrawInfo->rcText.bottom += PhScaleToDisplay(2, dpiValue);

                if (groupInfo.pszHeader)
                {
                    DrawInfo->rcText.left += PhScaleToDisplay(10, dpiValue);
                    DrawText(
                        DrawInfo->nmcd.hdc,
                        groupInfo.pszHeader,
                        (UINT)PhCountStringZ(groupInfo.pszHeader),
                        &DrawInfo->rcText,
                        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX
                        );
                    DrawInfo->rcText.left -= PhScaleToDisplay(10, dpiValue);
                }
            }

            if (oldFontHandle)
            {
                SelectFont(DrawInfo->nmcd.hdc, oldFontHandle);
            }

            if (fontHandle)
            {
                DeleteFont(fontHandle);
            }
        }
        return CDRF_SKIPDEFAULT;
    }

    return CDRF_DODEFAULT;
}

LRESULT CALLBACK PhpThemeWindowSubclassProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    WNDPROC oldWndProc;

    if (!(oldWndProc = PhGetWindowContext(hWnd, LONG_MAX)))
        return FALSE;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            PhSetWindowProcedure(hWnd, oldWndProc);
            PhRemoveWindowContext(hWnd, LONG_MAX);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR data = (LPNMHDR)lParam;

            switch (data->code)
            {
            case NM_CUSTOMDRAW:
                {
                    LPNMCUSTOMDRAW customDraw = (LPNMCUSTOMDRAW)lParam;
                    WCHAR className[MAX_PATH];

                    if (!NT_SUCCESS(PhGetClassName(customDraw->hdr.hwndFrom, className, RTL_NUMBER_OF(className), NULL)))
                        className[0] = UNICODE_NULL;

                    if (PhEqualStringZ(className, WC_BUTTON, FALSE))
                    {
                        return PhThemeWindowDrawButton(customDraw);
                    }
                    else if (PhEqualStringZ(className, REBARCLASSNAME, FALSE))
                    {
                        return PhThemeWindowDrawRebar(customDraw);
                    }
                    else if (PhEqualStringZ(className, TOOLBARCLASSNAME, FALSE))
                    {
                        return PhThemeWindowDrawToolbar((LPNMTBCUSTOMDRAW)customDraw);
                    }
                    else if (PhEqualStringZ(className, WC_LISTVIEW, FALSE))
                    {
                        LPNMLVCUSTOMDRAW listViewCustomDraw = (LPNMLVCUSTOMDRAW)customDraw;

                        if (listViewCustomDraw->dwItemType == LVCDI_GROUP)
                        {
                            return PhpThemeWindowDrawListViewGroup(listViewCustomDraw);
                        }
                    }
                    else
                    {
                        //dprintf("NM_CUSTOMDRAW: %S\r\n", className);
                    }
                }
            }
        }
        break;
    case WM_CTLCOLOREDIT:
        {
            HDC hdc = (HDC)wParam;

             //Fix typing in multiline edit (Dart Vanya)
            if (PhGetWindowStyle((HWND)lParam) & ES_MULTILINE)
                SetBkColor(hdc, PhThemeWindowBackground2Color);
            else
                SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, PhThemeWindowTextColor);
            SetDCBrushColor(hdc, PhThemeWindowBackground2Color);
            return (INT_PTR)PhGetStockBrush(DC_BRUSH);
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        {
            HDC hdc = (HDC)wParam;

            if (uMsg == WM_CTLCOLORBTN)     // for correct drawing of system KEYBOARDCUES
                SetBkColor(hdc, PhThemeWindowBackground2Color);
            else
                SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, PhThemeWindowTextColor);
            return (INT_PTR)PhThemeWindowBackgroundBrush;
        }
        break;

    case WM_MEASUREITEM:
        if (PhThemeWindowMeasureItem(hWnd, (LPMEASUREITEMSTRUCT)lParam))
            return TRUE;
        break;
    case WM_DRAWITEM:
        if (PhThemeWindowDrawItem(hWnd, (LPDRAWITEMSTRUCT)lParam))
            return TRUE;
        break;
    case WM_NCPAINT:
    case WM_NCACTIVATE:
        {
            LRESULT result;

            if (uMsg == WM_NCACTIVATE)
                PhpUpdateThemeWindowBorderColor(hWnd, !!wParam);

            result = CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);

            PhWindowThemeMainMenuBorder(hWnd);

            return result;
        }
        break;
    }

    return CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
}

VOID ThemeWindowRenderGroupBoxControl(
    _In_ HWND WindowHandle,
    _In_ HDC bufferDc,
    _In_ PRECT clientRect,
    _In_ WNDPROC WindowProcedure
    )
{
    ULONG returnLength;
    WCHAR text[0x80];
    LONG dpiValue;

    SetBkMode(bufferDc, TRANSPARENT);
    SelectFont(bufferDc, GetWindowFont(WindowHandle));
    SetDCBrushColor(bufferDc, PhThemeWindowBackground2Color);
    FrameRect(bufferDc, clientRect, PhpStockDCBrush);

    dpiValue = PhGetWindowDpi(WindowHandle);

    if (NT_SUCCESS(PhGetWindowTextToBuffer(WindowHandle, PH_GET_WINDOW_TEXT_INTERNAL, text, RTL_NUMBER_OF(text), &returnLength)))
    {
        SIZE nameSize = { 0 };
        RECT bufferRect;

        GetTextExtentPoint32(
            bufferDc,
            text,
            returnLength,
            &nameSize
            );

        bufferRect.left = 0;
        bufferRect.top = 0;
        bufferRect.right = clientRect->right;
        bufferRect.bottom = nameSize.cy;

        SetTextColor(bufferDc, PhThemeWindowTextColor);
        SetDCBrushColor(bufferDc, PhThemeWindowBackground2Color);
        FillRect(bufferDc, &bufferRect, PhpStockDCBrush);

        bufferRect.left += PhScaleToDisplay(10, dpiValue);
        DrawText(
            bufferDc,
            text,
            returnLength,
            &bufferRect,
            DT_LEFT | DT_END_ELLIPSIS | DT_SINGLELINE
            );
        bufferRect.left -= PhScaleToDisplay(10, dpiValue);
    }
}

VOID ThemeWindowRenderClippedGroupBoxControl(
    _In_ HWND WindowHandle,
    _In_ HDC BufferDc,
    _In_ PRECT ClientRect
    )
{
    ULONG returnLength = 0;
    WCHAR text[0x80];
    HFONT oldFont;
    SIZE textSize = { 0 };
    LONG dpiValue;

    SetBkMode(BufferDc, TRANSPARENT);
    SetDCBrushColor(BufferDc, PhThemeWindowBackgroundColor);
    FillRect(BufferDc, ClientRect, PhpStockDCBrush);

    oldFont = SelectFont(BufferDc, GetWindowFont(WindowHandle));

    if (!oldFont)
        oldFont = SelectFont(BufferDc, PhGetStockObject(DEFAULT_GUI_FONT));

    dpiValue = PhGetWindowDpi(WindowHandle);

    if (NT_SUCCESS(PhGetWindowTextToBuffer(WindowHandle, PH_GET_WINDOW_TEXT_INTERNAL, text, RTL_NUMBER_OF(text), &returnLength)))
    {
        GetTextExtentPoint32(
            BufferDc,
            text,
            returnLength,
            &textSize
            );
    }

    {
        RECT frameRect = *ClientRect;
        HPEN framePen;
        HPEN oldPen;
        HBRUSH oldBrush;

        frameRect.top += textSize.cy / 2;

        framePen = CreatePen(PS_SOLID, 1, PhThemeWindowBackground2Color);
        oldPen = framePen ? SelectPen(BufferDc, framePen) : NULL;
        oldBrush = SelectBrush(BufferDc, PhGetStockBrush(NULL_BRUSH));

        if (framePen)
            Rectangle(BufferDc, frameRect.left, frameRect.top, frameRect.right, frameRect.bottom);

        if (oldBrush)
            SelectBrush(BufferDc, oldBrush);
        if (oldPen)
            SelectPen(BufferDc, oldPen);
        if (framePen)
            DeletePen(framePen);
    }

    if (returnLength)
    {
        LONG textOffsetX = PhScaleToDisplay(9, dpiValue);
        LONG textPadding = PhScaleToDisplay(3, dpiValue);
        RECT textRect = { textOffsetX, 0, textOffsetX + textSize.cx + textPadding * 2, textSize.cy };

        SetTextColor(BufferDc, PhThemeWindowTextColor);
        SetDCBrushColor(BufferDc, PhThemeWindowBackgroundColor);
        FillRect(BufferDc, &textRect, PhpStockDCBrush);

        TextOut(BufferDc, textOffsetX + textPadding, 0, text, returnLength);
    }

    if (oldFont)
        SelectFont(BufferDc, oldFont);
}

LRESULT CALLBACK PhpThemeWindowGroupBoxSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    WNDPROC oldWndProc;

    if (!(oldWndProc = PhGetWindowContext(WindowHandle, LONG_MAX)))
        return FALSE;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            PhSetWindowProcedure(WindowHandle, oldWndProc);
            PhRemoveWindowContext(WindowHandle, LONG_MAX);
        }
        break;
    case WM_ERASEBKGND:
    //    {
    //        HDC hdc = (HDC)wParam;
    //        RECT clientRect;

    //        if (FlagOn(PhGetWindowStyle(WindowHandle), WS_CLIPSIBLINGS))
    //            return TRUE;

    //        if (!PhGetClientRect(WindowHandle, &clientRect))
    //            break;

    //        ThemeWindowRenderGroupBoxControl(WindowHandle, hdc, &clientRect, oldWndProc);
    //    }
        return TRUE;
    case WM_ENABLE:
        if (!wParam)    // fix drawing when window visible and switches to disabled
            return 0;
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;

            hdc = BeginPaint(WindowHandle, &ps);

            if (FlagOn(PhGetWindowStyle(WindowHandle), WS_CLIPSIBLINGS))
            {
                RECT clientRect;

                if (PhGetClientRect(WindowHandle, &clientRect))
                {
                    PH_BUFFERED_PAINT paintBuffer;
                    HDC bufferDc;

                    if (PhBeginBufferedPaint(hdc, &ps.rcPaint, &paintBuffer, &bufferDc))
                    {
                        ThemeWindowRenderClippedGroupBoxControl(WindowHandle, bufferDc, &clientRect);
                        PhEndBufferedPaint(&paintBuffer, TRUE);
                    }
                    else
                    {
                        ThemeWindowRenderClippedGroupBoxControl(WindowHandle, hdc, &clientRect);
                    }
                }
            }

            EndPaint(WindowHandle, &ps);
        }
        return 0;
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
}

LRESULT CALLBACK PhThemeWindowGroupBoxExSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    WNDPROC oldWndProc = PhGetWindowContext(WindowHandle, LONG_MAX);

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            PhSetWindowProcedure(WindowHandle, oldWndProc);
            PhRemoveWindowContext(WindowHandle, LONG_MAX);
        }
        break;
    case WM_ERASEBKGND:
        // Prevent flicker: suppress default erasure since we fill the background in WM_PAINT.
        return 1;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            RECT clientRect;
            HDC memoryDc;
            HBITMAP memoryBitmap;
            HBITMAP oldBitmap;
            HFONT font;
            HFONT oldFont;
            PPH_STRING text;
            SIZE textSize;
            RECT frameRect;
            HPEN framePen;
            HPEN oldPen;
            HBRUSH oldBrush;

            hdc = BeginPaint(WindowHandle, &ps);
            GetClientRect(WindowHandle, &clientRect);

            // Setup double-buffering to prevent flicker during resize.
            memoryDc = CreateCompatibleDC(hdc);
            memoryBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
            oldBitmap = (HBITMAP)SelectObject(memoryDc, memoryBitmap);

            SetBkMode(memoryDc, TRANSPARENT);

            FillRect(memoryDc, &clientRect, PhThemeWindowBackgroundBrush);

            // Setup font for text measurement.
            font = (HFONT)SendMessage(WindowHandle, WM_GETFONT, 0, 0);
            if (!font)
                font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            oldFont = (HFONT)SelectObject(memoryDc, font);

            // Get and measure the group box title text using PPH_STRING.
            text = PhGetWindowText(WindowHandle);
            textSize.cx = 0;
            textSize.cy = 0;

            if (!PhIsNullOrEmptyString(text))
            {
                GetTextExtentPoint32(
                    memoryDc,
                    text->Buffer,
                    (ULONG)(text->Length / sizeof(WCHAR)),
                    &textSize
                );
            }

            // Draw the frame border. The top edge starts at the vertical midpoint of
            // the text so the label sits centered on the top line.
            frameRect = clientRect;
            frameRect.top += (textSize.cy / 2);

            framePen = CreatePen(PS_SOLID, 1, PhThemeWindowGroupBoxFrameColor);
            oldPen = SelectPen(memoryDc, framePen);
            oldBrush = SelectBrush(memoryDc, PhGetStockObject(NULL_BRUSH));

            Rectangle(memoryDc, frameRect.left, frameRect.top, frameRect.right, frameRect.bottom);

            SelectBrush(memoryDc, oldBrush);
            SelectPen(memoryDc, oldPen);
            DeletePen(framePen);

            // Draw the text label over the top border line.
            if (!PhIsNullOrEmptyString(text))
            {
                RECT textRect;
                LONG dpiValue = PhGetWindowDpi(WindowHandle);
                LONG textOffsetX = PhScaleToDisplay(9, dpiValue);
                LONG textPadding = PhScaleToDisplay(3, dpiValue);

                textRect.left = textOffsetX;
                textRect.top = 0;
                textRect.right = textOffsetX + textSize.cx + (textPadding * 2);
                textRect.bottom = textSize.cy;

                FillRect(memoryDc, &textRect, PhThemeWindowBackgroundBrush);

                SetTextColor(memoryDc, PhThemeWindowTextColor);
                TextOut(
                    memoryDc,
                    textOffsetX + textPadding,
                    0,
                    text->Buffer,
                    (ULONG)(text->Length / sizeof(WCHAR))
                );
            }

            if (text)
                PhDereferenceObject(text);

            SelectFont(memoryDc, oldFont);

            // Blit to screen and clean up.
            BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, memoryDc, 0, 0, SRCCOPY);

            SelectFont(memoryDc, oldBitmap);
            DeleteBitmap(memoryBitmap);
            DeleteDC(memoryDc);

            EndPaint(WindowHandle, &ps);
        }
        return 0;
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
}

VOID ThemeWindowRenderTabControl(
    _In_ PPHP_THEME_WINDOW_TAB_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC bufferDc,
    _In_ PRECT clientRect,
    _In_ WNDPROC WindowProcedure
    )
{
    INT currentSelection;
    INT count;
    RECT contentRect;
    TCITEM tabItem;
    HFONT oldFont;
    LONG cxEdge;
    LONG cyEdge;
    LONG cxPad;
    LONG cyPad;
    WCHAR tabHeaderText[MAX_PATH] = L"";

    SetBkMode(bufferDc, TRANSPARENT);
    SetTextColor(bufferDc, PhThemeWindowTextColor);
    SetDCBrushColor(bufferDc, PhThemeWindowBackgroundColor);
    FillRect(bufferDc, clientRect, PhpStockDCBrush);

    oldFont = SelectFont(bufferDc, GetWindowFont(WindowHandle));

    if (!oldFont)
        oldFont = SelectFont(bufferDc, PhGetStockObject(DEFAULT_GUI_FONT));

    cxEdge = PhGetSystemMetrics(SM_CXEDGE, Context->WindowDpi);
    cyEdge = PhGetSystemMetrics(SM_CYEDGE, Context->WindowDpi);
    cxPad = cxEdge * 3;
    cyPad = (cyEdge * 3) / 2;

    contentRect = *clientRect;
    TabCtrl_AdjustRect(WindowHandle, FALSE, &contentRect);
    PhInflateRect(&contentRect, cxEdge * 2, cyEdge * 2);
    contentRect.top += cyEdge;

    SetDCBrushColor(bufferDc, PhThemeWindowBackground2Color);
    FrameRect(bufferDc, &contentRect, PhpStockDCBrush);
    PhInflateRect(&contentRect, -1, -1);
    SetDCBrushColor(bufferDc, PhThemeWindowBackgroundColor);
    FillRect(bufferDc, &contentRect, PhpStockDCBrush);

    currentSelection = TabCtrl_GetCurSel(WindowHandle);
    count = TabCtrl_GetItemCount(WindowHandle);

    memset(&tabItem, 0, sizeof(TCITEM));
    tabItem.mask = TCIF_TEXT;
    tabItem.cchTextMax = RTL_NUMBER_OF(tabHeaderText);
    tabItem.pszText = tabHeaderText;

    for (INT pass = 0; pass < 2; pass++)
    {
        for (INT i = 0; i < count; i++)
        {
            RECT itemRect;
            RECT textRect;
            BOOLEAN selected;
            BOOLEAN hot;

            selected = i == currentSelection;

            if ((pass == 0 && selected) || (pass == 1 && !selected))
                continue;
            if (!TabCtrl_GetItemRect(WindowHandle, i, &itemRect))
                continue;

            hot = PhPtInRect(&itemRect, &Context->CursorPos);
            textRect = itemRect;

            if (selected)
            {
                PhInflateRect(&itemRect, cxEdge, cyEdge);
                SetDCBrushColor(bufferDc, hot ? PhThemeWindowBackground2Color : PhThemeWindowBackgroundColor);
            }
            else
            {
                SetDCBrushColor(bufferDc, hot ? PhThemeWindowBackground2Color : PhThemeWindowBackgroundColor);
            }

            FillRect(bufferDc, &itemRect, PhGetStockBrush(DC_BRUSH));

            if (selected)
            {
                SetDCBrushColor(bufferDc, PhThemeWindowBackground2Color);
                FrameRect(bufferDc, &itemRect, PhGetStockBrush(DC_BRUSH));
            }

            if (selected)
            {
                RECT selectedGap = itemRect;

                selectedGap.top = itemRect.bottom - cyEdge;
                SetDCBrushColor(bufferDc, hot ? PhThemeWindowBackground2Color : PhThemeWindowBackgroundColor);
                FillRect(bufferDc, &selectedGap, PhGetStockBrush(DC_BRUSH));
            }

            if (TabCtrl_GetItem(WindowHandle, i, &tabItem))
            {
                PhInflateRect(&textRect, -cxPad, -cyPad);

                if (selected)
                    PhOffsetRect(&textRect, 0, -cyEdge);

                SetTextColor(bufferDc, PhThemeWindowTextColor);
                DrawText(
                    bufferDc,
                    tabItem.pszText,
                    (UINT)PhCountStringZ(tabItem.pszText),
                    &textRect,
                    DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX | DT_END_ELLIPSIS
                    );
            }
        }
    }

    if (oldFont)
        SelectFont(bufferDc, oldFont);
}

VOID ThemeWindowRenderTabControlOld(
    _In_ PPHP_THEME_WINDOW_TAB_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC bufferDc,
    _In_ PRECT clientRect,
    _In_ WNDPROC WindowProcedure
)
{
    //RECT windowRect;

    //GetWindowRect(WindowHandle, &windowRect);

    //CallWindowProc(WindowProcedure, WindowHandle, WM_PRINTCLIENT, (WPARAM)bufferDc, PRF_CLIENT);

    //TabCtrl_AdjustRect(WindowHandle, FALSE, clientRect); // Make sure we don't paint in the client area.
    //ExcludeClipRect(bufferDc, clientRect->left, clientRect->top, clientRect->right, clientRect->bottom);

    //windowRect.right -= windowRect.left;
    //windowRect.bottom -= windowRect.top;
    //windowRect.left = 0;
    //windowRect.top = 0;

    //clientRect->left = windowRect.left;
    //clientRect->top = windowRect.top;
    //clientRect->right = windowRect.right;
    //clientRect->bottom = windowRect.bottom;

    SetBkMode(bufferDc, TRANSPARENT);
    SelectFont(bufferDc, GetWindowFont(WindowHandle));

    SetTextColor(bufferDc, PhThemeWindowTextColor);
    FillRect(bufferDc, clientRect, PhThemeWindowBackgroundBrush);

    //switch (PhpThemeColorMode)
    //{
    //case 0: // New colors
    //    {
    //        //SetTextColor(DrawInfo->hdc, RGB(0x0, 0x0, 0x0));
    //        //SetDCBrushColor(DrawInfo->hdc, GetSysColor(COLOR_3DFACE)); // RGB(0xff, 0xff, 0xff));
    //    }
    //    break;
    //case 1: // Old colors
    //    {
    //        //SetTextColor(DrawInfo->hdc, RGB(0xff, 0xff, 0xff));
    //        //SetDCBrushColor(DrawInfo->hdc, PhThemeWindowBackground2Color);
    //    }
    //    break;
    //}

    //switch (PhpThemeColorMode)
    //{
    //case 0: // New colors
    //    //SetTextColor(hdc, RGB(0x0, 0xff, 0x0));
    //    SetDCBrushColor(hdc, GetSysColor(COLOR_3DFACE));// PhThemeWindowTextColor);
    //    FillRect(hdc, &clientRect, PhGetStockBrush(DC_BRUSH));
    //    break;
    //case 1: // Old colors
    //    //SetTextColor(hdc, PhThemeWindowTextColor);
    //    SetDCBrushColor(hdc, RPhThemeWindowBackground2Color);
    //    FillRect(hdc, &clientRect, PhGetStockBrush(DC_BRUSH));
    //    break;
    //}

    INT currentSelection = TabCtrl_GetCurSel(WindowHandle);
    INT count = TabCtrl_GetItemCount(WindowHandle);
    RECT itemRect = { 0 };
    //RECT itemRectHighlighted;
    //INT itemHighlighted = INT_ERROR;
    INT headerBottom;
    INT oldTop;

    oldTop = clientRect->top;
    TabCtrl_GetItemRect(WindowHandle, 0, &itemRect);
    clientRect->top += (itemRect.bottom - itemRect.top) * TabCtrl_GetRowCount(WindowHandle) + 2;

    //SetDCBrushColor(bufferDc, PhThemeWindowBackground2Color);
    //FrameRect(bufferDc, clientRect, PhGetStockBrush(DC_BRUSH));
    headerBottom = clientRect->top;
    clientRect->top = oldTop;

    TCITEM tabItem;
    WCHAR tabHeaderText[MAX_PATH] = L"";

    memset(&tabItem, 0, sizeof(TCITEM));

    tabItem.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_STATE;
    tabItem.dwStateMask = TCIS_BUTTONPRESSED | TCIS_HIGHLIGHTED;
    tabItem.cchTextMax = RTL_NUMBER_OF(tabHeaderText);
    tabItem.pszText = tabHeaderText;

    for (INT i = 0; i < count; i++)
    {
        if (i == currentSelection)
            continue;

        TabCtrl_GetItemRect(WindowHandle, i, &itemRect);

        PhOffsetRect(&itemRect, 2, 2);
        itemRect.bottom += itemRect.bottom + 1 < headerBottom ? 1 : -1;
        itemRect.right += itemRect.right + 1 < clientRect->right;

        if (PhPtInRect(&itemRect, &Context->CursorPos))
        {
            //switch (PhpThemeColorMode)
            //{
            //case 0: // New colors
            //    {
            //        if (currentSelection == i)
            //        {
            //            SetTextColor(bufferDc, RGB(0xff, 0xff, 0xff));
            //            SetDCBrushColor(bufferDc, PhThemeWindowHighlightColor);
            //            FillRect(bufferDc, &itemRect, PhGetStockBrush(DC_BRUSH));
            //        }
            //        else
            //        {
            //            SetTextColor(bufferDc, PhThemeWindowTextColor);
            //            SetDCBrushColor(bufferDc, PhThemeWindowBackgroundColor);
            //            FillRect(bufferDc, &itemRect, PhGetStockBrush(DC_BRUSH));
            //        }
            //    }
            //    break;
            //case 1: // Old colors
            //SetTextColor(bufferDc, PhThemeWindowTextColor);
            SetDCBrushColor(bufferDc, PhThemeWindowHighlightColor);
            FillRect(bufferDc, &itemRect, PhGetStockBrush(DC_BRUSH));

            //itemRectHighlighted = itemRect;
            //itemHighlighted = i;
            //continue;
        }
        else
        {
            //switch (PhpThemeColorMode)
            //{
            //case 0: // New colors
            //    {
            //        if (currentSelection == i)
            //        {
            //            SetTextColor(bufferDc, RGB(0x0, 0x0, 0x0));
            //            SetDCBrushColor(bufferDc, RGB(0xff, 0xff, 0xff));
            //            FillRect(bufferDc, &itemRect, PhGetStockBrush(DC_BRUSH));
            //        }
            //        else
            //        {
            //            SetTextColor(bufferDc, RGB(0, 0, 0));
            //            SetDCBrushColor(bufferDc, GetSysColor(COLOR_3DFACE));// PhThemeWindowTextColor);
            //            FillRect(bufferDc, &itemRect, PhGetStockBrush(DC_BRUSH));
            //        }
            //    }
            //    break;
            //case 1: // Old colors
            {
                // SetTextColor(bufferDc, PhThemeWindowTextColor);
                SetDCBrushColor(bufferDc, PhThemeWindowBackgroundColor);
                FillRect(bufferDc, &itemRect, PhGetStockBrush(DC_BRUSH));
                //SetDCBrushColor(bufferDc, PhThemeWindowBackground2Color);
                //FrameRect(bufferDc, &itemRect, PhGetStockBrush(DC_BRUSH));
            }
        }

        {
            if (TabCtrl_GetItem(WindowHandle, i, &tabItem))
            {
                DrawText(
                    bufferDc,
                    tabItem.pszText,
                    (UINT)PhCountStringZ(tabItem.pszText),
                    &itemRect,
                    DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX
                );
            }
        }
    }

    {
        TabCtrl_GetItemRect(WindowHandle, currentSelection, &itemRect);

        PhOffsetRect(&itemRect, 2, 2);
        itemRect.bottom += itemRect.bottom + 1 < headerBottom ? 1 : -1;
        itemRect.right += itemRect.right + 1 < clientRect->right;
        PhInflateRect(&itemRect, 1, 1);     // draw selected tab slightly bigger
        itemRect.bottom -= 1;
        SetDCBrushColor(bufferDc, PhPtInRect(&itemRect, &Context->CursorPos) ? PhThemeWindowHighlightColor : RGB(0x50, 0x50, 0x50));
        FillRect(bufferDc, &itemRect, PhGetStockBrush(DC_BRUSH));

        if (TabCtrl_GetItem(WindowHandle, currentSelection, &tabItem))
        {
            DrawText(
                bufferDc,
                tabItem.pszText,
                (UINT)PhCountStringZ(tabItem.pszText),
                &itemRect,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX
            );
        }

        //if (itemHighlighted != INT_ERROR)
        //{
        //    SetDCBrushColor(bufferDc, PhThemeWindowHighlightColor);
        //    FillRect(bufferDc, &itemRectHighlighted, PhGetStockBrush(DC_BRUSH));

        //    if (TabCtrl_GetItem(WindowHandle, itemHighlighted, &tabItem))
        //    {
        //        DrawText(
        //            bufferDc,
        //            tabItem.pszText,
        //            (UINT)PhCountStringZ(tabItem.pszText),
        //            &itemRectHighlighted,
        //            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX
        //            );
        //    }
        //}
    }
}
LRESULT CALLBACK PhpThemeWindowTabControlWndSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_THEME_WINDOW_TAB_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(WindowHandle, LONG_MAX)))
        return FALSE;

    oldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            PhSetWindowProcedure(WindowHandle, oldWndProc);
            PhRemoveWindowContext(WindowHandle, LONG_MAX);

            PhFree(context);
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
    case WM_THEMECHANGED:
        {
            context->WindowDpi = PhGetWindowDpi(WindowHandle);
        }
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_MOUSEMOVE:
        {
            //INT count;
            //INT i;
            //
            //count = TabCtrl_GetItemCount(WindowHandle);
            //
            //for (i = 0; i < count; i++)
            //{
            //    RECT rect = { 0, 0, 0, 0 };
            //    TCITEM entry =
            //    {
            //        TCIF_STATE,
            //        0,
            //        TCIS_HIGHLIGHTED
            //    };
            //
            //    TabCtrl_GetItemRect(WindowHandle, i, &rect);
            //    entry.dwState = PhPtInRect(&rect, context->CursorPos) ? TCIS_HIGHLIGHTED : 0;
            //    TabCtrl_SetItem(WindowHandle, i, &entry);
            //}

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
            //INT count;
            //INT i;
            //
            //count = TabCtrl_GetItemCount(WindowHandle);
            //
            //for (i = 0; i < count; i++)
            //{
            //    TCITEM entry =
            //    {
            //        TCIF_STATE,
            //        0,
            //        TCIS_HIGHLIGHTED
            //    };
            //
            //    TabCtrl_SetItem(WindowHandle, i, &entry);
            //}

            context->CursorPos.x = LONG_MIN;
            context->CursorPos.y = LONG_MIN;

            context->MouseActive = FALSE;
            InvalidateRect(WindowHandle, NULL, FALSE);
        }
        break;
    //case WM_PAINT:
    //{
    //    //PAINTSTRUCT ps;
    //    //HDC BufferedHDC;
    //    //HPAINTBUFFER BufferedPaint;
    //    //
    //    //if (!BeginPaint(WindowHandle, &ps))
    //    //    break;
    //    //
    //    //DEBUG_BEGINPAINT_RECT(WindowHandle, ps.rcPaint);
    //    //
    //    //{
    //    //    RECT clientRect;
    //    //    GetClientRect(WindowHandle, &clientRect);
    //    //    //CallWindowProc(oldWndProc, WindowHandle, WM_PAINT, wParam, lParam);
    //    //    TabCtrl_AdjustRect(WindowHandle, FALSE, &clientRect);
    //    //    ExcludeClipRect(ps.hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);
    //    //}
    //    //
    //    //if (BufferedPaint = BeginBufferedPaint(ps.hdc, &ps.rcPaint, BPBF_COMPATIBLEBITMAP, NULL, &BufferedHDC))
    //    //{
    //    //    ThemeWindowRenderTabControl(context, WindowHandle, BufferedHDC, &ps.rcPaint, oldWndProc);
    //    //    EndBufferedPaint(BufferedPaint, TRUE);
    //    //}
    //    //else
    //    {
    //        RECT clientRect;
    //        HDC hdc;
    //        HDC bufferDc;
    //        HBITMAP bufferBitmap;
    //        HBITMAP oldBufferBitmap;

    //        if (!PhGetClientRect(WindowHandle, &clientRect))
    //            break;

    //        hdc = GetDC(WindowHandle);
    //        bufferDc = CreateCompatibleDC(hdc);
    //        bufferBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
    //        oldBufferBitmap = SelectBitmap(bufferDc, bufferBitmap);

    //        {
    //            RECT clientRect2 = clientRect;
    //            TabCtrl_AdjustRect(WindowHandle, FALSE, &clientRect2);
    //            ExcludeClipRect(hdc, clientRect2.left, clientRect2.top, clientRect2.right, clientRect2.bottom);
    //        }

    //        ThemeWindowRenderTabControlOld(context, WindowHandle, bufferDc, &clientRect, oldWndProc);

    //        BitBlt(hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom, bufferDc, 0, 0, SRCCOPY);
    //        SelectBitmap(bufferDc, oldBufferBitmap);
    //        DeleteBitmap(bufferBitmap);
    //        DeleteDC(bufferDc);
    //        ReleaseDC(WindowHandle, hdc);
    //    }

    //    //EndPaint(WindowHandle, &ps);
    //}
    //break;
    case WM_PAINT:
        {
            PAINTSTRUCT paintStruct;
            HDC hdc;

            if (hdc = BeginPaint(WindowHandle, &paintStruct))
            {
                RECT clientRect;
                PH_BUFFERED_PAINT paintBuffer;
                HDC bufferDc;

                if (!PhGetClientRect(WindowHandle, &clientRect))
                {
                    EndPaint(WindowHandle, &paintStruct);
                    return 0;
                }

                if (PhBeginBufferedPaint(hdc, &paintStruct.rcPaint, &paintBuffer, &bufferDc))
                {
                    ThemeWindowRenderTabControl(context, WindowHandle, bufferDc, &clientRect, oldWndProc);
                    PhEndBufferedPaint(&paintBuffer, TRUE);
                }
                else
                {
                    ThemeWindowRenderTabControl(context, WindowHandle, hdc, &clientRect, oldWndProc);
                }

                EndPaint(WindowHandle, &paintStruct);
            }
        }
        return 0;
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
}

LRESULT CALLBACK PhpThemeWindowListBoxControlSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_THEME_WINDOW_STATUSBAR_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(WindowHandle, LONG_MAX)))
        return FALSE;

    oldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            PhSetWindowProcedure(WindowHandle, oldWndProc);
            PhRemoveWindowContext(WindowHandle, LONG_MAX);
        }
        break;
    //case WM_MOUSEMOVE:
    //case WM_NCMOUSEMOVE:
    //    {
    //        POINT windowPoint;
    //        RECT windowRect;
    //
    //        GetCursorPos(&windowPoint);
    //        GetWindowRect(WindowHandle, &windowRect);
    //        context->Hot = PhPtInRect(&windowRect, windowPoint);
    //
    //        if (!context->HotTrack)
    //        {
    //            TRACKMOUSEEVENT trackMouseEvent;
    //            trackMouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
    //            trackMouseEvent.dwFlags = TME_LEAVE;
    //            trackMouseEvent.hwndTrack = WindowHandle;
    //            trackMouseEvent.dwHoverTime = 0;
    //
    //            context->HotTrack = TRUE;
    //
    //            TrackMouseEvent(&trackMouseEvent);
    //        }
    //
    //        RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
    //    }
    //    break;
    //case WM_MOUSELEAVE:
    //case WM_NCMOUSELEAVE:
    //    {
    //        POINT windowPoint;
    //        RECT windowRect;
    //
    //        context->HotTrack = FALSE;
    //
    //        GetCursorPos(&windowPoint);
    //        GetWindowRect(WindowHandle, &windowRect);
    //        context->Hot = PhPtInRect(&windowRect, windowPoint);
    //
    //        RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
    //    }
    //    break;
    //case WM_CAPTURECHANGED:
    //    {
    //        POINT windowPoint;
    //        RECT windowRect;
    //
    //        GetCursorPos(&windowPoint);
    //        GetWindowRect(WindowHandle, &windowRect);
    //        context->Hot = PhPtInRect(&windowRect, windowPoint);
    //
    //        RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
    //    }
    //    break;
    case WM_NCCALCSIZE:
        {
            LPNCCALCSIZE_PARAMS ncCalcSize = (NCCALCSIZE_PARAMS*)lParam;

            CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);

            PhInflateRect(&ncCalcSize->rgrc[0], 1, 1);
        }
        return 0;
    case WM_NCPAINT:
        {
            HDC hdc;
            ULONG flags;
            RECT clientRect;
            RECT windowRect;
            HRGN updateRegion;
            LONG dpiValue = context->WindowDpi;
            INT cxEdge = PhGetSystemMetrics(SM_CXEDGE, dpiValue);
            INT cyEdge = PhGetSystemMetrics(SM_CYEDGE, dpiValue);

            updateRegion = (HRGN)wParam;

            if (!PhGetWindowRect(WindowHandle, &windowRect))
                break;

            // draw the scrollbar without the border.
            {
                HRGN rectregion = CreateRectRgn(
                    windowRect.left + cxEdge,
                    windowRect.top + cyEdge,
                    windowRect.right - cxEdge,
                    windowRect.bottom - cyEdge
                    );

                if (updateRegion != HRGN_FULL)
                    CombineRgn(rectregion, rectregion, updateRegion, RGN_AND);
                DefWindowProc(WindowHandle, WM_NCPAINT, (WPARAM)rectregion, 0);
                DeleteRgn(rectregion);
            }

            if (updateRegion == HRGN_FULL)
                updateRegion = NULL;

            flags = DCX_WINDOW | DCX_CACHE | DCX_USESTYLE;

            if (updateRegion)
                flags |= DCX_INTERSECTRGN | DCX_NODELETERGN;

            if (hdc = GetDCEx(WindowHandle, updateRegion, flags))
            {
                PhOffsetRect(&windowRect, -windowRect.left, -windowRect.top);
                clientRect = windowRect;
                clientRect.left += cxEdge;
                clientRect.top += cyEdge;
                clientRect.right -= cxEdge;
                clientRect.bottom -= cyEdge;

                {
                    SCROLLBARINFO scrollInfo = { sizeof(SCROLLBARINFO) };

                    if (GetScrollBarInfo(WindowHandle, OBJID_VSCROLL, &scrollInfo))
                    {
                        if ((scrollInfo.rgstate[0] & STATE_SYSTEM_INVISIBLE) == 0)
                        {
                            clientRect.right -= PhGetSystemMetrics(SM_CXVSCROLL, dpiValue);
                        }
                    }
                }

                ExcludeClipRect(hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);

                if (context->Hot)
                {
                    SetDCBrushColor(hdc, PhThemeWindowHighlightColor);
                    FrameRect(hdc, &windowRect, PhpStockDCBrush);
                }
                else
                {
                    PhpThemeFrameRect(hdc, &windowRect, PhThemeWindowBackground2Color);
                }

                ReleaseDC(WindowHandle, hdc);
                return TRUE;
            }
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
    case WM_THEMECHANGED:
        {
            context->WindowDpi = PhGetWindowDpi(WindowHandle);
        }
        break;
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
}

VOID ThemeWindowRenderComboBox(
    _In_ PPHP_THEME_WINDOW_COMBO_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC bufferDc,
    _In_ PRECT clientRect,
    _In_ WNDPROC WindowProcedure
    )
{
    RECT bufferRect =
    {
        0, 0,
       clientRect->right - clientRect->left,
       clientRect->bottom - clientRect->top
    };
    ULONG windowStyle = PhGetWindowStyle(WindowHandle);
    HFONT fontHandle;
    HFONT oldFont;
    //BOOLEAN isFocused = GetFocus() == WindowHandle;

    SetBkMode(bufferDc, TRANSPARENT);
    fontHandle = (HFONT)CallWindowProc(WindowProcedure, WindowHandle, WM_GETFONT, 0, 0);
    oldFont = SelectFont(bufferDc, fontHandle ? fontHandle : PhGetStockObject(DEFAULT_GUI_FONT));
    PhpThemeFillRect(bufferDc, clientRect, PhThemeWindowBackground2Color);

    if (PhPtInRect(clientRect, &Context->CursorPos))
    {
        SetDCBrushColor(bufferDc, PhThemeWindowHighlight2Color);
    }
    else
    {
        SetDCBrushColor(bufferDc, PhThemeWindowBackground2Color);
    }

    SetTextColor(bufferDc, PhThemeWindowTextColor);
    FrameRect(bufferDc, clientRect, PhpStockDCBrush);

    if (Context->ThemeHandle)
    {
        SIZE dropdownSize = { 0 };

        PhGetThemePartSize(
            Context->ThemeHandle,
            bufferDc,
            CP_DROPDOWNBUTTONRIGHT,
            CBXSR_NORMAL,
            NULL,
            THEMEPARTSIZE_TRUE,
            &dropdownSize
            );

        bufferRect.left = clientRect->right - dropdownSize.cx;

        PhDrawThemeBackground(
            Context->ThemeHandle,
            bufferDc,
            CP_DROPDOWNBUTTONRIGHT,
            CBXSR_DISABLED,
            &bufferRect,
            NULL
            );

        bufferRect.left = 0;
    }
    else
    {
        DrawFrameControl(bufferDc, &bufferRect, DFC_SCROLL, DFCS_SCROLLDOWN);
    }

    if ((windowStyle & CBS_DROPDOWNLIST) == CBS_DROPDOWNLIST)
    {
        INT index = ComboBox_GetCurSel(WindowHandle);

        if (index == CB_ERR)
            goto CleanupExit;

        INT length = ComboBox_GetLBTextLen(WindowHandle, index);

        if (length == CB_ERR)
            goto CleanupExit;

        if (length < MAX_PATH)
        {
            WCHAR comboText[MAX_PATH] = L"";

            if (ComboBox_GetLBText(WindowHandle, index, comboText) == CB_ERR)
                goto CleanupExit;

            bufferRect.left += 5;
            bufferRect.right -= 15; // info.rcItem.right
            DrawText(
                bufferDc,
                comboText,
                (UINT)PhCountStringZ(comboText),
                &bufferRect,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS
                );
        }
    }

CleanupExit:
    if (oldFont)
        SelectFont(bufferDc, oldFont);
}

VOID ThemeWindowComboBoxExcludeRect(
    _In_ PPHP_THEME_WINDOW_COMBO_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ PRECT clientRect,
    _In_ WNDPROC WindowProcedure
    )
{
    ULONG windowStyle = PhGetWindowStyle(WindowHandle);

    if ((windowStyle & CBS_DROPDOWNLIST) != CBS_DROPDOWNLIST || (windowStyle & CBS_DROPDOWN) != CBS_DROPDOWN)
    {
        COMBOBOXINFO info = { sizeof(COMBOBOXINFO) };

        if (CallWindowProc(WindowProcedure, WindowHandle, CB_GETCOMBOBOXINFO, 0, (LPARAM)&info))
        {
            //INT borderSize = 0;
            //if (Context->ThemeHandle)
            //{
            //    if (PhGetThemeInt(Context->ThemeHandle, 0, 0, TMT_BORDERSIZE, &borderSize))
            //    {
            //        borderSize = borderSize * 2;
            //    }
            //}

            ExcludeClipRect(
                Hdc,
                info.rcItem.left,
                info.rcItem.top,
                info.rcItem.right,
                info.rcItem.bottom
                );
        }
    }
}

LRESULT CALLBACK PhpThemeWindowComboBoxControlSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_THEME_WINDOW_COMBO_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(WindowHandle, LONG_MAX)))
        return FALSE;

    oldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            PhSetWindowProcedure(WindowHandle, oldWndProc);
            PhRemoveWindowContext(WindowHandle, LONG_MAX);

            if (context->ThemeHandle)
            {
                PhCloseThemeData(context->ThemeHandle);
            }

            PhFree(context);
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
    case WM_THEMECHANGED:
        {
            if (context->ThemeHandle)
            {
                PhCloseThemeData(context->ThemeHandle);
                context->ThemeHandle = NULL;
            }

            context->WindowDpi = PhGetWindowDpi(WindowHandle);
            context->ThemeHandle = PhOpenThemeData(WindowHandle, VSCLASS_COMBOBOX, context->WindowDpi);
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
            context->CursorPos.x = LONG_MIN;
            context->CursorPos.y = LONG_MIN;
            context->MouseActive = FALSE;
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
            //DEBUG_BEGINPAINT_RECT(WindowHandle, ps.rcPaint);
            //
            //if (BufferedPaint = BeginBufferedPaint(ps.hdc, &ps.rcPaint, BPBF_COMPATIBLEBITMAP, NULL, &BufferedHDC))
            //{
            //    ThemeWindowComboBoxExcludeRect(WindowHandle, ps.hdc, &ps.rcPaint, oldWndProc, context);
            //    ThemeWindowRenderComboBox(WindowHandle, BufferedHDC, &ps.rcPaint, oldWndProc, context);
            //    EndBufferedPaint(BufferedPaint, TRUE);
            //}
            //else
            {
                PAINTSTRUCT paintStruct;
                RECT clientRect;
                HDC hdc;
                PH_BUFFERED_PAINT paintBuffer;
                HDC bufferDc;

                if (!(hdc = BeginPaint(WindowHandle, &paintStruct)))
                    break;

                if (!PhGetClientRect(WindowHandle, &clientRect))
                {
                    EndPaint(WindowHandle, &paintStruct);
                    break;
                }

                ThemeWindowComboBoxExcludeRect(context, WindowHandle, hdc, &clientRect, oldWndProc);

                if (PhBeginBufferedPaint(hdc, &paintStruct.rcPaint, &paintBuffer, &bufferDc))
                {
                    ThemeWindowRenderComboBox(context, WindowHandle, bufferDc, &clientRect, oldWndProc);
                    PhEndBufferedPaint(&paintBuffer, TRUE);
                }
                else
                {
                    ThemeWindowRenderComboBox(context, WindowHandle, hdc, &clientRect, oldWndProc);
                }

                EndPaint(WindowHandle, &paintStruct);
            }
        }
        return 0;
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
}

LRESULT CALLBACK PhpThemeWindowACLUISubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    WNDPROC oldWndProc;

    if (!(oldWndProc = PhGetWindowContext(WindowHandle, LONG_MAX)))
        return FALSE;

    switch (uMsg)
    {
    case WM_VSCROLL:
    case WM_MOUSEWHEEL:
        InvalidateRect(WindowHandle, NULL, FALSE);
        break;
    case WM_NOTIFY:
        {
            LPNMHDR data = (LPNMHDR)lParam;

            if (data->code == NM_CUSTOMDRAW)
            {
                LPNMCUSTOMDRAW customDraw = (LPNMCUSTOMDRAW)lParam;
                WCHAR className[MAX_PATH];

                if (customDraw->dwDrawStage == CDDS_PREPAINT && !(customDraw->uItemState & CDIS_FOCUS))
                {
                    if (!NT_SUCCESS(PhGetClassName(customDraw->hdr.hwndFrom, className, RTL_NUMBER_OF(className), NULL)))
                        className[0] = UNICODE_NULL;
                    if (PhEqualStringZ(className, WC_BUTTON, FALSE))
                    {
                        HDC hdc = GetDC(WindowHandle);
                        RECT rectControl = customDraw->rc;
                        PhInflateRect(&rectControl, 2, 2);
                        MapWindowRect(customDraw->hdr.hwndFrom, WindowHandle, &rectControl);
                        FillRect(hdc, &rectControl, PhThemeWindowBackgroundBrush);   // fix the annoying white border left by the previous active control
                        ReleaseDC(WindowHandle, hdc);
                    }
                }
            }
        }
        break;
    case WM_NCDESTROY:
        {
            PhSetWindowProcedure(WindowHandle, oldWndProc);
            PhRemoveWindowContext(WindowHandle, LONG_MAX);
        }
        break;
    case WM_ERASEBKGND:
        {
            HDC hdc = (HDC)wParam;
            RECT clientRect;

            GetClipBox(hdc, &clientRect);
            FillRect(hdc, &clientRect, PhThemeWindowBackgroundBrush);
        }
        return TRUE;
    case WM_CTLCOLORSTATIC:
        {
            HDC hdc = (HDC)wParam;

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, PhThemeWindowTextColor);
            return (INT_PTR)PhThemeWindowBackgroundBrush;
        }
    }
    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
}

VOID PhpThemeWindowEditCreateBufferedContext(
    _In_ PPHP_THEME_WINDOW_EDIT_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PRECT BufferRect
    )
{
    Context->BufferedDc = CreateCompatibleDC(Hdc);

    if (!Context->BufferedDc)
        return;

    Context->BufferedContextRect = *BufferRect;
    Context->BufferedBitmap = CreateCompatibleBitmap(
        Hdc,
        Context->BufferedContextRect.right,
        Context->BufferedContextRect.bottom
        );

    Context->BufferedOldBitmap = SelectBitmap(Context->BufferedDc, Context->BufferedBitmap);
}

VOID PhpThemeWindowEditDestroyBufferedContext(
    _In_ PPHP_THEME_WINDOW_EDIT_CONTEXT Context
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

VOID PhpThemeWindowEditThemeChanged(
    _In_ PPHP_THEME_WINDOW_EDIT_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    Context->WindowBrush = PhThemeWindowBackgroundBrush;
    Context->FrameBrush = PhpStockDCBrush; // Will use DCBrushColor

    Context->BorderSize = PhGetSystemMetrics(SM_CXBORDER, Context->WindowDpi);
}

LRESULT CALLBACK PhEditBorderWndSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_THEME_WINDOW_EDIT_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(WindowHandle, SHRT_MAX)))
        return 0;

    oldWndProc = context->DefaultWindowProc;

    switch (WindowMessage)
    {
    case WM_NCDESTROY:
        {
            PhSetWindowProcedure(WindowHandle, oldWndProc);
            PhRemoveWindowContext(WindowHandle, SHRT_MAX);

            PhpThemeWindowEditDestroyBufferedContext(context);
            PhFree(context);
        }
        break;

    case WM_NCPAINT:
        {
            RECT windowRect;
            RECT windowRectStart;
            RECT bufferRect;
            LONG width;
            LONG height;
            HDC hdc;
            HRGN updateRegion;
            ULONG flags;

            if (!PhGetWindowRect(WindowHandle, &windowRect))
                break;

            width = windowRect.right - windowRect.left;
            height = windowRect.bottom - windowRect.top;

            if (width <= 0 || height <= 0)
                break;

            updateRegion = (HRGN)wParam;
            if (updateRegion == HRGN_FULL)
                updateRegion = NULL;

            flags = DCX_WINDOW | DCX_CACHE | DCX_USESTYLE;

            if (updateRegion)
                flags |= DCX_INTERSECTRGN | DCX_NODELETERGN;

            hdc = GetDCEx(WindowHandle, updateRegion, flags);
            if (!hdc)
                break;

            //
            // Normalize window coordinates → (0,0)
            //

            PhOffsetRect(&windowRect, -windowRect.left, -windowRect.top);
            windowRectStart = windowRect;

            bufferRect.left = 0;
            bufferRect.top = 0;
            bufferRect.right = width;
            bufferRect.bottom = height;

            if (context->BufferedDc && (
                context->BufferedContextRect.right < bufferRect.right ||
                context->BufferedContextRect.bottom < bufferRect.bottom))
            {
                PhpThemeWindowEditDestroyBufferedContext(context);
            }

            if (!context->BufferedDc)
                PhpThemeWindowEditCreateBufferedContext(context, hdc, &bufferRect);

            if (!context->BufferedDc)
                goto Cleanup;

            //
            // Exclude client area from NC drawing
            //

            ExcludeClipRect(
                hdc,
                windowRect.left + (context->BorderSize + 1),
                windowRect.top + (context->BorderSize + 1),
                windowRect.right - (context->BorderSize + 1),
                windowRect.bottom - (context->BorderSize + 1)
                );

            //
            // NC Frame
            //

            if (GetFocus() == WindowHandle)
            {
                SetDCBrushColor(context->BufferedDc, PhThemeWindowFocusBorderColor);
                FrameRect(context->BufferedDc, &windowRect, PhpStockDCBrush);
                PhInflateRect(&windowRect, -1, -1);
                FrameRect(context->BufferedDc, &windowRect, context->WindowBrush);
            }
            else if (context->Hot)
            {
                SetDCBrushColor(context->BufferedDc, PhThemeWindowEditHotBorderColor);

                FrameRect(context->BufferedDc, &windowRect, PhpStockDCBrush);
                PhInflateRect(&windowRect, -1, -1);
                FrameRect(context->BufferedDc, &windowRect, context->WindowBrush);
            }
            else
            {
                SetDCBrushColor(context->BufferedDc, PhThemeWindowEditNormalBorderColor);

                FrameRect(context->BufferedDc, &windowRect, context->FrameBrush);
                PhInflateRect(&windowRect, -1, -1);
                FrameRect(context->BufferedDc, &windowRect, context->WindowBrush);
            }

            //
            // Commit to the target DC.
            //

            BitBlt(hdc, 0, 0, bufferRect.right, bufferRect.bottom, context->BufferedDc, 0, 0, SRCCOPY);

        Cleanup:
            ReleaseDC(WindowHandle, hdc);
        }
        return 0;

    case WM_MOUSEMOVE:
        {
            if (!context->MouseActive)
            {
                TRACKMOUSEEVENT trackEvent = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, WindowHandle, 0 };
                TrackMouseEvent(&trackEvent);
                context->MouseActive = TRUE;

                if (!context->Hot)
                {
                    context->Hot = TRUE;
                    RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
                }
            }
        }
        break;
    case WM_MOUSELEAVE:
        {
            context->MouseActive = FALSE;

            if (!context->NonMouseActive && context->Hot)
            {
                context->Hot = FALSE;
                RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            }
        }
        break;
    case WM_NCMOUSEMOVE:
        {
            if (!context->NonMouseActive)
            {
                TRACKMOUSEEVENT trackEvent = { sizeof(TRACKMOUSEEVENT), TME_LEAVE | TME_NONCLIENT, WindowHandle, 0 };
                TrackMouseEvent(&trackEvent);
                context->NonMouseActive = TRUE;

                if (!context->Hot)
                {
                    context->Hot = TRUE;
                    RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
                }
            }
        }
        break;
    case WM_NCMOUSELEAVE:
        {
            context->NonMouseActive = FALSE;

            if (!context->MouseActive && context->Hot)
            {
                context->Hot = FALSE;
                RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            }
        }
        break;

    case WM_SETFOCUS:
        {
            context->WindowFocus = TRUE;
            context->PreviousFocusWindowHandle = (HWND)wParam;
        }
        break;
    case WM_KILLFOCUS:
        {
            context->WindowFocus = FALSE;

            RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_SETTINGCHANGE:
    case WM_SYSCOLORCHANGE:
    case WM_THEMECHANGED:
        {
            PhpThemeWindowEditThemeChanged(context, WindowHandle);

            SetWindowPos(WindowHandle, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
            context->WindowDpi = PhGetWindowDpi(context->ParentWindowHandle);

            PhpThemeWindowEditThemeChanged(context, WindowHandle);

            SetWindowPos(WindowHandle, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    }

    return CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);
}
