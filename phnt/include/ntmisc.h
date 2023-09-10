/*
 * Trace Control support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTMISC_H
#define _NTMISC_H

// Filter manager

#define FLT_PORT_CONNECT 0x0001
#define FLT_PORT_ALL_ACCESS (FLT_PORT_CONNECT | STANDARD_RIGHTS_ALL)

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
    VdmQueryVdmProcess,
    VdmPreInitialize
} VDMSERVICECLASS, *PVDMSERVICECLASS;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtVdmControl(
    _In_ VDMSERVICECLASS Service,
    _Inout_ PVOID ServiceData
    );

// WMI/ETW

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTraceEvent(
    _In_ HANDLE TraceHandle,
    _In_ ULONG Flags,
    _In_ ULONG FieldSize,
    _In_ PVOID Fields
    );

// rev
typedef enum _ETWTRACECONTROLCODE
{
    EtwStartLoggerCode = 1, // inout WMI_LOGGER_INFORMATION
    EtwStopLoggerCode = 2, // inout WMI_LOGGER_INFORMATION
    EtwQueryLoggerCode = 3, // inout WMI_LOGGER_INFORMATION
    EtwUpdateLoggerCode = 4, // inout WMI_LOGGER_INFORMATION
    EtwFlushLoggerCode = 5, // inout WMI_LOGGER_INFORMATION
    EtwIncrementLoggerFile = 6, // inout WMI_LOGGER_INFORMATION
    EtwRealtimeTransition = 7, // inout WMI_LOGGER_INFORMATION
    // reserved
    EtwRealtimeConnectCode = 11,
    EtwActivityIdCreate = 12,
    EtwWdiScenarioCode = 13,
    EtwRealtimeDisconnectCode = 14, // in HANDLE
    EtwRegisterGuidsCode = 15,
    EtwReceiveNotification = 16,
    EtwSendDataBlock = 17, // ETW_ENABLE_NOTIFICATION_PACKET
    EtwSendReplyDataBlock = 18,
    EtwReceiveReplyDataBlock = 19,
    EtwWdiSemUpdate = 20,
    EtwEnumTraceGuidList = 21, // out GUID[]
    EtwGetTraceGuidInfo = 22, // in GUID, out TRACE_GUID_INFO
    EtwEnumerateTraceGuids = 23,
    EtwRegisterSecurityProv = 24,
    EtwReferenceTimeCode = 25, // in ULONG, out ETW_REF_CLOCK
    EtwTrackBinaryCode = 26, // in HANDLE
    EtwAddNotificationEvent = 27,
    EtwUpdateDisallowList = 28,
    EtwSetEnableAllKeywordsCode = 29,
    EtwSetProviderTraitsCode = 30,
    EtwUseDescriptorTypeCode = 31,
    EtwEnumTraceGroupList = 32,
    EtwGetTraceGroupInfo = 33,
    EtwGetDisallowList = 34,
    EtwSetCompressionSettings = 35,
    EtwGetCompressionSettings = 36,
    EtwUpdatePeriodicCaptureState = 37,
    EtwGetPrivateSessionTraceHandle = 38,
    EtwRegisterPrivateSession = 39,
    EtwQuerySessionDemuxObject = 40,
    EtwSetProviderBinaryTracking = 41,
    EtwMaxLoggers = 42, // out ULONG
    EtwMaxPmcCounter = 43, // out ULONG
    EtwQueryUsedProcessorCount = 44, // ULONG // since WIN11
    EtwGetPmcOwnership = 45,
    EtwGetPmcSessions = 46,
} ETWTRACECONTROLCODE;

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtTraceControl(
    _In_ ETWTRACECONTROLCODE TraceControlCode,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG ReturnLength
    );
#endif

#endif
