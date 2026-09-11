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

#include <ph.h>
#include <simcp.h>
#include "pending.h"

typedef struct _SIMCP_PENDING_ENTRY
{
    PPH_BYTES Id;
} SIMCP_PENDING_ENTRY, *PSIMCP_PENDING_ENTRY;

_Function_class_(PH_HASHTABLE_EQUAL_FUNCTION)
static BOOLEAN SimcppPendingEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PSIMCP_PENDING_ENTRY entry1 = Entry1;
    PSIMCP_PENDING_ENTRY entry2 = Entry2;

    return entry1->Id->Length == entry2->Id->Length &&
        memcmp(entry1->Id->Buffer, entry2->Id->Buffer, entry1->Id->Length) == 0;
}

_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
static ULONG SimcppPendingHashFunction(
    _In_ PVOID Entry
    )
{
    PSIMCP_PENDING_ENTRY entry = Entry;

    return PhHashBytes((PUCHAR)entry->Id->Buffer, entry->Id->Length);
}

VOID SimcpInitializePending(
    _Out_ PSIMCP_PENDING Pending
    )
{
    Pending->Table = PhCreateHashtable(
        sizeof(SIMCP_PENDING_ENTRY),
        SimcppPendingEqualFunction,
        SimcppPendingHashFunction,
        32
        );
    PhInitializeQueuedLock(&Pending->Lock);
}

static VOID SimcppReleaseEntries(
    _Inout_ PSIMCP_PENDING Pending
    )
{
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PSIMCP_PENDING_ENTRY entry;

    PhBeginEnumHashtable(Pending->Table, &enumContext);

    while (entry = PhNextEnumHashtable(&enumContext))
        PhDereferenceObject(entry->Id);
}

VOID SimcpDeletePending(
    _Inout_ PSIMCP_PENDING Pending
    )
{
    SimcppReleaseEntries(Pending);
    PhDereferenceObject(Pending->Table);
    Pending->Table = NULL;
}

_Success_(return)
BOOLEAN SimcpAddPending(
    _Inout_ PSIMCP_PENDING Pending,
    _In_ PPH_BYTES Id
    )
{
    SIMCP_PENDING_ENTRY lookup;
    BOOLEAN added = FALSE;

    lookup.Id = Id;

    PhAcquireQueuedLockExclusive(&Pending->Lock);

    if (Pending->Table->Count < SIMCP_MAX_PENDING_REQUESTS)
    {
        PhAddEntryHashtableEx(Pending->Table, &lookup, &added);

        if (added)
            PhReferenceObject(Id);
    }

    PhReleaseQueuedLockExclusive(&Pending->Lock);

    return added;
}

BOOLEAN SimcpRemovePending(
    _Inout_ PSIMCP_PENDING Pending,
    _In_ PPH_BYTES Id
    )
{
    SIMCP_PENDING_ENTRY lookup;
    PSIMCP_PENDING_ENTRY entry;
    BOOLEAN removed = FALSE;

    lookup.Id = Id;

    PhAcquireQueuedLockExclusive(&Pending->Lock);

    if (entry = PhFindEntryHashtable(Pending->Table, &lookup))
    {
        PPH_BYTES id = entry->Id;

        removed = PhRemoveEntryHashtable(Pending->Table, &lookup);
        PhDereferenceObject(id);
    }

    PhReleaseQueuedLockExclusive(&Pending->Lock);

    return removed;
}

VOID SimcpClearPending(
    _Inout_ PSIMCP_PENDING Pending
    )
{
    PhAcquireQueuedLockExclusive(&Pending->Lock);

    SimcppReleaseEntries(Pending);
    PhClearHashtable(Pending->Table);

    PhReleaseQueuedLockExclusive(&Pending->Lock);
}

PPH_LIST SimcpTakePending(
    _Inout_ PSIMCP_PENDING Pending
    )
{
    PPH_LIST list;
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PSIMCP_PENDING_ENTRY entry;

    PhAcquireQueuedLockExclusive(&Pending->Lock);

    list = PhCreateList(Pending->Table->Count ? Pending->Table->Count : 1);

    PhBeginEnumHashtable(Pending->Table, &enumContext);

    while (entry = PhNextEnumHashtable(&enumContext))
        PhAddItemList(list, entry->Id);

    PhClearHashtable(Pending->Table);

    PhReleaseQueuedLockExclusive(&Pending->Lock);

    return list;
}

ULONG SimcpPendingCount(
    _In_ PSIMCP_PENDING Pending
    )
{
    ULONG count;

    PhAcquireQueuedLockShared(&Pending->Lock);
    count = Pending->Table->Count;
    PhReleaseQueuedLockShared(&Pending->Lock);

    return count;
}
