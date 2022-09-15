/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2015-2022
 *
 */

#include "devices.h"
#include <devguid.h>

typedef struct _DISK_ENUM_ENTRY
{
    ULONG DeviceIndex;
    BOOLEAN DevicePresent;
    PPH_STRING DevicePath;
    PPH_STRING DeviceName;
    PPH_STRING DeviceMountPoints;
} DISK_ENUM_ENTRY, *PDISK_ENUM_ENTRY;

static int __cdecl DiskEntryCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PDISK_ENUM_ENTRY entry1 = *(PDISK_ENUM_ENTRY *)elem1;
    PDISK_ENUM_ENTRY entry2 = *(PDISK_ENUM_ENTRY *)elem2;

    return uint64cmp(entry1->DeviceIndex, entry2->DeviceIndex);
}

VOID DiskDrivesLoadList(
    VOID
    )
{
    PPH_STRING settingsString;
    PH_STRINGREF remaining;

    settingsString = PhaGetStringSetting(SETTING_NAME_DISK_LIST);

    if (PhIsNullOrEmptyString(settingsString))
        return;

    remaining = PhGetStringRef(settingsString);

    while (remaining.Length != 0)
    {
        PH_STRINGREF part;
        DV_DISK_ID id;
        PDV_DISK_ENTRY entry;

        if (remaining.Length == 0)
            break;

        PhSplitStringRefAtChar(&remaining, L',', &part, &remaining);

        // Convert settings path for compatibility. (dmex)
        if (part.Length > sizeof(UNICODE_NULL) && part.Buffer[1] == OBJ_NAME_PATH_SEPARATOR)
            part.Buffer[1] = L'?';

        InitializeDiskId(&id, PhCreateString2(&part));
        entry = CreateDiskEntry(&id);
        DeleteDiskId(&id);

        entry->UserReference = TRUE;
    }
}

VOID DiskDrivesSaveList(
    VOID
    )
{
    PH_STRING_BUILDER stringBuilder;
    PPH_STRING settingsString;

    PhInitializeStringBuilder(&stringBuilder, 260);

    PhAcquireQueuedLockShared(&DiskDrivesListLock);

    for (ULONG i = 0; i < DiskDrivesList->Count; i++)
    {
        PDV_DISK_ENTRY entry = PhReferenceObjectSafe(DiskDrivesList->Items[i]);

        if (!entry)
            continue;

        if (entry->UserReference)
        {
            PhAppendStringBuilder(&stringBuilder, &entry->Id.DevicePath->sr);
            PhAppendCharStringBuilder(&stringBuilder, L',');
        }

        PhDereferenceObjectDeferDelete(entry);
    }

    PhReleaseQueuedLockShared(&DiskDrivesListLock);

    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    settingsString = PH_AUTO(PhFinalStringBuilderString(&stringBuilder));
    PhSetStringSetting2(SETTING_NAME_DISK_LIST, &settingsString->sr);
}

BOOLEAN FindDiskEntry(
    _In_ PDV_DISK_ID Id,
    _In_ BOOLEAN RemoveUserReference
    )
{
    BOOLEAN found = FALSE;

    PhAcquireQueuedLockShared(&DiskDrivesListLock);

    for (ULONG i = 0; i < DiskDrivesList->Count; i++)
    {
        PDV_DISK_ENTRY currentEntry = PhReferenceObjectSafe(DiskDrivesList->Items[i]);

        if (!currentEntry)
            continue;

        found = EquivalentDiskId(&currentEntry->Id, Id);

        if (found)
        {
            if (RemoveUserReference)
            {
                if (currentEntry->UserReference)
                {
                    PhDereferenceObjectDeferDelete(currentEntry);
                    currentEntry->UserReference = FALSE;
                }
            }

            PhDereferenceObjectDeferDelete(currentEntry);

            break;
        }
        else
        {
            PhDereferenceObjectDeferDelete(currentEntry);
        }
    }

    PhReleaseQueuedLockShared(&DiskDrivesListLock);

    return found;
}

