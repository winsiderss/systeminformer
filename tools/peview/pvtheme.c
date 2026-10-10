/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 */

#include <peview.h>

// The color tables for the properties window chrome. These are PE Viewer's own
// colors: the dark table is the reference appearance of the properties window
// and the light table is its counterpart for light mode.

static CONST PV_THEME_COLORS PvpThemeDarkColors =
{
    RGB(0x12, 0x12, 0x12), // WindowBackground
    RGB(0xe8, 0xed, 0xf3), // WindowText
    // The navigation pane shares the window surface: the separation comes from the
    // metadata cards on the right, not from a darker pane. (dmex)
    RGB(0x12, 0x12, 0x12), // SidebarBackground
    RGB(0xe8, 0xed, 0xf3), // SidebarText
    RGB(0x27, 0x36, 0x4a), // SidebarSelected
    RGB(0x12, 0x16, 0x1d), // SplitterColor
    RGB(0x2e, 0x36, 0x42), // BorderColor
    RGB(0x3d, 0x8b, 0xfd), // AccentColor
    RGB(0x1f, 0x6f, 0xeb), // AccentFill
    RGB(0x2f, 0x7f, 0xf5), // AccentFillHot
    RGB(0x18, 0x56, 0xb8), // AccentFillPressed
    RGB(0xff, 0xff, 0xff), // AccentText
    RGB(0x22, 0x29, 0x33), // SurfaceColor
    RGB(0x12, 0x12, 0x12), // ListBackground
    RGB(0xe3, 0xe8, 0xee), // ListText
    RGB(0x23, 0x2b, 0x37), // ListSelected
    RGB(0x1b, 0x21, 0x2b), // CardBackground
    RGB(0x2a, 0x32, 0x3e), // CardBorder
    RGB(0x9a, 0xa5, 0xb4), // SecondaryText
    RGB(0x1e, 0x2f, 0x4a), // ChipBackground
    RGB(0x7d, 0xb0, 0xff), // ChipText
    RGB(0x1f, 0x27, 0x33)  // SidebarHot
};

static CONST PV_THEME_COLORS PvpThemeLightColors =
{
    // White cards need a tinted window behind them or the layering disappears, which
    // is the light counterpart of the dark table above. (dmex)
    RGB(0xf3, 0xf5, 0xf7), // WindowBackground
    RGB(0x1a, 0x1a, 0x1a), // WindowText
    RGB(0xf3, 0xf5, 0xf7), // SidebarBackground
    RGB(0x1a, 0x1a, 0x1a), // SidebarText
    RGB(0xcf, 0xdf, 0xf2), // SidebarSelected
    RGB(0xd6, 0xd9, 0xdd), // SplitterColor
    RGB(0xc4, 0xc8, 0xcd), // BorderColor
    RGB(0x0f, 0x6c, 0xbd), // AccentColor
    RGB(0x0f, 0x6c, 0xbd), // AccentFill
    RGB(0x2b, 0x84, 0xd4), // AccentFillHot
    RGB(0x0b, 0x53, 0x94), // AccentFillPressed
    RGB(0xff, 0xff, 0xff), // AccentText
    RGB(0xef, 0xf1, 0xf4), // SurfaceColor
    RGB(0xf3, 0xf5, 0xf7), // ListBackground
    RGB(0x1a, 0x1a, 0x1a), // ListText
    RGB(0xdd, 0xe4, 0xed), // ListSelected
    RGB(0xff, 0xff, 0xff), // CardBackground
    RGB(0xe1, 0xe5, 0xea), // CardBorder
    RGB(0x5c, 0x63, 0x6d), // SecondaryText
    RGB(0xe6, 0xf0, 0xfb), // ChipBackground
    RGB(0x0b, 0x53, 0x94), // ChipText
    RGB(0xea, 0xef, 0xf5)  // SidebarHot
};

// Category accents. Index with PV_THEME_ACCENT; entry zero is the interactive
// accent and is kept in step with AccentColor above.
static CONST COLORREF PvpThemeDarkAccents[PvThemeAccentMax] =
{
    RGB(0x3d, 0x8b, 0xfd), // PvThemeAccentPrimary
    RGB(0x3d, 0x8b, 0xfd), // PvThemeAccentInfo
    RGB(0x4c, 0xc3, 0x8a), // PvThemeAccentDebug
    RGB(0x4c, 0xc3, 0x8a), // PvThemeAccentTrust
    RGB(0x5e, 0xc8, 0xd8)  // PvThemeAccentInternal
};

static CONST COLORREF PvpThemeLightAccents[PvThemeAccentMax] =
{
    RGB(0x0f, 0x6c, 0xbd), // PvThemeAccentPrimary
    RGB(0x0f, 0x6c, 0xbd), // PvThemeAccentInfo
    RGB(0x0e, 0x70, 0x4a), // PvThemeAccentDebug
    RGB(0x0e, 0x70, 0x4a), // PvThemeAccentTrust
    RGB(0x0d, 0x63, 0x70)  // PvThemeAccentInternal
};

static PV_THEME_COLORS PvpThemeColors = { 0 };
static CONST COLORREF *PvpThemeAccents = PvpThemeDarkAccents;
static BOOLEAN PvpThemeEnabled = FALSE;
static BOOLEAN PvpThemeDark = FALSE;
static HBRUSH PvpThemeBackgroundBrush = NULL;

// Category glyphs for the group headings, built on first use and rebuilt when the
// DPI they were rendered for changes. Owned by this module and released from
// PvDeleteTheme.
static HICON PvpThemeCategoryIcons[PvThemeAccentMax] = { NULL };
static LONG PvpThemeCategoryIconSize = 0;

