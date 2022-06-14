/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2018-2020
 *
 */

#include <ph.h>
#include <kphuser.h>
#include <kphuserp.h>

HANDLE PhKphHandle = NULL;
BOOLEAN PhKphVerified = FALSE;
KPH_KEY PhKphL1Key = 0;

NTSTATUS KphConnect(
    _In_opt_ PWSTR DeviceName
    )
{
    NTSTATUS status;
    HANDLE kphHandle;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK isb;
    OBJECT_HANDLE_FLAG_INFORMATION handleFlagInfo;
    WCHAR fullObjectName[256];

    if (PhKphHandle)
        return STATUS_ADDRESS_ALREADY_EXISTS;

    if (DeviceName)
    {
        PH_FORMAT format[2];

        PhInitFormatS(&format[0], L"\\Device\\");
        PhInitFormatS(&format[1], DeviceName);

        if (!PhFormatToBuffer(format, 2, fullObjectName, sizeof(fullObjectName), NULL))
            return STATUS_NAME_TOO_LONG;

        RtlInitUnicodeString(&objectName, fullObjectName);
    }
    else
        RtlInitUnicodeString(&objectName, KPH_DEVICE_NAME);

    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenFile(
        &kphHandle,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        &objectAttributes,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_NON_DIRECTORY_FILE
        );

    if (NT_SUCCESS(status))
    {
        // Protect the handle from being closed.

        handleFlagInfo.Inherit = FALSE;
        handleFlagInfo.ProtectFromClose = TRUE;

        NtSetInformationObject(
            kphHandle,
            ObjectHandleFlagInformation,
            &handleFlagInfo,
            sizeof(OBJECT_HANDLE_FLAG_INFORMATION)
            );

        PhKphHandle = kphHandle;
        PhKphVerified = FALSE;
        PhKphL1Key = 0;
    }

    return status;
}

