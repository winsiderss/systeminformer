/*
 * Process Hacker Driver - 
 *   executive
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

#ifndef _EX_H
#define _EX_H

/* HACK - version.c dependency */
#define WINDOWS_XP 51
#define WINDOWS_SERVER_2003 52
#define WINDOWS_VISTA 60
#define WINDOWS_7 61

extern ULONG WindowsVersion;

/* Handles */

struct _HANDLE_TABLE;
struct _HANDLE_TABLE_ENTRY;

typedef BOOLEAN (NTAPI *PEX_ENUM_HANDLE_CALLBACK)(
    struct _HANDLE_TABLE_ENTRY *HandleTableEntry,
    HANDLE Handle,
    PVOID Context
    );

BOOLEAN NTAPI ExEnumHandleTable(
    __in struct _HANDLE_TABLE *HandleTable,
    __in PEX_ENUM_HANDLE_CALLBACK EnumHandleProcedure,
    __inout PVOID Context,
    __out_opt PHANDLE Handle
    );

/* Push Locks */

/* Definition for Windows 2003 and above. This means we 
 * MUST use the slow path on Windows XP.
 */
typedef struct _EXI_PUSH_LOCK
{
    union
    {
        struct
        {
            ULONG_PTR Locked : 1;
            ULONG_PTR Waiting : 1;
            ULONG_PTR Waking : 1;
            ULONG_PTR MultipleShared : 1;
            ULONG_PTR Shared : sizeof(ULONG_PTR) * 8 - 4; /* ULONG_PTR bits minus 4 */
        };
        ULONG_PTR Value;
        PVOID Ptr;
    };
} EXI_PUSH_LOCK, *PEXI_PUSH_LOCK;

#define EX_PUSH_LOCK_LOCK_SHIFT 0
#define EX_PUSH_LOCK_LOCK ((ULONG_PTR)0x1)
/* Indicates chained waiters */
#define EX_PUSH_LOCK_WAITING ((ULONG_PTR)0x2)
/* Traversing the list */
#define EX_PUSH_LOCK_WAKING ((ULONG_PTR)0x4)
/* Multiple owners + waiters */
#define EX_PUSH_LOCK_MULTIPLE_SHARED ((ULONG_PTR)0x8)

#define EX_PUSH_LOCK_SHARE_INC ((ULONG_PTR)0x10)
#define EX_PUSH_LOCK_PTR_BITS ((ULONG_PTR)0xf)

NTKERNELAPI VOID FASTCALL ExfAcquirePushLockExclusive(
    __inout PEX_PUSH_LOCK PushLock
    );

NTKERNELAPI VOID FASTCALL ExfAcquirePushLockShared(
    __inout PEX_PUSH_LOCK PushLock
    );

NTKERNELAPI VOID FASTCALL ExfReleasePushLock(
    __inout PEX_PUSH_LOCK PushLock
    );

/* The below functions are only exported on Vista and higher. */

NTKERNELAPI VOID FASTCALL ExfReleasePushLockShared(
    __inout PEX_PUSH_LOCK PushLock
    );

NTKERNELAPI VOID FASTCALL ExfReleasePushLockExclusive(
    __inout PEX_PUSH_LOCK PushLock
    );

NTKERNELAPI BOOLEAN FASTCALL ExfTryAcquirePushLockShared(
    __inout PEX_PUSH_LOCK PushLock
    );

NTKERNELAPI VOID FASTCALL ExfTryToWakePushLock(
    __inout PEX_PUSH_LOCK PushLock
    );

/* Wrapper functions */

/* ExInitializePushLock
 * 
 * Initializes a push lock.
 */
FORCEINLINE VOID ExInitializePushLock(
    __out PEX_PUSH_LOCK PushLock
    )
{
    *PushLock = 0;
}

/* ExAcquirePushLockExclusive
 * 
 * Acquires a push lock in exclusive mode.
 */
FORCEINLINE VOID ExAcquirePushLockExclusive(
    __inout PEX_PUSH_LOCK PushLock
    )
{
    /* Fast path - acquire push lock, no function call. */
    if (WindowsVersion < WINDOWS_SERVER_2003 || InterlockedBitTestAndSet((PLONG)PushLock, EX_PUSH_LOCK_LOCK_SHIFT))
    {
        /* Slow path - call the function. */
        ExfAcquirePushLockExclusive(PushLock);
    }
}

/* ExAcquirePushLockShared
 * 
 * Acquires a push lock in shared mode.
 */
FORCEINLINE VOID ExAcquirePushLockShared(
    __inout PEX_PUSH_LOCK PushLock
    )
{
    /* Fast path - acquire push lock which is not held at all, no function call. */
    if (WindowsVersion < WINDOWS_SERVER_2003 || InterlockedCompareExchangePointer(
        (PVOID)PushLock,
        (PVOID)(EX_PUSH_LOCK_SHARE_INC | EX_PUSH_LOCK_LOCK),
        0
        ) != 0)
    {
        /* Slow path - call the function. */
        ExfAcquirePushLockShared(PushLock);
    }
}

/* ExReleasePushLock
 * 
 * Releases a push lock (for both types).
 */
FORCEINLINE VOID ExReleasePushLock(
    __inout PEX_PUSH_LOCK PushLock
    )
{
    EXI_PUSH_LOCK oldValue, newValue;
    
    oldValue.Value = *PushLock;
    
    /* If we are the last to release in shared mode or we 
     * are releasing in exclusive mode, we simply set 
     * the value to 0.
     */
    
    if (oldValue.Shared > 1)
    {
        /* One less shared holder. */
        newValue.Value = oldValue.Value - EX_PUSH_LOCK_SHARE_INC;
    }
    else
    {
        newValue.Value = 0;
    }
    
    /* If we have chained waiters, we can't release the 
     * push lock using the fast path since they need to 
     * be unblocked.
     */
    if (
        WindowsVersion < WINDOWS_SERVER_2003 || 
        oldValue.Waiting || 
        InterlockedCompareExchangePointer(
            (PVOID)PushLock,
            newValue.Ptr,
            oldValue.Ptr
            ) != oldValue.Ptr
        )
    {
        /* Slow path - call the function. */
        ExfReleasePushLock(PushLock);
    }
}

#ifndef NEVER_DEFINED
/* ExTryAcquirePushLockExclusive
 * 
 * Attempts to acquire a push lock in exclusive mode.
 * 
 * Return value: TRUE if the push lock was acquired, FALSE if 
 * the push lock was already acquired in exclusive mode.
 */
FORCEINLINE BOOLEAN ExTryAcquirePushLockExclusive(
    __inout PEX_PUSH_LOCK PushLock
    )
{
    if (!InterlockedBitTestAndSet((PLONG)PushLock, EX_PUSH_LOCK_LOCK_SHIFT))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/* ExTryAcquirePushLockShared
 * 
 * Attempts to acquire a push lock in shared mode.
 * 
 * Return value: TRUE if the push lock was acquired, FALSE if 
 * the push lock was already acquired in exclusive mode.
 */
FORCEINLINE BOOLEAN ExTryAcquirePushLockShared(
    __inout PEX_PUSH_LOCK PushLock
    )
{
    /* Fast path with the push lock not held at all. */
    if (InterlockedCompareExchangePointer(
        (PVOID)PushLock,
        (PVOID)(EX_PUSH_LOCK_SHARE_INC | EX_PUSH_LOCK_LOCK),
        0
        ) != 0)
    {
        return ExfTryAcquirePushLockShared(PushLock);
    }
    else
    {
        return TRUE;
    }
}
#endif

#endif