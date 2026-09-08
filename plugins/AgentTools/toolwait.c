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
#include <wct.h>

// WCTP_GETINFO_ALL_FLAGS in the SDK header leaves out network I/O, so the flags are spelled out
// here the way ExtendedTools does: a thread blocked on an SMB or socket wait is exactly the kind of
// hang this answers.
#define AT_WCT_GETINFO_ALL_FLAGS \
    (WCT_OUT_OF_PROC_FLAG | WCT_OUT_OF_PROC_COM_FLAG | WCT_OUT_OF_PROC_CS_FLAG | WCT_NETWORK_IO_FLAG)

#define AT_WAIT_CHAIN_DEFAULT_THREADS 64
#define AT_WAIT_CHAIN_MAXIMUM_THREADS 512

// Who is waiting on whom. A thread blocked on a lock says nothing about which thread holds it, and a
// stack shows the wait but not the owner; the Wait Chain Traversal API walks the ownership edges the
// kernel and the window manager know about, across processes, and says when the chain closes into a
// cycle - which is a deadlock, not a slow call.

PCWSTR AtpWaitChainObjectTypeString(
    _In_ ULONG ObjectType
    )
{
    switch (ObjectType)
    {
    case WctCriticalSectionType:
        return L"critical_section";
    case WctSendMessageType:
        return L"send_message";
    case WctMutexType:
        return L"mutex";
    case WctAlpcType:
        return L"alpc";
    case WctComType:
        return L"com";
    case WctThreadWaitType:
        return L"thread_wait";
    case WctProcessWaitType:
        return L"process_wait";
    case WctThreadType:
        return L"thread";
    case WctComActivationType:
        return L"com_activation";
    case WctSocketIoType:
        return L"socket_io";
    case WctSmbIoType:
        return L"smb_io";
    case WctUnknownType:
        return L"unknown";
    }

    return NULL;
}

PCWSTR AtpWaitChainObjectStatusString(
    _In_ ULONG ObjectStatus
    )
{
    switch (ObjectStatus)
    {
    case WctStatusNoAccess:
        return L"no_access";
    case WctStatusRunning:
        return L"running";
    case WctStatusBlocked:
        return L"blocked";
    case WctStatusPidOnly:
        return L"pid_only";
    case WctStatusPidOnlyRpcss:
        return L"pid_only_rpcss";
    case WctStatusOwned:
        return L"owned";
    case WctStatusNotOwned:
        return L"not_owned";
    case WctStatusAbandoned:
        return L"abandoned";
    case WctStatusUnknown:
        return L"unknown";
    case WctStatusError:
        return L"error";
    }

    return NULL;
}

// The name is a fixed-size buffer the API fills, and nothing promises it is terminated.
PPH_STRING AtpWaitChainObjectName(
    _In_ PWAITCHAIN_NODE_INFO Node
    )
{
    SIZE_T count;

    for (count = 0; count < RTL_NUMBER_OF(Node->LockObject.ObjectName); count++)
    {
        if (Node->LockObject.ObjectName[count] == UNICODE_NULL)
            break;
    }

    if (count == 0)
        return NULL;

    return PhCreateStringEx(Node->LockObject.ObjectName, count * sizeof(WCHAR));
}

