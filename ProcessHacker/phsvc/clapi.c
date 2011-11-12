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

    if (PhSvcClPortHandle)
        return STATUS_ADDRESS_ALREADY_EXISTS;

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

VOID PhSvcDisconnectFromServer(
    VOID
    )
{
    if (PhSvcClPortHeap)
    {
        RtlDestroyHeap(PhSvcClPortHeap);
        PhSvcClPortHeap = NULL;
    }

    if (PhSvcClPortHandle)
    {
        NtClose(PhSvcClPortHandle);
        PhSvcClPortHandle = NULL;
    }

    PhSvcClServerProcessId = NULL;
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

PVOID PhSvcpCreateString(
    __in PVOID String,
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

NTSTATUS PhSvcpCallExecuteRunAsCommand(
    __in PHSVC_API_NUMBER ApiNumber,
    __in PPH_RUNAS_SERVICE_PARAMETERS Parameters
    )
{
    NTSTATUS status;
    PHSVC_API_MSG m;
    PVOID userName = NULL;
    PVOID password = NULL;
    ULONG passwordLength;
    PVOID currentDirectory = NULL;
    PVOID commandLine = NULL;
    PVOID fileName = NULL;
    PVOID desktopName = NULL;
    PVOID serviceName = NULL;

    memset(&m, 0, sizeof(PHSVC_API_MSG));

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.ApiNumber = ApiNumber;

    m.u.ExecuteRunAsCommand.i.ProcessId = Parameters->ProcessId;
    m.u.ExecuteRunAsCommand.i.LogonType = Parameters->LogonType;
    m.u.ExecuteRunAsCommand.i.SessionId = Parameters->SessionId;
    m.u.ExecuteRunAsCommand.i.UseLinkedToken = Parameters->UseLinkedToken;

    status = STATUS_NO_MEMORY;

    if (Parameters->UserName && !(userName = PhSvcpCreateString(Parameters->UserName, -1, &m.u.ExecuteRunAsCommand.i.UserName)))
        goto CleanupExit;

    if (Parameters->Password)
    {
        if (!(password = PhSvcpCreateString(Parameters->Password, -1, &m.u.ExecuteRunAsCommand.i.Password)))
            goto CleanupExit;

        passwordLength = m.u.ExecuteRunAsCommand.i.Password.Length;
    }

    if (Parameters->CurrentDirectory && !(currentDirectory = PhSvcpCreateString(Parameters->CurrentDirectory, -1, &m.u.ExecuteRunAsCommand.i.CurrentDirectory)))
        goto CleanupExit;
    if (Parameters->CommandLine && !(commandLine = PhSvcpCreateString(Parameters->CommandLine, -1, &m.u.ExecuteRunAsCommand.i.CommandLine)))
        goto CleanupExit;
    if (Parameters->FileName && !(fileName = PhSvcpCreateString(Parameters->FileName, -1, &m.u.ExecuteRunAsCommand.i.FileName)))
        goto CleanupExit;
    if (Parameters->DesktopName && !(desktopName = PhSvcpCreateString(Parameters->DesktopName, -1, &m.u.ExecuteRunAsCommand.i.DesktopName)))
        goto CleanupExit;
    if (Parameters->ServiceName && !(serviceName = PhSvcpCreateString(Parameters->ServiceName, -1, &m.u.ExecuteRunAsCommand.i.ServiceName)))
        goto CleanupExit;

    status = PhSvcpCallServer(&m);

CleanupExit:
    if (serviceName) PhSvcpFreeHeap(serviceName);
    if (desktopName) PhSvcpFreeHeap(desktopName);
    if (fileName) PhSvcpFreeHeap(fileName);
    if (commandLine) PhSvcpFreeHeap(commandLine);
    if (currentDirectory) PhSvcpFreeHeap(currentDirectory);

    if (password)
    {
        RtlSecureZeroMemory(password, passwordLength);
        PhSvcpFreeHeap(password);
    }

    if (userName) PhSvcpFreeHeap(userName);

    return status;
}

NTSTATUS PhSvcCallExecuteRunAsCommand(
    __in PPH_RUNAS_SERVICE_PARAMETERS Parameters
    )
{
    return PhSvcpCallExecuteRunAsCommand(PhSvcExecuteRunAsCommandApiNumber, Parameters);
}

NTSTATUS PhSvcCallUnloadDriver(
    __in_opt PVOID BaseAddress,
    __in_opt PWSTR Name
    )
{
    NTSTATUS status;
    PHSVC_API_MSG m;
    PVOID name = NULL;

    memset(&m, 0, sizeof(PHSVC_API_MSG));

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.ApiNumber = PhSvcUnloadDriverApiNumber;

    m.u.UnloadDriver.i.BaseAddress = BaseAddress;

    if (Name)
    {
        name = PhSvcpCreateString(Name, -1, &m.u.UnloadDriver.i.Name);

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
    __in PHSVC_API_CONTROLPROCESS_COMMAND Command,
    __in ULONG Argument
    )
{
    PHSVC_API_MSG m;

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.ApiNumber = PhSvcControlProcessApiNumber;
    m.u.ControlProcess.i.ProcessId = ProcessId;
    m.u.ControlProcess.i.Command = Command;
    m.u.ControlProcess.i.Argument = Argument;

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

    serviceName = PhSvcpCreateString(ServiceName, -1, &m.u.ControlService.i.ServiceName);
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

NTSTATUS PhSvcCallCreateService(
    __in PWSTR ServiceName,
    __in_opt PWSTR DisplayName,
    __in ULONG ServiceType,
    __in ULONG StartType,
    __in ULONG ErrorControl,
    __in_opt PWSTR BinaryPathName,
    __in_opt PWSTR LoadOrderGroup,
    __out_opt PULONG TagId,
    __in_opt PWSTR Dependencies,
    __in_opt PWSTR ServiceStartName,
    __in_opt PWSTR Password
    )
{
    NTSTATUS status;
    PHSVC_API_MSG m;
    PVOID serviceName = NULL;
    PVOID displayName = NULL;
    PVOID binaryPathName = NULL;
    PVOID loadOrderGroup = NULL;
    PVOID dependencies = NULL;
    PVOID serviceStartName = NULL;
    PVOID password = NULL;
    ULONG passwordLength;

    memset(&m, 0, sizeof(PHSVC_API_MSG));

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.ApiNumber = PhSvcCreateServiceApiNumber;

    m.u.CreateService.i.ServiceType = ServiceType;
    m.u.CreateService.i.StartType = StartType;
    m.u.CreateService.i.ErrorControl = ErrorControl;
    m.u.CreateService.i.TagIdSpecified = TagId != NULL;

    status = STATUS_NO_MEMORY;

    if (!(serviceName = PhSvcpCreateString(ServiceName, -1, &m.u.CreateService.i.ServiceName)))
        goto CleanupExit;
    if (DisplayName && !(displayName = PhSvcpCreateString(DisplayName, -1, &m.u.CreateService.i.DisplayName)))
        goto CleanupExit;
    if (BinaryPathName && !(binaryPathName = PhSvcpCreateString(BinaryPathName, -1, &m.u.CreateService.i.BinaryPathName)))
        goto CleanupExit;
    if (LoadOrderGroup && !(loadOrderGroup = PhSvcpCreateString(LoadOrderGroup, -1, &m.u.CreateService.i.LoadOrderGroup)))
        goto CleanupExit;

    if (Dependencies)
    {
        SIZE_T dependenciesLength;
        SIZE_T partCount;
        PWSTR part;

        dependenciesLength = sizeof(WCHAR);
        part = Dependencies;

        do
        {
            partCount = wcslen(part) + 1;
            part += partCount;
            dependenciesLength += partCount * sizeof(WCHAR);
        } while (partCount != 1); // stop at empty dependency part

        if (!(dependencies = PhSvcpCreateString(Dependencies, dependenciesLength, &m.u.CreateService.i.Dependencies)))
            goto CleanupExit;
    }

    if (ServiceStartName && !(serviceStartName = PhSvcpCreateString(ServiceStartName, -1, &m.u.CreateService.i.ServiceStartName)))
        goto CleanupExit;

    if (Password)
    {
        if (!(password = PhSvcpCreateString(Password, -1, &m.u.CreateService.i.Password)))
            goto CleanupExit;

        passwordLength = m.u.CreateService.i.Password.Length;
    }

    status = PhSvcpCallServer(&m);

    if (NT_SUCCESS(status))
    {
        if (TagId)
            *TagId = m.u.CreateService.o.TagId;
    }

CleanupExit:
    if (password)
    {
        RtlSecureZeroMemory(password, passwordLength);
        PhSvcpFreeHeap(password);
    }

    if (serviceStartName) PhSvcpFreeHeap(serviceStartName);
    if (dependencies) PhSvcpFreeHeap(dependencies);
    if (loadOrderGroup) PhSvcpFreeHeap(loadOrderGroup);
    if (binaryPathName) PhSvcpFreeHeap(binaryPathName);
    if (displayName) PhSvcpFreeHeap(displayName);
    if (serviceName) PhSvcpFreeHeap(serviceName);

    return status;
}

NTSTATUS PhSvcCallChangeServiceConfig(
    __in PWSTR ServiceName,
    __in ULONG ServiceType,
    __in ULONG StartType,
    __in ULONG ErrorControl,
    __in_opt PWSTR BinaryPathName,
    __in_opt PWSTR LoadOrderGroup,
    __out_opt PULONG TagId,
    __in_opt PWSTR Dependencies,
    __in_opt PWSTR ServiceStartName,
    __in_opt PWSTR Password,
    __in_opt PWSTR DisplayName
    )
{
    NTSTATUS status;
    PHSVC_API_MSG m;
    PVOID serviceName = NULL;
    PVOID binaryPathName = NULL;
    PVOID loadOrderGroup = NULL;
    PVOID dependencies = NULL;
    PVOID serviceStartName = NULL;
    PVOID password = NULL;
    ULONG passwordLength;
    PVOID displayName = NULL;

    memset(&m, 0, sizeof(PHSVC_API_MSG));

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.ApiNumber = PhSvcChangeServiceConfigApiNumber;

    m.u.ChangeServiceConfig.i.ServiceType = ServiceType;
    m.u.ChangeServiceConfig.i.StartType = StartType;
    m.u.ChangeServiceConfig.i.ErrorControl = ErrorControl;
    m.u.ChangeServiceConfig.i.TagIdSpecified = TagId != NULL;

    status = STATUS_NO_MEMORY;

    if (!(serviceName = PhSvcpCreateString(ServiceName, -1, &m.u.ChangeServiceConfig.i.ServiceName)))
        goto CleanupExit;
    if (BinaryPathName && !(binaryPathName = PhSvcpCreateString(BinaryPathName, -1, &m.u.ChangeServiceConfig.i.BinaryPathName)))
        goto CleanupExit;
    if (LoadOrderGroup && !(loadOrderGroup = PhSvcpCreateString(LoadOrderGroup, -1, &m.u.ChangeServiceConfig.i.LoadOrderGroup)))
        goto CleanupExit;

    if (Dependencies)
    {
        SIZE_T dependenciesLength;
        SIZE_T partCount;
        PWSTR part;

        dependenciesLength = sizeof(WCHAR);
        part = Dependencies;

        do
        {
            partCount = wcslen(part) + 1;
            part += partCount;
            dependenciesLength += partCount * sizeof(WCHAR);
        } while (partCount != 1); // stop at empty dependency part

        if (!(dependencies = PhSvcpCreateString(Dependencies, dependenciesLength, &m.u.ChangeServiceConfig.i.Dependencies)))
            goto CleanupExit;
    }

    if (ServiceStartName && !(serviceStartName = PhSvcpCreateString(ServiceStartName, -1, &m.u.ChangeServiceConfig.i.ServiceStartName)))
        goto CleanupExit;

    if (Password)
    {
        if (!(password = PhSvcpCreateString(Password, -1, &m.u.ChangeServiceConfig.i.Password)))
            goto CleanupExit;

        passwordLength = m.u.ChangeServiceConfig.i.Password.Length;
    }

    if (DisplayName && !(displayName = PhSvcpCreateString(DisplayName, -1, &m.u.ChangeServiceConfig.i.DisplayName)))
        goto CleanupExit;

    status = PhSvcpCallServer(&m);

    if (NT_SUCCESS(status))
    {
        if (TagId)
            *TagId = m.u.ChangeServiceConfig.o.TagId;
    }

CleanupExit:
    if (displayName) PhSvcpFreeHeap(displayName);

    if (password)
    {
        RtlSecureZeroMemory(password, passwordLength);
        PhSvcpFreeHeap(password);
    }

    if (serviceStartName) PhSvcpFreeHeap(serviceStartName);
    if (dependencies) PhSvcpFreeHeap(dependencies);
    if (loadOrderGroup) PhSvcpFreeHeap(loadOrderGroup);
    if (binaryPathName) PhSvcpFreeHeap(binaryPathName);
    if (serviceName) PhSvcpFreeHeap(serviceName);

    return status;
}

NTSTATUS PhSvcCallChangeServiceConfig2(
    __in PWSTR ServiceName,
    __in ULONG InfoLevel,
    __in PVOID Info
    )
{
    NTSTATUS status;
    PHSVC_API_MSG m;
    PVOID serviceName = NULL;
    PVOID info = NULL;

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.ApiNumber = PhSvcChangeServiceConfig2ApiNumber;

    m.u.ChangeServiceConfig2.i.InfoLevel = InfoLevel;

    if (serviceName = PhSvcpCreateString(ServiceName, -1, &m.u.ChangeServiceConfig2.i.ServiceName))
    {
        switch (InfoLevel)
        {
        case SERVICE_CONFIG_DELAYED_AUTO_START_INFO:
            info = PhSvcpCreateString(Info, sizeof(SERVICE_DELAYED_AUTO_START_INFO), &m.u.ChangeServiceConfig2.i.Info);
            break;
        default:
            return STATUS_INVALID_PARAMETER;
        }
    }

    if (serviceName && info)
    {
        status = PhSvcpCallServer(&m);
    }
    else
    {
        status = STATUS_NO_MEMORY;
    }

    if (info)
        PhSvcpFreeHeap(info);
    if (serviceName)
        PhSvcpFreeHeap(serviceName);

    return status;
}

NTSTATUS PhSvcCallSetTcpEntry(
    __in PVOID TcpRow
    )
{
    PHSVC_API_MSG m;
    struct
    {
        DWORD dwState;
        DWORD dwLocalAddr;
        DWORD dwLocalPort;
        DWORD dwRemoteAddr;
        DWORD dwRemotePort;
    } *tcpRow = TcpRow;

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.ApiNumber = PhSvcSetTcpEntryApiNumber;

    m.u.SetTcpEntry.i.State = tcpRow->dwState;
    m.u.SetTcpEntry.i.LocalAddress = tcpRow->dwLocalAddr;
    m.u.SetTcpEntry.i.LocalPort = tcpRow->dwLocalPort;
    m.u.SetTcpEntry.i.RemoteAddress = tcpRow->dwRemoteAddr;
    m.u.SetTcpEntry.i.RemotePort = tcpRow->dwRemotePort;

    return PhSvcpCallServer(&m);
}

NTSTATUS PhSvcCallControlThread(
    __in HANDLE ThreadId,
    __in PHSVC_API_CONTROLTHREAD_COMMAND Command,
    __in ULONG Argument
    )
{
    PHSVC_API_MSG m;

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.ApiNumber = PhSvcControlThreadApiNumber;
    m.u.ControlThread.i.ThreadId = ThreadId;
    m.u.ControlThread.i.Command = Command;
    m.u.ControlThread.i.Argument = Argument;

    return PhSvcpCallServer(&m);
}

NTSTATUS PhSvcCallAddAccountRight(
    __in PSID AccountSid,
    __in PUNICODE_STRING UserRight
    )
{
    NTSTATUS status;
    PHSVC_API_MSG m;
    PVOID accountSid = NULL;
    PVOID userRight = NULL;

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.ApiNumber = PhSvcAddAccountRightApiNumber;

    status = STATUS_NO_MEMORY;

    if (!(accountSid = PhSvcpCreateString(AccountSid, RtlLengthSid(AccountSid), &m.u.AddAccountRight.i.AccountSid)))
        goto CleanupExit;
    if (!(userRight = PhSvcpCreateString(UserRight->Buffer, UserRight->Length, &m.u.AddAccountRight.i.UserRight)))
        goto CleanupExit;

    status = PhSvcpCallServer(&m);

CleanupExit:
    if (userRight) PhSvcpFreeHeap(userRight);
    if (accountSid) PhSvcpFreeHeap(accountSid);

    return status;
}

NTSTATUS PhSvcCallInvokeRunAsService(
    __in PPH_RUNAS_SERVICE_PARAMETERS Parameters
    )
{
    return PhSvcpCallExecuteRunAsCommand(PhSvcInvokeRunAsServiceApiNumber, Parameters);
}

NTSTATUS PhSvcCallIssueMemoryListCommand(
    __in SYSTEM_MEMORY_LIST_COMMAND Command
    )
{
    PHSVC_API_MSG m;

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.ApiNumber = PhSvcIssueMemoryListCommandApiNumber;
    m.u.IssueMemoryListCommand.i.Command = Command;

    return PhSvcpCallServer(&m);
}
