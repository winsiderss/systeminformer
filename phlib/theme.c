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

typedef struct _PHP_THEME_WINDOW_GROUPBOX_CONTEXT
{
    WNDPROC DefaultWindowProc;
    LONG WindowDpi;
} PHP_THEME_WINDOW_GROUPBOX_CONTEXT, *PPHP_THEME_WINDOW_GROUPBOX_CONTEXT;

typedef struct _PHP_THEME_WINDOW_STATUSBAR_CONTEXT
{
    WNDPROC DefaultWindowProc;
    LONG WindowDpi;
    // legacy bitfield replaced by explicit flags (status kept in Flags field when needed)
    BOOLEAN MouseActive;
    POINT CursorPos;

    HTHEME ThemeHandle;
    ULONG Flags; // status flags for statusbar context (bitfield replacement)
    HRGN NcPaintRegion; // scratch region reused across WM_NCPAINT (dmex)
} PHP_THEME_WINDOW_STATUSBAR_CONTEXT, *PPHP_THEME_WINDOW_STATUSBAR_CONTEXT;

#define PHP_THEME_STATUSBAR_FLAG_HOT (1u << 3)

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

    struct
    {
        ULONG Hot : 1;
        ULONG HotTrack : 1;
        ULONG WindowFocus : 1;
        ULONG ReadOnly : 1;
        ULONG Multiline : 1;
        ULONG Spare : 27;
    };

    HWND PreviousFocusWindowHandle;

    HBRUSH WindowBrush;
    HBRUSH FrameBrush;
    LONG BorderSize;
    HRGN NcPaintRegion; // scratch region reused across WM_NCPAINT (dmex)
} PHP_THEME_WINDOW_EDIT_CONTEXT, *PPHP_THEME_WINDOW_EDIT_CONTEXT;

typedef struct _PHP_THEME_WINDOW_PROGRESS_CONTEXT
{
    WNDPROC DefaultWindowProc;
    LONG WindowDpi;
} PHP_THEME_WINDOW_PROGRESS_CONTEXT, *PPHP_THEME_WINDOW_PROGRESS_CONTEXT;

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

typedef struct _PHP_THEME_WINDOW_ENUM_CONTEXT
{
    BOOLEAN Reinitialize;
} PHP_THEME_WINDOW_ENUM_CONTEXT, *PPHP_THEME_WINDOW_ENUM_CONTEXT;

VOID PhpApplyThemeWindow(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN Reinitialize
    );

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

_Function_class_(PH_WINDOW_ENUM_CALLBACK)
static BOOLEAN CALLBACK PhpThemeWindowForwardSysColorChangeCallback(
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

LRESULT CALLBACK PhpThemeWindowProgressBarSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhpThemeWindowEditThemeChanged(
    _In_ PPHP_THEME_WINDOW_EDIT_CONTEXT Context,
    _In_ HWND WindowHandle
    );

VOID PhpThemeWindowEditRedrawFrame(
    _In_ HWND WindowHandle
    );

VOID PhpThemeWindowEditExcludeClient(
    _In_ PPHP_THEME_WINDOW_EDIT_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ PRECT WindowRect,
    _In_ PRECT ScreenWindowRect
    );

VOID PhpThemeWindowEditPaintNativeFrame(
    _In_ PPHP_THEME_WINDOW_EDIT_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ WNDPROC DefaultWindowProc,
    _In_ WPARAM wParam
    );

BOOLEAN PhpThemeWindowEditPaintFrame(
    _In_ PPHP_THEME_WINDOW_EDIT_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ WPARAM wParam
    );

VOID ThemeWindowRenderClippedGroupBoxControl(
    _In_ HWND WindowHandle,
    _In_ HDC BufferDc,
    _In_ PRECT ClientRect
    );

VOID ThemeWindowRenderProgressBarControl(
    _In_ HWND WindowHandle,
    _In_ HDC BufferDc,
    _In_ PRECT ClientRect
    );

VOID ThemeWindowRenderTabControl(
    _In_ PPHP_THEME_WINDOW_TAB_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC BufferDc,
    _In_ PRECT ClientRect,
    _In_ WNDPROC WindowProcedure
    );

VOID ThemeWindowComboBoxExcludeRect(
    _In_ PPHP_THEME_WINDOW_COMBO_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ PRECT ClientRect,
    _In_ WNDPROC WindowProcedure
    );

VOID ThemeWindowRenderComboBox(
    _In_ PPHP_THEME_WINDOW_COMBO_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC BufferDc,
    _In_ PRECT ClientRect,
    _In_ WNDPROC WindowProcedure
    );

VOID PhpThemeWindowEditUpdateFrameStyle(
    _In_ PPHP_THEME_WINDOW_EDIT_CONTEXT Context,
    _In_ HWND WindowHandle
    );

VOID PhpUninitializeWindowTheme(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN IncludeProcessWindows,
    _In_ BOOLEAN RedrawRoot
    );

VOID PhWindowThemeSetDarkMode(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN EnableDarkMode
    );
LRESULT CALLBACK PhpThemeWindowDrawListViewGroup(
    _In_ LPNMLVCUSTOMDRAW DrawInfo
);
VOID PhpThemeRefreshBackgroundBrush(
    _In_ COLORREF BackgroundColor
);
VOID PhpThemeCopyPaletteToGlobals(
    _In_ const PH_WINDOW_THEME_PALETTE* Palette
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
    assert(PhEqualRect(&rect, &(RcPaint)));\
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

// Cached NULL brush returned by PhGetStockBrush(NULL_BRUSH)
static HBRUSH PhpStockNullBrush = NULL;

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

// Windows 11 File Explorer colors. The Highlight/Focus/border entries are
// placeholders replaced with accent-derived colors by PhpResolveExplorerPalette
// when the user's accent color is available.
static CONST PH_WINDOW_THEME_PALETTE PhpWindowThemeExplorerDarkPalette =
{
    RGB(32, 32, 32),    // ForegroundColor
    RGB(32, 32, 32),    // BackgroundColor
    RGB(43, 43, 43),    // Background2Color
    RGB(61, 61, 61),    // HighlightColor
    RGB(74, 74, 74),    // Highlight2Color
    RGB(255, 255, 255), // TextColor
    RGB(154, 154, 154), // DisabledTextColor
    RGB(56, 56, 56),    // BorderColor
    RGB(56, 56, 56),    // PressedColor
    RGB(45, 45, 45),    // EditColor
    RGB(32, 32, 32),    // ScrollbarColor
    RGB(222, 222, 222), // DropdownGlyphColor
    RGB(0, 120, 215),   // WindowActiveBorderColor
    RGB(56, 56, 56),    // WindowInactiveBorderColor
    RGB(255, 0, 0),     // FilteredBorderColor
    RGB(255, 128, 0),   // ProtectedBorderColor
    RGB(0, 120, 215),   // FocusBorderColor
    RGB(56, 56, 56),    // GroupBoxFrameColor
    RGB(56, 56, 56),    // WindowFrameColor
    RGB(0, 120, 215),   // EditHotBorderColor
    RGB(56, 56, 56),    // EditNormalBorderColor
    RGB(255, 255, 255), // MenuSelectedTextColor
    RGB(154, 154, 154)  // MenuDisabledTextColor
};

static CONST PH_WINDOW_THEME_PALETTE PhpWindowThemeExplorerLightPalette =
{
    RGB(255, 255, 255), // ForegroundColor
    RGB(255, 255, 255), // BackgroundColor
    RGB(243, 243, 243), // Background2Color
    RGB(217, 217, 217), // HighlightColor
    RGB(234, 234, 234), // Highlight2Color
    RGB(0, 0, 0),       // TextColor
    RGB(109, 109, 109), // DisabledTextColor
    RGB(229, 229, 229), // BorderColor
    RGB(234, 234, 234), // PressedColor
    RGB(255, 255, 255), // EditColor
    RGB(243, 243, 243), // ScrollbarColor
    RGB(60, 60, 60),    // DropdownGlyphColor
    RGB(0, 120, 215),   // WindowActiveBorderColor
    RGB(229, 229, 229), // WindowInactiveBorderColor
    RGB(255, 0, 0),     // FilteredBorderColor
    RGB(255, 128, 0),   // ProtectedBorderColor
    RGB(0, 120, 215),   // FocusBorderColor
    RGB(229, 229, 229), // GroupBoxFrameColor
    RGB(229, 229, 229), // WindowFrameColor
    RGB(0, 120, 215),   // EditHotBorderColor
    RGB(229, 229, 229), // EditNormalBorderColor
    RGB(255, 255, 255), // MenuSelectedTextColor
    RGB(109, 109, 109)  // MenuDisabledTextColor
};

static PH_WINDOW_THEME_PALETTE PhpWindowThemeExplorerPalette = { 0 };
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

// Reads the user's accent color (the same value DWM and Explorer use for
// selection tinting). The registry value is stored as 0xAABBGGRR while COLORREF
// is 0x00BBGGRR with the red and blue channels swapped relative to the DWM
// layout. Returns FALSE when the value is missing so callers keep their
// neutral fallback colors.
static BOOLEAN PhpQueryWindowsAccentColor(
    _Out_ COLORREF* AccentColor
    )
{
    static CONST PH_STRINGREF keyPath = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\DWM");
    HANDLE keyHandle;
    BOOLEAN success = FALSE;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_CURRENT_USER,
        &keyPath,
        0
        )))
    {
        ULONG accentColor;

        if (accentColor = PhQueryRegistryUlongZ(keyHandle, L"AccentColor"))
        {
            *AccentColor = RGB(
                (BYTE)(accentColor),
                (BYTE)(accentColor >> 8),
                (BYTE)(accentColor >> 16)
                );
            success = TRUE;
        }

        NtClose(keyHandle);
    }

    return success;
}

// Composites Foreground over Background with the specified alpha (0-255).
static COLORREF PhpBlendColor(
    _In_ COLORREF Background,
    _In_ COLORREF Foreground,
    _In_ ULONG Alpha
    )
{
    ULONG inverse = 255 - Alpha;

    return RGB(
        (BYTE)((GetRValue(Foreground) * Alpha + GetRValue(Background) * inverse) / 255),
        (BYTE)((GetGValue(Foreground) * Alpha + GetGValue(Background) * inverse) / 255),
        (BYTE)((GetBValue(Foreground) * Alpha + GetBValue(Background) * inverse) / 255)
        );
}

