/*
 * Process Hacker -
 *   network list
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

#include <phapp.h>
#include <settings.h>
#include <extmgri.h>
#include <phplug.h>
#include <cpysave.h>
#include <emenu.h>

BOOLEAN PhpNetworkNodeHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    );

ULONG PhpNetworkNodeHashtableHashFunction(
    __in PVOID Entry
    );

VOID PhpRemoveNetworkNode(
    __in PPH_NETWORK_NODE NetworkNode
    );

LONG PhpNetworkTreeNewPostSortFunction(
    __in LONG Result,
    __in PVOID Node1,
    __in PVOID Node2,
    __in PH_SORT_ORDER SortOrder
    );

BOOLEAN NTAPI PhpNetworkTreeNewCallback(
    __in HWND hwnd,
    __in PH_TREENEW_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in_opt PVOID Context
    );

PPH_STRING PhpGetNetworkItemProcessName(
    __in PPH_NETWORK_ITEM NetworkItem
    );

VOID PhpUpdateNetworkNodeAddressStrings(
    __in PPH_NETWORK_NODE NetworkNode
    );

static HWND NetworkTreeListHandle;
static ULONG NetworkTreeListSortColumn;
static PH_SORT_ORDER NetworkTreeListSortOrder;
static PH_CM_MANAGER NetworkTreeListCm;

static PPH_HASHTABLE NetworkNodeHashtable; // hashtable of all nodes
static PPH_LIST NetworkNodeList; // list of all nodes

BOOLEAN PhNetworkTreeListStateHighlighting = TRUE;
static PPH_POINTER_LIST NetworkNodeStateList = NULL; // list of nodes which need to be processed

VOID PhNetworkTreeListInitialization(
    VOID
    )
{
    NetworkNodeHashtable = PhCreateHashtable(
        sizeof(PPH_NETWORK_NODE),
        PhpNetworkNodeHashtableCompareFunction,
        PhpNetworkNodeHashtableHashFunction,
        100
        );
    NetworkNodeList = PhCreateList(100);
}

BOOLEAN PhpNetworkNodeHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    PPH_NETWORK_NODE networkNode1 = *(PPH_NETWORK_NODE *)Entry1;
    PPH_NETWORK_NODE networkNode2 = *(PPH_NETWORK_NODE *)Entry2;

    return networkNode1->NetworkItem == networkNode2->NetworkItem;
}

ULONG PhpNetworkNodeHashtableHashFunction(
    __in PVOID Entry
    )
{
#ifdef _M_IX86
    return PhHashInt32((ULONG)(*(PPH_NETWORK_NODE *)Entry)->NetworkItem);
#else
    return PhHashInt64((ULONG64)(*(PPH_NETWORK_NODE *)Entry)->NetworkItem);
#endif
}

VOID PhInitializeNetworkTreeList(
    __in HWND hwnd
    )
{
    NetworkTreeListHandle = hwnd;
    PhSetControlTheme(NetworkTreeListHandle, L"explorer");
    SendMessage(TreeNew_GetTooltips(NetworkTreeListHandle), TTM_SETDELAYTIME, TTDT_AUTOPOP, 0x7fff);

    TreeNew_SetCallback(hwnd, PhpNetworkTreeNewCallback, NULL);

    TreeNew_SetRedraw(hwnd, FALSE);

    // Default columns
    PhAddTreeNewColumn(hwnd, PHNETLC_PROCESS, TRUE, L"Name", 100, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(hwnd, PHNETLC_LOCALADDRESS, TRUE, L"Local Address", 120, PH_ALIGN_LEFT, 1, 0);
    PhAddTreeNewColumn(hwnd, PHNETLC_LOCALPORT, TRUE, L"Local Port", 50, PH_ALIGN_RIGHT, 2, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHNETLC_REMOTEADDRESS, TRUE, L"Remote Address", 120, PH_ALIGN_LEFT, 3, 0);
    PhAddTreeNewColumn(hwnd, PHNETLC_REMOTEPORT, TRUE, L"Remote Port", 50, PH_ALIGN_RIGHT, 4, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHNETLC_PROTOCOL, TRUE, L"Protocol", 45, PH_ALIGN_LEFT, 5, 0);
    PhAddTreeNewColumn(hwnd, PHNETLC_STATE, TRUE, L"State", 70, PH_ALIGN_LEFT, 6, 0);
    PhAddTreeNewColumn(hwnd, PHNETLC_OWNER, WINDOWS_HAS_SERVICE_TAGS, L"Owner", 80, PH_ALIGN_LEFT, 7, 0);
    PhAddTreeNewColumnEx(hwnd, PHNETLC_TIMESTAMP, FALSE, L"Time Stamp", 100, PH_ALIGN_LEFT, -1, 0, TRUE);

    TreeNew_SetRedraw(hwnd, TRUE);

    TreeNew_SetSort(hwnd, 0, AscendingSortOrder);

    PhCmInitializeManager(&NetworkTreeListCm, hwnd, PHNETLC_MAXIMUM, PhpNetworkTreeNewPostSortFunction);

    if (PhPluginsEnabled)
    {
        PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

        treeNewInfo.TreeNewHandle = hwnd;
        treeNewInfo.CmData = &NetworkTreeListCm;
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackNetworkTreeNewInitializing), &treeNewInfo);
    }
}

VOID PhLoadSettingsNetworkTreeList(
    VOID
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhGetStringSetting(L"NetworkTreeListColumns");
    sortSettings = PhGetStringSetting(L"NetworkTreeListSort");
    PhCmLoadSettingsEx(NetworkTreeListHandle, &NetworkTreeListCm, 0, &settings->sr, &sortSettings->sr);
    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PhSaveSettingsNetworkTreeList(
    VOID
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhCmSaveSettingsEx(NetworkTreeListHandle, &NetworkTreeListCm, 0, &sortSettings);
    PhSetStringSetting2(L"NetworkTreeListColumns", &settings->sr);
    PhSetStringSetting2(L"NetworkTreeListSort", &sortSettings->sr);
    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

PPH_NETWORK_NODE PhAddNetworkNode(
    __in PPH_NETWORK_ITEM NetworkItem,
    __in ULONG RunId
    )
{
    PPH_NETWORK_NODE networkNode;

    networkNode = PhAllocate(PhEmGetObjectSize(EmNetworkNodeType, sizeof(PH_NETWORK_NODE)));
    memset(networkNode, 0, sizeof(PH_NETWORK_NODE));
    PhInitializeTreeNewNode(&networkNode->Node);

    if (PhNetworkTreeListStateHighlighting && RunId != 1)
    {
        PhChangeShStateTn(
            &networkNode->Node,
            &networkNode->ShState,
            &NetworkNodeStateList,
            NewItemState,
            PhCsColorNew,
            NULL
            );
    }

    networkNode->NetworkItem = NetworkItem;
    PhReferenceObject(NetworkItem);

    memset(networkNode->TextCache, 0, sizeof(PH_STRINGREF) * PHNETLC_MAXIMUM);
    networkNode->Node.TextCache = networkNode->TextCache;
    networkNode->Node.TextCacheSize = PHNETLC_MAXIMUM;

    networkNode->ProcessNameText = PhpGetNetworkItemProcessName(NetworkItem);
    PhpUpdateNetworkNodeAddressStrings(networkNode);

    PhAddEntryHashtable(NetworkNodeHashtable, &networkNode);
    PhAddItemList(NetworkNodeList, networkNode);

    PhEmCallObjectOperation(EmNetworkNodeType, networkNode, EmObjectCreate);

    TreeNew_NodesStructured(NetworkTreeListHandle);

    return networkNode;
}

PPH_NETWORK_NODE PhFindNetworkNode(
    __in PPH_NETWORK_ITEM NetworkItem
    )
{
    PH_NETWORK_NODE lookupNetworkNode;
    PPH_NETWORK_NODE lookupNetworkNodePtr = &lookupNetworkNode;
    PPH_NETWORK_NODE *networkNode;

    lookupNetworkNode.NetworkItem = NetworkItem;

    networkNode = (PPH_NETWORK_NODE *)PhFindEntryHashtable(
        NetworkNodeHashtable,
        &lookupNetworkNodePtr
        );

    if (networkNode)
        return *networkNode;
    else
        return NULL;
}

VOID PhRemoveNetworkNode(
    __in PPH_NETWORK_NODE NetworkNode
    )
{
    // Remove from the hashtable here to avoid problems in case the key is re-used.
    PhRemoveEntryHashtable(NetworkNodeHashtable, &NetworkNode);

    if (PhNetworkTreeListStateHighlighting)
    {
        PhChangeShStateTn(
            &NetworkNode->Node,
            &NetworkNode->ShState,
            &NetworkNodeStateList,
            RemovingItemState,
            PhCsColorRemoved,
            NetworkTreeListHandle
            );
    }
    else
    {
        PhpRemoveNetworkNode(NetworkNode);
    }
}

VOID PhpRemoveNetworkNode(
    __in PPH_NETWORK_NODE NetworkNode
    )
{
    ULONG index;

    PhEmCallObjectOperation(EmNetworkNodeType, NetworkNode, EmObjectDelete);

    // Remove from list and cleanup.

    if ((index = PhFindItemList(NetworkNodeList, NetworkNode)) != -1)
        PhRemoveItemList(NetworkNodeList, index);

    if (NetworkNode->ProcessNameText) PhDereferenceObject(NetworkNode->ProcessNameText);
    if (NetworkNode->TimeStampText) PhDereferenceObject(NetworkNode->TimeStampText);
    if (NetworkNode->TooltipText) PhDereferenceObject(NetworkNode->TooltipText);

    PhDereferenceObject(NetworkNode->NetworkItem);

    PhFree(NetworkNode);

    TreeNew_NodesStructured(NetworkTreeListHandle);
}

VOID PhUpdateNetworkNode(
    __in PPH_NETWORK_NODE NetworkNode
    )
{
    memset(NetworkNode->TextCache, 0, sizeof(PH_STRINGREF) * PHNETLC_MAXIMUM);
    PhpUpdateNetworkNodeAddressStrings(NetworkNode);
    PhSwapReference(&NetworkNode->TooltipText, NULL);

    PhInvalidateTreeNewNode(&NetworkNode->Node, TN_CACHE_ICON);
    TreeNew_NodesStructured(NetworkTreeListHandle);
}

VOID PhTickNetworkNodes(
    VOID
    )
{
    if (NetworkTreeListSortOrder != NoSortOrder && NetworkTreeListSortColumn >= PHNETLC_MAXIMUM)
    {
        // Sorting is on, but it's not one of our columns. Force a rebuild. (If it was one of our
        // columns, the restructure would have been handled in PhUpdateNetworkNode.)
        TreeNew_NodesStructured(NetworkTreeListHandle);
    }

    PH_TICK_SH_STATE_TN(PH_NETWORK_NODE, ShState, NetworkNodeStateList, PhpRemoveNetworkNode, PhCsHighlightingDuration, NetworkTreeListHandle, TRUE, NULL);
}

#define SORT_FUNCTION(Column) PhpNetworkTreeNewCompare##Column

#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PhpNetworkTreeNewCompare##Column( \
    __in const void *_elem1, \
    __in const void *_elem2 \
    ) \
{ \
    PPH_NETWORK_NODE node1 = *(PPH_NETWORK_NODE *)_elem1; \
    PPH_NETWORK_NODE node2 = *(PPH_NETWORK_NODE *)_elem2; \
    PPH_NETWORK_ITEM networkItem1 = node1->NetworkItem; \
    PPH_NETWORK_ITEM networkItem2 = node2->NetworkItem; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    /* if (sortResult == 0) */ \
        /* sortResult = PhCompareString(networkItem1->Name, networkItem2->Name, TRUE); */ \
    \
    return PhModifySort(sortResult, NetworkTreeListSortOrder); \
}

