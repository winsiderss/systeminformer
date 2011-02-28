#ifndef _PH_TREELIST_H
#define _PH_TREELIST_H

#define PH_TREELIST_CLASSNAME L"PhTreeList"

typedef struct _PH_TREELIST_COLUMN
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG Visible : 1;
            ULONG CustomDraw : 1;
            ULONG Spare : 30;
        };
    };
    ULONG Id; // becomes the subitem index for the column
    PWSTR Text;
    ULONG Width;
    ULONG Alignment;
    ULONG DisplayIndex;

    ULONG TextFlags;

    struct
    {
        ULONG ViewIndex; // actual index in header control
        ULONG ViewX;
    } s;
} PH_TREELIST_COLUMN, *PPH_TREELIST_COLUMN;

typedef struct _PH_TREELIST_NODE
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG Visible : 1;
            ULONG Selected : 1;
            ULONG Focused : 1;
            ULONG Expanded : 1;
            ULONG UseTempBackColor : 1;
            ULONG Spare : 27;
        };
    };

    COLORREF BackColor;
    COLORREF ForeColor;
    COLORREF TempBackColor;
    ULONG ColorFlags;
    HFONT Font;
    HICON Icon;

    PPH_STRINGREF TextCache;
    ULONG TextCacheSize;

    ULONG Level; // 0 for root, 1, 2, ...

    struct
    {
        ULONG ViewIndex; // actual index in list view, -1 for none
        ULONG ViewState; // LVIS_*

        BOOLEAN IsLeaf;

        ULONG StateTickCount;

        // Cache
        BOOLEAN CachedColorValid;
        BOOLEAN CachedFontValid;
        BOOLEAN CachedIconValid;

        // Temp. Drawing Data
        COLORREF DrawBackColor;
        COLORREF DrawForeColor;
    } s;
} PH_TREELIST_NODE, *PPH_TREELIST_NODE;

typedef enum _PH_TREELIST_MESSAGE
{
    TreeListGetChildren, // PPH_TREELIST_GET_CHILDREN Parameter1
    TreeListIsLeaf, // PPH_TREELIST_IS_LEAF Parameter1
    TreeListGetNodeText, // PPH_TREELIST_GET_NODE_TEXT Parameter1
    TreeListGetNodeColor, // PPH_TREELIST_GET_NODE_COLOR Parameter1
    TreeListGetNodeFont, // PPH_TREELIST_GET_NODE_FONT Parameter1
    TreeListGetNodeIcon, // PPH_TREELIST_GET_NODE_ICON Parameter1
    TreeListGetNodeTooltip, // PPH_TREELIST_GET_NODE_TOOLTIP Parameter1
    TreeListCustomDraw, // PPH_TREELIST_CUSTOM_DRAW Parameter1

    // Notifications
    TreeListSortChanged,
    TreeListSelectionChanged,

    TreeListKeyDown, // SHORT Parameter1 (Virtual key code)

    TreeListHeaderRightClick,

    TreeListLeftClick, // PPH_TREELIST_NODE Parameter1, PPH_TREELIST_MOUSE_EVENT Parameter2
    TreeListRightClick, // PPH_TREELIST_NODE Parameter1, PPH_TREELIST_MOUSE_EVENT Parameter2
    TreeListLeftDoubleClick, // PPH_TREELIST_NODE Parameter1, PPH_TREELIST_MOUSE_EVENT Parameter2
    TreeListRightDoubleClick, // PPH_TREELIST_NODE Parameter1, PPH_TREELIST_MOUSE_EVENT Parameter2

    TreeListContextMenu, // PPH_TREELIST_NODE Parameter1, PPH_TREELIST_MOUSE_EVENT Parameter2

    TreeListNodePlusMinusChanged, // PPH_TREELIST_NODE Parameter1, PPH_TREELIST_NODE_EVENT Parameter2

    MaxTreeListMessage
} PH_TREELIST_MESSAGE;

typedef BOOLEAN (NTAPI *PPH_TREELIST_CALLBACK)(
    __in HWND hwnd,
    __in PH_TREELIST_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in_opt PVOID Context
    );

#define TLC_CACHE 0x1