// Builds the Explorer palette for the current light/dark preference, tinting the
// selection and focus colors with the user's accent color the way Explorer does.
// Re-resolved on every palette selection so accent and light/dark changes apply.
static VOID PhpResolveExplorerPalette(
    _Out_ PPH_WINDOW_THEME_PALETTE Palette,
    _In_ BOOLEAN DarkMode
    )
{
    COLORREF accentColor;

    *Palette = DarkMode ? PhpWindowThemeExplorerDarkPalette : PhpWindowThemeExplorerLightPalette;

    if (!PhpQueryWindowsAccentColor(&accentColor))
        return;

    Palette->HighlightColor = PhpBlendColor(Palette->BackgroundColor, accentColor, DarkMode ? 90 : 64);
    Palette->Highlight2Color = PhpBlendColor(Palette->BackgroundColor, accentColor, DarkMode ? 128 : 90);
    Palette->FocusBorderColor = accentColor;
    Palette->WindowActiveBorderColor = accentColor;
    Palette->EditHotBorderColor = accentColor;
}

typedef struct _PHP_THEME_WINDOW_CLASS_BRUSH_CONTEXT
{
    HBRUSH PreviousBrush;
} PHP_THEME_WINDOW_CLASS_BRUSH_CONTEXT, *PPHP_THEME_WINDOW_CLASS_BRUSH_CONTEXT;

_Function_class_(PH_WINDOW_ENUM_CALLBACK)
static BOOLEAN CALLBACK PhpUpdateThemeWindowClassBrushEnumChildWindows(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    PPHP_THEME_WINDOW_CLASS_BRUSH_CONTEXT context = Context;
    HBRUSH classBrush;

    PhEnumChildWindows(
        WindowHandle,
        PhpUpdateThemeWindowClassBrushEnumChildWindows,
        Context
        );

    classBrush = (HBRUSH)GetClassLongPtr(WindowHandle, GCLP_HBRBACKGROUND);

    if (classBrush == context->PreviousBrush)
    {
        SetClassLongPtr(WindowHandle, GCLP_HBRBACKGROUND, (LONG_PTR)PhThemeWindowBackgroundBrush);
    }

    if (PhGetWindowProcedure(WindowHandle) == PhEditBorderWndSubclassProc)
    {
        PPHP_THEME_WINDOW_EDIT_CONTEXT editContext;

        if (editContext = PhGetWindowContext(WindowHandle, SHRT_MAX))
        {
            if (editContext->WindowBrush == context->PreviousBrush)
                editContext->WindowBrush = PhThemeWindowBackgroundBrush;

            editContext->FrameBrush = PhpStockDCBrush;
        }
    }

    return TRUE;
}

static VOID PhpUpdateThemeWindowClassBrushes(
    _In_ HBRUSH PreviousBrush
    )
{
    PHP_THEME_WINDOW_CLASS_BRUSH_CONTEXT context;
    HWND currentWindow = NULL;

    if (!PreviousBrush)
        return;

    context.PreviousBrush = PreviousBrush;

    do
    {
        if (currentWindow = FindWindowEx(NULL, currentWindow, NULL, NULL))
        {
            ULONG processID = 0;

            GetWindowThreadProcessId(currentWindow, &processID);

            if (UlongToHandle(processID) == NtCurrentProcessId())
            {
                PhpUpdateThemeWindowClassBrushEnumChildWindows(
                    currentWindow,
                    &context
                    );
            }
        }
    } while (currentWindow);
}

