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

PCWSTR AtModuleTypeString(
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
    BOOLEAN IncludeDetails;
    BOOLEAN UnsignedOnly;
} AT_MODULE_CONTEXT, *PAT_MODULE_CONTEXT;

PCWSTR AtpLoadReasonString(
    _In_ USHORT LoadReason
    )
{
    switch (LoadReason)
    {
    case LoadReasonStaticDependency:
        return L"static_dependency";
    case LoadReasonStaticForwarderDependency:
        return L"static_forwarder_dependency";
    case LoadReasonDynamicForwarderDependency:
        return L"dynamic_forwarder_dependency";
    case LoadReasonDelayloadDependency:
        return L"delayload_dependency";
    case LoadReasonDynamicLoad:
        return L"dynamic_load";
    case LoadReasonAsImageLoad:
        return L"as_image_load";
    case LoadReasonAsDataLoad:
        return L"as_data_load";
    case LoadReasonEnclavePrimary:
        return L"enclave_primary";
    case LoadReasonEnclaveDependency:
        return L"enclave_dependency";
    }

    return NULL;
}

// What the file on disk says and who signed it. Verification is not cached here, so it is only done
// when the caller asked for the detail or is filtering on it.
VOID AtpAddModuleDetails(
    _In_ PVOID Row,
    _In_ PPH_STRING FileName,
    _In_ VERIFY_RESULT VerifyResult,
    _In_opt_ PPH_STRING Signer
    )
{
    PH_IMAGE_VERSION_INFO versionInfo;
    FILE_NETWORK_OPEN_INFORMATION fileInfo;
    PPH_STRING win32FileName;
    PVOID entry;

    AtJsonAddStringZ(Row, "verify_result", AtVerifyResultString(VerifyResult));
    AtJsonAddString(Row, "verify_signer", Signer);
    PhAddJsonObjectBoolean(Row, "is_microsoft_signed", AtIsMicrosoftSigned(FileName));

    win32FileName = PhGetFileName(FileName);

    if (win32FileName && NT_SUCCESS(PhInitializeImageVersionInfo(&versionInfo, win32FileName->Buffer)))
    {
        entry = PhCreateJsonObject();
        AtJsonAddString(entry, "company", versionInfo.CompanyName);
        AtJsonAddString(entry, "description", versionInfo.FileDescription);
        AtJsonAddString(entry, "file_version", versionInfo.FileVersion);
        AtJsonAddString(entry, "product", versionInfo.ProductName);
        PhAddJsonObjectValue(Row, "version_info", entry);
        PhDeleteImageVersionInfo(&versionInfo);
    }
    else
    {
        AtJsonAddNull(Row, "version_info");
    }

    if (win32FileName && NT_SUCCESS(PhQueryFullAttributesFileWin32(win32FileName->Buffer, &fileInfo)))
    {
        PhAddJsonObjectUInt64(Row, "file_size", fileInfo.EndOfFile.QuadPart);
        AtJsonAddTime(Row, "file_modified_time", &fileInfo.LastWriteTime);
    }
    else
    {
        AtJsonAddNull(Row, "file_size");
        AtJsonAddNull(Row, "file_modified_time");
    }

    PhClearReference(&win32FileName);
}

