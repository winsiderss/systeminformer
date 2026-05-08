/*
 * Thread Pool support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTTP_H
#define _NTTP_H

// Some types are already defined in winnt.h.

typedef struct _TP_ALPC TP_ALPC, *PTP_ALPC;

// private
/**
 * Defines the callback function for ALPC completion notifications.
 *
 * \param[in,out] Instance A pointer to a TP_CALLBACK_INSTANCE structure that defines the callback instance.
 * \param[in,out] Context Optional application-defined data specified when the ALPC completion object was created.
 * \param[in] Alpc A pointer to the ALPC completion object.
 */
typedef _Function_class_(TP_ALPC_CALLBACK)
VOID NTAPI TP_ALPC_CALLBACK(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID Context,
    _In_ PTP_ALPC Alpc
    );
typedef TP_ALPC_CALLBACK *PTP_ALPC_CALLBACK;

// rev
/**
 * Defines the extended callback function for ALPC completion notifications.
 *
 * \param[in,out] Instance A pointer to a TP_CALLBACK_INSTANCE structure that defines the callback instance.
 * \param[in,out] Context Optional application-defined data specified when the ALPC completion object was created.
 * \param[in] Alpc A pointer to the ALPC completion object.
 * \param[in] ApcContext A pointer to additional APC context data for the completion callback.
 */
typedef _Function_class_(TP_ALPC_CALLBACK_EX)
VOID NTAPI TP_ALPC_CALLBACK_EX(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID Context,
    _In_ PTP_ALPC Alpc,
    _In_ PVOID ApcContext
    );
typedef TP_ALPC_CALLBACK_EX *PTP_ALPC_CALLBACK_EX;

// winbase:CreateThreadpool
/**
 * Allocates a new pool of threads to execute callbacks.
 *
 * \param[out] PoolReturn Pointer to a variable that receives the address of the newly allocated thread pool.
 * \param[in] Reserved Reserved for future use. Must be NULL.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-createthreadpool
 */
NTSYSAPI
NTSTATUS
NTAPI
TpAllocPool(
    _Out_ PTP_POOL *PoolReturn,
    _Reserved_ PVOID Reserved
    );

// winbase:CloseThreadpool
/**
 * Closes the specified thread pool.
 *
 * \param[in,out] Pool A pointer to a TP_POOL structure that defines the thread pool.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-closethreadpool
 */
NTSYSAPI
VOID
NTAPI
TpReleasePool(
    _Inout_ PTP_POOL Pool
    );

// winbase:SetThreadpoolThreadMaximum
/**
 * Sets the maximum number of threads that the specified thread pool can allocate to process callbacks.
 *
 * \param[in,out] Pool A pointer to a TP_POOL structure that defines the thread pool.
 * \param[in] MaxThreads The maximum number of threads.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-setthreadpoolthreadmaximum
 */
NTSYSAPI
VOID
NTAPI
TpSetPoolMaxThreads(
    _Inout_ PTP_POOL Pool,
    _In_ ULONG MaxThreads
    );

// winbase:SetThreadpoolThreadMinimum
/**
 * Sets the minimum number of threads that the specified thread pool must make available to process callbacks.
 *
 * \param[in,out] Pool A pointer to a TP_POOL structure that defines the thread pool.
 * \param[in] MinThreads The minimum number of threads.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-setthreadpoolthreadminimum
 */
NTSYSAPI
NTSTATUS
NTAPI
TpSetPoolMinThreads(
    _Inout_ PTP_POOL Pool,
    _In_ ULONG MinThreads
    );

// winbase:QueryThreadpoolStackInformation
/**
 * Retrieves the stack reserve and commit sizes for threads in the specified thread pool.
 *
 * \param[in] Pool A pointer to a TP_POOL structure that defines the thread pool.
 * \param[out] PoolStackInformation A pointer to a TP_POOL_STACK_INFORMATION structure that receives the stack reserve and commit size.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-querythreadpoolstackinformation
 */
