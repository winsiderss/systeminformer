/*
 * Process Hacker Driver - 
 *   I/O manager
 * 
 * Copyright (C) 2009 wj32
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

#include "include/io.h"

VOID KphpCopyInfoUnicodeString(
    __out PVOID Information,
    __in PUNICODE_STRING UnicodeString
    );

/* KphOpenDriver
 * 
 * Opens a driver object.
 */
NTSTATUS KphOpenDriver(
    __out PHANDLE DriverHandle,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in KPROCESSOR_MODE AccessMode
    )
{
    return KphOpenNamedObject(
        DriverHandle,
        0,
        ObjectAttributes,
        *IoDriverObjectType,
        AccessMode
        );
}

/* KphQueryInformationDriver
 * 
 * Queries information about a driver object.
 */
NTSTATUS KphQueryInformationDriver(
    __in HANDLE DriverHandle,
    __in DRIVER_INFORMATION_CLASS DriverInformationClass,
    __out_bcount_opt(DriverInformationLength) PVOID DriverInformation,
    __in ULONG DriverInformationLength,
    __out_opt PULONG ReturnLength,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PDRIVER_OBJECT driverObject;
    
    if (
        DriverInformationClass < DriverBasicInformation || 
        DriverInformationClass >= MaxDriverInfoClass
        )
        return STATUS_INVALID_INFO_CLASS;
    
    /* Probe user input. */
    if (AccessMode != KernelMode)
    {
        __try
        {
            if (DriverInformation)
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
        KernelMode,
        &driverObject,
        NULL
        );
    
    if (!NT_SUCCESS(status))
        return status;
    
    __try
    {
        switch (DriverInformationClass)
        {
            /* DriverBasicInformation
             * 
             * Basic information such as flags, driver base and driver size.
             */
            case DriverBasicInformation:
            {
                if (DriverInformation)
                {
                    /* Check buffer length. */
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
                }
                
                if (ReturnLength)
                    *ReturnLength = sizeof(DRIVER_BASIC_INFORMATION);
            }
            break;
            
            /* DriverNameInformation
             * 
             * The name of the driver - e.g. \Driver\KProcessHacker.
             */
            case DriverNameInformation:
            {
                if (DriverInformation)
                {
                    /* Check buffer length. */
                    if (
                        sizeof(UNICODE_STRING) + 
                        driverObject->DriverName.Length <= 
                        DriverInformationLength
                        )
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
                
                /* Pass the ReturnLength. */
                if (ReturnLength)
                    *ReturnLength = sizeof(UNICODE_STRING) + driverObject->DriverName.Length;
            }
            break;
            
            /* DriverServiceKeyNameInformation
             * 
             * The name of the driver's service key - e.g. \REGISTRY\...
             */
            case DriverServiceKeyNameInformation:
            {
                if (driverObject->DriverExtension)
                {
                    if (DriverInformation)
                    {
                        if (
                            sizeof(UNICODE_STRING) + 
                            driverObject->DriverExtension->ServiceKeyName.Length <= 
                            DriverInformationLength
                            )
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
                        *ReturnLength = sizeof(UNICODE_STRING) + 
                            driverObject->DriverExtension->ServiceKeyName.Length;
                }
                else
                {
                    if (DriverInformation)
                    {
                        if (sizeof(UNICODE_STRING) <= DriverInformationLength)
                        {
                            /* Zero the information buffer. */
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

/* KphpCopyInfoUnicodeString
 * 
 * Copies a UNICODE_STRING to an information buffer. If 
 * the given string is NULL, the function zeros the 
 * destination UNICODE_STRING.
 */
VOID KphpCopyInfoUnicodeString(
    __out PVOID Information,
    __in PUNICODE_STRING UnicodeString
    )
{
    PUNICODE_STRING targetUnicodeString = (PUNICODE_STRING)Information;
    
    if (UnicodeString)
    {
        targetUnicodeString->Length = UnicodeString->Length;
        targetUnicodeString->MaximumLength = targetUnicodeString->Length;
        targetUnicodeString->Buffer = (PWSTR)((PCHAR)Information + sizeof(UNICODE_STRING));
        memcpy(
            targetUnicodeString->Buffer,
            UnicodeString->Buffer,
            targetUnicodeString->Length
            );
    }
    else
    {
        targetUnicodeString->Length = 0;
        targetUnicodeString->MaximumLength = 0;
        targetUnicodeString->Buffer = NULL;
    }
}
