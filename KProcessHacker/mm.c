/*
 * Process Hacker Driver - 
 *   memory manager
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

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KphReadVirtualMemory)
#pragma alloc_text(PAGE, KphUnsafeReadVirtualMemory)
#pragma alloc_text(PAGE, KphWriteVirtualMemory)
#pragma alloc_text(PAGE, MiDoMappedCopy)
#pragma alloc_text(PAGE, MiDoPoolCopy)
#pragma alloc_text(PAGE, MiGetExceptionInfo)
#pragma alloc_text(PAGE, MmCopyVirtualMemory)
#endif

/* KphReadVirtualMemory
 * 
 * Reads virtual memory from the specified process.
 */
NTSTATUS KphReadVirtualMemory(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __out_bcount(BufferLength) PVOID Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG ReturnLength,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS processObject;
    ULONG returnLength = 0;
    
    /* Probe user input if we're not from kernel-mode. */
    if (AccessMode != KernelMode)
    {
        if ((((ULONG_PTR)BaseAddress + BufferLength) < (ULONG_PTR)BaseAddress) || 
            (((ULONG_PTR)Buffer + BufferLength) < (ULONG_PTR)Buffer) || 
            (((ULONG_PTR)BaseAddress + BufferLength) > (ULONG_PTR)MmHighestUserAddress) || 
            (((ULONG_PTR)Buffer + BufferLength) > (ULONG_PTR)MmHighestUserAddress))
        {
            return STATUS_ACCESS_VIOLATION;
        }
        
        __try
        {
            if (ReturnLength)
                ProbeForWrite(ReturnLength, sizeof(ULONG), 1);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return STATUS_ACCESS_VIOLATION;
        }
    }
    
    /* If we actually have work to do, reference the process object and 
       call the internal function. */
    if (BufferLength)
    {
        status = ObReferenceObjectByHandle(
            ProcessHandle,
            PROCESS_VM_READ,
            *PsProcessType,
            KernelMode,
            &processObject,
            NULL
            );
        
        if (!NT_SUCCESS(status))
            return status;
        
        status = MmCopyVirtualMemory(
            processObject,
            BaseAddress,
            PsGetCurrentProcess(),
            Buffer,
            BufferLength,
            AccessMode,
            &returnLength
            );
        ObDereferenceObject(processObject);
    }
    
    if (ReturnLength)
    {
        __try
        {
            *ReturnLength = returnLength;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
        }
    }
    
    return status;
}

