/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include "agenttools.h"
#include <phfirmware.h>
#include <mapldr.h>

// Provider CPU usage, exported as data by SystemInformer.exe.
__declspec(dllimport) FLOAT PhCpuKernelUsage;
__declspec(dllimport) FLOAT PhCpuUserUsage;

PCWSTR AtpKphLevelString(
    _In_ KPH_LEVEL Level
    )
{
    switch (Level)
    {
    case KphLevelNone:
        return L"none";
    case KphLevelMin:
        return L"min";
    case KphLevelLow:
        return L"low";
    case KphLevelMed:
        return L"med";
    case KphLevelHigh:
        return L"high";
    case KphLevelMax:
        return L"max";
    }

    return NULL;
}

FIRMWARE_TYPE AtpGetFirmwareType(
    VOID
    )
{
    SYSTEM_BOOT_ENVIRONMENT_INFORMATION bootInfo;

    memset(&bootInfo, 0, sizeof(bootInfo));

    if (NT_SUCCESS(NtQuerySystemInformation(SystemBootEnvironmentInformation, &bootInfo, sizeof(bootInfo), NULL)))
        return bootInfo.FirmwareType;

    return FirmwareTypeUnknown;
}

PCWSTR AtpFirmwareTypeString(
    _In_ FIRMWARE_TYPE FirmwareType
    )
{
    switch (FirmwareType)
    {
    case FirmwareTypeBios:
        return L"bios";
    case FirmwareTypeUefi:
        return L"uefi";
    }

    return L"unknown";
}

