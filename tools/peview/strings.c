/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2024
 *
 */

#include <peview.h>
#include <strsrch.h>

static PH_STRINGREF EmptyStringsText = PH_STRINGREF_INIT(L"There are no strings to display.");
static PH_STRINGREF LoadingStringsText = PH_STRINGREF_INIT(L"Loading strings from image...");

typedef struct _PV_STRINGS_SETTINGS
{
    ULONG MinimumLength;

    union
    {
        struct
        {
            ULONG Ansi : 1;
            ULONG Unicode : 1;
            ULONG ExtendedCharSet : 1;
            ULONG Spare : 29;
        };

        ULONG Flags;
    };
} PV_STRINGS_SETTINGS, *PPV_STRINGS_SETTINGS;

typedef struct _PV_STRINGS_CONTEXT
{
    HWND WindowHandle;
    HWND TreeNewHandle;
    HWND SearchHandle;
    ULONG_PTR SearchMatchHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
    PH_TN_FILTER_SUPPORT FilterSupport;
    PH_CM_MANAGER Cm;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    ULONG StringsCount;
    PPH_LIST NodeList;

    BOOLEAN StopSearch;
    HANDLE SearchThreadHandle;
    ULONG SearchResultsAddIndex;
    PPH_LIST SearchResults;
    PH_QUEUED_LOCK SearchResultsLock;

    PV_STRINGS_SETTINGS Settings;
} PV_STRINGS_CONTEXT, *PPV_STRINGS_CONTEXT;

typedef enum _PV_STRINGS_TREE_COLUMN_ITEM
{
    PV_STRINGS_TREE_COLUMN_ITEM_INDEX,
    PV_STRINGS_TREE_COLUMN_ITEM_SECTION,
    PV_STRINGS_TREE_COLUMN_ITEM_RVA,
    PV_STRINGS_TREE_COLUMN_ITEM_TYPE,
    PV_STRINGS_TREE_COLUMN_ITEM_LENGTH,
    PV_STRINGS_TREE_COLUMN_ITEM_STRING,
    PV_STRINGS_TREE_COLUMN_ITEM_MAXIMUM
} PV_IMPORT_TREE_COLUMN_ITEM;

typedef struct _PV_STRINGS_NODE
{
    PH_TREENEW_NODE Node;

    ULONG Index;
    BOOLEAN Unicode;
    ULONG_PTR Rva;
    PPH_STRING String;

    WCHAR IndexString[PH_INT64_STR_LEN_1];
    WCHAR RvaString[PH_PTR_STR_LEN_1];
    WCHAR LengthString[PH_INT64_STR_LEN_1];
    WCHAR SectionName[IMAGE_SIZEOF_SHORT_NAME + 1];

    PH_STRINGREF TextCache[PV_STRINGS_TREE_COLUMN_ITEM_MAXIMUM];
} PV_STRINGS_NODE, *PPV_STRINGS_NODE;

_Function_class_(PH_STRING_SEARCH_NEXT_BUFFER)
_Must_inspect_result_
NTSTATUS NTAPI PvpStringSearchNextBuffer(
    _Inout_bytecount_(*Length) PVOID* Buffer,
    _Out_ PSIZE_T Length,
    _In_opt_ PVOID Context
    )
{
    UNREFERENCED_PARAMETER(Context);

    if (*Buffer)
    {
        *Buffer = NULL;
        *Length = 0;
    }
    else
    {
        *Buffer = PvMappedImage.ViewBase;
        *Length = PvMappedImage.ViewSize;
    }

    return STATUS_SUCCESS;
}

_Function_class_(PH_STRING_SEARCH_CALLBACK)
BOOLEAN NTAPI PvpStringSearchCallback(
    _In_ PPH_STRING_SEARCH_RESULT Result,
    _In_opt_ PVOID Context
    )
{
    PPV_STRINGS_CONTEXT context = Context;
    PPV_STRINGS_NODE node;
    PIMAGE_SECTION_HEADER section;

    assert(Context);

    node = PhAllocateZero(sizeof(PV_STRINGS_NODE));

    node->Index = ++context->StringsCount;
    PhPrintUInt64(node->IndexString, node->Index);

    node->Rva = (ULONG_PTR)PTR_SUB_OFFSET(Result->Address, PvMappedImage.ViewBase);
    PhPrintPointer(node->RvaString, (PVOID)node->Rva);
    node->Unicode = Result->Unicode;
    node->String = PhReferenceObject(Result->String);
    PhPrintUInt64(node->LengthString, node->String->Length / 2);

    PhAcquireQueuedLockExclusive(&context->SearchResultsLock);
    PhAddItemList(context->SearchResults, node);
    PhReleaseQueuedLockExclusive(&context->SearchResultsLock);

    if (section = PhMappedImageRvaToSection(&PvMappedImage, (ULONG)node->Rva))
    {
        for (ULONG i = 0; i < IMAGE_SIZEOF_SHORT_NAME; i++)
        {
            node->SectionName[i] = section->Name[i];
        }
    }

    return !!context->StopSearch;
}

