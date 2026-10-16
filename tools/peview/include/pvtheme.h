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

#ifndef PVTHEME_H
#define PVTHEME_H

// PE Viewer owns its own window colors for the properties window chrome. The
// palette here is deliberately independent of phlib: nothing in this module
// reads or writes the PhThemeWindow* globals or calls the phlib palette API.

// Window context slot for the property page window procedure chained in front of
// phlib's (which owns LONG_MAX).
#define PV_THEME_PAGE_CONTEXT_TAG ((ULONG)'pvth')

// Window context slot on a listview holding its group accent table.
#define PV_THEME_ACCENT_CONTEXT_TAG ((ULONG)'pvac')

// Window context slot on a pushbutton holding its PV_THEME_BUTTON_GLYPH.
#define PV_THEME_GLYPH_CONTEXT_TAG ((ULONG)'pvbg')

// Window context slot on a page dialog holding its PV_THEME_ERASE_CALLBACK.
#define PV_THEME_ERASE_CONTEXT_TAG ((ULONG)'pver')

// Listview row height at 96dpi, applied through the placeholder image list in
// PvSetListViewImageList. PvpGetTreeNewRowHeight falls back to the same value so
// treenew pages match the listview pages.
#define PV_LISTVIEW_ROW_HEIGHT 24

// Section sidebar metrics at 96dpi.
#define PV_SIDEBAR_ICON_SIZE 20
#define PV_SIDEBAR_ROW_HEIGHT 32

// Section sidebar icons. The order matches the image list built by
// PvSetTreeViewImageList and the glyphs drawn by PvGetSectionIcon.
typedef enum _PV_SECTION_ICON_INDEX
{
    PV_SECTION_ICON_GENERAL,
    PV_SECTION_ICON_HEADERS,
    PV_SECTION_ICON_LOAD_CONFIG,
    PV_SECTION_ICON_SECTIONS,
    PV_SECTION_ICON_DIRECTORIES,
    PV_SECTION_ICON_IMPORTS,
    PV_SECTION_ICON_EXPORTS,
    PV_SECTION_ICON_RESOURCES,
    PV_SECTION_ICON_CFG,
    PV_SECTION_ICON_PDBID,
    PV_SECTION_ICON_EXCEPTIONS,
    PV_SECTION_ICON_RELOCATIONS,
    PV_SECTION_ICON_CERTIFICATES,
    PV_SECTION_ICON_DEBUG,
    PV_SECTION_ICON_VOLATILE,
    PV_SECTION_ICON_EHCONT,
    PV_SECTION_ICON_POGO,
    PV_SECTION_ICON_CRT,
    PV_SECTION_ICON_PROPERTIES,
    PV_SECTION_ICON_ATTRIBUTES,
    PV_SECTION_ICON_STREAMS,
    PV_SECTION_ICON_LAYOUT,
    PV_SECTION_ICON_LINKS,
    PV_SECTION_ICON_PROCESSES,
    PV_SECTION_ICON_HASHES,
    PV_SECTION_ICON_PREVIEW,
    PV_SECTION_ICON_SYMBOLS,
    PV_SECTION_ICON_STRINGS,
    PV_SECTION_ICON_VERSION,
    PV_SECTION_ICON_MUI,
    PV_SECTION_ICON_ANOMALIES,
    PV_SECTION_ICON_GETLOADLIBRARY,
    PV_SECTION_ICON_GETPROCADDR,
    PV_SECTION_ICON_MAX
} PV_SECTION_ICON_INDEX;

EXTERN_C HICON PvGetSectionIcon(
    _In_ PV_SECTION_ICON_INDEX Index,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ LONG Dpi
    );

// Metadata card metrics at 96dpi. The corner radius is shared by the group cards,
// the General page hero card and the version chip so the surfaces read as one set.
#define PV_CARD_CORNER_RADIUS 6
#define PV_CARD_GUTTER 8
// Horizontal inset stays zero: rows are laid out across the full control width, so a
// left/right group border would leave them overhanging the card corners. (dmex)
#define PV_CARD_INSET 0

