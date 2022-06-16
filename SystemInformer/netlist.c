/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2015
 *     dmex    2017-2022
 *
 */

#include <phapp.h>
#include <netlist.h>

#include <cpysave.h>
#include <emenu.h>
#include <settings.h>

#include <colmgr.h>
#include <extmgri.h>
#include <mainwnd.h>
#include <netprv.h>
#include <phplug.h>
#include <phsettings.h>
#include <procprv.h>

BOOLEAN PhpNetworkNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

ULONG PhpNetworkNodeHashtableHashFunction(
    _In_ PVOID Entry
    );

VOID PhpRemoveNetworkNode(
    _In_ PPH_NETWORK_NODE NetworkNode,
    _In_opt_ PVOID Context
    );

LONG PhpNetworkTreeNewPostSortFunction(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    );

BOOLEAN NTAPI PhpNetworkTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    );

PPH_STRING PhpGetNetworkItemProcessName(
    _In_ PPH_NETWORK_ITEM NetworkItem
    );

static HWND NetworkTreeListHandle;
static ULONG NetworkTreeListSortColumn;
static PH_SORT_ORDER NetworkTreeListSortOrder;
static PH_CM_MANAGER NetworkTreeListCm;

static PPH_HASHTABLE NetworkNodeHashtable; // hashtable of all nodes
static PPH_LIST NetworkNodeList; // list of all nodes
static ULONG64 NextUniqueId = 0;

BOOLEAN PhNetworkTreeListStateHighlighting = TRUE;
static PPH_POINTER_LIST NetworkNodeStateList = NULL; // list of nodes which need to be processed

static PH_TN_FILTER_SUPPORT FilterSupport;

VOID PhNetworkTreeListInitialization(
    VOID
    )
{
    NetworkNodeHashtable = PhCreateHashtable(
        sizeof(PPH_NETWORK_NODE),
        PhpNetworkNodeHashtableEqualFunction,
        PhpNetworkNodeHashtableHashFunction,
        100
        );
    NetworkNodeList = PhCreateList(100);
}

BOOLEAN PhpNetworkNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_NETWORK_NODE networkNode1 = *(PPH_NETWORK_NODE *)Entry1;
    PPH_NETWORK_NODE networkNode2 = *(PPH_NETWORK_NODE *)Entry2;

    return networkNode1->NetworkItem == networkNode2->NetworkItem;
}

ULONG PhpNetworkNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashIntPtr((ULONG_PTR)(*(PPH_NETWORK_NODE *)Entry)->NetworkItem);
}

