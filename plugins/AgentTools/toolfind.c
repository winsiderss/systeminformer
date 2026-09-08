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

// The same question for loaded code: which processes have this module mapped. ListDLLs answers it
// for one machine-wide search; this adds the signature, because "which processes loaded this
// unsigned DLL" is the version of the question worth asking.
//
// Verification is by far the most expensive part, and the same DLL is loaded in a hundred
// processes, so a result is looked up once per file and remembered for the rest of the scan.

typedef struct _AT_FIND_MODULES_CONTEXT
{
    AT_ROWS Rows;
    PPH_STRING NameContains;
    BOOLEAN UnsignedOnly;
    BOOLEAN Verify;
    BOOLEAN IncludeMappedFiles;
    PPH_HASHTABLE VerifyCache;
    HANDLE ProcessId;
    PPH_STRING ProcessName;
    ULONG64 ProcessSequenceNumber;
    ULONG64 Deadline;
    ULONG Scanned;
    ULONG Verified;
    BOOLEAN TimedOut;
} AT_FIND_MODULES_CONTEXT, *PAT_FIND_MODULES_CONTEXT;

typedef struct _AT_VERIFY_ENTRY
{
    PPH_STRING FileName;
    VERIFY_RESULT Result;
    PPH_STRING Signer;
} AT_VERIFY_ENTRY, *PAT_VERIFY_ENTRY;

static BOOLEAN NTAPI AtpVerifyCacheCompare(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    return PhEqualString(((PAT_VERIFY_ENTRY)Entry1)->FileName, ((PAT_VERIFY_ENTRY)Entry2)->FileName, TRUE);
}

static ULONG NTAPI AtpVerifyCacheHash(
    _In_ PVOID Entry
    )
{
    return PhHashStringRefEx(&((PAT_VERIFY_ENTRY)Entry)->FileName->sr, TRUE, PH_STRING_HASH_X65599);
}

// One verification per distinct file for the life of the scan. Without this a machine-wide search
// verifies ntdll.dll once for every process on the machine.
BOOLEAN AtpVerifyModuleCached(
    _In_ PAT_FIND_MODULES_CONTEXT Context,
    _In_ PPH_STRING NativeFileName,
    _Out_ PVERIFY_RESULT VerifyResult,
    _Out_ PPH_STRING* Signer
    )
{
    AT_VERIFY_ENTRY lookup;
    PAT_VERIFY_ENTRY found;
    AT_VERIFY_ENTRY entry;
    PPH_STRING win32FileName;

    *VerifyResult = VrUnknown;
    *Signer = NULL;

    lookup.FileName = NativeFileName;

    if (found = PhFindEntryHashtable(Context->VerifyCache, &lookup))
    {
        *VerifyResult = found->Result;
        PhSetReference(Signer, found->Signer);
        return TRUE;
    }

    // PhVerifyFile takes a Win32 path; the module list carries native ones, and the mismatch is
    // silent - everything reads as unverified.
    if (!(win32FileName = PhGetFileName(NativeFileName)))
        return FALSE;

    memset(&entry, 0, sizeof(AT_VERIFY_ENTRY));
    entry.FileName = PhReferenceObject(NativeFileName);
    entry.Result = PhVerifyFile(win32FileName->Buffer, &entry.Signer);
    PhDereferenceObject(win32FileName);

    Context->Verified++;
    PhAddEntryHashtable(Context->VerifyCache, &entry);

    *VerifyResult = entry.Result;
    PhSetReference(Signer, entry.Signer);

    return TRUE;
}

_Function_class_(PH_ENUM_GENERIC_MODULES_CALLBACK)
BOOLEAN NTAPI AtpFindModulesCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_opt_ PVOID Context
    )
{
    PAT_FIND_MODULES_CONTEXT context = Context;
    PVOID row;
    VERIFY_RESULT verifyResult = VrUnknown;
    PPH_STRING signer = NULL;
    BOOLEAN verified = FALSE;

    if (!context)
        return FALSE;

    context->Scanned++;

    if (context->NameContains &&
        !AtContainsString(Module->Name, context->NameContains) &&
        !AtContainsString(Module->FileName, context->NameContains))
    {
        return TRUE;
    }

    if (context->Verify && Module->FileName)
        verified = AtpVerifyModuleCached(context, Module->FileName, &verifyResult, &signer);

    if (context->UnsignedOnly && (!verified || verifyResult == VrTrusted))
    {
        PhClearReference(&signer);
        return TRUE;
    }

    row = PhCreateJsonObject();
    PhAddJsonObjectUInt64(row, "pid", HandleToUlong(context->ProcessId));
    PhAddJsonObjectUInt64(row, "process_sequence_number", context->ProcessSequenceNumber);
    AtJsonAddString(row, "process_name", context->ProcessName);
    AtJsonAddString(row, "name", Module->Name);
    AtJsonAddWin32FileName(row, "file_path", Module->FileName);
    AtJsonAddStringZ(row, "type", AtModuleTypeString(Module->Type));
    AtJsonAddPointer(row, "base_address", Module->BaseAddress);
    PhAddJsonObjectUInt64(row, "size", Module->Size);

    if (context->Verify)
    {
        AtJsonAddStringZ(row, "signature", verified ? AtVerifyResultString(verifyResult) : NULL);
        AtJsonAddString(row, "signer", signer);
    }
    else
    {
        AtJsonAddNull(row, "signature");
        AtJsonAddNull(row, "signer");
    }

    AtAddRow(&context->Rows, row);
    PhClearReference(&signer);

    return TRUE;
}

