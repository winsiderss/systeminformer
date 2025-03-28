/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     diversenok   2025
 *
 */

#include <phafd.h>
#include <hndlinfo.h>
#include <ws2ipdef.h>
#include <ws2bth.h>
#include <hvsocket.h>

 /**
   * \brief Determines if an object name represents an AFD socket handle.
   *
   * \param[in] ObjectName A native object name.
   *
   * \return Whether the name matches an AFD socket name format.
   */
BOOLEAN
NTAPI
PhAfdIsSocketObjectName(
    _In_opt_ PPH_STRING ObjectName
    )
{
    return ObjectName && PhStartsWithString2(ObjectName, AFD_DEVICE_NAME, TRUE) &&
        (ObjectName->Length == sizeof(AFD_DEVICE_NAME) - sizeof(UNICODE_NULL) ||
         ObjectName->Buffer[ObjectName->Length / sizeof(WCHAR)] == OBJ_NAME_PATH_SEPARATOR);
}

/**
  * \brief Determines if a file handle is an AFD socket handle.
  *
  * \param[in] Handle A file handle.
  *
  * \return A successful status if the handle is an AFD socket or an errant status otherwise.
  */
NTSTATUS
NTAPI
PhAfdIsSocketHandle(
    _In_ HANDLE Handle
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;

    union {
        FILE_VOLUME_NAME_INFORMATION VolumeName;
        UCHAR Raw[sizeof(FILE_VOLUME_NAME_INFORMATION) + sizeof(AFD_DEVICE_NAME) - sizeof(UNICODE_NULL)];
    } Buffer;

    // Query the backing device name
    status = NtQueryInformationFile(
        Handle,
        &ioStatusBlock,
        &Buffer,
        sizeof(Buffer),
        FileVolumeNameInformation
        );

    // If the name does not fit into the buffer, it's not AFD
    if (status == STATUS_BUFFER_OVERFLOW)
        return STATUS_NOT_SAME_DEVICE;

    if (!NT_SUCCESS(status))
        return status;

    static UNICODE_STRING afdDeviceName = RTL_CONSTANT_STRING(AFD_DEVICE_NAME);
    UNICODE_STRING volumeName;

    volumeName.Buffer = Buffer.VolumeName.DeviceName;
    volumeName.Length = (USHORT)Buffer.VolumeName.DeviceNameLength;
    volumeName.MaximumLength = (USHORT)Buffer.VolumeName.DeviceNameLength;

    // Compare the file's device name to AFD
    return RtlEqualUnicodeString(&volumeName, &afdDeviceName, TRUE) ? STATUS_SUCCESS : STATUS_NOT_SAME_DEVICE;
}

 /**
  * \brief Issues an IOCTL on an AFD handle and waits for its completion.
  *
  * \param[in] SocketHandle An AFD socket handle.
  * \param[in] IoControlCode I/O control code
  * \param[in] InBuffer Input buffer.
  * \param[in] InBufferSize Input buffer size.
  * \param[out] OutputBuffer Output Buffer.
  * \param[in] OutputBufferSize Output buffer size.
  * \param[out] BytesReturned Optionally set to the number of bytes returned.
  * \param[in,out] Overlapped Optional overlapped structure.
  *
  * \return Successful or errant status.
  */
NTSTATUS
NTAPI
PhAfdDeviceIoControl(
    _In_ HANDLE SocketHandle,
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
    HANDLE hEvent;
    IO_STATUS_BLOCK ioStatusBlock;

    if (BytesReturned)
    {
        *BytesReturned = 0;
    }

    if (Overlapped)
    {
        hEvent = Overlapped->hEvent;
        Overlapped->Internal = STATUS_PENDING;
    }
    else
    {
        // We cannot wait on the file handle because it might not grant SYNCHRONIZE access.
        // Always use an event instead.

        status = NtCreateEvent(
            &hEvent,
            EVENT_ALL_ACCESS,
            NULL,
            SynchronizationEvent,
            FALSE
            );

        if (!NT_SUCCESS(status))
            return status;
    }

    status = NtDeviceIoControlFile(
        SocketHandle,
        hEvent,
        NULL,
        Overlapped,
        Overlapped ? (PIO_STATUS_BLOCK)Overlapped : &ioStatusBlock,
        IoControlCode,
        InBuffer,
        InBufferSize,
        OutputBuffer,
        OutputBufferSize
        );

    if (Overlapped)
    {
        if (NT_INFORMATION(status) && BytesReturned)
        {
            *BytesReturned = (ULONG)Overlapped->InternalHigh;
        }
    }
    else
    {
        if (status == STATUS_PENDING)
        {
            NtWaitForSingleObject(hEvent, FALSE, NULL);
            status = ioStatusBlock.Status;
        }

        NtClose(hEvent);

        if (BytesReturned)
        {
            *BytesReturned = (ULONG)ioStatusBlock.Information;
        }
    }

    return status;
}

