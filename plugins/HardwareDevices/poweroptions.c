/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2021-2023
 *
 */

#include "devices.h"
#include <devguid.h>
#include <emi.h>

typedef struct _RAPL_ENUM_ENTRY
{
    ULONG DeviceIndex;
    BOOLEAN DevicePresent;
    BOOLEAN DeviceCapable;
    BOOLEAN DeviceSupported;
    PPH_STRING DevicePath;
    PPH_STRING DeviceName;
    ULONG ChannelIndex[EV_EMI_DEVICE_INDEX_MAX];
} RAPL_ENUM_ENTRY, *PRAPL_ENUM_ENTRY;

static int __cdecl RaplDeviceEntryCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PRAPL_ENUM_ENTRY entry1 = *(PRAPL_ENUM_ENTRY*)elem1;
    PRAPL_ENUM_ENTRY entry2 = *(PRAPL_ENUM_ENTRY*)elem2;

    return uint64cmp(entry1->DeviceIndex, entry2->DeviceIndex);
}

VOID RaplDevicesLoadList(
    VOID
    )
{
    PPH_STRING settingsString;
    PH_STRINGREF remaining;

    settingsString = PhGetStringSetting(SETTING_NAME_RAPL_LIST);

    if (!PhIsNullOrEmptyString(settingsString))
    {
        remaining = PhGetStringRef(settingsString);

        while (remaining.Length != 0)
        {
            PH_STRINGREF part;
            DV_RAPL_ID id;
            PDV_RAPL_ENTRY entry;

            if (remaining.Length == 0)
                break;

            PhSplitStringRefAtChar(&remaining, L',', &part, &remaining);

            // Convert settings path for compatibility. (dmex)
            if (part.Length > sizeof(UNICODE_NULL) && part.Buffer[1] == OBJ_NAME_PATH_SEPARATOR)
                part.Buffer[1] = L'?';

            InitializeRaplDeviceId(&id, PhCreateString2(&part));
            entry = CreateRaplDeviceEntry(&id);
            DeleteRaplDeviceId(&id);

            entry->UserReference = TRUE;
        }
    }

    PhClearReference(&settingsString);
}

VOID RaplDevicesSaveList(
    VOID
    )
{
    PH_STRING_BUILDER stringBuilder;
    PPH_STRING settingsString;

    PhInitializeStringBuilder(&stringBuilder, 260);

    PhAcquireQueuedLockShared(&RaplDevicesListLock);

    for (ULONG i = 0; i < RaplDevicesList->Count; i++)
    {
        PDV_RAPL_ENTRY entry = PhReferenceObjectSafe(RaplDevicesList->Items[i]);

        if (!entry)
            continue;

        if (entry->UserReference)
        {
            PhAppendStringBuilder(&stringBuilder, &entry->Id.DevicePath->sr);
            PhAppendCharStringBuilder(&stringBuilder, L',');
        }

        PhDereferenceObjectDeferDelete(entry);
    }

    PhReleaseQueuedLockShared(&RaplDevicesListLock);

    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    settingsString = PH_AUTO(PhFinalStringBuilderString(&stringBuilder));
    PhSetStringSetting2(SETTING_NAME_RAPL_LIST, &settingsString->sr);
}

