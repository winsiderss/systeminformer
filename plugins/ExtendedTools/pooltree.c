/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2022
 *
 */

#include "exttools.h"
#include "poolmon.h"

#define SORT_FUNCTION(Column) EtPoolTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl EtPoolTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPOOLTAG_ROOT_NODE node1 = *(PPOOLTAG_ROOT_NODE*)_elem1; \
    PPOOLTAG_ROOT_NODE node2 = *(PPOOLTAG_ROOT_NODE*)_elem2; \
    PPOOL_ITEM poolItem1 = node1->PoolItem; \
    PPOOL_ITEM poolItem2 = node2->PoolItem; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)poolItem1->TagUlong, (ULONG_PTR)poolItem2->TagUlong); \
    \
    return PhModifySort(sortResult, ((PPOOLTAG_CONTEXT)_context)->TreeNewSortOrder); \
}

VOID EtLoadSettingsPoolTagTreeList(
    _Inout_ PPOOLTAG_CONTEXT Context
    )
{
    PPH_STRING settings;
    PH_INTEGER_PAIR sortSettings;

    settings = PhGetStringSetting(SETTING_NAME_POOL_TREE_LIST_COLUMNS);
    PhCmLoadSettings(Context->TreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);

    sortSettings = PhGetIntegerPairSetting(SETTING_NAME_POOL_TREE_LIST_SORT);
    TreeNew_SetSort(Context->TreeNewHandle, (ULONG)sortSettings.X, (PH_SORT_ORDER)sortSettings.Y);
}

VOID EtSaveSettingsPoolTagTreeList(
    _Inout_ PPOOLTAG_CONTEXT Context
    )
{
    PPH_STRING settings;
    PH_INTEGER_PAIR sortSettings;
    ULONG sortColumn;
    PH_SORT_ORDER sortOrder;

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(SETTING_NAME_POOL_TREE_LIST_COLUMNS, &settings->sr);
    PhDereferenceObject(settings);

    TreeNew_GetSort(Context->TreeNewHandle, &sortColumn, &sortOrder);
    sortSettings.X = sortColumn;
    sortSettings.Y = sortOrder;
    PhSetIntegerPairSetting(SETTING_NAME_POOL_TREE_LIST_SORT, sortSettings);
}

BOOLEAN EtPoolTagNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPOOLTAG_ROOT_NODE poolTagNode1 = *(PPOOLTAG_ROOT_NODE *)Entry1;
    PPOOLTAG_ROOT_NODE poolTagNode2 = *(PPOOLTAG_ROOT_NODE *)Entry2;

    return poolTagNode1->TagUlong == poolTagNode2->TagUlong;
}

ULONG EtPoolTagNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashInt32((*(PPOOLTAG_ROOT_NODE*)Entry)->TagUlong);
}

VOID EtDestroyPoolTagNode(
    _In_ PPOOLTAG_ROOT_NODE PoolTagNode
    )
{
    if (PoolTagNode->PagedAllocsValueString)
        PhDereferenceObject(PoolTagNode->PagedAllocsValueString);
    if (PoolTagNode->PagedFreesValueString)
        PhDereferenceObject(PoolTagNode->PagedFreesValueString);
    if (PoolTagNode->PagedCurrentValueString)
        PhDereferenceObject(PoolTagNode->PagedCurrentValueString);
    if (PoolTagNode->PagedTotalSizeValueString)
        PhDereferenceObject(PoolTagNode->PagedTotalSizeValueString);
    if (PoolTagNode->NonPagedAllocsValueString)
        PhDereferenceObject(PoolTagNode->NonPagedAllocsValueString);
    if (PoolTagNode->NonPagedFreesValueString)
        PhDereferenceObject(PoolTagNode->NonPagedFreesValueString);
    if (PoolTagNode->NonPagedCurrentValueString)
        PhDereferenceObject(PoolTagNode->NonPagedCurrentValueString);
    if (PoolTagNode->NonPagedTotalSizeValueString)
        PhDereferenceObject(PoolTagNode->NonPagedTotalSizeValueString);
    if (PoolTagNode->PagedAllocsDeltaString)
        PhDereferenceObject(PoolTagNode->PagedAllocsDeltaString);
    if (PoolTagNode->PagedFreesDeltaString)
        PhDereferenceObject(PoolTagNode->PagedFreesDeltaString);
    if (PoolTagNode->PagedCurrentDeltaString)
        PhDereferenceObject(PoolTagNode->PagedCurrentDeltaString);
    if (PoolTagNode->PagedTotalSizeDeltaString)
        PhDereferenceObject(PoolTagNode->PagedTotalSizeDeltaString);
    if (PoolTagNode->NonPagedAllocsDeltaString)
        PhDereferenceObject(PoolTagNode->NonPagedAllocsDeltaString);
    if (PoolTagNode->NonPagedFreesDeltaString)
        PhDereferenceObject(PoolTagNode->NonPagedFreesDeltaString);
    if (PoolTagNode->NonPagedCurrentDeltaString)
        PhDereferenceObject(PoolTagNode->NonPagedCurrentDeltaString);
    if (PoolTagNode->NonPagedTotalSizeDeltaString)
        PhDereferenceObject(PoolTagNode->NonPagedTotalSizeDeltaString);

    PhFree(PoolTagNode);
}

