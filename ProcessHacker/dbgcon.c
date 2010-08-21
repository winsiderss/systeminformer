/*
 * Process Hacker - 
 *   debug console
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

#include <phapp.h>
#include <phsync.h>
#include <refp.h>

VOID PhpPrintHashtableStatistics(
    __in PPH_HASHTABLE Hashtable
    );

NTSTATUS PhpDebugConsoleThreadStart(
    __in PVOID Parameter
    );

static HANDLE DebugConsoleThreadHandle;
static PPH_SYMBOL_PROVIDER DebugConsoleSymbolProvider;

static PPH_HASHTABLE ObjectListSnapshot = NULL;
#ifdef DEBUG
static PPH_LIST NewObjectList = NULL;
static PH_QUEUED_LOCK NewObjectListLock;
#endif

VOID PhShowDebugConsole()
{
    if (!AllocConsole())
        return;

    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    freopen("CONIN$", "r", stdin);
    DebugConsoleThreadHandle = PhCreateThread(0, PhpDebugConsoleThreadStart, NULL); 
}

static BOOLEAN NTAPI PhpLoadCurrentProcessSymbolsCallback(
    __in PPH_MODULE_INFO Module,
    __in_opt PVOID Context
    )
{
    PhLoadModuleSymbolProvider((PPH_SYMBOL_PROVIDER)Context, Module->FileName->Buffer,
        (ULONG64)Module->BaseAddress, Module->Size);

    return TRUE;
}

static PWSTR PhpGetSymbolForAddress(
    __in PVOID Address
    )
{
    return ((PPH_STRING)PHA_DEREFERENCE(PhGetSymbolFromAddress(
        DebugConsoleSymbolProvider, (ULONG64)Address, NULL, NULL, NULL, NULL
        )))->Buffer;
}

static VOID PhpPrintObjectInfo(
    __in PPH_OBJECT_HEADER ObjectHeader,
    __in LONG RefToSubtract
    )
{
    WCHAR c = ' ';

    wprintf(L"%Ix", PhObjectHeaderToObject(ObjectHeader));

    wprintf(L"\t% 20s", ObjectHeader->Type->Name);

    if (ObjectHeader->Flags & PHOBJ_FROM_SMALL_FREE_LIST)
        c = 'f';
    else if (ObjectHeader->Flags & PHOBJ_FROM_TYPE_FREE_LIST)
        c = 'F';

    wprintf(L"\t%4d %c", ObjectHeader->RefCount - RefToSubtract, c);

    if (!ObjectHeader->Type)
    {
        // Dummy
    }
    else if (ObjectHeader->Type == PhObjectTypeObject)
    {
        wprintf(L"\t%.32s", ((PPH_OBJECT_TYPE)PhObjectHeaderToObject(ObjectHeader))->Name);
    }
    else if (ObjectHeader->Type == PhStringType)
    {
        wprintf(L"\t%.32s", ((PPH_STRING)PhObjectHeaderToObject(ObjectHeader))->Buffer);
    }
    else if (ObjectHeader->Type == PhAnsiStringType)
    {
        wprintf(L"\t%.32S", ((PPH_ANSI_STRING)PhObjectHeaderToObject(ObjectHeader))->Buffer);
    }
    else if (ObjectHeader->Type == PhFullStringType)
    {
        wprintf(L"\t%.32s", ((PPH_FULL_STRING)PhObjectHeaderToObject(ObjectHeader))->Buffer);
    }
    else if (ObjectHeader->Type == PhListType)
    {
        wprintf(L"\tCount: %u", ((PPH_LIST)PhObjectHeaderToObject(ObjectHeader))->Count);
    }
    else if (ObjectHeader->Type == PhPointerListType)
    {
        wprintf(L"\tCount: %u", ((PPH_POINTER_LIST)PhObjectHeaderToObject(ObjectHeader))->Count);
    }
    else if (ObjectHeader->Type == PhQueueType)
    {
        wprintf(L"\tCount: %u", ((PPH_QUEUE)PhObjectHeaderToObject(ObjectHeader))->Count);
    }
    else if (ObjectHeader->Type == PhHashtableType)
    {
        wprintf(L"\tCount: %u", ((PPH_HASHTABLE)PhObjectHeaderToObject(ObjectHeader))->Count);
    }
    else if (ObjectHeader->Type == PhProcessItemType)
    {
        wprintf(
            L"\t%.28s (%Id)",
            (ULONG)((PPH_PROCESS_ITEM)PhObjectHeaderToObject(ObjectHeader))->ProcessName->Buffer,
            (ULONG)((PPH_PROCESS_ITEM)PhObjectHeaderToObject(ObjectHeader))->ProcessId
            );
    }
    else if (ObjectHeader->Type == PhServiceItemType)
    {
        wprintf(L"\t%s",
            (ULONG)((PPH_SERVICE_ITEM)PhObjectHeaderToObject(ObjectHeader))->Name->Buffer);
    }
    else if (ObjectHeader->Type == PhThreadItemType)
    {
        wprintf(L"\tTID: %u",
            (ULONG)((PPH_THREAD_ITEM)PhObjectHeaderToObject(ObjectHeader))->ThreadId);
    }

    wprintf(L"\n");
}

static VOID PhpDumpObjectInfo(
    __in PPH_OBJECT_HEADER ObjectHeader
    )
{
    __try
    {
        wprintf(L"Type: %s\n", ObjectHeader->Type->Name);

        if (ObjectHeader->Type == PhObjectTypeObject)
        {
            wprintf(L"%s\n", ((PPH_OBJECT_TYPE)PhObjectHeaderToObject(ObjectHeader))->Name);
        }
        else if (ObjectHeader->Type == PhStringType)
        {
            wprintf(L"%s\n", ((PPH_STRING)PhObjectHeaderToObject(ObjectHeader))->Buffer);
        }
        else if (ObjectHeader->Type == PhAnsiStringType)
        {
            wprintf(L"%S\n", ((PPH_ANSI_STRING)PhObjectHeaderToObject(ObjectHeader))->Buffer);
        }
        else if (ObjectHeader->Type == PhFullStringType)
        {
            wprintf(L"%s\n", ((PPH_FULL_STRING)PhObjectHeaderToObject(ObjectHeader))->Buffer);
        }
        else if (ObjectHeader->Type == PhHashtableType)
        {
            PhpPrintHashtableStatistics((PPH_HASHTABLE)PhObjectHeaderToObject(ObjectHeader));
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        wprintf(L"Error.\n");
    }
}

static VOID PhpPrintHashtableStatistics(
    __in PPH_HASHTABLE Hashtable
    )
{
    ULONG i;
    ULONG expectedLookupMisses = 0;

    wprintf(L"Count: %u\n", Hashtable->Count);
    wprintf(L"Allocated buckets: %u\n", Hashtable->AllocatedBuckets);
    wprintf(L"Allocated entries: %u\n", Hashtable->AllocatedEntries);
    wprintf(L"Next free entry: %d\n", Hashtable->FreeEntry);
    wprintf(L"Next usable entry: %d\n", Hashtable->NextEntry);

    wprintf(L"Hash function: %s\n", PhpGetSymbolForAddress(Hashtable->HashFunction));
    wprintf(L"Compare function: %s\n", PhpGetSymbolForAddress(Hashtable->CompareFunction));

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
            wprintf(L"%u: ", i);

            // Print out the entry indicies.

            index = Hashtable->Buckets[i];

            while (index != -1)
            {
                wprintf(L"%u", index);

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

    wprintf(L"\nExpected lookup misses: %u\n", expectedLookupMisses);
}

#ifdef DEBUG
static VOID PhpDebugCreateObjectHook(
    __in PVOID Object,
    __in SIZE_T Size,
    __in ULONG Flags,
    __in PPH_OBJECT_TYPE ObjectType
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
static VOID PhpDeleteNewObjectList()
{
    if (NewObjectList)
    {
        ULONG i;

        for (i = 0; i < NewObjectList->Count; i++)
            PhDereferenceObject(NewObjectList->Items[i]);

        PhDereferenceObject(NewObjectList);
        NewObjectList = NULL;
    }
}
#endif

typedef struct _STOPWATCH
{
    LARGE_INTEGER StartCounter;
    LARGE_INTEGER EndCounter;
    LARGE_INTEGER Frequency;
} STOPWATCH, *PSTOPWATCH;

static VOID PhInitializeStopwatch(
    __out PSTOPWATCH Stopwatch
    )
{
    Stopwatch->StartCounter.QuadPart = 0;
    Stopwatch->EndCounter.QuadPart = 0;
}

static VOID PhStartStopwatch(
    __inout PSTOPWATCH Stopwatch
    )
{
    NtQueryPerformanceCounter(&Stopwatch->StartCounter, &Stopwatch->Frequency);
}

static VOID PhStopStopwatch(
    __inout PSTOPWATCH Stopwatch
    )
{
    NtQueryPerformanceCounter(&Stopwatch->EndCounter, NULL);
}

static ULONG PhGetMillisecondsStopwatch(
    __in PSTOPWATCH Stopwatch
    )
{
    LARGE_INTEGER countsPerMs;

    countsPerMs = Stopwatch->Frequency;
    countsPerMs.QuadPart /= 1000;

    return (ULONG)((Stopwatch->EndCounter.QuadPart - Stopwatch->StartCounter.QuadPart) /
        countsPerMs.QuadPart);
}

typedef VOID (FASTCALL *PPHF_RW_LOCK_FUNCTION)(
    __in PVOID Parameter
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

static LONG RwReadersActive;
static LONG RwWritersActive;

static NTSTATUS PhpRwLockTestThreadStart(
    __in PVOID Parameter
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

    for (i = 0; i < RW_ITERS; i++)
    {
        for (j = 0; j < RW_READ_ITERS; j++)
        {
            // Read zone

            context.AcquireShared(context.Parameter);
            _InterlockedIncrement(&RwReadersActive);

            for (m = 0; m < RW_READ_SPIN_ITERS; m++)
                YieldProcessor();

            if (*(volatile LONG *)&RwWritersActive != 0)
            {
                wprintf(L"[fail]: writers active in read zone!\n");
                Sleep(INFINITE);
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

                    if (*(volatile LONG *)&RwReadersActive != 0)
                    {
                        wprintf(L"[fail]: readers active in write zone!\n");
                        Sleep(INFINITE);
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
    __in PRW_TEST_CONTEXT Context
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

    RwReadersActive = 0;
    RwWritersActive = 0;

    for (i = 0; i < RW_PROCESSORS; i++)
    {
        threadHandles[i] = PhCreateThread(0, PhpRwLockTestThreadStart, Context); 
    }

    PhStartStopwatch(&stopwatch);
    NtWaitForMultipleObjects(RW_PROCESSORS, threadHandles, WaitAll, FALSE, NULL);
    PhStopStopwatch(&stopwatch);

    for (i = 0; i < RW_PROCESSORS; i++)
        NtClose(threadHandles[i]);

    wprintf(L"[strs] %s: %ums\n", Context->Name, PhGetMillisecondsStopwatch(&stopwatch));
}

VOID FASTCALL PhfAcquireCriticalSection(
    __in PRTL_CRITICAL_SECTION CriticalSection
    )
{
    RtlEnterCriticalSection(CriticalSection);
}

VOID FASTCALL PhfReleaseCriticalSection(
    __in PRTL_CRITICAL_SECTION CriticalSection
    )
{
    RtlLeaveCriticalSection(CriticalSection);
}

NTSTATUS PhpDebugConsoleThreadStart(
    __in PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool; 

    PhInitializeAutoPool(&autoPool);

    DebugConsoleSymbolProvider = PhCreateSymbolProvider(NtCurrentProcessId());
    PhSetSearchPathSymbolProvider(DebugConsoleSymbolProvider, PhApplicationDirectory->Buffer);
    PhEnumGenericModules(NtCurrentProcessId(), NtCurrentProcess(),
        0, PhpLoadCurrentProcessSymbolsCallback, DebugConsoleSymbolProvider);

#ifdef DEBUG
    PhInitializeQueuedLock(&NewObjectListLock);
    PhDbgCreateObjectHook = PhpDebugCreateObjectHook;
#endif

    while (TRUE)
    {
        static PWSTR delims = L" \t";
        static PWSTR commandDebugOnly = L"This command is not available on non-debug builds.\n";

        WCHAR line[201];
        PWSTR context;
        PWSTR command;

        wprintf(L"dbg>");

        if (!fgetws(line, sizeof(line) / 2 - 1, stdin))
            continue;

        // Remove the terminating new line character.
        line[wcslen(line) - 1] = 0;

        context = NULL;
        command = wcstok_s(line, delims, &context);

        if (!command)
        {
            continue;
        }
        else if (WSTR_IEQUAL(command, L"help"))
        {
            wprintf(
                L"Commands:\n"
                L"testperf\n"
                L"testlocks\n"
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
                );
        }
        else if (WSTR_IEQUAL(command, L"testperf"))
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

            wprintf(L"Mutex: %ums\n", PhGetMillisecondsStopwatch(&stopwatch));

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
        else if (WSTR_IEQUAL(command, L"testlocks"))
        {
            RW_TEST_CONTEXT testContext;
            PH_FAST_LOCK fastLock;
            PH_QUEUED_LOCK queuedLock;
            PH_RESOURCE_LOCK resourceLock;
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

            testContext.Name = L"ResourceLock";
            testContext.AcquireExclusive = PhfAcquireResourceLockExclusive;
            testContext.AcquireShared = PhfAcquireResourceLockShared;
            testContext.ReleaseExclusive = PhfReleaseResourceLockExclusive;
            testContext.ReleaseShared = PhfReleaseResourceLockShared;
            testContext.Parameter = &resourceLock;
            PhInitializeResourceLock(&resourceLock);
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
        else if (WSTR_IEQUAL(command, L"objects"))
        {
#ifdef DEBUG
            PWSTR typeFilter = wcstok_s(NULL, delims, &context);
            PLIST_ENTRY currentEntry;
            ULONG totalNumberOfObjects = 0;
            SIZE_T totalNumberOfBytes = 0;

            if (typeFilter)
                wcslwr(typeFilter);

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
                totalNumberOfBytes += objectHeader->Size;

                if (typeFilter)
                {
                    wcscpy_s(typeName, sizeof(typeName) / 2, objectHeader->Type->Name);
                    wcslwr(typeName);
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

            wprintf(L"Total number: %u\n", totalNumberOfObjects);
            wprintf(L"Total size (excl. header): %s\n",
                ((PPH_STRING)PHA_DEREFERENCE(PhFormatSize(totalNumberOfBytes, 1)))->Buffer);
            wprintf(L"Total overhead (header): %s\n",
                ((PPH_STRING)PHA_DEREFERENCE(
                PhFormatSize(PhpAddObjectHeaderSize(0) * totalNumberOfObjects, 1)
                ))->Buffer);
#else
            wprintf(commandDebugOnly);
#endif
        }
        else if (WSTR_IEQUAL(command, L"objtrace"))
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

                    message = PhGetNtMessage(GetExceptionCode());
                    PHA_DEREFERENCE(message);
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
        else if (WSTR_IEQUAL(command, L"objmksnap"))
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
        else if (WSTR_IEQUAL(command, L"objcmpsnap"))
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
        else if (WSTR_IEQUAL(command, L"objmknew"))
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
        else if (WSTR_IEQUAL(command, L"objdelnew"))
        {
#ifdef DEBUG
            PhAcquireQueuedLockExclusive(&NewObjectListLock);
            PhpDeleteNewObjectList();
            PhReleaseQueuedLockExclusive(&NewObjectListLock);
#else
            wprintf(commandDebugOnly);
#endif
        }
        else if (WSTR_IEQUAL(command, L"objviewnew"))
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
        else if (WSTR_IEQUAL(command, L"dumpobj"))
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
        else if (WSTR_IEQUAL(command, L"dumpautopool"))
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
        else if (WSTR_IEQUAL(command, L"threads"))
        {
#ifdef DEBUG
            PLIST_ENTRY currentEntry;

            PhAcquireQueuedLockShared(&PhDbgThreadListLock);

            currentEntry = PhDbgThreadListHead.Flink;

            while (currentEntry != &PhDbgThreadListHead)
            {
                PPHP_BASE_THREAD_DBG dbg;

                dbg = CONTAINING_RECORD(currentEntry, PHP_BASE_THREAD_DBG, ListEntry);

                wprintf(L"Thread %u\n", (ULONG)dbg->ClientId.UniqueThread);
                wprintf(L"\tStart Address: %s\n", PhpGetSymbolForAddress(dbg->StartAddress));
                wprintf(L"\tParameter: %Ix\n", dbg->Parameter);
                wprintf(L"\tCurrent auto-pool: %Ix\n", dbg->CurrentAutoPool);

                currentEntry = currentEntry->Flink;
            }

            PhReleaseQueuedLockShared(&PhDbgThreadListLock);
#else
            wprintf(commandDebugOnly);
#endif
        }
        else if (WSTR_IEQUAL(command, L"provthreads"))
        {
#ifdef DEBUG
            PLIST_ENTRY currentEntry;

            PhAcquireQueuedLockShared(&PhDbgProviderListLock);

            currentEntry = PhDbgProviderListHead.Flink;

            while (currentEntry != &PhDbgProviderListHead)
            {
                PPH_PROVIDER_THREAD providerThread;
                THREAD_BASIC_INFORMATION basicInfo;
                PLIST_ENTRY providerEntry;

                providerThread = CONTAINING_RECORD(currentEntry, PH_PROVIDER_THREAD, DbgListEntry);

                if (providerThread->ThreadHandle)
                {
                    PhGetThreadBasicInformation(providerThread->ThreadHandle, &basicInfo);
                    wprintf(L"Thread %u\n", (ULONG)basicInfo.ClientId.UniqueThread);
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

                    wprintf(L"\tProvider registration at %Ix\n", registration);
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

                currentEntry = currentEntry->Flink;
            }

            PhReleaseQueuedLockShared(&PhDbgProviderListLock);
#else
            wprintf(commandDebugOnly);
#endif
        }
        else if (WSTR_IEQUAL(command, L"workqueues"))
        {
#ifdef DEBUG
            PLIST_ENTRY currentEntry;

            PhAcquireQueuedLockShared(&PhDbgWorkQueueListLock);

            currentEntry = PhDbgWorkQueueListHead.Flink;

            while (currentEntry != &PhDbgWorkQueueListHead)
            {
                PPH_WORK_QUEUE workQueue;
                PLIST_ENTRY workQueueItemEntry;

                workQueue = CONTAINING_RECORD(currentEntry, PH_WORK_QUEUE, DbgListEntry);

                wprintf(L"Work queue at %s\n", PhpGetSymbolForAddress(workQueue));
                wprintf(L"Maximum threads: %u\n", workQueue->MaximumThreads);
                wprintf(L"Minimum threads: %u\n", workQueue->MinimumThreads);
                wprintf(L"No work timeout: %d\n", workQueue->NoWorkTimeout);

                wprintf(L"Current threads: %u\n", workQueue->CurrentThreads);
                wprintf(L"Busy threads: %u\n", workQueue->BusyThreads);

                PhAcquireQueuedLockExclusive(&workQueue->QueueLock);

                // List the items backwards.
                workQueueItemEntry = workQueue->QueueListHead.Blink;

                while (workQueueItemEntry != &workQueue->QueueListHead)
                {
                    PPH_WORK_QUEUE_ITEM workQueueItem;

                    workQueueItem = CONTAINING_RECORD(workQueueItemEntry, PH_WORK_QUEUE_ITEM, ListEntry);

                    wprintf(L"\tWork queue item at %Ix\n", workQueueItem);
                    wprintf(L"\t\tFunction: %s\n", PhpGetSymbolForAddress(workQueueItem->Function));
                    wprintf(L"\t\tContext: %Ix\n", workQueueItem->Context);

                    workQueueItemEntry = workQueueItemEntry->Blink;
                }

                PhReleaseQueuedLockExclusive(&workQueue->QueueLock);

                wprintf(L"\n");

                currentEntry = currentEntry->Flink;
            }

            PhReleaseQueuedLockShared(&PhDbgWorkQueueListLock);
#else
            wprintf(commandDebugOnly);
#endif
        }
        else if (WSTR_IEQUAL(command, L"procrecords"))
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
                    ((PPH_STRING)PHA_DEREFERENCE(PhFormatDate(&systemTime, NULL)))->Buffer,
                    ((PPH_STRING)PHA_DEREFERENCE(PhFormatTime(&systemTime, NULL)))->Buffer
                    );

                startRecord = record;

                do
                {
                    wprintf(L"\t%s (%u) (refs: %d)\n", record->ProcessName->Buffer, (ULONG)record->ProcessId, record->RefCount);

                    if (record->FileName)
                        wprintf(L"\t\t%s\n", record->FileName->Buffer);

                    record = CONTAINING_RECORD(record->ListEntry.Flink, PH_PROCESS_RECORD, ListEntry);
                } while (record != startRecord);
            }

            PhReleaseQueuedLockShared(&PhProcessRecordListLock);
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
