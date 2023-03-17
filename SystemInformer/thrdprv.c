/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2023
 *
 */

/*
 * The thread provider is tied to the process provider, and runs by registering a callback for the
 * processes-updated event. This is because calculating CPU usage depends on deltas calculated by
 * the process provider. However, this does increase the complexity of the thread provider system.
 */

#include <phapp.h>
#include <phplug.h>
#include <thrdprv.h>

#include <svcsup.h>
#include <symprv.h>
#include <workqueue.h>

#include <extmgri.h>
#include <procprv.h>

typedef struct _PH_THREAD_QUERY_DATA
{
    SLIST_ENTRY ListEntry;
    PPH_THREAD_PROVIDER ThreadProvider;
    PPH_THREAD_ITEM ThreadItem;
    ULONG64 RunId;

    PPH_STRING StartAddressString;
    PPH_STRING StartAddressFileName;
    PH_SYMBOL_RESOLVE_LEVEL StartAddressResolveLevel;

    PPH_STRING ServiceName;
} PH_THREAD_QUERY_DATA, *PPH_THREAD_QUERY_DATA;

typedef struct _PH_THREAD_SYMBOL_LOAD_CONTEXT
{
    HANDLE ProcessId;
    PPH_THREAD_PROVIDER ThreadProvider;
    PPH_SYMBOL_PROVIDER SymbolProvider;
} PH_THREAD_SYMBOL_LOAD_CONTEXT, *PPH_THREAD_SYMBOL_LOAD_CONTEXT;

VOID NTAPI PhpThreadProviderDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

VOID NTAPI PhpThreadItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

BOOLEAN NTAPI PhpThreadHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

ULONG NTAPI PhpThreadHashtableHashFunction(
    _In_ PVOID Entry
    );

VOID PhpThreadProviderCallbackHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID PhpThreadProviderUpdate(
    _In_ PPH_THREAD_PROVIDER ThreadProvider,
    _In_ PVOID ProcessInformation
    );

PPH_OBJECT_TYPE PhThreadProviderType = NULL;
PPH_OBJECT_TYPE PhThreadItemType = NULL;
PH_WORK_QUEUE PhThreadProviderWorkQueue;
PH_INITONCE PhThreadProviderWorkQueueInitOnce = PH_INITONCE_INIT;

VOID PhpQueueThreadWorkQueueItem(
    _In_ PUSER_THREAD_START_ROUTINE Function,
    _In_opt_ PVOID Context
    )
{
    if (PhBeginInitOnce(&PhThreadProviderWorkQueueInitOnce))
    {
        PhInitializeWorkQueue(&PhThreadProviderWorkQueue, 0, 1, 1000);
        PhEndInitOnce(&PhThreadProviderWorkQueueInitOnce);
    }

    PhQueueItemWorkQueue(&PhThreadProviderWorkQueue, Function, Context);
}

PPH_THREAD_PROVIDER PhCreateThreadProvider(
    _In_ HANDLE ProcessId
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    PPH_THREAD_PROVIDER threadProvider;

    if (PhBeginInitOnce(&initOnce))
    {
        PhThreadProviderType = PhCreateObjectType(L"ThreadProvider", 0, PhpThreadProviderDeleteProcedure);
        PhThreadItemType = PhCreateObjectType(L"ThreadItem", 0, PhpThreadItemDeleteProcedure);
        PhEndInitOnce(&initOnce);
    }

    threadProvider = PhCreateObject(
        PhEmGetObjectSize(EmThreadProviderType, sizeof(PH_THREAD_PROVIDER)),
        PhThreadProviderType
        );
    memset(threadProvider, 0, sizeof(PH_THREAD_PROVIDER));

    threadProvider->ThreadHashtable = PhCreateHashtable(
        sizeof(PPH_THREAD_ITEM),
        PhpThreadHashtableEqualFunction,
        PhpThreadHashtableHashFunction,
        20
        );
    PhInitializeFastLock(&threadProvider->ThreadHashtableLock);

    PhInitializeCallback(&threadProvider->ThreadAddedEvent);
    PhInitializeCallback(&threadProvider->ThreadModifiedEvent);
    PhInitializeCallback(&threadProvider->ThreadRemovedEvent);
    PhInitializeCallback(&threadProvider->UpdatedEvent);
    PhInitializeCallback(&threadProvider->LoadingStateChangedEvent);

    threadProvider->ProcessId = ProcessId;
    threadProvider->SymbolProvider = PhCreateSymbolProvider(ProcessId);

    if (threadProvider->SymbolProvider)
    {
        if (threadProvider->SymbolProvider->IsRealHandle)
            threadProvider->ProcessHandle = threadProvider->SymbolProvider->ProcessHandle;
    }

    RtlInitializeSListHead(&threadProvider->QueryListHead);
    PhInitializeQueuedLock(&threadProvider->LoadSymbolsLock);

    threadProvider->RunId = 1;
    threadProvider->SymbolsLoadedRunId = 0; // Force symbols to be loaded the first time we try to resolve an address

    PhEmCallObjectOperation(EmThreadProviderType, threadProvider, EmObjectCreate);

    return threadProvider;
}