PPOOLTAG_ROOT_NODE EtAddPoolTagNode(
    _Inout_ PPOOLTAG_CONTEXT Context,
    _In_ PPOOL_ITEM PoolItem
    )
{
    PPOOLTAG_ROOT_NODE poolTagNode;

    poolTagNode = PhAllocateZero(sizeof(POOLTAG_ROOT_NODE));
    PhInitializeTreeNewNode(&poolTagNode->Node);

    poolTagNode->PoolItem = PoolItem;
    poolTagNode->TagUlong = PoolItem->TagUlong;

    memset(poolTagNode->TextCache, 0, sizeof(PH_STRINGREF) * TREE_COLUMN_ITEM_MAXIMUM);
    poolTagNode->Node.TextCache = poolTagNode->TextCache;
    poolTagNode->Node.TextCacheSize = TREE_COLUMN_ITEM_MAXIMUM;

    PhAddEntryHashtable(Context->NodeHashtable, &poolTagNode);
    PhAddItemList(Context->NodeList, poolTagNode);

    if (Context->FilterSupport.FilterList)
        poolTagNode->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->FilterSupport, &poolTagNode->Node);

    return poolTagNode;
}

PPOOLTAG_ROOT_NODE EtFindPoolTagNode(
    _In_ PPOOLTAG_CONTEXT Context,
    _In_ ULONG PoolTag
    )
{
    POOLTAG_ROOT_NODE lookupNode;
    PPOOLTAG_ROOT_NODE lookupNodePtr = &lookupNode;
    PPOOLTAG_ROOT_NODE *node;

    lookupNode.TagUlong = PoolTag;

    node = (PPOOLTAG_ROOT_NODE*)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupNodePtr
        );

    if (node)
        return *node;
    else
        return NULL;
}

