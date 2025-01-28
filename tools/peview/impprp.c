/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2017-2023
 *
 */

#include <peview.h>
#include "colmgr.h"

static PH_STRINGREF EmptyImportsText = PH_STRINGREF_INIT(L"There are no imports to display.");
static PH_STRINGREF LoadingImportsText = PH_STRINGREF_INIT(L"Loading imports from image...");

typedef enum _PV_IMPORT_TREE_COLUMN_ITEM
{
    PV_IMPORT_TREE_COLUMN_ITEM_INDEX,
    PV_IMPORT_TREE_COLUMN_ITEM_RVA,
    PV_IMPORT_TREE_COLUMN_ITEM_DLL,
    PV_IMPORT_TREE_COLUMN_ITEM_NAME,
    PV_IMPORT_TREE_COLUMN_ITEM_HINT,
    PV_IMPORT_TREE_COLUMN_ITEM_SYMBOL,
    PV_IMPORT_TREE_COLUMN_ITEM_ORDINAL,
    PV_IMPORT_TREE_COLUMN_ITEM_ORDINALNAME,
    //PV_IMPORT_TREE_COLUMN_ITEM_FIRSTTHUNK,
    //PV_IMPORT_TREE_COLUMN_ITEM_ORIGINALTHUNK,
    //PV_IMPORT_TREE_COLUMN_ITEM_NAMETHUNK,
    //PV_IMPORT_TREE_COLUMN_ITEM_ADDRESSTHUNK,
    PV_IMPORT_TREE_COLUMN_ITEM_MAXIMUM
} PV_IMPORT_TREE_COLUMN_ITEM;

typedef struct _PV_IMPORT_NODE
{
    PH_TREENEW_NODE Node;

    ULONG64 UniqueId;
    ULONG_PTR Address;
    ULONG Hint;
    PPH_STRING UniqueIdString;
    PPH_STRING AddressString;
    PPH_STRING DllString;
    PPH_STRING NameString;
    PPH_STRING HintString;
    PPH_STRING SymbolString;
    PPH_STRING OrdinalString;
    PPH_STRING OrdinalNameString;

    PH_STRINGREF TextCache[PV_IMPORT_TREE_COLUMN_ITEM_MAXIMUM];
} PV_IMPORT_NODE, *PPV_IMPORT_NODE;

typedef struct _PV_IMPORT_CONTEXT
{
    HWND DialogHandle;
    HWND SearchHandle;
    HWND TreeNewHandle;
    HWND ParentWindowHandle;

    ULONG_PTR SearchMatchHandle;
    PPH_STRING TreeText;

    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;

    ULONG SearchResultsAddIndex;
    PPH_LIST SearchResults;
    PH_QUEUED_LOCK SearchResultsLock;

    PH_CM_MANAGER Cm;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PH_TN_FILTER_SUPPORT FilterSupport;
    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;
} PV_IMPORT_CONTEXT, *PPV_IMPORT_CONTEXT;

BOOLEAN PvImportNodeHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

ULONG PvImportNodeHashtableHashFunction(
    _In_ PVOID Entry
    );

VOID PvDestroyImportNode(
    _In_ PPV_IMPORT_NODE Node
    );

VOID PvInitializeImportTree(
    _In_ PPV_IMPORT_CONTEXT Context,
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle
    );

VOID PvDeleteImportTree(
    _In_ PPV_IMPORT_CONTEXT Context
    );

VOID PvImportAddTreeNode(
    _In_ PPV_IMPORT_CONTEXT Context,
    _In_ PPV_IMPORT_NODE Entry
    );

BOOLEAN PvImportTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );

VOID PhLoadSettingsImportList(
    _Inout_ PPV_IMPORT_CONTEXT Context
    );

VOID PhSaveSettingsImportList(
    _Inout_ PPV_IMPORT_CONTEXT Context
    );

_Success_(return)
BOOLEAN PvGetSelectedImportNodes(
    _In_ PPV_IMPORT_CONTEXT Context,
    _Out_ PPV_IMPORT_NODE **Nodes,
    _Out_ PULONG NumberOfNodes
    );

