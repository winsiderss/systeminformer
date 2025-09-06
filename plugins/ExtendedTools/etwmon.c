/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2019-2024
 *
 */

#include "exttools.h"
#include "etwmon.h"
#include <symprv.h>

ULONG NTAPI EtpEtwBufferCallback(
    _In_ PEVENT_TRACE_LOGFILE Buffer
    );

VOID NTAPI EtpEtwEventCallback(
    _In_ PEVENT_RECORD EventRecord
    );

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS EtpEtwMonitorThreadStart(
    _In_ PVOID Parameter
    );

ULONG EtpStopEtwRundownSession(
    VOID
    );

ULONG NTAPI EtpRundownEtwBufferCallback(
    _In_ PEVENT_TRACE_LOGFILE Buffer
    );

VOID NTAPI EtpRundownEtwEventCallback(
    _In_ PEVENT_RECORD EventRecord
    );

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS EtpRundownEtwMonitorThreadStart(
    _In_ PVOID Parameter
    );

// 3875f5e7-8f79-406c-8cb9-ee8fd8bfcfbd
DEFINE_GUID(SystemInformerGuid, 0x3875f5e7, 0x8f79, 0x406c, 0x8c, 0xb9, 0xee, 0x8f, 0xd8, 0xbf, 0xcf, 0xbd);
// 9e814aad-3204-11d2-9a82-006008a86939
DEFINE_GUID(SystemTraceControlGuid_I, 0x9e814aad, 0x3204, 0x11d2, 0x9a, 0x82, 0x00, 0x60, 0x08, 0xa8, 0x69, 0x39);
// 3b9c9951-3480-4220-9377-9c8e5184f5cd
DEFINE_GUID(KernelRundownGuid_I, 0x3b9c9951, 0x3480, 0x4220, 0x93, 0x77, 0x9c, 0x8e, 0x51, 0x84, 0xf5, 0xcd);
// 3d6fa8d4-fe05-11d0-9dda-00c04fd7ba7c
DEFINE_GUID(DiskIoGuid_I, 0x3d6fa8d4, 0xfe05, 0x11d0, 0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c);
// 90cbdc39-4a3e-11d1-84f4-0000f80464e3
DEFINE_GUID(FileIoGuid_I, 0x90cbdc39, 0x4a3e, 0x11d1, 0x84, 0xf4, 0x00, 0x00, 0xf8, 0x04, 0x64, 0xe3);
// 9a280ac0-c8e0-11d1-84e2-00c04fb998a2
DEFINE_GUID(TcpIpGuid_I, 0x9a280ac0, 0xc8e0, 0x11d1, 0x84, 0xe2, 0x00, 0xc0, 0x4f, 0xb9, 0x98, 0xa2);
// bf3a50c5-a9c9-4988-a005-2df0b7c80f80
DEFINE_GUID(UdpIpGuid_I, 0xbf3a50c5, 0xa9c9, 0x4988, 0xa0, 0x05, 0x2d, 0xf0, 0xb7, 0xc8, 0x0f, 0x80);
// def2fe46-7bd6-4b80-bd94-f57fe20d0ce3
DEFINE_GUID(StackWalkGuid_I, 0xdef2fe46, 0x7bd6, 0x4b80, 0xbd, 0x94, 0xf5, 0x7f, 0xe2, 0x0d, 0x0c, 0xe3);

// ETW tracing layer

BOOLEAN EtEtwEnabled = FALSE;
ULONG EtEtwStatus = ERROR_SUCCESS;
static UNICODE_STRING EtpSharedKernelLoggerName = RTL_CONSTANT_STRING(KERNEL_LOGGER_NAME);
static UNICODE_STRING EtpPrivateKernelLoggerName = RTL_CONSTANT_STRING(L"SiKernelTraceSession");
static TRACEHANDLE EtpSessionHandle = INVALID_PROCESSTRACE_HANDLE;
static PUNICODE_STRING EtpActualKernelLoggerName = NULL;
static PCGUID EtpActualSessionGuid = NULL;
static UCHAR EtpTracePropertiesBuffer[sizeof(EVENT_TRACE_PROPERTIES) + max(sizeof(KERNEL_LOGGER_NAME), sizeof(L"SiKernelTraceSession"))];
static PEVENT_TRACE_PROPERTIES EtpTraceProperties = (PEVENT_TRACE_PROPERTIES)EtpTracePropertiesBuffer;
static BOOLEAN EtpStartedSession = FALSE;
static BOOLEAN EtpEtwExiting = FALSE;

// ETW rundown layer

