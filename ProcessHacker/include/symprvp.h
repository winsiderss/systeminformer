#ifndef SYMPRVP_H
#define SYMPRVP_H

#include <dbghelp.h>

typedef BOOL (WINAPI *_SymInitialize)(
    __in HANDLE hProcess,
    __in_opt PCTSTR UserSearchPath,
    __in BOOL fInvadeProcess
    );

typedef BOOL (WINAPI *_SymCleanup)(
    __in HANDLE hProcess
    );

typedef BOOL (WINAPI *_SymEnumSymbols)(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in_opt PCTSTR Mask,
    __in PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
    __in_opt const PVOID UserContext
    );

typedef BOOL (WINAPI *_SymFromAddr)(
    __in HANDLE hProcess,
    __in DWORD64 Address,
    __out_opt PDWORD64 Displacement,
    __inout PSYMBOL_INFOW Symbol
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

typedef BOOL (WINAPI *_SymGetSearchPath)(
    __in HANDLE hProcess,
    __out PTSTR SearchPath,
    __in DWORD SearchPathLength
    );

typedef BOOL (WINAPI *_SymSetSearchPath)(
    __in HANDLE hProcess,
    __in_opt PCTSTR SearchPath
    );

typedef BOOL (WINAPI *_SymUnloadModule64)(
    __in HANDLE hProcess,
    __in DWORD64 BaseOfDll
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

typedef UINT_PTR (CALLBACK *_SymbolServerGetOptions)();

typedef BOOL (CALLBACK *_SymbolServerSetOptions)(
    __in UINT_PTR options,
    __in ULONG64 data
    );

#endif
