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

/**
 * Checks whether an EMI channel name belongs to a RAPL package channel.
 *
 * Intel and AMD both expose RAPL counters under the `RAPL_Package*` prefix,
 * but the domain suffixes differ by platform.
 *
 * \param ChannelName EMI channel name.
 * \return TRUE if the channel name starts with the RAPL package prefix.
 */
BOOLEAN RaplChannelNameStartsWithPackage(
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
 * The suffix-based matching lets the parser accept both Intel-style package
 * domains such as `RAPL_Package0_PKG`, `RAPL_Package0_PP0`, `RAPL_Package0_PP1`
 * and `RAPL_Package0_DRAM`, and AMD-style per-core names such as
 * `RAPL_Package0_Core0_CORE`.
 *
 * \param ChannelName EMI channel name.
 * \param Suffix Expected channel suffix.
 * \return TRUE if the channel is a RAPL package channel with the requested suffix.
 */
BOOLEAN RaplChannelNameMatchesSuffix(
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
 * This matches package-level channels such as `RAPL_Package0_PKG` on both
 * Intel and AMD systems.
 *
 * \param ChannelName EMI channel name.
 * \return TRUE if the channel is a package power channel.
 */
BOOLEAN RaplChannelNameIsPackage(
    _In_ PCWSTR ChannelName
    )
{
    return RaplChannelNameMatchesSuffix(ChannelName, L"_PKG");
}

/**
 * Determines whether an EMI channel record lies wholly inside the metadata the
 * device returned, and whether its name is usable as a string.
 *
 * EMI metadata is variable length and the channel name size is device supplied,
 * so neither the record nor the name can be trusted to stay inside the returned
 * bytes. (dmex)
 *
 * \param Channel The channel record to validate.
 * \param End One past the last valid metadata byte.
 * \return TRUE if the record and its terminated name are within bounds.
 */
BOOLEAN RaplChannelFitsMetadata(
    _In_ EMI_CHANNEL_V2* Channel,
    _In_ PVOID End
    )
{
    SIZE_T remaining;
    ULONG nameSize;

    if ((ULONG_PTR)Channel >= (ULONG_PTR)End)
        return FALSE;

    remaining = (ULONG_PTR)End - (ULONG_PTR)Channel;

    if (remaining < (SIZE_T)FIELD_OFFSET(EMI_CHANNEL_V2, ChannelName))
        return FALSE;

    nameSize = Channel->ChannelNameSize;

    if (nameSize < sizeof(WCHAR) || (nameSize % sizeof(WCHAR)) != 0)
        return FALSE;

    if (remaining < EMI_CHANNEL_V2_LENGTH(nameSize))
        return FALSE;

    // The name is passed to string routines, so require a terminator. (dmex)
    if (Channel->ChannelName[nameSize / sizeof(WCHAR) - 1] != UNICODE_NULL)
        return FALSE;

    return TRUE;
}
//
///**
// * Determines whether a channel exposes the aggregate core domain.
// *
// * Intel systems typically expose the core/package subdomain as `PP0`, for
// * example `RAPL_Package0_PP0`.
// *
// * \param ChannelName EMI channel name.
// * \return TRUE if the channel is the PP0 aggregate core channel.
// */
//BOOLEAN RaplChannelNameIsCoreAggregate(
//    _In_ PCWSTR ChannelName
//    )
//{
//    return RaplChannelNameMatchesSuffix(ChannelName, L"_PP0");
//}

/**
 * Determines whether a channel represents a core-power domain.
 *
 * This accepts both Intel-style `PP0` names and AMD-style per-core `_CORE`
 * names during device enumeration.
 *
 * \param ChannelName EMI channel name.
 * \return TRUE if the channel represents core power.
 */
BOOLEAN RaplChannelNameIsCore(
    _In_ PCWSTR ChannelName
    )
{
    return RaplChannelNameMatchesSuffix(ChannelName, L"_PP0") ||
        RaplChannelNameMatchesSuffix(ChannelName, L"_CORE");
}

/**
 * Determines whether a channel exposes per-core measurements.
 *
 * Some AMD systems expose core power as separate per-core channels such as
 * `RAPL_Package0_Core0_CORE`, `RAPL_Package0_Core1_CORE`, and so on. The
 * sampler aggregates all matching `_CORE` channels into one core-power value
 * when no aggregate `PP0` channel is available.
 *
 * \param ChannelName EMI channel name.
 * \return TRUE if the channel name ends with the per-core suffix.
 */
BOOLEAN RaplChannelNameIsPerCore(
    _In_ PCWSTR ChannelName
    )
{
    return RaplChannelNameMatchesSuffix(ChannelName, L"_CORE");
}

/**
 * Determines whether a channel represents the discrete GPU domain.
 *
 * This matches Intel-style `PP1` channels when that domain is exposed.
 *
 * \param ChannelName EMI channel name.
 * \return TRUE if the channel is the PP1 discrete GPU channel.
 */
BOOLEAN RaplChannelNameIsDiscreteGpu(
    _In_ PCWSTR ChannelName
    )
{
    return RaplChannelNameMatchesSuffix(ChannelName, L"_PP1");
}

/**
 * Determines whether a channel represents the DRAM domain.
 *
 * This matches the optional DRAM domain on platforms that expose it.
 *
 * \param ChannelName EMI channel name.
 * \return TRUE if the channel is the DRAM channel.
 */
BOOLEAN RaplChannelNameIsDram(
    _In_ PCWSTR ChannelName
    )
{
    return RaplChannelNameMatchesSuffix(ChannelName, L"_DRAM");
}

/**
 * Samples one EMI channel and converts the delta to watts.
 *
 * \param MeasurementData Raw EMI measurement buffer.
 * \param ChannelIndex Index of the channel inside the EMI buffer.
 * \param ChannelData Cached absolute energy/time values for the channel.
 * \return The sampled channel power in watts.
 */
FLOAT RaplDeviceSampleSingleChannel(
    _In_ PVOID MeasurementData,
    _In_ ULONG ChannelIndex,
    _Inout_ EV_MEASUREMENT_DATA* ChannelData
    )
{
    EMI_CHANNEL_MEASUREMENT_DATA* data;
    ULONGLONG lastAbsoluteEnergy;
    ULONGLONG lastAbsoluteTime;
    DOUBLE numerator;
    DOUBLE denominator;
    DOUBLE counterValue;

    data = PTR_ADD_OFFSET(MeasurementData, sizeof(EMI_CHANNEL_MEASUREMENT_DATA) * ChannelIndex);
    lastAbsoluteEnergy = ChannelData->AbsoluteEnergy;
    lastAbsoluteTime = ChannelData->AbsoluteTime;
    ChannelData->AbsoluteEnergy = data->AbsoluteEnergy;
    ChannelData->AbsoluteTime = data->AbsoluteTime;

    if (!lastAbsoluteEnergy || !lastAbsoluteTime)
    {
        counterValue = 0.0;
    }
    else if (data->AbsoluteEnergy <= lastAbsoluteEnergy || data->AbsoluteTime <= lastAbsoluteTime)
    {
        counterValue = 0.0;
    }
    else
    {
        numerator = (DOUBLE)(data->AbsoluteEnergy - lastAbsoluteEnergy);
        denominator = (DOUBLE)(data->AbsoluteTime - lastAbsoluteTime) / 36.0;

        if (numerator > 0.0 && denominator > 0.0)
            counterValue = numerator / denominator;
        else
            counterValue = 0.0;
    }

    return counterValue > 0.0 ? (FLOAT)(counterValue / 1000.0) : 0.0f;
}

/**
 * Releases the resources owned by one underlying EMI interface source.
 *
 * \param Source RAPL source to release.
 */
VOID DeleteRaplDeviceSource(
    _Inout_ PDV_RAPL_SOURCE Source
    )
{
    if (Source->DeviceHandle)
    {
        NtClose(Source->DeviceHandle);
        Source->DeviceHandle = NULL;
    }

    if (Source->ChannelDataBuffer)
    {
        PhFree(Source->ChannelDataBuffer);
        Source->ChannelDataBuffer = NULL;
    }

    if (Source->CoreChannelIndexes)
    {
        PhFree(Source->CoreChannelIndexes);
        Source->CoreChannelIndexes = NULL;
    }

    if (Source->CoreChannelData)
    {
        PhFree(Source->CoreChannelData);
        Source->CoreChannelData = NULL;
    }

    PhClearReference(&Source->DevicePath);
}

/**
 * Deletes a RAPL device entry and releases its resources.
 *
 * \param Object RAPL device entry object.
 * \param Flags Object deletion flags.
 */
_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID RaplDeviceEntryDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PDV_RAPL_ENTRY entry = Object;

    PhAcquireQueuedLockExclusive(&RaplDevicesListLock);
    PhRemoveItemList(RaplDevicesList, PhFindItemList(RaplDevicesList, entry));
    PhReleaseQueuedLockExclusive(&RaplDevicesListLock);

    DeleteRaplDeviceId(&entry->Id);
    PhClearReference(&entry->DeviceName);

    if (entry->DeviceSources)
    {
        for (ULONG i = 0; i < entry->DeviceSources->Count; i++)
        {
            PDV_RAPL_SOURCE source = entry->DeviceSources->Items[i];

            DeleteRaplDeviceSource(source);
            PhFree(source);
        }

        PhDereferenceObject(entry->DeviceSources);
    }

    PhDeleteCircularBuffer_FLOAT(&entry->PackageBuffer);
    PhDeleteCircularBuffer_FLOAT(&entry->CoreBuffer);
    PhDeleteCircularBuffer_FLOAT(&entry->DimmBuffer);
    PhDeleteCircularBuffer_FLOAT(&entry->TotalBuffer);
}

/**
 * Initializes the global RAPL device list and object type.
 */
VOID RaplDeviceInitialize(
    VOID
    )
{
    RaplDevicesList = PhCreateList(1);
    RaplDeviceEntryType = PhCreateObjectType(L"RaplDeviceEntry", 0, RaplDeviceEntryDeleteProcedure);
}

VOID ResetRaplSourceSample(
    _Inout_ PDV_RAPL_SOURCE Source
    )
{
    Source->CurrentProcessorPower = 0.0f;
    Source->CurrentCorePower = 0.0f;
    Source->CurrentDramPower = 0.0f;
    Source->CurrentDiscreteGpuPower = 0.0f;
}

VOID ResetRaplEntrySample(
    _Inout_ PDV_RAPL_ENTRY Entry
    )
{
    Entry->CurrentProcessorPower = 0.0f;
    Entry->CurrentCorePower = 0.0f;
    Entry->CurrentDramPower = 0.0f;
    Entry->CurrentDiscreteGpuPower = 0.0f;
    Entry->CurrentComponentPower = 0.0f;
    Entry->CurrentTotalPower = 0.0f;
    Entry->DevicePresent = FALSE;
}

PDV_RAPL_SOURCE FindRaplDeviceSource(
    _In_ PDV_RAPL_ENTRY Entry,
    _In_ PPH_STRING DevicePath
    )
{
    if (!Entry->DeviceSources)
        return NULL;

    for (ULONG i = 0; i < Entry->DeviceSources->Count; i++)
    {
        PDV_RAPL_SOURCE source = Entry->DeviceSources->Items[i];

        if (PhEqualString(source->DevicePath, DevicePath, TRUE))
            return source;
    }

    return NULL;
}

PDV_RAPL_SOURCE AddRaplDeviceSource(
    _Inout_ PDV_RAPL_ENTRY Entry,
    _In_ PPH_STRING DevicePath
    )
{
    PDV_RAPL_SOURCE source;

    source = PhAllocateZero(sizeof(DV_RAPL_SOURCE));
    memset(source->ChannelIndex, ULONG_MAX, sizeof(source->ChannelIndex));
    PhSetReference(&source->DevicePath, DevicePath);
    PhAddItemList(Entry->DeviceSources, source);

    return source;
}

VOID ResolveRaplDeviceAggregateIdentity(
    _Inout_ PDV_RAPL_ENTRY Entry
    )
{
    DV_RAPL_ID resolvedId;
    PPH_STRING deviceDescription;

    if (!Entry->Id.DevicePath || Entry->Id.DeviceInstance)
        return;

    memset(&resolvedId, 0, sizeof(resolvedId));
    deviceDescription = NULL;

    if (QueryRaplDeviceInterfaceInfo(PhGetString(Entry->Id.DevicePath), &resolvedId, &deviceDescription))
    {
        if (resolvedId.DeviceInstance)
            PhSetReference(&Entry->Id.DeviceInstance, resolvedId.DeviceInstance);

        if (!Entry->DeviceName && deviceDescription)
            PhSetReference(&Entry->DeviceName, deviceDescription);

        if (!Entry->Id.DevicePath && resolvedId.DevicePath)
            PhSetReference(&Entry->Id.DevicePath, resolvedId.DevicePath);
    }

    DeleteRaplDeviceId(&resolvedId);
    PhClearReference(&deviceDescription);
}

VOID RefreshRaplDeviceSources(
    _Inout_ PDV_RAPL_ENTRY Entry
    )
{
    ULONG deviceCount = 0;
    const DEV_OBJECT* deviceObjects = NULL;
    const DEVPROPCOMPKEY requestedProperties[] =
    {
        { DEVPKEY_Device_InstanceId, DEVPROP_STORE_SYSTEM, NULL },
    };
    const DEVPROP_FILTER_EXPRESSION deviceFilter[] =
    {
        { DEVPROP_OPERATOR_EQUALS, {{ DEVPKEY_DeviceInterface_ClassGuid, DEVPROP_STORE_SYSTEM, NULL }, DEVPROP_TYPE_GUID, sizeof(GUID), (PVOID)&GUID_DEVICE_ENERGY_METER } }
    };

    ResolveRaplDeviceAggregateIdentity(Entry);

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
        if (Entry->Id.DevicePath && !FindRaplDeviceSource(Entry, Entry->Id.DevicePath))
            AddRaplDeviceSource(Entry, Entry->Id.DevicePath);

        return;
    }

    for (ULONG i = 0; i < deviceCount; i++)
    {
        DV_RAPL_ID deviceId;
        PPH_STRING deviceDescription;

        memset(&deviceId, 0, sizeof(deviceId));
        deviceDescription = NULL;

        if (!QueryRaplDeviceInterfaceInfo((PWSTR)deviceObjects[i].pszObjectId, &deviceId, &deviceDescription))
            continue;

        if (EquivalentRaplDeviceId(&Entry->Id, &deviceId))
        {
            if (!FindRaplDeviceSource(Entry, deviceId.DevicePath))
                AddRaplDeviceSource(Entry, deviceId.DevicePath);

            if (!Entry->Id.DevicePath && deviceId.DevicePath)
                PhSetReference(&Entry->Id.DevicePath, deviceId.DevicePath);

            if (!Entry->DeviceName && deviceDescription)
                PhSetReference(&Entry->DeviceName, deviceDescription);
        }

        DeleteRaplDeviceId(&deviceId);
        PhClearReference(&deviceDescription);
    }

    PhDevFreeObjects(deviceCount, deviceObjects);

    if (Entry->Id.DevicePath && !FindRaplDeviceSource(Entry, Entry->Id.DevicePath))
        AddRaplDeviceSource(Entry, Entry->Id.DevicePath);
}

