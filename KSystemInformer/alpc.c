/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2025
 *
 */

#include <kph.h>

#include <trace.h>

KPH_PAGED_FILE();

#define KPH_ALPC_NAME_BUFFER_SIZE ((MAX_PATH * 2) + sizeof(OBJECT_NAME_INFORMATION))

/**
 * \brief References any communication ports associated with the input port.
 * The caller must dereference any returned objects by calling ObDereferenceObject.
 *
 * \param[in] Dyn Dynamic configuration.
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
    _In_ PKPH_DYN Dyn,
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

    KPH_PAGED_CODE_PASSIVE();

    *ConnectionPort = NULL;
    *ServerPort = NULL;
    *ClientPort = NULL;

    connectionPort = NULL;
    serverPort = NULL;
    clientPort = NULL;

    if ((Dyn->AlpcCommunicationInfo == ULONG_MAX) ||
        (Dyn->AlpcHandleTable == ULONG_MAX) ||
        (Dyn->AlpcHandleTableLock == ULONG_MAX))
    {
        status = STATUS_NOINTERFACE;
        goto Exit;
    }

    communicationInfo = *(PVOID*)Add2Ptr(Port, Dyn->AlpcCommunicationInfo);
    if (!communicationInfo)
    {
        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    handleTable = Add2Ptr(communicationInfo, Dyn->AlpcHandleTable);
    handleTableLock = Add2Ptr(handleTable, Dyn->AlpcHandleTableLock);

    typeMismatch = FALSE;

    FltAcquirePushLockShared(handleTableLock);

    if (Dyn->AlpcConnectionPort != ULONG_MAX)
    {
        connectionPort = *(PVOID*)Add2Ptr(communicationInfo,
                                          Dyn->AlpcConnectionPort);
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

    if (Dyn->AlpcServerCommunicationPort != ULONG_MAX)
    {
        serverPort = *(PVOID*)Add2Ptr(communicationInfo,
                                      Dyn->AlpcServerCommunicationPort);
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

    if (Dyn->AlpcClientCommunicationPort != ULONG_MAX)
    {
        clientPort = *(PVOID*)Add2Ptr(communicationInfo,
                                      Dyn->AlpcClientCommunicationPort);
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
 * \param[in] Dyn Dynamic configuration.
 * \param[in] Port The ALPC port to get the basic information of.
 * \param[out] Info Populated with the basic information.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpAlpcBasicInfo(
    _In_ PKPH_DYN Dyn,
    _In_ PVOID Port,
    _Out_ PKPH_ALPC_BASIC_INFORMATION Info
    )
{
    PEPROCESS process;
    PVOID portObjectLock;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(Info, sizeof(*Info));

    if ((Dyn->AlpcOwnerProcess == ULONG_MAX) ||
        (Dyn->AlpcPortObjectLock == ULONG_MAX))
    {
        return STATUS_NOINTERFACE;
    }

    //
    // The OS uses the first bit of the OwnerProcess to denote if it is valid,
    // if the first bit of the OwnerProcess is set is it invalid. Checking the
    // bit should be done under the PortObjectLock.
    // See: ntoskrnl!AlpcpPortQueryServerSessionInfo
    //

    portObjectLock = Add2Ptr(Port, Dyn->AlpcPortObjectLock);
    FltAcquirePushLockShared(portObjectLock);

    process = *(PEPROCESS*)Add2Ptr(Port, Dyn->AlpcOwnerProcess);
    if (process && (((ULONG_PTR)process & 1) == 0))
    {
        Info->OwnerProcessId = PsGetProcessId(process);
    }

    FltReleasePushLock(portObjectLock);

    if ((Dyn->AlpcAttributes != ULONG_MAX) &&
        (Dyn->AlpcAttributesFlags != ULONG_MAX))
    {
        Info->Flags = *(PULONG)Add2Ptr(Add2Ptr(Port, Dyn->AlpcAttributes),
                                       Dyn->AlpcAttributesFlags);
    }

    if (Dyn->AlpcPortContext != ULONG_MAX)
    {
        Info->PortContext = *(PVOID*)Add2Ptr(Port, Dyn->AlpcPortContext);
    }

    if (Dyn->AlpcSequenceNo != ULONG_MAX)
    {
        Info->SequenceNo = *(PULONG)Add2Ptr(Port, Dyn->AlpcSequenceNo);
    }

    if (Dyn->AlpcState != ULONG_MAX)
    {
        Info->State = *(PULONG)Add2Ptr(Port, Dyn->AlpcState);
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Retrieves the communication information for an ALPC port.
 *
 * \param[in] Dyn Dynamic configuration.
 * \param[in] Port The ALPC port to get the information of.
 * \param[out] Info Populated with the communication information.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpAlpcCommunicationInfo(
    _In_ PKPH_DYN Dyn,
    _In_ PVOID Port,
    _Out_ PKPH_ALPC_COMMUNICATION_INFORMATION Info
    )
{
    NTSTATUS status;
    PVOID connectionPort;
    PVOID serverPort;
    PVOID clientPort;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(Info, sizeof(*Info));

    status = KphpReferenceAlpcCommunicationPorts(Dyn,
                                                 Port,
                                                 &connectionPort,
                                                 &serverPort,
                                                 &clientPort);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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
        status = KphpAlpcBasicInfo(Dyn,
                                   connectionPort,
                                   &Info->ConnectionPort);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    if (serverPort)
    {
        status = KphpAlpcBasicInfo(Dyn,
                                   serverPort,
                                   &Info->ServerCommunicationPort);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    if (clientPort)
    {
        status = KphpAlpcBasicInfo(Dyn,
                                   clientPort,
                                   &Info->ClientCommunicationPort);
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

    KPH_PAGED_CODE_PASSIVE();

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
 * \param[in] Dyn Dynamic configuration.
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
    _In_ PKPH_DYN Dyn,
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

    KPH_PAGED_CODE_PASSIVE();

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

    status = KphpReferenceAlpcCommunicationPorts(Dyn,
                                                 Port,
                                                 &connectionPort,
                                                 &serverPort,
                                                 &clientPort);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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
    PKPH_DYN dyn;
    PEPROCESS process;
    KAPC_STATE apcState;
    PVOID port;
    ULONG returnLength;
    PVOID buffer;
    BYTE stackBuffer[64];

    KPH_PAGED_CODE_PASSIVE();

    dyn = NULL;
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
                ProbeOutputBytes(AlpcInformation, AlpcInformationLength);
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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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

            dyn = KphReferenceDynData();
            if (!dyn)
            {
                status = STATUS_NOINTERFACE;
                goto Exit;
            }

            if (!AlpcInformation || (AlpcInformationLength < sizeof(info)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(info);
                goto Exit;
            }

            status = KphpAlpcBasicInfo(dyn, port, &info);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
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

            dyn = KphReferenceDynData();
            if (!dyn)
            {
                status = STATUS_NOINTERFACE;
                goto Exit;
            }

            if (!AlpcInformation || (AlpcInformationLength < sizeof(info)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(info);
                goto Exit;
            }

            status = KphpAlpcCommunicationInfo(dyn, port, &info);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
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

            dyn = KphReferenceDynData();
            if (!dyn)
            {
                status = STATUS_NOINTERFACE;
                goto Exit;
            }

            allocatedSize = AlpcInformationLength;
            if (allocatedSize < sizeof(KPH_ALPC_COMMUNICATION_NAMES_INFORMATION))
            {
                allocatedSize = sizeof(KPH_ALPC_COMMUNICATION_NAMES_INFORMATION);
            }

            buffer = KphAllocatePagedA(allocatedSize,
                                       KPH_TAG_ALPC_QUERY,
                                       stackBuffer);
            if (!buffer)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Exit;
            }

            info = buffer;
            status = KphpAlpcCommunicationNamesInfo(dyn,
                                                    port,
                                                    info,
                                                    AlpcInformationLength,
                                                    &returnLength);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
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

            RebaseUnicodeString(&info->ConnectionPort,
                                buffer,
                                AlpcInformation);
            RebaseUnicodeString(&info->ServerCommunicationPort,
                                buffer,
                                AlpcInformation);
            RebaseUnicodeString(&info->ClientCommunicationPort,
                                buffer,
                                AlpcInformation);

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

    if (buffer)
    {
        KphFreeA(buffer, KPH_TAG_ALPC_QUERY, stackBuffer);
    }

    if (port)
    {
        ObDereferenceObject(port);
    }

    if (process)
    {
        ObDereferenceObject(process);
    }

    if (dyn)
    {
        KphDereferenceObject(dyn);
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

