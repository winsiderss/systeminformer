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

#include <kph.h>

#include <trace.h>

#define KPH_HANDLE_LEVEL_MASK ((ULONG_PTR)3)
#define KPH_HANDLE_L0 ((ULONG_PTR)0)
#define KPH_HANDLE_L1 ((ULONG_PTR)1)
#define KPH_HANDLE_L2 ((ULONG_PTR)2)
#define KPH_HANDLE_POINTER_MASK ~KPH_HANDLE_LEVEL_MASK
#define KphpHandleTablePointer(x) ((PVOID)((x) & KPH_HANDLE_POINTER_MASK))

#define KPH_HANDLE_L0_COUNT (PAGE_SIZE / sizeof(KPH_HANDLE_TABLE_ENTRY))
#define KPH_HANDLE_L1_COUNT (PAGE_SIZE / sizeof(PKPH_HANDLE_TABLE_ENTRY))
#define KPH_HANDLE_INDEX_BITS 24
#define KPH_HANDLE_LIMIT ((ULONG_PTR)1 << KPH_HANDLE_INDEX_BITS)
#define KPH_HANDLE_L2_COUNT                                                    \
    (KPH_HANDLE_LIMIT / (KPH_HANDLE_L0_COUNT * KPH_HANDLE_L1_COUNT))

#define KPH_HANDLE_MAX_L0 KPH_HANDLE_L0_COUNT
#define KPH_HANDLE_MAX_L1 (KPH_HANDLE_L1_COUNT * KPH_HANDLE_L0_COUNT)
#define KPH_HANDLE_MAX_L2                                                      \
    (KPH_HANDLE_L2_COUNT * KPH_HANDLE_L1_COUNT * KPH_HANDLE_L0_COUNT)

C_ASSERT(KPH_HANDLE_MAX_L2 == KPH_HANDLE_LIMIT);
C_ASSERT((KPH_HANDLE_INDEX_MASK >> 2) == (KPH_HANDLE_LIMIT - 1));

/**
 * \brief Retrieves the index a handle names.
 *
 * \param[in] Handle The handle.
 * \param[out] Index Receives the index.
 *
 * \return TRUE when the handle is well formed, FALSE otherwise.
 */
_IRQL_requires_max_(HIGH_LEVEL)
BOOLEAN KphpHandleToIndex(
    _In_ HANDLE Handle,
    _Out_ PULONG_PTR Index
    )
{
    ULONG_PTR value;

    KPH_NPAGED_CODE_HIGH_MAX();

    value = (ULONG_PTR)Handle;

    if ((value & ~KPH_HANDLE_INDEX_MASK) != KPH_HANDLE_TAG)
    {
        *Index = 0;
        return FALSE;
    }

    *Index = ((value & KPH_HANDLE_INDEX_MASK) >> 2);
    return (*Index != 0);
}

/**
 * \brief Looks up an entry in a handle table.
 *
 * \param[in] Table The table.
 * \param[in] Index The index of the entry.
 *
 * \return The entry, NULL if no block covers the index.
 */