VOID PhpThreadProviderDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_THREAD_PROVIDER threadProvider = (PPH_THREAD_PROVIDER)Object;

    PhEmCallObjectOperation(EmThreadProviderType, threadProvider, EmObjectDelete);

    // Dereference all thread items (we referenced them
    // when we added them to the hashtable).
    PhDereferenceAllThreadItems(threadProvider);

    PhDereferenceObject(threadProvider->ThreadHashtable);
    PhDeleteFastLock(&threadProvider->ThreadHashtableLock);
    PhDeleteCallback(&threadProvider->ThreadAddedEvent);
    PhDeleteCallback(&threadProvider->ThreadModifiedEvent);
    PhDeleteCallback(&threadProvider->ThreadRemovedEvent);
    PhDeleteCallback(&threadProvider->UpdatedEvent);
    PhDeleteCallback(&threadProvider->LoadingStateChangedEvent);

    // Destroy all queue items.
    {
        PSLIST_ENTRY entry;
        PPH_THREAD_QUERY_DATA data;

        entry = RtlInterlockedFlushSList(&threadProvider->QueryListHead);

        while (entry)
        {
            data = CONTAINING_RECORD(entry, PH_THREAD_QUERY_DATA, ListEntry);
            entry = entry->Next;

            PhClearReference(&data->StartAddressString);
            PhClearReference(&data->ServiceName);
            PhDereferenceObject(data->ThreadItem);
            PhFree(data);
        }
    }

    // We don't close the process handle because it is owned by the symbol provider.
    if (threadProvider->SymbolProvider) PhDereferenceObject(threadProvider->SymbolProvider);
}

VOID PhRegisterThreadProvider(
    _In_ PPH_THREAD_PROVIDER ThreadProvider,
    _Out_ PPH_CALLBACK_REGISTRATION CallbackRegistration
    )
{
    PhReferenceObject(ThreadProvider);
    PhRegisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent), PhpThreadProviderCallbackHandler, ThreadProvider, CallbackRegistration);
}

VOID PhUnregisterThreadProvider(
    _In_ PPH_THREAD_PROVIDER ThreadProvider,
    _In_ PPH_CALLBACK_REGISTRATION CallbackRegistration
    )
{
    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent), CallbackRegistration);
    PhDereferenceObject(ThreadProvider);
}

VOID PhSetTerminatingThreadProvider(
    _Inout_ PPH_THREAD_PROVIDER ThreadProvider
    )
{
    ThreadProvider->Terminating = TRUE;
}

static BOOLEAN LoadSymbolsEnumGenericModulesCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_ PVOID Context
    )
{
    PPH_THREAD_SYMBOL_LOAD_CONTEXT context = Context;

    if (context->ThreadProvider->Terminating)
        return FALSE;

    // If we're loading kernel module symbols for a process other than System, ignore modules which
    // are in user space. This may happen in Windows 7.
    if (context->ProcessId == SYSTEM_PROCESS_ID &&
        context->ThreadProvider->ProcessId != SYSTEM_PROCESS_ID &&
        (ULONG_PTR)Module->BaseAddress <= PhSystemBasicInformation.MaximumUserModeAddress)
    {
        return TRUE;
    }

    PhLoadModuleSymbolProvider(
        context->SymbolProvider,
        Module->FileName,
        (ULONG64)Module->BaseAddress,
        Module->Size
        );

    return TRUE;
}

static BOOLEAN LoadBasicSymbolsEnumGenericModulesCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_ PVOID Context
    )
{
    PPH_THREAD_SYMBOL_LOAD_CONTEXT context = Context;

    if (context->ThreadProvider->Terminating)
        return FALSE;

    if (PhEqualString2(Module->Name, L"ntdll.dll", TRUE) ||
        PhEqualString2(Module->Name, L"kernel32.dll", TRUE))
    {
        PhLoadModuleSymbolProvider(
            context->SymbolProvider,
            Module->FileName,
            (ULONG64)Module->BaseAddress,
            Module->Size
            );
    }

    return TRUE;
}

