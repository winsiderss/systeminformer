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

BEGIN_SORT_FUNCTION(Ttl)
{
    sortResult = uint64cmp(node1->TTL, node2->TTL);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Ping1)
{
    sortResult = uint64cmp(node1->Ping1, node2->Ping1);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Ping2)
{
    sortResult = uint64cmp(node1->Ping2, node2->Ping2);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Ping3)
{
    sortResult = uint64cmp(node1->Ping2, node2->Ping2);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Ping4)
{
    sortResult = uint64cmp(node1->Ping2, node2->Ping2);
}
END_SORT_FUNCTION


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

    return poolTagNode1->TTL == poolTagNode2->TTL;
}

ULONG PmPoolTagNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return (*(PPOOLTAG_ROOT_NODE*)Entry)->TTL;
}

VOID PmDestroyPoolTagNode(
    _In_ PPOOLTAG_ROOT_NODE PoolTagNode
    )
{
   PhClearReference(&PoolTagNode->TtlString);
   PhClearReference(&PoolTagNode->Ping1String);
   PhClearReference(&PoolTagNode->Ping2String);
   PhClearReference(&PoolTagNode->Ping3String);
   PhClearReference(&PoolTagNode->Ping4String);
   PhClearReference(&PoolTagNode->CountryString);
   PhClearReference(&PoolTagNode->HostnameString);
   PhClearReference(&PoolTagNode->IpAddressString);
   PhClearReference(&PoolTagNode->RemoteCountryCode);
   PhClearReference(&PoolTagNode->RemoteCountryName);

   PhDereferenceObject(PoolTagNode);
}

PPOOLTAG_ROOT_NODE AddTracertNode(
    _Inout_ PNETWORK_TRACERT_CONTEXT Context,
    _In_ ULONG TTL
    )
{
    PPOOLTAG_ROOT_NODE poolTagNode;

    poolTagNode = PhCreateAlloc(sizeof(POOLTAG_ROOT_NODE));
    memset(poolTagNode, 0, sizeof(POOLTAG_ROOT_NODE));

    PhInitializeTreeNewNode(&poolTagNode->Node);

    poolTagNode->TTL = TTL;

    memset(poolTagNode->TextCache, 0, sizeof(PH_STRINGREF) * TREE_COLUMN_ITEM_MAXIMUM);
    poolTagNode->Node.TextCache = poolTagNode->TextCache;
    poolTagNode->Node.TextCacheSize = TREE_COLUMN_ITEM_MAXIMUM;

    PhAddEntryHashtable(Context->NodeHashtable, &poolTagNode);
    PhAddItemList(Context->NodeList, poolTagNode);

    return poolTagNode;
}

PPOOLTAG_ROOT_NODE FindTracertNode(
    _In_ PNETWORK_TRACERT_CONTEXT Context,
    _In_ ULONG TTL
    )
{
    POOLTAG_ROOT_NODE lookupWindowNode;
    PPOOLTAG_ROOT_NODE lookupWindowNodePtr = &lookupWindowNode;
    PPOOLTAG_ROOT_NODE *windowNode;

    lookupWindowNode.TTL = TTL;

    windowNode = (PPOOLTAG_ROOT_NODE*)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupWindowNodePtr
        );

    if (windowNode)
        return *windowNode;
    else
        return NULL;
}

