/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2024
 *
 */

#include <kph.h>

#include <trace.h>

#define KPH_CID_TABLE_LEVEL_MASK ((ULONG_PTR)3)
#define KPH_CID_TABLE_L0 ((ULONG_PTR)0)
#define KPH_CID_TABLE_L1 ((ULONG_PTR)1)
#define KPH_CID_TABLE_L2 ((ULONG_PTR)2)
#define KPH_CID_TABLE_POINTER_MASK ~KPH_CID_TABLE_LEVEL_MASK

#define KPH_CID_LIMIT (1 << 24)

#define KPH_CID_L0_COUNT (PAGE_SIZE / sizeof(KPH_CID_TABLE_ENTRY))
#define KPH_CID_L1_COUNT (PAGE_SIZE / sizeof(PKPH_CID_TABLE_ENTRY))
#define KPH_CID_L2_COUNT (KPH_CID_LIMIT / (KPH_CID_L0_COUNT * KPH_CID_L1_COUNT))

#define KPH_CID_MAX_L0 KPH_CID_L0_COUNT
#define KPH_CID_MAX_L1 (KPH_CID_L1_COUNT * KPH_CID_L0_COUNT)
#define KPH_CID_MAX_L2 (KPH_CID_L2_COUNT * KPH_CID_L1_COUNT * KPH_CID_L0_COUNT)

#define KPH_CID_MAX KPH_CID_MAX_L2
C_ASSERT(KPH_CID_MAX == KPH_CID_LIMIT);

#define KphpCidToId(x) ((ULONG_PTR)x / 4)

