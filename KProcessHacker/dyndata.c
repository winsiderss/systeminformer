/*
 * KProcessHacker
 *
 * Copyright (C) 2010-2016 wj32
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
#define _DYNDATA_PRIVATE
#include <dyndata.h>

#define C_2sTo4(x) ((unsigned int)(signed short)(x))

NTSTATUS KphpLoadDynamicConfiguration(
    __in PVOID Buffer,
    __in ULONG Length
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KphDynamicDataInitialization)
#pragma alloc_text(PAGE, KphReadDynamicDataParameters)
#pragma alloc_text(PAGE, KphpLoadDynamicConfiguration)
#endif

NTSTATUS KphDynamicDataInitialization(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    // Get Windows version information.

    KphDynOsVersionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
    status = RtlGetVersion((PRTL_OSVERSIONINFOW)&KphDynOsVersionInfo);

    return status;
}

NTSTATUS KphReadDynamicDataParameters(
    __in_opt HANDLE KeyHandle
    )
{
    NTSTATUS status;
    UNICODE_STRING valueName;
    PKEY_VALUE_PARTIAL_INFORMATION info;
    ULONG resultLength;

    PAGED_CODE();

    if (!KeyHandle)
        return STATUS_UNSUCCESSFUL;

    RtlInitUnicodeString(&valueName, L"DynamicConfiguration");

    status = ZwQueryValueKey(
        KeyHandle,
        &valueName,
        KeyValuePartialInformation,
        NULL,
        0,
        &resultLength
        );

    if (status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL)
    {
        // Unexpected status; fail now.
        return STATUS_UNSUCCESSFUL;
    }

    info = ExAllocatePoolWithTag(PagedPool, resultLength, 'ThpK');

    if (!info)
        return STATUS_INSUFFICIENT_RESOURCES;

    status = ZwQueryValueKey(
        KeyHandle,
        &valueName,
        KeyValuePartialInformation,
        info,
        resultLength,
        &resultLength
        );

    if (NT_SUCCESS(status))
    {
        if (info->Type == REG_BINARY)
            status = KphpLoadDynamicConfiguration(info->Data, info->DataLength);
        else
            status = STATUS_OBJECT_TYPE_MISMATCH;

        if (!NT_SUCCESS(status))
            dprintf("Unable to load dynamic configuration: 0x%x\n", status);
    }

    ExFreePoolWithTag(info, 'ThpK');

    return status;
}

NTSTATUS KphpLoadDynamicConfiguration(
    __in PVOID Buffer,
    __in ULONG Length
    )
{
    PKPH_DYN_CONFIGURATION config;
    ULONG i;
    PKPH_DYN_PACKAGE package;

    PAGED_CODE();

    config = Buffer;

    if (Length < FIELD_OFFSET(KPH_DYN_CONFIGURATION, Packages))
        return STATUS_INVALID_PARAMETER;
    if (config->Version != KPH_DYN_CONFIGURATION_VERSION)
        return STATUS_INVALID_PARAMETER;
    if (config->NumberOfPackages > KPH_DYN_MAXIMUM_PACKAGES)
        return STATUS_INVALID_PARAMETER;
    if (Length < FIELD_OFFSET(KPH_DYN_CONFIGURATION, Packages) + config->NumberOfPackages * sizeof(KPH_DYN_PACKAGE))
        return STATUS_INVALID_PARAMETER;

    dprintf("Loading dynamic configuration with %u package(s)\n", config->NumberOfPackages);

    for (i = 0; i < config->NumberOfPackages; i++)
    {
        package = &config->Packages[i];

        if (package->MajorVersion == KphDynOsVersionInfo.dwMajorVersion &&
            package->MinorVersion == KphDynOsVersionInfo.dwMinorVersion &&
            (package->ServicePackMajor == (USHORT)-1 || package->ServicePackMajor == KphDynOsVersionInfo.wServicePackMajor) &&
            (package->BuildNumber == (USHORT)-1 || package->BuildNumber == KphDynOsVersionInfo.dwBuildNumber))
        {
            dprintf("Found matching package at index %u for Windows %u.%u\n", i, package->MajorVersion, package->MinorVersion);

            KphDynNtVersion = package->ResultingNtVersion;

            KphDynEgeGuid = C_2sTo4(package->StructData.EgeGuid);
            KphDynEpObjectTable = C_2sTo4(package->StructData.EpObjectTable);
            KphDynEreGuidEntry = C_2sTo4(package->StructData.EreGuidEntry);
            KphDynHtHandleContentionEvent = C_2sTo4(package->StructData.HtHandleContentionEvent);
            KphDynOtName = C_2sTo4(package->StructData.OtName);
            KphDynOtIndex = C_2sTo4(package->StructData.OtIndex);
            KphDynObDecodeShift = C_2sTo4(package->StructData.ObDecodeShift);
            KphDynObAttributesShift = C_2sTo4(package->StructData.ObAttributesShift);

            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}
