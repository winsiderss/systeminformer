/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include "agenttools.h"

NTSTATUS AtResolveProcessTarget(
    _In_opt_ PVOID Arguments,
    _In_ BOOLEAN RequireSequenceNumber,
    _In_ ACCESS_MASK ProcessAccess,
    _Out_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    ULONG64 processId;
    ULONG64 sequenceNumber;
    BOOLEAN haveSequenceNumber;
    PPH_PROCESS_ITEM processItem;
    HANDLE processHandle = NULL;

    // Zero the whole target so AtDeleteTarget never frees an uninitialized field when a direct
    // caller (a read tool) did not memset its stack target first.
    memset(Target, 0, sizeof(AT_TARGET));

    if (!AtGetArgumentUInt64(Arguments, "pid", &processId) || processId > MAXULONG)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"pid is required and must be an integer.");
        return STATUS_INVALID_PARAMETER;
    }

    haveSequenceNumber = AtGetArgumentUInt64(Arguments, "process_sequence_number", &sequenceNumber);

    if (RequireSequenceNumber && !haveSequenceNumber)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"process_sequence_number is required; take it from list_processes or get_process.");
        return STATUS_INVALID_PARAMETER;
    }

    if (!(processItem = PhReferenceProcessItem(UlongToHandle((ULONG)processId))))
    {
        AtSetToolError(Result, "not_found", STATUS_NOT_FOUND, L"No process with pid %llu is in the provider cache.", processId);
        return STATUS_NOT_FOUND;
    }

    if (haveSequenceNumber && processItem->ProcessSequenceNumber != sequenceNumber)
    {
        AtSetToolError(
            Result,
            "identity_mismatch",
            STATUS_PROCESS_IS_TERMINATING,
            L"pid %llu is now process_sequence_number %llu, not %llu; the process you were shown has exited and the pid was reused. Re-list and try again.",
            processId,
            processItem->ProcessSequenceNumber,
            sequenceNumber
            );
        PhDereferenceObject(processItem);
        return STATUS_PROCESS_IS_TERMINATING;
    }

    if (ProcessAccess)
    {
        NTSTATUS status;
        ULONGLONG liveSequenceNumber;

        if (!PH_IS_REAL_PROCESS_ID(processItem->ProcessId))
        {
            AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_CID, L"This pid is not a real process.");
            PhDereferenceObject(processItem);
            return STATUS_INVALID_CID;
        }

        status = PhOpenProcess(
            &processHandle,
            ProcessAccess | PROCESS_QUERY_LIMITED_INFORMATION,
            processItem->ProcessId
            );

        if (!NT_SUCCESS(status))
        {
            AtSetToolStatusError(Result, status, L"Opening the process");
            PhDereferenceObject(processItem);
            return status;
        }

        status = PhGetProcessSequenceNumber(processHandle, &liveSequenceNumber);

        if (!NT_SUCCESS(status))
        {
            AtSetToolStatusError(Result, status, L"Validating the process identity");
            NtClose(processHandle);
            PhDereferenceObject(processItem);
            return status;
        }

        if (liveSequenceNumber != processItem->ProcessSequenceNumber)
        {
            AtSetToolError(
                Result,
                "identity_mismatch",
                STATUS_PROCESS_IS_TERMINATING,
                L"pid %llu is now process_sequence_number %llu, not %llu; the process you were shown has exited and the pid was reused. Re-list and try again.",
                processId,
                liveSequenceNumber,
                processItem->ProcessSequenceNumber
                );
            NtClose(processHandle);
            PhDereferenceObject(processItem);
            return STATUS_PROCESS_IS_TERMINATING;
        }
    }

    Target->Kind = AtTargetProcess;
    Target->ProcessItem = processItem;
    Target->ProcessHandle = processHandle;
    Target->Identity[0] = HandleToUlong(processItem->ProcessId);
    Target->Identity[1] = processItem->ProcessSequenceNumber;

    return STATUS_SUCCESS;
}

// The ISO 8601 form of a thread's creation time, the same text get_process_threads returns in a
// thread row. Tid reuse inside a process is what process_sequence_number solves for pids.
PPH_STRING AtFormatThreadCreateTime(
    _In_ HANDLE ThreadHandle
    )
{
    KERNEL_USER_TIMES times;
    SYSTEMTIME systemTime;

    if (!NT_SUCCESS(PhGetThreadTimes(ThreadHandle, &times)) || times.CreateTime.QuadPart == 0)
        return NULL;

    PhLargeIntegerToSystemTime(&systemTime, &times.CreateTime);

    return PhFormatSystemTimeISO(&systemTime);
}

