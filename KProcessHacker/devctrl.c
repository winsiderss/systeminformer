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

DRIVER_DISPATCH KphDispatchDeviceControl;

NTSTATUS KphDispatchDeviceControl(
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP Irp
    )
{
    NTSTATUS status;
    PIO_STACK_LOCATION stackLocation;
    PVOID systemBuffer;
    ULONG inputBufferLength;
    ULONG ioControlCode;
    ULONG returnLength;

#define VERIFY_INPUT_LENGTH \
    do { if (inputBufferLength != sizeof(*input)) \
    { \
        status = STATUS_INFO_LENGTH_MISMATCH; \
        goto ControlEnd; \
    } } while (0)

    stackLocation = IoGetCurrentIrpStackLocation(Irp);
    systemBuffer = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength = stackLocation->Parameters.DeviceIoControl.InputBufferLength;
    ioControlCode = stackLocation->Parameters.DeviceIoControl.IoControlCode;
    returnLength = 0;

    if (systemBuffer)
    {
        switch (ioControlCode)
        {
        case KPH_GETFEATURES:
            {
                struct
                {
                    PULONG Features;
                } *input = systemBuffer;

                VERIFY_INPUT_LENGTH;

                status = KpiGetFeatures(
                    input->Features,
                    Irp->RequestorMode
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
                } *input = systemBuffer;

                VERIFY_INPUT_LENGTH;

                status = KpiOpenProcess(
                    input->ProcessHandle,
                    input->DesiredAccess,
                    input->ClientId,
                    Irp->RequestorMode
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
                } *input = systemBuffer;

                VERIFY_INPUT_LENGTH;

                status = KpiOpenProcessToken(
                    input->ProcessHandle,
                    input->DesiredAccess,
                    input->TokenHandle,
                    Irp->RequestorMode
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
                } *input = systemBuffer;

                VERIFY_INPUT_LENGTH;

                status = KpiOpenProcessJob(
                    input->ProcessHandle,
                    input->DesiredAccess,
                    input->JobHandle,
                    Irp->RequestorMode
                    );
            }
            break;
        case KPH_SUSPENDPROCESS:
            {
                struct
                {
                    HANDLE ProcessHandle;
                } *input = systemBuffer;

                VERIFY_INPUT_LENGTH;

                status = KpiSuspendProcess(
                    input->ProcessHandle,
                    Irp->RequestorMode
                    );
            }
            break;
        case KPH_RESUMEPROCESS:
            {
                struct
                {
                    HANDLE ProcessHandle;
                } *input = systemBuffer;

                VERIFY_INPUT_LENGTH;

                status = KpiResumeProcess(
                    input->ProcessHandle,
                    Irp->RequestorMode
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
                } *input = systemBuffer;

                VERIFY_INPUT_LENGTH;

                status = KpiReadVirtualMemory(
                    input->ProcessHandle,
                    input->BaseAddress,
                    input->Buffer,
                    input->BufferSize,
                    input->NumberOfBytesRead,
                    Irp->RequestorMode
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
                } *input = systemBuffer;

                VERIFY_INPUT_LENGTH;

                status = KpiWriteVirtualMemory(
                    input->ProcessHandle,
                    input->BaseAddress,
                    input->Buffer,
                    input->BufferSize,
                    input->NumberOfBytesRead,
                    Irp->RequestorMode
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
                } *input = systemBuffer;

                VERIFY_INPUT_LENGTH;

                status = KpiReadVirtualMemoryUnsafe(
                    input->ProcessHandle,
                    input->BaseAddress,
                    input->Buffer,
                    input->BufferSize,
                    input->NumberOfBytesRead,
                    Irp->RequestorMode
                    );
            }
            break;
        }
    }

ControlEnd:
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = returnLength;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}
