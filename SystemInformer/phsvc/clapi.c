/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2015
 *     dmex    2017-2023
 *
 */

#include <phapp.h>
#include <phsvccl.h>

HANDLE PhSvcClPortHandle;
PVOID PhSvcClPortHeap;
HANDLE PhSvcClServerProcessId;

NTSTATUS PhSvcConnectToServer(
    _In_ PUNICODE_STRING PortName,
    _In_opt_ SIZE_T PortSectionSize
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
        PortSectionSize = UInt32x32To64(2, 1024 * 1024); // 2 MB

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
    connectInfo.ServerProcessId = ULONG_MAX;

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

    PhSvcClServerProcessId = UlongToHandle(connectInfo.ServerProcessId);

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

    RtlSetHeapInformation(
        PhSvcClPortHeap,
        HeapCompatibilityInformation,
        &(ULONG){ HEAP_COMPATIBILITY_LFH },
        sizeof(ULONG)
        );

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

_Success_(return != NULL)
PVOID PhSvcpAllocateHeap(
    _In_ SIZE_T Size,
    _Out_ PULONG Offset
    )
{
    PVOID memory;

    if (!PhSvcClPortHeap)
        return NULL;

    memory = RtlAllocateHeap(PhSvcClPortHeap, 0, Size);

    if (!memory)
        return NULL;

    *Offset = PtrToUlong(PTR_SUB_OFFSET(memory, PhSvcClPortHeap));

    return memory;
}

VOID PhSvcpFreeHeap(
    _In_ PVOID Memory
    )
{
    if (!PhSvcClPortHeap)
        return;

    RtlFreeHeap(PhSvcClPortHeap, 0, Memory);
}

_Success_(return != NULL)
PVOID PhSvcpCreateString(
    _In_opt_ PVOID String,
    _In_ SIZE_T Length,
    _Out_ PPH_RELATIVE_STRINGREF StringRef
    )
{
    PVOID memory;
    SIZE_T length;
    ULONG offset;

    if (Length != SIZE_MAX)
        length = Length;
    else
        length = PhCountStringZ(String) * sizeof(WCHAR);

    if (length > ULONG_MAX)
        return NULL;

    memory = PhSvcpAllocateHeap(length, &offset);

    if (!memory)
        return NULL;

    if (String)
        memcpy(memory, String, length);

    StringRef->Length = (ULONG)length;
    StringRef->Offset = offset;

    return memory;
}

NTSTATUS PhSvcpCallServer(
    _Inout_ PPHSVC_API_MSG Message
    )
{
    NTSTATUS status;

    Message->h.u1.s1.DataLength = sizeof(PHSVC_API_MSG) - FIELD_OFFSET(PHSVC_API_MSG, p);
    Message->h.u1.s1.TotalLength = sizeof(PHSVC_API_MSG);
    Message->h.u2.ZeroInit = 0;

    status = NtRequestWaitReplyPort(PhSvcClPortHandle, &Message->h, &Message->h);

    if (!NT_SUCCESS(status))
        return status;

    return Message->p.ReturnStatus;
}

NTSTATUS PhSvcCallPlugin(
    _In_ PPH_STRINGREF ApiId,
    _In_reads_bytes_opt_(InLength) PVOID InBuffer,
    _In_ ULONG InLength,
    _Out_writes_bytes_opt_(OutLength) PVOID OutBuffer,
    _In_ ULONG OutLength
    )
{
    NTSTATUS status;
    PHSVC_API_MSG m;
    PVOID apiId = NULL;

    memset(&m, 0, sizeof(PHSVC_API_MSG));

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;
    if (InLength > sizeof(m.p.u.Plugin.i.Data))
        return STATUS_BUFFER_OVERFLOW;

    m.p.ApiNumber = PhSvcPluginApiNumber;

    if (!(apiId = PhSvcpCreateString(ApiId->Buffer, ApiId->Length, &m.p.u.Plugin.i.ApiId)))
        return STATUS_NO_MEMORY;

    if (InLength != 0)
        memcpy(m.p.u.Plugin.i.Data, InBuffer, InLength);

    status = PhSvcpCallServer(&m);

    if (OutLength != 0)
        memcpy(OutBuffer, m.p.u.Plugin.o.Data, min(OutLength, sizeof(m.p.u.Plugin.o.Data)));

    if (apiId)
        PhSvcpFreeHeap(apiId);

    return status;
}

NTSTATUS PhSvcpCallExecuteRunAsCommand(
    _In_ PHSVC_API_NUMBER ApiNumber,
    _In_ PPH_RUNAS_SERVICE_PARAMETERS Parameters
    )
{
    NTSTATUS status;
    PHSVC_API_MSG m;
    PVOID userName = NULL;
    PVOID password = NULL;
    ULONG passwordLength = 0;
    PVOID currentDirectory = NULL;
    PVOID commandLine = NULL;
    PVOID fileName = NULL;
    PVOID desktopName = NULL;
    PVOID serviceName = NULL;

    memset(&m, 0, sizeof(PHSVC_API_MSG));

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.p.ApiNumber = ApiNumber;

    m.p.u.ExecuteRunAsCommand.i.ProcessId = Parameters->ProcessId;
    m.p.u.ExecuteRunAsCommand.i.LogonType = Parameters->LogonType;
    m.p.u.ExecuteRunAsCommand.i.SessionId = Parameters->SessionId;
    m.p.u.ExecuteRunAsCommand.i.UseLinkedToken = Parameters->UseLinkedToken;
    m.p.u.ExecuteRunAsCommand.i.CreateSuspendedProcess = Parameters->CreateSuspendedProcess;

    status = STATUS_NO_MEMORY;

    if (Parameters->UserName && !(userName = PhSvcpCreateString(Parameters->UserName, -1, &m.p.u.ExecuteRunAsCommand.i.UserName)))
        goto CleanupExit;

    if (Parameters->Password)
    {
        if (!(password = PhSvcpCreateString(Parameters->Password, -1, &m.p.u.ExecuteRunAsCommand.i.Password)))
            goto CleanupExit;

        passwordLength = m.p.u.ExecuteRunAsCommand.i.Password.Length;
    }

    if (Parameters->CurrentDirectory && !(currentDirectory = PhSvcpCreateString(Parameters->CurrentDirectory, -1, &m.p.u.ExecuteRunAsCommand.i.CurrentDirectory)))
        goto CleanupExit;
    if (Parameters->CommandLine && !(commandLine = PhSvcpCreateString(Parameters->CommandLine, -1, &m.p.u.ExecuteRunAsCommand.i.CommandLine)))
        goto CleanupExit;
    if (Parameters->FileName && !(fileName = PhSvcpCreateString(Parameters->FileName, -1, &m.p.u.ExecuteRunAsCommand.i.FileName)))
        goto CleanupExit;
    if (Parameters->DesktopName && !(desktopName = PhSvcpCreateString(Parameters->DesktopName, -1, &m.p.u.ExecuteRunAsCommand.i.DesktopName)))
        goto CleanupExit;
    if (Parameters->ServiceName && !(serviceName = PhSvcpCreateString(Parameters->ServiceName, -1, &m.p.u.ExecuteRunAsCommand.i.ServiceName)))
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
    _In_ PPH_RUNAS_SERVICE_PARAMETERS Parameters
    )
{
    return PhSvcpCallExecuteRunAsCommand(PhSvcExecuteRunAsCommandApiNumber, Parameters);
}

NTSTATUS PhSvcCallUnloadDriver(
    _In_opt_ PVOID BaseAddress,
    _In_opt_ PWSTR Name
    )
{
    NTSTATUS status;
    PHSVC_API_MSG m;
    PVOID name = NULL;

    memset(&m, 0, sizeof(PHSVC_API_MSG));

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.p.ApiNumber = PhSvcUnloadDriverApiNumber;

    m.p.u.UnloadDriver.i.BaseAddress = BaseAddress;

    if (Name)
    {
        name = PhSvcpCreateString(Name, -1, &m.p.u.UnloadDriver.i.Name);

        if (!name)
            return STATUS_NO_MEMORY;
    }

    status = PhSvcpCallServer(&m);

    if (name)
        PhSvcpFreeHeap(name);

    return status;
}

NTSTATUS PhSvcCallControlProcess(
    _In_opt_ HANDLE ProcessId,
    _In_ PHSVC_API_CONTROLPROCESS_COMMAND Command,
    _In_ ULONG Argument
    )
{
    PHSVC_API_MSG m;

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.p.ApiNumber = PhSvcControlProcessApiNumber;
    m.p.u.ControlProcess.i.ProcessId = ProcessId;
    m.p.u.ControlProcess.i.Command = Command;
    m.p.u.ControlProcess.i.Argument = Argument;

    return PhSvcpCallServer(&m);
}

NTSTATUS PhSvcCallControlService(
    _In_ PWSTR ServiceName,
    _In_ PHSVC_API_CONTROLSERVICE_COMMAND Command
    )
{
    NTSTATUS status;
    PHSVC_API_MSG m;
    PVOID serviceName;

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.p.ApiNumber = PhSvcControlServiceApiNumber;

    serviceName = PhSvcpCreateString(ServiceName, -1, &m.p.u.ControlService.i.ServiceName);
    m.p.u.ControlService.i.Command = Command;

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
    _In_ PWSTR ServiceName,
    _In_opt_ PWSTR DisplayName,
    _In_ ULONG ServiceType,
    _In_ ULONG StartType,
    _In_ ULONG ErrorControl,
    _In_opt_ PWSTR BinaryPathName,
    _In_opt_ PWSTR LoadOrderGroup,
    _Out_opt_ PULONG TagId,
    _In_opt_ PWSTR Dependencies,
    _In_opt_ PWSTR ServiceStartName,
    _In_opt_ PWSTR Password
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

    m.p.ApiNumber = PhSvcCreateServiceApiNumber;

    m.p.u.CreateService.i.ServiceType = ServiceType;
    m.p.u.CreateService.i.StartType = StartType;
    m.p.u.CreateService.i.ErrorControl = ErrorControl;
    m.p.u.CreateService.i.TagIdSpecified = TagId != NULL;

    status = STATUS_NO_MEMORY;

    if (!(serviceName = PhSvcpCreateString(ServiceName, -1, &m.p.u.CreateService.i.ServiceName)))
        goto CleanupExit;
    if (DisplayName && !(displayName = PhSvcpCreateString(DisplayName, -1, &m.p.u.CreateService.i.DisplayName)))
        goto CleanupExit;
    if (BinaryPathName && !(binaryPathName = PhSvcpCreateString(BinaryPathName, -1, &m.p.u.CreateService.i.BinaryPathName)))
        goto CleanupExit;
    if (LoadOrderGroup && !(loadOrderGroup = PhSvcpCreateString(LoadOrderGroup, -1, &m.p.u.CreateService.i.LoadOrderGroup)))
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
            partCount = PhCountStringZ(part) + 1;
            part += partCount;
            dependenciesLength += partCount * sizeof(WCHAR);
        } while (partCount != 1); // stop at empty dependency part

        if (!(dependencies = PhSvcpCreateString(Dependencies, dependenciesLength, &m.p.u.CreateService.i.Dependencies)))
            goto CleanupExit;
    }

    if (ServiceStartName && !(serviceStartName = PhSvcpCreateString(ServiceStartName, -1, &m.p.u.CreateService.i.ServiceStartName)))
        goto CleanupExit;

    if (Password)
    {
        if (!(password = PhSvcpCreateString(Password, -1, &m.p.u.CreateService.i.Password)))
            goto CleanupExit;

        passwordLength = m.p.u.CreateService.i.Password.Length;
    }

    status = PhSvcpCallServer(&m);

    if (NT_SUCCESS(status))
    {
        if (TagId)
            *TagId = m.p.u.CreateService.o.TagId;
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
    _In_ PWSTR ServiceName,
    _In_ ULONG ServiceType,
    _In_ ULONG StartType,
    _In_ ULONG ErrorControl,
    _In_opt_ PWSTR BinaryPathName,
    _In_opt_ PWSTR LoadOrderGroup,
    _Out_opt_ PULONG TagId,
    _In_opt_ PWSTR Dependencies,
    _In_opt_ PWSTR ServiceStartName,
    _In_opt_ PWSTR Password,
    _In_opt_ PWSTR DisplayName
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

    m.p.ApiNumber = PhSvcChangeServiceConfigApiNumber;

    m.p.u.ChangeServiceConfig.i.ServiceType = ServiceType;
    m.p.u.ChangeServiceConfig.i.StartType = StartType;
    m.p.u.ChangeServiceConfig.i.ErrorControl = ErrorControl;
    m.p.u.ChangeServiceConfig.i.TagIdSpecified = TagId != NULL;

    status = STATUS_NO_MEMORY;

    if (!(serviceName = PhSvcpCreateString(ServiceName, -1, &m.p.u.ChangeServiceConfig.i.ServiceName)))
        goto CleanupExit;
    if (BinaryPathName && !(binaryPathName = PhSvcpCreateString(BinaryPathName, -1, &m.p.u.ChangeServiceConfig.i.BinaryPathName)))
        goto CleanupExit;
    if (LoadOrderGroup && !(loadOrderGroup = PhSvcpCreateString(LoadOrderGroup, -1, &m.p.u.ChangeServiceConfig.i.LoadOrderGroup)))
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
            partCount = PhCountStringZ(part) + 1;
            part += partCount;
            dependenciesLength += partCount * sizeof(WCHAR);
        } while (partCount != 1); // stop at empty dependency part

        if (!(dependencies = PhSvcpCreateString(Dependencies, dependenciesLength, &m.p.u.ChangeServiceConfig.i.Dependencies)))
            goto CleanupExit;
    }

    if (ServiceStartName && !(serviceStartName = PhSvcpCreateString(ServiceStartName, -1, &m.p.u.ChangeServiceConfig.i.ServiceStartName)))
        goto CleanupExit;

    if (Password)
    {
        if (!(password = PhSvcpCreateString(Password, -1, &m.p.u.ChangeServiceConfig.i.Password)))
            goto CleanupExit;

        passwordLength = m.p.u.ChangeServiceConfig.i.Password.Length;
    }

    if (DisplayName && !(displayName = PhSvcpCreateString(DisplayName, -1, &m.p.u.ChangeServiceConfig.i.DisplayName)))
        goto CleanupExit;

    status = PhSvcpCallServer(&m);

    if (NT_SUCCESS(status))
    {
        if (TagId)
            *TagId = m.p.u.ChangeServiceConfig.o.TagId;
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

PVOID PhSvcpPackRoot(
    _Inout_ PPH_BYTES_BUILDER BytesBuilder,
    _In_ PVOID Buffer,
    _In_ SIZE_T Length
    )
{
    return PhAppendBytesBuilderEx(BytesBuilder, Buffer, Length, sizeof(ULONG_PTR), NULL);
}

VOID PhSvcpPackBuffer_V(
    _Inout_ PPH_BYTES_BUILDER BytesBuilder,
    _Inout_ PVOID *PointerInBytesBuilder,
    _In_ SIZE_T Length,
    _In_ SIZE_T Alignment,
    _In_ ULONG NumberOfPointersToRebase,
    _In_ va_list ArgPtr
    )
{
    va_list argptr;
    ULONG_PTR oldBase;
    SIZE_T oldLength;
    ULONG_PTR newBase;
    SIZE_T offset;
    ULONG i;
    PVOID *pointer;

    oldBase = (ULONG_PTR)BytesBuilder->Bytes->Buffer;
    oldLength = BytesBuilder->Bytes->Length;
    assert((ULONG_PTR)PointerInBytesBuilder >= oldBase && (ULONG_PTR)PointerInBytesBuilder + sizeof(PVOID) <= oldBase + oldLength);

    if (!*PointerInBytesBuilder)
        return;

    PhAppendBytesBuilderEx(BytesBuilder, *PointerInBytesBuilder, Length, Alignment, &offset);
    newBase = (ULONG_PTR)BytesBuilder->Bytes->Buffer;

    PointerInBytesBuilder = (PVOID *)((ULONG_PTR)PointerInBytesBuilder - oldBase + newBase);
    *PointerInBytesBuilder = (PVOID)offset;

    argptr = ArgPtr;

    for (i = 0; i < NumberOfPointersToRebase; i++)
    {
        pointer = va_arg(argptr, PVOID *);
        assert(!*pointer || ((ULONG_PTR)*pointer >= oldBase && (ULONG_PTR)*pointer + sizeof(PVOID) <= oldBase + oldLength));

        if (*pointer)
            *pointer = (PVOID)((ULONG_PTR)*pointer - oldBase + newBase);
    }
}

VOID PhSvcpPackBuffer(
    _Inout_ PPH_BYTES_BUILDER BytesBuilder,
    _Inout_ PVOID *PointerInBytesBuilder,
    _In_ SIZE_T Length,
    _In_ SIZE_T Alignment,
    _In_ ULONG NumberOfPointersToRebase,
    ...
    )
{
    va_list argptr;

    va_start(argptr, NumberOfPointersToRebase);
    PhSvcpPackBuffer_V(BytesBuilder, PointerInBytesBuilder, Length, Alignment, NumberOfPointersToRebase, argptr);
}

SIZE_T PhSvcpBufferLengthStringZ(
    _In_opt_ PWSTR String,
    _In_ BOOLEAN Multi
    )
{
    SIZE_T length = 0;

    if (String)
    {
        if (Multi)
        {
            PWSTR part = String;
            SIZE_T partCount;

            while (TRUE)
            {
                partCount = PhCountStringZ(part);
                length += (partCount + 1) * sizeof(WCHAR);

                if (partCount == 0)
                    break;

                part += partCount + 1;
            }
        }
        else
        {
            length = (PhCountStringZ(String) + 1) * sizeof(WCHAR);
        }
    }

    return length;
}

NTSTATUS PhSvcCallChangeServiceConfig2(
    _In_ PWSTR ServiceName,
    _In_ ULONG InfoLevel,
    _In_ PVOID Info
    )
{
    NTSTATUS status;
    PHSVC_API_MSG m;
    PVOID serviceName = NULL;
    PVOID info = NULL;
    PH_BYTES_BUILDER bb;

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.p.ApiNumber = PhSvcChangeServiceConfig2ApiNumber;

    m.p.u.ChangeServiceConfig2.i.InfoLevel = InfoLevel;

    if (serviceName = PhSvcpCreateString(ServiceName, -1, &m.p.u.ChangeServiceConfig2.i.ServiceName))
    {
        switch (InfoLevel)
        {
        case SERVICE_CONFIG_FAILURE_ACTIONS:
            {
                LPSERVICE_FAILURE_ACTIONS failureActions = Info;
                LPSERVICE_FAILURE_ACTIONS packedFailureActions;

                PhInitializeBytesBuilder(&bb, 200);
                packedFailureActions = PhSvcpPackRoot(&bb, failureActions, sizeof(SERVICE_FAILURE_ACTIONS));
                PhSvcpPackBuffer(&bb, &packedFailureActions->lpRebootMsg, PhSvcpBufferLengthStringZ(failureActions->lpRebootMsg, FALSE), sizeof(WCHAR),
                    1, &packedFailureActions);
                PhSvcpPackBuffer(&bb, &packedFailureActions->lpCommand, PhSvcpBufferLengthStringZ(failureActions->lpCommand, FALSE), sizeof(WCHAR),
                    1, &packedFailureActions);

                if (failureActions->cActions != 0 && failureActions->lpsaActions)
                {
                    PhSvcpPackBuffer(&bb, &packedFailureActions->lpsaActions, failureActions->cActions * sizeof(SC_ACTION), __alignof(SC_ACTION),
                        1, &packedFailureActions);
                }

                info = PhSvcpCreateString(bb.Bytes->Buffer, bb.Bytes->Length, &m.p.u.ChangeServiceConfig2.i.Info);
                PhDeleteBytesBuilder(&bb);
            }
            break;
        case SERVICE_CONFIG_DELAYED_AUTO_START_INFO:
            info = PhSvcpCreateString(Info, sizeof(SERVICE_DELAYED_AUTO_START_INFO), &m.p.u.ChangeServiceConfig2.i.Info);
            break;
        case SERVICE_CONFIG_FAILURE_ACTIONS_FLAG:
            info = PhSvcpCreateString(Info, sizeof(SERVICE_FAILURE_ACTIONS_FLAG), &m.p.u.ChangeServiceConfig2.i.Info);
            break;
        case SERVICE_CONFIG_SERVICE_SID_INFO:
            info = PhSvcpCreateString(Info, sizeof(SERVICE_SID_INFO), &m.p.u.ChangeServiceConfig2.i.Info);
            break;
        case SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO:
            {
                LPSERVICE_REQUIRED_PRIVILEGES_INFO requiredPrivilegesInfo = Info;
                LPSERVICE_REQUIRED_PRIVILEGES_INFO packedRequiredPrivilegesInfo;

                PhInitializeBytesBuilder(&bb, 100);
                packedRequiredPrivilegesInfo = PhSvcpPackRoot(&bb, requiredPrivilegesInfo, sizeof(SERVICE_REQUIRED_PRIVILEGES_INFO));
                PhSvcpPackBuffer(&bb, &packedRequiredPrivilegesInfo->pmszRequiredPrivileges, PhSvcpBufferLengthStringZ(requiredPrivilegesInfo->pmszRequiredPrivileges, TRUE), sizeof(WCHAR),
                    1, &packedRequiredPrivilegesInfo);

                info = PhSvcpCreateString(bb.Bytes->Buffer, bb.Bytes->Length, &m.p.u.ChangeServiceConfig2.i.Info);
                PhDeleteBytesBuilder(&bb);
            }
            break;
        case SERVICE_CONFIG_PRESHUTDOWN_INFO:
            info = PhSvcpCreateString(Info, sizeof(SERVICE_PRESHUTDOWN_INFO), &m.p.u.ChangeServiceConfig2.i.Info);
            break;
        case SERVICE_CONFIG_TRIGGER_INFO:
            {
                PSERVICE_TRIGGER_INFO triggerInfo = Info;
                PSERVICE_TRIGGER_INFO packedTriggerInfo;
                ULONG i;
                PSERVICE_TRIGGER packedTrigger;
                ULONG j;
                PSERVICE_TRIGGER_SPECIFIC_DATA_ITEM packedDataItem;
                ULONG alignment;

                PhInitializeBytesBuilder(&bb, 400);
                packedTriggerInfo = PhSvcpPackRoot(&bb, triggerInfo, sizeof(SERVICE_TRIGGER_INFO));

                if (triggerInfo->cTriggers != 0 && triggerInfo->pTriggers)
                {
                    PhSvcpPackBuffer(&bb, &packedTriggerInfo->pTriggers, triggerInfo->cTriggers * sizeof(SERVICE_TRIGGER), __alignof(SERVICE_TRIGGER),
                        1, &packedTriggerInfo);

                    for (i = 0; i < triggerInfo->cTriggers; i++)
                    {
                        packedTrigger = PhOffsetBytesBuilder(&bb, (SIZE_T)packedTriggerInfo->pTriggers + i * sizeof(SERVICE_TRIGGER));

                        PhSvcpPackBuffer(&bb, &packedTrigger->pTriggerSubtype, sizeof(GUID), __alignof(GUID),
                            2, &packedTriggerInfo, &packedTrigger);

                        if (packedTrigger->cDataItems != 0 && packedTrigger->pDataItems)
                        {
                            PhSvcpPackBuffer(&bb, &packedTrigger->pDataItems, packedTrigger->cDataItems * sizeof(SERVICE_TRIGGER_SPECIFIC_DATA_ITEM), __alignof(SERVICE_TRIGGER_SPECIFIC_DATA_ITEM),
                                2, &packedTriggerInfo, &packedTrigger);

                            for (j = 0; j < packedTrigger->cDataItems; j++)
                            {
                                packedDataItem = PhOffsetBytesBuilder(&bb, (SIZE_T)packedTrigger->pDataItems + j * sizeof(SERVICE_TRIGGER_SPECIFIC_DATA_ITEM));
                                alignment = 1;

                                switch (packedDataItem->dwDataType)
                                {
                                case SERVICE_TRIGGER_DATA_TYPE_BINARY:
                                case SERVICE_TRIGGER_DATA_TYPE_LEVEL:
                                    alignment = sizeof(CHAR);
                                    break;
                                case SERVICE_TRIGGER_DATA_TYPE_STRING:
                                    alignment = sizeof(WCHAR);
                                    break;
                                case SERVICE_TRIGGER_DATA_TYPE_KEYWORD_ANY:
                                case SERVICE_TRIGGER_DATA_TYPE_KEYWORD_ALL:
                                    alignment = sizeof(ULONG64);
                                    break;
                                }

                                PhSvcpPackBuffer(&bb, &packedDataItem->pData, packedDataItem->cbData, alignment,
                                    3, &packedTriggerInfo, &packedTrigger, &packedDataItem);
                            }
                        }
                    }
                }

                info = PhSvcpCreateString(bb.Bytes->Buffer, bb.Bytes->Length, &m.p.u.ChangeServiceConfig2.i.Info);
                PhDeleteBytesBuilder(&bb);
            }
            break;
        case SERVICE_CONFIG_LAUNCH_PROTECTED:
            info = PhSvcpCreateString(Info, sizeof(SERVICE_LAUNCH_PROTECTED_INFO), &m.p.u.ChangeServiceConfig2.i.Info);
            break;
        default:
            status = STATUS_INVALID_PARAMETER;
            break;
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
    _In_ PVOID TcpRow
    )
{
    PHSVC_API_MSG m;
    struct
    {
        ULONG dwState;
        ULONG dwLocalAddr;
        ULONG dwLocalPort;
        ULONG dwRemoteAddr;
        ULONG dwRemotePort;
    } *tcpRow = TcpRow;

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.p.ApiNumber = PhSvcSetTcpEntryApiNumber;

    m.p.u.SetTcpEntry.i.State = tcpRow->dwState;
    m.p.u.SetTcpEntry.i.LocalAddress = tcpRow->dwLocalAddr;
    m.p.u.SetTcpEntry.i.LocalPort = tcpRow->dwLocalPort;
    m.p.u.SetTcpEntry.i.RemoteAddress = tcpRow->dwRemoteAddr;
    m.p.u.SetTcpEntry.i.RemotePort = tcpRow->dwRemotePort;

    return PhSvcpCallServer(&m);
}

NTSTATUS PhSvcCallControlThread(
    _In_ HANDLE ThreadId,
    _In_ PHSVC_API_CONTROLTHREAD_COMMAND Command,
    _In_ ULONG Argument
    )
{
    PHSVC_API_MSG m;

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.p.ApiNumber = PhSvcControlThreadApiNumber;
    m.p.u.ControlThread.i.ThreadId = ThreadId;
    m.p.u.ControlThread.i.Command = Command;
    m.p.u.ControlThread.i.Argument = Argument;

    return PhSvcpCallServer(&m);
}

NTSTATUS PhSvcCallAddAccountRight(
    _In_ PSID AccountSid,
    _In_ PUNICODE_STRING UserRight
    )
{
    NTSTATUS status;
    PHSVC_API_MSG m;
    PVOID accountSid = NULL;
    PVOID userRight = NULL;

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.p.ApiNumber = PhSvcAddAccountRightApiNumber;

    status = STATUS_NO_MEMORY;

    if (!(accountSid = PhSvcpCreateString(AccountSid, PhLengthSid(AccountSid), &m.p.u.AddAccountRight.i.AccountSid)))
        goto CleanupExit;
    if (!(userRight = PhSvcpCreateString(UserRight->Buffer, UserRight->Length, &m.p.u.AddAccountRight.i.UserRight)))
        goto CleanupExit;

    status = PhSvcpCallServer(&m);

CleanupExit:
    if (userRight) PhSvcpFreeHeap(userRight);
    if (accountSid) PhSvcpFreeHeap(accountSid);

    return status;
}

NTSTATUS PhSvcCallInvokeRunAsService(
    _In_ PPH_RUNAS_SERVICE_PARAMETERS Parameters
    )
{
    return PhSvcpCallExecuteRunAsCommand(PhSvcInvokeRunAsServiceApiNumber, Parameters);
}

NTSTATUS PhSvcCallIssueMemoryListCommand(
    _In_ SYSTEM_MEMORY_LIST_COMMAND Command
    )
{
    PHSVC_API_MSG m;

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.p.ApiNumber = PhSvcIssueMemoryListCommandApiNumber;
    m.p.u.IssueMemoryListCommand.i.Command = Command;

    return PhSvcpCallServer(&m);
}

NTSTATUS PhSvcCallPostMessage(
    _In_opt_ HWND hWnd,
    _In_ UINT Msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PHSVC_API_MSG m;

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.p.ApiNumber = PhSvcPostMessageApiNumber;
    m.p.u.PostMessage.i.hWnd = hWnd;
    m.p.u.PostMessage.i.Msg = Msg;
    m.p.u.PostMessage.i.wParam = wParam;
    m.p.u.PostMessage.i.lParam = lParam;

    return PhSvcpCallServer(&m);
}

NTSTATUS PhSvcCallSendMessage(
    _In_opt_ HWND hWnd,
    _In_ UINT Msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PHSVC_API_MSG m;

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.p.ApiNumber = PhSvcSendMessageApiNumber;
    m.p.u.PostMessage.i.hWnd = hWnd;
    m.p.u.PostMessage.i.Msg = Msg;
    m.p.u.PostMessage.i.wParam = wParam;
    m.p.u.PostMessage.i.lParam = lParam;

    return PhSvcpCallServer(&m);
}

NTSTATUS PhSvcCallCreateProcessIgnoreIfeoDebugger(
    _In_ PWSTR FileName,
    _In_opt_ PWSTR CommandLine
    )
{
    NTSTATUS status;
    PHSVC_API_MSG m;
    PVOID fileName = NULL;
    PVOID commandLine = NULL;

    memset(&m, 0, sizeof(PHSVC_API_MSG));

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    m.p.ApiNumber = PhSvcCreateProcessIgnoreIfeoDebuggerApiNumber;
    fileName = PhSvcpCreateString(FileName, -1, &m.p.u.CreateProcessIgnoreIfeoDebugger.i.FileName);

    if (!fileName)
        return STATUS_NO_MEMORY;

    if (CommandLine)
    {
        commandLine = PhSvcpCreateString(CommandLine, -1, &m.p.u.CreateProcessIgnoreIfeoDebugger.i.CommandLine);

        if (!commandLine)
            return STATUS_NO_MEMORY;
    }

    status = PhSvcpCallServer(&m);

    if (commandLine)
        PhSvcpFreeHeap(commandLine);
    if (fileName)
        PhSvcpFreeHeap(fileName);

    return status;
}

_Success_(return != NULL)
PSECURITY_DESCRIPTOR PhpAbsoluteToSelfRelativeSD(
    _In_ PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
    _Out_ PULONG BufferSize
    )
{
    NTSTATUS status;
    ULONG bufferSize = 0;
    PSECURITY_DESCRIPTOR selfRelativeSecurityDescriptor;

    status = RtlAbsoluteToSelfRelativeSD(AbsoluteSecurityDescriptor, NULL, &bufferSize);

    if (status != STATUS_BUFFER_TOO_SMALL)
        return NULL;

    selfRelativeSecurityDescriptor = PhAllocate(bufferSize);
    status = RtlAbsoluteToSelfRelativeSD(AbsoluteSecurityDescriptor, selfRelativeSecurityDescriptor, &bufferSize);

    if (!NT_SUCCESS(status))
    {
        PhFree(selfRelativeSecurityDescriptor);
        return NULL;
    }

    *BufferSize = bufferSize;

    return selfRelativeSecurityDescriptor;
}

NTSTATUS PhSvcCallSetServiceSecurity(
    _In_ PWSTR ServiceName,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    NTSTATUS status;
    PHSVC_API_MSG m;
    PSECURITY_DESCRIPTOR selfRelativeSecurityDescriptor = NULL;
    ULONG bufferSize;
    PVOID serviceName = NULL;
    PVOID copiedSelfRelativeSecurityDescriptor = NULL;

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    selfRelativeSecurityDescriptor = PhpAbsoluteToSelfRelativeSD(SecurityDescriptor, &bufferSize);

    if (!selfRelativeSecurityDescriptor)
    {
        status = STATUS_BAD_DESCRIPTOR_FORMAT;
        goto CleanupExit;
    }

    m.p.ApiNumber = PhSvcSetServiceSecurityApiNumber;
    m.p.u.SetServiceSecurity.i.SecurityInformation = SecurityInformation;
    status = STATUS_NO_MEMORY;

    if (!(serviceName = PhSvcpCreateString(ServiceName, -1, &m.p.u.SetServiceSecurity.i.ServiceName)))
        goto CleanupExit;
    if (!(copiedSelfRelativeSecurityDescriptor = PhSvcpCreateString(selfRelativeSecurityDescriptor, bufferSize, &m.p.u.SetServiceSecurity.i.SecurityDescriptor)))
        goto CleanupExit;

    status = PhSvcpCallServer(&m);

CleanupExit:
    if (selfRelativeSecurityDescriptor) PhFree(selfRelativeSecurityDescriptor);
    if (serviceName) PhSvcpFreeHeap(serviceName);
    if (copiedSelfRelativeSecurityDescriptor) PhSvcpFreeHeap(copiedSelfRelativeSecurityDescriptor);

    return status;
}

NTSTATUS PhSvcCallWriteMiniDumpProcess(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE ProcessId,
    _In_ HANDLE FileHandle,
    _In_ ULONG DumpType
    )
{
    NTSTATUS status;
    PHSVC_API_MSG m;
    HANDLE serverHandle = NULL;
    HANDLE remoteProcessHandle = NULL;
    HANDLE remoteFileHandle = NULL;

    memset(&m, 0, sizeof(PHSVC_API_MSG));

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    // For typical uses of this function, the client has more privileges than the server.
    // We therefore duplicate our handles into the server's process.

    m.p.ApiNumber = PhSvcWriteMiniDumpProcessApiNumber;

    status = PhOpenProcess(
        &serverHandle,
        PROCESS_DUP_HANDLE,
        PhSvcClServerProcessId
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtDuplicateObject(
        NtCurrentProcess(),
        ProcessHandle,
        serverHandle,
        &remoteProcessHandle,
        0,
        0,
        DUPLICATE_SAME_ACCESS
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtDuplicateObject(
        NtCurrentProcess(),
        FileHandle,
        serverHandle,
        &remoteFileHandle,
        FILE_GENERIC_WRITE,
        0,
        0
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    m.p.u.WriteMiniDumpProcess.i.LocalProcessHandle = HandleToUlong(remoteProcessHandle);
    m.p.u.WriteMiniDumpProcess.i.ProcessId = HandleToUlong(ProcessId);
    m.p.u.WriteMiniDumpProcess.i.LocalFileHandle = HandleToUlong(remoteFileHandle);
    m.p.u.WriteMiniDumpProcess.i.DumpType = DumpType;

    status = PhSvcpCallServer(&m);

CleanupExit:
    if (serverHandle)
    {
        if (remoteProcessHandle)
            NtDuplicateObject(serverHandle, remoteProcessHandle, NULL, NULL, 0, 0, DUPLICATE_CLOSE_SOURCE);
        if (remoteFileHandle)
            NtDuplicateObject(serverHandle, remoteFileHandle, NULL, NULL, 0, 0, DUPLICATE_CLOSE_SOURCE);

        NtClose(serverHandle);
    }

    return status;
}

NTSTATUS PhSvcCallQueryProcessHeapInformation(
    _In_ HANDLE ProcessId,
    _Out_ PPH_STRING* HeapInformation
    )
{
    NTSTATUS status;
    PHSVC_API_MSG m;
    ULONG bufferSize;
    PVOID buffer;

    if (!PhSvcClPortHandle)
        return STATUS_PORT_DISCONNECTED;

    memset(&m, 0, sizeof(PHSVC_API_MSG));
    m.p.ApiNumber = PhSvcQueryProcessDebugInformationApiNumber;
    m.p.u.QueryProcessHeap.i.ProcessId = HandleToUlong(ProcessId);

    bufferSize = 0x1000;

    if (!(buffer = PhSvcpCreateString(NULL, bufferSize, &m.p.u.QueryProcessHeap.i.Data)))
        return STATUS_FAIL_CHECK;

    status = PhSvcpCallServer(&m);

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        PhSvcpFreeHeap(buffer);
        bufferSize = m.p.u.QueryProcessHeap.o.DataLength;

        if (!(buffer = PhSvcpCreateString(NULL, bufferSize, &m.p.u.QueryProcessHeap.i.Data)))
            return STATUS_FAIL_CHECK;

        status = PhSvcpCallServer(&m);
    }

    if (NT_SUCCESS(status))
    {
        if (HeapInformation)
            *HeapInformation = PhCreateStringEx(buffer, m.p.u.QueryProcessHeap.o.DataLength);
    }

    PhSvcpFreeHeap(buffer);

    return status;
}
