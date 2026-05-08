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

static CONST PH_STRINGREF KphDefaultPortName = PH_STRINGREF_INIT(KPH_PORT_NAME);
static PPH_OBJECT_TYPE KphMessageObjectType = NULL;
static PPH_OBJECT_TYPE KphUserMessageObjectType = NULL;

/**
 * Initializes the KPH message object types.
 */
VOID KphInitialize(
    VOID
    )
{
    PH_OBJECT_TYPE_PARAMETERS parameters;

    parameters.FreeListSize = ALIGN_UP_BY(KPH_MESSAGE_MIN_SIZE, 1024);
    parameters.FreeListCount = 65536;

    KphMessageObjectType = PhCreateObjectTypeEx(
        L"KsiMessage",
        PH_OBJECT_TYPE_TRY_USE_FREE_LIST,
        NULL,
        &parameters
        );

    parameters.FreeListSize = KPH_MESSAGE_MIN_SIZE;
    parameters.FreeListCount = 16;

    KphUserMessageObjectType = PhCreateObjectTypeEx(
        L"KsiUserMessage",
        PH_OBJECT_TYPE_USE_FREE_LIST,
        NULL,
        &parameters
        );
}

/**
 * Creates a new KPH message object.
 *
 * \param Size The size of the message object to create.
 * \return A pointer to the created message object.
 */
PKPH_MESSAGE KphCreateMessage(
    _In_ SIZE_T Size
    )
{
    assert(KphMessageObjectType);

    return PhCreateObject(Size, KphMessageObjectType);
}

/**
 * Creates a new KPH user message object.
 *
 * \param MessageId The message identifier for the new message.
 * \return A pointer to the created message object.
 */
PKPH_MESSAGE KphCreateUserMessage(
    _In_ KPH_MESSAGE_ID MessageId
    )
{
    PKPH_MESSAGE msg;

    assert(KphUserMessageObjectType);

    msg = PhCreateObject(KPH_MESSAGE_MIN_SIZE, KphUserMessageObjectType);

    KphMsgInit(msg, MessageId);

    return msg;
}

/**
 * Connects to the service.
 *
 * \param Config Configuration parameters for the connection.
 * \return Successful or errant status.
 */
