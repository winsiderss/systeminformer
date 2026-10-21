/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     diversenok   2025
 *     jxy-s        2026
 *
 */

#include <ph.h>

#include <ntafd.h>

#include <phafd.h>
#include <phnet.h>
#include <hndlinfo.h>
#include <ws2ipdef.h>
#include <ws2bth.h>
#include <afunix.h>
#include <hvsocketcontrol.h>
#include <secedit.h>


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
 * \param[in] OptionLength The size of the OptionValue buffer.
 * \param[in] ReturnLength The size of the returned option value, if available.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdQuerySocketOption(
    _In_ HANDLE SocketHandle,
    _In_ ULONG Level,
    _In_ ULONG OptionName,
    _Out_ PVOID OptionValue,
    _In_ ULONG OptionLength,
    _Out_opt_ PULONG ReturnLength
    )
{
    AFD_TL_IO_CONTROL_INFO controlInfo;

    memset(&controlInfo, 0, sizeof(AFD_TL_IO_CONTROL_INFO));
    controlInfo.Type = TlGetSockOptIoControlType;
    controlInfo.EndpointIoctl = TRUE;
    controlInfo.Level = Level;
    controlInfo.IoControlCode = OptionName;

    return PhAfdDeviceIoControl(
        SocketHandle,
        IOCTL_AFD_TRANSPORT_IOCTL,
        &controlInfo,
        sizeof(AFD_TL_IO_CONTROL_INFO),
        OptionValue,
        OptionLength,
        ReturnLength,
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
//
//NTSTATUS PhSocketConnect(
//    _In_ HANDLE SocketHandle,
//    _In_ PSOCKADDR RemoteAddress,
//    _In_ ULONG RemoteAddressLength
//    )
//{
//    SOCKET socketHandle;
//    int error;
//
//    if (!RemoteAddress || RemoteAddressLength < sizeof(SOCKADDR))
//        return STATUS_INVALID_PARAMETER;
//
//    socketHandle = (SOCKET)SocketHandle;
//    error = WSAConnect(socketHandle, RemoteAddress, RemoteAddressLength, NULL, NULL, NULL, NULL);
//
//    return error == 0 ? STATUS_SUCCESS : RtlNtStatusFromWin32(WSAGetLastError());
//}
//
//NTSTATUS PhSocketSend(
//    _In_ HANDLE SocketHandle,
//    _In_reads_(BufferCount) LPWSABUF BufferArray,
//    _In_ ULONG BufferCount,
//    _Out_opt_ PULONG BytesSent
//    )
//{
//    SOCKET socketHandle;
//    DWORD bytesSent = 0;
//    int error;
//
//    if (!BufferArray || !BufferCount)
//        return STATUS_INVALID_PARAMETER;
//      if (NT_SUCCESS(PhAfdDeviceIoControl(SocketHandle, IOCTL_AFD_SEND, NULL, 0, NULL, 0, NULL, NULL)))
//            return STATUS_NOT_FOUND;
//    socketHandle = (SOCKET)SocketHandle;
//    error = WSASend(socketHandle, BufferArray, BufferCount, &bytesSent, 0, NULL, NULL);
//
//    if (BytesSent)
//        *BytesSent = bytesSent;
//
//    return error == 0 ? STATUS_SUCCESS : RtlNtStatusFromWin32(WSAGetLastError());
//}
//
//NTSTATUS PhSocketReceive(
//    _In_ HANDLE SocketHandle,
//    _Out_writes_(BufferCount) LPWSABUF BufferArray,
//    _In_ ULONG BufferCount,
//    _Out_opt_ PULONG BytesReceived
//    )
//{
//    SOCKET socketHandle;
//    DWORD bytesReceived = 0;
//    DWORD flags = 0;
//    int error;
//
//    if (!BufferArray || !BufferCount)
//        return STATUS_INVALID_PARAMETER;
//
//    socketHandle = (SOCKET)SocketHandle;
//    error = WSARecv(socketHandle, BufferArray, BufferCount, &bytesReceived, &flags, NULL, NULL);
//
//    if (BytesReceived)
//        *BytesReceived = bytesReceived;
//
//    return error == 0 ? STATUS_SUCCESS : RtlNtStatusFromWin32(WSAGetLastError());
//}
//
//NTSTATUS PhSocketUdpSend(
//    _In_ HANDLE SocketHandle,
//    _In_reads_(BufferCount) LPWSABUF BufferArray,
//    _In_ ULONG BufferCount,
//    _In_ PSOCKADDR RemoteAddress,
//    _In_ ULONG RemoteAddressLength,
//    _Out_opt_ PULONG BytesSent
//    )
//{
//    SOCKET socketHandle;
//    DWORD bytesSent = 0;
//    int error;
//
//    if (!BufferArray || !BufferCount || !RemoteAddress || RemoteAddressLength < sizeof(SOCKADDR))
//        return STATUS_INVALID_PARAMETER;
//
//    socketHandle = (SOCKET)SocketHandle;
//    error = WSASendTo(socketHandle, BufferArray, BufferCount, &bytesSent, 0, RemoteAddress, RemoteAddressLength, NULL, NULL);
//
//    if (BytesSent)
//        *BytesSent = bytesSent;
//
//    return error == 0 ? STATUS_SUCCESS : RtlNtStatusFromWin32(WSAGetLastError());
//}
//
//NTSTATUS PhSocketUdpReceive(
//    _In_ HANDLE SocketHandle,
//    _Out_writes_(BufferCount) LPWSABUF BufferArray,
//    _In_ ULONG BufferCount,
//    _Out_writes_bytes_opt_(*RemoteAddressLength) PSOCKADDR RemoteAddress,
//    _Inout_opt_ PULONG RemoteAddressLength,
//    _Out_opt_ PULONG BytesReceived
//    )
//{
//    SOCKET socketHandle;
//    DWORD bytesReceived = 0;
//    DWORD flags = 0;
//    int error;
//
//    if (!BufferArray || !BufferCount)
//        return STATUS_INVALID_PARAMETER;
//
//    if (!RemoteAddressLength)
//        return STATUS_INVALID_PARAMETER;
//
//    socketHandle = (SOCKET)SocketHandle;
//    error = WSARecvFrom(socketHandle, BufferArray, BufferCount, &bytesReceived, &flags, RemoteAddress, RemoteAddressLength, NULL, NULL);
//
//    if (BytesReceived)
//        *BytesReceived = bytesReceived;
//
//    return error == 0 ? STATUS_SUCCESS : RtlNtStatusFromWin32(WSAGetLastError());
//}
//
//NTSTATUS PhSocketClose(
//    _In_ HANDLE SocketHandle
//    )
//{
//    if (!SocketHandle)
//        return STATUS_INVALID_HANDLE;
//
//    return closesocket((SOCKET)SocketHandle) == 0 ? STATUS_SUCCESS : RtlNtStatusFromWin32(WSAGetLastError());
//}

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
        {
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
        }
        break;
    case AF_BTH:
        {
            switch (Protocol)
            {
            case BTHPROTO_RFCOMM:
                return L"RFCOMM";
            case BTHPROTO_L2CAP:
                return L"L2CAP";
            }
        }
        break;
    case AF_HYPERV:
        {
            switch (Protocol)
            {
            case HV_PROTOCOL_RAW:
                return L"RAW";
            }
        }
        break;
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
 * Retrieves the address information required to format a human-readable name for an AFD socket.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \param[out] AddressInfo A buffer with the socket address information.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdQuerySocketAddressInfo(
    _In_ HANDLE SocketHandle,
    _Out_ PPH_AFD_SOCKET_ADDRESS_INFORMATION AddressInfo
    )
{
    RtlZeroMemory(AddressInfo, sizeof(PH_AFD_SOCKET_ADDRESS_INFORMATION));

    AddressInfo->HasSharedInfo = NT_SUCCESS(PhAfdQuerySharedInfo(SocketHandle, &AddressInfo->SharedInfo));
    AddressInfo->HasLocalAddress = NT_SUCCESS(PhAfdQueryAddress(SocketHandle, FALSE, &AddressInfo->LocalAddress));
    AddressInfo->HasRemoteAddress = NT_SUCCESS(PhAfdQueryAddress(SocketHandle, TRUE, &AddressInfo->RemoteAddress));

    if (AddressInfo->HasSharedInfo || AddressInfo->HasLocalAddress || AddressInfo->HasRemoteAddress)
        return STATUS_SUCCESS;

    return STATUS_UNSUCCESSFUL;
}

/**
 * Formats a human-readable name for an AFD socket.
 *
 * \param[in] AddressInfo The socket address information from PhAfdQuerySocketAddressInfo.
 * \return The best handle name representation available on success, or NULL on error.
 */
_Maybenull_
PPH_STRING PhAfdFormatSocketBestName(
    _In_ PPH_AFD_SOCKET_ADDRESS_INFORMATION AddressInfo
    )
{
    PPH_STRING string = NULL;
    PH_FORMAT format[9];
    ULONG count = 0;
    PPH_STRING addressString = NULL;
    PPH_STRING remoteAddressString = NULL;

    PhInitFormatS(&format[count++], L"AFD socket:");

    if (AddressInfo->HasSharedInfo)
    {
        PCWSTR detail;

        // State
        if (detail = PhpAfdGetSocketStateString(AddressInfo->SharedInfo.State))
        {
            PhInitFormatC(&format[count++], L' ');
            PhInitFormatS(&format[count++], detail);
        }

        // Protocol
        if (detail = PhpAfdGetProtocolSummary(AddressInfo->SharedInfo.AddressFamily, AddressInfo->SharedInfo.Protocol))
        {
            PhInitFormatC(&format[count++], L' ');
            PhInitFormatS(&format[count++], detail);
        }
    }

    // Local address
    if (AddressInfo->HasLocalAddress &&
        NT_SUCCESS(PhAfdFormatAddress(&AddressInfo->LocalAddress, &addressString, PH_AFD_ADDRESS_SIMPLIFY)) &&
        !PhIsNullOrEmptyString(addressString))
    {
        PhInitFormatS(&format[count++], L" on ");
        PhInitFormatSR(&format[count++], addressString->sr);
    }

    // Remote address
    if (AddressInfo->HasRemoteAddress &&
        NT_SUCCESS(PhAfdFormatAddress(&AddressInfo->RemoteAddress, &remoteAddressString, PH_AFD_ADDRESS_SIMPLIFY)) &&
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

#ifndef TDI_RECEIVE_NORMAL
#define TDI_RECEIVE_NORMAL 0x20
#endif

/**
 * Captures the completion state of an AFD operation.
 *
 * \param[in,out] Operation The operation.
 * \param[out] Information Optionally receives the number of bytes transferred.
 * \return STATUS_PENDING if the operation is still in progress, otherwise its final status.
 */
NTSTATUS PhpAfdCompleteOperation(
    _Inout_ PPH_AFD_IO_OPERATION Operation,
    _Out_opt_ PULONG_PTR Information
    )
{
    NTSTATUS status;

    status = Operation->IoStatusBlock.Status;

    if (status == STATUS_PENDING)
        return status;

    Operation->Active = FALSE;

    if (Information && NT_SUCCESS(status))
    {
        *Information = Operation->IoStatusBlock.Information;
    }

    return status;
}

/**
 * Initializes an AFD operation and creates its completion event.
 *
 * \param[out] Operation The operation to initialize.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdInitializeOperation(
    _Out_ PPH_AFD_IO_OPERATION Operation
    )
{
    NTSTATUS status;

    if (!Operation)
        return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(Operation, sizeof(PH_AFD_IO_OPERATION));

    status = NtCreateEvent(
        &Operation->EventHandle,
        EVENT_ALL_ACCESS,
        NULL,
        NotificationEvent,
        FALSE
        );

    if (!NT_SUCCESS(status))
    {
        Operation->EventHandle = NULL;
    }

    return status;
}

/**
 * Cancels any active request and releases the resources of an AFD operation.
 *
 * \param[in,out] Operation The operation.
 */
VOID PhAfdDeleteOperation(
    _Inout_ PPH_AFD_IO_OPERATION Operation
    )
{
    if (!Operation)
        return;

    if (Operation->Active)
    {
        PhAfdCancelOperation(Operation);
    }

    if (Operation->EventHandle)
    {
        NtClose(Operation->EventHandle);
    }

    RtlZeroMemory(Operation, sizeof(PH_AFD_IO_OPERATION));
}

/**
 * Queries the state of an active AFD operation without waiting.
 *
 * \param[in,out] Operation The operation.
 * \param[out] Information Optionally receives the number of bytes transferred.
 * \return STATUS_PENDING if the operation is still in progress, otherwise its final status.
 */
NTSTATUS PhAfdQueryOperation(
    _Inout_ PPH_AFD_IO_OPERATION Operation,
    _Out_opt_ PULONG_PTR Information
    )
{
    if (!Operation || !Operation->EventHandle || !Operation->Active)
        return STATUS_INVALID_DEVICE_STATE;

    return PhpAfdCompleteOperation(Operation, Information);
}

/**
 * Waits for an active AFD operation to complete.
 *
 * \param[in,out] Operation The operation.
 * \param[in] Timeout An optional timeout.
 * \param[out] Information Optionally receives the number of bytes transferred.
 * \return The final status of the operation, or STATUS_IO_TIMEOUT if the wait timed out.
 */
NTSTATUS PhAfdWaitOperation(
    _Inout_ PPH_AFD_IO_OPERATION Operation,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Out_opt_ PULONG_PTR Information
    )
{
    NTSTATUS status;

    if (!Operation || !Operation->EventHandle || !Operation->Active)
        return STATUS_INVALID_DEVICE_STATE;

    status = PhpAfdCompleteOperation(Operation, Information);

    if (status != STATUS_PENDING)
        return status;

    status = NtWaitForSingleObject(
        Operation->EventHandle,
        FALSE,
        Timeout
        );

    if (status == STATUS_TIMEOUT)
        return STATUS_IO_TIMEOUT;
    if (!NT_SUCCESS(status))
        return status;

    return PhpAfdCompleteOperation(Operation, Information);
}

/**
 * Cancels an active AFD operation and waits for the cancellation to complete.
 *
 * \param[in,out] Operation The operation.
 * \return The final status of the operation.
 */
NTSTATUS PhAfdCancelOperation(
    _Inout_ PPH_AFD_IO_OPERATION Operation
    )
{
    NTSTATUS status;

    if (!Operation || !Operation->EventHandle || !Operation->Active)
        return STATUS_INVALID_DEVICE_STATE;

    if (Operation->IoStatusBlock.Status != STATUS_PENDING)
        return PhpAfdCompleteOperation(Operation, NULL);

    RtlZeroMemory(&Operation->CancelIoStatusBlock, sizeof(IO_STATUS_BLOCK));

    status = NtCancelIoFileEx(
        Operation->SocketHandle,
        &Operation->IoStatusBlock,
        &Operation->CancelIoStatusBlock
        );

    if (!NT_SUCCESS(status) && status != STATUS_NOT_FOUND)
        return status;

    status = NtWaitForSingleObject(
        Operation->EventHandle,
        FALSE,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    return PhpAfdCompleteOperation(Operation, NULL);
}

/**
 * Creates an AFD socket endpoint.
 *
 * \param[out] SocketHandle Receives the socket handle.
 * \param[in] AddressFamily The address family. Only AF_INET is supported.
 * \param[in] SocketType The socket type, SOCK_STREAM or SOCK_DGRAM.
 * \param[in] Protocol The protocol.
 * \param[in] EndpointFlags AFD endpoint flags.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdCreateSocket(
    _Out_ PHANDLE SocketHandle,
    _In_ LONG AddressFamily,
    _In_ LONG SocketType,
    _In_ LONG Protocol,
    _In_ ULONG EndpointFlags
    )
{
    NTSTATUS status;
    HANDLE socketHandle;
    UNICODE_STRING deviceName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    AFD_OPEN_PACKET_FULL_EA ea;

    if (!SocketHandle)
        return STATUS_INVALID_PARAMETER;
    if (AddressFamily != AF_INET)
        return STATUS_INVALID_PARAMETER;
    if (SocketType != SOCK_STREAM && SocketType != SOCK_DGRAM)
        return STATUS_INVALID_PARAMETER;

    *SocketHandle = NULL;

    RtlInitUnicodeString(&deviceName, L"\\Device\\Afd\\Endpoint");
    InitializeObjectAttributes(
        &objectAttributes,
        &deviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    RtlZeroMemory(&ea, sizeof(AFD_OPEN_PACKET_FULL_EA));
    ea.EaNameLength = sizeof(AfdOpenPacket) - sizeof(ANSI_NULL);
    ea.EaValueLength = sizeof(AFD_OPEN_PACKET);
    RtlCopyMemory(ea.EaName, AfdOpenPacket, sizeof(AfdOpenPacket));
    ea.OpenPacket.__f.EndpointFlags = EndpointFlags;
    ea.OpenPacket.GroupID = 0;
    ea.OpenPacket.AddressFamily = AddressFamily;
    ea.OpenPacket.SocketType = SocketType;
    ea.OpenPacket.Protocol = Protocol;

    status = NtCreateFile(
        &socketHandle,
        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE | READ_CONTROL,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        0,
        &ea,
        sizeof(AFD_OPEN_PACKET_FULL_EA)
        );

    if (NT_SUCCESS(status))
    {
        *SocketHandle = socketHandle;
    }

    return status;
}

/**
 * Starts an asynchronous IOCTL on an AFD socket using the operation's event.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \param[in,out] Operation An initialized, inactive operation.
 * \param[in] IoControlCode I/O control code.
 * \param[in] InputBuffer Input buffer.
 * \param[in] InputLength Input buffer size.
 * \param[out] OutputBuffer Output buffer.
 * \param[in] OutputLength Output buffer size.
 * \return STATUS_PENDING if the request is in progress, otherwise its final status.
 */
NTSTATUS PhpAfdBeginControl(
    _In_ HANDLE SocketHandle,
    _Inout_ PPH_AFD_IO_OPERATION Operation,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputLength) PVOID InputBuffer,
    _In_ ULONG InputLength,
    _Out_writes_bytes_opt_(OutputLength) PVOID OutputBuffer,
    _In_ ULONG OutputLength
    )
{
    NTSTATUS status;

    if (!SocketHandle || !Operation || !Operation->EventHandle || Operation->Active)
        return STATUS_INVALID_PARAMETER;
    if (InputLength && !InputBuffer)
        return STATUS_INVALID_PARAMETER;
    if (OutputLength && !OutputBuffer)
        return STATUS_INVALID_PARAMETER;

    NtClearEvent(Operation->EventHandle);

    Operation->SocketHandle = SocketHandle;
    Operation->IoStatusBlock.Status = STATUS_PENDING;
    Operation->IoStatusBlock.Information = 0;
    Operation->Active = TRUE;

    status = NtDeviceIoControlFile(
        SocketHandle,
        Operation->EventHandle,
        NULL,
        NULL,
        &Operation->IoStatusBlock,
        IoControlCode,
        InputBuffer,
        InputLength,
        OutputBuffer,
        OutputLength
        );

    if (status != STATUS_PENDING)
    {
        if (Operation->IoStatusBlock.Status == STATUS_PENDING)
        {
            Operation->IoStatusBlock.Status = status;
        }

        Operation->Active = FALSE;
    }

    return status;
}

/**
 * Starts an asynchronous send on a connected AFD socket.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \param[in,out] Operation An initialized, inactive operation.
 * \param[in] Buffer The data to send. Must remain valid until the operation completes.
 * \param[in] Length The length of the data.
 * \return STATUS_PENDING if the request is in progress, otherwise its final status.
 */
NTSTATUS PhAfdBeginSend(
    _In_ HANDLE SocketHandle,
    _Inout_ PPH_AFD_IO_OPERATION Operation,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    PAFD_SEND_INFO sendInfo;

    if (!Buffer || !Length)
        return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(Operation->Request, sizeof(Operation->Request));
    Operation->Buffer.buf = Buffer;
    Operation->Buffer.len = Length;

    sendInfo = (PAFD_SEND_INFO)Operation->Request;
    sendInfo->BufferArray = &Operation->Buffer;
    sendInfo->BufferCount = 1;

    return PhpAfdBeginControl(
        SocketHandle,
        Operation,
        IOCTL_AFD_SEND,
        sendInfo,
        sizeof(AFD_SEND_INFO),
        NULL,
        0
        );
}

/**
 * Starts an asynchronous receive on a connected AFD socket.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \param[in,out] Operation An initialized, inactive operation.
 * \param[out] Buffer Receives the data. Must remain valid until the operation completes.
 * \param[in] Length The size of the buffer.
 * \return STATUS_PENDING if the request is in progress, otherwise its final status.
 */
NTSTATUS PhAfdBeginReceive(
    _In_ HANDLE SocketHandle,
    _Inout_ PPH_AFD_IO_OPERATION Operation,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    PAFD_RECV_INFO recvInfo;

    if (!Buffer || !Length)
        return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(Operation->Request, sizeof(Operation->Request));
    Operation->Buffer.buf = Buffer;
    Operation->Buffer.len = Length;

    recvInfo = (PAFD_RECV_INFO)Operation->Request;
    recvInfo->BufferArray = &Operation->Buffer;
    recvInfo->BufferCount = 1;
    recvInfo->TdiFlags = TDI_RECEIVE_NORMAL;

    return PhpAfdBeginControl(
        SocketHandle,
        Operation,
        IOCTL_AFD_RECEIVE,
        recvInfo,
        sizeof(AFD_RECV_INFO),
        NULL,
        0
        );
}

/**
 * Starts an asynchronous poll on a single AFD socket.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \param[in,out] Operation An initialized, inactive operation.
 * \param[in] PollEvents A bit mask of AFD_POLL_* events to wait for.
 * \param[in] TimeoutMilliseconds The poll timeout in milliseconds, or zero to poll without waiting.
 * \return STATUS_PENDING if the request is in progress, otherwise its final status.
 */
NTSTATUS PhAfdBeginPoll(
    _In_ HANDLE SocketHandle,
    _Inout_ PPH_AFD_IO_OPERATION Operation,
    _In_ ULONG PollEvents,
    _In_ LONGLONG TimeoutMilliseconds
    )
{
    PAFD_POLL_INFO pollInfo;
    ULONG pollInfoLength;

    if (!PollEvents || TimeoutMilliseconds < 0)
        return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(Operation->Request, sizeof(Operation->Request));

    pollInfo = (PAFD_POLL_INFO)Operation->Request;
    pollInfoLength = FIELD_OFFSET(AFD_POLL_INFO, Handles) + sizeof(AFD_POLL_HANDLE_INFO);

    pollInfo->Timeout.QuadPart = TimeoutMilliseconds ? -(TimeoutMilliseconds * PH_TIMEOUT_MS) : 0;
    pollInfo->NumberOfHandles = 1;
    pollInfo->Unique = FALSE;
    pollInfo->Handles[0].Handle = SocketHandle;
    pollInfo->Handles[0].PollEvents = PollEvents;

    return PhpAfdBeginControl(
        SocketHandle,
        Operation,
        IOCTL_AFD_POLL,
        pollInfo,
        pollInfoLength,
        pollInfo,
        pollInfoLength
        );
}

/**
 * Issues an IOCTL on an AFD socket and waits for its completion.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \param[in] IoControlCode I/O control code.
 * \param[in] InputBuffer Input buffer.
 * \param[in] InputLength Input buffer size.
 * \param[out] OutputBuffer Output buffer.
 * \param[in] OutputLength Output buffer size.
 * \param[out] Information Optionally receives the number of bytes returned.
 * \return Successful or errant status.
 */
NTSTATUS PhpAfdRun(
    _In_ HANDLE SocketHandle,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputLength) PVOID InputBuffer,
    _In_ ULONG InputLength,
    _Out_writes_bytes_opt_(OutputLength) PVOID OutputBuffer,
    _In_ ULONG OutputLength,
    _Out_opt_ PULONG_PTR Information
    )
{
    NTSTATUS status;
    PH_AFD_IO_OPERATION operation;

    status = PhAfdInitializeOperation(&operation);

    if (!NT_SUCCESS(status))
        return status;

    status = PhpAfdBeginControl(
        SocketHandle,
        &operation,
        IoControlCode,
        InputBuffer,
        InputLength,
        OutputBuffer,
        OutputLength
        );

    if (status == STATUS_PENDING)
    {
        status = PhAfdWaitOperation(&operation, NULL, Information);
    }
    else if (NT_SUCCESS(status) && Information)
    {
        *Information = operation.IoStatusBlock.Information;
    }

    PhAfdDeleteOperation(&operation);

    return status;
}

/**
 * Binds an AFD socket to a local address.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \param[in] ShareAccess An AFD_SHARE_* value controlling address reuse.
 * \param[in] Address The local IPv4 address.
 * \param[in] AddressLength The size of the address.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdBind(
    _In_ HANDLE SocketHandle,
    _In_ ULONG ShareAccess,
    _In_reads_bytes_(AddressLength) PSOCKADDR Address,
    _In_ ULONG AddressLength
    )
{
    UCHAR buffer[FIELD_OFFSET(AFD_BIND_INFO_TL, Address) + sizeof(SOCKADDR_STORAGE)] = { 0 };
    PAFD_BIND_INFO_TL bindInfo = (PAFD_BIND_INFO_TL)buffer;
    SOCKADDR_STORAGE boundAddress = { 0 };

    if (!Address || Address->sa_family != AF_INET)
        return STATUS_INVALID_PARAMETER;
    if (AddressLength < sizeof(SOCKADDR_IN) || AddressLength > sizeof(SOCKADDR_STORAGE))
        return STATUS_INVALID_PARAMETER;

    bindInfo->ShareAccess = ShareAccess;
    RtlCopyMemory(&bindInfo->Address, Address, AddressLength);

    return PhpAfdRun(
        SocketHandle,
        IOCTL_AFD_BIND,
        buffer,
        FIELD_OFFSET(AFD_BIND_INFO_TL, Address) + AddressLength,
        &boundAddress,
        sizeof(SOCKADDR_STORAGE),
        NULL
        );
}

/**
 * Connects an AFD socket to a remote address.
 *
 * \param[in] SocketHandle A bound AFD socket handle.
 * \param[in] Address The remote IPv4 address.
 * \param[in] AddressLength The size of the address.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdConnect(
    _In_ HANDLE SocketHandle,
    _In_reads_bytes_(AddressLength) PSOCKADDR Address,
    _In_ ULONG AddressLength
    )
{
    UCHAR buffer[FIELD_OFFSET(AFD_CONNECT_JOIN_INFO_TL, RemoteAddress) + sizeof(SOCKADDR_STORAGE)] = { 0 };
    PAFD_CONNECT_JOIN_INFO_TL connectInfo = (PAFD_CONNECT_JOIN_INFO_TL)buffer;

    if (!Address || Address->sa_family != AF_INET)
        return STATUS_INVALID_PARAMETER;
    if (AddressLength < sizeof(SOCKADDR_IN) || AddressLength > sizeof(SOCKADDR_STORAGE))
        return STATUS_INVALID_PARAMETER;

    RtlCopyMemory(&connectInfo->RemoteAddress, Address, AddressLength);

    return PhpAfdRun(
        SocketHandle,
        IOCTL_AFD_CONNECT,
        buffer,
        FIELD_OFFSET(AFD_CONNECT_JOIN_INFO_TL, RemoteAddress) + AddressLength,
        NULL,
        0,
        NULL
        );
}

/**
 * Sets the connect data sent with an outgoing connection request.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \param[in] Data The connect data.
 * \param[in] Length The length of the connect data.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdSetConnectData(
    _In_ HANDLE SocketHandle,
    _In_reads_bytes_(Length) PVOID Data,
    _In_ ULONG Length
    )
{
    AFD_UNACCEPTED_CONNECT_DATA_INFO connectDataInfo = { 0 };

    if (!Data || !Length)
        return STATUS_INVALID_PARAMETER;

    connectDataInfo.ConnectDataLength = Length;

    return PhpAfdRun(
        SocketHandle,
        IOCTL_AFD_SET_CONNECT_DATA,
        &connectDataInfo,
        sizeof(AFD_UNACCEPTED_CONNECT_DATA_INFO),
        Data,
        Length,
        NULL
        );
}

/**
 * Retrieves the connect data of a pending (unaccepted) connection.
 *
 * \param[in] ListenHandle A listening AFD socket handle.
 * \param[in] Sequence The connection sequence number from PhAfdWaitForListen.
 * \param[out] Data Receives the connect data.
 * \param[in] Length The size of the buffer.
 * \param[out] BytesTransferred Receives the number of bytes returned.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdGetUnacceptedConnectData(
    _In_ HANDLE ListenHandle,
    _In_ LONG Sequence,
    _Out_writes_bytes_(Length) PVOID Data,
    _In_ ULONG Length,
    _Out_ PULONG BytesTransferred
    )
{
    NTSTATUS status;
    AFD_UNACCEPTED_CONNECT_DATA_INFO connectDataInfo = { 0 };
    ULONG_PTR information = 0;

    if (!Data || !Length || !BytesTransferred)
        return STATUS_INVALID_PARAMETER;

    connectDataInfo.Sequence = Sequence;
    connectDataInfo.ConnectDataLength = Length;

    status = PhpAfdRun(
        ListenHandle,
        IOCTL_AFD_GET_UNACCEPTED_CONNECT_DATA,
        &connectDataInfo,
        sizeof(AFD_UNACCEPTED_CONNECT_DATA_INFO),
        Data,
        Length,
        &information
        );

    if (NT_SUCCESS(status))
    {
        *BytesTransferred = (ULONG)information;
    }

    return status;
}

/**
 * Retrieves the length of the connect data of a pending (unaccepted) connection.
 *
 * \param[in] ListenHandle A listening AFD socket handle.
 * \param[in] Sequence The connection sequence number from PhAfdWaitForListen.
 * \param[out] Length Receives the length of the connect data.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdQueryUnacceptedConnectDataLength(
    _In_ HANDLE ListenHandle,
    _In_ LONG Sequence,
    _Out_ PULONG Length
    )
{
    NTSTATUS status;
    AFD_UNACCEPTED_CONNECT_DATA_INFO inputInfo = { 0 };
    AFD_UNACCEPTED_CONNECT_DATA_INFO outputInfo = { 0 };

    if (!Length)
        return STATUS_INVALID_PARAMETER;

    inputInfo.Sequence = Sequence;
    inputInfo.LengthOnly = TRUE;

    status = PhpAfdRun(
        ListenHandle,
        IOCTL_AFD_GET_UNACCEPTED_CONNECT_DATA,
        &inputInfo,
        sizeof(AFD_UNACCEPTED_CONNECT_DATA_INFO),
        &outputInfo,
        sizeof(AFD_UNACCEPTED_CONNECT_DATA_INFO),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *Length = outputInfo.ConnectDataLength;
    }

    return status;
}

/**
 * Places a bound AFD socket into the listening state.
 *
 * \param[in] SocketHandle A bound AFD socket handle.
 * \param[in] Backlog The maximum length of the pending connection queue.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdListen(
    _In_ HANDLE SocketHandle,
    _In_ ULONG Backlog
    )
{
    AFD_LISTEN_INFO listenInfo = { 0 };

    if (!Backlog || Backlog > LONG_MAX)
        return STATUS_INVALID_PARAMETER;

    listenInfo.MaximumConnectionQueue = Backlog;

    return PhpAfdRun(
        SocketHandle,
        IOCTL_AFD_START_LISTEN,
        &listenInfo,
        sizeof(AFD_LISTEN_INFO),
        NULL,
        0,
        NULL
        );
}

/**
 * Waits for an incoming connection on a listening AFD socket.
 *
 * \param[in] SocketHandle A listening AFD socket handle.
 * \param[out] Response Receives the pending connection's sequence number and remote address.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdWaitForListen(
    _In_ HANDLE SocketHandle,
    _Out_ PAFD_LISTEN_RESPONSE_INFO_TL Response
    )
{
    if (!Response)
        return STATUS_INVALID_PARAMETER;

    return PhpAfdRun(
        SocketHandle,
        IOCTL_AFD_WAIT_FOR_LISTEN,
        NULL,
        0,
        Response,
        sizeof(AFD_LISTEN_RESPONSE_INFO_TL),
        NULL
        );
}

/**
 * Accepts a pending connection onto a separate AFD socket.
 *
 * \param[in] ListenHandle A listening AFD socket handle.
 * \param[in] Sequence The connection sequence number from PhAfdWaitForListen.
 * \param[in] AcceptHandle An unbound AFD socket handle that receives the connection.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdAccept(
    _In_ HANDLE ListenHandle,
    _In_ LONG Sequence,
    _In_ HANDLE AcceptHandle
    )
{
    AFD_ACCEPT_INFO acceptInfo = { 0 };

    if (!AcceptHandle)
        return STATUS_INVALID_PARAMETER;

    acceptInfo.Sequence = Sequence;
    acceptInfo.AcceptHandle = AcceptHandle;

    return PhpAfdRun(
        ListenHandle,
        IOCTL_AFD_ACCEPT,
        &acceptInfo,
        sizeof(AFD_ACCEPT_INFO),
        NULL,
        0,
        NULL
        );
}

/**
 * Sends or receives data on a connected AFD socket and waits for completion.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \param[in] Send TRUE to send, FALSE to receive.
 * \param[in,out] Buffer The data buffer.
 * \param[in] Length The size of the buffer.
 * \param[out] BytesTransferred Optionally receives the number of bytes transferred.
 * \return Successful or errant status.
 */
NTSTATUS PhpAfdTransfer(
    _In_ HANDLE SocketHandle,
    _In_ BOOLEAN Send,
    _Inout_updates_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_opt_ PULONG BytesTransferred
    )
{
    NTSTATUS status;
    PH_AFD_IO_OPERATION operation;
    ULONG_PTR information = 0;

    if (BytesTransferred)
    {
        *BytesTransferred = 0;
    }

    status = PhAfdInitializeOperation(&operation);

    if (!NT_SUCCESS(status))
        return status;

    if (Send)
        status = PhAfdBeginSend(SocketHandle, &operation, Buffer, Length);
    else
        status = PhAfdBeginReceive(SocketHandle, &operation, Buffer, Length);

    if (status == STATUS_PENDING)
    {
        status = PhAfdWaitOperation(&operation, NULL, &information);
    }
    else if (NT_SUCCESS(status))
    {
        information = operation.IoStatusBlock.Information;
    }

    PhAfdDeleteOperation(&operation);

    if (NT_SUCCESS(status) && BytesTransferred)
    {
        *BytesTransferred = (ULONG)information;
    }

    return status;
}

/**
 * Sends data on a connected AFD socket.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \param[in] Buffer The data to send.
 * \param[in] Length The length of the data.
 * \param[out] BytesTransferred Optionally receives the number of bytes sent.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdSend(
    _In_ HANDLE SocketHandle,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_opt_ PULONG BytesTransferred
    )
{
    return PhpAfdTransfer(SocketHandle, TRUE, Buffer, Length, BytesTransferred);
}

/**
 * Receives data from a connected AFD socket.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \param[out] Buffer Receives the data.
 * \param[in] Length The size of the buffer.
 * \param[out] BytesTransferred Optionally receives the number of bytes received.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdReceive(
    _In_ HANDLE SocketHandle,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_opt_ PULONG BytesTransferred
    )
{
    return PhpAfdTransfer(SocketHandle, FALSE, Buffer, Length, BytesTransferred);
}

/**
 * Polls a single AFD socket and waits for the result.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \param[in] PollEvents A bit mask of AFD_POLL_* events to wait for.
 * \param[in] TimeoutMilliseconds The poll timeout in milliseconds, or zero to poll without waiting.
 * \param[out] ReturnedEvents Receives the AFD_POLL_* events that were signaled.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdPoll(
    _In_ HANDLE SocketHandle,
    _In_ ULONG PollEvents,
    _In_ LONGLONG TimeoutMilliseconds,
    _Out_ PULONG ReturnedEvents
    )
{
    NTSTATUS status;
    PH_AFD_IO_OPERATION operation;

    if (!ReturnedEvents)
        return STATUS_INVALID_PARAMETER;

    *ReturnedEvents = 0;

    status = PhAfdInitializeOperation(&operation);

    if (!NT_SUCCESS(status))
        return status;

    status = PhAfdBeginPoll(
        SocketHandle,
        &operation,
        PollEvents,
        TimeoutMilliseconds
        );

    if (status == STATUS_PENDING)
    {
        status = PhAfdWaitOperation(&operation, NULL, NULL);
    }

    if (NT_SUCCESS(status))
    {
        *ReturnedEvents = ((PAFD_POLL_INFO)operation.Request)->Handles[0].PollEvents;
    }

    PhAfdDeleteOperation(&operation);

    return status;
}

/**
 * Retrieves the local or remote address of an AFD socket.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \param[in] IoControlCode IOCTL_AFD_GET_ADDRESS or IOCTL_AFD_GET_REMOTE_ADDRESS.
 * \param[out] Address Receives the address.
 * \param[in,out] AddressLength On input, the size of the buffer; on output, the size of the address.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdGetAddress(
    _In_ HANDLE SocketHandle,
    _In_ ULONG IoControlCode,
    _Out_writes_bytes_to_(*AddressLength, *AddressLength) PSOCKADDR Address,
    _Inout_ PULONG AddressLength
    )
{
    NTSTATUS status;
    ULONG_PTR information = 0;

    if (!Address || !AddressLength || !*AddressLength)
        return STATUS_INVALID_PARAMETER;
    if (IoControlCode != IOCTL_AFD_GET_ADDRESS && IoControlCode != IOCTL_AFD_GET_REMOTE_ADDRESS)
        return STATUS_INVALID_PARAMETER;

    status = PhpAfdRun(
        SocketHandle,
        IoControlCode,
        NULL,
        0,
        Address,
        *AddressLength,
        &information
        );

    if (NT_SUCCESS(status))
    {
        *AddressLength = (ULONG)information;
    }

    return status;
}

/**
 * Shuts down sending and/or receiving on an AFD socket.
 *
 * \param[in] SocketHandle An AFD socket handle.
 * \param[in] DisconnectMode A bit mask of AFD_PARTIAL_DISCONNECT_* / AFD_ABORTIVE_DISCONNECT flags.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdDisconnect(
    _In_ HANDLE SocketHandle,
    _In_ ULONG DisconnectMode
    )
{
    AFD_PARTIAL_DISCONNECT_INFO disconnectInfo = { 0 };

    disconnectInfo.DisconnectMode = DisconnectMode;

    return PhpAfdRun(
        SocketHandle,
        IOCTL_AFD_PARTIAL_DISCONNECT,
        &disconnectInfo,
        sizeof(AFD_PARTIAL_DISCONNECT_INFO),
        NULL,
        0,
        NULL
        );
}

/**
 * Sends a datagram to a remote address.
 *
 * \param[in] SocketHandle A datagram AFD socket handle.
 * \param[in] Address The remote IPv4 address.
 * \param[in] AddressLength The size of the address.
 * \param[in] Buffer The data to send.
 * \param[in] Length The length of the data.
 * \param[out] BytesTransferred Optionally receives the number of bytes sent.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdSendTo(
    _In_ HANDLE SocketHandle,
    _In_reads_bytes_(AddressLength) PSOCKADDR Address,
    _In_ ULONG AddressLength,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_opt_ PULONG BytesTransferred
    )
{
    NTSTATUS status;
    AFD_SEND_DATAGRAM_INFO datagramInfo = { 0 };
    WSABUF wsaBuffer;
    ULONG_PTR information = 0;

    if (BytesTransferred)
    {
        *BytesTransferred = 0;
    }

    if (!Address || AddressLength < sizeof(SOCKADDR_IN) || Address->sa_family != AF_INET)
        return STATUS_INVALID_PARAMETER;
    if (!Buffer || !Length)
        return STATUS_INVALID_PARAMETER;

    wsaBuffer.buf = Buffer;
    wsaBuffer.len = Length;

    datagramInfo.BufferArray = &wsaBuffer;
    datagramInfo.BufferCount = 1;
    datagramInfo.TdiConnInfo.RemoteAddress = Address;
    datagramInfo.TdiConnInfo.RemoteAddressLength = AddressLength;

    status = PhpAfdRun(
        SocketHandle,
        IOCTL_AFD_SEND_DATAGRAM,
        &datagramInfo,
        sizeof(AFD_SEND_DATAGRAM_INFO),
        NULL,
        0,
        &information
        );

    if (NT_SUCCESS(status) && BytesTransferred)
    {
        *BytesTransferred = (ULONG)information;
    }

    return status;
}

/**
 * Receives a datagram and optionally its source address.
 *
 * \param[in] SocketHandle A datagram AFD socket handle.
 * \param[out] Buffer Receives the data.
 * \param[in] Length The size of the buffer.
 * \param[out] Address Optionally receives the source address.
 * \param[in,out] AddressLength Required with Address. On input, the size of the address buffer; on output, the size of the address.
 * \param[out] BytesTransferred Optionally receives the number of bytes received.
 * \return Successful or errant status.
 */
NTSTATUS PhAfdReceiveFrom(
    _In_ HANDLE SocketHandle,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_writes_bytes_to_opt_(*AddressLength, *AddressLength) PSOCKADDR Address,
    _Inout_opt_ PULONG AddressLength,
    _Out_opt_ PULONG BytesTransferred
    )
{
    NTSTATUS status;
    AFD_DATAGRAM_INFO datagramInfo = { 0 };
    WSABUF wsaBuffer;
    ULONG_PTR information = 0;

    if (BytesTransferred)
    {
        *BytesTransferred = 0;
    }

    if (!Buffer || !Length)
        return STATUS_INVALID_PARAMETER;
    if (!!Address != !!AddressLength)
        return STATUS_INVALID_PARAMETER;
    if (AddressLength && *AddressLength < sizeof(SOCKADDR_IN))
        return STATUS_INVALID_PARAMETER;

    wsaBuffer.buf = Buffer;
    wsaBuffer.len = Length;

    datagramInfo.BufferArray = &wsaBuffer;
    datagramInfo.BufferCount = 1;
    datagramInfo.TdiFlags = TDI_RECEIVE_NORMAL;
    datagramInfo.Address = Address;
    datagramInfo.AddressLength = AddressLength;

    status = PhpAfdRun(
        SocketHandle,
        IOCTL_AFD_RECEIVE_DATAGRAM,
        &datagramInfo,
        sizeof(AFD_DATAGRAM_INFO),
        NULL,
        0,
        &information
        );

    if (NT_SUCCESS(status) && BytesTransferred)
    {
        *BytesTransferred = (ULONG)information;
    }

    return status;
}

static PH_WORK_QUEUE PhpAfdWorkQueue;
static PH_INITONCE PhpAfdWorkQueueInitOnce = PH_INITONCE_INIT;

/**
 * Retrieves the shared work queue for AFD socket operations.
 *
 * \return A pointer to the work queue, created on first use.
 */
PPH_WORK_QUEUE PhAfdGetWorkQueue(
    VOID
    )
{
    if (PhBeginInitOnce(&PhpAfdWorkQueueInitOnce))
    {
        PhInitializeWorkQueue(&PhpAfdWorkQueue, 2, 4, ULONG_MAX);
        PhEndInitOnce(&PhpAfdWorkQueueInitOnce);
    }

    return &PhpAfdWorkQueue;
}
