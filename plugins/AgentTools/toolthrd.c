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
#include <mapimg.h>

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

PPH_SYMBOL_PROVIDER AtCreateSymbolProvider(
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

typedef struct _AT_THREAD_MODULE
{
    ULONG_PTR Base;
    ULONG_PTR End;
    PPH_STRING FileName;
} AT_THREAD_MODULE, *PAT_THREAD_MODULE;

_Function_class_(PH_ENUM_GENERIC_MODULES_CALLBACK)
BOOLEAN NTAPI AtpThreadModuleCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_opt_ PVOID Context
    )
{
    PPH_LIST list = Context;
    PAT_THREAD_MODULE entry;

    if (!list || !Module->BaseAddress || !Module->Size)
        return TRUE;

    entry = PhAllocate(sizeof(AT_THREAD_MODULE));
    entry->Base = (ULONG_PTR)Module->BaseAddress;
    entry->End = entry->Base + Module->Size;
    PhSetReference(&entry->FileName, Module->FileName);
    PhAddItemList(list, entry);

    return TRUE;
}

PPH_LIST AtpCreateThreadModuleList(
    _In_ HANDLE ProcessId,
    _Out_ PBOOLEAN Complete
    )
{
    PPH_LIST list;

    list = PhCreateList(64);
    *Complete = NT_SUCCESS(PhEnumGenericModules(ProcessId, NULL, PH_ENUM_GENERIC_MAPPED_IMAGES, AtpThreadModuleCallback, list));

    return list;
}

VOID AtpDestroyThreadModuleList(
    _In_ PPH_LIST List
    )
{
    ULONG i;

    for (i = 0; i < List->Count; i++)
    {
        PAT_THREAD_MODULE entry = List->Items[i];

        PhClearReference(&entry->FileName);
        PhFree(entry);
    }

    PhDereferenceObject(List);
}

PPH_STRING AtpFindThreadModule(
    _In_opt_ PPH_LIST List,
    _In_opt_ PVOID Address
    )
{
    ULONG i;

    if (!List || !Address)
        return NULL;

    for (i = 0; i < List->Count; i++)
    {
        PAT_THREAD_MODULE entry = List->Items[i];

        if ((ULONG_PTR)Address >= entry->Base && (ULONG_PTR)Address < entry->End)
            return entry->FileName;
    }

    return NULL;
}

PCWSTR AtpResolveLevelString(
    _In_ PH_SYMBOL_RESOLVE_LEVEL Level
    )
{
    switch (Level)
    {
    case PhsrlFunction:
        return L"function";
    case PhsrlModule:
        return L"module";
    case PhsrlAddress:
        return L"address";
    }

    return NULL;
}

PVOID AtpQueryThreadStartAddress(
    _In_ HANDLE ThreadId
    )
{
    HANDLE threadHandle;
    PVOID startAddress = NULL;

    // The information class wants full query access on some builds; limited is the fallback.
    if (!NT_SUCCESS(PhOpenThread(&threadHandle, THREAD_QUERY_INFORMATION, ThreadId)) &&
        !NT_SUCCESS(PhOpenThread(&threadHandle, THREAD_QUERY_LIMITED_INFORMATION, ThreadId)))
    {
        return NULL;
    }

    if (!NT_SUCCESS(NtQueryInformationThread(
        threadHandle,
        ThreadQuerySetWin32StartAddress,
        &startAddress,
        sizeof(PVOID),
        NULL
        )))
    {
        startAddress = NULL;
    }

    NtClose(threadHandle);

    return startAddress;
}

