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
    status = ZwQueryInformationFile(FileHandle,
                                    &ioStatusBlock,
                                    buffer,
                                    FileInformationLength,
                                    FileInformationClass);
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
    status = ZwQueryVolumeInformationFile(FileHandle,
                                          &ioStatusBlock,
                                          buffer,
                                          FsInformationLength,
                                          FsInformationClass);
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
