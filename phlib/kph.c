/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2018-2022
 *
 */

#include <ph.h>
#include <svcsup.h>
#include <kphuser.h>
#include <kphdyn.h>

static PH_INITONCE KphMessageInitOnce = PH_INITONCE_INIT;
static PH_FREE_LIST KphMessageFreeList;

NTSTATUS KphConnect(
    _In_ PPH_STRINGREF ServiceName,
    _In_ PPH_STRINGREF ObjectName,
    _In_ PPH_STRINGREF PortName,
    _In_ PPH_STRINGREF FileName,
    _In_ PPH_STRINGREF Altitude,
    _In_ BOOLEAN DisableImageLoadProtection,
    _In_opt_ PKPH_COMMS_CALLBACK Callback
    )
{
    NTSTATUS status;
    SC_HANDLE scmHandle;
    SC_HANDLE serviceHandle = NULL;
    BOOLEAN started = FALSE;
    BOOLEAN created = FALSE;

    if (PhBeginInitOnce(&KphMessageInitOnce))
    {
        PhInitializeFreeList(&KphMessageFreeList, sizeof(KPH_MESSAGE), 16);
        PhEndInitOnce(&KphMessageInitOnce);
    }

    status = KphCommsStart(PortName, Callback);
    if (NT_SUCCESS(status) || (status == STATUS_ALREADY_INITIALIZED))
        return status;

    // Load the driver, and try again.

    // Try to start the service, if it exists.

    if (scmHandle = PhGetServiceManagerHandle())
    {
        if (serviceHandle = OpenService(scmHandle, PhGetStringRefZ(ServiceName), SERVICE_START))
        {
            if (StartService(serviceHandle, 0, NULL))
                started = TRUE;

            CloseServiceHandle(serviceHandle);
        }
    }

    if (!started && PhDoesFileExistWin32(PhGetStringRefZ(FileName)))
    {
        // Try to create the service.

        scmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);

        if (scmHandle)
        {
            serviceHandle = CreateService(
                scmHandle,
                PhGetStringRefZ(ServiceName),
                PhGetStringRefZ(ServiceName),
                SERVICE_ALL_ACCESS,
                SERVICE_KERNEL_DRIVER,
                SERVICE_DEMAND_START,
                SERVICE_ERROR_IGNORE,
                PhGetStringRefZ(FileName),
                NULL,
                NULL,
                NULL,
                WindowsVersion > WINDOWS_7 ? PhGetStringRefZ(ObjectName) : NULL,
                L""
                );

            if (serviceHandle)
            {
                created = TRUE;

                KphSetServiceSecurity(serviceHandle);

                status = KphSetParameters(
                    ServiceName,
                    PortName,
                    Altitude,
                    DisableImageLoadProtection
                    );

                if (!NT_SUCCESS(status))
                    goto CreateAndConnectEnd;

                if (StartService(serviceHandle, 0, NULL))
                    started = TRUE;
                else
                    status = PhGetLastWin32ErrorAsNtStatus();
            }
            else
            {
                status = PhGetLastWin32ErrorAsNtStatus();
            }

            CloseServiceHandle(scmHandle);
        }
    }

    if (started)
    {
        status = KphCommsStart(PortName, Callback);
    }

CreateAndConnectEnd:

    if (created && serviceHandle)
    {
        //
        // "Delete" the service (mark it for deletion), SCM will retain the
        // service entry as long as the "ObjectName" exists. We do not use a
        // device object, SCM will detect that the driver has gone away by the
        // driver object (the specified "ObjectName").
        //
        DeleteService(serviceHandle);
        CloseServiceHandle(serviceHandle);
    }

    return status;
}