VOID PhInitializeNetworkTreeList(
    _In_ HWND hwnd
    )
{
    NetworkTreeListHandle = hwnd;
    PhSetControlTheme(NetworkTreeListHandle, L"explorer");
    SendMessage(TreeNew_GetTooltips(NetworkTreeListHandle), TTM_SETDELAYTIME, TTDT_AUTOPOP, MAXSHORT);

    TreeNew_SetCallback(hwnd, PhpNetworkTreeNewCallback, NULL);
    TreeNew_SetImageList(hwnd, PhProcessSmallImageList);

    TreeNew_SetRedraw(hwnd, FALSE);

    // Default columns
    PhAddTreeNewColumn(hwnd, PHNETLC_PROCESS, TRUE, L"Name", 100, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(hwnd, PHNETLC_LOCALADDRESS, TRUE, L"Local address", 120, PH_ALIGN_LEFT, 1, 0);
    PhAddTreeNewColumn(hwnd, PHNETLC_LOCALPORT, TRUE, L"Local port", 50, PH_ALIGN_RIGHT, 2, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHNETLC_REMOTEADDRESS, TRUE, L"Remote address", 120, PH_ALIGN_LEFT, 3, 0);
    PhAddTreeNewColumn(hwnd, PHNETLC_REMOTEPORT, TRUE, L"Remote port", 50, PH_ALIGN_RIGHT, 4, DT_RIGHT);
    PhAddTreeNewColumn(hwnd, PHNETLC_PROTOCOL, TRUE, L"Protocol", 45, PH_ALIGN_LEFT, 5, 0);
    PhAddTreeNewColumn(hwnd, PHNETLC_STATE, TRUE, L"State", 70, PH_ALIGN_LEFT, 6, 0);
    PhAddTreeNewColumn(hwnd, PHNETLC_OWNER, TRUE, L"Owner", 80, PH_ALIGN_LEFT, 7, 0);
    PhAddTreeNewColumnEx(hwnd, PHNETLC_TIMESTAMP, FALSE, L"Time stamp", 100, PH_ALIGN_LEFT, -1, 0, TRUE);
    PhAddTreeNewColumn(hwnd, PHNETLC_LOCALHOSTNAME, FALSE, L"Local hostname", 120, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHNETLC_REMOTEHOSTNAME, FALSE, L"Remote hostname", 120, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHNETLC_PID, FALSE, L"PID", 50, PH_ALIGN_RIGHT, 0, DT_RIGHT);
    PhAddTreeNewColumnEx2(hwnd, PHNETLC_TIMELINE, FALSE, L"Timeline", 100, PH_ALIGN_LEFT, ULONG_MAX, 0, TN_COLUMN_FLAG_CUSTOMDRAW | TN_COLUMN_FLAG_SORTDESCENDING);

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

    PhInitializeTreeNewFilterSupport(&FilterSupport, hwnd, NetworkNodeList);
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

struct _PH_TN_FILTER_SUPPORT *PhGetFilterSupportNetworkTreeList(
    VOID
    )
{
    return &FilterSupport;
}

PPH_NETWORK_NODE PhAddNetworkNode(
    _In_ PPH_NETWORK_ITEM NetworkItem,
    _In_ ULONG RunId
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
    networkNode->UniqueId = ++NextUniqueId; // used to stabilize sorting

    memset(networkNode->TextCache, 0, sizeof(PH_STRINGREF) * PHNETLC_MAXIMUM);
    networkNode->Node.TextCache = networkNode->TextCache;
    networkNode->Node.TextCacheSize = PHNETLC_MAXIMUM;

    networkNode->ProcessNameText = PhpGetNetworkItemProcessName(NetworkItem);

    PhAddEntryHashtable(NetworkNodeHashtable, &networkNode);
    PhAddItemList(NetworkNodeList, networkNode);

    if (FilterSupport.NodeList)
        networkNode->Node.Visible = PhApplyTreeNewFiltersToNode(&FilterSupport, &networkNode->Node);

    PhEmCallObjectOperation(EmNetworkNodeType, networkNode, EmObjectCreate);

    TreeNew_NodesStructured(NetworkTreeListHandle);

    return networkNode;
}

PPH_NETWORK_NODE PhFindNetworkNode(
    _In_ PPH_NETWORK_ITEM NetworkItem
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
    _In_ PPH_NETWORK_NODE NetworkNode
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
        PhpRemoveNetworkNode(NetworkNode, NULL);
    }
}

VOID PhpRemoveNetworkNode(
    _In_ PPH_NETWORK_NODE NetworkNode,
    _In_opt_ PVOID Context
    )
{
    ULONG index;

    PhEmCallObjectOperation(EmNetworkNodeType, NetworkNode, EmObjectDelete);

    // Remove from list and cleanup.

    if ((index = PhFindItemList(NetworkNodeList, NetworkNode)) != -1)
        PhRemoveItemList(NetworkNodeList, index);

    if (NetworkNode->ProcessNameText) PhDereferenceObject(NetworkNode->ProcessNameText);
    if (NetworkNode->TimeStampText) PhDereferenceObject(NetworkNode->TimeStampText);
    if (NetworkNode->PidText) PhDereferenceObject(NetworkNode->PidText);
    if (NetworkNode->TooltipText) PhDereferenceObject(NetworkNode->TooltipText);

    PhDereferenceObject(NetworkNode->NetworkItem);

    PhFree(NetworkNode);

    TreeNew_NodesStructured(NetworkTreeListHandle);
}

VOID PhUpdateNetworkNode(
    _In_ PPH_NETWORK_NODE NetworkNode
    )
{
    memset(NetworkNode->TextCache, 0, sizeof(PH_STRINGREF) * PHNETLC_MAXIMUM);
    PhClearReference(&NetworkNode->TooltipText);

    PhInvalidateTreeNewNode(&NetworkNode->Node, TN_CACHE_ICON);
    TreeNew_InvalidateNode(NetworkTreeListHandle, &NetworkNode->Node);
}

VOID PhTickNetworkNodes(
    VOID
    )
{
    if (NetworkTreeListSortOrder != NoSortOrder)
    {
        // Sorting is on, but it's not one of our columns. Force a rebuild. (If it was one of our
        // columns, the restructure would have been handled in PhUpdateNetworkNode.)
        TreeNew_NodesStructured(NetworkTreeListHandle);
    }

    PH_TICK_SH_STATE_TN(PH_NETWORK_NODE, ShState, NetworkNodeStateList, PhpRemoveNetworkNode, PhCsHighlightingDuration, NetworkTreeListHandle, TRUE, NULL, NULL);
}

#define SORT_FUNCTION(Column) PhpNetworkTreeNewCompare##Column

#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PhpNetworkTreeNewCompare##Column( \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPH_NETWORK_NODE node1 = *(PPH_NETWORK_NODE *)_elem1; \
    PPH_NETWORK_NODE node2 = *(PPH_NETWORK_NODE *)_elem2; \
    PPH_NETWORK_ITEM networkItem1 = node1->NetworkItem; \
    PPH_NETWORK_ITEM networkItem2 = node2->NetworkItem; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uint64cmp(node1->UniqueId, node2->UniqueId); \
    \
    return PhModifySort(sortResult, NetworkTreeListSortOrder); \
}

