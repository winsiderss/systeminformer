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

// Who is holding a thing, across every process: the question get_process_handles_detailed cannot
// answer, because it needs to be told the process first. This is what says which process has the
// file that will not delete, or which one still has the registry key or the mutex.
//
// The scan opens every process it can and asks each handle for its name, so it is not free. It
// refuses to run with no filter at all, filters on what is cheap before it names anything, and
// stops on a time budget rather than running for as long as it takes.

#define AT_FIND_DEFAULT_SECONDS 20
#define AT_FIND_MAXIMUM_SECONDS 60

typedef struct _AT_FIND_HANDLES_CONTEXT
{
    AT_ROWS Rows;
    PPH_STRING NameContains;
    PPH_STRING TypeName;
    HANDLE ProcessId;
    BOOLEAN HaveProcessId;
    ULONG64 Deadline;
    ULONG Scanned;
    ULONG Named;
    BOOLEAN TimedOut;
} AT_FIND_HANDLES_CONTEXT, *PAT_FIND_HANDLES_CONTEXT;

VOID AtpAddFoundHandle(
    _In_ PAT_FIND_HANDLES_CONTEXT Context,
    _In_ PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Entry,
    _In_ PPH_STRING TypeName,
    _In_opt_ PPH_STRING ObjectName,
    _In_opt_ PPH_STRING BestName
    )
{
    PVOID row;
    PPH_PROCESS_ITEM processItem;

    row = PhCreateJsonObject();
    PhAddJsonObjectUInt64(row, "pid", HandleToUlong(Entry->UniqueProcessId));

    // The sequence number goes with the pid so a row can be handed straight to close_handle and
    // be refused if the process has since exited and the pid was reused.
    if (processItem = PhReferenceProcessItem(Entry->UniqueProcessId))
    {
        PhAddJsonObjectUInt64(row, "process_sequence_number", processItem->ProcessSequenceNumber);
        AtJsonAddString(row, "process_name", processItem->ProcessName);
        PhDereferenceObject(processItem);
    }
    else
    {
        AtJsonAddNull(row, "process_sequence_number");
        AtJsonAddNull(row, "process_name");
    }

    AtJsonAddPointer(row, "handle", Entry->HandleValue);
    AtJsonAddString(row, "type", TypeName);
    AtJsonAddString(row, "object_name", ObjectName);
    AtJsonAddString(row, "best_name", BestName);
    AtJsonAddPointer(row, "object_address", Entry->Object);
    AtJsonAddHex(row, "granted_access", Entry->GrantedAccess);

    AtAddRow(&Context->Rows, row);
}

