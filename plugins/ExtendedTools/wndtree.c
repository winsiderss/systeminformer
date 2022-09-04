/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2014
 *
 */

#include "exttools.h"
#include "wndtree.h"

BOOLEAN NTAPI WepWindowTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    );

BOOLEAN WepWindowNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PWCT_ROOT_NODE windowNode1 = *(PWCT_ROOT_NODE *)Entry1;
    PWCT_ROOT_NODE windowNode2 = *(PWCT_ROOT_NODE *)Entry2;

    return windowNode1->Node.Index == windowNode2->Node.Index;
}

ULONG WepWindowNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return (*(PWCT_ROOT_NODE*)Entry)->Node.Index;
}

VOID WepDestroyWindowNode(
    _In_ PWCT_ROOT_NODE WindowNode
    )
{
    PhDereferenceObject(WindowNode->Children);

    if (WindowNode->TimeoutString)
        PhDereferenceObject(WindowNode->TimeoutString);

    if (WindowNode->ProcessIdString)
        PhDereferenceObject(WindowNode->ProcessIdString);

    if (WindowNode->ThreadIdString)
        PhDereferenceObject(WindowNode->ThreadIdString);

    if (WindowNode->WaitTimeString)
        PhDereferenceObject(WindowNode->WaitTimeString);

    if (WindowNode->ContextSwitchesString)
        PhDereferenceObject(WindowNode->ContextSwitchesString);

    if (WindowNode->ObjectNameString)
        PhDereferenceObject(WindowNode->ObjectNameString);

    PhFree(WindowNode);
}

VOID WtcInitializeWindowTree(
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle,
    _Out_ PWCT_TREE_CONTEXT Context
    )
{
    HWND hwnd;
    PPH_STRING settings;

    memset(Context, 0, sizeof(WCT_TREE_CONTEXT));

    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PWCT_ROOT_NODE),
        WepWindowNodeHashtableEqualFunction,
        WepWindowNodeHashtableHashFunction,
        100
        );
    Context->NodeList = PhCreateList(100);
    Context->NodeRootList = PhCreateList(30);

    Context->ParentWindowHandle = ParentWindowHandle;
    Context->TreeNewHandle = TreeNewHandle;
    hwnd = TreeNewHandle;
    PhSetControlTheme(hwnd, L"explorer");

    TreeNew_SetCallback(hwnd, WepWindowTreeNewCallback, Context);

    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_TYPE, TRUE, L"Type", 80, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_THREADID, TRUE, L"ThreadId", 50, PH_ALIGN_LEFT, 1, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_PROCESSID, TRUE, L"ProcessId", 50, PH_ALIGN_LEFT, 2, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_STATUS, TRUE, L"Status", 80, PH_ALIGN_LEFT, 3, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_CONTEXTSWITCH, TRUE, L"Context Switches", 70, PH_ALIGN_LEFT, 4, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_WAITTIME, TRUE, L"WaitTime", 60, PH_ALIGN_LEFT, 5, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_TIMEOUT, TRUE, L"Timeout", 60, PH_ALIGN_LEFT, 6, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_ALERTABLE, TRUE, L"Alertable", 50, PH_ALIGN_LEFT, 7, 0);
    PhAddTreeNewColumn(hwnd, TREE_COLUMN_ITEM_NAME, TRUE, L"Name", 100, PH_ALIGN_LEFT, 8, 0);

    TreeNew_SetTriState(hwnd, TRUE);
    TreeNew_SetSort(hwnd, 0, NoSortOrder);

    settings = PhGetStringSetting(SETTING_NAME_WCT_TREE_LIST_COLUMNS);
    PhCmLoadSettings(hwnd, &settings->sr);
    PhDereferenceObject(settings);
}

VOID WtcDeleteWindowTree(
    _In_ PWCT_TREE_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(SETTING_NAME_WCT_TREE_LIST_COLUMNS, &settings->sr);
    PhDereferenceObject(settings);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        WepDestroyWindowNode(Context->NodeList->Items[i]);
    }

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
    PhDereferenceObject(Context->NodeRootList);
}

PWCT_ROOT_NODE WeAddWindowNode(
    _Inout_ PWCT_TREE_CONTEXT Context
    )
{
    PWCT_ROOT_NODE windowNode;

    windowNode = PhAllocate(sizeof(WCT_ROOT_NODE));
    memset(windowNode, 0, sizeof(WCT_ROOT_NODE));
    PhInitializeTreeNewNode(&windowNode->Node);

    memset(windowNode->TextCache, 0, sizeof(PH_STRINGREF) * TREE_COLUMN_ITEM_MAXIMUM);
    windowNode->Node.TextCache = windowNode->TextCache;
    windowNode->Node.TextCacheSize = TREE_COLUMN_ITEM_MAXIMUM;

    windowNode->Children = PhCreateList(1);

    PhAddEntryHashtable(Context->NodeHashtable, &windowNode);
    PhAddItemList(Context->NodeList, windowNode);

    //TreeNew_NodesStructured(Context->TreeNewHandle);

    return windowNode;
}

