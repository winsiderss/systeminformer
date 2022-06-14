/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *
 */

#include <phbase.h>
#include <handle.h>
#include <handlep.h>

static PH_INITONCE PhHandleTableInitOnce = PH_INITONCE_INIT;
static PH_FREE_LIST PhHandleTableLevel0FreeList;
static PH_FREE_LIST PhHandleTableLevel1FreeList;

PPH_HANDLE_TABLE PhCreateHandleTable(
    VOID
    )
{
    PPH_HANDLE_TABLE handleTable;
    ULONG i;

    if (PhBeginInitOnce(&PhHandleTableInitOnce))
    {
        PhInitializeFreeList(
            &PhHandleTableLevel0FreeList,
            sizeof(PH_HANDLE_TABLE_ENTRY) * PH_HANDLE_TABLE_LEVEL_ENTRIES,
            64
            );
        PhInitializeFreeList(
            &PhHandleTableLevel1FreeList,
            sizeof(PPH_HANDLE_TABLE_ENTRY) * PH_HANDLE_TABLE_LEVEL_ENTRIES,
            64
            );
        PhEndInitOnce(&PhHandleTableInitOnce);
    }

#ifdef PH_HANDLE_TABLE_SAFE
    handleTable = PhAllocateSafe(sizeof(PH_HANDLE_TABLE));

    if (!handleTable)
        return NULL;
#else
    handleTable = PhAllocate(sizeof(PH_HANDLE_TABLE));
#endif

    PhInitializeQueuedLock(&handleTable->Lock);
    PhInitializeWakeEvent(&handleTable->HandleWakeEvent);

    handleTable->NextValue = 0;

    handleTable->Count = 0;
    handleTable->TableValue = (ULONG_PTR)PhpCreateHandleTableLevel0(handleTable, TRUE);

#ifdef PH_HANDLE_TABLE_SAFE
    if (!handleTable->TableValue)
    {
        PhFree(handleTable);
        return NULL;
    }
#endif

    // We have now created the level 0 table. The free list can now be set up to point to handle 0,
    // which points to the rest of the free list (1 -> 2 -> 3 -> ...). The next batch of handles
    // that need to be created start at PH_HANDLE_TABLE_LEVEL_ENTRIES.

    handleTable->FreeValue = 0;
    handleTable->NextValue = PH_HANDLE_TABLE_LEVEL_ENTRIES;

    handleTable->FreeValueAlt = PH_HANDLE_VALUE_INVALID; // no entries in alt. free list

    handleTable->Flags = 0;

    for (i = 0; i < PH_HANDLE_TABLE_LOCKS; i++)
        PhInitializeQueuedLock(&handleTable->Locks[i]);

    return handleTable;
}

VOID PhDestroyHandleTable(
    _In_ _Post_invalid_ PPH_HANDLE_TABLE HandleTable
    )
{
    ULONG_PTR tableValue;
    ULONG tableLevel;
    PPH_HANDLE_TABLE_ENTRY table0;
    PPH_HANDLE_TABLE_ENTRY *table1;
    PPH_HANDLE_TABLE_ENTRY **table2;
    ULONG i;
    ULONG j;

    tableValue = HandleTable->TableValue;
    tableLevel = tableValue & PH_HANDLE_TABLE_LEVEL_MASK;
    tableValue -= tableLevel;

    switch (tableLevel)
    {
    case 0:
        {
            table0 = (PPH_HANDLE_TABLE_ENTRY)tableValue;

            PhpFreeHandleTableLevel0(table0);
        }
        break;
    case 1:
        {
            table1 = (PPH_HANDLE_TABLE_ENTRY *)tableValue;

            for (i = 0; i < PH_HANDLE_TABLE_LEVEL_ENTRIES; i++)
            {
                if (!table1[i])
                    break;

                PhpFreeHandleTableLevel0(table1[i]);
            }

            PhpFreeHandleTableLevel1(table1);
        }
        break;
    case 2:
        {
            table2 = (PPH_HANDLE_TABLE_ENTRY **)tableValue;

            for (i = 0; i < PH_HANDLE_TABLE_LEVEL_ENTRIES; i++)
            {
                if (!table2[i])
                    break;

                for (j = 0; j < PH_HANDLE_TABLE_LEVEL_ENTRIES; j++)
                {
                    if (!table2[i][j])
                        break;

                    PhpFreeHandleTableLevel0(table2[i][j]);
                }

                PhpFreeHandleTableLevel1(table2[i]);
            }

            PhpFreeHandleTableLevel2(table2);
        }
        break;
    default:
        ASSUME_NO_DEFAULT;
    }

    PhFree(HandleTable);
}

