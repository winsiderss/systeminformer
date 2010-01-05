/*
 * Process Hacker - 
 *   KProcessHacker interfacing
 * 
 * Copyright (C) 2009-2010 wj32
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

#include <kph.h>
#include <wchar.h>

typedef struct _KPH_ZWQUERYOBJECT_BUFFER
{
    NTSTATUS Status;
    ULONG ReturnLength;
    PVOID BufferBase;
    CHAR Buffer[1];
} KPH_ZWQUERYOBJECT_BUFFER, *PKPH_ZWQUERYOBJECT_BUFFER;

NTSTATUS KphpDeviceIoControl(
    HANDLE KphHandle,
    ULONG KphControlCode,
    PVOID InBuffer,
    ULONG InBufferLength,
    PVOID OutBuffer,
    ULONG OutBufferLength,
    PULONG ReturnLength
    );

NTSTATUS KphConnect(
    __out PHANDLE KphHandle,
    __in_opt PWSTR DeviceName
    )
{
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK isb;

    if (DeviceName)
        RtlInitUnicodeString(&objectName, DeviceName);
    else
        RtlInitUnicodeString(&objectName, KPH_DEVICE_NAME);

    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    return NtOpenFile(
        KphHandle,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        &objectAttributes,
        &isb,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );
}

NTSTATUS KphConnect2(
    __out PHANDLE KphHandle,
    __in PWSTR DeviceName,
    __in PWSTR FileName
    )
{
    NTSTATUS status;
    WCHAR fullDeviceName[MAX_PATH + 1];
    SC_HANDLE scmHandle;
    SC_HANDLE serviceHandle;
    BOOLEAN started = FALSE;
    BOOLEAN created = FALSE;

    _snwprintf(fullDeviceName, MAX_PATH, L"\\Device\\%s", DeviceName);

    // Try to open the device.
    status = KphConnect(KphHandle, fullDeviceName);

    if (NT_SUCCESS(status))
        return status;

    if (
        status != STATUS_NO_SUCH_DEVICE &&
        status != STATUS_NO_SUCH_FILE &&
        status != STATUS_OBJECT_NAME_NOT_FOUND
        )
        return status;

    // Load the driver, and try again.

    // Try to start the service, if it exists.

    scmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

    if (scmHandle)
    {
        serviceHandle = OpenService(scmHandle, DeviceName, SERVICE_START);

        if (serviceHandle)
        {
            if (StartService(serviceHandle, 0, NULL))
                started = TRUE;

            CloseServiceHandle(serviceHandle);
        }

        CloseServiceHandle(scmHandle);
    }

    if (!started)
    {
        // Try to create the service.

        scmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);

        if (scmHandle)
        {
            serviceHandle = CreateService(
                scmHandle,
                DeviceName,
                DeviceName,
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
                StartService(serviceHandle, 0, NULL);
            }

            CloseServiceHandle(scmHandle);
        }
    }

    // Try to open the device again.
    status = KphConnect(KphHandle, fullDeviceName);

    if (created)
    {
        // "Delete" the service. Since we (may) have a handle to 
        // the device, the SCM will delete the service automatically 
        // when it is stopped (upon reboot).
        DeleteService(serviceHandle);
        CloseServiceHandle(serviceHandle);
    }

    return status;
}

NTSTATUS KphDisconnect(
    __in HANDLE KphHandle
    )
{
    if (CloseHandle(KphHandle))
        return STATUS_SUCCESS;
    else
        return STATUS_INVALID_HANDLE;
}

NTSTATUS KphpDeviceIoControl(
    HANDLE KphHandle,
    ULONG KphControlCode,
    PVOID InBuffer,
    ULONG InBufferLength,
    PVOID OutBuffer,
    ULONG OutBufferLength,
    PULONG ReturnLength
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;

    status = NtDeviceIoControlFile(
        KphHandle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
        KphControlCode,
        InBuffer,
        InBufferLength,
        OutBuffer,
        OutBufferLength
        );

    if (NT_SUCCESS(status) && ReturnLength)
        *ReturnLength = (ULONG)ioStatusBlock.Information;

    return status;
}

NTSTATUS KphGetFeatures(
    __in HANDLE KphHandle,
    __out PULONG Features
    )
{
    NTSTATUS status;
    ULONG features;

    if (NT_SUCCESS(status = KphpDeviceIoControl(
        KphHandle,
        KPH_GETFEATURES,
        NULL,
        0,
        &features,
        sizeof(ULONG),
        NULL
        )))
        *Features = features;

    return status;
}

NTSTATUS KphCloseHandle(
    __in HANDLE KphHandle,
    __in HANDLE Handle
    )
{
    struct
    {
        HANDLE Handle;
    } args;

    args.Handle = Handle;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_CLOSEHANDLE,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphSsQueryClientEntry(
    __in HANDLE KphHandle,
    __in HANDLE ClientEntryHandle,
    __out PKPHSS_CLIENT_INFORMATION ClientInformation,
    __in ULONG ClientInformationLength,
    __out PULONG ReturnLength
    )
{
    struct
    {
        HANDLE ClientEntryHandle;
        PKPHSS_CLIENT_INFORMATION ClientInformation;
        ULONG ClientInformationLength;
        PULONG ReturnLength;
    } args;

    args.ClientEntryHandle = ClientEntryHandle;
    args.ClientInformation = ClientInformation;
    args.ClientInformationLength = ClientInformationLength;
    args.ReturnLength = ReturnLength;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_SSQUERYCLIENTENTRY,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphOpenProcess(
    __in HANDLE KphHandle,
    __out PHANDLE ProcessHandle,
    __in HANDLE ProcessId,
    __in ACCESS_MASK DesiredAccess
    )
{
    NTSTATUS status;
    struct
    {
        HANDLE ProcessId;
        ACCESS_MASK DesiredAccess;
    } args;
    struct
    {
        HANDLE ProcessHandle;
    } ret;

    args.ProcessId = ProcessId;
    args.DesiredAccess = DesiredAccess;

    status = KphpDeviceIoControl(
        KphHandle,
        KPH_OPENPROCESS,
        &args,
        sizeof(args),
        &ret,
        sizeof(ret),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *ProcessHandle = ret.ProcessHandle;
    }

    return status;
}

NTSTATUS KphOpenThread(
    __in HANDLE KphHandle,
    __out PHANDLE ThreadHandle,
    __in HANDLE ThreadId,
    __in ACCESS_MASK DesiredAccess
    )
{
    NTSTATUS status;
    struct
    {
        HANDLE ThreadId;
        ACCESS_MASK DesiredAccess;
    } args;
    struct
    {
        HANDLE ThreadHandle;
    } ret;

    args.ThreadId = ThreadId;
    args.DesiredAccess = DesiredAccess;

    status = KphpDeviceIoControl(
        KphHandle,
        KPH_OPENTHREAD,
        &args,
        sizeof(args),
        &ret,
        sizeof(ret),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *ThreadHandle = ret.ThreadHandle;
    }

    return status;
}

NTSTATUS KphOpenProcessToken(
    __in HANDLE KphHandle,
    __out PHANDLE TokenHandle,
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess
    )
{
    NTSTATUS status;
    struct
    {
        HANDLE ProcessHandle;
        ACCESS_MASK DesiredAccess;
    } args;
    struct
    {
        HANDLE TokenHandle;
    } ret;

    args.ProcessHandle = ProcessHandle;
    args.DesiredAccess = DesiredAccess;

    status = KphpDeviceIoControl(
        KphHandle,
        KPH_OPENPROCESSTOKEN,
        &args,
        sizeof(args),
        &ret,
        sizeof(ret),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *TokenHandle = ret.TokenHandle;
    }

    return status;
}

NTSTATUS KphGetProcessProtected(
    __in HANDLE KphHandle,
    __in HANDLE ProcessId,
    __out PBOOLEAN IsProtected
    )
{
    NTSTATUS status;
    struct
    {
        HANDLE ProcessId;
    } args;
    struct
    {
        BOOLEAN IsProtected;
    } ret;

    args.ProcessId = ProcessId;

    status = KphpDeviceIoControl(
        KphHandle,
        KPH_GETPROCESSPROTECTED,
        &args,
        sizeof(args),
        &ret,
        sizeof(ret),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *IsProtected = ret.IsProtected;
    }

    return status;
}

NTSTATUS KphSetProcessProtected(
    __in HANDLE KphHandle,
    __in HANDLE ProcessId,
    __in BOOLEAN IsProtected
    )
{
    struct
    {
        HANDLE ProcessId;
        BOOLEAN IsProtected;
    } args;

    args.ProcessId = ProcessId;
    args.IsProtected = IsProtected;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_SETPROCESSPROTECTED,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphTerminateProcess(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in NTSTATUS ExitStatus
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    struct
    {
        HANDLE ProcessHandle;
        NTSTATUS ExitStatus;
    } args;

    args.ProcessHandle = ProcessHandle;
    args.ExitStatus = ExitStatus;

    status = KphpDeviceIoControl(
        KphHandle,
        KPH_TERMINATEPROCESS,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );

    /* Check if we were trying to terminate the current 
     * process and do it now. */
    if (status == STATUS_CANT_TERMINATE_SELF)
        status = NtTerminateProcess(NtCurrentProcess(), ExitStatus);

    return status;
}

