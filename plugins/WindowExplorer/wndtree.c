/*
 * Process Hacker Window Explorer - 
 *   window treelist
 * 
 * Copyright (C) 2011 wj32
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

#include "wndexp.h"
#include <colmgr.h>
#include "resource.h"

BOOLEAN WepWindowNodeHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    );

ULONG WepWindowNodeHashtableHashFunction(
    __in PVOID Entry
    );

VOID WepDestroyWindowNode(
    __in PWE_WINDOW_NODE WindowNode
    );

BOOLEAN NTAPI WepWindowTreeNewCallback(
    __in HWND hwnd,
    __in PH_TREENEW_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in_opt PVOID Context
    );

VOID WeInitializeWindowTree(
    __in HWND ParentWindowHandle,
    __in HWND TreeNewHandle,
    __out PWE_WINDOW_TREE_CONTEXT Context
    )
{
    HWND hwnd;
    PPH_STRING settings;

    memset(Context, 0, sizeof(WE_WINDOW_TREE_CONTEXT));

    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PWE_WINDOW_NODE),
        WepWindowNodeHashtableCompareFunction,
        WepWindowNodeHashtableHashFunction,
        100
        );
    Context->NodeList = PhCreateList(100);
    Context->NodeRootList = PhCreateList(30);

    Context->ParentWindowHandle = ParentWindowHandle;
    Context->TreeNewHandle = TreeNewHandle;
    hwnd = TreeNewHandle;
    PhSetControlTheme(hwnd, L"explorer");
    PhSetWindowStyle(hwnd, WS_BORDER, WS_BORDER);

    TreeNew_SetCallback(hwnd, WepWindowTreeNewCallback, Context);

    PhAddTreeNewColumn(hwnd, WEWNTLC_CLASS, TRUE, L"Class", 180, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(hwnd, WEWNTLC_HANDLE, TRUE, L"Handle", 70, PH_ALIGN_LEFT, 1, 0);
    PhAddTreeNewColumn(hwnd, WEWNTLC_TEXT, TRUE, L"Text", 220, PH_ALIGN_LEFT, 2, 0);
    PhAddTreeNewColumn(hwnd, WEWNTLC_THREAD, TRUE, L"Thread", 150, PH_ALIGN_LEFT, 3, 0);

    TreeNew_SetTriState(hwnd, TRUE);
    TreeNew_SetSort(hwnd, 0, NoSortOrder);

    settings = PhGetStringSetting(SETTING_NAME_WINDOW_TREE_LIST_COLUMNS);
    PhCmLoadSettings(hwnd, &settings->sr);
    PhDereferenceObject(settings);
}

VOID WeDeleteWindowTree(
    __in PWE_WINDOW_TREE_CONTEXT Context
    )
{
    PPH_STRING settings;
    ULONG i;

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(SETTING_NAME_WINDOW_TREE_LIST_COLUMNS, &settings->sr);
    PhDereferenceObject(settings);

    for (i = 0; i < Context->NodeList->Count; i++)
        WepDestroyWindowNode(Context->NodeList->Items[i]);

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
    PhDereferenceObject(Context->NodeRootList);
}

BOOLEAN WepWindowNodeHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    PWE_WINDOW_NODE windowNode1 = *(PWE_WINDOW_NODE *)Entry1;
    PWE_WINDOW_NODE windowNode2 = *(PWE_WINDOW_NODE *)Entry2;

    return windowNode1->WindowHandle == windowNode2->WindowHandle;
}

ULONG WepWindowNodeHashtableHashFunction(
    __in PVOID Entry
    )
{
#ifdef _M_IX86
    return PhHashInt32((ULONG)(*(PWE_WINDOW_NODE *)Entry)->WindowHandle);
#else
    return PhHashInt64((ULONG64)(*(PWE_WINDOW_NODE *)Entry)->WindowHandle);
#endif
}

PWE_WINDOW_NODE WeAddWindowNode(
    __inout PWE_WINDOW_TREE_CONTEXT Context
    )
{
    PWE_WINDOW_NODE windowNode;

    windowNode = PhAllocate(sizeof(WE_WINDOW_NODE));
    memset(windowNode, 0, sizeof(WE_WINDOW_NODE));
    PhInitializeTreeNewNode(&windowNode->Node);

    memset(windowNode->TextCache, 0, sizeof(PH_STRINGREF) * WEWNTLC_MAXIMUM);
    windowNode->Node.TextCache = windowNode->TextCache;
    windowNode->Node.TextCacheSize = WEWNTLC_MAXIMUM;

    windowNode->Children = PhCreateList(1);

    PhAddEntryHashtable(Context->NodeHashtable, &windowNode);
    PhAddItemList(Context->NodeList, windowNode);

    TreeNew_NodesStructured(Context->TreeNewHandle);

    return windowNode;
}

PWE_WINDOW_NODE WeFindWindowNode(
    __in PWE_WINDOW_TREE_CONTEXT Context,
    __in HWND WindowHandle
    )
{
    WE_WINDOW_NODE lookupWindowNode;
    PWE_WINDOW_NODE lookupWindowNodePtr = &lookupWindowNode;
    PWE_WINDOW_NODE *windowNode;

    lookupWindowNode.WindowHandle = WindowHandle;

    windowNode = (PWE_WINDOW_NODE *)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupWindowNodePtr
        );

    if (windowNode)
        return *windowNode;
    else
        return NULL;
}

VOID WeRemoveWindowNode(
    __in PWE_WINDOW_TREE_CONTEXT Context,
    __in PWE_WINDOW_NODE WindowNode
    )
{
    ULONG index;

    // Remove from hashtable/list and cleanup.

    PhRemoveEntryHashtable(Context->NodeHashtable, &WindowNode);

    if ((index = PhFindItemList(Context->NodeList, WindowNode)) != -1)
        PhRemoveItemList(Context->NodeList, index);

    WepDestroyWindowNode(WindowNode);

    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID WepDestroyWindowNode(
    __in PWE_WINDOW_NODE WindowNode
    )
{
    PhDereferenceObject(WindowNode->Children);

    if (WindowNode->WindowText) PhDereferenceObject(WindowNode->WindowText);

    if (WindowNode->ThreadString) PhDereferenceObject(WindowNode->ThreadString);

    PhFree(WindowNode);
}

#define SORT_FUNCTION(Column) WepWindowTreeNewCompare##Column

#define BEGIN_SORT_FUNCTION(Column) static int __cdecl WepWindowTreeNewCompare##Column( \
    __in void *_context, \
    __in const void *_elem1, \
    __in const void *_elem2 \
    ) \
{ \
    PWE_WINDOW_NODE node1 = *(PWE_WINDOW_NODE *)_elem1; \
    PWE_WINDOW_NODE node2 = *(PWE_WINDOW_NODE *)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    return PhModifySort(sortResult, ((PWE_WINDOW_TREE_CONTEXT)_context)->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Class)
{
    sortResult = wcsicmp(node1->WindowClass, node2->WindowClass);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Handle)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->WindowHandle, (ULONG_PTR)node2->WindowHandle);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Text)
{
    sortResult = PhCompareString(node1->WindowText, node2->WindowText, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Thread)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->ClientId.UniqueProcess, (ULONG_PTR)node2->ClientId.UniqueProcess);

    if (sortResult == 0)
        sortResult = uintptrcmp((ULONG_PTR)node1->ClientId.UniqueThread, (ULONG_PTR)node2->ClientId.UniqueThread);
}
END_SORT_FUNCTION

BOOLEAN NTAPI WepWindowTreeNewCallback(
    __in HWND hwnd,
    __in PH_TREENEW_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in_opt PVOID Context
    )
{
    PWE_WINDOW_TREE_CONTEXT context;
    PWE_WINDOW_NODE node;

    context = Context;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            node = (PWE_WINDOW_NODE)getChildren->Node;

            if (context->TreeNewSortOrder == NoSortOrder)
            {
                if (!node)
                {
                    getChildren->Children = (PPH_TREENEW_NODE *)context->NodeRootList->Items;
                    getChildren->NumberOfChildren = context->NodeRootList->Count;
                }
                else
                {
                    getChildren->Children = (PPH_TREENEW_NODE *)node->Children->Items;
                    getChildren->NumberOfChildren = node->Children->Count;
                }
            }
            else
            {
                if (!node)
                {
                    static PVOID sortFunctions[] =
                    {
                        SORT_FUNCTION(Class),
                        SORT_FUNCTION(Handle),
                        SORT_FUNCTION(Text),
                        SORT_FUNCTION(Thread)
                    };
                    int (__cdecl *sortFunction)(void *, const void *, const void *);

                    if (context->TreeNewSortColumn < WEWNTLC_MAXIMUM)
                        sortFunction = sortFunctions[context->TreeNewSortColumn];
                    else
                        sortFunction = NULL;

                    if (sortFunction)
                    {
                        qsort_s(context->NodeList->Items, context->NodeList->Count, sizeof(PVOID), sortFunction, context);
                    }

                    getChildren->Children = (PPH_TREENEW_NODE *)context->NodeList->Items;
                    getChildren->NumberOfChildren = context->NodeList->Count;
                }
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;

            node = (PWE_WINDOW_NODE)isLeaf->Node;

            if (context->TreeNewSortOrder == NoSortOrder)
                isLeaf->IsLeaf = !node->HasChildren;
            else
                isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;

            node = (PWE_WINDOW_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case WEWNTLC_CLASS:
                PhInitializeStringRef(&getCellText->Text, node->WindowClass);
                break;
            case WEWNTLC_HANDLE:
                PhPrintPointer(node->WindowHandleString, node->WindowHandle);
                PhInitializeStringRef(&getCellText->Text, node->WindowHandleString);
                break;
            case WEWNTLC_TEXT:
                getCellText->Text = PhGetStringRef(node->WindowText);
                break;
            case WEWNTLC_THREAD:
                if (!node->ThreadString)
                    node->ThreadString = PhGetClientIdName(&node->ClientId);
                getCellText->Text = PhGetStringRef(node->ThreadString);
                break;
            default:
                return FALSE;
            }

            getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = Parameter1;

            node = (PWE_WINDOW_NODE)getNodeColor->Node;

            if (!node->WindowVisible)
                getNodeColor->ForeColor = RGB(0x55, 0x55, 0x55);

            getNodeColor->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            TreeNew_GetSort(hwnd, &context->TreeNewSortColumn, &context->TreeNewSortOrder);
            // Force a rebuild to sort the items.
            TreeNew_NodesStructured(hwnd);
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                    SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_WINDOW_COPY, 0);
                break;
            }
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_WINDOW_PROPERTIES, 0);
        }
        return TRUE;
    case TreeNewNodeExpanding:
        {
            SendMessage(context->ParentWindowHandle, WM_WE_PLUSMINUS, 0, (LPARAM)Parameter1);
        }
        return FALSE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_MOUSE_EVENT mouseEvent = Parameter1;

            SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_SHOWCONTEXTMENU, MAKELONG(mouseEvent->Location.x, mouseEvent->Location.y));
        }
        return TRUE;
    }

    return FALSE;
}

VOID WeClearWindowTree(
    __in PWE_WINDOW_TREE_CONTEXT Context
    )
{
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
        WepDestroyWindowNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);
    PhClearList(Context->NodeRootList);
}

PWE_WINDOW_NODE WeGetSelectedWindowNode(
    __in PWE_WINDOW_TREE_CONTEXT Context
    )
{
    PWE_WINDOW_NODE windowNode = NULL;
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        windowNode = Context->NodeList->Items[i];

        if (windowNode->Node.Selected)
            return windowNode;
    }

    return NULL;
}

VOID WeGetSelectedWindowNodes(
    __in PWE_WINDOW_TREE_CONTEXT Context,
    __out PWE_WINDOW_NODE **Windows,
    __out PULONG NumberOfWindows
    )
{
    PPH_LIST list;
    ULONG i;

    list = PhCreateList(2);

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PWE_WINDOW_NODE node = Context->NodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, node);
        }
    }

    *Windows = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
    *NumberOfWindows = list->Count;

    PhDereferenceObject(list);
}
