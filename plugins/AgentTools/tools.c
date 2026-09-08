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

VOID AtSetToolHint(
    _Inout_ PAT_TOOL_RESULT Result,
    _In_ ULONG Hints
    )
{
    SetFlag(Result->Hints, Hints);
}

BOOLEAN AtpIsConsentError(
    _In_opt_ PCSTR ErrorCode
    )
{
    static CONST PCSTR codes[] =
    {
        "disabled",
        "consent_denied",
        "consent_timeout",
        "consent_declined",
        "consent_failed",
        "elicitation_required",
    };
    ULONG i;

    if (!ErrorCode)
        return FALSE;

    for (i = 0; i < RTL_NUMBER_OF(codes); i++)
    {
        if (strcmp(ErrorCode, codes[i]) == 0)
            return TRUE;
    }

    return FALSE;
}

VOID AtAddErrorHints(
    _In_ PVOID Error,
    _In_ PAT_TOOL_RESULT Result
    )
{
    KPH_LEVEL level = KsiLevel();
    ULONG hints = Result->Hints;

    if (AtpIsConsentError(Result->ErrorCode))
        SetFlag(hints, AT_HINT_CONSENT_REQUIRED);

    if (Result->Status == STATUS_ACCESS_DENIED || Result->Status == STATUS_PRIVILEGE_NOT_HELD)
    {
        if (!PhGetOwnTokenAttributes().Elevated)
            SetFlag(hints, AT_HINT_NEEDS_ELEVATION);

        // Only when the driver is absent entirely: a connected driver already gave what it can,
        // and the level a plugin-loaded instance gets is capped by design.
        if (level == KphLevelNone)
            SetFlag(hints, AT_HINT_NEEDS_DRIVER);
    }

    switch (Result->Status)
    {
    case STATUS_INSUFFICIENT_RESOURCES:
    case STATUS_NO_MEMORY:
    case STATUS_TIMEOUT:
    case STATUS_RETRY:
        SetFlag(hints, AT_HINT_RETRYABLE);
        break;
    }

    // The user was asked and did not answer in time; asking again can still succeed.
    if (Result->ErrorCode && strcmp(Result->ErrorCode, "consent_timeout") == 0)
        SetFlag(hints, AT_HINT_RETRYABLE);

    if (FlagOn(hints, AT_HINT_NEEDS_ELEVATION))
        PhAddJsonObjectBoolean(Error, "needs_elevation", TRUE);

    if (FlagOn(hints, AT_HINT_NEEDS_DRIVER))
    {
        PhAddJsonObjectBoolean(Error, "needs_driver", TRUE);
        AtJsonAddStringZ(Error, "ksi_level", AtKphLevelString(level));
    }

    if (FlagOn(hints, AT_HINT_CONSENT_REQUIRED))
        PhAddJsonObjectBoolean(Error, "consent_required", TRUE);

    if (FlagOn(hints, AT_HINT_PLUGIN_MISSING))
        PhAddJsonObjectBoolean(Error, "plugin_missing", TRUE);

    if (FlagOn(hints, AT_HINT_RETRYABLE))
        PhAddJsonObjectBoolean(Error, "retryable", TRUE);
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
        // Everything but a plain read is asked about, network egress included.
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

_Success_(return)
BOOLEAN AtInitializeBatch(
    _Out_ PAT_BATCH Batch,
    _In_opt_ PVOID Arguments,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    memset(Batch, 0, sizeof(AT_BATCH));

    if (!(Batch->Pids = AtJsonGetObjectMember(Arguments, "pids", PH_JSON_OBJECT_TYPE_ARRAY)))
        return TRUE;

    Batch->Count = PhGetJsonArrayLength(Batch->Pids);

    if (Batch->Count == 0)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"pids must not be empty.");
        return FALSE;
    }

    if (Batch->Count > AT_MAX_BATCH_PIDS)
    {
        AtSetToolError(
            Result,
            "invalid_arguments",
            STATUS_INVALID_PARAMETER,
            L"pids holds %lu entries; at most %lu can be asked about in one call.",
            Batch->Count,
            (ULONG)AT_MAX_BATCH_PIDS
            );
        return FALSE;
    }

    Batch->Summary = AtJsonGetObjectBoolean(Arguments, "summary");

    return TRUE;
}

PPH_PROCESS_ITEM AtBatchReferenceProcessItem(
    _In_ PAT_BATCH Batch,
    _In_ ULONG Index,
    _Out_ PULONG ProcessId
    )
{
    ULONG64 processId;

    processId = (ULONG64)PhGetJsonArrayLong64(Batch->Pids, Index);

    if (processId > MAXULONG)
    {
        *ProcessId = 0;
        return NULL;
    }

    *ProcessId = (ULONG)processId;

    return PhReferenceProcessItem(UlongToHandle((ULONG)processId));
}

