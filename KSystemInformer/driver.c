/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     jxy-s   2022-2025
 *
 */

#include <kph.h>

#include <trace.h>

KPH_PAGED_FILE();

/**
 * \brief Opens a driver object.
 *
 * \param[out] DriverHandle Set to the opened handle to the driver.
 * \param[in] DesiredAccess Desired access to the driver object.
 * \param[in] ObjectAttributes Object attributes for opening the driver object.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphOpenDriver(
    _Out_ PHANDLE DriverHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return KphOpenNamedObject(DriverHandle,
                              DesiredAccess,
                              ObjectAttributes,
                              *IoDriverObjectType,
                              AccessMode);
}

/**
 * \brief Queries information about a driver.
 *
 * \param[in] DriverHandle Handle to driver to query.
 * \param[in] DriverInformationClass Information class to query.
 * \param[out] DriverInformation Populated with driver information by class.
 * \param[in] DriverInformationLength Length of the driver information buffer.
 * \param[out] ReturnLength Receives the number of bytes written or required.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryInformationDriver(
    _In_ HANDLE DriverHandle,
    _In_ KPH_DRIVER_INFORMATION_CLASS DriverInformationClass,
    _Out_writes_bytes_opt_(DriverInformationLength) PVOID DriverInformation,
    _In_ ULONG DriverInformationLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PDRIVER_OBJECT driverObject;
    ULONG returnLength;

    KPH_PAGED_CODE_PASSIVE();

    driverObject = NULL;
    returnLength = 0;

    if (AccessMode != KernelMode)
    {
        __try
        {
            if (DriverInformation)
            {
                ProbeOutputBytes(DriverInformation, DriverInformationLength);
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

    status = ObReferenceObjectByHandle(DriverHandle,
                                       0,
                                       *IoDriverObjectType,
                                       KernelMode,
                                       &driverObject,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        driverObject = NULL;
        goto Exit;
    }

    //
    // We reach into the driver object on purpose
    //
#pragma prefast(push)
#pragma prefast(disable : 28175)
    switch (DriverInformationClass)
    {
        case KphDriverBasicInformation:
        {
            //
            // Basic information such as flags, driver base and driver size.
            //

            KPH_DRIVER_BASIC_INFORMATION basicInfo;

            if (!DriverInformation ||
                (DriverInformationLength < sizeof(KPH_DRIVER_BASIC_INFORMATION)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(KPH_DRIVER_BASIC_INFORMATION);
                goto Exit;
            }

            RtlZeroMemory(&basicInfo, sizeof(KPH_DRIVER_BASIC_INFORMATION));

            basicInfo.Flags = driverObject->Flags;
            basicInfo.DriverStart = driverObject->DriverStart;
            basicInfo.DriverSize = driverObject->DriverSize;

            status = KphCopyToMode(DriverInformation,
                                   &basicInfo,
                                   sizeof(KPH_DRIVER_BASIC_INFORMATION),
                                   AccessMode);
            if (NT_SUCCESS(status))
            {
                returnLength = sizeof(KPH_DRIVER_BASIC_INFORMATION);
            }

            break;
        }
        case KphDriverNameInformation:
        {
            //
            // The name of the driver - e.g. \Driver\Null.
            //

            status = KphCopyUnicodeStringToMode(DriverInformation,
                                                DriverInformationLength,
                                                &driverObject->DriverName,
                                                &returnLength,
                                                AccessMode);

            break;
        }
        case KphDriverServiceKeyNameInformation:
        {
            PCUNICODE_STRING serviceKeyName;

            //
            // The name of the driver's service key - e.g. \REGISTRY\...
            //

            if (driverObject->DriverExtension)
            {
                serviceKeyName = &driverObject->DriverExtension->ServiceKeyName;
            }
            else
            {
                serviceKeyName = NULL;
            }

            status = KphCopyUnicodeStringToMode(DriverInformation,
                                                DriverInformationLength,
                                                serviceKeyName,
                                                &returnLength,
                                                AccessMode);

            break;
        }
        case KphDriverImageFileNameInformation:
        {
            UNICODE_STRING fullDriverPath;

            if ((KphOsVersionInfo.dwMajorVersion < 10) ||
                (KphOsVersionInfo.dwMajorVersion == 10) &&
                (KphOsVersionInfo.dwBuildNumber < 16299))
            {
                //
                // Per documentation it is not safe to call IoQueryFullDriverPath
                // on anything but our own driver object.
                //
                status = STATUS_NOINTERFACE;
                goto Exit;
            }

            RtlZeroMemory(&fullDriverPath, sizeof(UNICODE_STRING));

            status = IoQueryFullDriverPath(driverObject, &fullDriverPath);
            if (!NT_SUCCESS(status))
            {
                goto Exit;
            }

            status = KphCopyUnicodeStringToMode(DriverInformation,
                                                DriverInformationLength,
                                                &fullDriverPath,
                                                &returnLength,
                                                AccessMode);

            KphFreePool(fullDriverPath.Buffer);
            break;
        }
        default:
        {
            status = STATUS_INVALID_INFO_CLASS;
            break;
        }
    }
#pragma prefast(pop)

Exit:

    if (driverObject)
    {
        ObDereferenceObject(driverObject);
    }

    if (ReturnLength)
    {
        KphWriteULongToMode(ReturnLength, returnLength, AccessMode);
    }

    return status;
}
