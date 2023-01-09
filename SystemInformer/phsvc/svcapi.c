/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2015
 *     dmex    2019-2023
 *
 */

#include <phapp.h>
#include <phsvc.h>
#include <extmgri.h>
#include <lsasup.h>
#include <phplug.h>
#include <secedit.h>
#include <svcsup.h>

#include <accctrl.h>
#include <dbghelp.h>
#include <symprv.h>

typedef struct _PHSVCP_CAPTURED_RUNAS_SERVICE_PARAMETERS
{
    PPH_STRING UserName;
    PPH_STRING Password;
    PPH_STRING CurrentDirectory;
    PPH_STRING CommandLine;
    PPH_STRING FileName;
    PPH_STRING DesktopName;
    PPH_STRING ServiceName;
} PHSVCP_CAPTURED_RUNAS_SERVICE_PARAMETERS, *PPHSVCP_CAPTURED_RUNAS_SERVICE_PARAMETERS;

PPHSVC_API_PROCEDURE PhSvcApiCallTable[] =
{
    PhSvcApiPlugin,
    PhSvcApiExecuteRunAsCommand,
    PhSvcApiUnloadDriver,
    PhSvcApiControlProcess,
    PhSvcApiControlService,
    PhSvcApiCreateService,
    PhSvcApiChangeServiceConfig,
    PhSvcApiChangeServiceConfig2,
    PhSvcApiSetTcpEntry,
    PhSvcApiControlThread,
    PhSvcApiAddAccountRight,
    PhSvcApiInvokeRunAsService,
    PhSvcApiIssueMemoryListCommand,
    PhSvcApiPostMessage,
    PhSvcApiSendMessage,
    PhSvcApiCreateProcessIgnoreIfeoDebugger,
    PhSvcApiSetServiceSecurity,
    PhSvcApiWriteMiniDumpProcess,
    PhSvcApiQueryProcessHeapInformation
};
C_ASSERT(sizeof(PhSvcApiCallTable) / sizeof(PPHSVC_API_PROCEDURE) == PhSvcMaximumApiNumber - 1);

NTSTATUS PhSvcApiInitialization(
    VOID
    )
{
    return STATUS_SUCCESS;
}

VOID PhSvcDispatchApiCall(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload,
    _Out_ PHANDLE ReplyPortHandle
    )
{
    NTSTATUS status;

    if (
        Payload->ApiNumber == 0 ||
        (ULONG)Payload->ApiNumber >= (ULONG)PhSvcMaximumApiNumber ||
        !PhSvcApiCallTable[Payload->ApiNumber - 1]
        )
    {
        Payload->ReturnStatus = STATUS_INVALID_SYSTEM_SERVICE;
        *ReplyPortHandle = Client->PortHandle;
        return;
    }

    status = PhSvcApiCallTable[Payload->ApiNumber - 1](Client, Payload);
    Payload->ReturnStatus = status;

    *ReplyPortHandle = Client->PortHandle;
}

PVOID PhSvcValidateString(
    _In_ PPH_RELATIVE_STRINGREF String,
    _In_ ULONG Alignment
    )
{
    PPHSVC_CLIENT client = PhSvcGetCurrentClient();
    PVOID address;

    address = PTR_ADD_OFFSET(client->ClientViewBase, String->Offset);

    if ((ULONG_PTR)address + String->Length < (ULONG_PTR)address ||
        (ULONG_PTR)address < (ULONG_PTR)client->ClientViewBase ||
        (ULONG_PTR)address + String->Length > (ULONG_PTR)client->ClientViewLimit)
    {
        return NULL;
    }

    if ((ULONG_PTR)address & (Alignment - 1))
    {
        return NULL;
    }

    return address;
}