VOID PhpBlockOnLockedHandleTableEntry(
    _Inout_ PPH_HANDLE_TABLE HandleTable,
    _In_ PPH_HANDLE_TABLE_ENTRY HandleTableEntry
    )
{
    PH_QUEUED_WAIT_BLOCK waitBlock;
    ULONG_PTR value;

    PhQueueWakeEvent(&HandleTable->HandleWakeEvent, &waitBlock);

    value = HandleTableEntry->Value;

    if (
        (value & PH_HANDLE_TABLE_ENTRY_TYPE) != PH_HANDLE_TABLE_ENTRY_IN_USE ||
        (value & PH_HANDLE_TABLE_ENTRY_LOCKED)
        )
    {
        // Entry is not in use or has been unlocked; cancel the wait.
        PhSetWakeEvent(&HandleTable->HandleWakeEvent, &waitBlock);
    }
    else
    {
        PhWaitForWakeEvent(&HandleTable->HandleWakeEvent, &waitBlock, TRUE, NULL);
    }
}

BOOLEAN PhLockHandleTableEntry(
    _Inout_ PPH_HANDLE_TABLE HandleTable,
    _Inout_ PPH_HANDLE_TABLE_ENTRY HandleTableEntry
    )
{
    ULONG_PTR value;

    while (TRUE)
    {
        value = HandleTableEntry->Value;

        if ((value & PH_HANDLE_TABLE_ENTRY_TYPE) != PH_HANDLE_TABLE_ENTRY_IN_USE)
            return FALSE;

        if (value & PH_HANDLE_TABLE_ENTRY_LOCKED)
        {
            if ((ULONG_PTR)_InterlockedCompareExchangePointer(
                (PVOID *)&HandleTableEntry->Value,
                (PVOID)(value - PH_HANDLE_TABLE_ENTRY_LOCKED),
                (PVOID)value
                ) == value)
            {
                return TRUE;
            }
        }

        PhpBlockOnLockedHandleTableEntry(HandleTable, HandleTableEntry);
    }
}

VOID PhUnlockHandleTableEntry(
    _Inout_ PPH_HANDLE_TABLE HandleTable,
    _Inout_ PPH_HANDLE_TABLE_ENTRY HandleTableEntry
    )
{
    _interlockedbittestandset(
        (PLONG)&HandleTableEntry->Value,
        PH_HANDLE_TABLE_ENTRY_LOCKED_SHIFT
        );
    PhSetWakeEvent(&HandleTable->HandleWakeEvent, NULL);
}

HANDLE PhCreateHandle(
    _Inout_ PPH_HANDLE_TABLE HandleTable,
    _In_ PPH_HANDLE_TABLE_ENTRY HandleTableEntry
    )
{
    PPH_HANDLE_TABLE_ENTRY entry;
    ULONG handleValue;

    entry = PhpAllocateHandleTableEntry(HandleTable, &handleValue);

    if (!entry)
        return NULL;

    // Copy the given handle table entry to the allocated entry.

    // All free entries have the Free type and have the (not) locked bit clear. There is no problem
    // with setting the Type now; the entry is still locked, so they will block.
    entry->TypeAndValue.Type = PH_HANDLE_TABLE_ENTRY_IN_USE;
    entry->TypeAndValue.Value = HandleTableEntry->TypeAndValue.Value;
    entry->Value2 = HandleTableEntry->Value2;

    // Now we unlock this entry, waking anyone who was caught back there before we had finished
    // setting up the entry.
    PhUnlockHandleTableEntry(HandleTable, entry);

    return PhpEncodeHandle(handleValue);
}

