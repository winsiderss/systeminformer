/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2015-2021
 *
 */

#include "nettools.h"
#include "tracert.h"
#include <commonutil.h>

PPH_OBJECT_TYPE TracertTreeNodeItemType;

VOID NTAPI TracertTreeNodeItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PTRACERT_ROOT_NODE tracertNode = Object;

    if (tracertNode->TtlString)
        PhDereferenceObject(tracertNode->TtlString);
    if (tracertNode->HostnameString)
        PhDereferenceObject(tracertNode->HostnameString);
    if (tracertNode->IpAddressString)
        PhDereferenceObject(tracertNode->IpAddressString);
    if (tracertNode->RemoteCountryCode)
        PhDereferenceObject(tracertNode->RemoteCountryCode);
    if (tracertNode->RemoteCountryName)
        PhDereferenceObject(tracertNode->RemoteCountryName);

    for (ULONG i = 0; i < DEFAULT_MAXIMUM_PINGS; i++)
    {
        if (tracertNode->PingString[i])
            PhDereferenceObject(tracertNode->PingString[i]);
        if (tracertNode->PingMessage[i])
            PhDereferenceObject(tracertNode->PingMessage[i]);
    }
}

PTRACERT_ROOT_NODE TracertTreeCreateNode(
    VOID
    )
{
    static ULONG NextUniqueId = 1;
    PTRACERT_ROOT_NODE tracertNode;

    tracertNode = PhCreateObject(sizeof(TRACERT_ROOT_NODE), TracertTreeNodeItemType);
    memset(tracertNode, 0, sizeof(TRACERT_ROOT_NODE));

    PhInitializeTreeNewNode(&tracertNode->Node);

    tracertNode->UniqueId = NextUniqueId++; // used to stabilize sorting
    tracertNode->CountryIconIndex = INT_MAX;

    return tracertNode;
}