_Function_class_(PH_ENUM_GENERIC_MODULES_CALLBACK)
BOOLEAN NTAPI AtpModuleCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_opt_ PVOID Context
    )
{
    PAT_MODULE_CONTEXT context = Context;
    PVOID row;
    VERIFY_RESULT verifyResult = VrUnknown;
    PPH_STRING signer = NULL;
    BOOLEAN verified = FALSE;

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

    // Verification is needed for the unsigned_only filter as well as for the detail, so it happens
    // before the row is built and decides whether there is a row at all.
    if ((context->IncludeDetails || context->UnsignedOnly) && Module->FileName)
    {
        verifyResult = AtVerifyFileName(Module->FileName, &signer);
        verified = TRUE;
    }

    if (context->UnsignedOnly && (!verified || verifyResult == VrTrusted))
    {
        PhClearReference(&signer);
        return TRUE;
    }

    row = PhCreateJsonObject();
    AtJsonAddString(row, "name", Module->Name);
    AtJsonAddWin32FileName(row, "file_path", Module->FileName);
    AtJsonAddStringZ(row, "type", AtModuleTypeString(Module->Type));
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

    // Free from the module entry: where it wanted to be loaded, and why it was loaded at all.
    AtJsonAddPointer(row, "original_base_address", Module->OriginalBaseAddress);
    PhAddJsonObjectBoolean(
        row,
        "is_not_at_base",
        Module->OriginalBaseAddress != NULL && Module->OriginalBaseAddress != Module->BaseAddress
        );

    if (Module->LoadReason != USHRT_MAX)
        AtJsonAddStringZ(row, "load_reason", AtpLoadReasonString(Module->LoadReason));
    else
        AtJsonAddNull(row, "load_reason");

    if (verified)
    {
        AtpAddModuleDetails(row, Module->FileName, verifyResult, signer);
    }
    else if (context->IncludeDetails)
    {
        AtJsonAddNull(row, "verify_result");
        AtJsonAddNull(row, "verify_signer");
        AtJsonAddNull(row, "is_microsoft_signed");
        AtJsonAddNull(row, "version_info");
        AtJsonAddNull(row, "file_size");
        AtJsonAddNull(row, "file_modified_time");
    }

    PhClearReference(&signer);
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
    context.IncludeDetails = !Summary && AtJsonGetObjectBoolean(Call->Arguments, "include_details");
    context.UnsignedOnly = AtJsonGetObjectBoolean(Call->Arguments, "unsigned_only");

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

// ntdll keeps a small ring of unloaded modules, which outlives the module itself: a dll that was
// injected, did its work and unloaded leaves an entry here and nothing in the module list. The ring
// wraps.

VOID AtpGetProcessUnloadedModules(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    AT_TARGET target;
    AT_ROWS rows;
    PVOID eventTrace = NULL;
    PVOID currentEvent;
    ULONG elementSize = 0;
    ULONG elementCount = 0;
    BOOLEAN isWow64 = FALSE;
    PVOID structured;
    ULONG i;

    status = AtResolveProcessTarget(Call->Arguments, FALSE, PROCESS_QUERY_LIMITED_INFORMATION, &target, Result);

    if (!NT_SUCCESS(status))
        return;

    status = PhGetProcessUnloadedDlls(
        target.ProcessItem->ProcessId,
        &eventTrace,
        &elementSize,
        &elementCount
        );

    // A process that has never unloaded anything has no trace at all, which is an empty list and
    // not a failure to read one.
    if (!NT_SUCCESS(status) && status != STATUS_NOT_FOUND)
    {
        AtSetToolStatusError(Result, status, L"Reading the unloaded module trace");
        AtDeleteTarget(&target);
        return;
    }

    if (target.ProcessHandle)
        PhGetProcessIsWow64(target.ProcessHandle, &isWow64);

    AtInitializeRows(&rows, Call->Arguments);
    currentEvent = eventTrace;

    for (i = 0; NT_SUCCESS(status) && i < elementCount; i++)
    {
        PRTL_UNLOAD_EVENT_TRACE event = currentEvent;
        LARGE_INTEGER time;
        LARGE_INTEGER now;
        PPH_STRING name;
        PPH_STRING version;
        PVOID row;

        // The ring is a fixed array that is only partly filled; a null base is where it ends.
        if (!event->BaseAddress)
            break;

        row = PhCreateJsonObject();
        PhAddJsonObjectUInt64(row, "sequence", event->Sequence);

        // The name is a fixed field and is not guaranteed to be terminated.
        name = PhCreateStringEx(event->ImageName, sizeof(event->ImageName));
        PhTrimToNullTerminatorString(name);
        AtJsonAddString(row, "name", name);
        PhClearReference(&name);

        AtJsonAddPointer(row, "base_address", event->BaseAddress);
        PhAddJsonObjectUInt64(row, "size", event->SizeOfImage);
        AtJsonAddHex(row, "checksum", event->CheckSum);

        // A reproducible build puts a content hash in this field instead of a time, and a hash read
        // as seconds since 1970 lands in the future, so a date is reported only when the value can
        // be one.
        AtJsonAddHex(row, "time_date_stamp", event->TimeDateStamp);
        PhSecondsSince1970ToTime(event->TimeDateStamp, &time);
        PhQuerySystemTime(&now);

        if (event->TimeDateStamp && time.QuadPart <= now.QuadPart)
            AtJsonAddTime(row, "time_date_stamp_utc", &time);
        else
            AtJsonAddNull(row, "time_date_stamp_utc");

        version = PhFormatString(
            L"%hu.%hu.%hu.%hu",
            HIWORD(event->Version[0]),
            LOWORD(event->Version[0]),
            HIWORD(event->Version[1]),
            LOWORD(event->Version[1])
            );
        AtJsonAddString(row, "version", version);
        PhClearReference(&version);

        AtAddRow(&rows, row);

        currentEvent = PTR_ADD_OFFSET(currentEvent, elementSize);
    }

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, target.ProcessItem);
    AtAddRows(structured, "modules", &rows);
    PhAddJsonObjectBoolean(structured, "is_wow64", isWow64);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteRows(&rows);

    if (eventTrace)
        PhFree(eventTrace);

    AtDeleteTarget(&target);
}

