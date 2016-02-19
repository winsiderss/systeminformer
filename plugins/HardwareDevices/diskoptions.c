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

#pragma comment(lib, "Setupapi.lib")
#include <Setupapi.h>
#include <ntdddisk.h>

#define ITEM_CHECKED (INDEXTOSTATEIMAGEMASK(2))
#define ITEM_UNCHECKED (INDEXTOSTATEIMAGEMASK(1))

static _EnableThemeDialogTexture EnableThemeDialogTexture_I = NULL;

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

static BOOLEAN FindDiskEntry(
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

static VOID AddDiskDriveToListView(
    _In_ PDV_DISK_OPTIONS_CONTEXT Context,
    _In_ PPH_STRING DiskPath,
    _In_ PWSTR DiskName
    )
{
    DV_DISK_ID adapterId;
    INT lvItemIndex;
    BOOLEAN found = FALSE;
    PDV_DISK_ID newId = NULL;
    HANDLE deviceHandle = NULL;
    PPH_STRING diskName = NULL;

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

    if (NT_SUCCESS(DiskDriveCreateHandle(
        &deviceHandle,
        newId->DevicePath
        )))
    {
        ULONG diskIndex = ULONG_MAX; // Note: Do not initialize to zero.

        if (NT_SUCCESS(DiskDriveQueryDeviceTypeAndNumber(deviceHandle, &diskIndex, NULL)))
        {
            PPH_STRING diskMountPoints = PH_AUTO_T(PH_STRING, DiskDriveQueryDosMountPoints(diskIndex));

            if (!PhIsNullOrEmptyString(diskMountPoints))
            {
                diskName = PhFormatString(
                    L"Disk %lu (%s) [%s]",
                    diskIndex,
                    diskMountPoints->Buffer,
                    DiskName
                    );
            }
            else
            {
                diskName = PhFormatString(
                    L"Disk %lu [%s]",
                    diskIndex,
                    DiskName
                    );
            }
        }
    }

    lvItemIndex = PhAddListViewItem(
        Context->ListViewHandle,
        MAXINT,
        diskName ? diskName->Buffer : DiskName,
        newId
        );

    if (found)
        ListView_SetItemState(Context->ListViewHandle, lvItemIndex, ITEM_CHECKED, LVIS_STATEIMAGEMASK);

    DeleteDiskId(&adapterId);
}

static VOID FindDiskDrives(
    _In_ PDV_DISK_OPTIONS_CONTEXT Context
    )
{
    HDEVINFO deviceInfoHandle;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData = { sizeof(SP_DEVICE_INTERFACE_DATA) };
    SP_DEVINFO_DATA deviceInfoData = { sizeof(SP_DEVINFO_DATA) };
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetail = NULL;
    ULONG deviceInfoLength = 0;
    HANDLE ret_handle = NULL;

    if ((deviceInfoHandle = SetupDiGetClassDevs(&GUID_DEVINTERFACE_DISK, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE)) == INVALID_HANDLE_VALUE)
        return;

    for (ULONG i = 0; i < 1000; i++)
    {
        if (!SetupDiEnumDeviceInterfaces(deviceInfoHandle, 0, &GUID_DEVINTERFACE_DISK, i, &deviceInterfaceData))
            break;

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
            WCHAR diskFriendlyName[MAX_PATH] = L"";

            SetupDiGetDeviceRegistryProperty(
                deviceInfoHandle,
                &deviceInfoData,
                SPDRP_FRIENDLYNAME,
                NULL,
                (PBYTE)diskFriendlyName,
                ARRAYSIZE(diskFriendlyName),
                NULL
                );

            AddDiskDriveToListView(
                Context, 
                PhCreateString(deviceInterfaceDetail->DevicePath), 
                diskFriendlyName
                );
        }

        PhFree(deviceInterfaceDetail);
    }

    SetupDiDestroyDeviceInfoList(deviceInfoHandle);

    // HACK: Remove all disconnected devices.
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
            Context->OptionsChanged = TRUE;
            FindDiskEntry(&entry->Id, TRUE);
        }

        PhDereferenceObjectDeferDelete(entry);
    }
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
        context = (PDV_DISK_OPTIONS_CONTEXT)PhAllocate(sizeof(DV_DISK_OPTIONS_CONTEXT));
        memset(context, 0, sizeof(DV_DISK_OPTIONS_CONTEXT));

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PDV_DISK_OPTIONS_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
        {
            ULONG index = -1;

            while ((index = PhFindListViewItemByFlags(
                context->ListViewHandle,
                index,
                LVNI_ALL
                )) != -1)
            {
                PDV_DISK_ID param;

                if (PhGetListViewItemParam(context->ListViewHandle, index, &param))
                {
                    if (context->OptionsChanged)
                    {
                        BOOLEAN checked;

                        checked = ListView_GetItemState(context->ListViewHandle, index, LVIS_STATEIMAGEMASK) == ITEM_CHECKED;

                        if (checked)
                        {
                            if (!FindDiskEntry(param, FALSE))
                            {
                                PDV_DISK_ENTRY entry;

                                entry = CreateDiskEntry(param);
                                entry->UserReference = TRUE;
                            }
                        }
                        else
                        {
                            FindDiskEntry(param, TRUE);
                        }
                    }

                    DeleteDiskId(param);
                    PhFree(param);
                }
            }

            if (context->OptionsChanged)
                DiskDrivesSaveList();

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
            // TODO: Why does this need to use GetParent?
            PhCenterWindow(GetParent(hwndDlg), GetParent(GetParent(hwndDlg)));

            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_DISKDRIVE_LISTVIEW);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            ListView_SetExtendedListViewStyleEx(context->ListViewHandle, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 350, L"Disk Drives");
            PhSetExtendedListView(context->ListViewHandle);

            FindDiskDrives(context);

            context->OptionsChanged = FALSE;

            if (!EnableThemeDialogTexture_I)
                EnableThemeDialogTexture_I = PhGetModuleProcAddress(L"uxtheme.dll", "EnableThemeDialogTexture");

            if (EnableThemeDialogTexture_I)
                EnableThemeDialogTexture_I(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->code == LVN_ITEMCHANGED)
            {
                LPNM_LISTVIEW listView = (LPNM_LISTVIEW)lParam;

                if (listView->uChanged & LVIF_STATE)
                {
                    switch (listView->uNewState & 0x3000)
                    {
                    case 0x2000: // checked
                    case 0x1000: // unchecked
                        context->OptionsChanged = TRUE;
                        break;
                    }
                }
            }
        }
        break;
    }

    return FALSE;
}