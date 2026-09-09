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

PCWSTR AtpElevationTypeString(
    _In_ TOKEN_ELEVATION_TYPE Type
    )
{
    switch (Type)
    {
    case TokenElevationTypeDefault:
        return L"Default";
    case TokenElevationTypeFull:
        return L"Full";
    case TokenElevationTypeLimited:
        return L"Limited";
    }

    return NULL;
}

PCWSTR AtpProtectionString(
    _In_ PS_PROTECTION Protection
    )
{
    if (Protection.Type == PsProtectedTypeNone)
        return NULL;

    switch (Protection.Signer)
    {
    case PsProtectedSignerAuthenticode:
        return Protection.Type == PsProtectedTypeProtected ? L"Authenticode" : L"Authenticode (Light)";
    case PsProtectedSignerCodeGen:
        return Protection.Type == PsProtectedTypeProtected ? L"CodeGen" : L"CodeGen (Light)";
    case PsProtectedSignerAntimalware:
        return Protection.Type == PsProtectedTypeProtected ? L"Antimalware" : L"Antimalware (Light)";
    case PsProtectedSignerLsa:
        return Protection.Type == PsProtectedTypeProtected ? L"Lsa" : L"Lsa (Light)";
    case PsProtectedSignerWindows:
        return Protection.Type == PsProtectedTypeProtected ? L"Windows" : L"Windows (Light)";
    case PsProtectedSignerWinTcb:
        return Protection.Type == PsProtectedTypeProtected ? L"WinTcb" : L"WinTcb (Light)";
    case PsProtectedSignerWinSystem:
        return Protection.Type == PsProtectedTypeProtected ? L"WinSystem" : L"WinSystem (Light)";
    case PsProtectedSignerApp:
        return Protection.Type == PsProtectedTypeProtected ? L"App" : L"App (Light)";
    }

    return Protection.Type == PsProtectedTypeProtected ? L"Protected" : L"Protected (Light)";
}

PCWSTR AtpNativeArchitectureString(
    VOID
    )
{
#if defined(_M_ARM64)
    return L"ARM64";
#elif defined(_M_X64)
    return L"x64";
#else
    return L"x86";
#endif
}

VOID AtpAddParentPid(
    _In_ PVOID Object,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    if (ProcessItem->ParentProcessId)
        PhAddJsonObjectUInt64(Object, "parent_pid", HandleToUlong(ProcessItem->ParentProcessId));
    else
        AtJsonAddNull(Object, "parent_pid");
}

PVOID AtpCreateProcessRow(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ BOOLEAN VerifySignatures
    )
{
    PVOID row;

    row = PhCreateJsonObject();
    PhAddJsonObjectUInt64(row, "pid", HandleToUlong(ProcessItem->ProcessId));
    PhAddJsonObjectUInt64(row, "process_sequence_number", ProcessItem->ProcessSequenceNumber);
    AtpAddParentPid(row, ProcessItem);
    AtJsonAddString(row, "name", ProcessItem->ProcessName);
    AtJsonAddString(row, "user", ProcessItem->UserName);
    PhAddJsonObjectUInt64(row, "session_id", ProcessItem->SessionId);
    AtJsonAddTime(row, "start_time", &ProcessItem->CreateTime);
    PhAddJsonObjectDouble(row, "cpu_usage", ProcessItem->CpuUsage);
    PhAddJsonObjectUInt64(row, "private_bytes", ProcessItem->VmCounters.PagefileUsage);
    PhAddJsonObjectUInt64(row, "working_set_bytes", ProcessItem->VmCounters.WorkingSetSize);
    PhAddJsonObjectUInt64(row, "thread_count", ProcessItem->NumberOfThreads);
    PhAddJsonObjectUInt64(row, "handle_count", ProcessItem->NumberOfHandles);
    PhAddJsonObjectBoolean(row, "is_suspended", !!ProcessItem->IsSuspended);

    // A full signature verification per process, so only when asked for.
    if (VerifySignatures)
        AtJsonAddMicrosoftSigned(row, "is_microsoft_signed", ProcessItem->FileName);

    return row;
}

PCWSTR AtpKnownProcessTypeString(
    _In_ PH_KNOWN_PROCESS_TYPE Type
    )
{
    switch (Type & KnownProcessTypeMask)
    {
    case SystemProcessType:
        return L"system";
    case SessionManagerProcessType:
        return L"session_manager";
    case WindowsSubsystemProcessType:
        return L"windows_subsystem";
    case WindowsStartupProcessType:
        return L"windows_startup";
    case ServiceControlManagerProcessType:
        return L"service_control_manager";
    case LocalSecurityAuthorityProcessType:
        return L"local_security_authority";
    case LocalSessionManagerProcessType:
        return L"local_session_manager";
    case WindowsLogonProcessType:
        return L"windows_logon";
    case ServiceHostProcessType:
        return L"service_host";
    case RunDllAsAppProcessType:
        return L"rundll_as_app";
    case ComSurrogateProcessType:
        return L"com_surrogate";
    case TaskHostProcessType:
        return L"task_host";
    case ExplorerProcessType:
        return L"explorer";
    case UmdfHostProcessType:
        return L"umdf_host";
    case NtVdmHostProcessType:
        return L"ntvdm_host";
    case WmiProviderHostType:
        return L"wmi_provider_host";
    }

    return NULL;
}

VOID AtpAddKnownCommandLine(
    _In_ PVOID Object,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PH_KNOWN_PROCESS_COMMAND_LINE knownCommandLine;
    PVOID entry;

    if (!ProcessItem->CommandLine ||
        (ProcessItem->KnownProcessType & KnownProcessTypeMask) == UnknownProcessType ||
        !PhaGetProcessKnownCommandLine(ProcessItem->CommandLine, ProcessItem->KnownProcessType, &knownCommandLine))
    {
        AtJsonAddNull(Object, "known_command_line");
        return;
    }

    entry = PhCreateJsonObject();

    switch (ProcessItem->KnownProcessType & KnownProcessTypeMask)
    {
    case ServiceHostProcessType:
        AtJsonAddString(entry, "service_group", knownCommandLine.ServiceHost.GroupName);
        break;
    case RunDllAsAppProcessType:
        AtJsonAddWin32FileName(entry, "target_file", knownCommandLine.RunDllAsApp.FileName);
        AtJsonAddString(entry, "target_procedure", knownCommandLine.RunDllAsApp.ProcedureName);
        break;
    case ComSurrogateProcessType:
        {
            PPH_STRING guid = PhFormatGuid(&knownCommandLine.ComSurrogate.Guid);

            AtJsonAddString(entry, "com_clsid", guid);
            AtJsonAddString(entry, "com_name", knownCommandLine.ComSurrogate.Name);
            AtJsonAddWin32FileName(entry, "com_file", knownCommandLine.ComSurrogate.FileName);
            PhClearReference(&guid);
        }
        break;
    default:
        PhFreeJsonObject(entry);
        AtJsonAddNull(Object, "known_command_line");
        return;
    }

    PhAddJsonObjectValue(Object, "known_command_line", entry);
}

VOID AtpAddParent(
    _In_ PVOID Object,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PPH_PROCESS_RECORD record;
    PPH_PROCESS_ITEM parent;
    PVOID entry;

    if (!ProcessItem->ParentProcessId)
    {
        AtJsonAddNull(Object, "parent");
        return;
    }

    if (record = PhFindProcessRecord(ProcessItem->ParentProcessId, &ProcessItem->CreateTime))
    {
        entry = PhCreateJsonObject();
        PhAddJsonObjectUInt64(entry, "pid", HandleToUlong(record->ProcessId));
        PhAddJsonObjectUInt64(entry, "process_sequence_number", record->ProcessSequenceNumber);
        AtJsonAddString(entry, "name", record->ProcessName);
        AtJsonAddWin32FileName(entry, "image_path", record->FileName);
        AtJsonAddString(entry, "command_line", record->CommandLine);
        AtJsonAddTime(entry, "start_time", &record->CreateTime);
        PhAddJsonObjectBoolean(entry, "still_running", !FlagOn(record->Flags, PH_PROCESS_RECORD_DEAD));

        PhAddJsonObjectValue(Object, "parent", entry);
        PhDereferenceProcessRecord(record);
        return;
    }

    // A parent that started after its child is a recycled pid, not the parent.
    if (!(parent = PhReferenceProcessItem(ProcessItem->ParentProcessId)))
    {
        AtJsonAddNull(Object, "parent");
        return;
    }

    if (parent->CreateTime.QuadPart > ProcessItem->CreateTime.QuadPart)
    {
        AtJsonAddNull(Object, "parent");
        PhDereferenceObject(parent);
        return;
    }

    entry = PhCreateJsonObject();
    PhAddJsonObjectUInt64(entry, "pid", HandleToUlong(parent->ProcessId));
    PhAddJsonObjectUInt64(entry, "process_sequence_number", parent->ProcessSequenceNumber);
    AtJsonAddString(entry, "name", parent->ProcessName);
    AtJsonAddWin32FileName(entry, "image_path", parent->FileName);
    AtJsonAddString(entry, "command_line", parent->CommandLine);
    AtJsonAddTime(entry, "start_time", &parent->CreateTime);
    PhAddJsonObjectBoolean(entry, "still_running", TRUE);

    PhAddJsonObjectValue(Object, "parent", entry);
    PhDereferenceObject(parent);
}