NTSTATUS KphSuspendProcess(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle
    )
{
    struct
    {
        HANDLE ProcessHandle;
    } args;

    args.ProcessHandle = ProcessHandle;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_SUSPENDPROCESS,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphResumeProcess(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle
    )
{
    struct
    {
        HANDLE ProcessHandle;
    } args;

    args.ProcessHandle = ProcessHandle;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_RESUMEPROCESS,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphReadVirtualMemory(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __out_bcount(BufferLength) PVOID Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG ReturnLength
    )
{
    struct
    {
        HANDLE ProcessHandle;
        PVOID BaseAddress;
        PVOID Buffer;
        ULONG BufferLength;
        PULONG ReturnLength;
    } args;

    args.ProcessHandle = ProcessHandle;
    args.BaseAddress = BaseAddress;
    args.Buffer = Buffer;
    args.BufferLength = BufferLength;
    args.ReturnLength = ReturnLength;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_READVIRTUALMEMORY,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphWriteVirtualMemory(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __in_bcount(BufferLength) PVOID Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG ReturnLength
    )
{
    struct
    {
        HANDLE ProcessHandle;
        PVOID BaseAddress;
        PVOID Buffer;
        ULONG BufferLength;
        PULONG ReturnLength;
    } args;

    args.ProcessHandle = ProcessHandle;
    args.BaseAddress = BaseAddress;
    args.Buffer = Buffer;
    args.BufferLength = BufferLength;
    args.ReturnLength = ReturnLength;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_WRITEVIRTUALMEMORY,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphSetProcessToken(
    __in HANDLE KphHandle,
    __in HANDLE SourceProcessId,
    __in HANDLE TargetProcessId
    )
{
    struct
    {
        HANDLE SourceProcessId;
        HANDLE TargetProcessId;
    } args;

    args.SourceProcessId = SourceProcessId;
    args.TargetProcessId = TargetProcessId;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_SETPROCESSTOKEN,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphGetThreadStartAddress(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __out PPVOID StartAddress
    )
{
    NTSTATUS status;
    struct
    {
        HANDLE ThreadHandle;
    } args;
    struct
    {
        PVOID StartAddress;
    } ret;

    args.ThreadHandle = ThreadHandle;

    status = KphpDeviceIoControl(
        KphHandle,
        KPH_GETTHREADSTARTADDRESS,
        &args,
        sizeof(args),
        &ret,
        sizeof(ret),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *StartAddress = ret.StartAddress;
    }

    return status;
}

NTSTATUS KphSetHandleAttributes(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __in ULONG Flags
    )
{
    struct
    {
        HANDLE ProcessHandle;
        HANDLE Handle;
        ULONG Flags;
    } args;

    args.ProcessHandle = ProcessHandle;
    args.Handle = Handle;
    args.Flags = Flags;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_SETHANDLEATTRIBUTES,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphGetHandleObjectName(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __out_bcount_opt(BufferLength) PUNICODE_STRING Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG ReturnLength
    )
{
    NTSTATUS status;
    UNICODE_STRING buffer;
    ULONG returnLength;
    struct
    {
        HANDLE ProcessHandle;
        HANDLE Handle;
    } args;

    if (Buffer && BufferLength >= sizeof(UNICODE_STRING))
    {
        status = KphpDeviceIoControl(
            KphHandle,
            KPH_GETHANDLEOBJECTNAME,
            &args,
            sizeof(args),
            Buffer,
            BufferLength,
            &returnLength
            );

        if (ReturnLength)
        {
            if (status == STATUS_BUFFER_TOO_SMALL)
                *ReturnLength = Buffer->Length;
            else
                *ReturnLength = returnLength;
        }
    }
    else
    {
        // User doesn't give us a buffer, or it was too small.
        // Use our internal buffer.
        status = KphpDeviceIoControl(
            KphHandle,
            KPH_GETHANDLEOBJECTNAME,
            &args,
            sizeof(args),
            &buffer,
            sizeof(UNICODE_STRING),
            &returnLength
            );

        if (ReturnLength)
        {
            if (status == STATUS_BUFFER_TOO_SMALL)
                *ReturnLength = buffer.Length;
            else
                *ReturnLength = returnLength;
        }
    }

    return status;
}

NTSTATUS KphOpenProcessJob(
    __in HANDLE KphHandle,
    __out PHANDLE JobHandle,
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess
    )
{
    NTSTATUS status;
    struct
    {
        HANDLE ProcessHandle;
        ACCESS_MASK DesiredAccess;
    } args;
    struct
    {
        HANDLE JobHandle;
    } ret;

    args.ProcessHandle = ProcessHandle;
    args.DesiredAccess = DesiredAccess;

    status = KphpDeviceIoControl(
        KphHandle,
        KPH_OPENPROCESSJOB,
        &args,
        sizeof(args),
        &ret,
        sizeof(ret),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *JobHandle = ret.JobHandle;
    }

    return status;
}

NTSTATUS KphGetContextThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __inout PCONTEXT ThreadContext
    )
{
    struct
    {
        HANDLE ThreadHandle;
        PCONTEXT ThreadContext;
    } args;

    args.ThreadHandle = ThreadHandle;
    args.ThreadContext = ThreadContext;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_GETCONTEXTTHREAD,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphSetContextThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in PCONTEXT ThreadContext
    )
{
    struct
    {
        HANDLE ThreadHandle;
        PCONTEXT ThreadContext;
    } args;

    args.ThreadHandle = ThreadHandle;
    args.ThreadContext = ThreadContext;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_SETCONTEXTTHREAD,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphGetThreadWin32Thread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __out PPVOID Win32Thread
    )
{
    NTSTATUS status;
    struct
    {
        HANDLE ThreadHandle;
    } args;
    struct
    {
        PVOID Win32Thread;
    } ret;

    args.ThreadHandle = ThreadHandle;

    status = KphpDeviceIoControl(
        KphHandle,
        KPH_GETTHREADWIN32THREAD,
        &args,
        sizeof(args),
        &ret,
        sizeof(ret),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *Win32Thread = ret.Win32Thread;
    }

    return status;
}

NTSTATUS KphDuplicateObject(
    __in HANDLE KphHandle,
    __in HANDLE SourceProcessHandle,
    __in HANDLE SourceHandle,
    __in_opt HANDLE TargetProcessHandle,
    __out_opt PHANDLE TargetHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG HandleAttributes,
    __in ULONG Options
    )
{
    struct
    {
        HANDLE SourceProcessHandle;
        HANDLE SourceHandle;
        HANDLE TargetProcessHandle;
        PHANDLE TargetHandle;
        ACCESS_MASK DesiredAccess;
        ULONG HandleAttributes;
        ULONG Options;
    } args;

    args.SourceProcessHandle = SourceProcessHandle;
    args.SourceHandle = SourceHandle;
    args.TargetProcessHandle = TargetProcessHandle;
    args.TargetHandle = TargetHandle;
    args.DesiredAccess = DesiredAccess;
    args.HandleAttributes = HandleAttributes;
    args.Options = Options;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_DUPLICATEOBJECT,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphZwQueryObject(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __in OBJECT_INFORMATION_CLASS ObjectInformationClass,
    __in_bcount_opt(BufferLength) PVOID Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG ReturnLength
    )
{
    NTSTATUS status;
    struct
    {
        HANDLE ProcessHandle;
        HANDLE Handle;
        OBJECT_INFORMATION_CLASS ObjectInformationClass;
    } args;
    PKPH_ZWQUERYOBJECT_BUFFER ret;
    ULONG retLength;

    // Make sure we understand the object information class 
    // because we will be fixing the buffer later.
    if (
        ObjectInformationClass != ObjectBasicInformation &&
        ObjectInformationClass != ObjectNameInformation &&
        ObjectInformationClass != ObjectTypeInformation
        )
        return STATUS_INVALID_INFO_CLASS;

    args.ProcessHandle = ProcessHandle;
    args.Handle = Handle;
    args.ObjectInformationClass = ObjectInformationClass;

    retLength = FIELD_OFFSET(KPH_ZWQUERYOBJECT_BUFFER, Buffer) + BufferLength;
    ret = PhAllocate(retLength);
    status = KphpDeviceIoControl(
        KphHandle,
        KPH_ZWQUERYOBJECT,
        &args,
        sizeof(args),
        ret,
        retLength,
        NULL
        );

    // Make sure the actual I/O control request succeeded.
    if (NT_SUCCESS(status))
    {
        // Check if ZwQueryObject succeeded.
        status = ret->Status;

        if (NT_SUCCESS(status))
        {
            if (Buffer)
            {
                // Copy the buffer contents to the user buffer.
                memcpy(Buffer, ret->Buffer, ret->ReturnLength);

                // Rebase pointers in the buffer.
                if (ObjectInformationClass == ObjectNameInformation)
                {
                    POBJECT_NAME_INFORMATION nameInfo = (POBJECT_NAME_INFORMATION)Buffer;

                    nameInfo->Name.Buffer = (PWSTR)REBASE_ADDRESS(
                        nameInfo->Name.Buffer,
                        ret->BufferBase,
                        Buffer
                        );
                }
                else if (ObjectInformationClass == ObjectTypeInformation)
                {
                    POBJECT_TYPE_INFORMATION typeInfo = (POBJECT_TYPE_INFORMATION)Buffer;

                    typeInfo->TypeName.Buffer = (PWSTR)REBASE_ADDRESS(
                        typeInfo->TypeName.Buffer,
                        ret->BufferBase,
                        Buffer
                        );
                }
            }
        }

        if (ReturnLength)
            *ReturnLength = ret->ReturnLength;
    }

    PhFree(ret);

    return status;
}

NTSTATUS KphGetProcessId(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __out PHANDLE ProcessId
    )
{
    NTSTATUS status;
    struct
    {
        HANDLE ProcessHandle;
        HANDLE Handle;
    } args;
    struct
    {
        HANDLE ProcessId;
    } ret;

    args.ProcessHandle = ProcessHandle;
    args.Handle = Handle;

    status = KphpDeviceIoControl(
        KphHandle,
        KPH_GETPROCESSID,
        &args,
        sizeof(args),
        &ret,
        sizeof(ret),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *ProcessId = ret.ProcessId;
    }

    return status;
}

NTSTATUS KphGetThreadId(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __out PHANDLE ThreadId,
    __out_opt PHANDLE ProcessId
    )
{
    NTSTATUS status;
    struct
    {
        HANDLE ProcessHandle;
        HANDLE Handle;
    } args;
    struct
    {
        HANDLE ThreadId;
        HANDLE ProcessId;
    } ret;

    args.ProcessHandle = ProcessHandle;
    args.Handle = Handle;

    status = KphpDeviceIoControl(
        KphHandle,
        KPH_GETTHREADID,
        &args,
        sizeof(args),
        &ret,
        sizeof(ret),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *ThreadId = ret.ThreadId;

        if (ProcessId)
            *ProcessId = ret.ProcessId;
    }

    return status;
}

NTSTATUS KphTerminateThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    struct
    {
        HANDLE ThreadHandle;
        NTSTATUS ExitStatus;
    } args;

    args.ThreadHandle = ThreadHandle;
    args.ExitStatus = ExitStatus;

    status = KphpDeviceIoControl(
        KphHandle,
        KPH_TERMINATETHREAD,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );

    if (status == STATUS_CANT_TERMINATE_SELF)
        status = NtTerminateThread(NtCurrentThread(), ExitStatus);

    return status;
}

NTSTATUS KphSetHandleGrantedAccess(
    __in HANDLE KphHandle,
    __in HANDLE Handle,
    __in ACCESS_MASK GrantedAccess
    )
{
    struct
    {
        HANDLE Handle;
        ACCESS_MASK GrantedAccess;
    } args;

    args.Handle = Handle;
    args.GrantedAccess = GrantedAccess;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_SETHANDLEGRANTEDACCESS,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphAssignImpersonationToken(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in HANDLE TokenHandle
    )
{
    struct
    {
        HANDLE ThreadHandle;
        HANDLE TokenHandle;
    } args;

    args.ThreadHandle = ThreadHandle;
    args.TokenHandle = TokenHandle;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_ASSIGNIMPERSONATIONTOKEN,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphProtectAdd(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in BOOLEAN AllowKernelMode,
    __in ACCESS_MASK ProcessAllowMask,
    __in ACCESS_MASK ThreadAllowMask
    )
{
    struct
    {
        HANDLE ProcessHandle;
        LOGICAL AllowKernelMode;
        ACCESS_MASK ProcessAllowMask;
        ACCESS_MASK ThreadAllowMask;
    } args;

    args.ProcessHandle = ProcessHandle;
    args.AllowKernelMode = AllowKernelMode;
    args.ProcessAllowMask = ProcessAllowMask;
    args.ThreadAllowMask = ThreadAllowMask;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_PROTECTADD,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphProtectRemove(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle
    )
{
    struct
    {
        HANDLE ProcessHandle;
    } args;

    args.ProcessHandle = ProcessHandle;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_PROTECTREMOVE,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphProtectQuery(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __out PBOOLEAN AllowKernelMode,
    __out PACCESS_MASK ProcessAllowMask,
    __out PACCESS_MASK ThreadAllowMask
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    struct
    {
        HANDLE ProcessHandle;
        PLOGICAL AllowKernelMode;
        PACCESS_MASK ProcessAllowMask;
        PACCESS_MASK ThreadAllowMask;
    } args;
    LOGICAL allowKernelMode;

    args.ProcessHandle = ProcessHandle;
    args.AllowKernelMode = &allowKernelMode;
    args.ProcessAllowMask = ProcessAllowMask;
    args.ThreadAllowMask = ThreadAllowMask;

    status = KphpDeviceIoControl(
        KphHandle,
        KPH_PROTECTQUERY,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *AllowKernelMode = (BOOLEAN)allowKernelMode;
    }

    return status;
}

NTSTATUS KphUnsafeReadVirtualMemory(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __in_bcount(BufferLength) PVOID Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG ReturnLength
    )
{
    struct
    {
        HANDLE ProcessHandle;
        PVOID BaseAddress;
        PVOID Buffer;
        ULONG BufferLength;
        PULONG ReturnLength;
    } args;

    args.ProcessHandle = ProcessHandle;
    args.BaseAddress = BaseAddress;
    args.Buffer = Buffer;
    args.BufferLength = BufferLength;
    args.ReturnLength = ReturnLength;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_UNSAFEREADVIRTUALMEMORY,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphSetExecuteOptions(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in ULONG ExecuteOptions
    )
{
    struct
    {
        HANDLE ProcessHandle;
        ULONG ExecuteOptions;
    } args;

    args.ProcessHandle = ProcessHandle;
    args.ExecuteOptions = ExecuteOptions;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_SETEXECUTEOPTIONS,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphQueryProcessHandles(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __out_bcount_opt(BufferLength) PVOID Buffer,
    __in_opt ULONG BufferLength,
    __out_opt PULONG ReturnLength
    )
{
    struct
    {
        HANDLE ProcessHandle;
        PVOID Buffer;
        ULONG BufferLength;
        PULONG ReturnLength;
    } args;

    args.ProcessHandle = ProcessHandle;
    args.Buffer = Buffer;
    args.BufferLength = BufferLength;
    args.ReturnLength = ReturnLength;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_QUERYPROCESSHANDLES,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphOpenThreadProcess(
    __in HANDLE KphHandle,
    __out PHANDLE ProcessHandle,
    __in HANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess
    )
{
    NTSTATUS status;

    struct
    {
        HANDLE ThreadHandle;
        ACCESS_MASK DesiredAccess;
    } args;
    struct
    {
        HANDLE ProcessHandle;
    } ret;

    args.ThreadHandle = ThreadHandle;
    args.DesiredAccess = DesiredAccess;

    status = KphpDeviceIoControl(
        KphHandle,
        KPH_OPENTHREADPROCESS,
        &args,
        sizeof(args),
        &ret,
        sizeof(ret),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *ProcessHandle = ret.ProcessHandle;
    }

    return status;
}

NTSTATUS KphCaptureStackBackTraceThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in ULONG FramesToSkip,
    __in ULONG FramesToCapture,
    __out_ecount(FramesToCapture) PPVOID BackTrace,
    __out_opt PULONG CapturedFrames,
    __out_opt PULONG BackTraceHash
    )
{
    struct
    {
        HANDLE ThreadHandle;
        ULONG FramesToSkip;
        ULONG FramesToCapture;
        PPVOID BackTrace;
        PULONG CapturedFrames;
        PULONG BackTraceHash;
    } args;

    args.ThreadHandle = ThreadHandle;
    args.FramesToSkip = FramesToSkip;
    args.FramesToCapture = FramesToCapture;
    args.BackTrace = BackTrace;
    args.CapturedFrames = CapturedFrames;
    args.BackTraceHash = BackTraceHash;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_CAPTURESTACKBACKTRACETHREAD,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphDangerousTerminateThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus
    )
{
    struct
    {
        HANDLE ThreadHandle;
        NTSTATUS ExitStatus;
    } args;

    args.ThreadHandle = ThreadHandle;
    args.ExitStatus = ExitStatus;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_DANGEROUSTERMINATETHREAD,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphOpenType(
    __in HANDLE KphHandle,
    __out PHANDLE TypeHandle,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    struct
    {
        PHANDLE TypeHandle;
        POBJECT_ATTRIBUTES ObjectAttributes;
    } args;

    args.TypeHandle = TypeHandle;
    args.ObjectAttributes = ObjectAttributes;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_OPENTYPE,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphOpenDriver(
    __in HANDLE KphHandle,
    __out PHANDLE DriverHandle,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    struct
    {
        PHANDLE DriverHandle;
        POBJECT_ATTRIBUTES ObjectAttributes;
    } args;

    args.DriverHandle = DriverHandle;
    args.ObjectAttributes = ObjectAttributes;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_OPENDRIVER,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphQueryInformationDriver(
    __in HANDLE KphHandle,
    __in HANDLE DriverHandle,
    __in DRIVER_INFORMATION_CLASS DriverInformationClass,
    __in_opt PVOID DriverInformation,
    __in ULONG DriverInformationLength,
    __out_opt PULONG ReturnLength
    )
{
    struct
    {
        HANDLE DriverHandle;
        DRIVER_INFORMATION_CLASS DriverInformationClass;
        PVOID DriverInformation;
        ULONG DriverInformationLength;
        PULONG ReturnLength;
    } args;

    args.DriverHandle = DriverHandle;
    args.DriverInformationClass = DriverInformationClass;
    args.DriverInformation = DriverInformation;
    args.DriverInformationLength = DriverInformationLength;
    args.ReturnLength = ReturnLength;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_QUERYINFORMATIONDRIVER,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphOpenDirectoryObject(
    __in HANDLE KphHandle,
    __out PHANDLE DirectoryHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    struct
    {
        PHANDLE DirectoryHandle;
        ACCESS_MASK DesiredAccess;
        POBJECT_ATTRIBUTES ObjectAttributes;
    } args;

    args.DirectoryHandle = DirectoryHandle;
    args.DesiredAccess = DesiredAccess;
    args.ObjectAttributes = ObjectAttributes;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_OPENDIRECTORYOBJECT,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}

NTSTATUS KphOpenNamedObject(
    __in HANDLE KphHandle,
    __out PHANDLE Handle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    struct
    {
        PHANDLE Handle;
        ACCESS_MASK DesiredAccess;
        POBJECT_ATTRIBUTES ObjectAttributes;
    } args;

    args.Handle = Handle;
    args.DesiredAccess = DesiredAccess;
    args.ObjectAttributes = ObjectAttributes;

    return KphpDeviceIoControl(
        KphHandle,
        KPH_OPENNAMEDOBJECT,
        &args,
        sizeof(args),
        NULL,
        0,
        NULL
        );
}