NTSTATUS PvpSearchStringsThread(
    _In_ PPV_STRINGS_CONTEXT Context
    )
{
    PhSearchStrings(
        Context->Settings.MinimumLength,
        !!Context->Settings.ExtendedCharSet,
        PvpStringSearchNextBuffer,
        PvpStringSearchCallback,
        Context
        );

    PostMessage(Context->WindowHandle, WM_PV_SEARCH_FINISHED, 0, 0);

    return STATUS_SUCCESS;
}

VOID PvpStringsAddTreeNode(
    _In_ PPV_STRINGS_CONTEXT Context,
    _In_ PPV_STRINGS_NODE Entry
    )
{
    PhInitializeTreeNewNode(&Entry->Node);

    memset(Entry->TextCache, 0, sizeof(PH_STRINGREF) * PV_STRINGS_TREE_COLUMN_ITEM_MAXIMUM);
    Entry->Node.TextCache = Entry->TextCache;
    Entry->Node.TextCacheSize = PV_STRINGS_TREE_COLUMN_ITEM_MAXIMUM;

    PhAddItemList(Context->NodeList, Entry);

    if (Context->FilterSupport.NodeList)
    {
        Entry->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->FilterSupport, &Entry->Node);
    }
}

VOID PvpAddPendingStringsNodes(
    _In_ PPV_STRINGS_CONTEXT Context
    )
{
    ULONG i;
    BOOLEAN needsFullUpdate = FALSE;

    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    PhAcquireQueuedLockExclusive(&Context->SearchResultsLock);

    for (i = Context->SearchResultsAddIndex; i < Context->SearchResults->Count; i++)
    {
        PvpStringsAddTreeNode(Context, Context->SearchResults->Items[i]);
        needsFullUpdate = TRUE;
    }
    Context->SearchResultsAddIndex = i;

    PhReleaseQueuedLockExclusive(&Context->SearchResultsLock);

    if (needsFullUpdate)
        TreeNew_NodesStructured(Context->TreeNewHandle);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);
}

VOID PvpDeleteStringsTree(
    _In_ PPV_STRINGS_CONTEXT Context
    )
{
    Context->StopSearch = TRUE;
    if (Context->SearchThreadHandle)
    {
        NtWaitForSingleObject(Context->SearchThreadHandle, FALSE, NULL);
        NtClose(Context->SearchThreadHandle);
        Context->SearchThreadHandle = NULL;
    }
    Context->StopSearch = FALSE;

    PvpAddPendingStringsNodes(Context);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPV_STRINGS_NODE node = (PPV_STRINGS_NODE)Context->NodeList->Items[i];

        PhClearReference(&node->String);
        PhFree(node);
    }

    PhClearReference(&Context->NodeList);
    PhClearReference(&Context->SearchResults);
    Context->SearchResultsAddIndex = 0;
    Context->StringsCount = 0;
}

VOID PvpSearchStrings(
    _In_ PPV_STRINGS_CONTEXT Context
    )
{
    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    PvpDeleteStringsTree(Context);
    Context->NodeList = PhCreateList(100);
    Context->SearchResults = PhCreateList(100);

    TreeNew_SetEmptyText(Context->TreeNewHandle, &LoadingStringsText, 0);
    TreeNew_NodesStructured(Context->TreeNewHandle);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);

    PhCreateThreadEx(&Context->SearchThreadHandle, PvpSearchStringsThread, Context);
}

