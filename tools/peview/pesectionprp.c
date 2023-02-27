/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2019-2022
 *
 */

#include <peview.h>
#include "colmgr.h"

#include "..\thirdparty\ssdeep\fuzzy.h"
#include "..\thirdparty\tlsh\tlsh_wrapper.h"

static PH_STRINGREF EmptySectionsText = PH_STRINGREF_INIT(L"There are no sections to display.");
static PH_STRINGREF LoadingSectionsText = PH_STRINGREF_INIT(L"Loading sections from image...");

typedef enum _PV_SECTION_TREE_COLUMN_ITEM
{
    PV_SECTION_TREE_COLUMN_ITEM_INDEX,
    PV_SECTION_TREE_COLUMN_ITEM_NAME,
    PV_SECTION_TREE_COLUMN_ITEM_RAW_START,
    PV_SECTION_TREE_COLUMN_ITEM_RAW_END,
    PV_SECTION_TREE_COLUMN_ITEM_RAW_SIZE,
    PV_SECTION_TREE_COLUMN_ITEM_RVA_START,
    PV_SECTION_TREE_COLUMN_ITEM_RVA_END,
    PV_SECTION_TREE_COLUMN_ITEM_RVA_SIZE,
    PV_SECTION_TREE_COLUMN_ITEM_CHARACTERISTICS,
    PV_SECTION_TREE_COLUMN_ITEM_HASH,
    PV_SECTION_TREE_COLUMN_ITEM_ENTROPY,
    PV_SECTION_TREE_COLUMN_ITEM_SSDEEP,
    PV_SECTION_TREE_COLUMN_ITEM_TLSH,
    PV_SECTION_TREE_COLUMN_ITEM_MAXIMUM
} PV_SECTION_TREE_COLUMN_ITEM;

typedef struct _PV_SECTION_NODE
{
    PH_TREENEW_NODE Node;

    ULONG64 UniqueId;

    PVOID RawStart;
    PVOID RawEnd;
    ULONG RawSize;
    PVOID RvaStart;
    PVOID RvaEnd;
    ULONG RvaSize;
    ULONG Characteristics;
    DOUBLE SectionEntropy;
    PPH_STRING UniqueIdString;
    PPH_STRING SectionNameString;
    PPH_STRING RawStartString;
    PPH_STRING RawEndString;
    PPH_STRING RawSizeString;
    PPH_STRING RvaStartString;
    PPH_STRING RvaEndString;
    PPH_STRING RvaSizeString;
    PPH_STRING CharacteristicsString;
    PPH_STRING HashString;
    PPH_STRING EntropyString;
    PPH_STRING SsdeepString;
    PPH_STRING TlshString;

    PIMAGE_SECTION_HEADER SectionHeader;

    PH_STRINGREF TextCache[PV_SECTION_TREE_COLUMN_ITEM_MAXIMUM];
} PV_SECTION_NODE, *PPV_SECTION_NODE;

typedef struct _PV_SECTION_CONTEXT
{
    HWND DialogHandle;
    HWND SearchHandle;
    HWND TreeNewHandle;
    HWND ParentWindowHandle;

    PPH_STRING SearchboxText;
    PPH_STRING TreeText;

    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG Reserved : 1;
            ULONG HideWriteSection : 1;
            ULONG HideExecuteSection : 1;
            ULONG HideCodeSection : 1;
            ULONG HideReadSection : 1;
            ULONG HighlightWriteSection : 1;
            ULONG HighlightExecuteSection : 1;
            ULONG HighlightCodeSection : 1;
            ULONG HighlightReadSection : 1;
            ULONG Spare : 23;
        };
    };

    ULONG SearchResultsAddIndex;
    PPH_LIST SearchResults;
    PH_QUEUED_LOCK SearchResultsLock;

    PH_CM_MANAGER Cm;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PH_TN_FILTER_SUPPORT FilterSupport;
    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;
} PV_SECTION_CONTEXT, *PPV_SECTION_CONTEXT;

typedef enum _SECTION_TREE_MENU_ITEM
{
    SECTION_TREE_MENU_ITEM_HIDE_WRITE = 1,
    SECTION_TREE_MENU_ITEM_HIDE_EXECUTE,
    SECTION_TREE_MENU_ITEM_HIDE_CODE,
    SECTION_TREE_MENU_ITEM_HIDE_READ,
    SECTION_TREE_MENU_ITEM_HIGHLIGHT_WRITE,
    SECTION_TREE_MENU_ITEM_HIGHLIGHT_EXECUTE,
    SECTION_TREE_MENU_ITEM_HIGHLIGHT_CODE,
    SECTION_TREE_MENU_ITEM_HIGHLIGHT_READ,
    SECTION_TREE_MENU_ITEM_MAXIMUM
} SECTION_TREE_MENU_ITEM;

VOID PvSetOptionsSectionList(
    _Inout_ PPV_SECTION_CONTEXT Context,
    _In_ ULONG Options
    );

BOOLEAN PvSectionNodeHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

ULONG PvSectionNodeHashtableHashFunction(
    _In_ PVOID Entry
    );

