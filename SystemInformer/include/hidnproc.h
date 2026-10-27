/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2015
 *     dmex    2017-2026
 *
 */

#ifndef PH_HIDNPROC_H
#define PH_HIDNPROC_H

typedef struct _PH_CSR_HANDLE_INFO
{
    HANDLE CsrProcessHandle;
    HANDLE Handle;
    BOOLEAN IsThreadHandle;

    HANDLE ProcessId;
} PH_CSR_HANDLE_INFO, *PPH_CSR_HANDLE_INFO;

typedef BOOLEAN (NTAPI *PPH_ENUM_CSR_PROCESS_HANDLES_CALLBACK)(
    _In_ PPH_CSR_HANDLE_INFO Handle,
    _In_opt_ PVOID Context
    );

NTSTATUS
NTAPI
PhEnumCsrProcessHandles(
    _In_ PPH_ENUM_CSR_PROCESS_HANDLES_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

NTSTATUS
NTAPI
PhOpenProcessByCsrHandle(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PPH_CSR_HANDLE_INFO Handle
    );

NTSTATUS
NTAPI
PhOpenProcessByCsrHandles(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ HANDLE ProcessId
    );

#endif