VOID AtpGetSystemInfo(
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PVOID structured;
    SYSTEM_BASIC_INFORMATION basicInfo;
    SYSTEM_PERFORMANCE_INFORMATION perfInfo;
    SYSTEM_TIMEOFDAY_INFORMATION timeOfDay;
    PPH_STRING version;
    PPH_PROCESS_ITEM* processItems;
    ULONG numberOfProcessItems;
    ULONG64 threadCount = 0;
    ULONG64 handleCount = 0;
    KPH_LEVEL kphLevel;
    ULONG i;
    WCHAR computerName[256];
    ULONG computerNameLength = RTL_NUMBER_OF(computerName);

    structured = PhCreateJsonObject();

    if (GetComputerNameW(computerName, &computerNameLength))
        AtJsonAddStringZ(structured, "computer_name", computerName);
    else
        AtJsonAddNull(structured, "computer_name");

    // OS version from the PEB, which mirrors the real build.
    {
        PPEB peb = NtCurrentPeb();
        PPH_STRING osVersion = PhFormatString(L"%lu.%lu.%lu", peb->OSMajorVersion, peb->OSMinorVersion, peb->OSBuildNumber);

        AtJsonAddString(structured, "os_version", osVersion);
        PhDereferenceObject(osVersion);
        PhAddJsonObjectUInt64(structured, "os_build", peb->OSBuildNumber);
    }

#if defined(_M_ARM64)
    PhAddJsonObject(structured, "os_architecture", "ARM64");
#elif defined(_M_X64)
    PhAddJsonObject(structured, "os_architecture", "x64");
#else
    PhAddJsonObject(structured, "os_architecture", "x86");
#endif

    if (NT_SUCCESS(NtQuerySystemInformation(SystemTimeOfDayInformation, &timeOfDay, sizeof(timeOfDay), NULL)))
    {
        AtJsonAddTime(structured, "boot_time", &timeOfDay.BootTime);
        AtJsonAddDuration(structured, "uptime_seconds", timeOfDay.CurrentTime.QuadPart - timeOfDay.BootTime.QuadPart);
    }
    else
    {
        AtJsonAddNull(structured, "boot_time");
        PhAddJsonObjectDouble(structured, "uptime_seconds", 0);
    }

    memset(&basicInfo, 0, sizeof(basicInfo));
    NtQuerySystemInformation(SystemBasicInformation, &basicInfo, sizeof(basicInfo), NULL);
    PhAddJsonObjectUInt64(structured, "processor_count", basicInfo.NumberOfProcessors);
    AtJsonAddStringZ(structured, "firmware_type", AtpFirmwareTypeString(AtpGetFirmwareType()));
    PhAddJsonObjectDouble(structured, "cpu_usage", (DOUBLE)PhCpuKernelUsage + (DOUBLE)PhCpuUserUsage);
    PhAddJsonObjectDouble(structured, "cpu_kernel_usage", (DOUBLE)PhCpuKernelUsage);
    PhAddJsonObjectDouble(structured, "cpu_user_usage", (DOUBLE)PhCpuUserUsage);
    PhAddJsonObjectUInt64(structured, "physical_total_bytes", (ULONG64)basicInfo.NumberOfPhysicalPages * basicInfo.PageSize);

    memset(&perfInfo, 0, sizeof(perfInfo));

    if (NT_SUCCESS(NtQuerySystemInformation(SystemPerformanceInformation, &perfInfo, sizeof(perfInfo), NULL)))
    {
        PhAddJsonObjectUInt64(structured, "physical_available_bytes", (ULONG64)perfInfo.AvailablePages * basicInfo.PageSize);
        PhAddJsonObjectUInt64(structured, "commit_total_bytes", (ULONG64)perfInfo.CommittedPages * basicInfo.PageSize);
        PhAddJsonObjectUInt64(structured, "commit_limit_bytes", (ULONG64)perfInfo.CommitLimit * basicInfo.PageSize);
        PhAddJsonObjectUInt64(structured, "commit_peak_bytes", (ULONG64)perfInfo.PeakCommitment * basicInfo.PageSize);
    }
    else
    {
        PhAddJsonObjectUInt64(structured, "physical_available_bytes", 0);
        PhAddJsonObjectUInt64(structured, "commit_total_bytes", 0);
        PhAddJsonObjectUInt64(structured, "commit_limit_bytes", 0);
        PhAddJsonObjectUInt64(structured, "commit_peak_bytes", 0);
    }

    PhAddJsonObjectUInt64(structured, "page_size", basicInfo.PageSize);

    PhEnumProcessItems(&processItems, &numberOfProcessItems);

    for (i = 0; i < numberOfProcessItems; i++)
    {
        threadCount += processItems[i]->NumberOfThreads;
        handleCount += processItems[i]->NumberOfHandles;
        PhDereferenceObject(processItems[i]);
    }

    PhFree(processItems);

    PhAddJsonObjectUInt64(structured, "process_count", numberOfProcessItems);
    PhAddJsonObjectUInt64(structured, "thread_count", threadCount);
    PhAddJsonObjectUInt64(structured, "handle_count", handleCount);

    if (version = PhGetBuildVersion())
    {
        AtJsonAddString(structured, "system_informer_version", version);
        PhDereferenceObject(version);
    }
    else
    {
        AtJsonAddNull(structured, "system_informer_version");
    }

    PhAddJsonObjectBoolean(structured, "system_informer_elevated", !!PhGetOwnTokenAttributes().Elevated);
    PhAddJsonObjectUInt64(structured, "system_informer_pid", HandleToUlong(NtCurrentProcessId()));

    kphLevel = KsiLevel();
    PhAddJsonObjectBoolean(structured, "ksi_connected", kphLevel != KphLevelNone);
    AtJsonAddStringZ(structured, "ksi_level", AtpKphLevelString(kphLevel));
    PhAddJsonObjectUInt64(structured, "schema_version", AT_SCHEMA_VERSION);

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

VOID AtpListKernelDrivers(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    PRTL_PROCESS_MODULES modules;
    PPH_STRING nameContains;
    BOOLEAN verify;
    AT_ROWS rows;
    PVOID structured;
    ULONG i;

    status = PhEnumKernelModules(&modules);

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Enumerating kernel modules");
        return;
    }

    nameContains = AtGetArgumentString(Call->Arguments, "name_contains");
    verify = AtJsonGetObjectBoolean(Call->Arguments, "verify_signatures");

    structured = PhCreateJsonObject();
    AtInitializeRows(&rows, Call->Arguments);

    for (i = 0; i < modules->NumberOfModules; i++)
    {
        PRTL_PROCESS_MODULE_INFORMATION module = &modules->Modules[i];
        PPH_STRING fileName;
        PPH_STRING name;
        PVOID row;

        fileName = PhConvertUtf8ToUtf16((PCSTR)module->FullPathName);
        name = PhConvertUtf8ToUtf16((PCSTR)&module->FullPathName[module->OffsetToFileName]);

        if (nameContains && !AtContainsString(name, nameContains) && !AtContainsString(fileName, nameContains))
        {
            PhClearReference(&fileName);
            PhClearReference(&name);
            continue;
        }

        row = PhCreateJsonObject();
        AtJsonAddString(row, "name", name);
        AtJsonAddWin32FileName(row, "file_path", fileName);
        AtJsonAddPointer(row, "base_address", module->ImageBase);
        PhAddJsonObjectUInt64(row, "size", module->ImageSize);
        PhAddJsonObjectUInt64(row, "load_order_index", module->LoadOrderIndex);
        PhAddJsonObjectUInt64(row, "load_count", module->LoadCount);

        if (verify && fileName)
        {
            PPH_STRING signer = NULL;
            VERIFY_RESULT verifyResult;
            PPH_STRING win32FileName = PhGetFileName(fileName);

            verifyResult = PhVerifyFile(PhGetString(win32FileName), &signer);
            AtJsonAddStringZ(row, "verify_result", AtVerifyResultString(verifyResult));
            AtJsonAddString(row, "verify_signer", signer);
            PhClearReference(&signer);
            PhClearReference(&win32FileName);
        }
        else
        {
            AtJsonAddNull(row, "verify_result");
            AtJsonAddNull(row, "verify_signer");
        }

        AtAddRow(&rows, row);

        PhClearReference(&fileName);
        PhClearReference(&name);
    }

    PhFree(modules);
    PhClearReference(&nameContains);

    AtAddRows(structured, "drivers", &rows);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

VOID AtpGetKsiStatus(
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    KPH_LEVEL level;
    ULONG64 duration;
    ULONG64 durationDown;
    ULONG64 durationUp;
    LARGE_INTEGER counter;
    LARGE_INTEGER frequency;
    PVOID structured;

    level = KsiLevel();

    structured = PhCreateJsonObject();
    PhAddJsonObjectBoolean(structured, "connected", level != KphLevelNone);
    AtJsonAddStringZ(structured, "level", AtpKphLevelString(level));

    // One round trip into the driver, timed on the host clock: a health check for the connection.
    if (NT_SUCCESS(PhQueryKphCounters(&duration, &durationDown, &durationUp)) &&
        NT_SUCCESS(NtQueryPerformanceCounter(&counter, &frequency)) &&
        frequency.QuadPart)
    {
        PVOID roundTrip = PhCreateJsonObject();

        PhAddJsonObjectUInt64(roundTrip, "total_microseconds", duration * 1000000 / frequency.QuadPart);
        PhAddJsonObjectUInt64(roundTrip, "to_kernel_microseconds", durationDown * 1000000 / frequency.QuadPart);
        PhAddJsonObjectUInt64(roundTrip, "from_kernel_microseconds", durationUp * 1000000 / frequency.QuadPart);
        PhAddJsonObjectValue(structured, "round_trip", roundTrip);
    }
    else
    {
        AtJsonAddNull(structured, "round_trip");
    }

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

VOID AtpGetPagefileInfo(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 0x200;
    ULONG returnLength = 0;
    PSYSTEM_PAGEFILE_INFORMATION pagefile;
    ULONG pageSize;
    AT_ROWS rows;
    PVOID structured;

    {
        SYSTEM_BASIC_INFORMATION basicInfo;

        memset(&basicInfo, 0, sizeof(basicInfo));
        NtQuerySystemInformation(SystemBasicInformation, &basicInfo, sizeof(basicInfo), NULL);
        pageSize = basicInfo.PageSize ? basicInfo.PageSize : PAGE_SIZE;
    }

    buffer = PhAllocate(bufferSize);

    while ((status = NtQuerySystemInformation(SystemPageFileInformation, buffer, bufferSize, &returnLength)) == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(buffer);
        bufferSize *= 2;
        buffer = PhAllocate(bufferSize);
    }

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Querying pagefile information");
        PhFree(buffer);
        return;
    }

    structured = PhCreateJsonObject();
    AtInitializeRows(&rows, Call->Arguments);

    // An empty result (no configured pagefile) returns zero bytes.
    if (returnLength >= sizeof(SYSTEM_PAGEFILE_INFORMATION))
    {
        pagefile = buffer;

        for (;;)
        {
            PVOID row = PhCreateJsonObject();
            PH_STRINGREF name;

            PhUnicodeStringToStringRef(&pagefile->PageFileName, &name);
            AtJsonAddStringRef(row, "name", &name);
            PhAddJsonObjectUInt64(row, "total_bytes", (ULONG64)pagefile->TotalSize * pageSize);
            PhAddJsonObjectUInt64(row, "in_use_bytes", (ULONG64)pagefile->TotalInUse * pageSize);
            PhAddJsonObjectUInt64(row, "peak_bytes", (ULONG64)pagefile->PeakUsage * pageSize);
            AtAddRow(&rows, row);

            if (pagefile->NextEntryOffset == 0)
                break;

            pagefile = PTR_ADD_OFFSET(pagefile, pagefile->NextEntryOffset);
        }
    }

    PhFree(buffer);

    AtAddRows(structured, "pagefiles", &rows);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

typedef struct _AT_SMBIOS_CONTEXT
{
    PVOID Object;
    BOOLEAN VersionAdded;
} AT_SMBIOS_CONTEXT, *PAT_SMBIOS_CONTEXT;

VOID AtpAddSmbiosString(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ ULONG_PTR EnumHandle,
    _In_ UCHAR Index
    )
{
    PPH_STRING string;

    if (NT_SUCCESS(PhGetSMBIOSString(EnumHandle, Index, &string)))
    {
        AtJsonAddString(Object, Key, string);
        PhDereferenceObject(string);
    }
}

_Function_class_(PH_ENUM_SMBIOS_CALLBACK)
BOOLEAN NTAPI AtpSmbiosCallback(
    _In_ ULONG_PTR EnumHandle,
    _In_ UCHAR MajorVersion,
    _In_ UCHAR MinorVersion,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_opt_ PVOID Context
    )
{
    PAT_SMBIOS_CONTEXT context = Context;

    if (!context)
        return FALSE;

    if (!context->VersionAdded)
    {
        PPH_STRING version;

        version = PhFormatString(L"%hhu.%hhu", MajorVersion, MinorVersion);
        AtJsonAddString(context->Object, "smbios_version", version);
        PhDereferenceObject(version);
        context->VersionAdded = TRUE;
    }

    switch (Entry->Header.Type)
    {
    case SMBIOS_FIRMWARE_INFORMATION_TYPE:
        if (PH_SMBIOS_CONTAINS_STRING(Entry, Firmware, Vendor))
            AtpAddSmbiosString(context->Object, "bios_vendor", EnumHandle, Entry->Firmware.Vendor);
        if (PH_SMBIOS_CONTAINS_STRING(Entry, Firmware, Version))
            AtpAddSmbiosString(context->Object, "bios_version", EnumHandle, Entry->Firmware.Version);
        if (PH_SMBIOS_CONTAINS_STRING(Entry, Firmware, ReleaseDate))
            AtpAddSmbiosString(context->Object, "bios_release_date", EnumHandle, Entry->Firmware.ReleaseDate);
        if (PH_SMBIOS_CONTAINS_FIELD(Entry, Firmware, MinorRelease) &&
            !(Entry->Firmware.MajorRelease == 0xFF && Entry->Firmware.MinorRelease == 0xFF))
        {
            PPH_STRING revision;

            revision = PhFormatString(L"%hhu.%hhu", Entry->Firmware.MajorRelease, Entry->Firmware.MinorRelease);
            AtJsonAddString(context->Object, "bios_revision", revision);
            PhDereferenceObject(revision);
        }
        break;
    case SMBIOS_SYSTEM_INFORMATION_TYPE:
        if (PH_SMBIOS_CONTAINS_STRING(Entry, System, Manufacturer))
            AtpAddSmbiosString(context->Object, "system_manufacturer", EnumHandle, Entry->System.Manufacturer);
        if (PH_SMBIOS_CONTAINS_STRING(Entry, System, ProductName))
            AtpAddSmbiosString(context->Object, "system_product", EnumHandle, Entry->System.ProductName);
        if (PH_SMBIOS_CONTAINS_STRING(Entry, System, Version))
            AtpAddSmbiosString(context->Object, "system_version", EnumHandle, Entry->System.Version);
        if (PH_SMBIOS_CONTAINS_STRING(Entry, System, Family))
            AtpAddSmbiosString(context->Object, "system_family", EnumHandle, Entry->System.Family);
        // Serial number and system UUID are machine-unique identifiers, deliberately omitted.
        break;
    case SMBIOS_BASEBOARD_INFORMATION_TYPE:
        if (PH_SMBIOS_CONTAINS_STRING(Entry, Baseboard, Manufacturer))
            AtpAddSmbiosString(context->Object, "baseboard_manufacturer", EnumHandle, Entry->Baseboard.Manufacturer);
        if (PH_SMBIOS_CONTAINS_STRING(Entry, Baseboard, Product))
            AtpAddSmbiosString(context->Object, "baseboard_product", EnumHandle, Entry->Baseboard.Product);
        if (PH_SMBIOS_CONTAINS_STRING(Entry, Baseboard, Version))
            AtpAddSmbiosString(context->Object, "baseboard_version", EnumHandle, Entry->Baseboard.Version);
        break;
    }

    return FALSE;
}

VOID AtpGetSmbiosInfo(
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    AT_SMBIOS_CONTEXT context;
    PVOID structured;

    structured = PhCreateJsonObject();
    context.Object = structured;
    context.VersionAdded = FALSE;

    // A malformed table tail ends the walk but the entries already parsed are still valid, so the
    // status is ignored, as the SMBIOS viewer does.
    PhEnumSMBIOS(AtpSmbiosCallback, &context);

    AtAddSnapshot(structured);
    Result->StructuredContent = structured;
}

// Values are opaque binary; large blobs (e.g. db/dbx) are emitted as a bounded hex prefix.
#define AT_UEFI_VALUE_HEX_LIMIT 256

VOID AtpAddUefiAttributes(
    _In_ PVOID Object,
    _In_ ULONG Attributes
    )
{
    PVOID flags;

    flags = PhCreateJsonArray();

    if (Attributes & EFI_VARIABLE_NON_VOLATILE)
        PhAddJsonArrayObject(flags, PhCreateJsonStringObject("non_volatile"));
    if (Attributes & EFI_VARIABLE_BOOTSERVICE_ACCESS)
        PhAddJsonArrayObject(flags, PhCreateJsonStringObject("boot_service_access"));
    if (Attributes & EFI_VARIABLE_RUNTIME_ACCESS)
        PhAddJsonArrayObject(flags, PhCreateJsonStringObject("runtime_access"));
    if (Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD)
        PhAddJsonArrayObject(flags, PhCreateJsonStringObject("hardware_error_record"));
    if (Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS)
        PhAddJsonArrayObject(flags, PhCreateJsonStringObject("authenticated_write_access"));
    if (Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS)
        PhAddJsonArrayObject(flags, PhCreateJsonStringObject("time_based_authenticated_write_access"));
    if (Attributes & EFI_VARIABLE_APPEND_WRITE)
        PhAddJsonArrayObject(flags, PhCreateJsonStringObject("append_write"));
    if (Attributes & EFI_VARIABLE_ENHANCED_AUTHENTICATED_ACCESS)
        PhAddJsonArrayObject(flags, PhCreateJsonStringObject("enhanced_authenticated_access"));

    PhAddJsonObjectUInt64(Object, "attributes", Attributes);
    PhAddJsonObjectValue(Object, "attribute_flags", flags);
}

VOID AtpGetUefiVariables(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    PVOID variables;
    PVARIABLE_NAME_AND_VALUE variable;
    AT_ROWS rows;
    PVOID structured;
    PH_THREAD_PRIVILEGE_STATE privilegeState;

    if (AtpGetFirmwareType() != FirmwareTypeUefi)
    {
        AtSetToolError(Result, "failed", STATUS_NOT_SUPPORTED, L"This machine did not boot in UEFI mode; firmware variables are not available.");
        return;
    }

    // SeSystemEnvironmentPrivilege must be enabled, not merely held; an elevated token holds it
    // disabled. Enable it for this thread only, so the process token is untouched. Without it the
    // enumeration fails with STATUS_PRIVILEGE_NOT_HELD.
    status = PhAcquireCurrentThreadPrivilege(SE_SYSTEM_ENVIRONMENT_PRIVILEGE, &privilegeState);

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Enabling SeSystemEnvironmentPrivilege");
        return;
    }

    status = PhEnumFirmwareEnvironmentValues(SystemEnvironmentValueInformation, &variables);
    PhReleaseCurrentThreadPrivilege(&privilegeState);

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Enumerating firmware environment variables");
        return;
    }

    structured = PhCreateJsonObject();
    AtInitializeRows(&rows, Call->Arguments);

    for (variable = PH_FIRST_FIRMWARE_VALUE(variables); variable; variable = PH_NEXT_FIRMWARE_VALUE(variable))
    {
        PVOID row = PhCreateJsonObject();
        PPH_STRING string;

        AtJsonAddStringZ(row, "name", variable->Name);
        string = PhFormatGuid(&variable->VendorGuid);
        AtJsonAddString(row, "vendor_guid", string);
        PhDereferenceObject(string);
        AtpAddUefiAttributes(row, variable->Attributes);
        PhAddJsonObjectUInt64(row, "value_length", variable->ValueLength);

        if (variable->ValueLength)
        {
            ULONG length = min(variable->ValueLength, AT_UEFI_VALUE_HEX_LIMIT);

            string = PhBufferToHexString(PTR_ADD_OFFSET(variable, variable->ValueOffset), length);
            AtJsonAddString(row, "value_hex", string);
            PhDereferenceObject(string);
            PhAddJsonObjectBoolean(row, "value_truncated", variable->ValueLength > length);
        }

        AtAddRow(&rows, row);
    }

    PhFree(variables);

    AtAddRows(structured, "variables", &rows);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

// Mirrors TPM_DEVICE_INFO from tbs.h; tbs.dll is loaded at runtime so the plugin does not link tbs.lib.
typedef struct _AT_TPM_DEVICE_INFO
{
    ULONG StructVersion;
    ULONG TpmVersion;
    ULONG TpmInterfaceType;
    ULONG TpmImpRevision;
} AT_TPM_DEVICE_INFO, *PAT_TPM_DEVICE_INFO;

typedef ULONG (WINAPI* AT_TBSI_GET_DEVICE_INFO)(
    _In_ ULONG Size,
    _Out_writes_bytes_(Size) PVOID Info
    );

PCWSTR AtpTpmVersionString(
    _In_ ULONG Version
    )
{
    switch (Version)
    {
    case 1:
        return L"1.2";
    case 2:
        return L"2.0";
    }

    return L"unknown";
}

PCWSTR AtpTpmInterfaceTypeString(
    _In_ ULONG InterfaceType
    )
{
    switch (InterfaceType)
    {
    case 1:
        return L"port_or_mmio";
    case 2:
        return L"trustzone";
    case 3:
        return L"hardware";
    case 4:
        return L"emulator";
    case 5:
        return L"spb";
    }

    return L"unknown";
}

_Function_class_(PH_ENUM_SMBIOS_CALLBACK)
BOOLEAN NTAPI AtpTpmSmbiosCallback(
    _In_ ULONG_PTR EnumHandle,
    _In_ UCHAR MajorVersion,
    _In_ UCHAR MinorVersion,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_opt_ PVOID Context
    )
{
    PVOID object = Context;
    PVOID smbios;
    UCHAR major = 0;

    if (!object || Entry->Header.Type != SMBIOS_TPM_DEVICE_INFORMATION_TYPE)
        return FALSE;

    smbios = PhCreateJsonObject();

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, TPMDevice, VendorID))
    {
        WCHAR vendor[5];

        vendor[0] = (WCHAR)Entry->TPMDevice.VendorID[0];
        vendor[1] = (WCHAR)Entry->TPMDevice.VendorID[1];
        vendor[2] = (WCHAR)Entry->TPMDevice.VendorID[2];
        vendor[3] = (WCHAR)Entry->TPMDevice.VendorID[3];
        vendor[4] = UNICODE_NULL;
        AtJsonAddStringZ(smbios, "vendor_id", vendor);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, TPMDevice, MinorSpecVersion))
    {
        PPH_STRING version;

        major = Entry->TPMDevice.MajorSpecVersion;
        version = PhFormatString(L"%hhu.%hhu", Entry->TPMDevice.MajorSpecVersion, Entry->TPMDevice.MinorSpecVersion);
        AtJsonAddString(smbios, "spec_version", version);
        PhDereferenceObject(version);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, TPMDevice, FirmwareVersion1))
    {
        ULONG64 version = Entry->TPMDevice.FirmwareVersion1;

        // For TPM 2.0 the two fields form one 64-bit version; for 1.2 only the first is meaningful.
        if (major >= 2)
        {
            version <<= 32;

            if (PH_SMBIOS_CONTAINS_FIELD(Entry, TPMDevice, FirmwareVersion2))
                version |= Entry->TPMDevice.FirmwareVersion2;
        }

        PhAddJsonObjectUInt64(smbios, "firmware_version", version);
    }

    if (PH_SMBIOS_CONTAINS_STRING(Entry, TPMDevice, Description))
        AtpAddSmbiosString(smbios, "description", EnumHandle, Entry->TPMDevice.Description);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, TPMDevice, Characteristics))
    {
        ULONG64 characteristics = Entry->TPMDevice.Characteristics;

        PhAddJsonObjectBoolean(smbios, "configurable_via_firmware_update", !!(characteristics & SMBIOS_TPM_DEVICE_CONFIGURABLE_VIA_FIRMWARE_UPDATE));
        PhAddJsonObjectBoolean(smbios, "configurable_via_software_update", !!(characteristics & SMBIOS_TPM_DEVICE_CONFIGURABLE_VIA_SOFTWARE_UPDATE));
        PhAddJsonObjectBoolean(smbios, "configurable_via_proprietary_update", !!(characteristics & SMBIOS_TPM_DEVICE_CONFIGURABLE_VIA_PROPRIETARY_UPDATE));
    }

    PhAddJsonObjectValue(object, "smbios", smbios);

    return TRUE;
}

