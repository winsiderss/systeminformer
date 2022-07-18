/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2017-2022
 *
 */

#include <peview.h>
#include "colmgr.h"

static PH_STRINGREF EmptyExportsText = PH_STRINGREF_INIT(L"There are no exports to display.");
static PH_STRINGREF LoadingExportsText = PH_STRINGREF_INIT(L"Loading exports from image...");

typedef enum _PV_EXPORT_TREE_COLUMN_ITEM
{
    PV_EXPORT_TREE_COLUMN_ITEM_INDEX,
    PV_EXPORT_TREE_COLUMN_ITEM_RVA,
    PV_EXPORT_TREE_COLUMN_ITEM_NAME,
    PV_EXPORT_TREE_COLUMN_ITEM_ORDINAL,
    PV_EXPORT_TREE_COLUMN_ITEM_HINT,
    PV_EXPORT_TREE_COLUMN_ITEM_MAXIMUM
} PV_EXPORT_TREE_COLUMN_ITEM;

typedef struct _PV_EXPORT_NODE
{
    PH_TREENEW_NODE Node;

    ULONG64 UniqueId;
    ULONG_PTR Address;
    ULONG Ordinal;
    ULONG Hint;
    BOOLEAN ExportForwarded;
    PPH_STRING UniqueIdString;
    PPH_STRING AddressString;
    PPH_STRING NameString;
    PPH_STRING OrdinalString;
    PPH_STRING HintString;

    PH_STRINGREF TextCache[PV_EXPORT_TREE_COLUMN_ITEM_MAXIMUM];
} PV_EXPORT_NODE, *PPV_EXPORT_NODE;

typedef struct _PV_EXPORT_CONTEXT
{
    HWND WindowHandle;
    HWND SearchHandle;
    HWND TreeNewHandle;
    HWND ParentWindowHandle;

    PPH_STRING SearchboxText;
    PPH_STRING TreeText;

    PPV_PROPPAGECONTEXT PropSheetContext;
    PH_LAYOUT_MANAGER LayoutManager;

    ULONG SearchResultsAddIndex;
    PPH_LIST SearchResults;
    PH_QUEUED_LOCK SearchResultsLock;

    PH_CM_MANAGER Cm;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PH_TN_FILTER_SUPPORT FilterSupport;
    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;
} PV_EXPORT_CONTEXT, *PPV_EXPORT_CONTEXT;

BOOLEAN PvExportNodeHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

ULONG PvExportNodeHashtableHashFunction(
    _In_ PVOID Entry
    );

VOID PvDestroyExportNode(
    _In_ PPV_EXPORT_NODE Node
    );

VOID PvInitializeExportTree(
    _In_ PPV_EXPORT_CONTEXT Context,
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle
    );

VOID PvDeleteExportTree(
    _In_ PPV_EXPORT_CONTEXT Context
    );

VOID PvExportAddTreeNode(
    _In_ PPV_EXPORT_CONTEXT Context,
    _In_ PPV_EXPORT_NODE Entry
    );

BOOLEAN PvExportTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );

VOID PhLoadSettingsExportList(
    _Inout_ PPV_EXPORT_CONTEXT Context
    );

VOID PhSaveSettingsExportList(
    _Inout_ PPV_EXPORT_CONTEXT Context
    );

_Success_(return)
BOOLEAN PvGetSelectedExportNodes(
    _In_ PPV_EXPORT_CONTEXT Context,
    _Out_ PPV_EXPORT_NODE **Nodes,
    _Out_ PULONG NumberOfNodes
    );

VOID PvAddPendingExportNodes(
    _In_ PPV_EXPORT_CONTEXT Context
    )
{
    ULONG i;
    BOOLEAN needsFullUpdate = FALSE;

    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    PhAcquireQueuedLockExclusive(&Context->SearchResultsLock);

    for (i = Context->SearchResultsAddIndex; i < Context->SearchResults->Count; i++)
    {
        PvExportAddTreeNode(Context, Context->SearchResults->Items[i]);
        needsFullUpdate = TRUE;
    }
    Context->SearchResultsAddIndex = i;

    PhReleaseQueuedLockExclusive(&Context->SearchResultsLock);

    if (needsFullUpdate)
        TreeNew_NodesStructured(Context->TreeNewHandle);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);
}

