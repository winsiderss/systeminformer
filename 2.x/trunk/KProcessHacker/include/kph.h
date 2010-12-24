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

#endif