NTSTATUS PhSvcProbeBuffer(
    _In_ PPH_RELATIVE_STRINGREF String,
    _In_ ULONG Alignment,
    _In_ BOOLEAN AllowNull,
    _Out_ PVOID *Pointer
    )
{
    PVOID address;

    if (String->Offset != 0)
    {
        address = PhSvcValidateString(String, Alignment);

        if (!address)
            return STATUS_ACCESS_VIOLATION;

        *Pointer = address;
    }
    else
    {
        if (!AllowNull)
            return STATUS_ACCESS_VIOLATION;

        *Pointer = NULL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS PhSvcCaptureBuffer(
    _In_ PPH_RELATIVE_STRINGREF String,
    _In_ BOOLEAN AllowNull,
    _Out_ PVOID *CapturedBuffer
    )
{
    PVOID address;
    PVOID buffer;

    if (String->Offset != 0)
    {
        address = PhSvcValidateString(String, 1);

        if (!address)
            return STATUS_ACCESS_VIOLATION;

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
    _In_ PPH_RELATIVE_STRINGREF String,
    _In_ BOOLEAN AllowNull,
    _Out_ PPH_STRING *CapturedString
    )
{
    PVOID address;

    if (String->Length & 1)
        return STATUS_INVALID_BUFFER_SIZE;
    if (String->Length > 0xfffe)
        return STATUS_INVALID_BUFFER_SIZE;

    if (String->Offset != 0)
    {
        address = PhSvcValidateString(String, sizeof(WCHAR));

        if (!address)
            return STATUS_ACCESS_VIOLATION;

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

NTSTATUS PhSvcCaptureSid(
    _In_ PPH_RELATIVE_STRINGREF String,
    _In_ BOOLEAN AllowNull,
    _Out_ PSID *CapturedSid
    )
{
    NTSTATUS status;
    PSID sid;

    if (!NT_SUCCESS(status = PhSvcCaptureBuffer(String, AllowNull, &sid)))
        return status;

    if (sid)
    {
        if (String->Length < UFIELD_OFFSET(struct _SID, IdentifierAuthority) ||
            String->Length < RtlLengthRequiredSid(((struct _SID *)sid)->SubAuthorityCount) ||
            !RtlValidSid(sid))
        {
            PhFree(sid);
            return STATUS_INVALID_SID;
        }

        *CapturedSid = sid;
    }
    else
    {
        *CapturedSid = NULL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS PhSvcCaptureSecurityDescriptor(
    _In_ PPH_RELATIVE_STRINGREF String,
    _In_ BOOLEAN AllowNull,
    _In_ SECURITY_INFORMATION RequiredInformation,
    _Out_ PSECURITY_DESCRIPTOR *CapturedSecurityDescriptor
    )
{
    NTSTATUS status;
    PSECURITY_DESCRIPTOR securityDescriptor;
    ULONG bufferSize;

    if (!NT_SUCCESS(status = PhSvcCaptureBuffer(String, AllowNull, &securityDescriptor)))
        return status;

    if (securityDescriptor)
    {
        if (!RtlValidRelativeSecurityDescriptor(securityDescriptor, String->Length, RequiredInformation))
        {
            PhFree(securityDescriptor);
            return STATUS_INVALID_SECURITY_DESCR;
        }

        bufferSize = String->Length;
        status = RtlSelfRelativeToAbsoluteSD2(securityDescriptor, &bufferSize);

        if (status == STATUS_BUFFER_TOO_SMALL)
        {
            PVOID newBuffer;

            newBuffer = PhAllocate(bufferSize);
            memcpy(newBuffer, securityDescriptor, String->Length);
            PhFree(securityDescriptor);
            securityDescriptor = newBuffer;

            status = RtlSelfRelativeToAbsoluteSD2(securityDescriptor, &bufferSize);
        }

        if (!NT_SUCCESS(status))
        {
            PhFree(securityDescriptor);
            return status;
        }

        *CapturedSecurityDescriptor = securityDescriptor;
    }
    else
    {
        *CapturedSecurityDescriptor = NULL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS PhSvcApiDefault(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS PhSvcApiPlugin(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    )
{
    NTSTATUS status;
    PPH_STRING apiId;
    PPH_PLUGIN plugin;
    PH_STRINGREF pluginName;
    PH_PLUGIN_PHSVC_REQUEST request;

    if (NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.Plugin.i.ApiId, FALSE, &apiId)))
    {
        PH_AUTO(apiId);

        if (PhPluginsEnabled &&
            PhEmParseCompoundId(&apiId->sr, &pluginName, &request.SubId) &&
            (plugin = PhFindPlugin2(&pluginName)))
        {
            request.ReturnStatus = STATUS_NOT_IMPLEMENTED;
            request.InBuffer = Payload->u.Plugin.i.Data;
            request.InLength = sizeof(Payload->u.Plugin.i.Data);
            request.OutBuffer = Payload->u.Plugin.o.Data;
            request.OutLength = sizeof(Payload->u.Plugin.o.Data);

            request.ProbeBuffer = PhSvcProbeBuffer;
            request.CaptureBuffer = PhSvcCaptureBuffer;
            request.CaptureString = PhSvcCaptureString;

            PhInvokeCallback(PhGetPluginCallback(plugin, PluginCallbackPhSvcRequest), &request);
            status = request.ReturnStatus;
        }
        else
        {
            status = STATUS_NOT_FOUND;
        }
    }

    return status;
}

NTSTATUS PhSvcpCaptureRunAsServiceParameters(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload,
    _Out_ PPH_RUNAS_SERVICE_PARAMETERS Parameters,
    _Out_ PPHSVCP_CAPTURED_RUNAS_SERVICE_PARAMETERS CapturedParameters
    )
{
    NTSTATUS status;

    memset(CapturedParameters, 0, sizeof(PHSVCP_CAPTURED_RUNAS_SERVICE_PARAMETERS));

    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.ExecuteRunAsCommand.i.UserName, TRUE, &CapturedParameters->UserName)))
        return status;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.ExecuteRunAsCommand.i.Password, TRUE, &CapturedParameters->Password)))
        return status;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.ExecuteRunAsCommand.i.CurrentDirectory, TRUE, &CapturedParameters->CurrentDirectory)))
        return status;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.ExecuteRunAsCommand.i.CommandLine, TRUE, &CapturedParameters->CommandLine)))
        return status;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.ExecuteRunAsCommand.i.FileName, TRUE, &CapturedParameters->FileName)))
        return status;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.ExecuteRunAsCommand.i.DesktopName, TRUE, &CapturedParameters->DesktopName)))
        return status;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.ExecuteRunAsCommand.i.ServiceName, TRUE, &CapturedParameters->ServiceName)))
        return status;

    Parameters->ProcessId = Payload->u.ExecuteRunAsCommand.i.ProcessId;
    Parameters->UserName = PhGetString(CapturedParameters->UserName);
    Parameters->Password = PhGetString(CapturedParameters->Password);
    Parameters->LogonType = Payload->u.ExecuteRunAsCommand.i.LogonType;
    Parameters->SessionId = Payload->u.ExecuteRunAsCommand.i.SessionId;
    Parameters->CurrentDirectory = PhGetString(CapturedParameters->CurrentDirectory);
    Parameters->CommandLine = PhGetString(CapturedParameters->CommandLine);
    Parameters->FileName = PhGetString(CapturedParameters->FileName);
    Parameters->DesktopName = PhGetString(CapturedParameters->DesktopName);
    Parameters->UseLinkedToken = Payload->u.ExecuteRunAsCommand.i.UseLinkedToken;
    Parameters->ServiceName = PhGetString(CapturedParameters->ServiceName);
    Parameters->CreateSuspendedProcess = Payload->u.ExecuteRunAsCommand.i.CreateSuspendedProcess;

    return status;
}

