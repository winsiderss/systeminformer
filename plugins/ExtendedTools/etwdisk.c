/*
 * Process Hacker Extended Tools -
 *   ETW disk monitoring
 *
 * Copyright (C) 2011 wj32
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

#include "exttools.h"
#include "etwmon.h"

typedef struct _ETP_DISK_PACKET
{
    SLIST_ENTRY ListEntry;
    ET_ETW_DISK_EVENT Event;
    PPH_STRING FileName;
} ETP_DISK_PACKET, *PETP_DISK_PACKET;

VOID NTAPI EtpDiskItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

BOOLEAN NTAPI EtpDiskHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

ULONG NTAPI EtpDiskHashtableHashFunction(
    _In_ PVOID Entry
    );

VOID NTAPI ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

BOOLEAN EtDiskEnabled = FALSE;

PPH_OBJECT_TYPE EtDiskItemType;
PPH_HASHTABLE EtDiskHashtable;
PH_QUEUED_LOCK EtDiskHashtableLock = PH_QUEUED_LOCK_INIT;
LIST_ENTRY EtDiskAgeListHead;
PH_CALLBACK_DECLARE(EtDiskItemAddedEvent);
PH_CALLBACK_DECLARE(EtDiskItemModifiedEvent);
PH_CALLBACK_DECLARE(EtDiskItemRemovedEvent);
PH_CALLBACK_DECLARE(EtDiskItemsUpdatedEvent);

PH_FREE_LIST EtDiskPacketFreeList;
SLIST_HEADER EtDiskPacketListHead;
PPH_HASHTABLE EtFileNameHashtable;
PH_QUEUED_LOCK EtFileNameHashtableLock = PH_QUEUED_LOCK_INIT;

static LARGE_INTEGER EtpPerformanceFrequency;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;

VOID EtInitializeDiskInformation(
    VOID
    )
{
    LARGE_INTEGER performanceCounter;

    EtDiskItemType = PhCreateObjectType(L"DiskItem", 0, EtpDiskItemDeleteProcedure);
    EtDiskHashtable = PhCreateHashtable(
        sizeof(PET_DISK_ITEM),
        EtpDiskHashtableCompareFunction,
        EtpDiskHashtableHashFunction,
        128
        );
    InitializeListHead(&EtDiskAgeListHead);

    PhInitializeFreeList(&EtDiskPacketFreeList, sizeof(ETP_DISK_PACKET), 64);
    RtlInitializeSListHead(&EtDiskPacketListHead);
    EtFileNameHashtable = PhCreateSimpleHashtable(128);

    NtQueryPerformanceCounter(&performanceCounter, &EtpPerformanceFrequency);

    EtDiskEnabled = TRUE;

    // Collect all existing file names.
    EtStartEtwRundown();

    PhRegisterCallback(
        &PhProcessesUpdatedEvent,
        ProcessesUpdatedCallback,
        NULL,
        &ProcessesUpdatedCallbackRegistration
        );
}

PET_DISK_ITEM EtCreateDiskItem(
    VOID
    )
{
    PET_DISK_ITEM diskItem;

    diskItem = PhCreateObject(sizeof(ET_DISK_ITEM), EtDiskItemType);
    memset(diskItem, 0, sizeof(ET_DISK_ITEM));

    return diskItem;
}

VOID NTAPI EtpDiskItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PET_DISK_ITEM diskItem = Object;

    if (diskItem->FileName) PhDereferenceObject(diskItem->FileName);
    if (diskItem->FileNameWin32) PhDereferenceObject(diskItem->FileNameWin32);
    if (diskItem->ProcessName) PhDereferenceObject(diskItem->ProcessName);
    if (diskItem->ProcessIcon) EtProcIconDereferenceProcessIcon(diskItem->ProcessIcon);
    if (diskItem->ProcessRecord) PhDereferenceProcessRecord(diskItem->ProcessRecord);
}

BOOLEAN NTAPI EtpDiskHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PET_DISK_ITEM diskItem1 = *(PET_DISK_ITEM *)Entry1;
    PET_DISK_ITEM diskItem2 = *(PET_DISK_ITEM *)Entry2;

    return diskItem1->ProcessId == diskItem2->ProcessId && PhEqualString(diskItem1->FileName, diskItem2->FileName, TRUE);
}

ULONG NTAPI EtpDiskHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PET_DISK_ITEM diskItem = *(PET_DISK_ITEM *)Entry;

    return (HandleToUlong(diskItem->ProcessId) / 4) ^ PhHashStringRef(&diskItem->FileName->sr, TRUE);
}

PET_DISK_ITEM EtReferenceDiskItem(
    _In_ HANDLE ProcessId,
    _In_ PPH_STRING FileName
    )
{
    ET_DISK_ITEM lookupDiskItem;
    PET_DISK_ITEM lookupDiskItemPtr = &lookupDiskItem;
    PET_DISK_ITEM *diskItemPtr;
    PET_DISK_ITEM diskItem;

    lookupDiskItem.ProcessId = ProcessId;
    lookupDiskItem.FileName = FileName;

    PhAcquireQueuedLockShared(&EtDiskHashtableLock);

    diskItemPtr = (PET_DISK_ITEM *)PhFindEntryHashtable(
        EtDiskHashtable,
        &lookupDiskItemPtr
        );

    if (diskItemPtr)
        PhSetReference(&diskItem, *diskItemPtr);
    else
        diskItem = NULL;

    PhReleaseQueuedLockShared(&EtDiskHashtableLock);

    return diskItem;
}

VOID EtpRemoveDiskItem(
    _In_ PET_DISK_ITEM DiskItem
    )
{
    RemoveEntryList(&DiskItem->AgeListEntry);
    PhRemoveEntryHashtable(EtDiskHashtable, &DiskItem);
    PhDereferenceObject(DiskItem);
}

VOID EtDiskProcessDiskEvent(
    _In_ PET_ETW_DISK_EVENT Event
    )
{
    PETP_DISK_PACKET packet;

    if (!EtDiskEnabled)
        return;

    packet = PhAllocateFromFreeList(&EtDiskPacketFreeList);
    memcpy(&packet->Event, Event, sizeof(ET_ETW_DISK_EVENT));
    packet->FileName = EtFileObjectToFileName(Event->FileObject);
    RtlInterlockedPushEntrySList(&EtDiskPacketListHead, &packet->ListEntry);
}

VOID EtDiskProcessFileEvent(
    _In_ PET_ETW_FILE_EVENT Event
    )
{
    PH_KEY_VALUE_PAIR pair;
    PPH_KEY_VALUE_PAIR realPair;

    if (!EtDiskEnabled)
        return;

    if (Event->Type == EtEtwFileCreateType || Event->Type == EtEtwFileRundownType)
    {
        pair.Key = Event->FileObject;
        pair.Value = NULL;

        PhAcquireQueuedLockExclusive(&EtFileNameHashtableLock);

        realPair = PhAddEntryHashtableEx(EtFileNameHashtable, &pair, NULL);
        PhMoveReference(&realPair->Value, PhCreateString2(&Event->FileName));

        PhReleaseQueuedLockExclusive(&EtFileNameHashtableLock);
    }
    else if (Event->Type == EtEtwFileDeleteType)
    {
        pair.Key = Event->FileObject;

        PhAcquireQueuedLockExclusive(&EtFileNameHashtableLock);

        realPair = PhFindEntryHashtable(EtFileNameHashtable, &pair);

        if (realPair)
        {
            PhDereferenceObject(realPair->Value);
            PhRemoveEntryHashtable(EtFileNameHashtable, &pair);
        }

        PhReleaseQueuedLockExclusive(&EtFileNameHashtableLock);
    }
}

PPH_STRING EtFileObjectToFileName(
    _In_ PVOID FileObject
    )
{
    PH_KEY_VALUE_PAIR pair;
    PPH_KEY_VALUE_PAIR realPair;
    PPH_STRING fileName;

    pair.Key = FileObject;
    fileName = NULL;

    PhAcquireQueuedLockShared(&EtFileNameHashtableLock);

    realPair = PhFindEntryHashtable(EtFileNameHashtable, &pair);

    if (realPair)
        PhSetReference(&fileName, realPair->Value);

    PhReleaseQueuedLockShared(&EtFileNameHashtableLock);

    return fileName;
}

VOID EtpProcessDiskPacket(
    _In_ PETP_DISK_PACKET Packet,
    _In_ ULONG RunId
    )
{
    PET_ETW_DISK_EVENT diskEvent;
    PET_DISK_ITEM diskItem;
    BOOLEAN added = FALSE;

    diskEvent = &Packet->Event;

    // We only process non-zero read/write events.
    if (diskEvent->Type != EtEtwDiskReadType && diskEvent->Type != EtEtwDiskWriteType)
        return;
    if (diskEvent->TransferSize == 0)
        return;

    // Ignore packets with no file name - this is useless to the user.
    if (!Packet->FileName)
        return;

    diskItem = EtReferenceDiskItem(diskEvent->ClientId.UniqueProcess, Packet->FileName);

    if (!diskItem)
    {
        PPH_PROCESS_ITEM processItem;

        // Disk item not found (or the address was re-used), create it.

        diskItem = EtCreateDiskItem();

        diskItem->ProcessId = diskEvent->ClientId.UniqueProcess;
        PhSetReference(&diskItem->FileName, Packet->FileName);
        diskItem->FileNameWin32 = PhGetFileName(diskItem->FileName);

        if (processItem = PhReferenceProcessItem(diskItem->ProcessId))
        {
            PhSetReference(&diskItem->ProcessName, processItem->ProcessName);
            diskItem->ProcessIcon = EtProcIconReferenceSmallProcessIcon(EtGetProcessBlock(processItem));
            diskItem->ProcessRecord = processItem->Record;
            PhReferenceProcessRecord(diskItem->ProcessRecord);

            PhDereferenceObject(processItem);
        }

        // Add the disk item to the age list.
        diskItem->AddTime = RunId;
        diskItem->FreshTime = RunId;
        InsertHeadList(&EtDiskAgeListHead, &diskItem->AgeListEntry);

        // Add the disk item to the hashtable.
        PhAcquireQueuedLockExclusive(&EtDiskHashtableLock);
        PhAddEntryHashtable(EtDiskHashtable, &diskItem);
        PhReleaseQueuedLockExclusive(&EtDiskHashtableLock);

        // Raise the disk item added event.
        PhInvokeCallback(&EtDiskItemAddedEvent, diskItem);
        added = TRUE;
    }

    // The I/O priority number needs to be decoded.

    diskItem->IoPriority = (diskEvent->IrpFlags >> 17) & 7;

    if (diskItem->IoPriority == 0)
        diskItem->IoPriority = IoPriorityNormal;
    else
        diskItem->IoPriority--;

    // Accumulate statistics for this update period.

    if (diskEvent->Type == EtEtwDiskReadType)
        diskItem->ReadDelta += diskEvent->TransferSize;
    else
        diskItem->WriteDelta += diskEvent->TransferSize;

    if (EtpPerformanceFrequency.QuadPart != 0)
    {
        // Convert the response time to milliseconds.
        diskItem->ResponseTimeTotal += (FLOAT)diskEvent->HighResResponseTime * 1000 / EtpPerformanceFrequency.QuadPart;
        diskItem->ResponseTimeCount++;
    }

    if (!added)
    {
        if (diskItem->FreshTime != RunId)
        {
            diskItem->FreshTime = RunId;
            RemoveEntryList(&diskItem->AgeListEntry);
            InsertHeadList(&EtDiskAgeListHead, &diskItem->AgeListEntry);
        }

        PhDereferenceObject(diskItem);
    }
}

ULONG64 EtpCalculateAverage(
    _In_ PULONG64 Buffer,
    _In_ ULONG BufferSize,
    _In_ ULONG BufferPosition,
    _In_ ULONG BufferCount,
    _In_ ULONG NumberToConsider
    )
{
    ULONG64 sum;
    ULONG i;
    ULONG count;

    sum = 0;
    i = BufferPosition;

    if (NumberToConsider > BufferCount)
        NumberToConsider = BufferCount;

    if (NumberToConsider == 0)
        return 0;

    count = NumberToConsider;

    do
    {
        sum += Buffer[i];
        i++;

        if (i == BufferSize)
            i = 0;
    } while (--count != 0);

    return sum / NumberToConsider;
}

static VOID NTAPI ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    static ULONG runCount = 0;

    PSLIST_ENTRY listEntry;
    PLIST_ENTRY ageListEntry;

    // Process incoming disk event packets.

    listEntry = RtlInterlockedFlushSList(&EtDiskPacketListHead);

    while (listEntry)
    {
        PETP_DISK_PACKET packet;

        packet = CONTAINING_RECORD(listEntry, ETP_DISK_PACKET, ListEntry);
        listEntry = listEntry->Next;

        EtpProcessDiskPacket(packet, runCount);

        if (packet->FileName)
            PhDereferenceObject(packet->FileName);

        PhFreeToFreeList(&EtDiskPacketFreeList, packet);
    }

    // Remove old entries.

    ageListEntry = EtDiskAgeListHead.Blink;

    while (ageListEntry != &EtDiskAgeListHead)
    {
        PET_DISK_ITEM diskItem;

        diskItem = CONTAINING_RECORD(ageListEntry, ET_DISK_ITEM, AgeListEntry);
        ageListEntry = ageListEntry->Blink;

        if (runCount - diskItem->FreshTime < HISTORY_SIZE) // must compare like this to avoid overflow/underflow problems
            break;

        PhInvokeCallback(&EtDiskItemRemovedEvent, diskItem);

        PhAcquireQueuedLockExclusive(&EtDiskHashtableLock);
        EtpRemoveDiskItem(diskItem);
        PhReleaseQueuedLockExclusive(&EtDiskHashtableLock);
    }

    // Update existing items.

    ageListEntry = EtDiskAgeListHead.Flink;

    while (ageListEntry != &EtDiskAgeListHead)
    {
        PET_DISK_ITEM diskItem;

        diskItem = CONTAINING_RECORD(ageListEntry, ET_DISK_ITEM, AgeListEntry);

        // Update statistics.

        if (diskItem->HistoryPosition != 0)
            diskItem->HistoryPosition--;
        else
            diskItem->HistoryPosition = HISTORY_SIZE - 1;

        diskItem->ReadHistory[diskItem->HistoryPosition] = diskItem->ReadDelta;
        diskItem->WriteHistory[diskItem->HistoryPosition] = diskItem->WriteDelta;

        if (diskItem->HistoryCount < HISTORY_SIZE)
            diskItem->HistoryCount++;

        if (diskItem->ResponseTimeCount != 0)
        {
            diskItem->ResponseTimeAverage = (FLOAT)diskItem->ResponseTimeTotal / diskItem->ResponseTimeCount;

            // Reset the total once in a while to avoid the number getting too large (and thus losing precision).
            if (diskItem->ResponseTimeCount >= 1000)
            {
                diskItem->ResponseTimeTotal = diskItem->ResponseTimeAverage;
                diskItem->ResponseTimeCount = 1;
            }
        }

        diskItem->ReadTotal += diskItem->ReadDelta;
        diskItem->WriteTotal += diskItem->WriteDelta;
        diskItem->ReadDelta = 0;
        diskItem->WriteDelta = 0;
        diskItem->ReadAverage = EtpCalculateAverage(diskItem->ReadHistory, HISTORY_SIZE, diskItem->HistoryPosition, diskItem->HistoryCount, HISTORY_SIZE);
        diskItem->WriteAverage = EtpCalculateAverage(diskItem->WriteHistory, HISTORY_SIZE, diskItem->HistoryPosition, diskItem->HistoryCount, HISTORY_SIZE);

        if (diskItem->AddTime != runCount)
        {
            BOOLEAN modified = FALSE;
            PPH_PROCESS_ITEM processItem;

            if (!diskItem->ProcessName || !diskItem->ProcessIcon || !diskItem->ProcessRecord)
            {
                if (processItem = PhReferenceProcessItem(diskItem->ProcessId))
                {
                    if (!diskItem->ProcessName)
                    {
                        PhSetReference(&diskItem->ProcessName, processItem->ProcessName);
                        modified = TRUE;
                    }

                    if (!diskItem->ProcessIcon)
                    {
                        diskItem->ProcessIcon = EtProcIconReferenceSmallProcessIcon(EtGetProcessBlock(processItem));

                        if (diskItem->ProcessIcon)
                            modified = TRUE;
                    }

                    if (!diskItem->ProcessRecord)
                    {
                        diskItem->ProcessRecord = processItem->Record;
                        PhReferenceProcessRecord(diskItem->ProcessRecord);
                    }

                    PhDereferenceObject(processItem);
                }
            }

            if (modified)
            {
                // Raise the disk item modified event.
                PhInvokeCallback(&EtDiskItemModifiedEvent, diskItem);
            }
        }

        ageListEntry = ageListEntry->Flink;
    }

    PhInvokeCallback(&EtDiskItemsUpdatedEvent, NULL);
    runCount++;
}