typedef enum _PV_THEME_MODE
{
    PvThemeModeAutomatic = 0,
    PvThemeModeLight = 1,
    PvThemeModeDark = 2
} PV_THEME_MODE;

typedef struct _PV_THEME_COLORS
{
    COLORREF WindowBackground;      // dialog background and WM_ERASEBKGND fill
    COLORREF WindowText;            // static and button text
    COLORREF SidebarBackground;     // section treeview background
    COLORREF SidebarText;
    COLORREF SidebarSelected;       // section treeview selection fill
    COLORREF SplitterColor;
    COLORREF BorderColor;
    COLORREF AccentColor;           // group header text, sidebar bar, links
    COLORREF AccentFill;            // default pushbutton fill
    COLORREF AccentFillHot;
    COLORREF AccentFillPressed;
    COLORREF AccentText;            // text drawn on top of an accent fill
    COLORREF SurfaceColor;          // surfaces lifted off the window background,
                                    // including the listview group header band
    COLORREF ListBackground;        // listview rows
    COLORREF ListText;
    COLORREF ListSelected;
    COLORREF CardBackground;        // metadata card fill, one step off the window
    COLORREF CardBorder;            // card hairline, subtler than BorderColor
    COLORREF SecondaryText;         // subtitle and secondary metadata text
    COLORREF ChipBackground;        // version chip fill
    COLORREF ChipText;
    COLORREF SidebarHot;            // sidebar hover fill
} PV_THEME_COLORS, *PPV_THEME_COLORS;

typedef const PV_THEME_COLORS *PCPV_THEME_COLORS;

// Category accents. PvThemeAccentPrimary is the single interactive accent used for
// selection, focus and the default pushbutton; the rest tint a metadata category
// heading and its glyph so grouped information separates at a glance.
typedef enum _PV_THEME_ACCENT
{
    PvThemeAccentPrimary = 0,
    PvThemeAccentInfo,
    PvThemeAccentDebug,
    PvThemeAccentTrust,
    PvThemeAccentInternal,
    PvThemeAccentMax
} PV_THEME_ACCENT;

// EXTERN_C: the glyph renderer in peprpicon.cpp resolves its color through this.
EXTERN_C COLORREF PvGetThemeAccentColor(
    _In_ PV_THEME_ACCENT Accent
    );

VOID PvInitializeTheme(
    VOID
    );

VOID PvDeleteTheme(
    VOID
    );

EXTERN_C BOOLEAN PvThemeEnabled(
    VOID
    );

// EXTERN_C: also called from peprpicon.cpp, and this header has no C++ linkage
// wrapper of its own.
EXTERN_C BOOLEAN PvThemeDarkEnabled(
    VOID
    );

_Ret_notnull_
EXTERN_C PCPV_THEME_COLORS PvGetThemeColors(
    VOID
    );

_Ret_maybenull_
HBRUSH PvGetThemeBackgroundBrush(
    VOID
    );

_Ret_maybenull_
HBRUSH PvThemeHandleCtlColor(
    _In_ HDC Hdc,
    _In_ BOOLEAN Static
    );

BOOLEAN PvThemeEraseBackground(
    _In_ HWND WindowHandle,
    _In_ HDC Hdc
    );

BOOLEAN PvThemeFillRect(
    _In_ HDC Hdc,
    _In_ RECT* Rect,
    _In_ COLORREF Color
    );

// Rounded surface used by the metadata cards, the hero card and the chip. Passing
// BorderColor equal to FillColor draws a fill without a visible hairline.
BOOLEAN PvThemeFillRoundRect(
    _In_ HDC Hdc,
    _In_ RECT* Rect,
    _In_ COLORREF FillColor,
    _In_ COLORREF BorderColor,
    _In_ LONG Radius
    );

// Per-control colors for the hero card text, where the page needs a color other
// than WindowText on the card surface.
_Ret_maybenull_
HBRUSH PvThemeHandleCtlColorEx(
    _In_ HDC Hdc,
    _In_ COLORREF TextColor,
    _In_ COLORREF BackgroundColor
    );

VOID PvThemeApplyControl(
    _In_ HWND WindowHandle
    );

