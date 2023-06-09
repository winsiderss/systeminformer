/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2016
 *     dmex    2017-2023
 *
 */

#ifndef _PH_TREENEWP_H
#define _PH_TREENEWP_H

// Important notes about pointers:
//
// All memory allocation for nodes and strings is handled by the user. This usually means there is a
// very limited time during which they can be safely accessed.
//
// Node pointers are valid through the duration of message processing, and also up to the next
// restructure operation, either user- or control- initiated. This means that state such as the
// focused node, hot node and mark node must be carefully preserved through restructuring. If
// restructuring is suspended by a set-redraw call, all nodes must be considered invalid and no user
// input can be handled.
//
// Strings are valid only through the duration of message processing.

typedef struct _PH_TREENEW_CONTEXT
{
    HWND Handle;
    PVOID InstanceHandle;
    HWND FixedHeaderHandle;
    HWND HeaderHandle;
    HWND VScrollHandle;
    HWND HScrollHandle;
    HWND FillerBoxHandle;
    HWND TooltipsHandle;

    union
    {
        struct
        {
            ULONG FontOwned : 1;
            ULONG Tracking : 1; // tracking for fixed divider
            ULONG VScrollVisible : 1;
            ULONG HScrollVisible : 1;
            ULONG FixedColumnVisible : 1;
            ULONG FixedDividerVisible : 1;
            ULONG AnimateDivider : 1;
            ULONG AnimateDividerFadingIn : 1;
            ULONG AnimateDividerFadingOut : 1;
            ULONG CanAnyExpand : 1;
            ULONG TriState : 1;
            ULONG HasFocus : 1;
            ULONG ThemeInitialized : 1; // delay load theme data
            ULONG ThemeActive : 1;
            ULONG ThemeHasItemBackground : 1;
            ULONG ThemeHasGlyph : 1;
            ULONG ThemeHasHotGlyph : 1;
            ULONG FocusNodeFound : 1; // used to preserve the focused node across restructuring
            ULONG SearchFailed : 1; // used to prevent multiple beeps
            ULONG SearchSingleCharMode : 1; // LV style single-character search
            ULONG TooltipUnfolding : 1; // whether the current tooltip is unfolding
            ULONG DoubleBuffered : 1;
            ULONG SuspendUpdateStructure : 1;
            ULONG SuspendUpdateLayout : 1;
            ULONG SuspendUpdateMoveMouse : 1;
            ULONG DragSelectionActive : 1;
            ULONG SelectionRectangleAlpha : 1; // use alpha blending for the selection rectangle
            ULONG CustomRowHeight : 1;
            ULONG CustomColors : 1;
            ULONG ContextMenuActive : 1;
            ULONG ThemeSupport : 1;
            ULONG ImageListSupport : 1;
        };
        ULONG Flags;
    };
    ULONG Style;
    ULONG ExtendedStyle;
    ULONG ExtendedFlags;

    HFONT Font;
    HCURSOR Cursor;

    RECT ClientRect;
    LONG HeaderHeight;
    LONG RowHeight;
    ULONG VScrollWidth;
    ULONG HScrollHeight;
    LONG VScrollPosition;
    LONG HScrollPosition;
    LONG FixedWidth; // width of the fixed part of the tree list
    LONG FixedWidthMinimum;
    LONG NormalLeft; // FixedWidth + 1 if there is a fixed column, otherwise 0

    PPH_TREENEW_NODE FocusNode;
    ULONG HotNodeIndex;
    ULONG MarkNodeIndex; // selection mark

    ULONG MouseDownLast;
    POINT MouseDownLocation;

    PPH_TREENEW_CALLBACK Callback;
    PVOID CallbackContext;

    PPH_TREENEW_COLUMN *Columns; // columns, indexed by ID
    ULONG NextId;
    ULONG AllocatedColumns;
    ULONG NumberOfColumns; // just a statistic; do not use for actual logic

    PPH_TREENEW_COLUMN *ColumnsByDisplay; // columns, indexed by display order (excluding the fixed column)
    ULONG AllocatedColumnsByDisplay;
    ULONG NumberOfColumnsByDisplay; // the number of visible columns (excluding the fixed column)
    LONG TotalViewX; // total width of normal columns
    PPH_TREENEW_COLUMN FixedColumn;
    PPH_TREENEW_COLUMN FirstColumn; // first column, by display order (including the fixed column)
    PPH_TREENEW_COLUMN LastColumn; // last column, by display order (including the fixed column)

    PPH_TREENEW_COLUMN ResizingColumn;
    LONG OldColumnWidth;
    LONG TrackStartX;
    LONG TrackOldFixedWidth;
    ULONG DividerHot; // 0 for un-hot, 100 for completely hot

    PPH_LIST FlatList;

    ULONG SortColumn; // ID of the column to sort by
    PH_SORT_ORDER SortOrder;

    FLOAT VScrollRemainder;
    FLOAT HScrollRemainder;

    LONG SearchMessageTime;
    PWSTR SearchString;
    ULONG SearchStringCount;
    ULONG AllocatedSearchString;

    ULONG TooltipIndex;
    ULONG TooltipId;
    PPH_STRING TooltipText;
    RECT TooltipRect; // text rectangle of an unfolding tooltip
    HFONT TooltipFont;
    HFONT NewTooltipFont;
    ULONG TooltipColumnId;

    TEXTMETRIC TextMetrics;
    HTHEME ThemeData;
    COLORREF DefaultBackColor;
    COLORREF DefaultForeColor;

    // User configurable colors.
    COLORREF CustomTextColor;
    COLORREF CustomFocusColor;
    COLORREF CustomSelectedColor;

    LONG SystemBorderX;
    LONG SystemBorderY;
    LONG SystemEdgeX;
    LONG SystemEdgeY;

    HDC BufferedContext;
    HBITMAP BufferedOldBitmap;
    HBITMAP BufferedBitmap;
    RECT BufferedContextRect;

    LONG SystemDragX;
    LONG SystemDragY;
    RECT DragRect;

    LONG WindowDpi;
    LONG SmallIconWidth;
    LONG SmallIconHeight;

    LONG EnableRedraw;
    HRGN SuspendUpdateRegion;

    PH_STRINGREF EmptyText;

    WNDPROC HeaderWindowProc;
    WNDPROC FixedHeaderWindowProc;
    HIMAGELIST ImageListHandle;

    union
    {
        ULONG HeaderFlags;
        struct
        {
            ULONG HeaderCustomDraw : 1;
            ULONG HeaderMouseActive : 1;
            ULONG HeaderDragging : 1;
            ULONG HeaderUnused : 13;

            ULONG HeaderHotColumn : 16; // HACK (dmex)
        };
    };

    HTHEME HeaderThemeHandle;
    HFONT HeaderBoldFontHandle;
    HDC HeaderBufferedDc;
    HBITMAP HeaderBufferedOldBitmap;
    HBITMAP HeaderBufferedBitmap;
    RECT HeaderBufferedContextRect;

    ULONG HeaderColumnCacheMax;
    PPH_STRINGREF HeaderStringCache;
    PVOID HeaderTextCache;
} PH_TREENEW_CONTEXT, *PPH_TREENEW_CONTEXT;