VOID EtRemovePoolTagNode(
    _In_ PPOOLTAG_CONTEXT Context,
    _In_ PPOOLTAG_ROOT_NODE PoolTagNode
    )
{
    ULONG index = 0;

    PhRemoveEntryHashtable(Context->NodeHashtable, &PoolTagNode);

    if ((index = PhFindItemList(Context->NodeList, PoolTagNode)) != ULONG_MAX)
    {
        PhRemoveItemList(Context->NodeList, index);
    }

    EtDestroyPoolTagNode(PoolTagNode);
    //TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID EtUpdatePoolTagNode(
    _In_ PPOOLTAG_CONTEXT Context,
    _In_ PPOOLTAG_ROOT_NODE Node
    )
{
    memset(Node->TextCache, 0, sizeof(PH_STRINGREF) * TREE_COLUMN_ITEM_MAXIMUM);

    PhInvalidateTreeNewNode(&Node->Node, TN_CACHE_COLOR);
    //TreeNew_NodesStructured(Context->TreeNewHandle);
}

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareStringZ(poolItem1->TagString, poolItem2->TagString, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Type)
{
    sortResult = PhCompareStringWithNull(poolItem1->BinaryNameString, poolItem2->BinaryNameString, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Description)
{
    sortResult = PhCompareStringWithNull(poolItem1->DescriptionString, poolItem2->DescriptionString, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PagedAlloc)
{
    sortResult = uint64cmp(poolItem1->PagedAllocsDelta.Value, poolItem2->PagedAllocsDelta.Value);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PagedFree)
{
    sortResult = uint64cmp(poolItem1->PagedFreesDelta.Value, poolItem2->PagedFreesDelta.Value);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PagedCurrent)
{
    sortResult = uint64cmp(poolItem1->PagedCurrentDelta.Value, poolItem2->PagedCurrentDelta.Value);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PagedTotal)
{
    sortResult = uint64cmp(poolItem1->PagedTotalSizeDelta.Value, poolItem2->PagedTotalSizeDelta.Value);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(NonPagedAlloc)
{
    sortResult = uint64cmp(poolItem1->NonPagedAllocsDelta.Value, poolItem2->NonPagedAllocsDelta.Value);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(NonPagedFree)
{
    sortResult = uint64cmp(poolItem1->NonPagedFreesDelta.Value, poolItem2->NonPagedFreesDelta.Value);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(NonPagedCurrent)
{
    sortResult = uint64cmp(poolItem1->NonPagedCurrentDelta.Value, poolItem2->NonPagedCurrentDelta.Value);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(NonPagedTotal)
{
    sortResult = uint64cmp(poolItem1->NonPagedTotalSizeDelta.Value, poolItem2->NonPagedTotalSizeDelta.Value);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PagedAllocDelta)
{
    sortResult = uint64cmp(poolItem1->PagedAllocsDelta.Delta, poolItem2->PagedAllocsDelta.Delta);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PagedFreeDelta)
{
    sortResult = uint64cmp(poolItem1->PagedFreesDelta.Delta, poolItem2->PagedFreesDelta.Delta);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PagedCurrentDelta)
{
    sortResult = uint64cmp(poolItem1->PagedCurrentDelta.Delta, poolItem2->PagedCurrentDelta.Delta);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(PagedTotalDelta)
{
    sortResult = uint64cmp(poolItem1->PagedTotalSizeDelta.Delta, poolItem2->PagedTotalSizeDelta.Delta);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(NonPagedAllocDelta)
{
    sortResult = uint64cmp(poolItem1->NonPagedAllocsDelta.Delta, poolItem2->NonPagedAllocsDelta.Delta);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(NonPagedFreeDelta)
{
    sortResult = uint64cmp(poolItem1->NonPagedFreesDelta.Delta, poolItem2->NonPagedFreesDelta.Delta);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(NonPagedCurrentDelta)
{
    sortResult = uint64cmp(poolItem1->NonPagedCurrentDelta.Delta, poolItem2->NonPagedCurrentDelta.Delta);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(NonPagedTotalDelta)
{
    sortResult = uint64cmp(poolItem1->NonPagedTotalSizeDelta.Delta, poolItem2->NonPagedTotalSizeDelta.Delta);
}
END_SORT_FUNCTION

PPH_STRING EtPoolFormatDeltaValue(
    _In_ ULONG64 DeltaValue
    )
{
    LONG_PTR delta = (LONG_PTR)DeltaValue;
    PH_FORMAT format[2];

    if (delta == 0)
    {
        return PhReferenceEmptyString();
    }

    if (delta > 0)
    {
        PhInitFormatC(&format[0], L'+');
    }
    else
    {
        PhInitFormatC(&format[0], L'-');
        delta = -delta;
    }

    format[1].Type = UInt64FormatType;
    format[1].u.UInt64 = delta;

    return PhFormat(format, 2, 0);
}

PPH_STRING EtPoolFormatDeltaSize(
    _In_ ULONG64 DeltaValue
    )
{
    LONG_PTR delta = (LONG_PTR)DeltaValue;
    PH_FORMAT format[2];

    if (delta == 0)
    {
        return PhReferenceEmptyString();
    }

    if (delta > 0)
    {
        PhInitFormatC(&format[0], L'+');
    }
    else
    {
        PhInitFormatC(&format[0], L'-');
        delta = -delta;
    }

    format[1].Type = SizeFormatType | FormatUseRadix;
    format[1].Radix = (UCHAR)PhGetIntegerSetting(L"MaxSizeUnit");
    format[1].u.Size = delta;

    return PhFormat(format, 2, 0);
}

BOOLEAN NTAPI EtPoolTagTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PPOOLTAG_CONTEXT context = Context;
    PPOOLTAG_ROOT_NODE node;

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
                    SORT_FUNCTION(Name),
                    SORT_FUNCTION(Type),
                    SORT_FUNCTION(Description),
                    SORT_FUNCTION(PagedAlloc),
                    SORT_FUNCTION(PagedFree),
                    SORT_FUNCTION(PagedCurrent),
                    SORT_FUNCTION(PagedTotal),
                    SORT_FUNCTION(NonPagedAlloc),
                    SORT_FUNCTION(NonPagedFree),
                    SORT_FUNCTION(NonPagedCurrent),
                    SORT_FUNCTION(NonPagedTotal),
                    SORT_FUNCTION(PagedAllocDelta),
                    SORT_FUNCTION(PagedFreeDelta),
                    SORT_FUNCTION(PagedCurrentDelta),
                    SORT_FUNCTION(PagedTotalDelta),
                    SORT_FUNCTION(NonPagedAllocDelta),
                    SORT_FUNCTION(NonPagedFreeDelta),
                    SORT_FUNCTION(NonPagedCurrentDelta),
                    SORT_FUNCTION(NonPagedTotalDelta),
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
            PPOOL_ITEM poolItem;

            node = (PPOOLTAG_ROOT_NODE)getCellText->Node;
            poolItem = node->PoolItem;

            switch (getCellText->Id)
            {
            case TREE_COLUMN_ITEM_TAG:
                PhInitializeStringRefLongHint(&getCellText->Text, poolItem->TagString);
                break;
            case TREE_COLUMN_ITEM_DRIVER:
                PhInitializeStringRefLongHint(&getCellText->Text, PhGetStringOrEmpty(poolItem->BinaryNameString));
                break;
            case TREE_COLUMN_ITEM_DESCRIPTION:
                PhInitializeStringRefLongHint(&getCellText->Text, PhGetStringOrEmpty(poolItem->DescriptionString));
                break;
            case TREE_COLUMN_ITEM_PAGEDALLOC:
                {
                    if (poolItem->PagedAllocsDelta.Value != 0)
                    {
                        PhMoveReference(&node->PagedAllocsValueString, PhFormatUInt64(poolItem->PagedAllocsDelta.Value, TRUE));
                        getCellText->Text = node->PagedAllocsValueString->sr;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_PAGEDFREE:
                {
                    if (poolItem->PagedFreesDelta.Value != 0)
                    {
                        PhMoveReference(&node->PagedFreesValueString, PhFormatUInt64(poolItem->PagedFreesDelta.Value, TRUE));
                        getCellText->Text = node->PagedFreesValueString->sr;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_PAGEDCURRENT:
                {
                    if (poolItem->PagedCurrentDelta.Value != 0)
                    {
                        PhMoveReference(&node->PagedCurrentValueString, PhFormatUInt64(poolItem->PagedCurrentDelta.Value, TRUE));
                        getCellText->Text = node->PagedCurrentValueString->sr;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_PAGEDTOTAL:
                {
                    if (poolItem->PagedTotalSizeDelta.Value != 0)
                    {
                        PhMoveReference(&node->PagedTotalSizeValueString, PhFormatSize(poolItem->PagedTotalSizeDelta.Value, ULONG_MAX));
                        getCellText->Text = node->PagedTotalSizeValueString->sr;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_NONPAGEDALLOC:
                {
                    if (poolItem->NonPagedAllocsDelta.Value != 0)
                    {
                        PhMoveReference(&node->NonPagedAllocsValueString, PhFormatUInt64(poolItem->NonPagedAllocsDelta.Value, TRUE));
                        getCellText->Text = node->NonPagedAllocsValueString->sr;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_NONPAGEDFREE:
                {
                    if (poolItem->NonPagedFreesDelta.Value != 0)
                    {
                        PhMoveReference(&node->NonPagedFreesValueString, PhFormatUInt64(poolItem->NonPagedFreesDelta.Value, TRUE));
                        getCellText->Text = node->NonPagedFreesValueString->sr;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_NONPAGEDCURRENT:
                {
                    if (poolItem->NonPagedCurrentDelta.Value != 0)
                    {
                        PhMoveReference(&node->NonPagedCurrentValueString, PhFormatUInt64(poolItem->NonPagedCurrentDelta.Value, TRUE));
                        getCellText->Text = node->NonPagedCurrentValueString->sr;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_NONPAGEDTOTAL:
                {
                    if (poolItem->NonPagedTotalSizeDelta.Value != 0)
                    {
                        PhMoveReference(&node->NonPagedTotalSizeValueString, PhFormatSize(poolItem->NonPagedTotalSizeDelta.Value, ULONG_MAX));
                        getCellText->Text = node->NonPagedTotalSizeValueString->sr;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_PAGEDALLOCDELTA:
                {
                    if (!poolItem->HaveFirstSample)
                        break;

                    if (poolItem->PagedAllocsDelta.Delta != 0)
                    {
                        PhMoveReference(&node->PagedAllocsDeltaString, PhFormatUInt64(poolItem->PagedAllocsDelta.Delta, TRUE));
                        getCellText->Text = node->PagedAllocsDeltaString->sr;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_PAGEDFREEDELTA:
                {
                    if (!poolItem->HaveFirstSample)
                        break;

                    if (poolItem->PagedFreesDelta.Delta != 0)
                    {
                        PhMoveReference(&node->PagedFreesDeltaString, PhFormatUInt64(poolItem->PagedFreesDelta.Delta, TRUE));
                        getCellText->Text = node->PagedFreesDeltaString->sr;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_PAGEDCURRENTDELTA:
                {
                    if (!poolItem->HaveFirstSample)
                        break;

                    if (poolItem->PagedCurrentDelta.Delta != 0)
                    {
                        PhMoveReference(&node->PagedCurrentDeltaString, EtPoolFormatDeltaValue(poolItem->PagedCurrentDelta.Delta));
                        getCellText->Text = node->PagedCurrentDeltaString->sr;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_PAGEDTOTALDELTA:
                {
                    if (!poolItem->HaveFirstSample)
                        break;

                    if (poolItem->PagedTotalSizeDelta.Delta != 0)
                    {
                        PhMoveReference(&node->PagedTotalSizeDeltaString, EtPoolFormatDeltaSize(poolItem->PagedTotalSizeDelta.Delta));
                        getCellText->Text = node->PagedTotalSizeDeltaString->sr;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_NONPAGEDALLOCDELTA:
                {
                    if (!poolItem->HaveFirstSample)
                        break;

                    if (poolItem->NonPagedAllocsDelta.Delta != 0)
                    {
                        PhMoveReference(&node->NonPagedAllocsDeltaString, PhFormatUInt64(poolItem->NonPagedAllocsDelta.Delta, TRUE));
                        getCellText->Text = node->NonPagedAllocsDeltaString->sr;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_NONPAGEDFREEDELTA:
                {
                    if (!poolItem->HaveFirstSample)
                        break;

                    if (poolItem->NonPagedFreesDelta.Delta != 0)
                    {
                        PhMoveReference(&node->NonPagedFreesDeltaString, PhFormatUInt64(poolItem->NonPagedFreesDelta.Delta, TRUE));
                        getCellText->Text = node->NonPagedFreesDeltaString->sr;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_NONPAGEDCURRENTDELTA:
                {
                    if (!poolItem->HaveFirstSample)
                        break;

                    if (poolItem->NonPagedCurrentDelta.Delta != 0)
                    {
                        PhMoveReference(&node->NonPagedCurrentDeltaString, EtPoolFormatDeltaValue(poolItem->NonPagedCurrentDelta.Delta));
                        getCellText->Text = node->NonPagedCurrentDeltaString->sr;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_NONPAGEDTOTALDELTA:
                {
                    if (!poolItem->HaveFirstSample)
                        break;

                    if (poolItem->NonPagedTotalSizeDelta.Delta != 0)
                    {
                        PhMoveReference(&node->NonPagedTotalSizeDeltaString, EtPoolFormatDeltaSize(poolItem->NonPagedTotalSizeDelta.Delta));
                        getCellText->Text = node->NonPagedTotalSizeDeltaString->sr;
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
            //PPH_TREENEW_GET_NODE_COLOR getNodeColor = (PPH_TREENEW_GET_NODE_COLOR)Parameter1;
            //node = (PPOOLTAG_ROOT_NODE)getNodeColor->Node;

            //switch (node->PoolItem->Type)
            //{
            //case TPOOLTAG_TREE_ITEM_TYPE_OBJECT:
            //    getNodeColor->BackColor = RGB(255, 192, 203); //RGB(204, 255, 255);
            //    break;
            //case TPOOLTAG_TREE_ITEM_TYPE_DRIVER:
            //    getNodeColor->BackColor = RGB(170, 204, 255);
            //    break;
            //}

            //getNodeColor->Flags = TN_CACHE | TN_AUTO_FORECOLOR;
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

            SendMessage(
                context->WindowHandle,
                WM_COMMAND,
                WM_PH_UPDATE_DIALOG,
                (LPARAM)contextMenuEvent
                );
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            //PPOOLTAG_ROOT_NODE node;
            //
            //if (!(node = EtGetSelectedPoolTagNode(Context)))
            //    break;

            SendMessage(context->WindowHandle, WM_COMMAND, WM_PH_SHOW_DIALOG, 0);
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

            data.Selection = PhShowEMenu(data.Menu, hwnd, PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    }

    return FALSE;
}

VOID EtCopyPoolTagTree(
    _In_ PPOOLTAG_CONTEXT Context
    )
{
    PPH_STRING text;

    text = PhGetTreeNewText(Context->TreeNewHandle, 0);
    PhSetClipboardString(Context->TreeNewHandle, &text->sr);
    PhDereferenceObject(text);
}

VOID EtClearPoolTagTree(
    _In_ PPOOLTAG_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        EtDestroyPoolTagNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);
}

PPOOLTAG_ROOT_NODE EtGetSelectedPoolTagNode(
    _In_ PPOOLTAG_CONTEXT Context
    )
{
    PPOOLTAG_ROOT_NODE node = NULL;

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        node = Context->NodeList->Items[i];

        if (node->Node.Selected)
            return node;
    }

    return NULL;
}

VOID EtGetSelectedPoolTagNodes(
    _In_ PPOOLTAG_CONTEXT Context,
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

VOID EtInitializePoolTagTree(
    _Inout_ PPOOLTAG_CONTEXT Context
    )
{
    Context->NodeList = PhCreateList(100);
    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PPOOLTAG_ROOT_NODE),
        EtPoolTagNodeHashtableEqualFunction,
        EtPoolTagNodeHashtableHashFunction,
        100
        );

    PhSetControlTheme(Context->TreeNewHandle, L"explorer");

    TreeNew_SetCallback(Context->TreeNewHandle, EtPoolTagTreeNewCallback, Context);

    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_TAG, TRUE, L"Tag Name", 50, PH_ALIGN_LEFT, -2, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_DRIVER, TRUE, L"Driver", 82, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_DESCRIPTION, TRUE, L"Description", 140, PH_ALIGN_LEFT, 1, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_PAGEDALLOC, TRUE, L"Paged allocations (total)", 80, PH_ALIGN_RIGHT, 2, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_PAGEDFREE, TRUE, L"Paged frees (total)", 80, PH_ALIGN_RIGHT, 3, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_PAGEDCURRENT, TRUE, L"Paged current (total)", 80, PH_ALIGN_RIGHT, 4, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_PAGEDTOTAL, TRUE, L"Paged bytes (total)", 80, PH_ALIGN_LEFT, 5, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_NONPAGEDALLOC, TRUE, L"Non-paged allocations (total)", 80, PH_ALIGN_RIGHT, 6, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_NONPAGEDFREE, TRUE, L"Non-paged frees (total)", 80, PH_ALIGN_RIGHT, 7, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_NONPAGEDCURRENT, TRUE, L"Non-paged current (total)", 80, PH_ALIGN_RIGHT, 8, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_NONPAGEDTOTAL, TRUE, L"Non-paged bytes (total)", 80, PH_ALIGN_LEFT, 9, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_PAGEDALLOCDELTA, FALSE, L"Paged allocations (delta)", 80, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_PAGEDFREEDELTA, FALSE, L"Paged frees (delta)", 80, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_PAGEDCURRENTDELTA, FALSE, L"Paged current (delta)", 80, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_PAGEDTOTALDELTA, FALSE, L"Paged bytes (delta)", 80, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_NONPAGEDALLOCDELTA, FALSE, L"Non-paged allocations (delta)", 80, PH_ALIGN_RIGHT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_NONPAGEDFREEDELTA, FALSE, L"Non-paged frees (delta)", 80, PH_ALIGN_RIGHT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_NONPAGEDCURRENTDELTA, FALSE, L"Non-paged current (delta)", 80, PH_ALIGN_RIGHT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_NONPAGEDTOTALDELTA, FALSE, L"Non-paged bytes (delta)", 80, PH_ALIGN_LEFT, ULONG_MAX, 0);

    TreeNew_SetTriState(Context->TreeNewHandle, TRUE);
    TreeNew_SetSort(Context->TreeNewHandle, TREE_COLUMN_ITEM_NONPAGEDTOTAL, DescendingSortOrder);

    EtLoadSettingsPoolTagTreeList(Context);

    PhInitializeTreeNewFilterSupport(&Context->FilterSupport, Context->TreeNewHandle, Context->NodeList);
}

VOID EtDeletePoolTagTree(
    _In_ PPOOLTAG_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(SETTING_NAME_POOL_TREE_LIST_COLUMNS, &settings->sr);
    PhDereferenceObject(settings);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        EtDestroyPoolTagNode(Context->NodeList->Items[i]);
    }

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}