VOID PvDestroySectionNode(
    _In_ PPV_SECTION_NODE Node
    );

VOID PvInitializeSectionTree(
    _In_ PPV_SECTION_CONTEXT Context,
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle
    );

VOID PvDeleteSectionTree(
    _In_ PPV_SECTION_CONTEXT Context
    );

VOID PvSectionAddTreeNode(
    _In_ PPV_SECTION_CONTEXT Context,
    _In_ PPV_SECTION_NODE Entry
    );

BOOLEAN PvSectionTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );

VOID PhLoadSettingsSectionList(
    _Inout_ PPV_SECTION_CONTEXT Context
    );

VOID PhSaveSettingsSectionList(
    _Inout_ PPV_SECTION_CONTEXT Context
    );

_Success_(return)
BOOLEAN PvGetSelectedSectionNodes(
    _In_ PPV_SECTION_CONTEXT Context,
    _Out_ PPV_SECTION_NODE **Nodes,
    _Out_ PULONG NumberOfNodes
    );

VOID PvAddPendingSectionNodes(
    _In_ PPV_SECTION_CONTEXT Context
    )
{
    ULONG i;
    BOOLEAN needsFullUpdate = FALSE;

    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    PhAcquireQueuedLockExclusive(&Context->SearchResultsLock);

    for (i = Context->SearchResultsAddIndex; i < Context->SearchResults->Count; i++)
    {
        PvSectionAddTreeNode(Context, Context->SearchResults->Items[i]);
        needsFullUpdate = TRUE;
    }
    Context->SearchResultsAddIndex = i;

    PhReleaseQueuedLockExclusive(&Context->SearchResultsLock);

    if (needsFullUpdate)
        TreeNew_NodesStructured(Context->TreeNewHandle);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);
}