VOID WctAddChildWindowNode(
    _In_ PWCT_TREE_CONTEXT Context,
    _In_opt_ PWCT_ROOT_NODE ParentNode,
    _In_ PWAITCHAIN_NODE_INFO WctNode,
    _In_ BOOLEAN IsDeadLocked
    )
{
    PWCT_ROOT_NODE childNode = NULL;

    childNode = WeAddWindowNode(Context);

    childNode->IsDeadLocked = TRUE;
    childNode->ObjectType = WctNode->ObjectType;
    childNode->ObjectStatus = WctNode->ObjectStatus;
    childNode->Alertable = WctNode->LockObject.Alertable;
    childNode->ThreadId = UlongToHandle(WctNode->ThreadObject.ThreadId);
    childNode->ProcessIdString = PhFormatString(L"%lu", WctNode->ThreadObject.ProcessId);
    childNode->ThreadIdString = PhFormatString(L"%lu", WctNode->ThreadObject.ThreadId);
    childNode->WaitTimeString = PhFormatString(L"%lu", WctNode->ThreadObject.WaitTime);
    childNode->ContextSwitchesString = PhFormatString(L"%lu", WctNode->ThreadObject.ContextSwitches);

    if (WctNode->LockObject.ObjectName[0] != L'\0')
        childNode->ObjectNameString = PhFormatString(L"%s", WctNode->LockObject.ObjectName);

    if (WctNode->LockObject.Timeout.QuadPart > 0)
    {
        SYSTEMTIME systemTime;
        PPH_STRING dateString = NULL;
        PPH_STRING timeString = NULL;

        PhLargeIntegerToLocalSystemTime(&systemTime, &WctNode->LockObject.Timeout);

        dateString = PhFormatDate(&systemTime, NULL);
        timeString = PhFormatTime(&systemTime, NULL);

        childNode->TimeoutString = PhFormatString(L"%s %s", dateString->Buffer, timeString->Buffer);

        PhDereferenceObject(dateString);
        PhDereferenceObject(timeString);
    }

    if (ParentNode)
    {
        childNode->HasChildren = FALSE;

        // This is a child node.
        childNode->Parent = ParentNode;
        PhAddItemList(ParentNode->Children, childNode);
    }
    else
    {
        childNode->HasChildren = TRUE;
        childNode->Node.Expanded = TRUE;

        // This is a root node.
        PhAddItemList(Context->NodeRootList, childNode);
    }
}

PWCT_ROOT_NODE WeFindWindowNode(
    _In_ PWCT_TREE_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    WCT_ROOT_NODE lookupWindowNode;
    PWCT_ROOT_NODE lookupWindowNodePtr = &lookupWindowNode;
    PWCT_ROOT_NODE *windowNode;

    lookupWindowNode.Node.Index = HandleToUlong(WindowHandle);

    windowNode = (PWCT_ROOT_NODE*)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupWindowNodePtr
        );

    if (windowNode)
        return *windowNode;
    else
        return NULL;
}

VOID WeRemoveWindowNode(
    _In_ PWCT_TREE_CONTEXT Context,
    _In_ PWCT_ROOT_NODE WindowNode
    )
{
    ULONG index = 0;

    // Remove from hashtable/list and cleanup.
    PhRemoveEntryHashtable(Context->NodeHashtable, &WindowNode);

    if ((index = PhFindItemList(Context->NodeList, WindowNode)) != -1)
    {
        PhRemoveItemList(Context->NodeList, index);
    }

    WepDestroyWindowNode(WindowNode);

    TreeNew_NodesStructured(Context->TreeNewHandle);
}



