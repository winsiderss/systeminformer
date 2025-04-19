/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2021-2024
 *
 */

#include "devices.h"
#include <devguid.h>
#include <cfgmgr32.h>
#include <ntddvdeo.h>

typedef struct _GPU_ENUM_ENTRY
{
    ULONG DeviceIndex;

    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN DevicePresent : 1;
            BOOLEAN DeviceSupported : 1;
            BOOLEAN SoftwareDevice : 1;
            BOOLEAN NpuDevice : 1;
            BOOLEAN Spare : 4;
        };
    };

    PPH_STRING DevicePath;
    PPH_STRING DeviceName;
    LUID AdapterLuid;
} GPU_ENUM_ENTRY, *PGPU_ENUM_ENTRY;

static int __cdecl GraphicsDeviceEntryCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PGPU_ENUM_ENTRY entry1 = *(PGPU_ENUM_ENTRY*)elem1;
    PGPU_ENUM_ENTRY entry2 = *(PGPU_ENUM_ENTRY*)elem2;

    return uint64cmp(entry1->DeviceIndex, entry2->DeviceIndex);
}

VOID GraphicsDevicesLoadList(
    VOID
    )
{
    PPH_STRING settingsString;
    PH_STRINGREF remaining;

    settingsString = PhGetStringSetting(SETTING_NAME_GRAPHICS_LIST);

    if (!PhIsNullOrEmptyString(settingsString))
    {
        remaining = PhGetStringRef(settingsString);

        while (remaining.Length != 0)
        {
            PH_STRINGREF part;
            DV_GPU_ID id;
            PDV_GPU_ENTRY entry;

            if (remaining.Length == 0)
                break;

            PhSplitStringRefAtChar(&remaining, L',', &part, &remaining);

            InitializeGraphicsDeviceId(&id, PhCreateString2(&part));
            entry = CreateGraphicsDeviceEntry(&id);
            DeleteGraphicsDeviceId(&id);

            entry->UserReference = TRUE;
        }
    }

    PhClearReference(&settingsString);
}

VOID GraphicsDevicesSaveList(
    VOID
    )
{
    PH_STRING_BUILDER stringBuilder;
    PPH_STRING settingsString;

    PhInitializeStringBuilder(&stringBuilder, 260);

    PhAcquireQueuedLockShared(&GraphicsDevicesListLock);

    for (ULONG i = 0; i < GraphicsDevicesList->Count; i++)
    {
        PDV_GPU_ENTRY entry = PhReferenceObjectSafe(GraphicsDevicesList->Items[i]);

        if (!entry)
            continue;

        if (entry->UserReference)
        {
            // %s,
            PhAppendStringBuilder(&stringBuilder, &entry->Id.DevicePath->sr);
            PhAppendCharStringBuilder(&stringBuilder, L',');
        }

        PhDereferenceObjectDeferDelete(entry);
    }

    PhReleaseQueuedLockShared(&GraphicsDevicesListLock);

    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    settingsString = PhFinalStringBuilderString(&stringBuilder);
    PhSetStringSetting2(SETTING_NAME_GRAPHICS_LIST, &settingsString->sr);
    PhDereferenceObject(settingsString);
}