NTSTATUS KphSetParameters(
    _In_ PPH_STRINGREF ServiceName,
    _In_ PPH_STRINGREF PortName,
    _In_ PPH_STRINGREF Altitude,
    _In_ BOOLEAN DisableImageLoadProtection
    )
{
    NTSTATUS status;
    HANDLE parametersKeyHandle = NULL;
    ULONG disposition;
    PH_STRINGREF valueNameSr;
    SIZE_T returnLength;
    PH_STRINGREF parametersKeyNameSr;
    PH_FORMAT format[3];
    WCHAR parametersKeyName[MAX_PATH];

    PhInitFormatS(&format[0], L"System\\CurrentControlSet\\Services\\");
    PhInitFormatSR(&format[1], *ServiceName);
    PhInitFormatS(&format[2], L"\\Parameters");

    if (!PhFormatToBuffer(
        format,
        RTL_NUMBER_OF(format),
        parametersKeyName,
        sizeof(parametersKeyName),
        &returnLength
        ))
    {
        return STATUS_UNSUCCESSFUL;
    }

    parametersKeyNameSr.Buffer = parametersKeyName;
    parametersKeyNameSr.Length = returnLength - sizeof(UNICODE_NULL);

    status = PhCreateKey(
        &parametersKeyHandle,
        KEY_WRITE | DELETE,
        PH_KEY_LOCAL_MACHINE,
        &parametersKeyNameSr,
        0,
        0,
        &disposition
        );

    if (!NT_SUCCESS(status))
        return status;

    PhInitializeStringRef(&valueNameSr, L"KphPortName");
    status = PhSetValueKey(
        parametersKeyHandle,
        &valueNameSr,
        REG_SZ,
        PortName->Buffer,
        (ULONG)PortName->Length + sizeof(UNICODE_NULL)
        );
    if (!NT_SUCCESS(status))
        goto CleanupExit;

    PhInitializeStringRef(&valueNameSr, L"KphAltitude");
    status = PhSetValueKey(
        parametersKeyHandle,
        &valueNameSr,
        REG_SZ,
        Altitude->Buffer,
        (ULONG)Altitude->Length + sizeof(UNICODE_NULL)
        );
    if (!NT_SUCCESS(status))
        goto CleanupExit;

    PhInitializeStringRef(&valueNameSr, L"DynData");
    status = PhSetValueKey(
        parametersKeyHandle,
        &valueNameSr,
        REG_BINARY,
        KphDynData,
        KphDynDataLength
        );
    if (!NT_SUCCESS(status))
        goto CleanupExit;

    PhInitializeStringRef(&valueNameSr, L"DynDataSig");
    status = PhSetValueKey(
        parametersKeyHandle,
        &valueNameSr,
        REG_BINARY,
        KphDynDataSig,
        KphDynDataSigLength
        );
    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (DisableImageLoadProtection)
    {
        ULONG dynULongValue = 1;

        PhInitializeStringRef(&valueNameSr, L"DisableImageLoadProtection");
        status = PhSetValueKey(
            parametersKeyHandle,
            &valueNameSr,
            REG_DWORD,
            &dynULongValue,
            sizeof(dynULongValue)
            );
        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    // Put more parameters here...

    status = STATUS_SUCCESS;

CleanupExit:
    if (!NT_SUCCESS(status))
    {
        // Delete the key if we created it.
        if (disposition == REG_CREATED_NEW_KEY)
            NtDeleteKey(parametersKeyHandle);
    }

    NtClose(parametersKeyHandle);

    return status;
}

BOOLEAN KphParametersExists(
    _In_z_ PWSTR ServiceName
    )
{
    NTSTATUS status;
    HANDLE parametersKeyHandle = NULL;
    PH_STRINGREF parametersKeyNameSr;
    PH_FORMAT format[3];
    SIZE_T returnLength;
    WCHAR parametersKeyName[MAX_PATH];

    PhInitFormatS(&format[0], L"System\\CurrentControlSet\\Services\\");
    PhInitFormatS(&format[1], ServiceName);
    PhInitFormatS(&format[2], L"\\Parameters");

    if (!PhFormatToBuffer(
        format,
        RTL_NUMBER_OF(format),
        parametersKeyName,
        sizeof(parametersKeyName),
        &returnLength
        ))
    {
        return FALSE;
    }

    parametersKeyNameSr.Buffer = parametersKeyName;
    parametersKeyNameSr.Length = returnLength - sizeof(UNICODE_NULL);

    status = PhOpenKey(
        &parametersKeyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &parametersKeyNameSr,
        0
        );

    if (NT_SUCCESS(status) || status == STATUS_ACCESS_DENIED)
    {
        if (parametersKeyHandle)
            NtClose(parametersKeyHandle);
        return TRUE;
    }

    if (parametersKeyHandle)
        NtClose(parametersKeyHandle);
    return FALSE;
}

NTSTATUS KphResetParameters(
    _In_z_ PWSTR ServiceName
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    HANDLE parametersKeyHandle = NULL;
    PH_STRINGREF serviceNameSr;
    PH_STRINGREF parametersKeyName;
    PH_FORMAT format[3];
    SIZE_T returnLength;
    WCHAR parametersKeyNameBuffer[MAX_PATH];

    PhInitializeStringRefLongHint(&serviceNameSr, ServiceName);
    PhInitFormatS(&format[0], L"System\\CurrentControlSet\\Services\\");
    PhInitFormatSR(&format[1], serviceNameSr);
    PhInitFormatS(&format[2], L"\\Parameters");

    if (!PhFormatToBuffer(
        format,
        RTL_NUMBER_OF(format),
        parametersKeyNameBuffer,
        sizeof(parametersKeyNameBuffer),
        &returnLength
        ))
    {
        return STATUS_UNSUCCESSFUL;
    }

    parametersKeyName.Buffer = parametersKeyNameBuffer;
    parametersKeyName.Length = returnLength - sizeof(UNICODE_NULL);

    status = KphUninstall(&serviceNameSr);
    status = WIN32_FROM_NTSTATUS(status);

    if (status == ERROR_SERVICE_DOES_NOT_EXIST)
        status = STATUS_SUCCESS;

    if (NT_SUCCESS(status))
    {
        status = PhOpenKey(
            &parametersKeyHandle,
            DELETE,
            PH_KEY_LOCAL_MACHINE,
            &parametersKeyName,
            0
            );
    }

    if (NT_SUCCESS(status) && parametersKeyHandle)
        status = NtDeleteKey(parametersKeyHandle);

    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        status = STATUS_SUCCESS;

    if (parametersKeyHandle)
        NtClose(parametersKeyHandle);

    return status;
}

VOID KphSetServiceSecurity(
    _In_ SC_HANDLE ServiceHandle
    )
{
    static SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    PSECURITY_DESCRIPTOR securityDescriptor;
    ULONG sdAllocationLength;
    UCHAR administratorsSidBuffer[FIELD_OFFSET(SID, SubAuthority) + sizeof(ULONG) * 2];
    PSID administratorsSid;
    PACL dacl;

    administratorsSid = (PSID)administratorsSidBuffer;
    RtlInitializeSid(administratorsSid, &ntAuthority, 2);
    *RtlSubAuthoritySid(administratorsSid, 0) = SECURITY_BUILTIN_DOMAIN_RID;
    *RtlSubAuthoritySid(administratorsSid, 1) = DOMAIN_ALIAS_RID_ADMINS;

    sdAllocationLength = SECURITY_DESCRIPTOR_MIN_LENGTH +
        (ULONG)sizeof(ACL) +
        (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
        RtlLengthSid(&PhSeServiceSid) +
        (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
        RtlLengthSid(administratorsSid) +
        (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
        RtlLengthSid(&PhSeInteractiveSid);

    securityDescriptor = PhAllocate(sdAllocationLength);
    dacl = (PACL)PTR_ADD_OFFSET(securityDescriptor, SECURITY_DESCRIPTOR_MIN_LENGTH);

    RtlCreateSecurityDescriptor(securityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    RtlCreateAcl(dacl, sdAllocationLength - SECURITY_DESCRIPTOR_MIN_LENGTH, ACL_REVISION);
    RtlAddAccessAllowedAce(dacl, ACL_REVISION, SERVICE_ALL_ACCESS, &PhSeServiceSid);
    RtlAddAccessAllowedAce(dacl, ACL_REVISION, SERVICE_ALL_ACCESS, administratorsSid);
    RtlAddAccessAllowedAce(dacl, ACL_REVISION,
        SERVICE_QUERY_CONFIG |
        SERVICE_QUERY_STATUS |
        SERVICE_START |
        SERVICE_STOP |
        SERVICE_INTERROGATE |
        DELETE,
        &PhSeInteractiveSid
        );
    RtlSetDaclSecurityDescriptor(securityDescriptor, TRUE, dacl, FALSE);

    SetServiceObjectSecurity(ServiceHandle, DACL_SECURITY_INFORMATION, securityDescriptor);

    PhFree(securityDescriptor);
}

NTSTATUS KphInstall(
    _In_ PPH_STRINGREF ServiceName,
    _In_ PPH_STRINGREF ObjectName,
    _In_ PPH_STRINGREF PortName,
    _In_ PPH_STRINGREF FileName,
    _In_ PPH_STRINGREF Altitude,
    _In_ BOOLEAN DisableImageLoadProtection
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SC_HANDLE scmHandle;
    SC_HANDLE serviceHandle;

    if (!(scmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE)))
    {
        return PhGetLastWin32ErrorAsNtStatus();
    }

    serviceHandle = CreateService(
        scmHandle,
        PhGetStringRefZ(ServiceName),
        PhGetStringRefZ(ServiceName),
        SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER,
        SERVICE_SYSTEM_START,
        SERVICE_ERROR_IGNORE,
        PhGetStringRefZ(FileName),
        NULL,
        NULL,
        NULL,
        WindowsVersion > WINDOWS_7 ? PhGetStringRefZ(ObjectName) : NULL,
        L""
        );

    if (serviceHandle)
    {
        KphSetServiceSecurity(serviceHandle);

        status = KphSetParameters(
            ServiceName,
            PortName,
            Altitude,
            DisableImageLoadProtection
            );

        if (!NT_SUCCESS(status))
        {
            DeleteService(serviceHandle);
            goto CreateEnd;
        }

        if (!StartService(serviceHandle, 0, NULL))
            status = PhGetLastWin32ErrorAsNtStatus();

CreateEnd:
        CloseServiceHandle(serviceHandle);
    }
    else
    {
        status = PhGetLastWin32ErrorAsNtStatus();
    }

    CloseServiceHandle(scmHandle);

    return status;
}

NTSTATUS KphUninstall(
    _In_ PPH_STRINGREF ServiceName
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SC_HANDLE scmHandle;
    SC_HANDLE serviceHandle;

    if (!(scmHandle = PhGetServiceManagerHandle()))
    {
        return PhGetLastWin32ErrorAsNtStatus();
    }

    if (serviceHandle = OpenService(scmHandle, PhGetStringRefZ(ServiceName), SERVICE_STOP | DELETE))
    {
        SERVICE_STATUS serviceStatus;

        ControlService(serviceHandle, SERVICE_CONTROL_STOP, &serviceStatus);

        if (!DeleteService(serviceHandle))
            status = PhGetLastWin32ErrorAsNtStatus();

        CloseServiceHandle(serviceHandle);
    }
    else
    {
        status = PhGetLastWin32ErrorAsNtStatus();
    }

    return status;
}

PPH_FREE_LIST KphGetMessageFreeList(
    VOID
    )
{
    if (PhBeginInitOnce(&KphMessageInitOnce))
    {
        PhInitializeFreeList(&KphMessageFreeList, sizeof(KPH_MESSAGE), 16);
        PhEndInitOnce(&KphMessageInitOnce);
    }

    return &KphMessageFreeList;
}

NTSTATUS KphGetInformerSettings(
    _Out_ PKPH_INFORMER_SETTINGS Settings
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    RtlZeroMemory(Settings, sizeof(KPH_INFORMER_SETTINGS));

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgGetInformerSettings);

    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.GetInformerSettings.Status;
        RtlCopyMemory(Settings, &msg->User.GetInformerSettings.Settings, sizeof(KPH_INFORMER_SETTINGS));
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphSetInformerSettings(
    _In_ PKPH_INFORMER_SETTINGS Settings
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgSetInformerSettings);

    RtlCopyMemory(&msg->User.SetInformerSettings.Settings, Settings, sizeof(KPH_INFORMER_SETTINGS));
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.SetInformerSettings.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphOpenProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCLIENT_ID ClientId
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgOpenProcess);

    msg->User.OpenProcess.ProcessHandle = ProcessHandle;
    msg->User.OpenProcess.DesiredAccess = DesiredAccess;
    msg->User.OpenProcess.ClientId = ClientId;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.OpenProcess.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphOpenProcessToken(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE TokenHandle
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgOpenProcessToken);

    msg->User.OpenProcessToken.ProcessHandle = ProcessHandle;
    msg->User.OpenProcessToken.DesiredAccess = DesiredAccess;
    msg->User.OpenProcessToken.TokenHandle = TokenHandle;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.OpenProcessToken.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphOpenProcessJob(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE JobHandle
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgOpenProcessJob);

    msg->User.OpenProcessJob.ProcessHandle = ProcessHandle;
    msg->User.OpenProcessJob.DesiredAccess = DesiredAccess;
    msg->User.OpenProcessJob.JobHandle = JobHandle;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.OpenProcessJob.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphTerminateProcess(
    _In_ HANDLE ProcessHandle,
    _In_ NTSTATUS ExitStatus
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgTerminateProcess);

    msg->User.TerminateProcess.ProcessHandle = ProcessHandle;
    msg->User.TerminateProcess.ExitStatus = ExitStatus;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.TerminateProcess.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphReadVirtualMemoryUnsafe(
    _In_opt_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Inout_opt_ PSIZE_T NumberOfBytesRead
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgReadVirtualMemoryUnsafe);

    msg->User.ReadVirtualMemoryUnsafe.ProcessHandle = ProcessHandle;
    msg->User.ReadVirtualMemoryUnsafe.BaseAddress = BaseAddress;
    msg->User.ReadVirtualMemoryUnsafe.Buffer = Buffer;
    msg->User.ReadVirtualMemoryUnsafe.BufferSize = BufferSize;
    msg->User.ReadVirtualMemoryUnsafe.NumberOfBytesRead = NumberOfBytesRead;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.ReadVirtualMemoryUnsafe.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphOpenThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCLIENT_ID ClientId
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgOpenThread);

    msg->User.OpenThread.ThreadHandle = ThreadHandle;
    msg->User.OpenThread.DesiredAccess = DesiredAccess;
    msg->User.OpenThread.ClientId = ClientId;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.OpenThread.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphOpenThreadProcess(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE ProcessHandle
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgOpenThreadProcess);

    msg->User.OpenThreadProcess.ThreadHandle = ThreadHandle;
    msg->User.OpenThreadProcess.DesiredAccess = DesiredAccess;
    msg->User.OpenThreadProcess.ProcessHandle = ProcessHandle;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.OpenThreadProcess.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphCaptureStackBackTraceThread(
    _In_ HANDLE ThreadHandle,
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _Out_writes_(FramesToCapture) PVOID *BackTrace,
    _Inout_opt_ PULONG CapturedFrames,
    _Inout_opt_ PULONG BackTraceHash
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;
    LARGE_INTEGER timeout;

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgCaptureStackBackTraceThread);

    msg->User.CaptureStackBackTraceThread.ThreadHandle = ThreadHandle;
    msg->User.CaptureStackBackTraceThread.FramesToSkip = FramesToSkip;
    msg->User.CaptureStackBackTraceThread.FramesToCapture = FramesToCapture;
    msg->User.CaptureStackBackTraceThread.BackTrace = BackTrace;
    msg->User.CaptureStackBackTraceThread.CapturedFrames = CapturedFrames;
    msg->User.CaptureStackBackTraceThread.BackTraceHash = BackTraceHash;
    msg->User.CaptureStackBackTraceThread.Timeout = PhTimeoutFromMilliseconds(&timeout, 30);
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.CaptureStackBackTraceThread.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphEnumerateProcessHandles(
    _In_ HANDLE ProcessHandle,
    _Out_writes_bytes_(BufferLength) PVOID Buffer,
    _In_opt_ ULONG BufferLength,
    _Inout_opt_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgEnumerateProcessHandles);

    msg->User.EnumerateProcessHandles.ProcessHandle = ProcessHandle;
    msg->User.EnumerateProcessHandles.Buffer = Buffer;
    msg->User.EnumerateProcessHandles.BufferLength = BufferLength;
    msg->User.EnumerateProcessHandles.ReturnLength = ReturnLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.EnumerateProcessHandles.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphEnumerateProcessHandles2(
    _In_ HANDLE ProcessHandle,
    _Out_ PKPH_PROCESS_HANDLE_INFORMATION *Handles
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 2048;

    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = KphEnumerateProcessHandles(
            ProcessHandle,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (status == STATUS_BUFFER_TOO_SMALL)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);
        }
        else
        {
            break;
        }
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *Handles = buffer;

    return status;
}

NTSTATUS KphQueryInformationObject(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _Out_writes_bytes_opt_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength,
    _Out_opt_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgQueryInformationObject);

    msg->User.QueryInformationObject.ProcessHandle = ProcessHandle;
    msg->User.QueryInformationObject.Handle = Handle;
    msg->User.QueryInformationObject.ObjectInformationClass = ObjectInformationClass;
    msg->User.QueryInformationObject.ObjectInformation = ObjectInformation;
    msg->User.QueryInformationObject.ObjectInformationLength = ObjectInformationLength;
    msg->User.QueryInformationObject.ReturnLength = ReturnLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.QueryInformationObject.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphSetInformationObject(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _In_reads_bytes_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgSetInformationObject);

    msg->User.SetInformationObject.ProcessHandle = ProcessHandle;
    msg->User.SetInformationObject.Handle = Handle;
    msg->User.SetInformationObject.ObjectInformationClass = ObjectInformationClass;
    msg->User.SetInformationObject.ObjectInformation = ObjectInformation;
    msg->User.SetInformationObject.ObjectInformationLength = ObjectInformationLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.SetInformationObject.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphOpenDriver(
    _Out_ PHANDLE DriverHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgOpenDriver);

    msg->User.OpenDriver.DriverHandle = DriverHandle;
    msg->User.OpenDriver.DesiredAccess = DesiredAccess;
    msg->User.OpenDriver.ObjectAttributes = ObjectAttributes;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.OpenDriver.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphQueryInformationDriver(
    _In_ HANDLE DriverHandle,
    _In_ DRIVER_INFORMATION_CLASS DriverInformationClass,
    _Out_writes_bytes_opt_(DriverInformationLength) PVOID DriverInformation,
    _In_ ULONG DriverInformationLength,
    _Inout_opt_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgQueryInformationDriver);

    msg->User.QueryInformationDriver.DriverHandle = DriverHandle;
    msg->User.QueryInformationDriver.DriverInformationClass = DriverInformationClass;
    msg->User.QueryInformationDriver.DriverInformation = DriverInformation;
    msg->User.QueryInformationDriver.DriverInformationLength = DriverInformationLength;
    msg->User.QueryInformationDriver.ReturnLength = ReturnLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.QueryInformationDriver.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphQueryInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    _Out_writes_bytes_opt_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Inout_opt_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgQueryInformationProcess);

    msg->User.QueryInformationProcess.ProcessHandle = ProcessHandle;
    msg->User.QueryInformationProcess.ProcessInformationClass = ProcessInformationClass;
    msg->User.QueryInformationProcess.ProcessInformation = ProcessInformation;
    msg->User.QueryInformationProcess.ProcessInformationLength = ProcessInformationLength;
    msg->User.QueryInformationProcess.ReturnLength = ReturnLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.QueryInformationDriver.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

