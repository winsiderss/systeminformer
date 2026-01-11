/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2019-2023
 *
 */

/*
 * This file contains basic low-level code as well as general algorithms and data structures.
 *
 * Memory allocation. PhAllocate is a wrapper around RtlAllocateHeap, and always allocates from the
 * phlib heap. PhAllocatePage is a wrapper around NtAllocateVirtualMemory and allocates pages.
 *
 * Null-terminated strings. The Ph*StringZ functions manipulate null-terminated strings. The copying
 * functions provide a simple way to copy strings which may not be null-terminated, but have a
 * specified limit.
 *
 * String. The design of the string object was chosen for maximum compatibility. As such each string
 * buffer must be null-terminated, and each object contains an embedded PH_STRINGREF structure. Note
 * that efficient sub-string creation (no copying, only references the parent string object) could
 * not be implemented due to the mandatory null-termination. String objects must be regarded as
 * immutable (for thread-safety reasons) unless the object has just been created and no references
 * have been shared.
 *
 * String builder. This is a set of functions which allow for efficient modification of strings. For
 * performance reasons, these functions modify string objects directly, even though they are
 * normally immutable.
 *
 * List. A simple PVOID list that resizes itself when needed.
 *
 * Pointer list. Similar to the normal list object, but uses a free list in order to support
 * constant time insertion and deletion. In order for the free list to work, normal entries have
 * their lowest bit clear while free entries have their lowest bit set, with the index of the next
 * free entry in the upper bits.
 *
 * Hashtable. A hashtable with power-of-two bucket sizes and with all entries stored in a single
 * array. This improves locality but may be inefficient when resizing the hashtable. It is a good
 * idea to store pointers to objects as entries, as opposed to the objects themselves.
 *
 * Simple hashtable. A wrapper around the normal hashtable, with PVOID keys and PVOID values.
 *
 * Free list. A thread-safe memory allocation method where freed blocks are stored in a S-list, and
 * allocations are made from this list whenever possible.
 *
 * Callback. A thread-safe notification mechanism where clients can register callback functions
 * which are then invoked by other code.
 */

#include <phbase.h>
#include <phintrnl.h>
#include <phintrin.h>
#include <phnative.h>
#include <circbuf.h>
#include <thirdparty.h>
#include <ntintsafe.h>

#include <trace.h>

#ifndef PH_NATIVE_STRING_CONVERSION
#define PH_NATIVE_STRING_CONVERSION 1
#endif

#ifndef PH_NATIVE_THREAD_CREATE
#define PH_NATIVE_THREAD_CREATE 1
#endif

#ifndef PHNT_NATIVE_TIME
#define PHNT_NATIVE_TIME 1
#endif

typedef struct _PHP_BASE_THREAD_CONTEXT
{
    PUSER_THREAD_START_ROUTINE StartAddress;
    PVOID Parameter;
} PHP_BASE_THREAD_CONTEXT, *PPHP_BASE_THREAD_CONTEXT;

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI PhpListDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI PhpPointerListDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI PhpHashtableDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

// Types

PPH_OBJECT_TYPE PhStringType = NULL;
PPH_OBJECT_TYPE PhBytesType = NULL;
PPH_OBJECT_TYPE PhListType = NULL;
PPH_OBJECT_TYPE PhPointerListType = NULL;
PPH_OBJECT_TYPE PhHashtableType = NULL;

// Misc.

static PPH_STRING PhSharedEmptyString = NULL;

// Threads

static PH_FREE_LIST PhpBaseThreadContextFreeList;
#ifdef DEBUG
ULONG PhDbgThreadDbgTlsIndex;
RTL_STATIC_LIST_HEAD(PhDbgThreadListHead);
PH_QUEUED_LOCK PhDbgThreadListLock = PH_QUEUED_LOCK_INIT;
#endif

// Data

static CONST ULONG PhpPrimeNumbers[] =
{
    0x3, 0x7, 0xb, 0x11, 0x17, 0x1d, 0x25, 0x2f, 0x3b, 0x47, 0x59, 0x6b, 0x83,
    0xa3, 0xc5, 0xef, 0x125, 0x161, 0x1af, 0x209, 0x277, 0x2f9, 0x397, 0x44f,
    0x52f, 0x63d, 0x78b, 0x91d, 0xaf1, 0xd2b, 0xfd1, 0x12fd, 0x16cf, 0x1b65,
    0x20e3, 0x2777, 0x2f6f, 0x38ff, 0x446f, 0x521f, 0x628d, 0x7655, 0x8e01,
    0xaa6b, 0xcc89, 0xf583, 0x126a7, 0x1619b, 0x1a857, 0x1fd3b, 0x26315, 0x2dd67,
    0x3701b, 0x42023, 0x4f361, 0x5f0ed, 0x72125, 0x88e31, 0xa443b, 0xc51eb,
    0xec8c1, 0x11bdbf, 0x154a3f, 0x198c4f, 0x1ea867, 0x24ca19, 0x2c25c1, 0x34fa1b,
    0x3f928f, 0x4c4987, 0x5b8b6f, 0x6dda89
};

/**
 * Initializes the base support module.
 */
NTSTATUS PhBaseInitialization(
    VOID
    )
{
    PH_OBJECT_TYPE_PARAMETERS parameters;

    PhStringType = PhCreateObjectType(L"String", 0, NULL);
    PhBytesType = PhCreateObjectType(L"Bytes", 0, NULL);

    memset(&parameters, 0, sizeof(PH_OBJECT_TYPE_PARAMETERS));
    parameters.FreeListSize = sizeof(PH_LIST);
    parameters.FreeListCount = 128;

    PhListType = PhCreateObjectTypeEx(L"List", PH_OBJECT_TYPE_USE_FREE_LIST, PhpListDeleteProcedure, &parameters);
    PhPointerListType = PhCreateObjectType(L"PointerList", 0, PhpPointerListDeleteProcedure);

    memset(&parameters, 0, sizeof(PH_OBJECT_TYPE_PARAMETERS));
    parameters.FreeListSize = sizeof(PH_HASHTABLE);
    parameters.FreeListCount = 64;

    PhHashtableType = PhCreateObjectTypeEx(L"Hashtable", PH_OBJECT_TYPE_USE_FREE_LIST, PhpHashtableDeleteProcedure, &parameters);

    PhInitializeFreeList(&PhpBaseThreadContextFreeList, sizeof(PHP_BASE_THREAD_CONTEXT), 16);

#ifdef DEBUG
    PhDbgThreadDbgTlsIndex = PhTlsAlloc();

    if (PhDbgThreadDbgTlsIndex == TLS_OUT_OF_INDEXES)
        return STATUS_NO_MEMORY;
#endif

    return STATUS_SUCCESS;
}

/**
 * Entry point for a base thread in the system.
 *
 * This function serves as the start routine for a user thread. It initializes COM for the thread,
 * sets up debugging information (in debug builds), and invokes the user-supplied thread function.
 * After the user function returns, it performs necessary cleanup, including COM uninitialization
 * and removal of debugging information.
 * \param Parameter Pointer to a PHP_BASE_THREAD_CONTEXT structure containing the user-supplied
 *        thread start address and parameter.
 * \return NTSTATUS Status code returned by the user-supplied thread function.
 */
_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS PhpBaseThreadStart(
    _In_ PVOID Parameter
    )
{
    NTSTATUS status;
    HRESULT result;
    PHP_BASE_THREAD_CONTEXT context;
#ifdef DEBUG
    PHP_BASE_THREAD_DBG dbg;
#endif

    context = *(PPHP_BASE_THREAD_CONTEXT)Parameter;
    PhFreeToFreeList(&PhpBaseThreadContextFreeList, Parameter);

#ifdef DEBUG
    memset(&dbg, 0, sizeof(PHP_BASE_THREAD_DBG));
    dbg.ClientId = NtCurrentTeb()->ClientId;
    dbg.StartAddress = context.StartAddress;
    dbg.Parameter = context.Parameter;
    dbg.CurrentAutoPool = NULL;

    PhAcquireQueuedLockExclusive(&PhDbgThreadListLock);
    InsertTailList(&PhDbgThreadListHead, &dbg.ListEntry);
    PhReleaseQueuedLockExclusive(&PhDbgThreadListLock);

    PhTlsSetValue(PhDbgThreadDbgTlsIndex, &dbg);
#endif

    // Initialization code

    result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    // Call the user-supplied function.

    status = context.StartAddress(context.Parameter);

    // De-initialization code

    if (result == S_OK || result == S_FALSE)
        CoUninitialize();

#ifdef DEBUG
    PhAcquireQueuedLockExclusive(&PhDbgThreadListLock);
    RemoveEntryList(&dbg.ListEntry);
    PhReleaseQueuedLockExclusive(&PhDbgThreadListLock);
#endif

    return status;
}

/**
 * Creates a thread.
 *
 * \param ProcessHandle A handle to the process in which the thread is to be created.
 * \param ThreadSecurityDescriptor A pointer to a security descriptor for the new thread and
 * \a determines whether child processes can inherit the thread handle.
 * \param DesiredAccess The desired access to the thread.
 * \param CreateFlags The flags that control the creation of the thread.
 * \param ZeroBits The number of high-order address bits that must be zero.
 * \a If this parameter is zero, the new thread uses the default for the executable.
 * \param StackSize The initial size of the stack, in bytes. The system rounds this value to the nearest page.
 * \a If this parameter is zero, the new thread uses the default size for the executable.
 * \param MaximumStackSize The maximum size of the stack, in bytes. The system rounds this value to the nearest page.
 * \a If this parameter is zero, the new thread uses the default maximum for the executable.
 * \param StartRoutine A pointer to the starting address of the thread.
 * \param Argument A pointer to a variable to be passed to the StartRoutine.
 * \param ThreadHandle A pointer to a variable that receives a handle to the new thread.
 * \param ClientId A pointer to a variable that receives the thread identifier.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhCreateUserThread(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PSECURITY_DESCRIPTOR ThreadSecurityDescriptor,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ ULONG CreateFlags,
    _In_opt_ SIZE_T ZeroBits,
    _In_opt_ SIZE_T StackSize,
    _In_opt_ SIZE_T MaximumStackSize,
    _In_ PUSER_THREAD_START_ROUTINE StartRoutine,
    _In_opt_ PVOID Argument,
    _Out_opt_ PHANDLE ThreadHandle,
    _Out_opt_ PCLIENT_ID ClientId
    )
{
#if defined(PH_NATIVE_THREAD_CREATE)
    NTSTATUS status;
    HANDLE threadHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UCHAR buffer[FIELD_OFFSET(PS_ATTRIBUTE_LIST, Attributes) + sizeof(PS_ATTRIBUTE[1])] = { 0 };
    PPS_ATTRIBUTE_LIST attributeList = (PPS_ATTRIBUTE_LIST)buffer;
    CLIENT_ID clientId = { NULL, NULL };

    InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, ThreadSecurityDescriptor);
    attributeList->TotalLength = sizeof(buffer);
    attributeList->Attributes[0].Attribute = PS_ATTRIBUTE_CLIENT_ID;
    attributeList->Attributes[0].Size = sizeof(CLIENT_ID);
    attributeList->Attributes[0].ValuePtr = &clientId;
    attributeList->Attributes[0].ReturnLength = NULL;

    status = NtCreateThreadEx(
        &threadHandle,
        DesiredAccess,
        &objectAttributes,
        ProcessHandle,
        StartRoutine,
        Argument,
        CreateFlags,
        ZeroBits,
        StackSize,
        MaximumStackSize,
        attributeList
        );

    if (NT_SUCCESS(status))
    {
        if (ThreadHandle)
        {
            *ThreadHandle = threadHandle;
        }
        else if (threadHandle)
        {
            NtClose(threadHandle);
        }

        if (ClientId)
        {
            *ClientId = clientId;
        }
    }

    return status;
#else
    return RtlCreateUserThread(
        ProcessHandle,
        ThreadSecurityDescriptor,
        BooleanFlagOn(CreateFlags, THREAD_CREATE_FLAGS_CREATE_SUSPENDED),
        (ULONG)ZeroBits,
        MaximumStackSize,
        StackSize,
        StartRoutine,
        Argument,
        ThreadHandle,
        ClientId
        );
#endif
}

/**
 * Creates a thread.
 *
 * \param StackSize The initial stack size of the thread.
 * \param StartAddress The function to execute in the thread.
 * \param Parameter A user-defined value to pass to the function.
 * \return HANDLE A handle to the new thread.
 */
HANDLE PhCreateThread(
    _In_opt_ SIZE_T StackSize,
    _In_ PUSER_THREAD_START_ROUTINE StartAddress,
    _In_opt_ PVOID Parameter
    )
{
    NTSTATUS status;
    HANDLE threadHandle;
    PPHP_BASE_THREAD_CONTEXT context;

    context = PhAllocateFromFreeList(&PhpBaseThreadContextFreeList);
    context->StartAddress = StartAddress;
    context->Parameter = Parameter;

    status = PhCreateUserThread(
        NtCurrentProcess(),
        NULL,
        THREAD_ALL_ACCESS,
        0,
        0,
        0,
        StackSize,
        PhpBaseThreadStart,
        context,
        &threadHandle,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        PHLIB_INC_STATISTIC(BaseThreadsCreated);
        return threadHandle;
    }
    else
    {
        PHLIB_INC_STATISTIC(BaseThreadsCreateFailed);
        PhFreeToFreeList(&PhpBaseThreadContextFreeList, context);
        return NULL;
    }
}

/**
 * Creates a new thread in the current process.
 *
 * \param ThreadHandle Pointer to a variable that receives the handle of the newly created thread.
 * \param StartAddress Pointer to the function to be executed by the thread.
 * \param Parameter Optional parameter to be passed to the thread function.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhCreateThreadEx(
    _Out_ PHANDLE ThreadHandle,
    _In_ PUSER_THREAD_START_ROUTINE StartAddress,
    _In_opt_ PVOID Parameter
    )
{
    NTSTATUS status;
    HANDLE threadHandle;
    PPHP_BASE_THREAD_CONTEXT context;

    context = PhAllocateFromFreeList(&PhpBaseThreadContextFreeList);
    context->StartAddress = StartAddress;
    context->Parameter = Parameter;

    status = PhCreateUserThread(
        NtCurrentProcess(),
        NULL,
        THREAD_ALL_ACCESS,
        0,
        0,
        0,
        0,
        PhpBaseThreadStart,
        context,
        &threadHandle,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        PHLIB_INC_STATISTIC(BaseThreadsCreated);
        *ThreadHandle = threadHandle;
    }
    else
    {
        PHLIB_INC_STATISTIC(BaseThreadsCreateFailed);
        PhFreeToFreeList(&PhpBaseThreadContextFreeList, context);
    }

    return status;
}

/**
 * Creates a new thread and begins execution at the specified start address.
 *
 * \param StartAddress Pointer to the function to be executed by the new thread.
 * \param Parameter Optional parameter to be passed to the thread function.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhCreateThread2(
    _In_ PUSER_THREAD_START_ROUTINE StartAddress,
    _In_opt_ PVOID Parameter
    )
{
    NTSTATUS status;
    PPHP_BASE_THREAD_CONTEXT context;

    context = PhAllocateFromFreeList(&PhpBaseThreadContextFreeList);
    context->StartAddress = StartAddress;
    context->Parameter = Parameter;

    status = PhCreateUserThread(
        NtCurrentProcess(),
        NULL,
        THREAD_ALL_ACCESS,
        0,
        0,
        0,
        0,
        PhpBaseThreadStart,
        context,
        NULL,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        PHLIB_INC_STATISTIC(BaseThreadsCreated);
    }
    else
    {
        PHLIB_INC_STATISTIC(BaseThreadsCreateFailed);
        PhFreeToFreeList(&PhpBaseThreadContextFreeList, context);
    }

    return status;
}

/**
 * Callback function that starts a queued base thread.
 *
 * This function is called by the thread pool when a work item is dequeued.
 * It retrieves the thread context, frees the context structure, and invokes
 * the user-supplied start address with the provided parameter.
 * \param Instance Pointer to the thread pool callback instance.
 * \param Context Pointer to a PHP_BASE_THREAD_CONTEXT structure, which will be freed.
 */
_Function_class_(TP_CALLBACK_ROUTINE)
VOID PhpBaseThreadQueueStart(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _In_ _Frees_ptr_ PVOID Context
    )
{
    PHP_BASE_THREAD_CONTEXT context;

    context = *(PPHP_BASE_THREAD_CONTEXT)Context;
    PhFreeToFreeList(&PhpBaseThreadContextFreeList, Context);

    context.StartAddress(context.Parameter);
}

/**
 * Queues a callback to be executed by a thread pool thread.
 *
 * This function allocates a thread context, initializes a callback environment,
 * and posts the work item to a thread pool. If the operation succeeds, a statistic
 * for created threads is incremented; otherwise, a failure statistic is incremented
 * and the context is freed.
 * \param StartRoutine Pointer to the user-defined thread start routine to execute.
 * \param Parameter Optional parameter to pass to the start routine.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhQueueUserWorkItem(
    _In_ PUSER_THREAD_START_ROUTINE StartRoutine,
    _In_opt_ PVOID Parameter
    )
{
    NTSTATUS status;
    PPHP_BASE_THREAD_CONTEXT context;
    TP_CALLBACK_ENVIRON environment;

    context = PhAllocateFromFreeList(&PhpBaseThreadContextFreeList);
    context->StartAddress = StartRoutine;
    context->Parameter = Parameter;

    TpInitializeCallbackEnviron(&environment);
    TpSetCallbackLongFunction(&environment);
    TpSetCallbackPriority(&environment, TP_CALLBACK_PRIORITY_NORMAL);

    status = TpSimpleTryPost(PhpBaseThreadQueueStart, context, &environment);

    if (NT_SUCCESS(status))
    {
        PHLIB_INC_STATISTIC(BaseThreadsCreated);
    }
    else
    {
        PHLIB_INC_STATISTIC(BaseThreadsCreateFailed);
        PhFreeToFreeList(&PhpBaseThreadContextFreeList, context);
    }

    return status;
}

DOUBLE PhReadTimeStampFrequency(
    VOID
    )
{
    LARGE_INTEGER frequency;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    ULONG_PTR old_affinity = 0;
    DOUBLE elapsed_qpc;
    ULONG64 elapsed_tsc;
    DOUBLE tsc_freq;
    ULONG64 tsc_start;
    ULONG64 tsc_end;

    // Wait interval (in QPC ticks)
    PhQueryPerformanceFrequency(&frequency);
    const LONGLONG interval_ms = 100; // 100 ms
    const LONGLONG interval_ticks = (frequency.QuadPart * interval_ms) / 1000;

    // Warm up
    for (volatile ULONG i = 0UL; i < 1000000UL; ++i) {}

    // Pin thread to one CPU (optional, for best accuracy)
    PhGetThreadAffinityMask(NtCurrentThread(), &old_affinity);
    PhSetThreadAffinityMask(NtCurrentThread(), 1);

    PhQueryPerformanceCounter(&start);
    SpeculationFence();
    tsc_start = ReadTimeStampCounter();
    SpeculationFence();

    // Wait for interval
    for (;;)
    {
        PhQueryPerformanceCounter(&end);

        if ((end.QuadPart - start.QuadPart) >= interval_ticks)
            break;

        YieldProcessor();
    }

    SpeculationFence();
    tsc_end = ReadTimeStampCounter();
    SpeculationFence();

    if (old_affinity)
    {
        PhSetThreadAffinityMask(NtCurrentThread(), old_affinity);
    }

    elapsed_qpc = (DOUBLE)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
    elapsed_tsc = tsc_end - tsc_start;
    tsc_freq = elapsed_tsc / elapsed_qpc;
    return tsc_freq;
}

/**
 * Reads the time stamp counter.
 *
 * This function reads the time stamp counter using the `__rdtscp` instruction,
 * which is a serializing variant of the `rdtsc` instruction. It also includes
 * a memory fence to ensure proper ordering of memory operations.
 * \return The current value of the time stamp counter.
 */
ULONG64 PhReadTimeStampCounter(
    VOID
    )
{
#if defined(PHNT_RDTSCP)
    unsigned int processorIndex;
    ULONG64 value = __rdtscp(&processorIndex);
    SpeculationFence();
    return value;
#else
    MemoryBarrier();
    ULONG64 value = ReadTimeStampCounter();
    MemoryBarrier();
    return value;
#endif
}

// rev from QueryPerformanceCounter (dmex)
/**
 * Retrieves the current value of the performance counter, which is a high resolution (<1us) time stamp that can be used for time-interval measurements.
 *
 * \param PerformanceCounter A pointer to a variable that receives the current performance-counter value, in counts.
 * \return Successful or errant status.
 * \remarks On systems that run Windows XP or later, the function will always succeed and will thus never return zero.
 */
BOOLEAN PhQueryPerformanceCounter(
    _Out_ PLARGE_INTEGER PerformanceCounter
    )
{
#if defined(PH_WIN32_PERFCOUNTER)
    return !!QueryPerformanceCounter(PerformanceCounter);
#elif defined(PH_NATIVE_PERFCOUNTER)
    return NT_SUCCESS(NtQueryPerformanceCounter(PerformanceCounter, NULL));
#else
    return !!RtlQueryPerformanceCounter(PerformanceCounter);
#endif
}

// rev from QueryPerformanceFrequency (dmex)
/**
 * Retrieves the frequency of the performance counter.
 * The frequency of the performance counter is fixed at system boot and is consistent across all processors.
 * Therefore, the frequency need only be queried upon application initialization, and the result can be cached.
 *
 * \param PerformanceFrequency A pointer to a variable that receives the current performance-counter frequency, in counts per second.
 * \return Successful or errant status.
 * \remarks On systems that run Windows XP or later, the function will always succeed and will thus never return zero.
 */
BOOLEAN PhQueryPerformanceFrequency(
    _Out_ PLARGE_INTEGER PerformanceFrequency
    )
{
#if defined(PH_WIN32_PERFCOUNTER)
    return !!QueryPerformanceFrequency(PerformanceFrequency);
#elif defined(PH_NATIVE_PERFCOUNTER)
    LARGE_INTEGER performanceCounter;
    return NT_SUCCESS(NtQueryPerformanceCounter(&performanceCounter, PerformanceFrequency));
#else
    return !!RtlQueryPerformanceFrequency(PerformanceFrequency);
#endif
}

/**
 * Gets the current interrupt-time count.
 */
VOID PhQueryInterruptTime(
    _Out_ PLARGE_INTEGER InterruptTime
    )
{
#if defined(PHNT_NATIVE_TIME)

    while (TRUE)
    {
        InterruptTime->HighPart = USER_SHARED_DATA->InterruptTime.High1Time;
        InterruptTime->LowPart = USER_SHARED_DATA->InterruptTime.LowPart;

        if (InterruptTime->HighPart == USER_SHARED_DATA->InterruptTime.High2Time)
            break;

        YieldProcessor();
    }

#elif defined(PHNT_SYSTEM_TIME)

    ULONGLONG interruptTime;

    QueryInterruptTime(&interruptTime);

    InterruptTime->QuadPart = interruptTime;

#else

    do
    {
        InterruptTime->HighPart = USER_SHARED_DATA->InterruptTime.High1Time;
        InterruptTime->LowPart = USER_SHARED_DATA->InterruptTime.LowPart;
    } while (InterruptTime->HighPart != USER_SHARED_DATA->InterruptTime.High2Time);

#endif
}

/**
 * Gets the current system time (UTC).
 *
 * \remarks Use this function instead of NtQuerySystemTime() because no system calls are involved.
 */
VOID PhQuerySystemTime(
    _Out_ PLARGE_INTEGER SystemTime
    )
{
#if defined(PHNT_NATIVE_TIME)

    while (TRUE)
    {
        SystemTime->HighPart = USER_SHARED_DATA->SystemTime.High1Time;
        SystemTime->LowPart = USER_SHARED_DATA->SystemTime.LowPart;

        if (SystemTime->HighPart == USER_SHARED_DATA->SystemTime.High2Time)
            break;

        YieldProcessor();
    }

#elif defined(PHNT_SYSTRM_TIME)

    SYSTEMTIME systemTime;
    FILETIME fileTime;

    GetSystemTime(&systemTime);
    SystemTimeToFileTime(&systemTime, &fileTime);

    SystemTime->LowPart = fileTime.dwLowDateTime;
    SystemTime->HighPart = fileTime.dwHighDateTime;

#else

    do
    {
        SystemTime->HighPart = USER_SHARED_DATA->SystemTime.High1Time;
        SystemTime->LowPart = USER_SHARED_DATA->SystemTime.LowPart;
    } while (SystemTime->HighPart != USER_SHARED_DATA->SystemTime.High2Time);

#endif
}

/**
 * Gets the offset of the current time zone from UTC.
 *
 * \remarks Use this function instead of GetTimeZoneInformation() because no system calls are involved.
 */
NTSTATUS PhQueryTimeZoneBias(
    _Out_ PLARGE_INTEGER TimeZoneBias
    )
{
#if defined(PHNT_NATIVE_TIME)

    while (TRUE)
    {
        TimeZoneBias->HighPart = USER_SHARED_DATA->TimeZoneBias.High1Time;
        TimeZoneBias->LowPart = USER_SHARED_DATA->TimeZoneBias.LowPart;

        if (TimeZoneBias->HighPart == USER_SHARED_DATA->TimeZoneBias.High2Time)
            break;

        YieldProcessor();
    }

    return STATUS_SUCCESS;
#elif defined(PHNT_SYSTEM_TIME)
    NTSTATUS status;
    SYSTEM_TIMEOFDAY_INFORMATION timeOfDayInfo = { 0 };

    status = NtQuerySystemInformation(
        SystemTimeOfDayInformation,
        &timeOfDayInfo,
        sizeof(SYSTEM_TIMEOFDAY_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        TimeZoneBias->QuadPart = timeOfDayInfo.TimeZoneBias.QuadPart;
    }

    return status;
#else

    do
    {
        TimeZoneBias->HighPart = USER_SHARED_DATA->TimeZoneBias.High1Time;
        TimeZoneBias->LowPart = USER_SHARED_DATA->TimeZoneBias.LowPart;
    } while (TimeZoneBias->HighPart != USER_SHARED_DATA->TimeZoneBias.High2Time);

    return STATUS_SUCCESS;
#endif
}

/**
 * Converts system time to local time.
 *
 * \param SystemTime A UTC time value.
 * \param LocalTime A variable which receives the local time value. This may be the same variable as
 * \a SystemTime.
 * \return NTSTATUS Successful or errant status.
 * \remarks Use this function instead of RtlSystemTimeToLocalTime() because no system calls are
 * involved.
 */
NTSTATUS PhSystemTimeToLocalTime(
    _In_ PLARGE_INTEGER SystemTime,
    _Out_ PLARGE_INTEGER LocalTime
    )
{
#if defined(PHNT_NATIVE_TIME)
    NTSTATUS status;
    LARGE_INTEGER timeZoneBias;

    status = PhQueryTimeZoneBias(&timeZoneBias);

    if (NT_SUCCESS(status))
    {
        LocalTime->QuadPart = SystemTime->QuadPart - timeZoneBias.QuadPart;
    }

    return status;
#else
    return RtlSystemTimeToLocalTime(SystemTime, LocalTime);
#endif
}

/**
 * Converts local time to system time.
 *
 * \param LocalTime A local time value.
 * \param SystemTime A variable which receives the UTC time value. This may be the same variable as
 * \a LocalTime.
 * \return NTSTATUS Successful or errant status.
 * \remarks Use this function instead of RtlLocalTimeToSystemTime() because no system calls are
 * involved.
 */
NTSTATUS PhLocalTimeToSystemTime(
    _In_ PLARGE_INTEGER LocalTime,
    _Out_ PLARGE_INTEGER SystemTime
    )
{
#if defined(PHNT_NATIVE_TIME)
    NTSTATUS status;
    LARGE_INTEGER timeZoneBias;

    status = PhQueryTimeZoneBias(&timeZoneBias);

    if (NT_SUCCESS(status))
    {
        SystemTime->QuadPart = LocalTime->QuadPart + timeZoneBias.QuadPart;
    }

    return status;
#else
    return RtlLocalTimeToSystemTime(LocalTime, SystemTime);
#endif
}

/**
 * Converts a system time value to the number of seconds elapsed since January 1, 1980 (the DOS epoch).
 *
 * \param Time Pointer to a LARGE_INTEGER representing the system time (in 100-nanosecond intervals since January 1, 1601 UTC).
 * \param ElapsedSeconds Pointer to a ULONG that receives the number of seconds since January 1, 1980.
 * \return TRUE if the conversion was successful and the result fits in a ULONG; FALSE otherwise.
 */
BOOLEAN PhTimeToSecondsSince1980(
    _In_ PLARGE_INTEGER Time,
    _Out_ PULONG ElapsedSeconds
    )
{
#if defined(PHNT_NATIVE_TIME)
    LARGE_INTEGER time;

    time.QuadPart = Time->QuadPart - (SecondsToStartOf1980 * PH_TICKS_PER_SEC);
    time.QuadPart /= PH_TICKS_PER_SEC;

    if (time.HighPart)
    {
        *ElapsedSeconds = 0;
        return FALSE;
    }

    *ElapsedSeconds = time.LowPart;
    return TRUE;
#else
    return RtlTimeToSecondsSince1980(Time, ElapsedSeconds);
#endif
}

/**
 * Converts a system time value to the number of seconds elapsed since January 1, 1970 (the Unix epoch).
 *
 * \param Time Pointer to a LARGE_INTEGER representing the system time (in 100-nanosecond intervals since January 1, 1601 UTC).
 * \param ElapsedSeconds Pointer to a ULONG that receives the number of seconds since January 1, 1970.
 * \return TRUE if the conversion was successful and the result fits in a ULONG; FALSE otherwise.
 */
BOOLEAN PhTimeToSecondsSince1970(
    _In_ PLARGE_INTEGER Time,
    _Out_ PULONG ElapsedSeconds
    )
{
#if defined(PHNT_NATIVE_TIME)
    LARGE_INTEGER time;

    time.QuadPart = Time->QuadPart - (SecondsToStartOf1970 * PH_TICKS_PER_SEC);
    time.QuadPart /= PH_TICKS_PER_SEC;

    if (time.HighPart)
    {
        *ElapsedSeconds = 0;
        return FALSE;
    }

    *ElapsedSeconds = time.LowPart;
    return TRUE;
#else
    return RtlTimeToSecondsSince1970(Time, ElapsedSeconds);
#endif
}

/**
 * Converts the number of seconds elapsed since 1980 to a system time value.
 *
 * \param ElapsedSeconds The number of seconds since January 1, 1980.
 * \param Time Pointer to a LARGE_INTEGER that receives the converted time value.
 */
VOID PhSecondsSince1980ToTime(
    _In_ ULONG ElapsedSeconds,
    _Out_ PLARGE_INTEGER Time
    )
{
#if defined(PHNT_NATIVE_TIME)
    Time->QuadPart = PH_TICKS_PER_SEC * (SecondsToStartOf1980 + ElapsedSeconds);
#else
    RtlSecondsSince1980ToTime(ElapsedSeconds, Time);
#endif
}

/**
 * Converts the number of seconds elapsed since January 1, 1970 (the Unix epoch)
 * to a Windows FILETIME-compatible 64-bit time value.
 *
 * \param ElapsedSeconds The number of seconds since January 1, 1970.
 * \param Time Pointer to a LARGE_INTEGER that receives the converted time value.
 */
VOID PhSecondsSince1970ToTime(
    _In_ ULONG ElapsedSeconds,
    _Out_ PLARGE_INTEGER Time
    )
{
#if defined(PHNT_NATIVE_TIME)
    Time->QuadPart = PH_TICKS_PER_SEC * (SecondsToStartOf1970 + ElapsedSeconds);
#else
    RtlSecondsSince1970ToTime(ElapsedSeconds, Time);
#endif
}

/**
 * Allocates a block of memory.
 *
 * \param Size The number of bytes to allocate.
 * \return A pointer to the allocated block of memory.
 * \remarks If the function fails to allocate the block of memory, it raises an exception. The block
 * is guaranteed to be aligned at MEMORY_ALLOCATION_ALIGNMENT bytes.
 */
_Use_decl_annotations_
PVOID PhAllocate(
    _In_ SIZE_T Size
    )
{
    assert(Size > 0 && Size < PH_LARGE_BUFFER_SIZE);
#if defined(PH_DEBUG_HEAP)
    return malloc(Size);
#else
    return RtlAllocateHeap(PhHeapHandle, HEAP_GENERATE_EXCEPTIONS, Size);
#endif
}

/**
 * Allocates a block of memory.
 *
 * \param Size The number of bytes to allocate.
 * \return A pointer to the allocated block of memory,
 * or NULL if the block could not be allocated.
 */
_Use_decl_annotations_
PVOID PhAllocateSafe(
    _In_ SIZE_T Size
    )
{
    assert(Size > 0 && Size < PH_LARGE_BUFFER_SIZE);
#if defined(PH_DEBUG_HEAP)
    return malloc(Size);
#else
    return RtlAllocateHeap(PhHeapHandle, 0, Size);
#endif
}

/**
 * Allocates a block of memory.
 *
 * \param Size The number of bytes to allocate.
 * \param Flags Flags controlling the allocation.
 * \return A pointer to the allocated block of memory,
 * or NULL if the block could not be allocated.
 */
_Use_decl_annotations_
PVOID PhAllocateExSafe(
    _In_ SIZE_T Size,
    _In_ ULONG Flags
    )
{
    assert(Size > 0 && Size < PH_LARGE_BUFFER_SIZE);
#if defined(PH_DEBUG_HEAP)
    return malloc(Size);
#else
    return RtlAllocateHeap(PhHeapHandle, Flags, Size);
#endif
}

/**
 * Frees a block of memory allocated with PhAllocate().
 *
 * \param Memory A pointer to a block of memory.
 */
_Use_decl_annotations_
VOID PhFree(
    _Frees_ptr_opt_ PVOID Memory
    )
{
#if defined(PH_DEBUG_HEAP)
    free(Memory);
#else
    RtlFreeHeap(PhHeapHandle, 0, Memory);
#endif
}

/**
 * Re-allocates a block of memory originally allocated with PhAllocate().
 *
 * \param Memory A pointer to a block of memory.
 * \param Size The new size of the memory block, in bytes.
 * \return A pointer to the new block of memory. The existing contents of the memory block are
 * copied to the new block.
 * \remarks If the function fails to allocate the block of memory, it raises an exception.
 */
_Use_decl_annotations_
PVOID PhReAllocate(
    _Frees_ptr_opt_ PVOID Memory,
    _In_ SIZE_T Size
    )
{
    assert(Size > 0 && Size < PH_LARGE_BUFFER_SIZE);
#if defined(PH_DEBUG_HEAP)
    return realloc(Memory, Size);
#else
    if (Size == 0)
    {
        if (Memory)
            PhFree(Memory);
        return NULL;
    }
    if (Memory)
    {
        return RtlReAllocateHeap(PhHeapHandle, HEAP_GENERATE_EXCEPTIONS, Memory, Size);
    }

    return RtlAllocateHeap(PhHeapHandle, HEAP_GENERATE_EXCEPTIONS, Size);
#endif
}

/**
 * Re-allocates a block of memory originally allocated with PhAllocate().
 *
 * \param Memory A pointer to a block of memory.
 * \param Size The new size of the memory block, in bytes.
 * \return A pointer to the new block of memory, or NULL if the block could not be allocated. The
 * existing contents of the memory block are copied to the new block.
 */
_Use_decl_annotations_
PVOID PhReAllocateSafe(
    _In_opt_ PVOID Memory,
    _In_ SIZE_T Size
    )
{
    assert(Size > 0 && Size < PH_LARGE_BUFFER_SIZE);
#if defined(PH_DEBUG_HEAP)
    return realloc(Memory, Size);
#else
    if (Size == 0)
    {
        if (Memory)
            PhFree(Memory);
        return NULL;
    }
    if (Memory)
    {
        return RtlReAllocateHeap(PhHeapHandle, 0, Memory, Size);
    }

    return RtlAllocateHeap(PhHeapHandle, 0, Size);
#endif
}

_Use_decl_annotations_
SIZE_T PhSizeHeap(
    _In_ PVOID Memory
    )
{
#if defined(PH_DEBUG_HEAP)
    return _msize(Memory);
#else
    return RtlSizeHeap(PhHeapHandle, 0, Memory);
#endif
}

/**
 * Allocates pages of memory.
 *
 * \param Size The number of bytes to allocate. The number of pages allocated will be large enough
 * to contain \a Size bytes.
 * \param NewSize The number of bytes actually allocated. This is \a Size rounded up to the next
 * multiple of PAGE_SIZE.
 * \return A pointer to the allocated block of memory, or NULL if the block could not be allocated.
 */
_Use_decl_annotations_
PVOID PhAllocatePage(
    _In_ SIZE_T Size,
    _Out_opt_ PSIZE_T NewSize
    )
{
    PVOID baseAddress;
    SIZE_T regionSize;

    baseAddress = NULL;
    regionSize = Size;

    if (NT_SUCCESS(NtAllocateVirtualMemory(
        NtCurrentProcess(),
        &baseAddress,
        0,
        &regionSize,
        MEM_COMMIT,
        PAGE_READWRITE
        )))
    {
        if (NewSize)
            *NewSize = regionSize;

        return baseAddress;
    }
    else
    {
        return NULL;
    }
}

/**
 * Frees pages of memory allocated with PhAllocatePage().
 *
 * \param Memory A pointer to a block of memory.
 */
VOID PhFreePage(
    _In_ _Frees_ptr_ PVOID Memory
    )
{
    PhFreeVirtualMemory(NtCurrentProcess(), Memory, MEM_RELEASE);
}

/**
 * Allocates pages of memory.
 *
 * \param Size The number of bytes to allocate. The number of pages allocated will be large enough to contain \a Size bytes.
 * \param Alignment The alignment value, which must be an integer power of 2.
 * \return A pointer to the allocated block of memory, or NULL if the block could not be allocated.
 */
_Use_decl_annotations_
PVOID PhAllocatePageAligned(
    _In_ SIZE_T Size,
    _In_ SIZE_T Alignment
    )
{
    return _aligned_malloc(Size, Alignment);

    //NTSTATUS status;
    //PVOID baseAddress = NULL;
    //MEM_EXTENDED_PARAMETER extended[1];
    //MEM_ADDRESS_REQUIREMENTS requirements;
    //
    //memset(&requirements, 0, sizeof(MEM_ADDRESS_REQUIREMENTS));
    ////requirements.HighestEndingAddress = (PVOID)(ULONG_PTR)0x7fffffff; // Below 2GB
    //requirements.Alignment = Alignment;
    //
    //memset(extended, 0, sizeof(extended));
    //extended[0].Type = MemExtendedParameterAddressRequirements;
    //extended[0].Pointer = &requirements;
    //
    //status = NtAllocateVirtualMemoryEx(
    //    NtCurrentProcess(),
    //    &baseAddress,
    //    &Size,
    //    MEM_RESERVE | MEM_COMMIT,
    //    PAGE_READWRITE,
    //    extended,
    //    RTL_NUMBER_OF(extended)
    //    );
    //
    //if (NT_SUCCESS(status))
    //{
    //    if (NewSize)
    //        *NewSize = Size;
    //    if (BaseAddress)
    //        *BaseAddress = baseAddress;
    //}
    //
    //return status;
}

/**
 * Frees pages of memory allocated with PhAllocatePageAligned().
 *
 * \param Memory A pointer to a block of memory.
 */
VOID PhFreePageAligned(
    _In_ _Frees_ptr_ PVOID Memory
    )
{
    _aligned_free(Memory);
}

/**
 * Reserves, commits, or both, a region of pages within the user-mode virtual address space of a specified process.
 *
 * \param ProcessHandle A handle to the process.
 * \param BaseAddress The pointer that specifies a desired starting address for the region of pages that you want to allocate.
 * If you are reserving memory, the function rounds this address down to the nearest multiple of the allocation granularity.
 * If you are committing memory that is already reserved, the function rounds this address down to the nearest page boundary.
 * \param AllocationSize The size of the region of memory to allocate, in bytes. If BaseAddress is NULL, the function rounds Size up to the next page boundary.
 * \param AllocationType The type of memory allocation.
 * \param Protection The type of memory protection.
 * \return Successful or errant status.
 */
NTSTATUS PhAllocateVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _Out_ PVOID* BaseAddress,
    _In_ SIZE_T AllocationSize,
    _In_ ULONG AllocationType,
    _In_ ULONG Protection
    )
{
    NTSTATUS status;
    PVOID baseAddress = NULL;
    SIZE_T allocationSize = AllocationSize;

    status = NtAllocateVirtualMemory(
        ProcessHandle,
        &baseAddress,
        0,
        &allocationSize,
        AllocationType,
        Protection
        );

    if (NT_SUCCESS(status))
    {
        *BaseAddress = baseAddress;
    }

    return status;
}

