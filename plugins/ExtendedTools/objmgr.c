/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2022
 *
 */

#include "exttools.h"
#include <secedit.h>

static PH_STRINGREF EtObjectManagerRootDirectoryObject = PH_STRINGREF_INIT(L"\\"); // RtlNtPathSeperatorString
static HWND EtObjectManagerDialogHandle = NULL;
static HANDLE EtObjectManagerDialogThreadHandle = NULL;
static PH_EVENT EtObjectManagerDialogInitializedEvent = PH_EVENT_INIT;

typedef struct _OBJECT_ENTRY
{
    PPH_STRING Name;
    PPH_STRING TypeName;
} OBJECT_ENTRY, *POBJECT_ENTRY;

typedef struct _OBJ_CONTEXT
{
    HWND WindowHandle;
    HWND ParentWindowHandle;
    HWND ListViewHandle;
    HWND TreeViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;

    HTREEITEM RootTreeObject;
    HTREEITEM SelectedTreeItem;

    HIMAGELIST TreeImageList;
    HIMAGELIST ListImageList;
} OBJ_CONTEXT, *POBJ_CONTEXT;

typedef struct _DIRECTORY_ENUM_CONTEXT
{
    HWND TreeViewHandle;
    HTREEITEM RootTreeItem;
    PH_STRINGREF DirectoryPath;
} DIRECTORY_ENUM_CONTEXT, *PDIRECTORY_ENUM_CONTEXT;

NTSTATUS EnumDirectoryObjects(
    _In_ HWND TreeViewHandle,
    _In_ HTREEITEM RootTreeItem,
    _In_ PH_STRINGREF DirectoryPath
    );

PPH_STRING EtGetSelectedTreeViewPath(
    _In_ POBJ_CONTEXT Context
    )
{
    HTREEITEM treeItem;
    PPH_STRING treePath = NULL;
    WCHAR buffer[MAX_PATH] = L"";

    treeItem = Context->SelectedTreeItem;

    while (treeItem != Context->RootTreeObject)
    {
        TVITEM tvItem;
        tvItem.mask = TVIF_PARAM | TVIF_HANDLE | TVIF_TEXT;
        tvItem.hItem = treeItem;
        tvItem.pszText = buffer;
        tvItem.cchTextMax = ARRAYSIZE(buffer);

        if (!TreeView_GetItem(Context->TreeViewHandle, &tvItem))
            break;

        if (treePath)
            treePath = PhaConcatStrings(3, buffer, RtlNtPathSeperatorString.Buffer, treePath->Buffer);
        else
            treePath = PhaCreateString(buffer);

        treeItem = TreeView_GetParent(Context->TreeViewHandle, treeItem);
    }

    return PhConcatStrings2(RtlNtPathSeperatorString.Buffer, PhGetStringOrEmpty(treePath));
}

HTREEITEM EtTreeViewAddItem(
    _In_ HWND TreeViewHandle,
    _In_ HTREEITEM Parent,
    _In_ BOOLEAN Expanded,
    _In_ PPH_STRINGREF Text
    )
{
    TV_INSERTSTRUCT insert;

    memset(&insert, 0, sizeof(TV_INSERTSTRUCT));
    insert.item.mask = TVIF_TEXT | TVIF_STATE;
    insert.hInsertAfter = TVI_LAST;
    insert.hParent = Parent;
    insert.item.pszText = Text->Buffer;

    if (Expanded)
    {
        insert.item.state = insert.item.stateMask = TVIS_EXPANDED;
    }

    return TreeView_InsertItem(TreeViewHandle, &insert);
}

HTREEITEM EtTreeViewFindItem(
    _In_ HWND TreeViewHandle,
    _In_ HTREEITEM ParentTreeItem,
    _In_ PWSTR Name
    )
{
    TVITEM treeItem;

    for (
        treeItem.hItem = TreeView_GetChild(TreeViewHandle, ParentTreeItem);
        treeItem.hItem;
        treeItem.hItem = TreeView_GetNextSibling(TreeViewHandle, treeItem.hItem)
        )
    {
        WCHAR itemText[MAX_PATH] = L"";

        treeItem.mask = TVIF_TEXT | TVIF_CHILDREN;
        treeItem.pszText = itemText;
        treeItem.cchTextMax = ARRAYSIZE(itemText);

        if (TreeView_GetItem(TreeViewHandle, &treeItem))
        {
            if (PhEqualStringZ(treeItem.pszText, Name, TRUE))
            {
                return treeItem.hItem;
            }

            if (treeItem.cChildren)
            {
                HTREEITEM item;

                if (item = EtTreeViewFindItem(TreeViewHandle, treeItem.hItem, Name))
                    return item;
            }
        }
    }

    return NULL;
}

