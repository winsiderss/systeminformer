/*
 * Process Hacker - 
 *   service list
 * 
 * Copyright (C) 2010 wj32
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
#include <phplug.h>

BOOLEAN PhpServiceNodeHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    );

ULONG PhpServiceNodeHashtableHashFunction(
    __in PVOID Entry
    );

VOID PhpRemoveServiceNode(
    __in PPH_SERVICE_NODE ServiceNode
    );

BOOLEAN NTAPI PhpServiceTreeListCallback(
    __in HWND hwnd,
    __in PH_TREELIST_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in_opt PVOID Context
    );

static HWND ServiceTreeListHandle;
static ULONG ServiceTreeListSortColumn;
static PH_SORT_ORDER ServiceTreeListSortOrder;

static PPH_HASHTABLE ServiceNodeHashtable; // hashtable of all nodes
static PPH_LIST ServiceNodeList; // list of all nodes

static HICON ServiceApplicationIcon;
static HICON ServiceCogIcon;

BOOLEAN PhServiceTreeListStateHighlighting = TRUE;
static PPH_POINTER_LIST ServiceNodeStateList = NULL; // list of nodes which need to be processed

VOID PhServiceTreeListInitialization()
{
    ServiceNodeHashtable = PhCreateHashtable(
        sizeof(PPH_SERVICE_NODE),
        PhpServiceNodeHashtableCompareFunction,
        PhpServiceNodeHashtableHashFunction,
        100
        );
    ServiceNodeList = PhCreateList(100);

    ServiceApplicationIcon = PH_LOAD_SHARED_IMAGE(MAKEINTRESOURCE(IDI_PHAPPLICATION), IMAGE_ICON);
    ServiceCogIcon = PH_LOAD_SHARED_IMAGE(MAKEINTRESOURCE(IDI_COG), IMAGE_ICON);
}

FORCEINLINE VOID PhpEnsureServiceNodeStateListCreated()
{
    if (!ServiceNodeStateList)
        ServiceNodeStateList = PhCreatePointerList(4);
}

BOOLEAN PhpServiceNodeHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    PPH_SERVICE_NODE serviceNode1 = *(PPH_SERVICE_NODE *)Entry1;
    PPH_SERVICE_NODE serviceNode2 = *(PPH_SERVICE_NODE *)Entry2;

    return serviceNode1->ServiceItem == serviceNode2->ServiceItem;
}

ULONG PhpServiceNodeHashtableHashFunction(
    __in PVOID Entry
    )
{
#ifdef _M_IX86
    return PhHashInt32((ULONG)(*(PPH_SERVICE_NODE *)Entry)->ServiceItem);
#else
    return PhHashInt64((ULONG64)(*(PPH_SERVICE_NODE *)Entry)->ServiceItem);
#endif
}

VOID PhInitializeServiceTreeList(
    __in HWND hwnd
    )
{
    ServiceTreeListHandle = hwnd;
    SendMessage(ServiceTreeListHandle, WM_SETFONT, (WPARAM)PhIconTitleFont, FALSE);
    TreeList_EnableExplorerStyle(hwnd);

    TreeList_SetCallback(hwnd, PhpServiceTreeListCallback);

    // Default columns
    PhAddTreeListColumn(hwnd, PHSVTLC_NAME, TRUE, L"Name", 100, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeListColumn(hwnd, PHSVTLC_DISPLAYNAME, TRUE, L"Display Name", 180, PH_ALIGN_LEFT, 1, 0);
    PhAddTreeListColumn(hwnd, PHSVTLC_TYPE, TRUE, L"Type", 110, PH_ALIGN_LEFT, 2, 0);
    PhAddTreeListColumn(hwnd, PHSVTLC_STATUS, TRUE, L"Status", 70, PH_ALIGN_LEFT, 3, 0);
    PhAddTreeListColumn(hwnd, PHSVTLC_STARTTYPE, TRUE, L"Start Type", 100, PH_ALIGN_LEFT, 4, 0);
    PhAddTreeListColumn(hwnd, PHSVTLC_PID, TRUE, L"PID", 50, PH_ALIGN_RIGHT, 5, DT_RIGHT);

    PhAddTreeListColumn(hwnd, PHSVTLC_BINARYPATH, FALSE, L"Binary Path", 180, PH_ALIGN_LEFT, 6, DT_PATH_ELLIPSIS);
    PhAddTreeListColumn(hwnd, PHSVTLC_ERRORCONTROL, FALSE, L"Error Control", 70, PH_ALIGN_LEFT, 7, 0);
    PhAddTreeListColumn(hwnd, PHSVTLC_GROUP, FALSE, L"Group", 100, PH_ALIGN_LEFT, 7, 0);

    TreeList_SetSort(hwnd, 0, AscendingSortOrder);
}

VOID PhLoadSettingsServiceTreeList()
{
    ULONG sortColumn;
    PH_SORT_ORDER sortOrder;

    PhLoadTreeListColumnsFromSetting(L"ServiceTreeListColumns", ServiceTreeListHandle);

    sortColumn = PhGetIntegerSetting(L"ServiceTreeListSortColumn");
    sortOrder = PhGetIntegerSetting(L"ServiceTreeListSortOrder");
    TreeList_SetSort(ServiceTreeListHandle, sortColumn, sortOrder);
}

VOID PhSaveSettingsServiceTreeList()
{
    ULONG sortColumn;
    PH_SORT_ORDER sortOrder;

    PhSaveTreeListColumnsToSetting(L"ServiceTreeListColumns", ServiceTreeListHandle);

    TreeList_GetSort(ServiceTreeListHandle, &sortColumn, &sortOrder);
    PhSetIntegerSetting(L"ServiceTreeListSortColumn", sortColumn);
    PhSetIntegerSetting(L"ServiceTreeListSortOrder", sortOrder);
}

PPH_SERVICE_NODE PhAddServiceNode(
    __in PPH_SERVICE_ITEM ServiceItem,
    __in ULONG RunId
    )
{
    PPH_SERVICE_NODE serviceNode;

    serviceNode = PhAllocate(sizeof(PH_SERVICE_NODE));
    memset(serviceNode, 0, sizeof(PH_SERVICE_NODE));
    PhInitializeTreeListNode(&serviceNode->Node);

    if (PhServiceTreeListStateHighlighting && RunId != 1)
    {
        PhpEnsureServiceNodeStateListCreated();
        serviceNode->TickCount = GetTickCount();
        serviceNode->Node.UseTempBackColor = TRUE;
        serviceNode->Node.TempBackColor = PhCsColorNew;
        serviceNode->StateListHandle = PhAddItemPointerList(ServiceNodeStateList, serviceNode);
        serviceNode->State = NewItemState;
    }
    else
    {
        serviceNode->State = NormalItemState;
    }

    serviceNode->ServiceItem = ServiceItem;
    PhReferenceObject(ServiceItem);

    memset(serviceNode->TextCache, 0, sizeof(PH_STRINGREF) * PHSVTLC_MAXIMUM);
    serviceNode->Node.TextCache = serviceNode->TextCache;
    serviceNode->Node.TextCacheSize = PHSVTLC_MAXIMUM;

    PhAddEntryHashtable(ServiceNodeHashtable, &serviceNode);
    PhAddItemList(ServiceNodeList, serviceNode);

    TreeList_NodesStructured(ServiceTreeListHandle);

    return serviceNode;
}

PPH_SERVICE_NODE PhFindServiceNode(
    __in PPH_SERVICE_ITEM ServiceItem
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
    __in PPH_SERVICE_NODE ServiceNode
    )
{
    if (PhServiceTreeListStateHighlighting)
    {
        PhpEnsureServiceNodeStateListCreated();
        ServiceNode->TickCount = GetTickCount();
        ServiceNode->Node.UseTempBackColor = TRUE;
        ServiceNode->Node.TempBackColor = PhCsColorRemoved;
        if (ServiceNode->State == NormalItemState)
            ServiceNode->StateListHandle = PhAddItemPointerList(ServiceNodeStateList, ServiceNode);
        ServiceNode->State = RemovingItemState;

        TreeList_UpdateNode(ServiceTreeListHandle, &ServiceNode->Node);
    }
    else
    {
        PhpRemoveServiceNode(ServiceNode);
    }
}

VOID PhpRemoveServiceNode(
    __in PPH_SERVICE_NODE ServiceNode
    )
{
    ULONG index;

    // Remove from hashtable/list and cleanup.

    PhRemoveEntryHashtable(ServiceNodeHashtable, &ServiceNode);

    if ((index = PhFindItemList(ServiceNodeList, ServiceNode)) != -1)
        PhRemoveItemList(ServiceNodeList, index);

    if (ServiceNode->BinaryPath) PhDereferenceObject(ServiceNode->BinaryPath);
    if (ServiceNode->LoadOrderGroup) PhDereferenceObject(ServiceNode->LoadOrderGroup);

    if (ServiceNode->TooltipText) PhDereferenceObject(ServiceNode->TooltipText);

    PhDereferenceObject(ServiceNode->ServiceItem);

    PhFree(ServiceNode);

    TreeList_NodesStructured(ServiceTreeListHandle);
}

VOID PhUpdateServiceNode(
    __in PPH_SERVICE_NODE ServiceNode
    )
{
    memset(ServiceNode->TextCache, 0, sizeof(PH_STRINGREF) * PHSVTLC_MAXIMUM);
    PhSwapReference(&ServiceNode->TooltipText, NULL);

    ServiceNode->ValidMask = 0;
    PhInvalidateTreeListNode(&ServiceNode->Node, TLIN_COLOR | TLIN_ICON);
    TreeList_NodesStructured(ServiceTreeListHandle);
}

VOID PhTickServiceNodes()
{
    // State highlighting
    if (ServiceNodeStateList && ServiceNodeStateList->Count != 0)
    {
        PPH_SERVICE_NODE node;
        ULONG enumerationKey = 0;
        ULONG tickCount;
        HANDLE stateListHandle;
        BOOLEAN redrawDisabled = FALSE;
        BOOLEAN changed = FALSE;

        tickCount = GetTickCount();

        while (PhEnumPointerList(ServiceNodeStateList, &enumerationKey, &node))
        {
            if (PhRoundNumber(tickCount - node->TickCount, 100) < PhCsHighlightingDuration)
                continue;

            stateListHandle = node->StateListHandle;

            if (node->State == NewItemState)
            {
                node->State = NormalItemState;
                node->Node.UseTempBackColor = FALSE;
                changed = TRUE;
            }
            else if (node->State == RemovingItemState)
            {
                if (!redrawDisabled)
                {
                    TreeList_SetRedraw(ServiceTreeListHandle, FALSE);
                    redrawDisabled = TRUE;
                }

                PhpRemoveServiceNode(node);
                changed = TRUE;
            }

            PhRemoveItemPointerList(ServiceNodeStateList, stateListHandle);
        }

        if (redrawDisabled)
            TreeList_SetRedraw(ServiceTreeListHandle, TRUE);
        if (changed)
            InvalidateRect(ServiceTreeListHandle, NULL, FALSE);
    }
}

static VOID PhpUpdateServiceNodeConfig(
    __inout PPH_SERVICE_NODE ServiceNode
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
                    PhSwapReference2(&ServiceNode->BinaryPath, PhCreateString(serviceConfig->lpBinaryPathName));
                if (serviceConfig->lpLoadOrderGroup)
                    PhSwapReference2(&ServiceNode->LoadOrderGroup, PhCreateString(serviceConfig->lpLoadOrderGroup));

                PhFree(serviceConfig);
            }

            CloseServiceHandle(serviceHandle);
        }

        ServiceNode->ValidMask |= PHSN_CONFIG;
    }
}

#define SORT_FUNCTION(Column) PhpServiceTreeListCompare##Column

#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PhpServiceTreeListCompare##Column( \
    __in const void *_elem1, \
    __in const void *_elem2 \
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

BOOLEAN NTAPI PhpServiceTreeListCallback(
    __in HWND hwnd,
    __in PH_TREELIST_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in_opt PVOID Context
    )
{
    PPH_SERVICE_NODE node;

    switch (Message)
    {
    case TreeListGetChildren:
        {
            PPH_TREELIST_GET_CHILDREN getChildren = Parameter1;

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
                    SORT_FUNCTION(Group)
                };
                int (__cdecl *sortFunction)(const void *, const void *);

                if (ServiceTreeListSortColumn < PHSVTLC_MAXIMUM)
                    sortFunction = sortFunctions[ServiceTreeListSortColumn];
                else
                    sortFunction = NULL;

                if (sortFunction)
                {
                    // Don't use PhSortList to avoid overhead.
                    qsort(ServiceNodeList->Items, ServiceNodeList->Count, sizeof(PVOID), sortFunction);
                }

                getChildren->Children = (PPH_TREELIST_NODE *)ServiceNodeList->Items;
                getChildren->NumberOfChildren = ServiceNodeList->Count;
            }
        }
        return TRUE;
    case TreeListIsLeaf:
        {
            PPH_TREELIST_IS_LEAF isLeaf = Parameter1;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeListGetNodeText:
        {
            PPH_TREELIST_GET_NODE_TEXT getNodeText = Parameter1;
            PPH_SERVICE_ITEM serviceItem;

            node = (PPH_SERVICE_NODE)getNodeText->Node;
            serviceItem = node->ServiceItem;

            switch (getNodeText->Id)
            {
            case PHSVTLC_NAME:
                getNodeText->Text = serviceItem->Name->sr;
                break;
            case PHSVTLC_DISPLAYNAME:
                getNodeText->Text = serviceItem->DisplayName->sr;
                break;
            case PHSVTLC_TYPE:
                PhInitializeStringRef(&getNodeText->Text, PhGetServiceTypeString(serviceItem->Type));
                break;
            case PHSVTLC_STATUS:
                PhInitializeStringRef(&getNodeText->Text, PhGetServiceStateString(serviceItem->State));
                break;
            case PHSVTLC_STARTTYPE:
                PhInitializeStringRef(&getNodeText->Text, PhGetServiceStartTypeString(serviceItem->StartType));
                break;
            case PHSVTLC_PID:
                PhInitializeStringRef(&getNodeText->Text, serviceItem->ProcessIdString);
                break;
            case PHSVTLC_BINARYPATH:
                PhpUpdateServiceNodeConfig(node);
                getNodeText->Text = PhGetStringRef(node->BinaryPath);
                break;
            case PHSVTLC_ERRORCONTROL:
                PhInitializeStringRef(&getNodeText->Text, PhGetServiceErrorControlString(serviceItem->ErrorControl));
                break;
            case PHSVTLC_GROUP:
                PhpUpdateServiceNodeConfig(node);
                getNodeText->Text = PhGetStringRef(node->LoadOrderGroup);
                break;
            default:
                return FALSE;
            }

            getNodeText->Flags = TLC_CACHE;
        }
        return TRUE;
    case TreeListGetNodeIcon:
        {
            PPH_TREELIST_GET_NODE_ICON getNodeIcon = Parameter1;

            node = (PPH_SERVICE_NODE)getNodeIcon->Node;

            if (node->ServiceItem->Type == SERVICE_KERNEL_DRIVER || node->ServiceItem->Type == SERVICE_FILE_SYSTEM_DRIVER)
                getNodeIcon->Icon = ServiceCogIcon;
            else
                getNodeIcon->Icon = ServiceApplicationIcon;

            getNodeIcon->Flags = TLC_CACHE;
        }
        return TRUE;
    case TreeListGetNodeTooltip:
        {
            PPH_TREELIST_GET_NODE_TOOLTIP getNodeTooltip = Parameter1;

            node = (PPH_SERVICE_NODE)getNodeTooltip->Node;

            if (!node->TooltipText)
                node->TooltipText = PhGetServiceTooltipText(node->ServiceItem);

            if (node->TooltipText)
                getNodeTooltip->Text = node->TooltipText->sr;
            else
                return FALSE;
        }
        return TRUE;
    case TreeListSortChanged:
        {
            TreeList_GetSort(hwnd, &ServiceTreeListSortColumn, &ServiceTreeListSortOrder);
            // Force a rebuild to sort the items.
            TreeList_NodesStructured(hwnd);
        }
        return TRUE;
    case TreeListKeyDown:
        {
            switch ((SHORT)Parameter1)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                    SendMessage(PhMainWndHandle, WM_COMMAND, ID_SERVICE_COPY, 0);
                break;
            case VK_DELETE:
                SendMessage(PhMainWndHandle, WM_COMMAND, ID_SERVICE_DELETE, 0);
                break;
            case VK_RETURN:
                SendMessage(PhMainWndHandle, WM_COMMAND, ID_SERVICE_PROPERTIES, 0);
                break;
            }
        }
        return TRUE;
    case TreeListHeaderRightClick:
        {
            HMENU menu;
            HMENU subMenu;
            POINT point;

            menu = LoadMenu(PhInstanceHandle, MAKEINTRESOURCE(IDR_SERVICEHEADER));
            subMenu = GetSubMenu(menu, 0);
            GetCursorPos(&point);

            if ((UINT)TrackPopupMenu(
                subMenu,
                TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
                point.x,
                point.y,
                0,
                hwnd,
                NULL
                ) == ID_HEADER_CHOOSECOLUMNS)
            {
                PhShowChooseColumnsDialog(hwnd, hwnd);

                // Make sure the column we're sorting by is actually visible, 
                // and if not, don't sort any more.
                if (ServiceTreeListSortOrder != NoSortOrder)
                {
                    PH_TREELIST_COLUMN column;

                    column.Id = ServiceTreeListSortColumn;
                    TreeList_GetColumn(ServiceTreeListHandle, &column);

                    if (!column.Visible)
                        TreeList_SetSort(ServiceTreeListHandle, 0, AscendingSortOrder);
                }
            }

            DestroyMenu(menu);
        }
        return TRUE;
    case TreeListNodeRightClick:
        {
            PPH_TREELIST_MOUSE_EVENT mouseEvent = Parameter2;

            PhShowServiceContextMenu(mouseEvent->Location);
        }
        return TRUE;
    case TreeListNodeLeftDoubleClick:
        {
            SendMessage(PhMainWndHandle, WM_COMMAND, ID_SERVICE_PROPERTIES, 0);
        }
        return TRUE;
    }

    return FALSE;
}

PPH_SERVICE_ITEM PhGetSelectedServiceItem()
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
    __out PPH_SERVICE_ITEM **Services,
    __out PULONG NumberOfServices
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

VOID PhDeselectAllServiceNodes()
{
    ULONG i;

    for (i = 0; i < ServiceNodeList->Count; i++)
    {
        PPH_SERVICE_NODE node = ServiceNodeList->Items[i];

        node->Node.Selected = FALSE;
        PhInvalidateTreeListNode(&node->Node, TLIN_STATE);
    }

    InvalidateRect(ServiceTreeListHandle, NULL, TRUE);
}

VOID PhSelectAndEnsureVisibleServiceNode(
    __in PPH_SERVICE_NODE ServiceNode
    )
{
    PhDeselectAllServiceNodes();

    if (!ServiceNode->Node.Visible)
        return;

    ServiceNode->Node.Selected = TRUE;
    ServiceNode->Node.Focused = TRUE;
    PhInvalidateTreeListNode(&ServiceNode->Node, TLIN_STATE);
    ListView_SetItemState(TreeList_GetListView(ServiceTreeListHandle), ServiceNode->Node.s.ViewIndex,
        LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    TreeList_EnsureVisible(ServiceTreeListHandle, &ServiceNode->Node, FALSE);
}

VOID PhCopyServiceList()
{
    PPH_FULL_STRING text;

    text = PhGetTreeListText(ServiceTreeListHandle, PHSVTLC_MAXIMUM);
    PhSetClipboardStringEx(ServiceTreeListHandle, text->Buffer, text->Length);
    PhDereferenceObject(text);
}

VOID PhWriteServiceList(
    __inout PPH_FILE_STREAM FileStream,
    __in ULONG Mode
    )
{
    PPH_LIST lines;
    ULONG i;

    lines = PhGetServiceTreeListLines(ServiceTreeListHandle, ServiceNodeList, Mode);

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
