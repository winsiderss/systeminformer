/*
 * Process Hacker Driver - 
 *   handle table
 * 
 * Copyright (C) 2009 wj32
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

#include "include/handle.h"
#include "include/handlep.h"

NTSTATUS KphpAllocateHandleEntry(
    __in PKPH_HANDLE_TABLE HandleTable,
    __out PKPH_HANDLE_TABLE_ENTRY *Entry
    );

NTSTATUS KphpFreeHandleEntry(
    __in PKPH_HANDLE_TABLE HandleTable,
    __in PKPH_HANDLE_TABLE_ENTRY Entry
    );

/* KphCreateHandleTable
 * 
 * Creates a handle table.
 * 
 * HandleTable: A variable which receives a pointer to the handle table.
 * MaximumHandles: The maximum number of handles that can be created.
 * SizeOfEntry: The size of each handle table entry. This value must be 
 * divisible by 4.
 * Tag: The tag to use when allocating handle table resources.
 */
NTSTATUS KphCreateHandleTable(
    __out PKPH_HANDLE_TABLE *HandleTable,
    __in ULONG MaximumHandles,
    __in ULONG SizeOfEntry,
    __in ULONG Tag
    )
{
    PKPH_HANDLE_TABLE handleTable;
    
    /* Each handle entry must be at least the size of our 
     * handle table entry definition.
     */
    if (SizeOfEntry < sizeof(KPH_HANDLE_TABLE_ENTRY))
        return STATUS_INVALID_PARAMETER_3;
    /* Handle entries must be 4-byte aligned. */
    if (SizeOfEntry % 4 != 0)
        return STATUS_INVALID_PARAMETER_3;
    
    /* Allocate storage for the handle table structure. */
    handleTable = ExAllocatePoolWithTag(
        PagedPool,
        sizeof(KPH_HANDLE_TABLE),
        Tag
        );
    
    if (!handleTable)
        return STATUS_INSUFFICIENT_RESOURCES;
    
    /* Allocate storage for the handle table itself. */
    handleTable->Table = ExAllocatePoolWithTag(
        PagedPool,
        MaximumHandles * SizeOfEntry,
        Tag
        );
    
    if (!handleTable->Table)
    {
        ExFreePoolWithTag(handleTable, Tag);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    /* Initialize the rest of the table descriptor. */
    handleTable->Tag = Tag;
    handleTable->SizeOfEntry = SizeOfEntry;
    handleTable->NextHandle = (HANDLE)0;
    handleTable->FreeHandle = NULL;
    ExInitializeFastMutex(&handleTable->Mutex);
    handleTable->TableSize = MaximumHandles * SizeOfEntry;
    
    /* Zero the handle table. */
    memset(handleTable->Table, 0, handleTable->TableSize);
    
    /* Pass the pointer to the handle table back. */
    *HandleTable = handleTable;
    
    return STATUS_SUCCESS;
}

/* KphFreeHandleTable
 * 
 * Frees all handle table resources.
 */
VOID KphFreeHandleTable(
    __in PKPH_HANDLE_TABLE HandleTable
    )
{
    ULONG i;
    ULONG tag;
    
    /* Free all handle values. */
    for (i = 0; i < HandleTable->TableSize / HandleTable->SizeOfEntry; i++)
    {
        KphCloseHandle(HandleTable, KphHandleFromIndex(i));
    }
    
    /* Save the handle table tag first. */
    tag = HandleTable->Tag;
    /* Free the table. */
    ExFreePoolWithTag(HandleTable->Table, tag);
    /* Free the descriptor. */
    ExFreePoolWithTag(HandleTable, tag);
}

/* KphCloseHandle
 * 
 * Closes a handle, dereferencing the referenced object.
 */
NTSTATUS KphCloseHandle(
    __in PKPH_HANDLE_TABLE HandleTable,
    __in HANDLE Handle
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PKPH_HANDLE_TABLE_ENTRY entry;
    PVOID object;
    
    if (!KphValidHandle(HandleTable, Handle, &entry))
        return STATUS_INVALID_HANDLE;
    
    /* Save a pointer to the object referenced by the handle. */
    object = entry->Object;
    /* Free the handle. */
    status = KphpFreeHandleEntry(HandleTable, entry);
    
    if (!NT_SUCCESS(status))
        return status;
    
    /* Dereference the object. */
    KphDereferenceObject(object);
    
    return status;
}

/* KphCreateHandle
 * 
 * Creates a handle and references an object.
 */
NTSTATUS KphCreateHandle(
    __in PKPH_HANDLE_TABLE HandleTable,
    __in PVOID Object,
    __out PHANDLE Handle
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PKPH_HANDLE_TABLE_ENTRY entry;
    
    /* Allocate a handle. */
    status = KphpAllocateHandleEntry(HandleTable, &entry);
    
    if (!NT_SUCCESS(status))
        return status;
    
    /* Reference and set the object in the entry. */
    KphReferenceObject(Object);
    entry->Object = Object;
    
    /* Pass the handle back. */
    *Handle = KphGetHandleEntry(entry);
    
    return status;
}

/* KphReferenceObjectByHandle
 * 
 * References an object from a handle.
 */
NTSTATUS KphReferenceObjectByHandle(
    __in PKPH_HANDLE_TABLE HandleTable,
    __in HANDLE Handle,
    __in_opt PKPH_OBJECT_TYPE ObjectType,
    __out PVOID *Object
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PKPH_HANDLE_TABLE_ENTRY entry;
    
    if (!KphValidHandle(HandleTable, Handle, &entry))
        return STATUS_INVALID_HANDLE;
    
    /* Lock the entry. */
    if (!KphLockAllocatedHandleEntry(entry))
        return STATUS_INVALID_HANDLE;
    
    /* Check the type of object if the caller requested us 
     * to do that.
     */
    if (ObjectType)
    {
        if (KphGetObjectType(entry->Object) != ObjectType)
        {
            /* Bad type. */
            KphUnlockHandleEntry(entry);
            
            return STATUS_OBJECT_TYPE_MISMATCH;
        }
    }
    
    /* Reference and pass the object back. */
    KphReferenceObject(entry->Object);
    *Object = entry->Object;
    
    KphUnlockHandleEntry(entry);
    
    return status;
}

/* KphValidHandle
 * 
 * Checks if a handle is valid.
 */
BOOLEAN KphValidHandle(
    __in PKPH_HANDLE_TABLE HandleTable,
    __in HANDLE Handle,
    __out_opt PKPH_HANDLE_TABLE_ENTRY *Entry
    )
{
    PKPH_HANDLE_TABLE_ENTRY entry;
    BOOLEAN valid;
    
    entry = KphEntryFromHandle(HandleTable, Handle);
    valid =
        ((ULONG_PTR)entry >= (ULONG_PTR)HandleTable->Table) && 
        ((ULONG_PTR)entry + HandleTable->SizeOfEntry <= 
            (ULONG_PTR)HandleTable->Table + HandleTable->TableSize);
    
    if (valid)
        *Entry = entry;
    
    return valid;
}

/* KphpAllocateHandleEntry
 * 
 * Allocates a handle table entry.
 */
NTSTATUS KphpAllocateHandleEntry(
    __in PKPH_HANDLE_TABLE HandleTable,
    __out PKPH_HANDLE_TABLE_ENTRY *Entry
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PKPH_HANDLE_TABLE_ENTRY entry = NULL;
    
    /* Prevent others from modifying the handle table. */
    ExAcquireFastMutex(&HandleTable->Mutex);
    
    /* Check the free list first. If we have a free entry, 
     * claim it and update the free list. Otherwise, create 
     * a new entry from the NextHandle value.
     */
    if (HandleTable->FreeHandle)
    {
        /* We have a free entry. Update the free list. */
        entry = HandleTable->FreeHandle;
        /* The next free entry goes into FreeHandle. */
        HandleTable->FreeHandle = KphGetNextFreeEntry(entry);
    }
    else
    {
        /* No free handles. We have to initialize a new one 
         * based on the NextHandle value.
         */
        /* Make sure we don't go past the end of the table. */
        if (
            KphIndexFromHandle(HandleTable->NextHandle) * 
            HandleTable->SizeOfEntry <= 
            HandleTable->TableSize
            )
        {
            /* Get a pointer to the entry from the handle. */
            entry = KphEntryFromHandle(HandleTable, HandleTable->NextHandle);
            /* Increment the next handle value. */
            HandleTable->NextHandle = KphIncrementHandle(HandleTable->NextHandle);
        }
        else
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    
    if (NT_SUCCESS(status))
    {
        /* Set the entry's handle value. */
        entry->Handle = KphHandleFromEntry(HandleTable, entry);
        KphSetAllocatedEntry(entry);
        
        *Entry = entry;
    }
    
    ExReleaseFastMutex(&HandleTable->Mutex);
    
    return status;
}

/* KphpFreeHandleEntry
 * 
 * Frees a handle table entry.
 */
NTSTATUS KphpFreeHandleEntry(
    __in PKPH_HANDLE_TABLE HandleTable,
    __in PKPH_HANDLE_TABLE_ENTRY Entry
    )
{
    ExAcquireFastMutex(&HandleTable->Mutex);
    
    /* Lock the entry. */
    if (!KphLockAllocatedHandleEntry(Entry))
    {
        /* Someone else has already freed the entry (or it was never allocated). */
        ExReleaseFastMutex(&HandleTable->Mutex);
        return STATUS_INVALID_HANDLE;
    }
    
    /* Mark the entry as unallocated. */
    KphClearAllocatedEntry(Entry);
    
    /* Add the entry to the free list. */
    KphSetNextFreeEntry(Entry, HandleTable->FreeHandle);
    HandleTable->FreeHandle = Entry;
    
    /* Zero the entry (except for the Value). */
    memset(&Entry->Object, 0, HandleTable->SizeOfEntry - sizeof(ULONG_PTR));
    
    /* Unlock the entry. */
    KphUnlockHandleEntry(Entry);
    
    ExReleaseFastMutex(&HandleTable->Mutex);
    
    return STATUS_SUCCESS;
}
