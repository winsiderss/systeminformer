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
#include <kphuser.h>

#include <extmgri.h>
#include <procprv.h>

typedef struct _PH_THREAD_QUERY_DATA
{
    SLIST_ENTRY ListEntry;
    PPH_THREAD_PROVIDER ThreadProvider;
    PPH_THREAD_ITEM ThreadItem;
    ULONG64 RunId;

    PPH_STRING StartAddressWin32String;
    PPH_STRING StartAddressWin32FileName;
    PH_SYMBOL_RESOLVE_LEVEL StartAddressWin32ResolveLevel;

    PPH_STRING StartAddressString;
    PPH_STRING StartAddressFileName;
    PH_SYMBOL_RESOLVE_LEVEL StartAddressResolveLevel;

    PPH_STRING ServiceName;
} PH_THREAD_QUERY_DATA, *PPH_THREAD_QUERY_DATA;

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

    PhInitializeSListHead(&threadProvider->QueryListHead);
    PhInitializeQueuedLock(&threadProvider->LoadSymbolsLock);

    threadProvider->RunId = 1;
    threadProvider->SymbolsLoadedRunId = 0; // Force symbols to be loaded the first time we try to resolve an address

    PhEmCallObjectOperation(EmThreadProviderType, threadProvider, EmObjectCreate);

    return threadProvider;
}

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
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

VOID PhLoadSymbolsThreadProvider(
    _In_ PPH_THREAD_PROVIDER ThreadProvider
    )
{
    ULONG64 runId;

    PhAcquireQueuedLockExclusive(&ThreadProvider->LoadSymbolsLock);
    runId = ThreadProvider->RunId;
    PhLoadSymbolProviderOptions(ThreadProvider->SymbolProvider);

    PhLoadSymbolProviderModules(
        ThreadProvider->SymbolProvider,
        ThreadProvider->ProcessId
        );

    ThreadProvider->SymbolsLoadedRunId = runId;
    PhReleaseQueuedLockExclusive(&ThreadProvider->LoadSymbolsLock);
}

PPH_THREAD_ITEM PhCreateThreadItem(
    _In_ CLIENT_ID ClientId
    )
{
    PPH_THREAD_ITEM threadItem;

    threadItem = PhCreateObject(
        PhEmGetObjectSize(EmThreadItemType, sizeof(PH_THREAD_ITEM)),
        PhThreadItemType
        );
    memset(threadItem, 0, sizeof(PH_THREAD_ITEM));
    threadItem->ClientId = ClientId;

    PhPrintUInt32(threadItem->ThreadIdString, HandleToUlong(ClientId.UniqueThread));
    PhPrintUInt32IX(threadItem->ThreadIdHexString, HandleToUlong(ClientId.UniqueThread));

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
    if (threadItem->StartAddressWin32String) PhDereferenceObject(threadItem->StartAddressWin32String);
    if (threadItem->StartAddressWin32FileName) PhDereferenceObject(threadItem->StartAddressWin32FileName);
    if (threadItem->ServiceName) PhDereferenceObject(threadItem->ServiceName);
    if (threadItem->AffinityMasks) PhFree(threadItem->AffinityMasks);
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
        goto CleanupExit;

    newSymbolsLoading = _InterlockedIncrement(&data->ThreadProvider->SymbolsLoading);

    if (newSymbolsLoading == 1)
        PhInvokeCallback(&data->ThreadProvider->LoadingStateChangedEvent, UlongToPtr(TRUE));

    if (data->ThreadProvider->SymbolsLoadedRunId == 0)
        PhLoadSymbolsThreadProvider(data->ThreadProvider);

    // Start address
    {
        if (data->ThreadItem->StartAddressWin32)
        {
            data->StartAddressWin32String = PhGetSymbolFromAddress(
                data->ThreadProvider->SymbolProvider,
                data->ThreadItem->StartAddressWin32,
                &data->StartAddressWin32ResolveLevel,
                &data->StartAddressWin32FileName,
                NULL,
                NULL
                );

            if (data->StartAddressWin32ResolveLevel == PhsrlAddress && data->ThreadProvider->SymbolsLoadedRunId < data->RunId)
            {
                // The process may have loaded new modules, so load symbols for those and try again.

                PhLoadSymbolsThreadProvider(data->ThreadProvider);

                PhClearReference(&data->StartAddressWin32String);
                PhClearReference(&data->StartAddressWin32FileName);
                data->StartAddressWin32String = PhGetSymbolFromAddress(
                    data->ThreadProvider->SymbolProvider,
                    data->ThreadItem->StartAddressWin32,
                    &data->StartAddressWin32ResolveLevel,
                    &data->StartAddressWin32FileName,
                    NULL,
                    NULL
                    );
            }
        }

        if (data->ThreadItem->StartAddress)
        {
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
        }
    }

    newSymbolsLoading = _InterlockedDecrement(&data->ThreadProvider->SymbolsLoading);

    if (newSymbolsLoading == 0)
        PhInvokeCallback(&data->ThreadProvider->LoadingStateChangedEvent, UlongToPtr(FALSE));

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
    if (data->ThreadItem->ThreadHandle)
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

CleanupExit:
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

    data = PhAllocateZero(sizeof(PH_THREAD_QUERY_DATA));
    PhSetReference(&data->ThreadProvider, ThreadProvider);
    PhSetReference(&data->ThreadItem, ThreadItem);
    data->RunId = ThreadProvider->RunId;

    PhpQueueThreadWorkQueueItem(PhpThreadQueryWorker, data);
}

