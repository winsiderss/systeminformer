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

#define AT_STACK_DEFAULT_FRAMES 64
#define AT_STACK_MAXIMUM_FRAMES 512

PCWSTR AtpThreadStateString(
    _In_ KTHREAD_STATE State
    )
{
    static CONST PCWSTR names[] =
    {
        L"initialized",
        L"ready",
        L"running",
        L"standby",
        L"terminated",
        L"waiting",
        L"transition",
        L"deferred_ready",
        L"gate_wait",
        L"waiting_for_process_in_swap",
    };

    if ((ULONG)State < RTL_NUMBER_OF(names))
        return names[State];

    return NULL;
}

PCWSTR AtpWaitReasonString(
    _In_ KWAIT_REASON WaitReason
    )
{
    static CONST PCWSTR names[] =
    {
        L"Executive",
        L"FreePage",
        L"PageIn",
        L"PoolAllocation",
        L"DelayExecution",
        L"Suspended",
        L"UserRequest",
        L"WrExecutive",
        L"WrFreePage",
        L"WrPageIn",
        L"WrPoolAllocation",
        L"WrDelayExecution",
        L"WrSuspended",
        L"WrUserRequest",
        L"WrEventPair",
        L"WrQueue",
        L"WrLpcReceive",
        L"WrLpcReply",
        L"WrVirtualMemory",
        L"WrPageOut",
        L"WrRendezvous",
        L"WrKeyedEvent",
        L"WrTerminated",
        L"WrProcessInSwap",
        L"WrCpuRateControl",
        L"WrCalloutStack",
        L"WrKernel",
        L"WrResource",
        L"WrPushLock",
        L"WrMutex",
        L"WrQuantumEnd",
        L"WrDispatchInt",
        L"WrPreempted",
        L"WrYieldExecution",
        L"WrFastMutex",
        L"WrGuardedMutex",
        L"WrRundown",
        L"WrAlertByThreadId",
        L"WrDeferredPreempt",
        L"WrPhysicalFault",
        L"WrIoRing",
        L"WrMdlCache",
        L"WrRcu",
    };

    if ((ULONG)WaitReason < RTL_NUMBER_OF(names))
        return names[WaitReason];

    return NULL;
}

PPH_SYMBOL_PROVIDER AtpCreateSymbolProvider(
    _In_ HANDLE ProcessId
    )
{
    PPH_SYMBOL_PROVIDER symbolProvider;

    if (!(symbolProvider = PhCreateSymbolProvider(ProcessId)))
        return NULL;

    PhLoadSymbolProviderOptions(symbolProvider);
    PhLoadSymbolProviderModules(symbolProvider, ProcessId);

    return symbolProvider;
}