VOID EtInitializeTreeImages(
    _In_ POBJ_CONTEXT Context
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
    _In_ POBJ_CONTEXT Context
    )
{
    HICON icon;
    LONG dpiValue;
    LONG size;

    dpiValue = PhGetWindowDpi(Context->TreeViewHandle);
    size = PhGetDpi(16, dpiValue); // 24

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

static BOOLEAN NTAPI EnumDirectoryObjectsCallback(
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF TypeName,
    _In_ PVOID Context
    )
{
    PDIRECTORY_ENUM_CONTEXT context = (PDIRECTORY_ENUM_CONTEXT)Context;

    if (PhEqualStringZ(TypeName->Buffer, L"Directory", TRUE))
    {
        PPH_STRING childDirectoryPath;
        HTREEITEM currentItem;

        if (PhEqualStringRef(&context->DirectoryPath, &EtObjectManagerRootDirectoryObject, TRUE))
        {
            childDirectoryPath = PhConcatStringRef2(
                &context->DirectoryPath,
                Name
                );
        }
        else
        {
            childDirectoryPath = PhConcatStrings(
                3,
                context->DirectoryPath.Buffer,
                RtlNtPathSeperatorString.Buffer,
                Name->Buffer
                );
        }

        currentItem = EtTreeViewAddItem(
            context->TreeViewHandle,
            context->RootTreeItem,
            FALSE,
            Name
            );

        // Recursively enumerate all subdirectories.
        EnumDirectoryObjects(
            context->TreeViewHandle,
            currentItem,
            PhGetStringRef(childDirectoryPath)
            );

        PhDereferenceObject(childDirectoryPath);
    }

    return TRUE;
}

static BOOLEAN NTAPI EnumCurrentDirectoryObjectsCallback(
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF TypeName,
    _In_ PVOID Context
    )
{
    POBJ_CONTEXT context = (POBJ_CONTEXT)Context;

    if (PhEqualStringZ(TypeName->Buffer, L"Directory", TRUE))
    {
        if (!EtTreeViewFindItem(context->TreeViewHandle, context->SelectedTreeItem, Name->Buffer))
        {
            EtTreeViewAddItem(context->TreeViewHandle, context->SelectedTreeItem, TRUE, Name);
        }
    }
    else
    {
        INT index;
        INT imageIndex;
        POBJECT_ENTRY entry;

        entry = PhAllocateZero(sizeof(OBJECT_ENTRY));
        entry->Name = PhCreateString2(Name);
        entry->TypeName = PhCreateString2(TypeName);

        if (PhEqualStringRef2(TypeName, L"ALPC Port", TRUE))
        {
            imageIndex = 6;
        }
        else if (PhEqualStringRef2(TypeName, L"Device", TRUE))
        {
            imageIndex = 2;
        }
        else if (PhEqualStringRef2(TypeName, L"Driver", TRUE))
        {
            imageIndex = 9;
        }
        else if (PhEqualStringRef2(TypeName, L"Event", TRUE))
        {
            imageIndex = 8;
        }
        else if (PhEqualStringRef2(TypeName, L"Key", TRUE))
        {
            imageIndex = 5;
        }
        else if (PhEqualStringRef2(TypeName, L"Mutant", TRUE))
        {
            imageIndex = 1;
        }
        else if (PhEqualStringRef2(TypeName, L"Section", TRUE))
        {
            imageIndex = 3;
        }
        else if (PhEqualStringRef2(TypeName, L"Session", TRUE))
        {
            imageIndex = 7;
        }
        else if (PhEqualStringRef2(TypeName, L"SymbolicLink", TRUE))
        {
            imageIndex = 4;
        }
        else
        {
            imageIndex = 0;
        }

        index = PhAddListViewItem(
            context->ListViewHandle,
            MAXINT,
            Name->Buffer,
            entry
            );

        PhSetListViewItemImageIndex(
            context->ListViewHandle,
            index,
            imageIndex
            );

        PhSetListViewSubItem(
            context->ListViewHandle,
            index,
            1,
            TypeName->Buffer
            );
    }

    return TRUE;
}

NTSTATUS EnumDirectoryObjects(
    _In_ HWND TreeViewHandle,
    _In_ HTREEITEM RootTreeItem,
    _In_ PH_STRINGREF DirectoryPath
    )
{
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

    if (NT_SUCCESS(NtOpenDirectoryObject(
        &directoryHandle,
        DIRECTORY_QUERY,
        &objectAttributes
        )))
    {
        DIRECTORY_ENUM_CONTEXT enumContext;

        enumContext.TreeViewHandle = TreeViewHandle;
        enumContext.RootTreeItem = RootTreeItem;
        enumContext.DirectoryPath = DirectoryPath;

        PhEnumDirectoryObjects(
            directoryHandle,
            EnumDirectoryObjectsCallback,
            &enumContext
            );

        NtClose(directoryHandle);
    }

    TreeView_SortChildren(TreeViewHandle, RootTreeItem, TRUE);

    return STATUS_SUCCESS;
}

NTSTATUS EnumCurrentDirectoryObjects(
    _In_ POBJ_CONTEXT Context,
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
        status = PhEnumDirectoryObjects(
            directoryHandle,
            EnumCurrentDirectoryObjectsCallback,
            Context
            );

        NtClose(directoryHandle);
    }

    if (!NT_SUCCESS(status) && status != STATUS_NO_MORE_ENTRIES)
    {
        PhShowStatus(Context->WindowHandle, L"Unable to query directory object.", status, 0);
    }

    ExtendedListView_SortItems(Context->ListViewHandle);

    return STATUS_SUCCESS;
}

