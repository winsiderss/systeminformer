/*
 * Process Hacker Extended Tools - 
 *   ETW monitoring
 * 
 * Copyright (C) 2010 wj32
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

#include "exttools.h"
#include "etwmon.h"

ULONG NTAPI EtpEtwBufferCallback(
    __in PEVENT_TRACE_LOGFILE Buffer
    );

VOID NTAPI EtpEtwEventCallback(
    __in PEVENT_TRACE EventTrace
    );

NTSTATUS EtpEtwMonitorThreadStart(
    __in PVOID Parameter
    );

static GUID SystemTraceControlGuid_I = { 0x9e814aad, 0x3204, 0x11d2, { 0x9a, 0x82, 0x00, 0x60, 0x08, 0xa8, 0x69, 0x39 } };
static GUID DiskIoGuid_I = { 0x3d6fa8d4, 0xfe05, 0x11d0, { 0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c } };
static GUID TcpIpGuid_I = { 0x9a280ac0, 0xc8e0, 0x11d1, { 0x84, 0xe2, 0x00, 0xc0, 0x4f, 0xb9, 0x98, 0xa2 } };
static GUID UdpIpGuid_I = { 0xbf3a50c5, 0xa9c9, 0x4988, { 0xa0, 0x05, 0x2d, 0xf0, 0xb7, 0xc8, 0x0f, 0x80 } };

// ETW interfacing layer

BOOLEAN EtEtwEnabled;
static PH_STRINGREF EtpLoggerName = PH_STRINGREF_INIT(KERNEL_LOGGER_NAME);
static TRACEHANDLE EtpSessionHandle;
static TRACEHANDLE EtpTraceHandle;
static PEVENT_TRACE_PROPERTIES EtpTraceProperties;
static EVENT_TRACE_LOGFILE EtpLogFile;
static BOOLEAN EtpEtwActive;
static BOOLEAN EtpStartedSession;
static BOOLEAN EtpEtwExiting;
static HANDLE EtpEtwMonitorThreadHandle;

VOID EtEtwMonitorInitialization()
{
    if (PhElevated && PhGetIntegerSetting(SETTING_NAME_ENABLE_ETW_MONITOR))
    {
        EtStartEtwSession();

        if (EtEtwEnabled)
            EtpEtwMonitorThreadHandle = PhCreateThread(0, EtpEtwMonitorThreadStart, NULL);
    }
}

VOID EtEtwMonitorUninitialization()
{
    if (EtEtwEnabled)
    {
        EtpEtwExiting = TRUE;
        EtStopEtwSession();
    }
}

VOID EtStartEtwSession()
{
    ULONG result;
    ULONG bufferSize;

    bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + EtpLoggerName.Length + sizeof(WCHAR);

    if (!EtpTraceProperties)
        EtpTraceProperties = PhAllocate(bufferSize);

    memset(EtpTraceProperties, 0, sizeof(EVENT_TRACE_PROPERTIES));

    EtpTraceProperties->Wnode.BufferSize = bufferSize;
    EtpTraceProperties->Wnode.Guid = SystemTraceControlGuid_I;
    EtpTraceProperties->Wnode.ClientContext = 1;
    EtpTraceProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    EtpTraceProperties->MinimumBuffers = 1;
    EtpTraceProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    EtpTraceProperties->FlushTimer = 1;
    EtpTraceProperties->EnableFlags = EVENT_TRACE_FLAG_DISK_IO | EVENT_TRACE_FLAG_NETWORK_TCPIP;
    EtpTraceProperties->LogFileNameOffset = 0;
    EtpTraceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

    result = StartTrace(&EtpSessionHandle, EtpLoggerName.Buffer, EtpTraceProperties);

    if (result == ERROR_SUCCESS)
    {
        EtEtwEnabled = TRUE;
        EtpEtwActive = TRUE;
        EtpStartedSession = TRUE;
    }
    else if (result == ERROR_ALREADY_EXISTS)
    {
        EtEtwEnabled = TRUE;
        EtpEtwActive = TRUE;
        EtpStartedSession = FALSE;
        // The session already exists.
        //result = ControlTrace(0, EtpLoggerName.Buffer, EtpTraceProperties, EVENT_TRACE_CONTROL_UPDATE);
    }
    else
    {
        EtpEtwActive = FALSE;
        EtpStartedSession = FALSE;
    }
}

ULONG EtpControlEtwSession(
    __in ULONG ControlCode
    )
{
    // If we have a session handle, we use that instead of the logger name.

    EtpTraceProperties->LogFileNameOffset = 0; // make sure it is 0, otherwise ControlTrace crashes

    return ControlTrace(
        EtpStartedSession ? EtpSessionHandle : 0,
        EtpStartedSession ? NULL : EtpLoggerName.Buffer,
        EtpTraceProperties,
        ControlCode
        );
}

VOID EtStopEtwSession()
{
    if (EtEtwEnabled)
        EtpControlEtwSession(EVENT_TRACE_CONTROL_STOP);
}

VOID EtFlushEtwSession()
{
    if (EtEtwEnabled)
        EtpControlEtwSession(EVENT_TRACE_CONTROL_FLUSH);
}

ULONG NTAPI EtpEtwBufferCallback(
    __in PEVENT_TRACE_LOGFILE Buffer
    )
{
    return !EtpEtwExiting;
}

VOID NTAPI EtpEtwEventCallback(
    __in PEVENT_TRACE EventTrace
    )
{
    if (memcmp(&EventTrace->Header.Guid, &DiskIoGuid_I, sizeof(GUID)) == 0)
    {
        // DiskIo

        ET_ETW_DISK_EVENT diskEvent;

        memset(&diskEvent, 0, sizeof(ET_ETW_DISK_EVENT));

        switch (EventTrace->Header.Class.Type)
        {
        case EVENT_TRACE_TYPE_IO_READ:
            diskEvent.Type = EtEtwDiskReadType;
            break;
        case EVENT_TRACE_TYPE_IO_WRITE:
            diskEvent.Type = EtEtwDiskWriteType;
            break;
        default:
            break;
        }

        if (diskEvent.Type != 0)
        {
            DiskIo_TypeGroup1 *data = EventTrace->MofData;

            if (EventTrace->Header.ProcessId != -1)
            {
                diskEvent.ClientId.UniqueProcess = UlongToHandle(EventTrace->Header.ProcessId);
                diskEvent.ClientId.UniqueThread = UlongToHandle(EventTrace->Header.ThreadId);
            }

            diskEvent.TransferSize = data->TransferSize;

            EtProcessDiskEvent(&diskEvent);
        }
    }
    else if (
        memcmp(&EventTrace->Header.Guid, &TcpIpGuid_I, sizeof(GUID)) == 0 ||
        memcmp(&EventTrace->Header.Guid, &UdpIpGuid_I, sizeof(GUID)) == 0
        )
    {
        // TcpIp/UdpIp

        ET_ETW_NETWORK_EVENT networkEvent;

        memset(&networkEvent, 0, sizeof(ET_ETW_NETWORK_EVENT));

        switch (EventTrace->Header.Class.Type)
        {
        case 10: // send
            networkEvent.Type = EtEtwNetworkSendType;
            networkEvent.ProtocolType = PH_IPV4_NETWORK_TYPE;
            break;
        case 11: // receive
            networkEvent.Type = EtEtwNetworkReceiveType;
            networkEvent.ProtocolType = PH_IPV4_NETWORK_TYPE;
            break;
        case 26: // send ipv6
            networkEvent.Type = EtEtwNetworkSendType;
            networkEvent.ProtocolType = PH_IPV6_NETWORK_TYPE;
            break;
        case 27: // receive ipv6
            networkEvent.Type = EtEtwNetworkReceiveType;
            networkEvent.ProtocolType = PH_IPV6_NETWORK_TYPE;
            break;
        }

        if (memcmp(&EventTrace->Header.Guid, &TcpIpGuid_I, sizeof(GUID)) == 0)
            networkEvent.ProtocolType |= PH_TCP_PROTOCOL_TYPE;
        else
            networkEvent.ProtocolType |= PH_UDP_PROTOCOL_TYPE;

        if (networkEvent.Type != 0)
        {
            PH_IP_ENDPOINT source;
            PH_IP_ENDPOINT destination;

            if (networkEvent.ProtocolType & PH_IPV4_NETWORK_TYPE)
            {
                TcpIpOrUdpIp_IPV4_Header *data = EventTrace->MofData;

                networkEvent.ClientId.UniqueProcess = UlongToHandle(data->PID);
                networkEvent.TransferSize = data->size;

                source.Address.Type = PH_IPV4_NETWORK_TYPE;
                source.Address.Ipv4 = data->saddr;
                source.Port = _byteswap_ushort(data->sport);
                destination.Address.Type = PH_IPV4_NETWORK_TYPE;
                destination.Address.Ipv4 = data->daddr;
                destination.Port = _byteswap_ushort(data->dport);
            }
            else if (networkEvent.ProtocolType & PH_IPV6_NETWORK_TYPE)
            {
                TcpIpOrUdpIp_IPV6_Header *data = EventTrace->MofData;

                networkEvent.ClientId.UniqueProcess = UlongToHandle(data->PID);
                networkEvent.TransferSize = data->size;

                source.Address.Type = PH_IPV6_NETWORK_TYPE;
                source.Address.In6Addr = data->saddr;
                source.Port = _byteswap_ushort(data->sport);
                destination.Address.Type = PH_IPV6_NETWORK_TYPE;
                destination.Address.In6Addr = data->daddr;
                destination.Port = _byteswap_ushort(data->dport);
            }

            networkEvent.LocalEndpoint = source;

            if (networkEvent.ProtocolType & PH_TCP_PROTOCOL_TYPE)
                networkEvent.RemoteEndpoint = destination;

            EtProcessNetworkEvent(&networkEvent);
        }
    }
}

NTSTATUS NTAPI EtpEtwMonitorThreadStart(
    __in PVOID Parameter
    )
{
    ULONG result;

    memset(&EtpLogFile, 0, sizeof(EVENT_TRACE_LOGFILE));
    EtpLogFile.LoggerName = EtpLoggerName.Buffer;
    EtpLogFile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME;
    EtpLogFile.BufferCallback = EtpEtwBufferCallback;
    EtpLogFile.EventCallback = EtpEtwEventCallback;
    EtpLogFile.IsKernelTrace = TRUE;

    while (TRUE)
    {
        EtpTraceHandle = OpenTrace(&EtpLogFile);

        if (EtpTraceHandle != INVALID_PROCESSTRACE_HANDLE)
        {
            while (!EtpEtwExiting && (result = ProcessTrace(&EtpTraceHandle, 1, NULL, NULL)) == ERROR_SUCCESS)
                NOTHING;

            CloseTrace(EtpTraceHandle);
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
        if (!EtpEtwActive)
            Sleep(250);
    }

    return STATUS_SUCCESS;
}