BOOLEAN PhDestroyHandle(
    _Inout_ PPH_HANDLE_TABLE HandleTable,
    _In_ HANDLE Handle,
    _In_opt_ PPH_HANDLE_TABLE_ENTRY HandleTableEntry
    )
{
    ULONG handleValue;

    handleValue = PhpDecodeHandle(Handle);

    if (!HandleTableEntry)
    {
        HandleTableEntry = PhpLookupHandleTableEntry(HandleTable, handleValue);

        if (!HandleTableEntry)
            return FALSE;

        if (!PhLockHandleTableEntry(HandleTable, HandleTableEntry))
            return FALSE;
    }

    _InterlockedExchangePointer(
        (PVOID *)&HandleTableEntry->Value,
        (PVOID)PH_HANDLE_TABLE_ENTRY_FREE
        );

    // The handle table entry is now free; wake any waiters because they can't lock the entry now.
    // Any future lock attempts will fail because the entry is marked as being free.
    PhSetWakeEvent(&HandleTable->HandleWakeEvent, NULL);

    PhpFreeHandleTableEntry(HandleTable, handleValue, HandleTableEntry);

    return TRUE;
}

PPH_HANDLE_TABLE_ENTRY PhLookupHandleTableEntry(
    _In_ PPH_HANDLE_TABLE HandleTable,
    _In_ HANDLE Handle
    )
{
    PPH_HANDLE_TABLE_ENTRY entry;

    entry = PhpLookupHandleTableEntry(HandleTable, PhpDecodeHandle(Handle));

    if (!entry)
        return NULL;

    if (!PhLockHandleTableEntry(HandleTable, entry))
        return NULL;

    return entry;
}

VOID PhEnumHandleTable(
    _In_ PPH_HANDLE_TABLE HandleTable,
    _In_ PPH_ENUM_HANDLE_TABLE_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    ULONG handleValue;
    PPH_HANDLE_TABLE_ENTRY entry;
    BOOLEAN cont;

    handleValue = 0;

    while (entry = PhpLookupHandleTableEntry(HandleTable, handleValue))
    {
        if (PhLockHandleTableEntry(HandleTable, entry))
        {
            cont = Callback(
                HandleTable,
                PhpEncodeHandle(handleValue),
                entry,
                Context
                );
            PhUnlockHandleTableEntry(HandleTable, entry);

            if (!cont)
                break;
        }

        handleValue++;
    }
}

VOID PhSweepHandleTable(
    _In_ PPH_HANDLE_TABLE HandleTable,
    _In_ PPH_ENUM_HANDLE_TABLE_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    ULONG handleValue;
    PPH_HANDLE_TABLE_ENTRY entry;
    BOOLEAN cont;

    handleValue = 0;

    while (entry = PhpLookupHandleTableEntry(HandleTable, handleValue))
    {
        if (entry->TypeAndValue.Type == PH_HANDLE_TABLE_ENTRY_IN_USE)
        {
            cont = Callback(
                HandleTable,
                PhpEncodeHandle(handleValue),
                entry,
                Context
                );

            if (!cont)
                break;
        }

        handleValue++;
    }
}

NTSTATUS PhQueryInformationHandleTable(
    _In_ PPH_HANDLE_TABLE HandleTable,
    _In_ PH_HANDLE_TABLE_INFORMATION_CLASS InformationClass,
    _Out_writes_bytes_opt_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_opt_ PULONG ReturnLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG returnLength;

    switch (InformationClass)
    {
    case HandleTableBasicInformation:
        {
            PPH_HANDLE_TABLE_BASIC_INFORMATION basicInfo = Buffer;

            if (BufferLength == sizeof(PH_HANDLE_TABLE_BASIC_INFORMATION))
            {
                basicInfo->Count = HandleTable->Count;
                basicInfo->Flags = HandleTable->Flags;
                basicInfo->TableLevel = HandleTable->TableValue & PH_HANDLE_TABLE_LEVEL_MASK;
            }
            else
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
            }

            returnLength = sizeof(PH_HANDLE_TABLE_BASIC_INFORMATION);
        }
        break;
    case HandleTableFlagsInformation:
        {
            PPH_HANDLE_TABLE_FLAGS_INFORMATION flagsInfo = Buffer;

            if (BufferLength == sizeof(PH_HANDLE_TABLE_FLAGS_INFORMATION))
            {
                flagsInfo->Flags = HandleTable->Flags;
            }
            else
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
            }

            returnLength = sizeof(PH_HANDLE_TABLE_FLAGS_INFORMATION);
        }
        break;
    default:
        status = STATUS_INVALID_INFO_CLASS;
        returnLength = 0;
        break;
    }

    if (ReturnLength)
        *ReturnLength = returnLength;

    return status;
}