#define AT_COHERENCY_DEFAULT_MODULES 32
#define AT_COHERENCY_MAXIMUM_MODULES 512

typedef struct _AT_FIND_MODULE_CONTEXT
{
    PVOID Address;
    PPH_STRING Name;
    BOOLEAN Found;
    PVOID BaseAddress;
    SIZE_T Size;
    PPH_STRING FileName;
} AT_FIND_MODULE_CONTEXT, *PAT_FIND_MODULE_CONTEXT;

_Function_class_(PH_ENUM_GENERIC_MODULES_CALLBACK)
BOOLEAN NTAPI AtpFindModuleCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_opt_ PVOID Context
    )
{
    PAT_FIND_MODULE_CONTEXT context = Context;

    if (!context || context->Found || !Module->BaseAddress)
        return TRUE;

    if (context->Address)
    {
        // Anywhere inside the module, so an address from a stack frame or a page finds its owner.
        if ((ULONG_PTR)context->Address < (ULONG_PTR)Module->BaseAddress ||
            (ULONG_PTR)context->Address >= (ULONG_PTR)Module->BaseAddress + Module->Size)
        {
            return TRUE;
        }
    }
    else if (context->Name)
    {
        if (!Module->Name || !PhEqualString(Module->Name, context->Name, TRUE))
            return TRUE;
    }
    else if (Module->LoadOrderIndex != 0)
    {
        // No address and no name: the process's own image, which is always the first module.
        return TRUE;
    }

    context->Found = TRUE;
    context->BaseAddress = Module->BaseAddress;
    context->Size = Module->Size;
    PhSetReference(&context->FileName, Module->FileName);

    return TRUE;
}

_Success_(return)
BOOLEAN AtFindProcessModule(
    _In_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle,
    _In_opt_ PVOID Address,
    _In_opt_ PPH_STRING Name,
    _Out_ PVOID *BaseAddress,
    _Out_ PSIZE_T Size,
    _Out_ PPH_STRING *FileName
    )
{
    AT_FIND_MODULE_CONTEXT context;

    memset(&context, 0, sizeof(AT_FIND_MODULE_CONTEXT));
    context.Address = Address;
    context.Name = Name;

    PhEnumGenericModules(ProcessId, ProcessHandle, 0, AtpFindModuleCallback, &context);

    if (!context.Found)
    {
        PhClearReference(&context.FileName);
        return FALSE;
    }

    *BaseAddress = context.BaseAddress;
    *Size = context.Size;
    *FileName = context.FileName;

    return TRUE;
}

