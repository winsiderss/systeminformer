/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2024
 *
 */

#include "exttools.h"
#include <secedit.h>
#include <hndlinfo.h>

#include <kphcomms.h>
#include <kphuser.h>

static PH_STRINGREF EtObjectManagerRootDirectoryObject = PH_STRINGREF_INIT(L"\\"); // RtlNtPathSeparatorString
static PH_STRINGREF EtObjectManagerUserDirectoryObject = PH_STRINGREF_INIT(L"??"); // RtlDosDevicesPrefix
static PH_STRINGREF DirectoryObjectType = PH_STRINGREF_INIT(L"Directory");
HWND EtObjectManagerDialogHandle = NULL;
LARGE_INTEGER EtObjectManagerTimeCached = {0, 0};
static HANDLE EtObjectManagerDialogThreadHandle = NULL;
static PH_EVENT EtObjectManagerDialogInitializedEvent = PH_EVENT_INIT;

typedef struct _ET_OBJECT_ENTRY
{
    PPH_STRING Name;
    PPH_STRING TypeName;
    PPH_STRING Symlink;
    INT imageIndex;
} ET_OBJECT_ENTRY, *POBJECT_ENTRY;

typedef struct _ET_OBJECT_ITEM
{
    PPH_STRING Name;
} ET_OBJECT_ITEM, *POBJECT_ITEM;

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
    PPH_LIST CurrentDirectoryList;
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

NTSTATUS EtTreeViewEnumDirectoryObjects(
    _In_ HWND TreeViewHandle,
    _In_ HTREEITEM RootTreeItem,
    _In_ PH_STRINGREF DirectoryPath
    );

NTSTATUS EtObjectManagerOpenHandle(
    _Out_ PHANDLE Handle,
    _In_ PHANDLE_OPEN_CONTEXT Context,
    _In_ ACCESS_MASK DesiredAccess
    );

NTSTATUS EtObjectManagerHandleCloseCallback(
    _In_ PVOID Context
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
        POBJECT_ITEM item;

        if (!PhGetTreeViewItemParam(Context->TreeViewHandle, treeItem, &item, NULL))
            break;

        if (treePath)
            treePath = PH_AUTO(PhConcatStringRef3(&item->Name->sr, &PhNtPathSeperatorString, &treePath->sr));
        else
            treePath = PH_AUTO(PhCreateString2(&item->Name->sr));

        treeItem = TreeView_GetParent(Context->TreeViewHandle, treeItem);
    }

    if (!PhIsNullOrEmptyString(treePath))
    {
        return PhConcatStringRef2(&PhNtPathSeperatorString, &treePath->sr);
    }

    return PhCreateString2(&EtObjectManagerRootDirectoryObject);
}

HTREEITEM EtTreeViewAddItem(
    _In_ HWND TreeViewHandle,
    _In_ HTREEITEM Parent,
    _In_ BOOLEAN Expanded,
    _In_ PPH_STRINGREF Name
    )
{
    TV_INSERTSTRUCT insert;
    POBJECT_ITEM item;

    memset(&insert, 0, sizeof(TV_INSERTSTRUCT));
    insert.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
    insert.hInsertAfter = TVI_LAST;
    insert.hParent = Parent;
    insert.item.pszText = LPSTR_TEXTCALLBACK;

    if (Expanded)
    {
        insert.item.state = insert.item.stateMask = TVIS_EXPANDED;
    }

    item = PhAllocateZero(sizeof(ET_OBJECT_ITEM));
    item->Name = PhCreateString2(Name);
    insert.item.lParam = (LPARAM)item;

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
    POBJECT_ITEM item;
    ULONG children;

    for (
        current = TreeView_GetChild(TreeViewHandle, ParentTreeItem);
        current;
        current = TreeView_GetNextSibling(TreeViewHandle, current)
        )
    {
        if (PhGetTreeViewItemParam(TreeViewHandle, current, &item, &children))
        {
            if (PhEqualStringRef(&item->Name->sr, Name, TRUE))
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
    POBJECT_ITEM item;
    ULONG children;

    for (
        current = TreeView_GetChild(Context->TreeViewHandle, ParentTreeItem);
        current;
        current = TreeView_GetNextSibling(Context->TreeViewHandle, current)
        )
    {
        if (PhGetTreeViewItemParam(Context->TreeViewHandle, current, &item, &children))
        {
            if (children)
            {
                EtCleanupTreeViewItemParams(Context, current);
            }

            PhClearReference(&item->Name);
            PhFree(item);
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

    dpiValue = PhGetWindowDpi(Context->TreeViewHandle);
    size = PhGetDpi(20, dpiValue); // 24

    Context->ListImageList = PhImageListCreate(
        size,
        size,
        ILC_MASK | ILC_COLOR32,
        10, 10
        );

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_UNKNOWN), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_MUTANT), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_DRIVER), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_SECTION), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_LINK), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_KEY), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_PORT), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_SESSION), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_EVENT), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);

    icon = PhLoadIcon(PluginInstance->DllBase, MAKEINTRESOURCE(IDI_DEVICE), PH_LOAD_ICON_SIZE_LARGE, size, size, dpiValue);
    PhImageListAddIcon(Context->ListImageList, icon);
    DestroyIcon(icon);
}

