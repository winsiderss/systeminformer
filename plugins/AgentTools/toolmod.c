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

PCWSTR AtpModuleTypeString(
    _In_ ULONG Type
    )
{
    switch (Type)
    {
    case PH_MODULE_TYPE_MODULE:
        return L"module";
    case PH_MODULE_TYPE_MAPPED_FILE:
        return L"mapped_file";
    case PH_MODULE_TYPE_WOW64_MODULE:
        return L"wow64_module";
    case PH_MODULE_TYPE_KERNEL_MODULE:
        return L"kernel_module";
    case PH_MODULE_TYPE_MAPPED_IMAGE:
        return L"mapped_image";
    case PH_MODULE_TYPE_ENCLAVE_MODULE:
        return L"enclave";
    }

    return L"unknown";
}

typedef struct _AT_MODULE_CONTEXT
{
    AT_ROWS Modules;
    PPH_STRING NameContains;
    BOOLEAN Summary;
} AT_MODULE_CONTEXT, *PAT_MODULE_CONTEXT;

_Function_class_(PH_ENUM_GENERIC_MODULES_CALLBACK)
BOOLEAN NTAPI AtpModuleCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_opt_ PVOID Context
    )
{
    PAT_MODULE_CONTEXT context = Context;
    PVOID row;

    if (context->NameContains &&
        !AtContainsString(Module->Name, context->NameContains) &&
        !AtContainsString(Module->FileName, context->NameContains))
    {
        return TRUE;
    }

    if (context->Summary)
    {
        // A triage pass wants the count, not a row per module.
        AtAddRow(&context->Modules, NULL);
        return TRUE;
    }

    row = PhCreateJsonObject();
    AtJsonAddString(row, "name", Module->Name);
    AtJsonAddWin32FileName(row, "file_path", Module->FileName);
    AtJsonAddStringZ(row, "type", AtpModuleTypeString(Module->Type));
    AtJsonAddPointer(row, "base_address", Module->BaseAddress);
    PhAddJsonObjectUInt64(row, "size", Module->Size);
    AtJsonAddPointer(row, "entry_point", Module->EntryPoint);

    if (Module->LoadOrderIndex != USHRT_MAX)
        PhAddJsonObjectUInt64(row, "load_order_index", Module->LoadOrderIndex);
    else
        AtJsonAddNull(row, "load_order_index");

    if (Module->LoadCount != USHRT_MAX)
        PhAddJsonObjectUInt64(row, "load_count", Module->LoadCount);
    else
        AtJsonAddNull(row, "load_count");

    AtJsonAddTime(row, "load_time", &Module->LoadTime);

    AtAddRow(&context->Modules, row);

    return TRUE;
}

// One process worth of modules. Returns NULL, with Status set, when nothing could be enumerated;
// in summary mode the rows are counted but never built.
PVOID AtpCreateModulesResult(
    _In_ PAT_TOOL_CALL Call,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ BOOLEAN Summary,
    _Out_ PNTSTATUS Status
    )
{
    AT_MODULE_CONTEXT context;
    ULONG flags = 0;
    PVOID entry;

    memset(&context, 0, sizeof(AT_MODULE_CONTEXT));
    AtInitializeRows(&context.Modules, Call->Arguments);
    context.NameContains = AtGetArgumentString(Call->Arguments, "name_contains");
    context.Summary = Summary;

    if (AtJsonGetObjectBoolean(Call->Arguments, "include_mapped_files"))
        flags = PH_ENUM_GENERIC_MAPPED_FILES | PH_ENUM_GENERIC_MAPPED_IMAGES;

    *Status = PhEnumGenericModules(ProcessItem->ProcessId, NULL, flags, AtpModuleCallback, &context);

    if (!NT_SUCCESS(*Status) && context.Modules.TotalCount == 0)
    {
        AtDeleteRows(&context.Modules);
        PhClearReference(&context.NameContains);
        return NULL;
    }

    entry = PhCreateJsonObject();
    AtFillProcessIdentity(entry, ProcessItem);

    if (Summary)
    {
        PhAddJsonObjectUInt64(entry, "count", context.Modules.TotalCount);
        AtDeleteRows(&context.Modules);
    }
    else
    {
        AtAddRows(entry, "modules", &context.Modules);
    }

    PhClearReference(&context.NameContains);

    return entry;
}

VOID AtpGetProcessModules(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
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

            if (entry = AtpCreateModulesResult(Call, processItem, batch.Summary, &status))
            {
                PhAddJsonArrayObject(results, entry);
            }
            else
            {
                PhAddJsonArrayObject(results, AtCreateBatchError(
                    processId,
                    status == STATUS_ACCESS_DENIED ? "access_denied" : "failed",
                    L"The modules of this process could not be enumerated."
                    ));
            }

            PhDereferenceObject(processItem);
        }

        Result->StructuredContent = AtCreateBatchResult(results);
        return;
    }

    if (!NT_SUCCESS(AtResolveProcessTarget(Call->Arguments, FALSE, 0, &target, Result)))
        return;

    structured = AtpCreateModulesResult(Call, target.ProcessItem, FALSE, &status);

    if (!structured)
    {
        AtSetToolStatusError(Result, status, L"Enumerating modules");
        AtDeleteTarget(&target);
        return;
    }

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteTarget(&target);
}

VOID AtModuleInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    UNREFERENCED_PARAMETER(Target);

    switch (Tool->Action)
    {
    case AtActionGetProcessModules:
        AtpGetProcessModules(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
