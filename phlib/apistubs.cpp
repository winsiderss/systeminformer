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

static HANDLE WINAPI CreateFileW_Stub(
    _In_ PCWSTR lpFileName,
    _In_ ULONG dwDesiredAccess,
    _In_ ULONG dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ ULONG dwCreationDisposition,
    _In_ ULONG dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    ULONG createDisposition;
    ULONG createOptions;

    switch (dwCreationDisposition)
    {
    case CREATE_NEW:
        createDisposition = FILE_CREATE;
        break;
    case CREATE_ALWAYS:
        createDisposition = FILE_OVERWRITE_IF;
        break;
    case OPEN_EXISTING:
        createDisposition = FILE_OPEN;
        break;
    case OPEN_ALWAYS:
        createDisposition = FILE_OPEN_IF;
        break;
    case TRUNCATE_EXISTING:
        createDisposition = FILE_OVERWRITE;
        break;
    default:
        PhSetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    createOptions = 0;

    if (dwFlagsAndAttributes & FILE_FLAG_WRITE_THROUGH)
        createOptions |= FILE_WRITE_THROUGH;
    if (dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING)
        createOptions |= FILE_NO_INTERMEDIATE_BUFFERING;
    if (dwFlagsAndAttributes & FILE_FLAG_RANDOM_ACCESS)
        createOptions |= FILE_RANDOM_ACCESS;
    if (dwFlagsAndAttributes & FILE_FLAG_SEQUENTIAL_SCAN)
        createOptions |= FILE_SEQUENTIAL_ONLY;
    if (dwFlagsAndAttributes & FILE_FLAG_DELETE_ON_CLOSE)
        createOptions |= FILE_DELETE_ON_CLOSE;
    if (dwFlagsAndAttributes & FILE_FLAG_BACKUP_SEMANTICS)
        createOptions |= FILE_OPEN_FOR_BACKUP_INTENT;
    if (dwFlagsAndAttributes & FILE_FLAG_POSIX_SEMANTICS)
        createOptions |= FILE_DIRECTORY_FILE;
    if (dwFlagsAndAttributes & FILE_FLAG_OPEN_REPARSE_POINT)
        createOptions |= FILE_OPEN_REPARSE_POINT;
    if (dwFlagsAndAttributes & FILE_FLAG_OPEN_NO_RECALL)
        createOptions |= FILE_OPEN_NO_RECALL;

    if (!(dwFlagsAndAttributes & FILE_FLAG_OVERLAPPED))
        createOptions |= FILE_SYNCHRONOUS_IO_NONALERT;

    status = PhCreateFileWin32(
        &fileHandle,
        lpFileName,
        dwDesiredAccess,
        dwFlagsAndAttributes & 0x0000FFFF,
        dwShareMode,
        createDisposition,
        createOptions
        );

    PhSetLastError(PhNtStatusToDosError(status));

    if (NT_SUCCESS(status))
        return fileHandle;

    return INVALID_HANDLE_VALUE;
}

static BOOL WINAPI ReadFile_Stub(
    _In_ HANDLE File,
    _Out_writes_bytes_to_opt_(NumberOfBytesToRead, *NumberOfBytesRead) PVOID Buffer,
    _In_ ULONG NumberOfBytesToRead,
    _Out_opt_ PULONG NumberOfBytesRead,
    _Inout_opt_ LPOVERLAPPED Overlapped
    )
{
    NTSTATUS status;

    if (lpOverlapped)
    {
        PhSetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    status = PhReadFile(
        File,
        Buffer,
        NumberOfBytesToRead,
        NULL,
        NumberOfBytesRead
        );

    PhSetLastError(PhNtStatusToDosError(status));
    return NT_SUCCESS(status);
}

static BOOL WINAPI WriteFile_Stub(
    _In_ HANDLE hFile,
    _In_reads_bytes_opt_(NumberOfBytesToWrite) PCVOID Buffer,
    _In_ ULONG NumberOfBytesToWrite,
    _Out_opt_ LPULONG NumberOfBytesWritten,
    _Inout_opt_ LPOVERLAPPED Overlapped
    )
{
    NTSTATUS status;

    if (lpOverlapped)
    {
        PhSetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    status = PhWriteFile(
        hFile,
        (PVOID)Buffer,
        NumberOfBytesToWrite,
        NULL,
        NumberOfBytesWritten
        );

    PhSetLastError(PhNtStatusToDosError(status));
    return NT_SUCCESS(status);
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

static ULONG WINAPI GetTickCount_Stub(
    VOID
    )
{
    return (ULONG)NtGetTickCount();
}

static BOOL WINAPI FreeConsole_Stub(
    VOID
    )
{
    return TRUE;
}

static BOOL WINAPI HeapValidate_Stub(
    _In_ HANDLE Heap,
    _In_ ULONG Flags,
    _In_opt_ PCVOID Mem
    )
{
    return RtlValidateHeap(Heap, Flags, (PVOID)Mem);
}

static VOID WINAPI GetSystemInfo_Stub(
    _Out_ LPSYSTEM_INFO SystemInfo
    )
{
    SYSTEM_BASIC_INFORMATION basicInfo;
    SYSTEM_PROCESSOR_INFORMATION processorInfo;

    memset(SystemInfo, 0, sizeof(SYSTEM_INFO));

    if (NT_SUCCESS(NtQuerySystemInformation(SystemBasicInformation, &basicInfo, sizeof(SYSTEM_BASIC_INFORMATION), NULL)))
    {
        SystemInfo->dwPageSize = basicInfo.PageSize;
        SystemInfo->lpMinimumApplicationAddress = (PVOID)basicInfo.MinimumUserModeAddress;
        SystemInfo->lpMaximumApplicationAddress = (PVOID)basicInfo.MaximumUserModeAddress;
        SystemInfo->dwActiveProcessorMask = (ULONG_PTR)basicInfo.ActiveProcessorsAffinityMask;
        SystemInfo->dwNumberOfProcessors = basicInfo.NumberOfProcessors;
        SystemInfo->dwAllocationGranularity = basicInfo.AllocationGranularity;
    }

    if (NT_SUCCESS(NtQuerySystemInformation(SystemProcessorInformation, &processorInfo, sizeof(SYSTEM_PROCESSOR_INFORMATION), NULL)))
    {
        SystemInfo->wProcessorArchitecture = processorInfo.ProcessorArchitecture;
        SystemInfo->wProcessorLevel = processorInfo.ProcessorLevel;
        SystemInfo->wProcessorRevision = processorInfo.ProcessorRevision;
    }
}

static BOOL WINAPI GetModuleHandleExW_Stub(
    _In_ ULONG Flags,
    _In_opt_ PCWSTR ModuleName,
    _Out_ HMODULE* ModuleAddress
    )
{
    if (ModuleName)
    {
        PVOID moduleHandle;

        if (Flags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)
        {
            if (moduleHandle = (HMODULE)RtlPcToFileHeader((PVOID)ModuleName, (PVOID*)&moduleHandle))
            {
                *ModuleAddress = (HMODULE)moduleHandle;
                return TRUE;
            }
        }
        else
        {
            if (moduleHandle = PhGetDllHandle(ModuleName))
            {
                *ModuleAddress = (HMODULE)moduleHandle;
                return TRUE;
            }
        }

        PhSetLastError(PhNtStatusToDosError(STATUS_DLL_NOT_FOUND));
        return FALSE;
    }
    else
    {
        *ModuleAddress = (HMODULE)NtCurrentPeb()->ImageBaseAddress;;
        return TRUE;
    }
}

static SIZE_T WINAPI VirtualQuery_Stub(
    _In_opt_ LPCVOID lpAddress,
    _Out_writes_bytes_to_(dwLength, return) PMEMORY_BASIC_INFORMATION lpBuffer,
    _In_ SIZE_T dwLength
    )
{
    NTSTATUS status;
    SIZE_T returnLength;

    status = NtQueryVirtualMemory(
        NtCurrentProcess(),
        (PVOID)lpAddress,
        MemoryBasicInformation,
        lpBuffer,
        dwLength,
        &returnLength
        );

    if (NT_SUCCESS(status))
        return returnLength;

    PhSetLastError(PhNtStatusToDosError(status));
    return 0;
}

static PVOID WINAPI EncodePointer_Stub(
    _In_ PVOID Ptr
    )
{
    return RtlEncodePointer(Ptr);
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

static BOOL WINAPI SetFilePointerEx_Stub(
    _In_ HANDLE File,
    _In_ LARGE_INTEGER DistanceToMove,
    _Out_opt_ PLARGE_INTEGER NewFilePointer,
    _In_ ULONG MoveMethod
    )
{
    NTSTATUS status;
    LARGE_INTEGER position;
    LARGE_INTEGER current;
    LARGE_INTEGER fileSize;

    switch (MoveMethod)
    {
    case FILE_BEGIN:
        position = DistanceToMove;
        break;
    case FILE_CURRENT:
        {
            status = PhGetFilePosition(File, &current);

            if (!NT_SUCCESS(status))
            {
                PhSetLastError(PhNtStatusToDosError(status));
                return FALSE;
            }

            position.QuadPart = current.QuadPart + DistanceToMove.QuadPart;
        }
        break;
    case FILE_END:
        {
            status = PhGetFileSize(File, &fileSize);

            if (!NT_SUCCESS(status))
            {
                PhSetLastError(PhNtStatusToDosError(status));
                return FALSE;
            }

            position.QuadPart = fileSize.QuadPart + DistanceToMove.QuadPart;
        }
        break;
    default:
        PhSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    status = PhSetFilePosition(File, &position);

    PhSetLastError(PhNtStatusToDosError(status));

    if (NT_SUCCESS(status))
    {
        if (NewFilePointer)
        {
            *NewFilePointer = position;
        }

        return TRUE;
    }

    return FALSE;
}

DECLSPEC_NORETURN
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

static VOID WINAPI GetSystemTimeAsFileTime_Stub(
    _Out_ PFILETIME SystemTime
    )
{
    LARGE_INTEGER systemTime;

    PhQuerySystemTime(&systemTime);

    SystemTime->dwLowDateTime = systemTime.LowPart;
    SystemTime->dwHighDateTime = systemTime.HighPart;
}

static BOOL WINAPI VirtualProtect_Stub(
    _In_ PVOID Address,
    _In_ SIZE_T Size,
    _In_ ULONG NewProtect,
    _Out_opt_ PULONG OldProtect
    )
{
    NTSTATUS status;
    ULONG oldProtect = 0;

    status = PhProtectVirtualMemory(
        NtCurrentProcess(),
        Address,
        Size,
        NewProtect,
        &oldProtect
        );

    PhSetLastError(PhNtStatusToDosError(status));

    if (NT_SUCCESS(status))
    {
        if (OldProtect)
        {
            *OldProtect = oldProtect;
        }

        return TRUE;
    }

    return FALSE;
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

static BOOL WINAPI SetEnvironmentVariableW_Stub(
    _In_ PCWSTR Name,
    _In_opt_ PCWSTR Value
    )
{
    NTSTATUS status;

    status = RtlSetEnvironmentVar(
        NULL,
        Name,
        PhCountStringZ(Name),
        Value,
        PhCountStringZ(Value)
        );

    PhSetLastError(PhNtStatusToDosError(status));

    if (NT_SUCCESS(status))
        return TRUE;

    return FALSE;
}

static BOOL WINAPI HeapQueryInformation_Stub(
    _In_opt_ HANDLE HeapHandle,
    _In_ HEAP_INFORMATION_CLASS HeapInformationClass,
    _Out_writes_bytes_to_opt_(HeapInformationLength, *ReturnLength) PVOID HeapInformation,
    _In_ SIZE_T HeapInformationLength,
    _Out_opt_ PSIZE_T ReturnLength
    )
{
    NTSTATUS status;

    status = RtlQueryHeapInformation(
        HeapHandle,
        HeapInformationClass,
        HeapInformation,
        HeapInformationLength,
        ReturnLength
        );

    PhSetLastError(PhNtStatusToDosError(status));

    if (NT_SUCCESS(status))
        return TRUE;

    return FALSE;
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
__declspec(selectany) decltype(&CloseHandle) __identifier("_imp__CloseHandle@4") = CloseHandle_Stub;
__declspec(selectany) decltype(&CreateFileW) __identifier("_imp__CreateFileW@28") = CreateFileW_Stub;
__declspec(selectany) decltype(&ReadFile) __identifier("_imp__ReadFile@20") = ReadFile_Stub;
__declspec(selectany) decltype(&WriteFile) __identifier("_imp__WriteFile@20") = WriteFile_Stub;
__declspec(selectany) decltype(&GetFileSizeEx) __identifier("_imp__GetFileSizeEx@4") = GetFileSizeEx_Stub;
__declspec(selectany) decltype(&GetProcAddress) __identifier("_imp__GetProcAddress@8") = GetProcAddress_Stub;
__declspec(selectany) decltype(&FlushFileBuffers) __identifier("_imp__FlushFileBuffers@4") = FlushFileBuffers_Stub;
__declspec(selectany) decltype(&GetTickCount) __identifier("_imp__GetTickCount@0") = GetTickCount_Stub;
__declspec(selectany) decltype(&FreeConsole) __identifier("_imp__FreeConsole@0") = FreeConsole_Stub;
__declspec(selectany) decltype(&HeapValidate) __identifier("_imp__HeapValidate@12") = HeapValidate_Stub;
__declspec(selectany) decltype(&GetSystemInfo) __identifier("_imp__GetSystemInfo@4") = GetSystemInfo_Stub;
__declspec(selectany) decltype(&GetModuleHandleExW) __identifier("_imp__GetModuleHandleExW@12") = GetModuleHandleExW_Stub;
__declspec(selectany) decltype(&VirtualQuery) __identifier("_imp__VirtualQuery@12") = VirtualQuery_Stub;
__declspec(selectany) decltype(&EncodePointer) __identifier("_imp__EncodePointer@4") = EncodePointer_Stub;
__declspec(selectany) decltype(&TerminateProcess) __identifier("_imp__TerminateProcess@8") = TerminateProcess_Stub;
__declspec(selectany) decltype(&GetCurrentThreadId) __identifier("_imp__GetCurrentThreadId@0") = GetCurrentThreadId_Stub;
__declspec(selectany) decltype(&GetCurrentProcessId) __identifier("_imp__GetCurrentProcessId@0") = GetCurrentProcessId_Stub;
__declspec(selectany) decltype(&GetCurrentProcess) __identifier("_imp__GetCurrentProcess@0") = GetCurrentProcess_Stub;
__declspec(selectany) decltype(&GetProcessHeap) __identifier("_imp__GetProcessHeap@0") = GetProcessHeap_Stub;
__declspec(selectany) decltype(&GetCommandLineA) __identifier("_imp__GetCommandLineA@0") = GetCommandLineA_Stub;
__declspec(selectany) decltype(&GetCommandLineW) __identifier("_imp__GetCommandLineW@0") = GetCommandLineW_Stub;
__declspec(selectany) decltype(&GetModuleHandleW) __identifier("_imp__GetModuleHandleW@4") = GetModuleHandleW_Stub;
__declspec(selectany) decltype(&IsDebuggerPresent) __identifier("_imp__IsDebuggerPresent@0") = IsDebuggerPresent_Stub;
__declspec(selectany) decltype(&SetEndOfFile) __identifier("_imp__SetEndOfFile@4") = SetEndOfFile_Stub;
__declspec(selectany) decltype(&ExitProcess) __identifier("_imp__ExitProcess@4") = ExitProcess_Stub;
__declspec(selectany) decltype(&VirtualProtect) __identifier("_imp__VirtualProtect@16") = VirtualProtect_Stub;
__declspec(selectany) decltype(&InitializeSListHead) __identifier("_imp__InitializeSListHead@4") = PhInitializeSListHead;
__declspec(selectany) decltype(&InterlockedPushEntrySList) __identifier("_imp__InterlockedPushEntrySList@8") = RtlInterlockedPushEntrySList;
__declspec(selectany) decltype(&InterlockedFlushSList) __identifier("_imp__InterlockedFlushSList@4") = RtlInterlockedFlushSList;
__declspec(selectany) decltype(&QueryPerformanceCounter) __identifier("_imp__QueryPerformanceCounter@4") = QueryPerformanceCounter_Stub;
__declspec(selectany) decltype(&GetSystemTimeAsFileTime) __identifier("_imp__GetSystemTimeAsFileTime@4") = GetSystemTimeAsFileTime_Stub;
__declspec(selectany) decltype(&SetFilePointerEx) __identifier("_imp__SetFilePointerEx@16") = SetFilePointerEx_Stub;
__declspec(selectany) decltype(&EnterCriticalSection) __identifier("_imp__EnterCriticalSection@4") = EnterCriticalSection_Stub;
__declspec(selectany) decltype(&LeaveCriticalSection) __identifier("_imp__LeaveCriticalSection@4") = LeaveCriticalSection_Stub;
__declspec(selectany) decltype(&DeleteCriticalSection) __identifier("_imp__DeleteCriticalSection@4") = DeleteCriticalSection_Stub;
__declspec(selectany) decltype(&AcquireSRWLockExclusive) __identifier("_imp__AcquireSRWLockExclusive@4") = RtlAcquireSRWLockExclusive;
__declspec(selectany) decltype(&ReleaseSRWLockExclusive) __identifier("_imp__ReleaseSRWLockExclusive@4") = RtlReleaseSRWLockExclusive;
__declspec(selectany) decltype(&SetEnvironmentVariableW) __identifier("_imp__SetEnvironmentVariableW@8") = SetEnvironmentVariableW_Stub;
__declspec(selectany) decltype(&HeapQueryInformation) __identifier("_imp__HeapQueryInformation@20") = HeapQueryInformation_Stub;
__declspec(selectany) decltype(&HeapAlloc) __identifier("_imp__HeapAlloc@12") = HeapAllocate_Stub;
__declspec(selectany) decltype(&HeapReAlloc) __identifier("_imp__HeapReAlloc@16") = HeapReAlloc_Stub;
__declspec(selectany) decltype(&HeapSize) __identifier("_imp__HeapSize@12") = HeapSize_Stub;
__declspec(selectany) decltype(&HeapFree) __identifier("_imp__HeapFree@12") = HeapFree_Stub;
#pragma warning(pop)
#else
__declspec(selectany) decltype(&CloseHandle) __imp_CloseHandle = CloseHandle_Stub;
__declspec(selectany) decltype(&CreateFileW) __imp_CreateFileW = CreateFileW_Stub;
__declspec(selectany) decltype(&ReadFile) __imp_ReadFile = ReadFile_Stub;
__declspec(selectany) decltype(&WriteFile) __imp_WriteFile = WriteFile_Stub;
__declspec(selectany) decltype(&GetFileSizeEx) __imp_GetFileSizeEx = GetFileSizeEx_Stub;
__declspec(selectany) decltype(&GetProcAddress) __imp_GetProcAddress = GetProcAddress_Stub;
__declspec(selectany) decltype(&FlushFileBuffers) __imp_FlushFileBuffers = FlushFileBuffers_Stub;
__declspec(selectany) decltype(&GetTickCount) __imp_GetTickCount = GetTickCount_Stub;
__declspec(selectany) decltype(&FreeConsole) __imp_FreeConsole = FreeConsole_Stub;
__declspec(selectany) decltype(&HeapValidate) __imp_HeapValidate = HeapValidate_Stub;
__declspec(selectany) decltype(&GetSystemInfo) __imp_GetSystemInfo = GetSystemInfo_Stub;
__declspec(selectany) decltype(&GetModuleHandleExW) __imp_GetModuleHandleExW = GetModuleHandleExW_Stub;
__declspec(selectany) decltype(&VirtualQuery) __imp_VirtualQuery = VirtualQuery_Stub;
__declspec(selectany) decltype(&EncodePointer) __imp_EncodePointer = EncodePointer_Stub;
__declspec(selectany) decltype(&TerminateProcess) __imp_TerminateProcess = TerminateProcess_Stub;
__declspec(selectany) decltype(&GetCurrentThreadId) __imp_GetCurrentThreadId = GetCurrentThreadId_Stub;
__declspec(selectany) decltype(&GetCurrentProcessId) __imp_GetCurrentProcessId = GetCurrentProcessId_Stub;
__declspec(selectany) decltype(&GetCurrentProcess) __imp_GetCurrentProcess = GetCurrentProcess_Stub;
__declspec(selectany) decltype(&GetProcessHeap) __imp_GetProcessHeap = GetProcessHeap_Stub;
__declspec(selectany) decltype(&GetCommandLineA) __imp_GetCommandLineA = GetCommandLineA_Stub;
__declspec(selectany) decltype(&GetCommandLineW) __imp_GetCommandLineW = GetCommandLineW_Stub;
__declspec(selectany) decltype(&GetModuleHandleW) __imp_GetModuleHandleW = GetModuleHandleW_Stub;
__declspec(selectany) decltype(&IsDebuggerPresent) __imp_IsDebuggerPresent = IsDebuggerPresent_Stub;
__declspec(selectany) decltype(&SetEndOfFile) __imp_SetEndOfFile = SetEndOfFile_Stub;
__declspec(selectany) decltype(&ExitProcess) __imp_ExitProcess = ExitProcess_Stub;
__declspec(selectany) decltype(&VirtualProtect) __imp_VirtualProtect = VirtualProtect_Stub;
__declspec(selectany) decltype(&InitializeSListHead) __imp_InitializeSListHead = PhInitializeSListHead;
__declspec(selectany) decltype(&InterlockedPushEntrySList) __imp_InterlockedPushEntrySList = RtlInterlockedPushEntrySList;
__declspec(selectany) decltype(&InterlockedFlushSList) __imp_InterlockedFlushSList = RtlInterlockedFlushSList;
__declspec(selectany) decltype(&QueryPerformanceCounter) __imp_QueryPerformanceCounter = QueryPerformanceCounter_Stub;
__declspec(selectany) decltype(&GetSystemTimeAsFileTime) __imp_GetSystemTimeAsFileTime = GetSystemTimeAsFileTime_Stub;
__declspec(selectany) decltype(&SetFilePointerEx) __imp_SetFilePointerEx = SetFilePointerEx_Stub;
__declspec(selectany) decltype(&EnterCriticalSection) __imp_EnterCriticalSection = EnterCriticalSection_Stub;
__declspec(selectany) decltype(&LeaveCriticalSection) __imp_LeaveCriticalSection = LeaveCriticalSection_Stub;
__declspec(selectany) decltype(&DeleteCriticalSection) __imp_DeleteCriticalSection = DeleteCriticalSection_Stub;
__declspec(selectany) decltype(&AcquireSRWLockExclusive) __imp_AcquireSRWLockExclusive = RtlAcquireSRWLockExclusive;
__declspec(selectany) decltype(&ReleaseSRWLockExclusive) __imp_ReleaseSRWLockExclusive = RtlReleaseSRWLockExclusive;
__declspec(selectany) decltype(&SetEnvironmentVariableW) __imp_SetEnvironmentVariableW = SetEnvironmentVariableW_Stub;
__declspec(selectany) decltype(&HeapQueryInformation) __imp_HeapQueryInformation = HeapQueryInformation_Stub;
__declspec(selectany) decltype(&HeapAlloc) __imp_HeapAlloc = HeapAllocate_Stub;
__declspec(selectany) decltype(&HeapReAlloc) __imp_HeapReAlloc = HeapReAlloc_Stub;
__declspec(selectany) decltype(&HeapSize) __imp_HeapSize = HeapSize_Stub;
__declspec(selectany) decltype(&HeapFree) __imp_HeapFree = HeapFree_Stub;
#endif

EXTERN_C_END
