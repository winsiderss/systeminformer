/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011
 *     dmex    2017-2023
 *
 */

#include "wndexp.h"

BOOLEAN WepWindowNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

ULONG WepWindowNodeHashtableHashFunction(
    _In_ PVOID Entry
    );

VOID WepDestroyWindowNode(
    _In_ PWE_WINDOW_NODE WindowNode
    );

BOOLEAN NTAPI WepWindowTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    );

BOOLEAN WeWindowTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_ PVOID Context
    )
{
    PWE_WINDOW_NODE windowNode = (PWE_WINDOW_NODE)Node;
    PWE_WINDOW_TREE_CONTEXT context = Context;

    if (!context->SearchMatchHandle)
        return TRUE;

    if (windowNode->WindowHandle)
    {
        if (PhSearchControlMatchPointer(context->SearchMatchHandle, windowNode->WindowHandle))
            return TRUE;
    }

    if (windowNode->WindowClass[0])
    {
        if (PhSearchControlMatchLongHintZ(context->SearchMatchHandle, windowNode->WindowClass))
            return TRUE;
    }

    if (windowNode->WindowHandleString[0])
    {
        if (PhSearchControlMatchLongHintZ(context->SearchMatchHandle, windowNode->WindowHandleString))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(windowNode->WindowText))
    {
        if (PhSearchControlMatch(context->SearchMatchHandle, &windowNode->WindowText->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(windowNode->ThreadString))
    {
        if (PhSearchControlMatch(context->SearchMatchHandle, &windowNode->ThreadString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(windowNode->ModuleString))
    {
        if (PhSearchControlMatch(context->SearchMatchHandle, &windowNode->ModuleString->sr))
            return TRUE;
    }

    return FALSE;
}

VOID WeInitializeWindowTree(
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle,
    _Out_ PWE_WINDOW_TREE_CONTEXT Context
    )
{
    PPH_STRING settings;

    memset(Context, 0, sizeof(WE_WINDOW_TREE_CONTEXT));

    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PWE_WINDOW_NODE),
        WepWindowNodeHashtableEqualFunction,
        WepWindowNodeHashtableHashFunction,
        100
        );
    Context->NodeList = PhCreateList(100);
    Context->NodeRootList = PhCreateList(30);

    Context->ParentWindowHandle = ParentWindowHandle;
    Context->TreeNewHandle = TreeNewHandle;
    PhSetControlTheme(TreeNewHandle, L"explorer");

    TreeNew_SetRedraw(TreeNewHandle, FALSE);
    TreeNew_SetCallback(TreeNewHandle, WepWindowTreeNewCallback, Context);

    PhAddTreeNewColumn(TreeNewHandle, WEWNTLC_CLASS, TRUE, L"Class", 180, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(TreeNewHandle, WEWNTLC_HANDLE, TRUE, L"Handle", 70, PH_ALIGN_LEFT, 1, 0);
    PhAddTreeNewColumn(TreeNewHandle, WEWNTLC_TEXT, TRUE, L"Text", 220, PH_ALIGN_LEFT, 2, 0);
    PhAddTreeNewColumn(TreeNewHandle, WEWNTLC_THREAD, TRUE, L"Thread", 150, PH_ALIGN_LEFT, 3, 0);
    PhAddTreeNewColumn(TreeNewHandle, WEWNTLC_MODULE, TRUE, L"Module", 150, PH_ALIGN_LEFT, 4, 0);

    PhInitializeTreeNewFilterSupport(&Context->FilterSupport, Context->TreeNewHandle, Context->NodeList);
    Context->TreeFilterEntry = PhAddTreeNewFilter(&Context->FilterSupport, WeWindowTreeFilterCallback, Context);

    TreeNew_SetTriState(TreeNewHandle, TRUE);
    TreeNew_SetRedraw(TreeNewHandle, TRUE);

    settings = PhGetStringSetting(SETTING_NAME_WINDOW_TREE_LIST_COLUMNS);
    PhCmLoadSettings(TreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);
}

VOID WeDeleteWindowTree(
    _In_ PWE_WINDOW_TREE_CONTEXT Context
    )
{
    PPH_STRING settings;
    ULONG i;

    PhRemoveTreeNewFilter(&Context->FilterSupport, Context->TreeFilterEntry);
    PhDeleteTreeNewFilterSupport(&Context->FilterSupport);

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(SETTING_NAME_WINDOW_TREE_LIST_COLUMNS, &settings->sr);
    PhDereferenceObject(settings);

    for (i = 0; i < Context->NodeList->Count; i++)
        WepDestroyWindowNode(Context->NodeList->Items[i]);

    if (Context->NodeImageList)
        PhImageListDestroy(Context->NodeImageList);

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
    PhDereferenceObject(Context->NodeRootList);
}

VOID WeInitializeWindowTreeImageList(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ PWE_WINDOW_SELECTOR Selector
    )
{
    Context->EnableIcons = !!PhGetIntegerSetting(SETTING_NAME_WINDOW_ENABLE_ICONS);
    Context->EnableIconsInternal = !!PhGetIntegerSetting(SETTING_NAME_WINDOW_ENABLE_ICONS_INTERNAL);
    Context->SelectorType = Selector->Type;

    if (Context->EnableIcons && Context->SelectorType == WeWindowSelectorAll)
    {
        if (Context->EnableIconsInternal)
        {
            HICON iconSmall;
            LONG dpiValue;

            dpiValue = PhGetWindowDpi(Context->ParentWindowHandle);
            Context->NodeImageList = PhImageListCreate(
                PhGetSystemMetrics(SM_CXSMICON, dpiValue),
                PhGetSystemMetrics(SM_CYSMICON, dpiValue),
                ILC_MASK | ILC_COLOR32,
                200,
                200
                );
            PhImageListSetBkColor(Context->NodeImageList, CLR_NONE);
            TreeNew_SetImageList(Context->TreeNewHandle, Context->NodeImageList);

            PhGetStockApplicationIcon(&iconSmall, NULL);
            PhImageListAddIcon(Context->NodeImageList, iconSmall);
        }
        else
        {
            TreeNew_SetImageList(Context->TreeNewHandle, PhGetProcessSmallImageList());
        }
    }
}

BOOLEAN WepWindowNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PWE_WINDOW_NODE windowNode1 = *(PWE_WINDOW_NODE *)Entry1;
    PWE_WINDOW_NODE windowNode2 = *(PWE_WINDOW_NODE *)Entry2;

    return windowNode1->WindowHandle == windowNode2->WindowHandle;
}

ULONG WepWindowNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashIntPtr((ULONG_PTR)(*(PWE_WINDOW_NODE *)Entry)->WindowHandle);
}

PWE_WINDOW_NODE WeAddWindowNode(
    _Inout_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    PWE_WINDOW_NODE windowNode;

    windowNode = PhAllocate(sizeof(WE_WINDOW_NODE));
    memset(windowNode, 0, sizeof(WE_WINDOW_NODE));
    PhInitializeTreeNewNode(&windowNode->Node);

    memset(windowNode->TextCache, 0, sizeof(PH_STRINGREF) * WEWNTLC_MAXIMUM);
    windowNode->Node.TextCache = windowNode->TextCache;
    windowNode->Node.TextCacheSize = WEWNTLC_MAXIMUM;
    windowNode->WindowHandle = WindowHandle;
    windowNode->Children = PhCreateList(1);

    if (Context->EnableIcons && Context->SelectorType == WeWindowSelectorAll)
    {
        if (Context->EnableIconsInternal)
        {
            HICON windowIcon;

            if (windowIcon = WepGetInternalWindowIcon(WindowHandle, ICON_SMALL))
            {
                windowNode->WindowIconIndex = PhImageListAddIcon(Context->NodeImageList, windowIcon);
                DestroyIcon(windowIcon);
            }
        }
        else
        {
            CLIENT_ID clientId;
            PPH_PROCESS_ITEM processItem;

            PhGetWindowClientId(WindowHandle, &clientId);

            if (clientId.UniqueProcess && (processItem = PhReferenceProcessItem(clientId.UniqueProcess)))
            {
                windowNode->ProcessItem = processItem;

                if (PhTestEvent(&processItem->Stage1Event))
                {
                    windowNode->WindowIconIndex = processItem->SmallIconIndex;
                    windowNode->ProcessIconValid = TRUE;
                }
            }
        }
    }

    PhAddEntryHashtable(Context->NodeHashtable, &windowNode);
    PhAddItemList(Context->NodeList, windowNode);

    //if (Context->FilterSupport.FilterList)
    //   windowNode->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->FilterSupport, &windowNode->Node);
    //
    //TreeNew_NodesStructured(Context->TreeNewHandle);

    return windowNode;
}

PWE_WINDOW_NODE WeFindWindowNode(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ HWND WindowHandle
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
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ PWE_WINDOW_NODE WindowNode
    )
{
    ULONG index;

    // Remove from hashtable/list and cleanup.

    PhRemoveEntryHashtable(Context->NodeHashtable, &WindowNode);

    if ((index = PhFindItemList(Context->NodeList, WindowNode)) != ULONG_MAX)
        PhRemoveItemList(Context->NodeList, index);

    WepDestroyWindowNode(WindowNode);

    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID WepDestroyWindowNode(
    _In_ PWE_WINDOW_NODE WindowNode
    )
{
    PhDereferenceObject(WindowNode->Children);

    if (WindowNode->WindowText) PhDereferenceObject(WindowNode->WindowText);
    if (WindowNode->ThreadString) PhDereferenceObject(WindowNode->ThreadString);
    if (WindowNode->ModuleString) PhDereferenceObject(WindowNode->ModuleString);
    if (WindowNode->FileNameWin32) PhDereferenceObject(WindowNode->FileNameWin32);

    if (WindowNode->ProcessItem) PhDereferenceObject(WindowNode->ProcessItem);

    PhFree(WindowNode);
}

#define SORT_FUNCTION(Column) WepWindowTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl WepWindowTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
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
    sortResult = PhCompareStringZ(node1->WindowClass, node2->WindowClass, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Handle)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->WindowHandle, (ULONG_PTR)node2->WindowHandle);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Text)
{
    sortResult = PhCompareStringWithNull(node1->WindowText, node2->WindowText, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Thread)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->ClientId.UniqueProcess, (ULONG_PTR)node2->ClientId.UniqueProcess);

    if (sortResult == 0)
        sortResult = uintptrcmp((ULONG_PTR)node1->ClientId.UniqueThread, (ULONG_PTR)node2->ClientId.UniqueThread);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Module)
{
    sortResult = PhCompareStringWithNull(node1->ModuleString, node2->ModuleString, TRUE);
}
END_SORT_FUNCTION