NTSTATUS PhSetInformationHandleTable(
    _Inout_ PPH_HANDLE_TABLE HandleTable,
    _In_ PH_HANDLE_TABLE_INFORMATION_CLASS InformationClass,
    _In_reads_bytes_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    switch (InformationClass)
    {
    case HandleTableFlagsInformation:
        {
            PPH_HANDLE_TABLE_FLAGS_INFORMATION flagsInfo = Buffer;
            ULONG flags;

            if (BufferLength == sizeof(PH_HANDLE_TABLE_FLAGS_INFORMATION))
            {
                flags = flagsInfo->Flags;

                if ((flags & PH_HANDLE_TABLE_VALID_FLAGS) == flags)
                    HandleTable->Flags = flags;
                else
                    status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
            }
        }
        break;
    default:
        status = STATUS_INVALID_INFO_CLASS;
    }

    return status;
}

PPH_HANDLE_TABLE_ENTRY PhpAllocateHandleTableEntry(
    _Inout_ PPH_HANDLE_TABLE HandleTable,
    _Out_ PULONG HandleValue
    )
{
    PPH_HANDLE_TABLE_ENTRY entry;
    ULONG freeValue;
    ULONG lockIndex;
    ULONG nextFreeValue;
    ULONG oldFreeValue;
    BOOLEAN result;

    while (TRUE)
    {
        freeValue = HandleTable->FreeValue;

        while (freeValue == PH_HANDLE_VALUE_INVALID)
        {
            PhAcquireQueuedLockExclusive(&HandleTable->Lock);

            // Check again to see if we have free handles.

            freeValue = HandleTable->FreeValue;

            if (freeValue != PH_HANDLE_VALUE_INVALID)
            {
                PhReleaseQueuedLockExclusive(&HandleTable->Lock);
                break;
            }

            // Move handles from the alt. free list to the main free list, and check again.

            freeValue = PhpMoveFreeHandleTableEntries(HandleTable);

            if (freeValue != PH_HANDLE_VALUE_INVALID)
            {
                PhReleaseQueuedLockExclusive(&HandleTable->Lock);
                break;
            }

            result = PhpAllocateMoreHandleTableEntries(HandleTable, TRUE);

            PhReleaseQueuedLockExclusive(&HandleTable->Lock);

            freeValue = HandleTable->FreeValue;

            // Note that PhpAllocateMoreHandleTableEntries only returns FALSE if it failed to
            // allocate memory. Success does not guarantee a free handle to be allocated, as they
            // may have been all used up (however unlikely) when we reach this point. Success simply
            // means to retry the allocation using the fast path.

            if (!result && freeValue == PH_HANDLE_VALUE_INVALID)
                return NULL;
        }

        entry = PhpLookupHandleTableEntry(HandleTable, freeValue);
        lockIndex = PH_HANDLE_TABLE_LOCK_INDEX(freeValue);

        // To avoid the ABA problem, we would ideally have one queued lock per handle table entry.
        // That would make the overhead too large, so instead there is a fixed number of locks,
        // indexed by the handle value (mod no. locks).

        // Possibilities at this point:
        // 1. freeValue != A (our copy), but the other thread has freed A, so FreeValue = A. No ABA
        // problem since freeValue != A.
        // 2. freeValue != A, and FreeValue != A. No ABA problem.
        // 3. freeValue = A, and the other thread has freed A, so FreeValue = A. No ABA problem
        // since we haven't read NextFreeValue yet.
        // 4. freeValue = A, and FreeValue != A. No problem if this stays the same later, as the CAS
        // will take care of it.

        PhpLockHandleTableShared(HandleTable, lockIndex);

        if (HandleTable->FreeValue != freeValue)
        {
            PhpUnlockHandleTableShared(HandleTable, lockIndex);
            continue;
        }

        MemoryBarrier();

        nextFreeValue = entry->NextFreeValue;

        // Possibilities/non-possibilities at this point:
        // 1. freeValue != A (our copy), but the other thread has freed A, so FreeValue = A. This is
        // actually impossible since we have acquired the lock on A and the free code checks that
        // and uses the alt. free list instead.
        // 2. freeValue != A, and FreeValue != A. No ABA problem.
        // 3. freeValue = A, and the other thread has freed A, so FreeValue = A. Impossible like
        // above. This is *the* ABA problem which we have now prevented.
        // 4. freeValue = A, and FreeValue != A. CAS will take care of it.

        oldFreeValue = _InterlockedCompareExchange(
            &HandleTable->FreeValue,
            nextFreeValue,
            freeValue
            );

        PhpUnlockHandleTableShared(HandleTable, lockIndex);

        if (oldFreeValue == freeValue)
            break;
    }

    _InterlockedIncrement((PLONG)&HandleTable->Count);

    *HandleValue = freeValue;

    return entry;
}