VOID PhSvcpReleaseRunAsServiceParameters(
    _In_ PPHSVCP_CAPTURED_RUNAS_SERVICE_PARAMETERS CapturedParameters
    )
{
    if (CapturedParameters->UserName)
        PhDereferenceObject(CapturedParameters->UserName);

    if (CapturedParameters->Password)
    {
        RtlSecureZeroMemory(CapturedParameters->Password->Buffer, CapturedParameters->Password->Length);
        PhDereferenceObject(CapturedParameters->Password);
    }

    if (CapturedParameters->CurrentDirectory)
        PhDereferenceObject(CapturedParameters->CurrentDirectory);
    if (CapturedParameters->CommandLine)
        PhDereferenceObject(CapturedParameters->CommandLine);
    if (CapturedParameters->FileName)
        PhDereferenceObject(CapturedParameters->FileName);
    if (CapturedParameters->DesktopName)
        PhDereferenceObject(CapturedParameters->DesktopName);
    if (CapturedParameters->ServiceName)
        PhDereferenceObject(CapturedParameters->ServiceName);
}

NTSTATUS PhSvcpValidateRunAsServiceParameters(
    _In_ PPH_RUNAS_SERVICE_PARAMETERS Parameters
    )
{
    if ((!Parameters->UserName || !Parameters->Password) && !Parameters->ProcessId)
        return STATUS_INVALID_PARAMETER_MIX;
    if (!Parameters->FileName && !Parameters->CommandLine)
        return STATUS_INVALID_PARAMETER_MIX;
    if (!Parameters->ServiceName)
        return STATUS_INVALID_PARAMETER;

    return STATUS_SUCCESS;
}

NTSTATUS PhSvcApiExecuteRunAsCommand(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    )
{
    NTSTATUS status;
    PH_RUNAS_SERVICE_PARAMETERS parameters;
    PHSVCP_CAPTURED_RUNAS_SERVICE_PARAMETERS capturedParameters;

    if (NT_SUCCESS(status = PhSvcpCaptureRunAsServiceParameters(Client, Payload, &parameters, &capturedParameters)))
    {
        if (NT_SUCCESS(status = PhSvcpValidateRunAsServiceParameters(&parameters)))
        {
            status = PhExecuteRunAsCommand(&parameters);
        }
    }

    PhSvcpReleaseRunAsServiceParameters(&capturedParameters);

    return status;
}

NTSTATUS PhSvcApiUnloadDriver(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    )
{
    NTSTATUS status;
    PPH_STRING name;

    if (NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.UnloadDriver.i.Name, TRUE, &name)))
    {
        status = PhUnloadDriver(Payload->u.UnloadDriver.i.BaseAddress, PhGetString(name));
        PhClearReference(&name);
    }

    return status;
}

NTSTATUS PhSvcApiControlProcess(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    )
{
    NTSTATUS status;
    HANDLE processId;
    HANDLE processHandle;

    processId = Payload->u.ControlProcess.i.ProcessId;

    switch (Payload->u.ControlProcess.i.Command)
    {
    case PhSvcControlProcessTerminate:
        if (NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_TERMINATE, processId)))
        {
            status = PhTerminateProcess(processHandle, 1); // see notes in PhUiTerminateProcesses
            NtClose(processHandle);
        }
        break;
    case PhSvcControlProcessSuspend:
        if (NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_SUSPEND_RESUME, processId)))
        {
            status = NtSuspendProcess(processHandle);
            NtClose(processHandle);
        }
        break;
    case PhSvcControlProcessResume:
        if (NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_SUSPEND_RESUME, processId)))
        {
            status = NtResumeProcess(processHandle);
            NtClose(processHandle);
        }
        break;
    case PhSvcControlProcessPriority:
        if (processId != SYSTEM_PROCESS_ID)
        {
            if (NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_SET_INFORMATION, processId)))
            {
                UCHAR priorityClass;

                priorityClass = (UCHAR)Payload->u.ControlProcess.i.Argument;
                status = PhSetProcessPriority(processHandle, priorityClass);

                NtClose(processHandle);
            }
        }
        else
        {
            status = STATUS_UNSUCCESSFUL;
        }
        break;
    case PhSvcControlProcessIoPriority:
        if (processId != SYSTEM_PROCESS_ID)
        {
            if (NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_SET_INFORMATION, processId)))
            {
                status = PhSetProcessIoPriority(processHandle, Payload->u.ControlProcess.i.Argument);
                NtClose(processHandle);
            }
        }
        else
        {
            status = STATUS_UNSUCCESSFUL;
        }
        break;
    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

    return status;
}

NTSTATUS PhSvcApiControlService(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    )
{
    NTSTATUS status;
    PPH_STRING serviceName;
    SC_HANDLE serviceHandle;
    SERVICE_STATUS serviceStatus;

    if (NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.ControlService.i.ServiceName, FALSE, &serviceName)))
    {
        PH_AUTO(serviceName);

        switch (Payload->u.ControlService.i.Command)
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
    }

    return status;
}

