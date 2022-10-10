/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2013
 *     dmex    2016-2022
 *
 */

#ifndef WNDTREE_H
#define WNDTREE_H

#define WEWNTLC_CLASS 0
#define WEWNTLC_HANDLE 1
#define WEWNTLC_TEXT 2
#define WEWNTLC_THREAD 3
#define WEWNTLC_MODULE 4
#define WEWNTLC_MAXIMUM 5

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
            ULONG WindowVisible : 1;
            ULONG WindowMessageOnly : 1;
            ULONG Spare : 29;
        };
    };

    PH_STRINGREF TextCache[WEWNTLC_MAXIMUM];

    HWND WindowHandle;
    WCHAR WindowClass[64];
    PPH_STRING WindowText;
    CLIENT_ID ClientId;
    ULONG_PTR WindowIconIndex;

    WCHAR WindowHandleString[PH_PTR_STR_LEN_1];
    PPH_STRING ThreadString;
    PPH_STRING ModuleString;
    PPH_STRING FileNameWin32;
} WE_WINDOW_NODE, *PWE_WINDOW_NODE;

typedef struct _WE_WINDOW_TREE_CONTEXT
{
    HWND ParentWindowHandle;
    HWND TreeNewHandle;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;

    PPH_STRING SearchboxText;
    PH_TN_FILTER_SUPPORT FilterSupport;
    PPH_TN_FILTER_ENTRY TreeFilterEntry;

    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;
    PPH_LIST NodeRootList;
    HIMAGELIST NodeImageList;
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
    _Inout_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ HWND WindowHandle
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