_IRQL_requires_max_(HIGH_LEVEL)
_Must_inspect_result_
PKPH_HANDLE_TABLE_ENTRY KphpHandleLookupEntry(
    _In_ PKPH_HANDLE_TABLE Table,
    _In_ ULONG_PTR Index
    )
{
    ULONG_PTR table;
    PKPH_HANDLE_TABLE_ENTRY tableL0;
    PKPH_HANDLE_TABLE_ENTRY* tableL1;
    PKPH_HANDLE_TABLE_ENTRY** tableL2;

    KPH_NPAGED_CODE_HIGH_MAX();

    table = ReadULongPtrAcquire(&Table->Table);

    switch (table & KPH_HANDLE_LEVEL_MASK)
    {
        case KPH_HANDLE_L0:
        {
            if (Index >= KPH_HANDLE_MAX_L0)
            {
                return NULL;
            }

            tableL0 = KphpHandleTablePointer(table);
            if (!tableL0)
            {
                return NULL;
            }

            return &tableL0[Index];
        }
        case KPH_HANDLE_L1:
        {
            if (Index >= KPH_HANDLE_MAX_L1)
            {
                return NULL;
            }

            tableL1 = KphpHandleTablePointer(table);
            tableL0 = ReadPointerAcquire(&tableL1[Index / KPH_HANDLE_MAX_L0]);
            if (!tableL0)
            {
                return NULL;
            }

            return &tableL0[Index % KPH_HANDLE_MAX_L0];
        }
        case KPH_HANDLE_L2:
        {
            if (Index >= KPH_HANDLE_MAX_L2)
            {
                return NULL;
            }

            tableL2 = KphpHandleTablePointer(table);
            tableL1 = ReadPointerAcquire((PVOID*)&tableL2[Index / KPH_HANDLE_MAX_L1]);
            if (!tableL1)
            {
                return NULL;
            }

            tableL0 = ReadPointerAcquire(&tableL1[(Index % KPH_HANDLE_MAX_L1) / KPH_HANDLE_MAX_L0]);
            if (!tableL0)
            {
                return NULL;
            }

            return &tableL0[(Index % KPH_HANDLE_MAX_L1) % KPH_HANDLE_MAX_L0];
        }
        default:
        {
            NT_ASSERT(FALSE);
            return NULL;
        }
    }
}

