/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     dmex    2015-2020
 *
 */

#include "devices.h"

VOID DiskEntryDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PDV_DISK_ENTRY entry = Object;

    PhAcquireQueuedLockExclusive(&DiskDrivesListLock);
    PhRemoveItemList(DiskDrivesList, PhFindItemList(DiskDrivesList, entry));
    PhReleaseQueuedLockExclusive(&DiskDrivesListLock);

    DeleteDiskId(&entry->Id);
    PhClearReference(&entry->DiskName);

    PhDeleteCircularBuffer_ULONG64(&entry->ReadBuffer);
    PhDeleteCircularBuffer_ULONG64(&entry->WriteBuffer);

    AddRemoveDeviceChangeCallback();
}

VOID DiskDrivesInitialize(
    VOID
    )
{
    DiskDrivesList = PhCreateList(1);
    DiskDriveEntryType = PhCreateObjectType(L"DiskDriveEntry", 0, DiskEntryDeleteProcedure);
}

VOID DiskDrivesUpdate(
    VOID
    )
{
    static ULONG runCount = 0; // MUST keep in sync with runCount in process provider

    PhAcquireQueuedLockShared(&DiskDrivesListLock);

    for (ULONG i = 0; i < DiskDrivesList->Count; i++)
    {
        HANDLE deviceHandle;
        PDV_DISK_ENTRY entry;

        entry = PhReferenceObjectSafe(DiskDrivesList->Items[i]);

        if (!entry)
            continue;

        if (NT_SUCCESS(PhCreateFile(
            &deviceHandle,
            entry->Id.DevicePath,
            FILE_READ_ATTRIBUTES | SYNCHRONIZE,
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

            if (runCount > 1)
            {
                // Delay the first query for the disk name, index and type.
                //   1) This information is not needed until the user opens the sysinfo window.
                //   2) Try not to query this information while opening the sysinfo window (e.g. delay).
                //   3) Try not to query this information during startup (e.g. delay).
                //
                // Note: If the user opens the Sysinfo window before we query the disk info,
                // we have a second check in diskgraph.c that queries the information on demand.
                DiskDriveUpdateDeviceInfo(deviceHandle, entry);
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

        if (runCount != 0)
        {
            PhAddItemCircularBuffer_ULONG64(&entry->ReadBuffer, entry->BytesReadDelta.Delta);
            PhAddItemCircularBuffer_ULONG64(&entry->WriteBuffer, entry->BytesWrittenDelta.Delta);
        }

        PhDereferenceObjectDeferDelete(entry);
    }

    PhReleaseQueuedLockShared(&DiskDrivesListLock);

    runCount++;
}

VOID DiskDriveUpdateDeviceInfo(
    _In_opt_ HANDLE DeviceHandle,
    _In_ PDV_DISK_ENTRY DiskEntry
    )
{
    if (!DiskEntry->DiskName || DiskEntry->DiskIndex == ULONG_MAX)
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
                DiskEntry->Id.DevicePath,
                FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                );
        }

        if (deviceHandle)
        {
            if (!DiskEntry->DiskName)
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

    entry = PhCreateObject(sizeof(DV_DISK_ENTRY), DiskDriveEntryType);
    memset(entry, 0, sizeof(DV_DISK_ENTRY));

    entry->DiskIndex = ULONG_MAX;
    CopyDiskId(&entry->Id, Id);

    PhInitializeCircularBuffer_ULONG64(&entry->ReadBuffer, PhGetIntegerSetting(L"SampleCount"));
    PhInitializeCircularBuffer_ULONG64(&entry->WriteBuffer, PhGetIntegerSetting(L"SampleCount"));

    PhAcquireQueuedLockExclusive(&DiskDrivesListLock);
    PhAddItemList(DiskDrivesList, entry);
    PhReleaseQueuedLockExclusive(&DiskDrivesListLock);

    AddRemoveDeviceChangeCallback();

    return entry;
}
