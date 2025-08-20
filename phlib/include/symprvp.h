/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2015
 *     dmex    2017-2023
 *
 */

#ifndef _PH_SYMPRVP_H
#define _PH_SYMPRVP_H

// undocumented
typedef BOOL (WINAPI *_SymGetDiaSource)(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 BaseOfDll,
    _Out_ PVOID* IDiaDataSource
    );

// undocumented
typedef BOOL (WINAPI *_SymGetDiaSession)(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 BaseOfDll,
    _Out_ PVOID* IDiaSession
    );

// undocumented
typedef BOOL (WINAPI *_SymSetDiaSession)(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 BaseOfDll,
    _In_ PVOID IDiaSession
    );

// undocumented
typedef VOID (WINAPI *_SymFreeDiaString)(
    _In_ PCWSTR DiaString
    );

#endif
