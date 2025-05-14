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

#include <ph.h>
#include <phafd.h>
#include <phnet.h>
#include <hndlinfo.h>
#include <ws2ipdef.h>
#include <ws2bth.h>
#include <afunix.h>
#include <hvsocketcontrol.h>
#include <secedit.h>

#include <mstcpip.h>

/**
 * Determines if an object name represents an AFD socket handle.
 *
 * \param[in] ObjectName A native object name.
 * \return Whether the name matches an AFD socket name format.
 */
BOOLEAN PhAfdIsSocketObjectName(
    _In_opt_ PPH_STRING ObjectName
    )
{
    static CONST PH_STRINGREF deviceName = PH_STRINGREF_INIT(AFD_DEVICE_NAME);

    if (ObjectName && PhStartsWithStringRef(&ObjectName->sr, &deviceName, TRUE))
    {
        // N.B. Check explicitly for "\\Device\\Afd" or "\\Device\\Afd\\" to avoid "\\Device\\AfdABC"

        if (ObjectName->Length == deviceName.Length)
            return TRUE;

        assert(ObjectName->Length > deviceName.Length);

        if (ObjectName->Buffer[deviceName.Length / sizeof(WCHAR)] == OBJ_NAME_PATH_SEPARATOR)
            return TRUE;
    }

    return FALSE;
}

/**
 * Determines if a file handle is an AFD socket handle.
 *
 * \param[in] Handle A file handle.
 * \return A successful status if the handle is an AFD socket or an errant status otherwise.
 */
NTSTATUS PhAfdIsSocketHandle(
    _In_ HANDLE Handle
    )
{
    static CONST PH_STRINGREF deviceName = PH_STRINGREF_INIT(AFD_DEVICE_NAME);
    NTSTATUS status;
    PH_STRINGREF volumeName;
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

    volumeName.Buffer = Buffer.VolumeName.DeviceName;
    volumeName.Length = Buffer.VolumeName.DeviceNameLength;

    // Compare the file's device name to AFD
    return PhEqualStringRef(&volumeName, &deviceName, TRUE) ? STATUS_SUCCESS : STATUS_NOT_SAME_DEVICE;
}

