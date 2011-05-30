#ifndef _NTMISC_H
#define _NTMISC_H

// Boot graphics

#if (PHNT_VERSION >= PHNT_WIN7)
// rev
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDrawText(
    __in PUNICODE_STRING Text
    );
#endif

// Filter manager

#define FLT_PORT_CONNECT 0x0001
#define FLT_PORT_ALL_ACCESS (FLT_PORT_CONNECT | STANDARD_RIGHTS_ALL)

// NLS

// begin_private

#if (PHNT_VERSION >= PHNT_VISTA)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtInitializeNlsFiles(
    __out PVOID *BaseAddress,
    __out PLCID DefaultLocaleId,
    __out PLARGE_INTEGER DefaultCasingTableSize,
    __out_opt PULONG CurrentNLSVersion
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetNlsSectionPtr(
    __in ULONG SectionType,
    __in ULONG SectionData,
    __in PVOID ContextData,
    __out PVOID *SectionPointer,
    __out PULONG SectionSize
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAcquireCMFViewOwnership(
    __out PULONGLONG TimeStamp,
    __out PBOOLEAN tokenTaken,
    __in BOOLEAN replaceExisting
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReleaseCMFViewOwnership(
    VOID
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtMapCMFModule(
    __in ULONG What,
    __in ULONG Index,
    __out_opt PULONG CacheIndexOut,
    __out_opt PULONG CacheFlagsOut,
    __out_opt PULONG ViewSizeOut,
    __out_opt PVOID *BaseAddress
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetMUIRegistryInfo(
    __in ULONG Flags,
    __inout PULONG DataSize,
    __out PVOID Data
    );

#endif

// end_private

// VDM

typedef enum _VDMSERVICECLASS
{
    VdmStartExecution,
    VdmQueueInterrupt,
    VdmDelayInterrupt,
    VdmInitialize,
    VdmFeatures,
    VdmSetInt21Handler,
    VdmQueryDir,
    VdmPrinterDirectIoOpen,
    VdmPrinterDirectIoClose,
    VdmPrinterInitialize,
    VdmSetLdtEntries,
    VdmSetProcessLdtInfo,
    VdmAdlibEmulation,
    VdmPMCliControl,
    VdmQueryVdmProcess
} VDMSERVICECLASS, *PVDMSERVICECLASS;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtVdmControl(
    __in VDMSERVICECLASS Service,
    __inout PVOID ServiceData
    );

// WMI/ETW

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTraceEvent(
    __in HANDLE TraceHandle,
    __in ULONG Flags,
    __in ULONG FieldSize,
    __in PVOID Fields
    );

#if (PHNT_VERSION >= PHNT_VISTA)
// private
NTSYSCALLAPI
NTSTATUS
NTAPI
NtTraceControl(
    __in ULONG FunctionCode,
    __in_bcount_opt(InBufferLen) PVOID InBuffer,
    __in ULONG InBufferLen,
    __out_bcount_opt(OutBufferLen) PVOID OutBuffer,
    __in ULONG OutBufferLen,
    __out PULONG ReturnLength
    );
#endif

#endif