VOID PhLoadSymbolsThreadProvider(
    _In_ PPH_THREAD_PROVIDER ThreadProvider
    )
{
    PH_THREAD_SYMBOL_LOAD_CONTEXT loadContext;
    ULONG64 runId;

    loadContext.ThreadProvider = ThreadProvider;
    loadContext.SymbolProvider = ThreadProvider->SymbolProvider;

    PhAcquireQueuedLockExclusive(&ThreadProvider->LoadSymbolsLock);
    runId = ThreadProvider->RunId;
    PhLoadSymbolProviderOptions(ThreadProvider->SymbolProvider);

    if (ThreadProvider->ProcessId != SYSTEM_IDLE_PROCESS_ID)
    {
        if (ThreadProvider->SymbolProvider->IsRealHandle || ThreadProvider->ProcessId == SYSTEM_PROCESS_ID)
        {
            loadContext.ProcessId = ThreadProvider->ProcessId;
            PhEnumGenericModules(
                ThreadProvider->ProcessId,
                ThreadProvider->SymbolProvider->ProcessHandle,
                0,
                LoadSymbolsEnumGenericModulesCallback,
                &loadContext
                );
        }

        {
            // We can't enumerate the process modules. Load symbols for ntdll.dll and kernel32.dll.
            loadContext.ProcessId = NtCurrentProcessId();
            PhEnumGenericModules(
                NtCurrentProcessId(),
                NtCurrentProcess(),
                0,
                LoadBasicSymbolsEnumGenericModulesCallback,
                &loadContext
                );
        }

        // Load kernel module symbols as well.
        if (ThreadProvider->ProcessId != SYSTEM_PROCESS_ID)
        {
            loadContext.ProcessId = SYSTEM_PROCESS_ID;
            PhEnumGenericModules(
                SYSTEM_PROCESS_ID,
                NULL,
                0,
                LoadSymbolsEnumGenericModulesCallback,
                &loadContext
                );
        }
    }
    else
    {
        // System Idle Process has one thread for each CPU, each having a start address at
        // KiIdleLoop. We need to load symbols for the kernel.

        PRTL_PROCESS_MODULES kernelModules;

        if (NT_SUCCESS(PhEnumKernelModules(&kernelModules)))
        {
            if (kernelModules->NumberOfModules > 0)
            {
                PPH_STRING fileName;

                fileName = PhConvertMultiByteToUtf16(kernelModules->Modules[0].FullPathName);

                PhLoadModuleSymbolProvider(
                    ThreadProvider->SymbolProvider,
                    fileName,
                    (ULONG64)kernelModules->Modules[0].ImageBase,
                    kernelModules->Modules[0].ImageSize
                    );
                PhDereferenceObject(fileName);
            }

            PhFree(kernelModules);
        }
    }

    ThreadProvider->SymbolsLoadedRunId = runId;
    PhReleaseQueuedLockExclusive(&ThreadProvider->LoadSymbolsLock);
}

PPH_THREAD_ITEM PhCreateThreadItem(
    _In_ HANDLE ThreadId
    )
{
    PPH_THREAD_ITEM threadItem;

    threadItem = PhCreateObject(
        PhEmGetObjectSize(EmThreadItemType, sizeof(PH_THREAD_ITEM)),
        PhThreadItemType
        );
    memset(threadItem, 0, sizeof(PH_THREAD_ITEM));
    threadItem->ThreadId = ThreadId;

    PhPrintUInt32(threadItem->ThreadIdString, HandleToUlong(ThreadId));

    PhEmCallObjectOperation(EmThreadItemType, threadItem, EmObjectCreate);

    return threadItem;
}

VOID PhpThreadItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_THREAD_ITEM threadItem = (PPH_THREAD_ITEM)Object;

    PhEmCallObjectOperation(EmThreadItemType, threadItem, EmObjectDelete);

    if (threadItem->ThreadHandle) NtClose(threadItem->ThreadHandle);
    if (threadItem->StartAddressString) PhDereferenceObject(threadItem->StartAddressString);
    if (threadItem->StartAddressFileName) PhDereferenceObject(threadItem->StartAddressFileName);
    if (threadItem->ServiceName) PhDereferenceObject(threadItem->ServiceName);
}

BOOLEAN PhpThreadHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    return
        (*(PPH_THREAD_ITEM *)Entry1)->ThreadId ==
        (*(PPH_THREAD_ITEM *)Entry2)->ThreadId;
}

ULONG PhpThreadHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return HandleToUlong((*(PPH_THREAD_ITEM *)Entry)->ThreadId) / 4;
}

PPH_THREAD_ITEM PhReferenceThreadItem(
    _In_ PPH_THREAD_PROVIDER ThreadProvider,
    _In_ HANDLE ThreadId
    )
{
    PH_THREAD_ITEM lookupThreadItem;
    PPH_THREAD_ITEM lookupThreadItemPtr = &lookupThreadItem;
    PPH_THREAD_ITEM *threadItemPtr;
    PPH_THREAD_ITEM threadItem;

    lookupThreadItem.ThreadId = ThreadId;

    PhAcquireFastLockShared(&ThreadProvider->ThreadHashtableLock);

    threadItemPtr = (PPH_THREAD_ITEM *)PhFindEntryHashtable(
        ThreadProvider->ThreadHashtable,
        &lookupThreadItemPtr
        );

    if (threadItemPtr)
    {
        threadItem = *threadItemPtr;
        PhReferenceObject(threadItem);
    }
    else
    {
        threadItem = NULL;
    }

    PhReleaseFastLockShared(&ThreadProvider->ThreadHashtableLock);

    return threadItem;
}