typedef struct _PH_TREELIST_GET_CHILDREN
{
    ULONG Flags;
    PPH_TREELIST_NODE Node;

    ULONG NumberOfChildren;
    PPH_TREELIST_NODE *Children; // can be NULL if no children
} PH_TREELIST_GET_CHILDREN, *PPH_TREELIST_GET_CHILDREN;

typedef struct _PH_TREELIST_IS_LEAF
{
    ULONG Flags;
    PPH_TREELIST_NODE Node;

    BOOLEAN IsLeaf;
} PH_TREELIST_IS_LEAF, *PPH_TREELIST_IS_LEAF;

typedef struct _PH_TREELIST_GET_NODE_TEXT
{
    ULONG Flags;
    PPH_TREELIST_NODE Node;
    ULONG Id;

    PH_STRINGREF Text;
} PH_TREELIST_GET_NODE_TEXT, *PPH_TREELIST_GET_NODE_TEXT;

#define TLGNC_AUTO_FORECOLOR 0x1000

typedef struct _PH_TREELIST_GET_NODE_COLOR
{
    ULONG Flags;
    PPH_TREELIST_NODE Node;

    COLORREF BackColor;
    COLORREF ForeColor;
} PH_TREELIST_GET_NODE_COLOR, *PPH_TREELIST_GET_NODE_COLOR;

typedef struct _PH_TREELIST_GET_NODE_FONT
{
    ULONG Flags;
    PPH_TREELIST_NODE Node;

    HFONT Font;
} PH_TREELIST_GET_NODE_FONT, *PPH_TREELIST_GET_NODE_FONT;

typedef struct _PH_TREELIST_GET_NODE_ICON
{
    ULONG Flags;
    PPH_TREELIST_NODE Node;

    HICON Icon;
} PH_TREELIST_GET_NODE_ICON, *PPH_TREELIST_GET_NODE_ICON;

typedef struct _PH_TREELIST_GET_NODE_TOOLTIP
{
    ULONG Flags;
    PPH_TREELIST_NODE Node;
    PWSTR ExistingText;

    PH_STRINGREF Text;
} PH_TREELIST_GET_NODE_TOOLTIP, *PPH_TREELIST_GET_NODE_TOOLTIP;

typedef struct _PH_TREELIST_CUSTOM_DRAW
{
    PPH_TREELIST_NODE Node;
    PPH_TREELIST_COLUMN Column;

    HDC Dc;
    RECT CellRect;
    RECT TextRect;
} PH_TREELIST_CUSTOM_DRAW, *PPH_TREELIST_CUSTOM_DRAW;

typedef struct _PH_TREELIST_MOUSE_EVENT
{
    ULONG Index;
    ULONG Id;
    POINT Location;
    ULONG KeyFlags;
} PH_TREELIST_MOUSE_EVENT, *PPH_TREELIST_MOUSE_EVENT;

typedef struct _PH_TREELIST_NODE_EVENT
{
    BOOLEAN Handled;
    ULONG Flags;
    PVOID Reserved1;
    PVOID Reserved2;
} PH_TREELIST_NODE_EVENT, *PPH_TREELIST_NODE_EVENT;

#define TLM_SETCALLBACK (WM_APP + 1201)
#define TLM_SETCONTEXT (WM_APP + 1202)
#define TLM_NODESADDED (WM_APP + 1203)
#define TLM_NODESREMOVED (WM_APP + 1204)
#define TLM_NODESSTRUCTURED (WM_APP + 1205)
#define TLM_ADDCOLUMN (WM_APP + 1206)
#define TLM_REMOVECOLUMN (WM_APP + 1207)
#define TLM_GETCOLUMN (WM_APP + 1208)
#define TLM_SETCOLUMN (WM_APP + 1209)
#define TLM_RESERVED1 (WM_APP + 1210)
#define TLM_UPDATENODE (WM_APP + 1211)
#define TLM_SETCURSOR (WM_APP + 1212)
#define TLM_SETREDRAW (WM_APP + 1213)
#define TLM_GETSORT (WM_APP + 1214)
#define TLM_SETSORT (WM_APP + 1215)
#define TLM_SETTRISTATE (WM_APP + 1216)
#define TLM_ENSUREVISIBLE (WM_APP + 1217)
#define TLM_SETSELECTIONMARK (WM_APP + 1218)
#define TLM_SETSTATEALL (WM_APP + 1219)
#define TLM_GETCOLUMNCOUNT (WM_APP + 1220)
#define TLM_SCROLL (WM_APP + 1221)
#define TLM_SETCOLUMNORDERARRAY (WM_APP + 1222)
#define TLM_GETNODETEXT (WM_APP + 1223)
#define TLM_GETVISIBLENODECOUNT (WM_APP + 1224)
#define TLM_GETVISIBLENODE (WM_APP + 1225)
#define TLM_GETLISTVIEW (WM_APP + 1226)
#define TLM_ENABLEEXPLORERSTYLE (WM_APP + 1227)
#define TLM_GETMAXID (WM_APP + 1228)
#define TLM_SETNODESTATE (WM_APP + 1229)
#define TLM_SETNODEEXPANDED (WM_APP + 1230)
#define TLM_SETMAXID (WM_APP + 1231)

