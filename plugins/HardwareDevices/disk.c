/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     dmex    2015-2024
 *
 */

#include "devices.h"
_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID DiskEntryDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PDV_DISK_ENTRY entry = Object;

    PhAcquireQueuedLockExclusive(&DiskDevicesListLock);
    PhRemoveItemList(DiskDevicesList, PhFindItemList(DiskDevicesList, entry));
    PhReleaseQueuedLockExclusive(&DiskDevicesListLock);

    DeleteDiskId(&entry->Id);
    PhClearReference(&entry->DiskName);

    PhDeleteCircularBuffer_ULONG64(&entry->ReadBuffer);
    PhDeleteCircularBuffer_ULONG64(&entry->WriteBuffer);

    AddRemoveDeviceChangeCallback();
}

VOID DiskDevicesInitialize(
    VOID
    )
{
    DiskDevicesList = PhCreateList(1);
    DiskDeviceEntryType = PhCreateObjectType(L"DiskDeviceEntry", 0, DiskEntryDeleteProcedure);
}

VOID DiskDevicesUpdate(
    _In_ ULONG RunCount
    )
{
    PhAcquireQueuedLockShared(&DiskDevicesListLock);

    for (ULONG i = 0; i < DiskDevicesList->Count; i++)
    {
        HANDLE deviceHandle;
        PDV_DISK_ENTRY entry;

        entry = PhReferenceObjectSafe(DiskDevicesList->Items[i]);

        if (!entry)
            continue;

        if (NT_SUCCESS(PhCreateFile(
            &deviceHandle,
            &entry->Id.DevicePath->sr,
            FILE_READ_ATTRIBUTES | SYNCHRONIZE, // (PhGetOwnTokenAttributes().Elevated ? FILE_GENERIC_READ : FILE_READ_ATTRIBUTES)
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            DISK_PERFORMANCE diskPerformance;

            if (NT_SUCCESS(DiskDriveQueryStatistics(deviceHandle, &diskPerformance)))
            {
                ULONG64 readTime;
                ULONG64 writeTime;
                ULONG64 idleTime;
                ULONG readCount;
                ULONG writeCount;
                ULONG64 queryTime;

                PhUpdateDelta(&entry->BytesReadDelta, diskPerformance.BytesRead.QuadPart);
                PhUpdateDelta(&entry->BytesWrittenDelta, diskPerformance.BytesWritten.QuadPart);
                PhUpdateDelta(&entry->ReadTimeDelta, diskPerformance.ReadTime.QuadPart);
                PhUpdateDelta(&entry->WriteTimeDelta, diskPerformance.WriteTime.QuadPart);
                PhUpdateDelta(&entry->IdleTimeDelta, diskPerformance.IdleTime.QuadPart);
                PhUpdateDelta(&entry->ReadCountDelta, diskPerformance.ReadCount);
                PhUpdateDelta(&entry->WriteCountDelta, diskPerformance.WriteCount);
                PhUpdateDelta(&entry->QueryTimeDelta, diskPerformance.QueryTime.QuadPart);

                readTime = entry->ReadTimeDelta.Delta;
                writeTime = entry->WriteTimeDelta.Delta;
                idleTime = entry->IdleTimeDelta.Delta;
                readCount = entry->ReadCountDelta.Delta;
                writeCount = entry->WriteCountDelta.Delta;
                queryTime = entry->QueryTimeDelta.Delta;

                if (readCount + writeCount != 0)
                    entry->ResponseTime = ((FLOAT)readTime + (FLOAT)writeTime) / (readCount + writeCount);
                else
                    entry->ResponseTime = 0;

                if (queryTime != 0)
                    entry->ActiveTime = (FLOAT)(queryTime - idleTime) / queryTime * 100;
                else
                    entry->ActiveTime = 0.0f;

                if (entry->ActiveTime > 100.f)
                    entry->ActiveTime = 0.f;
                if (entry->ActiveTime < 0.f)
                    entry->ActiveTime = 0.f;

                entry->QueueDepth = diskPerformance.QueueDepth;
                entry->SplitCount = diskPerformance.SplitCount;
                entry->DiskIndex = diskPerformance.StorageDeviceNumber;
                entry->DevicePresent = TRUE;
            }
            else
            {
                // Disk has been disconnected or dismounted.
                PhInitializeDelta(&entry->BytesReadDelta);
                PhInitializeDelta(&entry->BytesWrittenDelta);
                PhInitializeDelta(&entry->ReadTimeDelta);
                PhInitializeDelta(&entry->WriteTimeDelta);
                PhInitializeDelta(&entry->IdleTimeDelta);
                PhInitializeDelta(&entry->ReadCountDelta);
                PhInitializeDelta(&entry->WriteCountDelta);
                PhInitializeDelta(&entry->QueryTimeDelta);

                entry->ResponseTime = 0;
                entry->ActiveTime = 0.0f;
                entry->QueueDepth = 0;
                entry->SplitCount = 0;
                entry->DiskIndex = ULONG_MAX;
                entry->DevicePresent = FALSE;
                PhClearReference(&entry->DiskIndexName);
            }

            NtClose(deviceHandle);
        }
        else
        {
            // Disk has been disconnected or dismounted.
            PhInitializeDelta(&entry->BytesReadDelta);
            PhInitializeDelta(&entry->BytesWrittenDelta);
            PhInitializeDelta(&entry->ReadTimeDelta);
            PhInitializeDelta(&entry->WriteTimeDelta);
            PhInitializeDelta(&entry->IdleTimeDelta);
            PhInitializeDelta(&entry->ReadCountDelta);
            PhInitializeDelta(&entry->WriteCountDelta);
            PhInitializeDelta(&entry->QueryTimeDelta);

            entry->ResponseTime = 0;
            entry->ActiveTime = 0.0f;
            entry->QueueDepth = 0;
            entry->SplitCount = 0;
            entry->DiskIndex = ULONG_MAX;
            entry->DevicePresent = FALSE;
            PhClearReference(&entry->DiskIndexName);
        }

        if (!entry->HaveFirstSample)
        {
            // The first sample must be zero.
            entry->BytesReadDelta.Delta = 0;
            entry->BytesWrittenDelta.Delta = 0;
            entry->HaveFirstSample = TRUE;
        }

        if (RunCount != 0)
        {
            PhAddItemCircularBuffer_ULONG64(&entry->ReadBuffer, entry->BytesReadDelta.Delta);
            PhAddItemCircularBuffer_ULONG64(&entry->WriteBuffer, entry->BytesWrittenDelta.Delta);
        }

        PhDereferenceObjectDeferDelete(entry);
    }

    PhReleaseQueuedLockShared(&DiskDevicesListLock);
}

VOID DiskDeviceUpdateDeviceInfo(
    _In_opt_ HANDLE DeviceHandle,
    _In_ PDV_DISK_ENTRY DiskEntry
    )
{
    if (PhIsNullOrEmptyString(DiskEntry->DiskName) || DiskEntry->DiskIndex == ULONG_MAX)
    {
        HANDLE deviceHandle = NULL;

        if (DeviceHandle)
        {
            deviceHandle = DeviceHandle;
        }
        else
        {
            PhCreateFile(
                &deviceHandle,
                &DiskEntry->Id.DevicePath->sr,
                FILE_READ_ATTRIBUTES | SYNCHRONIZE, // (PhGetOwnTokenAttributes().Elevated ? FILE_GENERIC_READ : FILE_READ_ATTRIBUTES)
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                );
        }

        if (deviceHandle)
        {
            if (PhIsNullOrEmptyString(DiskEntry->DiskName))
            {
                PPH_STRING diskName = NULL;

                if (NT_SUCCESS(DiskDriveQueryDeviceInformation(deviceHandle, NULL, &diskName, NULL, NULL)))
                {
                    DiskEntry->DiskName = diskName;
                }
            }

            if (DiskEntry->DiskIndex == ULONG_MAX)
            {
                ULONG diskIndex = ULONG_MAX; // Note: Do not initialize to zero.

                if (NT_SUCCESS(DiskDriveQueryDeviceTypeAndNumber(deviceHandle, &diskIndex, NULL)))
                {
                    DiskEntry->DiskIndex = diskIndex;
                }
            }

            if (!DeviceHandle && deviceHandle)
            {
                NtClose(deviceHandle);
            }
        }
    }
}

VOID InitializeDiskId(
    _Out_ PDV_DISK_ID Id,
    _In_ PPH_STRING DevicePath
    )
{
    PhSetReference(&Id->DevicePath, DevicePath);
}

VOID CopyDiskId(
    _Out_ PDV_DISK_ID Destination,
    _In_ PDV_DISK_ID Source
    )
{
    InitializeDiskId(
        Destination,
        Source->DevicePath
        );
}

VOID DeleteDiskId(
    _Inout_ PDV_DISK_ID Id
    )
{
    PhClearReference(&Id->DevicePath);
}

BOOLEAN EquivalentDiskId(
    _In_ PDV_DISK_ID Id1,
    _In_ PDV_DISK_ID Id2
    )
{
    return PhEqualString(Id1->DevicePath, Id2->DevicePath, TRUE);
}

PDV_DISK_ENTRY CreateDiskEntry(
    _In_ PDV_DISK_ID Id
    )
{
    PDV_DISK_ENTRY entry;
    ULONG sampleCount;

    entry = PhCreateObjectZero(sizeof(DV_DISK_ENTRY), DiskDeviceEntryType);
    entry->DiskIndex = ULONG_MAX;
    CopyDiskId(&entry->Id, Id);

    sampleCount = PhGetIntegerSetting(SETTING_SAMPLE_COUNT);
    PhInitializeCircularBuffer_ULONG64(&entry->ReadBuffer, sampleCount);
    PhInitializeCircularBuffer_ULONG64(&entry->WriteBuffer, sampleCount);

    PhAcquireQueuedLockExclusive(&DiskDevicesListLock);
    PhAddItemList(DiskDevicesList, entry);
    PhReleaseQueuedLockExclusive(&DiskDevicesListLock);

    AddRemoveDeviceChangeCallback();

    return entry;
}
