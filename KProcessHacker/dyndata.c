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
#define _DYNDATA_PRIVATE
#include <dyndata.h>

NTSTATUS KphpLoadDynamicConfiguration(
    _In_ PVOID Buffer,
    _In_ ULONG Length
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
    NTSTATUS status;

    PAGED_CODE();

    // Get Windows version information.

    memset(&KphDynOsVersionInfo, 0, sizeof(RTL_OSVERSIONINFOEXW));
    KphDynOsVersionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
    status = RtlGetVersion((PRTL_OSVERSIONINFOW)&KphDynOsVersionInfo);

    return status;
}

NTSTATUS KphReadDynamicDataParameters(
    _In_opt_ HANDLE KeyHandle
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

    info = ExAllocatePoolZero(PagedPool, resultLength, 'ThpK');

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
    _In_ PVOID Buffer,
    _In_ ULONG Length
    )
{
    PKPH_DYN_CONFIGURATION config;
    ULONG i;
    PKPH_DYN_PACKAGE package;

    PAGED_CODE();

    config = Buffer;

    if (Length < UFIELD_OFFSET(KPH_DYN_CONFIGURATION, Packages))
        return STATUS_INVALID_PARAMETER;
    if (config->Version != KPH_DYN_CONFIGURATION_VERSION)
        return STATUS_INVALID_PARAMETER;
    if (config->NumberOfPackages > KPH_DYN_MAXIMUM_PACKAGES)
        return STATUS_INVALID_PARAMETER;
    if (Length < UFIELD_OFFSET(KPH_DYN_CONFIGURATION, Packages) + config->NumberOfPackages * sizeof(KPH_DYN_PACKAGE))
        return STATUS_INVALID_PARAMETER;

    dprintf("Loading dynamic configuration with %lu package(s)\n", config->NumberOfPackages);

    for (i = 0; i < config->NumberOfPackages; i++)
    {
        package = &config->Packages[i];

        if (package->MajorVersion == KphDynOsVersionInfo.dwMajorVersion &&
            package->MinorVersion == KphDynOsVersionInfo.dwMinorVersion &&
            (package->ServicePackMajor == USHORT_MAX || package->ServicePackMajor == KphDynOsVersionInfo.wServicePackMajor) &&
            (package->BuildNumber == USHORT_MAX || package->BuildNumber == KphDynOsVersionInfo.dwBuildNumber))
        {
            dprintf("Found matching package at index %lu for Windows %u.%u\n", i, package->MajorVersion, package->MinorVersion);

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