/**
  * \brief Retrieves shared information for an AFD socket.
  *
  * \param[in] SocketHandle An AFD socket handle.
  * \param[out] SharedInfo A buffer with the shared socket information.
  *
  * \return Successful or errant status.
  */
NTSTATUS
NTAPI
PhAfdQuerySharedInfo(
    _In_ HANDLE SocketHandle,
    _Out_ PSOCK_SHARED_INFO SharedInfo
    )
{
    NTSTATUS status;
    ULONG returnedSize;

    status = PhAfdDeviceIoControl(
        SocketHandle,
        IOCTL_AFD_GET_CONTEXT,
        NULL,
        0,
        SharedInfo,
        sizeof(SOCK_SHARED_INFO),
        &returnedSize,
        NULL
        );

    if (status == STATUS_BUFFER_OVERFLOW)
        return STATUS_SUCCESS;

    if (!NT_SUCCESS(status))
        return status;

    // Shared information is provided by the Win32 level; do a sanity check on the returned size
    return returnedSize < sizeof(SOCK_SHARED_INFO) ? STATUS_NOT_FOUND : status;
}

/**
  * \brief Retrieves simple information for an AFD socket.
  *
  * \param[in] SocketHandle An AFD socket handle.
  * \param[in] InformationType The type of information to query, such as AFD_CONNECT_TIME or AFD_GROUP_ID_AND_TYPE.
  * \param[out] Information Output buffer.
  *
  * \return Successful or errant status.
  */
NTSTATUS
NTAPI
PhAfdQuerySimpleInfo(
    _In_ HANDLE SocketHandle,
    _In_ ULONG InformationType,
    _Out_ PAFD_INFORMATION Information
    )
{
    Information->InformationType = InformationType;

    return PhAfdDeviceIoControl(
        SocketHandle,
        IOCTL_AFD_GET_INFORMATION,
        Information,
        sizeof(AFD_INFORMATION),
        Information,
        sizeof(AFD_INFORMATION),
        NULL,
        NULL
        );
}

/**
  * \brief Retrieves socket option for an AFD socket.
  *
  * \param[in] Level A level for the option, such as SOL_SOCKET or IPPROTO_IP.
  * \param[in] OptionName An option identifier within the level, such as SO_REUSEADDR or IP_TTL.
  * \param[out] OptionValue A buffer that receives the option value.
  *
  * \return Successful or errant status.
  */
NTSTATUS
NTAPI
PhAfdQueryOption(
    _In_ HANDLE SocketHandle,
    _In_ ULONG Level,
    _In_ ULONG OptionName,
    _Out_ PULONG OptionValue
    )
{
    AFD_TL_IO_CONTROL_INFO controlInfo = { 0 };
    controlInfo.Type = TlGetSockOptIoControlType;
    controlInfo.EndpointIoctl = TRUE;
    controlInfo.Level = Level;
    controlInfo.IoControlCode = OptionName;
    *OptionValue = 0;

    return PhAfdDeviceIoControl(
        SocketHandle,
        IOCTL_AFD_TRANSPORT_IOCTL,
        &controlInfo,
        sizeof(AFD_TL_IO_CONTROL_INFO),
        OptionValue,
        sizeof(ULONG),
        NULL,
        NULL
        );
}

/**
  * \brief Opens an address or a connection handle to the underlying device for a TDI socket.
  *
  * \param[in] SocketHandle An AFD socket handle.
  * \param[in] QueryMode A type of the query, either AFD_QUERY_ADDRESS_HANDLE or AFD_QUERY_CONNECTION_HANDLE.
  * \param[out] TdiHandle A pointer to a variable that receives a TDI device handle, NULL (when no handle is available), or INVALID_HANDLE_VALUE (for non-TDI sockets).
  *
  * \return Successful or errant status.
  */
NTSTATUS
NTAPI
PhAfdQueryTdiHandle(
    _In_ HANDLE SocketHandle,
    _In_ ULONG QueryMode,
    _Out_ PHANDLE TdiHandle
    )
{
    NTSTATUS status;
    AFD_HANDLE_INFO handles;

    if (QueryMode != AFD_QUERY_ADDRESS_HANDLE && QueryMode != AFD_QUERY_CONNECTION_HANDLE)
        return STATUS_INVALID_INFO_CLASS;

    status = PhAfdDeviceIoControl(
        SocketHandle,
        IOCTL_AFD_QUERY_HANDLES,
        &QueryMode,
        sizeof(QueryMode),
        &handles,
        sizeof(handles),
        NULL,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        if (QueryMode == AFD_QUERY_ADDRESS_HANDLE)
            *TdiHandle = handles.TdiAddressHandle;
        else
            *TdiHandle = handles.TdiConnectionHandle;
    }

    return status;
}

/**
  * \brief Formats a TDI device name for a socket.
  *
  * \param[in] SocketHandle An AFD socket handle.
  * \param[in] QueryMode A type of the query, either AFD_QUERY_ADDRESS_HANDLE or AFD_QUERY_CONNECTION_HANDLE.
  * \param[out] TdiDeviceName A pointer to a variable that receives a device name string.
  *
  * \return Successful or errant status.
  */
