#ifndef _PH_SYMPRV_H
#define _PH_SYMPRV_H

extern PPH_OBJECT_TYPE PhSymbolProviderType;
extern PH_CALLBACK PhSymInitCallback;

#define PH_MAX_SYMBOL_NAME_LEN 128

typedef struct _PH_SYMBOL_PROVIDER
{
    LIST_ENTRY ModulesListHead;
    PH_QUEUED_LOCK ModulesListLock;
    HANDLE ProcessHandle;
    BOOLEAN IsRealHandle;
    BOOLEAN IsRegistered;

    PH_INITONCE InitOnce;
    PH_AVL_TREE ModulesSet;
    PH_CALLBACK EventCallback;
} PH_SYMBOL_PROVIDER, *PPH_SYMBOL_PROVIDER;

typedef enum _PH_SYMBOL_RESOLVE_LEVEL
{
    PhsrlFunction,
    PhsrlModule,
    PhsrlAddress,
    PhsrlInvalid
} PH_SYMBOL_RESOLVE_LEVEL, *PPH_SYMBOL_RESOLVE_LEVEL;

typedef struct _PH_SYMBOL_INFORMATION
{
    ULONG64 Address;
    ULONG64 ModuleBase;
    ULONG Index;
    ULONG Size;
} PH_SYMBOL_INFORMATION, *PPH_SYMBOL_INFORMATION;

typedef struct _PH_SYMBOL_LINE_INFORMATION
{
    ULONG LineNumber;
    ULONG64 Address;
} PH_SYMBOL_LINE_INFORMATION, *PPH_SYMBOL_LINE_INFORMATION;

typedef enum _PH_SYMBOL_EVENT_TYPE
{
    SymbolDeferredSymbolLoadStart = 1,
    SymbolDeferredSymbolLoadComplete = 2,
    SymbolDeferredSymbolLoadFailure = 3,
    SymbolSymbolsUnloaded = 4,
    SymbolDeferredSymbolLoadCancel = 7
} PH_SYMBOL_EVENT_TYPE;

typedef struct _PH_SYMBOL_EVENT_DATA
{
    PPH_SYMBOL_PROVIDER SymbolProvider;
    PH_SYMBOL_EVENT_TYPE Type;

    ULONG64 BaseAddress;
    ULONG CheckSum;
    ULONG TimeStamp;
    PPH_STRING FileName;
} PH_SYMBOL_EVENT_DATA, *PPH_SYMBOL_EVENT_DATA;

BOOLEAN
NTAPI
PhSymbolProviderInitialization(
    VOID
    );

VOID
NTAPI
PhSymbolProviderCompleteInitialization(
    _In_opt_ PVOID DbgHelpBase
    );

