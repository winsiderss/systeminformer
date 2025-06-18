/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
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

EXTERN_C_START

static BOOL WINAPI CloseHandle_Stub(
    _In_ HANDLE Handle
    )
{
    NTSTATUS status;

    status = NtClose(Handle);

    PhSetLastError(PhNtStatusToDosError(status));
    return NT_SUCCESS(status);
}

static BOOL WINAPI GetFileSizeEx_Stub(
    _In_ HANDLE hFile,
    _Out_ PLARGE_INTEGER lpFileSize
    )
{
    NTSTATUS status;

    status = PhGetFileSize(hFile, lpFileSize);

    PhSetLastError(PhNtStatusToDosError(status));
    return NT_SUCCESS(status);
}

static FARPROC WINAPI GetProcAddress_Stub(
    _In_ HMODULE Module,
    _In_ PCSTR Name
    )
{
    NTSTATUS status;
    PVOID procedureAddress;

    if (IS_INTRESOURCE(Name))
    {
        status = LdrGetProcedureAddress(
            Module,
            nullptr,
            PtrToUshort(Name),
            &procedureAddress
            );
    }
    else
    {
        ANSI_STRING procedureName;

        RtlInitAnsiString(&procedureName, Name);
        status = LdrGetProcedureAddress(
            Module,
            &procedureName,
            0,
            &procedureAddress
            );
    }

    PhSetLastError(PhNtStatusToDosError(status));

    if (NT_SUCCESS(status))
    {
        return reinterpret_cast<FARPROC>(procedureAddress);
    }

    return nullptr;
}

static BOOL WINAPI FlushFileBuffers_Stub(
    _In_ HANDLE hFile
    )
{
    NTSTATUS status;

    status = PhFlushBuffersFile(hFile);

    PhSetLastError(PhNtStatusToDosError(status));
    return NT_SUCCESS(status);
}

static BOOL WINAPI IsDebuggerPresent_Stub(
    VOID
    )
{
    NTSTATUS status;
    BOOL result;

    result = !!NtCurrentPeb()->BeingDebugged;
    status = STATUS_SUCCESS;

    PhSetLastError(PhNtStatusToDosError(status));
    return result;
}

static BOOL WINAPI SetEndOfFile_Stub(
    _In_ HANDLE hFile
    )
{
    NTSTATUS status;
    LARGE_INTEGER position;

    status = PhGetFilePosition(hFile, &position);

    if (NT_SUCCESS(status))
    {
        status = PhSetFilePosition(hFile, &position);
    }

    PhSetLastError(PhNtStatusToDosError(status));
    return NT_SUCCESS(status);
}

static VOID WINAPI ExitProcess_Stub(
    _In_ UINT uExitCode
    )
{
    PhExitApplication(PhDosErrorToNtStatus(uExitCode));
}

static BOOL WINAPI TerminateProcess_Stub(
    _In_ HANDLE hProcess,
    _In_ UINT uExitCode
    )
{
    NTSTATUS status;

    status = NtTerminateProcess(hProcess, PhDosErrorToNtStatus(uExitCode));

    PhSetLastError(PhNtStatusToDosError(status));
    return NT_SUCCESS(status);
}

static ULONG WINAPI GetCurrentThreadId_Stub(
    VOID
    )
{
    NTSTATUS status;
    ULONG result;

    result = HandleToUlong(NtCurrentThreadId());
    status = STATUS_SUCCESS;

    PhSetLastError(PhNtStatusToDosError(status));
    return result;
}

static ULONG WINAPI GetCurrentProcessId_Stub(
    VOID
    )
{
    NTSTATUS status;
    ULONG result;

    result = HandleToUlong(NtCurrentProcessId());
    status = STATUS_SUCCESS;

    PhSetLastError(PhNtStatusToDosError(status));
    return result;
}

static HANDLE WINAPI GetCurrentProcess_Stub(
    VOID
    )
{
    NTSTATUS status;
    HANDLE result;

    result = NtCurrentProcess();
    status = STATUS_SUCCESS;

    PhSetLastError(PhNtStatusToDosError(status));
    return result;
}

static HANDLE WINAPI GetProcessHeap_Stub(
    VOID
    )
{
    NTSTATUS status;
    HANDLE result;

    result = RtlProcessHeap();
    status = STATUS_SUCCESS;

    PhSetLastError(PhNtStatusToDosError(status));
    return result;
}

static LPSTR WINAPI GetCommandLineA_Stub(
    VOID
    )
{
    return nullptr;
}

static LPWSTR WINAPI GetCommandLineW_Stub(
    VOID
    )
{
    return nullptr;
}

