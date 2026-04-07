/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2015-2024
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

static PPH_STRING NormalizeDiskDeviceInterfacePath(
    _In_ PCWSTR DeviceInterface
    )
{
    PPH_STRING normalizedPath;

    normalizedPath = PhCreateString(DeviceInterface);

    if (normalizedPath->Length >= 2 * sizeof(WCHAR) && normalizedPath->Buffer[1] == L'?')
        normalizedPath->Buffer[1] = OBJ_NAME_PATH_SEPARATOR;

    return normalizedPath;
}

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

    settingsString = PhGetStringSetting(SETTING_NAME_DISK_LIST);

    if (!PhIsNullOrEmptyString(settingsString))
    {
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

    PhClearReference(&settingsString);
}

VOID DiskDrivesSaveList(
    VOID
    )
{
    PH_STRING_BUILDER stringBuilder;
    PPH_STRING settingsString;

    PhInitializeStringBuilder(&stringBuilder, 260);

    PhAcquireQueuedLockShared(&DiskDevicesListLock);

    for (ULONG i = 0; i < DiskDevicesList->Count; i++)
    {
        PDV_DISK_ENTRY entry = PhReferenceObjectUnsafe(DiskDevicesList->Items[i]);

        if (!entry)
            continue;

        if (entry->UserReference)
        {
            PhAppendStringBuilder(&stringBuilder, &entry->Id.DevicePath->sr);
            PhAppendCharStringBuilder(&stringBuilder, L',');
        }

        PhDereferenceObjectDeferDelete(entry);
    }

    PhReleaseQueuedLockShared(&DiskDevicesListLock);

    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    settingsString = PhFinalStringBuilderString(&stringBuilder);
    PhSetStringSetting2(SETTING_NAME_DISK_LIST, &settingsString->sr);
    PhDereferenceObject(settingsString);
}

