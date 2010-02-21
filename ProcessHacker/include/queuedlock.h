#ifndef QUEUEDLOCK_H
#define QUEUEDLOCK_H

#define PH_QUEUED_LOCK_OWNED ((ULONG_PTR)0x1)
#define PH_QUEUED_LOCK_WAITERS ((ULONG_PTR)0x2)

// Valid only if Waiters = 0
#define PH_QUEUED_LOCK_SHARED_INC ((ULONG_PTR)0x4)
#define PH_QUEUED_LOCK_SHARED_SHIFT 2

// Valid only if Waiters = 1
#define PH_QUEUED_LOCK_TRAVERSING ((ULONG_PTR)0x4)
#define PH_QUEUED_LOCK_MULTIPLE_SHARED ((ULONG_PTR)0x8)

#define PH_QUEUED_LOCK_FLAGS ((ULONG_PTR)0xf)

#define PhGetQueuedLockSharedOwners(Value) \
    ((ULONG_PTR)(Value) >> PH_QUEUED_LOCK_SHARED_SHIFT)
#define PhGetQueuedLockWaitBlock(Value) \
    ((PPH_QUEUED_WAIT_BLOCK)((ULONG_PTR)(Value) & ~PH_QUEUED_LOCK_FLAGS))

typedef struct _PH_QUEUED_LOCK
{
    ULONG_PTR Value;
} PH_QUEUED_LOCK, *PPH_QUEUED_LOCK;

#define PH_QUEUED_WAITER_EXCLUSIVE 0x1
#define PH_QUEUED_WAITER_SPINNING 0x2
#define PH_QUEUED_WAITER_SPINNING_SHIFT 1

typedef struct DECLSPEC_ALIGN(16) _PH_QUEUED_WAIT_BLOCK
{
    struct _PH_QUEUED_WAIT_BLOCK *Next;
    struct _PH_QUEUED_WAIT_BLOCK *Previous;
    struct _PH_QUEUED_WAIT_BLOCK *Last;

    ULONG SharedOwners;
    ULONG Flags;
} PH_QUEUED_WAIT_BLOCK, *PPH_QUEUED_WAIT_BLOCK;

BOOLEAN PhQueuedLockInitialization();

VOID FASTCALL PhfAcquireQueuedLockExclusive(
    __inout PPH_QUEUED_LOCK QueuedLock
    );

VOID FASTCALL PhfAcquireQueuedLockShared(
    __inout PPH_QUEUED_LOCK QueuedLock
    );

VOID FASTCALL PhfReleaseQueuedLockExclusive(
    __inout PPH_QUEUED_LOCK QueuedLock
    );

VOID FASTCALL PhfReleaseQueuedLockShared(
    __inout PPH_QUEUED_LOCK QueuedLock
    );

#endif
