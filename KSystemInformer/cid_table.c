/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *
 */

#include <cid_table.h>
#include <kph.h>

#include <trace.h>

PAGED_FILE();

#define CID_TABLE_LEVEL_MASK ((ULONG_PTR)3)
#define CID_TABLE_L0 ((ULONG_PTR)0)
#define CID_TABLE_L1 ((ULONG_PTR)1)
#define CID_TABLE_L2 ((ULONG_PTR)2)
#define CID_TABLE_POINTER_MASK ~CID_TABLE_LEVEL_MASK

#define CID_L0_COUNT (PAGE_SIZE / sizeof(CID_TABLE_ENTRY))
#define CID_L1_COUNT (PAGE_SIZE / sizeof(CID_TABLE_ENTRY))
#define CID_L2_COUNT ((1 << 24) / (CID_L0_COUNT * CID_L1_COUNT))

#define CID_MAX_L0 CID_L0_COUNT
#define CID_MAX_L1 (CID_L1_COUNT * CID_L0_COUNT)
#define CID_MAX_L2 (CID_L2_COUNT * CID_L1_COUNT * CID_L0_COUNT)

#define CID_MAX CID_MAX_L2
C_ASSERT(CID_MAX > 0);

#define CidToId(x) ((ULONG_PTR)x / 4)

/**
 * \brief Acquires the CID table entry object lock.
 *
 * \param[in] Entry The entry to acquire the object lock of.
 */
_IRQL_requires_max_(APC_LEVEL)
_Acquires_lock_(_Global_critical_region_)
VOID CidAcquireObjectLock(
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_) PCID_TABLE_ENTRY Entry
    )
{
    PAGED_CODE();

    KeEnterCriticalRegion();

    for (;; YieldProcessor())
    {
        ULONG_PTR object;
        PVOID expected;
        PVOID locked;

        object = Entry->Object;
        if (object & ~CID_LOCKED_OBJECT_MASK)
        {
            continue;
        }

        expected = (PVOID)object;
        locked = (PVOID)((ULONG_PTR)expected | ~CID_LOCKED_OBJECT_MASK);

        if (InterlockedCompareExchangePointer((PVOID*)&Entry->Object,
                                              locked,
                                              expected) == expected)
        {
            break;
        }
    }
}

/**
 * \brief Releases the CID table entry object lock.
 *
 * \param[in] Entry The entry to release the object lock of.
 */
_IRQL_requires_max_(APC_LEVEL)
_Releases_lock_(_Global_critical_region_)
VOID CidReleaseObjectLock(
    _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_) PCID_TABLE_ENTRY Entry
    )
{
    ULONG_PTR object;
    PVOID unlocked;

    PAGED_CODE();

    object = Entry->Object;
    NT_ASSERT(object & ~CID_LOCKED_OBJECT_MASK);

    unlocked = (PVOID)(object & CID_LOCKED_OBJECT_MASK);

    InterlockedExchangePointer((PVOID*)&Entry->Object, unlocked);

    KeLeaveCriticalRegion();
}

/**
 * \brief Assigns an object to the table entry.
 *
 * \param[in,out] Entry The entry to assign to.
 * \param[in] Object The object pointer to assign.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID CidAssignObject(
    _Inout_ _Requires_lock_held_(*_Curr_) PCID_TABLE_ENTRY Entry,
    _In_opt_ PVOID Object
    )
{
    ULONG_PTR object;

    PAGED_CODE();

    NT_ASSERT(((ULONG_PTR)Object & ~CID_LOCKED_OBJECT_MASK) == 0);

    object = ((ULONG_PTR)Object | ~CID_LOCKED_OBJECT_MASK);

    InterlockedExchangePointer((PVOID*)&Entry->Object, (PVOID)object);
}

/**
 * \brief Allocates a CID table for a given level.
 *
 * \param[in] Level The level to allocate for.
 *
 * \return Allocated table or null on allocation failure.
 */
