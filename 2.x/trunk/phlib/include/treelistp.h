#ifndef _PH_TREELISTP_H
#define _PH_TREELISTP_H

#define PH_TREELIST_LISTVIEW_ID 4000

#define PH_TREELIST_USE_HACKAROUNDS (WindowsVersion < WINDOWS_VISTA)

typedef struct _PHP_TREELIST_CONTEXT
{
    ULONG RefCount;

    HWND Handle;
    HWND ListViewHandle;
    LONG LvRecursionGuard; // unused

    PPH_TREELIST_CALLBACK Callback;
    PVOID Context;

    PPH_TREELIST_COLUMN *Columns; // columns, indexed by ID
    ULONG MaxId;
    ULONG AllocatedColumns;
    ULONG NumberOfColumns; // just a statistic; do not use for actual logic

    // We have to deal with three kinds of indicies here:
    // * Display index
    // * Id
    // * Index (in the list view)

    PPH_TREELIST_COLUMN *ColumnsForViewX; // columns, indexed by display order
    ULONG AllocatedColumnsForViewX;
    PPH_TREELIST_COLUMN *ColumnsForDraw; // columns, indexed by index
    ULONG AllocatedColumnsForDraw;
    PPH_LIST List; // list of nodes for the list view, in actual display order
    BOOLEAN CanAnyExpand;

    // Sorting

    BOOLEAN TriState;
    ULONG SortColumn;
    PH_SORT_ORDER SortOrder;

    // List View WndProc Hooking

    WNDPROC OldLvWndProc;

    // Misc.

    LONG EnableRedraw;
    BOOLEAN NeedsRestructure;
    HCURSOR Cursor;
    BOOLEAN HasFocus;

    // Drawing

    BOOLEAN TextMetricsValid;
    TEXTMETRIC TextMetrics;
    RECT RowRect;
    HTHEME ThemeData;
    BOOLEAN ThemeActive;
    BOOLEAN EnableExplorerStyle;
    HBITMAP PlusBitmap;
    PH_INTEGER_PAIR PlusBitmapSize;
    HBITMAP MinusBitmap;
    PH_INTEGER_PAIR MinusBitmapSize;
    HDC IconDc;
} PHP_TREELIST_CONTEXT, *PPHP_TREELIST_CONTEXT;

BOOLEAN NTAPI PhpColumnHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    );

ULONG NTAPI PhpColumnHashtableHashFunction(
    __in PVOID Entry
    );

LRESULT CALLBACK PhpTreeListWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

LRESULT CALLBACK PhpTreeListLvHookWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhpCustomDrawPrePaintItem(
    __in PPHP_TREELIST_CONTEXT Context,
    __in LPNMLVCUSTOMDRAW CustomDraw
    );

VOID PhpCustomDrawPrePaintSubItem(
    __in PPHP_TREELIST_CONTEXT Context,
    __in LPNMLVCUSTOMDRAW CustomDraw
    );

BOOLEAN PhpReferenceTreeListNode(
    __in PPHP_TREELIST_CONTEXT Context,
    __in PPH_TREELIST_NODE Node
    );

BOOLEAN PhpDereferenceTreeListNode(
    __in PPHP_TREELIST_CONTEXT Context,
    __in PPH_TREELIST_NODE Node
    );

BOOLEAN PhpGetNodeChildren(
    __in PPHP_TREELIST_CONTEXT Context,
    __in_opt PPH_TREELIST_NODE Node,
    __out PPH_TREELIST_NODE **Children,
    __out PULONG NumberOfChildren
    );

BOOLEAN PhpIsNodeLeaf(
    __in PPHP_TREELIST_CONTEXT Context,
    __in PPH_TREELIST_NODE Node
    );

BOOLEAN PhpGetNodeText(
    __in PPHP_TREELIST_CONTEXT Context,
    __in PPH_TREELIST_NODE Node,
    __in ULONG Id,
    __out PPH_STRINGREF Text
    );

VOID PhpInsertNodeChildren(
    __in PPHP_TREELIST_CONTEXT Context,
    __in PPH_TREELIST_NODE Node,
    __in ULONG Level
    );

VOID PhpRestructureNodes(
    __in PPHP_TREELIST_CONTEXT Context
    );

INT PhpInsertColumn(
    __in PPHP_TREELIST_CONTEXT Context,
    __in PPH_TREELIST_COLUMN Column
    );

VOID PhpDeleteColumn(
    __in PPHP_TREELIST_CONTEXT Context,
    __inout PPH_TREELIST_COLUMN Column
    );

VOID PhpRefreshColumns(
    __in PPHP_TREELIST_CONTEXT Context
    );

VOID PhpRefreshColumnsLookup(
    __in PPHP_TREELIST_CONTEXT Context
    );

VOID PhpApplyNodeState(
    __in PPH_TREELIST_NODE Node,
    __in ULONG State
    );

VOID PhpClearBrushCache(
    __in PPHP_TREELIST_CONTEXT Context
    );

VOID PhpReloadThemeData(
    __in PPHP_TREELIST_CONTEXT Context
    );

FORCEINLINE VOID PhpFillTreeListMouseEvent(
    __out PPH_TREELIST_MOUSE_EVENT MouseEvent,
    __in LPNMITEMACTIVATE ItemActivate
    )
{
    MouseEvent->Index = ItemActivate->iItem;
    MouseEvent->Id = ItemActivate->iSubItem;
    MouseEvent->Location = ItemActivate->ptAction;
    MouseEvent->KeyFlags = ItemActivate->uKeyFlags;
}

#endif
