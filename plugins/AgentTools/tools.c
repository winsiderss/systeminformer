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

VOID AtSetToolError(
    _Inout_ PAT_TOOL_RESULT Result,
    _In_ PCSTR ErrorCode,
    _In_ NTSTATUS Status,
    _In_ PCWSTR Format,
    ...
    )
{
    va_list argptr;

    Result->ErrorCode = ErrorCode;
    Result->Status = Status;

    va_start(argptr, Format);
    PhMoveReference(&Result->ErrorMessage, PhFormatString_V(Format, argptr));
    va_end(argptr);
}

VOID AtSetToolStatusError(
    _Inout_ PAT_TOOL_RESULT Result,
    _In_ NTSTATUS Status,
    _In_ PCWSTR Operation
    )
{
    PPH_STRING message;

    message = PhGetStatusMessage(Status, 0);

    AtSetToolError(
        Result,
        Status == STATUS_ACCESS_DENIED ? "access_denied" : "failed",
        Status,
        L"%s failed: %s",
        Operation,
        PhGetStringOrDefault(message, L"unknown error")
        );

    PhClearReference(&message);
}

VOID AtDeleteToolResult(
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    if (Result->StructuredContent)
    {
        PhFreeJsonObject(Result->StructuredContent);
        Result->StructuredContent = NULL;
    }

    PhClearReference(&Result->ErrorMessage);
}

ULONG AtToolDefaultAccess(
    _In_ PCAT_TOOL Tool
    )
{
    UNREFERENCED_PARAMETER(Tool);

    return AT_ACCESS_ALLOWED;
}

ULONG AtToolDefaultConfirm(
    _In_ PCAT_TOOL Tool
    )
{
    return Tool->Tier == AtTierRead ? AT_CONFIRM_NONE : AT_CONFIRM_ALWAYS;
}

VOID AtRegisterToolSettings(
    VOID
    )
{
    static PWSTR values[] = { L"0", L"1", L"2" };
    ULONG i;

    for (i = 0; i < AtToolCount; i++)
    {
        PCAT_TOOL tool = &AtTools[i];
        PH_SETTING_CREATE settings[2];

        settings[0].Type = IntegerSettingType;
        settings[0].Name = tool->AccessSetting;
        settings[0].DefaultValue = values[AtToolDefaultAccess(tool)];
        settings[1].Type = IntegerSettingType;
        settings[1].Name = tool->ConfirmSetting;
        settings[1].DefaultValue = values[AtToolDefaultConfirm(tool)];

        PhAddSettings(settings, RTL_NUMBER_OF(settings));
    }
}

PCAT_TOOL AtFindTool(
    _In_ PPH_STRING Name
    )
{
    ULONG i;

    for (i = 0; i < AtToolCount; i++)
    {
        PPH_STRING toolName;
        BOOLEAN match;

        toolName = PhZeroExtendToUtf16(AtTools[i].Name);
        match = PhEqualString(Name, toolName, FALSE);
        PhDereferenceObject(toolName);

        if (match)
            return &AtTools[i];
    }

    return NULL;
}

BOOLEAN AtIsToolEnabled(
    _In_ PCAT_TOOL Tool
    )
{
    return PhGetIntegerSetting(Tool->AccessSetting) == AT_ACCESS_ALLOWED;
}

VOID AtEnumTools(
    _In_ PVOID ToolsArray
    )
{
    ULONG i;

    for (i = 0; i < AtToolCount; i++)
    {
        PVOID definition;

        if (!AtIsToolEnabled(&AtTools[i]))
            continue;

        if (NT_SUCCESS(PhCreateJsonParser(&definition, AtTools[i].Definition)))
            PhAddJsonArrayObject(ToolsArray, definition);
        else
            NT_ASSERT(FALSE); // a definition in schema.c does not parse
    }
}

//
// Shared helpers
//

VOID AtAddSnapshot(
    _In_ PVOID Object
    )
{
    LARGE_INTEGER time;

    if (PhGetStatisticsTime(NULL, 0, &time))
        AtJsonAddTime(Object, "snapshot_time", &time);
    else
        AtJsonAddNull(Object, "snapshot_time");

    PhAddJsonObjectBoolean(Object, "updates_paused", !SystemInformer_GetUpdateAutomatically());
}

BOOLEAN AtGetArgumentUInt64(
    _In_opt_ PVOID Arguments,
    _In_ PCSTR Key,
    _Out_ PULONG64 Value
    )
{
    PVOID member;

    if (!Arguments || !(member = PhGetJsonObject(Arguments, Key)))
        return FALSE;

    if (PhGetJsonObjectType(member) != PH_JSON_OBJECT_TYPE_INT)
        return FALSE;

    *Value = (ULONG64)PhGetJsonInt64Object(member);
    return TRUE;
}