BOOLEAN FindDiskEntry(
    _In_ PDV_DISK_ID Id,
    _In_ BOOLEAN RemoveUserReference
    )
{
    BOOLEAN found = FALSE;

    PhAcquireQueuedLockShared(&DiskDevicesListLock);

    for (ULONG i = 0; i < DiskDevicesList->Count; i++)
    {
        PDV_DISK_ENTRY currentEntry = PhReferenceObjectUnsafe(DiskDevicesList->Items[i]);

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

    PhReleaseQueuedLockShared(&DiskDevicesListLock);

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

    for (ULONG i = 0; i < DiskDevicesList->Count; i++)
    {
        PDV_DISK_ENTRY entry = PhReferenceObjectUnsafe(DiskDevicesList->Items[i]);

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
    INT index = INT_ERROR;

    while ((index = PhFindListViewItemByFlags(
        Context->ListViewHandle,
        index,
        LVNI_ALL
        )) != INT_ERROR)
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
    ULONG deviceInterfacePropertyCount = 0;
    ULONG devicePropertyCount = 0;
    const DEVPROPERTY* deviceInterfaceProperties = NULL;
    const DEVPROPERTY* devicePropertiesList = NULL;
    const DEVPROPERTY* instanceIdProperty;
    PPH_STRING normalizedDeviceInterface;
    DEVINST deviceInstanceHandle;
    DEVPROPCOMPKEY requestedProperties[] =
    {
        { DEVPKEY_Device_InstanceId, DEVPROP_STORE_SYSTEM, NULL },
    };
    DEVPROPCOMPKEY deviceProperties[] =
    {
        { DEVPKEY_Device_FriendlyName, DEVPROP_STORE_SYSTEM, NULL },
        { DEVPKEY_NAME, DEVPROP_STORE_SYSTEM, NULL },
        { DEVPKEY_Device_DeviceDesc, DEVPROP_STORE_SYSTEM, NULL },
    };

    normalizedDeviceInterface = NormalizeDiskDeviceInterfacePath(DeviceInterface);

    if (HR_FAILED(PhDevGetObjectProperties(
        DevObjectTypeDeviceInterface,
        PhGetString(normalizedDeviceInterface),
        DevQueryFlagNone,
        RTL_NUMBER_OF(requestedProperties),
        requestedProperties,
        &deviceInterfacePropertyCount,
        &deviceInterfaceProperties
        )))
    {
        PhDereferenceObject(normalizedDeviceInterface);
        return FALSE;
    }

    PhDereferenceObject(normalizedDeviceInterface);

    instanceIdProperty = PhDevFindProperty(
        &DEVPKEY_Device_InstanceId,
        DEVPROP_STORE_SYSTEM,
        deviceInterfacePropertyCount,
        deviceInterfaceProperties
        );

    if (!instanceIdProperty || instanceIdProperty->Type != DEVPROP_TYPE_STRING || !instanceIdProperty->Buffer || instanceIdProperty->BufferSize < sizeof(UNICODE_NULL))
    {
        PhDevFreeObjectProperties(deviceInterfacePropertyCount, deviceInterfaceProperties);
        return FALSE;
    }

    if (CM_Locate_DevNode(
        &deviceInstanceHandle,
        instanceIdProperty->Buffer,
        CM_LOCATE_DEVNODE_PHANTOM
        ) != CR_SUCCESS)
    {
        PhDevFreeObjectProperties(deviceInterfacePropertyCount, deviceInterfaceProperties);
        return FALSE;
    }

    if (HR_FAILED(PhDevGetObjectProperties(
        DevObjectTypeDevice,
        instanceIdProperty->Buffer,
        DevQueryFlagNone,
        RTL_NUMBER_OF(deviceProperties),
        deviceProperties,
        &devicePropertyCount,
        &devicePropertiesList
        )) || devicePropertyCount == 0)
    {
        PhDevFreeObjectProperties(deviceInterfacePropertyCount, deviceInterfaceProperties);
        return FALSE;
    }

    *DeviceDescription = NULL;

    for (ULONG i = 0; i < RTL_NUMBER_OF(deviceProperties); i++)
    {
        const DEVPROPERTY* property;

        property = PhDevFindProperty(
            &deviceProperties[i].Key,
            deviceProperties[i].Store,
            devicePropertyCount,
            devicePropertiesList
            );

        if (property && property->Type == DEVPROP_TYPE_STRING && property->Buffer && property->BufferSize >= sizeof(UNICODE_NULL))
        {
            SIZE_T deviceDescriptionLength = property->BufferSize;

            if (((PWSTR)property->Buffer)[(property->BufferSize / sizeof(WCHAR)) - 1] == UNICODE_NULL)
                deviceDescriptionLength -= sizeof(UNICODE_NULL);

            *DeviceDescription = PhCreateStringEx((PWSTR)property->Buffer, deviceDescriptionLength);
            break;
        }
    }

    PhDevFreeObjectProperties(devicePropertyCount, devicePropertiesList);
    PhDevFreeObjectProperties(deviceInterfacePropertyCount, deviceInterfaceProperties);

    *DeviceInstanceHandle = deviceInstanceHandle;

    return *DeviceDescription != NULL;
}

VOID FindDiskDrives(
    _In_ PDV_DISK_OPTIONS_CONTEXT Context
    )
{
    ULONG deviceCount = 0;
    PPH_LIST deviceList;
    const DEV_OBJECT* deviceObjects = NULL;
    const DEVPROPCOMPKEY requestedProperties[] =
    {
        { DEVPKEY_Device_InstanceId, DEVPROP_STORE_SYSTEM, NULL },
    };
    const DEVPROP_FILTER_EXPRESSION deviceFilter[] =
    {
        { DEVPROP_OPERATOR_EQUALS, {{ DEVPKEY_DeviceInterface_ClassGuid, DEVPROP_STORE_SYSTEM, NULL }, DEVPROP_TYPE_GUID, sizeof(GUID), (PVOID)&GUID_DEVINTERFACE_DISK } }
    };

    if (HR_FAILED(PhDevGetObjects(
        DevObjectTypeDeviceInterface,
        DevQueryFlagNone,
        RTL_NUMBER_OF(requestedProperties),
        requestedProperties,
        RTL_NUMBER_OF(deviceFilter),
        deviceFilter,
        &deviceCount,
        &deviceObjects
        )))
    {
        return;
    }

    deviceList = PH_AUTO(PhCreateList(1));

    for (ULONG i = 0; i < deviceCount; i++)
    {
        DEVINST deviceInstanceHandle;
        PPH_STRING deviceDescription;
        PWSTR deviceInterface;
        HANDLE deviceHandle;
        PDISK_ENUM_ENTRY diskEntry;

        deviceInterface = (PWSTR)deviceObjects[i].pszObjectId;

        if (!QueryDiskDeviceInterfaceDescription(deviceInterface, &deviceInstanceHandle, &deviceDescription))
            continue;

        if (Context->UseAlternateMethod)
        {
            if (
                PhEndsWithStringRef2(&deviceDescription->sr, L"Xvd", TRUE) || // Windows Store Games DRM
                PhEndsWithStringRef2(&deviceDescription->sr, L"Microsoft Virtual Disk", TRUE)
                )
            {
                PhDereferenceObject(deviceDescription);
                continue;
            }
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

    PhDevFreeObjects(deviceCount, deviceObjects);

    // Sort the entries
    qsort(deviceList->Items, deviceList->Count, sizeof(PVOID), DiskEntryCompareFunction);

    PhAcquireQueuedLockShared(&DiskDevicesListLock);
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
    PhReleaseQueuedLockShared(&DiskDevicesListLock);

    // HACK: Show all unknown devices.
    PhAcquireQueuedLockShared(&DiskDevicesListLock);
    for (ULONG i = 0; i < DiskDevicesList->Count; i++)
    {
        INT index = INT_ERROR;
        BOOLEAN found = FALSE;
        PDV_DISK_ENTRY entry = PhReferenceObjectUnsafe(DiskDevicesList->Items[i]);

        if (!entry)
            continue;

        while ((index = PhFindListViewItemByFlags(
            Context->ListViewHandle,
            index,
            LVNI_ALL
            )) != INT_ERROR)
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
    PhReleaseQueuedLockShared(&DiskDevicesListLock);
}

PPH_STRING FindDiskDeviceInstance(
    _In_ PPH_STRING DevicePath
    )
{
    PPH_STRING deviceInstanceString = NULL;
    ULONG deviceCount = 0;
    const DEV_OBJECT* deviceObjects = NULL;
    const DEVPROPCOMPKEY requestedProperties[] =
    {
        { DEVPKEY_Device_InstanceId, DEVPROP_STORE_SYSTEM, NULL },
    };
    const DEVPROP_FILTER_EXPRESSION deviceFilter[] =
    {
        { DEVPROP_OPERATOR_EQUALS, {{ DEVPKEY_DeviceInterface_ClassGuid, DEVPROP_STORE_SYSTEM, NULL }, DEVPROP_TYPE_GUID, sizeof(GUID), (PVOID)&GUID_DEVINTERFACE_DISK } }
    };

    if (HR_FAILED(PhDevGetObjects(
        DevObjectTypeDeviceInterface,
        DevQueryFlagNone,
        RTL_NUMBER_OF(requestedProperties),
        requestedProperties,
        RTL_NUMBER_OF(deviceFilter),
        deviceFilter,
        &deviceCount,
        &deviceObjects
        )))
    {
        return NULL;
    }

    for (ULONG i = 0; i < deviceCount; i++)
    {
        const DEV_OBJECT* device;
        PWSTR deviceInterface;

        device = &deviceObjects[i];
        deviceInterface = (PWSTR)device->pszObjectId;

        if (deviceInterface[1] == OBJ_NAME_PATH_SEPARATOR)
            deviceInterface[1] = L'?';

        if (PhEqualStringZ(deviceInterface, DevicePath->Buffer, TRUE))
        {
            PH_STRINGREF string;

            string.Buffer = device->pProperties[0].Buffer;
            string.Length = device->pProperties[0].BufferSize ? device->pProperties[0].BufferSize - sizeof(UNICODE_NULL) : 0;
            deviceInstanceString = PhCreateString2(&string);
            break;
        }
    }

    PhDevFreeObjects(deviceCount, deviceObjects);

    return deviceInstanceString;
}

VOID LoadDiskDriveImages(
    _In_ PDV_DISK_OPTIONS_CONTEXT Context
    )
{
    HICON largeIcon;
    ULONG deviceInstallerClassPropertyCount = 0;
    const DEVPROPERTY* deviceInstallerClassProperties = NULL;
    const DEVPROPERTY* deviceIconPathProperty;
    PPH_STRING classGuidString;
    PPH_STRING deviceIconPath;
    PH_STRINGREF dllPartSr;
    PH_STRINGREF indexPartSr;
    ULONG64 index;
    LONG dpiValue;
    DEVPROPCOMPKEY requestedProperties[] =
    {
        { DEVPKEY_DeviceClass_IconPath, DEVPROP_STORE_SYSTEM, NULL },
    };

    dpiValue = PhGetWindowDpi(Context->ListViewHandle);
    classGuidString = PhFormatGuid(&GUID_DEVCLASS_DISKDRIVE);

    if (!classGuidString)
        return;

    if (HR_FAILED(PhDevGetObjectProperties(
        DevObjectTypeDeviceInstallerClass,
        PhGetString(classGuidString),
        DevQueryFlagNone,
        RTL_NUMBER_OF(requestedProperties),
        requestedProperties,
        &deviceInstallerClassPropertyCount,
        &deviceInstallerClassProperties
        )))
    {
        PhDereferenceObject(classGuidString);
        return;
    }

    PhDereferenceObject(classGuidString);

    deviceIconPathProperty = PhDevFindProperty(
        &DEVPKEY_DeviceClass_IconPath,
        DEVPROP_STORE_SYSTEM,
        deviceInstallerClassPropertyCount,
        deviceInstallerClassProperties
        );

    if (
        !deviceIconPathProperty ||
        (deviceIconPathProperty->Type != DEVPROP_TYPE_STRING && deviceIconPathProperty->Type != DEVPROP_TYPE_STRING_LIST) ||
        !deviceIconPathProperty->Buffer || deviceIconPathProperty->BufferSize < sizeof(UNICODE_NULL)
        )
    {
        PhDevFreeObjectProperties(deviceInstallerClassPropertyCount, deviceInstallerClassProperties);
        return;
    }

    deviceIconPath = PhCreateStringEx((PWSTR)deviceIconPathProperty->Buffer, deviceIconPathProperty->BufferSize);
    PhTrimToNullTerminatorString(deviceIconPath);
    PhDevFreeObjectProperties(deviceInstallerClassPropertyCount, deviceInstallerClassProperties);

    // %SystemRoot%\System32\setupapi.dll,-53
    PhSplitStringRefAtChar(&deviceIconPath->sr, L',', &dllPartSr, &indexPartSr);
    PhStringToInteger64(&indexPartSr, 10, &index);
    PhMoveReference(&deviceIconPath, PhExpandEnvironmentStrings(&dllPartSr));

    if (PhExtractIconEx(
        &deviceIconPath->sr,
        FALSE,
        (INT)index,
        PhGetSystemMetrics(SM_CXICON, dpiValue),
        PhGetSystemMetrics(SM_CYICON, dpiValue),
        0,
        0,
        &largeIcon,
        NULL
        ))
    {
        HIMAGELIST imageList = PhImageListCreate(
            PhGetDpi(24, dpiValue), // PhGetSystemMetrics(SM_CXSMICON, dpiValue)
            PhGetDpi(24, dpiValue), // PhGetSystemMetrics(SM_CYSMICON, dpiValue)
            ILC_MASK | ILC_COLOR32,
            1,
            1
            );

        if (imageList)
        {
            PhImageListAddIcon(imageList, largeIcon);
            ListView_SetImageList(Context->ListViewHandle, imageList, LVSIL_SMALL);
            DestroyIcon(largeIcon);
        }
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
        context = PhAllocateZero(sizeof(DV_DISK_OPTIONS_CONTEXT));
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
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_SHOW_HIDDEN_DEVICES), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);

            ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);
            FindDiskDrives(context);
            ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);

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
        }
        break;
    case WM_NCDESTROY:
        {
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
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_SHOW_HIDDEN_DEVICES:
                {
                    context->UseAlternateMethod = !context->UseAlternateMethod;

                    ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);
                    FreeListViewDiskDriveEntries(context);
                    ListView_DeleteAllItems(context->ListViewHandle);
                    FindDiskDrives(context);
                    ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);

                    if (ListView_GetItemCount(context->ListViewHandle) == 0)
                        PhSetWindowStyle(context->ListViewHandle, WS_BORDER, WS_BORDER);
                    else
                        PhSetWindowStyle(context->ListViewHandle, WS_BORDER, 0);

                    //ExtendedListView_SetColumnWidth(context->ListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->code == LVN_ITEMCHANGED)
            {
                LPNM_LISTVIEW listView = (LPNM_LISTVIEW)lParam;

                if (!PhTryAcquireReleaseQueuedLockExclusive(&DiskDevicesListLock))
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

                        ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);
                        FreeListViewDiskDriveEntries(context);
                        ListView_DeleteAllItems(context->ListViewHandle);
                        FindDiskDrives(context);
                        ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);
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
