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
    PVOID Modules;
    ULONG Count;
    PPH_STRING NameContains;
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

    PhAddJsonArrayObject(context->Modules, row);
    context->Count++;

    return TRUE;
}

VOID AtpGetProcessModules(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    AT_TARGET target;
    AT_MODULE_CONTEXT context;
    ULONG flags = 0;
    PVOID structured;

    if (!NT_SUCCESS(AtResolveProcessTarget(Call->Arguments, FALSE, 0, &target, Result)))
        return;

    memset(&context, 0, sizeof(AT_MODULE_CONTEXT));
    context.Modules = PhCreateJsonArray();
    context.NameContains = AtGetArgumentString(Call->Arguments, "name_contains");

    if (AtJsonGetObjectBoolean(Call->Arguments, "include_mapped_files"))
        flags = PH_ENUM_GENERIC_MAPPED_FILES | PH_ENUM_GENERIC_MAPPED_IMAGES;

    status = PhEnumGenericModules(target.ProcessItem->ProcessId, NULL, flags, AtpModuleCallback, &context);

    if (!NT_SUCCESS(status) && context.Count == 0)
    {
        AtSetToolStatusError(Result, status, L"Enumerating modules");
        PhFreeJsonObject(context.Modules);
        PhClearReference(&context.NameContains);
        AtDeleteTarget(&target);
        return;
    }

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, target.ProcessItem);
    PhAddJsonObjectValue(structured, "modules", context.Modules);
    PhAddJsonObjectUInt64(structured, "count", context.Count);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    PhClearReference(&context.NameContains);
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