VOID AtpAddProcessStatistics(
    _In_ PVOID Object,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PVOID statistics;
    PVOID entry;
    HANDLE processHandle = NULL;
    PH_PROCESS_WS_COUNTERS wsCounters;
    IO_PRIORITY_HINT ioPriority;
    ULONG pagePriority;
    ULONG depStatus;
    LARGE_INTEGER now;

    statistics = PhCreateJsonObject();

    if (PH_IS_REAL_PROCESS_ID(ProcessItem->ProcessId))
    {
        PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION,
            ProcessItem->ProcessId
            );
    }

    // Only the working set breakdown needs the process.
    entry = PhCreateJsonObject();
    PhAddJsonObjectUInt64(entry, "paged_pool_bytes", ProcessItem->VmCounters.QuotaPagedPoolUsage);
    PhAddJsonObjectUInt64(entry, "peak_paged_pool_bytes", ProcessItem->VmCounters.QuotaPeakPagedPoolUsage);
    PhAddJsonObjectUInt64(entry, "non_paged_pool_bytes", ProcessItem->VmCounters.QuotaNonPagedPoolUsage);
    PhAddJsonObjectUInt64(entry, "peak_non_paged_pool_bytes", ProcessItem->VmCounters.QuotaPeakNonPagedPoolUsage);
    PhAddJsonObjectUInt64(entry, "page_file_bytes", ProcessItem->VmCounters.PagefileUsage);
    PhAddJsonObjectUInt64(entry, "peak_page_file_bytes", ProcessItem->VmCounters.PeakPagefileUsage);
    PhAddJsonObjectValue(statistics, "quota", entry);

    if (processHandle && NT_SUCCESS(PhGetProcessWsCounters(processHandle, &wsCounters)))
    {
        entry = PhCreateJsonObject();
        PhAddJsonObjectUInt64(entry, "total_bytes", (ULONG64)wsCounters.NumberOfPages * PAGE_SIZE);
        PhAddJsonObjectUInt64(entry, "private_bytes", (ULONG64)wsCounters.NumberOfPrivatePages * PAGE_SIZE);
        PhAddJsonObjectUInt64(entry, "shared_bytes", (ULONG64)wsCounters.NumberOfSharedPages * PAGE_SIZE);
        PhAddJsonObjectUInt64(entry, "shareable_bytes", (ULONG64)wsCounters.NumberOfShareablePages * PAGE_SIZE);
        PhAddJsonObjectValue(statistics, "working_set", entry);
    }
    else
    {
        AtJsonAddNull(statistics, "working_set");
    }

    // GDI and USER handles, which is how a leaking UI process is recognised.
    if (processHandle)
    {
        entry = PhCreateJsonObject();
        PhAddJsonObjectUInt64(entry, "gdi_handles", GetGuiResources(processHandle, GR_GDIOBJECTS));
        PhAddJsonObjectUInt64(entry, "gdi_handles_peak", GetGuiResources(processHandle, GR_GDIOBJECTS_PEAK));
        PhAddJsonObjectUInt64(entry, "user_handles", GetGuiResources(processHandle, GR_USEROBJECTS));
        PhAddJsonObjectUInt64(entry, "user_handles_peak", GetGuiResources(processHandle, GR_USEROBJECTS_PEAK));
        PhAddJsonObjectValue(statistics, "gui_resources", entry);
    }
    else
    {
        AtJsonAddNull(statistics, "gui_resources");
    }

    if (processHandle && NT_SUCCESS(PhGetProcessPagePriority(processHandle, &pagePriority)))
        PhAddJsonObjectUInt64(statistics, "page_priority", pagePriority);
    else
        AtJsonAddNull(statistics, "page_priority");

    if (processHandle && NT_SUCCESS(PhGetProcessIoPriority(processHandle, &ioPriority)))
        AtJsonAddStringZ(statistics, "io_priority", AtIoPriorityString(ioPriority));
    else
        AtJsonAddNull(statistics, "io_priority");

    if (processHandle && NT_SUCCESS(PhGetProcessDepStatus(processHandle, &depStatus)))
    {
        entry = PhCreateJsonObject();
        PhAddJsonObjectBoolean(entry, "enabled", !!FlagOn(depStatus, PH_PROCESS_DEP_ENABLED));
        PhAddJsonObjectBoolean(entry, "permanent", !!FlagOn(depStatus, PH_PROCESS_DEP_PERMANENT));
        PhAddJsonObjectBoolean(entry, "atl_thunk_emulation_disabled", !!FlagOn(depStatus, PH_PROCESS_DEP_ATL_THUNK_EMULATION_DISABLED));
        PhAddJsonObjectValue(statistics, "dep", entry);
    }
    else
    {
        AtJsonAddNull(statistics, "dep");
    }

    // The running total the provider keeps alongside the per-run delta.
    PhAddJsonObjectUInt64(statistics, "cycle_time", ProcessItem->CycleTimeDelta.Value);
    PhAddJsonObjectUInt64(statistics, "page_faults", ProcessItem->VmCounters.PageFaultCount);
    PhAddJsonObjectUInt64(statistics, "peak_virtual_size", ProcessItem->VmCounters.PeakVirtualSize);

    PhQuerySystemTime(&now);

    if (ProcessItem->CreateTime.QuadPart && now.QuadPart > ProcessItem->CreateTime.QuadPart)
        AtJsonAddDuration(statistics, "uptime_seconds", now.QuadPart - ProcessItem->CreateTime.QuadPart);
    else
        AtJsonAddNull(statistics, "uptime_seconds");

    if (processHandle)
        NtClose(processHandle);

    PhAddJsonObjectValue(Object, "statistics", statistics);
}

