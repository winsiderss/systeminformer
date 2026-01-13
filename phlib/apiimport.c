/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2015
 *     dmex    2019-2024
 *
 */

#include <ph.h>

#include <mapldr.h>
#include <sddl.h>
#include <shlwapi.h>
#include <userenv.h>
#include <ntuser.h>
#include <xmllite.h>

#include <apiimport.h>

/**
 * Imports a procedure from a specified module.
 *
 * \param InitOnce A pointer to an initialization structure.
 * \param Cache A pointer to a cache for the procedure address.
 * \param Cookie A pointer to a cookie for the procedure address.
 * \param ModuleName The name of the module.
 * \param ProcedureName The name of the procedure.
 * \return A pointer to the imported procedure, or NULL if the procedure could not be imported.
 */
FORCEINLINE
PVOID PhpImportProcedure(
    _Inout_ PPH_INITONCE InitOnce,
    _Inout_ PVOID *Cache,
    _Inout_ PULONG_PTR Cookie,
    _In_ PCWSTR ModuleName,
    _In_ PCSTR ProcedureName
    )
{
    if (PhBeginInitOnce(InitOnce))
    {
        PVOID module;
        PVOID procedure;

        module = PhGetLoaderEntryDllBaseZ(ModuleName);

        if (!module)
            module = PhLoadLibrary(ModuleName);

        if (module)
        {
            if (procedure = PhGetDllBaseProcedureAddress(module, ProcedureName, 0))
            {
                *Cookie = (ULONG_PTR)PhReadTimeStampCounter();
                *Cache = (PVOID)((ULONG_PTR)procedure ^ (ULONG_PTR)*Cookie);
            }
        }

        PhEndInitOnce(InitOnce);
    }

    if (*Cache && *Cookie)
        return (PVOID)((ULONG_PTR)*Cache ^ (ULONG_PTR)*Cookie);

    return NULL;
}

/**
 * Imports a procedure from a specified module using native methods.
 *
 * \param InitOnce A pointer to an initialization structure.
 * \param Cache A pointer to a cache for the procedure address.
 * \param Cookie A pointer to a cookie for the procedure address.
 * \param ModuleName The name of the module.
 * \param ProcedureName The name of the procedure.
 * \return A pointer to the imported procedure, or NULL if the procedure could not be imported.
 */
FORCEINLINE
PVOID PhpImportProcedureNative(
    _Inout_ PPH_INITONCE InitOnce,
    _Inout_ PVOID *Cache,
    _Inout_ PULONG_PTR Cookie,
    _In_ PCWSTR ModuleName,
    _In_ PCSTR ProcedureName
    )
{
    if (PhBeginInitOnce(InitOnce))
    {
        PVOID module;
        PVOID procedure;

        module = PhGetLoaderEntryDllBaseZ(ModuleName);

        if (!module)
            module = PhLoadLibrary(ModuleName);

        if (module)
        {
            ANSI_STRING procedureName;

            RtlInitAnsiString(&procedureName, ProcedureName);

            if (NT_SUCCESS(LdrGetProcedureAddress(
                module,
                &procedureName,
                0,
                &procedure
                )))
            {
                *Cookie = (ULONG_PTR)PhReadTimeStampCounter();
                *Cache = (PVOID)((ULONG_PTR)procedure ^ (ULONG_PTR)*Cookie);
            }
        }

        PhEndInitOnce(InitOnce);
    }

    if (*Cache && *Cookie)
        return (PVOID)((ULONG_PTR)*Cache ^ (ULONG_PTR)*Cookie);

    return NULL;
}

/**
 * Defines an import function for a specified module and procedure.
 *
 * \param Module The name of the module.
 * \param Name The name of the procedure.
 */
