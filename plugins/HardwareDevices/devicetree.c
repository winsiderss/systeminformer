/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2023
 *
 */

#include "devices.h"
#include <toolstatusintf.h>

#include <devguid.h>

typedef struct _DEVICE_NODE
{
    PH_TREENEW_NODE Node;
    PH_SH_STATE ShState;
    PPH_DEVICE_ITEM DeviceItem;
    PPH_LIST Children;
    ULONG_PTR IconIndex;
    PH_STRINGREF TextCache[PhMaxDeviceProperty];
} DEVICE_NODE, *PDEVICE_NODE;

typedef struct _DEVICE_TREE
{
    PPH_DEVICE_TREE Tree;
    PPH_LIST Nodes;
    PPH_LIST Roots;
} DEVICE_TREE, *PDEVICE_TREE;

static BOOLEAN AutoRefreshDeviceTree = TRUE;
static BOOLEAN ShowDisconnected = TRUE;
static BOOLEAN ShowSoftwareComponents = TRUE;
static BOOLEAN HighlightUpperFiltered = TRUE;
static BOOLEAN HighlightLowerFiltered = TRUE;
static BOOLEAN ShowDeviceInterfaces = TRUE;
static BOOLEAN ShowDisabledDeviceInterfaces = TRUE;
static ULONG DeviceProblemColor = 0;
static ULONG DeviceDisabledColor = 0;
static ULONG DeviceDisconnectedColor = 0;
static ULONG DeviceHighlightColor = 0;
static ULONG DeviceInterfaceColor = 0;
static ULONG DeviceDisabledInterfaceColor = 0;
static ULONG DeviceArrivedColor = 0;
static ULONG DeviceHighlightingDuration = 0;

static BOOLEAN DeviceTabCreated = FALSE;
static HWND DeviceTreeHandle = NULL;
static PH_CALLBACK_REGISTRATION DeviceNotifyRegistration = { 0 };
static PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration = { 0 };
static PH_CALLBACK_REGISTRATION SettingsUpdatedCallbackRegistration = { 0 };
static PDEVICE_TREE DeviceTree = NULL;
static HIMAGELIST DeviceImageList = NULL;
static PH_INTEGER_PAIR DeviceIconSize = { 16, 16 };
static PH_LIST DeviceFilterList = { 0 };
static PPH_MAIN_TAB_PAGE DevicesAddedTabPage = NULL;
static PTOOLSTATUS_INTERFACE ToolStatusInterface = NULL;
static BOOLEAN DeviceTabSelected = FALSE;
static ULONG DeviceTreeSortColumn = 0;
static PH_SORT_ORDER DeviceTreeSortOrder = NoSortOrder;
static PH_TN_FILTER_SUPPORT DeviceTreeFilterSupport = { 0 };
static PPH_TN_FILTER_ENTRY DeviceTreeFilterEntry = NULL;
static PH_CALLBACK_REGISTRATION SearchChangedRegistration = { 0 };
static PPH_POINTER_LIST DeviceNodeStateList = NULL;

static int __cdecl DeviceListSortByNameFunction(
    const void* Left,
    const void* Right
    )
{
    PDEVICE_NODE lhsNode;
    PDEVICE_NODE rhsNode;
    PPH_DEVICE_PROPERTY lhs;
    PPH_DEVICE_PROPERTY rhs;

    lhsNode = *(PDEVICE_NODE*)Left;
    rhsNode = *(PDEVICE_NODE*)Right;
    lhs = PhGetDeviceProperty(lhsNode->DeviceItem, PhDevicePropertyName);
    rhs = PhGetDeviceProperty(rhsNode->DeviceItem, PhDevicePropertyName);

    return PhCompareStringWithNull(lhs->AsString, rhs->AsString, TRUE);
}

static int __cdecl DeviceNodeSortFunction(
    const void* Left,
    const void* Right
    )
{
    PDEVICE_NODE lhsItem;
    PDEVICE_NODE rhsItem;

    lhsItem = *(PDEVICE_NODE*)Left;
    rhsItem = *(PDEVICE_NODE*)Right;

    return uintcmp(lhsItem->DeviceItem->InstanceIdHash, rhsItem->DeviceItem->InstanceIdHash);
}

static int __cdecl DeviceNodeSearchFunction(
    const void* Hash,
    const void* Item
    )
{
    PDEVICE_NODE item;

    item = *(PDEVICE_NODE*)Item;

    return uintcmp(PtrToUlong(Hash), item->DeviceItem->InstanceIdHash);
}

_Success_(return != NULL)
_Must_inspect_result_
PDEVICE_NODE DeviceTreeLookupNode(
    _In_ PDEVICE_TREE Tree,
    _In_ ULONG InstanceIdHash
    )
{
    PDEVICE_NODE* deviceItem;

    deviceItem = bsearch(
        UlongToPtr(InstanceIdHash),
        Tree->Nodes->Items,
        Tree->Nodes->Count,
        sizeof(PVOID),
        DeviceNodeSearchFunction
        );

    return deviceItem ? *deviceItem : NULL;
}

BOOLEAN DeviceTreeShouldIncludeDeviceItem(
    _In_ PPH_DEVICE_ITEM DeviceItem
    )
{
    if (DeviceItem->DeviceInterface)
    {
        if (!ShowDeviceInterfaces)
            return FALSE;

        if (ShowDisabledDeviceInterfaces)
            return TRUE;

        return PhGetDeviceProperty(DeviceItem, PhDevicePropertyInterfaceEnabled)->Boolean;
    }
    else
    {
        if (ShowDisconnected)
            return TRUE;

        if (!ShowSoftwareComponents && IsEqualGUID(&DeviceItem->ClassGuid, &GUID_DEVCLASS_SOFTWARECOMPONENT))
            return FALSE;

        return PhGetDeviceProperty(DeviceItem, PhDevicePropertyIsPresent)->Boolean;
    }
}

BOOLEAN DeviceTreeIsJustArrivedDeviceItem(
    _In_ PPH_DEVICE_ITEM DeviceItem
    )
{
    LARGE_INTEGER lastArrival;
    LARGE_INTEGER systemTime;
    LARGE_INTEGER elapsed;

    lastArrival = PhGetDeviceProperty(DeviceItem, PhDevicePropertyLastArrivalDate)->TimeStamp;

    if (lastArrival.QuadPart <= 0)
        return FALSE;

    PhQuerySystemTime(&systemTime);

    elapsed.QuadPart = systemTime.QuadPart - lastArrival.QuadPart;

    // convert to milliseconds
    elapsed.QuadPart /= 10000;

    // consider devices that arrived in that last 10 seconds as "just arrived"
    if (elapsed.QuadPart < (10 * 1000))
    {
        return TRUE;
    }

    return FALSE;
}

PDEVICE_NODE DeviceTreeCreateNode(
    _In_ PPH_DEVICE_ITEM Item,
    _Inout_ PPH_LIST Nodes
    )
{
    PDEVICE_NODE node;
    HICON iconHandle;

    node = PhAllocateZero(sizeof(DEVICE_NODE));

    PhInitializeTreeNewNode(&node->Node);
    node->Node.TextCache = node->TextCache;
    node->Node.TextCacheSize = RTL_NUMBER_OF(node->TextCache);

    node->DeviceItem = Item;
    iconHandle = PhGetDeviceIcon(Item, &DeviceIconSize);
    if (iconHandle)
    {
        node->IconIndex = PhImageListAddIcon(DeviceImageList, iconHandle);
        DestroyIcon(iconHandle);
    }
    else
    {
        node->IconIndex = 0; // Must be set to zero (dmex)
    }

    if (DeviceTreeFilterSupport.NodeList)
        node->Node.Visible = PhApplyTreeNewFiltersToNode(&DeviceTreeFilterSupport, &node->Node);
    else
        node->Node.Visible = TRUE;

    node->Children = PhCreateList(Item->ChildrenCount);
    for (PPH_DEVICE_ITEM item = Item->Child; item; item = item->Sibling)
    {
        if (DeviceTreeShouldIncludeDeviceItem(item))
            PhAddItemList(node->Children, DeviceTreeCreateNode(item, Nodes));
    }

    if (PhGetIntegerSetting(SETTING_NAME_DEVICE_SORT_CHILDREN_BY_NAME))
        qsort(node->Children->Items, node->Children->Count, sizeof(PVOID), DeviceListSortByNameFunction);

    PhAddItemList(Nodes, node);
    return node;
}

