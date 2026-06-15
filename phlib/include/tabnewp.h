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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PH_TABNEW_INTERNAL_ITEM
{
    PPH_STRING Text;
    LPARAM Param;
    LONG ImageIndex;
    RECT Rect;            // strip-local
    UINT Row;             // for multi-line
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
    HBRUSH AccentBrush;
    HBRUSH HotBrush;
    HPEN OutlinePen;
    HPEN BackgroundPen;

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
    LONG DragTargetIndex;
    LONG DragOriginIndex;
    POINT DragStartPoint;
    RECT DragInsertMarker;

    RECT CachedPageRect;       // last computed, in client coords
} PH_TABNEW_CONTEXT, *PPH_TABNEW_CONTEXT;

// Internal helpers
VOID PhpTabNewLayout(
    _In_ PPH_TABNEW_CONTEXT Context
    );

VOID PhpTabNewPaint(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PRECT ClientRect
    );

VOID PhpTabNewPaintWin10(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PRECT ClientRect
    );

VOID PhpTabNewPaintWin7(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PRECT ClientRect
    );

VOID PhpTabNewPaintUxTheme(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ HDC Hdc,
    _In_ PRECT ClientRect
    );

LONG PhpTabNewHitTest(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ POINT Point,
    _Out_opt_ PULONG Flags
    );

VOID PhpTabNewSetSelection(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ LONG NewIndex,
    _In_ BOOLEAN Notify
    );

VOID PhpTabNewSendLayoutNotify(
    _In_ PPH_TABNEW_CONTEXT Context
    );

VOID PhpTabNewUpdateMetrics(
    _In_ PPH_TABNEW_CONTEXT Context
    );

VOID PhpTabNewUpdateFont(
    _In_ PPH_TABNEW_CONTEXT Context
    );

VOID PhpTabNewUpdateTheme(
    _In_ PPH_TABNEW_CONTEXT Context
    );

VOID PhpTabNewBeginDrag(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ LONG ItemIndex,
    _In_ POINT Point
    );

VOID PhpTabNewUpdateDrag(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ POINT Point
    );

VOID PhpTabNewEndDrag(
    _In_ PPH_TABNEW_CONTEXT Context,
    _In_ BOOLEAN Cancel
    );

#ifdef __cplusplus
}
#endif

#endif