VOID PvpLoadSettingsStrings(
    _In_ PPV_STRINGS_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhGetStringSetting(L"StringsTreeListColumns");
    sortSettings = PhGetStringSetting(L"StringsTreeListSort");
    Context->Settings.Flags = PhGetIntegerSetting(L"StringsTreeListFlags");
    Context->Settings.MinimumLength = PhGetIntegerSetting(L"StringsMinimumLength");

    PhCmLoadSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &settings->sr, &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PvpSaveSettingsStrings(
    _In_ PPV_STRINGS_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhCmSaveSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &sortSettings);

    PhSetIntegerSetting(L"StringsMinimumLength", Context->Settings.MinimumLength);
    PhSetIntegerSetting(L"StringsTreeListFlags", Context->Settings.Flags);
    PhSetStringSetting2(L"StringsTreeListColumns", &settings->sr);
    PhSetStringSetting2(L"StringsTreeListSort", &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

BOOLEAN PvpGetSelectedStringsNodes(
    _In_ PPV_STRINGS_CONTEXT Context,
    _Out_ PPV_STRINGS_NODE** Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPV_STRINGS_NODE node = (PPV_STRINGS_NODE)Context->NodeList->Items[i];

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

    *Nodes = NULL;
    *NumberOfNodes = 0;

    PhDereferenceObject(list);
    return FALSE;
}

BOOLEAN PvpStringsTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPV_STRINGS_CONTEXT context = Context;
    PPV_STRINGS_NODE node = (PPV_STRINGS_NODE)Node;

    assert(Context);

    if (!context->Settings.Ansi && !node->Unicode)
        return FALSE;
    if (!context->Settings.Unicode && node->Unicode)
        return FALSE;

    if (!context->SearchMatchHandle)
        return TRUE;

    return PvSearchControlMatch(context->SearchMatchHandle, &node->String->sr);
}

VOID NTAPI PvpStringsSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PPV_STRINGS_CONTEXT context = Context;

    assert(context);

    context->SearchMatchHandle = MatchHandle;

    PhApplyTreeNewFilters(&context->FilterSupport);
}

#define SORT_FUNCTION(Column) PvpStringsTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PvpStringsTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPV_STRINGS_NODE node1 = *(PPV_STRINGS_NODE *)_elem1; \
    PPV_STRINGS_NODE node2 = *(PPV_STRINGS_NODE *)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintcmp(node1->Index, node2->Index); \
    \
    return PhModifySort(sortResult, ((PPV_STRINGS_CONTEXT)_context)->TreeNewSortOrder); \
}

LONG PvpStringsTreeNewPostSortFunction(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    )
{
    if (Result == 0)
        Result = uintcmp(((PPV_STRINGS_NODE)Node1)->Index, ((PPV_STRINGS_NODE)Node2)->Index);

    return PhModifySort(Result, SortOrder);
}

