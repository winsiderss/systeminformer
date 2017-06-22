/*
 * PE viewer -
 *   pdb support
 *
 * Copyright (C) 2017 dmex
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

#include <peview.h>
#include <pdb.h>
#include <emenu.h>
#include "colmgr.h"

VOID PhInitializeTreeNewColumnMenu(
    _Inout_ PPH_TN_COLUMN_MENU_DATA Data
    )
{
    PhInitializeTreeNewColumnMenuEx(Data, 0);
}

VOID PhInitializeTreeNewColumnMenuEx(
    _Inout_ PPH_TN_COLUMN_MENU_DATA Data,
    _In_ ULONG Flags
    )
{
    PPH_EMENU_ITEM resetSortMenuItem = NULL;
    PPH_EMENU_ITEM sizeColumnToFitMenuItem;
    PPH_EMENU_ITEM sizeAllColumnsToFitMenuItem;
    PPH_EMENU_ITEM hideColumnMenuItem = NULL;
    PPH_EMENU_ITEM chooseColumnsMenuItem = NULL;
    ULONG minimumNumberOfColumns;

    Data->Menu = PhCreateEMenu();
    Data->Selection = NULL;
    Data->ProcessedId = 0;

    sizeColumnToFitMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_SIZE_COLUMN_TO_FIT_ID, L"Size column to fit", NULL, NULL);
    sizeAllColumnsToFitMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_SIZE_ALL_COLUMNS_TO_FIT_ID, L"Size all columns to fit", NULL, NULL);

    if (!(Flags & PH_TN_COLUMN_MENU_NO_VISIBILITY))
    {
        hideColumnMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_HIDE_COLUMN_ID, L"Hide column", NULL, NULL);
        chooseColumnsMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_CHOOSE_COLUMNS_ID, L"Choose columns...", NULL, NULL);
    }

    if (Flags & PH_TN_COLUMN_MENU_SHOW_RESET_SORT)
    {
        ULONG sortColumn;
        PH_SORT_ORDER sortOrder;

        TreeNew_GetSort(Data->TreeNewHandle, &sortColumn, &sortOrder);

        if (sortOrder != Data->DefaultSortOrder || (Data->DefaultSortOrder != NoSortOrder && sortColumn != Data->DefaultSortColumn))
            resetSortMenuItem = PhCreateEMenuItem(0, PH_TN_COLUMN_MENU_RESET_SORT_ID, L"Reset sort", NULL, NULL);
    }

    PhInsertEMenuItem(Data->Menu, sizeColumnToFitMenuItem, -1);
    PhInsertEMenuItem(Data->Menu, sizeAllColumnsToFitMenuItem, -1);

    if (!(Flags & PH_TN_COLUMN_MENU_NO_VISIBILITY))
    {
        PhInsertEMenuItem(Data->Menu, hideColumnMenuItem, -1);

        if (resetSortMenuItem)
            PhInsertEMenuItem(Data->Menu, resetSortMenuItem, -1);

        PhInsertEMenuItem(Data->Menu, PhCreateEMenuItem(PH_EMENU_SEPARATOR, 0, L"", NULL, NULL), -1);
        PhInsertEMenuItem(Data->Menu, chooseColumnsMenuItem, -1);

        if (TreeNew_GetFixedColumn(Data->TreeNewHandle))
            minimumNumberOfColumns = 2; // don't allow user to remove all normal columns (the fixed column can never be removed)
        else
            minimumNumberOfColumns = 1;

        if (!Data->MouseEvent || !Data->MouseEvent->Column ||
            Data->MouseEvent->Column->Fixed || // don't allow the fixed column to be hidden
            TreeNew_GetVisibleColumnCount(Data->TreeNewHandle) < minimumNumberOfColumns + 1
            )
        {
            hideColumnMenuItem->Flags |= PH_EMENU_DISABLED;
        }
    }
    else
    {
        if (resetSortMenuItem)
            PhInsertEMenuItem(Data->Menu, resetSortMenuItem, -1);
    }

    if (!Data->MouseEvent || !Data->MouseEvent->Column)
    {
        sizeColumnToFitMenuItem->Flags |= PH_EMENU_DISABLED;
    }
}

VOID PhpEnsureValidSortColumnTreeNew(
    _Inout_ HWND TreeNewHandle,
    _In_ ULONG DefaultSortColumn,
    _In_ PH_SORT_ORDER DefaultSortOrder
    )
{
    ULONG sortColumn;
    PH_SORT_ORDER sortOrder;

    // Make sure the column we're sorting by is actually visible, and if not, don't sort anymore.

    TreeNew_GetSort(TreeNewHandle, &sortColumn, &sortOrder);

    if (sortOrder != NoSortOrder)
    {
        PH_TREENEW_COLUMN column;

        TreeNew_GetColumn(TreeNewHandle, sortColumn, &column);

        if (!column.Visible)
        {
            if (DefaultSortOrder != NoSortOrder)
            {
                // Make sure the default sort column is visible.
                TreeNew_GetColumn(TreeNewHandle, DefaultSortColumn, &column);

                if (!column.Visible)
                {
                    ULONG maxId;
                    ULONG id;
                    BOOLEAN found;

                    // Use the first visible column.
                    maxId = TreeNew_GetMaxId(TreeNewHandle);
                    id = 0;
                    found = FALSE;

                    while (id <= maxId)
                    {
                        if (TreeNew_GetColumn(TreeNewHandle, id, &column))
                        {
                            if (column.Visible)
                            {
                                DefaultSortColumn = id;
                                found = TRUE;
                                break;
                            }
                        }

                        id++;
                    }

                    if (!found)
                    {
                        DefaultSortColumn = 0;
                        DefaultSortOrder = NoSortOrder;
                    }
                }
            }

            TreeNew_SetSort(TreeNewHandle, DefaultSortColumn, DefaultSortOrder);
        }
    }
}

BOOLEAN PhHandleTreeNewColumnMenu(
    _Inout_ PPH_TN_COLUMN_MENU_DATA Data
    )
{
    if (!Data->Selection)
        return FALSE;

    switch (Data->Selection->Id)
    {
    case PH_TN_COLUMN_MENU_RESET_SORT_ID:
        {
            TreeNew_SetSort(Data->TreeNewHandle, Data->DefaultSortColumn, Data->DefaultSortOrder);
        }
        break;
    case PH_TN_COLUMN_MENU_SIZE_COLUMN_TO_FIT_ID:
        {
            if (Data->MouseEvent && Data->MouseEvent->Column)
            {
                TreeNew_AutoSizeColumn(Data->TreeNewHandle, Data->MouseEvent->Column->Id, 0);
            }
        }
        break;
    case PH_TN_COLUMN_MENU_SIZE_ALL_COLUMNS_TO_FIT_ID:
        {
            ULONG maxId;
            ULONG id;

            maxId = TreeNew_GetMaxId(Data->TreeNewHandle);
            id = 0;

            while (id <= maxId)
            {
                TreeNew_AutoSizeColumn(Data->TreeNewHandle, id, 0);
                id++;
            }
        }
        break;
    case PH_TN_COLUMN_MENU_HIDE_COLUMN_ID:
        {
            PH_TREENEW_COLUMN column;

            if (Data->MouseEvent && Data->MouseEvent->Column && !Data->MouseEvent->Column->Fixed)
            {
                column.Id = Data->MouseEvent->Column->Id;
                column.Visible = FALSE;
                TreeNew_SetColumn(Data->TreeNewHandle, TN_COLUMN_FLAG_VISIBLE, &column);
                PhpEnsureValidSortColumnTreeNew(Data->TreeNewHandle, Data->DefaultSortColumn, Data->DefaultSortOrder);
                InvalidateRect(Data->TreeNewHandle, NULL, FALSE);
            }
        }
        break;
    case PH_TN_COLUMN_MENU_CHOOSE_COLUMNS_ID:
        {
            //PhShowChooseColumnsDialog(Data->TreeNewHandle, Data->TreeNewHandle, PH_CONTROL_TYPE_TREE_NEW);
            PhpEnsureValidSortColumnTreeNew(Data->TreeNewHandle, Data->DefaultSortColumn, Data->DefaultSortOrder);
        }
        break;
    default:
        return FALSE;
    }

    Data->ProcessedId = Data->Selection->Id;

    return TRUE;
}

VOID PhDeleteTreeNewColumnMenu(
    _In_ PPH_TN_COLUMN_MENU_DATA Data
    )
{
    if (Data->Menu)
    {
        PhDestroyEMenu(Data->Menu);
        Data->Menu = NULL;
    }
}

VOID PhInitializeTreeNewFilterSupport(
    _Out_ PPH_TN_FILTER_SUPPORT Support,
    _In_ HWND TreeNewHandle,
    _In_ PPH_LIST NodeList
    )
{
    Support->FilterList = NULL;
    Support->TreeNewHandle = TreeNewHandle;
    Support->NodeList = NodeList;
}

VOID PhDeleteTreeNewFilterSupport(
    _In_ PPH_TN_FILTER_SUPPORT Support
    )
{
    PhDereferenceObject(Support->FilterList);
}

PPH_TN_FILTER_ENTRY PhAddTreeNewFilter(
    _In_ PPH_TN_FILTER_SUPPORT Support,
    _In_ PPH_TN_FILTER_FUNCTION Filter,
    _In_opt_ PVOID Context
    )
{
    PPH_TN_FILTER_ENTRY entry;

    entry = PhAllocate(sizeof(PH_TN_FILTER_ENTRY));
    entry->Filter = Filter;
    entry->Context = Context;

    if (!Support->FilterList)
        Support->FilterList = PhCreateList(2);

    PhAddItemList(Support->FilterList, entry);

    return entry;
}

VOID PhRemoveTreeNewFilter(
    _In_ PPH_TN_FILTER_SUPPORT Support,
    _In_ PPH_TN_FILTER_ENTRY Entry
    )
{
    ULONG index;

    if (!Support->FilterList)
        return;

    index = PhFindItemList(Support->FilterList, Entry);

    if (index != -1)
    {
        PhRemoveItemList(Support->FilterList, index);
        PhFree(Entry);
    }
}

BOOLEAN PhApplyTreeNewFiltersToNode(
    _In_ PPH_TN_FILTER_SUPPORT Support,
    _In_ PPH_TREENEW_NODE Node
    )
{
    BOOLEAN show;
    ULONG i;

    show = TRUE;

    if (Support->FilterList)
    {
        for (i = 0; i < Support->FilterList->Count; i++)
        {
            PPH_TN_FILTER_ENTRY entry;

            entry = Support->FilterList->Items[i];

            if (!entry->Filter(Node, entry->Context))
            {
                show = FALSE;
                break;
            }
        }
    }

    return show;
}

VOID PhApplyTreeNewFilters(
    _In_ PPH_TN_FILTER_SUPPORT Support
    )
{
    ULONG i;

    for (i = 0; i < Support->NodeList->Count; i++)
    {
        PPH_TREENEW_NODE node;

        node = Support->NodeList->Items[i];
        node->Visible = PhApplyTreeNewFiltersToNode(Support, node);

        if (!node->Visible && node->Selected)
        {
            node->Selected = FALSE;
        }
    }

    TreeNew_NodesStructured(Support->TreeNewHandle);
}

BOOLEAN SymbolNodeHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );
ULONG SymbolNodeHashtableHashFunction(
    _In_ PVOID Entry
    );
VOID PvDestroySymbolNode(
    _In_ PPV_SYMBOL_NODE Node
    );

VOID PvDeleteSymbolTree(
    _In_ PPDB_SYMBOL_CONTEXT Context
    )
{
    PPH_STRING settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(L"PdbTreeListColumns", &settings->sr);
    PhDereferenceObject(settings);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PvDestroySymbolNode(Context->NodeList->Items[i]);
    }

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}

struct _PH_TN_FILTER_SUPPORT* GetSymbolListFilterSupport(
    _In_ PPDB_SYMBOL_CONTEXT Context
    )
{
    return &Context->FilterSupport;
}

BOOLEAN SymbolNodeHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPV_SYMBOL_NODE windowNode1 = *(PPV_SYMBOL_NODE *)Entry1;
    PPV_SYMBOL_NODE windowNode2 = *(PPV_SYMBOL_NODE *)Entry2;

    return PhEqualString(windowNode1->Name, windowNode2->Name, TRUE);
}

ULONG SymbolNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashInt64((*(PPV_SYMBOL_NODE*)Entry)->Index);
}

VOID PvSymbolAddTreeNode(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ PPV_SYMBOL_NODE Entry
    )
{
    static ULONG64 index = 0;

    PhInitializeTreeNewNode(&Entry->Node);

    Entry->Index = index++;
    memset(Entry->TextCache, 0, sizeof(PH_STRINGREF) * TREE_COLUMN_ITEM_MAXIMUM);
    Entry->Node.TextCache = Entry->TextCache;
    Entry->Node.TextCacheSize = TREE_COLUMN_ITEM_MAXIMUM;

    PhAddEntryHashtable(Context->NodeHashtable, &Entry);
    PhAddItemList(Context->NodeList, Entry);

    if (Context->FilterSupport.NodeList)
    {
        Entry->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->FilterSupport, &Entry->Node);
    }
}

PPV_SYMBOL_NODE PvFindSymbolNode(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ PPH_STRING Name
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPV_SYMBOL_NODE entry = Context->NodeList->Items[i];

        if (PhEqualString(entry->Name, Name, TRUE))
            return entry;
    }

    return NULL;
}

VOID PvRemoveSymbolNode(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ PPV_SYMBOL_NODE Node
    )
{
    ULONG index = 0;

    PhRemoveEntryHashtable(Context->NodeHashtable, &Node);

    if ((index = PhFindItemList(Context->NodeList, Node)) != -1)
        PhRemoveItemList(Context->NodeList, index);

    PvDestroySymbolNode(Node);
}

VOID PvDestroySymbolNode(
    _In_ PPV_SYMBOL_NODE Node
    )
{
    PhFree(Node);
}

#define SORT_FUNCTION(Column) PmPoolTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PmPoolTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPV_SYMBOL_NODE node1 = *(PPV_SYMBOL_NODE *)_elem1; \
    PPV_SYMBOL_NODE node2 = *(PPV_SYMBOL_NODE *)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)node1->Node.Index, (ULONG_PTR)node2->Node.Index); \
    \
    return PhModifySort(sortResult, ((PPDB_SYMBOL_CONTEXT)_context)->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Symbol)
{
    sortResult = PhCompareString(node1->Name, node2->Name, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(VA)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->Address, (ULONG_PTR)node2->Address);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareString(node1->Name, node2->Name, FALSE);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PvSymbolTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2,
    __in_opt PVOID Context
    )
{
    PPDB_SYMBOL_CONTEXT context;
    PPV_SYMBOL_NODE node;

    context = Context;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PPV_SYMBOL_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Name),
                    SORT_FUNCTION(VA),
                    SORT_FUNCTION(Symbol)
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
            node = (PPV_SYMBOL_NODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;
            node = (PPV_SYMBOL_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case TREE_COLUMN_ITEM_TYPE:
                {
                    switch (node->Type)
                    {
                    case PV_SYMBOL_TYPE_FUNCTION:
                        PhInitializeStringRef(&getCellText->Text, L"FUNCTION");
                        break;
                    case PV_SYMBOL_TYPE_SYMBOL:
                        PhInitializeStringRef(&getCellText->Text, L"SYMBOL");
                        break;
                    case PV_SYMBOL_TYPE_LOCAL_VAR:
                        PhInitializeStringRef(&getCellText->Text, L"LOCAL_VAR");
                        break;
                    case PV_SYMBOL_TYPE_STATIC_LOCAL_VAR:
                        PhInitializeStringRef(&getCellText->Text, L"STATIC_LOCAL_VAR");
                        break;
                    case PV_SYMBOL_TYPE_PARAMETER:
                        PhInitializeStringRef(&getCellText->Text, L"PARAMETER");
                        break;
                    case PV_SYMBOL_TYPE_OBJECT_PTR:
                        PhInitializeStringRef(&getCellText->Text, L"OBJECT_PTR");
                        break;
                    case PV_SYMBOL_TYPE_STATIC_VAR:
                        PhInitializeStringRef(&getCellText->Text, L"STATIC_VAR");
                        break;
                    case PV_SYMBOL_TYPE_GLOBAL_VAR:
                        PhInitializeStringRef(&getCellText->Text, L"GLOBAL_VAR");
                        break;
                    case PV_SYMBOL_TYPE_STRUCT:
                        PhInitializeStringRef(&getCellText->Text, L"STRUCT");
                        break;
                    case PV_SYMBOL_TYPE_STATIC_MEMBER:
                        PhInitializeStringRef(&getCellText->Text, L"STATIC_MEMBER");
                        break;
                    case PV_SYMBOL_TYPE_CONSTANT:
                        PhInitializeStringRef(&getCellText->Text, L"CONSTANT");
                        break;
                    case PV_SYMBOL_TYPE_CLASS:
                        PhInitializeStringRef(&getCellText->Text, L"CLASS");
                        break;
                    case PV_SYMBOL_TYPE_MEMBER:
                        PhInitializeStringRef(&getCellText->Text, L"MEMBER");
                        break;
                    }
                }
                break;
            case TREE_COLUMN_ITEM_VA:
                PhInitializeStringRefLongHint(&getCellText->Text, node->Pointer);
                break;
            case TREE_COLUMN_ITEM_NAME:
                getCellText->Text = PhGetStringRef(node->Name);
                break;
            case TREE_COLUMN_ITEM_SYMBOL:
                getCellText->Text = PhGetStringRef(node->Data);
                break;
            case TREE_COLUMN_ITEM_SIZE:
                {
                    if (node->Size != 0)
                    {
                        PH_FORMAT format[1];

                        PhInitFormatSize(&format[0], node->Size);
  
                        PhMoveReference(&node->SizeText, PhFormat(format, 1, 0));
                        getCellText->Text = node->SizeText->sr;
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
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = (PPH_TREENEW_GET_NODE_COLOR)Parameter1;
            node = (PPV_SYMBOL_NODE)getNodeColor->Node;

            getNodeColor->Flags = TN_CACHE | TN_AUTO_FORECOLOR;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            TreeNew_GetSort(hwnd, &context->TreeNewSortColumn, &context->TreeNewSortOrder);
            TreeNew_NodesStructured(hwnd);
        }
        return TRUE;
    case TreeNewKeyDown:
    case TreeNewNodeExpanding:
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
           // SendMessage(context->ParentWindowHandle, WM_COMMAND, WM_ACTION, (LPARAM)context);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_MOUSE_EVENT mouseEvent = (PPH_TREENEW_MOUSE_EVENT)Parameter1;

            //SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_WCTSHOWCONTEXTMENU, MAKELONG(mouseEvent->Location.x, mouseEvent->Location.y));
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

VOID PvSymbolClearTree(
    _In_ PPDB_SYMBOL_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        PvDestroySymbolNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);
}

PPV_SYMBOL_NODE PvGetSelectedSymbolNode(
    _In_ PPDB_SYMBOL_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPV_SYMBOL_NODE windowNode = Context->NodeList->Items[i];

        if (windowNode->Node.Selected)
            return windowNode;
    }

    return NULL;
}

VOID PvGetSelectedSymbolNodes(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _Out_ PPV_SYMBOL_NODE **Windows,
    _Out_ PULONG NumberOfWindows
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPV_SYMBOL_NODE node = (PPV_SYMBOL_NODE)Context->NodeList->Items[i];

        if (node->Node.Selected)
            PhAddItemList(list, node);
    }

    *Windows = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
    *NumberOfWindows = list->Count;

    PhDereferenceObject(list);
}

VOID PvInitializeSymbolTree(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle
    )
{
    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PPV_SYMBOL_NODE),
        SymbolNodeHashtableCompareFunction,
        SymbolNodeHashtableHashFunction,
        100
        );
    Context->NodeList = PhCreateList(100);

    Context->ParentWindowHandle = ParentWindowHandle;
    Context->TreeNewHandle = TreeNewHandle;
    PhSetControlTheme(TreeNewHandle, L"explorer");

    TreeNew_SetCallback(TreeNewHandle, PvSymbolTreeNewCallback, Context);

    PhAddTreeNewColumnEx2(TreeNewHandle, TREE_COLUMN_ITEM_TYPE, TRUE, L"Type", 80, PH_ALIGN_LEFT, TREE_COLUMN_ITEM_TYPE, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, TREE_COLUMN_ITEM_VA, TRUE, L"VA", 80, PH_ALIGN_LEFT, TREE_COLUMN_ITEM_VA, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, TREE_COLUMN_ITEM_NAME, TRUE, L"Symbol", 150, PH_ALIGN_LEFT, TREE_COLUMN_ITEM_NAME, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, TREE_COLUMN_ITEM_SYMBOL, TRUE, L"Data", 150, PH_ALIGN_LEFT, TREE_COLUMN_ITEM_SYMBOL, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, TREE_COLUMN_ITEM_SIZE, FALSE, L"Size", 40, PH_ALIGN_LEFT, TREE_COLUMN_ITEM_SIZE, 0, 0);

    TreeNew_SetSort(TreeNewHandle, 0, NoSortOrder);

    PPH_STRING settings = PhGetStringSetting(L"PdbTreeListColumns");
    PhCmLoadSettings(TreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);

    PhInitializeTreeNewFilterSupport(&Context->FilterSupport, TreeNewHandle, Context->NodeList);
}

BOOLEAN WordMatchStringRef(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ PPH_STRINGREF Text
    )
{
    PH_STRINGREF part;
    PH_STRINGREF remainingPart;

    remainingPart = Context->SearchboxText->sr;

    while (remainingPart.Length)
    {
        PhSplitStringRefAtChar(&remainingPart, '|', &part, &remainingPart);

        if (part.Length)
        {
            if (PhFindStringInStringRef(Text, &part, TRUE) != -1)
                return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN WordMatchStringZ(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ PWSTR Text
    )
{
    PH_STRINGREF text;

    PhInitializeStringRef(&text, Text);
    return WordMatchStringRef(Context, &text);
}

BOOLEAN PvSymbolTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPDB_SYMBOL_CONTEXT context = Context;
    PPV_SYMBOL_NODE node = (PPV_SYMBOL_NODE)Node;

    //if (node->Address == 0)
    //    return TRUE;

    if (PhIsNullOrEmptyString(context->SearchboxText))
        return TRUE;

    switch (node->Type)
    {
    case PV_SYMBOL_TYPE_FUNCTION:
        if (WordMatchStringZ(context, L"FUNCTION"))
            return TRUE;
        break;
    case PV_SYMBOL_TYPE_SYMBOL:
        if (WordMatchStringZ(context, L"SYMBOL"))
            return TRUE;
        break;
    case PV_SYMBOL_TYPE_LOCAL_VAR:
        if (WordMatchStringZ(context, L"LOCAL_VAR"))
            return TRUE;
        break;
    case PV_SYMBOL_TYPE_STATIC_LOCAL_VAR:
        if (WordMatchStringZ(context, L"STATIC_LOCAL_VAR"))
            return TRUE;
        break;
    case PV_SYMBOL_TYPE_PARAMETER:
        if (WordMatchStringZ(context, L"PARAMETER"))
            return TRUE;
        break;
    case PV_SYMBOL_TYPE_OBJECT_PTR:
        if (WordMatchStringZ(context, L"OBJECT_PTR"))
            return TRUE;
        break;
    case PV_SYMBOL_TYPE_STATIC_VAR:
        if (WordMatchStringZ(context, L"STATIC_VAR"))
            return TRUE;
        break;
    case PV_SYMBOL_TYPE_GLOBAL_VAR:
        if (WordMatchStringZ(context, L"GLOBAL_VAR"))
            return TRUE;
        break;
    case PV_SYMBOL_TYPE_STRUCT:
        if (WordMatchStringZ(context, L"MEMBER"))
            return TRUE;
        break;
    case PV_SYMBOL_TYPE_STATIC_MEMBER:
        if (WordMatchStringZ(context, L"STATIC_MEMBER"))
            return TRUE;
        break;
    case PV_SYMBOL_TYPE_CONSTANT:
        if (WordMatchStringZ(context, L"CONSTANT"))
            return TRUE;
        break;
    }

    if (!PhIsNullOrEmptyString(node->Name))
    {
        if (WordMatchStringRef(context, &node->Name->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->Data))
    {
        if (WordMatchStringRef(context, &node->Data->sr))
            return TRUE;
    }

    return FALSE;
}

VOID CALLBACK PvSymbolTreeUpdateCallback(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ BOOLEAN TimerOrWaitFired
    )
{
    ULONG i;

    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    PhAcquireQueuedLockExclusive(&SearchResultsLock);

    for (i = SearchResultsAddIndex; i < SearchResults->Count; i++)
    {
        PvSymbolAddTreeNode(Context, SearchResults->Items[i]);
    }
    SearchResultsAddIndex = i;

    PhReleaseQueuedLockExclusive(&SearchResultsLock);

    TreeNew_NodesStructured(Context->TreeNewHandle);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);

    RtlUpdateTimer(Context->TimerQueueHandle, Context->UpdateTimerHandle, 1000, INFINITE);
}

INT_PTR CALLBACK PvpSymbolsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPV_PROPPAGECONTEXT propPageContext;
    PPDB_SYMBOL_CONTEXT context;

    if (PvPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext))
    {
        context = (PPDB_SYMBOL_CONTEXT)propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HANDLE treeNewTimer = NULL;

            context = propPageContext->Context = PhAllocate(sizeof(PDB_SYMBOL_CONTEXT));
            memset(context, 0, sizeof(PDB_SYMBOL_CONTEXT));

            context->DialogHandle = hwndDlg;
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_SYMBOLTREE);
            context->SearchHandle = GetDlgItem(hwndDlg, IDC_SYMSEARCH);
            context->SearchboxText = PhReferenceEmptyString();

            PvCreateSearchControl(context->SearchHandle, L"Search Symbols (Ctrl+K)");

            PvInitializeSymbolTree(context, hwndDlg, context->TreeNewHandle);
            PhAddTreeNewFilter(GetSymbolListFilterSupport(context), PvSymbolTreeFilterCallback, context);

            PhSetWindowStyle(GetDlgItem(hwndDlg, IDC_PROGRESS), PBS_MARQUEE, PBS_MARQUEE);
            SendMessage(GetDlgItem(hwndDlg, IDC_PROGRESS), PBM_SETMARQUEE, TRUE, 75);

            SearchResults = PhCreateList(0x1000);
            context->UdtList = PhCreateList(0x100);

            PhCreateThread2(PeDumpFileSymbols, context);

            if (NT_SUCCESS(RtlCreateTimerQueue(&context->TimerQueueHandle)))
            {
                RtlCreateTimer(
                    context->TimerQueueHandle,
                    &context->UpdateTimerHandle,
                    PvSymbolTreeUpdateCallback,
                    context,
                    0,
                    1000,
                    0
                    );
            }

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_DESTROY:
        {
            if (context->UpdateTimerHandle)
                RtlDeleteTimer(context->TimerQueueHandle, context->UpdateTimerHandle, NULL);

            if (context->TimerQueueHandle)
                RtlDeleteTimerQueue(context->TimerQueueHandle);

            PvDeleteSymbolTree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, context->SearchHandle,
                    dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PvAddPropPageLayoutItem(hwndDlg, context->TreeNewHandle, dialogItem, PH_ANCHOR_ALL);

                PvDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case EN_CHANGE:
                {
                    PPH_STRING newSearchboxText;

                    newSearchboxText = PH_AUTO(PhGetWindowText(context->SearchHandle));

                    if (!PhEqualString(context->SearchboxText, newSearchboxText, FALSE))
                    {
                        PhSwapReference(&context->SearchboxText, newSearchboxText);

                        if (!PhIsNullOrEmptyString(context->SearchboxText))
                        {
                            //PhExpandAllProcessNodes(TRUE);
                            //PhDeselectAllProcessNodes();
                        }

                        PhApplyTreeNewFilters(GetSymbolListFilterSupport(context));
                    }
                }
                break;
            }
        }
        break;
    case WM_PV_SEARCH_FINISHED:
        {
            // Add any un-added items.
            //SendMessage(hwndDlg, WM_PV_SEARCH_UPDATE, 0, 0);

            //NtWaitForSingleObject(context->SearchThreadHandle, FALSE, NULL);
            //SearchStop = FALSE;
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;
            LPPSHNOTIFY pageNotify = (LPPSHNOTIFY)header;

            switch (pageNotify->hdr.code)
            {
            case PSN_SETACTIVE:
                PostMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(GetParent(hwndDlg), IDCANCEL), TRUE); // HACK
                break;
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)GetDlgItem(GetParent(hwndDlg), IDCANCEL));
                return TRUE;
            }
        }
        break;

    }

    return FALSE;
}

VOID PvPdbProperties(
    VOID
    )
{
    PPV_PROPCONTEXT propContext;

    if (!RtlDoesFileExists_U(PvFileName->Buffer))
    {
        PhShowStatus(NULL, L"Unable to load the pdb file", STATUS_FILE_NOT_AVAILABLE, 0);
        return;
    }

    if (propContext = PvCreatePropContext(PvFileName))
    {
        PPV_PROPPAGECONTEXT newPage;

        newPage = PvCreatePropPageContext(
            MAKEINTRESOURCE(IDD_PESYMBOLS),
            PvpSymbolsDlgProc,
            NULL
            );
        PvAddPropPage(propContext, newPage);

        PhModalPropertySheet(&propContext->PropSheetHeader);

        PhDereferenceObject(propContext);
    }
}
