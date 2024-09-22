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
    _In_ PWSTR ModuleName,
    _In_ PSTR ProcedureName
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
    _In_ PWSTR ModuleName,
    _In_ PSTR ProcedureName
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

#pragma region API Forwarders

NTSTATUS NTAPI NtCreateProcessStateChange_Stub(
    _Out_ PHANDLE ProcessStateChangeHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE ProcessHandle,
    _In_opt_ _Reserved_ ULONG64 Reserved
    )
{
    if (NtCreateProcessStateChange_Import())
        return NtCreateProcessStateChange_Import()(ProcessStateChangeHandle, DesiredAccess, ObjectAttributes, ProcessHandle, Reserved);
    return STATUS_INVALID_KERNEL_INFO_VERSION;
}

NTSTATUS NTAPI NtChangeProcessState_Stub(
    _In_ HANDLE ProcessStateChangeHandle,
    _In_ HANDLE ProcessHandle,
    _In_ PROCESS_STATE_CHANGE_TYPE StateChangeType,
    _In_opt_ _Reserved_ PVOID ExtendedInformation,
    _In_opt_ _Reserved_ SIZE_T ExtendedInformationLength,
    _In_opt_ _Reserved_ ULONG64 Reserved
    )
{
    if (NtChangeProcessState_Import())
        return NtChangeProcessState_Import()(ProcessStateChangeHandle, ProcessHandle, StateChangeType, ExtendedInformation, ExtendedInformationLength, Reserved);
    return STATUS_INVALID_KERNEL_INFO_VERSION;
}

NTSTATUS NTAPI NtPssCaptureVaSpaceBulk_Stub(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _In_ PNTPSS_MEMORY_BULK_INFORMATION BulkInformation,
    _In_ SIZE_T BulkInformationLength,
    _Out_opt_ PSIZE_T ReturnLength
    )
{
    if (NtPssCaptureVaSpaceBulk_Import())
        return NtPssCaptureVaSpaceBulk_Import()(ProcessHandle, BaseAddress, BulkInformation, BulkInformationLength, ReturnLength);
    return STATUS_INVALID_KERNEL_INFO_VERSION;
}

NTSTATUS NTAPI NtCopyFileChunk_Stub(
    _In_ HANDLE SourceHandle,
    _In_ HANDLE DestinationHandle,
    _In_opt_ HANDLE EventHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG Length,
    _In_ PLARGE_INTEGER SourceOffset,
    _In_ PLARGE_INTEGER DestOffset,
    _In_opt_ PULONG SourceKey,
    _In_opt_ PULONG DestKey,
    _In_ ULONG Flags
    )
{
    if (NtCopyFileChunk_Import())
        return NtCopyFileChunk_Import()(SourceHandle, DestinationHandle, EventHandle, IoStatusBlock, Length, SourceOffset, DestOffset, SourceKey, DestKey, Flags);
    return STATUS_INVALID_KERNEL_INFO_VERSION;
}

NTSTATUS NTAPI ConsoleControl_Stub(
    _In_ CONSOLECONTROL Command,
    _In_reads_bytes_(ConsoleInformationLength) PVOID ConsoleInformation,
    _In_ ULONG ConsoleInformationLength
    )
{
    if (ConsoleControl_Import())
        return ConsoleControl_Import()(Command, ConsoleInformation, ConsoleInformationLength);
    return STATUS_INVALID_KERNEL_INFO_VERSION;
}

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
    _In_ UINT uExitCode
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

DECLSPEC_SELECTANY typeof(&NtCreateProcessStateChange) __imp_NtCreateProcessStateChange = NtCreateProcessStateChange_Stub;
DECLSPEC_SELECTANY typeof(&NtChangeProcessState) __imp_NtChangeProcessState = NtChangeProcessState_Stub;
DECLSPEC_SELECTANY typeof(&NtPssCaptureVaSpaceBulk) __imp_NtPssCaptureVaSpaceBulk = NtPssCaptureVaSpaceBulk_Stub;
DECLSPEC_SELECTANY typeof(&NtCopyFileChunk) __imp_NtCopyFileChunk = NtCopyFileChunk_Stub;
DECLSPEC_SELECTANY typeof(&ConsoleControl) __imp_ConsoleControl = ConsoleControl_Stub;
DECLSPEC_SELECTANY typeof(&CloseHandle) __imp_CloseHandle = CloseHandle_Stub;
DECLSPEC_SELECTANY typeof(&GetFileSizeEx) __imp_GetFileSizeEx = GetFileSizeEx_Stub;
DECLSPEC_SELECTANY typeof(&FlushFileBuffers) __imp_FlushFileBuffers = FlushFileBuffers_Stub;
DECLSPEC_SELECTANY typeof(&IsDebuggerPresent) __imp_IsDebuggerPresent = IsDebuggerPresent_Stub;
DECLSPEC_SELECTANY typeof(&TerminateProcess) __imp_TerminateProcess = TerminateProcess_Stub;
DECLSPEC_SELECTANY typeof(&GetCurrentThreadId) __imp_GetCurrentThreadId = GetCurrentThreadId_Stub;
DECLSPEC_SELECTANY typeof(&GetCurrentProcessId) __imp_GetCurrentProcessId = GetCurrentProcessId_Stub;
DECLSPEC_SELECTANY typeof(&GetProcessHeap) __imp_GetProcessHeap = GetProcessHeap_Stub;

#pragma endregion