NTSTATUS PvpPeExportsEnumerateThread(
    _In_ PPV_EXPORT_CONTEXT Context
    )
{
    PH_MAPPED_IMAGE_EXPORTS exports;
    PH_MAPPED_IMAGE_EXPORT_ENTRY exportEntry;
    PH_MAPPED_IMAGE_EXPORT_FUNCTION exportFunction;
    ULONG i;

    if (NT_SUCCESS(PhGetMappedImageExports(&exports, &PvMappedImage)))
    {
        for (i = 0; i < exports.NumberOfEntries; i++)
        {
            if (
                NT_SUCCESS(PhGetMappedImageExportEntry(&exports, i, &exportEntry)) &&
                NT_SUCCESS(PhGetMappedImageExportFunction(&exports, NULL, exportEntry.Ordinal, &exportFunction))
                )
            {
                PPV_EXPORT_NODE exportNode;
                WCHAR value[PH_INT64_STR_LEN_1];

                exportNode = PhAllocateZero(sizeof(PV_EXPORT_NODE));
                exportNode->UniqueId = i + 1;
                exportNode->Address = (ULONG_PTR)exportFunction.Function;
                exportNode->Ordinal = exportEntry.Ordinal;
                exportNode->ExportForwarded = !!exportFunction.ForwardedName;
                exportNode->UniqueIdString = PhFormatUInt64(exportNode->UniqueId, FALSE);
                exportNode->OrdinalString = PhFormatUInt64(exportEntry.Ordinal, FALSE);

                if (exportEntry.Name) // Note: The 'Hint' is only valid for named exports. (dmex)
                {
                    exportNode->Hint = exportEntry.Hint;
                    exportNode->HintString = PhFormatUInt64(exportEntry.Hint, FALSE);
                }

                if (exportFunction.ForwardedName)
                {
                    PPH_STRING forwardName;

                    forwardName = PhConvertUtf8ToUtf16(exportFunction.ForwardedName);

                    if (forwardName->Buffer[0] == L'?')
                    {
                        PPH_STRING undecoratedName;
    
                        if (undecoratedName = PhUndecorateSymbolName(PvSymbolProvider, forwardName->Buffer))
                            PhMoveReference(&forwardName, undecoratedName);
                    }

                    exportNode->AddressString = forwardName;
                }
                else
                {
                    if (exportFunction.Function)
                    {
                        PhPrintPointer(value, exportFunction.Function);
                        exportNode->AddressString = PhCreateString(value);
                    }
                }

                if (exportEntry.Name)
                {
                    PPH_STRING exportName;

                    exportName = PhConvertUtf8ToUtf16(exportEntry.Name);

                    if (exportName->Buffer[0] == L'?')
                    {
                        PPH_STRING undecoratedName;
    
                        if (undecoratedName = PhUndecorateSymbolName(PvSymbolProvider, exportName->Buffer))
                            PhMoveReference(&exportName, undecoratedName);
                    }

                    exportNode->NameString = exportName;
                }
                else
                {
                    if (exportFunction.Function)
                    {
                        PPH_STRING exportSymbol = NULL;
                        PPH_STRING exportSymbolName = NULL;
    
                        // Try find the export name using symbols.
                        if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
                        {
                            exportSymbol = PhGetSymbolFromAddress(
                                PvSymbolProvider,
                                (ULONG64)PTR_ADD_OFFSET(PvMappedImage.NtHeaders32->OptionalHeader.ImageBase, exportFunction.Function),
                                NULL,
                                NULL,
                                &exportSymbolName,
                                NULL
                                );
                        }
                        else
                        {
                            exportSymbol = PhGetSymbolFromAddress(
                                PvSymbolProvider,
                                (ULONG64)PTR_ADD_OFFSET(PvMappedImage.NtHeaders->OptionalHeader.ImageBase, exportFunction.Function),
                                NULL,
                                NULL,
                                &exportSymbolName,
                                NULL
                                );
                        }

                        if (!PhIsNullOrEmptyString(exportSymbolName))
                        {
                            exportNode->NameString = PhConcatStringRefZ(&exportSymbolName->sr, L" (unnamed)");
                        }

                        PhClearReference(&exportSymbolName);
                        PhClearReference(&exportSymbol);
                    }
                }

                PhAcquireQueuedLockExclusive(&Context->SearchResultsLock);
                PhAddItemList(Context->SearchResults, exportNode);
                PhReleaseQueuedLockExclusive(&Context->SearchResultsLock);
            }
        }
    }

    PostMessage(Context->WindowHandle, WM_PV_SEARCH_FINISHED, 0, 0);
    return STATUS_SUCCESS;
}