PPH_STRING PvGetSectionCharacteristics(
    _In_ ULONG Characteristics
    )
{
    PH_STRING_BUILDER stringBuilder;
    WCHAR pointer[PH_PTR_STR_LEN_1];

    if (Characteristics == 0)
        return PhCreateString(L"Associative (0x0)");

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (Characteristics & IMAGE_SCN_TYPE_NO_PAD)
        PhAppendStringBuilder2(&stringBuilder, L"No Padding, ");
    if (Characteristics & IMAGE_SCN_CNT_CODE)
        PhAppendStringBuilder2(&stringBuilder, L"Code, ");
    if (Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
        PhAppendStringBuilder2(&stringBuilder, L"Initialized data, ");
    if (Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
        PhAppendStringBuilder2(&stringBuilder, L"Uninitialized data, ");
    if (Characteristics & IMAGE_SCN_LNK_INFO)
        PhAppendStringBuilder2(&stringBuilder, L"Comments, ");
    if (Characteristics & IMAGE_SCN_LNK_REMOVE)
        PhAppendStringBuilder2(&stringBuilder, L"Excluded, ");
    if (Characteristics & IMAGE_SCN_LNK_COMDAT)
        PhAppendStringBuilder2(&stringBuilder, L"COMDAT, ");
    if (Characteristics & IMAGE_SCN_NO_DEFER_SPEC_EXC)
        PhAppendStringBuilder2(&stringBuilder, L"Speculative exceptions, ");
    if (Characteristics & IMAGE_SCN_GPREL)
        PhAppendStringBuilder2(&stringBuilder, L"GP relative, ");
    if (Characteristics & IMAGE_SCN_MEM_PURGEABLE)
        PhAppendStringBuilder2(&stringBuilder, L"Purgeable, ");
    if (Characteristics & IMAGE_SCN_MEM_LOCKED)
        PhAppendStringBuilder2(&stringBuilder, L"Locked, ");
    if (Characteristics & IMAGE_SCN_MEM_PRELOAD)
        PhAppendStringBuilder2(&stringBuilder, L"Preload, ");
    if (Characteristics & IMAGE_SCN_LNK_NRELOC_OVFL)
        PhAppendStringBuilder2(&stringBuilder, L"Extended relocations, ");
    if (Characteristics & IMAGE_SCN_MEM_DISCARDABLE)
        PhAppendStringBuilder2(&stringBuilder, L"Discardable, ");
    if (Characteristics & IMAGE_SCN_MEM_NOT_CACHED)
        PhAppendStringBuilder2(&stringBuilder, L"Not cachable, ");
    if (Characteristics & IMAGE_SCN_MEM_NOT_PAGED)
        PhAppendStringBuilder2(&stringBuilder, L"Not pageable, ");
    if (Characteristics & IMAGE_SCN_MEM_SHARED)
        PhAppendStringBuilder2(&stringBuilder, L"Shareable, ");
    if (Characteristics & IMAGE_SCN_MEM_EXECUTE)
        PhAppendStringBuilder2(&stringBuilder, L"Executable, ");
    if (Characteristics & IMAGE_SCN_MEM_READ)
        PhAppendStringBuilder2(&stringBuilder, L"Readable, ");
    if (Characteristics & IMAGE_SCN_MEM_WRITE)
        PhAppendStringBuilder2(&stringBuilder, L"Writeable, ");

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhPrintPointer(pointer, UlongToPtr(Characteristics));
    PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);

    return PhFinalStringBuilderString(&stringBuilder);
}

NTSTATUS PvpPeSectionsEnumerateThread(
    _In_ PPV_SECTION_CONTEXT Context
    )
{
    for (ULONG i = 0; i < PvMappedImage.NumberOfSections; i++)
    {
        PPV_SECTION_NODE sectionNode;
        ULONG sectionNameLength = 0;
        WCHAR sectionName[IMAGE_SIZEOF_SHORT_NAME + 1];
        WCHAR value[PH_INT64_STR_LEN_1];

        sectionNode = PhAllocateZero(sizeof(PV_SECTION_NODE));
        sectionNode->UniqueId = UInt32Add32To64(i, 1);
        sectionNode->UniqueIdString = PhFormatUInt64(sectionNode->UniqueId, FALSE);
        sectionNode->SectionHeader = &PvMappedImage.Sections[i];

        if (PhGetMappedImageSectionName(
            &PvMappedImage.Sections[i],
            sectionName,
            RTL_NUMBER_OF(sectionName),
            &sectionNameLength
            ))
        {
            sectionNode->SectionNameString = PhCreateStringEx(sectionName, sectionNameLength * sizeof(WCHAR));
        }

        // RAW
        sectionNode->RawStart = UlongToPtr(PvMappedImage.Sections[i].PointerToRawData);
        PhPrintPointer(value, sectionNode->RawStart);
        sectionNode->RawStartString = PhCreateString(value);
        sectionNode->RawEnd = PTR_ADD_OFFSET(PvMappedImage.Sections[i].PointerToRawData, PvMappedImage.Sections[i].SizeOfRawData);
        PhPrintPointer(value, sectionNode->RawEnd);
        sectionNode->RawEndString = PhCreateString(value);
        sectionNode->RawSize = PvMappedImage.Sections[i].SizeOfRawData;
        sectionNode->RawSizeString = PhFormatSize(sectionNode->RawSize, ULONG_MAX);
        // RVA
        sectionNode->RvaStart = UlongToPtr(PvMappedImage.Sections[i].VirtualAddress);
        PhPrintPointer(value, sectionNode->RvaStart);
        sectionNode->RvaStartString = PhCreateString(value);
        sectionNode->RvaEnd = PTR_ADD_OFFSET(PvMappedImage.Sections[i].VirtualAddress, PvMappedImage.Sections[i].Misc.VirtualSize);
        PhPrintPointer(value, sectionNode->RvaEnd);
        sectionNode->RvaEndString = PhCreateString(value);
        sectionNode->RvaSize = PvMappedImage.Sections[i].Misc.VirtualSize;
        sectionNode->RvaSizeString = PhFormatSize(sectionNode->RvaSize, ULONG_MAX);
        // SECTION
        sectionNode->Characteristics = PvMappedImage.Sections[i].Characteristics;
        sectionNode->CharacteristicsString = PvGetSectionCharacteristics(sectionNode->Characteristics);

        if (PvMappedImage.Sections[i].VirtualAddress && PvMappedImage.Sections[i].SizeOfRawData)
        {
            PVOID imageSectionData;

            if (imageSectionData = PhMappedImageRvaToVa(&PvMappedImage, PvMappedImage.Sections[i].VirtualAddress, NULL))
            {
                sectionNode->HashString = PvHashBuffer(imageSectionData, PvMappedImage.Sections[i].SizeOfRawData);
            }

            __try
            {
                PVOID imageSectionData;
                DOUBLE imageSectionEntropy;

                if (imageSectionData = PhMappedImageRvaToVa(&PvMappedImage, PvMappedImage.Sections[i].VirtualAddress, NULL))
                {
                    if (PhCalculateEntropy(
                        imageSectionData,
                        PvMappedImage.Sections[i].SizeOfRawData,
                        &imageSectionEntropy,
                        NULL
                        ))
                    {
                        sectionNode->SectionEntropy = imageSectionEntropy;
                        sectionNode->EntropyString = PhFormatEntropy(imageSectionEntropy, 2, 0, 0);
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                //sectionNode->EntropyString = PhGetNtMessage(GetExceptionCode());
                sectionNode->EntropyString = PhGetWin32Message(PhNtStatusToDosError(GetExceptionCode())); // WIN32_FROM_NTSTATUS
            }

            __try
            {
                PVOID imageSectionData;
                PPH_STRING ssdeepHashString = NULL;

                if (imageSectionData = PhMappedImageRvaToVa(&PvMappedImage, PvMappedImage.Sections[i].VirtualAddress, NULL))
                {
                    fuzzy_hash_buffer(
                        imageSectionData,
                        PvMappedImage.Sections[i].SizeOfRawData,
                        &ssdeepHashString
                        );

                    if (ssdeepHashString)
                    {
                        sectionNode->SsdeepString = ssdeepHashString;
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                //sectionNode->SsdeepString = PhGetNtMessage(GetExceptionCode());
                sectionNode->SsdeepString = PhGetWin32Message(PhNtStatusToDosError(GetExceptionCode())); // WIN32_FROM_NTSTATUS
            }

            __try
            {
                PVOID imageSectionData;
                PPH_STRING tlshHashString = NULL;

                if (imageSectionData = PhMappedImageRvaToVa(&PvMappedImage, PvMappedImage.Sections[i].VirtualAddress, NULL))
                {
                    //
                    // This can fail in TLSH library during finalization when
                    // "buckets must be more than 50% non-zero" (see: tlsh_impl.cpp)
                    //
                    PvGetTlshBufferHash(
                        imageSectionData,
                        PvMappedImage.Sections[i].SizeOfRawData,
                        &tlshHashString
                        );

                    if (!PhIsNullOrEmptyString(tlshHashString))
                    {
                        sectionNode->TlshString = tlshHashString;
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                //sectionNode->TlshString = PhGetNtMessage(GetExceptionCode());
                sectionNode->TlshString = PhGetWin32Message(PhNtStatusToDosError(GetExceptionCode())); // WIN32_FROM_NTSTATUS
            }
        }

        PhAcquireQueuedLockExclusive(&Context->SearchResultsLock);
        PhAddItemList(Context->SearchResults, sectionNode);
        PhReleaseQueuedLockExclusive(&Context->SearchResultsLock);
    }

    PostMessage(Context->DialogHandle, WM_PV_SEARCH_FINISHED, 0, 0);
    return STATUS_SUCCESS;
}

INT_PTR CALLBACK PvPeSectionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_SECTION_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PV_SECTION_CONTEXT));
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

            PvCreateSearchControl(context->SearchHandle, L"Search Sections (Ctrl+K)");

            PvInitializeSectionTree(context, hwndDlg, context->TreeNewHandle);
            PhAddTreeNewFilter(&context->FilterSupport, PvSectionTreeFilterCallback, context);
            PhLoadSettingsSectionList(context);
            PvConfigTreeBorders(context->TreeNewHandle);

            TreeNew_SetEmptyText(context->TreeNewHandle, &LoadingSectionsText, 0);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->SearchHandle, NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);

            PhCreateThread2(PvpPeSectionsEnumerateThread, context);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveSettingsSectionList(context);
            PvDeleteSectionTree(context);
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

            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_SETTINGS:
                {
                    RECT rect;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM writableMenuItem;
                    PPH_EMENU_ITEM executableMenuItem;
                    PPH_EMENU_ITEM codeMenuItem;
                    PPH_EMENU_ITEM readMenuItem;
                    PPH_EMENU_ITEM highlightWriteMenuItem;
                    PPH_EMENU_ITEM highlightExecuteMenuItem;
                    PPH_EMENU_ITEM highlightCodeMenuItem;
                    PPH_EMENU_ITEM highlightReadMenuItem;
                    PPH_EMENU_ITEM selectedItem;

                    GetWindowRect(GetDlgItem(hwndDlg, IDC_SETTINGS), &rect);

                    writableMenuItem = PhCreateEMenuItem(0, SECTION_TREE_MENU_ITEM_HIDE_WRITE, L"Hide writable", NULL, NULL);
                    executableMenuItem = PhCreateEMenuItem(0, SECTION_TREE_MENU_ITEM_HIDE_EXECUTE, L"Hide executable", NULL, NULL);
                    codeMenuItem = PhCreateEMenuItem(0, SECTION_TREE_MENU_ITEM_HIDE_CODE, L"Hide code", NULL, NULL);
                    readMenuItem = PhCreateEMenuItem(0, SECTION_TREE_MENU_ITEM_HIDE_READ, L"Hide readable", NULL, NULL);
                    highlightWriteMenuItem = PhCreateEMenuItem(0, SECTION_TREE_MENU_ITEM_HIGHLIGHT_WRITE, L"Highlight writable", NULL, NULL);
                    highlightExecuteMenuItem = PhCreateEMenuItem(0, SECTION_TREE_MENU_ITEM_HIGHLIGHT_EXECUTE, L"Highlight executable", NULL, NULL);
                    highlightCodeMenuItem = PhCreateEMenuItem(0, SECTION_TREE_MENU_ITEM_HIGHLIGHT_CODE, L"Highlight code", NULL, NULL);
                    highlightReadMenuItem = PhCreateEMenuItem(0, SECTION_TREE_MENU_ITEM_HIGHLIGHT_READ, L"Highlight readable", NULL, NULL);

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, writableMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, executableMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, codeMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, readMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, highlightWriteMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, highlightExecuteMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, highlightCodeMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, highlightReadMenuItem, ULONG_MAX);

                    if (context->HideWriteSection)
                        writableMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HideExecuteSection)
                        executableMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HideCodeSection)
                        codeMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HideReadSection)
                        readMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HighlightWriteSection)
                        highlightWriteMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HighlightExecuteSection)
                        highlightExecuteMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HighlightCodeSection)
                        highlightCodeMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HighlightReadSection)
                        highlightReadMenuItem->Flags |= PH_EMENU_CHECKED;

                    selectedItem = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        rect.left,
                        rect.bottom
                        );

                    if (selectedItem && selectedItem->Id)
                    {
                        PvSetOptionsSectionList(context, selectedItem->Id);
                        PhSaveSettingsSectionList(context);
                        PhApplyTreeNewFilters(&context->FilterSupport);
                    }

                    PhDestroyEMenu(menu);
                }
                break;
            }
        }
        break;
    case WM_PV_SEARCH_FINISHED:
        {
            PvAddPendingSectionNodes(context);

            TreeNew_SetEmptyText(context->TreeNewHandle, &EmptySectionsText, 0);

            TreeNew_NodesStructured(context->TreeNewHandle);
        }
        break;
    case WM_PV_SEARCH_SHOWMENU:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
            PPH_EMENU menu;
            PPH_EMENU_ITEM selectedItem;
            PPV_SECTION_NODE* sectionNodes = NULL;
            ULONG numberOfNodes = 0;

            if (!PvGetSelectedSectionNodes(context, &sectionNodes, &numberOfNodes))
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