NTSTATUS KphConnect2Ex(
    _In_opt_ PWSTR ServiceName,
    _In_opt_ PWSTR DeviceName,
    _In_ PWSTR FileName,
    _In_opt_ PKPH_PARAMETERS Parameters
    )
{
    NTSTATUS status;
    SC_HANDLE scmHandle;
    SC_HANDLE serviceHandle = NULL;
    BOOLEAN started = FALSE;
    BOOLEAN created = FALSE;

    if (!ServiceName)
        ServiceName = KPH_DEVICE_SHORT_NAME;
    if (!DeviceName)
        DeviceName = KPH_DEVICE_SHORT_NAME;

    // Try to open the device.
    status = KphConnect(DeviceName);

    if (NT_SUCCESS(status) || status == STATUS_ADDRESS_ALREADY_EXISTS)
        return status;

    if (
        status != STATUS_NO_SUCH_DEVICE &&
        status != STATUS_NO_SUCH_FILE &&
        status != STATUS_OBJECT_NAME_NOT_FOUND &&
        status != STATUS_OBJECT_PATH_NOT_FOUND
        )
        return status;

    // Load the driver, and try again.

    // Try to start the service, if it exists.

    scmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

    if (scmHandle)
    {
        serviceHandle = OpenService(scmHandle, ServiceName, SERVICE_START);

        if (serviceHandle)
        {
            if (StartService(serviceHandle, 0, NULL))
                started = TRUE;

            CloseServiceHandle(serviceHandle);
        }

        CloseServiceHandle(scmHandle);
    }

    if (!started && PhDoesFileExistsWin32(FileName))
    {
        // Try to create the service.

        scmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);

        if (scmHandle)
        {
            serviceHandle = CreateService(
                scmHandle,
                ServiceName,
                ServiceName,
                SERVICE_ALL_ACCESS,
                SERVICE_KERNEL_DRIVER,
                SERVICE_DEMAND_START,
                SERVICE_ERROR_IGNORE,
                FileName,
                NULL,
                NULL,
                NULL,
                NULL,
                L""
                );

            if (serviceHandle)
            {
                created = TRUE;

                KphSetServiceSecurity(serviceHandle);

                // Set parameters if the caller supplied them. Note that we fail the entire function
                // if this fails, because failing to set parameters like SecurityLevel may result in
                // security vulnerabilities.
                if (Parameters)
                {
                    status = KphSetParameters(ServiceName, Parameters);

                    if (!NT_SUCCESS(status))
                    {
                        // Delete the service and fail.
                        goto CreateAndConnectEnd;
                    }
                }

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
        // Try to open the device again.
        status = KphConnect(DeviceName);
    }

CreateAndConnectEnd:
    if (created && serviceHandle)
    {
        // "Delete" the service. Since we (may) have a handle to the device, the SCM will delete the
        // service automatically when it is stopped (upon reboot). If we don't have a handle to the
        // device, the service will get deleted immediately, which is a good thing anyway.
        DeleteService(serviceHandle);
        CloseServiceHandle(serviceHandle);
    }

    return status;
}

NTSTATUS KphDisconnect(
    VOID
    )
{
    NTSTATUS status;
    OBJECT_HANDLE_FLAG_INFORMATION handleFlagInfo;

    if (!PhKphHandle)
        return STATUS_ALREADY_DISCONNECTED;

    // Unprotect the handle.

    handleFlagInfo.Inherit = FALSE;
    handleFlagInfo.ProtectFromClose = FALSE;

    NtSetInformationObject(
        PhKphHandle,
        ObjectHandleFlagInformation,
        &handleFlagInfo,
        sizeof(OBJECT_HANDLE_FLAG_INFORMATION)
        );

    status = NtClose(PhKphHandle);
    PhKphHandle = NULL;
    PhKphVerified = FALSE;
    PhKphL1Key = 0;

    return status;
}

BOOLEAN KphIsConnected(
    VOID
    )
{
    return PhKphHandle != NULL;
}

BOOLEAN KphIsVerified(
    VOID
    )
{
    return PhKphVerified;
}

NTSTATUS KphSetParameters(
    _In_opt_ PWSTR ServiceName,
    _In_ PKPH_PARAMETERS Parameters
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
    PhInitFormatS(&format[1], ServiceName ? ServiceName : KPH_DEVICE_SHORT_NAME);
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

    PhInitializeStringRefLongHint(&valueNameSr, L"SecurityLevel");
    status = PhSetValueKey(
        parametersKeyHandle,
        &valueNameSr,
        REG_DWORD,
        &Parameters->SecurityLevel,
        sizeof(ULONG)
        );

    if (!NT_SUCCESS(status))
        goto SetValuesEnd;

    if (Parameters->CreateDynamicConfiguration)
    {
        KPH_DYN_CONFIGURATION configuration;

        configuration.Version = KPH_DYN_CONFIGURATION_VERSION;
        configuration.NumberOfPackages = 1;

        if (NT_SUCCESS(KphInitializeDynamicPackage(&configuration.Packages[0])))
        {
            PhInitializeStringRefLongHint(&valueNameSr, L"DynamicConfiguration");
            status = PhSetValueKey(
                parametersKeyHandle,
                &valueNameSr,
                REG_BINARY,
                &configuration,
                sizeof(KPH_DYN_CONFIGURATION)
                );

            if (!NT_SUCCESS(status))
                goto SetValuesEnd;
        }
    }

    // Put more parameters here...

SetValuesEnd:
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
    _In_opt_ PWSTR ServiceName
    )
{
    NTSTATUS status;
    HANDLE parametersKeyHandle = NULL;
    PH_STRINGREF parametersKeyNameSr;
    PH_FORMAT format[3];
    SIZE_T returnLength;
    WCHAR parametersKeyName[MAX_PATH];

    PhInitFormatS(&format[0], L"System\\CurrentControlSet\\Services\\");
    PhInitFormatS(&format[1], ServiceName ? ServiceName : KPH_DEVICE_SHORT_NAME);
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
    _In_opt_ PWSTR ServiceName
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    HANDLE parametersKeyHandle = NULL;
    PH_STRINGREF parametersKeyNameSr;
    PH_FORMAT format[3];
    SIZE_T returnLength;
    WCHAR parametersKeyName[MAX_PATH];

    PhInitFormatS(&format[0], L"System\\CurrentControlSet\\Services\\");
    PhInitFormatS(&format[1], ServiceName ? ServiceName : KPH_DEVICE_SHORT_NAME);
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

    status = KphUninstall(ServiceName);
    status = WIN32_FROM_NTSTATUS(status);

    if (status == ERROR_SERVICE_DOES_NOT_EXIST)
        status = STATUS_SUCCESS;

    if (NT_SUCCESS(status))
    {
        status = PhOpenKey(
            &parametersKeyHandle,
            DELETE,
            PH_KEY_LOCAL_MACHINE,
            &parametersKeyNameSr,
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
    _In_opt_ PWSTR ServiceName,
    _In_ PWSTR FileName
    )
{
    return KphInstallEx(ServiceName, FileName, NULL);
}

NTSTATUS KphInstallEx(
    _In_opt_ PWSTR ServiceName,
    _In_ PWSTR FileName,
    _In_opt_ PKPH_PARAMETERS Parameters
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SC_HANDLE scmHandle;
    SC_HANDLE serviceHandle;

    if (!ServiceName)
        ServiceName = KPH_DEVICE_SHORT_NAME;

    scmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);

    if (!scmHandle)
        return PhGetLastWin32ErrorAsNtStatus();

    serviceHandle = CreateService(
        scmHandle,
        ServiceName,
        ServiceName,
        SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER,
        SERVICE_SYSTEM_START,
        SERVICE_ERROR_IGNORE,
        FileName,
        NULL,
        NULL,
        NULL,
        NULL,
        L""
        );

    if (serviceHandle)
    {
        KphSetServiceSecurity(serviceHandle);

        // See KphConnect2Ex for more details.
        if (Parameters)
        {
            status = KphSetParameters(ServiceName, Parameters);

            if (!NT_SUCCESS(status))
            {
                DeleteService(serviceHandle);
                goto CreateEnd;
            }
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
    _In_opt_ PWSTR ServiceName
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SC_HANDLE scmHandle;
    SC_HANDLE serviceHandle;

    scmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

    if (!scmHandle)
        return PhGetLastWin32ErrorAsNtStatus();

    serviceHandle = OpenService(scmHandle, ServiceName ? ServiceName : KPH_DEVICE_SHORT_NAME, SERVICE_STOP | DELETE);

    if (serviceHandle)
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

    CloseServiceHandle(scmHandle);

    return status;
}

NTSTATUS KphGetFeatures(
    _Inout_ PULONG Features
    )
{
    struct
    {
        PULONG Features;
    } input = { Features };

    return KphpDeviceIoControl(
        KPH_GETFEATURES,
        &input,
        sizeof(input)
        );
}

NTSTATUS KphVerifyClient(
    _In_reads_bytes_(SignatureSize) PUCHAR Signature,
    _In_ ULONG SignatureSize
    )
{
    NTSTATUS status;
    struct
    {
        PVOID CodeAddress;
        PUCHAR Signature;
        ULONG SignatureSize;
    } input = { KphpWithKeyApcRoutine, Signature, SignatureSize };

    status = KphpDeviceIoControl(
        KPH_VERIFYCLIENT,
        &input,
        sizeof(input)
        );

    if (NT_SUCCESS(status))
        PhKphVerified = TRUE;

    return status;
}

NTSTATUS KphOpenProcess(
    _Inout_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCLIENT_ID ClientId
    )
{
    KPH_OPEN_PROCESS_INPUT input = { ProcessHandle, DesiredAccess, ClientId, 0 };

    if ((DesiredAccess & KPH_PROCESS_READ_ACCESS) == DesiredAccess)
    {
        KphpGetL1Key(&input.Key);
        return KphpDeviceIoControl(
            KPH_OPENPROCESS,
            &input,
            sizeof(input)
            );
    }
    else
    {
        return KphpWithKey(KphKeyLevel2, KphpOpenProcessContinuation, &input);
    }
}

NTSTATUS KphOpenProcessToken(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Inout_ PHANDLE TokenHandle
    )
{
    KPH_OPEN_PROCESS_TOKEN_INPUT input = { ProcessHandle, DesiredAccess, TokenHandle, 0 };

    if ((DesiredAccess & KPH_TOKEN_READ_ACCESS) == DesiredAccess)
    {
        KphpGetL1Key(&input.Key);
        return KphpDeviceIoControl(
            KPH_OPENPROCESSTOKEN,
            &input,
            sizeof(input)
            );
    }
    else
    {
        return KphpWithKey(KphKeyLevel2, KphpOpenProcessTokenContinuation, &input);
    }
}

NTSTATUS KphOpenProcessJob(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Inout_ PHANDLE JobHandle
    )
{
    struct
    {
        HANDLE ProcessHandle;
        ACCESS_MASK DesiredAccess;
        PHANDLE JobHandle;
    } input = { ProcessHandle, DesiredAccess, JobHandle };

    return KphpDeviceIoControl(
        KPH_OPENPROCESSJOB,
        &input,
        sizeof(input)
        );
}

NTSTATUS KphTerminateProcess(
    _In_ HANDLE ProcessHandle,
    _In_ NTSTATUS ExitStatus
    )
{
    NTSTATUS status;
    KPH_TERMINATE_PROCESS_INPUT input = { ProcessHandle, ExitStatus, 0 };

    status = KphpWithKey(KphKeyLevel2, KphpTerminateProcessContinuation, &input);

    // Check if we're trying to terminate the current process, because kernel-mode can't do it.
    if (status == STATUS_CANT_TERMINATE_SELF)
    {
        PhExitApplication(ExitStatus);
    }

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
    KPH_READ_VIRTUAL_MEMORY_UNSAFE_INPUT input = { ProcessHandle, BaseAddress, Buffer, BufferSize, NumberOfBytesRead, 0 };

    return KphpWithKey(KphKeyLevel2, KphpReadVirtualMemoryUnsafeContinuation, &input);
}

NTSTATUS KphQueryInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    _Out_writes_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Inout_opt_ PULONG ReturnLength
    )
{
    struct
    {
        HANDLE ProcessHandle;
        KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass;
        PVOID ProcessInformation;
        ULONG ProcessInformationLength;
        PULONG ReturnLength;
    } input = { ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength };

    return KphpDeviceIoControl(
        KPH_QUERYINFORMATIONPROCESS,
        &input,
        sizeof(input)
        );
}

NTSTATUS KphSetInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    _In_reads_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength
    )
{
    struct
    {
        HANDLE ProcessHandle;
        KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass;
        PVOID ProcessInformation;
        ULONG ProcessInformationLength;
    } input = { ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength };

    return KphpDeviceIoControl(
        KPH_SETINFORMATIONPROCESS,
        &input,
        sizeof(input)
        );
}

NTSTATUS KphOpenThread(
    _Inout_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCLIENT_ID ClientId
    )
{
    KPH_OPEN_THREAD_INPUT input = { ThreadHandle, DesiredAccess, ClientId, 0 };

    if ((DesiredAccess & KPH_THREAD_READ_ACCESS) == DesiredAccess)
    {
        KphpGetL1Key(&input.Key);
        return KphpDeviceIoControl(
            KPH_OPENTHREAD,
            &input,
            sizeof(input)
            );
    }
    else
    {
        return KphpWithKey(KphKeyLevel2, KphpOpenThreadContinuation, &input);
    }
}

NTSTATUS KphOpenThreadProcess(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Inout_ PHANDLE ProcessHandle
    )
{
    struct
    {
        HANDLE ThreadHandle;
        ACCESS_MASK DesiredAccess;
        PHANDLE ProcessHandle;
    } input = { ThreadHandle, DesiredAccess, ProcessHandle };

    return KphpDeviceIoControl(
        KPH_OPENTHREADPROCESS,
        &input,
        sizeof(input)
        );
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
    struct
    {
        HANDLE ThreadHandle;
        ULONG FramesToSkip;
        ULONG FramesToCapture;
        PVOID *BackTrace;
        PULONG CapturedFrames;
        PULONG BackTraceHash;
    } input = { ThreadHandle, FramesToSkip, FramesToCapture, BackTrace, CapturedFrames, BackTraceHash };

    return KphpDeviceIoControl(
        KPH_CAPTURESTACKBACKTRACETHREAD,
        &input,
        sizeof(input)
        );
}

NTSTATUS KphQueryInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ KPH_THREAD_INFORMATION_CLASS ThreadInformationClass,
    _Out_writes_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _Inout_opt_ PULONG ReturnLength
    )
{
    struct
    {
        HANDLE ThreadHandle;
        KPH_THREAD_INFORMATION_CLASS ThreadInformationClass;
        PVOID ThreadInformation;
        ULONG ThreadInformationLength;
        PULONG ReturnLength;
    } input = { ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength };

    return KphpDeviceIoControl(
        KPH_QUERYINFORMATIONTHREAD,
        &input,
        sizeof(input)
        );
}

NTSTATUS KphSetInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ KPH_THREAD_INFORMATION_CLASS ThreadInformationClass,
    _In_reads_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength
    )
{
    struct
    {
        HANDLE ThreadHandle;
        KPH_THREAD_INFORMATION_CLASS ThreadInformationClass;
        PVOID ThreadInformation;
        ULONG ThreadInformationLength;
    } input = { ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationLength };

    return KphpDeviceIoControl(
        KPH_SETINFORMATIONTHREAD,
        &input,
        sizeof(input)
        );
}

