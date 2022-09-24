/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     jxy-s   2022
 *
 */

#include <kph.h>
#include <dyndata.h>

#include <trace.h>

PAGED_FILE();

#define KPH_STACK_COPY_BYTES 0x200
#define KPH_POOL_COPY_BYTES 0x10000
#define KPH_MAPPED_COPY_PAGES 14
#define KPH_POOL_COPY_THRESHOLD 0x3ff

/**
 * \brief Copies out the bad address from a virtual memory flavored exception.
 *
 * \param[in] ExceptionInfo Exception information to copy from.
 * \param[out] HaveBadAddress Set to true if the exception is flavored properly
 * to retrieve the bad address.
 * \param[out] BadAddress Set to the bad address if found.
 *
 * \return EXCEPTION_EXECUTE_HANDLER
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
ULONG KphpGetCopyExceptionInfo(
    _In_ PEXCEPTION_POINTERS ExceptionInfo,
    _Out_ PBOOLEAN HaveBadAddress,
    _Out_ PULONG_PTR BadAddress
    )
{
    PEXCEPTION_RECORD exceptionRecord;

    PAGED_PASSIVE();

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
 * \brief Copies memory from one process to another.
 *
 * \param[in] FromProcess The source process.
 * \param[in] FromAddress The source address.
 * \param[in] ToProcess The target process.
 * \param[out] ToAddress The target address.
 * \param[in] BufferLength The number of bytes to copy.
 * \param[in] AccessMode The mode in which to perform access checks.
 * \param[out] ReturnLength A variable which receives the number of bytes copied.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCopyVirtualMemory(
    _In_ PEPROCESS FromProcess,
    _In_ PVOID FromAddress,
    _In_ PEPROCESS ToProcess,
    _Out_writes_bytes_(BufferLength) PVOID ToAddress,
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

    PAGED_PASSIVE();

    sourceAddress = FromAddress;
    targetAddress = ToAddress;
    haveBadAddress = FALSE;
    badAddress = 0;

    //
    // We don't check if buffer == NULL when freeing. If buffer doesn't need to
    // be freed, set to stackBuffer, not NULL.
    //
    buffer = stackBuffer;

    mappedTotalSize = (KPH_MAPPED_COPY_PAGES - 2) * PAGE_SIZE;

    if (mappedTotalSize > BufferLength)
    {
        mappedTotalSize = BufferLength;
    }

    stillToCopy = BufferLength;
    blockSize = mappedTotalSize;

    while (stillToCopy)
    {
        //
        // If we're at the last copy block, copy the remaining bytes instead of
        // the whole block size.
        //
        if (blockSize > stillToCopy)
        {
            blockSize = stillToCopy;
        }

        //
        // Choose the best method based on the number of bytes left to copy.
        //
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
                {
                    KphFree(buffer, KPH_TAG_COPY_VM);
                }

                buffer = stackBuffer;
            }
            else
            {
                //
                // Don't allocate the buffer if we've done so already. Note
                // that the block size never increases, so this allocation will
                // always be OK.
                //
                if (buffer == stackBuffer)
                {
                    while (TRUE)
                    {
                        buffer = KphAllocateNPaged(blockSize, KPH_TAG_COPY_VM);

                        if (buffer)
                        {
                            break;
                        }

                        blockSize /= 2;

                        //
                        // Use the stack buffer if we can.
                        //
                        if (blockSize <= KPH_STACK_COPY_BYTES)
                        {
                            buffer = stackBuffer;
                            break;
                        }
                    }
                }
            }
        }

        //
        // Reset state.
        //
        mappedAddress = NULL;
        pagesLocked = FALSE;
        copyingToTarget = FALSE;

        KeStackAttachProcess(FromProcess, &apcState);

        __try
        {
            //
            // Probe only if this is the first time.
            //
            if ((sourceAddress == FromAddress) && (AccessMode != KernelMode))
            {
                probing = TRUE;
                ProbeForRead(sourceAddress,
                             BufferLength,
                             TYPE_ALIGNMENT(UCHAR));
                probing = FALSE;
            }

            if (doMappedCopy)
            {
                MmInitializeMdl(mdl, sourceAddress, blockSize);
                MmProbeAndLockPages(mdl, AccessMode, IoReadAccess);
                pagesLocked = TRUE;

                mappedAddress = MmMapLockedPagesSpecifyCache(mdl,
                                                             KernelMode,
                                                             MmCached,
                                                             NULL,
                                                             FALSE,
                                                             HighPagePriority);
                if (!mappedAddress)
                {
                    mapping = TRUE;
                    ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                }
            }
            else
            {
                RtlCopyMemory(buffer, sourceAddress, blockSize);
            }

            KeUnstackDetachProcess(&apcState);

            KeStackAttachProcess(ToProcess, &apcState);

            //
            // Probe only if this is the first time.
            //
            if (targetAddress == ToAddress && AccessMode != KernelMode)
            {
                probing = TRUE;
#pragma prefast(suppress : 6001)
                ProbeForWrite(targetAddress,
                              BufferLength,
                              TYPE_ALIGNMENT(UCHAR));
                probing = FALSE;
            }

            copyingToTarget = TRUE;

            if (doMappedCopy)
            {
                NT_ASSERT(mdl->ByteCount >= blockSize);
                RtlCopyMemory(targetAddress, mappedAddress, blockSize);
            }
            else
            {
                RtlCopyMemory(targetAddress, buffer, blockSize);
            }
        }
        __except (KphpGetCopyExceptionInfo(GetExceptionInformation(),
                                           &haveBadAddress,
                                           &badAddress))
        {
            KeUnstackDetachProcess(&apcState);

            if (mappedAddress)
            {
                MmUnmapLockedPages(mappedAddress, mdl);
            }

            if (pagesLocked)
            {
                MmUnlockPages(mdl);
            }

            if (buffer != stackBuffer)
            {
                KphFree(buffer, KPH_TAG_COPY_VM);
            }

            if (probing || mapping)
            {
                return GetExceptionCode();
            }

            // Determine which copy failed.
            if (copyingToTarget && haveBadAddress)
            {
                *ReturnLength = (SIZE_T)(badAddress + (ULONG_PTR)sourceAddress);
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
        sourceAddress = Add2Ptr(sourceAddress, blockSize);
        targetAddress = Add2Ptr(targetAddress, blockSize);
    }

    if (buffer != stackBuffer)
    {
        KphFree(buffer, KPH_TAG_COPY_VM);
    }

    *ReturnLength = BufferLength;

    return STATUS_SUCCESS;
}

/**
 * \brief Copies process or kernel memory into the current process.
 *
 * \param[in] ProcessHandle A handle to a process. The handle must have
 * PROCESS_VM_READ access. This parameter may be NULL if \a BaseAddress lies
 * above the user-mode range.
 * \param[in] BaseAddress The address from which memory is to be copied.
 * \param[out] Buffer A buffer which receives the copied memory.
 * \param[in] BufferSize The number of bytes to copy.
 * \param[out] NumberOfBytesRead A variable which receives the number of bytes
 * copied to the buffer.
 * \param[in] AccessMode The mode in which to perform access checks.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphReadVirtualMemoryUnsafe(
    _In_opt_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Out_opt_ PSIZE_T NumberOfBytesRead,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;
    SIZE_T numberOfBytesRead;
    BOOLEAN releaseModuleLock;

    PAGED_PASSIVE();

    numberOfBytesRead = 0;
    releaseModuleLock = FALSE;

    if (!Buffer)
    {
        status = STATUS_INVALID_PARAMETER_3;
        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        if ((Add2Ptr(BaseAddress, BufferSize) < BaseAddress) ||
            (Add2Ptr(Buffer, BufferSize) < Buffer) ||
            (Add2Ptr(Buffer, BufferSize) > MmHighestUserAddress))
        {
            status = STATUS_ACCESS_VIOLATION;
            goto Exit;
        }

        if (NumberOfBytesRead)
        {
            __try
            {
                ProbeOutputType(NumberOfBytesRead, SIZE_T);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
                goto Exit;
            }
        }
    }

    if (BufferSize != 0)
    {
        //
        // Select the appropriate copy method.
        //
        if (Add2Ptr(BaseAddress, BufferSize) > MmHighestUserAddress)
        {
            ULONG_PTR page;
            ULONG_PTR pageEnd;

            if (KphDynPsLoadedModuleResource)
            {
                //
                // Prevent TOCTOU between checking the system module list and
                // copying memory.
                //
                if (!ExAcquireResourceSharedLite(KphDynPsLoadedModuleResource,
                                                 TRUE))
                {
                    KphTracePrint(TRACE_LEVEL_ERROR,
                                  GENERAL,
                                  "Failed to acquire PsLoadedModuleResource");

                    status = STATUS_RESOURCE_NOT_OWNED;
                    goto Exit;
                }

                releaseModuleLock = TRUE;
            }

            status = KphValidateAddressForSystemModules(BaseAddress,
                                                        BufferSize);

            if (!NT_SUCCESS(status))
            {
                goto Exit;
            }

            //
            // Kernel memory copy (unsafe)
            //

            page = (ULONG_PTR)BaseAddress & ~(PAGE_SIZE - 1);
            pageEnd = ((ULONG_PTR)BaseAddress + BufferSize - 1) & ~(PAGE_SIZE - 1);

            __try
            {
                for (; page <= pageEnd; page += PAGE_SIZE)
                {
                    if (!MmIsAddressValid((PVOID)page))
                    {
                        ExRaiseStatus(STATUS_ACCESS_VIOLATION);
                    }
                }

                RtlCopyMemory(Buffer, BaseAddress, BufferSize);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
                goto Exit;
            }

            numberOfBytesRead = BufferSize;
            status = STATUS_SUCCESS;
        }
        else
        {
            //
            // User memory copy (safe)
            //

            if (!ProcessHandle)
            {
                status = STATUS_INVALID_PARAMETER_1;
                goto Exit;
            }

            status = ObReferenceObjectByHandle(ProcessHandle,
                                               PROCESS_VM_READ,
                                               *PsProcessType,
                                               AccessMode,
                                               &process,
                                               NULL);
            if (NT_SUCCESS(status))
            {
                status = KphCopyVirtualMemory(process,
                                              BaseAddress,
                                              PsGetCurrentProcess(),
                                              Buffer,
                                              BufferSize,
                                              AccessMode,
                                              &numberOfBytesRead);
                ObDereferenceObject(process);
            }
        }
    }
    else
    {
        numberOfBytesRead = 0;
        status = STATUS_SUCCESS;
    }

Exit:

    if (releaseModuleLock)
    {
        NT_ASSERT(KphDynPsLoadedModuleResource);
        ExReleaseResourceLite(KphDynPsLoadedModuleResource);
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