NTSTATUS
NTAPI
PhAfdQueryFormatTdiDeviceName(
    _In_ HANDLE SocketHandle,
    _In_ ULONG QueryMode,
    _Outptr_ PPH_STRING *TdiDeviceName
    )
{
    NTSTATUS status;
    HANDLE hTdiDevice;

    status = PhAfdQueryTdiHandle(
        SocketHandle,
        QueryMode,
        &hTdiDevice
        );

    if (!NT_SUCCESS(status))
        return status;

    if (hTdiDevice == INVALID_HANDLE_VALUE)
    {
        *TdiDeviceName = PhCreateString(L"N/A");
    }
    else if (hTdiDevice == NULL)
    {
        *TdiDeviceName = PhCreateString(L"None");
    }
    else
    {
        status = PhGetObjectName(NtCurrentProcess(), hTdiDevice, TRUE, TdiDeviceName);
        NtClose(hTdiDevice);
    }

    return status;
}

/**
  * \brief Determines if we know how to handle a specific address family.
  *
  * \param[in] AddressFamily A socket address family value.
  *
  * \return Whether the address family is supported.
  */
BOOLEAN
NTAPI
PhpAfdIsSupportedAddressFamily(
    _In_ LONG AddressFamily
    )
{
    switch (AddressFamily)
    {
    case AF_INET:
    case AF_INET6:
    case AF_BTH:
    case AF_HYPERV:
        return TRUE;
    default:
        return FALSE;
    }
}

/**
  * \brief Retrieves an address associated with an AFD socket.
  *
  * \param[in] SocketHandle An AFD socket handle.
  * \param[in] Remote Whether the function should return a remote or a local address.
  * \param[out] Address Output buffer.
  *
  * \return Successful or errant status.
  */