static HMODULE WINAPI GetModuleHandleW_Stub(
    _In_opt_ PCWSTR ModuleName
    )
{
    NTSTATUS status;
    HMODULE result;

    if (ModuleName)
        result = static_cast<HMODULE>(PhGetDllHandle(ModuleName));
    else
        result = static_cast<HMODULE>(NtCurrentPeb()->ImageBaseAddress);

    if (result)
        status = STATUS_SUCCESS;
    else
        status = STATUS_DLL_NOT_FOUND;

    PhSetLastError(PhNtStatusToDosError(status));
    return result;
}

static BOOL WINAPI QueryPerformanceCounter_Stub(
    _Out_ LARGE_INTEGER* PerformanceCount
    )
{
    return !!RtlQueryPerformanceCounter(PerformanceCount);
}

static VOID WINAPI EnterCriticalSection_Stub(
    _Inout_ LPCRITICAL_SECTION CriticalSection
    )
{
    RtlEnterCriticalSection(CriticalSection);
}

static VOID WINAPI LeaveCriticalSection_Stub(
    _Inout_ LPCRITICAL_SECTION CriticalSection
    )
{
    _Analysis_assume_lock_acquired_(*CriticalSection);

    RtlLeaveCriticalSection(CriticalSection);
}

static VOID WINAPI DeleteCriticalSection_Stub(
    _Inout_ LPCRITICAL_SECTION CriticalSection
    )
{
    RtlDeleteCriticalSection(CriticalSection);
}

static PVOID WINAPI HeapAllocate_Stub(
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG Flags,
    _In_ SIZE_T Size
    )
{
    return RtlAllocateHeap(HeapHandle, Flags, Size);
}

static PVOID WINAPI HeapReAlloc_Stub(
    _In_ PVOID HeapHandle,
    _In_ ULONG Flags,
    _Frees_ptr_opt_ PVOID BaseAddress,
    _In_ SIZE_T Size
    )
{
    return RtlReAllocateHeap(HeapHandle, Flags, BaseAddress, Size);
}

static SIZE_T WINAPI HeapSize_Stub(
    _In_ PVOID HeapHandle,
    _In_ ULONG Flags,
    _In_ PCVOID BaseAddress
    )
{
    return RtlSizeHeap(HeapHandle, Flags, BaseAddress);
}

static BOOL WINAPI HeapFree_Stub(
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG Flags,
    _Frees_ptr_opt_ _Post_invalid_ PVOID BaseAddress
    )
{
    return !!RtlFreeHeap(HeapHandle, Flags, BaseAddress);
}