NTSYSAPI
NTSTATUS
NTAPI
TpQueryPoolStackInformation(
    _In_ PTP_POOL Pool,
    _Out_ PTP_POOL_STACK_INFORMATION PoolStackInformation
    );

// winbase:SetThreadpoolStackInformation
/**
 * Sets the stack reserve and commit sizes for threads in the specified thread pool.
 *
 * \param[in,out] Pool A pointer to a TP_POOL structure that defines the thread pool.
 * \param[in] PoolStackInformation A pointer to a TP_POOL_STACK_INFORMATION structure that specifies stack reserve and commit sizes.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-setthreadpoolstackinformation
 */
NTSYSAPI
NTSTATUS
NTAPI
TpSetPoolStackInformation(
    _Inout_ PTP_POOL Pool,
    _In_ PTP_POOL_STACK_INFORMATION PoolStackInformation
    );

// rev
/**
 * Sets the base priority for worker threads in the specified thread pool.
 *
 * \param[in,out] Pool A pointer to a TP_POOL structure that defines the thread pool.
 * \param[in] BasePriority The new base priority value for worker threads.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
TpSetPoolThreadBasePriority(
    _Inout_ PTP_POOL Pool,
    _In_ ULONG BasePriority
    );

// winbase:CreateThreadpoolCleanupGroup
/**
 * Creates a cleanup group that applications can use to track one or more thread pool callbacks.
 *
 * \param[out] CleanupGroup A pointer to a variable that receives the address of the newly allocated cleanup group.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-createthreadpoolcleanupgroup
 */
NTSYSAPI
NTSTATUS
NTAPI
TpAllocCleanupGroup(
    _Out_ PTP_CLEANUP_GROUP *CleanupGroup
    );

// winbase:CloseThreadpoolCleanupGroup
/**
 * Closes the specified cleanup group.
 *
 * \param[in,out] CleanupGroup A pointer to a TP_CLEANUP_GROUP structure that defines the cleanup group.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-closethreadpoolcleanupgroup
 */
NTSYSAPI
VOID
NTAPI
TpReleaseCleanupGroup(
    _Inout_ PTP_CLEANUP_GROUP CleanupGroup
    );

// winbase:CloseThreadpoolCleanupGroupMembers
/**
 * Releases the members of the specified cleanup group.
 *
 * \param[in,out] CleanupGroup A pointer to a TP_CLEANUP_GROUP structure that defines the cleanup group.
 * \param[in] CancelPendingCallbacks If TRUE, pending callbacks that have not started are canceled.
 * \param[in,out] CleanupParameter Optional application-defined context to pass to cleanup callbacks.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-closethreadpoolcleanupgroupmembers
 */
NTSYSAPI
VOID
NTAPI
TpReleaseCleanupGroupMembers(
    _Inout_ PTP_CLEANUP_GROUP CleanupGroup,
    _In_ LOGICAL CancelPendingCallbacks,
    _Inout_opt_ PVOID CleanupParameter
    );

// winbase:SetEventWhenCallbackReturns
/**
 * Sets an event object when the current callback function returns.
 *
 * \param[in,out] Instance A pointer to a TP_CALLBACK_INSTANCE structure that defines the callback instance.
 * \param[in] Event A handle to an event object.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-seteventwhencallbackreturns
 */
NTSYSAPI
VOID
NTAPI
TpCallbackSetEventOnCompletion(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _In_ HANDLE Event
    );

// winbase:ReleaseSemaphoreWhenCallbackReturns
/**
 * Releases a semaphore when the current callback function returns.
 *
 * \param[in,out] Instance A pointer to a TP_CALLBACK_INSTANCE structure that defines the callback instance.
 * \param[in] Semaphore A handle to a semaphore object.
 * \param[in] ReleaseCount The amount by which to increment the semaphore's current count.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-releasesemaphorewhencallbackreturns
 */