/**
 * Issues an IOCTL on an AFD handle and waits for its completion.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \param[in] IoControlCode I/O control code
 * \param[in] InBuffer Input buffer.
 * \param[in] InBufferSize Input buffer size.
 * \param[out] OutputBuffer Output Buffer.
 * \param[in] OutputBufferSize Output buffer size.
 * \param[out] BytesReturned Optionally set to the number of bytes returned.
 * \param[in,out] Overlapped Optional overlapped structure.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdDeviceIoControl(
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
    HANDLE eventHandle;
    IO_STATUS_BLOCK ioStatusBlock;

    if (BytesReturned)
    {
        *BytesReturned = 0;
    }

    if (Overlapped)
    {
        eventHandle = Overlapped->hEvent;
        Overlapped->Internal = STATUS_PENDING;
    }
    else
    {
        // We cannot wait on the file handle because it might not grant SYNCHRONIZE access.
        // Always use an event instead.

        status = PhCreateEvent(
            &eventHandle,
            EVENT_ALL_ACCESS,
            SynchronizationEvent,
            FALSE
            );

        if (!NT_SUCCESS(status))
            return status;
    }

    status = NtDeviceIoControlFile(
        SocketHandle,
        eventHandle,
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
            NtWaitForSingleObject(eventHandle, FALSE, NULL);
            status = ioStatusBlock.Status;
        }

        NtClose(eventHandle);

        if (BytesReturned)
        {
            *BytesReturned = (ULONG)ioStatusBlock.Information;
        }
    }

    return status;
}

/**
 * Retrieves shared information for an AFD socket.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \param[out] SharedInfo A buffer with the shared socket information.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhAfdQuerySharedInfo(
    _In_ HANDLE SocketHandle,
    _Out_ PSOCK_SHARED_INFO SharedInfo
    )
{
    NTSTATUS status;
    ULONG returnLength;

    status = PhAfdDeviceIoControl(
        SocketHandle,
        IOCTL_AFD_GET_CONTEXT,
        NULL,
        0,
        SharedInfo,
        sizeof(SOCK_SHARED_INFO),
        &returnLength,
        NULL
        );

    if (status == STATUS_BUFFER_OVERFLOW)
        return STATUS_SUCCESS;

    if (!NT_SUCCESS(status))
        return status;

    // Shared information is provided by the Win32 level; do a sanity check on the returned size
    return returnLength < sizeof(SOCK_SHARED_INFO) ? STATUS_NOT_FOUND : status;
}

/**
 * Retrieves simple information for an AFD socket.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \param[in] InformationType The type of information to query, such as AFD_CONNECT_TIME or AFD_GROUP_ID_AND_TYPE.
 * \param[out] Information Output buffer.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdQuerySimpleInfo(
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
 * Retrieves socket option for an AFD socket.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \param[in] Level A level for the option, such as SOL_SOCKET or IPPROTO_IP.
 * \param[in] OptionName An option identifier within the level, such as SO_REUSEADDR or IP_TTL.
 * \param[out] OptionValue A buffer that receives the option value.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdQueryOption(
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
 * Retrieves the latest supported TCP_INFO for an AFD socket.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \param[out] TcpInfo A buffer that receives the TCP information.
 * \param[out] TcpInfoVersion The version of the returned structure.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdQueryTcpInfo(
    _In_ HANDLE SocketHandle,
    _Out_ PTCP_INFO_v2 TcpInfo,
    _Out_ PULONG TcpInfoVersion
    )
{
    static CONST ULONG tcpInfoSize[] =
    {
        sizeof(TCP_INFO_v0),
        sizeof(TCP_INFO_v1),
        sizeof(TCP_INFO_v2)
    };
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    AFD_TL_IO_CONTROL_INFO controlInfo = { 0 };
    LONG version;

    controlInfo.Type = TlSocketIoControlType;
    controlInfo.EndpointIoctl = TRUE;
    controlInfo.IoControlCode = SIO_TCP_INFO;
    controlInfo.InputBuffer = &version;
    controlInfo.InputBufferLength = sizeof(LONG);

    RtlZeroMemory(TcpInfo, sizeof(TCP_INFO_v2));

    for (version = 2; version >= 0; version--)
    {
        status = PhAfdDeviceIoControl(
            SocketHandle,
            IOCTL_AFD_TRANSPORT_IOCTL,
            &controlInfo,
            sizeof(AFD_TL_IO_CONTROL_INFO),
            TcpInfo,
            tcpInfoSize[version],
            NULL,
            NULL
            );

        if (NT_SUCCESS(status))
        {
            *TcpInfoVersion = version;
            break;
        }
    }

    return status;
}

/**
 * Opens an address or a connection handle to the underlying device for a TDI socket.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \param[in] QueryMode A type of the query, either AFD_QUERY_ADDRESS_HANDLE or AFD_QUERY_CONNECTION_HANDLE.
 * \param[out] TdiHandle A pointer to a variable that receives a TDI device handle, NULL (when no handle is available), or INVALID_HANDLE_VALUE (for non-TDI sockets).
 * \return Successful or errant status.
 */
