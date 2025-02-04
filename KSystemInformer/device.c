/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2024-2025
 *
 */

#include <kph.h>

#include <trace.h>

KPH_PAGED_FILE();

/**
 * \brief Open a device object.
 *
 * \param[out] DriverHandle Set to the opened handle to the device.
 * \param[in] DesiredAccess Desired access to the device object.
 * \param[in] ObjectAttributes Object attributes for opening the device object.
 * \param[in] AccessMode The mode in which to perform access checks.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphOpenDevice(
    _Out_ PHANDLE DeviceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    PFILE_OBJECT fileObject;
    PUNICODE_STRING capturedObjectName;
    OBJECT_ATTRIBUTES objectAttributes;
    POBJECT_ATTRIBUTES objectAttributesPtr;
    ULONG options;
    IO_STATUS_BLOCK ioStatusBlock;
    HANDLE deviceHandle;

    KPH_PAGED_CODE_PASSIVE();

    fileHandle = NULL;
    fileObject = NULL;
    capturedObjectName = NULL;

    if (AccessMode != KernelMode)
    {
        OBJECT_ATTRIBUTES capturedAttributes;

        __try
        {
            ProbeOutputType(DeviceHandle, HANDLE);
            ProbeInputType(ObjectAttributes, OBJECT_ATTRIBUTES);
            RtlCopyVolatileMemory(&capturedAttributes,
                                  ObjectAttributes,
                                  sizeof(OBJECT_ATTRIBUTES));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }

        if (capturedAttributes.ObjectName)
        {
            status = KphCaptureUnicodeString(capturedAttributes.ObjectName,
                                             &capturedObjectName);
            if (!NT_SUCCESS(status))
            {
                goto Exit;
            }
        }

        SetFlag(capturedAttributes.Attributes, OBJ_KERNEL_HANDLE);
        SetFlag(capturedAttributes.Attributes, OBJ_FORCE_ACCESS_CHECK);

        InitializeObjectAttributes(&objectAttributes,
                                   capturedObjectName,
                                   capturedAttributes.Attributes,
                                   capturedAttributes.RootDirectory,
                                   NULL);

        objectAttributesPtr = &objectAttributes;
        options = IO_FORCE_ACCESS_CHECK;
    }
    else
    {
        objectAttributesPtr = ObjectAttributes;
        options = 0;
    }

    status = KphCreateFile(&fileHandle,
                           DesiredAccess,
                           objectAttributesPtr,
                           &ioStatusBlock,
                           NULL,
                           0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN,
                           FILE_NON_DIRECTORY_FILE,
                           NULL,
                           0,
                           options,
                           KernelMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphCreateFile failed: %!STATUS!",
                      status);

        fileHandle = NULL;
        goto Exit;
    }

    status = ObReferenceObjectByHandle(fileHandle,
                                       0,
                                       *IoFileObjectType,
                                       KernelMode,
                                       &fileObject,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        fileObject = NULL;
        goto Exit;
    }

    status = ObOpenObjectByPointer(IoGetRelatedDeviceObject(fileObject),
                                   (AccessMode ? 0 : OBJ_KERNEL_HANDLE),
                                   NULL,
                                   DesiredAccess,
                                   *IoDeviceObjectType,
                                   AccessMode,
                                   &deviceHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObOpenObjectByPointer failed: %!STATUS!",
                      status);

        deviceHandle = NULL;
        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        __try
        {
            *DeviceHandle = deviceHandle;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            ObCloseHandle(deviceHandle, UserMode);
        }
    }
    else
    {
        *DeviceHandle = deviceHandle;
    }

Exit:

    if (fileObject)
    {
        ObDereferenceObject(fileObject);
    }

    if (fileHandle)
    {
        ObCloseHandle(fileHandle, KernelMode);
    }

    if (capturedObjectName)
    {
        KphReleaseUnicodeString(capturedObjectName);
    }

    return status;
}

/**
 * \brief Opens a driver object associated with a device object.
 *
 * \param[in] DeviceHandle Handle to a device object.
 * \param[in] DesiredAccess Desired access to the driver object.
 * \param[out] DriverHandle Set to the opened handle to the driver object.
 * \param[in] AccessMode The mode in which to perform access checks.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphOpenDeviceDriver(
    _In_ HANDLE DeviceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE DriverHandle,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject;
    HANDLE driverHandle;

    KPH_PAGED_CODE_PASSIVE();

    deviceObject = NULL;

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeOutputType(DriverHandle, HANDLE);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }

    status = ObReferenceObjectByHandle(DeviceHandle,
                                       DesiredAccess,
                                       *IoDeviceObjectType,
                                       AccessMode,
                                       &deviceObject,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        deviceObject = NULL;
        goto Exit;
    }

    status = ObOpenObjectByPointer(deviceObject->DriverObject,
                                   (AccessMode ? 0 : OBJ_KERNEL_HANDLE),
                                   NULL,
                                   DesiredAccess,
                                   *IoDriverObjectType,
                                   AccessMode,
                                   &driverHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObOpenObjectByPointer failed: %!STATUS!",
                      status);

        driverHandle = NULL;
        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        __try
        {
            *DriverHandle = driverHandle;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            ObCloseHandle(driverHandle, UserMode);
        }
    }
    else
    {
        *DriverHandle = driverHandle;
    }

Exit:

    if (deviceObject)
    {
        ObDereferenceObject(deviceObject);
    }

    return status;
}

/**
 * \brief Opens the base device object associated with a device object.
 *
 * \details The base device object is the lowest-level device object in the file
 * system or device driver stack.
 *
 * \param[in] DeviceHandle Handle to a device object.
 * \param[in] DesiredAccess Desired access to the base device object.
 * \param[out] BaseDeviceHandle Set to the opened handle to the device object.
 * \param[in] AccessMode The mode in which to perform access checks.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS KphOpenDeviceBaseDevice(
    _In_ HANDLE DeviceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE BaseDeviceHandle,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_OBJECT baseDeviceObject;
    HANDLE baseDeviceHandle;

    KPH_PAGED_CODE_PASSIVE();

    deviceObject = NULL;
    baseDeviceObject = NULL;

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeOutputType(BaseDeviceHandle, HANDLE);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }

    status = ObReferenceObjectByHandle(DeviceHandle,
                                       DesiredAccess,
                                       *IoDeviceObjectType,
                                       AccessMode,
                                       &deviceObject,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        deviceObject = NULL;
        goto Exit;
    }

    baseDeviceObject = IoGetDeviceAttachmentBaseRef(deviceObject);

    status = ObOpenObjectByPointer(baseDeviceObject,
                                   (AccessMode ? 0 : OBJ_KERNEL_HANDLE),
                                   NULL,
                                   DesiredAccess,
                                   *IoDeviceObjectType,
                                   AccessMode,
                                   &baseDeviceHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        baseDeviceHandle = NULL;
        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        __try
        {
            *BaseDeviceHandle = baseDeviceHandle;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            ObCloseHandle(baseDeviceHandle, UserMode);
        }
    }
    else
    {
        *BaseDeviceHandle = baseDeviceHandle;
    }

Exit:

    if (baseDeviceObject)
    {
        ObDereferenceObject(baseDeviceObject);
    }

    if (deviceObject)
    {
        ObDereferenceObject(deviceObject);
    }

    return status;
}
