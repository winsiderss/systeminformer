/*
 * Process Hacker Driver - 
 *   stack tracing
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

#ifndef _TRACE_H
#define _TRACE_H

#include "types.h"

/* Stack Tracing */

/* Sensible limit that may or may not correspond to the actual Windows value. */
#define MAX_STACK_DEPTH 64

#define RTL_WALK_USER_MODE_STACK 0x00000001
#define RTL_WALK_VALID_FLAGS 0x00000001

/* RtlWalkFrameChain
 * 
 * Walks an EBP chain and fills out an array of addresses.
 * 
 * Return value: the number of frames found.
 */
NTSYSAPI ULONG NTAPI RtlWalkFrameChain(
    __out PVOID *Callers,
    __in ULONG Count,
    __in ULONG Flags
    );

/* Trace Database */

#define RTL_TRACE_IN_USER_MODE 0x00000001
#define RTL_TRACE_IN_KERNEL_MODE 0x00000002
#define RTL_TRACE_USE_NONPAGED_POOL 0x00000004
#define RTL_TRACE_USE_PAGED_POOL 0x00000008

typedef struct _RTL_TRACE_BLOCK
{
    ULONG Magic;
    ULONG Count; /* Reference count */
    ULONG Size; /* Size, in PVOIDs, of the trace */
    
    SIZE_T UserCount;
    SIZE_T UserSize;
    PVOID UserContext;
    
    struct _RTL_TRACE_BLOCK *Next;
    PVOID *Trace;
} RTL_TRACE_BLOCK, *PRTL_TRACE_BLOCK;

typedef struct _RTL_TRACE_DATABASE *PRTL_TRACE_DATABASE;

/* Enumeration context. */
typedef struct _RTL_TRACE_ENUMERATE
{
    PRTL_TRACE_DATABASE Database;
    ULONG Index;
    PRTL_TRACE_BLOCK Block;
} RTL_TRACE_ENUMERATE, *PRTL_TRACE_ENUMERATE;

typedef ULONG (*RTL_TRACE_HASH_FUNCTION)(
    ULONG Count,
    PVOID *Trace
    );

PRTL_TRACE_DATABASE RtlTraceDatabaseCreate(
    __in ULONG Buckets,
    __in_opt SIZE_T MaximumSize,
    __in ULONG Flags, /* optional in user-mode */
    __in ULONG Tag, /* optional in user-mode */
    __in_opt RTL_TRACE_HASH_FUNCTION HashFunction
    );

BOOLEAN RtlTraceDatabaseDestroy(
    __in PRTL_TRACE_DATABASE Database
    );

BOOLEAN RtlTraceDatabaseValidate(
    __in PRTL_TRACE_DATABASE Database
    );

BOOLEAN RtlTraceDatabaseAdd(
    __in PRTL_TRACE_DATABASE Database,
    __in ULONG Count,
    __in PVOID *Trace,
    __out_opt PRTL_TRACE_BLOCK *TraceBlock
    );

/* RtlTraceDatabaseEnumerate
 * 
 * Enumerates the trace blocks in the specified trace database.
 * 
 * Database: The trace database to process.
 * Enumerate: A context structure for the enumeration. Zero the 
 * structure if you are using it for the first time.
 * TraceBlock: The trace block that was found by the function.
 * 
 * Return value: TRUE if a trace block was found, FALSE if there 
 * are no more trace blocks.
 */
BOOLEAN RtlTraceDatabaseEnumerate(
    __in PRTL_TRACE_DATABASE Database,
    __inout PRTL_TRACE_ENUMERATE Enumerate,
    __out PRTL_TRACE_BLOCK *TraceBlock
    );

BOOLEAN RtlTraceDatabaseFind(
    __in PRTL_TRACE_DATABASE Database,
    __in ULONG Count,
    __in PVOID *Trace,
    __out_opt PRTL_TRACE_BLOCK *TraceBlock
    );

/* Note: locking/unlocking is only needed when trace blocks are modified. 
 * It is not needed for adding/enumerating/finding. */
VOID RtlTraceDatabaseLock(
    __in PRTL_TRACE_DATABASE Database
    );

VOID RtlTraceDatabaseUnlock(
    __in PRTL_TRACE_DATABASE Database
    );

/* KPH trace interface */

typedef enum _KPH_CAPTURE_AND_ADD_STACK_TYPE
{
    KphCaptureAndAddKModeStack,
    KphCaptureAndAddUModeStack,
    KphCaptureAndAddBothStacks,
    KphCaptureAndAddMaximum
} KPH_CAPTURE_AND_ADD_STACK_TYPE, *PKPH_CAPTURE_AND_ADD_STACK_TYPE;

typedef struct _KPH_TRACE_DATABASE
{
    PRTL_TRACE_DATABASE Database;
} KPH_TRACE_DATABASE, *PKPH_TRACE_DATABASE;

typedef struct _KPH_TRACEDB_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG Count;
    ULONG TraceSize;
    PVOID Trace[1];
} KPH_TRACEDB_INFORMATION, *PKPH_TRACEDB_INFORMATION;

NTSTATUS KphTraceDatabaseInitialization();

BOOLEAN KphCaptureAndAddStack(
    __in PKPH_TRACE_DATABASE Database,
    __in KPH_CAPTURE_AND_ADD_STACK_TYPE Type,
    __out_opt PRTL_TRACE_BLOCK *TraceBlock
    );

ULONG KphCaptureStackBackTrace(
    __in ULONG FramesToSkip,
    __in ULONG FramesToCapture,
    __in_opt ULONG Flags,
    __out_ecount(FramesToCapture) PVOID *BackTrace,
    __out_opt PULONG BackTraceHash
    );

NTSTATUS KphCreateTraceDatabase(
    __out PKPH_TRACE_DATABASE *Database,
    __in_opt SIZE_T MaximumSize,
    __in ULONG Flags,
    __in ULONG Tag
    );

#endif