NTSTATUS PhAfdQueryTdiHandle(
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
 * Formats a TDI device name for a socket.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \param[in] QueryMode A type of the query, either AFD_QUERY_ADDRESS_HANDLE or AFD_QUERY_CONNECTION_HANDLE.
 * \param[out] TdiDeviceName A pointer to a variable that receives a device name string.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdQueryFormatTdiDeviceName(
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
        *TdiDeviceName = PhCreateString(L"N/A (transport is not TDI)");
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
 * Determines if we know how to handle a specific address family.
 *
 * \param[in] AddressFamily A socket address family value.
 * \return Whether the address family is supported.
 */
BOOLEAN PhpAfdIsSupportedAddressFamily(
    _In_ LONG AddressFamily
    )
{
    switch (AddressFamily)
    {
    case AF_UNIX:
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
 * Retrieves an address associated with an AFD socket.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \param[in] Remote Whether the function should return a remote or a local address.
 * \param[out] Address Output buffer.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdQueryAddress(
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
 * Formats a socket address as a strings.
 *
 * \param[in] Address The address buffer.
 * \param[out] AddressString The string representation of the address.
 * \param[in] Flags A bit mask of PH_AFD_ADDRESS_* flags that control address formatting.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdFormatAddress(
    _In_ PSOCKADDR_STORAGE Address,
    _Out_ PPH_STRING *AddressString,
    _In_ ULONG Flags
    )
{
    NTSTATUS status = STATUS_NOT_SUPPORTED;
    WCHAR buffer[70];
    ULONG characters = RTL_NUMBER_OF(buffer);

    if (Address->ss_family == AF_UNIX)
    {
        PSOCKADDR_UN address = (PSOCKADDR_UN)Address;

        *AddressString = PhConvertUtf8ToUtf16Ex(address->sun_path, strnlen(address->sun_path, UNIX_PATH_MAX));

        if (!(*AddressString))
            return STATUS_SOME_NOT_MAPPED;

        status = STATUS_SUCCESS;
    }
    else if (Address->ss_family == AF_INET)
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

        if (Flags & PH_AFD_ADDRESS_SIMPLIFY)
        {
            PH_FORMAT format[5];
            ULONG count = 0;
            PPH_STRING serviceString = NULL;
            PPH_STRING vmName = NULL;

            if (PhHvSocketIsVSockTemplate(&address->ServiceId))
            {
                PhInitFormatS(&format[count++], L"VSOCK:");
                PhInitFormatU(&format[count++], address->ServiceId.Data1);
            }
            else if (serviceString = PhHvSocketGetServiceName(&address->ServiceId))
            {
                PhInitFormatSR(&format[count++], serviceString->sr);
            }
            else
            {
                serviceString = PhFormatGuid(&address->ServiceId);
                PhInitFormatSR(&format[count++], serviceString->sr);
            }

            if (!IsEqualGUID(&address->VmId, &HV_GUID_WILDCARD))
            {
                PhInitFormatS(&format[count++], L" (VM: ");

                if (vmName = PhHvSocketGetVmName(&address->VmId))
                {
                    PhInitFormatSR(&format[count++], vmName->sr);
                }
                else
                {
                    vmName = PhHvSocketAddressString(&address->VmId);
                    PhInitFormatSR(&format[count++], vmName->sr);
                }

                PhInitFormatS(&format[count++], L")");
            }

            *AddressString = PhFormat(format, count, 80);

            PhClearReference(&serviceString);
            PhClearReference(&vmName);
        }
        else
        {
            PPH_STRING vmIdPart;
            PPH_STRING serviceIdPart;
            PH_FORMAT format[3];

            vmIdPart = PhFormatGuid(&address->VmId);
            serviceIdPart = PhFormatGuid(&address->ServiceId);

            PhInitFormatSR(&format[0], vmIdPart->sr);
            PhInitFormatC(&format[1], L':');
            PhInitFormatSR(&format[2], serviceIdPart->sr);

            *AddressString = PhFormat(format, 3, 80);

            PhDereferenceObject(vmIdPart);
            PhDereferenceObject(serviceIdPart);
        }

        status = STATUS_SUCCESS;
    }

    return status;
}

/**
 * Queries an address associated with an AFD socket and prints it into a string.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \param[in] Remote Whether the function should return a remote or a local address.
 * \param[out] AddressString The string representation of the address.
 * \param[in] Flags A bit mask of PH_AFD_ADDRESS_* flags that control address formatting.
 * Successful or errant status.
 */
NTSTATUS PhAfdQueryFormatAddress(
    _In_ HANDLE SocketHandle,
    _In_ BOOLEAN Remote,
    _Out_ PPH_STRING *AddressString,
    _In_ ULONG Flags
    )
{
    NTSTATUS status;
    SOCKADDR_STORAGE address;

    status = PhAfdQueryAddress(SocketHandle, Remote, &address);

    if (!NT_SUCCESS(status))
        return status;

    return PhAfdFormatAddress(&address, AddressString, Flags);
}

/**
 * Looks up a human-readable name of a known socket state.
 *
 * \param[in] SocketState The socket state value.
 * \return A string with the state name or NULL when the state value is not recognized.
 */
_Maybenull_
PCWSTR PhpAfdGetSocketStateString(
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
 * Formats a socket state as a string.
 *
 * \param[in] SocketState The socket state value.
 * \return A human-readable name of the socket state.
 */
PPH_STRING PhAfdFormatSocketState(
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
 * Looks up a human-readable name of a known socket type.
 *
 * \param[in] SocketType The socket type value.
 * \return A string with the socket type name or NULL when the value is not recognized.
 */
_Maybenull_
PCWSTR PhpAfdGetSocketTypeString(
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
 * Formats a socket type as a string.
 *
 * \param[in] SocketType The socket type value.
 * \return A human-readable name for the socket type.
 */
PPH_STRING PhAfdFormatSocketType(
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
 * Formats a human-readable name for a set of protocol provider flags.
 *
 * \param[in] ProviderFlags The provider flags value.
 * \return A string with the flag names.
 */
PPH_STRING PhAfdFormatProviderFlags(
    _In_ ULONG ProviderFlags
    )
{
#define PH_AFD_PROVIDER_FLAG(x, n) { TEXT(#x), x, FALSE, FALSE, n }

    static const PH_ACCESS_ENTRY providerFlags[] =
    {
        PH_AFD_PROVIDER_FLAG(PFL_MULTIPLE_PROTO_ENTRIES, L"Multiple entries"),
        PH_AFD_PROVIDER_FLAG(PFL_RECOMMENDED_PROTO_ENTRY, L"Recommended entry"),
        PH_AFD_PROVIDER_FLAG(PFL_HIDDEN, L"Hidden"),
        PH_AFD_PROVIDER_FLAG(PFL_MATCHES_PROTOCOL_ZERO, L"Matches protocol zero"),
        PH_AFD_PROVIDER_FLAG(PFL_NETWORKDIRECT_PROVIDER, L"Network direct"),
    };

    PPH_STRING string;
    PH_FORMAT format[5];

    if (ProviderFlags)
    {
        string = PhGetAccessString(ProviderFlags, (PPH_ACCESS_ENTRY)providerFlags, RTL_NUMBER_OF(providerFlags));

        PhInitFormatS(&format[0], L"0x");
        PhInitFormatX(&format[1], ProviderFlags);
        PhInitFormatS(&format[2], L" (");
        PhInitFormatSR(&format[3], string->sr);
        PhInitFormatC(&format[4], L')');

        PhMoveReference(&string, PhFormat(format, 5, 10));
    }
    else
    {
        string = PhCreateString(L"None");
    }

    return string;
}

/**
 * Formats a human-readable name for a set of service flags.
 *
 * \param[in] ServiceFlags The service flags value.
 * \return A string with the flag names.
 */
PPH_STRING PhAfdFormatServiceFlags(
    _In_ ULONG ServiceFlags
    )
{
#define PH_AFD_SERVICE_FLAG(x, n) { TEXT(#x), x, FALSE, FALSE, n }

    static const PH_ACCESS_ENTRY serviceFlags[] =
    {
        PH_AFD_SERVICE_FLAG(XP1_CONNECTIONLESS, L"Connectionless"),
        PH_AFD_SERVICE_FLAG(XP1_GUARANTEED_DELIVERY, L"Guaranteed delivery"),
        PH_AFD_SERVICE_FLAG(XP1_GUARANTEED_ORDER, L"Guaranteed order"),
        PH_AFD_SERVICE_FLAG(XP1_MESSAGE_ORIENTED, L"Message-oriented"),
        PH_AFD_SERVICE_FLAG(XP1_PSEUDO_STREAM, L"Pseudo-stream"),
        PH_AFD_SERVICE_FLAG(XP1_GRACEFUL_CLOSE, L"Graceful close"),
        PH_AFD_SERVICE_FLAG(XP1_EXPEDITED_DATA, L"Expedited data"),
        PH_AFD_SERVICE_FLAG(XP1_CONNECT_DATA, L"Connect data"),
        PH_AFD_SERVICE_FLAG(XP1_DISCONNECT_DATA, L"Disconnect data"),
        PH_AFD_SERVICE_FLAG(XP1_SUPPORT_BROADCAST, L"Broadcast"),
        PH_AFD_SERVICE_FLAG(XP1_SUPPORT_MULTIPOINT, L"Support multipoint"),
        PH_AFD_SERVICE_FLAG(XP1_MULTIPOINT_CONTROL_PLANE, L"Multipoint control plane"),
        PH_AFD_SERVICE_FLAG(XP1_MULTIPOINT_DATA_PLANE, L"Multipoint data plane"),
        PH_AFD_SERVICE_FLAG(XP1_QOS_SUPPORTED, L"QoS supported"),
        PH_AFD_SERVICE_FLAG(XP1_INTERRUPT, L"Interrupt"),
        PH_AFD_SERVICE_FLAG(XP1_UNI_SEND, L"Unidirectional send"),
        PH_AFD_SERVICE_FLAG(XP1_UNI_RECV, L"Unidirectional receive"),
        PH_AFD_SERVICE_FLAG(XP1_IFS_HANDLES, L"IFS handles"),
        PH_AFD_SERVICE_FLAG(XP1_PARTIAL_MESSAGE, L"Partial message"),
        PH_AFD_SERVICE_FLAG(XP1_SAN_SUPPORT_SDP, L"SAN"),
    };

    PPH_STRING string;
    PH_FORMAT format[5];

    if (ServiceFlags)
    {
        string = PhGetAccessString(ServiceFlags, (PPH_ACCESS_ENTRY)serviceFlags, RTL_NUMBER_OF(serviceFlags));

        PhInitFormatS(&format[0], L"0x");
        PhInitFormatX(&format[1], ServiceFlags);
        PhInitFormatS(&format[2], L" (");
        PhInitFormatSR(&format[3], string->sr);
        PhInitFormatC(&format[4], L')');

        PhMoveReference(&string, PhFormat(format, 5, 10));
    }
    else
    {
        string = PhCreateString(L"None");
    }

    return string;
}

/**
 * Formats a human-readable name for a set of socket creation flags.
 *
 * \param[in] CreationFlags The creation flags value.
 * \return A string with the flag names.
 */
PPH_STRING PhAfdFormatCreationFlags(
    _In_ ULONG CreationFlags
    )
{
#define PH_AFD_CREATION_FLAG(x, n) { TEXT(#x), x, FALSE, FALSE, n }

    static const PH_ACCESS_ENTRY creationFlags[] =
    {
        PH_AFD_CREATION_FLAG(WSA_FLAG_OVERLAPPED, L"Overlapped"),
        PH_AFD_CREATION_FLAG(WSA_FLAG_MULTIPOINT_C_ROOT, L"Multipoint control root"),
        PH_AFD_CREATION_FLAG(WSA_FLAG_MULTIPOINT_C_LEAF, L"Multipoint control leaf"),
        PH_AFD_CREATION_FLAG(WSA_FLAG_MULTIPOINT_D_ROOT, L"Multipoint data root"),
        PH_AFD_CREATION_FLAG(WSA_FLAG_MULTIPOINT_D_LEAF, L"Multipoint data leaf"),
        PH_AFD_CREATION_FLAG(WSA_FLAG_ACCESS_SYSTEM_SECURITY, L"Access SACL"),
        PH_AFD_CREATION_FLAG(WSA_FLAG_NO_HANDLE_INHERIT, L"No handle inherit"),
        PH_AFD_CREATION_FLAG(WSA_FLAG_REGISTERED_IO, L"Registered I/O"),
    };

    PPH_STRING string;
    PH_FORMAT format[5];

    if (CreationFlags)
    {
        string = PhGetAccessString(CreationFlags, (PPH_ACCESS_ENTRY)creationFlags, RTL_NUMBER_OF(creationFlags));

        PhInitFormatS(&format[0], L"0x");
        PhInitFormatX(&format[1], CreationFlags);
        PhInitFormatS(&format[2], L" (");
        PhInitFormatSR(&format[3], string->sr);
        PhInitFormatC(&format[4], L')');

        PhMoveReference(&string, PhFormat(format, 5, 10));
    }
    else
    {
        string = PhCreateString(L"None");
    }

    return string;
}

/**
 * Formats a human-readable name for a set of AFD socket flags.
 *
 * \param[in] SharedInfo The shared socket information buffer.
 * \return A string with the flag names.
 */
PPH_STRING PhAfdFormatSharedInfoFlags(
    _In_ PSOCK_SHARED_INFO SharedInfo
    )
{
    PH_STRING_BUILDER stringBuilder;

    if (!SharedInfo->Flags)
        return PhCreateString(L"None");

    PhInitializeStringBuilder(&stringBuilder, 80);

    PhAppendFormatStringBuilder(&stringBuilder, L"0x%x (", SharedInfo->Flags);

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

    // Remove the trailing comma
    PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhAppendStringBuilder2(&stringBuilder, L")");

    return PhFinalStringBuilderString(&stringBuilder);
}

/**
 * Looks up a name of a known address family.
 *
 * \param[in] AddressFamily The address family value.
 * \return A string with the address family name or NULL when it is not recognized.
 */
_Maybenull_
PCWSTR PhpAfdGetAddressFamilyString(
    _In_ LONG AddressFamily
    )
{
    switch (AddressFamily)
    {
    case AF_UNSPEC:
        return L"Unspecified";
    case AF_UNIX:
        return L"Unix";
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
 * Formats an address family as a string.
 *
 * \param[in] AddressFamily The address family value.
 * \return A string with the address family name.
 */
PPH_STRING PhAfdFormatAddressFamily(
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
 * Looks up a name for a known protocol.
 *
 * \param[in] AddressFamily The address family of the protocol.
 * \param[in] Protocol The protocol value.
 * \return A string with the protocol name or NULL when it is not recognized.
 */
_Maybenull_
PCWSTR PhpAfdGetProtocolString(
    _In_ LONG AddressFamily,
    _In_ LONG Protocol
    )
{
    switch (AddressFamily)
    {
    case AF_UNIX:
        return L"UNIX";
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
 * Formats a protocol name as a string.
 *
 * \param[in] AddressFamily The address family of the protocol.
 * \param[in] Protocol The protocol value.
 * \return A string with a human-readable protocol name.
 */
PPH_STRING PhAfdFormatProtocol(
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
 * Looks up a short human-readable identifier for a known protocol.
 *
 * \param[in] AddressFamily The address family of the protocol.
 * \param[in] Protocol The protocol value.
 * \return A string with the protocol name or NULL when it is not recognized.
 */
_Maybenull_
PCWSTR PhpAfdGetProtocolSummary(
    _In_ LONG AddressFamily,
    _In_ LONG Protocol
    )
{
    switch (AddressFamily)
    {
    case AF_UNIX:
        return L"UNIX";
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
 * Looks up a human-readable name of a group type.
 *
 * \param[in] GroupType The socket group type value.
 * \return A string with the group type name or NULL when the value is not recognized.
 */
_Maybenull_
PCWSTR PhpAfdGetGroupTypeString(
    _In_ AFD_GROUP_TYPE GroupType
    )
{
    switch (GroupType)
    {
    case GroupTypeNeither:
        return L"Neither";
    case GroupTypeUnconstrained:
        return L"Unconstrained";
    case GroupTypeConstrained:
        return L"Constrained";
    default:
        return NULL;
    }
}

/**
 * Formats a group type name as a string.
 *
 * \param[in] GroupType The socket group type value.
 * \return A string with a human-readable group type name.
 */
PPH_STRING PhAfdFormatGroupType(
    _In_ AFD_GROUP_TYPE GroupType
    )
{
    PCWSTR knownName = PhpAfdGetGroupTypeString(GroupType);

    if (knownName)
        return PhCreateString(knownName);
    else
        return PhFormatString(L"Unrecognized type (%d)", GroupType);
}

/**
 * Looks up a human-readable name for an IPv6 protection level.
 *
 * \param[in] ProtectionLevel The protection level value.
 * \return A string with the protection level name or NULL when the value is not recognized.
 */
_Maybenull_
PCWSTR PhpAfdGetProtectionLevelString(
    _In_ ULONG ProtectionLevel
    )
{
    switch (ProtectionLevel)
    {
    case PROTECTION_LEVEL_UNRESTRICTED:
        return L"Unrestricted";
    case PROTECTION_LEVEL_EDGERESTRICTED:
        return L"Edge-restricted";
    case PROTECTION_LEVEL_RESTRICTED:
        return L"Restricted";
    case PROTECTION_LEVEL_DEFAULT:
        return L"Default";
    default:
        return NULL;
    }
}

/**
 * Formats an IPv6 protection level name as a string.
 *
 * \param[in] ProtectionLevel The protection level value.
 * \return A string with a human-readable protection level name.
 */
PPH_STRING PhAfdFormatProtectionLevel(
    _In_ ULONG ProtectionLevel
    )
{
    PCWSTR knownName;

    if (knownName = PhpAfdGetProtectionLevelString(ProtectionLevel))
    {
        return PhCreateString(knownName);
    }

    return PhFormatString(L"Unrecognized value (%d)", ProtectionLevel);
}

/**
 * Looks up a human-readable name for MTU discovery mode.
 *
 * \param[in] MtuDiscover The MTU discovery value.
 * \return A string with the MTU discovery mode name or NULL when the value is not recognized.
 */
_Maybenull_
PCWSTR PhpAfdGetMtuDiscoveryString(
    _In_ ULONG MtuDiscover
    )
{
    switch (MtuDiscover)
    {
    case IP_PMTUDISC_NOT_SET:
        return L"Not set";
    case IP_PMTUDISC_DO:
        return L"Perform";
    case IP_PMTUDISC_DONT:
        return L"Don't perform";
    case IP_PMTUDISC_PROBE:
        return L"Probe";
    default:
        return NULL;
    }
}

/**
 * Formats an MTU discovery mode as a string.
 *
 * \param[in] MtuDiscover The MTU discovery value.
 * \return A string with a human-readable MTU discovery mode name.
 */
PPH_STRING PhAfdFormatMtuDiscoveryMode(
    _In_ ULONG MtuDiscover
    )
{
    PCWSTR knownName;

    if (knownName = PhpAfdGetMtuDiscoveryString(MtuDiscover))
    {
        return PhCreateString(knownName);
    }

    return PhFormatString(L"Unrecognized value (%d)", MtuDiscover);
}

/**
 * Looks up a human-readable name for a TCP state.
 *
 * \param[in] TcpState The TCP state value.
 * \return A string with the TCP state name or NULL when the value is not recognized.
 */
_Maybenull_
PCWSTR PhpAfdGetTcpStateString(
    _In_ TCPSTATE TcpState
    )
{
    switch (TcpState)
    {
    case TCPSTATE_CLOSED:
        return L"Closed";
    case TCPSTATE_LISTEN:
        return L"Listen";
    case TCPSTATE_SYN_SENT:
        return L"SYN sent";
    case TCPSTATE_SYN_RCVD:
        return L"SYN received";
    case TCPSTATE_ESTABLISHED:
        return L"Established";
    case TCPSTATE_FIN_WAIT_1:
        return L"FIN wait 1";
    case TCPSTATE_FIN_WAIT_2:
        return L"FIN wait 2";
    case TCPSTATE_CLOSE_WAIT:
        return L"Close wait";
    case TCPSTATE_CLOSING:
        return L"Closing";
    case TCPSTATE_LAST_ACK:
        return L"Last ACK";
    case TCPSTATE_TIME_WAIT:
        return L"Time wait";
    default:
        return NULL;
    }
}

/**
 * Formats a TCP state as a string.
 *
 * \param[in] TcpState The TCP state value.
 * \return A string with a human-readable TCP state name.
 */
PPH_STRING PhAfdFormatTcpState(
    _In_ TCPSTATE TcpState
    )
{
    PCWSTR knownName;

    if (knownName = PhpAfdGetTcpStateString(TcpState))
    {
        return PhCreateString(knownName);
    }

    return PhFormatString(L"Unrecognized state (%d)", TcpState);
}

/**
 * Formats an interface identifier as a string.
 *
 * \param[in] Interface The interface identifier.
 * \return A human-readable string with the interface IP address or scope ID.
 */
PPH_STRING PhAfdFormatInterfaceOption(
    _In_ ULONG Interface
    )
{
    if (Interface & 0x000000FF)
    {
        NTSTATUS status;
        IN_ADDR interfaceIp;
        ULONG addressStringLength = INET6_ADDRSTRLEN;
        WCHAR addressString[INET6_ADDRSTRLEN];

        // Values with a non-zero first octet identify an interface by IP address
        interfaceIp.S_un.S_addr = Interface;

        status = PhIpv4AddressToString(
            &interfaceIp,
            0,
            addressString,
            &addressStringLength
            );

        if (NT_SUCCESS(status))
        {
            PH_STRINGREF string;

            string.Buffer = addressString;
            string.Length = (addressStringLength - 1) * sizeof(WCHAR);

            return PhCreateString2(&string);
        }

        return PhFormatString(L"Error: %lu", status);
    }
    else if (Interface)
    {
        PH_FORMAT format[2];

        // Other values (0.0.0.0/24 addresses) store a big-endian interface index/scope ID
        PhInitFormatC(&format[0], L'%');
        PhInitFormatU(&format[1], _byteswap_ulong(Interface));
        return PhFormat(format, RTL_NUMBER_OF(format), 8);
    }
    else
    {
        // The zero interface is special
        return PhCreateString(L"Default");
    }
}

/**
 * Formats a human-readable name for an AFD socket.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \return The best handle name representation available on success, or NULL on error.
 */
_Maybenull_
PPH_STRING PhAfdFormatSocketBestName(
    _In_ HANDLE SocketHandle
    )
{
    PPH_STRING string = NULL;
    PH_FORMAT format[9];
    ULONG count = 0;
    SOCK_SHARED_INFO sharedInfo;
    PPH_STRING addressString = NULL;
    PPH_STRING remoteAddressString = NULL;

    PhInitFormatS(&format[count++], L"AFD socket:");

    if (NT_SUCCESS(PhAfdQuerySharedInfo(SocketHandle, &sharedInfo)))
    {
        PCWSTR detail;

        // State
        if (detail = PhpAfdGetSocketStateString(sharedInfo.State))
        {
            PhInitFormatC(&format[count++], L' ');
            PhInitFormatS(&format[count++], detail);
        }

        // Protocol
        if (detail = PhpAfdGetProtocolSummary(sharedInfo.AddressFamily, sharedInfo.Protocol))
        {
            PhInitFormatC(&format[count++], L' ');
            PhInitFormatS(&format[count++], detail);
        }
    }

    // Local address
    if (NT_SUCCESS(PhAfdQueryFormatAddress(SocketHandle, FALSE, &addressString, PH_AFD_ADDRESS_SIMPLIFY)) &&
        !PhIsNullOrEmptyString(addressString))
    {
        PhInitFormatS(&format[count++], L" on ");
        PhInitFormatSR(&format[count++], addressString->sr);
    }

    // Remote address
    if (NT_SUCCESS(PhAfdQueryFormatAddress(SocketHandle, TRUE, &remoteAddressString, PH_AFD_ADDRESS_SIMPLIFY)) &&
        !PhIsNullOrEmptyString(remoteAddressString))
    {
        PhInitFormatS(&format[count++], L" to ");
        PhInitFormatSR(&format[count++], remoteAddressString->sr);
    }

    if (count > 1)
        string = PhFormat(format, count, 10);

    PhClearReference(&addressString);
    PhClearReference(&remoteAddressString);

    return string;
}
