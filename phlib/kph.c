/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2018-2023
 *
 */

#include <ph.h>
#include <svcsup.h>
#include <kphuser.h>

static PH_STRINGREF KphDefaultPortName = PH_STRINGREF_INIT(KPH_PORT_NAME);
static PH_FREE_LIST KphMessageFreeList;

VOID KphInitialize(
    VOID
    )
{
    PhInitializeFreeList(&KphMessageFreeList, sizeof(KPH_MESSAGE), 16);
}

NTSTATUS KphConnect(
    _In_ PKPH_CONFIG_PARAMETERS Config
    )
{
    NTSTATUS status;
    SC_HANDLE serviceHandle;
    BOOLEAN created = FALSE;
    PPH_STRINGREF portName;

    portName = (Config->PortName ? Config->PortName : &KphDefaultPortName);

    status = KphCommsStart(portName, Config->Callback);

    if (NT_SUCCESS(status) || (status == STATUS_ALREADY_INITIALIZED))
        return status;

    // Load the driver, and try again.

    if (Config->EnableNativeLoad || Config->EnableFilterLoad)
    {
        status = KsiLoadUnloadService(Config, TRUE);

        if (NT_SUCCESS(status))
        {
            status = KphCommsStart(portName, Config->Callback);
        }

        return status;
    }

    // Try to start the service, if it exists.

    status = PhOpenService(&serviceHandle, SERVICE_START, PhGetStringRefZ(Config->ServiceName));

    if (NT_SUCCESS(status))
    {
        status = PhStartService(serviceHandle, 0, NULL);

        PhCloseServiceHandle(serviceHandle);

        if (!NT_SUCCESS(status))
            goto CreateAndConnectEnd;

        status = KphCommsStart(portName, Config->Callback);

        goto CreateAndConnectEnd;
    }

    if (!PhDoesFileExistWin32(PhGetStringRefZ(Config->FileName)))
    {
        status = STATUS_NO_SUCH_FILE;
        goto CreateAndConnectEnd;
    }

    // Try to create the service.
    status = PhCreateService(
        &serviceHandle,
        PhGetStringRefZ(Config->ServiceName),
        PhGetStringRefZ(Config->ServiceName),
        SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_IGNORE,
        PhGetStringRefZ(Config->FileName),
        PhGetStringRefZ(Config->ObjectName),
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CreateAndConnectEnd;

    created = TRUE;

    KphSetServiceSecurity(serviceHandle);

    status = KphSetParameters(Config);

    if (!NT_SUCCESS(status))
        goto CreateAndConnectEnd;

    status = PhStartService(serviceHandle, 0, NULL);

    if (!NT_SUCCESS(status))
        goto CreateAndConnectEnd;

    status = KphCommsStart(portName, Config->Callback);

CreateAndConnectEnd:

    if (created && serviceHandle)
    {
        //
        // "Delete" the service (mark it for deletion), SCM will retain the
        // service entry as long as the "ObjectName" exists. We do not use a
        // device object, SCM will detect that the driver has gone away by the
        // driver object (the specified "ObjectName").
        //
        PhDeleteService(serviceHandle);
        PhCloseServiceHandle(serviceHandle);
    }

    return status;
}

NTSTATUS KphpSetParametersService(
    _In_ PKPH_CONFIG_PARAMETERS Config
    )
{
#ifdef _WIN64
    NTSTATUS status;
    HANDLE servicesKeyHandle = NULL;
    ULONG disposition;
    SIZE_T returnLength;
    PH_STRINGREF servicesKeyName;
    PH_FORMAT format[2];
    WCHAR servicesKeyNameBuffer[MAX_PATH];

    PhInitFormatS(&format[0], L"System\\CurrentControlSet\\Services\\");
    PhInitFormatSR(&format[1], *Config->ServiceName);

    if (!PhFormatToBuffer(
        format,
        RTL_NUMBER_OF(format),
        servicesKeyNameBuffer,
        sizeof(servicesKeyNameBuffer),
        &returnLength
        ))
    {
        return STATUS_UNSUCCESSFUL;
    }

    servicesKeyName.Buffer = servicesKeyNameBuffer;
    servicesKeyName.Length = returnLength - sizeof(UNICODE_NULL);

    status = PhCreateKey(
        &servicesKeyHandle,
        KEY_WRITE | DELETE,
        PH_KEY_LOCAL_MACHINE,
        &servicesKeyName,
        0,
        0,
        &disposition
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhSetValueKeyZ(
        servicesKeyHandle,
        L"SupportedFeatures",
        REG_DWORD,
        &Config->FsSupportedFeatures,
        sizeof(ULONG)
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

CleanupExit:

    if (servicesKeyHandle)
        NtClose(servicesKeyHandle);

    return status;
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

NTSTATUS KphSetParameters(
    _In_ PKPH_CONFIG_PARAMETERS Config
    )
{
#ifdef _WIN64
    NTSTATUS status;
    HANDLE parametersKeyHandle = NULL;
    ULONG disposition;
    SIZE_T returnLength;
    PH_STRINGREF parametersKeyName;
    PH_FORMAT format[3];
    WCHAR parametersKeyNameBuffer[MAX_PATH];

    // Services key parameters.
    status = KphpSetParametersService(Config);
    if (!NT_SUCCESS(status))
        return status;

    if (!Config->PortName && !Config->Altitude && !Config->Flags.Flags)
    {
        // Don't create parameters key unless we must.
        return STATUS_SUCCESS;
    }

    PhInitFormatS(&format[0], L"System\\CurrentControlSet\\Services\\");
    PhInitFormatSR(&format[1], *Config->ServiceName);
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

    status = PhCreateKey(
        &parametersKeyHandle,
        KEY_WRITE | DELETE,
        PH_KEY_LOCAL_MACHINE,
        &parametersKeyName,
        0,
        0,
        &disposition
        );

    if (!NT_SUCCESS(status))
        return status;

    if (Config->PortName)
    {
        status = PhSetValueKeyZ(
            parametersKeyHandle,
            L"PortName",
            REG_SZ,
            Config->PortName->Buffer,
            (ULONG)Config->PortName->Length + sizeof(UNICODE_NULL)
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    if (Config->Altitude)
    {
        status = PhSetValueKeyZ(
            parametersKeyHandle,
            L"Altitude",
            REG_SZ,
            Config->Altitude->Buffer,
            (ULONG)Config->Altitude->Length + sizeof(UNICODE_NULL)
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    if (Config->Flags.Flags)
    {
        C_ASSERT(sizeof(KPH_PARAMETER_FLAGS) == sizeof(ULONG));

        status = PhSetValueKeyZ(
            parametersKeyHandle,
            L"Flags",
            REG_DWORD,
            &Config->Flags.Flags,
            sizeof(ULONG)
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
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

VOID KphSetServiceSecurity(
    _In_ SC_HANDLE ServiceHandle
    )
{
    PSID administratorsSid = PhSeAdministratorsSid();
    UCHAR securityDescriptorBuffer[SECURITY_DESCRIPTOR_MIN_LENGTH + 0x80];
    PSECURITY_DESCRIPTOR securityDescriptor;
    ULONG sdAllocationLength;
    PACL dacl;

    sdAllocationLength = SECURITY_DESCRIPTOR_MIN_LENGTH +
        (ULONG)sizeof(ACL) +
        (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
        PhLengthSid((PSID)&PhSeServiceSid) +
        (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
        PhLengthSid(administratorsSid) +
        (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
        PhLengthSid((PSID)&PhSeInteractiveSid);

    securityDescriptor = (PSECURITY_DESCRIPTOR)securityDescriptorBuffer;
    dacl = PTR_ADD_OFFSET(securityDescriptor, SECURITY_DESCRIPTOR_MIN_LENGTH);

    PhCreateSecurityDescriptor(securityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    PhCreateAcl(dacl, sdAllocationLength - SECURITY_DESCRIPTOR_MIN_LENGTH, ACL_REVISION);
    RtlAddAccessAllowedAce(dacl, ACL_REVISION, SERVICE_ALL_ACCESS, (PSID)&PhSeServiceSid);
    RtlAddAccessAllowedAce(dacl, ACL_REVISION, SERVICE_ALL_ACCESS, administratorsSid);
    RtlAddAccessAllowedAce(dacl, ACL_REVISION,
        SERVICE_QUERY_CONFIG |
        SERVICE_QUERY_STATUS |
        SERVICE_START |
        SERVICE_STOP |
        SERVICE_INTERROGATE |
        DELETE,
        (PSID)&PhSeInteractiveSid
        );
    PhSetDaclSecurityDescriptor(securityDescriptor, TRUE, dacl, FALSE);

    PhSetServiceObjectSecurity(ServiceHandle, DACL_SECURITY_INFORMATION, securityDescriptor);

#ifdef DEBUG
    assert(sdAllocationLength < sizeof(securityDescriptorBuffer));
    assert(RtlLengthSecurityDescriptor(securityDescriptor) < sizeof(securityDescriptorBuffer));
#endif
}

static BOOLEAN NTAPI KsiLoadUnloadServiceCleanupKeyCallback(
    _In_ HANDLE RootDirectory,
    _In_ PKEY_BASIC_INFORMATION Information,
    _In_ PVOID Context
    )
{
    HANDLE keyHandle;
    PH_STRINGREF keyName;

    keyName.Buffer = Information->Name;
    keyName.Length = Information->NameLength;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ | DELETE,
        RootDirectory,
        &keyName,
        0
        )))
    {
        PhEnumerateKey(keyHandle, KeyBasicInformation, KsiLoadUnloadServiceCleanupKeyCallback, NULL);
        NtDeleteKey(keyHandle);
        NtClose(keyHandle);
    }

    return TRUE;
}

NTSTATUS KsiLoadUnloadService(
    _In_ PKPH_CONFIG_PARAMETERS Config,
    _In_ BOOLEAN LoadDriver
    )
{
#ifdef _WIN64
    static PH_STRINGREF fullServicesKeyName = PH_STRINGREF_INIT(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    static PH_STRINGREF parametersKeyName = PH_STRINGREF_INIT(L"Parameters");
    NTSTATUS status;
    PPH_STRING fullServiceKeyName;
    PPH_STRING fullServiceFileName;
    UNICODE_STRING driverServiceKeyName;
    HANDLE serviceKeyHandle;
    HANDLE parametersKeyHandle = NULL;
    ULONG disposition;

    fullServiceKeyName = PhConcatStringRef2(&fullServicesKeyName, Config->ServiceName);

    if (!PhStringRefToUnicodeString(&fullServiceKeyName->sr, &driverServiceKeyName))
    {
        PhDereferenceObject(fullServiceKeyName);
        return STATUS_NAME_TOO_LONG;
    }

    if (LoadDriver)
    {
        status = PhCreateKey(
            &serviceKeyHandle,
            KEY_WRITE,
            NULL,
            &fullServiceKeyName->sr,
            0,
            0,
            &disposition
            );

        if (NT_SUCCESS(status))
        {
            if (disposition == REG_CREATED_NEW_KEY)
            {
                fullServiceFileName = PhConcatStringRef2(&PhNtDosDevicesPrefix, Config->FileName);
                PhSetValueKeyZ(serviceKeyHandle, L"ErrorControl", REG_DWORD, &(ULONG){ SERVICE_ERROR_NORMAL }, sizeof(ULONG));
                PhSetValueKeyZ(serviceKeyHandle, L"Type", REG_DWORD, &(ULONG){ SERVICE_KERNEL_DRIVER }, sizeof(ULONG));
                PhSetValueKeyZ(serviceKeyHandle, L"Start", REG_DWORD, &(ULONG){ SERVICE_DISABLED }, sizeof(ULONG));
                PhSetValueKeyZ(serviceKeyHandle, L"ImagePath", REG_SZ, fullServiceFileName->Buffer, (ULONG)fullServiceFileName->Length + sizeof(UNICODE_NULL));
                PhSetValueKeyZ(serviceKeyHandle, L"ObjectName", REG_SZ, Config->ObjectName->Buffer, (ULONG)Config->ObjectName->Length + sizeof(UNICODE_NULL));
                PhDereferenceObject(fullServiceFileName);

                KphSetParameters(Config);
            }

            NtClose(serviceKeyHandle);
        }

        if (Config->EnableFilterLoad)
            status = PhFilterLoadUnload(Config->ServiceName, TRUE);
        else
            status = NtLoadDriver(&driverServiceKeyName);
    }
    else
    {
        if (Config->EnableFilterLoad)
            status = PhFilterLoadUnload(Config->ServiceName, FALSE);
        else
            status = NtUnloadDriver(&driverServiceKeyName);
    }

    if (!NT_SUCCESS(status) || !LoadDriver)
    {
        if (NT_SUCCESS(PhOpenKey(
            &serviceKeyHandle,
            KEY_READ | DELETE,
            NULL,
            &fullServiceKeyName->sr,
            0
            )))
        {
            if (NT_SUCCESS(PhOpenKey(
                &parametersKeyHandle,
                DELETE,
                serviceKeyHandle,
                &parametersKeyName,
                0
                )))
            {
                NtDeleteKey(parametersKeyHandle);
                NtClose(parametersKeyHandle);
            }

            PhEnumerateKey(serviceKeyHandle, KeyBasicInformation, KsiLoadUnloadServiceCleanupKeyCallback, NULL);
            NtDeleteKey(serviceKeyHandle);
            NtClose(serviceKeyHandle);
        }
    }

    PhDereferenceObject(fullServiceKeyName);

    return status;
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

NTSTATUS KphServiceStop(
    _In_ PKPH_CONFIG_PARAMETERS Config
    )
{
    NTSTATUS status;

    //KphCommsStop();

    if (Config->EnableNativeLoad || Config->EnableFilterLoad)
    {
        status = KsiLoadUnloadService(Config, FALSE);
    }
    else
    {
        SC_HANDLE serviceHandle;

        status = PhOpenService(&serviceHandle, SERVICE_STOP, PhGetStringRefZ(Config->ServiceName));

        if (NT_SUCCESS(status))
        {
            status = PhStopService(serviceHandle);

            PhCloseServiceHandle(serviceHandle);
        }
    }

    return status;
}

PPH_FREE_LIST KphGetMessageFreeList(
    VOID
    )
{
    KSI_COMMS_INIT_ASSERT();

    return &KphMessageFreeList;
}

NTSTATUS KphGetInformerSettings(
    _Out_ PKPH_INFORMER_SETTINGS Settings
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

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

    KSI_COMMS_INIT_ASSERT();

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

    KSI_COMMS_INIT_ASSERT();

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

    KSI_COMMS_INIT_ASSERT();

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

    KSI_COMMS_INIT_ASSERT();

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

    KSI_COMMS_INIT_ASSERT();

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

NTSTATUS KphReadVirtualMemory(
    _In_opt_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Inout_opt_ PSIZE_T NumberOfBytesRead
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgReadVirtualMemory);
    msg->User.ReadVirtualMemory.ProcessHandle = ProcessHandle;
    msg->User.ReadVirtualMemory.BaseAddress = BaseAddress;
    msg->User.ReadVirtualMemory.Buffer = Buffer;
    msg->User.ReadVirtualMemory.BufferSize = BufferSize;
    msg->User.ReadVirtualMemory.NumberOfBytesRead = NumberOfBytesRead;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.ReadVirtualMemory.Status;
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

    KSI_COMMS_INIT_ASSERT();

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

    KSI_COMMS_INIT_ASSERT();

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
    _Out_ PULONG CapturedFrames,
    _Out_opt_ PULONG BackTraceHash,
    _In_ ULONG Flags
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;
    LARGE_INTEGER timeout;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgCaptureStackBackTraceThread);
    msg->User.CaptureStackBackTraceThread.ThreadHandle = ThreadHandle;
    msg->User.CaptureStackBackTraceThread.FramesToSkip = FramesToSkip;
    msg->User.CaptureStackBackTraceThread.FramesToCapture = FramesToCapture;
    msg->User.CaptureStackBackTraceThread.BackTrace = BackTrace;
    msg->User.CaptureStackBackTraceThread.CapturedFrames = CapturedFrames;
    msg->User.CaptureStackBackTraceThread.BackTraceHash = BackTraceHash;
    msg->User.CaptureStackBackTraceThread.Timeout = PhTimeoutFromMilliseconds(&timeout, 300);
    msg->User.CaptureStackBackTraceThread.Flags = Flags;
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

    KSI_COMMS_INIT_ASSERT();

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

NTSTATUS KsiEnumerateProcessHandles(
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

    KSI_COMMS_INIT_ASSERT();

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

NTSTATUS KphQueryObjectSectionMappingsInfo(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _Out_ PKPH_SECTION_MAPPINGS_INFORMATION* Info
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = MAX_PATH;

    *Info = NULL;

    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = KphQueryInformationObject(ProcessHandle,
                                           Handle,
                                           KphObjectSectionMappingsInformation,
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

    *Info = buffer;

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

    KSI_COMMS_INIT_ASSERT();

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
    _In_ PCOBJECT_ATTRIBUTES ObjectAttributes
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgOpenDriver);
    msg->User.OpenDriver.DriverHandle = DriverHandle;
    msg->User.OpenDriver.DesiredAccess = DesiredAccess;
    msg->User.OpenDriver.ObjectAttributes = (POBJECT_ATTRIBUTES)ObjectAttributes;
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
    _In_ KPH_DRIVER_INFORMATION_CLASS DriverInformationClass,
    _Out_writes_bytes_opt_(DriverInformationLength) PVOID DriverInformation,
    _In_ ULONG DriverInformationLength,
    _Inout_opt_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

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

    KSI_COMMS_INIT_ASSERT();

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
        ProcessHandle,
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
    return KphGetProcessState(NtCurrentProcess());
}

KPH_LEVEL KphProcessLevel(
    _In_ HANDLE ProcessHandle
    )
{
    KPH_PROCESS_STATE state;

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

KPH_LEVEL KphLevelEx(
    _In_ BOOLEAN Cached
    )
{
    static KPH_LEVEL level = KphLevelNone;

    if (!Cached)
        level = KphProcessLevel(NtCurrentProcess());

    return level;
}

KPH_LEVEL KsiLevel(
    VOID
    )
{
    return KphLevelEx(TRUE);
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

    KSI_COMMS_INIT_ASSERT();

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

    KSI_COMMS_INIT_ASSERT();

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

    KSI_COMMS_INIT_ASSERT();

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

    KSI_COMMS_INIT_ASSERT();

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

BOOLEAN KphpFileObjectIsBusy(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE FileHandle
    )
{
    KPH_FILE_OBJECT_INFORMATION fileInfo;
    if (NT_SUCCESS(KphQueryInformationObject(
        ProcessHandle,
        FileHandle,
        KphObjectFileObjectInformation,
        &fileInfo,
        sizeof(fileInfo),
        NULL
        )))
    {
        return fileInfo.Busy ? TRUE : FALSE;
    }

    return TRUE;
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

    KSI_COMMS_INIT_ASSERT();

    // TODO(jxy-s) safety added to driver (2023-06-19) remove this after next driver release
    if (KphpFileObjectIsBusy(ProcessHandle, FileHandle))
        return STATUS_POSSIBLE_DEADLOCK;

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

    KSI_COMMS_INIT_ASSERT();

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

    KSI_COMMS_INIT_ASSERT();

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

NTSTATUS KphQueryPerformanceCounter(
    _Out_ PLARGE_INTEGER PerformanceCounter,
    _Out_opt_ PLARGE_INTEGER PerformanceFrequency
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgQueryPerformanceCounter);
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        *PerformanceCounter = msg->User.QueryPerformanceCounter.PerformanceCounter;
        if (PerformanceFrequency)
        {
            *PerformanceFrequency = msg->User.QueryPerformanceCounter.PerformanceFrequency;
        }
    }
    else
    {
        PerformanceCounter->QuadPart = 0;
        if (PerformanceFrequency)
        {
            PerformanceFrequency->QuadPart = 0;
        }
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphCreateFile(
    _Out_ PHANDLE FileHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
    _In_ ULONG EaLength,
    _In_ ULONG Options
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgCreateFile);
    msg->User.CreateFile.FileHandle = FileHandle;
    msg->User.CreateFile.DesiredAccess = DesiredAccess;
    msg->User.CreateFile.ObjectAttributes = (POBJECT_ATTRIBUTES)ObjectAttributes;
    msg->User.CreateFile.IoStatusBlock = IoStatusBlock;
    msg->User.CreateFile.AllocationSize = AllocationSize;
    msg->User.CreateFile.FileAttributes = FileAttributes;
    msg->User.CreateFile.ShareAccess = ShareAccess;
    msg->User.CreateFile.CreateDisposition = CreateDisposition;
    msg->User.CreateFile.CreateOptions = CreateOptions;
    msg->User.CreateFile.EaBuffer = EaBuffer;
    msg->User.CreateFile.EaLength = EaLength;
    msg->User.CreateFile.Options = Options;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.CreateFile.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphQueryInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ KPH_THREAD_INFORMATION_CLASS ThreadInformationClass,
    _Out_writes_bytes_opt_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _Out_opt_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgQueryInformationThread);
    msg->User.QueryInformationThread.ThreadHandle = ThreadHandle;
    msg->User.QueryInformationThread.ThreadInformationClass = ThreadInformationClass;
    msg->User.QueryInformationThread.ThreadInformation = ThreadInformation;
    msg->User.QueryInformationThread.ThreadInformationLength = ThreadInformationLength;
    msg->User.QueryInformationThread.ReturnLength = ReturnLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.QueryInformationThread.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphQuerySection(
    _In_ HANDLE SectionHandle,
    _In_ KPH_SECTION_INFORMATION_CLASS SectionInformationClass,
    _Out_writes_bytes_(SectionInformationLength) PVOID SectionInformation,
    _In_ ULONG SectionInformationLength,
    _Out_opt_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgQuerySection);
    msg->User.QuerySection.SectionHandle = SectionHandle;
    msg->User.QuerySection.SectionInformationClass = SectionInformationClass;
    msg->User.QuerySection.SectionInformation = SectionInformation;
    msg->User.QuerySection.SectionInformationLength = SectionInformationLength;
    msg->User.QuerySection.ReturnLength = ReturnLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.QuerySection.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphQuerySectionMappingsInfo(
    _In_ HANDLE SectionHandle,
    _Out_ PKPH_SECTION_MAPPINGS_INFORMATION* Info
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = MAX_PATH;

    *Info = NULL;

    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = KphQuerySection(SectionHandle,
                                 KphSectionMappingsInformation,
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

    *Info = buffer;

    return status;
}

NTSTATUS KphCompareObjects(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE FirstObjectHandle,
    _In_ HANDLE SecondObjectHandle
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgCompareObjects);
    msg->User.CompareObjects.ProcessHandle = ProcessHandle;
    msg->User.CompareObjects.FirstObjectHandle = FirstObjectHandle;
    msg->User.CompareObjects.SecondObjectHandle = SecondObjectHandle;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.CompareObjects.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphGetMessageTimeouts(
    _Out_ PKPH_MESSAGE_TIMEOUTS Timeouts
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgGetMessageTimeouts);
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        RtlCopyMemory(Timeouts, &msg->User.GetMessageTimeouts.Timeouts, sizeof(*Timeouts));
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphSetMessageTimeouts(
    _In_ PKPH_MESSAGE_TIMEOUTS Timeouts
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgSetMessageTimeouts);
    RtlCopyMemory(&msg->User.SetMessageTimeouts.Timeouts, Timeouts, sizeof(*Timeouts));
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.SetMessageTimeouts.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphAcquireDriverUnloadProtection(
    _Out_opt_ PLONG PreviousCount,
    _Out_opt_ PLONG ClientPreviousCount
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    if (PreviousCount)
        *PreviousCount = 0;

    if (ClientPreviousCount)
        *ClientPreviousCount = 0;

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgAcquireDriverUnloadProtection);
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.AcquireDriverUnloadProtection.Status;

        if (NT_SUCCESS(status))
        {
            if (PreviousCount)
                *PreviousCount = msg->User.AcquireDriverUnloadProtection.PreviousCount;

            if (ClientPreviousCount)
                *ClientPreviousCount = msg->User.AcquireDriverUnloadProtection.ClientPreviousCount;
        }
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphReleaseDriverUnloadProtection(
    _Out_opt_ PLONG PreviousCount,
    _Out_opt_ PLONG ClientPreviousCount
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgReleaseDriverUnloadProtection);
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.ReleaseDriverUnloadProtection.Status;

        if (NT_SUCCESS(status))
        {
            if (PreviousCount)
                *PreviousCount = msg->User.ReleaseDriverUnloadProtection.PreviousCount;

            if (ClientPreviousCount)
                *ClientPreviousCount = msg->User.ReleaseDriverUnloadProtection.ClientPreviousCount;
        }
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphGetConnectedClientCount(
    _Out_ PULONG Count
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgGetConnectedClientCount);
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        *Count = msg->User.GetConnectedClientCount.Count;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphActivateDynData(
    _In_ PBYTE DynData,
    _In_ ULONG DynDataLength,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgActivateDynData);
    msg->User.ActivateDynData.DynData = DynData;
    msg->User.ActivateDynData.DynDataLength = DynDataLength;
    msg->User.ActivateDynData.Signature = Signature;
    msg->User.ActivateDynData.SignatureLength = SignatureLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.ActivateDynData.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphRequestSessionAccessToken(
    _Out_ PKPH_SESSION_ACCESS_TOKEN AccessToken,
    _In_ PLARGE_INTEGER Expiry,
    _In_ ULONG Privileges,
    _In_ LONG Uses
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgRequestSessionAccessToken);
    msg->User.RequestSessionAccessToken.Expiry = *Expiry;
    msg->User.RequestSessionAccessToken.Privileges = Privileges;
    msg->User.RequestSessionAccessToken.Uses = Uses;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        RtlCopyMemory(AccessToken,
                      &msg->User.RequestSessionAccessToken.AccessToken,
                      sizeof(KPH_SESSION_ACCESS_TOKEN));

        status = msg->User.RequestSessionAccessToken.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphAssignProcessSessionToken(
    _In_ HANDLE ProcessHandle,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgAssignProcessSessionToken);
    msg->User.AssignProcessSessionToken.ProcessHandle = ProcessHandle;
    msg->User.AssignProcessSessionToken.Signature = Signature;
    msg->User.AssignProcessSessionToken.SignatureLength = SignatureLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.AssignProcessSessionToken.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphAssignThreadSessionToken(
    _In_ HANDLE ThreadHandle,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgAssignThreadSessionToken);
    msg->User.AssignThreadSessionToken.ThreadHandle = ThreadHandle;
    msg->User.AssignThreadSessionToken.Signature = Signature;
    msg->User.AssignThreadSessionToken.SignatureLength = SignatureLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.AssignThreadSessionToken.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphGetInformerProcessFilter(
    _In_ HANDLE ProcessHandle,
    _Out_ PKPH_INFORMER_SETTINGS Filter
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgGetInformerProcessFilter);
    msg->User.GetInformerProcessFilter.ProcessHandle = ProcessHandle;
    msg->User.GetInformerProcessFilter.Filter = Filter;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.GetInformerProcessFilter.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphSetInformerProcessFilter(
    _In_opt_ HANDLE ProcessHandle,
    _In_ PKPH_INFORMER_SETTINGS Filter
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgSetInformerProcessFilter);
    msg->User.SetInformerProcessFilter.ProcessHandle = ProcessHandle;
    msg->User.SetInformerProcessFilter.Filter = Filter;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.SetInformerProcessFilter.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphStripProtectedProcessMasks(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK ProcessAllowedMask,
    _In_ ACCESS_MASK ThreadAllowedMask
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgStripProtectedProcessMasks);
    msg->User.StripProtectedProcessMasks.ProcessHandle = ProcessHandle;
    msg->User.StripProtectedProcessMasks.ProcessAllowedMask = ProcessAllowedMask;
    msg->User.StripProtectedProcessMasks.ThreadAllowedMask = ThreadAllowedMask;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.StripProtectedProcessMasks.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphQueryVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _In_ KPH_MEMORY_INFORMATION_CLASS MemoryInformationClass,
    _Out_writes_bytes_opt_(MemoryInformationLength) PVOID MemoryInformation,
    _In_ ULONG MemoryInformationLength,
    _Out_opt_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgQueryVirtualMemory);
    msg->User.QueryVirtualMemory.ProcessHandle = ProcessHandle;
    msg->User.QueryVirtualMemory.BaseAddress = BaseAddress;
    msg->User.QueryVirtualMemory.MemoryInformationClass = MemoryInformationClass;
    msg->User.QueryVirtualMemory.MemoryInformation = MemoryInformation;
    msg->User.QueryVirtualMemory.MemoryInformationLength = MemoryInformationLength;
    msg->User.QueryVirtualMemory.ReturnLength = ReturnLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.QueryVirtualMemory.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KsiQueryHashInformationFile(
    _In_ HANDLE FileHandle,
    _Inout_ PKPH_HASH_INFORMATION HashInformation,
    _In_ ULONG HashInformationLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgQueryHashInformationFile);
    msg->User.QueryHashInformationFile.FileHandle = FileHandle;
    msg->User.QueryHashInformationFile.HashingInformation = HashInformation;
    msg->User.QueryHashInformationFile.HashingInformationLength = HashInformationLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.QueryHashInformationFile.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphOpenDevice(
    _Out_ PHANDLE DeviceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgOpenDevice);
    msg->User.OpenDevice.DeviceHandle = DeviceHandle;
    msg->User.OpenDevice.DesiredAccess = DesiredAccess;
    msg->User.OpenDevice.ObjectAttributes = ObjectAttributes;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.OpenDevice.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphOpenDeviceDriver(
    _In_ HANDLE DeviceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE DriverHandle
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgOpenDeviceDriver);
    msg->User.OpenDeviceDriver.DeviceHandle = DeviceHandle;
    msg->User.OpenDeviceDriver.DesiredAccess = DesiredAccess;
    msg->User.OpenDeviceDriver.DriverHandle = DriverHandle;
    status = KphCommsSendMessage(msg);

    if (!NT_SUCCESS(status))
    {
        status = msg->User.OpenDeviceDriver.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;
}

NTSTATUS KphOpenDeviceBaseDevice(
    _In_ HANDLE DeviceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE BaseDeviceHandle
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    KSI_COMMS_INIT_ASSERT();

    msg = PhAllocateFromFreeList(&KphMessageFreeList);
    KphMsgInit(msg, KphMsgOpenDeviceBaseDevice);
    msg->User.OpenDeviceBaseDevice.DeviceHandle = DeviceHandle;
    msg->User.OpenDeviceBaseDevice.DesiredAccess = DesiredAccess;
    msg->User.OpenDeviceBaseDevice.BaseDeviceHandle = BaseDeviceHandle;
    status = KphCommsSendMessage(msg);

    if (!NT_SUCCESS(status))
    {
        status = msg->User.OpenDeviceDriver.Status;
    }

    PhFreeToFreeList(&KphMessageFreeList, msg);
    return status;

}
