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

/**
 * Checks whether an EMI channel name belongs to a RAPL package channel.
 *
 * \param ChannelName EMI channel name.
 * \return TRUE if the channel name starts with the RAPL package prefix.
 */
static BOOLEAN RaplChannelNameStartsWithPackage(
    _In_ PCWSTR ChannelName
    )
{
    PH_STRINGREF channelName;

    PhInitializeStringRefLongHint(&channelName, ChannelName);

    return PhStartsWithStringRef2(&channelName, L"RAPL_Package", TRUE);
}

/**
 * Matches a RAPL package channel name against a specific suffix.
 *
 * \param ChannelName EMI channel name.
 * \param Suffix Expected channel suffix.
 * \return TRUE if the channel is a RAPL package channel with the requested suffix.
 */
static BOOLEAN RaplChannelNameMatchesSuffix(
    _In_ PCWSTR ChannelName,
    _In_ PCWSTR Suffix
    )
{
    PH_STRINGREF channelName;

    if (!RaplChannelNameStartsWithPackage(ChannelName))
        return FALSE;

    PhInitializeStringRefLongHint(&channelName, ChannelName);

    return PhEndsWithStringRef2(&channelName, Suffix, TRUE);
}

/**
 * Determines whether a channel represents package power.
 *
 * \param ChannelName EMI channel name.
 * \return TRUE if the channel is a package power channel.
 */
static BOOLEAN RaplChannelNameIsPackage(
    _In_ PCWSTR ChannelName
    )
{
    return RaplChannelNameMatchesSuffix(ChannelName, L"_PKG");
}

/**
 * Determines whether a channel represents a core-power domain.
 *
 * This accepts both Intel-style `PP0` names and AMD-style per-core `_CORE`
 * names during device enumeration.
 *
 * \param ChannelName EMI channel name.
 * \return TRUE if the channel represents core power.
 */
static BOOLEAN RaplChannelNameIsCore(
    _In_ PCWSTR ChannelName
    )
{
    return RaplChannelNameMatchesSuffix(ChannelName, L"_PP0") ||
        RaplChannelNameMatchesSuffix(ChannelName, L"_CORE");
}

/**
 * Determines whether a channel represents the discrete GPU domain.
 *
 * \param ChannelName EMI channel name.
 * \return TRUE if the channel is the PP1 discrete GPU channel.
 */
static BOOLEAN RaplChannelNameIsDiscreteGpu(
    _In_ PCWSTR ChannelName
    )
{
    return RaplChannelNameMatchesSuffix(ChannelName, L"_PP1");
}

/**
 * Determines whether a channel represents the DRAM domain.
 *
 * \param ChannelName EMI channel name.
 * \return TRUE if the channel is the DRAM channel.
 */
static BOOLEAN RaplChannelNameIsDram(
    _In_ PCWSTR ChannelName
    )
{
    return RaplChannelNameMatchesSuffix(ChannelName, L"_DRAM");
}

/**
 * Converts a stored RAPL interface path into the DevQuery object ID form.
 *
 * \param DeviceInterface Stored device interface path.
 * \return A newly allocated normalized path string.
 */
static PPH_STRING NormalizeRaplDeviceInterfacePath(
    _In_ PCWSTR DeviceInterface
    )
{
    PPH_STRING normalizedPath;

    normalizedPath = PhCreateString(DeviceInterface);

    if (normalizedPath->Length >= 2 * sizeof(WCHAR) && normalizedPath->Buffer[1] == L'?')
        normalizedPath->Buffer[1] = OBJ_NAME_PATH_SEPARATOR;

    return normalizedPath;
}

/**
 * Compares two enumerated RAPL entries by discovery order.
 *
 * \param elem1 First sort element.
 * \param elem2 Second sort element.
 * \return Comparison result suitable for qsort.
 */
static int __cdecl RaplDeviceEntryCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PRAPL_ENUM_ENTRY entry1 = *(PRAPL_ENUM_ENTRY*)elem1;
    PRAPL_ENUM_ENTRY entry2 = *(PRAPL_ENUM_ENTRY*)elem2;

    return uint64cmp(entry1->DeviceIndex, entry2->DeviceIndex);
}

/**
 * Loads the persisted RAPL device selection from settings.
 */
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

/**
 * Saves the current user-selected RAPL device list to settings.
 */
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
        PDV_RAPL_ENTRY entry = PhReferenceObjectUnsafe(RaplDevicesList->Items[i]);

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

    settingsString = PhFinalStringBuilderString(&stringBuilder);
    PhSetStringSetting2(SETTING_NAME_RAPL_LIST, &settingsString->sr);
    PhDereferenceObject(settingsString);
}

/**
 * Finds a tracked RAPL device entry by identifier.
 *
 * \param Id Device identifier to search for.
 * \param RemoveUserReference Removes the user-reference flag when TRUE.
 * \return TRUE if the device entry was found.
 */
