/*
 * Process Hacker Plugins -
 *   Network Adapters Plugin
 *
 * Copyright (C) 2015-2016 dmex
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

#include "netadapters.h"

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
    PhClearReference(&entry->AdapterName);

    PhDeleteCircularBuffer_ULONG64(&entry->ReadBuffer);
    PhDeleteCircularBuffer_ULONG64(&entry->WriteBuffer);
}

VOID DiskInitialize(
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
        ULONG64 diskBytesReadOctets = 0;
        ULONG64 diskBytesWrittenOctets = 0;
        ULONG64 diskBytesRead = 0;
        ULONG64 diskBytesWritten = 0;

        entry = PhReferenceObjectSafe(DiskDrivesList->Items[i]);

        if (!entry)
            continue;

        if (NT_SUCCESS(PhCreateFileWin32(
            &deviceHandle,
            PhaFormatString(L"\\\\.\\PhysicalDrive%lu", entry->Id.DiskIndex)->Buffer,
            FILE_READ_ATTRIBUTES | SYNCHRONIZE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            DISK_PERFORMANCE diskPerformance;

            if (NT_SUCCESS(DiskDriveQueryStatistics(
                deviceHandle,
                &diskPerformance
                )))
            {
                ULONG64 ReadTime = diskPerformance.ReadTime.QuadPart - entry->LastReadTime;
                ULONG64 WriteTime = diskPerformance.WriteTime.QuadPart - entry->LastWriteTime;
                ULONG64 IdleTime = diskPerformance.IdleTime.QuadPart - entry->LastIdletime;
                ULONG64 QueryTime = diskPerformance.QueryTime.QuadPart - entry->LastQueryTime;

                diskBytesReadOctets = diskPerformance.BytesRead.QuadPart;
                diskBytesWrittenOctets = diskPerformance.BytesWritten.QuadPart;
                diskBytesRead = diskBytesReadOctets - entry->LastBytesReadValue;
                diskBytesWritten = diskBytesWrittenOctets - entry->LastBytesWriteValue;

                if (QueryTime != 0)
                {
                    // TODO: check math... Task Manager is a lot more accurate and has better precision.
                    entry->ResponseTime = (ReadTime + WriteTime / QueryTime) / PH_TICKS_PER_MS; // ReadTime + WriteTime + IdleTime
                    entry->ActiveTime = (FLOAT)(QueryTime - IdleTime) / QueryTime * 100;
                }
                else
                {
                    // TODO: If querytime is 0 then enough time hasn't passed to calculate results?
                    entry->ResponseTime = 0;
                    entry->ActiveTime = 0.0f;
                }

                if (entry->ActiveTime > 100.f)
                    entry->ActiveTime = 0.f;
                if (entry->ActiveTime < 0.f)
                    entry->ActiveTime = 0.f;

                entry->LastReadTime = diskPerformance.ReadTime.QuadPart;
                entry->LastWriteTime = diskPerformance.WriteTime.QuadPart;
                entry->LastIdletime = diskPerformance.IdleTime.QuadPart;
                entry->LastQueryTime = diskPerformance.QueryTime.QuadPart;
                entry->QueueDepth = diskPerformance.QueueDepth;
                entry->SplitCount = diskPerformance.SplitCount;
            }

            // HACK: Pull the Disk name from the current query.
            if (!entry->AdapterName)
            {
                DiskDriveQueryDeviceInformation(deviceHandle, NULL, &entry->AdapterName, NULL, NULL);
            }

            if (!entry->HaveFirstSample)
            {
                diskBytesRead = 0;
                diskBytesWritten = 0;
                entry->HaveFirstSample = TRUE;
            }

            PhAddItemCircularBuffer_ULONG64(&entry->ReadBuffer, diskBytesRead);
            PhAddItemCircularBuffer_ULONG64(&entry->WriteBuffer, diskBytesWritten);

            entry->BytesReadValue = diskBytesRead;
            entry->BytesWriteValue = diskBytesWritten;
            entry->LastBytesReadValue = diskBytesReadOctets;
            entry->LastBytesWriteValue = diskBytesWrittenOctets;

            NtClose(deviceHandle);
        }

        PhDereferenceObjectDeferDelete(entry);
    }

    PhReleaseQueuedLockShared(&DiskDrivesListLock);

    runCount++;
}

VOID InitializeDiskId(
    _Out_ PDV_DISK_ID Id,
    _In_ ULONG DiskIndex
    )
{
    Id->DiskIndex = DiskIndex;
}

VOID CopyDiskId(
    _Out_ PDV_DISK_ID Destination,
    _In_ PDV_DISK_ID Source
    )
{
    InitializeDiskId(
        Destination,
        Source->DiskIndex
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
    if (Id1->DiskIndex == Id2->DiskIndex)
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