/**
 * Releases, decommits, or both releases and decommits, a region of pages within the virtual address space of a specified process.
 *
 * \param ProcessHandle A handle to the process.
 * \param BaseAddress The pointer that specifies a desired starting address for the region of pages that you want to allocate.
 * \param FreeType A bitmask containing flags that describe the type of free operation.
 * \return Successful or errant status.
 */
NTSTATUS PhFreeVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ ULONG FreeType
    )
{
    NTSTATUS status;
    PVOID regionAddress = BaseAddress;
    SIZE_T regionSize = 0;

    status = NtFreeVirtualMemory(
        ProcessHandle,
        &regionAddress,
        &regionSize,
        FreeType
        );

    //if (status == STATUS_INVALID_PAGE_PROTECTION)
    //{
    //    if (RtlFlushSecureMemoryCache(regionAddress, regionSize))
    //    {
    //        status = NtFreeVirtualMemory(
    //            ProcessHandle,
    //            &regionAddress,
    //            &regionSize,
    //            FreeType
    //            );
    //    }
    //}

    return status;
}

NTSTATUS PhProtectVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T RegionSize,
    _In_ ULONG NewProtection,
    _Out_opt_ PULONG OldProtection
    )
{
    NTSTATUS status;
    PVOID regionAddress = BaseAddress;
    SIZE_T regionSize = RegionSize;
    ULONG oldProtection = 0;

    status = NtProtectVirtualMemory(
        ProcessHandle,
        &regionAddress,
        &regionSize,
        NewProtection,
        &oldProtection
        );

    if (NT_SUCCESS(status) && OldProtection)
    {
        *OldProtection = oldProtection;
        return STATUS_SUCCESS;
    }

    //if (status == STATUS_INVALID_PAGE_PROTECTION)
    //{
    //    if (RtlFlushSecureMemoryCache(regionAddress, regionSize))
    //    {
    //        status = NtProtectVirtualMemory(
    //            ProcessHandle,
    //            &regionAddress,
    //            &regionSize,
    //            NewProtection,
    //            &oldProtection
    //            );
    //
    //        if (NT_SUCCESS(status) && OldProtection)
    //        {
    //            *OldProtection = oldProtection;
    //            return STATUS_SUCCESS;
    //        }
    //    }
    //}

    return status;
}

/**
 * Reads virtual memory from a specified process.
 *
 * \param ProcessHandle Handle to the process from which the memory is to be read.
 * \param BaseAddress Optional pointer to the base address in the specified process from which to read.
 * \param Buffer Pointer to a buffer that receives the contents from the address space of the specified process.
 * \param BufferSize Size of the buffer, in bytes.
 * \param NumberOfBytesRead Optional pointer to a variable that receives the number of bytes read into the buffer.
 * \return Successful or errant status.
 */
NTSTATUS PhReadVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Out_opt_ PSIZE_T NumberOfBytesRead
    )
{
    NTSTATUS status;
    SIZE_T numberOfBytesRead;

    if (ProcessHandle == NtCurrentProcess())
    {
        RtlMoveMemory(Buffer, BaseAddress, BufferSize);
        if (NumberOfBytesRead)
            *NumberOfBytesRead = BufferSize;
        return STATUS_SUCCESS;
    }

    numberOfBytesRead = 0;
    status = NtReadVirtualMemory(
        ProcessHandle,
        BaseAddress,
        Buffer,
        BufferSize,
        &numberOfBytesRead
        );

    if (NT_SUCCESS(status))
    {
        assert(BufferSize == numberOfBytesRead);
    }

    if (NumberOfBytesRead)
    {
        *NumberOfBytesRead = numberOfBytesRead;
    }

    return status;
}

/**
 * Writes virtual memory to the specified process.
 *
 * \param ProcessHandle Handle to the process from which the memory is to be read.
 * \param BaseAddress Optional pointer to the base address in the specified process from which to read.
 * \param Buffer Pointer to a buffer that receives the contents from the address space of the specified process.
 * \param NumberOfBytesToWrite The number of bytes to be written to the specified process.
 * \param NumberOfBytesWritten A pointer to a variable that receives the number of bytes transferred into the specified buffer.
 * \return Successful or errant status.
 */
NTSTATUS PhWriteVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _In_reads_bytes_(NumberOfBytesToWrite) PVOID Buffer,
    _In_ SIZE_T NumberOfBytesToWrite,
    _Out_opt_ PSIZE_T NumberOfBytesWritten
    )
{
    NTSTATUS status;
    SIZE_T numberOfBytesWritten;

    numberOfBytesWritten = 0;
    status = NtWriteVirtualMemory(
        ProcessHandle,
        BaseAddress,
        Buffer,
        NumberOfBytesToWrite,
        &numberOfBytesWritten
        );

    if (NT_SUCCESS(status))
    {
        assert(NumberOfBytesToWrite == numberOfBytesWritten);
    }

    if (NumberOfBytesWritten)
    {
        *NumberOfBytesWritten = numberOfBytesWritten;
    }

    return status;
}

/**
 * Determines the length of the specified string, in characters.
 *
 * \param String The string.
 */
SIZE_T PhCountStringZ(
    _In_ PCWSTR String
    )
{
#ifndef _ARM64_
    if (PhHasAVX)
    {
        PWSTR p;
        ULONG unaligned;
        __m256i b;
        __m256i z;
        ULONG mask;
        ULONG index;

        p = (PWSTR)((ULONG_PTR)String & ~0x1e); // String should be 2 byte aligned
        unaligned = (ULONG_PTR)String & 0x1f;
        z = _mm256_setzero_si256();

        if (unaligned != 0)
        {
            b = _mm256_loadu_si256((__m256i const*)p);
            b = _mm256_cmpeq_epi16(b, z);
            mask = _mm256_movemask_epi8(b) >> unaligned;

            if (_BitScanForward(&index, mask))
                return index / sizeof(WCHAR);

            p += 32 / sizeof(WCHAR);
        }

        while (TRUE)
        {
            b = _mm256_load_si256((__m256i const*)p);
            b = _mm256_cmpeq_epi16(b, z);
            mask = _mm256_movemask_epi8(b);

            if (_BitScanForward(&index, mask))
                return (SIZE_T)(p - String) + index / sizeof(WCHAR);

            p += 32 / sizeof(WCHAR);
        }
    }
    else
#endif
    if (PhHasIntrinsics)
    {
        PWSTR p;
        ULONG unaligned;
        PH_INT128 b;
        PH_INT128 z;
        ULONG mask;
        ULONG index;

        p = (PWSTR)((ULONG_PTR)String & ~0xe); // String should be 2 byte aligned
        unaligned = (ULONG_PTR)String & 0xf;
        z = PhSetZeroINT128();

        if (unaligned != 0)
        {
            b = PhLoadINT128U((PLONG)p);
            b = PhCompareEqINT128by16(b, z);
            mask = PhMoveMaskINT128by8(b) >> unaligned;

            if (_BitScanForward(&index, mask))
                return index / sizeof(WCHAR);

            p += 16 / sizeof(WCHAR);
        }

        while (TRUE)
        {
            b = PhLoadINT128((PLONG)p);
            b = PhCompareEqINT128by16(b, z);
            mask = PhMoveMaskINT128by8(b);

            if (_BitScanForward(&index, mask))
                return (SIZE_T)(p - String) + index / sizeof(WCHAR);

            p += 16 / sizeof(WCHAR);
        }
    }
    else
    {
        return wcslen(String);
    }
}

/**
 * Allocates space for and copies a byte string.
 *
 * \param String The string to duplicate.
 *
 * \return The new string, which can be freed using PhFree().
 */
PSTR PhDuplicateBytesZ(
    _In_ PCSTR String
    )
{
    PSTR newString;
    SIZE_T length;

    length = strlen(String) + sizeof(ANSI_NULL); // include the null terminator

    newString = PhAllocate(length);
    memcpy(newString, String, length);

    return newString;
}

/**
 * Allocates space for and copies a byte string.
 *
 * \param String The string to duplicate.
 *
 * \return The new string, which can be freed using PhFree(), or NULL if storage could not be
 * allocated.
 */
PSTR PhDuplicateBytesZSafe(
    _In_ PCSTR String
    )
{
    PSTR newString;
    SIZE_T length;

    length = strlen(String) + sizeof(ANSI_NULL); // include the null terminator

    newString = PhAllocateSafe(length);

    if (!newString)
        return NULL;

    memcpy(newString, String, length);

    return newString;
}

/**
 * Allocates space for and copies a 16-bit string.
 *
 * \param String The string to duplicate.
 *
 * \return The new string, which can be freed using PhFree().
 */
PWSTR PhDuplicateStringZ(
    _In_ PCWSTR String
    )
{
    PWSTR newString;
    SIZE_T length;

    length = PhCountStringZ(String) * sizeof(WCHAR) + sizeof(UNICODE_NULL); // include the null terminator

    newString = PhAllocate(length);
    memcpy(newString, String, length);

    return newString;
}

/**
 * Copies a string with optional null termination and a maximum number of characters.
 *
 * \param InputBuffer A pointer to the input string.
 * \param InputCount The maximum number of characters to copy, not including the null terminator.
 * Specify -1 for no limit.
 * \param OutputBuffer A pointer to the output buffer.
 * \param OutputCount The number of characters in the output buffer, including the null terminator.
 * \param ReturnCount A variable which receives the number of characters required to contain the
 * input string, including the null terminator. If the function returns TRUE, this variable contains
 * the number of characters written to the output buffer.
 * \return TRUE if the input string was copied to the output string, otherwise FALSE.
 * \remarks The function stops copying when it encounters the first null character in the input
 * string. It will also stop copying when it reaches the character count specified in \a InputCount,
 * if \a InputCount is not -1.
 */
NTSTATUS PhCopyBytesZ(
    _In_ PCSTR InputBuffer,
    _In_ SIZE_T InputCount,
    _Out_writes_opt_z_(OutputCount) PSTR OutputBuffer,
    _In_ SIZE_T OutputCount,
    _Out_opt_ PSIZE_T ReturnCount
    )
{
    NTSTATUS status;
    SIZE_T i;

    // Determine the length of the input string.

    if (InputCount != SIZE_MAX)
    {
        i = 0;

        while (i < InputCount && InputBuffer[i])
            i++;
    }
    else
    {
        i = strlen(InputBuffer);
    }

    // Copy the string if there is enough room.

    if (OutputBuffer && OutputCount >= i + sizeof(ANSI_NULL)) // need one character for null terminator
    {
        memcpy(OutputBuffer, InputBuffer, i);
        OutputBuffer[i] = ANSI_NULL;
        status = STATUS_SUCCESS;
    }
    else
    {
        status = STATUS_BUFFER_TOO_SMALL;
    }

    if (ReturnCount)
    {
        *ReturnCount = i + sizeof(ANSI_NULL);
    }

    return status;
}

/**
 * Copies a string with optional null termination and a maximum number of characters.
 *
 * \param InputBuffer A pointer to the input string.
 * \param InputCount The maximum number of characters to copy, not including the null terminator.
 * Specify -1 for no limit.
 * \param OutputBuffer A pointer to the output buffer.
 * \param OutputCount The number of characters in the output buffer, including the null terminator.
 * \param ReturnCount A variable which receives the number of characters required to contain the
 * input string, including the null terminator. If the function returns TRUE, this variable contains
 * the number of characters written to the output buffer.
 * \return TRUE if the input string was copied to the output string, otherwise FALSE.
 * \remarks The function stops copying when it encounters the first null character in the input
 * string. It will also stop copying when it reaches the character count specified in \a InputCount,
 * if \a InputCount is not -1.
 */
NTSTATUS PhCopyStringZ(
    _In_ PCWSTR InputBuffer,
    _In_ SIZE_T InputCount,
    _Out_writes_opt_z_(OutputCount) PWSTR OutputBuffer,
    _In_ SIZE_T OutputCount,
    _Out_opt_ PSIZE_T ReturnCount
    )
{
    NTSTATUS status;
    SIZE_T i;

    // Determine the length of the input string.

    if (InputCount != SIZE_MAX)
    {
        i = 0;

        while (i < InputCount && InputBuffer[i])
            i++;
    }
    else
    {
        i = PhCountStringZ(InputBuffer);
    }

    // Copy the string if there is enough room.

    if (OutputBuffer && OutputCount >= i + 1) // need one character for null terminator
    {
        memcpy(OutputBuffer, InputBuffer, i * sizeof(WCHAR));
        OutputBuffer[i] = UNICODE_NULL;
        status = STATUS_SUCCESS;
    }
    else
    {
        status = STATUS_BUFFER_TOO_SMALL;
    }

    if (ReturnCount)
    {
        *ReturnCount = i + 1;
    }

    return status;
}

/**
 * Copies a string with optional null termination and a maximum number of characters.
 *
 * \param InputBuffer A pointer to the input string.
 * \param InputCount The maximum number of characters to copy, not including the null terminator.
 * Specify -1 for no limit.
 * \param OutputBuffer A pointer to the output buffer.
 * \param OutputCount The number of characters in the output buffer, including the null terminator.
 * \param ReturnCount A variable which receives the number of characters required to contain the
 * input string, including the null terminator. If the function returns TRUE, this variable contains
 * the number of characters written to the output buffer.
 * \return TRUE if the input string was copied to the output string, otherwise FALSE.
 * \remarks The function stops copying when it encounters the first null character in the input
 * string. It will also stop copying when it reaches the character count specified in \a InputCount,
 * if \a InputCount is not -1.
 */
NTSTATUS PhCopyStringZFromBytes(
    _In_ PCSTR InputBuffer,
    _In_ SIZE_T InputCount,
    _Out_writes_opt_z_(OutputCount) PWSTR OutputBuffer,
    _In_ SIZE_T OutputCount,
    _Out_opt_ PSIZE_T ReturnCount
    )
{
    NTSTATUS status;
    SIZE_T i;

    // Determine the length of the input string.

    if (InputCount != SIZE_MAX)
    {
        i = 0;

        while (i < InputCount && InputBuffer[i])
            i++;
    }
    else
    {
        i = strlen(InputBuffer);
    }

    // Copy the string if there is enough room.

    if (OutputBuffer && OutputCount >= i + 1) // need one character for null terminator
    {
        PhZeroExtendToUtf16Buffer(InputBuffer, i, OutputBuffer);
        OutputBuffer[i] = UNICODE_NULL;
        status = STATUS_SUCCESS;
    }
    else
    {
        status = STATUS_BUFFER_TOO_SMALL;
    }

    if (ReturnCount)
    {
        *ReturnCount = i + 1;
    }

    return status;
}

/**
 * Copies a string with optional null termination and a maximum number of characters.
 *
 * \param InputBuffer A pointer to the input string.
 * \param InputCount The maximum number of characters to copy, not including the null terminator.
 * Specify -1 for no limit.
 * \param OutputBuffer A pointer to the output buffer.
 * \param OutputCount The number of characters in the output buffer, including the null terminator.
 * \param ReturnCount A variable which receives the number of characters required to contain the
 * input string, including the null terminator. If the function returns TRUE, this variable contains
 * the number of characters written to the output buffer.
 * \return TRUE if the input string was copied to the output string, otherwise FALSE.
 * \remarks The function stops copying when it encounters the first null character in the input
 * string. It will also stop copying when it reaches the character count specified in \a InputCount,
 * if \a InputCount is not -1.
 */
NTSTATUS PhCopyStringZFromMultiByte(
    _In_ PCSTR InputBuffer,
    _In_ SIZE_T InputCount,
    _Out_writes_opt_z_(OutputCount) PWSTR OutputBuffer,
    _In_ SIZE_T OutputCount,
    _Out_opt_ PSIZE_T ReturnCount
    )
{
    NTSTATUS status;
    SIZE_T i;
    ULONG inputBytes;
    ULONG unicodeBytes;

    // Determine the length of the input string.

    if (InputCount != SIZE_MAX)
    {
        i = 0;

        while (i < InputCount && InputBuffer[i])
            i++;
    }
    else
    {
        i = strlen(InputBuffer);
    }

    status = RtlSizeTToULong(i, &inputBytes);

    if (!NT_SUCCESS(status))
    {
        if (ReturnCount)
            *ReturnCount = SIZE_MAX;

        return status;
    }

    // Determine the length of the output string.

    status = RtlMultiByteToUnicodeSize(
        &unicodeBytes,
        InputBuffer,
        inputBytes
        );

    if (!NT_SUCCESS(status))
    {
        if (ReturnCount)
            *ReturnCount = SIZE_MAX;

        return status;
    }

    // Convert the string to Unicode if there is enough room.

    if (OutputBuffer && OutputCount >= (unicodeBytes + sizeof(UNICODE_NULL)) / sizeof(WCHAR))
    {
        status = RtlMultiByteToUnicodeN(
            OutputBuffer,
            unicodeBytes,
            NULL,
            InputBuffer,
            inputBytes
            );

        if (NT_SUCCESS(status))
        {
            // RtlMultiByteToUnicodeN doesn't null terminate the string.
            *(PWCHAR)PTR_ADD_OFFSET(OutputBuffer, unicodeBytes) = UNICODE_NULL;
        }
    }
    else
    {
        status = STATUS_BUFFER_TOO_SMALL;
    }

    if (ReturnCount)
    {
        *ReturnCount = (unicodeBytes + sizeof(UNICODE_NULL)) / sizeof(WCHAR);
    }

    return status;
}

/**
 * Copies a UTF-8 encoded string to a wide-character (UTF-16) string buffer.
 *
 * \param InputBuffer Pointer to the input UTF-8 string buffer.
 * \param InputCount Size, in bytes, of the input buffer.
 * \param OutputBuffer Pointer to the output wide-character string buffer. Can be NULL if OutputCount is 0.
 * \param OutputCount Size, in characters, of the output buffer.
 * \param ReturnCount Optional pointer to receive the number of characters written to the output buffer (excluding the null terminator).
 * \return NTSTATUS Successful or errant status.
 * \remarks The output buffer will be null-terminated if OutputCount is greater than zero.
 */
NTSTATUS PhCopyStringZFromUtf8(
    _In_ PCSTR InputBuffer,
    _In_ SIZE_T InputCount,
    _Out_writes_opt_z_(OutputCount) PWSTR OutputBuffer,
    _In_ SIZE_T OutputCount,
    _Out_opt_ PSIZE_T ReturnCount
    )
{
    NTSTATUS status;
    SIZE_T i;
    ULONG inputBytes;
    ULONG unicodeBytes;

    // Determine the length of the input string.

    if (InputCount != SIZE_MAX)
    {
        i = 0;

        while (i < InputCount && InputBuffer[i])
            i++;
    }
    else
    {
        i = strlen(InputBuffer);
    }

    status = RtlSizeTToULong(i, &inputBytes);

    if (!NT_SUCCESS(status))
    {
        if (ReturnCount)
            *ReturnCount = SIZE_MAX;

        return status;
    }

    // Determine the length of the output string.

    status = RtlUTF8ToUnicodeN(
        NULL,
        0,
        &unicodeBytes,
        InputBuffer,
        inputBytes
        );

    if (!NT_SUCCESS(status))
    {
        if (ReturnCount)
            *ReturnCount = SIZE_MAX;

        return status;
    }

    // Convert the string to Unicode if there is enough room.

    if (OutputBuffer && OutputCount >= (unicodeBytes + sizeof(UNICODE_NULL)) / sizeof(WCHAR))
    {
        status = RtlUTF8ToUnicodeN(
            OutputBuffer,
            unicodeBytes,
            NULL,
            InputBuffer,
            inputBytes
            );

        if (NT_SUCCESS(status))
        {
            // RtlUTF8ToUnicodeN doesn't null terminate the string.
            *(PWCHAR)PTR_ADD_OFFSET(OutputBuffer, unicodeBytes) = UNICODE_NULL;
        }
    }
    else
    {
        status = STATUS_BUFFER_TOO_SMALL;
    }

    if (ReturnCount)
    {
        *ReturnCount = (unicodeBytes + sizeof(UNICODE_NULL)) / sizeof(WCHAR);
    }

    return status;
}

FORCEINLINE LONG PhpCompareRightNatural(
    _In_ PCWSTR A,
    _In_ PCWSTR B
    )
{
    LONG bias = 0;

    for (; ; A++, B++)
    {
        if (!PhIsDigitCharacter(*A) && !PhIsDigitCharacter(*B))
        {
            return bias;
        }
        else if (!PhIsDigitCharacter(*A))
        {
            return -1;
        }
        else if (!PhIsDigitCharacter(*B))
        {
            return 1;
        }
        else if (*A < *B)
        {
            if (bias == 0)
                bias = -1;
        }
        else if (*A > *B)
        {
            if (bias == 0)
                bias = 1;
        }
        else if (!*A && !*B)
        {
            return bias;
        }
    }

    return 0;
}

FORCEINLINE LONG PhpCompareLeftNatural(
    _In_ PCWSTR A,
    _In_ PCWSTR B
    )
{
    for (; ; A++, B++)
    {
        if (!PhIsDigitCharacter(*A) && !PhIsDigitCharacter(*B))
        {
            return 0;
        }
        else if (!PhIsDigitCharacter(*A))
        {
            return -1;
        }
        else if (!PhIsDigitCharacter(*B))
        {
            return 1;
        }
        else if (*A < *B)
        {
            return -1;
        }
        else if (*A > *B)
        {
            return 1;
        }
    }

    return 0;
}

FORCEINLINE LONG PhpCompareStringZNatural(
    _In_ PCWSTR A,
    _In_ PCWSTR B,
    _In_ BOOLEAN IgnoreCase
    )
{
    /*  strnatcmp.c -- Perform 'natural order' comparisons of strings in C.
        Copyright (C) 2000, 2004 by Martin Pool <mbp sourcefrog net>

        This software is provided 'as-is', without any express or implied
        warranty.  In no event will the authors be held liable for any damages
        arising from the use of this software.

        Permission is granted to anyone to use this software for any purpose,
        including commercial applications, and to alter it and redistribute it
        freely, subject to the following restrictions:

        1. The origin of this software must not be misrepresented; you must not
         claim that you wrote the original software. If you use this software
         in a product, an acknowledgment in the product documentation would be
         appreciated but is not required.
        2. Altered source versions must be plainly marked as such, and must not be
         misrepresented as being the original software.
        3. This notice may not be removed or altered from any source distribution.

        This code has been modified for System Informer.
     */

    ULONG ai, bi;
    WCHAR ca, cb;
    LONG result;
    BOOLEAN fractional;

    ai = 0;
    bi = 0;

    while (TRUE)
    {
        ca = A[ai];
        cb = B[bi];

        /* Skip over leading spaces or zeros. */

        while (ca == ' ')
            ca = A[++ai];

        while (cb == ' ')
            cb = B[++bi];

        /* Process run of digits. */
        if (PhIsDigitCharacter(ca) && PhIsDigitCharacter(cb))
        {
            fractional = (ca == '0' || cb == '0');

            if (fractional)
            {
                if ((result = PhpCompareLeftNatural(A + ai, B + bi)) != 0)
                    return result;
            }
            else
            {
                if ((result = PhpCompareRightNatural(A + ai, B + bi)) != 0)
                    return result;
            }
        }

        if (!ca && !cb)
        {
            /* Strings are considered the same. */
            return 0;
        }

        if (IgnoreCase)
        {
            ca = PhUpcaseUnicodeChar(ca);
            cb = PhUpcaseUnicodeChar(cb);
        }

        if (ca < cb)
            return -1;
        else if (ca > cb)
            return 1;

        ai++;
        bi++;
    }
}

/**
 * Compares two strings in natural sort order.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param IgnoreCase TRUE to perform a case-insensitive comparison, otherwise FALSE.
 * \return A value less than zero if \a String1 is less than \a String2, a value greater than zero if
 * \a String1 is greater than \a String2, or zero if the strings are equal.
 */
LONG PhCompareStringZNatural(
    _In_ PCWSTR String1,
    _In_ PCWSTR String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    return PhpCompareStringZNatural(String1, String2, IgnoreCase);
}

/**
 * Compares two strings.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param IgnoreCase TRUE to perform a case-insensitive comparison, otherwise FALSE.
 * \return A value less than zero if \a String1 is less than \a String2, a value greater than zero if
 * \a String1 is greater than \a String2, or zero if the strings are equal.
 */
LONG PhCompareStringRef(
    _In_ PCPH_STRINGREF String1,
    _In_ PCPH_STRINGREF String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    SIZE_T l1;
    SIZE_T l2;
    PWCHAR s1;
    PWCHAR s2;
    WCHAR c1;
    WCHAR c2;
    PWCHAR end;

    // Note: this function assumes that the difference between the lengths of the two strings can
    // fit inside a LONG.

    l1 = String1->Length;
    l2 = String2->Length;
    assert(!(l1 & 1));
    assert(!(l2 & 1));
    s1 = String1->Buffer;
    s2 = String2->Buffer;

    end = PTR_ADD_OFFSET(s1, l1 <= l2 ? l1 : l2);

    if (!IgnoreCase)
    {
        while (s1 != end)
        {
            c1 = *s1;
            c2 = *s2;

            if (c1 != c2)
                return (LONG)c1 - (LONG)c2;

            s1++;
            s2++;
        }
    }
    else
    {
        while (s1 != end)
        {
            c1 = *s1;
            c2 = *s2;

            if (c1 != c2)
            {
                c1 = PhUpcaseUnicodeChar(c1);
                c2 = PhUpcaseUnicodeChar(c2);

                if (c1 != c2)
                    return (LONG)c1 - (LONG)c2;
            }

            s1++;
            s2++;
        }
    }

    return (LONG)(l1 - l2);
}

/**
 * Determines if two strings are equal.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param IgnoreCase TRUE to perform a case-insensitive comparison, otherwise FALSE.
 * \return TRUE if the strings are equal, otherwise FALSE
 */