PPH_STRING PhpGetThreadBasicStartAddress(
    _In_ PPH_THREAD_PROVIDER ThreadProvider,
    _In_ PVOID Address,
    _Out_ PPH_SYMBOL_RESOLVE_LEVEL ResolveLevel
    )
{
    PVOID modBase;
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
        PhPrintPointer(symbol->Buffer, Address);
        PhTrimToNullTerminatorString(symbol);
    }
    else
    {
        PH_FORMAT format[3];

        baseName = PhGetBaseName(fileName);
        *ResolveLevel = PhsrlModule;

        PhInitFormatSR(&format[0], baseName->sr);
        PhInitFormatS(&format[1], L"+0x");
        PhInitFormatIX(&format[2], (ULONG_PTR)Address - (ULONG_PTR)modBase);

        symbol = PhFormat(format, 3, baseName->Length + 6 + 32);
    }

    if (fileName)
        PhDereferenceObject(fileName);
    if (baseName)
        PhDereferenceObject(baseName);

    return symbol;
}

static NTSTATUS PhThreadProviderOpenThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ PPH_THREAD_ITEM ThreadItem
    )
{
    NTSTATUS status;
    HANDLE threadHandle;

    if (ThreadItem->ProcessId == SYSTEM_IDLE_PROCESS_ID)
    {
        if (HandleToUlong(ThreadItem->ThreadId) < PhSystemProcessorInformation.NumberOfProcessors)
            return STATUS_UNSUCCESSFUL;
    }

    status = PhOpenThreadClientId(
        &threadHandle,
        THREAD_QUERY_INFORMATION,
        &ThreadItem->ClientId
        );

    if (!NT_SUCCESS(status))
    {
        status = PhOpenThreadClientId(
            &threadHandle,
            THREAD_QUERY_LIMITED_INFORMATION,
            &ThreadItem->ClientId
            );
    }

    if (NT_SUCCESS(status))
    {
        *ThreadHandle = threadHandle;
    }

    return status;
}