/**
 * \brief Grows a handle table by one block of entries.
 *
 * \param[in,out] Table The table to grow.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHandleGrowTable(
    _Inout_ PKPH_HANDLE_TABLE Table
    )
{
    NTSTATUS status;
    ULONG_PTR nextIndex;
    ULONG_PTR table;
    ULONG_PTR level;
    PKPH_HANDLE_TABLE_ENTRY block;
    PKPH_HANDLE_TABLE_ENTRY* newL1;
    PKPH_HANDLE_TABLE_ENTRY** newL2;
    PKPH_HANDLE_TABLE_ENTRY* tableL1;
    PKPH_HANDLE_TABLE_ENTRY** tableL2;
    ULONG_PTR first;
    KIRQL oldIrql;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    block = NULL;
    newL1 = NULL;
    newL2 = NULL;

    nextIndex = ReadULongPtrAcquire(&Table->NextIndex);
    if (nextIndex >= KPH_HANDLE_LIMIT)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "Handle table is full");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    table = ReadULongPtrAcquire(&Table->Table);
    level = (table & KPH_HANDLE_LEVEL_MASK);

    block = KphAllocateNPaged(PAGE_SIZE, KPH_TAG_HANDLE_TABLE_BLOCK);
    if (!block)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "Failed to allocate handle table block");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    if (((level == KPH_HANDLE_L0) && (nextIndex == KPH_HANDLE_MAX_L0)) ||
        ((level == KPH_HANDLE_L1) && (nextIndex == KPH_HANDLE_MAX_L1)) ||
        ((level == KPH_HANDLE_L2) && ((nextIndex % KPH_HANDLE_MAX_L1) == 0)))
    {
        newL1 = KphAllocateNPaged(PAGE_SIZE, KPH_TAG_HANDLE_TABLE_BLOCK);
        if (!newL1)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          TRACKING,
                          "Failed to allocate handle table block");

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }
    }

    if ((level == KPH_HANDLE_L1) && (nextIndex == KPH_HANDLE_MAX_L1))
    {
        newL2 = KphAllocateNPaged(KPH_HANDLE_L2_COUNT *
                                  sizeof(PKPH_HANDLE_TABLE_ENTRY*),
                                  KPH_TAG_HANDLE_TABLE_BLOCK);
        if (!newL2)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          TRACKING,
                          "Failed to allocate handle table block");

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }
    }

    KeAcquireSpinLock(&Table->Lock, &oldIrql);

    if ((Table->FirstFree != 0) ||
        (Table->NextIndex != nextIndex) ||
        (Table->Table != table))
    {
        KeReleaseSpinLock(&Table->Lock, oldIrql);
        status = STATUS_SUCCESS;
        goto Exit;
    }

    first = ((nextIndex == 0) ? 1 : nextIndex);
    for (ULONG_PTR i = (first - nextIndex); i < (KPH_HANDLE_L0_COUNT - 1); i++)
    {
        block[i].NextFree = (nextIndex + i + 1);
    }

    if (level == KPH_HANDLE_L0)
    {
        if (!newL1)
        {
            InterlockedExchangePointer((PVOID*)&Table->Table, block);
        }
        else
        {
            newL1[0] = KphpHandleTablePointer(table);
            newL1[1] = block;
            InterlockedExchangePointer((PVOID*)&Table->Table,
                                       (PVOID)((ULONG_PTR)newL1 |
                                               KPH_HANDLE_L1));
            newL1 = NULL;
        }
    }
    else if (level == KPH_HANDLE_L1)
    {
        tableL1 = KphpHandleTablePointer(table);

        if (!newL2)
        {
            WritePointerRelease(&tableL1[nextIndex / KPH_HANDLE_MAX_L0], block);
        }
        else
        {
            NT_ASSERT(newL1);

            newL1[0] = block;
            newL2[0] = tableL1;
            newL2[1] = newL1;
            InterlockedExchangePointer((PVOID*)&Table->Table,
                                       (PVOID)((ULONG_PTR)newL2 |
                                               KPH_HANDLE_L2));
            newL1 = NULL;
            newL2 = NULL;
        }
    }
    else
    {
        NT_ASSERT(level == KPH_HANDLE_L2);

        tableL2 = KphpHandleTablePointer(table);

        if (newL1)
        {
            newL1[0] = block;
            WritePointerRelease((PVOID*)&tableL2[nextIndex / KPH_HANDLE_MAX_L1],
                                newL1);
            newL1 = NULL;
        }
        else
        {
            tableL1 = tableL2[nextIndex / KPH_HANDLE_MAX_L1];
            WritePointerRelease(
                &tableL1[(nextIndex % KPH_HANDLE_MAX_L1) / KPH_HANDLE_MAX_L0],
                block
                );
        }
    }

    block = NULL;
    Table->FirstFree = first;
    WriteULongPtrRelease(&Table->NextIndex, nextIndex + KPH_HANDLE_L0_COUNT);

    KeReleaseSpinLock(&Table->Lock, oldIrql);

    status = STATUS_SUCCESS;

Exit:

    if (block)
    {
        KphFree(block, KPH_TAG_HANDLE_TABLE_BLOCK);
    }

    if (newL1)
    {
        KphFree(newL1, KPH_TAG_HANDLE_TABLE_BLOCK);
    }

    if (newL2)
    {
        KphFree(newL2, KPH_TAG_HANDLE_TABLE_BLOCK);
    }

    return status;
}

/**
 * \brief Retrieves the handle table of a process, creating it if asked.
 *
 * \param[in,out] Process The process.
 * \param[in] Create TRUE to create the table when the process has none.
 *
 * \return The table, NULL if there is none or it could not be created.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
PKPH_HANDLE_TABLE KphpHandleGetTable(
    _Inout_ PKPH_PROCESS_CONTEXT Process,
    _In_ BOOLEAN Create
    )
{
    PKPH_HANDLE_TABLE table;
    PKPH_HANDLE_TABLE newTable;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    newTable = NULL;

    table = ReadPointerAcquire(&Process->HandleTable);
    if (table || !Create)
    {
        goto Exit;
    }

    newTable = KphAllocateNPaged(sizeof(KPH_HANDLE_TABLE),
                                 KPH_TAG_HANDLE_TABLE);
    if (!newTable)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "Failed to allocate handle table");

        goto Exit;
    }

    KeInitializeSpinLock(&newTable->Lock);

    table = InterlockedCompareExchangePointer(&Process->HandleTable,
                                              newTable,
                                              NULL);
    if (!table)
    {
        table = newTable;
        newTable = NULL;
    }

Exit:

    if (newTable)
    {
        KphFree(newTable, KPH_TAG_HANDLE_TABLE);
    }

    return table;
}

/**
 * \brief Closes the entry of an index.
 *
 * \param[in,out] Table The table.
 * \param[in] Index The index.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS KphpHandleCloseIndex(
    _Inout_ PKPH_HANDLE_TABLE Table,
    _In_ ULONG_PTR Index
    )
{
    PKPH_HANDLE_TABLE_ENTRY entry;
    PVOID object;
    KIRQL oldIrql;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    entry = KphpHandleLookupEntry(Table, Index);
    if (!entry)
    {
        return STATUS_INVALID_HANDLE;
    }

    object = KphAtomicMoveObjectReference(&entry->ObjectRef, NULL);
    if (!object)
    {
        return STATUS_INVALID_HANDLE;
    }

    KeAcquireSpinLock(&Table->Lock, &oldIrql);
    entry->NextFree = Table->FirstFree;
    Table->FirstFree = Index;
    KeReleaseSpinLock(&Table->Lock, oldIrql);

    KphDereferenceObjectDeferDelete(object);

    return STATUS_SUCCESS;
}

/**
 * \brief Creates a handle to an object in the handle table of a process.
 *
 * \param[in,out] Process The process.
 * \param[in] Object The object. The table takes a reference to it.
 * \param[out] Handle Receives the handle.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS KphpCreateHandle(
    _Inout_ PKPH_PROCESS_CONTEXT Process,
    _In_ PVOID Object,
    _Out_ PHANDLE Handle
    )
{
    NTSTATUS status;
    PKPH_HANDLE_TABLE table;
    PKPH_HANDLE_TABLE_ENTRY entry;
    PVOID previous;
    ULONG_PTR index;
    KIRQL oldIrql;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    *Handle = NULL;

    table = KphpHandleGetTable(Process, TRUE);
    if (!table)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "KphpHandleGetTable failed");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    for (;;)
    {
        KeAcquireSpinLock(&table->Lock, &oldIrql);

        index = table->FirstFree;
        if (index != 0)
        {
            entry = KphpHandleLookupEntry(table, index);
            NT_ASSERT(entry);
            table->FirstFree = entry->NextFree;
            KeReleaseSpinLock(&table->Lock, oldIrql);
            break;
        }

        KeReleaseSpinLock(&table->Lock, oldIrql);

        status = KphpHandleGrowTable(table);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          TRACKING,
                          "KphpHandleGrowTable failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }

    KphReferenceObject(Object);
    previous = KphAtomicMoveObjectReference(&entry->ObjectRef, Object);
    NT_VERIFY(!previous);

    *Handle = (HANDLE)(KPH_HANDLE_TAG | (index << 2));

    status = STATUS_SUCCESS;

Exit:

    return status;
}

/**
 * \brief Creates a handle to an object.
 *
 * \param[in] Object The object. The handle table takes a reference to it.
 * \param[in] AccessMode The processor mode of the caller.
 * \param[out] Handle Receives the handle.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS KphCreateHandle(
    _In_ PVOID Object,
    _In_ KPROCESSOR_MODE AccessMode,
    _Out_ PHANDLE Handle
    )
{
    NTSTATUS status;
    PKPH_PROCESS_CONTEXT process;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    *Handle = NULL;

    process = KphGetProcessContextForMode(AccessMode);
    if (!process)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "KphGetProcessContextForMode failed");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphpCreateHandle(process, Object, Handle);

Exit:

    if (process)
    {
        KphDereferenceObject(process);
    }

    return status;
}

/**
 * \brief References the object a handle of a process names.
 *
 * \param[in] Process The process containing the handle.
 * \param[in] Handle The handle to reference the object of.
 * \param[in] ObjectType The type the object must have.
 * \param[out] Object Receives the referenced object. The caller must
 * dereference it.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS KphpReferenceObjectByHandle(
    _In_ PKPH_PROCESS_CONTEXT Process,
    _In_ HANDLE Handle,
    _In_ PKPH_OBJECT_TYPE ObjectType,
    _Outptr_result_nullonfailure_ PVOID* Object
    )
{
    NTSTATUS status;
    PKPH_HANDLE_TABLE table;
    PKPH_HANDLE_TABLE_ENTRY entry;
    PVOID object;
    ULONG_PTR index;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    *Object = NULL;

    if (!KphpHandleToIndex(Handle, &index))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "Invalid handle %p",
                      Handle);

        status = STATUS_INVALID_HANDLE;
        goto Exit;
    }

    table = KphpHandleGetTable(Process, FALSE);
    if (!table)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "No handle table for handle %p",
                      Handle);

        status = STATUS_INVALID_HANDLE;
        goto Exit;
    }

    entry = KphpHandleLookupEntry(table, index);
    if (!entry)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "No handle table entry for handle %p",
                      Handle);

        status = STATUS_INVALID_HANDLE;
        goto Exit;
    }

    object = KphAtomicReferenceObject(&entry->ObjectRef);
    if (!object)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "Handle %p is not open",
                      Handle);

        status = STATUS_INVALID_HANDLE;
        goto Exit;
    }

    if (KphGetObjectType(object) != ObjectType)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "Handle %p is of another type",
                      Handle);

        KphDereferenceObjectDeferDelete(object);
        status = STATUS_OBJECT_TYPE_MISMATCH;
        goto Exit;
    }

    *Object = object;
    status = STATUS_SUCCESS;

Exit:

    return status;
}

/**
 * \brief References the object a handle names.
 *
 * \param[in] Handle The handle to reference the object of.
 * \param[in] ObjectType The type the object must have.
 * \param[in] AccessMode The processor mode of the caller.
 * \param[out] Object Receives the referenced object. The caller must
 * dereference it.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS KphReferenceObjectByHandle(
    _In_ HANDLE Handle,
    _In_ PKPH_OBJECT_TYPE ObjectType,
    _In_ KPROCESSOR_MODE AccessMode,
    _Outptr_result_nullonfailure_ PVOID* Object
    )
{
    NTSTATUS status;
    PKPH_PROCESS_CONTEXT process;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    *Object = NULL;

    process = KphGetProcessContextForMode(AccessMode);
    if (!process)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "KphGetProcessContextForMode failed");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphpReferenceObjectByHandle(process, Handle, ObjectType, Object);

Exit:

    if (process)
    {
        KphDereferenceObject(process);
    }

    return status;
}

/**
 * \brief Closes a handle of a process.
 *
 * \param[in] Process The process for which the handle is to be closed.
 * \param[in] Handle The handle to close.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS KphpCloseHandle(
    _In_ PKPH_PROCESS_CONTEXT Process,
    _In_ HANDLE Handle
    )
{
    NTSTATUS status;
    PKPH_HANDLE_TABLE table;
    ULONG_PTR index;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    if (!KphpHandleToIndex(Handle, &index))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "Invalid handle %p",
                      Handle);

        status = STATUS_INVALID_HANDLE;
        goto Exit;
    }

    table = KphpHandleGetTable(Process, FALSE);
    if (!table)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "No handle table for handle %p",
                      Handle);

        status = STATUS_INVALID_HANDLE;
        goto Exit;
    }

    status = KphpHandleCloseIndex(table, index);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "Handle %p is not open",
                      Handle);
    }

Exit:

    return status;
}

/**
 * \brief Closes a handle.
 *
 * \param[in] Handle The handle to close.
 * \param[in] AccessMode The processor mode of the caller.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS KphCloseHandle(
    _In_ HANDLE Handle,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PKPH_PROCESS_CONTEXT process;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    process = KphGetProcessContextForMode(AccessMode);
    if (!process)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "KphGetProcessContextForMode failed");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphpCloseHandle(process, Handle);

Exit:

    if (process)
    {
        KphDereferenceObject(process);
    }

    return status;
}

/**
 * \brief Closes every handle of a process.
 *
 * \param[in] Process The process.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphRundownHandleTable(
    _In_ PKPH_PROCESS_CONTEXT Process
    )
{
    PKPH_HANDLE_TABLE table;
    ULONG_PTR nextIndex;
    KIRQL oldIrql;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    table = KphpHandleGetTable(Process, FALSE);
    if (!table)
    {
        return;
    }

    KeAcquireSpinLock(&table->Lock, &oldIrql);
    nextIndex = table->NextIndex;
    KeReleaseSpinLock(&table->Lock, oldIrql);

    for (ULONG_PTR i = 1; i < nextIndex; i++)
    {
        KphpHandleCloseIndex(table, i);
    }
}

/**
 * \brief Closes every handle of a process and frees its handle table.
 *
 * \param[in,out] Process The process.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphDeleteHandleTable(
    _Inout_ PKPH_PROCESS_CONTEXT Process
    )
{
    PKPH_HANDLE_TABLE handleTable;
    ULONG_PTR table;
    PKPH_HANDLE_TABLE_ENTRY tableL0;
    PKPH_HANDLE_TABLE_ENTRY* tableL1;
    PKPH_HANDLE_TABLE_ENTRY** tableL2;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    handleTable = KphpHandleGetTable(Process, FALSE);
    if (!handleTable)
    {
        return;
    }

    KphRundownHandleTable(Process);

    table = ReadULongPtrAcquire(&handleTable->Table);

    switch (table & KPH_HANDLE_LEVEL_MASK)
    {
        case KPH_HANDLE_L0:
        {
            tableL0 = KphpHandleTablePointer(table);
            if (tableL0)
            {
                KphFree(tableL0, KPH_TAG_HANDLE_TABLE_BLOCK);
            }

            break;
        }
        case KPH_HANDLE_L1:
        {
            tableL1 = KphpHandleTablePointer(table);

            for (ULONG i = 0; i < KPH_HANDLE_L1_COUNT; i++)
            {
#pragma prefast(suppress : 6001) // memory is initialized
                if (tableL1[i])
                {
                    KphFree(tableL1[i], KPH_TAG_HANDLE_TABLE_BLOCK);
                }
            }

            KphFree(tableL1, KPH_TAG_HANDLE_TABLE_BLOCK);

            break;
        }
        case KPH_HANDLE_L2:
        {
            tableL2 = KphpHandleTablePointer(table);

            for (ULONG i = 0; i < KPH_HANDLE_L2_COUNT; i++)
            {
                tableL1 = tableL2[i];
                if (!tableL1)
                {
                    continue;
                }

                for (ULONG j = 0; j < KPH_HANDLE_L1_COUNT; j++)
                {
#pragma prefast(suppress : 6001) // memory is initialized
                    if (tableL1[j])
                    {
                        KphFree(tableL1[j], KPH_TAG_HANDLE_TABLE_BLOCK);
                    }
                }

                KphFree(tableL1, KPH_TAG_HANDLE_TABLE_BLOCK);
            }

            KphFree(tableL2, KPH_TAG_HANDLE_TABLE_BLOCK);

            break;
        }
        default:
        {
            NT_ASSERT(FALSE);
            break;
        }
    }

    KphFree(handleTable, KPH_TAG_HANDLE_TABLE);

    Process->HandleTable = NULL;
}