PDEVICE_TREE DeviceTreeCreate(
    _In_ PPH_DEVICE_TREE Tree
    )
{
    PDEVICE_TREE tree = PhAllocateZero(sizeof(DEVICE_TREE));

    tree->Nodes = PhCreateList(Tree->DeviceList->AllocatedCount);
    if (PhGetIntegerSetting(SETTING_NAME_DEVICE_SHOW_ROOT))
    {
        tree->Roots = PhCreateList(1);
        PhAddItemList(tree->Roots, DeviceTreeCreateNode(Tree->Root, tree->Nodes));
    }
    else
    {
        tree->Roots = PhCreateList(Tree->Root->ChildrenCount);
        for (PPH_DEVICE_ITEM item = Tree->Root->Child; item; item = item->Sibling)
        {
            if (DeviceTreeShouldIncludeDeviceItem(item))
                PhAddItemList(tree->Roots, DeviceTreeCreateNode(item, tree->Nodes));
        }

        if (PhGetIntegerSetting(SETTING_NAME_DEVICE_SORT_CHILDREN_BY_NAME))
            qsort(tree->Roots->Items, tree->Roots->Count, sizeof(PVOID), DeviceListSortByNameFunction);
    }

    qsort(tree->Nodes->Items, tree->Nodes->Count, sizeof(PVOID), DeviceNodeSortFunction);

    tree->Tree = PhReferenceObject(Tree);
    return tree;
}

VOID DeviceTreeFree(
    _In_opt_ PDEVICE_TREE Tree
    )
{
    if (!Tree)
        return;

    for (ULONG i = 0; i < Tree->Nodes->Count; i++)
    {
        PDEVICE_NODE node = Tree->Nodes->Items[i];
        PhDereferenceObject(node->Children);
        PhFree(node);
    }

    PhDereferenceObject(Tree->Nodes);
    PhDereferenceObject(Tree->Roots);
    PhDereferenceObject(Tree->Tree);
    PhFree(Tree);
}

_Must_inspect_impl_
_Success_(return != NULL)
PDEVICE_TREE DeviceTreeCreateIfNecessary(
    _In_ BOOLEAN Force
    )
{
    PDEVICE_TREE deviceTree;
    PPH_DEVICE_TREE tree;

    tree = PhReferenceDeviceTreeEx(Force);
    if (Force || !DeviceTree || DeviceTree->Tree != tree)
    {
        deviceTree = DeviceTreeCreate(tree);
    }
    else
    {
        // the device tree hasn't changed, no need to create a new one
        deviceTree = NULL;
    }

    PhDereferenceObject(tree);

    return deviceTree;
}

VOID NTAPI DeviceTreePublish(
    _In_opt_ PDEVICE_TREE Tree
)
{
    PDEVICE_TREE oldTree;

    if (!Tree)
        return;

    TreeNew_SetRedraw(DeviceTreeHandle, FALSE);

    oldTree = DeviceTree;
    DeviceTree = Tree;
    DeviceFilterList.AllocatedCount = DeviceTree->Nodes->AllocatedCount;
    DeviceFilterList.Count = DeviceTree->Nodes->Count;
    DeviceFilterList.Items = DeviceTree->Nodes->Items;

    if (oldTree)
    {
        // TODO PhClearPointerList
        PhMoveReference(&DeviceNodeStateList, NULL);

        for (ULONG i = 0; i < DeviceTree->Nodes->Count; i++)
        {
            PDEVICE_NODE node = DeviceTree->Nodes->Items[i];
            PDEVICE_NODE old = DeviceTreeLookupNode(oldTree, node->DeviceItem->InstanceIdHash);

            if (old)
            {
                node->Node.Selected = old->Node.Selected;
            }

            if (DeviceTreeIsJustArrivedDeviceItem(node->DeviceItem))
            {
                PhChangeShStateTn(
                    &node->Node,
                    &node->ShState,
                    &DeviceNodeStateList,
                    NewItemState,
                    DeviceArrivedColor,
                    NULL
                    );
            }
        }
    }

    TreeNew_SetRedraw(DeviceTreeHandle, TRUE);

    TreeNew_NodesStructured(DeviceTreeHandle);

    if (DeviceTreeFilterSupport.FilterList)
        PhApplyTreeNewFilters(&DeviceTreeFilterSupport);

    DeviceTreeFree(oldTree);
}

NTSTATUS NTAPI DeviceTreePublishThread(
    _In_ PVOID Parameter
    )
{
    BOOLEAN force = PtrToUlong(Parameter) ? TRUE : FALSE;

    ProcessHacker_Invoke(DeviceTreePublish, DeviceTreeCreateIfNecessary(force));

    return STATUS_SUCCESS;
}

VOID DeviceTreePublishAsync(
    _In_ BOOLEAN Force
    )
{
    PhCreateThread2(DeviceTreePublishThread, ULongToPtr(Force ? 1ul : 0ul));
}

VOID InvalidateDeviceNodes(
    VOID
    )
{
    if (!DeviceTree)
        return;

    for (ULONG i = 0; i < DeviceTree->Nodes->Count; i++)
    {
        PDEVICE_NODE node;

        node = DeviceTree->Nodes->Items[i];

        PhInvalidateTreeNewNode(&node->Node, TN_CACHE_COLOR);
        TreeNew_InvalidateNode(DeviceTreeHandle, &node->Node);
    }
}

_Function_class_(PH_TN_FILTER_FUNCTION)
BOOLEAN NTAPI DeviceTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PDEVICE_NODE node = (PDEVICE_NODE)Node;

    if (PhIsNullOrEmptyString(ToolStatusInterface->GetSearchboxText()))
        return TRUE;

    for (ULONG i = 0; i < ARRAYSIZE(node->DeviceItem->Properties); i++)
    {
        PPH_DEVICE_PROPERTY prop;

        prop = PhGetDeviceProperty(node->DeviceItem, i);

        if (PhIsNullOrEmptyString(prop->AsString))
            continue;

        if (ToolStatusInterface->WordMatch(&prop->AsString->sr))
            return TRUE;
    }

    return FALSE;
}

VOID NTAPI DeviceTreeSearchChangedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (DeviceTabSelected)
        PhApplyTreeNewFilters(&DeviceTreeFilterSupport);
}

static int __cdecl DeviceTreeSortFunction(
    const void* Left,
    const void* Right
    )
{
    int sortResult;
    PDEVICE_NODE lhsNode;
    PDEVICE_NODE rhsNode;
    PPH_DEVICE_PROPERTY lhs;
    PPH_DEVICE_PROPERTY rhs;
    PH_STRINGREF srl;
    PH_STRINGREF srr;

    sortResult = 0;
    lhsNode = *(PDEVICE_NODE*)Left;
    rhsNode = *(PDEVICE_NODE*)Right;
    lhs = PhGetDeviceProperty(lhsNode->DeviceItem, DeviceTreeSortColumn);
    rhs = PhGetDeviceProperty(rhsNode->DeviceItem, DeviceTreeSortColumn);

    assert(lhs->Type == rhs->Type);

    if (!lhs->Valid && !rhs->Valid)
    {
        sortResult = 0;
    }
    else if (lhs->Valid && !rhs->Valid)
    {
        sortResult = 1;
    }
    else if (!lhs->Valid && rhs->Valid)
    {
        sortResult = -1;
    }
    else
    {
        switch (lhs->Type)
        {
        case PhDevicePropertyTypeString:
            sortResult = PhCompareString(lhs->String, rhs->String, TRUE);
            break;
        case PhDevicePropertyTypeUInt64:
            sortResult = uint64cmp(lhs->UInt64, rhs->UInt64);
            break;
        case PhDevicePropertyTypeUInt32:
            sortResult = uint64cmp(lhs->UInt32, rhs->UInt32);
            break;
        case PhDevicePropertyTypeInt32:
        case PhDevicePropertyTypeNTSTATUS:
            sortResult = int64cmp(lhs->Int32, rhs->Int32);
            break;
        case PhDevicePropertyTypeGUID:
            sortResult = memcmp(&lhs->Guid, &rhs->Guid, sizeof(GUID));
            break;
        case PhDevicePropertyTypeBoolean:
            {
                if (lhs->Boolean && !rhs->Boolean)
                    sortResult = 1;
                else if (!lhs->Boolean && rhs->Boolean)
                    sortResult = -1;
                else
                    sortResult = 0;
            }
            break;
        case PhDevicePropertyTypeTimeStamp:
            sortResult = int64cmp(lhs->TimeStamp.QuadPart, rhs->TimeStamp.QuadPart);
            break;
        case PhDevicePropertyTypeStringList:
            {
                srl = PhGetStringRef(lhs->AsString);
                srr = PhGetStringRef(rhs->AsString);
                sortResult = PhCompareStringRef(&srl, &srr, TRUE);
            }
            break;
        case PhDevicePropertyTypeBinary:
            {
                sortResult = memcmp(lhs->Binary.Buffer, rhs->Binary.Buffer, min(lhs->Binary.Size, rhs->Binary.Size));
                if (sortResult == 0)
                    sortResult = uint64cmp(lhs->Binary.Size, rhs->Binary.Size);
            }
            break;
        default:
            assert(FALSE);
        }
    }

    if (sortResult == 0)
    {
        srl = PhGetStringRef(lhsNode->DeviceItem->Properties[PhDevicePropertyName].AsString);
        srr = PhGetStringRef(rhsNode->DeviceItem->Properties[PhDevicePropertyName].AsString);
        sortResult = PhCompareStringRef(&srl, &srr, TRUE);
    }

    return PhModifySort(sortResult, DeviceTreeSortOrder);
}

