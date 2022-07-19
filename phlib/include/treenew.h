#ifndef _PH_TREENEW_H
#define _PH_TREENEW_H

#ifdef __cplusplus
extern "C" {
#endif

#define PH_TREENEW_CLASSNAME L"PhTreeNew"

#define PH_TREENEW_SEARCH_TIMEOUT 1000
#define PH_TREENEW_SEARCH_MAXIMUM_LENGTH 1023

#define PH_TREENEW_FIXED_WIDTH_FONT 0x80000000

typedef struct _PH_TREENEW_CREATEPARAMS
{
    COLORREF TextColor;
    COLORREF FocusColor;
    COLORREF SelectionColor;
    // Add new fields here.
} PH_TREENEW_CREATEPARAMS, *PPH_TREENEW_CREATEPARAMS;

typedef struct _PH_TREENEW_COLUMN
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG Visible : 1;
            ULONG CustomDraw : 1;
            ULONG Fixed : 1; // Whether this is the fixed column
            ULONG SortDescending : 1; // Sort descending on initial click rather than ascending
            ULONG DpiScaleOnAdd : 1; // Whether to DPI scale the width (only when adding)
            ULONG SpareFlags : 27;
        };
    };
    ULONG Id;
    PVOID Context;
    PWSTR Text;
    LONG Width;
    ULONG Alignment;
    ULONG DisplayIndex; // -1 for fixed column or invalid

    ULONG TextFlags;

    struct
    {
        LONG ViewIndex; // Actual index in header control
        LONG ViewX; // 0 for the fixed column, and an offset from the divider for normal columns
    } s;
} PH_TREENEW_COLUMN, *PPH_TREENEW_COLUMN;

typedef struct _PH_TREENEW_NODE
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG Visible : 1;
            ULONG Selected : 1;
            ULONG Expanded : 1;
            ULONG UseAutoForeColor : 1;
            ULONG UseTempBackColor : 1;
            ULONG Unselectable : 1;
            ULONG SpareFlags : 26;
        };
    };

    COLORREF BackColor;
    COLORREF ForeColor;
    COLORREF TempBackColor;
    HFONT Font;
    HICON Icon;

    PPH_STRINGREF TextCache;
    ULONG TextCacheSize;

    ULONG Index; // Index within the flat list
    ULONG Level; // 0 for root, 1, 2, ...

    struct
    {
        union
        {
            ULONG Flags2;
            struct
            {
                ULONG IsLeaf : 1;
                ULONG CachedColorValid : 1;
                ULONG CachedFontValid : 1;
                ULONG CachedIconValid : 1;
                ULONG PlusMinusHot : 1;
                ULONG SpareFlags2 : 27;
            };
        };

        // Temp. drawing data
        COLORREF DrawBackColor;
        COLORREF DrawForeColor;
    } s;
} PH_TREENEW_NODE, *PPH_TREENEW_NODE;

// Styles
#define TN_STYLE_ICONS 0x1
#define TN_STYLE_DOUBLE_BUFFERED 0x2
#define TN_STYLE_NO_DIVIDER 0x4
#define TN_STYLE_ANIMATE_DIVIDER 0x8
#define TN_STYLE_NO_COLUMN_SORT 0x10
#define TN_STYLE_NO_COLUMN_REORDER 0x20
#define TN_STYLE_THIN_ROWS 0x40
#define TN_STYLE_NO_COLUMN_HEADER 0x80
#define TN_STYLE_CUSTOM_COLORS 0x100
#define TN_STYLE_ALWAYS_SHOW_SELECTION 0x200
#define TN_STYLE_CUSTOM_HEADERDRAW 0x400

// Extended flags
#define TN_FLAG_ITEM_DRAG_SELECT 0x1
#define TN_FLAG_NO_UNFOLDING_TOOLTIPS 0x2

// Callback flags
#define TN_CACHE 0x1
#define TN_AUTO_FORECOLOR 0x1000

// Column change flags
#define TN_COLUMN_CONTEXT 0x1
#define TN_COLUMN_TEXT 0x2
#define TN_COLUMN_WIDTH 0x4
#define TN_COLUMN_ALIGNMENT 0x8
#define TN_COLUMN_DISPLAYINDEX 0x10
#define TN_COLUMN_TEXTFLAGS 0x20
#define TN_COLUMN_FLAG_VISIBLE 0x100000
#define TN_COLUMN_FLAG_CUSTOMDRAW 0x200000
#define TN_COLUMN_FLAG_FIXED 0x400000
#define TN_COLUMN_FLAG_SORTDESCENDING 0x800000
#define TN_COLUMN_FLAG_NODPISCALEONADD 0x1000000
#define TN_COLUMN_FLAGS 0xfff00000