BEGIN_SORT_FUNCTION(Index)
{
    NOTHING;
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(SectionName)
{
    sortResult = PhCompareStringZ(node1->SectionName, node2->SectionName, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Rva)
{
    sortResult = uintptrcmp(node1->Rva, node2->Rva);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Type)
{
    sortResult = uintcmp(node1->Unicode, node2->Unicode);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Length)
{
    sortResult = uintptrcmp(node1->String->Length, node2->String->Length);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(String)
{
    sortResult = PhCompareString(node1->String, node2->String, FALSE);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PvpStringsTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PPV_STRINGS_CONTEXT context = Context;
    PPV_STRINGS_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PPV_STRINGS_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Index),
                    SORT_FUNCTION(SectionName),
                    SORT_FUNCTION(Rva),
                    SORT_FUNCTION(Type),
                    SORT_FUNCTION(Length),
                    SORT_FUNCTION(String),
                };
                int (__cdecl *sortFunction)(void *, const void *, const void *);

                static_assert(RTL_NUMBER_OF(sortFunctions) == PV_STRINGS_TREE_COLUMN_ITEM_MAXIMUM, "SortFunctions must equal maximum.");

                if (context->TreeNewSortColumn < PV_STRINGS_TREE_COLUMN_ITEM_MAXIMUM)
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
            node = (PPV_STRINGS_NODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;
            node = (PPV_STRINGS_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PV_STRINGS_TREE_COLUMN_ITEM_INDEX:
                PhInitializeStringRefLongHint(&getCellText->Text, node->IndexString);
                break;
            case PV_STRINGS_TREE_COLUMN_ITEM_SECTION:
                PhInitializeStringRefLongHint(&getCellText->Text, node->SectionName);
                break;
            case PV_STRINGS_TREE_COLUMN_ITEM_RVA:
                PhInitializeStringRefLongHint(&getCellText->Text, node->RvaString);
                break;
            case PV_STRINGS_TREE_COLUMN_ITEM_TYPE:
                PhInitializeStringRef(&getCellText->Text, node->Unicode ? L"Unicode" : L"ANSI");
                break;
            case PV_STRINGS_TREE_COLUMN_ITEM_LENGTH:
                PhInitializeStringRefLongHint(&getCellText->Text, node->LengthString);
                break;
            case PV_STRINGS_TREE_COLUMN_ITEM_STRING:
                getCellText->Text = node->String->sr;
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
            node = (PPV_STRINGS_NODE)getNodeColor->Node;

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
            case 'A':
                if (GetKeyState(VK_CONTROL) < 0)
                    TreeNew_SelectRange(context->TreeNewHandle, 0, -1);
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

            SendMessage(context->WindowHandle, WM_PV_SEARCH_SHOWMENU, 0, (LPARAM)contextMenu);
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

VOID PvpInitializeStringsTree(
    _In_ PPV_STRINGS_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HWND TreeNewHandle
    )
{
    Context->WindowHandle = WindowHandle;
    Context->TreeNewHandle = TreeNewHandle;
    Context->NodeList = PhCreateList(1);
    Context->SearchResults = PhCreateList(1);

    PhSetControlTheme(TreeNewHandle, L"explorer");

    TreeNew_SetCallback(TreeNewHandle, PvpStringsTreeNewCallback, Context);
    TreeNew_SetRedraw(TreeNewHandle, FALSE);

    PhAddTreeNewColumnEx2(TreeNewHandle, PV_STRINGS_TREE_COLUMN_ITEM_INDEX, TRUE, L"#", 40, PH_ALIGN_LEFT, PV_STRINGS_TREE_COLUMN_ITEM_INDEX, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_STRINGS_TREE_COLUMN_ITEM_SECTION, TRUE, L"Section", 80, PH_ALIGN_LEFT, PV_STRINGS_TREE_COLUMN_ITEM_SECTION, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_STRINGS_TREE_COLUMN_ITEM_RVA, TRUE, L"RVA", 80, PH_ALIGN_LEFT, PV_STRINGS_TREE_COLUMN_ITEM_RVA, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_STRINGS_TREE_COLUMN_ITEM_TYPE, TRUE, L"Type", 80, PH_ALIGN_LEFT, PV_STRINGS_TREE_COLUMN_ITEM_TYPE, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_STRINGS_TREE_COLUMN_ITEM_LENGTH, TRUE, L"Length", 80, PH_ALIGN_LEFT, PV_STRINGS_TREE_COLUMN_ITEM_LENGTH, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PV_STRINGS_TREE_COLUMN_ITEM_STRING, TRUE, L"String", 600, PH_ALIGN_LEFT, PV_STRINGS_TREE_COLUMN_ITEM_STRING, 0, 0);

    TreeNew_SetRedraw(TreeNewHandle, TRUE);
    TreeNew_SetSort(TreeNewHandle, PV_STRINGS_TREE_COLUMN_ITEM_INDEX, AscendingSortOrder);

    PhCmInitializeManager(&Context->Cm, TreeNewHandle, PV_STRINGS_TREE_COLUMN_ITEM_MAXIMUM, PvpStringsTreeNewPostSortFunction);

    PvpLoadSettingsStrings(Context);

    PhInitializeTreeNewFilterSupport(&Context->FilterSupport, TreeNewHandle, Context->NodeList);
}

INT_PTR CALLBACK PvpStringsMinimumLengthDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    PULONG length;

    if (uMsg == WM_INITDIALOG)
    {
        length = (PULONG)lParam;

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, length);
    }
    else
    {
        length = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!length)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            WCHAR lengthString[PH_INT32_STR_LEN_1];

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhPrintUInt32(lengthString, *length);

            PhSetDialogItemText(hwndDlg, IDC_MINIMUMLENGTH, lengthString);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    ULONG64 minimumLength;

                    PhStringToInteger64(&PhaGetDlgItemText(hwndDlg, IDC_MINIMUMLENGTH)->sr, 0, &minimumLength);

                    if (!minimumLength || minimumLength > MAXULONG32)
                    {
                        PhShowError(hwndDlg, L"%s", L"Invalid minimum length");
                        break;
                    }

                    *length = (ULONG)minimumLength;
                    EndDialog(hwndDlg, IDOK);
                }
                break;
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

ULONG PvpStringsMinimumLengthDialog(
    _In_ HWND WindowHandle,
    _In_ ULONG CurrentMinimumLength
)
{
    ULONG length;

    length = CurrentMinimumLength;

    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_STRINGSMINLEN),
        WindowHandle,
        PvpStringsMinimumLengthDlgProc,
        &length
        );

    return length;
}