NTSTATUS
NTAPI
PhAfdQueryAddress(
    _In_ HANDLE SocketHandle,
    _In_ BOOLEAN Remote,
    _Out_ PSOCKADDR_STORAGE Address
    )
{
    NTSTATUS status;
    AFD_ADDRESS buffer = { 0 };

    if (Remote)
    {
        // HACK: If the socket has a suitable state but no remote address, the IOCTL can succeed without
        // writing anything to the buffer, yet setting IO_STATUS_BLOCK's Information to a non-zero value.
        // We issue a zero-size query to recognize this scenario. (diversenok)

        if (NT_SUCCESS(PhAfdDeviceIoControl(SocketHandle, IOCTL_AFD_GET_REMOTE_ADDRESS, NULL, 0, NULL, 0, NULL, NULL)))
            return STATUS_NOT_FOUND;
    }

    // Retrieve the address
    status = PhAfdDeviceIoControl(
        SocketHandle,
        Remote ? IOCTL_AFD_GET_REMOTE_ADDRESS : IOCTL_AFD_GET_ADDRESS,
        NULL,
        0,
        &buffer,
        sizeof(buffer),
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    // Most sockets are TLI; their addresses don't need conversion.
    if (PhpAfdIsSupportedAddressFamily(buffer.TliAddress.ss_family))
    {
        *Address = buffer.TliAddress;
        return status;
    }

    // Some sockets (like Bluetooth) use TDI. Verify the header and extarct the socket address.
    if (buffer.TdiAddress.ActivityCount > 0 &&
        buffer.TdiAddress.Address.TAAddressCount >= 1 &&
        buffer.TdiAddress.Address.Address[0].AddressLength <= sizeof(buffer) - RTL_SIZEOF_THROUGH_FIELD(TDI_ADDRESS_INFO, Address.Address[0].AddressType) &&
        PhpAfdIsSupportedAddressFamily(buffer.TdiAddress.Address.Address[0].AddressType))
    {
        RtlZeroMemory(Address, sizeof(SOCKADDR_STORAGE));

        // AddressLength covers the length after the AddressType field, while the socket address starts at the AddressType field.
        // See comments in AFD_ADDRESS for details about the layout.
        RtlCopyMemory(
            Address,
            &buffer.TdiAddressUnpacked.EmbeddedAddress,
            buffer.TdiAddress.Address.Address[0].AddressLength + RTL_FIELD_SIZE(TDI_ADDRESS_INFO, Address.Address[0].AddressType)
            );

        return status;
    }

    return STATUS_UNKNOWN_REVISION;
}

/**
  * \brief Formats a socket address as a strings.
  *
  * \param[in] Address The address buffer.
  * \param[out] AddressString The string representation of the address.
  * \param[in] Simplify A boolean indicating whether the function should prefer a human-readable or a complete version of the address.
  *
  * \return Successful or errant status.
  */
NTSTATUS
NTAPI
PhAfdFormatAddress(
    _In_ PSOCKADDR_STORAGE Address,
    _Out_ PPH_STRING *AddressString,
    _In_ BOOLEAN Simplify
    )
{
    NTSTATUS status = STATUS_NOT_SUPPORTED;
    WCHAR buffer[70];
    ULONG characters = RTL_NUMBER_OF(buffer);

    if (Address->ss_family == AF_INET)
    {
        PSOCKADDR_IN address = (PSOCKADDR_IN)Address;

        // Format an IPv4 address
        status = RtlIpv4AddressToStringExW(
            &address->sin_addr,
            address->sin_port,
            buffer,
            &characters
            );

        if (!NT_SUCCESS(status))
            return status;

        *AddressString = PhCreateStringEx(buffer, characters * sizeof(WCHAR) - sizeof(UNICODE_NULL));
    }
    else if (Address->ss_family == AF_INET6)
    {
        PSOCKADDR_IN6 address = (PSOCKADDR_IN6)Address;

        // Format an IPv6 address
        status = RtlIpv6AddressToStringExW(
            &address->sin6_addr,
            address->sin6_scope_id,
            address->sin6_port,
            buffer,
            &characters
            );

        if (!NT_SUCCESS(status))
            return status;

        *AddressString = PhCreateStringEx(buffer, characters * sizeof(WCHAR) - sizeof(UNICODE_NULL));
    }
    else if (Address->ss_family == AF_BTH)
    {
        PSOCKADDR_BTH address = (PSOCKADDR_BTH)Address;

        // Format a Bluetooth address
        *AddressString = PhFormatString(
            L"(%02X:%02X:%02X:%02X:%02X:%02X):%d",
            (UCHAR)(address->btAddr >> 40),
            (UCHAR)(address->btAddr >> 32),
            (UCHAR)(address->btAddr >> 24),
            (UCHAR)(address->btAddr >> 16),
            (UCHAR)(address->btAddr >> 8),
            (UCHAR)(address->btAddr),
            address->port
            );

        status = STATUS_SUCCESS;
    }
    else if (Address->ss_family == AF_HYPERV)
    {
        PSOCKADDR_HV address = (PSOCKADDR_HV)Address;
        PCWSTR knownVmId = NULL;
        PPH_STRING vmIdPart = NULL;
        PPH_STRING serviceIdPart;

        if (Simplify)
        {
            // Recognize placeholder VmId values
            if (IsEqualGUID(&address->VmId, &HV_GUID_WILDCARD))
                knownVmId = L"{Wildcard}";
            else if (IsEqualGUID(&address->VmId, &HV_GUID_BROADCAST))
                knownVmId = L"{Broadcast}";
            else if (IsEqualGUID(&address->VmId, &HV_GUID_CHILDREN))
                knownVmId = L"{Children}";
            else if (IsEqualGUID(&address->VmId, &HV_GUID_LOOPBACK))
                knownVmId = L"{Loopback}";
            else if (IsEqualGUID(&address->VmId, &HV_GUID_PARENT))
                knownVmId = L"{Parent}";
            else if (IsEqualGUID(&address->VmId, &HV_GUID_SILOHOST))
                knownVmId = L"{Silo host}";
        }

        if (!knownVmId)
        {
            // Fall back to using a GUID name
            vmIdPart = PhFormatGuid(&address->VmId);
        }

        serviceIdPart = PhFormatGuid(&address->ServiceId);

        // Format a Hyper-V address as {VmId}:{ServiceId}
        *AddressString = PhFormatString(
            L"%s:%s",
            vmIdPart ? vmIdPart->Buffer : knownVmId,
            serviceIdPart->Buffer
            );

        if (vmIdPart)
            PhDereferenceObject(vmIdPart);

        PhDereferenceObject(serviceIdPart);

        status = STATUS_SUCCESS;
    }

    return status;
}

/**
  * \brief Queries an address associated with an AFD socket and prints it into a string.
  *
  * \param[in] SocketHandle An AFD socket handle.
  * \param[in] Remote Whether the function should return a remote or a local address.
  * \param[out] AddressString The string representation of the address.
  * \param[in] Simplify A boolean indicating whether the function should prefer a human-readable or a complete version of the address.
  *
  * \return Successful or errant status.
  */
NTSTATUS
NTAPI
PhAfdQueryFormatAddress(
    _In_ HANDLE SocketHandle,
    _In_ BOOLEAN Remote,
    _Out_ PPH_STRING *AddressString,
    _In_ BOOLEAN Simplify
    )
{
    NTSTATUS status;
    SOCKADDR_STORAGE address;

    status = PhAfdQueryAddress(SocketHandle, Remote, &address);

    if (!NT_SUCCESS(status))
        return status;

    return PhAfdFormatAddress(&address, AddressString, Simplify);
}

/**
  * \brief Looks up a human-readable name of a known socket state.
  *
  * \param[in] SocketState The socket state value.
  *
  * \return A string with the state name or NULL when the state value is not recognized.
  */
_Maybenull_
PCWSTR
NTAPI
PhpAfdGetSocketStateString(
    _In_ SOCKET_STATE SocketState
    )
{
    switch (SocketState)
    {
    case SocketStateInitializing:
        return L"Initializing";
    case SocketStateOpen:
        return L"Open";
    case SocketStateBound:
        return L"Bound";
    case SocketStateBoundSpecific:
        return L"Bound (specific)";
    case SocketStateConnected:
        return L"Connected";
    case SocketStateClosing:
        return L"Closing";
    default:
        return NULL;
    }
}

/**
  * \brief Formats a socket state as a string.
  *
  * \param[in] SocketState The socket state value.
  *
  * \return A human-readable name of the socket state.
  */
PPH_STRING
NTAPI
PhAfdFormatSocketState(
    _In_ SOCKET_STATE SocketState
    )
{
    PCWSTR knownName = PhpAfdGetSocketStateString(SocketState);

    if (knownName)
        return PhCreateString(knownName);
    else
        return PhFormatString(L"Invalid state (%d)", SocketState);
}

/**
  * \brief Looks up a human-readable name of a known socket type.
  *
  * \param[in] SocketType The socket type value.
  *
  * \return A string with the socket type name or NULL when the value is not recognized.
  */
_Maybenull_
PCWSTR
NTAPI
PhpAfdGetSocketTypeString(
    _In_ LONG SocketType
    )
{
    switch (SocketType)
    {
    case SOCK_STREAM:
        return L"Stream";
    case SOCK_DGRAM:
        return L"Datagram";
    case SOCK_RAW:
        return L"Raw";
    case SOCK_RDM:
        return L"Reliably-delivered message";
    case SOCK_SEQPACKET:
        return L"Pseudo-stream";
    default:
        return NULL;
    }
}

/**
  * \brief Formats a socket type as a string.
  *
  * \param[in] SocketType The socket type value.
  *
  * \return A human-readable name for the socket type.
  */
PPH_STRING
NTAPI
PhAfdFormatSocketType(
    _In_ LONG SocketType
    )
{
    PCWSTR knownName = PhpAfdGetSocketTypeString(SocketType);

    if (knownName)
        return PhCreateString(knownName);
    else
        return PhFormatString(L"Unknown (%d)", SocketType);
}

/**
  * \brief Formats a human-readable name for a set of protocol provider flags.
  *
  * \param[in] ProviderFlags The provider flags value.
  *
  * \return A string with the flag names.
  */
PPH_STRING
NTAPI
PhAfdFormatProviderFlags(
    _In_ ULONG ProviderFlags
    )
{
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 60);
    PhAppendFormatStringBuilder(&stringBuilder, L"0x%04X (", ProviderFlags);

    if (ProviderFlags & PFL_MULTIPLE_PROTO_ENTRIES)
        PhAppendStringBuilder2(&stringBuilder, L"Multiple entries, ");
    if (ProviderFlags & PFL_RECOMMENDED_PROTO_ENTRY)
        PhAppendStringBuilder2(&stringBuilder, L"Recommended entry, ");
    if (ProviderFlags & PFL_HIDDEN)
        PhAppendStringBuilder2(&stringBuilder, L"Hidden, ");
    if (ProviderFlags & PFL_MATCHES_PROTOCOL_ZERO)
        PhAppendStringBuilder2(&stringBuilder, L"Matches protocol zero, ");
    if (ProviderFlags & PFL_NETWORKDIRECT_PROVIDER)
        PhAppendStringBuilder2(&stringBuilder, L"Network direct, ");

    if (!ProviderFlags)
    {
        PhAppendStringBuilder2(&stringBuilder, L"none");
    }
    else
    {
        // Remove the trailing comma
        PhRemoveEndStringBuilder(&stringBuilder, 2);
    }

    PhAppendCharStringBuilder(&stringBuilder, L')');
    return PhFinalStringBuilderString(&stringBuilder);
}

