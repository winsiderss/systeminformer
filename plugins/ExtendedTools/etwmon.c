/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2019-2023
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

NTSTATUS EtpRundownEtwMonitorThreadStart(
    _In_ PVOID Parameter
    );

// 3875f5e7-8f79-406c-8cb9-ee8fd8bfcfbd
static GUID SystemInformerGuid = { 0x3875f5e7, 0x8f79, 0x406c, { 0x8c, 0xb9, 0xee, 0x8f, 0xd8, 0xbf, 0xcf, 0xbd } };
// 9e814aad-3204-11d2-9a82-006008a86939
static GUID SystemTraceControlGuid_I = { 0x9e814aad, 0x3204, 0x11d2, { 0x9a, 0x82, 0x00, 0x60, 0x08, 0xa8, 0x69, 0x39 } };
// 3b9c9951-3480-4220-9377-9c8e5184f5cd
static GUID KernelRundownGuid_I = { 0x3b9c9951, 0x3480, 0x4220, { 0x93, 0x77, 0x9c, 0x8e, 0x51, 0x84, 0xf5, 0xcd } };
// 3d6fa8d4-fe05-11d0-9dda-00c04fd7ba7c
static GUID DiskIoGuid_I = { 0x3d6fa8d4, 0xfe05, 0x11d0, { 0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c } };
// 90cbdc39-4a3e-11d1-84f4-0000f80464e3
static GUID FileIoGuid_I = { 0x90cbdc39, 0x4a3e, 0x11d1, { 0x84, 0xf4, 0x00, 0x00, 0xf8, 0x04, 0x64, 0xe3 } };
// 9a280ac0-c8e0-11d1-84e2-00c04fb998a2
static GUID TcpIpGuid_I = { 0x9a280ac0, 0xc8e0, 0x11d1, { 0x84, 0xe2, 0x00, 0xc0, 0x4f, 0xb9, 0x98, 0xa2 } };
// bf3a50c5-a9c9-4988-a005-2df0b7c80f80
static GUID UdpIpGuid_I = { 0xbf3a50c5, 0xa9c9, 0x4988, { 0xa0, 0x05, 0x2d, 0xf0, 0xb7, 0xc8, 0x0f, 0x80 } };
// def2fe46-7bd6-4b80-bd94-f57fe20d0ce3
static GUID StackWalkGuid_I = { 0xdef2fe46, 0x7bd6, 0x4b80, { 0xbd, 0x94, 0xf5, 0x7f, 0xe2, 0x0d, 0x0c, 0xe3 } };

// ETW tracing layer