VOID AtpAddThreadDetails(
    _In_ PVOID Row,
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_opt_ HANDLE ProcessHandle
    )
{
    HANDLE threadHandle;
    THREAD_BASIC_INFORMATION basicInfo;
    THREAD_CYCLE_TIME_INFORMATION cycleTime;
    THREAD_LAST_SYSCALL_INFORMATION lastSystemCall;
    PROCESSOR_NUMBER idealProcessor;
    IO_PRIORITY_HINT ioPriority;
    ULONG pagePriority;
    ULONG ioPending;
    GUITHREADINFO guiThreadInfo;

    // GetGUIThreadInfo needs no handle at all and answers the question a stack alone does not:
    // whether this thread owns windows.
    memset(&guiThreadInfo, 0, sizeof(GUITHREADINFO));
    guiThreadInfo.cbSize = sizeof(GUITHREADINFO);
    PhAddJsonObjectBoolean(Row, "is_gui_thread", !!GetGUIThreadInfo(HandleToUlong(ThreadId), &guiThreadInfo));

    // The last system call and the pending-I/O flag need full query access; everything else is
    // happy with limited, so fall back rather than lose the whole row.
    if (!NT_SUCCESS(PhOpenThread(&threadHandle, THREAD_QUERY_INFORMATION, ThreadId)) &&
        !NT_SUCCESS(PhOpenThread(&threadHandle, THREAD_QUERY_LIMITED_INFORMATION, ThreadId)))
    {
        AtJsonAddNull(Row, "affinity");
        AtJsonAddNull(Row, "ideal_processor");
        AtJsonAddNull(Row, "io_priority");
        AtJsonAddNull(Row, "page_priority");
        AtJsonAddNull(Row, "cycle_time");
        AtJsonAddNull(Row, "last_system_call");
        AtJsonAddNull(Row, "io_pending");
        AtJsonAddNull(Row, "service_name");
        return;
    }

    if (NT_SUCCESS(PhGetThreadBasicInformation(threadHandle, &basicInfo)))
        AtJsonAddHex(Row, "affinity", basicInfo.AffinityMask);
    else
        AtJsonAddNull(Row, "affinity");

    if (NT_SUCCESS(NtQueryInformationThread(threadHandle, ThreadIdealProcessorEx, &idealProcessor, sizeof(PROCESSOR_NUMBER), NULL)))
    {
        PVOID entry = PhCreateJsonObject();

        PhAddJsonObjectUInt64(entry, "group", idealProcessor.Group);
        PhAddJsonObjectUInt64(entry, "number", idealProcessor.Number);
        PhAddJsonObjectValue(Row, "ideal_processor", entry);
    }
    else
    {
        AtJsonAddNull(Row, "ideal_processor");
    }

    if (NT_SUCCESS(PhGetThreadIoPriority(threadHandle, &ioPriority)))
        AtJsonAddStringZ(Row, "io_priority", AtIoPriorityString(ioPriority));
    else
        AtJsonAddNull(Row, "io_priority");

    if (NT_SUCCESS(PhGetThreadPagePriority(threadHandle, &pagePriority)))
        PhAddJsonObjectUInt64(Row, "page_priority", pagePriority);
    else
        AtJsonAddNull(Row, "page_priority");

    // The total, not a rate: computing a per-thread rate needs two samples, and the provider that
    // keeps those only runs while a process properties window is open. Compared against the other
    // threads of the same process it still says which one is doing the work.
    if (NT_SUCCESS(NtQueryInformationThread(threadHandle, ThreadCycleTime, &cycleTime, sizeof(THREAD_CYCLE_TIME_INFORMATION), NULL)))
        PhAddJsonObjectUInt64(Row, "cycle_time", cycleTime.AccumulatedCycles);
    else
        AtJsonAddNull(Row, "cycle_time");

    if (NT_SUCCESS(NtQueryInformationThread(threadHandle, ThreadLastSystemCall, &lastSystemCall, sizeof(THREAD_LAST_SYSCALL_INFORMATION), NULL)))
    {
        PVOID entry = PhCreateJsonObject();

        PhAddJsonObjectUInt64(entry, "number", lastSystemCall.SystemCallNumber);
        AtJsonAddDuration(entry, "wait_seconds", lastSystemCall.WaitTime);
        PhAddJsonObjectValue(Row, "last_system_call", entry);
    }
    else
    {
        AtJsonAddNull(Row, "last_system_call");
    }

    if (NT_SUCCESS(NtQueryInformationThread(threadHandle, ThreadIsIoPending, &ioPending, sizeof(ULONG), NULL)))
        PhAddJsonObjectBoolean(Row, "io_pending", !!ioPending);
    else
        AtJsonAddNull(Row, "io_pending");

    // Which service a thread belongs to inside a shared host, which is the only way to tell them
    // apart in svchost.
    if (ProcessHandle)
    {
        PVOID serviceTag;

        if (NT_SUCCESS(PhGetThreadServiceTag(threadHandle, ProcessHandle, &serviceTag)) && serviceTag)
        {
            PPH_STRING serviceName = PhGetServiceNameFromTag(ProcessId, serviceTag);

            AtJsonAddString(Row, "service_name", serviceName);
            PhClearReference(&serviceName);
        }
        else
        {
            AtJsonAddNull(Row, "service_name");
        }
    }
    else
    {
        AtJsonAddNull(Row, "service_name");
    }

    NtClose(threadHandle);
}