VOID PhpApplyWindowThemePalette(
    _In_ const PH_WINDOW_THEME_PALETTE* Palette
    )
{
    PhpThemeCopyPaletteToGlobals(Palette);
    PhpThemeRefreshBackgroundBrush(Palette->BackgroundColor);
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
    case PhWindowThemeExplorer:
        PhpResolveExplorerPalette(&PhpWindowThemeExplorerPalette, PhQueryWindowsUseDarkMode());
        selectedPalette = &PhpWindowThemeExplorerPalette;
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

typedef VOID (CALLBACK *PPHP_THEME_PAINT_CALLBACK)(
    _In_ HWND WindowHandle,
    _In_ HDC PaintDc,
    _In_ PRECT ClientRect,
    _In_opt_ PVOID Context
    );

typedef struct _PHP_THEME_TAB_PAINT_CONTEXT
{
    PPHP_THEME_WINDOW_TAB_CONTEXT Context;
    WNDPROC WindowProcedure;
} PHP_THEME_TAB_PAINT_CONTEXT, *PPHP_THEME_TAB_PAINT_CONTEXT;

typedef struct _PHP_THEME_COMBO_PAINT_CONTEXT
{
    PPHP_THEME_WINDOW_COMBO_CONTEXT Context;
    WNDPROC WindowProcedure;
} PHP_THEME_COMBO_PAINT_CONTEXT, *PPHP_THEME_COMBO_PAINT_CONTEXT;

static VOID PhpThemeEnsureBackgroundBrush(
    VOID
    )
{
    if (!PhThemeWindowBackgroundBrush)
        PhThemeWindowBackgroundBrush = CreateSolidBrush(PhThemeWindowBackgroundColor);
}

/**
 * Determines whether the current window palette is a dark one.
 *
 * \return TRUE when theme support is enabled and the window background color is dark.
 */
BOOLEAN PhThemeWindowUseDarkBackground(
    VOID
    )
{
    if (!PhEnableThemeSupport)
        return FALSE;

    return PhGetColorBrightness(PhThemeWindowBackgroundColor) < 128;
}

/**
 * Retrieves the brush that paints the background of a themed top-level window.
 *
 * \return The themed background brush, or the system COLOR_BTNFACE brush when the
 * palette is light or theme support is disabled.
 * \remarks Never returns NULL, so callers can use the result as a window class
 * background without leaving the client area unpainted (a white first frame).
 */
HBRUSH PhGetThemeWindowBackgroundBrush(
    VOID
    )
{
    if (PhThemeWindowUseDarkBackground())
    {
        PhpThemeEnsureBackgroundBrush();

        return PhThemeWindowBackgroundBrush;
    }

    // A real brush handle (rather than the COLOR_BTNFACE + 1 class-brush encoding)
    // so the result is equally valid for FillRect. System brushes are cached by
    // the system and must not be deleted. (dmex)
    return GetSysColorBrush(COLOR_BTNFACE);
}

/**
 * Re-points the window class background brush of \a WindowHandle at the current
 * themed background brush.
 *
 * \param WindowHandle Handle to a window whose class background should be updated.
 */
VOID PhUpdateWindowClassBackground(
    _In_ HWND WindowHandle
    )
{
    SetClassLongPtr(WindowHandle, GCLP_HBRBACKGROUND, (LONG_PTR)PhGetThemeWindowBackgroundBrush());
}

VOID PhpThemeCopyPaletteToGlobals(
    _In_ const PH_WINDOW_THEME_PALETTE* Palette
    )
{
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
}

VOID PhpThemeRefreshBackgroundBrush(
    _In_ COLORREF BackgroundColor
    )
{
    HBRUSH previousBrush;

    previousBrush = PhThemeWindowBackgroundBrush;
    PhThemeWindowBackgroundBrush = CreateSolidBrush(BackgroundColor);

    PhpUpdateThemeWindowClassBrushes(previousBrush);

    if (previousBrush)
        DeleteBrush(previousBrush);
}

VOID PhpThemeRestoreSubclassWindowProcedure(
    _In_ HWND WindowHandle,
    _In_ WNDPROC DefaultWindowProc,
    _In_ ULONG ContextTag
    )
{
    PhSetWindowProcedure(WindowHandle, DefaultWindowProc);
    PhRemoveWindowContext(WindowHandle, ContextTag);
}

static VOID PhpThemePaintBufferedWindow(
    _In_ HWND WindowHandle,
    _In_ PPHP_THEME_PAINT_CALLBACK PaintCallback,
    _In_opt_ PVOID Context
    )
{
    PAINTSTRUCT paintStruct;
    RECT clientRect;
    HDC hdc;
    HDC bufferDc;
    PH_BUFFERED_PAINT paintBuffer;

    hdc = BeginPaint(WindowHandle, &paintStruct);
    if (!hdc)
        return;

    if (!PaintCallback)
    {
        EndPaint(WindowHandle, &paintStruct);
        return;
    }

    if (!PhGetClientRect(WindowHandle, &clientRect))
    {
        EndPaint(WindowHandle, &paintStruct);
        return;
    }

    if (PhBeginBufferedPaint(hdc, &paintStruct.rcPaint, PHBF_TOPDOWNDIB, NULL, &paintBuffer, &bufferDc))
    {
        if (PaintCallback)
            PaintCallback(WindowHandle, bufferDc, &clientRect, Context);
        PhEndBufferedPaint(&paintBuffer, TRUE);
    }
    else
    {
        if (PaintCallback)
            PaintCallback(WindowHandle, hdc, &clientRect, Context);
    }

    EndPaint(WindowHandle, &paintStruct);
}

static LRESULT PhpThemeWindowHandleCustomDraw(
    _In_ LPNMCUSTOMDRAW CustomDraw
    )
{
    WCHAR className[MAX_PATH];

    if (!NT_SUCCESS(PhGetClassName(CustomDraw->hdr.hwndFrom, className, RTL_NUMBER_OF(className), NULL)))
        className[0] = UNICODE_NULL;

    if (PhEqualStringZ(className, WC_BUTTON, FALSE))
        return PhThemeWindowDrawButton(CustomDraw);

    if (PhEqualStringZ(className, REBARCLASSNAME, FALSE))
        return PhThemeWindowDrawRebar(CustomDraw);

    if (PhEqualStringZ(className, TOOLBARCLASSNAME, FALSE))
        return PhThemeWindowDrawToolbar((LPNMTBCUSTOMDRAW)CustomDraw);

    if (PhEqualStringZ(className, WC_LISTVIEW, FALSE))
    {
        LPNMLVCUSTOMDRAW listViewCustomDraw = (LPNMLVCUSTOMDRAW)CustomDraw;

        if (listViewCustomDraw->dwItemType == LVCDI_GROUP)
            return PhpThemeWindowDrawListViewGroup(listViewCustomDraw);
    }

    return CDRF_DODEFAULT;
}

static VOID CALLBACK PhpThemePaintGroupBoxCallback(
    _In_ HWND WindowHandle,
    _In_ HDC PaintDc,
    _In_ PRECT ClientRect,
    _In_opt_ PVOID Context
    )
{
    UNREFERENCED_PARAMETER(Context);

    ThemeWindowRenderClippedGroupBoxControl(WindowHandle, PaintDc, ClientRect);
}

static VOID CALLBACK PhpThemePaintProgressBarCallback(
    _In_ HWND WindowHandle,
    _In_ HDC PaintDc,
    _In_ PRECT ClientRect,
    _In_opt_ PVOID Context
    )
{
    UNREFERENCED_PARAMETER(Context);

    ThemeWindowRenderProgressBarControl(WindowHandle, PaintDc, ClientRect);
}

static VOID CALLBACK PhpThemePaintTabControlCallback(
    _In_ HWND WindowHandle,
    _In_ HDC PaintDc,
    _In_ PRECT ClientRect,
    _In_opt_ PVOID Context
    )
{
    PPHP_THEME_TAB_PAINT_CONTEXT paintContext = Context;

    ThemeWindowRenderTabControl(
        paintContext->Context,
        WindowHandle,
        PaintDc,
        ClientRect,
        paintContext->WindowProcedure
        );
}

static VOID CALLBACK PhpThemePaintComboBoxCallback(
    _In_ HWND WindowHandle,
    _In_ HDC PaintDc,
    _In_ PRECT ClientRect,
    _In_opt_ PVOID Context
    )
{
    PPHP_THEME_COMBO_PAINT_CONTEXT paintContext = Context;

    ThemeWindowComboBoxExcludeRect(
        paintContext->Context,
        WindowHandle,
        PaintDc,
        ClientRect,
        paintContext->WindowProcedure
        );

    ThemeWindowRenderComboBox(
        paintContext->Context,
        WindowHandle,
        PaintDc,
        ClientRect,
        paintContext->WindowProcedure
        );
}

VOID PhInitializeWindowTheme(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN EnableThemeSupport
    )
{
    static PH_INITONCE paletteInitOnce = PH_INITONCE_INIT;

    PhBufferedPaintInit();

    PhAllowDarkModeForWindow(WindowHandle, TRUE);

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

    PhpThemeEnsureBackgroundBrush();

    if (!PhpStockNullBrush)
        PhpStockNullBrush = PhGetStockBrush(NULL_BRUSH);

    if (!PhpStockDCBrush)
        PhpStockDCBrush = PhGetStockBrush(DC_BRUSH);

    if (EnableThemeSupport)
    {
        WNDPROC defaultWindowProc;

        defaultWindowProc = PhGetWindowProcedure(WindowHandle);

        if (defaultWindowProc != PhpThemeWindowSubclassProc)
        {
            PhSetWindowContext(WindowHandle, LONG_MAX, defaultWindowProc);
            PhSetWindowProcedure(WindowHandle, PhpThemeWindowSubclassProc);

            if (WindowsVersion >= WINDOWS_10_RS5)
            {
                WCHAR windowClassName[MAX_PATH];

                if (NT_SUCCESS(PhGetClassName(WindowHandle, windowClassName, RTL_NUMBER_OF(windowClassName), NULL)))
                {
                    if (PhEqualStringZ(windowClassName, L"PhTreeNew", FALSE) || PhEqualStringZ(windowClassName, WC_LISTVIEW, FALSE))
                    {
                        PhAllowDarkModeForWindow(WindowHandle, TRUE);   // HACK for dynamically generated plugin tabs
                    }
                }
            }
        }

        PhEnumChildWindows(
            WindowHandle,
            PhpThemeWindowEnumChildWindows,
            NULL
            );

        // Nested dialogs may initialize here too. Let their invalid regions
        // coalesce instead of painting while ancestors are still being themed.
        RedrawWindow(WindowHandle, NULL, NULL,
            RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
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
    PhpUninitializeWindowTheme(WindowHandle, TRUE, TRUE);
}

VOID PhpUninitializeWindowTheme(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN IncludeProcessWindows,
    _In_ BOOLEAN RedrawRoot
    )
{
    HWND currentWindow = NULL;
    WCHAR windowClassName[MAX_PATH];

    if (!WindowHandle)
        return;

    if (!NT_SUCCESS(PhGetClassName(WindowHandle, windowClassName, RTL_NUMBER_OF(windowClassName), NULL)))
        windowClassName[0] = UNICODE_NULL;

    do
    {
        if (currentWindow = FindWindowEx(WindowHandle, currentWindow, NULL, NULL))
        {
            PhpUninitializeWindowTheme(currentWindow, FALSE, FALSE);
        }
    } while (currentWindow);

    PhInitializeThemeWindowFrame(WindowHandle);

    if (PhEqualStringZ(windowClassName, L"PhTreeNew", FALSE))
    {
        TreeNew_ThemeSupport(WindowHandle, FALSE);

        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            HWND tooltipWindow = TreeNew_GetTooltips(WindowHandle);

            HWND headerWindow;

            PhWindowThemeSetDarkMode(tooltipWindow, FALSE);
            PhWindowThemeSetDarkMode(WindowHandle, FALSE);
            PhAllowDarkModeForWindow(WindowHandle, FALSE);

            // Undo the dark item-view theme applied to the native headers.
            if (headerWindow = TreeNew_GetFixedHeader(WindowHandle))
            {
                PhAllowDarkModeForWindow(headerWindow, FALSE);
                PhSetControlTheme(headerWindow, L"Explorer");
                InvalidateRect(headerWindow, NULL, FALSE);
            }

            if (headerWindow = TreeNew_GetHeader(WindowHandle))
            {
                PhAllowDarkModeForWindow(headerWindow, FALSE);
                PhSetControlTheme(headerWindow, L"Explorer");
                InvalidateRect(headerWindow, NULL, FALSE);
            }
        }

        //PhSetControlTheme(WindowHandle, L"");
        PhSetWindowExStyle(WindowHandle, WS_EX_CLIENTEDGE, WS_EX_CLIENTEDGE);
        PhSetWindowFrameChanged(WindowHandle);
    }
    else if (PhEqualStringZ(windowClassName, WC_TABCONTROL, FALSE))
    {
        PPHP_THEME_WINDOW_TAB_CONTEXT context;

        if (
            PhGetWindowProcedure(WindowHandle) == PhpThemeWindowTabControlWndSubclassProc &&
            (context = PhGetWindowContext(WindowHandle, LONG_MAX))
            )
        {
            PhpThemeRestoreSubclassWindowProcedure(WindowHandle, context->DefaultWindowProc, LONG_MAX);
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
        //SendMessage(WindowHandle, WM_THEMECHANGED, 0, 0);
    }
    else if (PhEqualStringZ(windowClassName, L"PhTabNew", FALSE))
    {
        SendMessage(WindowHandle, WM_THEMECHANGED, 0, 0);
    }
    else if (PhEqualStringZ(windowClassName, L"PhHeaderNew", FALSE))
    {
        PhAllowDarkModeForWindow(WindowHandle, FALSE);
        SendMessage(WindowHandle, WM_THEMECHANGED, 0, 0);
    }
    else if (PhEqualStringZ(windowClassName, WC_LISTVIEW, FALSE))
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            HWND tooltipWindow = ListView_GetToolTips(WindowHandle);

            PhAllowDarkModeForWindow(WindowHandle, FALSE);
            PhSetControlTheme(WindowHandle, L"Explorer");
            PhWindowThemeSetDarkMode(tooltipWindow, FALSE);
        }

        PhSetWindowStyle(WindowHandle, WS_BORDER, WS_BORDER);
        PhSetWindowExStyle(WindowHandle, WS_EX_CLIENTEDGE, WS_EX_CLIENTEDGE);
        PhSetWindowFrameChanged(WindowHandle);

        ListView_SetBkColor(WindowHandle, CLR_NONE);
        ListView_SetTextBkColor(WindowHandle, CLR_NONE);
        ListView_SetTextColor(WindowHandle, CLR_DEFAULT);
    }
    else if (PhEqualStringZ(windowClassName, WC_TREEVIEW, FALSE))
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            HWND tooltipWindow = TreeView_GetToolTips(WindowHandle);

            PhWindowThemeSetDarkMode(WindowHandle, FALSE);
            PhWindowThemeSetDarkMode(tooltipWindow, FALSE);
        }

        TreeView_SetBkColor(WindowHandle, CLR_NONE);
        TreeView_SetTextColor(WindowHandle, CLR_DEFAULT);
    }
    else if (PhEqualStringZ(windowClassName, L"RICHEDIT50W", FALSE))
    {
        SendMessage(WindowHandle, WM_USER + 67, TRUE, 0);
        PhSetWindowStyle(WindowHandle, WS_BORDER, WS_BORDER);
        PhWindowThemeSetDarkMode(WindowHandle, FALSE);
        PhSetWindowFrameChanged(WindowHandle);
    }
    else if (
        PhEqualStringZ(windowClassName, WC_LISTBOX, FALSE) ||
        PhEqualStringZ(windowClassName, L"ComboLBox", FALSE)
        )
    {
        PPHP_THEME_WINDOW_STATUSBAR_CONTEXT context;

        if (WindowsVersion >= WINDOWS_10_RS5)
            PhWindowThemeSetDarkMode(WindowHandle, FALSE);

        if (
            PhGetWindowProcedure(WindowHandle) == (WNDPROC)PhpThemeWindowListBoxControlSubclassProc &&
            (context = PhGetWindowContext(WindowHandle, LONG_MAX))
            )
        {
            PhpThemeRestoreSubclassWindowProcedure(WindowHandle, context->DefaultWindowProc, LONG_MAX);

            if (context->ThemeHandle)
                PhCloseThemeData(context->ThemeHandle);

            PhFree(context);
        }
    }
    else if (PhEqualStringZ(windowClassName, WC_COMBOBOX, FALSE))
    {
        PPHP_THEME_WINDOW_COMBO_CONTEXT context;
        COMBOBOXINFO info = { sizeof(COMBOBOXINFO) };

        if (SendMessage(WindowHandle, CB_GETCOMBOBOXINFO, 0, (LPARAM)&info))
        {
            if (info.hwndList)
                PhWindowThemeSetDarkMode(info.hwndList, FALSE);
        }

        if (
            PhGetWindowProcedure(WindowHandle) == (WNDPROC)PhpThemeWindowComboBoxControlSubclassProc &&
            (context = PhGetWindowContext(WindowHandle, LONG_MAX))
            )
        {
            PhpThemeRestoreSubclassWindowProcedure(WindowHandle, context->DefaultWindowProc, LONG_MAX);

            if (context->ThemeHandle)
                PhCloseThemeData(context->ThemeHandle);

            PhFree(context);
        }

        //InvalidateRect(WindowHandle, NULL, FALSE);
    }
    else if (PhEqualStringZ(windowClassName, L"CHECKLIST_ACLUI", FALSE))
    {
        WNDPROC oldWndProc;

        if (WindowsVersion >= WINDOWS_10_RS5)
            PhWindowThemeSetDarkMode(WindowHandle, FALSE);

        if (
            GetWindowLongPtr(WindowHandle, GWLP_WNDPROC) == (LONG_PTR)PhpThemeWindowACLUISubclassProc &&
            (oldWndProc = PhGetWindowContext(WindowHandle, LONG_MAX))
            )
        {
            PhpThemeRestoreSubclassWindowProcedure(WindowHandle, oldWndProc, LONG_MAX);
        }
    }
    else if (PhEqualStringZ(windowClassName, WC_BUTTON, FALSE))
    {
        ULONG style = PhGetWindowStyle(WindowHandle);

        if ((style & BS_TYPEMASK) == BS_GROUPBOX)
        {
            PPHP_THEME_WINDOW_GROUPBOX_CONTEXT context;
            LONG_PTR windowProc = GetWindowLongPtr(WindowHandle, GWLP_WNDPROC);

            if (
                (windowProc == (LONG_PTR)PhpThemeWindowGroupBoxSubclassProc ||
                windowProc == (LONG_PTR)PhThemeWindowGroupBoxExSubclassProc) &&
                (context = PhGetWindowContext(WindowHandle, LONG_MAX))
                )
            {
                PhpThemeRestoreSubclassWindowProcedure(WindowHandle, context->DefaultWindowProc, LONG_MAX);
                PhFree(context);
            }
        }
        else
        {
            PhWindowThemeSetDarkMode(WindowHandle, FALSE);
        }
    }
    else if (PhEqualStringZ(windowClassName, WC_EDIT, FALSE))
    {
        PPHP_THEME_WINDOW_EDIT_CONTEXT context;

        if (PhGetWindowStyle(WindowHandle) & ES_MULTILINE)
            PhWindowThemeSetDarkMode(WindowHandle, FALSE);

        PhSetControlTheme(WindowHandle, L"Explorer");
        //SendMessage(WindowHandle, WM_THEMECHANGED, 0, 0);

        if (
            PhGetWindowProcedure(WindowHandle) == PhEditBorderWndSubclassProc &&
            (context = PhGetWindowContext(WindowHandle, SHRT_MAX))
            )
        {
            PhpThemeRestoreSubclassWindowProcedure(WindowHandle, context->DefaultWindowProc, SHRT_MAX);

            PhFree(context);
        }

        SetWindowPos(WindowHandle, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
    else if (PhEqualStringZ(windowClassName, WC_LINK, FALSE))
    {
        PhAllowDarkModeForWindow(WindowHandle, FALSE);
        PhSetControlTheme(WindowHandle, L"Explorer");
    }

    if (PhGetWindowContext(WindowHandle, LONG_MAX) && PhGetWindowProcedure(WindowHandle) == PhpThemeWindowSubclassProc)
    {
        WNDPROC oldWndProc = (WNDPROC)PhGetWindowContext(WindowHandle, LONG_MAX);

        PhpThemeRestoreSubclassWindowProcedure(WindowHandle, oldWndProc, LONG_MAX);
    }

    // Only roots request a repaint, after all their descendants are restyled.
    // Queue rather than synchronously painting during a process-wide transition.
    if (RedrawRoot)
        RedrawWindow(WindowHandle, NULL, NULL,
            RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);

    if (IncludeProcessWindows)
    {
        currentWindow = NULL;

        do
        {
            if (currentWindow = FindWindowEx(NULL, currentWindow, NULL, NULL))
            {
                ULONG processID = 0;

                GetWindowThreadProcessId(currentWindow, &processID);

                if (UlongToHandle(processID) == NtCurrentProcessId() && currentWindow != WindowHandle)
                {
                    PhpUninitializeWindowTheme(currentWindow, FALSE, TRUE);
                }
            }
        } while (currentWindow);
    }
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

// Reads the Windows transparency effects preference (Settings >
// Personalization > Colors). DWM does not composite the Mica/acrylic backdrop
// when this is off. Kept private: the applications carry their own copy of this
// query (SystemInformer/delayhook.c, tools/peview/delayhook.c) and phlib must
// not collide with those symbols at link time.
static BOOLEAN PhpQueryThemeTransparencyEnabled(
    VOID
    )
{
    static CONST PH_STRINGREF keyPath = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize");
    HANDLE keyHandle;
    BOOLEAN enableTransparency = TRUE;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_CURRENT_USER,
        &keyPath,
        0
        )))
    {
        enableTransparency = !!PhQueryRegistryUlongZ(keyHandle, L"EnableTransparency");
        NtClose(keyHandle);
    }

    return enableTransparency;
}