NTSTATUS KphUnsafeReadVirtualMemory(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __out_bcount(BufferLength) PVOID Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG ReturnLength,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG returnLength = 0;
    
    /* Initial probing. */
    if (AccessMode != KernelMode)
    {
        if ((((ULONG_PTR)BaseAddress + BufferLength) < (ULONG_PTR)BaseAddress) || 
            (((ULONG_PTR)Buffer + BufferLength) < (ULONG_PTR)Buffer) || 
            (((ULONG_PTR)Buffer + BufferLength) > (ULONG_PTR)MmHighestUserAddress))
        {
            return STATUS_ACCESS_VIOLATION;
        }
        
        __try
        {
            ProbeForWrite(Buffer, BufferLength, 1);
            
            if (ReturnLength)
                ProbeForWrite(ReturnLength, sizeof(ULONG), 1);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }
    
    /* Make sure we have something to copy. */
    if (BufferLength == 0)
    {
        __try
        {
            *ReturnLength = 0;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
        
        return STATUS_SUCCESS;
    }
    
    /* Select the appropriate copy method. */
    if (((ULONG_PTR)BaseAddress + BufferLength) > (ULONG_PTR)MmHighestUserAddress)
    {
        /* Kernel memory unsafe copy. */
        
        __try
        {
            /* Probe the address range. */
            KphProbeSystemAddressRange(BaseAddress, BufferLength);
            
            /* Copy the data. */
            memcpy(Buffer, BaseAddress, BufferLength);
            returnLength = BufferLength;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
        }
        
        if (ReturnLength)
        {
            __try
            {
                *ReturnLength = returnLength;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
            }
        }
    }
    else
    {
        /* User memory safe copy. */
        status = KphReadVirtualMemory(
            ProcessHandle,
            BaseAddress,
            Buffer,
            BufferLength,
            ReturnLength,
            AccessMode
            );
    }
    
    return status;
}

/* KphWriteVirtualMemory
 * 
 * Writes virtual memory to the specified process.
 */
NTSTATUS KphWriteVirtualMemory(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __in_bcount(BufferLength) PVOID Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG ReturnLength,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS processObject;
    ULONG returnLength = 0;
    
    /* Probe user input if we're not from kernel-mode. */
    if (AccessMode != KernelMode)
    {
        if ((((ULONG_PTR)BaseAddress + BufferLength) < (ULONG_PTR)BaseAddress) || 
            (((ULONG_PTR)Buffer + BufferLength) < (ULONG_PTR)Buffer) || 
            (((ULONG_PTR)BaseAddress + BufferLength) > (ULONG_PTR)MmHighestUserAddress) || 
            (((ULONG_PTR)Buffer + BufferLength) > (ULONG_PTR)MmHighestUserAddress))
        {
            return STATUS_ACCESS_VIOLATION;
        }
        
        __try
        {
            if (ReturnLength)
                ProbeForWrite(ReturnLength, sizeof(ULONG), 1);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return STATUS_ACCESS_VIOLATION;
        }
    }
    
    /* If we actually have work to do, reference the process object and 
       call the internal function. */
    if (BufferLength)
    {
        status = ObReferenceObjectByHandle(
            ProcessHandle,
            PROCESS_VM_WRITE,
            *PsProcessType,
            KernelMode,
            &processObject,
            NULL
            );
        
        if (!NT_SUCCESS(status))
            return status;
        
        status = MmCopyVirtualMemory(
            PsGetCurrentProcess(),
            Buffer,
            processObject,
            BaseAddress,
            BufferLength,
            AccessMode,
            &returnLength
            );
        ObDereferenceObject(processObject);
    }
    
    if (ReturnLength)
    {
        __try
        {
            *ReturnLength = returnLength;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
        }
    }
    
    return status;
}

/* MiDoMappedCopy
 * 
 * Copies virtual memory from the source process to the target process 
 * using a memory mapping.
 */
NTSTATUS MiDoMappedCopy(
    __in PEPROCESS FromProcess,
    __in PVOID FromAddress,
    __in PEPROCESS ToProcess,
    __in PVOID ToAddress,
    __in ULONG BufferLength,
    __in KPROCESSOR_MODE AccessMode,
    __out PULONG ReturnLength
    )
{
    PFN_NUMBER mdlBuffer[(sizeof(MDL) / sizeof(PFN_NUMBER)) + MI_MAPPED_COPY_PAGES + 1];
    PMDL mdl = (PMDL)mdlBuffer;
    /* The mapped address. */
    PVOID mappedAddress;
    /* The total size allocated (mapped pages). */
    ULONG totalSize;
    /* The block size. */
    ULONG blockSize;
    /* The amount still left to copy. */
    ULONG stillToCopy;
    /* Attach state. */
    KPH_ATTACH_STATE attachState;
    /* The current source address. */
    PVOID sourceAddress;
    /* The current target address. */
    PVOID targetAddress;
    /* Whether the pages have been locked. */
    BOOLEAN pagesLocked;
    /* Whether we are currently copying. */
    BOOLEAN copying = FALSE;
    /* Whether we are currently probing. */
    BOOLEAN probing = FALSE;
    /* Whether we are currently mapping. */
    BOOLEAN mapping = FALSE;
    /* Whether we have the bad address. */
    BOOLEAN haveBadAddress;
    /* The bad address of the exception. */
    ULONG_PTR badAddress;
    
    sourceAddress = FromAddress;
    targetAddress = ToAddress;
    
    totalSize = (MI_MAPPED_COPY_PAGES - 2) * PAGE_SIZE;
    
    if (BufferLength <= totalSize)
        totalSize = BufferLength;
    
    stillToCopy = BufferLength;
    blockSize = totalSize;
    
    while (stillToCopy)
    {
        /* If we're at the last copy block, copy the remaining bytes instead 
           of the whole block size. */
        if (stillToCopy < blockSize)
            blockSize = stillToCopy;
        
        /* Reset state. */
        mappedAddress = NULL;
        pagesLocked = FALSE;
        copying = FALSE;
        
        KphAttachProcess(FromProcess, &attachState);
        
        __try
        {
            /* Probe only if this is the first time. */
            if ((sourceAddress == FromAddress) && (AccessMode != KernelMode))
            {
                probing = TRUE;
                ProbeForRead(sourceAddress, BufferLength, 1);
                probing = FALSE;
            }
            
            /* Initialize the MDL. */
            MmInitializeMdl(mdl, sourceAddress, blockSize);
            MmProbeAndLockPages(mdl, AccessMode, IoReadAccess);
            pagesLocked = TRUE;
            
            /* Map the pages. */
            mappedAddress = MmMapLockedPagesSpecifyCache(
                mdl,
                KernelMode,
                MmCached,
                NULL,
                FALSE,
                HighPagePriority
                );
            
            if (!mappedAddress)
            {
                /* Insufficient resources; exit. */
                mapping = TRUE;
                ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
            }
            
            KphDetachProcess(&attachState);
            
            /* Attach to the target process and copy the mapped contents. */
            KphAttachProcess(ToProcess, &attachState);
            
            /* Probe only if this is the first time. */
            if ((targetAddress == ToAddress) && (AccessMode != KernelMode))
            {
                probing = TRUE;
                ProbeForWrite(targetAddress, BufferLength, 1);
                probing = FALSE;
            }
            
            /* Copy the data. */
            copying = TRUE;
            memcpy(targetAddress, mappedAddress, blockSize);
        }
        __except (MiGetExceptionInfo(
            GetExceptionInformation(),
            &haveBadAddress,
            &badAddress
            ))
        {
            KphDetachProcess(&attachState);
            
            /* If we mapped the pages, unmap them. */
            if (mappedAddress)
                MmUnmapLockedPages(mappedAddress, mdl);
            
            /* If we locked the pages, unlock them. */
            if (pagesLocked)
                MmUnlockPages(mdl);
            
            /* If we failed when probing or mapping, return the error code. */
            if (probing || mapping)
                return GetExceptionCode();
            
            /* Otherwise, give the caller the number of bytes we copied. */
            *ReturnLength = BufferLength - stillToCopy;
            
            /* If we were copying, we can probably get the exact 
               number of bytes copied. */
            if (copying && haveBadAddress)
                *ReturnLength = (ULONG)(badAddress - (ULONG_PTR)sourceAddress);
            
            return STATUS_PARTIAL_COPY;
        }
        
        KphDetachProcess(&attachState);
        MmUnmapLockedPages(mappedAddress, mdl);
        MmUnlockPages(mdl);
        
        stillToCopy -= blockSize;
        sourceAddress = (PVOID)((ULONG_PTR)sourceAddress + blockSize);
        targetAddress = (PVOID)((ULONG_PTR)targetAddress + blockSize);
    }
    
    *ReturnLength = BufferLength;
    
    return STATUS_SUCCESS;
}

/* MiDoPoolCopy
 * 
 * Copies virtual memory from the source process to the target process 
 * using either a pool allocation or a stack buffer.
 */
NTSTATUS MiDoPoolCopy(
    __in PEPROCESS FromProcess,
    __in PVOID FromAddress,
    __in PEPROCESS ToProcess,
    __in PVOID ToAddress,
    __in ULONG BufferLength,
    __in KPROCESSOR_MODE AccessMode,
    __out PULONG ReturnLength
    )
{
    /* The size of the pool-allocated buffer. */
    ULONG allocSize = MI_MAX_TRANSFER_SIZE;
    /* The stack-based buffer. */
    CHAR stackBuffer[MI_COPY_STACK_SIZE];
    /* The buffer - could be from the pool or could be the stack buffer. */
    PVOID buffer = NULL;
    /* The block size - should be the same as the allocated size. */
    ULONG blockSize;
    /* The amount still left to copy. */
    ULONG stillToCopy;
    /* Attach state. */
    KPH_ATTACH_STATE attachState;
    /* The current source address. */
    PVOID sourceAddress;
    /* The current target address. */
    PVOID targetAddress;
    /* Whether we are currently copying. */
    BOOLEAN copying = FALSE;
    /* Whether we are currently probing. */
    BOOLEAN probing = FALSE;
    /* Whether we have the bad address. */
    BOOLEAN haveBadAddress;
    /* The bad address of the exception. */
    ULONG_PTR badAddress;
    
    sourceAddress = FromAddress;
    targetAddress = ToAddress;
    
    /* Don't allocate a buffer larger than the amount we're about to copy. */
    if (allocSize > BufferLength)
        allocSize = BufferLength;
    
    /* If we're copying MI_COPY_STACK_SIZE bytes or less, use the stack buffer. */
    if (BufferLength <= MI_COPY_STACK_SIZE)
    {
        buffer = stackBuffer;
    }
    else
    {
        /* Keep on trying to allocate a buffer, halving the size each time 
           we fail. */
        while (TRUE)
        {
            buffer = ExAllocatePoolWithTag(NonPagedPool, allocSize, TAG_POOL_COPY);
            
            /* Stop trying if we got a buffer. */
            if (buffer)
                break;
            
            /* Otherwise, halve the size and try again. */
            allocSize /= 2;
            /* Could we use the stack buffer? */
            if (allocSize <= MI_COPY_STACK_SIZE)
            {
                buffer = stackBuffer;
                break;
            }
        }
    }
    
    stillToCopy = BufferLength;
    blockSize = allocSize;
    
    /* Perform the copy in blocks of blockSize. */
    while (stillToCopy)
    {
        /* If we're at the last copy block, copy the remaining bytes instead 
           of the whole block size. */
        if (stillToCopy < blockSize)
            blockSize = stillToCopy;
        
        copying = FALSE;
        KphAttachProcess(FromProcess, &attachState);
        
        __try
        {
            /* Probe before reading the source contents. */
            /* Probe only if this is the first time. */
            if ((sourceAddress == FromAddress) && (AccessMode != KernelMode))
            {
                probing = TRUE;
                ProbeForRead(sourceAddress, BufferLength, 1);
                probing = FALSE;
            }
            
            /* Copy the source contents to the buffer. */
            memcpy(buffer, sourceAddress, blockSize);
            KphDetachProcess(&attachState);
            
            /* Probe before writing. */
            KphAttachProcess(ToProcess, &attachState);
            
            /* Probe only if this is the first time. */
            if ((targetAddress == ToAddress) && (AccessMode != KernelMode))
            {
                probing = TRUE;
                ProbeForWrite(targetAddress, BufferLength, 1);
                probing = FALSE;
            }
            
            /* Copy the buffer contents to the destination. */
            copying = TRUE;
            memcpy(targetAddress, buffer, blockSize);
        }
        __except (MiGetExceptionInfo(
            GetExceptionInformation(),
            &haveBadAddress,
            &badAddress
            ))
        {
            KphDetachProcess(&attachState);
            
            /* Free the allocated buffer if needed. */
            if (buffer != stackBuffer)
                ExFreePoolWithTag(buffer, TAG_POOL_COPY);
            
            /* If we were probing an address, return the error code. */
            if (probing)
                return GetExceptionCode();
            
            /* Otherwise, give the caller the number of bytes we copied. */
            *ReturnLength = BufferLength - stillToCopy;
            
            /* If we were copying, we can probably get the exact 
               number of bytes copied. */
            if (copying && haveBadAddress)
                *ReturnLength = (ULONG)(badAddress - (ULONG_PTR)sourceAddress);
            
            return STATUS_PARTIAL_COPY;
        }
        
        KphDetachProcess(&attachState);
        
        stillToCopy -= blockSize;
        sourceAddress = (PVOID)((ULONG_PTR)sourceAddress + blockSize);
        targetAddress = (PVOID)((ULONG_PTR)targetAddress + blockSize);
    }
    
    /* Free the buffer if it wasn't stack-allocated. */
    if (buffer != stackBuffer)
        ExFreePoolWithTag(buffer, TAG_POOL_COPY);
    
    *ReturnLength = BufferLength;
    
    return STATUS_SUCCESS;
}

ULONG MiGetExceptionInfo(
    __in PEXCEPTION_POINTERS ExceptionInfo,
    __out PBOOLEAN HaveBadAddress,
    __out PULONG_PTR BadAddress
    )
{
    PEXCEPTION_RECORD exceptionRecord;
    
    *HaveBadAddress = FALSE;
    exceptionRecord = ExceptionInfo->ExceptionRecord;
    
    if ((exceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION) || 
        (exceptionRecord->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION) || 
        (exceptionRecord->ExceptionCode == STATUS_IN_PAGE_ERROR))
    {
        if (exceptionRecord->NumberParameters > 1)
        {
            /* We have the address. */
            *HaveBadAddress = TRUE;
            *BadAddress = exceptionRecord->ExceptionInformation[1];
        }
    }
    
    return EXCEPTION_EXECUTE_HANDLER;
}

NTSTATUS MmCopyVirtualMemory(
    __in PEPROCESS FromProcess,
    __in PVOID FromAddress,
    __in PEPROCESS ToProcess,
    __in PVOID ToAddress,
    __in ULONG BufferLength,
    __in KPROCESSOR_MODE AccessMode,
    __out PULONG ReturnLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS processToLock = FromProcess;
    
    if (!BufferLength)
        return STATUS_SUCCESS;
    
    /* If we're copying from the current process, lock the target. */
    if (processToLock == PsGetCurrentProcess())
        processToLock = ToProcess;
    
    /* Prevent the process from terminating. */
    if (!KphAcquireProcessRundownProtection(processToLock))
        return STATUS_PROCESS_IS_TERMINATING;
    
    /* If the amount we're trying to copy is over the threshold 
       for MiDoPoolCopy, use MiDoMappedCopy. */
    if (BufferLength > MM_POOL_COPY_THRESHOLD)
    {
        status = MiDoMappedCopy(
            FromProcess,
            FromAddress,
            ToProcess,
            ToAddress,
            BufferLength,
            AccessMode,
            ReturnLength
            );
    }
    else
    {
        status = MiDoPoolCopy(
            FromProcess,
            FromAddress,
            ToProcess,
            ToAddress,
            BufferLength,
            AccessMode,
            ReturnLength
            );
    }
    
    /* Allow the process to terminate. */
    KphReleaseProcessRundownProtection(processToLock);
    
    return status;
}