NTSTATUS KphConnect(
    _In_ PKPH_CONFIG_PARAMETERS Config
    )
{
    NTSTATUS status;
    SC_HANDLE serviceHandle;
    BOOLEAN created = FALSE;
    PCPH_STRINGREF portName;

    portName = (Config->PortName ? Config->PortName : &KphDefaultPortName);

    status = KphCommsStart(portName, Config->Callback, Config->RingBufferLength);

    if (NT_SUCCESS(status) || (status == STATUS_ALREADY_INITIALIZED))
        return status;

    // Load the driver, and try again.

    if (Config->EnableNativeLoad || Config->EnableFilterLoad)
    {
        status = KsiLoadUnloadService(Config, TRUE);

        if (NT_SUCCESS(status))
        {
            status = KphCommsStart(
                portName,
                Config->Callback,
                Config->RingBufferLength
                );
        }

        return status;
    }

    // Try to start the service, if it exists.

    status = PhOpenService(
        &serviceHandle,
        SERVICE_START | SERVICE_QUERY_STATUS,
        PhGetStringRefZ(Config->ServiceName)
        );

    if (NT_SUCCESS(status))
    {
        status = PhStartService(serviceHandle, 0, NULL);

        if (NT_SUCCESS(status))
        {
            status = PhWaitForServiceStatus(
                serviceHandle,
                SERVICE_RUNNING,
                5000
                );
        }

        PhCloseServiceHandle(serviceHandle);
        serviceHandle = NULL;

        if (!NT_SUCCESS(status))
            goto CreateAndConnectEnd;

        status = KphCommsStart(
            portName,
            Config->Callback,
            Config->RingBufferLength
            );

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

    status = PhWaitForServiceStatus(serviceHandle, SERVICE_RUNNING, 5000);

    if (!NT_SUCCESS(status))
        goto CreateAndConnectEnd;

    status = KphCommsStart(portName, Config->Callback, Config->RingBufferLength);

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

/**
 * Sets service parameters in the registry.
 *
 * \param Config Configuration parameters for the service.
 * \return Successful or errant status.
 */
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
    PH_FORMAT format[3];
    WCHAR servicesKeyNameBuffer[MAX_PATH];

    PhInitFormatS(&format[0], L"System\\CurrentControlSet\\Services");
    PhInitFormatSR(&format[1], PhNtPathSeparatorString);
    PhInitFormatSR(&format[2], *Config->ServiceName);

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

/**
 * Sets optional driver parameters.
 *
 * \param Config Configuration parameters.
 * \return Successful or errant status.
 */
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
    PH_FORMAT format[4];
    WCHAR parametersKeyNameBuffer[MAX_PATH];

    // Services key parameters.
    status = KphpSetParametersService(Config);
    if (!NT_SUCCESS(status))
        return status;

    if (!Config->PortName &&
        !Config->Altitude &&
        !Config->Flags.Flags &&
        !Config->SystemProcessName)
    {
        // Don't create parameters key unless we must.
        return STATUS_SUCCESS;
    }

    PhInitFormatS(&format[0], L"System\\CurrentControlSet\\Services");
    PhInitFormatSR(&format[1], PhNtPathSeparatorString);
    PhInitFormatSR(&format[2], *Config->ServiceName);
    PhInitFormatS(&format[3], L"\\Parameters");

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
        status = PhSetValueKeyStringZ(parametersKeyHandle, L"PortName", Config->PortName);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    if (Config->Altitude)
    {
        status = PhSetValueKeyStringZ(parametersKeyHandle, L"Altitude", Config->Altitude);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    if (Config->Flags.Flags)
    {
        C_ASSERT(sizeof(KPH_PARAMETER_FLAGS) == sizeof(ULONG));

        status = PhSetValueKeyUlong(parametersKeyHandle, L"Flags", Config->Flags.Flags);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    if (Config->SystemProcessName)
    {
        status = PhSetValueKeyZ(
            parametersKeyHandle,
            L"SystemProcessName",
            REG_SZ,
            Config->SystemProcessName->Buffer,
            (ULONG)Config->SystemProcessName->Length + sizeof(UNICODE_NULL)
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

/**
 * Sets the security descriptor for the KPH service.
 *
 * \param ServiceHandle Handle to the service.
 * \return Successful or errant status.
 */
NTSTATUS KphSetServiceSecurity(
    _In_ SC_HANDLE ServiceHandle
    )
{
#define SERVICE_INTERACTIVE_ACCESS \
    (SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS | SERVICE_START | SERVICE_STOP | SERVICE_INTERROGATE | DELETE)
    NTSTATUS status;
    PSID administratorsSid = PhSeAdministratorsSid();
    UCHAR securityDescriptorBuffer[SECURITY_DESCRIPTOR_MIN_LENGTH + 0x80];
    PSECURITY_DESCRIPTOR securityDescriptor = (PSECURITY_DESCRIPTOR)securityDescriptorBuffer;
    PACL dacl = PTR_ADD_OFFSET(securityDescriptor, SECURITY_DESCRIPTOR_MIN_LENGTH);
    ULONG daclLength;

    if (!NT_SUCCESS(status = RtlULongAdd(SECURITY_DESCRIPTOR_MIN_LENGTH, sizeof(ACL), &daclLength)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = RtlULongAdd(daclLength, UFIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) + PhLengthSid(&PhSeServiceSid), &daclLength)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = RtlULongAdd(daclLength, UFIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) + PhLengthSid(administratorsSid), &daclLength)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = RtlULongAdd(daclLength, UFIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) + PhLengthSid(&PhSeInteractiveSid), &daclLength)))
        goto CleanupExit;

    if (!NT_SUCCESS(status = PhCreateSecurityDescriptor(securityDescriptor, SECURITY_DESCRIPTOR_REVISION)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhCreateAcl(dacl, daclLength - SECURITY_DESCRIPTOR_MIN_LENGTH, ACL_REVISION)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhAddAccessAllowedAce(dacl, ACL_REVISION, SERVICE_ALL_ACCESS, &PhSeServiceSid)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhAddAccessAllowedAce(dacl, ACL_REVISION, SERVICE_ALL_ACCESS, administratorsSid)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhAddAccessAllowedAce(dacl, ACL_REVISION, SERVICE_INTERACTIVE_ACCESS, &PhSeInteractiveSid)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhSetDaclSecurityDescriptor(securityDescriptor, TRUE, dacl, FALSE)))
        goto CleanupExit;

    status = PhSetServiceObjectSecurity(
        ServiceHandle,
        DACL_SECURITY_INFORMATION,
        securityDescriptor
        );

    NT_ASSERT(RtlValidSecurityDescriptor(securityDescriptor));
    NT_ASSERT(daclLength < sizeof(securityDescriptorBuffer));
    NT_ASSERT(PhLengthSecurityDescriptor(securityDescriptor) < sizeof(securityDescriptorBuffer));

CleanupExit:
    return status;
}

/**
 * Recursively deletes service subkeys during load/unload cleanup.
 *
 * \param RootDirectory Root key handle being enumerated.
 * \param Information Basic information for the current subkey.
 * \param Context Optional callback context.
 * \return TRUE to continue enumeration.
 */
_Function_class_(PH_ENUM_KEY_CALLBACK)
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

/**
 * Loads or unloads the service.
 *
 * \param Config Configuration parameters for the service.
 * \param LoadDriver TRUE to load the driver, FALSE to unload.
 * \return Successful or errant status.
 */
NTSTATUS KsiLoadUnloadService(
    _In_ PKPH_CONFIG_PARAMETERS Config,
    _In_ BOOLEAN LoadDriver
    )
{
#ifdef _WIN64
    static CONST PH_STRINGREF fullServicesKeyName = PH_STRINGREF_INIT(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    static CONST PH_STRINGREF parametersKeyName = PH_STRINGREF_INIT(L"Parameters");
    NTSTATUS status;
    PPH_STRING fullServiceKeyName;
    PPH_STRING fullServiceFileName;
    UNICODE_STRING driverServiceKeyName;
    HANDLE serviceKeyHandle;
    HANDLE parametersKeyHandle = NULL;
    ULONG disposition;

    NT_ASSERT(Config->FileName);
    NT_ASSERT(Config->ServiceName);
    NT_ASSERT(Config->ObjectName);

    fullServiceKeyName = PhConcatStringRef2(&fullServicesKeyName, Config->ServiceName);

    if (!PhStringRefToUnicodeString(&fullServiceKeyName->sr, &driverServiceKeyName))
    {
        PhDereferenceObject(fullServiceKeyName);
        return STATUS_NAME_TOO_LONG;
    }

    status = PhCreateKey(
        &serviceKeyHandle,
        KEY_WRITE | KEY_CREATE_SUB_KEY,
        NULL,
        &fullServiceKeyName->sr,
        0,
        0,
        &disposition
        );

    if (NT_SUCCESS(status) && disposition == REG_CREATED_NEW_KEY)
    {
        fullServiceFileName = PhConcatStringRef2(&PhNtDosDevicesPrefix, Config->FileName);

        if (!NT_SUCCESS(status = PhSetValueKeyUlong(serviceKeyHandle, L"ErrorControl", SERVICE_ERROR_NORMAL)) ||
            !NT_SUCCESS(status = PhSetValueKeyUlong(serviceKeyHandle, L"Type", SERVICE_KERNEL_DRIVER)) ||
            !NT_SUCCESS(status = PhSetValueKeyUlong(serviceKeyHandle, L"Start", SERVICE_DISABLED)) ||
            !NT_SUCCESS(status = PhSetValueKeyStringZ(serviceKeyHandle, L"ImagePath", &fullServiceFileName->sr)) ||
            !NT_SUCCESS(status = PhSetValueKeyStringZ(serviceKeyHandle, L"ObjectName", Config->ObjectName)) ||
            !NT_SUCCESS(status = KphSetParameters(Config)))
        {
            PhDereferenceObject(fullServiceFileName);
            NtClose(serviceKeyHandle);
            goto CleanupExit;
        }

        PhDereferenceObject(fullServiceFileName);
        NtClose(serviceKeyHandle);
    }

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (LoadDriver)
    {
        if (Config->EnableFilterLoad)
            status = PhFilterLoadUnload(Config->ServiceName, TRUE);
        else
            status = NtLoadDriver(&driverServiceKeyName);

        // Handle rare race condition with natively loading the driver.
        if (status == STATUS_IMAGE_ALREADY_LOADED)
            status = STATUS_SUCCESS;
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

CleanupExit:
    PhDereferenceObject(fullServiceKeyName);

    return status;
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

/**
 * Stops the KSI service.
 *
 * \param Config Configuration parameters for the service.
 * \return Successful or errant status.
 */
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

/**
 * Gets the current informer settings from KSI.
 *
 * \param Settings Pointer to a structure that receives the informer settings.
 * \return Successful or errant status.
 */
NTSTATUS KphGetInformerSettings(
    _Out_ PKPH_INFORMER_SETTINGS Settings
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgGetInformerSettings);
    msg->User.GetInformerSettings.Settings = Settings;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.GetInformerSettings.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Sets the driver settings in KSI.
 *
 * \param Settings Pointer to the informer settings to apply.
 * \return Successful or errant status.
 */
NTSTATUS KphSetInformerSettings(
    _In_ PKPH_INFORMER_SETTINGS Settings
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgSetInformerSettings);
    msg->User.SetInformerSettings.Settings = Settings;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.SetInformerSettings.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Opens a limited process.
 *
 * \param ProcessHandle A variable that receives the opened process handle.
 * \param DesiredAccess The access rights requested for the process.
 * \param ClientId Pointer to the client ID identifying the process.
 * \return Successful or errant status.
 */
NTSTATUS KphOpenProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCLIENT_ID ClientId
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgOpenProcess);
    msg->User.OpenProcess.ProcessHandle = ProcessHandle;
    msg->User.OpenProcess.DesiredAccess = DesiredAccess;
    msg->User.OpenProcess.ClientId = ClientId;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.OpenProcess.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Opens a limited process token.
 *
 * \param ProcessHandle Handle to the target process.
 * \param DesiredAccess The access rights requested for the token.
 * \param TokenHandle A variable that receives the opened token handle.
 * \return Successful or errant status.
 */
NTSTATUS KphOpenProcessToken(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE TokenHandle
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgOpenProcessToken);
    msg->User.OpenProcessToken.ProcessHandle = ProcessHandle;
    msg->User.OpenProcessToken.DesiredAccess = DesiredAccess;
    msg->User.OpenProcessToken.TokenHandle = TokenHandle;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.OpenProcessToken.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Opens the job object associated with a process.
 *
 * \param ProcessHandle Handle to the target process.
 * \param DesiredAccess The access rights requested for the job object.
 * \param JobHandle A variable that receives the opened job handle.
 * \return Successful or errant status.
 */
NTSTATUS KphOpenProcessJob(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE JobHandle
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgOpenProcessJob);
    msg->User.OpenProcessJob.ProcessHandle = ProcessHandle;
    msg->User.OpenProcessJob.DesiredAccess = DesiredAccess;
    msg->User.OpenProcessJob.JobHandle = JobHandle;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.OpenProcessJob.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Terminates a process through KSI.
 *
 * \param ProcessHandle Handle to the target process.
 * \param ExitStatus Exit status supplied for the terminated process.
 * \return Successful or errant status.
 */
NTSTATUS KphTerminateProcess(
    _In_ HANDLE ProcessHandle,
    _In_ NTSTATUS ExitStatus
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgTerminateProcess);
    msg->User.TerminateProcess.ProcessHandle = ProcessHandle;
    msg->User.TerminateProcess.ExitStatus = ExitStatus;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.TerminateProcess.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Reads limited memory from a process.
 *
 * \param ProcessHandle Handle to the target process, or NULL for the current process.
 * \param BaseAddress Base address to read from.
 * \param Buffer Buffer that receives the copied bytes.
 * \param BufferSize Size of the destination buffer, in bytes.
 * \param NumberOfBytesRead Optional variable that receives the number of bytes read.
 * \return Successful or errant status.
 */
NTSTATUS KphReadVirtualMemory(
    _In_opt_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Out_opt_ PSIZE_T NumberOfBytesRead
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgReadVirtualMemory);
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

    PhDereferenceObject(msg);
    return status;
}

/**
 * Opens a limited thread.
 *
 * \param ThreadHandle A variable that receives the opened thread handle.
 * \param DesiredAccess The access rights requested for the thread.
 * \param ClientId Pointer to the client ID identifying the thread.
 * \return Successful or errant status.
 */
NTSTATUS KphOpenThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCLIENT_ID ClientId
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgOpenThread);
    msg->User.OpenThread.ThreadHandle = ThreadHandle;
    msg->User.OpenThread.DesiredAccess = DesiredAccess;
    msg->User.OpenThread.ClientId = ClientId;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.OpenThread.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Opens the owning process for a thread.
 *
 * \param ThreadHandle Handle to the target thread.
 * \param DesiredAccess The access rights requested for the process.
 * \param ProcessHandle A variable that receives the opened process handle.
 * \return Successful or errant status.
 */
NTSTATUS KphOpenThreadProcess(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE ProcessHandle
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgOpenThreadProcess);
    msg->User.OpenThreadProcess.ThreadHandle = ThreadHandle;
    msg->User.OpenThreadProcess.DesiredAccess = DesiredAccess;
    msg->User.OpenThreadProcess.ProcessHandle = ProcessHandle;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.OpenThreadProcess.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Captures a limited stack backtrace for a thread.
 *
 * \param ThreadHandle Handle to the target thread.
 * \param FramesToSkip Number of initial frames to skip.
 * \param FramesToCapture Maximum number of frames to capture.
 * \param BackTrace Buffer that receives the captured stack frames.
 * \param CapturedFrames Variable that receives the number of frames captured.
 * \param BackTraceHash Optional variable that receives the backtrace hash.
 * \param Flags Capture behavior flags.
 * \return Successful or errant status.
 */
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

    msg = KphCreateUserMessage(KphMsgCaptureStackBackTraceThread);
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

    PhDereferenceObject(msg);
    return status;
}

/**
 * Enumerates handles for a process.
 *
 * \param ProcessHandle Handle to the target process.
 * \param Buffer Buffer that receives the handle information.
 * \param BufferLength Size of the buffer, in bytes.
 * \param ReturnLength Optional variable that receives the required or returned size.
 * \return Successful or errant status.
 */
NTSTATUS KphEnumerateProcessHandles(
    _In_ HANDLE ProcessHandle,
    _Out_writes_bytes_(BufferLength) PVOID Buffer,
    _In_opt_ ULONG BufferLength,
    _Inout_opt_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgEnumerateProcessHandles);
    msg->User.EnumerateProcessHandles.ProcessHandle = ProcessHandle;
    msg->User.EnumerateProcessHandles.Buffer = Buffer;
    msg->User.EnumerateProcessHandles.BufferLength = BufferLength;
    msg->User.EnumerateProcessHandles.ReturnLength = ReturnLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.EnumerateProcessHandles.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Enumerates handles for a process.
 *
 * \param ProcessHandle Handle to the process.
 * \param Handles Pointer to receive the handle information buffer.
 * \return Successful or errant status.
 */
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

/**
 * Queries limited object information through KSI.
 *
 * \param ProcessHandle Handle to the process that owns the object handle.
 * \param Handle Handle to the target object.
 * \param ObjectInformationClass The object information class to query.
 * \param ObjectInformation Buffer that receives the queried information.
 * \param ObjectInformationLength Size of the buffer, in bytes.
 * \param ReturnLength Optional variable that receives the returned size.
 * \return Successful or errant status.
 */
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

    msg = KphCreateUserMessage(KphMsgQueryInformationObject);
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

    PhDereferenceObject(msg);
    return status;
}

/**
 * Queries the name of a thread object.
 *
 * \param ProcessHandle Handle to the process.
 * \param Handle Handle to the thread object.
 * \param ThreadName Pointer to receive the thread name string.
 * \return Successful or errant status.
 */
NTSTATUS KphQueryObjectThreadName(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _Out_ PPH_STRING* ThreadName
    )
{
    NTSTATUS status;
    ULONG bufferSize;
    ULONG returnLength;
    PTHREAD_NAME_INFORMATION buffer;

    returnLength = 0;
    bufferSize = 0x100;
    buffer = PhAllocateStack(bufferSize);
    if (!buffer) return STATUS_NO_MEMORY;

    status = KphQueryInformationObject(
        ProcessHandle,
        Handle,
        KphObjectThreadNameInformation,
        buffer,
        bufferSize,
        &returnLength
        );

    if (status == STATUS_BUFFER_TOO_SMALL && returnLength > 0)
    {
        PhFreeStack(buffer);
        bufferSize = returnLength;
        buffer = PhAllocateStack(returnLength);

        status = KphQueryInformationObject(
            ProcessHandle,
            Handle,
            KphObjectThreadNameInformation,
            buffer,
            bufferSize,
            &returnLength
            );
    }

    if (NT_SUCCESS(status))
    {
        *ThreadName = PhCreateStringFromUnicodeString(&buffer->ThreadName);
    }

    PhFreeStack(buffer);

    return status;
}

/**
 * Queries limited section mapping information for an object.
 *
 * \param ProcessHandle Handle to the process.
 * \param Handle Handle to the object.
 * \param Info Pointer to receive the section mapping information.
 * \return Successful or errant status.
 */
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
        status = KphQueryInformationObject(
            ProcessHandle,
            Handle,
            KphObjectSectionMappingsInformation,
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

    *Info = buffer;

    return status;
}

/**
 * Sets limited information for the object.
 *
 * \param ProcessHandle Handle to the process.
 * \param Handle Handle to the object.
 * \param ObjectInformationClass Information class to set.
 * \param ObjectInformation Buffer containing the information to set.
 * \param ObjectInformationLength Length of the buffer.
 * \return Successful or errant status.
 */
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

    msg = KphCreateUserMessage(KphMsgSetInformationObject);
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

    PhDereferenceObject(msg);
    return status;
}