KPH_PROCESS_STATE KphGetProcessState(
    _In_ HANDLE ProcessHandle
    )
{
    KPH_PROCESS_STATE state;

    if (!KphCommsIsConnected())
        return 0;

    if (!NT_SUCCESS(KphQueryInformationProcess(
        ZwCurrentProcess(),
        KphProcessStateInformation,
        &state,
        sizeof(state),
        NULL
        )))
        return 0;

    return state;
}

KPH_PROCESS_STATE KphGetCurrentProcessState(
    VOID
    )
{
    if (!KphCommsIsConnected())
        return 0;

    return KphGetProcessState(NtCurrentProcess());
}

KPH_LEVEL KphProcessLevel(
    _In_ HANDLE ProcessHandle
    )
{
    KPH_PROCESS_STATE state;

    if (!KphCommsIsConnected())
        return KphLevelNone;

    //
    // This corresponds to the API access the client is currently given.
    // See comms_handlers.c
    //
    // Note that process state can change in runtime, so re-checking is
    // necessary.
    //

    state = KphGetProcessState(ProcessHandle);

    if ((state & KPH_PROCESS_STATE_MAXIMUM) == KPH_PROCESS_STATE_MAXIMUM)
        return KphLevelMax;

    if ((state & KPH_PROCESS_STATE_HIGH) == KPH_PROCESS_STATE_HIGH)
        return KphLevelHigh;

    if ((state & KPH_PROCESS_STATE_MEDIUM) == KPH_PROCESS_STATE_MEDIUM)
        return KphLevelMed;

    if ((state & KPH_PROCESS_STATE_LOW) == KPH_PROCESS_STATE_LOW)
        return KphLevelLow;

    if ((state & KPH_PROCESS_STATE_MINIMUM) == KPH_PROCESS_STATE_MINIMUM)
        return KphLevelMin;

    return KphLevelNone;

}

