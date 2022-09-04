/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2017
 *
 */

#include "exttools.h"
#include "firmware.h"
#include "Efi\EfiTypes.h"
#include "Efi\EfiDevicePath.h"

NTSTATUS EnumerateFirmwareValues(
    _Out_ PVOID *Values
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferLength;

    bufferLength = PAGE_SIZE;
    buffer = PhAllocate(bufferLength);

    while (TRUE)
    {
        status = NtEnumerateSystemEnvironmentValuesEx(
            SystemEnvironmentValueInformation,
            buffer,
            &bufferLength
            );

        if (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INFO_LENGTH_MISMATCH)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferLength);
        }
        else
        {
            break;
        }
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *Values = buffer;

    return status;
}

NTSTATUS EfiEnumerateBootEntries(
    _Out_ PVOID *Entries
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferLength;

    bufferLength = PAGE_SIZE;
    buffer = PhAllocate(bufferLength);

    while (TRUE)
    {
        status = NtEnumerateBootEntries(
            buffer,
            &bufferLength
            );

        if (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INFO_LENGTH_MISMATCH)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferLength);
        }
        else
        {
            break;
        }
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *Entries = buffer;

    return status;
}

BOOLEAN EfiSupported(
    VOID
    )
{
    // The GetFirmwareEnvironmentVariable function will always fail on a legacy BIOS-based system, 
    //  or if Windows was installed using legacy BIOS on a system that supports both legacy BIOS and UEFI. 
    // To identify these conditions, call the function with a dummy firmware environment name such as an empty string ("") 
    //  for the lpName parameter and a dummy GUID such as "{00000000-0000-0000-0000-000000000000}" for the lpGuid parameter. 
    // On a legacy BIOS-based system, or on a system that supports both legacy BIOS and UEFI where Windows was installed using legacy BIOS, 
    //  the function will fail with ERROR_INVALID_FUNCTION. 
    // On a UEFI-based system, the function will fail with an error specific to the firmware, such as ERROR_NOACCESS, 
    //  to indicate that the dummy GUID namespace does not exist.

    UNICODE_STRING variableName = RTL_CONSTANT_STRING(L" ");
    PVOID variableValue = NULL; 
    ULONG variableValueLength = 0;

    //GetFirmwareEnvironmentVariable(
    //    L"", 
    //    L"{00000000-0000-0000-0000-000000000000}", 
    //    NULL, 
    //    0
    //    );
    //if (GetLastError() == ERROR_INVALID_FUNCTION)

    if (NtQuerySystemEnvironmentValueEx(
        &variableName, 
        (PGUID)&GUID_NULL, 
        variableValue,
        &variableValueLength,
        NULL
        ) == STATUS_VARIABLE_NOT_FOUND)
    {
        return TRUE;
    }

    return FALSE;
}
