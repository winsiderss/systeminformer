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
#include "colmgr.h"

static PH_STRINGREF EmptyResourcesText = PH_STRINGREF_INIT(L"There are no resources to display.");
static PH_STRINGREF LoadingResourcesText = PH_STRINGREF_INIT(L"Loading resources from image...");

typedef enum _PV_RESOURCES_TREE_COLUMN_ITEM
{
    PV_RESOURCES_TREE_COLUMN_ITEM_INDEX,
    PV_RESOURCES_TREE_COLUMN_ITEM_TYPE,
    PV_RESOURCES_TREE_COLUMN_ITEM_NAME,
    PV_RESOURCES_TREE_COLUMN_ITEM_RVA_START,
    PV_RESOURCES_TREE_COLUMN_ITEM_RVA_END,
    PV_RESOURCES_TREE_COLUMN_ITEM_RVA_SIZE,
    PV_RESOURCES_TREE_COLUMN_ITEM_LCID,
    PV_RESOURCES_TREE_COLUMN_ITEM_HASH,
    PV_RESOURCES_TREE_COLUMN_ITEM_ENTROPY,
    PV_RESOURCES_TREE_COLUMN_ITEM_MAXIMUM
} PV_RESOURCES_TREE_COLUMN_ITEM;

typedef struct _PV_RESOURCE_NODE
{
    PH_TREENEW_NODE Node;

    ULONG64 UniqueId;

    PVOID RvaStart;
    PVOID RvaEnd;
    ULONG RvaSize;
    DOUBLE ResourcesEntropy;
    PPH_STRING UniqueIdString;
    PPH_STRING TypeString;
    PPH_STRING NameString;
    PPH_STRING RvaStartString;
    PPH_STRING RvaEndString;
    PPH_STRING RvaSizeString;
    PPH_STRING LcidString;
    PPH_STRING HashString;
    PPH_STRING EntropyString;

    PH_STRINGREF TextCache[PV_RESOURCES_TREE_COLUMN_ITEM_MAXIMUM];
} PV_RESOURCE_NODE, *PPV_RESOURCE_NODE;

typedef struct _PV_RESOURCES_CONTEXT
{
    HWND DialogHandle;
    HWND SearchHandle;
    HWND TreeNewHandle;
    HWND ParentWindowHandle;

    PPH_STRING SearchboxText;
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
} PV_RESOURCES_CONTEXT, *PPV_RESOURCES_CONTEXT;

BOOLEAN PvResourcesNodeHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

ULONG PvResourcesNodeHashtableHashFunction(
    _In_ PVOID Entry
    );

VOID PvDestroyResourcesNode(
    _In_ PPV_RESOURCE_NODE Node
    );

VOID PvInitializeResourcesTree(
    _In_ PPV_RESOURCES_CONTEXT Context,
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle
    );

VOID PvDeleteResourcesTree(
    _In_ PPV_RESOURCES_CONTEXT Context
    );

VOID PvResourcesAddTreeNode(
    _In_ PPV_RESOURCES_CONTEXT Context,
    _In_ PPV_RESOURCE_NODE Entry
    );

BOOLEAN PvResourcesTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );

VOID PhLoadSettingsResourcesList(
    _Inout_ PPV_RESOURCES_CONTEXT Context
    );

VOID PhSaveSettingsResourcesList(
    _Inout_ PPV_RESOURCES_CONTEXT Context
    );

_Success_(return)
BOOLEAN PvGetSelectedResourcesNodes(
    _In_ PPV_RESOURCES_CONTEXT Context,
    _Out_ PPV_RESOURCE_NODE **Nodes,
    _Out_ PULONG NumberOfNodes
    );

VOID PvAddPendingResourcesNodes(
    _In_ PPV_RESOURCES_CONTEXT Context
    )
{
    ULONG i;
    BOOLEAN needsFullUpdate = FALSE;

    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    PhAcquireQueuedLockExclusive(&Context->SearchResultsLock);

    for (i = Context->SearchResultsAddIndex; i < Context->SearchResults->Count; i++)
    {
        PvResourcesAddTreeNode(Context, Context->SearchResults->Items[i]);
        needsFullUpdate = TRUE;
    }
    Context->SearchResultsAddIndex = i;

    PhReleaseQueuedLockExclusive(&Context->SearchResultsLock);

    if (needsFullUpdate)
        TreeNew_NodesStructured(Context->TreeNewHandle);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);
}

