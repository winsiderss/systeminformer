/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex         2016-2024
 *     Dart Vanya   2024
 *
 */

#include "exttools.h"
#include <secedit.h>
#include <hndlinfo.h>
#include <commoncontrols.h>

#include <kphcomms.h>
#include <kphuser.h>

static PH_STRINGREF EtObjectManagerRootDirectoryObject = PH_STRINGREF_INIT(L"\\"); // RtlNtPathSeparatorString
static PH_STRINGREF EtObjectManagerUserDirectoryObject = PH_STRINGREF_INIT(L"??"); // RtlDosDevicesPrefix
static PH_STRINGREF DirectoryObjectType = PH_STRINGREF_INIT(L"Directory");
static HANDLE EtObjectManagerDialogThreadHandle = NULL;
static PH_EVENT EtObjectManagerDialogInitializedEvent = PH_EVENT_INIT;

HWND EtObjectManagerDialogHandle = NULL;
LARGE_INTEGER EtObjectManagerTimeCached = { 0 };
PPH_LIST EtObjectManagerOwnHandles = NULL;
PPH_HASHTABLE EtObjectManagerPropWindows = NULL;
HICON EtObjectManagerPropIcon = NULL;

// Columns

#define ETOBLVC_NAME 0
#define ETOBLVC_TYPE 1
#define ETOBLVC_TARGET 2

typedef enum _ET_OBJECT_TYPE
{
    EtObjectUnknown = 0,
    EtObjectDirectory,
    EtObjectSymLink,
    EtObjectEvent,
    EtObjectMutant,
    EtObjectSemaphore,
    EtObjectSection,
    EtObjectDriver,
    EtObjectDevice,
    EtObjectAlpcPort,
    EtObjectFilterPort,
    EtObjectJob,
    EtObjectSession,
    EtObjectKey,
    EtObjectCpuPartition,
    EtObjectMemoryPartition,
    EtObjectKeyedEvent,
    EtObjectTimer,
    EtObjectWindowStation,
    EtObjectType,
    EtObjectCallback,

    EtObjectMax,
} ET_OBJECT_TYPE;

typedef struct _ET_OBJECT_ENTRY
{
    PPH_STRING Name;
    PPH_STRING TypeName;
    PPH_STRING Target;
    PPH_STRING TargetDrvLow;
    PPH_STRING TargetDrvUp;
    ET_OBJECT_TYPE EtObjectType;
    BOOL TargetIsResolving;
    CLIENT_ID TargetClientId;
    BOOLEAN PdoDevice;
} ET_OBJECT_ENTRY, *POBJECT_ENTRY;

typedef struct _ET_OBJECT_CONTEXT
{
    HWND WindowHandle;
    HWND ParentWindowHandle;
    HWND ListViewHandle;
    HWND TreeViewHandle;
    HWND SearchBoxHandle;
    PH_LAYOUT_MANAGER LayoutManager;

    HTREEITEM RootTreeObject;
    HTREEITEM SelectedTreeItem;

    HIMAGELIST TreeImageList;
    HIMAGELIST ListImageList;

    PPH_STRING CurrentPath;
    PPH_LIST CurrentDirectoryList;
    BOOLEAN DisableSelChanged;
    PBOOL BreakResolverThread;
} ET_OBJECT_CONTEXT, *POBJECT_CONTEXT;

typedef struct _DIRECTORY_ENUM_CONTEXT
{
    HWND TreeViewHandle;
    HTREEITEM RootTreeItem;
    PH_STRINGREF DirectoryPath;
} DIRECTORY_ENUM_CONTEXT, *PDIRECTORY_ENUM_CONTEXT;

typedef struct _HANDLE_OPEN_CONTEXT
{
    PPH_STRING CurrentPath;
    POBJECT_ENTRY Object;
} HANDLE_OPEN_CONTEXT, * PHANDLE_OPEN_CONTEXT;

typedef struct _RESOLVER_THREAD_CONTEXT
{
    BOOL Break;
    POBJECT_CONTEXT Context;
} RESOLVER_THREAD_CONTEXT, * PRESOLVER_THREAD_CONTEXT;

typedef struct _RESOLVER_WORK_THREAD_CONTEXT
{
    POBJECT_CONTEXT Context;
    POBJECT_ENTRY entry;
    INT ItemIndex;
    //BOOLEAN SortItems;
} RESOLVER_WORK_THREAD_CONTEXT, * PRESOLVER_WORK_THREAD_CONTEXT;

NTSTATUS EtTreeViewEnumDirectoryObjects(
    _In_ HWND TreeViewHandle,
    _In_ HTREEITEM RootTreeItem,
    _In_ PH_STRINGREF DirectoryPath
    );

#define OBJECT_OPENSOURCE_ALPCPORT  1
#define OBJECT_OPENSOURCE_KEY       2
#define OBJECT_OPENSOURCE_ALL   OBJECT_OPENSOURCE_ALPCPORT|OBJECT_OPENSOURCE_KEY

NTSTATUS EtObjectManagerOpenHandle(
    _Out_ PHANDLE Handle,
    _In_ PHANDLE_OPEN_CONTEXT Context,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG OpenFlags
    );

NTSTATUS EtObjectManagerOpenRealObject(
    _Out_ PHANDLE Handle,
    _In_ PHANDLE_OPEN_CONTEXT Context,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_opt_ PHANDLE OwnerProcessId
    );

NTSTATUS EtObjectManagerHandleCloseCallback(
    _In_ PVOID Context
    );

VOID NTAPI EtpObjectManagerChangeSelection(
    _In_ POBJECT_CONTEXT context
    );

VOID NTAPI EtpObjectManagerSortAndSelectOld(
    _In_ POBJECT_CONTEXT context,
    _In_opt_ PPH_STRING oldSelection
    );

VOID NTAPI EtpObjectManagerSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    );

NTSTATUS EtpTargetResolverWorkThreadStart(
    _In_ PVOID Parameter
    );

_Success_(return)
BOOLEAN PhGetTreeViewItemParam(
    _In_ HWND TreeViewHandle,
    _In_ HTREEITEM TreeItemHandle,
    _Outptr_ PVOID *Param,
    _Out_opt_ PULONG Children
    )
{
    TVITEM item;

    memset(&item, 0, sizeof(TVITEM));
    item.mask = TVIF_HANDLE | TVIF_PARAM | (Children ? TVIF_CHILDREN : 0);
    item.hItem = TreeItemHandle;

    if (!TreeView_GetItem(TreeViewHandle, &item))
        return FALSE;

    *Param = (PVOID)item.lParam;

    if (Children)
        *Children = item.cChildren;

    return TRUE;
}

PPH_STRING EtGetSelectedTreeViewPath(
    _In_ POBJECT_CONTEXT Context
    )
{
    PPH_STRING treePath = NULL;
    HTREEITEM treeItem;

    treeItem = Context->SelectedTreeItem;

    while (treeItem != Context->RootTreeObject)
    {
        PPH_STRING directory;

        if (!PhGetTreeViewItemParam(Context->TreeViewHandle, treeItem, &directory, NULL))
            break;

        if (treePath)
            treePath = PH_AUTO(PhConcatStringRef3(&directory->sr, &PhNtPathSeperatorString, &treePath->sr));
        else
            treePath = PH_AUTO(PhCreateString2(&directory->sr));

        treeItem = TreeView_GetParent(Context->TreeViewHandle, treeItem);
    }

    if (!PhIsNullOrEmptyString(treePath))
    {
        return PhConcatStringRef2(&PhNtPathSeperatorString, &treePath->sr);
    }

    return PhCreateString2(&EtObjectManagerRootDirectoryObject);
}

FORCEINLINE
PPH_STRING EtGetObjectFullPath(
    _In_ PPH_STRING CurrentPath,
    _In_ PPH_STRING ObjectName
    )
{
    PH_FORMAT format[3];
    BOOLEAN needSeparator = !PhEqualStringRef(&CurrentPath->sr, &EtObjectManagerRootDirectoryObject, TRUE);

    PhInitFormatSR(&format[0], CurrentPath->sr);
    if (needSeparator)
        PhInitFormatSR(&format[1], PhNtPathSeperatorString);
    PhInitFormatSR(&format[1 + needSeparator], ObjectName->sr);
    return PhFormat(format, 2 + needSeparator, 0);
}

HTREEITEM EtTreeViewAddItem(
    _In_ HWND TreeViewHandle,
    _In_ HTREEITEM Parent,
    _In_ BOOLEAN Expanded,
    _In_ PPH_STRINGREF Name
    )
{
    TV_INSERTSTRUCT insert;

    memset(&insert, 0, sizeof(TV_INSERTSTRUCT));
    insert.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
    insert.hInsertAfter = TVI_LAST;
    insert.hParent = Parent;
    insert.item.pszText = LPSTR_TEXTCALLBACK;

    if (Expanded)
    {
        insert.item.state = insert.item.stateMask = TVIS_EXPANDED;
    }

    insert.item.lParam = (LPARAM)PhCreateString2(Name);

    return TreeView_InsertItem(TreeViewHandle, &insert);
}

HTREEITEM EtTreeViewFindItem(
    _In_ HWND TreeViewHandle,
    _In_ HTREEITEM ParentTreeItem,
    _In_ PPH_STRINGREF Name
    )
{
    HTREEITEM current;
    HTREEITEM child;
    PPH_STRING directory;
    ULONG children;

    for (
        current = TreeView_GetChild(TreeViewHandle, ParentTreeItem);
        current;
        current = TreeView_GetNextSibling(TreeViewHandle, current)
        )
    {
        if (PhGetTreeViewItemParam(TreeViewHandle, current, &directory, &children))
        {
            if (PhEqualStringRef(&directory->sr, Name, TRUE))
            {
                return current;
            }

            if (children)
            {
                if (child = EtTreeViewFindItem(TreeViewHandle, current, Name))
                {
                    return child;
                }
            }
        }
    }

    return NULL;
}

VOID EtCleanupTreeViewItemParams(
    _In_ POBJECT_CONTEXT Context,
    _In_ HTREEITEM ParentTreeItem
    )
{
    HTREEITEM current;
    PPH_STRING directory;
    ULONG children;

    for (
        current = TreeView_GetChild(Context->TreeViewHandle, ParentTreeItem);
        current;
        current = TreeView_GetNextSibling(Context->TreeViewHandle, current)
        )
    {
        if (PhGetTreeViewItemParam(Context->TreeViewHandle, current, &directory, &children))
        {
            if (children)
            {
                EtCleanupTreeViewItemParams(Context, current);
            }

            PhDereferenceObject(directory);
        }
    }
}

VOID EtInitializeTreeImages(
    _In_ POBJECT_CONTEXT Context
    )
{
    HICON icon;
    LONG dpiValue;

    dpiValue = PhGetWindowDpi(Context->TreeViewHandle);

    Context->TreeImageList = PhImageListCreate(
        PhGetDpi(24, dpiValue),
        PhGetDpi(24, dpiValue),
        ILC_MASK | ILC_COLOR32,
        1, 1
        );

    if (icon = PhLoadIcon(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDI_FOLDER),
        PH_LOAD_ICON_SIZE_LARGE,
        PhGetDpi(16, dpiValue),
        PhGetDpi(16, dpiValue),
        dpiValue
        ))
    {
        PhImageListAddIcon(Context->TreeImageList, icon);
        DestroyIcon(icon);
    }
}