/**
  * \brief Formats a human-readable name for a set of service flags.
  *
  * \param[in] ServiceFlags The service flags value.
  *
  * \return A string with the flag names.
  */
PPH_STRING
NTAPI
PhAfdFormatServiceFlags(
    _In_ ULONG ServiceFlags
    )
{
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 0x100);
    PhAppendFormatStringBuilder(&stringBuilder, L"0x%04X (", ServiceFlags);

    if (ServiceFlags & XP1_CONNECTIONLESS)
        PhAppendStringBuilder2(&stringBuilder, L"Connectionless, ");
    if (ServiceFlags & XP1_GUARANTEED_DELIVERY)
        PhAppendStringBuilder2(&stringBuilder, L"Guaranteed delivery, ");
    if (ServiceFlags & XP1_GUARANTEED_ORDER)
        PhAppendStringBuilder2(&stringBuilder, L"Guaranteed order, ");
    if (ServiceFlags & XP1_MESSAGE_ORIENTED)
        PhAppendStringBuilder2(&stringBuilder, L"Message-oriented, ");
    if (ServiceFlags & XP1_PSEUDO_STREAM)
        PhAppendStringBuilder2(&stringBuilder, L"Pseudo-stream, ");
    if (ServiceFlags & XP1_GRACEFUL_CLOSE)
        PhAppendStringBuilder2(&stringBuilder, L"Graceful close, ");
    if (ServiceFlags & XP1_EXPEDITED_DATA)
        PhAppendStringBuilder2(&stringBuilder, L"Expedited data, ");
    if (ServiceFlags & XP1_CONNECT_DATA)
        PhAppendStringBuilder2(&stringBuilder, L"Connect data, ");
    if (ServiceFlags & XP1_DISCONNECT_DATA)
        PhAppendStringBuilder2(&stringBuilder, L"Disconnect data, ");
    if (ServiceFlags & XP1_SUPPORT_BROADCAST)
        PhAppendStringBuilder2(&stringBuilder, L"Broadcast, ");
    if (ServiceFlags & XP1_SUPPORT_MULTIPOINT)
        PhAppendStringBuilder2(&stringBuilder, L"Support multipoint, ");
    if (ServiceFlags & XP1_MULTIPOINT_CONTROL_PLANE)
        PhAppendStringBuilder2(&stringBuilder, L"Multipoint control plane, ");
    if (ServiceFlags & XP1_MULTIPOINT_DATA_PLANE)
        PhAppendStringBuilder2(&stringBuilder, L"Multipoint data plane, ");
    if (ServiceFlags & XP1_QOS_SUPPORTED)
        PhAppendStringBuilder2(&stringBuilder, L"QoS supported, ");
    if (ServiceFlags & XP1_INTERRUPT)
        PhAppendStringBuilder2(&stringBuilder, L"Interrupt, ");
    if (ServiceFlags & XP1_UNI_SEND)
        PhAppendStringBuilder2(&stringBuilder, L"Unidirectional send, ");
    if (ServiceFlags & XP1_UNI_RECV)
        PhAppendStringBuilder2(&stringBuilder, L"Unidirectional receive, ");
    if (ServiceFlags & XP1_IFS_HANDLES)
        PhAppendStringBuilder2(&stringBuilder, L"IFS handles, ");
    if (ServiceFlags & XP1_PARTIAL_MESSAGE)
        PhAppendStringBuilder2(&stringBuilder, L"Partial message, ");
    if (ServiceFlags & XP1_SAN_SUPPORT_SDP)
        PhAppendStringBuilder2(&stringBuilder, L"SAN, ");

    if (!ServiceFlags)
    {
        PhAppendStringBuilder2(&stringBuilder, L"none");
    }
    else
    {
        // Remove the trailing comma
        PhRemoveEndStringBuilder(&stringBuilder, 2);
    }

    PhAppendCharStringBuilder(&stringBuilder, L')');
    return PhFinalStringBuilderString(&stringBuilder);
}

