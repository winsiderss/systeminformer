/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2024
 *
 */

#include <ph.h>
#include <apiimport.h>

/**
 * Creates an anonymous pipe.
 *
 * \param PipeReadHandle The pipe read handle.
 * \param PipeWriteHandle The pipe write handle.
 */
NTSTATUS PhCreatePipe(
    _Out_ PHANDLE PipeReadHandle,
    _Out_ PHANDLE PipeWriteHandle
    )
{
    return PhCreatePipeEx(PipeReadHandle, PipeWriteHandle, NULL, NULL);
}

/**
 * Creates an anonymous pipe.
 *
 * \param[out] PipeReadHandle The pipe read handle.
 * \param[out] PipeWriteHandle The pipe write handle.
 * \param[in] PipeReadAttributes Optional pipe read attributes.
 * \param[in] PipeWriteAttributes Optional pipe write attributes.
 */
NTSTATUS PhCreatePipeEx(
    _Out_ PHANDLE PipeReadHandle,
    _Out_ PHANDLE PipeWriteHandle,
    _In_opt_ PSECURITY_ATTRIBUTES PipeReadAttributes,
    _In_opt_ PSECURITY_ATTRIBUTES PipeWriteAttributes
    )
{
    NTSTATUS status;
    PACL pipeAcl = NULL;
    HANDLE pipeDirectoryHandle;
    HANDLE pipeReadHandle;
    HANDLE pipeWriteHandle;
    UNICODE_STRING pipeName;
    SECURITY_DESCRIPTOR securityDescriptor;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK isb;
    LARGE_INTEGER timeout;
    SECURITY_QUALITY_OF_SERVICE pipeSecurityQos =
    {
        sizeof(SECURITY_QUALITY_OF_SERVICE),
        SecurityAnonymous,
        SECURITY_STATIC_TRACKING,
        FALSE
    };

    RtlInitUnicodeString(&pipeName, DEVICE_NAMED_PIPE);
    InitializeObjectAttributes(
        &objectAttributes,
        &pipeName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenFile(
        &pipeDirectoryHandle,
        GENERIC_READ | SYNCHRONIZE,
        &objectAttributes,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return status;

    RtlInitEmptyUnicodeString(&pipeName, NULL, 0);
    InitializeObjectAttributesEx(
        &objectAttributes,
        &pipeName,
        OBJ_CASE_INSENSITIVE,
        pipeDirectoryHandle,
        NULL,
        &pipeSecurityQos
        );

    if (PipeReadAttributes)
    {
        if (PipeReadAttributes->bInheritHandle)
        {
            SetFlag(objectAttributes.Attributes, OBJ_INHERIT);
        }

        if (PipeReadAttributes->lpSecurityDescriptor)
        {
            objectAttributes.SecurityDescriptor = PipeReadAttributes->lpSecurityDescriptor;
        }
    }

    if (!objectAttributes.SecurityDescriptor)
    {
        status = PhDefaultNpAcl(&pipeAcl);

        if (NT_SUCCESS(status))
        {
            status = PhCreateSecurityDescriptor(&securityDescriptor, SECURITY_DESCRIPTOR_REVISION);

            if (NT_SUCCESS(status))
            {
                status = PhSetDaclSecurityDescriptor(&securityDescriptor, TRUE, pipeAcl, FALSE);

                if (NT_SUCCESS(status))
                {
                    objectAttributes.SecurityDescriptor = &securityDescriptor;
                }

                assert(RtlValidSecurityDescriptor(&securityDescriptor));
            }
        }

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    status = NtCreateNamedPipeFile(
        &pipeReadHandle,
        FILE_WRITE_ATTRIBUTES | GENERIC_READ | SYNCHRONIZE,
        &objectAttributes,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_CREATE,
        FILE_PIPE_INBOUND | FILE_SYNCHRONOUS_IO_NONALERT,
        FILE_PIPE_BYTE_STREAM_TYPE | FILE_PIPE_REJECT_REMOTE_CLIENTS,
        FILE_PIPE_BYTE_STREAM_MODE,
        FILE_PIPE_QUEUE_OPERATION,
        1,
        PAGE_SIZE,
        PAGE_SIZE,
        PhTimeoutFromMilliseconds(&timeout, 120000)
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    RtlInitEmptyUnicodeString(&pipeName, NULL, 0);
    InitializeObjectAttributesEx(
        &objectAttributes,
        &pipeName,
        OBJ_CASE_INSENSITIVE,
        pipeReadHandle,
        NULL,
        &pipeSecurityQos
        );

    if (PipeWriteAttributes)
    {
        if (PipeWriteAttributes->bInheritHandle)
        {
            SetFlag(objectAttributes.Attributes, OBJ_INHERIT);
        }

        if (PipeWriteAttributes->lpSecurityDescriptor)
        {
            objectAttributes.SecurityDescriptor = PipeWriteAttributes->lpSecurityDescriptor;
        }
    }

    status = NtOpenFile(
        &pipeWriteHandle,
        FILE_READ_ATTRIBUTES | GENERIC_WRITE | SYNCHRONIZE,
        &objectAttributes,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (NT_SUCCESS(status))
    {
        *PipeReadHandle = pipeReadHandle;
        *PipeWriteHandle = pipeWriteHandle;
    }

CleanupExit:
    if (pipeAcl)
    {
        PhFree(pipeAcl);
    }

    NtClose(pipeDirectoryHandle);
    return status;
}

/**
 * Creates an named pipe.
 *
 * \param PipeHandle The pipe read/write handle.
 * \param PipeName The pipe name.
 */
NTSTATUS PhCreateNamedPipe(
    _Out_ PHANDLE PipeHandle,
    _In_ PCPH_STRINGREF PipeName
    )
{
    static CONST PH_STRINGREF deviceName = PH_STRINGREF_INIT(DEVICE_NAMED_PIPE);
    NTSTATUS status;
    PACL pipeAcl = NULL;
    HANDLE pipeHandle;
    PPH_STRING pipeName;
    UNICODE_STRING pipeNameUs;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK isb;
    LARGE_INTEGER timeout;
    SECURITY_DESCRIPTOR securityDescriptor;
    SECURITY_QUALITY_OF_SERVICE pipeSecurityQos =
    {
        sizeof(SECURITY_QUALITY_OF_SERVICE),
        SecurityAnonymous,
        SECURITY_STATIC_TRACKING,
        FALSE
    };

    pipeName = PhConcatStringRef2(&deviceName, PipeName);

    if (!PhStringRefToUnicodeString(&pipeName->sr, &pipeNameUs))
    {
        PhDereferenceObject(pipeName);
        return STATUS_NAME_TOO_LONG;
    }

    InitializeObjectAttributesEx(
        &objectAttributes,
        &pipeNameUs,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL,
        &pipeSecurityQos
        );

    if (NT_SUCCESS(PhDefaultNpAcl(&pipeAcl)))
    {
        status = PhCreateSecurityDescriptor(&securityDescriptor, SECURITY_DESCRIPTOR_REVISION);

        if (NT_SUCCESS(status))
        {
            status = PhSetDaclSecurityDescriptor(&securityDescriptor, TRUE, pipeAcl, FALSE);

            if (NT_SUCCESS(status))
            {
                objectAttributes.SecurityDescriptor = &securityDescriptor;
            }
        }
    }

    status = NtCreateNamedPipeFile(
        &pipeHandle,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE | SYNCHRONIZE,
        &objectAttributes,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        FILE_PIPE_FULL_DUPLEX | FILE_SYNCHRONOUS_IO_NONALERT,
        FILE_PIPE_MESSAGE_TYPE | FILE_PIPE_REJECT_REMOTE_CLIENTS,
        FILE_PIPE_MESSAGE_MODE,
        FILE_PIPE_QUEUE_OPERATION,
        FILE_PIPE_UNLIMITED_INSTANCES,
        PAGE_SIZE,
        PAGE_SIZE,
        PhTimeoutFromMilliseconds(&timeout, 1000)
        );

    if (NT_SUCCESS(status))
    {
        *PipeHandle = pipeHandle;
    }

    if (pipeAcl)
    {
        PhFree(pipeAcl);
    }

    PhDereferenceObject(pipeName);
    return status;
}

/**
 * Connects to a named pipe.
 *
 * \param[out] PipeHandle The pipe read/write handle.
 * \param[in] PipeName The pipe name.
 */
NTSTATUS PhConnectPipe(
    _Out_ PHANDLE PipeHandle,
    _In_ PCPH_STRINGREF PipeName
    )
{
    static CONST PH_STRINGREF deviceName = PH_STRINGREF_INIT(DEVICE_NAMED_PIPE);
    NTSTATUS status;
    HANDLE pipeHandle;
    PPH_STRING pipeName;
    UNICODE_STRING pipeNameUs;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK isb;
    SECURITY_QUALITY_OF_SERVICE pipeSecurityQos =
    {
        sizeof(SECURITY_QUALITY_OF_SERVICE),
        SecurityAnonymous,
        SECURITY_STATIC_TRACKING,
        FALSE
    };

    pipeName = PhConcatStringRef2(&deviceName, PipeName);

    if (!PhStringRefToUnicodeString(&pipeName->sr, &pipeNameUs))
    {
        PhDereferenceObject(pipeName);
        return STATUS_NAME_TOO_LONG;
    }

    InitializeObjectAttributesEx(
        &objectAttributes,
        &pipeNameUs,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL,
        &pipeSecurityQos
        );

    status = NtCreateFile(
        &pipeHandle,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE | SYNCHRONIZE,
        &objectAttributes,
        &isb,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
        );

    if (NT_SUCCESS(status))
    {
        *PipeHandle = pipeHandle;
    }

    PhDereferenceObject(pipeName);
    return status;
}

/**
 * Listens for a connection from a client on a named pipe.
 *
 * \param[in] PipeHandle The pipe handle.
 */
NTSTATUS PhListenNamedPipe(
    _In_ HANDLE PipeHandle
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;

    status = NtFsControlFile(
        PipeHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_PIPE_LISTEN,
        NULL,
        0,
        NULL,
        0
        );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(PipeHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
            status = isb.Status;
    }

    return status;
}

/**
 * Disconnects a client from a named pipe.
 *
 * \param[in] PipeHandle The pipe handle.
 */
NTSTATUS PhDisconnectNamedPipe(
    _In_ HANDLE PipeHandle
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;

    status = NtFsControlFile(
        PipeHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_PIPE_DISCONNECT,
        NULL,
        0,
        NULL,
        0
        );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(PipeHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
            status = isb.Status;
    }

    return status;
}

/**
 * Peeks at data from a named pipe without removing it.
 *
 * \param[in] PipeHandle The pipe handle.
 * \param[out] Buffer Optional buffer to receive the data.
 * \param[in] Length The length of the buffer.
 * \param[out] NumberOfBytesRead Optional pointer to receive the number of bytes read.
 * \param[out] NumberOfBytesAvailable Optional pointer to receive the number of bytes available.
 * \param[out] NumberOfBytesLeftInMessage Optional pointer to receive the number of bytes left in the message.
 */
NTSTATUS PhPeekNamedPipe(
    _In_ HANDLE PipeHandle,
    _Out_writes_bytes_opt_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_opt_ PULONG NumberOfBytesRead,
    _Out_opt_ PULONG NumberOfBytesAvailable,
    _Out_opt_ PULONG NumberOfBytesLeftInMessage
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    PFILE_PIPE_PEEK_BUFFER peekBuffer;
    ULONG peekBufferLength;

    peekBufferLength = FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data) + Length;
    peekBuffer = PhAllocate(peekBufferLength);

    status = NtFsControlFile(
        PipeHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_PIPE_PEEK,
        NULL,
        0,
        peekBuffer,
        peekBufferLength
        );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(PipeHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
            status = isb.Status;
    }

    // STATUS_BUFFER_OVERFLOW means that there is data remaining; this is normal.
    if (status == STATUS_BUFFER_OVERFLOW)
        status = STATUS_SUCCESS;

    if (NT_SUCCESS(status))
    {
        ULONG numberOfBytesRead = 0;

        if (Buffer || NumberOfBytesRead || NumberOfBytesLeftInMessage)
            numberOfBytesRead = (ULONG)(isb.Information - FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data));

        if (Buffer)
            memcpy(Buffer, peekBuffer->Data, numberOfBytesRead);

        if (NumberOfBytesRead)
            *NumberOfBytesRead = numberOfBytesRead;

        if (NumberOfBytesAvailable)
            *NumberOfBytesAvailable = peekBuffer->ReadDataAvailable;

        if (NumberOfBytesLeftInMessage)
            *NumberOfBytesLeftInMessage = peekBuffer->MessageLength - numberOfBytesRead;
    }

    PhFree(peekBuffer);

    return status;
}

/**
 * Connects to a named pipe, writes a message, reads a response, and disconnects.
 *
 * \param[in] PipeName The pipe name.
 * \param[in] InputBuffer The input buffer.
 * \param[in] InputBufferLength The length of the input buffer.
 * \param[out] OutputBuffer The output buffer.
 * \param[in] OutputBufferLength The length of the output buffer.
 */
NTSTATUS PhCallNamedPipe(
    _In_ PCWSTR PipeName,
    _In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength
    )
{
    NTSTATUS status;
    HANDLE pipeHandle = NULL;

    status = PhConnectPipeZ(&pipeHandle, PipeName);

    if (!NT_SUCCESS(status))
    {
        PhWaitForNamedPipe(PipeName, 1000);

        status = PhConnectPipeZ(&pipeHandle, PipeName);
    }

    if (NT_SUCCESS(status))
    {
        FILE_PIPE_INFORMATION pipeInfo;
        IO_STATUS_BLOCK isb;

        memset(&pipeInfo, 0, sizeof(FILE_PIPE_INFORMATION));
        pipeInfo.CompletionMode = FILE_PIPE_QUEUE_OPERATION;
        pipeInfo.ReadMode = FILE_PIPE_MESSAGE_MODE;

        status = NtSetInformationFile(
            pipeHandle,
            &isb,
            &pipeInfo,
            sizeof(FILE_PIPE_INFORMATION),
            FilePipeInformation
            );
    }

    if (NT_SUCCESS(status))
    {
        status = PhTransceiveNamedPipe(
            pipeHandle,
            InputBuffer,
            InputBufferLength,
            OutputBuffer,
            OutputBufferLength
            );
    }

    if (pipeHandle)
    {
        IO_STATUS_BLOCK ioStatusBlock;

        NtFlushBuffersFile(pipeHandle, &ioStatusBlock);

        PhDisconnectNamedPipe(pipeHandle);

        NtClose(pipeHandle);
    }

    return status;
}

/**
 * Performs a transceive operation on a named pipe.
 *
 * \param[in] PipeHandle The pipe handle.
 * \param[in] InputBuffer The input buffer.
 * \param[in] InputBufferLength The length of the input buffer.
 * \param[out] OutputBuffer The output buffer.
 * \param[in] OutputBufferLength The length of the output buffer.
 */
NTSTATUS PhTransceiveNamedPipe(
    _In_ HANDLE PipeHandle,
    _In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;

    status = NtFsControlFile(
        PipeHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_PIPE_TRANSCEIVE,
        InputBuffer,
        InputBufferLength,
        OutputBuffer,
        OutputBufferLength
        );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(PipeHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
            status = isb.Status;
    }

    return status;
}

/**
 * Waits for a named pipe to become available.
 *
 * \param[in] PipeName The pipe name.
 * \param[in] Timeout Optional timeout in milliseconds.
 */
NTSTATUS PhWaitForNamedPipe(
    _In_ PCWSTR PipeName,
    _In_opt_ ULONG Timeout
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    PH_STRINGREF pipeName;
    UNICODE_STRING objectName;
    HANDLE fileSystemHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    PFILE_PIPE_WAIT_FOR_BUFFER waitForBuffer;
    ULONG waitForBufferLength;

    RtlInitUnicodeString(&objectName, DEVICE_NAMED_PIPE);
    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenFile(
        &fileSystemHandle,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        &objectAttributes,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return status;

    PhInitializeStringRefLongHint(&pipeName, PipeName);
    waitForBufferLength = FIELD_OFFSET(FILE_PIPE_WAIT_FOR_BUFFER, Name) + (ULONG)pipeName.Length;
    waitForBuffer = PhAllocate(waitForBufferLength);

    if (Timeout)
    {
        PhTimeoutFromMilliseconds(&waitForBuffer->Timeout, Timeout);
        waitForBuffer->TimeoutSpecified = TRUE;
    }
    else
    {
        waitForBuffer->Timeout.LowPart = 0;
        waitForBuffer->Timeout.HighPart = MINLONG; // a very long time
        waitForBuffer->TimeoutSpecified = TRUE;
    }

    waitForBuffer->NameLength = (ULONG)pipeName.Length;
    memcpy(waitForBuffer->Name, pipeName.Buffer, pipeName.Length);

    status = NtFsControlFile(
        fileSystemHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_PIPE_WAIT,
        waitForBuffer,
        waitForBufferLength,
        NULL,
        0
        );

    PhFree(waitForBuffer);
    NtClose(fileSystemHandle);

    return status;
}

/**
 * Impersonates the client of a named pipe.
 *
 * \param[in] PipeHandle The pipe handle.
 */
NTSTATUS PhImpersonateClientOfNamedPipe(
    _In_ HANDLE PipeHandle
    )
{
    IO_STATUS_BLOCK isb;

    return NtFsControlFile(
        PipeHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_PIPE_IMPERSONATE,
        NULL,
        0,
        NULL,
        0
        );
}

/**
 * Disables impersonation for a named pipe.
 *
 * \param[in] PipeHandle The pipe handle.
 */
NTSTATUS PhDisableImpersonateNamedPipe(
    _In_ HANDLE PipeHandle
    )
{
    IO_STATUS_BLOCK isb;

    return NtFsControlFile(
        PipeHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_PIPE_DISABLE_IMPERSONATE,
        NULL,
        0,
        NULL,
        0
        );
}

/**
 * Gets the computer name of the client of a named pipe.
 *
 * \param[in] PipeHandle The pipe handle.
 * \param[in] ClientComputerNameLength The length of the client computer name buffer.
 * \param[out] ClientComputerName The client computer name buffer.
 */
NTSTATUS PhGetNamedPipeClientComputerName(
    _In_ HANDLE PipeHandle,
    _In_ ULONG ClientComputerNameLength,
    _Out_ PVOID ClientComputerName
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;

    status = NtFsControlFile(
        PipeHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE,
        "ClientComputerName",
        sizeof("ClientComputerName"),
        ClientComputerName,
        ClientComputerNameLength
        );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(PipeHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
            status = isb.Status;
    }

    return status;
}

/**
 * Gets the process ID of the client of a named pipe.
 *
 * \param[in] PipeHandle The pipe handle.
 * \param[out] ClientProcessId The client process ID.
 */
NTSTATUS PhGetNamedPipeClientProcessId(
    _In_ HANDLE PipeHandle,
    _Out_ PHANDLE ClientProcessId
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    ULONG processId = 0;

    status = NtFsControlFile(
        PipeHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE,
        "ClientProcessId",
        sizeof("ClientProcessId"),
        &processId,
        sizeof(ULONG)
        );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(PipeHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
            status = isb.Status;
    }

    if (NT_SUCCESS(status))
    {
        if (ClientProcessId)
        {
            *ClientProcessId = UlongToHandle(processId);
        }
    }

    return status;
}

/**
 * Gets the session ID of the client of a named pipe.
 *
 * \param[in] PipeHandle The pipe handle.
 * \param[out] ClientSessionId The client session ID.
 */
NTSTATUS PhGetNamedPipeClientSessionId(
    _In_ HANDLE PipeHandle,
    _Out_ PHANDLE ClientSessionId
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    ULONG processId = 0;

    status = NtFsControlFile(
        PipeHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE,
        "ClientSessionId",
        sizeof("ClientSessionId"),
        &processId,
        sizeof(ULONG)
        );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(PipeHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
            status = isb.Status;
    }

    if (NT_SUCCESS(status))
    {
        if (ClientSessionId)
        {
            *ClientSessionId = UlongToHandle(processId);
        }
    }

    return status;
}

/**
 * Gets the process ID of the server of a named pipe.
 *
 * \param[in] PipeHandle The pipe handle.
 * \param[out] ServerProcessId The server process ID.
 */
NTSTATUS PhGetNamedPipeServerProcessId(
    _In_ HANDLE PipeHandle,
    _Out_ PHANDLE ServerProcessId
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    ULONG processId = 0;

    status = NtFsControlFile(
        PipeHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_PIPE_GET_PIPE_ATTRIBUTE,
        "ServerProcessId",
        sizeof("ServerProcessId"),
        &processId,
        sizeof(ULONG)
        );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(PipeHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
            status = isb.Status;
    }

    if (NT_SUCCESS(status))
    {
        if (ServerProcessId)
        {
            *ServerProcessId = UlongToHandle(processId);
        }
    }

    return status;
}

/**
 * Gets the session ID of the server of a named pipe.
 *
 * \param[in] PipeHandle The pipe handle.
 * \param[out] ServerSessionId The server session ID.
 */
NTSTATUS PhGetNamedPipeServerSessionId(
    _In_ HANDLE PipeHandle,
    _Out_ PHANDLE ServerSessionId
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    ULONG processId = 0;

    status = NtFsControlFile(
        PipeHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_PIPE_GET_PIPE_ATTRIBUTE,
        "ServerSessionId",
        sizeof("ServerSessionId"),
        &processId,
        sizeof(ULONG)
        );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(PipeHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
            status = isb.Status;
    }

    if (NT_SUCCESS(status))
    {
        if (ServerSessionId)
        {
            *ServerSessionId = UlongToHandle(processId);
        }
    }

    return status;
}

/**
 * Enumerates named pipes in the named pipe directory.
 *
 * \param[in] SearchPattern Optional search pattern.
 * \param[in] Callback The callback function.
 * \param[in] Context Optional context.
 */
NTSTATUS PhEnumDirectoryNamedPipe(
    _In_opt_ PCPH_STRINGREF SearchPattern,
    _In_ PPH_ENUM_DIRECTORY_FILE Callback,
    _In_opt_ PVOID Context
    )
{
    static CONST PH_STRINGREF objectName = PH_STRINGREF_INIT(DEVICE_NAMED_PIPE);
    HANDLE objectHandle;
    NTSTATUS status;

    status = PhOpenFile(
        &objectHandle,
        &objectName,
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        NULL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        status = PhEnumDirectoryFile(
            objectHandle,
            SearchPattern,
            Callback,
            Context
            );

        NtClose(objectHandle);
    }

    return status;
}

/**
 * Gets the default ACL for a named pipe.
 *
 * \param[out] DefaultNpAc The default ACL.
 */
 // rev from RtlDefaultNpAcl
NTSTATUS PhDefaultNpAcl(
    _Out_ PACL* DefaultNpAc
    )
{
    NTSTATUS status;
    PACL pipeAcl = NULL;
    PH_TOKEN_OWNER tokenQuery;

    status = PhGetTokenOwner(
        NtCurrentThreadEffectiveToken(),
        &tokenQuery
        );

    if (NT_SUCCESS(status))
    {
        APPCONTAINER_SID_TYPE appContainerSidType = InvalidAppContainerSidType;
        PH_TOKEN_APPCONTAINER tokenAppContainer = { 0 };
        PSID appContainerSidParent = NULL;
        ULONG defaultAclSize;

        if (NT_SUCCESS(PhGetTokenAppContainerSid(NtCurrentThreadEffectiveToken(), &tokenAppContainer)))
        {
            if (RtlGetAppContainerSidType_Import())
                RtlGetAppContainerSidType_Import()(tokenAppContainer.AppContainer.Sid, &appContainerSidType);

            if (appContainerSidType == ChildAppContainerSidType)
            {
                if (RtlGetAppContainerParent_Import())
                    RtlGetAppContainerParent_Import()(tokenAppContainer.AppContainer.Sid, &appContainerSidParent);
            }
        }

        if (!NT_SUCCESS(status = RtlULongAdd(SECURITY_DESCRIPTOR_MIN_LENGTH, sizeof(ACL), &defaultAclSize)))
            goto CleanupExit;
        if (!NT_SUCCESS(status = RtlULongAdd(defaultAclSize, sizeof(ACCESS_ALLOWED_ACE) + PhLengthSid(&PhSeLocalSystemSid), &defaultAclSize)))
            goto CleanupExit;
        if (!NT_SUCCESS(status = RtlULongAdd(defaultAclSize, sizeof(ACCESS_ALLOWED_ACE) + PhLengthSid(PhSeAdministratorsSid()), &defaultAclSize)))
            goto CleanupExit;
        if (!NT_SUCCESS(status = RtlULongAdd(defaultAclSize, sizeof(ACCESS_ALLOWED_ACE) + PhLengthSid(tokenQuery.TokenOwner.Owner), &defaultAclSize)))
            goto CleanupExit;
        if (!NT_SUCCESS(status = RtlULongAdd(defaultAclSize, sizeof(ACCESS_ALLOWED_ACE) + PhLengthSid(&PhSeEveryoneSid), &defaultAclSize)))
            goto CleanupExit;
        if (!NT_SUCCESS(status = RtlULongAdd(defaultAclSize, sizeof(ACCESS_ALLOWED_ACE) + PhLengthSid(&PhSeAnonymousLogonSid), &defaultAclSize)))
            goto CleanupExit;

        if (tokenAppContainer.AppContainer.Sid)
        {
            if (!NT_SUCCESS(status = RtlULongAdd(defaultAclSize, sizeof(ACCESS_ALLOWED_ACE) + PhLengthSid(tokenAppContainer.AppContainer.Sid), &defaultAclSize)))
                goto CleanupExit;
        }

        if (appContainerSidParent)
        {
            if (!NT_SUCCESS(status = RtlULongAdd(defaultAclSize, sizeof(ACCESS_ALLOWED_ACE) + PhLengthSid(appContainerSidParent), &defaultAclSize)))
                goto CleanupExit;
        }

        if (!(pipeAcl = PhAllocateZeroSafe(defaultAclSize)))
        {
            status = STATUS_NO_MEMORY;
            goto CleanupExit;
        }

        if (!NT_SUCCESS(status = PhCreateAcl(pipeAcl, defaultAclSize, ACL_REVISION)))
            goto CleanupExit;
        if (!NT_SUCCESS(status = PhAddAccessAllowedAce(pipeAcl, ACL_REVISION, GENERIC_ALL, &PhSeLocalSystemSid)))
            goto CleanupExit;
        if (!NT_SUCCESS(status = PhAddAccessAllowedAce(pipeAcl, ACL_REVISION, GENERIC_ALL, PhSeAdministratorsSid())))
            goto CleanupExit;

        if (tokenAppContainer.AppContainer.Sid)
        {
            if (!NT_SUCCESS(status = PhAddAccessAllowedAce(pipeAcl, ACL_REVISION, GENERIC_ALL, tokenAppContainer.AppContainer.Sid)))
                goto CleanupExit;
        }

        if (appContainerSidParent)
        {
            if (!NT_SUCCESS(status = PhAddAccessAllowedAce(pipeAcl, ACL_REVISION, GENERIC_ALL, appContainerSidParent)))
                goto CleanupExit;
        }

        if (!NT_SUCCESS(status = PhAddAccessAllowedAce(pipeAcl, ACL_REVISION, GENERIC_ALL, tokenQuery.TokenOwner.Owner)))
            goto CleanupExit;
        if (!NT_SUCCESS(status = PhAddAccessAllowedAce(pipeAcl, ACL_REVISION, GENERIC_READ, &PhSeEveryoneSid)))
            goto CleanupExit;
        if (!NT_SUCCESS(status = PhAddAccessAllowedAce(pipeAcl, ACL_REVISION, GENERIC_READ, &PhSeAnonymousLogonSid)))
            goto CleanupExit;

        *DefaultNpAc = pipeAcl;

    CleanupExit:
        if (!NT_SUCCESS(status) && pipeAcl)
            PhFree(pipeAcl);
        if (appContainerSidParent)
            RtlFreeSid(appContainerSidParent);
    }

    return status;
}