// Reads the per-user "AppsUseLightTheme" preference. Queried once from
// PvInitializeTheme so paint code never touches the registry.
static BOOLEAN PvpQueryAppsUseLightTheme(
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

// Everything phlib paints itself - menus, group boxes, scrollbars, the non-default
// buttons - reads the PhThemeWindow* globals rather than our palette. Assigning those
// globals directly is not enough: the first PhInitializeWindowTheme call runs a
// one-shot PhSetWindowThemePalette(PhWindowThemeDark) that overwrites them with
// phlib's own grays, which is what left the File group box filled #2b2b2b against a
// #171c24 window. Registering our colors as the Custom1 palette both applies them and
// marks the palette as already selected, so that one-shot is skipped. (dmex)
static VOID PvpApplyPhlibPalette(
    VOID
    )
{
    PH_WINDOW_THEME_PALETTE palette;

    // Start from phlib's own palette for the resolved mode so anything peview has no
    // opinion on keeps a sane value.
    PhSetWindowThemePalette(PvpThemeDark ? PhWindowThemeDark : PhWindowThemeLight, NULL);

    memset(&palette, 0, sizeof(PH_WINDOW_THEME_PALETTE));
    palette.ForegroundColor = PhThemeWindowForegroundColor;
    palette.BackgroundColor = PhThemeWindowBackgroundColor;
    palette.Background2Color = PhThemeWindowBackground2Color;
    palette.HighlightColor = PhThemeWindowHighlightColor;
    palette.Highlight2Color = PhThemeWindowHighlight2Color;
    palette.TextColor = PhThemeWindowTextColor;
    palette.DisabledTextColor = PhThemeWindowDisabledTextColor;
    palette.BorderColor = PhThemeWindowBorderColor;
    palette.EditColor = PhThemeWindowEditColor;
    palette.ScrollbarColor = PhThemeWindowScrollbarColor;
    palette.FilteredBorderColor = PhThemeWindowFilteredBorderColor;
    palette.ProtectedBorderColor = PhThemeWindowProtectedBorderColor;
    palette.FocusBorderColor = PhThemeWindowFocusBorderColor;
    palette.GroupBoxFrameColor = PhThemeWindowGroupBoxFrameColor;
    palette.WindowFrameColor = PhThemeWindowWindowFrameColor;
    palette.EditHotBorderColor = PhThemeWindowEditHotBorderColor;
    palette.EditNormalBorderColor = PhThemeWindowEditNormalBorderColor;
    palette.MenuSelectedTextColor = PhThemeWindowMenuSelectedTextColor;
    palette.MenuDisabledTextColor = PhThemeWindowMenuDisabledTextColor;

    // These four have no mirror global to read back, so they come straight from our
    // palette.
    palette.PressedColor = PvpThemeColors.SidebarBackground;
    palette.DropdownGlyphColor = PvpThemeColors.WindowText;
    palette.WindowActiveBorderColor = PvpThemeColors.BorderColor;
    palette.WindowInactiveBorderColor = PvpThemeColors.SplitterColor;

    // Now the surfaces peview does have an opinion on. Highlight/Highlight2 are
    // deliberately left as phlib had them: they drive scrollbar thumbs and menu
    // highlights, which need to stay lighter than the background.
    palette.ForegroundColor = PvpThemeColors.SidebarBackground;
    palette.BackgroundColor = PvpThemeColors.WindowBackground;
    palette.Background2Color = PvpThemeColors.CardBackground;
    palette.TextColor = PvpThemeColors.WindowText;
    palette.BorderColor = PvpThemeColors.BorderColor;
    palette.EditColor = PvpThemeColors.WindowBackground;
    palette.ScrollbarColor = PvpThemeColors.SidebarBackground;
    palette.FocusBorderColor = PvpThemeColors.AccentColor;
    palette.GroupBoxFrameColor = PvpThemeColors.BorderColor;
    palette.WindowFrameColor = PvpThemeColors.BorderColor;
    palette.EditHotBorderColor = PvpThemeColors.AccentColor;
    palette.EditNormalBorderColor = PvpThemeColors.BorderColor;

    PhSetWindowThemePalette(PhWindowThemeCustom1, &palette);
}

VOID PvInitializeTheme(
    VOID
    )
{
    ULONG themeMode;

    PvDeleteTheme();

    PvpThemeEnabled = !!PhGetIntegerSetting(L"EnableThemeSupport");
    themeMode = PhGetIntegerSetting(L"PvThemeMode");

    switch (themeMode)
    {
    case PvThemeModeLight:
        PvpThemeDark = FALSE;
        break;
    case PvThemeModeDark:
        PvpThemeDark = TRUE;
        break;
    case PvThemeModeAutomatic:
    default:
        PvpThemeDark = !PvpQueryAppsUseLightTheme();
        break;
    }

    PvpThemeColors = PvpThemeDark ? PvpThemeDarkColors : PvpThemeLightColors;
    PvpThemeAccents = PvpThemeDark ? PvpThemeDarkAccents : PvpThemeLightAccents;

    // phlib does its half of the work - dark mode on the controls, listview and
    // treenew colors, menus, the window frame - only when this global is set, and it
    // is loaded separately from the ini by PvUpdateCachedSettings. If the two ever
    // disagree, peview paints its own surfaces while every phlib-owned control stays
    // unthemed. Make our resolved state authoritative so that cannot happen. (dmex)
    PhEnableThemeSupport = PvpThemeEnabled;

    if (PvpThemeEnabled)
    {
        PvpThemeBackgroundBrush = CreateSolidBrush(PvpThemeColors.WindowBackground);

        PvpApplyPhlibPalette();
    }
}

VOID PvDeleteTheme(
    VOID
    )
{
    PV_THEME_ACCENT accent;

    if (PvpThemeBackgroundBrush)
    {
        DeleteObject(PvpThemeBackgroundBrush);
        PvpThemeBackgroundBrush = NULL;
    }

    // The glyphs carry the accent color of the palette they were rendered for, so
    // they cannot survive a mode change. (dmex)
    for (accent = PvThemeAccentPrimary; accent < PvThemeAccentMax; accent++)
    {
        if (PvpThemeCategoryIcons[accent])
        {
            DestroyIcon(PvpThemeCategoryIcons[accent]);
            PvpThemeCategoryIcons[accent] = NULL;
        }
    }

    PvpThemeCategoryIconSize = 0;
}

COLORREF PvGetThemeAccentColor(
    _In_ PV_THEME_ACCENT Accent
    )
{
    if (Accent >= PvThemeAccentMax)
        Accent = PvThemeAccentPrimary;

    return PvpThemeAccents[Accent];
}

// Returns the cached glyph for a category, rendering the whole set on the first
// call and whenever the requested size changes. The icons are owned here; callers
// must not destroy them.
_Ret_maybenull_
static HICON PvpGetThemeCategoryIcon(
    _In_ PV_THEME_ACCENT Accent,
    _In_ LONG Size,
    _In_ LONG DpiValue
    )
{
    if (Accent >= PvThemeAccentMax || Size <= 0)
        return NULL;

    if (PvpThemeCategoryIconSize != Size)
    {
        PV_THEME_ACCENT index;

        for (index = PvThemeAccentPrimary; index < PvThemeAccentMax; index++)
        {
            if (PvpThemeCategoryIcons[index])
            {
                DestroyIcon(PvpThemeCategoryIcons[index]);
                PvpThemeCategoryIcons[index] = NULL;
            }
        }

        PvpThemeCategoryIconSize = Size;
    }

    if (!PvpThemeCategoryIcons[Accent])
    {
        PvpThemeCategoryIcons[Accent] = PvGetCategoryIcon(Accent, Size, Size, DpiValue);
    }

    return PvpThemeCategoryIcons[Accent];
}

BOOLEAN PvThemeEnabled(
    VOID
    )
{
    return PvpThemeEnabled;
}

BOOLEAN PvThemeDarkEnabled(
    VOID
    )
{
    return PvpThemeEnabled && PvpThemeDark;
}

_Ret_notnull_
PCPV_THEME_COLORS PvGetThemeColors(
    VOID
    )
{
    return &PvpThemeColors;
}

_Ret_maybenull_
HBRUSH PvGetThemeBackgroundBrush(
    VOID
    )
{
    return PvpThemeBackgroundBrush;
}

// Handles WM_CTLCOLORDLG/BTN/STATIC for the properties window. Returns NULL
// when theming is disabled so the caller lets the dialog manager paint the
// control with the current system colors.
_Ret_maybenull_
HBRUSH PvThemeHandleCtlColor(
    _In_ HDC Hdc,
    _In_ BOOLEAN Static
    )
{
    if (!PvpThemeEnabled || !PvpThemeBackgroundBrush)
        return NULL;

    SetTextColor(Hdc, PvpThemeColors.WindowText);
    SetBkColor(Hdc, PvpThemeColors.WindowBackground);

    if (Static)
        SetBkMode(Hdc, TRANSPARENT);

    return PvpThemeBackgroundBrush;
}

BOOLEAN PvThemeEraseBackground(
    _In_ HWND WindowHandle,
    _In_ HDC Hdc
    )
{
    RECT rect;

    UNREFERENCED_PARAMETER(WindowHandle);

    if (!PvpThemeEnabled || !PvpThemeBackgroundBrush)
        return FALSE;

    SetBkMode(Hdc, TRANSPARENT);

    if (GetClipBox(Hdc, &rect) > NULLREGION)
        FillRect(Hdc, &rect, PvpThemeBackgroundBrush);

    return TRUE;
}

BOOLEAN PvThemeFillRect(
    _In_ HDC Hdc,
    _In_ RECT* Rect,
    _In_ COLORREF Color
    )
{
    HBRUSH brush;

    if (!PvpThemeEnabled)
        return FALSE;

    if (!(brush = CreateSolidBrush(Color)))
        return FALSE;

    SetBkMode(Hdc, TRANSPARENT);
    FillRect(Hdc, Rect, brush);
    DeleteBrush(brush);

    return TRUE;
}

BOOLEAN PvThemeFillRoundRect(
    _In_ HDC Hdc,
    _In_ RECT* Rect,
    _In_ COLORREF FillColor,
    _In_ COLORREF BorderColor,
    _In_ LONG Radius
    )
{
    INT savedDc;

    if (!PvpThemeEnabled)
        return FALSE;

    if (Rect->right <= Rect->left || Rect->bottom <= Rect->top)
        return FALSE;

    if (!(savedDc = SaveDC(Hdc)))
        return FALSE;

    SetBkMode(Hdc, TRANSPARENT);
    SelectPen(Hdc, PhGetStockPen(DC_PEN));
    SelectBrush(Hdc, PhGetStockBrush(DC_BRUSH));
    SetDCPenColor(Hdc, BorderColor);
    SetDCBrushColor(Hdc, FillColor);

    if (Radius > 0)
    {
        RoundRect(Hdc, Rect->left, Rect->top, Rect->right, Rect->bottom, Radius * 2, Radius * 2);
    }
    else
    {
        Rectangle(Hdc, Rect->left, Rect->top, Rect->right, Rect->bottom);
    }

    RestoreDC(Hdc, savedDc);

    return TRUE;
}

_Ret_maybenull_
HBRUSH PvThemeHandleCtlColorEx(
    _In_ HDC Hdc,
    _In_ COLORREF TextColor,
    _In_ COLORREF BackgroundColor
    )
{
    if (!PvpThemeEnabled || !PvpThemeBackgroundBrush)
        return NULL;

    SetTextColor(Hdc, TextColor);
    SetBkColor(Hdc, BackgroundColor);
    SetBkMode(Hdc, TRANSPARENT);

    // The control erases with the same color the card painted underneath it, so the
    // surface stays seamless without leaving stale text behind on an update. The DC
    // brush carries the color per device context, so no brush is owned here. (dmex)
    SetDCBrushColor(Hdc, BackgroundColor);

    return PhGetStockBrush(DC_BRUSH);
}

// Selects the uxtheme sub-app-name for a control based on PE Viewer's resolved
// mode rather than the current Windows preference, so an explicit Light/Dark
// selection is honored.
VOID PvThemeApplyControl(
    _In_ HWND WindowHandle
    )
{
    if (!WindowHandle)
        return;

    // PhSetControlTheme is a plain SetWindowTheme wrapper (uxtheme is resolved
    // dynamically there); the class list itself is chosen from our own mode.
    //
    // AllowDarkModeForWindow has to come first or the control keeps light
    // non-client bits - most visibly a light scrollbar - no matter which class list
    // is set. This is what phlib does for every control it themes itself. (dmex)
    if (PvpThemeEnabled)
    {
        PhAllowDarkModeForWindow(WindowHandle, PvpThemeDark);
        PhSetControlTheme(WindowHandle, PvpThemeDark ? L"DarkMode_Explorer" : L"Explorer");
    }
    else
    {
        PhAllowDarkModeForWindow(WindowHandle, FALSE);
        PhSetControlTheme(WindowHandle, NULL);
    }
}

VOID PvThemeApplyTreeView(
    _In_ HWND WindowHandle
    )
{
    LONG rowHeight;

    if (!WindowHandle)
        return;

    PvThemeApplyControl(WindowHandle);
    TreeView_SetExtendedStyle(WindowHandle, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);

    if (PvpThemeEnabled)
    {
        TreeView_SetBkColor(WindowHandle, PvpThemeColors.SidebarBackground);
        TreeView_SetTextColor(WindowHandle, PvpThemeColors.SidebarText);
    }
    else
    {
        // (COLORREF)-1 restores the control default colors.
        TreeView_SetBkColor(WindowHandle, (COLORREF)-1);
        TreeView_SetTextColor(WindowHandle, (COLORREF)-1);
    }

    rowHeight = PhScaleToDisplay(PV_SIDEBAR_ROW_HEIGHT, PhGetWindowDpi(WindowHandle));

    if (rowHeight > 0)
    {
        TreeView_SetItemHeight(WindowHandle, (WORD)rowHeight);
    }
}

// DWM attributes for the caption. Mirrors the values phlib defines privately.
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif
#ifndef DWMWA_TEXT_COLOR
#define DWMWA_TEXT_COLOR 36
#endif

VOID PvThemeUpdateWindowBorder(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN Active
    )
{
    HIGHCONTRAST highContrast = { sizeof(HIGHCONTRAST) };
    COLORREF borderColor = 0xffffffff; // DWMWA_COLOR_DEFAULT

    if (!WindowHandle || WindowsVersion < WINDOWS_11)
        return;
    if (PvThemeDarkEnabled() &&
        SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(highContrast), &highContrast, 0) &&
        !(highContrast.dwFlags & HCF_HIGHCONTRASTON))
    {
        // Keep in step with SetupUpdateWindowBorderColor in CustomSetupTool.
        borderColor = Active ? GetSysColor(COLOR_HOTLIGHT) : RGB(90, 90, 90);
    }
    PhSetWindowThemeAttribute(WindowHandle, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
}

VOID PvThemeApplyWindowFrame(
    _In_ HWND WindowHandle
    )
{
    HIGHCONTRAST highContrast = { sizeof(HIGHCONTRAST) };
    BOOLEAN customColors;
    BOOL darkMode;

    if (!WindowHandle)
        return;

    customColors = PvpThemeEnabled &&
        SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(highContrast), &highContrast, 0) &&
        !(highContrast.dwFlags & HCF_HIGHCONTRASTON);
    darkMode = customColors && PvpThemeDark;

    // Resolve from PE Viewer's palette, not phlib's theme identity. The generic
    // frame helper also changes SYSTEMBACKDROP_TYPE.
    if (WindowsVersion >= WINDOWS_10_RS5)
    {
        PhAllowDarkModeForWindow(WindowHandle, !!darkMode);
        PhSetControlTheme(WindowHandle, darkMode ? L"DarkMode_Explorer" : L"Explorer");
        if (!HR_SUCCESS(PhSetWindowThemeAttribute(WindowHandle, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */,
            &darkMode, sizeof(darkMode))))
        {
            PhSetWindowThemeAttribute(WindowHandle, 19 /* pre-20H1 */,
                &darkMode, sizeof(darkMode));
        }
    }

    if (WindowsVersion >= WINDOWS_11)
    {
        COLORREF captionColor = 0xffffffff; // DWM caption, as in CustomSetupTool
        COLORREF captionTextColor = 0xffffffff;

        PhSetWindowThemeAttribute(WindowHandle, DWMWA_CAPTION_COLOR, &captionColor, sizeof(COLORREF));
        PhSetWindowThemeAttribute(WindowHandle, DWMWA_TEXT_COLOR, &captionTextColor, sizeof(COLORREF));
        PvThemeUpdateWindowBorder(WindowHandle, GetForegroundWindow() == WindowHandle);
    }

    RedrawWindow(WindowHandle, NULL, NULL, RDW_INVALIDATE | RDW_FRAME);
}