BOOLEAN NTAPI WepWindowTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PWCT_TREE_CONTEXT context;
    PWCT_ROOT_NODE node;

    context = Context;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            node = (PWCT_ROOT_NODE)getChildren->Node;

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
                    getChildren->Children = (PPH_TREENEW_NODE *)context->NodeList->Items;
                    getChildren->NumberOfChildren = context->NodeList->Count;
                }
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = (PPH_TREENEW_IS_LEAF)Parameter1;

            node = (PWCT_ROOT_NODE)isLeaf->Node;

            if (context->TreeNewSortOrder == NoSortOrder)
                isLeaf->IsLeaf = !node->HasChildren;
            else
                isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;

            node = (PWCT_ROOT_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case TREE_COLUMN_ITEM_TYPE:
                {
                    switch (node->ObjectType)
                    {
                    case WctCriticalSectionType:
                        PhInitializeStringRef(&getCellText->Text, L"CriticalSection");
                        break;
                    case WctSendMessageType:
                        PhInitializeStringRef(&getCellText->Text, L"SendMessage");
                        break;
                    case WctMutexType:
                        PhInitializeStringRef(&getCellText->Text, L"Mutex");
                        break;
                    case WctAlpcType:
                        PhInitializeStringRef(&getCellText->Text, L"Alpc");
                        break;
                    case WctComType:
                        PhInitializeStringRef(&getCellText->Text, L"Com");
                        break;
                    case WctComActivationType:
                        PhInitializeStringRef(&getCellText->Text, L"ComActivation");
                        break;
                    case WctProcessWaitType:
                        PhInitializeStringRef(&getCellText->Text, L"ProcWait");
                        break;
                    case WctThreadType:
                        PhInitializeStringRef(&getCellText->Text, L"Thread");
                        break;
                    case WctThreadWaitType:
                        PhInitializeStringRef(&getCellText->Text, L"ThreadWait");
                        break;
                    case WctSocketIoType:
                        PhInitializeStringRef(&getCellText->Text, L"Socket I/O");
                        break;
                    case WctSmbIoType:
                        PhInitializeStringRef(&getCellText->Text, L"SMB I/O");
                        break;
                    case WctUnknownType:
                    case WctMaxType:
                    default:
                        PhInitializeStringRef(&getCellText->Text, L"Unknown");
                        break;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_STATUS:
                {
                    switch (node->ObjectStatus)
                    {
                    case WctStatusNoAccess:
                        PhInitializeStringRef(&getCellText->Text, L"No Access");
                        break;
                    case WctStatusRunning:
                        PhInitializeStringRef(&getCellText->Text, L"Running");
                        break;
                    case WctStatusBlocked:
                        PhInitializeStringRef(&getCellText->Text, L"Blocked");
                        break;
                    case WctStatusPidOnly:
                        PhInitializeStringRef(&getCellText->Text, L"Pid Only");
                        break;
                    case WctStatusPidOnlyRpcss:
                        PhInitializeStringRef(&getCellText->Text, L"Pid Only (Rpcss)");
                        break;
                    case WctStatusOwned:
                        PhInitializeStringRef(&getCellText->Text, L"Owned");
                        break;
                    case WctStatusNotOwned:
                        PhInitializeStringRef(&getCellText->Text, L"Not Owned");
                        break;
                    case WctStatusAbandoned:
                        PhInitializeStringRef(&getCellText->Text, L"Abandoned");
                        break;
                    case WctStatusError:
                        PhInitializeStringRef(&getCellText->Text, L"Error");
                        break;
                    case WctStatusUnknown:
                    case WctStatusMax:
                    default:
                        PhInitializeStringRef(&getCellText->Text, L"Unknown");
                        break;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_NAME:
                getCellText->Text = PhGetStringRef(node->ObjectNameString);
                break;
            case TREE_COLUMN_ITEM_TIMEOUT:
                getCellText->Text = PhGetStringRef(node->TimeoutString);
                break;
            case TREE_COLUMN_ITEM_ALERTABLE:
                {
                    if (node->Alertable)
                    {
                        PhInitializeStringRef(&getCellText->Text, L"true");
                    }
                    else
                    {
                        PhInitializeStringRef(&getCellText->Text, L"false");
                    }
                }
                break;
            case TREE_COLUMN_ITEM_PROCESSID:
                {
                    if (node->ObjectType == WctThreadType)
                    {
                        getCellText->Text = PhGetStringRef(node->ProcessIdString);
                    }
                }
                break;
            case TREE_COLUMN_ITEM_THREADID:
                {
                    if (node->ObjectType == WctThreadType)
                    {
                        getCellText->Text = PhGetStringRef(node->ThreadIdString);
                    }
                }
                break;
            case TREE_COLUMN_ITEM_WAITTIME:
                {
                    if (node->ObjectType == WctThreadType)
                    {
                        getCellText->Text = PhGetStringRef(node->WaitTimeString);
                    }
                }
                break;
            case TREE_COLUMN_ITEM_CONTEXTSWITCH:
                {
                    if (node->ObjectType == WctThreadType)
                    {
                        getCellText->Text = PhGetStringRef(node->ContextSwitchesString);
                    }
                }
                break;
            default:
                return FALSE;
            }

            getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = (PPH_TREENEW_GET_NODE_COLOR)Parameter1;
            node = (PWCT_ROOT_NODE)getNodeColor->Node;

            if (node->IsDeadLocked)
            {
                getNodeColor->ForeColor = RGB(255, 0, 0);
            }

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
    case TreeNewLeftDoubleClick:
    case TreeNewNodeExpanding:
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;

            SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_WCT_SHOWCONTEXTMENU, MAKELONG(contextMenuEvent->Location.x, contextMenuEvent->Location.y));
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = hwnd;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = 0;
            data.DefaultSortOrder = AscendingSortOrder;
            PhInitializeTreeNewColumnMenu(&data);

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
    _In_ PWCT_TREE_CONTEXT Context
    )
{
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
        WepDestroyWindowNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);
    PhClearList(Context->NodeRootList);
}

PWCT_ROOT_NODE WeGetSelectedWindowNode(
    _In_ PWCT_TREE_CONTEXT Context
    )
{
    PWCT_ROOT_NODE windowNode = NULL;
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
    _In_ PWCT_TREE_CONTEXT Context,
    _Out_ PWCT_ROOT_NODE **Windows,
    _Out_ PULONG NumberOfWindows
    )
{
    PPH_LIST list;
    ULONG i;

    list = PhCreateList(2);

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PWCT_ROOT_NODE node = (PWCT_ROOT_NODE)Context->NodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, node);
        }
    }

    *Windows = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
    *NumberOfWindows = list->Count;

    PhDereferenceObject(list);
}