VOID PhpFreeHandleTableEntry(
    _Inout_ PPH_HANDLE_TABLE HandleTable,
    _In_ ULONG HandleValue,
    _Inout_ PPH_HANDLE_TABLE_ENTRY HandleTableEntry
    )
{
    PULONG freeList;
    ULONG flags;
    ULONG oldValue;

    _InterlockedDecrement((PLONG)&HandleTable->Count);

    flags = HandleTable->Flags;

    // Choose the free list to use depending on whether someone is popping from the main free list
    // (see PhpAllocateHandleTableEntry for details). We always use the alt. free list if strict
    // FIFO is enabled.
    if (!(flags & PH_HANDLE_TABLE_STRICT_FIFO) &&
        PhTryAcquireReleaseQueuedLockExclusive(
        &HandleTable->Locks[PH_HANDLE_TABLE_LOCK_INDEX(HandleValue)]))
    {
        freeList = &HandleTable->FreeValue;
    }
    else
    {
        freeList = &HandleTable->FreeValueAlt;
    }

    while (TRUE)
    {
        oldValue = *freeList;
        HandleTableEntry->NextFreeValue = oldValue;

        if (_InterlockedCompareExchange(
            freeList,
            HandleValue,
            oldValue
            ) == oldValue)
        {
            break;
        }
    }
}

