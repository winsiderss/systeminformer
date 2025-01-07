/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2025
 *
 */

#include <ph.h>

#include <phfirmware.h>
#include <ntintsafe.h>

typedef struct _PH_SMBIOS_STRINGREF
{
    UCHAR Length;
    PSTR Buffer;
} PH_SMBIOS_STRINGREF, *PPH_SMBIOS_STRINGREF;

typedef struct _PH_SMBIOS_PARSE_CONTEXT
{
    PRAW_SMBIOS_DATA Data;
    PVOID End;
    PH_SMBIOS_STRINGREF Strings[256];
} PH_SMBIOS_PARSE_CONTEXT, *PPH_SMBIOS_PARSE_CONTEXT;

NTSTATUS PhpParseSMBIOS(
    _In_ PPH_SMBIOS_PARSE_CONTEXT Context,
    _In_ PSMBIOS_HEADER Current,
    _Out_ PSMBIOS_HEADER* Next
    )
{
    NTSTATUS status;
    PSTR pointer;
    PSTR string;
    UCHAR nulls;
    UCHAR length;
    UCHAR count;

    *Next = NULL;

    memset(Context->Strings, 0, sizeof(Context->Strings));

    pointer = SMBIOS_STRING_TABLE(Current);
    string = pointer;

    if ((ULONG_PTR)pointer >= (ULONG_PTR)Context->End)
        return STATUS_BUFFER_OVERFLOW;

    nulls = 0;
    length = 0;
    count = 0;

    while ((ULONG_PTR)pointer < (ULONG_PTR)Context->End)
    {
        if (*pointer++ != ANSI_NULL)
        {
            if (!NT_SUCCESS(status = RtlUInt8Add(length, 1, &length)))
                return status;

            nulls = 0;
            continue;
        }

        if (nulls++)
            break;

        Context->Strings[count].Length = length;
        Context->Strings[count].Buffer = string;

        count++;
        string = pointer;
        length = 0;
    }

    if (nulls != 2)
        return STATUS_INVALID_ADDRESS;

    if (Current->Type != SMBIOS_END_OF_TABLE_TYPE)
        *Next = (PSMBIOS_HEADER)pointer;

    return STATUS_SUCCESS;
}

NTSTATUS PhpEnumSMBIOS(
    _In_ PRAW_SMBIOS_DATA Data,
    _In_ PPH_ENUM_SMBIOS_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PH_SMBIOS_PARSE_CONTEXT context;
    PSMBIOS_HEADER current;
    PSMBIOS_HEADER next;

    memset(&context, 0, sizeof(context));

    context.Data = Data;
    context.End = PTR_ADD_OFFSET(Data->SMBIOSTableData, Data->Length);

    current = (PSMBIOS_HEADER)Data->SMBIOSTableData;
    next = NULL;

    while (NT_SUCCESS(status = PhpParseSMBIOS(&context, current, &next)))
    {
        if (Callback(
            (ULONG_PTR)&context,
            Data->SMBIOSMajorVersion,
            Data->SMBIOSMinorVersion,
            (PPH_SMBIOS_ENTRY)current,
            Context
            ))
            break;

        if (!next)
            break;

        current = next;
    }

    return status;
}

NTSTATUS PhEnumSMBIOS(
    _In_ PPH_ENUM_SMBIOS_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    ULONG length;
    PSYSTEM_FIRMWARE_TABLE_INFORMATION info;

    length = 1024;
    info = PhAllocateZero(length);

    info->ProviderSignature = 'RSMB';
    info->TableID = 0;
    info->Action = SystemFirmwareTableGet;
    info->TableBufferLength = length - sizeof(SYSTEM_FIRMWARE_TABLE_INFORMATION);

    status = NtQuerySystemInformation(
        SystemFirmwareTableInformation,
        info,
        length,
        &length
        );
    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        PhFree(info);
        info = PhAllocateZero(length);

        info->ProviderSignature = 'RSMB';
        info->TableID = 0;
        info->Action = SystemFirmwareTableGet;
        info->TableBufferLength = length - sizeof(SYSTEM_FIRMWARE_TABLE_INFORMATION);

        status = NtQuerySystemInformation(
            SystemFirmwareTableInformation,
            info,
            length,
            &length
            );
    }

    if (NT_SUCCESS(status))
    {
        status = PhpEnumSMBIOS(
            (PRAW_SMBIOS_DATA)info->TableBuffer,
            Callback,
            Context
            );
    }

    PhFree(info);

    return status;
}

NTSTATUS PhGetSMBIOSString(
    _In_ ULONG_PTR EnumHandle,
    _In_ UCHAR Index,
    _Out_ PPH_STRING* String
    )
{
    PPH_SMBIOS_PARSE_CONTEXT context;
    PPH_SMBIOS_STRINGREF string;

    context = (PPH_SMBIOS_PARSE_CONTEXT)EnumHandle;

    *String = NULL;

    if (Index == SMBIOS_INVALID_STRING)
        return STATUS_INVALID_PARAMETER;

    string = &context->Strings[Index - 1];
    if (!string->Buffer)
        return STATUS_INVALID_PARAMETER;

    *String = PhConvertUtf8ToUtf16Ex(string->Buffer, string->Length);

    return STATUS_SUCCESS;
}