/**
 * Determines whether the client area may expose the Mica backdrop. The backdrop
 * is only requested by the Explorer theme, requires the Windows 11 22H2 system
 * backdrop support, and is not composited when the user disabled transparency.
 *
 * \return TRUE when callers may extend the frame into the client area.
 */
BOOLEAN PhWindowThemeSupportsMicaClient(
    VOID
    )
{
    if (!PhEnableThemeSupport)
        return FALSE;
    if (PhpWindowThemeCurrentId != PhWindowThemeExplorer)
        return FALSE;
    if (WindowsVersion < WINDOWS_11_22H2)
        return FALSE;

    return PhpQueryThemeTransparencyEnabled();
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
    case PhThemeModeExplorer:
        themeId = PhWindowThemeExplorer;
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
    PHP_THEME_WINDOW_ENUM_CONTEXT enumContext;
    HWND currentWindow = NULL;

    PhInitializeThemeWindowFrame(WindowHandle);

    // Re-point the class background brush at the new palette. PhpUpdateThemeWindowClassBrushes
    // only rewrites classes that still hold the *previous* themed brush, so it misses a window
    // sitting on the system COLOR_BTNFACE brush (a light -> dark switch, or theme support being
    // turned off entirely). Without this the client margins keep the old color. (dmex)
    PhUpdateWindowClassBackground(WindowHandle);

    if (!PhEnableThemeSupport)
        return;

    PhpThemeEnsureBackgroundBrush();

    enumContext.Reinitialize = TRUE;

    PhpApplyThemeWindow(WindowHandle, TRUE);

    PhEnumChildWindows(
        WindowHandle,
        PhpReInitializeThemeWindowEnumChildWindows,
        &enumContext
        );

    //RedrawWindow(WindowHandle, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);

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
                        PhpApplyThemeWindow(currentWindow, TRUE);

                        PhEnumChildWindows(
                            currentWindow,
                            PhpReInitializeThemeWindowEnumChildWindows,
                            &enumContext
                            );
                        //PhReInitializeWindowTheme(currentWindow);
                    }

                    //RedrawWindow(currentWindow, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
                }
            }
        }
    } while (currentWindow);

    //RedrawWindow(WindowHandle, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
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
#ifndef DWMWA_COLOR_DEFAULT
#define DWMWA_COLOR_DEFAULT 0xffffffff
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
    {
        if (WindowsVersion >= WINDOWS_11)
            return DWMWA_COLOR_DEFAULT;
        return 0;
    }

    if (IsHandleFiltered)
        return PhpWindowThemeCurrentPalette.FilteredBorderColor;

    if (IsProtectedProcess || IsIsolatedUserMode)
        return PhpWindowThemeCurrentPalette.ProtectedBorderColor;

    return IsActive
        ? PhpWindowThemeCurrentPalette.WindowActiveBorderColor
        : PhpWindowThemeCurrentPalette.WindowInactiveBorderColor;
}

/**
 * Extends the window frame into the client area so DWM composites the system
 * backdrop wherever the client pixels have an alpha of zero. Pass an all-zero
 * MARGINS to remove a previous extension.
 *
 * \param WindowHandle The window to extend the frame into.
 * \param Margins The client area margins to extend the frame into.
 * \return Successful or errant status.
 */
HRESULT PhSetWindowFrameMargins(
    _In_ HWND WindowHandle,
    _In_ const PH_WINDOW_MARGINS* Margins
    )
{
    C_ASSERT(sizeof(PH_WINDOW_MARGINS) == sizeof(MARGINS));
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static HRESULT (WINAPI* DwmExtendFrameIntoClientArea_I)(
        _In_ HWND WindowHandle,
        _In_ const MARGINS* Margins
        );

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"dwmapi.dll"))
        {
            DwmExtendFrameIntoClientArea_I = PhGetDllBaseProcedureAddress(baseAddress, "DwmExtendFrameIntoClientArea", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!DwmExtendFrameIntoClientArea_I)
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    return DwmExtendFrameIntoClientArea_I(WindowHandle, (const MARGINS*)Margins);
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
            // The Explorer theme follows the system light/dark preference and lets
            // DWM draw the caption so the Mica backdrop shows through.
            BOOLEAN explorerTheme = PhpWindowThemeCurrentId == PhWindowThemeExplorer;
            BOOLEAN darkMode = !explorerTheme || PhQueryWindowsUseDarkMode();

            PhAllowDarkModeForWindow(WindowHandle, darkMode);
            PhSetControlTheme(WindowHandle, darkMode ? L"DarkMode_Explorer" : L"Explorer");

            boolAttribute = !!darkMode;

            if (FAILED(PhSetWindowThemeAttribute(WindowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE, &boolAttribute, sizeof(BOOL))))
            {
                PhSetWindowThemeAttribute(WindowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &boolAttribute, sizeof(BOOL));
            }

            if (WindowsVersion >= WINDOWS_11)
            {
                COLORREF captionColor = explorerTheme ? DWMWA_COLOR_DEFAULT : PhThemeWindowBackgroundColor;

                PhSetWindowThemeAttribute(WindowHandle, DWMWA_CAPTION_COLOR, &captionColor, sizeof(COLORREF));
            }
        }
        else
        {
            COLORREF colorAttribute;

            PhAllowDarkModeForWindow(WindowHandle, FALSE);
            PhSetControlTheme(WindowHandle, L"Explorer");

            boolAttribute = FALSE;
            PhSetWindowThemeAttribute(WindowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE, &boolAttribute, sizeof(BOOL));
            PhSetWindowThemeAttribute(WindowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &boolAttribute, sizeof(BOOL));

            if (WindowsVersion >= WINDOWS_11)
            {
                colorAttribute = DWMWA_COLOR_DEFAULT;
                PhSetWindowThemeAttribute(WindowHandle, DWMWA_CAPTION_COLOR, &colorAttribute, sizeof(COLORREF));
                PhSetWindowThemeAttribute(WindowHandle, DWMWA_BORDER_COLOR, &colorAttribute, sizeof(COLORREF));
            }

            SetWindowPos(
                WindowHandle,
                NULL,
                0,
                0,
                0,
                0,
                SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED
                );
        }

        if (WindowsVersion >= WINDOWS_11_22H2)
        {
            // DWMSBT_MAINWINDOW (Mica) for the Explorer theme, DWMSBT_AUTO otherwise.
            ulongAttribute = (PhEnableThemeSupport && PhpWindowThemeCurrentId == PhWindowThemeExplorer) ? 2 : 1;
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
            SetBkMode(Hdc, TRANSPARENT);
            //SetDCBrushColor(Hdc, PHP_THEME_WINDOW_SCROLLBAR_COLOR);
            return PhThemeWindowBackgroundBrush;
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
        // Cover the entire gap between the bottom of the menu bar and the top
        // of the client area — a fixed 1px leaves a residual line at high DPI.
        RECT rcAnnoyingLine = clientRect;
        MENUBARINFO menuBarInfo = { sizeof(MENUBARINFO) };

        rcAnnoyingLine.bottom = rcAnnoyingLine.top;

        if (GetMenuBarInfo(WindowHandle, OBJID_MENU, 0, &menuBarInfo))
        {
            // Nothing to cover while the menu bar is hidden (for example during a
            // minimize or when the bar has no height). (dmex)
            if (menuBarInfo.rcBar.bottom <= menuBarInfo.rcBar.top)
                return;

            rcAnnoyingLine.top = menuBarInfo.rcBar.bottom - windowRect.top;
        }
        else
        {
            rcAnnoyingLine.top--;
        }

        if (rcAnnoyingLine.top >= rcAnnoyingLine.bottom)
            rcAnnoyingLine.top = rcAnnoyingLine.bottom - 1;

        if (PhRectEmpty(&rcAnnoyingLine))
            return;

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

    //InvalidateRect(TabControlWindow, NULL, FALSE);
}

