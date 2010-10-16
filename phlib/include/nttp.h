#ifndef _NTTP_H
#define _NTTP_H

#if (PHNT_VERSION >= PHNT_VISTA)

// Worker factory

// begin_rev

typedef enum _WORKER_FACTORY_INFORMATION_CLASS
{
    WorkerFactoryAllInformation = 7,
    MaxWorkerFactoryInfoClass
} WORKER_FACTORY_INFORMATION_CLASS, *PWORKER_FACTORY_INFORMATION_CLASS;

typedef struct _WORKER_FACTORY_ALL_INFORMATION
{
    LARGE_INTEGER Timeout1;
    LARGE_INTEGER Timeout2;
    LARGE_INTEGER Timeout3;
    BOOLEAN EventSignalStateIsZero;
    BOOLEAN Reserved;
    BOOLEAN CreationTimerSet;
    BOOLEAN NoWaiters;
    BOOLEAN InCreationLogic;
    BOOLEAN Released;
    BOOLEAN Abandoned;
    ULONG WaitRelatedBoolean;
    ULONG MaximumCount2;
    ULONG MaximumCount;
    ULONG RefCount;
    ULONG WaitingCount;
    ULONG CurrentCount2;
    ULONG ReleaseCount;
    LARGE_INTEGER LargeCounter;
    PVOID WorkerThreadStart;
    PVOID WorkerThreadContext;
    HANDLE UniqueProcessId;
    SIZE_T SizeOfStackReserve;
    SIZE_T SizeOfStackCommit;
    NTSTATUS CreateThreadStatus;
} WORKER_FACTORY_ALL_INFORMATION, *PWORKER_FACTORY_ALL_INFORMATION;

