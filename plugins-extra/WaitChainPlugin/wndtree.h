/*
* Process Hacker Extra Plugins -
*   Wait Chain Traversal (WCT) Plugin
*
* Copyright (C) 2014 dmex
*
* This file is part of Process Hacker.
*
* Process Hacker is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Process Hacker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef WNDTREE_H
#define WNDTREE_H

#define ID_WCTSHOWCONTEXTMENU 19000
#define PLUGIN_NAME L"dmex.WaitChainPlugin"
#define SETTING_NAME_TREE_LIST_COLUMNS (PLUGIN_NAME L".TreeListColumns")
#define SETTING_NAME_WINDOW_POSITION (PLUGIN_NAME L".WindowPosition")
#define SETTING_NAME_WINDOW_SIZE (PLUGIN_NAME L".WindowSize")

typedef enum _WCT_TREE_COLUMN_ITEM_NAME
{
    TREE_COLUMN_ITEM_TYPE = 0,
    TREE_COLUMN_ITEM_STATUS = 1,
    TREE_COLUMN_ITEM_NAME = 2,
    TREE_COLUMN_ITEM_TIMEOUT = 3,
    TREE_COLUMN_ITEM_ALERTABLE = 4,
    TREE_COLUMN_ITEM_PROCESSID = 5,
    TREE_COLUMN_ITEM_THREADID = 6,
    TREE_COLUMN_ITEM_WAITTIME = 7,
    TREE_COLUMN_ITEM_CONTEXTSWITCH = 8,
    TREE_COLUMN_ITEM_MAXIMUM
} WCT_TREE_COLUMN_ITEM_NAME;

typedef struct _WCT_ROOT_NODE
{
    PH_TREENEW_NODE Node;
    struct _WCT_ROOT_NODE* Parent;
    PPH_LIST Children;
    BOOLEAN HasChildren;

    WAITCHAIN_NODE_INFO WctInfo;
    PPH_STRING TimeoutString;
    PPH_STRING ProcessIdString;
    PPH_STRING ThreadIdString;
    PPH_STRING WaitTimeString;
    PPH_STRING ContextSwitchesString;
    PPH_STRING ObjectNameString;

    BOOLEAN IsDeadLocked;

    HANDLE ProcessId;
    HANDLE ThreadId;

    PH_STRINGREF TextCache[TREE_COLUMN_ITEM_MAXIMUM];
    WCHAR WindowHandleString[PH_PTR_STR_LEN_1];
} WCT_ROOT_NODE, *PWCT_ROOT_NODE;

typedef struct _WCT_TREE_CONTEXT
{
    HWND ParentWindowHandle;
    HWND TreeNewHandle;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;

    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;
    PPH_LIST NodeRootList;
} WCT_TREE_CONTEXT, *PWCT_TREE_CONTEXT;

VOID WtcInitializeWindowTree(
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle,
    _Out_ PWCT_TREE_CONTEXT Context
    );

VOID WtcDeleteWindowTree(
    _In_ PWCT_TREE_CONTEXT Context
    );

VOID WctAddChildWindowNode(
    _In_ PWCT_TREE_CONTEXT Context,
    _In_opt_ PWCT_ROOT_NODE ParentNode,
    _In_ PWAITCHAIN_NODE_INFO WctNode,
    _In_ BOOLEAN IsDeadLocked
    );

PWCT_ROOT_NODE WeAddWindowNode(
    _Inout_ PWCT_TREE_CONTEXT Context
    );

PWCT_ROOT_NODE WeFindWindowNode(
    _In_ PWCT_TREE_CONTEXT Context,
    _In_ HWND WindowHandle
    );

VOID WeRemoveWindowNode(
    _In_ PWCT_TREE_CONTEXT Context,
    _In_ PWCT_ROOT_NODE WindowNode
    );

VOID WeClearWindowTree(
    _In_ PWCT_TREE_CONTEXT Context
    );

PWCT_ROOT_NODE WeGetSelectedWindowNode(
    _In_ PWCT_TREE_CONTEXT Context
    );

VOID WeGetSelectedWindowNodes(
    _In_ PWCT_TREE_CONTEXT Context,
    _Out_ PWCT_ROOT_NODE **Windows,
    _Out_ PULONG NumberOfWindows
    );

#endif