NTSTATUS AtpResolveThreadTarget(
    _In_opt_ PVOID Arguments,
    _In_ BOOLEAN RequireSequenceNumber,
    _In_ ACCESS_MASK ThreadAccess,
    _Out_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    ULONG64 threadId;
    HANDLE threadHandle;
    THREAD_BASIC_INFORMATION basicInfo;
    PPH_STRING createTime;

    memset(Target, 0, sizeof(AT_TARGET));

    if (!AtGetArgumentUInt64(Arguments, "tid", &threadId) || threadId > MAXULONG || threadId == 0)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"tid is required and must be an integer.");
        return STATUS_INVALID_PARAMETER;
    }

    status = AtResolveProcessTarget(Arguments, RequireSequenceNumber, PROCESS_QUERY_LIMITED_INFORMATION, Target, Result);

    if (!NT_SUCCESS(status))
        return status;

    status = PhOpenThread(
        &threadHandle,
        ThreadAccess | THREAD_QUERY_LIMITED_INFORMATION,
        UlongToHandle((ULONG)threadId)
        );

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Opening the thread");
        AtDeleteTarget(Target);
        return status;
    }

    status = PhGetThreadBasicInformation(threadHandle, &basicInfo);

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Validating the thread identity");
        NtClose(threadHandle);
        AtDeleteTarget(Target);
        return status;
    }

    if (basicInfo.ClientId.UniqueProcess != Target->ProcessItem->ProcessId)
    {
        AtSetToolError(
            Result,
            "identity_mismatch",
            STATUS_INVALID_CID,
            L"Thread %llu belongs to pid %lu, not pid %lu. Re-list the threads and try again.",
            threadId,
            HandleToUlong(basicInfo.ClientId.UniqueProcess),
            HandleToUlong(Target->ProcessItem->ProcessId)
            );
        NtClose(threadHandle);
        AtDeleteTarget(Target);
        return STATUS_INVALID_CID;
    }

    // Optional, like process_sequence_number on a read: when the caller passes the create_time it
    // saw, a recycled tid is refused instead of acted on.
    if (createTime = AtGetArgumentString(Arguments, "create_time"))
    {
        PPH_STRING liveCreateTime = AtFormatThreadCreateTime(threadHandle);

        if (!liveCreateTime)
        {
            AtSetToolError(
                Result,
                "identity_mismatch",
                STATUS_INVALID_CID,
                L"The creation time of thread %llu could not be read, so its identity could not be checked.",
                threadId
                );
            PhDereferenceObject(createTime);
            NtClose(threadHandle);
            AtDeleteTarget(Target);
            return STATUS_INVALID_CID;
        }

        if (!PhEqualString(liveCreateTime, createTime, TRUE))
        {
            AtSetToolError(
                Result,
                "identity_mismatch",
                STATUS_INVALID_CID,
                L"Thread %llu was created at %s, not %s; the tid has been reused. Re-list the threads and try again.",
                threadId,
                PhGetString(liveCreateTime),
                PhGetString(createTime)
                );
            PhDereferenceObject(liveCreateTime);
            PhDereferenceObject(createTime);
            NtClose(threadHandle);
            AtDeleteTarget(Target);
            return STATUS_INVALID_CID;
        }

        PhDereferenceObject(liveCreateTime);
        PhDereferenceObject(createTime);
    }

    Target->Kind = AtTargetThread;
    Target->ThreadId = UlongToHandle((ULONG)threadId);
    Target->ThreadHandle = threadHandle;
    Target->Identity[2] = threadId;

    return STATUS_SUCCESS;
}

