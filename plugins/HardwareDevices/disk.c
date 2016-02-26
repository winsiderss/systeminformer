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

        if (NT_SUCCESS(DiskDriveCreateHandle(&deviceHandle, entry->Id.DevicePath)))
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
                entry->DevicePresent = FALSE;
            }

            // Delay the query for the disk name, index and type from startup to later at runtime.
            // Note: If the user opens the Sysinfo window before this has fired, 
            //   we have a second check in the UpdateDiskIndexText function that queries this information. 
            if (runCount > 1)
            {
                if (
                    !entry->DiskName ||
                    entry->DiskIndex == ULONG_MAX ||
                    entry->DiskType == 0
                    )
                {
                    if (!entry->DiskName)
                    {
                        DiskDriveQueryDeviceInformation(deviceHandle, NULL, &entry->DiskName, NULL, NULL);
                    }

                    if (entry->DiskIndex == ULONG_MAX || entry->DiskType == 0)
                    {
                        ULONG diskIndex = ULONG_MAX; // Note: Do not initialize to zero.
                        DEVICE_TYPE diskType = 0;

                        if (NT_SUCCESS(DiskDriveQueryDeviceTypeAndNumber(deviceHandle, &diskIndex, &diskType)))
                        {
                            entry->DiskIndex = diskIndex;
                            entry->DiskType = diskType;
                        }
                    }
                }
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

            // Reset the DiskIndex so we can re-query the index on the next interval update.
            entry->DiskIndex = ULONG_MAX;
            // Reset the DiskIndexName so we can re-query the name on the next interval update.
            PhClearReference(&entry->DiskIndexName);

            entry->DevicePresent = FALSE;
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

    return entry;
}