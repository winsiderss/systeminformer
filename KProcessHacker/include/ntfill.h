#ifndef NTFILL_H
#define NTFILL_H

// Process manager

typedef NTSTATUS (NTAPI *_PsSuspendProcess)(
    __in PEPROCESS Process
    );

typedef NTSTATUS (NTAPI *_PsResumeProcess)(
    __in PEPROCESS Process
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationProcess(
    __in HANDLE ProcessHandle,
    __in PROCESSINFOCLASS ProcessInformationClass,
    __out_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwSetInformationProcess(
    __in HANDLE ProcessHandle,
    __in PROCESSINFOCLASS ProcessInformationClass,
    __in_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationThread(
    __in HANDLE ThreadHandle,
    __in THREADINFOCLASS ThreadInformationClass,
    __out_bcount(ThreadInformationLength) PVOID ThreadInformation,
    __in ULONG ThreadInformationLength,
    __out_opt PULONG ReturnLength
    );

NTKERNELAPI
NTSTATUS
NTAPI
PsLookupProcessThreadByCid(
    __in PCLIENT_ID ClientId,
    __out_opt PEPROCESS *Process,
    __out PETHREAD *Thread
    );

typedef struct _EJOB *PEJOB;

extern POBJECT_TYPE *PsJobType;

NTKERNELAPI
PEJOB
NTAPI
PsGetProcessJob(
    __in PEPROCESS Process
    );

#endif
