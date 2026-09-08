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
        PhAddJsonObjectBoolean(row, "is_microsoft_signed", AtIsMicrosoftSigned(ProcessItem->FileName));

    return row;
}

// Bytes (or events) per second from a per-run delta. The provider's interval is not the configured
// one when System Informer is throttling, so it is read rather than assumed, and reported alongside
// so the caller can see what the rates were divided by.
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

// A host process's own image says nothing about what it is running: svchost is a group, rundll32 is
// somebody else's entry point, dllhost is a COM object. That is the part worth reading.
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

// The parent as it was when this process started, which is the only honest way to name it: the pid
// on its own may since have been reused by something unrelated.
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

    // The record for the parent as it was at this process's start, which also covers a parent that
    // has since exited.
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

    // No record kept: fall back to the live process, but only when it could actually be the parent.
    // A parent that started after its child is a different process wearing a recycled pid, and
    // naming it would be worse than saying nothing.
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

// The Statistics tab's numbers, which need the process opened and so are only gathered on request.
// Anything that could not be read is null rather than zero: a zero working set or no GUI handles is
// itself a finding, and must not be manufactured by a failed query.
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

    // Pool charges and the page file charge are already on the item; only the working set breakdown
    // needs the process.
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

    // GDI and USER handles, which is how a leaking UI process is recognised. GetGuiResources is a
    // plain Win32 call, so no phlib export is involved.
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
    // Verified here rather than taken from the provider, which only fills verify_result in when the
    // signature stage is enabled and reports nothing at all when it is not.
    PhAddJsonObjectBoolean(Object, "is_microsoft_signed", AtIsMicrosoftSigned(ProcessItem->FileName));
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

    // What the image says about itself, which is the first thing a person reads and the first thing
    // an impostor gets wrong.
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

    // Import counts and the packed heuristic come from stage 2, which the user can turn off, and
    // ULONG_MAX is the provider's "could not read the image" sentinel. Both are null rather than a
    // zero that would read as "imports nothing", which is itself a finding.
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

    // How much of the image in memory still matches the file on disk. System Informer only computes
    // this when coherency support is on and the scan level is not zero; at level zero it marks the
    // status successful and leaves the value at zero, so trusting the status alone would report
    // every process on a default configuration as completely incoherent, which is what an injected
    // image looks like. Null unless it was really measured.
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

    // What changed in the last provider run, which is how to see what a process is doing now rather
    // than what it has done since it started.
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

    // Memory can be given back, and the delta is computed unsigned, so it wraps rather than going
    // negative: read it back as signed so a process releasing memory reports a fall, not a
    // nonsensical several exabytes.
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

    // Compared at the resolution the time was written in: a caller passing back a start_time it was
    // given means "after that process", so everything within that same millisecond is excluded.
    if (Filter->HaveStartedAfter &&
        ProcessItem->CreateTime.QuadPart < Filter->StartedAfter.QuadPart + PH_TICKS_PER_MS)
    {
        return FALSE;
    }

    if (Filter->ProtectedOnly && !ProcessItem->IsProtectedProcess)
        return FALSE;

    // The image the process was started from is no longer on disk: a process running from a
    // deleted file cannot be checked against anything, which is the reason to ask. A pseudo
    // process such as Registry or Memory Compression has a name where the path would be and no
    // image at all, which is not the same thing and is not what this asks for.
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

    // Last, and only when asked: each of these is a signature verification of the image on disk, so
    // every cheap filter above has already thrown away everything it can.
    if (Filter->ExcludeMicrosoft && AtIsMicrosoftSigned(ProcessItem->FileName))
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

        // Descendants: a process whose parent (by pid, and created after that parent so a
        // recycled parent pid does not adopt it) is matched.
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

            // Validated when the target was resolved.
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

    groupArray = PhCreateJsonArray();

    if (NT_SUCCESS(PhGetTokenGroups(tokenHandle, &groups)))
    {
        for (i = 0; i < groups->GroupCount; i++)
        {
            PVOID row = PhCreateJsonObject();

            AtpAddSidStrings(row, "name", "sid", groups->Groups[i].Sid);
            AtpAddGroupFlags(row, groups->Groups[i].Attributes);
            PhAddJsonArrayObject(groupArray, row);
        }

        PhFree(groups);
    }

    PhAddJsonObjectValue(structured, "groups", groupArray);

    privilegeArray = PhCreateJsonArray();

    if (NT_SUCCESS(PhGetTokenPrivileges(tokenHandle, &privileges)))
    {
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
    }

    PhAddJsonObjectValue(structured, "privileges", privilegeArray);
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

    PhEnumWindows(AtpWindowCallback, &context);

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, target.ProcessItem);
    AtAddRows(structured, "windows", &context.Windows);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteTarget(&target);
}

// The job a process belongs to. A job is how Windows puts a fence around a group of processes - a
// container, a sandbox, a service host, a browser's renderers - and the fence is what the limits
// say: how much memory, how many processes, what they may not do. Whether a process is in one is
// answerable by anyone; opening the job to read it needs the System Informer driver.

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

        // Each limit is only set when its flag is, so the rest are null rather than zero: a
        // maximum of zero processes and no maximum at all are not the same fence.
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

    // Anyone can ask whether a process is in a job; STATUS_PROCESS_NOT_IN_JOB is the answer "no"
    // rather than a failure to find out.
    if (target.ProcessHandle)
    {
        status = NtIsProcessInJob(target.ProcessHandle, NULL);
        isInJob = status == STATUS_PROCESS_IN_JOB;
    }

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, target.ProcessItem);
    PhAddJsonObjectBoolean(structured, "is_in_job", isInJob);

    // Reading the job itself means holding a handle to it, and there is no user-mode way to get one
    // from a process: the driver opens it. Without the driver the answer stops at is_in_job, and the
    // level is checked rather than the call attempted - KphCreateUserMessage asserts when there is
    // no connection, which in a debug build is a message box on a background thread.
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
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