KPH_LEVEL KphLevel(
    VOID
    )
{
    return KphProcessLevel(NtCurrentProcess());
}

NTSTATUS KphSetInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    _In_reads_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgSetInformationProcess);

    msg->User.SetInformationProcess.ProcessHandle = ProcessHandle;
    msg->User.SetInformationProcess.ProcessInformationClass = ProcessInformationClass;
    msg->User.SetInformationProcess.ProcessInformation = ProcessInformation;
    msg->User.SetInformationProcess.ProcessInformationLength = ProcessInformationLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.SetInformationProcess.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphSetInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ KPH_THREAD_INFORMATION_CLASS ThreadInformationClass,
    _In_reads_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgSetInformationThread);

    msg->User.SetInformationThread.ThreadHandle = ThreadHandle;
    msg->User.SetInformationThread.ThreadInformationClass = ThreadInformationClass;
    msg->User.SetInformationThread.ThreadInformation = ThreadInformation;
    msg->User.SetInformationThread.ThreadInformationLength = ThreadInformationLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.SetInformationThread.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphSystemControl(
    _In_ KPH_SYSTEM_CONTROL_CLASS SystemControlClass,
    _In_reads_bytes_(SystemControlInfoLength) PVOID SystemControlInfo,
    _In_ ULONG SystemControlInfoLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgSystemControl);

    msg->User.SystemControl.SystemControlClass = SystemControlClass;
    msg->User.SystemControl.SystemControlInfo = SystemControlInfo;
    msg->User.SystemControl.SystemControlInfoLength = SystemControlInfoLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.SystemControl.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphAlpcQueryInformation(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE PortHandle,
    _In_ KPH_ALPC_INFORMATION_CLASS AlpcInformationClass,
    _Out_writes_bytes_opt_(AlpcInformationLength) PVOID AlpcInformation,
    _In_ ULONG AlpcInformationLength,
    _Out_opt_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgAlpcQueryInformation);

    msg->User.AlpcQueryInformation.ProcessHandle = ProcessHandle;
    msg->User.AlpcQueryInformation.PortHandle = PortHandle;
    msg->User.AlpcQueryInformation.AlpcInformationClass = AlpcInformationClass;
    msg->User.AlpcQueryInformation.AlpcInformation = AlpcInformation;
    msg->User.AlpcQueryInformation.AlpcInformationLength = AlpcInformationLength;
    msg->User.AlpcQueryInformation.ReturnLength = ReturnLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.AlpcQueryInformation.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphAlpcQueryComminicationsNamesInfo(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE PortHandle,
    _Out_ PKPH_ALPC_COMMUNICATION_NAMES_INFORMATION* Names
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = MAX_PATH;

    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = KphAlpcQueryInformation(ProcessHandle,
                                         PortHandle,
                                         KphAlpcCommunicationNamesInformation,
                                         buffer,
                                         bufferSize,
                                         &bufferSize);
        if (status == STATUS_BUFFER_TOO_SMALL)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);
        }
        else
        {
            break;
        }
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *Names = buffer;

    return status;
}

