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

// Provider CPU usage, exported as data by SystemInformer.exe.
__declspec(dllimport) FLOAT PhCpuKernelUsage;
__declspec(dllimport) FLOAT PhCpuUserUsage;

//
// System information
//

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

    // Process/thread/handle totals from the provider cache.
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
    PVOID structured;
    PVOID rows;
    ULONG count = 0;
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
    rows = PhCreateJsonArray();

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

        PhAddJsonArrayObject(rows, row);
        count++;

        PhClearReference(&fileName);
        PhClearReference(&name);
    }

    PhFree(modules);
    PhClearReference(&nameContains);

    PhAddJsonObjectValue(structured, "drivers", rows);
    PhAddJsonObjectUInt64(structured, "count", count);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

//
// KSI driver status
//

VOID AtpGetKsiStatus(
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    KPH_LEVEL level;
    PVOID structured;

    level = KsiLevel();

    structured = PhCreateJsonObject();
    PhAddJsonObjectBoolean(structured, "connected", level != KphLevelNone);
    AtJsonAddStringZ(structured, "level", AtpKphLevelString(level));
    AtJsonAddNull(structured, "driver_image_path");
    AtJsonAddNull(structured, "driver_service_name");
    AtJsonAddNull(structured, "driver_size");

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

//
// Pagefile information
//

VOID AtpGetPagefileInfo(
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 0x200;
    ULONG returnLength = 0;
    PSYSTEM_PAGEFILE_INFORMATION pagefile;
    ULONG pageSize;
    PVOID structured;
    PVOID rows;
    ULONG count = 0;

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
    rows = PhCreateJsonArray();

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
            PhAddJsonArrayObject(rows, row);
            count++;

            if (pagefile->NextEntryOffset == 0)
                break;

            pagefile = PTR_ADD_OFFSET(pagefile, pagefile->NextEntryOffset);
        }
    }

    PhFree(buffer);

    PhAddJsonObjectValue(structured, "pagefiles", rows);
    PhAddJsonObjectUInt64(structured, "count", count);
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
        AtpGetPagefileInfo(Result);
        break;
    case AtActionListStartupEntries:
        AtListStartupEntries(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