NTSTATUS KphEnumerateProcessHandles(
    _In_ HANDLE ProcessHandle,
    _Out_writes_bytes_(BufferLength) PVOID Buffer,
    _In_opt_ ULONG BufferLength,
    _Inout_opt_ PULONG ReturnLength
    )
{
    struct
    {
        HANDLE ProcessHandle;
        PVOID Buffer;
        ULONG BufferLength;
        PULONG ReturnLength;
    } input = { ProcessHandle, Buffer, BufferLength, ReturnLength };

    return KphpDeviceIoControl(
        KPH_ENUMERATEPROCESSHANDLES,
        &input,
        sizeof(input)
        );
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
    _Out_writes_bytes_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength,
    _Inout_opt_ PULONG ReturnLength
    )
{
    struct
    {
        HANDLE ProcessHandle;
        HANDLE Handle;
        KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass;
        PVOID ObjectInformation;
        ULONG ObjectInformationLength;
        PULONG ReturnLength;
    } input = { ProcessHandle, Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength, ReturnLength };

    return KphpDeviceIoControl(
        KPH_QUERYINFORMATIONOBJECT,
        &input,
        sizeof(input)
        );
}

NTSTATUS KphSetInformationObject(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _In_reads_bytes_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength
    )
{
    struct
    {
        HANDLE ProcessHandle;
        HANDLE Handle;
        KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass;
        PVOID ObjectInformation;
        ULONG ObjectInformationLength;
    } input = { ProcessHandle, Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength };

    return KphpDeviceIoControl(
        KPH_SETINFORMATIONOBJECT,
        &input,
        sizeof(input)
        );
}