BOOLEAN PhEqualStringRef(
    _In_ PCPH_STRINGREF String1,
    _In_ PCPH_STRINGREF String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    SIZE_T l1;
    SIZE_T l2;
    PWSTR s1;
    PWSTR s2;
    WCHAR c1;
    WCHAR c2;
    SIZE_T length;

    l1 = String1->Length;
    l2 = String2->Length;
    assert(!(l1 & 1));
    assert(!(l2 & 1));

    if (l1 != l2)
        return FALSE;

    s1 = String1->Buffer;
    s2 = String2->Buffer;

    if (PhHasIntrinsics)
    {
        if (IgnoreCase)
        {
            length = l1 / 16;

            if (length != 0)
            {
                do
                {
                    PH_INT128 b1 = PhLoadINT128U((PLONG)s1);
                    PH_INT128 b2 = PhLoadINT128U((PLONG)s2);

                    // SIMD uppercase conversion
                    b1 = PhUppercaseASCIIINT128by16(b1);
                    b2 = PhUppercaseASCIIINT128by16(b2);

                    // Compare uppercased values
                    PH_INT128 cmp = PhCompareEqINT128by16(b1, b2);
                    if (PhMoveMaskINT128by8(cmp) != 0xffff)
                    {
                        // Mismatch - fall back to scalar
                        l1 = length * 16 + (l1 & 15);
                        l1 /= sizeof(WCHAR);
                        goto CompareCharacters;
                    }

                    s1 += 16 / sizeof(WCHAR);
                    s2 += 16 / sizeof(WCHAR);
                } while (--length != 0);
            }

            // Compare character-by-character because we have no more 16-byte blocks to compare.
            l1 = (l1 & 15) / sizeof(WCHAR);
        }
        else
        {
            length = l1 / 16;

            if (length != 0)
            {
                PH_INT128 b1;
                PH_INT128 b2;

                do
                {
                    b1 = PhLoadINT128U((PLONG)s1);
                    b2 = PhLoadINT128U((PLONG)s2);
                    b1 = PhCompareEqINT128by32(b1, b2);

                    if (PhMoveMaskINT128by8(b1) != 0xffff)
                    {
                        if (!IgnoreCase)
                        {
                            return FALSE;
                        }
                        else
                        {
                            // Compare character-by-character to ignore case.
                            l1 = length * 16 + (l1 & 15);
                            l1 /= sizeof(WCHAR);
                            goto CompareCharacters;
                        }
                    }

                    s1 += 16 / sizeof(WCHAR);
                    s2 += 16 / sizeof(WCHAR);
                } while (--length != 0);
            }

            // Compare character-by-character because we have no more 16-byte blocks to compare.
            l1 = (l1 & 15) / sizeof(WCHAR);
        }
    }
    else
    {
        length = l1 / sizeof(ULONG_PTR);

        if (length != 0)
        {
            do
            {
                if (*(PULONG_PTR)s1 != *(PULONG_PTR)s2)
                {
                    if (!IgnoreCase)
                    {
                        return FALSE;
                    }
                    else
                    {
                        // Compare character-by-character to ignore case.
                        l1 = length * sizeof(ULONG_PTR) + (l1 & (sizeof(ULONG_PTR) - 1));
                        l1 /= sizeof(WCHAR);
                        goto CompareCharacters;
                    }
                }

                s1 += sizeof(ULONG_PTR) / sizeof(WCHAR);
                s2 += sizeof(ULONG_PTR) / sizeof(WCHAR);
            } while (--length != 0);
        }

        // Compare character-by-character because we have no more ULONG_PTR blocks to compare.
        l1 = (l1 & (sizeof(ULONG_PTR) - 1)) / sizeof(WCHAR);
    }

CompareCharacters:
    if (l1 != 0)
    {
        if (!IgnoreCase)
        {
            do
            {
                c1 = *s1;
                c2 = *s2;

                if (c1 != c2)
                    return FALSE;

                s1++;
                s2++;
            } while (--l1 != 0);
        }
        else
        {
            do
            {
                c1 = *s1;
                c2 = *s2;

                if (c1 != c2)
                {
                    c1 = PhUpcaseUnicodeChar(c1);
                    c2 = PhUpcaseUnicodeChar(c2);

                    if (c1 != c2)
                        return FALSE;
                }

                s1++;
                s2++;
            } while (--l1 != 0);
        }
    }

    return TRUE;
}

/**
 * Locates a character in a string.
 *
 * \param String The string to search.
 * \param Character The character to search for.
 * \param IgnoreCase TRUE to perform a case-insensitive search, otherwise FALSE.
 * \return The index, in characters, of the first occurrence of \a Character in \a String1. If
 * \a Character was not found, -1 is returned.
 */
ULONG_PTR PhFindCharInStringRef(
    _In_ PCPH_STRINGREF String,
    _In_ WCHAR Character,
    _In_ BOOLEAN IgnoreCase
    )
{
    PWSTR buffer;
    SIZE_T length;

    buffer = String->Buffer;
    length = String->Length / sizeof(WCHAR);

    if (!IgnoreCase)
    {
        if (PhHasIntrinsics)
        {
            SIZE_T length16;

            length16 = String->Length / 16;
            length &= 7;

            if (length16 != 0)
            {
                PH_INT128 pattern;
                PH_INT128 block;
                ULONG mask;
                ULONG index;

                pattern = PhSetINT128by16(Character);

                do
                {
                    block = PhLoadINT128U((PLONG)buffer);
                    block = PhCompareEqINT128by16(block, pattern);
                    mask = PhMoveMaskINT128by8(block);

                    if (_BitScanForward(&index, mask))
                        return (String->Length - length16 * 16) / sizeof(WCHAR) - length + index / 2;

                    buffer += 16 / sizeof(WCHAR);
                } while (--length16 != 0);
            }
        }
#if defined (PHLIB_WCSCHR)
        else
        {
            if (buffer)
            {
                PWSTR offset;

                if (offset = wcschr(buffer, Character))
                    return offset - buffer;

                return SIZE_MAX;
            }
        }
#endif

        if (length != 0)
        {
            do
            {
                if (*buffer == Character)
                    return String->Length / sizeof(WCHAR) - length;

                buffer++;
            } while (--length != 0);
        }
    }
    else
    {
        if (length != 0)
        {
            WCHAR c;

            c = PhUpcaseUnicodeChar(Character);

            do
            {
                if (PhUpcaseUnicodeChar(*buffer) == c)
                    return String->Length / sizeof(WCHAR) - length;

                buffer++;
            } while (--length != 0);
        }
    }

    return SIZE_MAX;
}

/**
 * Locates a character in a string, searching backwards.
 *
 * \param String The string to search.
 * \param Character The character to search for.
 * \param IgnoreCase TRUE to perform a case-insensitive search, otherwise FALSE.
 * \return The index, in characters, of the last occurrence of \a Character in \a String1. If
 * \a Character was not found, -1 is returned.
 */
ULONG_PTR PhFindLastCharInStringRef(
    _In_ PCPH_STRINGREF String,
    _In_ WCHAR Character,
    _In_ BOOLEAN IgnoreCase
    )
{
    PWCHAR buffer;
    SIZE_T length;

    buffer = PTR_ADD_OFFSET(String->Buffer, String->Length);
    length = String->Length / sizeof(WCHAR);

    if (!IgnoreCase)
    {
        if (PhHasIntrinsics)
        {
            SIZE_T length16;

            length16 = String->Length / 16;
            length &= 7;

            if (length16 != 0)
            {
                PH_INT128 pattern;
                PH_INT128 block;
                ULONG mask;
                ULONG index;

                pattern = PhSetINT128by16(Character);
                buffer -= 16 / sizeof(WCHAR);

                do
                {
                    block = PhLoadINT128U((PLONG)buffer);
                    block = PhCompareEqINT128by16(block, pattern);
                    mask = PhMoveMaskINT128by8(block);

                    if (_BitScanReverse(&index, mask))
                        return (length16 - 1) * 16 / sizeof(WCHAR) + length + index / 2;

                    buffer -= 16 / sizeof(WCHAR);
                } while (--length16 != 0);

                buffer += 16 / sizeof(WCHAR);
            }
        }
#if defined (PHLIB_WCSCHR)
        else
        {
            if (buffer)
            {
                PWSTR offset;

                buffer = String->Buffer;

                if (offset = wcsrchr(buffer, Character))
                    return offset - buffer;

                return SIZE_MAX;
            }
        }
#endif

        if (length != 0)
        {
            buffer--;

            do
            {
                if (*buffer == Character)
                    return length - 1;

                buffer--;
            } while (--length != 0);
        }
    }
    else
    {
        if (length != 0)
        {
            WCHAR c;

            c = PhUpcaseUnicodeChar(Character);
            buffer--;

            do
            {
                if (PhUpcaseUnicodeChar(*buffer) == c)
                    return length - 1;

                buffer--;
            } while (--length != 0);
        }
    }

    return SIZE_MAX;
}

/**
 * Locates a string in a string.
 *
 * \param String The string to search.
 * \param SubString The string to search for.
 * \param IgnoreCase TRUE to perform a case-insensitive search, otherwise FALSE.
 * \return The index, in characters, of the first occurrence of \a SubString in \a String. If
 * \a SubString was not found, -1 is returned.
 */
ULONG_PTR PhFindStringInStringRef(
    _In_ PCPH_STRINGREF String,
    _In_ PCPH_STRINGREF SubString,
    _In_ BOOLEAN IgnoreCase
    )
{
    SIZE_T length1;
    SIZE_T length2;
    PH_STRINGREF sr1;
    PH_STRINGREF sr2;
    WCHAR c;
    SIZE_T i;

    length1 = String->Length / sizeof(WCHAR);
    length2 = SubString->Length / sizeof(WCHAR);

    // Can't be a substring if it's bigger than the first string.
    if (length2 > length1)
        return SIZE_MAX;
    // We always get a match if the substring is zero-length.
    if (length2 == 0)
        return 0;

    sr1.Buffer = String->Buffer;
    sr1.Length = SubString->Length - sizeof(WCHAR);
    sr2.Buffer = SubString->Buffer;
    sr2.Length = SubString->Length - sizeof(WCHAR);

    if (!IgnoreCase)
    {
        c = *sr2.Buffer++;

        for (i = length1 - length2 + 1; i != 0; i--)
        {
            if (*sr1.Buffer++ == c && PhEqualStringRef(&sr1, &sr2, FALSE))
            {
                goto FoundUString;
            }
        }
    }
    else
    {
        c = PhUpcaseUnicodeChar(*sr2.Buffer++);

        for (i = length1 - length2 + 1; i != 0; i--)
        {
            if (PhUpcaseUnicodeChar(*sr1.Buffer++) == c && PhEqualStringRef(&sr1, &sr2, TRUE))
            {
                goto FoundUString;
            }
        }
    }

    return SIZE_MAX;
FoundUString:
    return (ULONG_PTR)(sr1.Buffer - String->Buffer - 1);
}

/**
 * Splits a string.
 *
 * \param Input The input string.
 * \param Separator The character to split at.
 * \param FirstPart A variable which receives the part of \a Input before the separator. This may be
 * the same variable as \a Input. If the separator is not found in \a Input, this variable is set to
 * \a Input.
 * \param SecondPart A variable which receives the part of \a Input after the separator. This may be
 * the same variable as \a Input. If the separator is not found in \a Input, this variable is set to
 * an empty string.
 * \return TRUE if \a Separator was found in \a Input, otherwise FALSE.
 */
BOOLEAN PhSplitStringRefAtChar(
    _In_ PCPH_STRINGREF Input,
    _In_ WCHAR Separator,
    _Out_ PPH_STRINGREF FirstPart,
    _Out_ PPH_STRINGREF SecondPart
    )
{
    PH_STRINGREF input;
    ULONG_PTR index;

    input = *Input; // get a copy of the input because FirstPart/SecondPart may alias Input
    index = PhFindCharInStringRef(Input, Separator, FALSE);

    if (index == SIZE_MAX)
    {
        // The separator was not found.

        FirstPart->Buffer = Input->Buffer;
        FirstPart->Length = Input->Length;
        SecondPart->Buffer = NULL;
        SecondPart->Length = 0;

        return FALSE;
    }

    FirstPart->Buffer = input.Buffer;
    FirstPart->Length = index * sizeof(WCHAR);
    SecondPart->Buffer = PTR_ADD_OFFSET(input.Buffer, index * sizeof(WCHAR) + sizeof(UNICODE_NULL));
    SecondPart->Length = input.Length - index * sizeof(WCHAR) - sizeof(UNICODE_NULL);

    return TRUE;
}

/**
 * Splits a string at the last occurrence of a character.
 *
 * \param Input The input string.
 * \param Separator The character to split at.
 * \param FirstPart A variable which receives the part of \a Input before the separator. This may be
 * the same variable as \a Input. If the separator is not found in \a Input, this variable is set to
 * \a Input.
 * \param SecondPart A variable which receives the part of \a Input after the separator. This may be
 * the same variable as \a Input. If the separator is not found in \a Input, this variable is set to
 * an empty string.
 * \return TRUE if \a Separator was found in \a Input, otherwise FALSE.
 */
BOOLEAN PhSplitStringRefAtLastChar(
    _In_ PCPH_STRINGREF Input,
    _In_ WCHAR Separator,
    _Out_ PPH_STRINGREF FirstPart,
    _Out_ PPH_STRINGREF SecondPart
    )
{
    PH_STRINGREF input;
    ULONG_PTR index;

    input = *Input; // get a copy of the input because FirstPart/SecondPart may alias Input
    index = PhFindLastCharInStringRef(Input, Separator, FALSE);

    if (index == SIZE_MAX)
    {
        // The separator was not found.

        FirstPart->Buffer = Input->Buffer;
        FirstPart->Length = Input->Length;
        SecondPart->Buffer = NULL;
        SecondPart->Length = 0;

        return FALSE;
    }

    FirstPart->Buffer = input.Buffer;
    FirstPart->Length = index * sizeof(WCHAR);
    SecondPart->Buffer = PTR_ADD_OFFSET(input.Buffer, index * sizeof(WCHAR) + sizeof(UNICODE_NULL));
    SecondPart->Length = input.Length - index * sizeof(WCHAR) - sizeof(UNICODE_NULL);

    return TRUE;
}

/**
 * Splits a string.
 *
 * \param Input The input string.
 * \param Separator The string to split at.
 * \param IgnoreCase TRUE to perform a case-insensitive search, otherwise FALSE.
 * \param FirstPart A variable which receives the part of \a Input before the separator. This may be
 * the same variable as \a Input. If the separator is not found in \a Input, this variable is set to
 * \a Input.
 * \param SecondPart A variable which receives the part of \a Input after the separator. This may be
 * the same variable as \a Input. If the separator is not found in \a Input, this variable is set to
 * an empty string.
 * \return TRUE if \a Separator was found in \a Input, otherwise FALSE.
 */
BOOLEAN PhSplitStringRefAtString(
    _In_ PCPH_STRINGREF Input,
    _In_ PCPH_STRINGREF Separator,
    _In_ BOOLEAN IgnoreCase,
    _Out_ PPH_STRINGREF FirstPart,
    _Out_ PPH_STRINGREF SecondPart
    )
{
    PH_STRINGREF input;
    ULONG_PTR index;

    input = *Input; // get a copy of the input because FirstPart/SecondPart may alias Input
    index = PhFindStringInStringRef(Input, Separator, IgnoreCase);

    if (index == SIZE_MAX)
    {
        // The separator was not found.

        FirstPart->Buffer = Input->Buffer;
        FirstPart->Length = Input->Length;
        SecondPart->Buffer = NULL;
        SecondPart->Length = 0;

        return FALSE;
    }

    FirstPart->Buffer = input.Buffer;
    FirstPart->Length = index * sizeof(WCHAR);
    SecondPart->Buffer = PTR_ADD_OFFSET(input.Buffer, index * sizeof(WCHAR) + Separator->Length);
    SecondPart->Length = input.Length - index * sizeof(WCHAR) - Separator->Length;

    return TRUE;
}

/**
 * Splits a string.
 *
 * \param Input The input string.
 * \param Separator The character set, string or range to split at.
 * \param Flags A combination of flags.
 * \li \c PH_SPLIT_AT_CHAR_SET \a Separator specifies a character set. \a Input will be split at a
 * character which is contained in \a Separator.
 * \li \c PH_SPLIT_AT_STRING \a Separator specifies a string. \a Input will be split an occurrence
 * of \a Separator.
 * \li \c PH_SPLIT_AT_RANGE \a Separator specifies a range. The \a Buffer field contains a character
 * index cast to \c PWSTR and the \a Length field contains the length of the range, in bytes.
 * \li \c PH_SPLIT_CASE_INSENSITIVE Specifies a case-insensitive search.
 * \li \c PH_SPLIT_COMPLEMENT_CHAR_SET If used with \c PH_SPLIT_AT_CHAR_SET, the separator is a
 * character which is not contained in \a Separator.
 * \li \c PH_SPLIT_START_AT_END If used with \c PH_SPLIT_AT_CHAR_SET, the search is performed
 * starting from the end of the string.
 * \li \c PH_SPLIT_CHAR_SET_IS_UPPERCASE If used with \c PH_SPLIT_CASE_INSENSITIVE, specifies that
 * the character set in \a Separator contains only uppercase characters.
 * \param FirstPart A variable which receives the part of \a Input before the separator. This may be
 * the same variable as \a Input. If the separator is not found in \a Input, this variable is set to
 * \a Input.
 * \param SecondPart A variable which receives the part of \a Input after the separator. This may be
 * the same variable as \a Input. If the separator is not found in \a Input, this variable is set to
 * an empty string.
 * \param SeparatorPart A variable which receives the part of \a Input that is the separator. If the
 * separator is not found in \a Input, this variable is set to an empty string.
 * \return TRUE if a separator was found in \a Input, otherwise FALSE.
 */
BOOLEAN PhSplitStringRefEx(
    _In_ PCPH_STRINGREF Input,
    _In_ PCPH_STRINGREF Separator,
    _In_ ULONG Flags,
    _Out_ PPH_STRINGREF FirstPart,
    _Out_ PPH_STRINGREF SecondPart,
    _Out_opt_ PPH_STRINGREF SeparatorPart
    )
{
    PH_STRINGREF input;
    SIZE_T separatorIndex;
    SIZE_T separatorLength;
    PWCHAR charSet;
    SIZE_T charSetCount;
    BOOLEAN charSetTable[256];
    BOOLEAN charSetTableComplete;
    SIZE_T i;
    SIZE_T j;
    USHORT c;
    PWCHAR s;
    LONG_PTR direction;

    input = *Input; // Get a copy of the input because FirstPart/SecondPart/SeparatorPart may alias Input

    if (Flags & PH_SPLIT_AT_RANGE)
    {
        separatorIndex = (SIZE_T)Separator->Buffer;
        separatorLength = Separator->Length;

        if (separatorIndex == SIZE_MAX)
            goto SeparatorNotFound;

        goto SeparatorFound;
    }
    else if (Flags & PH_SPLIT_AT_STRING)
    {
        if (Flags & PH_SPLIT_START_AT_END)
        {
            // not implemented
            goto SeparatorNotFound;
        }

        separatorIndex = PhFindStringInStringRef(Input, Separator, !!(Flags & PH_SPLIT_CASE_INSENSITIVE));

        if (separatorIndex == SIZE_MAX)
            goto SeparatorNotFound;

        separatorLength = Separator->Length;
        goto SeparatorFound;
    }

    // Special case for character sets with only one character.
    if (!(Flags & PH_SPLIT_COMPLEMENT_CHAR_SET) && Separator->Length == sizeof(WCHAR))
    {
        if (!(Flags & PH_SPLIT_START_AT_END))
            separatorIndex = PhFindCharInStringRef(Input, Separator->Buffer[0], !!(Flags & PH_SPLIT_CASE_INSENSITIVE));
        else
            separatorIndex = PhFindLastCharInStringRef(Input, Separator->Buffer[0], !!(Flags & PH_SPLIT_CASE_INSENSITIVE));

        if (separatorIndex == SIZE_MAX)
            goto SeparatorNotFound;

        separatorLength = sizeof(WCHAR);
        goto SeparatorFound;
    }

    if (input.Length == 0)
        goto SeparatorNotFound;

    // Build the character set lookup table.

    charSet = Separator->Buffer;
    charSetCount = Separator->Length / sizeof(WCHAR);
    memset(charSetTable, 0, sizeof(charSetTable));
    charSetTableComplete = TRUE;

    for (i = 0; i < charSetCount; i++)
    {
        c = charSet[i];

        if (Flags & PH_SPLIT_CASE_INSENSITIVE)
            c = PhUpcaseUnicodeChar(c);

        charSetTable[c & 0xff] = TRUE;

        if (c >= 256)
            charSetTableComplete = FALSE;
    }

    // Perform the search.

    i = input.Length / sizeof(WCHAR);
    separatorLength = sizeof(WCHAR);

    if (!(Flags & PH_SPLIT_START_AT_END))
    {
        s = input.Buffer;
        direction = 1;
    }
    else
    {
        s = PTR_ADD_OFFSET(input.Buffer, input.Length - sizeof(WCHAR));
        direction = -1;
    }

    do
    {
        c = *s;

        if (Flags & PH_SPLIT_CASE_INSENSITIVE)
            c = PhUpcaseUnicodeChar(c);

        if (c < 256 && charSetTableComplete)
        {
            if (!(Flags & PH_SPLIT_COMPLEMENT_CHAR_SET))
            {
                if (charSetTable[c])
                    goto CharFound;
            }
            else
            {
                if (!charSetTable[c])
                    goto CharFound;
            }
        }
        else
        {
            if (!(Flags & PH_SPLIT_COMPLEMENT_CHAR_SET))
            {
                if (charSetTable[c & 0xff])
                {
                    if (!(Flags & PH_SPLIT_CASE_INSENSITIVE) || (Flags & PH_SPLIT_CHAR_SET_IS_UPPERCASE))
                    {
                        for (j = 0; j < charSetCount; j++)
                        {
                            if (charSet[j] == c)
                                goto CharFound;
                        }
                    }
                    else
                    {
                        for (j = 0; j < charSetCount; j++)
                        {
                            if (PhUpcaseUnicodeChar(charSet[j]) == c)
                                goto CharFound;
                        }
                    }
                }
            }
            else
            {
                if (charSetTable[c & 0xff])
                {
                    if (!(Flags & PH_SPLIT_CASE_INSENSITIVE) || (Flags & PH_SPLIT_CHAR_SET_IS_UPPERCASE))
                    {
                        for (j = 0; j < charSetCount; j++)
                        {
                            if (charSet[j] == c)
                                break;
                        }
                    }
                    else
                    {
                        for (j = 0; j < charSetCount; j++)
                        {
                            if (PhUpcaseUnicodeChar(charSet[j]) == c)
                                break;
                        }
                    }

                    if (j == charSetCount)
                        goto CharFound;
                }
                else
                {
                    goto CharFound;
                }
            }
        }

        s += direction;
    } while (--i != 0);

    goto SeparatorNotFound;

CharFound:
    separatorIndex = s - input.Buffer;

SeparatorFound:
    FirstPart->Buffer = input.Buffer;
    FirstPart->Length = separatorIndex * sizeof(WCHAR);
    SecondPart->Buffer = PTR_ADD_OFFSET(input.Buffer, separatorIndex * sizeof(WCHAR) + separatorLength);
    SecondPart->Length = input.Length - separatorIndex * sizeof(WCHAR) - separatorLength;

    if (SeparatorPart)
    {
        SeparatorPart->Buffer = input.Buffer + separatorIndex;
        SeparatorPart->Length = separatorLength;
    }

    return TRUE;

SeparatorNotFound:
    FirstPart->Buffer = input.Buffer;
    FirstPart->Length = input.Length;
    SecondPart->Buffer = NULL;
    SecondPart->Length = 0;

    if (SeparatorPart)
    {
        SeparatorPart->Buffer = NULL;
        SeparatorPart->Length = 0;
    }

    return FALSE;
}

/**
 * Trims characters from the beginning and/or end of a string reference.
 *
 * \param String Pointer to a PH_STRINGREF structure representing the string to be trimmed. The string is modified in place.
 * \param CharSet Pointer to a PH_STRINGREF structure containing the set of characters to trim from the string.
 * \param Flags Specifies trimming options. Can be used to indicate whether to trim from the left, right, or both ends.
 * \remarks This function removes all characters specified in CharSet from the start and/or end of the string referenced by String,
 * depending on the Flags provided.
 */
VOID PhTrimStringRef(
    _Inout_ PPH_STRINGREF String,
    _In_ PCPH_STRINGREF CharSet,
    _In_ ULONG Flags
    )
{
    PWCHAR charSet;
    SIZE_T charSetCount;
    BOOLEAN charSetTable[256];
    BOOLEAN charSetTableComplete;
    SIZE_T i;
    SIZE_T j;
    USHORT c;
    SIZE_T trimCount;
    SIZE_T count;
    PWCHAR s;

    if (!String->Buffer || String->Length == 0 || CharSet->Length == 0)
        return;

    if (CharSet->Length == sizeof(WCHAR))
    {
        c = CharSet->Buffer[0];

        if (!(Flags & PH_TRIM_END_ONLY))
        {
            trimCount = 0;
            count = String->Length / sizeof(WCHAR);
            s = String->Buffer;

            while (count-- != 0)
            {
                if (*s++ != c)
                    break;

                trimCount++;
            }

            PhSkipStringRef(String, trimCount * sizeof(WCHAR));
        }

        if (!(Flags & PH_TRIM_START_ONLY))
        {
            trimCount = 0;
            count = String->Length / sizeof(WCHAR);
            s = PTR_ADD_OFFSET(String->Buffer, String->Length - sizeof(WCHAR));

            while (count-- != 0)
            {
                if (*s-- != c)
                    break;

                trimCount++;
            }

            String->Length -= trimCount * sizeof(WCHAR);
        }

        return;
    }

    // Build the character set lookup table.

    charSet = CharSet->Buffer;
    charSetCount = CharSet->Length / sizeof(WCHAR);
    memset(charSetTable, 0, sizeof(charSetTable));
    charSetTableComplete = TRUE;

    for (i = 0; i < charSetCount; i++)
    {
        c = charSet[i];
        charSetTable[c & 0xff] = TRUE;

        if (c >= 256)
            charSetTableComplete = FALSE;
    }

    // Trim the string.

    if (!(Flags & PH_TRIM_END_ONLY))
    {
        trimCount = 0;
        count = String->Length / sizeof(WCHAR);
        s = String->Buffer;

        while (count-- != 0)
        {
            c = *s++;

            if (!charSetTable[c & 0xff])
                break;

            if (!charSetTableComplete)
            {
                for (j = 0; j < charSetCount; j++)
                {
                    if (charSet[j] == c)
                        goto CharFound;
                }

                break;
            }

CharFound:
            trimCount++;
        }

        PhSkipStringRef(String, trimCount * sizeof(WCHAR));
    }

    if (!(Flags & PH_TRIM_START_ONLY))
    {
        trimCount = 0;
        count = String->Length / sizeof(WCHAR);
        s = PTR_ADD_OFFSET(String->Buffer, String->Length - sizeof(WCHAR));

        while (count-- != 0)
        {
            c = *s--;

            if (!charSetTable[c & 0xff])
                break;

            if (!charSetTableComplete)
            {
                for (j = 0; j < charSetCount; j++)
                {
                    if (charSet[j] == c)
                        goto CharFound2;
                }

                break;
            }

CharFound2:
            trimCount++;
        }

        String->Length -= trimCount * sizeof(WCHAR);
    }
}

/**
 * Creates a string object using a specified length.
 *
 * \param Buffer A null-terminated Unicode string.
 * \param Length The length, in bytes, of the string.
 */
PPH_STRING PhCreateStringEx(
    _In_opt_ PCWCHAR Buffer,
    _In_ SIZE_T Length
    )
{
    PPH_STRING string;

    string = PhCreateObject(
        UFIELD_OFFSET(PH_STRING, Data) + Length + sizeof(UNICODE_NULL),
        PhStringType
        );

    assert(!(Length & 1));
    string->Length = Length;
    string->Buffer = string->Data;
    *(PWCHAR)PTR_ADD_OFFSET(string->Buffer, Length) = UNICODE_NULL;

    if (Buffer)
    {
        memcpy(string->Buffer, Buffer, Length);
    }

    return string;
}

/**
 * Creates a string object using a specified length and mode.
 *
 * \param String A string reference to create a new string object from.
 * \param Flags A combination of PH_STRING flags.
 * \param TrimCharSet A string containing characters to trim. If NULL, no trimming is performed.
 */
PPH_STRING PhCreateString3(
    _In_ PCPH_STRINGREF String,
    _In_ ULONG Flags,
    _In_opt_ PCPH_STRINGREF TrimCharSet
    )
{
    PPH_STRING string;
    PH_STRINGREF sr;

    sr = *String;
    if (TrimCharSet)
        PhTrimStringRef(&sr, TrimCharSet, Flags & PH_STRING_TRIM_MASK);

    string = PhCreateObject(
        UFIELD_OFFSET(PH_STRING, Data) + sr.Length + sizeof(UNICODE_NULL), // Null terminator for compatibility
        PhStringType
        );

    assert(!(sr.Length & 1));
    string->Length = sr.Length;
    string->Buffer = string->Data;
    *(PWCHAR)PTR_ADD_OFFSET(string->Buffer, sr.Length) = UNICODE_NULL;

    if (!sr.Buffer)
        return string;

    if (!(Flags & PH_STRING_CASE_MASK))
    {
        memcpy(string->Buffer, sr.Buffer, sr.Length);
        return string;
    }

    if (Flags & PH_STRING_UPPER_CASE)
        PhUpperStringRefInto(&string->sr, &sr);
    else
        PhLowerStringRefInto(&string->sr, &sr);

    return string;
}

/**
 * Obtains a reference to a zero-length string.
 */
PPH_STRING PhReferenceEmptyString(
    VOID
    )
{
    PPH_STRING string;
    PPH_STRING newString;

    // Initial lightweight atomic read
    string = ReadPointerAcquire(&PhSharedEmptyString);

    if (!string)
    {
        newString = PhCreateStringEx(NULL, 0);

        // Final atomic compare-and-swap
        string = InterlockedCompareExchangePointer(
            &PhSharedEmptyString,
            newString,
            NULL
            );

        if (string)
        {
            PhDereferenceObject(newString);
        }
        else
        {
            string = newString; // success
        }
    }

    return PhReferenceObject(string);
}

/**
 * Concatenates multiple strings.
 *
 * \param Count The number of strings to concatenate.
 * \param ... The list of strings.
 */
PPH_STRING PhConcatStrings(
    _In_ ULONG Count,
    ...
    )
{
    PPH_STRING string;
    va_list argptr;

    va_start(argptr, Count);
    string = PhConcatStrings_V(Count, argptr);
    va_end(argptr);

    return string;
}

/**
 * Concatenates multiple strings.
 *
 * \param Count The number of strings to concatenate.
 * \param ArgPtr A pointer to an array of strings.
 */
PPH_STRING PhConcatStrings_V(
    _In_ ULONG Count,
    _In_ va_list ArgPtr
    )
{
    va_list argptr;
    ULONG i;
    SIZE_T totalLength = 0;
    SIZE_T stringLength;
    SIZE_T cachedLengths[PH_CONCAT_STRINGS_LENGTH_CACHE_SIZE];
    PWSTR arg;
    PPH_STRING string;

    // Compute the total length, in bytes, of the strings.

    argptr = ArgPtr;

    for (i = 0; i < Count; i++)
    {
        arg = va_arg(argptr, PWSTR);
        stringLength = PhCountStringZ(arg) * sizeof(WCHAR);
        totalLength += stringLength;

        if (i < PH_CONCAT_STRINGS_LENGTH_CACHE_SIZE)
            cachedLengths[i] = stringLength;
    }

    // Create the string.

    string = PhCreateStringEx(NULL, totalLength);
    totalLength = 0;

    // Append the strings one by one.

    argptr = ArgPtr;

    for (i = 0; i < Count; i++)
    {
        arg = va_arg(argptr, PWSTR);

        if (i < PH_CONCAT_STRINGS_LENGTH_CACHE_SIZE)
            stringLength = cachedLengths[i];
        else
            stringLength = PhCountStringZ(arg) * sizeof(WCHAR);

        memcpy(
            PTR_ADD_OFFSET(string->Buffer, totalLength),
            arg,
            stringLength
            );
        totalLength += stringLength;
    }

    return string;
}

/**
 * Concatenates two strings.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 */
PPH_STRING PhConcatStrings2(
    _In_ PCWSTR String1,
    _In_ PCWSTR String2
    )
{
    PPH_STRING string;
    SIZE_T length1;
    SIZE_T length2;

    length1 = PhCountStringZ(String1) * sizeof(WCHAR);
    length2 = PhCountStringZ(String2) * sizeof(WCHAR);
    string = PhCreateStringEx(NULL, length1 + length2);
    memcpy(
        string->Buffer,
        String1,
        length1
        );
    memcpy(
        PTR_ADD_OFFSET(string->Buffer, length1),
        String2,
        length2
        );

    return string;
}

/**
 * Concatenates two strings.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 */
PPH_STRING PhConcatStringRef2(
    _In_ PCPH_STRINGREF String1,
    _In_ PCPH_STRINGREF String2
    )
{
    PPH_STRING string;

    assert(!(String1->Length & 1));
    assert(!(String2->Length & 1));

    string = PhCreateStringEx(NULL, String1->Length + String2->Length);
    memcpy(
        string->Buffer,
        String1->Buffer,
        String1->Length
        );
    memcpy(
        PTR_ADD_OFFSET(string->Buffer, String1->Length),
        String2->Buffer,
        String2->Length
        );

    return string;
}

/**
 * Concatenates three strings.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param String3 The third string.
 */
PPH_STRING PhConcatStringRef3(
    _In_ PCPH_STRINGREF String1,
    _In_ PCPH_STRINGREF String2,
    _In_ PCPH_STRINGREF String3
    )
{
    PPH_STRING string;
    PCHAR buffer;

    assert(!(String1->Length & 1));
    assert(!(String2->Length & 1));
    assert(!(String3->Length & 1));

    string = PhCreateStringEx(NULL, String1->Length + String2->Length + String3->Length);

    buffer = (PCHAR)string->Buffer;
    memcpy(buffer, String1->Buffer, String1->Length);

    buffer += String1->Length;
    memcpy(buffer, String2->Buffer, String2->Length);

    buffer += String2->Length;
    memcpy(buffer, String3->Buffer, String3->Length);

    return string;
}

/**
 * Concatenates four strings.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param String3 The third string.
 * \param String4 The forth string.
 */
