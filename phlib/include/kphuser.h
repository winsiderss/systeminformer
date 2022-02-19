#ifndef _PH_KPHUSER_H
#define _PH_KPHUSER_H

#include <kphapi.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _KPH_PARAMETERS
{
    KPH_SECURITY_LEVEL SecurityLevel; // deprecated 1-2-21, to be removed
    BOOLEAN CreateDynamicConfiguration;
} KPH_PARAMETERS, *PKPH_PARAMETERS;

PHLIBAPI
NTSTATUS
NTAPI
KphConnect(
    _In_opt_ PWSTR DeviceName
    );

PHLIBAPI
NTSTATUS
NTAPI
KphConnect2Ex(
    _In_opt_ PWSTR ServiceName,
    _In_opt_ PWSTR DeviceName,
    _In_ PWSTR FileName,
    _In_opt_ PKPH_PARAMETERS Parameters
    );

PHLIBAPI
NTSTATUS
NTAPI
KphDisconnect(
    VOID
    );

PHLIBAPI
BOOLEAN
NTAPI
KphIsConnected(
    VOID
    );

PHLIBAPI
BOOLEAN
NTAPI
KphIsVerified(
    VOID
    );

PHLIBAPI
NTSTATUS
NTAPI
KphSetParameters(
    _In_opt_ PWSTR ServiceName,
    _In_ PKPH_PARAMETERS Parameters
    );

PHLIBAPI
BOOLEAN
NTAPI
KphParametersExists(
    _In_opt_ PWSTR ServiceName
    );

PHLIBAPI
NTSTATUS
NTAPI
KphResetParameters(
    _In_opt_ PWSTR ServiceName
    );

PHLIBAPI
VOID
NTAPI
KphSetServiceSecurity(
    _In_ SC_HANDLE ServiceHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
KphInstall(
    _In_opt_ PWSTR ServiceName,
    _In_ PWSTR FileName
    );

PHLIBAPI
NTSTATUS
NTAPI
KphInstallEx(
    _In_opt_ PWSTR ServiceName,
    _In_ PWSTR FileName,
    _In_opt_ PKPH_PARAMETERS Parameters
    );

PHLIBAPI
NTSTATUS
NTAPI
KphUninstall(
    _In_opt_ PWSTR ServiceName
    );

PHLIBAPI
NTSTATUS
NTAPI
KphGetFeatures(
    _Inout_ PULONG Features
    );

PHLIBAPI
NTSTATUS
NTAPI
KphVerifyClient(
    _In_reads_bytes_(SignatureSize) PUCHAR Signature,
    _In_ ULONG SignatureSize
    );

PHLIBAPI
NTSTATUS
NTAPI
KphOpenProcess(
    _Inout_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCLIENT_ID ClientId
    );

PHLIBAPI
NTSTATUS
NTAPI
KphOpenProcessToken(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Inout_ PHANDLE TokenHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
KphOpenProcessJob(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Inout_ PHANDLE JobHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
KphTerminateProcess(
    _In_ HANDLE ProcessHandle,
    _In_ NTSTATUS ExitStatus
    );

PHLIBAPI
NTSTATUS
NTAPI
KphReadVirtualMemoryUnsafe(
    _In_opt_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Inout_opt_ PSIZE_T NumberOfBytesRead
    );

PHLIBAPI
NTSTATUS
NTAPI
KphQueryInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    _Out_writes_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Inout_opt_ PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphSetInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    _In_reads_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphOpenThread(
    _Inout_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCLIENT_ID ClientId
    );

PHLIBAPI
NTSTATUS
NTAPI
KphOpenThreadProcess(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Inout_ PHANDLE ProcessHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
KphCaptureStackBackTraceThread(
    _In_ HANDLE ThreadHandle,
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _Out_writes_(FramesToCapture) PVOID *BackTrace,
    _Inout_opt_ PULONG CapturedFrames,
    _Inout_opt_ PULONG BackTraceHash
    );

PHLIBAPI
NTSTATUS
NTAPI
KphQueryInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ KPH_THREAD_INFORMATION_CLASS ThreadInformationClass,
    _Out_writes_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _Inout_opt_ PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphSetInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ KPH_THREAD_INFORMATION_CLASS ThreadInformationClass,
    _In_reads_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphEnumerateProcessHandles(
    _In_ HANDLE ProcessHandle,
    _Out_writes_bytes_(BufferLength) PVOID Buffer,
    _In_opt_ ULONG BufferLength,
    _Inout_opt_ PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphEnumerateProcessHandles2(
    _In_ HANDLE ProcessHandle,
    _Out_ PKPH_PROCESS_HANDLE_INFORMATION *Handles
    );

PHLIBAPI
NTSTATUS
NTAPI
KphQueryInformationObject(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _Out_writes_bytes_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength,
    _Inout_opt_ PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphSetInformationObject(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _In_reads_bytes_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphOpenDriver(
    _Inout_ PHANDLE DriverHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    );

PHLIBAPI
NTSTATUS
NTAPI
KphQueryInformationDriver(
    _In_ HANDLE DriverHandle,
    _In_ DRIVER_INFORMATION_CLASS DriverInformationClass,
    _Out_writes_bytes_(DriverInformationLength) PVOID DriverInformation,
    _In_ ULONG DriverInformationLength,
    _Inout_opt_ PULONG ReturnLength
    );

// kphdata

PHLIBAPI
NTSTATUS
NTAPI
KphInitializeDynamicPackage(
    _Out_ PKPH_DYN_PACKAGE Package
    );

#ifdef __cplusplus
}
#endif

#endif
