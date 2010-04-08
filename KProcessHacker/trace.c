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

#include "include/kph.h"

BOOLEAN KphpCaptureAndAddStack(
    __in PRTL_TRACE_DATABASE Database,
    __in KPH_CAPTURE_AND_ADD_STACK_TYPE Type,
    __out_opt PRTL_TRACE_BLOCK *TraceBlock
    );

VOID KphpTraceDatabaseDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

PKPH_OBJECT_TYPE KphTraceDatabaseType;

/* KphTraceDatabaseInitialization
 * 
 * Creates the TraceDatabase object type.
 */
NTSTATUS KphTraceDatabaseInitialization()
{
    NTSTATUS status = STATUS_SUCCESS;
    
    status = KphCreateObjectType(
        &KphTraceDatabaseType,
        PagedPool,
        0,
        KphpTraceDatabaseDeleteProcedure
        );
    
    if (!NT_SUCCESS(status))
        return status;
    
    return status;
}

/* KphCaptureStackBackTrace
 * 
 * Walks the stack, capturing the return address from each frame.
 * 
 * Return value: the number of captured addresses in the buffer.
 */
ULONG KphCaptureStackBackTrace(
    __in ULONG FramesToSkip,
    __in ULONG FramesToCapture,
    __in_opt ULONG Flags,
    __out_ecount(FramesToCapture) PVOID *BackTrace,
    __out_opt PULONG BackTraceHash
    )
{
    PVOID backTrace[MAX_STACK_DEPTH];
    ULONG framesFound;
    ULONG hash;
    ULONG i;
    
    /* Skip the current frame (for this function). */
    FramesToSkip++;
    
    /* Check the input. */
    /* Ensure we won't overrun the buffer. */
    if (FramesToCapture + FramesToSkip > MAX_STACK_DEPTH)
        return 0;
    /* Make sure the flags are correct. */
    if ((Flags & RTL_WALK_VALID_FLAGS) != Flags)
        return 0;
    
    /* Walk the frame chain. */
    framesFound = RtlWalkFrameChain(
        backTrace,
        FramesToCapture + FramesToSkip,
        Flags
        );
    /* Return if we found fewer frames than we wanted to skip. */
    if (framesFound <= FramesToSkip)
        return 0;
    
    /* Copy over the stack trace. 
     * At the same time we calculate the stack trace hash by 
     * summing the addresses.
     */
    for (i = 0, hash = 0; i < FramesToCapture; i++)
    {
        if (FramesToSkip + i >= framesFound)
            break;
        
        BackTrace[i] = backTrace[FramesToSkip + i];
        hash += PtrToUlong(BackTrace[i]);
    }
    
    /* Pass the hash back if the caller requested it. */
    if (BackTraceHash)
        *BackTraceHash = hash;
    
    /* Return the number of addresses we copied. */
    return i;
}

/* KphCaptureAndAddStack
 * 
 * Captures a stack trace and adds it to a trace database.
 */
BOOLEAN KphCaptureAndAddStack(
    __in PKPH_TRACE_DATABASE Database,
    __in KPH_CAPTURE_AND_ADD_STACK_TYPE Type,
    __out_opt PRTL_TRACE_BLOCK *TraceBlock
    )
{
    return KphpCaptureAndAddStack(
        Database->Database,
        Type,
        TraceBlock
        );
}

/* KphCreateTraceDatabase
 * 
 * Creates a trace database.
 */
NTSTATUS KphCreateTraceDatabase(
    __out PKPH_TRACE_DATABASE *Database,
    __in_opt SIZE_T MaximumSize,
    __in ULONG Flags,
    __in ULONG Tag
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRTL_TRACE_DATABASE rtlDatabase;
    PKPH_TRACE_DATABASE database;
    
    /* Create the trace database. */
    rtlDatabase = RtlTraceDatabaseCreate(
        8,
        MaximumSize,
        Flags,
        Tag,
        NULL
        );
    
    if (!rtlDatabase)
        return STATUS_INSUFFICIENT_RESOURCES;
    
    /* Create the object. */
    status = KphCreateObject(
        &database,
        sizeof(KPH_TRACE_DATABASE),
        0,
        KphTraceDatabaseType,
        0
        );
    
    if (!NT_SUCCESS(status))
    {
        /* Destroy the trace database, since we can't use it. */
        RtlTraceDatabaseDestroy(rtlDatabase);
        
        return status;
    }
    
    /* Set up the trace database object. */
    database->Database = rtlDatabase;
    *Database = database;
    
    return status;
}