// Cache flags
#define TN_CACHE_COLOR 0x1
#define TN_CACHE_FONT 0x2
#define TN_CACHE_ICON 0x4

// Cell part input flags
#define TN_MEASURE_TEXT 0x1

// Cell part flags
#define TN_PART_CELL 0x1
#define TN_PART_PLUSMINUS 0x2
#define TN_PART_ICON 0x4
#define TN_PART_CONTENT 0x8
#define TN_PART_TEXT 0x10

// Hit test input flags
#define TN_TEST_COLUMN 0x1
#define TN_TEST_SUBITEM 0x2 // requires TN_TEST_COLUMN

// Hit test flags
#define TN_HIT_LEFT 0x1
#define TN_HIT_RIGHT 0x2
#define TN_HIT_ABOVE 0x4
#define TN_HIT_BELOW 0x8
#define TN_HIT_ITEM 0x10
#define TN_HIT_ITEM_PLUSMINUS 0x20 // requires TN_TEST_SUBITEM
#define TN_HIT_ITEM_ICON 0x40 // requires TN_TEST_SUBITEM
#define TN_HIT_ITEM_CONTENT 0x80 // requires TN_TEST_SUBITEM
#define TN_HIT_DIVIDER 0x100

// Selection flags
#define TN_SELECT_DESELECT 0x1
#define TN_SELECT_TOGGLE 0x2
#define TN_SELECT_RESET 0x4

// Auto-size flags
#define TN_AUTOSIZE_REMAINING_SPACE 0x1

typedef struct _PH_TREENEW_CELL_PARTS
{
    ULONG Flags;
    RECT RowRect;
    RECT CellRect; // TN_PART_CELL
    RECT PlusMinusRect; // TN_PART_PLUSMINUS
    RECT IconRect; // TN_PART_ICON
    RECT ContentRect; // TN_PART_CONTENT
    RECT TextRect; // TN_PART_TEXT
    PH_STRINGREF Text; // TN_PART_TEXT
    HFONT Font; // TN_PART_TEXT
} PH_TREENEW_CELL_PARTS, *PPH_TREENEW_CELL_PARTS;

typedef struct _PH_TREENEW_HIT_TEST
{
    POINT Point;
    ULONG InFlags;

    ULONG Flags;
    PPH_TREENEW_NODE Node;
    PPH_TREENEW_COLUMN Column; // requires TN_TEST_COLUMN
} PH_TREENEW_HIT_TEST, *PPH_TREENEW_HIT_TEST;

typedef enum _PH_TREENEW_MESSAGE
{
    TreeNewGetChildren, // PPH_TREENEW_GET_CHILDREN Parameter1
    TreeNewIsLeaf, // PPH_TREENEW_IS_LEAF Parameter1
    TreeNewGetCellText, // PPH_TREENEW_GET_CELL_TEXT Parameter1
    TreeNewGetNodeColor, // PPH_TREENEW_GET_NODE_COLOR Parameter1
    TreeNewGetNodeFont, // PPH_TREENEW_GET_NODE_FONT Parameter1
    TreeNewGetNodeIcon, // PPH_TREENEW_GET_NODE_ICON Parameter1
    TreeNewGetCellTooltip, // PPH_TREENEW_GET_CELL_TOOLTIP Parameter1
    TreeNewCustomDraw, // PPH_TREENEW_CUSTOM_DRAW Parameter1

    // Notifications
    TreeNewNodeExpanding, // PPH_TREENEW_NODE Parameter1, PPH_TREENEW_NODE_EVENT Parameter2
    TreeNewNodeSelecting, // PPH_TREENEW_NODE Parameter1

    TreeNewSortChanged,
    TreeNewSelectionChanged,

    TreeNewKeyDown, // PPH_TREENEW_KEY_EVENT Parameter1
    TreeNewLeftClick, // PPH_TREENEW_MOUSE_EVENT Parameter1
    TreeNewRightClick, // PPH_TREENEW_MOUSE_EVENT Parameter1
    TreeNewMiddleClick, // PPH_TREENEW_MOUSE_EVENT Parameter1
    TreeNewLeftDoubleClick, // PPH_TREENEW_MOUSE_EVENT Parameter1
    TreeNewRightDoubleClick, // PPH_TREENEW_MOUSE_EVENT Parameter1
    TreeNewContextMenu, // PPH_TREENEW_CONTEXT_MENU Parameter1

    TreeNewHeaderRightClick, // PPH_TREENEW_HEADER_MOUSE_EVENT Parameter1
    TreeNewIncrementalSearch, // PPH_TREENEW_SEARCH_EVENT Parameter1

    TreeNewColumnResized, // PPH_TREENEW_COLUMN Parameter1
    TreeNewColumnReordered,

    TreeNewDestroying,
    TreeNewGetDialogCode, // ULONG Parameter1, PULONG Parameter2

    TreeNewGetHeaderText,

    MaxTreeNewMessage
} PH_TREENEW_MESSAGE;

