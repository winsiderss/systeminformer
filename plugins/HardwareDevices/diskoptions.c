/*
 * Process Hacker Plugins -
 *   Hardware Devices Plugin
 *
 * Copyright (C) 2015-2016 dmex
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

#include "devices.h"
#include <Setupapi.h>

#define ITEM_CHECKED (INDEXTOSTATEIMAGEMASK(2))
#define ITEM_UNCHECKED (INDEXTOSTATEIMAGEMASK(1))

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
    remaining = settingsString->sr;

    while (remaining.Length != 0)
    {
        PH_STRINGREF part;
        DV_DISK_ID id;
        PDV_DISK_ENTRY entry;

        if (remaining.Length == 0)
            break;

        PhSplitStringRefAtChar(&remaining, ',', &part, &remaining);

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
            PhAppendFormatStringBuilder(
                &stringBuilder,
                L"%s,",
                entry->Id.DevicePath->Buffer // This value is SAFE and does not change.
                );
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

    lvItemIndex = AddListViewItemGroupId(
        Context->ListViewHandle,
        DiskPresent ? 0 : 1,
        MAXINT,
        DiskName->Buffer,
        newId
        );

    if (found)
        ListView_SetItemState(Context->ListViewHandle, lvItemIndex, ITEM_CHECKED, LVIS_STATEIMAGEMASK);

    DeleteDiskId(&adapterId);
}

VOID FreeListViewDiskDriveEntries(
    _In_ PDV_DISK_OPTIONS_CONTEXT Context
    )
{
    ULONG index = -1;

    while ((index = PhFindListViewItemByFlags(
        Context->ListViewHandle,
        index,
        LVNI_ALL
        )) != -1)
    {
        PDV_DISK_ID param;

        if (PhGetListViewItemParam(Context->ListViewHandle, index, &param))
        {
            DeleteDiskId(param);
            PhFree(param);
        }
    }
}

VOID FindDiskDrives(
    _In_ PDV_DISK_OPTIONS_CONTEXT Context
    )
{
    PPH_LIST deviceList;
    HDEVINFO deviceInfoHandle;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData = { sizeof(SP_DEVICE_INTERFACE_DATA) };
    SP_DEVINFO_DATA deviceInfoData = { sizeof(SP_DEVINFO_DATA) };
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetail;
    ULONG deviceInfoLength = 0;

    if ((deviceInfoHandle = SetupDiGetClassDevs(
        &GUID_DEVINTERFACE_DISK,
        NULL,
        NULL,
        DIGCF_DEVICEINTERFACE
        )) == INVALID_HANDLE_VALUE)
    {
        return;
    }

    deviceList = PH_AUTO(PhCreateList(1));

    for (ULONG i = 0; SetupDiEnumDeviceInterfaces(deviceInfoHandle, NULL, &GUID_DEVINTERFACE_DISK, i, &deviceInterfaceData); i++)
    {
        if (SetupDiGetDeviceInterfaceDetail(
            deviceInfoHandle,
            &deviceInterfaceData,
            0,
            0,
            &deviceInfoLength,
            &deviceInfoData
            ) || GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            continue;
        }

        deviceInterfaceDetail = PhAllocate(deviceInfoLength);
        deviceInterfaceDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (SetupDiGetDeviceInterfaceDetail(
            deviceInfoHandle,
            &deviceInterfaceData,
            deviceInterfaceDetail,
            deviceInfoLength,
            &deviceInfoLength,
            &deviceInfoData
            ))
        {
            HANDLE deviceHandle;
            PDISK_ENUM_ENTRY diskEntry;
            WCHAR diskFriendlyName[MAX_PATH] = L"";

            // This crashes on XP with error 0xC06D007F
            //SetupDiGetDeviceProperty(
            //    deviceInfoHandle,
            //    &deviceInfoData,
            //    &DEVPKEY_Device_FriendlyName,
            //    &devicePropertyType,
            //    (PBYTE)diskFriendlyName,
            //    ARRAYSIZE(diskFriendlyName),
            //    NULL,
            //    0
            //    );

            if (!SetupDiGetDeviceRegistryProperty(
                deviceInfoHandle,
                &deviceInfoData,
                SPDRP_FRIENDLYNAME,
                NULL,
                (PBYTE)diskFriendlyName,
                ARRAYSIZE(diskFriendlyName),
                NULL
                ))
            {
                continue;
            }

            diskEntry = PhAllocate(sizeof(DISK_ENUM_ENTRY));
            memset(diskEntry, 0, sizeof(DISK_ENUM_ENTRY));

            diskEntry->DeviceIndex = ULONG_MAX; // Note: Do not initialize to zero.
            diskEntry->DeviceName = PhCreateString(diskFriendlyName);
            diskEntry->DevicePath = PhCreateString(deviceInterfaceDetail->DevicePath);

            if (NT_SUCCESS(DiskDriveCreateHandle(
                &deviceHandle,
                diskEntry->DevicePath
                )))
            {
                ULONG diskIndex = ULONG_MAX; // Note: Do not initialize to zero

                if (NT_SUCCESS(DiskDriveQueryDeviceTypeAndNumber(
                    deviceHandle,
                    &diskIndex,
                    NULL
                    )))
                {
                    PPH_STRING diskMountPoints = PH_AUTO_T(PH_STRING, DiskDriveQueryDosMountPoints(diskIndex));

                    diskEntry->DeviceIndex = diskIndex;
                    diskEntry->DevicePresent = TRUE;

                    if (!PhIsNullOrEmptyString(diskMountPoints))
                    {
                        diskEntry->DeviceMountPoints = PhFormatString(
                            L"Disk %lu (%s) [%s]",
                            diskIndex,
                            diskMountPoints->Buffer,
                            diskFriendlyName
                            );
                    }
                    else
                    {
                        diskEntry->DeviceMountPoints = PhFormatString(
                            L"Disk %lu [%s]",
                            diskIndex,
                            diskFriendlyName
                            );
                    }
                }

                NtClose(deviceHandle);
            }

            PhAddItemList(deviceList, diskEntry);
        }

        PhFree(deviceInterfaceDetail);
    }

    SetupDiDestroyDeviceInfoList(deviceInfoHandle);

    // Sort the entries
    qsort(deviceList->Items, deviceList->Count, sizeof(PVOID), DiskEntryCompareFunction);

    Context->EnumeratingDisks = TRUE;
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
    Context->EnumeratingDisks = FALSE;


    // HACK: Show all unknown devices.
    Context->EnumeratingDisks = TRUE;
    PhAcquireQueuedLockShared(&DiskDrivesListLock);

    for (ULONG i = 0; i < DiskDrivesList->Count; i++)
    {
        ULONG index = -1;
        BOOLEAN found = FALSE;
        PDV_DISK_ENTRY entry = PhReferenceObjectSafe(DiskDrivesList->Items[i]);

        if (!entry)
            continue;

        while ((index = PhFindListViewItemByFlags(
            Context->ListViewHandle,
            index,
            LVNI_ALL
            )) != -1)
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
    Context->EnumeratingDisks = FALSE;
}

PPH_STRING FindDiskDeviceInstance(
    _In_ PPH_STRING DevicePath
    )
{
    PPH_STRING deviceIdString = NULL;
    HDEVINFO deviceInfoHandle;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData = { sizeof(SP_DEVICE_INTERFACE_DATA) };
    SP_DEVINFO_DATA deviceInfoData = { sizeof(SP_DEVINFO_DATA) };
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetail;
    ULONG deviceInfoLength = 0;

    if ((deviceInfoHandle = SetupDiGetClassDevs(
        &GUID_DEVINTERFACE_DISK,
        NULL,
        NULL,
        DIGCF_DEVICEINTERFACE
        )) == INVALID_HANDLE_VALUE)
    {
        return NULL;
    }

    for (ULONG i = 0; SetupDiEnumDeviceInterfaces(deviceInfoHandle, NULL, &GUID_DEVINTERFACE_DISK, i, &deviceInterfaceData); i++)
    {
        if (SetupDiGetDeviceInterfaceDetail(
            deviceInfoHandle,
            &deviceInterfaceData,
            0,
            0,
            &deviceInfoLength,
            &deviceInfoData
            ) || GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            continue;
        }

        deviceInterfaceDetail = PhAllocate(deviceInfoLength);
        deviceInterfaceDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (SetupDiGetDeviceInterfaceDetail(
            deviceInfoHandle,
            &deviceInterfaceData,
            deviceInterfaceDetail,
            deviceInfoLength,
            &deviceInfoLength,
            &deviceInfoData
            ))
        {
            if (PhEqualStringZ(deviceInterfaceDetail->DevicePath, DevicePath->Buffer, TRUE))
            {
                deviceIdString = PhCreateStringEx(NULL, 0x100);

                SetupDiGetDeviceInstanceId(
                    deviceInfoHandle,
                    &deviceInfoData,
                    deviceIdString->Buffer,
                    (ULONG)deviceIdString->Length,
                    NULL
                    );

                PhTrimToNullTerminatorString(deviceIdString);
            }
        }

        PhFree(deviceInterfaceDetail);
    }

    SetupDiDestroyDeviceInfoList(deviceInfoHandle);

    return deviceIdString;
}

//VOID LoadDiskDriveImages(
//    _In_ PDV_DISK_OPTIONS_CONTEXT Context
//    )
//{
//    HICON smallIcon = NULL;
//
//    Context->ImageList = ImageList_Create(
//        GetSystemMetrics(SM_CXSMICON),
//        GetSystemMetrics(SM_CYSMICON),
//        ILC_COLOR32,
//        1,
//        1
//        );
//
//    // We could use SetupDiLoadClassIcon but this works.
//    // Copied from HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Control\Class\{4d36e967-e325-11ce-bfc1-08002be10318}\\IconPath
//    // The index is only valid on Vista and above.
//    ExtractIconEx(
//        L"%SystemRoot%\\system32\\imageres.dll",
//        -32,
//        NULL,
//        &smallIcon,
//        1
//        );
//
//    if (smallIcon)
//    {
//        ImageList_AddIcon(Context->ImageList, smallIcon);
//        DestroyIcon(smallIcon);
//
//        // Set the imagelist only if the image was loaded.
//        ListView_SetImageList(
//            Context->ListViewHandle,
//            Context->ImageList,
//            LVSIL_SMALL
//            );
//    }
//}

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
        context = (PDV_DISK_OPTIONS_CONTEXT)PhAllocate(sizeof(DV_DISK_OPTIONS_CONTEXT));
        memset(context, 0, sizeof(DV_DISK_OPTIONS_CONTEXT));

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PDV_DISK_OPTIONS_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
        {
            if (context->OptionsChanged)
                DiskDrivesSaveList();

            FreeListViewDiskDriveEntries(context);

            RemoveProp(hwndDlg, L"Context");
            PhFree(context);
        }
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            // Center the property sheet.
            PhCenterWindow(GetParent(hwndDlg), GetParent(GetParent(hwndDlg)));
            // Hide the OK button.
            ShowWindow(GetDlgItem(GetParent(hwndDlg), IDOK), SW_HIDE);
            // Set the Cancel button text.
            Button_SetText(GetDlgItem(GetParent(hwndDlg), IDCANCEL), L"Close");

            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_DISKDRIVE_LISTVIEW);
            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            ListView_SetExtendedListViewStyleEx(context->ListViewHandle, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 350, L"Disk Drives");
            PhSetExtendedListView(context->ListViewHandle);
           
            if (WindowsVersion >= WINDOWS_VISTA)
            {
                ListView_EnableGroupView(context->ListViewHandle, TRUE);
                AddListViewGroup(context->ListViewHandle, 0, L"Connected");
                AddListViewGroup(context->ListViewHandle, 1, L"Disconnected");
            }

            FindDiskDrives(context);

            context->OptionsChanged = FALSE;

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->code == LVN_ITEMCHANGED)
            {
                LPNM_LISTVIEW listView = (LPNM_LISTVIEW)lParam;

                if (context->EnumeratingDisks)
                    break;

                if (listView->uChanged & LVIF_STATE)
                {
                    switch (listView->uNewState & LVIS_STATEIMAGEMASK)
                    {
                    case 0x2000: // checked
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
                    case 0x1000: // unchecked
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
                    }
                }
            }
        }
        break;
    }

    return FALSE;
}