#define PH_DEFINE_IMPORT(Module, Name) \
typeof(&(Name)) Name##_Import(VOID) \
{ \
    static PH_INITONCE initOnce = PH_INITONCE_INIT; \
    static PVOID cache = NULL; \
    static ULONG_PTR cookie = 0; \
\
    return (typeof(&(Name)))PhpImportProcedure(&initOnce, &cache, &cookie, (Module), (#Name)); \
}

/**
 * Defines an import function for a specified module and procedure for native loading.
 *
 * \param Module The name of the module.
 * \param Name The name of the procedure.
 */
#define PH_DEFINE_IMPORT_NATIVE(Module, Name) \
typeof(&(Name)) Name##_Import(VOID) \
{ \
    static PH_INITONCE initOnce = PH_INITONCE_INIT; \
    static PVOID cache = NULL; \
    static ULONG_PTR cookie = 0; \
\
    return (typeof(&(Name)))PhpImportProcedureNative(&initOnce, &cache, &cookie, (Module), (#Name)); \
}

PH_DEFINE_IMPORT(L"ntdll.dll", NtQueryInformationEnlistment);
PH_DEFINE_IMPORT(L"ntdll.dll", NtQueryInformationResourceManager);
PH_DEFINE_IMPORT(L"ntdll.dll", NtQueryInformationTransaction);
PH_DEFINE_IMPORT(L"ntdll.dll", NtQueryInformationTransactionManager);
PH_DEFINE_IMPORT(L"ntdll.dll", NtCreateProcessStateChange);
PH_DEFINE_IMPORT(L"ntdll.dll", NtChangeProcessState);
PH_DEFINE_IMPORT(L"ntdll.dll", NtCreateThreadStateChange);
PH_DEFINE_IMPORT(L"ntdll.dll", NtChangeThreadState);
PH_DEFINE_IMPORT(L"ntdll.dll", NtCopyFileChunk);
PH_DEFINE_IMPORT(L"ntdll.dll", NtCompareObjects);

PH_DEFINE_IMPORT_NATIVE(L"ntdll.dll", NtSetInformationVirtualMemory);
PH_DEFINE_IMPORT(L"ntdll.dll", LdrSystemDllInitBlock);
PH_DEFINE_IMPORT(L"ntdll.dll", LdrResFindResource);

PH_DEFINE_IMPORT(L"ntdll.dll", RtlDefaultNpAcl);
PH_DEFINE_IMPORT(L"ntdll.dll", RtlDelayExecution);
PH_DEFINE_IMPORT(L"ntdll.dll", RtlDeriveCapabilitySidsFromName);
PH_DEFINE_IMPORT(L"ntdll.dll", RtlDosLongPathNameToNtPathName_U_WithStatus);
PH_DEFINE_IMPORT(L"ntdll.dll", RtlGetTokenNamedObjectPath);
PH_DEFINE_IMPORT(L"ntdll.dll", RtlGetAppContainerNamedObjectPath);
PH_DEFINE_IMPORT(L"ntdll.dll", RtlGetAppContainerSidType);
PH_DEFINE_IMPORT(L"ntdll.dll", RtlGetAppContainerParent);

PH_DEFINE_IMPORT(L"ntdll.dll", PssNtCaptureSnapshot);
PH_DEFINE_IMPORT(L"ntdll.dll", PssNtQuerySnapshot);
PH_DEFINE_IMPORT(L"ntdll.dll", PssNtFreeSnapshot);
PH_DEFINE_IMPORT(L"ntdll.dll", PssNtFreeRemoteSnapshot);
PH_DEFINE_IMPORT(L"ntdll.dll", NtPssCaptureVaSpaceBulk);
PH_DEFINE_IMPORT(L"ntdll.dll", TpSetPoolThreadBasePriority);

PH_DEFINE_IMPORT(L"advapi32.dll", ConvertSecurityDescriptorToStringSecurityDescriptorW);
PH_DEFINE_IMPORT(L"advapi32.dll", ConvertStringSecurityDescriptorToSecurityDescriptorW);

PH_DEFINE_IMPORT(L"cfgmgr32.dll", DevGetObjects);
PH_DEFINE_IMPORT(L"cfgmgr32.dll", DevFreeObjects);
PH_DEFINE_IMPORT(L"cfgmgr32.dll", DevGetObjectProperties);
PH_DEFINE_IMPORT(L"cfgmgr32.dll", DevFreeObjectProperties);
PH_DEFINE_IMPORT(L"cfgmgr32.dll", DevCreateObjectQuery);
PH_DEFINE_IMPORT(L"cfgmgr32.dll", DevCloseObjectQuery);

PH_DEFINE_IMPORT(L"shlwapi.dll", SHAutoComplete);
PH_DEFINE_IMPORT(L"shlwapi.dll", SHCreateStreamOnFileEx);

PH_DEFINE_IMPORT(L"userenv.dll", CreateEnvironmentBlock);
PH_DEFINE_IMPORT(L"userenv.dll", DestroyEnvironmentBlock);
PH_DEFINE_IMPORT(L"userenv.dll", GetAppContainerRegistryLocation);
PH_DEFINE_IMPORT(L"userenv.dll", GetAppContainerFolderPath);

PH_DEFINE_IMPORT(L"user32.dll", ConsoleControl);
PH_DEFINE_IMPORT(L"user32.dll", GetCurrentInputMessageSource);
PH_DEFINE_IMPORT(L"user32.dll", GetCIMSSM);
PH_DEFINE_IMPORT(L"win32u.dll", NtUserBuildHwndList);

PH_DEFINE_IMPORT(L"xmllite.dll", CreateXmlReader);
PH_DEFINE_IMPORT(L"xmllite.dll", CreateXmlWriter);