VOID PhLoadSettingsSectionList(
    _Inout_ PPV_SECTION_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhGetStringSetting(L"ImageSectionsTreeListColumns");
    sortSettings = PhGetStringSetting(L"ImageSectionsTreeListSort");
    Context->Flags = PhGetIntegerSetting(L"ImageSectionsTreeListFlags");

    PhCmLoadSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &settings->sr, &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PhSaveSettingsSectionList(
    _Inout_ PPV_SECTION_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhCmSaveSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &sortSettings);

    PhSetIntegerSetting(L"ImageSectionsTreeListFlags", Context->Flags);
    PhSetStringSetting2(L"ImageSectionsTreeListColumns", &settings->sr);
    PhSetStringSetting2(L"ImageSectionsTreeListSort", &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PvDeleteSectionTree(
    _In_ PPV_SECTION_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PvDestroySectionNode(Context->NodeList->Items[i]);
    }

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}

struct _PH_TN_FILTER_SUPPORT* GetSectionListFilterSupport(
    _In_ PPV_SECTION_CONTEXT Context
    )
{
    return &Context->FilterSupport;
}

LONG PvSectionTreeNewPostSortFunction(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    )
{
    if (Result == 0)
        Result = uintptrcmp((ULONG_PTR)((PPV_SECTION_NODE)Node1)->UniqueId, (ULONG_PTR)((PPV_SECTION_NODE)Node2)->UniqueId);

    return PhModifySort(Result, SortOrder);
}

BOOLEAN PvSectionNodeHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPV_SECTION_NODE sectionNode1 = *(PPV_SECTION_NODE*)Entry1;
    PPV_SECTION_NODE sectionNode2 = *(PPV_SECTION_NODE*)Entry2;

    return sectionNode1->UniqueId == sectionNode2->UniqueId;
}

