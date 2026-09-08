/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include "agenttools.h"

// Snapshot identity and change tracking. Every read carries the snapshot id of the provider run it
// came from; a client that kept the id from its last list can ask what changed since, instead of
// diffing two full listings itself.
//
// The id is the process provider's own run count, and service changes are stamped with the id
// current when the service provider saw them: both providers run on the same interval, so one id
// space describes the whole cache.
//
// Live entries are bounded by the machine (a few hundred processes, a few hundred services) and
// removed ones by AT_SNAPSHOT_REMOVED_LIMIT, so the lists stay small enough that a linear scan is
// cheaper than a hash table would be to maintain.

#define AT_SNAPSHOT_REMOVED_LIMIT 256

typedef struct _AT_CHANGE
{
    // Processes are identified by their boot-unique sequence number, services by name.
    ULONG64 SequenceNumber;
    HANDLE ProcessId;
    PPH_STRING Name;

    ULONG AddedId;
    ULONG ModifiedId;
    ULONG RemovedId;
} AT_CHANGE, *PAT_CHANGE;

static PH_QUEUED_LOCK AtSnapshotLock = PH_QUEUED_LOCK_INIT;
static LONG AtSnapshotId = 0;
static LONG AtUpdateInterval = 1000;
static ULONG AtSnapshotTrackingSinceId = 0;
static ULONG AtSnapshotOldestForgottenId = 0;
static PPH_LIST AtProcessChanges = NULL;
static PPH_LIST AtServiceChanges = NULL;

static PH_CALLBACK_REGISTRATION AtProcessAddedRegistration;
static PH_CALLBACK_REGISTRATION AtProcessModifiedRegistration;
static PH_CALLBACK_REGISTRATION AtProcessRemovedRegistration;
static PH_CALLBACK_REGISTRATION AtProcessUpdatedRegistration;
static PH_CALLBACK_REGISTRATION AtServiceAddedRegistration;
static PH_CALLBACK_REGISTRATION AtServiceModifiedRegistration;
static PH_CALLBACK_REGISTRATION AtServiceRemovedRegistration;

ULONG AtGetSnapshotId(
    VOID
    )
{
    return (ULONG)ReadAcquire(&AtSnapshotId);
}

ULONG AtGetUpdateInterval(
    VOID
    )
{
    return (ULONG)ReadAcquire(&AtUpdateInterval);
}

// The id of the run in progress. A provider raises its item events during a run and publishes the
// run count only at the end of it, so a change seen now belongs to the run about to be published:
// stamping it with the last published id would hide it from a client holding that id.
ULONG AtpChangeId(
    VOID
    )
{
    return AtGetSnapshotId() + 1;
}

// Lock held.
PAT_CHANGE AtpFindProcessChange(
    _In_ ULONG64 SequenceNumber
    )
{
    ULONG i;

    for (i = 0; i < AtProcessChanges->Count; i++)
    {
        PAT_CHANGE change = AtProcessChanges->Items[i];

        if (change->SequenceNumber == SequenceNumber)
            return change;
    }

    return NULL;
}

// Lock held.
PAT_CHANGE AtpFindServiceChange(
    _In_ PPH_STRING Name
    )
{
    ULONG i;

    for (i = 0; i < AtServiceChanges->Count; i++)
    {
        PAT_CHANGE change = AtServiceChanges->Items[i];

        if (PhEqualString(change->Name, Name, TRUE))
            return change;
    }

    return NULL;
}

VOID AtpFreeChange(
    _In_ PAT_CHANGE Change
    )
{
    PhClearReference(&Change->Name);
    PhFree(Change);
}

