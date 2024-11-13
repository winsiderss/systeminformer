/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2024
 *
 */

#include <ph.h>

/**
 * \brief Wrapper which is essentially FilterpDeviceIoControl.
 *
 * \param[in] Handle Filter port handle.
 * \param[in] IoControlCode Filter I/O control code
 * \param[in] InBuffer Input buffer.
 * \param[in] InBufferSize Input buffer size.
 * \param[out] OutputBuffer Output Buffer.
 * \param[in] OutputBufferSize Output buffer size.
 * \param[out] BytesReturned Optionally set to the number of bytes returned.
 * \param[in,out] Overlapped Optional overlapped structure.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhpFilterDeviceIoControl(
    _In_ HANDLE Handle,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_(InBufferSize) PVOID InBuffer,
    _In_ ULONG InBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesReturned) PVOID OutputBuffer,
    _In_ ULONG OutputBufferSize,
    _Out_opt_ PULONG BytesReturned,
    _Inout_opt_ LPOVERLAPPED Overlapped
    )
{
    NTSTATUS status;

    if (BytesReturned)
    {
        *BytesReturned = 0;
    }

    if (Overlapped)
    {
        Overlapped->Internal = STATUS_PENDING;

        if (DEVICE_TYPE_FROM_CTL_CODE(IoControlCode) == FILE_DEVICE_FILE_SYSTEM)
        {
            status = NtFsControlFile(Handle,
                                     Overlapped->hEvent,
                                     NULL,
                                     Overlapped,
                                     (PIO_STATUS_BLOCK)Overlapped,
                                     IoControlCode,
                                     InBuffer,
                                     InBufferSize,
                                     OutputBuffer,
                                     OutputBufferSize);
        }
        else
        {
            status = NtDeviceIoControlFile(Handle,
                                           Overlapped->hEvent,
                                           NULL,
                                           Overlapped,
                                           (PIO_STATUS_BLOCK)Overlapped,
                                           IoControlCode,
                                           InBuffer,
                                           InBufferSize,
                                           OutputBuffer,
                                           OutputBufferSize);
        }

        if (NT_INFORMATION(status) && BytesReturned)
        {
            *BytesReturned = (ULONG)Overlapped->InternalHigh;
        }
    }
    else
    {
        IO_STATUS_BLOCK ioStatusBlock;

        if (DEVICE_TYPE_FROM_CTL_CODE(IoControlCode) == FILE_DEVICE_FILE_SYSTEM)
        {
            status = NtFsControlFile(Handle,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &ioStatusBlock,
                                     IoControlCode,
                                     InBuffer,
                                     InBufferSize,
                                     OutputBuffer,
                                     OutputBufferSize);
        }
        else
        {
            status = NtDeviceIoControlFile(Handle,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &ioStatusBlock,
                                           IoControlCode,
                                           InBuffer,
                                           InBufferSize,
                                           OutputBuffer,
                                           OutputBufferSize);
        }

        if (status == STATUS_PENDING)
        {
            status = NtWaitForSingleObject(Handle, FALSE, NULL);
            if (NT_SUCCESS(status))
            {
                status = ioStatusBlock.Status;
            }
        }

        if (BytesReturned)
        {
            *BytesReturned = (ULONG)ioStatusBlock.Information;
        }
    }

    return status;
}

// rev from FilterLoad/FilterUnload (fltlib) (dmex)
/**
 * \brief An application with mini-filter support can dynamically load and
 * unload the mini-filter.
 *
 * \param[in] ServiceName The service name from the registry.
 * \param[in] LoadDriver TRUE to load the kernel driver, FALSE to unload the
 * kernel driver.
 *
 * \remarks The caller must have SE_LOAD_DRIVER_PRIVILEGE.
 * \remarks This IOCTL is a kernel wrapper around NtLoad/UnloadDriver.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhFilterLoadUnload(
    _In_ PPH_STRINGREF ServiceName,
    _In_ BOOLEAN LoadDriver
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG filterNameBufferLength;
    PFLT_LOAD_PARAMETERS filterNameBuffer;
    SECURITY_QUALITY_OF_SERVICE filterSecurityQos =
    {
        sizeof(SECURITY_QUALITY_OF_SERVICE),
        SecurityImpersonation,
        SECURITY_DYNAMIC_TRACKING,
        TRUE
    };

    RtlInitUnicodeString(&objectName, FLT_MSG_DEVICE_NAME);
    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE | (WindowsVersion < WINDOWS_10 ? 0 : OBJ_DONT_REPARSE),
        NULL,
        NULL
        );
    objectAttributes.SecurityQualityOfService = &filterSecurityQos;

    status = NtCreateFile(
        &fileHandle,
        FILE_READ_ATTRIBUTES | GENERIC_WRITE | SYNCHRONIZE,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
        );

    if (!NT_SUCCESS(status))
        return status;

    filterNameBufferLength = UFIELD_OFFSET(FLT_LOAD_PARAMETERS, FilterName[ServiceName->Length]) + sizeof(UNICODE_NULL);
    filterNameBuffer = PhAllocateZero(filterNameBufferLength);
    filterNameBuffer->FilterNameSize = (USHORT)ServiceName->Length;
    RtlCopyMemory(filterNameBuffer->FilterName, ServiceName->Buffer, ServiceName->Length);

    status = NtDeviceIoControlFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
        LoadDriver ? FLT_CTL_LOAD : FLT_CTL_UNLOAD,
        filterNameBuffer,
        filterNameBufferLength,
        NULL,
        0
        );

    NtClose(fileHandle);
    PhFree(filterNameBuffer);

    return status;
}

/**
 * \brief Wrapper which is essentially FilterSendMessage.
 *
 * \param[in] Port Filter port handle.
 * \param[in] InBuffer Input buffer.
 * \param[in] InBufferSize Input buffer size.
 * \param[out] OutputBuffer Output Buffer.
 * \param[in] OutputBufferSize Output buffer size.
 * \param[out] BytesReturned Set to the number of bytes returned.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhFilterSendMessage(
    _In_ HANDLE Port,
    _In_reads_bytes_(InBufferSize) PVOID InBuffer,
    _In_ ULONG InBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesReturned) PVOID OutputBuffer,
    _In_ ULONG OutputBufferSize,
    _Out_ PULONG BytesReturned
    )
{
    return PhpFilterDeviceIoControl(Port,
                                    FLT_CTL_SEND_MESSAGE,
                                    InBuffer,
                                    InBufferSize,
                                    OutputBuffer,
                                    OutputBufferSize,
                                    BytesReturned,
                                    NULL);
}

/**
 * \brief Wrapper which is essentially FilterGetMessage.
 *
 * \param[in] Port Filter port handle.
 * \param[out] MessageBuffer Message buffer.
 * \param[in] MessageBufferSize Message buffer size.
 * \param[in] Overlapped Th overlapped structure.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhFilterGetMessage(
    _In_ HANDLE Port,
    _Out_writes_bytes_(MessageBufferSize) PFILTER_MESSAGE_HEADER MessageBuffer,
    _In_ ULONG MessageBufferSize,
    _Inout_ LPOVERLAPPED Overlapped
    )
{
    return PhpFilterDeviceIoControl(Port,
                                    FLT_CTL_GET_MESSAGE,
                                    NULL,
                                    0,
                                    MessageBuffer,
                                    MessageBufferSize,
                                    NULL,
                                    Overlapped);
}

/**
 * \brief Wrapper which is essentially FilterReplyMessage.
 *
 * \param[in] Port Filter port handle.
 * \param[in] ReplyBuffer Reply buffer.
 * \param[in] ReplyBufferSize Reply buffer size.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhFilterReplyMessage(
    _In_ HANDLE Port,
    _In_reads_bytes_(ReplyBufferSize) PFILTER_REPLY_HEADER ReplyBuffer,
    _In_ ULONG ReplyBufferSize
    )
{
    return PhpFilterDeviceIoControl(Port,
                                    FLT_CTL_REPLY_MESSAGE,
                                    ReplyBuffer,
                                    ReplyBufferSize,
                                    NULL,
                                    0,
                                    NULL,
                                    NULL);
}

/**
 * \brief Wrapper which is essentially FilterConnectCommunicationPort.
 *
 * \param[in] PortName Name of filter port to connect to.
 * \param[in] Options Filter port options.
 * \param[in] ConnectionContext Connection context.
 * \param[in] SizeOfContext Size of connection context.
 * \param[in] SecurityAttributes Security attributes for handle.
 * \param[out] Port Set to filter port handle on success, null on failure.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhFilterConnectCommunicationPort(
    _In_ PPH_STRINGREF PortName,
    _In_ ULONG Options,
    _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
    _In_ USHORT SizeOfContext,
    _In_opt_ PSECURITY_ATTRIBUTES SecurityAttributes,
    _Outptr_ PHANDLE Port
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;
    UNICODE_STRING portName;
    UNICODE_STRING64 portName64;
    ULONG eaLength;
    PFILE_FULL_EA_INFORMATION ea;
    PFLT_CONNECT_CONTEXT eaValue;
    IO_STATUS_BLOCK isb;

    *Port = NULL;

    if ((SizeOfContext > 0 && !ConnectionContext) ||
        (SizeOfContext == 0 && ConnectionContext))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (SizeOfContext >= FLT_PORT_CONTEXT_MAX)
        return STATUS_INTEGER_OVERFLOW;

    if (!PhStringRefToUnicodeString(PortName, &portName))
        return STATUS_NAME_TOO_LONG;

    portName64.Buffer = (ULONGLONG)portName.Buffer;
    portName64.Length = portName.Length;
    portName64.MaximumLength = portName.MaximumLength;

    //
    // Build the filter EA, this contains the port name and the context.
    //

    eaLength = FLT_PORT_FULL_EA_SIZE
             + FLT_PORT_FULL_EA_VALUE_SIZE
             + SizeOfContext;

    ea = PhAllocateZeroSafe(eaLength);
    if (!ea)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ea->Flags = 0;
    ea->EaNameLength = sizeof(FLT_PORT_EA_NAME) - sizeof(ANSI_NULL);
    ea->EaValueLength = FLT_PORT_FULL_EA_VALUE_SIZE + SizeOfContext;
    RtlCopyMemory(ea->EaName, FLT_PORT_EA_NAME, sizeof(FLT_PORT_EA_NAME));
    eaValue = PTR_ADD_OFFSET(ea->EaName, sizeof(FLT_PORT_EA_NAME));
    eaValue->PortName = &portName;
    eaValue->PortName64 = &portName64;
    eaValue->SizeOfContext = SizeOfContext;

    if (SizeOfContext > 0)
    {
        RtlCopyMemory(eaValue->Context,
                      ConnectionContext,
                      SizeOfContext);
    }

    RtlInitUnicodeString(&objectName, FLT_MSG_DEVICE_NAME);
    InitializeObjectAttributes(&objectAttributes,
                               &objectName,
                               OBJ_CASE_INSENSITIVE | (WindowsVersion < WINDOWS_10 ? 0 : OBJ_DONT_REPARSE),
                               NULL,
                               NULL);
    if (SecurityAttributes)
    {
        if (SecurityAttributes->bInheritHandle)
        {
            objectAttributes.Attributes |= OBJ_INHERIT;
        }
        objectAttributes.SecurityDescriptor = SecurityAttributes->lpSecurityDescriptor;
    }

    status = NtCreateFile(Port,
                          FILE_READ_ACCESS | FILE_WRITE_ACCESS | SYNCHRONIZE,
                          &objectAttributes,
                          &isb,
                          NULL,
                          0,
                          0,
                          FILE_OPEN_IF,
                          Options & FLT_PORT_FLAG_SYNC_HANDLE ? FILE_SYNCHRONOUS_IO_NONALERT : 0,
                          ea,
                          eaLength);

    PhFree(ea);

    return status;
}