PVOID AtpCreateThreadsResult(
    _In_ PAT_TOOL_CALL Call,
    _In_ PVOID Processes,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ BOOLEAN Summary
    )
{
    PSYSTEM_PROCESS_INFORMATION process;
    PPH_SYMBOL_PROVIDER symbolProvider = NULL;
    HANDLE processHandle = NULL;
    PPH_LIST moduleList = NULL;
    BOOLEAN modulesComplete = TRUE;
    BOOLEAN includeDetails;
    AT_ROWS rows;
    PVOID structured;
    ULONG i;

    if (!(process = PhFindProcessInformation(Processes, ProcessItem->ProcessId)))
        return NULL;

    includeDetails = !Summary && AtJsonGetObjectBoolean(Call->Arguments, "include_details");

    if (includeDetails)
        moduleList = AtpCreateThreadModuleList(ProcessItem->ProcessId, &modulesComplete);

    // One process handle for the whole walk; the service tag query needs it.
    if (includeDetails && PH_IS_REAL_PROCESS_ID(ProcessItem->ProcessId))
        PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, ProcessItem->ProcessId);

    if (!Summary && AtJsonGetObjectBoolean(Call->Arguments, "resolve_start_addresses"))
        symbolProvider = AtCreateSymbolProvider(ProcessItem->ProcessId);

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, ProcessItem);
    AtInitializeRows(&rows, Call->Arguments);

    for (i = 0; i < process->NumberOfThreads; i++)
    {
        PSYSTEM_EXTENDED_THREAD_INFORMATION thread = &((PSYSTEM_EXTENDED_THREAD_INFORMATION)process->Threads)[i];
        PVOID row;
        HANDLE threadHandle;
        PPH_STRING name = NULL;
        PVOID startAddress;
        LARGE_INTEGER createTime;

        if (Summary)
        {
            // A triage pass wants the count, not a row per thread.
            AtAddRow(&rows, NULL);
            continue;
        }

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

        if (!startAddress)
            startAddress = AtpQueryThreadStartAddress(thread->ThreadInfo.ClientId.UniqueThread);

        AtJsonAddPointer(row, "start_address", startAddress);

        if (symbolProvider && startAddress)
        {
            PPH_STRING symbol;
            PH_SYMBOL_RESOLVE_LEVEL resolveLevel = PhsrlInvalid;
            PPH_STRING moduleName = NULL;

            // The resolve level says how much of the name to believe: a module plus offset is a
            // fact, a symbol name came from a file on disk that the process does not have to match.
            if (symbol = PhGetSymbolFromAddress(symbolProvider, startAddress, &resolveLevel, &moduleName, NULL, NULL))
            {
                AtJsonAddString(row, "start_address_symbol", symbol);
                PhDereferenceObject(symbol);
            }
            else
            {
                AtJsonAddNull(row, "start_address_symbol");
            }

            AtJsonAddStringZ(row, "start_address_resolve_level", AtpResolveLevelString(resolveLevel));
            PhClearReference(&moduleName);
        }
        else
        {
            AtJsonAddNull(row, "start_address_symbol");
            AtJsonAddNull(row, "start_address_resolve_level");
        }

        // Independent of symbols. Null when include_details was not asked for, when the module list
        // could not be read, and when the address is in no module.
        AtJsonAddWin32FileName(row, "start_address_module", AtpFindThreadModule(moduleList, startAddress));

        createTime.QuadPart = thread->ThreadInfo.CreateTime.QuadPart;
        AtJsonAddTime(row, "create_time", &createTime);
        AtJsonAddDuration(row, "wait_seconds", thread->ThreadInfo.WaitTime);
        PhAddJsonObjectInt64(row, "priority_delta", thread->ThreadInfo.Priority - thread->ThreadInfo.BasePriority);
        AtJsonAddDuration(row, "kernel_time", thread->ThreadInfo.KernelTime.QuadPart);
        AtJsonAddDuration(row, "user_time", thread->ThreadInfo.UserTime.QuadPart);
        PhAddJsonObjectUInt64(row, "context_switches", thread->ThreadInfo.ContextSwitches);
        PhAddJsonObjectBoolean(
            row,
            "is_suspended",
            thread->ThreadInfo.ThreadState == Waiting &&
            (thread->ThreadInfo.WaitReason == Suspended || thread->ThreadInfo.WaitReason == WrSuspended)
            );

        if (includeDetails)
            AtpAddThreadDetails(row, ProcessItem->ProcessId, thread->ThreadInfo.ClientId.UniqueThread, processHandle);

        AtAddRow(&rows, row);
    }

    if (processHandle)
        NtClose(processHandle);

    if (moduleList)
        AtpDestroyThreadModuleList(moduleList);

    // start_address_module is null for a thread outside every module and for a list that could not
    // be read; this tells those apart when include_details was asked for.
    PhAddJsonObjectBoolean(structured, "modules_complete", modulesComplete);

    if (Summary)
    {
        PhAddJsonObjectUInt64(structured, "count", rows.TotalCount);
        AtDeleteRows(&rows);
    }
    else
    {
        AtAddRows(structured, "threads", &rows);
    }

    if (symbolProvider)
        PhDereferenceObject(symbolProvider);

    return structured;
}

VOID AtpGetProcessThreads(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    AT_BATCH batch;
    AT_TARGET target;
    PVOID processes;
    PVOID structured;

    if (!AtInitializeBatch(&batch, Call->Arguments, Result))
        return;

    status = PhEnumProcessesEx(&processes, SystemExtendedProcessInformation);

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Enumerating threads");
        return;
    }

    if (batch.Pids)
    {
        PVOID results = PhCreateJsonArray();
        ULONG i;

        for (i = 0; i < batch.Count; i++)
        {
            PPH_PROCESS_ITEM processItem;
            ULONG processId;
            PVOID entry = NULL;

            if (processItem = AtBatchReferenceProcessItem(&batch, i, &processId))
            {
                entry = AtpCreateThreadsResult(Call, processes, processItem, batch.Summary);
                PhDereferenceObject(processItem);
            }

            if (entry)
            {
                PhAddJsonArrayObject(results, entry);
            }
            else
            {
                PhAddJsonArrayObject(results, AtCreateBatchError(
                    processId,
                    "not_found",
                    L"This pid is not in the provider cache or has exited."
                    ));
            }
        }

        Result->StructuredContent = AtCreateBatchResult(results);
        PhFree(processes);
        return;
    }

    if (!NT_SUCCESS(AtResolveProcessTarget(Call->Arguments, FALSE, 0, &target, Result)))
    {
        PhFree(processes);
        return;
    }

    structured = AtpCreateThreadsResult(Call, processes, target.ProcessItem, FALSE);

    if (!structured)
    {
        AtSetToolError(Result, "not_found", STATUS_NOT_FOUND, L"pid %lu has exited.", HandleToUlong(target.ProcessItem->ProcessId));
        PhFree(processes);
        AtDeleteTarget(&target);
        return;
    }

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    PhFree(processes);
    AtDeleteTarget(&target);
}

typedef struct _AT_STACK_CONTEXT
{
    BOOLEAN ManagedSymbols;
    BOOLEAN IncludeLines;
    PPH_SYMBOL_PROVIDER SymbolProvider;
    PVOID Frames;
    ULONG Count;
    ULONG MaximumFrames;
    BOOLEAN Truncated;
} AT_STACK_CONTEXT, *PAT_STACK_CONTEXT;