NTSTATUS PhSvcApiCreateService(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
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

    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.CreateService.i.ServiceName, FALSE, &serviceName)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.CreateService.i.DisplayName, TRUE, &displayName)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.CreateService.i.BinaryPathName, TRUE, &binaryPathName)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.CreateService.i.LoadOrderGroup, TRUE, &loadOrderGroup)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.CreateService.i.Dependencies, TRUE, &dependencies)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.CreateService.i.ServiceStartName, TRUE, &serviceStartName)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.CreateService.i.Password, TRUE, &password)))
        goto CleanupExit;

    if (scManagerHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE))
    {
        if (serviceHandle = CreateService(
            scManagerHandle,
            serviceName->Buffer,
            PhGetString(displayName),
            SERVICE_CHANGE_CONFIG,
            Payload->u.CreateService.i.ServiceType,
            Payload->u.CreateService.i.StartType,
            Payload->u.CreateService.i.ErrorControl,
            PhGetString(binaryPathName),
            PhGetString(loadOrderGroup),
            Payload->u.CreateService.i.TagIdSpecified ? &tagId : NULL,
            PhGetString(dependencies),
            PhGetString(serviceStartName),
            PhGetString(password)
            ))
        {
            Payload->u.CreateService.o.TagId = tagId;
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
    if (password)
    {
        RtlSecureZeroMemory(password->Buffer, password->Length);
        PhDereferenceObject(password);
    }

    PhClearReference(&serviceStartName);
    PhClearReference(&dependencies);
    PhClearReference(&loadOrderGroup);
    PhClearReference(&binaryPathName);
    PhClearReference(&displayName);
    PhClearReference(&serviceName);

    return status;
}

NTSTATUS PhSvcApiChangeServiceConfig(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
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

    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.ChangeServiceConfig.i.ServiceName, FALSE, &serviceName)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.ChangeServiceConfig.i.BinaryPathName, TRUE, &binaryPathName)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.ChangeServiceConfig.i.LoadOrderGroup, TRUE, &loadOrderGroup)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.ChangeServiceConfig.i.Dependencies, TRUE, &dependencies)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.ChangeServiceConfig.i.ServiceStartName, TRUE, &serviceStartName)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.ChangeServiceConfig.i.Password, TRUE, &password)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.ChangeServiceConfig.i.DisplayName, TRUE, &displayName)))
        goto CleanupExit;

    if (serviceHandle = PhOpenService(serviceName->Buffer, SERVICE_CHANGE_CONFIG))
    {
        if (ChangeServiceConfig(
            serviceHandle,
            Payload->u.ChangeServiceConfig.i.ServiceType,
            Payload->u.ChangeServiceConfig.i.StartType,
            Payload->u.ChangeServiceConfig.i.ErrorControl,
            PhGetString(binaryPathName),
            PhGetString(loadOrderGroup),
            Payload->u.ChangeServiceConfig.i.TagIdSpecified ? &tagId : NULL,
            PhGetString(dependencies),
            PhGetString(serviceStartName),
            PhGetString(password),
            PhGetString(displayName)
            ))
        {
            Payload->u.ChangeServiceConfig.o.TagId = tagId;
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
    PhClearReference(&displayName);

    if (password)
    {
        RtlSecureZeroMemory(password->Buffer, password->Length);
        PhDereferenceObject(password);
    }

    PhClearReference(&serviceStartName);
    PhClearReference(&dependencies);
    PhClearReference(&loadOrderGroup);
    PhClearReference(&binaryPathName);
    PhClearReference(&serviceName);

    return status;
}

NTSTATUS PhSvcpUnpackRoot(
    _In_ PPH_RELATIVE_STRINGREF PackedData,
    _In_ PVOID CapturedBuffer,
    _In_ SIZE_T Length,
    _Out_ PVOID *ValidatedBuffer
    )
{
    if (Length > PackedData->Length)
        return STATUS_ACCESS_VIOLATION;

    *ValidatedBuffer = CapturedBuffer;

    return STATUS_SUCCESS;
}

NTSTATUS PhSvcpUnpackBuffer(
    _In_ PPH_RELATIVE_STRINGREF PackedData,
    _In_ PVOID CapturedBuffer,
    _In_ PVOID *OffsetInBuffer,
    _In_ SIZE_T Length,
    _In_ ULONG Alignment,
    _In_ BOOLEAN AllowNull
    )
{
    SIZE_T offset;

    offset = (SIZE_T)*OffsetInBuffer;

    if (offset == 0)
    {
        if (AllowNull)
            return STATUS_SUCCESS;
        else
            return STATUS_ACCESS_VIOLATION;
    }

    if (offset + Length < offset)
        return STATUS_ACCESS_VIOLATION;
    if (offset + Length > PackedData->Length)
        return STATUS_ACCESS_VIOLATION;
    if (offset & (Alignment - 1))
        return STATUS_DATATYPE_MISALIGNMENT;

    *OffsetInBuffer = PTR_ADD_OFFSET(CapturedBuffer, offset);

    return STATUS_SUCCESS;
}

NTSTATUS PhSvcpUnpackStringZ(
    _In_ PPH_RELATIVE_STRINGREF PackedData,
    _In_ PVOID CapturedBuffer,
    _In_ PVOID *OffsetInBuffer,
    _In_ BOOLEAN Multi,
    _In_ BOOLEAN AllowNull
    )
{
    SIZE_T offset;
    PWCHAR start;
    PWCHAR end;
    PH_STRINGREF remainingPart;
    PH_STRINGREF firstPart;

    offset = (SIZE_T)*OffsetInBuffer;

    if (offset == 0)
    {
        if (AllowNull)
            return STATUS_SUCCESS;
        else
            return STATUS_ACCESS_VIOLATION;
    }

    if (offset >= PackedData->Length)
        return STATUS_ACCESS_VIOLATION;
    if (offset & 1)
        return STATUS_DATATYPE_MISALIGNMENT;

    start = (PWCHAR)PTR_ADD_OFFSET(CapturedBuffer, offset);
    end = (PWCHAR)PTR_ADD_OFFSET(CapturedBuffer, (PackedData->Length & -2));
    remainingPart.Buffer = start;
    remainingPart.Length = (end - start) * sizeof(WCHAR);

    if (Multi)
    {
        SIZE_T validatedLength = 0;

        while (PhSplitStringRefAtChar(&remainingPart, UNICODE_NULL, &firstPart, &remainingPart))
        {
            validatedLength += firstPart.Length + sizeof(WCHAR);

            if (firstPart.Length == 0)
            {
                *OffsetInBuffer = start;

                return STATUS_SUCCESS;
            }
        }
    }
    else
    {
        if (PhSplitStringRefAtChar(&remainingPart, UNICODE_NULL, &firstPart, &remainingPart))
        {
            *OffsetInBuffer = start;

            return STATUS_SUCCESS;
        }
    }

    return STATUS_ACCESS_VIOLATION;
}

NTSTATUS PhSvcApiChangeServiceConfig2(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    )
{
    NTSTATUS status;
    PPH_STRING serviceName = NULL;
    PVOID info = NULL;
    SC_HANDLE serviceHandle = NULL;
    PH_RELATIVE_STRINGREF packedData;
    PVOID unpackedInfo = NULL;
    ACCESS_MASK desiredAccess = SERVICE_CHANGE_CONFIG;

    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.ChangeServiceConfig2.i.ServiceName, FALSE, &serviceName)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureBuffer(&Payload->u.ChangeServiceConfig2.i.Info, FALSE, &info)))
        goto CleanupExit;

    packedData = Payload->u.ChangeServiceConfig2.i.Info;

    switch (Payload->u.ChangeServiceConfig2.i.InfoLevel)
    {
    case SERVICE_CONFIG_FAILURE_ACTIONS:
        {
            LPSERVICE_FAILURE_ACTIONS failureActions;

            if (!NT_SUCCESS(status = PhSvcpUnpackRoot(&packedData, info, sizeof(SERVICE_FAILURE_ACTIONS), &failureActions)))
                goto CleanupExit;
            if (!NT_SUCCESS(status = PhSvcpUnpackStringZ(&packedData, info, &failureActions->lpRebootMsg, FALSE, TRUE)))
                goto CleanupExit;
            if (!NT_SUCCESS(status = PhSvcpUnpackStringZ(&packedData, info, &failureActions->lpCommand, FALSE, TRUE)))
                goto CleanupExit;
            if (!NT_SUCCESS(status = PhSvcpUnpackBuffer(&packedData, info, &failureActions->lpsaActions, failureActions->cActions * sizeof(SC_ACTION), __alignof(SC_ACTION), TRUE)))
                goto CleanupExit;

            if (failureActions->lpsaActions)
            {
                ULONG i;

                for (i = 0; i < failureActions->cActions; i++)
                {
                    if (failureActions->lpsaActions[i].Type == SC_ACTION_RESTART)
                    {
                        desiredAccess |= SERVICE_START;
                        break;
                    }
                }
            }

            unpackedInfo = failureActions;
        }
        break;
    case SERVICE_CONFIG_DELAYED_AUTO_START_INFO:
        status = PhSvcpUnpackRoot(&packedData, info, sizeof(SERVICE_DELAYED_AUTO_START_INFO), &unpackedInfo);
        break;
    case SERVICE_CONFIG_FAILURE_ACTIONS_FLAG:
        status = PhSvcpUnpackRoot(&packedData, info, sizeof(SERVICE_FAILURE_ACTIONS_FLAG), &unpackedInfo);
        break;
    case SERVICE_CONFIG_SERVICE_SID_INFO:
        status = PhSvcpUnpackRoot(&packedData, info, sizeof(SERVICE_SID_INFO), &unpackedInfo);
        break;
    case SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO:
        {
            LPSERVICE_REQUIRED_PRIVILEGES_INFO requiredPrivilegesInfo;

            if (!NT_SUCCESS(status = PhSvcpUnpackRoot(&packedData, info, sizeof(SERVICE_REQUIRED_PRIVILEGES_INFO), &requiredPrivilegesInfo)))
                goto CleanupExit;
            if (!NT_SUCCESS(status = PhSvcpUnpackStringZ(&packedData, info, &requiredPrivilegesInfo->pmszRequiredPrivileges, TRUE, FALSE)))
                goto CleanupExit;

            unpackedInfo = requiredPrivilegesInfo;
        }
        break;
    case SERVICE_CONFIG_PRESHUTDOWN_INFO:
        status = PhSvcpUnpackRoot(&packedData, info, sizeof(SERVICE_PRESHUTDOWN_INFO), &unpackedInfo);
        break;
    case SERVICE_CONFIG_TRIGGER_INFO:
        {
            PSERVICE_TRIGGER_INFO triggerInfo;
            ULONG i;
            PSERVICE_TRIGGER trigger;
            ULONG j;
            PSERVICE_TRIGGER_SPECIFIC_DATA_ITEM dataItem;
            ULONG alignment;

            if (!NT_SUCCESS(status = PhSvcpUnpackRoot(&packedData, info, sizeof(SERVICE_TRIGGER_INFO), &triggerInfo)))
                goto CleanupExit;
            if (!NT_SUCCESS(status = PhSvcpUnpackBuffer(&packedData, info, &triggerInfo->pTriggers, triggerInfo->cTriggers * sizeof(SERVICE_TRIGGER), __alignof(SERVICE_TRIGGER), TRUE)))
                goto CleanupExit;

            if (triggerInfo->pTriggers)
            {
                for (i = 0; i < triggerInfo->cTriggers; i++)
                {
                    trigger = &triggerInfo->pTriggers[i];

                    if (!NT_SUCCESS(status = PhSvcpUnpackBuffer(&packedData, info, &trigger->pTriggerSubtype, sizeof(GUID), __alignof(GUID), TRUE)))
                        goto CleanupExit;
                    if (!NT_SUCCESS(status = PhSvcpUnpackBuffer(&packedData, info, &trigger->pDataItems, trigger->cDataItems * sizeof(SERVICE_TRIGGER_SPECIFIC_DATA_ITEM), __alignof(SERVICE_TRIGGER_SPECIFIC_DATA_ITEM), TRUE)))
                        goto CleanupExit;

                    if (trigger->pDataItems)
                    {
                        for (j = 0; j < trigger->cDataItems; j++)
                        {
                            dataItem = &trigger->pDataItems[j];
                            alignment = 1;

                            switch (dataItem->dwDataType)
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

                            if (!NT_SUCCESS(status = PhSvcpUnpackBuffer(&packedData, info, &dataItem->pData, dataItem->cbData, alignment, FALSE)))
                                goto CleanupExit;
                        }
                    }
                }
            }

            unpackedInfo = triggerInfo;
        }
        break;
    case SERVICE_CONFIG_LAUNCH_PROTECTED:
        status = PhSvcpUnpackRoot(&packedData, info, sizeof(SERVICE_LAUNCH_PROTECTED_INFO), &unpackedInfo);
        break;
    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

    if (NT_SUCCESS(status))
    {
        assert(unpackedInfo);

        if (!(serviceHandle = PhOpenService(serviceName->Buffer, desiredAccess)))
        {
            status = PhGetLastWin32ErrorAsNtStatus();
            goto CleanupExit;
        }

        if (!ChangeServiceConfig2(
            serviceHandle,
            Payload->u.ChangeServiceConfig2.i.InfoLevel,
            unpackedInfo
            ))
        {
            status = PhGetLastWin32ErrorAsNtStatus();
        }
    }

CleanupExit:
    if (serviceHandle)
        CloseServiceHandle(serviceHandle);
    if (info)
        PhFree(info);
    if (serviceName)
        PhDereferenceObject(serviceName);

    return status;
}

