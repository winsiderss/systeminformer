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
    AtJsonAddHex(Row, "granted_access", Handle->GrantedAccess);
    PhAddJsonObjectUInt64(Row, "attributes", Handle->HandleAttributes);
    PhAddJsonObjectBoolean(Row, "inherit", !!FlagOn(Handle->HandleAttributes, OBJ_INHERIT));
    PhAddJsonObjectBoolean(Row, "protect_from_close", !!FlagOn(Handle->HandleAttributes, OBJ_PROTECT_CLOSE));
}

// The access mask spelled out against the type's own rights, so an agent does not have to know that
// 0x1f0fff means "everything" for a process and something else for a key.
VOID AtpAddGrantedAccessSymbolic(
    _In_ PVOID Row,
    _In_ ACCESS_MASK GrantedAccess,
    _In_opt_ PPH_STRING TypeName
    )
{
    PPH_ACCESS_ENTRY accessEntries;
    ULONG numberOfAccessEntries;
    PPH_STRING accessString;

    if (!TypeName || GrantedAccess == 0)
    {
        AtJsonAddNull(Row, "granted_access_symbolic");
        return;
    }

    if (!PhGetAccessEntries(PhGetString(TypeName), &accessEntries, &numberOfAccessEntries))
    {
        AtJsonAddNull(Row, "granted_access_symbolic");
        return;
    }

    accessString = PhGetAccessString(GrantedAccess, accessEntries, numberOfAccessEntries);
    AtJsonAddString(Row, "granted_access_symbolic", accessString);

    PhClearReference(&accessString);
    PhFree(accessEntries);
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
    AT_ROWS rows;
    PVOID structured;
    PVOID counts;
    PPH_LIST typeCounts;
    ULONG_PTR i;

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
    AtInitializeRows(&rows, Call->Arguments);
    typeCounts = PhCreateList(16); // AT_TYPE_COUNT entries; avoids dynamic JSON keys

    for (i = 0; i < handles->NumberOfHandles; i++)
    {
        PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX entry = &handles->Handles[i];
        PPH_STRING typeName = NULL;
        PPH_STRING objectName = NULL;
        PPH_STRING bestName = NULL;
        OBJECT_BASIC_INFORMATION basicInfo;
        BOOLEAN haveBasicInfo = FALSE;
        PVOID row;

        if (entry->UniqueProcessId != target->ProcessItem->ProcessId)
            continue;

        if (processHandle)
        {
            if (detailed)
            {
                // The same call that names the object also returns how many handles and references
                // the object has and what it costs the pools, so the detail is free here.
                haveBasicInfo = NT_SUCCESS(PhGetHandleInformationEx(
                    processHandle,
                    entry->HandleValue,
                    entry->ObjectTypeIndex,
                    0,
                    NULL,
                    &basicInfo,
                    &typeName,
                    &objectName,
                    &bestName,
                    NULL
                    ));
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

        if (countsOnly)
        {
            // Counted for counts_by_type and total_count; no row is built for it.
            AtAddRow(&rows, NULL);
        }
        else
        {
            row = PhCreateJsonObject();
            AtpAddHandleRow(row, entry, typeName);

            if (detailed)
            {
                AtJsonAddString(row, "object_name", objectName);
                AtJsonAddString(row, "best_name", bestName);
                AtJsonAddPointer(row, "object_address", entry->Object);
                AtpAddGrantedAccessSymbolic(row, entry->GrantedAccess, typeName);

                if (haveBasicInfo)
                {
                    // How many other handles exist to the same object, which is what says whether
                    // closing this one actually releases anything.
                    PhAddJsonObjectUInt64(row, "handle_count", basicInfo.HandleCount);
                    PhAddJsonObjectUInt64(row, "pointer_count", basicInfo.PointerCount);
                    PhAddJsonObjectUInt64(row, "paged_pool_charge", basicInfo.PagedPoolCharge);
                    PhAddJsonObjectUInt64(row, "non_paged_pool_charge", basicInfo.NonPagedPoolCharge);
                }
                else
                {
                    AtJsonAddNull(row, "handle_count");
                    AtJsonAddNull(row, "pointer_count");
                    AtJsonAddNull(row, "paged_pool_charge");
                    AtJsonAddNull(row, "non_paged_pool_charge");
                }
            }

            AtAddRow(&rows, row);
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

    AtAddRows(structured, "handles", &rows);
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

PCWSTR AtpMemoryRegionTypeString(
    _In_ PH_MEMORY_REGION_TYPE Type
    )
{
    switch (Type)
    {
    case CustomRegion:
        return L"custom";
    case UnusableRegion:
        return L"unusable";
    case MappedFileRegion:
        return L"mapped_file";
    case UserSharedDataRegion:
        return L"user_shared_data";
    case PebRegion:
    case Peb32Region:
        return L"peb";
    case TebRegion:
    case Teb32Region:
        return L"teb";
    case StackRegion:
    case Stack32Region:
        return L"stack";
    case HeapRegion:
    case Heap32Region:
        return L"heap";
    case HeapSegmentRegion:
    case HeapSegment32Region:
        return L"heap_segment";
    case CfgBitmapRegion:
    case CfgBitmap32Region:
        return L"cfg_bitmap";
    case ApiSetMapRegion:
        return L"api_set_map";
    case HypervisorSharedDataRegion:
        return L"hypervisor_shared_data";
    case ReadOnlySharedMemoryRegion:
        return L"read_only_shared_memory";
    case CodePageDataRegion:
        return L"code_page_data";
    case GdiSharedHandleTableRegion:
        return L"gdi_shared_handle_table";
    case ShimDataRegion:
        return L"shim_data";
    case ActivationContextDataRegion:
        return L"activation_context_data";
    case WerRegistrationDataRegion:
        return L"wer_registration_data";
    case SiloSharedDataRegion:
        return L"silo_shared_data";
    case TelemetryCoverageRegion:
        return L"telemetry_coverage";
    case ProcessParametersRegion:
        return L"process_parameters";
    case LeapSecondDataRegion:
        return L"leap_second_data";
    case DesktopHeapRegion:
        return L"desktop_heap";
    }

    return NULL;
}

// How the kernel judged the image backing this region. Unsigned executable memory in a process that
// should only be running signed code is the point of asking.
PCWSTR AtpSigningLevelString(
    _In_ SE_SIGNING_LEVEL SigningLevel
    )
{
    switch (SigningLevel)
    {
    case SE_SIGNING_LEVEL_UNCHECKED:
        return L"unchecked";
    case SE_SIGNING_LEVEL_UNSIGNED:
        return L"unsigned";
    case SE_SIGNING_LEVEL_ENTERPRISE:
        return L"enterprise";
    case SE_SIGNING_LEVEL_DEVELOPER:
        return L"developer";
    case SE_SIGNING_LEVEL_AUTHENTICODE:
        return L"authenticode";
    case SE_SIGNING_LEVEL_STORE:
        return L"store";
    case SE_SIGNING_LEVEL_ANTIMALWARE:
        return L"antimalware";
    case SE_SIGNING_LEVEL_MICROSOFT:
        return L"microsoft";
    case SE_SIGNING_LEVEL_DYNAMIC_CODEGEN:
        return L"dynamic_codegen";
    case SE_SIGNING_LEVEL_WINDOWS:
        return L"windows";
    case SE_SIGNING_LEVEL_WINDOWS_TCB:
        return L"windows_tcb";
    }

    return NULL;
}

VOID AtpAddRegionFlags(
    _In_ PVOID Row,
    _In_ PPH_MEMORY_ITEM Item
    )
{
    PVOID array = PhCreateJsonArray();

    #define AT_ADD_REGION_FLAG(Member, Name) \
        if (Item->Member) PhAddJsonArrayObject(array, PhCreateJsonStringObject(Name))

    AT_ADD_REGION_FLAG(Private, "private");
    AT_ADD_REGION_FLAG(MappedDataFile, "mapped_data_file");
    AT_ADD_REGION_FLAG(MappedImage, "mapped_image");
    AT_ADD_REGION_FLAG(MappedPageFile, "mapped_page_file");
    AT_ADD_REGION_FLAG(MappedPhysical, "mapped_physical");
    AT_ADD_REGION_FLAG(DirectMapped, "direct_mapped");
    AT_ADD_REGION_FLAG(SoftwareEnclave, "software_enclave");
    AT_ADD_REGION_FLAG(PageSize64K, "page_size_64k");
    AT_ADD_REGION_FLAG(PlaceholderReservation, "placeholder_reservation");
    AT_ADD_REGION_FLAG(MappedAwe, "mapped_awe");
    AT_ADD_REGION_FLAG(MappedWriteWatch, "mapped_write_watch");
    AT_ADD_REGION_FLAG(PageSizeLarge, "page_size_large");
    AT_ADD_REGION_FLAG(PageSizeHuge, "page_size_huge");

    PhAddJsonObjectValue(Row, "flags", array);
}

// What the region is for, in typed fields rather than only in the sentence "use" carries.
VOID AtpAddRegionDetail(
    _In_ PVOID Row,
    _In_ PPH_MEMORY_ITEM Item,
    _In_ ULONG PageSize
    )
{
    PVOID entry;

    AtJsonAddStringZ(Row, "region_type", AtpMemoryRegionTypeString(Item->RegionType));
    AtpAddRegionFlags(Row, Item);
    PhAddJsonObjectUInt64(Row, "page_priority", Item->Priority);

    entry = PhCreateJsonObject();
    PhAddJsonObjectUInt64(entry, "total_bytes", (ULONG64)Item->TotalWorkingSetPages * PageSize);
    PhAddJsonObjectUInt64(entry, "private_bytes", (ULONG64)Item->PrivateWorkingSetPages * PageSize);
    PhAddJsonObjectUInt64(entry, "shared_bytes", (ULONG64)Item->SharedWorkingSetPages * PageSize);
    PhAddJsonObjectUInt64(entry, "shareable_bytes", (ULONG64)Item->ShareableWorkingSetPages * PageSize);
    PhAddJsonObjectUInt64(entry, "locked_bytes", (ULONG64)Item->LockedWorkingSetPages * PageSize);
    PhAddJsonObjectValue(Row, "working_set", entry);

    switch (Item->RegionType)
    {
    case MappedFileRegion:
        AtJsonAddWin32FileName(Row, "mapped_file", Item->u.MappedFile.FileName);

        if (Item->u.MappedFile.SigningLevelValid)
            AtJsonAddStringZ(Row, "signing_level", AtpSigningLevelString(Item->u.MappedFile.SigningLevel));
        else
            AtJsonAddNull(Row, "signing_level");

        AtJsonAddNull(Row, "thread_id");
        AtJsonAddNull(Row, "heap");
        break;
    case TebRegion:
    case Teb32Region:
        AtJsonAddNull(Row, "mapped_file");
        AtJsonAddNull(Row, "signing_level");
        PhAddJsonObjectUInt64(Row, "thread_id", HandleToUlong(Item->u.Teb.ThreadId));
        AtJsonAddNull(Row, "heap");
        break;
    case StackRegion:
    case Stack32Region:
        AtJsonAddNull(Row, "mapped_file");
        AtJsonAddNull(Row, "signing_level");
        PhAddJsonObjectUInt64(Row, "thread_id", HandleToUlong(Item->u.Stack.ThreadId));
        AtJsonAddNull(Row, "heap");
        break;
    case HeapRegion:
    case Heap32Region:
        AtJsonAddNull(Row, "mapped_file");
        AtJsonAddNull(Row, "signing_level");
        AtJsonAddNull(Row, "thread_id");
        entry = PhCreateJsonObject();
        PhAddJsonObjectUInt64(entry, "index", Item->u.Heap.Index);

        if (Item->u.Heap.ClassValid)
            PhAddJsonObjectUInt64(entry, "class", Item->u.Heap.Class);
        else
            AtJsonAddNull(entry, "class");

        PhAddJsonObjectValue(Row, "heap", entry);
        break;
    default:
        AtJsonAddNull(Row, "mapped_file");
        AtJsonAddNull(Row, "signing_level");
        AtJsonAddNull(Row, "thread_id");
        AtJsonAddNull(Row, "heap");
        break;
    }
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
    BOOLEAN summaryOnly;
    ULONG pageSize;
    ULONG64 totalCommitted = 0;
    ULONG64 totalPrivate = 0;
    ULONG64 totalWorkingSet = 0;
    ULONG64 totalPrivateWorkingSet = 0;
    ULONG committedRegions = 0;
    ULONG reservedRegions = 0;
    ULONG freeRegions = 0;
    ULONG imageRegions = 0;
    ULONG privateRegions = 0;
    ULONG executablePrivateRegions = 0;
    ULONG64 executablePrivateBytes = 0;
    PLIST_ENTRY entry;
    AT_ROWS rows;
    PVOID structured;

    if (!NT_SUCCESS(AtResolveProcessTarget(Call->Arguments, FALSE, 0, &target, Result)))
        return;

    includeFree = AtJsonGetObjectBoolean(Call->Arguments, "include_free");
    allocationsOnly = AtJsonGetObjectBoolean(Call->Arguments, "allocations_only");
    summaryOnly = AtJsonGetObjectBoolean(Call->Arguments, "summary_only");
    pageSize = PhSystemBasicInformation.PageSize;

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
    AtInitializeRows(&rows, Call->Arguments);

    for (entry = list.ListHead.Flink; entry != &list.ListHead; entry = entry->Flink)
    {
        PPH_MEMORY_ITEM summaryItem = CONTAINING_RECORD(entry, PH_MEMORY_ITEM, ListEntry);

        // The rollup counts every region, not only the ones a filter or a page would return.
        totalCommitted += summaryItem->CommittedSize;
        totalPrivate += summaryItem->PrivateSize;
        totalWorkingSet += (ULONG64)summaryItem->TotalWorkingSetPages * pageSize;
        totalPrivateWorkingSet += (ULONG64)summaryItem->PrivateWorkingSetPages * pageSize;

        if (summaryItem->State & MEM_COMMIT)
            committedRegions++;
        else if (summaryItem->State & MEM_RESERVE)
            reservedRegions++;
        else
            freeRegions++;

        if (summaryItem->MappedImage)
            imageRegions++;
        else if (summaryItem->Private)
            privateRegions++;

        // Private memory that is executable is what shellcode lives in, so it is worth a number of
        // its own rather than making the caller walk every region to find out.
        if ((summaryItem->State & MEM_COMMIT) &&
            summaryItem->Private &&
            FlagOn(summaryItem->Protect, PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))
        {
            executablePrivateRegions++;
            executablePrivateBytes += summaryItem->RegionSize;
        }
    }

    for (entry = list.ListHead.Flink; entry != &list.ListHead; entry = entry->Flink)
    {
        if (summaryOnly)
            break;

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
        AtpAddRegionDetail(row, item, pageSize);

        AtAddRow(&rows, row);
    }

    PhDeleteMemoryItemList(&list);

    {
        PVOID summary = PhCreateJsonObject();

        PhAddJsonObjectUInt64(summary, "committed_bytes", totalCommitted);
        PhAddJsonObjectUInt64(summary, "private_bytes", totalPrivate);
        PhAddJsonObjectUInt64(summary, "working_set_bytes", totalWorkingSet);
        PhAddJsonObjectUInt64(summary, "private_working_set_bytes", totalPrivateWorkingSet);
        PhAddJsonObjectUInt64(summary, "committed_regions", committedRegions);
        PhAddJsonObjectUInt64(summary, "reserved_regions", reservedRegions);
        PhAddJsonObjectUInt64(summary, "free_regions", freeRegions);
        PhAddJsonObjectUInt64(summary, "image_regions", imageRegions);
        PhAddJsonObjectUInt64(summary, "private_regions", privateRegions);
        PhAddJsonObjectUInt64(summary, "executable_private_regions", executablePrivateRegions);
        PhAddJsonObjectUInt64(summary, "executable_private_bytes", executablePrivateBytes);
        PhAddJsonObjectValue(structured, "summary", summary);
    }

    if (summaryOnly)
    {
        // No rows were built, but the paging fields are part of every list answer: report zero
        // returned out of the regions that exist, rather than leaving the caller to wonder.
        AtDeleteRows(&rows);
        AtJsonAddNull(structured, "regions");
        PhAddJsonObjectUInt64(structured, "count", 0);
        PhAddJsonObjectUInt64(structured, "total_count", (ULONG64)committedRegions + reservedRegions + freeRegions);
        PhAddJsonObjectUInt64(structured, "offset", 0);
        PhAddJsonObjectUInt64(structured, "limit", 0);
        PhAddJsonObjectBoolean(structured, "truncated", FALSE);
    }
    else
    {
        AtAddRows(structured, "regions", &rows);
    }

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteTarget(&target);
}

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