LRESULT CALLBACK PhTnpWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

BOOLEAN NTAPI PhTnpNullCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    );

VOID PhTnpCreateTreeNewContext(
    _Out_ PPH_TREENEW_CONTEXT *Context
    );

VOID PhTnpDestroyTreeNewContext(
    _In_ PPH_TREENEW_CONTEXT Context
    );

// Event handlers

BOOLEAN PhTnpOnCreate(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ CREATESTRUCT *CreateStruct
    );

VOID PhTnpOnSize(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpOnSetFont(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_opt_ HFONT Font,
    _In_ LOGICAL Redraw
    );

VOID PhTnpOnStyleChanged(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG Type,
    _In_ STYLESTRUCT *StyleStruct
    );

VOID PhTnpOnSettingChange(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpOnThemeChanged(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpOnDpiChanged(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context
    );

ULONG PhTnpOnGetDlgCode(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG VirtualKey,
    _In_opt_ PMSG Message
    );

VOID PhTnpOnPaint(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpOnPrintClient(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ HDC hdc,
    _In_ ULONG Flags
    );

BOOLEAN PhTnpOnNcPaint(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_opt_ HRGN UpdateRegion
    );

BOOLEAN PhTnpOnSetCursor(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ HWND CursorWindowHandle
    );

VOID PhTnpOnTimer(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Id
    );

VOID PhTnpOnMouseMove(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG VirtualKeys,
    _In_ LONG CursorX,
    _In_ LONG CursorY
    );

VOID PhTnpOnMouseLeave(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpOnXxxButtonXxx(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Message,
    _In_ ULONG VirtualKeys,
    _In_ LONG CursorX,
    _In_ LONG CursorY
    );

VOID PhTnpOnCaptureChanged(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpOnKeyDown(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG VirtualKey,
    _In_ ULONG Data
    );

VOID PhTnpOnChar(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Character,
    _In_ ULONG Data
    );

VOID PhTnpOnMouseWheel(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG Distance,
    _In_ ULONG VirtualKeys,
    _In_ LONG CursorX,
    _In_ LONG CursorY
    );

VOID PhTnpOnMouseHWheel(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG Distance,
    _In_ ULONG VirtualKeys,
    _In_ LONG CursorX,
    _In_ LONG CursorY
    );

VOID PhTnpOnContextMenu(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG CursorScreenX,
    _In_ LONG CursorScreenY
    );

VOID PhTnpOnVScroll(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Request,
    _In_ USHORT Position
    );

VOID PhTnpOnHScroll(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Request,
    _In_ USHORT Position
    );

BOOLEAN PhTnpOnNotify(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ NMHDR *Header,
    _Out_ LRESULT *Result
    );

ULONG_PTR PhTnpOnUserMessage(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Message,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam
    );

// Misc.

VOID PhTnpSetFont(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_opt_ HFONT Font,
    _In_ BOOLEAN Redraw
    );

VOID PhTnpUpdateSystemMetrics(
    _In_ PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpUpdateTextMetrics(
    _In_ PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpUpdateThemeData(
    _In_ PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpInitializeThemeData(
    _In_ PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpCancelTrack(
    _In_ PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpLayout(
    _In_ PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpLayoutHeader(
    _In_ PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpSetFixedWidth(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG FixedWidth
    );

VOID PhTnpSetRedraw(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ BOOLEAN Redraw
    );

VOID PhTnpSendMouseEvent(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ LONG CursorX,
    _In_ LONG CursorY,
    _In_ PPH_TREENEW_NODE Node,
    _In_ PPH_TREENEW_COLUMN Column,
    _In_ ULONG VirtualKeys
    );

// Columns

PPH_TREENEW_COLUMN PhTnpLookupColumnById(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Id
    );

BOOLEAN PhTnpAddColumn(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPH_TREENEW_COLUMN Column
    );

BOOLEAN PhTnpRemoveColumn(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Id
    );

BOOLEAN PhTnpCopyColumn(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Id,
    _Out_ PPH_TREENEW_COLUMN Column
    );

BOOLEAN PhTnpChangeColumn(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Mask,
    _In_ ULONG Id,
    _In_ PPH_TREENEW_COLUMN Column
    );

VOID PhTnpExpandAllocatedColumns(
    _In_ PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpUpdateColumnMaps(
    _In_ PPH_TREENEW_CONTEXT Context
    );

// Columns (header control)

LONG PhTnpInsertColumnHeader(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPH_TREENEW_COLUMN Column
    );

VOID PhTnpChangeColumnHeader(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Mask,
    _In_ PPH_TREENEW_COLUMN Column
    );

VOID PhTnpDeleteColumnHeader(
    _In_ PPH_TREENEW_CONTEXT Context,
    _Inout_ PPH_TREENEW_COLUMN Column
    );

VOID PhTnpUpdateColumnHeaders(
    _In_ PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpProcessResizeColumn(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPH_TREENEW_COLUMN Column,
    _In_ LONG Delta
    );

VOID PhTnpProcessSortColumn(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPH_TREENEW_COLUMN NewColumn
    );

BOOLEAN PhTnpSetColumnHeaderSortIcon(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_opt_ PPH_TREENEW_COLUMN SortColumnPointer
    );

VOID PhTnpAutoSizeColumnHeader(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ HWND HeaderHandle,
    _In_ PPH_TREENEW_COLUMN Column,
    _In_ ULONG Flags
    );

// Nodes

BOOLEAN PhTnpGetNodeChildren(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_opt_ PPH_TREENEW_NODE Node,
    _Out_ PPH_TREENEW_NODE **Children,
    _Out_ PULONG NumberOfChildren
    );

BOOLEAN PhTnpIsNodeLeaf(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPH_TREENEW_NODE Node
    );

_Success_(return)
BOOLEAN PhTnpGetCellText(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPH_TREENEW_NODE Node,
    _In_ ULONG Id,
    _Out_ PPH_STRINGREF Text
    );

VOID PhTnpRestructureNodes(
    _In_ PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpInsertNodeChildren(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPH_TREENEW_NODE Node,
    _In_ ULONG Level
    );

VOID PhTnpSetExpandedNode(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPH_TREENEW_NODE Node,
    _In_ BOOLEAN Expanded
    );

BOOLEAN PhTnpGetCellParts(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Index,
    _In_opt_ PPH_TREENEW_COLUMN Column,
    _In_ ULONG Flags,
    _Out_ PPH_TREENEW_CELL_PARTS Parts
    );

BOOLEAN PhTnpGetRowRects(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Start,
    _In_ ULONG End,
    _In_ BOOLEAN Clip,
    _Out_ PRECT Rect
    );

VOID PhTnpHitTest(
    _In_ PPH_TREENEW_CONTEXT Context,
    _Inout_ PPH_TREENEW_HIT_TEST HitTest
    );

VOID PhTnpSelectRange(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Start,
    _In_ ULONG End,
    _In_ ULONG Flags,
    _Out_opt_ PULONG ChangedStart,
    _Out_opt_ PULONG ChangedEnd
    );

VOID PhTnpSetHotNode(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_opt_ PPH_TREENEW_NODE NewHotNode,
    _In_ BOOLEAN NewPlusMinusHot
    );

VOID PhTnpProcessSelectNode(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPH_TREENEW_NODE Node,
    _In_ LOGICAL ControlKey,
    _In_ LOGICAL ShiftKey,
    _In_ LOGICAL RightButton
    );

BOOLEAN PhTnpEnsureVisibleNode(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Index
    );

// Mouse

VOID PhTnpProcessMoveMouse(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG CursorX,
    _In_ LONG CursorY
    );

VOID PhTnpProcessMouseVWheel(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG Distance
    );

VOID PhTnpProcessMouseHWheel(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG Distance
    );

// Keyboard

BOOLEAN PhTnpProcessFocusKey(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG VirtualKey
    );

BOOLEAN PhTnpProcessNodeKey(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG VirtualKey
    );

VOID PhTnpProcessSearchKey(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG Character
    );

BOOLEAN PhTnpDefaultIncrementalSearch(
    _In_ PPH_TREENEW_CONTEXT Context,
    _Inout_ PPH_TREENEW_SEARCH_EVENT SearchEvent,
    _In_ BOOLEAN Partial,
    _In_ BOOLEAN Wrap
    );

// Scrolling

VOID PhTnpUpdateScrollBars(
    _In_ PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpScroll(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG DeltaRows,
    _In_ LONG DeltaX
    );

VOID PhTnpProcessScroll(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG DeltaRows,
    _In_ LONG DeltaX
    );

BOOLEAN PhTnpCanScroll(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ BOOLEAN Horizontal,
    _In_ BOOLEAN Positive
    );

// Drawing

VOID PhTnpPaint(
    _In_ HWND hwnd,
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ HDC hdc,
    _In_ PRECT PaintRect
    );

VOID PhTnpPrepareRowForDraw(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ HDC hdc,
    _Inout_ PPH_TREENEW_NODE Node
    );

VOID PhTnpDrawCell(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ HDC hdc,
    _In_ PRECT CellRect,
    _In_ PPH_TREENEW_NODE Node,
    _In_ PPH_TREENEW_COLUMN Column,
    _In_ LONG RowIndex,
    _In_ LONG ColumnIndex
    );

VOID PhTnpDrawDivider(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ HDC hdc
    );

VOID PhTnpDrawPlusMinusGlyph(
    _In_ HDC hdc,
    _In_ PRECT Rect,
    _In_ BOOLEAN Plus
    );

VOID PhTnpDrawSelectionRectangle(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ HDC hdc,
    _In_ PRECT Rect
    );

VOID PhTnpDrawThemedBorder(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ HDC hdc
    );

// Tooltips

VOID PhTnpInitializeTooltips(
    _In_ PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpGetTooltipText(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPOINT Point,
    _Outptr_ PWSTR *Text
    );

BOOLEAN PhTnpPrepareTooltipShow(
    _In_ PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpPrepareTooltipPop(
    _In_ PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpPopTooltip(
    _In_ PPH_TREENEW_CONTEXT Context
    );

PPH_TREENEW_COLUMN PhTnpHitTestHeader(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ BOOLEAN Fixed,
    _In_ PPOINT Point,
    _Out_opt_ PRECT ItemRect
    );

VOID PhTnpGetHeaderTooltipText(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ BOOLEAN Fixed,
    _In_ PPOINT Point,
    _Outptr_ PWSTR *Text
    );

LRESULT CALLBACK PhTnpHeaderHookWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// Drag selection

BOOLEAN PhTnpDetectDrag(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG CursorX,
    _In_ LONG CursorY,
    _In_ BOOLEAN DispatchMessages,
    _Out_opt_ PULONG CancelledByMessage
    );

VOID PhTnpDragSelect(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ LONG CursorX,
    _In_ LONG CursorY
    );

VOID PhTnpProcessDragSelect(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ ULONG VirtualKeys,
    _In_ PRECT OldRect,
    _In_ PRECT NewRect,
    _In_ PRECT TotalRect
    );

// Double buffering

VOID PhTnpCreateBufferedContext(
    _In_ PPH_TREENEW_CONTEXT Context
    );

VOID PhTnpDestroyBufferedContext(
    _In_ PPH_TREENEW_CONTEXT Context
    );

// Support functions

VOID PhTnpGetMessagePos(
    _In_ HWND hwnd,
    _Out_ PPOINT ClientPoint
    );

BOOLEAN PhTnpGetColumnHeaderText(
    _In_ PPH_TREENEW_CONTEXT Context,
    _In_ PPH_TREENEW_COLUMN Column,
    _Out_ PPH_STRINGREF Text
    );

// Macros

#define TNP_CELL_LEFT_MARGIN 6
#define TNP_CELL_RIGHT_MARGIN 6
#define TNP_ICON_RIGHT_PADDING 4

#define TNP_TIMER_NULL 1
#define TNP_TIMER_ANIMATE_DIVIDER 2

#define TNP_TOOLTIPS_ITEM 0
#define TNP_TOOLTIPS_FIXED_HEADER 1
#define TNP_TOOLTIPS_HEADER 2
#define TNP_TOOLTIPS_DEFAULT_MAXIMUM_WIDTH 550

#define TNP_ANIMATE_DIVIDER_INTERVAL 10
#define TNP_ANIMATE_DIVIDER_INCREMENT 17
#define TNP_ANIMATE_DIVIDER_DECREMENT 2

#define TNP_HIT_TEST_FIXED_DIVIDER(X, Context) \
    ((Context)->FixedDividerVisible && (X) >= (Context)->FixedWidth - 8 && (X) < (Context)->FixedWidth + 8)
#define TNP_HIT_TEST_PLUS_MINUS_GLYPH(X, NodeLevel) \
    (((X) >= TNP_CELL_LEFT_MARGIN + ((LONG)(NodeLevel) * SmallIconWidth)) && ((X) < TNP_CELL_LEFT_MARGIN + ((LONG)(NodeLevel) * SmallIconWidth) + SmallIconWidth))

#endif