NTSTATUS PhSvcApiSetTcpEntry(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    )
{
    static PVOID setTcpEntry = NULL;

    ULONG (__stdcall *localSetTcpEntry)(PVOID TcpRow);
    struct
    {
        ULONG dwState;
        ULONG dwLocalAddr;
        ULONG dwLocalPort;
        ULONG dwRemoteAddr;
        ULONG dwRemotePort;
    } tcpRow;
    ULONG result;

    localSetTcpEntry = InterlockedCompareExchangePointer(&setTcpEntry, NULL, NULL);

    if (!localSetTcpEntry)
    {
        HMODULE iphlpapiModule;

        iphlpapiModule = PhLoadLibrary(L"iphlpapi.dll");

        if (iphlpapiModule)
        {
            localSetTcpEntry = PhGetDllBaseProcedureAddress(iphlpapiModule, "SetTcpEntry", 0);

            if (localSetTcpEntry)
            {
                if (_InterlockedExchangePointer(&setTcpEntry, localSetTcpEntry) != NULL)
                {
                    // Another thread got the address of SetTcpEntry already.
                    // Decrement the reference count of iphlpapi.dll.
                    FreeLibrary(iphlpapiModule);
                }
            }
        }
    }

    if (!localSetTcpEntry)
        return STATUS_NOT_SUPPORTED;

    tcpRow.dwState = Payload->u.SetTcpEntry.i.State;
    tcpRow.dwLocalAddr = Payload->u.SetTcpEntry.i.LocalAddress;
    tcpRow.dwLocalPort = Payload->u.SetTcpEntry.i.LocalPort;
    tcpRow.dwRemoteAddr = Payload->u.SetTcpEntry.i.RemoteAddress;
    tcpRow.dwRemotePort = Payload->u.SetTcpEntry.i.RemotePort;
    result = localSetTcpEntry(&tcpRow);

    return NTSTATUS_FROM_WIN32(result);
}

