/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2015
 *     dmex    2017-2021
 *
 */

#include "dn.h"
#include "svcext.h"
#include "clrsup.h"

/**
 * Calls the 32-bit PhSvc helper to resolve a managed runtime symbol name at the given address.
 *
 * \param ProcessId The process identifier of the target process.
 * \param Address The virtual address to look up.
 * \param Displacement Optionally receives the byte offset from the start of the identified symbol.
 * \return The resolved symbol name, or NULL on failure. The caller must dereference the string.
 */
_Success_(return != NULL)
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

    status = PhPluginQueryPhSvc(&client);

    if (!NT_SUCCESS(status))
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

/**
 * Server-side handler for the GetRuntimeNameByAddress PhSvc request.
 *
 * \param Request The PhSvc request context used to probe shared memory buffers.
 * \param In The input parameters containing the process identifier and address to look up.
 * \param Out The output parameters that receive the symbol name length and displacement.
 * \return An NTSTATUS code. Returns STATUS_BUFFER_OVERFLOW if the name exceeds the provided buffer.
 */
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

    PhClearReference(&name);

    return status;
}

/**
 * Calls the 32-bit PhSvc helper to predict the next managed stack frame register values for a WOW64 process.
 *
 * \param ProcessId The process identifier of the target process.
 * \param ThreadId The OS thread identifier.
 * \param PcAddress The current program counter address.
 * \param FrameAddress The current frame (EBP) address.
 * \param StackAddress The current stack (ESP) address.
 * \param PredictedEip Receives the predicted next instruction pointer, or NULL on failure.
 * \param PredictedEbp Receives the predicted next frame pointer, or NULL on failure.
 * \param PredictedEsp Receives the predicted next stack pointer, or NULL on failure.
 */
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
    NTSTATUS status;
    PH_PLUGIN_PHSVC_CLIENT client;
    DN_API_PREDICTADDRESSESFROMCLRDATA in;
    DN_API_PREDICTADDRESSESFROMCLRDATA out;

    *PredictedEip = NULL;
    *PredictedEbp = NULL;
    *PredictedEsp = NULL;

    status = PhPluginQueryPhSvc(&client);

    if (!NT_SUCCESS(status))
        return;

    in.i.ProcessId = HandleToUlong(ProcessId);
    in.i.ThreadId = HandleToUlong(ThreadId);
    in.i.PcAddress = PtrToUlong(PcAddress);
    in.i.FrameAddress = PtrToUlong(FrameAddress);
    in.i.StackAddress = PtrToUlong(StackAddress);

    status = PhPluginCallPhSvc(
        PluginInstance,
        DnPredictAddressesFromClrDataApiNumber,
        &in,
        sizeof(in),
        &out,
        sizeof(out)
        );

    if (NT_SUCCESS(status))
    {
        *PredictedEip = UlongToPtr(out.o.PredictedEip);
        *PredictedEbp = UlongToPtr(out.o.PredictedEbp);
        *PredictedEsp = UlongToPtr(out.o.PredictedEsp);
    }
}

/**
 * Server-side handler for the PredictAddressesFromClrData PhSvc request.
 *
 * \param Request The PhSvc request context (unused by this handler).
 * \param In The input parameters containing the process, thread, and current register values.
 * \param Out The output parameters that receive the predicted next frame register values.
 * \return An NTSTATUS code.
 */
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

/**
 * Calls the 32-bit PhSvc helper to retrieve the current AppDomain name for the given thread.
 *
 * \param ProcessId The process identifier of the target process.
 * \param ThreadId The OS thread identifier whose current AppDomain is queried.
 * \return The AppDomain display name, or NULL on failure. The caller must dereference the string.
 */
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

    status = PhPluginQueryPhSvc(&client);

    if (!NT_SUCCESS(status))
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

/**
 * Server-side handler for the GetClrThreadAppDomain PhSvc request.
 *
 * \param Request The PhSvc request context used to probe shared memory buffers.
 * \param In The input parameters containing the process and thread identifiers.
 * \param Out The output parameters that receive the AppDomain name length.
 * \return An NTSTATUS code. Returns STATUS_BUFFER_OVERFLOW if the name exceeds the provided buffer.
 */