/**
  * \brief Formats a human-readable name for a set of socket creation flags.
  *
  * \param[in] CreationFlags The creation flags value.
  *
  * \return A string with the flag names.
  */
PPH_STRING
NTAPI
PhAfdFormatCreationFlags(
    _In_ ULONG CreationFlags
    )
{
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 60);
    PhAppendFormatStringBuilder(&stringBuilder, L"0x%04X (", CreationFlags);

    if (CreationFlags & WSA_FLAG_OVERLAPPED)
        PhAppendStringBuilder2(&stringBuilder, L"Overlapped, ");
    if (CreationFlags & WSA_FLAG_MULTIPOINT_C_ROOT)
        PhAppendStringBuilder2(&stringBuilder, L"Multipoint control root, ");
    if (CreationFlags & WSA_FLAG_MULTIPOINT_C_LEAF)
        PhAppendStringBuilder2(&stringBuilder, L"Multipoint control leaf, ");
    if (CreationFlags & WSA_FLAG_MULTIPOINT_D_ROOT)
        PhAppendStringBuilder2(&stringBuilder, L"Multipoint data root, ");
    if (CreationFlags & WSA_FLAG_MULTIPOINT_D_LEAF)
        PhAppendStringBuilder2(&stringBuilder, L"Multipoint data leaf, ");
    if (CreationFlags & WSA_FLAG_ACCESS_SYSTEM_SECURITY)
        PhAppendStringBuilder2(&stringBuilder, L"Access SACL, ");
    if (CreationFlags & WSA_FLAG_NO_HANDLE_INHERIT)
        PhAppendStringBuilder2(&stringBuilder, L"No handle inherit, ");
    if (CreationFlags & WSA_FLAG_REGISTERED_IO)
        PhAppendStringBuilder2(&stringBuilder, L"Registered I/O, ");

    if (!CreationFlags)
    {
        PhAppendStringBuilder2(&stringBuilder, L"none");
    }
    else
    {
        // Remove the trailing comma
        PhRemoveEndStringBuilder(&stringBuilder, 2);
    }

    PhAppendCharStringBuilder(&stringBuilder, L')');
    return PhFinalStringBuilderString(&stringBuilder);
}

/**
  * \brief Formats a human-readable name for a set of AFD socket flags.
  *
  * \param[in] SharedInfo The shared socket information buffer.
  *
  * \return A string with the flag names.
  */
