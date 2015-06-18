#ifndef _PH_SYMPRVP_H
#define _PH_SYMPRVP_H

typedef BOOL (WINAPI *_SymInitialize)(
    _In_ HANDLE hProcess,
    _In_opt_ PCSTR UserSearchPath,
    _In_ BOOL fInvadeProcess
    );

typedef BOOL (WINAPI *_SymCleanup)(
    _In_ HANDLE hProcess
    );

typedef BOOL (WINAPI *_SymEnumSymbols)(
    _In_ HANDLE hProcess,
    _In_ ULONG64 BaseOfDll,
    _In_opt_ PCSTR Mask,
    _In_ PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
    _In_opt_ const PVOID UserContext
    );

typedef BOOL (WINAPI *_SymEnumSymbolsW)(
    _In_ HANDLE hProcess,
    _In_ ULONG64 BaseOfDll,
    _In_opt_ PCWSTR Mask,
    _In_ PSYM_ENUMERATESYMBOLS_CALLBACKW EnumSymbolsCallback,
    _In_opt_ const PVOID UserContext
    );

typedef BOOL (WINAPI *_SymFromAddr)(
    _In_ HANDLE hProcess,
    _In_ DWORD64 Address,
    _Out_opt_ PDWORD64 Displacement,
    _Inout_ PSYMBOL_INFO Symbol
    );

typedef BOOL (WINAPI *_SymFromAddrW)(
    _In_ HANDLE hProcess,
    _In_ DWORD64 Address,
    _Out_opt_ PDWORD64 Displacement,
    _Inout_ PSYMBOL_INFOW Symbol
    );

typedef BOOL (WINAPI *_SymFromName)(
    _In_ HANDLE hProcess,
    _In_ PCSTR Name,
    _Inout_ PSYMBOL_INFO Symbol
    );

typedef BOOL (WINAPI *_SymFromNameW)(
    _In_ HANDLE hProcess,
    _In_ PCWSTR Name,
    _Inout_ PSYMBOL_INFOW Symbol
    );

typedef BOOL (WINAPI *_SymGetLineFromAddr64)(
    _In_ HANDLE hProcess,
    _In_ DWORD64 dwAddr,
    _Out_ PDWORD pdwDisplacement,
    _Out_ PIMAGEHLP_LINE64 Line
    );

typedef BOOL (WINAPI *_SymGetLineFromAddrW64)(
    _In_ HANDLE hProcess,
    _In_ DWORD64 dwAddr,
    _Out_ PDWORD pdwDisplacement,
    _Out_ PIMAGEHLP_LINEW64 Line
    );

typedef DWORD64 (WINAPI *_SymLoadModule64)(
    _In_ HANDLE hProcess,
    _In_opt_ HANDLE hFile,
    _In_opt_ PCSTR ImageName,
    _In_opt_ PCSTR ModuleName,
    _In_ DWORD64 BaseOfDll,
    _In_ DWORD SizeOfDll
    );

typedef DWORD64 (WINAPI *_SymLoadModuleExW)(
    _In_ HANDLE hProcess,
    _In_ HANDLE hFile,
    _In_ PCWSTR ImageName,
    _In_ PCWSTR ModuleName,
    _In_ DWORD64 BaseOfDll,
    _In_ DWORD DllSize,
    _In_ PMODLOAD_DATA Data,
    _In_ DWORD Flags
    );

typedef DWORD (WINAPI *_SymGetOptions)();

typedef DWORD (WINAPI *_SymSetOptions)(
    _In_ DWORD SymOptions
    );

typedef BOOL (WINAPI *_SymGetSearchPath)(
    _In_ HANDLE hProcess,
    _Out_ PSTR SearchPath,
    _In_ DWORD SearchPathLength
    );

typedef BOOL (WINAPI *_SymGetSearchPathW)(
    _In_ HANDLE hProcess,
    _Out_ PWSTR SearchPath,
    _In_ DWORD SearchPathLength
    );

typedef BOOL (WINAPI *_SymSetSearchPath)(
    _In_ HANDLE hProcess,
    _In_opt_ PCSTR SearchPath
    );

typedef BOOL (WINAPI *_SymSetSearchPathW)(
    _In_ HANDLE hProcess,
    _In_opt_ PCWSTR SearchPath
    );

typedef BOOL (WINAPI *_SymUnloadModule64)(
    _In_ HANDLE hProcess,
    _In_ DWORD64 BaseOfDll
    );

typedef PVOID (WINAPI *_SymFunctionTableAccess64)(
    _In_ HANDLE hProcess,
    _In_ DWORD64 AddrBase
    );

typedef DWORD64 (WINAPI *_SymGetModuleBase64)(
    _In_ HANDLE hProcess,
    _In_ DWORD64 dwAddr
    );

typedef BOOL (WINAPI *_SymRegisterCallbackW64)(
    _In_ HANDLE hProcess,
    _In_ PSYMBOL_REGISTERED_CALLBACK64 CallbackFunction,
    _In_ ULONG64 UserContext
    );

typedef BOOL (WINAPI *_StackWalk64)(
    _In_ DWORD MachineType,
    _In_ HANDLE hProcess,
    _In_ HANDLE hThread,
    _Inout_ LPSTACKFRAME64 StackFrame,
    _Inout_ PVOID ContextRecord,
    _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
    _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
    _In_opt_ PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
    _In_opt_ PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
    );

typedef BOOL (WINAPI *_MiniDumpWriteDump)(
    _In_ HANDLE hProcess,
    _In_ DWORD ProcessId,
    _In_ HANDLE hFile,
    _In_ MINIDUMP_TYPE DumpType,
    _In_ PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    _In_ PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    _In_ PMINIDUMP_CALLBACK_INFORMATION CallbackParam
    );

typedef UINT_PTR (CALLBACK *_SymbolServerGetOptions)(
    VOID
    );

typedef BOOL (CALLBACK *_SymbolServerSetOptions)(
    _In_ UINT_PTR options,
    _In_ ULONG64 data
    );

#endif