VOID AtpFillProcessDetail(
    _In_ PVOID Object,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    ULONG interval = AtGetUpdateInterval();
    BOOLEAN stage2 = !!PhGetIntegerSetting(L"EnableStage2");

    HANDLE processHandle;
    PVOID services;

    PhAddJsonObjectUInt64(Object, "pid", HandleToUlong(ProcessItem->ProcessId));
    PhAddJsonObjectUInt64(Object, "process_sequence_number", ProcessItem->ProcessSequenceNumber);

    if (ProcessItem->ProcessStartKey)
    {
        PH_FORMAT format[1];
        PPH_STRING startKey;

        PhInitFormatI64U(&format[0], ProcessItem->ProcessStartKey);
        startKey = PhFormat(format, RTL_NUMBER_OF(format), 24);
        AtJsonAddString(Object, "process_start_key", startKey);
        PhDereferenceObject(startKey);
    }
    else
    {
        AtJsonAddNull(Object, "process_start_key");
    }

    AtpAddParentPid(Object, ProcessItem);
    AtJsonAddString(Object, "name", ProcessItem->ProcessName);
    AtJsonAddWin32FileName(Object, "image_path", ProcessItem->FileName);
    AtJsonAddString(Object, "image_path_native", ProcessItem->FileName);
    AtJsonAddString(Object, "command_line", ProcessItem->CommandLine);

    // The current directory lives in the PEB; read it when the process lets us.
    if (PH_IS_REAL_PROCESS_ID(ProcessItem->ProcessId) &&
        NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, ProcessItem->ProcessId)))
    {
        PPH_STRING currentDirectory = NULL;

        if (NT_SUCCESS(PhGetProcessPebString(processHandle, PhpoCurrentDirectory, &currentDirectory)))
        {
            AtJsonAddString(Object, "current_directory", currentDirectory);
            PhDereferenceObject(currentDirectory);
        }
        else
        {
            AtJsonAddNull(Object, "current_directory");
        }

        NtClose(processHandle);
    }
    else
    {
        AtJsonAddNull(Object, "current_directory");
    }

    AtJsonAddString(Object, "user", ProcessItem->UserName);
    PhAddJsonObjectUInt64(Object, "session_id", ProcessItem->SessionId);
    AtJsonAddTime(Object, "start_time", &ProcessItem->CreateTime);
    AtJsonAddDuration(Object, "kernel_time", ProcessItem->KernelTime.QuadPart);
    AtJsonAddDuration(Object, "user_time", ProcessItem->UserTime.QuadPart);
    AtJsonAddStringRef(Object, "integrity_level", ProcessItem->IntegrityString);
    AtJsonAddStringZ(Object, "elevation_type", AtpElevationTypeString(ProcessItem->ElevationType));
    PhAddJsonObjectBoolean(Object, "is_elevated", !!ProcessItem->IsElevated);
    AtJsonAddStringZ(Object, "verify_result", AtVerifyResultString(ProcessItem->VerifyResult));
    AtJsonAddString(Object, "verify_signer", ProcessItem->VerifySignerName);
    // Verified here: the provider only fills verify_result when the signature stage is enabled.
    AtJsonAddMicrosoftSigned(Object, "is_microsoft_signed", ProcessItem->FileName);
    AtJsonAddString(Object, "package_full_name", ProcessItem->PackageFullName);
    PhAddJsonObjectBoolean(Object, "is_protected_process", !!ProcessItem->IsProtectedProcess);
    AtJsonAddStringZ(Object, "protection", AtpProtectionString(ProcessItem->Protection));
    PhAddJsonObjectBoolean(Object, "is_secure_process", !!ProcessItem->IsSecureProcess);
    PhAddJsonObjectBoolean(Object, "is_wow64", !!ProcessItem->IsWow64Process);
    AtJsonAddStringZ(Object, "architecture", ProcessItem->IsWow64Process ? L"x86" : AtpNativeArchitectureString());
    PhAddJsonObjectBoolean(Object, "is_suspended", !!ProcessItem->IsSuspended);
    PhAddJsonObjectBoolean(Object, "is_being_debugged", !!ProcessItem->IsBeingDebugged);
    PhAddJsonObjectBoolean(Object, "is_in_job", !!ProcessItem->IsInJob);
    PhAddJsonObjectBoolean(Object, "is_immersive", !!ProcessItem->IsImmersive);
    PhAddJsonObjectBoolean(Object, "is_packaged", !!ProcessItem->IsPackagedProcess);
    PhAddJsonObjectBoolean(Object, "is_dotnet", !!ProcessItem->IsDotNet);
    PhAddJsonObjectBoolean(Object, "is_subsystem_process", !!ProcessItem->IsSubsystemProcess);
    PhAddJsonObjectBoolean(Object, "is_ui_access", !!ProcessItem->IsUIAccessEnabled);
    PhAddJsonObjectBoolean(Object, "is_frozen", !!ProcessItem->IsFrozenProcess);
    PhAddJsonObjectBoolean(Object, "is_background", !!ProcessItem->IsBackgroundProcess);
    PhAddJsonObjectBoolean(Object, "is_cross_session", !!ProcessItem->IsCrossSessionProcess);
    PhAddJsonObjectBoolean(Object, "is_power_throttling", !!ProcessItem->IsPowerThrottling);
    PhAddJsonObjectBoolean(Object, "is_system_process", !!ProcessItem->IsSystemProcess);
    PhAddJsonObjectBoolean(Object, "is_secure_system", !!ProcessItem->IsSecureSystem);
    PhAddJsonObjectBoolean(Object, "is_partially_suspended", !!ProcessItem->IsPartiallySuspended);
    PhAddJsonObjectBoolean(Object, "is_in_significant_job", !!ProcessItem->IsInSignificantJob);
    PhAddJsonObjectBoolean(Object, "is_snapshot", !!ProcessItem->IsSnapshotProcess);
    if (stage2)
        PhAddJsonObjectBoolean(Object, "is_packed", !!ProcessItem->IsPacked);
    else
        AtJsonAddNull(Object, "is_packed");

    {
        PVOID version = PhCreateJsonObject();

        AtJsonAddString(version, "company", ProcessItem->VersionInfo.CompanyName);
        AtJsonAddString(version, "description", ProcessItem->VersionInfo.FileDescription);
        AtJsonAddString(version, "file_version", ProcessItem->VersionInfo.FileVersion);
        AtJsonAddString(version, "product", ProcessItem->VersionInfo.ProductName);
        PhAddJsonObjectValue(Object, "version_info", version);
    }

    AtJsonAddStringZ(Object, "known_type", AtpKnownProcessTypeString(ProcessItem->KnownProcessType));
    AtpAddKnownCommandLine(Object, ProcessItem);
    AtpAddParent(Object, ProcessItem);

    // ULONG_MAX is the provider's "could not read the image" sentinel; null rather than a zero that
    // reads as "imports nothing".
    if (stage2 && ProcessItem->ImportFunctions != ULONG_MAX)
        PhAddJsonObjectUInt64(Object, "import_functions", ProcessItem->ImportFunctions);
    else
        AtJsonAddNull(Object, "import_functions");

    if (stage2 && ProcessItem->ImportModules != ULONG_MAX)
        PhAddJsonObjectUInt64(Object, "import_modules", ProcessItem->ImportModules);
    else
        AtJsonAddNull(Object, "import_modules");
    AtJsonAddHex(Object, "image_checksum", ProcessItem->ImageChecksum);

    if (ProcessItem->ImageTimeStamp)
    {
        LARGE_INTEGER timeStamp;

        PhSecondsSince1970ToTime(ProcessItem->ImageTimeStamp, &timeStamp);
        AtJsonAddTime(Object, "image_timestamp", &timeStamp);
    }
    else
    {
        AtJsonAddNull(Object, "image_timestamp");
    }

    // Null unless really measured: at scan level zero the status is successful and the value zero,
    // which would report every process as incoherent.
    if (PhGetIntegerSetting(L"EnableImageCoherencySupport") &&
        PhGetIntegerSetting(L"ImageCoherencyScanLevel") != 0 &&
        NT_SUCCESS(ProcessItem->ImageCoherencyStatus))
    {
        PhAddJsonObjectDouble(Object, "image_coherency", ProcessItem->ImageCoherency);
    }
    else
    {
        AtJsonAddNull(Object, "image_coherency");
    }

    // A path-shaped name that no longer resolves: the process is running from a file that is gone.
    if (ProcessItem->FileName &&
        ProcessItem->FileName->Length >= sizeof(WCHAR) &&
        ProcessItem->FileName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
    {
        PhAddJsonObjectBoolean(Object, "image_file_exists", !!PhDoesFileExist(&ProcessItem->FileName->sr));
    }
    else
    {
        AtJsonAddNull(Object, "image_file_exists");
    }

    {
        PVOID disk = PhCreateJsonObject();
        PVOID network = PhCreateJsonObject();

        PhAddJsonObjectUInt64(disk, "read_bytes", ProcessItem->DiskCounters.BytesRead);
        PhAddJsonObjectUInt64(disk, "write_bytes", ProcessItem->DiskCounters.BytesWritten);
        PhAddJsonObjectUInt64(disk, "read_operations", ProcessItem->DiskCounters.ReadOperationCount);
        PhAddJsonObjectUInt64(disk, "write_operations", ProcessItem->DiskCounters.WriteOperationCount);
        PhAddJsonObjectUInt64(disk, "flush_operations", ProcessItem->DiskCounters.FlushOperationCount);
        PhAddJsonObjectValue(Object, "disk_counters", disk);

        // PROCESS_NETWORK_COUNTERS carries byte totals only; there are no packet counts to report.
        PhAddJsonObjectUInt64(network, "bytes_in", ProcessItem->NetworkCounters.BytesIn);
        PhAddJsonObjectUInt64(network, "bytes_out", ProcessItem->NetworkCounters.BytesOut);
        PhAddJsonObjectValue(Object, "network_counters", network);
    }

    PhAddJsonObjectUInt64(Object, "shared_commit_bytes", ProcessItem->SharedCommitCharge);
    PhAddJsonObjectUInt64(Object, "working_set_private_bytes", ProcessItem->WorkingSetPrivateSize);
    PhAddJsonObjectUInt64(Object, "peak_thread_count", ProcessItem->PeakNumberOfThreads);
    PhAddJsonObjectUInt64(Object, "hard_fault_count", ProcessItem->HardFaultCount);
    PhAddJsonObjectUInt64(Object, "context_switches", ProcessItem->ContextSwitches);
    PhAddJsonObjectUInt64(Object, "job_object_id", ProcessItem->JobObjectId);

    if (ProcessItem->LxssProcessId)
        PhAddJsonObjectUInt64(Object, "lxss_pid", ProcessItem->LxssProcessId);
    else
        AtJsonAddNull(Object, "lxss_pid");

    if (ProcessItem->Sid)
    {
        PPH_STRING sid = PhSidToStringSid(ProcessItem->Sid);

        AtJsonAddString(Object, "sid", sid);
        PhClearReference(&sid);
    }
    else
    {
        AtJsonAddNull(Object, "sid");
    }

    if (ProcessItem->ConsoleHostProcessId)
        PhAddJsonObjectUInt64(Object, "console_host_pid", HandleToUlong(ProcessItem->ConsoleHostProcessId));
    else
        AtJsonAddNull(Object, "console_host_pid");

    AtJsonAddStringZ(Object, "priority_class", AtPriorityClassString(ProcessItem->PriorityClass));
    PhAddJsonObjectInt64(Object, "base_priority", ProcessItem->BasePriority);
    PhAddJsonObjectDouble(Object, "cpu_usage", ProcessItem->CpuUsage);
    PhAddJsonObjectDouble(Object, "cpu_kernel_usage", ProcessItem->CpuKernelUsage);
    PhAddJsonObjectDouble(Object, "cpu_user_usage", ProcessItem->CpuUserUsage);
    PhAddJsonObjectUInt64(Object, "update_interval_ms", interval);

    // What changed in the last provider run.
    AtAddRate(Object, "io_read_rate", ProcessItem->IoReadDelta.Delta, interval);
    AtAddRate(Object, "io_write_rate", ProcessItem->IoWriteDelta.Delta, interval);
    AtAddRate(Object, "io_other_rate", ProcessItem->IoOtherDelta.Delta, interval);
    PhAddJsonObjectUInt64(Object, "io_read_delta", ProcessItem->IoReadDelta.Delta);
    PhAddJsonObjectUInt64(Object, "io_write_delta", ProcessItem->IoWriteDelta.Delta);
    PhAddJsonObjectUInt64(Object, "io_other_delta", ProcessItem->IoOtherDelta.Delta);
    PhAddJsonObjectUInt64(Object, "context_switches_delta", ProcessItem->ContextSwitchesDelta.Delta);
    PhAddJsonObjectUInt64(Object, "page_faults_delta", ProcessItem->PageFaultsDelta.Delta);
    PhAddJsonObjectUInt64(Object, "hard_faults_delta", ProcessItem->HardFaultsDelta.Delta);
    PhAddJsonObjectUInt64(Object, "cycle_time_delta", ProcessItem->CycleTimeDelta.Delta);

    // The delta is unsigned and wraps; read back as signed so a fall is not several exabytes.
    PhAddJsonObjectInt64(Object, "private_bytes_delta", (LONG64)(LONG_PTR)ProcessItem->PrivateBytesDelta.Delta);
    PhAddJsonObjectUInt64(Object, "private_bytes", ProcessItem->VmCounters.PagefileUsage);
    PhAddJsonObjectUInt64(Object, "peak_private_bytes", ProcessItem->VmCounters.PeakPagefileUsage);
    PhAddJsonObjectUInt64(Object, "working_set_bytes", ProcessItem->VmCounters.WorkingSetSize);
    PhAddJsonObjectUInt64(Object, "peak_working_set_bytes", ProcessItem->VmCounters.PeakWorkingSetSize);
    PhAddJsonObjectUInt64(Object, "virtual_size", ProcessItem->VmCounters.VirtualSize);
    PhAddJsonObjectUInt64(Object, "page_faults", ProcessItem->VmCounters.PageFaultCount);
    PhAddJsonObjectUInt64(Object, "io_read_bytes", ProcessItem->IoCounters.ReadTransferCount);
    PhAddJsonObjectUInt64(Object, "io_write_bytes", ProcessItem->IoCounters.WriteTransferCount);
    PhAddJsonObjectUInt64(Object, "io_other_bytes", ProcessItem->IoCounters.OtherTransferCount);
    PhAddJsonObjectUInt64(Object, "thread_count", ProcessItem->NumberOfThreads);
    PhAddJsonObjectUInt64(Object, "handle_count", ProcessItem->NumberOfHandles);

    services = PhCreateJsonArray();

    PhAcquireQueuedLockShared(&ProcessItem->ServiceListLock);

    if (ProcessItem->ServiceList)
    {
        ULONG enumerationKey = 0;
        PPH_SERVICE_ITEM serviceItem;

        while (PhEnumPointerList(ProcessItem->ServiceList, &enumerationKey, &serviceItem))
        {
            PPH_BYTES utf8;

            if (serviceItem->Name && (utf8 = PhConvertUtf16ToUtf8Ex(serviceItem->Name->Buffer, serviceItem->Name->Length)))
            {
                PhAddJsonArrayObject(services, PhCreateJsonStringObject(utf8->Buffer));
                PhDereferenceObject(utf8);
            }
        }
    }

    PhReleaseQueuedLockShared(&ProcessItem->ServiceListLock);

    PhAddJsonObjectValue(Object, "services", services);
    PhAddJsonObjectBoolean(Object, "access_denied", !ProcessItem->IsHandleValid);
}

typedef struct _AT_LIST_FILTER
{
    LARGE_INTEGER StartedAfter;
    BOOLEAN HaveStartedAfter;
    BOOLEAN ProtectedOnly;
    BOOLEAN ImageMissingOnly;
    PPH_STRING NameContains;
    PPH_STRING UserContains;
    PVOID Pids;
    BOOLEAN HaveParentPid;
    HANDLE ParentPid;
    BOOLEAN IncludeTree;
    BOOLEAN ExcludeMicrosoft;
    BOOLEAN UnsignedOnly;
} AT_LIST_FILTER, *PAT_LIST_FILTER;

BOOLEAN AtpMatchesFilter(
    _In_ PAT_LIST_FILTER Filter,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    if (!AtContainsString(ProcessItem->ProcessName, Filter->NameContains))
        return FALSE;

    if (!AtContainsString(ProcessItem->UserName, Filter->UserContains))
        return FALSE;

    if (Filter->HaveParentPid && ProcessItem->ParentProcessId != Filter->ParentPid)
        return FALSE;

    // Compared at the resolution the time was written in, so the named process itself is excluded.
    if (Filter->HaveStartedAfter &&
        ProcessItem->CreateTime.QuadPart < Filter->StartedAfter.QuadPart + PH_TICKS_PER_MS)
    {
        return FALSE;
    }

    if (Filter->ProtectedOnly && !ProcessItem->IsProtectedProcess)
        return FALSE;

    // A pseudo process such as Registry has a name where the path would be and no image, which is
    // not a missing image.
    if (Filter->ImageMissingOnly)
    {
        if (!ProcessItem->FileName ||
            ProcessItem->FileName->Length < sizeof(WCHAR) ||
            ProcessItem->FileName->Buffer[0] != OBJ_NAME_PATH_SEPARATOR ||
            PhDoesFileExist(&ProcessItem->FileName->sr))
        {
            return FALSE;
        }
    }

    // One signature verification per row, so every cheap filter runs first.
    if (Filter->ExcludeMicrosoft && AtIsMicrosoftSigned(ProcessItem->FileName, NULL))
        return FALSE;

    if (Filter->UnsignedOnly && AtVerifyFileName(ProcessItem->FileName, NULL) == VrTrusted)
        return FALSE;

    if (Filter->Pids)
    {
        ULONG count = PhGetJsonArrayLength(Filter->Pids);
        ULONG i;
        BOOLEAN found = FALSE;

        for (i = 0; i < count; i++)
        {
            if ((ULONG64)PhGetJsonArrayLong64(Filter->Pids, i) == HandleToUlong(ProcessItem->ProcessId))
            {
                found = TRUE;
                break;
            }
        }

        if (!found)
            return FALSE;
    }

    return TRUE;
}