NTSTATUS KphQueryInformationFile(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE FileHandle,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _Out_writes_bytes_(FileInformationLength) PVOID FileInformation,
    _In_ ULONG FileInformationLength,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgQueryInformationFile);

    msg->User.QueryInformationFile.ProcessHandle = ProcessHandle;
    msg->User.QueryInformationFile.FileHandle = FileHandle;
    msg->User.QueryInformationFile.FileInformationClass = FileInformationClass;
    msg->User.QueryInformationFile.FileInformation = FileInformation;
    msg->User.QueryInformationFile.FileInformationLength = FileInformationLength;
    msg->User.QueryInformationFile.IoStatusBlock = IoStatusBlock;

    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.QueryInformationFile.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphQueryVolumeInformationFile(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE FileHandle,
    _In_ FS_INFORMATION_CLASS FsInformationClass,
    _Out_writes_bytes_(FsInformationLength) PVOID FsInformation,
    _In_ ULONG FsInformationLength,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgQueryVolumeInformationFile);

    msg->User.QueryVolumeInformationFile.ProcessHandle = ProcessHandle;
    msg->User.QueryVolumeInformationFile.FileHandle = FileHandle;
    msg->User.QueryVolumeInformationFile.FsInformationClass = FsInformationClass;
    msg->User.QueryVolumeInformationFile.FsInformation = FsInformation;
    msg->User.QueryVolumeInformationFile.FsInformationLength = FsInformationLength;
    msg->User.QueryVolumeInformationFile.IoStatusBlock = IoStatusBlock;

    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.QueryVolumeInformationFile.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphDuplicateObject(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE SourceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE TargetHandle
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgDuplicateObject);

    msg->User.DuplicateObject.ProcessHandle = ProcessHandle;
    msg->User.DuplicateObject.SourceHandle = SourceHandle;
    msg->User.DuplicateObject.DesiredAccess = DesiredAccess;
    msg->User.DuplicateObject.TargetHandle = TargetHandle;

    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.DuplicateObject.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}