static NTSTATUS PhpGetThreadCycleTime(
    _In_ PPH_THREAD_ITEM ThreadItem,
    _Out_ PULONG64 CycleTime
    )
{
    if (ThreadItem->ProcessId == SYSTEM_IDLE_PROCESS_ID)
    {
        if (HandleToUlong(ThreadItem->ThreadId) < PhSystemProcessorInformation.NumberOfProcessors)
        {
            *CycleTime = PhCpuIdleCycleTime[HandleToUlong(ThreadItem->ThreadId)].CycleTime;
            return STATUS_SUCCESS;
        }
    }

    if (ThreadItem->ThreadHandle)
    {
        return PhGetThreadCycleTime(ThreadItem->ThreadHandle, CycleTime);
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

_Function_class_(PH_CALLBACK_FUNCTION)
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

        //
        // We want dbghelp.dll to release the symbol files so that the exited process symbols can
        // be rebuilt. This is most common for developers building, inspecting with system informer,
        // then rebuilding without closing the process properties dialog.
        //
        // There are downstream dependencies on the symbol provider and the process handle from it,
        // so we can't just dereference it immediately here.
        //
        // PhUnregisterSymbolProvider will unregister this symbol provider from dbghelp.dll which
        // will get it to release the symbol files. This gives us the desired result without
        // breaking downstream logic. The consequence is that symbol resolution downstream will
        // fail cleanly, but it shouldn't matter in this path as it relies on being able to query
        // information about the process threads, which no longer exist.
        //
        if (threadProvider->SymbolProvider)
            PhUnregisterSymbolProvider(threadProvider->SymbolProvider);
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

            if (data->StartAddressWin32ResolveLevel == PhsrlFunction && data->StartAddressWin32String)
            {
                PhSwapReference(&data->ThreadItem->StartAddressWin32String, data->StartAddressWin32String);
                PhSwapReference(&data->ThreadItem->StartAddressWin32FileName, data->StartAddressWin32FileName);
                data->ThreadItem->StartAddressWin32ResolveLevel = data->StartAddressWin32ResolveLevel;
            }

            if (data->StartAddressResolveLevel == PhsrlFunction && data->StartAddressString)
            {
                PhSwapReference(&data->ThreadItem->StartAddressString, data->StartAddressString);
                PhSwapReference(&data->ThreadItem->StartAddressFileName, data->StartAddressFileName);
                data->ThreadItem->StartAddressResolveLevel = data->StartAddressResolveLevel;
            }

            PhMoveReference(&data->ThreadItem->ServiceName, data->ServiceName);

            data->ThreadItem->JustResolved = TRUE;

            if (data->StartAddressWin32String) PhDereferenceObject(data->StartAddressWin32String);
            if (data->StartAddressWin32FileName) PhDereferenceObject(data->StartAddressWin32FileName);
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
            threadItem = PhCreateThreadItem(thread->ClientId);
            threadItem->KernelTime = thread->KernelTime;
            PhUpdateDelta(&threadItem->CpuKernelDelta, threadItem->KernelTime.QuadPart);
            threadItem->UserTime = thread->UserTime;
            PhUpdateDelta(&threadItem->CpuUserDelta, threadItem->UserTime.QuadPart);
            threadItem->CreateTime = thread->CreateTime;
            threadItem->StartAddress = thread->StartAddress;
            threadItem->Priority = thread->Priority;
            threadItem->BasePriority = thread->BasePriority;
            PhUpdateDelta(&threadItem->ContextSwitchesDelta, thread->ContextSwitches);
            threadItem->State = thread->ThreadState;
            threadItem->WaitReason = thread->WaitReason;

            {
                NTSTATUS status;
                HANDLE threadHandle;

                status = PhThreadProviderOpenThread(
                    &threadHandle,
                    threadItem
                    );

                if (NT_SUCCESS(status))
                {
                    threadItem->ThreadHandle = threadHandle;
                }

                threadItem->ThreadHandleStatus = threadItem->StartAddressStatus = status;
            }

            // Get the cycle count.
            if (threadItem->ThreadHandle)
            {
                ULONG64 cycleTime;

                if (NT_SUCCESS(PhGetThreadCycleTime(
                    threadItem->ThreadHandle,
                    &cycleTime
                    )))
                {
                    PhUpdateDelta(&threadItem->CyclesDelta, cycleTime);
                }
            }

            // Get the start address.

            if (threadItem->ThreadHandle)
            {
                NTSTATUS status;
                ULONG_PTR startAddress;

                status = PhGetThreadStartAddress(
                    threadItem->ThreadHandle,
                    &startAddress
                    );

                if (NT_SUCCESS(status))
                {
                    threadItem->StartAddressWin32 = (PVOID)startAddress;
                }

                threadItem->StartAddressStatus = status;
            }

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

            // Affinity
            {
                ULONG affinityPopulationCount = 0;

                threadItem->AffinityMasks = PhAllocateZero(sizeof(KAFFINITY) * PhSystemProcessorInformation.NumberOfProcessorGroups);

                for (USHORT i = 0; i < PhSystemProcessorInformation.NumberOfProcessorGroups; i++)
                {
                    GROUP_AFFINITY affinity;

                    affinity.Group = i;

                    if (threadItem->ThreadHandle &&
                        NT_SUCCESS(PhGetThreadGroupAffinity(threadItem->ThreadHandle, &affinity)))
                    {
                        threadItem->AffinityMasks[i] = affinity.Mask;
                    }
                    else
                    {
                        threadItem->AffinityMasks[i] = PhSystemProcessorInformation.ActiveProcessorsAffinityMasks[i];
                    }

                    affinityPopulationCount += PhCountBitsUlongPtr(threadItem->AffinityMasks[i]);
                }

                threadItem->AffinityPopulationCount = affinityPopulationCount;
            }

            // Start address
            {
                // Win32
                if (threadProvider->SymbolsLoadedRunId != 0 && threadItem->StartAddressWin32)
                {
                    threadItem->StartAddressWin32String = PhpGetThreadBasicStartAddress(
                        threadProvider,
                        threadItem->StartAddressWin32,
                        &threadItem->StartAddressWin32ResolveLevel
                        );
                }

                if (PhIsNullOrEmptyString(threadItem->StartAddressWin32String) && threadItem->StartAddressWin32)
                {
                    threadItem->StartAddressWin32ResolveLevel = PhsrlAddress;
                    threadItem->StartAddressWin32String = PhCreateStringEx(NULL, PH_PTR_STR_LEN * sizeof(WCHAR));
                    PhPrintPointer(
                        threadItem->StartAddressWin32String->Buffer,
                        threadItem->StartAddressWin32
                        );
                    PhTrimToNullTerminatorString(threadItem->StartAddressWin32String);
                }

                // Native
                if (threadProvider->SymbolsLoadedRunId != 0 && threadItem->StartAddress)
                {
                    threadItem->StartAddressString = PhpGetThreadBasicStartAddress(
                        threadProvider,
                        threadItem->StartAddress,
                        &threadItem->StartAddressResolveLevel
                        );
                }

                if (PhIsNullOrEmptyString(threadItem->StartAddressString) && threadItem->StartAddress)
                {
                    threadItem->StartAddressResolveLevel = PhsrlAddress;
                    threadItem->StartAddressString = PhCreateStringEx(NULL, PH_PTR_STR_LEN * sizeof(WCHAR));
                    PhPrintPointer(
                        threadItem->StartAddressString->Buffer,
                        threadItem->StartAddress
                        );
                    PhTrimToNullTerminatorString(threadItem->StartAddressString);
                }
            }

            // Is it a GUI thread?
            if (threadItem->ThreadId)
            {
                threadItem->IsGuiThread = PhGetThreadWin32Thread(threadItem->ThreadId);
            }

            if (WindowsVersion >= WINDOWS_10_22H2 && threadItem->ThreadHandle)
            {
                if (KsiLevel() >= KphLevelMed) // threadItem->IsSubsystemProcess
                {
                    ULONG lxssThreadId;

                    if (NT_SUCCESS(KphQueryInformationThread(
                        threadItem->ThreadHandle,
                        KphThreadWSLThreadId,
                        &lxssThreadId,
                        sizeof(ULONG),
                        NULL
                        )))
                    {
                        threadItem->LxssThreadId = lxssThreadId;
                        PhPrintUInt32(threadItem->LxssThreadIdString, lxssThreadId);
                    }
                }
            }

            if (WindowsVersion >= WINDOWS_11_22H2 && threadItem->ThreadHandle)
            {
                POWER_THROTTLING_THREAD_STATE powerThrottlingState;

                if (NT_SUCCESS(PhGetThreadPowerThrottlingState(threadItem->ThreadHandle, &powerThrottlingState)))
                {
                    if (powerThrottlingState.ControlMask & POWER_THROTTLING_THREAD_EXECUTION_SPEED &&
                        powerThrottlingState.StateMask & POWER_THROTTLING_THREAD_EXECUTION_SPEED)
                    {
                        threadItem->PowerThrottling = TRUE;
                    }
                }
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

            {
                // If the resolve level is only at address, it probably
                // means symbols weren't loaded the last time we
                // tried to get the start address. Try again.
                if (threadItem->StartAddressWin32ResolveLevel == PhsrlAddress)
                {
                    if (threadProvider->SymbolsLoadedRunId != 0 && threadItem->StartAddressWin32)
                    {
                        PPH_STRING newStartAddressString;

                        newStartAddressString = PhpGetThreadBasicStartAddress(
                            threadProvider,
                            threadItem->StartAddressWin32,
                            &threadItem->StartAddressWin32ResolveLevel
                            );

                        PhMoveReference(
                            &threadItem->StartAddressWin32String,
                            newStartAddressString
                            );

                        modified = TRUE;
                    }
                }

                if (threadItem->StartAddressResolveLevel == PhsrlAddress)
                {
                    if (threadProvider->SymbolsLoadedRunId != 0 && threadItem->StartAddress)
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
                    if (threadItem->StartAddress != thread->StartAddress)
                    {
                        threadItem->StartAddress = thread->StartAddress;
                        PhpQueueThreadQuery(threadProvider, threadItem);
                    }
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
                ULONG64 totalTime;

                if (PhEnableCycleCpuUsage)
                {
                    ULONG64 totalDelta;

                    if (threadProvider->ProcessId == SYSTEM_IDLE_PROCESS_ID || threadItem->ThreadHandle)
                    {
                        newCpuUsage = (FLOAT)threadItem->CyclesDelta.Delta / PhCpuTotalCycleDelta;
                    }
                    else
                    {
                        totalTime = PhCpuKernelDelta.Delta + PhCpuUserDelta.Delta + PhCpuIdleDelta.Delta;
                        newCpuUsage = (FLOAT)(threadItem->CpuKernelDelta.Delta + threadItem->CpuUserDelta.Delta) / totalTime;
                    }

                    totalDelta = (threadItem->CpuKernelDelta.Delta + threadItem->CpuUserDelta.Delta);

                    if (totalDelta != 0)
                    {
                        kernelCpuUsage = newCpuUsage * (FLOAT)threadItem->CpuKernelDelta.Delta / totalDelta;
                        userCpuUsage = newCpuUsage * (FLOAT)threadItem->CpuUserDelta.Delta / totalDelta;
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
                    totalTime = PhCpuKernelDelta.Delta + PhCpuUserDelta.Delta + PhCpuIdleDelta.Delta;
                    kernelCpuUsage = (FLOAT)threadItem->CpuKernelDelta.Delta / totalTime;
                    userCpuUsage = (FLOAT)threadItem->CpuUserDelta.Delta / totalTime;
                    newCpuUsage = kernelCpuUsage + userCpuUsage;
                }

                threadItem->CpuUsage = newCpuUsage;
                threadItem->CpuKernelUsage = kernelCpuUsage;
                threadItem->CpuUserUsage = userCpuUsage;
            }

            // Update the base priority increment.
            // Update the thread affinity.
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

            // Affinity
            {
                ULONG affinityPopulationCount = 0;

                for (USHORT i = 0; i < PhSystemProcessorInformation.NumberOfProcessorGroups; i++)
                {
                    GROUP_AFFINITY affinity;

                    affinity.Group = i;

                    if (threadItem->ThreadHandle &&
                        NT_SUCCESS(PhGetThreadGroupAffinity(threadItem->ThreadHandle, &affinity)))
                    {
                        threadItem->AffinityMasks[i] = affinity.Mask;
                    }
                    else
                    {
                        threadItem->AffinityMasks[i] = PhSystemProcessorInformation.ActiveProcessorsAffinityMasks[i];
                    }

                    affinityPopulationCount += PhCountBitsUlongPtr(threadItem->AffinityMasks[i]);
                }

                if (threadItem->AffinityPopulationCount != affinityPopulationCount)
                {
                    threadItem->AffinityPopulationCount = affinityPopulationCount;
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

            if (!threadItem->ThreadHandle || KsiLevel() < KphLevelMed ||
                !NT_SUCCESS(KphQueryInformationThread(
                    threadItem->ThreadHandle,
                    KphThreadIoCounters,
                    &threadItem->IoCounters,
                    sizeof(IO_COUNTERS),
                    NULL
                    )))
            {
                RtlZeroMemory(&threadItem->IoCounters, sizeof(IO_COUNTERS));
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
