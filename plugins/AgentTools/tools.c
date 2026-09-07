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
    switch (Tool->Action)
    {
    // Reads that describe the kernel or fingerprint the machine are asked about by default.
    case AtActionListKernelDrivers:
    case AtActionGetKsiStatus:
    case AtActionGetSmbiosInfo:
    case AtActionGetUefiVariables:
    case AtActionGetTpmInfo:
        return AT_CONFIRM_ALWAYS;
    default:
        return Tool->Tier == AtTierRead ? AT_CONFIRM_NONE : AT_CONFIRM_ALWAYS;
    }
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

VOID AtFillProcessIdentity(
    _In_ PVOID Object,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PhAddJsonObjectUInt64(Object, "pid", HandleToUlong(ProcessItem->ProcessId));
    PhAddJsonObjectUInt64(Object, "process_sequence_number", ProcessItem->ProcessSequenceNumber);
    AtJsonAddString(Object, "name", ProcessItem->ProcessName);
}

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
    case AtActionGetProcessToken:
    case AtActionGetProcessWindows:
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
        AtModuleInvokeTool(Tool, Call, Target, Result);
        break;
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
    case AtActionListKernelDrivers:
    case AtActionGetKsiStatus:
    case AtActionGetPagefileInfo:
    case AtActionListStartupEntries:
    case AtActionGetSmbiosInfo:
    case AtActionGetUefiVariables:
    case AtActionGetTpmInfo:
    case AtActionGetSystemEnvironment:
        AtSystemInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionVerifyFileSignature:
    case AtActionGetImageInfo:
    case AtActionReadProcessMemory:
    case AtActionSearchProcessMemory:
        AtPeInvokeTool(Tool, Call, Target, Result);
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
