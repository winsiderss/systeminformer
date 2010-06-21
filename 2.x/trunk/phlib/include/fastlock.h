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

VOID PhInitializeFastLock(
    __out PPH_FAST_LOCK FastLock
    );

VOID PhDeleteFastLock(
    __inout PPH_FAST_LOCK FastLock
    );

#define PhAcquireFastLockExclusive PhfAcquireFastLockExclusive
__mayRaise VOID FASTCALL PhfAcquireFastLockExclusive(
    __inout PPH_FAST_LOCK FastLock
    );

#define PhAcquireFastLockShared PhfAcquireFastLockShared
__mayRaise VOID FASTCALL PhfAcquireFastLockShared(
    __inout PPH_FAST_LOCK FastLock
    );

#define PhReleaseFastLockExclusive PhfReleaseFastLockExclusive
VOID FASTCALL PhfReleaseFastLockExclusive(
    __inout PPH_FAST_LOCK FastLock
    );

#define PhReleaseFastLockShared PhfReleaseFastLockShared
VOID FASTCALL PhfReleaseFastLockShared(
    __inout PPH_FAST_LOCK FastLock
    );

#define PhTryAcquireFastLockExclusive PhfTryAcquireFastLockExclusive
BOOLEAN FASTCALL PhfTryAcquireFastLockExclusive(
    __inout PPH_FAST_LOCK FastLock
    );

#define PhTryAcquireFastLockShared PhfTryAcquireFastLockShared
BOOLEAN FASTCALL PhfTryAcquireFastLockShared(
    __inout PPH_FAST_LOCK FastLock
    );

#endif