typedef BOOLEAN (NTAPI *PPH_TREENEW_CALLBACK)(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    );

typedef struct _PH_TREENEW_GET_CHILDREN
{
    ULONG Flags;
    PPH_TREENEW_NODE Node;

    ULONG NumberOfChildren;
    PPH_TREENEW_NODE *Children; // can be NULL if no children
} PH_TREENEW_GET_CHILDREN, *PPH_TREENEW_GET_CHILDREN;

typedef struct _PH_TREENEW_IS_LEAF
{
    ULONG Flags;
    PPH_TREENEW_NODE Node;

    BOOLEAN IsLeaf;
} PH_TREENEW_IS_LEAF, *PPH_TREENEW_IS_LEAF;

typedef struct _PH_TREENEW_GET_CELL_TEXT
{
    ULONG Flags;
    PPH_TREENEW_NODE Node;
    ULONG Id;

    PH_STRINGREF Text;
} PH_TREENEW_GET_CELL_TEXT, *PPH_TREENEW_GET_CELL_TEXT;

typedef struct _PH_TREENEW_GET_NODE_COLOR
{
    ULONG Flags;
    PPH_TREENEW_NODE Node;

    COLORREF BackColor;
    COLORREF ForeColor;
} PH_TREENEW_GET_NODE_COLOR, *PPH_TREENEW_GET_NODE_COLOR;

typedef struct _PH_TREENEW_GET_NODE_FONT
{
    ULONG Flags;
    PPH_TREENEW_NODE Node;

    HFONT Font;
} PH_TREENEW_GET_NODE_FONT, *PPH_TREENEW_GET_NODE_FONT;

typedef struct _PH_TREENEW_GET_NODE_ICON
{
    ULONG Flags;
    PPH_TREENEW_NODE Node;

    HICON Icon;
} PH_TREENEW_GET_NODE_ICON, *PPH_TREENEW_GET_NODE_ICON;

typedef struct _PH_TREENEW_GET_CELL_TOOLTIP
{
    ULONG Flags;
    PPH_TREENEW_NODE Node;
    PPH_TREENEW_COLUMN Column;

    BOOLEAN Unfolding;
    PH_STRINGREF Text;
    HFONT Font;
    ULONG MaximumWidth;
} PH_TREENEW_GET_CELL_TOOLTIP, *PPH_TREENEW_GET_CELL_TOOLTIP;

typedef struct _PH_TREENEW_CUSTOM_DRAW
{
    PPH_TREENEW_NODE Node;
    PPH_TREENEW_COLUMN Column;

    HDC Dc;
    RECT CellRect;
    RECT TextRect;
} PH_TREENEW_CUSTOM_DRAW, *PPH_TREENEW_CUSTOM_DRAW;

typedef struct _PH_TREENEW_MOUSE_EVENT
{
    POINT Location;
    PPH_TREENEW_NODE Node;
    PPH_TREENEW_COLUMN Column;
    ULONG KeyFlags;
} PH_TREENEW_MOUSE_EVENT, *PPH_TREENEW_MOUSE_EVENT;

typedef struct _PH_TREENEW_KEY_EVENT
{
    BOOLEAN Handled;
    ULONG VirtualKey;
    ULONG Data;
} PH_TREENEW_KEY_EVENT, *PPH_TREENEW_KEY_EVENT;

