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
typedef _Function_class_(TP_ALPC_CALLBACK)
VOID NTAPI TP_ALPC_CALLBACK(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID Context,
    _In_ PTP_ALPC Alpc
    );
typedef TP_ALPC_CALLBACK *PTP_ALPC_CALLBACK;

// rev
typedef _Function_class_(TP_ALPC_CALLBACK_EX)
VOID NTAPI TP_ALPC_CALLBACK_EX(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID Context,
    _In_ PTP_ALPC Alpc,
    _In_ PVOID ApcContext
    );
typedef TP_ALPC_CALLBACK_EX *PTP_ALPC_CALLBACK_EX;

#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)

// winbase:CreateThreadpool
NTSYSAPI
NTSTATUS
NTAPI
TpAllocPool(
    _Out_ PTP_POOL *PoolReturn,
    _Reserved_ PVOID Reserved
    );

// winbase:CloseThreadpool
NTSYSAPI
VOID
NTAPI
TpReleasePool(
    _Inout_ PTP_POOL Pool
    );

// winbase:SetThreadpoolThreadMaximum
NTSYSAPI
VOID
NTAPI
TpSetPoolMaxThreads(
    _Inout_ PTP_POOL Pool,
    _In_ ULONG MaxThreads
    );

// winbase:SetThreadpoolThreadMinimum
NTSYSAPI
NTSTATUS
NTAPI
TpSetPoolMinThreads(
    _Inout_ PTP_POOL Pool,
    _In_ ULONG MinThreads
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_7)
// winbase:QueryThreadpoolStackInformation
NTSYSAPI
NTSTATUS
NTAPI
TpQueryPoolStackInformation(
    _In_ PTP_POOL Pool,
    _Out_ PTP_POOL_STACK_INFORMATION PoolStackInformation
    );