ULONG PvSectionNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashInt64((ULONG_PTR)(*(PPV_SECTION_NODE*)Entry)->UniqueId);
}

VOID PvSectionAddTreeNode(
    _In_ PPV_SECTION_CONTEXT Context,
    _In_ PPV_SECTION_NODE Entry
    )
{
    PhInitializeTreeNewNode(&Entry->Node);

    memset(Entry->TextCache, 0, sizeof(PH_STRINGREF) * PV_SECTION_TREE_COLUMN_ITEM_MAXIMUM);
    Entry->Node.TextCache = Entry->TextCache;
    Entry->Node.TextCacheSize = PV_SECTION_TREE_COLUMN_ITEM_MAXIMUM;

    if (PhAddEntryHashtable(Context->NodeHashtable, &Entry)) // HACK
    {
        PhAddItemList(Context->NodeList, Entry);

        if (Context->FilterSupport.NodeList)
        {
            Entry->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->FilterSupport, &Entry->Node);
        }
    }
}

PPV_SECTION_NODE PvFindSectionNode(
    _In_ PPV_SECTION_CONTEXT Context,
    _In_ PPH_STRING Name
    )
{
    PV_SECTION_NODE lookupSymbolNode;
    PPV_SECTION_NODE lookupSymbolNodePtr = &lookupSymbolNode;
    PPV_SECTION_NODE* threadNode;

    lookupSymbolNode.SectionNameString = Name;

    threadNode = (PPV_SECTION_NODE*)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupSymbolNodePtr
        );

    if (threadNode)
        return *threadNode;
    else
        return NULL;
}

VOID PvRemoveSectionNode(
    _In_ PPV_SECTION_CONTEXT Context,
    _In_ PPV_SECTION_NODE Node
    )
{
    ULONG index;

    PhRemoveEntryHashtable(Context->NodeHashtable, &Node);

    if ((index = PhFindItemList(Context->NodeList, Node)) != ULONG_MAX)
        PhRemoveItemList(Context->NodeList, index);

    PvDestroySectionNode(Node);
}

