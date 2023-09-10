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

typedef enum _TRACE_CONTROL_INFORMATION_CLASS
{
    TraceControlStartLogger = 1, // inout WMI_LOGGER_INFORMATION
    TraceControlStopLogger = 2, // inout WMI_LOGGER_INFORMATION
    TraceControlQueryLogger = 3, // inout WMI_LOGGER_INFORMATION
    TraceControlUpdateLogger = 4, // inout WMI_LOGGER_INFORMATION
    TraceControlFlushLogger = 5, // inout WMI_LOGGER_INFORMATION
    TraceControlIncrementLoggerFile = 6, // inout WMI_LOGGER_INFORMATION
    TraceControlRealtimeTransition = 7, // inout WMI_LOGGER_INFORMATION
    // unused
    TraceControlRealtimeConnect = 11,
    TraceControlActivityIdCreate = 12,
    TraceControlWdiDispatchControl = 13,
    TraceControlRealtimeDisconnectConsumerByHandle = 14, // in HANDLE
    TraceControlRegisterGuidsCode = 15,
    TraceControlReceiveNotification = 16,
    TraceControlSendDataBlock = 17, // ETW_ENABLE_NOTIFICATION_PACKET
    TraceControlSendReplyDataBlock = 18,
    TraceControlReceiveReplyDataBlock = 19,
    TraceControlWdiUpdateSem = 20,
    TraceControlEnumTraceGuidList = 21, // out GUID[]
    TraceControlGetTraceGuidInfo = 22, // in GUID, out TRACE_GUID_INFO
    TraceControlEnumerateTraceGuids = 23,
    TraceControlRegisterSecurityProv = 24,
    TraceControlQueryReferenceTime = 25, // in ULONG, out ETW_REF_CLOCK
    TraceControlTrackProviderBinary = 26, // in HANDLE
    TraceControlAddNotificationEvent = 27,
    TraceControlUpdateDisallowList = 28,
    TraceControlSetEnableAllKeywordsCode = 29,
    TraceControlSetProviderTraitsCode = 30,
    TraceControlUseDescriptorTypeCode = 31,
    TraceControlEnumTraceGroupList = 32,
    TraceControlGetTraceGroupInfo = 33,
    TraceControlGetDisallowList = 34,
    TraceControlSetCompressionSettings = 35,
    TraceControlGetCompressionSettings = 36,
    TraceControlUpdatePeriodicCaptureState = 37,
    TraceControlGetPrivateSessionTraceHandle = 38,
    TraceControlRegisterPrivateSession = 39,
    TraceControlQuerySessionDemuxObject = 40,
    TraceControlSetProviderBinaryTracking = 41,
    TraceControlMaxLoggers = 42, // out ULONG
    TraceControlMaxPmcCounter = 43, // out ULONG
    TraceControlQueryUsedProcessorCount = 44, // ULONG // since WIN11
    TraceControlGetPmcOwnership = 45,
    TraceControlGetPmcSessions = 46,
} TRACE_CONTROL_INFORMATION_CLASS;

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtTraceControl(
    _In_ TRACE_CONTROL_INFORMATION_CLASS TraceInformationClass,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(TraceInformationLength) PVOID TraceInformation,
    _In_ ULONG TraceInformationLength,
    _Out_ PULONG ReturnLength
    );
#endif

#endif
