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

//
// Modules
//
//
// Handles
//

typedef struct _AT_TYPE_COUNT
{
    PPH_STRING Name;
    ULONG Count;
} AT_TYPE_COUNT, *PAT_TYPE_COUNT;

VOID AtpAddHandleRow(
    _In_ PVOID Row,
    _In_ PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Handle,
    _In_opt_ PPH_STRING TypeName
    )
{
    AtJsonAddPointer(Row, "handle", Handle->HandleValue);
    AtJsonAddString(Row, "type_name", TypeName);
    AtJsonAddPointer(Row, "granted_access", (PVOID)(ULONG_PTR)Handle->GrantedAccess);
    PhAddJsonObjectUInt64(Row, "attributes", Handle->HandleAttributes);
    PhAddJsonObjectBoolean(Row, "inherit", !!FlagOn(Handle->HandleAttributes, OBJ_INHERIT));
    PhAddJsonObjectBoolean(Row, "protect_from_close", !!FlagOn(Handle->HandleAttributes, OBJ_PROTECT_CLOSE));
}

VOID AtpGetProcessHandles(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _In_ PAT_TARGET SensitiveTarget,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    AT_TARGET localTarget;
    PAT_TARGET target;
    PSYSTEM_HANDLE_INFORMATION_EX handles;
    HANDLE processHandle = NULL;
    PPH_STRING typeFilter;
    PPH_STRING nameFilter;
    BOOLEAN detailed = Tool->Action == AtActionGetProcessHandlesDetailed;
    BOOLEAN countsOnly = !detailed && AtJsonGetObjectBoolean(Call->Arguments, "counts_only");
    PVOID structured;
    PVOID rows;
    PVOID counts;
    PPH_LIST typeCounts;
    ULONG_PTR i;
    ULONG count = 0;

    memset(&localTarget, 0, sizeof(AT_TARGET));

    // The detailed (sensitive) form is resolved and its process opened by the caller, held across
    // consent; the plain form resolves its own read-only target here.
    if (detailed)
    {
        target = SensitiveTarget;
    }
    else
    {
        if (!NT_SUCCESS(AtResolveProcessTarget(Call->Arguments, FALSE, 0, &localTarget, Result)))
            return;

        target = &localTarget;
    }

    status = PhEnumHandlesEx(&handles);

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Enumerating handles");
        AtDeleteTarget(&localTarget);
        return;
    }

    // Detailed reads use the resolved handle; plain reads open just enough to name types.
    if (detailed)
        processHandle = target->ProcessHandle;
    else
        PhOpenProcess(&processHandle, PROCESS_DUP_HANDLE | PROCESS_QUERY_LIMITED_INFORMATION, target->ProcessItem->ProcessId);

    typeFilter = AtGetArgumentString(Call->Arguments, "type_name");
    nameFilter = AtGetArgumentString(Call->Arguments, "name_contains");

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, target->ProcessItem);
    rows = PhCreateJsonArray();
    typeCounts = PhCreateList(16); // AT_TYPE_COUNT entries; avoids dynamic JSON keys

    for (i = 0; i < handles->NumberOfHandles; i++)
    {
        PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX entry = &handles->Handles[i];
        PPH_STRING typeName = NULL;
        PPH_STRING objectName = NULL;
        PPH_STRING bestName = NULL;
        PVOID row;

        if (entry->UniqueProcessId != target->ProcessItem->ProcessId)
            continue;

        if (processHandle)
        {
            if (detailed)
            {
                PhGetHandleInformation(processHandle, entry->HandleValue, entry->ObjectTypeIndex, NULL, &typeName, &objectName, &bestName);
            }
            else
            {
                PhGetObjectTypeName(processHandle, entry->HandleValue, entry->ObjectTypeIndex, &typeName);
            }
        }

        if (typeFilter && (!typeName || !PhEqualString(typeName, typeFilter, TRUE)))
            goto Next;

        if (detailed && nameFilter && !AtContainsString(bestName, nameFilter) && !AtContainsString(objectName, nameFilter))
            goto Next;

        // Per-type counts always; a null type name is not counted (bucketed nowhere).
        if (typeName)
        {
            PAT_TYPE_COUNT typeCount = NULL;
            ULONG j;

            for (j = 0; j < typeCounts->Count; j++)
            {
                PAT_TYPE_COUNT candidate = typeCounts->Items[j];

                if (PhEqualString(candidate->Name, typeName, FALSE))
                {
                    typeCount = candidate;
                    break;
                }
            }

            if (!typeCount)
            {
                typeCount = PhAllocate(sizeof(AT_TYPE_COUNT));
                typeCount->Name = PhReferenceObject(typeName);
                typeCount->Count = 0;
                PhAddItemList(typeCounts, typeCount);
            }

            typeCount->Count++;
        }

        count++;

        if (!countsOnly)
        {
            row = PhCreateJsonObject();
            AtpAddHandleRow(row, entry, typeName);

            if (detailed)
            {
                AtJsonAddString(row, "object_name", objectName);
                AtJsonAddString(row, "best_name", bestName);
                AtJsonAddPointer(row, "object_address", entry->Object);
            }

            PhAddJsonArrayObject(rows, row);
        }

Next:
        PhClearReference(&typeName);
        PhClearReference(&objectName);
        PhClearReference(&bestName);
    }

    if (!detailed && processHandle)
        NtClose(processHandle);

    PhFree(handles);
    PhClearReference(&typeFilter);
    PhClearReference(&nameFilter);

    if (!detailed)
    {
        counts = PhCreateJsonArray();

        for (i = 0; i < typeCounts->Count; i++)
        {
            PAT_TYPE_COUNT typeCount = typeCounts->Items[i];
            PVOID entry = PhCreateJsonObject();

            AtJsonAddString(entry, "type_name", typeCount->Name);
            PhAddJsonObjectUInt64(entry, "count", typeCount->Count);
            PhAddJsonArrayObject(counts, entry);
        }

        PhAddJsonObjectValue(structured, "counts_by_type", counts);
    }

    for (i = 0; i < typeCounts->Count; i++)
    {
        PAT_TYPE_COUNT typeCount = typeCounts->Items[i];

        PhDereferenceObject(typeCount->Name);
        PhFree(typeCount);
    }

    PhDereferenceObject(typeCounts);

    if (countsOnly)
        PhFreeJsonObject(rows);
    else
        PhAddJsonObjectValue(structured, "handles", rows);

    PhAddJsonObjectUInt64(structured, "count", count);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteTarget(&localTarget);
}