BOOLEAN PhpAllocateMoreHandleTableEntries(
    _In_ PPH_HANDLE_TABLE HandleTable,
    _In_ BOOLEAN Initialize
    )
{
    ULONG_PTR tableValue;
    ULONG tableLevel;
    PPH_HANDLE_TABLE_ENTRY table0;
    PPH_HANDLE_TABLE_ENTRY *table1;
    PPH_HANDLE_TABLE_ENTRY **table2;
    ULONG i;
    ULONG j;
    ULONG oldNextValue;
    ULONG freeValue;

    // Get a pointer to the table, and its level.

    tableValue = HandleTable->TableValue;
    tableLevel = tableValue & PH_HANDLE_TABLE_LEVEL_MASK;
    tableValue -= tableLevel;

    switch (tableLevel)
    {
    case 0:
        {
            // Create a level 1 table.

            table1 = PhpCreateHandleTableLevel1(HandleTable);

#ifdef PH_HANDLE_TABLE_SAFE
            if (!table1)
                return FALSE;
#endif

            // Create a new level 0 table and move the existing level into the new level 1 table.

            table0 = PhpCreateHandleTableLevel0(HandleTable, Initialize);

#ifdef PH_HANDLE_TABLE_SAFE
            if (!table0)
            {
                PhpFreeHandleTableLevel1(table1);
                return FALSE;
            }
#endif

            table1[0] = (PPH_HANDLE_TABLE_ENTRY)tableValue;
            table1[1] = table0;

            tableValue = (ULONG_PTR)table1 | 1;
            //_InterlockedExchangePointer((PVOID *)&HandleTable->TableValue, (PVOID)tableValue);
            HandleTable->TableValue = tableValue;
        }
        break;
    case 1:
        {
            table1 = (PPH_HANDLE_TABLE_ENTRY *)tableValue;

            // Determine whether we need to create a new level 0 table or create a level 2 table.

            i = HandleTable->NextValue / PH_HANDLE_TABLE_LEVEL_ENTRIES;

            if (i < PH_HANDLE_TABLE_LEVEL_ENTRIES)
            {
                table0 = PhpCreateHandleTableLevel0(HandleTable, TRUE);

#ifdef PH_HANDLE_TABLE_SAFE
                if (!table0)
                    return FALSE;
#endif

                //_InterlockedExchangePointer((PVOID *)&table1[i], table0);
                table1[i] = table0;
            }
            else
            {
                // Create a level 2 table.

                table2 = PhpCreateHandleTableLevel2(HandleTable);

#ifdef PH_HANDLE_TABLE_SAFE
                if (!table2)
                    return FALSE;
#endif

                // Create a new level 1 table and move the existing level into the new level 2
                // table.

                table1 = PhpCreateHandleTableLevel1(HandleTable);

#ifdef PH_HANDLE_TABLE_SAFE
                if (!table1)
                {
                    PhpFreeHandleTableLevel2(table2);
                    return FALSE;
                }
#endif

                table0 = PhpCreateHandleTableLevel0(HandleTable, Initialize);

#ifdef PH_HANDLE_TABLE_SAFE
                if (!table0)
                {
                    PhpFreeHandleTableLevel1(table1);
                    PhpFreeHandleTableLevel2(table2);
                    return FALSE;
                }
#endif

                table1[0] = table0;

                table2[0] = (PPH_HANDLE_TABLE_ENTRY *)tableValue;
                table2[1] = table1;

                tableValue = (ULONG_PTR)table2 | 2;
                //_InterlockedExchangePointer((PVOID *)&HandleTable->TableValue, (PVOID)tableValue);
                HandleTable->TableValue = tableValue;
            }
        }
        break;
    case 2:
        {
            table2 = (PPH_HANDLE_TABLE_ENTRY **)tableValue;

            i = HandleTable->NextValue /
                (PH_HANDLE_TABLE_LEVEL_ENTRIES * PH_HANDLE_TABLE_LEVEL_ENTRIES);
            // i contains an index into the level 2 table, of the containing level 1 table.

            // Check if we have exceeded the maximum number of handles.

            if (i >= PH_HANDLE_TABLE_LEVEL_ENTRIES)
                return FALSE;

            // Check if we should create a new level 0 table or a new level 2 table.
            if (table2[i])
            {
                table0 = PhpCreateHandleTableLevel0(HandleTable, Initialize);

#ifdef PH_HANDLE_TABLE_SAFE
                if (!table0)
                    return FALSE;
#endif

                // Same as j = HandleTable->NextValue % (no. entries * no. entries), but we already
                // calculated i so just use it.
                j = HandleTable->NextValue - i *
                    (PH_HANDLE_TABLE_LEVEL_ENTRIES * PH_HANDLE_TABLE_LEVEL_ENTRIES);
                j /= PH_HANDLE_TABLE_LEVEL_ENTRIES;
                // j now contains an index into the level 1 table, of the containing level 0 table
                // (the one which was created).

                //_InterlockedExchangePointer((PVOID *)&table2[i][j], table0);
                table2[i][j] = table0;
            }
            else
            {
                table1 = PhpCreateHandleTableLevel1(HandleTable);

#ifdef PH_HANDLE_TABLE_SAFE
                if (!table1)
                    return FALSE;
#endif

                table0 = PhpCreateHandleTableLevel0(HandleTable, TRUE);

#ifdef PH_HANDLE_TABLE_SAFE
                if (!table0)
                {
                    PhpFreeHandleTableLevel1(table1);
                    return FALSE;
                }
#endif

                table1[0] = table0;

                //_InterlockedExchangePointer((PVOID *)&table2[i], table1);
                table2[i] = table1;
            }
        }
        break;
    default:
        ASSUME_NO_DEFAULT;
    }

    // In each of the cases above, we allocated one additional level 0 table.
    oldNextValue = _InterlockedExchangeAdd(
        (PLONG)&HandleTable->NextValue,
        PH_HANDLE_TABLE_LEVEL_ENTRIES
        );

    if (Initialize)
    {
        // No ABA problem since these are new handles being pushed.

        while (TRUE)
        {
            freeValue = HandleTable->FreeValue;
            table0[PH_HANDLE_TABLE_LEVEL_ENTRIES - 1].NextFreeValue = freeValue;

            if (_InterlockedCompareExchange(
                &HandleTable->FreeValue,
                oldNextValue,
                freeValue
                ) == freeValue)
            {
                break;
            }
        }
    }

    return TRUE;
}

