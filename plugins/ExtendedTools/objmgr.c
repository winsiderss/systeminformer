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
#include <ntuser.h>

static CONST PH_STRINGREF EtObjectManagerRootDirectoryObject = PH_STRINGREF_INIT(L"\\"); // RtlNtPathSeparatorString
static CONST PH_STRINGREF EtObjectManagerUserDirectoryObject = PH_STRINGREF_INIT(L"??"); // RtlDosDevicesPrefix
static CONST PH_STRINGREF DirectoryObjectType = PH_STRINGREF_INIT(L"Directory");
static HANDLE EtObjectManagerDialogThreadHandle = NULL;
static PH_EVENT EtObjectManagerDialogInitializedEvent = PH_EVENT_INIT;

HWND EtObjectManagerDialogHandle = NULL;
PPH_LIST EtObjectManagerOwnHandles = NULL;
HICON EtObjectManagerPropIcon = NULL;
BOOLEAN EtObjectManagerShowHandlesPage = FALSE;

// Cached type indexes
ULONG EtAlpcPortTypeIndex = ULONG_MAX;
ULONG EtDeviceTypeIndex = ULONG_MAX;
ULONG EtFilterPortTypeIndex = ULONG_MAX;
ULONG EtFileTypeIndex = ULONG_MAX;
ULONG EtKeyTypeIndex = ULONG_MAX;
ULONG EtSectionTypeIndex = ULONG_MAX;
ULONG EtWinStaTypeIndex = ULONG_MAX;

PPH_OBJECT_TYPE EtObjectEntryType = NULL;

static PPH_STRING ObjectSignaledString = NULL;
static PPH_STRING ObjectNotificationString = NULL;
static PPH_STRING ObjectSignaledNotificationString = NULL;
static PPH_STRING ObjectSyncronizationString = NULL;
static PPH_STRING ObjectSignaledSyncronizationString = NULL;
static PPH_STRING ObjectAbandonedString = NULL;

// Columns

#define ETOBLVC_NAME 0
#define ETOBLVC_TYPE 1
#define ETOBLVC_TARGET 2
#define ETOBLVC_OBJECT 3

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

typedef struct _ET_OBJECT_CONTEXT
{
    HWND WindowHandle;
    HWND ParentWindowHandle;
    HWND ListViewHandle;
    HWND TreeViewHandle;
    HWND SearchBoxHandle;
    HWND PathControlHandle;
    HWND PathControlEdit;
    HWND StatusBarHandle;
    PH_LAYOUT_MANAGER LayoutManager;

    HTREEITEM RootTreeObject;
    HTREEITEM SelectedTreeItem;

    HIMAGELIST TreeImageList;
    HIMAGELIST ListImageList;

    PPH_STRING CurrentPath;
    volatile PPH_LIST CurrentDirectoryList;
    BOOLEAN DisableSelChanged;

    PBOOLEAN BreakResolverThread;

    BOOLEAN UseAddressColumn;
} ET_OBJECT_CONTEXT, * PET_OBJECT_CONTEXT;

typedef struct _ET_OBJECT_ENTRY
{
    PET_OBJECT_CONTEXT Context;
    PPH_STRING BaseDirectory;

    ET_OBJECT_TYPE EtObjectType;
    PPH_STRING Name;
    PPH_STRING TypeName;

    INT ItemIndex;
    PPH_STRING Target;
    BOOLEAN TargetIsResolving;
    BOOLEAN TargetIsInfoOnly;
    PPH_STRING TargetDrvLow;
    PPH_STRING TargetDrvUp;
    CLIENT_ID TargetClientId;
    ULONG TypeTypeIndex;

    PVOID Object;
    ULONG TypeIndex;
    ULONG Attributes;
    WCHAR ObjectString[PH_PTR_STR_LEN_1];
} ET_OBJECT_ENTRY, *PET_OBJECT_ENTRY;

typedef struct _ET_DIRECTORY_ENUM_CONTEXT
{
    HWND TreeViewHandle;
    HTREEITEM RootTreeItem;
    PH_STRINGREF DirectoryPath;
} ET_DIRECTORY_ENUM_CONTEXT, *PET_DIRECTORY_ENUM_CONTEXT;

typedef struct _ET_HANDLE_OPEN_CONTEXT
{
    PPH_STRING CurrentPath;
    PET_OBJECT_ENTRY Object;
    PPH_STRING FullName;
} ET_HANDLE_OPEN_CONTEXT, *PET_HANDLE_OPEN_CONTEXT;

typedef struct _RESOLVER_THREAD_CONTEXT
{
    PET_OBJECT_CONTEXT Context;
    volatile BOOLEAN Break;
    PPH_LIST EntryToResolve;
} RESOLVER_THREAD_CONTEXT, *PRESOLVER_THREAD_CONTEXT;

NTSTATUS EtTreeViewEnumDirectoryObjects(
    _In_ HWND TreeViewHandle,
    _In_ HTREEITEM RootTreeItem,
    _In_ PPH_STRING DirectoryPath
    );

#define OBJECT_OPENSOURCE_ALPCPORT  1
#define OBJECT_OPENSOURCE_KEY       2
#define OBJECT_OPENSOURCE_ALL   OBJECT_OPENSOURCE_ALPCPORT|OBJECT_OPENSOURCE_KEY

NTSTATUS EtObjectManagerOpenHandle(
    _Out_ PHANDLE Handle,
    _In_ PET_HANDLE_OPEN_CONTEXT Context,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG OpenFlags
    );

NTSTATUS EtObjectManagerOpenRealObject(
    _Out_ PHANDLE Handle,
    _In_ PET_HANDLE_OPEN_CONTEXT Context,
    _In_ ACCESS_MASK DesiredAccess,
    _Inout_opt_ PHANDLE OwnerProcessId
    );

NTSTATUS EtObjectManagerHandleCloseCallback(
    _In_ HANDLE Handle,
    _In_ BOOLEAN Release,
    _In_opt_ PVOID Context
    );

VOID NTAPI EtpObjectManagerChangeSelection(
    _In_ PET_OBJECT_CONTEXT context
    );

VOID NTAPI EtpObjectManagerSortAndSelectOld(
    _In_ PET_OBJECT_CONTEXT context,
    _In_opt_ PPH_STRING oldSelection
    );

VOID NTAPI EtpObjectManagerSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    );

NTSTATUS EtpTargetResolverWorkThreadStart(
    _In_ PVOID Parameter
    );

#define ET_CREATE_DIRECTORY_OBJECT_CONTEXT(DirectoryPath, ObjectContext) \
    ET_HANDLE_OPEN_CONTEXT ObjectContext; \
    ET_OBJECT_ENTRY entry_##ObjectContext = { 0 }; \
    entry_##ObjectContext.EtObjectType = EtObjectDirectory; \
    ObjectContext.CurrentPath = DirectoryPath; \
    ObjectContext.Object = &entry_##ObjectContext; \
    ObjectContext.FullName = NULL; \


#define ET_CREATE_DIRECTORY_OBJECT_ENTRY(DirectoryName, ObjectContext, Entry) \
    ET_OBJECT_ENTRY Entry = { 0 }; \
    Entry.Name = directory; \
    Entry.EtObjectType = EtObjectDirectory; \
    Entry.Context = ObjectContext; \

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
    _In_ PET_OBJECT_CONTEXT Context
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
    _In_ PPH_STRING BaseDirectory,
    _In_ PPH_STRING ObjectName
    )
{
    PH_FORMAT format[3];
    BOOLEAN needSeparator = !PhEqualStringRef(&BaseDirectory->sr, &EtObjectManagerRootDirectoryObject, TRUE);

    PhInitFormatSR(&format[0], BaseDirectory->sr);
    if (needSeparator)
        PhInitFormatSR(&format[1], PhNtPathSeperatorString);
    PhInitFormatSR(&format[1 + needSeparator], ObjectName->sr);
    return PhFormat(format, 2 + needSeparator, 0);
}

FORCEINLINE
PPH_STRING EtGetObjectFullPath2(
    _In_ PCPH_STRINGREF BaseDirectory,
    _In_ PCPH_STRINGREF ObjectName
    )
{
    PH_FORMAT format[3];
    BOOLEAN needSeparator = !PhEqualStringRef(BaseDirectory, &EtObjectManagerRootDirectoryObject, TRUE);

    PhInitFormatS(&format[0], BaseDirectory->Buffer);
    if (needSeparator)
        PhInitFormatSR(&format[1], PhNtPathSeperatorString);
    PhInitFormatS(&format[1 + needSeparator], ObjectName->Buffer);
    return PhFormat(format, 2 + needSeparator, 0);
}

HTREEITEM EtTreeViewAddItem(
    _In_ HWND TreeViewHandle,
    _In_ HTREEITEM Parent,
    _In_ BOOLEAN Expanded,
    _In_ PCPH_STRINGREF Name
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
    _In_ PCPH_STRINGREF Name,
    _In_ BOOLEAN Reverse
    )
{
    HTREEITEM current;
    HTREEITEM sibling;
    HTREEITEM child;
    PPH_STRING directory;
    ULONG children;

    current = TreeView_GetChild(TreeViewHandle, ParentTreeItem);

    if (Reverse)
        while (sibling = TreeView_GetNextSibling(TreeViewHandle, current))
            current = sibling;

    do
    {
        if (PhGetTreeViewItemParam(TreeViewHandle, current, &directory, &children))
        {
            if (PhEqualStringRef(&directory->sr, Name, TRUE))
            {
                return current;
            }

            if (children)
            {
                if (child = EtTreeViewFindItem(TreeViewHandle, current, Name, Reverse))
                {
                    return child;
                }
            }
        }
    } while (current =
        !Reverse ?
        TreeView_GetNextSibling(TreeViewHandle, current) :
        TreeView_GetPrevSibling(TreeViewHandle, current)
        );

    return NULL;
}

VOID EtCleanupTreeViewItemParams(
    _In_ PET_OBJECT_CONTEXT Context,
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
    _In_ PET_OBJECT_CONTEXT Context
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
    _In_ PET_OBJECT_CONTEXT Context
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
    PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_FOLDER), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_LINK), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_EVENT), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_MUTANT), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    index = PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_SEMAPHORE), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    index = PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_SECTION), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    index = PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_DRIVER), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    index = PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_DEVICE), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    index = PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_PORT), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    index = PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_FILTERPORT), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    index = PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_JOB), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    index = PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_SESSION), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    index = PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_KEY), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    index = PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_CPU), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    index = PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_MEMORY), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    index = PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_KEYEDEVENT), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    index = PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_TIMER), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    index = PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_WINSTA), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    index = PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_TYPE), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    index = PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_CALLBACK), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    index = PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);
}

static BOOLEAN NTAPI EtEnumDirectoryObjectsCallback(
    _In_ HANDLE RootDirectory,
    _In_ PCPH_STRINGREF Name,
    _In_ PCPH_STRINGREF TypeName,
    _In_ PET_DIRECTORY_ENUM_CONTEXT Context
    )
{
    if (PhEqualStringRef(TypeName, &DirectoryObjectType, TRUE))
    {
        PPH_STRING currentPath;
        HTREEITEM currentItem;

        currentPath = EtGetObjectFullPath2(&Context->DirectoryPath, Name);

        currentItem = EtTreeViewAddItem(
            Context->TreeViewHandle,
            Context->RootTreeItem,
            FALSE,
            Name
            );

        EtTreeViewEnumDirectoryObjects(
            Context->TreeViewHandle,
            currentItem,
            currentPath
            );

        PhDereferenceObject(currentPath);
    }

    return TRUE;
}

