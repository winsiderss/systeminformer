#ifndef EXTTOOLS_H
#define EXTTOOLS_H

#define PHNT_VERSION PHNT_VISTA
#include <phdk.h>

extern PPH_PLUGIN PluginInstance;
extern LIST_ENTRY EtProcessBlockListHead;
extern LIST_ENTRY EtNetworkBlockListHead;
extern HWND ProcessTreeNewHandle;
extern HWND NetworkTreeNewHandle;

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

// treeext

#define ETPRTNC_DISKREADS 1
#define ETPRTNC_DISKWRITES 2
#define ETPRTNC_DISKREADBYTES 3
#define ETPRTNC_DISKWRITEBYTES 4
#define ETPRTNC_DISKTOTALBYTES 5
#define ETPRTNC_DISKREADSDELTA 6
#define ETPRTNC_DISKWRITESDELTA 7
#define ETPRTNC_DISKREADBYTESDELTA 8
#define ETPRTNC_DISKWRITEBYTESDELTA 9
#define ETPRTNC_DISKTOTALBYTESDELTA 10
#define ETPRTNC_NETWORKRECEIVES 11
#define ETPRTNC_NETWORKSENDS 12
#define ETPRTNC_NETWORKRECEIVEBYTES 13
#define ETPRTNC_NETWORKSENDBYTES 14
#define ETPRTNC_NETWORKTOTALBYTES 15
#define ETPRTNC_NETWORKRECEIVESDELTA 16
#define ETPRTNC_NETWORKSENDSDELTA 17
#define ETPRTNC_NETWORKRECEIVEBYTESDELTA 18
#define ETPRTNC_NETWORKSENDBYTESDELTA 19
#define ETPRTNC_NETWORKTOTALBYTESDELTA 20
#define ETPRTNC_MAXIMUM 20

VOID EtEtwProcessTreeNewInitializing(
    __in PVOID Parameter
    );

VOID EtEtwProcessTreeNewMessage(
    __in PVOID Parameter
    );

#define ETNETNC_RECEIVES 1
#define ETNETNC_SENDS 2
#define ETNETNC_RECEIVEBYTES 3
#define ETNETNC_SENDBYTES 4
#define ETNETNC_TOTALBYTES 5
#define ETNETNC_RECEIVESDELTA 6
#define ETNETNC_SENDSDELTA 7
#define ETNETNC_RECEIVEBYTESDELTA 8
#define ETNETNC_SENDBYTESDELTA 9
#define ETNETNC_TOTALBYTESDELTA 10
#define ETNETNC_FIREWALLSTATUS 11
#define ETNETNC_MAXIMUM 11

VOID EtEtwNetworkTreeNewInitializing(
    __in PVOID Parameter
    );

VOID EtEtwNetworkTreeNewMessage(
    __in PVOID Parameter
    );

typedef enum _ET_FIREWALL_STATUS
{
    FirewallUnknownStatus,
    FirewallAllowedNotRestricted,
    FirewallAllowedRestricted,
    FirewallNotAllowedNotRestricted,
    FirewallNotAllowedRestricted,
    FirewallMaximumStatus
} ET_FIREWALL_STATUS;

ET_FIREWALL_STATUS EtQueryFirewallStatus(
    __in PPH_NETWORK_ITEM NetworkItem
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

VOID EtEtwStatisticsInitialization();

VOID EtEtwStatisticsUninitialization();

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

// main (extensions)

typedef struct _ET_PROCESS_BLOCK
{
    LIST_ENTRY ListEntry;
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
    PH_UINT32_DELTA DiskReadRawDelta;
    PH_UINT32_DELTA DiskWriteDelta;
    PH_UINT32_DELTA DiskWriteRawDelta;
    PH_UINT32_DELTA NetworkReceiveDelta;
    PH_UINT32_DELTA NetworkReceiveRawDelta;
    PH_UINT32_DELTA NetworkSendDelta;
    PH_UINT32_DELTA NetworkSendRawDelta;

    PH_QUEUED_LOCK TextCacheLock;
    PPH_STRING TextCache[ETPRTNC_MAXIMUM + 1];
    BOOLEAN TextCacheValid[ETPRTNC_MAXIMUM + 1];
} ET_PROCESS_BLOCK, *PET_PROCESS_BLOCK;

typedef struct _ET_NETWORK_BLOCK
{
    LIST_ENTRY ListEntry;
    PPH_NETWORK_ITEM NetworkItem;

    ULONG ReceiveCount;
    ULONG SendCount;
    ULONG ReceiveRaw;
    ULONG SendRaw;

    union
    {
        struct
        {
            PH_UINT32_DELTA ReceiveDelta;
            PH_UINT32_DELTA ReceiveRawDelta;
            PH_UINT32_DELTA SendDelta;
            PH_UINT32_DELTA SendRawDelta;
        };
        PH_UINT32_DELTA Deltas[4];
    };

    ET_FIREWALL_STATUS FirewallStatus;
    BOOLEAN FirewallStatusValid;

    PH_QUEUED_LOCK TextCacheLock;
    PPH_STRING TextCache[ETNETNC_MAXIMUM + 1];
    BOOLEAN TextCacheValid[ETNETNC_MAXIMUM + 1];
} ET_NETWORK_BLOCK, *PET_NETWORK_BLOCK;

PET_PROCESS_BLOCK EtGetProcessBlock(
    __in PPH_PROCESS_ITEM ProcessItem
    );

PET_NETWORK_BLOCK EtGetNetworkBlock(
    __in PPH_NETWORK_ITEM NetworkItem
    );

#endif