PPH_STRING PvpGetResourceTypeString(
    _In_ ULONG_PTR Type
    )
{
    switch (Type)
    {
    case RT_CURSOR:
        return PhCreateString(L"RT_CURSOR");
    case RT_BITMAP:
        return PhCreateString(L"RT_BITMAP");
    case RT_ICON:
        return PhCreateString(L"RT_ICON");
    case RT_MENU:
        return PhCreateString(L"RT_MENU");
    case RT_DIALOG:
        return PhCreateString(L"RT_DIALOG");
    case RT_STRING:
        return PhCreateString(L"RT_STRING");
    case RT_FONTDIR:
        return PhCreateString(L"RT_FONTDIR");
    case RT_FONT:
        return PhCreateString(L"RT_FONT");
    case RT_ACCELERATOR:
        return PhCreateString(L"RT_ACCELERATOR");
    case RT_RCDATA:
        return PhCreateString(L"RT_RCDATA");
    case RT_MESSAGETABLE:
        return PhCreateString(L"RT_MESSAGETABLE");
    case RT_GROUP_CURSOR:
        return PhCreateString(L"RT_GROUP_CURSOR");
    case RT_GROUP_ICON:
        return PhCreateString(L"RT_GROUP_ICON");
    case RT_VERSION:
        return PhCreateString(L"RT_VERSION");
    case RT_ANICURSOR:
        return PhCreateString(L"RT_ANICURSOR");
    case RT_MANIFEST:
        return PhCreateString(L"RT_MANIFEST");
    }

    return PhFormatUInt64((ULONG)Type, FALSE);
}

PPH_STRING PvpPeResourceDumpFileName(
    _In_ HWND ParentWindow
    )
{
    static PH_FILETYPE_FILTER filters[] =
    {
        { L"Resource data (*.data)", L"*.data" },
        { L"All files (*.*)", L"*.*" }
    };
    PPH_STRING fileName = NULL;
    PVOID fileDialog;

    if (fileDialog = PhCreateSaveFileDialog())
    {
        PhSetFileDialogFilter(fileDialog, filters, RTL_NUMBER_OF(filters));
        //PhSetFileDialogFileName(fileDialog, L"file.data");

        if (PhShowFileDialog(ParentWindow, fileDialog))
        {
            fileName = PhGetFileDialogFileName(fileDialog);
        }

        PhFreeFileDialog(fileDialog);
    }

    return fileName;
}

VOID PvpPeResourceSaveToFile(
    _In_ HWND WindowHandle,
    _In_ PPV_RESOURCE_NODE ResourceNode
    )
{
    PH_MAPPED_IMAGE_RESOURCES resources;
    PH_IMAGE_RESOURCE_ENTRY entry;
    PPH_STRING fileName;

    if (!(fileName = PvpPeResourceDumpFileName(WindowHandle)))
        return;

    if (NT_SUCCESS(PhGetMappedImageResources(&resources, &PvMappedImage)))
    {
        for (ULONG i = 0; i < resources.NumberOfEntries; i++)
        {
            if (i + 1 != ResourceNode->UniqueId)
                continue;

            entry = resources.ResourceEntries[i];

            if (entry.Data && entry.Size)
            {
                NTSTATUS status;
                HANDLE fileHandle;

                status = PhCreateFileWin32(
                    &fileHandle,
                    PhGetString(fileName),
                    FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_OVERWRITE_IF,
                    FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                    );

                if (NT_SUCCESS(status))
                {
                    IO_STATUS_BLOCK isb;

                    __try
                    {
                        status = NtWriteFile(
                            fileHandle,
                            NULL,
                            NULL,
                            NULL,
                            &isb,
                            entry.Data,
                            entry.Size,
                            NULL,
                            NULL
                            );
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        status = GetExceptionCode();    
                    }

                    NtClose(fileHandle);
                }

                if (!NT_SUCCESS(status))
                {
                    PhShowStatus(WindowHandle, L"Unable to save resource.", status, 0);
                }
            }
        }

        PhFree(resources.ResourceEntries);
    }

    PhDereferenceObject(fileName);
}