NTSTATUS AtpResolveServiceTarget(
    _In_opt_ PVOID Arguments,
    _In_ ACCESS_MASK ServiceAccess,
    _Out_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PPH_STRING name;
    PPH_SERVICE_ITEM serviceItem;
    SC_HANDLE serviceHandle = NULL;

    memset(Target, 0, sizeof(AT_TARGET));

    if (!(name = AtGetArgumentString(Arguments, "name")) || name->Length == 0)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"name is required and must be the service name (not the display name).");
        PhClearReference(&name);
        return STATUS_INVALID_PARAMETER;
    }

    if (!(serviceItem = PhReferenceServiceItem(&name->sr)))
    {
        AtSetToolError(Result, "not_found", STATUS_NOT_FOUND, L"No service named %s is in the provider cache. Use list_services to find the service name.", PhGetString(name));
        PhDereferenceObject(name);
        return STATUS_NOT_FOUND;
    }

    if (ServiceAccess)
    {
        NTSTATUS status;

        status = PhOpenService(&serviceHandle, ServiceAccess, PhGetString(serviceItem->Name));

        if (!NT_SUCCESS(status))
        {
            AtSetToolStatusError(Result, status, L"Opening the service");
            PhDereferenceObject(serviceItem);
            PhDereferenceObject(name);
            return status;
        }
    }

    Target->Kind = AtTargetService;
    Target->ServiceItem = serviceItem;
    Target->ServiceHandle = serviceHandle;
    Target->Identity[0] = PhHashStringRefEx(&serviceItem->Name->sr, TRUE, PH_STRING_HASH_X65599);

    PhDereferenceObject(name);

    return STATUS_SUCCESS;
}

NTSTATUS AtResolveHandleTarget(
    _In_opt_ PVOID Arguments,
    _In_ BOOLEAN RequireSequenceNumber,
    _In_ ACCESS_MASK ProcessAccess,
    _Out_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    ULONG64 handleValue;
    PPH_STRING expectedType = NULL;

    memset(Target, 0, sizeof(AT_TARGET));
    PSYSTEM_HANDLE_INFORMATION_EX handles;
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX entry = NULL;
    ULONG_PTR i;
    PPH_STRING typeName = NULL;
    PPH_STRING bestName = NULL;

    if (!AtGetArgumentPointer(Arguments, "handle", &handleValue) || handleValue == 0)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"handle is required; pass the handle value from get_process_handles.");
        return STATUS_INVALID_PARAMETER;
    }

    status = AtResolveProcessTarget(Arguments, RequireSequenceNumber, ProcessAccess, Target, Result);

    if (!NT_SUCCESS(status))
        return status;

    status = PhEnumHandlesEx(&handles);

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Enumerating handles");
        AtDeleteTarget(Target);
        return status;
    }

    for (i = 0; i < handles->NumberOfHandles; i++)
    {
        if (handles->Handles[i].UniqueProcessId == Target->ProcessItem->ProcessId &&
            (ULONG64)(ULONG_PTR)handles->Handles[i].HandleValue == handleValue)
        {
            entry = &handles->Handles[i];
            break;
        }
    }

    if (!entry)
    {
        AtSetToolError(Result, "not_found", STATUS_NOT_FOUND, L"pid %lu has no handle 0x%llx.", HandleToUlong(Target->ProcessItem->ProcessId), handleValue);
        PhFree(handles);
        AtDeleteTarget(Target);
        return STATUS_NOT_FOUND;
    }

    Target->HandleValue = entry->HandleValue;
    Target->HandleTypeIndex = entry->ObjectTypeIndex;
    Target->HandleAttributes = entry->HandleAttributes;
    Target->HandleObject = entry->Object;
    Target->Identity[2] = handleValue;
    Target->Identity[3] = (ULONG64)(ULONG_PTR)entry->Object;
    PhFree(handles);

    // Names come from the object itself, so the user is shown what the handle refers to.
    PhGetHandleInformation(
        Target->ProcessHandle,
        Target->HandleValue,
        Target->HandleTypeIndex,
        NULL,
        &typeName,
        NULL,
        &bestName
        );

    // Naming the object needs the object, which needs PROCESS_DUP_HANDLE or the driver. The type
    // does not: it is in the handle table entry, and a caller that only asked to read still gets it.
    if (!typeName)
        typeName = PhGetObjectTypeIndexName(Target->HandleTypeIndex);

    Target->HandleTypeName = typeName;
    Target->HandleObjectName = bestName;

    if (expectedType = AtGetArgumentString(Arguments, "type_name"))
    {
        if (!typeName || !PhEqualString(typeName, expectedType, TRUE))
        {
            AtSetToolError(
                Result,
                "identity_mismatch",
                STATUS_OBJECT_TYPE_MISMATCH,
                L"Handle 0x%llx is a %s handle, not %s. Re-list the handles and try again.",
                handleValue,
                PhGetStringOrDefault(typeName, L"(unknown type)"),
                PhGetString(expectedType)
                );
            PhDereferenceObject(expectedType);
            AtDeleteTarget(Target);
            return STATUS_OBJECT_TYPE_MISMATCH;
        }

        PhDereferenceObject(expectedType);
    }

    Target->Kind = AtTargetHandle;

    return STATUS_SUCCESS;
}