// Lock held. Keeps the newest removals only; what falls off tells a later caller that an answer
// covering that id can no longer be complete.
VOID AtpTrimRemoved(
    _Inout_ PPH_LIST List
    )
{
    ULONG removed = 0;
    ULONG i;

    for (i = 0; i < List->Count; i++)
    {
        PAT_CHANGE change = List->Items[i];

        if (change->RemovedId != 0)
            removed++;
    }

    while (removed > AT_SNAPSHOT_REMOVED_LIMIT)
    {
        PAT_CHANGE oldest = NULL;
        ULONG oldestIndex = 0;

        for (i = 0; i < List->Count; i++)
        {
            PAT_CHANGE change = List->Items[i];

            if (change->RemovedId == 0)
                continue;

            if (!oldest || change->RemovedId < oldest->RemovedId)
            {
                oldest = change;
                oldestIndex = i;
            }
        }

        if (!oldest)
            break;

        if (oldest->RemovedId > AtSnapshotOldestForgottenId)
            AtSnapshotOldestForgottenId = oldest->RemovedId;

        PhRemoveItemList(List, oldestIndex);
        AtpFreeChange(oldest);
        removed--;
    }
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI AtpProcessAddedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = Parameter;
    PAT_CHANGE change;

    if (!processItem)
        return;

    PhAcquireQueuedLockExclusive(&AtSnapshotLock);

    // A recycled sequence number cannot happen, but a re-add of one we still hold as removed can:
    // reuse the entry so the process is not both added and removed.
    if (!(change = AtpFindProcessChange(processItem->ProcessSequenceNumber)))
    {
        change = PhAllocateZero(sizeof(AT_CHANGE));
        change->SequenceNumber = processItem->ProcessSequenceNumber;
        PhAddItemList(AtProcessChanges, change);
    }

    change->ProcessId = processItem->ProcessId;
    PhSwapReference(&change->Name, processItem->ProcessName);
    change->AddedId = AtpChangeId();
    change->ModifiedId = 0;
    change->RemovedId = 0;

    PhReleaseQueuedLockExclusive(&AtSnapshotLock);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI AtpProcessModifiedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = Parameter;
    PAT_CHANGE change;

    if (!processItem)
        return;

    PhAcquireQueuedLockExclusive(&AtSnapshotLock);

    if (change = AtpFindProcessChange(processItem->ProcessSequenceNumber))
        change->ModifiedId = AtpChangeId();

    PhReleaseQueuedLockExclusive(&AtSnapshotLock);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI AtpProcessRemovedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = Parameter;
    PAT_CHANGE change;

    if (!processItem)
        return;

    PhAcquireQueuedLockExclusive(&AtSnapshotLock);

    if (!(change = AtpFindProcessChange(processItem->ProcessSequenceNumber)))
    {
        // Present before tracking started, so it was never added on our watch.
        change = PhAllocateZero(sizeof(AT_CHANGE));
        change->SequenceNumber = processItem->ProcessSequenceNumber;
        change->ProcessId = processItem->ProcessId;
        PhSetReference(&change->Name, processItem->ProcessName);
        PhAddItemList(AtProcessChanges, change);
    }

    change->RemovedId = AtpChangeId();
    AtpTrimRemoved(AtProcessChanges);

    PhReleaseQueuedLockExclusive(&AtSnapshotLock);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI AtpProcessUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PROVIDER_UPDATED_EVENT event = Parameter;

    if (!event)
        return;

    WriteRelease(&AtSnapshotId, (LONG)event->RunCount);

    // The interval the history buffers are sampled at; a sample index is that many milliseconds
    // further into the past.
    if (event->UpdateInterval != 0)
        WriteRelease(&AtUpdateInterval, (LONG)event->UpdateInterval);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI AtpServiceAddedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_SERVICE_ITEM serviceItem = Parameter;
    PAT_CHANGE change;

    if (!serviceItem || !serviceItem->Name)
        return;

    PhAcquireQueuedLockExclusive(&AtSnapshotLock);

    if (!(change = AtpFindServiceChange(serviceItem->Name)))
    {
        change = PhAllocateZero(sizeof(AT_CHANGE));
        PhSetReference(&change->Name, serviceItem->Name);
        PhAddItemList(AtServiceChanges, change);
    }

    change->AddedId = AtpChangeId();
    change->ModifiedId = 0;
    change->RemovedId = 0;

    PhReleaseQueuedLockExclusive(&AtSnapshotLock);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI AtpServiceModifiedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_SERVICE_MODIFIED_DATA data = Parameter;
    PAT_CHANGE change;

    if (!data || !data->ServiceItem || !data->ServiceItem->Name)
        return;

    PhAcquireQueuedLockExclusive(&AtSnapshotLock);

    if (change = AtpFindServiceChange(data->ServiceItem->Name))
        change->ModifiedId = AtpChangeId();

    PhReleaseQueuedLockExclusive(&AtSnapshotLock);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI AtpServiceRemovedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_SERVICE_ITEM serviceItem = Parameter;
    PAT_CHANGE change;

    if (!serviceItem || !serviceItem->Name)
        return;

    PhAcquireQueuedLockExclusive(&AtSnapshotLock);

    if (!(change = AtpFindServiceChange(serviceItem->Name)))
    {
        change = PhAllocateZero(sizeof(AT_CHANGE));
        PhSetReference(&change->Name, serviceItem->Name);
        PhAddItemList(AtServiceChanges, change);
    }

    change->RemovedId = AtpChangeId();
    AtpTrimRemoved(AtServiceChanges);

    PhReleaseQueuedLockExclusive(&AtSnapshotLock);
}

VOID AtSnapshotInitialize(
    VOID
    )
{
    AtProcessChanges = PhCreateList(512);
    AtServiceChanges = PhCreateList(512);
    AtSnapshotTrackingSinceId = AtGetSnapshotId();

    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackProcessProviderAddedEvent),
        AtpProcessAddedCallback,
        NULL,
        &AtProcessAddedRegistration
        );
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackProcessProviderModifiedEvent),
        AtpProcessModifiedCallback,
        NULL,
        &AtProcessModifiedRegistration
        );
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackProcessProviderRemovedEvent),
        AtpProcessRemovedCallback,
        NULL,
        &AtProcessRemovedRegistration
        );
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
        AtpProcessUpdatedCallback,
        NULL,
        &AtProcessUpdatedRegistration
        );
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackServiceProviderAddedEvent),
        AtpServiceAddedCallback,
        NULL,
        &AtServiceAddedRegistration
        );
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackServiceProviderModifiedEvent),
        AtpServiceModifiedCallback,
        NULL,
        &AtServiceModifiedRegistration
        );
    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackServiceProviderRemovedEvent),
        AtpServiceRemovedCallback,
        NULL,
        &AtServiceRemovedRegistration
        );
}