VOID AtpGetTpmInfo(
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static AT_TBSI_GET_DEVICE_INFO Tbsi_GetDeviceInfo_I = NULL;
    PVOID structured;
    AT_TPM_DEVICE_INFO deviceInfo;
    BOOLEAN present = FALSE;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"tbs.dll"))
            Tbsi_GetDeviceInfo_I = PhGetProcedureAddress(baseAddress, "Tbsi_GetDeviceInfo", 0);

        PhEndInitOnce(&initOnce);
    }

    structured = PhCreateJsonObject();

    // TBS reports the TPM the OS is using; SMBIOS describes what the firmware advertises.
    memset(&deviceInfo, 0, sizeof(deviceInfo));

    if (Tbsi_GetDeviceInfo_I && Tbsi_GetDeviceInfo_I(sizeof(deviceInfo), &deviceInfo) == 0)
    {
        present = TRUE;
        AtJsonAddStringZ(structured, "version", AtpTpmVersionString(deviceInfo.TpmVersion));
        AtJsonAddStringZ(structured, "interface_type", AtpTpmInterfaceTypeString(deviceInfo.TpmInterfaceType));
        PhAddJsonObjectUInt64(structured, "implementation_revision", deviceInfo.TpmImpRevision);
    }

    PhAddJsonObjectBoolean(structured, "present", present);

    PhEnumSMBIOS(AtpTpmSmbiosCallback, structured);

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