VOID AtpCloseHandle(
    _In_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    PVOID structured;

    if (FlagOn(Target->HandleAttributes, OBJ_PROTECT_CLOSE))
    {
        AtSetToolError(Result, "protected", STATUS_HANDLE_NOT_CLOSABLE, L"This handle is marked protect-from-close and cannot be closed.");
        return;
    }

    // Duplicate with DUPLICATE_CLOSE_SOURCE and no target: closes the source handle, as the UI does.
    status = NtDuplicateObject(
        Target->ProcessHandle,
        Target->HandleValue,
        NULL,
        NULL,
        0,
        0,
        DUPLICATE_CLOSE_SOURCE
        );

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Closing the handle");
        return;
    }

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, Target->ProcessItem);
    PhAddJsonObject(structured, "action", "close_handle");
    AtJsonAddPointer(structured, "handle", Target->HandleValue);
    AtJsonAddString(structured, "type_name", Target->HandleTypeName);
    AtJsonAddString(structured, "object_name", Target->HandleObjectName);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

//
// Memory regions
//

PCWSTR AtpMemoryStateString(
    _In_ ULONG State
    )
{
    if (State & MEM_COMMIT)
        return L"commit";
    if (State & MEM_RESERVE)
        return L"reserve";

    return L"free";
}

PCWSTR AtpMemoryTypeString(
    _In_ ULONG Type
    )
{
    if (Type & MEM_PRIVATE)
        return L"private";
    if (Type & MEM_MAPPED)
        return L"mapped";
    if (Type & MEM_IMAGE)
        return L"image";

    return NULL;
}

PCWSTR AtpMemoryProtectionString(
    _In_ ULONG Protection
    )
{
    if (Protection == 0)
        return NULL;
    if (Protection & PAGE_NOACCESS)
        return L"NA";
    if (Protection & PAGE_EXECUTE_WRITECOPY)
        return L"RWXC";
    if (Protection & PAGE_EXECUTE_READWRITE)
        return L"RWX";
    if (Protection & PAGE_EXECUTE_READ)
        return L"RX";
    if (Protection & PAGE_EXECUTE)
        return L"X";
    if (Protection & PAGE_WRITECOPY)
        return L"WC";
    if (Protection & PAGE_READWRITE)
        return L"RW";
    if (Protection & PAGE_READONLY)
        return L"R";

    return NULL;
}