/**
 * Opens a limited handle to the driver.
 *
 * \param DriverHandle Pointer to receive the handle to the driver.
 * \param DesiredAccess Desired access rights.
 * \param ObjectAttributes Object attributes for the driver.
 * \return Successful or errant status.
 */
NTSTATUS KphOpenDriver(
    _Out_ PHANDLE DriverHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgOpenDriver);
    msg->User.OpenDriver.DriverHandle = DriverHandle;
    msg->User.OpenDriver.DesiredAccess = DesiredAccess;
    msg->User.OpenDriver.ObjectAttributes = ObjectAttributes;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.OpenDriver.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Queries limited driver information.
 *
 * \param DriverHandle Handle to the target driver object.
 * \param DriverInformationClass The driver information class to query.
 * \param DriverInformation Buffer that receives the queried information.
 * \param DriverInformationLength Size of the buffer, in bytes.
 * \param ReturnLength Optional variable that receives the returned size.
 * \return Successful or errant status.
 */
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

    msg = KphCreateUserMessage(KphMsgQueryInformationDriver);
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

    PhDereferenceObject(msg);
    return status;
}

/**
 * Queries limited process information.
 *
 * \param ProcessHandle Handle to the target process.
 * \param ProcessInformationClass The process information class to query.
 * \param ProcessInformation Buffer that receives the queried information.
 * \param ProcessInformationLength Size of the buffer, in bytes.
 * \param ReturnLength Optional variable that receives the returned size.
 * \return Successful or errant status.
 */
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

    msg = KphCreateUserMessage(KphMsgQueryInformationProcess);
    msg->User.QueryInformationProcess.ProcessHandle = ProcessHandle;
    msg->User.QueryInformationProcess.ProcessInformationClass = ProcessInformationClass;
    msg->User.QueryInformationProcess.ProcessInformation = ProcessInformation;
    msg->User.QueryInformationProcess.ProcessInformationLength = ProcessInformationLength;
    msg->User.QueryInformationProcess.ReturnLength = ReturnLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.QueryInformationProcess.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Gets the process state for a given process handle.
 *
 * \param ProcessHandle Handle to the process.
 * \return The process state value.
 */