_IRQL_requires_max_(APC_LEVEL)
_When_(Level == CID_TABLE_L0, _Return_allocatesMem_size_(CID_L0_COUNT * sizeof(CID_TABLE_ENTRY)))
_When_(Level == CID_TABLE_L1, _Return_allocatesMem_size_(CID_L1_COUNT * sizeof(CID_TABLE_ENTRY)))
_When_(Level == CID_TABLE_L2, _Return_allocatesMem_size_(CID_L2_COUNT * sizeof(CID_TABLE_ENTRY)))
PVOID CidAllocateTable(
    _In_ ULONG_PTR Level
    )
{
    PAGED_CODE();

    switch (Level)
    {
        case CID_TABLE_L0:
        {
            return KphAllocatePaged(CID_L0_COUNT * sizeof(CID_TABLE_ENTRY),
                                    KPH_TAG_CID_TABLE);
        }
        case CID_TABLE_L1:
        {
            return KphAllocatePaged(CID_L1_COUNT * sizeof(CID_TABLE_ENTRY),
                                    KPH_TAG_CID_TABLE);
        }
        case CID_TABLE_L2:
        {
            return KphAllocatePaged(CID_L2_COUNT * sizeof(CID_TABLE_ENTRY),
                                    KPH_TAG_CID_TABLE);
        }
        default:
        {
            NT_ASSERT(FALSE);
            break;
        }
    }

    return NULL;
}

/**
 * \brief Creates and initializes a CID table
 *
 * \param[out] Table The table to create an initialize.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS CidTableCreate(
    _Out_ PCID_TABLE Table
    )
{
    PAGED_CODE();

    Table->Table = (ULONG_PTR)CidAllocateTable(CID_TABLE_L0);
    if (!Table->Table)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KphInitializeRWLock(&Table->Lock);

    return STATUS_SUCCESS;
}

/**
 * \brief Deletes a CID table.
 *
 * \param[in] Table The table to delete.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID CidTableDelete(
    _In_ PCID_TABLE Table
    )
{
    ULONG_PTR tableCode;
    PCID_TABLE_ENTRY tableL0;
    PCID_TABLE_ENTRY* tableL1;
    PCID_TABLE_ENTRY** tableL2;

    PAGED_CODE();

    tableCode = Table->Table;
    if (!tableCode)
    {
        return;
    }

    KphDeleteRWLock(&Table->Lock);

    switch (tableCode & CID_TABLE_LEVEL_MASK)
    {
        case 0:
        {
            tableL0 = (PCID_TABLE_ENTRY)(tableCode & CID_TABLE_POINTER_MASK);

            NT_ASSERT(tableL0);

            KphFree(tableL0, KPH_TAG_CID_TABLE);

            break;
        }
        case 1:
        {
            tableL1 = (PCID_TABLE_ENTRY*)(tableCode & CID_TABLE_POINTER_MASK);

            NT_ASSERT(tableL1);

            for (ULONG i = 0; i < CID_L1_COUNT; i++)
            {
#pragma prefast(suppress : 6001) // memory is initialized
                if (tableL1[i])
                {
                    KphFree(tableL1[i], KPH_TAG_CID_TABLE);
                }
            }

            KphFree(tableL1, KPH_TAG_CID_TABLE);

            break;
        }
        case 2:
        {
            tableL2 = (PCID_TABLE_ENTRY**)(tableCode & CID_TABLE_POINTER_MASK);

            NT_ASSERT(tableL2);

            for (ULONG i = 0; i < CID_L2_COUNT; i++)
            {
                tableL1 = tableL2[i];
                if (!tableL1)
                {
                    continue;
                }

                for (ULONG j = 0; j < CID_L1_COUNT; j++)
                {
#pragma prefast(suppress : 6001) // memory is initialized
                    if (tableL1[j])
                    {
                        KphFree(tableL1[j], KPH_TAG_CID_TABLE);
                    }
                }

                KphFree(tableL1, KPH_TAG_CID_TABLE);
            }

            KphFree(tableL2, KPH_TAG_CID_TABLE);

            break;
        }
        default:
        {
            NT_ASSERT(FALSE);
            break;
        }
    }
}

/**
 * \brief Looks up an entry in the CID table.
 *
 * \param[in] Cid The CID to look up the entry of.
 * \param[in] Table The table to look up the CID in.
 *
 * \return Pointer to the CID table entry, null if the table hasn't been
 * expanded enough.
 */