VOID PvThemeApplyListView(
    _In_ HWND WindowHandle
    )
{
    if (!WindowHandle)
        return;

    // Unlike the other controls the listview keeps the Explorer class list when
    // theming is off, matching what the pages requested before. AllowDarkModeForWindow
    // first, so the scrollbars follow. (dmex)
    PhAllowDarkModeForWindow(WindowHandle, PvThemeDarkEnabled());
    PhSetControlTheme(WindowHandle, PvThemeDarkEnabled() ? L"DarkMode_ItemsView" : L"Explorer");

    {
        HWND tooltipHandle;

        if (tooltipHandle = ListView_GetToolTips(WindowHandle))
        {
            PvThemeApplyControl(tooltipHandle);
        }
    }

    if (PvpThemeEnabled)
    {
        // The rows of a grouped listview sit on the card painted by
        // PvThemeDrawListViewCards, so the text background is the card surface while
        // the control background stays the window color behind the gutters. Ungrouped
        // pages have no cards and get the card color across the whole control, which
        // is what lifts them off the window the same way. (dmex)
        ListView_SetBkColor(WindowHandle, ListView_IsGroupViewEnabled(WindowHandle) ?
            PvpThemeColors.ListBackground : PvpThemeColors.CardBackground);
        ListView_SetTextBkColor(WindowHandle, PvpThemeColors.CardBackground);
        ListView_SetTextColor(WindowHandle, PvpThemeColors.ListText);
        ListView_SetOutlineColor(WindowHandle, PvpThemeColors.CardBorder);

        PvThemeApplyListViewGroupMetrics(WindowHandle);
    }
    else
    {
        ListView_SetBkColor(WindowHandle, CLR_NONE);
        ListView_SetTextBkColor(WindowHandle, CLR_NONE);
        ListView_SetTextColor(WindowHandle, CLR_DEFAULT);
    }
}