KPH_PROCESS_STATE KphGetProcessState(
    _In_ HANDLE ProcessHandle
    )
{
    if (KphCommsIsConnected())
    {
        KPH_PROCESS_STATE state;

        if (NT_SUCCESS(KphQueryInformationProcess(
            ProcessHandle,
            KphProcessStateInformation,
            &state,
            sizeof(state),
            NULL
            )))
        {
            return state;
        }
    }

    return KPH_PROCESS_STATE_NONE;
}

/**
 * Gets the process state for the current process.
 *
 * \return The process state value.
 */
KPH_PROCESS_STATE KphGetCurrentProcessState(
    VOID
    )
{
    return KphGetProcessState(NtCurrentProcess());
}

/**
 * Gets the access level for a process.
 *
 * \param ProcessHandle Handle to the process.
 * \return The KSI access level.
 */
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

/**
 * Gets the current process access level, optionally using a cached value.
 *
 * \param Cached Whether to use the cached value.
 * \return The KSI access level.
 */
KPH_LEVEL KphLevelEx(
    _In_ BOOLEAN Cached
    )
{
    static KPH_LEVEL level = KphLevelNone;

    if (!Cached)
        level = KphProcessLevel(NtCurrentProcess());

    return level;
}

/**
 * Gets the access level for the current process (cached).
 * \return The access level.
 */
