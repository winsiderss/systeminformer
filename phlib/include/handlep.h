/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *
 */

#ifndef _PH_HANDLEP_H
#define _PH_HANDLEP_H

#define PH_HANDLE_TABLE_ENTRY_TYPE 0x1
#define PH_HANDLE_TABLE_ENTRY_IN_USE 0x0
#define PH_HANDLE_TABLE_ENTRY_FREE 0x1

// Locked actually means Not Locked. This means that an in use, locked handle table entry can be
// used as-is.
#define PH_HANDLE_TABLE_ENTRY_LOCKED 0x2
#define PH_HANDLE_TABLE_ENTRY_LOCKED_SHIFT 1

// There is initially one handle table level, with 256 entries. When the handle table is expanded,
// the table is replaced with a level 1 table, which contains 256 pointers to level 0 tables (the
// first entry already points to the initial level 0 table). Similarly, when the handle table is
// expanded a second time, the table is replaced with a level 2 table, which contains 256 pointers
// to level 1 tables.
//
// This provides a maximum of 16,777,216 handles.

#define PH_HANDLE_TABLE_LEVEL_ENTRIES 256
#define PH_HANDLE_TABLE_LEVEL_MASK 0x3

#define PH_HANDLE_TABLE_LOCKS 8
#define PH_HANDLE_TABLE_LOCK_INDEX(HandleValue) ((HandleValue) % PH_HANDLE_TABLE_LOCKS)

typedef struct _PH_HANDLE_TABLE
{
    PH_QUEUED_LOCK Lock;
    PH_WAKE_EVENT HandleWakeEvent;

    ULONG Count;
    ULONG_PTR TableValue;
    ULONG FreeValue;
    ULONG NextValue;
    ULONG FreeValueAlt;

    ULONG Flags;

    PH_QUEUED_LOCK Locks[PH_HANDLE_TABLE_LOCKS];
} PH_HANDLE_TABLE, *PPH_HANDLE_TABLE;

FORCEINLINE VOID PhpLockHandleTableShared(
    _Inout_ PPH_HANDLE_TABLE HandleTable,
    _In_ ULONG Index
    )
{
    PhAcquireQueuedLockShared(&HandleTable->Locks[Index]);
}

FORCEINLINE VOID PhpUnlockHandleTableShared(
    _Inout_ PPH_HANDLE_TABLE HandleTable,
    _In_ ULONG Index
    )
{
    PhReleaseQueuedLockShared(&HandleTable->Locks[Index]);
}

// Handle values work by specifying indices into each
// level.
//
// Bits 0-7: level 0
// Bits 8-15: level 1
// Bits 16-23: level 2
// Bits 24-31: reserved

#define PH_HANDLE_VALUE_INVALID ((ULONG)-1)
#define PH_HANDLE_VALUE_SHIFT 2
#define PH_HANDLE_VALUE_BIAS 4

#define PH_HANDLE_VALUE_LEVEL0(HandleValue) ((HandleValue) & 0xff)
#define PH_HANDLE_VALUE_LEVEL1_U(HandleValue) ((HandleValue) >> 8)
#define PH_HANDLE_VALUE_LEVEL1(HandleValue) (PH_HANDLE_VALUE_LEVEL1_U(HandleValue) & 0xff)
#define PH_HANDLE_VALUE_LEVEL2_U(HandleValue) ((HandleValue) >> 16)
#define PH_HANDLE_VALUE_LEVEL2(HandleValue) (PH_HANDLE_VALUE_LEVEL2_U(HandleValue) & 0xff)
#define PH_HANDLE_VALUE_IS_INVALID(HandleValue) (((HandleValue) >> 24) != 0)

FORCEINLINE HANDLE PhpEncodeHandle(
    _In_ ULONG HandleValue
    )
{
    return UlongToHandle(((HandleValue << PH_HANDLE_VALUE_SHIFT) + PH_HANDLE_VALUE_BIAS));
}

FORCEINLINE ULONG PhpDecodeHandle(
    _In_ HANDLE Handle
    )
{
    return (HandleToUlong(Handle) - PH_HANDLE_VALUE_BIAS) >> PH_HANDLE_VALUE_SHIFT;
}

VOID PhpBlockOnLockedHandleTableEntry(
    _Inout_ PPH_HANDLE_TABLE HandleTable,
    _In_ PPH_HANDLE_TABLE_ENTRY HandleTableEntry
    );

PPH_HANDLE_TABLE_ENTRY PhpAllocateHandleTableEntry(
    _Inout_ PPH_HANDLE_TABLE HandleTable,
    _Out_ PULONG HandleValue
    );

VOID PhpFreeHandleTableEntry(
    _Inout_ PPH_HANDLE_TABLE HandleTable,
    _In_ ULONG HandleValue,
    _Inout_ PPH_HANDLE_TABLE_ENTRY HandleTableEntry
    );

BOOLEAN PhpAllocateMoreHandleTableEntries(
    _In_ PPH_HANDLE_TABLE HandleTable,
    _In_ BOOLEAN Initialize
    );

PPH_HANDLE_TABLE_ENTRY PhpLookupHandleTableEntry(
    _In_ PPH_HANDLE_TABLE HandleTable,
    _In_ ULONG HandleValue
    );

ULONG PhpMoveFreeHandleTableEntries(
    _Inout_ PPH_HANDLE_TABLE HandleTable
    );

PPH_HANDLE_TABLE_ENTRY PhpCreateHandleTableLevel0(
    _In_ PPH_HANDLE_TABLE HandleTable,
    _In_ BOOLEAN Initialize
    );

VOID PhpFreeHandleTableLevel0(
    _In_ PPH_HANDLE_TABLE_ENTRY Table
    );

PPH_HANDLE_TABLE_ENTRY *PhpCreateHandleTableLevel1(
    _In_ PPH_HANDLE_TABLE HandleTable
    );

VOID PhpFreeHandleTableLevel1(
    _In_ PPH_HANDLE_TABLE_ENTRY *Table
    );

PPH_HANDLE_TABLE_ENTRY **PhpCreateHandleTableLevel2(
    _In_ PPH_HANDLE_TABLE HandleTable
    );

VOID PhpFreeHandleTableLevel2(
    _In_ PPH_HANDLE_TABLE_ENTRY **Table
    );

#endif
