#ifndef _PH_PHSYNC_H
#define _PH_PHSYNC_H

// This header file defines synchronization primitives not included 
// in phbase.

// reslock

#define PH_RESOURCE_LOCK_OWNED 0x1
#define PH_RESOURCE_LOCK_OWNED_SHIFT 0
#define PH_RESOURCE_LOCK_WAITERS 0x2
#define PH_RESOURCE_LOCK_WAITERS_SHIFT 1

#define PH_RESOURCE_LOCK_SHARED_SHIFT 2
#define PH_RESOURCE_LOCK_SHARED_MASK 0x3fffffff
#define PH_RESOURCE_LOCK_SHARED_INC 0x4

typedef struct _PH_RESOURCE_WAITER *PPH_RESOURCE_WAITER;

typedef struct _PH_RESOURCE_LOCK
{
    ULONG Value;
    ULONG SpinCount;
    PH_QUEUED_LOCK WakeEvent;

    PH_QUEUED_LOCK WaiterListLock;
    LIST_ENTRY WaiterListHead;
    PLIST_ENTRY FirstSharedWaiter;
} PH_RESOURCE_LOCK, *PPH_RESOURCE_LOCK;

#define PH_RESOURCE_WAITER_EXCLUSIVE 0x1
#define PH_RESOURCE_WAITER_SIGNALED 0x2
#define PH_RESOURCE_WAITER_SIGNALED_SHIFT 1

typedef struct _PH_RESOURCE_WAITER
{
    LIST_ENTRY ListEntry;
    ULONG Flags;
} PH_RESOURCE_WAITER, *PPH_RESOURCE_WAITER;

#define PhInitializeResourceLock PhfInitializeResourceLock
PHLIBAPI
VOID
FASTCALL
PhfInitializeResourceLock(
    __out PPH_RESOURCE_LOCK Lock
    );

#define PhAcquireResourceLockExclusive PhfAcquireResourceLockExclusive
PHLIBAPI
VOID
FASTCALL
PhfAcquireResourceLockExclusive(
    __inout PPH_RESOURCE_LOCK Lock
    );

#define PhAcquireResourceLockShared PhfAcquireResourceLockShared
PHLIBAPI
VOID
FASTCALL
PhfAcquireResourceLockShared(
    __inout PPH_RESOURCE_LOCK Lock
    );

#define PhReleaseResourceLockExclusive PhfReleaseResourceLockExclusive
PHLIBAPI
VOID
FASTCALL
PhfReleaseResourceLockExclusive(
    __inout PPH_RESOURCE_LOCK Lock
    );

#define PhReleaseResourceLockShared PhfReleaseResourceLockShared
PHLIBAPI
VOID
FASTCALL
PhfReleaseResourceLockShared(
    __inout PPH_RESOURCE_LOCK Lock
    );

#define PhConvertResourceLockExclusiveToShared PhfConvertResourceLockExclusiveToShared
PHLIBAPI
VOID
FASTCALL
PhfConvertResourceLockExclusiveToShared(
    __inout PPH_RESOURCE_LOCK Lock
    );

#define PhConvertResourceLockSharedToExclusive PhfConvertResourceLockSharedToExclusive
PHLIBAPI
VOID
FASTCALL
PhfConvertResourceLockSharedToExclusive(
    __inout PPH_RESOURCE_LOCK Lock
    );

FORCEINLINE BOOLEAN PhTryAcquireResourceLockExclusive(
    __inout PPH_RESOURCE_LOCK Lock
    )
{
    if (!_interlockedbittestandset(&Lock->Value, PH_RESOURCE_LOCK_OWNED_SHIFT))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

#endif