NTSTATUS PhSvcApiControlThread(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    )
{
    NTSTATUS status;
    HANDLE threadId;
    HANDLE threadHandle;

    threadId = Payload->u.ControlThread.i.ThreadId;

    switch (Payload->u.ControlThread.i.Command)
    {
    case PhSvcControlThreadTerminate:
        if (NT_SUCCESS(status = PhOpenThread(&threadHandle, THREAD_TERMINATE, threadId)))
        {
            status = NtTerminateThread(threadHandle, STATUS_SUCCESS);
            NtClose(threadHandle);
        }
        break;
    case PhSvcControlThreadSuspend:
        if (NT_SUCCESS(status = PhOpenThread(&threadHandle, THREAD_SUSPEND_RESUME, threadId)))
        {
            status = NtSuspendThread(threadHandle, NULL);
            NtClose(threadHandle);
        }
        break;
    case PhSvcControlThreadResume:
        if (NT_SUCCESS(status = PhOpenThread(&threadHandle, THREAD_SUSPEND_RESUME, threadId)))
        {
            status = NtResumeThread(threadHandle, NULL);
            NtClose(threadHandle);
        }
        break;
    case PhSvcControlThreadIoPriority:
        if (NT_SUCCESS(status = PhOpenThread(&threadHandle, THREAD_SET_INFORMATION, threadId)))
        {
            status = PhSetThreadIoPriority(threadHandle, Payload->u.ControlThread.i.Argument);
            NtClose(threadHandle);
        }
        break;
    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

    return status;
}

NTSTATUS PhSvcApiAddAccountRight(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    )
{
    NTSTATUS status;
    PSID accountSid;
    PPH_STRING userRight;
    LSA_HANDLE policyHandle;
    UNICODE_STRING userRightUs;

    if (NT_SUCCESS(status = PhSvcCaptureSid(&Payload->u.AddAccountRight.i.AccountSid, FALSE, &accountSid)))
    {
        if (NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.AddAccountRight.i.UserRight, FALSE, &userRight)))
        {
            PH_AUTO(userRight);

            if (NT_SUCCESS(status = PhOpenLsaPolicy(&policyHandle, POLICY_LOOKUP_NAMES | POLICY_CREATE_ACCOUNT, NULL)))
            {
                PhStringRefToUnicodeString(&userRight->sr, &userRightUs);
                status = LsaAddAccountRights(policyHandle, accountSid, &userRightUs, 1);
                LsaClose(policyHandle);
            }
        }

        PhFree(accountSid);
    }

    return status;
}

