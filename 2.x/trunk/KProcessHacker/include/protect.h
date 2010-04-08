/*
 * Process Hacker Driver - 
 *   process protection
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

#ifndef _PROTECT_H
#define _PROTECT_H

#include "hook.h"

#define TAG_PROTECTION_ENTRY ('rPhP')

#define OBOPENOBJECTBYPOINTER_ARGS \
    PVOID Object, \
    ULONG HandleAttributes, \
    PACCESS_STATE PassedAccessState, \
    ACCESS_MASK DesiredAccess, \
    POBJECT_TYPE ObjectType, \
    KPROCESSOR_MODE AccessMode, \
    PHANDLE Handle

typedef struct _KPH_PROCESS_ENTRY
{
    LIST_ENTRY ListEntry;
    PEPROCESS Process;
    PEPROCESS CreatorProcess;
    HANDLE Tag;
    LOGICAL AllowKernelMode;
    ACCESS_MASK ProcessAllowMask;
    ACCESS_MASK ThreadAllowMask;
} KPH_PROCESS_ENTRY, *PKPH_PROCESS_ENTRY;

NTSTATUS NTAPI KphNewObOpenObjectByPointer(OBOPENOBJECTBYPOINTER_ARGS);
NTSTATUS NTAPI KphOldObOpenObjectByPointer(OBOPENOBJECTBYPOINTER_ARGS);

NTSTATUS NTAPI KphNewOpenProcedure51(
    __in OB_OPEN_REASON OpenReason,
    __in PEPROCESS Process,
    __in PVOID Object,
    __in ACCESS_MASK GrantedAccess,
    __in ULONG HandleCount
    );

NTSTATUS NTAPI KphNewOpenProcedure60(
    __in OB_OPEN_REASON OpenReason,
    __in KPROCESSOR_MODE AccessMode,
    __in PEPROCESS Process,
    __in PVOID Object,
    __in ACCESS_MASK GrantedAccess,
    __in ULONG HandleCount
    );

NTSTATUS KphProtectInit();
NTSTATUS KphProtectDeinit();

PKPH_PROCESS_ENTRY KphProtectAddEntry(
    __in PEPROCESS Process,
    __in HANDLE Tag,
    __in LOGICAL AllowKernelMode,
    __in ACCESS_MASK ProcessAllowMask,
    __in ACCESS_MASK ThreadAllowMask
    );

PKPH_PROCESS_ENTRY KphProtectFindEntry(
    __in PEPROCESS Process,
    __in HANDLE Tag,
    __out_opt PKPH_PROCESS_ENTRY ProcessEntryCopy
    );

BOOLEAN KphProtectRemoveByProcess(
    __in PEPROCESS Process
    );

ULONG KphProtectRemoveByTag(
    __in HANDLE Tag
    );

#endif
