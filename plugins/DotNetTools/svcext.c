/*
 * Process Hacker .NET Tools -
 *   phsvc extensions
 *
 * Copyright (C) 2015 wj32
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
    NTSTATUS status;
    PVOID nameBuffer;
    PCLR_PROCESS_SUPPORT support;
    PPH_STRING name;

    if (!NT_SUCCESS(status = Request->ProbeBuffer(&In->i.Name, sizeof(WCHAR), FALSE, &nameBuffer)))
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

VOID CallPredictAddressesFromClrData(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ PVOID PcAddress,
    _In_ PVOID FrameAddress,
    _In_ PVOID StackAddress,
    _Out_ PVOID *PredictedEip,
    _Out_ PVOID *PredictedEbp,
    _Out_ PVOID *PredictedEsp
    )
{
    PH_PLUGIN_PHSVC_CLIENT client;
    DN_API_PREDICTADDRESSESFROMCLRDATA in;
    DN_API_PREDICTADDRESSESFROMCLRDATA out;

    *PredictedEip = NULL;
    *PredictedEbp = NULL;
    *PredictedEsp = NULL;

    if (!PhPluginQueryPhSvc(&client))
        return;

    in.i.ProcessId = HandleToUlong(ProcessId);
    in.i.ThreadId = HandleToUlong(ThreadId);
    in.i.PcAddress = PtrToUlong(PcAddress);
    in.i.FrameAddress = PtrToUlong(FrameAddress);
    in.i.StackAddress = PtrToUlong(StackAddress);

    if (NT_SUCCESS(PhPluginCallPhSvc(PluginInstance, DnPredictAddressesFromClrDataApiNumber, &in, sizeof(in), &out, sizeof(out))))
    {
        *PredictedEip = UlongToPtr(out.o.PredictedEip);
        *PredictedEbp = UlongToPtr(out.o.PredictedEbp);
        *PredictedEsp = UlongToPtr(out.o.PredictedEsp);
    }
}

NTSTATUS DispatchPredictAddressesFromClrData(
    _In_ PPH_PLUGIN_PHSVC_REQUEST Request,
    _In_ PDN_API_PREDICTADDRESSESFROMCLRDATA In,
    _Out_ PDN_API_PREDICTADDRESSESFROMCLRDATA Out
    )
{
    PCLR_PROCESS_SUPPORT support;
    PVOID predictedEip;
    PVOID predictedEbp;
    PVOID predictedEsp;

    support = CreateClrProcessSupport(UlongToHandle(In->i.ProcessId));

    if (!support)
        return STATUS_UNSUCCESSFUL;

    PredictAddressesFromClrData(
        support,
        UlongToHandle(In->i.ThreadId),
        UlongToPtr(In->i.PcAddress),
        UlongToPtr(In->i.FrameAddress),
        UlongToPtr(In->i.StackAddress),
        &predictedEip,
        &predictedEbp,
        &predictedEsp
        );
    FreeClrProcessSupport(support);

    Out->o.PredictedEip = PtrToUlong(predictedEip);
    Out->o.PredictedEbp = PtrToUlong(predictedEbp);
    Out->o.PredictedEsp = PtrToUlong(predictedEsp);

    return STATUS_SUCCESS;
}

PPH_STRING CallGetClrThreadAppDomain(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId
    )
{
    NTSTATUS status;
    PH_PLUGIN_PHSVC_CLIENT client;
    DN_API_GETWOW64THREADAPPDOMAIN in;
    DN_API_GETWOW64THREADAPPDOMAIN out;
    ULONG bufferSize;
    PVOID buffer;
    PPH_STRING name = NULL;

    if (!PhPluginQueryPhSvc(&client))
        return NULL;

    in.i.ProcessId = HandleToUlong(ProcessId);
    in.i.ThreadId = HandleToUlong(ThreadId);

    bufferSize = 0x1000;

    if (!(buffer = client.CreateString(NULL, bufferSize, &in.i.Name)))
        return NULL;

    status = PhPluginCallPhSvc(PluginInstance, DnGetGetClrWow64ThreadAppDomainApiNumber, &in, sizeof(in), &out, sizeof(out));

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        client.FreeHeap(buffer);
        bufferSize = out.o.NameLength;

        if (!(buffer = client.CreateString(NULL, bufferSize, &in.i.Name)))
            return NULL;

        status = PhPluginCallPhSvc(PluginInstance, DnGetGetClrWow64ThreadAppDomainApiNumber, &in, sizeof(in), &out, sizeof(out));
    }

    if (NT_SUCCESS(status))
    {
        name = PhCreateStringEx(buffer, out.o.NameLength);
    }

    client.FreeHeap(buffer);

    return name;
}

NTSTATUS DispatchGetClrThreadAppDomain(
    _In_ PPH_PLUGIN_PHSVC_REQUEST Request,
    _In_ PDN_API_GETWOW64THREADAPPDOMAIN In,
    _Out_ PDN_API_GETWOW64THREADAPPDOMAIN Out
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID nameBuffer;
    PCLR_PROCESS_SUPPORT support;
    IXCLRDataProcess *process;
    IXCLRDataTask *task;
    IXCLRDataAppDomain *appDomain;
    PPH_STRING AppDomainText = NULL;

    if (!NT_SUCCESS(status = Request->ProbeBuffer(&In->i.Name, sizeof(WCHAR), FALSE, &nameBuffer)))
        return status;

    if (!(support = CreateClrProcessSupport(UlongToHandle(In->i.ProcessId))))
        return STATUS_UNSUCCESSFUL;

    if (!(process = support->DataProcess))
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    if (SUCCEEDED(IXCLRDataProcess_GetTaskByOSThreadID(process, In->i.ThreadId, &task)))
    {
        if (SUCCEEDED(IXCLRDataTask_GetCurrentAppDomain(task, &appDomain)))
        {
            AppDomainText = GetNameXClrDataAppDomain(appDomain);
            IXCLRDataAppDomain_Release(appDomain);
        }

        IXCLRDataTask_Release(task);
    }

    if (!AppDomainText)
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    memcpy(nameBuffer, AppDomainText->Buffer, min(AppDomainText->Length, In->i.Name.Length));
    Out->o.NameLength = (ULONG)AppDomainText->Length;

    if (In->i.Name.Length < AppDomainText->Length)
        status = STATUS_BUFFER_OVERFLOW;

CleanupExit:

    if (support)
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
    case DnPredictAddressesFromClrDataApiNumber:
        request->ReturnStatus = DispatchPredictAddressesFromClrData(request, inBuffer, request->OutBuffer);
        break;
    case DnGetGetClrWow64ThreadAppDomainApiNumber:
        request->ReturnStatus = DispatchGetClrThreadAppDomain(request, inBuffer, request->OutBuffer);
        break;
    }

    PhFree(inBuffer);
}