PVOID AtCreateBatchError(
    _In_ ULONG ProcessId,
    _In_ PCSTR ErrorCode,
    _In_ PCWSTR Message
    )
{
    PVOID entry;

    entry = PhCreateJsonObject();
    PhAddJsonObjectUInt64(entry, "pid", ProcessId);
    PhAddJsonObject(entry, "error", ErrorCode);
    AtJsonAddStringZ(entry, "message", Message);

    return entry;
}

PVOID AtCreateBatchResult(
    _In_ PVOID Results
    )
{
    PVOID structured;

    structured = PhCreateJsonObject();
    PhAddJsonObjectValue(structured, "results", Results);
    PhAddJsonObjectUInt64(structured, "result_count", PhGetJsonArrayLength(Results));
    AtAddSnapshot(structured);

    return structured;
}

// Paging and sorting for the list tools.

typedef enum _AT_SORT_RANK
{
    AtSortRankNull,
    AtSortRankBoolean,
    AtSortRankNumber,
    AtSortRankString,
    AtSortRankOther
} AT_SORT_RANK;

typedef struct _AT_ROW_SORT_ENTRY
{
    PVOID Row;
    ULONG Index;
    AT_SORT_RANK Rank;
    BOOLEAN Integral;
    LONG64 Integer;
    DOUBLE Double;
    PPH_STRING String;
} AT_ROW_SORT_ENTRY, *PAT_ROW_SORT_ENTRY;

VOID AtInitializeRows(
    _Out_ PAT_ROWS Rows,
    _In_opt_ PVOID Arguments
    )
{
    ULONG64 value;

    memset(Rows, 0, sizeof(AT_ROWS));

    Rows->Rows = PhCreateList(64);
    Rows->Limit = AT_ROWS_DEFAULT_LIMIT;

    if (AtGetArgumentUInt64(Arguments, "limit", &value) && value != 0)
        Rows->Limit = (ULONG)min(value, AT_ROWS_MAXIMUM_LIMIT);

    if (AtGetArgumentUInt64(Arguments, "offset", &value))
        Rows->Offset = (ULONG)min(value, MAXULONG);

    Rows->SortBy = AtGetArgumentString(Arguments, "sort_by");
    Rows->Descending = AtJsonGetObjectBoolean(Arguments, "descending");
}

// Row is optional: a counts-only mode counts the match without building a row for it.
VOID AtAddRow(
    _Inout_ PAT_ROWS Rows,
    _In_opt_ PVOID Row
    )
{
    Rows->TotalCount++;

    if (Row)
        PhAddItemList(Rows->Rows, Row);
}

VOID AtpInitializeSortEntry(
    _Out_ PAT_ROW_SORT_ENTRY Entry,
    _In_ PVOID Row,
    _In_ ULONG Index,
    _In_ PCSTR Key
    )
{
    PVOID member;

    memset(Entry, 0, sizeof(AT_ROW_SORT_ENTRY));
    Entry->Row = Row;
    Entry->Index = Index;
    Entry->Rank = AtSortRankNull;

    if (!(member = PhGetJsonObject(Row, Key)))
        return;

    switch (PhGetJsonObjectType(member))
    {
    case PH_JSON_OBJECT_TYPE_NULL:
        break;
    case PH_JSON_OBJECT_TYPE_BOOLEAN:
        Entry->Rank = AtSortRankBoolean;
        Entry->Integral = TRUE;
        Entry->Integer = PhGetJsonInt64Object(member);
        break;
    case PH_JSON_OBJECT_TYPE_INT:
        Entry->Rank = AtSortRankNumber;
        Entry->Integral = TRUE;
        Entry->Integer = PhGetJsonInt64Object(member);
        Entry->Double = (DOUBLE)Entry->Integer;
        break;
    case PH_JSON_OBJECT_TYPE_DOUBLE:
        Entry->Rank = AtSortRankNumber;
        Entry->Double = PhGetJsonDoubleObject(member);
        break;
    case PH_JSON_OBJECT_TYPE_STRING:
        Entry->Rank = AtSortRankString;
        Entry->String = PhGetJsonObjectString(member);
        break;
    default:
        Entry->Rank = AtSortRankOther;
        break;
    }
}