static UNICODE_STRING EtpRundownLoggerName = RTL_CONSTANT_STRING(L"SiKernelRundownSession");
static TRACEHANDLE EtpRundownSessionHandle = INVALID_PROCESSTRACE_HANDLE;
static UCHAR EtpRundownTracePropertiesBuffer[sizeof(EVENT_TRACE_PROPERTIES) + sizeof(L"SiKernelRundownSession")];
static PEVENT_TRACE_PROPERTIES EtpRundownTraceProperties = (PEVENT_TRACE_PROPERTIES)EtpRundownTracePropertiesBuffer;
static BOOLEAN EtpRundownActive = FALSE;
static BOOLEAN EtpRundownEnabled = FALSE;

VOID EtEtwMonitorInitialization(
    VOID
    )
{
    if (PhGetOwnTokenAttributes().Elevated && PhGetIntegerSetting(SETTING_NAME_ENABLE_ETW_MONITOR))
    {
        EtEtwEnabled = TRUE;

        PhCreateThread2(EtpEtwMonitorThreadStart, NULL);
    }
}

VOID EtEtwMonitorUninitialization(
    VOID
    )
{
    EtpEtwExiting = TRUE;

    if (EtEtwEnabled)
    {
        EtStopEtwSession();
    }

    if (EtpRundownActive)
    {
        EtpStopEtwRundownSession();
    }
}

VOID EtStartEtwSession(
    VOID
    )
{
    TRACEHANDLE traceHandle = INVALID_PROCESSTRACE_HANDLE;
    ULONG bufferSize;
    ULONG status;

    if (EtWindowsVersion >= WINDOWS_8)
    {
        EtpActualKernelLoggerName = &EtpPrivateKernelLoggerName;
        EtpActualSessionGuid = &SystemInformerGuid;
    }
    else
    {
        EtpActualKernelLoggerName = &EtpSharedKernelLoggerName;
        EtpActualSessionGuid = &SystemTraceControlGuid_I;
    }

    bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + EtpActualKernelLoggerName->Length + sizeof(UNICODE_NULL);

    memset(EtpTraceProperties, 0, sizeof(EtpTracePropertiesBuffer));
    EtpTraceProperties->Wnode.BufferSize = bufferSize;
    EtpTraceProperties->Wnode.Guid = *EtpActualSessionGuid;
    EtpTraceProperties->Wnode.ClientContext = 1;
    EtpTraceProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    EtpTraceProperties->MinimumBuffers = 1;
    EtpTraceProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    EtpTraceProperties->FlushTimer = 1;
    EtpTraceProperties->EnableFlags = EVENT_TRACE_FLAG_DISK_IO | EVENT_TRACE_FLAG_DISK_FILE_IO | EVENT_TRACE_FLAG_NETWORK_TCPIP | EVENT_TRACE_FLAG_NO_SYSCONFIG;
    EtpTraceProperties->LogFileNameOffset = 0;
    EtpTraceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

    if (EtWindowsVersion >= WINDOWS_8)
        EtpTraceProperties->LogFileMode |= EVENT_TRACE_SYSTEM_LOGGER_MODE;

    // Get the existing session handle.
    status = ControlTrace(
        0,
        EtpActualKernelLoggerName->Buffer,
        EtpTraceProperties,
        EVENT_TRACE_CONTROL_QUERY
        );

    if (status == ERROR_SUCCESS)
    {
        traceHandle = EtpTraceProperties->Wnode.HistoricalContext;
    }
    else
    {
        EtpTraceProperties->LogFileNameOffset = 0;
        status = StartTrace(
            &traceHandle,
            EtpActualKernelLoggerName->Buffer,
            EtpTraceProperties
            );
    }

    // Enable stack tracing.
    // NOTE: This only enables stack traces for SystemTraceControlGuid events while the
    // EVENT_ENABLE_PROPERTY_STACK_TRACE flag must be used for other guids. (dmex)
    //if (EtWindowsVersion >= WINDOWS_8 && EtEtwStatus == ERROR_SUCCESS)
    //{
    //    UCHAR eventBuffer[FIELD_OFFSET(EVENT_TRACE_SYSTEM_EVENT_INFORMATION, HookId) + sizeof(ULONG[1])];
    //    PEVENT_TRACE_SYSTEM_EVENT_INFORMATION eventTraceStackTracingInfo;
    //
    //    memset(eventBuffer, 0, sizeof(eventBuffer));
    //    eventTraceStackTracingInfo = (PEVENT_TRACE_SYSTEM_EVENT_INFORMATION)eventBuffer;
    //    eventTraceStackTracingInfo->EventTraceInformationClass = EventTraceStackTracingInformation;
    //    eventTraceStackTracingInfo->TraceHandle = EtpSessionHandle;
    //    eventTraceStackTracingInfo->HookId[0] = PERFINFO_LOG_TYPE_FILENAME_CREATE;
    //
    //    status = PhNtStatusToDosError(NtSetSystemInformation(
    //        SystemPerformanceTraceInformation,
    //        eventTraceStackTracingInfo,
    //        sizeof(eventBuffer)
    //        ));
    //}
    //
    // Enable trace flags. (dmex)
    //if (EtWindowsVersion >= WINDOWS_8 && status == ERROR_SUCCESS)
    //{
    //    EVENT_TRACE_GROUPMASK_INFORMATION eventTraceGroupMaskInfo;
    //    PERFINFO_MASK eventTraceInfoMask = PERF_DISK_IO | PERF_NETWORK | PERF_NO_SYSCONFIG;
    //
    //    memset(&eventTraceGroupMaskInfo, 0, sizeof(EVENT_TRACE_GROUPMASK_INFORMATION));
    //    eventTraceGroupMaskInfo.EventTraceInformationClass = EventTraceGroupMaskInformation;
    //    eventTraceGroupMaskInfo.TraceHandle = EtpSessionHandle;
    //    NtQuerySystemInformation(SystemPerformanceTraceInformation, &eventTraceGroupMaskInfo, sizeof(eventTraceGroupMaskInfo), 0);
    //    PERFINFO_OR_GROUP_WITH_GROUPMASK(eventTraceInfoMask, &eventTraceGroupMaskInfo.EventTraceGroupMasks);
    //
    //    status = PhNtStatusToDosError(NtSetSystemInformation(
    //        SystemPerformanceTraceInformation,
    //        &eventTraceGroupMaskInfo,
    //        sizeof(EVENT_TRACE_GROUPMASK_INFORMATION)
    //        ));
    //}

    if (status == ERROR_SUCCESS)
    {
        EtpStartedSession = TRUE;
        EtpSessionHandle = traceHandle;
        EtEtwStatus = status;
    }
    else
    {
        EtpStartedSession = FALSE;
        EtpSessionHandle = INVALID_PROCESSTRACE_HANDLE; // StartTrace set the handle 0 on failure. (dmex)
        EtEtwStatus = status;
    }
}