LONG PhpNetworkTreeNewPostSortFunction(
    __in LONG Result,
    __in PVOID Node1,
    __in PVOID Node2,
    __in PH_SORT_ORDER SortOrder
    )
{
    return PhModifySort(Result, SortOrder);
}

BEGIN_SORT_FUNCTION(Process)
{
    sortResult = PhCompareString(node1->ProcessNameText, node2->ProcessNameText, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LocalAddress)
{
    sortResult = PhCompareStringRef(&node1->LocalAddressText, &node2->LocalAddressText, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LocalPort)
{
    sortResult = uintcmp(networkItem1->LocalEndpoint.Port, networkItem2->LocalEndpoint.Port);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(RemoteAddress)
{
    sortResult = PhCompareStringRef(&node1->RemoteAddressText, &node2->RemoteAddressText, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(RemotePort)
{
    sortResult = uintcmp(networkItem1->RemoteEndpoint.Port, networkItem2->RemoteEndpoint.Port);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Protocol)
{
    sortResult = uintcmp(networkItem1->ProtocolType, networkItem2->ProtocolType);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(State)
{
    sortResult = uintcmp(networkItem1->State, networkItem2->State);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Owner)
{
    sortResult = PhCompareStringWithNull(networkItem1->OwnerName, networkItem2->OwnerName, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(TimeStamp)
{
    sortResult = uint64cmp(networkItem1->CreateTime.QuadPart, networkItem2->CreateTime.QuadPart);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PhpNetworkTreeNewCallback(
    __in HWND hwnd,
    __in PH_TREENEW_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in_opt PVOID Context
    )
{
    PPH_NETWORK_NODE node;

    if (PhCmForwardMessage(hwnd, Message, Parameter1, Parameter2, &NetworkTreeListCm))
        return TRUE;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Process),
                    SORT_FUNCTION(LocalAddress),
                    SORT_FUNCTION(LocalPort),
                    SORT_FUNCTION(RemoteAddress),
                    SORT_FUNCTION(RemotePort),
                    SORT_FUNCTION(Protocol),
                    SORT_FUNCTION(State),
                    SORT_FUNCTION(Owner),
                    SORT_FUNCTION(TimeStamp)
                };
                int (__cdecl *sortFunction)(const void *, const void *);

                if (!PhCmForwardSort(
                    (PPH_TREENEW_NODE *)NetworkNodeList->Items,
                    NetworkNodeList->Count,
                    NetworkTreeListSortColumn,
                    NetworkTreeListSortOrder,
                    &NetworkTreeListCm
                    ))
                {
                    if (NetworkTreeListSortColumn < PHNETLC_MAXIMUM)
                        sortFunction = sortFunctions[NetworkTreeListSortColumn];
                    else
                        sortFunction = NULL;

                    if (sortFunction)
                    {
                        qsort(NetworkNodeList->Items, NetworkNodeList->Count, sizeof(PVOID), sortFunction);
                    }
                }

                getChildren->Children = (PPH_TREENEW_NODE *)NetworkNodeList->Items;
                getChildren->NumberOfChildren = NetworkNodeList->Count;
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;
            PPH_NETWORK_ITEM networkItem;

            node = (PPH_NETWORK_NODE)getCellText->Node;
            networkItem = node->NetworkItem;

            switch (getCellText->Id)
            {
            case PHNETLC_PROCESS:
                getCellText->Text = node->ProcessNameText->sr;
                break;
            case PHNETLC_LOCALADDRESS:
                getCellText->Text = node->LocalAddressText;
                break;
            case PHNETLC_LOCALPORT:
                PhInitializeStringRef(&getCellText->Text, networkItem->LocalPortString);
                break;
            case PHNETLC_REMOTEADDRESS:
                getCellText->Text = node->RemoteAddressText;
                break;
            case PHNETLC_REMOTEPORT:
                PhInitializeStringRef(&getCellText->Text, networkItem->RemotePortString);
                break;
            case PHNETLC_PROTOCOL:
                PhInitializeStringRef(&getCellText->Text, PhGetProtocolTypeName(networkItem->ProtocolType));
                break;
            case PHNETLC_STATE:
                if (networkItem->ProtocolType & PH_TCP_PROTOCOL_TYPE)
                    PhInitializeStringRef(&getCellText->Text, PhGetTcpStateName(networkItem->State));
                else
                    PhInitializeEmptyStringRef(&getCellText->Text);
                break;
            case PHNETLC_OWNER:
                if (WINDOWS_HAS_SERVICE_TAGS)
                    getCellText->Text = PhGetStringRef(networkItem->OwnerName);
                else
                    PhInitializeStringRef(&getCellText->Text, L"N/A"); // make sure the user knows this column doesn't work on XP
                break;
            case PHNETLC_TIMESTAMP:
                {
                    SYSTEMTIME systemTime;

                    if (networkItem->CreateTime.QuadPart != 0)
                    {
                        PhLargeIntegerToLocalSystemTime(&systemTime, &networkItem->CreateTime);
                        PhSwapReference2(&node->TimeStampText, PhFormatDateTime(&systemTime));
                        getCellText->Text = node->TimeStampText->sr;
                    }
                    else
                    {
                        PhInitializeEmptyStringRef(&getCellText->Text);
                    }
                }
                break;
            default:
                return FALSE;
            }

            getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeIcon:
        {
            PPH_TREENEW_GET_NODE_ICON getNodeIcon = Parameter1;

            node = (PPH_NETWORK_NODE)getNodeIcon->Node;

            if (node->NetworkItem->ProcessIconValid)
            {
                // TODO: Check if the icon handle is actually valid, since the process item
                // might get destroyed while the network node is still valid.
                getNodeIcon->Icon = node->NetworkItem->ProcessIcon;
            }
            else
            {
                PhGetStockApplicationIcon(&getNodeIcon->Icon, NULL);
            }

            getNodeIcon->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetCellTooltip:
        {
            PPH_TREENEW_GET_CELL_TOOLTIP getCellTooltip = Parameter1;
            PPH_PROCESS_ITEM processItem;

            node = (PPH_NETWORK_NODE)getCellTooltip->Node;

            if (getCellTooltip->Column->Id != 0)
                return FALSE;

            if (!node->TooltipText)
            {
                if (processItem = PhReferenceProcessItem(node->NetworkItem->ProcessId))
                {
                    node->TooltipText = PhGetProcessTooltipText(processItem);
                    PhDereferenceObject(processItem);
                }
            }

            if (!PhIsNullOrEmptyString(node->TooltipText))
            {
                getCellTooltip->Text = node->TooltipText->sr;
                getCellTooltip->Unfolding = FALSE;
                getCellTooltip->MaximumWidth = 550;
            }
            else
            {
                return FALSE;
            }
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            TreeNew_GetSort(hwnd, &NetworkTreeListSortColumn, &NetworkTreeListSortOrder);
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
                    SendMessage(PhMainWndHandle, WM_COMMAND, ID_NETWORK_COPY, 0);
                break;
            case VK_RETURN:
                SendMessage(PhMainWndHandle, WM_COMMAND, ID_NETWORK_GOTOPROCESS, 0);
                break;
            }
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

            data.Selection = PhShowEMenu(data.Menu, hwnd, PH_EMENU_SHOW_LEFTRIGHT | PH_EMENU_SHOW_NONOTIFY,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            SendMessage(PhMainWndHandle, WM_COMMAND, ID_NETWORK_GOTOPROCESS, 0);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_MOUSE_EVENT mouseEvent = Parameter1;

            PhShowNetworkContextMenu(mouseEvent->Location);
        }
        return TRUE;
    }

    return FALSE;
}

PPH_STRING PhpGetNetworkItemProcessName(
    __in PPH_NETWORK_ITEM NetworkItem
    )
{
    PH_FORMAT format[4];

    if (!NetworkItem->ProcessId)
        return PhCreateString(L"Waiting Connections");

    PhInitFormatS(&format[1], L" (");
    PhInitFormatU(&format[2], (ULONG)NetworkItem->ProcessId);
    PhInitFormatC(&format[3], ')');

    if (NetworkItem->ProcessName)
        PhInitFormatSR(&format[0], NetworkItem->ProcessName->sr);
    else
        PhInitFormatS(&format[0], L"Unknown Process");

    return PhFormat(format, 4, 96);
}

VOID PhpUpdateNetworkNodeAddressStrings(
    __in PPH_NETWORK_NODE NetworkNode
    )
{
    if (NetworkNode->NetworkItem->LocalHostString)
        NetworkNode->LocalAddressText = NetworkNode->NetworkItem->LocalHostString->sr;
    else
        PhInitializeStringRef(&NetworkNode->LocalAddressText, NetworkNode->NetworkItem->LocalAddressString);

    if (NetworkNode->NetworkItem->RemoteHostString)
        NetworkNode->RemoteAddressText = NetworkNode->NetworkItem->RemoteHostString->sr;
    else
        PhInitializeStringRef(&NetworkNode->RemoteAddressText, NetworkNode->NetworkItem->RemoteAddressString);
}

PPH_NETWORK_ITEM PhGetSelectedNetworkItem(
    VOID
    )
{
    PPH_NETWORK_ITEM networkItem = NULL;
    ULONG i;

    for (i = 0; i < NetworkNodeList->Count; i++)
    {
        PPH_NETWORK_NODE node = NetworkNodeList->Items[i];

        if (node->Node.Selected)
        {
            networkItem = node->NetworkItem;
            break;
        }
    }

    return networkItem;
}

VOID PhGetSelectedNetworkItems(
    __out PPH_NETWORK_ITEM **NetworkItems,
    __out PULONG NumberOfNetworkItems
    )
{
    PPH_LIST list;
    ULONG i;

    list = PhCreateList(2);

    for (i = 0; i < NetworkNodeList->Count; i++)
    {
        PPH_NETWORK_NODE node = NetworkNodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, node->NetworkItem);
        }
    }

    *NetworkItems = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
    *NumberOfNetworkItems = list->Count;

    PhDereferenceObject(list);
}

VOID PhDeselectAllNetworkNodes(
    VOID
    )
{
    TreeNew_DeselectRange(NetworkTreeListHandle, 0, -1);
}

VOID PhSelectAndEnsureVisibleNetworkNode(
    __in PPH_NETWORK_NODE NetworkNode
    )
{
    PhDeselectAllNetworkNodes();

    if (!NetworkNode->Node.Visible)
        return;

    TreeNew_SetFocusNode(NetworkTreeListHandle, &NetworkNode->Node);
    TreeNew_SetMarkNode(NetworkTreeListHandle, &NetworkNode->Node);
    TreeNew_SelectRange(NetworkTreeListHandle, NetworkNode->Node.Index, NetworkNode->Node.Index);
    TreeNew_EnsureVisible(NetworkTreeListHandle, &NetworkNode->Node);
}

VOID PhCopyNetworkList(
    VOID
    )
{
    PPH_FULL_STRING text;

    text = PhGetTreeNewText(NetworkTreeListHandle, 0);
    PhSetClipboardStringEx(NetworkTreeListHandle, text->Buffer, text->Length);
    PhDereferenceObject(text);
}

VOID PhWriteNetworkList(
    __inout PPH_FILE_STREAM FileStream,
    __in ULONG Mode
    )
{
    PPH_LIST lines;
    ULONG i;

    lines = PhGetGenericTreeNewLines(NetworkTreeListHandle, Mode);

    for (i = 0; i < lines->Count; i++)
    {
        PPH_STRING line;

        line = lines->Items[i];
        PhWriteStringAsAnsiFileStream(FileStream, &line->sr);
        PhDereferenceObject(line);
        PhWriteStringAsAnsiFileStream2(FileStream, L"\r\n");
    }

    PhDereferenceObject(lines);
}
