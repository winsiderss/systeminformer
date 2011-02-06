/*
 * Process Hacker - 
 *   phsvc client
 * 
 * Copyright (C) 2011 wj32
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

#include <phapp.h>
#include <phsvccl.h>

HANDLE PhSvcClPortHandle;
PVOID PhSvcClPortHeap;
HANDLE PhSvcClServerProcessId;

NTSTATUS PhSvcConnectToServer(
    __in PUNICODE_STRING PortName,
    __in_opt SIZE_T PortSectionSize
    )
{
    NTSTATUS status;
    HANDLE sectionHandle;
    LARGE_INTEGER sectionSize;
    PORT_VIEW clientView;
    REMOTE_PORT_VIEW serverView;
    SECURITY_QUALITY_OF_SERVICE securityQos;
    ULONG maxMessageLength;
    PHSVC_API_CONNECTINFO connectInfo;
    ULONG connectInfoLength;

    if (PortSectionSize == 0)
        PortSectionSize = 512 * 1024;

    // Create the port section and connect to the port.

    sectionSize.QuadPart = PortSectionSize;
    status = NtCreateSection(
        &sectionHandle,
        SECTION_ALL_ACCESS,
        NULL,
        &sectionSize,
        PAGE_READWRITE,
        SEC_COMMIT,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    clientView.Length = sizeof(PORT_VIEW);
    clientView.SectionHandle = sectionHandle;
    clientView.SectionOffset = 0;
    clientView.ViewSize = PortSectionSize;
    clientView.ViewBase = NULL;
    clientView.ViewRemoteBase = NULL;

    serverView.Length = sizeof(REMOTE_PORT_VIEW);
    serverView.ViewSize = 0;
    serverView.ViewBase = NULL;

    securityQos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    securityQos.ImpersonationLevel = SecurityImpersonation;
    securityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    securityQos.EffectiveOnly = TRUE;

    connectInfoLength = sizeof(PHSVC_API_CONNECTINFO);

    status = NtConnectPort(
        &PhSvcClPortHandle,
        PortName,
        &securityQos,
        &clientView,
        &serverView,
        &maxMessageLength,
        &connectInfo,
        &connectInfoLength
        );
    NtClose(sectionHandle);

    if (!NT_SUCCESS(status))
        return status;

    PhSvcClServerProcessId = connectInfo.ServerProcessId;

    // Create the port heap.

    PhSvcClPortHeap = RtlCreateHeap(
        HEAP_CLASS_1,
        clientView.ViewBase,
        clientView.ViewSize,
        PAGE_SIZE,
        NULL,
        NULL
        );

    if (!PhSvcClPortHeap)
    {
        NtClose(PhSvcClPortHandle);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}

VOID PhSvcDisconnectFromServer()
{
    if (PhSvcClPortHandle)
    {
        NtClose(PhSvcClPortHandle);
        PhSvcClPortHandle = NULL;
        PhSvcClPortHeap = NULL;
        PhSvcClServerProcessId = NULL;
    }
}

PVOID PhSvcpAllocateHeap(
    __in SIZE_T Size,
    __out PULONG Offset
    )
{
    PVOID memory;

    if (!PhSvcClPortHeap)
        return NULL;

    memory = RtlAllocateHeap(PhSvcClPortHeap, 0, Size);

    if (!memory)
        return NULL;

    *Offset = (ULONG)((ULONG_PTR)memory - (ULONG_PTR)PhSvcClPortHeap);

    return memory;
}

VOID PhSvcpFreeHeap(
    __in PVOID Memory
    )
{
    if (!PhSvcClPortHeap)
        return;

    RtlFreeHeap(PhSvcClPortHeap, 0, Memory);
}

PVOID PhSvcpCreateRelativeStringRef(
    __in PWSTR String,
    __in SIZE_T Length,
    __out PPH_RELATIVE_STRINGREF StringRef
    )
{
    PVOID memory;
    SIZE_T length;
    ULONG offset;

    if (Length != -1)
        length = Length;
    else
        length = wcslen(String) * sizeof(WCHAR);

    if (length > MAXULONG32)
        return NULL;

    memory = PhSvcpAllocateHeap(length, &offset);

    if (!memory)
        return NULL;

    memcpy(memory, String, length);

    StringRef->Length = (ULONG)length;
    StringRef->Offset = offset;

    return memory;
}

NTSTATUS PhSvcpCallServer(
    __inout PPHSVC_API_MSG Message
    )
{
    NTSTATUS status;

    Message->h.u1.s1.DataLength = sizeof(PHSVC_API_MSG) - FIELD_OFFSET(PHSVC_API_MSG, ApiNumber);
    Message->h.u1.s1.TotalLength = sizeof(PHSVC_API_MSG);
    Message->h.u2.ZeroInit = 0;

    status = NtRequestWaitReplyPort(PhSvcClPortHandle, &Message->h, &Message->h);

    if (!NT_SUCCESS(status))
        return status;

    return Message->ReturnStatus;
}

NTSTATUS PhSvcCallClose(
    __in HANDLE Handle
    )
{
    PHSVC_API_MSG m;

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.ApiNumber = PhSvcCloseApiNumber;
    m.u.Close.i.Handle = Handle;

    return PhSvcpCallServer(&m);
}

NTSTATUS PhSvcCallExecuteRunAsCommand(
    __in PWSTR ServiceCommandLine,
    __in PWSTR ServiceName
    )
{
    NTSTATUS status;
    PHSVC_API_MSG m;
    PVOID serviceCommandLine;
    PVOID serviceName;

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.ApiNumber = PhSvcExecuteRunAsCommandApiNumber;

    serviceCommandLine = PhSvcpCreateRelativeStringRef(ServiceCommandLine, -1, &m.u.ExecuteRunAsCommand.i.ServiceCommandLine);
    serviceName = PhSvcpCreateRelativeStringRef(ServiceName, -1, &m.u.ExecuteRunAsCommand.i.ServiceName);

    if (serviceCommandLine && serviceName)
    {
        status = PhSvcpCallServer(&m);
    }
    else
    {
        status = STATUS_NO_MEMORY;
    }

    if (serviceCommandLine)
        PhSvcpFreeHeap(serviceCommandLine);
    if (serviceName)
        PhSvcpFreeHeap(serviceName);

    return status;
}

NTSTATUS PhSvcCallUnloadDriver(
    __in_opt PVOID BaseAddress,
    __in_opt PWSTR Name
    )
{
    NTSTATUS status;
    PHSVC_API_MSG m;
    PVOID name = NULL;

    memset(&m, sizeof(PHSVC_API_MSG), 0);

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.ApiNumber = PhSvcUnloadDriverApiNumber;

    if (Name)
    {
        name = PhSvcpCreateRelativeStringRef(Name, -1, &m.u.UnloadDriver.i.Name);

        if (!name)
            return STATUS_NO_MEMORY;
    }

    status = PhSvcpCallServer(&m);

    if (name)
        PhSvcpFreeHeap(name);

    return status;
}

NTSTATUS PhSvcCallControlProcess(
    __in HANDLE ProcessId,
    __in PHSVC_API_CONTROLPROCESS_COMMAND Command
    )
{
    PHSVC_API_MSG m;

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.ApiNumber = PhSvcControlProcessApiNumber;
    m.u.ControlProcess.i.ProcessId = ProcessId;
    m.u.ControlProcess.i.Command = Command;

    return PhSvcpCallServer(&m);
}

NTSTATUS PhSvcCallControlService(
    __in PWSTR ServiceName,
    __in PHSVC_API_CONTROLSERVICE_COMMAND Command
    )
{
    NTSTATUS status;
    PHSVC_API_MSG m;
    PVOID serviceName;

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.ApiNumber = PhSvcControlServiceApiNumber;

    serviceName = PhSvcpCreateRelativeStringRef(ServiceName, -1, &m.u.ControlService.i.ServiceName);
    m.u.ControlService.i.Command = Command;

    if (serviceName)
    {
        status = PhSvcpCallServer(&m);
    }
    else
    {
        status = STATUS_NO_MEMORY;
    }

    if (serviceName)
        PhSvcpFreeHeap(serviceName);

    return status;
}