NTSTATUS DispatchGetClrThreadAppDomain(
    _In_ PPH_PLUGIN_PHSVC_REQUEST Request,
    _In_ PDN_API_GETWOW64THREADAPPDOMAIN In,
    _Out_ PDN_API_GETWOW64THREADAPPDOMAIN Out
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID nameBuffer;
    PCLR_PROCESS_SUPPORT support;
    PPH_STRING appDomainText = NULL;

    if (!NT_SUCCESS(status = Request->ProbeBuffer(&In->i.Name, sizeof(WCHAR), FALSE, &nameBuffer)))
        return status;

    if (!(support = CreateClrProcessSupport(UlongToHandle(In->i.ProcessId))))
        return STATUS_UNSUCCESSFUL;

    if (!(appDomainText = DnGetClrThreadAppDomain(support, UlongToHandle(In->i.ThreadId))))
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    memcpy(nameBuffer, appDomainText->Buffer, min(appDomainText->Length, In->i.Name.Length));
    Out->o.NameLength = (ULONG)appDomainText->Length;

    if (In->i.Name.Length < appDomainText->Length)
        status = STATUS_BUFFER_OVERFLOW;

CleanupExit:

    PhClearReference(&appDomainText);

    if (support)
        FreeClrProcessSupport(support);

    return status;
}

/**
 * Calls the 32-bit PhSvc helper to retrieve the source file name for the managed method at the given address.
 *
 * \param ProcessId The process identifier of the target process.
 * \param Address The virtual address of the managed method.
 * \return The source file name, or NULL on failure. The caller must dereference the string.
 */
PPH_STRING CallGetFileNameByAddress(
    _In_ HANDLE ProcessId,
    _In_ ULONG64 Address
    )
{
    NTSTATUS status;
    PH_PLUGIN_PHSVC_CLIENT client;
    DN_API_GETFILENAMEBYADDRESS in;
    DN_API_GETFILENAMEBYADDRESS out;
    ULONG bufferSize;
    PVOID buffer;
    PPH_STRING name = NULL;

    status = PhPluginQueryPhSvc(&client);

    if (!NT_SUCCESS(status))
        return NULL;

    in.i.ProcessId = HandleToUlong(ProcessId);
    in.i.Address = Address;

    bufferSize = 0x1000;

    if (!(buffer = client.CreateString(NULL, bufferSize, &in.i.Name)))
        return NULL;

    status = PhPluginCallPhSvc(PluginInstance, DnGetFileNameByAddressApiNumber, &in, sizeof(in), &out, sizeof(out));

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        client.FreeHeap(buffer);
        bufferSize = out.o.NameLength;

        if (!(buffer = client.CreateString(NULL, bufferSize, &in.i.Name)))
            return NULL;

        status = PhPluginCallPhSvc(PluginInstance, DnGetFileNameByAddressApiNumber, &in, sizeof(in), &out, sizeof(out));
    }

    if (NT_SUCCESS(status))
    {
        name = PhCreateStringEx(buffer, out.o.NameLength);
    }

    client.FreeHeap(buffer);

    return name;
}

/**
 * Server-side handler for the GetFileNameByAddress PhSvc request.
 *
 * \param Request The PhSvc request context used to probe shared memory buffers.
 * \param In The input parameters containing the process identifier and address.
 * \param Out The output parameters that receive the file name length.
 * \return An NTSTATUS code. Returns STATUS_BUFFER_OVERFLOW if the name exceeds the provided buffer.
 */
NTSTATUS DispatchGetFileNameByAddress(
    _In_ PPH_PLUGIN_PHSVC_REQUEST Request,
    _In_ PDN_API_GETFILENAMEBYADDRESS In,
    _Out_ PDN_API_GETFILENAMEBYADDRESS Out
    )
{
    NTSTATUS status;
    PVOID nameBuffer;
    PCLR_PROCESS_SUPPORT support;
    PPH_STRING name = NULL;

    if (!NT_SUCCESS(status = Request->ProbeBuffer(&In->i.Name, sizeof(WCHAR), FALSE, &nameBuffer)))
        return status;

    support = CreateClrProcessSupport(UlongToHandle(In->i.ProcessId));

    if (!support)
        return STATUS_NOT_SUPPORTED;

    name = DnGetFileNameByAddressClrProcess(support, In->i.Address);

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
    PhClearReference(&name);
    FreeClrProcessSupport(support);

    return status;
}

/**
 * Calls the 32-bit PhSvc helper to retrieve the serialized AppDomain and assembly list for a process.
 *
 * \param ProcessId The process identifier of the target process.
 * \return A list of DN_PROCESS_APPDOMAIN_ENTRY objects, or NULL on failure.
 * \remarks The caller is responsible for freeing the list with DnDestroyProcessDotNetAppDomainList.
 */
