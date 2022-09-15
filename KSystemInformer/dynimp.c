/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     jxy-s   2020-2022
 *
 */

#include <kph.h>
#include <dyndata.h>

#include <trace.h>

PAGED_FILE();

/**
 * \brief Dynamically imports routines.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphDynamicImport(
    VOID
    )
{
    PAGED_PASSIVE();

    KphDynPsLoadedModuleResource = (PERESOURCE)KphGetSystemRoutineAddress(L"PsLoadedModuleResource");
    KphDynPsLoadedModuleList = (PLIST_ENTRY)KphGetSystemRoutineAddress(L"PsLoadedModuleList");
    KphDynRtlFindExportedRoutineByName = (PRTL_FIND_EXPORTED_ROUTINE_BY_NAME)KphGetSystemRoutineAddress(L"RtlFindExportedRoutineByName");
    KphDynPsGetProcessProtection = (PPS_GET_PROCESS_PROTECTION)KphGetSystemRoutineAddress(L"PsGetProcessProtection");
    KphDynRtlImageNtHeaderEx = (PRTL_IMAGE_NT_HEADER_EX)KphGetSystemRoutineAddress(L"RtlImageNtHeaderEx");
    KphDynKeRemoveQueueDpcEx = (PKE_REMOVE_QUEUE_DPC_EX)KphGetSystemRoutineAddress(L"KeRemoveQueueDpcEx");
    KphDynPsSetLoadImageNotifyRoutineEx = (PPS_SET_LOAD_IMAGE_NOTIFY_ROUTINE_EX)KphGetSystemRoutineAddress(L"PsSetLoadImageNotifyRoutineEx");
    KphDynPsSetCreateProcessNotifyRoutineEx2 = (PPS_SET_CREATE_PROCESS_NOTIFY_ROUTINE_EX2)KphGetSystemRoutineAddress(L"PsSetCreateProcessNotifyRoutineEx2");
    KphDynPsSetCreateThreadNotifyRoutineEx = (PPS_SET_CREATE_THREAD_NOTIFY_ROUTINE_EX)KphGetSystemRoutineAddress(L"PsSetCreateThreadNotifyRoutineEx");
    KphDynPsGetThreadExitStatus = (PPS_GET_THREAD_EXIT_STATUS)KphGetSystemRoutineAddress(L"PsGetThreadExitStatus");
}

/**
 * \brief Retrieves the address of a function exported by NTOS or HAL.
 *
 * \param SystemRoutineName The name of the function.
 *
 * \return The address of the function, or NULL if the function could
 * not be found.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PVOID KphGetSystemRoutineAddress(
    _In_z_ PCWSTR SystemRoutineName
    )
{
    UNICODE_STRING systemRoutineName;

    PAGED_PASSIVE();

    RtlInitUnicodeString(&systemRoutineName, SystemRoutineName);

    return MmGetSystemRoutineAddress(&systemRoutineName);
}

/**
 * \brief Locates an export by parsing the mapped image.
 *
 * \param BaseAddress The base address of the mapped image.
 * \param RoutineName The name of the routine to fine.
 *
 * \return The address of the function, or NULL if the function could
 * not be found.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PVOID KphpFindExportedRoutineByNameInImage(
    _In_ PVOID BaseAddress,
    _In_z_ PCSTR RoutineName
    )
{
    PIMAGE_EXPORT_DIRECTORY exports;
    ULONG size;
    PULONG nameRvas;
    PUSHORT ordinals;
    PULONG functionRvas;
    LONGLONG lower;
    LONGLONG upper;

    PAGED_PASSIVE();

    exports = RtlImageDirectoryEntryToData(BaseAddress,
                                           TRUE,
                                           IMAGE_DIRECTORY_ENTRY_EXPORT,
                                           &size);
    if (!exports ||
        (exports->NumberOfNames == 0) ||
        (exports->AddressOfNames == 0) ||
        (exports->AddressOfFunctions == 0))
    {
        return NULL;
    }

    nameRvas = Add2Ptr(BaseAddress, exports->AddressOfNames);
    ordinals = Add2Ptr(BaseAddress, exports->AddressOfNameOrdinals);
    functionRvas = Add2Ptr(BaseAddress, exports->AddressOfFunctions);

    lower = 0;
    upper = exports->NumberOfNames;
    while (lower <= upper)
    {
        LONGLONG mid;
        PCHAR name;
        int diff;

        mid = (lower + ((upper - lower) / 2));

        name = Add2Ptr(BaseAddress, nameRvas[(ULONG)mid]);

        diff = strcmp(name, RoutineName);
        if (diff == 0)
        {
            return Add2Ptr(BaseAddress, functionRvas[ordinals[(ULONG)mid]]);
        }

        if (diff < 0)
        {
            lower = mid + 1;
        }
        else
        {
            upper = mid - 1;
        }
    }

    return NULL;
}

/**
 * \brief Locates an export.
 *
 * \param BaseAddress The base address of the mapped image.
 * \param RoutineName The name of the routine to fine.
 *
 * \return The address of the function, or NULL if the function could
 * not be found.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PVOID KphFindExportedRoutineByName(
    _In_ PVOID BaseAddress,
    _In_z_ PCSTR RoutineName
    )
{
    PAGED_PASSIVE();

    if (KphDynRtlFindExportedRoutineByName)
    {
        return KphDynRtlFindExportedRoutineByName(BaseAddress, RoutineName);
    }

    return KphpFindExportedRoutineByNameInImage(BaseAddress, RoutineName);
}

/**
 * \brief Retrieves the address of a function using the exported module list.
 * Caller must guarantee that PsLoadedModuleList and PsLoadedModuleResource
 * is available prior to this call.
 *
 * \param ModuleName The name of the module to retrieve the function from.
 * \param RoutineName The name of the routine to retrieve.
 *
 * \return The address of the function, or NULL if the function could
 * not be found.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PVOID KphpGetRoutineAddressByModuleList(
    _In_z_ PCWSTR ModuleName,
    _In_z_ PCSTR RoutineName
    )
{
    PVOID routine;
    UNICODE_STRING moduleName;

    PAGED_PASSIVE();
    NT_ASSERT(KphDynPsLoadedModuleList && KphDynPsLoadedModuleResource);

    routine = NULL;
    RtlInitUnicodeString(&moduleName, ModuleName);

    if (!ExAcquireResourceSharedLite(KphDynPsLoadedModuleResource, TRUE))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Failed to acquire PsLoadedModuleResource to "
                      "get routine %ls!%hs",
                      ModuleName,
                      RoutineName);

        return routine;
    }

    for (PLIST_ENTRY link = KphDynPsLoadedModuleList->Flink;
         link != KphDynPsLoadedModuleList;
         link = link->Flink)
    {
        PKLDR_DATA_TABLE_ENTRY entry;

        entry = CONTAINING_RECORD(link, KLDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        if (!RtlEqualUnicodeString(&entry->BaseDllName, &moduleName, TRUE))
        {
            continue;
        }

        routine = KphFindExportedRoutineByName(entry->DllBase, RoutineName);
        if (routine)
        {
            break;
        }
    }

    ExReleaseResourceLite(KphDynPsLoadedModuleResource);

    if (!routine)
    {
        KphTracePrint(TRACE_LEVEL_WARNING,
                      GENERAL,
                      "Failed to find routine %ls!%hs",
                      ModuleName,
                      RoutineName);
    }

    return routine;
}

/**
 * \brief Retrieves the address of a function by querying for the modules list.
 * This is less efficient requiring ZwQuerySystemInformation, allocations,
 * and copies.
 *
 * \param ModuleName The name of the module to retrieve the function from.
 * \param RoutineName The name of the routine to retrieve.
 *
 * \return The address of the function, or NULL if the function could
 * not be found.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PVOID KphpGetRoutineAddressBySystemQuery(
    _In_z_ PCWSTR ModuleName,
    _In_z_ PCSTR RoutineName
    )
{
    PVOID routine;
    NTSTATUS status;
    PRTL_PROCESS_MODULES modules;
    UNICODE_STRING moduleNameWide;
    ANSI_STRING moduleName;

    routine = NULL;
    modules = NULL;

    PAGED_PASSIVE();

    RtlInitUnicodeString(&moduleNameWide, ModuleName);
    status = RtlUnicodeStringToAnsiString(&moduleName, &moduleNameWide, TRUE);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_WARNING,
                      GENERAL,
                      "Failed to find routine %ls!%hs "
                      "RtlUnicodeStringToAnsiString failed: %!STATUS!",
                      ModuleName,
                      RoutineName,
                      status);

        RtlZeroMemory(&moduleName, sizeof(moduleName));
        goto Exit;
    }

    status = KphGetSystemModules(&modules);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_WARNING,
                      GENERAL,
                      "Failed to find routine %ls!%hs "
                      "KphGetSystemModules failed: %!STATUS!",
                      ModuleName,
                      RoutineName,
                      status);

        modules = NULL;
        goto Exit;
    }

    for (ULONG i = 0; i < modules->NumberOfModules; i++)
    {
        PRTL_PROCESS_MODULE_INFORMATION info;
        ANSI_STRING dllName;

        info = &modules->Modules[i];
        RtlInitAnsiString(&dllName, (PCSZ)&info->FullPathName[info->OffsetToFileName]);

        if (!info->ImageBase || !RtlEqualString(&dllName, &moduleName, TRUE))
        {
            continue;
        }

        routine = KphFindExportedRoutineByName(info->ImageBase, RoutineName);
        if (routine)
        {
            break;
        }
    }

Exit:

    if (modules)
    {
        KphFreeSystemModules(modules);
    }

    if (moduleName.Buffer)
    {
        RtlFreeAnsiString(&moduleName);
    }

    return routine;
}

/**
 * \brief Retrieves the address of a function.
 *
 * \param ModuleName The name of the module to retrieve the function from.
 * \param RoutineName The name of the routine to retrieve.
 *
 * \return The address of the function, or NULL if the function could
 * not be found.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PVOID KphGetRoutineAddress(
    _In_z_ PCWSTR ModuleName,
    _In_z_ PCSTR RoutineName
    )
{
    PAGED_PASSIVE();

    if (KphDynPsLoadedModuleList && KphDynPsLoadedModuleResource)
    {
        return KphpGetRoutineAddressByModuleList(ModuleName, RoutineName);
    }

    return KphpGetRoutineAddressBySystemQuery(ModuleName, RoutineName);
}