BOOLEAN EtEtwEnabled = FALSE;
ULONG EtEtwStatus = ERROR_SUCCESS;
static UNICODE_STRING EtpSharedKernelLoggerName = RTL_CONSTANT_STRING(KERNEL_LOGGER_NAME);
static UNICODE_STRING EtpPrivateKernelLoggerName = RTL_CONSTANT_STRING(L"SiKernelTraceSession");
static TRACEHANDLE EtpSessionHandle = INVALID_PROCESSTRACE_HANDLE;
static PUNICODE_STRING EtpActualKernelLoggerName = NULL;
static PGUID EtpActualSessionGuid = NULL;
static UCHAR EtpTracePropertiesBuffer[sizeof(EVENT_TRACE_PROPERTIES) + max(sizeof(KERNEL_LOGGER_NAME), sizeof(L"SiKernelTraceSession"))];
static PEVENT_TRACE_PROPERTIES EtpTraceProperties = (PEVENT_TRACE_PROPERTIES)EtpTracePropertiesBuffer;
static BOOLEAN EtpEtwActive = FALSE;
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
        EtStartEtwSession();

        if (EtEtwEnabled)
        {
            PhCreateThread2(EtpEtwMonitorThreadStart, NULL);
        }
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
    ULONG bufferSize;

    if (PhWindowsVersion >= WINDOWS_8)
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

    if (PhWindowsVersion >= WINDOWS_8)
        EtpTraceProperties->LogFileMode |= EVENT_TRACE_SYSTEM_LOGGER_MODE;

    // Get the existing session handle.
    EtEtwStatus = ControlTrace(
        0,
        EtpActualKernelLoggerName->Buffer,
        EtpTraceProperties,
        EVENT_TRACE_CONTROL_QUERY
        );

    if (EtEtwStatus == ERROR_SUCCESS)
    {
        EtpSessionHandle = EtpTraceProperties->Wnode.HistoricalContext;
    }
    else
    {
        EtpTraceProperties->LogFileNameOffset = 0;
        EtEtwStatus = StartTrace(
            &EtpSessionHandle,
            EtpActualKernelLoggerName->Buffer,
            EtpTraceProperties
            );
    }

    // Enable stack tracing.
    // NOTE: This only enables stack traces for SystemTraceControlGuid events while the
    // EVENT_ENABLE_PROPERTY_STACK_TRACE flag must be used for other guids. (dmex)
    //if (PhWindowsVersion >= WINDOWS_8 && EtEtwStatus == ERROR_SUCCESS)
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
    //    EtEtwStatus = PhNtStatusToDosError(NtSetSystemInformation(
    //        SystemPerformanceTraceInformation,
    //        eventTraceStackTracingInfo,
    //        sizeof(eventBuffer)
    //        ));
    //}
    //
    // Enable trace flags. (dmex)
    //if (PhWindowsVersion >= WINDOWS_8 && EtEtwStatus == ERROR_SUCCESS)
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
    //    EtEtwStatus = PhNtStatusToDosError(NtSetSystemInformation(
    //        SystemPerformanceTraceInformation,
    //        &eventTraceGroupMaskInfo,
    //        sizeof(EVENT_TRACE_GROUPMASK_INFORMATION)
    //        ));
    //}

    if (EtEtwStatus == ERROR_SUCCESS)
    {
        EtEtwEnabled = TRUE;
        EtpEtwActive = TRUE;
        EtpStartedSession = TRUE;
    }
    else if (EtEtwStatus == ERROR_ALREADY_EXISTS)
    {
        EtEtwEnabled = TRUE;
        EtpEtwActive = TRUE;
        EtpStartedSession = FALSE;
        // The session already exists.
        //EtEtwStatus = ControlTrace(0, EtpActualKernelLoggerName->Buffer, EtpTraceProperties, EVENT_TRACE_CONTROL_UPDATE);
    }
    else
    {
        EtpEtwActive = FALSE;
        EtpStartedSession = FALSE;
        EtpSessionHandle = INVALID_PROCESSTRACE_HANDLE; // StartTrace set the handle 0 on failure. (dmex)
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
    if (EtEtwEnabled)
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
            DiskIo_TypeGroup1* data = EventRecord->UserData;

            if (PhWindowsVersion >= WINDOWS_8)
            {
                if (data->IssuingThreadId != ULONG_MAX)
                {
                    diskEvent.ClientId.UniqueThread = UlongToHandle(data->IssuingThreadId);
                    diskEvent.ClientId.UniqueProcess = EtThreadIdToProcessId(diskEvent.ClientId.UniqueThread);
                }
                else
                {
                    diskEvent.ClientId.UniqueThread = 0;
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
                    diskEvent.ClientId.UniqueThread = 0;
                    diskEvent.ClientId.UniqueProcess = SYSTEM_PROCESS_ID;
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
            if (PhIsExecutingInWow64())
            {
                if (EventRecord->EventHeader.EventDescriptor.Version == 2)
                {
                    ULONG fileNameLength = (EventRecord->UserDataLength - UFIELD_OFFSET(FileIo_Name_Wow64, FileName)) - sizeof(UNICODE_NULL);
                    FileIo_Name_Wow64* dataWow64 = EventRecord->UserData;

                    fileEvent.FileObject = (PVOID)dataWow64->FileObject;
                    fileEvent.FileName.Length = fileNameLength;
                    fileEvent.FileName.Buffer = dataWow64->FileName;
                }
                else
                {
                    FileIo_Name_Wow64* dataWow64 = EventRecord->UserData;

                    fileEvent.FileObject = (PVOID)dataWow64->FileObject;
                    PhInitializeStringRefLongHint(&fileEvent.FileName, dataWow64->FileName);
                }
            }
            else
            {
                if (EventRecord->EventHeader.EventDescriptor.Version == 2)
                {
                    ULONG fileNameLength = (EventRecord->UserDataLength - UFIELD_OFFSET(FileIo_Name, FileName)) - sizeof(UNICODE_NULL);
                    FileIo_Name* data = EventRecord->UserData;

                    fileEvent.FileObject = (PVOID)data->FileObject;
                    fileEvent.FileName.Length = fileNameLength;
                    fileEvent.FileName.Buffer = data->FileName;
                }
                else
                {
                    FileIo_Name* data = EventRecord->UserData;

                    fileEvent.FileObject = (PVOID)data->FileObject;
                    PhInitializeStringRefLongHint(&fileEvent.FileName, data->FileName);
                }
            }

            EtDiskProcessFileEvent(&fileEvent);
        }
    }
    else if (
        IsEqualGUID(&EventRecord->EventHeader.ProviderId, &TcpIpGuid_I) ||
        IsEqualGUID(&EventRecord->EventHeader.ProviderId, &UdpIpGuid_I)
        )
    {
        ET_ETW_NETWORK_EVENT networkEvent;

        memset(&networkEvent, 0, sizeof(ET_ETW_NETWORK_EVENT));
        networkEvent.Type = ULONG_MAX;

        switch (EventRecord->EventHeader.EventDescriptor.Opcode)
        {
        case EVENT_TRACE_TYPE_SEND:
            networkEvent.Type = EtEtwNetworkSendType;
            networkEvent.ProtocolType = PH_IPV4_NETWORK_TYPE;
            break;
        case EVENT_TRACE_TYPE_RECEIVE:
            networkEvent.Type = EtEtwNetworkReceiveType;
            networkEvent.ProtocolType = PH_IPV4_NETWORK_TYPE;
            break;
        case EVENT_TRACE_TYPE_TCPIP_SEND_IPV6:
            networkEvent.Type = EtEtwNetworkSendType;
            networkEvent.ProtocolType = PH_IPV6_NETWORK_TYPE;
            break;
        case EVENT_TRACE_TYPE_TCPIP_RECEIVE_IPV6:
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
                TcpIpOrUdpIp_IPV4_Header* data = EventRecord->UserData;

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
                TcpIpOrUdpIp_IPV6_Header* data = EventRecord->UserData;

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

NTSTATUS EtpEtwMonitorThreadStart(
    _In_ PVOID Parameter
    )
{
    ULONG result;
    EVENT_TRACE_LOGFILE logFile;
    TRACEHANDLE traceHandle;

    PhSetThreadName(NtCurrentThread(), L"EtwMonitorThread");

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
        if (!EtpEtwActive)
            PhDelayExecution(250);
    }

    return STATUS_SUCCESS;
}

ULONG EtStartEtwRundown(
    VOID
    )
{
    ULONG result;
    ULONG bufferSize;

    if (PhWindowsVersion >= WINDOWS_8 && EtEtwEnabled && EtpSessionHandle != INVALID_PROCESSTRACE_HANDLE)
    {
        // Note: Enable the filename rundown in our existing trace session 
        // without creating a seperate trace session/thread. If this returns an
        // error then we'll fallback to creating a sepeate trace session/thread. (dmex)
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
            return result;
    }

    bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + EtpRundownLoggerName.Length + sizeof(UNICODE_NULL);

    memset(EtpRundownTraceProperties, 0, sizeof(EtpRundownTracePropertiesBuffer));
    EtpRundownTraceProperties->Wnode.BufferSize = bufferSize;
    EtpRundownTraceProperties->Wnode.ClientContext = 1;
    EtpRundownTraceProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    EtpRundownTraceProperties->MinimumBuffers = 1;
    EtpRundownTraceProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    EtpRundownTraceProperties->FlushTimer = 1;
    EtpRundownTraceProperties->EnableFlags = EVENT_TRACE_FLAG_NO_SYSCONFIG;
    EtpRundownTraceProperties->LogFileNameOffset = 0;
    EtpRundownTraceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

    // Try get the existing session handle.
    result = ControlTrace(
        0,
        EtpRundownLoggerName.Buffer,
        EtpRundownTraceProperties,
        EVENT_TRACE_CONTROL_QUERY
        );

    if (result == ERROR_SUCCESS)
    {
        EtpStopEtwRundownSession();

        EtpRundownTraceProperties->LogFileNameOffset = 0;
        result = StartTrace(
            &EtpRundownSessionHandle,
            EtpRundownLoggerName.Buffer,
            EtpRundownTraceProperties
            );
    }
    else
    {
        EtpRundownTraceProperties->LogFileNameOffset = 0;
        result = StartTrace(
            &EtpRundownSessionHandle,
            EtpRundownLoggerName.Buffer,
            EtpRundownTraceProperties
            );
    }

    if (result != ERROR_SUCCESS)
    {
        EtpRundownSessionHandle = INVALID_PROCESSTRACE_HANDLE; // StartTrace set the handle 0 on failure. (dmex)
        return result;
    }

    EtpRundownActive = TRUE;
    PhCreateThread2(EtpRundownEtwMonitorThreadStart, NULL);

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
            if (PhIsExecutingInWow64())
            {
                FileIo_Name_Wow64* dataWow64 = EventRecord->UserData;

                fileEvent.FileObject = (PVOID)dataWow64->FileObject;
                PhInitializeStringRefLongHint(&fileEvent.FileName, dataWow64->FileName);
            }
            else
            {
                FileIo_Name* data = EventRecord->UserData;

                fileEvent.FileObject = (PVOID)data->FileObject;
                PhInitializeStringRefLongHint(&fileEvent.FileName, data->FileName);
            }

            EtDiskProcessFileEvent(&fileEvent);
        }
    }
}

NTSTATUS EtpRundownEtwMonitorThreadStart(
    _In_ PVOID Parameter
    )
{
    EVENT_TRACE_LOGFILE logFile;
    TRACEHANDLE traceHandle;

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