VOID EtInitializeListImages(
    _In_ POBJECT_CONTEXT Context
    )
{
    HICON icon;
    LONG dpiValue;
    LONG size;
    INT32 index;

    dpiValue = PhGetWindowDpi(Context->TreeViewHandle);
    size = PhGetDpi(20, dpiValue); // 24

    Context->ListImageList = PhImageListCreate(
        size,
        size,
        ILC_MASK | ILC_COLOR32,
        EtObjectMax, EtObjectMax
        );

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_UNKNOWN), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);

    for (INT i = 0; i < EtObjectMax; i++)
    {
        PhImageListAddIcon(Context->ListImageList, icon);
    }

    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_FOLDER), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    IImageList2_ReplaceIcon((IImageList2*)Context->ListImageList, EtObjectDirectory, icon, &index);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_LINK), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    IImageList2_ReplaceIcon((IImageList2*)Context->ListImageList, EtObjectSymLink, icon, &index);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_EVENT), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    IImageList2_ReplaceIcon((IImageList2*)Context->ListImageList, EtObjectEvent, icon, &index);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_MUTANT), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    IImageList2_ReplaceIcon((IImageList2*)Context->ListImageList, EtObjectMutant, icon, &index);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_SEMAPHORE), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    IImageList2_ReplaceIcon((IImageList2*)Context->ListImageList, EtObjectSemaphore, icon, &index);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_SECTION), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    IImageList2_ReplaceIcon((IImageList2*)Context->ListImageList, EtObjectSection, icon, &index);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_DRIVER), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    IImageList2_ReplaceIcon((IImageList2*)Context->ListImageList, EtObjectDriver, icon, &index);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_DEVICE), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    IImageList2_ReplaceIcon((IImageList2*)Context->ListImageList, EtObjectDevice, icon, &index);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_PORT), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    IImageList2_ReplaceIcon((IImageList2*)Context->ListImageList, EtObjectAlpcPort, icon, &index);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_FILTERPORT), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    IImageList2_ReplaceIcon((IImageList2*)Context->ListImageList, EtObjectFilterPort, icon, &index);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_JOB), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    IImageList2_ReplaceIcon((IImageList2*)Context->ListImageList, EtObjectJob, icon, &index);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_SESSION), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    IImageList2_ReplaceIcon((IImageList2*)Context->ListImageList, EtObjectSession, icon, &index);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_KEY), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    IImageList2_ReplaceIcon((IImageList2*)Context->ListImageList, EtObjectKey, icon, &index);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_CPU), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    IImageList2_ReplaceIcon((IImageList2*)Context->ListImageList, EtObjectCpuPartition, icon, &index);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_MEMORY), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    IImageList2_ReplaceIcon((IImageList2*)Context->ListImageList, EtObjectMemoryPartition, icon, &index);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_KEYEDEVENT), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    IImageList2_ReplaceIcon((IImageList2*)Context->ListImageList, EtObjectKeyedEvent, icon, &index);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_TIMER), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    IImageList2_ReplaceIcon((IImageList2*)Context->ListImageList, EtObjectTimer, icon, &index);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_WINSTA), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    IImageList2_ReplaceIcon((IImageList2*)Context->ListImageList, EtObjectWindowStation, icon, &index);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_TYPE), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    IImageList2_ReplaceIcon((IImageList2*)Context->ListImageList, EtObjectType, icon, &index);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_CALLBACK), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    IImageList2_ReplaceIcon((IImageList2*)Context->ListImageList, EtObjectCallback, icon, &index);
    DestroyIcon(icon);
}

static BOOLEAN NTAPI EtEnumDirectoryObjectsCallback(
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF TypeName,
    _In_ PDIRECTORY_ENUM_CONTEXT Context
    )
{
    if (PhEqualStringRef(TypeName, &DirectoryObjectType, TRUE))
    {
        PPH_STRING currentPath;
        HTREEITEM currentItem;

        if (PhEqualStringRef(&Context->DirectoryPath, &EtObjectManagerRootDirectoryObject, TRUE))
            currentPath = PhConcatStringRef2(&Context->DirectoryPath, Name);
        else
            currentPath = PhConcatStringRef3(&Context->DirectoryPath, &PhNtPathSeperatorString, Name);

        currentItem = EtTreeViewAddItem(
            Context->TreeViewHandle,
            Context->RootTreeItem,
            FALSE,
            Name
            );

        EtTreeViewEnumDirectoryObjects(
            Context->TreeViewHandle,
            currentItem,
            PhGetStringRef(currentPath)
            );

        PhDereferenceObject(currentPath);
    }

    return TRUE;
}

static BOOLEAN NTAPI EtEnumCurrentDirectoryObjectsCallback(
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF TypeName,
    _In_ POBJECT_CONTEXT Context
    )
{
    if (PhEqualStringRef(TypeName, &DirectoryObjectType, TRUE))
    {
        if (!EtTreeViewFindItem(Context->TreeViewHandle, Context->SelectedTreeItem, Name))
        {
            EtTreeViewAddItem(Context->TreeViewHandle, Context->SelectedTreeItem, TRUE, Name);
        }
    }
    else
    {
        INT index;
        POBJECT_ENTRY entry;

        entry = PhAllocateZero(sizeof(ET_OBJECT_ENTRY));
        entry->Name = PhCreateString2(Name);
        entry->TypeName = PhCreateString2(TypeName);
        entry->EtObjectType = EtObjectUnknown;

        if (PhEqualStringRef2(TypeName, L"ALPC Port", TRUE))
        {
            entry->EtObjectType = EtObjectAlpcPort;
        }
        else if (PhEqualStringRef2(TypeName, L"Callback", TRUE))
        {
            entry->EtObjectType = EtObjectCallback;
        }
        else if (PhEqualStringRef2(TypeName, L"CpuPartition", TRUE))
        {
            entry->EtObjectType = EtObjectCpuPartition;
        }
        else if (PhEqualStringRef2(TypeName, L"Device", TRUE))
        {
            entry->EtObjectType = EtObjectDevice;
        }
        else if (PhEqualStringRef(TypeName, &DirectoryObjectType, TRUE))
        {
            entry->EtObjectType = EtObjectDirectory;
        }
        else if (PhEqualStringRef2(TypeName, L"Driver", TRUE))
        {
            entry->EtObjectType = EtObjectDriver;
        }
        else if (PhEqualStringRef2(TypeName, L"Event", TRUE))
        {
            entry->EtObjectType = EtObjectEvent;
        }
        else if (PhEqualStringRef2(TypeName, L"FilterConnectionPort", TRUE))
        {
            entry->EtObjectType = EtObjectFilterPort;
        }
        else if (PhEqualStringRef2(TypeName, L"Job", TRUE))
        {
            entry->EtObjectType = EtObjectJob;
        }
        else if (PhEqualStringRef2(TypeName, L"Key", TRUE))
        {
            entry->EtObjectType = EtObjectKey;
        }
        else if (PhEqualStringRef2(TypeName, L"KeyedEvent", TRUE))
        {
            entry->EtObjectType = EtObjectKeyedEvent;
        }
        else if (PhEqualStringRef2(TypeName, L"Mutant", TRUE))
        {
            entry->EtObjectType = EtObjectMutant;
        }
        else if (PhEqualStringRef2(TypeName, L"Partition", TRUE))
        {
            entry->EtObjectType = EtObjectMemoryPartition;
        }
        else if (PhEqualStringRef2(TypeName, L"Section", TRUE))
        {
            entry->EtObjectType = EtObjectSection;
        }
        else if (PhEqualStringRef2(TypeName, L"Semaphore", TRUE))
        {
            entry->EtObjectType = EtObjectSemaphore;
        }
        else if (PhEqualStringRef2(TypeName, L"Session", TRUE))
        {
            entry->EtObjectType = EtObjectSession;
        }
        else if (PhEqualStringRef2(TypeName, L"SymbolicLink", TRUE))
        {
            entry->EtObjectType = EtObjectSymLink;
        }
        else if (PhEqualStringRef2(TypeName, L"Timer", TRUE))
        {
            entry->EtObjectType = EtObjectTimer;
        }
        else if (PhEqualStringRef2(TypeName, L"Type", TRUE))
        {
            entry->EtObjectType = EtObjectType;
        }
        else if (PhEqualStringRef2(TypeName, L"WindowStation", TRUE))
        {
            entry->EtObjectType = EtObjectWindowStation;
        }

        index = PhAddListViewItem(Context->ListViewHandle, MAXINT, LPSTR_TEXTCALLBACK, entry);
        PhSetListViewItemImageIndex(Context->ListViewHandle, index, entry->EtObjectType);

        if (entry->EtObjectType == EtObjectSymLink)
        {
            PPH_STRING fullPath;
            PPH_STRING linkTarget;

            fullPath = PH_AUTO(EtGetObjectFullPath(Context->CurrentPath, entry->Name));

            if (!PhIsNullOrEmptyString(fullPath) &&
                NT_SUCCESS(PhQuerySymbolicLinkObject(&linkTarget, NULL, &fullPath->sr)))
            {
                PhMoveReference(&entry->Target, linkTarget);
            }
        }
        else if (entry->EtObjectType == EtObjectDevice ||       // allow resolving without driver for PhGetPnPDeviceName()
                 entry->EtObjectType == EtObjectAlpcPort && KsiLevel() >= KphLevelMed ||
                 entry->EtObjectType == EtObjectMutant ||
                 entry->EtObjectType == EtObjectJob ||
                 entry->EtObjectType == EtObjectDriver && KsiLevel() == KphLevelMax)
        {
            entry->TargetIsResolving = TRUE;
        }
        else if (entry->EtObjectType == EtObjectWindowStation)
        {
            entry->Target = EtGetWindowStationType(&entry->Name->sr);
        }

        PhAddItemList(Context->CurrentDirectoryList, entry);
    }

    return TRUE;
}

NTSTATUS EtTreeViewEnumDirectoryObjects(
    _In_ HWND TreeViewHandle,
    _In_ HTREEITEM RootTreeItem,
    _In_ PH_STRINGREF DirectoryPath
    )
{
    NTSTATUS status;
    HANDLE directoryHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;

    PhStringRefToUnicodeString(&DirectoryPath, &objectName);
    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenDirectoryObject(
        &directoryHandle,
        DIRECTORY_QUERY,
        &objectAttributes
        );

    if (NT_SUCCESS(status))
    {
        DIRECTORY_ENUM_CONTEXT enumContext;

        enumContext.TreeViewHandle = TreeViewHandle;
        enumContext.RootTreeItem = RootTreeItem;
        enumContext.DirectoryPath = DirectoryPath;

        status = PhEnumDirectoryObjects(
            directoryHandle,
            EtEnumDirectoryObjectsCallback,
            &enumContext
            );

        NtClose(directoryHandle);
    }

    TreeView_SortChildren(TreeViewHandle, RootTreeItem, TRUE);

    return status;
}

NTSTATUS EtpTargetResolverThreadStart(
    _In_ PVOID Parameter
    )
{
    PRESOLVER_THREAD_CONTEXT threadContext = Parameter;
    POBJECT_CONTEXT context = threadContext->Context;

    NTSTATUS status;
    PRESOLVER_WORK_THREAD_CONTEXT workContext;
    POBJECT_ENTRY entry;
    PH_WORK_QUEUE workQueue;
    ULONG sortColumn;
    PH_SORT_ORDER sortOrder;

    PhInitializeWorkQueue(&workQueue, 1, 20, 1000);

    ExtendedListView_GetSort(context->ListViewHandle, &sortColumn, &sortOrder);
    BOOLEAN sortItems = sortOrder != NoSortOrder && sortColumn == 2;

    for (ULONG i = 0; i < context->CurrentDirectoryList->Count; i++)
    {
        // Thread was interrupted externally
        if (threadContext->Break)
            break;

        entry = context->CurrentDirectoryList->Items[i];

        if (!entry->TargetIsResolving)
            continue;

        workContext = PhAllocateZero(sizeof(RESOLVER_WORK_THREAD_CONTEXT));
        workContext->Context = context;
        workContext->entry = entry;
        workContext->ItemIndex = PhFindListViewItemByParam(context->ListViewHandle, INT_ERROR, entry);
        //workContext->SortItems = SortItems;

        PhQueueItemWorkQueue(&workQueue, EtpTargetResolverWorkThreadStart, workContext);
    }

    PhWaitForWorkQueue(&workQueue);
    PhDeleteWorkQueue(&workQueue);

    if (!threadContext->Break)
    {
        // Reapply sort and filter after done resolving and ensure selected item is visible
        PPH_STRING curentFilter = PhGetWindowText(context->SearchBoxHandle);
        if (!PhIsNullOrEmptyString(curentFilter))
        {
            EtpObjectManagerSearchControlCallback(  // HACK
                (ULONG_PTR)PhGetWindowContext(context->SearchBoxHandle, SHRT_MAX), context);
        }
        else
        {
            if (sortItems)
            {
                ExtendedListView_SortItems(context->ListViewHandle);

                INT index = ListView_GetNextItem(context->ListViewHandle, INT_ERROR, LVNI_SELECTED);
                if (index != INT_ERROR)
                    ListView_EnsureVisible(context->ListViewHandle, index, TRUE);
            }
        }
        if (curentFilter)
            PhDereferenceObject(curentFilter);

        context->BreakResolverThread = NULL;
        status = STATUS_SUCCESS;
    }
    else
        status = STATUS_ABANDONED;

    PhFree(threadContext);
    return status;
}