VOID PvAddPendingImportNodes(
    _In_ PPV_IMPORT_CONTEXT Context
    )
{
    ULONG i;
    BOOLEAN needsFullUpdate = FALSE;

    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    PhAcquireQueuedLockExclusive(&Context->SearchResultsLock);

    for (i = Context->SearchResultsAddIndex; i < Context->SearchResults->Count; i++)
    {
        PvImportAddTreeNode(Context, Context->SearchResults->Items[i]);
        needsFullUpdate = TRUE;
    }
    Context->SearchResultsAddIndex = i;

    PhReleaseQueuedLockExclusive(&Context->SearchResultsLock);

    if (needsFullUpdate)
        TreeNew_NodesStructured(Context->TreeNewHandle);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);
}

PPH_STRING PvpQueryModuleOrdinalName(
    _In_ PPH_STRING FileName,
    _In_ USHORT Ordinal
    )
{
    PPH_STRING exportName = NULL;
    PH_MAPPED_IMAGE mappedImage;

    if (NT_SUCCESS(PhLoadMappedImage(FileName->Buffer, NULL, &mappedImage)))
    {
        PH_MAPPED_IMAGE_EXPORTS exports;
        PH_MAPPED_IMAGE_EXPORT_ENTRY exportEntry;
        PH_MAPPED_IMAGE_EXPORT_FUNCTION exportFunction;
        ULONG i;

        if (NT_SUCCESS(PhGetMappedImageExports(&exports, &mappedImage)))
        {
            for (i = 0; i < exports.NumberOfEntries; i++)
            {
                if (
                    NT_SUCCESS(PhGetMappedImageExportEntry(&exports, i, &exportEntry)) &&
                    NT_SUCCESS(PhGetMappedImageExportFunction(&exports, NULL, exportEntry.Ordinal, &exportFunction))
                    )
                {
                    if (exportEntry.Ordinal == Ordinal)
                    {
                        if (exportEntry.Name)
                        {
                            exportName = PhConvertUtf8ToUtf16(exportEntry.Name);

                            if (exportName->Buffer[0] == L'?')
                            {
                                PPH_STRING undecoratedName;

                                if (undecoratedName = PhUndecorateSymbolName(PvSymbolProvider, exportName->Buffer))
                                    PhMoveReference(&exportName, undecoratedName);
                            }
                        }
                        else
                        {
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

                                PhMoveReference(&exportName, PhFormatString(L"%s (Forwarded)", forwardName->Buffer));
                                PhDereferenceObject(forwardName);
                            }
                            else if (exportFunction.Function)
                            {
                                PPH_STRING exportSymbol = NULL;
                                PPH_STRING exportSymbolName = NULL;
                                PPH_STRING exportFileName;

                                if (NT_SUCCESS(PhGetProcessMappedFileName(NtCurrentProcess(), (PVOID)mappedImage.ViewBase, &exportFileName)))
                                {
                                    PPH_SYMBOL_PROVIDER moduleSymbolProvider = NULL;

                                    if (PvpLoadDbgHelp(&moduleSymbolProvider))
                                    {
                                        if (PhLoadModuleSymbolProvider(
                                            moduleSymbolProvider,
                                            exportFileName,
                                            mappedImage.ViewBase,
                                            (ULONG)mappedImage.ViewSize
                                            ))
                                        {
                                            // Try find the export name using symbols.
                                            exportSymbol = PhGetSymbolFromAddress(
                                                moduleSymbolProvider,
                                                PTR_ADD_OFFSET(mappedImage.ViewBase, exportFunction.Function),
                                                NULL,
                                                NULL,
                                                &exportSymbolName,
                                                NULL
                                                );
                                        }

                                        PhDereferenceObject(moduleSymbolProvider);
                                    }

                                    PhDereferenceObject(exportFileName);
                                }

                                if (exportSymbolName)
                                {
                                    PhSetReference(&exportName, exportSymbolName);
                                    PhDereferenceObject(exportSymbolName);
                                }

                                if (exportSymbol)
                                    PhDereferenceObject(exportSymbol);
                            }
                        }

                        break;
                    }
                }
            }
        }

        PhUnloadMappedImage(&mappedImage);
    }

    return exportName;
}

