#ifndef _NTDBG_H
#define _NTDBG_H

// Definitions

typedef struct _DBGKM_EXCEPTION
{
    EXCEPTION_RECORD ExceptionRecord;
    ULONG FirstChance;
} DBGKM_EXCEPTION, *PDBGKM_EXCEPTION;

typedef struct _DBGKM_CREATE_THREAD
{
    ULONG SubSystemKey;
    PVOID StartAddress;
} DBGKM_CREATE_THREAD, *PDBGKM_CREATE_THREAD;

typedef struct _DBGKM_CREATE_PROCESS
{
    ULONG SubSystemKey;
    HANDLE FileHandle;
    PVOID BaseOfImage;
    ULONG DebugInfoFileOffset;
    ULONG DebugInfoSize;
    DBGKM_CREATE_THREAD InitialThread;
} DBGKM_CREATE_PROCESS, *PDBGKM_CREATE_PROCESS;

typedef struct _DBGKM_EXIT_THREAD
{
    NTSTATUS ExitStatus;
} DBGKM_EXIT_THREAD, *PDBGKM_EXIT_THREAD;

typedef struct _DBGKM_EXIT_PROCESS
{
    NTSTATUS ExitStatus;
} DBGKM_EXIT_PROCESS, *PDBGKM_EXIT_PROCESS;

typedef struct _DBGKM_LOAD_DLL
{
    HANDLE FileHandle;
    PVOID BaseOfDll;
    ULONG DebugInfoFileOffset;
    ULONG DebugInfoSize;
    PVOID NamePointer;
} DBGKM_LOAD_DLL, *PDBGKM_LOAD_DLL;

typedef struct _DBGKM_UNLOAD_DLL
{
    PVOID BaseAddress;
} DBGKM_UNLOAD_DLL, *PDBGKM_UNLOAD_DLL;

typedef enum _DBG_STATE
{
    DbgIdle,
    DbgReplyPending,
    DbgCreateThreadStateChange,
    DbgCreateProcessStateChange,
    DbgExitThreadStateChange,
    DbgExitProcessStateChange,
    DbgExceptionStateChange,
    DbgBreakpointStateChange,
    DbgSingleStepStateChange,
    DbgLoadDllStateChange,
    DbgUnloadDllStateChange
} DBG_STATE, *PDBG_STATE;

typedef struct _DBGUI_CREATE_THREAD
{
    HANDLE HandleToThread;
    DBGKM_CREATE_THREAD NewThread;
} DBGUI_CREATE_THREAD, *PDBGUI_CREATE_THREAD;

typedef struct _DBGUI_CREATE_PROCESS
{
    HANDLE HandleToProcess;
    HANDLE HandleToThread;
    DBGKM_CREATE_PROCESS NewProcess;
} DBGUI_CREATE_PROCESS, *PDBGUI_CREATE_PROCESS;

typedef struct _DBGUI_WAIT_STATE_CHANGE
{
    DBG_STATE NewState;
    CLIENT_ID AppClientId;
    union
    {
        DBGKM_EXCEPTION Exception;
        DBGUI_CREATE_THREAD CreateThread;
        DBGUI_CREATE_PROCESS CreateProcessInfo;
        DBGKM_EXIT_THREAD ExitThread;
        DBGKM_EXIT_PROCESS ExitProcess;
        DBGKM_LOAD_DLL LoadDll;
        DBGKM_UNLOAD_DLL UnloadDll;
    } StateInfo;
} DBGUI_WAIT_STATE_CHANGE, *PDBGUI_WAIT_STATE_CHANGE;

// System calls

#define DEBUG_READ_EVENT 0x0001
#define DEBUG_PROCESS_ASSIGN 0x0002
#define DEBUG_SET_INFORMATION 0x0004
#define DEBUG_QUERY_INFORMATION 0x0008
#define DEBUG_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | \
    DEBUG_READ_EVENT | DEBUG_PROCESS_ASSIGN | DEBUG_SET_INFORMATION | \
    DEBUG_QUERY_INFORMATION)

#define DEBUG_KILL_ON_CLOSE 0x1

typedef enum _DEBUGOBJECTINFOCLASS
{
    DebugObjectFlags = 1,
    MaxDebugObjectInfoClass
} DEBUGOBJECTINFOCLASS, *PDEBUGOBJECTINFOCLASS;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateDebugObject(
    __out PHANDLE DebugObjectHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in ULONG Flags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDebugActiveProcess(
    __in HANDLE ProcessHandle,
    __in HANDLE DebugObjectHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDebugContinue(
    __in HANDLE DebugObjectHandle,
    __in PCLIENT_ID ClientId,
    __in NTSTATUS ContinueStatus
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRemoveProcessDebug(
    __in HANDLE ProcessHandle,
    __in HANDLE DebugObjectHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationDebugObject(
    __in HANDLE DebugObjectHandle,
    __in DEBUGOBJECTINFOCLASS DebugObjectInformationClass,
    __in PVOID DebugInformation,
    __in ULONG DebugInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForDebugEvent(
    __in HANDLE DebugObjectHandle,
    __in BOOLEAN Alertable,
    __in_opt PLARGE_INTEGER Timeout,
    __out PVOID WaitStateChange
    );

// Debugging UI

NTSYSAPI
NTSTATUS
NTAPI
DbgUiConnectToDbg(
    VOID
    );

NTSYSAPI
HANDLE
NTAPI
DbgUiGetThreadDebugObject(
    VOID
    );

NTSYSAPI
VOID
NTAPI
DbgUiSetThreadDebugObject(
    __in HANDLE DebugObject
    );

NTSYSAPI
NTSTATUS
NTAPI
DbgUiWaitStateChange(
    __out PDBGUI_WAIT_STATE_CHANGE StateChange,
    __in_opt PLARGE_INTEGER Timeout
    );

NTSYSAPI
NTSTATUS
NTAPI
DbgUiContinue(
    __in PCLIENT_ID AppClientId,
    __in NTSTATUS ContinueStatus
    );

NTSYSAPI
NTSTATUS
NTAPI
DbgUiStopDebugging(
    __in HANDLE Process
    );

NTSYSAPI
NTSTATUS
NTAPI
DbgUiDebugActiveProcess(
    __in HANDLE Process
    );

NTSYSAPI
VOID
NTAPI
DbgUiRemoteBreakin(
    __in PVOID Context
    );

NTSYSAPI
NTSTATUS
NTAPI
DbgUiIssueRemoteBreakin(
    __in HANDLE Process
    );

struct _DEBUG_EVENT;

NTSYSAPI
NTSTATUS
NTAPI
DbgUiConvertStateChangeStructure(
    __in PDBGUI_WAIT_STATE_CHANGE StateChange,
    __out struct _DEBUG_EVENT *DebugEvent
    );

#endif
