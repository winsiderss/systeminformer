/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     jxy-s   2022-2023
 *
 */

#include <kph.h>

#include <trace.h>

/**
 * \brief Queries information on mappings for a given section object.
 *
 * \param[in] SectionObject The section object to query the info of.
 * \param[out] SectionInformation Populated with the information. This storage
 * must be located in non-paged system-space memory.
 * \param[in] SectionInformationLength Length of the section information.
 * \param[out] ReturnLength Set to the number of bytes written or the required
 * number of bytes if the input length is insufficient.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS KphpQuerySectionMappings(
    _In_ PVOID SectionObject,
    _Out_writes_bytes_opt_(SectionInformationLength) PVOID SectionInformation,
    _In_ ULONG SectionInformationLength,
    _Out_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    PKPH_DYN dyn;
    ULONG returnLength;
    PVOID controlArea;
    PLIST_ENTRY listHead;
    PKPH_SECTION_MAPPINGS_INFORMATION info;
    PEX_SPIN_LOCK lock;
    KIRQL oldIrql;

    NPAGED_CODE_DISPATCH_MAX();

    lock = NULL;
    oldIrql = 0;
    returnLength = 0;

    dyn = KphReferenceDynData();

    if (!dyn ||
        (dyn->MmSectionControlArea == ULONG_MAX) ||
        (dyn->MmControlAreaListHead == ULONG_MAX) ||
        (dyn->MmControlAreaLock == ULONG_MAX))
    {
        status = STATUS_NOINTERFACE;
        goto Exit;
    }

    controlArea = *(PVOID*)Add2Ptr(SectionObject, dyn->MmSectionControlArea);
    if ((ULONG_PTR)controlArea & 3)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "Section remote mappings not supported.");

        status = STATUS_NOT_SUPPORTED;
        goto Exit;
    }

    controlArea = (PVOID)((ULONG_PTR)controlArea & ~3);
    if (!controlArea)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "Section control area is null.");

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    lock = Add2Ptr(controlArea, dyn->MmControlAreaLock);
    oldIrql = ExAcquireSpinLockShared(lock);

    listHead = Add2Ptr(controlArea, dyn->MmControlAreaListHead);

    //
    // Links are shared in a union with AweContext pointer. Ensure that both
    // links look valid.
    //
    if (!listHead->Flink || !listHead->Blink ||
        (listHead->Flink->Blink != listHead) ||
        (listHead->Blink->Flink != listHead))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "Section unexpected control area links.");

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    returnLength = UFIELD_OFFSET(KPH_SECTION_MAPPINGS_INFORMATION, Mappings);

    for (PLIST_ENTRY link = listHead->Flink;
         link != listHead;
         link = link->Flink)
    {
        status = RtlULongAdd(returnLength,
                             sizeof(KPH_SECTION_MAP_ENTRY),
                             &returnLength);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "RtlULongAdd failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }

    if (!SectionInformation || (SectionInformationLength < returnLength))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        goto Exit;
    }

    info = SectionInformation;

    info->NumberOfMappings = 0;

    for (PLIST_ENTRY link = listHead->Flink;
         link != listHead;
         link = link->Flink)
    {
        PMMVAD vad;
        PKPH_SECTION_MAP_ENTRY entry;

        vad = CONTAINING_RECORD(link, MMVAD, ViewLinks);
        entry = &info->Mappings[info->NumberOfMappings++];

        entry->ViewMapType = vad->ViewMapType;
        entry->ProcessId = NULL;
        if (vad->ViewMapType == VIEW_MAP_TYPE_PROCESS)
        {
            PEPROCESS process;

            process = (PEPROCESS)((ULONG_PTR)vad->VadsProcess & ~VIEW_MAP_TYPE_PROCESS);
            if (process)
            {
                entry->ProcessId = PsGetProcessId(process);
            }
        }
        entry->StartVa = MiGetVadStartAddress(vad);
        entry->EndVa = MiGetVadEndAddress(vad);
    }

    status = STATUS_SUCCESS;

Exit:

    *ReturnLength = returnLength;

    if (lock)
    {
        ExReleaseSpinLockShared(lock, oldIrql);
    }

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    return status;
}

PAGED_FILE();

/**
 * \brief Copies process or kernel memory into the current process.
 *
 * \param[in] ProcessHandle A handle to a process. The handle must have
 * PROCESS_VM_READ access. This parameter may be NULL if BaseAddress lies above
 * the user-mode range.
 * \param[in] BaseAddress The address from which memory is to be copied.
 * \param[out] Buffer A buffer which receives the copied memory.
 * \param[in] BufferSize The number of bytes to copy.
 * \param[out] NumberOfBytesRead A variable which receives the number of bytes
 * copied to the buffer.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphReadVirtualMemory(
    _In_opt_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Out_opt_ PSIZE_T NumberOfBytesRead,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    SIZE_T numberOfBytesRead;
    BOOLEAN releaseModuleLock;
    PVOID buffer;
    BYTE stackBuffer[0x200];

    PAGED_CODE_PASSIVE();

    numberOfBytesRead = 0;
    releaseModuleLock = FALSE;
    buffer = NULL;

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

        __try
        {
            ProbeOutputBytes(Buffer, BufferSize);

            if (NumberOfBytesRead)
            {
                ProbeOutputType(NumberOfBytesRead, SIZE_T);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }

    if (!BufferSize)
    {
        status = STATUS_SUCCESS;
        goto Exit;
    }

    //
    // Select the appropriate copy method.
    //

    if (Add2Ptr(BaseAddress, BufferSize) > MmHighestUserAddress)
    {
        MM_COPY_ADDRESS copyAddress;

        //
        // Only permit reading from the system module ranges. Prevent TOCTOU
        // between checking the system module list and copying.
        //

        KeEnterCriticalRegion();
        if (!ExAcquireResourceSharedLite(PsLoadedModuleResource, TRUE))
        {
            KeLeaveCriticalRegion();

            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "Failed to acquire PsLoadedModuleResource");

            status = STATUS_RESOURCE_NOT_OWNED;
            goto Exit;
        }

        releaseModuleLock = TRUE;

        status = KphValidateAddressForSystemModules(BaseAddress, BufferSize);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "KphValidateAddressForSystemModules failed: %!STATUS!",
                          status);

            goto Exit;
        }

        if (BufferSize <= ARRAYSIZE(stackBuffer))
        {
            RtlZeroMemory(stackBuffer, ARRAYSIZE(stackBuffer));
            buffer = stackBuffer;
        }
        else
        {
            buffer = KphAllocateNPaged(BufferSize, KPH_TAG_COPY_VM);
            if (!buffer)
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "Failed to allocate copy buffer.");

                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Exit;
            }
        }

        copyAddress.VirtualAddress = BaseAddress;

        status = MmCopyMemory(buffer,
                              copyAddress,
                              BufferSize,
                              MM_COPY_MEMORY_VIRTUAL,
                              &numberOfBytesRead);

        __try
        {
            RtlCopyMemory(Buffer, buffer, numberOfBytesRead);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
        }
    }
    else
    {
        PEPROCESS process;

        if (!ProcessHandle)
        {
            status = STATUS_INVALID_PARAMETER_1;
            goto Exit;
        }

        status = ObReferenceObjectByHandle(ProcessHandle,
                                           0,
                                           *PsProcessType,
                                           AccessMode,
                                           &process,
                                           NULL);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "ObReferenceObjectByHandle failed: %!STATUS!",
                          status);

            goto Exit;
        }

        status = MmCopyVirtualMemory(process,
                                     BaseAddress,
                                     PsGetCurrentProcess(),
                                     Buffer,
                                     BufferSize,
                                     AccessMode,
                                     &numberOfBytesRead);

        ObDereferenceObject(process);
    }

Exit:

    if (buffer && (buffer != stackBuffer))
    {
        KphFree(buffer, KPH_TAG_COPY_VM);
    }

    if (releaseModuleLock)
    {
        ExReleaseResourceLite(PsLoadedModuleResource);
        KeLeaveCriticalRegion();
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

/**
 * \brief Queries information about a section.
 *
 * \param[in] SectionHandle Handle to the query to query information of.
 * \param[in] SectionInformationClass Classification of information to query.
 * \param[out] SectionInformation Populated with the requested information.
 * \param[in] SectionInformationLength Length of the information buffer.
 * \param[out] ReturnLength Set to the number of bytes written or the required
 * number of bytes if the input length is insufficient.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQuerySection(
    _In_ HANDLE SectionHandle,
    _In_ KPH_SECTION_INFORMATION_CLASS SectionInformationClass,
    _Out_writes_bytes_opt_(SectionInformationLength) PVOID SectionInformation,
    _In_ ULONG SectionInformationLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PVOID sectionObject;
    ULONG returnLength;
    PVOID buffer;
    BYTE stackBuffer[64];

    PAGED_CODE_PASSIVE();

    sectionObject = NULL;
    returnLength = 0;
    buffer = NULL;

    if (AccessMode != KernelMode)
    {
        __try
        {
            if (SectionInformation)
            {
                ProbeOutputBytes(SectionInformation, SectionInformationLength);
            }

            if (ReturnLength)
            {
                ProbeOutputType(ReturnLength, ULONG);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }

    status = ObReferenceObjectByHandle(SectionHandle,
                                       0,
                                       *MmSectionObjectType,
                                       KernelMode,
                                       &sectionObject,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        sectionObject = NULL;
        goto Exit;
    }

    switch (SectionInformationClass)
    {
        case KphSectionMappingsInformation:
        {
            if (SectionInformation)
            {
                if (SectionInformationLength <= ARRAYSIZE(stackBuffer))
                {
                    RtlZeroMemory(stackBuffer, ARRAYSIZE(stackBuffer));
                    buffer = stackBuffer;
                }
                else
                {
                    buffer = KphAllocateNPaged(SectionInformationLength,
                                               KPH_TAG_SECTION_QUERY);
                    if (!buffer)
                    {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        goto Exit;
                    }
                }
            }
            else
            {
                NT_ASSERT(!buffer);
                SectionInformationLength = 0;
            }

            status = KphpQuerySectionMappings(sectionObject,
                                              buffer,
                                              SectionInformationLength,
                                              &returnLength);
            if (!NT_SUCCESS(status))
            {
                goto Exit;
            }

            if (!SectionInformation)
            {
                status = STATUS_BUFFER_TOO_SMALL;
                goto Exit;
            }

            __try
            {
                RtlCopyMemory(SectionInformation, buffer, returnLength);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
            }

            break;
        }
        default:
        {
            status = STATUS_INVALID_INFO_CLASS;
            break;
        }
    }

Exit:

    if (ReturnLength)
    {
        if (AccessMode != KernelMode)
        {
            __try
            {
                *ReturnLength = returnLength;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                NOTHING;
            }
        }
        else
        {
            *ReturnLength = returnLength;
        }
    }

    if (sectionObject)
    {
        ObDereferenceObject(sectionObject);
    }

    if (buffer && (buffer != stackBuffer))
    {
        KphFree(buffer, KPH_TAG_SECTION_QUERY);
    }

    return status;
}

/**
 * \brief Queries information about a region of pages within the virtual address
 * space of the specified process.
 *
 * \param[in] ProcessHandle Handle for the process whose context the pages to be
 * queried resides.
 * \param[in] BaseAddress The base address of the region of pages to be queried.
 * \param[in] MemoryInformationClass The memory information to retrieve.
 * \param[out] MemoryInformation Populated with the requested information.
 * \param[in] MemoryInformationLength Length of the information buffer.
 * \param[out] ReturnLength Set to the number of bytes written or the required
 * number of bytes if the input length is insufficient.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _In_ KPH_MEMORY_INFORMATION_CLASS MemoryInformationClass,
    _Out_writes_bytes_opt_(MemoryInformationLength) PVOID MemoryInformation,
    _In_ ULONG MemoryInformationLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    ULONG returnLength;
    PKPH_THREAD_CONTEXT thread;
    PUNICODE_STRING mappedFileName;

    PAGED_CODE_PASSIVE();

    returnLength = 0;
    thread = NULL;
    mappedFileName = NULL;

    if (AccessMode != KernelMode)
    {
        __try
        {
            if (MemoryInformation)
            {
                ProbeOutputBytes(MemoryInformation, MemoryInformationLength);
            }

            if (ReturnLength)
            {
                ProbeOutputType(ReturnLength, ULONG);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }

    switch (MemoryInformationClass)
    {
        case KphMemoryImageSection:
        {
            SIZE_T length;
            OBJECT_ATTRIBUTES objectAttributes;
            IO_STATUS_BLOCK ioStatusBlock;
            HANDLE fileHandle;
            HANDLE sectionHandle;

            if (!MemoryInformation ||
                (MemoryInformationLength < sizeof(HANDLE)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                goto Exit;
            }

            //
            // At this time there is no known way to open the image section
            // for an address in a process without reopening the file.
            //
            // For image mappings the VAD does not retain a reference to the
            // actual section object but does contain a pointer to the file
            // object. There are a few exported APIs for creating sections
            // using a file object directly. And, using the file object in the
            // VAD, it is possible to map the data section but not the image
            // section. Internally, MiCreateSection has checks for SEC_IMAGE
            // when passing in a file object - this check will fail the
            // operation.
            //
            // Unfortunately, to create the image section, we have to query the
            // file name and open the file again. This introduces two points of
            // failure that could be avoided if we could create the image
            // section using the file object. The unfortunate failure mode is
            // the file being "gone" or otherwise inaccessible. The other cases
            // might be a resource constraint or a filter interfering with
            // access to the file.
            //

            mappedFileName = KphAllocatePaged(MAX_PATH, KPH_TAG_VM_QUERY);
            if (!mappedFileName)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Exit;
            }

            status = ZwQueryVirtualMemory(ProcessHandle,
                                          BaseAddress,
                                          MemoryMappedFilenameInformation,
                                          mappedFileName,
                                          MAX_PATH,
                                          &length);
            if ((status == STATUS_BUFFER_OVERFLOW) && (length > 0))
            {
                KphFree(mappedFileName, KPH_TAG_VM_QUERY);
                mappedFileName = KphAllocatePaged(length, KPH_TAG_VM_QUERY);
                if (!mappedFileName)
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Exit;
                }

                status = ZwQueryVirtualMemory(ProcessHandle,
                                              BaseAddress,
                                              MemoryMappedFilenameInformation,
                                              mappedFileName,
                                              length,
                                              &length);
            }

            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "ZwQueryVirtualMemory failed: %!STATUS!",
                              status);

                goto Exit;
            }

            InitializeObjectAttributes(&objectAttributes,
                                       mappedFileName,
                                       OBJ_KERNEL_HANDLE,
                                       NULL,
                                       NULL);

            status = KphCreateFile(&fileHandle,
                                   FILE_READ_ACCESS | SYNCHRONIZE,
                                   &objectAttributes,
                                   &ioStatusBlock,
                                   NULL,
                                   FILE_ATTRIBUTE_NORMAL,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   FILE_OPEN,
                                   FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                                   NULL,
                                   0,
                                   IO_IGNORE_SHARE_ACCESS_CHECK,
                                   KernelMode);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "KphCreateFile failed: %!STATUS!",
                              status);

                goto Exit;
            }

            InitializeObjectAttributes(&objectAttributes,
                                       NULL,
                                       (AccessMode ? 0 : OBJ_KERNEL_HANDLE),
                                       NULL,
                                       NULL);

            status = ZwCreateSection(&sectionHandle,
                                     SECTION_QUERY | SECTION_MAP_READ,
                                     &objectAttributes,
                                     NULL,
                                     PAGE_READONLY,
                                     SEC_IMAGE_NO_EXECUTE,
                                     fileHandle);

            ObCloseHandle(fileHandle, KernelMode);

            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "ZwCreateSection failed: %!STATUS!",
                              status);

                goto Exit;
            }

            if (AccessMode != KernelMode)
            {
                __try
                {
                    *(PHANDLE)MemoryInformation = sectionHandle;
                    returnLength = sizeof(HANDLE);
                    status = STATUS_SUCCESS;
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    ObCloseHandle(sectionHandle, UserMode);
                    status = GetExceptionCode();
                }
            }
            else
            {
                *(PHANDLE)MemoryInformation = sectionHandle;
                returnLength = sizeof(HANDLE);
                status = STATUS_SUCCESS;
            }

            break;
        }
        case KphMemoryDataSection:
        {
            SIZE_T length;
            KPH_VM_TLS_CREATE_DATA_SECTION tls;
            PKPH_MEMORY_DATA_SECTION memoryInformation;

            if (!MemoryInformation ||
                (MemoryInformationLength) < sizeof(KPH_MEMORY_DATA_SECTION))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                goto Exit;
            }

            //
            // The data section may be created using the file object in the VAD.
            // First we have to access the file object from the VAD. We could do
            // some dynamic data and walk the process VAD ourselves, but there
            // is a "cleaner" option. Querying for the memory mapped file name
            // will do the work to enumerate the VAD and will land us in our
            // mini-filter instance with the file object. This still comes with
            // possibility of filters interfering with the name query, but at
            // least we don't have to go through an entire IRP_MJ_CREATE. The
            // advantage here is we are able to create a section object for a
            // file that might be "gone" or otherwise inaccessible.
            //

            thread = KphGetCurrentThreadContext();
            if (!thread)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Exit;
            }

            RtlZeroMemory(&tls, sizeof(tls));

            tls.AccessMode = AccessMode;

            thread->VmTlsCreateDataSection = &tls;

            status = ZwQueryVirtualMemory(ProcessHandle,
                                          BaseAddress,
                                          MemoryMappedFilenameInformation,
                                          NULL,
                                          0,
                                          &length);

            if (thread->VmTlsCreateDataSection)
            {
                thread->VmTlsCreateDataSection = NULL;

                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "VmTlsCreateDataSection was not null! "
                              "ZwQueryVirtualMemory returned: %!STATUS!",
                              status);

                status = STATUS_UNEXPECTED_IO_ERROR;
                goto Exit;
            }

            status = tls.Status;
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "KPH_VM_TLS_CREATE_DATA_SECTION status: %!STATUS!",
                              status);

                goto Exit;
            }

            memoryInformation = MemoryInformation;

            if (AccessMode != KernelMode)
            {
                __try
                {
                    memoryInformation->SectionHandle = tls.SectionHandle;
                    memoryInformation->SectionFileSize = tls.SectionFileSize;
                    returnLength = sizeof(KPH_MEMORY_DATA_SECTION);
                    status = STATUS_SUCCESS;
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    ObCloseHandle(tls.SectionHandle, UserMode);
                    status = GetExceptionCode();
                }
            }
            else
            {
                memoryInformation->SectionHandle = tls.SectionHandle;
                memoryInformation->SectionFileSize = tls.SectionFileSize;
                returnLength = sizeof(KPH_MEMORY_DATA_SECTION);
                status = STATUS_SUCCESS;
            }

            break;
        }
        case KphMemoryMappedInformation:
        {
            SIZE_T length;
            KPH_MEMORY_MAPPED_INFORMATION tls;
            PKPH_MEMORY_MAPPED_INFORMATION memoryInformation;

            if (!MemoryInformation ||
                (MemoryInformationLength) < sizeof(KPH_MEMORY_MAPPED_INFORMATION))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                goto Exit;
            }

            thread = KphGetCurrentThreadContext();
            if (!thread)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Exit;
            }

            RtlZeroMemory(&tls, sizeof(tls));

            thread->VmTlsMappedInformation = &tls;

            status = ZwQueryVirtualMemory(ProcessHandle,
                                          BaseAddress,
                                          MemoryMappedFilenameInformation,
                                          NULL,
                                          0,
                                          &length);

            if (thread->VmTlsMappedInformation)
            {
                thread->VmTlsMappedInformation = NULL;

                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "VmTlsMappedInformation was not null! "
                              "ZwQueryVirtualMemory returned: %!STATUS!",
                              status);

                status = STATUS_UNEXPECTED_IO_ERROR;
                goto Exit;
            }

            memoryInformation = MemoryInformation;

            __try
            {
                memoryInformation->FileObject = tls.FileObject;
                memoryInformation->SectionObjectPointers = tls.SectionObjectPointers;
                memoryInformation->DataControlArea = tls.DataControlArea;
                memoryInformation->SharedCacheMap = tls.SharedCacheMap;
                memoryInformation->ImageControlArea = tls.ImageControlArea;
                memoryInformation->UserWritableReferences = tls.UserWritableReferences;
                returnLength = sizeof(KPH_MEMORY_MAPPED_INFORMATION);
                status = STATUS_SUCCESS;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
            }

            break;
        }
        default:
        {
            status = STATUS_INVALID_INFO_CLASS;
            break;
        }
    }

Exit:

    if (ReturnLength)
    {
        if (AccessMode != KernelMode)
        {
            __try
            {
                *ReturnLength = returnLength;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                NOTHING;
            }
        }
        else
        {
            *ReturnLength = returnLength;
        }
    }

    if (mappedFileName)
    {
        KphFree(mappedFileName, KPH_TAG_VM_QUERY);
    }

    if (thread)
    {
        KphDereferenceObject(thread);
    }

    return status;
}
