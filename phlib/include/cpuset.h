/*
 * Copyright (c) 2025 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer ("SI").
 *
 * Description:
 *
 *     This module contains the public routine prototypes for CPU Sets in SI.
 *
 * Authors:
 *
 *     wnstngs    2025
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef BOOLEAN (NTAPI *PPH_CPU_SETS_ENUMERATION_CALLBACK)(
    _In_ PSYSTEM_CPU_SET_INFORMATION Information,
    _In_opt_ PVOID Context
    );

NTSTATUS
NTAPI
PhQuerySystemCpuSetInformation(
    _Out_ PVOID *Information
    );

NTSTATUS
NTAPI
PhEnumerateSystemCpuSets(
    _In_ PPH_CPU_SETS_ENUMERATION_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

NTSTATUS
NTAPI
PhQueryCpuSetsProcess(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG *CpuSetIds,
    _Out_ PULONG CpuSetIdCount
    );

NTSTATUS
NTAPI
PhSetCpuSetsProcess(
    _In_ HANDLE ProcessHandle,
    _In_reads_opt_(CpuSetIdCount) const ULONG *CpuSetIds,
    _In_ ULONG CpuSetIdCount
    );

#ifdef __cplusplus
}
#endif