VOID PvpProcessImports(
    _In_ PPV_IMPORT_CONTEXT Context,
    _In_ PPH_MAPPED_IMAGE_IMPORTS Imports,
    _In_ BOOLEAN DelayImports,
    _Inout_ ULONG *Count
    )
{
    PH_MAPPED_IMAGE_IMPORT_DLL importDll;
    PH_MAPPED_IMAGE_IMPORT_ENTRY importEntry;
    ULONG i;
    ULONG j;

    for (i = 0; i < Imports->NumberOfDlls; i++)
    {
        if (NT_SUCCESS(PhGetMappedImageImportDll(Imports, i, &importDll)))
        {
            for (j = 0; j < importDll.NumberOfEntries; j++)
            {
                if (NT_SUCCESS(PhGetMappedImageImportEntry(&importDll, j, &importEntry)))
                {
                    PPV_IMPORT_NODE importNode;
                    PPH_STRING importDllName;

                    importNode = PhAllocateZero(sizeof(PV_IMPORT_NODE));
                    importNode->UniqueId = ++(*Count);
                    importNode->UniqueIdString = PhFormatUInt64(importNode->UniqueId, FALSE);

                    if (importEntry.Name)
                    {
                        importNode->Hint = importEntry.NameHint;
                        importNode->HintString = PhFormatUInt64(importEntry.NameHint, FALSE);
                    }

                    if (importNode->DllString = PhConvertUtf8ToUtf16(importDll.Name))
                    {
                        if (importDllName = PhApiSetResolveToHost(&importNode->DllString->sr))
                        {
                            PhMoveReference(&importNode->DllString, PhFormatString(
                                L"%s (%s)",
                                PhGetString(importNode->DllString),
                                PhGetString(importDllName))
                                );
                            PhDereferenceObject(importDllName);
                        }
                    }

                    if (DelayImports)
                    {
                        PhMoveReference(&importNode->DllString, PhFormatString(
                            L"%s (Delay)",
                            PhGetString(importNode->DllString))
                            );
                    }

                    if (importEntry.Name)
                    {
                        if (importNode->NameString = PhConvertUtf8ToUtf16(importEntry.Name))
                        {
                            if (importNode->NameString->Buffer[0] == L'?')
                            {
                                importNode->SymbolString = PhUndecorateSymbolName(PvSymbolProvider, PhGetString(importNode->NameString));
                            }
                        }
                    }
                    else
                    {
                        importNode->OrdinalString = PhFormatUInt64(importEntry.Ordinal, TRUE);

                        if (importDllName = PhConvertUtf8ToUtf16(importDll.Name))
                        {
                            PPH_STRING apisetFileName;
                            PPH_STRING importFileName;

                            if (apisetFileName = PhApiSetResolveToHost(&importDllName->sr))
                            {
                                PhMoveReference(&importDllName, apisetFileName);
                            }

                            // TODO: Add DLL directory to PhSearchFilePath for locating non-system images. (dmex)

                            if (importFileName = PhSearchFilePath(importDllName->Buffer, L".dll"))
                            {
                                PhMoveReference(&importDllName, importFileName);
                            }

                            importNode->OrdinalNameString = PvpQueryModuleOrdinalName(importDllName, importEntry.Ordinal);
                            PhDereferenceObject(importDllName);
                        }
                    }

                    {
                        WCHAR value[PH_INT64_STR_LEN_1];

                        importNode->Address = PhGetMappedImageImportEntryRva(&importDll, j, DelayImports);
                        PhPrintPointer(value, (PVOID)importNode->Address);
                        importNode->AddressString = PhCreateString(value);
                    }

                    PhAcquireQueuedLockExclusive(&Context->SearchResultsLock);
                    PhAddItemList(Context->SearchResults, importNode);
                    PhReleaseQueuedLockExclusive(&Context->SearchResultsLock);
                }
            }
        }
    }
}

