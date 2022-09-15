/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *
 */

#pragma once
#include <kph.h>

#define CIDAPI NTAPI

#define CID_LOCKED_OBJECT_MASK (((ULONG_PTR)-1) - 1)

typedef struct _CID_TABLE_ENTRY
{
    volatile ULONG_PTR Object;

} CID_TABLE_ENTRY, *PCID_TABLE_ENTRY;

typedef struct _CID_TABLE
{
    KPH_RWLOCK Lock;
    volatile ULONG_PTR Table;

} CID_TABLE, *PCID_TABLE;

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS CidTableCreate(
    _Out_ PCID_TABLE Table
    );

_IRQL_requires_max_(APC_LEVEL)
VOID CidTableDelete(
    _In_ PCID_TABLE Table
    );

_IRQL_requires_max_(APC_LEVEL)
_Acquires_lock_(_Global_critical_region_)
VOID CidAcquireObjectLock(
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_) PCID_TABLE_ENTRY Entry
    );

_IRQL_requires_max_(APC_LEVEL)
_Releases_lock_(_Global_critical_region_)
VOID CidReleaseObjectLock(
    _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_) PCID_TABLE_ENTRY Entry
    );

_IRQL_requires_max_(APC_LEVEL)
VOID CidAssignObject(
    _Inout_ _Requires_lock_held_(*_Curr_) PCID_TABLE_ENTRY Entry,
    _In_opt_ PVOID Object
    );

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
PCID_TABLE_ENTRY CidGetEntry(
    _In_ HANDLE Cid,
    _Inout_ PCID_TABLE Table
    );

typedef
_Function_class_(CID_ENUMERATE_CALLBACK)
BOOLEAN
CIDAPI
CID_ENUMERATE_CALLBACK(
    _In_ PVOID Object,
    _In_opt_ PVOID Parameter
    );
typedef CID_ENUMERATE_CALLBACK* PCID_ENUMERATE_CALLBACK;

_IRQL_requires_max_(APC_LEVEL)
VOID CidEnumerate(
    _In_ PCID_TABLE Table,
    _In_ PCID_ENUMERATE_CALLBACK Callback,
    _In_opt_ PVOID Parameter
    );

typedef
_Function_class_(CID_ENUMERATE_CALLBACK_EX)
BOOLEAN
CIDAPI
CID_ENUMERATE_CALLBACK_EX(
    _In_ PCID_TABLE_ENTRY Entry,
    _In_opt_ PVOID Parameter
    );
typedef CID_ENUMERATE_CALLBACK_EX* PCID_ENUMERATE_CALLBACK_EX;

_IRQL_requires_max_(APC_LEVEL)
VOID CidEnumerateEx(
    _In_ PCID_TABLE Table,
    _In_ PCID_ENUMERATE_CALLBACK_EX Callback,
    _In_opt_ PVOID Parameter
    );
