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

static VOID DiskEntryDeleteProcedure(
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
        HANDLE deviceHandle = NULL;
        PDV_DISK_ENTRY entry;

        entry = PhReferenceObjectSafe(DiskDrivesList->Items[i]);

        if (!entry)
            continue;

        if (NT_SUCCESS(DiskDriveCreateHandle(
            &deviceHandle, 
            entry->Id.DevicePath
            )))
        {
            DISK_PERFORMANCE diskPerformance;

            if (NT_SUCCESS(DiskDriveQueryStatistics(
                deviceHandle,
                &diskPerformance
                )))
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
            }

            // HACK: Pull the Disk name from the current query.
            if (!entry->DiskName)
            {
                DiskDriveQueryDeviceInformation(deviceHandle, NULL, &entry->DiskName, NULL, NULL);
            }

            if (!entry->HaveDiskIndex)
            {
                entry->HaveDiskIndex = NT_SUCCESS(DiskDriveQueryDeviceTypeAndNumber(deviceHandle, &entry->DiskIndex, NULL));
            }

            if (entry->HaveFirstSample)
            {
                PhAddItemCircularBuffer_ULONG64(&entry->ReadBuffer, entry->BytesReadDelta.Delta);
                PhAddItemCircularBuffer_ULONG64(&entry->WriteBuffer, entry->BytesWrittenDelta.Delta);
            }
            else
            {
                PhAddItemCircularBuffer_ULONG64(&entry->ReadBuffer, 0);
                PhAddItemCircularBuffer_ULONG64(&entry->WriteBuffer, 0);
                entry->HaveFirstSample = TRUE;
            }

            NtClose(deviceHandle);
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
    Id->DevicePath = DevicePath;
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
    NOTHING;
}

BOOLEAN EquivalentDiskId(
    _In_ PDV_DISK_ID Id1,
    _In_ PDV_DISK_ID Id2
    )
{
    if (PhEqualString(Id1->DevicePath, Id2->DevicePath, TRUE))
        return TRUE;

    return FALSE;
}

PDV_DISK_ENTRY CreateDiskEntry(
    _In_ PDV_DISK_ID Id
    )
{
    PDV_DISK_ENTRY entry;

    entry = PhCreateObject(sizeof(DV_DISK_ENTRY), DiskDriveEntryType);
    memset(entry, 0, sizeof(DV_DISK_ENTRY));

    CopyDiskId(&entry->Id, Id);

    PhInitializeCircularBuffer_ULONG64(&entry->ReadBuffer, PhGetIntegerSetting(L"SampleCount"));
    PhInitializeCircularBuffer_ULONG64(&entry->WriteBuffer, PhGetIntegerSetting(L"SampleCount"));

    PhAcquireQueuedLockExclusive(&DiskDrivesListLock);
    PhAddItemList(DiskDrivesList, entry);
    PhReleaseQueuedLockExclusive(&DiskDrivesListLock);

    return entry;
}