// How much of an image in memory still matches the file it was loaded from. A packer, a hollowed
// process and inline hooks all leave the mapped copy disagreeing with the file.

PH_IMAGE_COHERENCY_SCAN_TYPE AtpCoherencyScanType(
    _In_opt_ PPH_STRING Name
    )
{
    if (!Name)
        return PhImageCoherencyQuick;

    if (PhEqualString2(Name, L"normal", TRUE))
        return PhImageCoherencyNormal;
    if (PhEqualString2(Name, L"full", TRUE))
        return PhImageCoherencyFull;
    if (PhEqualString2(Name, L"shared_original", TRUE))
        return PhImageCoherencySharedOriginal;

    return PhImageCoherencyQuick;
}

VOID AtpAddCoherency(
    _In_ PVOID Object,
    _In_ NTSTATUS Status,
    _In_ FLOAT Coherency
    )
{
    if (NT_SUCCESS(Status))
    {
        PhAddJsonObjectDouble(Object, "coherency", Coherency);
        AtJsonAddNull(Object, "error");
        AtJsonAddNull(Object, "message");
    }
    else
    {
        PPH_STRING message = PhGetStatusMessage(Status, 0);

        // A number is not invented for an image that could not be compared: without the file, or
        // without the rights to read the mapping, there is no ratio to report.
        AtJsonAddNull(Object, "coherency");
        PhAddJsonObject(Object, "error", Status == STATUS_ACCESS_DENIED ? "access_denied" : "failed");
        AtJsonAddStringZ(Object, "message", PhGetStringOrDefault(message, L"unknown error"));
        PhClearReference(&message);
    }
}

typedef struct _AT_COHERENCY_CONTEXT
{
    AT_ROWS Rows;
    PPH_STRING NameContains;
    HANDLE ProcessHandle;
    PH_IMAGE_COHERENCY_SCAN_TYPE ScanType;
    ULONG Limit;
} AT_COHERENCY_CONTEXT, *PAT_COHERENCY_CONTEXT;

_Function_class_(PH_ENUM_GENERIC_MODULES_CALLBACK)
BOOLEAN NTAPI AtpCoherencyModuleCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_opt_ PVOID Context
    )
{
    PAT_COHERENCY_CONTEXT context = Context;
    NTSTATUS status;
    FLOAT coherency = 0;
    PVOID row;

    if (!context || !Module->FileName)
        return TRUE;

    if (context->NameContains &&
        !AtContainsString(Module->Name, context->NameContains) &&
        !AtContainsString(Module->FileName, context->NameContains))
    {
        return TRUE;
    }

    // Every module is a scan of its own, so the number of them is capped rather than the time.
    if (context->Rows.TotalCount >= context->Limit)
        return TRUE;

    status = PhGetProcessModuleImageCoherency(
        Module->FileName,
        context->ProcessHandle,
        Module->BaseAddress,
        Module->Size,
        FALSE,
        context->ScanType,
        &coherency
        );

    row = PhCreateJsonObject();
    AtJsonAddString(row, "name", Module->Name);
    AtJsonAddWin32FileName(row, "file_path", Module->FileName);
    AtJsonAddPointer(row, "base_address", Module->BaseAddress);
    PhAddJsonObjectUInt64(row, "size", Module->Size);
    AtpAddCoherency(row, status, coherency);

    AtAddRow(&context->Rows, row);

    return TRUE;
}

