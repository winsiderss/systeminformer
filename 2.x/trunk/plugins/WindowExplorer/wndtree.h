#ifndef WNDTREE_H
#define WNDTREE_H

#define WEWNTLC_CLASS 0
#define WEWNTLC_HANDLE 1
#define WEWNTLC_TEXT 2
#define WEWNTLC_THREAD 3
#define WEWNTLC_MAXIMUM 4

typedef struct _WE_WINDOW_NODE
{
    PH_TREENEW_NODE Node;

    struct _WE_WINDOW_NODE *Parent;
    PPH_LIST Children;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG HasChildren : 1;
            ULONG Opened : 1;
            ULONG WindowVisible : 1;
            ULONG Spare : 29;
        };
    };

    PH_STRINGREF TextCache[WEWNTLC_MAXIMUM];

    HWND WindowHandle;
    WCHAR WindowClass[64];
    PPH_STRING WindowText;
    CLIENT_ID ClientId;

    WCHAR WindowHandleString[PH_PTR_STR_LEN_1];
    PPH_STRING ThreadString;
} WE_WINDOW_NODE, *PWE_WINDOW_NODE;

typedef struct _WE_WINDOW_TREE_CONTEXT
{
    HWND ParentWindowHandle;
    HWND TreeNewHandle;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;

    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;
    PPH_LIST NodeRootList;
} WE_WINDOW_TREE_CONTEXT, *PWE_WINDOW_TREE_CONTEXT;

VOID WeInitializeWindowTree(
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle,
    _Out_ PWE_WINDOW_TREE_CONTEXT Context
    );

VOID WeDeleteWindowTree(
    _In_ PWE_WINDOW_TREE_CONTEXT Context
    );

PWE_WINDOW_NODE WeAddWindowNode(
    _Inout_ PWE_WINDOW_TREE_CONTEXT Context
    );

PWE_WINDOW_NODE WeFindWindowNode(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ HWND WindowHandle
    );

VOID WeRemoveWindowNode(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ PWE_WINDOW_NODE WindowNode
    );

VOID WeClearWindowTree(
    _In_ PWE_WINDOW_TREE_CONTEXT Context
    );

PWE_WINDOW_NODE WeGetSelectedWindowNode(
    _In_ PWE_WINDOW_TREE_CONTEXT Context
    );

VOID WeGetSelectedWindowNodes(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _Out_ PWE_WINDOW_NODE **Windows,
    _Out_ PULONG NumberOfWindows
    );

#endif