NTSTATUS PvpPeResourcesEnumerateThread(
    _In_ PPV_RESOURCES_CONTEXT Context
    )
{
    PH_MAPPED_IMAGE_RESOURCES resources;
    PH_IMAGE_RESOURCE_ENTRY entry;
    ULONG i;

    if (NT_SUCCESS(PhGetMappedImageResources(&resources, &PvMappedImage)))
    {
        for (i = 0; i < resources.NumberOfEntries; i++)
        {
            PPV_RESOURCE_NODE resourceNode;
            WCHAR value[PH_INT64_STR_LEN_1];

            entry = resources.ResourceEntries[i];

            resourceNode = PhAllocateZero(sizeof(PV_RESOURCE_NODE));
            resourceNode->UniqueId = i + 1;
            resourceNode->UniqueIdString = PhFormatUInt64(resourceNode->UniqueId, FALSE);

            resourceNode->RvaStart = UlongToPtr(entry.Offset);
            PhPrintPointer(value, resourceNode->RvaStart);
            resourceNode->RvaStartString = PhCreateString(value);
            resourceNode->RvaEnd = PTR_ADD_OFFSET(entry.Offset, entry.Size);
            PhPrintPointer(value, resourceNode->RvaEnd);
            resourceNode->RvaEndString = PhCreateString(value);
            resourceNode->RvaSize = entry.Size;
            resourceNode->RvaSizeString = PhFormatSize(resourceNode->RvaSize, ULONG_MAX);

            if (IS_INTRESOURCE(entry.Type))
            {
                resourceNode->TypeString = PvpGetResourceTypeString(entry.Type);
            }
            else
            {
                PIMAGE_RESOURCE_DIR_STRING_U resourceString = (PIMAGE_RESOURCE_DIR_STRING_U)entry.Type;

                resourceNode->TypeString = PhCreateStringEx(resourceString->NameString, resourceString->Length * sizeof(WCHAR));
            }

            if (IS_INTRESOURCE(entry.Name))
            {
                PhPrintUInt32(value, (ULONG)entry.Name);
                resourceNode->NameString = PhCreateString(value);
            }
            else
            {
                PIMAGE_RESOURCE_DIR_STRING_U resourceString = (PIMAGE_RESOURCE_DIR_STRING_U)entry.Name;

                resourceNode->NameString = PhCreateStringEx(resourceString->NameString, resourceString->Length * sizeof(WCHAR));
            }

            if (IS_INTRESOURCE(entry.Language))
            {
                if ((ULONG)entry.Language)
                {
#if (PHNT_VERSION >= PHNT_WIN7)
                    UNICODE_STRING localeNameUs;
                    WCHAR localeName[LOCALE_NAME_MAX_LENGTH] = { UNICODE_NULL };

                    RtlInitEmptyUnicodeString(&localeNameUs, localeName, sizeof(localeName));

                    if (NT_SUCCESS(RtlLcidToLocaleName((ULONG)entry.Language, &localeNameUs, 0, FALSE)))
                    {
                        //PhPrintUInt32(value, (ULONG)entry.Language);
                        //resourceNode->LcidString = PhCreateString(value);
                        resourceNode->LcidString = PhFormatString(L"%lu (%s)", (ULONG)entry.Language, localeName);
                    }
                    else
                    {
                        PhPrintUInt32(value, (ULONG)entry.Language);
                        resourceNode->LcidString = PhCreateString(value);
                    }
#else
                    PhPrintUInt32(value, (ULONG)entry.Language);
                    resourceNode->LcidString = PhCreateString(value);
#endif
                }
                else // LOCALE_NEUTRAL
                {
                    resourceNode->LcidString = PhCreateString(L"Neutral");
                }
            }
            else
            {
                PIMAGE_RESOURCE_DIR_STRING_U resourceString = (PIMAGE_RESOURCE_DIR_STRING_U)entry.Language;

                resourceNode->LcidString = PhCreateStringEx(resourceString->NameString, resourceString->Length * sizeof(WCHAR));
            }

            if (entry.Data && entry.Size)
            {
                __try
                {
                    PH_HASH_CONTEXT hashContext;
                    UCHAR hash[32];

                    PhInitializeHash(&hashContext, Md5HashAlgorithm);
                    PhUpdateHash(&hashContext, entry.Data, entry.Size);

                    if (PhFinalHash(&hashContext, hash, 16, NULL))
                    {
                        resourceNode->HashString = PhBufferToHexString(hash, 16);
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    //resourceNode->HashString = PhGetNtMessage(GetExceptionCode());
                    resourceNode->HashString = PhGetWin32Message(PhNtStatusToDosError(GetExceptionCode())); // WIN32_FROM_NTSTATUS
                }
            }

            if (entry.Data && entry.Size)
            {
                __try
                {
                    DOUBLE imageResourceEntropy;

                    imageResourceEntropy = PvCalculateEntropyBuffer(
                        entry.Data,
                        entry.Size,
                        NULL
                        );

                    resourceNode->ResourcesEntropy = imageResourceEntropy;
                    resourceNode->EntropyString = PvFormatDoubleCropZero(imageResourceEntropy, 2);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    //resourceNode->EntropyString = PhGetNtMessage(GetExceptionCode());
                    resourceNode->EntropyString = PhGetWin32Message(PhNtStatusToDosError(GetExceptionCode())); // WIN32_FROM_NTSTATUS
                }
            }

            PhAcquireQueuedLockExclusive(&Context->SearchResultsLock);
            PhAddItemList(Context->SearchResults, resourceNode);
            PhReleaseQueuedLockExclusive(&Context->SearchResultsLock);
        }

        PhFree(resources.ResourceEntries);
    }

    PostMessage(Context->DialogHandle, WM_PV_SEARCH_FINISHED, 0, 0);
    return STATUS_SUCCESS;
}

INT_PTR CALLBACK PvPeResourcesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_RESOURCES_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PV_RESOURCES_CONTEXT));
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
            context->SearchboxText = PhReferenceEmptyString();
            context->SearchResults = PhCreateList(1);

            PvCreateSearchControl(context->SearchHandle, L"Search Resources (Ctrl+K)");

            PvInitializeResourcesTree(context, hwndDlg, context->TreeNewHandle);
            PhAddTreeNewFilter(&context->FilterSupport, PvResourcesTreeFilterCallback, context);
            PhLoadSettingsResourcesList(context);
            PvConfigTreeBorders(context->TreeNewHandle);

            TreeNew_SetEmptyText(context->TreeNewHandle, &LoadingResourcesText, 0);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->SearchHandle, NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);

            PhCreateThread2(PvpPeResourcesEnumerateThread, context);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveSettingsResourcesList(context);
            PvDeleteResourcesTree(context);
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
            PvAddPendingResourcesNodes(context);

            TreeNew_SetEmptyText(context->TreeNewHandle, &EmptyResourcesText, 0);

            TreeNew_NodesStructured(context->TreeNewHandle);
        }
        break;
    case WM_PV_SEARCH_SHOWMENU:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
            PPH_EMENU menu;
            PPH_EMENU_ITEM selectedItem;
            PPV_RESOURCE_NODE* sectionNodes = NULL;
            ULONG numberOfNodes = 0;

            if (!PvGetSelectedResourcesNodes(context, &sectionNodes, &numberOfNodes))
                break;

            if (numberOfNodes != 0)
            {
                menu = PhCreateEMenu();
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"Save resource...", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, USHRT_MAX, L"Copy", NULL, NULL), ULONG_MAX);
                PhInsertCopyCellEMenuItem(menu, USHRT_MAX, context->TreeNewHandle, contextMenuEvent->Column);

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
                    if (!PhHandleCopyCellEMenuItem(selectedItem))
                    {
                        switch (selectedItem->Id)
                        {
                        case 1:
                            PvpPeResourceSaveToFile(hwndDlg, sectionNodes[0]);
                            break;
                        case USHRT_MAX:
                            {
                                PPH_STRING text;

                                text = PhGetTreeNewText(context->TreeNewHandle, 0);
                                PhSetClipboardString(context->TreeNewHandle, &text->sr);
                                PhDereferenceObject(text);
                            }
                            break;
                        }
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

VOID PhLoadSettingsResourcesList(
    _Inout_ PPV_RESOURCES_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhGetStringSetting(L"ImageResourcesTreeListColumns");
    sortSettings = PhGetStringSetting(L"ImageResourcesTreeListSort");
    //Context->Flags = PhGetIntegerSetting(L"ImageResourcesTreeListFlags");

    PhCmLoadSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &settings->sr, &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PhSaveSettingsResourcesList(
    _Inout_ PPV_RESOURCES_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;
    
    settings = PhCmSaveSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &sortSettings);
    
    //PhSetIntegerSetting(L"ImageResourcesTreeListFlags", Context->Flags);
    PhSetStringSetting2(L"ImageResourcesTreeListColumns", &settings->sr);
    PhSetStringSetting2(L"ImageResourcesTreeListSort", &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PvDeleteResourcesTree(
    _In_ PPV_RESOURCES_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PvDestroyResourcesNode(Context->NodeList->Items[i]);
    }

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}

struct _PH_TN_FILTER_SUPPORT* GetResourcesListFilterSupport(
    _In_ PPV_RESOURCES_CONTEXT Context
    )
{
    return &Context->FilterSupport;
}

LONG PvResourcesTreeNewPostSortFunction(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    )
{
    if (Result == 0)
        Result = uintptrcmp((ULONG_PTR)((PPV_RESOURCE_NODE)Node1)->UniqueId, (ULONG_PTR)((PPV_RESOURCE_NODE)Node2)->UniqueId);

    return PhModifySort(Result, SortOrder);
}

BOOLEAN PvResourcesNodeHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPV_RESOURCE_NODE sectionNode1 = *(PPV_RESOURCE_NODE*)Entry1;
    PPV_RESOURCE_NODE sectionNode2 = *(PPV_RESOURCE_NODE*)Entry2;

    return sectionNode1->UniqueId == sectionNode2->UniqueId;
}

ULONG PvResourcesNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashInt64((ULONG_PTR)(*(PPV_RESOURCE_NODE*)Entry)->UniqueId);
}