VOID PhDereferenceAllThreadItems(
    _In_ PPH_THREAD_PROVIDER ThreadProvider
    )
{
    ULONG enumerationKey = 0;
    PPH_THREAD_ITEM *threadItem;

    PhAcquireFastLockExclusive(&ThreadProvider->ThreadHashtableLock);

    while (PhEnumHashtable(ThreadProvider->ThreadHashtable, (PVOID *)&threadItem, &enumerationKey))
    {
        PhDereferenceObject(*threadItem);
    }

    PhReleaseFastLockExclusive(&ThreadProvider->ThreadHashtableLock);
}

VOID PhpRemoveThreadItem(
    _In_ PPH_THREAD_PROVIDER ThreadProvider,
    _In_ PPH_THREAD_ITEM ThreadItem
    )
{
    PhRemoveEntryHashtable(ThreadProvider->ThreadHashtable, &ThreadItem);
    PhDereferenceObject(ThreadItem);
}

NTSTATUS PhpThreadQueryWorker(
    _In_ PVOID Parameter
    )
{
    PPH_THREAD_QUERY_DATA data = (PPH_THREAD_QUERY_DATA)Parameter;
    LONG newSymbolsLoading;

    if (data->ThreadProvider->Terminating)
        goto Done;

    newSymbolsLoading = _InterlockedIncrement(&data->ThreadProvider->SymbolsLoading);

    if (newSymbolsLoading == 1)
        PhInvokeCallback(&data->ThreadProvider->LoadingStateChangedEvent, (PVOID)TRUE);

    if (data->ThreadProvider->SymbolsLoadedRunId == 0)
        PhLoadSymbolsThreadProvider(data->ThreadProvider);

    data->StartAddressString = PhGetSymbolFromAddress(
        data->ThreadProvider->SymbolProvider,
        data->ThreadItem->StartAddress,
        &data->StartAddressResolveLevel,
        &data->StartAddressFileName,
        NULL,
        NULL
        );

    if (data->StartAddressResolveLevel == PhsrlAddress && data->ThreadProvider->SymbolsLoadedRunId < data->RunId)
    {
        // The process may have loaded new modules, so load symbols for those and try again.

        PhLoadSymbolsThreadProvider(data->ThreadProvider);

        PhClearReference(&data->StartAddressString);
        PhClearReference(&data->StartAddressFileName);
        data->StartAddressString = PhGetSymbolFromAddress(
            data->ThreadProvider->SymbolProvider,
            data->ThreadItem->StartAddress,
            &data->StartAddressResolveLevel,
            &data->StartAddressFileName,
            NULL,
            NULL
            );
    }

    newSymbolsLoading = _InterlockedDecrement(&data->ThreadProvider->SymbolsLoading);

    if (newSymbolsLoading == 0)
        PhInvokeCallback(&data->ThreadProvider->LoadingStateChangedEvent, (PVOID)FALSE);

    // Check if the process has services - we'll need to know before getting service tag/name
    // information.
    if (!data->ThreadProvider->HasServicesKnown)
    {
        PPH_PROCESS_ITEM processItem;

        if (processItem = PhReferenceProcessItem(data->ThreadProvider->ProcessId))
        {
            data->ThreadProvider->HasServices = processItem->ServiceList && processItem->ServiceList->Count != 0;
            PhDereferenceObject(processItem);
        }

        data->ThreadProvider->HasServicesKnown = TRUE;
    }

    // Get the service tag, and the service name.
    if (
        data->ThreadProvider->SymbolProvider->IsRealHandle &&
        data->ThreadItem->ThreadHandle
        )
    {
        PVOID serviceTag;

        if (NT_SUCCESS(PhGetThreadServiceTag(
            data->ThreadItem->ThreadHandle,
            data->ThreadProvider->ProcessHandle,
            &serviceTag
            )))
        {
            data->ServiceName = PhGetServiceNameFromTag(
                data->ThreadProvider->ProcessId,
                serviceTag
                );
        }
    }

Done:
    RtlInterlockedPushEntrySList(&data->ThreadProvider->QueryListHead, &data->ListEntry);
    PhDereferenceObject(data->ThreadProvider);

    return STATUS_SUCCESS;
}

VOID PhpQueueThreadQuery(
    _In_ PPH_THREAD_PROVIDER ThreadProvider,
    _In_ PPH_THREAD_ITEM ThreadItem
    )
{
    PPH_THREAD_QUERY_DATA data;

    data = PhAllocate(sizeof(PH_THREAD_QUERY_DATA));
    memset(data, 0, sizeof(PH_THREAD_QUERY_DATA));
    PhSetReference(&data->ThreadProvider, ThreadProvider);
    PhSetReference(&data->ThreadItem, ThreadItem);
    data->RunId = ThreadProvider->RunId;

    PhpQueueThreadWorkQueueItem(PhpThreadQueryWorker, data);
}

