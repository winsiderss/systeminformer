/*
 * Process Hacker Driver - 
 *   main driver code
 * 
 * Copyright (C) 2009 wj32
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

#include "include/kph.h"
#include "include/kprocesshacker.h"

typedef struct _KPH_ZWQUERYOBJECT_BUFFER
{
    NTSTATUS Status;
    ULONG ReturnLength;
    PVOID BufferBase;
    UCHAR Buffer[1];
} KPH_ZWQUERYOBJECT_BUFFER, *PKPH_ZWQUERYOBJECT_BUFFER;

#define CHECK_IN_LENGTH \
    if (inLength < sizeof(*args)) \
    { \
        status = STATUS_BUFFER_TOO_SMALL; \
        goto IoControlEnd; \
    }
#define CHECK_OUT_LENGTH \
    if (outLength < sizeof(*ret)) \
    { \
        status = STATUS_BUFFER_TOO_SMALL; \
        goto IoControlEnd; \
    }
#define CHECK_IN_OUT_LENGTH \
    if (inLength < sizeof(*args) || outLength < sizeof(*ret)) \
    { \
        status = STATUS_BUFFER_TOO_SMALL; \
        goto IoControlEnd; \
    }

PDRIVER_OBJECT KphDriverObject;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DriverEntry)
#pragma alloc_text(PAGE, DriverUnload)
#pragma alloc_text(PAGE, KphDispatchCreate)
#pragma alloc_text(PAGE, KphDispatchClose)
#pragma alloc_text(PAGE, KphDispatchDeviceControl)
#pragma alloc_text(PAGE, KphDispatchRead)
#pragma alloc_text(PAGE, KphUnsupported)
#endif

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    NTSTATUS status = STATUS_SUCCESS;
    int i;
    PDEVICE_OBJECT deviceObject = NULL;
    UNICODE_STRING deviceName, dosDeviceName;
    
    KphDriverObject = DriverObject;
    
    /* Initialize version information. */
    status = KvInit();
    
    if (!NT_SUCCESS(status))
    {
        if (status == STATUS_NOT_SUPPORTED)
            dprintf("Your operating system is not supported by KProcessHacker\n");
        
        return status;
    }
    
    /* Initialize NT KPH. */
    status = KphNtInit();
    
    if (!NT_SUCCESS(status))
        return status;
    
    /* Initialize the KPH object manager. */
    status = KphRefInit();
    
    if (!NT_SUCCESS(status))
        return status;
    
    /* Initialize trace databases. */
    status = KphTraceDatabaseInitialization();
    
    if (!NT_SUCCESS(status))
    {
        KphRefDeinit();
        return status;
    }
    
    RtlInitUnicodeString(&deviceName, KPH_DEVICE_NAME);
    RtlInitUnicodeString(&dosDeviceName, KPH_DEVICE_DOS_NAME);
    
    /* Create the KProcessHacker device. */
    status = IoCreateDevice(DriverObject, 0, &deviceName, 
        FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &deviceObject);
    
    /* Set up the major functions. */
    for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
        DriverObject->MajorFunction[i] = NULL;
    
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = KphDispatchClose;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = KphDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_READ] = KphDispatchRead;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = KphDispatchDeviceControl;
    DriverObject->DriverUnload = DriverUnload;
    
    deviceObject->Flags |= DO_BUFFERED_IO;
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    
    IoCreateSymbolicLink(&dosDeviceName, &deviceName);
    
    dprintf("Driver loaded\n");
    
    return STATUS_SUCCESS;
}

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING dosDeviceName;
    
    RtlInitUnicodeString(&dosDeviceName, KPH_DEVICE_DOS_NAME);
    IoDeleteSymbolicLink(&dosDeviceName);
    IoDeleteDevice(DriverObject->DeviceObject);
    
    /* Free all objects in the object manager. */
    KphRefDeinit();
    
    dprintf("Driver unloaded\n");
}

NTSTATUS KphDispatchCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    NTSTATUS status = STATUS_SUCCESS;
    
#ifdef KPH_REQUIRE_DEBUG_PRIVILEGE
    if (!SeSinglePrivilegeCheck(SeExports->SeDebugPrivilege, UserMode))
    {
        dprintf("Client (PID %d) was refused\n", PsGetCurrentProcessId());
        Irp->IoStatus.Status = STATUS_PRIVILEGE_NOT_HELD;
        
        return STATUS_PRIVILEGE_NOT_HELD;
    }
#endif
    
    dprintf("Client (PID %d) connected\n", PsGetCurrentProcessId());
    
    return status;
}

NTSTATUS KphDispatchClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    NTSTATUS status = STATUS_SUCCESS;
    
    dprintf("Client (PID %d) disconnected\n", PsGetCurrentProcessId());
    
    return status;
}