VOID PvDestroySectionNode(
    _In_ PPV_SECTION_NODE Node
    )
{
    //if (Node->UniqueIdString)
    //    PhDereferenceObject(Node->UniqueIdString);
    //if (Node->SectionNameString)
    //    PhDereferenceObject(Node->SectionNameString);
    //if (Node->RawStartString)
    //    PhDereferenceObject(Node->RawStartString);
    //if (Node->RawEndString)
    //    PhDereferenceObject(Node->RawEndString);
    //if (Node->RawSizeString)
    //    PhDereferenceObject(Node->RawSizeString);
    //if (Node->RvaStartString)
    //    PhDereferenceObject(Node->RvaStartString);
    //if (Node->RvaEndString)
    //    PhDereferenceObject(Node->RvaEndString);
    //if (Node->RvaSizeString)
    //    PhDereferenceObject(Node->RvaSizeString);
    //if (Node->CharacteristicsString)
    //    PhDereferenceObject(Node->CharacteristicsString);
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

VOID PvSetOptionsSectionList(
    _Inout_ PPV_SECTION_CONTEXT Context,
    _In_ ULONG Options
    )
{
    switch (Options)
    {
    case SECTION_TREE_MENU_ITEM_HIDE_WRITE:
        Context->HideWriteSection = !Context->HideWriteSection;
        break;
    case SECTION_TREE_MENU_ITEM_HIDE_EXECUTE:
        Context->HideExecuteSection = !Context->HideExecuteSection;
        break;
    case SECTION_TREE_MENU_ITEM_HIDE_CODE:
        Context->HideCodeSection = !Context->HideCodeSection;
        break;
    case SECTION_TREE_MENU_ITEM_HIDE_READ:
        Context->HideReadSection = !Context->HideReadSection;
        break;
    case SECTION_TREE_MENU_ITEM_HIGHLIGHT_WRITE:
        Context->HighlightWriteSection = !Context->HighlightWriteSection;
        break;
    case SECTION_TREE_MENU_ITEM_HIGHLIGHT_EXECUTE:
        Context->HighlightExecuteSection = !Context->HighlightExecuteSection;
        break;
    case SECTION_TREE_MENU_ITEM_HIGHLIGHT_CODE:
        Context->HighlightCodeSection = !Context->HighlightCodeSection;
        break;
    case SECTION_TREE_MENU_ITEM_HIGHLIGHT_READ:
        Context->HighlightReadSection = !Context->HighlightReadSection;
        break;
    }
}

#define SORT_FUNCTION(Column) PvSectionTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PvSectionTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPV_SECTION_NODE node1 = *(PPV_SECTION_NODE *)_elem1; \
    PPV_SECTION_NODE node2 = *(PPV_SECTION_NODE *)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)node1->UniqueId, (ULONG_PTR)node2->UniqueId); \
    \
    return PhModifySort(sortResult, ((PPV_SECTION_CONTEXT)_context)->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Index)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->UniqueId, (ULONG_PTR)node2->UniqueId);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareStringWithNull(node1->SectionNameString, node2->SectionNameString, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(RawStart)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->RawStart, (ULONG_PTR)node2->RawStart);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(RawEnd)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->RawEnd, (ULONG_PTR)node2->RawEnd);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(RawSize)
{
    sortResult = uintcmp(node1->RawSize, node2->RawSize);
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

BEGIN_SORT_FUNCTION(Characteristics)
{
    sortResult = uintcmp(node1->Characteristics, node2->Characteristics);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Hash)
{
    sortResult = PhCompareStringWithNull(node1->HashString, node2->HashString, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Entropy)
{
    sortResult = doublecmp(node1->SectionEntropy, node2->SectionEntropy);
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

BOOLEAN NTAPI PvSectionTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PPV_SECTION_CONTEXT context = Context;
    PPV_SECTION_NODE node;

    if (!context)
        return FALSE;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            if (!getChildren)
                break;

            node = (PPV_SECTION_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Index),
                    SORT_FUNCTION(Name),
                    SORT_FUNCTION(RawStart),
                    SORT_FUNCTION(RawEnd),
                    SORT_FUNCTION(RawSize),
                    SORT_FUNCTION(RvaStart),
                    SORT_FUNCTION(RvaEnd),
                    SORT_FUNCTION(RvaSize),
                    SORT_FUNCTION(Characteristics),
                    SORT_FUNCTION(Hash),
                    SORT_FUNCTION(Entropy),
                    SORT_FUNCTION(Ssdeep),
                    SORT_FUNCTION(Tlsh),
                };
                int (__cdecl *sortFunction)(void *, const void *, const void *);

                if (context->TreeNewSortColumn < PV_SECTION_TREE_COLUMN_ITEM_MAXIMUM)
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

            node = (PPV_SECTION_NODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;

            if (!getCellText)
                break;

            node = (PPV_SECTION_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PV_SECTION_TREE_COLUMN_ITEM_INDEX:
                getCellText->Text = PhGetStringRef(node->UniqueIdString);
                break;
            case PV_SECTION_TREE_COLUMN_ITEM_NAME:
                getCellText->Text = PhGetStringRef(node->SectionNameString);
                break;
            case PV_SECTION_TREE_COLUMN_ITEM_RAW_START:
                getCellText->Text = PhGetStringRef(node->RawStartString);
                break;
            case PV_SECTION_TREE_COLUMN_ITEM_RAW_END:
                getCellText->Text = PhGetStringRef(node->RawEndString);
                break;
            case PV_SECTION_TREE_COLUMN_ITEM_RAW_SIZE:
                getCellText->Text = PhGetStringRef(node->RawSizeString);
                break;
            case PV_SECTION_TREE_COLUMN_ITEM_RVA_START:
                getCellText->Text = PhGetStringRef(node->RvaStartString);
                break;
            case PV_SECTION_TREE_COLUMN_ITEM_RVA_END:
                getCellText->Text = PhGetStringRef(node->RvaEndString);
                break;
            case PV_SECTION_TREE_COLUMN_ITEM_RVA_SIZE:
                getCellText->Text = PhGetStringRef(node->RvaSizeString);
                break;
            case PV_SECTION_TREE_COLUMN_ITEM_CHARACTERISTICS:
                getCellText->Text = PhGetStringRef(node->CharacteristicsString);
                break;
            case PV_SECTION_TREE_COLUMN_ITEM_HASH:
                getCellText->Text = PhGetStringRef(node->HashString);
                break;
            case PV_SECTION_TREE_COLUMN_ITEM_ENTROPY:
                getCellText->Text = PhGetStringRef(node->EntropyString);
                break;
            case PV_SECTION_TREE_COLUMN_ITEM_SSDEEP:
                getCellText->Text = PhGetStringRef(node->SsdeepString);
                break;
            case PV_SECTION_TREE_COLUMN_ITEM_TLSH:
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

            node = (PPV_SECTION_NODE)getNodeColor->Node;

            if (!node)
                NOTHING; // Dummy
            else if (context->HighlightWriteSection && node->Characteristics & IMAGE_SCN_MEM_WRITE)
                getNodeColor->BackColor = RGB(0xf0, 0xa0, 0xa0);
            else if (context->HighlightExecuteSection && node->Characteristics & IMAGE_SCN_MEM_EXECUTE)
                getNodeColor->BackColor = RGB(0xff, 0x93, 0x14);
            else if (context->HighlightCodeSection && node->Characteristics & IMAGE_SCN_CNT_CODE)
                getNodeColor->BackColor = RGB(0xe0, 0xf0, 0xe0);
            else if (context->HighlightReadSection && node->Characteristics & IMAGE_SCN_MEM_READ)
                getNodeColor->BackColor = RGB(0xc0, 0xf0, 0xc0);

            getNodeColor->Flags = TN_AUTO_FORECOLOR;
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

VOID PvSectionClearTree(
    _In_ PPV_SECTION_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        PvDestroySectionNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);
}

PPV_SECTION_NODE PvGetSelectedSectionNode(
    _In_ PPV_SECTION_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPV_SECTION_NODE node = Context->NodeList->Items[i];

        if (node->Node.Selected)
            return node;
    }

    return NULL;
}

_Success_(return)
BOOLEAN PvGetSelectedSectionNodes(
    _In_ PPV_SECTION_CONTEXT Context,
    _Out_ PPV_SECTION_NODE **Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPV_SECTION_NODE node = (PPV_SECTION_NODE)Context->NodeList->Items[i];

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

VOID PvInitializeSectionTree(
    _In_ PPV_SECTION_CONTEXT Context,
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle
    )
{
    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PPV_SECTION_NODE),
        PvSectionNodeHashtableCompareFunction,
        PvSectionNodeHashtableHashFunction,
        100
        );
    Context->NodeList = PhCreateList(100);

    Context->ParentWindowHandle = ParentWindowHandle;
    Context->TreeNewHandle = TreeNewHandle;
    PhSetControlTheme(TreeNewHandle, L"explorer");

    TreeNew_SetCallback(TreeNewHandle, PvSectionTreeNewCallback, Context);
    TreeNew_SetRedraw(TreeNewHandle, FALSE);

    PhAddTreeNewColumnEx2(TreeNewHandle, PV_SECTION_TREE_COLUMN_ITEM_INDEX, TRUE, L"#", 40, PH_ALIGN_LEFT, PV_SECTION_TREE_COLUMN_ITEM_INDEX, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_SECTION_TREE_COLUMN_ITEM_NAME, TRUE, L"Name", 80, PH_ALIGN_LEFT, PV_SECTION_TREE_COLUMN_ITEM_NAME, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_SECTION_TREE_COLUMN_ITEM_RAW_START, TRUE, L"RAW (start)", 100, PH_ALIGN_LEFT, PV_SECTION_TREE_COLUMN_ITEM_RAW_START, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_SECTION_TREE_COLUMN_ITEM_RAW_END, TRUE, L"RAW (end)", 100, PH_ALIGN_LEFT, PV_SECTION_TREE_COLUMN_ITEM_RAW_END, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_SECTION_TREE_COLUMN_ITEM_RAW_SIZE, TRUE, L"RAW (size)", 80, PH_ALIGN_LEFT, PV_SECTION_TREE_COLUMN_ITEM_RAW_SIZE, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_SECTION_TREE_COLUMN_ITEM_RVA_START, TRUE, L"RVA (start)", 100, PH_ALIGN_LEFT, PV_SECTION_TREE_COLUMN_ITEM_RVA_START, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_SECTION_TREE_COLUMN_ITEM_RVA_END, TRUE, L"RVA (end)", 100, PH_ALIGN_LEFT, PV_SECTION_TREE_COLUMN_ITEM_RVA_END, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_SECTION_TREE_COLUMN_ITEM_RVA_SIZE, TRUE, L"RVA (size)", 80, PH_ALIGN_LEFT, PV_SECTION_TREE_COLUMN_ITEM_RVA_SIZE, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_SECTION_TREE_COLUMN_ITEM_CHARACTERISTICS, TRUE, L"Characteristics", 250, PH_ALIGN_LEFT, PV_SECTION_TREE_COLUMN_ITEM_CHARACTERISTICS, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_SECTION_TREE_COLUMN_ITEM_HASH, TRUE, L"Hash", 80, PH_ALIGN_LEFT, PV_SECTION_TREE_COLUMN_ITEM_HASH, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_SECTION_TREE_COLUMN_ITEM_ENTROPY, TRUE, L"Entropy", 80, PH_ALIGN_LEFT, PV_SECTION_TREE_COLUMN_ITEM_ENTROPY, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_SECTION_TREE_COLUMN_ITEM_SSDEEP, TRUE, L"SSDEEP", 80, PH_ALIGN_LEFT, PV_SECTION_TREE_COLUMN_ITEM_SSDEEP, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_SECTION_TREE_COLUMN_ITEM_TLSH, TRUE, L"TLSH", 80, PH_ALIGN_LEFT, PV_SECTION_TREE_COLUMN_ITEM_TLSH, 0, 0);

    TreeNew_SetRowHeight(TreeNewHandle, 22);

    TreeNew_SetRedraw(TreeNewHandle, TRUE);
    TreeNew_SetSort(TreeNewHandle, PV_SECTION_TREE_COLUMN_ITEM_INDEX, AscendingSortOrder);

    PhCmInitializeManager(&Context->Cm, TreeNewHandle, PV_SECTION_TREE_COLUMN_ITEM_MAXIMUM, PvSectionTreeNewPostSortFunction);

    PhInitializeTreeNewFilterSupport(&Context->FilterSupport, TreeNewHandle, Context->NodeList);
}

BOOLEAN PvSectionWordMatchStringRef(
    _In_ PPV_SECTION_CONTEXT Context,
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

BOOLEAN PvSectionWordMatchStringZ(
    _In_ PPV_SECTION_CONTEXT Context,
    _In_ PWSTR Text
    )
{
    PH_STRINGREF text;

    PhInitializeStringRef(&text, Text);
    return PvSectionWordMatchStringRef(Context, &text);
}

BOOLEAN PvSectionTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPV_SECTION_CONTEXT context = Context;
    PPV_SECTION_NODE node = (PPV_SECTION_NODE)Node;

    if (context->HideWriteSection && node->Characteristics & IMAGE_SCN_MEM_WRITE)
        return FALSE;
    if (context->HideExecuteSection && node->Characteristics & IMAGE_SCN_MEM_EXECUTE)
        return FALSE;
    if (context->HideCodeSection && node->Characteristics & IMAGE_SCN_CNT_CODE)
        return FALSE;
    if (context->HideReadSection && node->Characteristics & IMAGE_SCN_MEM_READ)
        return FALSE;

    if (PhIsNullOrEmptyString(context->SearchboxText))
        return TRUE;

    if (!PhIsNullOrEmptyString(node->UniqueIdString))
    {
        if (PvSectionWordMatchStringRef(context, &node->UniqueIdString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->SectionNameString))
    {
        if (PvSectionWordMatchStringRef(context, &node->SectionNameString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->RawStartString))
    {
        if (PvSectionWordMatchStringRef(context, &node->RawStartString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->RawEndString))
    {
        if (PvSectionWordMatchStringRef(context, &node->RawEndString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->RawSizeString))
    {
        if (PvSectionWordMatchStringRef(context, &node->RawSizeString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->RvaStartString))
    {
        if (PvSectionWordMatchStringRef(context, &node->RvaStartString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->RvaEndString))
    {
        if (PvSectionWordMatchStringRef(context, &node->RvaEndString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->RvaSizeString))
    {
        if (PvSectionWordMatchStringRef(context, &node->RvaSizeString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->CharacteristicsString))
    {
        if (PvSectionWordMatchStringRef(context, &node->CharacteristicsString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->HashString))
    {
        if (PvSectionWordMatchStringRef(context, &node->HashString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->EntropyString))
    {
        if (PvSectionWordMatchStringRef(context, &node->EntropyString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->SsdeepString))
    {
        if (PvSectionWordMatchStringRef(context, &node->SsdeepString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->TlshString))
    {
        if (PvSectionWordMatchStringRef(context, &node->TlshString->sr))
            return TRUE;
    }

    return FALSE;
}