NTSTATUS EtpTargetResolverWorkThreadStart(
    _In_ PVOID Parameter
    )
{
    PRESOLVER_WORK_THREAD_CONTEXT threadContext = Parameter;
    POBJECT_CONTEXT context = threadContext->Context;
    POBJECT_ENTRY entry = threadContext->entry;

    NTSTATUS status;
    HANDLE_OPEN_CONTEXT objectContext;
    HANDLE objectHandle;

    objectContext.CurrentPath = PhReferenceObject(context->CurrentPath);
    objectContext.Object = entry;

    switch (entry->EtObjectType)
    {
        case EtObjectDevice:
            {
                HANDLE deviceObject;
                HANDLE deviceBaseObject;
                HANDLE driverObject;
                PPH_STRING deviceName;
                PPH_STRING driverNameLow = NULL;
                PPH_STRING driverNameUp = NULL;
                OBJECT_ATTRIBUTES objectAttributes;
                UNICODE_STRING objectName;

                deviceName = EtGetObjectFullPath(objectContext.CurrentPath, entry->Name);

                if (KsiLevel() == KphLevelMax)
                {
                    PhStringRefToUnicodeString(&deviceName->sr, &objectName);
                    InitializeObjectAttributes(&objectAttributes, &objectName, OBJ_CASE_INSENSITIVE, NULL, NULL);

                    if (NT_SUCCESS(KphOpenDevice(&deviceObject, READ_CONTROL, &objectAttributes)))
                    {
                        if (NT_SUCCESS(KphOpenDeviceDriver(deviceObject, READ_CONTROL, &driverObject)))
                        {
                            PhGetDriverName(driverObject, &driverNameUp);
                            NtClose(driverObject);
                        }

                        if (NT_SUCCESS(KphOpenDeviceBaseDevice(deviceObject, READ_CONTROL, &deviceBaseObject)))
                        {
                            if (NT_SUCCESS(KphOpenDeviceDriver(deviceBaseObject, READ_CONTROL, &driverObject)))
                            {
                                PhGetDriverName(driverObject, &driverNameLow);
                                NtClose(driverObject);
                            }
                            NtClose(deviceBaseObject);
                        }

                        NtClose(deviceObject);
                    }

                    if (driverNameLow && driverNameUp)
                    {
                        if (!PhEqualString(driverNameLow, driverNameUp, TRUE))
                            PhMoveReference(&entry->Target, PhFormatString(L"%s → %s", PhGetString(driverNameUp), PhGetString(driverNameLow)));
                        else
                            PhMoveReference(&entry->Target, PhReferenceObject(driverNameLow));
                        PhMoveReference(&entry->TargetDrvLow, driverNameLow);
                        PhMoveReference(&entry->TargetDrvUp, driverNameUp);
                    }
                    else if (driverNameLow)
                    {
                        PhMoveReference(&entry->Target, PhReferenceObject(driverNameLow));
                        PhMoveReference(&entry->TargetDrvLow, driverNameLow);
                    }
                    else if (driverNameUp)
                    {
                        PhMoveReference(&entry->Target, PhReferenceObject(driverNameUp));
                        PhMoveReference(&entry->TargetDrvUp, driverNameUp);
                    }
                }

                // The device might be a PDO... Query the PnP manager for the friendly name of the device.
                if (!entry->Target)
                {
                    PPH_STRING devicePdoName;
                    PPH_STRING devicePdoName2;

                    if (devicePdoName = PhGetPnPDeviceName(deviceName))
                    {
                        ULONG_PTR column_pos = PhFindLastCharInString(devicePdoName, 0, L':');
                        devicePdoName2 = PhSubstring(devicePdoName, 0, column_pos + 1);
                        devicePdoName2->Buffer[column_pos - 4] = L'[', devicePdoName2->Buffer[column_pos] = L']';
                        PhMoveReference(&entry->Target, devicePdoName2);
                        entry->PdoDevice = TRUE;
                        PhDereferenceObject(devicePdoName);
                    }
                }

                PhDereferenceObject(deviceName);
            }
            break;
        case EtObjectAlpcPort:
            {
                HANDLE processId = NtCurrentProcessId();
                HANDLE processHandle = NtCurrentProcess();
                // Using fast connect to port since we only need query connection OwnerProcessId
                if (!NT_SUCCESS(status = EtObjectManagerOpenHandle(&objectHandle, &objectContext, READ_CONTROL, 0)))
                {
                    // On failure try to open real (rare)
                    status = EtObjectManagerOpenRealObject(&objectHandle, &objectContext, READ_CONTROL, &processId);
                }

                if (NT_SUCCESS(status))
                {
                    KPH_ALPC_COMMUNICATION_INFORMATION connectionInfo;

                    if (processId != NtCurrentProcessId())
                    {
                        status = PhOpenProcess(
                            &processHandle,
                            PROCESS_QUERY_LIMITED_INFORMATION,
                            processId
                            );
                    }

                    if (NT_SUCCESS(status))
                    {
                        if (NT_SUCCESS(status = KphAlpcQueryInformation(
                            processHandle,
                            objectHandle,
                            KphAlpcCommunicationInformation,
                            &connectionInfo,
                            sizeof(connectionInfo),
                            NULL
                            )))
                        {
                            CLIENT_ID clientId;

                            if (connectionInfo.ConnectionPort.OwnerProcessId)
                            {
                                clientId.UniqueProcess = connectionInfo.ConnectionPort.OwnerProcessId;
                                clientId.UniqueThread = 0;

                                PhMoveReference(&entry->Target, PhStdGetClientIdName(&clientId));

                                entry->TargetClientId.UniqueProcess = clientId.UniqueProcess;
                                entry->TargetClientId.UniqueThread = clientId.UniqueThread;
                            }
                        }

                        NtClose(processId == NtCurrentProcessId() ? objectHandle : processHandle);
                    }
                }
            }
            break;
        case EtObjectMutant:
            {
                if (NT_SUCCESS(status = EtObjectManagerOpenHandle(&objectHandle, &objectContext, MUTANT_QUERY_STATE, 0)))
                {
                    MUTANT_OWNER_INFORMATION ownerInfo;

                    if (NT_SUCCESS(status = PhGetMutantOwnerInformation(objectHandle, &ownerInfo)))
                    {
                        if (ownerInfo.ClientId.UniqueProcess)
                        {
                            PhMoveReference(&entry->Target, PhGetClientIdName(&ownerInfo.ClientId));

                            entry->TargetClientId.UniqueProcess = ownerInfo.ClientId.UniqueProcess;
                            entry->TargetClientId.UniqueThread = ownerInfo.ClientId.UniqueThread;
                        }
                    }

                    NtClose(objectHandle);
                }
            }
            break;
        case EtObjectJob:
            {
                if (NT_SUCCESS(status = EtObjectManagerOpenHandle(&objectHandle, &objectContext, JOB_OBJECT_QUERY, 0)))
                {
                    PJOBOBJECT_BASIC_PROCESS_ID_LIST processIdList;

                    if (NT_SUCCESS(PhGetJobProcessIdList(objectHandle, &processIdList)))
                    {
                        PH_STRING_BUILDER sb;
                        ULONG i;
                        CLIENT_ID clientId;
                        PPH_STRING name;

                        PhInitializeStringBuilder(&sb, 40);
                        clientId.UniqueThread = NULL;

                        for (i = 0; i < processIdList->NumberOfProcessIdsInList; i++)
                        {
                            clientId.UniqueProcess = (HANDLE)processIdList->ProcessIdList[i];
                            name = PhGetClientIdName(&clientId);

                            if (name)
                            {
                                PhAppendStringBuilder(&sb, &name->sr);
                                PhAppendStringBuilder2(&sb, L"; ");
                                PhDereferenceObject(name);
                            }
                        }

                        PhFree(processIdList);

                        if (sb.String->Length != 0)
                            PhRemoveEndStringBuilder(&sb, 2);

                        if (sb.String->Length == 0)
                            PhAppendStringBuilder2(&sb, L"(No processes)");

                        PhMoveReference(&entry->Target, PhFinalStringBuilderString(&sb));
                    }
                    NtClose(objectHandle);
                }
            }
            break;
        case EtObjectDriver:
            {
                PPH_STRING driverName;

                if (NT_SUCCESS(status = EtObjectManagerOpenHandle(&objectHandle, &objectContext, READ_CONTROL, 0)))
                {
                    if (NT_SUCCESS(status = PhGetDriverImageFileName(objectHandle, &driverName)))
                    {
                        PhMoveReference(&entry->Target, driverName);
                    }
                    NtClose(objectHandle);
                }
            }
            break;
    }

    // Target was successfully resolved, redraw list entry
    entry->TargetIsResolving = FALSE;
    ListView_RedrawItems(context->ListViewHandle, threadContext->ItemIndex, threadContext->ItemIndex);
    //if (threadContext->SortItems)
    //    ExtendedListView_SortItems(Context->ListViewHandle);

    PhDereferenceObject(objectContext.CurrentPath);
    PhFree(threadContext);

    return STATUS_SUCCESS;
}

NTSTATUS EtEnumCurrentDirectoryObjects(
    _In_ POBJECT_CONTEXT Context
    )
{
    NTSTATUS status;
    HANDLE directoryHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;
    ULONG sortColumn;
    PH_SORT_ORDER sortOrder;
    WCHAR string[PH_INT32_STR_LEN_1];

    Context->CurrentPath = EtGetSelectedTreeViewPath(Context);

    PhStringRefToUnicodeString(&Context->CurrentPath->sr, &objectName);
    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenDirectoryObject(
        &directoryHandle,
        DIRECTORY_QUERY,
        &objectAttributes
        );

    if (NT_SUCCESS(status))
    {
        status = PhEnumDirectoryObjects(
            directoryHandle,
            EtEnumCurrentDirectoryObjectsCallback,
            Context
            );

        NtClose(directoryHandle);
    }

    if (!NT_SUCCESS(status) && status != STATUS_NO_MORE_ENTRIES)
    {
        PhShowStatus(Context->WindowHandle, L"Unable to query directory object.", status, 0);
    }

    PhSetWindowText(GetDlgItem(Context->WindowHandle, IDC_OBJMGR_PATH), PhGetString(Context->CurrentPath));

    PhPrintUInt32(string, Context->CurrentDirectoryList->Count);
    PhSetWindowText(GetDlgItem(Context->WindowHandle, IDC_OBJMGR_COUNT), string);

    // Apply current filter and sort
    PPH_STRING curentFilter = PH_AUTO(PhGetWindowText(Context->SearchBoxHandle));
    if (!PhIsNullOrEmptyString(curentFilter))
    {
        EtpObjectManagerSearchControlCallback(  // HACK
            (ULONG_PTR)PhGetWindowContext(Context->SearchBoxHandle, SHRT_MAX), Context);
    }

    ExtendedListView_GetSort(Context->ListViewHandle, &sortColumn, &sortOrder);
    if (sortOrder != NoSortOrder)
        ExtendedListView_SortItems(Context->ListViewHandle);

    PRESOLVER_THREAD_CONTEXT threadContext = PhAllocateZero(sizeof(RESOLVER_THREAD_CONTEXT));
    threadContext->Context = Context;
    Context->BreakResolverThread = &threadContext->Break;

    PhCreateThread2(EtpTargetResolverThreadStart, threadContext);

    return STATUS_SUCCESS;
}

VOID EtObjectManagerFreeListViewItems(
    _In_ POBJECT_CONTEXT Context
    )
{
    INT index = INT_ERROR;

    if (Context->BreakResolverThread)
    {
        *Context->BreakResolverThread = TRUE;
        Context->BreakResolverThread = NULL;
    }

    PhClearReference(&Context->CurrentPath);

    while ((index = PhFindListViewItemByFlags(
        Context->ListViewHandle,
        index,
        LVNI_ALL
        )) != INT_ERROR)
    {
        POBJECT_ENTRY entry;

        if (PhGetListViewItemParam(Context->ListViewHandle, index, &entry))
        {
            PhClearReference(&entry->Name);
            PhClearReference(&entry->TypeName);
            if (entry->Target)
                PhClearReference(&entry->Target);
            if (entry->TargetDrvLow)
                PhClearReference(&entry->TargetDrvLow);
            if (entry->TargetDrvUp)
                PhClearReference(&entry->TargetDrvUp);
            PhFree(entry);
        }
    }

    PhClearList(Context->CurrentDirectoryList);
}


