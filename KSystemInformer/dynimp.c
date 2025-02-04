/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     jxy-s   2020-2025
 *
 */

#include <kph.h>

#include <trace.h>

KPH_PROTECTED_DATA_SECTION_PUSH();
PPS_SET_LOAD_IMAGE_NOTIFY_ROUTINE_EX KphDynPsSetLoadImageNotifyRoutineEx = NULL;
PPS_SET_CREATE_PROCESS_NOTIFY_ROUTINE_EX2 KphDynPsSetCreateProcessNotifyRoutineEx2 = NULL;
PMM_PROTECT_DRIVER_SECTION KphDynMmProtectDriverSection = NULL;
PPS_GET_PROCESS_SEQUENCE_NUMBER KphDynPsGetProcessSequenceNumber = NULL;
PPS_GET_PROCESS_START_KEY KphDynPsGetProcessStartKey = NULL;
PSE_REGISTER_IMAGE_VERIFICATION_CALLBACK KphSeRegisterImageVerificationCallback = NULL;
PSE_UNREGISTER_IMAGE_VERIFICATION_CALLBACK KphSeUnregisterImageVerificationCallback = NULL;
PCI_VALIDATE_FILE_OBJECT KphDynCiValidateFileObject = NULL;
PCI_FREE_POLICY_INFO KphDynCiFreePolicyInfo = NULL;
PLXP_THREAD_GET_CURRENT KphDynLxpThreadGetCurrent = NULL;
KPH_PROTECTED_DATA_SECTION_POP();

KPH_PAGED_FILE();

/**
 * \brief Dynamically imports routines.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphDynamicImport(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphDynPsSetLoadImageNotifyRoutineEx = (PPS_SET_LOAD_IMAGE_NOTIFY_ROUTINE_EX)KphGetSystemRoutineAddress(L"PsSetLoadImageNotifyRoutineEx");
    KphDynPsSetCreateProcessNotifyRoutineEx2 = (PPS_SET_CREATE_PROCESS_NOTIFY_ROUTINE_EX2)KphGetSystemRoutineAddress(L"PsSetCreateProcessNotifyRoutineEx2");
    KphDynMmProtectDriverSection = (PMM_PROTECT_DRIVER_SECTION)KphGetSystemRoutineAddress(L"MmProtectDriverSection");
    KphDynPsGetProcessSequenceNumber = (PPS_GET_PROCESS_SEQUENCE_NUMBER)KphGetSystemRoutineAddress(L"PsGetProcessSequenceNumber");
    KphDynPsGetProcessStartKey = (PPS_GET_PROCESS_START_KEY)KphGetSystemRoutineAddress(L"PsGetProcessStartKey");
    KphSeRegisterImageVerificationCallback = (PSE_REGISTER_IMAGE_VERIFICATION_CALLBACK)KphGetSystemRoutineAddress(L"SeRegisterImageVerificationCallback");
    KphSeUnregisterImageVerificationCallback = (PSE_UNREGISTER_IMAGE_VERIFICATION_CALLBACK)KphGetSystemRoutineAddress(L"SeUnregisterImageVerificationCallback");
    KphDynCiValidateFileObject = (PCI_VALIDATE_FILE_OBJECT)KphGetRoutineAddress(L"ci.dll", "CiValidateFileObject");
    KphDynCiFreePolicyInfo = (PCI_FREE_POLICY_INFO)KphGetRoutineAddress(L"ci.dll", "CiFreePolicyInfo");
    KphDynLxpThreadGetCurrent = (PLXP_THREAD_GET_CURRENT)KphGetRoutineAddress(L"lxcore.sys", "LxpThreadGetCurrent");
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

    KPH_PAGED_CODE_PASSIVE();

    RtlInitUnicodeString(&systemRoutineName, SystemRoutineName);

    return MmGetSystemRoutineAddress(&systemRoutineName);
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

    KPH_PAGED_CODE_PASSIVE();

    routine = NULL;
    RtlInitUnicodeString(&moduleName, ModuleName);

    KeEnterCriticalRegion();
    if (!ExAcquireResourceSharedLite(PsLoadedModuleResource, TRUE))
    {
        KeLeaveCriticalRegion();

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "Failed to acquire PsLoadedModuleResource to "
                      "get routine %ls!%hs",
                      ModuleName,
                      RoutineName);

        return routine;
    }

    for (PLIST_ENTRY link = PsLoadedModuleList->Flink;
         link != PsLoadedModuleList;
         link = link->Flink)
    {
        PKLDR_DATA_TABLE_ENTRY entry;

        entry = CONTAINING_RECORD(link, KLDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        if (!RtlEqualUnicodeString(&entry->BaseDllName, &moduleName, TRUE))
        {
            continue;
        }

        routine = RtlFindExportedRoutineByName(entry->DllBase, RoutineName);
        if (routine)
        {
            break;
        }
    }

    ExReleaseResourceLite(PsLoadedModuleResource);
    KeLeaveCriticalRegion();

    if (!routine)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "Failed to find routine %ls!%hs",
                      ModuleName,
                      RoutineName);
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
    KPH_PAGED_CODE_PASSIVE();

    return KphpGetRoutineAddressByModuleList(ModuleName, RoutineName);
}
