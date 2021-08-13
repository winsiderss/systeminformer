/*
 * Process Hacker -
 *   Boot Configuration Data (BCD) wrapper header
 *
 * Copyright (C) 2021 dmex
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
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