VOID AtpListProcesses(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    AT_LIST_FILTER filter;
    AT_ROWS rows;
    PPH_PROCESS_ITEM* processItems;
    ULONG numberOfProcessItems;
    PBOOLEAN matched;
    PVOID structured;
    ULONG i;
    ULONG64 parentPid;
    ULONG64 sinceSnapshotId;
    ULONG64 startedWithin;
    PPH_STRING startedAfter;
    BOOLEAN verifySignatures;

    memset(&filter, 0, sizeof(AT_LIST_FILTER));
    verifySignatures = AtJsonGetObjectBoolean(Call->Arguments, "verify_signatures");

    if (Call->Arguments)
    {
        filter.NameContains = AtGetArgumentString(Call->Arguments, "name_contains");
        filter.UserContains = AtGetArgumentString(Call->Arguments, "user_contains");
        filter.Pids = AtJsonGetObjectMember(Call->Arguments, "pids", PH_JSON_OBJECT_TYPE_ARRAY);
        filter.IncludeTree = AtJsonGetObjectBoolean(Call->Arguments, "include_tree");
        filter.ProtectedOnly = AtJsonGetObjectBoolean(Call->Arguments, "protected_only");
        filter.ImageMissingOnly = AtJsonGetObjectBoolean(Call->Arguments, "image_missing_only");
        filter.ExcludeMicrosoft = AtJsonGetObjectBoolean(Call->Arguments, "exclude_microsoft");
        filter.UnsignedOnly = AtJsonGetObjectBoolean(Call->Arguments, "unsigned_only");

        if (startedAfter = AtGetArgumentString(Call->Arguments, "started_after"))
        {
            if (!AtParseTime(startedAfter, &filter.StartedAfter))
            {
                AtSetToolError(
                    Result,
                    "invalid_arguments",
                    STATUS_INVALID_PARAMETER,
                    L"started_after must be an ISO 8601 time such as 2026-09-08T01:02:03Z, as returned in start_time."
                    );
                PhDereferenceObject(startedAfter);
                PhClearReference(&filter.NameContains);
                PhClearReference(&filter.UserContains);
                return;
            }

            filter.HaveStartedAfter = TRUE;
            PhDereferenceObject(startedAfter);
        }

        // A window rather than an instant, which is what "started recently" usually means.
        if (AtGetArgumentUInt64(Call->Arguments, "started_within_seconds", &startedWithin) && startedWithin)
        {
            LARGE_INTEGER now;

            PhQuerySystemTime(&now);
            now.QuadPart -= (LONG64)min(startedWithin, MAXLONG64 / PH_TICKS_PER_SEC) * PH_TICKS_PER_SEC;

            if (!filter.HaveStartedAfter || now.QuadPart > filter.StartedAfter.QuadPart)
            {
                filter.StartedAfter = now;
                filter.HaveStartedAfter = TRUE;
            }
        }

        if (AtGetArgumentUInt64(Call->Arguments, "parent_pid", &parentPid) && parentPid <= MAXULONG)
        {
            filter.HaveParentPid = TRUE;
            filter.ParentPid = UlongToHandle((ULONG)parentPid);
        }
    }

    PhEnumProcessItems(&processItems, &numberOfProcessItems);
    matched = PhAllocateZero(numberOfProcessItems * sizeof(BOOLEAN));

    for (i = 0; i < numberOfProcessItems; i++)
        matched[i] = AtpMatchesFilter(&filter, processItems[i]);

    if (filter.IncludeTree)
    {
        BOOLEAN changed;

        // Descendants, matched on a parent created before the child so a recycled parent pid does
        // not adopt it.
        do
        {
            changed = FALSE;

            for (i = 0; i < numberOfProcessItems; i++)
            {
                ULONG j;

                if (matched[i])
                    continue;

                for (j = 0; j < numberOfProcessItems; j++)
                {
                    if (matched[j] &&
                        processItems[j]->ProcessId == processItems[i]->ParentProcessId &&
                        processItems[j]->CreateTime.QuadPart <= processItems[i]->CreateTime.QuadPart)
                    {
                        matched[i] = TRUE;
                        changed = TRUE;
                        break;
                    }
                }
            }
        } while (changed);
    }

    structured = PhCreateJsonObject();
    AtInitializeRows(&rows, Call->Arguments);

    for (i = 0; i < numberOfProcessItems; i++)
    {
        if (!matched[i])
            continue;

        AtAddRow(&rows, AtpCreateProcessRow(processItems[i], verifySignatures));
    }

    AtAddRows(structured, "processes", &rows);

    if (AtGetArgumentUInt64(Call->Arguments, "since_snapshot_id", &sinceSnapshotId))
        AtAddProcessChanges(structured, (ULONG)min(sinceSnapshotId, MAXULONG));

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    for (i = 0; i < numberOfProcessItems; i++)
        PhDereferenceObject(processItems[i]);

    PhFree(processItems);
    PhFree(matched);
    PhClearReference(&filter.NameContains);
    PhClearReference(&filter.UserContains);
}

VOID AtpGetProcess(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    BOOLEAN includeStatistics = AtJsonGetObjectBoolean(Call->Arguments, "include_statistics");
    AT_BATCH batch;
    AT_TARGET target;
    PVOID structured;

    if (!AtInitializeBatch(&batch, Call->Arguments, Result))
        return;

    if (batch.Pids)
    {
        PVOID results = PhCreateJsonArray();
        ULONG i;

        for (i = 0; i < batch.Count; i++)
        {
            PPH_PROCESS_ITEM processItem;
            ULONG processId;
            PVOID entry;

            if (!(processItem = AtBatchReferenceProcessItem(&batch, i, &processId)))
            {
                PhAddJsonArrayObject(results, AtCreateBatchError(
                    processId,
                    "not_found",
                    L"No process with this pid is in the provider cache."
                    ));
                continue;
            }

            if (batch.Summary)
            {
                entry = AtpCreateProcessRow(processItem, FALSE);
            }
            else
            {
                entry = PhCreateJsonObject();
                AtpFillProcessDetail(entry, processItem);

                if (includeStatistics)
                    AtpAddProcessStatistics(entry, processItem);
            }

            PhAddJsonArrayObject(results, entry);
            PhDereferenceObject(processItem);
        }

        Result->StructuredContent = AtCreateBatchResult(results);
        return;
    }

    if (!NT_SUCCESS(AtResolveProcessTarget(Call->Arguments, FALSE, 0, &target, Result)))
        return;

    structured = PhCreateJsonObject();
    AtpFillProcessDetail(structured, target.ProcessItem);

    if (includeStatistics)
        AtpAddProcessStatistics(structured, target.ProcessItem);

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteTarget(&target);
}

VOID AtpGetProcessEnvironment(
    _In_ PAT_TOOL_CALL Call,
    _In_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    PVOID environment;
    ULONG environmentLength;
    ULONG enumerationKey;
    PH_ENVIRONMENT_VARIABLE variable;
    AT_ROWS rows;
    PVOID structured;

    status = PhGetProcessEnvironment(
        Target->ProcessHandle,
        !!Target->ProcessItem->IsWow64Process,
        &environment,
        &environmentLength
        );

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Reading the environment block");
        return;
    }

    structured = PhCreateJsonObject();
    PhAddJsonObjectUInt64(structured, "pid", HandleToUlong(Target->ProcessItem->ProcessId));
    PhAddJsonObjectUInt64(structured, "process_sequence_number", Target->ProcessItem->ProcessSequenceNumber);
    AtInitializeRows(&rows, Call->Arguments);

    enumerationKey = 0;

    while (NT_SUCCESS(PhEnumProcessEnvironmentVariables(environment, environmentLength, &enumerationKey, &variable)))
    {
        PVOID entry;

        entry = PhCreateJsonObject();
        AtJsonAddStringRef(entry, "name", &variable.Name);
        AtJsonAddStringRef(entry, "value", &variable.Value);
        AtAddRow(&rows, entry);
    }

    PhFreePage(environment);

    AtAddRows(structured, "variables", &rows);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

