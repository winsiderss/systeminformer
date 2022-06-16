/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     jxy-s   2021
 *
 */

#include <kph.h>
#include <dyndata.h>

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;
_Dispatch_type_(IRP_MJ_CREATE) DRIVER_DISPATCH KphDispatchCreate;
_Dispatch_type_(IRP_MJ_CLOSE) DRIVER_DISPATCH KphDispatchClose;

KPH_EXTENTS NtdllExtents;
UNICODE_STRING NtdllKnownDllName = RTL_CONSTANT_STRING(L"\\KnownDlls\\ntdll.dll");

ULONG KphpReadIntegerParameter(
    _In_opt_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _In_ ULONG DefaultValue
    );

NTSTATUS KphpReadDriverParameters(
    _In_ PUNICODE_STRING RegistryPath
    );

NTSTATUS KpiPopulateKnownDllExtents(
    _In_ PUNICODE_STRING SectionName,
    _Out_ PKPH_EXTENTS ModuleExtents
    );

NTSTATUS KpiKnownDllInit(
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DriverEntry)
#pragma alloc_text(PAGE, DriverUnload)
#pragma alloc_text(PAGE, KphpReadIntegerParameter)
#pragma alloc_text(PAGE, KphpReadDriverParameters)
#pragma alloc_text(PAGE, KpiGetFeatures)
#pragma alloc_text(PAGE, KpiPopulateKnownDllExtents)
#pragma alloc_text(PAGE, KpiKnownDllInit)
#endif

PDRIVER_OBJECT KphDriverObject;
PDEVICE_OBJECT KphDeviceObject;
ULONG KphFeatures;

NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;
    UNICODE_STRING deviceName;
    PDEVICE_OBJECT deviceObject;

    PAGED_CODE();

    ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

    KphDriverObject = DriverObject;

    if (!NT_SUCCESS(status = KphDynamicDataInitialization()))
        return status;

    KphDynamicImport();

    if (!NT_SUCCESS(status = KphpReadDriverParameters(RegistryPath)))
        return status;

    if (!NT_SUCCESS(status = KpiKnownDllInit()))
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
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = KphDispatchClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = KphDispatchDeviceControl;
    DriverObject->DriverUnload = DriverUnload;

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    dprintf("Driver loaded\n");

    return status;
}

VOID DriverUnload(
    _In_ PDRIVER_OBJECT DriverObject
    )
{
    PAGED_CODE();

    IoDeleteDevice(KphDeviceObject);

    dprintf("Driver unloaded\n");
}