BOOLEAN FindRaplDeviceEntry(
    _In_ PDV_RAPL_ID Id,
    _In_ BOOLEAN RemoveUserReference
    )
{
    BOOLEAN found = FALSE;

    PhAcquireQueuedLockShared(&RaplDevicesListLock);

    for (ULONG i = 0; i < RaplDevicesList->Count; i++)
    {
        PDV_RAPL_ENTRY currentEntry = PhReferenceObjectSafe(RaplDevicesList->Items[i]);

        if (!currentEntry)
            continue;

        found = EquivalentRaplDeviceId(&currentEntry->Id, Id);

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

    PhReleaseQueuedLockShared(&RaplDevicesListLock);

    return found;
}

VOID AddRaplDeviceToListView(
    _In_ PDV_RAPL_OPTIONS_CONTEXT Context,
    _In_ BOOLEAN DevicePresent,
    _In_ PPH_STRING DevicePath,
    _In_ PPH_STRING DeviceName
    )
{
    DV_RAPL_ID deviceId;
    INT lvItemIndex;
    BOOLEAN found = FALSE;
    PDV_RAPL_ID newId = NULL;

    InitializeRaplDeviceId(&deviceId, DevicePath);

    for (ULONG i = 0; i < RaplDevicesList->Count; i++)
    {
        PDV_RAPL_ENTRY entry = PhReferenceObjectSafe(RaplDevicesList->Items[i]);

        if (!entry)
            continue;

        if (EquivalentRaplDeviceId(&entry->Id, &deviceId))
        {
            newId = PhAllocate(sizeof(DV_RAPL_ID));
            CopyRaplDeviceId(newId, &entry->Id);

            if (entry->UserReference)
                found = TRUE;
        }

        PhDereferenceObjectDeferDelete(entry);

        if (newId)
            break;
    }

    if (!newId)
    {
        newId = PhAllocate(sizeof(DV_RAPL_ID));
        CopyRaplDeviceId(newId, &deviceId);
        PhMoveReference(&newId->DevicePath, DevicePath);
    }

    lvItemIndex = PhAddListViewGroupItem(
        Context->ListViewHandle,
        DevicePresent ? 0 : 1,
        MAXINT,
        DeviceName->Buffer,
        newId
        );

    if (found)
        ListView_SetCheckState(Context->ListViewHandle, lvItemIndex, TRUE);

    DeleteRaplDeviceId(&deviceId);
}

VOID FreeListViewRaplDeviceEntries(
    _In_ PDV_RAPL_OPTIONS_CONTEXT Context
    )
{
    INT index = INT_ERROR;

    while ((index = PhFindListViewItemByFlags(
        Context->ListViewHandle,
        index,
        LVNI_ALL
        )) != INT_ERROR)
    {
        PDV_RAPL_ID param;

        if (PhGetListViewItemParam(Context->ListViewHandle, index, &param))
        {
            DeleteRaplDeviceId(param);
            PhFree(param);
        }
    }
}

_Success_(return)
BOOLEAN QueryRaplDeviceInterfaceDescription(
    _In_ PWSTR DeviceInterface,
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
    *DeviceDescription = deviceDescription;

    return TRUE;
}

VOID FindRaplDevices(
    _In_ PDV_RAPL_OPTIONS_CONTEXT Context
    )
{
    ULONG deviceIndex = 0;
    PPH_LIST deviceList;
    PWSTR deviceInterfaceList;
    ULONG deviceInterfaceListLength = 0;
    PWSTR deviceInterface;

    if (CM_Get_Device_Interface_List_Size(
        &deviceInterfaceListLength,
        (PGUID)&GUID_DEVICE_ENERGY_METER,
        NULL,
        CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES
        ) != CR_SUCCESS)
    {
        return;
    }

    deviceInterfaceList = PhAllocate(deviceInterfaceListLength * sizeof(WCHAR));
    memset(deviceInterfaceList, 0, deviceInterfaceListLength * sizeof(WCHAR));

    if (CM_Get_Device_Interface_List(
        (PGUID)&GUID_DEVICE_ENERGY_METER,
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
        PPH_STRING deviceDescription;
        HANDLE deviceHandle;
        PRAPL_ENUM_ENTRY deviceEntry;

        if (!QueryRaplDeviceInterfaceDescription(deviceInterface, &deviceDescription))
            continue;

        // Convert path to avoid conversion during interval updates. (dmex)
        if (deviceInterface[1] == OBJ_NAME_PATH_SEPARATOR)
            deviceInterface[1] = L'?';

        deviceEntry = PhAllocateZero(sizeof(RAPL_ENUM_ENTRY));
        deviceEntry->DeviceIndex = ++deviceIndex;
        deviceEntry->DeviceName = PhCreateString2(&deviceDescription->sr);
        deviceEntry->DevicePath = PhCreateString(deviceInterface);
        memset(deviceEntry->ChannelIndex, ULONG_MAX, sizeof(deviceEntry->ChannelIndex));

        if (NT_SUCCESS(PhCreateFile(
            &deviceHandle,
            &deviceEntry->DevicePath->sr,
            FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            EMI_VERSION version;

            memset(&version, 0, sizeof(EMI_VERSION));

            if (NT_SUCCESS(PhDeviceIoControlFile(
                deviceHandle,
                IOCTL_EMI_GET_VERSION,
                NULL,
                0,
                &version,
                sizeof(EMI_VERSION),
                NULL
                )))
            {
                if (version.EmiVersion == EMI_VERSION_V2)
                {
                    deviceEntry->DeviceCapable = TRUE;
                }
            }

            if (deviceEntry->DeviceCapable)
            {
                EMI_METADATA_SIZE metadataSize;
                EMI_METADATA_V2* metadata;

                memset(&metadataSize, 0, sizeof(EMI_METADATA_SIZE));

                if (NT_SUCCESS(PhDeviceIoControlFile(
                    deviceHandle,
                    IOCTL_EMI_GET_METADATA_SIZE,
                    NULL,
                    0,
                    &metadataSize,
                    sizeof(EMI_METADATA_SIZE),
                    NULL
                    )))
                {
                    metadata = PhAllocate(metadataSize.MetadataSize);
                    memset(metadata, 0, metadataSize.MetadataSize);

                    if (NT_SUCCESS(PhDeviceIoControlFile(
                        deviceHandle,
                        IOCTL_EMI_GET_METADATA,
                        NULL,
                        0,
                        metadata,
                        metadataSize.MetadataSize,
                        NULL
                        )))
                    {
                        EMI_CHANNEL_V2* channels = metadata->Channels;

                        if (channels->MeasurementUnit == EmiMeasurementUnitPicowattHours)
                        {
                            for (ULONG i = 0; i < metadata->ChannelCount; i++)
                            {
                                if (PhEqualStringZ(channels->ChannelName, L"RAPL_Package0_PKG", TRUE))
                                    deviceEntry->ChannelIndex[EV_EMI_DEVICE_INDEX_PACKAGE] = i;
                                if (PhEqualStringZ(channels->ChannelName, L"RAPL_Package0_PP0", TRUE))
                                    deviceEntry->ChannelIndex[EV_EMI_DEVICE_INDEX_CORE] = i;
                                if (PhEqualStringZ(channels->ChannelName, L"RAPL_Package0_PP1", TRUE))
                                    deviceEntry->ChannelIndex[EV_EMI_DEVICE_INDEX_GPUDISCRETE] = i;
                                if (PhEqualStringZ(channels->ChannelName, L"RAPL_Package0_DRAM", TRUE))
                                    deviceEntry->ChannelIndex[EV_EMI_DEVICE_INDEX_DIMM] = i;

                                channels = EMI_CHANNEL_V2_NEXT_CHANNEL(channels);
                            }
                        }

                        if (
                            deviceEntry->ChannelIndex[EV_EMI_DEVICE_INDEX_PACKAGE] != ULONG_MAX &&
                            deviceEntry->ChannelIndex[EV_EMI_DEVICE_INDEX_CORE] != ULONG_MAX &&
                            deviceEntry->ChannelIndex[EV_EMI_DEVICE_INDEX_GPUDISCRETE] != ULONG_MAX &&
                            deviceEntry->ChannelIndex[EV_EMI_DEVICE_INDEX_DIMM] != ULONG_MAX
                            )
                        {
                            deviceEntry->DeviceSupported = TRUE;
                        }
                    }

                    PhFree(metadata);
                }
            }

            deviceEntry->DevicePresent = TRUE;
            NtClose(deviceHandle);
        }

        if (deviceEntry->DevicePresent && !deviceEntry->DeviceSupported)
        {
            // TODO: Remove this once we know our channel names are correct. (dmex)
            PhMoveReference(&deviceEntry->DeviceName, PhConcatStringRefZ(&deviceEntry->DeviceName->sr, L" [UNKNOWN DEVICE - PLEASE REPORT ON GITHUB]"));
        }

        PhAddItemList(deviceList, deviceEntry);

        PhDereferenceObject(deviceDescription);
    }

    PhFree(deviceInterfaceList);

    // Sort the entries
    qsort(deviceList->Items, deviceList->Count, sizeof(PVOID), RaplDeviceEntryCompareFunction);

    PhAcquireQueuedLockShared(&RaplDevicesListLock);
    for (ULONG i = 0; i < deviceList->Count; i++)
    {
        PRAPL_ENUM_ENTRY entry = deviceList->Items[i];

        AddRaplDeviceToListView(
            Context,
            entry->DevicePresent,
            entry->DevicePath,
            entry->DeviceName
            );

        if (entry->DeviceName)
            PhDereferenceObject(entry->DeviceName);
        // Note: DevicePath is disposed by WM_DESTROY.

        PhFree(entry);
    }
    PhReleaseQueuedLockShared(&RaplDevicesListLock);

    // HACK: Show all unknown devices.
    PhAcquireQueuedLockShared(&RaplDevicesListLock);
    for (ULONG i = 0; i < RaplDevicesList->Count; i++)
    {
        INT index = INT_ERROR;
        BOOLEAN found = FALSE;
        PDV_RAPL_ENTRY entry = PhReferenceObjectSafe(RaplDevicesList->Items[i]);

        if (!entry)
            continue;

        while ((index = PhFindListViewItemByFlags(
            Context->ListViewHandle,
            index,
            LVNI_ALL
            )) != INT_ERROR)
        {
            PDV_RAPL_ID param;

            if (PhGetListViewItemParam(Context->ListViewHandle, index, &param))
            {
                if (EquivalentRaplDeviceId(param, &entry->Id))
                {
                    found = TRUE;
                }
            }
        }

        if (!found)
        {
            PPH_STRING description;

            if (description = PhCreateString(L"Unknown device"))
            {
                AddRaplDeviceToListView(
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
    PhReleaseQueuedLockShared(&RaplDevicesListLock);
}

PPH_STRING FindRaplDeviceInstance(
    _In_ PPH_STRING DevicePath
    )
{
    PPH_STRING deviceInstanceString = NULL;
    PWSTR deviceInterfaceList;
    ULONG deviceInterfaceListLength = 0;
    PWSTR deviceInterface;

    if (CM_Get_Device_Interface_List_Size(
        &deviceInterfaceListLength,
        (PGUID)&GUID_DEVICE_ENERGY_METER,
        NULL,
        CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES
        ) != CR_SUCCESS)
    {
        return NULL;
    }

    deviceInterfaceList = PhAllocate(deviceInterfaceListLength * sizeof(WCHAR));
    memset(deviceInterfaceList, 0, deviceInterfaceListLength * sizeof(WCHAR));

    if (CM_Get_Device_Interface_List(
        (PGUID)&GUID_DEVICE_ENERGY_METER,
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

VOID LoadRaplDeviceImages(
    _In_ PDV_RAPL_OPTIONS_CONTEXT Context
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
        &GUID_DEVCLASS_PROCESSOR,
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
            &GUID_DEVCLASS_PROCESSOR,
            &DEVPKEY_DeviceClass_IconPath,
            &devicePropertyType,
            (PBYTE)deviceIconPath->Buffer,
            &bufferSize,
            0
            );
    }

    PhTrimToNullTerminatorString(deviceIconPath);

    PhSplitStringRefAtChar(&deviceIconPath->sr, L',', &dllPartSr, &indexPartSr);
    PhStringToInteger64(&indexPartSr, 10, &index);
    PhMoveReference(&deviceIconPath, PhExpandEnvironmentStrings(&dllPartSr));

    if (PhExtractIconEx(&deviceIconPath->sr, FALSE, (INT)index, &smallIcon, NULL, dpiValue))
    {
        HIMAGELIST imageList = PhImageListCreate(
            PhGetDpi(24, dpiValue), // PhGetSystemMetrics(SM_CXSMICON)
            PhGetDpi(24, dpiValue), // PhGetSystemMetrics(SM_CYSMICON)
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

INT_PTR CALLBACK RaplDeviceOptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_RAPL_OPTIONS_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(DV_RAPL_OPTIONS_CONTEXT));
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
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_RAPLDEVICE_LISTVIEW);
            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            ListView_SetExtendedListViewStyleEx(context->ListViewHandle, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 350, L"RAPL Drives");
            PhSetExtendedListView(context->ListViewHandle);
            LoadRaplDeviceImages(context);

            ListView_EnableGroupView(context->ListViewHandle, TRUE);
            PhAddListViewGroup(context->ListViewHandle, 0, L"Connected");
            PhAddListViewGroup(context->ListViewHandle, 1, L"Disconnected");

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            FindRaplDevices(context);

            if (ListView_GetItemCount(context->ListViewHandle) == 0)
                PhSetWindowStyle(context->ListViewHandle, WS_BORDER, WS_BORDER);

            context->OptionsChanged = FALSE;
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);

            if (context->OptionsChanged)
                RaplDevicesSaveList();

            FreeListViewRaplDeviceEntries(context);
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
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->code == LVN_ITEMCHANGED)
            {
                LPNM_LISTVIEW listView = (LPNM_LISTVIEW)lParam;

                if (!PhTryAcquireReleaseQueuedLockExclusive(&RaplDevicesListLock))
                    break;

                if (listView->uChanged & LVIF_STATE)
                {
                    switch (listView->uNewState & LVIS_STATEIMAGEMASK)
                    {
                    case INDEXTOSTATEIMAGEMASK(2): // checked
                        {
                            PDV_RAPL_ID param = (PDV_RAPL_ID)listView->lParam;

                            if (!FindRaplDeviceEntry(param, FALSE))
                            {
                                PDV_RAPL_ENTRY entry;

                                entry = CreateRaplDeviceEntry(param);
                                entry->UserReference = TRUE;
                            }

                            context->OptionsChanged = TRUE;
                        }
                        break;
                    case INDEXTOSTATEIMAGEMASK(1): // unchecked
                        {
                            PDV_RAPL_ID param = (PDV_RAPL_ID)listView->lParam;

                            FindRaplDeviceEntry(param, TRUE);

                            context->OptionsChanged = TRUE;
                        }
                        break;
                    }
                }
            }
            else if (header->code == NM_RCLICK)
            {
                PDV_RAPL_ID param;
                PPH_STRING deviceInstance;

                if (param = PhGetSelectedListViewItemParam(context->ListViewHandle))
                {
                    if (deviceInstance = FindRaplDeviceInstance(param->DevicePath))
                    {
                        ShowDeviceMenu(hwndDlg, deviceInstance);
                        PhDereferenceObject(deviceInstance);

                        FreeListViewRaplDeviceEntries(context);
                        ListView_DeleteAllItems(context->ListViewHandle);
                        FindRaplDevices(context);
                    }
                }
            }
            else if (header->code == NM_DBLCLK)
            {
                PDV_RAPL_ID param;
                PPH_STRING deviceInstance;

                if (param = PhGetSelectedListViewItemParam(context->ListViewHandle))
                {
                    if (deviceInstance = FindRaplDeviceInstance(param->DevicePath))
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