VOID AtpBeginManagedSymbols(
    _Inout_ PAT_STACK_CONTEXT Context,
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ HANDLE ThreadHandle,
    _In_opt_ HANDLE ProcessHandle
    )
{
    PH_PLUGIN_THREAD_STACK_CONTROL control;
    BOOLEAN isWow64 = FALSE;

    if (!ProcessHandle)
        return;

#ifdef _WIN64
    if (!NT_SUCCESS(PhGetProcessIsWow64(ProcessHandle, &isWow64)) || isWow64)
        return;
#endif

    memset(&control, 0, sizeof(PH_PLUGIN_THREAD_STACK_CONTROL));
    control.Type = PluginThreadStackInitializing;
    control.UniqueKey = Context;
    control.u.Initializing.ProcessId = ProcessId;
    control.u.Initializing.ThreadId = ThreadId;
    control.u.Initializing.ThreadHandle = ThreadHandle;
    control.u.Initializing.ProcessHandle = ProcessHandle;
    control.u.Initializing.SymbolProvider = Context->SymbolProvider;
    control.u.Initializing.CustomWalk = FALSE;
    PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);

    // CustomWalk is ignored: this always walks the stack itself, which is the path the application
    // takes when a custom walk is unavailable, and the plugin still names every frame.
    memset(&control, 0, sizeof(PH_PLUGIN_THREAD_STACK_CONTROL));
    control.Type = PluginThreadStackBeginDefaultWalkStack;
    control.UniqueKey = Context;
    PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);

    Context->ManagedSymbols = TRUE;
}

VOID AtpEndManagedSymbols(
    _Inout_ PAT_STACK_CONTEXT Context
    )
{
    PH_PLUGIN_THREAD_STACK_CONTROL control;

    if (!Context->ManagedSymbols)
        return;

    memset(&control, 0, sizeof(PH_PLUGIN_THREAD_STACK_CONTROL));
    control.Type = PluginThreadStackEndDefaultWalkStack;
    control.UniqueKey = Context;
    PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);

    memset(&control, 0, sizeof(PH_PLUGIN_THREAD_STACK_CONTROL));
    control.Type = PluginThreadStackUninitializing;
    control.UniqueKey = Context;
    PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);
}

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
    BOOLEAN managed = FALSE;

    if (!context)
        return TRUE;

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

    symbol = PhGetSymbolFromAddress(context->SymbolProvider, StackFrame->PcAddress, NULL, &fileName, NULL, NULL);

    if (context->ManagedSymbols)
    {
        PH_PLUGIN_THREAD_STACK_CONTROL control;
        PPH_STRING nativeSymbol = symbol;

        // DotNetTools takes ownership of the symbol it is given and hands back what it wants
        // reported, so the returned pointer changing is what says the frame was managed.
        memset(&control, 0, sizeof(PH_PLUGIN_THREAD_STACK_CONTROL));
        control.Type = PluginThreadStackResolveSymbol;
        control.UniqueKey = context;
        control.u.ResolveSymbol.StackFrame = StackFrame;
        control.u.ResolveSymbol.Symbol = symbol;
        control.u.ResolveSymbol.FileName = fileName;

        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);

        managed = control.u.ResolveSymbol.Symbol != nativeSymbol;
        symbol = control.u.ResolveSymbol.Symbol;
        fileName = control.u.ResolveSymbol.FileName;
    }

    AtJsonAddString(row, "symbol", symbol);
    AtJsonAddString(row, "module", fileName);
    PhAddJsonObjectBoolean(row, "is_managed", managed);
    PhClearReference(&symbol);
    PhClearReference(&fileName);

    // Only a frame whose module has private symbols on the symbol path has a line at all, so most
    // frames answer null here even when the lookup is asked for.
    if (context->IncludeLines)
    {
        PPH_STRING lineFileName;
        PH_SYMBOL_LINE_INFORMATION lineInformation;

        if (PhGetLineFromAddress(context->SymbolProvider, StackFrame->PcAddress, &lineFileName, NULL, &lineInformation))
        {
            PVOID line = PhCreateJsonObject();

            AtJsonAddString(line, "file", lineFileName);
            PhAddJsonObjectUInt64(line, "number", lineInformation.LineNumber);
            PhAddJsonObjectValue(row, "line", line);
            PhClearReference(&lineFileName);
        }
        else
        {
            AtJsonAddNull(row, "line");
        }
    }

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

    context.IncludeLines = AtJsonGetObjectBoolean(Call->Arguments, "include_lines");

    if (!(context.SymbolProvider = AtCreateSymbolProvider(Target->ProcessItem->ProcessId)))
    {
        AtSetToolError(Result, "failed", STATUS_UNSUCCESSFUL, L"The symbol provider could not be created.");
        return;
    }

    // Symbols and the user stack need to read the process; the walk tolerates not having it.
    PhOpenProcess(&processHandle, PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, Target->ProcessItem->ProcessId);

    clientId.UniqueProcess = Target->ProcessItem->ProcessId;
    clientId.UniqueThread = Target->ThreadId;
    context.Frames = PhCreateJsonArray();

    AtpBeginManagedSymbols(
        &context,
        Target->ProcessItem->ProcessId,
        Target->ThreadId,
        Target->ThreadHandle,
        processHandle
        );

    status = PhWalkThreadStack(
        Target->ThreadHandle,
        processHandle,
        &clientId,
        context.SymbolProvider,
        PH_WALK_USER_STACK | PH_WALK_USER_WOW64_STACK | PH_WALK_KERNEL_STACK,
        AtpStackFrameCallback,
        &context
        );

    AtpEndManagedSymbols(&context);

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
    PhAddJsonObjectBoolean(structured, "managed_symbols", context.ManagedSymbols);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