INT_PTR CALLBACK PvPeExportsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_EXPORT_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PV_EXPORT_CONTEXT));
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
            context->SearchResults = PhCreateList(1);

            PvCreateSearchControl(context->SearchHandle, L"Search Exports (Ctrl+K)");

            PvInitializeExportTree(context, hwndDlg, context->TreeNewHandle);
            PhAddTreeNewFilter(&context->FilterSupport, PvExportTreeFilterCallback, context);
            PhLoadSettingsExportList(context);
            PvConfigTreeBorders(context->TreeNewHandle);

            TreeNew_SetEmptyText(context->TreeNewHandle, &LoadingExportsText, 0);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->SearchHandle, NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);

            PhCreateThread2(PvpPeExportsEnumerateThread, context);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveSettingsExportList(context);
            PvDeleteExportTree(context);
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
                            //PhExpandAllNodes(TRUE);
                            //PhDeselectAllNodes();
                        }

                        PhApplyTreeNewFilters(&context->FilterSupport);
                    }
                }
                break;
            }
        }
        break;
    case WM_PV_SEARCH_FINISHED:
        {
            PvAddPendingExportNodes(context);

            TreeNew_SetEmptyText(context->TreeNewHandle, &EmptyExportsText, 0);

            TreeNew_NodesStructured(context->TreeNewHandle);
        }
        break;
    case WM_PV_SEARCH_SHOWMENU:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
            PPH_EMENU menu;
            PPH_EMENU_ITEM selectedItem;
            PPV_EXPORT_NODE* exportNodes = NULL;
            ULONG numberOfNodes = 0;

            if (!PvGetSelectedExportNodes(context, &exportNodes, &numberOfNodes))
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
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetTextColor((HDC)wParam, RGB(0, 0, 0));
            SetDCBrushColor((HDC)wParam, RGB(255, 255, 255));
            return (INT_PTR)GetStockBrush(DC_BRUSH);
        }
        break;
    }

    return FALSE;
}

VOID PhLoadSettingsExportList(
    _Inout_ PPV_EXPORT_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhGetStringSetting(L"ImageExportsTreeListColumns");
    sortSettings = PhGetStringSetting(L"ImageExportsTreeListSort");
    //Context->Flags = PhGetIntegerSetting(L"ImageExportsTreeListFlags");

    PhCmLoadSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &settings->sr, &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PhSaveSettingsExportList(
    _Inout_ PPV_EXPORT_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhCmSaveSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &sortSettings);

    //PhSetIntegerSetting(L"ImageExportsTreeListFlags", Context->Flags);
    PhSetStringSetting2(L"ImageExportsTreeListColumns", &settings->sr);
    PhSetStringSetting2(L"ImageExportsTreeListSort", &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PvDeleteExportTree(
    _In_ PPV_EXPORT_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PvDestroyExportNode(Context->NodeList->Items[i]);
    }

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}

struct _PH_TN_FILTER_SUPPORT* GetExportListFilterSupport(
    _In_ PPV_EXPORT_CONTEXT Context
    )
{
    return &Context->FilterSupport;
}

LONG PvExportTreeNewPostSortFunction(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    )
{
    if (Result == 0)
        Result = uintptrcmp((ULONG_PTR)((PPV_EXPORT_NODE)Node1)->UniqueId, (ULONG_PTR)((PPV_EXPORT_NODE)Node2)->UniqueId);

    return PhModifySort(Result, SortOrder);
}

BOOLEAN PvExportNodeHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPV_EXPORT_NODE exportNode1 = *(PPV_EXPORT_NODE*)Entry1;
    PPV_EXPORT_NODE exportNode2 = *(PPV_EXPORT_NODE*)Entry2;

    return exportNode1->UniqueId == exportNode2->UniqueId;
}

ULONG PvExportNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashInt64((ULONG_PTR)(*(PPV_EXPORT_NODE*)Entry)->UniqueId);
}

