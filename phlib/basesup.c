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

PPH_STRING PhSharedEmptyString = NULL;

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

            _mm256_zeroupper();

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

            _mm256_zeroupper();

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

            _mm256_zeroupper();

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

            _mm256_zeroupper();

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

            _mm256_zeroupper();

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

            _mm256_zeroupper();

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