static BOOLEAN NTAPI EtEnumDirectoryObjectsCallback(
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF TypeName,
    _In_ PDIRECTORY_ENUM_CONTEXT Context
    )
{
    if (PhEqualStringRef2(TypeName, L"Directory", TRUE))
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
    if (PhEqualStringRef2(TypeName, L"Directory", TRUE))
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
        BOOLEAN isSymlink = FALSE;
        BOOLEAN isDevice = FALSE;

        entry = PhAllocateZero(sizeof(ET_OBJECT_ENTRY));
        entry->Name = PhCreateString2(Name);
        entry->TypeName = PhCreateString2(TypeName);

        if (PhEqualStringRef2(TypeName, L"ALPC Port", TRUE))
        {
            entry->imageIndex = 6;
        }
        else if (PhEqualStringRef2(TypeName, L"Device", TRUE))
        {
            entry->imageIndex = 2;
            isDevice = TRUE;
        }
        else if (PhEqualStringRef2(TypeName, L"Driver", TRUE))
        {
            entry->imageIndex = 9;
        }
        else if (PhEqualStringRef2(TypeName, L"Event", TRUE))
        {
            entry->imageIndex = 8;
        }
        else if (PhEqualStringRef2(TypeName, L"Key", TRUE))
        {
            entry->imageIndex = 5;
        }
        else if (PhEqualStringRef2(TypeName, L"Mutant", TRUE))
        {
            entry->imageIndex = 1;
        }
        else if (PhEqualStringRef2(TypeName, L"Section", TRUE))
        {
            entry->imageIndex = 3;
        }
        else if (PhEqualStringRef2(TypeName, L"Session", TRUE))
        {
            entry->imageIndex = 7;
        }
        else if (PhEqualStringRef2(TypeName, L"SymbolicLink", TRUE))
        {
            entry->imageIndex = 4;
            isSymlink = TRUE;
        }
        else
        {
            entry->imageIndex = 0;
        }

        index = PhAddListViewItem(Context->ListViewHandle, MAXINT, PhGetStringRefZ(Name), entry);
        PhSetListViewItemImageIndex(Context->ListViewHandle, index, entry->imageIndex);
        PhSetListViewSubItem(Context->ListViewHandle, index, 1, PhGetStringRefZ(TypeName));

        if (isSymlink)
        {
            NTSTATUS status;
            HANDLE objectHandle;
            HANDLE_OPEN_CONTEXT objectContext;

            objectContext.CurrentPath = EtGetSelectedTreeViewPath(Context);
            objectContext.Object = entry;

            if (NT_SUCCESS(status = EtObjectManagerOpenHandle(&objectHandle, &objectContext, SYMBOLIC_LINK_QUERY)))
            {
                UNICODE_STRING targetName;
                WCHAR targetNameBuffer[DOS_MAX_PATH_LENGTH];

                RtlInitEmptyUnicodeString(&targetName, targetNameBuffer, sizeof(targetNameBuffer));
                NTSTATUS status = NtQuerySymbolicLinkObject(objectHandle, &targetName, NULL);
                NtClose(objectHandle);

                if (NT_SUCCESS(status))
                {
                    entry->Symlink = PhCreateStringFromUnicodeString(&targetName);

                    PhSetListViewSubItem(Context->ListViewHandle, index, 2, entry->Symlink->Buffer);
                }
            }

            PhDereferenceObject(objectContext.CurrentPath);
        }

        if (isDevice)
        {
            //NTSTATUS status;
            //HANDLE objectHandle;
            //HANDLE_OPEN_CONTEXT objectContext;
            //KPH_FILE_OBJECT_DRIVER fileObjectDriver;

            //objectContext.CurrentPath = EtGetSelectedTreeViewPath(Context);
            //objectContext.Object = entry;

            //// Extreamly slow open of \Device folder (several seconds), PhCreateFile returns STATUS_IO_TIMEOUT and it tooks forever
            //if (NT_SUCCESS(status = EtObjectManagerOpenHandle(&objectHandle, &objectContext, READ_CONTROL)))
            //{
            //    if (NT_SUCCESS(KphQueryInformationObject(
            //        NtCurrentProcess(),
            //        objectHandle,
            //        KphObjectFileObjectDriver,
            //        &fileObjectDriver,
            //        sizeof(fileObjectDriver),
            //        NULL
            //    )))
            //    {
            //        PPH_STRING driverName;

            //        if (NT_SUCCESS(PhGetDriverName(fileObjectDriver.DriverHandle, &driverName)))
            //        {
            //            entry->Symlink = driverName;

            //            PhSetListViewSubItem(Context->ListViewHandle, index, 2, entry->Symlink->Buffer);
            //        }

            //        NtClose(fileObjectDriver.DriverHandle);
            //    }

            //    NtClose(objectHandle);
            //}

            //PhDereferenceObject(objectContext.CurrentPath);
        }

        PhAddItemList(Context->CurrentDirectoryList, entry);

        if (Context->CurrentDirectoryList->Count == 1)      // HACK
            PhSetWindowText(Context->SearchBoxHandle, L"");
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

NTSTATUS EtEnumCurrentDirectoryObjects(
    _In_ POBJECT_CONTEXT Context,
    _In_ PH_STRINGREF DirectoryPath
    )
{
    NTSTATUS status;
    HANDLE directoryHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;

    //if (!EtTreeViewFindItem(Context->TreeViewHandle, Context->SelectedTreeItem, &DirectoryPath))
    //{
    //    EtTreeViewAddItem(Context->TreeViewHandle, Context->SelectedTreeItem, TRUE, &DirectoryPath);
    //}

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

    WCHAR string[PH_INT32_STR_LEN_1];
    PhPrintUInt32(string, Context->CurrentDirectoryList->Count);
    PhSetWindowText(GetDlgItem(Context->WindowHandle, IDC_OBJMGR_COUNT), string);

    PPH_STRING currentPath = PH_AUTO(EtGetSelectedTreeViewPath(Context));
    PhSetWindowText(GetDlgItem(Context->WindowHandle, IDC_OBJMGR_PATH), PhGetString(currentPath));

    //ExtendedListView_SortItems(Context->ListViewHandle);

    return STATUS_SUCCESS;
}

VOID EtObjectManagerFreeListViewItems(
    _In_ POBJECT_CONTEXT Context
    )
{
    INT index = INT_ERROR;

    while ((index = PhFindListViewItemByFlags(
        Context->ListViewHandle,
        index,
        LVNI_ALL
        )) != INT_ERROR)
    {
        POBJECT_ENTRY param;

        if (PhGetListViewItemParam(Context->ListViewHandle, index, &param))
        {
            PhClearReference(&param->Name);
            PhClearReference(&param->TypeName);
            if (param->Symlink)
                PhClearReference(&param->Symlink);
            PhFree(param);
        }
    }

    PhClearList(Context->CurrentDirectoryList);
}

NTSTATUS EtObjectManagerOpenHandle(
    _Out_ PHANDLE Handle,
    _In_ PHANDLE_OPEN_CONTEXT Context,
    _In_ ACCESS_MASK DesiredAccess
    )
{
    NTSTATUS status;
    HANDLE directoryHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;

    *Handle = NULL;

    if (!Context->Object->TypeName)
        return STATUS_INVALID_PARAMETER;

    PhStringRefToUnicodeString(&Context->CurrentPath->sr, &objectName);
    InitializeObjectAttributes(&objectAttributes, &objectName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    if (!NT_SUCCESS(status = NtOpenDirectoryObject(
        &directoryHandle,
        DIRECTORY_QUERY,
        &objectAttributes
        )))
        return status;

    if (PhEqualStringRef2(&Context->Object->TypeName->sr, L"DirectoryObject", TRUE))
    {
        status = NtOpenDirectoryObject(Handle, DesiredAccess, &objectAttributes);
        NtClose(directoryHandle);
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

    if (PhEqualString2(Context->Object->TypeName, L"ALPC Port", TRUE))
    {
        static PH_INITONCE initOnce = PH_INITONCE_INIT;
        NTSTATUS (NTAPI* NtAlpcConnectPortEx_I)(
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

            if (PhEqualStringRef(&Context->CurrentPath->sr, &EtObjectManagerRootDirectoryObject, TRUE))
                clientName = PhFormatString(L"%s%s", PhGetString(Context->CurrentPath), PhGetString(Context->Object->Name));
            else
                clientName = PhFormatString(L"%s\\%s", PhGetString(Context->CurrentPath), PhGetString(Context->Object->Name));
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
    else if (PhEqualString2(Context->Object->TypeName, L"Device", TRUE))
    {
        PPH_STRING deviceName;
        deviceName = PhFormatString(L"%s\\%s", PhGetString(Context->CurrentPath), PhGetString(Context->Object->Name));
        status = PhCreateFile(Handle, &deviceName->sr, FILE_GENERIC_READ, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN, 0);
        PhDereferenceObject(deviceName);
    }
    else if (PhEqualString2(Context->Object->TypeName, L"Driver", TRUE))
    {
        status = PhOpenDriver(Handle, DesiredAccess, directoryHandle, &Context->Object->Name->sr);
    }
    else if (PhEqualStringRef2(&Context->Object->TypeName->sr, L"Event", TRUE))
    {
        status = NtOpenEvent(Handle, DesiredAccess, &objectAttributes);
    }
    else if (PhEqualStringRef2(&Context->Object->TypeName->sr, L"EventPair", TRUE))
    {
        status = NtOpenEventPair(Handle, DesiredAccess, &objectAttributes);
    }
    else if (PhEqualStringRef2(&Context->Object->TypeName->sr, L"Timer", TRUE))
    {
        status = NtOpenTimer(Handle, DesiredAccess, &objectAttributes);
    }
    else if (PhEqualStringRef2(&Context->Object->TypeName->sr, L"Job", TRUE))
    {
        status = NtOpenJobObject(Handle, DesiredAccess, &objectAttributes);
    }
    else if (PhEqualStringRef2(&Context->Object->TypeName->sr, L"Key", TRUE))
    {
        status = NtOpenKey(Handle, DesiredAccess, &objectAttributes);
    }
    else if (PhEqualString2(Context->Object->TypeName, L"KeyedEvent", TRUE))
    {
        status = NtOpenKeyedEvent(Handle, DesiredAccess, &objectAttributes);
    }
    else if (PhEqualStringRef2(&Context->Object->TypeName->sr, L"Mutant", TRUE))
    {
        status = NtOpenMutant(Handle, DesiredAccess, &objectAttributes);
    }
    else if (PhEqualStringRef2(&Context->Object->TypeName->sr, L"Semaphore", TRUE))
    {
        status = NtOpenSemaphore(Handle, DesiredAccess, &objectAttributes);
    }
    else if (PhEqualStringRef2(&Context->Object->TypeName->sr, L"Section", TRUE))
    {
        status = NtOpenSection(Handle, DesiredAccess, &objectAttributes);
    }
    else if (PhEqualStringRef2(&Context->Object->TypeName->sr, L"Session", TRUE))
    {
        status = NtOpenSession(Handle, DesiredAccess, &objectAttributes);
    }
    else if (PhEqualStringRef2(&Context->Object->TypeName->sr, L"SymbolicLink", TRUE))
    {
        status = NtOpenSymbolicLinkObject(Handle, DesiredAccess, &objectAttributes);
    }
    else if (PhEqualStringRef2(&Context->Object->TypeName->sr, L"WindowStation", TRUE))
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
    else if (PhEqualString2(Context->Object->TypeName, L"FilterConnectionPort", TRUE))
    {
        PPH_STRING clientName;  
        HANDLE portHandle;
    
        if (PhEqualStringRef(&Context->CurrentPath->sr, &EtObjectManagerRootDirectoryObject, TRUE))
            clientName = PhFormatString(L"%s%s", PhGetString(Context->CurrentPath), PhGetString(Context->Object->Name));
        else
            clientName = PhFormatString(L"%s\\%s", PhGetString(Context->CurrentPath), PhGetString(Context->Object->Name));
        PhStringRefToUnicodeString(&clientName->sr, &objectName);

        status = PhFilterConnectCommunicationPort(
            &clientName->sr,
            0,
            NULL,
            0,
            NULL,
            &portHandle
        );

//#include <fltuser.h>
//#pragma comment(lib, "FltLib.lib")
//        status = FilterConnectCommunicationPort(
//            PhGetString(clientName),
//            0,
//            NULL,
//            0,
//            NULL,
//            &portHandle
//            );

        PhDereferenceObject(clientName);
    
        if (status == ERROR_SUCCESS)
        {
            *Handle = portHandle;
        }
    }
    else if (PhEqualString2(Context->Object->TypeName, L"Partition", TRUE))
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

    NtClose(directoryHandle);

    return status;
}

NTSTATUS EtObjectManagerHandleOpenCallback(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PVOID Context
    )
{
    return EtObjectManagerOpenHandle(Handle, Context, DesiredAccess);
}

NTSTATUS EtObjectManagerHandleCloseCallback(
    _In_ PVOID Context
    )
{
    PHANDLE_OPEN_CONTEXT context = Context;

    if (context->Object)
    {
        POBJECT_ENTRY entry = context->Object;
        PhClearReference(&entry->Name);
        PhClearReference(&entry->TypeName);
        PhFree(entry);
    }
    if (context->CurrentPath)
    {
        PhClearReference(&context->CurrentPath);
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

INT NTAPI WinObjSymlinkCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
)
{
    POBJECT_ENTRY item1 = Item1;
    POBJECT_ENTRY item2 = Item2;

    return PhCompareStringZ(PhGetStringOrEmpty(item1->Symlink), PhGetStringOrEmpty(item2->Symlink), TRUE);
}

NTSTATUS NTAPI EtObjectManagerObjectProperties(
    _In_ HWND hwndDlg,
    _In_ POBJECT_CONTEXT context,
    _In_ POBJECT_ENTRY entry)
{
    NTSTATUS status;
    HANDLE objectHandle;
    HANDLE_OPEN_CONTEXT objectContext;

    objectContext.CurrentPath = EtGetSelectedTreeViewPath(context);
    objectContext.Object = entry;

    if (NT_SUCCESS(status = EtObjectManagerOpenHandle(&objectHandle, &objectContext, READ_CONTROL)))
    {
        if (objectHandle) {
            OBJECT_BASIC_INFORMATION objectInfo;
            PPH_STRING bestObjectName;
            PPH_HANDLE_ITEM handleItem;
            SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX HandleInfo;

            memset(&HandleInfo, 0, sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX));
            handleItem = PhCreateHandleItem(&HandleInfo);

            // Do not save window position at close
            PhCopyStringZ(L"PH_PLUGIN", RTL_NUMBER_OF(L"PH_PLUGIN"), handleItem->HandleString, RTL_NUMBER_OF(handleItem->HandleString), NULL);
            handleItem->ObjectName = PhReferenceObject(entry->Name);

            if (PhEqualStringRef2(&entry->TypeName->sr, L"Device", TRUE) ||
                PhEqualStringRef2(&entry->TypeName->sr, L"FilterConnectionPort", TRUE))
            {
                handleItem->TypeName = PhCreateString(L"File");
            }
            else
                handleItem->TypeName = PhReferenceObject(entry->TypeName);

            // Get object address using KSystemInformer
            if (KsiLevel() >= KphLevelMed)
            {
                PKPH_PROCESS_HANDLE_INFORMATION handles;
                ULONG i;
                if (NT_SUCCESS(status = KsiEnumerateProcessHandles(NtCurrentProcess(), &handles)))
                {
                    for (i = 0; i < handles->HandleCount; i++)
                    {
                        if (handles->Handles[i].Handle == objectHandle)
                        {
                            handleItem->Object = handles->Handles[i].Object;
                            break;
                        }
                    }
                    PhFree(handles);
                }
            }

            if (NT_SUCCESS(status = PhGetHandleInformation(
                NtCurrentProcess(),
                objectHandle,
                -1,
                &objectInfo,
                NULL,
                NULL,
                &bestObjectName
            )))
            {
                handleItem->Handle = objectHandle;
                // We will remove access row in EtHandlePropertiesWindowPreOpen callback
                //handleItem->GrantedAccess = objectInfo.GrantedAccess;
                handleItem->Attributes = objectInfo.Attributes;
                handleItem->BestObjectName = bestObjectName;
                EtObjectManagerTimeCached = objectInfo.CreationTime;
            }

            status = PhShowHandlePropertiesModal(hwndDlg, NtCurrentProcessId(), handleItem);

            NtClose(objectHandle);
        }
    }

    PhDereferenceObject(objectContext.CurrentPath);

    if (!NT_SUCCESS(status))
        PhShowStatus(hwndDlg, L"Unable to open object type.", status, 0);

    return status;
}

VOID NTAPI EtObjectManagerOpenSymlink(
    _In_ HWND hwndDlg,
    _In_ POBJECT_CONTEXT context,
    _In_ POBJECT_ENTRY entry)
{
    NTSTATUS status;
    HANDLE objectHandle;
    HANDLE_OPEN_CONTEXT objectContext;

    objectContext.CurrentPath = EtGetSelectedTreeViewPath(context);
    objectContext.Object = entry;

    if (NT_SUCCESS(status = EtObjectManagerOpenHandle(&objectHandle, &objectContext, SYMBOLIC_LINK_QUERY)))
    {
        UNICODE_STRING targetName;
        WCHAR targetNameBuffer[DOS_MAX_PATH_LENGTH];

        RtlInitEmptyUnicodeString(&targetName, targetNameBuffer, sizeof(targetNameBuffer));
        status = NtQuerySymbolicLinkObject(objectHandle, &targetName, NULL);
        NtClose(objectHandle);

        if (NT_SUCCESS(status))
        {
            PH_STRINGREF directoryPart = PhGetStringRef(NULL);
            PH_STRINGREF remainingPart;
            HTREEITEM selectedTreeItem;
            PPH_STRING selectedPath;

            selectedPath = PhCreateStringFromUnicodeString(&targetName);
            remainingPart = PhGetStringRef(selectedPath);
            selectedTreeItem = context->RootTreeObject;

            while (remainingPart.Length != 0)
            {
                PhSplitStringRefAtChar(&remainingPart, OBJ_NAME_PATH_SEPARATOR, &directoryPart, &remainingPart);

                if (directoryPart.Length != 0)
                {
                    HTREEITEM directoryItem = EtTreeViewFindItem(
                        context->TreeViewHandle,
                        selectedTreeItem,
                        &directoryPart
                    );

                    if (directoryItem)
                    {
                        TreeView_SelectItem(context->TreeViewHandle, directoryItem);
                        selectedTreeItem = directoryItem;
                    }
                }
            }

            if (directoryPart.Length > 0) {     // HACK
                NTSTATUS status;
                HANDLE directoryHandle;
                OBJECT_ATTRIBUTES objectAttributes;
                InitializeObjectAttributes(
                    &objectAttributes,
                    &targetName,
                    OBJ_CASE_INSENSITIVE,
                    NULL,
                    NULL
                );

                // Check if target was directory
                status = NtOpenDirectoryObject(
                    &directoryHandle,
                    DIRECTORY_QUERY,
                    &objectAttributes
                );

                if (NT_SUCCESS(status))
                    NtClose(directoryHandle);
                else
                {
                    LVFINDINFO findinfo;
                    findinfo.psz = directoryPart.Buffer;
                    findinfo.flags = LVFI_STRING;

                    int item = ListView_FindItem(context->ListViewHandle, -1, &findinfo);

                    // Navigate to target object
                    if (item != INT_ERROR) {
                        ListView_SetItemState(context->ListViewHandle, item, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                        ListView_EnsureVisible(context->ListViewHandle, item, TRUE);
                        //ListView_Scroll(context->ListViewHandle, 0, 200);
                    }
                }
            }

            PhDereferenceObject(selectedPath);
        }
    }

    PhDereferenceObject(objectContext.CurrentPath);

    if (!NT_SUCCESS(status))
        PhShowStatus(hwndDlg, L"Unable to open object type.", status, 0);
}

VOID NTAPI EtObjectManagerRefresh(
    _In_ HWND hwndDlg,
    _In_ POBJECT_CONTEXT context
    )
{
    PPH_STRING selectedPath = NULL;
    PPH_STRING oldFilter = PhGetWindowText(context->SearchBoxHandle);
    BOOLEAN rootWasSelected = context->SelectedTreeItem == context->RootTreeObject;
    HWND countControl = GetDlgItem(hwndDlg, IDC_OBJMGR_COUNT);
    PVOID* listviewItems;
    ULONG numberOfItems;
    PPH_STRING oldSelect = NULL;

    if (!rootWasSelected)
        selectedPath = EtGetSelectedTreeViewPath(context);

    PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);
    if (numberOfItems != 0)
        oldSelect = PhReferenceObject(((POBJECT_ENTRY)listviewItems[0])->Name);

    SendMessage(context->TreeViewHandle, WM_SETREDRAW, FALSE, 0);
    ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);
    SendMessage(countControl, WM_SETREDRAW, FALSE, 0);

    EtObjectManagerFreeListViewItems(context);
    ListView_DeleteAllItems(context->ListViewHandle);
    TreeView_DeleteAllItems(context->TreeViewHandle);
    PhClearList(context->CurrentDirectoryList);

    context->RootTreeObject = EtTreeViewAddItem(context->TreeViewHandle, TVI_ROOT, TRUE, &EtObjectManagerRootDirectoryObject);

    EtTreeViewEnumDirectoryObjects(
        context->TreeViewHandle,
        context->RootTreeObject,
        EtObjectManagerRootDirectoryObject
    );

    if (rootWasSelected)
        TreeView_SelectItem(context->TreeViewHandle, context->RootTreeObject);
    else
    {
        PH_STRINGREF directoryPart;
        PH_STRINGREF remainingPart;
        HTREEITEM selectedTreeItem;

        directoryPart = PhGetStringRef(NULL);
        remainingPart = PhGetStringRef(selectedPath);
        selectedTreeItem = context->RootTreeObject;

        while (remainingPart.Length != 0)
        {
            PhSplitStringRefAtChar(&remainingPart, OBJ_NAME_PATH_SEPARATOR, &directoryPart, &remainingPart);

            if (directoryPart.Length != 0)
            {
                HTREEITEM directoryItem = EtTreeViewFindItem(
                    context->TreeViewHandle,
                    selectedTreeItem,
                    &directoryPart
                );

                if (directoryItem)
                {
                    TreeView_SelectItem(context->TreeViewHandle, directoryItem);
                    selectedTreeItem = directoryItem;
                }
            }
        }
        PhDereferenceObject(selectedPath);
    }

    if (oldFilter->Length)
    {
        SetFocus(context->SearchBoxHandle);
        PhSetWindowText(context->SearchBoxHandle, PhGetString(oldFilter));
        SendMessage(context->SearchBoxHandle, EM_SETSEL, -2, -1);
    }
    PhDereferenceObject(oldFilter);

    // Reselect previously active listview item
    if (oldSelect)
    {
        LVFINDINFO findinfo;
        findinfo.psz = oldSelect->Buffer;
        findinfo.flags = LVFI_STRING;

        int item = ListView_FindItem(context->ListViewHandle, -1, &findinfo);

        if (item != INT_ERROR) {
            ListView_SetItemState(context->ListViewHandle, item, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            ListView_EnsureVisible(context->ListViewHandle, item, TRUE);
        }

        PhDereferenceObject(oldSelect);
    }

    SendMessage(context->TreeViewHandle, WM_SETREDRAW, TRUE, 0);
    ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);

    SendMessage(countControl, WM_SETREDRAW, TRUE, 0);
    WCHAR string[PH_INT32_STR_LEN_1];
    PhPrintUInt32(string, context->CurrentDirectoryList->Count);
    PhSetWindowText(countControl, string);
}

VOID NTAPI EtpObjectManagerOpenSecurity(
    _In_ HWND hwndDlg,
    _In_ POBJECT_CONTEXT context,
    _In_ POBJECT_ENTRY entry
    )
{
    POBJECT_ENTRY objectEntry;
    PHANDLE_OPEN_CONTEXT objectContext;

    objectContext = PhAllocateZero(sizeof(HANDLE_OPEN_CONTEXT));
    objectContext->CurrentPath = EtGetSelectedTreeViewPath(context);
    objectEntry = PhAllocateZero(sizeof(ET_OBJECT_ENTRY));
    PhReferenceObject(entry->Name);
    PhReferenceObject(entry->TypeName);
    objectContext->Object = entry;

    PhEditSecurity(
        hwndDlg,
        PhGetString(entry->Name),
        PhGetString(entry->TypeName),
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

    assert(context);

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
                entry->Symlink && PhSearchControlMatch(MatchHandle, &entry->Symlink->sr))
            {
                index = PhAddListViewItem(context->ListViewHandle, MAXINT, entry->Name->Buffer, entry);
                PhSetListViewItemImageIndex(context->ListViewHandle, index, entry->imageIndex);
                PhSetListViewSubItem(context->ListViewHandle, index, 1, entry->TypeName->Buffer);
                if (entry->Symlink != NULL)
                    PhSetListViewSubItem(context->ListViewHandle, index, 2, entry->Symlink->Buffer);
            }
        }
    }

    ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);
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
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 445, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 150, L"Type");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 200, L"Symbolic Link Target");
            PhLoadListViewColumnsFromSetting(SETTING_NAME_OBJMGR_COLUMNS, context->ListViewHandle);

            ExtendedListView_SetSortFast(context->ListViewHandle, TRUE);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 0, WinObjNameCompareFunction);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 1, WinObjTypeCompareFunction);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 2, WinObjSymlinkCompareFunction);

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

            SendMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM)context->TreeViewHandle, TRUE);
        }
        break;
    case WM_DESTROY:
        {
            EtObjectManagerFreeListViewItems(context);
            EtCleanupTreeViewItemParams(context, context->RootTreeObject);

            if (context->TreeImageList)
                PhImageListDestroy(context->TreeImageList);
            if (context->ListImageList)
                PhImageListDestroy(context->ListImageList);
            if (context->CurrentDirectoryList)
                PhDereferenceObject(context->CurrentDirectoryList);

            PhSaveWindowPlacementToSetting(SETTING_NAME_OBJMGR_WINDOW_POSITION, SETTING_NAME_OBJMGR_WINDOW_SIZE, hwndDlg);
            PhSaveListViewColumnsToSetting(SETTING_NAME_OBJMGR_COLUMNS, context->ListViewHandle);
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
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case WM_WINDOWPOSCHANGED:
        {
            LPWINDOWPOS pos = (LPWINDOWPOS)lParam;
            static int prev_w = -1;

            int new_w = pos->cx;
            if (prev_w == -1)
                prev_w = new_w;

            ListView_SetColumnWidth(context->ListViewHandle, 0,
                ListView_GetColumnWidth(context->ListViewHandle, 0) + (new_w - prev_w));
            prev_w = new_w;
        }
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                DestroyWindow(hwndDlg);
                break;
            case IDC_REFRESH:
                EtObjectManagerRefresh(hwndDlg, context);
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
                    if (header->hwndFrom != context->TreeViewHandle)
                        break;

                    context->SelectedTreeItem = TreeView_GetSelection(context->TreeViewHandle);

                    ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);
                    EtObjectManagerFreeListViewItems(context);
                    ListView_DeleteAllItems(context->ListViewHandle);

                    if (context->SelectedTreeItem == context->RootTreeObject)
                    {
                        EtEnumCurrentDirectoryObjects(context, EtObjectManagerRootDirectoryObject);
                    }
                    else
                    {
                        PPH_STRING selectedPath = EtGetSelectedTreeViewPath(context);

                        EtEnumCurrentDirectoryObjects(
                            context,
                            PhGetStringRef(selectedPath)
                            );

                        PhDereferenceObject(selectedPath);
                    }

                    ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);
                }
                break;
            case TVN_GETDISPINFO:
                {
                    NMTVDISPINFO* dispInfo = (NMTVDISPINFO*)header;

                    if (header->hwndFrom != context->TreeViewHandle)
                        break;

                    if (FlagOn(dispInfo->item.mask, TVIF_TEXT))
                    {
                        POBJECT_ITEM entry = (POBJECT_ITEM)dispInfo->item.lParam;

                        wcsncpy_s(dispInfo->item.pszText, dispInfo->item.cchTextMax, entry->Name->Buffer, _TRUNCATE);
                    }
                }
                break;
            case LVN_ITEMCHANGED:
                {
                    LPNMLISTVIEW info = (LPNMLISTVIEW)header;
                    POBJECT_ENTRY entry;

                    if (info->uChanged & LVIF_STATE && info->uNewState & (LVIS_ACTIVATING | LVIS_FOCUSED))
                    {
                        if (entry = PhGetSelectedListViewItemParam(context->ListViewHandle))
                        {
                            PPH_STRING fullPath;
                            PPH_STRING currentPath = PH_AUTO(EtGetSelectedTreeViewPath(context));

                            if (context->SelectedTreeItem != context->RootTreeObject)
                                fullPath = PH_AUTO(PhConcatStringRef3(&currentPath->sr, &PhNtPathSeperatorString, &entry->Name->sr));
                            else
                                fullPath = PH_AUTO(PhConcatStringRef2(&currentPath->sr, &entry->Name->sr));

                            PhSetWindowText(GetDlgItem(hwndDlg, IDC_OBJMGR_PATH), PhGetString(fullPath));
                        }
                    }
                }
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

                    if (entry = PhGetSelectedListViewItemParam(context->ListViewHandle))
                    {
                        if (PhEqualStringRef2(&entry->TypeName->sr, L"SymbolicLink", TRUE))
                            EtObjectManagerOpenSymlink(hwndDlg, context, entry);
                        else
                            EtObjectManagerObjectProperties(hwndDlg, context, entry);
                    }
                }
                break;
            }
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID *listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    POBJECT_ENTRY entry = listviewItems[0];
                    BOOLEAN isSymlink = !!PhEqualStringRef2(&entry->TypeName->sr, L"SymbolicLink", TRUE);

                    menu = PhCreateEMenu();
                    if (isSymlink)
                    {
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"&Open link", NULL, NULL), ULONG_MAX);
                    }
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1 + isSymlink, L"Prope&rties", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 2 + isSymlink, L"&Security", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 3 + isSymlink, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, 3 + isSymlink, context->ListViewHandle);

                    PhSetFlagsEMenuItem(menu, 1, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

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
                            ULONG id = item->Id;
                            if (isSymlink && id == 1)
                            {
                                EtObjectManagerOpenSymlink(hwndDlg, context, entry);
                            }
                            else if (id == 1 + isSymlink)
                            {
                                EtObjectManagerObjectProperties(hwndDlg, context, entry);
                            }
                            else if (id == 2 + isSymlink)
                            {
                                EtpObjectManagerOpenSecurity(hwndDlg, context, PhAllocateCopy(entry, sizeof(ET_OBJECT_ENTRY)));
                            }
                            else if (id == 3 + isSymlink)
                            {
                                PhCopyListView(context->ListViewHandle);
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
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
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
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"Prope&rties", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 2, L"&Security", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 3, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, 3, context->ListViewHandle);

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
                        POBJECT_ITEM directory;
                        POBJECT_ENTRY entry = NULL;

                        PhGetTreeViewItemParam(context->TreeViewHandle, context->SelectedTreeItem, &directory, NULL);

                        if (!PhHandleCopyListViewEMenuItem(item))
                        {
                            if (item->Id == 1 || item->Id == 2)
                            {
                                entry = PhAllocateZero(sizeof(ET_OBJECT_ENTRY));
                                entry->Name = directory->Name;
                                entry->TypeName = PhCreateString(L"DirectoryObject");
                            }

                            switch (item->Id)
                            {
                            case 1:
                                {
                                    EtObjectManagerObjectProperties(hwndDlg, context, entry);

                                    PhClearReference(&entry->TypeName);
                                    PhFree(entry);
                                }
                                break;
                            case 2:
                                {
                                    EtpObjectManagerOpenSecurity(hwndDlg, context, entry);
                                }
                                break;
                            case 3:
                                PhSetClipboardString(hwndDlg, &directory->Name->sr);
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
                PPH_EMENU menu;
                PPH_EMENU item;
                POINT point;
                RECT pathRect;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                GetWindowRect(pathControl, &pathRect);
                InflateRect(&pathRect, 0, 3);

                if (PtInRect(&pathRect, point))
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"&Copy Full Name", NULL, NULL), ULONG_MAX);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                    );

                    if (item && item->Id == 1)
                    {
                        PPH_STRING fullPath = PH_AUTO(PhGetWindowText(pathControl));
                        PhSetClipboardString(hwndDlg, &fullPath->sr);
                    }
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
                    EtObjectManagerRefresh(hwndDlg, context);
                    return TRUE;
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
            ((WNDPROC)GetWindowLongPtr(EtObjectManagerDialogHandle, GWLP_WNDPROC))(
                EtObjectManagerDialogHandle, message.message, message.wParam, message.lParam
                );
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
            PhShowError(ParentWindowHandle, L"%s", L"Unable to create the window.");
            return;
        }

        PhWaitForEvent(&EtObjectManagerDialogInitializedEvent, NULL);
    }

    PostMessage(EtObjectManagerDialogHandle, WM_PH_SHOW_DIALOG, 0, 0);
}
