/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2023-2025
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

static const OBJECT_ATTRIBUTES HvSocketSystemDeviceAttributes  = RTL_CONSTANT_OBJECT_ATTRIBUTES(&HvSocketSystemDevicePath, 0);
static const OBJECT_ATTRIBUTES HvSocketVmGroupDeviceAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(&HvSocketVmGroupDevicePath, 0);
static const OBJECT_ATTRIBUTES HvSocketControlAttributes       = RTL_CONSTANT_OBJECT_ATTRIBUTES(&HvSocketControlFileName, 0);

#define IOCTL_HVSOCKET_UPDATE_ADDRESS_INFO          CTL_CODE(FILE_DEVICE_TRANSPORT, 0x1, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_HVSOCKET_UPDATE_PARTITION_PROPERTIES  CTL_CODE(FILE_DEVICE_TRANSPORT, 0x2, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_HVSOCKET_UPDATE_SERVICE_INFO          CTL_CODE(FILE_DEVICE_TRANSPORT, 0x3, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_HVSOCKET_GET_SERVICE_INFO             CTL_CODE(FILE_DEVICE_TRANSPORT, 0x4, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_HVSOCKET_UPDATE_SERVICE_TABLE         CTL_CODE(FILE_DEVICE_TRANSPORT, 0x5, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_HVSOCKET_UPDATE_PROVIDER_PROPERTIES   CTL_CODE(FILE_DEVICE_TRANSPORT, 0x6, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_HVSOCKET_GET_PARTITION_LISTENERS      CTL_CODE(FILE_DEVICE_TRANSPORT, 0x7, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_HVSOCKET_GET_PARTITION_CONNECTIONS    CTL_CODE(FILE_DEVICE_TRANSPORT, 0x8, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define HVSOCKET_SYSTEM_PATH_LENGTH  (sizeof(L"\\Device\\HvSocketSystem") /* \\ */ + sizeof(L"AddressInfo") /* \\ */ + sizeof(L"{00000000-0000-0000-0000-000000000000}"))

