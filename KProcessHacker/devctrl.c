/*
 * KProcessHacker
 * 
 * Copyright (C) 2010 wj32
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

NTSTATUS KphDispatchDeviceControl(
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP Irp
    )
{
    NTSTATUS status;
    PIO_STACK_LOCATION stackLocation;
    PVOID originalInput;
    ULONG inputLength;
    ULONG ioControlCode;
    KPROCESSOR_MODE accessMode;
    UCHAR capturedInput[16 * sizeof(ULONG_PTR)];
    PVOID capturedInputPointer;

#define VERIFY_INPUT_LENGTH \
    do { \
        /* Ensure at compile time that our local buffer fits this particular call. */ \
        C_ASSERT(sizeof(*input) <= sizeof(capturedInput)); \
        \
        if (inputLength != sizeof(*input)) \
        { \
            status = STATUS_INFO_LENGTH_MISMATCH; \
            goto ControlEnd; \
        } \
    } while (0)

    stackLocation = IoGetCurrentIrpStackLocation(Irp);
    originalInput = stackLocation->Parameters.DeviceIoControl.Type3InputBuffer;
    inputLength = stackLocation->Parameters.DeviceIoControl.InputBufferLength;
    ioControlCode = stackLocation->Parameters.DeviceIoControl.IoControlCode;
    accessMode = Irp->RequestorMode;

    // Make sure we actually have input if the input length 
    // is non-zero.
    if (inputLength != 0 && !originalInput)
    {
        status = STATUS_INVALID_BUFFER_SIZE;
        goto ControlEnd;
    }

    // Make sure the caller isn't giving us a huge buffer.
    // If they are, it can't be correct because we have a 
    // compile-time check that makes sure our buffer can 
    // store the arguments for all the calls.
    if (inputLength > sizeof(capturedInput))
    {
        status = STATUS_INVALID_BUFFER_SIZE;
        goto ControlEnd;
    }

    // Probe and capture the input buffer.
    if (accessMode != KernelMode)
    {
        __try
        {
            ProbeForRead(originalInput, inputLength, sizeof(UCHAR));
            memcpy(capturedInput, originalInput, inputLength);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto ControlEnd;
        }
    }
    else
    {
        memcpy(capturedInput, originalInput, inputLength);
    }

    capturedInputPointer = capturedInput; // avoid casting below

    switch (ioControlCode)
    {
    case KPH_GETFEATURES:
        {
            struct
            {
                PULONG Features;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiGetFeatures(
                input->Features,
                accessMode
                );
        }
        break;
    case KPH_OPENPROCESS:
        {
            struct
            {
                PHANDLE ProcessHandle;
                ACCESS_MASK DesiredAccess;
                PCLIENT_ID ClientId;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiOpenProcess(
                input->ProcessHandle,
                input->DesiredAccess,
                input->ClientId,
                accessMode
                );
        }
        break;
    case KPH_OPENPROCESSTOKEN:
        {
            struct
            {
                HANDLE ProcessHandle;
                ACCESS_MASK DesiredAccess;
                PHANDLE TokenHandle;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiOpenProcessToken(
                input->ProcessHandle,
                input->DesiredAccess,
                input->TokenHandle,
                accessMode
                );
        }
        break;
    case KPH_OPENPROCESSJOB:
        {
            struct
            {
                HANDLE ProcessHandle;
                ACCESS_MASK DesiredAccess;
                PHANDLE JobHandle;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiOpenProcessJob(
                input->ProcessHandle,
                input->DesiredAccess,
                input->JobHandle,
                accessMode
                );
        }
        break;
    case KPH_SUSPENDPROCESS:
        {
            struct
            {
                HANDLE ProcessHandle;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiSuspendProcess(
                input->ProcessHandle,
                accessMode
                );
        }
        break;
    case KPH_RESUMEPROCESS:
        {
            struct
            {
                HANDLE ProcessHandle;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiResumeProcess(
                input->ProcessHandle,
                accessMode
                );
        }
        break;
    case KPH_TERMINATEPROCESS:
        {
            struct
            {
                HANDLE ProcessHandle;
                NTSTATUS ExitStatus;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiTerminateProcess(
                input->ProcessHandle,
                input->ExitStatus,
                accessMode
                );
        }
        break;
    case KPH_READVIRTUALMEMORY:
        {
            struct
            {
                HANDLE ProcessHandle;
                PVOID BaseAddress;
                PVOID Buffer;
                SIZE_T BufferSize;
                PSIZE_T NumberOfBytesRead;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiReadVirtualMemory(
                input->ProcessHandle,
                input->BaseAddress,
                input->Buffer,
                input->BufferSize,
                input->NumberOfBytesRead,
                accessMode
                );
        }
        break;
    case KPH_WRITEVIRTUALMEMORY:
        {
            struct
            {
                HANDLE ProcessHandle;
                PVOID BaseAddress;
                PVOID Buffer;
                SIZE_T BufferSize;
                PSIZE_T NumberOfBytesRead;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiWriteVirtualMemory(
                input->ProcessHandle,
                input->BaseAddress,
                input->Buffer,
                input->BufferSize,
                input->NumberOfBytesRead,
                accessMode
                );
        }
        break;
    case KPH_READVIRTUALMEMORYUNSAFE:
        {
            struct
            {
                HANDLE ProcessHandle;
                PVOID BaseAddress;
                PVOID Buffer;
                SIZE_T BufferSize;
                PSIZE_T NumberOfBytesRead;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiReadVirtualMemoryUnsafe(
                input->ProcessHandle,
                input->BaseAddress,
                input->Buffer,
                input->BufferSize,
                input->NumberOfBytesRead,
                accessMode
                );
        }
        break;
    case KPH_QUERYINFORMATIONPROCESS:
        {
            struct
            {
                HANDLE ProcessHandle;
                KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass;
                PVOID ProcessInformation;
                ULONG ProcessInformationLength;
                PULONG ReturnLength;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiQueryInformationProcess(
                input->ProcessHandle,
                input->ProcessInformationClass,
                input->ProcessInformation,
                input->ProcessInformationLength,
                input->ReturnLength,
                accessMode
                );
        }
        break;
    case KPH_SETINFORMATIONPROCESS:
        {
            struct
            {
                HANDLE ProcessHandle;
                KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass;
                PVOID ProcessInformation;
                ULONG ProcessInformationLength;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiSetInformationProcess(
                input->ProcessHandle,
                input->ProcessInformationClass,
                input->ProcessInformation,
                input->ProcessInformationLength,
                accessMode
                );
        }
        break;
    case KPH_OPENTHREAD:
        {
            struct
            {
                PHANDLE ThreadHandle;
                ACCESS_MASK DesiredAccess;
                PCLIENT_ID ClientId;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiOpenThread(
                input->ThreadHandle,
                input->DesiredAccess,
                input->ClientId,
                accessMode
                );
        }
        break;
    case KPH_OPENTHREADPROCESS:
        {
            struct
            {
                HANDLE ThreadHandle;
                ACCESS_MASK DesiredAccess;
                PHANDLE ProcessHandle;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiOpenThreadProcess(
                input->ThreadHandle,
                input->DesiredAccess,
                input->ProcessHandle,
                accessMode
                );
        }
        break;
    case KPH_TERMINATETHREAD:
        {
            struct
            {
                HANDLE ThreadHandle;
                NTSTATUS ExitStatus;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiTerminateThread(
                input->ThreadHandle,
                input->ExitStatus,
                accessMode
                );
        }
        break;
    case KPH_TERMINATETHREADUNSAFE:
        {
            struct
            {
                HANDLE ThreadHandle;
                NTSTATUS ExitStatus;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiTerminateThreadUnsafe(
                input->ThreadHandle,
                input->ExitStatus,
                accessMode
                );
        }
        break;
    case KPH_GETCONTEXTTHREAD:
        {
            struct
            {
                HANDLE ThreadHandle;
                PCONTEXT ThreadContext;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiGetContextThread(
                input->ThreadHandle,
                input->ThreadContext,
                accessMode
                );
        }
        break;
    case KPH_SETCONTEXTTHREAD:
        {
            struct
            {
                HANDLE ThreadHandle;
                PCONTEXT ThreadContext;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiSetContextThread(
                input->ThreadHandle,
                input->ThreadContext,
                accessMode
                );
        }
        break;
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
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiCaptureStackBackTraceThread(
                input->ThreadHandle,
                input->FramesToSkip,
                input->FramesToCapture,
                input->BackTrace,
                input->CapturedFrames,
                input->BackTraceHash,
                accessMode
                );
        }
        break;
    case KPH_QUERYINFORMATIONTHREAD:
        {
            struct
            {
                HANDLE ThreadHandle;
                KPH_THREAD_INFORMATION_CLASS ThreadInformationClass;
                PVOID ThreadInformation;
                ULONG ThreadInformationLength;
                PULONG ReturnLength;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiQueryInformationThread(
                input->ThreadHandle,
                input->ThreadInformationClass,
                input->ThreadInformation,
                input->ThreadInformationLength,
                input->ReturnLength,
                accessMode
                );
        }
        break;
    case KPH_SETINFORMATIONTHREAD:
        {
            struct
            {
                HANDLE ThreadHandle;
                KPH_THREAD_INFORMATION_CLASS ThreadInformationClass;
                PVOID ThreadInformation;
                ULONG ThreadInformationLength;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiSetInformationThread(
                input->ThreadHandle,
                input->ThreadInformationClass,
                input->ThreadInformation,
                input->ThreadInformationLength,
                accessMode
                );
        }
        break;
    case KPH_ENUMERATEPROCESSHANDLES:
        {
            struct
            {
                HANDLE ProcessHandle;
                PVOID Buffer;
                ULONG BufferLength;
                PULONG ReturnLength;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiEnumerateProcessHandles(
                input->ProcessHandle,
                input->Buffer,
                input->BufferLength,
                input->ReturnLength,
                accessMode
                );
        }
        break;
    case KPH_QUERYINFORMATIONOBJECT:
        {
            struct
            {
                HANDLE ProcessHandle;
                HANDLE Handle;
                KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass;
                PVOID ObjectInformation;
                ULONG ObjectInformationLength;
                PULONG ReturnLength;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiQueryInformationObject(
                input->ProcessHandle,
                input->Handle,
                input->ObjectInformationClass,
                input->ObjectInformation,
                input->ObjectInformationLength,
                input->ReturnLength,
                accessMode
                );
        }
        break;
    case KPH_SETINFORMATIONOBJECT:
        {
            struct
            {
                HANDLE ProcessHandle;
                HANDLE Handle;
                KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass;
                PVOID ObjectInformation;
                ULONG ObjectInformationLength;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiSetInformationObject(
                input->ProcessHandle,
                input->Handle,
                input->ObjectInformationClass,
                input->ObjectInformation,
                input->ObjectInformationLength,
                accessMode
                );
        }
        break;
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
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiDuplicateObject(
                input->SourceProcessHandle,
                input->SourceHandle,
                input->TargetProcessHandle,
                input->TargetHandle,
                input->DesiredAccess,
                input->HandleAttributes,
                input->Options,
                accessMode
                );
        }
        break;
    case KPH_OPENDRIVER:
        {
            struct
            {
                PHANDLE DriverHandle;
                POBJECT_ATTRIBUTES ObjectAttributes;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiOpenDriver(
                input->DriverHandle,
                input->ObjectAttributes,
                accessMode
                );
        }
        break;
    case KPH_QUERYINFORMATIONDRIVER:
        {
            struct
            {
                HANDLE DriverHandle;
                DRIVER_INFORMATION_CLASS DriverInformationClass;
                PVOID DriverInformation;
                ULONG DriverInformationLength;
                PULONG ReturnLength;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiQueryInformationDriver(
                input->DriverHandle,
                input->DriverInformationClass,
                input->DriverInformation,
                input->DriverInformationLength,
                input->ReturnLength,
                accessMode
                );
        }
        break;
    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

ControlEnd:
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}