NTSTATUS PhSvcApiInvokeRunAsService(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    )
{
    NTSTATUS status;
    PH_RUNAS_SERVICE_PARAMETERS parameters;
    PHSVCP_CAPTURED_RUNAS_SERVICE_PARAMETERS capturedParameters;

    if (NT_SUCCESS(status = PhSvcpCaptureRunAsServiceParameters(Client, Payload, &parameters, &capturedParameters)))
    {
        if (NT_SUCCESS(status = PhSvcpValidateRunAsServiceParameters(&parameters)))
        {
            status = PhInvokeRunAsService(&parameters);
        }
    }

    PhSvcpReleaseRunAsServiceParameters(&capturedParameters);

    return status;
}

NTSTATUS PhSvcApiIssueMemoryListCommand(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    )
{
    NTSTATUS status;

    status = NtSetSystemInformation(
        SystemMemoryListInformation,
        &Payload->u.IssueMemoryListCommand.i.Command,
        sizeof(SYSTEM_MEMORY_LIST_COMMAND)
        );

    return status;
}

NTSTATUS PhSvcApiPostMessage(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    )
{
    if (PostMessage(
        Payload->u.PostMessage.i.hWnd,
        Payload->u.PostMessage.i.Msg,
        Payload->u.PostMessage.i.wParam,
        Payload->u.PostMessage.i.lParam
        ))
    {
        return STATUS_SUCCESS;
    }
    else
    {
        return PhGetLastWin32ErrorAsNtStatus();
    }
}

NTSTATUS PhSvcApiSendMessage(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    )
{
    if (SendMessage(
        Payload->u.PostMessage.i.hWnd,
        Payload->u.PostMessage.i.Msg,
        Payload->u.PostMessage.i.wParam,
        Payload->u.PostMessage.i.lParam
        ))
    {
        return STATUS_SUCCESS;
    }
    else
    {
        return PhGetLastWin32ErrorAsNtStatus();
    }
}

NTSTATUS PhSvcApiCreateProcessIgnoreIfeoDebugger(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    )
{
    NTSTATUS status;
    PPH_STRING fileName = NULL;
    PPH_STRING commandLine = NULL;

    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.CreateProcessIgnoreIfeoDebugger.i.FileName, FALSE, &fileName)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.CreateProcessIgnoreIfeoDebugger.i.CommandLine, TRUE, &commandLine)))
        goto CleanupExit;

    if (!PhCreateProcessIgnoreIfeoDebugger(PhGetString(fileName), PhGetString(commandLine)))
        status = STATUS_UNSUCCESSFUL;

CleanupExit:
    if (fileName)
        PhDereferenceObject(fileName);
    if (commandLine)
        PhDereferenceObject(commandLine);

    return status;
}

