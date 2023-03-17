/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *
 */

/*
 * This is a simple debugging console which is able to explore phlib's
 * systems easily. Commands are provided to debug reference counting
 * problems and memory usage, as well as to do general performance testing.
 */

#include <phapp.h>

#include <phintrnl.h>
#include <refp.h>
#include <settings.h>
#include <symprv.h>
#include <mapldr.h>
#include <workqueue.h>
#include <workqueuep.h>

#include <procprv.h>
#include <srvprv.h>
#include <thrdprv.h>

typedef struct _STRING_TABLE_ENTRY
{
    PPH_STRING String;
    ULONG_PTR Count;
} STRING_TABLE_ENTRY, *PSTRING_TABLE_ENTRY;

BOOL ConsoleHandlerRoutine(
    _In_ ULONG dwCtrlType
    );

VOID PhpPrintHashtableStatistics(
    _In_ PPH_HASHTABLE Hashtable
    );

NTSTATUS PhpDebugConsoleThreadStart(
    _In_ PVOID Parameter
    );

extern PH_FREE_LIST PhObjectSmallFreeList;

static HANDLE DebugConsoleThreadHandle;
static PPH_SYMBOL_PROVIDER DebugConsoleSymbolProvider;

static PPH_HASHTABLE ObjectListSnapshot = NULL;
#ifdef DEBUG
static PPH_LIST NewObjectList = NULL;
static PH_QUEUED_LOCK NewObjectListLock;
#endif

static BOOLEAN ShowAllLeaks = FALSE;
static BOOLEAN InLeakDetection = FALSE;
static ULONG NumberOfLeaks;
static ULONG NumberOfLeaksShown;

VOID PhShowDebugConsole(
    VOID
    )
{
    if (AllocConsole())
    {
        HMENU menu;

        // Disable the close button because it's impossible to handle
        // those events.
        menu = GetSystemMenu(GetConsoleWindow(), FALSE);
        EnableMenuItem(menu, SC_CLOSE, MF_GRAYED | MF_DISABLED);
        DeleteMenu(menu, SC_CLOSE, 0);

        // Set a handler so we can catch Ctrl+C and Ctrl+Break.
        SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE);

        _wfreopen(L"CONOUT$", L"w", stdout);
        _wfreopen(L"CONOUT$", L"w", stderr);
        _wfreopen(L"CONIN$", L"r", stdin);

        PhCreateThreadEx(&DebugConsoleThreadHandle, PhpDebugConsoleThreadStart, NULL);
    }
    else
    {
        HWND consoleWindow;

        consoleWindow = GetConsoleWindow();

        // Console window already exists, so bring it to the top.
        if (IsMinimized(consoleWindow))
            ShowWindow(consoleWindow, SW_RESTORE);
        else
            BringWindowToTop(consoleWindow);

        return;
    }
}

VOID PhCloseDebugConsole(
    VOID
    )
{
    _wfreopen(L"NUL", L"w", stdout);
    _wfreopen(L"NUL", L"w", stderr);
    _wfreopen(L"NUL", L"r", stdin);

    FreeConsole();
}

static BOOL ConsoleHandlerRoutine(
    _In_ ULONG dwCtrlType
    )
{
    switch (dwCtrlType)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
        PhCloseDebugConsole();
        return TRUE;
    }

    return FALSE;
}

static BOOLEAN NTAPI PhpLoadCurrentProcessSymbolsCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_ PVOID Context
    )
{
    if (!PhLoadModuleSymbolProvider(
        (PPH_SYMBOL_PROVIDER)Context,
        Module->FileName,
        (ULONG64)Module->BaseAddress,
        Module->Size
        ))
    {
        wprintf(L"Unable to load symbols: %s\n", PhGetStringOrEmpty(Module->FileName));
    }

    return TRUE;
}

static PWSTR PhpGetSymbolForAddress(
    _In_ PVOID Address
    )
{
    return PH_AUTO_T(PH_STRING, PhGetSymbolFromAddress(
        DebugConsoleSymbolProvider, (ULONG64)Address, NULL, NULL, NULL, NULL
        ))->Buffer;
}

static VOID PhpPrintObjectInfo(
    _In_ PPH_OBJECT_HEADER ObjectHeader,
    _In_ LONG RefToSubtract
    )
{
    PVOID object;
    PPH_OBJECT_TYPE objectType;
    WCHAR c = L' ';

    object = PhObjectHeaderToObject(ObjectHeader);
    wprintf(L"%Ix", (ULONG_PTR)object);
    objectType = PhGetObjectType(object);

    wprintf(L"\t% 20s", objectType->Name);

    if (ObjectHeader->Flags & PH_OBJECT_FROM_SMALL_FREE_LIST)
        c = L'f';
    else if (ObjectHeader->Flags & PH_OBJECT_FROM_TYPE_FREE_LIST)
        c = L'F';

    wprintf(L"\t%4d %c", ObjectHeader->RefCount - RefToSubtract, c);

    if (!objectType)
    {
        // Dummy
    }
    else if (objectType == PhObjectTypeObject)
    {
        wprintf(L"\t%.32s", ((PPH_OBJECT_TYPE)object)->Name);
    }
    else if (objectType == PhStringType)
    {
        wprintf(L"\t%.32s", ((PPH_STRING)object)->Buffer);
    }
    else if (objectType == PhBytesType)
    {
        wprintf(L"\t%.32S", ((PPH_BYTES)object)->Buffer);
    }
    else if (objectType == PhListType)
    {
        wprintf(L"\tCount: %u", ((PPH_LIST)object)->Count);
    }
    else if (objectType == PhPointerListType)
    {
        wprintf(L"\tCount: %u", ((PPH_POINTER_LIST)object)->Count);
    }
    else if (objectType == PhHashtableType)
    {
        wprintf(L"\tCount: %u", ((PPH_HASHTABLE)object)->Count);
    }
    else if (objectType == PhProcessItemType)
    {
        wprintf(
            L"\t%.28s (%lu)",
            ((PPH_PROCESS_ITEM)object)->ProcessName->Buffer,
            HandleToUlong(((PPH_PROCESS_ITEM)object)->ProcessId)
            );
    }
    else if (objectType == PhServiceItemType)
    {
        wprintf(L"\t%s", ((PPH_SERVICE_ITEM)object)->Name->Buffer);
    }
    else if (objectType == PhThreadItemType)
    {
        wprintf(L"\tTID: %lu", HandleToUlong(((PPH_THREAD_ITEM)object)->ThreadId));
    }

    wprintf(L"\n");
}

