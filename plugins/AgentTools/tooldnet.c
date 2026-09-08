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

#include <dotnettoolsintf.h>

// The assemblies a .NET process has loaded, which get_process_modules cannot see: an assembly is
// not a mapped image, and one loaded from memory or emitted at run time has no file behind it at
// all. That last kind is worth the tool on its own - code that was never on disk is what a
// reflection loader leaves behind.
//
// Read through the DotNetTools plugin, which walks the target's runtime with the debugging data
// access layer.

typedef struct _AT_ASSEMBLY_CONTEXT
{
    AT_ROWS Rows;
    PPH_STRING NameContains;
    BOOLEAN DynamicOnly;
} AT_ASSEMBLY_CONTEXT, *PAT_ASSEMBLY_CONTEXT;

PDOTNETTOOLS_INTERFACE AtGetDotNetToolsInterface(
    VOID
    )
{
    static PDOTNETTOOLS_INTERFACE pluginInterface = NULL;
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        PPH_PLUGIN plugin;

        if (plugin = PhFindPlugin(DOTNETTOOLS_PLUGIN_NAME))
        {
            pluginInterface = PhGetPluginInformation(plugin)->Interface;

            if (pluginInterface && pluginInterface->Version < DOTNETTOOLS_INTERFACE_VERSION)
                pluginInterface = NULL;
        }

        PhEndInitOnce(&initOnce);
    }

    return pluginInterface;
}

PCSTR AtpAppDomainTypeString(
    _In_ ULONG AppDomainType
    )
{
    // DN_CLR_APPDOMAIN_TYPE. Its first value is the runtime's word for an ordinary application
    // domain, "dynamic", which reported next to is_dynamic would read as an assembly the runtime
    // generated. It is called what it is instead.
    switch (AppDomainType)
    {
    case 0:
        return "application";
    case 1:
        return "shared";
    case 2:
        return "system";
    }

    return "unknown";
}

_Function_class_(DOTNETTOOLS_ASSEMBLY_CALLBACK)
BOOLEAN NTAPI AtpAssemblyCallback(
    _In_ PDOTNETTOOLS_ASSEMBLY Assembly,
    _In_opt_ PVOID Context
    )
{
    PAT_ASSEMBLY_CONTEXT context = Context;
    PVOID row;

    if (!context)
        return FALSE;

    if (context->DynamicOnly && !Assembly->IsDynamic && !Assembly->IsDynamicModule)
        return TRUE;

    if (context->NameContains &&
        !AtContainsString(Assembly->AssemblyName, context->NameContains) &&
        !AtContainsString(Assembly->DisplayName, context->NameContains) &&
        !AtContainsString(Assembly->ModuleName, context->NameContains))
    {
        return TRUE;
    }

    row = PhCreateJsonObject();
    AtJsonAddString(row, "name", Assembly->AssemblyName);
    AtJsonAddString(row, "display_name", Assembly->DisplayName);
    AtJsonAddString(row, "module_name", Assembly->ModuleName);
    AtJsonAddString(row, "native_image_file", Assembly->NativeFileName);
    AtJsonAddPointer(row, "base_address", Assembly->BaseAddress);
    PhAddJsonObject(row, "app_domain_type", AtpAppDomainTypeString(Assembly->AppDomainType));
    AtJsonAddString(row, "app_domain", Assembly->AppDomainName);
    PhAddJsonObjectUInt64(row, "app_domain_number", Assembly->AppDomainNumber);
    PhAddJsonObjectBoolean(row, "is_dynamic", !!Assembly->IsDynamic);
    PhAddJsonObjectBoolean(row, "is_reflection", !!Assembly->IsReflection);
    PhAddJsonObjectBoolean(row, "is_dynamic_module", !!Assembly->IsDynamicModule);
    PhAddJsonObjectBoolean(row, "is_memory_stream", !!Assembly->IsMemoryStream);
    PhAddJsonObjectBoolean(row, "is_main_module", !!Assembly->IsMainModule);

    {
        PPH_STRING mvid = PhFormatGuid(&Assembly->Mvid);

        AtJsonAddString(row, "mvid", mvid);
        PhClearReference(&mvid);
    }

    AtAddRow(&context->Rows, row);

    return TRUE;
}

VOID AtpGetDotNetAssemblies(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PDOTNETTOOLS_INTERFACE pluginInterface;
    AT_ASSEMBLY_CONTEXT context;
    AT_TARGET target;
    DOTNETTOOLS_ASSEMBLY_STATUS status;
    PVOID structured;

    if (!(pluginInterface = AtGetDotNetToolsInterface()))
    {
        AtSetToolHint(Result, AT_HINT_PLUGIN_MISSING);
        AtSetToolError(
            Result,
            "plugin_missing",
            STATUS_NOT_FOUND,
            L"The DotNetTools plugin is not loaded, so the managed runtime cannot be read."
            );
        return;
    }

    if (!NT_SUCCESS(AtResolveProcessTarget(Call->Arguments, FALSE, 0, &target, Result)))
        return;

    memset(&context, 0, sizeof(AT_ASSEMBLY_CONTEXT));
    AtInitializeRows(&context.Rows, Call->Arguments);
    context.NameContains = AtGetArgumentString(Call->Arguments, "name_contains");
    context.DynamicOnly = AtJsonGetObjectBoolean(Call->Arguments, "dynamic_only");

    status = pluginInterface->EnumProcessAssemblies(
        target.ProcessItem->ProcessId,
        AtpAssemblyCallback,
        &context
        );

    // An empty list from a process that is running a CLR and one from a process that is not are
    // different answers, and only the plugin knows which this was.
    switch (status)
    {
    case DotNetToolsAssembliesNotDotNet:
        AtSetToolError(Result, "not_found", STATUS_NOT_FOUND, L"This process is not running a .NET runtime.");
        break;
    case DotNetToolsAssembliesWow64:
        AtSetToolError(
            Result,
            "unavailable",
            STATUS_NOT_SUPPORTED,
            L"Reading a 32-bit process's runtime needs a helper that prompts for elevation, so it is not done here."
            );
        break;
    case DotNetToolsAssembliesFailed:
        AtSetToolError(
            Result,
            "failed",
            STATUS_UNSUCCESSFUL,
            L"The runtime would not answer. Its debugging support has to match the runtime, and a process that is starting up or shutting down may not answer at all."
            );
        break;
    }

    if (status != DotNetToolsAssembliesOk)
    {
        AtDeleteRows(&context.Rows);
        PhClearReference(&context.NameContains);
        AtDeleteTarget(&target);
        return;
    }

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, target.ProcessItem);
    AtAddRows(structured, "assemblies", &context.Rows);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteRows(&context.Rows);
    PhClearReference(&context.NameContains);
    AtDeleteTarget(&target);
}

VOID AtDotNetInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    UNREFERENCED_PARAMETER(Target);

    switch (Tool->Action)
    {
    case AtActionGetDotNetAssemblies:
        AtpGetDotNetAssemblies(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