VOID PvResourcesAddTreeNode(
    _In_ PPV_RESOURCES_CONTEXT Context,
    _In_ PPV_RESOURCE_NODE Entry
    )
{
    PhInitializeTreeNewNode(&Entry->Node);

    memset(Entry->TextCache, 0, sizeof(PH_STRINGREF) * PV_RESOURCES_TREE_COLUMN_ITEM_MAXIMUM);
    Entry->Node.TextCache = Entry->TextCache;
    Entry->Node.TextCacheSize = PV_RESOURCES_TREE_COLUMN_ITEM_MAXIMUM;

    if (PhAddEntryHashtable(Context->NodeHashtable, &Entry)) // HACK
    {
        PhAddItemList(Context->NodeList, Entry);

        if (Context->FilterSupport.NodeList)
        {
            Entry->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->FilterSupport, &Entry->Node);
        }
    }
}

PPV_RESOURCE_NODE PvFindResourcesNode(
    _In_ PPV_RESOURCES_CONTEXT Context,
    _In_ PPH_STRING Name
    )
{
    PV_RESOURCE_NODE lookupSymbolNode;
    PPV_RESOURCE_NODE lookupSymbolNodePtr = &lookupSymbolNode;
    PPV_RESOURCE_NODE* threadNode;

    lookupSymbolNode.UniqueIdString = Name;

    threadNode = (PPV_RESOURCE_NODE*)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupSymbolNodePtr
        );

    if (threadNode)
        return *threadNode;
    else
        return NULL;
}