static VOID PhpDumpObjectInfo(
    _In_ PPH_OBJECT_HEADER ObjectHeader
    )
{
    PVOID object;
    PPH_OBJECT_TYPE objectType;

    object = PhObjectHeaderToObject(ObjectHeader);
    objectType = PhGetObjectType(object);

    __try
    {
        wprintf(L"Type: %s\n", objectType->Name);
        wprintf(L"Reference count: %ld\n", ObjectHeader->RefCount);
        wprintf(L"Flags: %x\n", ObjectHeader->Flags);

        if (objectType == PhObjectTypeObject)
        {
            wprintf(L"Name: %s\n", ((PPH_OBJECT_TYPE)object)->Name);
            wprintf(L"Number of objects: %lu\n", ((PPH_OBJECT_TYPE)object)->NumberOfObjects);
            wprintf(L"Flags: %u\n", ((PPH_OBJECT_TYPE)object)->Flags);
            wprintf(L"Type index: %u\n", ((PPH_OBJECT_TYPE)object)->TypeIndex);
            wprintf(L"Free list count: %lu\n", ((PPH_OBJECT_TYPE)object)->FreeList.Count);
        }
        else if (objectType == PhStringType)
        {
            wprintf(L"%s\n", ((PPH_STRING)object)->Buffer);
        }
        else if (objectType == PhBytesType)
        {
            wprintf(L"%S\n", ((PPH_BYTES)object)->Buffer);
        }
        else if (objectType == PhHashtableType)
        {
            PhpPrintHashtableStatistics((PPH_HASHTABLE)object);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        wprintf(L"Error.\n");
    }
}

static VOID PhpPrintHashtableStatistics(
    _In_ PPH_HASHTABLE Hashtable
    )
{
    ULONG i;
    ULONG expectedLookupMisses = 0;

    wprintf(L"Count: %u\n", Hashtable->Count);
    wprintf(L"Allocated buckets: %u\n", Hashtable->AllocatedBuckets);
    wprintf(L"Allocated entries: %u\n", Hashtable->AllocatedEntries);
    wprintf(L"Next free entry: %d\n", Hashtable->FreeEntry);
    wprintf(L"Next usable entry: %d\n", Hashtable->NextEntry);

    wprintf(L"Equal function: %s\n", PhpGetSymbolForAddress(Hashtable->EqualFunction));
    wprintf(L"Hash function: %s\n", PhpGetSymbolForAddress(Hashtable->HashFunction));

    wprintf(L"\nBuckets:\n");

    for (i = 0; i < Hashtable->AllocatedBuckets; i++)
    {
        ULONG index;
        ULONG count = 0;

        // Count the number of entries in this bucket.

        index = Hashtable->Buckets[i];

        while (index != -1)
        {
            index = PH_HASHTABLE_GET_ENTRY(Hashtable, index)->Next;
            count++;
        }

        if (count != 0)
        {
            expectedLookupMisses += count - 1;
        }

        if (count != 0)
        {
            wprintf(L"%lu: ", i);

            // Print out the entry indicies.

            index = Hashtable->Buckets[i];

            while (index != -1)
            {
                wprintf(L"%lu", index);

                index = PH_HASHTABLE_GET_ENTRY(Hashtable, index)->Next;
                count--;

                if (count != 0)
                    wprintf(L", ");
            }

            wprintf(L"\n");
        }
        else
        {
            //wprintf(L"%u: (empty)\n");
        }
    }

    wprintf(L"\nExpected lookup misses: %lu\n", expectedLookupMisses);
}

#ifdef DEBUG
static VOID PhpDebugCreateObjectHook(
    _In_ PVOID Object,
    _In_ SIZE_T Size,
    _In_ ULONG Flags,
    _In_ PPH_OBJECT_TYPE ObjectType
    )
{
    PhAcquireQueuedLockExclusive(&NewObjectListLock);

    if (NewObjectList)
    {
        PhReferenceObject(Object);
        PhAddItemList(NewObjectList, Object);
    }

    PhReleaseQueuedLockExclusive(&NewObjectListLock);
}
#endif

#ifdef DEBUG
static VOID PhpDeleteNewObjectList(
    VOID
    )
{
    if (NewObjectList)
    {
        PhDereferenceObjects(NewObjectList->Items, NewObjectList->Count);
        PhDereferenceObject(NewObjectList);
        NewObjectList = NULL;
    }
}
#endif

static BOOLEAN PhpStringHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PSTRING_TABLE_ENTRY entry1 = Entry1;
    PSTRING_TABLE_ENTRY entry2 = Entry2;

    return PhEqualString(entry1->String, entry2->String, FALSE);
}

static ULONG PhpStringHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PSTRING_TABLE_ENTRY entry = Entry;

    return PhHashBytes((PUCHAR)entry->String->Buffer, entry->String->Length);
}

static int __cdecl PhpStringEntryCompareByCount(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PSTRING_TABLE_ENTRY entry1 = *(PSTRING_TABLE_ENTRY *)elem1;
    PSTRING_TABLE_ENTRY entry2 = *(PSTRING_TABLE_ENTRY *)elem2;

    return uintptrcmp(entry2->Count, entry1->Count);
}

static NTSTATUS PhpLeakEnumerationRoutine(
    _In_ LONG Reserved,
    _In_ PVOID HeapHandle,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T BlockSize,
    _In_ ULONG StackTraceDepth,
    _In_ PVOID *StackTrace
    )
{
    ULONG i;

    if (!InLeakDetection)
        return 0;

    if (!HeapHandle) // means no more entries
        return 0;

    if (ShowAllLeaks || HeapHandle == PhHeapHandle)
    {
        wprintf(L"Leak at 0x%Ix (%Iu bytes). Stack trace:\n", (ULONG_PTR)BaseAddress, BlockSize);

        for (i = 0; i < StackTraceDepth; i++)
        {
            PPH_STRING symbol;

            symbol = PhGetSymbolFromAddress(DebugConsoleSymbolProvider, (ULONG64)StackTrace[i], NULL, NULL, NULL, NULL);

            if (symbol)
            {
                wprintf(L"\t%s\n", symbol->Buffer);
                PhDereferenceObject(symbol);
            }
            else
            {
                wprintf(L"\t?\n");
            }
        }

        NumberOfLeaksShown++;
    }

    NumberOfLeaks++;

    return 0;
}

typedef struct _STOPWATCH
{
    LARGE_INTEGER StartCounter;
    LARGE_INTEGER EndCounter;
    LARGE_INTEGER Frequency;
} STOPWATCH, *PSTOPWATCH;

static VOID PhInitializeStopwatch(
    _Out_ PSTOPWATCH Stopwatch
    )
{
    Stopwatch->StartCounter.QuadPart = 0;
    Stopwatch->EndCounter.QuadPart = 0;
}

static VOID PhStartStopwatch(
    _Inout_ PSTOPWATCH Stopwatch
    )
{
    PhQueryPerformanceCounter(&Stopwatch->StartCounter);
    PhQueryPerformanceFrequency(&Stopwatch->Frequency);
}

static VOID PhStopStopwatch(
    _Inout_ PSTOPWATCH Stopwatch
    )
{
    NtQueryPerformanceCounter(&Stopwatch->EndCounter, NULL);
}

static ULONG PhGetMillisecondsStopwatch(
    _In_ PSTOPWATCH Stopwatch
    )
{
    LARGE_INTEGER countsPerMs;

    countsPerMs = Stopwatch->Frequency;
    countsPerMs.QuadPart /= 1000;

    return (ULONG)((Stopwatch->EndCounter.QuadPart - Stopwatch->StartCounter.QuadPart) /
        countsPerMs.QuadPart);
}

typedef VOID (FASTCALL *PPHF_RW_LOCK_FUNCTION)(
    _In_ PVOID Parameter
    );