KPH_LEVEL KsiLevel(
    VOID
    )
{
    return KphLevelEx(TRUE);
}

/**
 * Sets limited information for a process.
 *
 * \param ProcessHandle Handle to the process.
 * \param ProcessInformationClass The information class to set.
 * \param ProcessInformation Buffer containing the information to set.
 * \param ProcessInformationLength Length of the buffer.
 * \return Successful or errant status.
 */
NTSTATUS KphSetInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    _In_reads_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgSetInformationProcess);
    msg->User.SetInformationProcess.ProcessHandle = ProcessHandle;
    msg->User.SetInformationProcess.ProcessInformationClass = ProcessInformationClass;
    msg->User.SetInformationProcess.ProcessInformation = ProcessInformation;
    msg->User.SetInformationProcess.ProcessInformationLength = ProcessInformationLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.SetInformationProcess.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Sets limited information for a thread.
 *
 * \param ThreadHandle Handle to the thread.
 * \param ThreadInformationClass The information class to set.
 * \param ThreadInformation Buffer containing the information to set.
 * \param ThreadInformationLength Length of the buffer.
 * \return Successful or errant status.
 */
NTSTATUS KphSetInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ KPH_THREAD_INFORMATION_CLASS ThreadInformationClass,
    _In_reads_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgSetInformationThread);
    msg->User.SetInformationThread.ThreadHandle = ThreadHandle;
    msg->User.SetInformationThread.ThreadInformationClass = ThreadInformationClass;
    msg->User.SetInformationThread.ThreadInformation = ThreadInformation;
    msg->User.SetInformationThread.ThreadInformationLength = ThreadInformationLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.SetInformationThread.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Sends a limited system control command.
 *
 * \param SystemControlClass The system control class.
 * \param SystemControlInfo Buffer containing the control information.
 * \param SystemControlInfoLength Length of the buffer.
 * \return Successful or errant status.
 */
NTSTATUS KphSystemControl(
    _In_ KPH_SYSTEM_CONTROL_CLASS SystemControlClass,
    _In_reads_bytes_(SystemControlInfoLength) PVOID SystemControlInfo,
    _In_ ULONG SystemControlInfoLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgSystemControl);
    msg->User.SystemControl.SystemControlClass = SystemControlClass;
    msg->User.SystemControl.SystemControlInfo = SystemControlInfo;
    msg->User.SystemControl.SystemControlInfoLength = SystemControlInfoLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.SystemControl.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Queries limited ALPC information for the object.
 *
 * \param ProcessHandle Handle to the process.
 * \param PortHandle Handle to the ALPC port.
 * \param AlpcInformationClass The ALPC information class.
 * \param AlpcInformation Buffer to receive the information.
 * \param AlpcInformationLength Length of the buffer.
 * \param ReturnLength Pointer to receive the length of data returned.
 * \return Successful or errant status.
 */
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

    msg = KphCreateUserMessage(KphMsgAlpcQueryInformation);
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

    PhDereferenceObject(msg);
    return status;
}