INT_PTR CALLBACK PvStringsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_STRINGS_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PV_STRINGS_CONTEXT));
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
            context->SearchHandle = GetDlgItem(hwndDlg, IDC_TREESEARCH);
            PvCreateSearchControl(
                context->SearchHandle,
                L"Search Strings (Ctrl+K)",
                PvpStringsSearchControlCallback,
                context
                );

            PvpInitializeStringsTree(context, hwndDlg, GetDlgItem(hwndDlg, IDC_TREELIST));
            PhAddTreeNewFilter(&context->FilterSupport, PvpStringsTreeFilterCallback, context);
            PvConfigTreeBorders(context->TreeNewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->SearchHandle, NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);

            PvpSearchStrings(context);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PvpSaveSettingsStrings(context);
            PvpDeleteStringsTree(context);

            PhDeleteLayoutManager(&context->LayoutManager);

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
            PvpAddPendingStringsNodes(context);

            TreeNew_SetEmptyText(context->TreeNewHandle, &EmptyStringsText, 0);

            TreeNew_NodesStructured(context->TreeNewHandle);
        }
        break;
    case WM_PV_SEARCH_SHOWMENU:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
            PPH_EMENU menu;
            PPH_EMENU_ITEM selectedItem;
            PPV_STRINGS_NODE* stringsNodes = NULL;
            ULONG numberOfNodes = 0;

            if (!PvpGetSelectedStringsNodes(context, &stringsNodes, &numberOfNodes))
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
                PhFree(stringsNodes);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_SETTINGS:
                {
                    RECT rect;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM selectedItem;
                    PPH_EMENU_ITEM ansi;
                    PPH_EMENU_ITEM unicode;
                    PPH_EMENU_ITEM extendedUnicode;
                    PPH_EMENU_ITEM minimumLength;
                    PPH_EMENU_ITEM refresh;

                    GetWindowRect(GetDlgItem(hwndDlg, IDC_SETTINGS), &rect);

                    ansi = PhCreateEMenuItem(0, 1, L"ANSI", NULL, NULL);
                    unicode = PhCreateEMenuItem(0, 2, L"Unicode", NULL, NULL);
                    extendedUnicode = PhCreateEMenuItem(0, 3, L"Extended character set", NULL, NULL);
                    minimumLength = PhCreateEMenuItem(0, 4, L"Minimum length...", NULL, NULL);
                    refresh = PhCreateEMenuItem(0, 5, L"Refresh", NULL, NULL);

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, ansi, ULONG_MAX);
                    PhInsertEMenuItem(menu, unicode, ULONG_MAX);
                    PhInsertEMenuItem(menu, extendedUnicode, ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, minimumLength, ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, refresh, ULONG_MAX);

                    if (context->Settings.Ansi)
                        ansi->Flags |= PH_EMENU_CHECKED;
                    if (context->Settings.Unicode)
                        unicode->Flags |= PH_EMENU_CHECKED;
                    if (context->Settings.ExtendedCharSet)
                        extendedUnicode->Flags |= PH_EMENU_CHECKED;

                    selectedItem = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        rect.left,
                        rect.bottom
                        );

                    if (selectedItem && selectedItem->Id != ULONG_MAX)
                    {
                        if (selectedItem == ansi)
                        {
                            context->Settings.Ansi = !context->Settings.Ansi;
                            PvpSaveSettingsStrings(context);
                            PhApplyTreeNewFilters(&context->FilterSupport);
                        }
                        else if (selectedItem == unicode)
                        {
                            context->Settings.Unicode = !context->Settings.Unicode;
                            PvpSaveSettingsStrings(context);
                            PhApplyTreeNewFilters(&context->FilterSupport);
                        }
                        else if (selectedItem == extendedUnicode)
                        {
                            context->Settings.ExtendedCharSet = !context->Settings.ExtendedCharSet;
                            PvpSaveSettingsStrings(context);
                            PvpSearchStrings(context);
                        }
                        else if (selectedItem == minimumLength)
                        {
                            ULONG length = PvpStringsMinimumLengthDialog(hwndDlg, context->Settings.MinimumLength);
                            if (length != context->Settings.MinimumLength)
                            {
                                context->Settings.MinimumLength = length;
                                PvpSaveSettingsStrings(context);
                                PvpSearchStrings(context);
                            }
                        }
                        else if (selectedItem == refresh)
                        {
                            PvpSearchStrings(context);
                        }
                    }

                    PhDestroyEMenu(menu);
                }
                break;
            }
        }
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