NTSTATUS PvpPeImportsEnumerateThread(
    _In_ PPV_IMPORT_CONTEXT Context
    )
{
    ULONG count = 0;
    PH_MAPPED_IMAGE_IMPORTS imports;

    if (NT_SUCCESS(PhGetMappedImageImports(&imports, &PvMappedImage)))
    {
        PvpProcessImports(Context, &imports, FALSE, &count);
    }

    if (NT_SUCCESS(PhGetMappedImageDelayImports(&imports, &PvMappedImage)))
    {
        PvpProcessImports(Context, &imports, TRUE, &count);
    }

    PostMessage(Context->DialogHandle, WM_PV_SEARCH_FINISHED, 0, 0);
    return STATUS_SUCCESS;
}

VOID NTAPI PvpPeImportsSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
)
{
    PPV_IMPORT_CONTEXT context = Context;

    assert(context);

    context->SearchMatchHandle = MatchHandle;

    if (!context->SearchMatchHandle)
    {
        //PhExpandAllNodes(TRUE);
        //PhDeselectAllNodes();
    }

    PhApplyTreeNewFilters(&context->FilterSupport);
}

INT_PTR CALLBACK PvPeImportsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_IMPORT_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PV_IMPORT_CONTEXT));
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
            context->DialogHandle = hwndDlg;
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_TREELIST);
            context->SearchHandle = GetDlgItem(hwndDlg, IDC_TREESEARCH);
            context->SearchResults = PhCreateList(1);

            PvCreateSearchControl(
                hwndDlg,
                context->SearchHandle,
                L"Search Imports (Ctrl+K)",
                PvpPeImportsSearchControlCallback,
                context
                );

            PvInitializeImportTree(context, hwndDlg, context->TreeNewHandle);
            PhAddTreeNewFilter(&context->FilterSupport, PvImportTreeFilterCallback, context);
            PhLoadSettingsImportList(context);
            PvConfigTreeBorders(context->TreeNewHandle);

            TreeNew_SetEmptyText(context->TreeNewHandle, &LoadingImportsText, 0);
            TreeNew_SetRowHeight(context->TreeNewHandle, PhGetDpi(22, PhGetWindowDpi(hwndDlg)));

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->SearchHandle, NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);

            PhCreateThread2(PvpPeImportsEnumerateThread, context);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveSettingsImportList(context);
            PvDeleteImportTree(context);
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
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
            PvAddPendingImportNodes(context);

            TreeNew_SetEmptyText(context->TreeNewHandle, &EmptyImportsText, 0);

            TreeNew_NodesStructured(context->TreeNewHandle);
        }
        break;
    case WM_PV_SEARCH_SHOWMENU:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
            PPH_EMENU menu;
            PPH_EMENU_ITEM selectedItem;
            PPV_IMPORT_NODE* importNodes = NULL;
            ULONG numberOfNodes = 0;

            if (!PvGetSelectedImportNodes(context, &importNodes, &numberOfNodes))
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
            return (INT_PTR)PhGetStockBrush(DC_BRUSH);
        }
        break;
    case WM_KEYDOWN:
        {
            if (LOWORD(wParam) == 'K' && GetKeyState(VK_CONTROL) < 0)
            {
                SetFocus(context->SearchHandle);
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}

VOID PhLoadSettingsImportList(
    _Inout_ PPV_IMPORT_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhGetStringSetting(L"ImageImportTreeListColumns");
    sortSettings = PhGetStringSetting(L"ImageImportTreeListSort");
    //Context->Flags = PhGetIntegerSetting(L"ImageImportTreeListFlags");

    PhCmLoadSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &settings->sr, &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PhSaveSettingsImportList(
    _Inout_ PPV_IMPORT_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhCmSaveSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &sortSettings);

    //PhSetIntegerSetting(L"ImageImportsTreeListFlags", Context->Flags);
    PhSetStringSetting2(L"ImageImportTreeListColumns", &settings->sr);
    PhSetStringSetting2(L"ImageImportTreeListSort", &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PvDeleteImportTree(
    _In_ PPV_IMPORT_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PvDestroyImportNode(Context->NodeList->Items[i]);
    }

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}

struct _PH_TN_FILTER_SUPPORT* GetImportListFilterSupport(
    _In_ PPV_IMPORT_CONTEXT Context
    )
{
    return &Context->FilterSupport;
}

LONG PvImportTreeNewPostSortFunction(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    )
{
    if (Result == 0)
        Result = uintptrcmp((ULONG_PTR)((PPV_IMPORT_NODE)Node1)->UniqueId, (ULONG_PTR)((PPV_IMPORT_NODE)Node2)->UniqueId);

    return PhModifySort(Result, SortOrder);
}

BOOLEAN PvImportNodeHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPV_IMPORT_NODE importNode1 = *(PPV_IMPORT_NODE*)Entry1;
    PPV_IMPORT_NODE importNode2 = *(PPV_IMPORT_NODE*)Entry2;

    return importNode1->UniqueId == importNode2->UniqueId;
}

ULONG PvImportNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashInt64((ULONG_PTR)(*(PPV_IMPORT_NODE*)Entry)->UniqueId);
}