/**
 * Reads an argument that names an address or handle: an integer, or a string in hexadecimal
 * (with or without 0x) or decimal.
 */
BOOLEAN AtGetArgumentPointer(
    _In_opt_ PVOID Arguments,
    _In_ PCSTR Key,
    _Out_ PULONG64 Value
    )
{
    PVOID member;
    PPH_STRING string;
    PH_STRINGREF sr;
    ULONG64 value;
    BOOLEAN result;

    if (!Arguments || !(member = PhGetJsonObject(Arguments, Key)))
        return FALSE;

    if (PhGetJsonObjectType(member) == PH_JSON_OBJECT_TYPE_INT)
    {
        *Value = (ULONG64)PhGetJsonInt64Object(member);
        return TRUE;
    }

    if (PhGetJsonObjectType(member) != PH_JSON_OBJECT_TYPE_STRING)
        return FALSE;

    if (!(string = PhGetJsonValueAsString(Arguments, Key)))
        return FALSE;

    sr = string->sr;

    if (PhStartsWithStringRef2(&sr, L"0x", TRUE))
    {
        PhSkipStringRef(&sr, 2 * sizeof(WCHAR));
        result = PhStringToUInt64(&sr, 16, &value);
    }
    else
    {
        result = PhStringToUInt64(&sr, 10, &value);
    }

    PhDereferenceObject(string);

    if (result)
        *Value = value;

    return result;
}

PPH_STRING AtGetArgumentString(
    _In_opt_ PVOID Arguments,
    _In_ PCSTR Key
    )
{
    if (!Arguments || !AtJsonGetObjectMember(Arguments, Key, PH_JSON_OBJECT_TYPE_STRING))
        return NULL;

    return PhGetJsonValueAsString(Arguments, Key);
}

VOID AtJsonAddStringZ(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_opt_ PCWSTR String
    )
{
    PH_STRINGREF sr;

    if (!String)
    {
        AtJsonAddNull(Object, Key);
        return;
    }

    PhInitializeStringRef(&sr, String);
    AtJsonAddStringRef(Object, Key, &sr);
}

VOID AtJsonAddPointer(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_opt_ PVOID Pointer
    )
{
    PH_FORMAT format[2];
    PPH_STRING string;

    if (!Pointer)
    {
        AtJsonAddNull(Object, Key);
        return;
    }

    PhInitFormatS(&format[0], L"0x");
    PhInitFormatIX(&format[1], (ULONG_PTR)Pointer);
    string = PhFormat(format, RTL_NUMBER_OF(format), 24);
    AtJsonAddString(Object, Key, string);
    PhDereferenceObject(string);
}

VOID AtJsonAddWin32FileName(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_opt_ PPH_STRING NativeFileName
    )
{
    PPH_STRING fileName;

    if (!NativeFileName)
    {
        AtJsonAddNull(Object, Key);
        return;
    }

    fileName = PhGetFileName(NativeFileName);
    AtJsonAddString(Object, Key, fileName);
    PhClearReference(&fileName);
}

/**
 * Adds a duration in 100 ns units as seconds.
 */
VOID AtJsonAddDuration(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ ULONG64 Duration100ns
    )
{
    PhAddJsonObjectDouble(Object, Key, (DOUBLE)Duration100ns / (DOUBLE)PH_TICKS_PER_SEC);
}

BOOLEAN AtContainsString(
    _In_opt_ PPH_STRING String,
    _In_opt_ PPH_STRING Needle
    )
{
    if (!Needle)
        return TRUE;
    if (!String)
        return FALSE;

    return PhFindStringInStringRef(&String->sr, &Needle->sr, TRUE) != SIZE_MAX;
}

PCWSTR AtVerifyResultString(
    _In_ VERIFY_RESULT Result
    )
{
    switch (Result)
    {
    case VrNoSignature:
        return L"No signature";
    case VrTrusted:
        return L"Trusted";
    case VrExpired:
        return L"Expired certificate";
    case VrRevoked:
        return L"Revoked certificate";
    case VrDistrust:
        return L"Not trusted";
    case VrSecuritySettings:
        return L"Security policy failure";
    case VrBadSignature:
        return L"Invalid hash";
    }

    return NULL;
}

PCWSTR AtIoPriorityString(
    _In_ IO_PRIORITY_HINT IoPriority
    )
{
    switch (IoPriority)
    {
    case IoPriorityVeryLow:
        return L"Very low";
    case IoPriorityLow:
        return L"Low";
    case IoPriorityNormal:
        return L"Normal";
    case IoPriorityHigh:
        return L"High";
    case IoPriorityCritical:
        return L"Critical";
    }

    return NULL;
}

