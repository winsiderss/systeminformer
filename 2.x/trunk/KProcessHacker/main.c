/*
 * KProcessHacker
 * 
 * Copyright (C) 2010 wj32
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <kph.h>

NTSTATUS KphpReadDriverParameters(
    __in PUNICODE_STRING RegistryPath
    );

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;
DRIVER_DISPATCH KphDispatchCreate;
DRIVER_DISPATCH KphDispatchDeviceControl;

PDRIVER_OBJECT KphDriverObject;
PDEVICE_OBJECT KphDeviceObject;
ULONG KphFeatures;
KPH_PARAMETERS KphParameters;

NTSTATUS DriverEntry(
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;
    UNICODE_STRING deviceName;
    PDEVICE_OBJECT deviceObject;

    KphDriverObject = DriverObject;

    if (!NT_SUCCESS(status = KphpReadDriverParameters(RegistryPath)))
        return status;

    // Create the device.

    RtlInitUnicodeString(&deviceName, KPH_DEVICE_NAME);

    status = IoCreateDevice(
        DriverObject,
        0,
        &deviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &deviceObject
        );

    if (!NT_SUCCESS(status))
        return status;

    KphDeviceObject = deviceObject;

    // Set up I/O.

    DriverObject->MajorFunction[IRP_MJ_CREATE] = KphDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = KphDispatchDeviceControl;
    DriverObject->DriverUnload = DriverUnload;

    deviceObject->Flags |= DO_BUFFERED_IO;
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    dprintf(L"Driver loaded\n");

    return status;
}

VOID DriverUnload(
    __in PDRIVER_OBJECT DriverObject
    )
{
    dprintf(L"Driver unloaded\n");
}

NTSTATUS KphDispatchCreate(
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP Irp
    )
{
    NTSTATUS status;
    PIO_STACK_LOCATION stackLocation;
    PIO_SECURITY_CONTEXT securityContext;

    stackLocation = IoGetCurrentIrpStackLocation(Irp);
    securityContext = stackLocation->Parameters.Create.SecurityContext;

    if (KphParameters.SecurityLevel == KphSecurityPrivilegeCheck)
    {
        UCHAR requiredPrivilegesBuffer[FIELD_OFFSET(PRIVILEGE_SET, Privilege) + sizeof(LUID_AND_ATTRIBUTES)];
        PPRIVILEGE_SET requiredPrivileges;

        // Check for SeDebugPrivilege.

        requiredPrivileges = (PPRIVILEGE_SET)requiredPrivilegesBuffer;
        requiredPrivileges->PrivilegeCount = 1;
        requiredPrivileges->Control = PRIVILEGE_SET_ALL_NECESSARY;
        requiredPrivileges->Privilege[0].Luid.LowPart = SE_DEBUG_PRIVILEGE;
        requiredPrivileges->Privilege[0].Luid.HighPart = 0;
        requiredPrivileges->Privilege[0].Attributes = 0;

        if (!SePrivilegeCheck(
            requiredPrivileges,
            &securityContext->AccessState->SubjectSecurityContext,
            Irp->RequestorMode
            ))
        {
            status = STATUS_PRIVILEGE_NOT_HELD;
        }
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS KphDispatchDeviceControl(
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP Irp
    )
{
    NTSTATUS status;
    PIO_STACK_LOCATION stackLocation;
    PVOID systemBuffer;
    ULONG inputBufferLength;
    ULONG ioControlCode;
    ULONG returnLength;

#define VERIFY_INPUT_LENGTH \
    do { if (inputBufferLength != sizeof(*input)) \
    { \
        status = STATUS_INFO_LENGTH_MISMATCH; \
        goto ControlEnd; \
    } } while (0)

    stackLocation = IoGetCurrentIrpStackLocation(Irp);
    systemBuffer = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength = stackLocation->Parameters.DeviceIoControl.InputBufferLength;
    ioControlCode = stackLocation->Parameters.DeviceIoControl.IoControlCode;
    returnLength = 0;

    if (systemBuffer)
    {
        switch (ioControlCode)
        {
        case KPH_GETFEATURES:
            {
                struct
                {
                    PULONG Features;
                } *input = systemBuffer;

                VERIFY_INPUT_LENGTH;

                status = KpiGetFeatures(
                    input->Features,
                    Irp->RequestorMode
                    );
            }
            break;
        case KPH_OPENPROCESS:
            {
                struct
                {
                    PHANDLE ProcessHandle;
                    ACCESS_MASK DesiredAccess;
                    PCLIENT_ID ClientId;
                } *input = systemBuffer;

                VERIFY_INPUT_LENGTH;

                status = KpiOpenProcess(
                    input->ProcessHandle,
                    input->DesiredAccess,
                    input->ClientId,
                    Irp->RequestorMode
                    );
            }
            break;
        case KPH_OPENPROCESSTOKEN:
            {
                struct
                {
                    HANDLE ProcessHandle;
                    ACCESS_MASK DesiredAccess;
                    PHANDLE TokenHandle;
                } *input = systemBuffer;

                VERIFY_INPUT_LENGTH;

                status = KpiOpenProcessToken(
                    input->ProcessHandle,
                    input->DesiredAccess,
                    input->TokenHandle,
                    Irp->RequestorMode
                    );
            }
            break;
        case KPH_OPENPROCESSJOB:
            {
                struct
                {
                    HANDLE ProcessHandle;
                    ACCESS_MASK DesiredAccess;
                    PHANDLE JobHandle;
                } *input = systemBuffer;

                VERIFY_INPUT_LENGTH;

                status = KpiOpenProcessJob(
                    input->ProcessHandle,
                    input->DesiredAccess,
                    input->JobHandle,
                    Irp->RequestorMode
                    );
            }
            break;
        case KPH_SUSPENDPROCESS:
            {
                struct
                {
                    HANDLE ProcessHandle;
                } *input = systemBuffer;

                VERIFY_INPUT_LENGTH;

                status = KpiSuspendProcess(
                    input->ProcessHandle,
                    Irp->RequestorMode
                    );
            }
            break;
        case KPH_RESUMEPROCESS:
            {
                struct
                {
                    HANDLE ProcessHandle;
                } *input = systemBuffer;

                VERIFY_INPUT_LENGTH;

                status = KpiResumeProcess(
                    input->ProcessHandle,
                    Irp->RequestorMode
                    );
            }
            break;
        }
    }

ControlEnd:
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = returnLength;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

ULONG KphpReadIntegerParameter(
    __in HANDLE KeyHandle,
    __in PUNICODE_STRING ValueName,
    __in ULONG DefaultValue
    )
{
    NTSTATUS status;
    UCHAR buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION info;
    ULONG resultLength;

    info = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;

    status = ZwQueryValueKey(
        KeyHandle,
        ValueName,
        KeyValuePartialInformation,
        info,
        sizeof(buffer),
        &resultLength
        );

    if (!NT_SUCCESS(status))
    {
        dprintf("Unable to query parameter %.*s: 0x%x", ValueName->Length / sizeof(WCHAR), ValueName->Buffer, status);
        return DefaultValue;
    }

    return *(PULONG)info->Data;
}

NTSTATUS KphpReadDriverParameters(
    __in PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;
    HANDLE parametersKeyHandle;
    UNICODE_STRING parametersString;
    UNICODE_STRING parametersKeyName;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING valueName;

    // Open the Parameters key.

    RtlInitUnicodeString(&parametersString, L"\\Parameters");

    parametersKeyName.Length = RegistryPath->Length + parametersString.Length;
    parametersKeyName.MaximumLength = parametersKeyName.Length + sizeof(WCHAR);
    parametersKeyName.Buffer = ExAllocatePoolWithTag(parametersKeyName.MaximumLength, PagedPool, 'ThpK');

    if (!parametersKeyName.Buffer)
        return STATUS_INSUFFICIENT_RESOURCES;

    memcpy(parametersKeyName.Buffer, RegistryPath->Buffer, RegistryPath->Length);
    memcpy(&parametersKeyName.Buffer[RegistryPath->Length / sizeof(WCHAR)], parametersString.Buffer, parametersString.Length);

    InitializeObjectAttributes(
        &objectAttributes,
        &parametersKeyName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
        );

    status = ZwOpenKey(
        &parametersKeyHandle,
        KEY_READ,
        &objectAttributes
        );
    ExFreePoolWithTag(parametersKeyName.Buffer, 'ThpK');

    if (!NT_SUCCESS(status))
        return status;

    // Read in the parameters.

    RtlInitUnicodeString(&valueName, L"SecurityLevel");
    KphParameters.SecurityLevel = KphpReadIntegerParameter(parametersKeyHandle, &valueName, KphSecurityNone);

    ZwClose(parametersKeyHandle);

    return status;
}

NTSTATUS KpiGetFeatures(
    __out PULONG Features,
    __in KPROCESSOR_MODE AccessMode
    )
{
    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(Features, sizeof(ULONG), sizeof(ULONG));
            *Features = KphFeatures;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }
    else
    {
        *Features = KphFeatures;
    }

    return STATUS_SUCCESS;
}
