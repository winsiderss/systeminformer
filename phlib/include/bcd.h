/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2021
 *
 */

#ifndef _PH_BCD_H
#define _PH_BCD_H

#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS PhBcdOpenObject(
    _In_ HANDLE StoreHandle,
    _In_ PGUID Identifier,
    _Out_ PHANDLE ObjectHandle
    );

NTSTATUS PhBcdGetElementData(
    _In_ HANDLE ObjectHandle,
    _In_ ULONG ElementType,
    _Out_writes_bytes_opt_(*BufferSize) PVOID Buffer,
    _Inout_ PULONG BufferSize
    );

NTSTATUS PhBcdSetElementData(
    _In_ HANDLE ObjectHandle,
    _In_ ULONG ElementType,
    _In_reads_bytes_opt_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize
    );

NTSTATUS PhBcdSetAdvancedOptionsOneTime(
    _In_ BOOLEAN Enable
    );

NTSTATUS PhBcdSetBootApplicationOneTime(
    _In_ GUID Identifier,
    _In_opt_ BOOLEAN UpdateOneTimeFirmware
    );

NTSTATUS PhBcdSetFirmwareBootApplicationOneTime(
    _In_ GUID Identifier
    );

typedef struct _PH_BCD_OBJECT_LIST
{
    GUID ObjectGuid;
    PPH_STRING ObjectName;
} PH_BCD_OBJECT_LIST, *PPH_BCD_OBJECT_LIST;

PPH_LIST PhBcdQueryFirmwareBootApplicationList(
    VOID
    );

PPH_LIST PhBcdQueryBootApplicationList(
    _In_ BOOLEAN EnumerateAllObjects
    );

VOID PhBcdDestroyBootApplicationList(
    _In_ PPH_LIST ObjectApplicationList
    );

#ifdef __cplusplus
}
#endif

#endif
