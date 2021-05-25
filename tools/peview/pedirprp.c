/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2019-2021 dmex
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
#include "colmgr.h"

#include "ssdeep/fuzzy.h"
#include "tlsh/tlsh_wrapper.h"

static PH_STRINGREF EmptyDirectoriesText = PH_STRINGREF_INIT(L"There are no directories to display.");
static PH_STRINGREF LoadingDirectoriesText = PH_STRINGREF_INIT(L"Loading directories from image...");

typedef enum _PV_DIRECTORY_TREE_COLUMN_ITEM
{
    PV_DIRECTORY_TREE_COLUMN_ITEM_INDEX,
    PV_DIRECTORY_TREE_COLUMN_ITEM_NAME,
    PV_DIRECTORY_TREE_COLUMN_ITEM_RVA_START,
    PV_DIRECTORY_TREE_COLUMN_ITEM_RVA_END,
    PV_DIRECTORY_TREE_COLUMN_ITEM_RVA_SIZE,
    PV_DIRECTORY_TREE_COLUMN_ITEM_SECTION,
    PV_DIRECTORY_TREE_COLUMN_ITEM_HASH,
    PV_DIRECTORY_TREE_COLUMN_ITEM_ENTROPY,
    PV_DIRECTORY_TREE_COLUMN_ITEM_SSDEEP,
    PV_DIRECTORY_TREE_COLUMN_ITEM_TLSH,
    PV_DIRECTORY_TREE_COLUMN_ITEM_MAXIMUM
} PV_DIRECTORY_TREE_COLUMN_ITEM;

typedef struct _PV_DIRECTORY_NODE
{
    PH_TREENEW_NODE Node;

    ULONG64 UniqueId;

    PVOID RvaStart;
    PVOID RvaEnd;
    ULONG RvaSize;
    DOUBLE DirectoryEntropy;
    PPH_STRING UniqueIdString;
    PPH_STRING DirectoryNameString;
    PPH_STRING RvaStartString;
    PPH_STRING RvaEndString;
    PPH_STRING RvaSizeString;
    PPH_STRING SectionNameString;
    PPH_STRING HashString;
    PPH_STRING EntropyString;
    PPH_STRING SsdeepString;
    PPH_STRING TlshString;

    PH_STRINGREF TextCache[PV_DIRECTORY_TREE_COLUMN_ITEM_MAXIMUM];
} PV_DIRECTORY_NODE, *PPV_DIRECTORY_NODE;

typedef struct _PV_DIRECTORY_CONTEXT
{
    HWND DialogHandle;
    HWND SearchHandle;
    HWND TreeNewHandle;
    HWND ParentWindowHandle;
    HANDLE UpdateTimerHandle;

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
} PV_DIRECTORY_CONTEXT, *PPV_DIRECTORY_CONTEXT;

BOOLEAN PvDirectoryNodeHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

ULONG PvDirectoryNodeHashtableHashFunction(
    _In_ PVOID Entry
    );

VOID PvDestroyDirectoryNode(
    _In_ PPV_DIRECTORY_NODE Node
    );

VOID PvInitializeDirectoryTree(
    _In_ PPV_DIRECTORY_CONTEXT Context,
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle
    );

VOID PvDeleteDirectoryTree(
    _In_ PPV_DIRECTORY_CONTEXT Context
    );

VOID PvDirectoryAddTreeNode(
    _In_ PPV_DIRECTORY_CONTEXT Context,
    _In_ PPV_DIRECTORY_NODE Entry
    );

BOOLEAN PvDirectoryTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );

VOID PhLoadSettingsDirectoryList(
    _Inout_ PPV_DIRECTORY_CONTEXT Context
    );

VOID PhSaveSettingsDirectoryList(
    _Inout_ PPV_DIRECTORY_CONTEXT Context
    );

_Success_(return)
BOOLEAN PvGetSelectedDirectoryNodes(
    _In_ PPV_DIRECTORY_CONTEXT Context,
    _Out_ PPV_DIRECTORY_NODE **Nodes,
    _Out_ PULONG NumberOfNodes
    );