VOID PvThemeSetListViewGroupAccents(
    _In_ HWND ListViewHandle,
    _In_ CONST PV_THEME_ACCENT *Accents
    )
{
    if (!ListViewHandle)
        return;

    PvThemeRemoveListViewGroupAccents(ListViewHandle);
    PhSetWindowContext(ListViewHandle, PV_THEME_ACCENT_CONTEXT_TAG, (PVOID)Accents);
}

VOID PvThemeRemoveListViewGroupAccents(
    _In_ HWND ListViewHandle
    )
{
    if (ListViewHandle)
    {
        PhRemoveWindowContext(ListViewHandle, PV_THEME_ACCENT_CONTEXT_TAG);
    }
}

// The accent a group heading and its glyph are drawn in. Pages that never
// registered a table, and group ids past the end of one, use the interactive accent.
static PV_THEME_ACCENT PvpGetListViewGroupAccent(
    _In_ HWND ListViewHandle,
    _In_ ULONG GroupId
    )
{
    CONST PV_THEME_ACCENT *accents;
    ULONG index;

    if (!(accents = PhGetWindowContext(ListViewHandle, PV_THEME_ACCENT_CONTEXT_TAG)))
        return PvThemeAccentPrimary;

    // The table is terminated rather than counted, so walking it is also the bounds
    // check: a terminator at or before the group id means there is no entry. (dmex)
    for (index = 0; index < GroupId; index++)
    {
        if (accents[index] >= PvThemeAccentMax)
            return PvThemeAccentPrimary;
    }

    if (accents[GroupId] >= PvThemeAccentMax)
        return PvThemeAccentPrimary;

    return accents[GroupId];
}

PV_THEME_ACCENT PvThemeGetListViewGroupAccent(
    _In_ HWND ListViewHandle,
    _In_ ULONG GroupId
    )
{
    if (!ListViewHandle)
        return PvThemeAccentPrimary;

    return PvpGetListViewGroupAccent(ListViewHandle, GroupId);
}

VOID PvThemeApplyListViewGroupMetrics(
    _In_ HWND ListViewHandle
    )
{
    LVGROUPMETRICS metrics;
    LONG dpiValue;

    if (!ListViewHandle || !PvpThemeEnabled)
        return;

    if (!ListView_IsGroupViewEnabled(ListViewHandle))
        return;

    dpiValue = PhGetWindowDpi(ListViewHandle);

    memset(&metrics, 0, sizeof(LVGROUPMETRICS));
    metrics.cbSize = sizeof(LVGROUPMETRICS);
    metrics.mask = LVGMF_BORDERSIZE;
    // The gutter between two cards is the bottom border of one plus the top border
    // of the next, so each carries half of it. (dmex)
    metrics.Left = PhScaleToDisplay(PV_CARD_INSET, dpiValue);
    metrics.Right = PhScaleToDisplay(PV_CARD_INSET, dpiValue);
    metrics.Top = PhScaleToDisplay(PV_CARD_GUTTER / 2, dpiValue);
    metrics.Bottom = PhScaleToDisplay(PV_CARD_GUTTER / 2, dpiValue);

    ListView_SetGroupMetrics(ListViewHandle, &metrics);
}

// Control-level custom draw. The cards themselves are painted from the group stage
// in PvThemeDrawListViewGroup: the listview erases its background *after* the
// control-level CDDS_PREPAINT, so anything drawn here is wiped before the rows
// appear, while the group stage runs after the erase and survives. What is left here
// is handing the rows the card color as their text background.
LRESULT PvThemeDrawListViewCards(
    _In_ LPNMLVCUSTOMDRAW DrawInfo
    )
{
    if (!PvpThemeEnabled)
        return CDRF_DODEFAULT;

    switch (DrawInfo->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT:
        {
            // Rows are painted on top of the card, so they must not repaint the
            // control background underneath their text. (dmex)
            DrawInfo->clrTextBk = PvpThemeColors.CardBackground;
        }
        return CDRF_DODEFAULT;
    }

    return CDRF_DODEFAULT;
}

// The rounded card behind one group: header band plus all of its rows. Drawn from the
// group prepaint, before the header content and before the rows.
static VOID PvpThemeDrawGroupCard(
    _In_ HDC Hdc,
    _In_ HWND ListViewHandle,
    _In_ ULONG GroupId,
    _In_ LONG DpiValue
    )
{
    RECT cardRect;

    if (!ListView_GetGroupRect(ListViewHandle, GroupId, LVGGR_GROUP, &cardRect))
        return;

    // GetGroupRect already excludes the group metrics borders, so the gutter between
    // cards is preserved without insetting again. (dmex)
    PvThemeFillRoundRect(
        Hdc,
        &cardRect,
        PvpThemeColors.CardBackground,
        PvpThemeColors.CardBorder,
        PhScaleToDisplay(PV_CARD_CORNER_RADIUS, DpiValue)
        );
}