NTSTATUS KphDispatchCreate(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION stackLocation;
    PFILE_OBJECT fileObject;
    PIO_SECURITY_CONTEXT securityContext;
    PKPH_CLIENT client;
    UCHAR requiredPrivilegesBuffer[FIELD_OFFSET(PRIVILEGE_SET, Privilege) + sizeof(LUID_AND_ATTRIBUTES)];
    PPRIVILEGE_SET requiredPrivileges;

    stackLocation = IoGetCurrentIrpStackLocation(Irp);
    fileObject = stackLocation->FileObject;
    securityContext = stackLocation->Parameters.Create.SecurityContext;

    dprintf("Client (PID %lu) is connecting\n", HandleToUlong(PsGetCurrentProcessId()));

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
        dprintf("Client (PID %lu) was rejected\n", HandleToUlong(PsGetCurrentProcessId()));
    }

    if (NT_SUCCESS(status))
    {
        client = ExAllocatePoolZero(PagedPool, sizeof(KPH_CLIENT), 'ChpK');

        if (client)
        {
            ExInitializeFastMutex(&client->StateMutex);
            ExInitializeFastMutex(&client->KeyBackoffMutex);

            fileObject->FsContext = client;
        }
        else
        {
            dprintf("Unable to allocate memory for client (PID %lu)\n", HandleToUlong(PsGetCurrentProcessId()));
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS KphDispatchClose(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION stackLocation;
    PFILE_OBJECT fileObject;
    PKPH_CLIENT client;

    stackLocation = IoGetCurrentIrpStackLocation(Irp);
    fileObject = stackLocation->FileObject;
    client = fileObject->FsContext;

    if (client)
    {
        ExFreePoolWithTag(client, 'ChpK');
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

/**
 * Reads an integer (REG_DWORD) parameter from the registry.
 *
 * \param KeyHandle A handle to the Parameters key. If NULL, the function
 * fails immediately and returns \a DefaultValue.
 * \param ValueName The name of the parameter.
 * \param DefaultValue The value that is returned if the function fails
 * to retrieve the parameter from the registry.
 *
 * \return The parameter value, or \a DefaultValue if the function failed.
 */
ULONG KphpReadIntegerParameter(
    _In_opt_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _In_ ULONG DefaultValue
    )
{
    NTSTATUS status;
    UCHAR buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION info;
    ULONG resultLength;

    PAGED_CODE();

    if (!KeyHandle)
        return DefaultValue;

    info = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;

    status = ZwQueryValueKey(
        KeyHandle,
        ValueName,
        KeyValuePartialInformation,
        info,
        sizeof(buffer),
        &resultLength
        );

    if (info->Type != REG_DWORD)
        status = STATUS_OBJECT_TYPE_MISMATCH;

    if (!NT_SUCCESS(status))
    {
        dprintf("Unable to query parameter %.*S: 0x%x\n", ValueName->Length / (USHORT)sizeof(WCHAR), ValueName->Buffer, status);
        return DefaultValue;
    }

    return *(PULONG)info->Data;
}

/**
 * Reads the driver parameters.
 *
 * \param RegistryPath The registry path of the driver.
 */
NTSTATUS KphpReadDriverParameters(
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;
    HANDLE parametersKeyHandle;
    UNICODE_STRING parametersString;
    UNICODE_STRING parametersKeyName;
    OBJECT_ATTRIBUTES objectAttributes;

    PAGED_CODE();

    // Open the Parameters key.

    RtlInitUnicodeString(&parametersString, L"\\Parameters");

    parametersKeyName.Length = RegistryPath->Length + parametersString.Length;
    parametersKeyName.MaximumLength = parametersKeyName.Length;
    parametersKeyName.Buffer = ExAllocatePoolZero(PagedPool, parametersKeyName.MaximumLength, 'ThpK');

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
    {
        dprintf("Unable to open Parameters key: 0x%x\n", status);
        status = STATUS_SUCCESS;
        parametersKeyHandle = NULL;
        // Continue so we can set up defaults.
    }

    // Read in the parameters.

    KphReadDynamicDataParameters(parametersKeyHandle);

    if (parametersKeyHandle)
        ZwClose(parametersKeyHandle);

    return status;
}

NTSTATUS KpiGetFeatures(
    _Out_ PULONG Features,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    PAGED_CODE();

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

/**
 * Populates known DLL module extents.
 *
 * \param[in] SectionName - Name of the section to open.
 * \param[out] ModuleExtents - On success, populated with the module extents.
 *
 * \return Appropriate status.
*/
NTSTATUS KpiPopulateKnownDllExtents(
    _In_ PUNICODE_STRING SectionName,
    _Out_ PKPH_EXTENTS ModuleExtents
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE sectionHandle;
    SECTION_IMAGE_INFORMATION sectionImageInfo;
    PVOID sectionObject;
    PVOID mappedBase;
    SIZE_T mappedSize;

    NT_ASSERT(PsInitialSystemProcess == PsGetCurrentProcess());

    RtlZeroMemory(ModuleExtents, sizeof(*ModuleExtents));

    sectionHandle = NULL;
    sectionObject = NULL;
    mappedBase = NULL;
    mappedSize = 0;

    InitializeObjectAttributes(
        &objectAttributes,
        SectionName,
        OBJ_KERNEL_HANDLE,
        NULL,
        NULL
        );

    status = ZwOpenSection(
        &sectionHandle,
        SECTION_MAP_READ | SECTION_QUERY,
        &objectAttributes
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = ZwQuerySection(
        sectionHandle,
        SectionImageInformation,
        &sectionImageInfo,
        sizeof(sectionImageInfo),
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    //
    // 21H2 no longer maps ntdll as an image in System. Querying the transfer
    // address to get the extents in this context will fail. Rather than rely
    // on ZwQueryVirtualMemory at all, we'll map it into system space to get
    // the extents.
    //
    // Note, in the future if Microsoft decides to relocate KnownDLLs in
    // every process we'll need to revisit this. That will likely involve
    // retrieving the module extents per-process, in our case this is used
    // for KPH communications validation so we'll need to the ntdll module
    // extents out of PH.
    //

    status = ObReferenceObjectByHandle(
        sectionHandle,
        SECTION_MAP_READ | SECTION_QUERY,
        *MmSectionObjectType,
        KernelMode,
        &sectionObject,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        sectionObject = NULL;
        goto CleanupExit;
    }

    status = MmMapViewInSystemSpace(sectionObject, &mappedBase, &mappedSize);

    if (!NT_SUCCESS(status))
    {
        mappedBase = NULL;
        mappedSize = 0;
        goto CleanupExit;
    }

    ModuleExtents->BaseAddress = sectionImageInfo.TransferAddress;
    ModuleExtents->EndAddress = PTR_ADD_OFFSET(ModuleExtents->BaseAddress, mappedSize);

CleanupExit:

    if (mappedBase)
    {
        MmUnmapViewInSystemSpace(mappedBase);
    }

    if (sectionObject)
    {
        ObDereferenceObject(sectionObject);
    }

    if (sectionHandle)
    {
        ObCloseHandle(sectionHandle, KernelMode);
    }

    return status;
}

/**
 * Initializes known module extents.
 *
 * \return Appropriate status.
*/
NTSTATUS KpiKnownDllInit(
    VOID
    )
{
    return KpiPopulateKnownDllExtents(&NtdllKnownDllName, &NtdllExtents);
}
