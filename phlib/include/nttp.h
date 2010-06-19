#ifndef NTTP_H
#define NTTP_H

#if (PHNT_VERSION >= PHNT_VISTA)

// This header file provides thread pool definitions.
// Source: Windows SDK, reverse engineering

__checkReturn
NTSYSAPI
NTSTATUS
NTAPI
TpAllocPool(
    __out PTP_POOL *PoolReturn,
    __in __reserved PVOID Reserved
    );

NTSYSAPI
VOID
NTAPI
TpReleasePool(
    __inout PTP_POOL Pool
    );

NTSYSAPI
VOID
NTAPI
TpSetPoolMaxThreads(
    __inout PTP_POOL Pool,
    __in LONG MaxThreads
    );

NTSYSAPI
NTSTATUS
NTAPI
TpSetPoolMinThreads(
    __inout PTP_POOL Pool,
    __in LONG MinThreads
    );

#if (PHNT_VERSION >= PHNT_WIN7)
NTSYSAPI
NTSTATUS
NTAPI
TpQueryPoolStackInformation(
    __in PTP_POOL Pool,
    __out PTP_POOL_STACK_INFORMATION PoolStackInformation
    );
#endif

#if (PHNT_VERSION >= PHNT_WIN7)
NTSYSAPI
NTSTATUS
NTAPI
TpSetPoolStackInformation(
    __inout PTP_POOL Pool,
    __in PTP_POOL_STACK_INFORMATION PoolStackInformation
    );
#endif

__checkReturn
NTSYSAPI
NTSTATUS
NTAPI
TpAllocCleanupGroup(
    __out PTP_CLEANUP_GROUP *CleanupGroupReturn
    );

NTSYSAPI
VOID
NTAPI
TpReleaseCleanupGroup(
    __inout PTP_CLEANUP_GROUP CleanupGroup
    );

NTSYSAPI
VOID
NTAPI
TpReleaseCleanupGroupMembers(
    __inout PTP_CLEANUP_GROUP CleanupGroup,
    __in LOGICAL CancelPendingCallbacks,
    __inout_opt PVOID CleanupParameter
    );

NTSYSAPI
VOID
NTAPI
TpCallbackSetEventOnCompletion(
    __inout PTP_CALLBACK_INSTANCE Instance,
    __in HANDLE Event
    );

NTSYSAPI
VOID
NTAPI
TpCallbackReleaseSemaphoreOnCompletion(
    __inout PTP_CALLBACK_INSTANCE Instance,
    __in HANDLE Semaphore,
    __in LONG ReleaseCount
    );

NTSYSAPI
VOID
NTAPI
TpCallbackReleaseMutexOnCompletion(
    __inout PTP_CALLBACK_INSTANCE Instance,
    __in HANDLE Mutex
    );

NTSYSAPI
VOID
NTAPI
TpCallbackLeaveCriticalSectionOnCompletion(
    __inout PTP_CALLBACK_INSTANCE Instance,
    __inout PRTL_CRITICAL_SECTION CriticalSection
    );