#if defined(_M_IX86)
#pragma warning(push)
#pragma warning(disable: 4483)
const decltype(&CloseHandle) __identifier("_imp__CloseHandle@4") = CloseHandle_Stub;
const decltype(&GetFileSizeEx) __identifier("_imp__GetFileSizeEx@4") = GetFileSizeEx_Stub;
const decltype(&GetProcAddress) __identifier("_imp__GetProcAddress@8") = GetProcAddress_Stub;
const decltype(&FlushFileBuffers) __identifier("_imp__FlushFileBuffers@4") = FlushFileBuffers_Stub;
const decltype(&TerminateProcess) __identifier("_imp__TerminateProcess@8") = TerminateProcess_Stub;
const decltype(&GetCurrentThreadId) __identifier("_imp__GetCurrentThreadId@0") = GetCurrentThreadId_Stub;
const decltype(&GetCurrentProcessId) __identifier("_imp__GetCurrentProcessId@0") = GetCurrentProcessId_Stub;
const decltype(&GetCurrentProcess) __identifier("_imp__GetCurrentProcess@0") = GetCurrentProcess_Stub;
const decltype(&GetProcessHeap) __identifier("_imp__GetProcessHeap@0") = GetProcessHeap_Stub;
const decltype(&GetCommandLineA) __identifier("_imp__GetCommandLineA@0") = GetCommandLineA_Stub;
const decltype(&GetCommandLineW) __identifier("_imp__GetCommandLineW@0") = GetCommandLineW_Stub;
const decltype(&GetModuleHandleW) __identifier("_imp__GetModuleHandleW@4") = GetModuleHandleW_Stub;
const decltype(&IsDebuggerPresent) __identifier("_imp__IsDebuggerPresent@0") = IsDebuggerPresent_Stub;
const decltype(&SetEndOfFile) __identifier("_imp__SetEndOfFile@4") = SetEndOfFile_Stub;
const decltype(&ExitProcess) __identifier("_imp__ExitProcess@4") = ExitProcess_Stub;
const decltype(&InitializeSListHead) __identifier("_imp__InitializeSListHead@4") = PhInitializeSListHead;
const decltype(&InterlockedPushEntrySList) __identifier("_imp__InterlockedPushEntrySList@8") = RtlInterlockedPushEntrySList;
const decltype(&InterlockedFlushSList) __identifier("_imp__InterlockedFlushSList@4") = RtlInterlockedFlushSList;
const decltype(&QueryPerformanceCounter) __identifier("_imp__QueryPerformanceCounter@4") = QueryPerformanceCounter_Stub;
const decltype(&EnterCriticalSection) __identifier("_imp__EnterCriticalSection@4") = EnterCriticalSection_Stub;
const decltype(&LeaveCriticalSection) __identifier("_imp__LeaveCriticalSection@4") = LeaveCriticalSection_Stub;
const decltype(&DeleteCriticalSection) __identifier("_imp__DeleteCriticalSection@4") = DeleteCriticalSection_Stub;
const decltype(&AcquireSRWLockExclusive) __identifier("_imp__AcquireSRWLockExclusive@4") = RtlAcquireSRWLockExclusive;
const decltype(&ReleaseSRWLockExclusive) __identifier("_imp__ReleaseSRWLockExclusive@4") = RtlReleaseSRWLockExclusive;
const decltype(&HeapAlloc) __identifier("_imp__HeapAlloc@12") = HeapAllocate_Stub;
const decltype(&HeapReAlloc) __identifier("_imp__HeapReAlloc@16") = HeapReAlloc_Stub;
const decltype(&HeapSize) __identifier("_imp__HeapSize@12") = HeapSize_Stub;
const decltype(&HeapFree) __identifier("_imp__HeapFree@12") = HeapFree_Stub;
#pragma warning(pop)
#else
const decltype(&CloseHandle) __imp_CloseHandle = CloseHandle_Stub;
const decltype(&GetFileSizeEx) __imp_GetFileSizeEx = GetFileSizeEx_Stub;
const decltype(&GetProcAddress) __imp_GetProcAddress = GetProcAddress_Stub;
const decltype(&FlushFileBuffers) __imp_FlushFileBuffers = FlushFileBuffers_Stub;
const decltype(&TerminateProcess) __imp_TerminateProcess = TerminateProcess_Stub;
const decltype(&GetCurrentThreadId) __imp_GetCurrentThreadId = GetCurrentThreadId_Stub;
const decltype(&GetCurrentProcessId) __imp_GetCurrentProcessId = GetCurrentProcessId_Stub;
const decltype(&GetCurrentProcess) __imp_GetCurrentProcess = GetCurrentProcess_Stub;
const decltype(&GetProcessHeap) __imp_GetProcessHeap = GetProcessHeap_Stub;
const decltype(&GetCommandLineA) __imp_GetCommandLineA = GetCommandLineA_Stub;
const decltype(&GetCommandLineW) __imp_GetCommandLineW = GetCommandLineW_Stub;
const decltype(&GetModuleHandleW) __imp_GetModuleHandleW = GetModuleHandleW_Stub;
const decltype(&IsDebuggerPresent) __imp_IsDebuggerPresent = IsDebuggerPresent_Stub;
const decltype(&SetEndOfFile) __imp_SetEndOfFile = SetEndOfFile_Stub;
const decltype(&ExitProcess) __imp_ExitProcess = ExitProcess_Stub;
const decltype(&InitializeSListHead) __imp_InitializeSListHead = PhInitializeSListHead;
const decltype(&InterlockedPushEntrySList) __imp_InterlockedPushEntrySList = RtlInterlockedPushEntrySList;
const decltype(&InterlockedFlushSList) __imp_InterlockedFlushSList = RtlInterlockedFlushSList;
const decltype(&QueryPerformanceCounter) __imp_QueryPerformanceCounter = QueryPerformanceCounter_Stub;
const decltype(&EnterCriticalSection) __imp_EnterCriticalSection = EnterCriticalSection_Stub;
const decltype(&LeaveCriticalSection) __imp_LeaveCriticalSection = LeaveCriticalSection_Stub;
const decltype(&DeleteCriticalSection) __imp_DeleteCriticalSection = DeleteCriticalSection_Stub;
const decltype(&AcquireSRWLockExclusive) __imp_AcquireSRWLockExclusive = RtlAcquireSRWLockExclusive;
const decltype(&ReleaseSRWLockExclusive) __imp_ReleaseSRWLockExclusive = RtlReleaseSRWLockExclusive;
const decltype(&HeapAlloc) __imp_HeapAlloc = HeapAllocate_Stub;
const decltype(&HeapReAlloc) __imp_HeapReAlloc = HeapReAlloc_Stub;
const decltype(&HeapSize) __imp_HeapSize = HeapSize_Stub;
const decltype(&HeapFree) __imp_HeapFree = HeapFree_Stub;
#endif

EXTERN_C_END