NTSTATUS EtDuplicateHandleFromProcessEx(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ProcessId,
    _In_ HANDLE SourceHandle
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_DUP_HANDLE,
        ProcessId
        )))
    {
        status = NtDuplicateObject(
            processHandle,
            SourceHandle,
            NtCurrentProcess(),
            Handle,
            DesiredAccess,
            0,
            0
            );
        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status) && KsiLevel() == KphLevelMax)
    {
        if (NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_LIMITED_INFORMATION,
            ProcessId
            )))
        {
            status = KphDuplicateObject(
                processHandle,
                SourceHandle,
                DesiredAccess,
                Handle
                );
            NtClose(processHandle);
        }
    }

    return status;
}

NTSTATUS EtObjectManagerOpenHandle(
    _Out_ PHANDLE Handle,
    _In_ PHANDLE_OPEN_CONTEXT Context,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG OpenFlags
     )
{
    NTSTATUS status;
    HANDLE directoryHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;

    *Handle = NULL;

    if (Context->Object->EtObjectType >= EtObjectMax || PhIsNullOrEmptyString(Context->Object->TypeName))
        return STATUS_INVALID_PARAMETER;

    if (Context->Object->EtObjectType == EtObjectDirectory)
    {
        PhStringRefToUnicodeString(&Context->CurrentPath->sr, &objectName);
        InitializeObjectAttributes(&objectAttributes, &objectName, OBJ_CASE_INSENSITIVE, NULL, NULL);

        return NtOpenDirectoryObject(Handle, DesiredAccess, &objectAttributes);
    }

    PhStringRefToUnicodeString(&Context->CurrentPath->sr, &objectName);
    InitializeObjectAttributes(&objectAttributes, &objectName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    if (!NT_SUCCESS(status = NtOpenDirectoryObject(
        &directoryHandle,
        DIRECTORY_QUERY,
        &objectAttributes
        )))
    {
        return status;
    }

    PhStringRefToUnicodeString(&Context->Object->Name->sr, &objectName);
    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        directoryHandle,
        NULL
        );

    status = STATUS_UNSUCCESSFUL;

    switch (Context->Object->EtObjectType)
    {
        case EtObjectAlpcPort:
            {
                static PH_INITONCE initOnce = PH_INITONCE_INIT;
                static NTSTATUS (NTAPI* NtAlpcConnectPortEx_I)(
                    _Out_ PHANDLE PortHandle,
                    _In_ POBJECT_ATTRIBUTES ConnectionPortObjectAttributes,
                    _In_opt_ POBJECT_ATTRIBUTES ClientPortObjectAttributes,
                    _In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes,
                    _In_ ULONG Flags,
                    _In_opt_ PSECURITY_DESCRIPTOR ServerSecurityRequirements,
                    _Inout_updates_bytes_to_opt_(*BufferLength, *BufferLength) PPORT_MESSAGE ConnectionMessage,
                    _Inout_opt_ PSIZE_T BufferLength,
                    _Inout_opt_ PALPC_MESSAGE_ATTRIBUTES OutMessageAttributes,
                    _Inout_opt_ PALPC_MESSAGE_ATTRIBUTES InMessageAttributes,
                    _In_opt_ PLARGE_INTEGER Timeout
                    ) = NULL;

                if (PhBeginInitOnce(&initOnce))
                {
                    NtAlpcConnectPortEx_I = PhGetModuleProcAddress(L"ntdll.dll", "NtAlpcConnectPortEx");
                    PhEndInitOnce(&initOnce);
                }

                if (OpenFlags & OBJECT_OPENSOURCE_ALPCPORT)
                {
                    status = EtObjectManagerOpenRealObject(Handle, Context, DesiredAccess, NULL);
                }
                else
                {
                    if (NtAlpcConnectPortEx_I)
                    {
                        status = NtAlpcConnectPortEx_I(
                            Handle,
                            &objectAttributes,
                            NULL,
                            NULL,
                            0,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            PhTimeoutFromMillisecondsEx(1000)
                            );
                    }
                    else
                    {
                        PPH_STRING clientName;

                        clientName = EtGetObjectFullPath(Context->CurrentPath, Context->Object->Name);
                        PhStringRefToUnicodeString(&clientName->sr, &objectName);

                        status = NtAlpcConnectPort(
                            Handle,
                            &objectName,
                            NULL,
                            NULL,
                            0,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            PhTimeoutFromMillisecondsEx(1000)
                            );
                        PhDereferenceObject(clientName);
                    }
                }
            }
            break;
        case EtObjectDevice:
            {
                HANDLE deviceObject;
                HANDLE deviceBaseObject;

                if (KsiLevel() == KphLevelMax)
                {
                    if (NT_SUCCESS(status = KphOpenDevice(&deviceObject, DesiredAccess, &objectAttributes)))
                    {
                        if (NT_SUCCESS(status = KphOpenDeviceBaseDevice(deviceObject, DesiredAccess, &deviceBaseObject)))
                        {
                            *Handle = deviceBaseObject;
                            NtClose(deviceObject);
                        }
                        else
                        {
                            *Handle = deviceObject;
                            status = STATUS_SUCCESS;
                        }
                    }
                }

                if (!NT_SUCCESS(status))
                {
                    PPH_STRING deviceName;
                    deviceName = EtGetObjectFullPath(Context->CurrentPath, Context->Object->Name);

                    if (NT_SUCCESS(status = PhCreateFile(
                        Handle,
                        &deviceName->sr,
                        DesiredAccess,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_OPEN,
                        FILE_NON_DIRECTORY_FILE
                        )))
                    {
                        status = STATUS_NOT_ALL_ASSIGNED;
                    }
                    PhDereferenceObject(deviceName);
                }
            }
            break;
        case EtObjectDriver:
            {
                status = PhOpenDriver(Handle, DesiredAccess, directoryHandle, &Context->Object->Name->sr);
            }
            break;
        case EtObjectEvent:
            {
                status = NtOpenEvent(Handle, DesiredAccess, &objectAttributes);
            }
            break;
        case EtObjectTimer:
            {
                status = NtOpenTimer(Handle, DesiredAccess, &objectAttributes);
            }
            break;
        case EtObjectJob:
            {
                status = NtOpenJobObject(Handle, DesiredAccess, &objectAttributes);
            }
            break;
        case EtObjectKey:
            {
                if (OpenFlags & OBJECT_OPENSOURCE_KEY)
                {
                    if (!NT_SUCCESS(status = EtObjectManagerOpenRealObject(Handle, Context, DesiredAccess, NULL)))
                    {
                        if (NT_SUCCESS(status = NtOpenKey(Handle, DesiredAccess, &objectAttributes)))
                        {
                            status = STATUS_NOT_ALL_ASSIGNED;
                        }
                    }
                }
                else
                {
                    status = NtOpenKey(Handle, DesiredAccess, &objectAttributes);
                }
            }
            break;
        case EtObjectKeyedEvent:
            {
                status = NtOpenKeyedEvent(Handle, DesiredAccess, &objectAttributes);
            }
            break;
        case EtObjectMutant:
            {
                status = NtOpenMutant(Handle, DesiredAccess, &objectAttributes);
            }
            break;
        case EtObjectSemaphore:
            {
                status = NtOpenSemaphore(Handle, DesiredAccess, &objectAttributes);
            }
            break;
        case EtObjectSection:
            {
                status = NtOpenSection(Handle, DesiredAccess, &objectAttributes);
            }
            break;
        case EtObjectSession:
            {
                status = NtOpenSession(Handle, DesiredAccess, &objectAttributes);
            }
            break;
        case EtObjectSymLink:
            {
                status = NtOpenSymbolicLinkObject(Handle, DesiredAccess, &objectAttributes);
            }
            break;
        case EtObjectWindowStation:
            {
                static PH_INITONCE initOnce = PH_INITONCE_INIT;
                static HWINSTA (NTAPI* NtUserOpenWindowStation_I)(
                    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
                    _In_ ACCESS_MASK DesiredAccess
                    );
                HANDLE windowStationHandle;

                if (PhBeginInitOnce(&initOnce))
                {
                    NtUserOpenWindowStation_I = PhGetModuleProcAddress(L"win32u.dll", "NtUserOpenWindowStation");
                    PhEndInitOnce(&initOnce);
                }

                if (NtUserOpenWindowStation_I)
                {
                    if (windowStationHandle = NtUserOpenWindowStation_I(&objectAttributes, DesiredAccess))
                    {
                        *Handle = windowStationHandle;
                        status = STATUS_SUCCESS;
                    }
                    else
                    {
                        status = PhGetLastWin32ErrorAsNtStatus();
                    }
                }
            }
            break;
        case EtObjectFilterPort:
            {
                if (!NT_SUCCESS(status = EtObjectManagerOpenRealObject(Handle, Context, DesiredAccess, NULL)))
                {
                    PPH_STRING clientName;
                    HANDLE portHandle;

                    clientName = EtGetObjectFullPath(Context->CurrentPath, Context->Object->Name);
                    PhStringRefToUnicodeString(&clientName->sr, &objectName);

                    if (NT_SUCCESS(status = PhFilterConnectCommunicationPort(
                        &clientName->sr,
                        0,
                        NULL,
                        0,
                        NULL,
                        &portHandle
                        )))
                    {
                        *Handle = portHandle;
                        status = STATUS_NOT_ALL_ASSIGNED;
                    }

                    PhDereferenceObject(clientName);
                }
            }
            break;
        case EtObjectMemoryPartition:
            {
                static PH_INITONCE initOnce = PH_INITONCE_INIT;
                static NTSTATUS (NTAPI *NtOpenPartition_I)(
                    _Out_ PHANDLE PartitionHandle,
                    _In_ ACCESS_MASK DesiredAccess,
                    _In_ POBJECT_ATTRIBUTES ObjectAttributes
                    );

                if (PhBeginInitOnce(&initOnce))
                {
                    NtOpenPartition_I = PhGetModuleProcAddress(L"ntdll.dll", "NtOpenPartition");
                    PhEndInitOnce(&initOnce);
                }

                if (NtOpenPartition_I)
                {
                    status = NtOpenPartition_I(Handle, DesiredAccess, &objectAttributes);
                }
            }
            break;
        case EtObjectCpuPartition:
            {
                static PH_INITONCE initOnce = PH_INITONCE_INIT;
                static NTSTATUS(NTAPI * NtOpenCpuPartition_I)(
                    _Out_ PHANDLE CpuPartitionHandle,
                    _In_ ACCESS_MASK DesiredAccess,
                    _In_ POBJECT_ATTRIBUTES ObjectAttributes
                    );

                if (PhBeginInitOnce(&initOnce))
                {
                    NtOpenCpuPartition_I = PhGetModuleProcAddress(L"ntdll.dll", "NtOpenCpuPartition");
                    PhEndInitOnce(&initOnce);
                }

                if (NtOpenCpuPartition_I)
                {
                    status = NtOpenCpuPartition_I(Handle, DesiredAccess, &objectAttributes);
                }
            }
            break;
        default:
            break;
    }

    NtClose(directoryHandle);

    return status;
}

