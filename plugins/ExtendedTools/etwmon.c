/*
 * Process Hacker Extended Tools -
 *   ETW monitoring
 *
 * Copyright (C) 2010-2011 wj32
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

ULONG NTAPI EtpRundownEtwBufferCallback(
    __in PEVENT_TRACE_LOGFILE Buffer
    );

VOID NTAPI EtpRundownEtwEventCallback(
    __in PEVENT_RECORD EventRecord
    );

NTSTATUS EtpRundownEtwMonitorThreadStart(
    __in PVOID Parameter
    );

static GUID SystemTraceControlGuid_I = { 0x9e814aad, 0x3204, 0x11d2, { 0x9a, 0x82, 0x00, 0x60, 0x08, 0xa8, 0x69, 0x39 } };
static GUID KernelRundownGuid_I = { 0x3b9c9951, 0x3480, 0x4220, { 0x93, 0x77, 0x9c, 0x8e, 0x51, 0x84, 0xf5, 0xcd } };
static GUID DiskIoGuid_I = { 0x3d6fa8d4, 0xfe05, 0x11d0, { 0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c } };
static GUID FileIoGuid_I = { 0x90cbdc39, 0x4a3e, 0x11d1, { 0x84, 0xf4, 0x00, 0x00, 0xf8, 0x04, 0x64, 0xe3 } };
static GUID TcpIpGuid_I = { 0x9a280ac0, 0xc8e0, 0x11d1, { 0x84, 0xe2, 0x00, 0xc0, 0x4f, 0xb9, 0x98, 0xa2 } };
static GUID UdpIpGuid_I = { 0xbf3a50c5, 0xa9c9, 0x4988, { 0xa0, 0x05, 0x2d, 0xf0, 0xb7, 0xc8, 0x0f, 0x80 } };

// ETW tracing layer

BOOLEAN EtEtwEnabled;
static PH_STRINGREF EtpLoggerName = PH_STRINGREF_INIT(KERNEL_LOGGER_NAME);
static TRACEHANDLE EtpSessionHandle;
static PEVENT_TRACE_PROPERTIES EtpTraceProperties;
static BOOLEAN EtpEtwActive;
static BOOLEAN EtpStartedSession;
static BOOLEAN EtpEtwExiting;
static HANDLE EtpEtwMonitorThreadHandle;

// ETW rundown layer

static PH_STRINGREF EtpRundownLoggerName = PH_STRINGREF_INIT(L"PhEtRundownLogger");
static TRACEHANDLE EtpRundownSessionHandle;
static PEVENT_TRACE_PROPERTIES EtpRundownTraceProperties;
static BOOLEAN EtpRundownActive;
static HANDLE EtpRundownEtwMonitorThreadHandle;

VOID EtEtwMonitorInitialization(
    VOID
    )
{
    if (PhElevated && PhGetIntegerSetting(SETTING_NAME_ENABLE_ETW_MONITOR))
    {
        EtStartEtwSession();

        if (EtEtwEnabled)
            EtpEtwMonitorThreadHandle = PhCreateThread(0, EtpEtwMonitorThreadStart, NULL);
    }
}

VOID EtEtwMonitorUninitialization(
    VOID
    )
{
    if (EtEtwEnabled)
    {
        EtpEtwExiting = TRUE;
        EtStopEtwSession();
    }

    if (EtpRundownActive)
    {
        EtpRundownTraceProperties->LogFileNameOffset = 0;
        ControlTrace(0, EtpRundownLoggerName.Buffer, EtpRundownTraceProperties, EVENT_TRACE_CONTROL_STOP);
    }
}

VOID EtStartEtwSession(
    VOID
    )
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
    EtpTraceProperties->EnableFlags = EVENT_TRACE_FLAG_DISK_IO | EVENT_TRACE_FLAG_DISK_FILE_IO | EVENT_TRACE_FLAG_NETWORK_TCPIP;
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

VOID EtStopEtwSession(
    VOID
    )
{
    if (EtEtwEnabled)
        EtpControlEtwSession(EVENT_TRACE_CONTROL_STOP);
}

VOID EtFlushEtwSession(
    VOID
    )
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
        diskEvent.Type = -1;

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

        if (diskEvent.Type != -1)
        {
            DiskIo_TypeGroup1 *data = EventTrace->MofData;

            if (WindowsVersion >= WINDOWS_8)
            {
                diskEvent.ClientId.UniqueThread = UlongToHandle(data->IssuingThreadId);
                diskEvent.ClientId.UniqueProcess = EtThreadIdToProcessId(diskEvent.ClientId.UniqueThread);
            }
            else
            {
                if (EventTrace->Header.ProcessId != -1)
                {
                    diskEvent.ClientId.UniqueProcess = UlongToHandle(EventTrace->Header.ProcessId);
                    diskEvent.ClientId.UniqueThread = UlongToHandle(EventTrace->Header.ThreadId);
                }
            }

            diskEvent.IrpFlags = data->IrpFlags;
            diskEvent.TransferSize = data->TransferSize;
            diskEvent.FileObject = (PVOID)data->FileObject;
            diskEvent.HighResResponseTime = data->HighResResponseTime;

            EtProcessDiskEvent(&diskEvent);
            EtDiskProcessDiskEvent(&diskEvent);
        }
    }
    else if (memcmp(&EventTrace->Header.Guid, &FileIoGuid_I, sizeof(GUID)) == 0)
    {
        // FileIo

        ET_ETW_FILE_EVENT fileEvent;

        memset(&fileEvent, 0, sizeof(ET_ETW_FILE_EVENT));
        fileEvent.Type = -1;

        switch (EventTrace->Header.Class.Type)
        {
        case 0: // Name
            fileEvent.Type = EtEtwFileNameType;
            break;
        case 32: // FileCreate
            fileEvent.Type = EtEtwFileCreateType;
            break;
        case 35: // FileDelete
            fileEvent.Type = EtEtwFileDeleteType;
            break;
        default:
            break;
        }

        if (fileEvent.Type != -1)
        {
            FileIo_Name *data = EventTrace->MofData;

            fileEvent.FileObject = (PVOID)data->FileObject;
            PhInitializeStringRef(&fileEvent.FileName, data->FileName);

            EtDiskProcessFileEvent(&fileEvent);
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
        networkEvent.Type = -1;

        switch (EventTrace->Header.Class.Type)
        {
        case EVENT_TRACE_TYPE_SEND: // send
            networkEvent.Type = EtEtwNetworkSendType;
            networkEvent.ProtocolType = PH_IPV4_NETWORK_TYPE;
            break;
        case EVENT_TRACE_TYPE_RECEIVE: // receive
            networkEvent.Type = EtEtwNetworkReceiveType;
            networkEvent.ProtocolType = PH_IPV4_NETWORK_TYPE;
            break;
        case EVENT_TRACE_TYPE_SEND + 16: // send ipv6
            networkEvent.Type = EtEtwNetworkSendType;
            networkEvent.ProtocolType = PH_IPV6_NETWORK_TYPE;
            break;
        case EVENT_TRACE_TYPE_RECEIVE + 16: // receive ipv6
            networkEvent.Type = EtEtwNetworkReceiveType;
            networkEvent.ProtocolType = PH_IPV6_NETWORK_TYPE;
            break;
        }

        if (memcmp(&EventTrace->Header.Guid, &TcpIpGuid_I, sizeof(GUID)) == 0)
            networkEvent.ProtocolType |= PH_TCP_PROTOCOL_TYPE;
        else
            networkEvent.ProtocolType |= PH_UDP_PROTOCOL_TYPE;

        if (networkEvent.Type != -1)
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

NTSTATUS EtpEtwMonitorThreadStart(
    __in PVOID Parameter
    )
{
    ULONG result;
    EVENT_TRACE_LOGFILE logFile;
    TRACEHANDLE traceHandle;

    memset(&logFile, 0, sizeof(EVENT_TRACE_LOGFILE));
    logFile.LoggerName = EtpLoggerName.Buffer;
    logFile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME;
    logFile.BufferCallback = EtpEtwBufferCallback;
    logFile.EventCallback = EtpEtwEventCallback;

    while (TRUE)
    {
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
        if (!EtpEtwActive)
            Sleep(250);
    }

    return STATUS_SUCCESS;
}

ULONG EtStartEtwRundown(
    VOID
    )
{
    ULONG result;
    ULONG bufferSize;

    bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + EtpRundownLoggerName.Length + sizeof(WCHAR);

    if (!EtpRundownTraceProperties)
        EtpRundownTraceProperties = PhAllocate(bufferSize);

    memset(EtpRundownTraceProperties, 0, sizeof(EVENT_TRACE_PROPERTIES));

    EtpRundownTraceProperties->Wnode.BufferSize = bufferSize;
    EtpRundownTraceProperties->Wnode.ClientContext = 1;
    EtpRundownTraceProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    EtpRundownTraceProperties->MinimumBuffers = 1;
    EtpRundownTraceProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    EtpRundownTraceProperties->FlushTimer = 1;
    EtpRundownTraceProperties->LogFileNameOffset = 0;
    EtpRundownTraceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

    result = StartTrace(&EtpRundownSessionHandle, EtpRundownLoggerName.Buffer, EtpRundownTraceProperties);

    if (result == ERROR_ALREADY_EXISTS)
    {
        EtpRundownTraceProperties->LogFileNameOffset = 0;
        ControlTrace(0, EtpRundownLoggerName.Buffer, EtpRundownTraceProperties, EVENT_TRACE_CONTROL_STOP);
        // ControlTrace screws up the structure.
        EtpRundownTraceProperties->Wnode.BufferSize = bufferSize;
        EtpRundownTraceProperties->LogFileNameOffset = 0;
        EtpRundownTraceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
        result = StartTrace(&EtpRundownSessionHandle, EtpRundownLoggerName.Buffer, EtpRundownTraceProperties);
    }

    if (result != ERROR_SUCCESS)
        return result;

    result = EnableTraceEx(&KernelRundownGuid_I, NULL, EtpRundownSessionHandle, 1, 0, 0x10, 0, 0, NULL);

    if (result != ERROR_SUCCESS)
    {
        EtpRundownTraceProperties->LogFileNameOffset = 0;
        ControlTrace(0, EtpRundownLoggerName.Buffer, EtpRundownTraceProperties, EVENT_TRACE_CONTROL_STOP);
        return result;
    }

    EtpRundownActive = TRUE;
    EtpRundownEtwMonitorThreadHandle = PhCreateThread(0, EtpRundownEtwMonitorThreadStart, NULL);

    return result;
}

ULONG NTAPI EtpRundownEtwBufferCallback(
    __in PEVENT_TRACE_LOGFILE Buffer
    )
{
    return !EtpEtwExiting;
}

VOID NTAPI EtpRundownEtwEventCallback(
    __in PEVENT_RECORD EventRecord
    )
{
    // TODO: Find a way to call CloseTrace when the enumeration finishes so we can
    // stop the trace cleanly.

    if (memcmp(&EventRecord->EventHeader.ProviderId, &FileIoGuid_I, sizeof(GUID)) == 0)
    {
        // FileIo

        ET_ETW_FILE_EVENT fileEvent;

        memset(&fileEvent, 0, sizeof(ET_ETW_FILE_EVENT));
        fileEvent.Type = -1;

        switch (EventRecord->EventHeader.EventDescriptor.Opcode)
        {
        case 36: // FileRundown
            fileEvent.Type = EtEtwFileRundownType;
            break;
        default:
            break;
        }

        if (fileEvent.Type != -1)
        {
            FileIo_Name *data = EventRecord->UserData;

            fileEvent.FileObject = (PVOID)data->FileObject;
            PhInitializeStringRef(&fileEvent.FileName, data->FileName);

            EtDiskProcessFileEvent(&fileEvent);
        }
    }
}

NTSTATUS EtpRundownEtwMonitorThreadStart(
    __in PVOID Parameter
    )
{
    EVENT_TRACE_LOGFILE logFile;
    TRACEHANDLE traceHandle;

    memset(&logFile, 0, sizeof(EVENT_TRACE_LOGFILE));
    logFile.LoggerName = EtpRundownLoggerName.Buffer;
    logFile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
    logFile.BufferCallback = EtpRundownEtwBufferCallback;
    logFile.EventRecordCallback = EtpRundownEtwEventCallback;
    logFile.Context = &traceHandle;

    traceHandle = OpenTrace(&logFile);

    if (traceHandle != INVALID_PROCESSTRACE_HANDLE)
    {
        ProcessTrace(&traceHandle, 1, NULL, NULL);

        if (traceHandle != 0)
            CloseTrace(traceHandle);

        EtpRundownTraceProperties->LogFileNameOffset = 0;
        ControlTrace(0, EtpRundownLoggerName.Buffer, EtpRundownTraceProperties, EVENT_TRACE_CONTROL_STOP);
    }

    NtClose(EtpRundownEtwMonitorThreadHandle);
    EtpRundownEtwMonitorThreadHandle = NULL;

    return STATUS_SUCCESS;
}