PVOID AtpCreateWaitChainNode(
    _In_ PWAITCHAIN_NODE_INFO Node,
    _In_ ULONG Index
    )
{
    PVOID row;

    row = PhCreateJsonObject();
    PhAddJsonObjectUInt64(row, "index", Index);
    AtJsonAddStringZ(row, "object_type", AtpWaitChainObjectTypeString(Node->ObjectType));
    AtJsonAddStringZ(row, "object_status", AtpWaitChainObjectStatusString(Node->ObjectStatus));

    // A thread node carries the client id of the thread the chain reached; every other kind of node
    // is the object being waited on, and only that one has a name.
    if (Node->ObjectType == WctThreadType)
    {
        PPH_PROCESS_ITEM processItem;

        PhAddJsonObjectUInt64(row, "pid", Node->ThreadObject.ProcessId);
        PhAddJsonObjectUInt64(row, "tid", Node->ThreadObject.ThreadId);
        PhAddJsonObjectUInt64(row, "wait_milliseconds", Node->ThreadObject.WaitTime);
        PhAddJsonObjectUInt64(row, "context_switches", Node->ThreadObject.ContextSwitches);

        if (processItem = PhReferenceProcessItem(UlongToHandle(Node->ThreadObject.ProcessId)))
        {
            AtJsonAddString(row, "process_name", processItem->ProcessName);
            PhDereferenceObject(processItem);
        }
        else
        {
            AtJsonAddNull(row, "process_name");
        }

        AtJsonAddNull(row, "name");
    }
    else
    {
        PPH_STRING name = AtpWaitChainObjectName(Node);

        AtJsonAddString(row, "name", name);
        PhClearReference(&name);

        AtJsonAddNull(row, "pid");
        AtJsonAddNull(row, "tid");
        AtJsonAddNull(row, "process_name");
        AtJsonAddNull(row, "wait_milliseconds");
        AtJsonAddNull(row, "context_switches");
    }

    return row;
}

typedef struct _AT_WAIT_CHAIN_CONTEXT
{
    HWCT SessionHandle;
    PVOID Rows;
    ULONG Count;
    ULONG TotalCount;
    ULONG MaximumThreads;
    ULONG DeadlockCount;
} AT_WAIT_CHAIN_CONTEXT, *PAT_WAIT_CHAIN_CONTEXT;

VOID AtpAddThreadWaitChain(
    _Inout_ PAT_WAIT_CHAIN_CONTEXT Context,
    _In_ HANDLE ThreadId
    )
{
    WAITCHAIN_NODE_INFO nodes[WCT_MAX_NODE_COUNT];
    ULONG nodeCount = WCT_MAX_NODE_COUNT;
    BOOL isCycle = FALSE;
    PVOID row;

    memset(nodes, 0, sizeof(nodes));

    row = PhCreateJsonObject();
    PhAddJsonObjectUInt64(row, "tid", HandleToUlong(ThreadId));

    if (GetThreadWaitChain(
        Context->SessionHandle,
        0,
        AT_WCT_GETINFO_ALL_FLAGS,
        HandleToUlong(ThreadId),
        &nodeCount,
        nodes,
        &isCycle
        ))
    {
        PVOID array;
        ULONG i;

        // The API reports what it found, which can be more than it was given room for.
        PhAddJsonObjectBoolean(row, "truncated", nodeCount > WCT_MAX_NODE_COUNT);
        nodeCount = min(nodeCount, WCT_MAX_NODE_COUNT);

        array = PhCreateJsonArray();

        for (i = 0; i < nodeCount; i++)
            PhAddJsonArrayObject(array, AtpCreateWaitChainNode(&nodes[i], i));

        PhAddJsonObjectValue(row, "nodes", array);
        PhAddJsonObjectUInt64(row, "node_count", nodeCount);
        PhAddJsonObjectBoolean(row, "is_deadlocked", !!isCycle);
        AtJsonAddNull(row, "error");
        AtJsonAddNull(row, "message");

        if (isCycle)
            Context->DeadlockCount++;
    }
    else
    {
        NTSTATUS status = PhDosErrorToNtStatus(PhGetLastError());
        PPH_STRING message = PhGetStatusMessage(status, 0);

        // One thread the chain could not be read for is one row that says so: a thread that exits
        // while the process is being looked at is ordinary, and the other chains still answer.
        PhAddJsonObjectValue(row, "nodes", PhCreateJsonArray());
        PhAddJsonObjectUInt64(row, "node_count", 0);
        PhAddJsonObjectBoolean(row, "is_deadlocked", FALSE);
        PhAddJsonObjectBoolean(row, "truncated", FALSE);
        PhAddJsonObject(row, "error", status == STATUS_ACCESS_DENIED ? "access_denied" : "failed");
        AtJsonAddStringZ(row, "message", PhGetStringOrDefault(message, L"unknown error"));
        PhClearReference(&message);
    }

    PhAddJsonArrayObject(Context->Rows, row);
    Context->Count++;
}