typedef struct _IO_COMPLETION_MINIPACKET
{
    PVOID KeyContext;
    PVOID ApcContext;
    NTSTATUS IoStatus;
    ULONG_PTR IoStatusInformation;
} IO_COMPLETION_MINIPACKET, *PIO_COMPLETION_MINIPACKET;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateWorkerFactory(
    __out PHANDLE WorkerFactoryHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in HANDLE IoCompletionHandle,
    __in HANDLE ProcessHandle,
    __in PVOID WorkerThreadStart,
    __in_opt PVOID WorkerThreadContext,
    __in_opt ULONG MaximumCount,
    __in_opt SIZE_T SizeOfStackReserve,
    __in_opt SIZE_T SizeOfStackCommit
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationWorkerFactory(
    __in HANDLE WorkerFactoryHandle,
    __in WORKER_FACTORY_INFORMATION_CLASS WorkerFactoryInformationClass,
    __out_bcount(WorkerFactoryInformationLength) PVOID WorkerFactoryInformation,
    __in ULONG WorkerFactoryInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationWorkerFactory(
    __in HANDLE WorkerFactoryHandle,
    __in WORKER_FACTORY_INFORMATION_CLASS WorkerFactoryInformationClass,
    __in_bcount(WorkerFactoryInformationLength) PVOID WorkerFactoryInformation,
    __in ULONG WorkerFactoryInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtShutdownWorkerFactory(
    __in HANDLE WorkerFactoryHandle,
    __inout PULONG RefCount
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReleaseWorkerFactoryWorker(
    __in HANDLE WorkerFactoryHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWorkerFactoryWorkerReady(
    __in HANDLE WorkerFactoryHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForWorkViaWorkerFactory(
    __in HANDLE WorkerFactoryHandle,
    __out PIO_COMPLETION_MINIPACKET MiniPacket
    );

// end_rev

// User-mode functions

// rev
__checkReturn
NTSYSAPI
NTSTATUS
NTAPI
TpAllocPool(
    __out PTP_POOL *PoolReturn,
    __reserved PVOID Reserved
    );

// winbase:CloseThreadpool
NTSYSAPI
VOID
NTAPI
TpReleasePool(
    __inout PTP_POOL Pool
    );

// winbase:SetThreadpoolThreadMaximum
NTSYSAPI
VOID
NTAPI
TpSetPoolMaxThreads(
    __inout PTP_POOL Pool,
    __in LONG MaxThreads
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
TpSetPoolMinThreads(
    __inout PTP_POOL Pool,
    __in LONG MinThreads
    );

#if (PHNT_VERSION >= PHNT_WIN7)
// rev
NTSYSAPI
NTSTATUS
NTAPI
TpQueryPoolStackInformation(
    __in PTP_POOL Pool,
    __out PTP_POOL_STACK_INFORMATION PoolStackInformation
    );
#endif

#if (PHNT_VERSION >= PHNT_WIN7)
// rev
NTSYSAPI
NTSTATUS
NTAPI
TpSetPoolStackInformation(
    __inout PTP_POOL Pool,
    __in PTP_POOL_STACK_INFORMATION PoolStackInformation
    );
#endif

// rev
__checkReturn
NTSYSAPI
NTSTATUS
NTAPI
TpAllocCleanupGroup(
    __out PTP_CLEANUP_GROUP *CleanupGroupReturn
    );

// winbase:CloseThreadpoolCleanupGroup
NTSYSAPI
VOID
NTAPI
TpReleaseCleanupGroup(
    __inout PTP_CLEANUP_GROUP CleanupGroup
    );

// winbase:CloseThreadpoolCleanupGroupMembers
NTSYSAPI
VOID
NTAPI
TpReleaseCleanupGroupMembers(
    __inout PTP_CLEANUP_GROUP CleanupGroup,
    __in LOGICAL CancelPendingCallbacks,
    __inout_opt PVOID CleanupParameter
    );

// winbase:SetEventWhenCallbackReturns
NTSYSAPI
VOID
NTAPI
TpCallbackSetEventOnCompletion(
    __inout PTP_CALLBACK_INSTANCE Instance,
    __in HANDLE Event
    );

// winbase:ReleaseSemaphoreWhenCallbackReturns
NTSYSAPI
VOID
NTAPI
TpCallbackReleaseSemaphoreOnCompletion(
    __inout PTP_CALLBACK_INSTANCE Instance,
    __in HANDLE Semaphore,
    __in LONG ReleaseCount
    );

// winbase:ReleaseMutexWhenCallbackReturns
NTSYSAPI
VOID
NTAPI
TpCallbackReleaseMutexOnCompletion(
    __inout PTP_CALLBACK_INSTANCE Instance,
    __in HANDLE Mutex
    );

// winbase:LeaveCriticalSectionWhenCallbackReturns
NTSYSAPI
VOID
NTAPI
TpCallbackLeaveCriticalSectionOnCompletion(
    __inout PTP_CALLBACK_INSTANCE Instance,
    __inout PRTL_CRITICAL_SECTION CriticalSection
    );

// winbase:FreeLibraryWhenCallbackReturns
NTSYSAPI
VOID
NTAPI
TpCallbackUnloadDllOnCompletion(
    __inout PTP_CALLBACK_INSTANCE Instance,
    __in PVOID DllHandle
    );

// winbase:CallbackMayRunLong
NTSYSAPI
NTSTATUS
NTAPI
TpCallbackMayRunLong(
    __inout PTP_CALLBACK_INSTANCE Instance
    );

// winbase:DisassociateCurrentThreadFromCallback
NTSYSAPI
VOID
NTAPI
TpDisassociateCallback(
    __inout PTP_CALLBACK_INSTANCE Instance
    );

// winbase:TrySubmitThreadpoolCallback
__checkReturn
NTSYSAPI
NTSTATUS
NTAPI
TpSimpleTryPost(
    __in PTP_SIMPLE_CALLBACK Callback,
    __inout_opt PVOID Context,
    __in_opt PTP_CALLBACK_ENVIRON CallbackEnviron
    );

// rev
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

// winbase:CloseThreadpoolWork
NTSYSAPI
VOID
NTAPI
TpReleaseWork(
    __inout PTP_WORK Work
    );

// winbase:SubmitThreadpoolWork
NTSYSAPI
VOID
NTAPI
TpPostWork(
    __inout PTP_WORK Work
    );

// winbase:WaitForThreadpoolWorkCallbacks
NTSYSAPI
VOID
NTAPI
TpWaitForWork(
    __inout PTP_WORK Work,
    __in LOGICAL CancelPendingCallbacks
    );

// rev
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

// winbase:CloseThreadpoolTimer
NTSYSAPI
VOID
NTAPI
TpReleaseTimer(
    __inout PTP_TIMER Timer
    );

// winbase:SetThreadpoolTimer
NTSYSAPI
VOID
NTAPI
TpSetTimer(
    __inout PTP_TIMER Timer,
    __in_opt PLARGE_INTEGER DueTime,
    __in LONG Period,
    __in_opt LONG WindowLength
    );

// winbase:IsThreadpoolTimerSet
NTSYSAPI
LOGICAL
NTAPI
TpIsTimerSet(
    __in PTP_TIMER Timer
    );

// winbase:WaitForThreadpoolTimerCallbacks
NTSYSAPI
VOID
NTAPI
TpWaitForTimer(
    __inout PTP_TIMER Timer,
    __in LOGICAL CancelPendingCallbacks
    );

// rev
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

// winbase:CloseThreadpoolWait
NTSYSAPI
VOID
NTAPI
TpReleaseWait(
    __inout PTP_WAIT Wait
    );

// winbase:SetThreadpoolWait
NTSYSAPI
VOID
NTAPI
TpSetWait(
    __inout PTP_WAIT Wait,
    __in_opt HANDLE Handle,
    __in_opt PLARGE_INTEGER Timeout
    );

// winbase:WaitForThreadpoolWaitCallbacks
NTSYSAPI
VOID
NTAPI
TpWaitForWait(
    __inout PTP_WAIT Wait,
    __in LOGICAL CancelPendingCallbacks
    );

// rev
typedef VOID (NTAPI *PTP_IO_CALLBACK)(
    __inout PTP_CALLBACK_INSTANCE Instance,
    __inout_opt PVOID Context,
    __in PVOID ApcContext,
    __in PIO_STATUS_BLOCK IoSB,
    __in PTP_IO Io
    );

// rev
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

// winbase:CloseThreadpoolIo
NTSYSAPI
VOID
NTAPI
TpReleaseIoCompletion(
    __inout PTP_IO Io
    );

// winbase:StartThreadpoolIo
NTSYSAPI
VOID
NTAPI
TpStartAsyncIoOperation(
    __inout PTP_IO Io
    );

// winbase:CancelThreadpoolIo
NTSYSAPI
VOID
NTAPI
TpCancelAsyncIoOperation(
    __inout PTP_IO Io
    );

// winbase:WaitForThreadpoolIoCallbacks
NTSYSAPI
VOID
NTAPI
TpWaitForIoCompletion(
    __inout PTP_IO Io,
    __in LOGICAL CancelPendingCallbacks
    );

// begin_rev

typedef struct _TP_ALPC TP_ALPC, *PTP_ALPC;

// unsure
typedef VOID (NTAPI *PTP_ALPC_CALLBACK)(
    __inout PTP_CALLBACK_INSTANCE Instance,
    __inout_opt PVOID Context,
    __in PTP_ALPC Alpc // ?
    );

// unsure
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

#if (PHNT_VERSION >= PHNT_WIN7)
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
#endif

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

// end_rev

// rev
NTSYSAPI
VOID
NTAPI
TpCheckTerminateWorker(
    __in_opt HANDLE Thread
    );

#endif

#endif