// Draws the circular collapse affordance at the right edge of a group header.
static VOID PvpThemeDrawGroupChevron(
    _In_ HDC Hdc,
    _In_ PRECT HeaderRect,
    _In_ LONG DpiValue,
    _In_ BOOLEAN Collapsed,
    _In_ COLORREF AccentColor
    )
{
    INT savedDc;
    RECT buttonRect;
    POINT arrow[3];
    HPEN arrowPen;
    LONG buttonSize;
    LONG buttonMargin;
    LONG arrowWidth;
    LONG arrowHeight;
    LONG arrowWeight;
    LONG centerX;
    LONG centerY;

    buttonSize = PhScaleToDisplay(18, DpiValue);
    buttonMargin = PhScaleToDisplay(8, DpiValue);
    arrowWidth = PhScaleToDisplay(7, DpiValue);
    arrowHeight = PhScaleToDisplay(4, DpiValue);
    arrowWeight = PhScaleToDisplay(1, DpiValue);

    if (buttonSize < 12)
        buttonSize = 12;
    if (arrowWidth < 5)
        arrowWidth = 5;
    if (arrowHeight < 3)
        arrowHeight = 3;
    if (arrowWeight < 1)
        arrowWeight = 1;

    if (HeaderRect->right - HeaderRect->left < buttonSize + buttonMargin)
        return;

    buttonRect.right = HeaderRect->right - buttonMargin;
    buttonRect.left = buttonRect.right - buttonSize;
    buttonRect.top = HeaderRect->top + ((HeaderRect->bottom - HeaderRect->top) - buttonSize) / 2;
    buttonRect.bottom = buttonRect.top + buttonSize;

    if (!(savedDc = SaveDC(Hdc)))
        return;

    // The circle is an outline only, so the header fill shows through it. (dmex)
    SelectPen(Hdc, PhGetStockPen(DC_PEN));
    SelectBrush(Hdc, PhGetStockBrush(NULL_BRUSH));
    SetDCPenColor(Hdc, AccentColor);
    Ellipse(Hdc, buttonRect.left, buttonRect.top, buttonRect.right, buttonRect.bottom);

    centerX = buttonRect.left + (buttonRect.right - buttonRect.left) / 2;
    centerY = buttonRect.top + (buttonRect.bottom - buttonRect.top) / 2;

    // A stroked chevron rather than a filled triangle, matching the glyph weight of
    // the circle around it. (dmex)
    if (Collapsed)
    {
        arrow[0].x = centerX - arrowHeight / 2;
        arrow[0].y = centerY - arrowWidth / 2;
        arrow[1].x = centerX + arrowHeight / 2;
        arrow[1].y = centerY;
        arrow[2].x = centerX - arrowHeight / 2;
        arrow[2].y = centerY + arrowWidth / 2;
    }
    else
    {
        arrow[0].x = centerX - arrowWidth / 2;
        arrow[0].y = centerY - arrowHeight / 2;
        arrow[1].x = centerX;
        arrow[1].y = centerY + arrowHeight / 2;
        arrow[2].x = centerX + arrowWidth / 2;
        arrow[2].y = centerY - arrowHeight / 2;
    }

    if (arrowPen = CreatePen(PS_SOLID, arrowWeight, AccentColor))
    {
        SelectPen(Hdc, arrowPen);
        Polyline(Hdc, arrow, RTL_NUMBER_OF(arrow));
        SelectPen(Hdc, PhGetStockPen(DC_PEN));
        DeletePen(arrowPen);
    }

    RestoreDC(Hdc, savedDc);
}