BOOLEAN NTAPI WepWindowTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PWE_WINDOW_TREE_CONTEXT context = Context;
    PWE_WINDOW_NODE node;

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
                        SORT_FUNCTION(Thread),
                        SORT_FUNCTION(Module)
                    };
                    int (__cdecl *sortFunction)(void *, const void *, const void *);

                    static_assert(RTL_NUMBER_OF(sortFunctions) == WEWNTLC_MAXIMUM, "SortFunctions must equal maximum.");

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
                PhInitializeStringRefLongHint(&getCellText->Text, node->WindowClass);
                break;
            case WEWNTLC_HANDLE:
                PhInitializeStringRefLongHint(&getCellText->Text, node->WindowHandleString);
                break;
            case WEWNTLC_TEXT:
                getCellText->Text = PhGetStringRef(node->WindowText);
                break;
            case WEWNTLC_THREAD:
                getCellText->Text = PhGetStringRef(node->ThreadString);
                break;
            case WEWNTLC_MODULE:
                getCellText->Text = PhGetStringRef(node->ModuleString);
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

            if (node->WindowMessageOnly)
            {
                getNodeColor->BackColor = PhGetIntegerSetting(L"ColorServiceProcesses");
            }

            getNodeColor->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeIcon:
        {
            PPH_TREENEW_GET_NODE_ICON getNodeIcon = Parameter1;

            if (!(context->EnableIcons || context->EnableIconsInternal))
                break;

            node = (PWE_WINDOW_NODE)getNodeIcon->Node;
            getNodeIcon->Icon = (HICON)node->WindowIconIndex;
            //getNodeIcon->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            PPH_TREENEW_SORT_CHANGED_EVENT sorting = Parameter1;

            context->TreeNewSortColumn = sorting->SortColumn;
            context->TreeNewSortOrder = sorting->SortOrder;

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
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;

            SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_SHOWCONTEXTMENU, (LPARAM)contextMenuEvent);
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = hwnd;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = WEWNTLC_CLASS;
            data.DefaultSortOrder = NoSortOrder;
            PhInitializeTreeNewColumnMenuEx(&data, PH_TN_COLUMN_MENU_SHOW_RESET_SORT);

            data.Selection = PhShowEMenu(data.Menu, hwnd, PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    }

    return FALSE;
}