PPH_STRING PhpGetThreadBasicStartAddress(
    _In_ PPH_THREAD_PROVIDER ThreadProvider,
    _In_ ULONG64 Address,
    _Out_ PPH_SYMBOL_RESOLVE_LEVEL ResolveLevel
    )
{
    ULONG64 modBase;
    PPH_STRING fileName = NULL;
    PPH_STRING baseName = NULL;
    PPH_STRING symbol;

    modBase = PhGetModuleFromAddress(
        ThreadProvider->SymbolProvider,
        Address,
        &fileName
        );

    if (fileName == NULL)
    {
        *ResolveLevel = PhsrlAddress;

        symbol = PhCreateStringEx(NULL, PH_PTR_STR_LEN * sizeof(WCHAR));
        PhPrintPointer(symbol->Buffer, (PVOID)Address);
        PhTrimToNullTerminatorString(symbol);
    }
    else
    {
        PH_FORMAT format[3];

        baseName = PhGetBaseName(fileName);
        *ResolveLevel = PhsrlModule;

        PhInitFormatSR(&format[0], baseName->sr);
        PhInitFormatS(&format[1], L"+0x");
        PhInitFormatIX(&format[2], (ULONG_PTR)(Address - modBase));

        symbol = PhFormat(format, 3, baseName->Length + 6 + 32);
    }

    if (fileName)
        PhDereferenceObject(fileName);
    if (baseName)
        PhDereferenceObject(baseName);

    return symbol;
}

static NTSTATUS PhpGetThreadHandle(
    _Out_ PHANDLE ThreadHandle,
    _In_ PPH_THREAD_PROVIDER ThreadProvider,
    _In_ PPH_THREAD_ITEM ThreadItem
    )
{
    NTSTATUS status;

    if (ThreadProvider->ProcessId == SYSTEM_IDLE_PROCESS_ID)
    {
        if (HandleToUlong(ThreadItem->ThreadId) < PhSystemProcessorInformation.NumberOfProcessors)
            return STATUS_UNSUCCESSFUL;
    }

    status = PhOpenThread(
        ThreadHandle,
        THREAD_QUERY_INFORMATION,
        ThreadItem->ThreadId
        );

    if (!NT_SUCCESS(status))
    {
        status = PhOpenThread(
            ThreadHandle,
            THREAD_QUERY_LIMITED_INFORMATION,
            ThreadItem->ThreadId
            );
    }

    return status;
}