PCHAR KphpGetControlCodeName(ULONG ControlCode)
{
    switch (ControlCode)
    {
        case KPH_GETFEATURES:
            return "KphGetFeatures";
        
        case KPH_OPENPROCESS:
            return "KphOpenProcess";
        case KPH_OPENPROCESSTOKEN:
            return "KphOpenProcessToken";
        case KPH_OPENPROCESSJOB:
            return "KphOpenProcessJob";
        case KPH_SUSPENDPROCESS:
            return "KphSuspendProcess";
        case KPH_RESUMEPROCESS:
            return "KphResumeProcess";
        case KPH_TERMINATEPROCESS:
            return "KphTerminateProcess";
        case KPH_READVIRTUALMEMORY:
            return "KphReadVirtualMemory";
        case KPH_WRITEVIRTUALMEMORY:
            return "KphWriteVirtualMemory";
        case KPH_UNSAFEREADVIRTUALMEMORY:
            return "KphUnsafeReadVirtualMemory";
        case KPH_GETPROCESSPROTECTED:
            return "KphGetProcessProtected";
        case KPH_SETPROCESSPROTECTED:
            return "KphSetProcessProtected";
        case KPH_SETEXECUTEOPTIONS:
            return "KphSetExecuteOptions";
        case KPH_SETPROCESSTOKEN:
            return "KphSetProcessToken";
        case KPH_QUERYINFORMATIONPROCESS:
            return "KphQueryInformationProcess";
        case KPH_QUERYINFORMATIONTHREAD:
            return "KphQueryInformationThread";
        case KPH_SETINFORMATIONPROCESS:
            return "KphSetInformationProcess";
        case KPH_SETINFORMATIONTHREAD:
            return "KphSetInformationThread";
        
        case KPH_OPENTHREAD:
            return "KphOpenThread";
        case KPH_OPENTHREADPROCESS:
            return "KphOpenThreadProcess";
        case KPH_TERMINATETHREAD:
            return "KphTerminateThread";
        case KPH_DANGEROUSTERMINATETHREAD:
            return "KphDangerousTerminateThread";
        case KPH_GETCONTEXTTHREAD:
            return "KphGetContextThread";
        case KPH_SETCONTEXTTHREAD:
            return "KphSetContextThread";
        case KPH_CAPTURESTACKBACKTRACETHREAD:
            return "KphCaptureStackBackTraceThread";
        case KPH_GETTHREADWIN32THREAD:
            return "KphGetThreadWin32Thread";
        case KPH_ASSIGNIMPERSONATIONTOKEN:
            return "KphAssignImpersonationToken";
        
        case KPH_QUERYPROCESSHANDLES:
            return "KphQueryProcessHandles";
        case KPH_GETHANDLEOBJECTNAME:
            return "KphGetHandleObjectName";
        case KPH_ZWQUERYOBJECT:
            return "KphZwQueryObject";
        case KPH_DUPLICATEOBJECT:
            return "KphDuplicateObject";
        case KPH_SETHANDLEATTRIBUTES:
            return "KphSetHandleAttributes";
        case KPH_SETHANDLEGRANTEDACCESS:
            return "KphSetHandleGrantedAccess";
        case KPH_GETPROCESSID:
            return "KphGetProcessId";
        case KPH_GETTHREADID:
            return "KphGetThreadId";
        
        case KPH_OPENNAMEDOBJECT:
            return "KphOpenNamedObject";
        case KPH_OPENDIRECTORYOBJECT:
            return "KphOpenDirectoryObject";
        case KPH_OPENDRIVER:
            return "KphOpenDriver";
        case KPH_QUERYINFORMATIONDRIVER:
            return "KphQueryInformationDriver";
        case KPH_OPENTYPE:
            return "KphOpenType";
        
        default:
            return "Unknown";
    }
}

