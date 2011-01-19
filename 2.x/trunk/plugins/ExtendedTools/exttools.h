#ifndef EXTTOOLS_H
#define EXTTOOLS_H

#define PHNT_VERSION PHNT_VISTA
#include <phdk.h>

extern PPH_PLUGIN PluginInstance;

#define SETTING_PREFIX L"ProcessHacker.ExtendedTools."
#define SETTING_NAME_ENABLE_ETW_MONITOR (SETTING_PREFIX L"EnableEtwMonitor")
#define SETTING_NAME_ETWSYS_ALWAYS_ON_TOP (SETTING_PREFIX L"EtwSysAlwaysOnTop")
#define SETTING_NAME_ETWSYS_WINDOW_POSITION (SETTING_PREFIX L"EtwSysWindowPosition")
#define SETTING_NAME_ETWSYS_WINDOW_SIZE (SETTING_PREFIX L"EtwSysWindowSize")
#define SETTING_NAME_MEMORY_LISTS_WINDOW_POSITION (SETTING_PREFIX L"MemoryListsWindowPosition")

// etwmon

extern BOOLEAN EtEtwEnabled;

// etwprprp

VOID EtEtwProcessPropertiesInitializing(
    __in PVOID Parameter
    );

// etwstat

extern ULONG EtDiskReadCount;
extern ULONG EtDiskWriteCount;
extern ULONG EtNetworkReceiveCount;
extern ULONG EtNetworkSendCount;

extern PH_UINT32_DELTA EtDiskReadDelta;
extern PH_UINT32_DELTA EtDiskWriteDelta;
extern PH_UINT32_DELTA EtNetworkReceiveDelta;
extern PH_UINT32_DELTA EtNetworkSendDelta;

extern PH_CIRCULAR_BUFFER_ULONG EtDiskReadHistory;
extern PH_CIRCULAR_BUFFER_ULONG EtDiskWriteHistory;
extern PH_CIRCULAR_BUFFER_ULONG EtNetworkReceiveHistory;
extern PH_CIRCULAR_BUFFER_ULONG EtNetworkSendHistory;
extern PH_CIRCULAR_BUFFER_ULONG EtMaxDiskHistory;
extern PH_CIRCULAR_BUFFER_ULONG EtMaxNetworkHistory;

typedef struct _ET_PROCESS_ETW_BLOCK
{
    LONG RefCount;
    PPH_PROCESS_ITEM ProcessItem;

    ULONG DiskReadCount;
    ULONG DiskWriteCount;
    ULONG NetworkReceiveCount;
    ULONG NetworkSendCount;

    ULONG DiskReadRaw;
    ULONG DiskWriteRaw;
    ULONG NetworkReceiveRaw;
    ULONG NetworkSendRaw;

    PH_UINT32_DELTA DiskReadDelta;
    PH_UINT32_DELTA DiskWriteDelta;
    PH_UINT32_DELTA NetworkReceiveDelta;
    PH_UINT32_DELTA NetworkSendDelta;
} ET_PROCESS_ETW_BLOCK, *PET_PROCESS_ETW_BLOCK;

VOID EtEtwStatisticsInitialization();

VOID EtEtwStatisticsUninitialization();

PET_PROCESS_ETW_BLOCK EtCreateProcessEtwBlock(
    __in PPH_PROCESS_ITEM ProcessItem
    );

VOID EtReferenceProcessEtwBlock(
    __inout PET_PROCESS_ETW_BLOCK Block
    );

VOID EtDereferenceProcessEtwBlock(
    __inout PET_PROCESS_ETW_BLOCK Block
    );

PET_PROCESS_ETW_BLOCK EtFindProcessEtwBlock(
    __in PPH_PROCESS_ITEM ProcessItem
    );

// etwsys

VOID EtEtwShowSystemDialog();

// memlists

VOID EtShowMemoryListsDialog();

// modsrv

VOID EtShowModuleServicesDialog(
    __in HWND ParentWindowHandle,
    __in HANDLE ProcessId,
    __in PWSTR ModuleName
    );

// objprp

VOID EtHandlePropertiesInitializing(
    __in PVOID Parameter
    );

// options

VOID EtShowOptionsDialog(
    __in HWND ParentWindowHandle
    );

// thrdact

BOOLEAN EtUiCancelIoThread(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM Thread
    );

// unldll

VOID EtShowUnloadedDllsDialog(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_ITEM ProcessItem
    );

// wswatch

VOID EtShowWsWatchDialog(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_ITEM ProcessItem
    );

#endif
