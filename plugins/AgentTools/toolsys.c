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

VOID AtpAddCapabilities(
    _In_ PVOID Object,
    _In_ KPH_LEVEL KphLevel,
    _In_ BOOLEAN Elevated
    )
{
    PVOID capabilities;
    BOOLEAN extendedTools;

    capabilities = PhCreateJsonObject();
    extendedTools = !!PhFindPlugin(L"ExtendedTools");

    AtJsonAddStringZ(capabilities, "ksi_level", AtKphLevelString(KphLevel));
    PhAddJsonObjectBoolean(capabilities, "elevated", Elevated);

    // Mirrors EtEtwMonitorInitialization: the kernel trace session needs elevation and the setting.
    PhAddJsonObjectBoolean(
        capabilities,
        "etw",
        extendedTools && Elevated && !!PhGetIntegerSetting(L"ExtendedTools.EnableEtwMonitor")
        );
    PhAddJsonObjectBoolean(
        capabilities,
        "gpu",
        extendedTools && !!PhGetIntegerSetting(L"ExtendedTools.EnableGpuMonitor")
        );
    PhAddJsonObjectBoolean(capabilities, "dotnet", !!PhFindPlugin(L"DotNetTools"));
    PhAddJsonObjectBoolean(capabilities, "online_checks", !!PhFindPlugin(L"OnlineChecks"));

    // Mirrors the main window: the informer feed needs the driver at medium and the setting.
    PhAddJsonObjectBoolean(
        capabilities,
        "process_monitor",
        KphLevel >= KphLevelMed && !!PhGetIntegerSetting(L"EnableProcessMonitor")
        );

    PhAddJsonObjectValue(Object, "capabilities", capabilities);
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
    BOOLEAN elevated;
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

    elevated = !!PhGetOwnTokenAttributes().Elevated;
    PhAddJsonObjectBoolean(structured, "system_informer_elevated", elevated);
    PhAddJsonObjectUInt64(structured, "system_informer_pid", HandleToUlong(NtCurrentProcessId()));

    kphLevel = KsiLevel();
    PhAddJsonObjectBoolean(structured, "ksi_connected", kphLevel != KphLevelNone);
    AtJsonAddStringZ(structured, "ksi_level", AtKphLevelString(kphLevel));
    PhAddJsonObjectUInt64(structured, "schema_version", AT_SCHEMA_VERSION);
    AtpAddCapabilities(structured, kphLevel, elevated);

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
    BOOLEAN verifySignatures;
    BOOLEAN needVerify;
    BOOLEAN needMicrosoft;
    BOOLEAN excludeMicrosoft;
    BOOLEAN unsignedOnly;
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
    excludeMicrosoft = AtJsonGetObjectBoolean(Call->Arguments, "exclude_microsoft");
    unsignedOnly = AtJsonGetObjectBoolean(Call->Arguments, "unsigned_only");
    verifySignatures = AtJsonGetObjectBoolean(Call->Arguments, "verify_signatures");

    // Each of these is a separate full signature check of the same file, so each is done only if
    // something actually needs its answer. Asking for both doubles the wait for no reason.
    needVerify = verifySignatures || unsignedOnly;
    needMicrosoft = verifySignatures || excludeMicrosoft;
    verify = needVerify || needMicrosoft;

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
            VERIFY_RESULT verifyResult = VrUnknown;
            BOOLEAN microsoft = FALSE;
            BOOLEAN microsoftKnown = FALSE;

            if (needVerify)
                verifyResult = AtVerifyFileName(fileName, &signer);

            if (needMicrosoft)
                microsoft = AtIsMicrosoftSigned(fileName, &microsoftKnown);

            if ((excludeMicrosoft && microsoft) || (unsignedOnly && verifyResult == VrTrusted))
            {
                PhClearReference(&signer);
                PhFreeJsonObject(row);
                PhClearReference(&fileName);
                PhClearReference(&name);
                continue;
            }

            if (needVerify)
            {
                AtJsonAddStringZ(row, "verify_result", AtVerifyResultString(verifyResult));
                AtJsonAddString(row, "verify_signer", signer);
            }
            else
            {
                AtJsonAddNull(row, "verify_result");
                AtJsonAddNull(row, "verify_signer");
            }

            if (needMicrosoft && microsoftKnown)
                PhAddJsonObjectBoolean(row, "is_microsoft_signed", microsoft);
            else
                AtJsonAddNull(row, "is_microsoft_signed");

            PhClearReference(&signer);
        }
        else
        {
            AtJsonAddNull(row, "verify_result");
            AtJsonAddNull(row, "verify_signer");
            AtJsonAddNull(row, "is_microsoft_signed");
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
    AtJsonAddStringZ(structured, "level", AtKphLevelString(level));

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
    // disabled. Enabled for this thread only, so the process token is untouched.
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

#define AT_TBS_E_TPM_NOT_FOUND ((ULONG)0x8028400FL)

VOID AtpGetTpmInfo(
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static AT_TBSI_GET_DEVICE_INFO Tbsi_GetDeviceInfo_I = NULL;
    PVOID structured;
    AT_TPM_DEVICE_INFO deviceInfo;
    ULONG status;

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

    PhAddJsonObjectBoolean(structured, "tbs_available", !!Tbsi_GetDeviceInfo_I);

    if (!Tbsi_GetDeviceInfo_I)
    {
        // Nothing was asked, so nothing is known: a machine without TPM Base Services is not a
        // machine without a TPM.
        AtJsonAddNull(structured, "present");
        AtJsonAddNull(structured, "version");
        AtJsonAddNull(structured, "interface_type");
        AtJsonAddNull(structured, "implementation_revision");
        AtJsonAddNull(structured, "error_code");
    }
    else if ((status = Tbsi_GetDeviceInfo_I(sizeof(deviceInfo), &deviceInfo)) == 0)
    {
        PhAddJsonObjectBoolean(structured, "present", TRUE);
        AtJsonAddStringZ(structured, "version", AtpTpmVersionString(deviceInfo.TpmVersion));
        AtJsonAddStringZ(structured, "interface_type", AtpTpmInterfaceTypeString(deviceInfo.TpmInterfaceType));
        PhAddJsonObjectUInt64(structured, "implementation_revision", deviceInfo.TpmImpRevision);
        AtJsonAddNull(structured, "error_code");
    }
    else
    {
        // Only this one answer means there is no TPM; every other failure means the question went
        // unanswered.
        if (status == AT_TBS_E_TPM_NOT_FOUND)
            PhAddJsonObjectBoolean(structured, "present", FALSE);
        else
            AtJsonAddNull(structured, "present");

        AtJsonAddNull(structured, "version");
        AtJsonAddNull(structured, "interface_type");
        AtJsonAddNull(structured, "implementation_revision");
        AtJsonAddHex(structured, "error_code", status);
    }

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
        // DataLength is not guaranteed to be a whole number of WCHARs; drop a dangling odd byte.
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

VOID AtpAddMemoryList(
    _In_ PVOID Structured,
    _In_ ULONG PageSize
    )
{
    SYSTEM_MEMORY_LIST_INFORMATION memoryList;
    ULONG64 standby = 0;
    ULONG64 repurposed = 0;
    PVOID entry;
    PVOID priorities;
    ULONG i;

    if (!NT_SUCCESS(NtQuerySystemInformation(
        SystemMemoryListInformation,
        &memoryList,
        sizeof(SYSTEM_MEMORY_LIST_INFORMATION),
        NULL
        )))
    {
        AtJsonAddNull(Structured, "lists");
        return;
    }

    priorities = PhCreateJsonArray();

    // Standby is kept in eight priority buckets and the total is their sum. Priority 0 is
    // repurposed first, so a machine under pressure has its low buckets emptied and its high ones
    // intact.
    for (i = 0; i < RTL_NUMBER_OF(memoryList.PageCountByPriority); i++)
    {
        PVOID row = PhCreateJsonObject();

        standby += memoryList.PageCountByPriority[i];
        repurposed += memoryList.RepurposedPagesByPriority[i];

        PhAddJsonObjectUInt64(row, "priority", i);
        PhAddJsonObjectUInt64(row, "standby_bytes", (ULONG64)memoryList.PageCountByPriority[i] * PageSize);
        PhAddJsonObjectUInt64(row, "repurposed_pages", memoryList.RepurposedPagesByPriority[i]);
        PhAddJsonArrayObject(priorities, row);
    }

    entry = PhCreateJsonObject();
    PhAddJsonObjectUInt64(entry, "zeroed_bytes", (ULONG64)memoryList.ZeroPageCount * PageSize);
    PhAddJsonObjectUInt64(entry, "free_bytes", (ULONG64)memoryList.FreePageCount * PageSize);
    PhAddJsonObjectUInt64(entry, "modified_bytes", (ULONG64)memoryList.ModifiedPageCount * PageSize);
    PhAddJsonObjectUInt64(entry, "modified_no_write_bytes", (ULONG64)memoryList.ModifiedNoWritePageCount * PageSize);
    PhAddJsonObjectUInt64(entry, "modified_page_file_bytes", (ULONG64)memoryList.ModifiedPageCountPageFile * PageSize);
    PhAddJsonObjectUInt64(entry, "bad_bytes", (ULONG64)memoryList.BadPageCount * PageSize);
    PhAddJsonObjectUInt64(entry, "standby_bytes", standby * PageSize);
    PhAddJsonObjectUInt64(entry, "repurposed_pages", repurposed);
    PhAddJsonObjectValue(entry, "by_priority", priorities);
    PhAddJsonObjectValue(Structured, "lists", entry);
}

VOID AtpGetMemoryDetails(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    SYSTEM_BASIC_INFORMATION basicInfo;
    SYSTEM_PERFORMANCE_INFORMATION perfInfo;
    SYSTEM_FILECACHE_INFORMATION cacheInfo;
    PH_SYSTEM_STORE_COMPRESSION_INFORMATION compressionInfo;
    NTSTATUS status;
    ULONG pageSize;
    PVOID structured;
    PVOID entry;

    UNREFERENCED_PARAMETER(Call);

    memset(&basicInfo, 0, sizeof(SYSTEM_BASIC_INFORMATION));

    if (!NT_SUCCESS(status = NtQuerySystemInformation(SystemBasicInformation, &basicInfo, sizeof(SYSTEM_BASIC_INFORMATION), NULL)))
    {
        AtSetToolStatusError(Result, status, L"Reading the system memory information");
        return;
    }

    pageSize = basicInfo.PageSize;

    structured = PhCreateJsonObject();
    PhAddJsonObjectUInt64(structured, "page_size", pageSize);
    PhAddJsonObjectUInt64(structured, "physical_total_bytes", (ULONG64)basicInfo.NumberOfPhysicalPages * pageSize);

    memset(&perfInfo, 0, sizeof(SYSTEM_PERFORMANCE_INFORMATION));

    if (NT_SUCCESS(NtQuerySystemInformation(SystemPerformanceInformation, &perfInfo, sizeof(SYSTEM_PERFORMANCE_INFORMATION), NULL)))
    {
        PhAddJsonObjectUInt64(structured, "physical_available_bytes", (ULONG64)perfInfo.AvailablePages * pageSize);
        PhAddJsonObjectUInt64(structured, "resident_available_bytes", (ULONG64)perfInfo.ResidentAvailablePages * pageSize);
        PhAddJsonObjectUInt64(structured, "cache_resident_bytes", (ULONG64)perfInfo.ResidentSystemCachePage * pageSize);

        entry = PhCreateJsonObject();
        PhAddJsonObjectUInt64(entry, "total_bytes", (ULONG64)perfInfo.CommittedPages * pageSize);
        PhAddJsonObjectUInt64(entry, "limit_bytes", (ULONG64)perfInfo.CommitLimit * pageSize);
        PhAddJsonObjectUInt64(entry, "peak_bytes", (ULONG64)perfInfo.PeakCommitment * pageSize);
        PhAddJsonObjectValue(structured, "commit", entry);

        // Pool usage is here for anyone; the pool LIMITS are only in kernel variables that need
        // symbols and the driver to read, so they are deliberately not reported rather than guessed.
        entry = PhCreateJsonObject();
        PhAddJsonObjectUInt64(entry, "paged_bytes", (ULONG64)perfInfo.PagedPoolPages * pageSize);
        PhAddJsonObjectUInt64(entry, "paged_available_bytes", (ULONG64)perfInfo.AvailablePagedPoolPages * pageSize);
        PhAddJsonObjectUInt64(entry, "paged_resident_bytes", (ULONG64)perfInfo.ResidentPagedPoolPage * pageSize);
        PhAddJsonObjectUInt64(entry, "non_paged_bytes", (ULONG64)perfInfo.NonPagedPoolPages * pageSize);
        PhAddJsonObjectUInt64(entry, "paged_allocs", perfInfo.PagedPoolAllocs);
        PhAddJsonObjectUInt64(entry, "paged_frees", perfInfo.PagedPoolFrees);
        PhAddJsonObjectUInt64(entry, "non_paged_allocs", perfInfo.NonPagedPoolAllocs);
        PhAddJsonObjectUInt64(entry, "non_paged_frees", perfInfo.NonPagedPoolFrees);
        PhAddJsonObjectValue(structured, "pools", entry);

        // The fault breakdown, which says what kind of pressure this is: transitions come back
        // from the standby list and cost nothing, demand-zero is new memory being handed out.
        entry = PhCreateJsonObject();
        PhAddJsonObjectUInt64(entry, "total", perfInfo.PageFaultCount);
        PhAddJsonObjectUInt64(entry, "transition", perfInfo.TransitionCount);
        PhAddJsonObjectUInt64(entry, "cache_transition", perfInfo.CacheTransitionCount);
        PhAddJsonObjectUInt64(entry, "demand_zero", perfInfo.DemandZeroCount);
        PhAddJsonObjectUInt64(entry, "copy_on_write", perfInfo.CopyOnWriteCount);
        PhAddJsonObjectValue(structured, "page_faults", entry);
    }
    else
    {
        AtJsonAddNull(structured, "physical_available_bytes");
        AtJsonAddNull(structured, "resident_available_bytes");
        AtJsonAddNull(structured, "cache_resident_bytes");
        AtJsonAddNull(structured, "commit");
        AtJsonAddNull(structured, "pools");
        AtJsonAddNull(structured, "page_faults");
    }

    AtpAddMemoryList(structured, pageSize);

    if (NT_SUCCESS(PhGetSystemFileCacheSize(&cacheInfo)))
    {
        entry = PhCreateJsonObject();
        PhAddJsonObjectUInt64(entry, "current_bytes", cacheInfo.CurrentSize);
        PhAddJsonObjectUInt64(entry, "peak_bytes", cacheInfo.PeakSize);
        PhAddJsonObjectUInt64(entry, "minimum_working_set_bytes", cacheInfo.MinimumWorkingSet);
        PhAddJsonObjectUInt64(entry, "maximum_working_set_bytes", cacheInfo.MaximumWorkingSet);
        PhAddJsonObjectUInt64(entry, "current_including_transition_bytes", cacheInfo.CurrentSizeIncludingTransitionInPages);
        PhAddJsonObjectUInt64(entry, "page_fault_count", cacheInfo.PageFaultCount);
        PhAddJsonObjectValue(structured, "file_cache", entry);
    }
    else
    {
        AtJsonAddNull(structured, "file_cache");
    }

    // The compression store is a process holding compressed pages, so the memory it accounts for is
    // not on any list above: it is inside that process's working set.
    if (NT_SUCCESS(PhGetSystemCompressionStoreInformation(&compressionInfo)))
    {
        entry = PhCreateJsonObject();
        PhAddJsonObjectUInt64(entry, "pid", compressionInfo.CompressionPid);
        PhAddJsonObjectUInt64(entry, "working_set_bytes", compressionInfo.WorkingSetSize);
        PhAddJsonObjectUInt64(entry, "total_data_compressed_bytes", compressionInfo.TotalDataCompressed);
        PhAddJsonObjectUInt64(entry, "total_compressed_size_bytes", compressionInfo.TotalCompressedSize);
        PhAddJsonObjectUInt64(entry, "total_unique_data_compressed_bytes", compressionInfo.TotalUniqueDataCompressed);
        PhAddJsonObjectValue(structured, "compression_store", entry);
    }
    else
    {
        AtJsonAddNull(structured, "compression_store");
    }

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

PCWSTR AtpVirtualStatusString(
    _In_ PH_VIRTUAL_STATUS Status
    )
{
    switch (Status)
    {
    case PhVirtualStatusEnabledHyperV:
        return L"enabled_hyper_v";
    case PhVirtualStatusEnabledFirmware:
        return L"enabled_firmware";
    case PhVirtualStatusDisabledWithHyperV:
        return L"disabled_with_hyper_v";
    case PhVirtualStatusDisabled:
        return L"disabled";
    case PhVirtualStatusVirtualMachine:
        return L"virtual_machine";
    case PhVirtualStatusNotCapable:
        return L"not_capable";
    }

    return NULL;
}

VOID AtpAddSecureBoot(
    _In_ PVOID Structured
    )
{
    SYSTEM_SECUREBOOT_INFORMATION secureBoot;
    PVOID entry;

    if (!NT_SUCCESS(NtQuerySystemInformation(
        SystemSecureBootInformation,
        &secureBoot,
        sizeof(SYSTEM_SECUREBOOT_INFORMATION),
        NULL
        )))
    {
        AtJsonAddNull(Structured, "secure_boot");
        return;
    }

    entry = PhCreateJsonObject();
    PhAddJsonObjectBoolean(entry, "enabled", !!secureBoot.SecureBootEnabled);
    PhAddJsonObjectBoolean(entry, "capable", !!secureBoot.SecureBootCapable);
    PhAddJsonObjectValue(Structured, "secure_boot", entry);
}

VOID AtpAddCodeIntegrity(
    _In_ PVOID Structured
    )
{
    static CONST ULONG optionFlags[] =
    {
        CODEINTEGRITY_OPTION_ENABLED, CODEINTEGRITY_OPTION_TESTSIGN,
        CODEINTEGRITY_OPTION_UMCI_ENABLED, CODEINTEGRITY_OPTION_UMCI_AUDITMODE_ENABLED,
        CODEINTEGRITY_OPTION_UMCI_EXCLUSIONPATHS_ENABLED, CODEINTEGRITY_OPTION_TEST_BUILD,
        CODEINTEGRITY_OPTION_PREPRODUCTION_BUILD, CODEINTEGRITY_OPTION_DEBUGMODE_ENABLED,
        CODEINTEGRITY_OPTION_FLIGHT_BUILD, CODEINTEGRITY_OPTION_FLIGHTING_ENABLED,
        CODEINTEGRITY_OPTION_HVCI_KMCI_ENABLED, CODEINTEGRITY_OPTION_HVCI_KMCI_AUDITMODE_ENABLED,
        CODEINTEGRITY_OPTION_HVCI_KMCI_STRICTMODE_ENABLED, CODEINTEGRITY_OPTION_HVCI_IUM_ENABLED,
        CODEINTEGRITY_OPTION_WHQL_ENFORCEMENT_ENABLED, CODEINTEGRITY_OPTION_WHQL_AUDITMODE_ENABLED,
    };
    static CONST PWSTR optionNames[] =
    {
        L"enabled", L"test_signing", L"umci_enabled", L"umci_audit_mode",
        L"umci_exclusion_paths", L"test_build", L"preproduction_build", L"debug_mode",
        L"flight_build", L"flighting_enabled", L"hvci_kmci_enabled", L"hvci_kmci_audit_mode",
        L"hvci_kmci_strict_mode", L"hvci_ium_enabled", L"whql_enforcement", L"whql_audit_mode",
    };
    SYSTEM_CODEINTEGRITY_INFORMATION codeIntegrity;
    PVOID entry;

    memset(&codeIntegrity, 0, sizeof(SYSTEM_CODEINTEGRITY_INFORMATION));
    codeIntegrity.Length = sizeof(SYSTEM_CODEINTEGRITY_INFORMATION);

    if (!NT_SUCCESS(NtQuerySystemInformation(
        SystemCodeIntegrityInformation,
        &codeIntegrity,
        sizeof(SYSTEM_CODEINTEGRITY_INFORMATION),
        NULL
        )))
    {
        AtJsonAddNull(Structured, "code_integrity");
        return;
    }

    entry = PhCreateJsonObject();
    AtJsonAddHex(entry, "options_value", codeIntegrity.CodeIntegrityOptions);
    AtJsonAddFlagStrings(entry, "options", codeIntegrity.CodeIntegrityOptions, optionFlags, (CONST PWSTR*)optionNames, RTL_NUMBER_OF(optionFlags));
    PhAddJsonObjectBoolean(entry, "enabled", !!FlagOn(codeIntegrity.CodeIntegrityOptions, CODEINTEGRITY_OPTION_ENABLED));
    PhAddJsonObjectBoolean(entry, "test_signing", !!FlagOn(codeIntegrity.CodeIntegrityOptions, CODEINTEGRITY_OPTION_TESTSIGN));
    PhAddJsonObjectBoolean(entry, "debug_mode", !!FlagOn(codeIntegrity.CodeIntegrityOptions, CODEINTEGRITY_OPTION_DEBUGMODE_ENABLED));
    PhAddJsonObjectValue(Structured, "code_integrity", entry);
}

VOID AtpAddVirtualizationSecurity(
    _In_ PVOID Structured
    )
{
    SYSTEM_ISOLATED_USER_MODE_INFORMATION isolatedUserMode;
    PVOID entry;

    if (!NT_SUCCESS(NtQuerySystemInformation(
        SystemIsolatedUserModeInformation,
        &isolatedUserMode,
        sizeof(SYSTEM_ISOLATED_USER_MODE_INFORMATION),
        NULL
        )))
    {
        AtJsonAddNull(Structured, "vbs");
        return;
    }

    entry = PhCreateJsonObject();
    PhAddJsonObjectBoolean(entry, "secure_kernel_running", !!isolatedUserMode.SecureKernelRunning);
    PhAddJsonObjectBoolean(entry, "hvci_enabled", !!isolatedUserMode.HvciEnabled);
    PhAddJsonObjectBoolean(entry, "hvci_strict_mode", !!isolatedUserMode.HvciStrictMode);
    PhAddJsonObjectBoolean(entry, "hvci_disable_allowed", !!isolatedUserMode.HvciDisableAllowed);
    PhAddJsonObjectBoolean(entry, "debug_enabled", !!isolatedUserMode.DebugEnabled);
    PhAddJsonObjectBoolean(entry, "firmware_page_protection", !!isolatedUserMode.FirmwarePageProtection);
    PhAddJsonObjectBoolean(entry, "trustlet_running", !!isolatedUserMode.TrustletRunning);
    PhAddJsonObjectBoolean(entry, "hardware_enforced_vbs", !!isolatedUserMode.HardwareEnforcedVbs);
    PhAddJsonObjectBoolean(entry, "encryption_key_available", !!isolatedUserMode.EncryptionKeyAvailable);
    PhAddJsonObjectBoolean(entry, "encryption_key_tpm_bound", !!isolatedUserMode.EncryptionKeyTpmBound);
    PhAddJsonObjectValue(Structured, "vbs", entry);
}

VOID AtpAddKvaShadow(
    _In_ PVOID Structured
    )
{
    SYSTEM_KERNEL_VA_SHADOW_INFORMATION kvaShadow;
    PVOID entry;

    if (!NT_SUCCESS(NtQuerySystemInformation(
        SystemKernelVaShadowInformation,
        &kvaShadow,
        sizeof(SYSTEM_KERNEL_VA_SHADOW_INFORMATION),
        NULL
        )))
    {
        AtJsonAddNull(Structured, "kva_shadow");
        return;
    }

    entry = PhCreateJsonObject();
    AtJsonAddHex(entry, "flags_value", kvaShadow.KvaShadowFlags);
    PhAddJsonObjectBoolean(entry, "enabled", !!kvaShadow.KvaShadowEnabled);
    PhAddJsonObjectBoolean(entry, "required", !!kvaShadow.KvaShadowRequired);
    PhAddJsonObjectBoolean(entry, "required_available", !!kvaShadow.KvaShadowRequiredAvailable);
    PhAddJsonObjectBoolean(entry, "pcid", !!kvaShadow.KvaShadowPcid);
    PhAddJsonObjectBoolean(entry, "invpcid", !!kvaShadow.KvaShadowInvpcid);
    PhAddJsonObjectBoolean(entry, "l1_data_cache_flush_supported", !!kvaShadow.L1DataCacheFlushSupported);
    PhAddJsonObjectBoolean(entry, "l1_terminal_fault_mitigation", !!kvaShadow.L1TerminalFaultMitigationPresent);
    PhAddJsonObjectValue(Structured, "kva_shadow", entry);
}

VOID AtpAddSpeculationControl(
    _In_ PVOID Structured
    )
{
    SYSTEM_SPECULATION_CONTROL_INFORMATION speculation;
    PVOID entry;

    if (!NT_SUCCESS(NtQuerySystemInformation(
        SystemSpeculationControlInformation,
        &speculation,
        sizeof(SYSTEM_SPECULATION_CONTROL_INFORMATION),
        NULL
        )))
    {
        AtJsonAddNull(Structured, "speculation_control");
        return;
    }

    entry = PhCreateJsonObject();
    AtJsonAddHex(entry, "flags_value", speculation.SpeculationControlFlags.Flags);
    PhAddJsonObjectBoolean(entry, "branch_prediction_barrier_enabled", !!speculation.SpeculationControlFlags.BpbEnabled);
    PhAddJsonObjectBoolean(entry, "branch_prediction_barrier_disabled_by_policy", !!speculation.SpeculationControlFlags.BpbDisabledSystemPolicy);
    PhAddJsonObjectBoolean(entry, "branch_prediction_barrier_no_hardware", !!speculation.SpeculationControlFlags.BpbDisabledNoHardwareSupport);
    PhAddJsonObjectBoolean(entry, "ibrs_present", !!speculation.SpeculationControlFlags.IbrsPresent);
    PhAddJsonObjectBoolean(entry, "enhanced_ibrs", !!speculation.SpeculationControlFlags.EnhancedIbrs);
    PhAddJsonObjectBoolean(entry, "stibp_present", !!speculation.SpeculationControlFlags.StibpPresent);
    PhAddJsonObjectBoolean(entry, "smep_present", !!speculation.SpeculationControlFlags.SmepPresent);
    PhAddJsonObjectBoolean(entry, "retpoline_enabled", !!speculation.SpeculationControlFlags.SpecCtrlRetpolineEnabled);
    PhAddJsonObjectBoolean(entry, "import_optimization_enabled", !!speculation.SpeculationControlFlags.SpecCtrlImportOptimizationEnabled);
    PhAddJsonObjectBoolean(entry, "ssbd_available", !!speculation.SpeculationControlFlags.SpeculativeStoreBypassDisableAvailable);
    PhAddJsonObjectBoolean(entry, "ssbd_system_wide", !!speculation.SpeculationControlFlags.SpeculativeStoreBypassDisabledSystemWide);
    PhAddJsonObjectBoolean(entry, "ssbd_kernel", !!speculation.SpeculationControlFlags.SpeculativeStoreBypassDisabledKernel);
    PhAddJsonObjectValue(Structured, "speculation_control", entry);
}

VOID AtpAddShadowStack(
    _In_ PVOID Structured
    )
{
    SYSTEM_SHADOW_STACK_INFORMATION shadowStack;
    PVOID entry;

    if (!NT_SUCCESS(PhGetSystemShadowStackInformation(&shadowStack)))
    {
        AtJsonAddNull(Structured, "cet");
        return;
    }

    entry = PhCreateJsonObject();
    AtJsonAddHex(entry, "flags_value", shadowStack.Flags);
    PhAddJsonObjectBoolean(entry, "capable", !!shadowStack.CetCapable);
    PhAddJsonObjectBoolean(entry, "user_allowed", !!shadowStack.UserCetAllowed);
    PhAddJsonObjectBoolean(entry, "kernel_enabled", !!shadowStack.KernelCetEnabled);
    PhAddJsonObjectBoolean(entry, "kernel_audit_mode", !!shadowStack.KernelCetAuditModeEnabled);
    PhAddJsonObjectValue(Structured, "cet", entry);
}

VOID AtpGetSecurityPosture(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    BOOLEAN debuggerEnabled = FALSE;
    BOOLEAN debuggerPresent = FALSE;
    PVOID structured;
    PVOID entry;

    UNREFERENCED_PARAMETER(Call);

    structured = PhCreateJsonObject();

    AtpAddSecureBoot(structured);
    AtpAddCodeIntegrity(structured);
    AtpAddVirtualizationSecurity(structured);
    AtpAddKvaShadow(structured);
    AtpAddSpeculationControl(structured);
    AtpAddShadowStack(structured);

    // A kernel debugger can read and write anything, so it is reported next to the mitigations
    // rather than buried: it is the one setting that makes all the others beside the point.
    if (NT_SUCCESS(PhGetKernelDebuggerInformation(&debuggerEnabled, &debuggerPresent)))
    {
        entry = PhCreateJsonObject();
        PhAddJsonObjectBoolean(entry, "enabled", debuggerEnabled);
        PhAddJsonObjectBoolean(entry, "present", debuggerPresent);
        PhAddJsonObjectValue(structured, "kernel_debugger", entry);
    }
    else
    {
        AtJsonAddNull(structured, "kernel_debugger");
    }

    AtJsonAddStringZ(structured, "virtualization", AtpVirtualStatusString(PhGetVirtualStatus()));
    AtJsonAddStringZ(structured, "firmware_type", AtpFirmwareTypeString(AtpGetFirmwareType()));

    // The driver's own state belongs here too, because it is what decides how much of the rest of
    // this server works; get_ksi_status has the detail.
    AtJsonAddStringZ(structured, "ksi_level", AtKphLevelString(KsiLevel()));
    PhAddJsonObjectBoolean(structured, "system_informer_elevated", !!PhGetOwnTokenAttributes().Elevated);

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}


PPH_STRING AtpGetCpuBrandString(
    VOID
    )
{
    SYSTEM_PROCESSOR_BRAND_STRING brandString;
    PPH_STRING brand;

    if (!NT_SUCCESS(NtQuerySystemInformation(
        SystemProcessorBrandString,
        &brandString,
        sizeof(SYSTEM_PROCESSOR_BRAND_STRING),
        NULL
        )))
    {
        return NULL;
    }

    // The field is a fixed buffer the firmware filled and is padded with spaces on most parts.
    brand = PhConvertUtf8ToUtf16Ex(brandString.BrandString, sizeof(brandString.BrandString) - sizeof(ANSI_NULL));

    if (brand)
    {
        static CONST PH_STRINGREF whitespace = PH_STRINGREF_INIT(L" ");

        PhTrimToNullTerminatorString(brand);
        PhTrimStringRef(&brand->sr, &whitespace, 0);
        PhMoveReference(&brand, PhCreateString2(&brand->sr));
    }

    return brand;
}

PCWSTR AtpCacheTypeString(
    _In_ PROCESSOR_CACHE_TYPE Type
    )
{
    switch (Type)
    {
    case CacheUnified:
        return L"unified";
    case CacheInstruction:
        return L"instruction";
    case CacheData:
        return L"data";
    case CacheTrace:
        return L"trace";
    }

    return NULL;
}

VOID AtpAddCpuCaches(
    _In_ PVOID Structured
    )
{
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX buffer;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX entry;
    ULONG bufferLength;
    ULONG offset;
    PVOID array;

    if (!NT_SUCCESS(PhGetSystemLogicalProcessorInformation(RelationCache, &buffer, &bufferLength)))
    {
        AtJsonAddNull(Structured, "caches");
        return;
    }

    array = PhCreateJsonArray();

    // Variable-length records: each carries its own size and the next one follows it.
    for (offset = 0; offset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX) <= bufferLength; offset += entry->Size)
    {
        PVOID row;

        entry = PTR_ADD_OFFSET(buffer, offset);

        if (entry->Size == 0)
            break;
        if (entry->Relationship != RelationCache)
            continue;

        row = PhCreateJsonObject();
        PhAddJsonObjectUInt64(row, "level", entry->Cache.Level);
        AtJsonAddStringZ(row, "type", AtpCacheTypeString(entry->Cache.Type));
        PhAddJsonObjectUInt64(row, "size_bytes", entry->Cache.CacheSize);
        PhAddJsonObjectUInt64(row, "line_size", entry->Cache.LineSize);
        PhAddJsonObjectUInt64(row, "associativity", entry->Cache.Associativity);
        PhAddJsonObjectUInt64(row, "group", entry->Cache.GroupMask.Group);
        AtJsonAddHex(row, "processor_mask", entry->Cache.GroupMask.Mask);
        PhAddJsonArrayObject(array, row);
    }

    PhAddJsonObjectValue(Structured, "caches", array);
    PhFree(buffer);
}

VOID AtpAddCpuProcessors(
    _In_ PVOID Structured,
    _In_ PAT_TOOL_CALL Call
    )
{
    PSYSTEM_CPU_SET_INFORMATION buffer;
    ULONG bufferLength = 0;
    ULONG offset;
    AT_ROWS rows;
    ULONG efficiencyClasses = 0;
    ULONG parkedCount = 0;

    // One query answers the whole per-processor picture: which core and cache and NUMA node each
    // logical processor belongs to, whether it is parked, and its efficiency class.
    if (!NT_SUCCESS(NtQuerySystemInformationEx(
        SystemCpuSetInformation,
        &(HANDLE){ NULL },
        sizeof(HANDLE),
        NULL,
        0,
        &bufferLength
        )) && bufferLength == 0)
    {
        AtJsonAddNull(Structured, "processors");
        AtJsonAddNull(Structured, "parked_count");
        AtJsonAddNull(Structured, "efficiency_classes");
        return;
    }

    buffer = PhAllocate(bufferLength);

    if (!NT_SUCCESS(NtQuerySystemInformationEx(
        SystemCpuSetInformation,
        &(HANDLE){ NULL },
        sizeof(HANDLE),
        buffer,
        bufferLength,
        &bufferLength
        )))
    {
        AtJsonAddNull(Structured, "processors");
        AtJsonAddNull(Structured, "parked_count");
        AtJsonAddNull(Structured, "efficiency_classes");
        PhFree(buffer);
        return;
    }

    AtInitializeRows(&rows, Call->Arguments);

    for (offset = 0; offset < bufferLength; )
    {
        PSYSTEM_CPU_SET_INFORMATION entry = PTR_ADD_OFFSET(buffer, offset);
        PH_PROCESSOR_NUMBER processorNumber;
        ULONG frequency;
        PVOID row;

        if (entry->Size == 0)
            break;

        offset += entry->Size;

        if (entry->Type != CpuSetInformation)
            continue;

        row = PhCreateJsonObject();
        PhAddJsonObjectUInt64(row, "id", entry->CpuSet.Id);
        PhAddJsonObjectUInt64(row, "group", entry->CpuSet.Group);
        PhAddJsonObjectUInt64(row, "logical_processor_index", entry->CpuSet.LogicalProcessorIndex);
        PhAddJsonObjectUInt64(row, "core_index", entry->CpuSet.CoreIndex);
        PhAddJsonObjectUInt64(row, "last_level_cache_index", entry->CpuSet.LastLevelCacheIndex);
        PhAddJsonObjectUInt64(row, "numa_node_index", entry->CpuSet.NumaNodeIndex);
        // On a hybrid part this is what separates performance cores from efficiency cores: the
        // higher the class, the more performant. Every processor reports 0 on a part that is not
        // hybrid, which is why the count of distinct classes is reported alongside.
        PhAddJsonObjectUInt64(row, "efficiency_class", entry->CpuSet.EfficiencyClass);
        PhAddJsonObjectUInt64(row, "scheduling_class", entry->CpuSet.SchedulingClass);
        PhAddJsonObjectBoolean(row, "parked", !!entry->CpuSet.Parked);
        PhAddJsonObjectBoolean(row, "allocated", !!entry->CpuSet.Allocated);
        PhAddJsonObjectBoolean(row, "real_time", !!entry->CpuSet.RealTime);

        processorNumber.Group = entry->CpuSet.Group;
        processorNumber.Number = entry->CpuSet.LogicalProcessorIndex;

        if (NT_SUCCESS(PhGetProcessorNominalFrequency(&processorNumber, &frequency)))
            PhAddJsonObjectUInt64(row, "nominal_frequency_mhz", frequency);
        else
            AtJsonAddNull(row, "nominal_frequency_mhz");

        if (entry->CpuSet.Parked)
            parkedCount++;

        SetFlag(efficiencyClasses, 1ul << (entry->CpuSet.EfficiencyClass & 31));

        AtAddRow(&rows, row);
    }

    AtAddRows(Structured, "processors", &rows);
    PhAddJsonObjectUInt64(Structured, "parked_count", parkedCount);
    {
        ULONG classCount = 0;
        ULONG i;

        for (i = 0; i < 32; i++)
        {
            if (FlagOn(efficiencyClasses, 1ul << i))
                classCount++;
        }

        PhAddJsonObjectUInt64(Structured, "efficiency_classes", classCount);
    }

    AtDeleteRows(&rows);
    PhFree(buffer);
}

VOID AtpGetCpuInfo(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    SYSTEM_BASIC_INFORMATION basicInfo;
    PH_LOGICAL_PROCESSOR_INFORMATION logicalInfo;
    PPH_STRING brand;
    NTSTATUS status;
    PVOID structured;
    PVOID entry;

    memset(&basicInfo, 0, sizeof(SYSTEM_BASIC_INFORMATION));

    if (!NT_SUCCESS(status = NtQuerySystemInformation(SystemBasicInformation, &basicInfo, sizeof(SYSTEM_BASIC_INFORMATION), NULL)))
    {
        AtSetToolStatusError(Result, status, L"Reading the processor information");
        return;
    }

    structured = PhCreateJsonObject();

    brand = AtpGetCpuBrandString();
    AtJsonAddString(structured, "brand", brand);
    PhClearReference(&brand);

#if defined(_M_ARM64)
    PhAddJsonObject(structured, "architecture", "ARM64");
#elif defined(_M_X64)
    PhAddJsonObject(structured, "architecture", "x64");
#else
    PhAddJsonObject(structured, "architecture", "x86");
#endif

    PhAddJsonObjectUInt64(structured, "logical_processor_count", basicInfo.NumberOfProcessors);
    PhAddJsonObjectUInt64(structured, "page_size", basicInfo.PageSize);
    PhAddJsonObjectUInt64(structured, "allocation_granularity", basicInfo.AllocationGranularity);

    if (NT_SUCCESS(PhGetSystemLogicalProcessorRelationInformation(&logicalInfo)))
    {
        entry = PhCreateJsonObject();
        PhAddJsonObjectUInt64(entry, "cores", logicalInfo.ProcessorCoreCount);
        PhAddJsonObjectUInt64(entry, "logical_processors", logicalInfo.ProcessorLogicalCount);
        PhAddJsonObjectUInt64(entry, "packages", logicalInfo.ProcessorPackageCount);
        PhAddJsonObjectUInt64(entry, "numa_nodes", logicalInfo.ProcessorNumaCount);
        PhAddJsonObjectBoolean(
            entry,
            "hyperthreaded",
            logicalInfo.ProcessorCoreCount != 0 && logicalInfo.ProcessorLogicalCount > logicalInfo.ProcessorCoreCount
            );
        PhAddJsonObjectValue(structured, "topology", entry);
    }
    else
    {
        AtJsonAddNull(structured, "topology");
    }

    AtpAddCpuCaches(structured);
    AtpAddCpuProcessors(structured, Call);

    // The provider's own per-interval usage fractions, not counters since boot: the same figures
    // get_system_history records.
    entry = PhCreateJsonObject();
    PhAddJsonObjectDouble(entry, "cpu_usage", (DOUBLE)PhCpuKernelUsage + (DOUBLE)PhCpuUserUsage);
    PhAddJsonObjectDouble(entry, "cpu_kernel_usage", (DOUBLE)PhCpuKernelUsage);
    PhAddJsonObjectDouble(entry, "cpu_user_usage", (DOUBLE)PhCpuUserUsage);
    PhAddJsonObjectValue(structured, "usage", entry);

    AtJsonAddStringZ(structured, "virtualization", AtpVirtualStatusString(PhGetVirtualStatus()));

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

typedef struct _AT_POOL_TAG_ENTRY
{
    ULONG TagUlong;
    ULONG PagedAllocs;
    ULONG PagedFrees;
    ULONG64 PagedUsed;
    ULONG NonPagedAllocs;
    ULONG NonPagedFrees;
    ULONG64 NonPagedUsed;
    ULONG BigAllocations;
    ULONG64 BigBytes;
} AT_POOL_TAG_ENTRY, *PAT_POOL_TAG_ENTRY;

#define AT_POOL_TAG_PROTECTED 0x80000000

PPH_STRING AtpFormatPoolTag(
    _In_ ULONG TagUlong
    )
{
    WCHAR buffer[5];
    ULONG tag;
    ULONG i;

    // The four characters are the tag's own bytes, taken out of it by shifting rather than by
    // writing a ULONG through a byte array.
    tag = TagUlong & ~AT_POOL_TAG_PROTECTED;

    for (i = 0; i < 4; i++)
    {
        UCHAR value = (UCHAR)(tag >> (i * 8));

        buffer[i] = (value >= 0x20 && value <= 0x7e) ? (WCHAR)value : L'.';
    }

    buffer[4] = UNICODE_NULL;

    return PhCreateString(buffer);
}

VOID AtpListPoolTags(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    AT_ROWS rows;
    PSYSTEM_POOLTAG_INFORMATION poolTable = NULL;
    PSYSTEM_BIGPOOL_INFORMATION bigPool = NULL;
    PPH_HASHTABLE index = NULL;
    PAT_POOL_TAG_ENTRY entries = NULL;
    SIZE_T entriesSize;
    PPH_STRING tagFilter;
    PVOID structured;
    ULONG64 minimumBytes = 0;
    ULONG64 pagedTotal = 0;
    ULONG64 nonPagedTotal = 0;
    ULONG64 bigTotal = 0;
    ULONG bigCount = 0;
    ULONG i;

    if (!NT_SUCCESS(status = PhEnumPoolTagInformation(&poolTable)))
    {
        AtSetToolStatusError(Result, status, L"Reading the pool tag table");
        return;
    }

    tagFilter = AtGetArgumentString(Call->Arguments, "tag");
    AtGetArgumentUInt64(Call->Arguments, "min_bytes", &minimumBytes);

    if (!NT_SUCCESS(RtlSizeTMult(sizeof(AT_POOL_TAG_ENTRY), poolTable->Count, &entriesSize)))
    {
        AtSetToolError(Result, "failed", STATUS_INTEGER_OVERFLOW, L"The pool tag table is too large to read.");
        PhFree(poolTable);
        return;
    }

    entries = PhAllocateZero(entriesSize);
    index = PhCreateSimpleHashtable(poolTable->Count);

    for (i = 0; i < poolTable->Count; i++)
    {
        PSYSTEM_POOLTAG tag = &poolTable->TagInfo[i];

        entries[i].TagUlong = tag->TagUlong;
        entries[i].PagedAllocs = tag->PagedAllocs;
        entries[i].PagedFrees = tag->PagedFrees;
        entries[i].PagedUsed = tag->PagedUsed;
        entries[i].NonPagedAllocs = tag->NonPagedAllocs;
        entries[i].NonPagedFrees = tag->NonPagedFrees;
        entries[i].NonPagedUsed = tag->NonPagedUsed;

        pagedTotal += tag->PagedUsed;
        nonPagedTotal += tag->NonPagedUsed;

        // The index maps a tag to its row so the big pool list can be folded in by tag; the stored
        // value is the row number plus one, and the read below subtracts it.
        PhAddItemSimpleHashtable(index, (PVOID)(ULONG_PTR)tag->TagUlong, (PVOID)(ULONG_PTR)(i + 1));
    }

    // Allocations too big for the pool blocks are tracked one by one rather than by tag, so they
    // are counted per tag here: a tag whose big allocations are growing is a leak the tag table
    // alone does not show.
    if (NT_SUCCESS(PhEnumBigPoolInformation(&bigPool)))
    {
        for (i = 0; i < bigPool->Count; i++)
        {
            PSYSTEM_BIGPOOL_ENTRY allocation = &bigPool->AllocatedInfo[i];
            PVOID* found;

            bigCount++;
            bigTotal += allocation->SizeInBytes;

            if (found = PhFindItemSimpleHashtable(index, (PVOID)(ULONG_PTR)allocation->TagUlong))
            {
                PAT_POOL_TAG_ENTRY entry = &entries[(ULONG)(ULONG_PTR)*found - 1];

                entry->BigAllocations++;
                entry->BigBytes += allocation->SizeInBytes;
            }
        }
    }

    structured = PhCreateJsonObject();
    AtInitializeRows(&rows, Call->Arguments);

    for (i = 0; i < poolTable->Count; i++)
    {
        PAT_POOL_TAG_ENTRY entry = &entries[i];
        PPH_STRING tag;
        PVOID row;
        ULONG64 totalBytes;

        totalBytes = entry->PagedUsed + entry->NonPagedUsed;

        if (totalBytes < minimumBytes)
            continue;

        tag = AtpFormatPoolTag(entry->TagUlong);

        if (tagFilter && !PhEqualString(tag, tagFilter, TRUE))
        {
            PhDereferenceObject(tag);
            continue;
        }

        row = PhCreateJsonObject();
        AtJsonAddString(row, "tag", tag);
        AtJsonAddHex(row, "tag_value", entry->TagUlong);
        PhAddJsonObjectBoolean(row, "protected", !!(entry->TagUlong & AT_POOL_TAG_PROTECTED));

        PhAddJsonObjectUInt64(row, "paged_allocs", entry->PagedAllocs);
        PhAddJsonObjectUInt64(row, "paged_frees", entry->PagedFrees);
        // Allocation counts are 32 bit and wrap, so the difference is reported as the signed number
        // it is rather than as an enormous unsigned one.
        PhAddJsonObjectInt64(row, "paged_current", (LONG)(entry->PagedAllocs - entry->PagedFrees));
        PhAddJsonObjectUInt64(row, "paged_bytes", entry->PagedUsed);

        PhAddJsonObjectUInt64(row, "nonpaged_allocs", entry->NonPagedAllocs);
        PhAddJsonObjectUInt64(row, "nonpaged_frees", entry->NonPagedFrees);
        PhAddJsonObjectInt64(row, "nonpaged_current", (LONG)(entry->NonPagedAllocs - entry->NonPagedFrees));
        PhAddJsonObjectUInt64(row, "nonpaged_bytes", entry->NonPagedUsed);

        PhAddJsonObjectUInt64(row, "total_bytes", totalBytes);
        PhAddJsonObjectUInt64(row, "big_allocations", entry->BigAllocations);
        PhAddJsonObjectUInt64(row, "big_bytes", entry->BigBytes);

        AtAddRow(&rows, row);
        PhDereferenceObject(tag);
    }

    AtAddRows(structured, "tags", &rows);
    PhAddJsonObjectUInt64(structured, "tag_count", poolTable->Count);
    PhAddJsonObjectUInt64(structured, "paged_bytes_total", pagedTotal);
    PhAddJsonObjectUInt64(structured, "nonpaged_bytes_total", nonPagedTotal);
    PhAddJsonObjectBoolean(structured, "big_pool_read", !!bigPool);

    if (bigPool)
    {
        PhAddJsonObjectUInt64(structured, "big_pool_allocations", bigCount);
        PhAddJsonObjectUInt64(structured, "big_pool_bytes", bigTotal);
    }
    else
    {
        AtJsonAddNull(structured, "big_pool_allocations");
        AtJsonAddNull(structured, "big_pool_bytes");
    }

    AtAddSnapshot(structured);
    Result->StructuredContent = structured;

    PhDereferenceObject(index);
    PhFree(entries);
    PhClearReference(&tagFilter);

    if (bigPool)
        PhFree(bigPool);

    PhFree(poolTable);
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
    case AtActionListPoolTags:
        AtpListPoolTags(Call, Result);
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
    case AtActionGetMemoryDetails:
        AtpGetMemoryDetails(Call, Result);
        break;
    case AtActionGetSecurityPosture:
        AtpGetSecurityPosture(Call, Result);
        break;
    case AtActionGetCpuInfo:
        AtpGetCpuInfo(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