NTSTATUS KphOpenDriver(
    _Inout_ PHANDLE DriverHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    struct
    {
        PHANDLE DriverHandle;
        ACCESS_MASK DesiredAccess;
        POBJECT_ATTRIBUTES ObjectAttributes;
    } input = { DriverHandle, DesiredAccess, ObjectAttributes };

    return KphpDeviceIoControl(
        KPH_OPENDRIVER,
        &input,
        sizeof(input)
        );
}

NTSTATUS KphQueryInformationDriver(
    _In_ HANDLE DriverHandle,
    _In_ DRIVER_INFORMATION_CLASS DriverInformationClass,
    _Out_writes_bytes_(DriverInformationLength) PVOID DriverInformation,
    _In_ ULONG DriverInformationLength,
    _Inout_opt_ PULONG ReturnLength
    )
{
    struct
    {
        HANDLE DriverHandle;
        DRIVER_INFORMATION_CLASS DriverInformationClass;
        PVOID DriverInformation;
        ULONG DriverInformationLength;
        PULONG ReturnLength;
    } input = { DriverHandle, DriverInformationClass, DriverInformation, DriverInformationLength, ReturnLength };

    return KphpDeviceIoControl(
        KPH_QUERYINFORMATIONDRIVER,
        &input,
        sizeof(input)
        );
}