/**
 * Queries limited ALPC communication names information.
 *
 * \param ProcessHandle Handle to the process.
 * \param PortHandle Handle to the ALPC port.
 * \param Names Pointer to receive the communication names information.
 * \return Successful or errant status.
 */
NTSTATUS KphAlpcQueryCommunicationsNamesInfo(
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

/**
 * Queries limited file information.
 *
 * \param ProcessHandle Handle to the process that owns the file handle.
 * \param FileHandle Handle to the target file.
 * \param FileInformationClass The file information class to query.
 * \param FileInformation Buffer that receives the queried information.
 * \param FileInformationLength Size of the buffer, in bytes.
 * \param IoStatusBlock Pointer to the I/O status block to receive native status details.
 * \return Successful or errant status.
 */
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

    msg = KphCreateUserMessage(KphMsgQueryInformationFile);
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

    PhDereferenceObject(msg);
    return status;
}

/**
 * Queries limited volume information for a file.
 *
 * \param ProcessHandle Handle to the process.
 * \param FileHandle Handle to the file.
 * \param FsInformationClass Information class to query.
 * \param FsInformation Buffer for the information.
 * \param FsInformationLength Length of the buffer.
 * \param IoStatusBlock Pointer to the I/O status block.
 * \return Successful or errant status.
 */
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

    msg = KphCreateUserMessage(KphMsgQueryVolumeInformationFile);
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

    PhDereferenceObject(msg);
    return status;
}

/**
 * Duplicates an object handle.
 *
 * \param ProcessHandle Handle to the process.
 * \param SourceHandle Source handle.
 * \param DesiredAccess Desired access rights.
 * \param TargetHandle Pointer to receive the target handle.
 * \return Successful or errant status.
 */
NTSTATUS KphDuplicateObject(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE SourceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE TargetHandle
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgDuplicateObject);
    msg->User.DuplicateObject.ProcessHandle = ProcessHandle;
    msg->User.DuplicateObject.SourceHandle = SourceHandle;
    msg->User.DuplicateObject.DesiredAccess = DesiredAccess;
    msg->User.DuplicateObject.TargetHandle = TargetHandle;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.DuplicateObject.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Queries the performance counter and frequency.
 *
 * \param PerformanceCounter Pointer to receive the performance counter value.
 * \param PerformanceFrequency Pointer to receive the performance frequency value (optional).
 * \return Successful or errant status.
 */
NTSTATUS KphQueryPerformanceCounter(
    _Out_ PLARGE_INTEGER PerformanceCounter,
    _Out_opt_ PLARGE_INTEGER PerformanceFrequency
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgQueryPerformanceCounter);
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

    PhDereferenceObject(msg);
    return status;
}

/**
 * Creates or opens a file.
 *
 * \param FileHandle Pointer to receive the file handle.
 * \param DesiredAccess Desired access rights.
 * \param ObjectAttributes Object attributes for the file.
 * \param IoStatusBlock Pointer to the I/O status block.
 * \param AllocationSize Optional allocation size.
 * \param FileAttributes File attributes.
 * \param ShareAccess Share access flags.
 * \param CreateDisposition Create disposition value.
 * \param CreateOptions Create options flags.
 * \param EaBuffer Optional extended attributes buffer.
 * \param EaLength Length of the extended attributes buffer.
 * \param Options Additional options.
 * \return Successful or errant status.
 */
NTSTATUS KphCreateFile(
    _Out_ PHANDLE FileHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
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

    msg = KphCreateUserMessage(KphMsgCreateFile);
    msg->User.CreateFile.FileHandle = FileHandle;
    msg->User.CreateFile.DesiredAccess = DesiredAccess;
    msg->User.CreateFile.ObjectAttributes = ObjectAttributes;
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

    PhDereferenceObject(msg);
    return status;
}

/**
 * Queries information about a thread.
 *
 * \param ThreadHandle Handle to the thread.
 * \param ThreadInformationClass The information class to query.
 * \param ThreadInformation Buffer to receive the information.
 * \param ThreadInformationLength Length of the buffer.
 * \param ReturnLength Pointer to receive the length of data returned (optional).
 * \return Successful or errant status.
 */
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

    msg = KphCreateUserMessage(KphMsgQueryInformationThread);
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

    PhDereferenceObject(msg);
    return status;
}

/**
 * Queries limited information about a section object.
 *
 * \param SectionHandle Handle to the section.
 * \param SectionInformationClass The information class to query.
 * \param SectionInformation Buffer to receive the information.
 * \param SectionInformationLength Length of the buffer.
 * \param ReturnLength Pointer to receive the length of data returned (optional).
 * \return Successful or errant status.
 */
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

    msg = KphCreateUserMessage(KphMsgQuerySection);
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

    PhDereferenceObject(msg);
    return status;
}

/**
 * Queries limited section mapping information for a section object.
 *
 * \param SectionHandle Handle to the section.
 * \param Info Pointer to receive the section mapping information.
 * \return Successful or errant status.
 */
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

/**
 * Compares two object handles for equality.
 *
 * \param ProcessHandle Handle to the process.
 * \param FirstObjectHandle First object handle.
 * \param SecondObjectHandle Second object handle.
 * \return Successful or errant status.
 */
NTSTATUS KphCompareObjects(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE FirstObjectHandle,
    _In_ HANDLE SecondObjectHandle
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgCompareObjects);
    msg->User.CompareObjects.ProcessHandle = ProcessHandle;
    msg->User.CompareObjects.FirstObjectHandle = FirstObjectHandle;
    msg->User.CompareObjects.SecondObjectHandle = SecondObjectHandle;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.CompareObjects.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Gets the driver client settings.
 *
 * \param Settings Pointer to receive the client settings.
 * \return Successful or errant status.
 */
