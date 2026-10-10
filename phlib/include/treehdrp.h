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

#ifndef _PH_TREEHDRP_H
#define _PH_TREEHDRP_H

#include <treehdr.h>

EXTERN_C_START

// Default metrics, in 96dpi units.
#define PH_HEADERNEW_PADDING_X 6
#define PH_HEADERNEW_PADDING_Y 3
#define PH_HEADERNEW_DIVIDER_GRIP 4  // half-width of the divider drag zone
#define PH_HEADERNEW_BITMAP_MARGIN 3
#define PH_HEADERNEW_DRAG_THRESHOLD 4
#define PH_HEADERNEW_MINIMUM_HEIGHT 16

// Light theme fallbacks, used when no uxtheme handle is available.
#define PH_HEADERNEW_COLOR_BACKGROUND RGB(255, 255, 255)
#define PH_HEADERNEW_COLOR_HOT        RGB(232, 242, 254)
#define PH_HEADERNEW_COLOR_PRESSED    RGB(205, 226, 252)
#define PH_HEADERNEW_COLOR_DIVIDER    RGB(229, 229, 229)
#define PH_HEADERNEW_COLOR_TEXT       RGB(0, 0, 0)

// Matches the tab control's dark-theme detection threshold.
#define PH_HEADERNEW_DARK_THEME_BRIGHTNESS 128

/**
 * A single header item. The index of an item is its position in
 * PH_HEADERNEW_CONTEXT.Items; its left-to-right position is Order. The two are
 * independent, exactly as with SysHeader32.
 */
typedef struct _PH_HEADERNEW_ITEM
{
    PPH_STRING Text;
    LPARAM Param;
    LONG Width;     // HDITEM.cxy; HDI_HEIGHT is an alias of HDI_WIDTH
    LONG Order;
    ULONG Format;               // HDF_*
    ULONG State;                // HDIS_*
    LONG ImageIndex;
    HBITMAP Bitmap;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG TextCallback : 1;     // LPSTR_TEXTCALLBACK
            ULONG ImageCallback : 1;    // I_IMAGECALLBACK
            ULONG Spare : 30;
        };
    };

    RECT Rect;                  // computed by PhHeaderNewLayoutItems
} PH_HEADERNEW_ITEM, *PPH_HEADERNEW_ITEM;

typedef struct _PH_HEADERNEW_CONTEXT
{
    HWND WindowHandle;
    HWND ParentHandle;
    ULONG Style;                // HDS_* | WS_*
    LONG_PTR Id;

    PPH_LIST Items;
    HFONT FontHandle;
    HIMAGELIST ImageListHandle;
    HTHEME ThemeHandle;

    LONG WindowDpi;
    LONG HeaderHeight;          // measured, or CustomHeight when non-zero
    LONG CustomHeight;          // PHHM_SETHEIGHT override, 0 = measure
    LONG PaddingX;
    LONG PaddingY;
    LONG DividerGrip;
    LONG BitmapMargin;

    ULONG Flags;                // PHHF_*
    ULONG Theme;                // PH_HEADERNEW_THEME

    LONG HotIndex;
    LONG PressedIndex;
    LONG FocusedIndex;
    LONG HotDivider;            // index of the item whose right divider is marked

    // Divider tracking (column resize).
    LONG TrackIndex;
    LONG TrackStartX;
    LONG TrackStartWidth;

    // Drag reorder.
    LONG DragIndex;
    LONG DragOrder;             // proposed order, INT_ERROR while undecided
    POINT DragStartPoint;

    union
    {
        ULONG StateFlags;
        struct
        {
            ULONG OwnFont : 1;
            ULONG ThemeDark : 1;
            ULONG Tracking : 1;
            ULONG DragPending : 1;
            ULONG Dragging : 1;
            ULONG MouseActive : 1;
            ULONG Redraw : 1;
            ULONG UnicodeFormat : 1;
            ULONG NeedsLayout : 1;
            ULONG Spare : 23;
        };
    };

    // Back buffer.
    HDC BufferedDc;
    HBITMAP BufferedBitmap;
    HBITMAP BufferedOldBitmap;
    RECT BufferedRect;

    PPH_HEADERNEW_MESSAGE_CALLBACK Callback;
    PVOID Context;
} PH_HEADERNEW_CONTEXT, *PPH_HEADERNEW_CONTEXT;

LRESULT CALLBACK PhHeaderNewWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

EXTERN_C_END

#endif