VOID AtpGetProcessMemoryRegions(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    AT_TARGET target;
    PH_MEMORY_ITEM_LIST list;
    ULONG flags = PH_QUERY_MEMORY_REGION_TYPE | PH_QUERY_MEMORY_WS_COUNTERS;
    BOOLEAN includeFree;
    BOOLEAN allocationsOnly;
    PLIST_ENTRY entry;
    PVOID structured;
    PVOID regions;
    ULONG count = 0;

    if (!NT_SUCCESS(AtResolveProcessTarget(Call->Arguments, FALSE, 0, &target, Result)))
        return;

    includeFree = AtJsonGetObjectBoolean(Call->Arguments, "include_free");
    allocationsOnly = AtJsonGetObjectBoolean(Call->Arguments, "allocations_only");

    if (!includeFree)
        flags |= PH_QUERY_MEMORY_IGNORE_FREE;

    status = PhQueryMemoryItemList(target.ProcessItem->ProcessId, flags, &list);

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Querying memory");
        AtDeleteTarget(&target);
        return;
    }

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, target.ProcessItem);
    regions = PhCreateJsonArray();

    for (entry = list.ListHead.Flink; entry != &list.ListHead; entry = entry->Flink)
    {
        PPH_MEMORY_ITEM item = CONTAINING_RECORD(entry, PH_MEMORY_ITEM, ListEntry);
        PPH_STRING use;
        PVOID row;

        if (allocationsOnly && item->AllocationBase != item->BaseAddress)
            continue;

        row = PhCreateJsonObject();
        AtJsonAddPointer(row, "base_address", item->BaseAddress);
        AtJsonAddPointer(row, "allocation_base", item->AllocationBase);
        PhAddJsonObjectUInt64(row, "size", item->RegionSize);
        AtJsonAddStringZ(row, "state", AtpMemoryStateString(item->State));
        AtJsonAddStringZ(row, "type", AtpMemoryTypeString(item->Type));

        if (item->State & (MEM_COMMIT | MEM_RESERVE))
        {
            AtJsonAddStringZ(row, "protection", AtpMemoryProtectionString(item->Protect));
            AtJsonAddStringZ(row, "allocation_protection", AtpMemoryProtectionString(item->AllocationProtect));
        }
        else
        {
            AtJsonAddNull(row, "protection");
            AtJsonAddNull(row, "allocation_protection");
        }

        use = PhGetMemoryRegionUseText(item);
        AtJsonAddString(row, "use", use);
        PhClearReference(&use);

        PhAddJsonObjectUInt64(row, "committed_bytes", item->CommittedSize);
        PhAddJsonObjectUInt64(row, "private_bytes", item->PrivateSize);

        PhAddJsonArrayObject(regions, row);
        count++;
    }

    PhDeleteMemoryItemList(&list);

    PhAddJsonObjectValue(structured, "regions", regions);
    PhAddJsonObjectUInt64(structured, "count", count);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteTarget(&target);
}

//
// Minidump
//

VOID AtpCreateProcessMinidump(
    _In_ PAT_TOOL_CALL Call,
    _In_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    HRESULT result;
    HANDLE fileHandle;
    MINIDUMP_TYPE dumpType;
    LARGE_INTEGER fileSize = { 0 };
    PVOID structured;

    // Target->Parameter holds the validated absolute path; full_memory comes from the arguments.
    dumpType = AtJsonGetObjectBoolean(Call->Arguments, "full_memory")
        ? (MiniDumpWithFullMemory | MiniDumpWithHandleData | MiniDumpWithUnloadedModules | MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo | MiniDumpWithTokenInformation)
        : (MiniDumpWithHandleData | MiniDumpWithUnloadedModules | MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo);

    // Create the file; FILE_CREATE fails if it already exists so we never overwrite.
    status = PhCreateFileWin32(
        &fileHandle,
        PhGetString(Target->Parameter),
        FILE_GENERIC_WRITE | DELETE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_DELETE,
        FILE_CREATE,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Creating the dump file");
        return;
    }

    result = PhWriteMiniDumpProcess(
        Target->ProcessHandle,
        Target->ProcessItem->ProcessId,
        fileHandle,
        dumpType,
        NULL,
        NULL,
        NULL
        );

    if (HR_SUCCESS(result))
        PhGetFileSize(fileHandle, &fileSize);

    if (!HR_SUCCESS(result))
    {
        // Delete the partial file so a failed dump leaves nothing behind.
        PhSetFileDelete(fileHandle);
        NtClose(fileHandle);
        AtSetToolError(Result, "failed", (NTSTATUS)result, L"Writing the dump failed (0x%08x).", result);
        return;
    }

    NtClose(fileHandle);

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, Target->ProcessItem);
    PhAddJsonObject(structured, "action", "create_process_minidump");
    AtJsonAddString(structured, "path", Target->Parameter);
    PhAddJsonObjectUInt64(structured, "size", fileSize.QuadPart);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

VOID AtMemoryInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    switch (Tool->Action)
    {
    case AtActionGetProcessHandles:
    case AtActionGetProcessHandlesDetailed:
        AtpGetProcessHandles(Tool, Call, Target, Result);
        break;
    case AtActionGetProcessMemoryRegions:
        AtpGetProcessMemoryRegions(Call, Result);
        break;
    case AtActionCreateProcessMinidump:
        AtpCreateProcessMinidump(Call, Target, Result);
        break;
    case AtActionCloseHandle:
        AtpCloseHandle(Target, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
