/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2021-2026
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
    DV_RAPL_ID Id;
    PPH_STRING DeviceName;
} RAPL_ENUM_ENTRY, *PRAPL_ENUM_ENTRY;

#define RAPL_SETTING_DEVICEPATH_PREFIX L"path:"
#define RAPL_SETTING_DEVICEINSTANCE_PREFIX L"instance:"

/**
 * Converts a stored RAPL interface path into the DevQuery object ID form.
 *
 * \param DeviceInterface Stored device interface path.
 * \return A newly allocated normalized path string.
 */
PPH_STRING NormalizeRaplDeviceInterfacePath(
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

BOOLEAN RaplSettingPartStartsWithStringRef(
    _In_ PPH_STRINGREF Part,
    _In_ PPH_STRINGREF Prefix
    )
{
    return Part->Length >= Prefix->Length && PhStartsWithStringRef(Part, Prefix, TRUE);
}

VOID InitializeRaplDeviceIdFromSettingPart(
    _Out_ PDV_RAPL_ID Id,
    _In_ PPH_STRINGREF Part
    )
{
    static PH_STRINGREF pathPrefix = PH_STRINGREF_INIT(RAPL_SETTING_DEVICEPATH_PREFIX);
    static PH_STRINGREF instancePrefix = PH_STRINGREF_INIT(RAPL_SETTING_DEVICEINSTANCE_PREFIX);
    PH_STRINGREF value;

    value = *Part;

    if (RaplSettingPartStartsWithStringRef(Part, &instancePrefix))
    {
        value.Buffer += instancePrefix.Length / sizeof(WCHAR);
        value.Length -= instancePrefix.Length;
        InitializeRaplDeviceId(Id, NULL, PhCreateString2(&value));
    }
    else
    {
        if (RaplSettingPartStartsWithStringRef(Part, &pathPrefix))
        {
            value.Buffer += pathPrefix.Length / sizeof(WCHAR);
            value.Length -= pathPrefix.Length;
        }

        // Convert legacy settings paths for compatibility. (dmex)
        if (value.Length > sizeof(UNICODE_NULL) && value.Buffer[1] == OBJ_NAME_PATH_SEPARATOR)
            value.Buffer[1] = L'?';

        InitializeRaplDeviceId(Id, PhCreateString2(&value), NULL);
    }
}

VOID AppendRaplDeviceIdToSettingString(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ PDV_RAPL_ID Id
    )
{
    if (Id->DeviceInstance)
    {
        PhAppendStringBuilder2(StringBuilder, RAPL_SETTING_DEVICEINSTANCE_PREFIX);
        PhAppendStringBuilder(StringBuilder, &Id->DeviceInstance->sr);
    }
    else if (Id->DevicePath)
    {
        PhAppendStringBuilder2(StringBuilder, RAPL_SETTING_DEVICEPATH_PREFIX);
        PhAppendStringBuilder(StringBuilder, &Id->DevicePath->sr);
    }
}

PRAPL_ENUM_ENTRY FindRaplEnumEntry(
    _In_ PPH_LIST DeviceList,
    _In_ PDV_RAPL_ID Id
    )
{
    for (ULONG i = 0; i < DeviceList->Count; i++)
    {
        PRAPL_ENUM_ENTRY entry = DeviceList->Items[i];

        if (EquivalentRaplDeviceId(&entry->Id, Id))
            return entry;
    }

    return NULL;
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

            InitializeRaplDeviceIdFromSettingPart(&id, &part);
            entry = CreateRaplDeviceEntry(&id);

            if (entry->Id.DevicePath && !entry->Id.DeviceInstance)
            {
                DV_RAPL_ID resolvedId;
                PPH_STRING deviceDescription;

                memset(&resolvedId, 0, sizeof(resolvedId));
                deviceDescription = NULL;

                if (QueryRaplDeviceInterfaceInfo(PhGetString(entry->Id.DevicePath), &resolvedId, &deviceDescription))
                {
                    if (resolvedId.DeviceInstance)
                        PhSetReference(&entry->Id.DeviceInstance, resolvedId.DeviceInstance);
                    if (!entry->DeviceName && deviceDescription)
                        PhSetReference(&entry->DeviceName, deviceDescription);
                }

                DeleteRaplDeviceId(&resolvedId);
                PhClearReference(&deviceDescription);
            }

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
            AppendRaplDeviceIdToSettingString(&stringBuilder, &entry->Id);

            if (stringBuilder.String->Length != 0)
                PhAppendCharStringBuilder(&stringBuilder, L',');
        }

        PhDereferenceObjectDeferDelete(entry);
    }

    PhReleaseQueuedLockShared(&RaplDevicesListLock);

    if (stringBuilder.String->Length != 0 && stringBuilder.String->Buffer[(stringBuilder.String->Length / sizeof(WCHAR)) - 1] == L',')
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

    InitializeRaplDeviceId(&deviceId, DevicePath, NULL);

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
PPH_STRING CreateRaplStringFromProperty(
    _In_ const DEVPROPERTY* Property
    )
{
    SIZE_T stringLength;

    if (!Property || Property->Type != DEVPROP_TYPE_STRING || !Property->Buffer || Property->BufferSize < sizeof(UNICODE_NULL))
        return NULL;

    stringLength = Property->BufferSize;

    if (((PWSTR)Property->Buffer)[(Property->BufferSize / sizeof(WCHAR)) - 1] == UNICODE_NULL)
        stringLength -= sizeof(UNICODE_NULL);

    return PhCreateStringEx((PWSTR)Property->Buffer, stringLength);
}

