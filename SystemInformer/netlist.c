/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2015
 *     dmex    2017-2023
 *
 */

#include <phapp.h>
#include <phuisup.h>
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
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_opt_ PVOID Context
    );

VOID PhpUpdateNetworkItemProcessName(
    _In_ PPH_NETWORK_ITEM NetworkItem,
    _In_ PPH_NETWORK_NODE NetworkNode
    );

VOID PhpUpdateNetworkItemProcessId(
    _In_ PPH_NETWORK_ITEM NetworkItem,
    _In_ PPH_NETWORK_NODE NetworkNode
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
    _In_ HWND TreeNewHandle
    )
{
    NetworkTreeListHandle = TreeNewHandle;
    PhSetControlTheme(NetworkTreeListHandle, L"explorer");
    SendMessage(TreeNew_GetTooltips(NetworkTreeListHandle), TTM_SETDELAYTIME, TTDT_AUTOPOP, MAXSHORT);

    TreeNew_SetCallback(TreeNewHandle, PhpNetworkTreeNewCallback, NULL);
    TreeNew_SetImageList(TreeNewHandle, PhProcessSmallImageList);

    TreeNew_SetRedraw(TreeNewHandle, FALSE);

    // Default columns
    PhAddTreeNewColumn(TreeNewHandle, PHNETLC_PROCESS, TRUE, L"Name", 100, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(TreeNewHandle, PHNETLC_PID, TRUE, L"PID", 50, PH_ALIGN_RIGHT, 1, DT_RIGHT);
    PhAddTreeNewColumn(TreeNewHandle, PHNETLC_LOCALADDRESS, TRUE, L"Local address", 120, PH_ALIGN_LEFT, 2, 0);
    PhAddTreeNewColumn(TreeNewHandle, PHNETLC_LOCALPORT, TRUE, L"Local port", 50, PH_ALIGN_RIGHT, 3, DT_RIGHT);
    PhAddTreeNewColumn(TreeNewHandle, PHNETLC_REMOTEADDRESS, TRUE, L"Remote address", 120, PH_ALIGN_LEFT, 4, 0);
    PhAddTreeNewColumn(TreeNewHandle, PHNETLC_REMOTEPORT, TRUE, L"Remote port", 50, PH_ALIGN_RIGHT, 5, DT_RIGHT);
    PhAddTreeNewColumn(TreeNewHandle, PHNETLC_PROTOCOL, TRUE, L"Protocol", 45, PH_ALIGN_LEFT, 6, 0);
    PhAddTreeNewColumn(TreeNewHandle, PHNETLC_STATE, TRUE, L"State", 70, PH_ALIGN_LEFT, 7, 0);
    PhAddTreeNewColumn(TreeNewHandle, PHNETLC_OWNER, TRUE, L"Owner", 80, PH_ALIGN_LEFT, 8, 0);
    PhAddTreeNewColumnEx(TreeNewHandle, PHNETLC_TIMESTAMP, FALSE, L"Time stamp", 100, PH_ALIGN_LEFT, ULONG_MAX, 0, TRUE);
    PhAddTreeNewColumn(TreeNewHandle, PHNETLC_LOCALHOSTNAME, FALSE, L"Local hostname", 120, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(TreeNewHandle, PHNETLC_REMOTEHOSTNAME, FALSE, L"Remote hostname", 120, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PHNETLC_TIMELINE, FALSE, L"Timeline", 100, PH_ALIGN_LEFT, ULONG_MAX, 0, TN_COLUMN_FLAG_CUSTOMDRAW | TN_COLUMN_FLAG_SORTDESCENDING);

    TreeNew_SetRedraw(TreeNewHandle, TRUE);

    TreeNew_SetSort(TreeNewHandle, 0, AscendingSortOrder);

    PhCmInitializeManager(&NetworkTreeListCm, TreeNewHandle, PHNETLC_MAXIMUM, PhpNetworkTreeNewPostSortFunction);

    if (PhPluginsEnabled)
    {
        PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

        treeNewInfo.TreeNewHandle = TreeNewHandle;
        treeNewInfo.CmData = &NetworkTreeListCm;
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackNetworkTreeNewInitializing), &treeNewInfo);
    }

    PhInitializeTreeNewFilterSupport(&FilterSupport, TreeNewHandle, NetworkNodeList);
}