PPH_STRING PhConcatStringRef4(
    _In_ PCPH_STRINGREF String1,
    _In_ PCPH_STRINGREF String2,
    _In_ PCPH_STRINGREF String3,
    _In_ PCPH_STRINGREF String4
    )
{
    PPH_STRING string;
    PCHAR buffer;

    assert(!(String1->Length & 1));
    assert(!(String2->Length & 1));
    assert(!(String3->Length & 1));
    assert(!(String4->Length & 1));

    string = PhCreateStringEx(NULL, String1->Length + String2->Length + String3->Length + String4->Length);

    buffer = (PCHAR)string->Buffer;
    memcpy(buffer, String1->Buffer, String1->Length);

    buffer += String1->Length;
    memcpy(buffer, String2->Buffer, String2->Length);

    buffer += String2->Length;
    memcpy(buffer, String3->Buffer, String3->Length);

    buffer += String3->Length;
    memcpy(buffer, String4->Buffer, String4->Length);

    return string;
}

/**
 * Creates a string using format specifiers.
 *
 * \param Format The format-control string.
 * \param ... The list of arguments.
 */
PPH_STRING PhFormatString(
    _In_ _Printf_format_string_ PCWSTR Format,
    ...
    )
{
    PPH_STRING string;
    va_list argptr;

    va_start(argptr, Format);
    string = PhFormatString_V(Format, argptr);
    va_end(argptr);

    return string;
}

/**
 * Creates a string using format specifiers.
 *
 * \param Format The format-control string.
 * \param ArgPtr A pointer to the list of arguments.
 */
PPH_STRING PhFormatString_V(
    _In_ _Printf_format_string_ PCWSTR Format,
    _In_ va_list ArgPtr
    )
{
    PPH_STRING string;
    LONG length;

    length = _vscwprintf(Format, ArgPtr);

    if (length == INT_ERROR)
        return NULL;

    string = PhCreateStringEx(NULL, length * sizeof(WCHAR));
    _vsnwprintf(string->Buffer, length, Format, ArgPtr);

    return string;
}

/**
 * Creates a bytes object.
 *
 * \param Buffer An array of bytes.
 * \param Length The length of \a Buffer, in bytes.
 */
PPH_BYTES PhCreateBytesEx(
    _In_opt_ PCCH Buffer,
    _In_ SIZE_T Length
    )
{
    PPH_BYTES bytes;

    bytes = PhCreateObject(
        UFIELD_OFFSET(PH_BYTES, Data) + Length + sizeof(ANSI_NULL), // Null terminator for compatibility
        PhBytesType
        );

    bytes->Length = Length;
    bytes->Buffer = bytes->Data;
    bytes->Buffer[Length] = ANSI_NULL;

    if (Buffer)
    {
        memcpy(bytes->Buffer, Buffer, Length);
    }

    return bytes;
}

/**
 * Formats a string using the specified format and argument list.
 *
 * \param Format A printf-style format string.
 * \param ArgPtr A va_list containing the arguments to format.
 * \return A pointer to a PPH_BYTES structure containing the formatted string.
 */
PPH_BYTES PhFormatBytes_V(
    _In_ _Printf_format_string_ PCSTR Format,
    _In_ va_list ArgPtr
    )
{
    PPH_BYTES string;
    LONG length;

    length = _vscprintf(Format, ArgPtr);

    if (length == INT_ERROR)
        return NULL;

    string = PhCreateBytesEx(NULL, length * sizeof(CHAR));
    _vsnprintf(string->Buffer, length, Format, ArgPtr);

    return string;
}

/**
 * Formats a sequence of bytes according to a specified format string.
 *
 * \param Format A printf-style format string that specifies how the bytes should be formatted.
 * \param ... Additional arguments required by the format string.
 * \return A pointer to a PPH_BYTES structure containing the formatted bytes.
 */
PPH_BYTES PhFormatBytes(
    _In_ _Printf_format_string_ PCSTR Format,
    ...
    )
{
    PPH_BYTES string;
    va_list argptr;

    va_start(argptr, Format);
    string = PhFormatBytes_V(Format, argptr);
    va_end(argptr);

    return string;
}

BOOLEAN PhWriteUnicodeDecoder(
    _Inout_ PPH_UNICODE_DECODER Decoder,
    _In_ ULONG CodeUnit
    )
{
    switch (Decoder->Encoding)
    {
    case PH_UNICODE_UTF8:
        if (Decoder->InputCount >= 4)
            return FALSE;
        Decoder->u.Utf8.Input[Decoder->InputCount] = (UCHAR)CodeUnit;
        Decoder->InputCount++;
        return TRUE;
    case PH_UNICODE_UTF16:
        if (Decoder->InputCount >= 2)
            return FALSE;
        Decoder->u.Utf16.Input[Decoder->InputCount] = (USHORT)CodeUnit;
        Decoder->InputCount++;
        return TRUE;
    case PH_UNICODE_UTF32:
        if (Decoder->InputCount >= 1)
            return FALSE;
        Decoder->u.Utf32.Input = CodeUnit;
        Decoder->InputCount = 1;
        return TRUE;
    default:
        PhRaiseStatus(STATUS_UNSUCCESSFUL);
    }
}

_Success_(return)
BOOLEAN PhpReadUnicodeDecoder(
    _Inout_ PPH_UNICODE_DECODER Decoder,
    _Out_ PULONG CodeUnit
    )
{
    switch (Decoder->Encoding)
    {
    case PH_UNICODE_UTF8:
        if (Decoder->InputCount == 0)
            return FALSE;
        *CodeUnit = Decoder->u.Utf8.Input[0];
        Decoder->u.Utf8.Input[0] = Decoder->u.Utf8.Input[1];
        Decoder->u.Utf8.Input[1] = Decoder->u.Utf8.Input[2];
        Decoder->u.Utf8.Input[2] = Decoder->u.Utf8.Input[3];
        Decoder->InputCount--;
        return TRUE;
    case PH_UNICODE_UTF16:
        if (Decoder->InputCount == 0)
            return FALSE;
        *CodeUnit = Decoder->u.Utf16.Input[0];
        Decoder->u.Utf16.Input[0] = Decoder->u.Utf16.Input[1];
        Decoder->InputCount--;
        return TRUE;
    case PH_UNICODE_UTF32:
        if (Decoder->InputCount == 0)
            return FALSE;
        *CodeUnit = Decoder->u.Utf32.Input;
        Decoder->InputCount--;
        return TRUE;
    default:
        PhRaiseStatus(STATUS_UNSUCCESSFUL);
    }
}

VOID PhpUnreadUnicodeDecoder(
    _Inout_ PPH_UNICODE_DECODER Decoder,
    _In_ ULONG CodeUnit
    )
{
    switch (Decoder->Encoding)
    {
    case PH_UNICODE_UTF8:
        if (Decoder->InputCount >= 4)
            PhRaiseStatus(STATUS_UNSUCCESSFUL);
        Decoder->u.Utf8.Input[3] = Decoder->u.Utf8.Input[2];
        Decoder->u.Utf8.Input[2] = Decoder->u.Utf8.Input[1];
        Decoder->u.Utf8.Input[1] = Decoder->u.Utf8.Input[0];
        Decoder->u.Utf8.Input[0] = (UCHAR)CodeUnit;
        Decoder->InputCount++;
        break;
    case PH_UNICODE_UTF16:
        if (Decoder->InputCount >= 2)
            PhRaiseStatus(STATUS_UNSUCCESSFUL);
        Decoder->u.Utf16.Input[1] = Decoder->u.Utf16.Input[0];
        Decoder->u.Utf16.Input[0] = (USHORT)CodeUnit;
        Decoder->InputCount++;
        break;
    case PH_UNICODE_UTF32:
        if (Decoder->InputCount >= 1)
            PhRaiseStatus(STATUS_UNSUCCESSFUL);
        Decoder->u.Utf32.Input = CodeUnit;
        Decoder->InputCount = 1;
        break;
    default:
        PhRaiseStatus(STATUS_UNSUCCESSFUL);
    }
}

BOOLEAN PhpDecodeUtf8Error(
    _Inout_ PPH_UNICODE_DECODER Decoder,
    _Out_ PULONG CodePoint,
    _In_ ULONG Which
    )
{
    if (Which >= 4)
        PhpUnreadUnicodeDecoder(Decoder, Decoder->u.Utf8.CodeUnit4);
    if (Which >= 3)
        PhpUnreadUnicodeDecoder(Decoder, Decoder->u.Utf8.CodeUnit3);
    if (Which >= 2)
        PhpUnreadUnicodeDecoder(Decoder, Decoder->u.Utf8.CodeUnit2);

    *CodePoint = (ULONG)Decoder->u.Utf8.CodeUnit1 + 0xdc00;
    Decoder->State = 0;

    return TRUE;
}

_Use_decl_annotations_
BOOLEAN PhDecodeUnicodeDecoder(
    _Inout_ PPH_UNICODE_DECODER Decoder,
    _Out_ PULONG CodePoint
    )
{
    ULONG codeUnit;

    while (TRUE)
    {
        switch (Decoder->Encoding)
        {
        case PH_UNICODE_UTF8:
            if (!PhpReadUnicodeDecoder(Decoder, &codeUnit))
                return FALSE;

            switch (Decoder->State)
            {
            case 0:
                Decoder->u.Utf8.CodeUnit1 = (UCHAR)codeUnit;

                if (codeUnit < 0x80)
                {
                    *CodePoint = codeUnit;
                    return TRUE;
                }
                else if (codeUnit < 0xc2)
                {
                    return PhpDecodeUtf8Error(Decoder, CodePoint, 1);
                }
                else if (codeUnit < 0xe0)
                {
                    Decoder->State = 1; // 2 byte sequence
                    continue;
                }
                else if (codeUnit < 0xf0)
                {
                    Decoder->State = 2; // 3 byte sequence
                    continue;
                }
                else if (codeUnit < 0xf5)
                {
                    Decoder->State = 3; // 4 byte sequence
                    continue;
                }
                else
                {
                    return PhpDecodeUtf8Error(Decoder, CodePoint, 1);
                }

                break;
            case 1: // 2 byte sequence
                Decoder->u.Utf8.CodeUnit2 = (UCHAR)codeUnit;

                if ((codeUnit & 0xc0) == 0x80)
                {
                    *CodePoint = ((ULONG)Decoder->u.Utf8.CodeUnit1 << 6) + codeUnit - 0x3080;
                    Decoder->State = 0;
                    return TRUE;
                }
                else
                {
                    return PhpDecodeUtf8Error(Decoder, CodePoint, 2);
                }

                break;
            case 2: // 3 byte sequence (1)
                Decoder->u.Utf8.CodeUnit2 = (UCHAR)codeUnit;

                if (((codeUnit & 0xc0) == 0x80) &&
                    (Decoder->u.Utf8.CodeUnit1 != 0xe0 || codeUnit >= 0xa0))
                {
                    Decoder->State = 4; // 3 byte sequence (2)
                    continue;
                }
                else
                {
                    return PhpDecodeUtf8Error(Decoder, CodePoint, 2);
                }

                break;
            case 3: // 4 byte sequence (1)
                Decoder->u.Utf8.CodeUnit2 = (UCHAR)codeUnit;

                if (((codeUnit & 0xc0) == 0x80) &&
                    (Decoder->u.Utf8.CodeUnit1 != 0xf0 || codeUnit >= 0x90) &&
                    (Decoder->u.Utf8.CodeUnit1 != 0xf4 || codeUnit < 0x90))
                {
                    Decoder->State = 5; // 4 byte sequence (2)
                    continue;
                }
                else
                {
                    return PhpDecodeUtf8Error(Decoder, CodePoint, 2);
                }

                break;
            case 4: // 3 byte sequence (2)
                Decoder->u.Utf8.CodeUnit3 = (UCHAR)codeUnit;

                if ((codeUnit & 0xc0) == 0x80)
                {
                    *CodePoint =
                        ((ULONG)Decoder->u.Utf8.CodeUnit1 << 12) +
                        ((ULONG)Decoder->u.Utf8.CodeUnit2 << 6) +
                        codeUnit - 0xe2080;
                    Decoder->State = 0;
                    return TRUE;
                }
                else
                {
                    return PhpDecodeUtf8Error(Decoder, CodePoint, 3);
                }

                break;
            case 5: // 4 byte sequence (2)
                Decoder->u.Utf8.CodeUnit3 = (UCHAR)codeUnit;

                if ((codeUnit & 0xc0) == 0x80)
                {
                    Decoder->State = 6; // 4 byte sequence (3)
                    continue;
                }
                else
                {
                    return PhpDecodeUtf8Error(Decoder, CodePoint, 3);
                }

                break;
            case 6: // 4 byte sequence (3)
                Decoder->u.Utf8.CodeUnit4 = (UCHAR)codeUnit;

                if ((codeUnit & 0xc0) == 0x80)
                {
                    *CodePoint =
                        ((ULONG)Decoder->u.Utf8.CodeUnit1 << 18) +
                        ((ULONG)Decoder->u.Utf8.CodeUnit2 << 12) +
                        ((ULONG)Decoder->u.Utf8.CodeUnit3 << 6) +
                        codeUnit - 0x3c82080;
                    Decoder->State = 0;
                    return TRUE;
                }
                else
                {
                    return PhpDecodeUtf8Error(Decoder, CodePoint, 4);
                }

                break;
            }

            return FALSE;
        case PH_UNICODE_UTF16:
            if (!PhpReadUnicodeDecoder(Decoder, &codeUnit))
                return FALSE;

            switch (Decoder->State)
            {
            case 0:
                if (PH_UNICODE_UTF16_IS_HIGH_SURROGATE(codeUnit))
                {
                    Decoder->u.Utf16.CodeUnit = (USHORT)codeUnit;
                    Decoder->State = 1;
                    continue;
                }
                else
                {
                    *CodePoint = codeUnit;
                    return TRUE;
                }
                break;
            case 1:
                if (PH_UNICODE_UTF16_IS_LOW_SURROGATE(codeUnit))
                {
                    *CodePoint = PH_UNICODE_UTF16_TO_CODE_POINT(Decoder->u.Utf16.CodeUnit, codeUnit);
                    Decoder->State = 0;
                    return TRUE;
                }
                else
                {
                    *CodePoint = Decoder->u.Utf16.CodeUnit;
                    PhpUnreadUnicodeDecoder(Decoder, codeUnit);
                    Decoder->State = 0;
                    return TRUE;
                }
                break;
            }

            return FALSE;
        case PH_UNICODE_UTF32:
            if (PhpReadUnicodeDecoder(Decoder, CodePoint))
                return TRUE;
            return FALSE;
        default:
            return FALSE;
        }
    }
}

_Use_decl_annotations_
BOOLEAN PhEncodeUnicode(
    _In_ UCHAR Encoding,
    _In_ ULONG CodePoint,
    _Out_opt_ PVOID CodeUnits,
    _Out_ PULONG NumberOfCodeUnits
    )
{
    switch (Encoding)
    {
    case PH_UNICODE_UTF8:
        {
            PUCHAR codeUnits = CodeUnits;

            if (CodePoint < 0x80)
            {
                *NumberOfCodeUnits = 1;

                if (codeUnits)
                    codeUnits[0] = (UCHAR)CodePoint;
            }
            else if (CodePoint <= 0x7ff)
            {
                *NumberOfCodeUnits = 2;

                if (codeUnits)
                {
                    codeUnits[0] = (UCHAR)(CodePoint >> 6) + 0xc0;
                    codeUnits[1] = (UCHAR)(CodePoint & 0x3f) + 0x80;
                }
            }
            else if (CodePoint <= 0xffff)
            {
                *NumberOfCodeUnits = 3;

                if (codeUnits)
                {
                    codeUnits[0] = (UCHAR)(CodePoint >> 12) + 0xe0;
                    codeUnits[1] = (UCHAR)((CodePoint >> 6) & 0x3f) + 0x80;
                    codeUnits[2] = (UCHAR)(CodePoint & 0x3f) + 0x80;
                }
            }
            else if (CodePoint <= PH_UNICODE_MAX_CODE_POINT)
            {
                *NumberOfCodeUnits = 4;

                if (codeUnits)
                {
                    codeUnits[0] = (UCHAR)(CodePoint >> 18) + 0xf0;
                    codeUnits[1] = (UCHAR)((CodePoint >> 12) & 0x3f) + 0x80;
                    codeUnits[2] = (UCHAR)((CodePoint >> 6) & 0x3f) + 0x80;
                    codeUnits[3] = (UCHAR)(CodePoint & 0x3f) + 0x80;
                }
            }
            else
            {
                return FALSE;
            }
        }
        return TRUE;
    case PH_UNICODE_UTF16:
        {
            PUSHORT codeUnits = CodeUnits;

            if (CodePoint < 0x10000)
            {
                *NumberOfCodeUnits = 1;

                if (codeUnits)
                    codeUnits[0] = (USHORT)CodePoint;
            }
            else if (CodePoint <= PH_UNICODE_MAX_CODE_POINT)
            {
                *NumberOfCodeUnits = 2;

                if (codeUnits)
                {
                    codeUnits[0] = PH_UNICODE_UTF16_TO_HIGH_SURROGATE(CodePoint);
                    codeUnits[1] = PH_UNICODE_UTF16_TO_LOW_SURROGATE(CodePoint);
                }
            }
            else
            {
                return FALSE;
            }
        }
        return TRUE;
    case PH_UNICODE_UTF32:
        *NumberOfCodeUnits = 1;
        if (CodeUnits)
            *(PULONG)CodeUnits = CodePoint;
        return TRUE;
    default:
        return FALSE;
    }
}

/**
 * Converts an ASCII string to a UTF-16 string by zero-extending each byte.
 *
 * \param Input The original ASCII string.
 * \param InputLength The length of \a Input.
 * \param Output A buffer which will contain the converted string.
 */
VOID PhZeroExtendToUtf16Buffer(
    _In_reads_bytes_(InputLength) PCCH Input,
    _In_ SIZE_T InputLength,
    _Out_writes_bytes_(InputLength * sizeof(WCHAR)) PWCH Output
    )
{
    SIZE_T inputLength;

    inputLength = InputLength & -4;

    if (inputLength)
    {
        do
        {
            Output[0] = C_1uTo2(Input[0]);
            Output[1] = C_1uTo2(Input[1]);
            Output[2] = C_1uTo2(Input[2]);
            Output[3] = C_1uTo2(Input[3]);
            Input += 4;
            Output += 4;
            inputLength -= 4;
        } while (inputLength != 0);
    }

    switch (InputLength & 3)
    {
    case 3:
        *Output++ = C_1uTo2(*Input++);
    case 2:
        *Output++ = C_1uTo2(*Input++);
    case 1:
        *Output++ = C_1uTo2(*Input++);
    }
}

/**
 * Converts a zero-terminated ANSI string to a UTF-16 string, extending the input as needed.
 *
 * \param Input Pointer to the input ANSI string.
 * \param InputLength Length of the input string in bytes.
 * \return A pointer to a PPH_STRING containing the UTF-16 representation of the input string.
 */
PPH_STRING PhZeroExtendToUtf16Ex(
    _In_reads_bytes_(InputLength) PCCH Input,
    _In_ SIZE_T InputLength
    )
{
    PPH_STRING string;

    string = PhCreateStringEx(NULL, InputLength * sizeof(WCHAR));
    PhZeroExtendToUtf16Buffer(Input, InputLength, string->Buffer);

    return string;
}

/**
 * Converts a UTF-16 string to an ASCII byte array.
 *
 * \param Buffer Pointer to the UTF-16 string buffer to convert.
 * \param Length Length of the UTF-16 string, in characters.
 * \param Replacement Optional character to use when a UTF-16 character cannot be represented in ASCII.
 * \return Pointer to a PPH_BYTES structure containing the converted ASCII bytes.
 */
PPH_BYTES PhConvertUtf16ToAsciiEx(
    _In_ PCWCH Buffer,
    _In_ SIZE_T Length,
    _In_opt_ CHAR Replacement
    )
{
    PPH_BYTES bytes;
    PH_UNICODE_DECODER decoder;
    PWCH in;
    SIZE_T inRemaining;
    PCH out;
    SIZE_T outLength;
    ULONG codePoint;

    bytes = PhCreateBytesEx(NULL, Length / sizeof(WCHAR));
    PhInitializeUnicodeDecoder(&decoder, PH_UNICODE_UTF16);
    in = (PWCH)Buffer;
    inRemaining = Length / sizeof(WCHAR);
    out = bytes->Buffer;
    outLength = 0;

    while (inRemaining != 0)
    {
        PhWriteUnicodeDecoder(&decoder, (USHORT)*in);
        in++;
        inRemaining--;

        while (PhDecodeUnicodeDecoder(&decoder, &codePoint))
        {
            if (codePoint < 0x80)
            {
                *out++ = (CHAR)codePoint;
                outLength++;
            }
            else if (Replacement)
            {
                *out++ = Replacement;
                outLength++;
            }
        }
    }

    bytes->Length = outLength;
    bytes->Buffer[outLength] = ANSI_NULL;

    return bytes;
}

/**
 * Creates a string object from an existing null-terminated multi-byte string.
 *
 * \param Buffer A null-terminated multi-byte string.
 */
PPH_STRING PhConvertMultiByteToUtf16(
    _In_ PCSTR Buffer
    )
{
    return PhConvertMultiByteToUtf16Ex(
        Buffer,
        strlen(Buffer)
        );
}

/**
 * Creates a string object from an existing null-terminated multi-byte string.
 *
 * \param Buffer A null-terminated multi-byte string.
 * \param Length The number of bytes to use.
 */
PPH_STRING PhConvertMultiByteToUtf16Ex(
    _In_ PCSTR Buffer,
    _In_ SIZE_T Length
    )
{
    NTSTATUS status;
    PPH_STRING string;
    ULONG bufferLength;
    ULONG unicodeBytes;

    status = RtlSizeTToULong(Length, &bufferLength);

    if (!NT_SUCCESS(status))
        return NULL;

    status = RtlMultiByteToUnicodeSize(
        &unicodeBytes,
        Buffer,
        bufferLength
        );

    if (!NT_SUCCESS(status))
        return NULL;

    string = PhCreateStringEx(NULL, unicodeBytes);
    status = RtlMultiByteToUnicodeN(
        string->Buffer,
        (ULONG)string->Length,
        NULL,
        Buffer,
        bufferLength
        );

    if (!NT_SUCCESS(status))
    {
        PhDereferenceObject(string);
        return NULL;
    }

    return string;
}

/**
 * Creates a multi-byte string from an existing null-terminated UTF-16 string.
 *
 * \param Buffer A null-terminated UTF-16 string.
 */
PPH_BYTES PhConvertUtf16ToMultiByte(
    _In_ PCWSTR Buffer
    )
{
    return PhConvertUtf16ToMultiByteEx(
        Buffer,
        PhCountStringZ(Buffer) * sizeof(WCHAR)
        );
}

/**
 * Creates a multi-byte string from an existing null-terminated UTF-16 string.
 *
 * \param Buffer A null-terminated UTF-16 string.
 * \param Length The number of bytes to use.
 */
PPH_BYTES PhConvertUtf16ToMultiByteEx(
    _In_ PCWCH Buffer,
    _In_ SIZE_T Length
    )
{
    NTSTATUS status;
    PPH_BYTES bytes;
    ULONG bufferLength;
    ULONG multiByteLength;

    status = RtlSizeTToULong(Length, &bufferLength);

    if (!NT_SUCCESS(status))
        return NULL;

    status = RtlUnicodeToMultiByteSize(
        &multiByteLength,
        Buffer,
        bufferLength
        );

    if (!NT_SUCCESS(status))
        return NULL;

    bytes = PhCreateBytesEx(NULL, multiByteLength);
    status = RtlUnicodeToMultiByteN(
        bytes->Buffer,
        (ULONG)bytes->Length,
        NULL,
        Buffer,
        bufferLength
        );

    if (!NT_SUCCESS(status))
    {
        PhDereferenceObject(bytes);
        return NULL;
    }

    return bytes;
}

/**
 * Converts a UTF-8 encoded string to its UTF-16 equivalent and calculates the size in bytes of the resulting UTF-16 string.
 *
 * \param BytesInUtf16String Pointer to a variable that receives the size in bytes of the UTF-16 string.
 * \param Utf8String Pointer to the UTF-8 encoded input string.
 * \param BytesInUtf8String The size in bytes of the UTF-8 input string.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhConvertUtf8ToUtf16Size(
    _Out_ PSIZE_T BytesInUtf16String,
    _In_reads_bytes_(BytesInUtf8String) PCCH Utf8String,
    _In_ SIZE_T BytesInUtf8String
    )
{
#if defined(PH_NATIVE_STRING_CONVERSION)
    NTSTATUS status;
    ULONG bytesInUtf16String = 0;

    status = RtlUTF8ToUnicodeN(
        NULL,
        0,
        &bytesInUtf16String,
        Utf8String,
        (ULONG)BytesInUtf8String
        );

    if (NT_SUCCESS(status))
    {
        if (BytesInUtf16String)
        {
            *BytesInUtf16String = bytesInUtf16String;
        }
    }

    return status;
#else
    NTSTATUS status;
    PH_UNICODE_DECODER decoder;
    PCH in;
    SIZE_T inRemaining;
    SIZE_T bytesInUtf16String;
    ULONG codePoint;
    ULONG numberOfCodeUnits;

    status = STATUS_SUCCESS;
    PhInitializeUnicodeDecoder(&decoder, PH_UNICODE_UTF8);
    in = Utf8String;
    inRemaining = BytesInUtf8String;
    bytesInUtf16String = 0;

    while (inRemaining != 0)
    {
        PhWriteUnicodeDecoder(&decoder, (UCHAR)*in);
        in++;
        inRemaining--;

        while (PhDecodeUnicodeDecoder(&decoder, &codePoint))
        {
            if (PhEncodeUnicode(PH_UNICODE_UTF16, codePoint, NULL, &numberOfCodeUnits))
            {
                bytesInUtf16String += numberOfCodeUnits * sizeof(WCHAR);
            }
            else
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
        }

        if (!NT_SUCCESS(status))
            break;
    }

    *BytesInUtf16String = bytesInUtf16String;

    return status;
#endif
}

/**
 * Converts a UTF-8 encoded string to a UTF-16 encoded buffer.
 *
 * \param Utf16String Pointer to the buffer that receives the converted UTF-16 string.
 * \param MaxBytesInUtf16String The maximum number of bytes that can be written to Utf16String.
 * \param BytesInUtf16String Optional pointer that receives the number of bytes written to Utf16String.
 * \param Utf8String Pointer to the UTF-8 encoded input string.
 * \param BytesInUtf8String The number of bytes in the input UTF-8 string.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhConvertUtf8ToUtf16Buffer(
    _Out_writes_bytes_to_(MaxBytesInUtf16String, *BytesInUtf16String) PWCH Utf16String,
    _In_ SIZE_T MaxBytesInUtf16String,
    _Out_opt_ PSIZE_T BytesInUtf16String,
    _In_reads_bytes_(BytesInUtf8String) PCCH Utf8String,
    _In_ SIZE_T BytesInUtf8String
    )
{
#if defined(PH_NATIVE_STRING_CONVERSION)
    NTSTATUS status;
    ULONG bytesInUtf16String = 0;

    status = RtlUTF8ToUnicodeN(
        Utf16String,
        (ULONG)MaxBytesInUtf16String,
        &bytesInUtf16String,
        Utf8String,
        (ULONG)BytesInUtf8String
        );

    if (NT_SUCCESS(status))
    {
        if (BytesInUtf16String)
        {
            *BytesInUtf16String = bytesInUtf16String;
        }
    }

    return status;
#else
    NTSTATUS status;
    PH_UNICODE_DECODER decoder;
    PCH in;
    SIZE_T inRemaining;
    PWCH out;
    SIZE_T outRemaining;
    SIZE_T bytesInUtf16String;
    ULONG codePoint;
    USHORT codeUnits[2];
    ULONG numberOfCodeUnits;

    status = STATUS_SUCCESS;
    PhInitializeUnicodeDecoder(&decoder, PH_UNICODE_UTF8);
    in = Utf8String;
    inRemaining = BytesInUtf8String;
    out = Utf16String;
    outRemaining = MaxBytesInUtf16String / sizeof(WCHAR);
    bytesInUtf16String = 0;

    while (inRemaining != 0)
    {
        PhWriteUnicodeDecoder(&decoder, (UCHAR)*in);
        in++;
        inRemaining--;

        while (PhDecodeUnicodeDecoder(&decoder, &codePoint))
        {
            if (PhEncodeUnicode(PH_UNICODE_UTF16, codePoint, codeUnits, &numberOfCodeUnits))
            {
                bytesInUtf16String += numberOfCodeUnits * sizeof(WCHAR);

                if (outRemaining >= numberOfCodeUnits)
                {
                    *out++ = codeUnits[0];

                    if (numberOfCodeUnits >= 2)
                        *out++ = codeUnits[1];

                    outRemaining -= numberOfCodeUnits;
                }
                else
                {
                    status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }
            }
            else
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
        }

        if (!NT_SUCCESS(status))
            break;
    }

    if (BytesInUtf16String)
        *BytesInUtf16String = bytesInUtf16String;

    return status;
#endif
}

/**
 * Converts a UTF-8 encoded string to a UTF-16 encoded string.
 *
 * \param Buffer A pointer to a null-terminated UTF-8 encoded string.
 * \return A pointer to a PPH_STRING containing the converted UTF-16 string.
 * \remarks Returns NULL if the conversion fails.
 */
PPH_STRING PhConvertUtf8ToUtf16(
    _In_ PCSTR Buffer
    )
{
    return PhConvertUtf8ToUtf16Ex(
        Buffer,
        strlen(Buffer)
        );
}

/**
 * Converts a UTF-8 encoded string to a UTF-16 string.
 *
 * \param Buffer Pointer to the UTF-8 encoded input buffer.
 * \param Length Length of the input buffer in bytes.
 * \return A pointer to a PPH_STRING containing the converted UTF-16 string.
 * \remarks Returns NULL if the conversion fails.
 */
PPH_STRING PhConvertUtf8ToUtf16Ex(
    _In_ PCCH Buffer,
    _In_ SIZE_T Length
    )
{
    NTSTATUS status;
    PPH_STRING string;
    SIZE_T utf16Bytes;

    status = PhConvertUtf8ToUtf16Size(
        &utf16Bytes,
        Buffer,
        Length
        );

    if (!NT_SUCCESS(status))
        return NULL;

    string = PhCreateStringEx(NULL, utf16Bytes);
    status = PhConvertUtf8ToUtf16Buffer(
        string->Buffer,
        string->Length,
        NULL,
        Buffer,
        Length
        );

    if (!NT_SUCCESS(status))
    {
        PhDereferenceObject(string);
        return NULL;
    }

    return string;
}

/**
 * Calculates the size in bytes required to store the UTF-8 encoded version of a given UTF-16 string.
 *
 * \param BytesInUtf8String Pointer to a variable that receives the required size in bytes for the UTF-8 string.
 * \param Utf16String Pointer to the UTF-16 string to be converted.
 * \param BytesInUtf16String The size in bytes of the input UTF-16 string.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhConvertUtf16ToUtf8Size(
    _Out_ PSIZE_T BytesInUtf8String,
    _In_reads_bytes_(BytesInUtf16String) PCWCH Utf16String,
    _In_ SIZE_T BytesInUtf16String
    )
{
#if defined(PH_NATIVE_STRING_CONVERSION)
    NTSTATUS status;
    ULONG bytesInUtf8String = 0;

    status = RtlUnicodeToUTF8N(
        NULL,
        0,
        &bytesInUtf8String,
        Utf16String,
        (ULONG)BytesInUtf16String
        );

    if (NT_SUCCESS(status))
    {
        if (BytesInUtf8String)
        {
            *BytesInUtf8String = bytesInUtf8String;
        }
    }

    return status;
#else
    NTSTATUS status;
    PH_UNICODE_DECODER decoder;
    PWCH in;
    SIZE_T inRemaining;
    SIZE_T bytesInUtf8String;
    ULONG codePoint;
    ULONG numberOfCodeUnits;

    status = STATUS_SUCCESS;
    PhInitializeUnicodeDecoder(&decoder, PH_UNICODE_UTF16);
    in = Utf16String;
    inRemaining = BytesInUtf16String / sizeof(WCHAR);
    bytesInUtf8String = 0;

    while (inRemaining != 0)
    {
        PhWriteUnicodeDecoder(&decoder, (USHORT)*in);
        in++;
        inRemaining--;

        while (PhDecodeUnicodeDecoder(&decoder, &codePoint))
        {
            if (PhEncodeUnicode(PH_UNICODE_UTF8, codePoint, NULL, &numberOfCodeUnits))
            {
                bytesInUtf8String += numberOfCodeUnits;
            }
            else
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
        }

        if (!NT_SUCCESS(status))
            break;
    }

    *BytesInUtf8String = bytesInUtf8String;

    return status;
#endif
}

/**
 * Converts a UTF-16 encoded string to a UTF-8 encoded buffer.
 *
 * \param Utf8String A pointer to the buffer that receives the UTF-8 encoded string. *
 * \param MaxBytesInUtf8String The maximum number of bytes that can be written to the Utf8String buffer.
 * \param BytesInUtf8String Optional pointer to a variable that receives the number of bytes written to Utf8String.
 * \param Utf16String A pointer to the UTF-16 encoded input string.
 * \param BytesInUtf16String The number of bytes in the UTF-16 encoded input string.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhConvertUtf16ToUtf8Buffer(
    _Out_writes_bytes_to_(MaxBytesInUtf8String, *BytesInUtf8String) PCH Utf8String,
    _In_ SIZE_T MaxBytesInUtf8String,
    _Out_opt_ PSIZE_T BytesInUtf8String,
    _In_reads_bytes_(BytesInUtf16String) PCWCH Utf16String,
    _In_ SIZE_T BytesInUtf16String
    )
{
#if defined(PH_NATIVE_STRING_CONVERSION)
    NTSTATUS status;
    ULONG bytesInUtf8String = 0;

    status = RtlUnicodeToUTF8N(
        Utf8String,
        (ULONG)MaxBytesInUtf8String,
        &bytesInUtf8String,
        Utf16String,
        (ULONG)BytesInUtf16String
        );

    if (NT_SUCCESS(status))
    {
        if (BytesInUtf8String)
        {
            *BytesInUtf8String = bytesInUtf8String;
        }
    }

    return status;
#else
    NTSTATUS status;
    PH_UNICODE_DECODER decoder;
    PWCH in;
    SIZE_T inRemaining;
    PCH out;
    SIZE_T outRemaining;
    SIZE_T bytesInUtf8String;
    ULONG codePoint;
    UCHAR codeUnits[4];
    ULONG numberOfCodeUnits;

    PhInitializeUnicodeDecoder(&decoder, PH_UNICODE_UTF16);
    in = Utf16String;
    inRemaining = BytesInUtf16String / sizeof(WCHAR);
    out = Utf8String;
    outRemaining = MaxBytesInUtf8String;
    bytesInUtf8String = 0;

    while (inRemaining != 0)
    {
        PhWriteUnicodeDecoder(&decoder, (USHORT)*in);
        in++;
        inRemaining--;

        while (PhDecodeUnicodeDecoder(&decoder, &codePoint))
        {
            if (PhEncodeUnicode(PH_UNICODE_UTF8, codePoint, codeUnits, &numberOfCodeUnits))
            {
                bytesInUtf8String += numberOfCodeUnits;

                if (outRemaining >= numberOfCodeUnits)
                {
                    *out++ = codeUnits[0];

                    if (numberOfCodeUnits >= 2)
                        *out++ = codeUnits[1];
                    if (numberOfCodeUnits >= 3)
                        *out++ = codeUnits[2];
                    if (numberOfCodeUnits >= 4)
                        *out++ = codeUnits[3];

                    outRemaining -= numberOfCodeUnits;
                }
                else
                {
                    status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }
            }
            else
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
        }

        if (!NT_SUCCESS(status))
            break;
    }

    if (BytesInUtf8String)
    {
        *BytesInUtf8String = bytesInUtf8String;
    }

    return status;
#endif
}

/**
 * Converts a UTF-16 encoded wide string to a UTF-8 encoded byte array.
 *
 * \param Buffer Pointer to a null-terminated UTF-16 string (PCWSTR) to be converted.
 * \return A pointer to a PPH_BYTES structure containing the UTF-8 encoded result.
 * \remarks Returns NULL if the conversion fails.
 */