typedef struct _RW_TEST_CONTEXT
{
    PWSTR Name;

    PPHF_RW_LOCK_FUNCTION AcquireExclusive;
    PPHF_RW_LOCK_FUNCTION AcquireShared;
    PPHF_RW_LOCK_FUNCTION ReleaseExclusive;
    PPHF_RW_LOCK_FUNCTION ReleaseShared;

    PVOID Parameter;
} RW_TEST_CONTEXT, *PRW_TEST_CONTEXT;

static PH_BARRIER RwStartBarrier;
static LONG RwReadersActive;
static LONG RwWritersActive;

static NTSTATUS PhpRwLockTestThreadStart(
    _In_ PVOID Parameter
    )
{
#define RW_ITERS 10000
#define RW_READ_ITERS 100
#define RW_WRITE_ITERS 10
#define RW_READ_SPIN_ITERS 60
#define RW_WRITE_SPIN_ITERS 200

    RW_TEST_CONTEXT context = *(PRW_TEST_CONTEXT)Parameter;
    ULONG i;
    ULONG j;
    ULONG k;
    ULONG m;

    PhWaitForBarrier(&RwStartBarrier, FALSE);

    for (i = 0; i < RW_ITERS; i++)
    {
        for (j = 0; j < RW_READ_ITERS; j++)
        {
            // Read zone

            context.AcquireShared(context.Parameter);
            _InterlockedIncrement(&RwReadersActive);

            for (m = 0; m < RW_READ_SPIN_ITERS; m++)
                YieldProcessor();

            if (RwWritersActive != 0)
            {
                wprintf(L"[fail]: writers active in read zone!\n");
                NtWaitForSingleObject(NtCurrentProcess(), FALSE, NULL);
            }

            _InterlockedDecrement(&RwReadersActive);
            context.ReleaseShared(context.Parameter);

            // Spin for a while

            for (m = 0; m < 10; m++)
                YieldProcessor();

            if (j == RW_READ_ITERS / 2)
            {
                // Write zone

                for (k = 0; k < RW_WRITE_ITERS; k++)
                {
                    context.AcquireExclusive(context.Parameter);
                    _InterlockedIncrement(&RwWritersActive);

                    for (m = 0; m < RW_WRITE_SPIN_ITERS; m++)
                        YieldProcessor();

                    if (RwReadersActive != 0)
                    {
                        wprintf(L"[fail]: readers active in write zone!\n");
                        NtWaitForSingleObject(NtCurrentProcess(), FALSE, NULL);
                    }

                    _InterlockedDecrement(&RwWritersActive);
                    context.ReleaseExclusive(context.Parameter);
                }
            }
        }
    }

    return STATUS_SUCCESS;
}

static VOID PhpTestRwLock(
    _In_ PRW_TEST_CONTEXT Context
    )
{
#define RW_PROCESSORS 4

    STOPWATCH stopwatch;
    ULONG i;
    HANDLE threadHandles[RW_PROCESSORS];

    // Dummy

    Context->AcquireExclusive(Context->Parameter);
    Context->ReleaseExclusive(Context->Parameter);
    Context->AcquireShared(Context->Parameter);
    Context->ReleaseShared(Context->Parameter);

    // Null test
    PhInitializeStopwatch(&stopwatch);
    PhStartStopwatch(&stopwatch);

    for (i = 0; i < 2000000; i++)
    {
        Context->AcquireExclusive(Context->Parameter);
        Context->ReleaseExclusive(Context->Parameter);
        Context->AcquireShared(Context->Parameter);
        Context->ReleaseShared(Context->Parameter);
    }

    PhStopStopwatch(&stopwatch);

    wprintf(L"[null] %s: %ums\n", Context->Name, PhGetMillisecondsStopwatch(&stopwatch));

    // Stress test

    PhInitializeBarrier(&RwStartBarrier, RW_PROCESSORS + 1);
    RwReadersActive = 0;
    RwWritersActive = 0;

    for (i = 0; i < RW_PROCESSORS; i++)
    {
        PhCreateThreadEx(&threadHandles[i], PhpRwLockTestThreadStart, Context);
    }

    PhWaitForBarrier(&RwStartBarrier, FALSE);
    PhInitializeStopwatch(&stopwatch);
    PhStartStopwatch(&stopwatch);
    NtWaitForMultipleObjects(RW_PROCESSORS, threadHandles, WaitAll, FALSE, NULL);
    PhStopStopwatch(&stopwatch);

    for (i = 0; i < RW_PROCESSORS; i++)
        NtClose(threadHandles[i]);

    wprintf(L"[strs] %s: %ums\n", Context->Name, PhGetMillisecondsStopwatch(&stopwatch));
}

VOID FASTCALL PhfAcquireCriticalSection(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
    )
{
    RtlEnterCriticalSection(CriticalSection);
}

VOID FASTCALL PhfReleaseCriticalSection(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
    )
{
    RtlLeaveCriticalSection(CriticalSection);
}