NTSTATUS PhSvcApiSetServiceSecurity(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    )
{
    NTSTATUS status;
    PPH_STRING serviceName;
    PSECURITY_DESCRIPTOR securityDescriptor;
    ACCESS_MASK desiredAccess;
    SC_HANDLE serviceHandle;

    if (NT_SUCCESS(status = PhSvcCaptureString(&Payload->u.SetServiceSecurity.i.ServiceName, FALSE, &serviceName)))
    {
        PH_AUTO(serviceName);

        if (NT_SUCCESS(status = PhSvcCaptureSecurityDescriptor(&Payload->u.SetServiceSecurity.i.SecurityDescriptor, FALSE, 0, &securityDescriptor)))
        {
            desiredAccess = 0;

            if ((Payload->u.SetServiceSecurity.i.SecurityInformation & OWNER_SECURITY_INFORMATION) ||
                (Payload->u.SetServiceSecurity.i.SecurityInformation & GROUP_SECURITY_INFORMATION))
            {
                desiredAccess |= WRITE_OWNER;
            }

            if (Payload->u.SetServiceSecurity.i.SecurityInformation & DACL_SECURITY_INFORMATION)
            {
                desiredAccess |= WRITE_DAC;
            }

            if (Payload->u.SetServiceSecurity.i.SecurityInformation & SACL_SECURITY_INFORMATION)
            {
                desiredAccess |= ACCESS_SYSTEM_SECURITY;
            }

            if (serviceHandle = PhOpenService(serviceName->Buffer, desiredAccess))
            {
                status = PhSetSeObjectSecurity(
                    serviceHandle,
                    SE_SERVICE,
                    Payload->u.SetServiceSecurity.i.SecurityInformation,
                    securityDescriptor
                    );
                CloseServiceHandle(serviceHandle);
            }
            else
            {
                status = PhGetLastWin32ErrorAsNtStatus();
            }

            PhFree(securityDescriptor);
        }
    }

    return status;
}

static BOOL CALLBACK PhpProcessMiniDumpCallback(
    _In_ PVOID CallbackParam,
    _In_ const PMINIDUMP_CALLBACK_INPUT CallbackInput,
    _Inout_ PMINIDUMP_CALLBACK_OUTPUT CallbackOutput
    )
{
    switch (CallbackInput->CallbackType)
    {
    case IsProcessSnapshotCallback:
        if (CallbackParam)
            CallbackOutput->Status = S_FALSE;
        break;
    case ReadMemoryFailureCallback:
        CallbackOutput->Status = S_OK;
        break;
    }

    return TRUE;
}

NTSTATUS PhSvcApiWriteMiniDumpProcess(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    )
{
    MINIDUMP_CALLBACK_INFORMATION callbackInfo = { 0 };
    HANDLE processHandle = UlongToHandle(Payload->u.WriteMiniDumpProcess.i.LocalProcessHandle);
    ULONG processDumpType = Payload->u.WriteMiniDumpProcess.i.DumpType;
    HANDLE snapshotHandle = NULL;

    if (NT_SUCCESS(PhCreateProcessSnapshot(&snapshotHandle, processHandle, NULL)))
    {
        processDumpType =
            MiniDumpWithFullMemory |
            MiniDumpWithHandleData |
            MiniDumpWithUnloadedModules |
            MiniDumpWithFullMemoryInfo |
            MiniDumpWithThreadInfo |
            MiniDumpIgnoreInaccessibleMemory |
            MiniDumpWithIptTrace;
    }

    callbackInfo.CallbackRoutine = PhpProcessMiniDumpCallback;
    callbackInfo.CallbackParam = snapshotHandle;

    if (PhWriteMiniDumpProcess(
        snapshotHandle ? snapshotHandle : processHandle,
        UlongToHandle(Payload->u.WriteMiniDumpProcess.i.ProcessId),
        UlongToHandle(Payload->u.WriteMiniDumpProcess.i.LocalFileHandle),
        processDumpType,
        NULL,
        NULL,
        &callbackInfo
        ))
    {
        if (snapshotHandle)
        {
            PhFreeProcessSnapshot(snapshotHandle, processHandle);
        }

        return STATUS_SUCCESS;
    }
    else
    {
        ULONG error = GetLastError();

        if (snapshotHandle)
        {
            PhFreeProcessSnapshot(snapshotHandle, processHandle);
        }

        if (error == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER))
            return STATUS_INVALID_PARAMETER;
        else
            return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS PhSvcApiQueryProcessHeapInformation(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    )
{
    NTSTATUS status;
    PVOID dataBuffer;
    PPH_PROCESS_DEBUG_HEAP_INFORMATION heapInfo;
    PPH_STRING heapInfoHexBuffer;

    if (!NT_SUCCESS(status = PhSvcProbeBuffer(&Payload->u.QueryProcessHeap.i.Data, sizeof(WCHAR), FALSE, &dataBuffer)))
        return status;

    if (!NT_SUCCESS(status = PhQueryProcessHeapInformation(UlongToHandle(Payload->u.QueryProcessHeap.i.ProcessId), &heapInfo)))
        return status;

    heapInfoHexBuffer = PhBufferToHexString(
        (PUCHAR)heapInfo,
        sizeof(PH_PROCESS_DEBUG_HEAP_INFORMATION) + heapInfo->NumberOfHeaps * sizeof(PH_PROCESS_DEBUG_HEAP_ENTRY)
        );

    if (Payload->u.QueryProcessHeap.i.Data.Length < heapInfoHexBuffer->Length)
    {
        status = STATUS_BUFFER_OVERFLOW;
        goto CleanupExit;
    }

    memcpy(dataBuffer, heapInfoHexBuffer->Buffer, min(heapInfoHexBuffer->Length, Payload->u.QueryProcessHeap.i.Data.Length));
    Payload->u.QueryProcessHeap.o.DataLength = (ULONG)heapInfoHexBuffer->Length;

CleanupExit:
    if (heapInfoHexBuffer)
        PhDereferenceObject(heapInfoHexBuffer);
    if (heapInfo)
        PhFree(heapInfo);

    return status;
}
