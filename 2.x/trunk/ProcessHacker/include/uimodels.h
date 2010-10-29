#ifndef UIMODELS_H
#define UIMODELS_H

// proctree

// Columns

// Default columns should go first
#define PHPRTLC_NAME 0
#define PHPRTLC_PID 1
#define PHPRTLC_CPU 2
#define PHPRTLC_IOTOTAL 3
#define PHPRTLC_PVTMEMORY 4
#define PHPRTLC_USERNAME 5
#define PHPRTLC_DESCRIPTION 6

#define PHPRTLC_COMPANYNAME 7
#define PHPRTLC_VERSION 8
#define PHPRTLC_FILENAME 9
#define PHPRTLC_COMMANDLINE 10

#define PHPRTLC_PEAKPVTMEMORY 11
#define PHPRTLC_WORKINGSET 12
#define PHPRTLC_PEAKWORKINGSET 13
#define PHPRTLC_PRIVATEWS 14
#define PHPRTLC_SHAREDWS 15
#define PHPRTLC_SHAREABLEWS 16
#define PHPRTLC_VIRTUALSIZE 17
#define PHPRTLC_PEAKVIRTUALSIZE 18
#define PHPRTLC_PAGEFAULTS 19
#define PHPRTLC_SESSIONID 20
#define PHPRTLC_PRIORITYCLASS 21
#define PHPRTLC_BASEPRIORITY 22

#define PHPRTLC_THREADS 23
#define PHPRTLC_HANDLES 24
#define PHPRTLC_GDIHANDLES 25
#define PHPRTLC_USERHANDLES 26
#define PHPRTLC_IORO 27
#define PHPRTLC_IOW 28
#define PHPRTLC_INTEGRITY 29
#define PHPRTLC_IOPRIORITY 30
#define PHPRTLC_PAGEPRIORITY 31
#define PHPRTLC_STARTTIME 32
#define PHPRTLC_TOTALCPUTIME 33
#define PHPRTLC_KERNELCPUTIME 34
#define PHPRTLC_USERCPUTIME 35
#define PHPRTLC_VERIFICATIONSTATUS 36
#define PHPRTLC_VERIFIEDSIGNER 37
#define PHPRTLC_RESERVED1 38
#define PHPRTLC_RELATIVESTARTTIME 39
#define PHPRTLC_BITS 40
#define PHPRTLC_ELEVATION 41
#define PHPRTLC_WINDOWTITLE 42
#define PHPRTLC_WINDOWSTATUS 43
#define PHPRTLC_CYCLES 44
#define PHPRTLC_CYCLESDELTA 45

#define PHPRTLC_MAXIMUM 46

#define PHPN_WSCOUNTERS 0x1
#define PHPN_GDIUSERHANDLES 0x2
#define PHPN_IOPAGEPRIORITY 0x4
#define PHPN_WINDOW 0x8

typedef struct _PH_PROCESS_NODE
{
    PH_TREELIST_NODE Node;

    PH_HASH_ENTRY HashEntry;

    PH_ITEM_STATE State;
    HANDLE StateListHandle;
    ULONG TickCount;

    HANDLE ProcessId;
    PPH_PROCESS_ITEM ProcessItem;

    struct _PH_PROCESS_NODE *Parent;
    PPH_LIST Children;

    PH_STRINGREF TextCache[PHPRTLC_MAXIMUM];

    // If the user has selected certain columns we need extra information 
    // that isn't retrieved by the process provider.
    ULONG ValidMask;

    // WS counters
    PH_PROCESS_WS_COUNTERS WsCounters;
    // GDI, USER handles
    ULONG GdiHandles;
    ULONG UserHandles;
    // I/O, Page priority
    ULONG IoPriority;
    ULONG PagePriority;
    // Window
    HWND WindowHandle;
    PPH_STRING WindowText;
    BOOLEAN WindowHung;
    // Cycles
    PH_UINT64_DELTA CyclesDelta;

    PPH_STRING TooltipText;

    WCHAR CpuUsageText[PH_INT32_STR_LEN_1];
    PPH_STRING IoTotalText;
    PPH_STRING PrivateMemoryText;
    PPH_STRING PeakPrivateMemoryText;
    PPH_STRING WorkingSetText;
    PPH_STRING PeakWorkingSetText;
    PPH_STRING PrivateWsText;
    PPH_STRING SharedWsText;
    PPH_STRING ShareableWsText;
    PPH_STRING VirtualSizeText;
    PPH_STRING PeakVirtualSizeText;
    PPH_STRING PageFaultsText;
    WCHAR BasePriorityText[PH_INT32_STR_LEN_1];
    WCHAR ThreadsText[PH_INT32_STR_LEN_1];
    WCHAR HandlesText[PH_INT32_STR_LEN_1];
    WCHAR GdiHandlesText[PH_INT32_STR_LEN_1];
    WCHAR UserHandlesText[PH_INT32_STR_LEN_1];
    PPH_STRING IoRoText;
    PPH_STRING IoWText;
    WCHAR IoPriorityText[PH_INT32_STR_LEN_1];
    WCHAR PagePriorityText[PH_INT32_STR_LEN_1];
    PPH_STRING StartTimeText;
    WCHAR TotalCpuTimeText[PH_TIMESPAN_STR_LEN_1];
    WCHAR KernelCpuTimeText[PH_TIMESPAN_STR_LEN_1];
    WCHAR UserCpuTimeText[PH_TIMESPAN_STR_LEN_1];
    PPH_STRING RelativeStartTimeText;
    PPH_STRING WindowTitleText;
    PPH_STRING CyclesText;
    PPH_STRING CyclesDeltaText;
} PH_PROCESS_NODE, *PPH_PROCESS_NODE;