// winbase:SetThreadpoolStackInformation
NTSYSAPI
NTSTATUS
NTAPI
TpSetPoolStackInformation(
    _Inout_ PTP_POOL Pool,
    _In_ PTP_POOL_STACK_INFORMATION PoolStackInformation
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
TpSetPoolThreadBasePriority(
    _Inout_ PTP_POOL Pool,
    _In_ ULONG BasePriority
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_7)

// winbase:CreateThreadpoolCleanupGroup
NTSYSAPI
NTSTATUS
NTAPI
TpAllocCleanupGroup(
    _Out_ PTP_CLEANUP_GROUP *CleanupGroupReturn
    );

// winbase:CloseThreadpoolCleanupGroup
NTSYSAPI
VOID
NTAPI
TpReleaseCleanupGroup(
    _Inout_ PTP_CLEANUP_GROUP CleanupGroup
    );

// winbase:CloseThreadpoolCleanupGroupMembers
NTSYSAPI
VOID
NTAPI
TpReleaseCleanupGroupMembers(
    _Inout_ PTP_CLEANUP_GROUP CleanupGroup,
    _In_ LOGICAL CancelPendingCallbacks,
    _Inout_opt_ PVOID CleanupParameter
    );

// winbase:SetEventWhenCallbackReturns
NTSYSAPI
VOID
NTAPI
TpCallbackSetEventOnCompletion(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _In_ HANDLE Event
    );

// winbase:ReleaseSemaphoreWhenCallbackReturns
NTSYSAPI
VOID
NTAPI
TpCallbackReleaseSemaphoreOnCompletion(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _In_ HANDLE Semaphore,
    _In_ ULONG ReleaseCount
    );

// winbase:ReleaseMutexWhenCallbackReturns
NTSYSAPI
VOID
NTAPI
TpCallbackReleaseMutexOnCompletion(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _In_ HANDLE Mutex
    );

// winbase:LeaveCriticalSectionWhenCallbackReturns
NTSYSAPI
VOID
NTAPI
TpCallbackLeaveCriticalSectionOnCompletion(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    );

// winbase:FreeLibraryWhenCallbackReturns
NTSYSAPI
VOID
NTAPI
TpCallbackUnloadDllOnCompletion(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _In_ PVOID DllHandle
    );

// winbase:CallbackMayRunLong
NTSYSAPI
NTSTATUS
NTAPI
TpCallbackMayRunLong(
    _Inout_ PTP_CALLBACK_INSTANCE Instance
    );

// winbase:DisassociateCurrentThreadFromCallback
NTSYSAPI
VOID
NTAPI
TpDisassociateCallback(
    _Inout_ PTP_CALLBACK_INSTANCE Instance
    );

// winbase:TrySubmitThreadpoolCallback
NTSYSAPI
NTSTATUS
NTAPI
TpSimpleTryPost(
    _In_ PTP_SIMPLE_CALLBACK Callback,
    _Inout_opt_ PVOID Context,
    _In_opt_ PTP_CALLBACK_ENVIRON CallbackEnviron
    );

// winbase:CreateThreadpoolWork
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
NTSYSAPI
VOID
NTAPI
TpReleaseWork(
    _Inout_ PTP_WORK Work
    );

// winbase:SubmitThreadpoolWork
NTSYSAPI
VOID
NTAPI
TpPostWork(
    _Inout_ PTP_WORK Work
    );

// winbase:WaitForThreadpoolWorkCallbacks
NTSYSAPI
VOID
NTAPI
TpWaitForWork(
    _Inout_ PTP_WORK Work,
    _In_ LOGICAL CancelPendingCallbacks
    );

// winbase:CreateThreadpoolTimer
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
NTSYSAPI
VOID
NTAPI
TpReleaseTimer(
    _Inout_ PTP_TIMER Timer
    );

// winbase:SetThreadpoolTimer
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
NTSYSAPI
LOGICAL
NTAPI
TpIsTimerSet(
    _In_ PTP_TIMER Timer
    );

// winbase:WaitForThreadpoolTimerCallbacks
NTSYSAPI
VOID
NTAPI
TpWaitForTimer(
    _Inout_ PTP_TIMER Timer,
    _In_ LOGICAL CancelPendingCallbacks
    );

// winbase:CreateThreadpoolWait
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
NTSYSAPI
VOID
NTAPI
TpReleaseWait(
    _Inout_ PTP_WAIT Wait
    );

// winbase:SetThreadpoolWait
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
NTSYSAPI
VOID
NTAPI
TpWaitForWait(
    _Inout_ PTP_WAIT Wait,
    _In_ LOGICAL CancelPendingCallbacks
    );

// private
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
NTSYSAPI
VOID
NTAPI
TpReleaseIoCompletion(
    _Inout_ PTP_IO Io
    );

// winbase:StartThreadpoolIo
NTSYSAPI
VOID
NTAPI
TpStartAsyncIoOperation(
    _Inout_ PTP_IO Io
    );

// winbase:CancelThreadpoolIo
NTSYSAPI
VOID
NTAPI
TpCancelAsyncIoOperation(
    _Inout_ PTP_IO Io
    );

// winbase:WaitForThreadpoolIoCallbacks
NTSYSAPI
VOID
NTAPI
TpWaitForIoCompletion(
    _Inout_ PTP_IO Io,
    _In_ LOGICAL CancelPendingCallbacks
    );

// private
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

#if (PHNT_VERSION >= PHNT_WINDOWS_7)
// rev
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
#endif

// private
NTSYSAPI
VOID
NTAPI
TpReleaseAlpcCompletion(
    _Inout_ PTP_ALPC Alpc
    );

// private
NTSYSAPI
VOID
NTAPI
TpWaitForAlpcCompletion(
    _Inout_ PTP_ALPC Alpc
    );

// rev
NTSYSAPI
VOID
NTAPI
TpAlpcRegisterCompletionList(
    _Inout_ PTP_ALPC Alpc
    );

// rev
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
NTSYSAPI
VOID
NTAPI
TpCaptureCaller(
    _In_ TP_TRACE_TYPE Type
    );

// private
NTSYSAPI
VOID
NTAPI
TpCheckTerminateWorker(
    _In_ HANDLE Thread
    );

#endif // (PHNT_VERSION >= PHNT_WINDOWS_VISTA)

#endif // _NTTP_H