// Group headers sit on the card painted by PvThemeDrawListViewCards, carrying the
// category glyph, accent-colored bold text and a hairline that separates the heading
// from the rows below it, rather than the raised gray bar phlib draws.
LRESULT PvThemeDrawListViewGroup(
    _In_ LPNMLVCUSTOMDRAW DrawInfo
    )
{
    if (!PvpThemeEnabled)
        return CDRF_DODEFAULT;

    switch (DrawInfo->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        {
            LONG dpiValue = PhGetWindowDpi(DrawInfo->nmcd.hdr.hwndFrom);
            HFONT fontHandle = NULL;
            HFONT oldFontHandle = NULL;
            NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };
            LVGROUP groupInfo;

            if (PhGetSystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, dpiValue))
            {
                metrics.lfMessageFont.lfHeight = PhScaleToDisplay(-11, dpiValue);
                metrics.lfMessageFont.lfWeight = FW_BOLD;

                fontHandle = CreateFontIndirect(&metrics.lfMessageFont);
            }

            SetBkMode(DrawInfo->nmcd.hdc, TRANSPARENT);

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
                RECT headerRect;
                RECT textRect;
                RECT separatorRect;
                COLORREF accentColor;
                HICON glyphIcon;
                LONG glyphSize;
                LONG padding;
                BOOLEAN collapsible;

                accentColor = PvGetThemeAccentColor(PvpGetListViewGroupAccent(
                    DrawInfo->nmcd.hdr.hwndFrom,
                    (ULONG)DrawInfo->nmcd.dwItemSpec
                    ));

                // rcText only covers the label, so the rest of the header row would
                // keep whatever the control erased it with and read as a lighter
                // band. Fill the real header rect instead. (dmex)
                if (!ListView_GetGroupRect(
                    DrawInfo->nmcd.hdr.hwndFrom,
                    (ULONG)DrawInfo->nmcd.dwItemSpec,
                    LVGGR_HEADER,
                    &headerRect
                    ))
                {
                    headerRect = DrawInfo->rcText;
                }

                // The card first, then the heading on top of it. Both are drawn here
                // because the group stage is the first point after the control erases
                // its background. (dmex)
                PvpThemeDrawGroupCard(
                    DrawInfo->nmcd.hdc,
                    DrawInfo->nmcd.hdr.hwndFrom,
                    (ULONG)DrawInfo->nmcd.dwItemSpec,
                    dpiValue
                    );

                // No fill for the heading: the card underneath already covers it, and a
                // second rectangular fill would square off the rounded top corners.
                // The heading separates from the rows through the hairline below. (dmex)
                padding = PhScaleToDisplay(10, dpiValue);
                glyphSize = PhScaleToDisplay(16, dpiValue);

                if (glyphSize < 12)
                    glyphSize = 12;

                // Everything in the heading is laid out inside the card, not against
                // the control edges: the label, the divider and the chevron all work
                // from the padded rect. (dmex)
                InflateRect(&headerRect, -padding, 0);

                textRect = headerRect;
                collapsible = !!(groupInfo.state & LVGS_COLLAPSIBLE);

                if (collapsible)
                {
                    textRect.right -= PhScaleToDisplay(32, dpiValue);

                    if (textRect.right < textRect.left)
                        textRect.right = textRect.left;
                }

                if (glyphIcon = PvpGetThemeCategoryIcon(
                    PvpGetListViewGroupAccent(DrawInfo->nmcd.hdr.hwndFrom, (ULONG)DrawInfo->nmcd.dwItemSpec),
                    glyphSize,
                    dpiValue
                    ))
                {
                    if (textRect.left + glyphSize <= textRect.right)
                    {
                        DrawIconEx(
                            DrawInfo->nmcd.hdc,
                            textRect.left,
                            textRect.top + ((textRect.bottom - textRect.top) - glyphSize) / 2,
                            glyphIcon,
                            glyphSize,
                            glyphSize,
                            0,
                            NULL,
                            DI_NORMAL
                            );

                        textRect.left += glyphSize + PhScaleToDisplay(8, dpiValue);
                    }
                }

                // A hairline along the bottom of the heading, inset from the card
                // edges so it reads as a divider rather than a second border. (dmex)
                separatorRect = headerRect;
                separatorRect.top = separatorRect.bottom - PhScaleToDisplay(1, dpiValue);

                if (separatorRect.bottom > separatorRect.top && separatorRect.right > separatorRect.left)
                {
                    PvThemeFillRect(DrawInfo->nmcd.hdc, &separatorRect, PvpThemeColors.CardBorder);
                }

                if (groupInfo.pszHeader)
                {
                    if (textRect.right < textRect.left)
                        textRect.right = textRect.left;

                    SetTextColor(DrawInfo->nmcd.hdc, accentColor);
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
                    PvpThemeDrawGroupChevron(
                        DrawInfo->nmcd.hdc,
                        &headerRect,
                        dpiValue,
                        !!(groupInfo.state & LVGS_COLLAPSED),
                        accentColor
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

// Blends two colors, used for the hover and pressed states of the secondary
// buttons so they stay in the same family as their resting fill.
static COLORREF PvpThemeBlendColor(
    _In_ COLORREF Color,
    _In_ COLORREF Target,
    _In_ ULONG Percent
    )
{
    ULONG remaining = 100 - Percent;

    return RGB(
        (GetRValue(Color) * remaining + GetRValue(Target) * Percent) / 100,
        (GetGValue(Color) * remaining + GetGValue(Target) * Percent) / 100,
        (GetBValue(Color) * remaining + GetBValue(Target) * Percent) / 100
        );
}

VOID PvThemeSetButtonGlyph(
    _In_ HWND ButtonHandle,
    _In_ PV_THEME_BUTTON_GLYPH Glyph
    )
{
    if (!ButtonHandle)
        return;

    PvThemeRemoveButtonGlyph(ButtonHandle);

    if (Glyph != PvThemeButtonGlyphNone)
    {
        PhSetWindowContext(ButtonHandle, PV_THEME_GLYPH_CONTEXT_TAG, (PVOID)(ULONG_PTR)Glyph);
    }
}

VOID PvThemeRemoveButtonGlyph(
    _In_ HWND ButtonHandle
    )
{
    if (ButtonHandle)
    {
        PhRemoveWindowContext(ButtonHandle, PV_THEME_GLYPH_CONTEXT_TAG);
    }
}

// The command bar glyphs, stroked in the label color so they carry the button state
// with them. Both are drawn inside a square box of Size at (X, Y).
static VOID PvpThemeDrawButtonGlyph(
    _In_ HDC Hdc,
    _In_ PV_THEME_BUTTON_GLYPH Glyph,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ LONG Size,
    _In_ COLORREF Color
    )
{
    INT savedDc;
    LONG unit = Size / 8;

    if (unit < 1)
        unit = 1;

    if (!(savedDc = SaveDC(Hdc)))
        return;

    SelectPen(Hdc, PhGetStockPen(DC_PEN));
    SelectBrush(Hdc, PhGetStockBrush(DC_BRUSH));
    SetDCPenColor(Hdc, Color);
    SetDCBrushColor(Hdc, Color);

    switch (Glyph)
    {
    case PvThemeButtonGlyphOptions:
        {
            // Three sliders, matching the Properties glyph in the sidebar.
            LONG row;

            for (row = 0; row < 3; row++)
            {
                LONG y = Y + unit * 2 + row * unit * 2;
                LONG knobX = X + unit * (row == 1 ? 4 : 2);

                MoveToEx(Hdc, X, y, NULL);
                LineTo(Hdc, X + Size, y);
                Ellipse(Hdc, knobX - unit, y - unit, knobX + unit, y + unit);
            }
        }
        break;
    case PvThemeButtonGlyphSecurity:
        {
            // A shield: straight shoulders down to a point.
            POINT shield[6];

            shield[0].x = X + Size / 2;
            shield[0].y = Y;
            shield[1].x = X + Size;
            shield[1].y = Y + unit * 2;
            shield[2].x = X + Size;
            shield[2].y = Y + Size / 2;
            shield[3].x = X + Size / 2;
            shield[3].y = Y + Size;
            shield[4].x = X;
            shield[4].y = Y + Size / 2;
            shield[5].x = X;
            shield[5].y = Y + unit * 2;

            SelectBrush(Hdc, PhGetStockBrush(NULL_BRUSH));
            Polygon(Hdc, shield, RTL_NUMBER_OF(shield));
        }
        break;
    }

    RestoreDC(Hdc, savedDc);
}

// Pushbuttons are rounded pills. The default button (Close) is accent filled; the
// rest sit on the card surface with a hairline, so the command bar keeps one
// hierarchy rather than falling back to the phlib appearance.
LRESULT PvThemeDrawButton(
    _In_ LPNMCUSTOMDRAW DrawInfo
    )
{
    ULONG style;

    if (!PvpThemeEnabled)
        return CDRF_DODEFAULT;

    style = (ULONG)PhGetWindowStyle(DrawInfo->hdr.hwndFrom);

    // Only plain pushbuttons are ours. Checkboxes, radios and group boxes share the
    // button class and are still drawn by phlib. (dmex)
    if ((style & BS_TYPEMASK) != BS_DEFPUSHBUTTON && (style & BS_TYPEMASK) != BS_PUSHBUTTON)
        return PhThemeWindowDrawButton(DrawInfo);

    switch (DrawInfo->dwDrawStage)
    {
    case CDDS_PREPAINT:
        {
            // The dialog manager moves BS_DEFPUSHBUTTON around as the focus changes, so
            // the style bit alone is not a reliable answer to "is this the default
            // button" - ask the dialog which id it considers default. (dmex)
            BOOLEAN defaultButton = (style & BS_TYPEMASK) == BS_DEFPUSHBUTTON;
            BOOLEAN disabled = !!(DrawInfo->uItemState & CDIS_DISABLED);
            COLORREF fillColor;
            COLORREF borderColor;
            COLORREF textColor;
            PPH_STRING text;
            LONG dpiValue;
            LONG radius;

            dpiValue = PhGetWindowDpi(DrawInfo->hdr.hwndFrom);
            radius = PhScaleToDisplay(4, dpiValue);

            if (!defaultButton)
            {
                HWND parentHandle;

                if (parentHandle = GetParent(DrawInfo->hdr.hwndFrom))
                {
                    LRESULT defaultId = SendMessage(parentHandle, DM_GETDEFID, 0, 0);

                    if (HIWORD(defaultId) == DC_HASDEFID &&
                        LOWORD(defaultId) == (WORD)GetDlgCtrlID(DrawInfo->hdr.hwndFrom))
                    {
                        defaultButton = TRUE;
                    }
                }
            }

            if (defaultButton)
            {
                if (DrawInfo->uItemState & CDIS_SELECTED)
                    fillColor = PvpThemeColors.AccentFillPressed;
                else if (DrawInfo->uItemState & CDIS_HOT)
                    fillColor = PvpThemeColors.AccentFillHot;
                else
                    fillColor = PvpThemeColors.AccentFill;

                borderColor = fillColor;
                textColor = PvpThemeColors.AccentText;
            }
            else
            {
                // A secondary button has to separate from the window it sits on, and the
                // card color alone is too close to it to read as a control. Lift the
                // resting fill and use the heavier border. (dmex)
                if (DrawInfo->uItemState & CDIS_SELECTED)
                    fillColor = PvpThemeBlendColor(PvpThemeColors.CardBackground, PvpThemeColors.WindowBackground, 60);
                else if (DrawInfo->uItemState & CDIS_HOT)
                    fillColor = PvpThemeBlendColor(PvpThemeColors.CardBackground, PvpThemeColors.WindowText, 14);
                else
                    fillColor = PvpThemeBlendColor(PvpThemeColors.CardBackground, PvpThemeColors.WindowText, 6);

                borderColor = PvpThemeColors.BorderColor;
                textColor = PvpThemeColors.WindowText;
            }

            if (disabled)
            {
                textColor = PvpThemeColors.SecondaryText;
            }

            PvThemeFillRoundRect(DrawInfo->hdc, &DrawInfo->rc, fillColor, borderColor, radius);

            if (text = PhGetWindowText(DrawInfo->hdr.hwndFrom))
            {
                PV_THEME_BUTTON_GLYPH glyph;
                RECT textRect = DrawInfo->rc;

                SetBkMode(DrawInfo->hdc, TRANSPARENT);
                SetTextColor(DrawInfo->hdc, textColor);

                glyph = (PV_THEME_BUTTON_GLYPH)(ULONG_PTR)PhGetWindowContext(
                    DrawInfo->hdr.hwndFrom,
                    PV_THEME_GLYPH_CONTEXT_TAG
                    );

                if (glyph != PvThemeButtonGlyphNone)
                {
                    SIZE textSize;
                    LONG glyphSize = PhScaleToDisplay(12, dpiValue);
                    LONG spacing = PhScaleToDisplay(6, dpiValue);

                    // The glyph and the label are centered as one block, so a button
                    // with a glyph still reads as centered. (dmex)
                    if (GetTextExtentPoint32(DrawInfo->hdc, text->Buffer, (LONG)text->Length / sizeof(WCHAR), &textSize))
                    {
                        LONG blockWidth = glyphSize + spacing + textSize.cx;
                        LONG blockLeft = DrawInfo->rc.left + ((DrawInfo->rc.right - DrawInfo->rc.left) - blockWidth) / 2;

                        if (blockLeft > DrawInfo->rc.left)
                        {
                            PvpThemeDrawButtonGlyph(
                                DrawInfo->hdc,
                                glyph,
                                blockLeft,
                                DrawInfo->rc.top + ((DrawInfo->rc.bottom - DrawInfo->rc.top) - glyphSize) / 2,
                                glyphSize,
                                textColor
                                );

                            textRect.left = blockLeft + glyphSize + spacing;
                        }
                    }
                }

                DrawText(
                    DrawInfo->hdc,
                    text->Buffer,
                    (UINT)text->Length / sizeof(WCHAR),
                    &textRect,
                    (textRect.left != DrawInfo->rc.left ? DT_LEFT : DT_CENTER) |
                    DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX
                    );
                PhDereferenceObject(text);
            }

            if (DrawInfo->uItemState & CDIS_FOCUS)
            {
                RECT focusRect = DrawInfo->rc;

                InflateRect(&focusRect, -PhScaleToDisplay(2, dpiValue), -PhScaleToDisplay(2, dpiValue));

                if (focusRect.right > focusRect.left && focusRect.bottom > focusRect.top)
                {
                    INT savedDc;

                    if (savedDc = SaveDC(DrawInfo->hdc))
                    {
                        SelectPen(DrawInfo->hdc, PhGetStockPen(DC_PEN));
                        SelectBrush(DrawInfo->hdc, PhGetStockBrush(NULL_BRUSH));
                        SetDCPenColor(DrawInfo->hdc, defaultButton ?
                            PvpThemeColors.AccentText : PvGetThemeAccentColor(PvThemeAccentPrimary));
                        RoundRect(
                            DrawInfo->hdc,
                            focusRect.left,
                            focusRect.top,
                            focusRect.right,
                            focusRect.bottom,
                            radius,
                            radius
                            );
                        RestoreDC(DrawInfo->hdc, savedDc);
                    }
                }
            }
        }
        return CDRF_SKIPDEFAULT;
    }

    return CDRF_DODEFAULT;
}

LRESULT PvThemeDrawSidebarItem(
    _In_ LPNMTVCUSTOMDRAW DrawInfo
    )
{
    if (!PvpThemeEnabled)
        return CDRF_DODEFAULT;

    switch (DrawInfo->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT:
        {
            LONG dpiValue;
            LONG accentWidth;
            LONG cornerSize;
            COLORREF fillColor;
            BOOLEAN selected;
            RECT clientRect;
            RECT itemRect;
            RECT accentRect;
            INT savedDc;

            DrawInfo->clrText = PvpThemeColors.SidebarText;
            DrawInfo->clrTextBk = PvpThemeColors.SidebarBackground;

            if (!(DrawInfo->nmcd.uItemState & (CDIS_SELECTED | CDIS_HOT)))
                return CDRF_DODEFAULT;

            selected = !!(DrawInfo->nmcd.uItemState & CDIS_SELECTED);
            dpiValue = PhGetWindowDpi(DrawInfo->nmcd.hdr.hwndFrom);
            accentWidth = PhScaleToDisplay(3, dpiValue);
            cornerSize = PhScaleToDisplay(4, dpiValue);

            if (accentWidth < 2)
                accentWidth = 2;

            // The item rect excludes the indent/icon area; use the full row so the
            // selection fill spans the sidebar the way the icons suggest, inset so the
            // pill floats rather than touching the pane edges. (dmex)
            GetClientRect(DrawInfo->nmcd.hdr.hwndFrom, &clientRect);
            itemRect = DrawInfo->nmcd.rc;
            itemRect.left = clientRect.left + PhScaleToDisplay(4, dpiValue);
            itemRect.right = clientRect.right - PhScaleToDisplay(4, dpiValue);

            if (itemRect.right <= itemRect.left)
            {
                itemRect.left = clientRect.left;
                itemRect.right = clientRect.right;
            }

            fillColor = selected ? PvpThemeColors.SidebarSelected : PvpThemeColors.SidebarHot;

            if (savedDc = SaveDC(DrawInfo->nmcd.hdc))
            {
                SelectPen(DrawInfo->nmcd.hdc, PhGetStockPen(DC_PEN));
                SelectBrush(DrawInfo->nmcd.hdc, PhGetStockBrush(DC_BRUSH));
                SetDCPenColor(DrawInfo->nmcd.hdc, fillColor);
                SetDCBrushColor(DrawInfo->nmcd.hdc, fillColor);
                RoundRect(
                    DrawInfo->nmcd.hdc,
                    itemRect.left,
                    itemRect.top,
                    itemRect.right,
                    itemRect.bottom,
                    cornerSize * 2,
                    cornerSize * 2
                    );
                RestoreDC(DrawInfo->nmcd.hdc, savedDc);
            }

            if (selected)
            {
                accentRect = itemRect;
                accentRect.top += PhScaleToDisplay(4, dpiValue);
                accentRect.bottom -= PhScaleToDisplay(4, dpiValue);
                accentRect.right = accentRect.left + accentWidth;

                if (accentRect.bottom > accentRect.top)
                {
                    PvThemeFillRect(
                        DrawInfo->nmcd.hdc,
                        &accentRect,
                        PvGetThemeAccentColor(PvThemeAccentPrimary)
                        );
                }
            }

            DrawInfo->clrTextBk = fillColor;

            // Clear the state bits so the uxtheme selection/hot rectangle isn't
            // painted on top of the fill above. (dmex)
            DrawInfo->nmcd.uItemState &= ~(ULONG)(CDIS_SELECTED | CDIS_HOT | CDIS_FOCUS);
        }
        return CDRF_NEWFONT;
    }

    return CDRF_DODEFAULT;
}

VOID PvThemeSetPageEraseCallback(
    _In_ HWND WindowHandle,
    _In_ PV_THEME_ERASE_CALLBACK Callback
    )
{
    if (!WindowHandle)
        return;

    PvThemeRemovePageEraseCallback(WindowHandle);
    PhSetWindowContext(WindowHandle, PV_THEME_ERASE_CONTEXT_TAG, (PVOID)Callback);
}

VOID PvThemeRemovePageEraseCallback(
    _In_ HWND WindowHandle
    )
{
    if (WindowHandle)
    {
        PhRemoveWindowContext(WindowHandle, PV_THEME_ERASE_CONTEXT_TAG);
    }
}

// PhInitializeWindowTheme replaces the dialog window procedure with its own, so it
// receives NM_CUSTOMDRAW before the dialog procedure ever runs. Chaining in front of
// it here is what lets PE Viewer override the group header and button drawing.
static LRESULT CALLBACK PvpThemePageWindowProc(
    _In_ HWND WindowHandle,
    _In_ UINT Message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    WNDPROC oldWindowProc;

    if (!(oldWindowProc = PhGetWindowContext(WindowHandle, PV_THEME_PAGE_CONTEXT_TAG)))
        return FALSE;

    switch (Message)
    {
    case WM_NCACTIVATE:
        {
            LRESULT result = CallWindowProc(oldWindowProc, WindowHandle, Message, wParam, lParam);
            PvThemeUpdateWindowBorder(WindowHandle, !!wParam);
            return result;
        }
    case WM_NCDESTROY:
        {
            PhSetWindowProcedure(WindowHandle, oldWindowProc);
            PhRemoveWindowContext(WindowHandle, PV_THEME_PAGE_CONTEXT_TAG);
            PhRemoveWindowContext(WindowHandle, PV_THEME_ERASE_CONTEXT_TAG);
        }
        break;
    case WM_ERASEBKGND:
        {
            PV_THEME_ERASE_CALLBACK eraseCallback;

            // phlib's subclass answers this message itself and returns TRUE without
            // ever reaching the page dialog procedure, so a page that decorates its own
            // background has to be called from here. (dmex)
            if (eraseCallback = PhGetWindowContext(WindowHandle, PV_THEME_ERASE_CONTEXT_TAG))
            {
                HDC hdc = (HDC)wParam;

                if (PvThemeEraseBackground(WindowHandle, hdc))
                {
                    eraseCallback(WindowHandle, hdc);
                    return TRUE;
                }
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->code == NM_CUSTOMDRAW && PvpThemeEnabled)
            {
                LPNMCUSTOMDRAW customDraw = (LPNMCUSTOMDRAW)lParam;
                WCHAR className[MAX_PATH];

                if (!NT_SUCCESS(PhGetClassName(customDraw->hdr.hwndFrom, className, RTL_NUMBER_OF(className), NULL)))
                    className[0] = UNICODE_NULL;

                if (PhEqualStringZ(className, WC_LISTVIEW, FALSE))
                {
                    LPNMLVCUSTOMDRAW listViewCustomDraw = (LPNMLVCUSTOMDRAW)customDraw;

                    if (listViewCustomDraw->dwItemType == LVCDI_GROUP)
                    {
                        return PvThemeDrawListViewGroup(listViewCustomDraw);
                    }
                    else
                    {
                        // The card stage never claims the notification: the
                        // ExtendedListView subclass owns the item and subitem stages
                        // for these pages (sort colors, PvpDrawPeImageDebugRepoHash),
                        // so the chained procedure has to see the message too and its
                        // CDRF_* flags are merged with ours. (dmex)
                        LRESULT result = PvThemeDrawListViewCards(listViewCustomDraw);

                        result |= CallWindowProc(oldWindowProc, WindowHandle, Message, wParam, lParam);

                        return result;
                    }
                }
                else if (PhEqualStringZ(className, WC_BUTTON, FALSE))
                {
                    return PvThemeDrawButton(customDraw);
                }
            }
        }
        break;
    }

    return CallWindowProc(oldWindowProc, WindowHandle, Message, wParam, lParam);
}

VOID PvThemeInitializePageDialog(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN EnableThemeSupport
    )
{
    WNDPROC oldWindowProc;

    // Must be idempotent. A page dialog reaches here twice: from its own
    // WM_INITDIALOG and again from PvCreateTabSectionDialog. Without this guard the
    // second pass is fatal: PhInitializeWindowTheme sees an outer procedure that is
    // not its own, stashes *ours* in its LONG_MAX slot and installs itself in front,
    // and then the chain below puts us back in front of that. Each procedure then
    // holds the other as "previous" and the first message recurses until the stack
    // runs out. (dmex)
    if (PhGetWindowContext(WindowHandle, PV_THEME_PAGE_CONTEXT_TAG))
        return;

    PhInitializeWindowTheme(WindowHandle, EnableThemeSupport);

    // phlib owns the LONG_MAX context slot for its own procedure, so this chains in
    // front of whatever PhInitializeWindowTheme just installed.
    oldWindowProc = (WNDPROC)GetWindowLongPtr(WindowHandle, GWLP_WNDPROC);

    if (oldWindowProc != PvpThemePageWindowProc)
    {
        PhSetWindowContext(WindowHandle, PV_THEME_PAGE_CONTEXT_TAG, oldWindowProc);
        PhSetWindowProcedure(WindowHandle, PvpThemePageWindowProc);
    }
}

// Re-resolves the palette and refreshes an open window after the theme
// settings change.
VOID PvReapplyTheme(
    _In_opt_ HWND WindowHandle
    )
{
    // Re-entrancy guard. Both PhReInitializeWindowTheme and PvThemeApplyWindowFrame
    // end up in SetWindowTheme, which synchronously sends WM_THEMECHANGED back to
    // this same window - and the tab window answers that by calling here again. The
    // first pass has already resolved the palette, so the nested calls have nothing
    // to add and would otherwise recurse until the stack ran out. (dmex)
    static BOOLEAN reapplying = FALSE;

    if (reapplying)
        return;

    reapplying = TRUE;

    PvInitializeTheme();

    if (!WindowHandle)
    {
        reapplying = FALSE;
        return;
    }

    // PvInitializeTheme re-registered the phlib palette; push it through the control
    // tree so the surfaces phlib owns pick up the new colors too. (dmex)
    PhReInitializeWindowTheme(WindowHandle);

    PvThemeApplyTreeView(GetDlgItem(WindowHandle, IDC_SECTIONTREE));
    PvThemeApplyControl(GetDlgItem(WindowHandle, IDD_CONTAINER));
    PvThemeApplyWindowFrame(WindowHandle);

    // Created pages re-apply their own listview colors from WM_THEMECHANGED. (dmex)
    {
        HWND containerHandle;

        if (containerHandle = GetDlgItem(WindowHandle, IDD_CONTAINER))
        {
            HWND childHandle = GetWindow(containerHandle, GW_CHILD);

            while (childHandle)
            {
                SendMessage(childHandle, WM_THEMECHANGED, 0, 0);
                childHandle = GetWindow(childHandle, GW_HWNDNEXT);
            }
        }
    }

    // PvInitializeTheme deleted and recreated the brush, which would leave the
    // "#32770" class brush registered by PhRegisterDialogSuperClass dangling. The
    // superclass is process-wide, so updating it through the container updates the
    // class for every dialog. (dmex)
    if (PvpThemeBackgroundBrush)
    {
        HWND containerHandle;

        if (containerHandle = GetDlgItem(WindowHandle, IDD_CONTAINER))
        {
            //SetClassLongPtr(containerHandle, GCLP_HBRBACKGROUND, (LONG_PTR)PvpThemeBackgroundBrush);
        }
    }

    InvalidateRect(WindowHandle, NULL, TRUE);

    reapplying = FALSE;
}
