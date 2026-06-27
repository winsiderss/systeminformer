/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2013
 *     dmex    2016-2023
 *
 */

#ifndef WNDTREE_H
#define WNDTREE_H

typedef struct _WE_WINDOW_ITEM WE_WINDOW_ITEM, *PWE_WINDOW_ITEM;
typedef struct _WINDOWS_CONTEXT WINDOWS_CONTEXT, *PWINDOWS_CONTEXT;

typedef enum _WE_WINDOW_SELECTOR_TYPE
{
    WeWindowSelectorAll,
    WeWindowSelectorProcess,
    WeWindowSelectorThread,
    WeWindowSelectorDesktop
} WE_WINDOW_SELECTOR_TYPE;

typedef struct _WE_WINDOW_SELECTOR
{
    WE_WINDOW_SELECTOR_TYPE Type;
    union
    {
        struct
        {
            HANDLE ProcessId;
        } Process;
        struct
        {
            HANDLE ThreadId;
        } Thread;
        struct
        {
            PPH_STRING DesktopName;
        } Desktop;
    };
} WE_WINDOW_SELECTOR, *PWE_WINDOW_SELECTOR;

#define WEWNTLC_CLASS 0
#define WEWNTLC_HANDLE 1
#define WEWNTLC_TEXT 2
#define WEWNTLC_PROCESS 3
#define WEWNTLC_THREAD 4
#define WEWNTLC_MODULE 5
#define WEWNTLC_MAXIMUM 6

typedef struct _WE_WINDOW_NODE
{
    PH_TREENEW_NODE Node;

    PH_SH_STATE ShState;

    struct _WE_WINDOW_NODE *Parent;
    PPH_LIST Children;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG HasChildren : 1;
            ULONG ProcessIconValid : 1;
            ULONG Spare : 30;
        };
    };

    PWE_WINDOW_ITEM WindowItem;

    USHORT WindowIndex;
    USHORT WindowGeneration;

    HWND WindowHandle;

    PPH_PROCESS_ITEM ProcessItem;
    union
    {
        ULONG_PTR ProcessIconIndex;
        ULONG_PTR WindowIconIndex;
    };

    PPH_STRING ProcessString;
    PPH_STRING ThreadString;
    PPH_STRING ModuleString;
    PPH_STRING FileNameWin32;
    WCHAR WindowHandleString[PH_PTR_STR_LEN_1];
    PH_STRINGREF TextCache[WEWNTLC_MAXIMUM];
} WE_WINDOW_NODE, *PWE_WINDOW_NODE;

typedef enum _WE_WINDOW_SELECTOR_TYPE WE_WINDOW_SELECTOR_TYPE;

typedef enum _WE_WINDOW_VIEW_MODE
{
    WeWindowViewModeParentChild,
    WeWindowViewModeZOrder,
    WeWindowViewModeOwner
} WE_WINDOW_VIEW_MODE;

typedef struct _WE_WINDOW_TREE_CONTEXT
{
    HWND ParentWindowHandle;
    HWND TreeNewHandle;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;

    WE_WINDOW_SELECTOR_TYPE SelectorType;
    WE_WINDOW_VIEW_MODE ViewMode; // New field for viewing mode

    union
    {
        ULONG Flags;
        struct
        {
            ULONG EnableStateHighlighting : 1;

            ULONG EnableIcons : 1;
            ULONG EnableIconsInternal : 1;

            ULONG HighlightMessageOnly : 1;

            ULONG EnableWindowVisible : 1;
            ULONG WindowVisibleOnly : 1;

            ULONG EnableMessageOnly : 1;
            ULONG WindowMessageOnly : 1;

            ULONG Spare : 26;
        };
    };

    COLORREF MessageOnlyWindowColor;
    COLORREF ColorNew;
    COLORREF ColorRemoved;

    ULONG_PTR SearchMatchHandle;
    PH_TN_FILTER_SUPPORT FilterSupport;
    PPH_TN_FILTER_ENTRY TreeFilterEntry;

    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;
    PPH_LIST NodeRootList;
    PPH_POINTER_LIST NodeStateList;
    HIMAGELIST NodeImageList;
} WE_WINDOW_TREE_CONTEXT, *PWE_WINDOW_TREE_CONTEXT;

// Set by WeAddWindowNode/WeRemoveWindowNode when the tree topology changes; honored by
// WeTickWindowNodes and the WE_WM_WINDOWS_UPDATED handler to restructure the tree.
extern BOOLEAN NeedsNodesStructured;

VOID WeInitializeWindowTree(
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle,
    _Out_ PWE_WINDOW_TREE_CONTEXT Context
    );

VOID WeDeleteWindowTree(
    _In_ PWE_WINDOW_TREE_CONTEXT Context
    );

typedef struct _WE_WINDOW_SELECTOR WE_WINDOW_SELECTOR, *PWE_WINDOW_SELECTOR;

VOID WeInitializeWindowTreeImageList(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ PWE_WINDOW_SELECTOR Selector
    );

PPH_STRING WeGetClientIdName(
    _In_ PCLIENT_ID ClientId
    );

PWE_WINDOW_NODE WeAddWindowNode(
    _Inout_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ PWE_WINDOW_ITEM WindowItem,
    _In_ ULONG RunId
    );

PWE_WINDOW_NODE WeFindWindowNode(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ HWND WindowHandle
    );

VOID WeUpdateWindowNode(
    _In_ PWE_WINDOW_NODE WindowNode,
    _In_ PWE_WINDOW_TREE_CONTEXT Context
    );

VOID WeInvalidateWindowTreeColors(
    _In_ PWE_WINDOW_TREE_CONTEXT Context
    );

VOID WeSetWindowTreeIconsEnabled(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ BOOLEAN EnableIcons
    );

VOID WeRemoveWindowNode(
    _In_ PWE_WINDOW_NODE WindowNode,
    _In_ PWE_WINDOW_TREE_CONTEXT Context
    );

VOID WeTickWindowNodes(
    _In_ PWINDOWS_CONTEXT Context,
    _In_ PWE_WINDOW_TREE_CONTEXT TreeContext
    );

VOID WeClearWindowTree(
    _In_ PWE_WINDOW_TREE_CONTEXT Context
    );

PWE_WINDOW_NODE WeGetSelectedWindowNode(
    _In_ PWE_WINDOW_TREE_CONTEXT Context
    );

_Success_(return)
BOOLEAN WeGetSelectedWindowNodes(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _Out_ PWE_WINDOW_NODE** Nodes,
    _Out_ PULONG NumberOfNodes
    );

VOID WeExpandAllWindowNodes(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ BOOLEAN Expand
    );

VOID WeDeselectAllWindowNodes(
    _In_ PWE_WINDOW_TREE_CONTEXT Context
    );

VOID WeSelectAndEnsureVisibleWindowNodes(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ PWE_WINDOW_NODE* WindowNodes,
    _In_ ULONG NumberOfWindowNodes
    );

#endif