PCWSTR AtPriorityClassString(
    _In_ ULONG PriorityClass
    )
{
    switch (PriorityClass)
    {
    case PROCESS_PRIORITY_CLASS_IDLE:
        return L"Idle";
    case PROCESS_PRIORITY_CLASS_BELOW_NORMAL:
        return L"Below normal";
    case PROCESS_PRIORITY_CLASS_NORMAL:
        return L"Normal";
    case PROCESS_PRIORITY_CLASS_ABOVE_NORMAL:
        return L"Above normal";
    case PROCESS_PRIORITY_CLASS_HIGH:
        return L"High";
    case PROCESS_PRIORITY_CLASS_REALTIME:
        return L"Real time";
    }

    return NULL;
}

BOOLEAN AtParsePriorityClass(
    _In_opt_ PPH_STRING String,
    _Out_ PULONG PriorityClass
    )
{
    static CONST struct { PCWSTR Name; ULONG Value; } table[] =
    {
        { L"idle", PROCESS_PRIORITY_CLASS_IDLE },
        { L"below_normal", PROCESS_PRIORITY_CLASS_BELOW_NORMAL },
        { L"normal", PROCESS_PRIORITY_CLASS_NORMAL },
        { L"above_normal", PROCESS_PRIORITY_CLASS_ABOVE_NORMAL },
        { L"high", PROCESS_PRIORITY_CLASS_HIGH },
        { L"realtime", PROCESS_PRIORITY_CLASS_REALTIME },
    };
    ULONG i;

    if (!String)
        return FALSE;

    for (i = 0; i < RTL_NUMBER_OF(table); i++)
    {
        if (PhEqualStringZ(String->Buffer, table[i].Name, TRUE))
        {
            *PriorityClass = table[i].Value;
            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN AtParseIoPriority(
    _In_opt_ PPH_STRING String,
    _Out_ IO_PRIORITY_HINT* IoPriority
    )
{
    static CONST struct { PCWSTR Name; IO_PRIORITY_HINT Value; } table[] =
    {
        { L"very_low", IoPriorityVeryLow },
        { L"low", IoPriorityLow },
        { L"normal", IoPriorityNormal },
        { L"high", IoPriorityHigh },
    };
    ULONG i;

    if (!String)
        return FALSE;

    for (i = 0; i < RTL_NUMBER_OF(table); i++)
    {
        if (PhEqualStringZ(String->Buffer, table[i].Name, TRUE))
        {
            *IoPriority = table[i].Value;
            return TRUE;
        }
    }

    return FALSE;
}

static PCWSTR AtpElevationTypeString(
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

static PCWSTR AtpProtectionString(
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

static PCWSTR AtpNativeArchitectureString(
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

VOID AtFillProcessIdentity(
    _In_ PVOID Object,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PhAddJsonObjectUInt64(Object, "pid", HandleToUlong(ProcessItem->ProcessId));
    PhAddJsonObjectUInt64(Object, "process_sequence_number", ProcessItem->ProcessSequenceNumber);
    AtJsonAddString(Object, "name", ProcessItem->ProcessName);
}

static VOID AtpAddParentPid(
    _In_ PVOID Object,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    if (ProcessItem->ParentProcessId)
        PhAddJsonObjectUInt64(Object, "parent_pid", HandleToUlong(ProcessItem->ParentProcessId));
    else
        AtJsonAddNull(Object, "parent_pid");
}

//
// Processes
//

static PVOID AtpCreateProcessRow(
    _In_ PPH_PROCESS_ITEM ProcessItem
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

    return row;
}

static VOID AtpFillProcessDetail(
    _In_ PVOID Object,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
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

    if (ProcessItem->ConsoleHostProcessId)
        PhAddJsonObjectUInt64(Object, "console_host_pid", HandleToUlong(ProcessItem->ConsoleHostProcessId));
    else
        AtJsonAddNull(Object, "console_host_pid");

    AtJsonAddStringZ(Object, "priority_class", AtPriorityClassString(ProcessItem->PriorityClass));
    PhAddJsonObjectInt64(Object, "base_priority", ProcessItem->BasePriority);
    PhAddJsonObjectDouble(Object, "cpu_usage", ProcessItem->CpuUsage);
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
    PPH_STRING NameContains;
    PPH_STRING UserContains;
    PVOID Pids; // JSON array or NULL
    BOOLEAN HaveParentPid;
    HANDLE ParentPid;
    BOOLEAN IncludeTree;
} AT_LIST_FILTER, *PAT_LIST_FILTER;

static BOOLEAN AtpMatchesFilter(
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

static VOID AtpListProcesses(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    AT_LIST_FILTER filter;
    PPH_PROCESS_ITEM* processItems;
    ULONG numberOfProcessItems;
    PBOOLEAN matched;
    PVOID structured;
    PVOID rows;
    ULONG count = 0;
    ULONG i;
    ULONG64 parentPid;

    memset(&filter, 0, sizeof(AT_LIST_FILTER));

    if (Call->Arguments)
    {
        filter.NameContains = AtGetArgumentString(Call->Arguments, "name_contains");
        filter.UserContains = AtGetArgumentString(Call->Arguments, "user_contains");
        filter.Pids = AtJsonGetObjectMember(Call->Arguments, "pids", PH_JSON_OBJECT_TYPE_ARRAY);
        filter.IncludeTree = AtJsonGetObjectBoolean(Call->Arguments, "include_tree");

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
    rows = PhCreateJsonArray();

    for (i = 0; i < numberOfProcessItems; i++)
    {
        if (!matched[i])
            continue;

        PhAddJsonArrayObject(rows, AtpCreateProcessRow(processItems[i]));
        count++;
    }

    PhAddJsonObjectValue(structured, "processes", rows);
    PhAddJsonObjectUInt64(structured, "count", count);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    for (i = 0; i < numberOfProcessItems; i++)
        PhDereferenceObject(processItems[i]);

    PhFree(processItems);
    PhFree(matched);
    PhClearReference(&filter.NameContains);
    PhClearReference(&filter.UserContains);
}

static VOID AtpGetProcess(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    AT_TARGET target;
    PVOID structured;

    if (!NT_SUCCESS(AtResolveProcessTarget(Call->Arguments, FALSE, 0, &target, Result)))
        return;

    structured = PhCreateJsonObject();
    AtpFillProcessDetail(structured, target.ProcessItem);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteTarget(&target);
}

static VOID AtpGetProcessEnvironment(
    _In_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    PVOID environment;
    ULONG environmentLength;
    ULONG enumerationKey;
    PH_ENVIRONMENT_VARIABLE variable;
    PVOID structured;
    PVOID variables;
    ULONG count = 0;

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
    variables = PhCreateJsonArray();

    enumerationKey = 0;

    while (NT_SUCCESS(PhEnumProcessEnvironmentVariables(environment, environmentLength, &enumerationKey, &variable)))
    {
        PVOID entry;

        entry = PhCreateJsonObject();
        AtJsonAddStringRef(entry, "name", &variable.Name);
        AtJsonAddStringRef(entry, "value", &variable.Value);
        PhAddJsonArrayObject(variables, entry);
        count++;
    }

    PhFreePage(environment);

    PhAddJsonObjectValue(structured, "variables", variables);
    PhAddJsonObjectUInt64(structured, "count", count);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

static VOID AtpControlProcess(
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
        AtpGetProcessEnvironment(Target, Result);
        break;
    case AtActionTerminateProcess:
    case AtActionSuspendProcess:
    case AtActionResumeProcess:
    case AtActionSetProcessPriority:
    case AtActionSetProcessIoPriority:
        AtpControlProcess(Tool, Call, Target, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}

//
// Dispatch
//

VOID AtInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PCAT_ACTION_INFO action = &AtActionInfo[Tool->Action];

    switch (Tool->Action)
    {
    case AtActionListProcesses:
    case AtActionGetProcess:
    case AtActionReadProcessEnvironment:
    case AtActionTerminateProcess:
    case AtActionSuspendProcess:
    case AtActionResumeProcess:
    case AtActionSetProcessPriority:
    case AtActionSetProcessIoPriority:
        AtProcessInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionGetProcessThreads:
    case AtActionGetThreadStack:
    case AtActionSuspendThread:
    case AtActionResumeThread:
    case AtActionTerminateThread:
        AtThreadInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionGetProcessModules:
    case AtActionGetProcessHandles:
    case AtActionGetProcessHandlesDetailed:
    case AtActionGetProcessMemoryRegions:
    case AtActionCreateProcessMinidump:
    case AtActionCloseHandle:
        AtMemoryInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionListServices:
    case AtActionGetService:
    case AtActionStartService:
    case AtActionStopService:
    case AtActionRestartService:
    case AtActionSetServiceConfig:
        AtServiceInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionListNetworkConnections:
    case AtActionCloseNetworkConnection:
        AtNetworkInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionGetSystemInfo:
    case AtActionGetProcessToken:
    case AtActionGetProcessWindows:
    case AtActionListKernelDrivers:
        AtSystemInvokeTool(Tool, Call, Target, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }

    if (action->Tier != AtTierRead && Target->Kind != AtTargetNone)
    {
        AtAudit(
            Call->Connection,
            action,
            Target,
            Result->ErrorCode ? PhaFormatString(L"failed (%S)", Result->ErrorCode)->Buffer : L"succeeded"
            );
    }
}
