#ifndef FASTLOCK_H
#define FASTLOCK_H

#include <base.h>

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

VOID PhAcquireFastLockExclusive(
    __inout PPH_FAST_LOCK FastLock
    );

VOID PhAcquireFastLockShared(
    __inout PPH_FAST_LOCK FastLock
    );

VOID PhReleaseFastLockExclusive(
    __inout PPH_FAST_LOCK FastLock
    );

VOID PhReleaseFastLockShared(
    __inout PPH_FAST_LOCK FastLock
    );

BOOLEAN PhTryAcquireFastLockExclusive(
    __inout PPH_FAST_LOCK FastLock
    );

BOOLEAN PhTryAcquireFastLockShared(
    __inout PPH_FAST_LOCK FastLock
    );

#endif