VOID AtpControlProcess(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _In_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    PVOID structured;
    ULONG priorityClass = 0;
    IO_PRIORITY_HINT ioPriority = IoPriorityNormal;
    GROUP_AFFINITY groupAffinity;
    KAFFINITY previousMask = 0;
    PH_PROCESS_WS_COUNTERS wsCounters;
    BOOLEAN freezeChanged = FALSE;
    BOOLEAN freezeHeldHere = FALSE;
    ULONG64 workingSetBefore = 0;
    BOOLEAN hasWorkingSetBefore = FALSE;
    ULONG previousPagePriority = 0;
    ULONG64 pagePriority = 0;
    BOOLEAN hasPreviousPagePriority = FALSE;
    ULONG64 affinityMask = 0;
    ULONG64 affinityGroup = 0;
    BOOLEAN hasGroup = FALSE;
    BOOLEAN hasPreviousMask = FALSE;

    memset(&groupAffinity, 0, sizeof(groupAffinity));

    switch (Tool->Action)
    {
    case AtActionTerminateProcess:
        status = PhTerminateProcess(Target->ProcessHandle, STATUS_SUCCESS);
        break;
    case AtActionSuspendProcess:
        status = PhSuspendProcess(Target->ProcessHandle);
        break;
    case AtActionResumeProcess:
        status = PhResumeProcess(Target->ProcessHandle);
        break;
    case AtActionSetProcessPriority:
        {
            PPH_STRING value = AtGetArgumentString(Call->Arguments, "priority_class");

            NT_VERIFY(AtParsePriorityClass(value, &priorityClass));
            PhClearReference(&value);
            status = PhSetProcessPriorityClass(Target->ProcessHandle, (UCHAR)priorityClass);
        }
        break;
    case AtActionSetProcessIoPriority:
        {
            PPH_STRING value = AtGetArgumentString(Call->Arguments, "io_priority");

            NT_VERIFY(AtParseIoPriority(value, &ioPriority));
            PhClearReference(&value);
            status = PhSetProcessIoPriority(Target->ProcessHandle, ioPriority);
        }
        break;
    case AtActionFreezeProcess:
        {
            HANDLE freezeHandle;
            HANDLE previousHandle;

            // The freeze lasts as long as the handle, which lives on the process item so the
            // Processes window and this agree about what is frozen.
            if (ReadPointerAcquire(&Target->ProcessItem->FreezeHandle))
            {
                freezeHeldHere = TRUE;
                status = STATUS_SUCCESS;
                break;
            }

            status = PhFreezeProcess(&freezeHandle, Target->ProcessHandle);

            if (NT_SUCCESS(status))
            {
                previousHandle = InterlockedExchangePointer(&Target->ProcessItem->FreezeHandle, freezeHandle);

                if (previousHandle)
                    NtClose(previousHandle);

                freezeChanged = TRUE;
                freezeHeldHere = TRUE;
            }
        }
        break;
    case AtActionThawProcess:
        {
            HANDLE freezeHandle;

            // The record is not given up until the thaw has actually happened. Closing the handle
            // is itself what ends the freeze, so clearing and closing first would thaw the process,
            // lose the Processes window's record of it, and still report a failure - which is the
            // opposite of every one of the three. PhUiThawTreeProcess orders it this way too.
            freezeHandle = ReadPointerAcquire(&Target->ProcessItem->FreezeHandle);

            if (!freezeHandle)
            {
                // Frozen by something else; this cannot undo it.
                status = STATUS_SUCCESS;
                break;
            }

            status = PhThawProcess(freezeHandle, Target->ProcessHandle);

            if (NT_SUCCESS(status))
            {
                if (freezeHandle = InterlockedExchangePointer(&Target->ProcessItem->FreezeHandle, NULL))
                    NtClose(freezeHandle);

                freezeChanged = TRUE;
            }
        }
        break;
    case AtActionEmptyProcessWorkingSet:
        {
            if (hasWorkingSetBefore = NT_SUCCESS(PhGetProcessWsCounters(Target->ProcessHandle, &wsCounters)))
                workingSetBefore = (ULONG64)wsCounters.NumberOfPages * PAGE_SIZE;

            status = PhSetProcessEmptyWorkingSet(Target->ProcessHandle);
        }
        break;
    case AtActionSetProcessPagePriority:
        {
            NT_VERIFY(AtGetArgumentUInt64(Call->Arguments, "page_priority", &pagePriority));

            // Read before writing: the level a process had is the only way back to it.
            hasPreviousPagePriority = NT_SUCCESS(PhGetProcessPagePriority(Target->ProcessHandle, &previousPagePriority));

            status = PhSetProcessPagePriority(Target->ProcessHandle, (ULONG)pagePriority);
        }
        break;
    case AtActionSetProcessAffinity:
        {
            NT_VERIFY(AtGetArgumentUInt64(Call->Arguments, "affinity_mask", &affinityMask));
            hasGroup = AtGetArgumentUInt64(Call->Arguments, "group", &affinityGroup);

            // Read before writing: nothing else records the previous mask.
            hasPreviousMask = NT_SUCCESS(PhGetProcessAffinityMask(Target->ProcessHandle, &previousMask));

            if (hasGroup)
            {
                groupAffinity.Group = (USHORT)affinityGroup;
                groupAffinity.Mask = (KAFFINITY)affinityMask;
                status = PhSetProcessGroupAffinity(Target->ProcessHandle, groupAffinity);
            }
            else
            {
                status = PhSetProcessAffinityMask(Target->ProcessHandle, (KAFFINITY)affinityMask);
            }
        }
        break;
    default:
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"The operation");
        return;
    }

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, Target->ProcessItem);
    PhAddJsonObject(structured, "action", Tool->Name);

    if (Tool->Action == AtActionSetProcessPriority)
        AtJsonAddStringZ(structured, "priority_class", AtPriorityClassString(priorityClass));
    else if (Tool->Action == AtActionSetProcessIoPriority)
        AtJsonAddStringZ(structured, "io_priority", AtIoPriorityString(ioPriority));
    else if (Tool->Action == AtActionFreezeProcess || Tool->Action == AtActionThawProcess)
    {
        PROCESS_EXTENDED_BASIC_INFORMATION extendedInfo;

        // Asked of the process rather than assumed from the call, and rather than the once-a-second
        // provider.
        if (NT_SUCCESS(PhGetProcessExtendedBasicInformation(Target->ProcessHandle, &extendedInfo)))
            PhAddJsonObjectBoolean(structured, "frozen", !!extendedInfo.IsFrozen);
        else
            AtJsonAddNull(structured, "frozen");

        PhAddJsonObjectBoolean(structured, "changed", freezeChanged);
        PhAddJsonObjectBoolean(structured, "frozen_by_this_instance", freezeHeldHere);
    }
    else if (Tool->Action == AtActionEmptyProcessWorkingSet)
    {
        if (hasWorkingSetBefore)
            PhAddJsonObjectUInt64(structured, "working_set_bytes_before", workingSetBefore);
        else
            AtJsonAddNull(structured, "working_set_bytes_before");

        // Read straight after the call; the process is already faulting pages back.
        if (NT_SUCCESS(PhGetProcessWsCounters(Target->ProcessHandle, &wsCounters)))
            PhAddJsonObjectUInt64(structured, "working_set_bytes_after", (ULONG64)wsCounters.NumberOfPages * PAGE_SIZE);
        else
            AtJsonAddNull(structured, "working_set_bytes_after");
    }
    else if (Tool->Action == AtActionSetProcessPagePriority)
    {
        PhAddJsonObjectUInt64(structured, "page_priority", pagePriority);
        AtJsonAddStringZ(structured, "page_priority_name", AtPagePriorityString((ULONG)pagePriority));

        if (hasPreviousPagePriority)
        {
            PhAddJsonObjectUInt64(structured, "previous_page_priority", previousPagePriority);
            AtJsonAddStringZ(structured, "previous_page_priority_name", AtPagePriorityString(previousPagePriority));
        }
        else
        {
            AtJsonAddNull(structured, "previous_page_priority");
            AtJsonAddNull(structured, "previous_page_priority_name");
        }
    }
    else if (Tool->Action == AtActionSetProcessAffinity)
    {
        AtJsonAddHex(structured, "affinity_mask", affinityMask);

        if (hasGroup)
            PhAddJsonObjectUInt64(structured, "group", affinityGroup);
        else
            AtJsonAddNull(structured, "group");

        if (hasPreviousMask)
            AtJsonAddHex(structured, "previous_affinity_mask", previousMask);
        else
            AtJsonAddNull(structured, "previous_affinity_mask");
    }

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

VOID AtpAddSidStrings(
    _In_ PVOID Object,
    _In_ PCSTR NameKey,
    _In_ PCSTR SidKey,
    _In_opt_ PSID Sid
    )
{
    PPH_STRING fullName;
    PPH_STRING sidString;

    if (!Sid)
    {
        if (NameKey)
            AtJsonAddNull(Object, NameKey);
        if (SidKey)
            AtJsonAddNull(Object, SidKey);
        return;
    }

    if (NameKey)
    {
        fullName = PhGetSidFullName(Sid, TRUE, NULL);
        AtJsonAddString(Object, NameKey, fullName);
        PhClearReference(&fullName);
    }

    if (SidKey)
    {
        sidString = PhSidToStringSid(Sid);
        AtJsonAddString(Object, SidKey, sidString);
        PhClearReference(&sidString);
    }
}

VOID AtpAddGroupFlags(
    _In_ PVOID Object,
    _In_ ULONG Attributes
    )
{
    PVOID flags = PhCreateJsonArray();

    if (Attributes & SE_GROUP_ENABLED)
        PhAddJsonArrayObject(flags, PhCreateJsonStringObject("enabled"));
    if (Attributes & SE_GROUP_ENABLED_BY_DEFAULT)
        PhAddJsonArrayObject(flags, PhCreateJsonStringObject("enabled_by_default"));
    if (Attributes & SE_GROUP_MANDATORY)
        PhAddJsonArrayObject(flags, PhCreateJsonStringObject("mandatory"));
    if (Attributes & SE_GROUP_OWNER)
        PhAddJsonArrayObject(flags, PhCreateJsonStringObject("owner"));
    if (Attributes & SE_GROUP_USE_FOR_DENY_ONLY)
        PhAddJsonArrayObject(flags, PhCreateJsonStringObject("use_for_deny_only"));
    if (Attributes & SE_GROUP_LOGON_ID)
        PhAddJsonArrayObject(flags, PhCreateJsonStringObject("logon_id"));
    if (Attributes & SE_GROUP_INTEGRITY)
        PhAddJsonArrayObject(flags, PhCreateJsonStringObject("integrity"));
    if (Attributes & SE_GROUP_RESOURCE)
        PhAddJsonArrayObject(flags, PhCreateJsonStringObject("resource"));

    PhAddJsonObjectValue(Object, "flags", flags);
}

ULONG AtpQueryTokenUlong(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_INFORMATION_CLASS InfoClass
    )
{
    ULONG value = 0;
    ULONG returnLength;

    NtQueryInformationToken(TokenHandle, InfoClass, &value, sizeof(value), &returnLength);

    return value;
}

VOID AtpGetProcessToken(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    AT_TARGET target;
    HANDLE processHandle;
    HANDLE tokenHandle;
    PVOID structured;
    PH_TOKEN_USER tokenUser;
    PH_TOKEN_OWNER tokenOwner;
    PTOKEN_PRIMARY_GROUP primaryGroup;
    PTOKEN_GROUPS groups;
    PTOKEN_PRIVILEGES privileges;
    PH_TOKEN_APPCONTAINER appContainerSid;
    PVOID groupArray;
    PVOID privilegeArray;
    ULONG i;

    if (!NT_SUCCESS(AtResolveProcessTarget(Call->Arguments, FALSE, 0, &target, Result)))
        return;

    if (!PH_IS_REAL_PROCESS_ID(target.ProcessItem->ProcessId) ||
        !NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, target.ProcessItem->ProcessId)))
    {
        AtSetToolStatusError(Result, PH_IS_REAL_PROCESS_ID(target.ProcessItem->ProcessId) ? status : STATUS_INVALID_CID, L"Opening the process");
        AtDeleteTarget(&target);
        return;
    }

    status = PhOpenProcessToken(processHandle, TOKEN_QUERY, &tokenHandle);
    NtClose(processHandle);

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Opening the process token");
        AtDeleteTarget(&target);
        return;
    }

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, target.ProcessItem);

    if (NT_SUCCESS(PhGetTokenUser(tokenHandle, &tokenUser)))
        AtpAddSidStrings(structured, "user", "user_sid", tokenUser.User.Sid);
    else
    {
        AtJsonAddNull(structured, "user");
        AtJsonAddNull(structured, "user_sid");
    }

    if (NT_SUCCESS(PhGetTokenOwner(tokenHandle, &tokenOwner)))
        AtpAddSidStrings(structured, "owner", NULL, tokenOwner.Owner.Sid);
    else
        AtJsonAddNull(structured, "owner");

    if (NT_SUCCESS(PhGetTokenPrimaryGroup(tokenHandle, &primaryGroup)))
    {
        AtpAddSidStrings(structured, "primary_group", NULL, primaryGroup->PrimaryGroup);
        PhFree(primaryGroup);
    }
    else
    {
        AtJsonAddNull(structured, "primary_group");
    }

    AtJsonAddStringRef(structured, "integrity_level", target.ProcessItem->IntegrityString);
    AtJsonAddStringZ(structured, "elevation_type",
        target.ProcessItem->ElevationType == TokenElevationTypeFull ? L"Full" :
        target.ProcessItem->ElevationType == TokenElevationTypeLimited ? L"Limited" :
        target.ProcessItem->ElevationType == TokenElevationTypeDefault ? L"Default" : NULL);
    PhAddJsonObjectBoolean(structured, "is_elevated", !!target.ProcessItem->IsElevated);
    PhAddJsonObjectUInt64(structured, "session_id", AtpQueryTokenUlong(tokenHandle, TokenSessionId));

    if (NT_SUCCESS(PhGetTokenAppContainerSid(tokenHandle, &appContainerSid)) &&
        appContainerSid.TokenAppContainer.TokenAppContainer)
    {
        PhAddJsonObjectBoolean(structured, "is_app_container", TRUE);
        AtpAddSidStrings(structured, NULL, "app_container_sid", appContainerSid.TokenAppContainer.TokenAppContainer);
    }
    else
    {
        PhAddJsonObjectBoolean(structured, "is_app_container", FALSE);
        AtJsonAddNull(structured, "app_container_sid");
    }

    AtJsonAddString(structured, "package_full_name", target.ProcessItem->PackageFullName);
    PhAddJsonObjectBoolean(structured, "is_restricted", !!AtpQueryTokenUlong(tokenHandle, TokenHasRestrictions));
    PhAddJsonObjectBoolean(structured, "ui_access", !!AtpQueryTokenUlong(tokenHandle, TokenUIAccess));
    PhAddJsonObjectBoolean(structured, "virtualization_allowed", !!AtpQueryTokenUlong(tokenHandle, TokenVirtualizationAllowed));
    PhAddJsonObjectBoolean(structured, "virtualization_enabled", !!AtpQueryTokenUlong(tokenHandle, TokenVirtualizationEnabled));

    if (NT_SUCCESS(PhGetTokenGroups(tokenHandle, &groups)))
    {
        groupArray = PhCreateJsonArray();

        for (i = 0; i < groups->GroupCount; i++)
        {
            PVOID row = PhCreateJsonObject();

            AtpAddSidStrings(row, "name", "sid", groups->Groups[i].Sid);
            AtpAddGroupFlags(row, groups->Groups[i].Attributes);
            PhAddJsonArrayObject(groupArray, row);
        }

        PhFree(groups);
        PhAddJsonObjectValue(structured, "groups", groupArray);
    }
    else
    {
        // Holding no groups and not being able to read them are opposite answers.
        AtJsonAddNull(structured, "groups");
    }

    if (NT_SUCCESS(PhGetTokenPrivileges(tokenHandle, &privileges)))
    {
        privilegeArray = PhCreateJsonArray();

        for (i = 0; i < privileges->PrivilegeCount; i++)
        {
            PVOID row = PhCreateJsonObject();
            PPH_STRING name = NULL;

            if (NT_SUCCESS(PhLookupPrivilegeName(&privileges->Privileges[i].Luid, &name)))
            {
                AtJsonAddString(row, "name", name);
                PhDereferenceObject(name);
            }
            else
            {
                AtJsonAddNull(row, "name");
            }

            PhAddJsonObjectBoolean(row, "enabled", !!(privileges->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED));
            PhAddJsonObjectBoolean(row, "enabled_by_default", !!(privileges->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT));
            PhAddJsonObjectBoolean(row, "removed", !!(privileges->Privileges[i].Attributes & SE_PRIVILEGE_REMOVED));
            PhAddJsonArrayObject(privilegeArray, row);
        }

        PhFree(privileges);
        PhAddJsonObjectValue(structured, "privileges", privilegeArray);
    }
    else
    {
        AtJsonAddNull(structured, "privileges");
    }

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    NtClose(tokenHandle);
    AtDeleteTarget(&target);
}

