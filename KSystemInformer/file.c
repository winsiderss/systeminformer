/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *
 */

#include <kph.h>
#include <dyndata.h>

#include <trace.h>

PAGED_FILE();

/**
 * \brief Checks if a file handle is safe to issue a query through.
 *
 * \details Expects to be stack attached to the process that owns the handle.
 *
 * \param[in] FileHandle The file handle to check.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpCheckFileHandleForQuery(
    _In_ HANDLE FileHandle
    )
{
    NTSTATUS status;
    PFILE_OBJECT fileObject;

    PAGED_PASSIVE();

    //
    // We are stack attached and "invading" the process to perform the query.
    // If the file object is "Busy" it means it's a synchronous file object
    // that is currently servicing an I/O request. If we were to issue another
    // request the call would block until the other completes. Since we don't
    // control this handle it's possible the call will never complete.
    //
    // This check isn't perfect, there is still a race, but it's narrower and
    // makes it safer and faster for the client. As this stands it's a pretty
    // gross hack, but we get value out of these APIs, so the hack is better
    // than nothing.
    //
    // TODO(jxy-s) investigate other options to close or eliminate the race
    //

    status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       *IoFileObjectType,
                                       KernelMode,
                                       &fileObject,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "ObReferenceObjectByHandle failed %!STATUS!",
                      status);

        return status;
    }

    status = fileObject->Busy ? STATUS_POSSIBLE_DEADLOCK : STATUS_SUCCESS;

    ObDereferenceObject(fileObject);

    return status;
}

/**
 * \brief Queries file information.
 *
 * \param[in] ProcessHandle A handle to a process.
 * \param[in] FileHandle A file handle which is present in the process.
 * \param[in] FileInformationClass The type of information to retrieve.
 * \param[out] FileInformation The buffer to store the information.
 * \param[in] FileInformationLength Length of the information buffer.
 * \param[in] IoStatusBlock Receives completion status and information.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryInformationFile(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE FileHandle,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _Out_writes_bytes_(FileInformationLength) PVOID FileInformation,
    _In_ ULONG FileInformationLength,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;
    KAPC_STATE apcState;
    IO_STATUS_BLOCK ioStatusBlock;
    PVOID buffer;
    BYTE stackBuffer[64];

    PAGED_PASSIVE();

    process = NULL;
    buffer = NULL;
    RtlZeroMemory(&ioStatusBlock, sizeof(ioStatusBlock));

    if (!FileInformation || !IoStatusBlock)
    {
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(FileInformation, FileInformationLength, 1);
            ProbeOutputType(IoStatusBlock, IO_STATUS_BLOCK);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }

    status = ObReferenceObjectByHandle(ProcessHandle,
                                       0,
                                       *PsProcessType,
                                       AccessMode,
                                       &process,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "ObReferenceObjectByHandle failed %!STATUS!",
                      status);

        goto Exit;
    }

    if (process == PsInitialSystemProcess)
    {
        FileHandle = MakeKernelHandle(FileHandle);
        AccessMode = KernelMode;
    }
    else
    {
        if (IsKernelHandle(FileHandle))
        {
            status = STATUS_INVALID_HANDLE;
            goto Exit;
        }
    }

    if (FileInformationLength <= ARRAYSIZE(stackBuffer))
    {
        buffer = stackBuffer;
    }
    else
    {
        buffer = KphAllocatePaged(FileInformationLength, KPH_TAG_FILE_QUERY);
        if (!buffer)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }
    }

    KeStackAttachProcess(process, &apcState);
    status = KphpCheckFileHandleForQuery(FileHandle);
    if (NT_SUCCESS(status))
    {
        status = ZwQueryInformationFile(FileHandle,
                                        &ioStatusBlock,
                                        buffer,
                                        FileInformationLength,
                                        FileInformationClass);
    }
    KeUnstackDetachProcess(&apcState);
    if (NT_SUCCESS(status))
    {
        __try
        {
            RtlCopyMemory(FileInformation, buffer, FileInformationLength);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
        }
    }

    __try
    {
        RtlCopyMemory(IoStatusBlock, &ioStatusBlock, sizeof(ioStatusBlock));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        NOTHING;
    }

Exit:

    if (buffer && (buffer != stackBuffer))
    {
        KphFree(buffer, KPH_TAG_FILE_QUERY);
    }

    if (process)
    {
        ObDereferenceObject(process);
    }

    return status;
}

/**
 * \brief Queries file volume information.
 *
 * \param[in] ProcessHandle A handle to a process.
 * \param[in] FileHandle A file handle which is present in the process.
 * \param[in] FsInformationClass The type of information to retrieve.
 * \param[out] FsInformation The buffer to store the information.
 * \param[in] FsInformationLength Length of the information buffer.
 * \param[in] IoStatusBlock Receives completion status and information.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryVolumeInformationFile(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE FileHandle,
    _In_ FS_INFORMATION_CLASS FsInformationClass,
    _Out_writes_bytes_(FsInformationLength) PVOID FsInformation,
    _In_ ULONG FsInformationLength,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;
    KAPC_STATE apcState;
    IO_STATUS_BLOCK ioStatusBlock;
    PVOID buffer;
    BYTE stackBuffer[64];

    PAGED_PASSIVE();

    process = NULL;
    buffer = NULL;
    RtlZeroMemory(&ioStatusBlock, sizeof(ioStatusBlock));

    if (!FsInformation || !IoStatusBlock)
    {
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(FsInformation, FsInformationLength, 1);
            ProbeOutputType(IoStatusBlock, IO_STATUS_BLOCK);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }

    status = ObReferenceObjectByHandle(ProcessHandle,
                                       0,
                                       *PsProcessType,
                                       AccessMode,
                                       &process,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "ObReferenceObjectByHandle failed %!STATUS!",
                      status);

        goto Exit;
    }

    if (process == PsInitialSystemProcess)
    {
        FileHandle = MakeKernelHandle(FileHandle);
        AccessMode = KernelMode;
    }
    else
    {
        if (IsKernelHandle(FileHandle))
        {
            status = STATUS_INVALID_HANDLE;
            goto Exit;
        }
    }

    if (FsInformationLength <= ARRAYSIZE(stackBuffer))
    {
        buffer = stackBuffer;
    }
    else
    {
        buffer = KphAllocatePaged(FsInformationLength, KPH_TAG_VOL_FILE_QUERY);
        if (!buffer)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }
    }

    KeStackAttachProcess(process, &apcState);
    status = KphpCheckFileHandleForQuery(FileHandle);
    if (NT_SUCCESS(status))
    {
        status = ZwQueryVolumeInformationFile(FileHandle,
                                              &ioStatusBlock,
                                              buffer,
                                              FsInformationLength,
                                              FsInformationClass);
    }
    KeUnstackDetachProcess(&apcState);
    if (NT_SUCCESS(status))
    {
        __try
        {
            RtlCopyMemory(FsInformation, buffer, FsInformationLength);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
        }
    }

    __try
    {
        RtlCopyMemory(IoStatusBlock, &ioStatusBlock, sizeof(ioStatusBlock));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        NOTHING;
    }

Exit:

    if (buffer && (buffer != stackBuffer))
    {
        KphFree(buffer, KPH_TAG_VOL_FILE_QUERY);
    }

    if (process)
    {
        ObDereferenceObject(process);
    }

    return status;
}

/**
 * \brief Creates a file.
 *
 * \param[out] FileHandle Populated with file handle on success.
 * \param[in] DesiredAccess The desired access to the file.
 * \param[in] ObjectAttributes Object attributes for the create.
 * \param[out] IoStatusBlock Receives final completion status and information.
 * \param[in] AllocationSize Optional allocation size, in bytes, for the file.
 * \param[in] FileAttributes Explicitly specified attributes are applied only
 * when the file is created, superseded, or, in some cases, overwritten.
 * \param[in] ShareAccess Specifies the type of share access to the file that
 * the caller would like.
 * \param[in] CreateDisposition Value that determines how the file should be
 * handled when the file already exists. 
 * \param[in] CreateOptions Specifies the options to be applied when creating
 * or opening the file.
 * \param[in] EaBuffer Optional pointer to an EA buffer.
 * \param[in] EaLength Length of EA buffer.
 * \param[in] Options Specifies options to be used during the generation of
 * the create request.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCreateFile(
    _Out_ PHANDLE FileHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
    _In_ ULONG EaLength,
    _In_ ULONG Options,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;

    PAGED_PASSIVE();

    if (AccessMode != KernelMode)
    {
        if (ExGetPreviousMode() == KernelMode)
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          GENERAL,
                          "Unexpected previous mode");

            NT_ASSERT(ExGetPreviousMode() != KernelMode);
            status = STATUS_INVALID_LEVEL;
            goto Exit;
        }

        if (Options & ~(IO_OPEN_TARGET_DIRECTORY |
                        IO_STOP_ON_SYMLINK |
                        IO_IGNORE_SHARE_ACCESS_CHECK))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          GENERAL,
                          "Invalid options 0x%lx",
                          Options);

            status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
    }
    else
    {
        Options |= (IO_NO_PARAMETER_CHECKING | IO_CHECK_CREATE_PARAMETERS);
    }

    status = IoCreateFile(FileHandle,
                          DesiredAccess,
                          ObjectAttributes,
                          IoStatusBlock,
                          AllocationSize,
                          FileAttributes,
                          ShareAccess,
                          CreateDisposition,
                          CreateOptions,
                          EaBuffer,
                          EaLength,
                          CreateFileTypeNone,
                          NULL,
                          Options);

Exit:

    return status;
}