VOID RefreshRaplSourceSupport(
    _Inout_ PDV_RAPL_SOURCE Source
    )
{
    EMI_METADATA_SIZE metadataSize;
    EMI_METADATA_V2* metadata;
    ULONG returnLength;

    memset(&metadataSize, 0, sizeof(EMI_METADATA_SIZE));

    if (!NT_SUCCESS(PhDeviceIoControlFile(
        Source->DeviceHandle,
        IOCTL_EMI_GET_METADATA_SIZE,
        NULL,
        0,
        &metadataSize,
        sizeof(EMI_METADATA_SIZE),
        NULL
        )))
    {
        Source->CheckedDeviceSupport = TRUE;
        return;
    }

    // The device reports the metadata size; require it to cover the fixed fields
    // and refuse an implausibly large allocation. (dmex)
    if (metadataSize.MetadataSize < (ULONG)FIELD_OFFSET(EMI_METADATA_V2, Channels) ||
        metadataSize.MetadataSize > EV_EMI_MAX_METADATA_SIZE)
    {
        Source->CheckedDeviceSupport = TRUE;
        return;
    }

    metadata = PhAllocate(metadataSize.MetadataSize);
    memset(metadata, 0, metadataSize.MetadataSize);

    returnLength = 0;

    if (NT_SUCCESS(PhDeviceIoControlFile(
        Source->DeviceHandle,
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
        ULONG channelCount;

        // The bytes actually returned bound the channel chain, so clamp the
        // declared count to the number of records that can possibly fit. (dmex)
        channelCount = min((ULONG)metadata->ChannelCount,
            (returnLength - (ULONG)FIELD_OFFSET(EMI_METADATA_V2, Channels)) / (ULONG)EMI_CHANNEL_V2_LENGTH(sizeof(WCHAR)));

        if (
            channelCount &&
            RaplChannelFitsMetadata(channels, end) &&
            channels->MeasurementUnit == EmiMeasurementUnitPicowattHours
            )
        {
            Source->ChannelDataBufferLength = sizeof(EMI_CHANNEL_MEASUREMENT_DATA) * channelCount;

            if (!Source->CoreChannelIndexes)
                Source->CoreChannelIndexes = PhAllocateZero(sizeof(ULONG) * channelCount);
            if (!Source->CoreChannelData)
                Source->CoreChannelData = PhAllocateZero(sizeof(EV_MEASUREMENT_DATA) * channelCount);

            Source->NumberOfCoreChannels = 0;

            for (ULONG j = 0; j < channelCount; j++)
            {
                // Stop at the first record that does not lie wholly inside the
                // returned bytes. (dmex)
                if (!RaplChannelFitsMetadata(channels, end))
                    break;

                if (RaplChannelNameIsPackage(channels->ChannelName))
                    Source->ChannelIndex[EV_EMI_DEVICE_INDEX_PACKAGE] = j;
                if (RaplChannelNameIsCore(channels->ChannelName))
                    Source->ChannelIndex[EV_EMI_DEVICE_INDEX_CORE] = j;
                if (RaplChannelNameIsDiscreteGpu(channels->ChannelName))
                    Source->ChannelIndex[EV_EMI_DEVICE_INDEX_GPUDISCRETE] = j;
                if (RaplChannelNameIsDram(channels->ChannelName))
                    Source->ChannelIndex[EV_EMI_DEVICE_INDEX_DIMM] = j;
                if (Source->CoreChannelIndexes && RaplChannelNameIsPerCore(channels->ChannelName))
                    Source->CoreChannelIndexes[Source->NumberOfCoreChannels++] = j;

                channels = EMI_CHANNEL_V2_NEXT_CHANNEL(channels);
            }

            if (
                Source->ChannelIndex[EV_EMI_DEVICE_INDEX_PACKAGE] != ULONG_MAX &&
                (Source->ChannelIndex[EV_EMI_DEVICE_INDEX_CORE] != ULONG_MAX || Source->NumberOfCoreChannels != 0)
                )
            {
                Source->DeviceSupported = TRUE;
            }
        }
    }

    PhFree(metadata);
    Source->CheckedDeviceSupport = TRUE;
}

/**
 * Refreshes all tracked RAPL devices and samples the available channels.
 *
 * Device support is accepted when a package channel is present together with
 * either an Intel-style aggregate core channel (`PP0`) or AMD-style per-core
 * `_CORE` channels. Optional domains such as `PP1` and `DRAM` are sampled when
 * present but are not required to treat the device as usable.
 *
 * \param RunCount Current provider update count.
 */
VOID RaplDevicesUpdate(
    _In_ ULONG RunCount
    )
{
    PhAcquireQueuedLockShared(&RaplDevicesListLock);

    for (ULONG i = 0; i < RaplDevicesList->Count; i++)
    {
        PDV_RAPL_ENTRY entry;

        entry = PhReferenceObjectUnsafe(RaplDevicesList->Items[i]);

        if (!entry)
            continue;

        ResetRaplEntrySample(entry);
        RefreshRaplDeviceSources(entry);

        if (entry->DeviceSources)
        {
            for (ULONG j = 0; j < entry->DeviceSources->Count; j++)
            {
                PDV_RAPL_SOURCE source = entry->DeviceSources->Items[j];

                ResetRaplSourceSample(source);

                if (!source->DeviceHandle && source->DevicePath)
                {
                    HANDLE deviceHandle;

                    if (NT_SUCCESS(PhCreateFile(
                        &deviceHandle,
                        &source->DevicePath->sr,
                        FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_OPEN,
                        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                        )))
                    {
                        source->DeviceHandle = deviceHandle;
                    }
                }

                if (source->DeviceHandle && !source->CheckedDeviceSupport)
                    RefreshRaplSourceSupport(source);

                if (source->DeviceHandle)
                {
                    entry->DevicePresent = TRUE;
                    source->DevicePresent = TRUE;

                    if (source->DeviceSupported)
                    {
                        if (!source->ChannelDataBuffer)
                            source->ChannelDataBuffer = PhAllocateZero(source->ChannelDataBufferLength);

                        if (source->ChannelDataBuffer && NT_SUCCESS(PhDeviceIoControlFile(
                            source->DeviceHandle,
                            IOCTL_EMI_GET_MEASUREMENT,
                            NULL,
                            0,
                            source->ChannelDataBuffer,
                            source->ChannelDataBufferLength,
                            NULL
                            )))
                        {
                            RaplDeviceSampleData(source, source->ChannelDataBuffer, EV_EMI_DEVICE_INDEX_PACKAGE);
                            RaplDeviceSampleData(source, source->ChannelDataBuffer, EV_EMI_DEVICE_INDEX_CORE);
                            RaplDeviceSampleData(source, source->ChannelDataBuffer, EV_EMI_DEVICE_INDEX_GPUDISCRETE);
                            RaplDeviceSampleData(source, source->ChannelDataBuffer, EV_EMI_DEVICE_INDEX_DIMM);

                            entry->CurrentProcessorPower += source->CurrentProcessorPower;
                            entry->CurrentCorePower += source->CurrentCorePower;
                            entry->CurrentDramPower += source->CurrentDramPower;
                            entry->CurrentDiscreteGpuPower += source->CurrentDiscreteGpuPower;
                        }
                    }
                }
                else
                {
                    source->DevicePresent = FALSE;
                }
            }
        }

        {
            FLOAT accountedPower;

            accountedPower = entry->CurrentCorePower + entry->CurrentDiscreteGpuPower;

            if (entry->CurrentProcessorPower > accountedPower && accountedPower > 0.0f)
                entry->CurrentComponentPower = (entry->CurrentProcessorPower - accountedPower);
            else
                entry->CurrentComponentPower = 0.0f;

            entry->CurrentTotalPower =
                entry->CurrentProcessorPower +
                entry->CurrentDiscreteGpuPower +
                entry->CurrentDramPower;
        }

        if (RunCount != 0)
        {
            PhAddItemCircularBuffer_FLOAT(&entry->PackageBuffer, entry->CurrentProcessorPower);
            PhAddItemCircularBuffer_FLOAT(&entry->CoreBuffer, entry->CurrentCorePower);
            PhAddItemCircularBuffer_FLOAT(&entry->DimmBuffer, entry->CurrentDramPower);
            PhAddItemCircularBuffer_FLOAT(&entry->TotalBuffer, entry->CurrentTotalPower);
        }

        PhDereferenceObjectDeferDelete(entry);
    }

    PhReleaseQueuedLockShared(&RaplDevicesListLock);
}

/**
 * Initializes a RAPL device identifier from a device path.
 * \param Id Destination identifier.
 * \param DevicePath Device interface path.
 */
VOID InitializeRaplDeviceId(
    _Out_ PDV_RAPL_ID Id,
    _In_opt_ PPH_STRING DevicePath,
    _In_opt_ PPH_STRING DeviceInstance
    )
{
    PhSetReference(&Id->DevicePath, DevicePath);
    PhSetReference(&Id->DeviceInstance, DeviceInstance);
}

/**
 * Copies a RAPL device identifier.
 * \param Destination Destination identifier.
 * \param Source Source identifier.
 */
VOID CopyRaplDeviceId(
    _Out_ PDV_RAPL_ID Destination,
    _In_ PDV_RAPL_ID Source
    )
{
    InitializeRaplDeviceId(
        Destination,
        Source->DevicePath,
        Source->DeviceInstance
        );
}

/**
 * Releases the references held by a RAPL device identifier.
 * \param Id Identifier to release.
 */
VOID DeleteRaplDeviceId(
    _Inout_ PDV_RAPL_ID Id
    )
{
    PhClearReference(&Id->DevicePath);
    PhClearReference(&Id->DeviceInstance);
}

/**
 * Compares two RAPL device identifiers.
 *
 * \param Id1 First identifier.
 * \param Id2 Second identifier.
 * \return TRUE if both identifiers refer to the same logical RAPL package.
 */
BOOLEAN EquivalentRaplDeviceId(
    _In_ PDV_RAPL_ID Id1,
    _In_ PDV_RAPL_ID Id2
    )
{
    if (Id1->DeviceInstance && Id2->DeviceInstance)
        return PhEqualString(Id1->DeviceInstance, Id2->DeviceInstance, TRUE);

    return PhEqualString(Id1->DevicePath, Id2->DevicePath, TRUE);
}

/**
 * Creates and registers a new RAPL device entry.
 *
 * \param Id Identifier for the new device.
 * \return The newly created RAPL device entry.
 */
PDV_RAPL_ENTRY CreateRaplDeviceEntry(
    _In_ PDV_RAPL_ID Id
    )
{
    PDV_RAPL_ENTRY entry;
    ULONG sampleCount;

    entry = PhCreateObjectZero(sizeof(DV_RAPL_ENTRY), RaplDeviceEntryType);
    CopyRaplDeviceId(&entry->Id, Id);
    entry->DeviceSources = PhCreateList(1);

    sampleCount = PhGetIntegerSetting(SETTING_SAMPLE_COUNT);
    PhInitializeCircularBuffer_FLOAT(&entry->PackageBuffer, sampleCount);
    PhInitializeCircularBuffer_FLOAT(&entry->CoreBuffer, sampleCount);
    PhInitializeCircularBuffer_FLOAT(&entry->DimmBuffer, sampleCount);
    PhInitializeCircularBuffer_FLOAT(&entry->TotalBuffer, sampleCount);

    PhAcquireQueuedLockExclusive(&RaplDevicesListLock);
    PhAddItemList(RaplDevicesList, entry);
    PhReleaseQueuedLockExclusive(&RaplDevicesListLock);

    return entry;
}

/**
 * Samples one logical RAPL domain from the EMI measurement buffer.
 *
 * Core sampling handles both naming schemes: Intel-style aggregate core power
 * is read from a single `PP0` channel, while AMD-style `CoreN_CORE` channels
 * are summed to produce the logical core-power value shown by the UI.
 *
 * \param DeviceSource Target RAPL device source.
 * \param MeasurementData Raw EMI measurement buffer.
 * \param DeviceIndex Logical RAPL domain to sample.
 */
VOID RaplDeviceSampleData(
    _In_ PDV_RAPL_SOURCE DeviceSource,
    _In_ PVOID MeasurementData,
    _In_ EV_EMI_DEVICE_INDEX DeviceIndex
    )
{
    if (
        DeviceIndex == EV_EMI_DEVICE_INDEX_CORE &&
        DeviceSource->ChannelIndex[DeviceIndex] == ULONG_MAX &&
        DeviceSource->NumberOfCoreChannels != 0 &&
        DeviceSource->CoreChannelData
        )
    {
        FLOAT totalCorePower = 0.0f;

        for (ULONG i = 0; i < DeviceSource->NumberOfCoreChannels; i++)
        {
            totalCorePower += RaplDeviceSampleSingleChannel(
                MeasurementData,
                DeviceSource->CoreChannelIndexes[i],
                &DeviceSource->CoreChannelData[i]
                );
        }

        DeviceSource->CurrentCorePower = totalCorePower;
        return;
    }

    if (DeviceSource->ChannelIndex[DeviceIndex] == ULONG_MAX)
        return;

    switch (DeviceIndex)
    {
    case EV_EMI_DEVICE_INDEX_PACKAGE:
        DeviceSource->CurrentProcessorPower = RaplDeviceSampleSingleChannel(
            MeasurementData,
            DeviceSource->ChannelIndex[DeviceIndex],
            &DeviceSource->ChannelData[DeviceIndex]
            );
        break;
    case EV_EMI_DEVICE_INDEX_CORE:
        DeviceSource->CurrentCorePower = RaplDeviceSampleSingleChannel(
            MeasurementData,
            DeviceSource->ChannelIndex[DeviceIndex],
            &DeviceSource->ChannelData[DeviceIndex]
            );
        break;
    case EV_EMI_DEVICE_INDEX_DIMM:
        DeviceSource->CurrentDramPower = RaplDeviceSampleSingleChannel(
            MeasurementData,
            DeviceSource->ChannelIndex[DeviceIndex],
            &DeviceSource->ChannelData[DeviceIndex]
            );
        break;
    case EV_EMI_DEVICE_INDEX_GPUDISCRETE:
        DeviceSource->CurrentDiscreteGpuPower = RaplDeviceSampleSingleChannel(
            MeasurementData,
            DeviceSource->ChannelIndex[DeviceIndex],
            &DeviceSource->ChannelData[DeviceIndex]
            );
        break;
    }
}