VOID AtpGetProcessThreads(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    AT_TARGET target;
    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;
    PPH_SYMBOL_PROVIDER symbolProvider = NULL;
    PVOID structured;
    PVOID threads;
    ULONG count = 0;
    ULONG i;

    if (!NT_SUCCESS(AtResolveProcessTarget(Call->Arguments, FALSE, 0, &target, Result)))
        return;

    status = PhEnumProcessesEx(&processes, SystemExtendedProcessInformation);

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Enumerating threads");
        AtDeleteTarget(&target);
        return;
    }

    if (!(process = PhFindProcessInformation(processes, target.ProcessItem->ProcessId)))
    {
        AtSetToolError(Result, "not_found", STATUS_NOT_FOUND, L"pid %lu has exited.", HandleToUlong(target.ProcessItem->ProcessId));
        PhFree(processes);
        AtDeleteTarget(&target);
        return;
    }

    if (AtJsonGetObjectBoolean(Call->Arguments, "resolve_start_addresses"))
        symbolProvider = AtpCreateSymbolProvider(target.ProcessItem->ProcessId);

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, target.ProcessItem);
    threads = PhCreateJsonArray();

    for (i = 0; i < process->NumberOfThreads; i++)
    {
        PSYSTEM_EXTENDED_THREAD_INFORMATION thread = &((PSYSTEM_EXTENDED_THREAD_INFORMATION)process->Threads)[i];
        PVOID row;
        HANDLE threadHandle;
        PPH_STRING name = NULL;
        PVOID startAddress;
        LARGE_INTEGER createTime;

        row = PhCreateJsonObject();
        PhAddJsonObjectUInt64(row, "tid", HandleToUlong(thread->ThreadInfo.ClientId.UniqueThread));

        if (NT_SUCCESS(PhOpenThread(&threadHandle, THREAD_QUERY_LIMITED_INFORMATION, thread->ThreadInfo.ClientId.UniqueThread)))
        {
            PhGetThreadName(threadHandle, &name);
            NtClose(threadHandle);
        }

        AtJsonAddString(row, "name", name);
        PhClearReference(&name);

        AtJsonAddStringZ(row, "state", AtpThreadStateString(thread->ThreadInfo.ThreadState));

        if (thread->ThreadInfo.ThreadState == Waiting)
            AtJsonAddStringZ(row, "wait_reason", AtpWaitReasonString(thread->ThreadInfo.WaitReason));
        else
            AtJsonAddNull(row, "wait_reason");

        PhAddJsonObjectInt64(row, "priority", thread->ThreadInfo.Priority);
        PhAddJsonObjectInt64(row, "base_priority", thread->ThreadInfo.BasePriority);

        startAddress = thread->Win32StartAddress ? thread->Win32StartAddress : thread->ThreadInfo.StartAddress;
        AtJsonAddPointer(row, "start_address", startAddress);

        if (symbolProvider && startAddress)
        {
            PPH_STRING symbol;

            if (symbol = PhGetSymbolFromAddress(symbolProvider, startAddress, NULL, NULL, NULL, NULL))
            {
                AtJsonAddString(row, "start_address_symbol", symbol);
                PhDereferenceObject(symbol);
            }
            else
            {
                AtJsonAddNull(row, "start_address_symbol");
            }
        }
        else
        {
            AtJsonAddNull(row, "start_address_symbol");
        }

        createTime.QuadPart = thread->ThreadInfo.CreateTime.QuadPart;
        AtJsonAddTime(row, "create_time", &createTime);
        AtJsonAddDuration(row, "kernel_time", thread->ThreadInfo.KernelTime.QuadPart);
        AtJsonAddDuration(row, "user_time", thread->ThreadInfo.UserTime.QuadPart);
        PhAddJsonObjectUInt64(row, "context_switches", thread->ThreadInfo.ContextSwitches);
        PhAddJsonObjectBoolean(
            row,
            "is_suspended",
            thread->ThreadInfo.ThreadState == Waiting &&
            (thread->ThreadInfo.WaitReason == Suspended || thread->ThreadInfo.WaitReason == WrSuspended)
            );

        PhAddJsonArrayObject(threads, row);
        count++;
    }

    PhAddJsonObjectValue(structured, "threads", threads);
    PhAddJsonObjectUInt64(structured, "count", count);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    if (symbolProvider)
        PhDereferenceObject(symbolProvider);

    PhFree(processes);
    AtDeleteTarget(&target);
}

typedef struct _AT_STACK_CONTEXT
{
    PPH_SYMBOL_PROVIDER SymbolProvider;
    PVOID Frames;
    ULONG Count;
    ULONG MaximumFrames;
    BOOLEAN Truncated;
} AT_STACK_CONTEXT, *PAT_STACK_CONTEXT;

_Function_class_(PH_WALK_THREAD_STACK_CALLBACK)
BOOLEAN NTAPI AtpStackFrameCallback(
    _In_ PPH_THREAD_STACK_FRAME StackFrame,
    _In_opt_ PVOID Context
    )
{
    PAT_STACK_CONTEXT context = Context;
    PVOID row;
    PPH_STRING symbol;
    PPH_STRING fileName = NULL;

    if (context->Count >= context->MaximumFrames)
    {
        context->Truncated = TRUE;
        return FALSE;
    }

    row = PhCreateJsonObject();
    PhAddJsonObjectUInt64(row, "index", context->Count);
    AtJsonAddPointer(row, "pc", StackFrame->PcAddress);
    AtJsonAddPointer(row, "return_address", StackFrame->ReturnAddress);
    AtJsonAddPointer(row, "frame_address", StackFrame->FrameAddress);
    AtJsonAddPointer(row, "stack_address", StackFrame->StackAddress);

    if (symbol = PhGetSymbolFromAddress(context->SymbolProvider, StackFrame->PcAddress, NULL, &fileName, NULL, NULL))
    {
        AtJsonAddString(row, "symbol", symbol);
        PhDereferenceObject(symbol);
    }
    else
    {
        AtJsonAddNull(row, "symbol");
    }

    AtJsonAddString(row, "module", fileName);
    PhClearReference(&fileName);

    PhAddJsonObjectBoolean(row, "is_kernel", !!FlagOn(StackFrame->Flags, PH_THREAD_STACK_FRAME_KERNEL));

    PhAddJsonArrayObject(context->Frames, row);
    context->Count++;

    return TRUE;
}

