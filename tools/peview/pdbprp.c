/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017-2022
 *
 */

#include <peview.h>
#include <emenu.h>
#include "colmgr.h"

static PH_STRINGREF EmptySymbolsText = PH_STRINGREF_INIT(L"There are no symbols to display.");
static PH_STRINGREF LoadingSymbolsText = PH_STRINGREF_INIT(L"Loading symbols from image...");

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
    PPV_SYMBOL_NODE node1 = *(PPV_SYMBOL_NODE *)Entry1;
    PPV_SYMBOL_NODE node2 = *(PPV_SYMBOL_NODE *)Entry2;

    return PhEqualString(node1->Name, node2->Name, TRUE);
}

ULONG SymbolNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashStringRef(&(*(PPV_SYMBOL_NODE*)Entry)->Name->sr, TRUE);
}

VOID PvSymbolAddTreeNode(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ PPV_SYMBOL_NODE Entry
    )
{
    PhInitializeTreeNewNode(&Entry->Node);

    memset(Entry->TextCache, 0, sizeof(PH_STRINGREF) * TREE_COLUMN_ITEM_MAXIMUM);
    Entry->Node.TextCache = Entry->TextCache;
    Entry->Node.TextCacheSize = TREE_COLUMN_ITEM_MAXIMUM;

    if (PhAddEntryHashtable(Context->NodeHashtable, &Entry)) // HACK
    {
        PhAddItemList(Context->NodeList, Entry);

        if (Context->FilterSupport.NodeList)
        {
            Entry->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->FilterSupport, &Entry->Node);
        }
    }
}

PPV_SYMBOL_NODE PvFindSymbolNode(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ PPH_STRING Name
    )
{
    PV_SYMBOL_NODE lookupSymbolNode;
    PPV_SYMBOL_NODE lookupSymbolNodePtr = &lookupSymbolNode;
    PPV_SYMBOL_NODE *threadNode;

    lookupSymbolNode.Name = Name;

    threadNode = (PPV_SYMBOL_NODE *)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupSymbolNodePtr
        );

    if (threadNode)
        return *threadNode;
    else
        return NULL;
}

VOID PvRemoveSymbolNode(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ PPV_SYMBOL_NODE Node
    )
{
    ULONG index = 0;

    PhRemoveEntryHashtable(Context->NodeHashtable, &Node);

    if ((index = PhFindItemList(Context->NodeList, Node)) != ULONG_MAX)
        PhRemoveItemList(Context->NodeList, index);

    PvDestroySymbolNode(Node);
}

VOID PvDestroySymbolNode(
    _In_ PPV_SYMBOL_NODE Node
    )
{
    PhFree(Node);
}

