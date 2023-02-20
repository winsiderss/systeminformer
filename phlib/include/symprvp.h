/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2015
 *     dmex    2017-2023
 *
 */

#ifndef _PH_SYMPRVP_H
#define _PH_SYMPRVP_H

typedef BOOL (WINAPI *_SymInitializeW)(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PCWSTR UserSearchPath,
    _In_ BOOL fInvadeProcess
    );

typedef BOOL (WINAPI *_SymCleanup)(
    _In_ HANDLE ProcessHandle
    );

typedef BOOL (WINAPI *_SymEnumSymbolsW)(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 BaseOfDll,
    _In_opt_ PCWSTR Mask,
    _In_ PSYM_ENUMERATESYMBOLS_CALLBACKW EnumSymbolsCallback,
    _In_opt_ PVOID UserContext
    );

typedef BOOL (WINAPI *_SymFromAddrW)(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 Address,
    _Out_opt_ PULONG64 Displacement,
    _Inout_ PSYMBOL_INFOW Symbol
    );

typedef BOOL (WINAPI *_SymFromNameW)(
    _In_ HANDLE ProcessHandle,
    _In_ PCWSTR Name,
    _Inout_ PSYMBOL_INFOW Symbol
    );

typedef BOOL (WINAPI *_SymEnumTypesW)(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 BaseOfDll,
    _In_ PSYM_ENUMERATESYMBOLS_CALLBACKW EnumSymbolsCallback,
    _In_opt_ PVOID UserContext
    );

typedef BOOL (WINAPI *_SymGetModuleInfoW64)(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 qwAddr,
    _Out_ PIMAGEHLP_MODULEW64 ModuleInfo
    );

typedef BOOL (WINAPI *_SymGetTypeFromNameW)(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 BaseOfDll,
    _In_ PCWSTR Name,
    _Inout_ PSYMBOL_INFOW Symbol
    );

typedef BOOL (WINAPI* _SymGetTypeInfo)(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 BaseOfDll,
    _In_ ULONG TypeId,
    _In_ IMAGEHLP_SYMBOL_TYPE_INFO GetType,
    _Out_ PVOID pInfo
    );

typedef BOOL (WINAPI *_SymGetLineFromAddrW64)(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 dwAddr,
    _Out_ PULONG pdwDisplacement,
    _Out_ PIMAGEHLP_LINEW64 Line
    );

typedef ULONG64 (WINAPI *_SymLoadModuleExW)(
    _In_ HANDLE ProcessHandle,
    _In_opt_ HANDLE FileHandle,
    _In_opt_ PCWSTR ImageName,
    _In_opt_ PCWSTR ModuleName,
    _In_ ULONG64 BaseOfDll,
    _In_ ULONG DllSize,
    _In_opt_ PMODLOAD_DATA Data,
    _In_opt_ ULONG Flags
    );

typedef ULONG (WINAPI *_SymGetOptions)();

typedef ULONG (WINAPI *_SymSetOptions)(
    _In_ ULONG SymOptions
    );

typedef BOOL (WINAPI *_SymGetSearchPathW)(
    _In_ HANDLE ProcessHandle,
    _Out_ PWSTR SearchPath,
    _In_ ULONG SearchPathLength
    );

typedef BOOL (WINAPI *_SymSetSearchPathW)(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PCWSTR SearchPath
    );

typedef BOOL (WINAPI *_SymUnloadModule64)(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 BaseOfDll
    );

typedef PVOID (WINAPI *_SymFunctionTableAccess64)(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 AddrBase
    );

typedef ULONG64 (WINAPI *_SymGetModuleBase64)(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 dwAddr
    );

typedef BOOL (WINAPI *_SymRegisterCallbackW64)(
    _In_ HANDLE ProcessHandle,
    _In_ PSYMBOL_REGISTERED_CALLBACK64 CallbackFunction,
    _In_ ULONG64 UserContext
    );

typedef BOOL (WINAPI *_StackWalk64)(
    _In_ ULONG MachineType,
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE ThreadHandle,
    _Inout_ LPSTACKFRAME64 StackFrame,
    _Inout_ PVOID ContextRecord,
    _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
    _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
    _In_opt_ PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
    _In_opt_ PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
    );

typedef BOOL (WINAPI* _StackWalkEx)(
    _In_ ULONG MachineType,
    _In_ HANDLE hProcess,
    _In_ HANDLE hThread,
    _Inout_ LPSTACKFRAME_EX StackFrame,
    _Inout_ PVOID ContextRecord,
    _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
    _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
    _In_opt_ PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
    _In_opt_ PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress,
    _In_ ULONG Flags
    );

typedef ULONG (WINAPI* _SymAddrIncludeInlineTrace)(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 Address
    );

typedef BOOL (WINAPI* _SymQueryInlineTrace)(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 StartAddress,
    _In_ ULONG StartContext,
    _In_ ULONG64 StartRetAddress,
    _In_ ULONG64 CurAddress,
    _Out_ PULONG CurContext,
    _Out_ PULONG CurFrameIndex
    );

typedef BOOL (WINAPI* _SymFromInlineContextW)(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 Address,
    _In_ ULONG InlineContext,
    _Out_opt_ PULONG64 Displacement,
    _Inout_ PSYMBOL_INFOW Symbol
    );

typedef BOOL (WINAPI* _SymGetLineFromInlineContextW)(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 Address,
    _In_ ULONG InlineContext,
    _In_opt_ ULONG64 ModuleBaseAddress,
    _Out_ PULONG Displacement,
    _Out_ PIMAGEHLP_LINEW64 Line
    );

typedef BOOL (WINAPI *_MiniDumpWriteDump)(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG ProcessId,
    _In_ HANDLE FileHandle,
    _In_ MINIDUMP_TYPE DumpType,
    _In_opt_ PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    _In_opt_ PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    _In_opt_ PMINIDUMP_CALLBACK_INFORMATION CallbackParam
    );

typedef UINT_PTR (CALLBACK *_SymbolServerGetOptions)(
    VOID
    );

typedef BOOL (CALLBACK *_SymbolServerSetOptions)(
    _In_ UINT_PTR options,
    _In_ ULONG64 data
    );

typedef ULONG (WINAPI *_UnDecorateSymbolNameW)(
    _In_ PCWSTR DecoratedName,
    _Out_ PWSTR UnDecoratedName,
    _In_ ULONG UndecoratedLength,
    _In_ ULONG Flags
    );

// undocumented
typedef BOOLEAN (WINAPI *_SymGetDiaSource)(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 BaseOfDll,
    _Out_ PVOID* IDiaDataSource
    );

// undocumented
typedef BOOLEAN (WINAPI *_SymGetDiaSession)(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 BaseOfDll,
    _Out_ PVOID* IDiaSession
    );

// undocumented
typedef BOOLEAN (WINAPI *_SymSetDiaSession)(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 BaseOfDll,
    _In_ PVOID IDiaSession
    );

// undocumented
typedef VOID (WINAPI *_SymFreeDiaString)(
    _In_ PWSTR DiaString
    );

#endif