VOID AtSnapshotUninitialize(
    VOID
    )
{
    ULONG i;

    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderAddedEvent), &AtProcessAddedRegistration);
    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderModifiedEvent), &AtProcessModifiedRegistration);
    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderRemovedEvent), &AtProcessRemovedRegistration);
    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent), &AtProcessUpdatedRegistration);
    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackServiceProviderAddedEvent), &AtServiceAddedRegistration);
    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackServiceProviderModifiedEvent), &AtServiceModifiedRegistration);
    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackServiceProviderRemovedEvent), &AtServiceRemovedRegistration);

    PhAcquireQueuedLockExclusive(&AtSnapshotLock);

    for (i = 0; i < AtProcessChanges->Count; i++)
        AtpFreeChange(AtProcessChanges->Items[i]);

    for (i = 0; i < AtServiceChanges->Count; i++)
        AtpFreeChange(AtServiceChanges->Items[i]);

    PhClearReference(&AtProcessChanges);
    PhClearReference(&AtServiceChanges);

    PhReleaseQueuedLockExclusive(&AtSnapshotLock);
}

VOID AtpAddProcessChangeRow(
    _In_ PVOID Array,
    _In_ PAT_CHANGE Change
    )
{
    PVOID row;

    row = PhCreateJsonObject();
    PhAddJsonObjectUInt64(row, "pid", HandleToUlong(Change->ProcessId));
    PhAddJsonObjectUInt64(row, "process_sequence_number", Change->SequenceNumber);
    AtJsonAddString(row, "name", Change->Name);
    PhAddJsonArrayObject(Array, row);
}

VOID AtpAddServiceChangeRow(
    _In_ PVOID Array,
    _In_ PAT_CHANGE Change
    )
{
    PVOID row;

    row = PhCreateJsonObject();
    AtJsonAddString(row, "name", Change->Name);
    PhAddJsonArrayObject(Array, row);
}

VOID AtpAddChanges(
    _In_ PVOID Object,
    _In_ PPH_LIST List,
    _In_ ULONG SinceId,
    _In_ BOOLEAN Processes
    )
{
    PVOID changes;
    PVOID added;
    PVOID changed;
    PVOID removed;
    BOOLEAN complete;
    ULONG i;

    changes = PhCreateJsonObject();
    added = PhCreateJsonArray();
    changed = PhCreateJsonArray();
    removed = PhCreateJsonArray();

    PhAcquireQueuedLockShared(&AtSnapshotLock);

    // Nothing before tracking started can be described, and neither can a removal old enough to
    // have been forgotten; either way the caller has to re-list rather than trust the delta.
    complete = SinceId >= AtSnapshotTrackingSinceId && SinceId >= AtSnapshotOldestForgottenId;

    for (i = 0; i < List->Count; i++)
    {
        PAT_CHANGE change = List->Items[i];

        if (change->RemovedId > SinceId)
        {
            if (Processes)
                AtpAddProcessChangeRow(removed, change);
            else
                AtpAddServiceChangeRow(removed, change);
        }
        else if (change->RemovedId != 0)
        {
            continue; // removed before the caller's snapshot; it never saw it live
        }
        else if (change->AddedId > SinceId)
        {
            if (Processes)
                AtpAddProcessChangeRow(added, change);
            else
                AtpAddServiceChangeRow(added, change);
        }
        else if (change->ModifiedId > SinceId)
        {
            if (Processes)
                AtpAddProcessChangeRow(changed, change);
            else
                AtpAddServiceChangeRow(changed, change);
        }
    }

    PhReleaseQueuedLockShared(&AtSnapshotLock);

    PhAddJsonObjectUInt64(changes, "since_snapshot_id", SinceId);
    PhAddJsonObjectBoolean(changes, "complete", complete);
    PhAddJsonObjectValue(changes, "added", added);
    PhAddJsonObjectValue(changes, "changed", changed);
    PhAddJsonObjectValue(changes, "removed", removed);

    PhAddJsonObjectValue(Object, "changes", changes);
}

VOID AtAddProcessChanges(
    _In_ PVOID Object,
    _In_ ULONG SinceId
    )
{
    AtpAddChanges(Object, AtProcessChanges, SinceId, TRUE);
}

VOID AtAddServiceChanges(
    _In_ PVOID Object,
    _In_ ULONG SinceId
    )
{
    AtpAddChanges(Object, AtServiceChanges, SinceId, FALSE);
}
