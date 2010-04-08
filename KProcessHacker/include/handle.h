/*
 * Process Hacker Driver - 
 *   handle table
 * 
 * Copyright (C) 2009 wj32
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

#ifndef _HANDLE_H
#define _HANDLE_H

#include "kph.h"
#include "ref.h"

struct _KPH_HANDLE_TABLE;
typedef struct _KPH_HANDLE_TABLE *PKPH_HANDLE_TABLE;

typedef struct _KPH_HANDLE_TABLE_ENTRY
{
    union
    {
        HANDLE Handle;
        ULONG_PTR Value;
        struct _KPH_HANDLE_TABLE_ENTRY *NextFree;
    };
    PVOID Object;
} KPH_HANDLE_TABLE_ENTRY, *PKPH_HANDLE_TABLE_ENTRY;

NTSTATUS KphCreateHandleTable(
    __out PKPH_HANDLE_TABLE *HandleTable,
    __in ULONG MaximumHandles,
    __in ULONG SizeOfEntry,
    __in ULONG Tag
    );

VOID KphFreeHandleTable(
    __in PKPH_HANDLE_TABLE HandleTable
    );

NTSTATUS KphCloseHandle(
    __in PKPH_HANDLE_TABLE HandleTable,
    __in HANDLE Handle
    );

NTSTATUS KphCreateHandle(
    __in PKPH_HANDLE_TABLE HandleTable,
    __in PVOID Object,
    __out PHANDLE Handle
    );

NTSTATUS KphReferenceObjectByHandle(
    __in PKPH_HANDLE_TABLE HandleTable,
    __in HANDLE Handle,
    __in_opt PKPH_OBJECT_TYPE ObjectType,
    __out PVOID *Object
    );

BOOLEAN KphValidHandle(
    __in PKPH_HANDLE_TABLE HandleTable,
    __in HANDLE Handle,
    __out_opt PKPH_HANDLE_TABLE_ENTRY *Entry
    );

#endif