PPH_STRING QueryRaplDeviceDescriptionFromProperties(
    _In_ ULONG DevicePropertyCount,
    _In_reads_(DevicePropertyCount) const DEVPROPERTY* DevicePropertiesList
    )
{
    DEVPROPCOMPKEY deviceProperties[3];
    const DEVPROPERTY* property;

    deviceProperties[0].Key = DEVPKEY_Device_FriendlyName;
    deviceProperties[0].Store = DEVPROP_STORE_SYSTEM;
    deviceProperties[1].Key = DEVPKEY_NAME;
    deviceProperties[1].Store = DEVPROP_STORE_SYSTEM;
    deviceProperties[2].Key = DEVPKEY_Device_DeviceDesc;
    deviceProperties[2].Store = DEVPROP_STORE_SYSTEM;

    for (ULONG i = 0; i < RTL_NUMBER_OF(deviceProperties); i++)
    {
         if (property = PhDevFindProperty(
             &deviceProperties[i].Key,
             deviceProperties[i].Store,
             DevicePropertyCount,
             DevicePropertiesList))
         {
             return CreateRaplStringFromProperty(property);
         }
    }

    return NULL;
}

_Success_(return)
BOOLEAN QueryRaplDeviceInterfaceInfo(
    _In_ PCWSTR DeviceInterface,
    _Out_opt_ PDV_RAPL_ID DeviceId,
    _Out_ PPH_STRING* DeviceDescription
    )
{
    ULONG deviceInterfacePropertyCount = 0;
    ULONG devicePropertyCount = 0;
    ULONG parentPropertyCount = 0;
    const DEVPROPERTY* deviceInterfaceProperties = NULL;
    const DEVPROPERTY* devicePropertiesList = NULL;
    const DEVPROPERTY* parentPropertiesList = NULL;
    const DEVPROPERTY* instanceIdProperty;
    const DEVPROPERTY* parentIdProperty;
    PPH_STRING normalizedDeviceInterface;
    PPH_STRING interfacePath = NULL;
    PPH_STRING aggregateDeviceInstance = NULL;
    DEVPROPCOMPKEY requestedProperties[] =
    {
        { DEVPKEY_Device_InstanceId, DEVPROP_STORE_SYSTEM, NULL },
    };
    DEVPROPCOMPKEY deviceProperties[] =
    {
        { DEVPKEY_Device_Parent, DEVPROP_STORE_SYSTEM, NULL },
        { DEVPKEY_Device_FriendlyName, DEVPROP_STORE_SYSTEM, NULL },
        { DEVPKEY_NAME, DEVPROP_STORE_SYSTEM, NULL },
        { DEVPKEY_Device_DeviceDesc, DEVPROP_STORE_SYSTEM, NULL },
    };
    DEVPROPCOMPKEY parentProperties[] =
    {
        { DEVPKEY_Device_FriendlyName, DEVPROP_STORE_SYSTEM, NULL },
        { DEVPKEY_NAME, DEVPROP_STORE_SYSTEM, NULL },
        { DEVPKEY_Device_DeviceDesc, DEVPROP_STORE_SYSTEM, NULL },
    };

    if (DeviceId)
    {
        memset(DeviceId, 0, sizeof(*DeviceId));
    }

    *DeviceDescription = NULL;
    normalizedDeviceInterface = NormalizeRaplDeviceInterfacePath(DeviceInterface);

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

    if (!instanceIdProperty || !(interfacePath = NormalizeRaplDeviceInterfacePath(DeviceInterface)))
        goto CleanupExit;

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
        goto CleanupExit;
    }

    parentIdProperty = PhDevFindProperty(
        &DEVPKEY_Device_Parent,
        DEVPROP_STORE_SYSTEM,
        devicePropertyCount,
        devicePropertiesList
        );

    aggregateDeviceInstance = CreateRaplStringFromProperty(parentIdProperty);

    if (!aggregateDeviceInstance)
    {
        aggregateDeviceInstance = CreateRaplStringFromProperty(instanceIdProperty);
    }

    if (aggregateDeviceInstance && HR_SUCCESS(PhDevGetObjectProperties(
        DevObjectTypeDevice,
        PhGetString(aggregateDeviceInstance),
        DevQueryFlagNone,
        RTL_NUMBER_OF(parentProperties),
        parentProperties,
        &parentPropertyCount,
        &parentPropertiesList
        )))
    {
        *DeviceDescription = QueryRaplDeviceDescriptionFromProperties(parentPropertyCount, parentPropertiesList);
    }

    if (!*DeviceDescription)
    {
        *DeviceDescription = QueryRaplDeviceDescriptionFromProperties(devicePropertyCount, devicePropertiesList);
    }

    if (!*DeviceDescription && aggregateDeviceInstance)
    {
        *DeviceDescription = PhCreateString2(&aggregateDeviceInstance->sr);
    }

    if (!*DeviceDescription && interfacePath)
    {
        *DeviceDescription = PhCreateString2(&interfacePath->sr);
    }

    if (DeviceId)
    {
        DeviceId->DevicePath = interfacePath;
        DeviceId->DeviceInstance = aggregateDeviceInstance;
        interfacePath = NULL;
        aggregateDeviceInstance = NULL;
    }