typedef struct _AT_ENVIRONMENT_CONTEXT
{
    PAT_ROWS Entries;
    PCWSTR Scope;
} AT_ENVIRONMENT_CONTEXT, *PAT_ENVIRONMENT_CONTEXT;

_Function_class_(PH_ENUM_KEY_CALLBACK)
BOOLEAN NTAPI AtpEnvironmentValueCallback(
    _In_ HANDLE RootDirectory,
    _In_ PKEY_VALUE_FULL_INFORMATION Information,
    _In_opt_ PVOID Context
    )
{
    PAT_ENVIRONMENT_CONTEXT context = Context;
    PVOID entry;
    PPH_STRING name;
    PPH_STRING value = NULL;

    if (!context)
        return TRUE;

    if (Information->Type == REG_SZ || Information->Type == REG_EXPAND_SZ)
    {
        // Registry string data is not guaranteed to be WCHAR-aligned; drop a dangling odd byte.
        SIZE_T dataLength = Information->DataLength & ~(sizeof(WCHAR) - 1);

        if (dataLength >= sizeof(WCHAR) &&
            *(PWCHAR)PTR_ADD_OFFSET(Information, Information->DataOffset + dataLength - sizeof(WCHAR)) == UNICODE_NULL)
        {
            dataLength -= sizeof(WCHAR);
        }

        value = PhCreateStringEx(PTR_ADD_OFFSET(Information, Information->DataOffset), dataLength);
    }

    name = PhCreateStringEx(Information->Name, Information->NameLength);

    entry = PhCreateJsonObject();
    AtJsonAddString(entry, "name", name);
    AtJsonAddString(entry, "value", value);
    AtJsonAddStringZ(entry, "scope", context->Scope);
    AtAddRow(context->Entries, entry);

    PhClearReference(&name);
    PhClearReference(&value);

    return TRUE;
}