VOID PhLoadSettingsNetworkTreeUpdateMask(
    VOID
    )
{
    PH_TREENEW_COLUMN column;
    BOOLEAN current;

    current = BooleanFlagOn(PhNetworkProviderFlagsMask, PH_NETWORK_PROVIDER_FLAG_HOSTNAME);

    if (
        (TreeNew_GetColumn(NetworkTreeListHandle, PHNETLC_LOCALHOSTNAME, &column) && column.Visible) ||
        (TreeNew_GetColumn(NetworkTreeListHandle, PHNETLC_REMOTEHOSTNAME, &column) && column.Visible)
        )
    {
        SetFlag(PhNetworkProviderFlagsMask, PH_NETWORK_PROVIDER_FLAG_HOSTNAME);
    }
    else
    {
        ClearFlag(PhNetworkProviderFlagsMask, PH_NETWORK_PROVIDER_FLAG_HOSTNAME);
    }

    if (NextUniqueId != 0 && current != BooleanFlagOn(PhNetworkProviderFlagsMask, PH_NETWORK_PROVIDER_FLAG_HOSTNAME))
    {
        PhInvalidateAllNetworkNodesHostnames();
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

    if (PhGetIntegerSetting(L"EnableInstantTooltips"))
    {
        SendMessage(TreeNew_GetTooltips(NetworkTreeListHandle), TTM_SETDELAYTIME, TTDT_INITIAL, 0);
    }

    PhLoadSettingsNetworkTreeUpdateMask();
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

VOID PhReloadSettingsNetworkTreeList(
    VOID
    )
{
    SendMessage(TreeNew_GetTooltips(NetworkTreeListHandle), TTM_SETDELAYTIME, TTDT_INITIAL,
        PhGetIntegerSetting(L"EnableInstantTooltips") ? 0 : -1);

    PhLoadSettingsNetworkTreeUpdateMask();
}

PPH_TN_FILTER_SUPPORT PhGetFilterSupportNetworkTreeList(
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

    PhpUpdateNetworkItemProcessName(NetworkItem, networkNode);
    PhpUpdateNetworkItemProcessId(NetworkItem, networkNode);

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

    if ((index = PhFindItemList(NetworkNodeList, NetworkNode)) != ULONG_MAX)
        PhRemoveItemList(NetworkNodeList, index);

    if (NetworkNode->ProcessNameText) PhDereferenceObject(NetworkNode->ProcessNameText);
    if (NetworkNode->TimeStampText) PhDereferenceObject(NetworkNode->TimeStampText);
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

BEGIN_SORT_FUNCTION(Pid)
{
    sortResult = intptrcmp((LONG_PTR)networkItem1->ProcessId, (LONG_PTR)networkItem2->ProcessId);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(LocalAddress)
{
    SOCKADDR_IN6 localAddress1;
    SOCKADDR_IN6 localAddress2;

    memset(&localAddress1, 0, sizeof(SOCKADDR_IN6)); // memset for zero padding (dmex)
    memset(&localAddress2, 0, sizeof(SOCKADDR_IN6));

    if (networkItem1->LocalEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
    {
        localAddress1.sin6_family = AF_INET6;
        IN6_SET_ADDR_V4COMPAT(&localAddress1.sin6_addr, &networkItem1->LocalEndpoint.Address.InAddr);
        IN4_UNCANONICALIZE_SCOPE_ID(&networkItem1->LocalEndpoint.Address.InAddr, &localAddress1.sin6_scope_struct);
        //IN6ADDR_SETV4MAPPED(&localAddress1, &networkItem1->LocalEndpoint.Address.InAddr, (SCOPE_ID)SCOPEID_UNSPECIFIED_INIT, 0);
    }
    else if (networkItem1->LocalEndpoint.Address.Type == PH_IPV6_NETWORK_TYPE)
    {
        IN6ADDR_SETSOCKADDR(&localAddress1, &networkItem1->LocalEndpoint.Address.In6Addr, (SCOPE_ID){ .Value = networkItem1->LocalScopeId }, 0);
    }

    if (networkItem2->LocalEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
    {
        localAddress2.sin6_family = AF_INET6;
        IN6_SET_ADDR_V4COMPAT(&localAddress2.sin6_addr, &networkItem2->LocalEndpoint.Address.InAddr);
        IN4_UNCANONICALIZE_SCOPE_ID(&networkItem2->LocalEndpoint.Address.InAddr, &localAddress2.sin6_scope_struct);
        //IN6ADDR_SETV4MAPPED(&localAddress2, &networkItem2->LocalEndpoint.Address.InAddr, (SCOPE_ID)SCOPEID_UNSPECIFIED_INIT, 0);
    }
    else if (networkItem2->LocalEndpoint.Address.Type == PH_IPV6_NETWORK_TYPE)
    {
        IN6ADDR_SETSOCKADDR(&localAddress2, &networkItem2->LocalEndpoint.Address.In6Addr, (SCOPE_ID){ .Value = networkItem2->LocalScopeId }, 0);
    }

    sortResult = memcmp(&localAddress1, &localAddress2, sizeof(SOCKADDR_IN6));
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
    SOCKADDR_IN6 remoteAddress1;
    SOCKADDR_IN6 remoteAddress2;

    memset(&remoteAddress1, 0, sizeof(SOCKADDR_IN6)); // memset for zero padding (dmex)
    memset(&remoteAddress2, 0, sizeof(SOCKADDR_IN6));

    if (networkItem1->RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
    {
        remoteAddress1.sin6_family = AF_INET6;
        IN6_SET_ADDR_V4COMPAT(&remoteAddress1.sin6_addr, &networkItem1->RemoteEndpoint.Address.InAddr);
        IN4_UNCANONICALIZE_SCOPE_ID(&networkItem1->RemoteEndpoint.Address.InAddr, &remoteAddress1.sin6_scope_struct);
        //IN6ADDR_SETV4MAPPED(&remoteAddress1, &networkItem1->RemoteEndpoint.Address.InAddr, (SCOPE_ID)SCOPEID_UNSPECIFIED_INIT, 0);
    }
    else if (networkItem1->RemoteEndpoint.Address.Type == PH_IPV6_NETWORK_TYPE)
    {
        IN6ADDR_SETSOCKADDR(&remoteAddress1, &networkItem1->RemoteEndpoint.Address.In6Addr, (SCOPE_ID){ .Value = networkItem1->RemoteScopeId }, 0);
    }

    if (networkItem2->RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
    {
        remoteAddress2.sin6_family = AF_INET6;
        IN6_SET_ADDR_V4COMPAT(&remoteAddress2.sin6_addr, &networkItem2->RemoteEndpoint.Address.InAddr);
        IN4_UNCANONICALIZE_SCOPE_ID(&networkItem2->RemoteEndpoint.Address.InAddr, &remoteAddress2.sin6_scope_struct);
        //IN6ADDR_SETV4MAPPED(&remoteAddress2, &networkItem2->RemoteEndpoint.Address.InAddr, (SCOPE_ID)SCOPEID_UNSPECIFIED_INIT, 0);
    }
    else if (networkItem2->RemoteEndpoint.Address.Type == PH_IPV6_NETWORK_TYPE)
    {
        IN6ADDR_SETSOCKADDR(&remoteAddress2, &networkItem2->RemoteEndpoint.Address.In6Addr, (SCOPE_ID){ .Value = networkItem2->RemoteScopeId }, 0);
    }

    sortResult = memcmp(&remoteAddress1, &remoteAddress2, sizeof(SOCKADDR_IN6));
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

BOOLEAN NTAPI PhpNetworkTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
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

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Process),
                    SORT_FUNCTION(Pid),
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
                    SORT_FUNCTION(TimeStamp),
                };
                int (__cdecl *sortFunction)(const void *, const void *);

                static_assert(RTL_NUMBER_OF(sortFunctions) == PHNETLC_MAXIMUM, "SortFunctions must equal maximum.");

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
                getCellText->Text = PhGetStringRef(node->ProcessNameText);
                break;
            case PHNETLC_PID:
                {
                    PhInitializeStringRefLongHint(&getCellText->Text, node->ProcessIdString);
                }
                break;
            case PHNETLC_LOCALADDRESS:
                {
                    getCellText->Text = PhGetStringRef(networkItem->LocalAddressString);
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
                {
                    PhInitializeStringRefLongHint(&getCellText->Text, networkItem->LocalPortString);
                }
                break;
            case PHNETLC_REMOTEADDRESS:
                {
                    getCellText->Text = PhGetStringRef(networkItem->RemoteAddressString);
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
                {
                    PhInitializeStringRefLongHint(&getCellText->Text, networkItem->RemotePortString);
                }
                break;
            case PHNETLC_PROTOCOL:
                {
                    PPH_STRINGREF protocolType;

                    if (protocolType = PhGetProtocolTypeName(networkItem->ProtocolType))
                    {
                        getCellText->Text.Buffer = protocolType->Buffer;
                        getCellText->Text.Length = protocolType->Length;
                    }
                    else
                    {
                        PhInitializeEmptyStringRef(&getCellText->Text);
                    }
                }
                break;
            case PHNETLC_STATE:
                {
                    if (FlagOn(networkItem->ProtocolType, PH_TCP_PROTOCOL_TYPE))
                    {
                        PPH_STRINGREF stateName;

                        if (stateName = PhGetTcpStateName(networkItem->State))
                        {
                            getCellText->Text.Buffer = stateName->Buffer;
                            getCellText->Text.Length = stateName->Length;
                        }
                        else
                        {
                            PhInitializeEmptyStringRef(&getCellText->Text);
                        }
                    }
                    else if (networkItem->ProtocolType == PH_HV_NETWORK_PROTOCOL)
                    {
                        if (networkItem->State)
                        {
                            PhInitializeStringRef(&getCellText->Text, L"Connected");
                        }
                        else
                        {
                            PhInitializeStringRef(&getCellText->Text, L"Listen");
                        }
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
            getNodeIcon->Icon = (HICON)(ULONG_PTR)node->NetworkItem->ProcessIconIndex;
            //getNodeIcon->Flags = TN_CACHE;
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
            PhInitializeTreeNewColumnMenuEx(&data, PH_TN_COLUMN_MENU_SHOW_RESET_SORT);

            data.Selection = PhShowEMenu(
                data.Menu,
                hwnd,
                PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP,
                data.MouseEvent->ScreenLocation.x,
                data.MouseEvent->ScreenLocation.y
                );

            PhHandleTreeNewColumnMenu(&data);

            if (data.Selection)
            {
                if (data.Selection->Id == PH_TN_COLUMN_MENU_HIDE_COLUMN_ID ||
                    data.Selection->Id == PH_TN_COLUMN_MENU_CHOOSE_COLUMNS_ID)
                {
                    // TODO: Reset flags for enabled/disabled columns. (dmex)
                    PhReloadSettingsNetworkTreeList();
                }
            }

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

            PhShowNetworkContextMenu(contextMenu);
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = Parameter1;
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

VOID PhpUpdateNetworkItemProcessName(
    _In_ PPH_NETWORK_ITEM NetworkItem,
    _In_ PPH_NETWORK_NODE NetworkNode
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PH_STRINGREF processNameUnknown = PH_STRINGREF_INIT(L"Unknown process");
    static PH_STRINGREF processNameWaiting = PH_STRINGREF_INIT(L"Waiting connections");
    static PPH_STRING cachedNameUnknown = NULL;
    static PPH_STRING cachedNameWaiting = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        cachedNameUnknown = PhCreateString2(&processNameUnknown);
        cachedNameWaiting = PhCreateString2(&processNameWaiting);
        PhEndInitOnce(&initOnce);
    }

    if (NetworkItem->ProcessId)
    {
        if (NetworkItem->ProcessName)
            PhSetReference(&NetworkNode->ProcessNameText, NetworkItem->ProcessName);
        else
            PhSetReference(&NetworkNode->ProcessNameText, cachedNameUnknown);
    }
    else
    {
        PhSetReference(&NetworkNode->ProcessNameText, cachedNameWaiting);
    }
}

VOID PhpUpdateNetworkItemProcessId(
    _In_ PPH_NETWORK_ITEM NetworkItem,
    _In_ PPH_NETWORK_NODE NetworkNode
    )
{
    if (NetworkItem->ProcessId)
    {
        if (NetworkItem->ProcessItem)
        {
            memcpy(
                NetworkNode->ProcessIdString,
                NetworkItem->ProcessItem->ProcessIdString,
                sizeof(NetworkNode->ProcessIdString)
                );
        }
        else
        {
            PhPrintUInt32(NetworkNode->ProcessIdString, HandleToUlong(NetworkItem->ProcessId));
        }
    }
    else
    {
        PhPrintUInt32(NetworkNode->ProcessIdString, HandleToUlong(SYSTEM_PROCESS_ID));
    }
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

VOID PhInvalidateAllNetworkNodes(
    VOID
    )
{
    for (ULONG i = 0; i < NetworkNodeList->Count; i++)
    {
        PPH_NETWORK_NODE node = NetworkNodeList->Items[i];

        memset(node->TextCache, 0, sizeof(PH_STRINGREF) * PHNETLC_MAXIMUM);
        PhInvalidateTreeNewNode(&node->Node, TN_CACHE_COLOR);
    }

    InvalidateRect(NetworkTreeListHandle, NULL, FALSE);
}

VOID PhInvalidateAllNetworkNodesHostnames(
    VOID
    )
{
    PhFlushNetworkItemResolveCache();

    for (ULONG i = 0; i < NetworkNodeList->Count; i++)
    {
        PPH_NETWORK_NODE node = NetworkNodeList->Items[i];

        node->NetworkItem->InvalidateHostname = TRUE;

        memset(node->TextCache, 0, sizeof(PH_STRINGREF) * PHNETLC_MAXIMUM);
        PhInvalidateTreeNewNode(&node->Node, TN_CACHE_COLOR);
    }

    //PPH_NETWORK_ITEM* networkItems;
    //ULONG numberOfNetworkItems;
    //
    //PhEnumNetworkItems(&networkItems, &numberOfNetworkItems);
    //
    //for (ULONG j = 0; j < numberOfNetworkItems; j++)
    //{
    //    PPH_NETWORK_ITEM networkItem = networkItems[j];
    //
    //    networkItem->InvalidateHostname = TRUE;
    //}
    //
    //PhDereferenceObjects(networkItems, numberOfNetworkItems);
    //PhFree(networkItems);
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

PPH_LIST PhDuplicateNetworkNodeList(
    VOID
    )
{
    PPH_LIST newList;

    newList = PhCreateList(NetworkNodeList->Count);
    PhInsertItemsList(newList, 0, NetworkNodeList->Items, NetworkNodeList->Count);

    return newList;
}
