/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2023
 *
 */

#include <ph.h>
#include <hvsocketcontrol.h>

#ifdef _WIN64

static const UNICODE_STRING HvSocketVmGroupSddlString = RTL_CONSTANT_STRING(L"D:P(A;;GA;;;BA)(A;;GA;;;SY)(A;;GA;;;S-1-5-83-0)(A;;GA;;;S-1-15-3-1024-2268835264-3721307629-241982045-173645152-1490879176-104643441-2915960892-1612460704)");
static const UNICODE_STRING HvSocketSystemDevicePath  = RTL_CONSTANT_STRING(L"\\Device\\HvSocketSystem"); // SDDL_DEVOBJ_SYS_ALL_ADM_ALL "D:P(A;;GA;;;SY)(A;;GA;;;BA)"
static const UNICODE_STRING HvSocketSystemSymLink     = RTL_CONSTANT_STRING(L"\\DosDevices\\HvSocketSystem");
static const UNICODE_STRING HvSocketVmGroupDevicePath = RTL_CONSTANT_STRING(L"\\Device\\HvSocket"); // HvSocketVmGroupSddlString
static const UNICODE_STRING HvSocketVmGroupSymLink    = RTL_CONSTANT_STRING(L"\\DosDevices\\HvSocket");
static const UNICODE_STRING HvSocketControlName       = RTL_CONSTANT_STRING(L"HvSocketControl");
static const UNICODE_STRING HvSocketAddressInfoName   = RTL_CONSTANT_STRING(L"AddressInfo");
static const UNICODE_STRING HvSocketControlFileName   = RTL_CONSTANT_STRING(L"\\Device\\HvSocketSystem\\HvSocketControl");

static const OBJECT_ATTRIBUTES HvSocketSystemDeviceAttributes  = RTL_CONSTANT_OBJECT_ATTRIBUTES((PUNICODE_STRING)&HvSocketSystemDevicePath, 0);
static const OBJECT_ATTRIBUTES HvSocketVmGroupDeviceAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES((PUNICODE_STRING)&HvSocketVmGroupDevicePath, 0);
static const OBJECT_ATTRIBUTES HvSocketControlAttributes       = RTL_CONSTANT_OBJECT_ATTRIBUTES((PUNICODE_STRING)&HvSocketControlFileName, 0);

#define IOCTL_HVSOCKET_UPDATE_ADDRESS_INFO          CTL_CODE(FILE_DEVICE_TRANSPORT, 0x1, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_HVSOCKET_UPDATE_PARTITION_PROPERTIES  CTL_CODE(FILE_DEVICE_TRANSPORT, 0x2, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_HVSOCKET_UPDATE_SERVICE_INFO          CTL_CODE(FILE_DEVICE_TRANSPORT, 0x3, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_HVSOCKET_GET_SERVICE_INFO             CTL_CODE(FILE_DEVICE_TRANSPORT, 0x4, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_HVSOCKET_UPDATE_SERVICE_TABLE         CTL_CODE(FILE_DEVICE_TRANSPORT, 0x5, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_HVSOCKET_UPDATE_PROVIDER_PROPERTIES   CTL_CODE(FILE_DEVICE_TRANSPORT, 0x6, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_HVSOCKET_GET_PARTITION_LISTENERS      CTL_CODE(FILE_DEVICE_TRANSPORT, 0x7, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_HVSOCKET_GET_PARTITION_CONNECTIONS    CTL_CODE(FILE_DEVICE_TRANSPORT, 0x8, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define HVSOCKET_SYSTEM_PATH_LENGTH  (sizeof(L"\\Device\\HvSocketSystem") /* \\ */ + sizeof(L"AddressInfo") /* \\ */ + sizeof(L"{00000000-0000-0000-0000-000000000000}"))