NTSYSAPI
VOID
NTAPI
TpCallbackReleaseSemaphoreOnCompletion(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _In_ HANDLE Semaphore,
    _In_ ULONG ReleaseCount
    );

// winbase:ReleaseMutexWhenCallbackReturns
/**
 * Releases a mutex when the current callback function returns.
 *
 * \param[in,out] Instance A pointer to a TP_CALLBACK_INSTANCE structure that defines the callback instance.
 * \param[in] Mutex A handle to a mutex object.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-releasemutexwhencallbackreturns
 */
NTSYSAPI
VOID
NTAPI
TpCallbackReleaseMutexOnCompletion(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _In_ HANDLE Mutex
    );

// winbase:LeaveCriticalSectionWhenCallbackReturns
/**
 * Leaves a critical section when the current callback function returns.
 *
 * \param[in,out] Instance A pointer to a TP_CALLBACK_INSTANCE structure that defines the callback instance.
 * \param[in,out] CriticalSection A pointer to a critical section object.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-leavecriticalsectionwhencallbackreturns
 */
NTSYSAPI
VOID
NTAPI
TpCallbackLeaveCriticalSectionOnCompletion(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    );

// winbase:FreeLibraryWhenCallbackReturns
/**
 * Frees the specified DLL when the current callback function returns.
 *
 * \param[in,out] Instance A pointer to a TP_CALLBACK_INSTANCE structure that defines the callback instance.
 * \param[in] DllHandle A module handle for the DLL to unload.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-freelibrarywhencallbackreturns
 */
NTSYSAPI
VOID
NTAPI
TpCallbackUnloadDllOnCompletion(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _In_ PVOID DllHandle
    );

// winbase:CallbackMayRunLong
/**
 * Indicates that the callback may not return quickly.
 *
 * \param[in,out] Instance A pointer to a TP_CALLBACK_INSTANCE structure that defines the callback instance.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-callbackmayrunlong
 */
NTSYSAPI
NTSTATUS
NTAPI
TpCallbackMayRunLong(
    _Inout_ PTP_CALLBACK_INSTANCE Instance
    );

// winbase:DisassociateCurrentThreadFromCallback
/**
 * Removes the association between the current thread and the callback instance.
 *
 * \param[in,out] Instance A pointer to a TP_CALLBACK_INSTANCE structure that defines the callback instance.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-disassociatecurrentthreadfromcallback
 */
NTSYSAPI
VOID
NTAPI
TpDisassociateCallback(
    _Inout_ PTP_CALLBACK_INSTANCE Instance
    );

/**
 * Defines the callback function that is executed by a thread pool worker thread.
 *
 * \param[in,out] Instance A pointer to a TP_CALLBACK_INSTANCE structure that defines the callback instance.
 * \param[in,out] Context Optional application-defined data passed to the callback.
 */
typedef _Function_class_(TP_CALLBACK_ROUTINE)
VOID NTAPI TP_CALLBACK_ROUTINE(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID Context
    );
typedef TP_CALLBACK_ROUTINE* PTP_CALLBACK_ROUTINE;

// winbase:TrySubmitThreadpoolCallback
/**
 * Requests that a thread pool worker thread call the specified callback function.
 *
 * \param[in] Callback The callback function.
 * \param[in,out] Context Optional application-defined data to pass to the callback function.
 * \param[in] CallbackEnviron A pointer to a TP_CALLBACK_ENVIRON structure that defines the environment in which to execute the callback function.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-trysubmitthreadpoolcallback
 */
NTSYSAPI
NTSTATUS
NTAPI
TpSimpleTryPost(
    _In_ PTP_CALLBACK_ROUTINE Callback,
    _Inout_opt_ PVOID Context,
    _In_opt_ PTP_CALLBACK_ENVIRON CallbackEnviron
    );

// winbase:CreateThreadpoolWork
/**
 * Allocates a work object.
 *
 * \param[out] WorkReturn A pointer to a variable that receives the new work object.
 * \param[in] Callback The callback function to execute.
 * \param[in,out] Context Optional application-defined data to pass to the callback function.
 * \param[in] CallbackEnviron Optional callback environment for the callback.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-createthreadpoolwork
 */