typedef struct _PH_TREENEW_NODE_EVENT
{
    BOOLEAN Handled;
    ULONG Flags;
    PVOID Reserved1;
    PVOID Reserved2;
} PH_TREENEW_NODE_EVENT, *PPH_TREENEW_NODE_EVENT;

typedef struct _PH_TREENEW_CONTEXT_MENU
{
    POINT Location;
    POINT ClientLocation;
    PPH_TREENEW_NODE Node;
    PPH_TREENEW_COLUMN Column;
    BOOLEAN KeyboardInvoked;
} PH_TREENEW_CONTEXT_MENU, *PPH_TREENEW_CONTEXT_MENU;

typedef struct _PH_TREENEW_HEADER_MOUSE_EVENT
{
    POINT ScreenLocation;
    POINT Location;
    POINT HeaderLocation;
    PPH_TREENEW_COLUMN Column;
} PH_TREENEW_HEADER_MOUSE_EVENT, *PPH_TREENEW_HEADER_MOUSE_EVENT;

typedef struct _PH_TREENEW_SEARCH_EVENT
{
    LONG FoundIndex;
    LONG StartIndex;
    PH_STRINGREF String;
} PH_TREENEW_SEARCH_EVENT, *PPH_TREENEW_SEARCH_EVENT;

typedef struct _PH_TREENEW_GET_HEADER_TEXT
{
    PPH_TREENEW_COLUMN Column;
    PH_STRINGREF Text;
    PWSTR TextCache;
    ULONG TextCacheSize;
} PH_TREENEW_GET_HEADER_TEXT, *PPH_TREENEW_GET_HEADER_TEXT;

#define TNM_FIRST (WM_USER + 1)
#define TNM_SETCALLBACK (WM_USER + 1)
#define TNM_NODESADDED (WM_USER + 2) // unimplemented
#define TNM_NODESREMOVED (WM_USER + 3) // unimplemented
#define TNM_NODESSTRUCTURED (WM_USER + 4)
#define TNM_ADDCOLUMN (WM_USER + 5)
#define TNM_REMOVECOLUMN (WM_USER + 6)
#define TNM_GETCOLUMN (WM_USER + 7)
#define TNM_SETCOLUMN (WM_USER + 8)
#define TNM_GETCOLUMNORDERARRAY (WM_USER + 9)
#define TNM_SETCOLUMNORDERARRAY (WM_USER + 10)
#define TNM_SETCURSOR (WM_USER + 11)
#define TNM_GETSORT (WM_USER + 12)
#define TNM_SETSORT (WM_USER + 13)
#define TNM_SETTRISTATE (WM_USER + 14)
#define TNM_ENSUREVISIBLE (WM_USER + 15)
#define TNM_SCROLL (WM_USER + 16)
#define TNM_GETFLATNODECOUNT (WM_USER + 17)
#define TNM_GETFLATNODE (WM_USER + 18)
#define TNM_GETCELLTEXT (WM_USER + 19)
#define TNM_SETNODEEXPANDED (WM_USER + 20)
#define TNM_GETMAXID (WM_USER + 21)
#define TNM_SETMAXID (WM_USER + 22)
#define TNM_INVALIDATENODE (WM_USER + 23)
#define TNM_INVALIDATENODES (WM_USER + 24)
#define TNM_GETFIXEDHEADER (WM_USER + 25)
#define TNM_GETHEADER (WM_USER + 26)
#define TNM_GETTOOLTIPS (WM_USER + 27)
#define TNM_SELECTRANGE (WM_USER + 28)
#define TNM_DESELECTRANGE (WM_USER + 29)
#define TNM_GETCOLUMNCOUNT (WM_USER + 30)
#define TNM_SETREDRAW (WM_USER + 31)
#define TNM_GETVIEWPARTS (WM_USER + 32)
#define TNM_GETFIXEDCOLUMN (WM_USER + 33)
#define TNM_GETFIRSTCOLUMN (WM_USER + 34)
#define TNM_SETFOCUSNODE (WM_USER + 35)
#define TNM_SETMARKNODE (WM_USER + 36)
#define TNM_SETHOTNODE (WM_USER + 37)
#define TNM_SETEXTENDEDFLAGS (WM_USER + 38)
#define TNM_GETCALLBACK (WM_USER + 39)
#define TNM_HITTEST (WM_USER + 40)
#define TNM_GETVISIBLECOLUMNCOUNT (WM_USER + 41)
#define TNM_AUTOSIZECOLUMN (WM_USER + 42)
#define TNM_SETEMPTYTEXT (WM_USER + 43)
#define TNM_SETROWHEIGHT (WM_USER + 44)
#define TNM_ISFLATNODEVALID (WM_USER + 45)
#define TNM_THEMESUPPORT (WM_USER + 46)
#define TNM_SETIMAGELIST (WM_USER + 47)
#define TNM_LAST (WM_USER + 48)