VOID AtpGetProcessImageCoherency(
    _In_ PAT_TOOL_CALL Call,
    _In_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    AT_COHERENCY_CONTEXT context;
    PPH_STRING scanTypeName;
    FLOAT coherency = 0;
    ULONG64 maxModules = AT_COHERENCY_DEFAULT_MODULES;
    PVOID structured;

    memset(&context, 0, sizeof(AT_COHERENCY_CONTEXT));

    if (!Target->ProcessItem->FileName)
    {
        AtSetToolError(Result, "failed", STATUS_NOT_FOUND, L"The process has no image file to compare against.");
        return;
    }

    scanTypeName = AtGetArgumentString(Call->Arguments, "scan_type");
    context.ScanType = AtpCoherencyScanType(scanTypeName);

    status = PhGetProcessImageCoherency(
        Target->ProcessItem->FileName,
        Target->ProcessItem->ProcessId,
        context.ScanType,
        &coherency
        );

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, Target->ProcessItem);
    AtJsonAddWin32FileName(structured, "file_path", Target->ProcessItem->FileName);
    AtJsonAddStringZ(structured, "scan_type", scanTypeName ? PhGetString(scanTypeName) : L"quick");
    AtpAddCoherency(structured, status, coherency);

    if (AtJsonGetObjectBoolean(Call->Arguments, "include_modules"))
    {
        if (AtGetArgumentUInt64(Call->Arguments, "max_modules", &maxModules) && maxModules > 0)
            maxModules = min(maxModules, AT_COHERENCY_MAXIMUM_MODULES);

        context.Limit = (ULONG)maxModules;
        context.NameContains = AtGetArgumentString(Call->Arguments, "name_contains");
        context.ProcessHandle = Target->ProcessHandle;

        AtInitializeRows(&context.Rows, Call->Arguments);
        PhEnumGenericModules(Target->ProcessItem->ProcessId, Target->ProcessHandle, 0, AtpCoherencyModuleCallback, &context);
        AtAddRows(structured, "modules", &context.Rows);
        AtDeleteRows(&context.Rows);
        PhClearReference(&context.NameContains);
    }
    else
    {
        AtJsonAddNull(structured, "modules");
    }

    AtAddSnapshot(structured);
    Result->StructuredContent = structured;

    PhClearReference(&scanTypeName);
}

// Windows maps an image copy-on-write, so a page that has been written to - an inline hook, a
// patched jump table - stops being backed by the file and says so in its working set attributes.

typedef struct _AT_PAGE_MODIFICATION_CONTEXT
{
    AT_ROWS Rows;
    PPH_SYMBOL_PROVIDER SymbolProvider;
    SIZE_T PageCount;
    SIZE_T ModifiedCount;
} AT_PAGE_MODIFICATION_CONTEXT, *PAT_PAGE_MODIFICATION_CONTEXT;

_Function_class_(PH_ENUM_MEMORY_ATTRIBUTE_CALLBACK)
NTSTATUS NTAPI AtpPageModificationCallback(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T SizeOfImage,
    _In_ ULONG_PTR NumberOfEntries,
    _In_ PMEMORY_WORKING_SET_EX_INFORMATION Blocks,
    _In_ PVOID Context
    )
{
    PAT_PAGE_MODIFICATION_CONTEXT context = Context;
    ULONG_PTR i;

    UNREFERENCED_PARAMETER(ProcessHandle);
    UNREFERENCED_PARAMETER(BaseAddress);
    UNREFERENCED_PARAMETER(SizeOfImage);

    if (!context)
        return STATUS_SUCCESS;

    for (i = 0; i < NumberOfEntries; i++)
    {
        PMEMORY_WORKING_SET_EX_INFORMATION page = &Blocks[i];
        PVOID row;

        context->PageCount++;

        // SharedOriginal is the whole test: the page still comes from the file's mapping. A page
        // that has lost that is a page this process wrote over.
        if (page->VirtualAttributes.SharedOriginal)
            continue;

        context->ModifiedCount++;

        row = PhCreateJsonObject();
        AtJsonAddPointer(row, "address", page->VirtualAddress);
        PhAddJsonObjectBoolean(row, "valid", !!page->VirtualAttributes.Valid);

        if (context->SymbolProvider)
        {
            PPH_STRING symbol;

            symbol = PhGetSymbolFromAddress(context->SymbolProvider, page->VirtualAddress, NULL, NULL, NULL, NULL);
            AtJsonAddString(row, "symbol", symbol);
            PhClearReference(&symbol);
        }
        else
        {
            AtJsonAddNull(row, "symbol");
        }

        AtAddRow(&context->Rows, row);
    }

    return STATUS_SUCCESS;
}