CleanupExit:
    PhClearReference(&interfacePath);
    PhClearReference(&aggregateDeviceInstance);

    PhDevFreeObjectProperties(parentPropertyCount, parentPropertiesList);
    PhDevFreeObjectProperties(devicePropertyCount, devicePropertiesList);
    PhDevFreeObjectProperties(deviceInterfacePropertyCount, deviceInterfaceProperties);

    return DeviceId ? !!DeviceId->DevicePath : !!*DeviceDescription;
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
        DV_RAPL_ID deviceId;
        PWSTR deviceInterface;

        deviceInterface = (PWSTR)deviceObjects[i].pszObjectId;

        memset(&deviceId, 0, sizeof(deviceId));

        if (!QueryRaplDeviceInterfaceInfo(deviceInterface, &deviceId, &deviceDescription))
            continue;

        deviceEntry = FindRaplEnumEntry(deviceList, &deviceId);

        if (!deviceEntry)
        {
            deviceEntry = PhAllocateZero(sizeof(RAPL_ENUM_ENTRY));
            deviceEntry->DeviceIndex = ++deviceIndex;
            deviceEntry->DeviceName = PhCreateString2(&deviceDescription->sr);
            CopyRaplDeviceId(&deviceEntry->Id, &deviceId);
            PhAddItemList(deviceList, deviceEntry);
        }
        else if (deviceDescription && !deviceEntry->DeviceName)
        {
            deviceEntry->DeviceName = PhCreateString2(&deviceDescription->sr);
        }

        if (NT_SUCCESS(PhCreateFile(
            &deviceHandle,
            &deviceId.DevicePath->sr,
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
                ULONG returnLength;

                memset(&metadataSize, 0, sizeof(EMI_METADATA_SIZE));

                if (NT_SUCCESS(PhDeviceIoControlFile(
                    deviceHandle,
                    IOCTL_EMI_GET_METADATA_SIZE,
                    NULL,
                    0,
                    &metadataSize,
                    sizeof(EMI_METADATA_SIZE),
                    NULL
                    )) &&
                    metadataSize.MetadataSize >= (ULONG)FIELD_OFFSET(EMI_METADATA_V2, Channels) &&
                    metadataSize.MetadataSize <= EV_EMI_MAX_METADATA_SIZE) // (dmex)
                {
                    metadata = PhAllocate(metadataSize.MetadataSize);
                    memset(metadata, 0, metadataSize.MetadataSize);

                    returnLength = 0;

                    if (NT_SUCCESS(PhDeviceIoControlFile(
                        deviceHandle,
                        IOCTL_EMI_GET_METADATA,
                        NULL,
                        0,
                        metadata,
                        metadataSize.MetadataSize,
                        &returnLength
                        )) &&
                        returnLength >= (ULONG)FIELD_OFFSET(EMI_METADATA_V2, Channels) &&
                        returnLength <= metadataSize.MetadataSize)
                    {
                        EMI_CHANNEL_V2* channels = metadata->Channels;
                        PVOID end = PTR_ADD_OFFSET(metadata, returnLength);

                        // Only power counters are relevant for RAPL-style reporting.
                        if (RaplChannelFitsMetadata(channels, end) &&
                            channels->MeasurementUnit == EmiMeasurementUnitPicowattHours)
                        {
                            BOOLEAN hasPackageChannel = FALSE;
                            BOOLEAN hasCoreChannel = FALSE;

                            // Map the exported EMI channels into the logical RAPL domains we expose.
                            for (ULONG i = 0; i < metadata->ChannelCount; i++)
                            {
                                // Stop at the first record that runs past the
                                // returned bytes. (dmex)
                                if (!RaplChannelFitsMetadata(channels, end))
                                    break;

                                if (RaplChannelNameIsPackage(channels->ChannelName))
                                    hasPackageChannel = TRUE;
                                if (RaplChannelNameIsCore(channels->ChannelName))
                                    hasCoreChannel = TRUE;

                                channels = EMI_CHANNEL_V2_NEXT_CHANNEL(channels);
                            }

                            // A usable device must expose package power and at least one core domain.
                            if (hasPackageChannel && hasCoreChannel)
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

        PhDereferenceObject(deviceDescription);
        DeleteRaplDeviceId(&deviceId);
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
            entry->Id.DevicePath,
            entry->DeviceName
            );

        if (entry->DeviceName)
            PhDereferenceObject(entry->DeviceName);
        DeleteRaplDeviceId(&entry->Id);

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

    if (HR_SUCCESS(PhDevGetObjectProperties(
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
        }

        DestroyIcon(largeIcon);
    }

    PhDereferenceObject(deviceIconPath);
}

/**
 * Handles the RAPL devices options dialog.
 *
 * \param WindowHandle Dialog window handle.
 * \param WindowMessage Window message identifier.
 * \param wParam Message-specific wParam value.
 * \param lParam Message-specific lParam value.
 * \return Dialog procedure result.
 */
INT_PTR CALLBACK RaplDeviceOptionsDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_RAPL_OPTIONS_CONTEXT context = NULL;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(DV_RAPL_OPTIONS_CONTEXT));
        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (context == NULL)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            context->ListViewHandle = GetDlgItem(WindowHandle, IDC_RAPLDEVICE_LISTVIEW);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            ListView_SetExtendedListViewStyleEx(context->ListViewHandle, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 350, L"RAPL Drives");
            PhSetExtendedListView(context->ListViewHandle);
            LoadRaplDeviceImages(context);

            ListView_EnableGroupView(context->ListViewHandle, TRUE);
            PhAddListViewGroup(context->ListViewHandle, 0, L"Connected");
            PhAddListViewGroup(context->ListViewHandle, 1, L"Disconnected");

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            FindRaplDevices(context);

            if (ListView_GetItemCount(context->ListViewHandle) == 0)
            {
                PhSetWindowStyle(context->ListViewHandle, WS_BORDER, WS_BORDER);
                PhSetWindowFrameChanged(context->ListViewHandle);
            }

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
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
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
                    if (param->DeviceInstance)
                        deviceInstance = PhReferenceObject(param->DeviceInstance);
                    else
                        deviceInstance = FindRaplDeviceInstance(param->DevicePath);

                    if (deviceInstance)
                    {
                        ShowDeviceMenu(WindowHandle, deviceInstance);
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
                    if (param->DeviceInstance)
                        deviceInstance = PhReferenceObject(param->DeviceInstance);
                    else
                        deviceInstance = FindRaplDeviceInstance(param->DevicePath);

                    if (deviceInstance)
                    {
                        HardwareDeviceShowProperties(WindowHandle, deviceInstance);
                        PhDereferenceObject(deviceInstance);
                    }
                }
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}