typedef struct _HANDLE_OPEN_CONTEXT
{
    PPH_STRING CurrentPath;
    POBJECT_ENTRY Object;
} HANDLE_OPEN_CONTEXT, *PHANDLE_OPEN_CONTEXT;

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

    PhStringRefToUnicodeString(&Context->CurrentPath->sr, &objectName);
    InitializeObjectAttributes(&objectAttributes, &objectName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = NtOpenDirectoryObject(
        &directoryHandle,
        DIRECTORY_QUERY,
        &objectAttributes
        );

    if (!NT_SUCCESS(status))
        return status;

    if (PhEqualStringRef2(&Context->Object->TypeName->sr, L"DirectoryObject", TRUE))
    {
        return NtOpenDirectoryObject(Handle, DesiredAccess, &objectAttributes);
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
    else if (PhEqualString2(Context->Object->TypeName, L"Driver", TRUE))
    {
        status = PhOpenDriver(Handle, DesiredAccess, directoryHandle, &Context->Object->Name->sr);
    }
    else if (PhEqualStringRef2(&Context->Object->TypeName->sr, L"Event", TRUE))
    {
        status = NtOpenEvent(Handle, DesiredAccess, &objectAttributes);
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
    //else if (PhEqualString2(Context->Object->TypeName, L"FilterConnectionPort", TRUE))
    //{
    //    PPH_STRING clientName;
    //    HANDLE portHandle;
    //
    //    if (PhEqualStringRef(&Context->CurrentPath->sr, &EtObjectManagerRootDirectoryObject, TRUE))
    //        clientName = PhFormatString(L"%s%s", PhGetString(Context->CurrentPath), PhGetString(Context->Object->Name));
    //    else
    //        clientName = PhFormatString(L"%s\\%s", PhGetString(Context->CurrentPath), PhGetString(Context->Object->Name));
    //    PhStringRefToUnicodeString(&clientName->sr, &objectName);
    //
    //    status = FilterConnectCommunicationPort(
    //        PhGetString(clientName),
    //        0,
    //        NULL,
    //        0,
    //        NULL,
    //        &portHandle
    //        );
    //    PhDereferenceObject(clientName);
    //
    //    if (status == ERROR_SUCCESS)
    //    {
    //        *Handle = portHandle;
    //    }
    //}
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

    if (context->CurrentPath)
        PhDereferenceObject(context->CurrentPath);
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

INT_PTR CALLBACK WinObjDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    POBJ_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(OBJ_CONTEXT));
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
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 620, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 150, L"Type");
            PhLoadListViewColumnsFromSetting(SETTING_NAME_OBJMGR_COLUMNS, context->ListViewHandle);

            ExtendedListView_SetSortFast(context->ListViewHandle, TRUE);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 0, WinObjNameCompareFunction);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 1, WinObjTypeCompareFunction);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->TreeViewHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            if (PhGetIntegerPairSetting(SETTING_NAME_OBJMGR_WINDOW_POSITION).X != 0)
                PhLoadWindowPlacementFromSetting(SETTING_NAME_OBJMGR_WINDOW_POSITION, SETTING_NAME_OBJMGR_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, context->ParentWindowHandle);

            EnumDirectoryObjects(
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
            if (context->TreeImageList)
                PhImageListDestroy(context->TreeImageList);
            if (context->ListImageList)
                PhImageListDestroy(context->ListImageList);

            PhSaveWindowPlacementToSetting(SETTING_NAME_OBJMGR_WINDOW_POSITION, SETTING_NAME_OBJMGR_WINDOW_SIZE, hwndDlg);
            PhSaveListViewColumnsToSetting(SETTING_NAME_OBJMGR_COLUMNS, context->ListViewHandle);
            PhDeleteLayoutManager(&context->LayoutManager);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);

            PostQuitMessage(0);
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
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                DestroyWindow(hwndDlg);
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
                    context->SelectedTreeItem = TreeView_GetSelection(context->TreeViewHandle);

                    ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);
                    ListView_DeleteAllItems(context->ListViewHandle);

                    if (context->SelectedTreeItem == context->RootTreeObject)
                    {
                        EnumCurrentDirectoryObjects(context, EtObjectManagerRootDirectoryObject);
                    }
                    else
                    {
                        PPH_STRING selectedPath = EtGetSelectedTreeViewPath(context);

                        EnumCurrentDirectoryObjects(
                            context,
                            PhGetStringRef(selectedPath)
                            );

                        PhDereferenceObject(selectedPath);
                    }

                    ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);
                }
                break;
            case NM_SETCURSOR:
                {
                    if (header->hwndFrom == context->TreeViewHandle)
                    {
                        HCURSOR cursor = (HCURSOR)LoadImage(
                            NULL,
                            IDC_ARROW,
                            IMAGE_CURSOR,
                            0,
                            0,
                            LR_SHARED
                            );

                        if (cursor != GetCursor())
                        {
                            SetCursor(cursor);
                        }

                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                        return TRUE;
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
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"Prope&rties", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 2, L"&Security", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 3, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, 3, context->ListViewHandle);

                    PhSetFlagsEMenuItem(menu, 1, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                    //PhSetFlagsEMenuItem(menu, 1, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

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
                            case 2:
                                {
                                    POBJECT_ENTRY entry = listviewItems[0];
                                    PHANDLE_OPEN_CONTEXT objectContext;
                                    NTSTATUS status;
                                    HANDLE objectHandle;

                                    objectContext = PhAllocateZero(sizeof(HANDLE_OPEN_CONTEXT));
                                    objectContext->CurrentPath = EtGetSelectedTreeViewPath(context);
                                    objectContext->Object = entry;

                                    if (NT_SUCCESS(status = EtObjectManagerOpenHandle(&objectHandle, objectContext, READ_CONTROL)))
                                    {
                                        NtClose(objectHandle);

                                        PhEditSecurity(
                                            hwndDlg,
                                            PhGetString(entry->Name),
                                            PhGetString(entry->TypeName),
                                            EtObjectManagerHandleOpenCallback,
                                            EtObjectManagerHandleCloseCallback,
                                            objectContext
                                            );
                                    }
                                    else
                                    {
                                        PhShowStatus(hwndDlg, L"Unable to open object type.", status, 0);
                                        EtObjectManagerHandleCloseCallback(objectContext);
                                    }
                                }
                                break;
                            case 3:
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
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"&Security", NULL, NULL), ULONG_MAX);
                    //PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    //PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 2, L"Prope&rties", NULL, NULL), ULONG_MAX);
                    //PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    //PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 3, L"&Copy", NULL, NULL), ULONG_MAX);
                    //PhInsertCopyListViewEMenuItem(menu, 3, context->ListViewHandle);

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
                            case 1:
                                {
                                    PHANDLE_OPEN_CONTEXT objectContext;
                                    POBJECT_ENTRY entry;
                                    PPH_STRING name;
                                    NTSTATUS status;
                                    HANDLE objectHandle;

                                    name = EtGetSelectedTreeViewPath(context);

                                    entry = PhAllocateZero(sizeof(OBJECT_ENTRY));
                                    entry->Name = name;
                                    entry->TypeName = PhCreateString(L"DirectoryObject");

                                    objectContext = PhAllocateZero(sizeof(HANDLE_OPEN_CONTEXT));
                                    objectContext->CurrentPath = name;
                                    objectContext->Object = entry;

                                    if (NT_SUCCESS(status = EtObjectManagerOpenHandle(&objectHandle, objectContext, READ_CONTROL)))
                                    {
                                        PhEditSecurity(
                                            hwndDlg,
                                            PhGetString(objectContext->CurrentPath),
                                            L"Directory",
                                            EtObjectManagerHandleOpenCallback,
                                            EtObjectManagerHandleCloseCallback,
                                            objectContext
                                            );
                                    }
                                    else
                                    {
                                        PhShowStatus(hwndDlg, L"Unable to query directory object.", status, 0);
                                        EtObjectManagerHandleCloseCallback(objectContext);
                                    }

                                    //PhFree(entry);
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
        if (result == -1)
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
