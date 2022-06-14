/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *
 */

#include <kph.h>

VOID KphpCopyInfoUnicodeString(
    _Out_ PVOID Information,
    _In_opt_ PUNICODE_STRING UnicodeString
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KpiOpenDriver)
#pragma alloc_text(PAGE, KpiQueryInformationDriver)
#pragma alloc_text(PAGE, KphpCopyInfoUnicodeString)
#endif

NTSTATUS KpiOpenDriver(
    _Out_ PHANDLE DriverHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    PAGED_CODE();

    return KphOpenNamedObject(
        DriverHandle,
        DesiredAccess,
        ObjectAttributes,
        *IoDriverObjectType,
        AccessMode
        );
}

NTSTATUS KpiQueryInformationDriver(
    _In_ HANDLE DriverHandle,
    _In_ DRIVER_INFORMATION_CLASS DriverInformationClass,
    _Out_writes_bytes_(DriverInformationLength) PVOID DriverInformation,
    _In_ ULONG DriverInformationLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PDRIVER_OBJECT driverObject;

    PAGED_CODE();

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(DriverInformation, DriverInformationLength, 1);

            if (ReturnLength)
                ProbeForWrite(ReturnLength, sizeof(ULONG), 1);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }

    status = ObReferenceObjectByHandle(
        DriverHandle,
        0,
        *IoDriverObjectType,
        AccessMode,
        &driverObject,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    __try
    {
        switch (DriverInformationClass)
        {
        // Basic information such as flags, driver base and driver size.
        case DriverBasicInformation:
            {
                if (DriverInformationLength == sizeof(DRIVER_BASIC_INFORMATION))
                {
                    PDRIVER_BASIC_INFORMATION basicInfo;

                    basicInfo = (PDRIVER_BASIC_INFORMATION)DriverInformation;
                    basicInfo->Flags = driverObject->Flags;
                    basicInfo->DriverStart = driverObject->DriverStart;
                    basicInfo->DriverSize = driverObject->DriverSize;
                }
                else
                {
                    status = STATUS_INFO_LENGTH_MISMATCH;
                }

                if (ReturnLength)
                    *ReturnLength = sizeof(DRIVER_BASIC_INFORMATION);
            }
            break;

        // The name of the driver - e.g. \Driver\Null.
        case DriverNameInformation:
            {
                if (DriverInformation)
                {
                    /* Check buffer length. */
                    if (sizeof(UNICODE_STRING) + driverObject->DriverName.Length <=
                        DriverInformationLength)
                    {
                        KphpCopyInfoUnicodeString(
                            DriverInformation,
                            &driverObject->DriverName
                            );
                    }
                    else
                    {
                        status = STATUS_BUFFER_TOO_SMALL;
                    }
                }

                if (ReturnLength)
                    *ReturnLength = sizeof(UNICODE_STRING) + driverObject->DriverName.Length;
            }
            break;

        // The name of the driver's service key - e.g. \REGISTRY\...
        case DriverServiceKeyNameInformation:
            {
                if (driverObject->DriverExtension)
                {
                    if (DriverInformation)
                    {
                        if (sizeof(UNICODE_STRING) +
                            driverObject->DriverExtension->ServiceKeyName.Length <=
                            DriverInformationLength)
                        {
                            KphpCopyInfoUnicodeString(
                                DriverInformation,
                                &driverObject->DriverExtension->ServiceKeyName
                                );
                        }
                        else
                        {
                            status = STATUS_BUFFER_TOO_SMALL;
                        }
                    }

                    if (ReturnLength)
                    {
                        *ReturnLength = sizeof(UNICODE_STRING) +
                            driverObject->DriverExtension->ServiceKeyName.Length;
                    }
                }
                else
                {
                    if (DriverInformation)
                    {
                        if (sizeof(UNICODE_STRING) <= DriverInformationLength)
                        {
                            // Zero the information buffer.
                            KphpCopyInfoUnicodeString(
                                DriverInformation,
                                NULL
                                );
                        }
                        else
                        {
                            status = STATUS_BUFFER_TOO_SMALL;
                        }
                    }

                    if (ReturnLength)
                        *ReturnLength = sizeof(UNICODE_STRING);
                }
            }
            break;

        default:
            {
                status = STATUS_INVALID_INFO_CLASS;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
    }

    ObDereferenceObject(driverObject);

    return status;
}

VOID KphpCopyInfoUnicodeString(
    _Out_ PVOID Information,
    _In_opt_ PUNICODE_STRING UnicodeString
    )
{
    PUNICODE_STRING targetUnicodeString = Information;
    PWCHAR targetBuffer;

    PAGED_CODE();

    if (UnicodeString)
    {
        targetBuffer = PTR_ADD_OFFSET(Information, sizeof(UNICODE_STRING));

        targetUnicodeString->Length = UnicodeString->Length;
        targetUnicodeString->MaximumLength = UnicodeString->Length;
        targetUnicodeString->Buffer = targetBuffer;
        memcpy(targetBuffer, UnicodeString->Buffer, UnicodeString->Length);
    }
    else
    {
        targetUnicodeString->Length = 0;
        targetUnicodeString->MaximumLength = 0;
        targetUnicodeString->Buffer = NULL;
    }
}