NTSTATUS KphpDeviceIoControl(
    _In_ ULONG KphControlCode,
    _In_ PVOID InBuffer,
    _In_ ULONG InBufferLength
    )
{
    IO_STATUS_BLOCK iosb;

    return NtDeviceIoControlFile(
        PhKphHandle,
        NULL,
        NULL,
        NULL,
        &iosb,
        KphControlCode,
        InBuffer,
        InBufferLength,
        NULL,
        0
        );
}

VOID KphpWithKeyApcRoutine(
    _In_ PVOID ApcContext,
    _In_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG Reserved
    )
{
    PKPHP_RETRIEVE_KEY_CONTEXT context = CONTAINING_RECORD(IoStatusBlock, KPHP_RETRIEVE_KEY_CONTEXT, Iosb);
    KPH_KEY key = PtrToUlong(ApcContext);

    if (context->Continuation != KphpGetL1KeyContinuation &&
        context->Continuation != KphpOpenProcessContinuation &&
        context->Continuation != KphpOpenProcessTokenContinuation &&
        context->Continuation != KphpTerminateProcessContinuation &&
        context->Continuation != KphpReadVirtualMemoryUnsafeContinuation &&
        context->Continuation != KphpOpenThreadContinuation)
    {
        PhRaiseStatus(STATUS_ACCESS_DENIED);
        context->Status = STATUS_ACCESS_DENIED;
        return;
    }

    context->Status = context->Continuation(key, context->Context);
}