_IRQL_requires_max_(APC_LEVEL)
PCID_TABLE_ENTRY CidLookupEntry(
    _In_ HANDLE Cid,
    _In_ PCID_TABLE Table
    )
{
    ULONG_PTR table;
    PCID_TABLE_ENTRY tableL0;
    PCID_TABLE_ENTRY* tableL1;
    PCID_TABLE_ENTRY** tableL2;
    ULONG_PTR id;

    PAGED_CODE();

    id = CidToId(Cid);

    //
    // N.B. Capture the volatile table pointer. This is a lock-free lookup.
    //
    table = Table->Table;

    switch (table & CID_TABLE_LEVEL_MASK)
    {
        case CID_TABLE_L0:
        {
            if (id >= CID_MAX_L0)
            {
                return NULL;
            }

            tableL0 = (PCID_TABLE_ENTRY)(table & CID_TABLE_POINTER_MASK);

            return &tableL0[id];
        }
        case CID_TABLE_L1:
        {
            if (id >= CID_MAX_L1)
            {
                return NULL;
            }

            tableL1 = (PCID_TABLE_ENTRY*)(table & CID_TABLE_POINTER_MASK);
            tableL0 = tableL1[id / CID_MAX_L0];
            if (!tableL0)
            {
                return NULL;
            }

            return &tableL0[id % CID_MAX_L0];
        }
        case CID_TABLE_L2:
        {
            if (id >= CID_MAX_L2)
            {
                return NULL;
            }

            tableL2 = (PCID_TABLE_ENTRY**)(table & CID_TABLE_POINTER_MASK);
            tableL1 = tableL2[id / CID_MAX_L1];
            if (!tableL1)
            {
                return NULL;
            }

            tableL0 = tableL1[(id % CID_MAX_L1) / CID_MAX_L0];
            if (!tableL0)
            {
                return NULL;
            }

            return &tableL0[(id % CID_MAX_L1) % CID_MAX_L0];
        }
        default:
        {
            NT_ASSERT(FALSE);
            return NULL;
        }
    }
}

/**
 * \brief Expands a CID table making it capable of holding an entry for a CID.
 *
 * \param[in] Cid The CID for which the table should be expanded for.
 * \param[in,out] Table The table that should be expanded to hold the CID.
 *
 * \return Pointer to the table entry for the CID, null on allocation failure.
 */
_IRQL_requires_max_(APC_LEVEL)
PCID_TABLE_ENTRY CidExpandTableFor(
    _In_ HANDLE Cid,
    _Inout_ PCID_TABLE Table
    )
{
    PCID_TABLE_ENTRY entry;
    ULONG_PTR table;
    ULONG_PTR level;
    PCID_TABLE_ENTRY tableL0;
    PCID_TABLE_ENTRY* tableL1;
    PCID_TABLE_ENTRY** tableL2;
    ULONG_PTR id;

    PAGED_CODE();

    id = CidToId(Cid);

    if (id >= CID_MAX)
    {
        //
        // We can't, as of now, service CIDs over the maximum. It is extremely
        // unlikely we'll ever get here. The system is likely to run out of
        // memory before this happens.
        // One alternative would be to overflow these into a hash table.
        //
        NT_ASSERT(FALSE);
        return NULL;
    }

    KphAcquireRWLockExclusive(&Table->Lock);

    //
    // See if another thread beat us.
    //
    entry = CidLookupEntry(Cid, Table);
    if (entry)
    {
        //
        // Rock and roll!
        //
        goto Exit;
    }

    //
    // We are the chosen one. Go expand the tree.
    //

    table = Table->Table;
    level = (table & CID_TABLE_LEVEL_MASK);

    if (level == CID_TABLE_L0)
    {
        //
        // If we're here then we *must* be migrating to level 1.
        //
        NT_ASSERT(id >= CID_MAX_L0);

        tableL1 = CidAllocateTable(CID_TABLE_L1);
        if (!tableL1)
        {
            entry = NULL;
            goto Exit;
        }

        tableL1[0] = (PCID_TABLE_ENTRY)(table & CID_TABLE_POINTER_MASK);
        table = ((ULONG_PTR)tableL1 | CID_TABLE_L1);
        level = CID_TABLE_L1;
        InterlockedExchangePointer((PVOID*)&Table->Table, (PVOID)table);

        //
        // Fall through for level 1 expansion.
        //
    }

    if (level == CID_TABLE_L1)
    {
        if (id < CID_MAX_L1)
        {
            //
            // Allocate a new block in the level 1 table.
            //
            tableL1 = (PCID_TABLE_ENTRY*)(table & CID_TABLE_POINTER_MASK);
            NT_ASSERT(!tableL1[id / CID_MAX_L0]);
            tableL1[id / CID_MAX_L0] = CidAllocateTable(CID_TABLE_L0);
            tableL0 = tableL1[id / CID_MAX_L0];
            if (!tableL0)
            {
                entry = NULL;
                goto Exit;
            }

            entry = &tableL0[id % CID_MAX_L0];
            goto Exit;
        }

        //
        // We have to migrate to a level 2 table.
        //
        tableL2 = CidAllocateTable(CID_TABLE_L2);
        if (!tableL2)
        {
            entry = NULL;
            goto Exit;
        }

        tableL2[0] = (PCID_TABLE_ENTRY*)(table & CID_TABLE_POINTER_MASK);
        table = ((ULONG_PTR)tableL2 | CID_TABLE_L2);
        level = CID_TABLE_L2;
        InterlockedExchangePointer((PVOID*)&Table->Table, (PVOID)table);

        //
        // Fall through for level 2 expansion
        //
    }

    NT_ASSERT(level == CID_TABLE_L2);
    NT_ASSERT(id < CID_MAX_L2);

    //
    // Allocate new block(s) in the level 2 table.
    //

    tableL2 = (PCID_TABLE_ENTRY**)(table & CID_TABLE_POINTER_MASK);
    tableL1 = tableL2[id / CID_MAX_L1];
    if (!tableL1)
    {
        tableL2[id / CID_MAX_L1] = CidAllocateTable(CID_TABLE_L1);
        tableL1 = tableL2[id / CID_MAX_L1];
        if (!tableL1)
        {
            entry = NULL;
            goto Exit;
        }
    }

    NT_ASSERT(!tableL1[(id % CID_MAX_L1) / CID_MAX_L0]);
    tableL1[(id % CID_MAX_L1) / CID_MAX_L0] = CidAllocateTable(CID_TABLE_L0);
    tableL0 = tableL1[(id % CID_MAX_L1) / CID_MAX_L0];
    if (!tableL0)
    {
        entry = NULL;
        goto Exit;
    }

    entry = &tableL0[(id % CID_MAX_L1) % CID_MAX_L0];

Exit:

    KphReleaseRWLock(&Table->Lock);

    return entry;
}