VOID AtpGetImagePageModifications(
    _In_ PAT_TOOL_CALL Call,
    _In_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    AT_PAGE_MODIFICATION_CONTEXT context;
    PPH_STRING moduleName;
    PPH_STRING fileName = NULL;
    PVOID baseAddress = NULL;
    SIZE_T size = 0;
    ULONG64 address;
    PVOID structured;

    memset(&context, 0, sizeof(AT_PAGE_MODIFICATION_CONTEXT));

    moduleName = AtGetArgumentString(Call->Arguments, "module_name");

    if (AtGetArgumentPointer(Call->Arguments, "base_address", &address) && address != 0)
    {
        if (!AtFindProcessModule(Target->ProcessItem->ProcessId, Target->ProcessHandle, (PVOID)address, NULL, &baseAddress, &size, &fileName))
        {
            AtSetToolError(Result, "not_found", STATUS_NOT_FOUND, L"No module of that process is loaded at that address.");
            PhClearReference(&moduleName);
            return;
        }
    }
    else if (!AtFindProcessModule(Target->ProcessItem->ProcessId, Target->ProcessHandle, NULL, moduleName, &baseAddress, &size, &fileName))
    {
        AtSetToolError(
            Result,
            "not_found",
            STATUS_NOT_FOUND,
            moduleName ? L"That process has no module named %s." : L"The process's own image could not be found in its module list.",
            PhGetStringOrEmpty(moduleName)
            );
        PhClearReference(&moduleName);
        return;
    }

    if (AtJsonGetObjectBoolean(Call->Arguments, "resolve_symbols"))
        context.SymbolProvider = AtCreateSymbolProvider(Target->ProcessItem->ProcessId);

    AtInitializeRows(&context.Rows, Call->Arguments);

    status = PhEnumVirtualMemoryAttributes(
        Target->ProcessHandle,
        baseAddress,
        size,
        AtpPageModificationCallback,
        &context
        );

    if (!NT_SUCCESS(status) && context.PageCount == 0)
    {
        AtSetToolStatusError(Result, status, L"Reading the page attributes");
        AtDeleteRows(&context.Rows);

        if (context.SymbolProvider)
            PhDereferenceObject(context.SymbolProvider);

        PhClearReference(&fileName);
        PhClearReference(&moduleName);
        return;
    }

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, Target->ProcessItem);
    AtJsonAddWin32FileName(structured, "file_path", fileName);
    AtJsonAddPointer(structured, "base_address", baseAddress);
    PhAddJsonObjectUInt64(structured, "size", size);
    AtAddRows(structured, "pages", &context.Rows);
    PhAddJsonObjectUInt64(structured, "page_count", context.PageCount);
    PhAddJsonObjectUInt64(structured, "modified_count", context.ModifiedCount);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteRows(&context.Rows);

    if (context.SymbolProvider)
        PhDereferenceObject(context.SymbolProvider);

    PhClearReference(&fileName);
    PhClearReference(&moduleName);
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
    case AtActionGetProcessUnloadedModules:
        AtpGetProcessUnloadedModules(Call, Result);
        break;
    case AtActionGetProcessImageCoherency:
        AtpGetProcessImageCoherency(Call, Target, Result);
        break;
    case AtActionGetImagePageModifications:
        AtpGetImagePageModifications(Call, Target, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