NTSTATUS PhHvSocketOpenSystemControl(
    _Out_ PHANDLE SystemHandle,
    _In_opt_ const GUID* VmId
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;

    if (VmId)
    {
#if defined(PHNT_USE_NATIVE_APPEND)
        UNICODE_STRING systemPath;
        UNICODE_STRING guidString;
        OBJECT_ATTRIBUTES objectAttributes;
        BYTE buffer[HVSOCKET_SYSTEM_PATH_LENGTH];

        RtlInitEmptyUnicodeString(&systemPath, (PWCHAR)buffer, sizeof(buffer));

        status = RtlAppendUnicodeStringToString(&systemPath, &HvSocketSystemDevicePath);
        if (!NT_SUCCESS(status))
            return status;

        status = RtlAppendUnicodeToString(&systemPath, L"\\");
        if (!NT_SUCCESS(status))
            return status;

        status = RtlAppendUnicodeStringToString(&systemPath, &HvSocketAddressInfoName);
        if (!NT_SUCCESS(status))
            return status;

        status = RtlAppendUnicodeToString(&systemPath, L"\\");
        if (!NT_SUCCESS(status))
            return status;

        status = RtlStringFromGUID((PGUID)VmId, &guidString);
        if (!NT_SUCCESS(status))
            return status;

        status = RtlAppendUnicodeStringToString(&systemPath, &guidString);
        RtlFreeUnicodeString(&guidString);

        if (!NT_SUCCESS(status))
            return status;
#else
        PPH_STRING gidString;
        UNICODE_STRING objectName;
        OBJECT_ATTRIBUTES objectAttributes;
        PH_STRINGREF stringRef;
        SIZE_T returnLength;
        PH_FORMAT format[5];
        WCHAR formatBuffer[0x100];

        if (!(gidString = PhFormatGuid((PGUID)VmId)))
            return STATUS_NO_MEMORY;

        PhInitFormatUCS(&format[0], &HvSocketSystemDevicePath);
        PhInitFormatSR(&format[1], PhNtPathSeperatorString);
        PhInitFormatUCS(&format[2], &HvSocketAddressInfoName);
        PhInitFormatSR(&format[3], PhNtPathSeperatorString);
        PhInitFormatSR(&format[4], gidString->sr);

        if (!PhFormatToBuffer(format, 1, formatBuffer, sizeof(formatBuffer), &returnLength))
        {
            PhDereferenceObject(gidString);
            return STATUS_NO_MEMORY;
        }

        PhDereferenceObject(gidString);

        stringRef.Length = returnLength - sizeof(UNICODE_NULL);
        stringRef.Buffer = formatBuffer;

        if (!PhStringRefToUnicodeString(&stringRef, &objectName))
            return STATUS_NO_MEMORY;
#endif

        InitializeObjectAttributes(
            &objectAttributes,
            &objectName,
            0,
            NULL,
            NULL
            );

        status = NtCreateFile(
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
    else
    {
        status = NtCreateFile(
            SystemHandle,
            FILE_READ_ACCESS | FILE_WRITE_ACCESS | SYNCHRONIZE,
            &HvSocketControlAttributes,
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

    return status;
}

NTSTATUS PhHvSocketGetListeners(
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

NTSTATUS PhHvSocketGetConnections(
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

NTSTATUS PhHvSocketGetServiceInfo(
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

BOOLEAN PhHvSocketIsVSockTemplate(
    _In_ PGUID ServiceId
    )
{
    GUID template = *ServiceId;

    template.Data1 = 0;

    return !!IsEqualGUID(&template, &HV_GUID_VSOCK_TEMPLATE);
}

_Maybenull_
PPH_STRING PhHvSocketGetVmName(
    _In_ PGUID VmId
    )
{
    static const PH_STRINGREF hvComputeSystemKey = PH_STRINGREF_INIT(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\HostComputeService\\VolatileStore\\ComputeSystem\\");
    static const PH_STRINGREF trimSet = PH_STRINGREF_INIT(L"{}");
    PPH_STRING vmName = NULL;
    PPH_STRING guidString;
    PH_STRINGREF guidStringTrimmed;
    PPH_STRING keyName;
    HANDLE keyHandle;

    guidString = PhFormatGuid(VmId);
    guidStringTrimmed = guidString->sr;
    PhTrimStringRef(&guidStringTrimmed, &trimSet, 0);

    keyName = PhConcatStringRef2(&hvComputeSystemKey, &guidStringTrimmed);

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_QUERY_VALUE,
        PH_KEY_LOCAL_MACHINE,
        &keyName->sr,
        0
        )))
    {
        PPH_STRING value;

        if (value = PhQueryRegistryStringZ(keyHandle, L"VmName"))
            PhMoveReference(&vmName, value);
        else if (value = PhQueryRegistryStringZ(keyHandle, L"VmId"))
            PhMoveReference(&vmName, value);
        else
            PhMoveReference(&vmName, PhCreateString3(&guidStringTrimmed, PH_STRING_UPPER_CASE, NULL));

        NtClose(keyHandle);
    }

    PhDereferenceObject(keyName);
    PhDereferenceObject(guidString);

    return vmName;
}

_Maybenull_
PPH_STRING PhHvSocketGetServiceName(
    _In_ PGUID ServiceId
    )
{
    static const PH_STRINGREF hvGuestServicesKey = PH_STRINGREF_INIT(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Virtualization\\GuestCommunicationServices\\");
    static const PH_STRINGREF trimSet = PH_STRINGREF_INIT(L"{}");
    PPH_STRING serviceName = NULL;
    PPH_STRING guidString;
    PH_STRINGREF guidStringTrimmed;
    PPH_STRING keyName;
    HANDLE keyHandle;

    guidString = PhFormatGuid(ServiceId);
    guidStringTrimmed = guidString->sr;
    PhTrimStringRef(&guidStringTrimmed, &trimSet, 0);

    keyName = PhConcatStringRef2(&hvGuestServicesKey, &guidStringTrimmed);

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_QUERY_VALUE,
        PH_KEY_LOCAL_MACHINE,
        &keyName->sr,
        0
        )))
    {
        PPH_STRING elementName;

        if (elementName = PhQueryRegistryStringZ(keyHandle, L"ElementName"))
            PhMoveReference(&serviceName, elementName);

        NtClose(keyHandle);
    }

    // Keep this after element name lookup. Certain services define the VSOCK
    // template with a specific port (e.g. Docker). (jxy-s)
    if (!serviceName && PhHvSocketIsVSockTemplate(ServiceId))
        PhMoveReference(&serviceName, PhCreateString(L"VSOCK"));

    PhDereferenceObject(keyName);
    PhDereferenceObject(guidString);

    return serviceName;
}

PPH_STRING PhHvSocketAddressString(
    _In_ PGUID Address
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PPH_STRING wildcardString;
    static PPH_STRING broadcastString;
    static PPH_STRING childrenString;
    static PPH_STRING loopbackString;
    static PPH_STRING hostString;
    static PPH_STRING siloHostString;
    PPH_STRING addressString = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        wildcardString = PhCreateString(L"*");
        broadcastString = PhCreateString(L"Broadcast");
        childrenString = PhCreateString(L"Children");
        loopbackString = PhCreateString(L"Loopback");
        hostString = PhCreateString(L"Host");
        siloHostString = PhCreateString(L"Silo Host");
        PhEndInitOnce(&initOnce);
    }

    if (IsEqualGUID(Address, &HV_GUID_WILDCARD))
        return PhReferenceObject(wildcardString);
    if (IsEqualGUID(Address, &HV_GUID_BROADCAST))
        return PhReferenceObject(broadcastString);
    if (IsEqualGUID(Address, &HV_GUID_CHILDREN))
        return PhReferenceObject(childrenString);
    if (IsEqualGUID(Address, &HV_GUID_LOOPBACK))
        return PhReferenceObject(loopbackString);
    if (IsEqualGUID(Address, &HV_GUID_PARENT))
        return PhReferenceObject(hostString);
    if (IsEqualGUID(Address, &HV_GUID_SILOHOST))
        return PhReferenceObject(siloHostString);

    return PhFormatGuid(Address);
}
