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

#include "extendedtools.h"
#include <evntcons.h>

static PH_STRINGREF EtpLoggerName = PH_STRINGREF_INIT(L"PH2 ET");
static TRACEHANDLE EtpSessionHandle;
static TRACEHANDLE EtpTraceHandle;

ULONG NTAPI EtpEtwBufferCallback(
    __in PEVENT_TRACE_LOGFILE Buffer
    );

VOID NTAPI EtpEtwEventRecordCallback(
    __in PEVENT_RECORD EventRecord
    );

NTSTATUS EtEtwMonitorThreadStart(
    __in PVOID Parameter
    );

VOID EtpStartEtwSession()
{
    PEVENT_TRACE_PROPERTIES properties;
    ULONG bufferSize;

    bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + EtpLoggerName.Length;
    properties = PhAllocate(bufferSize);

    properties->Wnode.BufferSize = bufferSize;
    //properties->Wnode.Guid = ?;
    properties->LogFileNameOffset = 0;
    properties->LoggerNameOffset = 0;

    StartTrace(&EtpSessionHandle, EtpLoggerName.Buffer, &properties);
}

VOID EtpStartEtwTrace()
{
    EVENT_TRACE_LOGFILE logFile;

    memset(&logFile, 0, sizeof(EVENT_TRACE_LOGFILE));
    logFile.LoggerName = EtpLoggerName.Buffer;
    logFile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME;
    logFile.BufferCallback = EtpEtwBufferCallback;
    logFile.EventRecordCallback = EtpEtwEventRecordCallback;
    logFile.IsKernelTrace = FALSE;

    EtpTraceHandle = OpenTrace(&logFile);
}

ULONG NTAPI EtpEtwBufferCallback(
    __in PEVENT_TRACE_LOGFILE Buffer
    )
{
    return TRUE;
}

VOID NTAPI EtpEtwEventRecordCallback(
    __in PEVENT_RECORD EventRecord
    )
{
    EtpStartEtwMonitor();

    while (ProcessTrace(&EtpTraceHandle, 1, NULL, NULL) == ERROR_SUCCESS)
    {
        
    }

    CloseTrace(EtpTraceHandle);
}

NTSTATUS EtEtwMonitorThreadStart(
    __in PVOID Parameter
    )
{
    
}