VOID PvExportAddTreeNode(
    _In_ PPV_EXPORT_CONTEXT Context,
    _In_ PPV_EXPORT_NODE Entry
    )
{
    PhInitializeTreeNewNode(&Entry->Node);

    memset(Entry->TextCache, 0, sizeof(PH_STRINGREF) * PV_EXPORT_TREE_COLUMN_ITEM_MAXIMUM);
    Entry->Node.TextCache = Entry->TextCache;
    Entry->Node.TextCacheSize = PV_EXPORT_TREE_COLUMN_ITEM_MAXIMUM;

    if (PhAddEntryHashtable(Context->NodeHashtable, &Entry)) // HACK
    {
        PhAddItemList(Context->NodeList, Entry);

        if (Context->FilterSupport.NodeList)
        {
            Entry->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->FilterSupport, &Entry->Node);
        }
    }
}

PPV_EXPORT_NODE PvFindExportNode(
    _In_ PPV_EXPORT_CONTEXT Context,
    _In_ PPH_STRING Name
    )
{
    PV_EXPORT_NODE lookupSymbolNode;
    PPV_EXPORT_NODE lookupSymbolNodePtr = &lookupSymbolNode;
    PPV_EXPORT_NODE *threadNode;

    lookupSymbolNode.NameString = Name;

    threadNode = (PPV_EXPORT_NODE*)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupSymbolNodePtr
        );

    if (threadNode)
        return *threadNode;
    else
        return NULL;
}

VOID PvRemoveExportNode(
    _In_ PPV_EXPORT_CONTEXT Context,
    _In_ PPV_EXPORT_NODE Node
    )
{
    ULONG index;

    PhRemoveEntryHashtable(Context->NodeHashtable, &Node);

    if ((index = PhFindItemList(Context->NodeList, Node)) != ULONG_MAX)
        PhRemoveItemList(Context->NodeList, index);

    PvDestroyExportNode(Node);
}

VOID PvDestroyExportNode(
    _In_ PPV_EXPORT_NODE Node
    )
{
    PhFree(Node);
}