VOID PvAddPendingDirectoryNodes(
    _In_ PPV_DIRECTORY_CONTEXT Context
    )
{
    ULONG i;
    BOOLEAN needsFullUpdate = FALSE;

    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    PhAcquireQueuedLockExclusive(&Context->SearchResultsLock);

    for (i = Context->SearchResultsAddIndex; i < Context->SearchResults->Count; i++)
    {
        PvDirectoryAddTreeNode(Context, Context->SearchResults->Items[i]);
        needsFullUpdate = TRUE;
    }
    Context->SearchResultsAddIndex = i;

    PhReleaseQueuedLockExclusive(&Context->SearchResultsLock);

    if (needsFullUpdate)
        TreeNew_NodesStructured(Context->TreeNewHandle);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);
}

VOID CALLBACK PvDirectoryTreeUpdateCallback(
    _In_ PPV_DIRECTORY_CONTEXT Context,
    _In_ BOOLEAN TimerOrWaitFired
    )
{
    if (!Context->UpdateTimerHandle)
        return;

    PvAddPendingDirectoryNodes(Context);

    RtlUpdateTimer(PhGetGlobalTimerQueue(), Context->UpdateTimerHandle, 1000, INFINITE);
}

//BOOLEAN PvpPeCheckImageDataEntryAddress(
//    _In_ ULONG Index,
//    _In_ ULONG StartRva,
//    _In_ ULONG EndRva
//    )
//{
//    PIMAGE_DATA_DIRECTORY directory;
//
//    for (ULONG i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; i++)
//    {
//        if (i == Index)
//            continue;
//
//        if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, i, &directory)))
//        {
//            if ((StartRva >= directory->VirtualAddress) &&
//                (StartRva < directory->VirtualAddress + directory->Size))
//            {
//                return TRUE;
//            }
//
//            if ((EndRva >= directory->VirtualAddress) &&
//                (EndRva < directory->VirtualAddress + directory->Size))
//            {
//                return TRUE;
//            }
//        }
//    }
//
//    return FALSE;
//}