typedef struct _PH_TL_GETNODETEXT
{
    PPH_TREELIST_NODE Node;
    ULONG Id;
    PH_STRINGREF Text;
} PH_TL_GETNODETEXT, *PPH_TL_GETNODETEXT;

#define TreeList_SetCallback(hWnd, Callback) \
    SendMessage((hWnd), TLM_SETCALLBACK, 0, (LPARAM)(Callback))

#define TreeList_SetContext(hWnd, Context) \
    SendMessage((hWnd), TLM_SETCONTEXT, 0, (LPARAM)(Context))

#define TreeList_NodesAdded(hWnd, Nodes, NumberOfNodes) \
    SendMessage((hWnd), TLM_NODESADDED, (WPARAM)(NumberOfNodes), (LPARAM)(Nodes))

#define TreeList_NodesRemoved(hWnd, Nodes, NumberOfNodes) \
    SendMessage((hWnd), TLM_NODESREMOVED, (WPARAM)(NumberOfNodes), (LPARAM)(Nodes))

#define TreeList_NodesStructured(hWnd) \
    SendMessage((hWnd), TLM_NODESSTRUCTURED, 0, 0)

#define TreeList_AddColumn(hWnd, Column) \
    SendMessage((hWnd), TLM_ADDCOLUMN, 0, (LPARAM)(Column))

#define TreeList_RemoveColumn(hWnd, Column) \
    SendMessage((hWnd), TLM_REMOVECOLUMN, 0, (LPARAM)(Column))

#define TLCM_FLAGS 0x1
#define TLCM_TEXT 0x2
#define TLCM_WIDTH 0x4
#define TLCM_ALIGNMENT 0x8
#define TLCM_DISPLAYINDEX 0x10

#define TreeList_GetColumn(hWnd, Column) \
    SendMessage((hWnd), TLM_GETCOLUMN, 0, (LPARAM)(Column))

#define TreeList_SetColumn(hWnd, Column, Mask) \
    SendMessage((hWnd), TLM_SETCOLUMN, (WPARAM)(Mask), (LPARAM)(Column))

#define TreeList_UpdateNode(hWnd, Node) \
    SendMessage((hWnd), TLM_UPDATENODE, 0, (LPARAM)(Node))

#define TreeList_SetCursor(hWnd, Cursor) \
    SendMessage((hWnd), TLM_SETCURSOR, 0, (LPARAM)(Cursor))

#define TreeList_SetRedraw(hWnd, Redraw) \
    SendMessage((hWnd), TLM_SETREDRAW, (WPARAM)(Redraw), 0)

#define TreeList_GetSort(hWnd, Column, Order) \
    SendMessage((hWnd), TLM_GETSORT, (WPARAM)(Column), (LPARAM)(Order))

#define TreeList_SetSort(hWnd, Column, Order) \
    SendMessage((hWnd), TLM_SETSORT, (WPARAM)(Column), (LPARAM)(Order))

#define TreeList_SetTriState(hWnd, TriState) \
    SendMessage((hWnd), TLM_SETTRISTATE, (WPARAM)(TriState), 0)

#define TreeList_EnsureVisible(hWnd, Node, PartialOk) \
    SendMessage((hWnd), TLM_ENSUREVISIBLE, (WPARAM)(PartialOk), (LPARAM)(Node))

#define TreeList_SetSelectionMark(hWnd, Index) \
    SendMessage((hWnd), TLM_SETSELECTIONMARK, (WPARAM)(Index), 0)