VOID PhInitializeThemeWindowGroupBox(
    _In_ HWND GroupBoxHandle
    )
{
    PPHP_THEME_WINDOW_GROUPBOX_CONTEXT context;

    context = PhAllocateZero(sizeof(PHP_THEME_WINDOW_GROUPBOX_CONTEXT));
    context->DefaultWindowProc = PhGetWindowProcedure(GroupBoxHandle);
    context->WindowDpi = PhGetWindowDpi(GroupBoxHandle);
    PhSetWindowContext(GroupBoxHandle, LONG_MAX, context);
    PhSetWindowProcedure(GroupBoxHandle, PhpThemeWindowGroupBoxSubclassProc);

    //PhSetWindowStyle(GroupBoxHandle, WS_CLIPSIBLINGS, WS_CLIPSIBLINGS);

    //InvalidateRect(GroupBoxHandle, NULL, FALSE);
}

VOID PhInitializeThemeWindowGroupBoxEx(
    _In_ HWND GroupBoxHandle
    )
{
    PPHP_THEME_WINDOW_GROUPBOX_CONTEXT context;

    context = PhAllocateZero(sizeof(PHP_THEME_WINDOW_GROUPBOX_CONTEXT));
    context->DefaultWindowProc = PhGetWindowProcedure(GroupBoxHandle);
    context->WindowDpi = PhGetWindowDpi(GroupBoxHandle);
    PhSetWindowContext(GroupBoxHandle, LONG_MAX, context);
    PhSetWindowProcedure(GroupBoxHandle, PhThemeWindowGroupBoxExSubclassProc);

    //PhSetWindowStyle(GroupBoxHandle, WS_CLIPSIBLINGS, WS_CLIPSIBLINGS);

    //InvalidateRect(GroupBoxHandle, NULL, FALSE);
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

BOOLEAN PhThemeWindowUahWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _Out_ LRESULT *Result
    )
{
    UNREFERENCED_PARAMETER(WindowHandle);
    UNREFERENCED_PARAMETER(wParam);

    switch (WindowMessage)
    {
    case WM_UAHDRAWMENU:
        {
            PUAHMENU menu = (PUAHMENU)lParam;
            RECT clipRect;
            if (!menu || !menu->hdc)
                break;

            SetDCBrushColor(menu->hdc, PhThemeWindowBackgroundColor);
            if (GetClipBox(menu->hdc, &clipRect) > NULLREGION)
                FillRect(menu->hdc, &clipRect, PhThemeWindowBackgroundBrush);
            *Result = 0;
            return TRUE;
        }
    }

    return FALSE;
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

    //InvalidateRect(ListBoxControl, NULL, FALSE);
    //SetWindowPos(ListBoxControl, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
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

    //InvalidateRect(ComboBoxControl, NULL, FALSE);
}

VOID PhInitializeWindowThemeACLUI(
    _In_ HWND ACLUIControl
)
{
    PhSetWindowContext(ACLUIControl, LONG_MAX, PhGetWindowProcedure(ACLUIControl));
    PhSetWindowProcedure(ACLUIControl, PhpThemeWindowACLUISubclassProc);

    //InvalidateRect(ACLUIControl, NULL, FALSE);
}

VOID PhInitializeWindowThemeEditControl(
    _In_ HWND EditControl
    )
{
    PPHP_THEME_WINDOW_EDIT_CONTEXT context;
    WCHAR windowClassName[MAX_PATH];

    if (!NT_SUCCESS(PhGetClassName(EditControl, windowClassName, RTL_NUMBER_OF(windowClassName), NULL)))
        return;
    if (!PhEqualStringZ(windowClassName, WC_EDIT, FALSE))
        return;
    if (PhGetWindowContext(EditControl, SHRT_MAX))
        return;

    context = PhAllocateZero(sizeof(PHP_THEME_WINDOW_EDIT_CONTEXT));
    context->DefaultWindowProc = PhGetWindowProcedure(EditControl);
    context->ParentWindowHandle = GetParent(EditControl);
    context->WindowDpi = PhGetWindowDpi(EditControl);
    context->WindowFocus = GetFocus() == EditControl;
    context->BorderSize = PhGetSystemMetrics(SM_CXBORDER, context->WindowDpi);

    PhpThemeWindowEditThemeChanged(context, EditControl);

    PhSetWindowContext(EditControl, SHRT_MAX, context);
    PhSetWindowProcedure(EditControl, PhEditBorderWndSubclassProc);
    PhSetWindowExStyle(EditControl, WS_EX_CLIENTEDGE, WS_EX_CLIENTEDGE);
    SetWindowPos(EditControl, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

    //PhpThemeWindowEditRedrawFrame(EditControl);
    //InvalidateRect(EditControl, NULL, FALSE);
}

VOID PhpApplyThemeWindow(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN Reinitialize
    )
{
    WCHAR windowClassName[MAX_PATH];

    if (!NT_SUCCESS(PhGetClassName(WindowHandle, windowClassName, RTL_NUMBER_OF(windowClassName), NULL)))
        windowClassName[0] = UNICODE_NULL;

    if (PhEqualStringZ(windowClassName, L"#32770", TRUE))
    {
        if (!PhGetWindowContext(WindowHandle, LONG_MAX) || PhGetWindowProcedure(WindowHandle) != PhpThemeWindowSubclassProc)
            PhInitializeWindowTheme(WindowHandle, TRUE);
    }
    else if (PhEqualStringZ(windowClassName, WC_BUTTON, FALSE))
    {
        ULONG style = PhGetWindowStyle(WindowHandle);

        if ((style & BS_TYPEMASK) == BS_GROUPBOX)
        {
            if (!PhGetWindowContext(WindowHandle, LONG_MAX))
                PhInitializeThemeWindowGroupBox(WindowHandle);
            //else
            //    PhSetWindowStyle(WindowHandle, WS_CLIPSIBLINGS, WS_CLIPSIBLINGS);
        }
        else    // apply theme for CheckBox, Radio (Dart Vanya)
        {
            PhWindowThemeSetDarkMode(WindowHandle, TRUE);
        }
    }
    else if (PhEqualStringZ(windowClassName, WC_TABCONTROL, FALSE))
    {
        if (!PhGetWindowContext(WindowHandle, LONG_MAX))
            PhInitializeThemeWindowTabControl(WindowHandle);
        else
            SetWindowFont(WindowHandle, PhApplicationFont, FALSE);
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
    else if (PhEqualStringZ(windowClassName, L"PhTabNew", FALSE))
    {
        // The control rebuilds its cached theme brushes from the current palette
        // on WM_THEMECHANGED; without this a runtime palette switch leaves the
        // tab strip painted with the previous theme's colors.
        SendMessage(WindowHandle, WM_THEMECHANGED, 0, 0);
    }
    else if (PhEqualStringZ(windowClassName, L"PhHeaderNew", FALSE))
    {
        // Same reason as PhTabNew: the header caches its theme handle and
        // resolves its colors from PhEnableThemeSupport at WM_THEMECHANGED.
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            PhAllowDarkModeForWindow(WindowHandle, TRUE);
        }

        SendMessage(WindowHandle, WM_THEMECHANGED, 0, 0);
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

        PhSetWindowFrameChanged(WindowHandle);

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

        PhSetWindowFrameChanged(WindowHandle);

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

        PhSetWindowFrameChanged(WindowHandle);

        TreeNew_ThemeSupport(WindowHandle, TRUE);

        // Trees without TN_STYLE_CUSTOM_HEADERDRAW let the native header paint itself
        // (TnHeaderCustomPaint returns CDRF_DODEFAULT), so the header controls need the
        // dark item-view theme of their own. Without it comctl32 draws the light class
        // over a dark background: a near-black header with black text. The controls are
        // created during treenew WM_CREATE, before this window is dark-mode capable, so
        // anything they painted in the meantime has to be invalidated as well.
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            HWND headerWindow;

            if (headerWindow = TreeNew_GetFixedHeader(WindowHandle))
            {
                PhAllowDarkModeForWindow(headerWindow, TRUE);
                PhSetControlTheme(headerWindow, L"DarkMode_ItemsView");
                InvalidateRect(headerWindow, NULL, FALSE);
            }

            if (headerWindow = TreeNew_GetHeader(WindowHandle))
            {
                PhAllowDarkModeForWindow(headerWindow, TRUE);
                PhSetControlTheme(headerWindow, L"DarkMode_ItemsView");
                InvalidateRect(headerWindow, NULL, FALSE);
            }
        }

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

        if (!PhGetWindowContext(WindowHandle, LONG_MAX))
            PhInitializeWindowThemeListboxControl(WindowHandle);
        else
            PhSetWindowFrameChanged(WindowHandle);
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
            if (!PhGetWindowContext(WindowHandle, LONG_MAX))
                PhInitializeWindowThemeComboboxControl(WindowHandle);
            //else
            //    InvalidateRect(WindowHandle, NULL, FALSE);
        }
    }
    else if (PhEqualStringZ(windowClassName, L"CHECKLIST_ACLUI", FALSE))
    {
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            PhWindowThemeSetDarkMode(WindowHandle, TRUE);
        }

        if (!PhGetWindowContext(WindowHandle, LONG_MAX))
            PhInitializeWindowThemeACLUI(WindowHandle);
    }
    else if (PhEqualStringZ(windowClassName, WC_EDIT, FALSE))
    {
        // Multiline edits keep the native (dark) theme border. Single-line bordered edits get a
        // custom border from PhInitializeWindowThemeEditControl, so don't apply DarkMode_Explorer
        // to them or it draws a competing native edge underneath the custom one. (dmex)
        if (PhGetWindowStyle(WindowHandle) & ES_MULTILINE)
        {
            PhWindowThemeSetDarkMode(
                WindowHandle,
                PhGetColorBrightness(PhThemeWindowBackgroundColor) < 128
                );
        }

        PhInitializeWindowThemeEditControl(WindowHandle);

        SendMessage(WindowHandle, WM_THEMECHANGED, 0, 0); // searchbox.c
    }
    else if (PhEqualStringZ(windowClassName, WC_LINK, FALSE))
    {
        // SysLink theme support (Dart Vanya)
        PhAllowDarkModeForWindow(WindowHandle, TRUE);
    }

    //RedrawWindow(WindowHandle, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

