#ifndef TREELISTP_H
#define TREELISTP_H

#define PH_TREELIST_LISTVIEW_ID 4000

typedef struct _PHP_TREELIST_CONTEXT
{
    ULONG RefCount;

    HWND Handle;
    HWND ListViewHandle;

    PPH_TREELIST_CALLBACK Callback;
    PVOID Context;

    ULONG MaxId;
    PPH_TREELIST_COLUMN *Columns;
    ULONG NumberOfColumns;
    ULONG AllocatedColumns;
    PPH_LIST List; // list of nodes for the list view, in actual display order
    BOOLEAN CanAnyExpand;

    // Sorting

    BOOLEAN TriState;
    ULONG SortColumn;
    PH_SORT_ORDER SortOrder;

    // State Highlighting

    LONG EnableStateHighlighting;
    ULONG HighlightingDuration;
    COLORREF NewColor;
    COLORREF RemovingColor;

    // List View WndProc Hooking

    WNDPROC OldLvWndProc;

    // Misc.

    LONG EnableRedraw;
    HCURSOR Cursor;
    BOOLEAN HasFocus;

    // Drawing

    TEXTMETRIC TextMetrics;
    PPH_HASHTABLE BrushCache;
    HTHEME ThemeData;
    BOOLEAN ThemeActive;
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

VOID PhpTreeListTick(
    __in PPHP_TREELIST_CONTEXT Context
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
    __in PPH_TREELIST_NODE Node,
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

#endif