static NTSTATUS PhpGetThreadCycleTime(
    _In_ PPH_THREAD_PROVIDER ThreadProvider,
    _In_ PPH_THREAD_ITEM ThreadItem,
    _Out_ PULONG64 CycleTime
    )
{
    if (ThreadProvider->ProcessId != SYSTEM_IDLE_PROCESS_ID)
    {
        if (ThreadItem->ThreadHandle)
        {
            return PhGetThreadCycleTime(ThreadItem->ThreadHandle, CycleTime);
        }
    }
    else
    {
        if (HandleToUlong(ThreadItem->ThreadId) < PhSystemProcessorInformation.NumberOfProcessors)
        {
            *CycleTime = PhCpuIdleCycleTime[HandleToUlong(ThreadItem->ThreadId)].QuadPart;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_INVALID_PARAMETER;
}

PPH_STRING PhGetBasePriorityIncrementString(
    _In_ KPRIORITY Increment
    )
{
    switch (Increment)
    {
    case THREAD_BASE_PRIORITY_LOWRT + 1:
    case THREAD_BASE_PRIORITY_LOWRT:
        return PhCreateString(L"Time critical");
    case THREAD_PRIORITY_HIGHEST:
        return PhCreateString(L"Highest");
    case THREAD_PRIORITY_ABOVE_NORMAL:
        return PhCreateString(L"Above normal");
    case THREAD_PRIORITY_NORMAL:
        return PhCreateString(L"Normal");
    case THREAD_PRIORITY_BELOW_NORMAL:
        return PhCreateString(L"Below normal");
    case THREAD_PRIORITY_LOWEST:
        return PhCreateString(L"Lowest");
    case THREAD_BASE_PRIORITY_IDLE:
    case THREAD_BASE_PRIORITY_IDLE - 1:
        return PhCreateString(L"Idle");
    case THREAD_PRIORITY_ERROR_RETURN:
        return NULL;
    default:
        return PhFormatString(L"%ld", Increment);
    }
}

VOID PhThreadProviderInitialUpdate(
    _In_ PPH_THREAD_PROVIDER ThreadProvider
    )
{
    PVOID processes;

    if (NT_SUCCESS(PhEnumProcesses(&processes)))
    {
        PhpThreadProviderUpdate(ThreadProvider, processes);
        PhFree(processes);
    }
}

VOID PhpThreadProviderCallbackHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (Context && PhProcessInformation)
    {
        PhpThreadProviderUpdate((PPH_THREAD_PROVIDER)Context, PhProcessInformation);
    }
}

VOID PhpThreadProviderUpdate(
    _In_ PPH_THREAD_PROVIDER ThreadProvider,
    _In_ PVOID ProcessInformation
    )
{
    PPH_THREAD_PROVIDER threadProvider = ThreadProvider;
    PSYSTEM_PROCESS_INFORMATION process;
    SYSTEM_PROCESS_INFORMATION localProcess;
    PSYSTEM_THREAD_INFORMATION threads;
    ULONG numberOfThreads;
    ULONG i;

    process = PhFindProcessInformation(ProcessInformation, threadProvider->ProcessId);

    if (!process)
    {
        // The process doesn't exist anymore. Pretend it does but has no threads.
        process = &localProcess;
        process->NumberOfThreads = 0;
    }

    threads = process->Threads;
    numberOfThreads = process->NumberOfThreads;

    // System Idle Process has one thread per CPU. They all have a TID of 0. We can't have duplicate
    // TIDs, so we'll assign unique TIDs. (wj32)
    if (threadProvider->ProcessId == SYSTEM_IDLE_PROCESS_ID)
    {
        ULONG processorIndex = 0;

        for (i = 0; i < numberOfThreads; i++)
        {
            if (!threads[i].ClientId.UniqueThread)
            {
                threads[i].ClientId.UniqueThread = UlongToHandle(processorIndex);
                processorIndex++;
            }
        }
    }

    // Look for dead threads.
    {
        PPH_LIST threadsToRemove = NULL;
        ULONG enumerationKey = 0;
        PPH_THREAD_ITEM *threadItem;

        while (PhEnumHashtable(threadProvider->ThreadHashtable, (PVOID *)&threadItem, &enumerationKey))
        {
            BOOLEAN found = FALSE;

            // Check if the thread still exists.
            for (i = 0; i < numberOfThreads; i++)
            {
                PSYSTEM_THREAD_INFORMATION thread = &threads[i];

                if ((*threadItem)->ThreadId == thread->ClientId.UniqueThread)
                {
                    found = TRUE;
                    break;
                }
            }

            if (!found)
            {
                // Raise the thread removed event.
                PhInvokeCallback(&threadProvider->ThreadRemovedEvent, *threadItem);

                if (!threadsToRemove)
                    threadsToRemove = PhCreateList(2);

                PhAddItemList(threadsToRemove, *threadItem);
            }
        }

        if (threadsToRemove)
        {
            PhAcquireFastLockExclusive(&threadProvider->ThreadHashtableLock);

            for (i = 0; i < threadsToRemove->Count; i++)
            {
                PhpRemoveThreadItem(
                    threadProvider,
                    (PPH_THREAD_ITEM)threadsToRemove->Items[i]
                    );
            }

            PhReleaseFastLockExclusive(&threadProvider->ThreadHashtableLock);
            PhDereferenceObject(threadsToRemove);
        }
    }

    // Go through the queued thread query data.
    {
        PSLIST_ENTRY entry;
        PPH_THREAD_QUERY_DATA data;

        entry = RtlInterlockedFlushSList(&threadProvider->QueryListHead);

        while (entry)
        {
            data = CONTAINING_RECORD(entry, PH_THREAD_QUERY_DATA, ListEntry);
            entry = entry->Next;

            if (data->StartAddressResolveLevel == PhsrlFunction && data->StartAddressString)
            {
                PhSwapReference(&data->ThreadItem->StartAddressString, data->StartAddressString);
                PhSwapReference(&data->ThreadItem->StartAddressFileName, data->StartAddressFileName);
                data->ThreadItem->StartAddressResolveLevel = data->StartAddressResolveLevel;
            }

            PhMoveReference(&data->ThreadItem->ServiceName, data->ServiceName);

            data->ThreadItem->JustResolved = TRUE;

            if (data->StartAddressString) PhDereferenceObject(data->StartAddressString);
            if (data->StartAddressFileName) PhDereferenceObject(data->StartAddressFileName);
            PhDereferenceObject(data->ThreadItem);
            PhFree(data);
        }
    }

    // Look for new threads and update existing ones.
    for (i = 0; i < numberOfThreads; i++)
    {
        PSYSTEM_THREAD_INFORMATION thread = &threads[i];
        PPH_THREAD_ITEM threadItem;
        THREAD_BASIC_INFORMATION basicInfo;

        threadItem = PhReferenceThreadItem(threadProvider, thread->ClientId.UniqueThread);

        if (!threadItem)
        {
            PVOID startAddress = NULL;

            threadItem = PhCreateThreadItem(thread->ClientId.UniqueThread);
            threadItem->CreateTime = thread->CreateTime;
            threadItem->KernelTime = thread->KernelTime;
            threadItem->UserTime = thread->UserTime;
            PhUpdateDelta(&threadItem->ContextSwitchesDelta, thread->ContextSwitches);
            threadItem->Priority = thread->Priority;
            threadItem->BasePriority = thread->BasePriority;
            threadItem->State = (KTHREAD_STATE)thread->ThreadState;
            threadItem->WaitReason = thread->WaitReason;

            PhpGetThreadHandle(
                &threadItem->ThreadHandle,
                threadProvider,
                threadItem
                );

            // Get the cycle count.
            {
                ULONG64 cycles;

                if (NT_SUCCESS(PhpGetThreadCycleTime(
                    threadProvider,
                    threadItem,
                    &cycles
                    )))
                {
                    PhUpdateDelta(&threadItem->CyclesDelta, cycles);
                }
            }

            // Initialize the CPU time deltas.
            PhUpdateDelta(&threadItem->CpuKernelDelta, threadItem->KernelTime.QuadPart);
            PhUpdateDelta(&threadItem->CpuUserDelta, threadItem->UserTime.QuadPart);

            // Try to get the start address.

            if (threadItem->ThreadHandle)
            {
                PhGetThreadStartAddress(threadItem->ThreadHandle, &startAddress);
            }

            if (!startAddress)
                startAddress = thread->StartAddress;

            threadItem->StartAddress = (ULONG64)startAddress;

            // Get the base priority increment (relative to the process priority).
            if (threadItem->ThreadHandle && NT_SUCCESS(PhGetThreadBasicInformation(
                threadItem->ThreadHandle,
                &basicInfo
                )))
            {
                threadItem->BasePriorityIncrement = basicInfo.BasePriority;
            }
            else
            {
                threadItem->BasePriorityIncrement = THREAD_PRIORITY_ERROR_RETURN;
            }

            if (threadProvider->SymbolsLoadedRunId != 0)
            {
                threadItem->StartAddressString = PhpGetThreadBasicStartAddress(
                    threadProvider,
                    threadItem->StartAddress,
                    &threadItem->StartAddressResolveLevel
                    );
            }

            if (!threadItem->StartAddressString)
            {
                threadItem->StartAddressResolveLevel = PhsrlAddress;
                threadItem->StartAddressString = PhCreateStringEx(NULL, PH_PTR_STR_LEN * sizeof(WCHAR));
                PhPrintPointer(
                    threadItem->StartAddressString->Buffer,
                    (PVOID)threadItem->StartAddress
                    );
                PhTrimToNullTerminatorString(threadItem->StartAddressString);
            }

            // Is it a GUI thread?
            if (threadItem->ThreadId)
            {
                GUITHREADINFO info = { sizeof(GUITHREADINFO) };

                threadItem->IsGuiThread = !!GetGUIThreadInfo(HandleToUlong(threadItem->ThreadId), &info);
            }

            PhpQueueThreadQuery(threadProvider, threadItem);

            // Add the thread item to the hashtable.
            PhAcquireFastLockExclusive(&threadProvider->ThreadHashtableLock);
            PhAddEntryHashtable(threadProvider->ThreadHashtable, &threadItem);
            PhReleaseFastLockExclusive(&threadProvider->ThreadHashtableLock);

            // Raise the thread added event.
            PhInvokeCallback(&threadProvider->ThreadAddedEvent, threadItem);
        }
        else
        {
            BOOLEAN modified = FALSE;

            if (threadItem->JustResolved)
                modified = TRUE;

            threadItem->KernelTime = thread->KernelTime;
            threadItem->UserTime = thread->UserTime;
            threadItem->Priority = thread->Priority;
            threadItem->BasePriority = thread->BasePriority;
            threadItem->State = thread->ThreadState;
            threadItem->WaitTime = thread->WaitTime;

            if (threadItem->WaitReason != thread->WaitReason)
            {
                threadItem->WaitReason = thread->WaitReason;
                modified = TRUE;
            }

            // If the resolve level is only at address, it probably
            // means symbols weren't loaded the last time we
            // tried to get the start address. Try again.
            if (threadItem->StartAddressResolveLevel == PhsrlAddress)
            {
                if (threadProvider->SymbolsLoadedRunId != 0)
                {
                    PPH_STRING newStartAddressString;

                    newStartAddressString = PhpGetThreadBasicStartAddress(
                        threadProvider,
                        threadItem->StartAddress,
                        &threadItem->StartAddressResolveLevel
                        );

                    PhMoveReference(
                        &threadItem->StartAddressString,
                        newStartAddressString
                        );

                    modified = TRUE;
                }
            }

            // If we couldn't resolve the start address to a module+offset, use the StartAddress
            // instead of the Win32StartAddress and try again. Note that we check the resolve level
            // again because we may have changed it in the previous block.
            if (threadItem->JustResolved &&
                threadItem->StartAddressResolveLevel == PhsrlAddress)
            {
                if (threadItem->StartAddress != (ULONG64)thread->StartAddress)
                {
                    threadItem->StartAddress = (ULONG64)thread->StartAddress;
                    PhpQueueThreadQuery(threadProvider, threadItem);
                }
            }

            // Update the context switch count.
            {
                ULONG oldDelta;

                oldDelta = threadItem->ContextSwitchesDelta.Delta;
                PhUpdateDelta(&threadItem->ContextSwitchesDelta, thread->ContextSwitches);

                if (threadItem->ContextSwitchesDelta.Delta != oldDelta)
                {
                    modified = TRUE;
                }
            }

            // Update the cycle count.
            {
                ULONG64 cycles;
                ULONG64 oldDelta;

                oldDelta = threadItem->CyclesDelta.Delta;

                if (NT_SUCCESS(PhpGetThreadCycleTime(
                    threadProvider,
                    threadItem,
                    &cycles
                    )))
                {
                    PhUpdateDelta(&threadItem->CyclesDelta, cycles);

                    if (threadItem->CyclesDelta.Delta != oldDelta)
                    {
                        modified = TRUE;
                    }
                }
            }

            // Update the CPU time deltas.
            PhUpdateDelta(&threadItem->CpuKernelDelta, threadItem->KernelTime.QuadPart);
            PhUpdateDelta(&threadItem->CpuUserDelta, threadItem->UserTime.QuadPart);

            // Update the CPU usage.
            // If the cycle time isn't available, we'll fall back to using the CPU time.
            //if (PhEnableCycleCpuUsage && (threadProvider->ProcessId == SYSTEM_IDLE_PROCESS_ID || threadItem->ThreadHandle))
            //{
            //    threadItem->CpuUsage = (FLOAT)threadItem->CyclesDelta.Delta / PhCpuTotalCycleDelta;
            //}
            //else
            //{
            //    threadItem->CpuUsage = (FLOAT)(threadItem->CpuKernelDelta.Delta + threadItem->CpuUserDelta.Delta) /
            //        (PhCpuKernelDelta.Delta + PhCpuUserDelta.Delta + PhCpuIdleDelta.Delta);
            //}

            // Update the CPU user/kernel usage (based on the same code from procprv.c) (dmex)
            {
                FLOAT newCpuUsage;
                FLOAT kernelCpuUsage;
                FLOAT userCpuUsage;

                if (PhEnableCycleCpuUsage)
                {
                    FLOAT totalDelta;

                    if (threadProvider->ProcessId == SYSTEM_IDLE_PROCESS_ID || threadItem->ThreadHandle)
                    {
                        newCpuUsage = (FLOAT)threadItem->CyclesDelta.Delta / PhCpuTotalCycleDelta;
                    }
                    else
                    {
                        newCpuUsage = (FLOAT)(threadItem->CpuKernelDelta.Delta + threadItem->CpuUserDelta.Delta) /
                            (PhCpuKernelDelta.Delta + PhCpuUserDelta.Delta + PhCpuIdleDelta.Delta);
                    }

                    totalDelta = (FLOAT)(threadItem->CpuKernelDelta.Delta + threadItem->CpuUserDelta.Delta);

                    if (totalDelta != 0)
                    {
                        kernelCpuUsage = newCpuUsage * ((FLOAT)threadItem->CpuKernelDelta.Delta / totalDelta);
                        userCpuUsage = newCpuUsage * ((FLOAT)threadItem->CpuUserDelta.Delta / totalDelta);
                    }
                    else
                    {
                        if (threadItem->UserTime.QuadPart != 0)
                        {
                            kernelCpuUsage = newCpuUsage / 2;
                            userCpuUsage = newCpuUsage / 2;
                        }
                        else
                        {
                            kernelCpuUsage = newCpuUsage;
                            userCpuUsage = 0;
                        }
                    }
                }
                else
                {
                    ULONG64 sysTotalTime;

                    sysTotalTime = PhCpuKernelDelta.Delta + PhCpuUserDelta.Delta + PhCpuIdleDelta.Delta;
                    kernelCpuUsage = (FLOAT)threadItem->CpuKernelDelta.Delta / sysTotalTime;
                    userCpuUsage = (FLOAT)threadItem->CpuUserDelta.Delta / sysTotalTime;
                    newCpuUsage = kernelCpuUsage + userCpuUsage;
                }

                threadItem->CpuUsage = newCpuUsage;
                threadItem->CpuKernelUsage = kernelCpuUsage;
                threadItem->CpuUserUsage = userCpuUsage;
            }

            // Update the base priority increment.
            {
                KPRIORITY oldBasePriorityIncrement = threadItem->BasePriorityIncrement;

                if (threadItem->ThreadHandle && NT_SUCCESS(PhGetThreadBasicInformation(
                    threadItem->ThreadHandle,
                    &basicInfo
                    )))
                {
                    threadItem->BasePriorityIncrement = basicInfo.BasePriority;
                }
                else
                {
                    threadItem->BasePriorityIncrement = THREAD_PRIORITY_ERROR_RETURN;
                }

                if (threadItem->BasePriorityIncrement != oldBasePriorityIncrement)
                {
                    modified = TRUE;
                }
            }

            // Update the GUI thread status.
            if (threadItem->ThreadId)
            {
                GUITHREADINFO info = { sizeof(GUITHREADINFO) };
                BOOLEAN oldIsGuiThread = threadItem->IsGuiThread;

                threadItem->IsGuiThread = !!GetGUIThreadInfo(HandleToUlong(threadItem->ThreadId), &info);

                if (threadItem->IsGuiThread != oldIsGuiThread)
                    modified = TRUE;
            }

            threadItem->JustResolved = FALSE;

            if (modified)
            {
                // Raise the thread modified event.
                PhInvokeCallback(&threadProvider->ThreadModifiedEvent, threadItem);
            }

            PhDereferenceObject(threadItem);
        }
    }

    PhInvokeCallback(&threadProvider->UpdatedEvent, NULL);
    threadProvider->RunId++;
}
