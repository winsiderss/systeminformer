/*
 * KProcessHacker
 * 
 * Copyright (C) 2010 wj32
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

#include <kph.h>

ULONG KphpGetCopyExceptionInfo(
    __in PEXCEPTION_POINTERS ExceptionInfo,
    __out PBOOLEAN HaveBadAddress,
    __out PULONG_PTR BadAddress
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KphpGetCopyExceptionInfo)
#pragma alloc_text(PAGE, KphCopyVirtualMemory)
#pragma alloc_text(PAGE, KpiReadVirtualMemory)
#pragma alloc_text(PAGE, KpiWriteVirtualMemory)
#endif

#define KPH_STACK_COPY_BYTES 0x200
#define KPH_POOL_COPY_BYTES 0x10000
#define KPH_MAPPED_COPY_PAGES 14
#define KPH_POOL_COPY_THRESHOLD 0x3ff

ULONG KphpGetCopyExceptionInfo(
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

NTSTATUS KphCopyVirtualMemory(
    __in PEPROCESS FromProcess,
    __in PVOID FromAddress,
    __in PEPROCESS ToProcess,
    __in PVOID ToAddress,
    __in SIZE_T BufferLength,
    __in KPROCESSOR_MODE AccessMode,
    __out PSIZE_T ReturnLength
    )
{
    UCHAR stackBuffer[KPH_STACK_COPY_BYTES];
    PVOID buffer = NULL;
    PFN_NUMBER mdlBuffer[(sizeof(MDL) / sizeof(PFN_NUMBER)) + KPH_MAPPED_COPY_PAGES + 1];
    PMDL mdl = (PMDL)mdlBuffer;
    PVOID mappedAddress;
    SIZE_T mappedTotalSize;
    SIZE_T blockSize;
    SIZE_T stillToCopy;
    KAPC_STATE apcState;
    PVOID sourceAddress;
    PVOID targetAddress;
    BOOLEAN doMappedCopy;
    BOOLEAN pagesLocked;
    BOOLEAN copyingToTarget = FALSE;
    BOOLEAN probing = FALSE;
    BOOLEAN mapping = FALSE;
    BOOLEAN haveBadAddress;
    ULONG_PTR badAddress;

    sourceAddress = FromAddress;
    targetAddress = ToAddress;

    mappedTotalSize = (KPH_MAPPED_COPY_PAGES - 2) * PAGE_SIZE;

    if (mappedTotalSize > BufferLength)
        mappedTotalSize = BufferLength;

    stillToCopy = BufferLength;
    blockSize = mappedTotalSize;

    while (stillToCopy)
    {
        // If we're at the last copy block, copy the remaining bytes instead
        // of the whole block size.
        if (blockSize > stillToCopy)
            blockSize = stillToCopy;

        // Choose the best method based on the number of bytes left to copy.
        if (blockSize > KPH_POOL_COPY_THRESHOLD)
        {
            doMappedCopy = TRUE;
        }
        else
        {
            doMappedCopy = FALSE;

            if (blockSize <= KPH_STACK_COPY_BYTES)
            {
                if (buffer)
                    ExFreePoolWithTag(buffer, 'ChpK');

                buffer = stackBuffer;
            }
            else
            {
                // Don't allocate the buffer if we've done so already. 
                // Note that the block size never increases, so this allocation 
                // will always be OK.
                if (!buffer)
                {
                    // Keep trying to allocate a buffer.

                    while (TRUE)
                    {
                        buffer = ExAllocatePoolWithTag(NonPagedPool, blockSize, 'ChpK');

                        // Stop trying if we got a buffer.
                        if (buffer)
                            break;

                        blockSize /= 2;

                        // Use the stack buffer if we can.
                        if (blockSize <= KPH_STACK_COPY_BYTES)
                        {
                            buffer = stackBuffer;
                            break;
                        }
                    }
                }
            }
        }

        // Reset state.
        mappedAddress = NULL;
        pagesLocked = FALSE;
        copyingToTarget = FALSE;

        KeStackAttachProcess(FromProcess, &apcState);

        __try
        {
            // Probe only if this is the first time.
            if (sourceAddress == FromAddress && AccessMode != KernelMode)
            {
                probing = TRUE;
                ProbeForRead(sourceAddress, BufferLength, sizeof(UCHAR));
                probing = FALSE;
            }

            if (doMappedCopy)
            {
                // Initialize the MDL.
                MmInitializeMdl(mdl, sourceAddress, blockSize);
                MmProbeAndLockPages(mdl, AccessMode, IoReadAccess);
                pagesLocked = TRUE;

                // Map the pages.
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
                    // Insufficient resources; exit.
                    mapping = TRUE;
                    ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                }
            }
            else
            {
                memcpy(buffer, sourceAddress, blockSize);
            }

            KeUnstackDetachProcess(&apcState);

            // Attach to the target process and copy the contents out.
            KeStackAttachProcess(ToProcess, &apcState);

            // Probe only if this is the first time.
            if (targetAddress == ToAddress && AccessMode != KernelMode)
            {
                probing = TRUE;
                ProbeForWrite(targetAddress, BufferLength, sizeof(UCHAR));
                probing = FALSE;
            }

            // Copy the data.
            copyingToTarget = TRUE;

            if (doMappedCopy)
                memcpy(targetAddress, mappedAddress, blockSize);
            else
                memcpy(targetAddress, buffer, blockSize);
        }
        __except (KphpGetCopyExceptionInfo(
            GetExceptionInformation(),
            &haveBadAddress,
            &badAddress
            ))
        {
            KeUnstackDetachProcess(&apcState);

            // If we mapped the pages, unmap them.
            if (mappedAddress)
                MmUnmapLockedPages(mappedAddress, mdl);

            // If we locked the pages, unlock them.
            if (pagesLocked)
                MmUnlockPages(mdl);

            // If we allocated pool storage, free it.
            if (buffer != stackBuffer)
                ExFreePoolWithTag(buffer, 'ChpK');

            // If we failed when probing or mapping, return the error status.
            if (probing || mapping)
                return GetExceptionCode();

            // Determine which copy failed.
            if (copyingToTarget && haveBadAddress)
            {
                *ReturnLength = (ULONG)(badAddress - (ULONG_PTR)sourceAddress);
            }
            else
            {
                *ReturnLength = BufferLength - stillToCopy;
            }

            return STATUS_PARTIAL_COPY;
        }

        KeUnstackDetachProcess(&apcState);
        MmUnmapLockedPages(mappedAddress, mdl);
        MmUnlockPages(mdl);

        stillToCopy -= blockSize;
        sourceAddress = (PVOID)((ULONG_PTR)sourceAddress + blockSize);
        targetAddress = (PVOID)((ULONG_PTR)targetAddress + blockSize);
    }

    if (buffer != stackBuffer)
        ExFreePoolWithTag(buffer, 'ChpK');

    *ReturnLength = BufferLength;

    return STATUS_SUCCESS;
}

NTSTATUS KpiReadVirtualMemory(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __out_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesRead,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;
    SIZE_T numberOfBytesRead;

    if (AccessMode != KernelMode)
    {
        if (
            (ULONG_PTR)BaseAddress + BufferSize < (ULONG_PTR)BaseAddress ||
            (ULONG_PTR)Buffer + BufferSize < (ULONG_PTR)Buffer ||
            (ULONG_PTR)BaseAddress + BufferSize > (ULONG_PTR)MmHighestUserAddress ||
            (ULONG_PTR)Buffer + BufferSize > (ULONG_PTR)MmHighestUserAddress
            )
        {
            return STATUS_ACCESS_VIOLATION;
        }

        if (NumberOfBytesRead)
        {
            __try
            {
                ProbeForWrite(NumberOfBytesRead, sizeof(SIZE_T), sizeof(SIZE_T));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return GetExceptionCode();
            }
        }
    }

    if (BufferSize != 0)
    {
        status = ObReferenceObjectByHandle(
            ProcessHandle,
            PROCESS_VM_READ,
            *PsProcessType,
            AccessMode,
            &process,
            NULL
            );

        if (NT_SUCCESS(status))
        {
            status = KphCopyVirtualMemory(
                process,
                BaseAddress,
                PsGetCurrentProcess(),
                Buffer,
                BufferSize,
                AccessMode,
                &numberOfBytesRead
                );
            ObDereferenceObject(process);
        }
    }
    else
    {
        numberOfBytesRead = 0;
        status = STATUS_SUCCESS;
    }

    if (NumberOfBytesRead)
    {
        if (AccessMode != KernelMode)
        {
            __try
            {
                *NumberOfBytesRead = numberOfBytesRead;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                // Don't mess with the status.
                NOTHING;
            }
        }
        else
        {
            *NumberOfBytesRead = numberOfBytesRead;
        }
    }

    return status;
}

NTSTATUS KpiWriteVirtualMemory(
    __in HANDLE ProcessHandle,
    __in_opt PVOID BaseAddress,
    __in_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesWritten,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;
    SIZE_T numberOfBytesWritten;

    if (AccessMode != KernelMode)
    {
        if (
            (ULONG_PTR)BaseAddress + BufferSize < (ULONG_PTR)BaseAddress ||
            (ULONG_PTR)Buffer + BufferSize < (ULONG_PTR)Buffer ||
            (ULONG_PTR)BaseAddress + BufferSize > (ULONG_PTR)MmHighestUserAddress ||
            (ULONG_PTR)Buffer + BufferSize > (ULONG_PTR)MmHighestUserAddress
            )
        {
            return STATUS_ACCESS_VIOLATION;
        }

        if (NumberOfBytesWritten)
        {
            __try
            {
                ProbeForWrite(NumberOfBytesWritten, sizeof(SIZE_T), sizeof(SIZE_T));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return GetExceptionCode();
            }
        }
    }

    if (BufferSize != 0)
    {
        status = ObReferenceObjectByHandle(
            ProcessHandle,
            PROCESS_VM_WRITE,
            *PsProcessType,
            AccessMode,
            &process,
            NULL
            );

        if (NT_SUCCESS(status))
        {
            status = KphCopyVirtualMemory(
                PsGetCurrentProcess(),
                Buffer,
                process,
                BaseAddress,
                BufferSize,
                AccessMode,
                &numberOfBytesWritten
                );
            ObDereferenceObject(process);
        }
    }
    else
    {
        numberOfBytesWritten = 0;
        status = STATUS_SUCCESS;
    }

    if (NumberOfBytesWritten)
    {
        if (AccessMode != KernelMode)
        {
            __try
            {
                *NumberOfBytesWritten = numberOfBytesWritten;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                // Don't mess with the status.
                NOTHING;
            }
        }
        else
        {
            *NumberOfBytesWritten = numberOfBytesWritten;
        }
    }

    return status;
}

NTSTATUS KpiReadVirtualMemoryUnsafe(
    __in_opt HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __out_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesRead,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;
    SIZE_T numberOfBytesRead;

    if (AccessMode != KernelMode)
    {
        if (
            (ULONG_PTR)BaseAddress + BufferSize < (ULONG_PTR)BaseAddress ||
            (ULONG_PTR)Buffer + BufferSize < (ULONG_PTR)Buffer ||
            (ULONG_PTR)Buffer + BufferSize > (ULONG_PTR)MmHighestUserAddress
            )
        {
            return STATUS_ACCESS_VIOLATION;
        }

        if (NumberOfBytesRead)
        {
            __try
            {
                ProbeForWrite(NumberOfBytesRead, sizeof(SIZE_T), sizeof(SIZE_T));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return GetExceptionCode();
            }
        }
    }

    if (BufferSize != 0)
    {
        // Select the appropriate copy method.
        if ((ULONG_PTR)BaseAddress + BufferSize > (ULONG_PTR)MmHighestUserAddress)
        {
            ULONG_PTR page;
            ULONG_PTR pageEnd;

            // Kernel memory copy (unsafe)

            page = (ULONG_PTR)BaseAddress & ~(PAGE_SIZE - 1);
            pageEnd = ((ULONG_PTR)BaseAddress + BufferSize - 1) & ~(PAGE_SIZE - 1);

            __try
            {
                // This will obviously fail if any of the pages aren't resident.
                for (; page <= pageEnd; page += PAGE_SIZE)
                {
                    if (!MmIsAddressValid((PVOID)page))
                        ExRaiseStatus(STATUS_ACCESS_VIOLATION);
                }

                memcpy(Buffer, BaseAddress, BufferSize);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return GetExceptionCode();
            }

            numberOfBytesRead = BufferSize;
            status = STATUS_SUCCESS;
        }
        else
        {
            // User memory copy (safe)

            status = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_VM_READ,
                *PsProcessType,
                AccessMode,
                &process,
                NULL
                );

            if (NT_SUCCESS(status))
            {
                status = KphCopyVirtualMemory(
                    process,
                    BaseAddress,
                    PsGetCurrentProcess(),
                    Buffer,
                    BufferSize,
                    AccessMode,
                    &numberOfBytesRead
                    );
                ObDereferenceObject(process);
            }
        }
    }
    else
    {
        numberOfBytesRead = 0;
        status = STATUS_SUCCESS;
    }

    if (NumberOfBytesRead)
    {
        if (AccessMode != KernelMode)
        {
            __try
            {
                *NumberOfBytesRead = numberOfBytesRead;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                // Don't mess with the status.
                NOTHING;
            }
        }
        else
        {
            *NumberOfBytesRead = numberOfBytesRead;
        }
    }

    return status;
}