typedef struct _AT_WINDOW_CONTEXT
{
    HANDLE ProcessId;
    BOOLEAN VisibleOnly;
    AT_ROWS Windows;
} AT_WINDOW_CONTEXT, *PAT_WINDOW_CONTEXT;

_Function_class_(PH_WINDOW_ENUM_CALLBACK)
BOOLEAN NTAPI AtpWindowCallback(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    PAT_WINDOW_CONTEXT context = Context;
    CLIENT_ID clientId;
    PVOID row;
    PPH_STRING text;
    WCHAR className[256];
    RECT rect;
    BOOLEAN visible;

    clientId.UniqueProcess = NULL;
    clientId.UniqueThread = NULL;
    GetWindowThreadProcessId(WindowHandle, (PDWORD)&clientId.UniqueProcess);

    if (clientId.UniqueProcess != context->ProcessId)
        return TRUE;

    visible = !!IsWindowVisible(WindowHandle);

    if (context->VisibleOnly && !visible)
        return TRUE;

    row = PhCreateJsonObject();
    AtJsonAddPointer(row, "handle", WindowHandle);

    text = PhGetWindowText(WindowHandle);
    AtJsonAddString(row, "title", text);
    PhClearReference(&text);

    if (NT_SUCCESS(PhGetClassName(WindowHandle, className, RTL_NUMBER_OF(className), NULL)))
        AtJsonAddStringZ(row, "class_name", className);
    else
        AtJsonAddNull(row, "class_name");

    PhAddJsonObjectUInt64(row, "tid", GetWindowThreadProcessId(WindowHandle, NULL));
    PhAddJsonObjectBoolean(row, "is_visible", visible);
    PhAddJsonObjectBoolean(row, "is_minimized", !!IsIconic(WindowHandle));
    PhAddJsonObjectBoolean(row, "is_hung", !!IsHungAppWindow(WindowHandle));

    if (GetWindowRect(WindowHandle, &rect))
    {
        PVOID rectObject = PhCreateJsonObject();

        PhAddJsonObjectInt64(rectObject, "left", rect.left);
        PhAddJsonObjectInt64(rectObject, "top", rect.top);
        PhAddJsonObjectInt64(rectObject, "right", rect.right);
        PhAddJsonObjectInt64(rectObject, "bottom", rect.bottom);
        PhAddJsonObjectValue(row, "rect", rectObject);
    }

    AtAddRow(&context->Windows, row);

    return TRUE;
}

VOID AtpGetProcessWindows(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    BOOLEAN enumComplete = TRUE;
    AT_TARGET target;
    AT_WINDOW_CONTEXT context;
    PVOID visibleMember;
    PVOID structured;

    if (!NT_SUCCESS(AtResolveProcessTarget(Call->Arguments, FALSE, 0, &target, Result)))
        return;

    memset(&context, 0, sizeof(AT_WINDOW_CONTEXT));
    context.ProcessId = target.ProcessItem->ProcessId;
    context.VisibleOnly = TRUE;
    AtInitializeRows(&context.Windows, Call->Arguments);

    if (visibleMember = AtJsonGetObjectMember(Call->Arguments, "visible_only", PH_JSON_OBJECT_TYPE_BOOLEAN))
        context.VisibleOnly = AtJsonGetObjectBoolean(Call->Arguments, "visible_only");

    enumComplete = !!PhEnumWindows(AtpWindowCallback, &context);

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, target.ProcessItem);
    PhAddJsonObjectBoolean(structured, "enumeration_complete", enumComplete);
    AtAddRows(structured, "windows", &context.Windows);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteTarget(&target);
}

VOID AtpAddJobLimits(
    _In_ PVOID Object,
    _In_ HANDLE JobHandle
    )
{
    static CONST ULONG limitFlags[] =
    {
        JOB_OBJECT_LIMIT_WORKINGSET, JOB_OBJECT_LIMIT_PROCESS_TIME, JOB_OBJECT_LIMIT_JOB_TIME,
        JOB_OBJECT_LIMIT_ACTIVE_PROCESS, JOB_OBJECT_LIMIT_AFFINITY, JOB_OBJECT_LIMIT_PRIORITY_CLASS,
        JOB_OBJECT_LIMIT_PRESERVE_JOB_TIME, JOB_OBJECT_LIMIT_SCHEDULING_CLASS,
        JOB_OBJECT_LIMIT_PROCESS_MEMORY, JOB_OBJECT_LIMIT_JOB_MEMORY,
        JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION, JOB_OBJECT_LIMIT_BREAKAWAY_OK,
        JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK, JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE,
        JOB_OBJECT_LIMIT_SUBSET_AFFINITY,
    };
    static CONST PWSTR limitNames[] =
    {
        L"working_set", L"process_time", L"job_time", L"active_process", L"affinity",
        L"priority_class", L"preserve_job_time", L"scheduling_class", L"process_memory",
        L"job_memory", L"die_on_unhandled_exception", L"breakaway_ok", L"silent_breakaway_ok",
        L"kill_on_job_close", L"subset_affinity",
    };
    static CONST ULONG uiFlags[] =
    {
        JOB_OBJECT_UILIMIT_HANDLES, JOB_OBJECT_UILIMIT_READCLIPBOARD,
        JOB_OBJECT_UILIMIT_WRITECLIPBOARD, JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS,
        JOB_OBJECT_UILIMIT_DISPLAYSETTINGS, JOB_OBJECT_UILIMIT_GLOBALATOMS,
        JOB_OBJECT_UILIMIT_DESKTOP, JOB_OBJECT_UILIMIT_EXITWINDOWS,
    };
    static CONST PWSTR uiNames[] =
    {
        L"handles", L"read_clipboard", L"write_clipboard", L"system_parameters",
        L"display_settings", L"global_atoms", L"desktop", L"exit_windows",
    };
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION extendedLimits;
    JOBOBJECT_BASIC_UI_RESTRICTIONS uiRestrictions;
    PVOID entry;

    if (NT_SUCCESS(PhGetJobExtendedLimits(JobHandle, &extendedLimits)))
    {
        PJOBOBJECT_BASIC_LIMIT_INFORMATION basic = &extendedLimits.BasicLimitInformation;

        entry = PhCreateJsonObject();
        AtJsonAddFlagStrings(entry, "flags", basic->LimitFlags, limitFlags, (CONST PWSTR*)limitNames, RTL_NUMBER_OF(limitFlags));

        // Each limit is null unless its flag is set: a maximum of zero and no maximum are
        // different.
        if (FlagOn(basic->LimitFlags, JOB_OBJECT_LIMIT_ACTIVE_PROCESS))
            PhAddJsonObjectUInt64(entry, "active_process_limit", basic->ActiveProcessLimit);
        else
            AtJsonAddNull(entry, "active_process_limit");

        if (FlagOn(basic->LimitFlags, JOB_OBJECT_LIMIT_PRIORITY_CLASS))
            PhAddJsonObjectUInt64(entry, "priority_class", basic->PriorityClass);
        else
            AtJsonAddNull(entry, "priority_class");

        if (FlagOn(basic->LimitFlags, JOB_OBJECT_LIMIT_SCHEDULING_CLASS))
            PhAddJsonObjectUInt64(entry, "scheduling_class", basic->SchedulingClass);
        else
            AtJsonAddNull(entry, "scheduling_class");

        if (FlagOn(basic->LimitFlags, JOB_OBJECT_LIMIT_AFFINITY))
            AtJsonAddHex(entry, "affinity", basic->Affinity);
        else
            AtJsonAddNull(entry, "affinity");

        if (FlagOn(basic->LimitFlags, JOB_OBJECT_LIMIT_WORKINGSET))
        {
            PhAddJsonObjectUInt64(entry, "minimum_working_set", basic->MinimumWorkingSetSize);
            PhAddJsonObjectUInt64(entry, "maximum_working_set", basic->MaximumWorkingSetSize);
        }
        else
        {
            AtJsonAddNull(entry, "minimum_working_set");
            AtJsonAddNull(entry, "maximum_working_set");
        }

        if (FlagOn(basic->LimitFlags, JOB_OBJECT_LIMIT_PROCESS_TIME))
            AtJsonAddDuration(entry, "per_process_user_time", basic->PerProcessUserTimeLimit.QuadPart);
        else
            AtJsonAddNull(entry, "per_process_user_time");

        if (FlagOn(basic->LimitFlags, JOB_OBJECT_LIMIT_JOB_TIME))
            AtJsonAddDuration(entry, "per_job_user_time", basic->PerJobUserTimeLimit.QuadPart);
        else
            AtJsonAddNull(entry, "per_job_user_time");

        if (FlagOn(basic->LimitFlags, JOB_OBJECT_LIMIT_PROCESS_MEMORY))
            PhAddJsonObjectUInt64(entry, "process_memory_limit", extendedLimits.ProcessMemoryLimit);
        else
            AtJsonAddNull(entry, "process_memory_limit");

        if (FlagOn(basic->LimitFlags, JOB_OBJECT_LIMIT_JOB_MEMORY))
            PhAddJsonObjectUInt64(entry, "job_memory_limit", extendedLimits.JobMemoryLimit);
        else
            AtJsonAddNull(entry, "job_memory_limit");

        PhAddJsonObjectUInt64(entry, "peak_process_memory_used", extendedLimits.PeakProcessMemoryUsed);
        PhAddJsonObjectUInt64(entry, "peak_job_memory_used", extendedLimits.PeakJobMemoryUsed);
        PhAddJsonObjectValue(Object, "limits", entry);
    }
    else
    {
        AtJsonAddNull(Object, "limits");
    }

    if (NT_SUCCESS(PhGetJobBasicUiRestrictions(JobHandle, &uiRestrictions)))
    {
        AtJsonAddFlagStrings(Object, "ui_restrictions", uiRestrictions.UIRestrictionsClass, uiFlags, (CONST PWSTR*)uiNames, RTL_NUMBER_OF(uiFlags));
    }
    else
    {
        AtJsonAddNull(Object, "ui_restrictions");
    }
}