VOID PvImportAddTreeNode(
    _In_ PPV_IMPORT_CONTEXT Context,
    _In_ PPV_IMPORT_NODE Entry
    )
{
    PhInitializeTreeNewNode(&Entry->Node);

    memset(Entry->TextCache, 0, sizeof(PH_STRINGREF) * PV_IMPORT_TREE_COLUMN_ITEM_MAXIMUM);
    Entry->Node.TextCache = Entry->TextCache;
    Entry->Node.TextCacheSize = PV_IMPORT_TREE_COLUMN_ITEM_MAXIMUM;

    if (PhAddEntryHashtable(Context->NodeHashtable, &Entry)) // HACK
    {
        PhAddItemList(Context->NodeList, Entry);

        if (Context->FilterSupport.NodeList)
        {
            Entry->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->FilterSupport, &Entry->Node);
        }
    }
}

PPV_IMPORT_NODE PvFindImportNode(
    _In_ PPV_IMPORT_CONTEXT Context,
    _In_ PPH_STRING Name
    )
{
    PV_IMPORT_NODE lookupSymbolNode;
    PPV_IMPORT_NODE lookupSymbolNodePtr = &lookupSymbolNode;
    PPV_IMPORT_NODE*threadNode;

    lookupSymbolNode.NameString = Name;

    threadNode = (PPV_IMPORT_NODE*)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupSymbolNodePtr
        );

    if (threadNode)
        return *threadNode;
    else
        return NULL;
}

VOID PvRemoveImportNode(
    _In_ PPV_IMPORT_CONTEXT Context,
    _In_ PPV_IMPORT_NODE Node
    )
{
    ULONG index;

    PhRemoveEntryHashtable(Context->NodeHashtable, &Node);

    if ((index = PhFindItemList(Context->NodeList, Node)) != ULONG_MAX)
        PhRemoveItemList(Context->NodeList, index);

    PvDestroyImportNode(Node);
}

VOID PvDestroyImportNode(
    _In_ PPV_IMPORT_NODE Node
    )
{
    PhFree(Node);
}

