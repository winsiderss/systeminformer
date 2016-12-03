/*
 * Process Hacker Network Tools -
 *   Tracert dialog
 *
 * Copyright (C) 2015-2016 dmex
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

#include "nettools.h"
#include "tracert.h"
#include <commonutil.h>

#define SORT_FUNCTION(Column) PmPoolTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PmPoolTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPOOLTAG_ROOT_NODE node1 = *(PPOOLTAG_ROOT_NODE*)_elem1; \
    PPOOLTAG_ROOT_NODE node2 = *(PPOOLTAG_ROOT_NODE*)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)node1->Node.Index, (ULONG_PTR)node2->Node.Index); \
    \
    return PhModifySort(sortResult, ((PNETWORK_TRACERT_CONTEXT)_context)->TreeNewSortOrder); \
}

VOID PmLoadSettingsTreeList(
    _Inout_ PNETWORK_TRACERT_CONTEXT Context
    )
{
//    PPH_STRING settings;
//    PH_INTEGER_PAIR sortSettings;

    //settings = PhGetStringSetting(SETTING_NAME_TREE_LIST_COLUMNS);
    //PhCmLoadSettings(Context->TreeNewHandle, &settings->sr);
    //PhDereferenceObject(settings);

    //sortSettings = PhGetIntegerPairSetting(SETTING_NAME_TREE_LIST_SORT);
    //TreeNew_SetSort(Context->TreeNewHandle, (ULONG)sortSettings.X, (PH_SORT_ORDER)sortSettings.Y);
}

VOID PmSaveSettingsTreeList(
    _Inout_ PNETWORK_TRACERT_CONTEXT Context
    )
{
//    PPH_STRING settings;
//    PH_INTEGER_PAIR sortSettings;
//    ULONG sortColumn;
//    PH_SORT_ORDER sortOrder;
        
    //settings = PhCmSaveSettings(Context->TreeNewHandle);
    //PhSetStringSetting2(SETTING_NAME_TREE_LIST_COLUMNS, &settings->sr);
    //PhDereferenceObject(settings);

    //TreeNew_GetSort(Context->TreeNewHandle, &sortColumn, &sortOrder);
    //sortSettings.X = sortColumn;
    //sortSettings.Y = sortOrder;
    //PhSetIntegerPairSetting(SETTING_NAME_TREE_LIST_SORT, sortSettings);
}

BOOLEAN PmPoolTagNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPOOLTAG_ROOT_NODE poolTagNode1 = *(PPOOLTAG_ROOT_NODE *)Entry1;
    PPOOLTAG_ROOT_NODE poolTagNode2 = *(PPOOLTAG_ROOT_NODE *)Entry2;

    return poolTagNode1->Node.Index == poolTagNode2->Node.Index;
}

ULONG PmPoolTagNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return (*(PPOOLTAG_ROOT_NODE*)Entry)->Node.Index;
}

VOID PmDestroyPoolTagNode(
    _In_ PPOOLTAG_ROOT_NODE PoolTagNode
    )
{
   PhClearReference(&PoolTagNode->TtlString);

   PhFree(PoolTagNode);
}

PPOOLTAG_ROOT_NODE PmAddPoolTagNode(
    _Inout_ PNETWORK_TRACERT_CONTEXT Context,
    _In_ ULONG TTL
    )
{
    PPOOLTAG_ROOT_NODE poolTagNode;

    poolTagNode = PhAllocate(sizeof(POOLTAG_ROOT_NODE));
    memset(poolTagNode, 0, sizeof(POOLTAG_ROOT_NODE));
    PhInitializeTreeNewNode(&poolTagNode->Node);

    poolTagNode->TTL = TTL;

    memset(poolTagNode->TextCache, 0, sizeof(PH_STRINGREF) * TREE_COLUMN_ITEM_MAXIMUM);
    poolTagNode->Node.TextCache = poolTagNode->TextCache;
    poolTagNode->Node.TextCacheSize = TREE_COLUMN_ITEM_MAXIMUM;

    PhAddEntryHashtable(Context->NodeHashtable, &poolTagNode);
    //PhAddItemList(Context->NodeList, poolTagNode);
    PhInsertItemList(Context->NodeList, 0, poolTagNode);

    //if (Context->FilterSupport.FilterList)
    //    poolTagNode->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->FilterSupport, &poolTagNode->Node);

    return poolTagNode;
}

PPOOLTAG_ROOT_NODE PmFindPoolTagNode(
    _In_ PNETWORK_TRACERT_CONTEXT Context,
    _In_ ULONG PoolTag
    )
{
    POOLTAG_ROOT_NODE lookupWindowNode;
    PPOOLTAG_ROOT_NODE lookupWindowNodePtr = &lookupWindowNode;
    PPOOLTAG_ROOT_NODE *windowNode;

    lookupWindowNode.Node.Index = PoolTag;

    windowNode = (PPOOLTAG_ROOT_NODE*)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupWindowNodePtr
        );

    if (windowNode)
        return *windowNode;
    else
        return NULL;
}

VOID PmRemovePoolTagNode(
    _In_ PNETWORK_TRACERT_CONTEXT Context,
    _In_ PPOOLTAG_ROOT_NODE PoolTagNode
    )
{
    ULONG index = 0;

    // Remove from hashtable/list and cleanup.
    PhRemoveEntryHashtable(Context->NodeHashtable, &PoolTagNode);

    if ((index = PhFindItemList(Context->NodeList, PoolTagNode)) != -1)
    {
        PhRemoveItemList(Context->NodeList, index);
    }

    PmDestroyPoolTagNode(PoolTagNode);

    //TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PmUpdatePoolTagNode(
    _In_ PNETWORK_TRACERT_CONTEXT Context,
    _In_ PPOOLTAG_ROOT_NODE PoolTagNode
    )
{
    memset(PoolTagNode->TextCache, 0, sizeof(PH_STRINGREF) * TREE_COLUMN_ITEM_MAXIMUM);

    PhInvalidateTreeNewNode(&PoolTagNode->Node, TN_CACHE_COLOR);
    //TreeNew_NodesStructured(Context->TreeNewHandle);
}

BEGIN_SORT_FUNCTION(Ttl)
{
    sortResult = uint64cmp(node1->TTL, node2->TTL);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Ping1)
{
    sortResult = PhCompareStringWithNull(node1->Ping1String, node2->Ping1String, TRUE);
}
END_SORT_FUNCTION

//BEGIN_SORT_FUNCTION(Ping2)
//{
//    sortResult = uint64cmp(poolItem1->PagedAllocsDelta.Value, poolItem2->PagedAllocsDelta.Value);
//}
//END_SORT_FUNCTION

BOOLEAN NTAPI PmPoolTagTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PNETWORK_TRACERT_CONTEXT context;
    PPOOLTAG_ROOT_NODE node;

    context = Context;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            node = (PPOOLTAG_ROOT_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Ttl),
                    SORT_FUNCTION(Ping1),
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

            node = (PPOOLTAG_ROOT_NODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;
            node = (PPOOLTAG_ROOT_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case TREE_COLUMN_ITEM_TTL:
                PhMoveReference(&node->TtlString, PhFormatUInt64(node->TTL, TRUE));
                PhInitializeStringRefLongHint(&getCellText->Text, PhGetStringOrEmpty(node->TtlString));
                break;
            case TREE_COLUMN_ITEM_PING1:
                PhInitializeStringRefLongHint(&getCellText->Text, PhGetStringOrEmpty(node->Ping1String));
                break;
            case TREE_COLUMN_ITEM_PING2:
                PhInitializeStringRefLongHint(&getCellText->Text, PhGetStringOrEmpty(node->Ping2String));
                break;
            case TREE_COLUMN_ITEM_PING3:
                PhInitializeStringRefLongHint(&getCellText->Text, PhGetStringOrEmpty(node->Ping3String));
                break;
            case TREE_COLUMN_ITEM_PING4:
                PhInitializeStringRefLongHint(&getCellText->Text, PhGetStringOrEmpty(node->Ping4String));
                break;
            case TREE_COLUMN_ITEM_COUNTRY:
                PhInitializeStringRefLongHint(&getCellText->Text, PhGetStringOrEmpty(node->CountryString));
                break;
            case TREE_COLUMN_ITEM_IPADDR:
                PhInitializeStringRefLongHint(&getCellText->Text, PhGetStringOrEmpty(node->IpAddressString));
                break;
            case TREE_COLUMN_ITEM_HOSTNAME:
                PhInitializeStringRefLongHint(&getCellText->Text, PhGetStringOrEmpty(node->HostnameString));
                break;

            //case TREE_COLUMN_ITEM_PAGEDALLOC:
            //    {
            //        if (poolItem->PagedAllocsDelta.Value != 0)
            //        {
            //            PhMoveReference(&node->PagedAllocsDeltaString, PhFormatUInt64(poolItem->PagedAllocsDelta.Value, TRUE));
            //            getCellText->Text = node->PagedAllocsDeltaString->sr;
            //        }
            //    }
            //    break;
            //case TREE_COLUMN_ITEM_PAGEDFREE:
            //    {
            //        if (poolItem->PagedFreesDelta.Value != 0)
            //        {
            //            PhMoveReference(&node->PagedFreesDeltaString, PhFormatUInt64(poolItem->PagedFreesDelta.Value, TRUE));
            //            getCellText->Text = node->PagedFreesDeltaString->sr;
            //        }
            //    }
            //    break;
            //case TREE_COLUMN_ITEM_PAGEDCURRENT:
            //    {
            //        if (poolItem->PagedCurrentDelta.Value != 0)
            //        {
            //            PhMoveReference(&node->PagedCurrentDeltaString, PhFormatUInt64(poolItem->PagedCurrentDelta.Value, TRUE));
            //            getCellText->Text = node->PagedCurrentDeltaString->sr;
            //        }
            //    }
            //    break;
            //case TREE_COLUMN_ITEM_PAGEDTOTAL:
            //    {
            //        if (poolItem->PagedTotalSizeDelta.Value != 0)
            //        {
            //            PhMoveReference(&node->PagedTotalSizeDeltaString, PhFormatSize(poolItem->PagedTotalSizeDelta.Value, -1));
            //            getCellText->Text = node->PagedTotalSizeDeltaString->sr;
            //        }
            //    }
            //    break;
            //case TREE_COLUMN_ITEM_NONPAGEDALLOC:
            //    {
            //        if (poolItem->NonPagedAllocsDelta.Value != 0)
            //        {
            //            PhMoveReference(&node->NonPagedAllocsDeltaString, PhFormatUInt64(poolItem->NonPagedAllocsDelta.Value, TRUE));
            //            getCellText->Text = node->NonPagedAllocsDeltaString->sr;
            //        }
            //    }
            //    break;
            //case TREE_COLUMN_ITEM_NONPAGEDFREE:
            //    {
            //        if (poolItem->NonPagedFreesDelta.Value != 0)
            //        {
            //            PhMoveReference(&node->NonPagedFreesDeltaString, PhFormatUInt64(poolItem->NonPagedFreesDelta.Value, TRUE));
            //            getCellText->Text = node->NonPagedFreesDeltaString->sr;
            //        }
            //    }
            //    break;
            //case TREE_COLUMN_ITEM_NONPAGEDCURRENT:
            //    {
            //        if (poolItem->NonPagedCurrentDelta.Value != 0)
            //        {
            //            PhMoveReference(&node->NonPagedCurrentDeltaString, PhFormatUInt64(poolItem->NonPagedCurrentDelta.Value, TRUE));
            //            getCellText->Text = node->NonPagedCurrentDeltaString->sr;
            //        }
            //    }
            //    break;
            //case TREE_COLUMN_ITEM_NONPAGEDTOTAL:
            //    {
            //        if (poolItem->NonPagedTotalSizeDelta.Value != 0)
            //        {
            //            PhMoveReference(&node->NonPagedTotalSizeDeltaString, PhFormatSize(poolItem->NonPagedTotalSizeDelta.Value, -1));
            //            getCellText->Text = node->NonPagedTotalSizeDeltaString->sr;
            //        }
            //    }
            //    break;
            default:
                return FALSE;
            }

            getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = (PPH_TREENEW_GET_NODE_COLOR)Parameter1;
            node = (PPOOLTAG_ROOT_NODE)getNodeColor->Node;

            //switch (node->PoolItem->Type)
            //{
            //case TPOOLTAG_TREE_ITEM_TYPE_OBJECT:
            //    getNodeColor->BackColor = RGB(204, 255, 255);
            //    break;
            //case TPOOLTAG_TREE_ITEM_TYPE_DRIVER:
            //    getNodeColor->BackColor = RGB(170, 204, 255);
            //    break; 
            //}

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
            PPH_TREENEW_MOUSE_EVENT mouseEvent = (PPH_TREENEW_MOUSE_EVENT)Parameter1;

            SendMessage(
                context->WindowHandle,
                WM_COMMAND, 
                POOL_TABLE_SHOWCONTEXTMENU, 
                (LPARAM)mouseEvent
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

            data.Selection = PhShowEMenu(data.Menu, hwnd, PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    }

    return FALSE;
}

VOID PmClearPoolTagTree(
    _In_ PNETWORK_TRACERT_CONTEXT Context
    )
{
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
        PmDestroyPoolTagNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);
}

PPOOLTAG_ROOT_NODE PmGetSelectedPoolTagNode(
    _In_ PNETWORK_TRACERT_CONTEXT Context
    )
{
    PPOOLTAG_ROOT_NODE windowNode = NULL;
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        windowNode = Context->NodeList->Items[i];

        if (windowNode->Node.Selected)
            return windowNode;
    }

    return NULL;
}

VOID PmGetSelectedPoolTagNodes(
    _In_ PNETWORK_TRACERT_CONTEXT Context,
    _Out_ PPOOLTAG_ROOT_NODE **PoolTags,
    _Out_ PULONG NumberOfPoolTags
    )
{
    PPH_LIST list;
    ULONG i;

    list = PhCreateList(2);

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PPOOLTAG_ROOT_NODE node = (PPOOLTAG_ROOT_NODE)Context->NodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, node);
        }
    }

    *PoolTags = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
    *NumberOfPoolTags = list->Count;

    PhDereferenceObject(list);
}

VOID PmInitializePoolTagTree(
    _Inout_ PNETWORK_TRACERT_CONTEXT Context
    )
{
    Context->NodeList = PhCreateList(100);
    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PPOOLTAG_ROOT_NODE),
        PmPoolTagNodeHashtableEqualFunction,
        PmPoolTagNodeHashtableHashFunction,
        100
        );

    PhSetControlTheme(Context->TreeNewHandle, L"explorer");

    TreeNew_SetCallback(Context->TreeNewHandle, PmPoolTagTreeNewCallback, Context);

    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_TTL, TRUE, L"TTL", 30, PH_ALIGN_LEFT, -2, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_PING1, TRUE, L"Time", 50, PH_ALIGN_RIGHT, 0, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_PING2, TRUE, L"Time", 50, PH_ALIGN_RIGHT, 1, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_PING3, TRUE, L"Time", 50, PH_ALIGN_RIGHT, 2, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_PING4, TRUE, L"Time", 50, PH_ALIGN_RIGHT, 3, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_COUNTRY, TRUE, L"GeoIP", 100, PH_ALIGN_LEFT, 5, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_IPADDR, TRUE, L"IP Address", 100, PH_ALIGN_RIGHT, 4, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_HOSTNAME, TRUE, L"Hostname", 150, PH_ALIGN_LEFT, 6, 0);

    //for (INT i = 0; i < MAX_PINGS; i++)
    //    PhAddTreeNewColumn(context->TreeNewHandle, i + 1, i + 1, i + 1, LVCFMT_RIGHT, 50, L"Time");

    TreeNew_SetTriState(Context->TreeNewHandle, TRUE);
    TreeNew_SetSort(Context->TreeNewHandle, TREE_COLUMN_ITEM_TTL, AscendingSortOrder);

    PmLoadSettingsTreeList(Context);

    PhInitializeTreeNewFilterSupport(&Context->FilterSupport, Context->TreeNewHandle, Context->NodeList);
}

VOID PmDeletePoolTagTree(
    _In_ PNETWORK_TRACERT_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    //PhSetStringSetting2(SETTING_NAME_TREE_LIST_COLUMNS, &settings->sr);
    PhDereferenceObject(settings);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PmDestroyPoolTagNode(Context->NodeList->Items[i]);
    }

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}