static BOOLEAN NTAPI EtEnumCurrentDirectoryObjectsCallback(
    _In_ HANDLE RootDirectory,
    _In_ PCPH_STRINGREF Name,
    _In_ PCPH_STRINGREF TypeName,
    _In_ PET_OBJECT_CONTEXT Context
    )
{
    //if (PhEqualStringRef(TypeName, &DirectoryObjectType, TRUE))
    //{
    //    if (!EtTreeViewFindItem(Context->TreeViewHandle, Context->SelectedTreeItem, Name, FALSE))
    //    {
    //        EtTreeViewAddItem(Context->TreeViewHandle, Context->SelectedTreeItem, TRUE, Name);
    //    }
    //}
    //else
    if (!PhEqualStringRef(TypeName, &DirectoryObjectType, TRUE))
    {
        PET_OBJECT_ENTRY entry;

        entry = PhCreateObjectZero(sizeof(ET_OBJECT_ENTRY), EtObjectEntryType);
        entry->Context = Context;
        entry->Name = PhCreateString2(Name);
        entry->TypeName = PhCreateString2(TypeName);
        entry->BaseDirectory = PhReferenceObject(Context->CurrentPath);

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

        entry->ItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, LPSTR_TEXTCALLBACK, entry);
        PhSetListViewItemImageIndex(Context->ListViewHandle, entry->ItemIndex, entry->EtObjectType);

        if (entry->EtObjectType == EtObjectSymLink)
        {
            PPH_STRING linkTarget;
            PPH_STRING driveLetter;

            if (NT_SUCCESS(PhQuerySymbolicLinkObject(&linkTarget, RootDirectory, &entry->Name->sr)))
            {
                PhMoveReference(&entry->Target, linkTarget);

                if (PhStartsWithString2(entry->Target, L"\\Device\\HarddiskVolume", TRUE) &&
                    (driveLetter = PhResolveDevicePrefix(&entry->Target->sr)))
                {
                    entry->TargetDrvLow = PhReferenceObject(entry->Target); // HACK
                    PhMoveReference(&entry->Target, PhFormatString(L"%s (%s)", PhGetString(entry->Target), PhGetString(driveLetter)));
                    PhDereferenceObject(driveLetter);
                }
            }
        }
        else if (entry->EtObjectType == EtObjectWindowStation)
        {
            entry->Target = EtGetWindowStationType(&entry->Name->sr);
        }

        if (Context->UseAddressColumn)
        {
            entry->TargetIsResolving = TRUE;
        }
        else if (
            entry->EtObjectType == EtObjectDevice ||       // allow resolving without driver for PhGetPnPDeviceName()
            entry->EtObjectType == EtObjectMutant ||
            entry->EtObjectType == EtObjectJob ||
            entry->EtObjectType == EtObjectSection ||
            entry->EtObjectType == EtObjectEvent ||
            entry->EtObjectType == EtObjectTimer ||
            entry->EtObjectType == EtObjectSemaphore ||
            entry->EtObjectType == EtObjectType ||
            entry->EtObjectType == EtObjectSession
            )
        {
            entry->TargetIsResolving = TRUE;
        }

        PhAddItemList(Context->CurrentDirectoryList, entry);
    }

    return TRUE;
}

NTSTATUS EtTreeViewEnumDirectoryObjects(
    _In_ HWND TreeViewHandle,
    _In_ HTREEITEM RootTreeItem,
    _In_ PPH_STRING DirectoryPath
    )
{
    NTSTATUS status;
    HANDLE directoryHandle;

    ET_CREATE_DIRECTORY_OBJECT_CONTEXT(DirectoryPath, objectContext);

    if (NT_SUCCESS(status = EtObjectManagerOpenHandle(&directoryHandle, &objectContext, DIRECTORY_QUERY, 0)))
    {
        ET_DIRECTORY_ENUM_CONTEXT enumContext;
        enumContext.TreeViewHandle = TreeViewHandle;
        enumContext.RootTreeItem = RootTreeItem;
        enumContext.DirectoryPath = PhGetStringRef(DirectoryPath);

        status = PhEnumDirectoryObjects(
            directoryHandle,
            EtEnumDirectoryObjectsCallback,
            &enumContext
            );

        NtClose(directoryHandle);

        TreeView_SortChildren(TreeViewHandle, RootTreeItem, TRUE); // add ?? item last

        if (PhEqualStringRef(&DirectoryPath->sr, &EtObjectManagerRootDirectoryObject, FALSE))
        {
            enumContext.DirectoryPath = EtObjectManagerRootDirectoryObject;

            // enumerate \??
            EtEnumDirectoryObjectsCallback(
                NULL,
                &EtObjectManagerUserDirectoryObject,
                &DirectoryObjectType,
                &enumContext
                );
        }
    }

    return status;
}

NTSTATUS EtpTargetResolverThreadStart(
    _In_ PVOID Parameter
    )
{
    PRESOLVER_THREAD_CONTEXT threadContext = Parameter;
    PET_OBJECT_CONTEXT context = threadContext->Context;

    NTSTATUS status = STATUS_SUCCESS;
    PET_OBJECT_ENTRY entry;
    PH_WORK_QUEUE workQueue;
    ULONG sortColumn;
    PH_SORT_ORDER sortOrder;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    ExtendedListView_SetColumnWidth(context->ListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);

    PhInitializeWorkQueue(&workQueue, 1, 20, 1000);

    for (ULONG i = 0; i < threadContext->EntryToResolve->Count; i++)
    {
        // Thread was interrupted externally
        if (status != STATUS_ABANDONED && ReadAcquire8(&threadContext->Break))
        {
            status = STATUS_ABANDONED;
        }

        entry = threadContext->EntryToResolve->Items[i];

        if (status != STATUS_ABANDONED)
        {
            entry->ItemIndex = PhFindListViewItemByParam(context->ListViewHandle, INT_ERROR, entry);    // need to update index
            PhQueueItemWorkQueue(&workQueue, EtpTargetResolverWorkThreadStart, entry);
        }
        else
        {
            PhDereferenceObject(entry);
        }
    }

    PhWaitForWorkQueue(&workQueue);
    PhDeleteWorkQueue(&workQueue);

    if (status != STATUS_ABANDONED && !ReadAcquire8(&threadContext->Break))
    {
        // Reapply sort and filter after done resolving and ensure selected item is visible
        PPH_STRING curentFilter = PhGetWindowText(context->SearchBoxHandle);
        if (!PhIsNullOrEmptyString(curentFilter))
        {
            EtpObjectManagerSearchControlCallback((ULONG_PTR)PhGetWindowContext(context->SearchBoxHandle, SHRT_MAX), context);  // HACK
        }
        else
        {
            ExtendedListView_GetSort(context->ListViewHandle, &sortColumn, &sortOrder);
            if (sortOrder != NoSortOrder && sortColumn == 2)
            {
                ExtendedListView_SortItems(context->ListViewHandle);

                INT index = ListView_GetNextItem(context->ListViewHandle, INT_ERROR, LVNI_SELECTED);
                if (index != INT_ERROR)
                    ListView_EnsureVisible(context->ListViewHandle, index, TRUE);
            }
        }
        if (curentFilter)
            PhDereferenceObject(curentFilter);

        WritePointerRelease(&context->BreakResolverThread, NULL);
    }

    PhDereferenceObject(threadContext->EntryToResolve);
    PhFree(threadContext);

    PhDeleteAutoPool(&autoPool);

    return status;
}

