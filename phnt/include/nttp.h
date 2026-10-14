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
 * \remarks Creates a private pool. Reserved must be NULL and PoolReturn must be writable. 
 * Invalid contracts can raise STATUS_INVALID_PARAMETER through TppRaiseInvalidParameter rather than return a failure status. 
 * Close owned objects and drain callbacks before releasing their pool.
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
 * \remarks Releases the caller pool reference; it is not a substitute for closing each work/wait/timer/I/O object and draining callbacks. 
 * Objects retain their own references and lifetime obligations.
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
 * \remarks Configures this pool worker concurrency limits. Maximum is a VOID setter; Minimum reports NTSTATUS. 
 * These are scheduling limits, not a guarantee that a particular callback has run or completed.
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
 * \remarks Configures this pool worker concurrency limits. Maximum is a VOID setter; Minimum reports NTSTATUS.
 * These are scheduling limits, not a guarantee that a particular callback has run or completed.
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
 * \remarks Queries or configures worker stack reserve/commit sizes in bytes through TP_POOL_STACK_INFORMATION.
 * These are SIZE_T fields, so preserve pointer-width layout on x64. The setter does not resize an already active callback stack.
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
 * \remarks Queries or configures worker stack reserve/commit sizes in bytes through TP_POOL_STACK_INFORMATION. 
 * These are SIZE_T fields, so preserve pointer-width layout on x64. The setter does not resize an already active callback stack.
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
 * \remarks BasePriority is signed LONG. The inspected code maps values >= 15 to 16 and values <= -15 to -16 before forwarding worker-factory class 11. 
 * It is not an unsigned absolute priority. Apply changes only to an owned pool.
 */
NTSYSAPI
NTSTATUS
NTAPI
TpSetPoolThreadBasePriority(
    _Inout_ PTP_POOL Pool,
    _In_ LONG BasePriority
    );

// winbase:CreateThreadpoolCleanupGroup
/**
 * Creates a cleanup group that applications can use to track one or more thread pool callbacks.
 *
 * \param[out] CleanupGroup A pointer to a variable that receives the address of the newly allocated cleanup group.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-createthreadpoolcleanupgroup
 * \remarks A cleanup group owns registrations of member callback objects; release group members first, then release the group itself. 
 * Releasing an empty group does not certify cancellation behavior for executing callbacks.
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
 * \remarks A cleanup group owns registrations of member callback objects; release group members first, then release the group itself. 
 * Releasing an empty group does not certify cancellation behavior for executing callbacks.
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
 * \remarks Releases the group member objects and waits according to the cleanup protocol; CancelPendingCallbacks can cancel callbacks 
 * not yet started but cannot forcibly terminate one already executing. After this call do not separately release a member object whose group ownership has been consumed.
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
 * \remarks Registers an action for callback completion, rather than performing the action immediately. 
 * Instance must belong to the currently running callback. Keep the target object/handle/module valid until the action executes; for lock release the callback must own the lock. 
 * These are not general-purpose cross-thread release helpers.
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
 * \remarks Registers an action for callback completion, rather than performing the action immediately. Instance must belong to the currently running callback.
 * Keep the target object/handle/module valid until the action executes; for lock release the callback must own the lock. These are not general-purpose cross-thread release helpers.
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
 * \remarks Registers an action for callback completion, rather than performing the action immediately.
 * Instance must belong to the currently running callback. Keep the target object/handle/module valid until the action executes; for lock release the callback must own the lock. 
 * These are not general-purpose cross-thread release helpers.
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
 * \remarks Registers an action for callback completion, rather than performing the action immediately. 
 * nstance must belong to the currently running callback. Keep the target object/handle/module valid until the action executes; for lock release the callback must own the lock. 
 * These are not general-purpose cross-thread release helpers.
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
 * \remarks Registers an action for callback completion, rather than performing the action immediately.
 * Instance must belong to the currently running callback. Keep the target object/handle/module valid until the action executes; for lock release the callback must own the lock. 
 * These are not general-purpose cross-thread release helpers.
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
 * \remarks Notifies the pool that this active callback may block/run long. The returned NTSTATUS is not callback completion, and the call does not transfer ownership of callback resources.
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
 * \remarks Ends the callback association used by callback-object waits while execution can continue. 
 * Code/context still in use after disassociation must have an independent lifetime guarantee; a successful wait alone no longer proves that code has returned.
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
 * \remarks Allocates/posts a one-shot callback using the optional environment. Success indicates submission, not completion. 
 * There is no returned work object, so arrange explicit completion/lifetime synchronization.
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
 * \remarks On success the caller receives an owned callback object; callback and context are borrowed. 
 * The optional environment chooses the pool/cleanup group. An absent environment uses runtime defaults. Allocation does not post work, arm a timer or associate a wait.
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
 * \remarks Releases the caller callback-object reference. Explicitly stop/rearm as appropriate and drain callbacks before reclaiming borrowed context;
 * closing an object is not a safe way to terminate executing user code.
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
 * \remarks Posts an execution of an existing work object. Multiple posts can schedule multiple callbacks;
 * use TpWaitForWork for drainage and keep borrowed context alive until callbacks finish.
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
 * \remarks Waits for the associated callback-object protocol, optionally canceling pending callbacks.
 * Cancellation does not terminate executing callbacks. First disarm a timer/wait if further dispatch is unwanted, 
 * and avoid self-deadlock by waiting on the callback object from inside its own associated callback.
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
 * \remarks On success the caller receives an owned callback object; callback and context are borrowed. 
 * The optional environment chooses the pool/cleanup group. An absent environment uses runtime defaults. 
 * Allocation does not post work, arm a timer or associate a wait.
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
 * \remarks Releases the caller callback-object reference. Explicitly stop/rearm as appropriate and drain callbacks 
 * before reclaiming borrowed context; closing an object is not a safe way to terminate executing user code.
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
 * \remarks DueTime NULL disarms; negative values are relative 100-nanosecond intervals and nonnegative values are absolute time. 
 * Period and WindowLength are milliseconds. The Ex return is a 32-bit LOGICAL cancellation result for the prior timer, not NTSTATUS or success of the newly requested schedule. 
 * A newly armed timer can return FALSE; replacing/disarming an outstanding timer can return TRUE. Already queued/executing callbacks still require drainage.
 */
