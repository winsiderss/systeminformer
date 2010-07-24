#ifndef _PH_PHDK_H
#define _PH_PHDK_H

#pragma once

#define PHLIB_IMPORT
#define PHAPPAPI __declspec(dllimport)

#include "ph.h"
#include "phplug.h"

#include "circbuf.h"
#include "providers.h"

// phapp

PHAPPAPI
VOID PhRegisterDialog(
    __in HWND DialogWindowHandle
    );

PHAPPAPI
VOID PhUnregisterDialog(
    __in HWND DialogWindowHandle
    );

PHAPPAPI extern HWND PhMainWndHandle;

#define ProcessHacker_ShowProcessProperties(hWnd, ProcessItem) \
    SendMessage(hWnd, WM_PH_SHOW_PROCESS_PROPERTIES, 0, (LPARAM)(ProcessItem))
#define ProcessHacker_Destroy(hWnd) \
    SendMessage(hWnd, WM_PH_DESTROY, 0, 0)
#define ProcessHacker_ToggleVisible(hWnd) \
    SendMessage(hWnd, WM_PH_TOGGLE_VISIBLE, 0, 0)

PHAPPAPI
VOID PhLogMessageEntry(
    __in UCHAR Type,
    __in PPH_STRING Message
    );

PHAPPAPI
BOOLEAN PhUiLockComputer(
    __in HWND hWnd
    );

PHAPPAPI
BOOLEAN PhUiLogoffComputer(
    __in HWND hWnd
    );

PHAPPAPI
BOOLEAN PhUiSleepComputer(
    __in HWND hWnd
    );

PHAPPAPI
BOOLEAN PhUiHibernateComputer(
    __in HWND hWnd
    );

PHAPPAPI
BOOLEAN PhUiRestartComputer(
    __in HWND hWnd
    );

PHAPPAPI
BOOLEAN PhUiShutdownComputer(
    __in HWND hWnd
    );

PHAPPAPI
BOOLEAN PhUiPoweroffComputer(
    __in HWND hWnd
    );

PHAPPAPI
BOOLEAN PhUiConnectSession(
    __in HWND hWnd,
    __in ULONG SessionId
    );

PHAPPAPI
BOOLEAN PhUiDisconnectSession(
    __in HWND hWnd,
    __in ULONG SessionId
    );

PHAPPAPI
BOOLEAN PhUiLogoffSession(
    __in HWND hWnd,
    __in ULONG SessionId
    );

PHAPPAPI
BOOLEAN PhUiTerminateProcesses(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
    );

PHAPPAPI
BOOLEAN PhUiTerminateTreeProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN PhUiSuspendProcesses(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
    );

PHAPPAPI
BOOLEAN PhUiResumeProcesses(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
    );

PHAPPAPI
BOOLEAN PhUiRestartProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN PhUiDebugProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN PhUiReduceWorkingSetProcesses(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM *Processes,
    __in ULONG NumberOfProcesses
    );

PHAPPAPI
BOOLEAN PhUiSetVirtualizationProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process,
    __in BOOLEAN Enable
    );

PHAPPAPI
BOOLEAN PhUiDetachFromDebuggerProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN PhUiInjectDllProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN PhUiSetIoPriorityProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process,
    __in ULONG IoPriority
    );

PHAPPAPI
BOOLEAN PhUiSetPriorityProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process,
    __in ULONG PriorityClassWin32
    );

PHAPPAPI
BOOLEAN PhUiSetDepStatusProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN PhUiSetProtectionProcess(
    __in HWND hWnd,
    __in PPH_PROCESS_ITEM Process
    );

PHAPPAPI
BOOLEAN PhUiStartService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN PhUiContinueService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN PhUiPauseService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN PhUiStopService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN PhUiDeleteService(
    __in HWND hWnd,
    __in PPH_SERVICE_ITEM Service
    );

PHAPPAPI
BOOLEAN PhUiCloseConnections(
    __in HWND hWnd,
    __in PPH_NETWORK_ITEM *Connections,
    __in ULONG NumberOfConnections
    );

PHAPPAPI
BOOLEAN PhUiTerminateThreads(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM *Threads,
    __in ULONG NumberOfThreads
    );

PHAPPAPI
BOOLEAN PhUiForceTerminateThreads(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_THREAD_ITEM *Threads,
    __in ULONG NumberOfThreads
    );

PHAPPAPI
BOOLEAN PhUiSuspendThreads(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM *Threads,
    __in ULONG NumberOfThreads
    );

PHAPPAPI
BOOLEAN PhUiResumeThreads(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM *Threads,
    __in ULONG NumberOfThreads
    );

PHAPPAPI
BOOLEAN PhUiSetPriorityThread(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM Thread,
    __in ULONG ThreadPriorityWin32
    );

PHAPPAPI
BOOLEAN PhUiSetIoPriorityThread(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM Thread,
    __in ULONG IoPriority
    );

PHAPPAPI
BOOLEAN PhUiUnloadModule(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_MODULE_ITEM Module
    );

PHAPPAPI
BOOLEAN PhUiFreeMemory(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_MEMORY_ITEM MemoryItem,
    __in BOOLEAN Free
    );

PHAPPAPI
BOOLEAN PhUiCloseHandles(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_HANDLE_ITEM *Handles,
    __in ULONG NumberOfHandles,
    __in BOOLEAN Warn
    );

PHAPPAPI
BOOLEAN PhUiSetAttributesHandle(
    __in HWND hWnd,
    __in HANDLE ProcessId,
    __in PPH_HANDLE_ITEM Handle,
    __in ULONG Attributes
    );

// settings

typedef enum _PH_SETTING_TYPE
{
    StringSettingType,
    IntegerSettingType,
    IntegerPairSettingType
} PH_SETTING_TYPE, PPH_SETTING_TYPE;

PHAPPAPI
__mayRaise ULONG PhGetIntegerSetting(
    __in PWSTR Name
    );

PHAPPAPI
__mayRaise PH_INTEGER_PAIR PhGetIntegerPairSetting(
    __in PWSTR Name
    );

PHAPPAPI
__mayRaise PPH_STRING PhGetStringSetting(
    __in PWSTR Name
    );

PHAPPAPI
__mayRaise VOID PhSetIntegerSetting(
    __in PWSTR Name,
    __in ULONG Value
    );

PHAPPAPI
__mayRaise VOID PhSetIntegerPairSetting(
    __in PWSTR Name,
    __in PH_INTEGER_PAIR Value
    );

PHAPPAPI
__mayRaise VOID PhSetStringSetting(
    __in PWSTR Name,
    __in PWSTR Value
    );

PHAPPAPI
__mayRaise VOID PhSetStringSetting2(
    __in PWSTR Name,
    __in PPH_STRINGREF Value
    );

// High-level settings creation

typedef struct _PH_CREATE_SETTING
{
    PH_SETTING_TYPE Type;
    PWSTR Name;
    PWSTR DefaultValue;
} PH_SETTING_CREATE, *PPH_SETTING_CREATE;

PHAPPAPI
VOID PhAddSettings(
    __in PPH_SETTING_CREATE Settings,
    __in ULONG NumberOfSettings
    );

#endif