// Open real ALPC port by duplicating its "Connection" handle from the process that created the port
// https://github.com/zodiacon/ObjectExplorer/blob/master/ObjExp/ObjectManager.cpp#L191
// Open real FilterConnectionPort
// Open real \REGISTRY key
//
// If OwnerProcessId is specified and handle can't be duplicated returns UniqueProcessId and HandleValue instead.
// This allows to extract object information through driver APIs even for objects with kernel only access (OBJ_KERNEL_HANDLE).
NTSTATUS EtObjectManagerOpenRealObject(
    _Out_ PHANDLE Handle,
    _In_ PHANDLE_OPEN_CONTEXT Context,
    _In_ ACCESS_MASK DesiredAccess,
    _Inout_opt_ PHANDLE OwnerProcessId
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static ULONG alpcPortTypeIndex;
    static ULONG filterPortIndex;
    static ULONG keyTypeIndex;
    static ULONG sectionIndex;
    static ULONG winStaIndex;

    *Handle = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        alpcPortTypeIndex = PhGetObjectTypeNumberZ(L"ALPC Port");
        filterPortIndex = PhGetObjectTypeNumberZ(L"FilterConnectionPort");
        keyTypeIndex = PhGetObjectTypeNumberZ(L"Key");
        sectionIndex = PhGetObjectTypeNumberZ(L"Section");
        winStaIndex = PhGetObjectTypeNumberZ(L"WindowStation");
        PhEndInitOnce(&initOnce);
    }

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PPH_STRING fullName;
    ULONG targetIndex;
    ULONG i;
    PSYSTEM_HANDLE_INFORMATION_EX handles;
    PPH_HASHTABLE processHandleHashtable;
    PVOID* processHandlePtr;
    HANDLE processHandle;
    PPH_KEY_VALUE_PAIR procEntry;

    // Use cached index for most common types for better performance
    switch (Context->Object->EtObjectType)
    {
        case EtObjectAlpcPort:
            targetIndex = alpcPortTypeIndex;
            break;
        case EtObjectFilterPort:
            targetIndex = filterPortIndex;
            break;
        case EtObjectKey:
            targetIndex = keyTypeIndex;
            break;
        case EtObjectSection:
            targetIndex = sectionIndex;
            break;
        case EtObjectWindowStation:
            targetIndex = winStaIndex;
            break;
        default:
            targetIndex = PhGetObjectTypeNumberZ(PhGetString(Context->Object->TypeName));
    }

    fullName = EtGetObjectFullPath(Context->CurrentPath, Context->Object->Name);

    if (NT_SUCCESS(PhEnumHandlesEx(&handles)))
    {
        processHandleHashtable = PhCreateSimpleHashtable(8);

        for (i = 0; i < handles->NumberOfHandles; i++)
        {
            PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handleInfo = &handles->Handles[i];
            HANDLE objectHandle;
            PPH_STRING objectName;

            if (handleInfo->ObjectTypeIndex == targetIndex)
            {
                // Open a handle to the process if we don't already have one.
                processHandlePtr = PhFindItemSimpleHashtable(
                    processHandleHashtable,
                    (PVOID)handleInfo->UniqueProcessId
                    );

                if (processHandlePtr)
                {
                    processHandle = *processHandlePtr;
                }
                else
                {
                    if (NT_SUCCESS(PhOpenProcess(
                        &processHandle,
                        (KsiLevel() >= KphLevelMed ? PROCESS_QUERY_LIMITED_INFORMATION : PROCESS_DUP_HANDLE),
                        (HANDLE)handleInfo->UniqueProcessId
                        )))
                    {
                        PhAddItemSimpleHashtable(
                            processHandleHashtable,
                            (PVOID)handleInfo->UniqueProcessId,
                            processHandle
                            );
                    }
                    else
                    {
                        continue;
                    }
                }

                if (NT_SUCCESS(PhGetHandleInformation(
                    processHandle,
                    (HANDLE)handleInfo->HandleValue,
                    handleInfo->ObjectTypeIndex,
                    NULL,
                    NULL,
                    &objectName,
                    NULL
                    )))
                {
                    if (PhEqualString(objectName, fullName, TRUE))
                    {
                        NTSTATUS subStatus;

                        if (!NT_SUCCESS(subStatus = EtDuplicateHandleFromProcessEx(
                            &objectHandle,
                            DesiredAccess,
                            (HANDLE)handleInfo->UniqueProcessId,
                            (HANDLE)handleInfo->HandleValue
                            )))
                        {
                            subStatus = EtDuplicateHandleFromProcessEx(
                                &objectHandle,
                                0,
                                (HANDLE)handleInfo->UniqueProcessId,
                                (HANDLE)handleInfo->HandleValue
                                );
                        }

                        if (NT_SUCCESS(subStatus))
                        {
                            *Handle = objectHandle;
                            status = STATUS_SUCCESS;
                        }
                        else if (OwnerProcessId)
                        {
                            *OwnerProcessId = (HANDLE)handleInfo->UniqueProcessId;
                            *Handle = (HANDLE)handleInfo->HandleValue;
                            status = STATUS_SUCCESS;
                        }

                        PhDereferenceObject(objectName);
                        break;
                    }

                    PhDereferenceObject(objectName);
                }
            }
        }

        i = 0;
        while (PhEnumHashtable(processHandleHashtable, &procEntry, &i))
            NtClose(procEntry->Value);

        PhDereferenceObject(processHandleHashtable);
        PhFree(handles);
    }

    PhDereferenceObject(fullName);

    return status;
}

NTSTATUS EtObjectManagerHandleOpenCallback(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PVOID Context
    )
{
    NTSTATUS status;

    if (NT_SUCCESS(status = EtObjectManagerOpenHandle(Handle, Context, DesiredAccess, OBJECT_OPENSOURCE_ALPCPORT))) // HACK for \REGISTRY permissions
        return status;

    return EtObjectManagerOpenRealObject(Handle, Context, DesiredAccess, NULL);
}

NTSTATUS EtObjectManagerHandleCloseCallback(
    _In_ PVOID Context
    )
{
    PHANDLE_OPEN_CONTEXT context = Context;

    if (context->CurrentPath)
    {
        PhClearReference(&context->CurrentPath);
    }
    if (context->Object)
    {
        PhClearReference(&context->Object->Name);
        PhClearReference(&context->Object->TypeName);
        PhFree(context->Object);
    }

    PhFree(context);
    return STATUS_SUCCESS;
}

INT NTAPI WinObjNameCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    POBJECT_ENTRY item1 = Item1;
    POBJECT_ENTRY item2 = Item2;

    return PhCompareStringZ(PhGetStringOrEmpty(item1->Name), PhGetStringOrEmpty(item2->Name), TRUE);
}

INT NTAPI WinObjTypeCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    POBJECT_ENTRY item1 = Item1;
    POBJECT_ENTRY item2 = Item2;

    return PhCompareStringZ(PhGetStringOrEmpty(item1->TypeName), PhGetStringOrEmpty(item2->TypeName), TRUE);
}

INT NTAPI WinObjTargetCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    POBJECT_ENTRY item1 = Item1;
    POBJECT_ENTRY item2 = Item2;

    return PhCompareStringZ(PhGetStringOrEmpty(item1->Target), PhGetStringOrEmpty(item2->Target), TRUE);
}

INT NTAPI WinObjTriStateCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    POBJECT_CONTEXT context = Context;

    assert(context);

    return PhFindItemList(context->CurrentDirectoryList, Item1) - PhFindItemList(context->CurrentDirectoryList, Item2);
}

NTSTATUS EtObjectManagerGetHandleInfoEx(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE ObjectHandle,
    _Out_ PVOID* ObjectAddres,
    _Out_ PULONG TypeIndex
    )
{
    NTSTATUS status = STATUS_INVALID_HANDLE;
    PVOID buffer = NULL;
    ULONG i;

    if (KsiLevel() >= KphLevelMed)
    {
        PKPH_PROCESS_HANDLE_INFORMATION handles;

        if (NT_SUCCESS(KsiEnumerateProcessHandles(ProcessHandle, &handles)))
        {
            buffer = handles;

            for (i = 0; i < handles->HandleCount; i++)
            {
                if (handles->Handles[i].Handle == ObjectHandle)
                {
                    *ObjectAddres = handles->Handles[i].Object;
                    *TypeIndex = handles->Handles[i].ObjectTypeIndex;
                    status = STATUS_SUCCESS;
                    break;
                }
            }
        }
    }
    else
    {
        PSYSTEM_HANDLE_INFORMATION_EX handles;

        if (NT_SUCCESS(PhEnumHandlesEx(&handles)))
        {
            buffer = handles;

            for (i = 0; i < handles->NumberOfHandles; i++)
            {
                if ((HANDLE)handles->Handles[i].UniqueProcessId == ProcessId &&
                    (HANDLE)handles->Handles[i].HandleValue == ObjectHandle)
                {
                    *ObjectAddres = handles->Handles[i].Object;
                    *TypeIndex = handles->Handles[i].ObjectTypeIndex;
                    status = STATUS_SUCCESS;
                    break;
                }
            }
        }
    }

    if (buffer)
        PhFree(buffer);
    return status;
}

VOID NTAPI EtpObjectManagerObjectProperties(
    _In_ HWND hwndDlg,
    _In_ POBJECT_CONTEXT Context,
    _In_ POBJECT_ENTRY Entry
    )
{
    NTSTATUS status;
    NTSTATUS subStatus = STATUS_UNSUCCESSFUL;
    HANDLE objectHandle = NULL;
    HANDLE_OPEN_CONTEXT objectContext;
    HANDLE processId = NtCurrentProcessId();
    HANDLE processHandle = NtCurrentProcess();

    objectContext.CurrentPath = PhReferenceObject(Context->CurrentPath);
    objectContext.Object = Entry;

    if (Entry->EtObjectType == EtObjectDirectory)
        objectContext.Object->TypeName = PhCreateString2(&DirectoryObjectType);

    PhSetCursor(PhLoadCursor(NULL, IDC_WAIT));

    // Try to open with WRITE_DAC to allow change security from "Security" page
    if (!NT_SUCCESS(status = EtObjectManagerOpenHandle(&objectHandle, &objectContext, READ_CONTROL | WRITE_DAC, OBJECT_OPENSOURCE_ALL)))
    {
        // Can open KernelOnlyAccess object if any opened handles for this object exists (ex. \PowerPort, \Win32kCrossSessionGlobals)
        status = EtObjectManagerOpenRealObject(&objectHandle, &objectContext, READ_CONTROL | WRITE_DAC, &processId);
    }

    PPH_HANDLE_ITEM handleItem;
    SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX HandleInfo = { 0 };
    OBJECT_BASIC_INFORMATION objectInfo;

    handleItem = PhCreateHandleItem(&HandleInfo);
    handleItem->Handle = objectHandle;
    EtObjectManagerTimeCached.QuadPart = 0;

    switch (Entry->EtObjectType)
    {
        case EtObjectDirectory:
            handleItem->TypeName = Entry->TypeName = objectContext.Object->TypeName;
            break;
        default:
            handleItem->TypeName = PhReferenceObject(Entry->TypeName);
            break;
    }

    if (Entry->EtObjectType == EtObjectDirectory)
        handleItem->BestObjectName = PhReferenceObject(Context->CurrentPath);
    else
        handleItem->BestObjectName = EtGetObjectFullPath(Context->CurrentPath, Entry->Name);

    handleItem->ObjectName = PhReferenceObject(handleItem->BestObjectName);

    if (NT_SUCCESS(status))
    {
        if (processId != NtCurrentProcessId())
        {
            status = PhOpenProcess(
                &processHandle,
                PROCESS_QUERY_LIMITED_INFORMATION,
                processId
                );
        }

        if (NT_SUCCESS(status))
        {
            // Get object address and type index
            if (!NT_SUCCESS(EtObjectManagerGetHandleInfoEx(processId, processHandle, objectHandle, &handleItem->Object, &handleItem->TypeIndex)))
                handleItem->TypeIndex = PhGetObjectTypeNumber(&Entry->TypeName->sr);

            // Show only real source object address
            if (status == STATUS_NOT_ALL_ASSIGNED)
            {
                handleItem->Object = 0;
                if (Entry->EtObjectType == EtObjectDevice)
                    handleItem->TypeIndex = PhGetObjectTypeNumber(&Entry->TypeName->sr);
            }

            if (KsiLevel() >= KphLevelMed)
            {
                KPH_OBJECT_ATTRIBUTES_INFORMATION attribInformation;

                if (NT_SUCCESS(subStatus = KphQueryInformationObject(
                    processHandle,
                    objectHandle,
                    KphObjectAttributesInformation,
                    &attribInformation,
                    sizeof(attribInformation),
                    NULL
                    )))
                {
                    if (attribInformation.ExclusiveObject)
                        handleItem->Attributes |= OBJ_EXCLUSIVE;
                    if (attribInformation.PermanentObject)
                        handleItem->Attributes |= OBJ_PERMANENT;
                    if (attribInformation.KernelObject)
                        handleItem->Attributes |= OBJ_KERNEL_HANDLE;
                    if (attribInformation.KernelOnlyAccess)
                        handleItem->Attributes |= PH_OBJ_KERNEL_ACCESS_ONLY;
                }
            }

            if (NT_SUCCESS(status = PhGetHandleInformation(
                processHandle,
                objectHandle,
                ULONG_MAX,
                &objectInfo,
                NULL,
                NULL,
                NULL
                )))
            {
                // We will remove access row in EtHandlePropertiesWindowInitialized callback
                //handleItem->GrantedAccess = objectInfo.GrantedAccess;
                if (!NT_SUCCESS(subStatus))
                    handleItem->Attributes = objectInfo.Attributes;
                EtObjectManagerTimeCached = objectInfo.CreationTime;
            }

            if (processHandle != NtCurrentProcess())
                NtClose(processHandle);
        }

        if (processId == NtCurrentProcessId())
        {
            // HACK for \REGISTRY permissions
            if (Entry->EtObjectType == EtObjectKey && PhEqualString2(handleItem->BestObjectName, L"\\REGISTRY", TRUE))
            {
                HANDLE registryHandle;
                if (NT_SUCCESS(EtObjectManagerOpenHandle(&registryHandle, &objectContext, READ_CONTROL | WRITE_DAC, 0)))
                {
                    NtClose(objectHandle);
                    handleItem->Handle = objectHandle = registryHandle;
                }
            }

            PhReferenceObject(EtObjectManagerOwnHandles);
            PhAddItemList(EtObjectManagerOwnHandles, objectHandle);
        }
    }
    else
    {
        handleItem->TypeIndex = PhGetObjectTypeNumber(&Entry->TypeName->sr);
    }

    EtObjectManagerPropIcon = PhImageListGetIcon(Context->ListImageList, Entry->EtObjectType, ILD_NORMAL | ILD_TRANSPARENT);

    // Object Manager plugin window
    PhShowHandlePropertiesEx(hwndDlg, processId, handleItem, PluginInstance, PhGetString(Entry->TypeName));

    PhDereferenceObject(Context->CurrentPath);
}

