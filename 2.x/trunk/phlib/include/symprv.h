#ifndef _PH_SYMPRV_H
#define _PH_SYMPRV_H

#include <dbghelp.h>

typedef BOOL (WINAPI *_SymInitialize)(
    __in HANDLE hProcess,
    __in_opt PCSTR UserSearchPath,
    __in BOOL fInvadeProcess
    );

typedef BOOL (WINAPI *_SymCleanup)(
    __in HANDLE hProcess
    );

typedef BOOL (WINAPI *_SymEnumSymbolsW)(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in_opt PCWSTR Mask,
    __in PSYM_ENUMERATESYMBOLS_CALLBACKW EnumSymbolsCallback,
    __in_opt const PVOID UserContext
    );

typedef BOOL (WINAPI *_SymFromAddrW)(
    __in HANDLE hProcess,
    __in DWORD64 Address,
    __out_opt PDWORD64 Displacement,
    __inout PSYMBOL_INFOW Symbol
    );

typedef BOOL (WINAPI *_SymFromNameW)(
    __in HANDLE hProcess,
    __in PCWSTR Name,
    __inout PSYMBOL_INFOW Symbol
    );

typedef BOOL (WINAPI *_SymGetLineFromAddrW64)(
    __in HANDLE hProcess,
    __in DWORD64 dwAddr,
    __out PDWORD pdwDisplacement,
    __out PIMAGEHLP_LINEW64 Line
    );

typedef DWORD64 (WINAPI *_SymLoadModule64)(
    __in HANDLE hProcess,
    __in_opt HANDLE hFile,
    __in_opt PCSTR ImageName,
    __in_opt PCSTR ModuleName,
    __in DWORD64 BaseOfDll,
    __in DWORD SizeOfDll
    );

typedef DWORD (WINAPI *_SymGetOptions)();

typedef DWORD (WINAPI *_SymSetOptions)(
    __in DWORD SymOptions
    );

typedef BOOL (WINAPI *_SymGetSearchPathW)(
    __in HANDLE hProcess,
    __out PWSTR SearchPath,
    __in DWORD SearchPathLength
    );

typedef BOOL (WINAPI *_SymSetSearchPathW)(
    __in HANDLE hProcess,
    __in_opt PCWSTR SearchPath
    );

typedef BOOL (WINAPI *_SymUnloadModule64)(
    __in HANDLE hProcess,
    __in DWORD64 BaseOfDll
    );

typedef PVOID (WINAPI *_SymFunctionTableAccess64)(
    __in HANDLE hProcess,
    __in DWORD64 AddrBase
    );

typedef DWORD64 (WINAPI *_SymGetModuleBase64)(
    __in HANDLE hProcess,
    __in DWORD64 dwAddr
    );

typedef BOOL (WINAPI *_StackWalk64)(
    __in DWORD MachineType,
    __in HANDLE hProcess,
    __in HANDLE hThread,
    __inout LPSTACKFRAME64 StackFrame,
    __inout PVOID ContextRecord,
    __in_opt PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
    __in_opt PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
    __in_opt PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
    __in_opt PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
    );

typedef BOOL (WINAPI *_MiniDumpWriteDump)(
    __in HANDLE hProcess,
    __in DWORD ProcessId,
    __in HANDLE hFile,
    __in MINIDUMP_TYPE DumpType,
    __in PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    __in PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    __in PMINIDUMP_CALLBACK_INFORMATION CallbackParam
    );

typedef UINT_PTR (CALLBACK *_SymbolServerGetOptions)();

typedef BOOL (CALLBACK *_SymbolServerSetOptions)(
    __in UINT_PTR options,
    __in ULONG64 data
    );

BOOLEAN PhStackWalk(
    __in ULONG MachineType,
    __in HANDLE ProcessHandle,
    __in HANDLE ThreadHandle,
    __inout STACKFRAME64 *StackFrame,
    __inout PVOID ContextRecord,
    __in_opt PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
    __in_opt PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
    __in_opt PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
    __in_opt PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
    );

#ifndef SYMPRV_PRIVATE
extern _MiniDumpWriteDump MiniDumpWriteDump_I;
#endif

#endif