NTSTATUS KphpWithKey(
    _In_ KPH_KEY_LEVEL KeyLevel,
    _In_ PKPHP_WITH_KEY_CONTINUATION Continuation,
    _In_ PVOID Context
    )
{
    NTSTATUS status;
    struct
    {
        KPH_KEY_LEVEL KeyLevel;
    } input = { KeyLevel };
    KPHP_RETRIEVE_KEY_CONTEXT context;

    context.Continuation = Continuation;
    context.Context = Context;
    context.Status = STATUS_UNSUCCESSFUL;

    status = NtDeviceIoControlFile(
        PhKphHandle,
        NULL,
        KphpWithKeyApcRoutine,
        NULL,
        &context.Iosb,
        KPH_RETRIEVEKEY,
        &input,
        sizeof(input),
        NULL,
        0
        );

    NtTestAlert();

    if (!NT_SUCCESS(status))
        return status;

    return context.Status;
}

NTSTATUS KphpGetL1KeyContinuation(
    _In_ KPH_KEY Key,
    _In_ PVOID Context
    )
{
    PKPHP_GET_L1_KEY_CONTEXT context = Context;

    *context->Key = Key;
    PhKphL1Key = Key;

    return STATUS_SUCCESS;
}

NTSTATUS KphpGetL1Key(
    _Inout_ PKPH_KEY Key
    )
{
    KPHP_GET_L1_KEY_CONTEXT context;

    if (PhKphL1Key)
    {
        *Key = PhKphL1Key;
        return STATUS_SUCCESS;
    }

    context.Key = Key;

    return KphpWithKey(KphKeyLevel1, KphpGetL1KeyContinuation, &context);
}

NTSTATUS KphpOpenProcessContinuation(
    _In_ KPH_KEY Key,
    _In_ PVOID Context
    )
{
    PKPH_OPEN_PROCESS_INPUT input = Context;

    input->Key = Key;

    return KphpDeviceIoControl(
        KPH_OPENPROCESS,
        input,
        sizeof(*input)
        );
}

NTSTATUS KphpOpenProcessTokenContinuation(
    _In_ KPH_KEY Key,
    _In_ PVOID Context
    )
{
    PKPH_OPEN_PROCESS_TOKEN_INPUT input = Context;

    input->Key = Key;

    return KphpDeviceIoControl(
        KPH_OPENPROCESSTOKEN,
        input,
        sizeof(*input)
        );
}

NTSTATUS KphpTerminateProcessContinuation(
    _In_ KPH_KEY Key,
    _In_ PVOID Context
    )
{
    PKPH_TERMINATE_PROCESS_INPUT input = Context;

    input->Key = Key;

    return KphpDeviceIoControl(
        KPH_TERMINATEPROCESS,
        input,
        sizeof(*input)
        );
}

NTSTATUS KphpReadVirtualMemoryUnsafeContinuation(
    _In_ KPH_KEY Key,
    _In_ PVOID Context
    )
{
    PKPH_READ_VIRTUAL_MEMORY_UNSAFE_INPUT input = Context;

    input->Key = Key;

    return KphpDeviceIoControl(
        KPH_READVIRTUALMEMORYUNSAFE,
        input,
        sizeof(*input)
        );
}

NTSTATUS KphpOpenThreadContinuation(
    _In_ KPH_KEY Key,
    _In_ PVOID Context
    )
{
    PKPH_OPEN_PROCESS_INPUT input = Context;

    input->Key = Key;

    return KphpDeviceIoControl(
        KPH_OPENTHREAD,
        input,
        sizeof(*input)
        );
}
