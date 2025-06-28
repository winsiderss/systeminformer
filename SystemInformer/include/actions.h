/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     dmex    2018-2023
 *
 */

#ifndef PH_ACTIONS_H
#define PH_ACTIONS_H

EXTERN_C_START

typedef enum _PH_ACTION_ELEVATION_LEVEL
{
    NeverElevateAction = 0,
    PromptElevateAction = 1,
    AlwaysElevateAction = 2
} PH_ACTION_ELEVATION_LEVEL;

// begin_phapppub
typedef enum _PH_PHSVC_MODE
{
    ElevatedPhSvcMode,
    Wow64PhSvcMode
} PH_PHSVC_MODE;

PHAPPAPI
BOOLEAN
NTAPI
PhUiConnectToPhSvc(
    _In_opt_ HWND WindowHandle,
    _In_ BOOLEAN ConnectOnly
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiConnectToPhSvcEx(
    _In_opt_ HWND WindowHandle,
    _In_ PH_PHSVC_MODE Mode,
    _In_ BOOLEAN ConnectOnly
    );

PHAPPAPI
VOID
NTAPI
PhUiDisconnectFromPhSvc(
    VOID
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiLockComputer(
    _In_ HWND WindowHandle
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiLogoffComputer(
    _In_ HWND WindowHandle
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSleepComputer(
    _In_ HWND WindowHandle
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiHibernateComputer(
    _In_ HWND WindowHandle
    );

typedef enum _PH_POWERACTION_TYPE
{
    PH_POWERACTION_TYPE_NONE,
    PH_POWERACTION_TYPE_WIN32,
    PH_POWERACTION_TYPE_NATIVE,
    PH_POWERACTION_TYPE_CRITICAL,
    PH_POWERACTION_TYPE_ADVANCEDBOOT,
    PH_POWERACTION_TYPE_FIRMWAREBOOT,
    PH_POWERACTION_TYPE_UPDATE,
    PH_POWERACTION_TYPE_WDOSCAN,
    PH_POWERACTION_TYPE_MAXIMUM
} PH_POWERACTION_TYPE;

PHAPPAPI
BOOLEAN
NTAPI
PhUiRestartComputer(
    _In_ HWND WindowHandle,
    _In_ PH_POWERACTION_TYPE Action,
    _In_ ULONG Flags
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiShutdownComputer(
    _In_ HWND WindowHandle,
    _In_ PH_POWERACTION_TYPE Action,
    _In_ ULONG Flags
    );

PVOID PhUiCreateComputerBootDeviceMenu(
    _In_ BOOLEAN DelayLoadMenu
    );

PVOID PhUiCreateComputerFirmwareDeviceMenu(
    _In_ BOOLEAN DelayLoadMenu
    );

VOID PhUiHandleComputerBootApplicationMenu(
    _In_ HWND WindowHandle,
    _In_ ULONG MenuIndex
    );

VOID PhUiHandleComputerFirmwareApplicationMenu(
    _In_ HWND WindowHandle,
    _In_ ULONG MenuIndex
    );

VOID PhUiCreateSessionMenu(
    _In_ PVOID UsersMenuItem
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiConnectSession(
    _In_ HWND WindowHandle,
    _In_ ULONG SessionId
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiDisconnectSession(
    _In_ HWND WindowHandle,
    _In_ ULONG SessionId
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiLogoffSession(
    _In_ HWND WindowHandle,
    _In_ ULONG SessionId
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiTerminateProcesses(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiTerminateTreeProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSuspendProcesses(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSuspendTreeProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiResumeProcesses(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiResumeTreeProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiFreezeTreeProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiThawTreeProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiRestartProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiDebugProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiReduceWorkingSetProcesses(
    _In_ HWND WindowHandle,
    _In_ CONST PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetEmptyWorkingSetProcesses(
    _In_ HWND WindowHandle,
    _In_ CONST PPH_PROCESS_ITEM* Processes,
    _In_ ULONG NumberOfProcesses
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetActivityModeration(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetVirtualizationProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process,
    _In_ BOOLEAN Enable
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetCriticalProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetEcoModeProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiDetachFromDebuggerProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetExecutionRequiredProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiLoadDllProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetIoPriorityProcesses(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses,
    _In_ IO_PRIORITY_HINT IoPriority
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetPagePriorityProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process,
    _In_ ULONG PagePriority
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetPriorityProcesses(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses,
    _In_ ULONG PriorityClass
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetBoostPriorityProcesses(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM* Processes,
    _In_ ULONG NumberOfProcesses,
    _In_ BOOLEAN PriorityBoost
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetBoostPriorityProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process,
    _In_ BOOLEAN PriorityBoost
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiStartServices(
    _In_ HWND WindowHandle,
    _In_ PPH_SERVICE_ITEM* Services,
    _In_ ULONG NumberOfServices
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiStartService(
    _In_ HWND WindowHandle,
    _In_ PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiContinueServices(
    _In_ HWND WindowHandle,
    _In_ PPH_SERVICE_ITEM* Services,
    _In_ ULONG NumberOfServices
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiContinueService(
    _In_ HWND WindowHandle,
    _In_ PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiPauseServices(
    _In_ HWND WindowHandle,
    _In_ PPH_SERVICE_ITEM* Services,
    _In_ ULONG NumberOfServices
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiPauseService(
    _In_ HWND WindowHandle,
    _In_ PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiStopServices(
    _In_ HWND WindowHandle,
    _In_ PPH_SERVICE_ITEM* Services,
    _In_ ULONG NumberOfServices
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiStopService(
    _In_ HWND WindowHandle,
    _In_ PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiDeleteService(
    _In_ HWND WindowHandle,
    _In_ PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiRestartServices(
    _In_ HWND WindowHandle,
    _In_ PPH_SERVICE_ITEM* Services,
    _In_ ULONG NumberOfServices
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiCloseConnections(
    _In_ HWND WindowHandle,
    _In_ PPH_NETWORK_ITEM *Connections,
    _In_ ULONG NumberOfConnections
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiTerminateThreads(
    _In_ HWND WindowHandle,
    _In_ PPH_THREAD_ITEM *Threads,
    _In_ ULONG NumberOfThreads
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSuspendThreads(
    _In_ HWND WindowHandle,
    _In_ PPH_THREAD_ITEM *Threads,
    _In_ ULONG NumberOfThreads
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiResumeThreads(
    _In_ HWND WindowHandle,
    _In_ PPH_THREAD_ITEM *Threads,
    _In_ ULONG NumberOfThreads
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetBoostPriorityThreads(
    _In_ HWND WindowHandle,
    _In_ PPH_THREAD_ITEM* Threads,
    _In_ ULONG NumberOfThreads,
    _In_ BOOLEAN PriorityBoost
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetBoostPriorityThread(
    _In_ HWND WindowHandle,
    _In_ PPH_THREAD_ITEM Thread,
    _In_ BOOLEAN PriorityBoost
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetPriorityThreads(
    _In_ HWND WindowHandle,
    _In_ PPH_THREAD_ITEM* Threads,
    _In_ ULONG NumberOfThreads,
    _In_ LONG Increment
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetPriorityThread(
    _In_ HWND WindowHandle,
    _In_ PPH_THREAD_ITEM Thread,
    _In_ LONG Increment
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetIoPriorityThread(
    _In_ HWND WindowHandle,
    _In_ PPH_THREAD_ITEM Thread,
    _In_ IO_PRIORITY_HINT IoPriority
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetPagePriorityThread(
    _In_ HWND WindowHandle,
    _In_ PPH_THREAD_ITEM Thread,
    _In_ ULONG PagePriority
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiUnloadModule(
    _In_ HWND WindowHandle,
    _In_ HANDLE ProcessId,
    _In_ PPH_MODULE_ITEM Module
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiFreeMemory(
    _In_ HWND WindowHandle,
    _In_ HANDLE ProcessId,
    _In_ PPH_MEMORY_ITEM MemoryItem,
    _In_ BOOLEAN Free
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiEmptyProcessMemoryWorkingSet(
    _In_ HWND WindowHandle,
    _In_ HANDLE ProcessId,
    _In_ PPH_MEMORY_ITEM MemoryItem
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiCloseHandles(
    _In_ HWND WindowHandle,
    _In_ HANDLE ProcessId,
    _In_ PPH_HANDLE_ITEM *Handles,
    _In_ ULONG NumberOfHandles,
    _In_ BOOLEAN Warn
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiSetAttributesHandle(
    _In_ HWND WindowHandle,
    _In_ HANDLE ProcessId,
    _In_ PPH_HANDLE_ITEM Handle,
    _In_ ULONG Attributes
    );

PHAPPAPI
BOOLEAN
NTAPI
PhUiFlushHeapProcesses(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM* Processes,
    _In_ ULONG NumberOfProcesses
    );
// end_phapppub

EXTERN_C_END

#endif
