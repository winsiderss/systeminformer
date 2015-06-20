/*
 * Process Hacker -
 *   service list
 *
 * Copyright (C) 2010-2015 wj32
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

BOOLEAN PhpServiceNodeHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

ULONG PhpServiceNodeHashtableHashFunction(
    _In_ PVOID Entry
    );

VOID PhpRemoveServiceNode(
    _In_ PPH_SERVICE_NODE ServiceNode
    );

LONG PhpServiceTreeNewPostSortFunction(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    );

BOOLEAN NTAPI PhpServiceTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    );

static HWND ServiceTreeListHandle;
static ULONG ServiceTreeListSortColumn;
static PH_SORT_ORDER ServiceTreeListSortOrder;
static PH_CM_MANAGER ServiceTreeListCm;

static PPH_HASHTABLE ServiceNodeHashtable; // hashtable of all nodes
static PPH_LIST ServiceNodeList; // list of all nodes

static PH_TN_FILTER_SUPPORT FilterSupport;

static HICON ServiceApplicationIcon;
static HICON ServiceApplicationGoIcon;
static HICON ServiceCogIcon;
static HICON ServiceCogGoIcon;

BOOLEAN PhServiceTreeListStateHighlighting = TRUE;
static PPH_POINTER_LIST ServiceNodeStateList = NULL; // list of nodes which need to be processed

VOID PhServiceTreeListInitialization(
    VOID
    )
{
    ServiceNodeHashtable = PhCreateHashtable(
        sizeof(PPH_SERVICE_NODE),
        PhpServiceNodeHashtableCompareFunction,
        PhpServiceNodeHashtableHashFunction,
        100
        );
    ServiceNodeList = PhCreateList(100);
}

BOOLEAN PhpServiceNodeHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_SERVICE_NODE serviceNode1 = *(PPH_SERVICE_NODE *)Entry1;
    PPH_SERVICE_NODE serviceNode2 = *(PPH_SERVICE_NODE *)Entry2;

    return serviceNode1->ServiceItem == serviceNode2->ServiceItem;
}

ULONG PhpServiceNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashIntPtr((ULONG_PTR)(*(PPH_SERVICE_NODE *)Entry)->ServiceItem);
}

VOID PhInitializeServiceTreeList(
    _In_ HWND hwnd
    )
{
    ServiceApplicationIcon = PH_LOAD_SHARED_IMAGE(MAKEINTRESOURCE(IDI_PHAPPLICATION), IMAGE_ICON);
    ServiceApplicationGoIcon = PH_LOAD_SHARED_IMAGE(MAKEINTRESOURCE(IDI_PHAPPLICATIONGO), IMAGE_ICON);
    ServiceCogIcon = PH_LOAD_SHARED_IMAGE(MAKEINTRESOURCE(IDI_COG), IMAGE_ICON);
    ServiceCogGoIcon = PH_LOAD_SHARED_IMAGE(MAKEINTRESOURCE(IDI_COGGO), IMAGE_ICON);

    ServiceTreeListHandle = hwnd;
    PhSetControlTheme(ServiceTreeListHandle, L"explorer");
    SendMessage(TreeNew_GetTooltips(ServiceTreeListHandle), TTM_SETDELAYTIME, TTDT_AUTOPOP, MAXSHORT);

    TreeNew_SetCallback(hwnd, PhpServiceTreeNewCallback, NULL);

    TreeNew_SetRedraw(hwnd, FALSE);

    // Default columns
    PhAddTreeNewColumn(hwnd, PHSVTLC_NAME, TRUE, L"Name", 140, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_DISPLAYNAME, TRUE, L"Display Name", 220, PH_ALIGN_LEFT, 1, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_TYPE, TRUE, L"Type", 100, PH_ALIGN_LEFT, 2, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_STATUS, TRUE, L"Status", 70, PH_ALIGN_LEFT, 3, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_STARTTYPE, TRUE, L"Start Type", 130, PH_ALIGN_LEFT, 4, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_PID, TRUE, L"PID", 50, PH_ALIGN_RIGHT, 5, DT_RIGHT);

    PhAddTreeNewColumn(hwnd, PHSVTLC_BINARYPATH, FALSE, L"Binary Path", 180, PH_ALIGN_LEFT, -1, DT_PATH_ELLIPSIS);
    PhAddTreeNewColumn(hwnd, PHSVTLC_ERRORCONTROL, FALSE, L"Error Control", 70, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_GROUP, FALSE, L"Group", 100, PH_ALIGN_LEFT, -1, 0);
    PhAddTreeNewColumn(hwnd, PHSVTLC_DESCRIPTION, FALSE, L"Description", 200, PH_ALIGN_LEFT, -1, 0);

    TreeNew_SetRedraw(hwnd, TRUE);

    TreeNew_SetSort(hwnd, 0, AscendingSortOrder);

    PhCmInitializeManager(&ServiceTreeListCm, hwnd, PHSVTLC_MAXIMUM, PhpServiceTreeNewPostSortFunction);

    if (PhPluginsEnabled)
    {
        PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

        treeNewInfo.TreeNewHandle = hwnd;
        treeNewInfo.CmData = &ServiceTreeListCm;
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackServiceTreeNewInitializing), &treeNewInfo);
    }

    PhInitializeTreeNewFilterSupport(&FilterSupport, hwnd, ServiceNodeList);
}

VOID PhLoadSettingsServiceTreeList(
    VOID
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhGetStringSetting(L"ServiceTreeListColumns");
    sortSettings = PhGetStringSetting(L"ServiceTreeListSort");
    PhCmLoadSettingsEx(ServiceTreeListHandle, &ServiceTreeListCm, 0, &settings->sr, &sortSettings->sr);
    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PhSaveSettingsServiceTreeList(
    VOID
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhCmSaveSettingsEx(ServiceTreeListHandle, &ServiceTreeListCm, 0, &sortSettings);
    PhSetStringSetting2(L"ServiceTreeListColumns", &settings->sr);
    PhSetStringSetting2(L"ServiceTreeListSort", &sortSettings->sr);
    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

struct _PH_TN_FILTER_SUPPORT *PhGetFilterSupportServiceTreeList(
    VOID
    )
{
    return &FilterSupport;
}

PPH_SERVICE_NODE PhAddServiceNode(
    _In_ PPH_SERVICE_ITEM ServiceItem,
    _In_ ULONG RunId
    )
{
    PPH_SERVICE_NODE serviceNode;

    serviceNode = PhAllocate(PhEmGetObjectSize(EmServiceNodeType, sizeof(PH_SERVICE_NODE)));
    memset(serviceNode, 0, sizeof(PH_SERVICE_NODE));
    PhInitializeTreeNewNode(&serviceNode->Node);

    if (PhServiceTreeListStateHighlighting && RunId != 1)
    {
        PhChangeShStateTn(
            &serviceNode->Node,
            &serviceNode->ShState,
            &ServiceNodeStateList,
            NewItemState,
            PhCsColorNew,
            NULL
            );
    }

    serviceNode->ServiceItem = ServiceItem;
    PhReferenceObject(ServiceItem);

    memset(serviceNode->TextCache, 0, sizeof(PH_STRINGREF) * PHSVTLC_MAXIMUM);
    serviceNode->Node.TextCache = serviceNode->TextCache;
    serviceNode->Node.TextCacheSize = PHSVTLC_MAXIMUM;

    PhAddEntryHashtable(ServiceNodeHashtable, &serviceNode);
    PhAddItemList(ServiceNodeList, serviceNode);

    if (FilterSupport.FilterList)
        serviceNode->Node.Visible = PhApplyTreeNewFiltersToNode(&FilterSupport, &serviceNode->Node);

    PhEmCallObjectOperation(EmServiceNodeType, serviceNode, EmObjectCreate);

    TreeNew_NodesStructured(ServiceTreeListHandle);

    return serviceNode;
}

PPH_SERVICE_NODE PhFindServiceNode(
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    PH_SERVICE_NODE lookupServiceNode;
    PPH_SERVICE_NODE lookupServiceNodePtr = &lookupServiceNode;
    PPH_SERVICE_NODE *serviceNode;

    lookupServiceNode.ServiceItem = ServiceItem;

    serviceNode = (PPH_SERVICE_NODE *)PhFindEntryHashtable(
        ServiceNodeHashtable,
        &lookupServiceNodePtr
        );

    if (serviceNode)
        return *serviceNode;
    else
        return NULL;
}

VOID PhRemoveServiceNode(
    _In_ PPH_SERVICE_NODE ServiceNode
    )
{
    // Remove from the hashtable here to avoid problems in case the key is re-used.
    PhRemoveEntryHashtable(ServiceNodeHashtable, &ServiceNode);

    if (PhServiceTreeListStateHighlighting)
    {
        PhChangeShStateTn(
            &ServiceNode->Node,
            &ServiceNode->ShState,
            &ServiceNodeStateList,
            RemovingItemState,
            PhCsColorRemoved,
            ServiceTreeListHandle
            );
    }
    else
    {
        PhpRemoveServiceNode(ServiceNode);
    }
}

VOID PhpRemoveServiceNode(
    _In_ PPH_SERVICE_NODE ServiceNode
    )
{
    ULONG index;

    PhEmCallObjectOperation(EmServiceNodeType, ServiceNode, EmObjectDelete);

    // Remove from list and cleanup.

    if ((index = PhFindItemList(ServiceNodeList, ServiceNode)) != -1)
        PhRemoveItemList(ServiceNodeList, index);

    if (ServiceNode->BinaryPath) PhDereferenceObject(ServiceNode->BinaryPath);
    if (ServiceNode->LoadOrderGroup) PhDereferenceObject(ServiceNode->LoadOrderGroup);
    if (ServiceNode->Description) PhDereferenceObject(ServiceNode->Description);

    if (ServiceNode->TooltipText) PhDereferenceObject(ServiceNode->TooltipText);

    PhDereferenceObject(ServiceNode->ServiceItem);

    PhFree(ServiceNode);

    TreeNew_NodesStructured(ServiceTreeListHandle);
}

VOID PhUpdateServiceNode(
    _In_ PPH_SERVICE_NODE ServiceNode
    )
{
    memset(ServiceNode->TextCache, 0, sizeof(PH_STRINGREF) * PHSVTLC_MAXIMUM);
    PhClearReference(&ServiceNode->TooltipText);

    ServiceNode->ValidMask = 0;
    PhInvalidateTreeNewNode(&ServiceNode->Node, TN_CACHE_ICON);
    TreeNew_NodesStructured(ServiceTreeListHandle);
}

VOID PhTickServiceNodes(
    VOID
    )
{
    if (ServiceTreeListSortOrder != NoSortOrder && ServiceTreeListSortColumn >= PHSVTLC_MAXIMUM)
    {
        // Sorting is on, but it's not one of our columns. Force a rebuild. (If it was one of our
        // columns, the restructure would have been handled in PhUpdateServiceNode.)
        TreeNew_NodesStructured(ServiceTreeListHandle);
    }

    PH_TICK_SH_STATE_TN(PH_SERVICE_NODE, ShState, ServiceNodeStateList, PhpRemoveServiceNode, PhCsHighlightingDuration, ServiceTreeListHandle, TRUE, NULL);
}

static VOID PhpUpdateServiceNodeConfig(
    _Inout_ PPH_SERVICE_NODE ServiceNode
    )
{
    if (!(ServiceNode->ValidMask & PHSN_CONFIG))
    {
        SC_HANDLE serviceHandle;
        LPQUERY_SERVICE_CONFIG serviceConfig;

        if (serviceHandle = PhOpenService(ServiceNode->ServiceItem->Name->Buffer, SERVICE_QUERY_CONFIG))
        {
            if (serviceConfig = PhGetServiceConfig(serviceHandle))
            {
                if (serviceConfig->lpBinaryPathName)
                    PhMoveReference(&ServiceNode->BinaryPath, PhCreateString(serviceConfig->lpBinaryPathName));
                if (serviceConfig->lpLoadOrderGroup)
                    PhMoveReference(&ServiceNode->LoadOrderGroup, PhCreateString(serviceConfig->lpLoadOrderGroup));

                PhFree(serviceConfig);
            }

            CloseServiceHandle(serviceHandle);
        }

        ServiceNode->ValidMask |= PHSN_CONFIG;
    }
}

static VOID PhpUpdateServiceNodeDescription(
    _Inout_ PPH_SERVICE_NODE ServiceNode
    )
{
    if (!(ServiceNode->ValidMask & PHSN_DESCRIPTION))
    {
        SC_HANDLE serviceHandle;

        if (serviceHandle = PhOpenService(ServiceNode->ServiceItem->Name->Buffer, SERVICE_QUERY_CONFIG))
        {
            PhMoveReference(&ServiceNode->Description, PhGetServiceDescription(serviceHandle));

            CloseServiceHandle(serviceHandle);
        }

        ServiceNode->ValidMask |= PHSN_DESCRIPTION;
    }
}

#define SORT_FUNCTION(Column) PhpServiceTreeNewCompare##Column

#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PhpServiceTreeNewCompare##Column( \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPH_SERVICE_NODE node1 = *(PPH_SERVICE_NODE *)_elem1; \
    PPH_SERVICE_NODE node2 = *(PPH_SERVICE_NODE *)_elem2; \
    PPH_SERVICE_ITEM serviceItem1 = node1->ServiceItem; \
    PPH_SERVICE_ITEM serviceItem2 = node2->ServiceItem; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    /* if (sortResult == 0) */ \
        /* sortResult = PhCompareString(serviceItem1->Name, serviceItem2->Name, TRUE); */ \
    \
    return PhModifySort(sortResult, ServiceTreeListSortOrder); \
}