VOID AtpAddJobAccounting(
    _In_ PVOID Object,
    _In_ HANDLE JobHandle
    )
{
    JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION accounting;
    PVOID entry;

    if (!NT_SUCCESS(PhGetJobBasicAndIoAccounting(JobHandle, &accounting)))
    {
        AtJsonAddNull(Object, "accounting");
        return;
    }

    entry = PhCreateJsonObject();
    AtJsonAddDuration(entry, "total_user_time", accounting.BasicInfo.TotalUserTime.QuadPart);
    AtJsonAddDuration(entry, "total_kernel_time", accounting.BasicInfo.TotalKernelTime.QuadPart);
    PhAddJsonObjectUInt64(entry, "total_page_fault_count", accounting.BasicInfo.TotalPageFaultCount);
    PhAddJsonObjectUInt64(entry, "total_processes", accounting.BasicInfo.TotalProcesses);
    PhAddJsonObjectUInt64(entry, "active_processes", accounting.BasicInfo.ActiveProcesses);
    PhAddJsonObjectUInt64(entry, "terminated_processes", accounting.BasicInfo.TotalTerminatedProcesses);
    PhAddJsonObjectUInt64(entry, "read_operation_count", accounting.IoInfo.ReadOperationCount);
    PhAddJsonObjectUInt64(entry, "write_operation_count", accounting.IoInfo.WriteOperationCount);
    PhAddJsonObjectUInt64(entry, "other_operation_count", accounting.IoInfo.OtherOperationCount);
    PhAddJsonObjectUInt64(entry, "read_transfer_count", accounting.IoInfo.ReadTransferCount);
    PhAddJsonObjectUInt64(entry, "write_transfer_count", accounting.IoInfo.WriteTransferCount);
    PhAddJsonObjectUInt64(entry, "other_transfer_count", accounting.IoInfo.OtherTransferCount);
    PhAddJsonObjectValue(Object, "accounting", entry);
}

VOID AtpAddJobProcesses(
    _In_ PVOID Object,
    _In_ HANDLE JobHandle,
    _In_ PAT_TOOL_CALL Call
    )
{
    PJOBOBJECT_BASIC_PROCESS_ID_LIST processIdList;
    AT_ROWS rows;
    ULONG i;

    if (!NT_SUCCESS(PhGetJobProcessIdList(JobHandle, &processIdList)))
    {
        AtJsonAddNull(Object, "processes");
        return;
    }

    AtInitializeRows(&rows, Call->Arguments);

    for (i = 0; i < processIdList->NumberOfProcessIdsInList; i++)
    {
        HANDLE processId = (HANDLE)processIdList->ProcessIdList[i];
        PPH_PROCESS_ITEM processItem;
        PVOID row;

        row = PhCreateJsonObject();
        PhAddJsonObjectUInt64(row, "pid", HandleToUlong(processId));

        if (processItem = PhReferenceProcessItem(processId))
        {
            AtJsonAddString(row, "name", processItem->ProcessName);
            PhAddJsonObjectUInt64(row, "process_sequence_number", processItem->ProcessSequenceNumber);
            PhDereferenceObject(processItem);
        }
        else
        {
            AtJsonAddNull(row, "name");
            AtJsonAddNull(row, "process_sequence_number");
        }

        AtAddRow(&rows, row);
    }

    AtAddRows(Object, "processes", &rows);
    AtDeleteRows(&rows);
    PhFree(processIdList);
}

VOID AtpGetProcessJob(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    AT_TARGET target;
    HANDLE jobHandle = NULL;
    BOOLEAN isInJob = FALSE;
    PVOID structured;

    status = AtResolveProcessTarget(Call->Arguments, FALSE, PROCESS_QUERY_LIMITED_INFORMATION, &target, Result);

    if (!NT_SUCCESS(status))
        return;

    // STATUS_PROCESS_NOT_IN_JOB is the answer "no", not a failure.
    if (target.ProcessHandle)
    {
        status = NtIsProcessInJob(target.ProcessHandle, NULL);
        isInJob = status == STATUS_PROCESS_IN_JOB;
    }

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, target.ProcessItem);
    PhAddJsonObjectBoolean(structured, "is_in_job", isInJob);

    // There is no user-mode way to open a process's job. The level is checked rather than the call
    // attempted: KphCreateUserMessage asserts with no connection.
    if (isInJob && target.ProcessHandle && KsiLevel() != KphLevelNone)
        status = KphOpenProcessJob(target.ProcessHandle, JOB_OBJECT_QUERY, &jobHandle);
    else
        status = STATUS_NOT_SUPPORTED;

    if (isInJob && NT_SUCCESS(status) && jobHandle)
    {
        PPH_STRING name = NULL;

        PhGetHandleInformation(NtCurrentProcess(), jobHandle, ULONG_MAX, NULL, NULL, NULL, &name);
        AtJsonAddString(structured, "name", name);
        PhClearReference(&name);

        AtpAddJobLimits(structured, jobHandle);
        AtpAddJobAccounting(structured, jobHandle);
        AtpAddJobProcesses(structured, jobHandle, Call);
        AtJsonAddNull(structured, "error");
        AtJsonAddNull(structured, "message");

        NtClose(jobHandle);
    }
    else
    {
        AtJsonAddNull(structured, "name");
        AtJsonAddNull(structured, "limits");
        AtJsonAddNull(structured, "ui_restrictions");
        AtJsonAddNull(structured, "accounting");
        AtJsonAddNull(structured, "processes");

        if (isInJob && KsiLevel() == KphLevelNone)
        {
            PPH_STRING message;

            message = PhFormatString(
                L"The process is in a job, but opening one has no user-mode route: it comes from the "
                L"System Informer driver, which is not available to this instance (access level: %s).",
                AtKphLevelString(KsiLevel())
                );

            PhAddJsonObject(structured, "error", "failed");
            AtJsonAddString(structured, "message", message);
            AtSetToolHint(Result, AT_HINT_NEEDS_DRIVER);
            PhClearReference(&message);
        }
        else if (isInJob)
        {
            PPH_STRING message = PhGetStatusMessage(status, 0);

            PhAddJsonObject(structured, "error", status == STATUS_ACCESS_DENIED ? "access_denied" : "failed");
            AtJsonAddStringZ(structured, "message", PhGetStringOrDefault(message, L"unknown error"));
            PhClearReference(&message);
        }
        else
        {
            AtJsonAddNull(structured, "error");
            AtJsonAddNull(structured, "message");
        }
    }

    AtAddSnapshot(structured);
    Result->StructuredContent = structured;

    AtDeleteTarget(&target);
}

PCWSTR AtpProcessStateLevelString(
    _In_ KPH_PROCESS_STATE State
    )
{
    if ((State & KPH_PROCESS_STATE_MAXIMUM) == KPH_PROCESS_STATE_MAXIMUM)
        return L"maximum";
    if ((State & KPH_PROCESS_STATE_HIGH) == KPH_PROCESS_STATE_HIGH)
        return L"high";
    if ((State & KPH_PROCESS_STATE_MEDIUM) == KPH_PROCESS_STATE_MEDIUM)
        return L"medium";
    if ((State & KPH_PROCESS_STATE_LOW) == KPH_PROCESS_STATE_LOW)
        return L"low";
    if ((State & KPH_PROCESS_STATE_MINIMUM) == KPH_PROCESS_STATE_MINIMUM)
        return L"minimum";

    return L"none";
}

VOID AtpGetProcessKsiState(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    static CONST ULONG stateFlags[] =
    {
        KPH_PROCESS_SECURELY_CREATED, KPH_PROCESS_VERIFIED_PROCESS, KPH_PROCESS_PROTECTED_PROCESS,
        KPH_PROCESS_NO_UNTRUSTED_IMAGES, KPH_PROCESS_HAS_FILE_OBJECT,
        KPH_PROCESS_HAS_SECTION_OBJECT_POINTERS, KPH_PROCESS_NO_USER_WRITABLE_REFERENCES,
        KPH_PROCESS_NO_FILE_TRANSACTION, KPH_PROCESS_NOT_BEING_DEBUGGED,
        KPH_PROCESS_NO_WRITABLE_FILE_OBJECT, KPH_PROCESS_CREATE_NOTIFICATION
    };
    static CONST PWSTR stateNames[] =
    {
        L"securely_created", L"verified_process", L"protected_process",
        L"no_untrusted_images", L"has_file_object",
        L"has_section_object_pointers", L"no_user_writable_references",
        L"no_file_transaction", L"not_being_debugged",
        L"no_writable_file_object", L"create_notification"
    };
    NTSTATUS status;
    AT_TARGET target;
    KPH_PROCESS_BASIC_INFORMATION basicInfo;
    PVOID structured;
    PVOID entry;

    // A read-tier tool resolves its own target; the dispatcher only resolves for tiers that hold an
    // object across a prompt.
    status = AtResolveProcessTarget(Call->Arguments, FALSE, PROCESS_QUERY_LIMITED_INFORMATION, &target, Result);

    if (!NT_SUCCESS(status))
        return;

    // KphCreateUserMessage asserts when there is no connection.
    if (KsiLevel() < KphLevelMed)
    {
        AtSetToolError(
            Result,
            "failed",
            STATUS_NOT_SUPPORTED,
            L"This is the System Informer driver's own view of a process and there is no user-mode "
            L"equivalent; the driver is not available to this instance (access level: %s).",
            AtKphLevelString(KsiLevel())
            );
        AtSetToolHint(Result, AT_HINT_NEEDS_DRIVER);
        AtDeleteTarget(&target);
        return;
    }

    memset(&basicInfo, 0, sizeof(basicInfo));

    status = KphQueryInformationProcess(
        target.ProcessHandle,
        KphProcessBasicInformation,
        &basicInfo,
        sizeof(basicInfo),
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Querying the driver for the process state");
        AtDeleteTarget(&target);
        return;
    }

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, target.ProcessItem);
    AtJsonAddStringZ(structured, "ksi_level", AtKphLevelString(KsiLevel()));

    AtJsonAddHex(structured, "state", (ULONG)basicInfo.ProcessState);
    AtJsonAddFlagStrings(structured, "state_names", (ULONG)basicInfo.ProcessState,
        stateFlags, stateNames, RTL_NUMBER_OF(stateFlags));
    AtJsonAddStringZ(structured, "state_level", AtpProcessStateLevelString(basicInfo.ProcessState));

    // These describe the driver's trust relationship with a client it protects, not the process,
    // and state_names carries them. Not a Windows protected process.
    PhAddJsonObjectBoolean(structured, "create_notification", !!basicInfo.CreateNotification);
    PhAddJsonObjectBoolean(structured, "exit_notification", !!basicInfo.ExitNotification);
    PhAddJsonObjectBoolean(structured, "is_wow64", !!basicInfo.IsWow64);
    PhAddJsonObjectBoolean(structured, "is_subsystem_process", !!basicInfo.IsSubsystemProcess);

    AtJsonAddHex(structured, "process_start_key", basicInfo.ProcessStartKey);
    PhAddJsonObjectUInt64(structured, "user_writable_references", basicInfo.UserWritableReferences);
    PhAddJsonObjectUInt64(structured, "thread_count", basicInfo.NumberOfThreads);

    // Recorded at creation, so it still names the creator after that process has exited.
    entry = PhCreateJsonObject();
    PhAddJsonObjectUInt64(entry, "pid", HandleToUlong(basicInfo.CreatorClientId.UniqueProcess));
    PhAddJsonObjectUInt64(entry, "tid", HandleToUlong(basicInfo.CreatorClientId.UniqueThread));
    PhAddJsonObjectValue(structured, "creator", entry);

    PhAddJsonObjectUInt64(structured, "image_loads", basicInfo.NumberOfImageLoads);

    // Only tracked for a verified process; zero for any other would read as "nothing untrusted was
    // loaded".
    if (FlagOn(basicInfo.ProcessState, KPH_PROCESS_VERIFIED_PROCESS))
    {
        entry = PhCreateJsonObject();
        PhAddJsonObjectUInt64(entry, "microsoft", basicInfo.NumberOfMicrosoftImageLoads);
        PhAddJsonObjectUInt64(entry, "antimalware", basicInfo.NumberOfAntimalwareImageLoads);
        PhAddJsonObjectUInt64(entry, "verified", basicInfo.NumberOfVerifiedImageLoads);
        PhAddJsonObjectUInt64(entry, "untrusted", basicInfo.NumberOfUntrustedImageLoads);
        PhAddJsonObjectValue(structured, "image_load_counts", entry);
    }
    else
    {
        AtJsonAddNull(structured, "image_load_counts");
    }

    // The allowed masks are not reported: they exist only for a process the driver protects, which
    // is System Informer's own.

    AtAddSnapshot(structured);
    Result->StructuredContent = structured;

    AtDeleteTarget(&target);
}

