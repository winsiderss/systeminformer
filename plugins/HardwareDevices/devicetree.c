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
#include "deviceprv.h"
#include <toolstatusintf.h>

#include <devguid.h>

static BOOLEAN AutoRefreshDeviceTree = TRUE;
static BOOLEAN ShowDisconnected = TRUE;
static BOOLEAN ShowSoftwareComponents = TRUE;
static BOOLEAN HighlightUpperFiltered = TRUE;
static BOOLEAN HighlightLowerFiltered = TRUE;
static ULONG DeviceProblemColor = 0;
static ULONG DeviceDisabledColor = 0;
static ULONG DeviceDisconnectedColor = 0;
static ULONG DeviceHighlightColor = 0;

static BOOLEAN DeviceTabCreated = FALSE;
static HWND DeviceTreeHandle = NULL;
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
static HCMNOTIFICATION DeviceNotification = NULL;
static HCMNOTIFICATION DeviceInterfaceNotification = NULL;

PDEVICE_TREE DevicesTabCreateDeviceTree(
    VOID
    )
{
    DEVICE_TREE_OPTIONS options;

    RtlZeroMemory(&options, sizeof(DEVICE_TREE_OPTIONS));

    options.IncludeSoftwareComponents = ShowSoftwareComponents;
    options.SortChildrenByName = !!PhGetIntegerSetting(SETTING_NAME_DEVICE_SORT_CHILDREN_BY_NAME);
    options.ImageList = DeviceImageList;
    options.IconSize = DeviceIconSize;
    options.FilterSupport = &DeviceTreeFilterSupport;

    return CreateDeviceTree(&options);
}

VOID NTAPI DeviceTreePublish(
    _In_ PDEVICE_TREE Tree
    )
{
    PDEVICE_TREE oldDeviceTree;

    TreeNew_SetRedraw(DeviceTreeHandle, FALSE);

    oldDeviceTree = DeviceTree;
    DeviceTree = Tree;

    DeviceFilterList.AllocatedCount = DeviceTree->DeviceList->AllocatedCount;
    DeviceFilterList.Count = DeviceTree->DeviceList->Count;
    DeviceFilterList.Items = DeviceTree->DeviceList->Items;

    TreeNew_SetRedraw(DeviceTreeHandle, TRUE);

    TreeNew_NodesStructured(DeviceTreeHandle);

    if (DeviceTreeFilterSupport.FilterList)
        PhApplyTreeNewFilters(&DeviceTreeFilterSupport);

    if (oldDeviceTree)
        PhDereferenceObject(oldDeviceTree);
}

NTSTATUS NTAPI DeviceTreePublishThread(
    _In_ PVOID Parameter
    )
{
    ProcessHacker_Invoke(DeviceTreePublish, DevicesTabCreateDeviceTree());

    return STATUS_SUCCESS;
}

VOID DeviceTreePublicAsync(
    VOID
    )
{
    PhCreateThread2(DeviceTreePublishThread, NULL);
}

_Function_class_(PCM_NOTIFY_CALLBACK)
ULONG CALLBACK CmNotifyCallback(
    _In_ HCMNOTIFICATION hNotify,
    _In_opt_ PVOID Context,
    _In_ CM_NOTIFY_ACTION Action,
    _In_reads_bytes_(EventDataSize) PCM_NOTIFY_EVENT_DATA EventData,
    _In_ ULONG EventDataSize
    )
{
    // TODO(jxy-s) expand upon deviceprv to handle device notifications itself, have it follow the provider model
    if (Action == CM_NOTIFY_ACTION_DEVICEINSTANCESTARTED)
        HandleDeviceNotify(FALSE, EventData->u.DeviceInstance.InstanceId);
    else if (Action == CM_NOTIFY_ACTION_DEVICEINSTANCEREMOVED)
        HandleDeviceNotify(TRUE, EventData->u.DeviceInstance.InstanceId);

    if (DeviceTabCreated && DeviceTabSelected && AutoRefreshDeviceTree)
        ProcessHacker_Invoke(DeviceTreePublish, DevicesTabCreateDeviceTree());

    return ERROR_SUCCESS;
}