#define TreeNew_SetCallback(hWnd, Callback, Context) \
    SendMessage((hWnd), TNM_SETCALLBACK, (WPARAM)(Context), (LPARAM)(Callback))

#define TreeNew_NodesStructured(hWnd) \
    SendMessage((hWnd), TNM_NODESSTRUCTURED, 0, 0)

#define TreeNew_AddColumn(hWnd, Column) \
    SendMessage((hWnd), TNM_ADDCOLUMN, 0, (LPARAM)(Column))

#define TreeNew_RemoveColumn(hWnd, Id) \
    SendMessage((hWnd), TNM_REMOVECOLUMN, (WPARAM)(Id), 0)

#define TreeNew_GetColumn(hWnd, Id, Column) \
    SendMessage((hWnd), TNM_GETCOLUMN, (WPARAM)(Id), (LPARAM)(Column))

#define TreeNew_SetColumn(hWnd, Mask, Column) \
    SendMessage((hWnd), TNM_SETCOLUMN, (WPARAM)(Mask), (LPARAM)(Column))

#define TreeNew_GetColumnOrderArray(hWnd, Count, Array) \
    SendMessage((hWnd), TNM_GETCOLUMNORDERARRAY, (WPARAM)(Count), (LPARAM)(Array))

#define TreeNew_SetColumnOrderArray(hWnd, Count, Array) \
    SendMessage((hWnd), TNM_SETCOLUMNORDERARRAY, (WPARAM)(Count), (LPARAM)(Array))

#define TreeNew_SetCursor(hWnd, Cursor) \
    SendMessage((hWnd), TNM_SETCURSOR, 0, (LPARAM)(Cursor))

#define TreeNew_GetSort(hWnd, Column, Order) \
    SendMessage((hWnd), TNM_GETSORT, (WPARAM)(Column), (LPARAM)(Order))

#define TreeNew_SetSort(hWnd, Column, Order) \
    SendMessage((hWnd), TNM_SETSORT, (WPARAM)(Column), (LPARAM)(Order))

#define TreeNew_SetTriState(hWnd, TriState) \
    SendMessage((hWnd), TNM_SETTRISTATE, (WPARAM)(TriState), 0)

#define TreeNew_EnsureVisible(hWnd, Node) \
    SendMessage((hWnd), TNM_ENSUREVISIBLE, 0, (LPARAM)(Node))

#define TreeNew_Scroll(hWnd, DeltaRows, DeltaX) \
    SendMessage((hWnd), TNM_SCROLL, (WPARAM)(DeltaRows), (LPARAM)(DeltaX))

#define TreeNew_GetFlatNodeCount(hWnd) \
    ((ULONG)SendMessage((hWnd), TNM_GETFLATNODECOUNT, 0, 0))

#define TreeNew_GetFlatNode(hWnd, Index) \
    ((PPH_TREENEW_NODE)SendMessage((hWnd), TNM_GETFLATNODE, (WPARAM)(Index), 0))

#define TreeNew_GetCellText(hWnd, GetCellText) \
    SendMessage((hWnd), TNM_GETCELLTEXT, 0, (LPARAM)(GetCellText))

#define TreeNew_SetNodeExpanded(hWnd, Node, Expanded) \
    SendMessage((hWnd), TNM_SETNODEEXPANDED, (WPARAM)(Expanded), (LPARAM)(Node))

#define TreeNew_GetMaxId(hWnd) \
    ((ULONG)SendMessage((hWnd), TNM_GETMAXID, 0, 0))

#define TreeNew_SetMaxId(hWnd, MaxId) \
    SendMessage((hWnd), TNM_SETMAXID, (WPARAM)(MaxId), 0)

#define TreeNew_InvalidateNode(hWnd, Node) \
    SendMessage((hWnd), TNM_INVALIDATENODE, 0, (LPARAM)(Node))

#define TreeNew_InvalidateNodes(hWnd, Start, End) \
    SendMessage((hWnd), TNM_INVALIDATENODES, (WPARAM)(Start), (LPARAM)(End))