NTSTATUS KphQueryTraceDatabase(
    __in PKPH_TRACE_DATABASE Database,
    __out_bcount_opt(BufferLength) PKPH_TRACEDB_INFORMATION Buffer,
    __in_opt ULONG BufferLength,
    __out_opt PULONG ReturnLength,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRTL_TRACE_DATABASE rtlDatabase = Database->Database;
    PKPH_TRACEDB_INFORMATION nextEntry;
    RTL_TRACE_ENUMERATE enumContext = { 0 };
    PRTL_TRACE_BLOCK currentBlock;
    
    /* Probe buffers. */
    if (AccessMode != KernelMode)
    {
        __try
        {
            if (Buffer)
                ProbeForWrite(Buffer, BufferLength, 1);
            if (ReturnLength)
                ProbeForWrite(ReturnLength, sizeof(ULONG), 1);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }
    
    /* First entry to write to. */
    /* Note that this is completely safe if Buffer is NULL. */
    nextEntry = Buffer;
    
    /* Enumerate the trace blocks. */
    while (RtlTraceDatabaseEnumerate(rtlDatabase, &enumContext, &currentBlock))
    {
        PKPH_TRACEDB_INFORMATION currentEntry;
        
        /* Save the pointer to the entry we are about to write to. */
        currentEntry = nextEntry;
        /* Compute the location of the next entry. */
        nextEntry = (PKPH_TRACEDB_INFORMATION)(
            (ULONG_PTR)currentEntry + /* Current entry plus */
            sizeof(KPH_TRACEDB_INFORMATION) - /* the size of the current entry minus */
            sizeof(PVOID) + /* the extra PVOID in the Trace array plus */
            currentBlock->Size * sizeof(PVOID) /* the size of the stack trace. */
            );
        
        if (
            /* If we got an error last time we tried to write to the buffer, 
             * don't try again this time. */
            NT_SUCCESS(status) && 
            /* Make sure the buffer isn't NULL. */
            Buffer && 
            /* Make sure we don't exceed the buffer length. */
            ((ULONG_PTR)nextEntry - (ULONG_PTR)Buffer) <= BufferLength
            )
        {
            __try
            {
                currentEntry->NextEntryOffset = (ULONG)((ULONG_PTR)nextEntry - (ULONG_PTR)currentEntry);
                currentEntry->Count = currentBlock->Count;
                currentEntry->TraceSize = currentBlock->Size;
                memcpy(currentEntry->Trace, currentBlock->Trace, currentBlock->Size * sizeof(PVOID));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
            }
        }
        else
        {
            status = STATUS_BUFFER_TOO_SMALL;
        }
    }
    
    if (ReturnLength)
    {
        __try
        {
            *ReturnLength = (ULONG)((ULONG_PTR)nextEntry - (ULONG_PTR)Buffer);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
        }
    }
    
    return status;
}

/* KphCaptureAndAddStack
 * 
 * Captures a stack trace and adds it to a trace database.
 */
BOOLEAN KphpCaptureAndAddStack(
    __in PRTL_TRACE_DATABASE Database,
    __in KPH_CAPTURE_AND_ADD_STACK_TYPE Type,
    __out_opt PRTL_TRACE_BLOCK *TraceBlock
    )
{
    PVOID trace[MAX_STACK_DEPTH * 2];
    ULONG kmodeFramesFound = 0;
    ULONG umodeFramesFound = 0;
    
    /* Check input. */
    if (Type >= KphCaptureAndAddMaximum)
        return FALSE;
    
    /* Capture the kernel-mode stack if needed. */
    if (
        Type == KphCaptureAndAddKModeStack || 
        Type == KphCaptureAndAddBothStacks
        )
        kmodeFramesFound = KphCaptureStackBackTrace(
            1,
            MAX_STACK_DEPTH - 1,
            0,
            trace,
            NULL
            );
    /* Capture the user-mode stack if needed. */
    if (
        Type == KphCaptureAndAddUModeStack || 
        Type == KphCaptureAndAddBothStacks
        )
        umodeFramesFound = KphCaptureStackBackTrace(
            0,
            MAX_STACK_DEPTH - 1,
            RTL_WALK_USER_MODE_STACK,
            &trace[kmodeFramesFound],
            NULL
            );
    
    /* Add the trace to the database. */
    return RtlTraceDatabaseAdd(
        Database,
        kmodeFramesFound + umodeFramesFound,
        trace,
        TraceBlock
        );
}

/* KphpTraceDatabaseDeleteProcedure
 * 
 * Destroys a trace database.
 */
VOID KphpTraceDatabaseDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PKPH_TRACE_DATABASE database = (PKPH_TRACE_DATABASE)Object;
    
    RtlTraceDatabaseDestroy(database->Database);
}
