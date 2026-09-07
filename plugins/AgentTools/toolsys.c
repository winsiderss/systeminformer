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

static PCWSTR AtpKphLevelString(
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

static VOID AtpGetSystemInfo(
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

//
// Process token
//

static VOID AtpAddSidStrings(
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

static VOID AtpAddGroupFlags(
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

static ULONG AtpQueryTokenUlong(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_INFORMATION_CLASS InfoClass
    )
{
    ULONG value = 0;
    ULONG returnLength;

    NtQueryInformationToken(TokenHandle, InfoClass, &value, sizeof(value), &returnLength);

    return value;
}

static VOID AtpGetProcessToken(
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

            if (PhLookupPrivilegeName(&privileges->Privileges[i].Luid, &name))
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

//
// Process windows
//

typedef struct _AT_WINDOW_CONTEXT
{
    HANDLE ProcessId;
    BOOLEAN VisibleOnly;
    PVOID Windows;
    ULONG Count;
} AT_WINDOW_CONTEXT, *PAT_WINDOW_CONTEXT;

_Function_class_(PH_WINDOW_ENUM_CALLBACK)
static BOOLEAN NTAPI AtpWindowCallback(
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

    PhAddJsonArrayObject(context->Windows, row);
    context->Count++;

    return TRUE;
}

static VOID AtpGetProcessWindows(
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
    context.Windows = PhCreateJsonArray();

    // visible_only defaults to true unless the caller passes false.
    if (visibleMember = AtJsonGetObjectMember(Call->Arguments, "visible_only", PH_JSON_OBJECT_TYPE_BOOLEAN))
        context.VisibleOnly = AtJsonGetObjectBoolean(Call->Arguments, "visible_only");

    PhEnumWindows(AtpWindowCallback, &context);

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, target.ProcessItem);
    PhAddJsonObjectValue(structured, "windows", context.Windows);
    PhAddJsonObjectUInt64(structured, "count", context.Count);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteTarget(&target);
}

//
// Kernel drivers
//

static VOID AtpListKernelDrivers(
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

static VOID AtpGetKsiStatus(
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
    case AtActionGetProcessToken:
        AtpGetProcessToken(Call, Result);
        break;
    case AtActionGetProcessWindows:
        AtpGetProcessWindows(Call, Result);
        break;
    case AtActionListKernelDrivers:
        AtpListKernelDrivers(Call, Result);
        break;
    case AtActionGetKsiStatus:
        AtpGetKsiStatus(Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
