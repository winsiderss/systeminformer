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

// One process worth of threads, out of a snapshot the caller already took so a batch enumerates
// once. Returns NULL when the process is not in that snapshot; in summary mode the threads are
// counted but no rows are built.
// The modules of a process by address range. A thread whose start address falls in none of them did
// not start in anything that was loaded as a module, which is the shape injected code has; deriving
// that from the module list rather than from symbols means it works with no symbol server, no PDBs
// and no symbol path at all.

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
    _In_ HANDLE ProcessId
    )
{
    PPH_LIST list;

    list = PhCreateList(64);
    PhEnumGenericModules(ProcessId, NULL, PH_ENUM_GENERIC_MAPPED_IMAGES, AtpThreadModuleCallback, list);

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

// The system-wide thread enumeration reports a start address of zero to a caller that is not
// elevated: the field is withheld rather than absent, which is why System Informer's own thread
// provider asks each thread for it (thrdprv.c:905). Same here, when the thread can be opened.
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

// The per-thread detail that needs the thread itself opened. Everything here is null when it could
// not be read, and the whole block is only gathered when the caller asks, because a process with a
// few hundred threads would otherwise cost a few hundred opens and several queries each.
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
    BOOLEAN includeDetails;
    AT_ROWS rows;
    PVOID structured;
    ULONG i;

    if (!(process = PhFindProcessInformation(Processes, ProcessItem->ProcessId)))
        return NULL;

    includeDetails = !Summary && AtJsonGetObjectBoolean(Call->Arguments, "include_details");

    if (includeDetails)
        moduleList = AtpCreateThreadModuleList(ProcessItem->ProcessId);

    // One process handle for the whole walk; the service tag query needs it.
    if (includeDetails && PH_IS_REAL_PROCESS_ID(ProcessItem->ProcessId))
        PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, ProcessItem->ProcessId);

    if (!Summary && AtJsonGetObjectBoolean(Call->Arguments, "resolve_start_addresses"))
        symbolProvider = AtpCreateSymbolProvider(ProcessItem->ProcessId);

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

        // Independent of symbols: null here means the thread started outside every loaded module.
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
    PPH_SYMBOL_PROVIDER SymbolProvider;
    PVOID Frames;
    ULONG Count;
    ULONG MaximumFrames;
    BOOLEAN Truncated;
} AT_STACK_CONTEXT, *PAT_STACK_CONTEXT;

_Function_class_(PH_WALK_THREAD_STACK_CALLBACK)
// A managed frame has no native symbol worth reading: dbghelp resolves it to whatever jitted code
// happens to sit at that address, or to nothing at all. The DotNetTools plugin can name it, through
// the thread stack control callback the application fires around its own walk, so this fires the
// same sequence: initialize, announce the default walk, resolve each frame, tear down.
//
// Not done for a 32-bit process on a 64-bit build. DotNetTools reaches a WOW64 target's CLR through
// phsvc, and starting phsvc prompts for elevation - a background tool call must not put a consent
// dialog on the screen, and would block on it for as long as it took to answer.
VOID AtpBeginManagedSymbols(
    _Inout_ PAT_STACK_CONTEXT Context,
    _In_ PAT_TARGET Target,
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
    control.u.Initializing.ProcessId = Target->ProcessItem->ProcessId;
    control.u.Initializing.ThreadId = Target->ThreadId;
    control.u.Initializing.ThreadHandle = Target->ThreadHandle;
    control.u.Initializing.ProcessHandle = ProcessHandle;
    control.u.Initializing.SymbolProvider = Context->SymbolProvider;
    control.u.Initializing.CustomWalk = FALSE;
    PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadStackControl), &control);

    // CustomWalk is deliberately ignored: this tool always walks the stack itself, which is the
    // path the application takes whenever a custom walk is unavailable or fails, and the plugin
    // still gets to name every frame.
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

        // DotNetTools answers this with the managed method name for a frame the CLR owns, and
        // leaves the symbol alone for the rest. It takes ownership of what it is given and hands
        // back what it wants reported, so the returned pointer changing is what says the frame was
        // managed - there is no other way to tell a jitted frame from an unresolved native one.
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

    AtpBeginManagedSymbols(&context, Target, processHandle);

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