VOID PvpPeEnumerateImageDataDirectory(
    _In_ PPV_DIRECTORY_CONTEXT Context,
    _In_ ULONG Index,
    _In_ PWSTR Name
    )
{
    PPV_DIRECTORY_NODE directoryNode;
    ULONG directoryAddress = 0;
    ULONG directorySize = 0;
    //BOOLEAN directoryOverlay = FALSE;
    PIMAGE_DATA_DIRECTORY directory;
    PIMAGE_SECTION_HEADER directorySection = NULL;
    WCHAR value[PH_INT64_STR_LEN_1];

    directoryNode = PhAllocateZero(sizeof(PV_DIRECTORY_NODE));
    directoryNode->UniqueId = Index + 1;
    directoryNode->UniqueIdString = PhFormatUInt64(directoryNode->UniqueId, FALSE);
    directoryNode->DirectoryNameString = PhCreateString(Name);

    if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, Index, &directory)))
    {
        if (directory->VirtualAddress)
        {
            directoryAddress = directory->VirtualAddress;
        }

        if (directory->Size)
        {
            directorySize = directory->Size;
        }

        if (directoryAddress)
        {
            directorySection = PhMappedImageRvaToSection(&PvMappedImage, directoryAddress);
        }

        //if (directoryAddress && directorySize)
        //{
        //    directoryOverlay = PvpPeCheckImageDataEntryAddress(
        //        Index,
        //        directoryAddress,
        //        PtrToUlong(PTR_ADD_OFFSET(directoryAddress, directorySize))
        //        );
        //}
    }

    if (directoryAddress)
    {
        directoryNode->RvaStart = UlongToPtr(directoryAddress);
        PhPrintPointer(value, directoryNode->RvaStart);
        directoryNode->RvaStartString = PhCreateString(value);
    }

    if (directorySize)
    {
        directoryNode->RvaEnd = PTR_ADD_OFFSET(directoryAddress, directorySize);
        PhPrintPointer(value, directoryNode->RvaEnd);
        directoryNode->RvaEndString = PhCreateString(value);
        directoryNode->RvaSize = directorySize;
        directoryNode->RvaSizeString = PhFormatSize(directorySize, ULONG_MAX);
    }

    if (directorySection)
    {
        ULONG sectionNameLength = 0;
        WCHAR sectionName[IMAGE_SIZEOF_SHORT_NAME + 1];

        if (PhGetMappedImageSectionName(
            directorySection,
            sectionName,
            RTL_NUMBER_OF(sectionName),
            &sectionNameLength
            ))
        {
            directoryNode->SectionNameString = PhCreateStringEx(sectionName, sectionNameLength * sizeof(WCHAR));
        }
    }

    if (directoryAddress && directorySize)
    {
        __try
        {
            PVOID imageDirectoryData;
            PH_HASH_CONTEXT hashContext;
            UCHAR hash[32];

            if (imageDirectoryData = PhMappedImageRvaToVa(&PvMappedImage, directoryAddress, NULL))
            {
                PhInitializeHash(&hashContext, Md5HashAlgorithm); // PhGetIntegerSetting(L"HashAlgorithm")
                PhUpdateHash(&hashContext, imageDirectoryData, directorySize);

                if (PhFinalHash(&hashContext, hash, 16, NULL))
                {
                    directoryNode->HashString = PhBufferToHexString(hash, 16);
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            //directoryNode->HashString = PhGetNtMessage(GetExceptionCode());
            directoryNode->HashString = PhGetWin32Message(RtlNtStatusToDosError(GetExceptionCode())); // WIN32_FROM_NTSTATUS
        }

        __try
        {
            PVOID imageDirectoryData;
            DOUBLE imageDirectoryEntropy;

            if (imageDirectoryData = PhMappedImageRvaToVa(&PvMappedImage, directoryAddress, NULL))
            {
                imageDirectoryEntropy = PvCalculateEntropyBuffer(
                    imageDirectoryData,
                    directorySize,
                    NULL
                    );

                directoryNode->DirectoryEntropy = imageDirectoryEntropy;
                directoryNode->EntropyString = PvFormatDoubleCropZero(imageDirectoryEntropy, 2);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            //directoryNode->EntropyString = PhGetNtMessage(GetExceptionCode());
            directoryNode->EntropyString = PhGetWin32Message(RtlNtStatusToDosError(GetExceptionCode())); // WIN32_FROM_NTSTATUS
        }

        __try
        {
            PVOID imageDirectoryData;
            PPH_STRING ssdeepHashString = NULL;

            if (imageDirectoryData = PhMappedImageRvaToVa(&PvMappedImage, directoryAddress, NULL))
            {
                fuzzy_hash_buffer(
                    imageDirectoryData,
                    directorySize,
                    &ssdeepHashString
                    );

                if (ssdeepHashString)
                {
                    directoryNode->SsdeepString = ssdeepHashString;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            //directoryNode->SsdeepString = PhGetNtMessage(GetExceptionCode());
            directoryNode->SsdeepString = PhGetWin32Message(RtlNtStatusToDosError(GetExceptionCode())); // WIN32_FROM_NTSTATUS
        }

        __try
        {
            PVOID imageDirectoryData;
            PPH_STRING tlshHashString = NULL;

            if (imageDirectoryData = PhMappedImageRvaToVa(&PvMappedImage, directoryAddress, NULL))
            {
                //
                // This can fail in TLSH library during finalization when
                // "buckets must be more than 50% non-zero" (see: tlsh_impl.cpp)
                //
                PvGetTlshBufferHash(
                    imageDirectoryData,
                    directorySize,
                    &tlshHashString
                    );

                if (!PhIsNullOrEmptyString(tlshHashString))
                {
                    directoryNode->TlshString = tlshHashString;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            //sectionNode->TlshString = PhGetNtMessage(GetExceptionCode());
            directoryNode->TlshString = PhGetWin32Message(RtlNtStatusToDosError(GetExceptionCode())); // WIN32_FROM_NTSTATUS
        }
    }

    PhAcquireQueuedLockExclusive(&Context->SearchResultsLock);
    PhAddItemList(Context->SearchResults, directoryNode);
    PhReleaseQueuedLockExclusive(&Context->SearchResultsLock);
}

NTSTATUS PvpPeDirectoryEnumerateThread(
    _In_ PPV_DIRECTORY_CONTEXT Context
    )
{
    // for (ULONG i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; i++)
    PvpPeEnumerateImageDataDirectory(Context, IMAGE_DIRECTORY_ENTRY_EXPORT, L"Export");
    PvpPeEnumerateImageDataDirectory(Context, IMAGE_DIRECTORY_ENTRY_IMPORT, L"Import");
    PvpPeEnumerateImageDataDirectory(Context, IMAGE_DIRECTORY_ENTRY_RESOURCE, L"Resource");
    PvpPeEnumerateImageDataDirectory(Context, IMAGE_DIRECTORY_ENTRY_EXCEPTION, L"Exception");
    PvpPeEnumerateImageDataDirectory(Context, IMAGE_DIRECTORY_ENTRY_SECURITY, L"Security");
    PvpPeEnumerateImageDataDirectory(Context, IMAGE_DIRECTORY_ENTRY_BASERELOC, L"Base relocation");
    PvpPeEnumerateImageDataDirectory(Context, IMAGE_DIRECTORY_ENTRY_DEBUG, L"Debug");
    PvpPeEnumerateImageDataDirectory(Context, IMAGE_DIRECTORY_ENTRY_ARCHITECTURE, L"Architecture");
    PvpPeEnumerateImageDataDirectory(Context, IMAGE_DIRECTORY_ENTRY_GLOBALPTR, L"Global PTR");
    PvpPeEnumerateImageDataDirectory(Context, IMAGE_DIRECTORY_ENTRY_TLS, L"TLS");
    PvpPeEnumerateImageDataDirectory(Context, IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, L"Load configuration");
    PvpPeEnumerateImageDataDirectory(Context, IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT, L"Bound imports");
    PvpPeEnumerateImageDataDirectory(Context, IMAGE_DIRECTORY_ENTRY_IAT, L"IAT");
    PvpPeEnumerateImageDataDirectory(Context, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT, L"Delay load imports");
    PvpPeEnumerateImageDataDirectory(Context, IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR, L"CLR");

    PostMessage(Context->DialogHandle, WM_PV_SEARCH_FINISHED, 0, 0);
    return STATUS_SUCCESS;
}

INT_PTR CALLBACK PvPeDirectoryDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_DIRECTORY_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PV_DIRECTORY_CONTEXT));
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

            PvCreateSearchControl(context->SearchHandle, L"Search Directories (Ctrl+K)");

            PvInitializeDirectoryTree(context, hwndDlg, context->TreeNewHandle);
            PhAddTreeNewFilter(&context->FilterSupport, PvDirectoryTreeFilterCallback, context);
            PhLoadSettingsDirectoryList(context);
            PvConfigTreeBorders(context->TreeNewHandle);

            TreeNew_SetEmptyText(context->TreeNewHandle, &LoadingDirectoriesText, 0);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->SearchHandle, NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);

            PhCreateThread2(PvpPeDirectoryEnumerateThread, context);

            RtlCreateTimer(
                PhGetGlobalTimerQueue(),
                &context->UpdateTimerHandle,
                PvDirectoryTreeUpdateCallback,
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

            PhSaveSettingsDirectoryList(context);
            PvDeleteDirectoryTree(context);
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
            if (context->UpdateTimerHandle)
            {
                RtlDeleteTimer(PhGetGlobalTimerQueue(), context->UpdateTimerHandle, NULL);
                context->UpdateTimerHandle = NULL;
            }

            PvAddPendingDirectoryNodes(context);

            TreeNew_SetEmptyText(context->TreeNewHandle, &EmptyDirectoriesText, 0);

            TreeNew_NodesStructured(context->TreeNewHandle);
        }
        break;
    case WM_PV_SEARCH_SHOWMENU:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
            PPH_EMENU menu;
            PPH_EMENU_ITEM selectedItem;
            PPV_DIRECTORY_NODE* directoryNodes = NULL;
            ULONG numberOfNodes = 0;

            if (!PvGetSelectedDirectoryNodes(context, &directoryNodes, &numberOfNodes))
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

VOID PhLoadSettingsDirectoryList(
    _Inout_ PPV_DIRECTORY_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;
    
    settings = PhGetStringSetting(L"ImageDirectoryTreeListColumns");
    sortSettings = PhGetStringSetting(L"ImageDirectoryTreeListSort");
    //Context->Flags = PhGetIntegerSetting(L"ImageDirectoryTreeListFlags");

    PhCmLoadSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &settings->sr, &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PhSaveSettingsDirectoryList(
    _Inout_ PPV_DIRECTORY_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;
    
    settings = PhCmSaveSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &sortSettings);
    
    //PhSetIntegerSetting(L"ImageDirectoryTreeListFlags", Context->Flags);
    PhSetStringSetting2(L"ImageDirectoryTreeListColumns", &settings->sr);
    PhSetStringSetting2(L"ImageDirectoryTreeListSort", &sortSettings->sr);
    
    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PvDeleteDirectoryTree(
    _In_ PPV_DIRECTORY_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PvDestroyDirectoryNode(Context->NodeList->Items[i]);
    }

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}

struct _PH_TN_FILTER_SUPPORT* GetDirectoryListFilterSupport(
    _In_ PPV_DIRECTORY_CONTEXT Context
    )
{
    return &Context->FilterSupport;
}

LONG PvDirectoryTreeNewPostSortFunction(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    )
{
    if (Result == 0)
        Result = uintptrcmp((ULONG_PTR)((PPV_DIRECTORY_NODE)Node1)->UniqueId, (ULONG_PTR)((PPV_DIRECTORY_NODE)Node2)->UniqueId);

    return PhModifySort(Result, SortOrder);
}

BOOLEAN PvDirectoryNodeHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPV_DIRECTORY_NODE sectionNode1 = *(PPV_DIRECTORY_NODE*)Entry1;
    PPV_DIRECTORY_NODE sectionNode2 = *(PPV_DIRECTORY_NODE*)Entry2;

    return sectionNode1->UniqueId == sectionNode2->UniqueId;
}

ULONG PvDirectoryNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashInt64((ULONG_PTR)(*(PPV_DIRECTORY_NODE*)Entry)->UniqueId);
}

VOID PvDirectoryAddTreeNode(
    _In_ PPV_DIRECTORY_CONTEXT Context,
    _In_ PPV_DIRECTORY_NODE Entry
    )
{
    PhInitializeTreeNewNode(&Entry->Node);

    memset(Entry->TextCache, 0, sizeof(PH_STRINGREF) * PV_DIRECTORY_TREE_COLUMN_ITEM_MAXIMUM);
    Entry->Node.TextCache = Entry->TextCache;
    Entry->Node.TextCacheSize = PV_DIRECTORY_TREE_COLUMN_ITEM_MAXIMUM;

    if (PhAddEntryHashtable(Context->NodeHashtable, &Entry)) // HACK
    {
        PhAddItemList(Context->NodeList, Entry);

        if (Context->FilterSupport.NodeList)
        {
            Entry->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->FilterSupport, &Entry->Node);
        }
    }
}