PHLIBAPI
PPH_SYMBOL_PROVIDER
NTAPI
PhCreateSymbolProvider(
    _In_opt_ HANDLE ProcessId
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetLineFromAddress(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ ULONG64 Address,
    _Out_ PPH_STRING *FileName,
    _Out_opt_ PULONG Displacement,
    _Out_opt_ PPH_SYMBOL_LINE_INFORMATION Information
    );

PHLIBAPI
ULONG64
NTAPI
PhGetModuleFromAddress(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ ULONG64 Address,
    _Out_opt_ PPH_STRING *FileName
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetSymbolFromAddress(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ ULONG64 Address,
    _Out_opt_ PPH_SYMBOL_RESOLVE_LEVEL ResolveLevel,
    _Out_opt_ PPH_STRING *FileName,
    _Out_opt_ PPH_STRING *SymbolName,
    _Out_opt_ PULONG64 Displacement
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetSymbolFromName(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PWSTR Name,
    _Out_ PPH_SYMBOL_INFORMATION Information
    );

PHLIBAPI
BOOLEAN
NTAPI
PhLoadModuleSymbolProvider(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PWSTR FileName,
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Size
    );

PHLIBAPI
VOID
NTAPI
PhSetOptionsSymbolProvider(
    _In_ ULONG Mask,
    _In_ ULONG Value
    );

PHLIBAPI
VOID
NTAPI
PhSetSearchPathSymbolProvider(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PWSTR Path
    );

#ifdef _WIN64
NTSTATUS
NTAPI
PhAccessOutOfProcessFunctionEntry(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 ControlPc,
    _Out_ PRUNTIME_FUNCTION Function
    );
#endif

ULONG64
__stdcall
PhGetModuleBase64(
    _In_ HANDLE hProcess,
    _In_ DWORD64 dwAddr
    );

PVOID
__stdcall
PhFunctionTableAccess64(
    _In_ HANDLE hProcess,
    _In_ DWORD64 AddrBase
    );

#ifndef _DBGHELP_

// Some of the types used below are defined in dbghelp.h.

typedef struct _tagSTACKFRAME64 *LPSTACKFRAME64;
typedef struct _tagADDRESS64 *LPADDRESS64;

typedef BOOL (__stdcall *PREAD_PROCESS_MEMORY_ROUTINE64)(
    _In_ HANDLE hProcess,
    _In_ DWORD64 qwBaseAddress,
    _Out_writes_bytes_(nSize) PVOID lpBuffer,
    _In_ DWORD nSize,
    _Out_ LPDWORD lpNumberOfBytesRead
    );

typedef PVOID (__stdcall *PFUNCTION_TABLE_ACCESS_ROUTINE64)(
    _In_ HANDLE ahProcess,
    _In_ DWORD64 AddrBase
    );

typedef DWORD64 (__stdcall *PGET_MODULE_BASE_ROUTINE64)(
    _In_ HANDLE hProcess,
    _In_ DWORD64 Address
    );

typedef DWORD64 (__stdcall *PTRANSLATE_ADDRESS_ROUTINE64)(
    _In_ HANDLE hProcess,
    _In_ HANDLE hThread,
    _In_ LPADDRESS64 lpaddr
    );

typedef enum _MINIDUMP_TYPE MINIDUMP_TYPE;
typedef struct _MINIDUMP_EXCEPTION_INFORMATION *PMINIDUMP_EXCEPTION_INFORMATION;
typedef struct _MINIDUMP_USER_STREAM_INFORMATION *PMINIDUMP_USER_STREAM_INFORMATION;
typedef struct _MINIDUMP_CALLBACK_INFORMATION *PMINIDUMP_CALLBACK_INFORMATION;

#endif

PHLIBAPI
BOOLEAN
NTAPI
PhStackWalk(
    _In_ ULONG MachineType,
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE ThreadHandle,
    _Inout_ LPSTACKFRAME64 StackFrame,
    _Inout_ PVOID ContextRecord,
    _In_opt_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
    _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
    _In_opt_ PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
    _In_opt_ PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
    );

PHLIBAPI
BOOLEAN
NTAPI
PhWriteMiniDumpProcess(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE ProcessId,
    _In_ HANDLE FileHandle,
    _In_ MINIDUMP_TYPE DumpType,
    _In_opt_ PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    _In_opt_ PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    _In_opt_ PMINIDUMP_CALLBACK_INFORMATION CallbackParam
    );

// High-level stack walking

#define PH_THREAD_STACK_FRAME_I386 0x1
#define PH_THREAD_STACK_FRAME_AMD64 0x2
#define PH_THREAD_STACK_FRAME_KERNEL 0x4
#define PH_THREAD_STACK_FRAME_FPO_DATA_PRESENT 0x100

/** Contains information about a thread stack frame. */
typedef struct _PH_THREAD_STACK_FRAME
{
    PVOID PcAddress;
    PVOID ReturnAddress;
    PVOID FrameAddress;
    PVOID StackAddress;
    PVOID BStoreAddress;
    PVOID Params[4];
    ULONG Flags;
} PH_THREAD_STACK_FRAME, *PPH_THREAD_STACK_FRAME;

#define PH_WALK_I386_STACK 0x1
#define PH_WALK_AMD64_STACK 0x2
#define PH_WALK_KERNEL_STACK 0x10

/**
 * A callback function passed to PhWalkThreadStack()
 * and called for each stack frame.
 *
 * \param StackFrame A structure providing information about
 * the stack frame.
 * \param Context A user-defined value passed to
 * PhWalkThreadStack().
 *
 * \return TRUE to continue the stack walk, FALSE to
 * stop.
 */
typedef BOOLEAN (NTAPI *PPH_WALK_THREAD_STACK_CALLBACK)(
    _In_ PPH_THREAD_STACK_FRAME StackFrame,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhWalkThreadStack(
    _In_ HANDLE ThreadHandle,
    _In_opt_ HANDLE ProcessHandle,
    _In_opt_ PCLIENT_ID ClientId,
    _In_opt_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ ULONG Flags,
    _In_ PPH_WALK_THREAD_STACK_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

#endif