NTSTATUS HvSocketOpenSystemControl(
    _Out_ PHANDLE SystemHandle,
    _In_opt_ const GUID* VmId
    )
{
    IO_STATUS_BLOCK ioStatusBlock;
    BYTE buffer[HVSOCKET_SYSTEM_PATH_LENGTH];
    UNICODE_STRING systemPath;
    UNICODE_STRING guidString;
    OBJECT_ATTRIBUTES objectAttributes;

    if (!VmId)
    {
        return NtCreateFile(
            SystemHandle,
            FILE_READ_ACCESS | FILE_WRITE_ACCESS | SYNCHRONIZE,
            (POBJECT_ATTRIBUTES)&HvSocketControlAttributes,
            &ioStatusBlock,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_CREATE, // required by hvsocketcontrol.sys
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
            NULL,
            0
            );
    }

    systemPath.Buffer = (PWCHAR)buffer;
    systemPath.Length = 0;
    systemPath.MaximumLength = sizeof(buffer);
    RtlAppendUnicodeStringToString(&systemPath, &HvSocketSystemDevicePath);
    RtlAppendUnicodeToString(&systemPath, L"\\");
    RtlAppendUnicodeStringToString(&systemPath, &HvSocketAddressInfoName);
    RtlAppendUnicodeToString(&systemPath, L"\\");

    guidString.Buffer = &systemPath.Buffer[systemPath.Length / 2];
    guidString.Length = 0;
    guidString.MaximumLength = systemPath.MaximumLength - systemPath.Length;
    RtlStringFromGUIDEx((PGUID)VmId, &guidString, FALSE);

    systemPath.Length += guidString.Length;

    InitializeObjectAttributes(&objectAttributes, &systemPath, 0, NULL, NULL);

    return NtCreateFile(
        SystemHandle,
        FILE_READ_ACCESS | FILE_WRITE_ACCESS | SYNCHRONIZE,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_CREATE, // required by hvsocketcontrol.sys
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
        );
}

NTSTATUS HvSocketGetListeners(
    _In_ HANDLE SystemHandle,
    _In_ const GUID* VmId,
    _In_opt_ PHVSOCKET_LISTENERS Listeners,
    _In_ ULONG ListenersLength,
    _Out_opt_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;

    RtlZeroMemory(&ioStatusBlock, sizeof(IO_STATUS_BLOCK));

    status = NtDeviceIoControlFile(
        SystemHandle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
        IOCTL_HVSOCKET_GET_PARTITION_LISTENERS,
        (PVOID)VmId,
        sizeof(GUID),
        Listeners,
        ListenersLength
        );

    if (ReturnLength)
    {
        *ReturnLength = (ULONG)ioStatusBlock.Information;
    }

    return status;
}

NTSTATUS HvSocketGetConnections(
    _In_ HANDLE SystemHandle,
    _In_ const GUID* VmId,
    _In_opt_ PHVSOCKET_CONNECTIONS Connections,
    _In_ ULONG ConnectionsLength,
    _Out_opt_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;

    RtlZeroMemory(&ioStatusBlock, sizeof(IO_STATUS_BLOCK));

    status = NtDeviceIoControlFile(
        SystemHandle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
        IOCTL_HVSOCKET_GET_PARTITION_CONNECTIONS,
        (PVOID)VmId,
        sizeof(GUID),
        Connections,
        ConnectionsLength
        );

    if (ReturnLength)
    {
        *ReturnLength = (ULONG)ioStatusBlock.Information;
    }

    return status;
}

NTSTATUS HvSocketGetServiceInfo(
    _In_ HANDLE SystemHandle,
    _In_ const GUID* ServiceId,
    _Out_ PHVSOCKET_SERVICE_INFO ServiceInfo
    )
{
    //
    // This can bug check the system (jxy-s)
    //
    //IO_STATUS_BLOCK ioStatusBlock;
    //
    //return NtDeviceIoControlFile(
    //    SystemHandle,
    //    NULL,
    //    NULL,
    //    NULL,
    //    &ioStatusBlock,
    //    IOCTL_HVSOCKET_GET_SERVICE_INFO,
    //    (PVOID)ServiceId,
    //    sizeof(GUID),
    //    ServiceInfo,
    //    sizeof(HVSOCKET_SERVICE_INFO)
    //    );

    UNREFERENCED_PARAMETER(SystemHandle);
    UNREFERENCED_PARAMETER(ServiceId);
    UNREFERENCED_PARAMETER(ServiceInfo);
    return STATUS_NOT_IMPLEMENTED;
}

#endif // _WIN64