#define TreeNew_GetFixedHeader(hWnd) \
    ((HWND)SendMessage((hWnd), TNM_GETFIXEDHEADER, 0, 0))

#define TreeNew_GetHeader(hWnd) \
    ((HWND)SendMessage((hWnd), TNM_GETHEADER, 0, 0))

#define TreeNew_GetTooltips(hWnd) \
    ((HWND)SendMessage((hWnd), TNM_GETTOOLTIPS, 0, 0))

#define TreeNew_SelectRange(hWnd, Start, End) \
    SendMessage((hWnd), TNM_SELECTRANGE, (WPARAM)(Start), (LPARAM)(End))

#define TreeNew_DeselectRange(hWnd, Start, End) \
    SendMessage((hWnd), TNM_DESELECTRANGE, (WPARAM)(Start), (LPARAM)(End))

#define TreeNew_GetColumnCount(hWnd) \
    ((ULONG)SendMessage((hWnd), TNM_GETCOLUMNCOUNT, 0, 0))

#define TreeNew_SetRedraw(hWnd, Redraw) \
    ((LONG)SendMessage((hWnd), TNM_SETREDRAW, (WPARAM)(Redraw), 0))

#define TreeNew_GetViewParts(hWnd, Parts) \
    SendMessage((hWnd), TNM_GETVIEWPARTS, 0, (LPARAM)(Parts))

#define TreeNew_GetFixedColumn(hWnd) \
    ((PPH_TREENEW_COLUMN)SendMessage((hWnd), TNM_GETFIXEDCOLUMN, 0, 0))

#define TreeNew_GetFirstColumn(hWnd) \
    ((PPH_TREENEW_COLUMN)SendMessage((hWnd), TNM_GETFIRSTCOLUMN, 0, 0))

#define TreeNew_SetFocusNode(hWnd, Node) \
    SendMessage((hWnd), TNM_SETFOCUSNODE, 0, (LPARAM)(Node))

#define TreeNew_SetMarkNode(hWnd, Node) \
    SendMessage((hWnd), TNM_SETMARKNODE, 0, (LPARAM)(Node))

#define TreeNew_SetHotNode(hWnd, Node) \
    SendMessage((hWnd), TNM_SETHOTNODE, 0, (LPARAM)(Node))

#define TreeNew_SetExtendedFlags(hWnd, Mask, Value) \
    SendMessage((hWnd), TNM_SETEXTENDEDFLAGS, (WPARAM)(Mask), (LPARAM)(Value))

#define TreeNew_GetCallback(hWnd, Callback, Context) \
    SendMessage((hWnd), TNM_GETCALLBACK, (WPARAM)(Context), (LPARAM)(Callback))

#define TreeNew_HitTest(hWnd, HitTest) \
    SendMessage((hWnd), TNM_HITTEST, 0, (LPARAM)(HitTest))

#define TreeNew_GetVisibleColumnCount(hWnd) \
    ((ULONG)SendMessage((hWnd), TNM_GETVISIBLECOLUMNCOUNT, 0, 0))

#define TreeNew_AutoSizeColumn(hWnd, Id, Flags) \
    SendMessage((hWnd), TNM_AUTOSIZECOLUMN, (WPARAM)(Id), (LPARAM)(Flags))

#define TreeNew_SetEmptyText(hWnd, Text, Flags) \
    SendMessage((hWnd), TNM_SETEMPTYTEXT, (WPARAM)(Flags), (LPARAM)(Text))

#define TreeNew_SetRowHeight(hWnd, RowHeight) \
    SendMessage((hWnd), TNM_SETROWHEIGHT, (WPARAM)(RowHeight), 0)

#define TreeNew_IsFlatNodeValid(hWnd) \
    ((BOOLEAN)SendMessage((hWnd), TNM_ISFLATNODEVALID, 0, 0))

#define TreeNew_ThemeSupport(hWnd, Enable) \
    SendMessage((hWnd), TNM_THEMESUPPORT, (WPARAM)(Enable), 0);

#define TreeNew_SetImageList(hWnd, ImageListHandle) \
    SendMessage((hWnd), TNM_SETIMAGELIST, (WPARAM)(ImageListHandle), 0);