PPV_DIRECTORY_NODE PvFindDirectoryNode(
    _In_ PPV_DIRECTORY_CONTEXT Context,
    _In_ PPH_STRING Name
    )
{
    PV_DIRECTORY_NODE lookupSymbolNode;
    PPV_DIRECTORY_NODE lookupSymbolNodePtr = &lookupSymbolNode;
    PPV_DIRECTORY_NODE* threadNode;

    lookupSymbolNode.DirectoryNameString = Name;

    threadNode = (PPV_DIRECTORY_NODE*)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupSymbolNodePtr
        );

    if (threadNode)
        return *threadNode;
    else
        return NULL;
}

VOID PvRemoveDirectoryNode(
    _In_ PPV_DIRECTORY_CONTEXT Context,
    _In_ PPV_DIRECTORY_NODE Node
    )
{
    ULONG index;

    PhRemoveEntryHashtable(Context->NodeHashtable, &Node);

    if ((index = PhFindItemList(Context->NodeList, Node)) != ULONG_MAX)
        PhRemoveItemList(Context->NodeList, index);

    PvDestroyDirectoryNode(Node);
}

VOID PvDestroyDirectoryNode(
    _In_ PPV_DIRECTORY_NODE Node
    )
{
    //if (Node->UniqueIdString)
    //    PhDereferenceObject(Node->UniqueIdString);
    //if (Node->SectionNameString)
    //    PhDereferenceObject(Node->SectionNameString);
    //if (Node->RvaStartString)
    //    PhDereferenceObject(Node->RvaStartString);
    //if (Node->RvaEndString)
    //    PhDereferenceObject(Node->RvaEndString);
    //if (Node->RvaSizeString)
    //    PhDereferenceObject(Node->RvaSizeString);
    //if (Node->HashString)
    //    PhDereferenceObject(Node->HashString);
    //if (Node->EntropyString)
    //    PhDereferenceObject(Node->EntropyString);
    //if (Node->SsdeepString)
    //    PhDereferenceObject(Node->SsdeepString);
    //if (Node->TlshString)
    //    PhDereferenceObject(Node->TlshString);

    PhFree(Node);
}