VOID InvalidateDeviceTree(
    _In_ PDEVICE_TREE Tree
    )
{
    for (ULONG i = 0; i < Tree->DeviceList->Count; i++)
    {
        PDEVICE_NODE node;

        node = Tree->DeviceList->Items[i];

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

    if (!ShowDisconnected && (node->ProblemCode == CM_PROB_PHANTOM))
        return FALSE;

    if (PhIsNullOrEmptyString(ToolStatusInterface->GetSearchboxText()))
        return TRUE;

    for (ULONG i = 0; i < ARRAYSIZE(node->Properties); i++)
    {
        PDEVNODE_PROP prop;

        prop = GetDeviceNodeProperty(node, i);

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
    PDEVNODE_PROP lhs;
    PDEVNODE_PROP rhs;
    PH_STRINGREF srl;
    PH_STRINGREF srr;

    sortResult = 0;
    lhsNode = *(PDEVICE_NODE*)Left;
    rhsNode = *(PDEVICE_NODE*)Right;
    lhs = GetDeviceNodeProperty(lhsNode, DeviceTreeSortColumn);
    rhs = GetDeviceNodeProperty(rhsNode, DeviceTreeSortColumn);

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
        case DevPropTypeString:
            sortResult = PhCompareString(lhs->String, rhs->String, TRUE);
            break;
        case DevPropTypeUInt64:
            sortResult = uint64cmp(lhs->UInt64, rhs->UInt64);
            break;
        case DevPropTypeUInt32:
            sortResult = uint64cmp(lhs->UInt32, rhs->UInt32);
            break;
        case DevPropTypeInt32:
        case DevPropTypeNTSTATUS:
            sortResult = int64cmp(lhs->Int32, rhs->Int32);
            break;
        case DevPropTypeGUID:
            sortResult = memcmp(&lhs->Guid, &rhs->Guid, sizeof(GUID));
            break;
        case DevPropTypeBoolean:
            {
                if (lhs->Boolean && !rhs->Boolean)
                    sortResult = 1;
                else if (!lhs->Boolean && rhs->Boolean)
                    sortResult = -1;
                else
                    sortResult = 0;
            }
            break;
        case DevPropTypeTimeStamp:
            sortResult = int64cmp(lhs->TimeStamp.QuadPart, rhs->TimeStamp.QuadPart);
            break;
        case DevPropTypeStringList:
        {
            srl = PhGetStringRef(lhs->AsString);
            srr = PhGetStringRef(rhs->AsString);
            sortResult = PhCompareStringRef(&srl, &srr, TRUE);
            break;
        }
        default:
            assert(FALSE);
        }
    }

    if (sortResult == 0)
    {
        srl = PhGetStringRef(lhsNode->Properties[DevKeyName].AsString);
        srr = PhGetStringRef(rhsNode->Properties[DevKeyName].AsString);
        sortResult = PhCompareStringRef(&srl, &srr, TRUE);
    }

    return PhModifySort(sortResult, DeviceTreeSortOrder);
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
                        if (PhGetIntegerSetting(SETTING_NAME_DEVICE_SHOW_ROOT))
                        {
                            getChildren->Children = (PPH_TREENEW_NODE*)DeviceTree->DeviceRootList->Items;
                            getChildren->NumberOfChildren = DeviceTree->DeviceRootList->Count;
                        }
                        else if (DeviceTree->DeviceRootList->Count)
                        {
                            PDEVICE_NODE root;

                            root = DeviceTree->DeviceRootList->Items[0];

                            getChildren->Children = (PPH_TREENEW_NODE*)root->Children->Items;
                            getChildren->NumberOfChildren = root->Children->Count;
                        }
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
                        if (DeviceTreeSortColumn < MaxDevKey)
                        {
                            qsort(
                                DeviceTree->DeviceList->Items,
                                DeviceTree->DeviceList->Count,
                                sizeof(PVOID),
                                DeviceTreeSortFunction
                                );
                        }
                    }

                    getChildren->Children = (PPH_TREENEW_NODE*)DeviceTree->DeviceList->Items;
                    getChildren->NumberOfChildren = DeviceTree->DeviceList->Count;
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

            PPH_STRING text = GetDeviceNodeProperty(node, getCellText->Id)->AsString;

            getCellText->Text = PhGetStringRef(text);
            getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = Parameter1;
            node = (PDEVICE_NODE)getNodeColor->Node;

            getNodeColor->Flags = TN_CACHE | TN_AUTO_FORECOLOR;

            if (node->DevNodeStatus & DN_HAS_PROBLEM && (node->ProblemCode != CM_PROB_DISABLED))
            {
                getNodeColor->BackColor = DeviceProblemColor;
            }
            else if ((node->Capabilities & CM_DEVCAP_HARDWAREDISABLED) || (node->ProblemCode == CM_PROB_DISABLED))
            {
                getNodeColor->BackColor = DeviceDisabledColor;
            }
            else if (node->ProblemCode == CM_PROB_PHANTOM)
            {
                getNodeColor->BackColor = DeviceDisconnectedColor;
            }
            else if ((HighlightUpperFiltered && node->HasUpperFilters) || (HighlightLowerFiltered && node->HasLowerFilters))
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
            PhInsertEMenuItem(menu, highlightUpperFiltered = PhCreateEMenuItem(0, 104, L"Highlight upper filtered", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, highlightLowerFiltered = PhCreateEMenuItem(0, 105, L"Highlight lower filtered", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
            PhInsertEMenuItem(menu, gotoServiceItem = PhCreateEMenuItem(0, 106, L"Go to service...", NULL, NULL), ULONG_MAX);
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

            if (!node)
            {
                PhSetDisabledEMenuItem(gotoServiceItem);
                PhSetDisabledEMenuItem(subMenu);
                PhSetDisabledEMenuItem(properties);
            }

            if (gotoServiceItem && node)
            {
                PPH_STRING serviceName = GetDeviceNodeProperty(node, DevKeyService)->AsString;

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
                        if (node->InstanceId)
                            republish = HardwareDeviceEnableDisable(hwnd, node->InstanceId, selectedItem->Id == 0);
                        break;
                    case 2:
                        if (node->InstanceId)
                            republish = HardwareDeviceRestart(hwnd, node->InstanceId);
                        break;
                    case 3:
                        if (node->InstanceId)
                            republish = HardwareDeviceUninstall(hwnd, node->InstanceId);
                        break;
                    case HW_KEY_INDEX_HARDWARE:
                    case HW_KEY_INDEX_SOFTWARE:
                    case HW_KEY_INDEX_USER:
                    case HW_KEY_INDEX_CONFIG:
                        if (node->InstanceId)
                            HardwareDeviceOpenKey(hwnd, node->InstanceId, selectedItem->Id);
                        break;
                    case 10:
                        if (node->InstanceId)
                            HardwareDeviceShowProperties(hwnd, node->InstanceId);
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
                        PhSetIntegerSetting(SETTING_NAME_DEVICE_TREE_AUTO_REFRESH, ShowDisconnected);
                        break;
                    case 102:
                        ShowDisconnected = !ShowDisconnected;
                        PhSetIntegerSetting(SETTING_NAME_DEVICE_TREE_SHOW_DISCONNECTED, ShowDisconnected);
                        invalidate = TRUE;
                        break;
                    case 103:
                        ShowSoftwareComponents = !ShowSoftwareComponents;
                        PhSetIntegerSetting(SETTING_NAME_DEVICE_SHOW_SOFTWARE_COMPONENTS, ShowSoftwareComponents);
                        republish = TRUE;
                        break;
                    case 104:
                        HighlightUpperFiltered = !HighlightUpperFiltered;
                        PhSetIntegerSetting(SETTING_NAME_DEVICE_TREE_HIGHLIGHT_UPPER_FILTERED, HighlightUpperFiltered);
                        invalidate = TRUE;
                        break;
                    case 105:
                        HighlightLowerFiltered = !HighlightLowerFiltered;
                        PhSetIntegerSetting(SETTING_NAME_DEVICE_TREE_HIGHLIGHT_LOWER_FILTERED, HighlightLowerFiltered);
                        invalidate = TRUE;
                        break;
                    case 106:
                        {
                            PPH_STRING serviceName = GetDeviceNodeProperty(node, DevKeyService)->AsString;
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
                DeviceTreePublicAsync();
            }
            else if (invalidate)
            {
                InvalidateDeviceTree(DeviceTree);
                if (DeviceTreeFilterSupport.FilterList)
                    PhApplyTreeNewFilters(&DeviceTreeFilterSupport);
            }
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            PPH_TREENEW_MOUSE_EVENT mouseEvent = Parameter1;
            node = (PDEVICE_NODE)mouseEvent->Node;

            if (node->InstanceId)
                HardwareDeviceShowProperties(hwnd, node->InstanceId);
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

VOID DevicesTreeInitialize(
    _In_ HWND TreeNewHandle
    )
{
    DeviceTreeHandle = TreeNewHandle;

    PhSetControlTheme(DeviceTreeHandle, L"explorer");
    TreeNew_SetCallback(DeviceTreeHandle, DeviceTreeCallback, NULL);
    TreeNew_SetExtendedFlags(DeviceTreeHandle, TN_FLAG_ITEM_DRAG_SELECT, TN_FLAG_ITEM_DRAG_SELECT);
    SendMessage(TreeNew_GetTooltips(DeviceTreeHandle), TTM_SETDELAYTIME, TTDT_AUTOPOP, MAXSHORT);

    DevicesTreeImageListInitialize(DeviceTreeHandle);

    TreeNew_SetRedraw(DeviceTreeHandle, FALSE);

    for (ULONG i = 0; i < DevNodePropTableCount; i++)
    {
        ULONG displayIndex;
        PDEVNODE_PROP_TABLE_ENTRY entry;

        entry = &DevNodePropTable[i];

        assert(i == entry->NodeKey);

        if (entry->NodeKey == DevKeyName)
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
            entry->NodeKey,
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
                PhMainWndHandle,
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
                DeviceTreePublicAsync();
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

                if (DeviceTree && DeviceTree->DeviceList)
                {
                    for (ULONG i = 0; i < DeviceTree->DeviceList->Count; i++)
                    {
                        PDEVICE_NODE node = DeviceTree->DeviceList->Items[i];
                        HICON iconHandle;

                        if (SetupDiLoadDeviceIcon(
                            node->DeviceInfoHandle,
                            &node->DeviceInfoData,
                            DeviceIconSize.X,
                            DeviceIconSize.X,
                            0,
                            &iconHandle
                            ))
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

VOID InitializeDevicesTab(
    VOID
    )
{
    PH_MAIN_TAB_PAGE page;
    PPH_PLUGIN toolStatusPlugin;
    CM_NOTIFY_FILTER cmFilter;

    AutoRefreshDeviceTree = !!PhGetIntegerSetting(SETTING_NAME_DEVICE_TREE_AUTO_REFRESH);
    ShowDisconnected = !!PhGetIntegerSetting(SETTING_NAME_DEVICE_TREE_SHOW_DISCONNECTED);
    ShowSoftwareComponents = !!PhGetIntegerSetting(SETTING_NAME_DEVICE_SHOW_SOFTWARE_COMPONENTS);
    HighlightUpperFiltered = !!PhGetIntegerSetting(SETTING_NAME_DEVICE_TREE_HIGHLIGHT_UPPER_FILTERED);
    HighlightLowerFiltered = !!PhGetIntegerSetting(SETTING_NAME_DEVICE_TREE_HIGHLIGHT_LOWER_FILTERED);
    DeviceProblemColor = PhGetIntegerSetting(SETTING_NAME_DEVICE_PROBLEM_COLOR);
    DeviceDisabledColor = PhGetIntegerSetting(SETTING_NAME_DEVICE_DISABLED_COLOR);
    DeviceDisconnectedColor = PhGetIntegerSetting(SETTING_NAME_DEVICE_DISCONNECTED_COLOR);
    DeviceHighlightColor = PhGetIntegerSetting(SETTING_NAME_DEVICE_HIGHLIGHT_COLOR);

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

    RtlZeroMemory(&cmFilter, sizeof(CM_NOTIFY_FILTER));
    cmFilter.cbSize = sizeof(CM_NOTIFY_FILTER);

    cmFilter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINSTANCE;
    cmFilter.Flags = CM_NOTIFY_FILTER_FLAG_ALL_DEVICE_INSTANCES;
    if (CM_Register_Notification(
        &cmFilter,
        NULL,
        CmNotifyCallback,
        &DeviceNotification
        ) != CR_SUCCESS)
    {
        DeviceNotification = NULL;
    }

    cmFilter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
    cmFilter.Flags = CM_NOTIFY_FILTER_FLAG_ALL_INTERFACE_CLASSES;
    if (CM_Register_Notification(
        &cmFilter,
        NULL,
        CmNotifyCallback,
        &DeviceInterfaceNotification
        ) != CR_SUCCESS)
    {
        DeviceInterfaceNotification = NULL;
    }
}
