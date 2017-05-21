/*
 * Process Hacker Plugins -
 *   Hardware Devices Plugin
 *
 * Copyright (C) 2015-2016 dmex
 * Copyright (C) 2016 wj32
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

#define INITGUID
#include "devices.h"
#include <cfgmgr32.h>
#include <ndisguid.h>

#define ITEM_CHECKED (INDEXTOSTATEIMAGEMASK(2))
#define ITEM_UNCHECKED (INDEXTOSTATEIMAGEMASK(1))

typedef struct _NET_ENUM_ENTRY
{
    BOOLEAN DevicePresent;
    IF_LUID DeviceLuid;
    PPH_STRING DeviceGuid;
    PPH_STRING DeviceName;
} NET_ENUM_ENTRY, *PNET_ENUM_ENTRY;

static int __cdecl AdapterEntryCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PNET_ENUM_ENTRY entry1 = *(PNET_ENUM_ENTRY *)elem1;
    PNET_ENUM_ENTRY entry2 = *(PNET_ENUM_ENTRY *)elem2;

    return uint64cmp(entry1->DeviceLuid.Value, entry2->DeviceLuid.Value);
}

VOID NetAdaptersLoadList(
    VOID
    )
{
    PPH_STRING settingsString;
    PH_STRINGREF remaining;

    settingsString = PhaGetStringSetting(SETTING_NAME_INTERFACE_LIST);
    remaining = settingsString->sr;

    while (remaining.Length != 0)
    {
        ULONG64 ifindex;
        ULONG64 luid64;
        PH_STRINGREF part1;
        PH_STRINGREF part2;
        PH_STRINGREF part3;
        IF_LUID ifLuid;
        DV_NETADAPTER_ID id;
        PDV_NETADAPTER_ENTRY entry;

        if (remaining.Length == 0)
            break;

        PhSplitStringRefAtChar(&remaining, ',', &part1, &remaining);
        PhSplitStringRefAtChar(&remaining, ',', &part2, &remaining);
        PhSplitStringRefAtChar(&remaining, ',', &part3, &remaining);

        PhStringToInteger64(&part1, 10, &ifindex);
        PhStringToInteger64(&part2, 10, &luid64);

        ifLuid.Value = luid64;
        InitializeNetAdapterId(&id, (IF_INDEX)ifindex, ifLuid, PhCreateString2(&part3));
        entry = CreateNetAdapterEntry(&id);
        DeleteNetAdapterId(&id);

        entry->UserReference = TRUE;
    }
}

VOID NetAdaptersSaveList(
    VOID
    )
{
    PH_STRING_BUILDER stringBuilder;
    PPH_STRING settingsString;

    PhInitializeStringBuilder(&stringBuilder, 260);

    PhAcquireQueuedLockShared(&NetworkAdaptersListLock);

    for (ULONG i = 0; i < NetworkAdaptersList->Count; i++)
    {
        PDV_NETADAPTER_ENTRY entry = PhReferenceObjectSafe(NetworkAdaptersList->Items[i]);

        if (!entry)
            continue;

        if (entry->UserReference)
        {
            PhAppendFormatStringBuilder(
                &stringBuilder,
                L"%lu,%I64u,%s,",
                entry->Id.InterfaceIndex,    // This value is UNSAFE and may change after reboot.
                entry->Id.InterfaceLuid.Value, // This value is SAFE and does not change (Vista+).
                entry->Id.InterfaceGuid->Buffer
                );
        }

        PhDereferenceObjectDeferDelete(entry);
    }

    PhReleaseQueuedLockShared(&NetworkAdaptersListLock);

    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    settingsString = PH_AUTO(PhFinalStringBuilderString(&stringBuilder));
    PhSetStringSetting2(SETTING_NAME_INTERFACE_LIST, &settingsString->sr);
}

BOOLEAN FindAdapterEntry(
    _In_ PDV_NETADAPTER_ID Id,
    _In_ BOOLEAN RemoveUserReference
    )
{
    BOOLEAN found = FALSE;

    PhAcquireQueuedLockShared(&NetworkAdaptersListLock);

    for (ULONG i = 0; i < NetworkAdaptersList->Count; i++)
    {
        PDV_NETADAPTER_ENTRY currentEntry = PhReferenceObjectSafe(NetworkAdaptersList->Items[i]);

        if (!currentEntry)
            continue;

        found = EquivalentNetAdapterId(&currentEntry->Id, Id);

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

    PhReleaseQueuedLockShared(&NetworkAdaptersListLock);

    return found;
}

VOID AddNetworkAdapterToListView(
    _In_ PDV_NETADAPTER_CONTEXT Context,
    _In_ BOOLEAN AdapterPresent,
    _In_ IF_INDEX IfIndex,
    _In_ IF_LUID Luid,
    _In_ PPH_STRING Guid,
    _In_ PPH_STRING Description
    )
{
    DV_NETADAPTER_ID adapterId;
    INT lvItemIndex;
    BOOLEAN found = FALSE;
    PDV_NETADAPTER_ID newId = NULL;

    InitializeNetAdapterId(&adapterId, IfIndex, Luid, NULL);

    for (ULONG i = 0; i < NetworkAdaptersList->Count; i++)
    {
        PDV_NETADAPTER_ENTRY entry = PhReferenceObjectSafe(NetworkAdaptersList->Items[i]);

        if (!entry)
            continue;

        if (EquivalentNetAdapterId(&entry->Id, &adapterId))
        {
            newId = PhAllocate(sizeof(DV_NETADAPTER_ID));
            CopyNetAdapterId(newId, &entry->Id);

            if (entry->UserReference)
                found = TRUE;
        }

        PhDereferenceObjectDeferDelete(entry);

        if (newId)
            break;
    }

    if (!newId)
    {
        newId = PhAllocate(sizeof(DV_NETADAPTER_ID));
        CopyNetAdapterId(newId, &adapterId);
        PhMoveReference(&newId->InterfaceGuid, Guid);
    }

    lvItemIndex = AddListViewItemGroupId(
        Context->ListViewHandle,
        AdapterPresent ? 0 : 1,
        MAXINT,
        Description->Buffer,
        newId
        );

    if (found)
        ListView_SetItemState(Context->ListViewHandle, lvItemIndex, ITEM_CHECKED, LVIS_STATEIMAGEMASK);

    DeleteNetAdapterId(&adapterId);
}

VOID FreeListViewAdapterEntries(
    _In_ PDV_NETADAPTER_CONTEXT Context
    )
{
    ULONG index = -1;

    while ((index = PhFindListViewItemByFlags(
        Context->ListViewHandle,
        index,
        LVNI_ALL
        )) != -1)
    {
        PDV_NETADAPTER_ID param;

        if (PhGetListViewItemParam(Context->ListViewHandle, index, &param))
        {
            DeleteNetAdapterId(param);
            PhFree(param);
        }
    }
}

BOOLEAN QueryNetworkDeviceInterfaceDescription(
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

    // DEVPKEY_Device_DeviceDesc doesn't give us the full adapter name.
    // DEVPKEY_Device_FriendlyName does give us the full adapter name but is only
    //  supported on Windows 8 and above.

    // We use our NetworkAdapterQueryName function to query the full adapter name
    // from the NDIS driver directly, if that fails then we use one of the above properties.

    if ((result = CM_Get_DevNode_Property( // CM_Get_DevNode_Registry_Property with CM_DRP_DEVICEDESC??
        deviceInstanceHandle,
        WindowsVersion >= WINDOWS_8 ? &DEVPKEY_Device_FriendlyName : &DEVPKEY_Device_DeviceDesc,
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
            WindowsVersion >= WINDOWS_8 ? &DEVPKEY_Device_FriendlyName : &DEVPKEY_Device_DeviceDesc,
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

VOID FindNetworkAdapters(
    _In_ PDV_NETADAPTER_CONTEXT Context
    )
{
    if (Context->UseAlternateMethod)
    {
        ULONG flags = GAA_FLAG_SKIP_UNICAST | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_INCLUDE_ALL_INTERFACES;
        ULONG bufferLength = 0;
        PVOID buffer;

        if (GetAdaptersAddresses(AF_UNSPEC, flags, NULL, NULL, &bufferLength) != ERROR_BUFFER_OVERFLOW)
            return;

        buffer = PhAllocate(bufferLength);
        memset(buffer, 0, bufferLength);

        if (GetAdaptersAddresses(AF_UNSPEC, flags, NULL, buffer, &bufferLength) == ERROR_SUCCESS)
        {
            PhAcquireQueuedLockShared(&NetworkAdaptersListLock);

            for (PIP_ADAPTER_ADDRESSES i = buffer; i; i = i->Next)
            {
                PPH_STRING description;

                if (description = PhCreateString(i->Description))
                {
                    AddNetworkAdapterToListView(
                        Context,
                        TRUE,
                        i->IfIndex,
                        i->Luid,
                        PhConvertMultiByteToUtf16(i->AdapterName),
                        description
                        );

                    PhDereferenceObject(description);
                }
            }

            PhReleaseQueuedLockShared(&NetworkAdaptersListLock);
        }

        PhFree(buffer);
    }
    else
    {
        PPH_LIST deviceList;
        PWSTR deviceInterfaceList;
        ULONG deviceInterfaceListLength = 0;
        PWSTR deviceInterface;

        if (CM_Get_Device_Interface_List_Size(
            &deviceInterfaceListLength,
            (PGUID)&GUID_DEVINTERFACE_NET,
            NULL,
            CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES
            ) != CR_SUCCESS)
        {
            return;
        }

        deviceInterfaceList = PhAllocate(deviceInterfaceListLength * sizeof(WCHAR));
        memset(deviceInterfaceList, 0, deviceInterfaceListLength * sizeof(WCHAR));

        if (CM_Get_Device_Interface_List(
            (PGUID)&GUID_DEVINTERFACE_NET,
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
            HKEY keyHandle;
            DEVINST deviceInstanceHandle;
            PPH_STRING deviceDescription = NULL;

            if (!QueryNetworkDeviceInterfaceDescription(deviceInterface, &deviceInstanceHandle, &deviceDescription))
                continue;

            if (CM_Open_DevInst_Key(
                deviceInstanceHandle,
                KEY_QUERY_VALUE,
                0,
                RegDisposition_OpenExisting,
                &keyHandle,
                CM_REGISTRY_SOFTWARE
                ) == CR_SUCCESS)
            {
                PNET_ENUM_ENTRY adapterEntry;
                HANDLE deviceHandle;

                adapterEntry = PhAllocate(sizeof(NET_ENUM_ENTRY));
                memset(adapterEntry, 0, sizeof(NET_ENUM_ENTRY));

                adapterEntry->DeviceGuid = PhQueryRegistryString(keyHandle, L"NetCfgInstanceId");
                adapterEntry->DeviceLuid.Info.IfType = QueryRegistryUlong64(keyHandle, L"*IfType");
                adapterEntry->DeviceLuid.Info.NetLuidIndex = QueryRegistryUlong64(keyHandle, L"NetLuidIndex");

                if (NT_SUCCESS(NetworkAdapterCreateHandle(&deviceHandle, adapterEntry->DeviceGuid)))
                {
                    PPH_STRING adapterName;

                    // Try query the full adapter name
                    adapterName = NetworkAdapterQueryName(deviceHandle, adapterEntry->DeviceGuid);

                    if (adapterName)
                        adapterEntry->DeviceName = adapterName;

                    adapterEntry->DevicePresent = TRUE;

                    NtClose(deviceHandle);
                }

                if (!adapterEntry->DeviceName)
                    adapterEntry->DeviceName = PhCreateString2(&deviceDescription->sr);

                PhAddItemList(deviceList, adapterEntry);

                NtClose(keyHandle);
            }

            PhClearReference(&deviceDescription);
        }

        // Sort the entries
        qsort(deviceList->Items, deviceList->Count, sizeof(PVOID), AdapterEntryCompareFunction);

        PhAcquireQueuedLockShared(&NetworkAdaptersListLock);

        for (ULONG i = 0; i < deviceList->Count; i++)
        {
            PNET_ENUM_ENTRY entry = deviceList->Items[i];

            AddNetworkAdapterToListView(
                Context,
                entry->DevicePresent,
                0,
                entry->DeviceLuid,
                entry->DeviceGuid,
                entry->DeviceName
                );

            if (entry->DeviceName)
                PhDereferenceObject(entry->DeviceName);
            // Note: DeviceGuid is disposed by WM_DESTROY.

            PhFree(entry);
        }

        PhReleaseQueuedLockShared(&NetworkAdaptersListLock);
    }

    // HACK: Show all unknown devices.
    PhAcquireQueuedLockShared(&NetworkAdaptersListLock);

    for (ULONG i = 0; i < NetworkAdaptersList->Count; i++)
    {
        ULONG index = -1;
        BOOLEAN found = FALSE;
        PDV_NETADAPTER_ENTRY entry = PhReferenceObjectSafe(NetworkAdaptersList->Items[i]);

        if (!entry)
            continue;

        while ((index = PhFindListViewItemByFlags(
            Context->ListViewHandle,
            index,
            LVNI_ALL
            )) != -1)
        {
            PDV_NETADAPTER_ID param;

            if (PhGetListViewItemParam(Context->ListViewHandle, index, &param))
            {
                if (EquivalentNetAdapterId(param, &entry->Id))
                {
                    found = TRUE;
                }
            }
        }

        if (!found)
        {
            PPH_STRING description;

            if (description = PhCreateString(L"Unknown network adapter"))
            {
                AddNetworkAdapterToListView(
                    Context,
                    FALSE,
                    entry->Id.InterfaceIndex,
                    entry->Id.InterfaceLuid,
                    entry->Id.InterfaceGuid,
                    description
                    );

                PhDereferenceObject(description);
            }
        }

        PhDereferenceObjectDeferDelete(entry);
    }

    PhReleaseQueuedLockShared(&NetworkAdaptersListLock);
}

PPH_STRING FindNetworkDeviceInstance(
    _In_ PPH_STRING DevicePath
    )
{
    PPH_STRING deviceInstanceString = NULL;
    PWSTR deviceInterfaceList;
    ULONG deviceInterfaceListLength = 0;
    PWSTR deviceInterface;

    if (CM_Get_Device_Interface_List_Size(
        &deviceInterfaceListLength,
        (PGUID)&GUID_DEVINTERFACE_NET,
        NULL,
        CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES
        ) != CR_SUCCESS)
    {
        return NULL;
    }

    deviceInterfaceList = PhAllocate(deviceInterfaceListLength * sizeof(WCHAR));
    memset(deviceInterfaceList, 0, deviceInterfaceListLength * sizeof(WCHAR));

    if (CM_Get_Device_Interface_List(
        (PGUID)&GUID_DEVINTERFACE_NET,
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
        HKEY keyHandle;
        DEVPROPTYPE devicePropertyType;
        DEVINST deviceInstanceHandle;
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

        if (CM_Locate_DevNode(
            &deviceInstanceHandle,
            deviceInstanceId,
            CM_LOCATE_DEVNODE_PHANTOM
            ) != CR_SUCCESS)
        {
            continue;
        }

        if (CM_Open_DevInst_Key(
            deviceInstanceHandle,
            KEY_QUERY_VALUE,
            0,
            RegDisposition_OpenExisting,
            &keyHandle,
            CM_REGISTRY_SOFTWARE
            ) == CR_SUCCESS)
        {
            PPH_STRING deviceGuid;

            if (deviceGuid = PhQueryRegistryString(keyHandle, L"NetCfgInstanceId"))
            {
                if (PhEqualString(deviceGuid, DevicePath, TRUE))
                {
                    deviceInstanceString = PhCreateString(deviceInstanceId);

                    PhDereferenceObject(deviceGuid);
                    NtClose(keyHandle);
                    break;
                }

                PhDereferenceObject(deviceGuid);
            }

            NtClose(keyHandle);
        }
    }

    return deviceInstanceString;
}

//VOID LoadNetworkAdapterImages(
//    _In_ PDV_NETADAPTER_CONTEXT Context
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
//    // Copied from HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Control\Class\{4d36e972-e325-11ce-bfc1-08002be10318}\\IconPath
//    // The path and index hasn't changed since Win2k.
//    ExtractIconEx(
//        L"%SystemRoot%\\system32\\setupapi.dll",
//        -5,
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

INT_PTR CALLBACK NetworkAdapterOptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_NETADAPTER_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PDV_NETADAPTER_CONTEXT)PhAllocate(sizeof(DV_NETADAPTER_CONTEXT));
        memset(context, 0, sizeof(DV_NETADAPTER_CONTEXT));

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PDV_NETADAPTER_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
        {
            if (context->OptionsChanged)
                NetAdaptersSaveList();

            FreeListViewAdapterEntries(context);

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
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_NETADAPTERS_LISTVIEW);
            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            ListView_SetExtendedListViewStyleEx(context->ListViewHandle, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 350, L"Network Adapters");
            PhSetExtendedListView(context->ListViewHandle);

            ListView_EnableGroupView(context->ListViewHandle, TRUE);
            AddListViewGroup(context->ListViewHandle, 0, L"Connected");
            AddListViewGroup(context->ListViewHandle, 1, L"Disconnected");

            FindNetworkAdapters(context);

            context->OptionsChanged = FALSE;
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_SHOW_HIDDEN_ADAPTERS:
                {
                    context->UseAlternateMethod = !context->UseAlternateMethod;

                    if (context->UseAlternateMethod)
                    {
                        ListView_EnableGroupView(context->ListViewHandle, FALSE);
                    }
                    else
                    {
                        ListView_EnableGroupView(context->ListViewHandle, TRUE);
                    }

                    FreeListViewAdapterEntries(context);
                    ListView_DeleteAllItems(context->ListViewHandle);

                    FindNetworkAdapters(context);
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

                if (!PhTryAcquireReleaseQueuedLockExclusive(&NetworkAdaptersListLock))
                    break;

                if (listView->uChanged & LVIF_STATE)
                {
                    switch (listView->uNewState & LVIS_STATEIMAGEMASK)
                    {
                    case 0x2000: // checked
                        {
                            PDV_NETADAPTER_ID param = (PDV_NETADAPTER_ID)listView->lParam;

                            if (!FindAdapterEntry(param, FALSE))
                            {
                                PDV_NETADAPTER_ENTRY entry;

                                entry = CreateNetAdapterEntry(param);
                                entry->UserReference = TRUE;
                            }

                            context->OptionsChanged = TRUE;
                        }
                        break;
                    case 0x1000: // unchecked
                        {
                            PDV_NETADAPTER_ID param = (PDV_NETADAPTER_ID)listView->lParam;

                            FindAdapterEntry(param, TRUE);

                            context->OptionsChanged = TRUE;
                        }
                        break;
                    }
                }
            }
            else if (header->code == NM_RCLICK)
            {
                PDV_NETADAPTER_ENTRY param;
                PPH_STRING deviceInstance;

                if (param = PhGetSelectedListViewItemParam(context->ListViewHandle))
                {
                    if (deviceInstance = FindNetworkDeviceInstance(param->Id.InterfaceGuid))
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