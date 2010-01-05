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

VOID NTAPI PhpThreadProviderDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
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

    // We don't close the process handle because it is owned by 
    // the symbol provider.
    if (threadProvider->SymbolProvider) PhDereferenceObject(threadProvider->SymbolProvider);
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
    PhPrintInteger(threadItem->ThreadIdString, (ULONG)ThreadId);

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

    // Look for new threads.
    for (i = 0; i < numberOfThreads; i++)
    {
        PSYSTEM_THREAD_INFORMATION thread = &threads[i];
        PPH_THREAD_ITEM threadItem;

        threadItem = PhReferenceThreadItem(threadProvider, thread->ClientId.UniqueThread);

        if (!threadItem)
        {
            PVOID startAddress;

            threadItem = PhCreateThreadItem(thread->ClientId.UniqueThread);

            threadItem->ContextSwitches = thread->ContextSwitches;
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

            threadItem->StartAddressString = PhCreateStringEx(NULL, 20);
            PhPrintPointer(threadItem->StartAddressString->Buffer, startAddress);

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
            PhDereferenceObject(threadItem);
        }
    }

    PhFree(processes);
}