#define SORT_FUNCTION(Column) TracertTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl TracertTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PTRACERT_ROOT_NODE node1 = *(PTRACERT_ROOT_NODE*)_elem1; \
    PTRACERT_ROOT_NODE node2 = *(PTRACERT_ROOT_NODE*)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintcmp(node1->UniqueId, node2->UniqueId); \
    \
    return PhModifySort(sortResult, ((PNETWORK_TRACERT_CONTEXT)_context)->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Ttl)
{
    sortResult = uint64cmp(node1->TTL, node2->TTL);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Ping1)
{
    sortResult = singlecmp(node1->PingList[0], node2->PingList[0]);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Ping2)
{
    sortResult = singlecmp(node1->PingList[1], node2->PingList[1]);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Ping3)
{
    sortResult = singlecmp(node1->PingList[2], node2->PingList[2]);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Ping4)
{
    sortResult = singlecmp(node1->PingList[3], node2->PingList[3]);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(IpAddress)
{
    sortResult = PhCompareStringWithNull(node1->IpAddressString, node2->IpAddressString, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Hostname)
{
    sortResult = PhCompareStringWithNull(node1->HostnameString, node2->HostnameString, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Country)
{
    sortResult = PhCompareStringWithNull(node1->RemoteCountryName, node2->RemoteCountryName, TRUE);
}
END_SORT_FUNCTION

VOID TracertLoadSettingsTreeList(
    _Inout_ PNETWORK_TRACERT_CONTEXT Context
    )
{
    PPH_STRING settings;
    PH_INTEGER_PAIR sortSettings;

    settings = PhGetStringSetting(SETTING_NAME_TRACERT_TREE_LIST_COLUMNS);
    PhCmLoadSettings(Context->TreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);

    sortSettings = PhGetIntegerPairSetting(SETTING_NAME_TRACERT_TREE_LIST_SORT);
    TreeNew_SetSort(Context->TreeNewHandle, (ULONG)sortSettings.X, (PH_SORT_ORDER)sortSettings.Y);
}

VOID TracertSaveSettingsTreeList(
    _Inout_ PNETWORK_TRACERT_CONTEXT Context
    )
{
    PPH_STRING settings;
    PH_INTEGER_PAIR sortSettings;
    ULONG sortColumn;
    PH_SORT_ORDER sortOrder;

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(SETTING_NAME_TRACERT_TREE_LIST_COLUMNS, &settings->sr);
    PhDereferenceObject(settings);

    TreeNew_GetSort(Context->TreeNewHandle, &sortColumn, &sortOrder);
    sortSettings.X = sortColumn;
    sortSettings.Y = sortOrder;
    PhSetIntegerPairSetting(SETTING_NAME_TRACERT_TREE_LIST_SORT, sortSettings);
}

BOOLEAN TracertNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PTRACERT_ROOT_NODE node1 = *(PTRACERT_ROOT_NODE *)Entry1;
    PTRACERT_ROOT_NODE node2 = *(PTRACERT_ROOT_NODE *)Entry2;

    return node1->TTL == node2->TTL;
}

ULONG TracertNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashInt32((*(PTRACERT_ROOT_NODE*)Entry)->TTL);
}

VOID DestroyTracertNode(
    _In_ PTRACERT_ROOT_NODE Node
    )
{
    PhDereferenceObject(Node);
}

PTRACERT_ROOT_NODE AddTracertNode(
    _Inout_ PNETWORK_TRACERT_CONTEXT Context,
    _In_ ULONG TTL
    )
{
    PTRACERT_ROOT_NODE tracertNode;

    tracertNode = TracertTreeCreateNode();

    tracertNode->TTL = TTL;
    memset(tracertNode->PingStatus, STATUS_FAIL_CHECK, sizeof(tracertNode->PingStatus));

    memset(tracertNode->TextCache, 0, sizeof(PH_STRINGREF) * TREE_COLUMN_ITEM_MAXIMUM);
    tracertNode->Node.TextCache = tracertNode->TextCache;
    tracertNode->Node.TextCacheSize = TREE_COLUMN_ITEM_MAXIMUM;

    PhAddEntryHashtable(Context->NodeHashtable, &tracertNode);
    PhAddItemList(Context->NodeList, tracertNode);

    TreeNew_NodesStructured(Context->TreeNewHandle);

    return tracertNode;
}

PTRACERT_ROOT_NODE FindTracertNode(
    _In_ PNETWORK_TRACERT_CONTEXT Context,
    _In_ ULONG TTL
    )
{
    TRACERT_ROOT_NODE lookupTracertNode;
    PTRACERT_ROOT_NODE lookupTracertNodePtr = &lookupTracertNode;
    PTRACERT_ROOT_NODE *tracertNode;

    lookupTracertNode.TTL = TTL;

    tracertNode = (PTRACERT_ROOT_NODE*)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupTracertNodePtr
        );

    if (tracertNode)
        return *tracertNode;
    else
        return NULL;
}