PPH_LIST CallGetClrAppDomainAssemblyList(
    _In_ HANDLE ProcessId
    )
{
    NTSTATUS status;
    PH_PLUGIN_PHSVC_CLIENT client;
    DN_API_GETAPPDOMAINASSEMBLYDATA in;
    DN_API_GETAPPDOMAINASSEMBLYDATA out;
    ULONG bufferSize;
    PVOID buffer;
    PPH_LIST appdomainlist = NULL;

    status = PhPluginQueryPhSvc(&client);

    if (!NT_SUCCESS(status))
        return NULL;

    in.i.ProcessId = HandleToUlong(ProcessId);
    bufferSize = 0x1000;

    if (!(buffer = client.CreateString(NULL, bufferSize, &in.i.Name)))
        return NULL;

    status = PhPluginCallPhSvc(PluginInstance, DnGetGetClrWow64AppdomainDataApiNumber, &in, sizeof(in), &out, sizeof(out));

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        client.FreeHeap(buffer);
        bufferSize = out.o.NameLength;

        if (!(buffer = client.CreateString(NULL, bufferSize, &in.i.Name)))
            return NULL;

        status = PhPluginCallPhSvc(PluginInstance, DnGetGetClrWow64AppdomainDataApiNumber, &in, sizeof(in), &out, sizeof(out));
    }

    if (NT_SUCCESS(status))
    {
        PPH_BYTES string = PhCreateBytesEx(buffer, out.o.NameLength);

        appdomainlist = DnProcessAppDomainListDeserialize(string);

        PhDereferenceObject(string);
    }

    client.FreeHeap(buffer);

    return appdomainlist;
}

/**
 * Server-side handler for the GetClrAppDomainAssemblyList PhSvc request.
 *
 * \param Request The PhSvc request context used to probe shared memory buffers.
 * \param In The input parameters containing the process identifier.
 * \param Out The output parameters that receive the serialized data length.
 * \return An NTSTATUS code. Returns STATUS_BUFFER_OVERFLOW if the data exceeds the provided buffer.
 */
NTSTATUS DispatchGetClrAppDomainAssemblyList(
    _In_ PPH_PLUGIN_PHSVC_REQUEST Request,
    _In_ PDN_API_GETAPPDOMAINASSEMBLYDATA In,
    _Out_ PDN_API_GETAPPDOMAINASSEMBLYDATA Out
    )
{
    NTSTATUS status;
    PVOID nameBuffer;
    PCLR_PROCESS_SUPPORT support;
    PPH_LIST appdomainlist = NULL;
    PPH_BYTES string = NULL;

    if (!NT_SUCCESS(status = Request->ProbeBuffer(&In->i.Name, sizeof(CHAR), FALSE, &nameBuffer)))
        return status;

    support = CreateClrProcessSupport(UlongToHandle(In->i.ProcessId));

    if (!support)
        return STATUS_NOT_SUPPORTED;

    appdomainlist = DnGetClrAppDomainAssemblyList(support);

    if (!appdomainlist)
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    string = DnProcessAppDomainListSerialize(appdomainlist);

    if (!(string && string->Length))
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    memcpy(nameBuffer, string->Buffer, min(string->Length, In->i.Name.Length));
    Out->o.NameLength = (ULONG)string->Length;

    if (In->i.Name.Length < string->Length)
        status = STATUS_BUFFER_OVERFLOW;

CleanupExit:
    if (appdomainlist)
        DnDestroyProcessDotNetAppDomainList(appdomainlist);
    if (string)
        PhDereferenceObject(string);

    FreeClrProcessSupport(support);

    return status;
}

/**
 * Dispatches an incoming PhSvc request to the appropriate DotNetTools handler.
 *
 * \param Parameter A pointer to the PPH_PLUGIN_PHSVC_REQUEST describing the request.
 * \remarks The input buffer is copied before dispatching because InBuffer and OutBuffer may alias.
 */
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
    case DnGetFileNameByAddressApiNumber:
        request->ReturnStatus = DispatchGetFileNameByAddress(request, inBuffer, request->OutBuffer);
        break;
    case DnGetGetClrWow64AppdomainDataApiNumber:
        request->ReturnStatus = DispatchGetClrAppDomainAssemblyList(request, inBuffer, request->OutBuffer);
        break;
    }

    PhFree(inBuffer);
}
