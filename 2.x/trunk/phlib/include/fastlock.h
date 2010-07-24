#ifndef _PH_FASTLOCK_H
#define _PH_FASTLOCK_H

// FastLock is a port of FastResourceLock from PH 1.x.

typedef struct _PH_FAST_LOCK
{
    ULONG Value;
    HANDLE ExclusiveWakeEvent;
    HANDLE SharedWakeEvent;
} PH_FAST_LOCK, *PPH_FAST_LOCK;

#define PH_FAST_LOCK_INIT { 0, NULL, NULL } 

VOID PhFastLockInitialization();

PHLIBAPI
VOID
NTAPI
PhInitializeFastLock(
    __out PPH_FAST_LOCK FastLock
    );

PHLIBAPI
VOID
NTAPI
PhDeleteFastLock(
    __inout PPH_FAST_LOCK FastLock
    );

#define PhAcquireFastLockExclusive PhfAcquireFastLockExclusive
__mayRaise
PHLIBAPI
VOID
FASTCALL
PhfAcquireFastLockExclusive(
    __inout PPH_FAST_LOCK FastLock
    );

#define PhAcquireFastLockShared PhfAcquireFastLockShared
__mayRaise
PHLIBAPI
VOID
FASTCALL
PhfAcquireFastLockShared(
    __inout PPH_FAST_LOCK FastLock
    );

#define PhReleaseFastLockExclusive PhfReleaseFastLockExclusive
PHLIBAPI
VOID
FASTCALL
PhfReleaseFastLockExclusive(
    __inout PPH_FAST_LOCK FastLock
    );

#define PhReleaseFastLockShared PhfReleaseFastLockShared
PHLIBAPI
VOID
FASTCALL
PhfReleaseFastLockShared(
    __inout PPH_FAST_LOCK FastLock
    );

#define PhTryAcquireFastLockExclusive PhfTryAcquireFastLockExclusive
PHLIBAPI
BOOLEAN
FASTCALL
PhfTryAcquireFastLockExclusive(
    __inout PPH_FAST_LOCK FastLock
    );

#define PhTryAcquireFastLockShared PhfTryAcquireFastLockShared
PHLIBAPI
BOOLEAN
FASTCALL
PhfTryAcquireFastLockShared(
    __inout PPH_FAST_LOCK FastLock
    );

#endif
