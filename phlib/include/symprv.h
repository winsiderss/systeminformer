/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2023
 *
 */

#ifndef _PH_SYMPRV_H
#define _PH_SYMPRV_H

EXTERN_C_START

extern PPH_OBJECT_TYPE PhSymbolProviderType;
extern PH_CALLBACK PhSymbolEventCallback;

#define PH_MAX_SYMBOL_NAME_LEN MAX_SYM_NAME

typedef struct _PH_SYMBOL_PROVIDER
{
    LIST_ENTRY ModulesListHead;
    PH_QUEUED_LOCK ModulesListLock;
    HANDLE ProcessHandle;

    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN IsRealHandle : 1;
            BOOLEAN IsRegistered : 1;
            BOOLEAN Terminating : 1;
            BOOLEAN Spare : 5;
        };
    };

    PH_INITONCE InitOnce;
    PH_AVL_TREE ModulesSet;
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
    PVOID Address;
    PVOID ModuleBase;
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
    PH_SYMBOL_EVENT_TYPE_LOAD_START,
    PH_SYMBOL_EVENT_TYPE_LOAD_END,
    PH_SYMBOL_EVENT_TYPE_PROGRESS,
} PH_SYMBOL_EVENT_TYPE;

typedef struct _PH_SYMBOL_EVENT_DATA
{
    PH_SYMBOL_EVENT_TYPE EventType;
    PVOID EventMessage;
    ULONG64 EventProgress;
} PH_SYMBOL_EVENT_DATA, *PPH_SYMBOL_EVENT_DATA;