NTSTATUS AtpResolveConnectionTarget(
    _In_opt_ PVOID Arguments,
    _Out_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    PPH_NETWORK_ITEM networkItem;
    PPH_STRING local;
    PPH_STRING remote;

    status = AtResolveProcessTarget(Arguments, TRUE, 0, Target, Result);

    if (!NT_SUCCESS(status))
        return status;

    status = AtFindNetworkConnection(Arguments, Target->ProcessItem, &networkItem, Result);

    if (!NT_SUCCESS(status))
    {
        AtDeleteTarget(Target);
        return status;
    }

    local = AtFormatNetworkEndpoint(&networkItem->LocalEndpoint, networkItem->ProtocolType, networkItem->LocalScopeId, TRUE);
    remote = AtFormatNetworkEndpoint(&networkItem->RemoteEndpoint, networkItem->ProtocolType, networkItem->RemoteScopeId, TRUE);

    Target->Kind = AtTargetConnection;
    Target->NetworkItem = networkItem;
    Target->ConnectionText = PhFormatString(
        L"%s %s -> %s",
        AtProtocolTypeString(networkItem->ProtocolType),
        PhGetString(local),
        PhGetString(remote)
        );
    Target->Identity[2] = ((ULONG64)networkItem->ProtocolType << 32) | PhHashStringRefEx(&local->sr, TRUE, PH_STRING_HASH_X65599);
    Target->Identity[3] = PhHashStringRefEx(&remote->sr, TRUE, PH_STRING_HASH_X65599);

    PhDereferenceObject(local);
    PhDereferenceObject(remote);

    return STATUS_SUCCESS;
}

NTSTATUS AtpResolveTargetParameter(
    _In_ PCAT_TOOL Tool,
    _In_opt_ PVOID Arguments,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PPH_STRING value;
    PPH_STRING text = NULL;

    switch (Tool->Action)
    {
    case AtActionSetProcessPriority:
        {
            ULONG priorityClass;

            value = AtGetArgumentString(Arguments, "priority_class");

            if (!AtParsePriorityClass(value, &priorityClass))
            {
                AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"priority_class must be one of idle, below_normal, normal, above_normal, high, realtime.");
                PhClearReference(&value);
                return STATUS_INVALID_PARAMETER;
            }

            text = PhCreateString(AtPriorityClassString(priorityClass));
            PhClearReference(&value);
        }
        break;
    case AtActionSetProcessIoPriority:
        {
            IO_PRIORITY_HINT ioPriority;

            value = AtGetArgumentString(Arguments, "io_priority");

            if (!AtParseIoPriority(value, &ioPriority))
            {
                AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"io_priority must be one of very_low, low, normal, high.");
                PhClearReference(&value);
                return STATUS_INVALID_PARAMETER;
            }

            text = PhCreateString(AtIoPriorityString(ioPriority));
            PhClearReference(&value);
        }
        break;
    case AtActionSetServiceConfig:
        {
            text = AtFormatServiceConfigParameter(Arguments, Result);

            if (!text)
                return STATUS_INVALID_PARAMETER;
        }
        break;
    case AtActionCreateProcessMinidump:
        {
            value = AtGetArgumentString(Arguments, "path");

            if (!value || PhDetermineDosPathNameType(&value->sr) != RtlPathTypeDriveAbsolute)
            {
                AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"path is required and must be an absolute drive path such as C:\\dumps\\process.dmp.");
                PhClearReference(&value);
                return STATUS_INVALID_PARAMETER;
            }

            text = value;
        }
        break;
    default:
        return STATUS_SUCCESS;
    }

    PhMoveReference(&Target->Parameter, text);
    Target->Identity[3] ^= (ULONG64)PhHashStringRefEx(&text->sr, TRUE, PH_STRING_HASH_X65599) << 32;

    return STATUS_SUCCESS;
}

