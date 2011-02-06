/*
 * Process Hacker - 
 *   server API
 * 
 * Copyright (C) 2011 wj32
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

#include <phapp.h>
#include <phsvc.h>

PPHSVC_API_PROCEDURE PhSvcApiCallTable[] =
{
    PhSvcApiClose,
    PhSvcApiExecuteRunAsCommand
};

NTSTATUS PhSvcApiInitialization()
{
    return STATUS_SUCCESS;
}

VOID PhSvcDispatchApiCall(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message,
    __out PPHSVC_API_MSG *ReplyMessage,
    __out PHANDLE ReplyPortHandle
    )
{
    NTSTATUS status;

    if (
        Message->ApiNumber == 0 ||
        (ULONG)Message->ApiNumber >= (ULONG)PhSvcMaximumApiNumber ||
        !PhSvcApiCallTable[Message->ApiNumber - 1]
        )
    {
        Message->ReturnStatus = STATUS_INVALID_SYSTEM_SERVICE;
        *ReplyMessage = Message;
        return;
    }

    status = PhSvcApiCallTable[Message->ApiNumber - 1](Client, Message);
    Message->ReturnStatus = status;

    *ReplyMessage = Message;
    *ReplyPortHandle = Client->PortHandle;
}

NTSTATUS PhSvcCaptureBuffer(
    __in PPH_RELATIVE_STRINGREF String,
    __out PVOID *CapturedBuffer
    )
{
    PPHSVC_CLIENT client = PhSvcGetCurrentClient();
    PVOID address;
    PVOID buffer;

    address = (PCHAR)client->ClientViewBase + String->Offset;

    if (
        (ULONG_PTR)address + String->Length < (ULONG_PTR)address ||
        (ULONG_PTR)address < (ULONG_PTR)client->ClientViewBase ||
        (ULONG_PTR)address + String->Length >= (ULONG_PTR)client->ClientViewLimit
        )
    {
        return STATUS_ACCESS_VIOLATION;
    }

    buffer = PhAllocateSafe(String->Length);

    if (!buffer)
        return STATUS_NO_MEMORY;

    memcpy(buffer, address, String->Length);
    *CapturedBuffer = buffer;

    return STATUS_SUCCESS;
}

NTSTATUS PhSvcCaptureString(
    __in PPH_RELATIVE_STRINGREF String,
    __out PPH_STRING *CapturedString
    )
{
    PPHSVC_CLIENT client = PhSvcGetCurrentClient();
    PVOID address;

    if (String->Length & 1)
        return STATUS_INVALID_BUFFER_SIZE;
    if (String->Length > 0xfffe)
        return STATUS_INVALID_BUFFER_SIZE;

    address = (PCHAR)client->ClientViewBase + String->Offset;

    if ((ULONG_PTR)address & 1)
    {
        return STATUS_DATATYPE_MISALIGNMENT;
    }

    if (
        (ULONG_PTR)address + String->Length < (ULONG_PTR)address ||
        (ULONG_PTR)address < (ULONG_PTR)client->ClientViewBase ||
        (ULONG_PTR)address + String->Length >= (ULONG_PTR)client->ClientViewLimit
        )
    {
        return STATUS_ACCESS_VIOLATION;
    }

    *CapturedString = PhCreateStringEx(address, String->Length);

    return STATUS_SUCCESS;
}

NTSTATUS PhSvcApiClose(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    )
{
    return PhSvcCloseHandle(Message->Close.i.Handle);
}

NTSTATUS PhSvcApiExecuteRunAsCommand(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    )
{
    NTSTATUS status;
    PPH_STRING serviceCommandLine;
    PPH_STRING serviceName;

    if (NT_SUCCESS(status = PhSvcCaptureString(&Message->ExecuteRunAsCommand.i.ServiceCommandLine, &serviceCommandLine)))
    {
        if (NT_SUCCESS(status = PhSvcCaptureString(&Message->ExecuteRunAsCommand.i.ServiceName, &serviceName)))
        {
            status = PhExecuteRunAsCommand(serviceCommandLine->Buffer, serviceName->Buffer);
            PhDereferenceObject(serviceName);
        }

        PhDereferenceObject(serviceCommandLine);
    }

    return status;
}