VOID PvThemeApplyTreeView(
    _In_ HWND WindowHandle
    );

// Callers must enable group view before this runs: the background color and the card
// gutter are chosen from whether the listview is grouped.
VOID PvThemeApplyListView(
    _In_ HWND WindowHandle
    );

// Match CustomSetupTool's active/inactive dark non-client border.
EXTERN_C VOID PvThemeUpdateWindowBorder(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN Active
    );

// Dark caption/border for a top-level window.
EXTERN_C VOID PvThemeApplyWindowFrame(
    _In_ HWND WindowHandle
    );

// Custom draw handlers. Each returns a CDRF_* value and is only meaningful while
// PvThemeEnabled(); the callers fall back to the default painting otherwise.

LRESULT PvThemeDrawListViewGroup(
    _In_ LPNMLVCUSTOMDRAW DrawInfo
    );

// Control-level prepaint: paints the rounded card behind every group before the
// group headers and rows are drawn. Never returns CDRF_SKIPDEFAULT, so the caller
// must merge the result with the rest of the custom draw chain.
LRESULT PvThemeDrawListViewCards(
    _In_ LPNMLVCUSTOMDRAW DrawInfo
    );

// Registers the group accent table for a listview. Accents is indexed by group id
// and terminated with PvThemeAccentMax; the array is not copied, so it has to
// outlive the control (the pages pass a static table). Groups past the end of the
// table fall back to PvThemeAccentPrimary.
VOID PvThemeSetListViewGroupAccents(
    _In_ HWND ListViewHandle,
    _In_ CONST PV_THEME_ACCENT *Accents
    );

VOID PvThemeRemoveListViewGroupAccents(
    _In_ HWND ListViewHandle
    );

// The accent registered for one group.
EXTERN_C PV_THEME_ACCENT PvThemeGetListViewGroupAccent(
    _In_ HWND ListViewHandle,
    _In_ ULONG GroupId
    );

// Applies the card gutter and row height to a grouped listview.
VOID PvThemeApplyListViewGroupMetrics(
    _In_ HWND ListViewHandle
    );

LRESULT PvThemeDrawButton(
    _In_ LPNMCUSTOMDRAW DrawInfo
    );

// Glyph drawn to the left of a command bar button label, in the label color.
typedef enum _PV_THEME_BUTTON_GLYPH
{
    PvThemeButtonGlyphNone = 0,
    PvThemeButtonGlyphOptions,
    PvThemeButtonGlyphSecurity
} PV_THEME_BUTTON_GLYPH;

VOID PvThemeSetButtonGlyph(
    _In_ HWND ButtonHandle,
    _In_ PV_THEME_BUTTON_GLYPH Glyph
    );

VOID PvThemeRemoveButtonGlyph(
    _In_ HWND ButtonHandle
    );

LRESULT PvThemeDrawSidebarItem(
    _In_ LPNMTVCUSTOMDRAW DrawInfo
    );

// Extra decoration a page paints on its own background, such as the General page
// hero card. phlib's subclass answers WM_ERASEBKGND itself and returns without
// reaching DefDlgProc, so a page dialog procedure never sees the message; the
// chained procedure calls this instead, right after the background is filled.
typedef VOID (NTAPI *PV_THEME_ERASE_CALLBACK)(
    _In_ HWND WindowHandle,
    _In_ HDC Hdc
    );

VOID PvThemeSetPageEraseCallback(
    _In_ HWND WindowHandle,
    _In_ PV_THEME_ERASE_CALLBACK Callback
    );

VOID PvThemeRemovePageEraseCallback(
    _In_ HWND WindowHandle
    );

// Initializes phlib theming for a property page and then chains PE Viewer's own
// window procedure in front of it, so the drawing above overrides phlib's.
VOID PvThemeInitializePageDialog(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN EnableThemeSupport
    );

VOID PvReapplyTheme(
    _In_opt_ HWND WindowHandle
    );

EXTERN_C HICON PvGetCategoryIcon(
    _In_ PV_THEME_ACCENT Accent,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ LONG Dpi
    );

#endif