VOID RemoveTracertNode(
    _In_ PNETWORK_TRACERT_CONTEXT Context,
    _In_ PTRACERT_ROOT_NODE Node
    )
{
    ULONG index = 0;

    PhRemoveEntryHashtable(Context->NodeHashtable, &Node);

    if ((index = PhFindItemList(Context->NodeList, Node)) != ULONG_MAX)
    {
        PhRemoveItemList(Context->NodeList, index);
    }

    DestroyTracertNode(Node);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID UpdateTracertNode(
    _In_ PNETWORK_TRACERT_CONTEXT Context,
    _In_ PTRACERT_ROOT_NODE Node
    )
{
    memset(Node->TextCache, 0, sizeof(PH_STRINGREF) * TREE_COLUMN_ITEM_MAXIMUM);

    PhInvalidateTreeNewNode(&Node->Node, TN_CACHE_COLOR);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID UpdateTracertNodePingText(
    _In_ PTRACERT_ROOT_NODE Node, 
    _In_ PPH_TREENEW_GET_CELL_TEXT CellText,
    _In_ ULONG Index
    )
{
    if (Node->PingStatus[Index] == IP_SUCCESS ||
        Node->PingStatus[Index] == IP_TTL_EXPIRED_TRANSIT) // IP_HOP_LIMIT_EXCEEDED
    {
        if (Node->PingList[Index] != 0.0f)
        {
            PH_FORMAT format[2];

            // %.2f ms
            PhInitFormatF(&format[0], Node->PingList[Index], 2);
            PhInitFormatS(&format[1], L" ms");

            PhMoveReference(&Node->PingString[Index], PhFormat(format, RTL_NUMBER_OF(format), 0));
            CellText->Text = PhGetStringRef(Node->PingString[Index]);
        }
        else
        {
            PhInitializeStringRef(&CellText->Text, L"<1 ms");
        }
    }
    else if (Node->PingStatus[Index] == IP_REQ_TIMED_OUT)
    {
        PhInitializeStringRef(&CellText->Text, L"*");
    }
    else if (Node->PingStatus[Index] == IP_DEST_NO_ROUTE)
    {
        PhInitializeStringRef(&CellText->Text, L"The destination address route is unreachable.");
    }
    else
    {
        Node->PingMessage[Index] = TracertGetErrorMessage(Node->PingStatus[Index]);
        CellText->Text = PhGetStringRef(Node->PingMessage[Index]);
    }
}

BOOLEAN NTAPI TracertTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PNETWORK_TRACERT_CONTEXT context = Context;
    PTRACERT_ROOT_NODE node;

    if (!context)
        return FALSE;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            if (!getChildren)
                break;

            node = (PTRACERT_ROOT_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Ttl),
                    SORT_FUNCTION(Ping1),
                    SORT_FUNCTION(Ping2),
                    SORT_FUNCTION(Ping3),
                    SORT_FUNCTION(Ping4),
                    SORT_FUNCTION(IpAddress),
                    SORT_FUNCTION(Hostname),
                    SORT_FUNCTION(Country),
                };
                int (__cdecl *sortFunction)(void *, const void *, const void *);

                if (context->TreeNewSortColumn < TREE_COLUMN_ITEM_MAXIMUM)
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
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = (PPH_TREENEW_IS_LEAF)Parameter1;

            if (!isLeaf)
                break;

            node = (PTRACERT_ROOT_NODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;

            if (!getCellText)
                break;

            node = (PTRACERT_ROOT_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case TREE_COLUMN_ITEM_TTL:
                {
                    PhMoveReference(&node->TtlString, PhFormatUInt64(node->TTL, TRUE));
                    getCellText->Text = PhGetStringRef(node->TtlString);
                }
                break;
            case TREE_COLUMN_ITEM_PING1:
                UpdateTracertNodePingText(node, getCellText, 0);
                break;
            case TREE_COLUMN_ITEM_PING2:
                UpdateTracertNodePingText(node, getCellText, 1);
                break;
            case TREE_COLUMN_ITEM_PING3:
                UpdateTracertNodePingText(node, getCellText, 2);
                break;
            case TREE_COLUMN_ITEM_PING4:
                UpdateTracertNodePingText(node, getCellText, 3);
                break;
            case TREE_COLUMN_ITEM_IPADDR:
                getCellText->Text = PhGetStringRef(node->IpAddressString);
                break;
            case TREE_COLUMN_ITEM_HOSTNAME:
                getCellText->Text = PhGetStringRef(node->HostnameString);
                break;
            case TREE_COLUMN_ITEM_COUNTRY:
                getCellText->Text = PhGetStringRef(node->RemoteCountryName);
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

            if (!getNodeColor)
                break;

            node = (PTRACERT_ROOT_NODE)getNodeColor->Node;

            getNodeColor->Flags = TN_CACHE | TN_AUTO_FORECOLOR;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            TreeNew_GetSort(hwnd, &context->TreeNewSortColumn, &context->TreeNewSortOrder);
            // Force a rebuild to sort the items.
            TreeNew_NodesStructured(hwnd);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;

            if (!contextMenuEvent)
                break;

            SendMessage(
                context->WindowHandle,
                WM_COMMAND, 
                TRACERT_SHOWCONTEXTMENU, 
                (LPARAM)contextMenuEvent
                );
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

            data.Selection = PhShowEMenu(
                data.Menu,
                hwnd,
                PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP,
                data.MouseEvent->ScreenLocation.x,
                data.MouseEvent->ScreenLocation.y
                );

            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    case TreeNewCustomDraw:
        {
            PPH_TREENEW_CUSTOM_DRAW customDraw = Parameter1;  
            HDC hdc;
            RECT rect;

            if (!customDraw)
                break;

            hdc = customDraw->Dc;
            rect = customDraw->CellRect;
            node = (PTRACERT_ROOT_NODE)customDraw->Node;

            // Check if this is the country column
            if (customDraw->Column->Id != TREE_COLUMN_ITEM_COUNTRY)
                break;

            // Check if there's something to draw
            if (rect.right - rect.left <= 1)
            {
                // nothing to draw
                break;
            }

            // Padding
            rect.left += 5;

            if (node->RemoteCountryCode && node->RemoteCountryName)
            {
                if (node->CountryIconIndex == INT_MAX)
                    node->CountryIconIndex = LookupCountryIcon(node->RemoteCountryCode);

                if (node->CountryIconIndex != INT_MAX)
                {
                    DrawCountryIcon(hdc, rect, node->CountryIconIndex);
                    rect.left += 16 + 2;
                }

                DrawText(
                    hdc,
                    node->RemoteCountryName->Buffer,
                    (INT)node->RemoteCountryName->Length / sizeof(WCHAR),
                    &rect,
                    DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS | DT_SINGLELINE
                    );
            }
            else if (!GeoDbInitialized)
            {
                DrawText(hdc, L"Geoip database not found.", -1, &rect, DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS | DT_SINGLELINE);
            }
        }
        return TRUE;
    }

    return FALSE;
}

VOID ClearTracertTree(
    _In_ PNETWORK_TRACERT_CONTEXT Context
    )
{
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
        DestroyTracertNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);

    TreeNew_NodesStructured(Context->TreeNewHandle);
}