VOID AtpFindModules(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    AT_FIND_MODULES_CONTEXT context;
    PPH_PROCESS_ITEM* processItems;
    ULONG numberOfProcessItems;
    PVOID structured;
    ULONG64 processId;
    ULONG64 seconds = AT_FIND_DEFAULT_SECONDS;
    BOOLEAN haveProcessId;
    ULONG i;

    memset(&context, 0, sizeof(AT_FIND_MODULES_CONTEXT));
    context.NameContains = AtGetArgumentString(Call->Arguments, "name_contains");
    context.UnsignedOnly = AtJsonGetObjectBoolean(Call->Arguments, "unsigned_only");
    context.Verify = context.UnsignedOnly || AtJsonGetObjectBoolean(Call->Arguments, "verify_signatures");
    context.IncludeMappedFiles = AtJsonGetObjectBoolean(Call->Arguments, "include_mapped_files");
    haveProcessId = AtGetArgumentUInt64(Call->Arguments, "pid", &processId);

    if (!context.NameContains && !context.UnsignedOnly && !haveProcessId)
    {
        AtSetToolError(
            Result,
            "invalid_arguments",
            STATUS_INVALID_PARAMETER,
            L"Give at least one of name_contains, unsigned_only or pid; this walks the modules of every process."
            );
        PhClearReference(&context.NameContains);
        return;
    }

    if (AtGetArgumentUInt64(Call->Arguments, "max_seconds", &seconds))
        seconds = min(max(seconds, 1), AT_FIND_MAXIMUM_SECONDS);

    context.Deadline = NtGetTickCount64() + seconds * 1000;
    context.VerifyCache = PhCreateHashtable(sizeof(AT_VERIFY_ENTRY), AtpVerifyCacheCompare, AtpVerifyCacheHash, 64);

    AtInitializeRows(&context.Rows, Call->Arguments);
    PhEnumProcessItems(&processItems, &numberOfProcessItems);

    for (i = 0; i < numberOfProcessItems; i++)
    {
        PPH_PROCESS_ITEM processItem = processItems[i];

        if (NtGetTickCount64() > context.Deadline)
        {
            context.TimedOut = TRUE;
            break;
        }

        if (haveProcessId && processItem->ProcessId != UlongToHandle((ULONG)processId))
            continue;

        if (!PH_IS_REAL_PROCESS_ID(processItem->ProcessId))
            continue;

        context.ProcessId = processItem->ProcessId;
        context.ProcessName = processItem->ProcessName;
        context.ProcessSequenceNumber = processItem->ProcessSequenceNumber;

        // Mapped images as well as loaded modules, because an image mapped without being loaded
        // is exactly what is worth finding. Mapped data files are left out unless asked for: they
        // are not code, so every one of them is trivially unsigned, and including them buries a
        // machine-wide unsigned_only search under cache and database files.
        PhEnumGenericModules(
            processItem->ProcessId,
            NULL,
            context.IncludeMappedFiles ? (PH_ENUM_GENERIC_MAPPED_FILES | PH_ENUM_GENERIC_MAPPED_IMAGES) : PH_ENUM_GENERIC_MAPPED_IMAGES,
            AtpFindModulesCallback,
            &context
            );
    }

    PhDereferenceObjects(processItems, numberOfProcessItems);
    PhFree(processItems);

    {
        PH_HASHTABLE_ENUM_CONTEXT enumContext;
        PAT_VERIFY_ENTRY entry;

        PhBeginEnumHashtable(context.VerifyCache, &enumContext);

        while (entry = PhNextEnumHashtable(&enumContext))
        {
            PhClearReference(&entry->FileName);
            PhClearReference(&entry->Signer);
        }

        PhDereferenceObject(context.VerifyCache);
    }

    structured = PhCreateJsonObject();
    AtAddRows(structured, "modules", &context.Rows);
    PhAddJsonObjectUInt64(structured, "scanned", context.Scanned);
    PhAddJsonObjectUInt64(structured, "files_verified", context.Verified);
    PhAddJsonObjectBoolean(structured, "timed_out", context.TimedOut);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteRows(&context.Rows);
    PhClearReference(&context.NameContains);
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
    case AtActionFindModules:
        AtpFindModules(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