ULONG EtControlEtwSession(
    _In_ ULONG ControlCode
    )
{
    // If we have a session handle, we use that instead of the logger name.
    EtpTraceProperties->LogFileNameOffset = 0; // make sure it is 0, otherwise ControlTrace crashes
    return ControlTrace(
        EtpStartedSession ? EtpSessionHandle : 0,
        EtpStartedSession ? NULL : EtpActualKernelLoggerName->Buffer,
        EtpTraceProperties,
        ControlCode
        );
}

VOID EtStopEtwSession(
    VOID
    )
{
    if (EtpStartedSession)
        EtControlEtwSession(EVENT_TRACE_CONTROL_STOP);
}

ULONG NTAPI EtpEtwBufferCallback(
    _In_ PEVENT_TRACE_LOGFILE Buffer
    )
{
    return !EtpEtwExiting;
}

VOID NTAPI EtpEtwEventCallback(
    _In_ PEVENT_RECORD EventRecord
    )
{
    if (EtpEtwExiting)
        return;

    if (IsEqualGUID(&EventRecord->EventHeader.ProviderId, &DiskIoGuid_I))
    {
        ET_ETW_DISK_EVENT diskEvent;

        memset(&diskEvent, 0, sizeof(ET_ETW_DISK_EVENT));
        diskEvent.Type = ULONG_MAX;

        switch (EventRecord->EventHeader.EventDescriptor.Opcode)
        {
        case EVENT_TRACE_TYPE_IO_READ:
            diskEvent.Type = EtEtwDiskReadType;
            break;
        case EVENT_TRACE_TYPE_IO_WRITE:
            diskEvent.Type = EtEtwDiskWriteType;
            break;
        }

        if (diskEvent.Type != ULONG_MAX)
        {
            PETW_DISKIO_READWRITE_V3 data = EventRecord->UserData;

            if (EtWindowsVersion >= WINDOWS_8)
            {
                if (data->IssuingThreadId != ULONG_MAX)
                {
                    diskEvent.ClientId.UniqueThread = UlongToHandle(data->IssuingThreadId);
                    diskEvent.ClientId.UniqueProcess = EtThreadIdToProcessId(diskEvent.ClientId.UniqueThread);
                }
                else
                {
                    diskEvent.ClientId.UniqueThread = NULL;
                    diskEvent.ClientId.UniqueProcess = SYSTEM_PROCESS_ID;
                }
            }
            else
            {
                if (EventRecord->EventHeader.ProcessId != ULONG_MAX)
                {
                    diskEvent.ClientId.UniqueProcess = UlongToHandle(EventRecord->EventHeader.ProcessId);
                    diskEvent.ClientId.UniqueThread = UlongToHandle(EventRecord->EventHeader.ThreadId);
                }
                else
                {
                    diskEvent.ClientId.UniqueThread = NULL;
                    diskEvent.ClientId.UniqueProcess = SYSTEM_PROCESS_ID;
                }
            }

            diskEvent.IrpFlags = data->IrpFlags;
            diskEvent.TransferSize = data->Size;
            diskEvent.FileObject = data->FileObject;
            diskEvent.HighResResponseTime = data->HighResResponseTime;

            EtProcessDiskEvent(&diskEvent);
            EtDiskProcessDiskEvent(&diskEvent);
        }
    }
    else if (IsEqualGUID(&EventRecord->EventHeader.ProviderId, &FileIoGuid_I))
    {
        ET_ETW_FILE_EVENT fileEvent;

        memset(&fileEvent, 0, sizeof(ET_ETW_FILE_EVENT));
        fileEvent.Type = ULONG_MAX;

        switch (EventRecord->EventHeader.EventDescriptor.Opcode)
        {
        case EVENT_TRACE_TYPE_FILENAME:
            fileEvent.Type = EtEtwFileNameType;
            break;
        case EVENT_TRACE_TYPE_FILENAME_CREATE:
            fileEvent.Type = EtEtwFileCreateType;
            break;
        case EVENT_TRACE_TYPE_FILENAME_DELETE:
            fileEvent.Type = EtEtwFileDeleteType;
            break;
        case EVENT_TRACE_TYPE_FILENAME_RUNDOWN:
            fileEvent.Type = EtEtwFileRundownType;
            break;
        }

        if (fileEvent.Type != ULONG_MAX)
        {
            if (EtIsExecutingInWow64)
            {
                if (EventRecord->EventHeader.EventDescriptor.Version == 2)
                {
                    PWMI_FILE_IO_WOW64 dataWow64 = EventRecord->UserData;

                    fileEvent.FileObject = (PVOID)dataWow64->FileObject;
                    fileEvent.FileName.Length = (EventRecord->UserDataLength - UFIELD_OFFSET(WMI_FILE_IO_WOW64, FileName)) - sizeof(UNICODE_NULL);
                    fileEvent.FileName.Buffer = dataWow64->FileName;
                }
                else
                {
                    PWMI_FILE_IO_WOW64 dataWow64 = EventRecord->UserData;

                    fileEvent.FileObject = (PVOID)dataWow64->FileObject;
                    PhInitializeStringRefLongHint(&fileEvent.FileName, dataWow64->FileName);
                }
            }
            else
            {
                if (EventRecord->EventHeader.EventDescriptor.Version == 2)
                {
                    PWMI_FILE_IO data = EventRecord->UserData;

                    fileEvent.FileObject = data->FileObject;
                    fileEvent.FileName.Length = (EventRecord->UserDataLength - UFIELD_OFFSET(WMI_FILE_IO, FileName)) - sizeof(UNICODE_NULL);
                    fileEvent.FileName.Buffer = data->FileName;
                }
                else
                {
                    PWMI_FILE_IO data = EventRecord->UserData;

                    fileEvent.FileObject = data->FileObject;
                    PhInitializeStringRefLongHint(&fileEvent.FileName, data->FileName);
                }
            }

            EtDiskProcessFileEvent(&fileEvent);
        }
    }
    else if (IsEqualGUID(&EventRecord->EventHeader.ProviderId, &TcpIpGuid_I))
    {
        switch (EventRecord->EventHeader.EventDescriptor.Opcode)
        {
        case EVENT_TRACE_TYPE_SEND:
            {
                PWMI_TCPIP_V4 data = EventRecord->UserData;
                ET_ETW_NETWORK_EVENT networkEvent;
                PH_IP_ENDPOINT source;
                PH_IP_ENDPOINT destination;

                source.Address.Type = PH_NETWORK_TYPE_IPV4;
                source.Port = _byteswap_ushort(data->SourcePort);
                memcpy(&source.Address.InAddr, data->SourceAddress, sizeof(IN_ADDR));
                destination.Address.Type = PH_NETWORK_TYPE_IPV4;
                destination.Port = _byteswap_ushort(data->DestinationPort);
                memcpy(&destination.Address.InAddr, data->DestinationAddress, sizeof(IN_ADDR));

                memset(&networkEvent, 0, sizeof(ET_ETW_NETWORK_EVENT));
                networkEvent.Type = EtEtwNetworkSendType;
                networkEvent.ProtocolType = PH_PROTOCOL_TYPE_TCP | PH_NETWORK_TYPE_IPV4;
                networkEvent.TransferSize = data->TransferSize;
                networkEvent.ClientId.UniqueProcess = UlongToHandle(data->ProcessId);
                networkEvent.LocalEndpoint = source;
                networkEvent.RemoteEndpoint = destination;

                EtProcessNetworkEvent(&networkEvent);
            }
            break;
        case EVENT_TRACE_TYPE_RECEIVE:
            {
                PWMI_TCPIP_V4 data = EventRecord->UserData;
                ET_ETW_NETWORK_EVENT networkEvent;
                PH_IP_ENDPOINT source;
                PH_IP_ENDPOINT destination;

                source.Address.Type = PH_NETWORK_TYPE_IPV4;
                source.Port = _byteswap_ushort(data->SourcePort);
                memcpy(source.Address.Ipv4, data->SourceAddress, sizeof(IN_ADDR));
                destination.Address.Type = PH_NETWORK_TYPE_IPV4;
                destination.Port = _byteswap_ushort(data->DestinationPort);
                memcpy(destination.Address.Ipv4, data->DestinationAddress, sizeof(IN_ADDR));

                memset(&networkEvent, 0, sizeof(ET_ETW_NETWORK_EVENT));
                networkEvent.Type = EtEtwNetworkReceiveType;
                networkEvent.ProtocolType = PH_PROTOCOL_TYPE_TCP | PH_NETWORK_TYPE_IPV4;
                networkEvent.TransferSize = data->TransferSize;
                networkEvent.ClientId.UniqueProcess = UlongToHandle(data->ProcessId);
                networkEvent.LocalEndpoint = source;
                networkEvent.RemoteEndpoint = destination;

                EtProcessNetworkEvent(&networkEvent);
            }
            break;
        case EVENT_TRACE_TYPE_TCPIP_SEND_IPV6:
            {
                PWMI_TCPIP_V6 data = EventRecord->UserData;
                ET_ETW_NETWORK_EVENT networkEvent;
                PH_IP_ENDPOINT source;
                PH_IP_ENDPOINT destination;

                source.Address.Type = PH_NETWORK_TYPE_IPV6;
                source.Port = _byteswap_ushort(data->SourcePort);
                memcpy(source.Address.Ipv6, data->SourceAddress, sizeof(IN6_ADDR));
                destination.Address.Type = PH_NETWORK_TYPE_IPV6;
                destination.Port = _byteswap_ushort(data->DestinationPort);
                memcpy(destination.Address.Ipv6, data->DestinationAddress, sizeof(IN6_ADDR));

                memset(&networkEvent, 0, sizeof(ET_ETW_NETWORK_EVENT));
                networkEvent.Type = EtEtwNetworkSendType;
                networkEvent.ProtocolType = PH_PROTOCOL_TYPE_TCP | PH_NETWORK_TYPE_IPV6;
                networkEvent.TransferSize = data->TransferSize;
                networkEvent.ClientId.UniqueProcess = UlongToHandle(data->ProcessId);
                networkEvent.LocalEndpoint = source;
                networkEvent.RemoteEndpoint = destination;

                EtProcessNetworkEvent(&networkEvent);
            }
            break;
        case EVENT_TRACE_TYPE_TCPIP_RECEIVE_IPV6:
            {
                PWMI_TCPIP_V6 data = EventRecord->UserData;
                ET_ETW_NETWORK_EVENT networkEvent;
                PH_IP_ENDPOINT source;
                PH_IP_ENDPOINT destination;

                source.Address.Type = PH_NETWORK_TYPE_IPV6;
                source.Port = _byteswap_ushort(data->SourcePort);
                memcpy(source.Address.Ipv6, data->SourceAddress, sizeof(IN6_ADDR));
                destination.Address.Type = PH_NETWORK_TYPE_IPV6;
                destination.Port = _byteswap_ushort(data->DestinationPort);
                memcpy(destination.Address.Ipv6, data->DestinationAddress, sizeof(IN6_ADDR));

                memset(&networkEvent, 0, sizeof(ET_ETW_NETWORK_EVENT));
                networkEvent.Type = EtEtwNetworkReceiveType;
                networkEvent.ProtocolType = PH_PROTOCOL_TYPE_TCP | PH_NETWORK_TYPE_IPV6;
                networkEvent.TransferSize = data->TransferSize;
                networkEvent.ClientId.UniqueProcess = UlongToHandle(data->ProcessId);
                networkEvent.LocalEndpoint = source;
                networkEvent.RemoteEndpoint = destination;

                EtProcessNetworkEvent(&networkEvent);
            }
            break;
        }
    }
    else if (IsEqualGUID(&EventRecord->EventHeader.ProviderId, &UdpIpGuid_I))
    {
        switch (EventRecord->EventHeader.EventDescriptor.Opcode)
        {
        case EVENT_TRACE_TYPE_SEND:
            {
                PWMI_UDP_V4 data = EventRecord->UserData;
                ET_ETW_NETWORK_EVENT networkEvent;
                PH_IP_ENDPOINT source;
                //PH_IP_ENDPOINT destination;

                source.Address.Type = PH_NETWORK_TYPE_IPV4;
                memcpy(source.Address.Ipv4, data->SourceAddress, sizeof(IN_ADDR));
                source.Port = _byteswap_ushort(data->SourcePort);
                //destination.Address.Type = PH_NETWORK_TYPE_IPV4;
                //destination.Address.Ipv4 = data->daddr.s_addr;
                //destination.Port = _byteswap_ushort(data->dport);

                memset(&networkEvent, 0, sizeof(ET_ETW_NETWORK_EVENT));
                networkEvent.Type = EtEtwNetworkSendType;
                networkEvent.ProtocolType = PH_PROTOCOL_TYPE_UDP | PH_NETWORK_TYPE_IPV4;
                networkEvent.TransferSize = data->TransferSize;
                networkEvent.ClientId.UniqueProcess = UlongToHandle(data->ProcessId);
                networkEvent.LocalEndpoint = source;
                //networkEvent.RemoteEndpoint = destination;

                EtProcessNetworkEvent(&networkEvent);
            }
            break;
        case EVENT_TRACE_TYPE_RECEIVE:
            {
                PWMI_UDP_V4 data = EventRecord->UserData;
                ET_ETW_NETWORK_EVENT networkEvent;
                PH_IP_ENDPOINT source;
                //PH_IP_ENDPOINT destination;

                // Note: The endpoints are swapped for incoming UDP packets. The destination endpoint
                // corresponds to the local socket not the source endpoint. (DavidXanatos)
                source.Address.Type = PH_NETWORK_TYPE_IPV4;
                memcpy(source.Address.Ipv4, data->DestinationAddress, sizeof(IN_ADDR));
                source.Port = _byteswap_ushort(data->SourcePort);
                //destination.Address.Type = PH_NETWORK_TYPE_IPV4;
                //destination.Address.Ipv4 = data->saddr.s_addr;
                //destination.Port = _byteswap_ushort(data->dport);

                memset(&networkEvent, 0, sizeof(ET_ETW_NETWORK_EVENT));
                networkEvent.Type = EtEtwNetworkReceiveType;
                networkEvent.ProtocolType = PH_PROTOCOL_TYPE_UDP | PH_NETWORK_TYPE_IPV4;
                networkEvent.TransferSize = data->TransferSize;
                networkEvent.ClientId.UniqueProcess = UlongToHandle(data->ProcessId);
                networkEvent.LocalEndpoint = source;
                //networkEvent.RemoteEndpoint = destination;

                EtProcessNetworkEvent(&networkEvent);
            }
            break;
        case EVENT_TRACE_TYPE_TCPIP_SEND_IPV6:
            {
                PWMI_UDP_V6 data = EventRecord->UserData;
                ET_ETW_NETWORK_EVENT networkEvent;
                PH_IP_ENDPOINT source;
                //PH_IP_ENDPOINT destination;

                source.Address.Type = PH_NETWORK_TYPE_IPV6;
                memcpy(source.Address.Ipv6, data->SourceAddress, sizeof(IN6_ADDR));
                source.Port = _byteswap_ushort(data->SourcePort);
                //destination.Address.Type = PH_NETWORK_TYPE_IPV6;
                //destination.Address.In6Addr = data->daddr;
                //destination.Port = _byteswap_ushort(data->dport);

                memset(&networkEvent, 0, sizeof(ET_ETW_NETWORK_EVENT));
                networkEvent.Type = EtEtwNetworkSendType;
                networkEvent.ProtocolType = PH_PROTOCOL_TYPE_UDP | PH_NETWORK_TYPE_IPV6;
                networkEvent.TransferSize = data->TransferSize;
                networkEvent.ClientId.UniqueProcess = UlongToHandle(data->ProcessId);
                networkEvent.LocalEndpoint = source;
                //networkEvent.RemoteEndpoint = destination;

                EtProcessNetworkEvent(&networkEvent);
            }
            break;
        case EVENT_TRACE_TYPE_TCPIP_RECEIVE_IPV6:
            {
                PWMI_UDP_V6 data = EventRecord->UserData;
                ET_ETW_NETWORK_EVENT networkEvent;
                PH_IP_ENDPOINT source;
                //PH_IP_ENDPOINT destination;

                // Note: The endpoints are swapped for incoming UDP packets. The destination endpoint
                // corresponds to the local socket not the source endpoint. (DavidXanatos)
                source.Address.Type = PH_NETWORK_TYPE_IPV6;
                memcpy(source.Address.Ipv6, data->DestinationAddress, sizeof(IN6_ADDR));
                source.Port = _byteswap_ushort(data->DestinationPort);
                //destination.Address.Type = PH_NETWORK_TYPE_IPV6;
                //destination.Address.In6Addr = data->saddr;
                //destination.Port = _byteswap_ushort(data->dport);

                memset(&networkEvent, 0, sizeof(ET_ETW_NETWORK_EVENT));
                networkEvent.Type = EtEtwNetworkReceiveType;
                networkEvent.ProtocolType = PH_PROTOCOL_TYPE_UDP | PH_NETWORK_TYPE_IPV6;
                networkEvent.TransferSize = data->TransferSize;
                networkEvent.ClientId.UniqueProcess = UlongToHandle(data->ProcessId);
                networkEvent.LocalEndpoint = source;
                //networkEvent.RemoteEndpoint = destination;

                EtProcessNetworkEvent(&networkEvent);
            }
            break;
        }
    }
    //else if (IsEqualGUID(&EventRecord->EventHeader.ProviderId, &StackWalkGuid_I))
    //{
    //    ULONG stackWalkEventCount = (EventRecord->UserDataLength - UFIELD_OFFSET(ET_ETW_STACKWALK_EVENT, Stack)) / sizeof(ULONG_PTR);
    //    PET_ETW_STACKWALK_EVENT stackWalkEvent = EventRecord->UserData;
    //    PPH_SYMBOL_PROVIDER symbolProvider;
    //
    //    symbolProvider = PhCreateSymbolProvider(UlongToHandle(stackWalkEvent->StackProcess));
    //    PhLoadSymbolProviderOptions(symbolProvider);
    //    PhLoadModulesForProcessSymbolProvider(symbolProvider, UlongToHandle(stackWalkEvent->StackProcess));
    //    //dprintf("Stack for process: %lu [TID: %lu]\n", stackWalkEvent->StackProcess, stackWalkEvent->StackThread);
    //
    //    for (ULONG i = 0; i < stackWalkEventCount; i++)
    //    {
    //        PPH_STRING name;
    //
    //        if (!stackWalkEvent->Stack[i])
    //            break;
    //
    //        name = PhGetSymbolFromAddress(
    //            symbolProvider,
    //            (ULONG64)stackWalkEvent->Stack[i],
    //            NULL,
    //            NULL,
    //            NULL,
    //            NULL
    //            );
    //
    //        //dprintf("%lu: %S\n", i, PhGetStringOrEmpty(name));
    //        PhClearReference(&name);
    //    }
    //
    //    PhDereferenceObject(symbolProvider);
    //}
    //else
    //{
    //    PPH_STRING guidString = PhFormatGuid(&EventRecord->EventHeader.ProviderId);
    //
    //    if (guidString)
    //    {
    //        dprintf("Event: %S (opcode: %lu)\n", guidString->Buffer, EventRecord->EventHeader.EventDescriptor.Opcode);
    //        PhDereferenceObject(guidString);
    //    }
    //}
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS EtpEtwMonitorThreadStart(
    _In_ PVOID Parameter
    )
{
    ULONG result;
    EVENT_TRACE_LOGFILE logFile;
    TRACEHANDLE traceHandle;

    PhSetThreadName(NtCurrentThread(), L"EtwMonitorThread");
    PhSetThreadBasePriority(NtCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    EtStartEtwSession();

    memset(&logFile, 0, sizeof(EVENT_TRACE_LOGFILE));
    logFile.LoggerName = EtpActualKernelLoggerName->Buffer;
    logFile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_RAW_TIMESTAMP;
    logFile.BufferCallback = EtpEtwBufferCallback;
    logFile.EventRecordCallback = EtpEtwEventCallback;

    while (!EtpEtwExiting)
    {
        result = ERROR_SUCCESS;
        traceHandle = OpenTrace(&logFile);

        if (traceHandle != INVALID_PROCESSTRACE_HANDLE)
        {
            while (!EtpEtwExiting && (result = ProcessTrace(&traceHandle, 1, NULL, NULL)) == ERROR_SUCCESS)
                NOTHING;

            CloseTrace(traceHandle);
        }

        if (EtpEtwExiting)
            break;

        if (result == ERROR_WMI_INSTANCE_NOT_FOUND)
        {
            // The session was stopped by another program. Try to start it again.
            EtStartEtwSession();
        }

        // Some error occurred, so sleep for a while before trying again.
        // Don't sleep if we just successfully started a session, though.
        if (!EtpStartedSession)
            PhDelayExecution(250);
    }

    return STATUS_SUCCESS;
}

VOID EtStartEtwRundown(
    VOID
    )
{
    if (EtWindowsVersion >= WINDOWS_8 && EtpStartedSession && EtpSessionHandle != INVALID_PROCESSTRACE_HANDLE)
    {
        ULONG result;

        // Enable the filename rundown in our existing trace session. (dmex)

        result = EnableTraceEx2(
            EtpSessionHandle,
            &KernelRundownGuid_I,
            EVENT_CONTROL_CODE_ENABLE_PROVIDER,
            TRACE_LEVEL_NONE,
            KERNEL_FILE_KEYWORD_FILENAME,
            0,
            INFINITE,
            NULL
            );

        if (result == ERROR_SUCCESS)
            return;

        // Fallback to the legacy trace session/thread. (dmex)
    }

    PhCreateThread2(EtpRundownEtwMonitorThreadStart, NULL);
}

ULONG EtpStartRundownSession(
    VOID
    )
{
    ULONG result;
    ULONG bufferSize;

    bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + EtpRundownLoggerName.Length + sizeof(UNICODE_NULL);

    memset(EtpRundownTraceProperties, 0, sizeof(EtpRundownTracePropertiesBuffer));
    EtpRundownTraceProperties->Wnode.BufferSize = bufferSize;
    EtpRundownTraceProperties->Wnode.ClientContext = EVENT_TRACE_CLOCK_RAW;
    EtpRundownTraceProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    EtpRundownTraceProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    EtpRundownTraceProperties->FlushTimer = 1;
    EtpRundownTraceProperties->EnableFlags = EVENT_TRACE_FLAG_NO_SYSCONFIG;
    EtpRundownTraceProperties->LogFileNameOffset = 0;
    EtpRundownTraceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

    result = ControlTrace(
        0,
        EtpRundownLoggerName.Buffer,
        EtpRundownTraceProperties,
        EVENT_TRACE_CONTROL_QUERY
        );

    if (result == ERROR_SUCCESS)
    {
        EtpStopEtwRundownSession();
    }

    EtpRundownTraceProperties->LogFileNameOffset = 0;
    result = StartTrace(
        &EtpRundownSessionHandle,
        EtpRundownLoggerName.Buffer,
        EtpRundownTraceProperties
        );

    if (result != ERROR_SUCCESS)
    {
        EtpRundownSessionHandle = INVALID_PROCESSTRACE_HANDLE; // StartTrace set the handle 0 on failure. (dmex)
        return result;
    }

    EtpRundownActive = TRUE;

    return result;
}

ULONG EtpStopEtwRundownSession(
    VOID
    )
{
    EtpRundownTraceProperties->LogFileNameOffset = 0;
    return ControlTrace(0, EtpRundownLoggerName.Buffer, EtpRundownTraceProperties, EVENT_TRACE_CONTROL_STOP);
}

ULONG NTAPI EtpRundownEtwBufferCallback(
    _In_ PEVENT_TRACE_LOGFILE Buffer
    )
{
    return !EtpEtwExiting;
}

VOID NTAPI EtpRundownEtwEventCallback(
    _In_ PEVENT_RECORD EventRecord
    )
{
    if (IsEqualGUID(&EventRecord->EventHeader.ProviderId, &FileIoGuid_I))
    {
        ET_ETW_FILE_EVENT fileEvent;

        memset(&fileEvent, 0, sizeof(ET_ETW_FILE_EVENT));
        fileEvent.Type = ULONG_MAX;

        switch (EventRecord->EventHeader.EventDescriptor.Opcode)
        {
        case EVENT_TRACE_TYPE_FILENAME_RUNDOWN:
            fileEvent.Type = EtEtwFileRundownType;
            break;
        }

        if (fileEvent.Type != ULONG_MAX)
        {
            if (EtIsExecutingInWow64)
            {
                PWMI_FILE_IO_WOW64 dataWow64 = EventRecord->UserData;

                fileEvent.FileObject = (PVOID)dataWow64->FileObject;
                PhInitializeStringRefLongHint(&fileEvent.FileName, dataWow64->FileName);
            }
            else
            {
                PWMI_FILE_IO data = EventRecord->UserData;

                fileEvent.FileObject = data->FileObject;
                PhInitializeStringRefLongHint(&fileEvent.FileName, data->FileName);
            }

            EtDiskProcessFileEvent(&fileEvent);
        }
    }
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS EtpRundownEtwMonitorThreadStart(
    _In_ PVOID Parameter
    )
{
    EVENT_TRACE_LOGFILE logFile;
    TRACEHANDLE traceHandle;

    EtpStartRundownSession();

    memset(&logFile, 0, sizeof(EVENT_TRACE_LOGFILE));
    logFile.LoggerName = EtpRundownLoggerName.Buffer;
    logFile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_RAW_TIMESTAMP;
    logFile.BufferCallback = EtpRundownEtwBufferCallback;
    logFile.EventRecordCallback = EtpRundownEtwEventCallback;

    traceHandle = OpenTrace(&logFile);

    if (traceHandle != INVALID_PROCESSTRACE_HANDLE)
    {
        // Note: The rundown session must only be enabled after we've opened the trace otherwise
        // the provider will generate eventlog warnings about no realtime listeners and drop events.
        // Wdc.dll will only enable the trace after the first IO_READ/IO_WRITE event so we'll enable
        // here. However if we're still losing events and getting eventlog warnings then move this code
        // into EtpEtwEventCallback and enable after the first IO_READ/IO_WRITE event. (dmex)
        if (!EtpRundownEnabled && EtpRundownSessionHandle != INVALID_PROCESSTRACE_HANDLE)
        {
            EnableTraceEx2(
                EtpRundownSessionHandle,
                &KernelRundownGuid_I,
                EVENT_CONTROL_CODE_ENABLE_PROVIDER,
                TRACE_LEVEL_NONE,
                KERNEL_FILE_KEYWORD_FILENAME,
                0,
                INFINITE,
                NULL
                );

            EtpRundownEnabled = TRUE;
        }

        ProcessTrace(&traceHandle, 1, NULL, NULL);

        CloseTrace(traceHandle);
    }

    return STATUS_SUCCESS;
}
