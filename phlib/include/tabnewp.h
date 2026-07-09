/*
 * Copyright (c) 2026 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 */

#ifndef _PH_TABNEWP_H
#define _PH_TABNEWP_H

#include <tabnew.h>

EXTERN_C_START

typedef struct _PH_TABNEW_INTERNAL_ITEM
{
    PPH_STRING Text;
    LPARAM Param;
    LONG ImageIndex;
    RECT Rect;            // strip-local
    LONG Row;             // for multi-line
    union
    {
        ULONG Flags;
        struct
        {
            ULONG Hot : 1;
            ULONG Disabled : 1;
            ULONG Spare : 30;
        };
    };
} PH_TABNEW_INTERNAL_ITEM, *PPH_TABNEW_INTERNAL_ITEM;

typedef struct _PH_TABNEW_CONTEXT
{
    HWND WindowHandle;
    HWND ParentHandle;
    ULONG_PTR Id;
    PPH_TABNEW_MESSAGE_CALLBACK Callback;
    PVOID Context;

    PPH_LIST Items;            // of PPH_TABNEW_INTERNAL_ITEM
    PPH_LIST Pages;            // of PPH_TABNEW_PAGE (helper layer)

    LONG SelectedIndex;
    LONG HotIndex;

    PH_TABNEW_SKIN Skin;
    ULONG Side;                // TNS_TOP/BOTTOM/LEFT/RIGHT
    ULONG StyleFlags;          // TNS_MULTILINE | TNS_FIXEDWIDTH | TNS_REORDER

    LONG WindowDpi;
    HFONT Font;
    HIMAGELIST ImageList;
    HTHEME ThemeHandle;        // for PhTabNewSkinUxTheme
    HBRUSH BackgroundBrush;
    HBRUSH ActiveBrush;
    HBRUSH AccentBrush;
    HBRUSH HotBrush;
    HPEN OutlinePen;
    HPEN BackgroundPen;
    HPEN ActivePen;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG ThemeDark : 1;
            ULONG TrackingMouse : 1;
            ULONG DragArmed : 1;
            ULONG DragActive : 1;
            ULONG LayoutDirty : 1;
            ULONG HasFocus : 1;
            ULONG LayoutSuspended : 1;
            ULONG Spare : 25;
        };
    };

    LONG RowCount;
    LONG RowHeight;
    LONG StripThickness;
    LONG MinTabWidth;          // effective (DPI-scaled)
    LONG PaddingX;             // effective (DPI-scaled)
    LONG PaddingY;             // effective (DPI-scaled)
    LONG BaseMinTabWidth;      // 96-dpi base value
    LONG BasePaddingX;         // 96-dpi base value
    LONG BasePaddingY;         // 96-dpi base value
    LONG InsertMarkerWidth;    // effective (DPI-scaled)

    LONG DragSourceIndex;
    LONG DragTargetIndex;   // insertion gap [0, Items->Count], -1 when none
    LONG DragOriginIndex;
    POINT DragStartPoint;
    RECT DragInsertMarker;  // in client coords; empty when not shown
    HIMAGELIST DragImageList;

    RECT CachedPageRect;       // last computed, in client coords
} PH_TABNEW_CONTEXT, *PPH_TABNEW_CONTEXT;

#define PH_TABNEW_DEFAULT_MIN_WIDTH     60
#define PH_TABNEW_DEFAULT_PADDING_X     10
#define PH_TABNEW_DEFAULT_PADDING_Y     6
#define PH_TABNEW_INSERT_MARKER_WIDTH   3
#define PH_TABNEW_INITIAL_LIST_CAPACITY 8
#define PH_TABNEW_DEFAULT_FONT_HEIGHT   -11
#define PH_TABNEW_DARK_THEME_BRIGHTNESS 128
#define PH_TABNEW_ROW_STACK_COUNT       32
#define PH_TABNEW_PEN_WIDTH             1
#define PH_TABNEW_ACCENT_THICKNESS      2
#define PH_TABNEW_AERO_TAB_INSET        2
#define PH_TABNEW_PIXEL_OVERLAP         1
#define PH_TABNEW_LAYOUT_BUILDER_SIZE   0x80
#define PH_TABNEW_PAGE_NAME_LENGTH      256
#define PH_TABNEW_LAYOUT_DECIMAL_RADIX  10
#define PH_TABNEW_TRIVERTEX_COLOR_SHIFT 8
#define PH_TABNEW_TRIVERTEX_COUNT       2
#define PH_TABNEW_GRADIENT_RECT_LOWER   0
#define PH_TABNEW_GRADIENT_RECT_UPPER   1