VOID AtpFindHandles(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    AT_FIND_HANDLES_CONTEXT context;
    PSYSTEM_HANDLE_INFORMATION_EX handles;
    PPH_HASHTABLE processHandles;
    PPH_KEY_VALUE_PAIR pair;
    PVOID structured;
    ULONG64 processId;
    ULONG64 seconds = AT_FIND_DEFAULT_SECONDS;
    NTSTATUS status;
    ULONG i;

    memset(&context, 0, sizeof(AT_FIND_HANDLES_CONTEXT));
    context.NameContains = AtGetArgumentString(Call->Arguments, "name_contains");
    context.TypeName = AtGetArgumentString(Call->Arguments, "type_name");
    context.HaveProcessId = AtGetArgumentUInt64(Call->Arguments, "pid", &processId);

    if (context.HaveProcessId)
        context.ProcessId = UlongToHandle((ULONG)processId);

    // A scan with nothing to match would name every handle on the machine and answer a question
    // nobody asked.
    if (!context.NameContains && !context.TypeName && !context.HaveProcessId)
    {
        AtSetToolError(
            Result,
            "invalid_arguments",
            STATUS_INVALID_PARAMETER,
            L"Give at least one of name_contains, type_name or pid; this searches every handle on the machine."
            );
        PhClearReference(&context.NameContains);
        PhClearReference(&context.TypeName);
        return;
    }

    if (AtGetArgumentUInt64(Call->Arguments, "max_seconds", &seconds))
        seconds = min(max(seconds, 1), AT_FIND_MAXIMUM_SECONDS);

    if (!NT_SUCCESS(status = PhEnumHandlesEx(&handles)))
    {
        AtSetToolStatusError(Result, status, L"Enumerating the handles");
        PhClearReference(&context.NameContains);
        PhClearReference(&context.TypeName);
        return;
    }

    context.Deadline = NtGetTickCount64() + seconds * 1000;

    AtInitializeRows(&context.Rows, Call->Arguments);
    processHandles = PhCreateSimpleHashtable(16);

    for (i = 0; i < handles->NumberOfHandles; i++)
    {
        PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX entry = &handles->Handles[i];
        HANDLE processHandle;
        PPH_STRING typeName = NULL;
        PPH_STRING objectName = NULL;
        PPH_STRING bestName = NULL;

        if (NtGetTickCount64() > context.Deadline)
        {
            context.TimedOut = TRUE;
            break;
        }

        if (context.HaveProcessId && entry->UniqueProcessId != context.ProcessId)
            continue;

        // The type comes from the index alone, with no process opened and no object queried, so
        // a type filter costs nothing and removes almost everything.
        if (context.TypeName)
        {
            PPH_STRING indexName = PhGetObjectTypeIndexName(entry->ObjectTypeIndex);

            if (!indexName)
                continue;

            if (!PhEqualString(indexName, context.TypeName, TRUE))
            {
                PhDereferenceObject(indexName);
                continue;
            }

            PhDereferenceObject(indexName);
        }

        context.Scanned++;

        if (!(processHandle = PhFindItemSimpleHashtable2(processHandles, entry->UniqueProcessId)))
        {
            HANDLE opened = NULL;

            // Duplicating the handle is what names most objects; without that access the entry
            // can still be reported by type and address, so a failure to open is not fatal.
            if (!NT_SUCCESS(PhOpenProcess(&opened, PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, entry->UniqueProcessId)))
                PhOpenProcess(&opened, PROCESS_QUERY_INFORMATION, entry->UniqueProcessId);

            // Cached either way: a process that cannot be opened must not be retried for every
            // one of its handles.
            PhAddItemSimpleHashtable(processHandles, entry->UniqueProcessId, opened);
            processHandle = opened;
        }

        if (!processHandle)
            continue;

        // File handles can block a name query forever; PhGetHandleInformationEx already asks for
        // those through a thread it can abandon, which is why this can run on the server thread.
        if (NT_SUCCESS(PhGetHandleInformationEx(
            processHandle,
            entry->HandleValue,
            entry->ObjectTypeIndex,
            0,
            NULL,
            NULL,
            &typeName,
            &objectName,
            &bestName,
            NULL
            )))
        {
            context.Named++;

            if (!context.NameContains ||
                AtContainsString(objectName, context.NameContains) ||
                AtContainsString(bestName, context.NameContains))
            {
                AtpAddFoundHandle(&context, entry, typeName, objectName, bestName);
            }
        }

        PhClearReference(&typeName);
        PhClearReference(&objectName);
        PhClearReference(&bestName);
    }

    {
        ULONG enumerationKey = 0;

        while (PhEnumHashtable(processHandles, &pair, &enumerationKey))
        {
            if (pair->Value)
                NtClose(pair->Value);
        }
    }

    PhDereferenceObject(processHandles);
    PhFree(handles);

    structured = PhCreateJsonObject();
    AtAddRows(structured, "handles", &context.Rows);
    PhAddJsonObjectUInt64(structured, "scanned", context.Scanned);
    PhAddJsonObjectUInt64(structured, "named", context.Named);
    // A scan that ran out of time has looked at some of the machine, not all of it, and an empty
    // answer from it means nothing.
    PhAddJsonObjectBoolean(structured, "timed_out", context.TimedOut);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteRows(&context.Rows);
    PhClearReference(&context.NameContains);
    PhClearReference(&context.TypeName);
}

VOID AtFindInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    UNREFERENCED_PARAMETER(Target);

    switch (Tool->Action)
    {
    case AtActionFindHandles:
        AtpFindHandles(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