#define TreeList_SetStateAll(hWnd, State, Mask) \
    SendMessage((hWnd), TLM_SETSTATEALL, (WPARAM)(State), (LPARAM)(Mask))

#define TreeList_GetColumnCount(hWnd) \
    ((ULONG)SendMessage((hWnd), TLM_GETCOLUMNCOUNT, 0, 0))

#define TreeList_Scroll(hWnd, X, Y) \
    SendMessage((hWnd), TLM_SCROLL, (WPARAM)(X), (LPARAM)(Y))

#define TreeList_SetColumnOrderArray(hWnd, Count, Array) \
    SendMessage((hWnd), TLM_SETCOLUMNORDERARRAY, (WPARAM)(Count), (LPARAM)(Array))

#define TreeList_GetNodeText(hWnd, GetNodeText) \
    SendMessage((hWnd), TLM_GETNODETEXT, 0, (LPARAM)(GetNodeText))

#define TreeList_GetVisibleNodeCount(hWnd) \
    ((ULONG)SendMessage((hWnd), TLM_GETVISIBLENODECOUNT, 0, 0))

#define TreeList_GetVisibleNode(hWnd, Index) \
    ((PPH_TREELIST_NODE)SendMessage((hWnd), TLM_GETVISIBLENODE, (WPARAM)(Index), 0))

#define TreeList_GetListView(hWnd) \
    ((HWND)SendMessage((hWnd), TLM_GETLISTVIEW, 0, 0))

#define TreeList_EnableExplorerStyle(hWnd) \
    SendMessage((hWnd), TLM_ENABLEEXPLORERSTYLE, 0, 0)

#define TreeList_GetMaxId(hWnd) \
    ((ULONG)SendMessage((hWnd), TLM_GETMAXID, 0, 0))

#define TreeList_SetNodeState(hWnd, Node, State) \
    SendMessage((hWnd), TLM_SETNODESTATE, (WPARAM)(State), (LPARAM)(Node))

#define TreeList_SetNodeExpanded(hWnd, Node, Expanded) \
    SendMessage((hWnd), TLM_SETNODEEXPANDED, (WPARAM)(Expanded), (LPARAM)(Node))

#define TreeList_SetMaxId(hWnd, MaxId) \
    SendMessage((hWnd), TLM_SETMAXID, (WPARAM)(MaxId), 0)

BOOLEAN PhTreeListInitialization();

#define TLSTYLE_BORDER 0x1
#define TLSTYLE_CLIENTEDGE 0x2
#define TLSTYLE_ICONS 0x4

HWND PhCreateTreeListControl(
    __in HWND ParentHandle,
    __in INT_PTR Id
    );

HWND PhCreateTreeListControlEx(
    __in HWND ParentHandle,
    __in INT_PTR Id,
    __in ULONG Style
    );

__callback BOOLEAN NTAPI PhTreeListNullCallback(
    __in HWND hwnd,
    __in PH_TREELIST_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in_opt PVOID Context
    );

PHLIBAPI
VOID PhInitializeTreeListNode(
    __in PPH_TREELIST_NODE Node
    );

#define TLIN_RESERVED 0x1
#define TLIN_COLOR 0x2
#define TLIN_FONT 0x4
#define TLIN_ICON 0x8

PHLIBAPI
VOID PhInvalidateTreeListNode(
    __inout PPH_TREELIST_NODE Node,
    __in ULONG Flags
    );

PHLIBAPI
VOID PhInvalidateStateTreeListNode(
    __in HWND hwnd,
    __inout PPH_TREELIST_NODE Node
    );

PHLIBAPI
VOID PhExpandTreeListNode(
    __inout PPH_TREELIST_NODE Node,
    __in BOOLEAN Recursive
    );

PHLIBAPI
BOOLEAN PhAddTreeListColumn(
    __in HWND hwnd,
    __in ULONG Id,
    __in BOOLEAN Visible,
    __in PWSTR Text,
    __in ULONG Width,
    __in ULONG Alignment,
    __in ULONG DisplayIndex,
    __in ULONG TextFlags
    );

PHLIBAPI
BOOLEAN PhLoadTreeListColumnSettings(
    __in HWND TreeListHandle,
    __in PPH_STRING Settings
    );

PHLIBAPI
PPH_STRING PhSaveTreeListColumnSettings(
    __in HWND TreeListHandle
    );

#endif