_Function_class_(PH_WINDOW_ENUM_CALLBACK)
BOOLEAN CALLBACK PhpThemeWindowEnumChildWindows(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    PPHP_THEME_WINDOW_ENUM_CONTEXT enumContext = Context;

    PhEnumChildWindows(
        WindowHandle,
        PhpThemeWindowEnumChildWindows,
        Context
        );

    PhpApplyThemeWindow(WindowHandle, enumContext ? enumContext->Reinitialize : FALSE);

    return TRUE;
}

_Function_class_(PH_WINDOW_ENUM_CALLBACK)
BOOLEAN CALLBACK PhpReInitializeThemeWindowEnumChildWindows(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    PPHP_THEME_WINDOW_ENUM_CONTEXT enumContext = Context;

    PhEnumChildWindows(
        WindowHandle,
        PhpReInitializeThemeWindowEnumChildWindows,
        Context
        );

    PhpApplyThemeWindow(WindowHandle, enumContext ? enumContext->Reinitialize : TRUE);

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
            FillRect(DrawInfo->hDC, &DrawInfo->rcItem, PhThemeWindowBackgroundBrush);

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
    switch (DrawInfo->dwDrawStage)
    {
    case CDDS_PREPAINT:
        {
            // Note: The background is erased by the WM_PAINT in PhRebarWindowHookProcedure
            SetTextColor(DrawInfo->hdc, PhThemeWindowTextColor); 
            FillRect(DrawInfo->hdc, &DrawInfo->rc, PhThemeWindowBackgroundBrush);
        }
        return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT:
        return CDRF_DODEFAULT;
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
            LONG edgeWidth;

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

            BOOLEAN isPressed = !!(buttonInfo.fsState & TBSTATE_PRESSED);
            BOOLEAN isChecked = !!(buttonInfo.fsState & TBSTATE_CHECKED);

            SetTextColor(
                DrawInfo->nmcd.hdc,
                isEnabled ? PhThemeWindowTextColor : PHP_THEME_WINDOW_DISABLED_TEXT_COLOR
                );

            if (isPressed)
            {
                // click: mouse currently held down
                PhpThemeFillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, PHP_THEME_WINDOW_PRESSED_COLOR);
            }
            else if (isChecked)
            {
                // active / latched: reuse pressed color; brighten when also hovered
                PhpThemeFillRect(
                    DrawInfo->nmcd.hdc,
                    &DrawInfo->nmcd.rc,
                    isHighlighted ? PhThemeWindowHighlight2Color : PHP_THEME_WINDOW_PRESSED_COLOR
                    );
            }
            else if (isHighlighted)
            {
                // hover
                SetDCBrushColor(DrawInfo->nmcd.hdc, PhThemeWindowHighlightColor);
                FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, PhpStockDCBrush);
            }
            else
            {
                // normal
                FillRect(DrawInfo->nmcd.hdc, &DrawInfo->nmcd.rc, PhThemeWindowBackgroundBrush);
            }

            dpiValue = PhGetWindowDpi(DrawInfo->nmcd.hdr.hwndFrom);
            edgeWidth = PhGetSystemMetrics(SM_CXEDGE, dpiValue);

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
                        x = DrawInfo->nmcd.rc.left + edgeWidth;
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
                        RECT glyphRect = DrawInfo->nmcd.rc;
                        int triangleLeft = glyphRect.right - 11;
                        int triangleTop = (glyphRect.bottom - glyphRect.top) / 2 - 2;
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

                textRect.left += edgeWidth + bitmapWidth - (isDropDown * 12); // PhScaleToDisplay(10, dpiValue);
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

VOID PhpThemeWindowDrawListViewGroupButton(
    _In_ HDC Hdc,
    _In_ PRECT HeaderRect,
    _In_ LONG DpiValue,
    _In_ BOOLEAN Collapsed
    )
{
    INT savedDc;
    RECT buttonRect;
    POINT arrow[3];
    LONG buttonSize;
    LONG buttonMargin;
    LONG arrowWidth;
    LONG arrowHeight;
    LONG centerX;
    LONG centerY;

    buttonSize = PhScaleToDisplay(14, DpiValue);
    buttonMargin = PhScaleToDisplay(6, DpiValue);
    arrowWidth = PhScaleToDisplay(6, DpiValue);
    arrowHeight = PhScaleToDisplay(7, DpiValue);

    if (buttonSize < 10)
        buttonSize = 10;
    if (arrowWidth < 4)
        arrowWidth = 4;
    if (arrowHeight < 5)
        arrowHeight = 5;

    if (HeaderRect->right - HeaderRect->left < buttonSize + buttonMargin)
        return;

    buttonRect.right = HeaderRect->right - buttonMargin;
    buttonRect.left = buttonRect.right - buttonSize;
    buttonRect.top = HeaderRect->top + ((HeaderRect->bottom - HeaderRect->top) - buttonSize) / 2;
    buttonRect.bottom = buttonRect.top + buttonSize;

    savedDc = SaveDC(Hdc);
    if (!savedDc)
        return;

    SelectPen(Hdc, PhGetStockPen(DC_PEN));
    SelectBrush(Hdc, PhpStockDCBrush);

    SetDCPenColor(Hdc, PhThemeWindowTextColor);
    SetDCBrushColor(Hdc, PhThemeWindowBackground2Color);
    Ellipse(Hdc, buttonRect.left, buttonRect.top, buttonRect.right, buttonRect.bottom);

    centerX = buttonRect.left + (buttonRect.right - buttonRect.left) / 2;
    centerY = buttonRect.top + (buttonRect.bottom - buttonRect.top) / 2;

    if (Collapsed)
    {
        arrow[0].x = centerX + arrowWidth / 2;
        arrow[0].y = centerY;
        arrow[1].x = centerX - arrowWidth / 2;
        arrow[1].y = centerY - arrowHeight / 2;
        arrow[2].x = centerX - arrowWidth / 2;
        arrow[2].y = centerY + arrowHeight / 2;
    }
    else
    {
        arrow[0].x = centerX;
        arrow[0].y = centerY + arrowHeight / 2;
        arrow[1].x = centerX - arrowWidth / 2;
        arrow[1].y = centerY - arrowHeight / 2;
        arrow[2].x = centerX + arrowWidth / 2;
        arrow[2].y = centerY - arrowHeight / 2;
    }

    SetDCPenColor(Hdc, PhThemeWindowTextColor);
    SetDCBrushColor(Hdc, PhThemeWindowTextColor);
    Polygon(Hdc, arrow, RTL_NUMBER_OF(arrow));

    RestoreDC(Hdc, savedDc);
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
            groupInfo.mask = LVGF_HEADER | LVGF_STATE;
            groupInfo.stateMask = LVGS_COLLAPSIBLE | LVGS_COLLAPSED;

            if (ListView_GetGroupInfo(DrawInfo->nmcd.hdr.hwndFrom, (ULONG)DrawInfo->nmcd.dwItemSpec, &groupInfo) != INT_ERROR)
            {
                RECT textRect;
                BOOLEAN collapsible;

                SetTextColor(DrawInfo->nmcd.hdc, PhThemeWindowTextColor);
                SetDCBrushColor(DrawInfo->nmcd.hdc, PhThemeWindowBackground2Color);

                DrawInfo->rcText.top += PhScaleToDisplay(2, dpiValue);
                DrawInfo->rcText.bottom -= PhScaleToDisplay(2, dpiValue);
                FillRect(DrawInfo->nmcd.hdc, &DrawInfo->rcText, PhpStockDCBrush);
                DrawInfo->rcText.top -= PhScaleToDisplay(2, dpiValue);
                DrawInfo->rcText.bottom += PhScaleToDisplay(2, dpiValue);

                textRect = DrawInfo->rcText;
                collapsible = !!(groupInfo.state & LVGS_COLLAPSIBLE);

                if (collapsible)
                {
                    textRect.right -= PhScaleToDisplay(26, dpiValue);

                    if (textRect.right < textRect.left)
                        textRect.right = textRect.left;
                }

                if (groupInfo.pszHeader)
                {
                    textRect.left += PhScaleToDisplay(10, dpiValue);
                    DrawText(
                        DrawInfo->nmcd.hdc,
                        groupInfo.pszHeader,
                        (UINT)PhCountStringZ(groupInfo.pszHeader),
                        &textRect,
                        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX
                        );
                }

                if (collapsible)
                {
                    PhpThemeWindowDrawListViewGroupButton(
                        DrawInfo->nmcd.hdc,
                        &DrawInfo->rcText,
                        dpiValue,
                        !!(groupInfo.state & LVGS_COLLAPSED)
                        );
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

_Function_class_(PH_WINDOW_ENUM_CALLBACK)
static BOOLEAN CALLBACK PhpThemeWindowForwardSysColorChangeCallback(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    // WM_SYSCOLORCHANGE is only delivered to top-level windows; forward it to
    // descendant common controls (e.g. toolbars using the "3D Objects" color) so
    // they refresh their cached system colors instead of painting with stale ones.
    SendMessage(WindowHandle, WM_SYSCOLORCHANGE, 0, 0);

    return TRUE;
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
            PhpThemeRestoreSubclassWindowProcedure(hWnd, oldWndProc, LONG_MAX);
        }
        break;
    case WM_ERASEBKGND:
        {
            HDC hdc = (HDC)wParam;
            RECT clientRect;

            // Fill only the region that actually needs erasing. Filling the entire
            // client rect repaints the area behind every child control, which shows
            // up as a full-window flash on each step of a resize drag. (dmex)
            if (GetClipBox(hdc, &clientRect) <= NULLREGION)
                return TRUE;

            SetBkMode(hdc, TRANSPARENT);
            FillRect(hdc, &clientRect, PhThemeWindowBackgroundBrush);

            return TRUE;
        }
        break;
    case WM_SYSCOLORCHANGE:
        {
            LRESULT result = CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);

            PhEnumChildWindows(hWnd, PhpThemeWindowForwardSysColorChangeCallback, NULL);

            return result;
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR data = (LPNMHDR)lParam;

            switch (data->code)
            {
            case NM_CUSTOMDRAW:
                return PhpThemeWindowHandleCustomDraw((LPNMCUSTOMDRAW)lParam);
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
            return (INT_PTR)PhpStockDCBrush;
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
    FillRect(BufferDc, ClientRect, PhThemeWindowBackgroundBrush);

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
        oldBrush = SelectBrush(BufferDc, PhpStockNullBrush);

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
    PPHP_THEME_WINDOW_GROUPBOX_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(WindowHandle, LONG_MAX)))
        return FALSE;

    oldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            PhpThemeRestoreSubclassWindowProcedure(WindowHandle, oldWndProc, LONG_MAX);
            PhFree(context);
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
        PhpThemePaintBufferedWindow(
            WindowHandle,
            FlagOn(PhGetWindowStyle(WindowHandle), WS_CLIPSIBLINGS) ? PhpThemePaintGroupBoxCallback : NULL,
            NULL
            );
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
    PPHP_THEME_WINDOW_GROUPBOX_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(WindowHandle, LONG_MAX)))
        return FALSE;

    oldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            PhpThemeRestoreSubclassWindowProcedure(WindowHandle, oldWndProc, LONG_MAX);
            PhFree(context);
        }
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            RECT clientRect;
            HDC memoryDc;
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

            PH_BUFFERED_PAINT paintBuffer;
            PH_PAINTPARAMS paintParams;

            memset(&paintParams, 0, sizeof(PH_PAINTPARAMS));
            paintParams.Size = sizeof(PH_PAINTPARAMS);
            paintParams.Flags = BPPF_ERASE;

            if (PhBeginBufferedPaint(hdc, &clientRect, PHBF_COMPATIBLEBITMAP, &paintParams, &paintBuffer, &memoryDc))
            {

            }

            SetBkMode(memoryDc, TRANSPARENT);
            FillRect(memoryDc, &clientRect, PhThemeWindowBackgroundBrush);

            // Setup font for text measurement.
            font = GetWindowFont(WindowHandle);
            oldFont = SelectFont(memoryDc, font);

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

            PhEndBufferedPaint(&paintBuffer, TRUE);

            EndPaint(WindowHandle, &ps);
        }
        return 0;
    }

    return CallWindowProc(oldWndProc, WindowHandle, uMsg, wParam, lParam);
}

