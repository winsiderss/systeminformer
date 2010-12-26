#ifndef _PH_KPHUSER_H
#define _PH_KPHUSER_H

#include <kphapi.h>

typedef struct _KPH_PARAMETERS
{
    KPH_SECURITY_LEVEL SecurityLevel;
} KPH_PARAMETERS, *PKPH_PARAMETERS;

PHLIBAPI
NTSTATUS
NTAPI
KphConnect(
    __out PHANDLE KphHandle,
    __in_opt PWSTR DeviceName
    );

PHLIBAPI
NTSTATUS
NTAPI
KphConnect2(
    __out PHANDLE KphHandle,
    __in_opt PWSTR DeviceName,
    __in PWSTR FileName
    );

PHLIBAPI
NTSTATUS
NTAPI
KphConnect2Ex(
    __out PHANDLE KphHandle,
    __in_opt PWSTR DeviceName,
    __in PWSTR FileName,
    __in_opt PKPH_PARAMETERS Parameters
    );

PHLIBAPI
NTSTATUS
NTAPI
KphDisconnect(
    __in HANDLE KphHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
KphSetParameters(
    __in_opt PWSTR DeviceName,
    __in PKPH_PARAMETERS Parameters
    );

PHLIBAPI
NTSTATUS
NTAPI
KphInstall(
    __in_opt PWSTR DeviceName,
    __in PWSTR FileName
    );

PHLIBAPI
NTSTATUS
NTAPI
KphInstallEx(
    __in_opt PWSTR DeviceName,
    __in PWSTR FileName,
    __in_opt PKPH_PARAMETERS Parameters
    );

PHLIBAPI
NTSTATUS
NTAPI
KphUninstall(
    __in_opt PWSTR DeviceName
    );

PHLIBAPI
NTSTATUS
NTAPI
KphGetFeatures(
    __in HANDLE KphHandle,
    __out PULONG Features
    );

PHLIBAPI
NTSTATUS
NTAPI
KphOpenProcess(
    __in HANDLE KphHandle,
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in PCLIENT_ID ClientId
    );

PHLIBAPI
NTSTATUS
NTAPI
KphOpenProcessToken(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __out PHANDLE TokenHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
KphOpenProcessJob(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __out PHANDLE JobHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
KphSuspendProcess(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
KphResumeProcess(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
KphTerminateProcess(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in NTSTATUS ExitStatus
    );

PHLIBAPI
NTSTATUS
NTAPI
KphReadVirtualMemory(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __out_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesRead
    );

PHLIBAPI
NTSTATUS
NTAPI
KphWriteVirtualMemory(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in_opt PVOID BaseAddress,
    __in_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesWritten
    );

PHLIBAPI
NTSTATUS
NTAPI
KphReadVirtualMemoryUnsafe(
    __in HANDLE KphHandle,
    __in_opt HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __out_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesRead
    );

PHLIBAPI
NTSTATUS
NTAPI
KphQueryInformationProcess(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    __out_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __out_opt PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphSetInformationProcess(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    __in_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphOpenThread(
    __in HANDLE KphHandle,
    __out PHANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in PCLIENT_ID ClientId
    );

PHLIBAPI
NTSTATUS
NTAPI
KphOpenThreadProcess(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __out PHANDLE ProcessHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
KphTerminateThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus
    );

PHLIBAPI
NTSTATUS
NTAPI
KphTerminateThreadUnsafe(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus
    );

PHLIBAPI
NTSTATUS
NTAPI
KphGetContextThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __inout PCONTEXT ThreadContext
    );

PHLIBAPI
NTSTATUS
NTAPI
KphSetContextThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in PCONTEXT ThreadContext
    );

PHLIBAPI
NTSTATUS
NTAPI
KphCaptureStackBackTraceThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in ULONG FramesToSkip,
    __in ULONG FramesToCapture,
    __out_ecount(FramesToCapture) PVOID *BackTrace,
    __out_opt PULONG CapturedFrames,
    __out_opt PULONG BackTraceHash
    );

PHLIBAPI
NTSTATUS
NTAPI
KphQueryInformationThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in KPH_THREAD_INFORMATION_CLASS ThreadInformationClass,
    __out_bcount(ProcessInformationLength) PVOID ThreadInformation,
    __in ULONG ThreadInformationLength,
    __out_opt PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphSetInformationThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in KPH_THREAD_INFORMATION_CLASS ThreadInformationClass,
    __in_bcount(ThreadInformationLength) PVOID ThreadInformation,
    __in ULONG ThreadInformationLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphEnumerateProcessHandles(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __out_bcount(BufferLength) PVOID Buffer,
    __in_opt ULONG BufferLength,
    __out_opt PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphQueryInformationObject(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __in KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass,
    __out_bcount(ObjectInformationLength) PVOID ObjectInformation,
    __in ULONG ObjectInformationLength,
    __out_opt PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphSetInformationObject(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __in KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass,
    __in_bcount(ObjectInformationLength) PVOID ObjectInformation,
    __in ULONG ObjectInformationLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphDuplicateObject(
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
NTSTATUS
NTAPI
KphOpenDriver(
    __in HANDLE KphHandle,
    __out PHANDLE DriverHandle,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

PHLIBAPI
NTSTATUS
NTAPI
KphQueryInformationDriver(
    __in HANDLE KphHandle,
    __in HANDLE DriverHandle,
    __in DRIVER_INFORMATION_CLASS DriverInformationClass,
    __out_bcount(DriverInformationLength) PVOID DriverInformation,
    __in ULONG DriverInformationLength,
    __out_opt PULONG ReturnLength
    );

#endif