PPH_BYTES PhConvertUtf16ToUtf8(
    _In_ PCWSTR Buffer
    )
{
    return PhConvertUtf16ToUtf8Ex(
        Buffer,
        PhCountStringZ(Buffer) * sizeof(WCHAR)
        );
}

PPH_BYTES PhConvertUtf16ToUtf8Ex(
    _In_ PCWCH Buffer,
    _In_ SIZE_T Length
    )
{
    NTSTATUS status;
    PPH_BYTES bytes;
    SIZE_T utf8Bytes;

    status = PhConvertUtf16ToUtf8Size(
        &utf8Bytes,
        Buffer,
        Length
        );

    if (!NT_SUCCESS(status))
        return NULL;

    bytes = PhCreateBytesEx(NULL, utf8Bytes);
    status = PhConvertUtf16ToUtf8Buffer(
        bytes->Buffer,
        bytes->Length,
        NULL,
        Buffer,
        Length
        );

    if (!NT_SUCCESS(status))
    {
        PhDereferenceObject(bytes);
        return NULL;
    }

    return bytes;
}

/**
 * Initializes a string builder object.
 *
 * \param StringBuilder A string builder object.
 * \param InitialCapacity The number of bytes to allocate initially.
 */
VOID PhInitializeStringBuilder(
    _Out_ PPH_STRING_BUILDER StringBuilder,
    _In_ SIZE_T InitialCapacity
    )
{
    // Make sure the initial capacity is even, as required for all string objects.
    if (InitialCapacity & 1)
        InitialCapacity++;

    StringBuilder->AllocatedLength = InitialCapacity;

    // Allocate a PH_STRING for the string builder.
    // We will dereference it and allocate a new one when we need to resize the string.

    StringBuilder->String = PhCreateStringEx(NULL, StringBuilder->AllocatedLength);

    // We will keep modifying the Length field of the string so that:
    // 1. We know how much of the string is used, and
    // 2. The user can simply get a reference to the string and use it as-is.

    StringBuilder->String->Length = 0;

    // Write the null terminator.
    StringBuilder->String->Buffer[0] = UNICODE_NULL;

    PHLIB_INC_STATISTIC(BaseStringBuildersCreated);
}

/**
 * Frees resources used by a string builder object.
 *
 * \param StringBuilder A string builder object.
 */
VOID PhDeleteStringBuilder(
    _Inout_ PPH_STRING_BUILDER StringBuilder
    )
{
    PhDereferenceObject(StringBuilder->String);
}

/**
 * Obtains a reference to the string constructed by a string builder object and frees resources used
 * by the object.
 *
 * \param StringBuilder A string builder object.
 *
 * \return A pointer to a string. You must free the string using PhDereferenceObject() when you no
 * longer need it.
 */
PPH_STRING PhFinalStringBuilderString(
    _Inout_ PPH_STRING_BUILDER StringBuilder
    )
{
    return StringBuilder->String;
}

VOID PhpResizeStringBuilder(
    _In_ PPH_STRING_BUILDER StringBuilder,
    _In_ SIZE_T NewCapacity
    )
{
    PPH_STRING newString;

    // Double the string size. If that still isn't enough room, just use the new length.

    StringBuilder->AllocatedLength *= 2;

    if (StringBuilder->AllocatedLength < NewCapacity)
        StringBuilder->AllocatedLength = NewCapacity;

    // Allocate a new string.
    newString = PhCreateStringEx(NULL, StringBuilder->AllocatedLength);

    // Copy the old string to the new string.
    memcpy(
        newString->Buffer,
        StringBuilder->String->Buffer,
        StringBuilder->String->Length + sizeof(UNICODE_NULL) // Include null terminator
        );

    // Copy the old string length.
    newString->Length = StringBuilder->String->Length;

    // Dereference the old string and replace it with the new string.
    PhMoveReference(&StringBuilder->String, newString);

    PHLIB_INC_STATISTIC(BaseStringBuildersResized);
}

FORCEINLINE VOID PhpWriteNullTerminatorStringBuilder(
    _In_ PPH_STRING_BUILDER StringBuilder
    )
{
    assert(!(StringBuilder->String->Length & 1));
    *(PWCHAR)PTR_ADD_OFFSET(StringBuilder->String->Buffer, StringBuilder->String->Length) = UNICODE_NULL;
}

/**
 * Appends a string to the end of a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param String The string to append. Specify NULL to simply reserve \a Length bytes.
 * \param Length The number of bytes to append.
 */
VOID PhAppendStringBuilderEx(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_opt_ PWCHAR String,
    _In_ SIZE_T Length
    )
{
    if (Length == 0)
        return;

    // See if we need to re-allocate the string.
    if (StringBuilder->AllocatedLength < StringBuilder->String->Length + Length)
    {
        PhpResizeStringBuilder(StringBuilder, StringBuilder->String->Length + Length);
    }

    // Copy the string, add the length, then write the null terminator.

    if (String)
    {
        memcpy(
            PTR_ADD_OFFSET(StringBuilder->String->Buffer, StringBuilder->String->Length),
            String,
            Length
            );
    }

    StringBuilder->String->Length += Length;
    PhpWriteNullTerminatorStringBuilder(StringBuilder);
}

/**
 * Appends a character to the end of a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param Character The character to append.
 */
VOID PhAppendCharStringBuilder(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ WCHAR Character
    )
{
    if (StringBuilder->AllocatedLength < StringBuilder->String->Length + sizeof(WCHAR))
    {
        PhpResizeStringBuilder(StringBuilder, StringBuilder->String->Length + sizeof(WCHAR));
    }

    *(PWCHAR)PTR_ADD_OFFSET(StringBuilder->String->Buffer, StringBuilder->String->Length) = Character;
    StringBuilder->String->Length += sizeof(WCHAR);
    PhpWriteNullTerminatorStringBuilder(StringBuilder);
}

/**
 * Appends a number of characters to the end of a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param Character The character to append.
 * \param Count The number of times to append the character.
 */
VOID PhAppendCharStringBuilder2(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ WCHAR Character,
    _In_ SIZE_T Count
    )
{
    if (Count == 0)
        return;

    // See if we need to re-allocate the string.
    if (StringBuilder->AllocatedLength < StringBuilder->String->Length + Count * sizeof(WCHAR))
    {
        PhpResizeStringBuilder(StringBuilder, StringBuilder->String->Length + Count * sizeof(WCHAR));
    }

    wmemset(
        PTR_ADD_OFFSET(StringBuilder->String->Buffer, StringBuilder->String->Length),
        Character,
        Count
        );

    StringBuilder->String->Length += Count * sizeof(WCHAR);
    PhpWriteNullTerminatorStringBuilder(StringBuilder);
}

/**
 * Appends a formatted string to the end of a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param Format The format-control string.
 * \param ... The list of strings.
 */
VOID PhAppendFormatStringBuilder(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ _Printf_format_string_ PCWSTR Format,
    ...
    )
{
    va_list argptr;

    va_start(argptr, Format);
    PhAppendFormatStringBuilder_V(StringBuilder, Format, argptr);
    va_end(argptr);
}

/**
 * Appends a formatted string to the specified string builder using a variable argument list.
 *
 * \param StringBuilder A pointer to the string builder to which the formatted string will be appended.
 * \param Format A printf-style format string that specifies how to format the arguments.
 * \param ArgPtr A va_list containing the arguments to format according to the format string.
 */
VOID PhAppendFormatStringBuilder_V(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ _Printf_format_string_ PCWSTR Format,
    _In_ va_list ArgPtr
    )
{
    LONG length;
    SIZE_T lengthInBytes;

    length = _vscwprintf(Format, ArgPtr);

    if (length == INT_ERROR || length == 0)
        return;

    lengthInBytes = length * sizeof(WCHAR);

    if (StringBuilder->AllocatedLength < StringBuilder->String->Length + lengthInBytes)
        PhpResizeStringBuilder(StringBuilder, StringBuilder->String->Length + lengthInBytes);

    _vsnwprintf(
        PTR_ADD_OFFSET(StringBuilder->String->Buffer, StringBuilder->String->Length),
        length,
        Format,
        ArgPtr
        );

    StringBuilder->String->Length += lengthInBytes;
    PhpWriteNullTerminatorStringBuilder(StringBuilder);
}

/**
 * Inserts a string into a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param Index The index, in characters, at which to insert the string.
 * \param String The string to insert.
 */
VOID PhInsertStringBuilder(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ SIZE_T Index,
    _In_ PCPH_STRINGREF String
    )
{
    PhInsertStringBuilderEx(
        StringBuilder,
        Index,
        String->Buffer,
        String->Length
        );
}

/**
 * Inserts a string into a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param Index The index, in characters, at which to insert the string.
 * \param String The string to insert.
 */
VOID PhInsertStringBuilder2(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ SIZE_T Index,
    _In_ PCWSTR String
    )
{
    PhInsertStringBuilderEx(
        StringBuilder,
        Index,
        String,
        PhCountStringZ(String) * sizeof(WCHAR)
        );
}

/**
 * Inserts a string into a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param Index The index, in characters, at which to insert the string.
 * \param String The string to insert. Specify NULL to simply reserve \a Length bytes at \a Index.
 * \param Length The number of bytes to insert.
 */
VOID PhInsertStringBuilderEx(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ SIZE_T Index,
    _In_opt_ PCWCHAR String,
    _In_ SIZE_T Length
    )
{
    if (Length == 0)
        return;

    // See if we need to re-allocate the string.
    if (StringBuilder->AllocatedLength < StringBuilder->String->Length + Length)
    {
        PhpResizeStringBuilder(StringBuilder, StringBuilder->String->Length + Length);
    }

    if (Index * sizeof(WCHAR) < StringBuilder->String->Length)
    {
        // Create some space for the string.
        memmove(
            &StringBuilder->String->Buffer[Index + Length / sizeof(WCHAR)],
            &StringBuilder->String->Buffer[Index],
            StringBuilder->String->Length - Index * sizeof(WCHAR)
            );
    }

    if (String)
    {
        // Copy the new string.
        memcpy(
            &StringBuilder->String->Buffer[Index],
            String,
            Length
            );
    }

    StringBuilder->String->Length += Length;
    PhpWriteNullTerminatorStringBuilder(StringBuilder);
}

/**
 * Removes characters from a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param StartIndex The index, in characters, at which to begin removing characters.
 * \param Count The number of characters to remove.
 */
VOID PhRemoveStringBuilder(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ SIZE_T StartIndex,
    _In_ SIZE_T Count
    )
{
    // Overwrite the removed part with the part
    // behind it.

    memmove(
        &StringBuilder->String->Buffer[StartIndex],
        &StringBuilder->String->Buffer[StartIndex + Count],
        StringBuilder->String->Length - (Count + StartIndex) * sizeof(WCHAR)
        );
    StringBuilder->String->Length -= Count * sizeof(WCHAR);
    PhpWriteNullTerminatorStringBuilder(StringBuilder);
}

/**
 * Initializes a byte string builder object.
 *
 * \param BytesBuilder A byte string builder object.
 * \param InitialCapacity The number of bytes to allocate initially.
 */
VOID PhInitializeBytesBuilder(
    _Out_ PPH_BYTES_BUILDER BytesBuilder,
    _In_ SIZE_T InitialCapacity
    )
{
    BytesBuilder->AllocatedLength = InitialCapacity;
    BytesBuilder->Bytes = PhCreateBytesEx(NULL, BytesBuilder->AllocatedLength);
    BytesBuilder->Bytes->Length = 0;
    BytesBuilder->Bytes->Buffer[0] = ANSI_NULL;
}

/**
 * Frees resources used by a byte string builder object.
 *
 * \param BytesBuilder A byte string builder object.
 */
VOID PhDeleteBytesBuilder(
    _Inout_ PPH_BYTES_BUILDER BytesBuilder
    )
{
    PhDereferenceObject(BytesBuilder->Bytes);
}

/**
 * Obtains a reference to the byte string constructed by a byte string builder object and frees
 * resources used by the object.
 *
 * \param BytesBuilder A byte string builder object.
 * \return A pointer to a byte string. You must free the byte string using PhDereferenceObject()
 * when you no longer need it.
 */
PPH_BYTES PhFinalBytesBuilderBytes(
    _Inout_ PPH_BYTES_BUILDER BytesBuilder
    )
{
    return BytesBuilder->Bytes;
}

VOID PhpResizeBytesBuilder(
    _In_ PPH_BYTES_BUILDER BytesBuilder,
    _In_ SIZE_T NewCapacity
    )
{
    PPH_BYTES newBytes;

    // Double the byte string size. If that still isn't enough room, just use the new length.

    BytesBuilder->AllocatedLength *= 2;

    if (BytesBuilder->AllocatedLength < NewCapacity)
        BytesBuilder->AllocatedLength = NewCapacity;

    // Allocate a new byte string.
    newBytes = PhCreateBytesEx(NULL, BytesBuilder->AllocatedLength);

    // Copy the old byte string to the new byte string.
    memcpy(
        newBytes->Buffer,
        BytesBuilder->Bytes->Buffer,
        BytesBuilder->Bytes->Length + sizeof(ANSI_NULL) // Include null terminator
        );

    // Copy the old byte string length.
    newBytes->Length = BytesBuilder->Bytes->Length;

    // Dereference the old byte string and replace it with the new byte string.
    PhMoveReference(&BytesBuilder->Bytes, newBytes);
}

FORCEINLINE VOID PhpWriteNullTerminatorBytesBuilder(
    _In_ PPH_BYTES_BUILDER BytesBuilder
    )
{
    BytesBuilder->Bytes->Buffer[BytesBuilder->Bytes->Length] = ANSI_NULL;
}

/**
 * Appends a byte string to the end of a byte string builder string.
 *
 * \param BytesBuilder A byte string builder object.
 * \param Bytes The byte string to append.
 */
VOID PhAppendBytesBuilder(
    _Inout_ PPH_BYTES_BUILDER BytesBuilder,
    _In_ PPH_BYTESREF Bytes
    )
{
    PhAppendBytesBuilderEx(
        BytesBuilder,
        Bytes->Buffer,
        Bytes->Length,
        0,
        NULL
        );
}

/**
 * Appends a byte string to the end of a byte string builder string.
 *
 * \param BytesBuilder A byte string builder object.
 * \param Buffer The byte string to append. Specify NULL to simply reserve \a Length bytes.
 * \param Length The number of bytes to append.
 * \param Alignment The required alignment. This should not be greater than 8.
 * \param Offset A variable which receives the byte offset of the appended or reserved bytes in the
 * byte string builder string.
 *
 * \return A pointer to the appended or reserved bytes.
 */
PVOID PhAppendBytesBuilderEx(
    _Inout_ PPH_BYTES_BUILDER BytesBuilder,
    _In_opt_ PVOID Buffer,
    _In_ SIZE_T Length,
    _In_opt_ SIZE_T Alignment,
    _Out_opt_ PSIZE_T Offset
    )
{
    SIZE_T currentLength;

    currentLength = BytesBuilder->Bytes->Length;

    if (Length == 0)
        goto Done;

    if (Alignment)
        currentLength = ALIGN_UP_BY(currentLength, Alignment);

    // See if we need to re-allocate the byte string.
    if (BytesBuilder->AllocatedLength < currentLength + Length)
        PhpResizeBytesBuilder(BytesBuilder, currentLength + Length);

    // Copy the byte string, add the length, then write the null terminator.

    if (Buffer)
        memcpy(BytesBuilder->Bytes->Buffer + currentLength, Buffer, Length);

    BytesBuilder->Bytes->Length = currentLength + Length;
    PhpWriteNullTerminatorBytesBuilder(BytesBuilder);

Done:
    if (Offset)
        *Offset = currentLength;

    return BytesBuilder->Bytes->Buffer + currentLength;
}

/**
 * Appends formatted data to a bytes builder using a variable argument list.
 *
 * \param BytesBuilder Pointer to the bytes builder structure to which the formatted data will be appended.
 * \param Format A printf-style format string specifying how to format the data.
 * \param ArgPtr A variable argument list containing the values to format according to the Format string.
 */
VOID PhAppendFormatBytesBuilder_V(
    _Inout_ PPH_BYTES_BUILDER BytesBuilder,
    _In_ _Printf_format_string_ PCSTR Format,
    _In_ va_list ArgPtr
    )
{
    LONG length;
    SIZE_T lengthInBytes;

    length = _vscprintf(Format, ArgPtr);

    if (length == INT_ERROR || length == 0)
        return;

    lengthInBytes = length * sizeof(CHAR);

    if (BytesBuilder->AllocatedLength < BytesBuilder->Bytes->Length + lengthInBytes)
        PhpResizeBytesBuilder(BytesBuilder, BytesBuilder->Bytes->Length + lengthInBytes);

    _vsnprintf(
        PTR_ADD_OFFSET(BytesBuilder->Bytes->Buffer, BytesBuilder->Bytes->Length),
        length,
        Format,
        ArgPtr
        );

    BytesBuilder->Bytes->Length += lengthInBytes;
    PhpWriteNullTerminatorBytesBuilder(BytesBuilder);
}

/**
 * Appends formatted data to a bytes builder.
 *
 * This function formats a string using the specified format and arguments,
 * then appends the resulting bytes to the provided bytes builder.
 *
 * \param BytesBuilder A pointer to the bytes builder structure to which the formatted bytes will be appended.
 * \param Format A printf-style format string specifying how to format the data.
 * \param ... Additional arguments to be formatted according to the format string.
 */
VOID PhAppendFormatBytesBuilder(
    _Inout_ PPH_BYTES_BUILDER BytesBuilder,
    _In_ _Printf_format_string_ PCSTR Format,
    ...
    )
{
    va_list argptr;

    va_start(argptr, Format);
    PhAppendFormatBytesBuilder_V(BytesBuilder, Format, argptr);
    va_end(argptr);
}

/**
 * Creates an array object.
 *
 * \param Array An array object.
 * \param ItemSize The size of each item, in bytes.
 * \param InitialCapacity The number of elements to allocate storage for, initially.
 */
VOID PhInitializeArray(
    _Out_ PPH_ARRAY Array,
    _In_ SIZE_T ItemSize,
    _In_ SIZE_T InitialCapacity
    )
{
    // Initial capacity of 0 is not allowed.
    if (InitialCapacity == 0)
        InitialCapacity = 1;

    Array->Count = 0;
    Array->AllocatedCount = InitialCapacity;
    Array->ItemSize = ItemSize;
    Array->Items = PhAllocate(Array->AllocatedCount * ItemSize);
}

/**
 * Frees resources used by an array object.
 *
 * \param Array An array object.
 */
VOID PhDeleteArray(
    _Inout_ PPH_ARRAY Array
    )
{
    PhFree(Array->Items);
}

/**
 * Resizes an array.
 *
 * \param Array An array object.
 * \param NewCapacity The new required number of elements for which storage has been reserved. This
 * must not be smaller than the current number of items in the array.
 */
VOID PhResizeArray(
    _Inout_ PPH_ARRAY Array,
    _In_ SIZE_T NewCapacity
    )
{
    if (Array->Count > NewCapacity)
        PhRaiseStatus(STATUS_INVALID_PARAMETER_2);

    Array->AllocatedCount = NewCapacity;
    Array->Items = PhReAllocate(Array->Items, Array->AllocatedCount * Array->ItemSize);
}

/**
 * Adds an item to an array.
 *
 * \param Array An array object.
 * \param Item The item to add.
 */
VOID PhAddItemArray(
    _Inout_ PPH_ARRAY Array,
    _In_ PVOID Item
    )
{
    // See if we need to resize the list.
    if (Array->Count == Array->AllocatedCount)
    {
        Array->AllocatedCount *= 2;
        Array->Items = PhReAllocate(Array->Items, Array->AllocatedCount * Array->ItemSize);
    }

    memcpy(PhItemArray(Array, Array->Count), Item, Array->ItemSize);
    Array->Count++;
}

/**
 * Adds items to an array.
 *
 * \param Array An array object.
 * \param Items An array containing the items to add.
 * \param Count The number of items to add.
 */
VOID PhAddItemsArray(
    _Inout_ PPH_ARRAY Array,
    _In_ PVOID Items,
    _In_ SIZE_T Count
    )
{
    // See if we need to resize the list.
    if (Array->AllocatedCount < Array->Count + Count)
    {
        Array->AllocatedCount *= 2;

        if (Array->AllocatedCount < Array->Count + Count)
            Array->AllocatedCount = Array->Count + Count;

        Array->Items = PhReAllocate(Array->Items, Array->AllocatedCount * Array->ItemSize);
    }

    memcpy(
        PhItemArray(Array, Array->Count),
        Items,
        Count * Array->ItemSize
        );
    Array->Count += Count;
}

/**
 * Clears an array.
 *
 * \param Array An array object.
 */
VOID PhClearArray(
    _Inout_ PPH_ARRAY Array
    )
{
    Array->Count = 0;
}

/**
 * Removes an item from an array.
 *
 * \param Array An array object.
 * \param Index The index of the item.
 */
VOID PhRemoveItemArray(
    _Inout_ PPH_ARRAY Array,
    _In_ SIZE_T Index
    )
{
    PhRemoveItemsArray(Array, Index, 1);
}

/**
 * Removes items from an array.
 *
 * \param Array An array object.
 * \param StartIndex The index at which to begin removing items.
 * \param Count The number of items to remove.
 */
VOID PhRemoveItemsArray(
    _Inout_ PPH_ARRAY Array,
    _In_ SIZE_T StartIndex,
    _In_ SIZE_T Count
    )
{
    // Shift the items after the items forward.
    memmove(
        PhItemArray(Array, StartIndex),
        PhItemArray(Array, StartIndex + Count),
        (Array->Count - StartIndex - Count) * Array->ItemSize
        );
    Array->Count -= Count;
}

/**
 * Creates a list object.
 *
 * \param InitialCapacity The number of elements to allocate storage for, initially.
 */
PPH_LIST PhCreateList(
    _In_ ULONG InitialCapacity
    )
{
    PPH_LIST list;

    list = PhCreateObject(sizeof(PH_LIST), PhListType);

    // Initial capacity of 0 is not allowed.
    if (InitialCapacity == 0)
        InitialCapacity = 1;

    list->Count = 0;
    list->AllocatedCount = InitialCapacity;
    list->Items = PhAllocate(list->AllocatedCount * sizeof(PVOID));

    return list;
}

_Use_decl_annotations_
VOID PhpListDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_LIST list = (PPH_LIST)Object;

    PhFree(list->Items);
}

/**
 * Resizes a list.
 *
 * \param List A list object.
 * \param NewCapacity The new required number of elements for which storage has been reserved. This
 * must not be smaller than the current number of items in the list.
 */
VOID PhResizeList(
    _Inout_ PPH_LIST List,
    _In_ ULONG NewCapacity
    )
{
    if (List->Count > NewCapacity)
        PhRaiseStatus(STATUS_INVALID_PARAMETER_2);

    List->AllocatedCount = NewCapacity;
    List->Items = PhReAllocate(List->Items, List->AllocatedCount * sizeof(PVOID));
}

/**
 * Adds an item to a list.
 *
 * \param List A list object.
 * \param Item The item to add.
 */
VOID PhAddItemList(
    _Inout_ PPH_LIST List,
    _In_ PVOID Item
    )
{
    // See if we need to resize the list.
    if (List->Count == List->AllocatedCount)
    {
        List->AllocatedCount *= 2;
        List->Items = PhReAllocate(List->Items, List->AllocatedCount * sizeof(PVOID));
    }

    List->Items[List->Count++] = Item;
}

/**
 * Adds items to a list.
 *
 * \param List A list object.
 * \param Items An array containing the items to add.
 * \param Count The number of items to add.
 */
VOID PhAddItemsList(
    _Inout_ PPH_LIST List,
    _In_ PVOID *Items,
    _In_ ULONG Count
    )
{
    // See if we need to resize the list.
    if (List->AllocatedCount < List->Count + Count)
    {
        List->AllocatedCount *= 2;

        if (List->AllocatedCount < List->Count + Count)
            List->AllocatedCount = List->Count + Count;

        List->Items = PhReAllocate(List->Items, List->AllocatedCount * sizeof(PVOID));
    }

    memcpy(
        &List->Items[List->Count],
        Items,
        Count * sizeof(PVOID)
        );
    List->Count += Count;
}

/**
 * Clears a list.
 *
 * \param List A list object.
 */
VOID PhClearList(
    _Inout_ PPH_LIST List
    )
{
    List->Count = 0;
}

/**
 * Locates an item in a list.
 *
 * \param List A list object.
 * \param Item The item to search for.
 *
 * \return The index of the item. If the
 * item was not found, -1 is returned.
 */
_Use_decl_annotations_
ULONG PhFindItemList(
    _In_ PPH_LIST List,
    _In_ PVOID Item
    )
{
    for (ULONG i = 0; i < List->Count; i++)
    {
        if (List->Items[i] == Item)
            return i;
    }

    return ULONG_MAX;
}

/**
 * Inserts an item into a list.
 *
 * \param List A list object.
 * \param Index The index at which to insert the item.
 * \param Item The item to add.
 */
VOID PhInsertItemList(
    _Inout_ PPH_LIST List,
    _In_ ULONG Index,
    _In_ PVOID Item
    )
{
    PhInsertItemsList(List, Index, &Item, 1);
}

/**
 * Inserts items into a list.
 *
 * \param List A list object.
 * \param Index The index at which to insert the items.
 * \param Items An array containing the items to add.
 * \param Count The number of items to add.
 */
VOID PhInsertItemsList(
    _Inout_ PPH_LIST List,
    _In_ ULONG Index,
    _In_ PVOID *Items,
    _In_ ULONG Count
    )
{
    // See if we need to resize the list.
    if (List->AllocatedCount < List->Count + Count)
    {
        List->AllocatedCount *= 2;

        if (List->AllocatedCount < List->Count + Count)
            List->AllocatedCount = List->Count + Count;

        List->Items = PhReAllocate(List->Items, List->AllocatedCount * sizeof(PVOID));
    }

    if (Index < List->Count)
    {
        // Shift the existing items backward.
        memmove(
            &List->Items[Index + Count],
            &List->Items[Index],
            (List->Count - Index) * sizeof(PVOID)
            );
    }

    // Copy the new items into the list.
    memcpy(
        &List->Items[Index],
        Items,
        Count * sizeof(PVOID)
        );

    List->Count += Count;
}

/**
 * Removes an item from a list.
 *
 * \param List A list object.
 * \param Index The index of the item.
 */
VOID PhRemoveItemList(
    _Inout_ PPH_LIST List,
    _In_ ULONG Index
    )
{
    PhRemoveItemsList(List, Index, 1);
}

/**
 * Removes items from a list.
 *
 * \param List A list object.
 * \param StartIndex The index at which to begin removing items.
 * \param Count The number of items to remove.
 */
VOID PhRemoveItemsList(
    _Inout_ PPH_LIST List,
    _In_ ULONG StartIndex,
    _In_ ULONG Count
    )
{
    // Shift the items after the items forward.
    memmove(
        &List->Items[StartIndex],
        &List->Items[StartIndex + Count],
        (List->Count - StartIndex - Count) * sizeof(PVOID)
        );
    List->Count -= Count;
}

/**
 * Creates a pointer list object.
 *
 * \param InitialCapacity The number of elements to allocate storage for initially.
 */
PPH_POINTER_LIST PhCreatePointerList(
    _In_ ULONG InitialCapacity
    )
{
    PPH_POINTER_LIST pointerList;

    pointerList = PhCreateObject(sizeof(PH_POINTER_LIST), PhPointerListType);

    // Initial capacity of 0 is not allowed.
    if (InitialCapacity == 0)
        InitialCapacity = 1;

    pointerList->Count = 0;
    pointerList->AllocatedCount = InitialCapacity;
    pointerList->FreeEntry = ULONG_MAX;
    pointerList->NextEntry = 0;
    pointerList->Items = PhAllocate(pointerList->AllocatedCount * sizeof(PVOID));

    return pointerList;
}

_Use_decl_annotations_
VOID NTAPI PhpPointerListDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_POINTER_LIST pointerList = (PPH_POINTER_LIST)Object;

    PhFree(pointerList->Items);
}

/**
 * Decodes an index stored in a free entry.
 */
FORCEINLINE ULONG PhpDecodePointerListIndex(
    _In_ PVOID Index
    )
{
    // At least with Microsoft's compiler, shift right on a signed value preserves the sign. This is
    // important because we want decode(encode(-1)) = ((-1 << 1) | 1) >> 1 = -1.
    return (ULONG)((LONG_PTR)Index >> 1);
}

/**
 * Encodes an index for storage in a free entry.
 */
FORCEINLINE PVOID PhpEncodePointerListIndex(
    _In_ ULONG Index
    )
{
    return (PVOID)(((ULONG_PTR)Index << 1) | 0x1);
}

FORCEINLINE HANDLE PhpPointerListIndexToHandle(
    _In_ ULONG Index
    )
{
    // Add one to allow NULL handles to indicate failure/an invalid index.
    return UlongToHandle(Index + 1);
}

FORCEINLINE ULONG PhpPointerListHandleToIndex(
    _In_ HANDLE Handle
    )
{
    return HandleToUlong(Handle) - 1;
}

/**
 * Adds a pointer to a pointer list.
 *
 * \param PointerList A pointer list object.
 * \param Pointer The pointer to add. The pointer must be at least 2 byte aligned.
 * \return A handle to the pointer, valid until the pointer is removed from the pointer list.
 */
HANDLE PhAddItemPointerList(
    _Inout_ PPH_POINTER_LIST PointerList,
    _In_ PVOID Pointer
    )
{
    ULONG index;

    assert(PH_IS_LIST_POINTER_VALID(Pointer));

    // Use a free entry if possible.
    if (PointerList->FreeEntry != ULONG_MAX)
    {
        PVOID oldPointer;

        index = PointerList->FreeEntry;
        oldPointer = PointerList->Items[index];
        PointerList->Items[index] = Pointer;
        PointerList->FreeEntry = PhpDecodePointerListIndex(oldPointer);
    }
    else
    {
        // Use the next entry.
        if (PointerList->NextEntry == PointerList->AllocatedCount)
        {
            PointerList->AllocatedCount *= 2;
            PointerList->Items = PhReAllocate(PointerList->Items, PointerList->AllocatedCount * sizeof(PVOID));
        }

        index = PointerList->NextEntry++;
        PointerList->Items[index] = Pointer;
    }

    PointerList->Count++;

    return PhpPointerListIndexToHandle(index);
}

/**
 * Enumerates the next pointer in a pointer list.
 *
 * \param PointerList A pointer to the pointer list to enumerate.
 * \param EnumerationKey A pointer to a variable that maintains the enumeration state.
 * This should be initialized to zero before the first call.
 * \param Pointer Receives the next pointer in the list.
 * \param PointerHandle Receives the handle associated with the pointer, if any.
 * \return TRUE if a pointer was successfully enumerated; FALSE if there are no more pointers.
 */