PHLIBAPI
PPH_SYMBOL_PROVIDER
NTAPI
PhCreateSymbolProvider(
    _In_opt_ HANDLE ProcessId
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetLineFromAddress(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PVOID Address,
    _Out_ PPH_STRING *FileName,
    _Out_opt_ PULONG Displacement,
    _Out_opt_ PPH_SYMBOL_LINE_INFORMATION Information
    );

_Success_(return != 0)
PHLIBAPI
PVOID
NTAPI
PhGetModuleFromAddress(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PVOID Address,
    _Out_opt_ PPH_STRING *FileName
    );

typedef struct _PH_SYMBOL_MODULE *PPH_SYMBOL_MODULE;

PHLIBAPI
PPH_SYMBOL_MODULE
NTAPI
PhGetSymbolModuleFromAddress(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PVOID Address
    );

_Success_(return != NULL)
PHLIBAPI
PPH_STRING
NTAPI
PhGetSymbolFromAddress(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PVOID Address,
    _Out_opt_ PPH_SYMBOL_RESOLVE_LEVEL ResolveLevel,
    _Out_opt_ PPH_STRING *FileName,
    _Out_opt_ PPH_STRING *SymbolName,
    _Out_opt_ PULONG64 Displacement
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetSymbolFromName(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PCWSTR Name,
    _Out_ PPH_SYMBOL_INFORMATION Information
    );

PHLIBAPI
BOOLEAN
NTAPI
PhLoadModuleSymbolProvider(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PPH_STRING FileName,
    _In_ PVOID BaseAddress,
    _In_ ULONG Size
    );

PHLIBAPI
BOOLEAN
NTAPI
PhLoadFileNameSymbolProvider(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PPH_STRING FileName,
    _In_ PVOID BaseAddress,
    _In_ ULONG Size
    );

PHLIBAPI
VOID
NTAPI
PhLoadSymbolProviderModules(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ HANDLE ProcessId
    );

PHLIBAPI
VOID
NTAPI
PhLoadModulesForVirtualSymbolProvider(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_opt_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle
    );

#define PH_SYMOPT_UNDNAME 0x1

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
    _In_ PCWSTR Path
    );

#ifdef _WIN64
PHLIBAPI
NTSTATUS
NTAPI
PhAccessOutOfProcessFunctionEntry(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG64 ControlPc,
    _Out_ PRUNTIME_FUNCTION Function
    );
#endif

PHLIBAPI
ULONG64
__stdcall
PhGetModuleBase64(
    _In_ HANDLE hProcess,
    _In_ ULONG64 dwAddr
    );

PHLIBAPI
PVOID
__stdcall
PhFunctionTableAccess64(
    _In_ HANDLE hProcess,
    _In_ ULONG64 AddrBase
    );

#ifndef _DBGHELP_

// Some of the types used below are defined in dbghelp.h.

typedef struct _tagSTACKFRAME64 *LPSTACKFRAME64;
typedef struct _tagSTACKFRAME_EX* LPSTACKFRAME_EX;
typedef struct _tagADDRESS64 *LPADDRESS64;

typedef BOOL (__stdcall *PREAD_PROCESS_MEMORY_ROUTINE64)(
    _In_ HANDLE hProcess,
    _In_ ULONG64 qwBaseAddress,
    _Out_writes_bytes_(nSize) PVOID lpBuffer,
    _In_ ULONG nSize,
    _Out_ PULONG lpNumberOfBytesRead
    );

typedef PVOID (__stdcall *PFUNCTION_TABLE_ACCESS_ROUTINE64)(
    _In_ HANDLE ahProcess,
    _In_ ULONG64 AddrBase
    );

typedef ULONG64 (__stdcall *PGET_MODULE_BASE_ROUTINE64)(
    _In_ HANDLE hProcess,
    _In_ ULONG64 Address
    );

typedef ULONG64 (__stdcall *PTRANSLATE_ADDRESS_ROUTINE64)(
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
    _Inout_ LPSTACKFRAME_EX StackFrame,
    _Inout_ PVOID ContextRecord,
    _In_opt_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
    _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
    _In_opt_ PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
    _In_opt_ PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
    );

PHLIBAPI
HRESULT
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

#define PH_THREAD_STACK_FRAME_KERNEL           0x0001
#define PH_THREAD_STACK_FRAME_FPO_DATA_PRESENT 0x0002

/** Contains information about a thread stack frame. */
typedef struct _PH_THREAD_STACK_FRAME
{
    PVOID PcAddress;
    PVOID ReturnAddress;
    PVOID FrameAddress;
    PVOID StackAddress;
    PVOID BStoreAddress;
    PVOID Params[4];
    USHORT Machine;
    USHORT Flags;
    ULONG InlineFrameContext;
} PH_THREAD_STACK_FRAME, *PPH_THREAD_STACK_FRAME;

#define PH_WALK_USER_STACK 0x1
#define PH_WALK_USER_WOW64_STACK 0x2
#define PH_WALK_KERNEL_STACK 0x10

/**
 * A callback function passed to PhWalkThreadStack() and called for each stack frame.
 *
 * \param StackFrame A structure providing information about the stack frame.
 * \param Context A user-defined value passed to PhWalkThreadStack().
 * \return TRUE to continue the stack walk, FALSE to stop.
 */
typedef _Function_class_(PH_WALK_THREAD_STACK_CALLBACK)
BOOLEAN NTAPI PH_WALK_THREAD_STACK_CALLBACK(
    _In_ PPH_THREAD_STACK_FRAME StackFrame,
    _In_opt_ PVOID Context
    );
typedef PH_WALK_THREAD_STACK_CALLBACK* PPH_WALK_THREAD_STACK_CALLBACK;

/**
 * Walks the stack of a thread and invokes a callback for each stack frame.
 *
 * This function performs a stack walk for the specified thread, optionally using symbol information
 * to resolve stack frames. For each frame encountered, the provided callback function is called.
 *
 * \param ThreadHandle Handle to the thread whose stack is to be walked.
 * \param ProcessHandle Optional handle to the process containing the thread. May be NULL.
 * \param ClientId Optional pointer to a CLIENT_ID structure identifying the thread. May be NULL.
 * \param SymbolProvider Optional pointer to a symbol provider for resolving symbols. May be NULL.
 * \param Flags Flags controlling the stack walk behavior (e.g., PH_WALK_USER_STACK, PH_WALK_KERNEL_STACK).
 * \param Callback Pointer to a callback function to be called for each stack frame.
 * \param Context Optional user-defined context value passed to the callback.
 * \return Returns STATUS_SUCCESS on success, or an appropriate NTSTATUS error code on failure.
 */
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

PHLIBAPI
PPH_STRING
NTAPI
PhUndecorateSymbolName(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PCWSTR DecoratedName
    );

typedef struct _PH_SYMBOL_INFO
{
    PH_STRINGREF Name;
    ULONG TypeIndex;   // Type Index of symbol
    ULONG Index;
    ULONG Size;
    ULONG64 ModBase;   // Base Address of module containing this symbol
    ULONG Flags;
    ULONG64 Value;     // Value of symbol, ValuePresent should be 1
    ULONG64 Address;   // Address of symbol including base address of module
    ULONG Register;    // register holding value or pointer to value
    ULONG Scope;       // scope of the symbol
    ULONG Tag;         // pdb classification
} PH_SYMBOL_INFO, *PPH_SYMBOL_INFO;

typedef BOOLEAN (NTAPI* PPH_ENUMERATE_SYMBOLS_CALLBACK)(
    _In_ PPH_SYMBOL_INFO SymbolInfo,
    _In_ ULONG SymbolSize,
    _In_opt_ PVOID UserContext
    );

PHLIBAPI
BOOLEAN
NTAPI
PhEnumerateSymbols(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseOfDll,
    _In_opt_ PCWSTR Mask,
    _In_ PPH_ENUMERATE_SYMBOLS_CALLBACK EnumSymbolsCallback,
    _In_opt_ PVOID UserContext
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetSymbolProviderDiaSource(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PVOID BaseOfDll,
    _Out_ PVOID* DiaSource
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetSymbolProviderDiaSession(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PVOID BaseOfDll,
    _Out_ PVOID* DiaSession
    );

PHLIBAPI
VOID
NTAPI
PhSymbolProviderFreeDiaString(
    _In_ PCWSTR DiaString
    );

// Inline stack support

typedef union _INLINE_FRAME_CONTEXT
{
    ULONG ContextValue;
    struct
    {
        UCHAR FrameId;
        UCHAR FrameType;
        USHORT FrameSignature;
    };
} INLINE_FRAME_CONTEXT, *PINLINE_FRAME_CONTEXT;

#define STACK_FRAME_TYPE_INIT 0x00
#define STACK_FRAME_TYPE_STACK 0x01
#define STACK_FRAME_TYPE_INLINE 0x02
#define STACK_FRAME_TYPE_RA 0x80 // Whether the instruction pointer is the current IP or a RA from callee frame.
#define STACK_FRAME_TYPE_IGNORE 0xFF

#ifndef INLINE_FRAME_CONTEXT_INIT
#define INLINE_FRAME_CONTEXT_INIT 0
#endif

#ifndef INLINE_FRAME_CONTEXT_IGNORE
#define INLINE_FRAME_CONTEXT_IGNORE 0xFFFFFFFF
#endif

FORCEINLINE
BOOLEAN
PhIsStackFrameTypeInline(
    _In_ ULONG InlineFrameContext
    )
{
    INLINE_FRAME_CONTEXT frameContext = { InlineFrameContext };

    if (frameContext.ContextValue == INLINE_FRAME_CONTEXT_IGNORE)
        return FALSE;

    if (frameContext.FrameType & STACK_FRAME_TYPE_INLINE)
        return TRUE;

    return FALSE;
}

BOOLEAN PhSymbolProviderInlineContextSupported(
    VOID
    );

_Success_(return != NULL)
PHLIBAPI
PPH_STRING
NTAPI
PhGetSymbolFromInlineContext(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PPH_THREAD_STACK_FRAME StackFrame,
    _Out_opt_ PPH_SYMBOL_RESOLVE_LEVEL ResolveLevel,
    _Out_opt_ PPH_STRING* FileName,
    _Out_opt_ PPH_STRING* SymbolName,
    _Out_opt_ PULONG64 Displacement,
    _Out_opt_ PPVOID BaseAddress
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhGetLineFromInlineContext(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PPH_THREAD_STACK_FRAME StackFrame,
    _In_opt_ PVOID BaseAddress,
    _Out_ PPH_STRING* FileName,
    _Out_opt_ PULONG Displacement,
    _Out_opt_ PPH_SYMBOL_LINE_INFORMATION Information
    );

//typedef struct _PH_INLINE_STACK_FRAME
//{
//    PH_SYMBOL_RESOLVE_LEVEL ResolveLevel;
//    PPH_STRING Symbol;
//    PPH_STRING FileName;
//
//    ULONG64 LineAddress;
//    ULONG LineDisplacement;
//    ULONG LineNumber;
//    PPH_STRING LineFileName;
//} PH_INLINE_STACK_FRAME, *PPH_INLINE_STACK_FRAME;
//
//PPH_LIST PhGetInlineStackSymbolsFromAddress(
//    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
//    _In_ PPH_THREAD_STACK_FRAME StackFrame,
//    _In_ BOOLEAN IncludeLineInformation
//    );
//
//VOID PhFreeInlineStackSymbols(
//    _In_ PPH_LIST InlineSymbolList
//    );

typedef struct _PH_DIA_SYMBOL_INFORMATION
{
    ULONG FunctionLength;
    PPH_STRING UndecoratedName;
    PPH_STRING SymbolInformation;
    PPH_STRING SymbolLangugage;
} PH_DIA_SYMBOL_INFORMATION, *PPH_DIA_SYMBOL_INFORMATION;

_Success_(return)
BOOLEAN PhGetDiaSymbolInformation(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PVOID Address,
    _Out_ PPH_DIA_SYMBOL_INFORMATION SymbolInformation
    );

PHLIBAPI
VOID
NTAPI
PhUnregisterSymbolProvider(
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider
    );

// symprv_std.cpp (dmex)

EXTERN_C VOID PhPrintCurrentStacktrace(
    VOID
    );

EXTERN_C PPH_STRING PhGetStacktraceAsString(
    VOID
    );

EXTERN_C PPH_STRING PhGetStacktraceSymbolFromAddress(
    _In_ PVOID Address
    );

EXTERN_C PPH_STRING PhGetObjectTypeStacktraceToString(
    _In_ PVOID Object
    );

EXTERN_C_END

#endif