#define SORT_FUNCTION(Column) PvExportTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PvExportTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPV_EXPORT_NODE node1 = *(PPV_EXPORT_NODE *)_elem1; \
    PPV_EXPORT_NODE node2 = *(PPV_EXPORT_NODE *)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)node1->UniqueId, (ULONG_PTR)node2->UniqueId); \
    \
    return PhModifySort(sortResult, ((PPV_EXPORT_CONTEXT)_context)->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Index)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->UniqueId, (ULONG_PTR)node2->UniqueId);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(RVA)
{
    sortResult = uintptrcmp(node1->Address, node2->Address);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareStringWithNull(node1->NameString, node2->NameString, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Ordinal)
{
    sortResult = uintcmp(node1->Ordinal, node2->Ordinal);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Hint)
{
    sortResult = uintcmp(node1->Hint, node2->Hint);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PvExportTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PPV_EXPORT_CONTEXT context = Context;
    PPV_EXPORT_NODE node;

    if (!context)
        return FALSE;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            if (!getChildren)
                break;

            node = (PPV_EXPORT_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Index),
                    SORT_FUNCTION(RVA),
                    SORT_FUNCTION(Name),
                    SORT_FUNCTION(Ordinal),
                    SORT_FUNCTION(Hint),
                };
                int (__cdecl *sortFunction)(void *, const void *, const void *);

                if (context->TreeNewSortColumn < PV_EXPORT_TREE_COLUMN_ITEM_MAXIMUM)
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

            node = (PPV_EXPORT_NODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;

            if (!getCellText)
                break;

            node = (PPV_EXPORT_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PV_EXPORT_TREE_COLUMN_ITEM_INDEX:
                getCellText->Text = PhGetStringRef(node->UniqueIdString);
                break;
            case PV_EXPORT_TREE_COLUMN_ITEM_RVA:
                getCellText->Text = PhGetStringRef(node->AddressString);
                break;
            case PV_EXPORT_TREE_COLUMN_ITEM_NAME:
                getCellText->Text = PhGetStringRef(node->NameString);
                break;
            case PV_EXPORT_TREE_COLUMN_ITEM_ORDINAL:
                getCellText->Text = PhGetStringRef(node->OrdinalString);
                break;
            case PV_EXPORT_TREE_COLUMN_ITEM_HINT:
                getCellText->Text = PhGetStringRef(node->HintString);
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

            node = (PPV_EXPORT_NODE)getNodeColor->Node;

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

VOID PvExportClearTree(
    _In_ PPV_EXPORT_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        PvDestroyExportNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);
}

PPV_EXPORT_NODE PvGetSelectedExportNode(
    _In_ PPV_EXPORT_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPV_EXPORT_NODE exportNode = Context->NodeList->Items[i];

        if (exportNode->Node.Selected)
            return exportNode;
    }

    return NULL;
}

_Success_(return)
BOOLEAN PvGetSelectedExportNodes(
    _In_ PPV_EXPORT_CONTEXT Context,
    _Out_ PPV_EXPORT_NODE **Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPV_EXPORT_NODE node = (PPV_EXPORT_NODE)Context->NodeList->Items[i];

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

VOID PvInitializeExportTree(
    _In_ PPV_EXPORT_CONTEXT Context,
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle
    )
{
    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PPV_EXPORT_NODE),
        PvExportNodeHashtableCompareFunction,
        PvExportNodeHashtableHashFunction,
        100
        );
    Context->NodeList = PhCreateList(100);

    Context->ParentWindowHandle = ParentWindowHandle;
    Context->TreeNewHandle = TreeNewHandle;
    PhSetControlTheme(TreeNewHandle, L"explorer");

    TreeNew_SetCallback(TreeNewHandle, PvExportTreeNewCallback, Context);
    TreeNew_SetRedraw(TreeNewHandle, FALSE);

    PhAddTreeNewColumnEx2(TreeNewHandle, PV_EXPORT_TREE_COLUMN_ITEM_INDEX, TRUE, L"#", 40, PH_ALIGN_LEFT, PV_EXPORT_TREE_COLUMN_ITEM_INDEX, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_EXPORT_TREE_COLUMN_ITEM_RVA, TRUE, L"RVA", 80, PH_ALIGN_LEFT, PV_EXPORT_TREE_COLUMN_ITEM_RVA, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_EXPORT_TREE_COLUMN_ITEM_NAME, TRUE, L"Name", 250, PH_ALIGN_LEFT, PV_EXPORT_TREE_COLUMN_ITEM_NAME, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_EXPORT_TREE_COLUMN_ITEM_ORDINAL, TRUE, L"Ordinal", 50, PH_ALIGN_LEFT, PV_EXPORT_TREE_COLUMN_ITEM_ORDINAL, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_EXPORT_TREE_COLUMN_ITEM_HINT, TRUE, L"Hint", 50, PH_ALIGN_LEFT, PV_EXPORT_TREE_COLUMN_ITEM_HINT, 0, 0);

    TreeNew_SetRedraw(TreeNewHandle, TRUE);
    TreeNew_SetSort(TreeNewHandle, PV_EXPORT_TREE_COLUMN_ITEM_INDEX, AscendingSortOrder);

    PhCmInitializeManager(&Context->Cm, TreeNewHandle, PV_EXPORT_TREE_COLUMN_ITEM_MAXIMUM, PvExportTreeNewPostSortFunction);

    PhInitializeTreeNewFilterSupport(&Context->FilterSupport, TreeNewHandle, Context->NodeList);
}

BOOLEAN PvExportWordMatchStringRef(
    _In_ PPV_EXPORT_CONTEXT Context,
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

BOOLEAN PvExportWordMatchStringZ(
    _In_ PPV_EXPORT_CONTEXT Context,
    _In_ PWSTR Text
    )
{
    PH_STRINGREF text;

    PhInitializeStringRef(&text, Text);
    return PvExportWordMatchStringRef(Context, &text);
}

BOOLEAN PvExportTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPV_EXPORT_CONTEXT context = Context;
    PPV_EXPORT_NODE node = (PPV_EXPORT_NODE)Node;

    if (PhIsNullOrEmptyString(context->SearchboxText))
        return TRUE;

    if (!PhIsNullOrEmptyString(node->AddressString))
    {
        if (PvExportWordMatchStringRef(context, &node->AddressString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->NameString))
    {
        if (PvExportWordMatchStringRef(context, &node->NameString->sr))
            return TRUE;
    }
    else
    {
        static PH_STRINGREF exportNameSr = PH_STRINGREF_INIT(L"(unnamed)");

        if (PvExportWordMatchStringRef(context, &exportNameSr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->OrdinalString))
    {
        if (PvExportWordMatchStringRef(context, &node->OrdinalString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->HintString))
    {
        if (PvExportWordMatchStringRef(context, &node->HintString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->UniqueIdString))
    {
        if (PvExportWordMatchStringRef(context, &node->UniqueIdString->sr))
            return TRUE;
    }

    return FALSE;
}