PPH_HANDLE_TABLE_ENTRY PhpLookupHandleTableEntry(
    _In_ PPH_HANDLE_TABLE HandleTable,
    _In_ ULONG HandleValue
    )
{
    ULONG_PTR tableValue;
    ULONG tableLevel;
    PPH_HANDLE_TABLE_ENTRY table0;
    PPH_HANDLE_TABLE_ENTRY *table1;
    PPH_HANDLE_TABLE_ENTRY **table2;
    PPH_HANDLE_TABLE_ENTRY entry;

    if (HandleValue >= HandleTable->NextValue)
        return NULL;

    // Get a pointer to the table, and its level.

    tableValue = HandleTable->TableValue;
    tableLevel = tableValue & PH_HANDLE_TABLE_LEVEL_MASK;
    tableValue -= tableLevel;

    // No additional checking needed; already checked against NextValue.

    switch (tableLevel)
    {
    case 0:
        {
            table0 = (PPH_HANDLE_TABLE_ENTRY)tableValue;
            entry = &table0[HandleValue];
        }
        break;
    case 1:
        {
            table1 = (PPH_HANDLE_TABLE_ENTRY *)tableValue;
            table0 = table1[PH_HANDLE_VALUE_LEVEL1_U(HandleValue)];
            entry = &table0[PH_HANDLE_VALUE_LEVEL0(HandleValue)];
        }
        break;
    case 2:
        {
            table2 = (PPH_HANDLE_TABLE_ENTRY **)tableValue;
            table1 = table2[PH_HANDLE_VALUE_LEVEL2_U(HandleValue)];
            table0 = table1[PH_HANDLE_VALUE_LEVEL1(HandleValue)];
            entry = &table0[PH_HANDLE_VALUE_LEVEL0(HandleValue)];
        }
        break;
    default:
        ASSUME_NO_DEFAULT;
    }

    return entry;
}

ULONG PhpMoveFreeHandleTableEntries(
    _Inout_ PPH_HANDLE_TABLE HandleTable
    )
{
    ULONG freeValueAlt;
    ULONG flags;
    ULONG i;
    ULONG index;
    ULONG nextIndex;
    ULONG lastIndex;
    PPH_HANDLE_TABLE_ENTRY entry;
    PPH_HANDLE_TABLE_ENTRY firstEntry;
    ULONG count;
    ULONG freeValue;

    // Remove all entries from the alt. free list.
    freeValueAlt = _InterlockedExchange(&HandleTable->FreeValueAlt, PH_HANDLE_VALUE_INVALID);

    if (freeValueAlt == PH_HANDLE_VALUE_INVALID)
    {
        // No handles on the alt. free list.
        return PH_HANDLE_VALUE_INVALID;
    }

    // Avoid the ABA problem by testing all locks (see PhpAllocateHandleTableEntry for details).
    // Unlike in PhpFreeHandleTableEntry we have no "alternative" list, so we must allow blocking.
    for (i = 0; i < PH_HANDLE_TABLE_LOCKS; i++)
        PhAcquireReleaseQueuedLockExclusive(&HandleTable->Locks[i]);

    flags = HandleTable->Flags;

    if (!(flags & PH_HANDLE_TABLE_STRICT_FIFO))
    {
        // Shortcut: if there are no entries in the main free list and we don't need to reverse the
        // chain, just return.
        if (_InterlockedCompareExchange(
            &HandleTable->FreeValue,
            freeValueAlt,
            PH_HANDLE_VALUE_INVALID
            ) == PH_HANDLE_VALUE_INVALID)
            return freeValueAlt;
    }

    // Reverse the chain (even if strict FIFO is off; we have to traverse the list to find the last
    // entry, so we might as well reverse it along the way).

    index = freeValueAlt;
    lastIndex = PH_HANDLE_VALUE_INVALID;
    count = 0;

    while (TRUE)
    {
        entry = PhpLookupHandleTableEntry(HandleTable, index);
        count++;

        if (lastIndex == PH_HANDLE_VALUE_INVALID)
            firstEntry = entry;

        nextIndex = entry->NextFreeValue;
        entry->NextFreeValue = lastIndex;
        lastIndex = index;

        if (nextIndex == PH_HANDLE_VALUE_INVALID)
            break;

        index = nextIndex;
    }

    // Note that firstEntry actually contains the last free entry, since we reversed the list.
    // Similarly index/lastIndex both contain the index of the first free entry.

    // Push the entries onto the free list.
    while (TRUE)
    {
        freeValue = HandleTable->FreeValue;
        firstEntry->NextFreeValue = freeValue;

        if (_InterlockedCompareExchange(
            &HandleTable->FreeValue,
            index,
            freeValue
            ) == freeValue)
            break;
    }

    // Force expansion if we don't have enough free handles.
    if (
        (flags & PH_HANDLE_TABLE_STRICT_FIFO) &&
        count < PH_HANDLE_TABLE_FREE_COUNT
        )
    {
        index = PH_HANDLE_VALUE_INVALID;
    }

    return index;
}