NTSTATUS KphGetInformerClientSettings(
    _Out_ PKPH_INFORMER_CLIENT_SETTINGS Settings
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgGetInformerClientSettings);
    msg->User.GetInformerClientSettings.Settings = Settings;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.GetInformerClientSettings.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Sets the driver client settings.
 *
 * \param Settings Pointer to the client settings to set.
 * \return Successful or errant status.
 */
NTSTATUS KphSetInformerClientSettings(
    _Out_ PKPH_INFORMER_CLIENT_SETTINGS Settings
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgSetInformerClientSettings);
    msg->User.SetInformerClientSettings.Settings = Settings;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.SetInformerClientSettings.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Acquires driver unload protection.
 *
 * \param PreviousCount Pointer to receive the previous count (optional).
 * \param ClientPreviousCount Pointer to receive the client previous count (optional).
 * \return Successful or errant status.
 */
NTSTATUS KphAcquireDriverUnloadProtection(
    _Out_opt_ PLONG PreviousCount,
    _Out_opt_ PLONG ClientPreviousCount
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    if (PreviousCount)
        *PreviousCount = 0;

    if (ClientPreviousCount)
        *ClientPreviousCount = 0;

    msg = KphCreateUserMessage(KphMsgAcquireDriverUnloadProtection);
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

    PhDereferenceObject(msg);
    return status;
}

/**
 * Releases driver unload protection.
 *
 * \param PreviousCount Pointer to receive the previous count (optional).
 * \param ClientPreviousCount Pointer to receive the client previous count (optional).
 * \return Successful or errant status.
 */
NTSTATUS KphReleaseDriverUnloadProtection(
    _Out_opt_ PLONG PreviousCount,
    _Out_opt_ PLONG ClientPreviousCount
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgReleaseDriverUnloadProtection);
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

    PhDereferenceObject(msg);
    return status;
}

/**
 * Gets the number of connected clients.
 *
 * \param Count Pointer to receive the client count.
 * \return Successful or errant status.
 */
NTSTATUS KphGetConnectedClientCount(
    _Out_ PULONG Count
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgGetConnectedClientCount);
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        *Count = msg->User.GetConnectedClientCount.Count;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Activates dynamic data.
 *
 * \param DynData Pointer to the dynamic data.
 * \param DynDataLength Length of the dynamic data.
 * \param Signature Pointer to the signature.
 * \param SignatureLength Length of the signature.
 * \return Successful or errant status.
 */
NTSTATUS KphActivateDynData(
    _In_ PBYTE DynData,
    _In_ ULONG DynDataLength,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgActivateDynData);
    msg->User.ActivateDynData.DynData = DynData;
    msg->User.ActivateDynData.DynDataLength = DynDataLength;
    msg->User.ActivateDynData.Signature = Signature;
    msg->User.ActivateDynData.SignatureLength = SignatureLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.ActivateDynData.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Checks if dynamic data is active.
 *
 * \param IsActive Pointer to receive the active state.
 * \return Successful or errant status.
 */
NTSTATUS KphIsDynDataActive(
    _Out_ PBOOLEAN IsActive
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    *IsActive = FALSE;

    if (!KphCommsIsConnected())
        return STATUS_PORT_DISCONNECTED;

    msg = KphCreateUserMessage(KphMsgIsDynDataActive);
    msg->User.IsDynDataActive.IsActive = IsActive;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.IsDynDataActive.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Requests a session access token.
 *
 * \param AccessToken Pointer to receive the access token.
 * \param Expiry Pointer to the expiry time.
 * \param Privileges Privileges to assign.
 * \param Uses Number of uses for the token.
 * \return Successful or errant status.
 */
NTSTATUS KphRequestSessionAccessToken(
    _Out_ PKPH_SESSION_ACCESS_TOKEN AccessToken,
    _In_ PLARGE_INTEGER Expiry,
    _In_ ULONG Privileges,
    _In_ LONG Uses
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgRequestSessionAccessToken);
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

    PhDereferenceObject(msg);
    return status;
}

/**
 * Assigns a session token to a process.
 *
 * \param ProcessHandle Handle to the process.
 * \param Signature Pointer to the signature.
 * \param SignatureLength Length of the signature.
 * \return Successful or errant status.
 */
NTSTATUS KphAssignProcessSessionToken(
    _In_ HANDLE ProcessHandle,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgAssignProcessSessionToken);
    msg->User.AssignProcessSessionToken.ProcessHandle = ProcessHandle;
    msg->User.AssignProcessSessionToken.Signature = Signature;
    msg->User.AssignProcessSessionToken.SignatureLength = SignatureLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.AssignProcessSessionToken.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Assigns a session token to a thread.
 *
 * \param ThreadHandle Handle to the thread.
 * \param Signature Pointer to the signature.
 * \param SignatureLength Length of the signature.
 * \return Successful or errant status.
 */
NTSTATUS KphAssignThreadSessionToken(
    _In_ HANDLE ThreadHandle,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgAssignThreadSessionToken);
    msg->User.AssignThreadSessionToken.ThreadHandle = ThreadHandle;
    msg->User.AssignThreadSessionToken.Signature = Signature;
    msg->User.AssignThreadSessionToken.SignatureLength = SignatureLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.AssignThreadSessionToken.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Gets the informer settings for a process.
 *
 * \param ProcessHandle Handle to the process.
 * \param Settings Pointer to receive the informer settings.
 * \return Successful or errant status.
 */
NTSTATUS KphGetInformerProcessSettings(
    _In_ HANDLE ProcessHandle,
    _Out_ PKPH_INFORMER_SETTINGS Settings
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgGetInformerProcessSettings);
    msg->User.GetInformerProcessSettings.ProcessHandle = ProcessHandle;
    msg->User.GetInformerProcessSettings.Settings = Settings;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.GetInformerProcessSettings.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Sets the informer settings for a process.
 *
 * \param ProcessHandle Handle to the process (optional).
 * \param Settings Pointer to the informer settings to set.
 * \return Successful or errant status.
 */
