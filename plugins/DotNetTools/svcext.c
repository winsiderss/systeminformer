/*
 * Process Hacker .NET Tools -
 *   phsvc extensions
 *
 * Copyright (C) 2015 wj32
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

#include "dn.h"
#include "svcext.h"
#include "clrsup.h"

PPH_STRING CallGetRuntimeNameByAddress(
    _In_ HANDLE ProcessId,
    _In_ ULONG64 Address,
    _Out_opt_ PULONG64 Displacement
    )
{
    NTSTATUS status;
    PH_PLUGIN_PHSVC_CLIENT client;
    DN_API_GETRUNTIMENAMEBYADDRESS in;
    DN_API_GETRUNTIMENAMEBYADDRESS out;
    ULONG bufferSize;
    PVOID buffer;
    PPH_STRING name = NULL;

    if (!PhPluginQueryPhSvc(&client))
        return NULL;

    in.i.ProcessId = HandleToUlong(ProcessId);
    in.i.Address = Address;

    bufferSize = 0x1000;

    if (!(buffer = client.CreateString(NULL, bufferSize, &in.i.Name)))
        return NULL;

    status = PhPluginCallPhSvc(PluginInstance, DnGetRuntimeNameByAddressApiNumber, &in, sizeof(in), &out, sizeof(out));

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        client.FreeHeap(buffer);
        bufferSize = out.o.NameLength;

        if (!(buffer = client.CreateString(NULL, bufferSize, &in.i.Name)))
            return NULL;

        status = PhPluginCallPhSvc(PluginInstance, DnGetRuntimeNameByAddressApiNumber, &in, sizeof(in), &out, sizeof(out));
    }

    if (NT_SUCCESS(status))
    {
        name = PhCreateStringEx(buffer, out.o.NameLength);

        if (Displacement)
            *Displacement = out.o.Displacement;
    }

    client.FreeHeap(buffer);

    return name;
}

NTSTATUS DispatchGetRuntimeNameByAddress(
    _In_ PPH_PLUGIN_PHSVC_REQUEST Request,
    _In_ PDN_API_GETRUNTIMENAMEBYADDRESS In,
    _Out_ PDN_API_GETRUNTIMENAMEBYADDRESS Out
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID nameBuffer;
    PCLR_PROCESS_SUPPORT support;
    PPH_STRING name;

    if (!NT_SUCCESS(status = Request->ProbeBuffer(&In->i.Name, FALSE, &nameBuffer)))
        return status;

    support = CreateClrProcessSupport(UlongToHandle(In->i.ProcessId));

    if (!support)
        return STATUS_UNSUCCESSFUL;

    name = GetRuntimeNameByAddressClrProcess(support, In->i.Address, &Out->o.Displacement);

    if (!name)
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    memcpy(nameBuffer, name->Buffer, min(name->Length, In->i.Name.Length));
    Out->o.NameLength = (ULONG)name->Length;

    if (In->i.Name.Length < name->Length)
        status = STATUS_BUFFER_OVERFLOW;

CleanupExit:
    FreeClrProcessSupport(support);

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
    case DnGetRuntimeNameByAddressApiNumber:
        request->ReturnStatus = DispatchGetRuntimeNameByAddress(request, inBuffer, request->OutBuffer);
        break;
    }

    PhFree(inBuffer);
}
