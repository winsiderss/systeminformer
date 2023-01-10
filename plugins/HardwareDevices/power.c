/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2021-2022
 *
 */

#include "devices.h"
#include <emi.h>

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

    PhDeleteCircularBuffer_FLOAT(&entry->PackageBuffer);
    PhDeleteCircularBuffer_FLOAT(&entry->CoreBuffer);
    PhDeleteCircularBuffer_FLOAT(&entry->DimmBuffer);
    PhDeleteCircularBuffer_FLOAT(&entry->TotalBuffer);
}

VOID RaplDeviceInitialize(
    VOID
    )
{
    RaplDevicesList = PhCreateList(1);
    RaplDeviceEntryType = PhCreateObjectType(L"RaplDeviceEntry", 0, RaplDeviceEntryDeleteProcedure);
}

VOID RaplDevicesUpdate(
    VOID
    )
{
    static ULONG runCount = 0; // MUST keep in sync with runCount in process provider

    PhAcquireQueuedLockShared(&RaplDevicesListLock);

    for (ULONG i = 0; i < RaplDevicesList->Count; i++)
    {
        PDV_RAPL_ENTRY entry;

        entry = PhReferenceObjectSafe(RaplDevicesList->Items[i]);

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

                            for (ULONG j = 0; j < metadata->ChannelCount; j++)
                            {
                                if (PhEqualStringZ(channels->ChannelName, L"RAPL_Package0_PKG", TRUE))
                                    entry->ChannelIndex[EV_EMI_DEVICE_INDEX_PACKAGE] = j;
                                if (PhEqualStringZ(channels->ChannelName, L"RAPL_Package0_PP0", TRUE))
                                    entry->ChannelIndex[EV_EMI_DEVICE_INDEX_CORE] = j;
                                if (PhEqualStringZ(channels->ChannelName, L"RAPL_Package0_PP1", TRUE))
                                    entry->ChannelIndex[EV_EMI_DEVICE_INDEX_GPUDISCRETE] = j;
                                if (PhEqualStringZ(channels->ChannelName, L"RAPL_Package0_DRAM", TRUE))
                                    entry->ChannelIndex[EV_EMI_DEVICE_INDEX_DIMM] = j;

                                channels = EMI_CHANNEL_V2_NEXT_CHANNEL(channels);
                            }
                        }

                        if (
                            entry->ChannelIndex[EV_EMI_DEVICE_INDEX_PACKAGE] != ULONG_MAX &&
                            entry->ChannelIndex[EV_EMI_DEVICE_INDEX_CORE] != ULONG_MAX &&
                            entry->ChannelIndex[EV_EMI_DEVICE_INDEX_GPUDISCRETE] != ULONG_MAX &&
                            entry->ChannelIndex[EV_EMI_DEVICE_INDEX_DIMM] != ULONG_MAX
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

        if (runCount != 0)
        {
            PhAddItemCircularBuffer_FLOAT(&entry->PackageBuffer, entry->CurrentProcessorPower);
            PhAddItemCircularBuffer_FLOAT(&entry->CoreBuffer, entry->CurrentCorePower);
            PhAddItemCircularBuffer_FLOAT(&entry->DimmBuffer, entry->CurrentDramPower);
            PhAddItemCircularBuffer_FLOAT(&entry->TotalBuffer, entry->CurrentTotalPower);
        }

        PhDereferenceObjectDeferDelete(entry);
    }

    PhReleaseQueuedLockShared(&RaplDevicesListLock);

    runCount++;
}

VOID InitializeRaplDeviceId(
    _Out_ PDV_RAPL_ID Id,
    _In_ PPH_STRING DevicePath
    )
{
    PhSetReference(&Id->DevicePath, DevicePath);
}

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

VOID DeleteRaplDeviceId(
    _Inout_ PDV_RAPL_ID Id
    )
{
    PhClearReference(&Id->DevicePath);
}

BOOLEAN EquivalentRaplDeviceId(
    _In_ PDV_RAPL_ID Id1,
    _In_ PDV_RAPL_ID Id2
    )
{
    return PhEqualString(Id1->DevicePath, Id2->DevicePath, TRUE);
}