VOID AtpGetThreadStack(
    _In_ PAT_TOOL_CALL Call,
    _In_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    AT_STACK_CONTEXT context;
    HANDLE processHandle = NULL;
    CLIENT_ID clientId;
    ULONG64 maxFrames;
    PVOID structured;

    memset(&context, 0, sizeof(AT_STACK_CONTEXT));
    context.MaximumFrames = AT_STACK_DEFAULT_FRAMES;

    if (AtGetArgumentUInt64(Call->Arguments, "max_frames", &maxFrames) && maxFrames > 0)
        context.MaximumFrames = (ULONG)min(maxFrames, AT_STACK_MAXIMUM_FRAMES);

    if (!(context.SymbolProvider = AtpCreateSymbolProvider(Target->ProcessItem->ProcessId)))
    {
        AtSetToolError(Result, "failed", STATUS_UNSUCCESSFUL, L"The symbol provider could not be created.");
        return;
    }

    // Symbols and the user stack need to read the process; the walk tolerates not having it.
    PhOpenProcess(&processHandle, PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, Target->ProcessItem->ProcessId);

    clientId.UniqueProcess = Target->ProcessItem->ProcessId;
    clientId.UniqueThread = Target->ThreadId;
    context.Frames = PhCreateJsonArray();

    status = PhWalkThreadStack(
        Target->ThreadHandle,
        processHandle,
        &clientId,
        context.SymbolProvider,
        PH_WALK_USER_STACK | PH_WALK_USER_WOW64_STACK | PH_WALK_KERNEL_STACK,
        AtpStackFrameCallback,
        &context
        );

    if (processHandle)
        NtClose(processHandle);

    PhDereferenceObject(context.SymbolProvider);

    if (!NT_SUCCESS(status) && context.Count == 0)
    {
        AtSetToolStatusError(Result, status, L"Walking the thread stack");
        PhFreeJsonObject(context.Frames);
        return;
    }

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, Target->ProcessItem);
    PhAddJsonObjectUInt64(structured, "tid", HandleToUlong(Target->ThreadId));
    PhAddJsonObjectValue(structured, "frames", context.Frames);
    PhAddJsonObjectUInt64(structured, "count", context.Count);
    PhAddJsonObjectBoolean(structured, "truncated", context.Truncated);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

VOID AtpControlThread(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    PVOID structured;

    switch (Tool->Action)
    {
    case AtActionSuspendThread:
        status = PhSuspendThread(Target->ThreadHandle, NULL);
        break;
    case AtActionResumeThread:
        status = PhResumeThread(Target->ThreadHandle, NULL);
        break;
    case AtActionTerminateThread:
        status = PhTerminateThread(Target->ThreadHandle, STATUS_SUCCESS);
        break;
    default:
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"The operation");
        return;
    }

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, Target->ProcessItem);
    PhAddJsonObjectUInt64(structured, "tid", HandleToUlong(Target->ThreadId));
    PhAddJsonObject(structured, "action", Tool->Name);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

VOID AtThreadInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    switch (Tool->Action)
    {
    case AtActionGetProcessThreads:
        AtpGetProcessThreads(Call, Result);
        break;
    case AtActionGetThreadStack:
        AtpGetThreadStack(Call, Target, Result);
        break;
    case AtActionSuspendThread:
    case AtActionResumeThread:
    case AtActionTerminateThread:
        AtpControlThread(Tool, Target, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