PPH_STRING
NTAPI
PhAfdFormatSharedInfoFlags(
    _In_ PSOCK_SHARED_INFO SharedInfo
    )
{
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 80);
    PhAppendFormatStringBuilder(&stringBuilder, L"0x%04X (", SharedInfo->Flags);

    if (SharedInfo->Listening)
        PhAppendStringBuilder2(&stringBuilder, L"Listening, ");
    if (SharedInfo->Broadcast)
        PhAppendStringBuilder2(&stringBuilder, L"Broadcast, ");
    if (SharedInfo->Debug)
        PhAppendStringBuilder2(&stringBuilder, L"Debug, ");
    if (SharedInfo->OobInline)
        PhAppendStringBuilder2(&stringBuilder, L"OOB in line, ");
    if (SharedInfo->ReuseAddresses)
        PhAppendStringBuilder2(&stringBuilder, L"Reuse addresses, ");
    if (SharedInfo->ExclusiveAddressUse)
        PhAppendStringBuilder2(&stringBuilder, L"Exclusive address use, ");
    if (SharedInfo->NonBlocking)
        PhAppendStringBuilder2(&stringBuilder, L"Non-blocking, ");
    if (SharedInfo->DontUseWildcard)
        PhAppendStringBuilder2(&stringBuilder, L"Don't use wildcard, ");
    if (SharedInfo->ReceiveShutdown)
        PhAppendStringBuilder2(&stringBuilder, L"Receive shutdown, ");
    if (SharedInfo->SendShutdown)
        PhAppendStringBuilder2(&stringBuilder, L"Send shutdown, ");
    if (SharedInfo->ConditionalAccept)
        PhAppendStringBuilder2(&stringBuilder, L"Conditional accept, ");
    if (SharedInfo->IsSANSocket)
        PhAppendStringBuilder2(&stringBuilder, L"SAN, ");
    if (SharedInfo->fIsTLI)
        PhAppendStringBuilder2(&stringBuilder, L"TLI, ");
    if (SharedInfo->Rio)
        PhAppendStringBuilder2(&stringBuilder, L"RIO, ");
    if (SharedInfo->ReceiveBufferSizeSet)
        PhAppendStringBuilder2(&stringBuilder, L"Receive buffer size set, ");
    if (SharedInfo->SendBufferSizeSet)
        PhAppendStringBuilder2(&stringBuilder, L"Send buffer size set, ");

    if (!SharedInfo->Flags)
    {
        PhAppendStringBuilder2(&stringBuilder, L"none");
    }
    else
    {
        // Remove the trailing comma
        PhRemoveEndStringBuilder(&stringBuilder, 2);
    }

    PhAppendCharStringBuilder(&stringBuilder, L')');
    return PhFinalStringBuilderString(&stringBuilder);
}

/**
  * \brief Looks up a name of a known address family.
  *
  * \param[in] AddressFamily The address family value.
  *
  * \return A string with the address family name or NULL when it is not recognized.
  */
_Maybenull_
PCWSTR
NTAPI
PhpAfdGetAddressFamilyString(
    _In_ LONG AddressFamily
    )
{
    switch (AddressFamily)
    {
    case AF_UNSPEC:
        return L"Unspecified";
    case AF_INET:
        return L"Internet";
    case AF_INET6:
        return L"Internet v6";
    case AF_BTH:
        return L"Bluetooth";
    case AF_HYPERV:
        return L"Hyper-V";
    default:
        return NULL;
    }
}

/**
  * \brief Formats an address family as a string.
  *
  * \param[in] AddressFamily The address family value.
  *
  * \return A string with the address family name.
  */
PPH_STRING
NTAPI
PhAfdFormatAddressFamily(
    _In_ LONG AddressFamily
    )
{
    PCWSTR knownName = PhpAfdGetAddressFamilyString(AddressFamily);

    if (knownName)
        return PhCreateString(knownName);
    else
        return PhFormatString(L"Unrecognized address family %d", AddressFamily);
}

/**
  * \brief Looks up a name for a known protocol.
  *
  * \param[in] AddressFamily The address family of the protocol.
  * \param[in] Protocol The protocol value.
  *
  * \return A string with the protocol name or NULL when it is not recognized.
  */
_Maybenull_
PCWSTR
NTAPI
PhpAfdGetProtocolString(
    _In_ LONG AddressFamily,
    _In_ LONG Protocol
    )
{
    switch (AddressFamily)
    {
    case AF_INET:
    case AF_INET6:
        switch (Protocol)
        {
        case IPPROTO_ICMP:
            return L"ICMP";
        case IPPROTO_IGMP:
            return L"IGMP";
        case IPPROTO_TCP:
            return L"TCP";
        case IPPROTO_UDP:
            return L"UDP";
        case IPPROTO_RDP:
            return L"RDP";
        case IPPROTO_ICMPV6:
            return L"ICMPv6";
        case IPPROTO_PGM:
            return L"PGM";
        case IPPROTO_L2TP:
            return L"L2TP";
        case IPPROTO_SCTP:
            return L"SCTP";
        case IPPROTO_RAW:
            return L"RAW";
        case IPPROTO_RESERVED_IPSEC:
            return L"IPSec";
        }
    case AF_BTH:
        switch (Protocol)
        {
        case BTHPROTO_RFCOMM:
            return L"RFCOMM";
        case BTHPROTO_L2CAP:
            return L"L2CAP";
        }
    case AF_HYPERV:
        switch (Protocol)
        {
        case HV_PROTOCOL_RAW:
            return L"RAW";
        }
    }

    return NULL;
}

