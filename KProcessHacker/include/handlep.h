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

#ifndef _HANDLEP_H
#define _HANDLEP_H

#define _HANDLE_PRIVATE
#include "handle.h"
#include "sync.h"

#define KPH_HANDLE_INCREMENT 4
#define KPH_HANDLE_LOCKED 0x1
#define KPH_HANDLE_LOCKED_SHIFT 0
#define KPH_HANDLE_ALLOCATED 0x2
#define KPH_HANDLE_FLAGS 0x3

#define KphGetFlagsEntry(Entry) ((Entry)->Value & KPH_HANDLE_FLAGS)
#define KphGetHandleEntry(Entry) ((HANDLE)((Entry)->Value & ~KPH_HANDLE_FLAGS))
#define KphIncrementHandle(Handle) ((HANDLE)((ULONG_PTR)(Handle) + KPH_HANDLE_INCREMENT))

#define KphIsAllocatedEntry(Entry) ((Entry)->Value & KPH_HANDLE_ALLOCATED)
#define KphClearAllocatedEntry(Entry) ((Entry)->Value &= ~KPH_HANDLE_ALLOCATED)
#define KphSetAllocatedEntry(Entry) ((Entry)->Value |= KPH_HANDLE_ALLOCATED)

#define KphGetNextFreeEntry(Entry) ((PKPH_HANDLE_TABLE_ENTRY)((Entry)->Value & ~KPH_HANDLE_FLAGS))
#define KphSetNextFreeEntry(Entry, NextFree) ((Entry)->Value = ((ULONG_PTR)(NextFree) | KphGetFlagsEntry(Entry)))

#define KphHandleFromIndex(Index) ((HANDLE)((ULONG_PTR)(Index) * KPH_HANDLE_INCREMENT))
#define KphHandleFromIndexEx(Index, Flags) ((HANDLE)(((Index) * KPH_HANDLE_INCREMENT) | (Flags)))
#define KphIndexFromHandle(Handle) (((ULONG_PTR)(Handle) & ~KPH_HANDLE_FLAGS) / KPH_HANDLE_INCREMENT)

#define KphEntryFromHandle(HandleTable, Handle) KphEntryFromIndex((HandleTable), KphIndexFromHandle(Handle))
#define KphEntryFromIndex(HandleTable, Index) \
    ((PKPH_HANDLE_TABLE_ENTRY)((ULONG_PTR)(HandleTable)->Table + (Index) * (HandleTable)->SizeOfEntry))
#define KphHandleFromEntry(HandleTable, Entry) KphHandleFromIndex(KphIndexFromEntry((HandleTable), (Entry)))
#define KphHandleFromEntryEx(HandleTable, Entry, Flags) \
    KphHandleFromIndexEx(KphIndexFromEntry((HandleTable), (Entry)), (Flags))
#define KphIndexFromEntry(HandleTable, Entry) \
    (((ULONG_PTR)(Entry) - (ULONG_PTR)(HandleTable)->Table) / (HandleTable)->SizeOfEntry)

typedef struct _KPH_HANDLE_TABLE
{
    /* The pool tag used for this descriptor and the table itself. */
    ULONG Tag;
    /* The size of each handle table entry. */
    ULONG SizeOfEntry;
    /* The next handle value to use. */
    HANDLE NextHandle;
    /* The free list of handle table entries. */
    struct _KPH_HANDLE_TABLE_ENTRY *FreeHandle;
    
    /* A fast mutex guarding writes to the handle table. */
    FAST_MUTEX Mutex;
    /* The size of the table, in bytes. */
    ULONG TableSize;
    /* The actual handle table. */
    PVOID Table;
} KPH_HANDLE_TABLE, *PKPH_HANDLE_TABLE;

FORCEINLINE BOOLEAN KphLockHandleEntry(
    __inout PKPH_HANDLE_TABLE_ENTRY Entry
    );

FORCEINLINE BOOLEAN KphLockAllocatedHandleEntry(
    __inout PKPH_HANDLE_TABLE_ENTRY Entry
    );

FORCEINLINE VOID KphUnlockHandleEntry(
    __inout PKPH_HANDLE_TABLE_ENTRY Entry
    );

/* KphLockHandle
 * 
 * Locks a handle table entry for exclusive access. Do not 
 * modify the lowest bit of the entry's value while you 
 * hold the lock.
 * 
 * Return value: TRUE if the entry is allocated, otherwise FALSE.
 */
FORCEINLINE BOOLEAN KphLockHandleEntry(
    __inout PKPH_HANDLE_TABLE_ENTRY Entry
    )
{
    /* Acquire the spinlock. */
    KphAcquireBitSpinLock((PLONG)&Entry->Value, KPH_HANDLE_LOCKED_SHIFT);
    
    /* Return whether the entry is allocated. */
    return !!(Entry->Value & KPH_HANDLE_ALLOCATED);
}

/* KphLockAllocatedHandle
 * 
 * Locks a handle table entry for exclusive access. Do not 
 * modify the lowest bit of the entry's value while you 
 * hold the lock.
 * The function will not lock the handle if it is unallocated.
 * 
 * Return value: TRUE if the entry was locked, otherwise FALSE.
 */
FORCEINLINE BOOLEAN KphLockAllocatedHandleEntry(
    __inout PKPH_HANDLE_TABLE_ENTRY Entry
    )
{
    if (!KphLockHandleEntry(Entry))
    {
        KphUnlockHandleEntry(Entry);
        return FALSE;
    }
    
    return TRUE;
}

/* KphUnlockHandle
 * 
 * Unlocks a handle table entry.
 */
FORCEINLINE VOID KphUnlockHandleEntry(
    __inout PKPH_HANDLE_TABLE_ENTRY Entry
    )
{
    /* Unlock the spinlock. */
    KphReleaseBitSpinLock((PLONG)&Entry->Value, KPH_HANDLE_LOCKED_SHIFT);
}

#endif