/**
 * \brief Assigns an object to the table entry.
 *
 * \param[in,out] Entry The entry to assign to.
 * \param[in] Object Optional object pointer to assign into the table entry.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphCidAssignObject(
    _Inout_ PKPH_CID_TABLE_ENTRY Entry,
    _In_opt_ PVOID Object
    )
{
    NPAGED_CODE_DISPATCH_MAX();

    KphAtomicAssignObjectReference(&Entry->ObjectRef, Object);
}

/**
 * \brief References the object in the table entry.
 *
 * \param[in,out] Entry The entry to reference the object of.
 *
 * \return Referenced object pointer, NULL if no object is assigned.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
PVOID KphCidReferenceObject(
    _In_ PKPH_CID_TABLE_ENTRY Entry
    )
{
    NPAGED_CODE_DISPATCH_MAX();

    return KphAtomicReferenceObject(&Entry->ObjectRef);
}

/**
 * \brief Allocates a CID table for a given level.
 *
 * \param[in] Level The level to allocate for.
 *
 * \return Allocated table or null on allocation failure.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_When_(Level == KPH_CID_TABLE_L0, _Return_allocatesMem_size_(KPH_CID_L0_COUNT * sizeof(KPH_CID_TABLE_ENTRY)))
_When_(Level == KPH_CID_TABLE_L1, _Return_allocatesMem_size_(KPH_CID_L1_COUNT * sizeof(PKPH_CID_TABLE_ENTRY)))
_When_(Level == KPH_CID_TABLE_L2, _Return_allocatesMem_size_(KPH_CID_L2_COUNT * sizeof(PKPH_CID_TABLE_ENTRY)))
PVOID KphpCidAllocateTable(
    _In_ ULONG_PTR Level
    )
{
    NPAGED_CODE_DISPATCH_MAX();

    switch (Level)
    {
        case KPH_CID_TABLE_L0:
        {
            return KphAllocateNPaged(KPH_CID_L0_COUNT * sizeof(KPH_CID_TABLE_ENTRY),
                                     KPH_TAG_CID_TABLE);
        }
        case KPH_CID_TABLE_L1:
        {
            return KphAllocateNPaged(KPH_CID_L1_COUNT * sizeof(PKPH_CID_TABLE_ENTRY),
                                     KPH_TAG_CID_TABLE);
        }
        case KPH_CID_TABLE_L2:
        {
            return KphAllocateNPaged(KPH_CID_L2_COUNT * sizeof(PKPH_CID_TABLE_ENTRY),
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
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS KphCidTableCreate(
    _Out_ PKPH_CID_TABLE Table
    )
{
    NPAGED_CODE_DISPATCH_MAX();

    Table->Table = (ULONG_PTR)KphpCidAllocateTable(KPH_CID_TABLE_L0);
    if (!Table->Table)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeSpinLock(&Table->Lock);

    return STATUS_SUCCESS;
}

/**
 * \brief Deletes a CID table.
 *
 * \param[in] Table The table to delete.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphCidTableDelete(
    _In_ PKPH_CID_TABLE Table
    )
{
    ULONG_PTR tableCode;
    PKPH_CID_TABLE_ENTRY tableL0;
    PKPH_CID_TABLE_ENTRY* tableL1;
    PKPH_CID_TABLE_ENTRY** tableL2;

    NPAGED_CODE_DISPATCH_MAX();

    tableCode = ReadULongPtrAcquire(&Table->Table);

    if (!tableCode)
    {
        return;
    }

    switch (tableCode & KPH_CID_TABLE_LEVEL_MASK)
    {
        case 0:
        {
            tableL0 = (PKPH_CID_TABLE_ENTRY)(tableCode & KPH_CID_TABLE_POINTER_MASK);

            NT_ASSERT(tableL0);

            KphFree(tableL0, KPH_TAG_CID_TABLE);

            break;
        }
        case 1:
        {
            tableL1 = (PKPH_CID_TABLE_ENTRY*)(tableCode & KPH_CID_TABLE_POINTER_MASK);

            NT_ASSERT(tableL1);

            for (ULONG i = 0; i < KPH_CID_L1_COUNT; i++)
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
            tableL2 = (PKPH_CID_TABLE_ENTRY**)(tableCode & KPH_CID_TABLE_POINTER_MASK);

            NT_ASSERT(tableL2);

            for (ULONG i = 0; i < KPH_CID_L2_COUNT; i++)
            {
                tableL1 = tableL2[i];
                if (!tableL1)
                {
                    continue;
                }

                for (ULONG j = 0; j < KPH_CID_L1_COUNT; j++)
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
_IRQL_requires_max_(DISPATCH_LEVEL)
PKPH_CID_TABLE_ENTRY KphpCidLookupEntry(
    _In_ HANDLE Cid,
    _In_ PKPH_CID_TABLE Table
    )
{
    ULONG_PTR table;
    PKPH_CID_TABLE_ENTRY tableL0;
    PKPH_CID_TABLE_ENTRY* tableL1;
    PKPH_CID_TABLE_ENTRY** tableL2;
    ULONG_PTR id;

    NPAGED_CODE_DISPATCH_MAX();

    id = KphpCidToId(Cid);

    //
    // N.B. Capture the volatile table pointer. This is a lock-free lookup.
    //
    table = ReadULongPtrAcquire(&Table->Table);

    switch (table & KPH_CID_TABLE_LEVEL_MASK)
    {
        case KPH_CID_TABLE_L0:
        {
            if (id >= KPH_CID_MAX_L0)
            {
                return NULL;
            }

            tableL0 = (PKPH_CID_TABLE_ENTRY)(table & KPH_CID_TABLE_POINTER_MASK);

            return &tableL0[id];
        }
        case KPH_CID_TABLE_L1:
        {
            if (id >= KPH_CID_MAX_L1)
            {
                return NULL;
            }

            tableL1 = (PKPH_CID_TABLE_ENTRY*)(table & KPH_CID_TABLE_POINTER_MASK);
            tableL0 = tableL1[id / KPH_CID_MAX_L0];
            if (!tableL0)
            {
                return NULL;
            }

            return &tableL0[id % KPH_CID_MAX_L0];
        }
        case KPH_CID_TABLE_L2:
        {
            if (id >= KPH_CID_MAX_L2)
            {
                return NULL;
            }

            tableL2 = (PKPH_CID_TABLE_ENTRY**)(table & KPH_CID_TABLE_POINTER_MASK);
            tableL1 = tableL2[id / KPH_CID_MAX_L1];
            if (!tableL1)
            {
                return NULL;
            }

            tableL0 = tableL1[(id % KPH_CID_MAX_L1) / KPH_CID_MAX_L0];
            if (!tableL0)
            {
                return NULL;
            }

            return &tableL0[(id % KPH_CID_MAX_L1) % KPH_CID_MAX_L0];
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
_IRQL_requires_max_(DISPATCH_LEVEL)
PKPH_CID_TABLE_ENTRY KphpCidExpandTableFor(
    _In_ HANDLE Cid,
    _Inout_ PKPH_CID_TABLE Table
    )
{
    PKPH_CID_TABLE_ENTRY entry;
    ULONG_PTR table;
    ULONG_PTR level;
    PKPH_CID_TABLE_ENTRY tableL0;
    PKPH_CID_TABLE_ENTRY* tableL1;
    PKPH_CID_TABLE_ENTRY** tableL2;
    ULONG_PTR id;
    KIRQL oldIrql;

    NPAGED_CODE_DISPATCH_MAX();

    id = KphpCidToId(Cid);

    if (id >= KPH_CID_MAX)
    {
        //
        // We can't, as of now, service CIDs over the maximum. It is extremely
        // unlikely we'll ever get here. The system is likely to run out of
        // memory before this happens. It also should be impossible given the
        // limit enforced by handle tables for PspCidTable. Unless Microsoft
        // changes it, the limit is 1 << 24.
        //
        NT_ASSERT(FALSE);
        return NULL;
    }

    KeAcquireSpinLock(&Table->Lock, &oldIrql);

    //
    // See if another thread beat us.
    //
    entry = KphpCidLookupEntry(Cid, Table);
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

    table = ReadULongPtrAcquire(&Table->Table);

    level = (table & KPH_CID_TABLE_LEVEL_MASK);

    if (level == KPH_CID_TABLE_L0)
    {
        //
        // If we're here then we *must* be migrating to level 1.
        //
        NT_ASSERT(id >= KPH_CID_MAX_L0);

        tableL1 = KphpCidAllocateTable(KPH_CID_TABLE_L1);
        if (!tableL1)
        {
            entry = NULL;
            goto Exit;
        }

        tableL1[0] = (PKPH_CID_TABLE_ENTRY)(table & KPH_CID_TABLE_POINTER_MASK);
        table = ((ULONG_PTR)tableL1 | KPH_CID_TABLE_L1);
        level = KPH_CID_TABLE_L1;
        InterlockedExchangePointer((PVOID*)&Table->Table, (PVOID)table);

        //
        // Fall through for level 1 expansion.
        //
    }

    if (level == KPH_CID_TABLE_L1)
    {
        if (id < KPH_CID_MAX_L1)
        {
            //
            // Allocate a new block in the level 1 table.
            //
            tableL1 = (PKPH_CID_TABLE_ENTRY*)(table & KPH_CID_TABLE_POINTER_MASK);
            NT_ASSERT(!tableL1[id / KPH_CID_MAX_L0]);
            tableL1[id / KPH_CID_MAX_L0] = KphpCidAllocateTable(KPH_CID_TABLE_L0);
            tableL0 = tableL1[id / KPH_CID_MAX_L0];
            if (!tableL0)
            {
                entry = NULL;
                goto Exit;
            }

            entry = &tableL0[id % KPH_CID_MAX_L0];
            goto Exit;
        }

        //
        // We have to migrate to a level 2 table.
        //
        tableL2 = KphpCidAllocateTable(KPH_CID_TABLE_L2);
        if (!tableL2)
        {
            entry = NULL;
            goto Exit;
        }

        tableL2[0] = (PKPH_CID_TABLE_ENTRY*)(table & KPH_CID_TABLE_POINTER_MASK);
        table = ((ULONG_PTR)tableL2 | KPH_CID_TABLE_L2);
        level = KPH_CID_TABLE_L2;
        InterlockedExchangePointer((PVOID*)&Table->Table, (PVOID)table);

        //
        // Fall through for level 2 expansion
        //
    }

    NT_ASSERT(level == KPH_CID_TABLE_L2);
    NT_ASSERT(id < KPH_CID_MAX_L2);

    //
    // Allocate new block(s) in the level 2 table.
    //

    tableL2 = (PKPH_CID_TABLE_ENTRY**)(table & KPH_CID_TABLE_POINTER_MASK);
    tableL1 = tableL2[id / KPH_CID_MAX_L1];
    if (!tableL1)
    {
        tableL2[id / KPH_CID_MAX_L1] = KphpCidAllocateTable(KPH_CID_TABLE_L1);
        tableL1 = tableL2[id / KPH_CID_MAX_L1];
        if (!tableL1)
        {
            entry = NULL;
            goto Exit;
        }
    }

    NT_ASSERT(!tableL1[(id % KPH_CID_MAX_L1) / KPH_CID_MAX_L0]);
    tableL1[(id % KPH_CID_MAX_L1) / KPH_CID_MAX_L0] = KphpCidAllocateTable(KPH_CID_TABLE_L0);
    tableL0 = tableL1[(id % KPH_CID_MAX_L1) / KPH_CID_MAX_L0];
    if (!tableL0)
    {
        entry = NULL;
        goto Exit;
    }

    entry = &tableL0[(id % KPH_CID_MAX_L1) % KPH_CID_MAX_L0];

Exit:

    KeReleaseSpinLock(&Table->Lock, oldIrql);

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
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
PKPH_CID_TABLE_ENTRY KphCidGetEntry(
    _In_ HANDLE Cid,
    _Inout_ PKPH_CID_TABLE Table
    )
{
    PKPH_CID_TABLE_ENTRY entry;

    NPAGED_CODE_DISPATCH_MAX();

    entry = KphpCidLookupEntry(Cid, Table);
    if (entry)
    {
        return entry;
    }

    return KphpCidExpandTableFor(Cid, Table);
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
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
BOOLEAN KphpCidEnumerateInvokeCallback(
    _In_ PKPH_CID_TABLE_ENTRY Entry,
    _When_(!Rundown, _In_) _When_(Rundown, _Pre_null_) PKPH_CID_ENUMERATE_CALLBACK Callback,
    _When_(!Callback, _In_) _When_(Callback, _Pre_null_) PKPH_CID_RUNDOWN_CALLBACK Rundown,
    _In_opt_ PVOID Parameter
    )
{
    PVOID object;
    BOOLEAN res;

    NPAGED_CODE_DISPATCH_MAX();

    res = FALSE;

    if (Callback)
    {
        object = KphAtomicReferenceObject(&Entry->ObjectRef);

        if (object)
        {
            res = Callback(object, Parameter);

            KphDereferenceObject(object);
        }
    }
    else
    {
        NT_ASSERT(Rundown);

        object = KphAtomicMoveObjectReference(&Entry->ObjectRef, NULL);

        if (object)
        {
            Rundown(object, Parameter);

            KphDereferenceObject(object);
        }
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
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphpCidEnumerate(
    _In_ PKPH_CID_TABLE Table,
    _When_(!Rundown, _In_) _When_(Rundown, _Pre_null_) PKPH_CID_ENUMERATE_CALLBACK Callback,
    _When_(!Callback, _In_) _When_(Callback, _Pre_null_) PKPH_CID_RUNDOWN_CALLBACK Rundown,
    _In_opt_ PVOID Parameter
    )
{
    ULONG_PTR table;
    PKPH_CID_TABLE_ENTRY tableL0;
    PKPH_CID_TABLE_ENTRY* tableL1;
    PKPH_CID_TABLE_ENTRY** tableL2;

    NPAGED_CODE_DISPATCH_MAX();

    table = ReadULongPtrAcquire(&Table->Table);

    if (!table)
    {
        return;
    }

    KeEnterCriticalRegion();

    switch (table & KPH_CID_TABLE_LEVEL_MASK)
    {
        case KPH_CID_TABLE_L0:
        {
            tableL0 = (PKPH_CID_TABLE_ENTRY)(table & KPH_CID_TABLE_POINTER_MASK);

            NT_ASSERT(tableL0);

            for (ULONG i = 0; i < KPH_CID_L0_COUNT; i++)
            {
                if (KphpCidEnumerateInvokeCallback(&tableL0[i],
                                                   Callback,
                                                   Rundown,
                                                   Parameter))
                {
                    goto Exit;
                }
            }

            break;
        }
        case KPH_CID_TABLE_L1:
        {
            tableL1 = (PKPH_CID_TABLE_ENTRY*)(table & KPH_CID_TABLE_POINTER_MASK);

            NT_ASSERT(tableL1);

            for (ULONG i = 0; i < KPH_CID_L1_COUNT; i++)
            {
                tableL0 = tableL1[i];
                if (!tableL0)
                {
                    continue;
                }

                for (ULONG j = 0; j < KPH_CID_L0_COUNT; j++)
                {
                    if (KphpCidEnumerateInvokeCallback(&tableL0[j],
                                                       Callback,
                                                       Rundown,
                                                       Parameter))
                    {
                        goto Exit;
                    }
                }
            }

            break;
        }
        case KPH_CID_TABLE_L2:
        {
            tableL2 = (PKPH_CID_TABLE_ENTRY**)(table & KPH_CID_TABLE_POINTER_MASK);

            NT_ASSERT(tableL2);

            for (ULONG i = 0; i < KPH_CID_L2_COUNT; i++)
            {
                tableL1 = tableL2[i];
                if (!tableL1)
                {
                    continue;
                }

                for (ULONG j = 0; j < KPH_CID_L1_COUNT; j++)
                {
                    tableL0 = tableL1[j];
                    if (!tableL0)
                    {
                        continue;
                    }

                    for (ULONG k = 0; k < KPH_CID_L0_COUNT; k++)
                    {
                        if (KphpCidEnumerateInvokeCallback(&tableL0[k],
                                                           Callback,
                                                           Rundown,
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
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphCidEnumerate(
    _In_ PKPH_CID_TABLE Table,
    _In_ PKPH_CID_ENUMERATE_CALLBACK Callback,
    _In_opt_ PVOID Parameter
    )
{
    NPAGED_CODE_DISPATCH_MAX();

    KphpCidEnumerate(Table, Callback, NULL, Parameter);
}

/**
 * \brief Enumerates CID entires a CID table for rundown.
 *
 * \details This routine removes all items from the table. After the callback
 * returns the object reference in the table is dereferenced.
 *
 * \param[in] Table The table to enumerate.
 * \param[in] Callback The extended callback to invoke.
 * \param[in] Parameter Optional parameter passed to the callback.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphCidRundown(
    _In_ PKPH_CID_TABLE Table,
    _In_ PKPH_CID_RUNDOWN_CALLBACK Callback,
    _In_opt_ PVOID Parameter
    )
{
    NPAGED_CODE_DISPATCH_MAX();

    KphpCidEnumerate(Table, NULL, Callback, Parameter);
}