BOOLEAN FindRaplDeviceEntry(
    _In_ PDV_RAPL_ID Id,
    _In_ BOOLEAN RemoveUserReference
    )
{
    BOOLEAN found = FALSE;

    PhAcquireQueuedLockShared(&RaplDevicesListLock);

    for (ULONG i = 0; i < RaplDevicesList->Count; i++)
    {
        PDV_RAPL_ENTRY currentEntry = PhReferenceObjectUnsafe(RaplDevicesList->Items[i]);

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

/**
 * Adds a RAPL device row to the options list view.
 *
 * \param Context Options dialog context.
 * \param DevicePresent TRUE if the device is currently connected.
 * \param DevicePath Device interface path.
 * \param DeviceName Display name for the row.
 */
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
        PDV_RAPL_ENTRY entry = PhReferenceObjectUnsafe(RaplDevicesList->Items[i]);

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

/**
 * Releases all RAPL device identifiers stored in list view item parameters.
 *
 * \param Context Options dialog context.
 */
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

/**
 * Resolves a RAPL device interface to a user-visible device description.
 *
 * \param DeviceInterface Device interface path.
 * \param DeviceDescription Receives the allocated device description on success.
 * \return TRUE if a description was resolved.
 */
_Success_(return)
BOOLEAN QueryRaplDeviceInterfaceDescription(
    _In_ PCWSTR DeviceInterface,
    _Out_ PPH_STRING* DeviceDescription
    )
{
    ULONG deviceInterfacePropertyCount = 0;
    ULONG devicePropertyCount = 0;
    const DEVPROPERTY* deviceInterfaceProperties = NULL;
    const DEVPROPERTY* devicePropertiesList = NULL;
    const DEVPROPERTY* instanceIdProperty;
    PPH_STRING normalizedDeviceInterface;
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

    // Convert the stored Win32-style interface path into the object ID form used by DevQuery.
    normalizedDeviceInterface = NormalizeRaplDeviceInterfacePath(DeviceInterface);

    // Query the interface object just far enough to recover the stable device instance ID.
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

    // Use the instance ID to query the backing device and read its display properties.
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

    // Prefer the most user-friendly property and fall back until one is populated.
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

            // Keep the stored string length aligned with the visible character count.
            if (((PWSTR)property->Buffer)[(property->BufferSize / sizeof(WCHAR)) - 1] == UNICODE_NULL)
                deviceDescriptionLength -= sizeof(UNICODE_NULL);

            *DeviceDescription = PhCreateStringEx((PWSTR)property->Buffer, deviceDescriptionLength);
            break;
        }
    }

    PhDevFreeObjectProperties(devicePropertyCount, devicePropertiesList);
    PhDevFreeObjectProperties(deviceInterfacePropertyCount, deviceInterfaceProperties);

    return *DeviceDescription != NULL;
}

/**
 * Enumerates available RAPL devices and populates the options list view.
 *
 * \param Context Options dialog context.
 */
VOID FindRaplDevices(
    _In_ PDV_RAPL_OPTIONS_CONTEXT Context
    )
{
    ULONG deviceIndex = 0;
    ULONG deviceCount = 0;
    PPH_LIST deviceList;
    const DEV_OBJECT* deviceObjects = NULL;
    const DEVPROPCOMPKEY requestedProperties[] =
    {
        { DEVPKEY_Device_InstanceId, DEVPROP_STORE_SYSTEM, NULL },
    };
    const DEVPROP_FILTER_EXPRESSION deviceFilter[] =
    {
        { DEVPROP_OPERATOR_EQUALS, {{ DEVPKEY_DeviceInterface_ClassGuid, DEVPROP_STORE_SYSTEM, NULL }, DEVPROP_TYPE_GUID, sizeof(GUID), (PVOID)&GUID_DEVICE_ENERGY_METER } }
    };

    deviceList = PH_AUTO(PhCreateList(1));

    // Enumerate every energy-meter interface exposed through DevQuery.
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

    for (ULONG i = 0; i < deviceCount; i++)
    {
        PPH_STRING deviceDescription;
        HANDLE deviceHandle;
        PRAPL_ENUM_ENTRY deviceEntry;
        PWSTR deviceInterface;

        deviceInterface = (PWSTR)deviceObjects[i].pszObjectId;

        if (!QueryRaplDeviceInterfaceDescription(deviceInterface, &deviceDescription))
            continue;

        // Convert the object ID form back to the runtime Win32 path form once up front.
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

            // Only EMI v2 devices expose the metadata needed for RAPL channel discovery.
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

                        // Only power counters are relevant for RAPL-style reporting.
                        if (channels->MeasurementUnit == EmiMeasurementUnitPicowattHours)
                        {
                            BOOLEAN hasCoreChannel = FALSE;

                            // Map the exported EMI channels into the logical RAPL domains we expose.
                            for (ULONG i = 0; i < metadata->ChannelCount; i++)
                            {
                                if (RaplChannelNameIsPackage(channels->ChannelName))
                                    deviceEntry->ChannelIndex[EV_EMI_DEVICE_INDEX_PACKAGE] = i;
                                if (RaplChannelNameIsCore(channels->ChannelName))
                                {
                                    deviceEntry->ChannelIndex[EV_EMI_DEVICE_INDEX_CORE] = i;
                                    hasCoreChannel = TRUE;
                                }
                                if (RaplChannelNameIsDiscreteGpu(channels->ChannelName))
                                    deviceEntry->ChannelIndex[EV_EMI_DEVICE_INDEX_GPUDISCRETE] = i;
                                if (RaplChannelNameIsDram(channels->ChannelName))
                                    deviceEntry->ChannelIndex[EV_EMI_DEVICE_INDEX_DIMM] = i;

                                channels = EMI_CHANNEL_V2_NEXT_CHANNEL(channels);
                            }

                            // A usable device must expose package power and at least one core domain.
                            if (deviceEntry->ChannelIndex[EV_EMI_DEVICE_INDEX_PACKAGE] != ULONG_MAX && hasCoreChannel)
                            {
                                deviceEntry->DeviceSupported = TRUE;
                            }
                        }
                    }

                    PhFree(metadata);
                }
            }

            deviceEntry->DevicePresent = TRUE;
            NtClose(deviceHandle);
        }

        PhAddItemList(deviceList, deviceEntry);

        PhDereferenceObject(deviceDescription);
    }

    PhDevFreeObjects(deviceCount, deviceObjects);

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
        PDV_RAPL_ENTRY entry = PhReferenceObjectUnsafe(RaplDevicesList->Items[i]);

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