#define AT_STACKS_DEFAULT_THREADS 8
#define AT_STACKS_MAXIMUM_THREADS 64

typedef struct _AT_STACK_THREAD
{
    HANDLE ThreadId;
    ULONG64 CpuTime;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    KTHREAD_STATE State;
    KWAIT_REASON WaitReason;
    ULONG WaitTime;
} AT_STACK_THREAD, *PAT_STACK_THREAD;

int __cdecl AtpCompareStackThreads(
    _In_ void* Context,
    _In_ const void* Elem1,
    _In_ const void* Elem2
    )
{
    PAT_STACK_THREAD thread1 = (PAT_STACK_THREAD)Elem1;
    PAT_STACK_THREAD thread2 = (PAT_STACK_THREAD)Elem2;
    PPH_STRING order = Context;

    if (order && PhEqualString2(order, L"tid", TRUE))
        return uint64cmp(HandleToUlong(thread1->ThreadId), HandleToUlong(thread2->ThreadId));

    if (order && PhEqualString2(order, L"newest", TRUE))
        return int64cmp(thread2->CreateTime.QuadPart, thread1->CreateTime.QuadPart);

    return uint64cmp(thread2->CpuTime, thread1->CpuTime);
}

VOID AtpAddStackThreadIdentity(
    _In_ PVOID Row,
    _In_ PAT_STACK_THREAD Thread
    )
{
    HANDLE threadHandle;
    PPH_STRING name = NULL;

    PhAddJsonObjectUInt64(Row, "tid", HandleToUlong(Thread->ThreadId));

    if (NT_SUCCESS(PhOpenThread(&threadHandle, THREAD_QUERY_LIMITED_INFORMATION, Thread->ThreadId)))
    {
        PhGetThreadName(threadHandle, &name);
        NtClose(threadHandle);
    }

    AtJsonAddString(Row, "name", name);
    PhClearReference(&name);

    AtJsonAddStringZ(Row, "state", AtpThreadStateString(Thread->State));

    if (Thread->State == Waiting)
        AtJsonAddStringZ(Row, "wait_reason", AtpWaitReasonString(Thread->WaitReason));
    else
        AtJsonAddNull(Row, "wait_reason");

    AtJsonAddDuration(Row, "wait_seconds", Thread->WaitTime);
    AtJsonAddDuration(Row, "kernel_time", Thread->KernelTime.QuadPart);
    AtJsonAddDuration(Row, "user_time", Thread->UserTime.QuadPart);
    AtJsonAddTime(Row, "create_time", &Thread->CreateTime);
}