VOID AddDiskDriveToListView(
    _In_ PDV_DISK_OPTIONS_CONTEXT Context,
    _In_ BOOLEAN DiskPresent,
    _In_ PPH_STRING DiskPath,
    _In_ PPH_STRING DiskName
    )
{
    DV_DISK_ID adapterId;
    INT lvItemIndex;
    BOOLEAN found = FALSE;
    PDV_DISK_ID newId = NULL;

    InitializeDiskId(&adapterId, DiskPath);

    for (ULONG i = 0; i < DiskDrivesList->Count; i++)
    {
        PDV_DISK_ENTRY entry = PhReferenceObjectSafe(DiskDrivesList->Items[i]);

        if (!entry)
            continue;

        if (EquivalentDiskId(&entry->Id, &adapterId))
        {
            newId = PhAllocate(sizeof(DV_DISK_ID));
            CopyDiskId(newId, &entry->Id);

            if (entry->UserReference)
                found = TRUE;
        }

        PhDereferenceObjectDeferDelete(entry);

        if (newId)
            break;
    }

    if (!newId)
    {
        newId = PhAllocate(sizeof(DV_DISK_ID));
        CopyDiskId(newId, &adapterId);
        PhMoveReference(&newId->DevicePath, DiskPath);
    }

    lvItemIndex = PhAddListViewGroupItem(
        Context->ListViewHandle,
        DiskPresent ? 0 : 1,
        MAXINT,
        DiskName->Buffer,
        newId
        );

    if (found)
        ListView_SetCheckState(Context->ListViewHandle, lvItemIndex, TRUE);

    DeleteDiskId(&adapterId);
}

VOID FreeListViewDiskDriveEntries(
    _In_ PDV_DISK_OPTIONS_CONTEXT Context
    )
{
    ULONG index = ULONG_MAX;

    while ((index = PhFindListViewItemByFlags(
        Context->ListViewHandle,
        index,
        LVNI_ALL
        )) != ULONG_MAX)
    {
        PDV_DISK_ID param;

        if (PhGetListViewItemParam(Context->ListViewHandle, index, &param))
        {
            DeleteDiskId(param);
            PhFree(param);
        }
    }
}

_Success_(return)
BOOLEAN QueryDiskDeviceInterfaceDescription(
    _In_ PWSTR DeviceInterface,
    _Out_ DEVINST *DeviceInstanceHandle,
    _Out_ PPH_STRING *DeviceDescription
    )
{
    CONFIGRET result;
    ULONG bufferSize;
    PPH_STRING deviceDescription;
    DEVPROPTYPE devicePropertyType;
    DEVINST deviceInstanceHandle;
    ULONG deviceInstanceIdLength = MAX_DEVICE_ID_LEN;
    WCHAR deviceInstanceId[MAX_DEVICE_ID_LEN + 1] = L"";

    if (CM_Get_Device_Interface_Property(
        DeviceInterface,
        &DEVPKEY_Device_InstanceId,
        &devicePropertyType,
        (PBYTE)deviceInstanceId,
        &deviceInstanceIdLength,
        0
        ) != CR_SUCCESS)
    {
        return FALSE;
    }

    if (CM_Locate_DevNode(
        &deviceInstanceHandle,
        deviceInstanceId,
        CM_LOCATE_DEVNODE_PHANTOM
        ) != CR_SUCCESS)
    {
        return FALSE;
    }

    bufferSize = 0x40;
    deviceDescription = PhCreateStringEx(NULL, bufferSize);

    if ((result = CM_Get_DevNode_Property(
        deviceInstanceHandle,
        &DEVPKEY_Device_FriendlyName,
        &devicePropertyType,
        (PBYTE)deviceDescription->Buffer,
        &bufferSize,
        0
        )) != CR_SUCCESS)
    {
        PhDereferenceObject(deviceDescription);
        deviceDescription = PhCreateStringEx(NULL, bufferSize);

        result = CM_Get_DevNode_Property(
            deviceInstanceHandle,
            &DEVPKEY_Device_FriendlyName,
            &devicePropertyType,
            (PBYTE)deviceDescription->Buffer,
            &bufferSize,
            0
            );
    }

    if (result != CR_SUCCESS)
    {
        PhDereferenceObject(deviceDescription);
        return FALSE;
    }

    PhTrimToNullTerminatorString(deviceDescription);

    *DeviceInstanceHandle = deviceInstanceHandle;
    *DeviceDescription = deviceDescription;

    return TRUE;
}