VOID ThemeWindowRenderProgressBarControl(
    _In_ HWND WindowHandle,
    _In_ HDC BufferDc,
    _In_ PRECT ClientRect
    )
{
    LONG_PTR style;
    INT position;
    INT state;
    RECT fillRect;
    COLORREF fillColor;
    LONG borderSize;

    style = PhGetWindowStyle(WindowHandle);

    SetBkMode(BufferDc, TRANSPARENT);
    SetDCBrushColor(BufferDc, PhThemeWindowBackground2Color);
    FillRect(BufferDc, ClientRect, PhpStockDCBrush);

    borderSize = 1;
    fillRect = *ClientRect;
    InflateRect(&fillRect, -borderSize, -borderSize);

    state = (INT)SendMessage(WindowHandle, PBM_GETSTATE, 0, 0);

    switch (state)
    {
    case PBST_ERROR:
        fillColor = RGB(196, 43, 28);
        break;
    case PBST_PAUSED:
        fillColor = RGB(230, 180, 40);
        break;
    default:
        fillColor = PhThemeWindowHighlightColor;
        break;
    }

    if (style & PBS_MARQUEE)
    {
        // Marquee mode animates via the control's own timer/PBM_SETMARQUEE;
        // draw a static highlighted bar since we own WM_PAINT entirely.
        SetDCBrushColor(BufferDc, fillColor);
        FillRect(BufferDc, &fillRect, PhpStockDCBrush);
    }
    else
    {
        PBRANGE range;

        SendMessage(WindowHandle, PBM_GETRANGE, TRUE, (LPARAM)&range);
        position = (INT)SendMessage(WindowHandle, PBM_GETPOS, 0, 0);

        if (range.iHigh > range.iLow)
        {
            LONG fillWidth = MulDiv(
                fillRect.right - fillRect.left,
                position - range.iLow,
                range.iHigh - range.iLow
                );
            RECT barRect = fillRect;

            barRect.right = barRect.left + fillWidth;

            if (barRect.right > barRect.left)
            {
                SetDCBrushColor(BufferDc, fillColor);
                FillRect(BufferDc, &barRect, PhpStockDCBrush);
            }
        }
    }

    SetDCBrushColor(BufferDc, PhThemeWindowBorderColor);
    FrameRect(BufferDc, ClientRect, PhpStockDCBrush);
}

LRESULT CALLBACK PhpThemeWindowProgressBarSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_THEME_WINDOW_PROGRESS_CONTEXT context;

    if (!(context = PhGetWindowContext(WindowHandle, LONG_MAX)))
        return FALSE;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            PhpThemeRestoreSubclassWindowProcedure(WindowHandle, context->DefaultWindowProc, LONG_MAX);
            PhFree(context);
        }
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        PhpThemePaintBufferedWindow(WindowHandle, PhpThemePaintProgressBarCallback, NULL);
        return 0;
    case PBM_SETPOS:
    case PBM_DELTAPOS:
    case PBM_SETRANGE:
    case PBM_SETRANGE32:
    case PBM_SETSTATE:
    case PBM_SETMARQUEE:
        {
            LRESULT result = CallWindowProc(context->DefaultWindowProc, WindowHandle, uMsg, wParam, lParam);
            InvalidateRect(WindowHandle, NULL, FALSE);
            return result;
        }
    }

    return CallWindowProc(context->DefaultWindowProc, WindowHandle, uMsg, wParam, lParam);
}