NTSYSAPI
VOID
NTAPI
TpCallbackUnloadDllOnCompletion(
    __inout PTP_CALLBACK_INSTANCE Instance,
    __in PVOID DllHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
TpCallbackMayRunLong(
    __inout PTP_CALLBACK_INSTANCE Instance
    );

NTSYSAPI
VOID
NTAPI
TpDisassociateCallback(
    __inout PTP_CALLBACK_INSTANCE Instance
    );

__checkReturn
NTSYSAPI
NTSTATUS
NTAPI
TpSimpleTryPost(
    __in PTP_SIMPLE_CALLBACK Callback,
    __inout_opt PVOID Context,
    __in_opt PTP_CALLBACK_ENVIRON CallbackEnviron
    );

__checkReturn
NTSYSAPI
NTSTATUS
NTAPI
TpAllocWork(
    __out PTP_WORK *WorkReturn,
    __in PTP_WORK_CALLBACK Callback,
    __inout_opt PVOID Context,
    __in_opt PTP_CALLBACK_ENVIRON CallbackEnviron
    );

NTSYSAPI
VOID
NTAPI
TpReleaseWork(
    __inout PTP_WORK Work
    );

NTSYSAPI
VOID
NTAPI
TpPostWork(
    __inout PTP_WORK Work
    );

NTSYSAPI
VOID
NTAPI
TpWaitForWork(
    __inout PTP_WORK Work,
    __in LOGICAL CancelPendingCallbacks
    );

__checkReturn
NTSYSAPI
NTSTATUS
NTAPI
TpAllocTimer(
    __out PTP_TIMER *Timer,
    __in PTP_TIMER_CALLBACK Callback,
    __inout_opt PVOID Context,
    __in_opt PTP_CALLBACK_ENVIRON CallbackEnviron
    );

NTSYSAPI
VOID
NTAPI
TpReleaseTimer(
    __inout PTP_TIMER Timer
    );

NTSYSAPI
VOID
NTAPI
TpSetTimer(
    __inout PTP_TIMER Timer,
    __in_opt PLARGE_INTEGER DueTime,
    __in LONG Period,
    __in_opt LONG WindowLength
    );

NTSYSAPI
LOGICAL
NTAPI
TpIsTimerSet(
    __in PTP_TIMER Timer
    );

NTSYSAPI
VOID
NTAPI
TpWaitForTimer(
    __inout PTP_TIMER Timer,
    __in LOGICAL CancelPendingCallbacks
    );

__checkReturn
NTSYSAPI
NTSTATUS
NTAPI
TpAllocWait(
    __out PTP_WAIT *WaitReturn,
    __in PTP_WAIT_CALLBACK Callback,
    __inout_opt PVOID Context,
    __in_opt PTP_CALLBACK_ENVIRON CallbackEnviron
    );

NTSYSAPI
VOID
NTAPI
TpReleaseWait(
    __inout PTP_WAIT Wait
    );

NTSYSAPI
VOID
NTAPI
TpSetWait(
    __inout PTP_WAIT Wait,
    __in_opt HANDLE Handle,
    __in_opt PLARGE_INTEGER Timeout
    );

NTSYSAPI
VOID
NTAPI
TpWaitForWait(
    __inout PTP_WAIT Wait,
    __in LOGICAL CancelPendingCallbacks
    );

typedef VOID (NTAPI *PTP_IO_CALLBACK)(
    __inout PTP_CALLBACK_INSTANCE Instance,
    __inout_opt PVOID Context,
    __in PVOID ApcContext,
    __in PIO_STATUS_BLOCK IoSB,
    __in PTP_IO Io
    );

__checkReturn
NTSYSAPI
NTSTATUS
NTAPI
TpAllocIoCompletion(
    __out PTP_IO *IoReturn,
    __in HANDLE FileHandle,
    __in PTP_IO_CALLBACK Callback,
    __inout_opt PVOID Context,
    __in_opt PTP_CALLBACK_ENVIRON CallbackEnviron
    );

NTSYSAPI
VOID
NTAPI
TpReleaseIoCompletion(
    __inout PTP_IO Io
    );

NTSYSAPI
VOID
NTAPI
TpStartAsyncIoOperation(
    __inout PTP_IO Io
    );

NTSYSAPI
VOID
NTAPI
TpCancelAsyncIoOperation(
    __inout PTP_IO Io
    );

NTSYSAPI
VOID
NTAPI
TpWaitForIoCompletion(
    __inout PTP_IO Io,
    __in LOGICAL CancelPendingCallbacks
    );

typedef struct _TP_ALPC TP_ALPC, *PTP_ALPC;

typedef VOID (NTAPI *PTP_ALPC_CALLBACK)(
    __inout PTP_CALLBACK_INSTANCE Instance,
    __inout_opt PVOID Context,
    __in PTP_ALPC Alpc // ?
    );

typedef VOID (NTAPI *PTP_ALPC_CALLBACK_EX)(
    __inout PTP_CALLBACK_INSTANCE Instance,
    __inout_opt PVOID Context,
    __in PTP_ALPC Alpc, // ?
    __in PVOID AlpcContext // ?
    );

NTSYSAPI
NTSTATUS
NTAPI
TpAllocAlpcCompletion(
    __out PTP_ALPC *AlpcReturn,
    __in HANDLE AlpcPort,
    __in PTP_ALPC_CALLBACK Callback,
    __inout_opt PVOID Context,
    __in_opt PTP_CALLBACK_ENVIRON CallbackEnviron
    );

NTSYSAPI
NTSTATUS
NTAPI
TpAllocAlpcCompletionEx(
    __out PTP_ALPC *AlpcReturn,
    __in HANDLE AlpcPort,
    __in PTP_ALPC_CALLBACK_EX Callback,
    __inout_opt PVOID Context,
    __in_opt PTP_CALLBACK_ENVIRON CallbackEnviron
    );

NTSYSAPI
VOID
NTAPI
TpReleaseAlpcCompletion(
    __inout PTP_ALPC Alpc
    );

NTSYSAPI
VOID
NTAPI
TpWaitForAlpcCompletion(
    __inout PTP_ALPC Alpc
    );

NTSYSAPI
VOID
NTAPI
TpCheckTerminateWorker(
    __in_opt HANDLE Thread
    );

#endif

#endif
