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

#define KPH_ALPC_NAME_BUFFER_SIZE ((MAX_PATH * 2) + sizeof(OBJECT_NAME_INFORMATION))

/**
 * \brief References any communication ports associated with the input port.
 * The caller must dereference any returned objects by calling ObDereferenceObject.
 *
 * \param[in] Port The ALPC port to references the associated ports of.
 * \param[out] ConnectionPort Set to the connection port.
 * \param[out] ServerPort Set to the server port.
 * \param[out] ClientPort Set to the client port.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpReferenceAlpcCommunicationPorts(
    _In_ PVOID Port,
    _Outptr_result_maybenull_ PVOID* ConnectionPort,
    _Outptr_result_maybenull_ PVOID* ServerPort,
    _Outptr_result_maybenull_ PVOID* ClientPort
    )
{
    NTSTATUS status;
    PVOID communicationInfo;
    PVOID handleTable;
    PVOID handleTableLock;
    BOOLEAN typeMismatch;
    PVOID connectionPort;
    PVOID serverPort;
    PVOID clientPort;

    PAGED_PASSIVE();

    *ConnectionPort = NULL;
    *ServerPort = NULL;
    *ClientPort = NULL;

    connectionPort = NULL;
    serverPort = NULL;
    clientPort = NULL;

    if ((KphDynAlpcCommunicationInfo == ULONG_MAX) ||
        (KphDynAlpcHandleTable == ULONG_MAX) ||
        (KphDynAlpcHandleTableLock == ULONG_MAX))
    {
        status = STATUS_NOINTERFACE;
        goto Exit;
    }

    communicationInfo = *(PVOID*)Add2Ptr(Port, KphDynAlpcCommunicationInfo);
    if (!communicationInfo)
    {
        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    handleTable = Add2Ptr(communicationInfo, KphDynAlpcHandleTable);
    handleTableLock = Add2Ptr(handleTable, KphDynAlpcHandleTableLock);

    typeMismatch = FALSE;

    FltAcquirePushLockShared(handleTableLock);

    if (KphDynAlpcConnectionPort != ULONG_MAX)
    {
        connectionPort = *(PVOID*)Add2Ptr(communicationInfo, KphDynAlpcConnectionPort);
        if (connectionPort)
        {
            if (ObGetObjectType(connectionPort) != *AlpcPortObjectType)
            {
                typeMismatch = TRUE;
                connectionPort = NULL;
            }
            else
            {
                ObReferenceObject(connectionPort);
            }
        }
    }

    if (KphDynAlpcServerCommunicationPort != ULONG_MAX)
    {
        serverPort = *(PVOID*)Add2Ptr(communicationInfo, KphDynAlpcServerCommunicationPort);
        if (serverPort)
        {
            if (ObGetObjectType(serverPort) != *AlpcPortObjectType)
            {
                typeMismatch = TRUE;
                serverPort = NULL;
            }
            else
            {
                ObReferenceObject(serverPort);
            }
        }
    }

    if (KphDynAlpcClientCommunicationPort != ULONG_MAX)
    {
        clientPort = *(PVOID*)Add2Ptr(communicationInfo, KphDynAlpcClientCommunicationPort);
        if (clientPort)
        {
            if (ObGetObjectType(clientPort) != *AlpcPortObjectType)
            {
                typeMismatch = TRUE;
                clientPort = NULL;
            }
            else
            {
                ObReferenceObject(clientPort);
            }
        }
    }

    FltReleasePushLock(handleTableLock);

    if (typeMismatch)
    {
        status = STATUS_OBJECT_TYPE_MISMATCH;
        goto Exit;
    }

    *ConnectionPort = connectionPort;
    connectionPort = NULL;
    *ServerPort = serverPort;
    serverPort = NULL;
    *ClientPort = clientPort;
    clientPort = NULL;
    status = STATUS_SUCCESS;

Exit:

    if (connectionPort)
    {
        ObDereferenceObject(connectionPort);
    }

    if (serverPort)
    {
        ObDereferenceObject(serverPort);
    }

    if (clientPort)
    {
        ObDereferenceObject(clientPort);
    }

    return status;
}

/**
 * \brief Retrieves the basic information for an ALPC port.
 *
 * \param[in] Port The ALPC port to get the basic information of.
 * \param[out] Info Populated with the basic information.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpAlpcBasicInfo(
    _In_ PVOID Port,
    _Out_ PKPH_ALPC_BASIC_INFORMATION Info
    )
{
    PEPROCESS process;
    PVOID portObjectLock;

    PAGED_PASSIVE();

    RtlZeroMemory(Info, sizeof(*Info));

    if ((KphDynAlpcOwnerProcess == ULONG_MAX) ||
        (KphDynAlpcPortObjectLock == ULONG_MAX))
    {
        return STATUS_NOINTERFACE;
    }

    //
    // The OS uses the first bit of the OwnerProcess to denote if it is valid,
    // if the first bit of the OwnerProcess is set is it invalid. Checking the  
    // bit should be done under the PortObjectLock.
    // See: ntoskrnl!AlpcpPortQueryServerSessionInfo
    //

    portObjectLock = Add2Ptr(Port, KphDynAlpcPortObjectLock);
    FltAcquirePushLockShared(portObjectLock);

    process = *(PEPROCESS*)Add2Ptr(Port, KphDynAlpcOwnerProcess);
    if (process && (((ULONG_PTR)process & 1) == 0))
    {
        Info->OwnerProcessId = PsGetProcessId(process);
    }

    FltReleasePushLock(portObjectLock);

    if ((KphDynAlpcAttributes != ULONG_MAX) &&
        (KphDynAlpcAttributesFlags != ULONG_MAX))
    {
        Info->Flags = *(PULONG)Add2Ptr(Add2Ptr(Port, KphDynAlpcAttributes),
                                       KphDynAlpcAttributesFlags);
    }

    if (KphDynAlpcPortContext != ULONG_MAX)
    {
        Info->PortContext = *(PVOID*)Add2Ptr(Port, KphDynAlpcPortContext);
    }

    if (KphDynAlpcSequenceNo != ULONG_MAX)
    {
        Info->SequenceNo = *(PULONG)Add2Ptr(Port, KphDynAlpcSequenceNo);
    }

    if (KphDynAlpcState != ULONG_MAX)
    {
        Info->State = *(PULONG)Add2Ptr(Port, KphDynAlpcState);
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Retrieves the communication information for an ALPC port.
 *
 * \param[in] Port The ALPC port to get the information of.
 * \param[out] Info Populated with the communication information.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpAlpcCommunicationInfo(
    _In_ PVOID Port,
    _Out_ PKPH_ALPC_COMMUNICATION_INFORMATION Info
    )
{
    NTSTATUS status;
    PVOID connectionPort;
    PVOID serverPort;
    PVOID clientPort;

    PAGED_PASSIVE();

    RtlZeroMemory(Info, sizeof(*Info));

    status = KphpReferenceAlpcCommunicationPorts(Port,
                                                 &connectionPort,
                                                 &serverPort,
                                                 &clientPort);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphpReferenceAlpcCommunicationPorts failed: %!STATUS!",
                      status);

        connectionPort = NULL;
        serverPort = NULL;
        clientPort = NULL;
        goto Exit;
    }

    if (connectionPort)
    {
        status = KphpAlpcBasicInfo(connectionPort, &Info->ConnectionPort);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    if (serverPort)
    {
        status = KphpAlpcBasicInfo(serverPort, &Info->ServerCommunicationPort);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    if (clientPort)
    {
        status = KphpAlpcBasicInfo(clientPort, &Info->ClientCommunicationPort);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

Exit:

    if (connectionPort)
    {
        ObDereferenceObject(connectionPort);
    }

    if (serverPort)
    {
        ObDereferenceObject(serverPort);
    }

    if (clientPort)
    {
        ObDereferenceObject(clientPort);
    }

    return status;
}

/**
 * \brief Utility function for copying the ALPC port name into an output string.
 *
 * \param[in] Port The ALPC port to copy the name of.
 * \param[in] NameBuffer Preallocated name buffer to use to get the name.
 * \param[in,out] Buffer The space to write the string.
 * \param[in,out] RemainingLength The remaining space to write the string.
 * \param[in,out] ReturnLength The return length, updated even if the string won't fit.
 * \param[out] String The string to populate.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpAlpcCopyPortName(
    _In_ PVOID Port,
    _In_bytecount_(KPH_ALPC_NAME_BUFFER_SIZE) POBJECT_NAME_INFORMATION NameBuffer,
    _Inout_ PVOID* Buffer,
    _Inout_ PULONG RemainingLength,
    _Inout_ PULONG ReturnLength,
    _Out_opt_ PUNICODE_STRING String
    )
{
    NTSTATUS status;
    ULONG returnLength;

    PAGED_PASSIVE();

    if (String)
    {
        RtlZeroMemory(String, sizeof(*String));
    }

    status = KphQueryNameObject(Port,
                                NameBuffer,
                                KPH_ALPC_NAME_BUFFER_SIZE,
                                &returnLength);
    if (!NT_SUCCESS(status) || (NameBuffer->Name.Length == 0))
    {
        //
        // We're best effort here.
        //
        return STATUS_SUCCESS;
    }

    status = RtlULongAdd(*ReturnLength, NameBuffer->Name.Length, ReturnLength);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (!String || (NameBuffer->Name.Length > *RemainingLength))
    {
        //
        // Success, return length is updated with the needed length.
        //
        return STATUS_SUCCESS;
    }

    String->Buffer = *Buffer;
    String->MaximumLength = NameBuffer->Name.Length;
    *Buffer = Add2Ptr(*Buffer, NameBuffer->Name.Length);
    *RemainingLength -= NameBuffer->Name.Length;
    RtlCopyUnicodeString(String, &NameBuffer->Name);

    return STATUS_SUCCESS;
}

/**
 * \brief Retrieves the names of the communication ports for a given ALPC port.
 *
 * \param[in] Port The port to retrieve the names of.
 * \param[out] Info Populated with the name information.
 * \param[in] InfoLength The length of the information buffer.
 * \param[out] ReturnLength Populated with the length of the written information,
 * or the needed length if it is insufficient.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpAlpcCommunicationNamesInfo(
    _In_ PVOID Port,
    _Out_writes_bytes_opt_(InfoLength) PKPH_ALPC_COMMUNICATION_NAMES_INFORMATION Info,
    _In_ ULONG InfoLength,
    _Out_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    POBJECT_NAME_INFORMATION nameBuffer;
    PVOID connectionPort;
    PVOID serverPort;
    PVOID clientPort;
    PVOID buffer;
    ULONG remainingLength;

    PAGED_PASSIVE();

    *ReturnLength = sizeof(KPH_ALPC_COMMUNICATION_NAMES_INFORMATION);

    nameBuffer = NULL;

    if (Info && (InfoLength >= sizeof(KPH_ALPC_COMMUNICATION_NAMES_INFORMATION)))
    {
        RtlZeroMemory(Info, sizeof(KPH_ALPC_COMMUNICATION_NAMES_INFORMATION));
        remainingLength = (InfoLength - sizeof(KPH_ALPC_COMMUNICATION_NAMES_INFORMATION));
        buffer = Add2Ptr(Info, sizeof(KPH_ALPC_COMMUNICATION_NAMES_INFORMATION));
    }
    else
    {
        remainingLength = 0;
        buffer = NULL;
    }

    status = KphpReferenceAlpcCommunicationPorts(Port,
                                                 &connectionPort,
                                                 &serverPort,
                                                 &clientPort);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphpReferenceAlpcCommunicationPorts failed: %!STATUS!",
                      status);

        connectionPort = NULL;
        serverPort = NULL;
        clientPort = NULL;
        goto Exit;
    }

    nameBuffer = KphAllocatePaged(KPH_ALPC_NAME_BUFFER_SIZE,
                                  KPH_TAG_ALPC_NAME_QUERY);
    if (!nameBuffer)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    if (connectionPort)
    {
        status = KphpAlpcCopyPortName(connectionPort,
                                      nameBuffer,
                                      &buffer,
                                      &remainingLength,
                                      ReturnLength,
                                      (Info ? &Info->ConnectionPort : NULL));
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    if (serverPort)
    {
        status = KphpAlpcCopyPortName(serverPort,
                                      nameBuffer,
                                      &buffer,
                                      &remainingLength,
                                      ReturnLength,
                                      (Info ? &Info->ServerCommunicationPort : NULL));
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    if (clientPort)
    {
        status = KphpAlpcCopyPortName(clientPort,
                                      nameBuffer,
                                      &buffer,
                                      &remainingLength,
                                      ReturnLength,
                                      (Info ? &Info->ClientCommunicationPort : NULL));
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    if (*ReturnLength > InfoLength)
    {
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        status = STATUS_SUCCESS;
    }

Exit:

    if (nameBuffer)
    {
        KphFree(nameBuffer, KPH_TAG_ALPC_NAME_QUERY);
    }

    if (connectionPort)
    {
        ObDereferenceObject(connectionPort);
    }

    if (serverPort)
    {
        ObDereferenceObject(serverPort);
    }

    if (clientPort)
    {
        ObDereferenceObject(clientPort);
    }

    return status;
}

/**
 * \brief Queries information about an ALPC port.
 *
 * \param[in] ProceessHandle Handle to the process where the ALPC handle resides.
 * \param[in] PortHandle Handle to the ALPC port to query.
 * \param[in] AlpcInformationClass Information class to query.
 * \param[out] AlpcInformation Populated with information by ALPC port.
 * \param[in] AlpcInformationLength Length of the ALPC information buffer.
 * \param[out] ReturnLength Number of bytes written or necessary for the
 * information.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphAlpcQueryInformation(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE PortHandle,
    _In_ KPH_ALPC_INFORMATION_CLASS AlpcInformationClass,
    _Out_writes_bytes_opt_(AlpcInformationLength) PVOID AlpcInformation,
    _In_ ULONG AlpcInformationLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;
    KAPC_STATE apcState;
    PVOID port;
    ULONG returnLength;
    PVOID buffer;
    BYTE stackBuffer[64];

    PAGED_PASSIVE();

    process = NULL;
    port = NULL;
    returnLength = 0;
    buffer = NULL;

    if (AccessMode != KernelMode)
    {
        __try
        {
            if (AlpcInformation)
            {
                ProbeForWrite(AlpcInformation, AlpcInformationLength, 1);
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
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        process = NULL;
        goto Exit;
    }

    if (process == PsInitialSystemProcess)
    {
        PortHandle = MakeKernelHandle(PortHandle);
        AccessMode = KernelMode;
    }
    else
    {
        if (IsKernelHandle(PortHandle))
        {
            status = STATUS_INVALID_HANDLE;
            goto Exit;
        }
    }

    KeStackAttachProcess(process, &apcState);
    status = ObReferenceObjectByHandle(PortHandle,
                                       0,
                                       *AlpcPortObjectType,
                                       AccessMode,
                                       &port,
                                       NULL);
    KeUnstackDetachProcess(&apcState);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        port = NULL;
        goto Exit;
    }

    switch (AlpcInformationClass)
    {
        case KphAlpcBasicInformation:
        {
            KPH_ALPC_BASIC_INFORMATION info;

            if (!AlpcInformation || (AlpcInformationLength < sizeof(info)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(info);
                goto Exit;
            }

            status = KphpAlpcBasicInfo(port, &info);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_ERROR,
                              GENERAL,
                              "KphpAlpcBasicInfo failed: %!STATUS!",
                              status);
                goto Exit;
            }

            __try
            {
                RtlCopyMemory(AlpcInformation, &info, sizeof(info));
                returnLength = sizeof(info);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
                goto Exit;
            }

            break;
        }
        case KphAlpcCommunicationInformation:
        {
            KPH_ALPC_COMMUNICATION_INFORMATION info;

            if (!AlpcInformation || (AlpcInformationLength < sizeof(info)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(info);
                goto Exit;
            }

            status = KphpAlpcCommunicationInfo(port, &info);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_ERROR,
                              GENERAL,
                              "KphpAlpcCommunicationInfo failed: %!STATUS!",
                              status);
                goto Exit;
            }

            __try
            {
                RtlCopyMemory(AlpcInformation, &info, sizeof(info));
                returnLength = sizeof(info);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
                goto Exit;
            }

            break;
        }
        case KphAlpcCommunicationNamesInformation:
        {
            ULONG allocatedSize;
            PKPH_ALPC_COMMUNICATION_NAMES_INFORMATION info;

            allocatedSize = AlpcInformationLength;
            if (allocatedSize < sizeof(KPH_ALPC_COMMUNICATION_NAMES_INFORMATION))
            {
                allocatedSize = sizeof(KPH_ALPC_COMMUNICATION_NAMES_INFORMATION);
            }

            if (allocatedSize <= ARRAYSIZE(stackBuffer))
            {
                buffer = stackBuffer;
            }
            else
            {
                buffer = KphAllocatePaged(allocatedSize, KPH_TAG_ALPC_QUERY);
                if (!buffer)
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Exit;
                }
            }

            info = buffer;
            status = KphpAlpcCommunicationNamesInfo(port,
                                                    info,
                                                    AlpcInformationLength,
                                                    &returnLength);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_ERROR,
                              GENERAL,
                              "KphpAlpcCommunicationNamesInfo failed: %!STATUS!",
                              status);
                goto Exit;
            }

            if (!AlpcInformation)
            {
                status = STATUS_BUFFER_TOO_SMALL;
                goto Exit;
            }

            if (info->ConnectionPort.Buffer)
            {
                RebaseUnicodeString(&info->ConnectionPort,
                                    buffer,
                                    AlpcInformation);
            }

            if (info->ServerCommunicationPort.Buffer)
            {
                RebaseUnicodeString(&info->ServerCommunicationPort,
                                    buffer,
                                    AlpcInformation);
            }

            if (info->ClientCommunicationPort.Buffer)
            {
                RebaseUnicodeString(&info->ClientCommunicationPort,
                                    buffer,
                                    AlpcInformation);
            }

            __try
            {
                RtlCopyMemory(AlpcInformation, info, returnLength);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
                goto Exit;
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

    if (buffer && (buffer != stackBuffer))
    {
        KphFree(buffer, KPH_TAG_ALPC_QUERY);
    }

    if (port)
    {
        ObDereferenceObject(port);
    }

    if (process)
    {
        ObDereferenceObject(process);
    }

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

    return status;
}

