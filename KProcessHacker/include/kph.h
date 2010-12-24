#ifndef KPH_H
#define KPH_H

#include <ntifs.h>
#define PHNT_MODE PHNT_MODE_KERNEL
#include <phnt.h>
#include <ntfill.h>
#include <kphapi.h>

// Debugging

#ifdef DBG
#define dprintf(Format, ...) DbgPrint("KProcessHacker: " Format, __VA_ARGS__)
#else
#define dprintf
#endif

typedef struct _KPH_PARAMETERS
{
    KPH_SECURITY_LEVEL SecurityLevel;
} KPH_PARAMETERS, *PKPH_PARAMETERS;

// main

extern ULONG KphFeatures;
extern KPH_PARAMETERS KphParameters;

NTSTATUS KpiGetFeatures(
    __out PULONG Features,
    __in KPROCESSOR_MODE AccessMode
    );

// devctrl

NTSTATUS KphDispatchDeviceControl(
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP Irp
    );

// dynimp

extern _PsSuspendProcess PsSuspendProcess_I;
extern _PsResumeProcess PsResumeProcess_I;

VOID KphDynamicImport(
    VOID
    );

// process

NTSTATUS KpiOpenProcess(
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in PCLIENT_ID ClientId,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiOpenProcessToken(
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __out PHANDLE TokenHandle,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiOpenProcessJob(
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __out PHANDLE JobHandle,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiSuspendProcess(
    __in HANDLE ProcessHandle,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiResumeProcess(
    __in HANDLE ProcessHandle,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiTerminateProcess(
    __in HANDLE ProcessHandle,
    __in NTSTATUS ExitStatus,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiQueryInformationProcess(
    __in HANDLE ProcessHandle,
    __in KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    __out_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __out_opt PULONG ReturnLength,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiSetInformationProcess(
    __in HANDLE ProcessHandle,
    __in KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    __in_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __in KPROCESSOR_MODE AccessMode
    );

// thread

NTSTATUS KpiOpenThread(
    __out PHANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in PCLIENT_ID ClientId,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiOpenThreadProcess(
    __in HANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __out PHANDLE ProcessHandle,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiTerminateThread(
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiTerminateThreadUnsafe(
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus,
    __in KPROCESSOR_MODE AccessMode
    );

// vm

NTSTATUS KphCopyVirtualMemory(
    __in PEPROCESS FromProcess,
    __in PVOID FromAddress,
    __in PEPROCESS ToProcess,
    __in PVOID ToAddress,
    __in SIZE_T BufferLength,
    __in KPROCESSOR_MODE AccessMode,
    __out PSIZE_T ReturnLength
    );

NTSTATUS KpiReadVirtualMemory(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __out_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesRead,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiWriteVirtualMemory(
    __in HANDLE ProcessHandle,
    __in_opt PVOID BaseAddress,
    __in_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesWritten,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiReadVirtualMemoryUnsafe(
    __in_opt HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __out_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesRead,
    __in KPROCESSOR_MODE AccessMode
    );

#endif
