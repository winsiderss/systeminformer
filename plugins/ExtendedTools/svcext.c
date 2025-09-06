/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2018
 *
 */

#include "exttools.h"

NTSTATUS CallGetProcessUnloadedDlls(
    _In_ HANDLE ProcessId,
    _Out_ PPH_BYTES *UnloadedDlls
    )
{
    NTSTATUS status;
    PH_PLUGIN_PHSVC_CLIENT client;
    ET_PHSVC_API_GETWOW64THREADAPPDOMAIN in;
    ET_PHSVC_API_GETWOW64THREADAPPDOMAIN out;
    ULONG bufferSize;
    PVOID buffer;

    status = PhPluginQueryPhSvc(&client);

    if (!NT_SUCCESS(status))
        return status;

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
        {
            *UnloadedDlls = PhCreateBytesEx(buffer, out.o.DataLength);
        }
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
    PPH_BYTES eventHexBufferUtf8Text = NULL;

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
        UInt32x32To64(capturedElementSize, capturedElementCount)
        );

    if (!eventHexBufferText)
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    if (!(eventHexBufferUtf8Text = PhConvertStringToUtf8(eventHexBufferText)))
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    if (In->i.Data.Length < eventHexBufferUtf8Text->Length)
    {
        status = STATUS_BUFFER_OVERFLOW;
        goto CleanupExit;
    }

    if (memcpy_s(dataBuffer, In->i.Data.Length, eventHexBufferUtf8Text->Buffer, eventHexBufferUtf8Text->Length) != 0)
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    status = RtlSIZETToULong(eventHexBufferUtf8Text->Length, &Out->o.DataLength);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

CleanupExit:
    PhClearReference(&eventHexBufferUtf8Text);
    PhClearReference(&eventHexBufferText);

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
