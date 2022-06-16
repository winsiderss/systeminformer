/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *
 */

#include <kph.h>

NTSTATUS KphDispatchDeviceControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
    )
{
    NTSTATUS status;
    PIO_STACK_LOCATION stackLocation;
    PFILE_OBJECT fileObject;
    PKPH_CLIENT client;
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
    fileObject = stackLocation->FileObject;
    client = fileObject->FsContext;

    originalInput = stackLocation->Parameters.DeviceIoControl.Type3InputBuffer;
    inputLength = stackLocation->Parameters.DeviceIoControl.InputBufferLength;
    ioControlCode = stackLocation->Parameters.DeviceIoControl.IoControlCode;
    accessMode = Irp->RequestorMode;

    // Make sure we have a client object.
    if (!client)
    {
        status = STATUS_INTERNAL_ERROR;
        goto ControlEnd;
    }

    // Enforce signature requirement
    if ((ioControlCode != KPH_GETFEATURES && ioControlCode != KPH_VERIFYCLIENT) &&
        !client->VerificationSucceeded)
    {
        status = STATUS_ACCESS_DENIED;
        goto ControlEnd;
    }

    // Make sure we actually have input if the input length is non-zero.
    if (inputLength != 0 && !originalInput)
    {
        status = STATUS_INVALID_BUFFER_SIZE;
        goto ControlEnd;
    }

    // Make sure the caller isn't giving us a huge buffer. If they are, it can't be correct because
    // we have a compile-time check that makes sure our buffer can store the arguments for all the
    // calls.
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
    case KPH_VERIFYCLIENT:
        {
            struct
            {
                PVOID CodeAddress;
                PVOID Signature;
                ULONG SignatureSize;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            if (accessMode == UserMode)
            {
                status = KpiVerifyClient(
                    input->CodeAddress,
                    input->Signature,
                    input->SignatureSize,
                    client
                    );
            }
            else
            {
                status = STATUS_UNSUCCESSFUL;
            }
        }
        break;
    case KPH_RETRIEVEKEY:
        {
            struct
            {
                KPH_KEY_LEVEL KeyLevel;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            if (accessMode == UserMode)
            {
                status = KphRetrieveKeyViaApc(
                    client,
                    input->KeyLevel,
                    Irp
                    );
            }
            else
            {
                status = STATUS_UNSUCCESSFUL;
            }
        }
        break;
    case KPH_OPENPROCESS:
        {
            struct
            {
                PHANDLE ProcessHandle;
                ACCESS_MASK DesiredAccess;
                PCLIENT_ID ClientId;
                KPH_KEY Key;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiOpenProcess(
                input->ProcessHandle,
                input->DesiredAccess,
                input->ClientId,
                input->Key,
                client,
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
                KPH_KEY Key;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiOpenProcessToken(
                input->ProcessHandle,
                input->DesiredAccess,
                input->TokenHandle,
                input->Key,
                client,
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
                0, // TODO: interface needs updated to pass key
                client,
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
                KPH_KEY Key;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiTerminateProcess(
                input->ProcessHandle,
                input->ExitStatus,
                input->Key,
                client,
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
                KPH_KEY Key;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiReadVirtualMemoryUnsafe(
                input->ProcessHandle,
                input->BaseAddress,
                input->Buffer,
                input->BufferSize,
                input->NumberOfBytesRead,
                input->Key,
                client,
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
                KPH_KEY Key;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiOpenThread(
                input->ThreadHandle,
                input->DesiredAccess,
                input->ClientId,
                input->Key,
                client,
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
                0, // TODO: interface needs updated to pass key
                client,
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
                accessMode,
                0
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
    case KPH_OPENDRIVER:
        {
            struct
            {
                PHANDLE DriverHandle;
                ACCESS_MASK DesiredAccess;
                POBJECT_ATTRIBUTES ObjectAttributes;
            } *input = capturedInputPointer;

            VERIFY_INPUT_LENGTH;

            status = KpiOpenDriver(
                input->DriverHandle,
                input->DesiredAccess,
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