VOID AtpGetProcessStacks(
    _In_ PAT_TOOL_CALL Call,
    _In_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    PVOID processes;
    SIZE_T threadsSize;
    PSYSTEM_PROCESS_INFORMATION process;
    PPH_SYMBOL_PROVIDER symbolProvider;
    PPH_STRING order;
    PAT_STACK_THREAD threads;
    ULONG threadCount;
    ULONG walkCount;
    ULONG64 maximumThreads = AT_STACKS_DEFAULT_THREADS;
    ULONG64 maximumFrames = AT_STACK_DEFAULT_FRAMES;
    BOOLEAN includeLines;
    PVOID array;
    PVOID structured;
    ULONG i;

    // Extended, because the thread array is read as SYSTEM_EXTENDED_THREAD_INFORMATION below and a
    // plain enumeration returns the smaller SYSTEM_THREAD_INFORMATION at a different stride.
    if (!NT_SUCCESS(status = PhEnumProcessesEx(&processes, SystemExtendedProcessInformation)))
    {
        AtSetToolStatusError(Result, status, L"Enumerating the processes");
        return;
    }

    if (!(process = PhFindProcessInformation(processes, Target->ProcessItem->ProcessId)))
    {
        AtSetToolError(Result, "not_found", STATUS_NOT_FOUND, L"The process is no longer running.");
        PhFree(processes);
        return;
    }

    if (AtGetArgumentUInt64(Call->Arguments, "max_threads", &maximumThreads) && maximumThreads == 0)
        maximumThreads = AT_STACKS_DEFAULT_THREADS;

    if (AtGetArgumentUInt64(Call->Arguments, "max_frames", &maximumFrames) && maximumFrames == 0)
        maximumFrames = AT_STACK_DEFAULT_FRAMES;

    maximumThreads = min(maximumThreads, AT_STACKS_MAXIMUM_THREADS);
    maximumFrames = min(maximumFrames, AT_STACK_MAXIMUM_FRAMES);
    includeLines = AtJsonGetObjectBoolean(Call->Arguments, "include_lines");
    order = AtGetArgumentString(Call->Arguments, "order");

    // The threads are taken out of the snapshot before any walking starts, so the selection is made
    // against one consistent view rather than against a process that is still creating threads.
    threadCount = process->NumberOfThreads;
    // threadCount comes from the snapshot, so the size is checked rather than assumed to fit.
    if (!NT_SUCCESS(RtlSizeTMult(sizeof(AT_STACK_THREAD), max(threadCount, 1), &threadsSize)))
    {
        AtSetToolError(Result, "failed", STATUS_INTEGER_OVERFLOW, L"That process has too many threads to walk.");
        PhFree(processes);
        return;
    }

    threads = PhAllocate(threadsSize);

    for (i = 0; i < threadCount; i++)
    {
        PSYSTEM_EXTENDED_THREAD_INFORMATION thread = &((PSYSTEM_EXTENDED_THREAD_INFORMATION)process->Threads)[i];

        threads[i].ThreadId = thread->ThreadInfo.ClientId.UniqueThread;
        threads[i].KernelTime = thread->ThreadInfo.KernelTime;
        threads[i].UserTime = thread->ThreadInfo.UserTime;
        threads[i].CpuTime = (ULONG64)(thread->ThreadInfo.KernelTime.QuadPart + thread->ThreadInfo.UserTime.QuadPart);
        threads[i].CreateTime = thread->ThreadInfo.CreateTime;
        threads[i].State = thread->ThreadInfo.ThreadState;
        threads[i].WaitReason = thread->ThreadInfo.WaitReason;
        threads[i].WaitTime = thread->ThreadInfo.WaitTime;
    }

    if (threadCount > 1)
        qsort_s(threads, threadCount, sizeof(AT_STACK_THREAD), AtpCompareStackThreads, order);

    walkCount = (ULONG)min(threadCount, maximumThreads);

    if (!(symbolProvider = AtCreateSymbolProvider(Target->ProcessItem->ProcessId)))
    {
        AtSetToolError(Result, "failed", STATUS_UNSUCCESSFUL, L"The symbol provider could not be created.");
        PhClearReference(&order);
        PhFree(threads);
        PhFree(processes);
        return;
    }

    array = PhCreateJsonArray();

    for (i = 0; i < walkCount; i++)
    {
        AT_STACK_CONTEXT context;
        CLIENT_ID clientId;
        HANDLE threadHandle;
        PVOID row;

        row = PhCreateJsonObject();
        AtpAddStackThreadIdentity(row, &threads[i]);

        memset(&context, 0, sizeof(AT_STACK_CONTEXT));
        context.SymbolProvider = symbolProvider;
        context.MaximumFrames = (ULONG)maximumFrames;
        context.IncludeLines = includeLines;
        context.Frames = PhCreateJsonArray();

        // A thread that cannot be opened or walked becomes one row that says so; failing the whole
        // call would throw away every other thread's stack.
        status = PhOpenThread(
            &threadHandle,
            THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME,
            threads[i].ThreadId
            );

        if (NT_SUCCESS(status))
        {
            clientId.UniqueProcess = Target->ProcessItem->ProcessId;
            clientId.UniqueThread = threads[i].ThreadId;

            AtpBeginManagedSymbols(
                &context,
                Target->ProcessItem->ProcessId,
                threads[i].ThreadId,
                threadHandle,
                Target->ProcessHandle
                );

            status = PhWalkThreadStack(
                threadHandle,
                Target->ProcessHandle,
                &clientId,
                symbolProvider,
                PH_WALK_USER_STACK | PH_WALK_USER_WOW64_STACK | PH_WALK_KERNEL_STACK,
                AtpStackFrameCallback,
                &context
                );

            AtpEndManagedSymbols(&context);
            NtClose(threadHandle);
        }

        if (!NT_SUCCESS(status) && context.Count == 0)
        {
            PPH_STRING message = PhGetStatusMessage(status, 0);

            PhAddJsonObject(row, "error", status == STATUS_ACCESS_DENIED ? "access_denied" : "failed");
            AtJsonAddStringZ(row, "message", PhGetStringOrDefault(message, L"unknown error"));
            PhClearReference(&message);
        }
        else
        {
            AtJsonAddNull(row, "error");
            AtJsonAddNull(row, "message");
        }

        PhAddJsonObjectValue(row, "frames", context.Frames);
        PhAddJsonObjectUInt64(row, "frame_count", context.Count);
        PhAddJsonObjectBoolean(row, "truncated", context.Truncated);
        PhAddJsonObjectBoolean(row, "managed_symbols", context.ManagedSymbols);

        PhAddJsonArrayObject(array, row);
    }

    PhDereferenceObject(symbolProvider);

    structured = PhCreateJsonObject();
    AtFillProcessIdentity(structured, Target->ProcessItem);
    AtJsonAddStringZ(structured, "order", order ? PhGetString(order) : L"cpu_time");
    PhAddJsonObjectValue(structured, "threads", array);
    PhAddJsonObjectUInt64(structured, "count", walkCount);
    PhAddJsonObjectUInt64(structured, "total_count", threadCount);
    PhAddJsonObjectBoolean(structured, "truncated", walkCount < threadCount);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    PhClearReference(&order);
    PhFree(threads);
    PhFree(processes);
}

VOID AtpAddSymbolLine(
    _In_ PVOID Structured,
    _In_ PPH_SYMBOL_PROVIDER SymbolProvider,
    _In_ PVOID Address
    )
{
    PPH_STRING fileName;
    PH_SYMBOL_LINE_INFORMATION lineInformation;

    if (PhGetLineFromAddress(SymbolProvider, Address, &fileName, NULL, &lineInformation))
    {
        PVOID line = PhCreateJsonObject();

        AtJsonAddString(line, "file", fileName);
        PhAddJsonObjectUInt64(line, "number", lineInformation.LineNumber);
        PhAddJsonObjectValue(Structured, "line", line);
        PhClearReference(&fileName);
    }
    else
    {
        AtJsonAddNull(Structured, "line");
    }
}

