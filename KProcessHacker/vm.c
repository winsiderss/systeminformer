/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *
 */

#include <kph.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KphCopyVirtualMemory)
#pragma alloc_text(PAGE, KpiReadVirtualMemoryUnsafe)
#endif

#define KPH_STACK_COPY_BYTES 0x200
#define KPH_POOL_COPY_BYTES 0x10000
#define KPH_MAPPED_COPY_PAGES 14
#define KPH_POOL_COPY_THRESHOLD 0x3ff

ULONG KphpGetCopyExceptionInfo(
    _In_ PEXCEPTION_POINTERS ExceptionInfo,
    _Out_ PBOOLEAN HaveBadAddress,
    _Out_ PULONG_PTR BadAddress
    )
{
    PEXCEPTION_RECORD exceptionRecord;

    *HaveBadAddress = FALSE;
    *BadAddress = 0;
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

/**
 * Copies memory from one process to another.
 *
 * \param FromProcess The source process.
 * \param FromAddress The source address.
 * \param ToProcess The target process.
 * \param ToAddress The target address.
 * \param BufferLength The number of bytes to copy.
 * \param AccessMode The mode in which to perform access checks.
 * \param ReturnLength A variable which receives the number of bytes copied.
 */
NTSTATUS KphCopyVirtualMemory(
    _In_ PEPROCESS FromProcess,
    _In_ PVOID FromAddress,
    _In_ PEPROCESS ToProcess,
    _In_ PVOID ToAddress,
    _In_ SIZE_T BufferLength,
    _In_ KPROCESSOR_MODE AccessMode,
    _Out_ PSIZE_T ReturnLength
    )
{
    UCHAR stackBuffer[KPH_STACK_COPY_BYTES];
    PVOID buffer;
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

    PAGED_CODE();

    sourceAddress = FromAddress;
    targetAddress = ToAddress;

    // We don't check if buffer == NULL when freeing. If buffer doesn't need to be freed, set to
    // stackBuffer, not NULL.
    buffer = stackBuffer;

    mappedTotalSize = (KPH_MAPPED_COPY_PAGES - 2) * PAGE_SIZE;

    if (mappedTotalSize > BufferLength)
        mappedTotalSize = BufferLength;

    stillToCopy = BufferLength;
    blockSize = mappedTotalSize;

    while (stillToCopy)
    {
        // If we're at the last copy block, copy the remaining bytes instead of the whole block
        // size.
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
                if (buffer != stackBuffer)
                    ExFreePoolWithTag(buffer, 'ChpK');

                buffer = stackBuffer;
            }
            else
            {
                // Don't allocate the buffer if we've done so already. Note that the block size
                // never increases, so this allocation will always be OK.
                if (buffer == stackBuffer)
                {
                    // Keep trying to allocate a buffer.

                    while (TRUE)
                    {
                        buffer = ExAllocatePoolZero(NonPagedPool, blockSize, 'ChpK');

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
                *ReturnLength = (SIZE_T)PTR_SUB_OFFSET(badAddress, sourceAddress);
            }
            else
            {
                *ReturnLength = BufferLength - stillToCopy;
            }

            return STATUS_PARTIAL_COPY;
        }

        KeUnstackDetachProcess(&apcState);

        if (doMappedCopy)
        {
            MmUnmapLockedPages(mappedAddress, mdl);
            MmUnlockPages(mdl);
        }

        stillToCopy -= blockSize;
        sourceAddress = PTR_ADD_OFFSET(sourceAddress, blockSize);
        targetAddress = PTR_ADD_OFFSET(targetAddress, blockSize);
    }

    if (buffer != stackBuffer)
        ExFreePoolWithTag(buffer, 'ChpK');

    *ReturnLength = BufferLength;

    return STATUS_SUCCESS;
}

/**
 * Copies process or kernel memory into the current process.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_VM_READ access. This
 * parameter may be NULL if \a BaseAddress lies above the user-mode range.
 * \param BaseAddress The address from which memory is to be copied.
 * \param Buffer A buffer which receives the copied memory.
 * \param BufferSize The number of bytes to copy.
 * \param NumberOfBytesRead A variable which receives the number of bytes copied to the buffer.
 * \param Key An access key. If no valid L2 key is provided, the function fails.
 * \param Client The client that initiated the request.
 * \param AccessMode The mode in which to perform access checks.
 */
NTSTATUS KpiReadVirtualMemoryUnsafe(
    _In_opt_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Out_opt_ PSIZE_T NumberOfBytesRead,
    _In_opt_ KPH_KEY Key,
    _In_ PKPH_CLIENT Client,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;
    SIZE_T numberOfBytesRead = 0;

    PAGED_CODE();

    if (!NT_SUCCESS(status = KphValidateKey(KphKeyLevel2, Key, Client, AccessMode)))
        return status;

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

            status = KphValidateAddressForSystemModules(BaseAddress, BufferSize);

            if (!NT_SUCCESS(status))
                return status;

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