NTSTATUS KphDispatchDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION ioStackIrp = NULL;
    PVOID dataBuffer;
    ULONG controlCode;
    ULONG inLength = 0;
    ULONG outLength = 0;
    ULONG retLength = 0;
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    
    ioStackIrp = IoGetCurrentIrpStackLocation(Irp);
    
    if (ioStackIrp == NULL)
    {
        status = STATUS_INTERNAL_ERROR;
        goto IoControlEnd;
    }
    
    dataBuffer = Irp->AssociatedIrp.SystemBuffer;
    
    if (dataBuffer == NULL && (inLength != 0 || outLength != 0))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        goto IoControlEnd;
    }
    
    inLength = ioStackIrp->Parameters.DeviceIoControl.InputBufferLength;
    outLength = ioStackIrp->Parameters.DeviceIoControl.OutputBufferLength;
    controlCode = ioStackIrp->Parameters.DeviceIoControl.IoControlCode;
    
    dprintf("IoControl 0x%08x (%s)\n", controlCode, KphpGetControlCodeName(controlCode));
    
    switch (controlCode)
    {
        /* Get Features
         * 
         * Gets the features supported by the driver.
         */
        case KPH_GETFEATURES:
        {
            struct
            {
                ULONG Features;
            } *ret = dataBuffer;
            ULONG features = 0;
            
            CHECK_OUT_LENGTH;
            
            if (__PsTerminateProcess)
                features |= KPHF_PSTERMINATEPROCESS;
            if (__PspTerminateThreadByPointer)
                features |= KPHF_PSPTERMINATETHREADBPYPOINTER;
            
            ret->Features = features;
            retLength = sizeof(*ret);
        }
        break;
        
        /* KphOpenProcess
         * 
         * Opens the specified process. This call will never fail unless:
         * 1. PsLookupProcessByProcessId, ObOpenObjectByPointer or some lower-level 
         *    function is hooked, or 
         * 2. The process is protected.
         */
        case KPH_OPENPROCESS:
        {
            struct
            {
                HANDLE ProcessId;
                ACCESS_MASK DesiredAccess;
            } *args = dataBuffer;
            struct
            {
                HANDLE ProcessHandle;
            } *ret = dataBuffer;
            OBJECT_ATTRIBUTES objectAttributes = { 0 };
            CLIENT_ID clientId;
            
            CHECK_IN_OUT_LENGTH;
            
            clientId.UniqueThread = 0;
            clientId.UniqueProcess = args->ProcessId;
            status = KphOpenProcess(
                &ret->ProcessHandle,
                args->DesiredAccess,
                &objectAttributes,
                &clientId,
                KernelMode
                );
            
            if (!NT_SUCCESS(status))
                goto IoControlEnd;
            
            retLength = sizeof(*ret);
        }
        break;
        
        /* KphOpenProcessToken
         * 
         * Opens the specified process' token. This call will never fail unless 
         * a low-level function is hooked.
         */
        case KPH_OPENPROCESSTOKEN:
        {
            struct
            {
                HANDLE ProcessHandle;
                ACCESS_MASK DesiredAccess;
            } *args = dataBuffer;
            struct
            {
                HANDLE TokenHandle;
            } *ret = dataBuffer;
            
            CHECK_IN_OUT_LENGTH;
            
            status = KphOpenProcessTokenEx(
                args->ProcessHandle,
                args->DesiredAccess,
                0,
                &ret->TokenHandle,
                KernelMode
                );
            
            if (!NT_SUCCESS(status))
                goto IoControlEnd;
            
            retLength = sizeof(*ret);
        }
        break;
        
        /* KphOpenProcessJob
         * 
         * Opens the job object that the process is assigned to. If the process is 
         * not assigned to any job object, the call will fail with STATUS_PROCESS_NOT_IN_JOB.
         */
        case KPH_OPENPROCESSJOB:
        {
            struct
            {
                HANDLE ProcessHandle;
                ACCESS_MASK DesiredAccess;
            } *args = dataBuffer;
            struct
            {
                HANDLE JobHandle;
            } *ret = dataBuffer;
            
            CHECK_IN_OUT_LENGTH;
            
            status = KphOpenProcessJob(args->ProcessHandle, args->DesiredAccess, &ret->JobHandle, KernelMode);
            
            if (!NT_SUCCESS(status))
                goto IoControlEnd;
            
            retLength = sizeof(*ret);
        }
        break;
        
        /* KphSuspendProcess
         * 
         * Suspends the specified process. This call will fail on Windows XP 
         * and below.
         */
        case KPH_SUSPENDPROCESS:
        {
            struct
            {
                HANDLE ProcessHandle;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            status = KphSuspendProcess(args->ProcessHandle);
        }
        break;
        
        /* KphResumeProcess
         * 
         * Resumes the specified process. This call will fail on Windows XP 
         * and below.
         */
        case KPH_RESUMEPROCESS:
        {
            struct
            {
                HANDLE ProcessHandle;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            status = KphResumeProcess(args->ProcessHandle);
        }
        break;
        
        /* KphTerminateProcess
         * 
         * Terminates the specified process. This call will never fail unless
         * PsTerminateProcess could not be located and Zw/NtTerminateProcess 
         * is hooked, or an attempt was made to terminate the current process. 
         * In that case, the call will fail with STATUS_CANT_TERMINATE_SELF.
         */
        case KPH_TERMINATEPROCESS:
        {
            struct
            {
                HANDLE ProcessHandle;
                NTSTATUS ExitStatus;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            status = KphTerminateProcess(args->ProcessHandle, args->ExitStatus);
        }
        break;
        
        /* KphReadVirtualMemory
         * 
         * Reads process memory.
         */
        case KPH_READVIRTUALMEMORY:
        {
            struct
            {
                HANDLE ProcessHandle;
                PVOID BaseAddress;
                PVOID Buffer;
                ULONG BufferLength;
                PULONG ReturnLength;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            status = KphReadVirtualMemory(
                args->ProcessHandle,
                args->BaseAddress,
                args->Buffer,
                args->BufferLength,
                args->ReturnLength,
                UserMode
                );
        }
        break;
        
        /* KphWriteVirtualMemory
         * 
         * Writes to process memory.
         */
        case KPH_WRITEVIRTUALMEMORY:
        {
            struct
            {
                HANDLE ProcessHandle;
                PVOID BaseAddress;
                PVOID Buffer;
                ULONG BufferLength;
                PULONG ReturnLength;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            status = KphWriteVirtualMemory(
                args->ProcessHandle,
                args->BaseAddress,
                args->Buffer,
                args->BufferLength,
                args->ReturnLength,
                UserMode
                );
        }
        break;
        
        /* KphUnsafeReadVirtualMemory
         * 
         * Reads process memory or kernel memory.
         */
        case KPH_UNSAFEREADVIRTUALMEMORY:
        {
            struct
            {
                HANDLE ProcessHandle;
                PVOID BaseAddress;
                PVOID Buffer;
                ULONG BufferLength;
                PULONG ReturnLength;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            status = KphUnsafeReadVirtualMemory(
                args->ProcessHandle,
                args->BaseAddress,
                args->Buffer,
                args->BufferLength,
                args->ReturnLength,
                UserMode
                );
        }
        break;
        
        /* Get Process Protected
         * 
         * Gets whether the process is protected.
         */
        case KPH_GETPROCESSPROTECTED:
        {
            struct
            {
                HANDLE ProcessId;
            } *args = dataBuffer;
            struct
            {
                BOOLEAN IsProtected;
            } *ret = dataBuffer;
            PEPROCESS processObject;
            
            CHECK_IN_OUT_LENGTH;
            
            status = PsLookupProcessByProcessId(args->ProcessId, &processObject);
            
            if (!NT_SUCCESS(status))
                goto IoControlEnd;
            
            ret->IsProtected = 
                (CHAR)GET_BIT(
                    *(PULONG)KVOFF(processObject, OffEpProtectedProcessOff),
                    OffEpProtectedProcessBit
                    );
            ObDereferenceObject(processObject);
            retLength = sizeof(*ret);
        }
        break;
        
        /* Set Process Protected
         * 
         * Sets whether the process is protected.
         */
        case KPH_SETPROCESSPROTECTED:
        {
            struct
            {
                HANDLE ProcessId;
                BOOLEAN IsProtected;
            } *args = dataBuffer;
            PEPROCESS processObject;
            
            CHECK_IN_LENGTH;
            
            status = PsLookupProcessByProcessId(args->ProcessId, &processObject);
            
            if (!NT_SUCCESS(status))
                goto IoControlEnd;
            
            if (args->IsProtected)
            {
                SET_BIT(
                    *(PULONG)KVOFF(processObject, OffEpProtectedProcessOff),
                    OffEpProtectedProcessBit
                    );
            }
            else
            {
                CLEAR_BIT(
                    *(PULONG)KVOFF(processObject, OffEpProtectedProcessOff),
                    OffEpProtectedProcessBit
                    );
            }
            
            ObDereferenceObject(processObject);
        }
        break;
        
        /* Set Execute Options
         * 
         * Sets NX status for a process.
         */
        case KPH_SETEXECUTEOPTIONS:
        {
            struct
            {
                HANDLE ProcessHandle;
                ULONG ExecuteOptions;
            } *args = dataBuffer;
            KPH_ATTACH_STATE attachState;
            
            CHECK_IN_LENGTH;
            
            status = KphAttachProcessHandle(args->ProcessHandle, &attachState);
            
            if (!NT_SUCCESS(status))
                goto IoControlEnd;
            
            status = ZwSetInformationProcess(
                NtCurrentProcess(),
                ProcessExecuteFlags,
                &args->ExecuteOptions,
                sizeof(ULONG)
                );
            KphDetachProcess(&attachState);
        }
        break;
        
        /* Set Process Token
         * 
         * Assigns the primary token of a source process to a target process.
         */
        case KPH_SETPROCESSTOKEN:
        {
            struct
            {
                HANDLE SourceProcessId;
                HANDLE TargetProcessId;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            status = SetProcessToken(args->SourceProcessId, args->TargetProcessId);
        }
        break;
        
        case KPH_QUERYINFORMATIONPROCESS:
        {
            struct
            {
                HANDLE ProcessHandle;
                PROCESSINFOCLASS ProcessInformationClass;
                PVOID ProcessInformation;
                ULONG ProcessInformationLength;
                PULONG ReturnLength;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            if (
                args->ProcessInformationClass != ProcessIoPriority
                )
            {
                status = STATUS_INVALID_PARAMETER;
                goto IoControlEnd;
            }
            
            __try
            {
                ProbeForWrite(args->ProcessInformation, args->ProcessInformationLength, 1);
                
                if (args->ReturnLength)
                    ProbeForWrite(args->ReturnLength, sizeof(ULONG), 1);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
                goto IoControlEnd;
            }
            
            /* Very unsafe and implementation dependent, but it should work. */
            __try
            {
                status = ZwQueryInformationProcess(
                    args->ProcessHandle,
                    args->ProcessInformationClass,
                    args->ProcessInformation,
                    args->ProcessInformationLength,
                    args->ReturnLength
                    );
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
            }
        }
        break;
        
        case KPH_QUERYINFORMATIONTHREAD:
        {
            struct
            {
                HANDLE ThreadHandle;
                THREADINFOCLASS ThreadInformationClass;
                PVOID ThreadInformation;
                ULONG ThreadInformationLength;
                PULONG ReturnLength;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            if (
                args->ThreadInformationClass != ThreadIoPriority
                )
            {
                status = STATUS_INVALID_PARAMETER;
                goto IoControlEnd;
            }
            
            __try
            {
                ProbeForWrite(args->ThreadInformation, args->ThreadInformationLength, 1);
                
                if (args->ReturnLength)
                    ProbeForWrite(args->ReturnLength, sizeof(ULONG), 1);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
                goto IoControlEnd;
            }
            
            __try
            {
                status = ZwQueryInformationThread(
                    args->ThreadHandle,
                    args->ThreadInformationClass,
                    args->ThreadInformation,
                    args->ThreadInformationLength,
                    args->ReturnLength
                    );
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
            }
        }
        break;
        
        case KPH_SETINFORMATIONPROCESS:
        {
            struct
            {
                HANDLE ProcessHandle;
                PROCESSINFOCLASS ProcessInformationClass;
                PVOID ProcessInformation;
                ULONG ProcessInformationLength;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            if (
                args->ProcessInformationClass != ProcessIoPriority
                )
            {
                status = STATUS_INVALID_PARAMETER;
                goto IoControlEnd;
            }
            
            __try
            {
                ProbeForRead(args->ProcessInformation, args->ProcessInformationLength, 1);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
                goto IoControlEnd;
            }
            
            __try
            {
                status = ZwSetInformationProcess(
                    args->ProcessHandle,
                    args->ProcessInformationClass,
                    args->ProcessInformation,
                    args->ProcessInformationLength
                    );
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
            }
        }
        break;
        
        case KPH_SETINFORMATIONTHREAD:
        {
            struct
            {
                HANDLE ThreadHandle;
                THREADINFOCLASS ThreadInformationClass;
                PVOID ThreadInformation;
                ULONG ThreadInformationLength;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            if (
                args->ThreadInformationClass != ThreadIoPriority
                )
            {
                status = STATUS_INVALID_PARAMETER;
                goto IoControlEnd;
            }
            
            __try
            {
                ProbeForRead(args->ThreadInformation, args->ThreadInformationLength, 1);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
                goto IoControlEnd;
            }
            
            __try
            {
                status = ZwSetInformationThread(
                    args->ThreadHandle,
                    args->ThreadInformationClass,
                    args->ThreadInformation,
                    args->ThreadInformationLength
                    );
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
            }
        }
        break;
        
        /* KphOpenThread
         * 
         * Opens the specified thread. This call will never fail unless:
         * 1. PsLookupProcessThreadByCid, ObOpenObjectByPointer or some lower-level 
         *    function is hooked, or 
         * 2. The thread's process is protected.
         */
        case KPH_OPENTHREAD:
        {
            struct
            {
                HANDLE ThreadId;
                ACCESS_MASK DesiredAccess;
            } *args = dataBuffer;
            struct
            {
                HANDLE ThreadHandle;
            } *ret = dataBuffer;
            OBJECT_ATTRIBUTES objectAttributes = { 0 };
            CLIENT_ID clientId;
            
            CHECK_IN_OUT_LENGTH;
            
            clientId.UniqueThread = args->ThreadId;
            clientId.UniqueProcess = 0;
            status = KphOpenThread(
                &ret->ThreadHandle,
                args->DesiredAccess,
                &objectAttributes,
                &clientId,
                KernelMode
                );
            
            if (!NT_SUCCESS(status))
                goto IoControlEnd;
            
            retLength = sizeof(*ret);
        }
        break;
        
        /* KphOpenThreadProcess
         * 
         * Opens the process associated with the specified thread.
         */
        case KPH_OPENTHREADPROCESS:
        {
            struct
            {
                HANDLE ThreadHandle;
                ACCESS_MASK DesiredAccess;
            } *args = dataBuffer;
            struct
            {
                HANDLE ProcessHandle;
            } *ret = dataBuffer;
            
            CHECK_IN_OUT_LENGTH;
            
            status = KphOpenThreadProcess(
                args->ThreadHandle,
                args->DesiredAccess,
                &ret->ProcessHandle,
                KernelMode
                );
            
            if (!NT_SUCCESS(status))
                goto IoControlEnd;
            
            retLength = sizeof(*ret);
        }
        break;
        
        /* KphTerminateThread
         * 
         * Terminates the specified thread. This call will fail if 
         * PspTerminateThreadByPointer could not be located or if an attempt 
         * was made to terminate the current thread. In that case, the call 
         * will return STATUS_CANT_TERMINATE_SELF.
         */
        case KPH_TERMINATETHREAD:
        {
            struct
            {
                HANDLE ThreadHandle;
                NTSTATUS ExitStatus;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            status = KphTerminateThread(args->ThreadHandle, args->ExitStatus);
        }
        break;
        
        /* KphDangerousTerminateThread
         * 
         * Terminates the specified thread. This operation may cause a bugcheck.
         */
        case KPH_DANGEROUSTERMINATETHREAD:
        {
            struct
            {
                HANDLE ThreadHandle;
                NTSTATUS ExitStatus;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            status = KphDangerousTerminateThread(args->ThreadHandle, args->ExitStatus);
        }
        break;
        
        /* KphGetContextThread
         * 
         * Gets the context of the specified thread.
         */
        case KPH_GETCONTEXTTHREAD:
        {
            struct
            {
                HANDLE ThreadHandle;
                PCONTEXT ThreadContext;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            status = KphGetContextThread(args->ThreadHandle, args->ThreadContext, UserMode);
        }
        break;
        
        /* KphSetContextThread
         * 
         * Sets the context of the specified thread.
         */
        case KPH_SETCONTEXTTHREAD:
        {
            struct
            {
                HANDLE ThreadHandle;
                PCONTEXT ThreadContext;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            status = KphSetContextThread(args->ThreadHandle, args->ThreadContext, UserMode);
        }
        break;
        
        /* KphCaptureStackBackTraceThread
         * 
         * Captures a kernel stack trace for the specified thread.
         */
        case KPH_CAPTURESTACKBACKTRACETHREAD:
        {
            struct
            {
                HANDLE ThreadHandle;
                ULONG FramesToSkip;
                ULONG FramesToCapture;
                PVOID *BackTrace;
                PULONG CapturedFrames;
                PULONG BackTraceHash;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            status = KphCaptureStackBackTraceThread(
                args->ThreadHandle,
                args->FramesToSkip,
                args->FramesToCapture,
                args->BackTrace,
                args->CapturedFrames,
                args->BackTraceHash,
                UserMode
                );
        }
        break;
        
        /* KphGetThreadWin32Thread
         * 
         * Gets a pointer to the specified thread's Win32Thread structure.
         */
        case KPH_GETTHREADWIN32THREAD:
        {
            struct
            {
                HANDLE ThreadHandle;
            } *args = dataBuffer;
            struct
            {
                PVOID Win32Thread;
            } *ret = dataBuffer;
            
            CHECK_IN_OUT_LENGTH;
            
            status = KphGetThreadWin32Thread(args->ThreadHandle, &ret->Win32Thread, KernelMode);
            
            if (!NT_SUCCESS(status))
                goto IoControlEnd;
            
            retLength = sizeof(*ret);
        }
        break;
        
        /* KphAssignImpersonationToken
         * 
         * Assigns an impersonation token to a thread.
         */
        case KPH_ASSIGNIMPERSONATIONTOKEN:
        {
            struct
            {
                HANDLE ThreadHandle;
                HANDLE TokenHandle;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            status = KphAssignImpersonationToken(args->ThreadHandle, args->TokenHandle);
        }
        break;
        
        /* KphQueryProcessHandles
         * 
         * Gets the handles in a process handle table.
         */
        case KPH_QUERYPROCESSHANDLES:
        {
            struct
            {
                HANDLE ProcessHandle;
                PVOID Buffer;
                ULONG BufferLength;
                PULONG ReturnLength;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            status = KphQueryProcessHandles(
                args->ProcessHandle,
                (PPROCESS_HANDLE_INFORMATION)args->Buffer,
                args->BufferLength,
                args->ReturnLength,
                UserMode
                );
        }
        break;
        
        /* Get Handle Object Name
         * 
         * Gets the name of the specified handle. The handle can be remote; in 
         * that case a valid process handle must be passed. Otherwise, set the 
         * process handle to -1 (NtCurrentProcess()).
         */
        case KPH_GETHANDLEOBJECTNAME:
        {
            struct
            {
                HANDLE ProcessHandle;
                HANDLE Handle;
            } *args = dataBuffer;
            KPH_ATTACH_STATE attachState;
            PVOID object;
            
            CHECK_IN_LENGTH;
            
            status = KphAttachProcessHandle(args->ProcessHandle, &attachState);
            
            if (!NT_SUCCESS(status))
                goto IoControlEnd;
            
            /* See the block for KPH_ZWQUERYOBJECT for information. */
            if (attachState.Process == PsInitialSystemProcess)
                MakeKernelHandle(args->Handle);
            
            status = ObReferenceObjectByHandle(args->Handle, 0, NULL, KernelMode, &object, NULL);
            KphDetachProcess(&attachState);
            
            if (!NT_SUCCESS(status))
                goto IoControlEnd;
            
            status = KphQueryNameObject(object, (PUNICODE_STRING)dataBuffer, outLength, &retLength);
            ObDereferenceObject(object);
            
            /* Check if the return length is greater than the length of the user buffer. 
             * If so, it means the user needs to provide a larger buffer. In that case, 
             * store the length in the Unicode string structure.
             */
            if (retLength > outLength)
            {
                if (outLength >= sizeof(UNICODE_STRING))
                {
                    ((PUNICODE_STRING)dataBuffer)->Length = (USHORT)retLength;
                    retLength = sizeof(UNICODE_STRING);
                }
            }
        }
        break;
        
        /* ZwQueryObject
         * 
         * Performs ZwQueryObject in the context of another process.
         */
        case KPH_ZWQUERYOBJECT:
        {
            struct
            {
                HANDLE ProcessHandle;
                HANDLE Handle;
                OBJECT_INFORMATION_CLASS ObjectInformationClass;
            } *args = dataBuffer;
            PKPH_ZWQUERYOBJECT_BUFFER ret = dataBuffer;
            NTSTATUS status2 = STATUS_SUCCESS;
            KPH_ATTACH_STATE attachState;
            BOOLEAN attached;
            
            if (inLength < sizeof(*args) || outLength < FIELD_OFFSET(KPH_ZWQUERYOBJECT_BUFFER, Buffer))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                goto IoControlEnd;
            }
            
            status = KphAttachProcessHandle(args->ProcessHandle, &attachState);
            
            if (!NT_SUCCESS(status))
                goto IoControlEnd;
            
            /* Are we attached to the system process? If we are, 
             * we must set the high bit in the handle to indicate 
             * that it is a kernel handle - a new check for this 
             * was added in Windows 7.
             */
            if (attachState.Process == PsInitialSystemProcess)
                MakeKernelHandle(args->Handle);
            
            status2 = ZwQueryObject(
                args->Handle,
                args->ObjectInformationClass,
                ret->Buffer,
                outLength - FIELD_OFFSET(KPH_ZWQUERYOBJECT_BUFFER, Buffer),
                &retLength
                );
            KphDetachProcess(&attachState);
            
            ret->ReturnLength = retLength;
            ret->BufferBase = ret->Buffer;
            
            if (NT_SUCCESS(status2))
                retLength += FIELD_OFFSET(KPH_ZWQUERYOBJECT_BUFFER, Buffer);
            else
                retLength = FIELD_OFFSET(KPH_ZWQUERYOBJECT_BUFFER, Buffer);
            
            ret->Status = status2;
        }
        break;
        
        /* KphDuplicateObject
         * 
         * Duplicates the specified handle from the source process to the target process. 
         * Do not use this call to duplicate file handles; it may freeze indefinitely if 
         * the file is a named pipe.
         */
        case KPH_DUPLICATEOBJECT:
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
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            status = KphDuplicateObject(
                args->SourceProcessHandle,
                args->SourceHandle,
                args->TargetProcessHandle,
                args->TargetHandle,
                args->DesiredAccess,
                args->HandleAttributes,
                args->Options,
                UserMode
                );
        }
        break;
        
        /* Set Handle Attributes
         * 
         * Sets handle flags in the specified process.
         */
        case KPH_SETHANDLEATTRIBUTES:
        {
            struct
            {
                HANDLE ProcessHandle;
                HANDLE Handle;
                ULONG Flags;
            } *args = dataBuffer;
            KPH_ATTACH_STATE attachState;
            OBJECT_HANDLE_FLAG_INFORMATION handleFlags = { 0 };
            
            CHECK_IN_LENGTH;
            
            status = KphAttachProcessHandle(args->ProcessHandle, &attachState);
            
            if (!NT_SUCCESS(status))
                goto IoControlEnd;
            
            if (args->Flags & OBJ_PROTECT_CLOSE)
                handleFlags.ProtectFromClose = TRUE;
            if (args->Flags & OBJ_INHERIT)
                handleFlags.Inherit = TRUE;
            
            status = ObSetHandleAttributes(args->Handle, &handleFlags, UserMode);
            KphDetachProcess(&attachState);
        }
        break;
        
        /* KphSetHandleGrantedAccess
         * 
         * Sets the granted access for a handle.
         */
        case KPH_SETHANDLEGRANTEDACCESS:
        {
            struct
            {
                HANDLE Handle;
                ACCESS_MASK GrantedAccess;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            status = KphSetHandleGrantedAccess(
                PsGetCurrentProcess(),
                args->Handle,
                args->GrantedAccess
                );
        }
        break;
        
        /* KphGetProcessId
         * 
         * Gets the process ID of a process handle in the context of another process.
         */
        case KPH_GETPROCESSID:
        {
            struct
            {
                HANDLE ProcessHandle;
                HANDLE Handle;
            } *args = dataBuffer;
            struct
            {
                HANDLE ProcessId;
            } *ret = dataBuffer;
            KPH_ATTACH_STATE attachState;
            
            CHECK_IN_OUT_LENGTH;
            
            status = KphAttachProcessHandle(args->ProcessHandle, &attachState);
            
            if (!NT_SUCCESS(status))
                goto IoControlEnd;
            
            if (attachState.Process == PsInitialSystemProcess)
                MakeKernelHandle(args->Handle);
            
            ret->ProcessId = KphGetProcessId(args->Handle);
            KphDetachProcess(&attachState);
            retLength = sizeof(*ret);
        }
        break;
        
        /* KphGetThreadId
         * 
         * Gets the thread ID of a thread handle in the context of another process.
         */
        case KPH_GETTHREADID:
        {
            struct
            {
                HANDLE ProcessHandle;
                HANDLE Handle;
            } *args = dataBuffer;
            struct
            {
                HANDLE ThreadId;
                HANDLE ProcessId;
            } *ret = dataBuffer;
            KPH_ATTACH_STATE attachState;
            
            CHECK_IN_OUT_LENGTH;
            
            status = KphAttachProcessHandle(args->ProcessHandle, &attachState);
            
            if (!NT_SUCCESS(status))
                goto IoControlEnd;
            
            if (attachState.Process == PsInitialSystemProcess)
                MakeKernelHandle(args->Handle);
            
            ret->ThreadId = KphGetThreadId(args->Handle, &ret->ProcessId);
            KphDetachProcess(&attachState);
            retLength = sizeof(*ret);
        }
        break;
        
        /* KphOpenNamedObject
         * 
         * Opens a named object of any type.
         */
        case KPH_OPENNAMEDOBJECT:
        {
            struct
            {
                PHANDLE Handle;
                ACCESS_MASK DesiredAccess;
                POBJECT_ATTRIBUTES ObjectAttributes;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            status = KphOpenNamedObject(
                args->Handle,
                args->DesiredAccess,
                args->ObjectAttributes,
                NULL,
                UserMode
                );
        }
        break;
        
        /* KphOpenDirectoryObject
         * 
         * Opens a directory object.
         */
        case KPH_OPENDIRECTORYOBJECT:
        {
            struct
            {
                PHANDLE DirectoryObjectHandle;
                ACCESS_MASK DesiredAccess;
                POBJECT_ATTRIBUTES ObjectAttributes;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            status = KphOpenDirectoryObject(
                args->DirectoryObjectHandle,
                args->DesiredAccess,
                args->ObjectAttributes,
                UserMode
                );
        }
        break;
        
        /* KphOpenDriver
         * 
         * Opens a driver object.
         */
        case KPH_OPENDRIVER:
        {
            struct
            {
                PHANDLE DriverHandle;
                POBJECT_ATTRIBUTES ObjectAttributes;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            status = KphOpenDriver(args->DriverHandle, args->ObjectAttributes, UserMode);
        }
        break;
        
        /* KphQueryInformationDriver
         * 
         * Queries information about a driver object.
         */
        case KPH_QUERYINFORMATIONDRIVER:
        {
            struct
            {
                HANDLE DriverHandle;
                DRIVER_INFORMATION_CLASS DriverInformationClass;
                PVOID DriverInformation;
                ULONG DriverInformationLength;
                PULONG ReturnLength;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            status = KphQueryInformationDriver(
                args->DriverHandle,
                args->DriverInformationClass,
                args->DriverInformation,
                args->DriverInformationLength,
                args->ReturnLength,
                UserMode
                );
        }
        break;
        
        /* KphOpenType
         * 
         * Opens a type object.
         */
        case KPH_OPENTYPE:
        {
            struct
            {
                PHANDLE TypeHandle;
                POBJECT_ATTRIBUTES ObjectAttributes;
            } *args = dataBuffer;
            
            CHECK_IN_LENGTH;
            
            status = KphOpenType(args->TypeHandle, args->ObjectAttributes, UserMode);
        }
        break;
        
        default:
        {
            dprintf("Unrecognized IOCTL code 0x%08x\n", controlCode);
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
        break;
    }
    
IoControlEnd:
    Irp->IoStatus.Information = retLength;
    Irp->IoStatus.Status = status;
    dprintf("IOCTL 0x%08x result was 0x%08x\n", controlCode, status);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return status;
}

NTSTATUS KphDispatchRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION ioStackIrp = NULL;
    ULONG retLength = 0;
    
    ioStackIrp = IoGetCurrentIrpStackLocation(Irp);
    
    if (ioStackIrp != NULL)
    {
        PCHAR readDataBuffer = (PCHAR)Irp->AssociatedIrp.SystemBuffer;
        ULONG readLength = ioStackIrp->Parameters.Read.Length;
        
        if (readDataBuffer != NULL)
        {
            dprintf("Client read %d bytes!\n", readLength);
            
            if (readLength == 4)
            {
                *(ULONG *)readDataBuffer = KPH_CTL_CODE(0);
                retLength = 4;
            }
            else
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
            }
        }
    }
    
    Irp->IoStatus.Information = retLength;
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return status;
}

NTSTATUS KphUnsupported(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    dfprintf("Unsupported function called.\n");
    
    return STATUS_NOT_SUPPORTED;
}