_Success_(return != NULL)
PPH_SYMBOL_PROVIDER AtpCreateFileSymbolProvider(
    _In_ PPH_STRING FileName,
    _Out_ PVOID *ImageBase,
    _Out_ PULONG ImageSize
    )
{
    NTSTATUS status;
    PH_MAPPED_IMAGE mappedImage;
    PPH_SYMBOL_PROVIDER symbolProvider;
    PVOID imageBase;
    ULONG imageSize;

    status = PhLoadMappedImageEx(&FileName->sr, NULL, &mappedImage);

    if (!NT_SUCCESS(status))
        return NULL;

    if (mappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        PIMAGE_OPTIONAL_HEADER64 optionalHeader = (PIMAGE_OPTIONAL_HEADER64)&mappedImage.NtHeaders->OptionalHeader;

        imageBase = (PVOID)optionalHeader->ImageBase;
        imageSize = optionalHeader->SizeOfImage;
    }
    else if (mappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        imageBase = (PVOID)(ULONG_PTR)mappedImage.NtHeaders32->OptionalHeader.ImageBase;
        imageSize = mappedImage.NtHeaders32->OptionalHeader.SizeOfImage;
    }
    else
    {
        PhUnloadMappedImage(&mappedImage);
        return NULL;
    }

    PhUnloadMappedImage(&mappedImage);

    if (!(symbolProvider = PhCreateSymbolProvider(NULL)))
        return NULL;

    PhLoadSymbolProviderOptions(symbolProvider);

    if (!PhLoadModuleSymbolProvider(symbolProvider, FileName, imageBase, imageSize))
    {
        PhDereferenceObject(symbolProvider);
        return NULL;
    }

    *ImageBase = imageBase;
    *ImageSize = imageSize;

    return symbolProvider;
}