typedef struct _PH_TREENEW_VIEW_PARTS
{
    RECT ClientRect;
    LONG HeaderHeight;
    LONG RowHeight;
    ULONG VScrollWidth;
    ULONG HScrollHeight;
    LONG VScrollPosition;
    LONG HScrollPosition;
    LONG FixedWidth;
    LONG NormalLeft;
    LONG NormalWidth;
} PH_TREENEW_VIEW_PARTS, *PPH_TREENEW_VIEW_PARTS;

PHLIBAPI
BOOLEAN PhTreeNewInitialization(
    VOID
    );

FORCEINLINE VOID PhInitializeTreeNewNode(
    _In_ PPH_TREENEW_NODE Node
    )
{
    memset(Node, 0, sizeof(PH_TREENEW_NODE));

    Node->Visible = TRUE;
    Node->Expanded = TRUE;
}

FORCEINLINE VOID PhInvalidateTreeNewNode(
    _Inout_ PPH_TREENEW_NODE Node,
    _In_ ULONG Flags
    )
{
    if (Flags & TN_CACHE_COLOR)
        Node->s.CachedColorValid = FALSE;
    if (Flags & TN_CACHE_FONT)
        Node->s.CachedFontValid = FALSE;
    if (Flags & TN_CACHE_ICON)
        Node->s.CachedIconValid = FALSE;
}

FORCEINLINE BOOLEAN PhAddTreeNewColumn(
    _In_ HWND hwnd,
    _In_ ULONG Id,
    _In_ BOOLEAN Visible,
    _In_ PWSTR Text,
    _In_ ULONG Width,
    _In_ ULONG Alignment,
    _In_ ULONG DisplayIndex,
    _In_ ULONG TextFlags
    )
{
    PH_TREENEW_COLUMN column;

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Id = Id;
    column.Visible = Visible;
    column.Text = Text;
    column.Width = Width;
    column.Alignment = Alignment;
    column.DisplayIndex = DisplayIndex;
    column.TextFlags = TextFlags;
    column.DpiScaleOnAdd = TRUE;

    if (DisplayIndex == -2)
        column.Fixed = TRUE;

    return !!TreeNew_AddColumn(hwnd, &column);
}

FORCEINLINE BOOLEAN PhAddTreeNewColumnEx(
    _In_ HWND hwnd,
    _In_ ULONG Id,
    _In_ BOOLEAN Visible,
    _In_ PWSTR Text,
    _In_ ULONG Width,
    _In_ ULONG Alignment,
    _In_ ULONG DisplayIndex,
    _In_ ULONG TextFlags,
    _In_ BOOLEAN SortDescending
    )
{
    PH_TREENEW_COLUMN column;

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Id = Id;
    column.Visible = Visible;
    column.Text = Text;
    column.Width = Width;
    column.Alignment = Alignment;
    column.DisplayIndex = DisplayIndex;
    column.TextFlags = TextFlags;
    column.DpiScaleOnAdd = TRUE;

    if (DisplayIndex == -2)
        column.Fixed = TRUE;
    if (SortDescending)
        column.SortDescending = TRUE;

    return !!TreeNew_AddColumn(hwnd, &column);
}

FORCEINLINE BOOLEAN PhAddTreeNewColumnEx2(
    _In_ HWND hwnd,
    _In_ ULONG Id,
    _In_ BOOLEAN Visible,
    _In_ PWSTR Text,
    _In_ ULONG Width,
    _In_ ULONG Alignment,
    _In_ ULONG DisplayIndex,
    _In_ ULONG TextFlags,
    _In_ ULONG ExtraFlags
    )
{
    PH_TREENEW_COLUMN column;

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Id = Id;
    column.Visible = Visible;
    column.Text = Text;
    column.Width = Width;
    column.Alignment = Alignment;
    column.DisplayIndex = DisplayIndex;
    column.TextFlags = TextFlags;

    if (DisplayIndex == -2)
        column.Fixed = TRUE;
    if (ExtraFlags & TN_COLUMN_FLAG_CUSTOMDRAW)
        column.CustomDraw = TRUE;
    if (ExtraFlags & TN_COLUMN_FLAG_SORTDESCENDING)
        column.SortDescending = TRUE;
    if (!(ExtraFlags & TN_COLUMN_FLAG_NODPISCALEONADD))
        column.DpiScaleOnAdd = TRUE;

    return !!TreeNew_AddColumn(hwnd, &column);
}

#ifdef __cplusplus
}
#endif

#endif
