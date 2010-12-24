#ifndef _PH_KPHUSER_H
#define _PH_KPHUSER_H

#include <kphapi.h>

PHLIBAPI
NTSTATUS KphConnect(
    __out PHANDLE KphHandle,
    __in_opt PWSTR DeviceName
    );

PHLIBAPI
NTSTATUS KphConnect2(
    __out PHANDLE KphHandle,
    __in_opt PWSTR DeviceName,
    __in PWSTR FileName
    );

PHLIBAPI
NTSTATUS KphDisconnect(
    __in HANDLE KphHandle
    );

PHLIBAPI
NTSTATUS KphInstall(
    __in_opt PWSTR DeviceName,
    __in PWSTR FileName
    );

PHLIBAPI
NTSTATUS KphUninstall(
    __in_opt PWSTR DeviceName
    );

PHLIBAPI
NTSTATUS KphGetFeatures(
    __in HANDLE KphHandle,
    __out PULONG Features
    );

PHLIBAPI
NTSTATUS KphOpenProcess(
    __in HANDLE KphHandle,
    __out PHANDLE ProcessHandle,
    __in HANDLE ProcessId,
    __in ACCESS_MASK DesiredAccess
    );

PHLIBAPI
NTSTATUS KphOpenProcessToken(
    __in HANDLE KphHandle,
    __out PHANDLE TokenHandle,
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess
    );

PHLIBAPI
NTSTATUS KphOpenProcessJob(
    __in HANDLE KphHandle,
    __out PHANDLE JobHandle,
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess
    );

PHLIBAPI
NTSTATUS KphSuspendProcess(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle
    );

PHLIBAPI
NTSTATUS KphResumeProcess(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle
    );

PHLIBAPI
NTSTATUS KphTerminateProcess(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in NTSTATUS ExitStatus
    );

PHLIBAPI
NTSTATUS KphReadVirtualMemory(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __out_bcount(BufferLength) PVOID Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS KphWriteVirtualMemory(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __in_bcount(BufferLength) PVOID Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS KphReadVirtualMemoryUnsafe(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __in_bcount(BufferLength) PVOID Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS KphGetProcessProtected(
    __in HANDLE KphHandle,
    __in HANDLE ProcessId,
    __out PBOOLEAN IsProtected
    );

PHLIBAPI
NTSTATUS KphSetProcessProtected(
    __in HANDLE KphHandle,
    __in HANDLE ProcessId,
    __in BOOLEAN IsProtected
    );

PHLIBAPI
NTSTATUS KphSetExecuteOptions(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in ULONG ExecuteOptions
    );

PHLIBAPI
NTSTATUS KphSetProcessToken(
    __in HANDLE KphHandle,
    __in HANDLE SourceProcessId,
    __in HANDLE TargetProcessId
    );

PHLIBAPI
NTSTATUS KphQueryInformationProcess(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in PROCESS_INFORMATION_CLASS ProcessInformationClass,
    __out_bcount_opt(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __out_opt PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS KphQueryInformationThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in THREAD_INFORMATION_CLASS ThreadInformationClass,
    __out_bcount_opt(ThreadInformationLength) PVOID ThreadInformation,
    __in ULONG ThreadInformationLength,
    __out_opt PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS KphSetInformationProcess(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in PROCESS_INFORMATION_CLASS ProcessInformationClass,
    __in_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength
    );

PHLIBAPI
NTSTATUS KphSetInformationThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in THREAD_INFORMATION_CLASS ThreadInformationClass,
    __in_bcount(ThreadInformationLength) PVOID ThreadInformation,
    __in ULONG ThreadInformationLength
    );

PHLIBAPI
NTSTATUS KphOpenThread(
    __in HANDLE KphHandle,
    __out PHANDLE ThreadHandle,
    __in HANDLE ThreadId,
    __in ACCESS_MASK DesiredAccess
    );

PHLIBAPI
NTSTATUS KphOpenThreadProcess(
    __in HANDLE KphHandle,
    __out PHANDLE ProcessHandle,
    __in HANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess
    );

PHLIBAPI
NTSTATUS KphTerminateThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus
    );

PHLIBAPI
NTSTATUS KphTerminateThreadUnsafe(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus
    );

PHLIBAPI
NTSTATUS KphGetContextThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __inout PCONTEXT ThreadContext
    );

PHLIBAPI
NTSTATUS KphSetContextThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in PCONTEXT ThreadContext
    );

PHLIBAPI
NTSTATUS KphCaptureStackBackTraceThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in ULONG FramesToSkip,
    __in ULONG FramesToCapture,
    __out_ecount(FramesToCapture) PPVOID BackTrace,
    __out_opt PULONG CapturedFrames,
    __out_opt PULONG BackTraceHash
    );

PHLIBAPI
NTSTATUS KphGetThreadWin32Thread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __out PPVOID Win32Thread
    );

PHLIBAPI
NTSTATUS KphAssignImpersonationToken(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in HANDLE TokenHandle
    );

PHLIBAPI
NTSTATUS KphQueryProcessHandles(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __out_bcount_opt(BufferLength) PVOID Buffer,
    __in_opt ULONG BufferLength,
    __out_opt PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS KphGetHandleObjectName(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __out_bcount_opt(BufferLength) PUNICODE_STRING Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS KphZwQueryObject(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __in OBJECT_INFORMATION_CLASS ObjectInformationClass,
    __in_bcount_opt(BufferLength) PVOID Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS KphDuplicateObject(
    __in HANDLE KphHandle,
    __in HANDLE SourceProcessHandle,
    __in HANDLE SourceHandle,
    __in_opt HANDLE TargetProcessHandle,
    __out_opt PHANDLE TargetHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG HandleAttributes,
    __in ULONG Options
    );

PHLIBAPI
NTSTATUS KphSetHandleAttributes(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __in ULONG Flags
    );

PHLIBAPI
NTSTATUS KphSetHandleGrantedAccess(
    __in HANDLE KphHandle,
    __in HANDLE Handle,
    __in ACCESS_MASK GrantedAccess
    );

PHLIBAPI
NTSTATUS KphGetProcessId(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __out PHANDLE ProcessId
    );

PHLIBAPI
NTSTATUS KphGetThreadId(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __out PHANDLE ThreadId,
    __out_opt PHANDLE ProcessId
    );

PHLIBAPI
NTSTATUS KphOpenNamedObject(
    __in HANDLE KphHandle,
    __out PHANDLE Handle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

PHLIBAPI
NTSTATUS KphOpenDirectoryObject(
    __in HANDLE KphHandle,
    __out PHANDLE DirectoryHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

PHLIBAPI
NTSTATUS KphOpenDriver(
    __in HANDLE KphHandle,
    __out PHANDLE DriverHandle,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

PHLIBAPI
NTSTATUS KphQueryInformationDriver(
    __in HANDLE KphHandle,
    __in HANDLE DriverHandle,
    __in DRIVER_INFORMATION_CLASS DriverInformationClass,
    __out_bcount_opt(DriverInformationLength) PVOID DriverInformation,
    __in ULONG DriverInformationLength,
    __out_opt PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS KphOpenType(
    __in HANDLE KphHandle,
    __out PHANDLE TypeHandle,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

PHLIBAPI
NTSTATUS KphQueryInformationEtwReg(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in HANDLE EtwRegHandle,
    __in ETWREG_INFORMATION_CLASS EtwRegInformationClass,
    __out_bcount_opt(EtwRegInformationLength) PVOID EtwRegInformation,
    __in ULONG EtwRegInformationLength,
    __out_opt PULONG ReturnLength
    );

#endif