VOID PhInitializeThemeWindowProgressBar(
    _In_ HWND ProgressBarHandle
    )
{
    PPHP_THEME_WINDOW_PROGRESS_CONTEXT context;

    context = PhAllocateZero(sizeof(PHP_THEME_WINDOW_PROGRESS_CONTEXT));
    context->DefaultWindowProc = PhGetWindowProcedure(ProgressBarHandle);
    context->WindowDpi = PhGetWindowDpi(ProgressBarHandle);

    PhSetWindowContext(ProgressBarHandle, LONG_MAX, context);
    PhSetWindowProcedure(ProgressBarHandle, PhpThemeWindowProgressBarSubclassProc);

    //InvalidateRect(ProgressBarHandle, NULL, FALSE);
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
    RECT clipRect;

    // The paint DC only covers the invalid region (a rcPaint-sized buffer or the
    // BeginPaint DC), so fill just that and skip tabs outside of it. (dmex)
    if (GetClipBox(bufferDc, &clipRect) <= NULLREGION)
        return;

    SetBkMode(bufferDc, TRANSPARENT);
    SetTextColor(bufferDc, PhThemeWindowTextColor);
    FillRect(bufferDc, &clipRect, PhThemeWindowBackgroundBrush);

    oldFont = SelectFont(bufferDc, GetWindowFont(WindowHandle));
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
            }

            if (!RectVisible(bufferDc, &itemRect))
                continue;

            PhpThemeFillRect(
                bufferDc,
                &itemRect,
                hot ? PhThemeWindowBackground2Color : PhThemeWindowBackgroundColor
                );

            if (selected)
            {
                PhpThemeFrameRect(bufferDc, &itemRect, PhThemeWindowBackground2Color);
            }

            if (selected)
            {
                RECT selectedGap = itemRect;

                selectedGap.top = itemRect.bottom - cyEdge;
                PhpThemeFillRect(
                    bufferDc,
                    &selectedGap,
                    hot ? PhThemeWindowBackground2Color : PhThemeWindowBackgroundColor
                    );
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
    INT currentSelection = TabCtrl_GetCurSel(WindowHandle);
    INT count = TabCtrl_GetItemCount(WindowHandle);
    RECT itemRect = { 0 };
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

    HBRUSH dcBrush = PhpStockDCBrush;

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
            FillRect(bufferDc, &itemRect, dcBrush);

        }
        else
        {
            {
                SetDCBrushColor(bufferDc, PhThemeWindowBackgroundColor);
                FillRect(bufferDc, &itemRect, dcBrush);
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
        FillRect(bufferDc, &itemRect, dcBrush);

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
            PhpThemeRestoreSubclassWindowProcedure(WindowHandle, oldWndProc, LONG_MAX);

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
    case WM_PAINT:
        {
            PHP_THEME_TAB_PAINT_CONTEXT paintContext =
            {
                context,
                oldWndProc
            };

            PhpThemePaintBufferedWindow(WindowHandle, PhpThemePaintTabControlCallback, &paintContext);
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
            PhpThemeRestoreSubclassWindowProcedure(WindowHandle, oldWndProc, LONG_MAX);
            PhDeleteScratchRegion(&context->NcPaintRegion);

            PhFree(context);
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
            // WM_NCCALCSIZE above returns 1px of the default edge to the client
            // area, so the remaining non-client band is one pixel narrower than
            // SM_CXEDGE/SM_CYEDGE. Use the same band width everywhere below.
            INT cxEdge = PhGetSystemMetrics(SM_CXEDGE, dpiValue);
            INT cyEdge = PhGetSystemMetrics(SM_CYEDGE, dpiValue);

            updateRegion = (HRGN)wParam;

            if (!PhGetWindowRect(WindowHandle, &windowRect))
                break;

            // draw the scrollbar without the border. DefWindowProc doesn't take
            // ownership of the region, so reuse a cached one instead of allocating
            // a new region on every non-client paint. (dmex)
            {
                HRGN rectregion = PhGetScratchRegion(&context->NcPaintRegion);

                if (rectregion)
                {
                    SetRectRgn(
                        rectregion,
                        windowRect.left + cxEdge,
                        windowRect.top + cyEdge,
                        windowRect.right - cxEdge,
                        windowRect.bottom - cyEdge
                        );

                    if (updateRegion != HRGN_FULL)
                        CombineRgn(rectregion, rectregion, updateRegion, RGN_AND);
                    DefWindowProc(WindowHandle, WM_NCPAINT, (WPARAM)rectregion, 0);
                }
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

                if (context->Flags & PHP_THEME_STATUSBAR_FLAG_HOT)
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
            PhpThemeRestoreSubclassWindowProcedure(WindowHandle, oldWndProc, LONG_MAX);

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
            PHP_THEME_COMBO_PAINT_CONTEXT paintContext =
            {
                context,
                oldWndProc
            };

            PhpThemePaintBufferedWindow(WindowHandle, PhpThemePaintComboBoxCallback, &paintContext);
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
                    if (
                        NT_SUCCESS(PhGetClassName(customDraw->hdr.hwndFrom, className, RTL_NUMBER_OF(className), NULL)) &&
                        PhEqualStringZ(className, WC_BUTTON, FALSE)
                        )
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
            PhpThemeRestoreSubclassWindowProcedure(WindowHandle, oldWndProc, LONG_MAX);
        }
        break;
    case WM_ERASEBKGND:
        {
            HDC hdc = (HDC)wParam;
            RECT clientRect;

            if (GetClipBox(hdc, &clientRect) <= NULLREGION)
                return TRUE;

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

VOID PhTheme_PaintControlBorder(
    _In_ HDC Hdc,
    _In_ PRECT Rect,
    _In_ BOOLEAN Focused,
    _In_ BOOLEAN Hot
    )
{
    COLORREF outerColor;
    //COLORREF innerColor;

    if (Focused)
    {
        outerColor = PhThemeWindowFocusBorderColor;
        //innerColor = PhThemeWindowEditHotBorderColor;
    }
    else if (Hot)
    {
        outerColor = PhThemeWindowEditHotBorderColor;
        //innerColor = PhThemeWindowEditNormalBorderColor;
    }
    else
    {
        outerColor = PhThemeWindowScrollbarColor;// PhThemeWindowWindowFrameColor;
        //innerColor = PhThemeWindowEditNormalBorderColor;
    }

    // Fill the entire non-client band first so no ring of stale pixels survives
    // between the frame lines. The caller excludes the client area from the clip,
    // so this cannot bleed into client pixels.
    //FillRect(Hdc, Rect, PhThemeWindowBackgroundBrush);

    // Draw outer border (1px)
    SetDCBrushColor(Hdc, outerColor);
    FrameRect(Hdc, Rect, PhpStockDCBrush);

    // Draw inner border (1px) if space remains. The inner line sits directly
    // inside the outer line so both fit within the reserved WS_EX_CLIENTEDGE band.
    RECT rcInner = *Rect;
    PhInflateRect(&rcInner, -1, -1);
    //if (rcInner.right > rcInner.left && rcInner.bottom > rcInner.top)
    //{
        FrameRect(Hdc, &rcInner, PhThemeWindowBackgroundBrush);
    //}
}

VOID PhpThemeWindowEditThemeChanged(
    _In_ PPHP_THEME_WINDOW_EDIT_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    ULONG style;

    style = PhGetWindowStyle(WindowHandle);

    Context->ReadOnly = !!(style & ES_READONLY);
    Context->Multiline = !!(style & ES_MULTILINE);
    Context->WindowFocus = GetFocus() == WindowHandle;

    Context->WindowBrush = PhpStockDCBrush;
    Context->FrameBrush = PhpStockDCBrush;
    // Match the non-client band actually reserved by WS_EX_CLIENTEDGE so the
    // painted frame and the client exclusion stay in sync across DPI changes.
    Context->BorderSize = PhGetSystemMetrics(SM_CXEDGE, Context->WindowDpi);
}

VOID PhpThemeWindowEditRedrawFrame(
    _In_ HWND WindowHandle
    )
{
    RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
}

VOID PhpThemeWindowEditUpdateFrameStyle(
    _In_ PPHP_THEME_WINDOW_EDIT_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    PhSetWindowExStyle(WindowHandle, WS_EX_CLIENTEDGE, WS_EX_CLIENTEDGE);
}

VOID PhpThemeWindowEditExcludeClient(
    _In_ PPHP_THEME_WINDOW_EDIT_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HDC Hdc,
    _In_ PRECT WindowRect,
    _In_ PRECT ScreenWindowRect
    )
{
    ExcludeClipRect(
        Hdc,
        WindowRect->left + Context->BorderSize,
        WindowRect->top + Context->BorderSize,
        WindowRect->right - Context->BorderSize,
        WindowRect->bottom - Context->BorderSize
        );

    if (Context->Multiline)
    {
        ULONG style;

        style = PhGetWindowStyle(WindowHandle);

        if (style & WS_VSCROLL)
        {
            SCROLLBARINFO scrollInfo = { sizeof(SCROLLBARINFO) };

            if (GetScrollBarInfo(WindowHandle, OBJID_VSCROLL, &scrollInfo) &&
                (scrollInfo.rgstate[0] & STATE_SYSTEM_INVISIBLE) == 0)
            {
                RECT scrollBarRect = scrollInfo.rcScrollBar;

                PhOffsetRect(&scrollBarRect, -ScreenWindowRect->left, -ScreenWindowRect->top);
                ExcludeClipRect(Hdc, scrollBarRect.left, scrollBarRect.top, scrollBarRect.right, scrollBarRect.bottom);
            }
        }

        if (style & WS_HSCROLL)
        {
            SCROLLBARINFO scrollInfo = { sizeof(SCROLLBARINFO) };

            if (GetScrollBarInfo(WindowHandle, OBJID_HSCROLL, &scrollInfo) &&
                (scrollInfo.rgstate[0] & STATE_SYSTEM_INVISIBLE) == 0)
            {
                RECT scrollBarRect = scrollInfo.rcScrollBar;

                PhOffsetRect(&scrollBarRect, -ScreenWindowRect->left, -ScreenWindowRect->top);
                ExcludeClipRect(Hdc, scrollBarRect.left, scrollBarRect.top, scrollBarRect.right, scrollBarRect.bottom);
            }
        }
    }
}

VOID PhpThemeWindowEditPaintNativeFrame(
    _In_ PPHP_THEME_WINDOW_EDIT_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ WNDPROC DefaultWindowProc,
    _In_ WPARAM wParam
    )
{
    RECT windowRect;
    HRGN updateRegion;
    HRGN nativeRegion;

    if (!PhGetWindowRect(WindowHandle, &windowRect))
        return;

    PhInflateRect(&windowRect, -Context->BorderSize, -Context->BorderSize);

    if (windowRect.right <= windowRect.left || windowRect.bottom <= windowRect.top)
        return;

    // CallWindowProc doesn't take ownership of the region, so reuse a cached one
    // instead of allocating a new region on every non-client paint. (dmex)
    if (nativeRegion = PhGetScratchRegion(&Context->NcPaintRegion))
    {
        SetRectRgn(
            nativeRegion,
            windowRect.left,
            windowRect.top,
            windowRect.right,
            windowRect.bottom
            );

        updateRegion = (HRGN)wParam;

        if (updateRegion && updateRegion != HRGN_FULL)
        {
            CombineRgn(nativeRegion, nativeRegion, updateRegion, RGN_AND);
        }

        CallWindowProc(DefaultWindowProc, WindowHandle, WM_NCPAINT, (WPARAM)nativeRegion, 0);
    }
}

BOOLEAN PhpThemeWindowEditPaintFrame(
    _In_ PPHP_THEME_WINDOW_EDIT_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ WPARAM wParam
    )
{
    RECT windowRect;
    LONG width;
    LONG height;
    HDC hdc;
    HRGN updateRegion;
    ULONG flags;
    RECT windowRectScreen;

    // Only paint when matching non-client space was reserved (WS_EX_CLIENTEDGE);
    // otherwise the frame lands on client pixels and WM_PAINT overdraws it.
    ///if (!Context->DrawCustomBorder)
    //    return FALSE;

    if (!PhGetWindowRect(WindowHandle, &windowRect))
        return TRUE;

    windowRectScreen = windowRect;
    width = windowRect.right - windowRect.left;
    height = windowRect.bottom - windowRect.top;

    if (PhRectEmpty(&windowRect))
        return TRUE;

    updateRegion = (HRGN)wParam;

    if (updateRegion != HRGN_FULL && updateRegion != NULL)
    {
        if (!RectInRegion(updateRegion, &windowRectScreen))
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

        PhpThemeWindowEditExcludeClient(Context, WindowHandle, hdc, &windowRect, &windowRectScreen);
        PhTheme_PaintControlBorder(hdc, &windowRect, !!Context->WindowFocus, !!Context->Hot);

        ReleaseDC(WindowHandle, hdc);
        return TRUE;
    }

    return FALSE;
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
            PhpThemeRestoreSubclassWindowProcedure(WindowHandle, oldWndProc, SHRT_MAX);

            PhDeleteScratchRegion(&context->NcPaintRegion);

            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            LRESULT result = CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);

            if (wParam)
            {
                PhpThemeWindowEditRedrawFrame(WindowHandle);
            }

            return result;
        }
    case WM_ERASEBKGND:
        return TRUE;
    case WM_STYLECHANGED:
        {
            LRESULT result = CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);

            if (wParam == GWL_STYLE)
            {
                PhpThemeWindowEditThemeChanged(context, WindowHandle);
                PhpThemeWindowEditUpdateFrameStyle(context, WindowHandle);

                SetWindowPos(WindowHandle, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

                RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            }

            return result;
        }
    case WM_NCPAINT:
        {
            //if (!context->DrawCustomBorder)
            //    return CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);

            PhpThemeWindowEditPaintNativeFrame(context, WindowHandle, oldWndProc, wParam);

            if (!PhpThemeWindowEditPaintFrame(context, WindowHandle, wParam))
                return CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);
        }
        return 0;
    case WM_MOUSEMOVE:
    case WM_NCMOUSEMOVE:
        {
            LRESULT result;
            POINT windowPoint;
            RECT windowRect;

            if (!context->HotTrack)
            {
                TRACKMOUSEEVENT trackMouseEvent;

                trackMouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
                trackMouseEvent.dwFlags = TME_LEAVE | TME_NONCLIENT;
                trackMouseEvent.hwndTrack = WindowHandle;
                trackMouseEvent.dwHoverTime = 0;

                context->HotTrack = TRUE;

                TrackMouseEvent(&trackMouseEvent);
            }

            if (PhGetMessagePos(&windowPoint) && PhGetWindowRect(WindowHandle, &windowRect))
            {
                BOOLEAN hot;
                BOOLEAN hotChanged;

                hot = PhPtInRect(&windowRect, &windowPoint);
                hotChanged = context->Hot != hot;
                context->Hot = hot;

                result = CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);

                if (hotChanged)
                    PhpThemeWindowEditRedrawFrame(WindowHandle);

                return result;
            }

            result = CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);

            return result;
        }
    case WM_MOUSELEAVE:
    case WM_NCMOUSELEAVE:
        {
            LRESULT result;
            POINT windowPoint;
            RECT windowRect;
            BOOLEAN hot = FALSE;
            BOOLEAN hotChanged;

            context->HotTrack = FALSE;

            if (PhGetMessagePos(&windowPoint) && PhGetWindowRect(WindowHandle, &windowRect))
                hot = PhPtInRect(&windowRect, &windowPoint);

            hotChanged = context->Hot != hot;
            context->Hot = hot;

            result = CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);

            if (hotChanged)
                RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);

            return result;
        }
    case WM_SETFOCUS:
        {
            LRESULT result;

            context->WindowFocus = TRUE;
            context->PreviousFocusWindowHandle = (HWND)wParam;

            result = CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);

            RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);

            return result;
        }
    case WM_KILLFOCUS:
        {
            LRESULT result;

            context->WindowFocus = FALSE;

            result = CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);

            RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);

            return result;
        }
    case WM_SETTINGCHANGE:
    case WM_SYSCOLORCHANGE:
    case WM_THEMECHANGED:
        {
            LRESULT result = CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);

            PhpThemeWindowEditThemeChanged(context, WindowHandle);
            PhpThemeWindowEditUpdateFrameStyle(context, WindowHandle);
            SetWindowPos(WindowHandle, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

            RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);

            return result;
        }
    case WM_DPICHANGED_AFTERPARENT:
        {
            LRESULT result = CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);

            context->WindowDpi = PhGetWindowDpi(context->ParentWindowHandle);

            PhpThemeWindowEditThemeChanged(context, WindowHandle);
            PhpThemeWindowEditUpdateFrameStyle(context, WindowHandle);
            SetWindowPos(WindowHandle, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

            RedrawWindow(WindowHandle, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);

            return result;
        }
    }

    return CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);
}