#define SORT_FUNCTION(Column) PvImportTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PvImportTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPV_IMPORT_NODE node1 = *(PPV_IMPORT_NODE *)_elem1; \
    PPV_IMPORT_NODE node2 = *(PPV_IMPORT_NODE *)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)node1->UniqueId, (ULONG_PTR)node2->UniqueId); \
    \
    return PhModifySort(sortResult, ((PPV_IMPORT_CONTEXT)_context)->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Index)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->UniqueId, (ULONG_PTR)node2->UniqueId);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Rva)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->Address, (ULONG_PTR)node2->Address);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Dll)
{
    sortResult = PhCompareStringWithNull(node1->DllString, node2->DllString, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareStringWithNull(node1->NameString, node2->NameString, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Hint)
{
    sortResult = uintcmp(node1->Hint, node2->Hint);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Symbol)
{
    sortResult = PhCompareStringWithNull(node1->SymbolString, node2->SymbolString, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Ordinal)
{
    sortResult = PhCompareStringWithNull(node1->OrdinalString, node2->OrdinalString, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(OrdinalName)
{
    sortResult = PhCompareStringWithNull(node1->OrdinalNameString, node2->OrdinalNameString, FALSE);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PvImportTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PPV_IMPORT_CONTEXT context = Context;
    PPV_IMPORT_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PPV_IMPORT_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Index),
                    SORT_FUNCTION(Rva),
                    SORT_FUNCTION(Dll),
                    SORT_FUNCTION(Name),
                    SORT_FUNCTION(Hint),
                    SORT_FUNCTION(Symbol),
                    SORT_FUNCTION(Ordinal),
                    SORT_FUNCTION(OrdinalName),
                };
                int (__cdecl *sortFunction)(void *, const void *, const void *);

                static_assert(RTL_NUMBER_OF(sortFunctions) == PV_IMPORT_TREE_COLUMN_ITEM_MAXIMUM, "SortFunctions must equal maximum.");

                if (context->TreeNewSortColumn < PV_IMPORT_TREE_COLUMN_ITEM_MAXIMUM)
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
            node = (PPV_IMPORT_NODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;
            node = (PPV_IMPORT_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PV_IMPORT_TREE_COLUMN_ITEM_INDEX:
                getCellText->Text = PhGetStringRef(node->UniqueIdString);
                break;
            case PV_IMPORT_TREE_COLUMN_ITEM_RVA:
                getCellText->Text = PhGetStringRef(node->AddressString);
                break;
            case PV_IMPORT_TREE_COLUMN_ITEM_DLL:
                getCellText->Text = PhGetStringRef(node->DllString);
                break;
            case PV_IMPORT_TREE_COLUMN_ITEM_NAME:
                getCellText->Text = PhGetStringRef(node->NameString);
                break;
            case PV_IMPORT_TREE_COLUMN_ITEM_HINT:
                getCellText->Text = PhGetStringRef(node->HintString);
                break;
            case PV_IMPORT_TREE_COLUMN_ITEM_ORDINAL:
                getCellText->Text = PhGetStringRef(node->OrdinalString);
                break;
            case PV_IMPORT_TREE_COLUMN_ITEM_ORDINALNAME:
                getCellText->Text = PhGetStringRef(node->OrdinalNameString);
                break;
            case PV_IMPORT_TREE_COLUMN_ITEM_SYMBOL:
                getCellText->Text = PhGetStringRef(node->SymbolString);
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
            node = (PPV_IMPORT_NODE)getNodeColor->Node;

            getNodeColor->Flags = TN_CACHE | TN_AUTO_FORECOLOR;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            PPH_TREENEW_SORT_CHANGED_EVENT sorting = Parameter1;

            context->TreeNewSortColumn = sorting->SortColumn;
            context->TreeNewSortOrder = sorting->SortOrder;

            TreeNew_NodesStructured(hwnd);
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                {
                    if (GetKeyState(VK_CONTROL) < 0)
                    {
                        PPH_STRING text;

                        text = PhGetTreeNewText(hwnd, 0);
                        PhSetClipboardString(hwnd, &text->sr);
                        PhDereferenceObject(text);
                    }
                }
                break;
            }
        }
        return TRUE;
    case TreeNewNodeExpanding:
    case TreeNewLeftDoubleClick:
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

VOID PvImportClearTree(
    _In_ PPV_IMPORT_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        PvDestroyImportNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);
}

PPV_IMPORT_NODE PvGetSelectedImportNode(
    _In_ PPV_IMPORT_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPV_IMPORT_NODE importNode = Context->NodeList->Items[i];

        if (importNode->Node.Selected)
            return importNode;
    }

    return NULL;
}

_Success_(return)
BOOLEAN PvGetSelectedImportNodes(
    _In_ PPV_IMPORT_CONTEXT Context,
    _Out_ PPV_IMPORT_NODE **Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPV_IMPORT_NODE node = (PPV_IMPORT_NODE)Context->NodeList->Items[i];

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

VOID PvInitializeImportTree(
    _In_ PPV_IMPORT_CONTEXT Context,
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle
    )
{
    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PPV_IMPORT_NODE),
        PvImportNodeHashtableCompareFunction,
        PvImportNodeHashtableHashFunction,
        100
        );
    Context->NodeList = PhCreateList(100);

    Context->ParentWindowHandle = ParentWindowHandle;
    Context->TreeNewHandle = TreeNewHandle;
    PhSetControlTheme(TreeNewHandle, L"explorer");

    TreeNew_SetCallback(TreeNewHandle, PvImportTreeNewCallback, Context);
    TreeNew_SetRedraw(TreeNewHandle, FALSE);

    PhAddTreeNewColumnEx2(TreeNewHandle, PV_IMPORT_TREE_COLUMN_ITEM_INDEX, TRUE, L"#", 40, PH_ALIGN_LEFT, PV_IMPORT_TREE_COLUMN_ITEM_INDEX, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_IMPORT_TREE_COLUMN_ITEM_RVA, TRUE, L"RVA", 80, PH_ALIGN_LEFT, PV_IMPORT_TREE_COLUMN_ITEM_RVA, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_IMPORT_TREE_COLUMN_ITEM_DLL, TRUE, L"DLL", 80, PH_ALIGN_LEFT, PV_IMPORT_TREE_COLUMN_ITEM_DLL, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_IMPORT_TREE_COLUMN_ITEM_NAME, TRUE, L"Name", 250, PH_ALIGN_LEFT, PV_IMPORT_TREE_COLUMN_ITEM_NAME, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_IMPORT_TREE_COLUMN_ITEM_HINT, TRUE, L"Hint", 50, PH_ALIGN_LEFT, PV_IMPORT_TREE_COLUMN_ITEM_HINT, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_IMPORT_TREE_COLUMN_ITEM_ORDINAL, TRUE, L"Ordinal", 80, PH_ALIGN_LEFT, PV_IMPORT_TREE_COLUMN_ITEM_ORDINAL, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_IMPORT_TREE_COLUMN_ITEM_ORDINALNAME, TRUE, L"Ordinal name", 80, PH_ALIGN_LEFT, PV_IMPORT_TREE_COLUMN_ITEM_ORDINALNAME, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_IMPORT_TREE_COLUMN_ITEM_SYMBOL, TRUE, L"Undecorated name", 150, PH_ALIGN_LEFT, PV_IMPORT_TREE_COLUMN_ITEM_SYMBOL, 0, 0);

    TreeNew_SetRedraw(TreeNewHandle, TRUE);
    TreeNew_SetSort(TreeNewHandle, PV_IMPORT_TREE_COLUMN_ITEM_INDEX, AscendingSortOrder);

    PhCmInitializeManager(&Context->Cm, TreeNewHandle, PV_IMPORT_TREE_COLUMN_ITEM_MAXIMUM, PvImportTreeNewPostSortFunction);

    PhInitializeTreeNewFilterSupport(&Context->FilterSupport, TreeNewHandle, Context->NodeList);
}

BOOLEAN PvImportTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPV_IMPORT_CONTEXT context = Context;
    PPV_IMPORT_NODE node = (PPV_IMPORT_NODE)Node;

    if (!context->SearchMatchHandle)
        return TRUE;

    if (!PhIsNullOrEmptyString(node->AddressString))
    {
        if (PvSearchControlMatch(context->SearchMatchHandle, &node->AddressString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->DllString))
    {
        if (PvSearchControlMatch(context->SearchMatchHandle, &node->DllString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->NameString))
    {
        if (PvSearchControlMatch(context->SearchMatchHandle, &node->NameString->sr))
            return TRUE;
    }
    else
    {
        static PH_STRINGREF ImportNameSr = PH_STRINGREF_INIT(L"(unnamed)");

        if (PvSearchControlMatch(context->SearchMatchHandle, &ImportNameSr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->HintString))
    {
        if (PvSearchControlMatch(context->SearchMatchHandle, &node->HintString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->UniqueIdString))
    {
        if (PvSearchControlMatch(context->SearchMatchHandle, &node->UniqueIdString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->SymbolString))
    {
        if (PvSearchControlMatch(context->SearchMatchHandle, &node->SymbolString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->OrdinalString))
    {
        if (PvSearchControlMatch(context->SearchMatchHandle, &node->OrdinalString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->OrdinalNameString))
    {
        if (PvSearchControlMatch(context->SearchMatchHandle, &node->OrdinalNameString->sr))
            return TRUE;
    }

    return FALSE;
}
