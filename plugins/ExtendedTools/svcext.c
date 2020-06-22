/*
 * Process Hacker Extended Tools -
 *   phsvc extensions
 *
 * Copyright (C) 2018 dmex
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

#include "exttools.h"

NTSTATUS CallGetProcessUnloadedDlls(
    _In_ HANDLE ProcessId,
    _Out_ PPH_STRING *UnloadedDlls
    )
{
    NTSTATUS status;
    PH_PLUGIN_PHSVC_CLIENT client;
    ET_PHSVC_API_GETWOW64THREADAPPDOMAIN in;
    ET_PHSVC_API_GETWOW64THREADAPPDOMAIN out;
    ULONG bufferSize;
    PVOID buffer;

    if (!PhPluginQueryPhSvc(&client))
        return STATUS_FAIL_CHECK;

    in.i.ProcessId = HandleToUlong(ProcessId);
    bufferSize = 0x1000;

    if (!(buffer = client.CreateString(NULL, bufferSize, &in.i.Data)))
        return STATUS_FAIL_CHECK;

    status = PhPluginCallPhSvc(PluginInstance, EtPhSvcGetProcessUnloadedDllsApiNumber, &in, sizeof(in), &out, sizeof(out));

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        client.FreeHeap(buffer);
        bufferSize = out.o.DataLength;

        if (!(buffer = client.CreateString(NULL, bufferSize, &in.i.Data)))
            return STATUS_FAIL_CHECK;

        status = PhPluginCallPhSvc(PluginInstance, EtPhSvcGetProcessUnloadedDllsApiNumber, &in, sizeof(in), &out, sizeof(out));
    }

    if (NT_SUCCESS(status))
    {
        if (UnloadedDlls)
            *UnloadedDlls = PhCreateStringEx(buffer, out.o.DataLength);
    }

    client.FreeHeap(buffer);

    return status;
}

NTSTATUS DispatchGetProcessUnloadedDlls(
    _In_ PPH_PLUGIN_PHSVC_REQUEST Request,
    _In_ PET_PHSVC_API_GETWOW64THREADAPPDOMAIN In,
    _Out_ PET_PHSVC_API_GETWOW64THREADAPPDOMAIN Out
    )
{
    NTSTATUS status;
    PVOID dataBuffer;
    ULONG capturedElementSize;
    ULONG capturedElementCount;
    PVOID capturedEventTrace = NULL;
    PPH_STRING eventHexBufferText = NULL;

    if (!NT_SUCCESS(status = Request->ProbeBuffer(&In->i.Data, sizeof(WCHAR), FALSE, &dataBuffer)))
        return status;

    status = PhGetProcessUnloadedDlls(
        UlongToHandle(In->i.ProcessId),
        &capturedEventTrace,
        &capturedElementSize,
        &capturedElementCount
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    eventHexBufferText = PhBufferToHexString(
        capturedEventTrace,
        capturedElementSize * capturedElementCount
        );

    if (!eventHexBufferText)
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    if (In->i.Data.Length < eventHexBufferText->Length)
    {
        status = STATUS_BUFFER_OVERFLOW;
        goto CleanupExit;
    }

    memcpy(dataBuffer, eventHexBufferText->Buffer, min(eventHexBufferText->Length, In->i.Data.Length));
    Out->o.DataLength = (ULONG)eventHexBufferText->Length;

CleanupExit:
    if (eventHexBufferText)
        PhDereferenceObject(eventHexBufferText);

    return status;
}

VOID DispatchPhSvcRequest(
    _In_ PVOID Parameter
    )
{
    PPH_PLUGIN_PHSVC_REQUEST request = Parameter;
    PVOID inBuffer;

    // InBuffer can alias OutBuffer, so make a copy of InBuffer.
    inBuffer = PhAllocateCopy(request->InBuffer, request->InLength);

    switch (request->SubId)
    {
    case EtPhSvcGetProcessUnloadedDllsApiNumber:
        request->ReturnStatus = DispatchGetProcessUnloadedDlls(request, inBuffer, request->OutBuffer);
        break;
    }

    PhFree(inBuffer);
}
