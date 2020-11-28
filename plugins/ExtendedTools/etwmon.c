/*
 * Process Hacker Extended Tools -
 *   ETW monitoring
 *
 * Copyright (C) 2010-2015 wj32
 * Copyright (C) 2019-2020 dmex
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

static GUID ProcessHackerGuid = { 0x1288c53b, 0xaf35, 0x481b, { 0xb6, 0xb5, 0xa0, 0x5c, 0x39, 0x87, 0x2e, 0xd } };
static GUID SystemTraceControlGuid_I = { 0x9e814aad, 0x3204, 0x11d2, { 0x9a, 0x82, 0x00, 0x60, 0x08, 0xa8, 0x69, 0x39 } };
static GUID KernelRundownGuid_I = { 0x3b9c9951, 0x3480, 0x4220, { 0x93, 0x77, 0x9c, 0x8e, 0x51, 0x84, 0xf5, 0xcd } };
static GUID DiskIoGuid_I = { 0x3d6fa8d4, 0xfe05, 0x11d0, { 0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c } };
static GUID FileIoGuid_I = { 0x90cbdc39, 0x4a3e, 0x11d1, { 0x84, 0xf4, 0x00, 0x00, 0xf8, 0x04, 0x64, 0xe3 } };
static GUID TcpIpGuid_I = { 0x9a280ac0, 0xc8e0, 0x11d1, { 0x84, 0xe2, 0x00, 0xc0, 0x4f, 0xb9, 0x98, 0xa2 } };
static GUID UdpIpGuid_I = { 0xbf3a50c5, 0xa9c9, 0x4988, { 0xa0, 0x05, 0x2d, 0xf0, 0xb7, 0xc8, 0x0f, 0x80 } };

// ETW tracing layer

BOOLEAN EtEtwEnabled = FALSE;
ULONG EtEtwStatus = ERROR_SUCCESS;
ULONG EtEtwRundownStatus = ERROR_SUCCESS;
static UNICODE_STRING EtpSharedKernelLoggerName = RTL_CONSTANT_STRING(KERNEL_LOGGER_NAME);
static UNICODE_STRING EtpPrivateKernelLoggerName = RTL_CONSTANT_STRING(L"PhEtKernelLogger");
static TRACEHANDLE EtSessionHandle = INVALID_PROCESSTRACE_HANDLE;
static PUNICODE_STRING EtpActualKernelLoggerName = NULL;
static PGUID EtpActualSessionGuid = NULL;
static PEVENT_TRACE_PROPERTIES EtpTraceProperties = NULL;
static BOOLEAN EtpEtwActive = FALSE;
static BOOLEAN EtpEtwRundownActive = FALSE;
static BOOLEAN EtpEtwExiting = FALSE;

// ETW rundown layer

static UNICODE_STRING EtpRundownLoggerName = RTL_CONSTANT_STRING(L"PhEtRundownLogger");
static TRACEHANDLE EtpRundownSessionHandle = INVALID_PROCESSTRACE_HANDLE;
static PEVENT_TRACE_PROPERTIES EtpRundownTraceProperties = NULL;
static BOOLEAN EtpRundownActive = FALSE;

ULONG NTAPI EtEtwBufferCallback(
    _In_ PEVENT_TRACE_LOGFILE Buffer
    );
VOID NTAPI EtEtwEventCallback(
    _In_ PEVENT_RECORD EventRecord
    );
NTSTATUS EtEtwMonitorThreadStart(
    _In_ PVOID Parameter
    );
ULONG EtStopEtwSession(
    VOID
    );
ULONG EtStopEtwRundownSession(
    VOID
    );
NTSTATUS EtRundownEtwMonitorThreadStart(
    _In_ PVOID Parameter
    );

VOID EtEtwMonitorInitialization(
    VOID
    )
{
    if (PhGetOwnTokenAttributes().Elevated && PhGetIntegerSetting(SETTING_NAME_ENABLE_ETW_MONITOR))
    {
        ULONG bufferSize;

        if (PhWindowsVersion >= WINDOWS_8)
        {
            EtpActualKernelLoggerName = &EtpPrivateKernelLoggerName;
            EtpActualSessionGuid = &ProcessHackerGuid;
        }
        else
        {
            EtpActualKernelLoggerName = &EtpSharedKernelLoggerName;
            EtpActualSessionGuid = &SystemTraceControlGuid_I;
        }

        bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + EtpActualKernelLoggerName->Length + sizeof(UNICODE_NULL);
        EtpTraceProperties = PhAllocate(bufferSize);
        memset(EtpTraceProperties, 0, sizeof(EVENT_TRACE_PROPERTIES));

        EtpTraceProperties->Wnode.BufferSize = bufferSize;
        EtpTraceProperties->Wnode.Guid = *EtpActualSessionGuid;
        EtpTraceProperties->Wnode.ClientContext = 1;
        EtpTraceProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
        EtpTraceProperties->MinimumBuffers = 1;
        EtpTraceProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
        EtpTraceProperties->FlushTimer = 1;
        EtpTraceProperties->EnableFlags = EVENT_TRACE_FLAG_DISK_IO | EVENT_TRACE_FLAG_DISK_FILE_IO | EVENT_TRACE_FLAG_NETWORK_TCPIP;
        EtpTraceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

        if (PhWindowsVersion >= WINDOWS_8)
            EtpTraceProperties->LogFileMode |= EVENT_TRACE_SYSTEM_LOGGER_MODE;

        bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + EtpRundownLoggerName.Length + sizeof(UNICODE_NULL);
        EtpRundownTraceProperties = PhAllocate(bufferSize);
        memset(EtpRundownTraceProperties, 0, sizeof(EVENT_TRACE_PROPERTIES));

        EtpRundownTraceProperties->Wnode.BufferSize = bufferSize;
        EtpRundownTraceProperties->Wnode.ClientContext = 1;
        EtpRundownTraceProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
        EtpRundownTraceProperties->MinimumBuffers = 1;
        EtpRundownTraceProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
        EtpRundownTraceProperties->FlushTimer = 1;
        EtpRundownTraceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

        EtpRundownActive = TRUE;
        EtEtwEnabled = TRUE;

        PhCreateThread2(EtRundownEtwMonitorThreadStart, NULL);
        PhCreateThread2(EtEtwMonitorThreadStart, NULL);
    }
}

VOID EtEtwMonitorUninitialization(
    VOID
    )
{
    if (PhGetOwnTokenAttributes().Elevated && PhGetIntegerSetting(SETTING_NAME_ENABLE_ETW_MONITOR))
    {
        EtpEtwExiting = TRUE;

        if (EtpEtwActive)
        {
            EtStopEtwSession();
        }

        if (EtpEtwRundownActive)
        {
            EtStopEtwRundownSession();
        }
    }
}

TRACEHANDLE EtOpenEtwTrace(
    VOID
    )
{
    TRACEHANDLE traceHandle = INVALID_PROCESSTRACE_HANDLE;
    EVENT_TRACE_LOGFILE logFile;

    if (!(EtpTraceProperties && EtpActualKernelLoggerName))
        return INVALID_PROCESSTRACE_HANDLE;

    memset(&logFile, 0, sizeof(EVENT_TRACE_LOGFILE));
    logFile.LoggerName = EtpActualKernelLoggerName->Buffer;
    logFile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
    logFile.BufferCallback = EtEtwBufferCallback;
    logFile.EventRecordCallback = EtEtwEventCallback;

    traceHandle = OpenTrace(&logFile);

    if (traceHandle != INVALID_PROCESSTRACE_HANDLE)
    {
        EtSessionHandle = traceHandle;
        EtpEtwActive = TRUE;
    }
    else
    {
        EtpEtwActive = FALSE;
        EtSessionHandle = INVALID_PROCESSTRACE_HANDLE;
    }

    return traceHandle;
}

VOID EtStartEtwSession(
    VOID
    )
{
    TRACEHANDLE traceHandle = INVALID_PROCESSTRACE_HANDLE;

    EtpTraceProperties->LogFileNameOffset = 0;
    EtEtwStatus = StartTrace(
        &traceHandle,
        EtpActualKernelLoggerName->Buffer,
        EtpTraceProperties
        );

    if (EtEtwStatus == ERROR_ALREADY_EXISTS)
    {
        // Session already exists, so use that. Get the existing session handle.
        EtpTraceProperties->LogFileNameOffset = 0;
        EtEtwStatus = ControlTrace(
            0,
            EtpActualKernelLoggerName->Buffer,
            EtpTraceProperties,
            EVENT_TRACE_CONTROL_QUERY
            );

        if (EtEtwStatus == ERROR_SUCCESS)
        {
            traceHandle = EtpTraceProperties->Wnode.HistoricalContext;
        }
    }

    if (EtEtwStatus == ERROR_SUCCESS)
    {
        EtSessionHandle = traceHandle;
        EtpEtwActive = TRUE;
    }
    else
    {
        EtpEtwActive = FALSE;
        EtSessionHandle = INVALID_PROCESSTRACE_HANDLE;
    }
}

ULONG EtStopEtwSession(
    VOID
    )
{
    if (!(EtpTraceProperties && EtpActualKernelLoggerName))
        return 0;

    EtpTraceProperties->LogFileNameOffset = 0;
    return ControlTrace(
        EtSessionHandle,
        EtpActualKernelLoggerName->Buffer,
        EtpTraceProperties,
        EVENT_TRACE_CONTROL_STOP
        );
}

ULONG EtEtwControlEtwSession(
    _In_ ULONG ControlCode
    )
{
    if (!(EtpTraceProperties && EtpActualKernelLoggerName))
        return 0;

    // If we have a session handle, we use that instead of the logger name. (wj32)
    EtpTraceProperties->LogFileNameOffset = 0;
    return ControlTrace(
        EtSessionHandle,
        EtpActualKernelLoggerName->Buffer,
        EtpTraceProperties,
        ControlCode
        );
}

VOID EtFlushEtwSession(
    VOID
    )
{
    if (EtSessionHandle == INVALID_PROCESSTRACE_HANDLE)
        return;

    // Note: Using FLUSH controlcode to flush the session instead of the trace (e.g. when EtSessionHandle is NULL)
    // causes memory/handle leaks starting with Windows 10. The ControlTraceW function will allocate a
    // seperate trace session with GUID {3595ab5c-042a-4c8e-b942-2d059bfeb1b1} for the
    // PrivateLoggerNotificationGuid forgetting to cleanup afterwards, creating new trace sessions and
    // new handles during every call to ControlTraceW. Our default flush interval is 1-sec so this bug would leak
    // an average 60 handles a second... We don't currently flush the session (only the trace)
    // so make sure EtSessionHandle is valid before calling FLUSH. (dmex)
    EtEtwControlEtwSession(EVENT_TRACE_CONTROL_FLUSH);
}

ULONG NTAPI EtEtwBufferCallback(
    _In_ PEVENT_TRACE_LOGFILE Buffer
    )
{
    return !EtpEtwExiting;
}

VOID NTAPI EtEtwEventCallback(
    _In_ PEVENT_RECORD EventRecord
    )
{
    if (EtpEtwExiting)
        return;

    if (IsEqualGUID(&EventRecord->EventHeader.ProviderId, &DiskIoGuid_I))
    {
        // DiskIo

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
        default:
            break;
        }

        if (diskEvent.Type != ULONG_MAX)
        {
            DiskIo_TypeGroup1 *data = EventRecord->UserData;

            if (PhWindowsVersion >= WINDOWS_8)
            {
                diskEvent.ClientId.UniqueThread = UlongToHandle(data->IssuingThreadId);
                diskEvent.ClientId.UniqueProcess = EtThreadIdToProcessId(diskEvent.ClientId.UniqueThread);
            }
            else
            {
                if (EventRecord->EventHeader.ProcessId != ULONG_MAX)
                {
                    diskEvent.ClientId.UniqueProcess = UlongToHandle(EventRecord->EventHeader.ProcessId);
                    diskEvent.ClientId.UniqueThread = UlongToHandle(EventRecord->EventHeader.ThreadId);
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
    else if (IsEqualGUID(&EventRecord->EventHeader.ProviderId, &FileIoGuid_I))
    {
        // FileIo

        ET_ETW_FILE_EVENT fileEvent;

        memset(&fileEvent, 0, sizeof(ET_ETW_FILE_EVENT));
        fileEvent.Type = ULONG_MAX;

        switch (EventRecord->EventHeader.EventDescriptor.Opcode)
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
        case 36: // FileRundown
            fileEvent.Type = EtEtwFileRundownType;
            break;
        }

        if (fileEvent.Type != ULONG_MAX)
        {
            if (PhIsExecutingInWow64())
            {
                FileIo_Name_Wow64 *dataWow64 = EventRecord->UserData;

                fileEvent.FileObject = (PVOID)dataWow64->FileObject;
                PhInitializeStringRef(&fileEvent.FileName, dataWow64->FileName);
            }
            else
            {
                FileIo_Name *data = EventRecord->UserData;

                fileEvent.FileObject = (PVOID)data->FileObject;
                PhInitializeStringRef(&fileEvent.FileName, data->FileName);
            }

            EtDiskProcessFileEvent(&fileEvent);
        }
    }
    else if (
        IsEqualGUID(&EventRecord->EventHeader.ProviderId, &TcpIpGuid_I) ||
        IsEqualGUID(&EventRecord->EventHeader.ProviderId, &UdpIpGuid_I)
        )
    {
        // TcpIp/UdpIp

        ET_ETW_NETWORK_EVENT networkEvent;

        memset(&networkEvent, 0, sizeof(ET_ETW_NETWORK_EVENT));
        networkEvent.Type = ULONG_MAX;

        switch (EventRecord->EventHeader.EventDescriptor.Opcode)
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

        if (IsEqualGUID(&EventRecord->EventHeader.ProviderId, &TcpIpGuid_I))
            networkEvent.ProtocolType |= PH_TCP_PROTOCOL_TYPE;
        else
            networkEvent.ProtocolType |= PH_UDP_PROTOCOL_TYPE;

        if (networkEvent.Type != ULONG_MAX)
        {
            PH_IP_ENDPOINT source;
            PH_IP_ENDPOINT destination;

            if (networkEvent.ProtocolType & PH_IPV4_NETWORK_TYPE)
            {
                TcpIpOrUdpIp_IPV4_Header *data = EventRecord->UserData;

                networkEvent.ClientId.UniqueProcess = UlongToHandle(data->PID);
                networkEvent.TransferSize = data->size;

                source.Address.Type = PH_IPV4_NETWORK_TYPE;
                source.Address.Ipv4 = data->saddr.s_addr;
                source.Port = _byteswap_ushort(data->sport);
                destination.Address.Type = PH_IPV4_NETWORK_TYPE;
                destination.Address.Ipv4 = data->daddr.s_addr;
                destination.Port = _byteswap_ushort(data->dport);
            }
            else if (networkEvent.ProtocolType & PH_IPV6_NETWORK_TYPE)
            {
                TcpIpOrUdpIp_IPV6_Header *data = EventRecord->UserData;

                networkEvent.ClientId.UniqueProcess = UlongToHandle(data->PID);
                networkEvent.TransferSize = data->size;

                source.Address.Type = PH_IPV6_NETWORK_TYPE;
                source.Address.In6Addr = data->saddr;
                source.Port = _byteswap_ushort(data->sport);
                destination.Address.Type = PH_IPV6_NETWORK_TYPE;
                destination.Address.In6Addr = data->daddr;
                destination.Port = _byteswap_ushort(data->dport);
            }

            // Note: The endpoints are swapped for incoming UDP packets. The destination endpoint
            // corresponds to the local socket not the source endpoint. (DavidXanatos)
            if ((networkEvent.ProtocolType & PH_UDP_PROTOCOL_TYPE) != 0 &&
                networkEvent.Type == EtEtwNetworkReceiveType)
            {
                PH_IP_ENDPOINT swapsource = source;
                source = destination;
                destination = swapsource;
            }

            networkEvent.LocalEndpoint = source;

            if (networkEvent.ProtocolType & PH_TCP_PROTOCOL_TYPE)
                networkEvent.RemoteEndpoint = destination;

            EtProcessNetworkEvent(&networkEvent);
        }
    }
}

NTSTATUS EtEtwMonitorThreadStart(
    _In_ PVOID Parameter
    )
{
    while (!EtpEtwExiting)
    {
        ULONG result = ERROR_SUCCESS;
        TRACEHANDLE traceHandle = EtOpenEtwTrace();

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
            PhDelayExecution(1000);
    }

    return STATUS_SUCCESS;
}

VOID EtStartEtwRundownSession(
    VOID
    )
{
    TRACEHANDLE traceHandle = INVALID_PROCESSTRACE_HANDLE;

    EtpRundownTraceProperties->LogFileNameOffset = 0;
    EtEtwRundownStatus = StartTrace(
        &traceHandle,
        EtpRundownLoggerName.Buffer,
        EtpRundownTraceProperties
        );

    if (EtEtwRundownStatus == ERROR_ALREADY_EXISTS)
    {
        EtStopEtwRundownSession();

        EtpRundownTraceProperties->LogFileNameOffset = 0;
        EtEtwRundownStatus = StartTrace(
            &traceHandle,
            EtpRundownLoggerName.Buffer,
            EtpRundownTraceProperties
            );
    }

    if (EtEtwRundownStatus == ERROR_SUCCESS)
    {
        ENABLE_TRACE_PARAMETERS enableParameters;

        memset(&enableParameters, 0, sizeof(ENABLE_TRACE_PARAMETERS));
        enableParameters.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;

        EtEtwRundownStatus = EnableTraceEx2(
            traceHandle,
            &KernelRundownGuid_I,
            EVENT_CONTROL_CODE_ENABLE_PROVIDER,
            TRACE_LEVEL_VERBOSE,
            0x10,
            0,
            0,
            &enableParameters
            );
    }

    if (EtEtwRundownStatus == ERROR_SUCCESS)
    {
        EtpRundownSessionHandle = traceHandle;
        EtpEtwRundownActive = TRUE;
    }
    else
    {
        EtpEtwRundownActive = FALSE;
        EtpRundownSessionHandle = INVALID_PROCESSTRACE_HANDLE;
    }
}

TRACEHANDLE EtOpenEtwRundownTrace(
    VOID
    )
{
    EVENT_TRACE_LOGFILE logFile;

    memset(&logFile, 0, sizeof(EVENT_TRACE_LOGFILE));
    logFile.LoggerName = EtpRundownLoggerName.Buffer;
    logFile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
    logFile.BufferCallback = EtEtwBufferCallback;
    logFile.EventRecordCallback = EtEtwEventCallback;

    return OpenTrace(&logFile);
}

ULONG EtStopEtwRundownSession(
    VOID
    )
{
    if (!EtpRundownTraceProperties)
        return ERROR_SUCCESS;

    EtpRundownTraceProperties->LogFileNameOffset = 0;
    return ControlTrace(
        0,
        EtpRundownLoggerName.Buffer,
        EtpRundownTraceProperties,
        EVENT_TRACE_CONTROL_STOP
        );
}

ULONG EtFlushEtwRundownSession(
    VOID
    )
{
    if (!EtpRundownTraceProperties)
        return ERROR_SUCCESS;

    EtpRundownTraceProperties->LogFileNameOffset = 0;
    return ControlTrace(
        0,
        EtpRundownLoggerName.Buffer,
        EtpRundownTraceProperties,
        EVENT_TRACE_CONTROL_FLUSH
        );
}

// Note: Open/close the rundown provider since we're using a realtime session and
// the provider only enumerates open files at the end of the session. This works
// because once the rundown completes we don't restart the trace. (dmex)
NTSTATUS EtRundownEtwMonitorThreadStart(
    _In_ PVOID Parameter
    )
{
    TRACEHANDLE traceHandle;

    EtStopEtwRundownSession();
    EtStartEtwRundownSession();

    traceHandle = EtOpenEtwRundownTrace();

    if (traceHandle != INVALID_PROCESSTRACE_HANDLE)
    {
        EtpEtwRundownActive = TRUE;

        ProcessTrace(&traceHandle, 1, NULL, NULL);

        CloseTrace(traceHandle);
    }

    return STATUS_SUCCESS;
}