VOID AtpReadEnvironmentKey(
    _In_ HANDLE RootDirectory,
    _In_ PCWSTR SubKey,
    _In_ PCWSTR Scope,
    _Inout_ PAT_ROWS Entries
    )
{
    AT_ENVIRONMENT_CONTEXT context;
    HANDLE keyHandle;
    PH_STRINGREF subKey;

    context.Entries = Entries;
    context.Scope = Scope;

    PhInitializeStringRef(&subKey, SubKey);

    if (NT_SUCCESS(PhOpenKey(&keyHandle, KEY_QUERY_VALUE, RootDirectory, &subKey, 0)))
    {
        PhEnumerateValueKey(keyHandle, KeyValueFullInformation, AtpEnvironmentValueCallback, &context);
        NtClose(keyHandle);
    }
}

VOID AtpGetSystemEnvironment(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    AT_ROWS rows;
    PVOID structured;

    structured = PhCreateJsonObject();
    AtInitializeRows(&rows, Call->Arguments);

    AtpReadEnvironmentKey(
        PH_KEY_LOCAL_MACHINE,
        L"System\\CurrentControlSet\\Control\\Session Manager\\Environment",
        L"machine",
        &rows
        );
    AtpReadEnvironmentKey(
        PH_KEY_CURRENT_USER,
        L"Environment",
        L"user",
        &rows
        );

    AtAddRows(structured, "variables", &rows);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

VOID AtSystemInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    switch (Tool->Action)
    {
    case AtActionGetSystemInfo:
        AtpGetSystemInfo(Result);
        break;
    case AtActionListKernelDrivers:
        AtpListKernelDrivers(Call, Result);
        break;
    case AtActionGetKsiStatus:
        AtpGetKsiStatus(Result);
        break;
    case AtActionGetPagefileInfo:
        AtpGetPagefileInfo(Call, Result);
        break;
    case AtActionListStartupEntries:
        AtListStartupEntries(Call, Result);
        break;
    case AtActionGetSmbiosInfo:
        AtpGetSmbiosInfo(Result);
        break;
    case AtActionGetUefiVariables:
        AtpGetUefiVariables(Call, Result);
        break;
    case AtActionGetTpmInfo:
        AtpGetTpmInfo(Result);
        break;
    case AtActionGetSystemEnvironment:
        AtpGetSystemEnvironment(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