PDV_RAPL_ENTRY CreateRaplDeviceEntry(
    _In_ PDV_RAPL_ID Id
    )
{
    PDV_RAPL_ENTRY entry;
    ULONG sampleCount;

    entry = PhCreateObjectZero(sizeof(DV_RAPL_ENTRY), RaplDeviceEntryType);
    memset(entry->ChannelIndex, ULONG_MAX, sizeof(entry->ChannelIndex));

    CopyRaplDeviceId(&entry->Id, Id);

    sampleCount = PhGetIntegerSetting(L"SampleCount");
    PhInitializeCircularBuffer_FLOAT(&entry->PackageBuffer, sampleCount);
    PhInitializeCircularBuffer_FLOAT(&entry->CoreBuffer, sampleCount);
    PhInitializeCircularBuffer_FLOAT(&entry->DimmBuffer, sampleCount);
    PhInitializeCircularBuffer_FLOAT(&entry->TotalBuffer, sampleCount);

    PhAcquireQueuedLockExclusive(&RaplDevicesListLock);
    PhAddItemList(RaplDevicesList, entry);
    PhReleaseQueuedLockExclusive(&RaplDevicesListLock);

    return entry;
}

VOID RaplDeviceSampleData(
    _In_ PDV_RAPL_ENTRY DeviceEntry,
    _In_ PVOID MeasurementData,
    _In_ EV_EMI_DEVICE_INDEX DeviceIndex
    )
{
    EMI_CHANNEL_MEASUREMENT_DATA* data;
    ULONGLONG lastAbsoluteEnergy;
    ULONGLONG lastAbsoluteTime;
    FLOAT numerator;
    FLOAT denomenator;
    FLOAT counterValue;

    if (DeviceIndex == EV_EMI_DEVICE_INDEX_MAX)
    {
        if (DeviceEntry->CurrentProcessorPower && DeviceEntry->CurrentCorePower)
            DeviceEntry->CurrentComponentPower = (DeviceEntry->CurrentProcessorPower - (DeviceEntry->CurrentCorePower + DeviceEntry->CurrentDiscreteGpuPower));
        else
            DeviceEntry->CurrentComponentPower = 0.0;

        DeviceEntry->CurrentTotalPower =
            DeviceEntry->CurrentProcessorPower +
            DeviceEntry->CurrentCorePower +
            DeviceEntry->CurrentDiscreteGpuPower +
            DeviceEntry->CurrentDramPower;
        return;
    }

    data = PTR_ADD_OFFSET(MeasurementData, sizeof(EMI_CHANNEL_MEASUREMENT_DATA) * DeviceEntry->ChannelIndex[DeviceIndex]);
    lastAbsoluteEnergy = DeviceEntry->ChannelData[DeviceIndex].AbsoluteEnergy;
    lastAbsoluteTime = DeviceEntry->ChannelData[DeviceIndex].AbsoluteTime;
    DeviceEntry->ChannelData[DeviceIndex].AbsoluteEnergy = data->AbsoluteEnergy;
    DeviceEntry->ChannelData[DeviceIndex].AbsoluteTime = data->AbsoluteTime;

    numerator = (FLOAT)lastAbsoluteEnergy - (FLOAT)data->AbsoluteEnergy;
    denomenator = (FLOAT)(lastAbsoluteTime / 36) - (FLOAT)(data->AbsoluteTime / 36);

    if (numerator && denomenator)
        counterValue = numerator / denomenator;
    else
        counterValue = 0.0;

    switch (DeviceIndex)
    {
    case EV_EMI_DEVICE_INDEX_PACKAGE:
        DeviceEntry->CurrentProcessorPower = counterValue ? counterValue / 1000.f : 0.f; // always supported per spec
        break;
    case EV_EMI_DEVICE_INDEX_CORE:
        DeviceEntry->CurrentCorePower = counterValue ? counterValue / 1000.f : 0.f; // always supported per spec
        break;
    case EV_EMI_DEVICE_INDEX_DIMM:
        DeviceEntry->CurrentDramPower = counterValue ? counterValue / 1000.f : 0.f; // might be unavailable
        break;
    case EV_EMI_DEVICE_INDEX_GPUDISCRETE:
        DeviceEntry->CurrentDiscreteGpuPower = counterValue ? counterValue / 1000.f : 0.f; // might be unavailable
        break;
    }
}
