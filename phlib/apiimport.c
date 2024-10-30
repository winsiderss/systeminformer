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
#include <apiimport.h>
#include <ntuser.h>
#include <mapldr.h>
#include <sddl.h>

/**
 * Imports a procedure from a specified module.
 *
 * @param InitOnce A pointer to an initialization structure.
 * @param Cache A pointer to a cache for the procedure address.
 * @param Cookie A pointer to a cookie for the procedure address.
 * @param ModuleName The name of the module.
 * @param ProcedureName The name of the procedure.
 *
 * @return A pointer to the imported procedure, or NULL if the procedure could not be imported.
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
                *Cookie = (ULONG_PTR)NtGetTickCount64();
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
 * @param InitOnce A pointer to an initialization structure.
 * @param Cache A pointer to a cache for the procedure address.
 * @param Cookie A pointer to a cookie for the procedure address.
 * @param ModuleName The name of the module.
 * @param ProcedureName The name of the procedure.
 *
 * @return A pointer to the imported procedure, or NULL if the procedure could not be imported.
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
                *Cookie = (ULONG_PTR)NtGetTickCount64();
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
 * @param Module The name of the module.
 * @param Name The name of the procedure.
 */
#define PH_DEFINE_IMPORT(Module, Name) \
_##Name Name##_Import(VOID) \
{ \
    static PH_INITONCE initOnce = PH_INITONCE_INIT; \
    static PVOID cache = NULL; \
    static ULONG_PTR cookie = 0; \
\
    return (_##Name)PhpImportProcedure(&initOnce, &cache, &cookie, Module, #Name); \
}

/**
 * Defines an import function for a specified module and procedure for native loading.
 *
 * @param Module The name of the module.
 * @param Name The name of the procedure.
 */
#define PH_DEFINE_IMPORT_NATIVE(Module, Name) \
_##Name Name##_Import(VOID) \
{ \
    static PH_INITONCE initOnce = PH_INITONCE_INIT; \
    static PVOID cache = NULL; \
    static ULONG_PTR cookie = 0; \
\
    return (_##Name)PhpImportProcedureNative(&initOnce, &cache, &cookie, Module, #Name); \
}

PH_DEFINE_IMPORT(L"ntdll.dll", NtQueryInformationEnlistment);
PH_DEFINE_IMPORT(L"ntdll.dll", NtQueryInformationResourceManager);
PH_DEFINE_IMPORT(L"ntdll.dll", NtQueryInformationTransaction);
PH_DEFINE_IMPORT(L"ntdll.dll", NtQueryInformationTransactionManager);
PH_DEFINE_IMPORT_NATIVE(L"ntdll.dll", NtSetInformationVirtualMemory);
PH_DEFINE_IMPORT(L"ntdll.dll", NtCreateProcessStateChange);
PH_DEFINE_IMPORT(L"ntdll.dll", NtChangeProcessState);
PH_DEFINE_IMPORT_NATIVE(L"ntdll.dll", LdrControlFlowGuardEnforcedWithExportSuppression);
PH_DEFINE_IMPORT(L"ntdll.dll", NtCopyFileChunk);

PH_DEFINE_IMPORT(L"ntdll.dll", RtlDefaultNpAcl);
PH_DEFINE_IMPORT(L"ntdll.dll", RtlGetTokenNamedObjectPath);
PH_DEFINE_IMPORT(L"ntdll.dll", RtlGetAppContainerNamedObjectPath);
PH_DEFINE_IMPORT(L"ntdll.dll", RtlGetAppContainerSidType);
PH_DEFINE_IMPORT(L"ntdll.dll", RtlGetAppContainerParent);
PH_DEFINE_IMPORT(L"ntdll.dll", RtlDeriveCapabilitySidsFromName);

PH_DEFINE_IMPORT(L"ntdll.dll", PssNtCaptureSnapshot);
PH_DEFINE_IMPORT(L"ntdll.dll", PssNtQuerySnapshot);
PH_DEFINE_IMPORT(L"ntdll.dll", PssNtFreeSnapshot);
PH_DEFINE_IMPORT(L"ntdll.dll", PssNtFreeRemoteSnapshot);
PH_DEFINE_IMPORT(L"ntdll.dll", NtPssCaptureVaSpaceBulk);

PH_DEFINE_IMPORT(L"advapi32.dll", ConvertSecurityDescriptorToStringSecurityDescriptorW);
PH_DEFINE_IMPORT(L"advapi32.dll", ConvertStringSecurityDescriptorToSecurityDescriptorW);

PH_DEFINE_IMPORT(L"shlwapi.dll", SHAutoComplete);

PH_DEFINE_IMPORT(L"userenv.dll", CreateEnvironmentBlock);
PH_DEFINE_IMPORT(L"userenv.dll", DestroyEnvironmentBlock);
PH_DEFINE_IMPORT(L"userenv.dll", GetAppContainerRegistryLocation);
PH_DEFINE_IMPORT(L"userenv.dll", GetAppContainerFolderPath);

PH_DEFINE_IMPORT(L"user32.dll", SetWindowDisplayAffinity);
PH_DEFINE_IMPORT(L"user32.dll", ConsoleControl);

// CRT

#ifdef _WIN64

BOOL NTAPI CloseHandle_Stub(
    _In_ HANDLE Handle
    )
{
    return NT_SUCCESS(NtClose(Handle));
}

BOOL NTAPI GetFileSizeEx_Stub(
    _In_ HANDLE hFile,
    _Out_ PLARGE_INTEGER lpFileSize
    )
{
    return NT_SUCCESS(PhGetFileSize(hFile, lpFileSize));
}

BOOL NTAPI FlushFileBuffers_Stub(
    _In_ HANDLE hFile
    )
{
    return NT_SUCCESS(PhFlushBuffersFile(hFile));
}

BOOL NTAPI IsDebuggerPresent_Stub(
    VOID
    )
{
    return !!NtCurrentPeb()->BeingDebugged;
}

BOOL NTAPI TerminateProcess_Stub(
    _In_ HANDLE hProcess,
    _In_ ULONG uExitCode
    )
{
    return NT_SUCCESS(NtTerminateProcess(hProcess, PhDosErrorToNtStatus(uExitCode)));
}

ULONG NTAPI GetCurrentThreadId_Stub(
    VOID
    )
{
    return HandleToUlong(NtCurrentThreadId());
}

ULONG NTAPI GetCurrentProcessId_Stub(
    VOID
    )
{
    return HandleToUlong(NtCurrentProcessId());
}

HANDLE NTAPI GetProcessHeap_Stub(
    VOID
    )
{
    return NtCurrentPeb()->ProcessHeap;
}

LPSTR WINAPI GetCommandLineA_Stub(
    VOID
    )
{
    return NULL;
}

LPWSTR WINAPI GetCommandLineW_Stub(
    VOID
    )
{
    return NULL;
}

VOID WINAPI InitializeSListHead_Stub(
    _Out_ PSLIST_HEADER ListHead
    )
{
    PhInitializeSListHead(ListHead);
}

PSLIST_ENTRY NTAPI InterlockedPushEntrySList_Stub(
    _Inout_ PSLIST_HEADER ListHead,
    _Inout_ __drv_aliasesMem PSLIST_ENTRY ListEntry
    )
{
    return RtlInterlockedPushEntrySList(ListHead, ListEntry);
}

PSLIST_ENTRY NTAPI InterlockedFlushSList_Stub(
    _Inout_ PSLIST_HEADER ListHead
    )
{
    return RtlInterlockedFlushSList(ListHead);
}

DECLSPEC_SELECTANY LPCVOID __imp_CloseHandle = CloseHandle_Stub;
DECLSPEC_SELECTANY LPCVOID __imp_GetFileSizeEx = GetFileSizeEx_Stub;
DECLSPEC_SELECTANY LPCVOID __imp_FlushFileBuffers = FlushFileBuffers_Stub;
DECLSPEC_SELECTANY LPCVOID __imp_IsDebuggerPresent = IsDebuggerPresent_Stub;
DECLSPEC_SELECTANY LPCVOID __imp_TerminateProcess = TerminateProcess_Stub;
DECLSPEC_SELECTANY LPCVOID __imp_GetCurrentThreadId = GetCurrentThreadId_Stub;
DECLSPEC_SELECTANY LPCVOID __imp_GetCurrentProcessId = GetCurrentProcessId_Stub;
DECLSPEC_SELECTANY LPCVOID __imp_GetProcessHeap = GetProcessHeap_Stub;
DECLSPEC_SELECTANY LPCVOID __imp_GetCommandLineA = GetCommandLineA_Stub;
DECLSPEC_SELECTANY LPCVOID __imp_GetCommandLineW = GetCommandLineW_Stub;
DECLSPEC_SELECTANY LPCVOID __imp_InitializeSListHead = InitializeSListHead_Stub;
DECLSPEC_SELECTANY LPCVOID __imp_InterlockedPushEntrySList = InterlockedPushEntrySList_Stub;
DECLSPEC_SELECTANY LPCVOID __imp_InterlockedFlushSList = InterlockedFlushSList_Stub;

#endif