NTSTATUS KphSetInformerProcessSettings(
    _In_opt_ HANDLE ProcessHandle,
    _In_ PKPH_INFORMER_SETTINGS Settings
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgSetInformerProcessSettings);
    msg->User.SetInformerProcessSettings.ProcessHandle = ProcessHandle;
    msg->User.SetInformerProcessSettings.Settings = Settings;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.SetInformerProcessSettings.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Strips protected process masks.
 *
 * \param ProcessHandle Handle to the process.
 * \param ProcessAllowedMask Allowed process access mask.
 * \param ThreadAllowedMask Allowed thread access mask.
 * \return Successful or errant status.
 */
NTSTATUS KphStripProtectedProcessMasks(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK ProcessAllowedMask,
    _In_ ACCESS_MASK ThreadAllowedMask
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgStripProtectedProcessMasks);
    msg->User.StripProtectedProcessMasks.ProcessHandle = ProcessHandle;
    msg->User.StripProtectedProcessMasks.ProcessAllowedMask = ProcessAllowedMask;
    msg->User.StripProtectedProcessMasks.ThreadAllowedMask = ThreadAllowedMask;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.StripProtectedProcessMasks.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Queries virtual memory information for a process.
 *
 * \param ProcessHandle Handle to the process.
 * \param BaseAddress Optional base address.
 * \param MemoryInformationClass The memory information class to query.
 * \param MemoryInformation Buffer to receive the information.
 * \param MemoryInformationLength Length of the buffer.
 * \param ReturnLength Pointer to receive the length of data returned (optional).
 * \return Successful or errant status.
 */
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

    msg = KphCreateUserMessage(KphMsgQueryVirtualMemory);
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

    PhDereferenceObject(msg);
    return status;
}

/**
 * Queries hash information for a file.
 *
 * \param FileHandle Handle to the file.
 * \param HashInformation Pointer to the hash information structure.
 * \param HashInformationLength Length of the hash information structure.
 * \return Successful or errant status.
 */
NTSTATUS KsiQueryHashInformationFile(
    _In_ HANDLE FileHandle,
    _Inout_ PKPH_HASH_INFORMATION HashInformation,
    _In_ ULONG HashInformationLength
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgQueryHashInformationFile);
    msg->User.QueryHashInformationFile.FileHandle = FileHandle;
    msg->User.QueryHashInformationFile.HashingInformation = HashInformation;
    msg->User.QueryHashInformationFile.HashingInformationLength = HashInformationLength;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.QueryHashInformationFile.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Opens a device.
 *
 * \param DeviceHandle Pointer to receive the device handle.
 * \param DesiredAccess Desired access rights.
 * \param ObjectAttributes Object attributes for the device.
 * \return Successful or errant status.
 */
NTSTATUS KphOpenDevice(
    _Out_ PHANDLE DeviceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgOpenDevice);
    msg->User.OpenDevice.DeviceHandle = DeviceHandle;
    msg->User.OpenDevice.DesiredAccess = DesiredAccess;
    msg->User.OpenDevice.ObjectAttributes = ObjectAttributes;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.OpenDevice.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Opens a device driver.
 *
 * \param DeviceHandle Handle to the device.
 * \param DesiredAccess Desired access rights.
 * \param DriverHandle Pointer to receive the driver handle.
 * \return Successful or errant status.
 */
NTSTATUS KphOpenDeviceDriver(
    _In_ HANDLE DeviceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE DriverHandle
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgOpenDeviceDriver);
    msg->User.OpenDeviceDriver.DeviceHandle = DeviceHandle;
    msg->User.OpenDeviceDriver.DesiredAccess = DesiredAccess;
    msg->User.OpenDeviceDriver.DriverHandle = DriverHandle;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.OpenDeviceDriver.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Opens the base device for a device.
 *
 * \param DeviceHandle Handle to the device.
 * \param DesiredAccess Desired access rights.
 * \param BaseDeviceHandle Pointer to receive the base device handle.
 * \return Successful or errant status.
 */
NTSTATUS KphOpenDeviceBaseDevice(
    _In_ HANDLE DeviceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE BaseDeviceHandle
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgOpenDeviceBaseDevice);
    msg->User.OpenDeviceBaseDevice.DeviceHandle = DeviceHandle;
    msg->User.OpenDeviceBaseDevice.DesiredAccess = DesiredAccess;
    msg->User.OpenDeviceBaseDevice.BaseDeviceHandle = BaseDeviceHandle;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.OpenDeviceBaseDevice.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Gets informer statistics.
 *
 * \param ProcessHandle Optional handle to the process.
 * \param Stats Pointer to receive the informer statistics.
 * \return Successful or errant status.
 */
NTSTATUS KphGetInformerStats(
    _In_opt_ HANDLE ProcessHandle,
    _Out_ PKPH_INFORMER_STATS Stats
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgGetInformerStats);
    msg->User.GetInformerStats.ProcessHandle = ProcessHandle;
    msg->User.GetInformerStats.Stats = Stats;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.GetInformerStats.Status;
    }

    PhDereferenceObject(msg);
    return status;
}

/**
 * Gets informer client statistics.
 *
 * \param Stats Pointer to receive the informer client statistics.
 * \return Successful or errant status.
 */
NTSTATUS KphGetInformerClientStats(
    _Out_ PKPH_INFORMER_CLIENT_STATS Stats
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    msg = KphCreateUserMessage(KphMsgGetInformerClientStats);
    msg->User.GetInformerClientStats.Stats = Stats;
    status = KphCommsSendMessage(msg);

    if (NT_SUCCESS(status))
    {
        status = msg->User.GetInformerClientStats.Status;
    }

    PhDereferenceObject(msg);
    return status;
}