BOOLEAN FindGraphicsDeviceEntry(
    _In_ PDV_GPU_ID Id,
    _In_ BOOLEAN RemoveUserReference
    )
{
    BOOLEAN found = FALSE;

    PhAcquireQueuedLockShared(&GraphicsDevicesListLock);

    for (ULONG i = 0; i < GraphicsDevicesList->Count; i++)
    {
        PDV_GPU_ENTRY currentEntry = PhReferenceObjectSafe(GraphicsDevicesList->Items[i]);

        if (!currentEntry)
            continue;

        found = EquivalentGraphicsDeviceId(&currentEntry->Id, Id);

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

    PhReleaseQueuedLockShared(&GraphicsDevicesListLock);

    return found;
}

VOID AddGraphicsDeviceToListView(
    _In_ PDV_GPU_OPTIONS_CONTEXT Context,
    _In_ BOOLEAN DevicePresent,
    _In_ PPH_STRING DevicePath,
    _In_ PPH_STRING DeviceName
    )
{
    DV_GPU_ID deviceId;
    INT lvItemIndex;
    BOOLEAN found = FALSE;
    PDV_GPU_ID newId = NULL;

    InitializeGraphicsDeviceId(&deviceId, DevicePath);

    for (ULONG i = 0; i < GraphicsDevicesList->Count; i++)
    {
        PDV_GPU_ENTRY entry = PhReferenceObjectSafe(GraphicsDevicesList->Items[i]);

        if (!entry)
            continue;

        if (EquivalentGraphicsDeviceId(&entry->Id, &deviceId))
        {
            newId = PhAllocate(sizeof(DV_GPU_ID));
            CopyGraphicsDeviceId(newId, &entry->Id);

            if (entry->UserReference)
                found = TRUE;
        }

        PhDereferenceObjectDeferDelete(entry);

        if (newId)
            break;
    }

    if (!newId)
    {
        newId = PhAllocate(sizeof(DV_GPU_ID));
        CopyGraphicsDeviceId(newId, &deviceId);
        PhMoveReference(&newId->DevicePath, DevicePath);
    }

    lvItemIndex = PhAddListViewGroupItem(
        Context->ListViewHandle,
        DevicePresent ? 0 : 1,
        MAXINT,
        PhGetString(DeviceName),
        newId
        );

    if (found)
        ListView_SetCheckState(Context->ListViewHandle, lvItemIndex, TRUE);

    DeleteGraphicsDeviceId(&deviceId);
}

VOID FreeListViewGraphicsDeviceEntries(
    _In_ PDV_GPU_OPTIONS_CONTEXT Context
    )
{
    INT index = INT_ERROR;

    while ((index = PhFindListViewItemByFlags(
        Context->ListViewHandle,
        index,
        LVNI_ALL
        )) != INT_ERROR)
    {
        PDV_GPU_ID param;

        if (PhGetListViewItemParam(Context->ListViewHandle, index, &param))
        {
            DeleteGraphicsDeviceId(param);
            PhFree(param);
        }
    }
}

BOOLEAN GraphicsDeviceIsSoftwareDevice(
    _In_ D3DKMT_HANDLE AdapterHandle
    )
{
    D3DKMT_ADAPTERTYPE adapterType;

    memset(&adapterType, 0, sizeof(D3DKMT_ADAPTERTYPE));

    if (NT_SUCCESS(GraphicsQueryAdapterInformation(
        AdapterHandle,
        KMTQAITYPE_ADAPTERTYPE,
        &adapterType,
        sizeof(D3DKMT_ADAPTERTYPE)
        )))
    {
        if (adapterType.SoftwareDevice) // adapterType.HybridIntegrated
        {
            return TRUE;
        }
    }

    return FALSE;
}

PPH_STRING GraphicsDeviceQueryInterfaceName(
    _In_ PWSTR DeviceInterface
    )
{
    DEVPROPTYPE devicePropertyType;
    DEVINST deviceInstanceHandle;
    ULONG deviceInstanceIdLength = MAX_DEVICE_ID_LEN;
    WCHAR deviceInstanceId[MAX_DEVICE_ID_LEN];

    if (CM_Get_Device_Interface_Property(
        DeviceInterface,
        &DEVPKEY_Device_InstanceId,
        &devicePropertyType,
        (PBYTE)deviceInstanceId,
        &deviceInstanceIdLength,
        0
        ) == CR_SUCCESS)
    {
        if (CM_Locate_DevNode(&deviceInstanceHandle, deviceInstanceId, CM_LOCATE_DEVNODE_NORMAL) == CR_SUCCESS)
        {
            return GraphicsQueryDevicePropertyString(deviceInstanceHandle, &DEVPKEY_NAME);
        }
    }

    return PhCreateString(L"Unknown device");
}

VOID FindGraphicsDevices(
    _In_ PDV_GPU_OPTIONS_CONTEXT Context
    )
{
    PPH_LIST deviceList;
    ULONG deviceIndex = 0;
    ULONG deviceCount = 0;
    PDEV_OBJECT deviceObjects = NULL;
    LUID fakeLuid;

    const DEVPROPCOMPKEY deviceProperties[] =
    {
        { DEVPKEY_Device_InstanceId, DEVPROP_STORE_SYSTEM, NULL },
        { DEVPKEY_Device_DeviceDesc, DEVPROP_STORE_SYSTEM, NULL },
    };

    const DEVPROP_FILTER_EXPRESSION deviceFilter[] =
    {
        {DEVPROP_OPERATOR_OR_OPEN, {0}},
        {DEVPROP_OPERATOR_EQUALS,{{DEVPKEY_DeviceInterface_ClassGuid, DEVPROP_STORE_SYSTEM, NULL}, DEVPROP_TYPE_GUID, sizeof(GUID), (PVOID)&GUID_DISPLAY_DEVICE_ARRIVAL}},
        {DEVPROP_OPERATOR_EQUALS,{{DEVPKEY_DeviceInterface_ClassGuid, DEVPROP_STORE_SYSTEM, NULL}, DEVPROP_TYPE_GUID, sizeof(GUID), (PVOID)&GUID_COMPUTE_DEVICE_ARRIVAL}},
        {DEVPROP_OPERATOR_OR_CLOSE, {0}}
    };

    fakeLuid.HighPart = LONG_MAX;
    fakeLuid.LowPart = ULONG_MAX;

    if (SUCCEEDED(PhDevGetObjects(
        DevObjectTypeDeviceInterface,
        DevQueryFlagNone,
        RTL_NUMBER_OF(deviceProperties),
        deviceProperties,
        RTL_NUMBER_OF(deviceFilter),
        deviceFilter,
        &deviceCount,
        &deviceObjects
        )))
    {
        deviceList = PhCreateList(deviceCount);

        for (ULONG i = 0; i < deviceCount; i++)
        {
            PDEV_OBJECT device = &deviceObjects[i];
            D3DKMT_HANDLE adapterHandle;
            PGPU_ENUM_ENTRY entry;
            ULONG propertyCount;
            const DEVPROPERTY* propertyList;
            DEVPROPCOMPKEY requestedProperties[] =
            {
                { DEVPKEY_NAME, DEVPROP_STORE_SYSTEM, NULL },
                { DEVPKEY_Device_LocationInfo, DEVPROP_STORE_SYSTEM, NULL },
                { DEVPKEY_Gpu_PhysicalAdapterIndex, DEVPROP_STORE_SYSTEM, NULL },
                { DEVPKEY_Gpu_Luid, DEVPROP_STORE_SYSTEM, NULL },
            };

            entry = PhAllocateZero(sizeof(GPU_ENUM_ENTRY));
            entry->DeviceIndex = ++deviceIndex;
            entry->DevicePath = PhCreateString(device->pszObjectId);

            entry->AdapterLuid = fakeLuid;
            fakeLuid.HighPart--;

            if (NT_SUCCESS(GraphicsOpenAdapterFromDeviceName(&adapterHandle, NULL, PhGetString(entry->DevicePath))))
            {
                GX_ADAPTER_ATTRIBUTES adapterAttributes;

                entry->DevicePresent = TRUE;

                if (NT_SUCCESS(GraphicsQueryAdapterAttributes(
                    adapterHandle,
                    &adapterAttributes
                    )))
                {
                    entry->NpuDevice = !!adapterAttributes.TypeNpu;
                }

                if (GraphicsDeviceIsSoftwareDevice(adapterHandle))
                    entry->SoftwareDevice = TRUE;

                GraphicsCloseAdapterHandle(adapterHandle);
            }

            if (PhIsNullOrEmptyString(entry->DeviceName))
            {
                PhSetReference(&entry->DeviceName, entry->DevicePath);
            }

            if (entry->DevicePresent)
            {
                PPH_STRING locationString = NULL;

                if (SUCCEEDED(PhDevGetObjectProperties(
                    DevObjectTypeDevice,
                    device->pProperties[0].Buffer,
                    DevQueryFlagNone,
                    RTL_NUMBER_OF(requestedProperties),
                    requestedProperties,
                    &propertyCount,
                    &propertyList
                    )))
                {
                    PH_STRINGREF string;

                    string.Buffer = propertyList[0].Buffer;
                    string.Length = propertyList[0].BufferSize ? propertyList[0].BufferSize - sizeof(UNICODE_NULL) : 0;
                    entry->DeviceName = PhLoadIndirectString(&string);

                    if (propertyList[1].BufferSize)
                    {
                        SIZE_T returnLength;
                        PH_FORMAT format[2];
                        WCHAR formatBuffer[512];

                        PhInitFormatS(&format[0], entry->NpuDevice ? L"NPU " : L"GPU ");
                        PhInitFormatU(&format[1], *(PULONG)propertyList[2].Buffer);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), &returnLength))
                        {
                            locationString = PhCreateStringEx(formatBuffer, returnLength - sizeof(UNICODE_NULL));
                        }
                        else
                        {
                            locationString = PhFormat(format, RTL_NUMBER_OF(format), 0);
                        }
                    }

                    if (propertyList[3].BufferSize >= sizeof(LUID))
                        entry->AdapterLuid = *(PLUID)propertyList[3].Buffer;

                    if (locationString)
                    {
                        PhMoveReference(&entry->DeviceName, PhFormatString(
                            L"%s [%s]",
                            PhGetString(locationString),
                            PhGetString(entry->DeviceName)
                            ));
                        PhDereferenceObject(locationString);
                    }

                    PhDevFreeObjectProperties(propertyCount, propertyList);
                }
            }

            // Avoid duplicates, querying for GUID_DISPLAY_DEVICE_ARRIVAL and
            // GUID_COMPUTE_DEVICE_ARRIVAL together will return duplicates.
            BOOLEAN duplicate = FALSE;
            for (ULONG i = 0; i < deviceList->Count; i++)
            {
                PGPU_ENUM_ENTRY currentEntry = deviceList->Items[i];

                if (currentEntry->AdapterLuid.HighPart == entry->AdapterLuid.HighPart &&
                    currentEntry->AdapterLuid.LowPart == entry->AdapterLuid.LowPart)
                {
                    duplicate = TRUE;
                    break;
                }
            }

            if (duplicate)
            {
                PhClearReference(&entry->DeviceName);
                PhClearReference(&entry->DevicePath);
                PhFree(entry);
            }
            else
            {
                PhAddItemList(deviceList, entry);
            }
        }

        PhDevFreeObjects(deviceCount, deviceObjects);
    }
    else
    {
        return;
    }

    // Sort the entries
    qsort(deviceList->Items, deviceList->Count, sizeof(PVOID), GraphicsDeviceEntryCompareFunction);

    PhAcquireQueuedLockShared(&GraphicsDevicesListLock);
    for (ULONG i = 0; i < deviceList->Count; i++)
    {
        PGPU_ENUM_ENTRY entry = deviceList->Items[i];

        if (!entry)
            continue;

        if (Context->UseAlternateMethod)
        {
            AddGraphicsDeviceToListView(
                Context,
                entry->DevicePresent,
                entry->DevicePath,
                entry->DeviceName
                );
        }
        else
        {
            if (entry->DevicePresent && !entry->SoftwareDevice)
            {
                AddGraphicsDeviceToListView(
                    Context,
                    entry->DevicePresent,
                    entry->DevicePath,
                    entry->DeviceName
                    );
            }
        }

        if (entry->DeviceName)
            PhDereferenceObject(entry->DeviceName);
        // Note: DevicePath is disposed by WM_DESTROY.

        PhFree(entry);
    }
    PhReleaseQueuedLockShared(&GraphicsDevicesListLock);
    PhDereferenceObject(deviceList);

    // HACK: Show all unknown devices.
    PhAcquireQueuedLockShared(&GraphicsDevicesListLock);
    for (ULONG i = 0; i < GraphicsDevicesList->Count; i++)
    {
        INT index = INT_ERROR;
        BOOLEAN found = FALSE;
        PDV_GPU_ENTRY entry = PhReferenceObjectSafe(GraphicsDevicesList->Items[i]);

        if (!entry)
            continue;

        while ((index = PhFindListViewItemByFlags(
            Context->ListViewHandle,
            index,
            LVNI_ALL
            )) != INT_ERROR)
        {
            PDV_GPU_ID param;

            if (PhGetListViewItemParam(Context->ListViewHandle, index, &param))
            {
                if (EquivalentGraphicsDeviceId(param, &entry->Id))
                {
                    found = TRUE;
                }
            }
        }

        if (!found)
        {
            PPH_STRING description;

            if (description = GraphicsDeviceQueryInterfaceName(PhGetString(entry->Id.DevicePath)))
            {
                AddGraphicsDeviceToListView(
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
    PhReleaseQueuedLockShared(&GraphicsDevicesListLock);
}

PPH_STRING FindGraphicsDeviceInstance(
    _In_ PPH_STRING DevicePath
    )
{
    ULONG deviceCount = 0;
    PDEV_OBJECT deviceObjects = NULL;
    PPH_STRING deviceInstanceString = NULL;

    const DEVPROPCOMPKEY deviceProperties[] =
    {
        { DEVPKEY_Device_InstanceId, DEVPROP_STORE_SYSTEM, NULL },
    };

    const DEVPROP_FILTER_EXPRESSION deviceFilter[] =
    {
        {DEVPROP_OPERATOR_OR_OPEN, {0}},
        {DEVPROP_OPERATOR_EQUALS,{{DEVPKEY_DeviceInterface_ClassGuid, DEVPROP_STORE_SYSTEM, NULL}, DEVPROP_TYPE_GUID, sizeof(GUID), (PVOID)&GUID_DISPLAY_DEVICE_ARRIVAL}},
        {DEVPROP_OPERATOR_EQUALS,{{DEVPKEY_DeviceInterface_ClassGuid, DEVPROP_STORE_SYSTEM, NULL}, DEVPROP_TYPE_GUID, sizeof(GUID), (PVOID)&GUID_COMPUTE_DEVICE_ARRIVAL}},
        {DEVPROP_OPERATOR_OR_CLOSE, {0}}
    };

    if (SUCCEEDED(PhDevGetObjects(
        DevObjectTypeDeviceInterface,
        DevQueryFlagNone,
        RTL_NUMBER_OF(deviceProperties),
        deviceProperties,
        RTL_NUMBER_OF(deviceFilter),
        deviceFilter,
        &deviceCount,
        &deviceObjects
        )))
    {
        for (ULONG i = 0; i < deviceCount; i++)
        {
            PDEV_OBJECT device = &deviceObjects[i];

            if (PhEqualStringZ(device->pszObjectId, PhGetString(DevicePath), TRUE))
            {
                PH_STRINGREF string;

                string.Buffer = device->pProperties[0].Buffer;
                string.Length = device->pProperties[0].BufferSize ? device->pProperties[0].BufferSize - sizeof(UNICODE_NULL) : 0;
                deviceInstanceString = PhCreateString2(&string);
                break;
            }
        }

        PhDevFreeObjects(deviceCount, deviceObjects);
    }
    else
    {
        PWSTR deviceInterfaceList;
        ULONG deviceInterfaceListLength = 0;
        PWSTR deviceInterface;

        if (CM_Get_Device_Interface_List_Size(
            &deviceInterfaceListLength,
            (PGUID)&GUID_DISPLAY_DEVICE_ARRIVAL,
            NULL,
            CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES
            ) != CR_SUCCESS)
        {
            return NULL;
        }

        deviceInterfaceList = PhAllocate(deviceInterfaceListLength * sizeof(WCHAR));
        memset(deviceInterfaceList, 0, deviceInterfaceListLength * sizeof(WCHAR));

        if (CM_Get_Device_Interface_List(
            (PGUID)&GUID_DISPLAY_DEVICE_ARRIVAL,
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

            if (PhEqualStringZ(deviceInterface, DevicePath->Buffer, TRUE))
            {
                deviceInstanceString = PhCreateString(deviceInstanceId);
                break;
            }
        }

        PhFree(deviceInterfaceList);
    }

    return deviceInstanceString;
}

VOID LoadGraphicsDeviceImages(
    _In_ PDV_GPU_OPTIONS_CONTEXT Context
    )
{
    HICON largeIcon;
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

    dpiValue = PhGetWindowDpi(Context->ListViewHandle);

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

INT_PTR CALLBACK GraphicsDeviceOptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_GPU_OPTIONS_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(DV_GPU_OPTIONS_CONTEXT));
        PhSetDialogContext(hwndDlg, context);
    }
    else
    {
        context = PhGetDialogContext(hwndDlg);
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_GPUDEVICE_LISTVIEW);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            ListView_SetExtendedListViewStyleEx(context->ListViewHandle, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 350, L"Graphics Devices");
            PhSetExtendedListView(context->ListViewHandle);
            LoadGraphicsDeviceImages(context);

            ListView_EnableGroupView(context->ListViewHandle, TRUE);
            PhAddListViewGroup(context->ListViewHandle, 0, L"Connected");
            PhAddListViewGroup(context->ListViewHandle, 1, L"Disconnected");

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_SHOW_HIDDEN_DEVICES), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);

            ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);
            FindGraphicsDevices(context);
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
                GraphicsDevicesSaveList();

            FreeListViewGraphicsDeviceEntries(context);
        }
        break;
    case WM_NCDESTROY:
        {
            PhRemoveDialogContext(hwndDlg);
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
                    FreeListViewGraphicsDeviceEntries(context);
                    ListView_DeleteAllItems(context->ListViewHandle);
                    FindGraphicsDevices(context);
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

                if (!PhTryAcquireReleaseQueuedLockExclusive(&GraphicsDevicesListLock))
                    break;

                if (FlagOn(listView->uChanged, LVIF_STATE))
                {
                    switch (FlagOn(listView->uNewState, LVIS_STATEIMAGEMASK))
                    {
                    case INDEXTOSTATEIMAGEMASK(2): // checked
                        {
                            PDV_GPU_ID param = (PDV_GPU_ID)listView->lParam;

                            if (!FindGraphicsDeviceEntry(param, FALSE))
                            {
                                PDV_GPU_ENTRY entry;

                                entry = CreateGraphicsDeviceEntry(param);
                                entry->UserReference = TRUE;
                            }

                            context->OptionsChanged = TRUE;
                        }
                        break;
                    case INDEXTOSTATEIMAGEMASK(1): // unchecked
                        {
                            PDV_GPU_ID param = (PDV_GPU_ID)listView->lParam;

                            FindGraphicsDeviceEntry(param, TRUE);

                            context->OptionsChanged = TRUE;
                        }
                        break;
                    }
                }
            }
            else if (header->code == NM_RCLICK)
            {
                PDV_GPU_ID param;
                PPH_STRING deviceInstance;

                if (param = PhGetSelectedListViewItemParam(context->ListViewHandle))
                {
                    if (deviceInstance = FindGraphicsDeviceInstance(param->DevicePath))
                    {
                        ShowDeviceMenu(hwndDlg, deviceInstance);
                        PhDereferenceObject(deviceInstance);

                        ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);
                        FreeListViewGraphicsDeviceEntries(context);
                        ListView_DeleteAllItems(context->ListViewHandle);
                        FindGraphicsDevices(context);
                        ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);
                    }
                }
            }
            else if (header->code == NM_DBLCLK)
            {
                PDV_GPU_ID param;
                PPH_STRING deviceInstance;

                if (param = PhGetSelectedListViewItemParam(context->ListViewHandle))
                {
                    if (deviceInstance = FindGraphicsDeviceInstance(param->DevicePath))
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
