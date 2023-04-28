/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2015
 *
 */

#ifndef _PH_FASTLOCK_H
#define _PH_FASTLOCK_H

// FastLock is a port of FastResourceLock from PH 1.x.

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PH_FAST_LOCK
{
    ULONG Value;
    HANDLE ExclusiveWakeEvent;
    HANDLE SharedWakeEvent;
} PH_FAST_LOCK, *PPH_FAST_LOCK;

#define PH_FAST_LOCK_INIT { 0, NULL, NULL }

PHLIBAPI
VOID
NTAPI
PhInitializeFastLock(
    _Out_ PPH_FAST_LOCK FastLock
    );

PHLIBAPI
VOID
NTAPI
PhDeleteFastLock(
    _Inout_ PPH_FAST_LOCK FastLock
    );

#define PhAcquireFastLockExclusive PhfAcquireFastLockExclusive
_May_raise_
_Acquires_exclusive_lock_(*FastLock)
PHLIBAPI
VOID
FASTCALL
PhfAcquireFastLockExclusive(
    _Inout_ PPH_FAST_LOCK FastLock
    );

#define PhAcquireFastLockShared PhfAcquireFastLockShared
_May_raise_
_Acquires_shared_lock_(*FastLock)
PHLIBAPI
VOID
FASTCALL
PhfAcquireFastLockShared(
    _Inout_ PPH_FAST_LOCK FastLock
    );

#define PhReleaseFastLockExclusive PhfReleaseFastLockExclusive
_Releases_exclusive_lock_(*FastLock)
PHLIBAPI
VOID
FASTCALL
PhfReleaseFastLockExclusive(
    _Inout_ PPH_FAST_LOCK FastLock
    );

#define PhReleaseFastLockShared PhfReleaseFastLockShared
_Releases_shared_lock_(*FastLock)
PHLIBAPI
VOID
FASTCALL
PhfReleaseFastLockShared(
    _Inout_ PPH_FAST_LOCK FastLock
    );

#define PhTryAcquireFastLockExclusive PhfTryAcquireFastLockExclusive
_When_(return != 0, _Acquires_exclusive_lock_(*FastLock))
PHLIBAPI
BOOLEAN
FASTCALL
PhfTryAcquireFastLockExclusive(
    _Inout_ PPH_FAST_LOCK FastLock
    );

#define PhTryAcquireFastLockShared PhfTryAcquireFastLockShared
_When_(return != 0, _Acquires_shared_lock_(*FastLock))
PHLIBAPI
BOOLEAN
FASTCALL
PhfTryAcquireFastLockShared(
    _Inout_ PPH_FAST_LOCK FastLock
    );

#ifdef __cplusplus
}
#endif

#endif