VOID PhProcessTreeListInitialization();

VOID PhInitializeProcessTreeList(
    __in HWND hwnd
    );

VOID PhLoadSettingsProcessTreeList();

VOID PhSaveSettingsProcessTreeList();

PPH_PROCESS_NODE PhAddProcessNode(
    __in PPH_PROCESS_ITEM ProcessItem,
    __in ULONG RunId
    );

PHAPPAPI
PPH_PROCESS_NODE PhFindProcessNode(
    __in HANDLE ProcessId
    );

VOID PhRemoveProcessNode(
    __in PPH_PROCESS_NODE ProcessNode
    );

PHAPPAPI
VOID PhUpdateProcessNode(
    __in PPH_PROCESS_NODE ProcessNode
    );

VOID PhTickProcessNodes();

PHAPPAPI
PPH_PROCESS_ITEM PhGetSelectedProcessItem();

PHAPPAPI
VOID PhGetSelectedProcessItems(
    __out PPH_PROCESS_ITEM **Processes,
    __out PULONG NumberOfProcesses
    );

PHAPPAPI
VOID PhDeselectAllProcessNodes();

PHAPPAPI
VOID PhInvalidateAllProcessNodes();

PHAPPAPI
VOID PhSelectAndEnsureVisibleProcessNode(
    __in PPH_PROCESS_NODE ProcessNode
    );

typedef BOOLEAN (NTAPI *PPH_PROCESS_TREE_FILTER)(
    __in PPH_PROCESS_NODE ProcessNode,
    __in_opt PVOID Context
    );

typedef struct _PH_PROCESS_TREE_FILTER_ENTRY
{
    PPH_PROCESS_TREE_FILTER Filter;
    PVOID Context;
} PH_PROCESS_TREE_FILTER_ENTRY, *PPH_PROCESS_TREE_FILTER_ENTRY;

PHAPPAPI
PPH_PROCESS_TREE_FILTER_ENTRY PhAddProcessTreeFilter(
    __in PPH_PROCESS_TREE_FILTER Filter,
    __in_opt PVOID Context
    );

PHAPPAPI
VOID PhRemoveProcessTreeFilter(
    __in PPH_PROCESS_TREE_FILTER_ENTRY Entry
    );

PHAPPAPI
VOID PhApplyProcessTreeFilters();

VOID PhCopyProcessTree();

VOID PhWriteProcessTree(
    __inout PPH_FILE_STREAM FileStream,
    __in ULONG Mode
    );

// srvlist

// Columns

#define PHSVTLC_NAME 0
#define PHSVTLC_DISPLAYNAME 1
#define PHSVTLC_TYPE 2
#define PHSVTLC_STATUS 3
#define PHSVTLC_STARTTYPE 4
#define PHSVTLC_PID 5

#define PHSVTLC_BINARYPATH 6
#define PHSVTLC_ERRORCONTROL 7

#define PHSVTLC_MAXIMUM 6

#define PHSN_CONFIG 0x1

typedef struct _PH_SERVICE_NODE
{
    PH_TREELIST_NODE Node;

    PH_ITEM_STATE State;
    HANDLE StateListHandle;
    ULONG TickCount;

    PPH_SERVICE_ITEM ServiceItem;

    PH_STRINGREF TextCache[PHSVTLC_MAXIMUM];

    ULONG ValidMask;

    // Binary Path
    PPH_STRING BinaryPath;

    PPH_STRING TooltipText;
} PH_SERVICE_NODE, *PPH_SERVICE_NODE;

VOID PhServiceTreeListInitialization();

VOID PhInitializeServiceTreeList(
    __in HWND hwnd
    );

VOID PhLoadSettingsServiceTreeList();

VOID PhSaveSettingsServiceTreeList();

PPH_SERVICE_NODE PhAddServiceNode(
    __in PPH_SERVICE_ITEM ServiceItem,
    __in ULONG RunId
    );

PHAPPAPI
PPH_SERVICE_NODE PhFindServiceNode(
    __in PPH_SERVICE_ITEM ServiceItem
    );

VOID PhRemoveServiceNode(
    __in PPH_SERVICE_NODE ServiceNode
    );

PHAPPAPI
VOID PhUpdateServiceNode(
    __in PPH_SERVICE_NODE ServiceNode
    );

VOID PhTickServiceNodes();

PHAPPAPI
PPH_SERVICE_ITEM PhGetSelectedServiceItem();

PHAPPAPI
VOID PhGetSelectedServiceItems(
    __out PPH_SERVICE_ITEM **Services,
    __out PULONG NumberOfServices
    );

PHAPPAPI
VOID PhDeselectAllServiceNodes();

PHAPPAPI
VOID PhSelectAndEnsureVisibleServiceNode(
    __in PPH_SERVICE_NODE ServiceNode
    );

VOID PhCopyServiceList();

VOID PhWriteServiceList(
    __inout PPH_FILE_STREAM FileStream,
    __in ULONG Mode
    );

#endif