/**
  * \brief Formats a protocol name as a string.
  *
  * \param[in] AddressFamily The address family of the protocol.
  * \param[in] Protocol The protocol value.
  *
  * \return A string with a human-readable protocol name.
  */
PPH_STRING
NTAPI
PhAfdFormatProtocol(
    _In_ LONG AddressFamily,
    _In_ LONG Protocol
    )
{
    PCWSTR knownName = PhpAfdGetProtocolString(AddressFamily, Protocol);

    if (knownName)
        return PhCreateString(knownName);
    else
        return PhFormatString(L"Unrecognized protocol %d", Protocol);
}

/**
  * \brief Looks up a short human-readable identifier for a known protocol.
  *
  * \param[in] AddressFamily The address family of the protocol.
  * \param[in] Protocol The protocol value.
  *
  * \return A string with the protocol name or NULL when it is not recognized.
  */
_Maybenull_
PCWSTR
NTAPI
PhpAfdGetProtocolSummary(
    _In_ LONG AddressFamily,
    _In_ LONG Protocol
    )
{
    switch (AddressFamily)
    {
    case AF_INET:
        switch (Protocol)
        {
        case IPPROTO_ICMP:
            return L"ICMP";
        case IPPROTO_TCP:
            return L"TCP";
        case IPPROTO_UDP:
            return L"UDP";
        case IPPROTO_RAW:
            return L"RAW/IPv4";
        }
    case AF_INET6:
        switch (Protocol)
        {
        case IPPROTO_ICMPV6:
            return L"ICMP6";
        case IPPROTO_TCP:
            return L"TCP6";
        case IPPROTO_UDP:
            return L"UDP6";
        case IPPROTO_RAW:
            return L"RAW/IPv6";
        }
    case AF_BTH:
        switch (Protocol)
        {
        case BTHPROTO_RFCOMM:
            return L"RFCOMM [Bluetooth]";
        case BTHPROTO_L2CAP:
            return L"L2CAP [Bluetooth]";
        }
    case AF_HYPERV:
        switch (Protocol)
        {
        case HV_PROTOCOL_RAW:
            return L"Hyper-V RAW";
        }
    }

    return NULL;
}

/**
  * \brief Formats a human-readable name for an AFD socket.
  *
  * \param[in] SocketHandle An AFD socket handle.
  *
  * \return The best handle name representation available on success, or NULL on error.
  */
_Maybenull_
PPH_STRING
NTAPI
PhAfdFormatSocketBestName(
    _In_ HANDLE SocketHandle
    )
{
    PH_STRING_BUILDER stringBuilder;
    SOCK_SHARED_INFO sharedInfo;
    PPH_STRING addressString;
    BOOLEAN sharedInfoAvailable;
    BOOLEAN addressAvailable;

    // Query socket information
    sharedInfoAvailable = NT_SUCCESS(PhAfdQuerySharedInfo(SocketHandle, &sharedInfo));
    addressAvailable = NT_SUCCESS(PhAfdQueryFormatAddress(SocketHandle, FALSE, &addressString, TRUE));

    if (!sharedInfoAvailable && !addressAvailable)
        return NULL;

    PhInitializeStringBuilder(&stringBuilder, 0x100);
    PhAppendStringBuilder2(&stringBuilder, L"AFD socket: ");

    if (sharedInfoAvailable)
    {
        PCWSTR detail;

        // State
        if (detail = PhpAfdGetSocketStateString(sharedInfo.State))
        {
            PhAppendStringBuilder2(&stringBuilder, detail);
            PhAppendStringBuilder2(&stringBuilder, L" ");
        }

        // Protocol
        if (detail = PhpAfdGetProtocolSummary(sharedInfo.AddressFamily, sharedInfo.Protocol))
        {
            PhAppendStringBuilder2(&stringBuilder, detail);
            PhAppendStringBuilder2(&stringBuilder, L" ");
        }
    }

    if (addressAvailable)
    {
        PPH_STRING remoteAddressString;

        // Local address
        PhAppendStringBuilder2(&stringBuilder, L"on ");
        PhAppendStringBuilder(&stringBuilder, &addressString->sr);

        // Remote address
        if (NT_SUCCESS(PhAfdQueryFormatAddress(SocketHandle, TRUE, &remoteAddressString, TRUE)))
        {
            PhAppendStringBuilder2(&stringBuilder, L" --> ");
            PhAppendStringBuilder(&stringBuilder, &remoteAddressString->sr);
            PhDereferenceObject(remoteAddressString);
        }

        PhDereferenceObject(addressString);
    }

    return PhFinalStringBuilderString(&stringBuilder);
}
