#ifndef NTDBG_H
#define NTDBG_H

// This header file provides debugging definitions.
// Source: NT headers

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

#endif