NTSTATUS EtpTargetResolverWorkThreadStart(
    _In_ PVOID Parameter
    )
{
    PET_OBJECT_ENTRY entry = Parameter;

    NTSTATUS status;
    ET_HANDLE_OPEN_CONTEXT objectContext;
    HANDLE objectHandle = NULL;
    HANDLE processId = NtCurrentProcessId();
    HANDLE processHandle = NtCurrentProcess();
    BOOLEAN alpcSourceOpened = FALSE;

    objectContext.CurrentPath = entry->BaseDirectory;
    objectContext.Object = entry;
    objectContext.FullName = NULL;

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

                deviceName = EtGetObjectFullPath(entry->BaseDirectory, entry->Name);

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
                            objectHandle = deviceBaseObject;
                        }

                        if (!objectHandle)
                            objectHandle = deviceObject;
                        else
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
                        PhDereferenceObject(devicePdoName);

                        entry->TargetIsInfoOnly = TRUE;
                        PhMoveReference(&entry->Target, devicePdoName2);
                    }
                }

                PhDereferenceObject(deviceName);
            }
            break;
        case EtObjectAlpcPort:
            if (KsiLevel() >= KphLevelMed)
            {
                // Using fast connect to port since we only need query connection OwnerProcessId
                if (!NT_SUCCESS(status = EtObjectManagerOpenHandle(&objectHandle, &objectContext, READ_CONTROL, 0)))
                {
                    // On failure try to open real (rare)
                    status = EtObjectManagerOpenRealObject(&objectHandle, &objectContext, 0, &processId);
                    alpcSourceOpened = NT_SUCCESS(status);
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
                    }
                }
            }
            break;
        case EtObjectMutant:
            {
                if (NT_SUCCESS(status = EtObjectManagerOpenHandle(&objectHandle, &objectContext, MUTANT_QUERY_STATE, 0)))
                {
                    MUTANT_OWNER_INFORMATION ownerInfo;
                    MUTANT_BASIC_INFORMATION basicInfo;

                    if (NT_SUCCESS(status = PhGetMutantOwnerInformation(objectHandle, &ownerInfo)))
                    {
                        if (ownerInfo.ClientId.UniqueProcess)
                        {
                            PhMoveReference(&entry->Target, PhGetClientIdName(&ownerInfo.ClientId));

                            entry->TargetClientId.UniqueProcess = ownerInfo.ClientId.UniqueProcess;
                            entry->TargetClientId.UniqueThread = ownerInfo.ClientId.UniqueThread;
                        }
                    }

                    if (!entry->TargetClientId.UniqueThread)    // HACK
                    {
                        if (NT_SUCCESS(status = PhGetMutantBasicInformation(objectHandle, &basicInfo)))
                        {
                            if (basicInfo.AbandonedState)
                            {
                                entry->TargetIsInfoOnly = TRUE;
                                entry->Target = PhReferenceObject(ObjectAbandonedString);
                            }
                        }
                    }
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

                        entry->TargetIsInfoOnly = TRUE;
                        PhMoveReference(&entry->Target, PhFinalStringBuilderString(&sb));
                    }
                }
            }
            break;
        case EtObjectDriver:
            if (KsiLevel() == KphLevelMax)
            {
                if (NT_SUCCESS(status = EtObjectManagerOpenHandle(&objectHandle, &objectContext, READ_CONTROL, 0)))
                {
                    PPH_STRING driverName;

                    if (NT_SUCCESS(status = PhGetDriverImageFileName(objectHandle, &driverName)))
                    {
                        PhMoveReference(&entry->Target, driverName);
                    }
                }
            }
            break;
        case EtObjectSection:
            {
                BOOLEAN useKsi = KsiLevel() >= KphLevelMed;

                if (!NT_SUCCESS(status = EtObjectManagerOpenHandle(&objectHandle, &objectContext, SECTION_QUERY | SECTION_MAP_READ, 0)) &&
                    useKsi)
                {
                    status = EtObjectManagerOpenRealObject(&objectHandle, &objectContext, 0, &processId);
                }

                if (NT_SUCCESS(status))
                {
                    PPH_STRING fileName;
                    PPH_STRING newFileName;
                    SECTION_BASIC_INFORMATION basicInfo;

                    if (NT_SUCCESS(status = PhGetSectionFileName(objectHandle, &fileName)))
                    {
                        if (newFileName = PhGetFileName(fileName))
                            PhMoveReference(&fileName, newFileName);

                        PhMoveReference(&entry->Target, fileName);
                    }
                    else
                    {
                        if (useKsi && processId != NtCurrentProcessId())
                        {
                            if (NT_SUCCESS(status = PhOpenProcess(
                                &processHandle,
                                PROCESS_QUERY_LIMITED_INFORMATION,
                                processId
                                )))
                            {
                                status = KphQueryInformationObject(
                                    processHandle,
                                    objectHandle,
                                    KphObjectSectionBasicInformation,
                                    &basicInfo,
                                    sizeof(basicInfo),
                                    NULL
                                    );
                            }
                        }
                        else
                        {
                            status = PhGetSectionBasicInformation(objectHandle, &basicInfo);
                        }

                        if (NT_SUCCESS(status))
                        {
                            PH_FORMAT format[4];
                            PWSTR sectionType = L"Unknown";

                            if (basicInfo.AllocationAttributes & SEC_COMMIT)
                                sectionType = L"Commit";
                            else if (basicInfo.AllocationAttributes & SEC_FILE)
                                sectionType = L"File";
                            else if (basicInfo.AllocationAttributes & SEC_IMAGE)
                                sectionType = L"Image";
                            else if (basicInfo.AllocationAttributes & SEC_RESERVE)
                                sectionType = L"Reserve";

                            PhInitFormatS(&format[0], sectionType);
                            PhInitFormatS(&format[1], L" (");
                            PhInitFormatSize(&format[2], basicInfo.MaximumSize.QuadPart);
                            PhInitFormatC(&format[3], ')');

                            entry->TargetIsInfoOnly = TRUE;
                            entry->Target = PhFormat(format, 4, 0);
                        }
                    }
                }
            }
            break;
        case EtObjectEvent:
            {
                if (NT_SUCCESS(status = EtObjectManagerOpenHandle(&objectHandle, &objectContext, EVENT_QUERY_STATE, 0)))
                {
                    EVENT_BASIC_INFORMATION basicInfo;

                    if (NT_SUCCESS(PhGetEventBasicInformation(objectHandle, &basicInfo)))
                    {
                        entry->TargetIsInfoOnly = TRUE;

                        switch (basicInfo.EventType)
                        {
                        case NotificationEvent:
                            entry->Target = PhReferenceObject(basicInfo.EventState > 0 ? ObjectSignaledNotificationString : ObjectNotificationString);
                            break;
                        case SynchronizationEvent:
                            entry->Target = PhReferenceObject(basicInfo.EventState > 0 ? ObjectSignaledSyncronizationString : ObjectSyncronizationString);
                            break;
                        default:
                            if (basicInfo.EventState > 0) entry->Target = PhReferenceObject(ObjectSignaledString);
                        }
                    }
                }
            }
            break;
        case EtObjectTimer:
            {
                if (NT_SUCCESS(status = EtObjectManagerOpenHandle(&objectHandle, &objectContext, TIMER_QUERY_STATE, 0)))
                {
                    TIMER_BASIC_INFORMATION basicInfo;

                    if (NT_SUCCESS(PhGetTimerBasicInformation(objectHandle, &basicInfo)))
                    {
                        if (basicInfo.TimerState)
                        {
                            entry->TargetIsInfoOnly = TRUE;
                            entry->Target = PhReferenceObject(ObjectSignaledString);
                        }
                    }
                }
            }
            break;
        case EtObjectSemaphore:
            {
                if (NT_SUCCESS(status = EtObjectManagerOpenHandle(&objectHandle, &objectContext, SEMAPHORE_QUERY_STATE, 0)))
                {
                    SEMAPHORE_BASIC_INFORMATION basicInfo;

                    if (NT_SUCCESS(PhGetSemaphoreBasicInformation(objectHandle, &basicInfo)))
                    {
                        entry->TargetIsInfoOnly = TRUE;
                        entry->Target = PhFormatString(L"Current count: %d/%d", basicInfo.CurrentCount, basicInfo.MaximumCount);
                    }
                }
            }
            break;
        case EtObjectType:
            {
                POBJECT_TYPES_INFORMATION objectTypes;
                POBJECT_TYPE_INFORMATION objectType;
                ULONG typeIndex;

                typeIndex = PhGetObjectTypeNumber(&entry->Name->sr);

                if (typeIndex != ULONG_MAX &&
                    NT_SUCCESS(PhEnumObjectTypes(&objectTypes)))
                {
                    objectType = PH_FIRST_OBJECT_TYPE(objectTypes);

                    for (ULONG i = 0; i < objectTypes->NumberOfTypes; i++)
                    {
                        if (objectType->TypeIndex == typeIndex)
                        {
                            entry->TypeTypeIndex = typeIndex;
                            entry->TargetIsInfoOnly = TRUE;
                            entry->Target = PhFormatString(
                                L"Index: %d, Objects: %d, Handles: %d",
                                objectType->TypeIndex,
                                objectType->TotalNumberOfObjects,
                                objectType->TotalNumberOfHandles);

                            break;
                        }
                        objectType = PH_NEXT_OBJECT_TYPE(objectType);
                    }
                    PhFree(objectTypes);
                }
            }
            break;
        case EtObjectSession:
            {
                WINSTATIONINFORMATION winStationInfo;
                ULONG returnLength;
                ULONG sessionId;

                if ((sessionId = EtSessionIdFromObjectName(&entry->Name->sr)) != ULONG_MAX)
                {
                    if (WinStationQueryInformationW(
                        WINSTATION_CURRENT_SERVER,
                        sessionId,
                        WinStationInformation,
                        &winStationInfo,
                        sizeof(WINSTATIONINFORMATION),
                        &returnLength
                        ))
                    {
                        entry->TargetIsInfoOnly = TRUE;

                        if (winStationInfo.Domain[0] == UNICODE_NULL || winStationInfo.UserName[0] == UNICODE_NULL)
                        {
                            entry->Target = PhFormatString(L"%s (%s)", winStationInfo.WinStationName, EtMapSessionConnectState(winStationInfo.ConnectState));
                        }

                        else
                        {
                            entry->Target = PhFormatString(
                                L"%s%c%s (%s)",
                                winStationInfo.Domain,
                                winStationInfo.Domain[0] != UNICODE_NULL ? OBJ_NAME_PATH_SEPARATOR : UNICODE_NULL,
                                winStationInfo.UserName,
                                EtMapSessionConnectState(winStationInfo.ConnectState));
                        }
                    }
                }
            }
            break;
    }

    if (KsiLevel() >= KphLevelMed && (entry->EtObjectType != EtObjectAlpcPort || alpcSourceOpened)) // show port address if source object was already opened
    {
        PVOID objectAddress = NULL;
        ULONG typeIndex = ULONG_MAX;
        ULONG attributes = 0;

        if (objectHandle)
        {
            if (NT_SUCCESS(EtObjectManagerGetHandleInfoEx(processId, processHandle, objectHandle, &objectAddress, &typeIndex, &attributes)))
            {
                entry->Object = objectAddress;
                entry->TypeIndex = typeIndex;
                entry->Attributes = attributes;     // TODO: add attributes column
                PhPrintPointer(entry->ObjectString, objectAddress);
            }
        }
        else
        {
            if (NT_SUCCESS(status = EtObjectManagerOpenHandle(&objectHandle, &objectContext, READ_CONTROL, OBJECT_OPENSOURCE_KEY)) ||
                NT_SUCCESS(status = EtObjectManagerOpenHandle(&objectHandle, &objectContext, MAXIMUM_ALLOWED, OBJECT_OPENSOURCE_KEY))
                /*|| NT_SUCCESS(status = EtObjectManagerOpenRealObject(&objectHandle, &objectContext, 0, &processId))*/     // very slow for BNOs directories
                )
            {
                //if (processId != NtCurrentProcessId())
                //{
                //    status = PhOpenProcess(
                //        &processHandle,
                //        PROCESS_QUERY_LIMITED_INFORMATION,
                //        processId
                //        );
                //}

                if (/*NT_SUCCESS(status) &&*/
                    NT_SUCCESS(EtObjectManagerGetHandleInfoEx(processId, processHandle, objectHandle, &objectAddress, &typeIndex, &attributes)))
                {
                    entry->Object = objectAddress;
                    entry->TypeIndex = typeIndex;
                    entry->Attributes = attributes;
                    PhPrintPointer(entry->ObjectString, objectAddress);
                }
            }
        }
    }

    if (objectHandle && processHandle == NtCurrentProcess())
    {
        NtClose(objectHandle);
    }

    if (processHandle && processHandle != NtCurrentProcess())
    {
        NtClose(processHandle);
    }

    // Target was successfully resolved, redraw list entry
    entry->TargetIsResolving = FALSE;
    ListView_RedrawItems(entry->Context->ListViewHandle, entry->ItemIndex, entry->ItemIndex);

    PhDereferenceObject(entry);

    return STATUS_SUCCESS;
}

NTSTATUS EtpObjectManagerStartResolver(
    _In_ PET_OBJECT_CONTEXT Context
    )
{
    PRESOLVER_THREAD_CONTEXT threadContext = PhAllocateZero(sizeof(RESOLVER_THREAD_CONTEXT));
    threadContext->Context = Context;
    Context->BreakResolverThread = (PBOOLEAN)&threadContext->Break;
    threadContext->EntryToResolve = PhCreateList(Context->CurrentDirectoryList->Count);

    for (ULONG i = 0; i < Context->CurrentDirectoryList->Count; i++)
    {
        PET_OBJECT_ENTRY entry = Context->CurrentDirectoryList->Items[i];
        if (entry->TargetIsResolving)
        {
            PhAddItemList(threadContext->EntryToResolve, PhReferenceObject(entry));
        }
    }

    return PhCreateThread2(EtpTargetResolverThreadStart, threadContext);
}

NTSTATUS EtEnumCurrentDirectoryObjects(
    _In_ PET_OBJECT_CONTEXT Context
    )
{
    NTSTATUS status;
    HANDLE directoryHandle;
    ULONG sortColumn;
    PH_SORT_ORDER sortOrder;
    WCHAR string[PH_INT32_STR_LEN_1];

    PhMoveReference(&Context->CurrentPath, EtGetSelectedTreeViewPath(Context));

    ET_CREATE_DIRECTORY_OBJECT_CONTEXT(Context->CurrentPath, objectContext);

    if (NT_SUCCESS(status = EtObjectManagerOpenHandle(&directoryHandle, &objectContext, DIRECTORY_QUERY, 0)))
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

    PhSetWindowText(Context->PathControlHandle, PhGetString(Context->CurrentPath));
    Edit_SetSel(Context->PathControlEdit, -2, -1);

    PhPrintUInt32(string, ListView_GetItemCount(Context->ListViewHandle));
    PhSetWindowText(Context->StatusBarHandle, PH_AUTO_T(PH_STRING, PhFormatString(L"Objects in current directory: %s", string))->Buffer);

    // Apply current filter and sort
    PPH_STRING curentFilter = PH_AUTO(PhGetWindowText(Context->SearchBoxHandle));
    if (!PhIsNullOrEmptyString(curentFilter))
    {
        EtpObjectManagerSearchControlCallback((ULONG_PTR)PhGetWindowContext(Context->SearchBoxHandle, SHRT_MAX), Context); // HACK
    }

    ExtendedListView_GetSort(Context->ListViewHandle, &sortColumn, &sortOrder);
    if (sortOrder != NoSortOrder)
        ExtendedListView_SortItems(Context->ListViewHandle);

    if (NT_SUCCESS(status))
    {
        EtpObjectManagerStartResolver(Context);
    }

    return STATUS_SUCCESS;
}

