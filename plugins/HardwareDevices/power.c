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
 * The suffix-based matching lets the parser accept both Intel-style package
 * domains such as `RAPL_Package0_PKG`, `RAPL_Package0_PP0`, `RAPL_Package0_PP1`
 * and `RAPL_Package0_DRAM`, and AMD-style per-core names such as
 * `RAPL_Package0_Core0_CORE`.
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
 * This matches package-level channels such as `RAPL_Package0_PKG` on both
 * Intel and AMD systems.
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
 * Determines whether a channel exposes the aggregate core domain.
 *
 * Intel systems typically expose the core/package subdomain as `PP0`, for
 * example `RAPL_Package0_PP0`.
 *
 * \param ChannelName EMI channel name.
 * \return TRUE if the channel is the PP0 aggregate core channel.
 */
static BOOLEAN RaplChannelNameIsCoreAggregate(
    _In_ PCWSTR ChannelName
    )
{
    return RaplChannelNameMatchesSuffix(ChannelName, L"_PP0");
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
static BOOLEAN RaplChannelNameIsPerCore(
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
static BOOLEAN RaplChannelNameIsDiscreteGpu(
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
static BOOLEAN RaplChannelNameIsDram(
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
static FLOAT RaplDeviceSampleSingleChannel(
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
 * Deletes a RAPL device entry and releases its resources.
 *
 * \param Object RAPL device entry object.
 * \param Flags Object deletion flags.
 */
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

    if (entry->DeviceHandle)
    {
        NtClose(entry->DeviceHandle);
        entry->DeviceHandle = NULL;
    }

    if (entry->ChannelDataBuffer)
    {
        PhFree(entry->ChannelDataBuffer);
        entry->ChannelDataBuffer = NULL;
    }

    if (entry->CoreChannelIndexes)
    {
        PhFree(entry->CoreChannelIndexes);
        entry->CoreChannelIndexes = NULL;
    }

    if (entry->CoreChannelData)
    {
        PhFree(entry->CoreChannelData);
        entry->CoreChannelData = NULL;
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

        if (!entry->DeviceHandle)
        {
            HANDLE deviceHandle;

            if (NT_SUCCESS(PhCreateFile(
                &deviceHandle,
                &entry->Id.DevicePath->sr,
                FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                )))
            {
                entry->DeviceHandle = deviceHandle;
            }

            if (entry->DeviceHandle && !entry->CheckedDeviceSupport)
            {
                EMI_METADATA_SIZE metadataSize;
                EMI_METADATA_V2* metadata;

                memset(&metadataSize, 0, sizeof(EMI_METADATA_SIZE));

                if (NT_SUCCESS(PhDeviceIoControlFile(
                    entry->DeviceHandle,
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
                        entry->DeviceHandle,
                        IOCTL_EMI_GET_METADATA,
                        NULL,
                        0,
                        metadata,
                        metadataSize.MetadataSize,
                        NULL
                        )))
                    {
                        EMI_CHANNEL_V2* channels = metadata->Channels;

                        if (
                            metadata->ChannelCount &&
                            channels->MeasurementUnit == EmiMeasurementUnitPicowattHours
                            )
                        {
                            entry->ChannelDataBufferLength = sizeof(EMI_CHANNEL_MEASUREMENT_DATA) * metadata->ChannelCount;

                            if (!entry->CoreChannelIndexes)
                                entry->CoreChannelIndexes = PhAllocateZero(sizeof(ULONG) * metadata->ChannelCount);
                            if (!entry->CoreChannelData)
                                entry->CoreChannelData = PhAllocateZero(sizeof(EV_MEASUREMENT_DATA) * metadata->ChannelCount);

                            for (ULONG j = 0; j < metadata->ChannelCount; j++)
                            {
                                if (RaplChannelNameIsPackage(channels->ChannelName))
                                    entry->ChannelIndex[EV_EMI_DEVICE_INDEX_PACKAGE] = j;
                                if (RaplChannelNameIsCoreAggregate(channels->ChannelName))
                                    entry->ChannelIndex[EV_EMI_DEVICE_INDEX_CORE] = j;
                                if (RaplChannelNameIsDiscreteGpu(channels->ChannelName))
                                    entry->ChannelIndex[EV_EMI_DEVICE_INDEX_GPUDISCRETE] = j;
                                if (RaplChannelNameIsDram(channels->ChannelName))
                                    entry->ChannelIndex[EV_EMI_DEVICE_INDEX_DIMM] = j;
                                if (entry->CoreChannelIndexes && RaplChannelNameIsPerCore(channels->ChannelName))
                                    entry->CoreChannelIndexes[entry->NumberOfCoreChannels++] = j;

                                channels = EMI_CHANNEL_V2_NEXT_CHANNEL(channels);
                            }
                        }

                        if (
                            entry->ChannelIndex[EV_EMI_DEVICE_INDEX_PACKAGE] != ULONG_MAX &&
                            (entry->ChannelIndex[EV_EMI_DEVICE_INDEX_CORE] != ULONG_MAX || entry->NumberOfCoreChannels != 0)
                            )
                        {
                            entry->DeviceSupported = TRUE;
                        }
                    }

                    PhFree(metadata);
                }

                entry->CheckedDeviceSupport = TRUE;
            }
        }

        if (entry->DeviceHandle)
        {
            if (entry->DeviceSupported)
            {
                if (!entry->ChannelDataBuffer)
                {
                    entry->ChannelDataBuffer = PhAllocateZero(entry->ChannelDataBufferLength);
                }

                if (entry->ChannelDataBuffer && NT_SUCCESS(PhDeviceIoControlFile(
                    entry->DeviceHandle,
                    IOCTL_EMI_GET_MEASUREMENT,
                    NULL,
                    0,
                    entry->ChannelDataBuffer,
                    entry->ChannelDataBufferLength,
                    NULL
                    )))
                {
                    RaplDeviceSampleData(entry, entry->ChannelDataBuffer, EV_EMI_DEVICE_INDEX_PACKAGE);
                    RaplDeviceSampleData(entry, entry->ChannelDataBuffer, EV_EMI_DEVICE_INDEX_CORE);
                    RaplDeviceSampleData(entry, entry->ChannelDataBuffer, EV_EMI_DEVICE_INDEX_GPUDISCRETE);
                    RaplDeviceSampleData(entry, entry->ChannelDataBuffer, EV_EMI_DEVICE_INDEX_DIMM);
                    RaplDeviceSampleData(entry, entry->ChannelDataBuffer, EV_EMI_DEVICE_INDEX_MAX);
                }
            }

            entry->DevicePresent = TRUE;
        }
        else
        {
            entry->CurrentProcessorPower = 0.f;
            entry->CurrentCorePower = 0.f;
            entry->CurrentDramPower = 0.f;
            entry->CurrentDiscreteGpuPower = 0.f;
            entry->CurrentComponentPower = 0.f;
            entry->CurrentTotalPower = 0.f;
            entry->DevicePresent = FALSE;
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
    _In_ PPH_STRING DevicePath
    )
{
    PhSetReference(&Id->DevicePath, DevicePath);
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
        Source->DevicePath
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
}

/**
 * Compares two RAPL device identifiers.
 *
 * \param Id1 First identifier.
 * \param Id2 Second identifier.
 * \return TRUE if both identifiers refer to the same device path.
 */
BOOLEAN EquivalentRaplDeviceId(
    _In_ PDV_RAPL_ID Id1,
    _In_ PDV_RAPL_ID Id2
    )
{
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
    memset(entry->ChannelIndex, ULONG_MAX, sizeof(entry->ChannelIndex));

    CopyRaplDeviceId(&entry->Id, Id);

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
 * \param DeviceEntry Target RAPL device entry.
 * \param MeasurementData Raw EMI measurement buffer.
 * \param DeviceIndex Logical RAPL domain to sample.
 */
VOID RaplDeviceSampleData(
    _In_ PDV_RAPL_ENTRY DeviceEntry,
    _In_ PVOID MeasurementData,
    _In_ EV_EMI_DEVICE_INDEX DeviceIndex
    )
{
    if (DeviceIndex == EV_EMI_DEVICE_INDEX_MAX)
    {
        FLOAT accountedPower;

        accountedPower = DeviceEntry->CurrentCorePower + DeviceEntry->CurrentDiscreteGpuPower;

        if (DeviceEntry->CurrentProcessorPower > accountedPower && accountedPower > 0.0f)
            DeviceEntry->CurrentComponentPower = (DeviceEntry->CurrentProcessorPower - accountedPower);
        else
            DeviceEntry->CurrentComponentPower = 0.0f;

        DeviceEntry->CurrentTotalPower =
            DeviceEntry->CurrentProcessorPower +
            //DeviceEntry->CurrentCorePower +
            DeviceEntry->CurrentDiscreteGpuPower +
            DeviceEntry->CurrentDramPower;
        return;
    }

    if (
        DeviceIndex == EV_EMI_DEVICE_INDEX_CORE &&
        DeviceEntry->ChannelIndex[DeviceIndex] == ULONG_MAX &&
        DeviceEntry->NumberOfCoreChannels != 0 &&
        DeviceEntry->CoreChannelData
        )
    {
        FLOAT totalCorePower = 0.0f;

        for (ULONG i = 0; i < DeviceEntry->NumberOfCoreChannels; i++)
        {
            totalCorePower += RaplDeviceSampleSingleChannel(
                MeasurementData,
                DeviceEntry->CoreChannelIndexes[i],
                &DeviceEntry->CoreChannelData[i]
                );
        }

        DeviceEntry->CurrentCorePower = totalCorePower;
        return;
    }

    if (DeviceEntry->ChannelIndex[DeviceIndex] == ULONG_MAX)
        return;

    switch (DeviceIndex)
    {
    case EV_EMI_DEVICE_INDEX_PACKAGE:
        DeviceEntry->CurrentProcessorPower = RaplDeviceSampleSingleChannel(
            MeasurementData,
            DeviceEntry->ChannelIndex[DeviceIndex],
            &DeviceEntry->ChannelData[DeviceIndex]
            );
        break;
    case EV_EMI_DEVICE_INDEX_CORE:
        DeviceEntry->CurrentCorePower = RaplDeviceSampleSingleChannel(
            MeasurementData,
            DeviceEntry->ChannelIndex[DeviceIndex],
            &DeviceEntry->ChannelData[DeviceIndex]
            );
        break;
    case EV_EMI_DEVICE_INDEX_DIMM:
        DeviceEntry->CurrentDramPower = RaplDeviceSampleSingleChannel(
            MeasurementData,
            DeviceEntry->ChannelIndex[DeviceIndex],
            &DeviceEntry->ChannelData[DeviceIndex]
            );
        break;
    case EV_EMI_DEVICE_INDEX_GPUDISCRETE:
        DeviceEntry->CurrentDiscreteGpuPower = RaplDeviceSampleSingleChannel(
            MeasurementData,
            DeviceEntry->ChannelIndex[DeviceIndex],
            &DeviceEntry->ChannelData[DeviceIndex]
            );
        break;
    }
}