#define PH_TABNEW_LIGHT_HOT_COLOR       RGB(0xE5, 0xF1, 0xFB)
#define PH_TABNEW_LIGHT_OUTLINE_COLOR   RGB(0xAC, 0xAC, 0xAC)
#define PH_TABNEW_DARK_OUTLINE_COLOR    RGB(0x55, 0x55, 0x55)
#define PH_TABNEW_DARK_INACTIVE_TOP     RGB(0x33, 0x33, 0x33)
#define PH_TABNEW_DARK_INACTIVE_BOTTOM  RGB(0x28, 0x28, 0x28)
#define PH_TABNEW_LIGHT_INACTIVE_TOP    RGB(0xF5, 0xF5, 0xF5)
#define PH_TABNEW_LIGHT_INACTIVE_BOTTOM RGB(0xE2, 0xE2, 0xE2)
#define PH_TABNEW_DARK_HOT_TOP          RGB(0x3D, 0x4A, 0x5C)
#define PH_TABNEW_DARK_HOT_BOTTOM       RGB(0x2A, 0x36, 0x48)
#define PH_TABNEW_LIGHT_HOT_TOP         RGB(0xEA, 0xF6, 0xFD)
#define PH_TABNEW_LIGHT_HOT_BOTTOM      RGB(0xD9, 0xEE, 0xF7)

LRESULT CALLBACK PhTabNewWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhTabNewDeleteCachedResources(
    _In_ PPH_TABNEW_CONTEXT Context
    );

VOID PhTabNewUpdateCachedResources(
    _In_ PPH_TABNEW_CONTEXT Context
    );

BOOLEAN PhTabNewGetItemLayoutIdentifier(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ LONG Index,
    _In_ PPH_TABNEW_LAYOUT_CALLBACK Callback,
    _In_opt_ PVOID CallbackContext,
    _Out_ PPH_STRINGREF Identifier
    );

BOOLEAN PhTabNewReadLayoutToken(
    _Inout_ PPH_STRINGREF Remaining,
    _Out_ PPH_STRINGREF Token
    );

BOOL PhTabNewMoveItem(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ LONG FromIndex,
    _In_ LONG ToIndex,
    _In_ BOOLEAN NotifyLayout
    );

// Internal helpers
VOID PhTabNewLayout(
    _In_ PPH_TABNEW_CONTEXT Context
    );

VOID PhTabNewPaint(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PRECT ClientRect
    );

VOID PhTabNewPaintWin10(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PRECT ClientRect
    );

VOID PhTabNewPaintWin7(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PRECT ClientRect
    );

VOID PhTabNewPaintUxTheme(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PRECT ClientRect
    );

LONG PhTabNewHitTest(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ POINT Point,
    _Out_opt_ PULONG Flags
    );

VOID PhTabNewSetSelection(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ LONG NewIndex,
    _In_ BOOLEAN Notify
    );

VOID PhTabNewSendLayoutNotify(
    _In_ PPH_TABNEW_CONTEXT Context
    );

VOID PhTabNewUpdateMetrics(
    _In_ PPH_TABNEW_CONTEXT Context
    );

VOID PhTabNewUpdateFont(
    _In_ PPH_TABNEW_CONTEXT Context
    );

VOID PhTabNewUpdateTheme(
    _In_ PPH_TABNEW_CONTEXT Context
    );

HIMAGELIST PhTabNewCreateDragImage(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ LONG ItemIndex,
    _Out_ PPOINT Hotspot,
    _In_ POINT Point
    );

VOID PhTabNewComputeDropTarget(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ POINT Point,
    _Out_ PLONG InsertIndex,
    _Out_ PRECT Marker
    );

VOID PhTabNewBeginDrag(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ LONG ItemIndex,
    _In_ POINT Point
    );

VOID PhTabNewUpdateDrag(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ POINT Point
    );

VOID PhTabNewEndDrag(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ BOOLEAN Cancel
    );

EXTERN_C_END

#endif