NTSYSAPI
VOID
NTAPI
TpSetTimer(
    _Inout_ PTP_TIMER Timer,
    _In_opt_ PLARGE_INTEGER DueTime,
    _In_ ULONG Period,
    _In_ ULONG WindowLength
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// winbase:SetThreadpoolTimerEx
/**
 * TpSetTimerEx contract.
 *
 * \param Timer Timer object to update.
 * \param DueTime Native-time due value, or NULL to disarm.
 * \param Period Period in milliseconds.
 * \param WindowLength Coalescing window in milliseconds.
 * \remarks DueTime NULL disarms; negative values are relative 100-nanosecond intervals and nonnegative values are absolute time. 
 * Period and WindowLength are milliseconds. The Ex return is a 32-bit LOGICAL cancellation result for the prior timer, not NTSTATUS or success of the newly requested schedule. 
 * A newly armed timer can return FALSE; replacing/disarming an outstanding timer can return TRUE. Already queued/executing callbacks still require drainage.
 */
NTSYSAPI
LOGICAL
NTAPI
TpSetTimerEx(
    _Inout_ PTP_TIMER Timer,
    _In_opt_ PLARGE_INTEGER DueTime,
    _In_ ULONG Period,
    _In_ ULONG WindowLength
    );
#endif

// winbase:IsThreadpoolTimerSet
/**
 * Determines whether a timer object is currently set.
 *
 * \param[in] Timer A pointer to a TP_TIMER structure that defines the timer object.
 * \return LOGICAL TRUE if the timer is set; otherwise FALSE.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-isthreadpooltimerset
 * \remarks Reports whether the timer is currently armed, not whether every previous callback has completed. Use TpWaitForTimer and lifetime synchronization for the latter.
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
 * \remarks Waits for the associated callback-object protocol, optionally canceling pending callbacks. 
 * Cancellation does not terminate executing callbacks. First disarm a timer/wait if further dispatch is unwanted, 
 * and avoid self-deadlock by waiting on the callback object from inside its own associated callback.
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
 * \remarks On success the caller receives an owned callback object; callback and context are borrowed. 
 * The optional environment chooses the pool/cleanup group. An absent environment uses runtime defaults.
 * Allocation does not post work, arm a timer or associate a wait.
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
 * \remarks Releases the caller callback-object reference. Explicitly stop/rearm as appropriate and drain callbacks before reclaiming borrowed context;
 * closing an object is not a safe way to terminate executing user code.
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
 * \remarks Handle NULL disarms the association; a non-NULL Timeout uses LARGE_INTEGER native time units. The Ex Reserved argument must be NULL. 
 * The Ex result is a 32-bit LOGICAL prior-association cancellation result, not NTSTATUS; FALSE can accompany a valid new association. Rearming/disarming does not forcibly terminate a callback already running.
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
 * TpSetWaitEx contract.
 *
 * \param Wait Wait object to update.
 * \param Handle Object to wait for, or NULL to disarm.
 * \param Timeout Optional native-time timeout.
 * \param Reserved Must be NULL.
 * \remarks Handle NULL disarms the association; a non-NULL Timeout uses LARGE_INTEGER native time units. The Ex Reserved argument must be NULL. 
 * The Ex result is a 32-bit LOGICAL prior-association cancellation result, not NTSTATUS; FALSE can accompany a valid new association. Rearming/disarming does not forcibly terminate a callback already running.
 */
NTSYSAPI
LOGICAL
NTAPI
TpSetWaitEx(
    _Inout_ PTP_WAIT Wait,
    _In_opt_ HANDLE Handle,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Reserved_ PVOID Reserved
    );
#endif

// winbase:WaitForThreadpoolWaitCallbacks
/**
 * Waits for outstanding wait callbacks to complete and optionally cancels pending callbacks.
 *
 * \param[in,out] Wait A pointer to a TP_WAIT structure that defines the wait object.
 * \param[in] CancelPendingCallbacks If TRUE, pending callbacks that have not started are canceled.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoolapiset/nf-threadpoolapiset-waitforthreadpoolwaitcallbacks
 * \remarks Waits for the associated callback-object protocol, optionally canceling pending callbacks. Cancellation does not terminate executing callbacks.
 * First disarm a timer/wait if further dispatch is unwanted, and avoid self-deadlock by waiting on the callback object from inside its own associated callback.
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
 * \remarks Binds callback delivery to the supplied file completion mechanism. Native TP_IO_CALLBACK receives ApcContext and IO_STATUS_BLOCK, not the Win32 callback argument list. Keep the file, callback and context alive through I/O completion and object drainage.
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
 * \remarks Binds callback delivery to the supplied file completion mechanism. Native TP_IO_CALLBACK receives ApcContext and IO_STATUS_BLOCK, not the Win32 callback argument list. Keep the file, callback and context alive through I/O completion and object drainage.
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
 * \remarks Accounts for an operation expected to post an I/O completion. CancelAsyncIoOperation balances that accounting when no completion will arrive; it does not itself cancel the underlying file I/O. Do not double-cancel the accounting or both cancel it and later deliver the same completion.
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
 * \remarks Accounts for an operation expected to post an I/O completion. CancelAsyncIoOperation balances that accounting when no completion will arrive; it does not itself cancel the underlying file I/O. Do not double-cancel the accounting or both cancel it and later deliver the same completion.
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
 * \remarks Drains callback accounting for this object. CancelPendingCallbacks concerns callback dispatch, not cancellation of the underlying I/O request. Coordinate actual I/O cancellation and file-handle lifetime separately.
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
 * \remarks Uses an ALPC-specific completion object and port association, not a TP_IO object. The Ex allocator selects the extended callback dispatch form. Full port association, outstanding-message and teardown semantics require ALPC-specific coordination; a generic work-object lifecycle is not sufficient.
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
 * \remarks Uses an ALPC-specific completion object and port association, not a TP_IO object. The Ex allocator selects the extended callback dispatch form. Full port association, outstanding-message and teardown semantics require ALPC-specific coordination; a generic work-object lifecycle is not sufficient.
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
 * \remarks Uses an ALPC-specific completion object and port association, not a TP_IO object. The Ex allocator selects the extended callback dispatch form. Full port association, outstanding-message and teardown semantics require ALPC-specific coordination; a generic work-object lifecycle is not sufficient.
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
 * \remarks Uses an ALPC-specific completion object and port association, not a TP_IO object. The Ex allocator selects the extended callback dispatch form. Full port association, outstanding-message and teardown semantics require ALPC-specific coordination; a generic work-object lifecycle is not sufficient.
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
 * \remarks Adjusts ALPC completion-list association/concurrency state on an existing valid TP_ALPC object. These are not standalone port-creation or message-receive APIs and can raise on invalid object state.
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
 * \remarks Adjusts ALPC completion-list association/concurrency state on an existing valid TP_ALPC object. These are not standalone port-creation or message-receive APIs and can raise on invalid object state.
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
 * \remarks Internal thread-pool diagnostics/worker checks. These do not submit a callback or provide a general safe worker-termination API. Keep use confined to the corresponding runtime protocol.
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
 * \remarks Internal thread-pool diagnostics/worker checks. These do not submit a callback or provide a general safe worker-termination API. Keep use confined to the corresponding runtime protocol.
 */
NTSYSAPI
VOID
NTAPI
TpCheckTerminateWorker(
    _In_ HANDLE Thread
    );

#endif // _NTTP_H