/**
 * \brief Retrieves the table entry for a given CID.
 *
 * \param[in] Cid The CID to retrieve the entry for.
 * \param[in,out] Table The table to retrieve the entry from.
 *
 * \return Pointer to the table entry for the CID, null on allocation failure.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
PCID_TABLE_ENTRY CidGetEntry(
    _In_ HANDLE Cid,
    _Inout_ PCID_TABLE Table
    )
{
    PCID_TABLE_ENTRY entry;

    PAGED_CODE();

    entry = CidLookupEntry(Cid, Table);
    if (entry)
    {
        return entry;
    }

    return CidExpandTableFor(Cid, Table);
}

/**
 * \brief Handles invoking the callback during enumeration.
 *
 * \param[in] Entry The CID table entry to invoke the callback for.
 * \param[in] Callback The object callback to invoke.
 * \param[in] CallbackEx The extended callback to invoke.
 * \param[in] Parameter Optional parameter passed to the callback.
 *
 * \return TRUE if enumeration should stop, FALSE otherwise.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
BOOLEAN CidEnumerateInvokeCallback(
    _In_ PCID_TABLE_ENTRY Entry,
    _When_(!CallbackEx, _In_) _When_(CallbackEx, _Pre_null_) PCID_ENUMERATE_CALLBACK Callback,
    _When_(!Callback, _In_) _When_(Callback, _Pre_null_) PCID_ENUMERATE_CALLBACK CallbackEx,
    _In_opt_ PVOID Parameter
    )
{
    BOOLEAN res;

    PAGED_CODE();

    res = FALSE;

    if (Callback)
    {
        PVOID object;

        //
        // This path enumerates outside of the object lock.
        //

        CidAcquireObjectLock(Entry);

        object = (PVOID)(Entry->Object & CID_LOCKED_OBJECT_MASK);
        if (object)
        {
            KphReferenceObject(object);
        }

        CidReleaseObjectLock(Entry);

        if (object)
        {
            res = Callback(object, Parameter);

            KphDereferenceObject(object);
        }
    }
    else
    {
        NT_ASSERT(CallbackEx);

        //
        // This path provides the entire table entry is be done under lock!
        // The provides a way to manipulate the table entry via enumeration.
        //

        CidAcquireObjectLock(Entry);

        if (Entry->Object & CID_LOCKED_OBJECT_MASK)
        {
            res = CallbackEx(Entry, Parameter);
        }

        CidReleaseObjectLock(Entry);
    }

    return res;
}

/**
 * \brief Enumerates a CID table.
 *
 * \param[in] Table The table to enumerate.
 * \param[in] Callback The object callback to invoke.
 * \param[in] CallbackEx The extended callback to invoke.
 * \param[in] Parameter Optional parameter passed to the callback.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID CidEnumerateInternal(
    _In_ PCID_TABLE Table,
    _When_(!CallbackEx, _In_) _When_(CallbackEx, _Pre_null_) PCID_ENUMERATE_CALLBACK Callback,
    _When_(!Callback, _In_) _When_(Callback, _Pre_null_) PCID_ENUMERATE_CALLBACK CallbackEx,
    _In_opt_ PVOID Parameter
    )
{
    ULONG_PTR table;
    PCID_TABLE_ENTRY tableL0;
    PCID_TABLE_ENTRY* tableL1;
    PCID_TABLE_ENTRY** tableL2;

    PAGED_CODE();

    table = Table->Table;

    if (!table)
    {
        return;
    }

    KeEnterCriticalRegion();

    switch (table & CID_TABLE_LEVEL_MASK)
    {
        case CID_TABLE_L0:
        {
            tableL0 = (PCID_TABLE_ENTRY)(table & CID_TABLE_POINTER_MASK);

            NT_ASSERT(tableL0);

            for (ULONG i = 0; i < CID_L0_COUNT; i++)
            {
                if (CidEnumerateInvokeCallback(&tableL0[i],
                                               Callback,
                                               CallbackEx,
                                               Parameter))
                {
                    goto Exit;
                }
            }

            break;
        }
        case CID_TABLE_L1:
        {
            tableL1 = (PCID_TABLE_ENTRY*)(table & CID_TABLE_POINTER_MASK);

            NT_ASSERT(tableL1);

            for (ULONG i = 0; i < CID_L1_COUNT; i++)
            {
                tableL0 = tableL1[i];
                if (!tableL0)
                {
                    continue;
                }

                for (ULONG j = 0; j < CID_L0_COUNT; j++)
                {
                    if (CidEnumerateInvokeCallback(&tableL0[j],
                                                   Callback,
                                                   CallbackEx,
                                                   Parameter))
                    {
                        goto Exit;
                    }
                }
            }

            break;
        }
        case CID_TABLE_L2:
        {
            tableL2 = (PCID_TABLE_ENTRY**)(table & CID_TABLE_POINTER_MASK);

            NT_ASSERT(tableL2);

            for (ULONG i = 0; i < CID_L2_COUNT; i++)
            {
                tableL1 = tableL2[i];
                if (!tableL1)
                {
                    continue;
                }

                for (ULONG j = 0; j < CID_L1_COUNT; j++)
                {
                    tableL0 = tableL1[j];
                    if (!tableL0)
                    {
                        continue;
                    }

                    for (ULONG k = 0; k < CID_L0_COUNT; k++)
                    {
                        if (CidEnumerateInvokeCallback(&tableL0[k],
                                                       Callback,
                                                       CallbackEx,
                                                       Parameter))
                        {
                            goto Exit;
                        }
                    }
                }
            }

            break;
        }
        default:
        {
            NT_ASSERT(FALSE);
            break;
        }
    }

Exit:

    KeLeaveCriticalRegion();
}

/**
 * \brief Enumerates objects in a CID table.
 *
 * \param[in] Table The table to enumerate.
 * \param[in] Callback The object callback to invoke. The callback should
 * return TRUE to stop enumerating and return FALSE to continue.
 * \param[in] Parameter Optional parameter passed to the callback.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID CidEnumerate(
    _In_ PCID_TABLE Table,
    _In_ PCID_ENUMERATE_CALLBACK Callback,
    _In_opt_ PVOID Parameter
    )
{
    PAGED_CODE();

    CidEnumerateInternal(Table, Callback, NULL, Parameter);
}

/**
 * \brief Enumerates CID entires a CID table.
 *
 * \param[in] Table The table to enumerate.
 * \param[in] Callback The extended callback to invoke. The callback is invoked
 * with the object lock held. The callback should return TRUE to stop
 * enumerating and return FALSE to continue.
 * \param[in] Parameter Optional parameter passed to the callback.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID CidEnumerateEx(
    _In_ PCID_TABLE Table,
    _In_ PCID_ENUMERATE_CALLBACK_EX Callback,
    _In_opt_ PVOID Parameter
    )
{
    PAGED_CODE();

    CidEnumerateInternal(Table, NULL, Callback, Parameter);
}