PTRACERT_ROOT_NODE GetSelectedTracertNode(
    _In_ PNETWORK_TRACERT_CONTEXT Context
    )
{
    PTRACERT_ROOT_NODE windowNode = NULL;
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        windowNode = Context->NodeList->Items[i];

        if (windowNode->Node.Selected)
            return windowNode;
    }

    return NULL;
}

BOOLEAN GetSelectedTracertNodes(
    _In_ PNETWORK_TRACERT_CONTEXT Context,
    _Out_ PTRACERT_ROOT_NODE **Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PTRACERT_ROOT_NODE node = (PTRACERT_ROOT_NODE)Context->NodeList->Items[i];

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

VOID InitializeTracertTree(
    _Inout_ PNETWORK_TRACERT_CONTEXT Context
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        TracertTreeNodeItemType = PhCreateObjectType(L"TracertTreeNodeItem", 0, TracertTreeNodeItemDeleteProcedure);
        PhEndInitOnce(&initOnce);
    }

    Context->NodeList = PhCreateList(100);
    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PTRACERT_ROOT_NODE),
        TracertNodeHashtableEqualFunction,
        TracertNodeHashtableHashFunction,
        100
        );

    PhSetControlTheme(Context->TreeNewHandle, L"explorer");

    TreeNew_SetCallback(Context->TreeNewHandle, TracertTreeNewCallback, Context);

    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_TTL, TRUE, L"TTL", 30, PH_ALIGN_LEFT, -2, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_PING1, TRUE, L"Time", 70, PH_ALIGN_RIGHT, TREE_COLUMN_ITEM_PING1, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_PING2, TRUE, L"Time", 70, PH_ALIGN_RIGHT, TREE_COLUMN_ITEM_PING2, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_PING3, TRUE, L"Time", 70, PH_ALIGN_RIGHT, TREE_COLUMN_ITEM_PING3, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_PING4, TRUE, L"Time", 70, PH_ALIGN_RIGHT, TREE_COLUMN_ITEM_PING4, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_IPADDR, TRUE, L"IP Address", 120, PH_ALIGN_LEFT, TREE_COLUMN_ITEM_IPADDR, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_HOSTNAME, TRUE, L"Hostname", 150, PH_ALIGN_LEFT, TREE_COLUMN_ITEM_HOSTNAME, 0);
    PhAddTreeNewColumnEx2(Context->TreeNewHandle, TREE_COLUMN_ITEM_COUNTRY, TRUE, L"Country", 130, PH_ALIGN_LEFT, TREE_COLUMN_ITEM_COUNTRY, 0, TN_COLUMN_FLAG_CUSTOMDRAW);

    //for (INT i = 0; i < MAX_PINGS; i++)
    //    PhAddTreeNewColumn(context->TreeNewHandle, i + 1, i + 1, i + 1, LVCFMT_RIGHT, 50, L"Time");

    TreeNew_SetSort(Context->TreeNewHandle, TREE_COLUMN_ITEM_TTL, AscendingSortOrder);

    TracertLoadSettingsTreeList(Context);
}

VOID DeleteTracertTree(
    _In_ PNETWORK_TRACERT_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        DestroyTracertNode(Context->NodeList->Items[i]);
    }

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}