VOID FindDiskDrives(
    _In_ PDV_DISK_OPTIONS_CONTEXT Context
    )
{
    PPH_LIST deviceList;
    PWSTR deviceInterfaceList;
    ULONG deviceInterfaceListLength = 0;
    PWSTR deviceInterface;

    if (CM_Get_Device_Interface_List_Size(
        &deviceInterfaceListLength,
        (PGUID)&GUID_DEVINTERFACE_DISK,
        NULL,
        CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES
        ) != CR_SUCCESS)
    {
        return;
    }

    deviceInterfaceList = PhAllocate(deviceInterfaceListLength * sizeof(WCHAR));
    memset(deviceInterfaceList, 0, deviceInterfaceListLength * sizeof(WCHAR));

    if (CM_Get_Device_Interface_List(
        (PGUID)&GUID_DEVINTERFACE_DISK,
        NULL,
        deviceInterfaceList,
        deviceInterfaceListLength,
        CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES
        ) != CR_SUCCESS)
    {
        PhFree(deviceInterfaceList);
        return;
    }

    deviceList = PH_AUTO(PhCreateList(1));

    for (deviceInterface = deviceInterfaceList; *deviceInterface; deviceInterface += PhCountStringZ(deviceInterface) + 1)
    {
        DEVINST deviceInstanceHandle;
        PPH_STRING deviceDescription;
        HANDLE deviceHandle;
        PDISK_ENUM_ENTRY diskEntry;

        if (!QueryDiskDeviceInterfaceDescription(deviceInterface, &deviceInstanceHandle, &deviceDescription))
            continue;

        // Ignore Windows Store DRM installer spam (dmex)
        if (
            PhEndsWithStringRef2(&deviceDescription->sr, L"Xvd", TRUE) ||
            PhEndsWithStringRef2(&deviceDescription->sr, L"Microsoft Virtual Disk", TRUE)
            )
        {
            PhDereferenceObject(deviceDescription);
            continue;
        }

        // Convert path now to avoid conversion during every interval update. (dmex)
        if (deviceInterface[1] == OBJ_NAME_PATH_SEPARATOR)
            deviceInterface[1] = L'?';

        diskEntry = PhAllocateZero(sizeof(DISK_ENUM_ENTRY));
        diskEntry->DeviceIndex = ULONG_MAX;
        diskEntry->DeviceName = PhCreateString2(&deviceDescription->sr);
        diskEntry->DevicePath = PhCreateString(deviceInterface);

        //if (
        //    PhFindStringInString(diskEntry->DevicePath, 0, L"Ven_MSFT&Prod_XVDD") != -1 ||
        //    PhFindStringInString(diskEntry->DevicePath, 0, L"Ven_Msft&Prod_Virtual_Disk") != -1
        //    )
        //{
        //    PhDereferenceObject(diskEntry->DevicePath);
        //    PhDereferenceObject(diskEntry->DeviceName);
        //    PhFree(diskEntry);
        //    continue;
        //}

        if (NT_SUCCESS(PhCreateFile(
            &deviceHandle,
            &diskEntry->DevicePath->sr,
            FILE_READ_ATTRIBUTES | SYNCHRONIZE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            ULONG diskIndex = ULONG_MAX;

            if (NT_SUCCESS(DiskDriveQueryDeviceTypeAndNumber(
                deviceHandle,
                &diskIndex,
                NULL
                )))
            {
                PPH_STRING diskMountPoints;

                diskMountPoints = PH_AUTO_T(PH_STRING, DiskDriveQueryDosMountPoints(diskIndex));

                diskEntry->DeviceIndex = diskIndex;
                diskEntry->DevicePresent = TRUE;

                if (!PhIsNullOrEmptyString(diskMountPoints))
                {
                    PH_FORMAT format[7];

                    // Disk %lu (%s) [%s]
                    PhInitFormatS(&format[0], L"Disk ");
                    PhInitFormatU(&format[1], diskIndex);
                    PhInitFormatS(&format[2], L" (");
                    PhInitFormatSR(&format[3], diskMountPoints->sr);
                    PhInitFormatS(&format[4], L") [");
                    PhInitFormatSR(&format[5], deviceDescription->sr);
                    PhInitFormatC(&format[6], L']');

                    diskEntry->DeviceMountPoints = PhFormat(format, RTL_NUMBER_OF(format), 0);
                }
                else
                {
                    PH_FORMAT format[5];

                    // Disk %lu (%s)
                    PhInitFormatS(&format[0], L"Disk ");
                    PhInitFormatU(&format[1], diskIndex);
                    PhInitFormatS(&format[2], L" [");
                    PhInitFormatSR(&format[3], deviceDescription->sr);
                    PhInitFormatC(&format[4], L']');

                    diskEntry->DeviceMountPoints = PhFormat(format, RTL_NUMBER_OF(format), 0);
                }
            }

            NtClose(deviceHandle);
        }

        PhAddItemList(deviceList, diskEntry);

        PhDereferenceObject(deviceDescription);
    }

    PhFree(deviceInterfaceList);

    // Sort the entries
    qsort(deviceList->Items, deviceList->Count, sizeof(PVOID), DiskEntryCompareFunction);

    PhAcquireQueuedLockShared(&DiskDrivesListLock);
    for (ULONG i = 0; i < deviceList->Count; i++)
    {
        PDISK_ENUM_ENTRY entry = deviceList->Items[i];

        AddDiskDriveToListView(
            Context,
            entry->DevicePresent,
            entry->DevicePath,
            entry->DeviceMountPoints ? entry->DeviceMountPoints : entry->DeviceName
            );

        if (entry->DeviceMountPoints)
            PhDereferenceObject(entry->DeviceMountPoints);
        if (entry->DeviceName)
            PhDereferenceObject(entry->DeviceName);
        // Note: DevicePath is disposed by WM_DESTROY.

        PhFree(entry);
    }
    PhReleaseQueuedLockShared(&DiskDrivesListLock);

    // HACK: Show all unknown devices.
    PhAcquireQueuedLockShared(&DiskDrivesListLock);
    for (ULONG i = 0; i < DiskDrivesList->Count; i++)
    {
        ULONG index = ULONG_MAX;
        BOOLEAN found = FALSE;
        PDV_DISK_ENTRY entry = PhReferenceObjectSafe(DiskDrivesList->Items[i]);

        if (!entry)
            continue;

        while ((index = PhFindListViewItemByFlags(
            Context->ListViewHandle,
            index,
            LVNI_ALL
            )) != ULONG_MAX)
        {
            PDV_DISK_ID param;

            if (PhGetListViewItemParam(Context->ListViewHandle, index, &param))
            {
                if (EquivalentDiskId(param, &entry->Id))
                {
                    found = TRUE;
                }
            }
        }

        if (!found)
        {
            PPH_STRING description;

            if (description = PhCreateString(L"Unknown disk"))
            {
                AddDiskDriveToListView(
                    Context,
                    FALSE,
                    entry->Id.DevicePath,
                    description
                    );

                PhDereferenceObject(description);
            }
        }

        PhDereferenceObjectDeferDelete(entry);
    }
    PhReleaseQueuedLockShared(&DiskDrivesListLock);
}

PPH_STRING FindDiskDeviceInstance(
    _In_ PPH_STRING DevicePath
    )
{
    PPH_STRING deviceInstanceString = NULL;
    PWSTR deviceInterfaceList;
    ULONG deviceInterfaceListLength = 0;
    PWSTR deviceInterface;

    if (CM_Get_Device_Interface_List_Size(
        &deviceInterfaceListLength,
        (PGUID)&GUID_DEVINTERFACE_DISK,
        NULL,
        CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES
        ) != CR_SUCCESS)
    {
        return NULL;
    }

    deviceInterfaceList = PhAllocate(deviceInterfaceListLength * sizeof(WCHAR));
    memset(deviceInterfaceList, 0, deviceInterfaceListLength * sizeof(WCHAR));

    if (CM_Get_Device_Interface_List(
        (PGUID)&GUID_DEVINTERFACE_DISK,
        NULL,
        deviceInterfaceList,
        deviceInterfaceListLength,
        CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES
        ) != CR_SUCCESS)
    {
        PhFree(deviceInterfaceList);
        return NULL;
    }

    for (deviceInterface = deviceInterfaceList; *deviceInterface; deviceInterface += PhCountStringZ(deviceInterface) + 1)
    {
        DEVPROPTYPE devicePropertyType;
        ULONG deviceInstanceIdLength = MAX_DEVICE_ID_LEN;
        WCHAR deviceInstanceId[MAX_DEVICE_ID_LEN + 1] = L"";

        if (CM_Get_Device_Interface_Property(
            deviceInterface,
            &DEVPKEY_Device_InstanceId,
            &devicePropertyType,
            (PBYTE)deviceInstanceId,
            &deviceInstanceIdLength,
            0
            ) != CR_SUCCESS)
        {
            continue;
        }

        if (deviceInterface[1] == OBJ_NAME_PATH_SEPARATOR)
            deviceInterface[1] = L'?';

        if (PhEqualStringZ(deviceInterface, DevicePath->Buffer, TRUE))
        {
            deviceInstanceString = PhCreateString(deviceInstanceId);
            break;
        }
    }

    PhFree(deviceInterfaceList);

    return deviceInstanceString;
}

VOID LoadDiskDriveImages(
    _In_ PDV_DISK_OPTIONS_CONTEXT Context
    )
{
    HICON smallIcon;
    CONFIGRET result;
    ULONG bufferSize;
    PPH_STRING deviceIconPath;
    PH_STRINGREF dllPartSr;
    PH_STRINGREF indexPartSr;
    ULONG64 index;
    DEVPROPTYPE devicePropertyType;
    ULONG deviceInstanceIdLength = MAX_DEVICE_ID_LEN;
    WCHAR deviceInstanceId[MAX_DEVICE_ID_LEN + 1] = L"";
    LONG dpiValue;

    bufferSize = 0x40;
    deviceIconPath = PhCreateStringEx(NULL, bufferSize);

    dpiValue = PhGetWindowDpi(Context->ListViewHandle);

    if ((result = CM_Get_Class_Property(
        &GUID_DEVCLASS_DISKDRIVE,
        &DEVPKEY_DeviceClass_IconPath,
        &devicePropertyType,
        (PBYTE)deviceIconPath->Buffer,
        &bufferSize,
        0
        )) != CR_SUCCESS)
    {
        PhDereferenceObject(deviceIconPath);
        deviceIconPath = PhCreateStringEx(NULL, bufferSize);

        result = CM_Get_Class_Property(
            &GUID_DEVCLASS_DISKDRIVE,
            &DEVPKEY_DeviceClass_IconPath,
            &devicePropertyType,
            (PBYTE)deviceIconPath->Buffer,
            &bufferSize,
            0
            );
    }

    PhTrimToNullTerminatorString(deviceIconPath);

    // %SystemRoot%\System32\setupapi.dll,-53
    PhSplitStringRefAtChar(&deviceIconPath->sr, L',', &dllPartSr, &indexPartSr);
    PhStringToInteger64(&indexPartSr, 10, &index);
    PhMoveReference(&deviceIconPath, PhExpandEnvironmentStrings(&dllPartSr));

    if (PhExtractIconEx(deviceIconPath, FALSE, (INT)index, &smallIcon, NULL, dpiValue))
    {
        HIMAGELIST imageList = PhImageListCreate(
            PhGetSystemMetrics(SM_CXSMICON, dpiValue),
            PhGetSystemMetrics(SM_CYSMICON, dpiValue),
            ILC_MASK | ILC_COLOR32,
            1,
            1
            );

        PhImageListAddIcon(imageList, smallIcon);
        DestroyIcon(smallIcon);

        ListView_SetImageList(Context->ListViewHandle, imageList, LVSIL_SMALL);
    }

    PhDereferenceObject(deviceIconPath);
}

INT_PTR CALLBACK DiskDriveOptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_DISK_OPTIONS_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocate(sizeof(DV_DISK_OPTIONS_CONTEXT));
        memset(context, 0, sizeof(DV_DISK_OPTIONS_CONTEXT));

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_DISKDRIVE_LISTVIEW);
            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            ListView_SetExtendedListViewStyleEx(context->ListViewHandle, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 350, L"Disk Drives");
            PhSetExtendedListView(context->ListViewHandle);
            LoadDiskDriveImages(context);

            ListView_EnableGroupView(context->ListViewHandle, TRUE);
            PhAddListViewGroup(context->ListViewHandle, 0, L"Connected");
            PhAddListViewGroup(context->ListViewHandle, 1, L"Disconnected");

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            FindDiskDrives(context);

            if (ListView_GetItemCount(context->ListViewHandle) == 0)
                PhSetWindowStyle(context->ListViewHandle, WS_BORDER, WS_BORDER);

            context->OptionsChanged = FALSE;
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);

            if (context->OptionsChanged)
                DiskDrivesSaveList();

            FreeListViewDiskDriveEntries(context);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);

            ExtendedListView_SetColumnWidth(context->ListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->code == LVN_ITEMCHANGED)
            {
                LPNM_LISTVIEW listView = (LPNM_LISTVIEW)lParam;

                if (!PhTryAcquireReleaseQueuedLockExclusive(&DiskDrivesListLock))
                    break;

                if (listView->uChanged & LVIF_STATE)
                {
                    switch (listView->uNewState & LVIS_STATEIMAGEMASK)
                    {
                    case INDEXTOSTATEIMAGEMASK(2): // checked
                        {
                            PDV_DISK_ID param = (PDV_DISK_ID)listView->lParam;

                            if (!FindDiskEntry(param, FALSE))
                            {
                                PDV_DISK_ENTRY entry;

                                entry = CreateDiskEntry(param);
                                entry->UserReference = TRUE;
                            }

                            context->OptionsChanged = TRUE;
                        }
                        break;
                    case INDEXTOSTATEIMAGEMASK(1): // unchecked
                        {
                            PDV_DISK_ID param = (PDV_DISK_ID)listView->lParam;

                            FindDiskEntry(param, TRUE);

                            context->OptionsChanged = TRUE;
                        }
                        break;
                    }
                }
            }
            else if (header->code == NM_RCLICK)
            {
                PDV_DISK_ID param;
                PPH_STRING deviceInstance;

                if (param = PhGetSelectedListViewItemParam(context->ListViewHandle))
                {
                    if (deviceInstance = FindDiskDeviceInstance(param->DevicePath))
                    {
                        ShowDeviceMenu(hwndDlg, deviceInstance);
                        PhDereferenceObject(deviceInstance);

                        FreeListViewDiskDriveEntries(context);
                        ListView_DeleteAllItems(context->ListViewHandle);
                        FindDiskDrives(context);
                    }
                }
            }
            else if (header->code == NM_DBLCLK)
            {
                PDV_DISK_ID param;
                PPH_STRING deviceInstance;

                if (param = PhGetSelectedListViewItemParam(context->ListViewHandle))
                {
                    if (deviceInstance = FindDiskDeviceInstance(param->DevicePath))
                    {
                        HardwareDeviceShowProperties(hwndDlg, deviceInstance);
                        PhDereferenceObject(deviceInstance);
                    }
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