int __cdecl AtpCompareRowSortEntries(
    _In_ void* Context,
    _In_ const void* Elem1,
    _In_ const void* Elem2
    )
{
    PAT_ROWS rows = Context;
    PAT_ROW_SORT_ENTRY entry1 = (PAT_ROW_SORT_ENTRY)Elem1;
    PAT_ROW_SORT_ENTRY entry2 = (PAT_ROW_SORT_ENTRY)Elem2;
    int result;

    if (entry1->Rank != entry2->Rank)
    {
        result = entry1->Rank < entry2->Rank ? -1 : 1;
    }
    else
    {
        switch (entry1->Rank)
        {
        case AtSortRankBoolean:
            result = int64cmp(entry1->Integer, entry2->Integer);
            break;
        case AtSortRankNumber:
            if (entry1->Integral && entry2->Integral)
                result = int64cmp(entry1->Integer, entry2->Integer);
            else if (entry1->Double < entry2->Double)
                result = -1;
            else if (entry1->Double > entry2->Double)
                result = 1;
            else
                result = 0;
            break;
        case AtSortRankString:
            result = PhCompareString(entry1->String, entry2->String, TRUE);
            break;
        default:
            result = 0;
            break;
        }
    }

    if (rows->Descending)
        result = -result;

    // Ties keep enumeration order in both directions, so paging is deterministic.
    if (result == 0)
        result = uintcmp(entry1->Index, entry2->Index);

    return result;
}

VOID AtpSortRows(
    _Inout_ PAT_ROWS Rows
    )
{
    PPH_BYTES key;
    PAT_ROW_SORT_ENTRY entries;
    ULONG i;

    key = PhConvertUtf16ToUtf8Ex(Rows->SortBy->Buffer, Rows->SortBy->Length);
    entries = PhAllocate(Rows->Rows->Count * sizeof(AT_ROW_SORT_ENTRY));

    for (i = 0; i < Rows->Rows->Count; i++)
        AtpInitializeSortEntry(&entries[i], Rows->Rows->Items[i], i, key->Buffer);

    qsort_s(entries, Rows->Rows->Count, sizeof(AT_ROW_SORT_ENTRY), AtpCompareRowSortEntries, Rows);

    for (i = 0; i < Rows->Rows->Count; i++)
    {
        Rows->Rows->Items[i] = entries[i].Row;
        PhClearReference(&entries[i].String);
    }

    PhFree(entries);
    PhDereferenceObject(key);
}

VOID AtAddRows(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _Inout_ PAT_ROWS Rows
    )
{
    PVOID array;
    ULONG count = 0;
    ULONG i;

    if (Rows->SortBy && Rows->Rows->Count > 1)
        AtpSortRows(Rows);

    array = PhCreateJsonArray();

    for (i = 0; i < Rows->Rows->Count; i++)
    {
        if (i < Rows->Offset || count >= Rows->Limit)
        {
            PhFreeJsonObject(Rows->Rows->Items[i]);
            continue;
        }

        PhAddJsonArrayObject(array, Rows->Rows->Items[i]);
        count++;
    }

    PhAddJsonObjectValue(Object, Key, array);
    PhAddJsonObjectUInt64(Object, "count", count);
    PhAddJsonObjectUInt64(Object, "total_count", Rows->TotalCount);
    PhAddJsonObjectUInt64(Object, "offset", Rows->Offset);
    PhAddJsonObjectUInt64(Object, "limit", Rows->Limit);
    PhAddJsonObjectBoolean(Object, "truncated", (ULONG64)Rows->Offset + count < Rows->Rows->Count);

    PhClearReference(&Rows->Rows);
    PhClearReference(&Rows->SortBy);
}

VOID AtDeleteRows(
    _Inout_ PAT_ROWS Rows
    )
{
    ULONG i;

    if (Rows->Rows)
    {
        for (i = 0; i < Rows->Rows->Count; i++)
            PhFreeJsonObject(Rows->Rows->Items[i]);

        PhClearReference(&Rows->Rows);
    }

    PhClearReference(&Rows->SortBy);
}

VOID AtAddSnapshot(
    _In_ PVOID Object
    )
{
    LARGE_INTEGER time;

    PhAddJsonObjectUInt64(Object, "snapshot_id", AtGetSnapshotId());

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

VOID AtJsonAddHex(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ ULONG64 Value
    )
{
    PH_FORMAT format[2];
    PPH_STRING string;

    PhInitFormatS(&format[0], L"0x");
    PhInitFormatI64X(&format[1], Value);
    string = PhFormat(format, RTL_NUMBER_OF(format), 24);
    AtJsonAddString(Object, Key, string);
    PhDereferenceObject(string);
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

PCWSTR AtKphLevelString(
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

PCWSTR AtTierString(
    _In_ AT_TIER Tier
    )
{
    switch (Tier)
    {
    case AtTierRead:
        return L"Read";
    case AtTierSensitiveRead:
        return L"Sensitive read";
    case AtTierWrite:
        return L"Write";
    case AtTierNetworkEgress:
        return L"Network egress";
    }

    return NULL;
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

    if (action->Tier != AtTierRead && (Target->Kind != AtTargetNone || action->Tier == AtTierNetworkEgress))
    {
        AtAudit(
            Call->Connection,
            action,
            Target,
            Result->ErrorCode ? PhaFormatString(L"failed (%S)", Result->ErrorCode)->Buffer : L"succeeded"
            );
    }
}