_Use_decl_annotations_
BOOLEAN PhEnumPointerListEx(
    _In_ PPH_POINTER_LIST PointerList,
    _Inout_ PULONG EnumerationKey,
    _Out_ PVOID *Pointer,
    _Out_ PHANDLE PointerHandle
    )
{
    ULONG index;

    while ((index = *EnumerationKey) < PointerList->NextEntry)
    {
        PVOID pointer = PointerList->Items[index];

        (*EnumerationKey)++;

        if (PH_IS_LIST_POINTER_VALID(pointer))
        {
            *Pointer = pointer;
            *PointerHandle = PhpPointerListIndexToHandle(index);

            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Locates a pointer in a pointer list.
 *
 * \param PointerList A pointer list object.
 * \param Pointer The pointer to find. The pointer must be at least 2 byte aligned.
 * \return A handle to the pointer, valid until the pointer is removed from the pointer list. If the
 * pointer is not contained in the pointer list, NULL is returned.
 */
HANDLE PhFindItemPointerList(
    _In_ PPH_POINTER_LIST PointerList,
    _In_ PVOID Pointer
    )
{
    ULONG i;

    assert(PH_IS_LIST_POINTER_VALID(Pointer));

    for (i = 0; i < PointerList->NextEntry; i++)
    {
        if (PointerList->Items[i] == Pointer)
            return PhpPointerListIndexToHandle(i);
    }

    return NULL;
}

/**
 * Removes a pointer from a pointer list.
 *
 * \param PointerList A pointer list object.
 * \param PointerHandle A handle to the pointer to remove.
 * \remarks No checking is performed on the pointer handle. Make sure the handle is valid before
 * calling the function.
 */
VOID PhRemoveItemPointerList(
    _Inout_ PPH_POINTER_LIST PointerList,
    _In_ HANDLE PointerHandle
    )
{
    ULONG index;

    assert(PointerHandle);

    index = PhpPointerListHandleToIndex(PointerHandle);

    PointerList->Items[index] = PhpEncodePointerListIndex(PointerList->FreeEntry);
    PointerList->FreeEntry = index;

    PointerList->Count--;
}

FORCEINLINE ULONG PhpValidateHash(
    _In_ ULONG Hash
    )
{
    // No point in using a full hash when we're going to AND with size minus one anyway.
#if defined(PH_HASHTABLE_FULL_HASH) && !defined(PH_HASHTABLE_POWER_OF_TWO_SIZE)
    if (Hash != ULONG_MAX)
        return Hash;
    else
        return 0;
#else
    return Hash & MAXLONG;
#endif
}

FORCEINLINE ULONG PhpIndexFromHash(
    _In_ PPH_HASHTABLE Hashtable,
    _In_ ULONG Hash
    )
{
#ifdef PH_HASHTABLE_POWER_OF_TWO_SIZE
    return Hash & (Hashtable->AllocatedBuckets - 1);
#else
    return Hash % Hashtable->AllocatedBuckets;
#endif
}

FORCEINLINE ULONG PhpGetNumberOfBuckets(
    _In_ ULONG Capacity
    )
{
#ifdef PH_HASHTABLE_POWER_OF_TWO_SIZE
    return PhRoundUpToPowerOfTwo(Capacity);
#else
    return PhGetPrimeNumber(Capacity);
#endif
}

/**
 * Creates a hashtable object.
 *
 * \param EntrySize The size of each hashtable entry, in bytes.
 * \param EqualFunction A comparison function that is executed to compare two hashtable entries.
 * \param HashFunction A hash function that is executed to generate a hash code for a hashtable entry.
 * \param InitialCapacity The number of entries to allocate storage for initially.
 */
PPH_HASHTABLE PhCreateHashtable(
    _In_ ULONG EntrySize,
    _In_ PPH_HASHTABLE_EQUAL_FUNCTION EqualFunction,
    _In_ PPH_HASHTABLE_HASH_FUNCTION HashFunction,
    _In_ ULONG InitialCapacity
    )
{
    PPH_HASHTABLE hashtable;

    hashtable = PhCreateObject(sizeof(PH_HASHTABLE), PhHashtableType);

    // Initial capacity of 0 is not allowed.
    if (InitialCapacity == 0)
        InitialCapacity = 1;

    hashtable->EntrySize = EntrySize;
    hashtable->EqualFunction = EqualFunction;
    hashtable->HashFunction = HashFunction;

    // Allocate the buckets.
    hashtable->AllocatedBuckets = PhpGetNumberOfBuckets(InitialCapacity);
    hashtable->Buckets = PhAllocate(sizeof(ULONG) * hashtable->AllocatedBuckets);
    // Set all bucket values to -1.
    memset(hashtable->Buckets, 0xff, sizeof(ULONG) * hashtable->AllocatedBuckets);

    // Allocate the entries.
    hashtable->AllocatedEntries = hashtable->AllocatedBuckets;
    hashtable->Entries = PhAllocate(PH_HASHTABLE_ENTRY_SIZE(EntrySize) * hashtable->AllocatedEntries);

    hashtable->Count = 0;
    hashtable->FreeEntry = ULONG_MAX;
    hashtable->NextEntry = 0;

    return hashtable;
}

_Use_decl_annotations_
VOID PhpHashtableDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_HASHTABLE hashtable = (PPH_HASHTABLE)Object;

    PhFree(hashtable->Buckets);
    PhFree(hashtable->Entries);
}

VOID PhpResizeHashtable(
    _Inout_ PPH_HASHTABLE Hashtable,
    _In_ ULONG NewCapacity
    )
{
    PPH_HASHTABLE_ENTRY entry;
    ULONG i;

    // Re-allocate the buckets. Note that we don't need to keep the contents.
    Hashtable->AllocatedBuckets = PhpGetNumberOfBuckets(NewCapacity);
    PhFree(Hashtable->Buckets);
    Hashtable->Buckets = PhAllocate(sizeof(ULONG) * Hashtable->AllocatedBuckets);
    // Set all bucket values to -1.
    memset(Hashtable->Buckets, 0xff, sizeof(ULONG) * Hashtable->AllocatedBuckets);

    // Re-allocate the entries.
    Hashtable->AllocatedEntries = Hashtable->AllocatedBuckets;
    Hashtable->Entries = PhReAllocate(
        Hashtable->Entries,
        PH_HASHTABLE_ENTRY_SIZE(Hashtable->EntrySize) * Hashtable->AllocatedEntries
        );

    // Re-distribute the entries among the buckets.

    // PH_HASHTABLE_GET_ENTRY is quite slow (it involves a multiply), so we use a pointer here.
    entry = Hashtable->Entries;

    for (i = 0; i < Hashtable->NextEntry; i++)
    {
        if (entry->HashCode != ULONG_MAX)
        {
            ULONG index = PhpIndexFromHash(Hashtable, entry->HashCode);

            entry->Next = Hashtable->Buckets[index];
            Hashtable->Buckets[index] = i;
        }

        entry = PTR_ADD_OFFSET(entry, PH_HASHTABLE_ENTRY_SIZE(Hashtable->EntrySize));
    }
}

FORCEINLINE PVOID PhpAddEntryHashtable(
    _Inout_ PPH_HASHTABLE Hashtable,
    _In_ PVOID Entry,
    _In_ BOOLEAN CheckForDuplicate,
    _Out_opt_ PBOOLEAN Added
    )
{
    ULONG hashCode; // hash code of the new entry
    ULONG index; // bucket index of the new entry
    ULONG freeEntry; // index of new entry in entry array
    PPH_HASHTABLE_ENTRY entry; // pointer to new entry in entry array

    hashCode = PhpValidateHash(Hashtable->HashFunction(Entry));
    index = PhpIndexFromHash(Hashtable, hashCode);

    if (CheckForDuplicate)
    {
        ULONG i;

        for (i = Hashtable->Buckets[index]; i != ULONG_MAX; i = entry->Next)
        {
            entry = PH_HASHTABLE_GET_ENTRY(Hashtable, i);

            if (entry->HashCode == hashCode && Hashtable->EqualFunction(&entry->Body, Entry))
            {
                if (Added)
                    *Added = FALSE;

                return &entry->Body;
            }
        }
    }

    // Use a free entry if possible.
    if (Hashtable->FreeEntry != ULONG_MAX)
    {
        freeEntry = Hashtable->FreeEntry;
        entry = PH_HASHTABLE_GET_ENTRY(Hashtable, freeEntry);
        Hashtable->FreeEntry = entry->Next;
    }
    else
    {
        // Use the next entry in the entry array.

        if (Hashtable->NextEntry == Hashtable->AllocatedEntries)
        {
            // Resize the hashtable.
            PhpResizeHashtable(Hashtable, Hashtable->AllocatedBuckets * 2);
            index = PhpIndexFromHash(Hashtable, hashCode);
        }

        freeEntry = Hashtable->NextEntry++;
        entry = PH_HASHTABLE_GET_ENTRY(Hashtable, freeEntry);
    }

    // Initialize the entry.
    entry->HashCode = hashCode;
    entry->Next = Hashtable->Buckets[index];
    Hashtable->Buckets[index] = freeEntry;
    // Copy the user-supplied data to the entry.
    memcpy(&entry->Body, Entry, Hashtable->EntrySize);

    Hashtable->Count++;

    if (Added)
        *Added = TRUE;

    return &entry->Body;
}

/**
 * Adds an entry to a hashtable.
 *
 * \param Hashtable A hashtable object.
 * \param Entry The entry to add.
 * \return A pointer to the entry as stored in the hashtable. This pointer is valid until the
 * hashtable is modified. If the hashtable already contained an equal entry, NULL is returned.
 * \remarks Entries are only guaranteed to be 8 byte aligned, even on 64-bit systems.
 */
PVOID PhAddEntryHashtable(
    _Inout_ PPH_HASHTABLE Hashtable,
    _In_ PVOID Entry
    )
{
    PVOID entry;
    BOOLEAN added;

    entry = PhpAddEntryHashtable(Hashtable, Entry, TRUE, &added);

    if (added)
        return entry;
    else
        return NULL;
}

/**
 * Adds an entry to a hashtable or returns an existing one.
 *
 * \param Hashtable A hashtable object.
 * \param Entry The entry to add.
 * \param Added A variable which receives TRUE if a new entry was created, and FALSE if an existing
 * entry was returned.
 * \return A pointer to the entry as stored in the hashtable. This pointer is valid until the
 * hashtable is modified. If the hashtable already contained an equal entry, the existing entry is
 * returned. Check the value of \a Added to determine whether the returned entry is new or existing.
 * \remarks Entries are only guaranteed to be 8 byte aligned, even on 64-bit systems.
 */
PVOID PhAddEntryHashtableEx(
    _Inout_ PPH_HASHTABLE Hashtable,
    _In_ PVOID Entry,
    _Out_opt_ PBOOLEAN Added
    )
{
    return PhpAddEntryHashtable(Hashtable, Entry, TRUE, Added);
}

/**
 * Clears a hashtable.
 *
 * \param Hashtable A hashtable object.
 */
VOID PhClearHashtable(
    _Inout_ PPH_HASHTABLE Hashtable
    )
{
    if (Hashtable->Count > 0)
    {
        memset(Hashtable->Buckets, 0xff, sizeof(ULONG) * Hashtable->AllocatedBuckets);
        Hashtable->Count = 0;
        Hashtable->FreeEntry = ULONG_MAX;
        Hashtable->NextEntry = 0;
    }
}

/**
 * Enumerates the entries in a hashtable.
 *
 * \param Hashtable A hashtable object.
 * \param Entry A variable which receives a pointer to the hashtable entry. The pointer is valid
 * until the hashtable is modified.
 * \param EnumerationKey A variable which is initialized to 0 before first calling this function.
 * \return TRUE if an entry pointer was stored in \a Entry, FALSE if there are no more entries.
 * \remarks Do not modify the hashtable while the hashtable is being enumerated (between calls to
 * this function). Otherwise, the function may behave unexpectedly. You may reset the
 * \a EnumerationKey variable to 0 if you wish to restart the enumeration.
 */
_Use_decl_annotations_
BOOLEAN PhEnumHashtable(
    _In_ PPH_HASHTABLE Hashtable,
    _Out_ PVOID *Entry,
    _Inout_ PULONG EnumerationKey
    )
{
    while (*EnumerationKey < Hashtable->NextEntry)
    {
        PPH_HASHTABLE_ENTRY entry = PH_HASHTABLE_GET_ENTRY(Hashtable, *EnumerationKey);

        (*EnumerationKey)++;

        if (entry->HashCode != ULONG_MAX)
        {
            *Entry = &entry->Body;
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Locates an entry in a hashtable.
 *
 * \param Hashtable A hashtable object.
 * \param Entry An entry representing the entry to find.
 * \return A pointer to the entry as stored in the hashtable. This pointer is valid until the
 * hashtable is modified. If the entry could not be found, NULL is returned.
 * \remarks The entry specified in \a Entry can be a partial entry that is filled in enough so that
 * the comparison and hash functions can work with them.
 */
PVOID PhFindEntryHashtable(
    _In_ PPH_HASHTABLE Hashtable,
    _In_ PVOID Entry
    )
{
    ULONG hashCode;
    ULONG index;
    ULONG i;
    PPH_HASHTABLE_ENTRY entry;

    hashCode = PhpValidateHash(Hashtable->HashFunction(Entry));
    index = PhpIndexFromHash(Hashtable, hashCode);

    for (i = Hashtable->Buckets[index]; i != ULONG_MAX; i = entry->Next)
    {
        entry = PH_HASHTABLE_GET_ENTRY(Hashtable, i);

        if (entry->HashCode == hashCode && Hashtable->EqualFunction(&entry->Body, Entry))
        {
            return &entry->Body;
        }
    }

    return NULL;
}

/**
 * Removes an entry from a hashtable.
 *
 * \param Hashtable A hashtable object.
 * \param Entry The entry to remove.
 * \return TRUE if the entry was removed, FALSE if the entry could not be found.
 * \remarks The entry specified in \a Entry can be an actual entry pointer returned by
 * PhFindEntryHashtable, or a partial entry.
 */
BOOLEAN PhRemoveEntryHashtable(
    _Inout_ PPH_HASHTABLE Hashtable,
    _In_ PVOID Entry
    )
{
    ULONG hashCode;
    ULONG index;
    ULONG i;
    ULONG previousIndex;
    PPH_HASHTABLE_ENTRY entry;

    hashCode = PhpValidateHash(Hashtable->HashFunction(Entry));
    index = PhpIndexFromHash(Hashtable, hashCode);
    previousIndex = ULONG_MAX;

    for (i = Hashtable->Buckets[index]; i != ULONG_MAX; i = entry->Next)
    {
        entry = PH_HASHTABLE_GET_ENTRY(Hashtable, i);

        if (entry->HashCode == hashCode && Hashtable->EqualFunction(&entry->Body, Entry))
        {
            // Unlink the entry from the bucket.
            if (previousIndex == ULONG_MAX)
            {
                Hashtable->Buckets[index] = entry->Next;
            }
            else
            {
                PH_HASHTABLE_GET_ENTRY(Hashtable, previousIndex)->Next = entry->Next;
            }

            entry->HashCode = ULONG_MAX; // indicates the entry is not being used
            entry->Next = Hashtable->FreeEntry;
            Hashtable->FreeEntry = i;

            Hashtable->Count--;

            return TRUE;
        }

        previousIndex = i;
    }

    return FALSE;
}

/**
 * Generates a hash code for a sequence of bytes.
 *
 * \param Bytes A pointer to a byte array.
 * \param Length The number of bytes to hash.
 */
ULONG PhHashBytes(
    _In_reads_(Length) PUCHAR Bytes,
    _In_ SIZE_T Length
    )
{
    ULONG hash = 0;

    if (Length == 0)
        return hash;

    // FNV-1a algorithm: http://www.isthe.com/chongo/src/fnv/hash_32a.c

    while (Length-- != 0)
    {
        hash ^= *Bytes++;
        hash *= 0x01000193;
    }

    return hash;
}

/**
 * Generates a hash code for a string.
 *
 * \param String The string to hash.
 * \param IgnoreCase TRUE for a case-insensitive hash function, otherwise FALSE.
 */
ULONG PhHashStringRefOriginal(
    _In_ PCPH_STRINGREF String,
    _In_ BOOLEAN IgnoreCase
    )
{
    ULONG hash = 0;
    SIZE_T count;
    PWCHAR p;

    if (String->Length == 0)
        return 0;

    count = String->Length / sizeof(WCHAR);
    p = String->Buffer;

    if (IgnoreCase)
    {
        while (count-- != 0)
        {
            hash ^= (USHORT)PhUpcaseUnicodeChar(*p++);
            hash *= 0x01000193;
        }
    }
    else
    {
        return PhHashBytes((PUCHAR)String->Buffer, String->Length);
    }

    return hash;
}

/**
 * Generates a hash code for a string.
 *
 * \param String The string to hash.
 * \param IgnoreCase TRUE for a case-insensitive hash function, otherwise FALSE.
 */
ULONG PhHashStringRef(
    _In_ PCPH_STRINGREF String,
    _In_ BOOLEAN IgnoreCase
    )
{
    ULONG hash = 0;
    SIZE_T count;
    PWCHAR p;

    if (String->Length == 0)
        return 0;

    count = String->Length / sizeof(WCHAR);
    p = String->Buffer;

    if (IgnoreCase)
    {
        if (PhHasIntrinsics && count >= 8)
        {
            // SIMD hash path for case-insensitive
            SIZE_T length16 = count / 8;
            WCHAR uppercased[8];

            for (SIZE_T i = 0; i < length16; i++)
            {
                PH_INT128 chunk;

                chunk = PhLoadINT128((PLONG)p);
                chunk = PhUppercaseASCIIINT128by16(chunk);
                PhStoreINT128((PLONG)uppercased, chunk);

                // Hash 8 uppercased characters
                for (ULONG j = 0; j < 8; j++)
                {
                    hash ^= (USHORT)uppercased[j];
                    hash *= 0x01000193;
                }

                p += 8;
            }

            count &= 7;
        }

        while (count-- != 0)
        {
            hash ^= (USHORT)PhUpcaseUnicodeChar(*p++);
            hash *= 0x01000193;
        }
    }
    else
    {
        return PhHashBytes((PUCHAR)String->Buffer, String->Length);
    }

    return hash;
}

/**
 * Computes a hash value for the specified string reference using the given hash algorithm.
 *
 * \param String Pointer to a PH_STRINGREF structure representing the string to hash.
 * \param IgnoreCase If TRUE, the hash computation ignores case differences; otherwise, case is considered.
 * \param HashAlgorithm The hash algorithm to use for computing the hash value.
 * \return The computed hash value as an unsigned long.
 */
ULONG PhHashStringRefEx(
    _In_ PCPH_STRINGREF String,
    _In_ BOOLEAN IgnoreCase,
    _In_ PH_STRING_HASH HashAlgorithm
    )
{
    switch (HashAlgorithm)
    {
    case PH_STRING_HASH_DEFAULT:
    case PH_STRING_HASH_FNV1A:
        return PhHashStringRef(String, IgnoreCase);
    case PH_STRING_HASH_X65599:
        {
            ULONG hash = 0;
            PWCHAR end;
            PWCHAR p;

            if (String->Length == 0)
                return 0;

            end = String->Buffer + (String->Length / sizeof(WCHAR));

            if (IgnoreCase)
            {
                // This is the fastest implementation (copied from ReactOS) (dmex)
                for (p = String->Buffer; p != end; p++)
                {
                    hash = ((65599 * (hash)) + (ULONG)(((*p) >= L'a' && (*p) <= L'z') ? (*p) - L'a' + L'A' : (*p)));
                }

                // Medium fast
                //UNICODE_STRING unicodeString;
                //
                //if (!PhStringRefToUnicodeString(String, &unicodeString))
                //    return 0;
                //
                //if (!NT_SUCCESS(RtlHashUnicodeString(&unicodeString, TRUE, HASH_STRING_ALGORITHM_X65599, &hash)))
                //    return 0;
                //
                // Slower than the above two (based on PhHashBytes) (dmex)
                //SIZE_T count = String->Length / sizeof(WCHAR);
                //PWCHAR p = String->Buffer;
                //do
                //{
                //    hash += (USHORT)PhUpcaseUnicodeChar(*p++); // __ascii_towupper(*p++);
                //    hash *= 0x1003F;
                //} while (--count != 0);
            }
            else
            {
                // This is the fastest implementation (copied from ReactOS) (dmex)
                for (p = String->Buffer; p != end; p++)
                {
                    hash = ((65599 * (hash)) + (ULONG)(*p));
                }

                // This is fast but slightly slower (based on PhHashBytes) (dmex)
                //SIZE_T count = String->Length / sizeof(WCHAR);
                //PWCHAR p = String->Buffer;
                //do
                //{
                //    hash *= 0x1003F;
                //    hash += *p++;
                //} while (--count != 0);
            }

            return hash;
        }
    case PH_STRING_HASH_XXH32:
        {
            if (String->Length == 0)
                return 0;

            return PhHashXXH32(String->Buffer, String->Length, 0);
        }
    }

    return 0;
}

_Function_class_(PH_HASHTABLE_EQUAL_FUNCTION)
BOOLEAN NTAPI PhpSimpleHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_KEY_VALUE_PAIR entry1 = Entry1;
    PPH_KEY_VALUE_PAIR entry2 = Entry2;

    return entry1->Key == entry2->Key;
}

_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
ULONG NTAPI PhpSimpleHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PPH_KEY_VALUE_PAIR entry = Entry;

    return PhHashIntPtr((ULONG_PTR)entry->Key);
}

/**
 * Creates a simple hash table with the specified initial capacity.
 *
 * \param InitialCapacity The initial number of buckets to allocate for the hash table.
 * \return A pointer to the newly created hash table (PPH_HASHTABLE), or NULL if allocation fails.
 */
PPH_HASHTABLE PhCreateSimpleHashtable(
    _In_ ULONG InitialCapacity
    )
{
    return PhCreateHashtable(
        sizeof(PH_KEY_VALUE_PAIR),
        PhpSimpleHashtableEqualFunction,
        PhpSimpleHashtableHashFunction,
        InitialCapacity
        );
}

/**
 * Adds an item to a simple hashtable.
 *
 * \param SimpleHashtable Pointer to the hashtable to which the item will be added.
 * \param Key Optional pointer to the key for the item.
 * \param Value Optional pointer to the value to associate with the key.
 * \return Returns a pointer to the added item, or NULL if the operation fails.
 */
PVOID PhAddItemSimpleHashtable(
    _Inout_ PPH_HASHTABLE SimpleHashtable,
    _In_opt_ PVOID Key,
    _In_opt_ PVOID Value
    )
{
    PH_KEY_VALUE_PAIR entry;

    entry.Key = Key;
    entry.Value = Value;

    if (PhAddEntryHashtable(SimpleHashtable, &entry))
        return Value;
    else
        return NULL;
}

/**
 * Finds an item in a simple hashtable by its key.
 *
 * \param SimpleHashtable A pointer to the hashtable to search.
 * \param Key An optional pointer to the key to search for. If NULL, the function may behave differently depending on implementation.
 * \return A pointer to the found item, or NULL if the key is not present in the hashtable.
 */
PVOID *PhFindItemSimpleHashtable(
    _In_ PPH_HASHTABLE SimpleHashtable,
    _In_opt_ PVOID Key
    )
{
    PH_KEY_VALUE_PAIR lookupEntry;
    PPH_KEY_VALUE_PAIR entry;

    lookupEntry.Key = Key;
    entry = PhFindEntryHashtable(SimpleHashtable, &lookupEntry);

    if (entry)
        return &entry->Value;
    else
        return NULL;
}

/**
 * Removes an item from a simple hashtable.
 *
 * \param SimpleHashtable Pointer to the hashtable from which the item will be removed.
 * \param Key Optional pointer to the key of the item to remove. If NULL, no item is removed.
 * \return TRUE if the item was successfully removed; FALSE otherwise.
 */
BOOLEAN PhRemoveItemSimpleHashtable(
    _Inout_ PPH_HASHTABLE SimpleHashtable,
    _In_opt_ PVOID Key
    )
{
    PH_KEY_VALUE_PAIR lookupEntry;

    lookupEntry.Key = Key;

    return PhRemoveEntryHashtable(SimpleHashtable, &lookupEntry);
}

/**
 * Initializes a free list object.
 *
 * \param FreeList A pointer to the free list object.
 * \param Size The number of bytes in each allocation.
 * \param MaximumCount The number of unused allocations to store.
 */
VOID PhInitializeFreeList(
    _Out_ PPH_FREE_LIST FreeList,
    _In_ SIZE_T Size,
    _In_ ULONG MaximumCount
    )
{
    PhInitializeSListHead(&FreeList->ListHead);
    FreeList->Count = 0;
    FreeList->MaximumCount = MaximumCount;
    FreeList->Size = Size;
}

/**
 * Frees resources used by a free list object.
 *
 * \param FreeList A pointer to the free list object.
 */
VOID PhDeleteFreeList(
    _Inout_ PPH_FREE_LIST FreeList
    )
{
    PPH_FREE_LIST_ENTRY entry;
    PSLIST_ENTRY listEntry;

    listEntry = RtlInterlockedFlushSList(&FreeList->ListHead);

    while (listEntry)
    {
        entry = CONTAINING_RECORD(listEntry, PH_FREE_LIST_ENTRY, ListEntry);
        listEntry = listEntry->Next;
        PhFree(entry);
    }
}

/**
 * Allocates a block of memory from a free list.
 *
 * \param FreeList A pointer to a free list object.
 *
 * \return A pointer to the allocated block of memory. The memory must be freed using
 * PhFreeToFreeList(). The block is guaranteed to be aligned at MEMORY_ALLOCATION_ALIGNMENT bytes.
 */
PVOID PhAllocateFromFreeList(
    _Inout_ PPH_FREE_LIST FreeList
    )
{
    PPH_FREE_LIST_ENTRY entry;
    PSLIST_ENTRY listEntry;

    listEntry = RtlInterlockedPopEntrySList(&FreeList->ListHead);

    if (listEntry)
    {
        _InterlockedDecrement((PLONG)&FreeList->Count);
        entry = CONTAINING_RECORD(listEntry, PH_FREE_LIST_ENTRY, ListEntry);
    }
    else
    {
        entry = PhAllocate(UFIELD_OFFSET(PH_FREE_LIST_ENTRY, Body) + FreeList->Size);
    }

    return &entry->Body;
}

/**
 * Frees a block of memory to a free list.
 *
 * \param FreeList A pointer to a free list object.
 * \param Memory A pointer to a block of memory.
 */
VOID PhFreeToFreeList(
    _Inout_ PPH_FREE_LIST FreeList,
    _In_ _Post_invalid_ PVOID Memory
    )
{
    PPH_FREE_LIST_ENTRY entry;

    entry = CONTAINING_RECORD(Memory, PH_FREE_LIST_ENTRY, Body);

    // We don't enforce Count <= MaximumCount (that would require locking),
    // but we do check it.
    if (FreeList->Count < FreeList->MaximumCount)
    {
        RtlInterlockedPushEntrySList(&FreeList->ListHead, &entry->ListEntry);
        _InterlockedIncrement((PLONG)&FreeList->Count);
    }
    else
    {
        PhFree(entry);
    }
}

/**
 * Initializes a callback object.
 *
 * \param Callback A pointer to a callback object.
 */
VOID PhInitializeCallback(
    _Out_ PPH_CALLBACK Callback
    )
{
    InitializeListHead(&Callback->ListHead);
    PhInitializeQueuedLock(&Callback->ListLock);
    PhInitializeCondition(&Callback->BusyCondition);
}

/**
 * Frees resources used by a callback object.
 *
 * \param Callback A pointer to a callback object.
 */
VOID PhDeleteCallback(
    _Inout_ PPH_CALLBACK Callback
    )
{
    PhAcquireQueuedLockExclusive(&Callback->ListLock);
    
    // Assert that all callbacks have been unregistered
    assert(IsListEmpty(&Callback->ListHead));
    
    PhReleaseQueuedLockExclusive(&Callback->ListLock);
}

/**
 * Registers a callback function to be notified.
 *
 * \param Callback A pointer to a callback object.
 * \param Function The callback function.
 * \param Context A user-defined value to pass to the callback function.
 * \param Registration A variable which receives registration information for the callback. Do not
 * modify the contents of this structure and do not free the storage for this structure until you
 * have unregistered the callback.
 */
VOID PhRegisterCallback(
    _Inout_ PPH_CALLBACK Callback,
    _In_ PPH_CALLBACK_FUNCTION Function,
    _In_opt_ PVOID Context,
    _Out_ PPH_CALLBACK_REGISTRATION Registration
    )
{
    PhRegisterCallbackEx(
        Callback,
        Function,
        Context,
        0,
        Registration
        );
}

/**
 * Registers a callback function to be notified.
 *
 * \param Callback A pointer to a callback object.
 * \param Function The callback function.
 * \param Context A user-defined value to pass to the callback function.
 * \param Flags A combination of flags controlling the callback. Set this parameter to 0.
 * \param Registration A variable which receives registration information for the callback. Do not
 * modify the contents of this structure and do not free the storage for this structure until you
 * have unregistered the callback.
 */
VOID PhRegisterCallbackEx(
    _Inout_ PPH_CALLBACK Callback,
    _In_ PPH_CALLBACK_FUNCTION Function,
    _In_opt_ PVOID Context,
    _In_ USHORT Flags,
    _Out_ PPH_CALLBACK_REGISTRATION Registration
    )
{
    Registration->Function = Function;
    Registration->Context = Context;
    Registration->Busy = 0;
    Registration->Unregistering = FALSE;
    Registration->Flags = Flags;

    PhAcquireQueuedLockExclusive(&Callback->ListLock);
    InsertTailList(&Callback->ListHead, &Registration->ListEntry);
    PhReleaseQueuedLockExclusive(&Callback->ListLock);
}

/**
 * Unregisters a callback function.
 *
 * \param Callback A pointer to a callback object.
 * \param Registration The structure returned by PhRegisterCallback().
 *
 * \remarks It is guaranteed that the callback function will not be in execution once this function
 * returns. Attempting to unregister a callback function from within the same function will result
 * in a deadlock.
 */
VOID PhUnregisterCallback(
    _Inout_ PPH_CALLBACK Callback,
    _Inout_ PPH_CALLBACK_REGISTRATION Registration
    )
{
    Registration->Unregistering = TRUE;

    PhAcquireQueuedLockExclusive(&Callback->ListLock);

    // Wait for the callback to be unbusy.
    while (Registration->Busy)
        PhWaitForCondition(&Callback->BusyCondition, &Callback->ListLock, NULL);

    RemoveEntryList(&Registration->ListEntry);

    PhReleaseQueuedLockExclusive(&Callback->ListLock);
}

/**
 * Notifies all registered callback functions.
 *
 * \param Callback A pointer to a callback object.
 * \param Parameter A value to pass to all callback functions.
 */
VOID PhInvokeCallback(
    _In_ PPH_CALLBACK Callback,
    _In_opt_ PVOID Parameter
    )
{
    PLIST_ENTRY listEntry;

    PhTraceFuncEnter("Invoke callback %p", Callback);

    PhAcquireQueuedLockShared(&Callback->ListLock);

    for (listEntry = Callback->ListHead.Flink; listEntry != &Callback->ListHead; listEntry = listEntry->Flink)
    {
        PPH_CALLBACK_REGISTRATION registration;
        LONG busy;

        registration = CONTAINING_RECORD(listEntry, PH_CALLBACK_REGISTRATION, ListEntry);

        // Don't bother executing the callback function if it is being unregistered.
        if (registration->Unregistering)
            continue;

        _InterlockedIncrement(&registration->Busy);

        // Execute the callback function.

        PhReleaseQueuedLockShared(&Callback->ListLock);
        registration->Function(
            Parameter,
            registration->Context
            );
        PhAcquireQueuedLockShared(&Callback->ListLock);

        busy = _InterlockedDecrement(&registration->Busy);

        if (registration->Unregistering && busy == 0)
        {
            // Someone started unregistering while the callback function was executing, and we must
            // wake them.
            PhPulseAllCondition(&Callback->BusyCondition);
        }
    }

    PhReleaseQueuedLockShared(&Callback->ListLock);

    PhTraceFuncExit("Invoke callback %p", Callback);
}

/**
 * Retrieves a prime number bigger than or equal to the specified number.
 */
ULONG PhGetPrimeNumber(
    _In_ ULONG Minimum
    )
{
    ULONG i, j;

    for (i = 0; i < sizeof(PhpPrimeNumbers) / sizeof(ULONG); i++)
    {
        if (PhpPrimeNumbers[i] >= Minimum)
            return PhpPrimeNumbers[i];
    }

    for (i = Minimum | 1; i < MAXLONG; i += 2)
    {
        ULONG sqrtI = (ULONG)sqrt(i);

        for (j = 3; j <= sqrtI; j += 2)
        {
            if (i % j == 0)
            {
                // Not a prime.
                goto NextPrime;
            }
        }

        // Success.
        return i;
NextPrime:
        NOTHING;
    }

    return Minimum;
}

/**
 * Rounds up a number to the next power of two.
 */
ULONG PhRoundUpToPowerOfTwo(
    _In_ ULONG Number
    )
{
    Number--;
    Number |= Number >> 1;
    Number |= Number >> 2;
    Number |= Number >> 4;
    Number |= Number >> 8;
    Number |= Number >> 16;
    Number++;

    return Number;
}

/**
 * Performs exponentiation.
 */
ULONG PhExponentiate(
    _In_ ULONG Base,
    _In_ ULONG Exponent
    )
{
    ULONG result = 1;

    while (Exponent)
    {
        if (Exponent & 1)
            result *= Base;

        Exponent >>= 1;
        Base *= Base;
    }

    return result;
}

/**
 * Performs 64-bit exponentiation.
 */
ULONG64 PhExponentiate64(
    _In_ ULONG64 Base,
    _In_ ULONG Exponent
    )
{
    ULONG64 result = 1;

    while (Exponent)
    {
        if (Exponent & 1)
            result *= Base;

        Exponent >>= 1;
        Base *= Base;
    }

    return result;
}

/**
 * Converts a sequence of hexadecimal digits into a byte array.
 *
 * \param String A string containing hexadecimal digits to convert. The string must have an even
 * number of digits, because each pair of hexadecimal digits represents one byte. Example:
 * "129a2eff5c0b".
 * \param Buffer The output buffer.
 * \return TRUE if the string was successfully converted, otherwise FALSE.
 */
BOOLEAN PhHexStringToBuffer(
    _In_ PCPH_STRINGREF String,
    _Out_writes_bytes_(String->Length / sizeof(WCHAR) / 2) PUCHAR Buffer
    )
{
    SIZE_T i;
    SIZE_T length;

    // The string must have an even length.
    if ((String->Length / sizeof(WCHAR)) & 1)
        return FALSE;

    length = String->Length / sizeof(WCHAR) / 2;

    for (i = 0; i < length; i++)
    {
        Buffer[i] =
            (UCHAR)(PhCharToInteger[(UCHAR)String->Buffer[i * sizeof(WCHAR)]] << 4) +
            (UCHAR)PhCharToInteger[(UCHAR)String->Buffer[i * sizeof(WCHAR) + 1]];
    }

    return TRUE;
}

/**
 * Converts a hexadecimal string to a binary buffer.
 *
 * \param String Pointer to a PH_STRINGREF structure containing the hexadecimal string to convert.
 * \param BufferLength The length, in bytes, of the output buffer.
 * \param Buffer Pointer to the buffer that receives the converted binary data. Must be at least BufferLength bytes.
 * \return TRUE if the conversion was successful; FALSE otherwise.
 */

BOOLEAN PhHexStringToBufferEx(
    _In_ PCPH_STRINGREF String,
    _In_ SIZE_T BufferLength,
    _Out_writes_bytes_(BufferLength) PVOID Buffer
    )
{
    SIZE_T length;
    SIZE_T i;

    if ((String->Length / sizeof(WCHAR)) & 1)
        return FALSE;

    length = String->Length / sizeof(WCHAR) / 2;

    if (length > BufferLength)
        return FALSE;

    for (i = 0; i < length; i++)
    {
        ((PBYTE)Buffer)[i] =
            (BYTE)(PhCharToInteger[(BYTE)String->Buffer[i * sizeof(WCHAR)]] << 4) +
            (BYTE)PhCharToInteger[(BYTE)String->Buffer[i * sizeof(WCHAR) + 1]];
    }

    return TRUE;
}

/**
 * Converts a byte array into a sequence of hexadecimal digits.
 *
 * \param Buffer The input buffer.
 * \param Length The number of bytes to convert.
 * \return A string containing a sequence of hexadecimal digits.
 */
PPH_STRING PhBufferToHexString(
    _In_reads_bytes_(Length) PUCHAR Buffer,
    _In_ SIZE_T Length
    )
{
    return PhBufferToHexStringEx(Buffer, Length, FALSE);
}

/**
 * Converts a byte array into a sequence of hexadecimal digits.
 *
 * \param Buffer The input buffer.
 * \param Length The number of bytes to convert.
 * \param UpperCase TRUE to use uppercase characters, otherwise FALSE.
 * \return A string containing a sequence of hexadecimal digits.
 */
PPH_STRING PhBufferToHexStringEx(
    _In_reads_bytes_(Length) PUCHAR Buffer,
    _In_ SIZE_T Length,
    _In_ BOOLEAN UpperCase
    )
{
    PPH_STRING string;
    PCCH table;
    SIZE_T i;

    if (UpperCase)
        table = PhIntegerToCharUpper;
    else
        table = PhIntegerToChar;

    string = PhCreateStringEx(NULL, Length * sizeof(WCHAR) * 2);

    for (i = 0; i < Length; i++)
    {
        string->Buffer[i * sizeof(WCHAR)] = table[Buffer[i] >> 4];
        string->Buffer[i * sizeof(WCHAR) + 1] = table[Buffer[i] & 0xf];
    }

    return string;
}

/**
 * Converts a binary buffer to a hexadecimal string representation.
 *
 * \param InputBuffer Pointer to the input buffer containing binary data.
 * \param InputLength Length of the input buffer, in bytes.
 * \param UpperCase If TRUE, output hex digits in uppercase; otherwise, lowercase.
 * \param OutputBuffer Pointer to the buffer that receives the hexadecimal string.
 * \param OutputLength Size of the output buffer, in bytes.
 * \param ReturnLength Optional pointer to receive the number of bytes written to OutputBuffer.
 * \return TRUE if the conversion was successful and the output buffer was large enough; FALSE otherwise.
 */
_Use_decl_annotations_
BOOLEAN PhBufferToHexStringBuffer(
    _In_reads_bytes_(InputLength) PUCHAR InputBuffer,
    _In_ SIZE_T InputLength,
    _In_ BOOLEAN UpperCase,
    _Out_writes_bytes_to_(OutputLength, *ReturnLength) PWSTR OutputBuffer,
    _In_ SIZE_T OutputLength,
    _Out_opt_ PSIZE_T ReturnLength
    )
{
    PCCH table;
    ULONG i;

    if (OutputLength < InputLength * sizeof(WCHAR) * 2)
    {
        if (ReturnLength)
            *ReturnLength = InputLength * sizeof(WCHAR) * 2;
        return FALSE;
    }

    if (UpperCase)
        table = PhIntegerToCharUpper;
    else
        table = PhIntegerToChar;

    for (i = 0; i < InputLength; i++)
    {
        OutputBuffer[i * sizeof(WCHAR)] = table[InputBuffer[i] >> 4];
        OutputBuffer[i * sizeof(WCHAR) + 1] = table[InputBuffer[i] & 0xf];
    }

    OutputBuffer[i * sizeof(WCHAR)] = UNICODE_NULL;

    if (ReturnLength)
        *ReturnLength = i * sizeof(WCHAR) * 2;

    return TRUE;
}

/**
 * Converts a string to an integer.
 *
 * \param String The string to process.
 * \param Base The base which the string uses to represent the integer. The maximum value is 69.
 * \param Integer The resulting integer.
 */
BOOLEAN PhpStringToInteger64(
    _In_ PCPH_STRINGREF String,
    _In_ ULONG Base,
    _Out_opt_ PULONG64 Integer
    )
{
    BOOLEAN valid = TRUE;
    ULONG64 result;
    SIZE_T length;
    SIZE_T i;

    length = String->Length / sizeof(WCHAR);
    result = 0;

    for (i = 0; i < length; i++)
    {
        ULONG value;

        value = PhCharToInteger[(UCHAR)String->Buffer[i]];

        if (value < Base)
            result = result * Base + value;
        else
            valid = FALSE;
    }

    if (Integer)
    {
        *Integer = result;
    }

    return valid;
}

/**
 * Converts a string to an integer.
 *
 * \param String The string to process.
 * \param Base The base which the string uses to represent the integer. The maximum value is 69. If
 * the parameter is 0, the base is inferred from the string.
 * \param Integer The resulting integer.
 *
 * \remarks If \a Base is 0, the following prefixes may be used to indicate bases:
 *
 * \li \c 0x Base 16.
 * \li \c 0o Base 8.
 * \li \c 0b Base 2.
 * \li \c 0t Base 3.
 * \li \c 0q Base 4.
 * \li \c 0w Base 12.
 * \li \c 0r Base 32.
 *
 * If there is no recognized prefix, base 10 is used.
 */
_Use_decl_annotations_
BOOLEAN PhStringToInteger64(
    _In_ PCPH_STRINGREF String,
    _In_opt_ ULONG Base,
    _Out_opt_ PLONG64 Integer
    )
{
    BOOLEAN valid;
    ULONG64 result;
    PH_STRINGREF string;
    BOOLEAN negative;
    ULONG base;

    if (Base > 69)
        return FALSE;

    string = *String;
    negative = FALSE;

    if (string.Length != 0 && (string.Buffer[0] == L'-' || string.Buffer[0] == L'+'))
    {
        if (string.Buffer[0] == L'-')
            negative = TRUE;

        PhSkipStringRef(&string, sizeof(WCHAR));
    }

    // If the caller specified a base, don't perform any additional processing.

    if (Base)
    {
        base = Base;
    }
    else
    {
        base = 10;

        if (string.Length >= 2 * sizeof(WCHAR) && string.Buffer[0] == L'0')
        {
            switch (string.Buffer[1])
            {
            case L'x':
            case L'X':
                base = 16;
                break;
            case L'o':
            case L'O':
                base = 8;
                break;
            case L'b':
            case L'B':
                base = 2;
                break;
            case L't': // ternary
            case L'T':
                base = 3;
                break;
            case L'q': // quaternary
            case L'Q':
                base = 4;
                break;
            case L'w': // base 12
            case L'W':
                base = 12;
                break;
            case L'r': // base 32
            case L'R':
                base = 32;
                break;
            }

            if (base != 10)
                PhSkipStringRef(&string, 2 * sizeof(WCHAR));
        }
    }

    valid = PhpStringToInteger64(&string, base, &result);

    if (Integer)
        *Integer = negative ? -(LONG64)result : result;

    return valid;
}

/**
 * Converts a string reference to an unsigned 64-bit integer.
 *
 * \param String Pointer to a PH_STRINGREF structure containing the string to convert.
 * \param Base Optional base for conversion (e.g., 10 for decimal, 16 for hexadecimal).
 * \param Integer Optional pointer to a ULONG64 variable that receives the converted value.
 * \return TRUE if the conversion was successful; otherwise, FALSE.
 */
_Use_decl_annotations_
BOOLEAN PhStringToUInt64(
    _In_ PCPH_STRINGREF String,
    _In_opt_ ULONG Base,
    _Out_opt_ PULONG64 Integer
    )
{
    PH_STRINGREF string;
    ULONG base;

    if (Base > 69)
        return FALSE;

    string = *String;

    if (string.Length != 0 && (string.Buffer[0] == L'-' || string.Buffer[0] == L'+'))
    {
        PhSkipStringRef(&string, sizeof(WCHAR));
    }

    // If the caller specified a base, don't perform any additional processing.

    if (Base)
    {
        base = Base;
    }
    else
    {
        base = 10;

        if (string.Length >= 2 * sizeof(WCHAR) && string.Buffer[0] == L'0')
        {
            switch (string.Buffer[1])
            {
            case L'x':
            case L'X':
                base = 16;
                break;
            case L'o':
            case L'O':
                base = 8;
                break;
            case L'b':
            case L'B':
                base = 2;
                break;
            case L't': // ternary
            case L'T':
                base = 3;
                break;
            case L'q': // quaternary
            case L'Q':
                base = 4;
                break;
            case L'w': // base 12
            case L'W':
                base = 12;
                break;
            case L'r': // base 32
            case L'R':
                base = 32;
                break;
            }

            if (base != 10)
                PhSkipStringRef(&string, 2 * sizeof(WCHAR));
        }
    }

    return PhpStringToInteger64(&string, base, Integer);
}

BOOLEAN PhpStringToDouble(
    _In_ PCPH_STRINGREF String,
    _In_ ULONG Base,
    _Out_ DOUBLE *Double
    )
{
    BOOLEAN valid = TRUE;
    BOOLEAN dotSeen = FALSE;
    DOUBLE result;
    DOUBLE fraction;
    SIZE_T length;
    SIZE_T i;

    length = String->Length / sizeof(WCHAR);
    result = 0;
    fraction = 1;

    for (i = 0; i < length; i++)
    {
        if (String->Buffer[i] == L'.')
        {
            if (!dotSeen)
                dotSeen = TRUE;
            else
                valid = FALSE;
        }
        else
        {
            ULONG value;

            value = PhCharToInteger[(UCHAR)String->Buffer[i]];

            if (value < Base)
            {
                if (!dotSeen)
                {
                    result = result * Base + value;
                }
                else
                {
                    fraction /= Base;
                    result = result + value * fraction;
                }
            }
            else
            {
                valid = FALSE;
            }
        }
    }

    *Double = result;

    return valid;
}

/**
 * Converts a string to a double-precision floating point value.
 *
 * \param String The string to process.
 * \param Base Reserved.
 * \param Double The resulting double value.
 */
BOOLEAN PhStringToDouble(
    _In_ PCPH_STRINGREF String,
    _Reserved_ ULONG Base,
    _Out_opt_ DOUBLE *Double
    )
{
    BOOLEAN valid;
    DOUBLE result;
    PH_STRINGREF string;
    BOOLEAN negative;

    string = *String;
    negative = FALSE;

    if (string.Length != 0 && (string.Buffer[0] == L'-' || string.Buffer[0] == L'+'))
    {
        if (string.Buffer[0] == L'-')
            negative = TRUE;

        PhSkipStringRef(&string, sizeof(WCHAR));
    }

    valid = PhpStringToDouble(&string, 10, &result);

    if (Double)
        *Double = negative ? -result : result;

    return valid;
}

/**
 * Converts an integer to a string.
 *
 * \param Integer The integer to process.
 * \param Base The base which the integer is represented with. The maximum value is 69. The base
 * cannot be 1. If the parameter is 0, the base used is 10.
 * \param Signed TRUE if \a Integer is a signed value, otherwise FALSE.
 *
 * \return The resulting string, or NULL if an error occurred.
 */
PPH_STRING PhIntegerToString64(
    _In_ LONG64 Integer,
    _In_opt_ ULONG Base,
    _In_ BOOLEAN Signed
    )
{
    PH_FORMAT format;

    if (Base == 1 || Base > 69)
        return NULL;

    if (Signed)
        PhInitFormatI64D(&format, Integer);
    else
        PhInitFormatI64U(&format, Integer);

    if (Base != 0)
    {
        format.Type |= FormatUseRadix;
        format.Radix = (UCHAR)Base;
    }

    return PhFormat(&format, 1, 0);
}

/**
 * Formats a time span, specified in ticks, into a human-readable string.
 *
 * \param Destination A pointer to a buffer that receives the formatted time span string.
 * \param Ticks The time span to format, in ticks.
 * \param Mode Optional formatting mode. If specified, determines the output format.
 */
VOID PhPrintTimeSpan(
    _Out_writes_(PH_TIMESPAN_STR_LEN_1) PWSTR Destination,
    _In_ ULONG64 Ticks,
    _In_opt_ ULONG Mode
    )
{
    PhPrintTimeSpanToBuffer(
        Ticks,
        Mode,
        Destination,
        PH_TIMESPAN_STR_LEN_1 * sizeof(WCHAR),
        NULL
        );
}

/**
 * Converts a time span specified in ticks to a human-readable string and writes it to the provided buffer.
 *
 * \param Ticks The time span to print, in ticks (typically 100-nanosecond intervals).
 * \param Mode mode specifying the formatting style. Can be NULL for default formatting.
 * \param Buffer Pointer to the buffer that receives the formatted time span string.
 * \param BufferLength Size of the buffer, in bytes.
 * \param ReturnLength Optional pointer that receives the number of characters written to the buffer (excluding the null terminator).
 * \return TRUE if the time span was successfully formatted and written to the buffer; FALSE otherwise.
 */
BOOLEAN PhPrintTimeSpanToBuffer(
    _In_ ULONG64 Ticks,
    _In_opt_ ULONG Mode,
    _Out_writes_bytes_(BufferLength) PWSTR Buffer,
    _In_ SIZE_T BufferLength,
    _Out_opt_ PSIZE_T ReturnLength
    )
{
    switch (Mode)
    {
    case PH_TIMESPAN_HMSM:
        {
            PH_FORMAT format[7];

            // %02I64u:%02I64u:%02I64u.%03I64u
            PhInitFormatI64UWithWidth(&format[0], PH_TICKS_PARTIAL_HOURS(Ticks), 2);
            PhInitFormatC(&format[1], L':');
            PhInitFormatI64UWithWidth(&format[2], PH_TICKS_PARTIAL_MIN(Ticks), 2);
            PhInitFormatC(&format[3], L':');
            PhInitFormatI64UWithWidth(&format[4], PH_TICKS_PARTIAL_SEC(Ticks), 2);
            PhInitFormatC(&format[5], L'.');
            PhInitFormatI64UWithWidth(&format[6], PH_TICKS_PARTIAL_MS(Ticks), 3);

            return PhFormatToBuffer(format, RTL_NUMBER_OF(format), Buffer, BufferLength, ReturnLength);
        }
        break;
    case PH_TIMESPAN_DHMS:
        {
            PH_FORMAT format[7];

            // %llu:%02I64u:%02I64u:%02I64u
            PhInitFormatI64U(&format[0], PH_TICKS_PARTIAL_DAYS(Ticks));
            PhInitFormatC(&format[1], L':');
            PhInitFormatI64UWithWidth(&format[2], PH_TICKS_PARTIAL_HOURS(Ticks), 2);
            PhInitFormatC(&format[3], L':');
            PhInitFormatI64UWithWidth(&format[4], PH_TICKS_PARTIAL_MIN(Ticks), 2);
            PhInitFormatC(&format[5], L':');
            PhInitFormatI64UWithWidth(&format[6], PH_TICKS_PARTIAL_SEC(Ticks), 2);

            return PhFormatToBuffer(format, RTL_NUMBER_OF(format), Buffer, BufferLength, ReturnLength);
        }
        break;
    case PH_TIMESPAN_DHMSM:
        {
            PH_FORMAT format[9];

            // %llu:%02I64u:%02I64u:%02I64u
            PhInitFormatI64U(&format[0], PH_TICKS_PARTIAL_DAYS(Ticks));
            PhInitFormatC(&format[1], L':');
            PhInitFormatI64UWithWidth(&format[2], PH_TICKS_PARTIAL_HOURS(Ticks), 2);
            PhInitFormatC(&format[3], L':');
            PhInitFormatI64UWithWidth(&format[4], PH_TICKS_PARTIAL_MIN(Ticks), 2);
            PhInitFormatC(&format[5], L':');
            PhInitFormatI64UWithWidth(&format[6], PH_TICKS_PARTIAL_SEC(Ticks), 2);
            PhInitFormatC(&format[7], L':');
            PhInitFormatI64UWithWidth(&format[8], PH_TICKS_PARTIAL_MS(Ticks), 3);

            return PhFormatToBuffer(format, RTL_NUMBER_OF(format), Buffer, BufferLength, ReturnLength);
        }
        break;
    default:
        {
            PH_FORMAT format[5];

            // %02I64u:%02I64u:%02I64u
            PhInitFormatI64UWithWidth(&format[0], PH_TICKS_PARTIAL_HOURS(Ticks), 2);
            PhInitFormatC(&format[1], L':');
            PhInitFormatI64UWithWidth(&format[2], PH_TICKS_PARTIAL_MIN(Ticks), 2);
            PhInitFormatC(&format[3], L'.');
            PhInitFormatI64UWithWidth(&format[4], PH_TICKS_PARTIAL_SEC(Ticks), 2);

            return PhFormatToBuffer(format, RTL_NUMBER_OF(format), Buffer, BufferLength, ReturnLength);
        }
        break;
    }

    return FALSE;
}

/**
 * Calculates the entropy, mean, and variance of a given buffer.
 *
 * \param Buffer Pointer to the buffer containing data to analyze.
 * \param BufferLength Length of the buffer in bytes.
 * \param Entropy Optional pointer to a FLOAT to receive the calculated entropy.
 * \param Mean Optional pointer to a FLOAT to receive the calculated mean.
 * \param Variance Optional pointer to a FLOAT to receive the calculated variance.
 * \return TRUE if the calculation was successful, FALSE otherwise.
 */
BOOLEAN PhCalculateEntropy(
    _In_ PBYTE Buffer,
    _In_ ULONG64 BufferLength,
    _Out_opt_ PFLOAT Entropy,
    _Out_opt_ PFLOAT Mean,
    _Out_opt_ PFLOAT Variance
    )
{
    FLOAT bufferEntropy = 0.f;
    FLOAT bufferMeanValue = 0.f;
    FLOAT bufferVarianceValue = 0.f;
    ULONG64 bufferOffset = 0;
    ULONG64 bufferSumValue = 0;
    ULONG64 counts[UCHAR_MAX + 1];

    memset(counts, 0, sizeof(counts));

    while (bufferOffset < BufferLength)
    {
        BYTE value = *(PBYTE)PTR_ADD_OFFSET(Buffer, bufferOffset++);

        bufferSumValue += value;
        counts[value]++;
    }

    // Calculate entropy
    for (ULONG i = 0; i < RTL_NUMBER_OF(counts); i++)
    {
        FLOAT value = (FLOAT)counts[i] / (FLOAT)BufferLength;

        if (value > 0.f)
            bufferEntropy -= value * log2f(value);
    }

    bufferMeanValue = (FLOAT)bufferSumValue / (FLOAT)BufferLength;

    // Calculate variance
    if (BufferLength > 0)
    {
        for (ULONG i = 0; i < RTL_NUMBER_OF(counts); i++)
        {
            FLOAT diff = (FLOAT)i - bufferMeanValue;
            bufferVarianceValue += counts[i] * diff * diff;
        }
        bufferVarianceValue /= (FLOAT)BufferLength;
    }

    if (Entropy)
        *Entropy = bufferEntropy;
    if (Mean)
        *Mean = bufferMeanValue;
    if (Variance)
        *Variance = bufferVarianceValue;

    return TRUE;
}

/**
 * Formats entropy, mean, and variance values into a string representation with specified precision.
 *
 * \param Entropy The entropy value to format.
 * \param EntropyPrecision The number of decimal places for the entropy value.
 * \param Mean (Optional) The mean value to format.
 * \param MeanPrecision (Optional) The number of decimal places for the mean value.
 * \param Variance (Optional) The variance value to format.
 * \param VariancePrecision (Optional) The number of decimal places for the variance value.
 * \return A pointer to a PPH_STRING containing the formatted string.
 */
PPH_STRING PhFormatEntropy(
    _In_ FLOAT Entropy,
    _In_ USHORT EntropyPrecision,
    _In_opt_ FLOAT Mean,
    _In_opt_ USHORT MeanPrecision,
    _In_opt_ FLOAT Variance,
    _In_opt_ USHORT VariancePrecision
    )
{
    if (Mean && Variance)
    {
        PH_FORMAT format[6];

        // %s S (%s X)
        format[0].Type = SingleFormatType | FormatUsePrecision | FormatCropZeros;
        format[0].u.Single = Entropy;
        format[0].Precision = EntropyPrecision;
        PhInitFormatS(&format[1], L" S (");
        format[2].Type = SingleFormatType | FormatUsePrecision | FormatCropZeros;
        format[2].u.Single = Mean;
        format[2].Precision = MeanPrecision;
        PhInitFormatS(&format[3], L" M) (");
        format[4].Type = SingleFormatType | FormatUsePrecision | FormatCropZeros;
        format[4].u.Single = Variance;
        format[4].Precision = VariancePrecision;
        PhInitFormatS(&format[5], L" X)");

        return PhFormat(format, ARRAYSIZE(format), 0);
    }
    else if (Variance)
    {
        PH_FORMAT format[4];

        // %s S (%s X)
        format[0].Type = SingleFormatType | FormatUsePrecision | FormatCropZeros;
        format[0].u.Single = Entropy;
        format[0].Precision = EntropyPrecision;
        PhInitFormatS(&format[1], L" S (");
        format[2].Type = SingleFormatType | FormatUsePrecision | FormatCropZeros;
        format[2].u.Single = Variance;
        format[2].Precision = VariancePrecision;
        PhInitFormatS(&format[3], L" X)");

        return PhFormat(format, ARRAYSIZE(format), 0);
    }
    else
    {
        PH_FORMAT format;

        format.Type = SingleFormatType | FormatUsePrecision | FormatCropZeros;
        format.u.Single = Entropy;
        format.Precision = EntropyPrecision;

        return PhFormat(&format, 1, 0);
    }
}

/**
 * Fills a memory block with a ULONG pattern.
 *
 * \param Memory The memory block. The block must be 4 byte aligned.
 * \param Value The ULONG pattern.
 * \param Count The number of elements.
 */
VOID PhFillMemoryUlongOriginal(
    _Inout_updates_(Count) PULONG Memory,
    _In_ ULONG Value,
    _In_ SIZE_T Count
    )
{
    PH_INT128 pattern;
    SIZE_T count;

    if (!PhHasIntrinsics)
    {
        if (Count != 0)
        {
            do
            {
                *Memory++ = Value;
            } while (--Count != 0);
        }

        return;
    }

    if ((ULONG_PTR)Memory & 0xf)
    {
        switch ((ULONG_PTR)Memory & 0xf)
        {
        case 0x4:
            if (Count >= 1)
            {
                *Memory++ = Value;
                Count--;
            }
            __fallthrough;
        case 0x8:
            if (Count >= 1)
            {
                *Memory++ = Value;
                Count--;
            }
            __fallthrough;
        case 0xc:
            if (Count >= 1)
            {
                *Memory++ = Value;
                Count--;
            }
            break;
        }
    }

    pattern = PhSetINT128by32(Value);
    count = Count / 4;

    if (count != 0)
    {
        do
        {
            PhStoreINT128((PLONG)Memory, pattern);
            Memory += 4;
        } while (--count != 0);
    }

    switch (Count & 0x3)
    {
    case 0x3:
        *Memory++ = Value;
        __fallthrough;
    case 0x2:
        *Memory++ = Value;
        __fallthrough;
    case 0x1:
        *Memory++ = Value;
        break;
    }
}

/**
 * Fills a memory block with a ULONG pattern.
 *
 * \param Memory The memory block. The block must be 4 byte aligned.
 * \param Value The ULONG pattern.
 * \param Count The number of elements.
 */
VOID PhFillMemoryUlong(
    _Inout_updates_(Count) PULONG Memory,
    _In_ ULONG Value,
    _In_ SIZE_T Count
    )
{
    if (Count == 0)
        return;

#ifndef _ARM64_
    if (PhHasAVX && IS_ALIGNED(Memory, 32))
    {
        SIZE_T count = Count & ~0x7;

        if (count != 0)
        {
            PULONG end;
            __m256i pattern;

            end = (PULONG)(ULONG_PTR)(Memory + count);
            pattern = _mm256_set1_epi32(Value);

            while (Memory != end)
            {
                _mm256_store_si256((__m256i*)Memory, pattern);
                Memory += 8;
            }

            Count &= 0x7;
        }
    }
#endif

    if (PhHasIntrinsics && IS_ALIGNED(Memory, 16))
    {
        SIZE_T count = Count & ~0x3;

        if (count != 0)
        {
            PULONG end;
            PH_INT128 pattern;

            end = (PULONG)(ULONG_PTR)(Memory + count);
            pattern = PhSetINT128by32(Value);

            while (Memory != end)
            {
                PhStoreINT128((PLONG)Memory, pattern);
                Memory += 4;
            }

            Count &= 0x3;
        }
    }

    while (Count-- != 0)
    {
        *Memory++ = Value;
    }
}

/**
 * Divides an array of numbers by a number.
 *
 * \param A The destination array, divided by \a B.
 * \param B The number.
 * \param Count The number of elements.
 */
VOID PhDivideSinglesBySingleOriginal(
    _Inout_updates_(Count) PFLOAT A,
    _In_ FLOAT B,
    _In_ SIZE_T Count
    )
{
    PFLOAT endA;
    PH_FLOAT128 b;

    if (!PhHasIntrinsics)
    {
        while (Count--)
            *A++ /= B;

        return;
    }

    if ((ULONG_PTR)A & 0xf)
    {
        switch ((ULONG_PTR)A & 0xf)
        {
        case 0x4:
            if (Count >= 1)
            {
                *A++ /= B;
                Count--;
            }
            __fallthrough;
        case 0x8:
            if (Count >= 1)
            {
                *A++ /= B;
                Count--;
            }
            __fallthrough;
        case 0xc:
            if (Count >= 1)
            {
                *A++ /= B;
                Count--;
            }
            else
            {
                return; // essential; A may not be aligned properly
            }
            break;
        }
    }

    endA = (PFLOAT)((ULONG_PTR)(A + Count) & ~0xf);
    b = PhSetFLOAT128by32(B);

    while (A != endA)
    {
        PH_FLOAT128 a;

        a = PhLoadFLOAT128(A);
        a = PhDivideFLOAT128(a, b);
        PhStoreFLOAT128(A, a);

        A += 4;
    }

    switch (Count & 0x3)
    {
    case 0x3:
        *A++ /= B;
        __fallthrough;
    case 0x2:
        *A++ /= B;
        __fallthrough;
    case 0x1:
        *A++ /= B;
        break;
    }
}

/**
 * Divides an array of numbers by a number.
 *
 * \param A The destination array, divided by \a B.
 * \param B The number.
 * \param Count The number of elements.
 */
VOID PhDivideSinglesBySingle(
    _Inout_updates_(Count) PFLOAT A,
    _In_ FLOAT B,
    _In_ SIZE_T Count
    )
{
    if (Count == 0)
        return;
    if (B == 1.0f || B == 0.0f)
        return;

    // Note: This uses reciprocal multiply since it's faster than per-element divides
    // and preserves IEEE-754 behavior for +/-0, +/-INF, and NaN (0/0 -> NaN, x/0 -> +/-INF). (dmex)
    const FLOAT invB = 1.0f / B;

#ifndef _ARM64_
    if (PhHasAVX && IS_ALIGNED(A, 32))
    {
        SIZE_T count = Count & ~0x7;

        if (count != 0)
        {
            PFLOAT end;
            __m256 a;
            __m256 b;

            end = (PFLOAT)(ULONG_PTR)(A + count);
            b = _mm256_set1_ps(invB); // _mm256_broadcast_ss(&B);

            while (A != end)
            {
                a = _mm256_load_ps(A);
                a = _mm256_mul_ps(a, b); // _mm256_div_ps(a, b);
                _mm256_store_ps(A, a);

                A += 8;
            }

            Count &= 0x7;
        }
    }
#endif

    if (PhHasIntrinsics && IS_ALIGNED(A, 16))
    {
        SIZE_T count = Count & ~0x3;

        if (count != 0)
        {
            PFLOAT end;
            PH_FLOAT128 a;
            PH_FLOAT128 b;

            end = A + count;
            b = PhSetFLOAT128by32(invB); // PhSetFLOAT128by32(B);

            while (A != end)
            {
                a = PhLoadFLOAT128(A);
                a = PhMultiplyFLOAT128(a, b); // PhDivideFLOAT128(a, b);
                PhStoreFLOAT128(A, a);

                A += 4;
            }

            Count &= 0x3;
        }
    }

    while (Count--)
    {
        *A++ *= invB;
    }
}

/**
 * Adds one array of integers to another.
 *
 * \param A The destination array to which the source
 * array is added. The array must be 16 byte aligned.
 * \param B The source array. The array must be 16
 * byte aligned.
 * \param Count The number of elements.
 */
VOID PhAddMemoryUlongOriginal(
    _Inout_ PULONG A,
    _In_ PULONG B,
    _In_ ULONG Count
    )
{
#ifndef _ARM64_
    if (PhHasIntrinsics)
    {
        while (Count >= 4)
        {
            __m128i a;
            __m128i b;

            a = _mm_load_si128((__m128i*)A);
            b = _mm_load_si128((__m128i*)B);
            a = _mm_add_epi32(a, b);
            _mm_store_si128((__m128i*)A, a);

            A += 4;
            B += 4;
            Count -= 4;
        }

        switch (Count & 0x3)
        {
        case 0x3:
            *A++ += *B++;
            __fallthrough;
        case 0x2:
            *A++ += *B++;
            __fallthrough;
        case 0x1:
            *A++ += *B++;
            break;
        }
    }
    else
#endif
    {
        while (Count--)
            *A++ += *B++;
    }
}

/**
 * Returns the maximum value of an array of floats.
 *
 * \param A The array.
 * \param Count The total number of array elements.
 * \return The maximum of any single element.
 */
FLOAT PhMaxMemorySingles(
    _In_ PFLOAT A,
    _In_ SIZE_T Count
    )
{
    FLOAT maximum = 0.0f;

    if (Count == 0)
        return maximum;

#ifndef _ARM64_
    if (PhHasAVX && IS_ALIGNED(A, 32))
    {
        SIZE_T count = Count & ~0x7;

        if (count != 0)
        {
            PFLOAT end;
            __m256 a;
            __m256 c;
            __m128 hi;
            __m128 lo;
            __m128 d;

            end = (PFLOAT)(ULONG_PTR)(A + count);
            c = _mm256_setzero_ps();

            while (A != end)
            {
                a = _mm256_load_ps(A);
                c = _mm256_max_ps(c, a);

                A += 8;
            }

            c = _mm256_max_ps(c, _mm256_permute_ps(c, _MM_SHUFFLE(2, 3, 0, 1)));  // swap neighbors
            c = _mm256_max_ps(c, _mm256_permute_ps(c, _MM_SHUFFLE(1, 0, 3, 2)));  // swap pairs

            lo = _mm256_castps256_ps128(c);
            hi = _mm256_extractf128_ps(c, 1);
            d = _mm_max_ps(lo, hi);

            d = _mm_max_ps(d, _mm_shuffle_ps(d, d, _MM_SHUFFLE(2, 3, 0, 1)));
            d = _mm_max_ps(d, _mm_shuffle_ps(d, d, _MM_SHUFFLE(1, 0, 3, 2)));
            maximum = _mm_cvtss_f32(d);

            Count &= 0x7;
        }
    }
#endif

    if (PhHasIntrinsics && IS_ALIGNED(A, 16))
    {
        SIZE_T count = Count & ~0x3;

        if (count != 0)
        {
            FLOAT value;
            PFLOAT end;
            PH_FLOAT128 a;
            PH_FLOAT128 c;

            end = (PFLOAT)(ULONG_PTR)(A + count);
            c = PhZeroFLOAT128();

            while (A != end)
            {
                a = PhLoadFLOAT128(A);
                c = PhMaxFLOAT128(c, a);

                A += 4;
            }

            // Compare adjacent pairs (swap elements within pairs)
            c = PhMaxFLOAT128(c, PhShuffleFLOAT128_2301(c, c));
            // Compare results (swap pairs themselves)
            c = PhMaxFLOAT128(c, PhShuffleFLOAT128_1032(c, c));
            PhStoreFLOAT128LowSingle(&value, c);

            if (maximum < value)
                maximum = value;

            Count &= 0x3;
        }
    }

    while (Count--)
    {
        FLOAT value = *A++;

        if (maximum < value)
            maximum = value;
    }

    return maximum;
}

/**
 * Adds one array of floats to another and returns the maximum.
 *
 * \param A The first array.
 * \param B The second array.
 * \param Count The total number of array elements.
 * \return The maximum of any single element.
 */
FLOAT PhAddPlusMaxMemorySingles(
    _Inout_updates_(Count) PFLOAT A,
    _Inout_updates_(Count) PFLOAT B,
    _In_ SIZE_T Count
    )
{
    FLOAT maximum = 0.0f;

    if (Count == 0)
        return maximum;

#ifndef _ARM64_
    if (PhHasAVX && (IS_ALIGNED(A, 32) && IS_ALIGNED(B, 32)))
    {
        SIZE_T count = Count & ~0x7;

        if (count != 0)
        {
            PFLOAT end;
            __m256 a;
            __m256 b;
            __m256 c;
            __m128 lo;
            __m128 hi;
            __m128 d;

            end = (PFLOAT)(ULONG_PTR)(A + count);
            c = _mm256_setzero_ps();

            while (A != end)
            {
                a = _mm256_load_ps(A);
                b = _mm256_load_ps(B);
                a = _mm256_add_ps(b, a);
                c = _mm256_max_ps(c, a);

                A += 8;
                B += 8;
            }

            c = _mm256_max_ps(c, _mm256_permute_ps(c, _MM_SHUFFLE(2, 3, 0, 1)));
            c = _mm256_max_ps(c, _mm256_permute_ps(c, _MM_SHUFFLE(1, 0, 3, 2)));

            lo = _mm256_castps256_ps128(c);
            hi = _mm256_extractf128_ps(c, 1);
            d = _mm_max_ps(lo, hi);

            d = _mm_max_ps(d, _mm_shuffle_ps(d, d, _MM_SHUFFLE(2, 3, 0, 1)));
            d = _mm_max_ps(d, _mm_shuffle_ps(d, d, _MM_SHUFFLE(1, 0, 3, 2)));
            maximum = _mm_cvtss_f32(d);

            Count &= 0x7;
        }
    }
#endif

    if (PhHasIntrinsics && (IS_ALIGNED(A, 16) && IS_ALIGNED(B, 16)))
    {
        SIZE_T count = Count & ~0x3;

        if (count != 0)
        {
            FLOAT value;
            PFLOAT end;
            PH_FLOAT128 a;
            PH_FLOAT128 b;
            PH_FLOAT128 c;

            end = (PFLOAT)(ULONG_PTR)(A + count);
            c = PhZeroFLOAT128();

            while (A != end)
            {
                a = PhLoadFLOAT128(A);
                b = PhLoadFLOAT128(B);
                a = PhAddFLOAT128(b, a);
                c = PhMaxFLOAT128(c, a);

                A += 4;
                B += 4;
            }

            c = PhMaxFLOAT128(c, PhShuffleFLOAT128_2301(c, c));  // swap neighbors
            c = PhMaxFLOAT128(c, PhShuffleFLOAT128_1032(c, c));  // swap pairs
            PhStoreFLOAT128LowSingle(&value, c);

            if (maximum < value)
                maximum = value;

            Count &= 0x3;
        }
    }

    while (Count--)
    {
        FLOAT value = *A++ + *B++;

        if (maximum < value)
            maximum = value;
    }

    return maximum;
}

/**
 * Converts an array of integers to floats.
 *
 * \param From The source integers.
 * \param To The destination floats.
 * \param Count The number of elements.
 */
VOID PhConvertCopyMemoryUlong(
    _Inout_updates_(Count) PULONG From,
    _Inout_updates_(Count) PFLOAT To,
    _In_ SIZE_T Count
    )
{
    if (Count == 0)
        return;

#ifndef _ARM64_
    if (PhHasAVX && IS_ALIGNED(From, 32) && IS_ALIGNED(To, 32))
    {
        SIZE_T count = Count & ~0x7;

        if (count != 0)
        {
            PULONG end;
            __m256i a;
            __m256 b;

            end = (PULONG)(ULONG_PTR)(From + count);

            while (From != end)
            {
                a = _mm256_load_si256((__m256i const*)From);
                b = _mm256_cvtf_epu32(a); // _mm256_cvtepi32_ps
                _mm256_store_ps(To, b);

                From += 8;
                To += 8;
            }

            Count &= 0x7;
        }
    }
#endif

    if (PhHasIntrinsics && IS_ALIGNED(From, 16) && IS_ALIGNED(To, 16))
    {
        SIZE_T count = Count & ~0x3;

        if (count != 0)
        {
            PULONG end;
            PH_INT128 a;
            PH_FLOAT128 b;

            end = (PULONG)(ULONG_PTR)(From + count);

            while (From != end)
            {
                a = PhLoadINT128(From); // _mm_loadl_epi64
                b = PhConvertUINT128ToFLOAT128(a); // _mm_cvtepi32_ps // _mm_cvtepu32_ps
                PhStoreFLOAT128(To, b);

                From += 4;
                To += 4;
            }

            Count &= 0x3;
        }
    }

    while (Count--)
    {
        *To++ = (FLOAT)*From++;
    }
}

/**
 * Converts an array of 64-bit unsigned integers to floats.
 *
 * \param From The source 64-bit integers.
 * \param To The destination floats.
 * \param Count The number of elements.
 */
VOID PhConvertCopyMemoryUlong64(
    _Inout_updates_(Count) PULONG64 From,
    _Inout_updates_(Count) PFLOAT To,
    _In_ SIZE_T Count
    )
{
    if (Count == 0)
        return;

#ifndef _ARM64_
#if defined(PH_NATIVE_AVX512)
    if (PhHasAVX512)
    {
        SIZE_T count = Count & ~0xF;
    
        if (count)
        {
            PULONG64 end = From + count;
    
            // Convert 16 × uint64 → 16 × float in single iteration
            while (From != end)
            {
                // Load 16 × uint64 (2 × 512-bit loads = 128 bytes)
                __m512i v0 = _mm512_load_si512((__m512i const*)From);       // 8 × uint64
                __m512i v1 = _mm512_load_si512((__m512i const*)(From + 8)); // 8 × uint64
    
                // Direct uint64 → double conversion (AVX-512 DQ)
                __m512d d0 = _mm512_cvtepu64_pd(v0);  // 8 × double
                __m512d d1 = _mm512_cvtepu64_pd(v1);  // 8 × double
    
                // Double → float conversion with rounding
                __m256 f0 = _mm512_cvtpd_ps(d0);  // 8 × float
                __m256 f1 = _mm512_cvtpd_ps(d1);  // 8 × float
    
                // Combine into single 512-bit register
                __m512 result = _mm512_insertf32x8(_mm512_castps256_ps512(f0), f1, 1);
    
                // Store 16 floats
                _mm512_storeu_ps(To, result);
    
                From += 16;
                To += 16;
            }
    
            Count &= 0xF;
        }
    }
#endif

    if (PhHasAVX && IS_ALIGNED(From, 32) && IS_ALIGNED(To, 32))
    {
        SIZE_T count = Count & ~0x7;

        if (count)
        {
            PULONG64 end = From + count;
            // Constant for reconstructing float from high/low 32-bit parts.
            const __m256 ps2p32 = _mm256_set1_ps(4294967296.0f); // 2^32
            // Mask to de-interleave low and high 32-bit words from 64-bit lanes.
            const __m256i deinterleave_mask = _mm256_setr_epi32(0, 2, 4, 6, 1, 3, 5, 7);

            while (From != end)
            {
                // Load 8 uint64_t values into two 256-bit registers.
                __m256i v0 = _mm256_load_si256((__m256i const*)From);       // u0, u1, u2, u3
                __m256i v1 = _mm256_load_si256((__m256i const*)(From + 4)); // u4, u5, u6, u7

                // Within each register, shuffle to group low 32-bit parts and high 32-bit parts.
                // shuf0 becomes [u0_lo, u1_lo, u2_lo, u3_lo, u0_hi, u1_hi, u2_hi, u3_hi]
                __m256i shuf0 = _mm256_permutevar8x32_epi32(v0, deinterleave_mask);
                // shuf1 becomes [u4_lo, u5_lo, u6_lo, u7_lo, u4_hi, u5_hi, u6_hi, u7_hi]
                __m256i shuf1 = _mm256_permutevar8x32_epi32(v1, deinterleave_mask);

                // Combine the parts from both registers to get two vectors:
                // one with all 8 low parts, and one with all 8 high parts.
                __m256i all_los = _mm256_permute2x128_si256(shuf0, shuf1, 0x20); // combines low(shuf0), low(shuf1)
                __m256i all_his = _mm256_permute2x128_si256(shuf0, shuf1, 0x31); // combines high(shuf0), high(shuf1)

                // Convert the two vectors of 8 unsigned 32-bit integers to floats
                // using the helper from phintrin.h.
                __m256 los_ps = _mm256_cvtf_epu32(all_los);
                __m256 his_ps = _mm256_cvtf_epu32(all_his);

                // Reconstruct the float representation of the original uint64_t values.
                // float(u64) approx= float(low32) + float(high32) * 2^32
                // FMA (fused multiply-add) is used for better performance.
                __m256 val = _mm256_fmadd_ps(his_ps, ps2p32, los_ps);

                // Store the 8 resulting floats.
                _mm256_storeu_ps(To, val);

                From += 8;
                To += 8;
            }

            Count &= 0x7;
        }
    }

//#ifndef _ARM64_
//    if(PhHasAVX)
//    {
//        SIZE_T count = Count & ~0x3;
//
//        if (count)
//        {
//            PULONG64 end = From + count;
//            const __m256i MaskLo32_64 = _mm256_set1_epi64x(0xFFFFFFFFULL); // for AND on 64-bit lanes
//            const __m256i PackIdx = _mm256_setr_epi32(0, 2, 4, 6, 0, 0, 0, 0);  // take elements 0,2,4,6 into lower 128
//            const __m128  Ps2p31 = _mm_set1_ps(2147483648.0f); // 2^31
//            const __m128  Ps2p32 = _mm_set1_ps(4294967296.0f); // 2^32 (exact power of two)
//            const __m128i Mask7fffffff = _mm_set1_epi32(0x7FFFFFFF);
//
//            while (From != end)
//            {
//                // Load 4x uint64_t (256 bits)
//                __m256i v = _mm256_loadu_si256((__m256i const*)From);
//
//                // Split each 64-bit lane into its low/high 32-bit halves (still in 64-bit lanes)
//                __m256i lo32_64 = _mm256_and_si256(v, MaskLo32_64);   // [u0_lo,0, u1_lo,0, u2_lo,0, u3_lo,0]
//                __m256i hi32_64 = _mm256_srli_epi64(v, 32);           // [u0_hi,0, u1_hi,0, u2_hi,0, u3_hi,0]
//
//                // Pack 32-bit elements (indices 0,2,4,6) into lower 128 as contiguous 4x int32
//                __m256i loPacked = _mm256_permutevar8x32_epi32(lo32_64, PackIdx);
//                __m256i hiPacked = _mm256_permutevar8x32_epi32(hi32_64, PackIdx);
//
//                __m128i lo128 = _mm256_castsi256_si128(loPacked); // [u0_lo, u1_lo, u2_lo, u3_lo]
//                __m128i hi128 = _mm256_castsi256_si128(hiPacked); // [u0_hi, u1_hi, u2_hi, u3_hi]
//
//                // Convert unsigned 32 -> float:
//                //   f = float(x & 0x7fffffff) + float(x >> 31) * 2^31
//                __m128i loCarryI = _mm_srli_epi32(lo128, 31);
//                __m128  loPs = _mm_cvtepi32_ps(_mm_and_si128(lo128, Mask7fffffff));
//                loPs = _mm_add_ps(loPs, _mm_mul_ps(_mm_cvtepi32_ps(loCarryI), Ps2p31));
//
//                __m128i hiCarryI = _mm_srli_epi32(hi128, 31);
//                __m128  hiPs = _mm_cvtepi32_ps(_mm_and_si128(hi128, Mask7fffffff));
//                hiPs = _mm_add_ps(hiPs, _mm_mul_ps(_mm_cvtepi32_ps(hiCarryI), Ps2p31));
//
//                // Reconstruct: float(u64) = float(low32) + float(high32) * 2^32
//                // (Use FMA if you dispatch it)
//                __m128 val = _mm_add_ps(loPs, _mm_mul_ps(hiPs, Ps2p32));
//
//                _mm_storeu_ps(To, val); // store exactly 4 floats
//
//                From += 4;
//                To   += 4;
//            }
//
//            Count &= 0x3;
//        }
//    }
//#endif

    if (PhHasIntrinsics && IS_ALIGNED(From, 16) && IS_ALIGNED(To, 16))
    {
        SIZE_T count = Count & ~0x3;  // Process 4 at a time

        if (count)
        {
            PULONG64 end = From + count;
            const __m128d scale = _mm_set1_pd(4294967296.0);

            while (From != end)
            {
                // Load 4 × uint64 as 2 × __m128i
                __m128i v0 = _mm_load_si128((__m128i const*)From);       // 2 × uint64
                __m128i v1 = _mm_load_si128((__m128i const*)(From + 2)); // 2 × uint64

                // Split low/high 32-bit
                __m128i mask = _mm_set1_epi64x(0xFFFFFFFFULL);
                __m128i v0_lo = _mm_and_si128(v0, mask);
                __m128i v0_hi = _mm_srli_epi64(v0, 32);
                __m128i v1_lo = _mm_and_si128(v1, mask);
                __m128i v1_hi = _mm_srli_epi64(v1, 32);

                // Convert uint32 → double (exact)
                __m128d d0_lo = _mm_cvtepi32_pd(_mm_shuffle_epi32(v0_lo, 0x08));
                __m128d d0_hi = _mm_cvtepi32_pd(_mm_shuffle_epi32(v0_hi, 0x08));
                __m128d d1_lo = _mm_cvtepi32_pd(_mm_shuffle_epi32(v1_lo, 0x08));
                __m128d d1_hi = _mm_cvtepi32_pd(_mm_shuffle_epi32(v1_hi, 0x08));

                // Reconstruct
                __m128d d0 = _mm_add_pd(d0_lo, _mm_mul_pd(d0_hi, scale));
                __m128d d1 = _mm_add_pd(d1_lo, _mm_mul_pd(d1_hi, scale));

                // Double → float
                __m128 f0 = _mm_cvtpd_ps(d0);  // 2 floats in low 64 bits
                __m128 f1 = _mm_cvtpd_ps(d1);  // 2 floats in low 64 bits

                // Combine into 4 floats
                __m128 result = _mm_movelh_ps(f0, f1);

                // Store 4 floats
                _mm_storeu_ps(To, result);

                From += 4;
                To += 4;
            }

            Count &= 0x3;
        }
    }
#endif

    while (Count--)
    {
        *To++ = (FLOAT)*From++;
    }
}

/**
 * Converts an array of floats to integers.
 *
 * \param From The source floats.
 * \param To The destination integers.
 * \param Count The number of elements.
 */
VOID PhConvertCopyMemorySingles(
    _Inout_updates_(Count) PFLOAT From,
    _Inout_updates_(Count) PULONG To,
    _In_ SIZE_T Count
    )
{
    if (Count == 0)
        return;

#ifndef _ARM64_
    if (PhHasAVX && IS_ALIGNED(From, 32) && IS_ALIGNED(To, 32))
    {
        SIZE_T count = Count & ~0x7;

        if (count != 0)
        {
            PFLOAT end = From + count;

            __m256 a;
            __m256i b;

            while (From != end)
            {
                a = _mm256_load_ps(From);
                // Truncate toward zero to match scalar (C cast) semantics.
                b = _mm256_cvttps_epi32(a); // _mm256_cvtps_epi32 // _mm256_cvtps_epu32
                _mm256_store_si256((__m256i*)To, b);

                From += 8;
                To += 8;
            }

            Count &= 0x7;
        }
    }
#endif

    if (PhHasIntrinsics && IS_ALIGNED(From, 16) && IS_ALIGNED(To, 16))
    {
        SIZE_T count = Count & ~0x3;

        if (count != 0)
        {
            PFLOAT end;
            PH_FLOAT128 a;
            PH_INT128 b;

            end = (PFLOAT)(ULONG_PTR)(From + count);

            while (From != end)
            {
                a = PhLoadFLOAT128(From);
                b = PhConvertFLOAT128ToUINT128(a);
                PhStoreINT128(To, b);

                From += 4;
                To += 4;
            }

            Count &= 0x3;
        }
    }

    while (Count--)
    {
        *To++ = (ULONG)*From++;
    }
}

// based on PhCopyCircularBuffer (FLOAT) (\phlib\circbuf_i.h) (dmex)
VOID PhCopyConvertCircularBufferULONG(
    _Inout_ PPH_CIRCULAR_BUFFER_ULONG Buffer,
    _Out_writes_(Count) FLOAT* Destination,
    _In_ ULONG Count
    )
{
    ULONG tailSize;
    ULONG headSize;

    tailSize = (ULONG)(Buffer->Size - Buffer->Index);
    headSize = Buffer->Count - tailSize;

    if (Count > Buffer->Count)
        Count = Buffer->Count;

    if (tailSize >= Count)
    {
        // Convert and copy only a part of the tail.
        PhConvertCopyMemoryUlong(&Buffer->Data[Buffer->Index], Destination, Count);
    }
    else
    {
        // Convert and copy the tail, then only part of the head.
        PhConvertCopyMemoryUlong(&Buffer->Data[Buffer->Index], Destination, tailSize);
        PhConvertCopyMemoryUlong(Buffer->Data, &Destination[tailSize], (Count - tailSize));
    }
}

VOID PhCopyConvertCircularBufferULONG64(
    _Inout_ PPH_CIRCULAR_BUFFER_ULONG64 Buffer,
    _Out_writes_(Count) FLOAT* Destination,
    _In_ ULONG Count
    )
{
    ULONG tailSize;
    ULONG headSize;

    tailSize = (ULONG)(Buffer->Size - Buffer->Index);
    headSize = Buffer->Count - tailSize;

    if (Count > Buffer->Count)
        Count = Buffer->Count;

    if (tailSize >= Count)
    {
        // Convert and copy only a part of the tail.
        PhConvertCopyMemoryUlong64(&Buffer->Data[Buffer->Index], Destination, Count);
    }
    else
    {
        // Convert and copy the tail, then only part of the head.
        PhConvertCopyMemoryUlong64(&Buffer->Data[Buffer->Index], Destination, tailSize);
        PhConvertCopyMemoryUlong64(Buffer->Data, &Destination[tailSize], (Count - tailSize));
    }
}

/**
 * Counts the number of set bits (1s) in the given 32-bit unsigned integer value.
 *
 * \param Value The 32-bit unsigned integer whose bits are to be counted.
 * \return The number of bits set to 1 in the input value.
 */
ULONG PhCountBits(
    _In_ ULONG Value
    )
{
    if (PhHasPopulationCount)
    {
        return PhPopulationCount32(Value);
    }
    else
    {
        #undef T
        #define T ULONG
        ULONG count;

        // http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
        Value = Value - ((Value >> 1) & (T)~(T)0 / 3);
        Value = (Value & (T)~(T)0 / 15 * 3) + ((Value >> 2) & (T)~(T)0 / 15 * 3);
        Value = (Value + (Value >> 4)) & (T)~(T)0 / 255 * 15;
        count = (T)(Value * ((T)~(T)0 / 255)) >> (sizeof(T) - 1) * CHAR_BIT;

        return count;

        //ULONG count = 0;
        //
        //while (Value)
        //{
        //    count++;
        //    Value &= Value - 1;
        //}
        //
        //return count;
    }
}

/**
 * Counts the number of set bits (1s) in the specified ULONG_PTR value.
 *
 * \param Value The ULONG_PTR value whose bits are to be counted.
 * \return The number of bits set to 1 in the input value.
 */
ULONG PhCountBitsUlongPtr(
    _In_ ULONG_PTR Value
    )
{
#if defined(PH_NATIVE_COUNTBITS)
    return RtlNumberOfSetBitsUlongPtr(Value);
#else
#ifdef _WIN64
    if (PhHasPopulationCount)
    {
        return (ULONG)PhPopulationCount64(Value);
    }
    else
#endif
    {
        #undef T
        #define T ULONG
        ULONG count;

        // http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
        Value = Value - ((Value >> 1) & (T)~(T)0 / 3);
        Value = (Value & (T)~(T)0 / 15 * 3) + ((Value >> 2) & (T)~(T)0 / 15 * 3);
        Value = (Value + (Value >> 4)) & (T)~(T)0 / 255 * 15;
        count = (T)(Value * ((T)~(T)0 / 255)) >> (sizeof(T) - 1) * CHAR_BIT;

        return count;

        //ULONG count = 0;
        //
        //while (Value)
        //{
        //    count++;
        //    Value &= Value - 1;
        //}
        //
        //return count;
    }
#endif
}

#pragma region Thread Local Storage (TLS)

/**
 * Allocates a new TLS (Thread Local Storage) index.
 *
 * \return Returns the allocated TLS index as an ULONG value.
 */
ULONG PhTlsAlloc(
    VOID
    )
{
    if (WindowsVersion < WINDOWS_NEW)
    {
        PTEB currentTeb;
        PPEB currentPeb;
        ULONG i;

        currentTeb = NtCurrentTeb();
        currentPeb = currentTeb->ProcessEnvironmentBlock;
        RtlAcquirePebLock();

        for (
            i = RtlFindClearBitsAndSet(currentPeb->TlsBitmap, 1, 0);
            ;
            i = RtlFindClearBitsAndSet(currentPeb->TlsBitmap, 1, 0)
            )
        {
            if (i != ULONG_MAX)
            {
                RtlReleasePebLock();
                currentTeb->TlsSlots[i] = NULL;
                return i;
            }

            if (currentTeb->TlsExpansionSlots)
                break;

            RtlReleasePebLock();
            currentTeb->TlsExpansionSlots = (PVOID*)RtlAllocateHeap(RtlProcessHeap(), HEAP_ZERO_MEMORY, TLS_EXPANSION_SLOTS * sizeof(PVOID));
            if (!currentTeb->TlsExpansionSlots) goto CleanupExit;
            RtlAcquirePebLock();
        }

        i = RtlFindClearBitsAndSet(currentPeb->TlsExpansionBitmap, 1, 0);
        RtlReleasePebLock();

        if (i != ULONG_MAX)
        {
            currentTeb->TlsExpansionSlots[i] = NULL;
            return i + TLS_MINIMUM_AVAILABLE;
        }
    }

CleanupExit:
    //RtlSetLastWin32ErrorAndNtStatusFromNtStatus(STATUS_NO_MEMORY);
    //return ULONG_MAX;
    return TlsAlloc();
}

/**
 * Frees a thread-local storage (TLS) slot previously allocated.
 *
 * \param Index The index of the TLS slot to be freed.
 * \return Returns an NTSTATUS code indicating success or failure of the operation.
 */
NTSTATUS PhTlsFree(
    _In_ ULONG Index
    )
{
    NTSTATUS status;

    if (WindowsVersion < WINDOWS_NEW)
    {
        PTEB currentTeb;
        PPEB currentPeb;
        PRTL_BITMAP bitmap;
        ULONG i;

        currentTeb = NtCurrentTeb();
        currentPeb = currentTeb->ProcessEnvironmentBlock;

        if (Index >= TLS_MINIMUM_AVAILABLE)
        {
            i = Index - TLS_MINIMUM_AVAILABLE;

            if (i >= TLS_EXPANSION_SLOTS)
            {
                return STATUS_INVALID_PARAMETER;
            }

            bitmap = currentPeb->TlsExpansionBitmap;
            Index = i;
        }
        else
        {
            bitmap = currentPeb->TlsBitmap;
        }

        RtlAcquirePebLock();

        if (RtlAreBitsSet(bitmap, Index, 1))
        {
            status = NtSetInformationThread(
                NtCurrentThread(),
                ThreadZeroTlsCell,
                &Index,
                sizeof(ULONG)
                );

            if (NT_SUCCESS(status))
            {
                RtlClearBits(bitmap, Index, 1);
            }
            else
            {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }

        RtlReleasePebLock();

        return status;
    }
    else
    {
        if (TlsFree(Index))
            status = STATUS_SUCCESS;
        else
            status = PhGetLastWin32ErrorAsNtStatus();

        return status;
    }
}

/**
 * Retrieves the value stored in the thread-local storage (TLS) slot specified by the given index.
 *
 * \param Index The index of the TLS slot to retrieve the value from.
 * \return A pointer to the value stored in the specified TLS slot, or NULL if no value is set.
 */
PVOID PhTlsGetValue(
    _In_ ULONG Index
    )
{
    if (WindowsVersion < WINDOWS_NEW && Index < TLS_MINIMUM_AVAILABLE)
    {
        return NtCurrentTeb()->TlsSlots[Index];
    }

    return TlsGetValue(Index);
}

/**
 * Retrieves the value stored in the thread-local storage (TLS) slot specified by the given index.
 *
 * \param Index The index of the TLS slot to retrieve the value from.
 * \param Value A pointer to a variable that receives the value stored in the specified TLS slot.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhTlsGetValueEx(
    _In_ ULONG Index,
    _Out_ PVOID* Value
    )
{
    if (WindowsVersion < WINDOWS_NEW && Index < TLS_MINIMUM_AVAILABLE)
    {
        *Value = NtCurrentTeb()->TlsSlots[Index];
        return STATUS_SUCCESS;
    }

    *Value = TlsGetValue(Index);
    return PhGetLastWin32ErrorAsNtStatus();
}

/**
 * Sets the value for a thread-local storage (TLS) slot identified by the specified index.
 *
 * \param Index The index of the TLS slot to set the value for.
 * \param Value The value to set for the TLS slot. Can be NULL.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhTlsSetValue(
    _In_ ULONG Index,
    _In_opt_ PVOID Value
    )
{
    if (WindowsVersion < WINDOWS_NEW && Index < TLS_MINIMUM_AVAILABLE)
    {
        NtCurrentTeb()->TlsSlots[Index] = Value;
        return STATUS_SUCCESS;
    }

    if (TlsSetValue(Index, Value))
        return STATUS_SUCCESS;

    return PhGetLastWin32ErrorAsNtStatus();
}

#pragma endregion

/**
 * Retrieves the last error code generated by the system or the current thread.
 *
 * \return The last error code as an unsigned long value.
 */
ULONG PhGetLastError(
    VOID
    )
{
    if (WindowsVersion < WINDOWS_NEW)
        return NtReadCurrentTebUlong(FIELD_OFFSET(TEB, LastErrorValue)); // NtCurrentTeb()->LastErrorValue
    return GetLastError();
}

/**
 * Sets the last error value for the current thread.
 *
 * \param ErrorValue The error code to set as the last error.
 */
VOID PhSetLastError(
    _In_ ULONG ErrorValue
    )
{
    if (WindowsVersion < WINDOWS_NEW)
        NtCurrentTeb()->LastErrorValue = ErrorValue;
    else
        SetLastError(ErrorValue);
}


// PhDoesNameContainWildCards and PhIsNameInExpression compared to
// RtlDoesNameContainWildCards and RtlIsNameInExpression (dmex)
// - Full wildcard support:
// - * -Standard wildcard - zero or more any chars
// - ? -Single character wildcard
// - < (ANSI_DOS_STAR) - Zero or more chars until dot
// - > (ANSI_DOS_QM) - Zero or one char (not dot)
// - " (ANSI_DOS_DOT) - Zero or one dot
// - DOS semantics:
// -  -DOS_STAR(<) stops matching at dots (for filename matching like file<.txt)
// -  -DOS_QM(>) matches 0 or 1 non-dot character.
// -  -DOS_DOT(") matches optional dot character.
//
// Example patterns:
// - *chrome* - matches anything with "chrome"
// - chrome.??? - matches chrome.exe, chrome.dll(3 - char extension)
// - file<.txt - matches file.txt, fileabc.txt(< stops at dot)
// - test>" - matches test, testa, test. (> is 0-1 char, " is optional dot)

// replacement RtlDoesNameContainWildCards (dmex)
/**
 * Determines whether a string contains wildcard characters.
 *
 * \param Expression A pointer to the string to be checked.
 * \return TRUE if one or more wildcard characters were found, FALSE otherwise.
 * \remarks The following are wildcard characters: *, ?, ANSI_DOS_STAR (<), ANSI_DOS_DOT ("), and ANSI_DOS_QM (>).
 */
BOOLEAN PhDoesNameContainWildCards(
    _In_ PCPH_STRINGREF Expression
    )
{
    for (SIZE_T i = 0; i < Expression->Length / sizeof(WCHAR); i++)
    {
        WCHAR c = Expression->Buffer[i];
        
        if (c == L'*' ||
            c == L'?' || 
            c == ANSI_DOS_STAR_W ||
            c == ANSI_DOS_DOT_W ||
            c == ANSI_DOS_QM_W
            )
        {
            return TRUE;
        }
    }
    
    return FALSE;
}

// replacement RtlIsNameInExpression (dmex)
/**
 * Determines whether a string matches the specified wildcard pattern.
 *
 * \param Expression A pointer to the pattern string. Can contain wildcards: *, ?, < (DOS_STAR), > (DOS_QM), " (DOS_DOT).
 * \param Name A pointer to the string to match against the pattern.
 * \param IgnoreCase TRUE for case-insensitive matching, FALSE for case-sensitive matching.
 * \return TRUE if the string matches the pattern, FALSE otherwise.
 * \remarks Wildcard semantics:
 *   * - Matches zero or more characters
 *   ? - Matches exactly one character
 *   < (ANSI_DOS_STAR) - Matches zero or more characters until dot or end
 *   > (ANSI_DOS_QM) - Matches zero or one character (not dot)
 *   " (ANSI_DOS_DOT) - Matches zero or one dot
 */
BOOLEAN PhIsNameInExpression(
    _In_ PCPH_STRINGREF Expression,
    _In_ PCPH_STRINGREF Name,
    _In_ BOOLEAN IgnoreCase
    )
{
    SIZE_T exprLen = Expression->Length / sizeof(WCHAR);
    SIZE_T nameLen = Name->Length / sizeof(WCHAR);
    SIZE_T e = 0; // expression index
    SIZE_T n = 0; // name index
    SIZE_T starE = SIZE_MAX; // last * or < position in expression
    SIZE_T starN = SIZE_MAX; // name position when * or < was encountered
    BOOLEAN dosStarMode = FALSE; // TRUE if last star was DOS_STAR (<)

    while (n < nameLen)
    {
        if (e < exprLen)
        {
            WCHAR exprChar = Expression->Buffer[e];
            WCHAR nameChar = Name->Buffer[n];
            WCHAR exprCharUpper = IgnoreCase ? PhUpcaseUnicodeChar(exprChar) : exprChar;
            WCHAR nameCharUpper = IgnoreCase ? PhUpcaseUnicodeChar(nameChar) : nameChar;

            // * - matches zero or more of any character
            if (exprChar == L'*')
            {
                starE = e++;
                starN = n;
                dosStarMode = FALSE;
                continue;
            }
            // < (ANSI_DOS_STAR) - matches zero or more characters until dot or end
            else if (exprChar == ANSI_DOS_STAR_W)
            {
                starE = e++;
                starN = n;
                dosStarMode = TRUE;
                continue;
            }
            // " (ANSI_DOS_DOT) - matches zero or one dot
            else if (exprChar == ANSI_DOS_DOT_W)
            {
                if (nameChar == L'.')
                {
                    e++;
                    n++;
                }
                else
                {
                    e++; // skip the DOS_DOT, match zero dots
                }
                continue;
            }
            // > (ANSI_DOS_QM) - matches zero or one character (not dot)
            else if (exprChar == ANSI_DOS_QM_W)
            {
                if (nameChar != L'.')
                {
                    e++;
                    n++; // consume one character
                }
                else
                {
                    e++; // skip DOS_QM, match zero characters
                }
                continue;
            }
            // ? - matches exactly one character
            else if (exprChar == L'?')
            {
                e++;
                n++;
                continue;
            }
            // Exact character match
            else if (exprCharUpper == nameCharUpper)
            {
                e++;
                n++;
                continue;
            }
        }

        // Mismatch - backtrack if we saw a * or <
        if (starE != SIZE_MAX)
        {
            e = starE + 1;
            n = ++starN;

            // If in DOS_STAR mode, stop backtracking at dot
            if (dosStarMode && starN < nameLen && Name->Buffer[starN] == L'.')
            {
                dosStarMode = FALSE;
                starE = SIZE_MAX; // can't backtrack past dot
            }
            continue;
        }

        return FALSE;
    }

    // Consume remaining wildcards in expression
    while (e < exprLen)
    {
        WCHAR c = Expression->Buffer[e];
        
        if (
            c == L'*' ||
            c == ANSI_DOS_STAR_W ||
            c == ANSI_DOS_DOT_W ||
            c == ANSI_DOS_QM_W
            )
        {
            e++;
        }
        else
        {
            break;
        }
    }

    return e == exprLen;
}

/**
 * Performs fuzzy matching between a pattern and a text string.
 *
 * \param Pattern A pointer to the search pattern.
 * \param Text A pointer to the text to match against.
 * \param IgnoreCase TRUE for case-insensitive matching, FALSE for case-sensitive matching.
 * \return TRUE if the text fuzzy-matches the pattern, FALSE otherwise.
 * \remarks Fuzzy matching allows characters from the pattern to appear in order
 * in the text, but not necessarily consecutively. his is useful for quick
 * filtering where users type a few key characters.
 */
BOOLEAN PhStringFuzzyMatch(
    _In_ PCPH_STRINGREF Pattern,
    _In_ PCPH_STRINGREF Text,
    _In_ BOOLEAN IgnoreCase
    )
{
    SIZE_T patternLength = Pattern->Length / sizeof(WCHAR);
    SIZE_T textLength = Text->Length / sizeof(WCHAR);
    SIZE_T p = 0; // pattern index
    SIZE_T t = 0; // text index

    if (patternLength == 0)
        return TRUE;
    if (textLength == 0)
        return FALSE;

    while (t < textLength && p < patternLength)
    {
        WCHAR pattern = Pattern->Buffer[p];
        WCHAR text = Text->Buffer[t];

        if (IgnoreCase)
        {
            pattern = PhUpcaseUnicodeChar(pattern);
            text = PhUpcaseUnicodeChar(text);
        }

        if (pattern == text)
        {
            p++; // advance on match
        }

        t++; // always advance text
    }

    // Match succeeds if entire pattern was consumed
    return (p == patternLength);
}