VOID RemoveTracertNode(
    _In_ PNETWORK_TRACERT_CONTEXT Context,
    _In_ PPOOLTAG_ROOT_NODE PoolTagNode
    )
{
    ULONG index = 0;

    PhRemoveEntryHashtable(Context->NodeHashtable, &PoolTagNode);

    if ((index = PhFindItemList(Context->NodeList, PoolTagNode)) != -1)
    {
        PhRemoveItemList(Context->NodeList, index);
    }

    PmDestroyPoolTagNode(PoolTagNode);

    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID UpdateTracertNode(
    _In_ PNETWORK_TRACERT_CONTEXT Context,
    _In_ PPOOLTAG_ROOT_NODE PoolTagNode
    )
{
    memset(PoolTagNode->TextCache, 0, sizeof(PH_STRINGREF) * TREE_COLUMN_ITEM_MAXIMUM);

    PhInvalidateTreeNewNode(&PoolTagNode->Node, TN_CACHE_COLOR);
    TreeNew_NodesStructured(Context->TreeNewHandle);

    TreeNew_AutoSizeColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_HOSTNAME, TN_AUTOSIZE_REMAINING_SPACE);
}

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
                    SORT_FUNCTION(Ping2),
                    SORT_FUNCTION(Ping3),
                    SORT_FUNCTION(Ping4),
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
                {
                    PhMoveReference(&node->TtlString, PhFormatUInt64(node->TTL, TRUE));
                    getCellText->Text = node->TtlString->sr;
                }
                break;
            case TREE_COLUMN_ITEM_PING1:
                {
                    if (node->Ping1 == ULONG_MAX)
                    {
                        PhMoveReference(&node->Ping1String, PhFormatString(L"*"));
                        getCellText->Text = node->Ping1String->sr;
                    }
                    //else if (node->Ping1 == ULONG_MAX - 1)
                    //{
                    //    PhMoveReference(&node->Ping1String, PhFormatString(L"<1 ms"));
                    //    getCellText->Text = node->Ping1String->sr;
                    //}
                    else if (node->Ping1)
                    {
                        PhMoveReference(&node->Ping1String, PhFormatString(L"%s ms", PhaFormatUInt64(node->Ping1, TRUE)->Buffer));
                        getCellText->Text = node->Ping1String->sr;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_PING2:
                {
                    if (node->Ping2 == ULONG_MAX)
                    {
                        PhMoveReference(&node->Ping2String, PhFormatString(L"*"));
                        getCellText->Text = node->Ping2String->sr;
                    }
                    //else if (node->Ping2 == ULONG_MAX - 1)
                    //{
                        //PhMoveReference(&node->Ping2String, PhFormatString(L"<1 ms"));
                        //getCellText->Text = node->Ping2String->sr;
                    //}
                    else if (node->Ping2)
                    {
                        PhMoveReference(&node->Ping2String, PhFormatString(L"%s ms", PhaFormatUInt64(node->Ping2, TRUE)->Buffer));
                        getCellText->Text = node->Ping2String->sr;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_PING3:
                {
                    if (node->Ping3 == ULONG_MAX)
                    {
                        PhMoveReference(&node->Ping3String, PhFormatString(L"*"));
                        getCellText->Text = node->Ping3String->sr;
                    }
                    //else if (node->Ping3 == ULONG_MAX - 1)
                    //{
                    //    PhMoveReference(&node->Ping3String, PhFormatString(L"<1 ms"));
                    //    getCellText->Text = node->Ping3String->sr;
                    //}
                    else if (node->Ping3)
                    {
                        PhMoveReference(&node->Ping3String, PhFormatString(L"%s ms", PhaFormatUInt64(node->Ping3, TRUE)->Buffer));
                        getCellText->Text = node->Ping3String->sr;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_PING4:
                {
                    if (node->Ping4 == ULONG_MAX)
                    {
                        PhMoveReference(&node->Ping4String, PhFormatString(L"*"));
                        getCellText->Text = node->Ping4String->sr;
                    }
                    //else  if (node->Ping4 == ULONG_MAX - 1)
                    //{
                    //    PhMoveReference(&node->Ping4String, PhFormatString(L"<1 ms"));
                    //    getCellText->Text = node->Ping4String->sr;
                    //}
                    else if (node->Ping4)
                    {
                        PhMoveReference(&node->Ping4String, PhFormatString(L"%s ms", PhaFormatUInt64(node->Ping4, TRUE)->Buffer));
                        getCellText->Text = node->Ping4String->sr;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_COUNTRY:
                {
                    if (node->CountryString)
                    {
                        getCellText->Text = node->CountryString->sr;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_IPADDR:
                {
                    if (node->IpAddressString)
                    {
                        getCellText->Text = node->IpAddressString->sr;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_HOSTNAME:
                {
                    if (node->HostnameString)
                    {
                        getCellText->Text = node->HostnameString->sr;
                    }
                }
                break;
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
            default:
                return FALSE;
            }

            getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = Parameter1;
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
            PPH_TREENEW_MOUSE_EVENT mouseEvent = Parameter1;

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
    case TreeNewCustomDraw:
        {
            PPH_TREENEW_CUSTOM_DRAW customDraw = Parameter1;  
            HDC hdc = customDraw->Dc;
            RECT rect = customDraw->CellRect;

            node = (PPOOLTAG_ROOT_NODE)customDraw->Node;

            // Check if this is the country column
            //if (message->SubId != NETWORK_COLUMN_ID_REMOTE_COUNTRY)
             //   break;

            // Check if there's something to draw
            if (rect.right - rect.left <= 1)
            {
                // nothing to draw
                break;
            }

            // Padding
            rect.left += 5;

            //if (GeoDbLoaded && !node->RemoteCountryCode)
            //{
            //    PPH_STRING remoteCountryCode;
            //    PPH_STRING remoteCountryName;

            //    if (LookupCountryCode(
            //        node->RemoteEndpoint.Address,
            //        &remoteCountryCode,
            //        &remoteCountryName
            //        ))
            //    {
            //        PhSwapReference(&extension->RemoteCountryCode, remoteCountryCode);
            //        PhSwapReference(&extension->RemoteCountryName, remoteCountryName);
            //    }
            //}

            // Draw the column data
            if (GeoDbLoaded && node->RemoteCountryCode && node->RemoteCountryName)
            {
                if (!node->CountryIcon)
                {
                    INT resourceCode;

                    if ((resourceCode = LookupResourceCode(node->RemoteCountryCode)) != 0)
                    {
                        HBITMAP countryBitmap;

                        if (countryBitmap = LoadImageFromResources(PluginInstance->DllBase, 16, 11, MAKEINTRESOURCE(resourceCode), TRUE))
                        {
                            node->CountryIcon = CommonBitmapToIcon(countryBitmap, 16, 11);
                        }
                    }
                }

                if (node->CountryIcon)
                {
                    DrawIconEx(
                        hdc,
                        rect.left,
                        rect.top + ((rect.bottom - rect.top) - 11) / 2,
                        node->CountryIcon,
                        16,
                        11,
                        0,
                        NULL,
                        DI_NORMAL
                        );

                    rect.left += 16 + 2;
                }

                DrawText(
                    hdc,
                    PhGetStringOrEmpty(node->RemoteCountryName),
                    -1,
                    &rect,
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE
                    );
            }

            if (GeoDbExpired && !node->CountryIcon)
            {
                DrawText(hdc, L"Geoip database expired.", -1, &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            }

            if (!GeoDbLoaded)
            {
                DrawText(hdc, L"Geoip database error.", -1, &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            }
        }
        break;
    }

    return FALSE;
}

VOID ClearTracertTree(
    _In_ PNETWORK_TRACERT_CONTEXT Context
    )
{
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
        PmDestroyPoolTagNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);
}

PPOOLTAG_ROOT_NODE GetSelectedTracertNode(
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

VOID GetSelectedTracertNodes(
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

VOID InitializeTracertTree(
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
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_HOSTNAME, TRUE, L"Hostname", 150, PH_ALIGN_LEFT, TREE_COLUMN_ITEM_HOSTNAME, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_PING1, TRUE, L"Time", 50, PH_ALIGN_RIGHT, TREE_COLUMN_ITEM_PING1, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_PING2, TRUE, L"Time", 50, PH_ALIGN_RIGHT, TREE_COLUMN_ITEM_PING2, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_PING3, TRUE, L"Time", 50, PH_ALIGN_RIGHT, TREE_COLUMN_ITEM_PING3, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_PING4, TRUE, L"Time", 50, PH_ALIGN_RIGHT, TREE_COLUMN_ITEM_PING4, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, TREE_COLUMN_ITEM_IPADDR, TRUE, L"IP Address", 120, PH_ALIGN_LEFT, TREE_COLUMN_ITEM_IPADDR, 0);
    PhAddTreeNewColumnEx2(Context->TreeNewHandle, TREE_COLUMN_ITEM_COUNTRY, TRUE, L"Country", 130, PH_ALIGN_LEFT, TREE_COLUMN_ITEM_COUNTRY, 0, TN_COLUMN_FLAG_CUSTOMDRAW);

    //for (INT i = 0; i < MAX_PINGS; i++)
    //    PhAddTreeNewColumn(context->TreeNewHandle, i + 1, i + 1, i + 1, LVCFMT_RIGHT, 50, L"Time");

    TreeNew_SetTriState(Context->TreeNewHandle, TRUE);
    TreeNew_SetSort(Context->TreeNewHandle, TREE_COLUMN_ITEM_TTL, AscendingSortOrder);

    PmLoadSettingsTreeList(Context);
}

VOID DeleteTracertTree(
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