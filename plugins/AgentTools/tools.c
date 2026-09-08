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

PEXTENDEDTOOLS_INTERFACE AtGetExtendedToolsInterface(
    VOID
    )
{
    static PEXTENDEDTOOLS_INTERFACE pluginInterface = NULL;
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        PPH_PLUGIN plugin;

        if (plugin = PhFindPlugin(EXTENDEDTOOLS_PLUGIN_NAME))
        {
            pluginInterface = PhGetPluginInformation(plugin)->Interface;

            if (pluginInterface && pluginInterface->Version < EXTENDEDTOOLS_INTERFACE_VERSION)
                pluginInterface = NULL;
        }

        PhEndInitOnce(&initOnce);
    }

    return pluginInterface;
}

// A per-second rate from a delta and the interval it covers. Null when the interval is unknown,
// because a rate divided by a guess is worse than no rate.
VOID AtAddRate(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ ULONG64 Delta,
    _In_ ULONG IntervalMs
    )
{
    if (IntervalMs == 0)
    {
        AtJsonAddNull(Object, Key);
        return;
    }

    PhAddJsonObjectDouble(Object, Key, (DOUBLE)Delta * 1000.0 / IntervalMs);
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

PCAT_PROMPT AtFindPrompt(
    _In_ PPH_STRING Name
    )
{
    ULONG i;

    for (i = 0; i < AtPromptCount; i++)
    {
        PPH_STRING name;
        BOOLEAN match;

        name = PhZeroExtendToUtf16(AtPrompts[i].Name);
        match = PhEqualString(Name, name, FALSE);
        PhDereferenceObject(name);

        if (match)
            return &AtPrompts[i];
    }

    return NULL;
}

VOID AtEnumPrompts(
    _In_ PVOID PromptsArray
    )
{
    ULONG i;

    for (i = 0; i < AtPromptCount; i++)
    {
        PVOID definition;

        if (NT_SUCCESS(PhCreateJsonParser(&definition, AtPrompts[i].Definition)))
            PhAddJsonArrayObject(PromptsArray, definition);
        else
            NT_ASSERT(FALSE); // a definition in schema.c does not parse
    }
}

PCAT_RESOURCE AtFindResource(
    _In_ PPH_STRING Uri
    )
{
    ULONG i;

    for (i = 0; i < AtResourceCount; i++)
    {
        PPH_STRING uri;
        BOOLEAN match;

        uri = PhZeroExtendToUtf16(AtResources[i].Uri);
        match = PhEqualString(Uri, uri, FALSE);
        PhDereferenceObject(uri);

        if (match)
            return &AtResources[i];
    }

    return NULL;
}

BOOLEAN AtIsToolEnabled(
    _In_ PCAT_TOOL Tool
    )
{
    return PhGetIntegerSetting(Tool->AccessSetting) == AT_ACCESS_ALLOWED;
}

// A resource whose tool is turned off is not listed: the resource is that tool's answer under
// another name, and offering it would be offering a way around the setting.
VOID AtEnumResources(
    _In_ PVOID ResourcesArray
    )
{
    ULONG i;

    for (i = 0; i < AtResourceCount; i++)
    {
        PPH_STRING name;
        PCAT_TOOL tool;
        PVOID definition;

        name = PhZeroExtendToUtf16(AtResources[i].ToolName);
        tool = AtFindTool(name);
        PhDereferenceObject(name);

        if (!tool || !AtIsToolEnabled(tool))
            continue;

        if (NT_SUCCESS(PhCreateJsonParser(&definition, AtResources[i].Definition)))
            PhAddJsonArrayObject(ResourcesArray, definition);
        else
            NT_ASSERT(FALSE); // a definition in schema.c does not parse
    }
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

PCWSTR AtStringEncodingString(
    _In_ PH_STRING_SEARCH_ENCODING Encoding
    )
{
    switch (Encoding)
    {
    case PH_STRING_SEARCH_ENCODING_ANSI:
        return L"ansi";
    case PH_STRING_SEARCH_ENCODING_UTF8:
        return L"utf8";
    case PH_STRING_SEARCH_ENCODING_UTF16:
        return L"utf16";
    }

    return NULL;
}

// The string search finds all three encodings at once and reports which one each result was; the
// argument narrows what is kept rather than what is looked for.
_Success_(return)
BOOLEAN AtGetArgumentEncoding(
    _In_opt_ PVOID Arguments,
    _Out_ PPH_STRING_SEARCH_ENCODING Encoding,
    _Out_ PBOOLEAN HaveEncoding,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PPH_STRING encoding;

    *Encoding = PH_STRING_SEARCH_ENCODING_ANSI;
    *HaveEncoding = FALSE;

    if (!(encoding = AtGetArgumentString(Arguments, "encoding")))
        return TRUE;

    if (PhEqualString2(encoding, L"ansi", TRUE))
        *Encoding = PH_STRING_SEARCH_ENCODING_ANSI;
    else if (PhEqualString2(encoding, L"utf8", TRUE))
        *Encoding = PH_STRING_SEARCH_ENCODING_UTF8;
    else if (PhEqualString2(encoding, L"utf16", TRUE))
        *Encoding = PH_STRING_SEARCH_ENCODING_UTF16;
    else
    {
        AtSetToolError(
            Result,
            "invalid_arguments",
            STATUS_INVALID_PARAMETER,
            L"encoding must be ansi, utf8 or utf16."
            );
        PhDereferenceObject(encoding);
        return FALSE;
    }

    *HaveEncoding = TRUE;
    PhDereferenceObject(encoding);

    return TRUE;
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

// Parses the ISO 8601 form this server emits, so a time it returned can be handed straight back:
// YYYY-MM-DDTHH:MM:SS, optionally with milliseconds and a trailing Z. Deliberately strict — a
// half-understood time silently filters the wrong rows.
_Success_(return)
BOOLEAN AtParseTime(
    _In_opt_ PPH_STRING String,
    _Out_ PLARGE_INTEGER Time
    )
{
    static CONST struct { ULONG Offset; ULONG Length; } fields[] =
    {
        { 0, 4 },   // year
        { 5, 2 },   // month
        { 8, 2 },   // day
        { 11, 2 },  // hour
        { 14, 2 },  // minute
        { 17, 2 },  // second
    };
    SYSTEMTIME systemTime;
    PUSHORT values[RTL_NUMBER_OF(fields)];
    ULONG i;

    if (!String || String->Length < 19 * sizeof(WCHAR))
        return FALSE;

    if (String->Buffer[4] != L'-' || String->Buffer[7] != L'-' ||
        (String->Buffer[10] != L'T' && String->Buffer[10] != L' ') ||
        String->Buffer[13] != L':' || String->Buffer[16] != L':')
    {
        return FALSE;
    }

    memset(&systemTime, 0, sizeof(SYSTEMTIME));
    values[0] = &systemTime.wYear;
    values[1] = &systemTime.wMonth;
    values[2] = &systemTime.wDay;
    values[3] = &systemTime.wHour;
    values[4] = &systemTime.wMinute;
    values[5] = &systemTime.wSecond;

    for (i = 0; i < RTL_NUMBER_OF(fields); i++)
    {
        PH_STRINGREF part;
        ULONG64 value;

        part.Buffer = &String->Buffer[fields[i].Offset];
        part.Length = fields[i].Length * sizeof(WCHAR);

        if (!PhStringToUInt64(&part, 10, &value) || value > USHRT_MAX)
            return FALSE;

        *values[i] = (USHORT)value;
    }

    if (String->Length >= 23 * sizeof(WCHAR) && String->Buffer[19] == L'.')
    {
        PH_STRINGREF part;
        ULONG64 value;

        part.Buffer = &String->Buffer[20];
        part.Length = 3 * sizeof(WCHAR);

        if (PhStringToUInt64(&part, 10, &value) && value <= 999)
            systemTime.wMilliseconds = (USHORT)value;
    }

    return !!PhSystemTimeToLargeInteger(Time, &systemTime);
}

/**
 * Splits a registry path into the root it names and the rest. A hive prefix picks a predefined
 * root and the remainder is relative to it; a native \Registry path opens on its own, with no
 * root and NativeRoot left null.
 */
VOID AtParseRegistryPath(
    _In_ PPH_STRING Path,
    _Out_ PHANDLE Root,
    _Out_ PPH_STRING* SubKey,
    _Out_ PCWSTR* NativeRoot
    )
{
    static CONST struct
    {
        PCWSTR Prefix;
        HANDLE Root;
        PCWSTR Native;
    } roots[] =
    {
        { L"HKEY_LOCAL_MACHINE", PH_KEY_LOCAL_MACHINE, L"\\Registry\\Machine" },
        { L"HKLM", PH_KEY_LOCAL_MACHINE, L"\\Registry\\Machine" },
        { L"HKEY_CURRENT_USER", PH_KEY_CURRENT_USER, L"\\Registry\\User\\<current>" },
        { L"HKCU", PH_KEY_CURRENT_USER, L"\\Registry\\User\\<current>" },
        { L"HKEY_USERS", PH_KEY_USERS, L"\\Registry\\User" },
        { L"HKU", PH_KEY_USERS, L"\\Registry\\User" },
        { L"HKEY_CLASSES_ROOT", PH_KEY_CLASSES_ROOT, L"\\Registry\\Machine\\Software\\Classes" },
        { L"HKCR", PH_KEY_CLASSES_ROOT, L"\\Registry\\Machine\\Software\\Classes" },
    };
    ULONG i;

    *Root = NULL;
    *SubKey = NULL;
    *NativeRoot = NULL;

    for (i = 0; i < RTL_NUMBER_OF(roots); i++)
    {
        PH_STRINGREF prefix;
        PH_STRINGREF remaining;

        PhInitializeStringRef(&prefix, roots[i].Prefix);

        if (!PhStartsWithStringRef(&Path->sr, &prefix, TRUE))
            continue;

        remaining = Path->sr;
        PhSkipStringRef(&remaining, prefix.Length);

        if (remaining.Length != 0 && remaining.Buffer[0] != OBJ_NAME_PATH_SEPARATOR)
            continue;

        if (remaining.Length != 0)
            PhSkipStringRef(&remaining, sizeof(WCHAR));

        *Root = roots[i].Root;
        *NativeRoot = roots[i].Native;
        *SubKey = PhCreateString2(&remaining);
        return;
    }

    *SubKey = PhReferenceObject(Path);
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

/**
 * The memory priority levels, which are the numbers every tool here reports a page priority as.
 * Five is the default a process starts with; below it the pages are the first to be trimmed.
 */
PCWSTR AtPagePriorityString(
    _In_ ULONG PagePriority
    )
{
    switch (PagePriority)
    {
    case MEMORY_PRIORITY_LOWEST:
        return L"lowest";
    case MEMORY_PRIORITY_VERY_LOW:
        return L"very_low";
    case MEMORY_PRIORITY_LOW:
        return L"low";
    case MEMORY_PRIORITY_MEDIUM:
        return L"medium";
    case MEMORY_PRIORITY_BELOW_NORMAL:
        return L"below_normal";
    case MEMORY_PRIORITY_NORMAL:
        return L"normal";
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

PCWSTR AtMachineString(
    _In_ USHORT Machine
    )
{
    switch (Machine)
    {
    case IMAGE_FILE_MACHINE_I386:
        return L"x86";
    case IMAGE_FILE_MACHINE_AMD64:
        return L"x64";
    case IMAGE_FILE_MACHINE_ARM64:
        return L"ARM64";
    case IMAGE_FILE_MACHINE_ARMNT:
        return L"ARM";
    case IMAGE_FILE_MACHINE_IA64:
        return L"IA64";
    }

    return NULL;
}

PCWSTR AtSubsystemString(
    _In_ USHORT Subsystem
    )
{
    switch (Subsystem)
    {
    case IMAGE_SUBSYSTEM_NATIVE:
        return L"native";
    case IMAGE_SUBSYSTEM_WINDOWS_GUI:
        return L"windows_gui";
    case IMAGE_SUBSYSTEM_WINDOWS_CUI:
        return L"windows_cui";
    case IMAGE_SUBSYSTEM_EFI_APPLICATION:
        return L"efi_application";
    case IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
        return L"efi_boot_service_driver";
    case IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
        return L"efi_runtime_driver";
    case IMAGE_SUBSYSTEM_XBOX:
        return L"xbox";
    }

    return NULL;
}

VOID AtJsonAddFlagStrings(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ ULONG Value,
    _In_reads_(Count) CONST ULONG* Flags,
    _In_reads_(Count) CONST PWSTR* Names,
    _In_ ULONG Count
    )
{
    PVOID array = PhCreateJsonArray();
    ULONG i;

    for (i = 0; i < Count; i++)
    {
        if (Value & Flags[i])
        {
            PH_STRINGREF sr;
            PPH_BYTES utf8;

            PhInitializeStringRef(&sr, Names[i]);

            if (utf8 = PhConvertUtf16ToUtf8Ex(sr.Buffer, sr.Length))
            {
                PhAddJsonArrayObject(array, PhCreateJsonStringObject(utf8->Buffer));
                PhDereferenceObject(utf8);
            }
        }
    }

    PhAddJsonObjectValue(Object, Key, array);
}

// A section, rendered the same way whether the information came from the object itself or from the
// driver on behalf of another process. The caller queries; this only formats.
// Whether the file's signature chains to a Microsoft root, which is a different question from the
// signer name reading as Microsoft: anyone can put that in a certificate, and a binary signed
// through a Microsoft CA does not chain to the root Windows itself is signed with.
//
// Both of these convert to a Win32 path first and say so. Telling PhVerifyFileIsChainedToMicrosoft
// that a Win32 path is native, or handing PhVerifyFile a native one, makes every file look unsigned
// - and an "unsigned" answer is the one nobody questions.
BOOLEAN AtIsMicrosoftSigned(
    _In_opt_ PPH_STRING FileName
    )
{
    PPH_STRING win32FileName;
    BOOLEAN chained;

    if (PhIsNullOrEmptyString(FileName))
        return FALSE;

    if (!(win32FileName = PhGetFileName(FileName)))
        return FALSE;

    chained = !!PhVerifyFileIsChainedToMicrosoft(&win32FileName->sr, FALSE);
    PhDereferenceObject(win32FileName);

    return chained;
}

VERIFY_RESULT AtVerifyFileName(
    _In_opt_ PPH_STRING FileName,
    _Out_opt_ PPH_STRING *Signer
    )
{
    PPH_STRING win32FileName;
    VERIFY_RESULT result;

    if (Signer)
        *Signer = NULL;

    if (PhIsNullOrEmptyString(FileName))
        return VrUnknown;

    if (!(win32FileName = PhGetFileName(FileName)))
        return VrUnknown;

    result = PhVerifyFile(PhGetString(win32FileName), Signer);
    PhDereferenceObject(win32FileName);

    return result;
}

VOID AtAddSectionInfo(
    _In_ PVOID Structured,
    _In_ PSECTION_BASIC_INFORMATION Basic,
    _In_opt_ PSECTION_IMAGE_INFORMATION Image,
    _In_opt_ PPH_STRING FileName
    )
{
    static CONST ULONG sectionFlags[] =
    {
        SEC_BASED, SEC_NO_CHANGE, SEC_FILE, SEC_IMAGE, SEC_PROTECTED_IMAGE,
        SEC_RESERVE, SEC_COMMIT, SEC_NOCACHE, SEC_WRITECOMBINE, SEC_LARGE_PAGES
    };
    static CONST PWSTR sectionNames[] =
    {
        L"based", L"no_change", L"file", L"image", L"protected_image",
        L"reserve", L"commit", L"no_cache", L"write_combine", L"large_pages"
    };
    PVOID details;

    details = PhCreateJsonObject();
    PhAddJsonObjectUInt64(details, "size", Basic->MaximumSize.QuadPart);
    AtJsonAddPointer(details, "base_address", Basic->BaseAddress);
    AtJsonAddFlagStrings(details, "attributes", Basic->AllocationAttributes,
        sectionFlags, (CONST PWSTR*)sectionNames, RTL_NUMBER_OF(sectionFlags));
    AtJsonAddString(details, "file_name", FileName);
    AtJsonAddWin32FileName(details, "file_path", FileName);

    if (Image)
    {
        PVOID image = PhCreateJsonObject();

        AtJsonAddStringZ(image, "machine", AtMachineString(Image->Machine));
        AtJsonAddStringZ(image, "subsystem", AtSubsystemString((USHORT)Image->SubSystemType));
        AtJsonAddPointer(image, "entry_point", Image->TransferAddress);
        PhAddJsonObjectUInt64(image, "image_file_size", Image->ImageFileSize);
        PhAddJsonObjectUInt64(image, "maximum_stack_size", Image->MaximumStackSize);
        PhAddJsonObjectBoolean(image, "contains_code", !!Image->ImageContainsCode);
        PhAddJsonObjectBoolean(image, "dynamically_relocated", !!Image->ImageDynamicallyRelocated);
        PhAddJsonObjectBoolean(image, "dotnet_il_only", !!Image->ComPlusILOnly);
        PhAddJsonObjectValue(details, "image", image);
    }
    else
    {
        AtJsonAddNull(details, "image");
    }

    PhAddJsonObjectValue(Structured, "section", details);
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
    case AtActionSetProcessAffinity:
    case AtActionSetProcessPagePriority:
    case AtActionEmptyProcessWorkingSet:
    case AtActionFreezeProcess:
    case AtActionThawProcess:
    case AtActionGetProcessToken:
    case AtActionGetProcessWindows:
    case AtActionGetProcessJob:
    case AtActionListHiddenProcesses:
    case AtActionGetProcessKsiState:
        AtProcessInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionGetProcessThreads:
    case AtActionGetThreadStack:
    case AtActionGetProcessStacks:
    case AtActionResolveSymbol:
    case AtActionSuspendThread:
    case AtActionResumeThread:
    case AtActionTerminateThread:
    case AtActionCancelThreadIo:
        AtThreadInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionGetThreadWaitChain:
    case AtActionAnalyzeThreadWait:
        AtWaitInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionGetProcessModules:
    case AtActionGetProcessUnloadedModules:
    case AtActionGetProcessImageCoherency:
    case AtActionGetImagePageModifications:
        AtModuleInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionGetProcessMitigations:
        AtMitigationInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionGetProcessHistory:
    case AtActionGetSystemHistory:
    case AtActionRankProcesses:
        AtHistoryInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionGetGpuUsage:
    case AtActionListGpuAdapters:
    case AtActionGetProcessGpuStats:
        AtGpuInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionListDevices:
    case AtActionGetDeviceResources:
    case AtActionSetDeviceEnabled:
        AtDeviceInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionListNetworkAdapters:
        AtAdapterInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionGetDotNetAssemblies:
        AtDotNetInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionGetProcessNotes:
    case AtActionSetProcessComment:
        AtNoteInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionListWindows:
    case AtActionCloseWindow:
    case AtActionSetWindowState:
    case AtActionGetWindowInfo:
        AtWindowInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionGetDiskPerformance:
    case AtActionGetDiskIdentity:
    case AtActionGetDiskHealth:
        AtDiskInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionGetProcessIoRates:
        AtIoInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionListRecentEvents:
    case AtActionListRecentProcessExits:
        AtEventInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionGetProcessHandles:
    case AtActionGetProcessHandlesDetailed:
    case AtActionGetProcessMemoryRegions:
    case AtActionSearchProcessStrings:
    case AtActionCreateProcessMinidump:
    case AtActionCloseHandle:
        AtMemoryInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionListServices:
    case AtActionGetService:
    case AtActionStartService:
    case AtActionPauseService:
    case AtActionContinueService:
    case AtActionStopService:
    case AtActionRestartService:
    case AtActionSetServiceConfig:
        AtServiceInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionFindHandles:
    case AtActionFindModules:
    case AtActionGetFileUsers:
    case AtActionListObjectDirectory:
    case AtActionGetObjectInfo:
    case AtActionGetDriverObject:
        AtFindInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionGetAlpcPortInfo:
    case AtActionGetHandleDetails:
    case AtActionListNamedPipes:
    case AtActionGetSectionMappings:
    case AtActionFindObjectHandles:
        AtHandleInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionListFirewallEvents:
        AtFirewallInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionPingHost:
    case AtActionWhoisLookup:
        AtEgressInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionLookupIpCountry:
    case AtActionListNetworkConnections:
    case AtActionCloseNetworkConnection:
        AtNetworkInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionGetSystemInfo:
    case AtActionListPoolTags:
    case AtActionListKernelDrivers:
    case AtActionGetKsiStatus:
    case AtActionGetPagefileInfo:
    case AtActionListStartupEntries:
    case AtActionGetSmbiosInfo:
    case AtActionGetUefiVariables:
    case AtActionGetTpmInfo:
    case AtActionGetSystemEnvironment:
    case AtActionGetMemoryDetails:
    case AtActionGetSecurityPosture:
    case AtActionGetCpuInfo:
        AtSystemInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionListLogonSessions:
    case AtActionListTerminalSessions:
    case AtActionLookupAccount:
        AtSessionInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionListScheduledTasks:
        AtTaskInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionListWmiSubscriptions:
        AtWmiInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionGetObjectSecurity:
        AtSecurityInvokeTool(Tool, Call, Target, Result);
        break;
    case AtActionVerifyFileSignature:
    case AtActionGetFileHashes:
    case AtActionGetImageStrings:
    case AtActionGetFileInfo:
    case AtActionListDirectory:
    case AtActionGetFileScanResultCached:
    case AtActionLookupFileHashVirusTotal:
    case AtActionLookupFileHashHybridAnalysis:
    case AtActionReadRegistryKey:
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
