/*
 * Process Hacker -
 *   Trace Control support functions
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
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
    VdmQueryVdmProcess
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
    TraceControlStartLogger = 1,
    TraceControlStopLogger = 2,
    TraceControlQueryLogger = 3,
    TraceControlUpdateLogger = 4,
    TraceControlFlushLogger = 5,
    TraceControlIncrementLoggerFile = 6,

    TraceControlRealtimeConnect = 11,
    TraceControlWdiDispatchControl = 13,
    TraceControlRealtimeDisconnectConsumerByHandle = 14,

    TraceControlReceiveNotification = 16,
    TraceControlEnableGuid = 17,
    TraceControlSendReplyDataBlock = 18,
    TraceControlReceiveReplyDataBlock = 19,
    TraceControlWdiUpdateSem = 20,
    TraceControlGetTraceGuidList = 21,
    TraceControlGetTraceGuidInfo = 22,
    TraceControlEnumerateTraceGuids = 23,

    TraceControlQueryReferenceTime = 25,
    TraceControlTrackProviderBinary = 26,
    TraceControlAddNotificationEvent = 27,
    TraceControlUpdateDisallowList = 28,

    TraceControlUseDescriptorTypeUm = 31,
    TraceControlGetTraceGroupList = 32,
    TraceControlGetTraceGroupInfo = 33,
    TraceControlTraceSetDisallowList= 34,
    TraceControlSetCompressionSettings = 35,
    TraceControlGetCompressionSettings= 36,
    TraceControlUpdatePeriodicCaptureState = 37,
    TraceControlGetPrivateSessionTraceHandle = 38,
    TraceControlRegisterPrivateSession = 39,
    TraceControlQuerySessionDemuxObject = 40,
    TraceControlSetProviderBinaryTracking = 41,
    TraceControlMaxLoggers = 42,
    TraceControlMaxPmcCounter = 43
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