VOID DeviceNodeShowProperties(
    _In_ HWND ParentWindowHandle,
    _In_ PDEVICE_NODE DeviceNode
    )
{
    PPH_DEVICE_ITEM deviceItem;

    if (DeviceNode->DeviceItem->DeviceInterface)
        deviceItem = DeviceNode->DeviceItem->Parent;
    else
        deviceItem = DeviceNode->DeviceItem;

    if (deviceItem->InstanceId)
        DeviceShowProperties(ParentWindowHandle, deviceItem);
}

BOOLEAN NTAPI DeviceTreeCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PDEVICE_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            if (!DeviceTree)
            {
                getChildren->Children = NULL;
                getChildren->NumberOfChildren = 0;
            }
            else
            {
                node = (PDEVICE_NODE)getChildren->Node;

                if (DeviceTreeSortOrder == NoSortOrder)
                {
                    if (!node)
                    {
                        getChildren->Children = (PPH_TREENEW_NODE*)DeviceTree->Roots->Items;
                        getChildren->NumberOfChildren = DeviceTree->Roots->Count;
                    }
                    else
                    {
                        getChildren->Children = (PPH_TREENEW_NODE*)node->Children->Items;
                        getChildren->NumberOfChildren = node->Children->Count;
                    }
                }
                else
                {
                    if (!node)
                    {
                        if (DeviceTreeSortColumn < PhMaxDeviceProperty)
                        {
                            qsort(
                                DeviceTree->Nodes->Items,
                                DeviceTree->Nodes->Count,
                                sizeof(PVOID),
                                DeviceTreeSortFunction
                                );
                        }
                    }

                    getChildren->Children = (PPH_TREENEW_NODE*)DeviceTree->Nodes->Items;
                    getChildren->NumberOfChildren = DeviceTree->Nodes->Count;
                }
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;
            node = (PDEVICE_NODE)isLeaf->Node;

            if (DeviceTreeSortOrder == NoSortOrder)
                isLeaf->IsLeaf = node->Children->Count == 0;
            else
                isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;
            node = (PDEVICE_NODE)getCellText->Node;

            PPH_STRING text = PhGetDeviceProperty(node->DeviceItem, getCellText->Id)->AsString;

            getCellText->Text = PhGetStringRef(text);
            getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = Parameter1;
            node = (PDEVICE_NODE)getNodeColor->Node;

            getNodeColor->Flags = TN_CACHE | TN_AUTO_FORECOLOR;

            if (node->DeviceItem->DeviceInterface)
            {
                if (PhGetDeviceProperty(node->DeviceItem, PhDevicePropertyInterfaceEnabled)->Boolean)
                    getNodeColor->BackColor = DeviceInterfaceColor;
                else
                    getNodeColor->BackColor = DeviceDisabledInterfaceColor;
            }
            else if (node->DeviceItem->DevNodeStatus & DN_HAS_PROBLEM && (node->DeviceItem->ProblemCode != CM_PROB_DISABLED))
            {
                getNodeColor->BackColor = DeviceProblemColor;
            }
            else if (!PhGetDeviceProperty(node->DeviceItem, PhDevicePropertyIsPresent)->Boolean)
            {
                getNodeColor->BackColor = DeviceDisconnectedColor;
            }
            else if ((node->DeviceItem->Capabilities & CM_DEVCAP_HARDWAREDISABLED) || (node->DeviceItem->ProblemCode == CM_PROB_DISABLED))
            {
                getNodeColor->BackColor = DeviceDisabledColor;
            }
            else if ((HighlightUpperFiltered && node->DeviceItem->HasUpperFilters) || (HighlightLowerFiltered && node->DeviceItem->HasLowerFilters))
            {
                getNodeColor->BackColor = DeviceHighlightColor;
            }
        }
        return TRUE;
    case TreeNewGetNodeIcon:
        {
            PPH_TREENEW_GET_NODE_ICON getNodeIcon = Parameter1;

            node = (PDEVICE_NODE)getNodeIcon->Node;
            getNodeIcon->Icon = (HICON)(ULONG_PTR)node->IconIndex;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            TreeNew_GetSort(hwnd, &DeviceTreeSortColumn, &DeviceTreeSortOrder);
            TreeNew_NodesStructured(hwnd);
            if (DeviceTreeFilterSupport.FilterList)
                PhApplyTreeNewFilters(&DeviceTreeFilterSupport);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;
            PPH_EMENU menu;
            PPH_EMENU subMenu;
            PPH_EMENU_ITEM selectedItem;
            PPH_EMENU_ITEM autoRefresh;
            PPH_EMENU_ITEM showDisconnectedDevices;
            PPH_EMENU_ITEM showSoftwareDevices;
            PPH_EMENU_ITEM showDeviceInterfaces;
            PPH_EMENU_ITEM showDisabledDeviceInterfaces;
            PPH_EMENU_ITEM highlightUpperFiltered;
            PPH_EMENU_ITEM highlightLowerFiltered;
            PPH_EMENU_ITEM gotoServiceItem;
            PPH_EMENU_ITEM enable;
            PPH_EMENU_ITEM disable;
            PPH_EMENU_ITEM restart;
            PPH_EMENU_ITEM uninstall;
            PPH_EMENU_ITEM properties;
            BOOLEAN republish;
            BOOLEAN invalidate;

            node = (PDEVICE_NODE)contextMenuEvent->Node;

            menu = PhCreateEMenu();
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 100, L"Refresh", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, autoRefresh = PhCreateEMenuItem(0, 101, L"Refresh automatically", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
            PhInsertEMenuItem(menu, showDisconnectedDevices = PhCreateEMenuItem(0, 102, L"Show disconnected devices", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, showSoftwareDevices = PhCreateEMenuItem(0, 103, L"Show software components", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, showDeviceInterfaces = PhCreateEMenuItem(0, 104, L"Show device interfaces", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, showDisabledDeviceInterfaces = PhCreateEMenuItem(0, 105, L"Show disabled device interfaces", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, highlightUpperFiltered = PhCreateEMenuItem(0, 106, L"Highlight upper filtered", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, highlightLowerFiltered = PhCreateEMenuItem(0, 107, L"Highlight lower filtered", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
            PhInsertEMenuItem(menu, gotoServiceItem = PhCreateEMenuItem(0, 108, L"Go to service...", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
            PhInsertEMenuItem(menu, enable = PhCreateEMenuItem(0, 0, L"Enable", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, disable = PhCreateEMenuItem(0, 1, L"Disable", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, restart = PhCreateEMenuItem(0, 2, L"Restart", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, uninstall = PhCreateEMenuItem(0, 3, L"Uninstall", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
            subMenu = PhCreateEMenuItem(0, 0, L"Open key", NULL, NULL);
            PhInsertEMenuItem(subMenu, PhCreateEMenuItem(0, HW_KEY_INDEX_HARDWARE, L"Hardware", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(subMenu, PhCreateEMenuItem(0, HW_KEY_INDEX_SOFTWARE, L"Software", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(subMenu, PhCreateEMenuItem(0, HW_KEY_INDEX_USER, L"User", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(subMenu, PhCreateEMenuItem(0, HW_KEY_INDEX_CONFIG, L"Config", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, subMenu, ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
            PhInsertEMenuItem(menu, properties = PhCreateEMenuItem(0, 10, L"Properties", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 11, L"Copy", NULL, NULL), ULONG_MAX);
            PhInsertCopyCellEMenuItem(menu, 11, DeviceTreeHandle, contextMenuEvent->Column);
            PhSetFlagsEMenuItem(menu, 10, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

            if (AutoRefreshDeviceTree)
                autoRefresh->Flags |= PH_EMENU_CHECKED;
            if (ShowDisconnected)
                showDisconnectedDevices->Flags |= PH_EMENU_CHECKED;
            if (ShowSoftwareComponents)
                showSoftwareDevices->Flags |= PH_EMENU_CHECKED;
            if (HighlightUpperFiltered)
                highlightUpperFiltered->Flags |= PH_EMENU_CHECKED;
            if (HighlightLowerFiltered)
                highlightLowerFiltered->Flags |= PH_EMENU_CHECKED;
            if (ShowDeviceInterfaces)
                showDeviceInterfaces->Flags |= PH_EMENU_CHECKED;
            if (ShowDisabledDeviceInterfaces)
                showDisabledDeviceInterfaces->Flags |= PH_EMENU_CHECKED;

            if (!node)
            {
                PhSetDisabledEMenuItem(gotoServiceItem);
                PhSetDisabledEMenuItem(subMenu);
                PhSetDisabledEMenuItem(properties);
            }

            if (gotoServiceItem && node)
            {
                PPH_STRING serviceName = PhGetDeviceProperty(node->DeviceItem, PhDevicePropertyService)->AsString;

                if (PhIsNullOrEmptyString(serviceName))
                {
                    PhSetDisabledEMenuItem(gotoServiceItem);
                }
            }

            if (!PhGetOwnTokenAttributes().Elevated)
            {
                enable->Flags |= PH_EMENU_DISABLED;
                disable->Flags |= PH_EMENU_DISABLED;
                restart->Flags |= PH_EMENU_DISABLED;
                uninstall->Flags |= PH_EMENU_DISABLED;
            }

            selectedItem = PhShowEMenu(
                menu,
                PhMainWndHandle,
                PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP,
                contextMenuEvent->Location.x,
                contextMenuEvent->Location.y
                );

            republish = FALSE;
            invalidate = FALSE;

            if (selectedItem && selectedItem->Id != ULONG_MAX)
            {
                if (!PhHandleCopyCellEMenuItem(selectedItem))
                {
                    switch (selectedItem->Id)
                    {
                    case 0:
                    case 1:
                        if (node->DeviceItem->InstanceId)
                            republish = HardwareDeviceEnableDisable(hwnd, node->DeviceItem->InstanceId, selectedItem->Id == 0);
                        break;
                    case 2:
                        if (node->DeviceItem->InstanceId)
                            republish = HardwareDeviceRestart(hwnd, node->DeviceItem->InstanceId);
                        break;
                    case 3:
                        if (node->DeviceItem->InstanceId)
                            republish = HardwareDeviceUninstall(hwnd, node->DeviceItem->InstanceId);
                        break;
                    case HW_KEY_INDEX_HARDWARE:
                    case HW_KEY_INDEX_SOFTWARE:
                    case HW_KEY_INDEX_USER:
                    case HW_KEY_INDEX_CONFIG:
                        if (node->DeviceItem->InstanceId)
                            HardwareDeviceOpenKey(hwnd, node->DeviceItem->InstanceId, selectedItem->Id);
                        break;
                    case 10:
                        DeviceNodeShowProperties(hwnd, node);
                        break;
                    case 11:
                        {
                            PPH_STRING text;

                            text = PhGetTreeNewText(DeviceTreeHandle, 0);
                            PhSetClipboardString(DeviceTreeHandle, &text->sr);
                            PhDereferenceObject(text);
                        }
                        break;
                    case 100:
                        republish = TRUE;
                        break;
                    case 101:
                        AutoRefreshDeviceTree = !AutoRefreshDeviceTree;
                        PhSetIntegerSetting(SETTING_NAME_DEVICE_TREE_AUTO_REFRESH, AutoRefreshDeviceTree);
                        break;
                    case 102:
                        ShowDisconnected = !ShowDisconnected;
                        PhSetIntegerSetting(SETTING_NAME_DEVICE_TREE_SHOW_DISCONNECTED, ShowDisconnected);
                        republish = TRUE;
                        break;
                    case 103:
                        ShowSoftwareComponents = !ShowSoftwareComponents;
                        PhSetIntegerSetting(SETTING_NAME_DEVICE_SHOW_SOFTWARE_COMPONENTS, ShowSoftwareComponents);
                        republish = TRUE;
                        break;
                    case 104:
                        ShowDeviceInterfaces = !ShowDeviceInterfaces;
                        PhSetIntegerSetting(SETTING_NAME_DEVICE_SHOW_DEVICE_INTERFACES, ShowDeviceInterfaces);
                        republish = TRUE;
                        break;
                    case 105:
                        ShowDisabledDeviceInterfaces = !ShowDisabledDeviceInterfaces;
                        PhSetIntegerSetting(SETTING_NAME_DEVICE_SHOW_DISABLED_DEVICE_INTERFACES, ShowDisabledDeviceInterfaces);
                        republish = TRUE;
                        break;
                    case 106:
                        HighlightUpperFiltered = !HighlightUpperFiltered;
                        PhSetIntegerSetting(SETTING_NAME_DEVICE_TREE_HIGHLIGHT_UPPER_FILTERED, HighlightUpperFiltered);
                        invalidate = TRUE;
                        break;
                    case 107:
                        HighlightLowerFiltered = !HighlightLowerFiltered;
                        PhSetIntegerSetting(SETTING_NAME_DEVICE_TREE_HIGHLIGHT_LOWER_FILTERED, HighlightLowerFiltered);
                        invalidate = TRUE;
                        break;
                    case 108:
                        {
                            PPH_STRING serviceName = PhGetDeviceProperty(node->DeviceItem, PhDevicePropertyService)->AsString;
                            PPH_SERVICE_ITEM serviceItem;

                            if (!PhIsNullOrEmptyString(serviceName))
                            {
                                if (serviceItem = PhReferenceServiceItem(PhGetString(serviceName)))
                                {
                                    ProcessHacker_SelectTabPage(1);
                                    ProcessHacker_SelectServiceItem(serviceItem);
                                    PhDereferenceObject(serviceItem);
                                }
                            }
                        }
                        break;
                    }
                }
            }

            PhDestroyEMenu(menu);

            if (republish)
            {
                DeviceTreePublishAsync(TRUE);
            }
            else if (invalidate)
            {
                InvalidateDeviceNodes();
                if (DeviceTreeFilterSupport.FilterList)
                    PhApplyTreeNewFilters(&DeviceTreeFilterSupport);
            }
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            PPH_TREENEW_MOUSE_EVENT mouseEvent = Parameter1;
            node = (PDEVICE_NODE)mouseEvent->Node;

            if (node)
                DeviceNodeShowProperties(hwnd, node);
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = hwnd;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = 0;
            data.DefaultSortOrder = NoSortOrder;
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
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    }

    return FALSE;
}

VOID DevicesTreeLoadSettings(
    _In_ HWND TreeNewHandle
    )
{
    PPH_STRING settings;
    PH_INTEGER_PAIR sortSettings;

    settings = PhGetStringSetting(SETTING_NAME_DEVICE_TREE_COLUMNS);
    sortSettings = PhGetIntegerPairSetting(SETTING_NAME_DEVICE_TREE_SORT);
    PhCmLoadSettings(TreeNewHandle, &settings->sr);
    TreeNew_SetSort(TreeNewHandle, (ULONG)sortSettings.X, (ULONG)sortSettings.Y);
    PhDereferenceObject(settings);
}

VOID DevicesTreeSaveSettings(
    VOID
    )
{
    PPH_STRING settings;
    PH_INTEGER_PAIR sortSettings;
    ULONG sortColumn;
    ULONG sortOrder;

    if (!DeviceTabCreated)
        return;

    settings = PhCmSaveSettings(DeviceTreeHandle);
    TreeNew_GetSort(DeviceTreeHandle, &sortColumn, &sortOrder);
    sortSettings.X = sortColumn;
    sortSettings.Y = sortOrder;
    PhSetStringSetting2(SETTING_NAME_DEVICE_TREE_COLUMNS, &settings->sr);
    PhSetIntegerPairSetting(SETTING_NAME_DEVICE_TREE_SORT, sortSettings);
    PhDereferenceObject(settings);
}

VOID DevicesTreeImageListInitialize(
    _In_ HWND TreeNewHandle
    )
{
    LONG dpi;

    dpi = PhGetWindowDpi(TreeNewHandle);
    DeviceIconSize.X = PhGetSystemMetrics(SM_CXSMICON, dpi);
    DeviceIconSize.Y = PhGetSystemMetrics(SM_CYSMICON, dpi);

    if (DeviceImageList)
    {
        PhImageListSetIconSize(DeviceImageList, DeviceIconSize.X, DeviceIconSize.Y);
    }
    else
    {
        DeviceImageList = PhImageListCreate(
            DeviceIconSize.X,
            DeviceIconSize.Y,
            ILC_MASK | ILC_COLOR32,
            200,
            100
            );
    }

    PhImageListAddIcon(DeviceImageList, PhGetApplicationIcon(TRUE));

    TreeNew_SetImageList(DeviceTreeHandle, DeviceImageList);
}

const DEVICE_PROPERTY_TABLE_ENTRY DeviceItemPropertyTable[] =
{
    { PhDevicePropertyName, L"Name", TRUE, 400, 0 },
    { PhDevicePropertyManufacturer, L"Manufacturer", TRUE, 180, 0 },
    { PhDevicePropertyService, L"Service", TRUE, 120, 0 },
    { PhDevicePropertyClass, L"Class", TRUE, 120, 0 },
    { PhDevicePropertyEnumeratorName, L"Enumerator", TRUE, 80, 0 },
    { PhDevicePropertyInstallDate, L"Installed", TRUE, 160, 0 },

    { PhDevicePropertyFirstInstallDate, L"First installed", FALSE, 160, 0 },
    { PhDevicePropertyLastArrivalDate, L"Last arrival", FALSE, 160, 0 },
    { PhDevicePropertyLastRemovalDate, L"Last removal", FALSE, 160, 0 },
    { PhDevicePropertyDeviceDesc, L"Description", FALSE, 280, 0 },
    { PhDevicePropertyFriendlyName, L"Friendly name", FALSE, 220, 0 },
    { PhDevicePropertyInstanceId, L"Instance ID", FALSE, 240, DT_PATH_ELLIPSIS },
    { PhDevicePropertyParentInstanceId, L"Parent instance ID", FALSE, 240, DT_PATH_ELLIPSIS },
    { PhDevicePropertyPDOName, L"PDO name", FALSE, 180, DT_PATH_ELLIPSIS },
    { PhDevicePropertyLocationInfo, L"Location info", FALSE, 180, DT_PATH_ELLIPSIS },
    { PhDevicePropertyClassGuid, L"Class GUID", FALSE, 80, 0 },
    { PhDevicePropertyDriver, L"Driver", FALSE, 180, DT_PATH_ELLIPSIS },
    { PhDevicePropertyDriverVersion, L"Driver version", FALSE, 80, 0 },
    { PhDevicePropertyDriverDate, L"Driver date", FALSE, 80, 0 },
    { PhDevicePropertyFirmwareDate, L"Firmware date", FALSE, 80, 0 },
    { PhDevicePropertyFirmwareVersion, L"Firmware version", FALSE, 80, 0 },
    { PhDevicePropertyFirmwareRevision, L"Firmware revision", FALSE, 80, 0 },
    { PhDevicePropertyHasProblem, L"Has problem", FALSE, 80, 0 },
    { PhDevicePropertyProblemCode, L"Problem code", FALSE, 80, 0 },
    { PhDevicePropertyProblemStatus, L"Problem status", FALSE, 80, 0 },
    { PhDevicePropertyDevNodeStatus, L"Node status flags", FALSE, 80, 0 },
    { PhDevicePropertyDevCapabilities, L"Capabilities", FALSE, 80, 0 },
    { PhDevicePropertyUpperFilters, L"Upper filters", FALSE, 80, 0 },
    { PhDevicePropertyLowerFilters, L"Lower filters", FALSE, 80, 0 },
    { PhDevicePropertyHardwareIds, L"Hardware IDs ", FALSE, 80, 0 },
    { PhDevicePropertyCompatibleIds, L"Compatible IDs", FALSE, 80, 0 },
    { PhDevicePropertyConfigFlags, L"Configuration flags", FALSE, 80, 0 },
    { PhDevicePropertyUINumber, L"Number", FALSE, 80, 0 },
    { PhDevicePropertyBusTypeGuid, L"Bus type GUID", FALSE, 80, 0 },
    { PhDevicePropertyLegacyBusType, L"Legacy bus type", FALSE, 80, 0 },
    { PhDevicePropertyBusNumber, L"Bus number", FALSE, 80, 0 },
    { PhDevicePropertySecurity, L"Security descriptor (binary)", FALSE, 80, 0 },
    { PhDevicePropertySecuritySDS, L"Security descriptor", FALSE, 80, 0 },
    { PhDevicePropertyDevType, L"Type", FALSE, 80, 0 },
    { PhDevicePropertyExclusive, L"Exclusive", FALSE, 80, 0 },
    { PhDevicePropertyCharacteristics, L"Characteristics", FALSE, 80, 0 },
    { PhDevicePropertyAddress, L"Address", FALSE, 80, 0 },
    { PhDevicePropertyPowerData, L"Power data", FALSE, 80, 0 },
    { PhDevicePropertyRemovalPolicy, L"Removal policy", FALSE, 80, 0 },
    { PhDevicePropertyRemovalPolicyDefault, L"Removal policy default", FALSE, 80, 0 },
    { PhDevicePropertyRemovalPolicyOverride, L"Removal policy override", FALSE, 80, 0 },
    { PhDevicePropertyInstallState, L"Install state", FALSE, 80, 0 },
    { PhDevicePropertyLocationPaths, L"Location paths", FALSE, 80, 0 },
    { PhDevicePropertyBaseContainerId, L"Base container ID", FALSE, 80, 0 },
    { PhDevicePropertyEjectionRelations, L"Ejection relations", FALSE, 80, 0 },
    { PhDevicePropertyRemovalRelations, L"Removal relations", FALSE, 80, 0 },
    { PhDevicePropertyPowerRelations, L"Power relations", FALSE, 80, 0 },
    { PhDevicePropertyBusRelations, L"Bus relations", FALSE, 80, 0 },
    { PhDevicePropertyChildren, L"Children", FALSE, 80, 0 },
    { PhDevicePropertySiblings, L"Siblings", FALSE, 80, 0 },
    { PhDevicePropertyTransportRelations, L"Transport relations", FALSE, 80, 0 },
    { PhDevicePropertyReported, L"Reported", FALSE, 80, 0 },
    { PhDevicePropertyLegacy, L"Legacy", FALSE, 80, 0 },
    { PhDevicePropertyContainerId, L"Container ID", FALSE, 80, 0 },
    { PhDevicePropertyInLocalMachineContainer, L"Local machine container", FALSE, 80, 0 },
    { PhDevicePropertyModel, L"Model", FALSE, 80, 0 },
    { PhDevicePropertyModelId, L"Model ID", FALSE, 80, 0 },
    { PhDevicePropertyFriendlyNameAttributes, L"Friendly name attributes", FALSE, 80, 0 },
    { PhDevicePropertyManufacturerAttributes, L"Manufacture attributes", FALSE, 80, 0 },
    { PhDevicePropertyPresenceNotForDevice, L"Presence not for device", FALSE, 80, 0 },
    { PhDevicePropertySignalStrength, L"Signal strength", FALSE, 80, 0 },
    { PhDevicePropertyIsAssociateableByUserAction, L"Associateable by user action", FALSE, 80, 0 },
    { PhDevicePropertyShowInUninstallUI, L"Show uninstall UI", FALSE, 80, 0 },
    { PhDevicePropertyNumaProximityDomain, L"Numa proximity default", FALSE, 80, 0 },
    { PhDevicePropertyDHPRebalancePolicy, L"DHP rebalance policy", FALSE, 80, 0 },
    { PhDevicePropertyNumaNode, L"Numa Node", FALSE, 80, 0 },
    { PhDevicePropertyBusReportedDeviceDesc, L"Bus reported description", FALSE, 80, 0 },
    { PhDevicePropertyIsPresent, L"Present", FALSE, 80, 0 },
    { PhDevicePropertyConfigurationId, L"Configuration ID", FALSE, 80, 0 },
    { PhDevicePropertyReportedDeviceIdsHash, L"Reported IDs hash", FALSE, 80, 0 },
    { PhDevicePropertyPhysicalDeviceLocation, L"Physical location", FALSE, 80, 0 },
    { PhDevicePropertyBiosDeviceName, L"BIOS name", FALSE, 80, 0 },
    { PhDevicePropertyDriverProblemDesc, L"Problem description", FALSE, 80, 0 },
    { PhDevicePropertyDebuggerSafe, L"Debugger safe", FALSE, 80, 0 },
    { PhDevicePropertyPostInstallInProgress, L"Post install in progress", FALSE, 80, 0 },
    { PhDevicePropertyStack, L"Stack", FALSE, 80, 0 },
    { PhDevicePropertyExtendedConfigurationIds, L"Extended configuration IDs", FALSE, 80, 0 },
    { PhDevicePropertyIsRebootRequired, L"Reboot required", FALSE, 80, 0 },
    { PhDevicePropertyDependencyProviders, L"Dependency providers", FALSE, 80, 0 },
    { PhDevicePropertyDependencyDependents, L"Dependency dependents", FALSE, 80, 0 },
    { PhDevicePropertySoftRestartSupported, L"Soft restart supported", FALSE, 80, 0 },
    { PhDevicePropertyExtendedAddress, L"Extended address", FALSE, 80, 0 },
    { PhDevicePropertyAssignedToGuest, L"Assigned to guest", FALSE, 80, 0 },
    { PhDevicePropertyCreatorProcessId, L"Creator process ID", FALSE, 80, 0 },
    { PhDevicePropertyFirmwareVendor, L"Firmware vendor", FALSE, 80, 0 },
    { PhDevicePropertySessionId, L"Session ID", FALSE, 80, 0 },
    { PhDevicePropertyDriverDesc, L"Driver description", FALSE, 80, 0 },
    { PhDevicePropertyDriverInfPath, L"Driver INF path", FALSE, 80, 0 },
    { PhDevicePropertyDriverInfSection, L"Driver INF section", FALSE, 80, 0 },
    { PhDevicePropertyDriverInfSectionExt, L"Driver INF section extended", FALSE, 80, 0 },
    { PhDevicePropertyMatchingDeviceId, L"Matching ID", FALSE, 80, 0 },
    { PhDevicePropertyDriverProvider, L"Driver provider", FALSE, 80, 0 },
    { PhDevicePropertyDriverPropPageProvider, L"Driver property page provider", FALSE, 80, 0 },
    { PhDevicePropertyDriverCoInstallers, L"Driver co-installers", FALSE, 80, 0 },
    { PhDevicePropertyResourcePickerTags, L"Resource picker tags", FALSE, 80, 0 },
    { PhDevicePropertyResourcePickerExceptions, L"Resource picker exceptions", FALSE, 80, 0 },
    { PhDevicePropertyDriverRank, L"Driver rank", FALSE, 80, 0 },
    { PhDevicePropertyDriverLogoLevel, L"Driver LOGO level", FALSE, 80, 0 },
    { PhDevicePropertyNoConnectSound, L"No connect sound", FALSE, 80, 0 },
    { PhDevicePropertyGenericDriverInstalled, L"Generic driver installed", FALSE, 80, 0 },
    { PhDevicePropertyAdditionalSoftwareRequested, L"Additional software requested", FALSE, 80, 0 },
    { PhDevicePropertySafeRemovalRequired, L"Safe removal required", FALSE, 80, 0 },
    { PhDevicePropertySafeRemovalRequiredOverride, L"Save removal required override", FALSE, 80, 0 },

    { PhDevicePropertyPkgModel, L"Package model", FALSE, 80, 0 },
    { PhDevicePropertyPkgVendorWebSite, L"Package vendor website", FALSE, 80, 0 },
    { PhDevicePropertyPkgDetailedDescription, L"Package description", FALSE, 80, 0 },
    { PhDevicePropertyPkgDocumentationLink, L"Package documentation", FALSE, 80, 0 },
    { PhDevicePropertyPkgIcon, L"Package icon", FALSE, 80, 0 },
    { PhDevicePropertyPkgBrandingIcon, L"Package branding icon", FALSE, 80, 0 },

    { PhDevicePropertyClassUpperFilters, L"Class upper filters", FALSE, 80, 0 },
    { PhDevicePropertyClassLowerFilters, L"Class lower filters", FALSE, 80, 0 },
    { PhDevicePropertyClassSecurity, L"Class security descriptor (binary)", FALSE, 80, 0 },
    { PhDevicePropertyClassSecuritySDS, L"Class security descriptor", FALSE, 80, 0 },
    { PhDevicePropertyClassDevType, L"Class type", FALSE, 80, 0 },
    { PhDevicePropertyClassExclusive, L"Class exclusive", FALSE, 80, 0 },
    { PhDevicePropertyClassCharacteristics, L"Class characteristics", FALSE, 80, 0 },
    { PhDevicePropertyClassName, L"Class device name", FALSE, 80, 0 },
    { PhDevicePropertyClassClassName, L"Class name", FALSE, 80, 0 },
    { PhDevicePropertyClassIcon, L"Class icon", FALSE, 80, 0 },
    { PhDevicePropertyClassClassInstaller, L"Class installer", FALSE, 80, 0 },
    { PhDevicePropertyClassPropPageProvider, L"Class property page provider", FALSE, 80, 0 },
    { PhDevicePropertyClassNoInstallClass, L"Class no install", FALSE, 80, 0 },
    { PhDevicePropertyClassNoDisplayClass, L"Class no display", FALSE, 80, 0 },
    { PhDevicePropertyClassSilentInstall, L"Class silent install", FALSE, 80, 0 },
    { PhDevicePropertyClassNoUseClass, L"Class no use class", FALSE, 80, 0 },
    { PhDevicePropertyClassDefaultService, L"Class default service", FALSE, 80, 0 },
    { PhDevicePropertyClassIconPath, L"Class icon path", FALSE, 80, 0 },
    { PhDevicePropertyClassDHPRebalanceOptOut, L"Class DHP rebalance opt-out", FALSE, 80, 0 },
    { PhDevicePropertyClassClassCoInstallers, L"Class co-installers", FALSE, 80, 0 },

    { PhDevicePropertyInterfaceFriendlyName, L"Interface friendly name", FALSE, 80, 0 },
    { PhDevicePropertyInterfaceEnabled, L"Interface enabled", FALSE, 80, 0 },
    { PhDevicePropertyInterfaceClassGuid, L"Interface class GUID", FALSE, 80, 0 },
    { PhDevicePropertyInterfaceReferenceString, L"Interface reference", FALSE, 80, 0 },
    { PhDevicePropertyInterfaceRestricted, L"Interface restricted", FALSE, 80, 0 },
    { PhDevicePropertyInterfaceUnrestrictedAppCapabilities, L"Interface unrestricted application capabilities", FALSE, 80, 0 },
    { PhDevicePropertyInterfaceSchematicName, L"Interface schematic name", FALSE, 80, 0 },

    { PhDevicePropertyInterfaceClassDefaultInterface, L"Interface class default interface", FALSE, 80, 0 },
    { PhDevicePropertyInterfaceClassName, L"Interface class name", FALSE, 80, 0 },

    { PhDevicePropertyContainerAddress, L"Container address", FALSE, 80, 0 },
    { PhDevicePropertyContainerDiscoveryMethod, L"Container discovery method", FALSE, 80, 0 },
    { PhDevicePropertyContainerIsEncrypted, L"Container encrypted", FALSE, 80, 0 },
    { PhDevicePropertyContainerIsAuthenticated, L"Container authenticated", FALSE, 80, 0 },
    { PhDevicePropertyContainerIsConnected, L"Container connected", FALSE, 80, 0 },
    { PhDevicePropertyContainerIsPaired, L"Container paired", FALSE, 80, 0 },
    { PhDevicePropertyContainerIcon, L"Container icon", FALSE, 80, 0 },
    { PhDevicePropertyContainerVersion, L"Container version", FALSE, 80, 0 },
    { PhDevicePropertyContainerLastSeen, L"Container last seen", FALSE, 80, 0 },
    { PhDevicePropertyContainerLastConnected, L"Container last connected", FALSE, 80, 0 },
    { PhDevicePropertyContainerIsShowInDisconnectedState, L"Container show in disconnected state", FALSE, 80, 0 },
    { PhDevicePropertyContainerIsLocalMachine, L"Container local machine", FALSE, 80, 0 },
    { PhDevicePropertyContainerMetadataPath, L"Container metadata path", FALSE, 80, 0 },
    { PhDevicePropertyContainerIsMetadataSearchInProgress, L"Container metadata search in progress", FALSE, 80, 0 },
    { PhDevicePropertyContainerIsMetadataChecksum, L"Metadata checksum", FALSE, 80, 0 },
    { PhDevicePropertyContainerIsNotInterestingForDisplay, L"Container not interesting for display", FALSE, 80, 0 },
    { PhDevicePropertyContainerLaunchDeviceStageOnDeviceConnect, L"Container launch on connect", FALSE, 80, 0 },
    { PhDevicePropertyContainerLaunchDeviceStageFromExplorer, L"Container launch from explorer", FALSE, 80, 0 },
    { PhDevicePropertyContainerBaselineExperienceId, L"Container baseline experience ID", FALSE, 80, 0 },
    { PhDevicePropertyContainerIsDeviceUniquelyIdentifiable, L"Container uniquely identifiable", FALSE, 80, 0 },
    { PhDevicePropertyContainerAssociationArray, L"Container association", FALSE, 80, 0 },
    { PhDevicePropertyContainerDeviceDescription1, L"Container description", FALSE, 80, 0 },
    { PhDevicePropertyContainerDeviceDescription2, L"Container description other", FALSE, 80, 0 },
    { PhDevicePropertyContainerHasProblem, L"Container has problem", FALSE, 80, 0 },
    { PhDevicePropertyContainerIsSharedDevice, L"Container shared device", FALSE, 80, 0 },
    { PhDevicePropertyContainerIsNetworkDevice, L"Container network device", FALSE, 80, 0 },
    { PhDevicePropertyContainerIsDefaultDevice, L"Container default device", FALSE, 80, 0 },
    { PhDevicePropertyContainerMetadataCabinet, L"Container metadata cabinet", FALSE, 80, 0 },
    { PhDevicePropertyContainerRequiresPairingElevation, L"Container requires pairing elevation", FALSE, 80, 0 },
    { PhDevicePropertyContainerExperienceId, L"Container experience ID", FALSE, 80, 0 },
    { PhDevicePropertyContainerCategory, L"Container category", FALSE, 80, 0 },
    { PhDevicePropertyContainerCategoryDescSingular, L"Container category description", FALSE, 80, 0 },
    { PhDevicePropertyContainerCategoryDescPlural, L"Container category description plural", FALSE, 80, 0 },
    { PhDevicePropertyContainerCategoryIcon, L"Container category icon", FALSE, 80, 0 },
    { PhDevicePropertyContainerCategoryGroupDesc, L"Container category group description", FALSE, 80, 0 },
    { PhDevicePropertyContainerCategoryGroupIcon, L"Container category group icon", FALSE, 80, 0 },
    { PhDevicePropertyContainerPrimaryCategory, L"Container primary category", FALSE, 80, 0 },
    { PhDevicePropertyContainerUnpairUninstall, L"Container unpair uninstall", FALSE, 80, 0 },
    { PhDevicePropertyContainerRequiresUninstallElevation, L"Container requires uninstall elevation", FALSE, 80, 0 },
    { PhDevicePropertyContainerDeviceFunctionSubRank, L"Container function sub-rank", FALSE, 80, 0 },
    { PhDevicePropertyContainerAlwaysShowDeviceAsConnected, L"Container always show connected", FALSE, 80, 0 },
    { PhDevicePropertyContainerConfigFlags, L"Container control flags", FALSE, 80, 0 },
    { PhDevicePropertyContainerPrivilegedPackageFamilyNames, L"Container privileged package family names", FALSE, 80, 0 },
    { PhDevicePropertyContainerCustomPrivilegedPackageFamilyNames, L"Container custom privileged package family names", FALSE, 80, 0 },
    { PhDevicePropertyContainerIsRebootRequired, L"Container reboot required", FALSE, 80, 0 },
    { PhDevicePropertyContainerFriendlyName, L"Container friendly name", FALSE, 80, 0 },
    { PhDevicePropertyContainerManufacturer, L"Container manufacture", FALSE, 80, 0 },
    { PhDevicePropertyContainerModelName, L"Container model name", FALSE, 80, 0 },
    { PhDevicePropertyContainerModelNumber, L"Container model number", FALSE, 80, 0 },
    { PhDevicePropertyContainerInstallInProgress, L"Container install in progress", FALSE, 80, 0 },

    { PhDevicePropertyObjectType, L"Object type", FALSE, 80, 0 },

    { PhDevicePropertyPciInterruptSupport, L"PCI interrupt support", FALSE, 80, 0 },
    { PhDevicePropertyPciExpressCapabilityControl, L"PCI express capability control", FALSE, 80, 0 },
    { PhDevicePropertyPciNativeExpressControl, L"PCI native express control", FALSE, 80, 0 },
    { PhDevicePropertyPciSystemMsiSupport, L"PCI system MSI support", FALSE, 80, 0 },

    { PhDevicePropertyStoragePortable, L"Storage portable", FALSE, 80, 0 },
    { PhDevicePropertyStorageRemovableMedia, L"Storage removable media", FALSE, 80, 0 },
    { PhDevicePropertyStorageSystemCritical, L"Storage system critical", FALSE, 80, 0 },
    { PhDevicePropertyStorageDiskNumber, L"Storage disk number", FALSE, 80, 0 },
    { PhDevicePropertyStoragePartitionNumber, L"Storage disk partition number", FALSE, 80, 0 },
};
C_ASSERT(RTL_NUMBER_OF(DeviceItemPropertyTable) == PhMaxDeviceProperty);
const ULONG DeviceItemPropertyTableCount = RTL_NUMBER_OF(DeviceItemPropertyTable);

VOID DevicesTreeInitialize(
    _In_ HWND TreeNewHandle
    )
{
    ULONG count = 0;

    DeviceTreeHandle = TreeNewHandle;

    PhSetControlTheme(DeviceTreeHandle, L"explorer");
    TreeNew_SetCallback(DeviceTreeHandle, DeviceTreeCallback, NULL);
    TreeNew_SetExtendedFlags(DeviceTreeHandle, TN_FLAG_ITEM_DRAG_SELECT, TN_FLAG_ITEM_DRAG_SELECT);
    SendMessage(TreeNew_GetTooltips(DeviceTreeHandle), TTM_SETDELAYTIME, TTDT_AUTOPOP, MAXSHORT);

    DevicesTreeImageListInitialize(DeviceTreeHandle);

    TreeNew_SetRedraw(DeviceTreeHandle, FALSE);

    for (ULONG i = 0; i < DeviceItemPropertyTableCount; i++)
    {
        ULONG displayIndex;
        const DEVICE_PROPERTY_TABLE_ENTRY* entry;

        entry = &DeviceItemPropertyTable[i];

        assert(i == entry->PropClass);

        if (entry->PropClass == PhDevicePropertyName)
        {
            assert(i == 0);
            displayIndex = -2;
        }
        else
        {
            assert(i > 0);
            displayIndex = i - 1;
        }

        PhAddTreeNewColumn(
            DeviceTreeHandle,
            entry->PropClass,
            entry->ColumnVisible,
            entry->ColumnName,
            entry->ColumnWidth,
            PH_ALIGN_LEFT,
            displayIndex,
            entry->ColumnTextFlags
            );
    }

    DevicesTreeLoadSettings(DeviceTreeHandle);

    TreeNew_SetRedraw(DeviceTreeHandle, TRUE);

    TreeNew_SetTriState(DeviceTreeHandle, TRUE);

    if (PhGetIntegerSetting(L"TreeListCustomRowSize"))
    {
        ULONG treelistCustomRowSize = PhGetIntegerSetting(L"TreeListCustomRowSize");

        if (treelistCustomRowSize < 15)
            treelistCustomRowSize = 15;

        TreeNew_SetRowHeight(DeviceTreeHandle, treelistCustomRowSize);
    }

    PhInitializeTreeNewFilterSupport(&DeviceTreeFilterSupport, DeviceTreeHandle, &DeviceFilterList);
    if (ToolStatusInterface)
    {
        PhRegisterCallback(
            ToolStatusInterface->SearchChangedEvent,
            DeviceTreeSearchChangedHandler,
            NULL,
            &SearchChangedRegistration);
        PhAddTreeNewFilter(&DeviceTreeFilterSupport, DeviceTreeFilterCallback, NULL);
    }

    if (PhGetIntegerSetting(L"EnableThemeSupport"))
    {
        PhInitializeWindowTheme(DeviceTreeHandle, TRUE);
        TreeNew_ThemeSupport(DeviceTreeHandle, TRUE);
    }
}

BOOLEAN DevicesTabPageCallback(
    _In_ struct _PH_MAIN_TAB_PAGE* Page,
    _In_ PH_MAIN_TAB_PAGE_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    switch (Message)
    {
    case MainTabPageCreateWindow:
        {
            HWND hwnd;
            ULONG thinRows;
            ULONG treelistBorder;
            ULONG treelistCustomColors;
            PH_TREENEW_CREATEPARAMS treelistCreateParams = { 0 };

            thinRows = PhGetIntegerSetting(L"ThinRows") ? TN_STYLE_THIN_ROWS : 0;
            treelistBorder = (PhGetIntegerSetting(L"TreeListBorderEnable") && !PhGetIntegerSetting(L"EnableThemeSupport")) ? WS_BORDER : 0;
            treelistCustomColors = PhGetIntegerSetting(L"TreeListCustomColorsEnable") ? TN_STYLE_CUSTOM_COLORS : 0;

            if (treelistCustomColors)
            {
                treelistCreateParams.TextColor = PhGetIntegerSetting(L"TreeListCustomColorText");
                treelistCreateParams.FocusColor = PhGetIntegerSetting(L"TreeListCustomColorFocus");
                treelistCreateParams.SelectionColor = PhGetIntegerSetting(L"TreeListCustomColorSelection");
            }

            hwnd = CreateWindow(
                PH_TREENEW_CLASSNAME,
                NULL,
                WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TN_STYLE_ICONS | TN_STYLE_DOUBLE_BUFFERED | TN_STYLE_ANIMATE_DIVIDER | thinRows | treelistBorder | treelistCustomColors,
                0,
                0,
                3,
                3,
                Parameter2,
                NULL,
                PluginInstance->DllBase,
                &treelistCreateParams
                );

            if (!hwnd)
                return FALSE;

            DeviceTabCreated = TRUE;

            DevicesTreeInitialize(hwnd);

            if (Parameter1)
            {
                *(HWND*)Parameter1 = hwnd;
            }
        }
        return TRUE;
    case MainTabPageLoadSettings:
        {
            NOTHING;
        }
        return TRUE;
    case MainTabPageSaveSettings:
        {
            DevicesTreeSaveSettings();
        }
        return TRUE;
    case MainTabPageSelected:
        {
            DeviceTabSelected = (BOOLEAN)Parameter1;
            if (DeviceTabSelected)
                DeviceTreePublishAsync(FALSE);
        }
        break;
    case MainTabPageFontChanged:
        {
            HFONT font = (HFONT)Parameter1;

            if (DeviceTreeHandle)
                SendMessage(DeviceTreeHandle, WM_SETFONT, (WPARAM)Parameter1, TRUE);
        }
        break;
    case MainTabPageDpiChanged:
        {
            if (DeviceImageList)
            {
                DevicesTreeImageListInitialize(DeviceTreeHandle);

                if (DeviceTree)
                {
                    for (ULONG i = 0; i < DeviceTree->Nodes->Count; i++)
                    {
                        PDEVICE_NODE node = DeviceTree->Nodes->Items[i];
                        HICON iconHandle = PhGetDeviceIcon(node->DeviceItem, &DeviceIconSize);
                        if (iconHandle)
                        {
                            node->IconIndex = PhImageListAddIcon(DeviceImageList, iconHandle);
                            DestroyIcon(iconHandle);
                        }
                        else
                        {
                            node->IconIndex = 0; // Must be reset (dmex)
                        }
                    }
                }
            }
        }
        break;
    }

    return FALSE;
}

VOID NTAPI ToolStatusActivateContent(
    _In_ BOOLEAN Select
    )
{
    SetFocus(DeviceTreeHandle);

    if (Select)
    {
        if (TreeNew_GetFlatNodeCount(DeviceTreeHandle) > 0)
        {
            PDEVICE_NODE node;

            TreeNew_DeselectRange(DeviceTreeHandle, 0, -1);

            node = (PDEVICE_NODE)TreeNew_GetFlatNode(DeviceTreeHandle, 0);

            if (!node->Node.Visible)
            {
                TreeNew_SetFocusNode(DeviceTreeHandle, &node->Node);
                TreeNew_SetMarkNode(DeviceTreeHandle, &node->Node);
                TreeNew_SelectRange(DeviceTreeHandle, node->Node.Index, node->Node.Index);
                TreeNew_EnsureVisible(DeviceTreeHandle, &node->Node);
            }
        }
    }
}

HWND NTAPI ToolStatusGetTreeNewHandle(
    VOID
    )
{
    return DeviceTreeHandle;
}

VOID NTAPI DeviceProviderCallbackHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (DeviceTabCreated && DeviceTabSelected && AutoRefreshDeviceTree)
        ProcessHacker_Invoke(DeviceTreePublish, DeviceTreeCreateIfNecessary(FALSE));
}

VOID DeviceTreeRemoveDeviceNode(
    _In_ PDEVICE_NODE Node,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

VOID NTAPI DeviceTreeProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (!DeviceTreeHandle)
        return;

    // piggy back off the processes update callback to handle state changes
    PH_TICK_SH_STATE_TN(
        DEVICE_NODE,
        ShState,
        DeviceNodeStateList,
        DeviceTreeRemoveDeviceNode,
        DeviceHighlightingDuration,
        DeviceTreeHandle,
        TRUE,
        NULL,
        NULL
        );
}

VOID DeviceTreeUpdateCachedSettings(
    _In_ BOOLEAN UpdateColors
    )
{
    AutoRefreshDeviceTree = !!PhGetIntegerSetting(SETTING_NAME_DEVICE_TREE_AUTO_REFRESH);
    ShowDisconnected = !!PhGetIntegerSetting(SETTING_NAME_DEVICE_TREE_SHOW_DISCONNECTED);
    ShowSoftwareComponents = !!PhGetIntegerSetting(SETTING_NAME_DEVICE_SHOW_SOFTWARE_COMPONENTS);
    ShowDeviceInterfaces = !!PhGetIntegerSetting(SETTING_NAME_DEVICE_SHOW_DEVICE_INTERFACES);
    ShowDisabledDeviceInterfaces = !!PhGetIntegerSetting(SETTING_NAME_DEVICE_SHOW_DISABLED_DEVICE_INTERFACES);
    HighlightUpperFiltered = !!PhGetIntegerSetting(SETTING_NAME_DEVICE_TREE_HIGHLIGHT_UPPER_FILTERED);
    HighlightLowerFiltered = !!PhGetIntegerSetting(SETTING_NAME_DEVICE_TREE_HIGHLIGHT_LOWER_FILTERED);
    DeviceHighlightingDuration = PhGetIntegerSetting(SETTING_NAME_DEVICE_HIGHLIGHTING_DURATION);

    if (UpdateColors)
    {
        DeviceProblemColor = PhGetIntegerSetting(SETTING_NAME_DEVICE_PROBLEM_COLOR);
        DeviceDisabledColor = PhGetIntegerSetting(SETTING_NAME_DEVICE_DISABLED_COLOR);
        DeviceDisconnectedColor = PhGetIntegerSetting(SETTING_NAME_DEVICE_DISCONNECTED_COLOR);
        DeviceHighlightColor = PhGetIntegerSetting(SETTING_NAME_DEVICE_HIGHLIGHT_COLOR);
        DeviceInterfaceColor = PhGetIntegerSetting(SETTING_NAME_DEVICE_INTERFACE_COLOR);
        DeviceDisabledInterfaceColor = PhGetIntegerSetting(SETTING_NAME_DEVICE_DISABLED_INTERFACE_COLOR);
        DeviceArrivedColor = PhGetIntegerSetting(SETTING_NAME_DEVICE_ARRIVED_COLOR);
    }
}

VOID NTAPI DeviceTreeSettingsUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    DeviceTreeUpdateCachedSettings(FALSE);
}

VOID InitializeDevicesTab(
    VOID
    )
{
    PH_MAIN_TAB_PAGE page;
    PPH_PLUGIN toolStatusPlugin;

    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackDeviceNotificationEvent),
        DeviceProviderCallbackHandler,
        NULL,
        &DeviceNotifyRegistration
        );
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackProcessesUpdated),
        DeviceTreeProcessesUpdatedCallback,
        NULL,
        &ProcessesUpdatedCallbackRegistration
        );
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackSettingsUpdated),
        DeviceTreeSettingsUpdatedCallback,
        NULL,
        &SettingsUpdatedCallbackRegistration
        );

    DeviceTreeUpdateCachedSettings(TRUE);

    RtlZeroMemory(&page, sizeof(PH_MAIN_TAB_PAGE));
    PhInitializeStringRef(&page.Name, L"Devices");
    page.Callback = DevicesTabPageCallback;

    DevicesAddedTabPage = PhPluginCreateTabPage(&page);

    if (toolStatusPlugin = PhFindPlugin(TOOLSTATUS_PLUGIN_NAME))
    {
        ToolStatusInterface = PhGetPluginInformation(toolStatusPlugin)->Interface;

        if (ToolStatusInterface->Version <= TOOLSTATUS_INTERFACE_VERSION)
        {
            PTOOLSTATUS_TAB_INFO tabInfo;

            tabInfo = ToolStatusInterface->RegisterTabInfo(DevicesAddedTabPage->Index);
            tabInfo->BannerText = L"Search Devices";
            tabInfo->ActivateContent = ToolStatusActivateContent;
            tabInfo->GetTreeNewHandle = ToolStatusGetTreeNewHandle;
        }
    }
}