VOID EtObjectManagerFreeListViewItems(
    _In_ PET_OBJECT_CONTEXT Context
    )
{
    if (ReadPointerAcquire(&Context->BreakResolverThread))
    {
        WriteRelease8(Context->BreakResolverThread, TRUE);
        WritePointerRelease(&Context->BreakResolverThread, NULL);
    }

    PhClearReference(&Context->CurrentPath);

    for (ULONG i = 0; i < Context->CurrentDirectoryList->Count; i++)
    {
        PhDereferenceObject(Context->CurrentDirectoryList->Items[i]);
    }

    //INT index = INT_ERROR;
    //while ((index = PhFindListViewItemByFlags(
    //    Context->ListViewHandle,
    //    index,
    //    LVNI_ALL
    //    )) != INT_ERROR)
    //{
    //    PET_OBJECT_ENTRY entry;

    //    if (PhGetListViewItemParam(Context->ListViewHandle, index, &entry))
    //    {
    //        PhDereferenceObject(entry);
    //    }
    //}

    PhClearList(Context->CurrentDirectoryList);
}

NTSTATUS EtDuplicateHandleFromProcessEx(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle,
    _In_ HANDLE SourceHandle
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    HANDLE processHandle;

    *Handle = NULL;
    processHandle = ProcessHandle;

    if (ProcessHandle ||
        ProcessId && NT_SUCCESS(status = PhOpenProcess(
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
        if (!ProcessHandle) NtClose(processHandle);
    }

    if (!NT_SUCCESS(status) && KsiLevel() == KphLevelMax)
    {
        processHandle = ProcessHandle;

        if (ProcessHandle ||
            ProcessId && NT_SUCCESS(status = PhOpenProcess(
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
            if (!ProcessHandle) NtClose(processHandle);
        }
    }

    return status;
}

NTSTATUS EtObjectManagerOpenHandle(
    _Out_ PHANDLE Handle,
    _In_ PET_HANDLE_OPEN_CONTEXT Context,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG OpenFlags
     )
{
    NTSTATUS status;
    HANDLE directoryHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;

    *Handle = NULL;

    if (Context->Object->EtObjectType >= EtObjectMax)
        return STATUS_INVALID_PARAMETER;

    if (Context->Object->EtObjectType == EtObjectDirectory)
    {
        if (PhIsNullOrEmptyString(Context->CurrentPath))
            return STATUS_INVALID_PARAMETER;

        return PhOpenDirectoryObject(Handle, DesiredAccess, NULL, &Context->CurrentPath->sr);
    }

    if (PhIsNullOrEmptyString(Context->Object->Name) ||
        PhIsNullOrEmptyString(Context->Object->BaseDirectory))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!NT_SUCCESS(status = PhOpenDirectoryObject(
        &directoryHandle,
        DIRECTORY_QUERY,
        NULL,
        &Context->Object->BaseDirectory->sr
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
                static __typeof__(&NtAlpcConnectPortEx) NtAlpcConnectPortEx_I = NULL;
                LARGE_INTEGER timeout;

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
                            PhTimeoutFromMilliseconds(&timeout, 1000)
                            );
                    }
                    else
                    {
                        PPH_STRING clientName;

                        clientName = EtGetObjectFullPath(Context->Object->BaseDirectory, Context->Object->Name);
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
                            PhTimeoutFromMilliseconds(&timeout, 1000)
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
                    deviceName = EtGetObjectFullPath(Context->Object->BaseDirectory, Context->Object->Name);

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
                static __typeof__(&NtUserOpenWindowStation) NtUserOpenWindowStation_I = NULL;
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

                    clientName = EtGetObjectFullPath(Context->Object->BaseDirectory, Context->Object->Name);
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
                static __typeof__(&NtOpenPartition) NtOpenPartition_I = NULL;

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
                static __typeof__(&NtOpenCpuPartition) NtOpenCpuPartition_I = NULL;

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
            {
                if (PhIsNullOrEmptyString(Context->Object->TypeName))
                {
                    status = STATUS_INVALID_PARAMETER;
                }
                else
                {
                    status = STATUS_NOINTERFACE;
                }
            }
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
// If OwnerProcessId is specified returns UniqueProcessId and HandleValue instead.
// This allows to extract object information through driver APIs even for objects with kernel only access (OBJ_KERNEL_HANDLE).
NTSTATUS EtObjectManagerOpenRealObject(
    _Out_ PHANDLE Handle,
    _In_ PET_HANDLE_OPEN_CONTEXT Context,
    _In_ ACCESS_MASK DesiredAccess,
    _Inout_opt_ PHANDLE OwnerProcessId
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PPH_STRING fullName;
    ULONG targetIndex;
    ULONG i;
    PSYSTEM_HANDLE_INFORMATION_EX handles;
    PPH_HASHTABLE processHandleHashtable;
    PVOID* processHandlePtr;
    HANDLE processHandle;
    PPH_KEY_VALUE_PAIR procEntry;

    *Handle = NULL;

    // Use cached index for most common types for better performance
    switch (Context->Object->EtObjectType)
    {
        case EtObjectAlpcPort:
            targetIndex = EtAlpcPortTypeIndex;
            break;
        case EtObjectDevice:
            targetIndex = EtDeviceTypeIndex;
            break;
        case EtObjectFilterPort:
            targetIndex = EtFilterPortTypeIndex;
            break;
        case EtObjectKey:
            targetIndex = EtKeyTypeIndex;
            break;
        case EtObjectSection:
            targetIndex = EtSectionTypeIndex;
            break;
        case EtObjectWindowStation:
            targetIndex = EtWinStaTypeIndex;
            break;
        default:
            targetIndex = PhGetObjectTypeNumber(&Context->Object->TypeName->sr);
    }

    if (Context->Object->EtObjectType == EtObjectDirectory)
        fullName = PhReferenceObject(Context->CurrentPath);
    else
        fullName = EtGetObjectFullPath(Context->Object->BaseDirectory, Context->Object->Name);

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
                if (processHandlePtr = PhFindItemSimpleHashtable(
                    processHandleHashtable,
                    handleInfo->UniqueProcessId
                    ))
                {
                    processHandle = *processHandlePtr;
                }
                else
                {
                    if (NT_SUCCESS(PhOpenProcess(
                        &processHandle,
                        (KsiLevel() >= KphLevelMed ? PROCESS_QUERY_LIMITED_INFORMATION : PROCESS_DUP_HANDLE),
                        handleInfo->UniqueProcessId
                        )))
                    {
                        PhAddItemSimpleHashtable(
                            processHandleHashtable,
                            handleInfo->UniqueProcessId,
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
                    handleInfo->HandleValue,
                    handleInfo->ObjectTypeIndex,
                    NULL,
                    NULL,
                    &objectName,
                    NULL
                    )))
                {
                    if (PhEqualString(objectName, fullName, TRUE))
                    {
                        if (OwnerProcessId)
                        {
                            *OwnerProcessId = handleInfo->UniqueProcessId;
                            *Handle = handleInfo->HandleValue;
                            status = STATUS_SUCCESS;

                            PhDereferenceObject(objectName);
                            break;
                        }

                        if (!NT_SUCCESS(status = EtDuplicateHandleFromProcessEx(
                            &objectHandle,
                            DesiredAccess,
                            NULL,
                            processHandle,
                            handleInfo->HandleValue
                            )))
                        {
                            status = EtDuplicateHandleFromProcessEx(
                                &objectHandle,
                                0,
                                NULL,
                                processHandle,
                                handleInfo->HandleValue
                                );
                        }

                        if (NT_SUCCESS(status))
                        {
                            *Handle = objectHandle;
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
        {
            status = NtClose(procEntry->Value);

            if (!NT_SUCCESS(status))
            {
                PhShowStatus(NULL, L"Unidentified third party object.", status, 0);
            }
        }

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
    _In_ HANDLE Handle,
    _In_ BOOLEAN Release,
    _In_opt_ PVOID Context
    )
{
    PET_HANDLE_OPEN_CONTEXT context = Context;

    if (Handle)
    {
        NtClose(Handle);
    }

    if (Release && Context)
    {
        PhClearReference(&context->CurrentPath);
        PhClearReference(&context->FullName);
        PhClearReference(&context->Object);
        PhFree(context);
    }

    return STATUS_SUCCESS;
}

INT NTAPI WinObjNameCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PET_OBJECT_ENTRY item1 = Item1;
    PET_OBJECT_ENTRY item2 = Item2;

    return PhCompareStringWithNull(item1->Name, item2->Name, TRUE);
}

INT NTAPI WinObjTypeCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PET_OBJECT_ENTRY item1 = Item1;
    PET_OBJECT_ENTRY item2 = Item2;

    return PhCompareStringWithNull(item1->TypeName, item2->TypeName, TRUE);
}

INT NTAPI WinObjTargetCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PET_OBJECT_ENTRY item1 = Item1;
    PET_OBJECT_ENTRY item2 = Item2;

    if (item1->EtObjectType == EtObjectType && item2->EtObjectType == EtObjectType)
        return uintcmp(item1->TypeTypeIndex, item2->TypeTypeIndex);
    else
        return PhCompareStringWithNull(item1->Target, item2->Target, TRUE);
}

INT NTAPI WinObjTriStateCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PET_OBJECT_CONTEXT context = Context;

    assert(context);

    return PhFindItemList(context->CurrentDirectoryList, Item1) - PhFindItemList(context->CurrentDirectoryList, Item2);
}

INT NTAPI WinObjObjectCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PET_OBJECT_ENTRY item1 = Item1;
    PET_OBJECT_ENTRY item2 = Item2;

    return uintptrcmp((ULONG_PTR)item1->Object, (ULONG_PTR)item2->Object);
}

NTSTATUS EtObjectManagerGetHandleInfoEx(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE ObjectHandle,
    _Out_opt_ PVOID* Object,
    _Out_opt_ PULONG TypeIndex,
    _Out_opt_ PULONG Attributes
    )
{
    NTSTATUS status = STATUS_INVALID_HANDLE;
    ULONG i;

    if (KsiLevel() >= KphLevelMed)
    {
        if (Attributes)
        {
            KPH_OBJECT_ATTRIBUTES_INFORMATION extendedInfo;
            if (NT_SUCCESS(status = KphQueryInformationObject(
                ProcessHandle,
                ObjectHandle,
                KphObjectAttributesInformation,
                &extendedInfo,
                sizeof(extendedInfo),
                NULL
                )))
            {
                // not implemented
                //if (Object) *Object = extendedInfo.Object;
                //if (TypeIndex) *TypeIndex = extendedInfo.ObjectTypeIndex;

                if (extendedInfo.ExclusiveObject)
                    *Attributes |= OBJ_EXCLUSIVE;
                if (extendedInfo.PermanentObject)
                    *Attributes |= OBJ_PERMANENT;
                if (extendedInfo.KernelObject)
                    *Attributes |= OBJ_KERNEL_HANDLE;
                if (extendedInfo.KernelOnlyAccess)
                    *Attributes |= PH_OBJ_KERNEL_ACCESS_ONLY;
            }
        }

        if (Object || TypeIndex)
        {
            PKPH_PROCESS_HANDLE_INFORMATION handles;
            if (NT_SUCCESS(status = KsiEnumerateProcessHandles(ProcessHandle, &handles)))
            {
                for (i = 0; i < handles->HandleCount; i++)
                {
                    if (handles->Handles[i].Handle == ObjectHandle)
                    {
                        if (Object) *Object = handles->Handles[i].Object;
                        if (TypeIndex) *TypeIndex = handles->Handles[i].ObjectTypeIndex;
                        break;
                    }
                }
                PhFree(handles);
            }
        }
    }

    if (!NT_SUCCESS(status))
    {
        PSYSTEM_HANDLE_INFORMATION_EX handles;

        if (NT_SUCCESS(PhEnumHandlesEx(&handles)))
        {
            for (i = 0; i < handles->NumberOfHandles; i++)
            {
                if (handles->Handles[i].UniqueProcessId == ProcessId &&
                    handles->Handles[i].HandleValue == ObjectHandle)
                {
                    if (Object) *Object = handles->Handles[i].Object;
                    if (TypeIndex) *TypeIndex = handles->Handles[i].ObjectTypeIndex;
                    if (Attributes) *Attributes = handles->Handles[i].HandleAttributes;
                    status = STATUS_SUCCESS;
                    break;
                }
            }
            PhFree(handles);
        }
    }

    return status;
}

VOID NTAPI EtpObjectManagerObjectProperties(
    _In_ PET_OBJECT_ENTRY Entry
    )
{
    PET_OBJECT_CONTEXT context = Entry->Context;
    NTSTATUS status;
    NTSTATUS subStatus = STATUS_UNSUCCESSFUL;
    HANDLE objectHandle = NULL;
    ET_HANDLE_OPEN_CONTEXT objectContext;
    PPH_HANDLE_ITEM handleItem;
    HANDLE processId = NtCurrentProcessId();
    HANDLE processHandle = NtCurrentProcess();
    PPH_STRING objectName;

    if (Entry->EtObjectType == EtObjectDirectory)
        objectName = PhReferenceObject(context->CurrentPath);
    else
        objectName = EtGetObjectFullPath(Entry->BaseDirectory, Entry->Name);

    objectContext.CurrentPath = PhReferenceObject(context->CurrentPath);
    objectContext.Object = PhReferenceObject(Entry);
    objectContext.FullName = NULL;

    if (Entry->EtObjectType == EtObjectDirectory)
        objectContext.Object->TypeName = PhCreateString2(&DirectoryObjectType);

    PhSetCursor(PhLoadCursor(NULL, IDC_WAIT));

    // Try to open with WRITE_DAC to allow change security from "Security" page
    if (!NT_SUCCESS(status = EtObjectManagerOpenHandle(&objectHandle, &objectContext, READ_CONTROL | WRITE_DAC, OBJECT_OPENSOURCE_ALL)))
    {
        if (status == STATUS_NOT_SUPPORTED ||
            !NT_SUCCESS(status = EtObjectManagerOpenHandle(&objectHandle, &objectContext, MAXIMUM_ALLOWED, OBJECT_OPENSOURCE_ALL)))
        {
            // Can open KernelOnlyAccess object if any opened handles for this object exists (ex. \PowerPort, \Win32kCrossSessionGlobals)
            status = EtObjectManagerOpenRealObject(&objectHandle, &objectContext, 0, &processId);
        }
    }

    handleItem = PhCreateHandleItem(NULL);
    handleItem->Handle = objectHandle;
    handleItem->TypeIndex = ULONG_MAX;

    switch (Entry->EtObjectType)
    {
        case EtObjectDirectory:
            handleItem->TypeName = Entry->TypeName = objectContext.Object->TypeName;
            break;
        default:
            handleItem->TypeName = PhReferenceObject(Entry->TypeName);
            break;
    }

    handleItem->ObjectName = objectName;
    handleItem->BestObjectName = PhReferenceObject(objectName);

    if (NT_SUCCESS(status))
    {
        if (processId != NtCurrentProcessId())
        {
            status = PhOpenProcess(
                &processHandle,
                (KsiLevel() >= KphLevelMed ? PROCESS_QUERY_LIMITED_INFORMATION : PROCESS_DUP_HANDLE),
                processId
                );
        }

        if (NT_SUCCESS(status))
        {
            OBJECT_BASIC_INFORMATION basicInfo;

            // Get object address, type index and attributes
            subStatus = EtObjectManagerGetHandleInfoEx(
                processId,
                processHandle,
                objectHandle,
                &handleItem->Object,
                &handleItem->TypeIndex,
                &handleItem->Attributes
                );

            if (NT_SUCCESS(status = PhGetHandleInformation(
                processHandle,
                objectHandle,
                handleItem->TypeIndex,
                &basicInfo,
                NULL,
                NULL,
                NULL
                )))
            {
                // We will remove access row in EtHandlePropertiesWindowInitialized callback
                if (!NT_SUCCESS(subStatus) || KsiLevel() < KphLevelMed)
                {
                    handleItem->Attributes = basicInfo.Attributes;
                }
            }

            if (processHandle != NtCurrentProcess())
                NtClose(processHandle);
        }

        if (processId == NtCurrentProcessId())
        {
            PhReferenceObject(EtObjectManagerOwnHandles);
            PhAddItemList(EtObjectManagerOwnHandles, objectHandle);
        }
    }

    if (handleItem->TypeIndex == ULONG_MAX)
    {
        handleItem->TypeIndex = PhGetObjectTypeNumber(&Entry->TypeName->sr);
    }

    EtObjectManagerPropIcon = PhImageListGetIcon(context->ListImageList, Entry->EtObjectType, ILD_NORMAL | ILD_TRANSPARENT);

    // Object Manager plugin window
    PhShowHandlePropertiesEx(context->WindowHandle, processId, handleItem, PluginInstance, PhGetString(Entry->TypeName));

    PhDereferenceObject(Entry);
    PhDereferenceObject(objectContext.CurrentPath);
}

VOID NTAPI EtpObjectManagerObjectHandles(
    _In_ PET_OBJECT_ENTRY Entry
    )
{
    EtObjectManagerShowHandlesPage = TRUE;

    EtpObjectManagerObjectProperties(Entry);
}

BOOLEAN EtListViewFindAndSelectItem(
    _In_ PET_OBJECT_CONTEXT Context,
    _In_ PCPH_STRINGREF Name
    )
{
    LVFINDINFO findinfo;
    findinfo.psz = PH_AUTO_T(PH_STRING, PhCreateString2(Name))->Buffer;
    findinfo.flags = LVFI_STRING;

    INT item = ListView_FindItem(Context->ListViewHandle, INT_ERROR, &findinfo);

    // Navigate to target object
    if (item != INT_ERROR)
    {
        ListView_SetItemState(Context->ListViewHandle, item, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(Context->ListViewHandle, item, TRUE);
    }
    return item != INT_ERROR;
}

BOOLEAN NTAPI EtpObjectManagerOpenTarget(
    _In_ PET_OBJECT_CONTEXT Context,
    _In_ PPH_STRING Target
    )
{
    PH_STRINGREF pathPart;
    PH_STRINGREF namePart;
    PH_STRINGREF directoryPart = PhGetStringRef(NULL);
    PH_STRINGREF remainingPart;
    BOOLEAN reverseScan = FALSE;
    BOOLEAN startFromRoot;
    HTREEITEM directoryItem;
    BOOLEAN targetFound = FALSE;

    remainingPart = PhGetStringRef(PhReferenceObject(Target));
    Context->SelectedTreeItem = Context->RootTreeObject;
    ListView_SetItemState(Context->ListViewHandle, -1, 0, LVIS_SELECTED);

    PhSplitStringRefAtLastChar(&remainingPart, OBJ_NAME_PATH_SEPARATOR, &pathPart, &namePart);

    // Check if target directory is equal to current
    if (!PhEqualStringRef(&Target->sr, &Context->CurrentPath->sr, TRUE))
    {
        if (PhStartsWithStringRef(&remainingPart, &EtObjectManagerRootDirectoryObject, FALSE))
        {
            PhSetWindowText(Context->SearchBoxHandle, L"");
start_scan:
            startFromRoot = TRUE;

            while (remainingPart.Length != 0)
            {
                if (!startFromRoot)
                    PhSplitStringRefAtChar(&remainingPart, OBJ_NAME_PATH_SEPARATOR, &directoryPart, &remainingPart);

                if (directoryPart.Length != 0 || startFromRoot)
                {
                    if (!startFromRoot)
                    {
                        directoryItem = EtTreeViewFindItem(
                            Context->TreeViewHandle,
                            Context->SelectedTreeItem,
                            &directoryPart,
                            reverseScan
                            );
                    }
                    else
                    {
                        directoryItem = Context->RootTreeObject;
                    }

                    if (directoryItem)
                    {
                        TreeView_SelectItem(Context->TreeViewHandle, directoryItem);
                        Context->SelectedTreeItem = directoryItem;
                        targetFound = TRUE;
                    }
                    else if (EtListViewFindAndSelectItem(Context, &directoryPart)) // try to find and select target in listview
                    {
                        targetFound = TRUE;
                        break;
                    }
                }

                if (startFromRoot)
                    startFromRoot = FALSE;
            }

            if (!reverseScan && !PhStartsWithString(Target, PH_AUTO_T(PH_STRING, EtGetSelectedTreeViewPath(Context)), TRUE))
            {
                remainingPart = PhGetStringRef(Target);
                Context->SelectedTreeItem = Context->RootTreeObject;
                reverseScan = TRUE;
                targetFound = FALSE;
                goto start_scan;
            }
        }

        // If we did jump to new tree target, then focus to listview target item
        if (directoryPart.Length > 0 || PhEqualStringRef(&Target->sr, &EtObjectManagerRootDirectoryObject, FALSE))   // HACK
        {
            EtpObjectManagerSearchControlCallback(0, Context);
        }
        else    // browse to target in explorer
        {
            if (!PhIsNullOrEmptyString(Target) &&
                (PhDoesFileExist(&Target->sr) || PhDoesFileExistWin32(Target->Buffer)))
            {
                PhShellExecuteUserString(
                    Context->WindowHandle,
                    L"FileBrowseExecutable",
                    PhGetString(Target),
                    FALSE,
                    L"Make sure the Explorer executable file is present."
                    );
            }
            else
            {
                PhShowStatus(Context->WindowHandle, L"Unable to locate the target.", STATUS_NOT_FOUND, 0);
            }
        }
    }
    else if (namePart.Length > 0)   // Same directory
    {
        PhSetWindowText(Context->SearchBoxHandle, L"");
        EtpObjectManagerSearchControlCallback(0, Context);

        targetFound = EtListViewFindAndSelectItem(Context, &namePart);
    }

    PhDereferenceObject(Target);
    return targetFound;
}

VOID NTAPI EtpObjectManagerRefresh(
    _In_ PET_OBJECT_CONTEXT Context
    )
{
    PPH_STRING currentPath = PhReferenceObject(Context->CurrentPath);
    PPH_STRING oldSelect = NULL;
    PET_OBJECT_ENTRY* listviewItems;
    ULONG numberOfItems;
    PH_STRINGREF directoryPart;
    PH_STRINGREF remainingPart;
    BOOLEAN reverseScan = FALSE;

    SendMessage(Context->TreeViewHandle, WM_SETREDRAW, FALSE, 0);

    PhGetSelectedListViewItemParams(Context->ListViewHandle, &listviewItems, &numberOfItems);
    if (numberOfItems != 0)
        oldSelect = PhReferenceObject(listviewItems[0]->Name);

    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);

    EtObjectManagerFreeListViewItems(Context);
    ListView_DeleteAllItems(Context->ListViewHandle);
    EtCleanupTreeViewItemParams(Context, Context->RootTreeObject);
    TreeView_DeleteAllItems(Context->TreeViewHandle);

    Context->DisableSelChanged = TRUE;
    Context->RootTreeObject = EtTreeViewAddItem(Context->TreeViewHandle, TVI_ROOT, TRUE, &EtObjectManagerRootDirectoryObject);
    PhMoveReference(&Context->CurrentPath, PhCreateString2(&EtObjectManagerRootDirectoryObject));

    EtTreeViewEnumDirectoryObjects(
        Context->TreeViewHandle,
        Context->RootTreeObject,
        Context->CurrentPath
        );

start_scan:
    remainingPart = PhGetStringRef(currentPath);
    Context->SelectedTreeItem = Context->RootTreeObject;

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtChar(&remainingPart, OBJ_NAME_PATH_SEPARATOR, &directoryPart, &remainingPart);

        if (directoryPart.Length != 0)
        {
            HTREEITEM directoryItem = EtTreeViewFindItem(
                Context->TreeViewHandle,
                Context->SelectedTreeItem,
                &directoryPart,
                reverseScan
                );

            if (directoryItem)
            {
                TreeView_SelectItem(Context->TreeViewHandle, directoryItem);
                Context->SelectedTreeItem = directoryItem;
            }
        }
    }

    if (!reverseScan && !PhStartsWithString(currentPath, PH_AUTO_T(PH_STRING, EtGetSelectedTreeViewPath(Context)), TRUE))
    {
        reverseScan = TRUE;
        goto start_scan;
    }

    TreeView_EnsureVisible(Context->TreeViewHandle, Context->SelectedTreeItem);
    // Will reapply filter and sort
    EtEnumCurrentDirectoryObjects(Context);

    // Reapply old selection
    EtpObjectManagerSortAndSelectOld(Context, oldSelect);

    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
    Context->DisableSelChanged = FALSE;

    PhDereferenceObject(currentPath);

    SendMessage(Context->TreeViewHandle, WM_SETREDRAW, TRUE, 0);
}

VOID NTAPI EtpObjectManagerOpenSecurity(
    _In_ PET_OBJECT_ENTRY Entry
    )
{
    PET_OBJECT_CONTEXT context = Entry->Context;
    PET_HANDLE_OPEN_CONTEXT objectContext;

    objectContext = PhAllocateZero(sizeof(ET_HANDLE_OPEN_CONTEXT));
    objectContext->Object = PhCreateObjectZero(sizeof(ET_OBJECT_ENTRY), EtObjectEntryType);
    objectContext->Object->EtObjectType = Entry->EtObjectType;
    objectContext->Object->Name = PhReferenceObject(Entry->Name);
    objectContext->Object->Context = Entry->Context;

    switch (Entry->EtObjectType)
    {
        case EtObjectDirectory:
            objectContext->Object->TypeName = PhCreateString2(&DirectoryObjectType);
            objectContext->CurrentPath = PhReferenceObject(context->CurrentPath);
            objectContext->FullName = PhReferenceObject(context->CurrentPath);
            break;
        default:
            objectContext->Object->TypeName = PhReferenceObject(Entry->TypeName);
            objectContext->Object->BaseDirectory = PhReferenceObject(Entry->BaseDirectory);
            objectContext->FullName = EtGetObjectFullPath(Entry->BaseDirectory, Entry->Name);
            break;
    }

    PhEditSecurity(
        !!PhGetIntegerSetting(L"ForceNoParent") ? NULL : context->WindowHandle,
        PhGetString(objectContext->FullName),
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
    PET_OBJECT_CONTEXT context = Context;
    PET_OBJECT_ENTRY* listviewItems;
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
        PET_OBJECT_ENTRY entry = Array->Items[i];

        if (!MatchHandle ||
            PhSearchControlMatch(MatchHandle, &entry->Name->sr) ||
            PhSearchControlMatch(MatchHandle, &entry->TypeName->sr) ||
            entry->Target && PhSearchControlMatch(MatchHandle, &entry->Target->sr) ||
            entry->Object && PhSearchControlMatchPointer(MatchHandle, entry->Object)
            )
        {
            entry->ItemIndex = PhAddListViewItem(context->ListViewHandle, MAXINT, LPSTR_TEXTCALLBACK, entry);
            PhSetListViewItemImageIndex(context->ListViewHandle, entry->ItemIndex, entry->EtObjectType);
        }
    }

    // Keep current sort and selection after apply new filter
    EtpObjectManagerSortAndSelectOld(context, oldSelect);

    ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);

    PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);
    if (numberOfItems != 0)
        oldSelect = EtGetObjectFullPath(listviewItems[0]->BaseDirectory, listviewItems[0]->Name);
    else
        oldSelect = PhReferenceObject(context->CurrentPath);

    PhSetDialogItemText(context->WindowHandle, IDC_OBJMGR_PATH, PhGetString(oldSelect));
    PhDereferenceObject(oldSelect);

    WCHAR string[PH_INT32_STR_LEN_1];
    PhPrintUInt32(string, ListView_GetItemCount(context->ListViewHandle));
    PhSetWindowText(context->StatusBarHandle, PH_AUTO_T(PH_STRING, PhFormatString(L"Objects in current directory: %s", string))->Buffer);
}

VOID NTAPI EtpObjectManagerSortAndSelectOld(
    _In_ PET_OBJECT_CONTEXT Context,
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
    _In_ PET_OBJECT_CONTEXT Context
    )
{
    Context->SelectedTreeItem = TreeView_GetSelection(Context->TreeViewHandle);

    if (!Context->DisableSelChanged)
        ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    EtObjectManagerFreeListViewItems(Context);
    ListView_DeleteAllItems(Context->ListViewHandle);

    EtEnumCurrentDirectoryObjects(Context);

    if (!Context->DisableSelChanged)
        ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

VOID EtpObjectManagerCopyObjectAddress(
    _In_ PET_OBJECT_ENTRY Entry
    )
{
    PET_OBJECT_CONTEXT context = Entry->Context;
    NTSTATUS status;
    WCHAR string[PH_INT64_STR_LEN_1] = L"";
    PH_STRINGREF pointer = PH_STRINGREF_INIT(string);
    HANDLE objectHandle;
    HANDLE processId = NtCurrentProcessId();
    HANDLE processHandle = NtCurrentProcess();
    ET_HANDLE_OPEN_CONTEXT objectContext;

    if (Entry->Object)
    {
        PhInitializeStringRef(&pointer, Entry->ObjectString);
        PhSetClipboardString(context->WindowHandle, &pointer);
        return;
    }

    objectContext.CurrentPath = PhReferenceObject(context->CurrentPath);
    objectContext.Object = PhReferenceObject(Entry);
    objectContext.FullName = NULL;

    if (NT_SUCCESS(status = EtObjectManagerOpenHandle(&objectHandle, &objectContext, READ_CONTROL, OBJECT_OPENSOURCE_ALL)) ||
        NT_SUCCESS(status = EtObjectManagerOpenRealObject(&objectHandle, &objectContext, 0, &processId)))
    {
        PVOID objectAddress = NULL;

        if (status == STATUS_NOT_ALL_ASSIGNED)
        {
            NtClose(objectHandle);
            goto cleanup_exit;
        }

        if (processId != NtCurrentProcessId())
        {
            status = PhOpenProcess(
                &processHandle,
                (KsiLevel() >= KphLevelMed ? PROCESS_QUERY_LIMITED_INFORMATION : PROCESS_DUP_HANDLE),
                processId
                );
        }

        if (NT_SUCCESS(status))
        {
            if (NT_SUCCESS(EtObjectManagerGetHandleInfoEx(processId, processHandle, objectHandle, &objectAddress, NULL, NULL)))
            {
                Entry->Object = objectAddress;
                PhPrintPointer(Entry->ObjectString, objectAddress);
                ListView_RedrawItems(Entry->Context->ListViewHandle, Entry->ItemIndex, Entry->ItemIndex);
                PhInitializeStringRef(&pointer, Entry->ObjectString);
            }
        }

        NtClose(processId == NtCurrentProcessId() ? objectHandle : processHandle);
    }

cleanup_exit:
    PhSetClipboardString(context->WindowHandle, &pointer);

    PhDereferenceObject(Entry);
    PhDereferenceObject(objectContext.CurrentPath);
}

VOID EtpObjectEntryDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PET_OBJECT_ENTRY entry = Object;

    PhClearReference(&entry->Name);
    PhClearReference(&entry->TypeName);
    PhClearReference(&entry->Target);
    PhClearReference(&entry->TargetDrvLow);
    PhClearReference(&entry->TargetDrvUp);
    PhClearReference(&entry->BaseDirectory);
}

VOID EtpLoadComboBoxHistoryToSettings(
    _In_ PET_OBJECT_CONTEXT Context
    )
{
    PPH_STRING savedChoices = PhGetStringSetting(SETTING_NAME_OBJMGR_HISTORY);
    PPH_STRING savedChoice;
    ULONG_PTR indexOfDelim;
    ULONG_PTR i = 0;

    // Split the saved choices using the delimiter.
    while (i < savedChoices->Length / sizeof(WCHAR))
    {
        indexOfDelim = PhFindStringInString(savedChoices, i, ET_OBJMGR_HISTORY_SEPARATOR);

        if (indexOfDelim == SIZE_MAX)
            indexOfDelim = savedChoices->Length / sizeof(WCHAR);

        savedChoice = PhSubstring(savedChoices, i, indexOfDelim - i);

        if (savedChoice->Length)
        {
            ComboBox_InsertString(Context->PathControlHandle, UINT_MAX, savedChoice->Buffer);
        }

        PhDereferenceObject(savedChoice);

        i = indexOfDelim + 2;
    }

    PhDereferenceObject(savedChoices);
}

VOID EtpSaveComboBoxHistoryToSettings(
    _In_ PET_OBJECT_CONTEXT Context
    )
{
    PH_STRING_BUILDER savedChoices;
    PPH_STRING choice;

    PhInitializeStringBuilder(&savedChoices, 100);

    for (ULONG i = 0; i < PH_CHOICE_DIALOG_SAVED_CHOICES * 2; i++)
    {
        choice = PhGetComboBoxString(Context->PathControlHandle, i);

        if (!choice)
            break;

        PhAppendStringBuilder(&savedChoices, &choice->sr);
        PhDereferenceObject(choice);

        PhAppendStringBuilder2(&savedChoices, ET_OBJMGR_HISTORY_SEPARATOR);
    }

    if (PhEndsWithString2(savedChoices.String, ET_OBJMGR_HISTORY_SEPARATOR, FALSE))
        PhRemoveEndStringBuilder(&savedChoices, 2);

    PhSetStringSetting2(SETTING_NAME_OBJMGR_HISTORY, &savedChoices.String->sr);
    PhDeleteStringBuilder(&savedChoices);
}

INT_PTR CALLBACK WinObjDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    PET_OBJECT_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(ET_OBJECT_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);

        if (PhBeginInitOnce(&initOnce))
        {
            EtObjectEntryType = PhCreateObjectType(L"ObjectItem", 0, EtpObjectEntryDeleteProcedure);

            EtAlpcPortTypeIndex = PhGetObjectTypeNumberZ(L"ALPC Port");
            EtDeviceTypeIndex = PhGetObjectTypeNumberZ(L"Device");
            EtFilterPortTypeIndex = PhGetObjectTypeNumberZ(L"FilterConnectionPort");
            EtFileTypeIndex = PhGetObjectTypeNumberZ(L"File");
            EtKeyTypeIndex = PhGetObjectTypeNumberZ(L"Key");
            EtSectionTypeIndex = PhGetObjectTypeNumberZ(L"Section");
            EtWinStaTypeIndex = PhGetObjectTypeNumberZ(L"WindowStation");

            ObjectSignaledString = PhCreateString(L"Signaled");
            ObjectNotificationString = PhCreateString(L"Notification");
            ObjectSignaledNotificationString = PhCreateString(L"Signaled, Notification");
            ObjectSyncronizationString = PhCreateString(L"Syncronization");
            ObjectSignaledSyncronizationString = PhCreateString(L"Signaled, Syncronization");
            ObjectAbandonedString = PhCreateString(L"Abandoned");
            PhEndInitOnce(&initOnce);
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
            COMBOBOXINFO info = { sizeof(COMBOBOXINFO) };

            context->WindowHandle = hwndDlg;
            context->ParentWindowHandle = (HWND)lParam;
            context->TreeViewHandle = GetDlgItem(hwndDlg, IDC_OBJMGR_TREE);
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_OBJMGR_LIST);
            context->SearchBoxHandle = GetDlgItem(hwndDlg, IDC_OBJMGR_SEARCH);
            context->PathControlHandle = GetDlgItem(hwndDlg, IDC_OBJMGR_PATH);

            if (GetComboBoxInfo(context->PathControlHandle, &info))
            {
                context->PathControlEdit = info.hwndItem;
                SetWindowFont(context->PathControlHandle, GetWindowFont(hwndDlg), TRUE);
            }

            context->CurrentDirectoryList = PhCreateList(100);
            if (!EtObjectManagerOwnHandles || !PhReferenceObjectSafe(EtObjectManagerOwnHandles))
                EtObjectManagerOwnHandles = PhCreateList(10);

            PhSetApplicationWindowIcon(hwndDlg);

            EtInitializeTreeImages(context);
            EtInitializeListImages(context);

            PhSetControlTheme(context->TreeViewHandle, L"explorer");
            TreeView_SetExtendedStyle(context->TreeViewHandle, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
            TreeView_SetImageList(context->TreeViewHandle, context->TreeImageList, TVSIL_NORMAL);
            context->RootTreeObject = EtTreeViewAddItem(context->TreeViewHandle, TVI_ROOT, TRUE, &EtObjectManagerRootDirectoryObject);
            context->CurrentPath = PhCreateString2(&EtObjectManagerRootDirectoryObject);

            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhSetListViewStyle(context->ListViewHandle, TRUE, FALSE);
            PhSetExtendedListView(context->ListViewHandle);
            ListView_SetImageList(context->ListViewHandle, context->ListImageList, LVSIL_SMALL);
            PhAddListViewColumn(context->ListViewHandle, ETOBLVC_NAME, ETOBLVC_NAME, ETOBLVC_NAME, LVCFMT_LEFT, 445, L"Name");
            PhAddListViewColumn(context->ListViewHandle, ETOBLVC_TYPE, ETOBLVC_TYPE, ETOBLVC_TYPE, LVCFMT_LEFT, 150, L"Type");
            PhAddListViewColumn(context->ListViewHandle, ETOBLVC_TARGET, ETOBLVC_TARGET, ETOBLVC_TARGET, LVCFMT_LEFT, 200, L"Target");
            context->UseAddressColumn = KsiLevel() >= KphLevelMed;
            if (context->UseAddressColumn)
                PhAddListViewColumn(context->ListViewHandle, ETOBLVC_OBJECT, ETOBLVC_OBJECT, ETOBLVC_OBJECT, LVCFMT_LEFT, 120, L"Object address");
            PhLoadListViewColumnsFromSetting(SETTING_NAME_OBJMGR_COLUMNS, context->ListViewHandle);

            PH_INTEGER_PAIR sortSettings;
            sortSettings = PhGetIntegerPairSetting(SETTING_NAME_OBJMGR_LIST_SORT);

            ExtendedListView_SetContext(context->ListViewHandle, context);
            ExtendedListView_SetTriState(context->ListViewHandle, TRUE);
            ExtendedListView_SetSort(context->ListViewHandle, (ULONG)sortSettings.X, (PH_SORT_ORDER)sortSettings.Y);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 0, WinObjNameCompareFunction);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 1, WinObjTypeCompareFunction);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 2, WinObjTargetCompareFunction);
            if (context->UseAddressColumn)
                ExtendedListView_SetCompareFunction(context->ListViewHandle, 3, WinObjObjectCompareFunction);
            ExtendedListView_SetTriStateCompareFunction(context->ListViewHandle, WinObjTriStateCompareFunction);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->PathControlHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->SearchBoxHandle, NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeViewHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PhCreateSearchControl(
                hwndDlg,
                context->SearchBoxHandle,
                L"Search Objects (Ctrl+K)",
                EtpObjectManagerSearchControlCallback,
                context
                );

            context->StatusBarHandle = CreateWindow(
                STATUSCLASSNAME,
                NULL,
                WS_VISIBLE | WS_CHILD | CCS_BOTTOM | SBARS_SIZEGRIP, // SBARS_TOOLTIPS
                0,
                0,
                0,
                0,
                hwndDlg,
                NULL,
                NULL,
                NULL
                );

            PhAddLayoutItem(&context->LayoutManager, context->StatusBarHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT | PH_LAYOUT_IMMEDIATE_RESIZE);

            if (PhGetIntegerPairSetting(SETTING_NAME_OBJMGR_WINDOW_POSITION).X != 0)
                PhLoadWindowPlacementFromSetting(SETTING_NAME_OBJMGR_WINDOW_POSITION, SETTING_NAME_OBJMGR_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, context->ParentWindowHandle);

            EtTreeViewEnumDirectoryObjects(
                context->TreeViewHandle,
                context->RootTreeObject,
                context->CurrentPath
                );

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

            {
                PPH_STRING Target = PH_AUTO(PhGetStringSetting(SETTING_NAME_OBJMGR_LAST_PATH));

                if (PhIsNullOrEmptyString(Target))  // HACK
                {
                    Target = PH_AUTO(PhCreateString2(&EtObjectManagerRootDirectoryObject));
                }

                EtpLoadComboBoxHistoryToSettings(context);

                context->DisableSelChanged = TRUE;
                EtpObjectManagerOpenTarget(context, Target);
                TreeView_EnsureVisible(context->TreeViewHandle, context->SelectedTreeItem);
                context->DisableSelChanged = FALSE;

                EtEnumCurrentDirectoryObjects(context);
            }

            PhSetDialogFocus(hwndDlg, context->TreeViewHandle);
            ExtendedListView_SetColumnWidth(context->ListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
        }
        break;
    case WM_DESTROY:
        {
            PPH_STRING currentPath = PhReferenceObject(context->CurrentPath);
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

            PhSaveWindowPlacementToSetting(SETTING_NAME_OBJMGR_WINDOW_POSITION, SETTING_NAME_OBJMGR_WINDOW_SIZE, hwndDlg);
            PhSaveListViewColumnsToSetting(SETTING_NAME_OBJMGR_COLUMNS, context->ListViewHandle);

            ExtendedListView_GetSort(context->ListViewHandle, &sortColumn, &sortOrder);
            sortSettings.X = sortColumn;
            sortSettings.Y = sortOrder;
            PhSetIntegerPairSetting(SETTING_NAME_OBJMGR_LIST_SORT, sortSettings);

            PhSetStringSetting(SETTING_NAME_OBJMGR_LAST_PATH, PhGetString(currentPath));
            PhDereferenceObject(currentPath);

            EtpSaveComboBoxHistoryToSettings(context);

            PhDeleteLayoutManager(&context->LayoutManager);

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
                EtpObjectManagerRefresh(context);
                break;
            default:
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELENDOK &&
                        GET_WM_COMMAND_HWND(wParam, lParam) == context->PathControlHandle)
                    {
                        PPH_STRING newTarget = PH_AUTO(PhGetComboBoxString(context->PathControlHandle, ComboBox_GetCurSel(context->PathControlHandle)));

                        if (!PhIsNullOrEmptyString(newTarget))
                        {
                            EtpObjectManagerOpenTarget(context, newTarget);
                            //SetFocus(context->ListViewHandle);
                        }
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
            case TVN_SELCHANGED:
                {
                    if (!context->DisableSelChanged &&
                        header->hwndFrom == context->TreeViewHandle)
                    {
                        EtpObjectManagerChangeSelection(context);
                    }
                }
                break;
            case TVN_GETDISPINFO:
                {
                    NMTVDISPINFO* dispInfo = (NMTVDISPINFO*)header;

                    if (header->hwndFrom == context->TreeViewHandle && FlagOn(dispInfo->item.mask, TVIF_TEXT))
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
                        PET_OBJECT_ENTRY entry = (PET_OBJECT_ENTRY)dispInfo->item.lParam;

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
                            case ETOBLVC_OBJECT:
                                dispInfo->item.pszText = !entry->TargetIsResolving ? entry->ObjectString : L"Resolving...";
                                break;
                        }
                    }
                }
                break;
            case LVN_ITEMCHANGED:
                {
                    LPNMLISTVIEW info = (LPNMLISTVIEW)header;

                    if (header->hwndFrom == context->ListViewHandle && info->uChanged & LVIF_STATE && info->uNewState & (LVIS_ACTIVATING | LVIS_FOCUSED))
                    {
                        PET_OBJECT_ENTRY entry = (PET_OBJECT_ENTRY)info->lParam;

                        PhSetWindowText(
                            context->PathControlHandle,
                            PhGetString(PH_AUTO_T(PH_STRING, EtGetObjectFullPath(entry->BaseDirectory, entry->Name)))
                            );
                        Edit_SetSel(context->PathControlEdit, -2, -1);  // HACK
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
                    PET_OBJECT_ENTRY entry;

                    if (header->hwndFrom == context->ListViewHandle &&
                        (entry = PhGetSelectedListViewItemParam(context->ListViewHandle)))
                    {
                        if (GetKeyState(VK_CONTROL) < 0)
                        {
                            EtpObjectManagerOpenSecurity(entry);
                        }
                        else if (entry->EtObjectType == EtObjectSymLink && !(GetKeyState(VK_SHIFT) < 0))
                        {
                            if (!PhIsNullOrEmptyString(entry->Target))
                                EtpObjectManagerOpenTarget(context, entry->TargetDrvLow ? entry->TargetDrvLow : entry->Target); // HACK
                        }
                        else
                        {
                            EtpObjectManagerObjectProperties(entry);
                        }
                    }
                }
                break;
            case LVN_KEYDOWN:
                if (header->hwndFrom == context->ListViewHandle)
                {
                    LPNMLVKEYDOWN keyDown = (LPNMLVKEYDOWN)lParam;
                    PET_OBJECT_ENTRY* listviewItems;
                    ULONG numberOfItems;

                    if ((keyDown->wVKey == 'C' || keyDown->wVKey == 'H') && GetKeyState(VK_CONTROL))
                    {
                        PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);
                        if (numberOfItems == 1)
                        {
                            if (keyDown->wVKey == 'C')
                            {
                                if (GetKeyState(VK_MENU) < 0)
                                {
                                    PhSetClipboardString(hwndDlg, &PH_AUTO_T(PH_STRING, EtGetObjectFullPath(listviewItems[0]->BaseDirectory, listviewItems[0]->Name))->sr);
                                    break;
                                }
                                else if (GetKeyState(VK_SHIFT) < 0)
                                {
                                    EtpObjectManagerCopyObjectAddress(listviewItems[0]);
                                    break;
                                }
                            }
                            else
                            {
                                EtpObjectManagerObjectHandles(listviewItems[0]);
                            }
                        }
                    }

                    PhHandleListViewNotifyBehaviors(lParam, context->ListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);
                }
                break;
            case TVN_KEYDOWN:
                if (header->hwndFrom == context->TreeViewHandle)
                {
                    LPNMTVKEYDOWN keyDown = (LPNMTVKEYDOWN)lParam;
                    PPH_STRING directory;

                    if ((keyDown->wVKey == 'C' || keyDown->wVKey == 'H') && GetKeyState(VK_CONTROL))
                    {
                        if (PhGetTreeViewItemParam(context->TreeViewHandle, context->SelectedTreeItem, &directory, NULL))
                        {
                            ET_CREATE_DIRECTORY_OBJECT_ENTRY(directory, context, entry);

                            if (keyDown->wVKey == 'C')
                            {
                                if (GetKeyState(VK_SHIFT) < 0)
                                    EtpObjectManagerCopyObjectAddress(&entry);
                                else if (GetKeyState(VK_MENU) < 0)
                                    PhSetClipboardString(hwndDlg, &context->CurrentPath->sr);
                                else
                                    PhSetClipboardString(hwndDlg, &directory->sr);
                            }
                            else
                            {
                                EtpObjectManagerObjectHandles(&entry);
                            }
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
                    PET_OBJECT_ENTRY entry = listviewItems[0];

                    menu = PhCreateEMenu();

                    BOOLEAN hasTarget = !PhIsNullOrEmptyString(entry->Target) && !entry->TargetIsInfoOnly;
                    BOOLEAN isSymlink = entry->EtObjectType == EtObjectSymLink;
                    BOOLEAN targetIsDriveVolume = isSymlink && entry->TargetDrvLow;
                    PPH_EMENU_ITEM propMenuItem;
                    PPH_EMENU_ITEM secMenuItem;
                    PPH_EMENU_ITEM gotoMenuItem = NULL;
                    PPH_EMENU_ITEM gotoMenuItem2 = NULL;
                    PPH_EMENU_ITEM copyPathMenuItem;
                    PPH_EMENU_ITEM copyAddressMenuItem;
                    PPH_EMENU_ITEM handlesMenuItem;

                    PhInsertEMenuItem(menu, propMenuItem = PhCreateEMenuItem(0, IDC_PROPERTIES,
                        !isSymlink ? L"Prope&rties\bEnter" : L"Prope&rties\bShift+Enter", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, handlesMenuItem = PhCreateEMenuItem(0, IDC_OPENHANDLES, L"&Handles\bCtrl+H", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);

                    if (isSymlink)
                    {
                        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), 0);
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
                        {
                            PhInsertEMenuItem(menu, gotoMenuItem = PhCreateEMenuItem(0, IDC_GOTODRIVER, L"&Go to device driver", NULL, NULL), ULONG_MAX);
                        }
                        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    }
                    else if (entry->EtObjectType == EtObjectAlpcPort)
                    {
                        PhInsertEMenuItem(menu, gotoMenuItem = PhCreateEMenuItem(0, IDC_GOTOPROCESS, L"&Go to process...", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    }
                    else if (entry->EtObjectType == EtObjectMutant)
                    {
                        PhInsertEMenuItem(menu, gotoMenuItem = PhCreateEMenuItem(0, IDC_GOTOTHREAD, L"&Go to thread...", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    }
                    else if (
                        entry->EtObjectType == EtObjectDriver ||
                        entry->EtObjectType == EtObjectSection
                        )
                    {
                        PhInsertEMenuItem(menu, gotoMenuItem = PhCreateEMenuItem(0, IDC_OPENFILELOCATION, L"&Open file location", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    }
                    if (entry->EtObjectType == EtObjectSymLink && hasTarget &&
                        ( PhStartsWithString2(entry->Target, L"\\SystemRoot", TRUE) ||
                          targetIsDriveVolume))
                    {
                        PhInsertEMenuItem(menu, gotoMenuItem = PhCreateEMenuItem(0, IDC_OPENFILELOCATION,
                            targetIsDriveVolume ? L"Sh&ow drive volume" : L"&Open file location", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    }

                    PhInsertEMenuItem(menu, secMenuItem = PhCreateEMenuItem(0, IDC_SECURITY, L"&Security\bCtrl+Enter", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, copyAddressMenuItem = PhCreateEMenuItem(0, IDC_COPYOBJECTADDRESS, L"Copy Object &Address\bCtrl+Shift+C", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, copyPathMenuItem = PhCreateEMenuItem(0, IDC_COPYPATH, L"Copy &Full Name\bCtrl+Alt+C", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy\bCtrl+C", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, IDC_COPY, context->ListViewHandle);
                    PhSetFlagsEMenuItem(menu, isSymlink ? IDC_OPENLINK : IDC_PROPERTIES, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

                    if (numberOfItems > 1)
                    {
                        PhSetDisabledEMenuItem(propMenuItem);
                        if (gotoMenuItem) PhSetDisabledEMenuItem(gotoMenuItem);
                        if (gotoMenuItem2) PhSetDisabledEMenuItem(gotoMenuItem2);
                        PhSetDisabledEMenuItem(secMenuItem);
                        PhSetDisabledEMenuItem(copyPathMenuItem);
                        PhSetDisabledEMenuItem(copyAddressMenuItem);
                        PhSetDisabledEMenuItem(handlesMenuItem);
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
                                    EtpObjectManagerObjectProperties(entry);
                                }
                                break;
                            case IDC_OPENLINK:
                                {
                                    EtpObjectManagerOpenTarget(context, entry->TargetDrvLow ? entry->TargetDrvLow : entry->Target); // HACK
                                }
                                break;
                            case IDC_GOTODRIVER:
                                {
                                    EtpObjectManagerOpenTarget(context, entry->TargetDrvLow);
                                }
                                break;
                            case IDC_GOTODRIVER2:
                                {
                                    EtpObjectManagerOpenTarget(context, entry->TargetDrvUp);
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
                                {
                                    PPH_STRING target = targetIsDriveVolume ? entry->TargetDrvLow : entry->Target; // HACK

                                    if (!PhIsNullOrEmptyString(target) &&
                                        (targetIsDriveVolume || PhDoesFileExist(&target->sr) || PhDoesFileExistWin32(target->Buffer)))
                                    {
                                        PhShellExecuteUserString(
                                            hwndDlg,
                                            L"FileBrowseExecutable",
                                            PhGetString(target),
                                            FALSE,
                                            L"Make sure the Explorer executable file is present."
                                        );
                                    }
                                    else
                                    {
                                        PhShowStatus(hwndDlg, L"Unable to locate the target.", STATUS_NOT_FOUND, 0);
                                    }
                                }
                                break;
                            case IDC_SECURITY:
                                {
                                    EtpObjectManagerOpenSecurity(entry);
                                }
                                break;
                            case IDC_COPY:
                                {
                                    PhCopyListView(context->ListViewHandle);
                                }
                                break;
                            case IDC_COPYPATH:
                                {
                                    PhSetClipboardString(hwndDlg, &PH_AUTO_T(PH_STRING, EtGetObjectFullPath(entry->BaseDirectory, entry->Name))->sr);
                                }
                                break;
                            case IDC_COPYOBJECTADDRESS:
                                {
                                    EtpObjectManagerCopyObjectAddress(entry);
                                }
                                break;
                            case IDC_OPENHANDLES:
                                {
                                    EtpObjectManagerObjectHandles(entry);
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
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_OPENHANDLES, L"&Handles\bCtrl+H", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_SECURITY, L"&Security\bCtrl+Enter", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPYOBJECTADDRESS, L"Copy Object &Address\bCtrl+Shift+C", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPYPATH, L"Copy &Full Name\bCtrl+Alt+C", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy\bCtrl+C", NULL, NULL), ULONG_MAX);

                    PhInsertCopyListViewEMenuItem(menu, IDC_COPYOBJECTADDRESS, context->ListViewHandle);

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
                            ET_CREATE_DIRECTORY_OBJECT_ENTRY(directory, context, entry);

                            switch (item->Id)
                            {
                            case IDC_PROPERTIES:
                                {
                                    EtpObjectManagerObjectProperties(&entry);
                                }
                                break;
                            case IDC_SECURITY:
                                {
                                    EtpObjectManagerOpenSecurity(&entry);
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
                            case IDC_COPYOBJECTADDRESS:
                                {
                                    EtpObjectManagerCopyObjectAddress(&entry);
                                }
                                break;
                            case IDC_OPENHANDLES:
                                {
                                    EtpObjectManagerObjectHandles(&entry);
                                }
                                break;
                            }
                        }
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
                    EtpObjectManagerRefresh(context);
                    return TRUE;
                }
                break;
            case VK_RETURN:
                if (GetFocus() == context->ListViewHandle)
                {
                    PET_OBJECT_ENTRY* listviewItems;
                    ULONG numberOfItems;

                    PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);
                    if (numberOfItems == 1)
                    {
                        if (listviewItems[0]->EtObjectType == EtObjectSymLink && GetKeyState(VK_SHIFT) < 0)
                        {
                            EtpObjectManagerObjectProperties(listviewItems[0]);
                        }
                        else if (GetKeyState(VK_CONTROL) < 0)
                        {
                            EtpObjectManagerOpenSecurity(listviewItems[0]);
                        }
                        else
                        {
                            if (listviewItems[0]->EtObjectType == EtObjectSymLink)
                            {
                                if (!PhIsNullOrEmptyString(listviewItems[0]->Target))
                                    EtpObjectManagerOpenTarget(context, listviewItems[0]->TargetDrvLow ? listviewItems[0]->TargetDrvLow : listviewItems[0]->Target); // HACK
                                else
                                    break;
                            }
                            else
                            {
                                EtpObjectManagerObjectProperties(listviewItems[0]);
                            }
                        }

                        return TRUE;
                    }
                }
                else if (GetFocus() == context->TreeViewHandle)
                {
                    PPH_STRING directory;

                    if (GetKeyState(VK_SHIFT) < 0)
                    {
                        if (PhGetTreeViewItemParam(context->TreeViewHandle, context->SelectedTreeItem, &directory, NULL))
                        {
                            ET_CREATE_DIRECTORY_OBJECT_ENTRY(directory, context, entry);
                            EtpObjectManagerObjectProperties(&entry);
                            return TRUE;
                        }
                    }
                    else if (GetKeyState(VK_CONTROL) < 0)
                    {
                        if (PhGetTreeViewItemParam(context->TreeViewHandle, context->SelectedTreeItem, &directory, NULL))
                        {
                            ET_CREATE_DIRECTORY_OBJECT_ENTRY(directory, context, entry);
                            EtpObjectManagerOpenSecurity(&entry);
                            return TRUE;
                        }
                    }
                }
                else if (GetFocus() == context->PathControlEdit)
                {
                    PPH_STRING newTarget = PH_AUTO(PhGetWindowText(context->PathControlHandle));
                    PPH_STRING comboEntry;
                    BOOLEAN alreadyExist = FALSE;

                    if (!PhIsNullOrEmptyString(newTarget) &&
                        EtpObjectManagerOpenTarget(context, newTarget))
                    {
                        for (INT i = 0; i < ComboBox_GetCount(context->PathControlHandle); i++)
                        {
                            comboEntry = PH_AUTO(PhGetComboBoxString(context->PathControlHandle, i));
                            if (PhEqualString(newTarget, comboEntry, TRUE))
                            {
                                alreadyExist = TRUE;
                                break;
                            }
                        }

                        if (!alreadyExist)
                            ComboBox_InsertString(context->PathControlHandle, 0, PhGetString(newTarget));
                    }
                }
                break;
        }
        break;
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

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == INT_ERROR)
            break;

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

    EtObjectManagerDialogHandle = NULL;

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