LONG PhpServiceTreeNewPostSortFunction(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    )
{
    return PhModifySort(Result, SortOrder);
}

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareString(serviceItem1->Name, serviceItem2->Name, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(DisplayName)
{
    sortResult = PhCompareStringWithNull(serviceItem1->DisplayName, serviceItem2->DisplayName, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Type)
{
    sortResult = intcmp(serviceItem1->Type, serviceItem2->Type);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Status)
{
    sortResult = intcmp(serviceItem1->State, serviceItem2->State);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(StartType)
{
    sortResult = intcmp(serviceItem1->StartType, serviceItem2->StartType);

    if (sortResult == 0)
        sortResult = intcmp(serviceItem1->DelayedStart, serviceItem2->DelayedStart);
    if (sortResult == 0)
        sortResult = intcmp(serviceItem1->HasTriggers, serviceItem2->HasTriggers);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Pid)
{
    sortResult = uintptrcmp((ULONG_PTR)serviceItem1->ProcessId, (ULONG_PTR)serviceItem2->ProcessId);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(BinaryPath)
{
    PhpUpdateServiceNodeConfig(node1);
    PhpUpdateServiceNodeConfig(node2);
    sortResult = PhCompareStringWithNull(node1->BinaryPath, node2->BinaryPath, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(ErrorControl)
{
    sortResult = intcmp(serviceItem1->ErrorControl, serviceItem2->ErrorControl);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Group)
{
    PhpUpdateServiceNodeConfig(node1);
    PhpUpdateServiceNodeConfig(node2);
    sortResult = PhCompareStringWithNull(node1->LoadOrderGroup, node2->LoadOrderGroup, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Description)
{
    PhpUpdateServiceNodeDescription(node1);
    PhpUpdateServiceNodeDescription(node2);
    sortResult = PhCompareStringWithNull(node1->Description, node2->Description, TRUE);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PhpServiceTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PPH_SERVICE_NODE node;

    if (PhCmForwardMessage(hwnd, Message, Parameter1, Parameter2, &ServiceTreeListCm))
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
                    SORT_FUNCTION(Name),
                    SORT_FUNCTION(DisplayName),
                    SORT_FUNCTION(Type),
                    SORT_FUNCTION(Status),
                    SORT_FUNCTION(StartType),
                    SORT_FUNCTION(Pid),
                    SORT_FUNCTION(BinaryPath),
                    SORT_FUNCTION(ErrorControl),
                    SORT_FUNCTION(Group),
                    SORT_FUNCTION(Description)
                };
                int (__cdecl *sortFunction)(const void *, const void *);

                if (!PhCmForwardSort(
                    (PPH_TREENEW_NODE *)ServiceNodeList->Items,
                    ServiceNodeList->Count,
                    ServiceTreeListSortColumn,
                    ServiceTreeListSortOrder,
                    &ServiceTreeListCm
                    ))
                {
                    if (ServiceTreeListSortColumn < PHSVTLC_MAXIMUM)
                        sortFunction = sortFunctions[ServiceTreeListSortColumn];
                    else
                        sortFunction = NULL;

                    if (sortFunction)
                    {
                        qsort(ServiceNodeList->Items, ServiceNodeList->Count, sizeof(PVOID), sortFunction);
                    }
                }

                getChildren->Children = (PPH_TREENEW_NODE *)ServiceNodeList->Items;
                getChildren->NumberOfChildren = ServiceNodeList->Count;
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
            PPH_SERVICE_ITEM serviceItem;

            node = (PPH_SERVICE_NODE)getCellText->Node;
            serviceItem = node->ServiceItem;

            switch (getCellText->Id)
            {
            case PHSVTLC_NAME:
                getCellText->Text = serviceItem->Name->sr;
                break;
            case PHSVTLC_DISPLAYNAME:
                getCellText->Text = serviceItem->DisplayName->sr;
                break;
            case PHSVTLC_TYPE:
                PhInitializeStringRefLongHint(&getCellText->Text, PhGetServiceTypeString(serviceItem->Type));
                break;
            case PHSVTLC_STATUS:
                PhInitializeStringRefLongHint(&getCellText->Text, PhGetServiceStateString(serviceItem->State));
                break;
            case PHSVTLC_STARTTYPE:
                {
                    PH_FORMAT format[2];
                    PWSTR additional = NULL;
                    SIZE_T returnLength;

                    PhInitFormatS(&format[0], PhGetServiceStartTypeString(serviceItem->StartType));

                    if (serviceItem->DelayedStart && serviceItem->HasTriggers)
                        additional = L" (Delayed, Trigger)";
                    else if (serviceItem->DelayedStart)
                        additional = L" (Delayed)";
                    else if (serviceItem->HasTriggers)
                        additional = L" (Trigger)";

                    if (additional)
                        PhInitFormatS(&format[1], additional);

                    if (PhFormatToBuffer(format, 1 + (additional ? 1 : 0), node->StartTypeText,
                        sizeof(node->StartTypeText), &returnLength))
                    {
                        getCellText->Text.Buffer = node->StartTypeText;
                        getCellText->Text.Length = returnLength - sizeof(WCHAR); // minus null terminator
                    }
                }
                break;
            case PHSVTLC_PID:
                PhInitializeStringRefLongHint(&getCellText->Text, serviceItem->ProcessIdString);
                break;
            case PHSVTLC_BINARYPATH:
                PhpUpdateServiceNodeConfig(node);
                getCellText->Text = PhGetStringRef(node->BinaryPath);
                break;
            case PHSVTLC_ERRORCONTROL:
                PhInitializeStringRefLongHint(&getCellText->Text, PhGetServiceErrorControlString(serviceItem->ErrorControl));
                break;
            case PHSVTLC_GROUP:
                PhpUpdateServiceNodeConfig(node);
                getCellText->Text = PhGetStringRef(node->LoadOrderGroup);
                break;
            case PHSVTLC_DESCRIPTION:
                PhpUpdateServiceNodeDescription(node);
                getCellText->Text = PhGetStringRef(node->Description);
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

            node = (PPH_SERVICE_NODE)getNodeIcon->Node;

            if (node->ServiceItem->Type == SERVICE_KERNEL_DRIVER || node->ServiceItem->Type == SERVICE_FILE_SYSTEM_DRIVER)
            {
                if (node->ServiceItem->State == SERVICE_RUNNING)
                    getNodeIcon->Icon = ServiceCogGoIcon;
                else
                    getNodeIcon->Icon = ServiceCogIcon;
            }
            else
            {
                if (node->ServiceItem->State == SERVICE_RUNNING)
                    getNodeIcon->Icon = ServiceApplicationGoIcon;
                else
                    getNodeIcon->Icon = ServiceApplicationIcon;
            }

            getNodeIcon->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetCellTooltip:
        {
            PPH_TREENEW_GET_CELL_TOOLTIP getCellTooltip = Parameter1;

            node = (PPH_SERVICE_NODE)getCellTooltip->Node;

            if (getCellTooltip->Column->Id != 0)
                return FALSE;

            if (!node->TooltipText)
                node->TooltipText = PhGetServiceTooltipText(node->ServiceItem);

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
            TreeNew_GetSort(hwnd, &ServiceTreeListSortColumn, &ServiceTreeListSortOrder);
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
                    SendMessage(PhMainWndHandle, WM_COMMAND, ID_SERVICE_COPY, 0);
                break;
            case 'A':
                if (GetKeyState(VK_CONTROL) < 0)
                    TreeNew_SelectRange(ServiceTreeListHandle, 0, -1);
                break;
            case VK_DELETE:
                SendMessage(PhMainWndHandle, WM_COMMAND, ID_SERVICE_DELETE, 0);
                break;
            case VK_RETURN:
                if (GetKeyState(VK_CONTROL) >= 0)
                    SendMessage(PhMainWndHandle, WM_COMMAND, ID_SERVICE_PROPERTIES, 0);
                else
                    SendMessage(PhMainWndHandle, WM_COMMAND, ID_SERVICE_OPENFILELOCATION, 0);
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
            SendMessage(PhMainWndHandle, WM_COMMAND, ID_SERVICE_PROPERTIES, 0);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenu = Parameter1;

            PhShowServiceContextMenu(contextMenu);
        }
        return TRUE;
    }

    return FALSE;
}

PPH_SERVICE_ITEM PhGetSelectedServiceItem(
    VOID
    )
{
    PPH_SERVICE_ITEM serviceItem = NULL;
    ULONG i;

    for (i = 0; i < ServiceNodeList->Count; i++)
    {
        PPH_SERVICE_NODE node = ServiceNodeList->Items[i];

        if (node->Node.Selected)
        {
            serviceItem = node->ServiceItem;
            break;
        }
    }

    return serviceItem;
}

VOID PhGetSelectedServiceItems(
    _Out_ PPH_SERVICE_ITEM **Services,
    _Out_ PULONG NumberOfServices
    )
{
    PPH_LIST list;
    ULONG i;

    list = PhCreateList(2);

    for (i = 0; i < ServiceNodeList->Count; i++)
    {
        PPH_SERVICE_NODE node = ServiceNodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, node->ServiceItem);
        }
    }

    *Services = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
    *NumberOfServices = list->Count;

    PhDereferenceObject(list);
}

VOID PhDeselectAllServiceNodes(
    VOID
    )
{
    TreeNew_DeselectRange(ServiceTreeListHandle, 0, -1);
}

VOID PhSelectAndEnsureVisibleServiceNode(
    _In_ PPH_SERVICE_NODE ServiceNode
    )
{
    PhDeselectAllServiceNodes();

    if (!ServiceNode->Node.Visible)
        return;

    TreeNew_SetFocusNode(ServiceTreeListHandle, &ServiceNode->Node);
    TreeNew_SetMarkNode(ServiceTreeListHandle, &ServiceNode->Node);
    TreeNew_SelectRange(ServiceTreeListHandle, ServiceNode->Node.Index, ServiceNode->Node.Index);
    TreeNew_EnsureVisible(ServiceTreeListHandle, &ServiceNode->Node);
}

VOID PhCopyServiceList(
    VOID
    )
{
    PPH_STRING text;

    text = PhGetTreeNewText(ServiceTreeListHandle, 0);
    PhSetClipboardString(ServiceTreeListHandle, &text->sr);
    PhDereferenceObject(text);
}

VOID PhWriteServiceList(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ ULONG Mode
    )
{
    PPH_LIST lines;
    ULONG i;

    lines = PhGetGenericTreeNewLines(ServiceTreeListHandle, Mode);

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