// A scan finds every process it can open, not only the interesting ones, and a machine has hundreds.
#define AT_HIDDEN_MAXIMUM_ENTRIES 4096

typedef struct _AT_HIDDEN_ENTRY
{
    HANDLE ProcessId;
    PPH_STRING FileName;
    PH_ZOMBIE_PROCESS_TYPE Type;
    ULONG HandleCount;
    BOOLEAN HasHandleCount;
} AT_HIDDEN_ENTRY, *PAT_HIDDEN_ENTRY;

typedef struct _AT_HIDDEN_CONTEXT
{
    PPH_LIST Entries;
    PPH_HASHTABLE Seen;
    ULONG EnumeratedCount;
    ULONG NormalCount;
    BOOLEAN IncludeNormal;
    BOOLEAN LimitReached;
} AT_HIDDEN_CONTEXT, *PAT_HIDDEN_CONTEXT;

PCWSTR AtpZombieTypeString(
    _In_ PH_ZOMBIE_PROCESS_TYPE Type
    )
{
    switch (Type)
    {
    case UnknownProcess:
        return L"unknown";
    case NormalProcess:
        return L"normal";
    case ZombieProcess:
        return L"zombie";
    case TerminatedProcess:
        return L"terminated";
    }

    return NULL;
}

BOOLEAN AtpParseZombieMethod(
    _In_opt_ PPH_STRING Name,
    _Out_ PH_ZOMBIE_PROCESS_METHOD* Method
    )
{
    *Method = BruteForceScanMethod;

    if (!Name)
        return TRUE;

    if (PhEqualString2(Name, L"brute_force", TRUE))
        *Method = BruteForceScanMethod;
    // csr_handles is not offered: it needs PROCESS_DUP_HANDLE on csrss, which a protected process
    // grants to nobody.
    else if (PhEqualString2(Name, L"process_handles", TRUE))
        *Method = ProcessHandleScanMethod;
    else if (PhEqualString2(Name, L"registry", TRUE))
        *Method = RegistryScanMethod;
    else if (PhEqualString2(Name, L"etw_guid", TRUE))
        *Method = EtwGuidScanMethod;
    else if (PhEqualString2(Name, L"ntdll", TRUE))
        *Method = NtdllScanMethod;
    else
        return FALSE;

    return TRUE;
}

_Function_class_(PPH_ENUM_ZOMBIE_PROCESSES_CALLBACK)
BOOLEAN NTAPI AtpZombieProcessCallback(
    _In_ PPH_ZOMBIE_PROCESS_ENTRY Process,
    _In_opt_ PVOID Context
    )
{
    PAT_HIDDEN_CONTEXT context = Context;
    PAT_HIDDEN_ENTRY entry;

    if (!context)
        return FALSE;

    context->EnumeratedCount++;

    // A method can report the same process many times; the first sighting is kept.
    if (PhFindItemSimpleHashtable(context->Seen, Process->ProcessId))
        return TRUE;

    PhAddItemSimpleHashtable(context->Seen, Process->ProcessId, NULL);

    if (context->Entries->Count >= AT_HIDDEN_MAXIMUM_ENTRIES)
    {
        context->LimitReached = TRUE;
        return FALSE;
    }

    entry = PhAllocateZero(sizeof(AT_HIDDEN_ENTRY));
    entry->ProcessId = Process->ProcessId;
    entry->Type = Process->Type;
    entry->HandleCount = Process->HandleCount;
    entry->HasHandleCount = Process->HasHandleCount;
    PhSetReference(&entry->FileName, Process->FileName);
    PhAddItemList(context->Entries, entry);

    return TRUE;
}

VOID AtpListHiddenProcesses(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    AT_HIDDEN_CONTEXT context;
    AT_ROWS rows;
    PH_ZOMBIE_PROCESS_METHOD method;
    PPH_STRING methodName;
    PVOID structured;
    PVOID processes = NULL;
    ULONG i;

    methodName = AtGetArgumentString(Call->Arguments, "method");

    if (!AtpParseZombieMethod(methodName, &method))
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER,
            L"method must be brute_force, process_handles, registry, etw_guid or ntdll.");
        PhClearReference(&methodName);
        return;
    }

    memset(&context, 0, sizeof(AT_HIDDEN_CONTEXT));
    context.Entries = PhCreateList(64);
    context.Seen = PhCreateSimpleHashtable(64);
    context.IncludeNormal = AtJsonGetObjectBoolean(Call->Arguments, "include_normal");

    status = PhEnumZombieProcesses(method, AtpZombieProcessCallback, &context);

    if (!NT_SUCCESS(status))
    {
        PPH_STRING operation;

        operation = PhFormatString(L"The %s scan",
            PhGetStringOrDefault(methodName, L"brute_force"));
        AtSetToolStatusError(Result, status, PhGetString(operation));
        PhDereferenceObject(operation);
        goto CleanupExit;
    }

    // The list is read after the scan: a process that started or exited during it is in one view
    // and not the other. Without it every entry would report as not in the process list.
    status = PhEnumProcesses(&processes);

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Reading the process list");
        goto CleanupExit;
    }

    structured = PhCreateJsonObject();
    AtInitializeRows(&rows, Call->Arguments);

    for (i = 0; i < context.Entries->Count; i++)
    {
        PAT_HIDDEN_ENTRY entry = context.Entries->Items[i];
        PVOID row;
        PPH_STRING baseName = NULL;
        BOOLEAN inList = FALSE;

        if (processes)
        {
            PSYSTEM_PROCESS_INFORMATION process;

            process = PH_FIRST_PROCESS(processes);

            do
            {
                if (process->UniqueProcessId == entry->ProcessId)
                {
                    inList = TRUE;
                    break;
                }
            } while (process = PH_NEXT_PROCESS(process));
        }

        // Decided on in_process_list, not the scan's type: a protected process is "unknown" to the
        // scan and right there in the list.
        if (inList)
        {
            context.NormalCount++;

            if (!context.IncludeNormal)
                continue;
        }

        if (entry->FileName)
            baseName = PhGetBaseName(entry->FileName);

        row = PhCreateJsonObject();
        PhAddJsonObjectUInt64(row, "pid", HandleToUlong(entry->ProcessId));
        AtJsonAddString(row, "name", baseName);
        AtJsonAddWin32FileName(row, "file_path", entry->FileName);
        AtJsonAddStringZ(row, "type", AtpZombieTypeString(entry->Type));
        PhAddJsonObjectBoolean(row, "in_process_list", inList);

        if (entry->HasHandleCount)
            PhAddJsonObjectUInt64(row, "handle_count", entry->HandleCount);
        else
            AtJsonAddNull(row, "handle_count");

        PhClearReference(&baseName);
        AtAddRow(&rows, row);
    }

    if (processes)
        PhFree(processes);

    AtAddRows(structured, "processes", &rows);
    AtJsonAddStringZ(structured, "method", PhGetStringOrDefault(methodName, L"brute_force"));
    PhAddJsonObjectUInt64(structured, "enumerated_count", context.EnumeratedCount);
    PhAddJsonObjectUInt64(structured, "distinct_count", context.Entries->Count);
    PhAddJsonObjectUInt64(structured, "normal_count", context.NormalCount);
    PhAddJsonObjectBoolean(structured, "scan_limit_reached", context.LimitReached);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

CleanupExit:
    for (i = 0; i < context.Entries->Count; i++)
    {
        PAT_HIDDEN_ENTRY entry = context.Entries->Items[i];

        PhClearReference(&entry->FileName);
        PhFree(entry);
    }

    PhDereferenceObject(context.Entries);
    PhDereferenceObject(context.Seen);
    PhClearReference(&methodName);
}

VOID AtProcessInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    switch (Tool->Action)
    {
    case AtActionListProcesses:
        AtpListProcesses(Call, Result);
        break;
    case AtActionGetProcess:
        AtpGetProcess(Call, Result);
        break;
    case AtActionReadProcessEnvironment:
        AtpGetProcessEnvironment(Call, Target, Result);
        break;
    case AtActionTerminateProcess:
    case AtActionSuspendProcess:
    case AtActionResumeProcess:
    case AtActionSetProcessPriority:
    case AtActionSetProcessIoPriority:
    case AtActionSetProcessAffinity:
    case AtActionSetProcessPagePriority:
    case AtActionEmptyProcessWorkingSet:
    case AtActionFreezeProcess:
    case AtActionThawProcess:
        AtpControlProcess(Tool, Call, Target, Result);
        break;
    case AtActionGetProcessToken:
        AtpGetProcessToken(Call, Result);
        break;
    case AtActionGetProcessWindows:
        AtpGetProcessWindows(Call, Result);
        break;
    case AtActionGetProcessJob:
        AtpGetProcessJob(Call, Result);
        break;
    case AtActionListHiddenProcesses:
        AtpListHiddenProcesses(Call, Result);
        break;
    case AtActionGetProcessKsiState:
        AtpGetProcessKsiState(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