_Function_class_(PH_ENUM_NEXT_THREAD)
NTSTATUS NTAPI AtpWaitChainThreadCallback(
    _In_ HANDLE ThreadHandle,
    _In_opt_ PVOID Context
    )
{
    PAT_WAIT_CHAIN_CONTEXT context = Context;
    THREAD_BASIC_INFORMATION basicInformation;

    if (!context)
        return STATUS_SUCCESS;

    context->TotalCount++;

    if (context->Count >= context->MaximumThreads)
        return STATUS_SUCCESS;

    if (NT_SUCCESS(PhGetThreadBasicInformation(ThreadHandle, &basicInformation)))
        AtpAddThreadWaitChain(context, basicInformation.ClientId.UniqueThread);

    return STATUS_SUCCESS;
}

VOID AtpGetThreadWaitChain(
    _In_ PAT_TOOL_CALL Call,
    _In_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    AT_WAIT_CHAIN_CONTEXT context;
    ULONG64 threadId = 0;
    ULONG64 maximumThreads = AT_WAIT_CHAIN_DEFAULT_THREADS;
    PVOID structured;

    memset(&context, 0, sizeof(AT_WAIT_CHAIN_CONTEXT));

    if (AtGetArgumentUInt64(Call->Arguments, "max_threads", &maximumThreads) && maximumThreads == 0)
        maximumThreads = AT_WAIT_CHAIN_DEFAULT_THREADS;

    context.MaximumThreads = (ULONG)min(maximumThreads, AT_WAIT_CHAIN_MAXIMUM_THREADS);

    // A synchronous session: the call blocks until the chain is walked, which is what a tool that
    // has to answer in one message wants.
    if (!(context.SessionHandle = OpenThreadWaitChainSession(0, NULL)))
    {
        AtSetToolStatusError(Result, PhDosErrorToNtStatus(PhGetLastError()), L"Opening a wait chain session");
        return;
    }

    context.Rows = PhCreateJsonArray();

    if (AtGetArgumentUInt64(Call->Arguments, "tid", &threadId) && threadId != 0)
    {
        HANDLE threadHandle;
        CLIENT_ID clientId;

        // A tid is only meaningful inside the process it was given with, and tids are reused, so
        // the thread is opened by client id - which the kernel refuses if it is not that process's
        // thread - rather than passed straight to an API that takes a bare tid.
        clientId.UniqueProcess = Target->ProcessItem->ProcessId;
        clientId.UniqueThread = UlongToHandle((ULONG)threadId);

        status = PhOpenThreadClientId(
            &threadHandle,
            THREAD_QUERY_LIMITED_INFORMATION,
            &clientId
            );

        if (!NT_SUCCESS(status))
        {
            AtSetToolStatusError(Result, status, L"Opening the thread");
            CloseThreadWaitChainSession(context.SessionHandle);
            PhFreeJsonObject(context.Rows);
            return;
        }

        NtClose(threadHandle);

        context.TotalCount = 1;
        AtpAddThreadWaitChain(&context, UlongToHandle((ULONG)threadId));
    }
    else
    {
        status = PhEnumNextThread(
            Target->ProcessHandle,
            NULL,
            THREAD_QUERY_LIMITED_INFORMATION,
            AtpWaitChainThreadCallback,
            &context
            );

        if (!NT_SUCCESS(status) && context.Count == 0)
        {
            AtSetToolStatusError(Result, status, L"Enumerating the threads");
            CloseThreadWaitChainSession(context.SessionHandle);
            PhFreeJsonObject(context.Rows);
            return;
        }
    }

    CloseThreadWaitChainSession(context.SessionHandle);

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, Target->ProcessItem);
    PhAddJsonObjectValue(structured, "threads", context.Rows);
    PhAddJsonObjectUInt64(structured, "count", context.Count);
    PhAddJsonObjectUInt64(structured, "total_count", context.TotalCount);
    PhAddJsonObjectBoolean(structured, "truncated", context.Count < context.TotalCount);
    PhAddJsonObjectUInt64(structured, "deadlocked_count", context.DeadlockCount);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

VOID AtWaitInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    switch (Tool->Action)
    {
    case AtActionGetThreadWaitChain:
        AtpGetThreadWaitChain(Call, Target, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