PPH_HANDLE_TABLE_ENTRY PhpCreateHandleTableLevel0(
    _In_ PPH_HANDLE_TABLE HandleTable,
    _In_ BOOLEAN Initialize
    )
{
    PPH_HANDLE_TABLE_ENTRY table;
    PPH_HANDLE_TABLE_ENTRY entry;
    ULONG baseValue;
    ULONG i;

#ifdef PH_HANDLE_TABLE_SAFE
    __try
    {
        table = PhAllocateFromFreeList(&PhHandleTableLevel0FreeList);
    }
    __except (SIMPLE_EXCEPTION_FILTER(GetExceptionCode() == STATUS_NO_MEMORY))
    {
        return NULL;
    }
#else
    table = PhAllocateFromFreeList(&PhHandleTableLevel0FreeList);
#endif

    if (Initialize)
    {
        entry = &table[0];
        baseValue = HandleTable->NextValue;

        for (i = baseValue + 1; i < baseValue + PH_HANDLE_TABLE_LEVEL_ENTRIES; i++)
        {
            entry->Value = PH_HANDLE_TABLE_ENTRY_FREE;
            entry->NextFreeValue = i;
            entry++;
        }

        entry->Value = PH_HANDLE_TABLE_ENTRY_FREE;
        entry->NextFreeValue = PH_HANDLE_VALUE_INVALID;
    }

    return table;
}

VOID PhpFreeHandleTableLevel0(
    _In_ PPH_HANDLE_TABLE_ENTRY Table
    )
{
    PhFreeToFreeList(&PhHandleTableLevel0FreeList, Table);
}

PPH_HANDLE_TABLE_ENTRY *PhpCreateHandleTableLevel1(
    _In_ PPH_HANDLE_TABLE HandleTable
    )
{
    PPH_HANDLE_TABLE_ENTRY *table;

#ifdef PH_HANDLE_TABLE_SAFE
    __try
    {
        table = PhAllocateFromFreeList(&PhHandleTableLevel1FreeList);
    }
    __except (SIMPLE_EXCEPTION_FILTER(GetExceptionCode() == STATUS_NO_MEMORY))
    {
        return NULL;
    }
#else
    table = PhAllocateFromFreeList(&PhHandleTableLevel1FreeList);
#endif

    memset(table, 0, sizeof(PPH_HANDLE_TABLE_ENTRY) * PH_HANDLE_TABLE_LEVEL_ENTRIES);

    return table;
}

VOID PhpFreeHandleTableLevel1(
    _In_ PPH_HANDLE_TABLE_ENTRY *Table
    )
{
    PhFreeToFreeList(&PhHandleTableLevel1FreeList, Table);
}

PPH_HANDLE_TABLE_ENTRY **PhpCreateHandleTableLevel2(
    _In_ PPH_HANDLE_TABLE HandleTable
    )
{
    PPH_HANDLE_TABLE_ENTRY **table;

#ifdef PH_HANDLE_TABLE_SAFE
    table = PhAllocateSafe(sizeof(PPH_HANDLE_TABLE_ENTRY *) * PH_HANDLE_TABLE_LEVEL_ENTRIES);

    if (!table)
        return NULL;
#else
    table = PhAllocate(sizeof(PPH_HANDLE_TABLE_ENTRY *) * PH_HANDLE_TABLE_LEVEL_ENTRIES);
#endif

    memset(table, 0, sizeof(PPH_HANDLE_TABLE_ENTRY *) * PH_HANDLE_TABLE_LEVEL_ENTRIES);

    return table;
}

VOID PhpFreeHandleTableLevel2(
    _In_ PPH_HANDLE_TABLE_ENTRY **Table
    )
{
    PhFree(Table);
}
