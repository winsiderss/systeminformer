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
    SINGLE_LIST_ENTRY ListEntry;
    ET_ETW_DISK_EVENT Event;
    PPH_STRING FileName;
} ETP_DISK_PACKET, *PETP_DISK_PACKET;

VOID NTAPI EtpDiskItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

BOOLEAN NTAPI EtpDiskHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    );

ULONG NTAPI EtpDiskHashtableHashFunction(
    __in PVOID Entry
    );

VOID NTAPI ProcessesUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
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

VOID EtInitializeDiskInformation()
{
    NTSTATUS status;
    LARGE_INTEGER performanceCounter;

    if (!NT_SUCCESS(status = PhCreateObjectType(
        &EtDiskItemType,
        L"DiskItem",
        0,
        EtpDiskItemDeleteProcedure
        )))
        PhRaiseStatus(status);

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

PET_DISK_ITEM EtCreateDiskItem()
{
    PET_DISK_ITEM diskItem;

    if (!NT_SUCCESS(PhCreateObject(
        &diskItem,
        sizeof(ET_DISK_ITEM),
        0,
        EtDiskItemType
        )))
        return NULL;

    memset(diskItem, 0, sizeof(ET_DISK_ITEM));

    return diskItem;
}

VOID NTAPI EtpDiskItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PET_DISK_ITEM diskItem = Object;

    if (diskItem->FileName) PhDereferenceObject(diskItem->FileName);
    if (diskItem->FileNameWin32) PhDereferenceObject(diskItem->FileNameWin32);
    if (diskItem->ProcessName) PhDereferenceObject(diskItem->ProcessName);
    if (diskItem->ProcessIcon) EtProcIconDereferenceProcessIcon(diskItem->ProcessIcon);
}

// Copied from srvprv.c
/**
 * Generates a hash code for a string, case-insensitive.
 *
 * \param String The string.
 * \param Count The number of characters to hash.
 */
FORCEINLINE ULONG EtpHashStringIgnoreCase(
    __in PWSTR String,
    __in ULONG Count
    )
{
    ULONG hash = Count;

    if (Count == 0)
        return 0;

    do
    {
        hash = RtlUpcaseUnicodeChar(*String) + (hash << 6) + (hash << 16) - hash;
        String++;
    } while (--Count != 0);

    return hash;
}

BOOLEAN NTAPI EtpDiskHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    PET_DISK_ITEM diskItem1 = *(PET_DISK_ITEM *)Entry1;
    PET_DISK_ITEM diskItem2 = *(PET_DISK_ITEM *)Entry2;

    return diskItem1->ProcessId == diskItem2->ProcessId && PhEqualString(diskItem1->FileName, diskItem2->FileName, TRUE);
}

ULONG NTAPI EtpDiskHashtableHashFunction(
    __in PVOID Entry
    )
{
    PET_DISK_ITEM diskItem = *(PET_DISK_ITEM *)Entry;

    return ((ULONG)diskItem->ProcessId / 4) ^ EtpHashStringIgnoreCase(diskItem->FileName->Buffer, diskItem->FileName->Length / sizeof(WCHAR));
}

PET_DISK_ITEM EtReferenceDiskItem(
    __in HANDLE ProcessId,
    __in PPH_STRING FileName
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
    {
        diskItem = *diskItemPtr;
        PhReferenceObject(diskItem);
    }
    else
    {
        diskItem = NULL;
    }

    PhReleaseQueuedLockShared(&EtDiskHashtableLock);

    return diskItem;
}

__assumeLocked VOID EtpRemoveDiskItem(
    __in PET_DISK_ITEM DiskItem
    )
{
    RemoveEntryList(&DiskItem->AgeListEntry);
    PhRemoveEntryHashtable(EtDiskHashtable, &DiskItem);
    PhDereferenceObject(DiskItem);
}

VOID EtDiskProcessDiskEvent(
    __in PET_ETW_DISK_EVENT Event
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
    __in PET_ETW_FILE_EVENT Event
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
        PhSwapReference2(&realPair->Value, PhCreateStringEx(Event->FileName.Buffer, Event->FileName.Length));

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
    __in PVOID FileObject
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
    {
        fileName = realPair->Value;
        PhReferenceObject(fileName);
    }

    PhReleaseQueuedLockShared(&EtFileNameHashtableLock);

    return fileName;
}

VOID EtpProcessDiskPacket(
    __in PETP_DISK_PACKET Packet,
    __in ULONG RunId
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
        diskItem->FileName = Packet->FileName;
        PhReferenceObject(Packet->FileName);
        diskItem->FileNameWin32 = PhGetFileName(diskItem->FileName);

        if (processItem = PhReferenceProcessItem(diskItem->ProcessId))
        {
            diskItem->ProcessName = processItem->ProcessName;
            PhReferenceObject(processItem->ProcessName);

            diskItem->ProcessIcon = EtProcIconReferenceSmallProcessIcon(EtGetProcessBlock(processItem));

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
    __in PULONG64 Buffer,
    __in ULONG BufferSize,
    __in ULONG BufferPosition,
    __in ULONG BufferCount,
    __in ULONG NumberToConsider
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
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    static ULONG runCount = 0;

    PSINGLE_LIST_ENTRY listEntry;
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
            if (diskItem->ResponseTimeCount == 1000)
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

            if (!diskItem->ProcessName || !diskItem->ProcessIcon)
            {
                if (processItem = PhReferenceProcessItem(diskItem->ProcessId))
                {
                    if (!diskItem->ProcessName)
                    {
                        diskItem->ProcessName = processItem->ProcessName;
                        PhReferenceObject(processItem->ProcessName);
                        modified = TRUE;
                    }

                    if (!diskItem->ProcessIcon)
                    {
                        diskItem->ProcessIcon = EtProcIconReferenceSmallProcessIcon(EtGetProcessBlock(processItem));

                        if (diskItem->ProcessIcon)
                            modified = TRUE;
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
