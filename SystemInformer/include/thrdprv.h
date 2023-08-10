/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2023
 *
 */

#ifndef PH_THRDPRV_H
#define PH_THRDPRV_H

extern PPH_OBJECT_TYPE PhThreadProviderType;
extern PPH_OBJECT_TYPE PhThreadItemType;

// begin_phapppub
typedef struct _PH_THREAD_ITEM
{
    HANDLE ThreadId;

    LARGE_INTEGER CreateTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    PH_UINT64_DELTA CpuKernelDelta;
    PH_UINT64_DELTA CpuUserDelta;
    PH_UINT32_DELTA ContextSwitchesDelta;
    PH_UINT64_DELTA CyclesDelta;

    FLOAT CpuUsage;
    FLOAT CpuKernelUsage;
    FLOAT CpuUserUsage;

    KPRIORITY Priority;
    KPRIORITY BasePriority;
    ULONG WaitTime;
    KTHREAD_STATE State;
    KWAIT_REASON WaitReason;
    KPRIORITY BasePriorityIncrement;

    HANDLE ThreadHandle;

    PPH_STRING ServiceName;

    ULONG64 StartAddress;
    PPH_STRING StartAddressString;
    PPH_STRING StartAddressFileName;
    enum _PH_SYMBOL_RESOLVE_LEVEL StartAddressResolveLevel;

    BOOLEAN IsGuiThread;
    BOOLEAN JustResolved;
    WCHAR ThreadIdString[PH_INT32_STR_LEN_1];
    PPH_STRING AlternateThreadIdString;

    IO_COUNTERS IoCounters;
} PH_THREAD_ITEM, *PPH_THREAD_ITEM;

typedef enum _PH_KNOWN_PROCESS_TYPE PH_KNOWN_PROCESS_TYPE;

typedef struct _PH_THREAD_PROVIDER
{
    PPH_HASHTABLE ThreadHashtable;
    PH_FAST_LOCK ThreadHashtableLock;
    PH_CALLBACK ThreadAddedEvent;
    PH_CALLBACK ThreadModifiedEvent;
    PH_CALLBACK ThreadRemovedEvent;
    PH_CALLBACK UpdatedEvent;
    PH_CALLBACK LoadingStateChangedEvent;

    HANDLE ProcessId;
    HANDLE ProcessHandle;

    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN HasServices : 1;
            BOOLEAN HasServicesKnown : 1;
            BOOLEAN Terminating : 1;
            BOOLEAN Spare : 5;
        };
    };

    struct _PH_SYMBOL_PROVIDER *SymbolProvider;

    SLIST_HEADER QueryListHead;
    PH_QUEUED_LOCK LoadSymbolsLock;
    LONG SymbolsLoading;
    ULONG64 RunId;
    ULONG64 SymbolsLoadedRunId;
} PH_THREAD_PROVIDER, *PPH_THREAD_PROVIDER;
// end_phapppub

PPH_THREAD_PROVIDER PhCreateThreadProvider(
    _In_ HANDLE ProcessId
    );

VOID PhRegisterThreadProvider(
    _In_ PPH_THREAD_PROVIDER ThreadProvider,
    _Out_ PPH_CALLBACK_REGISTRATION CallbackRegistration
    );

VOID PhUnregisterThreadProvider(
    _In_ PPH_THREAD_PROVIDER ThreadProvider,
    _In_ PPH_CALLBACK_REGISTRATION CallbackRegistration
    );

VOID PhSetTerminatingThreadProvider(
    _Inout_ PPH_THREAD_PROVIDER ThreadProvider
    );

VOID PhLoadSymbolsThreadProvider(
    _In_ PPH_THREAD_PROVIDER ThreadProvider
    );

PPH_THREAD_ITEM PhCreateThreadItem(
    _In_ HANDLE ThreadId
    );

PPH_THREAD_ITEM PhReferenceThreadItem(
    _In_ PPH_THREAD_PROVIDER ThreadProvider,
    _In_ HANDLE ThreadId
    );

VOID PhDereferenceAllThreadItems(
    _In_ PPH_THREAD_PROVIDER ThreadProvider
    );

PPH_STRING PhGetBasePriorityIncrementString(
    _In_ KPRIORITY Increment
    );

VOID PhThreadProviderInitialUpdate(
    _In_ PPH_THREAD_PROVIDER ThreadProvider
    );

#endif
