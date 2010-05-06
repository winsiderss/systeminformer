#ifndef TREELIST_H
#define TREELIST_H

#define PH_TREELIST_CLASSNAME L"PhTreeList"
//#define PH_TREELIST_ENABLE_EXPLORER_STYLE

typedef struct _PH_TREELIST_COLUMN
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG Visible : 1;
            ULONG Spare : 31;
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
            ULONG Spare : 28;
        };
    };

    PH_ITEM_STATE State;
    COLORREF BackColor;
    COLORREF ForeColor;
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
        COLORREF DrawForeColor;
    } s;
} PH_TREELIST_NODE, *PPH_TREELIST_NODE;

typedef enum _PH_TREELIST_MESSAGE
{
    TreeListReferenceNode, // PPH_TREELIST_NODE Parameter1
    TreeListDereferenceNode, // PPH_TREELIST_NODE Parameter1
    TreeListGetChildren, // PPH_TREELIST_GET_CHILDREN Parameter1
    TreeListIsLeaf, // PPH_TREELIST_IS_LEAF Parameter1
    TreeListGetNodeText, // PPH_TREELIST_GET_NODE_TEXT Parameter1
    TreeListGetNodeColor, // PPH_TREELIST_GET_NODE_COLOR Parameter1
    TreeListGetNodeFont, // PPH_TREELIST_GET_NODE_FONT Parameter1
    TreeListGetNodeIcon, // PPH_TREELIST_GET_NODE_ICON Parameter1
    TreeListGetNodeTooltip, // PPH_TREELIST_GET_NODE_TOOLTIP Parameter1

    // Notifications
    TreeListSortChanged,

    TreeListSelectionChanged,

    TreeListKeyDown, // SHORT Parameter1 (Virtual key code)

    TreeListHeaderRightClick,

    TreeListNodeLeftClick, // PPH_TREELIST_NODE Parameter1, PPH_TREELIST_MOUSE_EVENT Parameter2
    TreeListNodeRightClick, // PPH_TREELIST_NODE Parameter1, PPH_TREELIST_MOUSE_EVENT Parameter2
    TreeListNodeLeftDoubleClick, // PPH_TREELIST_NODE Parameter1, PPH_TREELIST_MOUSE_EVENT Parameter2
    TreeListNodeRightDoubleClick, // PPH_TREELIST_NODE Parameter1, PPH_TREELIST_MOUSE_EVENT Parameter2
} PH_TREELIST_MESSAGE;

typedef BOOLEAN (NTAPI *PPH_TREELIST_CALLBACK)(
    __in HWND hwnd,
    __in PH_TREELIST_MESSAGE Message,
    __in PVOID Parameter1,
    __in PVOID Parameter2,
    __in PVOID Context
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

typedef struct _PH_TREELIST_MOUSE_EVENT
{
    ULONG Index;
    ULONG Id;
    POINT Location;
    ULONG KeyFlags;
} PH_TREELIST_MOUSE_EVENT, *PPH_TREELIST_MOUSE_EVENT;

#define TLM_SETCALLBACK (WM_APP + 1201)
#define TLM_SETCONTEXT (WM_APP + 1202)
#define TLM_NODESADDED (WM_APP + 1203)
#define TLM_NODESREMOVED (WM_APP + 1204)
#define TLM_NODESSTRUCTURED (WM_APP + 1205)
#define TLM_ADDCOLUMN (WM_APP + 1206)
#define TLM_REMOVECOLUMN (WM_APP + 1207)
#define TLM_GETCOLUMN (WM_APP + 1208)
#define TLM_SETCOLUMN (WM_APP + 1209)
#define TLM_SETPLUSMINUS (WM_APP + 1210)
#define TLM_UPDATENODE (WM_APP + 1211)
#define TLM_SETCURSOR (WM_APP + 1212)
#define TLM_SETREDRAW (WM_APP + 1213)
#define TLM_SETNEWCOLOR (WM_APP + 1214)
#define TLM_SETREMOVINGCOLOR (WM_APP + 1215)
#define TLM_SETSTATEHIGHLIGHTING (WM_APP + 1216)
#define TLM_GETSORT (WM_APP + 1217)
#define TLM_SETSORT (WM_APP + 1218)
#define TLM_SETTRISTATE (WM_APP + 1219)
#define TLM_TICK (WM_APP + 1220)

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

#define TLCM_VISIBLE 0x1
#define TLCM_TEXT 0x2
#define TLCM_WIDTH 0x4
#define TLCM_ALIGNMENT 0x8
#define TLCM_DISPLAYINDEX 0x10

#define TreeList_GetColumn(hWnd, Column, Mask) \
    SendMessage((hWnd), TLM_GETCOLUMN, (WPARAM)(Mask), (LPARAM)(Column))

#define TreeList_SetColumn(hWnd, Column, Mask) \
    SendMessage((hWnd), TLM_SETCOLUMN, (WPARAM)(Mask), (LPARAM)(Column))

#define TreeList_SetPlusMinus(hWnd, Plus, Minus) \
    SendMessage((hWnd), TLM_SETPLUSMINUS, (WPARAM)(Plus), (LPARAM)(Minus))

#define TreeList_UpdateNode(hWnd, Node) \
    SendMessage((hWnd), TLM_UPDATENODE, 0, (LPARAM)(Node))

#define TreeList_SetCursor(hWnd, Cursor) \
    SendMessage((hWnd), TLM_SETCURSOR, 0, (LPARAM)(Cursor))

#define TreeList_SetRedraw(hWnd, Redraw) \
    SendMessage((hWnd), TLM_SETCURSOR, (WPARAM)(Redraw), 0)

#define TreeList_SetNewColor(hWnd, NewColor) \
    SendMessage((hWnd), TLM_SETNEWCOLOR, (WPARAM)(NewColor), 0)

#define TreeList_SetRemovingColor(hWnd, RemovingColor) \
    SendMessage((hWnd), TLM_SETREMOVINGCOLOR, (WPARAM)(RemovingColor), 0)

#define TreeList_SetStateHighlighting(hWnd, StateHighlighting) \
    SendMessage((hWnd), TLM_SETREMOVINGCOLOR, (WPARAM)(StateHighlighting), 0)

#define TreeList_GetSort(hWnd, Column, Order) \
    SendMessage((hWnd), TLM_GETSORT, (WPARAM)(Column), (LPARAM)(Order))

#define TreeList_SetSort(hWnd, Column, Order) \
    SendMessage((hWnd), TLM_SETSORT, (WPARAM)(Column), (LPARAM)(Order))

#define TreeList_SetTriState(hWnd, TriState) \
    SendMessage((hWnd), TLM_SETTRISTATE, (WPARAM)(TriState), 0)

#define TreeList_Tick(hWnd) \
    SendMessage((hWnd), TLM_TICK, 0, 0)

BOOLEAN PhTreeListInitialization();

HWND PhCreateTreeListControl(
    __in HWND ParentHandle,
    __in INT_PTR Id
    );

VOID PhInitializeTreeListNode(
    __in PPH_TREELIST_NODE Node
    );

#define TLIN_STATE 0x1
#define TLIN_COLOR 0x2
#define TLIN_FONT 0x4
#define TLIN_ICON 0x8

VOID PhInvalidateTreeListNode(
    __inout PPH_TREELIST_NODE Node,
    __in ULONG Flags
    );

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

__callback BOOLEAN NTAPI PhTreeListNullCallback(
    __in HWND hwnd,
    __in PH_TREELIST_MESSAGE Message,
    __in PVOID Parameter1,
    __in PVOID Parameter2,
    __in PVOID Context
    );

#endif