VOID AtpResolveSymbol(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    AT_TARGET target;
    PPH_SYMBOL_PROVIDER symbolProvider = NULL;
    PPH_STRING path;
    PPH_STRING name;
    PPH_STRING nativePath = NULL;
    ULONG64 address = 0;
    PVOID imageBase = NULL;
    ULONG imageSize = 0;
    ULONG64 rva = 0;
    BOOLEAN haveAddress;
    BOOLEAN haveRva;
    BOOLEAN isFile;
    PVOID structured;

    memset(&target, 0, sizeof(AT_TARGET));

    path = AtGetArgumentString(Call->Arguments, "path");
    name = AtGetArgumentString(Call->Arguments, "name");
    haveAddress = AtGetArgumentPointer(Call->Arguments, "address", &address);
    haveRva = AtGetArgumentUInt64(Call->Arguments, "rva", &rva);
    isFile = !!path;

    if (!isFile && !AtJsonGetObjectMember(Call->Arguments, "pid", PH_JSON_OBJECT_TYPE_INT))
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"Either pid or path is required.");
        PhClearReference(&path);
        PhClearReference(&name);
        return;
    }

    if ((haveAddress ? 1 : 0) + (haveRva ? 1 : 0) + (name ? 1 : 0) != 1)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"Exactly one of address, rva or name is required.");
        PhClearReference(&path);
        PhClearReference(&name);
        return;
    }

    if (haveRva && !isFile)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"rva is only meaningful with path; a running process takes an address.");
        PhClearReference(&path);
        PhClearReference(&name);
        return;
    }

    if (isFile)
    {
        nativePath = PhDosPathNameToNtPathName(&path->sr);

        if (!nativePath)
        {
            AtSetToolError(Result, "invalid_arguments", STATUS_OBJECT_PATH_INVALID, L"The path could not be resolved.");
            PhClearReference(&path);
            PhClearReference(&name);
            return;
        }

        symbolProvider = AtpCreateFileSymbolProvider(nativePath, &imageBase, &imageSize);

        if (!symbolProvider)
        {
            AtSetToolError(Result, "failed", STATUS_UNSUCCESSFUL, L"The file could not be loaded for symbols.");
            PhClearReference(&nativePath);
            PhClearReference(&path);
            PhClearReference(&name);
            return;
        }

        if (haveRva)
            address = (ULONG64)PTR_ADD_OFFSET(imageBase, rva);
    }
    else
    {
        status = AtResolveProcessTarget(Call->Arguments, FALSE, PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, &target, Result);

        if (!NT_SUCCESS(status))
        {
            PhClearReference(&name);
            return;
        }

        symbolProvider = AtCreateSymbolProvider(target.ProcessItem->ProcessId);

        if (!symbolProvider)
        {
            AtSetToolError(Result, "failed", STATUS_UNSUCCESSFUL, L"The symbol provider could not be created.");
            AtDeleteTarget(&target);
            PhClearReference(&name);
            return;
        }
    }

    structured = PhCreateJsonObject();
    PhAddJsonObject(structured, "mode", isFile ? "file" : "process");

    if (isFile)
    {
        AtJsonAddString(structured, "path", path);
        AtJsonAddPointer(structured, "image_base", imageBase);
        PhAddJsonObjectUInt64(structured, "image_size", imageSize);
    }
    else
    {
        AtFillProcessIdentity(structured, target.ProcessItem);
    }

    if (name)
    {
        PH_SYMBOL_INFORMATION information;

        // The reverse direction. dbghelp wants module!symbol or a bare name, and answers from the
        // export table when there is no symbol file, which is why a name can resolve with no pdb.
        if (PhGetSymbolFromName(symbolProvider, PhGetString(name), &information))
        {
            PPH_STRING symbol;
            PPH_STRING fileName = NULL;
            PH_SYMBOL_RESOLVE_LEVEL resolveLevel = PhsrlInvalid;

            AtJsonAddString(structured, "name", name);
            AtJsonAddPointer(structured, "address", information.Address);
            AtJsonAddPointer(structured, "module_base", information.ModuleBase);
            PhAddJsonObjectUInt64(structured, "size", information.Size);

            if (isFile)
                PhAddJsonObjectUInt64(structured, "rva", (ULONG64)PTR_SUB_OFFSET(information.Address, imageBase));
            else
                AtJsonAddNull(structured, "rva");

            // A bare name is searched across every loaded module, so the answer can come from a
            // module the caller did not have in mind - an import thunk rather than the function
            // itself. Resolving the address back says which.
            symbol = PhGetSymbolFromAddress(symbolProvider, information.Address, &resolveLevel, &fileName, NULL, NULL);

            AtJsonAddString(structured, "symbol", symbol);
            AtJsonAddWin32FileName(structured, "module", fileName);
            AtJsonAddStringZ(structured, "resolve_level", AtpResolveLevelString(resolveLevel));
            AtJsonAddNull(structured, "displacement");
            PhClearReference(&symbol);
            PhClearReference(&fileName);

            if (AtJsonGetObjectBoolean(Call->Arguments, "include_line"))
                AtpAddSymbolLine(structured, symbolProvider, information.Address);
            else
                AtJsonAddNull(structured, "line");
        }
        else
        {
            AtSetToolError(Result, "not_found", STATUS_NOT_FOUND, L"No symbol named %s was found. A name resolves only from a symbol file or an export table.", PhGetString(name));
            PhFreeJsonObject(structured);
            goto CleanupExit;
        }
    }
    else
    {
        PPH_STRING symbol;
        PPH_STRING fileName = NULL;
        PPH_STRING symbolName = NULL;
        PH_SYMBOL_RESOLVE_LEVEL resolveLevel = PhsrlInvalid;
        ULONG64 displacement = 0;

        symbol = PhGetSymbolFromAddress(symbolProvider, (PVOID)address, &resolveLevel, &fileName, &symbolName, &displacement);

        AtJsonAddPointer(structured, "address", (PVOID)address);

        if (isFile)
            PhAddJsonObjectUInt64(structured, "rva", (ULONG64)PTR_SUB_OFFSET(address, imageBase));
        else
            AtJsonAddNull(structured, "rva");

        AtJsonAddString(structured, "symbol", symbol);
        AtJsonAddString(structured, "name", symbolName);
        AtJsonAddWin32FileName(structured, "module", fileName);
        AtJsonAddHex(structured, "displacement", displacement);
        // The resolve level says how much of the answer to believe: an address that resolved only to
        // a module is a fact about where it lives, a function name came from a symbol file.
        AtJsonAddStringZ(structured, "resolve_level", AtpResolveLevelString(resolveLevel));
        AtJsonAddNull(structured, "module_base");
        AtJsonAddNull(structured, "size");

        if (AtJsonGetObjectBoolean(Call->Arguments, "include_line"))
            AtpAddSymbolLine(structured, symbolProvider, (PVOID)address);
        else
            AtJsonAddNull(structured, "line");

        PhClearReference(&symbol);
        PhClearReference(&fileName);
        PhClearReference(&symbolName);
    }

    AtAddSnapshot(structured);
    Result->StructuredContent = structured;

CleanupExit:

    PhDereferenceObject(symbolProvider);

    if (!isFile)
        AtDeleteTarget(&target);

    PhClearReference(&nativePath);
    PhClearReference(&path);
    PhClearReference(&name);
}

VOID AtpControlThread(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    PVOID structured;
    IO_STATUS_BLOCK isb;
    BOOLEAN hadPendingIo = FALSE;

    switch (Tool->Action)
    {
    case AtActionCancelThreadIo:
        // THREAD_TERMINATE is what this call wants, which is more than it sounds like: cancelling
        // a thread's I/O is as disruptive to the thread as stopping it, and the access reflects it.
        status = NtCancelSynchronousIoFile(Target->ThreadHandle, NULL, &isb);

        // Nothing to cancel is an answer, not a failure: a thread not waiting on synchronous I/O
        // reports STATUS_NOT_FOUND, and reporting that as an error sends a caller looking for a
        // permission problem.
        if (status == STATUS_NOT_FOUND)
            status = STATUS_SUCCESS;
        else if (NT_SUCCESS(status))
            hadPendingIo = TRUE;

        break;
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

    if (Tool->Action == AtActionCancelThreadIo)
        PhAddJsonObjectBoolean(structured, "cancelled", hadPendingIo);

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
    case AtActionGetProcessStacks:
        AtpGetProcessStacks(Call, Target, Result);
        break;
    case AtActionResolveSymbol:
        AtpResolveSymbol(Call, Result);
        break;
    case AtActionSuspendThread:
    case AtActionResumeThread:
    case AtActionTerminateThread:
    case AtActionCancelThreadIo:
        AtpControlThread(Tool, Target, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