NTSYSAPI
NTSTATUS
NTAPI
TpAllocWork(
    _Out_ PTP_WORK *WorkReturn,
    _In_ PTP_WORK_CALLBACK Callback,
    _Inout_opt_ PVOID Context,
    _In_opt_ PTP_CALLBACK_ENVIRON CallbackEnviron
    );

// winbase:CloseThreadpoolWork
/**
 * Closes the specified work object.
 *
 * \param[in,out] Work A pointer to a TP_WORK structure that defines the work object.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-closethreadpoolwork
 */
NTSYSAPI
VOID
NTAPI
TpReleaseWork(
    _Inout_ PTP_WORK Work
    );

// winbase:SubmitThreadpoolWork
/**
 * Submits a work object to the thread pool.
 *
 * \param[in,out] Work A pointer to a TP_WORK structure that defines the work object.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-submitthreadpoolwork
 */
NTSYSAPI
VOID
NTAPI
TpPostWork(
    _Inout_ PTP_WORK Work
    );

// winbase:WaitForThreadpoolWorkCallbacks
/**
 * Waits for outstanding work callbacks to complete and optionally cancels pending callbacks.
 *
 * \param[in,out] Work A pointer to a TP_WORK structure that defines the work object.
 * \param[in] CancelPendingCallbacks If TRUE, pending callbacks that have not started are canceled.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-waitforthreadpoolworkcallbacks
 */
NTSYSAPI
VOID
NTAPI
TpWaitForWork(
    _Inout_ PTP_WORK Work,
    _In_ LOGICAL CancelPendingCallbacks
    );

// winbase:CreateThreadpoolTimer
/**
 * Allocates a timer object.
 *
 * \param[out] Timer A pointer to a variable that receives the new timer object.
 * \param[in] Callback The callback function to execute when the timer expires.
 * \param[in,out] Context Optional application-defined data to pass to the callback function.
 * \param[in] CallbackEnviron Optional callback environment for the callback.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-createthreadpooltimer
 */
NTSYSAPI
NTSTATUS
NTAPI
TpAllocTimer(
    _Out_ PTP_TIMER *Timer,
    _In_ PTP_TIMER_CALLBACK Callback,
    _Inout_opt_ PVOID Context,
    _In_opt_ PTP_CALLBACK_ENVIRON CallbackEnviron
    );

// winbase:CloseThreadpoolTimer
/**
 * Closes the specified timer object.
 *
 * \param[in,out] Timer A pointer to a TP_TIMER structure that defines the timer object.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-closethreadpooltimer
 */
NTSYSAPI
VOID
NTAPI
TpReleaseTimer(
    _Inout_ PTP_TIMER Timer
    );

// winbase:SetThreadpoolTimer
/**
 * Sets the due time and periodic interval for the specified timer object.
 *
 * \param[in,out] Timer A pointer to a TP_TIMER structure that defines the timer object.
 * \param[in] DueTime A pointer to a FILETIME-based value that specifies when the timer expires.
 * \param[in] Period The period, in milliseconds, for periodic timer callbacks.
 * \param[in] WindowLength The maximum amount of time, in milliseconds, that can elapse before the timer callback is invoked.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-setthreadpooltimer
 */