VOID NTAPI EtpObjectManagerOpenTarget(
    _In_ HWND hwndDlg,
    _In_ POBJECT_CONTEXT Context,
    _In_ PPH_STRING Target
    )
{
    PH_STRINGREF directoryPart = PhGetStringRef(NULL);
    PH_STRINGREF pathPart;
    PH_STRINGREF remainingPart;
    PH_STRINGREF namePart = PhGetStringRef(NULL);
    HTREEITEM selectedTreeItem;
    PPH_STRING targetPath;

    targetPath = PhReferenceObject(Target);
    remainingPart = PhGetStringRef(targetPath);
    selectedTreeItem = Context->RootTreeObject;

    PhSplitStringRefAtLastChar(&remainingPart, OBJ_NAME_PATH_SEPARATOR, &pathPart, &namePart);

    // Check if target directory is equal to current
    if (!Context->CurrentPath || !PhEqualStringRef(&pathPart, &Context->CurrentPath->sr, TRUE))
    {
        if (PhStartsWithStringRef(&remainingPart, &EtObjectManagerRootDirectoryObject, TRUE))
        {
            while (remainingPart.Length != 0)
            {
                PhSplitStringRefAtChar(&remainingPart, OBJ_NAME_PATH_SEPARATOR, &directoryPart, &remainingPart);

                if (directoryPart.Length != 0)
                {
                    HTREEITEM directoryItem = EtTreeViewFindItem(
                        Context->TreeViewHandle,
                        selectedTreeItem,
                        &directoryPart
                        );

                    if (directoryItem)
                    {
                        PhSetWindowText(Context->SearchBoxHandle, L"");

                        TreeView_SelectItem(Context->TreeViewHandle, directoryItem);
                        selectedTreeItem = directoryItem;
                    }
                }
            }
        }

        if (!Context->CurrentPath)
        {
            goto cleanup_exit;
        }

        // If we did jump to new tree target, then focus to listview target item
        if (selectedTreeItem != Context->RootTreeObject && directoryPart.Length > 0)    // HACK
        {
            LVFINDINFO findinfo;
            findinfo.psz = directoryPart.Buffer;
            findinfo.flags = LVFI_STRING;

            INT item = ListView_FindItem(Context->ListViewHandle, INT_ERROR, &findinfo);

            // Navigate to target object
            if (item != INT_ERROR)
            {
                ListView_SetItemState(Context->ListViewHandle, item, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                ListView_EnsureVisible(Context->ListViewHandle, item, TRUE);
            }
        }
        else    // browse to target in explorer
        {
            if (!PhIsNullOrEmptyString(Target) &&
                (PhDoesFileExist(&Target->sr) || PhDoesFileExistWin32(Target->Buffer)))
            {
                PhShellExecuteUserString(
                    hwndDlg,
                    L"FileBrowseExecutable",
                    PhGetString(Target),
                    FALSE,
                    L"Make sure the Explorer executable file is present."
                    );
            }
            else
            {
                PhShowStatus(hwndDlg, L"Unable to locate the target.", STATUS_NOT_FOUND, 0);
            }
        }
    }
    else    // Same directory
    {
        LVFINDINFO findinfo;
        findinfo.psz = namePart.Buffer;
        findinfo.flags = LVFI_STRING;

        PPH_STRING curentFilter = PH_AUTO(PhGetWindowText(Context->SearchBoxHandle));
        if (!PhIsNullOrEmptyString(curentFilter))
        {
            PhSetWindowText(Context->SearchBoxHandle, L"");
            EtpObjectManagerSearchControlCallback(0, Context);
        }

        INT item = ListView_FindItem(Context->ListViewHandle, INT_ERROR, &findinfo);

        if (item != INT_ERROR)
        {
            ListView_SetItemState(Context->ListViewHandle, item, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            ListView_EnsureVisible(Context->ListViewHandle, item, TRUE);
        }
    }

cleanup_exit:
    PhDereferenceObject(targetPath);
}

VOID NTAPI EtpObjectManagerRefresh(
    _In_ HWND hwndDlg,
    _In_ POBJECT_CONTEXT Context
    )
{
    PPH_STRING selectedPath = NULL;
    BOOLEAN rootWasSelected = Context->SelectedTreeItem == Context->RootTreeObject;
    HWND countControl = GetDlgItem(hwndDlg, IDC_OBJMGR_COUNT);
    PPH_STRING oldSelect = NULL;
    POBJECT_ENTRY* listviewItems;
    ULONG numberOfItems;

    if (!rootWasSelected)
        selectedPath = EtGetSelectedTreeViewPath(Context);

    PhGetSelectedListViewItemParams(Context->ListViewHandle, &listviewItems, &numberOfItems);
    if (numberOfItems != 0)
        oldSelect = PhReferenceObject(listviewItems[0]->Name);

    SendMessage(Context->TreeViewHandle, WM_SETREDRAW, FALSE, 0);
    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);

    EtObjectManagerFreeListViewItems(Context);
    ListView_DeleteAllItems(Context->ListViewHandle);
    EtCleanupTreeViewItemParams(Context, Context->RootTreeObject);
    TreeView_DeleteAllItems(Context->TreeViewHandle);

    Context->RootTreeObject = EtTreeViewAddItem(Context->TreeViewHandle, TVI_ROOT, TRUE, &EtObjectManagerRootDirectoryObject);

    EtTreeViewEnumDirectoryObjects(
        Context->TreeViewHandle,
        Context->RootTreeObject,
        EtObjectManagerRootDirectoryObject
        );

    if (rootWasSelected)
    {
        TreeView_SelectItem(Context->TreeViewHandle, Context->RootTreeObject);
    }
    else
    {
        PH_STRINGREF directoryPart;
        PH_STRINGREF remainingPart;
        HTREEITEM selectedTreeItem = NULL;

        Context->DisableSelChanged = TRUE;

        directoryPart = PhGetStringRef(NULL);
        remainingPart = PhGetStringRef(selectedPath);
        selectedTreeItem = Context->RootTreeObject;

        while (remainingPart.Length != 0)
        {
            PhSplitStringRefAtChar(&remainingPart, OBJ_NAME_PATH_SEPARATOR, &directoryPart, &remainingPart);

            if (directoryPart.Length != 0)
            {
                HTREEITEM directoryItem = EtTreeViewFindItem(
                    Context->TreeViewHandle,
                    selectedTreeItem,
                    &directoryPart
                    );

                if (directoryItem)
                {
                    TreeView_SelectItem(Context->TreeViewHandle, directoryItem);
                    selectedTreeItem = directoryItem;
                }
            }
        }

        PhDereferenceObject(selectedPath);

        // Will reapply filter and sort
        if (selectedTreeItem)
            EtpObjectManagerChangeSelection(Context);

        Context->DisableSelChanged = FALSE;
    }

    // Reapply old selection
    EtpObjectManagerSortAndSelectOld(Context, oldSelect);

    SendMessage(Context->TreeViewHandle, WM_SETREDRAW, TRUE, 0);
    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

VOID NTAPI EtpObjectManagerOpenSecurity(
    _In_ HWND hwndDlg,
    _In_ POBJECT_CONTEXT Context,
    _In_ POBJECT_ENTRY Entry
    )
{
    PHANDLE_OPEN_CONTEXT objectContext;

    objectContext = PhAllocateZero(sizeof(HANDLE_OPEN_CONTEXT));
    objectContext->CurrentPath = PhReferenceObject(Context->CurrentPath);
    objectContext->Object = PhAllocateZero(sizeof(ET_OBJECT_ENTRY));
    objectContext->Object->Name = PhReferenceObject(Entry->Name);
    objectContext->Object->EtObjectType = Entry->EtObjectType;

    switch (Entry->EtObjectType)
    {
        case EtObjectDirectory:
            objectContext->Object->TypeName = PhCreateString2(&DirectoryObjectType);
            break;
        default:
            objectContext->Object->TypeName = PhReferenceObject(Entry->TypeName);
            break;
    }

    PhEditSecurity(
        !!PhGetIntegerSetting(L"ForceNoParent") ? NULL : Context->WindowHandle,
        PhGetString(objectContext->Object->Name),
        PhGetString(objectContext->Object->TypeName),
        EtObjectManagerHandleOpenCallback,
        EtObjectManagerHandleCloseCallback,
        objectContext
        );
}

VOID NTAPI EtpObjectManagerSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    POBJECT_CONTEXT context = Context;
    POBJECT_ENTRY* listviewItems;
    ULONG numberOfItems;
    PPH_STRING oldSelect = NULL;

    assert(context);

    PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);
    if (numberOfItems != 0)
        oldSelect = PhReferenceObject(listviewItems[0]->Name);

    ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);
    ListView_DeleteAllItems(context->ListViewHandle);

    PPH_LIST Array = context->CurrentDirectoryList;

    for (ULONG i = 0; i < Array->Count; i++)
    {
        INT index;
        POBJECT_ENTRY entry = Array->Items[i];

        if (entry->Name != NULL)
        {
            if (!MatchHandle ||
                PhSearchControlMatch(MatchHandle, &entry->Name->sr) ||
                PhSearchControlMatch(MatchHandle, &entry->TypeName->sr) ||
                entry->Target && PhSearchControlMatch(MatchHandle, &entry->Target->sr))
            {
                index = PhAddListViewItem(context->ListViewHandle, MAXINT, LPSTR_TEXTCALLBACK, entry);
                PhSetListViewItemImageIndex(context->ListViewHandle, index, entry->EtObjectType);
            }
        }
    }

    // Keep current sort and selection after apply new filter
    EtpObjectManagerSortAndSelectOld(context, oldSelect);

    ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);

    WCHAR string[PH_INT32_STR_LEN_1];
    PhPrintUInt32(string, ListView_GetItemCount(context->ListViewHandle));
    PhSetWindowText(GetDlgItem(context->WindowHandle, IDC_OBJMGR_COUNT), string);
}