#define SORT_FUNCTION(Column) PvSymbolTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PvSymbolTreeNewCompare##Column( \
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
        sortResult = uintptrcmp((ULONG_PTR)node1->UniqueId, (ULONG_PTR)node2->UniqueId); \
    \
    return PhModifySort(sortResult, ((PPDB_SYMBOL_CONTEXT)_context)->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Index)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->UniqueId, (ULONG_PTR)node2->UniqueId);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Type)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->Type, (ULONG_PTR)node2->Type);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(VA)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->Address, (ULONG_PTR)node2->Address);

    if (sortResult == 0)
        sortResult = uintptrcmp((ULONG_PTR)node1->Node.Index, (ULONG_PTR)node2->Node.Index);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Symbol)
{
    sortResult = PhCompareStringWithNull(node1->Name, node2->Name, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Data)
{
    sortResult = PhCompareStringWithNull(node1->Data, node2->Data, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Size)
{
    sortResult = uint64cmp(node1->Size, node2->Size);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PvSymbolTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PPDB_SYMBOL_CONTEXT context = Context;
    PPV_SYMBOL_NODE node;

    if (!context)
        return FALSE;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            if (!getChildren)
                break;

            node = (PPV_SYMBOL_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Index),
                    SORT_FUNCTION(Type),
                    SORT_FUNCTION(VA),
                    SORT_FUNCTION(Symbol),
                    SORT_FUNCTION(Data),
                    SORT_FUNCTION(Size)
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

            if (!isLeaf)
                break;

            node = (PPV_SYMBOL_NODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;

            if (!getCellText)
                break;

            node = (PPV_SYMBOL_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case TREE_COLUMN_ITEM_INDEX:
                PhInitializeStringRefLongHint(&getCellText->Text, node->Index);
                break;
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

            if (!getNodeColor)
                break;

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
            PPH_TREENEW_CONTEXT_MENU contextMenu = Parameter1;

            SendMessage(context->ParentWindowHandle, WM_PV_SEARCH_SHOWMENU, 0, (LPARAM)contextMenu);
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
        PPV_SYMBOL_NODE node = Context->NodeList->Items[i];

        if (node->Node.Selected)
            return node;
    }

    return NULL;
}

BOOLEAN PvGetSelectedSymbolNodes(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _Out_ PPV_SYMBOL_NODE **Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPV_SYMBOL_NODE node = (PPV_SYMBOL_NODE)Context->NodeList->Items[i];

        if (node->Node.Selected)
            PhAddItemList(list, node);
    }

    if (list->Count)
    {
        *Nodes = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
        *NumberOfNodes = list->Count;

        PhDereferenceObject(list);
        return TRUE;
    }

    PhDereferenceObject(list);
    return FALSE;
}

VOID PvInitializeSymbolTree(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle
    )
{
    PPH_STRING settings;

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
    TreeNew_SetRedraw(TreeNewHandle, FALSE);

    PhAddTreeNewColumnEx2(TreeNewHandle, TREE_COLUMN_ITEM_INDEX, TRUE, L"#", 80, PH_ALIGN_LEFT, TREE_COLUMN_ITEM_INDEX, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, TREE_COLUMN_ITEM_TYPE, TRUE, L"Type", 80, PH_ALIGN_LEFT, TREE_COLUMN_ITEM_TYPE, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, TREE_COLUMN_ITEM_VA, TRUE, L"RVA", 80, PH_ALIGN_LEFT, TREE_COLUMN_ITEM_VA, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, TREE_COLUMN_ITEM_NAME, TRUE, L"Symbol", 150, PH_ALIGN_LEFT, TREE_COLUMN_ITEM_NAME, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, TREE_COLUMN_ITEM_SYMBOL, TRUE, L"Data", 150, PH_ALIGN_LEFT, TREE_COLUMN_ITEM_SYMBOL, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, TREE_COLUMN_ITEM_SIZE, TRUE, L"Size", 40, PH_ALIGN_LEFT, TREE_COLUMN_ITEM_SIZE, 0, 0);

    TreeNew_SetRedraw(TreeNewHandle, TRUE);
    TreeNew_SetSort(TreeNewHandle, TREE_COLUMN_ITEM_INDEX, AscendingSortOrder);

    settings = PhGetStringSetting(L"PdbTreeListColumns");
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

    remainingPart = PhGetStringRef(Context->SearchboxText);

    while (remainingPart.Length)
    {
        PhSplitStringRefAtChar(&remainingPart, L'|', &part, &remainingPart);

        if (part.Length)
        {
            if (PhFindStringInStringRef(Text, &part, TRUE) != SIZE_MAX)
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

    if (node->Pointer[0])
    {
        if (WordMatchStringZ(context, node->Pointer))
            return TRUE;
    }

    return FALSE;
}

VOID PvAddPendingSymbolNodes(
    _In_ PPDB_SYMBOL_CONTEXT Context
    )
{
    ULONG i;
    BOOLEAN needsFullUpdate = FALSE;

    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    PhAcquireQueuedLockExclusive(&SearchResultsLock);

    for (i = SearchResultsAddIndex; i < SearchResults->Count; i++)
    {
        PvSymbolAddTreeNode(Context, SearchResults->Items[i]);
        needsFullUpdate = TRUE;
    }
    SearchResultsAddIndex = i;

    PhReleaseQueuedLockExclusive(&SearchResultsLock);

    if (needsFullUpdate)
        TreeNew_NodesStructured(Context->TreeNewHandle);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);
}

VOID CALLBACK PvSymbolTreeUpdateCallback(
    _In_ PPDB_SYMBOL_CONTEXT Context,
    _In_ BOOLEAN TimerOrWaitFired
    )
{
    if (!Context->UpdateTimerHandle)
        return;

    PvAddPendingSymbolNodes(Context);

    RtlUpdateTimer(PhGetGlobalTimerQueue(), Context->UpdateTimerHandle, 1000, INFINITE);
}

INT_PTR CALLBACK PvpSymbolsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPDB_SYMBOL_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PDB_SYMBOL_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);

        if (lParam)
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            context->PropSheetContext = (PPV_PROPPAGECONTEXT)propSheetPage->lParam;
        }
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_TREELIST);
            context->SearchHandle = GetDlgItem(hwndDlg, IDC_TREESEARCH);
            context->SearchboxText = PhReferenceEmptyString();

            PvCreateSearchControl(context->SearchHandle, L"Search Symbols (Ctrl+K)");

            PvInitializeSymbolTree(context, hwndDlg, context->TreeNewHandle);
            PvConfigTreeBorders(context->TreeNewHandle);
            PhAddTreeNewFilter(GetSymbolListFilterSupport(context), PvSymbolTreeFilterCallback, context);

            SearchResults = PhCreateList(0x1000);
            context->UdtList = PhCreateList(0x100);

            TreeNew_SetEmptyText(context->TreeNewHandle, &LoadingSymbolsText, 0);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->SearchHandle, NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);

            PhCreateThread2(PeDumpFileSymbols, context);

            RtlCreateTimer(
                PhGetGlobalTimerQueue(),
                &context->UpdateTimerHandle,
                PvSymbolTreeUpdateCallback,
                context,
                0,
                1000,
                0
                );

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            if (context->UpdateTimerHandle)
            {
                RtlDeleteTimer(PhGetGlobalTimerQueue(), context->UpdateTimerHandle, NULL);
                context->UpdateTimerHandle = NULL;
            }

            PvDeleteSymbolTree(context);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (context->PropSheetContext && !context->PropSheetContext->LayoutInitialized)
            {
                PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                context->PropSheetContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
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
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)context->TreeNewHandle);
                return TRUE;
            }
        }
        break;
    case WM_PV_SEARCH_FINISHED:
        {
            if (context->UpdateTimerHandle)
            {
                RtlDeleteTimer(PhGetGlobalTimerQueue(), context->UpdateTimerHandle, NULL);
                context->UpdateTimerHandle = NULL;
            }

            PvAddPendingSymbolNodes(context);

            TreeNew_SetEmptyText(context->TreeNewHandle, &EmptySymbolsText, 0);

            TreeNew_NodesStructured(context->TreeNewHandle);
        }
        break;
    case WM_PV_SEARCH_SHOWMENU:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
            PPH_EMENU menu;
            PPH_EMENU_ITEM selectedItem;
            PPV_SYMBOL_NODE *symbolNodes = NULL;
            ULONG numberOfNodes = 0;

            if (!PvGetSelectedSymbolNodes(context, &symbolNodes, &numberOfNodes))
                break;

            if (numberOfNodes != 0)
            {
                menu = PhCreateEMenu();
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"Copy", NULL, NULL), ULONG_MAX);
                PhInsertCopyCellEMenuItem(menu, 1, context->TreeNewHandle, contextMenuEvent->Column);

                selectedItem = PhShowEMenu(
                    menu,
                    hwndDlg,
                    PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                    PH_ALIGN_LEFT | PH_ALIGN_TOP,
                    contextMenuEvent->Location.x,
                    contextMenuEvent->Location.y
                    );

                if (selectedItem && selectedItem->Id != ULONG_MAX)
                {
                    BOOLEAN handled = FALSE;

                    handled = PhHandleCopyCellEMenuItem(selectedItem);

                    if (!handled && selectedItem->Id == 1)
                    {
                        PPH_STRING text;

                        text = PhGetTreeNewText(context->TreeNewHandle, 0);
                        PhSetClipboardString(context->TreeNewHandle, &text->sr);
                        PhDereferenceObject(text);
                    }
                }

                PhDestroyEMenu(menu);
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

    if (!PhDoesFileExistWin32(PhGetString(PvFileName)))
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

        // CLR page for v1.0 PDBs which are CLR metadata images. (dmex)
        {
            HANDLE fileHandle;

            if (NT_SUCCESS(PhCreateFileWin32(
                &fileHandle,
                PhGetString(PvFileName),
                FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                )))
            {
                PVOID viewBase;
                SIZE_T size;

                if (NT_SUCCESS(PhMapViewOfEntireFile(
                    PhGetString(PvFileName),
                    fileHandle,
                    &viewBase,
                    &size
                    )))
                {
                    if (size > sizeof(ULONG) && RtlEqualMemory(viewBase, "BSJB", 4)) // BSJB signature 0x424a5342
                    {
                        newPage = PvCreatePropPageContext(
                            MAKEINTRESOURCE(IDD_PECLR),
                            PvpPeClrDlgProc,
                            viewBase
                            );
                        PvAddPropPage(propContext, newPage);
                    }
                    else
                    {
                        NtUnmapViewOfSection(NtCurrentProcess(), viewBase);
                    }
                }

                NtClose(fileHandle);
            }
        }

        PhModalPropertySheet(&propContext->PropSheetHeader);

        PhDereferenceObject(propContext);
    }
}