VOID PvRemoveResourcesNode(
    _In_ PPV_RESOURCES_CONTEXT Context,
    _In_ PPV_RESOURCE_NODE Node
    )
{
    ULONG index;

    PhRemoveEntryHashtable(Context->NodeHashtable, &Node);

    if ((index = PhFindItemList(Context->NodeList, Node)) != ULONG_MAX)
        PhRemoveItemList(Context->NodeList, index);

    PvDestroyResourcesNode(Node);
}

VOID PvDestroyResourcesNode(
    _In_ PPV_RESOURCE_NODE Node
    )
{
    PhFree(Node);
}

#define SORT_FUNCTION(Column) PvResourcesTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PvResourcesTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPV_RESOURCE_NODE node1 = *(PPV_RESOURCE_NODE *)_elem1; \
    PPV_RESOURCE_NODE node2 = *(PPV_RESOURCE_NODE *)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)node1->UniqueId, (ULONG_PTR)node2->UniqueId); \
    \
    return PhModifySort(sortResult, ((PPV_RESOURCES_CONTEXT)_context)->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Index)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->UniqueId, (ULONG_PTR)node2->UniqueId);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Type)
{
    sortResult = PhCompareStringWithNull(node1->TypeString, node2->TypeString, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareStringWithNull(node1->NameString, node2->NameString, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(RvaStart)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->RvaStart, (ULONG_PTR)node2->RvaStart);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(RvaEnd)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->RvaEnd, (ULONG_PTR)node2->RvaEnd);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(RvaSize)
{
    sortResult = uintcmp(node1->RvaSize, node2->RvaSize);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Lcid)
{
    sortResult = PhCompareStringWithNull(node1->LcidString, node2->LcidString, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Hash)
{
    sortResult = PhCompareStringWithNull(node1->HashString, node2->HashString, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Entropy)
{
    sortResult = doublecmp(node1->ResourcesEntropy, node2->ResourcesEntropy);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PvResourcesTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PPV_RESOURCES_CONTEXT context = Context;
    PPV_RESOURCE_NODE node;

    if (!context)
        return FALSE;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            if (!getChildren)
                break;

            node = (PPV_RESOURCE_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Index),
                    SORT_FUNCTION(Type),
                    SORT_FUNCTION(Name),
                    SORT_FUNCTION(RvaStart),
                    SORT_FUNCTION(RvaEnd),
                    SORT_FUNCTION(RvaSize),
                    SORT_FUNCTION(Lcid),
                    SORT_FUNCTION(Hash),
                    SORT_FUNCTION(Entropy),
                };
                int (__cdecl *sortFunction)(void *, const void *, const void *);

                if (context->TreeNewSortColumn < PV_RESOURCES_TREE_COLUMN_ITEM_MAXIMUM)
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

            node = (PPV_RESOURCE_NODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;

            if (!getCellText)
                break;

            node = (PPV_RESOURCE_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PV_RESOURCES_TREE_COLUMN_ITEM_INDEX:
                getCellText->Text = PhGetStringRef(node->UniqueIdString);
                break;
            case PV_RESOURCES_TREE_COLUMN_ITEM_TYPE:
                getCellText->Text = PhGetStringRef(node->TypeString);
                break;
            case PV_RESOURCES_TREE_COLUMN_ITEM_NAME:
                getCellText->Text = PhGetStringRef(node->NameString);
                break;
            case PV_RESOURCES_TREE_COLUMN_ITEM_RVA_START:
                getCellText->Text = PhGetStringRef(node->RvaStartString);
                break;
            case PV_RESOURCES_TREE_COLUMN_ITEM_RVA_END:
                getCellText->Text = PhGetStringRef(node->RvaEndString);
                break;
            case PV_RESOURCES_TREE_COLUMN_ITEM_RVA_SIZE:
                getCellText->Text = PhGetStringRef(node->RvaSizeString);
                break;
            case PV_RESOURCES_TREE_COLUMN_ITEM_LCID:
                getCellText->Text = PhGetStringRef(node->LcidString);
                break;
            case PV_RESOURCES_TREE_COLUMN_ITEM_HASH:
                getCellText->Text = PhGetStringRef(node->HashString);
                break;
            case PV_RESOURCES_TREE_COLUMN_ITEM_ENTROPY:
                getCellText->Text = PhGetStringRef(node->EntropyString);
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

            node = (PPV_RESOURCE_NODE)getNodeColor->Node;

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

VOID PvResourcesClearTree(
    _In_ PPV_RESOURCES_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        PvDestroyResourcesNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);
}

PPV_RESOURCE_NODE PvGetSelectedResourcesNode(
    _In_ PPV_RESOURCES_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPV_RESOURCE_NODE node = Context->NodeList->Items[i];

        if (node->Node.Selected)
            return node;
    }

    return NULL;
}

_Success_(return)
BOOLEAN PvGetSelectedResourcesNodes(
    _In_ PPV_RESOURCES_CONTEXT Context,
    _Out_ PPV_RESOURCE_NODE **Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPV_RESOURCE_NODE node = (PPV_RESOURCE_NODE)Context->NodeList->Items[i];

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

VOID PvInitializeResourcesTree(
    _In_ PPV_RESOURCES_CONTEXT Context,
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle
    )
{
    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PPV_RESOURCE_NODE),
        PvResourcesNodeHashtableCompareFunction,
        PvResourcesNodeHashtableHashFunction,
        100
        );
    Context->NodeList = PhCreateList(100);

    Context->ParentWindowHandle = ParentWindowHandle;
    Context->TreeNewHandle = TreeNewHandle;
    PhSetControlTheme(TreeNewHandle, L"explorer");

    TreeNew_SetCallback(TreeNewHandle, PvResourcesTreeNewCallback, Context);
    TreeNew_SetRedraw(TreeNewHandle, FALSE);

    PhAddTreeNewColumnEx2(TreeNewHandle, PV_RESOURCES_TREE_COLUMN_ITEM_INDEX, TRUE, L"#", 40, PH_ALIGN_LEFT, PV_RESOURCES_TREE_COLUMN_ITEM_INDEX, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_RESOURCES_TREE_COLUMN_ITEM_TYPE, TRUE, L"Type", 150, PH_ALIGN_LEFT, PV_RESOURCES_TREE_COLUMN_ITEM_TYPE, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_RESOURCES_TREE_COLUMN_ITEM_NAME, TRUE, L"Name", 80, PH_ALIGN_LEFT, PV_RESOURCES_TREE_COLUMN_ITEM_NAME, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_RESOURCES_TREE_COLUMN_ITEM_RVA_START, TRUE, L"RVA (start)", 80, PH_ALIGN_LEFT, PV_RESOURCES_TREE_COLUMN_ITEM_RVA_START, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_RESOURCES_TREE_COLUMN_ITEM_RVA_END, TRUE, L"RVA (end)", 80, PH_ALIGN_LEFT, PV_RESOURCES_TREE_COLUMN_ITEM_RVA_END, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_RESOURCES_TREE_COLUMN_ITEM_RVA_SIZE, TRUE, L"Size", 80, PH_ALIGN_LEFT, PV_RESOURCES_TREE_COLUMN_ITEM_RVA_SIZE, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_RESOURCES_TREE_COLUMN_ITEM_LCID, TRUE, L"Language", 100, PH_ALIGN_LEFT, PV_RESOURCES_TREE_COLUMN_ITEM_LCID, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_RESOURCES_TREE_COLUMN_ITEM_HASH, TRUE, L"Hash", 100, PH_ALIGN_LEFT, PV_RESOURCES_TREE_COLUMN_ITEM_HASH, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_RESOURCES_TREE_COLUMN_ITEM_ENTROPY, TRUE, L"Entropy", 100, PH_ALIGN_LEFT, PV_RESOURCES_TREE_COLUMN_ITEM_ENTROPY, 0, 0);

    //TreeNew_SetRowHeight(TreeNewHandle, 22);

    TreeNew_SetRedraw(TreeNewHandle, TRUE);
    TreeNew_SetSort(TreeNewHandle, PV_RESOURCES_TREE_COLUMN_ITEM_INDEX, AscendingSortOrder);

    PhCmInitializeManager(&Context->Cm, TreeNewHandle, PV_RESOURCES_TREE_COLUMN_ITEM_MAXIMUM, PvResourcesTreeNewPostSortFunction);

    PhInitializeTreeNewFilterSupport(&Context->FilterSupport, TreeNewHandle, Context->NodeList);
}

BOOLEAN PvResourcesWordMatchStringRef(
    _In_ PPV_RESOURCES_CONTEXT Context,
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

BOOLEAN PvResourcesWordMatchStringZ(
    _In_ PPV_RESOURCES_CONTEXT Context,
    _In_ PWSTR Text
    )
{
    PH_STRINGREF text;

    PhInitializeStringRef(&text, Text);
    return PvResourcesWordMatchStringRef(Context, &text);
}

BOOLEAN PvResourcesTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPV_RESOURCES_CONTEXT context = Context;
    PPV_RESOURCE_NODE node = (PPV_RESOURCE_NODE)Node;

    if (PhIsNullOrEmptyString(context->SearchboxText))
        return TRUE;

    if (!PhIsNullOrEmptyString(node->UniqueIdString))
    {
        if (PvResourcesWordMatchStringRef(context, &node->UniqueIdString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->TypeString))
    {
        if (PvResourcesWordMatchStringRef(context, &node->TypeString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->NameString))
    {
        if (PvResourcesWordMatchStringRef(context, &node->NameString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->RvaStartString))
    {
        if (PvResourcesWordMatchStringRef(context, &node->RvaStartString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->RvaEndString))
    {
        if (PvResourcesWordMatchStringRef(context, &node->RvaEndString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->RvaSizeString))
    {
        if (PvResourcesWordMatchStringRef(context, &node->RvaSizeString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->LcidString))
    {
        if (PvResourcesWordMatchStringRef(context, &node->LcidString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->HashString))
    {
        if (PvResourcesWordMatchStringRef(context, &node->HashString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->EntropyString))
    {
        if (PvResourcesWordMatchStringRef(context, &node->EntropyString->sr))
            return TRUE;
    }

    return FALSE;
}