VOID NTAPI EtpObjectManagerSortAndSelectOld(
    _In_ POBJECT_CONTEXT Context,
    _In_opt_ PPH_STRING OldSelection
    )
{
    ULONG sortColumn;
    PH_SORT_ORDER sortOrder;

    ExtendedListView_GetSort(Context->ListViewHandle, &sortColumn, &sortOrder);
    if (sortOrder != NoSortOrder)
        ExtendedListView_SortItems(Context->ListViewHandle);

    // Reselect previously active listview item
    if (OldSelection)
    {
        LVFINDINFO findinfo;
        findinfo.psz = OldSelection->Buffer;
        findinfo.flags = LVFI_STRING;

        INT item = ListView_FindItem(Context->ListViewHandle, INT_ERROR, &findinfo);

        if (item != INT_ERROR)
        {
            ListView_SetItemState(Context->ListViewHandle, item, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            ListView_EnsureVisible(Context->ListViewHandle, item, TRUE);
        }

        PhDereferenceObject(OldSelection);
    }
}

VOID NTAPI EtpObjectManagerChangeSelection(
    _In_ POBJECT_CONTEXT Context
    )
{
    Context->SelectedTreeItem = TreeView_GetSelection(Context->TreeViewHandle);

    if (!Context->DisableSelChanged)
        ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    ListView_DeleteAllItems(Context->ListViewHandle);
    EtObjectManagerFreeListViewItems(Context);

    EtEnumCurrentDirectoryObjects(Context);

    if (!Context->DisableSelChanged)
        ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

INT_PTR CALLBACK WinObjDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    POBJECT_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(ET_OBJECT_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
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
            context->ParentWindowHandle = (HWND)lParam;
            context->TreeViewHandle = GetDlgItem(hwndDlg, IDC_OBJMGR_TREE);
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_OBJMGR_LIST);
            context->SearchBoxHandle = GetDlgItem(hwndDlg, IDC_OBJMGR_SEARCH);
            context->CurrentDirectoryList = PhCreateList(100);
            if (!EtObjectManagerOwnHandles || !PhReferenceObjectSafe(EtObjectManagerOwnHandles))
                EtObjectManagerOwnHandles = PhCreateList(10);
            if (!EtObjectManagerPropWindows || !PhReferenceObjectSafe(EtObjectManagerPropWindows))
                EtObjectManagerPropWindows = PhCreateSimpleHashtable(10);

            PhSetApplicationWindowIcon(hwndDlg);

            EtInitializeTreeImages(context);
            EtInitializeListImages(context);

            PhSetControlTheme(context->TreeViewHandle, L"explorer");
            TreeView_SetExtendedStyle(context->TreeViewHandle, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
            TreeView_SetImageList(context->TreeViewHandle, context->TreeImageList, TVSIL_NORMAL);
            context->RootTreeObject = EtTreeViewAddItem(context->TreeViewHandle, TVI_ROOT, TRUE, &EtObjectManagerRootDirectoryObject);

            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhSetListViewStyle(context->ListViewHandle, TRUE, FALSE);
            PhSetExtendedListView(context->ListViewHandle);
            ListView_SetImageList(context->ListViewHandle, context->ListImageList, LVSIL_SMALL);
            PhAddListViewColumn(context->ListViewHandle, ETOBLVC_NAME, ETOBLVC_NAME, ETOBLVC_NAME, LVCFMT_LEFT, 445, L"Name");
            PhAddListViewColumn(context->ListViewHandle, ETOBLVC_TYPE, ETOBLVC_TYPE, ETOBLVC_TYPE, LVCFMT_LEFT, 150, L"Type");
            PhAddListViewColumn(context->ListViewHandle, ETOBLVC_TARGET, ETOBLVC_TARGET, ETOBLVC_TARGET, LVCFMT_LEFT, 200, L"Target");
            PhLoadListViewColumnsFromSetting(SETTING_NAME_OBJMGR_COLUMNS, context->ListViewHandle);

            PH_INTEGER_PAIR sortSettings;
            sortSettings = PhGetIntegerPairSetting(SETTING_NAME_OBJMGR_LIST_SORT);

            ExtendedListView_SetContext(context->ListViewHandle, context);
            //ExtendedListView_SetSortFast(context->ListViewHandle, TRUE);
            ExtendedListView_SetTriState(context->ListViewHandle, TRUE);
            ExtendedListView_SetSort(context->ListViewHandle, (ULONG)sortSettings.X, (PH_SORT_ORDER)sortSettings.Y);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 0, WinObjNameCompareFunction);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 1, WinObjTypeCompareFunction);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 2, WinObjTargetCompareFunction);
            ExtendedListView_SetTriStateCompareFunction(context->ListViewHandle, WinObjTriStateCompareFunction);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->TreeViewHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, context->SearchBoxHandle, NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_OBJMGR_PATH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_OBJMGR_COUNT_L), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_OBJMGR_COUNT), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_REFRESH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            PhCreateSearchControl(
                hwndDlg,
                context->SearchBoxHandle,
                L"Search Objects (Ctrl+K)",
                EtpObjectManagerSearchControlCallback,
                context
                );

            if (PhGetIntegerPairSetting(SETTING_NAME_OBJMGR_WINDOW_POSITION).X != 0)
                PhLoadWindowPlacementFromSetting(SETTING_NAME_OBJMGR_WINDOW_POSITION, SETTING_NAME_OBJMGR_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, context->ParentWindowHandle);

            EtTreeViewEnumDirectoryObjects(
                context->TreeViewHandle,
                context->RootTreeObject,
                EtObjectManagerRootDirectoryObject
                );

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

            PPH_STRING Target = PH_AUTO(PhGetStringSetting(SETTING_NAME_OBJMGR_LAST_PATH));

            EtpObjectManagerOpenTarget(hwndDlg, context, Target);

            SendMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM)context->TreeViewHandle, TRUE);
            ExtendedListView_SetColumnWidth(context->ListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
        }
        break;
    case WM_DESTROY:
        {
            PPH_STRING CurrentPath = PhReferenceObject(context->CurrentPath);
            PH_INTEGER_PAIR sortSettings;
            ULONG sortColumn;
            PH_SORT_ORDER sortOrder;

            EtObjectManagerFreeListViewItems(context);
            EtCleanupTreeViewItemParams(context, context->RootTreeObject);

            if (context->TreeImageList)
                PhImageListDestroy(context->TreeImageList);
            if (context->ListImageList)
                PhImageListDestroy(context->ListImageList);
            if (context->CurrentDirectoryList)
                PhDereferenceObject(context->CurrentDirectoryList);
            if (EtObjectManagerOwnHandles)
                PhDereferenceObject(EtObjectManagerOwnHandles);
            if (EtObjectManagerPropWindows)
                PhDereferenceObject(EtObjectManagerPropWindows);

            PhSaveWindowPlacementToSetting(SETTING_NAME_OBJMGR_WINDOW_POSITION, SETTING_NAME_OBJMGR_WINDOW_SIZE, hwndDlg);
            PhSaveListViewColumnsToSetting(SETTING_NAME_OBJMGR_COLUMNS, context->ListViewHandle);

            ExtendedListView_GetSort(context->ListViewHandle, &sortColumn, &sortOrder);
            sortSettings.X = sortColumn;
            sortSettings.Y = sortOrder;
            PhSetIntegerPairSetting(SETTING_NAME_OBJMGR_LIST_SORT, sortSettings);

            PhSetStringSetting(SETTING_NAME_OBJMGR_LAST_PATH, PhGetString(CurrentPath));
            PhDereferenceObject(CurrentPath);

            PhDeleteLayoutManager(&context->LayoutManager);

            EtObjectManagerDialogHandle = NULL;

            PostQuitMessage(0);
        }
        break;
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_PH_SHOW_DIALOG:
        {
            if (IsMinimized(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
            ExtendedListView_SetColumnWidth(context->ListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDCANCEL:
                    DestroyWindow(hwndDlg);
                    break;
                case IDC_REFRESH:
                    EtpObjectManagerRefresh(hwndDlg, context);
                    break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            PhHandleListViewNotifyBehaviors(lParam, context->ListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);

            switch (header->code)
            {
            case TVN_SELCHANGED:
                if (!context->DisableSelChanged &&
                    header->hwndFrom == context->TreeViewHandle)
                {
                    EtpObjectManagerChangeSelection(context);
                }
                break;
            case TVN_GETDISPINFO:
                {
                    NMTVDISPINFO* dispInfo = (NMTVDISPINFO*)header;

                    if (header->hwndFrom == context->TreeViewHandle &&
                        FlagOn(dispInfo->item.mask, TVIF_TEXT))
                    {
                        //wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, entry->Name->Buffer, _TRUNCATE);
                        dispInfo->item.pszText = PhGetString((PPH_STRING)dispInfo->item.lParam);
                    }
                }
                break;
            case LVN_GETDISPINFO:
                {
                    NMLVDISPINFO* dispInfo = (NMLVDISPINFO*)header;

                    if (header->hwndFrom == context->ListViewHandle &&
                        FlagOn(dispInfo->item.mask, TVIF_TEXT))
                    {
                        POBJECT_ENTRY entry = (POBJECT_ENTRY)dispInfo->item.lParam;

                        switch (dispInfo->item.iSubItem)
                        {
                            case ETOBLVC_NAME:
                                dispInfo->item.pszText = PhGetString(entry->Name);
                                break;
                            case ETOBLVC_TYPE:
                                dispInfo->item.pszText = PhGetString(entry->TypeName);
                                break;
                            case ETOBLVC_TARGET:
                                dispInfo->item.pszText = !entry->TargetIsResolving ? PhGetString(entry->Target) : L"Resolving...";
                                break;
                        }
                    }
                }
                break;
            case LVN_ITEMCHANGED:
                {
                    LPNMLISTVIEW info = (LPNMLISTVIEW)header;

                    if (header->hwndFrom == context->ListViewHandle &&
                        info->uChanged & LVIF_STATE && info->uNewState & (LVIS_ACTIVATING | LVIS_FOCUSED))
                    {
                        POBJECT_ENTRY entry = (POBJECT_ENTRY)info->lParam;
                        PPH_STRING fullPath;

                        fullPath = PH_AUTO(EtGetObjectFullPath(context->CurrentPath, entry->Name));

                        PhSetWindowText(GetDlgItem(hwndDlg, IDC_OBJMGR_PATH), PhGetString(fullPath));
                    }
                }
                break;
            case NM_SETCURSOR:
                {
                    if (header->hwndFrom == context->TreeViewHandle)
                    {
                        PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                        return TRUE;
                    }
                }
                break;
            case NM_DBLCLK:
                {
                    LPNMITEMACTIVATE info = (LPNMITEMACTIVATE)header;
                    POBJECT_ENTRY entry;

                    if (header->hwndFrom == context->ListViewHandle &&
                        (entry = PhGetSelectedListViewItemParam(context->ListViewHandle)))
                    {
                        if (GetKeyState(VK_CONTROL) < 0)
                        {
                            EtpObjectManagerOpenSecurity(hwndDlg, context, entry);
                        }
                        else if (entry->EtObjectType == EtObjectSymLink && !(GetKeyState(VK_SHIFT) < 0))
                        {
                            if (!PhIsNullOrEmptyString(entry->Target))
                                EtpObjectManagerOpenTarget(hwndDlg, context, entry->Target);
                        }
                        else
                        {
                            EtpObjectManagerObjectProperties(hwndDlg, context, entry);
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_CONTEXTMENU:
        {
            PPH_EMENU menu;
            PPH_EMENU_ITEM item;
            POINT point;

            if ((HWND)wParam == context->ListViewHandle)
            {
                PVOID *listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                if (WindowFromPoint(point) == ListView_GetHeader(context->ListViewHandle))
                {
                    ULONG sortColumn;
                    PH_SORT_ORDER sortOrder;

                    ExtendedListView_GetSort(context->ListViewHandle, &sortColumn, &sortOrder);
                    if (sortOrder != NoSortOrder)
                    {
                        menu = PhCreateEMenu();
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_RESETSORT, L"&Reset sort", NULL, NULL), ULONG_MAX);

                        item = PhShowEMenu(
                            menu,
                            hwndDlg,
                            PH_EMENU_SHOW_LEFTRIGHT,
                            PH_ALIGN_LEFT | PH_ALIGN_TOP,
                            point.x,
                            point.y
                            );

                        if (item && item->Id == IDC_RESETSORT)
                        {
                            ExtendedListView_SetSort(context->ListViewHandle, 0, NoSortOrder);
                            ExtendedListView_SortItems(context->ListViewHandle);
                        }

                        PhDestroyEMenu(menu);
                    }
                    break;
                }

                PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);
                if (numberOfItems != 0)
                {
                    POBJECT_ENTRY entry = listviewItems[0];

                    menu = PhCreateEMenu();

                    BOOLEAN hasTarget = !PhIsNullOrEmptyString(entry->Target) && !entry->PdoDevice;
                    BOOLEAN isSymlink = entry->EtObjectType == EtObjectSymLink;
                    PPH_EMENU_ITEM propMenuItem;
                    PPH_EMENU_ITEM secMenuItem;
                    PPH_EMENU_ITEM gotoMenuItem = NULL;
                    PPH_EMENU_ITEM gotoMenuItem2 = NULL;
                    PPH_EMENU_ITEM copyPathMenuItem;

                    PhInsertEMenuItem(menu, propMenuItem = PhCreateEMenuItem(0, IDC_PROPERTIES,
                        !isSymlink ? L"Prope&rties\bEnter" : L"Prope&rties\bShift+Enter", NULL, NULL), ULONG_MAX);

                    if (isSymlink)
                    {
                        PhInsertEMenuItem(menu, gotoMenuItem = PhCreateEMenuItem(0, IDC_OPENLINK, L"&Open link\bEnter", NULL, NULL), 0);
                    }
                    else if (entry->EtObjectType == EtObjectDevice)
                    {
                        if (entry->TargetDrvLow && entry->TargetDrvUp && !PhEqualString(entry->TargetDrvLow, entry->TargetDrvUp, TRUE))
                        {
                            PhInsertEMenuItem(menu, gotoMenuItem = PhCreateEMenuItem(0, IDC_GOTODRIVER2, L"Go to &upper device driver", NULL, NULL), ULONG_MAX);
                            PhInsertEMenuItem(menu, gotoMenuItem2 = PhCreateEMenuItem(0, IDC_GOTODRIVER, L"&Go to lower device driver", NULL, NULL), ULONG_MAX);
                        }
                        else
                            PhInsertEMenuItem(menu, gotoMenuItem = PhCreateEMenuItem(0, IDC_GOTODRIVER, L"&Go to device driver", NULL, NULL), ULONG_MAX);
                    }
                    else if (entry->EtObjectType == EtObjectAlpcPort)
                    {
                        PhInsertEMenuItem(menu, gotoMenuItem = PhCreateEMenuItem(0, IDC_GOTOPROCESS, L"&Go to process...", NULL, NULL), ULONG_MAX);
                    }
                    else if (entry->EtObjectType == EtObjectMutant)
                    {
                        PhInsertEMenuItem(menu, gotoMenuItem = PhCreateEMenuItem(0, IDC_GOTOTHREAD, L"&Go to thread...", NULL, NULL), ULONG_MAX);
                    }
                    else if (entry->EtObjectType == EtObjectDriver)
                    {
                        PhInsertEMenuItem(menu, gotoMenuItem = PhCreateEMenuItem(0, IDC_OPENFILELOCATION, L"&Open file location", NULL, NULL), ULONG_MAX);
                    }

                    PhInsertEMenuItem(menu, secMenuItem = PhCreateEMenuItem(0, IDC_SECURITY, L"&Security\bCtrl+Enter", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy\bCtrl+C", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, copyPathMenuItem = PhCreateEMenuItem(0, IDC_COPYPATH, L"Copy &Full Name", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, IDC_COPYPATH, context->ListViewHandle);
                    PhSetFlagsEMenuItem(menu, isSymlink ? IDC_OPENLINK : IDC_PROPERTIES, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

                    if (numberOfItems > 1)
                    {
                        PhSetDisabledEMenuItem(propMenuItem);
                        if (gotoMenuItem) PhSetDisabledEMenuItem(gotoMenuItem);
                        if (gotoMenuItem2) PhSetDisabledEMenuItem(gotoMenuItem2);
                        PhSetDisabledEMenuItem(secMenuItem);
                        PhSetDisabledEMenuItem(copyPathMenuItem);
                    }
                    else if (!hasTarget)
                    {
                        if (gotoMenuItem) PhSetDisabledEMenuItem(gotoMenuItem);
                        if (gotoMenuItem2) PhSetDisabledEMenuItem(gotoMenuItem2);
                    }

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        if (!PhHandleCopyListViewEMenuItem(item))
                        {
                            switch (item->Id)
                            {
                            case IDC_PROPERTIES:
                                {
                                    EtpObjectManagerObjectProperties(hwndDlg, context, entry);
                                }
                                break;
                            case IDC_OPENLINK:
                                {
                                    EtpObjectManagerOpenTarget(hwndDlg, context, entry->Target);
                                }
                                break;
                            case IDC_GOTODRIVER:
                                {
                                    EtpObjectManagerOpenTarget(hwndDlg, context, entry->TargetDrvLow);
                                }
                                break;
                            case IDC_GOTODRIVER2:
                                {
                                    EtpObjectManagerOpenTarget(hwndDlg, context, entry->TargetDrvUp);
                                }
                                break;
                            case IDC_GOTOPROCESS:
                                {
                                    PPH_PROCESS_ITEM processItem;

                                    if (processItem = PhReferenceProcessItem(entry->TargetClientId.UniqueProcess))
                                    {
                                        SystemInformer_ShowProcessProperties(processItem);
                                        PhDereferenceObject(processItem);
                                    }
                                }
                                break;
                            case IDC_GOTOTHREAD:
                                {
                                    PPH_PROCESS_ITEM processItem;
                                    PPH_PROCESS_PROPCONTEXT propContext;

                                    if (processItem = PhReferenceProcessItem(entry->TargetClientId.UniqueProcess))
                                    {
                                        if (propContext = PhCreateProcessPropContext(NULL, processItem))
                                        {
                                            PhSetSelectThreadIdProcessPropContext(propContext, entry->TargetClientId.UniqueThread);
                                            PhShowProcessProperties(propContext);
                                            PhDereferenceObject(propContext);
                                        }

                                        PhDereferenceObject(processItem);
                                    }
                                }
                                break;
                            case IDC_OPENFILELOCATION:
                                if (!PhIsNullOrEmptyString(entry->Target) &&
                                    (PhDoesFileExist(&entry->Target->sr) || PhDoesFileExistWin32(entry->Target->Buffer)))
                                {
                                    PhShellExecuteUserString(
                                        hwndDlg,
                                        L"FileBrowseExecutable",
                                        PhGetString(entry->Target),
                                        FALSE,
                                        L"Make sure the Explorer executable file is present."
                                        );
                                }
                                else
                                {
                                    PhShowStatus(hwndDlg, L"Unable to locate the target.", STATUS_NOT_FOUND, 0);
                                }
                                break;
                            case IDC_SECURITY:
                                {
                                    EtpObjectManagerOpenSecurity(hwndDlg, context, entry);
                                }
                                break;
                            case IDC_COPY:
                                {
                                    PhCopyListView(context->ListViewHandle);
                                }
                                break;
                            case IDC_COPYPATH:
                                {
                                    PhSetClipboardString(hwndDlg, &PH_AUTO_T(PH_STRING, EtGetObjectFullPath(context->CurrentPath, entry->Name))->sr);
                                }
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }

                PhFree(listviewItems);
            }
            else if ((HWND)wParam == context->TreeViewHandle)
            {
                TVHITTESTINFO treeHitTest = { 0 };
                HTREEITEM treeItem;
                RECT treeWindowRect;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                GetWindowRect(context->TreeViewHandle, &treeWindowRect);
                treeHitTest.pt.x = point.x - treeWindowRect.left;
                treeHitTest.pt.y = point.y - treeWindowRect.top;

                if (treeItem = TreeView_HitTest(context->TreeViewHandle, &treeHitTest))
                {
                    TreeView_SelectItem(context->TreeViewHandle, treeItem);

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_PROPERTIES, L"Prope&rties\bShift+Enter", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_SECURITY, L"&Security\bCtrl+Enter", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPYPATH, L"Copy &Full Name", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, IDC_COPYPATH, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        PPH_STRING directory;

                        if (PhGetTreeViewItemParam(context->TreeViewHandle, context->SelectedTreeItem, &directory, NULL))
                        {
                            ET_OBJECT_ENTRY entry = { 0 };
                            entry.Name = directory;
                            entry.EtObjectType = EtObjectDirectory;

                            switch (item->Id)
                            {
                            case IDC_PROPERTIES:
                                {
                                    EtpObjectManagerObjectProperties(hwndDlg, context, &entry);
                                }
                                break;
                            case IDC_SECURITY:
                                {
                                    EtpObjectManagerOpenSecurity(hwndDlg, context, &entry);
                                }
                                break;
                            case IDC_COPY:
                                {
                                    PhSetClipboardString(hwndDlg, &directory->sr);
                                }
                                break;
                            case IDC_COPYPATH:
                                {
                                    PhSetClipboardString(hwndDlg, &context->CurrentPath->sr);
                                }
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }
            }
            else if ((HWND)wParam == hwndDlg)
            {
                HWND pathControl = GetDlgItem(hwndDlg, IDC_OBJMGR_PATH);
                POINT point;
                RECT pathRect;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                GetWindowRect(pathControl, &pathRect);
                InflateRect(&pathRect, 0, 3);

                if (PtInRect(&pathRect, point))
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPYPATH, L"Copy &Full Name", NULL, NULL), ULONG_MAX);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item && item->Id == IDC_COPYPATH)
                    {
                        PhSetClipboardString(hwndDlg, &PH_AUTO_T(PH_STRING, PhGetWindowText(pathControl))->sr);
                    }

                    PhDestroyEMenu(menu);
                }
            }
        }
        break;
    case WM_KEYDOWN:
    {
        switch (LOWORD(wParam))
        {
            case 'K':
                if (GetKeyState(VK_CONTROL) < 0)
                {
                    SetFocus(context->SearchBoxHandle);
                    return TRUE;
                }
                break;
            case VK_F5:
                {
                    EtpObjectManagerRefresh(hwndDlg, context);
                    return TRUE;
                }
                break;
            case VK_RETURN:
                if (GetFocus() == context->ListViewHandle)
                {
                    POBJECT_ENTRY* listviewItems;
                    ULONG numberOfItems;

                    PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);
                    if (numberOfItems == 1)
                    {
                        if (listviewItems[0]->EtObjectType == EtObjectSymLink && GetKeyState(VK_SHIFT) < 0)
                        {
                            EtpObjectManagerObjectProperties(hwndDlg, context, listviewItems[0]);
                        }
                        else if (GetKeyState(VK_CONTROL) < 0)
                        {
                            EtpObjectManagerOpenSecurity(hwndDlg, context, listviewItems[0]);
                        }
                        else
                        {
                            if (listviewItems[0]->EtObjectType == EtObjectSymLink)
                            {
                                if (!PhIsNullOrEmptyString(listviewItems[0]->Target))
                                    EtpObjectManagerOpenTarget(hwndDlg, context, listviewItems[0]->Target);
                                else
                                    break;
                            }
                            else
                            {
                                EtpObjectManagerObjectProperties(hwndDlg, context, listviewItems[0]);
                            }
                        }

                        return TRUE;
                    }
                }
                else if (GetFocus() == context->TreeViewHandle)
                {
                    PPH_STRING directory;
                    ET_OBJECT_ENTRY entry = { 0 };
                    entry.EtObjectType = EtObjectDirectory;

                    if (GetKeyState(VK_SHIFT) < 0)
                    {
                        if (PhGetTreeViewItemParam(context->TreeViewHandle, context->SelectedTreeItem, &directory, NULL))
                        {
                            entry.Name = directory;
                            EtpObjectManagerObjectProperties(hwndDlg, context, &entry);
                            return TRUE;
                        }
                    }
                    else if (GetKeyState(VK_CONTROL) < 0)
                    {
                        if (PhGetTreeViewItemParam(context->TreeViewHandle, context->SelectedTreeItem, &directory, NULL))
                        {
                            entry.Name = directory;
                            EtpObjectManagerOpenSecurity(hwndDlg, context, &entry);
                            return TRUE;
                        }
                    }
                }
                break;
        }
    }
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

NTSTATUS EtShowObjectManagerDialogThread(
    _In_ PVOID Parameter
    )
{
    BOOL result;
    MSG message;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    EtObjectManagerDialogHandle = PhCreateDialog(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OBJMGR),
        NULL,
        WinObjDlgProc,
        Parameter
        );

    PhSetEvent(&EtObjectManagerDialogInitializedEvent);

    PostMessage(EtObjectManagerDialogHandle, WM_PH_SHOW_DIALOG, 0, 0);
    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == INT_ERROR)
            break;

        if (message.message == WM_KEYDOWN /*|| message.message == WM_KEYUP*/) // forward key messages (Dart Vanya)
        {
            CallWindowProc(WinObjDlgProc, EtObjectManagerDialogHandle, message.message, message.wParam, message.lParam);
        }

        if (!IsDialogMessage(EtObjectManagerDialogHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);

    if (EtObjectManagerDialogThreadHandle)
    {
        NtClose(EtObjectManagerDialogThreadHandle);
        EtObjectManagerDialogThreadHandle = NULL;
    }

    PhResetEvent(&EtObjectManagerDialogInitializedEvent);

    return STATUS_SUCCESS;
}

VOID EtShowObjectManagerDialog(
    _In_ HWND ParentWindowHandle
    )
{
    if (!EtObjectManagerDialogThreadHandle)
    {
        if (!NT_SUCCESS(PhCreateThreadEx(&EtObjectManagerDialogThreadHandle, EtShowObjectManagerDialogThread, ParentWindowHandle)))
        {
            PhShowError2(ParentWindowHandle, L"Unable to create the window.", L"%s", L"");
            return;
        }

        PhWaitForEvent(&EtObjectManagerDialogInitializedEvent, NULL);
    }

    PostMessage(EtObjectManagerDialogHandle, WM_PH_SHOW_DIALOG, 0, 0);
}