#define SORT_FUNCTION(Column) PvDirectoryTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PvDirectoryTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPV_DIRECTORY_NODE node1 = *(PPV_DIRECTORY_NODE *)_elem1; \
    PPV_DIRECTORY_NODE node2 = *(PPV_DIRECTORY_NODE *)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)node1->UniqueId, (ULONG_PTR)node2->UniqueId); \
    \
    return PhModifySort(sortResult, ((PPV_DIRECTORY_CONTEXT)_context)->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Index)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->UniqueId, (ULONG_PTR)node2->UniqueId);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareStringWithNull(node1->DirectoryNameString, node2->DirectoryNameString, FALSE);
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

BEGIN_SORT_FUNCTION(Section)
{
    sortResult = PhCompareStringWithNull(node1->SectionNameString, node2->SectionNameString, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Hash)
{
    sortResult = PhCompareStringWithNull(node1->HashString, node2->HashString, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Entropy)
{
    sortResult = doublecmp(node1->DirectoryEntropy, node2->DirectoryEntropy);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Ssdeep)
{
    sortResult = PhCompareStringWithNull(node1->SsdeepString, node2->SsdeepString, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Tlsh)
{
    sortResult = PhCompareStringWithNull(node1->TlshString, node2->TlshString, FALSE);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PvDirectoryTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PPV_DIRECTORY_CONTEXT context = Context;
    PPV_DIRECTORY_NODE node;

    if (!context)
        return FALSE;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            if (!getChildren)
                break;

            node = (PPV_DIRECTORY_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Index),
                    SORT_FUNCTION(Name),
                    SORT_FUNCTION(RvaStart),
                    SORT_FUNCTION(RvaEnd),
                    SORT_FUNCTION(RvaSize),
                    SORT_FUNCTION(Section),
                    SORT_FUNCTION(Hash),
                    SORT_FUNCTION(Entropy),
                    SORT_FUNCTION(Ssdeep),
                    SORT_FUNCTION(Tlsh),
                };
                int (__cdecl *sortFunction)(void *, const void *, const void *);

                if (context->TreeNewSortColumn < PV_DIRECTORY_TREE_COLUMN_ITEM_MAXIMUM)
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

            node = (PPV_DIRECTORY_NODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;

            if (!getCellText)
                break;

            node = (PPV_DIRECTORY_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PV_DIRECTORY_TREE_COLUMN_ITEM_INDEX:
                getCellText->Text = PhGetStringRef(node->UniqueIdString);
                break;
            case PV_DIRECTORY_TREE_COLUMN_ITEM_NAME:
                getCellText->Text = PhGetStringRef(node->DirectoryNameString);
                break;
            case PV_DIRECTORY_TREE_COLUMN_ITEM_RVA_START:
                getCellText->Text = PhGetStringRef(node->RvaStartString);
                break;
            case PV_DIRECTORY_TREE_COLUMN_ITEM_RVA_END:
                getCellText->Text = PhGetStringRef(node->RvaEndString);
                break;
            case PV_DIRECTORY_TREE_COLUMN_ITEM_RVA_SIZE:
                getCellText->Text = PhGetStringRef(node->RvaSizeString);
                break;
            case PV_DIRECTORY_TREE_COLUMN_ITEM_SECTION:
                getCellText->Text = PhGetStringRef(node->SectionNameString);
                break;
            case PV_DIRECTORY_TREE_COLUMN_ITEM_HASH:
                getCellText->Text = PhGetStringRef(node->HashString);
                break;
            case PV_DIRECTORY_TREE_COLUMN_ITEM_ENTROPY:
                getCellText->Text = PhGetStringRef(node->EntropyString);
                break;
            case PV_DIRECTORY_TREE_COLUMN_ITEM_SSDEEP:
                getCellText->Text = PhGetStringRef(node->SsdeepString);
                break;
            case PV_DIRECTORY_TREE_COLUMN_ITEM_TLSH:
                getCellText->Text = PhGetStringRef(node->TlshString);
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

            node = (PPV_DIRECTORY_NODE)getNodeColor->Node;

            //if (!node)
            //    NOTHING; // Dummy
            //else if (node->Characteristics & IMAGE_SCN_MEM_WRITE)
            //    getNodeColor->BackColor = RGB(0xf0, 0xa0, 0xa0);
            //else if (node->Characteristics & IMAGE_SCN_MEM_EXECUTE)
            //    getNodeColor->BackColor = RGB(0xff, 0x93, 0x14);
            //else if (node->Characteristics & IMAGE_SCN_CNT_CODE)
            //    getNodeColor->BackColor = RGB(0xe0, 0xf0, 0xe0);
            //else if (node->Characteristics & IMAGE_SCN_MEM_READ)
            //    getNodeColor->BackColor = RGB(0xc0, 0xf0, 0xc0);

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

VOID PvDirectoryClearTree(
    _In_ PPV_DIRECTORY_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        PvDestroyDirectoryNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);
}

PPV_DIRECTORY_NODE PvGetSelectedDirectoryNode(
    _In_ PPV_DIRECTORY_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPV_DIRECTORY_NODE node = Context->NodeList->Items[i];

        if (node->Node.Selected)
            return node;
    }

    return NULL;
}

_Success_(return)
BOOLEAN PvGetSelectedDirectoryNodes(
    _In_ PPV_DIRECTORY_CONTEXT Context,
    _Out_ PPV_DIRECTORY_NODE **Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPV_DIRECTORY_NODE node = (PPV_DIRECTORY_NODE)Context->NodeList->Items[i];

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

VOID PvInitializeDirectoryTree(
    _In_ PPV_DIRECTORY_CONTEXT Context,
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle
    )
{
    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PPV_DIRECTORY_NODE),
        PvDirectoryNodeHashtableCompareFunction,
        PvDirectoryNodeHashtableHashFunction,
        100
        );
    Context->NodeList = PhCreateList(100);

    Context->ParentWindowHandle = ParentWindowHandle;
    Context->TreeNewHandle = TreeNewHandle;
    PhSetControlTheme(TreeNewHandle, L"explorer");

    TreeNew_SetCallback(TreeNewHandle, PvDirectoryTreeNewCallback, Context);
    TreeNew_SetRedraw(TreeNewHandle, FALSE);

    PhAddTreeNewColumnEx2(TreeNewHandle, PV_DIRECTORY_TREE_COLUMN_ITEM_INDEX, TRUE, L"#", 40, PH_ALIGN_LEFT, PV_DIRECTORY_TREE_COLUMN_ITEM_INDEX, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_DIRECTORY_TREE_COLUMN_ITEM_NAME, TRUE, L"Name", 130, PH_ALIGN_LEFT, PV_DIRECTORY_TREE_COLUMN_ITEM_NAME, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_DIRECTORY_TREE_COLUMN_ITEM_RVA_START, TRUE, L"RVA (start)", 100, PH_ALIGN_LEFT, PV_DIRECTORY_TREE_COLUMN_ITEM_RVA_START, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_DIRECTORY_TREE_COLUMN_ITEM_RVA_END, TRUE, L"RVA (end)", 100, PH_ALIGN_LEFT, PV_DIRECTORY_TREE_COLUMN_ITEM_RVA_END, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_DIRECTORY_TREE_COLUMN_ITEM_RVA_SIZE, TRUE, L"Size", 80, PH_ALIGN_LEFT, PV_DIRECTORY_TREE_COLUMN_ITEM_RVA_SIZE, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_DIRECTORY_TREE_COLUMN_ITEM_SECTION, TRUE, L"Section", 100, PH_ALIGN_LEFT, PV_DIRECTORY_TREE_COLUMN_ITEM_SECTION, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_DIRECTORY_TREE_COLUMN_ITEM_HASH, TRUE, L"Hash", 80, PH_ALIGN_LEFT, PV_DIRECTORY_TREE_COLUMN_ITEM_HASH, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_DIRECTORY_TREE_COLUMN_ITEM_ENTROPY, TRUE, L"Entropy", 80, PH_ALIGN_LEFT, PV_DIRECTORY_TREE_COLUMN_ITEM_ENTROPY, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_DIRECTORY_TREE_COLUMN_ITEM_SSDEEP, TRUE, L"SSDEEP", 80, PH_ALIGN_LEFT, PV_DIRECTORY_TREE_COLUMN_ITEM_SSDEEP, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_DIRECTORY_TREE_COLUMN_ITEM_TLSH, TRUE, L"TLSH", 80, PH_ALIGN_LEFT, PV_DIRECTORY_TREE_COLUMN_ITEM_TLSH, 0, 0);

    TreeNew_SetRowHeight(TreeNewHandle, 22);

    TreeNew_SetRedraw(TreeNewHandle, TRUE);
    TreeNew_SetSort(TreeNewHandle, PV_DIRECTORY_TREE_COLUMN_ITEM_INDEX, AscendingSortOrder);

    PhCmInitializeManager(&Context->Cm, TreeNewHandle, PV_DIRECTORY_TREE_COLUMN_ITEM_MAXIMUM, PvDirectoryTreeNewPostSortFunction);

    PhInitializeTreeNewFilterSupport(&Context->FilterSupport, TreeNewHandle, Context->NodeList);
}

BOOLEAN PvDirectoryWordMatchStringRef(
    _In_ PPV_DIRECTORY_CONTEXT Context,
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

BOOLEAN PvDirectoryWordMatchStringZ(
    _In_ PPV_DIRECTORY_CONTEXT Context,
    _In_ PWSTR Text
    )
{
    PH_STRINGREF text;

    PhInitializeStringRef(&text, Text);
    return PvDirectoryWordMatchStringRef(Context, &text);
}

BOOLEAN PvDirectoryTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPV_DIRECTORY_CONTEXT context = Context;
    PPV_DIRECTORY_NODE node = (PPV_DIRECTORY_NODE)Node;

    if (PhIsNullOrEmptyString(context->SearchboxText))
        return TRUE;

    if (!PhIsNullOrEmptyString(node->UniqueIdString))
    {
        if (PvDirectoryWordMatchStringRef(context, &node->UniqueIdString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->DirectoryNameString))
    {
        if (PvDirectoryWordMatchStringRef(context, &node->DirectoryNameString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->RvaStartString))
    {
        if (PvDirectoryWordMatchStringRef(context, &node->RvaStartString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->RvaEndString))
    {
        if (PvDirectoryWordMatchStringRef(context, &node->RvaEndString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->RvaSizeString))
    {
        if (PvDirectoryWordMatchStringRef(context, &node->RvaSizeString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->SectionNameString))
    {
        if (PvDirectoryWordMatchStringRef(context, &node->SectionNameString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->HashString))
    {
        if (PvDirectoryWordMatchStringRef(context, &node->HashString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->EntropyString))
    {
        if (PvDirectoryWordMatchStringRef(context, &node->EntropyString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->SsdeepString))
    {
        if (PvDirectoryWordMatchStringRef(context, &node->SsdeepString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->TlshString))
    {
        if (PvDirectoryWordMatchStringRef(context, &node->TlshString->sr))
            return TRUE;
    }

    return FALSE;
}