NTSTATUS AtResolveTarget(
    _In_ PCAT_TOOL Tool,
    _In_opt_ PVOID Arguments,
    _Out_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    PCAT_ACTION_INFO action = &AtActionInfo[Tool->Action];
    BOOLEAN requireSequenceNumber = Tool->Tier == AtTierWrite;

    memset(Target, 0, sizeof(AT_TARGET));

    switch (action->TargetKind)
    {
    case AtTargetNone:
        status = STATUS_SUCCESS;
        break;
    case AtTargetProcess:
        status = AtResolveProcessTarget(Arguments, requireSequenceNumber, action->TargetAccess, Target, Result);
        break;
    case AtTargetThread:
        status = AtpResolveThreadTarget(Arguments, requireSequenceNumber, action->TargetAccess, Target, Result);
        break;
    case AtTargetService:
        status = AtpResolveServiceTarget(Arguments, action->TargetAccess, Target, Result);
        break;
    case AtTargetHandle:
        status = AtResolveHandleTarget(Arguments, requireSequenceNumber, action->TargetAccess, Target, Result);
        break;
    case AtTargetConnection:
        status = AtpResolveConnectionTarget(Arguments, Target, Result);
        break;
    default:
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    if (!NT_SUCCESS(status))
        return status;

    status = AtpResolveTargetParameter(Tool, Arguments, Target, Result);

    if (!NT_SUCCESS(status))
    {
        AtDeleteTarget(Target);
        return status;
    }

    return STATUS_SUCCESS;
}

VOID AtDeleteTarget(
    _Inout_ PAT_TARGET Target
    )
{
    if (Target->ThreadHandle)
        NtClose(Target->ThreadHandle);
    if (Target->ProcessHandle)
        NtClose(Target->ProcessHandle);
    if (Target->ServiceHandle)
        PhCloseServiceHandle(Target->ServiceHandle);

    PhClearReference(&Target->ProcessItem);
    PhClearReference(&Target->ServiceItem);
    PhClearReference(&Target->HandleTypeName);
    PhClearReference(&Target->HandleObjectName);
    PhClearReference(&Target->ConnectionText);
    PhClearReference(&Target->Parameter);

    if (Target->NetworkItem)
        PhFree(Target->NetworkItem);

    memset(Target, 0, sizeof(AT_TARGET));
}

VOID AtSetTargetParameter(
    _Inout_ PAT_TARGET Target,
    _In_ PCWSTR Parameter
    )
{
    PhMoveReference(&Target->Parameter, PhCreateString(Parameter));
}

PPH_STRING AtpFormatProcessHeadline(
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    return PhFormatString(
        L"%s (PID %lu)",
        PhGetStringOrDefault(ProcessItem->ProcessName, L"(unnamed)"),
        HandleToUlong(ProcessItem->ProcessId)
        );
}

PPH_STRING AtFormatTargetHeadline(
    _In_ PAT_TARGET Target
    )
{
    PPH_STRING process = NULL;
    PPH_STRING result;

    if (Target->ProcessItem)
        process = AtpFormatProcessHeadline(Target->ProcessItem);

    switch (Target->Kind)
    {
    case AtTargetProcess:
        result = PhReferenceObject(process);
        break;
    case AtTargetThread:
        result = PhFormatString(L"thread %lu of %s", HandleToUlong(Target->ThreadId), PhGetString(process));
        break;
    case AtTargetService:
        if (Target->ServiceItem->DisplayName && !PhEqualString(Target->ServiceItem->DisplayName, Target->ServiceItem->Name, TRUE))
            result = PhFormatString(L"service %s (%s)", PhGetString(Target->ServiceItem->Name), PhGetString(Target->ServiceItem->DisplayName));
        else
            result = PhFormatString(L"service %s", PhGetString(Target->ServiceItem->Name));
        break;
    case AtTargetHandle:
        result = PhFormatString(
            L"handle 0x%Ix (%s) in %s",
            (ULONG_PTR)Target->HandleValue,
            PhGetStringOrDefault(Target->HandleTypeName, L"unknown type"),
            PhGetString(process)
            );
        break;
    case AtTargetConnection:
        result = PhFormatString(L"connection %s of %s", PhGetString(Target->ConnectionText), PhGetString(process));
        break;
    default:
        result = PhCreateString(L"(no target)");
        break;
    }

    PhClearReference(&process);

    return result;
}

VOID AtpAppendProcessDescription(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PhAppendFormatStringBuilder(Builder, L"\nSequence: %I64u", ProcessItem->ProcessSequenceNumber);
    PhAppendFormatStringBuilder(Builder, L"\nImage: %s", PhGetStringOrDefault(ProcessItem->FileName, L"(unknown)"));

    if (ProcessItem->VerifyResult == VrTrusted)
        PhAppendFormatStringBuilder(Builder, L"\nSigner: Trusted (%s)", PhGetStringOrDefault(ProcessItem->VerifySignerName, L"unknown"));
    else if (ProcessItem->VerifyResult == VrUnknown)
        PhAppendStringBuilder2(Builder, L"\nSigner: not verified");
    else
        PhAppendStringBuilder2(Builder, L"\nSigner: not trusted");

    if (ProcessItem->UserName)
        PhAppendFormatStringBuilder(Builder, L"\nUser: %s", PhGetString(ProcessItem->UserName));
}

PPH_STRING AtFormatTargetDescription(
    _In_ PAT_TARGET Target
    )
{
    PH_STRING_BUILDER builder;
    PPH_STRING headline;

    PhInitializeStringBuilder(&builder, 256);

    headline = AtFormatTargetHeadline(Target);
    PhAppendStringBuilder(&builder, &headline->sr);
    PhDereferenceObject(headline);

    switch (Target->Kind)
    {
    case AtTargetService:
        {
            PPH_SERVICE_ITEM serviceItem = Target->ServiceItem;

            PhAppendFormatStringBuilder(&builder, L"\nState: %s", PhGetServiceStateString(serviceItem->State)->Buffer);
            PhAppendFormatStringBuilder(&builder, L"\nStart type: %s", PhGetServiceStartTypeString(serviceItem->StartType)->Buffer);
            PhAppendFormatStringBuilder(&builder, L"\nImage: %s", PhGetStringOrDefault(serviceItem->FileName, L"(unknown)"));

            if (serviceItem->VerifyResult == VrTrusted)
                PhAppendFormatStringBuilder(&builder, L"\nSigner: Trusted (%s)", PhGetStringOrDefault(serviceItem->VerifySignerName, L"unknown"));
            else if (serviceItem->VerifyResult == VrUnknown)
                PhAppendStringBuilder2(&builder, L"\nSigner: not verified");
            else
                PhAppendStringBuilder2(&builder, L"\nSigner: not trusted");

            if (serviceItem->ProcessId)
                PhAppendFormatStringBuilder(&builder, L"\nPID: %lu", HandleToUlong(serviceItem->ProcessId));
        }
        break;
    case AtTargetHandle:
        {
            PhAppendFormatStringBuilder(&builder, L"\nObject: %s", PhGetStringOrDefault(Target->HandleObjectName, L"(unnamed)"));
            AtpAppendProcessDescription(&builder, Target->ProcessItem);
        }
        break;
    case AtTargetConnection:
        {
            PhAppendFormatStringBuilder(&builder, L"\nState: %s", PhGetTcpStateName(Target->NetworkItem->State)->Buffer);
            AtpAppendProcessDescription(&builder, Target->ProcessItem);
        }
        break;
    default:
        {
            if (Target->ProcessItem)
                AtpAppendProcessDescription(&builder, Target->ProcessItem);
        }
        break;
    }

    return PhFinalStringBuilderString(&builder);
}

PPH_STRING AtFormatTargetAudit(
    _In_ PAT_TARGET Target
    )
{
    PPH_STRING headline;
    PPH_STRING result;

    headline = AtFormatTargetHeadline(Target);

    if (Target->ProcessItem)
    {
        result = PhFormatString(
            L"%s [sequence %I64u, %s]",
            PhGetString(headline),
            Target->ProcessItem->ProcessSequenceNumber,
            PhGetStringOrDefault(Target->ProcessItem->FileName, L"unknown image")
            );
    }
    else if (Target->Kind == AtTargetService)
    {
        result = PhFormatString(
            L"%s [%s]",
            PhGetString(headline),
            PhGetStringOrDefault(Target->ServiceItem->FileName, L"unknown image")
            );
    }
    else
    {
        result = PhReferenceObject(headline);
    }

    PhDereferenceObject(headline);

    return result;
}