VOID WeClearWindowTree(
    _In_ PWE_WINDOW_TREE_CONTEXT Context
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
    _In_ PWE_WINDOW_TREE_CONTEXT Context
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

BOOLEAN WeGetSelectedWindowNodes(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _Out_ PWE_WINDOW_NODE **Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PWE_WINDOW_NODE node = Context->NodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, node);
        }
    }

    if (list->Count)
    {
        *Nodes = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
        *NumberOfNodes = list->Count;

        PhDereferenceObject(list);
        return TRUE;
    }

    PhDereferenceObject(list);
    return FALSE;
}

VOID WeExpandAllWindowNodes(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ BOOLEAN Expand
    )
{
    ULONG i;
    BOOLEAN needsRestructure = FALSE;

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PWE_WINDOW_NODE node = Context->NodeList->Items[i];

        if (node->Children->Count != 0 && node->Node.Expanded != Expand)
        {
            node->Node.Expanded = Expand;
            needsRestructure = TRUE;
        }
    }

    if (needsRestructure)
        TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID WeDeselectAllWindowNodes(
    _In_ PWE_WINDOW_TREE_CONTEXT Context
    )
{
    TreeNew_DeselectRange(Context->TreeNewHandle, 0, -1);
}

VOID WeSelectAndEnsureVisibleWindowNodes(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ PWE_WINDOW_NODE* WindowNodes,
    _In_ ULONG NumberOfWindowNodes
    )
{
    ULONG i;
    PWE_WINDOW_NODE leader = NULL;
    PWE_WINDOW_NODE node;
    BOOLEAN needsRestructure = FALSE;

    WeDeselectAllWindowNodes(Context);

    for (i = 0; i < NumberOfWindowNodes; i++)
    {
        if (WindowNodes[i]->Node.Visible)
        {
            leader = WindowNodes[i];
            break;
        }
    }

    if (!leader)
        return;

    // Expand recursively upwards, and select the nodes.

    for (i = 0; i < NumberOfWindowNodes; i++)
    {
        if (!WindowNodes[i]->Node.Visible)
            continue;

        node = WindowNodes[i]->Parent;

        while (node)
        {
            if (!node->Node.Expanded)
                needsRestructure = TRUE;

            node->Node.Expanded = TRUE;
            node = node->Parent;
        }

        WindowNodes[i]->Node.Selected = TRUE;
    }

    if (needsRestructure)
        TreeNew_NodesStructured(Context->TreeNewHandle);

    TreeNew_FocusMarkSelectNode(Context->TreeNewHandle, &leader->Node);
}
