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

typedef enum _PV_RESOURCES_TREE_NODE_TYPE
{
    PV_RESOURCES_TREE_NODE_TYPE_IMAGE,
    PV_RESOURCES_TREE_NODE_TYPE_MUI,
    PV_RESOURCES_TREE_NODE_TYPE_MAXIMUM
} PV_RESOURCES_TREE_NODE_TYPE;

typedef struct _PV_RESOURCE_NODE
{
    PH_TREENEW_NODE Node;

    struct _PV_RESOURCE_NODE* Parent;
    PPH_LIST Children;
    BOOLEAN HasChildren;

    ULONG64 UniqueId;
    PV_RESOURCES_TREE_NODE_TYPE NodeType;

    PVOID RvaStart;
    PVOID RvaEnd;
    ULONG RvaSize;
    FLOAT ResourcesEntropy;
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

    PH_MAPPED_IMAGE MuiMappedImage;
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
    case (ULONG_PTR)RT_CURSOR:
        return PhCreateString(L"RT_CURSOR");
    case (ULONG_PTR)RT_BITMAP:
        return PhCreateString(L"RT_BITMAP");
    case (ULONG_PTR)RT_ICON:
        return PhCreateString(L"RT_ICON");
    case (ULONG_PTR)RT_MENU:
        return PhCreateString(L"RT_MENU");
    case (ULONG_PTR)RT_DIALOG:
        return PhCreateString(L"RT_DIALOG");
    case (ULONG_PTR)RT_STRING:
        return PhCreateString(L"RT_STRING");
    case (ULONG_PTR)RT_FONTDIR:
        return PhCreateString(L"RT_FONTDIR");
    case (ULONG_PTR)RT_FONT:
        return PhCreateString(L"RT_FONT");
    case (ULONG_PTR)RT_ACCELERATOR:
        return PhCreateString(L"RT_ACCELERATOR");
    case (ULONG_PTR)RT_RCDATA:
        return PhCreateString(L"RT_RCDATA");
    case (ULONG_PTR)RT_MESSAGETABLE:
        return PhCreateString(L"RT_MESSAGETABLE");
    case (ULONG_PTR)RT_GROUP_CURSOR:
        return PhCreateString(L"RT_GROUP_CURSOR");
    case (ULONG_PTR)RT_GROUP_ICON:
        return PhCreateString(L"RT_GROUP_ICON");
    case (ULONG_PTR)RT_VERSION:
        return PhCreateString(L"RT_VERSION");
    case (ULONG_PTR)RT_ANICURSOR:
        return PhCreateString(L"RT_ANICURSOR");
    case (ULONG_PTR)RT_MANIFEST:
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
    _In_ PPV_RESOURCES_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ PPV_RESOURCE_NODE ResourceNode
    )
{
    PH_MAPPED_IMAGE_RESOURCES resources = { 0 };
    PH_IMAGE_RESOURCE_ENTRY entry;
    PPH_STRING fileName;

    if (!(fileName = PvpPeResourceDumpFileName(WindowHandle)))
        return;

    if (ResourceNode->NodeType == PV_RESOURCES_TREE_NODE_TYPE_IMAGE)
    {
        if (!NT_SUCCESS(PhGetMappedImageResources(&resources, &PvMappedImage)))
        {
            PhDereferenceObject(fileName);
            return;
        }
    }
    else if (ResourceNode->NodeType == PV_RESOURCES_TREE_NODE_TYPE_MUI)
    {
        if (!NT_SUCCESS(PhGetMappedImageResources(&resources, &Context->MuiMappedImage)))
        {
            PhDereferenceObject(fileName);
            return;
        }
    }

    for (ULONG i = 0; i < resources.NumberOfEntries; i++)
    {
        entry = resources.ResourceEntries[i];

        if (UlongToPtr(entry.Offset) != ResourceNode->RvaStart)
            continue;

        if (entry.Size)
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

                PVOID resourceData = PhMappedImageRvaToVa(&PvMappedImage, entry.Offset, NULL);

                __try
                {
                    status = NtWriteFile(
                        fileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &isb,
                        resourceData,
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
    PhDereferenceObject(fileName);
}

VOID PvpPeEnumMappedImageResources(
    _In_ PPV_RESOURCES_CONTEXT Context,
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ BOOLEAN IsMuiFile
    )
{
    static ULONG64 NextUniqueId = 0;
    PH_MAPPED_IMAGE_RESOURCES resources;
    PH_IMAGE_RESOURCE_ENTRY entry;
    ULONG i;

    if (NT_SUCCESS(PhGetMappedImageResources(&resources, MappedImage)))
    {
        for (i = 0; i < resources.NumberOfEntries; i++)
        {
            PPV_RESOURCE_NODE resourceNode;
            WCHAR value[PH_INT64_STR_LEN_1];

            entry = resources.ResourceEntries[i];

            resourceNode = PhAllocateZero(sizeof(PV_RESOURCE_NODE));
            resourceNode->UniqueId = ++NextUniqueId;
            resourceNode->UniqueIdString = PhFormatUInt64(resourceNode->UniqueId, FALSE);
            resourceNode->NodeType = IsMuiFile ? PV_RESOURCES_TREE_NODE_TYPE_MUI : PV_RESOURCES_TREE_NODE_TYPE_IMAGE;

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

            // Language

            if (IS_INTRESOURCE(entry.Language))
            {
                if ((ULONG)entry.Language)
                {
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
                }
                else
                {
                    resourceNode->LcidString = PhCreateString(L"Neutral"); // LOCALE_NEUTRAL
                }
            }
            else
            {
                PIMAGE_RESOURCE_DIR_STRING_U resourceString = (PIMAGE_RESOURCE_DIR_STRING_U)entry.Language;

                resourceNode->LcidString = PhCreateStringEx(resourceString->NameString, resourceString->Length * sizeof(WCHAR));
            }

            // Hash

            if (entry.Size)
            {
                PVOID resourceData = PhMappedImageRvaToVa(&PvMappedImage, entry.Offset, NULL);

                if (resourceData)
                {
                    resourceNode->HashString = PvHashBuffer(resourceData, entry.Size);
                }
            }

            // Entropy

            if (entry.Size)
            {
                PVOID resourceData = PhMappedImageRvaToVa(&PvMappedImage, entry.Offset, NULL);

                if (resourceData)
                {
                    __try
                    {
                        FLOAT imageResourceEntropy;

                        if (PhCalculateEntropy(
                            resourceData,
                            entry.Size,
                            &imageResourceEntropy,
                            NULL
                            ))
                        {
                            resourceNode->ResourcesEntropy = imageResourceEntropy;
                            resourceNode->EntropyString = PhFormatEntropy(imageResourceEntropy, 2, 0, 0);
                        }
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        //resourceNode->EntropyString = PhGetNtMessage(GetExceptionCode());
                        resourceNode->EntropyString = PhGetWin32Message(PhNtStatusToDosError(GetExceptionCode())); // WIN32_FROM_NTSTATUS
                    }
                }
            }

            PhAcquireQueuedLockExclusive(&Context->SearchResultsLock);
            PhAddItemList(Context->SearchResults, resourceNode);
            PhReleaseQueuedLockExclusive(&Context->SearchResultsLock);
        }

        PhFree(resources.ResourceEntries);
    }
}

VOID PvpPeEnumAlternateMappedImageResources(
    _In_ PPV_RESOURCES_CONTEXT Context,
    _In_ PPH_STRING FileName
    )
{
    if (!Context->MuiMappedImage.ViewBase)
    {
        PVOID baseAddress;
        PVOID muiBaseAddress;
        PPH_STRING muiFileName;
        PH_MAPPED_IMAGE muiMappedImage;

        if (NT_SUCCESS(PhLoadLibraryAsImageResource(&FileName->sr, FALSE, &baseAddress)))
        {
            if (NT_SUCCESS(LdrLoadAlternateResourceModule(baseAddress, &muiBaseAddress, NULL, 0)))
            {
                if (NT_SUCCESS(PhGetProcessMappedFileName(NtCurrentProcess(), muiBaseAddress, &muiFileName)))
                {
                    if (NT_SUCCESS(PhLoadMappedImageEx(&muiFileName->sr, NULL, &muiMappedImage)))
                    {
                        Context->MuiMappedImage = muiMappedImage;
                    }

                    //PPH_STRING win32MuiFileName = PhGetFileName(muiFileName);
                    //PvpPeEnumAlternateMappedImageResources(Context, win32MuiFileName);
                    //PhDereferenceObject(win32MuiFileName);
                    PhDereferenceObject(muiFileName);
                }

                LdrUnloadAlternateResourceModule(muiBaseAddress);
            }

            PhFreeLibraryAsImageResource(baseAddress);
        }
    }

    if (Context->MuiMappedImage.ViewBase)
    {
        PvpPeEnumMappedImageResources(Context, &Context->MuiMappedImage, TRUE);
    }
}

NTSTATUS PvpPeResourcesEnumerateThread(
    _In_ PPV_RESOURCES_CONTEXT Context
    )
{
    // Enumerate the resources in the current image.
    PvpPeEnumMappedImageResources(Context, &PvMappedImage, FALSE);

    // Enumerate the resources in the alternate image.
    PvpPeEnumAlternateMappedImageResources(Context, PvFileName);

    PostMessage(Context->DialogHandle, WM_PV_SEARCH_FINISHED, 0, 0);
    return STATUS_SUCCESS;
}

VOID NTAPI PvpPeResourcesSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
)
{
    PPV_RESOURCES_CONTEXT context = Context;

    assert(context);

    context->SearchMatchHandle = MatchHandle;

    if (!context->SearchMatchHandle)
    {
        //PhExpandAllNodes(TRUE);
        //PhDeselectAllNodes();
    }

    PhApplyTreeNewFilters(&context->FilterSupport);
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
            context->SearchResults = PhCreateList(1);

            PvCreateSearchControl(
                hwndDlg,
                context->SearchHandle,
                L"Search Resources (Ctrl+K)",
                PvpPeResourcesSearchControlCallback,
                context
                );

            PvInitializeResourcesTree(context, hwndDlg, context->TreeNewHandle);
            PhAddTreeNewFilter(&context->FilterSupport, PvResourcesTreeFilterCallback, context);
            PhLoadSettingsResourcesList(context);
            PvConfigTreeBorders(context->TreeNewHandle);

            TreeNew_SetEmptyText(context->TreeNewHandle, &LoadingResourcesText, 0);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->SearchHandle, NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);

            PhCreateThread2(PvpPeResourcesEnumerateThread, context);

            PhInitializeWindowTheme(hwndDlg);
        }
        break;
    case WM_DESTROY:
        {
            //if (context->MuiMappedImage.ViewBase)
            //    PhUnloadMappedImage(&context->MuiMappedImage);

            PhSaveSettingsResourcesList(context);
            PvDeleteResourcesTree(context);
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
                            PvpPeResourceSaveToFile(context, hwndDlg, sectionNodes[0]);
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
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PPV_RESOURCES_CONTEXT context = Context;
    PPV_RESOURCE_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
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

                static_assert(RTL_NUMBER_OF(sortFunctions) == PV_RESOURCES_TREE_COLUMN_ITEM_MAXIMUM, "SortFunctions must equal maximum.");

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
            node = (PPV_RESOURCE_NODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;
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
            node = (PPV_RESOURCE_NODE)getNodeColor->Node;

            if (node->NodeType == PV_RESOURCES_TREE_NODE_TYPE_MUI)
            {
                getNodeColor->BackColor = RGB(0xcc, 0xcc, 0xff);
            }

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

BOOLEAN PvResourcesTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPV_RESOURCES_CONTEXT context = Context;
    PPV_RESOURCE_NODE node = (PPV_RESOURCE_NODE)Node;

    if (!context->SearchMatchHandle)
        return TRUE;

    if (!PhIsNullOrEmptyString(node->UniqueIdString))
    {
        if (PvSearchControlMatch(context->SearchMatchHandle, &node->UniqueIdString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->TypeString))
    {
        if (PvSearchControlMatch(context->SearchMatchHandle, &node->TypeString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->NameString))
    {
        if (PvSearchControlMatch(context->SearchMatchHandle, &node->NameString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->RvaStartString))
    {
        if (PvSearchControlMatch(context->SearchMatchHandle, &node->RvaStartString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->RvaEndString))
    {
        if (PvSearchControlMatch(context->SearchMatchHandle, &node->RvaEndString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->RvaSizeString))
    {
        if (PvSearchControlMatch(context->SearchMatchHandle, &node->RvaSizeString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->LcidString))
    {
        if (PvSearchControlMatch(context->SearchMatchHandle, &node->LcidString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->HashString))
    {
        if (PvSearchControlMatch(context->SearchMatchHandle, &node->HashString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->EntropyString))
    {
        if (PvSearchControlMatch(context->SearchMatchHandle, &node->EntropyString->sr))
            return TRUE;
    }

    return FALSE;
}