NTSYSAPI
VOID
NTAPI
TpSetTimer(
    _Inout_ PTP_TIMER Timer,
    _In_opt_ PLARGE_INTEGER DueTime,
    _In_ ULONG Period,
    _In_opt_ ULONG WindowLength
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// winbase:SetThreadpoolTimerEx
/**
 * Sets the due time and periodic interval for the specified timer object and returns status.
 *
 * \param[in,out] Timer A pointer to a TP_TIMER structure that defines the timer object.
 * \param[in] DueTime A pointer to a FILETIME-based value that specifies when the timer expires.
 * \param[in] Period The period, in milliseconds, for periodic timer callbacks.
 * \param[in] WindowLength The maximum amount of time, in milliseconds, that can elapse before the timer callback is invoked.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-setthreadpooltimerex
 */
NTSYSAPI
NTSTATUS
NTAPI
TpSetTimerEx(
    _Inout_ PTP_TIMER Timer,
    _In_opt_ PLARGE_INTEGER DueTime,
    _In_ ULONG Period,
    _In_opt_ ULONG WindowLength
    );
#endif

// winbase:IsThreadpoolTimerSet
/**
 * Determines whether a timer object is currently set.
 *
 * \param[in] Timer A pointer to a TP_TIMER structure that defines the timer object.
 * \return LOGICAL TRUE if the timer is set; otherwise FALSE.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-isthreadpooltimerset
 */
NTSYSAPI
LOGICAL
NTAPI
TpIsTimerSet(
    _In_ PTP_TIMER Timer
    );

// winbase:WaitForThreadpoolTimerCallbacks
/**
 * Waits for outstanding timer callbacks to complete and optionally cancels pending callbacks.
 *
 * \param[in,out] Timer A pointer to a TP_TIMER structure that defines the timer object.
 * \param[in] CancelPendingCallbacks If TRUE, pending callbacks that have not started are canceled.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-waitforthreadpooltimercallbacks
 */
NTSYSAPI
VOID
NTAPI
TpWaitForTimer(
    _Inout_ PTP_TIMER Timer,
    _In_ LOGICAL CancelPendingCallbacks
    );

// winbase:CreateThreadpoolWait
/**
 * Allocates a wait object.
 *
 * \param[out] WaitReturn A pointer to a variable that receives the new wait object.
 * \param[in] Callback The callback function to execute when the wait is satisfied or times out.
 * \param[in,out] Context Optional application-defined data to pass to the callback function.
 * \param[in] CallbackEnviron Optional callback environment for the callback.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-createthreadpoolwait
 */
NTSYSAPI
NTSTATUS
NTAPI
TpAllocWait(
    _Out_ PTP_WAIT *WaitReturn,
    _In_ PTP_WAIT_CALLBACK Callback,
    _Inout_opt_ PVOID Context,
    _In_opt_ PTP_CALLBACK_ENVIRON CallbackEnviron
    );

// winbase:CloseThreadpoolWait
/**
 * Closes the specified wait object.
 *
 * \param[in,out] Wait A pointer to a TP_WAIT structure that defines the wait object.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-closethreadpoolwait
 */
NTSYSAPI
VOID
NTAPI
TpReleaseWait(
    _Inout_ PTP_WAIT Wait
    );

// winbase:SetThreadpoolWait
/**
 * Sets the object to wait on and optional timeout for a wait object.
 *
 * \param[in,out] Wait A pointer to a TP_WAIT structure that defines the wait object.
 * \param[in] Handle A handle to the object to wait on.
 * \param[in] Timeout A pointer to a FILETIME-based timeout value.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-setthreadpoolwait
 */
NTSYSAPI
VOID
NTAPI
TpSetWait(
    _Inout_ PTP_WAIT Wait,
    _In_opt_ HANDLE Handle,
    _In_opt_ PLARGE_INTEGER Timeout
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// winbase:SetThreadpoolWaitEx
/**
 * Sets the object to wait on and optional timeout for a wait object and returns status.
 *
 * \param[in,out] Wait A pointer to a TP_WAIT structure that defines the wait object.
 * \param[in] Handle A handle to the object to wait on.
 * \param[in] Timeout A pointer to a FILETIME-based timeout value.
 * \param[in] Reserved Reserved for future use.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-setthreadpoolwaitex
 */
NTSYSAPI
NTSTATUS
NTAPI
TpSetWaitEx(
    _Inout_ PTP_WAIT Wait,
    _In_opt_ HANDLE Handle,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_opt_ PVOID Reserved
    );
#endif

// winbase:WaitForThreadpoolWaitCallbacks
/**
 * Waits for outstanding wait callbacks to complete and optionally cancels pending callbacks.
 *
 * \param[in,out] Wait A pointer to a TP_WAIT structure that defines the wait object.
 * \param[in] CancelPendingCallbacks If TRUE, pending callbacks that have not started are canceled.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-waitforthreadpoolwaitcallbacks
 */
NTSYSAPI
VOID
NTAPI
TpWaitForWait(
    _Inout_ PTP_WAIT Wait,
    _In_ LOGICAL CancelPendingCallbacks
    );

// private
/**
 * Defines the callback function for asynchronous I/O completion callbacks.
 *
 * \param[in,out] Instance A pointer to a TP_CALLBACK_INSTANCE structure that defines the callback instance.
 * \param[in,out] Context Optional application-defined data passed to the callback.
 * \param[in] ApcContext A pointer to the asynchronous procedure call (APC) context.
 * \param[in] IoSB A pointer to an IO_STATUS_BLOCK structure that contains the final I/O status.
 * \param[in] Io A pointer to the I/O completion object.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nc-threadpoolapiset-ptp_win32_io_callback
 */
typedef _Function_class_(TP_IO_CALLBACK)
VOID NTAPI TP_IO_CALLBACK(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID Context,
    _In_ PVOID ApcContext,
    _In_ PIO_STATUS_BLOCK IoSB,
    _In_ PTP_IO Io
    );
typedef TP_IO_CALLBACK *PTP_IO_CALLBACK;

// winbase:CreateThreadpoolIo
/**
 * Allocates an I/O completion object associated with a file handle.
 *
 * \param[out] IoReturn A pointer to a variable that receives the new I/O completion object.
 * \param[in] File A file handle associated with an I/O completion port.
 * \param[in] Callback The callback function to execute when asynchronous I/O completes.
 * \param[in,out] Context Optional application-defined data to pass to the callback function.
 * \param[in] CallbackEnviron Optional callback environment for the callback.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-createthreadpoolio
 */
NTSYSAPI
NTSTATUS
NTAPI
TpAllocIoCompletion(
    _Out_ PTP_IO *IoReturn,
    _In_ HANDLE File,
    _In_ PTP_IO_CALLBACK Callback,
    _Inout_opt_ PVOID Context,
    _In_opt_ PTP_CALLBACK_ENVIRON CallbackEnviron
    );

// winbase:CloseThreadpoolIo
/**
 * Closes the specified I/O completion object.
 *
 * \param[in,out] Io A pointer to a TP_IO structure that defines the I/O completion object.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-closethreadpoolio
 */
NTSYSAPI
VOID
NTAPI
TpReleaseIoCompletion(
    _Inout_ PTP_IO Io
    );

// winbase:StartThreadpoolIo
/**
 * Marks the start of one or more asynchronous I/O operations on the I/O completion object.
 *
 * \param[in,out] Io A pointer to a TP_IO structure that defines the I/O completion object.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-startthreadpoolio
 */
NTSYSAPI
VOID
NTAPI
TpStartAsyncIoOperation(
    _Inout_ PTP_IO Io
    );

// winbase:CancelThreadpoolIo
/**
 * Cancels callbacks for outstanding asynchronous I/O operations.
 *
 * \param[in,out] Io A pointer to a TP_IO structure that defines the I/O completion object.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-cancelthreadpoolio
 */
NTSYSAPI
VOID
NTAPI
TpCancelAsyncIoOperation(
    _Inout_ PTP_IO Io
    );

// winbase:WaitForThreadpoolIoCallbacks
/**
 * Waits for outstanding I/O callbacks to complete and optionally cancels pending callbacks.
 *
 * \param[in,out] Io A pointer to a TP_IO structure that defines the I/O completion object.
 * \param[in] CancelPendingCallbacks If TRUE, pending callbacks that have not started are canceled.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-waitforthreadpooliocallbacks
 */
NTSYSAPI
VOID
NTAPI
TpWaitForIoCompletion(
    _Inout_ PTP_IO Io,
    _In_ LOGICAL CancelPendingCallbacks
    );

// private
/**
 * Allocates an ALPC completion object.
 *
 * \param[out] AlpcReturn A pointer to a variable that receives the new ALPC completion object.
 * \param[in] AlpcPort A handle to an ALPC port.
 * \param[in] Callback The callback function to execute for ALPC completions.
 * \param[in,out] Context Optional application-defined data to pass to the callback function.
 * \param[in] CallbackEnviron Optional callback environment for the callback.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
TpAllocAlpcCompletion(
    _Out_ PTP_ALPC *AlpcReturn,
    _In_ HANDLE AlpcPort,
    _In_ PTP_ALPC_CALLBACK Callback,
    _Inout_opt_ PVOID Context,
    _In_opt_ PTP_CALLBACK_ENVIRON CallbackEnviron
    );

// rev
/**
 * Allocates an ALPC completion object with extended callback context.
 *
 * \param[out] AlpcReturn A pointer to a variable that receives the new ALPC completion object.
 * \param[in] AlpcPort A handle to an ALPC port.
 * \param[in] Callback The extended callback function to execute for ALPC completions.
 * \param[in,out] Context Optional application-defined data to pass to the callback function.
 * \param[in] CallbackEnviron Optional callback environment for the callback.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
TpAllocAlpcCompletionEx(
    _Out_ PTP_ALPC *AlpcReturn,
    _In_ HANDLE AlpcPort,
    _In_ PTP_ALPC_CALLBACK_EX Callback,
    _Inout_opt_ PVOID Context,
    _In_opt_ PTP_CALLBACK_ENVIRON CallbackEnviron
    );

// private
/**
 * Releases an ALPC completion object.
 *
 * \param[in,out] Alpc A pointer to a TP_ALPC structure that defines the ALPC completion object.
 */
NTSYSAPI
VOID
NTAPI
TpReleaseAlpcCompletion(
    _Inout_ PTP_ALPC Alpc
    );

// private
/**
 * Waits for ALPC completion processing to finish.
 *
 * \param[in,out] Alpc A pointer to a TP_ALPC structure that defines the ALPC completion object.
 */
NTSYSAPI
VOID
NTAPI
TpWaitForAlpcCompletion(
    _Inout_ PTP_ALPC Alpc
    );

// rev
/**
 * Registers the ALPC completion list for the specified ALPC completion object.
 *
 * \param[in,out] Alpc A pointer to a TP_ALPC structure that defines the ALPC completion object.
 */
NTSYSAPI
VOID
NTAPI
TpAlpcRegisterCompletionList(
    _Inout_ PTP_ALPC Alpc
    );

// rev
/**
 * Unregisters the ALPC completion list for the specified ALPC completion object.
 *
 * \param[in,out] Alpc A pointer to a TP_ALPC structure that defines the ALPC completion object.
 */
NTSYSAPI
VOID
NTAPI
TpAlpcUnregisterCompletionList(
    _Inout_ PTP_ALPC Alpc
    );

// private
typedef enum _TP_TRACE_TYPE
{
    TpTraceThreadPriority = 1,
    TpTraceThreadAffinity,
    MaxTpTraceType
} TP_TRACE_TYPE;

// private
/**
 * Captures caller context for thread pool tracing.
 *
 * \param[in] Type The trace operation type.
 */
NTSYSAPI
VOID
NTAPI
TpCaptureCaller(
    _In_ TP_TRACE_TYPE Type
    );

// private
/**
 * Checks whether a worker thread should terminate.
 *
 * \param[in] Thread A handle to the worker thread.
 */
NTSYSAPI
VOID
NTAPI
TpCheckTerminateWorker(
    _In_ HANDLE Thread
    );

#endif // _NTTP_H
