/*
 * Process Hacker - 
 *   thread provider
 * 
 * Copyright (C) 2010 wj32
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

#define THRDPRV_PRIVATE
#include <ph.h>
#include <kph.h>

typedef struct _PH_THREAD_QUERY_DATA
{
    PPH_THREAD_PROVIDER ThreadProvider;
    PPH_THREAD_ITEM ThreadItem;
    PPH_STRING StartAddressString;
    PH_SYMBOL_RESOLVE_LEVEL StartAddressResolveLevel;
} PH_THREAD_QUERY_DATA, *PPH_THREAD_QUERY_DATA;

VOID NTAPI PhpThreadProviderDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

NTSTATUS PhpThreadProviderLoadSymbols(
    __in PVOID Parameter
    );

VOID NTAPI PhpThreadItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

BOOLEAN NTAPI PhpThreadHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    );

ULONG NTAPI PhpThreadHashtableHashFunction(
    __in PVOID Entry
    );

PPH_OBJECT_TYPE PhThreadProviderType;
PPH_OBJECT_TYPE PhThreadItemType;

PH_WORK_QUEUE PhThreadProviderWorkQueue;

BOOLEAN PhInitializeThreadProvider()
{
    if (!NT_SUCCESS(PhCreateObjectType(
        &PhThreadProviderType,
        0,
        PhpThreadProviderDeleteProcedure
        )))
        return FALSE;

    if (!NT_SUCCESS(PhCreateObjectType(
        &PhThreadItemType,
        0,
        PhpThreadItemDeleteProcedure
        )))
        return FALSE;

    PhInitializeWorkQueue(&PhThreadProviderWorkQueue, 0, 1, 1000);

    return TRUE;
}

PPH_THREAD_PROVIDER PhCreateThreadProvider(
    __in HANDLE ProcessId
    )
{
    PPH_THREAD_PROVIDER threadProvider;

    if (!NT_SUCCESS(PhCreateObject(
        &threadProvider,
        sizeof(PH_THREAD_PROVIDER),
        0,
        PhThreadProviderType,
        0
        )))
        return NULL;

    threadProvider->ThreadHashtable = PhCreateHashtable(
        sizeof(PPH_THREAD_ITEM),
        PhpThreadHashtableCompareFunction,
        PhpThreadHashtableHashFunction,
        20
        );
    PhInitializeFastLock(&threadProvider->ThreadHashtableLock);

    PhInitializeCallback(&threadProvider->ThreadAddedEvent);
    PhInitializeCallback(&threadProvider->ThreadModifiedEvent);
    PhInitializeCallback(&threadProvider->ThreadRemovedEvent);

    threadProvider->ProcessId = ProcessId;
    threadProvider->SymbolProvider = PhCreateSymbolProvider(ProcessId);

    if (threadProvider->SymbolProvider)
    {
        if (threadProvider->SymbolProvider->IsRealHandle)
            threadProvider->ProcessHandle = threadProvider->SymbolProvider->ProcessHandle;
    }

    PhInitializeEvent(&threadProvider->SymbolsLoadedEvent);
    threadProvider->QueryQueue = PhCreateQueue(1);
    PhInitializeMutex(&threadProvider->QueryQueueLock);

    // Begin loading symbols for the process' modules.
    PhReferenceObject(threadProvider);
    PhQueueWorkQueueItem(
        &PhThreadProviderWorkQueue,
        PhpThreadProviderLoadSymbols,
        threadProvider
        );

    return threadProvider;
}

VOID PhpThreadProviderDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_THREAD_PROVIDER threadProvider = (PPH_THREAD_PROVIDER)Object;

    // Dereference all thread items (we referenced them 
    // when we added them to the hashtable).
    PhDereferenceAllThreadItems(threadProvider);

    PhDereferenceObject(threadProvider->ThreadHashtable);
    PhDeleteFastLock(&threadProvider->ThreadHashtableLock);
    PhDeleteCallback(&threadProvider->ThreadAddedEvent);
    PhDeleteCallback(&threadProvider->ThreadModifiedEvent);
    PhDeleteCallback(&threadProvider->ThreadRemovedEvent);

    // Destroy all queue items.
    {
        PPH_THREAD_QUERY_DATA data;

        while (PhDequeueQueueItem(threadProvider->QueryQueue, &data))
        {
            PhDereferenceObject(data->StartAddressString);
            PhDereferenceObject(data->ThreadItem);
            PhFree(data);
        }
    }

    PhDereferenceObject(threadProvider->QueryQueue);
    PhDeleteMutex(&threadProvider->QueryQueueLock);

    // We don't close the process handle because it is owned by 
    // the symbol provider.
    if (threadProvider->SymbolProvider) PhDereferenceObject(threadProvider->SymbolProvider);
}

static BOOLEAN LoadSymbolsEnumGenericModulesCallback(
    __in PPH_MODULE_INFO Module,
    __in PVOID Context
    )
{
    PPH_SYMBOL_PROVIDER symbolProvider = (PPH_SYMBOL_PROVIDER)Context;

    PhSymbolProviderLoadModule(
        symbolProvider,
        Module->FileName->Buffer,
        (ULONG64)Module->BaseAddress,
        Module->Size
        );

    return TRUE;
}

static BOOLEAN LoadBasicSymbolsEnumGenericModulesCallback(
    __in PPH_MODULE_INFO Module,
    __in PVOID Context
    )
{
    PPH_SYMBOL_PROVIDER symbolProvider = (PPH_SYMBOL_PROVIDER)Context;

    if (
        PhStringEquals2(Module->Name, L"ntdll.dll", TRUE) ||
        PhStringEquals2(Module->Name, L"kernel32.dll", TRUE)
        )
    {
        PhSymbolProviderLoadModule(
            symbolProvider,
            Module->FileName->Buffer,
            (ULONG64)Module->BaseAddress,
            Module->Size
            );
    }

    return TRUE;
}

NTSTATUS PhpThreadProviderLoadSymbols(
    __in PVOID Parameter
    )
{
    PPH_THREAD_PROVIDER threadProvider = (PPH_THREAD_PROVIDER)Parameter;

    if (threadProvider->ProcessId != SYSTEM_IDLE_PROCESS_ID)
    {
        if (
            threadProvider->SymbolProvider->IsRealHandle ||
            threadProvider->ProcessId == SYSTEM_PROCESS_ID
            )
        {
            PhEnumGenericModules(
                threadProvider->ProcessId,
                threadProvider->SymbolProvider->ProcessHandle,
                0,
                LoadSymbolsEnumGenericModulesCallback,
                threadProvider->SymbolProvider
                );
        }
        else
        {
            // We can't enumerate the process modules. Load 
            // symbols for ntdll.dll and kernel32.dll.
            PhEnumGenericModules(
                NtCurrentProcessId(),
                NtCurrentProcess(),
                0,
                LoadBasicSymbolsEnumGenericModulesCallback,
                threadProvider->SymbolProvider
                );
        }

        // Load kernel module symbols as well.
        if (threadProvider->ProcessId != SYSTEM_PROCESS_ID)
        {
            PhEnumGenericModules(
                SYSTEM_PROCESS_ID,
                NULL,
                0,
                LoadSymbolsEnumGenericModulesCallback,
                threadProvider->SymbolProvider
                );
        }
    }
    else
    {
        // System Idle Process has one thread for each CPU,
        // each having a start address at KiIdleLoop. We 
        // need to load symbols for the kernel.

        PRTL_PROCESS_MODULES kernelModules;

        if (NT_SUCCESS(PhEnumKernelModules(&kernelModules)))
        {
            if (kernelModules->NumberOfModules > 0)
            {
                PPH_STRING fileName;
                PPH_STRING newFileName;

                fileName = PhCreateStringFromAnsi(kernelModules->Modules[0].FullPathName);
                newFileName = PhGetFileName(fileName);
                PhDereferenceObject(fileName);

                PhSymbolProviderLoadModule(
                    threadProvider->SymbolProvider,
                    newFileName->Buffer,
                    (ULONG64)kernelModules->Modules[0].ImageBase,
                    kernelModules->Modules[0].ImageSize
                    );
                PhDereferenceObject(newFileName);
            }

            PhFree(kernelModules);
        }
    }

    PhSetEvent(&threadProvider->SymbolsLoadedEvent);

    PhDereferenceObject(threadProvider);

    return STATUS_SUCCESS;
}

PPH_THREAD_ITEM PhCreateThreadItem(
    __in HANDLE ThreadId
    )
{
    PPH_THREAD_ITEM threadItem;

    if (!NT_SUCCESS(PhCreateObject(
        &threadItem,
        sizeof(PH_THREAD_ITEM),
        0,
        PhThreadItemType,
        0
        )))
        return NULL;

    memset(threadItem, 0, sizeof(PH_THREAD_ITEM));
    threadItem->ThreadId = ThreadId;
    PhPrintUInt32(threadItem->ThreadIdString, (ULONG)ThreadId);

    return threadItem;
}

VOID PhpThreadItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_THREAD_ITEM threadItem = (PPH_THREAD_ITEM)Object;

    if (threadItem->ThreadHandle) CloseHandle(threadItem->ThreadHandle);
    if (threadItem->StartAddressString) PhDereferenceObject(threadItem->StartAddressString);
}

BOOLEAN PhpThreadHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    return
        (*(PPH_THREAD_ITEM *)Entry1)->ThreadId ==
        (*(PPH_THREAD_ITEM *)Entry2)->ThreadId;
}

ULONG PhpThreadHashtableHashFunction(
    __in PVOID Entry
    )
{
    return (ULONG)(*(PPH_THREAD_ITEM *)Entry)->ThreadId / 4;
}

PPH_THREAD_ITEM PhReferenceThreadItem(
    __in PPH_THREAD_PROVIDER ThreadProvider,
    __in HANDLE ThreadId
    )
{
    PH_THREAD_ITEM lookupThreadItem;
    PPH_THREAD_ITEM lookupThreadItemPtr = &lookupThreadItem;
    PPH_THREAD_ITEM *threadItemPtr;
    PPH_THREAD_ITEM threadItem;

    lookupThreadItem.ThreadId = ThreadId;

    PhAcquireFastLockShared(&ThreadProvider->ThreadHashtableLock);

    threadItemPtr = (PPH_THREAD_ITEM *)PhGetHashtableEntry(
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
    __in PPH_THREAD_PROVIDER ThreadProvider
    )
{
    ULONG enumerationKey = 0;
    PPH_THREAD_ITEM *threadItem;

    PhAcquireFastLockExclusive(&ThreadProvider->ThreadHashtableLock);

    while (PhEnumHashtable(ThreadProvider->ThreadHashtable, (PPVOID)&threadItem, &enumerationKey))
    {
        PhDereferenceObject(*threadItem);
    }

    PhReleaseFastLockExclusive(&ThreadProvider->ThreadHashtableLock);
}

VOID PhpRemoveThreadItem(
    __in PPH_THREAD_PROVIDER ThreadProvider,
    __in PPH_THREAD_ITEM ThreadItem
    )
{
    PhRemoveHashtableEntry(ThreadProvider->ThreadHashtable, &ThreadItem);
    PhDereferenceObject(ThreadItem);
}

NTSTATUS PhpThreadQueryWorker(
    __in PVOID Parameter
    )
{
    PPH_THREAD_QUERY_DATA data = (PPH_THREAD_QUERY_DATA)Parameter;

    // We can't resolve the start address until symbols have 
    // been loaded.
    PhWaitForEvent(&data->ThreadProvider->SymbolsLoadedEvent, INFINITE);

    data->StartAddressString = PhGetSymbolFromAddress(
        data->ThreadProvider->SymbolProvider,
        data->ThreadItem->StartAddress,
        &data->StartAddressResolveLevel,
        NULL,
        NULL,
        NULL
        );

    PhAcquireMutex(&data->ThreadProvider->QueryQueueLock);
    PhEnqueueQueueItem(data->ThreadProvider->QueryQueue, data);
    PhReleaseMutex(&data->ThreadProvider->QueryQueueLock);

    PhDereferenceObject(data->ThreadProvider);

    return STATUS_SUCCESS;
}

VOID PhpQueueThreadQuery(
    __in PPH_THREAD_PROVIDER ThreadProvider,
    __in PPH_THREAD_ITEM ThreadItem
    )
{
    PPH_THREAD_QUERY_DATA data;

    data = PhAllocate(sizeof(PH_THREAD_QUERY_DATA));
    memset(data, 0, sizeof(PH_THREAD_QUERY_DATA));
    data->ThreadProvider = ThreadProvider;
    data->ThreadItem = ThreadItem;

    PhReferenceObject(ThreadProvider);
    PhReferenceObject(ThreadItem);
    PhQueueGlobalWorkQueueItem(PhpThreadQueryWorker, data);
}

PPH_STRING PhpGetThreadBasicStartAddress(
    __in PPH_THREAD_PROVIDER ThreadProvider,
    __in ULONG64 Address,
    __out PPH_SYMBOL_RESOLVE_LEVEL ResolveLevel
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

        symbol = PhCreateStringEx(NULL, PH_PTR_STR_LEN * 2);
        PhPrintPointer(symbol->Buffer, (PVOID)Address);
        PhTrimStringToNullTerminator(symbol);
    }
    else
    {
        baseName = PhGetBaseName(fileName);
        *ResolveLevel = PhsrlModule;

        symbol = PhFormatString(L"%s+0x%Ix", baseName->Buffer, (PVOID)(Address - modBase));
    }

    if (fileName)
        PhDereferenceObject(fileName);
    if (baseName)
        PhDereferenceObject(baseName);

    return symbol;
}

VOID PhThreadProviderUpdate(
    __in PVOID Object
    )
{
    PPH_THREAD_PROVIDER threadProvider = (PPH_THREAD_PROVIDER)Object;
    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;
    PSYSTEM_THREAD_INFORMATION threads;
    ULONG numberOfThreads;
    ULONG i;

    if (!NT_SUCCESS(PhEnumProcesses(&processes)))
        return;

    process = PhFindProcessInformation(processes, threadProvider->ProcessId);

    if (!process)
        return;

    threads = process->Threads;
    numberOfThreads = process->NumberOfThreads;

    // System Idle Process has one thread per CPU. 
    // They all have a TID of 0, but we can't have 
    // multiple TIDs, so we'll assign unique TIDs.
    if (threadProvider->ProcessId == SYSTEM_IDLE_PROCESS_ID)
    {
        for (i = 0; i < numberOfThreads; i++)
        {
            threads[i].ClientId.UniqueThread = (HANDLE)i;
        }
    }

    // Look for dead threads.
    {
        PPH_LIST threadsToRemove = NULL;
        ULONG enumerationKey = 0;
        PPH_THREAD_ITEM *threadItem;

        while (PhEnumHashtable(threadProvider->ThreadHashtable, (PPVOID)&threadItem, &enumerationKey))
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

                PhAddListItem(threadsToRemove, *threadItem);
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
        PPH_THREAD_QUERY_DATA data;

        PhAcquireMutex(&threadProvider->QueryQueueLock);

        while (PhDequeueQueueItem(threadProvider->QueryQueue, &data))
        {
            PhReleaseMutex(&threadProvider->QueryQueueLock);

            if (data->StartAddressResolveLevel == PhsrlFunction)
            {
                PhSwapReference(&data->ThreadItem->StartAddressString, data->StartAddressString);
                data->ThreadItem->StartAddressResolveLevel = data->StartAddressResolveLevel;
            }

            data->ThreadItem->JustResolved = TRUE;

            PhDereferenceObject(data->ThreadItem);
            PhFree(data);
            PhAcquireMutex(&threadProvider->QueryQueueLock);
        }

        PhReleaseMutex(&threadProvider->QueryQueueLock);
    }

    // Look for new threads and update existing ones.
    for (i = 0; i < numberOfThreads; i++)
    {
        PSYSTEM_THREAD_INFORMATION thread = &threads[i];
        PPH_THREAD_ITEM threadItem;

        threadItem = PhReferenceThreadItem(threadProvider, thread->ClientId.UniqueThread);

        if (!threadItem)
        {
            PVOID startAddress = NULL;

            threadItem = PhCreateThreadItem(thread->ClientId.UniqueThread);

            PhUpdateDelta(&threadItem->ContextSwitchesDelta, thread->ContextSwitches);
            threadItem->Priority = thread->Priority;
            threadItem->WaitReason = thread->WaitReason;

            // Try to open a handle to the thread.
            if (!NT_SUCCESS(PhOpenThread(
                &threadItem->ThreadHandle,
                THREAD_QUERY_INFORMATION,
                threadItem->ThreadId
                )))
            {
                PhOpenThread(
                    &threadItem->ThreadHandle,
                    ThreadQueryAccess,
                    threadItem->ThreadId
                    );
            }

            // Get the cycle count.
            if (WindowsVersion >= WINDOWS_VISTA)
            {
                ULONG64 cycles;

                if (NT_SUCCESS(PhGetThreadCycleTime(
                    threadItem->ThreadHandle,
                    &cycles
                    )))
                {
                    PhUpdateDelta(&threadItem->CyclesDelta, cycles);
                }
            }

            // Try to get the start address.

            if (threadItem->ThreadHandle)
            {
                if (PhKphHandle)
                {
                    KphGetThreadStartAddress(
                        PhKphHandle,
                        threadItem->ThreadHandle,
                        &startAddress
                        );
                }

                if (!startAddress)
                {
                    NtQueryInformationThread(
                        threadItem->ThreadHandle,
                        ThreadQuerySetWin32StartAddress,
                        &startAddress,
                        sizeof(PVOID),
                        NULL
                        );
                }
            }

            if (!startAddress)
                startAddress = thread->StartAddress;

            threadItem->StartAddress = (ULONG64)startAddress;

            if (PhWaitForEvent(&threadProvider->SymbolsLoadedEvent, 0))
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
                threadItem->StartAddressString = PhCreateStringEx(NULL, PH_PTR_STR_LEN * 2);
                PhPrintPointer(
                    threadItem->StartAddressString->Buffer,
                    (PVOID)threadItem->StartAddress
                    );
                PhTrimStringToNullTerminator(threadItem->StartAddressString);
            }

            PhpQueueThreadQuery(threadProvider, threadItem);

            // Is it a GUI thread?

            if (threadItem->ThreadHandle && PhKphHandle)
            {
                PVOID win32Thread;

                if (NT_SUCCESS(KphGetThreadWin32Thread(
                    PhKphHandle,
                    threadItem->ThreadHandle,
                    &win32Thread
                    )))
                {
                    threadItem->IsGuiThread = win32Thread != NULL;
                }
            }

            // Add the thread item to the hashtable.
            PhAcquireFastLockExclusive(&threadProvider->ThreadHashtableLock);
            PhAddHashtableEntry(threadProvider->ThreadHashtable, &threadItem);
            PhReleaseFastLockExclusive(&threadProvider->ThreadHashtableLock);

            // Raise the thread added event.
            PhInvokeCallback(&threadProvider->ThreadAddedEvent, threadItem);
        }
        else
        {
            BOOLEAN modified = FALSE;

            if (threadItem->JustResolved)
                modified = TRUE;

            // If the resolve level is only at address, it probably 
            // means symbols weren't loaded the last time we 
            // tried to get the start address. Try again.
            if (threadItem->StartAddressResolveLevel == PhsrlAddress)
            {
                if (PhWaitForEvent(&threadProvider->SymbolsLoadedEvent, 0))
                {
                    PPH_STRING newStartAddressString;

                    newStartAddressString = PhpGetThreadBasicStartAddress(
                        threadProvider,
                        threadItem->StartAddress,
                        &threadItem->StartAddressResolveLevel
                        );

                    PhSwapReference(
                        &threadItem->StartAddressString,
                        newStartAddressString
                        );

                    modified = TRUE;
                }
            }

            // If we couldn't resolve the start address to a 
            // module+offset, use the StartAddress instead 
            // of the Win32StartAddress and try again.
            // Note that we check the resolve level again 
            // because we may have changed it in the previous 
            // block.
            if (
                threadItem->JustResolved &&
                threadItem->StartAddressResolveLevel == PhsrlAddress
                )
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
                    WCHAR deltaString[PH_INT32_STR_LEN_1];

                    if (threadItem->ContextSwitchesDelta.Delta != 0)
                    {
                        PhPrintUInt32(deltaString, threadItem->ContextSwitchesDelta.Delta);
                        PhSwapReference(
                            &threadItem->ContextSwitchesDeltaString,
                            PhFormatDecimal(deltaString, 0, TRUE)
                            );
                    }
                    else
                    {
                        PhSwapReference(
                            &threadItem->ContextSwitchesDeltaString,
                            PhCreateString(L"")
                            );
                    }

                    modified = TRUE;
                }
            }

            // Update the cycle count.
            if (WindowsVersion >= WINDOWS_VISTA)
            {
                ULONG64 cycles;
                ULONG64 oldDelta;

                oldDelta = threadItem->CyclesDelta.Delta;

                if (NT_SUCCESS(PhGetThreadCycleTime(
                    threadItem->ThreadHandle,
                    &cycles
                    )))
                {
                    PhUpdateDelta(&threadItem->CyclesDelta, cycles);

                    if (threadItem->CyclesDelta.Delta != oldDelta)
                    {
                        WCHAR deltaString[PH_INT64_STR_LEN_1];

                        if (threadItem->CyclesDelta.Delta != 0)
                        {
                            PhPrintUInt64(deltaString, threadItem->CyclesDelta.Delta);
                            PhSwapReference(
                                &threadItem->CyclesDeltaString,
                                PhFormatDecimal(deltaString, 0, TRUE)
                                );
                        }
                        else
                        {
                            PhSwapReference(
                                &threadItem->CyclesDeltaString,
                                PhCreateString(L"")
                                );
                        }

                        modified = TRUE;
                    }
                }
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

    PhFree(processes);
}