LONG PhpNetworkTreeNewPostSortFunction(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    )
{
    if (Result == 0)
        Result = uint64cmp(((PPH_NETWORK_NODE)Node1)->UniqueId, ((PPH_NETWORK_NODE)Node2)->UniqueId);

    return PhModifySort(Result, SortOrder);
}

BEGIN_SORT_FUNCTION(Process)
{
    sortResult = PhCompareString(node1->ProcessNameText, node2->ProcessNameText, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LocalAddress)
{
    if (networkItem1->ProtocolType & PH_IPV4_NETWORK_TYPE && networkItem2->ProtocolType & PH_IPV4_NETWORK_TYPE)
    {
        sortResult = uintcmp(networkItem1->LocalEndpoint.Address.InAddr.s_addr, networkItem2->LocalEndpoint.Address.InAddr.s_addr);
    }
    else if (networkItem1->ProtocolType & PH_IPV6_NETWORK_TYPE && networkItem2->ProtocolType & PH_IPV6_NETWORK_TYPE)
    {
        sortResult = memcmp(networkItem1->LocalEndpoint.Address.In6Addr.s6_addr, networkItem2->LocalEndpoint.Address.In6Addr.s6_addr, sizeof(IN6_ADDR));
    }
    else
    {
        sortResult = PhCompareStringZ(networkItem1->LocalAddressString, networkItem2->LocalAddressString, FALSE);
    }
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LocalHostname)
{
    sortResult = PhCompareStringWithNullSortOrder(networkItem1->LocalHostString, networkItem2->LocalHostString, NetworkTreeListSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LocalPort)
{
    sortResult = uintcmp(networkItem1->LocalEndpoint.Port, networkItem2->LocalEndpoint.Port);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(RemoteAddress)
{
    if (networkItem1->ProtocolType & PH_IPV4_NETWORK_TYPE && networkItem2->ProtocolType & PH_IPV4_NETWORK_TYPE)
    {
        sortResult = uintcmp(networkItem1->RemoteEndpoint.Address.InAddr.s_addr, networkItem2->RemoteEndpoint.Address.InAddr.s_addr);
    }
    else if (networkItem1->ProtocolType & PH_IPV6_NETWORK_TYPE && networkItem2->ProtocolType & PH_IPV6_NETWORK_TYPE)
    {
        sortResult = memcmp(networkItem1->RemoteEndpoint.Address.In6Addr.s6_addr, networkItem2->RemoteEndpoint.Address.In6Addr.s6_addr, sizeof(IN6_ADDR));
    }
    else
    {
        sortResult = PhCompareStringZ(networkItem1->RemoteAddressString, networkItem2->RemoteAddressString, FALSE);
    }
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(RemoteHostname)
{
    sortResult = PhCompareStringWithNullSortOrder(networkItem1->RemoteHostString, networkItem2->RemoteHostString, NetworkTreeListSortOrder, TRUE);
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
    sortResult = PhCompareStringWithNullSortOrder(networkItem1->OwnerName, networkItem2->OwnerName, NetworkTreeListSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(TimeStamp)
{
    sortResult = uint64cmp(networkItem1->CreateTime.QuadPart, networkItem2->CreateTime.QuadPart);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Pid)
{
    sortResult = intptrcmp((LONG_PTR)networkItem1->ProcessId, (LONG_PTR)networkItem2->ProcessId);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PhpNetworkTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
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

            if (!getChildren)
                break;

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
                    SORT_FUNCTION(TimeStamp),
                    SORT_FUNCTION(LocalHostname),
                    SORT_FUNCTION(RemoteHostname),
                    SORT_FUNCTION(Pid),
                    SORT_FUNCTION(TimeStamp),
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

            if (!isLeaf)
                break;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;
            PPH_NETWORK_ITEM networkItem;

            if (!getCellText)
                break;

            node = (PPH_NETWORK_NODE)getCellText->Node;
            networkItem = node->NetworkItem;

            switch (getCellText->Id)
            {
            case PHNETLC_PROCESS:
                getCellText->Text = node->ProcessNameText->sr;
                break;
            case PHNETLC_LOCALADDRESS:
                {
                    if (networkItem->LocalAddressStringLength)
                    {
                        getCellText->Text.Buffer = networkItem->LocalAddressString;
                        getCellText->Text.Length = networkItem->LocalAddressStringLength;
                    }
                    else
                    {
                        PhInitializeEmptyStringRef(&getCellText->Text);
                    }
                }
                break;
            case PHNETLC_LOCALHOSTNAME:
                {
                    if (networkItem->LocalHostnameResolved)
                        getCellText->Text = PhGetStringRef(networkItem->LocalHostString);
                    else
                        PhInitializeStringRef(&getCellText->Text, L"Resolving....");
                }
                break;
            case PHNETLC_LOCALPORT:
                PhInitializeStringRefLongHint(&getCellText->Text, networkItem->LocalPortString);
                break;
            case PHNETLC_REMOTEADDRESS:
                {
                    if (networkItem->RemoteAddressStringLength)
                    {
                        getCellText->Text.Buffer = networkItem->RemoteAddressString;
                        getCellText->Text.Length = networkItem->RemoteAddressStringLength;
                    }
                    else
                    {
                        PhInitializeEmptyStringRef(&getCellText->Text);
                    }
                }
                break;
            case PHNETLC_REMOTEHOSTNAME:
                {
                    if (networkItem->RemoteHostnameResolved)
                        getCellText->Text = PhGetStringRef(networkItem->RemoteHostString);
                    else
                        PhInitializeStringRef(&getCellText->Text, L"Resolving....");
                }
                break;
            case PHNETLC_REMOTEPORT:
                PhInitializeStringRefLongHint(&getCellText->Text, networkItem->RemotePortString);
                break;
            case PHNETLC_PROTOCOL:
                {
                    PH_STRINGREF protocolType;

                    protocolType = PhGetProtocolTypeName(networkItem->ProtocolType);
                    getCellText->Text.Buffer = protocolType.Buffer;
                    getCellText->Text.Length = protocolType.Length;
                }
                break;
            case PHNETLC_STATE:
                {
                    if (networkItem->ProtocolType & PH_TCP_PROTOCOL_TYPE)
                    {
                        PH_STRINGREF stateName;

                        stateName = PhGetTcpStateName(networkItem->State);
                        getCellText->Text.Buffer = stateName.Buffer;
                        getCellText->Text.Length = stateName.Length;
                    }
                    else
                    {
                        PhInitializeEmptyStringRef(&getCellText->Text);
                    }
                }
                break;
            case PHNETLC_OWNER:
                getCellText->Text = PhGetStringRef(networkItem->OwnerName);
                break;
            case PHNETLC_TIMESTAMP:
                {
                    SYSTEMTIME systemTime;

                    if (networkItem->CreateTime.QuadPart != 0)
                    {
                        PhLargeIntegerToLocalSystemTime(&systemTime, &networkItem->CreateTime);
                        PhMoveReference(&node->TimeStampText, PhFormatDateTime(&systemTime));
                        getCellText->Text = node->TimeStampText->sr;
                    }
                    else
                    {
                        PhInitializeEmptyStringRef(&getCellText->Text);
                    }
                }
                break;
            case PHNETLC_PID:
                {
                    PH_FORMAT format[1];

                    if (networkItem->ProcessId)
                        PhInitFormatU(&format[0], HandleToUlong(networkItem->ProcessId));
                    else
                        PhInitFormatS(&format[0], L"Waiting connections");

                    PhMoveReference(&node->PidText, PhFormat(format, 1, 96));
                    getCellText->Text = node->PidText->sr;
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

            if (!getNodeIcon)
                break;

            node = (PPH_NETWORK_NODE)getNodeIcon->Node;
            getNodeIcon->Icon = (HICON)(ULONG_PTR)node->NetworkItem->ProcessIconIndex;
            getNodeIcon->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetCellTooltip:
        {
            PPH_TREENEW_GET_CELL_TOOLTIP getCellTooltip = Parameter1;
            PPH_PROCESS_ITEM processItem;

            if (!getCellTooltip)
                break;

            node = (PPH_NETWORK_NODE)getCellTooltip->Node;

            if (getCellTooltip->Column->Id != 0)
                return FALSE;

            if (!node->TooltipText)
            {
                if (processItem = PhReferenceProcessItem(node->NetworkItem->ProcessId))
                {
                    node->TooltipText = PhGetProcessTooltipText(processItem, NULL);
                    PhDereferenceObject(processItem);
                }
            }

            if (!PhIsNullOrEmptyString(node->TooltipText))
            {
                getCellTooltip->Text = PhGetStringRef(node->TooltipText);
                getCellTooltip->Unfolding = FALSE;
                getCellTooltip->MaximumWidth = ULONG_MAX;
            }
            else
            {
                return FALSE;
            }
        }
        return TRUE;
    case TreeNewCustomDraw:
        {
            PPH_TREENEW_CUSTOM_DRAW customDraw = Parameter1;
            PPH_NETWORK_ITEM networkItem;
            RECT rect;

            if (!customDraw)
                break;

            node = (PPH_NETWORK_NODE)customDraw->Node;
            networkItem = node->NetworkItem;
            rect = customDraw->CellRect;

            if (rect.right - rect.left <= 1)
                break; // nothing to draw

            switch (customDraw->Column->Id)
            {
            case PHNETLC_TIMELINE:
                {
                    if (networkItem->CreateTime.QuadPart == 0)
                        break; // nothing to draw

                    PhCustomDrawTreeTimeLine(
                        customDraw->Dc,
                        customDraw->CellRect,
                        PhEnableThemeSupport ? PH_DRAW_TIMELINE_DARKTHEME : 0,
                        NULL,
                        &networkItem->CreateTime
                        );
                }
                break;
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

            if (!keyEvent)
                break;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                    SendMessage(PhMainWndHandle, WM_COMMAND, ID_NETWORK_COPY, 0);
                break;
            case 'A':
                if (GetKeyState(VK_CONTROL) < 0)
                    TreeNew_SelectRange(NetworkTreeListHandle, 0, -1);
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

            data.Selection = PhShowEMenu(data.Menu, hwnd, PH_EMENU_SHOW_LEFTRIGHT,
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
            PPH_TREENEW_CONTEXT_MENU contextMenu = Parameter1;

            if (!contextMenu)
                break;

            PhShowNetworkContextMenu(contextMenu);
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = Parameter1;

            if (!getNodeColor)
                break;

            node = (PPH_NETWORK_NODE)getNodeColor->Node;

            if (!node->NetworkItem)
            {
                NOTHING;
            }
            else if (!node->NetworkItem->ProcessId)
            {
                NOTHING;
            }
            else if (PhCsUseColorPacked && node->NetworkItem->UnknownProcess)
            {
                getNodeColor->BackColor = PhCsColorPacked;
            }
            else if (PhCsUseColorPicoProcesses && node->NetworkItem->SubsystemProcess)
            {
                getNodeColor->BackColor = PhCsColorPicoProcesses;
            }

            getNodeColor->Flags |= TN_AUTO_FORECOLOR;
        }
        return TRUE;
    }

    return FALSE;
}

PPH_STRING PhpGetNetworkItemProcessName(
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    PH_FORMAT format[1];

    if (NetworkItem->ProcessId)
    {
        if (NetworkItem->ProcessName)
            PhInitFormatSR(&format[0], NetworkItem->ProcessName->sr);
        else
            PhInitFormatS(&format[0], L"Unknown process");
    }
    else
    {
        PhInitFormatS(&format[0], L"Waiting connections");
    }

    return PhFormat(format, 1, 96);
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
    _Out_ PPH_NETWORK_ITEM **NetworkItems,
    _Out_ PULONG NumberOfNetworkItems
    )
{
    PH_ARRAY array;
    ULONG i;

    PhInitializeArray(&array, sizeof(PVOID), 2);

    for (i = 0; i < NetworkNodeList->Count; i++)
    {
        PPH_NETWORK_NODE node = NetworkNodeList->Items[i];

        if (node->Node.Selected)
            PhAddItemArray(&array, &node->NetworkItem);
    }

    *NumberOfNetworkItems = (ULONG)array.Count;
    *NetworkItems = PhFinalArrayItems(&array);
}

VOID PhDeselectAllNetworkNodes(
    VOID
    )
{
    TreeNew_DeselectRange(NetworkTreeListHandle, 0, -1);
}

VOID PhSelectAndEnsureVisibleNetworkNode(
    _In_ PPH_NETWORK_NODE NetworkNode
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
    PPH_STRING text;

    text = PhGetTreeNewText(NetworkTreeListHandle, 0);
    PhSetClipboardString(NetworkTreeListHandle, &text->sr);
    PhDereferenceObject(text);
}

VOID PhWriteNetworkList(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ ULONG Mode
    )
{
    PPH_LIST lines;
    ULONG i;

    lines = PhGetGenericTreeNewLines(NetworkTreeListHandle, Mode);

    for (i = 0; i < lines->Count; i++)
    {
        PPH_STRING line;

        line = lines->Items[i];
        PhWriteStringAsUtf8FileStream(FileStream, &line->sr);
        PhDereferenceObject(line);
        PhWriteStringAsUtf8FileStream2(FileStream, L"\r\n");
    }

    PhDereferenceObject(lines);
}
