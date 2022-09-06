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
