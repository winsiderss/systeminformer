/*
 * Process Hacker - 
 *   server API
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
#include <phsvc.h>

PPHSVC_API_PROCEDURE PhSvcApiCallTable[] =
{
    PhSvcApiClose,
    PhSvcApiExecuteRunAsCommand,
    PhSvcApiUnloadDriver,
    PhSvcApiControlProcess,
    PhSvcApiControlService,
    PhSvcApiCreateService,
    PhSvcApiChangeServiceConfig,
    PhSvcApiChangeServiceConfig2
};

NTSTATUS PhSvcApiInitialization()
{
    return STATUS_SUCCESS;
}

VOID PhSvcDispatchApiCall(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message,
    __out PPHSVC_API_MSG *ReplyMessage,
    __out PHANDLE ReplyPortHandle
    )
{
    NTSTATUS status;

    if (
        Message->ApiNumber == 0 ||
        (ULONG)Message->ApiNumber >= (ULONG)PhSvcMaximumApiNumber ||
        !PhSvcApiCallTable[Message->ApiNumber - 1]
        )
    {
        Message->ReturnStatus = STATUS_INVALID_SYSTEM_SERVICE;
        *ReplyMessage = Message;
        return;
    }

    status = PhSvcApiCallTable[Message->ApiNumber - 1](Client, Message);
    Message->ReturnStatus = status;

    *ReplyMessage = Message;
    *ReplyPortHandle = Client->PortHandle;
}

NTSTATUS PhSvcCaptureBuffer(
    __in PPH_RELATIVE_STRINGREF String,
    __in BOOLEAN AllowNull,
    __out PVOID *CapturedBuffer
    )
{
    PPHSVC_CLIENT client = PhSvcGetCurrentClient();
    PVOID address;
    PVOID buffer;

    if (String->Offset != 0)
    {
        address = (PCHAR)client->ClientViewBase + String->Offset;

        if (
            (ULONG_PTR)address + String->Length < (ULONG_PTR)address ||
            (ULONG_PTR)address < (ULONG_PTR)client->ClientViewBase ||
            (ULONG_PTR)address + String->Length >= (ULONG_PTR)client->ClientViewLimit
            )
        {
            return STATUS_ACCESS_VIOLATION;
        }

        buffer = PhAllocateSafe(String->Length);

        if (!buffer)
            return STATUS_NO_MEMORY;

        memcpy(buffer, address, String->Length);
        *CapturedBuffer = buffer;
    }
    else
    {
        if (!AllowNull)
            return STATUS_ACCESS_VIOLATION;

        *CapturedBuffer = NULL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS PhSvcCaptureString(
    __in PPH_RELATIVE_STRINGREF String,
    __in BOOLEAN AllowNull,
    __out PPH_STRING *CapturedString
    )
{
    PPHSVC_CLIENT client = PhSvcGetCurrentClient();
    PVOID address;

    if (String->Length & 1)
        return STATUS_INVALID_BUFFER_SIZE;
    if (String->Length > 0xfffe)
        return STATUS_INVALID_BUFFER_SIZE;

    if (String->Offset != 0)
    {
        address = (PCHAR)client->ClientViewBase + String->Offset;

        if ((ULONG_PTR)address & 1)
        {
            return STATUS_DATATYPE_MISALIGNMENT;
        }

        if (
            (ULONG_PTR)address + String->Length < (ULONG_PTR)address ||
            (ULONG_PTR)address < (ULONG_PTR)client->ClientViewBase ||
            (ULONG_PTR)address + String->Length >= (ULONG_PTR)client->ClientViewLimit
            )
        {
            return STATUS_ACCESS_VIOLATION;
        }

        if (String->Length != 0)
            *CapturedString = PhCreateStringEx(address, String->Length);
        else
            *CapturedString = PhReferenceEmptyString();
    }
    else
    {
        if (!AllowNull)
            return STATUS_ACCESS_VIOLATION;

        *CapturedString = NULL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS PhSvcApiClose(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    )
{
    return PhSvcCloseHandle(Message->u.Close.i.Handle);
}

NTSTATUS PhSvcApiExecuteRunAsCommand(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    )
{
    NTSTATUS status;
    PPH_STRING serviceCommandLine;
    PPH_STRING serviceName;

    if (NT_SUCCESS(status = PhSvcCaptureString(&Message->u.ExecuteRunAsCommand.i.ServiceCommandLine, FALSE, &serviceCommandLine)))
    {
        if (NT_SUCCESS(status = PhSvcCaptureString(&Message->u.ExecuteRunAsCommand.i.ServiceName, FALSE, &serviceName)))
        {
            status = PhExecuteRunAsCommand(serviceCommandLine->Buffer, serviceName->Buffer);
            PhDereferenceObject(serviceName);
        }

        PhDereferenceObject(serviceCommandLine);
    }

    return status;
}

NTSTATUS PhSvcApiUnloadDriver(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    )
{
    NTSTATUS status;
    PPH_STRING name;

    if (NT_SUCCESS(status = PhSvcCaptureString(&Message->u.UnloadDriver.i.Name, TRUE, &name)))
    {
        status = PhUnloadDriver(Message->u.UnloadDriver.i.BaseAddress, PhGetString(name));
        PhSwapReference(&name, NULL);
    }

    return status;
}

NTSTATUS PhSvcApiControlProcess(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    )
{
    NTSTATUS status;
    HANDLE processId;
    HANDLE processHandle;

    processId = Message->u.ControlProcess.i.ProcessId;

    switch (Message->u.ControlProcess.i.Command)
    {
    case PhSvcControlProcessTerminate:
        if (NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_TERMINATE, processId)))
        {
            status = PhTerminateProcess(processHandle, STATUS_SUCCESS);
            NtClose(processHandle);
        }
        break;
    case PhSvcControlProcessSuspend:
        if (NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_SUSPEND_RESUME, processId)))
        {
            status = PhSuspendProcess(processHandle);
            NtClose(processHandle);
        }
        break;
    case PhSvcControlProcessResume:
        if (NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_SUSPEND_RESUME, processId)))
        {
            status = PhResumeProcess(processHandle);
            NtClose(processHandle);
        }
        break;
    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

    return status;
}

NTSTATUS PhSvcApiControlService(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    )
{
    NTSTATUS status;
    PPH_STRING serviceName;
    SC_HANDLE serviceHandle;
    SERVICE_STATUS serviceStatus;

    if (NT_SUCCESS(status = PhSvcCaptureString(&Message->u.ControlService.i.ServiceName, FALSE, &serviceName)))
    {
        switch (Message->u.ControlService.i.Command)
        {
        case PhSvcControlServiceStart:
            if (serviceHandle = PhOpenService(
                serviceName->Buffer,
                SERVICE_START
                ))
            {
                if (!StartService(serviceHandle, 0, NULL))
                    status = PhGetLastWin32ErrorAsNtStatus();

                CloseServiceHandle(serviceHandle);
            }
            else
            {
                status = PhGetLastWin32ErrorAsNtStatus();
            }
            break;
        case PhSvcControlServiceContinue:
            if (serviceHandle = PhOpenService(
                serviceName->Buffer,
                SERVICE_PAUSE_CONTINUE
                ))
            {
                if (!ControlService(serviceHandle, SERVICE_CONTROL_CONTINUE, &serviceStatus))
                    status = PhGetLastWin32ErrorAsNtStatus();

                CloseServiceHandle(serviceHandle);
            }
            else
            {
                status = PhGetLastWin32ErrorAsNtStatus();
            }
            break;
        case PhSvcControlServicePause:
            if (serviceHandle = PhOpenService(
                serviceName->Buffer,
                SERVICE_PAUSE_CONTINUE
                ))
            {
                if (!ControlService(serviceHandle, SERVICE_CONTROL_PAUSE, &serviceStatus))
                    status = PhGetLastWin32ErrorAsNtStatus();

                CloseServiceHandle(serviceHandle);
            }
            else
            {
                status = PhGetLastWin32ErrorAsNtStatus();
            }
            break;
        case PhSvcControlServiceStop:
            if (serviceHandle = PhOpenService(
                serviceName->Buffer,
                SERVICE_STOP
                ))
            {
                if (!ControlService(serviceHandle, SERVICE_CONTROL_STOP, &serviceStatus))
                    status = PhGetLastWin32ErrorAsNtStatus();

                CloseServiceHandle(serviceHandle);
            }
            else
            {
                status = PhGetLastWin32ErrorAsNtStatus();
            }
            break;
        case PhSvcControlServiceDelete:
            if (serviceHandle = PhOpenService(
                serviceName->Buffer,
                DELETE
                ))
            {
                if (!DeleteService(serviceHandle))
                    status = PhGetLastWin32ErrorAsNtStatus();

                CloseServiceHandle(serviceHandle);
            }
            else
            {
                status = PhGetLastWin32ErrorAsNtStatus();
            }
            break;
        default:
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        PhDereferenceObject(serviceName);
    }

    return status;
}

NTSTATUS PhSvcApiCreateService(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    )
{
    NTSTATUS status;
    PPH_STRING serviceName = NULL;
    PPH_STRING displayName = NULL;
    PPH_STRING binaryPathName = NULL;
    PPH_STRING loadOrderGroup = NULL;
    PPH_STRING dependencies = NULL;
    PPH_STRING serviceStartName = NULL;
    PPH_STRING password = NULL;
    ULONG tagId = 0;
    SC_HANDLE scManagerHandle;
    SC_HANDLE serviceHandle;

    if (!NT_SUCCESS(status = PhSvcCaptureString(&Message->u.CreateService.i.ServiceName, FALSE, &serviceName)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Message->u.CreateService.i.DisplayName, TRUE, &displayName)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Message->u.CreateService.i.BinaryPathName, TRUE, &binaryPathName)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Message->u.CreateService.i.LoadOrderGroup, TRUE, &loadOrderGroup)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Message->u.CreateService.i.Dependencies, TRUE, &dependencies)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Message->u.CreateService.i.ServiceStartName, TRUE, &serviceStartName)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Message->u.CreateService.i.Password, TRUE, &password)))
        goto CleanupExit;

    if (scManagerHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE))
    {
        if (serviceHandle = CreateService(
            scManagerHandle,
            serviceName->Buffer,
            PhGetString(displayName),
            SERVICE_CHANGE_CONFIG,
            Message->u.CreateService.i.ServiceType,
            Message->u.CreateService.i.StartType,
            Message->u.CreateService.i.ErrorControl,
            PhGetString(binaryPathName),
            PhGetString(loadOrderGroup),
            Message->u.CreateService.i.TagIdSpecified ? &tagId : NULL,
            PhGetString(dependencies),
            PhGetString(serviceStartName),
            PhGetString(password)
            ))
        {
            Message->u.CreateService.o.TagId = tagId;
            CloseServiceHandle(serviceHandle);
        }
        else
        {
            status = PhGetLastWin32ErrorAsNtStatus();
        }

        CloseServiceHandle(scManagerHandle);
    }
    else
    {
        status = PhGetLastWin32ErrorAsNtStatus();
    }

CleanupExit:
    PhSwapReference(&password, NULL);
    PhSwapReference(&serviceStartName, NULL);
    PhSwapReference(&dependencies, NULL);
    PhSwapReference(&loadOrderGroup, NULL);
    PhSwapReference(&binaryPathName, NULL);
    PhSwapReference(&displayName, NULL);
    PhSwapReference(&serviceName, NULL);

    return status;
}

NTSTATUS PhSvcApiChangeServiceConfig(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    )
{
    NTSTATUS status;
    PPH_STRING serviceName = NULL;
    PPH_STRING binaryPathName = NULL;
    PPH_STRING loadOrderGroup = NULL;
    PPH_STRING dependencies = NULL;
    PPH_STRING serviceStartName = NULL;
    PPH_STRING password = NULL;
    PPH_STRING displayName = NULL;
    ULONG tagId = 0;
    SC_HANDLE serviceHandle;

    if (!NT_SUCCESS(status = PhSvcCaptureString(&Message->u.ChangeServiceConfig.i.ServiceName, FALSE, &serviceName)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Message->u.ChangeServiceConfig.i.BinaryPathName, TRUE, &binaryPathName)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Message->u.ChangeServiceConfig.i.LoadOrderGroup, TRUE, &loadOrderGroup)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Message->u.ChangeServiceConfig.i.Dependencies, TRUE, &dependencies)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Message->u.ChangeServiceConfig.i.ServiceStartName, TRUE, &serviceStartName)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Message->u.ChangeServiceConfig.i.Password, TRUE, &password)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Message->u.ChangeServiceConfig.i.DisplayName, TRUE, &displayName)))
        goto CleanupExit;

    if (serviceHandle = PhOpenService(serviceName->Buffer, SERVICE_CHANGE_CONFIG))
    {
        if (ChangeServiceConfig(
            serviceHandle,
            Message->u.ChangeServiceConfig.i.ServiceType,
            Message->u.ChangeServiceConfig.i.StartType,
            Message->u.ChangeServiceConfig.i.ErrorControl,
            PhGetString(binaryPathName),
            PhGetString(loadOrderGroup),
            Message->u.ChangeServiceConfig.i.TagIdSpecified ? &tagId : NULL,
            PhGetString(dependencies),
            PhGetString(serviceStartName),
            PhGetString(password),
            PhGetString(displayName)
            ))
        {
            Message->u.ChangeServiceConfig.o.TagId = tagId;
        }
        else
        {
            status = PhGetLastWin32ErrorAsNtStatus();
        }

        CloseServiceHandle(serviceHandle);
    }
    else
    {
        status = PhGetLastWin32ErrorAsNtStatus();
    }

CleanupExit:
    PhSwapReference(&displayName, NULL);
    PhSwapReference(&password, NULL);
    PhSwapReference(&serviceStartName, NULL);
    PhSwapReference(&dependencies, NULL);
    PhSwapReference(&loadOrderGroup, NULL);
    PhSwapReference(&binaryPathName, NULL);
    PhSwapReference(&serviceName, NULL);

    return status;
}

NTSTATUS PhSvcApiChangeServiceConfig2(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    )
{
    NTSTATUS status;
    PPH_STRING serviceName;
    PVOID info;
    SC_HANDLE serviceHandle;

    if (NT_SUCCESS(status = PhSvcCaptureString(&Message->u.ChangeServiceConfig2.i.ServiceName, FALSE, &serviceName)))
    {
        switch (Message->u.ChangeServiceConfig2.i.InfoLevel)
        {
        case SERVICE_CONFIG_DELAYED_AUTO_START_INFO:
            if (NT_SUCCESS(status = PhSvcCaptureBuffer(&Message->u.ChangeServiceConfig2.i.Info, FALSE, &info)))
            {
                if (serviceHandle = PhOpenService(serviceName->Buffer, SERVICE_CHANGE_CONFIG))
                {
                    if (!ChangeServiceConfig2(
                        serviceHandle,
                        SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
                        (LPSERVICE_DELAYED_AUTO_START_INFO)info
                        ))
                    {
                        status = PhGetLastWin32ErrorAsNtStatus();
                    }

                    CloseServiceHandle(serviceHandle);
                }
                else
                {
                    status = PhGetLastWin32ErrorAsNtStatus();
                }

                PhFree(info);
            }
            break;
        default:
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        PhDereferenceObject(serviceName);
    }

    return status;
}