NTSTATUS PhpDebugConsoleThreadStart(
    _In_ PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    BOOLEAN exit = FALSE;

    PhInitializeAutoPool(&autoPool);

    DebugConsoleSymbolProvider = PhCreateSymbolProvider(NtCurrentProcessId());
    PhLoadSymbolProviderOptions(DebugConsoleSymbolProvider);

    {
        static PH_STRINGREF variableNameSr = PH_STRINGREF_INIT(L"_NT_SYMBOL_PATH");
        PPH_STRING variableValue;
        PPH_STRING newSearchPath;

        if (NT_SUCCESS(PhQueryEnvironmentVariable(NULL, &variableNameSr, &variableValue)))
        {
            PPH_STRING currentDirectory = PhGetApplicationDirectoryWin32();
            PPH_STRING currentSearchPath = PhGetStringSetting(L"DbgHelpSearchPath");

            if (currentSearchPath->Length != 0)
            {
                newSearchPath = PhFormatString(
                    L"%s;%s;%s",
                    variableValue->Buffer,
                    PhGetStringOrEmpty(currentSearchPath),
                    PhGetStringOrEmpty(currentDirectory)
                    );
            }
            else
            {
                newSearchPath = PhFormatString(
                    L"%s;%s",
                    variableValue->Buffer,
                    PhGetStringOrEmpty(currentDirectory)
                    );
            }

            PhSetSearchPathSymbolProvider(DebugConsoleSymbolProvider, PhGetString(newSearchPath));

            PhDereferenceObject(variableValue);
            PhDereferenceObject(newSearchPath);
            PhDereferenceObject(currentDirectory);
        }
    }

    PhEnumGenericModules(
        NtCurrentProcessId(),
        NtCurrentProcess(),
        0,
        PhpLoadCurrentProcessSymbolsCallback,
        DebugConsoleSymbolProvider
        );

#ifdef DEBUG
    PhInitializeQueuedLock(&NewObjectListLock);
    PhDbgCreateObjectHook = PhpDebugCreateObjectHook;
#endif

    wprintf(L"Press Ctrl+C or type \"exit\" to close the debug console. Type \"help\" for a list of commands.\n");

    while (!exit)
    {
        static PWSTR delims = L" \t";
        static PWSTR commandDebugOnly = L"This command is not available on non-debug builds.\n";

        WCHAR line[201];
        ULONG inputLength;
        PWSTR context;
        PWSTR command;

        wprintf(L"dbg>");

        if (!fgetws(line, sizeof(line) / sizeof(WCHAR) - 1, stdin))
            break;

        // Remove the terminating new line character.

        inputLength = (ULONG)PhCountStringZ(line);

        if (inputLength != 0)
            line[inputLength - 1] = UNICODE_NULL;

        context = NULL;
        command = wcstok_s(line, delims, &context);

        if (!command)
        {
            continue;
        }
        else if (PhEqualStringZ(command, L"help", TRUE))
        {
            wprintf(
                L"Commands:\n"
                L"exit\n"
                L"testperf\n"
                L"testlocks\n"
                L"stats\n"
                L"objects [type-name-filter]\n"
                L"objtrace object-address\n"
                L"objmksnap\n"
                L"objcmpsnap\n"
                L"objmknew\n"
                L"objdelnew\n"
                L"objviewnew\n"
                L"dumpobj\n"
                L"dumpautopool\n"
                L"threads\n"
                L"provthreads\n"
                L"workqueues\n"
                L"procrecords\n"
                L"procitem\n"
                L"uniquestr\n"
                L"enableleakdetect\n"
                L"leakdetect\n"
                L"mem\n"
                );
        }
        else if (PhEqualStringZ(command, L"exit", TRUE))
        {
            PhCloseDebugConsole();
            exit = TRUE;
        }
        else if (PhEqualStringZ(command, L"testperf", TRUE))
        {
            STOPWATCH stopwatch;
            ULONG i;
            PPH_STRING testString;
            RTL_CRITICAL_SECTION testCriticalSection;
            PH_FAST_LOCK testFastLock;
            PH_QUEUED_LOCK testQueuedLock;

            // Control (string reference counting)

            testString = PhCreateString(L"");
            PhReferenceObject(testString);
            PhDereferenceObject(testString);
            PhStartStopwatch(&stopwatch);

            for (i = 0; i < 10000000; i++)
            {
                PhReferenceObject(testString);
                PhDereferenceObject(testString);
            }

            PhStopStopwatch(&stopwatch);
            PhDereferenceObject(testString);

            wprintf(L"Referencing: %ums\n", PhGetMillisecondsStopwatch(&stopwatch));

            // Critical section

            RtlInitializeCriticalSection(&testCriticalSection);
            RtlEnterCriticalSection(&testCriticalSection);
            RtlLeaveCriticalSection(&testCriticalSection);
            PhStartStopwatch(&stopwatch);

            for (i = 0; i < 10000000; i++)
            {
                RtlEnterCriticalSection(&testCriticalSection);
                RtlLeaveCriticalSection(&testCriticalSection);
            }

            PhStopStopwatch(&stopwatch);
            RtlDeleteCriticalSection(&testCriticalSection);

            wprintf(L"Critical section: %ums\n", PhGetMillisecondsStopwatch(&stopwatch));

            // Fast lock

            PhInitializeFastLock(&testFastLock);
            PhAcquireFastLockExclusive(&testFastLock);
            PhReleaseFastLockExclusive(&testFastLock);
            PhStartStopwatch(&stopwatch);

            for (i = 0; i < 10000000; i++)
            {
                PhAcquireFastLockExclusive(&testFastLock);
                PhReleaseFastLockExclusive(&testFastLock);
            }

            PhStopStopwatch(&stopwatch);
            PhDeleteFastLock(&testFastLock);

            wprintf(L"Fast lock: %ums\n", PhGetMillisecondsStopwatch(&stopwatch));

            // Queued lock

            PhInitializeQueuedLock(&testQueuedLock);
            PhAcquireQueuedLockExclusive(&testQueuedLock);
            PhReleaseQueuedLockExclusive(&testQueuedLock);
            PhStartStopwatch(&stopwatch);

            for (i = 0; i < 10000000; i++)
            {
                PhAcquireQueuedLockExclusive(&testQueuedLock);
                PhReleaseQueuedLockExclusive(&testQueuedLock);
            }

            PhStopStopwatch(&stopwatch);

            wprintf(L"Queued lock: %ums\n", PhGetMillisecondsStopwatch(&stopwatch));
        }
        else if (PhEqualStringZ(command, L"testlocks", TRUE))
        {
            RW_TEST_CONTEXT testContext;
            PH_FAST_LOCK fastLock;
            PH_QUEUED_LOCK queuedLock;
            RTL_CRITICAL_SECTION criticalSection;

            testContext.Name = L"FastLock";
            testContext.AcquireExclusive = PhfAcquireFastLockExclusive;
            testContext.AcquireShared = PhfAcquireFastLockShared;
            testContext.ReleaseExclusive = PhfReleaseFastLockExclusive;
            testContext.ReleaseShared = PhfReleaseFastLockShared;
            testContext.Parameter = &fastLock;
            PhInitializeFastLock(&fastLock);
            PhpTestRwLock(&testContext);
            PhDeleteFastLock(&fastLock);

            testContext.Name = L"QueuedLock";
            testContext.AcquireExclusive = PhfAcquireQueuedLockExclusive;
            testContext.AcquireShared = PhfAcquireQueuedLockShared;
            testContext.ReleaseExclusive = PhfReleaseQueuedLockExclusive;
            testContext.ReleaseShared = PhfReleaseQueuedLockShared;
            testContext.Parameter = &queuedLock;
            PhInitializeQueuedLock(&queuedLock);
            PhpTestRwLock(&testContext);

            testContext.Name = L"CriticalSection";
            testContext.AcquireExclusive = PhfAcquireCriticalSection;
            testContext.AcquireShared = PhfAcquireCriticalSection;
            testContext.ReleaseExclusive = PhfReleaseCriticalSection;
            testContext.ReleaseShared = PhfReleaseCriticalSection;
            testContext.Parameter = &criticalSection;
            RtlInitializeCriticalSection(&criticalSection);
            PhpTestRwLock(&testContext);
            RtlDeleteCriticalSection(&criticalSection);

            testContext.Name = L"QueuedLockMutex";
            testContext.AcquireExclusive = PhfAcquireQueuedLockExclusive;
            testContext.AcquireShared = PhfAcquireQueuedLockExclusive;
            testContext.ReleaseExclusive = PhfReleaseQueuedLockExclusive;
            testContext.ReleaseShared = PhfReleaseQueuedLockExclusive;
            testContext.Parameter = &queuedLock;
            PhInitializeQueuedLock(&queuedLock);
            PhpTestRwLock(&testContext);
        }
        else if (PhEqualStringZ(command, L"stats", TRUE))
        {
#ifdef DEBUG
            wprintf(L"Object small free list count: %u\n", PhObjectSmallFreeList.Count);
            wprintf(L"Statistics:\n");
#define PRINT_STATISTIC(Name) wprintf(TEXT(#Name) L": %u\n", PhLibStatisticsBlock.Name);

            PRINT_STATISTIC(BaseThreadsCreated);
            PRINT_STATISTIC(BaseThreadsCreateFailed);
            PRINT_STATISTIC(BaseStringBuildersCreated);
            PRINT_STATISTIC(BaseStringBuildersResized);
            PRINT_STATISTIC(RefObjectsCreated);
            PRINT_STATISTIC(RefObjectsDestroyed);
            PRINT_STATISTIC(RefObjectsAllocated);
            PRINT_STATISTIC(RefObjectsFreed);
            PRINT_STATISTIC(RefObjectsAllocatedFromSmallFreeList);
            PRINT_STATISTIC(RefObjectsFreedToSmallFreeList);
            PRINT_STATISTIC(RefObjectsAllocatedFromTypeFreeList);
            PRINT_STATISTIC(RefObjectsFreedToTypeFreeList);
            PRINT_STATISTIC(RefObjectsDeleteDeferred);
            PRINT_STATISTIC(RefAutoPoolsCreated);
            PRINT_STATISTIC(RefAutoPoolsDestroyed);
            PRINT_STATISTIC(RefAutoPoolsDynamicAllocated);
            PRINT_STATISTIC(RefAutoPoolsDynamicResized);
            PRINT_STATISTIC(QlBlockSpins);
            PRINT_STATISTIC(QlBlockWaits);
            PRINT_STATISTIC(QlAcquireExclusiveBlocks);
            PRINT_STATISTIC(QlAcquireSharedBlocks);
            PRINT_STATISTIC(WqWorkQueueThreadsCreated);
            PRINT_STATISTIC(WqWorkQueueThreadsCreateFailed);
            PRINT_STATISTIC(WqWorkItemsQueued);

#else
            wprintf(commandDebugOnly);
#endif
        }
        else if (PhEqualStringZ(command, L"objects", TRUE))
        {
#ifdef DEBUG
            PWSTR typeFilter = wcstok_s(NULL, delims, &context);
            PLIST_ENTRY currentEntry;
            ULONG totalNumberOfObjects = 0;
            //SIZE_T totalNumberOfBytes = 0;

            if (typeFilter)
                _wcslwr(typeFilter);

            PhAcquireQueuedLockShared(&PhDbgObjectListLock);

            currentEntry = PhDbgObjectListHead.Flink;

            while (currentEntry != &PhDbgObjectListHead)
            {
                PPH_OBJECT_HEADER objectHeader;
                WCHAR typeName[32];

                objectHeader = CONTAINING_RECORD(currentEntry, PH_OBJECT_HEADER, ObjectListEntry);

                // Make sure the object isn't being destroyed.
                if (!PhReferenceObjectSafe(PhObjectHeaderToObject(objectHeader)))
                {
                    currentEntry = currentEntry->Flink;
                    continue;
                }

                totalNumberOfObjects++;
                //totalNumberOfBytes += objectHeader->Size;

                if (typeFilter)
                {
                    wcscpy_s(typeName, sizeof(typeName) / sizeof(WCHAR), PhGetObjectType(PhObjectHeaderToObject(objectHeader))->Name);
                    _wcslwr(typeName);
                }

                if (
                    !typeFilter ||
                    (typeFilter && wcsstr(typeName, typeFilter))
                    )
                {
                    PhpPrintObjectInfo(objectHeader, 1);
                }

                currentEntry = currentEntry->Flink;
                PhDereferenceObjectDeferDelete(PhObjectHeaderToObject(objectHeader));
            }

            PhReleaseQueuedLockShared(&PhDbgObjectListLock);

            wprintf(L"\n");
            wprintf(L"Total number: %lu\n", totalNumberOfObjects);
            /*wprintf(L"Total size (excl. header): %s\n",
                ((PPH_STRING)PH_AUTO(PhFormatSize(totalNumberOfBytes, 1)))->Buffer);*/
            wprintf(L"Total overhead (header): %s\n",
                ((PPH_STRING)PH_AUTO(
                PhFormatSize(PhAddObjectHeaderSize(0) * totalNumberOfObjects, 1)
                ))->Buffer);
#else
            wprintf(commandDebugOnly);
#endif
        }
        else if (PhEqualStringZ(command, L"objtrace", TRUE))
        {
#ifdef DEBUG
            PWSTR objectAddress = wcstok_s(NULL, delims, &context);
            PH_STRINGREF objectAddressString;
            ULONG64 address;

            if (!objectAddress)
            {
                wprintf(L"Missing object address.\n");
                goto EndCommand;
            }

            PhInitializeStringRef(&objectAddressString, objectAddress);

            if (PhStringToInteger64(&objectAddressString, 16, &address))
            {
                PPH_OBJECT_HEADER objectHeader = (PPH_OBJECT_HEADER)PhObjectToObjectHeader((PVOID)address);
                PVOID stackBackTrace[16];
                ULONG i;

                // The address may not be valid.
                __try
                {
                    memcpy(stackBackTrace, objectHeader->StackBackTrace, 16 * sizeof(PVOID));
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    PPH_STRING message;

                    message = PH_AUTO(PhGetNtMessage(GetExceptionCode()));
                    wprintf(L"Error: %s\n", PhGetString(message));

                    goto EndCommand;
                }

                for (i = 0; i < 16; i++)
                {
                    if (!stackBackTrace[i])
                        break;

                    wprintf(L"%s\n", PhpGetSymbolForAddress(stackBackTrace[i]));
                }
            }
            else
            {
                wprintf(L"Invalid object address.\n");
            }
#else
            wprintf(commandDebugOnly);
#endif
        }
        else if (PhEqualStringZ(command, L"objmksnap", TRUE))
        {
#ifdef DEBUG
            PLIST_ENTRY currentEntry;

            if (ObjectListSnapshot)
            {
                PhDereferenceObject(ObjectListSnapshot);
                ObjectListSnapshot = NULL;
            }

            ObjectListSnapshot = PhCreateSimpleHashtable(100);

            PhAcquireQueuedLockShared(&PhDbgObjectListLock);

            currentEntry = PhDbgObjectListHead.Flink;

            while (currentEntry != &PhDbgObjectListHead)
            {
                PPH_OBJECT_HEADER objectHeader;

                objectHeader = CONTAINING_RECORD(currentEntry, PH_OBJECT_HEADER, ObjectListEntry);
                currentEntry = currentEntry->Flink;

                if (PhObjectHeaderToObject(objectHeader) != ObjectListSnapshot)
                    PhAddItemSimpleHashtable(ObjectListSnapshot, objectHeader, NULL);
            }

            PhReleaseQueuedLockShared(&PhDbgObjectListLock);
#else
            wprintf(commandDebugOnly);
#endif
        }
        else if (PhEqualStringZ(command, L"objcmpsnap", TRUE))
        {
#ifdef DEBUG
            PLIST_ENTRY currentEntry;
            PPH_LIST newObjects;
            ULONG i;

            if (!ObjectListSnapshot)
            {
                wprintf(L"No snapshot.\n");
                goto EndCommand;
            }

            newObjects = PhCreateList(10);

            PhAcquireQueuedLockShared(&PhDbgObjectListLock);

            currentEntry = PhDbgObjectListHead.Flink;

            while (currentEntry != &PhDbgObjectListHead)
            {
                PPH_OBJECT_HEADER objectHeader;

                objectHeader = CONTAINING_RECORD(currentEntry, PH_OBJECT_HEADER, ObjectListEntry);
                currentEntry = currentEntry->Flink;

                if (
                    PhObjectHeaderToObject(objectHeader) != ObjectListSnapshot &&
                    PhObjectHeaderToObject(objectHeader) != newObjects
                    )
                {
                    if (!PhFindItemSimpleHashtable(ObjectListSnapshot, objectHeader))
                    {
                        if (PhReferenceObjectSafe(PhObjectHeaderToObject(objectHeader)))
                            PhAddItemList(newObjects, objectHeader);
                    }
                }
            }

            PhReleaseQueuedLockShared(&PhDbgObjectListLock);

            for (i = 0; i < newObjects->Count; i++)
            {
                PPH_OBJECT_HEADER objectHeader = newObjects->Items[i];

                PhpPrintObjectInfo(objectHeader, 1);

                PhDereferenceObject(PhObjectHeaderToObject(objectHeader));
            }

            PhDereferenceObject(newObjects);
#else
            wprintf(commandDebugOnly);
#endif
        }
        else if (PhEqualStringZ(command, L"objmknew", TRUE))
        {
#ifdef DEBUG
            PhAcquireQueuedLockExclusive(&NewObjectListLock);
            PhpDeleteNewObjectList();
            PhReleaseQueuedLockExclusive(&NewObjectListLock);

            // Creation needs to be done outside of the lock,
            // otherwise a deadlock will occur.
            NewObjectList = PhCreateList(100);
#else
            wprintf(commandDebugOnly);
#endif
        }
        else if (PhEqualStringZ(command, L"objdelnew", TRUE))
        {
#ifdef DEBUG
            PhAcquireQueuedLockExclusive(&NewObjectListLock);
            PhpDeleteNewObjectList();
            PhReleaseQueuedLockExclusive(&NewObjectListLock);
#else
            wprintf(commandDebugOnly);
#endif
        }
        else if (PhEqualStringZ(command, L"objviewnew", TRUE))
        {
#ifdef DEBUG
            ULONG i;

            PhAcquireQueuedLockExclusive(&NewObjectListLock);

            if (!NewObjectList)
            {
                wprintf(L"Object creation hooking not active.\n");
                PhReleaseQueuedLockExclusive(&NewObjectListLock);
                goto EndCommand;
            }

            for (i = 0; i < NewObjectList->Count; i++)
            {
                PhpPrintObjectInfo(PhObjectToObjectHeader(NewObjectList->Items[i]), 1);
            }

            PhReleaseQueuedLockExclusive(&NewObjectListLock);
#else
            wprintf(commandDebugOnly);
#endif
        }
        else if (PhEqualStringZ(command, L"dumpobj", TRUE))
        {
            PWSTR addressString = wcstok_s(NULL, delims, &context);
            PH_STRINGREF addressStringRef;
            ULONG64 address;

            if (!addressString)
                goto EndCommand;

            PhInitializeStringRef(&addressStringRef, addressString);

            if (PhStringToInteger64(&addressStringRef, 16, &address))
            {
                PhpDumpObjectInfo((PPH_OBJECT_HEADER)PhObjectToObjectHeader((PVOID)address));
            }
        }
        else if (PhEqualStringZ(command, L"dumpautopool", TRUE))
        {
            PWSTR addressString = wcstok_s(NULL, delims, &context);
            PH_STRINGREF addressStringRef;
            ULONG64 address;

            if (!addressString)
                goto EndCommand;

            PhInitializeStringRef(&addressStringRef, addressString);

            if (PhStringToInteger64(&addressStringRef, 16, &address))
            {
                PPH_AUTO_POOL userAutoPool = (PPH_AUTO_POOL)address;
                ULONG i;

                __try
                {
                    wprintf(L"Static count: %u\n", userAutoPool->StaticCount);
                    wprintf(L"Dynamic count: %u\n", userAutoPool->DynamicCount);
                    wprintf(L"Dynamic allocated: %u\n", userAutoPool->DynamicAllocated);

                    wprintf(L"Static objects:\n");

                    for (i = 0; i < userAutoPool->StaticCount; i++)
                        PhpPrintObjectInfo(PhObjectToObjectHeader(userAutoPool->StaticObjects[i]), 0);

                    wprintf(L"Dynamic objects:\n");

                    for (i = 0; i < userAutoPool->DynamicCount; i++)
                        PhpPrintObjectInfo(PhObjectToObjectHeader(userAutoPool->DynamicObjects[i]), 0);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    goto EndCommand;
                }
            }
        }
        else if (PhEqualStringZ(command, L"threads", TRUE))
        {
#ifdef DEBUG
            PLIST_ENTRY currentEntry;

            PhAcquireQueuedLockShared(&PhDbgThreadListLock);

            currentEntry = PhDbgThreadListHead.Flink;

            while (currentEntry != &PhDbgThreadListHead)
            {
                PPHP_BASE_THREAD_DBG dbg;

                dbg = CONTAINING_RECORD(currentEntry, PHP_BASE_THREAD_DBG, ListEntry);

                wprintf(L"Thread %u\n", HandleToUlong(dbg->ClientId.UniqueThread));
                wprintf(L"\tStart Address: %s\n", PhpGetSymbolForAddress(dbg->StartAddress));
                wprintf(L"\tParameter: %Ix\n", (ULONG_PTR)dbg->Parameter);
                wprintf(L"\tCurrent auto-pool: %Ix\n", (ULONG_PTR)dbg->CurrentAutoPool);

                currentEntry = currentEntry->Flink;
            }

            PhReleaseQueuedLockShared(&PhDbgThreadListLock);
#else
            wprintf(commandDebugOnly);
#endif
        }
        else if (PhEqualStringZ(command, L"provthreads", TRUE))
        {
#ifdef DEBUG
            ULONG i;

            if (PhDbgProviderList)
            {
                PhAcquireQueuedLockShared(&PhDbgProviderListLock);

                for (i = 0; i < PhDbgProviderList->Count; i++)
                {
                    PPH_PROVIDER_THREAD providerThread = PhDbgProviderList->Items[i];
                    THREAD_BASIC_INFORMATION basicInfo;
                    PLIST_ENTRY providerEntry;

                    if (providerThread->ThreadHandle)
                    {
                        PhGetThreadBasicInformation(providerThread->ThreadHandle, &basicInfo);
                        wprintf(L"Thread %u\n", HandleToUlong(basicInfo.ClientId.UniqueThread));
                    }
                    else
                    {
                        wprintf(L"Thread not running\n");
                    }

                    PhAcquireQueuedLockExclusive(&providerThread->Lock);

                    providerEntry = providerThread->ListHead.Flink;

                    while (providerEntry != &providerThread->ListHead)
                    {
                        PPH_PROVIDER_REGISTRATION registration;

                        registration = CONTAINING_RECORD(providerEntry, PH_PROVIDER_REGISTRATION, ListEntry);

                        wprintf(L"\tProvider registration at %Ix\n", (ULONG_PTR)registration);
                        wprintf(L"\t\tEnabled: %s\n", registration->Enabled ? L"Yes" : L"No");
                        wprintf(L"\t\tFunction: %s\n", PhpGetSymbolForAddress(registration->Function));

                        if (registration->Object)
                        {
                            wprintf(L"\t\tObject:\n");
                            PhpPrintObjectInfo(PhObjectToObjectHeader(registration->Object), 0);
                        }

                        providerEntry = providerEntry->Flink;
                    }

                    PhReleaseQueuedLockExclusive(&providerThread->Lock);

                    wprintf(L"\n");
                }

                PhReleaseQueuedLockShared(&PhDbgProviderListLock);
            }
#else
            wprintf(commandDebugOnly);
#endif
        }
        else if (PhEqualStringZ(command, L"workqueues", TRUE))
        {
#ifdef DEBUG
            ULONG i;

            if (PhDbgWorkQueueList)
            {
                PhAcquireQueuedLockShared(&PhDbgWorkQueueListLock);

                for (i = 0; i < PhDbgWorkQueueList->Count; i++)
                {
                    PPH_WORK_QUEUE workQueue = PhDbgWorkQueueList->Items[i];
                    PLIST_ENTRY workQueueItemEntry;

                    wprintf(L"Work queue at %s\n", PhpGetSymbolForAddress(workQueue));
                    wprintf(L"Maximum threads: %lu\n", workQueue->MaximumThreads);
                    wprintf(L"Minimum threads: %lu\n", workQueue->MinimumThreads);
                    wprintf(L"No work timeout: %lu\n", workQueue->NoWorkTimeout);

                    wprintf(L"Current threads: %lu\n", workQueue->CurrentThreads);
                    wprintf(L"Busy count: %lu\n", workQueue->BusyCount);

                    PhAcquireQueuedLockExclusive(&workQueue->QueueLock);

                    // List the items backwards.
                    workQueueItemEntry = workQueue->QueueListHead.Blink;

                    while (workQueueItemEntry != &workQueue->QueueListHead)
                    {
                        PPH_WORK_QUEUE_ITEM workQueueItem;

                        workQueueItem = CONTAINING_RECORD(workQueueItemEntry, PH_WORK_QUEUE_ITEM, ListEntry);

                        wprintf(L"\tWork queue item at %Ix\n", (ULONG_PTR)workQueueItem);
                        wprintf(L"\t\tFunction: %s\n", PhpGetSymbolForAddress(workQueueItem->Function));
                        wprintf(L"\t\tContext: %Ix\n", (ULONG_PTR)workQueueItem->Context);

                        workQueueItemEntry = workQueueItemEntry->Blink;
                    }

                    PhReleaseQueuedLockExclusive(&workQueue->QueueLock);

                    wprintf(L"\n");
                }

                PhReleaseQueuedLockShared(&PhDbgWorkQueueListLock);
            }
#else
            wprintf(commandDebugOnly);
#endif
        }
        else if (PhEqualStringZ(command, L"procrecords", TRUE))
        {
            PPH_PROCESS_RECORD record;
            ULONG i;
            SYSTEMTIME systemTime;
            PPH_PROCESS_RECORD startRecord;

            PhAcquireQueuedLockShared(&PhProcessRecordListLock);

            for (i = 0; i < PhProcessRecordList->Count; i++)
            {
                record = (PPH_PROCESS_RECORD)PhProcessRecordList->Items[i];

                PhLargeIntegerToLocalSystemTime(&systemTime, &record->CreateTime);
                wprintf(L"Records for %s %s:\n",
                    ((PPH_STRING)PH_AUTO(PhFormatDate(&systemTime, NULL)))->Buffer,
                    ((PPH_STRING)PH_AUTO(PhFormatTime(&systemTime, NULL)))->Buffer
                    );

                startRecord = record;

                do
                {
                    wprintf(L"\tRecord at %Ix: %s (%lu) (refs: %ld)\n", (ULONG_PTR)record, record->ProcessName->Buffer, HandleToUlong(record->ProcessId), record->RefCount);

                    if (record->FileName)
                        wprintf(L"\t\t%s\n", record->FileName->Buffer);

                    record = CONTAINING_RECORD(record->ListEntry.Flink, PH_PROCESS_RECORD, ListEntry);
                } while (record != startRecord);
            }

            PhReleaseQueuedLockShared(&PhProcessRecordListLock);
        }
        else if (PhEqualStringZ(command, L"procitem", TRUE))
        {
            PWSTR filterString;
            PH_STRINGREF filterRef;
            ULONG64 filter64;
            LONG_PTR processIdFilter = -9; // can't use -2, -1 or 0 because they're all used for process IDs
            ULONG_PTR processAddressFilter = 0;
            PWSTR imageNameFilter = NULL;
            BOOLEAN showAll = FALSE;
            PPH_PROCESS_ITEM *processes;
            ULONG numberOfProcesses;
            ULONG i;

            filterString = wcstok_s(NULL, delims, &context);

            if (filterString)
            {
                PhInitializeStringRef(&filterRef, filterString);

                if (PhStringToInteger64(&filterRef, 10, &filter64))
                    processIdFilter = (LONG_PTR)filter64;
                if (PhStringToInteger64(&filterRef, 16, &filter64))
                    processAddressFilter = (ULONG_PTR)filter64;

                imageNameFilter = filterString;
            }
            else
            {
                showAll = TRUE;
            }

            PhEnumProcessItems(&processes, &numberOfProcesses);

            for (i = 0; i < numberOfProcesses; i++)
            {
                PPH_PROCESS_ITEM process = processes[i];

                if (
                    showAll ||
                    (processIdFilter != -9 && (LONG_PTR)process->ProcessId == processIdFilter) ||
                    (processAddressFilter != 0 && (ULONG_PTR)process == processAddressFilter) ||
                    (imageNameFilter && PhMatchWildcards(imageNameFilter, process->ProcessName->Buffer, TRUE))
                    )
                {
                    wprintf(L"Process item at %Ix: %s (%u)\n", (ULONG_PTR)process, process->ProcessName->Buffer, HandleToUlong(process->ProcessId));
                    wprintf(L"\tRecord at %Ix\n", (ULONG_PTR)process->Record);
                    wprintf(L"\tQuery handle %Ix\n", (ULONG_PTR)process->QueryHandle);
                    wprintf(L"\tFile name at %Ix: %s\n", (ULONG_PTR)process->FileNameWin32, PhGetStringOrDefault(process->FileNameWin32, L"(null)"));
                    wprintf(L"\tCommand line at %Ix: %s\n", (ULONG_PTR)process->CommandLine, PhGetStringOrDefault(process->CommandLine, L"(null)"));
                    wprintf(L"\tFlags: %u\n", process->Flags);
                    wprintf(L"\n");
                }
            }

            PhDereferenceObjects(processes, numberOfProcesses);
        }
        else if (PhEqualStringZ(command, L"uniquestr", TRUE))
        {
#ifdef DEBUG
            PLIST_ENTRY currentEntry;
            PPH_HASHTABLE hashtable;
            PPH_LIST list;
            PSTRING_TABLE_ENTRY stringEntry;
            ULONG enumerationKey;
            ULONG i;

            hashtable = PhCreateHashtable(
                sizeof(STRING_TABLE_ENTRY),
                PhpStringHashtableEqualFunction,
                PhpStringHashtableHashFunction,
                1024
                );

            PhAcquireQueuedLockShared(&PhDbgObjectListLock);

            currentEntry = PhDbgObjectListHead.Flink;

            while (currentEntry != &PhDbgObjectListHead)
            {
                PPH_OBJECT_HEADER objectHeader;
                PPH_STRING string;
                STRING_TABLE_ENTRY localStringEntry;
                BOOLEAN added;

                objectHeader = CONTAINING_RECORD(currentEntry, PH_OBJECT_HEADER, ObjectListEntry);
                currentEntry = currentEntry->Flink;
                string = PhObjectHeaderToObject(objectHeader);

                // Make sure this is a string.
                if (PhGetObjectType(string) != PhStringType)
                    continue;

                // Make sure the object isn't being destroyed.
                if (!PhReferenceObjectSafe(string))
                    continue;

                localStringEntry.String = string;
                stringEntry = PhAddEntryHashtableEx(hashtable, &localStringEntry, &added);

                if (added)
                {
                    stringEntry->Count = 1;
                    PhReferenceObject(string);
                }
                else
                {
                    stringEntry->Count++;
                }

                PhDereferenceObjectDeferDelete(string);
            }

            PhReleaseQueuedLockShared(&PhDbgObjectListLock);

            // Sort the string entries by count.

            list = PhCreateList(hashtable->Count);

            enumerationKey = 0;

            while (PhEnumHashtable(hashtable, &stringEntry, &enumerationKey))
            {
                PhAddItemList(list, stringEntry);
            }

            qsort(list->Items, list->Count, sizeof(PSTRING_TABLE_ENTRY), PhpStringEntryCompareByCount);

            // Display...

            for (i = 0; i < 40 && i < list->Count; i++)
            {
                stringEntry = list->Items[i];
                wprintf(L"%Iu\t%.64s\n", stringEntry->Count, stringEntry->String->Buffer);
            }

            wprintf(L"\nTotal unique strings: %u\n", list->Count);

            // Cleanup

            for (i = 0; i < list->Count; i++)
            {
                stringEntry = list->Items[i];
                PhDereferenceObject(stringEntry->String);
            }

            PhDereferenceObject(list);
            PhDereferenceObject(hashtable);
#else
            wprintf(commandDebugOnly);
#endif
        }
        else if (PhEqualStringZ(command, L"enableleakdetect", TRUE))
        {
            HEAP_DEBUGGING_INFORMATION debuggingInfo;

            memset(&debuggingInfo, 0, sizeof(HEAP_DEBUGGING_INFORMATION));
            debuggingInfo.StackTraceDepth = 32;
            debuggingInfo.HeapLeakEnumerationRoutine = PhpLeakEnumerationRoutine;

            if (!NT_SUCCESS(RtlSetHeapInformation(NULL, HeapSetDebuggingInformation, &debuggingInfo, sizeof(HEAP_DEBUGGING_INFORMATION))))
            {
                wprintf(L"Unable to initialize heap debugging. Make sure that you are using Windows 7 or above.");
            }
        }
        else if (PhEqualStringZ(command, L"leakdetect", TRUE))
        {
            VOID (NTAPI *rtlDetectHeapLeaks)(VOID);
            PWSTR options = wcstok_s(NULL, delims, &context);

            rtlDetectHeapLeaks = PhGetDllProcedureAddress(L"ntdll.dll", "RtlDetectHeapLeaks", 0);

            if (!(NtCurrentPeb()->NtGlobalFlag & FLG_USER_STACK_TRACE_DB))
            {
                wprintf(L"Warning: user-mode stack trace database is not enabled. Stack traces will not be displayed.\n");
            }

            ShowAllLeaks = FALSE;

            if (options)
            {
                if (PhEqualStringZ(options, L"all", TRUE))
                    ShowAllLeaks = TRUE;
            }

            if (rtlDetectHeapLeaks)
            {
                InLeakDetection = TRUE;
                NumberOfLeaks = 0;
                NumberOfLeaksShown = 0;
                rtlDetectHeapLeaks();
                InLeakDetection = FALSE;

                wprintf(L"\nNumber of leaks: %lu (%lu displayed)\n", NumberOfLeaks, NumberOfLeaksShown);
            }
        }
        else if (PhEqualStringZ(command, L"mem", TRUE))
        {
            PWSTR addressString;
            PWSTR bytesString;
            PH_STRINGREF addressStringRef;
            PH_STRINGREF bytesStringRef;
            ULONG64 address64;
            ULONG64 numberOfBytes64;
            PUCHAR address;
            ULONG numberOfBytes;
            ULONG blockSize;
            UCHAR buffer[16];
            ULONG i;

            addressString = wcstok_s(NULL, delims, &context);

            if (!addressString)
                goto PrintMemUsage;

            bytesString = wcstok_s(NULL, delims, &context);

            if (!bytesString)
            {
                bytesString = L"16";
            }

            PhInitializeStringRef(&addressStringRef, addressString);
            PhInitializeStringRef(&bytesStringRef, bytesString);

            if (PhStringToInteger64(&addressStringRef, 16, &address64) && PhStringToInteger64(&bytesStringRef, 10, &numberOfBytes64))
            {
                address = (PUCHAR)address64;
                numberOfBytes = (ULONG)numberOfBytes64;

                if (numberOfBytes > 256)
                {
                    wprintf(L"Number of bytes must be 256 or smaller.\n");
                    goto EndCommand;
                }

                blockSize = sizeof(buffer);

                while (numberOfBytes != 0)
                {
                    if (blockSize > numberOfBytes)
                        blockSize = numberOfBytes;

                    __try
                    {
                        memcpy(buffer, address, blockSize);
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        wprintf(L"Error reading address near %Ix.\n", (ULONG_PTR)address);
                        goto EndCommand;
                    }

                    // Print hex dump
                    for (i = 0; i < blockSize; i++)
                        wprintf(L"%02x ", buffer[i]);

                    // Fill remaining space (for last, possibly partial block)
                    for (; i < sizeof(buffer); i++)
                        wprintf(L"   ");

                    wprintf(L"   ");

                    // Print ASCII dump
                    for (i = 0; i < blockSize; i++)
                        putwchar((ULONG)(buffer[i] - ' ') <= (ULONG)('~' - ' ') ? buffer[i] : '.');

                    wprintf(L"\n");

                    address += blockSize;
                    numberOfBytes -= blockSize;
                }
            }

            goto EndCommand;
PrintMemUsage:
            wprintf(L"Usage: mem address [numberOfBytes]\n");
            wprintf(L"Example: mem 12345678 16\n");
        }
        else
        {
            wprintf(L"Unrecognized command.\n");
            goto EndCommand; // get rid of the compiler warning about the label being unreferenced
        }

EndCommand:
        PhDrainAutoPool(&autoPool);
    }

    PhDereferenceObject(DebugConsoleSymbolProvider);

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}