/**
 * Resolves a stored device path back to its device instance identifier.
 *
 * \param DevicePath Stored device interface path.
 * \return The matching device instance identifier, or NULL if none is found.
 */
PPH_STRING FindRaplDeviceInstance(
    _In_ PPH_STRING DevicePath
    )
{
    PPH_STRING deviceInstanceString = NULL;
    ULONG propertyCount = 0;
    const DEVPROPERTY* properties = NULL;
    const DEVPROPERTY* instanceIdProperty;
    PPH_STRING normalizedDevicePath;
    DEVPROPCOMPKEY requestedProperty =
    {
        DEVPKEY_Device_InstanceId,
        DEVPROP_STORE_SYSTEM,
        NULL
    };

    normalizedDevicePath = NormalizeRaplDeviceInterfacePath(PhGetString(DevicePath));

    if (SUCCEEDED(PhDevGetObjectProperties(
        DevObjectTypeDeviceInterface,
        PhGetString(normalizedDevicePath),
        DevQueryFlagNone,
        1,
        &requestedProperty,
        &propertyCount,
        &properties
        )))
    {
        instanceIdProperty = PhDevFindProperty(
            &DEVPKEY_Device_InstanceId,
            DEVPROP_STORE_SYSTEM,
            propertyCount,
            properties
            );

        if (instanceIdProperty && instanceIdProperty->Type == DEVPROP_TYPE_STRING && instanceIdProperty->Buffer && instanceIdProperty->BufferSize >= sizeof(UNICODE_NULL))
        {
            SIZE_T deviceInstanceLength = instanceIdProperty->BufferSize;

            if (((PWSTR)instanceIdProperty->Buffer)[(instanceIdProperty->BufferSize / sizeof(WCHAR)) - 1] == UNICODE_NULL)
                deviceInstanceLength -= sizeof(UNICODE_NULL);

            deviceInstanceString = PhCreateStringEx((PWSTR)instanceIdProperty->Buffer, deviceInstanceLength);
        }

        PhDevFreeObjectProperties(propertyCount, properties);
    }

    PhDereferenceObject(normalizedDevicePath);

    return deviceInstanceString;
}

/**
 * Loads the icon used by the RAPL options list view.
 *
 * \param Context Options dialog context.
 */
VOID LoadRaplDeviceImages(
    _In_ PDV_RAPL_OPTIONS_CONTEXT Context
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
    classGuidString = PhFormatGuid(&GUID_DEVCLASS_PROCESSOR);

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
            PhScaleToDisplay(24, dpiValue), // PhGetSystemMetrics(SM_CXSMICON)
            PhScaleToDisplay(24, dpiValue), // PhGetSystemMetrics(SM_CYSMICON)
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

/**
 * Handles the RAPL devices options dialog.
 *
 * \param hwndDlg Dialog window handle.
 * \param uMsg Window message identifier.
 * \param wParam Message-specific wParam value.
 * \param lParam Message-specific lParam value.
 * \return Dialog procedure result.
 */
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

                if (FlagOn(listView->uChanged, LVIF_STATE))
                {
                    switch (FlagOn(listView->uNewState, LVIS_STATEIMAGEMASK))
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
