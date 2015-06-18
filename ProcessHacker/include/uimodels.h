#ifndef PH_UIMODELS_H
#define PH_UIMODELS_H

// begin_phapppub
// Common state highlighting support

typedef struct _PH_SH_STATE
{
    PH_ITEM_STATE State;
    HANDLE StateListHandle;
    ULONG TickCount;
} PH_SH_STATE, *PPH_SH_STATE;
// end_phapppub

FORCEINLINE VOID PhChangeShStateTn(
    _Inout_ PPH_TREENEW_NODE Node,
    _Inout_ PPH_SH_STATE ShState,
    _Inout_ PPH_POINTER_LIST *StateList,
    _In_ PH_ITEM_STATE NewState,
    _In_ COLORREF NewTempBackColor,
    _In_opt_ HWND TreeNewHandleForUpdate
    )
{
    if (!*StateList)
        *StateList = PhCreatePointerList(4);

    if (ShState->State == NormalItemState)
        ShState->StateListHandle = PhAddItemPointerList(*StateList, Node);

    ShState->TickCount = GetTickCount();
    ShState->State = NewState;

    Node->UseTempBackColor = TRUE;
    Node->TempBackColor = NewTempBackColor;

    if (TreeNewHandleForUpdate)
        TreeNew_InvalidateNode(TreeNewHandleForUpdate, Node);
}

#define PH_TICK_SH_STATE_TN(NodeType, ShStateFieldName, StateList, RemoveFunction, HighlightingDuration, TreeNewHandleForUpdate, Invalidate, FullyInvalidated, ...) \
    do { \
        NodeType *node; \
        ULONG enumerationKey = 0; \
        ULONG tickCount; \
        BOOLEAN preferFullInvalidate; \
        HANDLE stateListHandle; \
        BOOLEAN redrawDisabled = FALSE; \
        BOOLEAN needsFullInvalidate = FALSE; \
\
        if (!StateList || StateList->Count == 0) \
            break; \
\
        tickCount = GetTickCount(); \
        preferFullInvalidate = StateList->Count > 8; \
\
        while (PhEnumPointerList(StateList, &enumerationKey, &node)) \
        { \
            if (PhRoundNumber(tickCount - node->ShStateFieldName.TickCount, 100) < (HighlightingDuration)) \
                continue; \
\
            stateListHandle = node->ShStateFieldName.StateListHandle; \
\
            if (node->ShStateFieldName.State == NewItemState) \
            { \
                node->ShStateFieldName.State = NormalItemState; \
                ((PPH_TREENEW_NODE)node)->UseTempBackColor = FALSE; \
                if (Invalidate) \
                { \
                    if (preferFullInvalidate) \
                    { \
                        needsFullInvalidate = TRUE; \
                    } \
                    else \
                    { \
                        TreeNew_InvalidateNode(TreeNewHandleForUpdate, node); \
                    } \
                } \
            } \
            else if (node->ShStateFieldName.State == RemovingItemState) \
            { \
                if (TreeNewHandleForUpdate) \
                { \
                    if (!redrawDisabled) \
                    { \
                        TreeNew_SetRedraw((TreeNewHandleForUpdate), FALSE); \
                        redrawDisabled = TRUE; \
                    } \
                } \
\
                RemoveFunction(node, __VA_ARGS__); \
                needsFullInvalidate = TRUE; \
            } \
\
            PhRemoveItemPointerList(StateList, stateListHandle); \
        } \
\
        if (TreeNewHandleForUpdate) \
        { \
            if (redrawDisabled) \
                TreeNew_SetRedraw((TreeNewHandleForUpdate), TRUE); \
            if (needsFullInvalidate) \
            { \
                InvalidateRect((TreeNewHandleForUpdate), NULL, FALSE); \
                if (FullyInvalidated) \
                    *((PBOOLEAN)FullyInvalidated) = TRUE; \
            } \
        } \
    } while (0)

// proctree

// Columns

// Default columns should go first
#define PHPRTLC_NAME 0
#define PHPRTLC_PID 1
#define PHPRTLC_CPU 2
#define PHPRTLC_IOTOTALRATE 3
#define PHPRTLC_PRIVATEBYTES 4
#define PHPRTLC_USERNAME 5
#define PHPRTLC_DESCRIPTION 6

#define PHPRTLC_COMPANYNAME 7
#define PHPRTLC_VERSION 8
#define PHPRTLC_FILENAME 9
#define PHPRTLC_COMMANDLINE 10
#define PHPRTLC_PEAKPRIVATEBYTES 11
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
#define PHPRTLC_IORORATE 27
#define PHPRTLC_IOWRATE 28
#define PHPRTLC_INTEGRITY 29
#define PHPRTLC_IOPRIORITY 30
#define PHPRTLC_PAGEPRIORITY 31
#define PHPRTLC_STARTTIME 32
#define PHPRTLC_TOTALCPUTIME 33
#define PHPRTLC_KERNELCPUTIME 34
#define PHPRTLC_USERCPUTIME 35
#define PHPRTLC_VERIFICATIONSTATUS 36
#define PHPRTLC_VERIFIEDSIGNER 37
#define PHPRTLC_ASLR 38
#define PHPRTLC_RELATIVESTARTTIME 39
#define PHPRTLC_BITS 40
#define PHPRTLC_ELEVATION 41
#define PHPRTLC_WINDOWTITLE 42
#define PHPRTLC_WINDOWSTATUS 43
#define PHPRTLC_CYCLES 44
#define PHPRTLC_CYCLESDELTA 45
#define PHPRTLC_CPUHISTORY 46
#define PHPRTLC_PRIVATEBYTESHISTORY 47
#define PHPRTLC_IOHISTORY 48
#define PHPRTLC_DEPSTATUS 49
#define PHPRTLC_VIRTUALIZED 50
#define PHPRTLC_CONTEXTSWITCHES 51
#define PHPRTLC_CONTEXTSWITCHESDELTA 52
#define PHPRTLC_PAGEFAULTSDELTA 53

#define PHPRTLC_IOREADS 54
#define PHPRTLC_IOWRITES 55
#define PHPRTLC_IOOTHER 56
#define PHPRTLC_IOREADBYTES 57
#define PHPRTLC_IOWRITEBYTES 58
#define PHPRTLC_IOOTHERBYTES 59
#define PHPRTLC_IOREADSDELTA 60
#define PHPRTLC_IOWRITESDELTA 61
#define PHPRTLC_IOOTHERDELTA 62

#define PHPRTLC_OSCONTEXT 63
#define PHPRTLC_PAGEDPOOL 64
#define PHPRTLC_PEAKPAGEDPOOL 65
#define PHPRTLC_NONPAGEDPOOL 66
#define PHPRTLC_PEAKNONPAGEDPOOL 67
#define PHPRTLC_MINIMUMWORKINGSET 68
#define PHPRTLC_MAXIMUMWORKINGSET 69
#define PHPRTLC_PRIVATEBYTESDELTA 70
#define PHPRTLC_SUBSYSTEM 71
#define PHPRTLC_PACKAGENAME 72
#define PHPRTLC_APPID 73
#define PHPRTLC_DPIAWARENESS 74
#define PHPRTLC_CFGUARD 75

#define PHPRTLC_MAXIMUM 76
#define PHPRTLC_IOGROUP_COUNT 9

#define PHPN_WSCOUNTERS 0x1
#define PHPN_GDIUSERHANDLES 0x2
#define PHPN_IOPAGEPRIORITY 0x4
#define PHPN_WINDOW 0x8
#define PHPN_DEPSTATUS 0x10
#define PHPN_TOKEN 0x20
#define PHPN_OSCONTEXT 0x40
#define PHPN_QUOTALIMITS 0x80
#define PHPN_IMAGE 0x100
#define PHPN_APPID 0x200
#define PHPN_DPIAWARENESS 0x400

// begin_phapppub
typedef struct _PH_PROCESS_NODE
{
    PH_TREENEW_NODE Node;

    PH_HASH_ENTRY HashEntry;

    PH_SH_STATE ShState;

    HANDLE ProcessId;
    PPH_PROCESS_ITEM ProcessItem;

    struct _PH_PROCESS_NODE *Parent;
    PPH_LIST Children;
// end_phapppub

    PH_STRINGREF TextCache[PHPRTLC_MAXIMUM];

    PH_STRINGREF DescriptionText;

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
    // DEP status
    ULONG DepStatus;
    // Token
    BOOLEAN VirtualizationAllowed;
    BOOLEAN VirtualizationEnabled;
    // OS Context
    GUID OsContextGuid;
    ULONG OsContextVersion;
    // Quota Limits
    SIZE_T MinimumWorkingSetSize;
    SIZE_T MaximumWorkingSetSize;
    // Image
    USHORT ImageCharacteristics;
    USHORT ImageReserved;
    USHORT ImageSubsystem;
    USHORT ImageDllCharacteristics;
    // App ID
    PPH_STRING AppIdText;
    // Cycles (Vista only)
    PH_UINT64_DELTA CyclesDelta;
    // DPI Awareness
    ULONG DpiAwareness;

    PPH_STRING TooltipText;
    ULONG TooltipTextValidToTickCount;

    // Text buffers
    WCHAR CpuUsageText[PH_INT32_STR_LEN_1];
    PPH_STRING IoTotalRateText;
    PPH_STRING PrivateBytesText;
    PPH_STRING PeakPrivateBytesText;
    PPH_STRING WorkingSetText;
    PPH_STRING PeakWorkingSetText;
    PPH_STRING PrivateWsText;
    PPH_STRING SharedWsText;
    PPH_STRING ShareableWsText;
    PPH_STRING VirtualSizeText;
    PPH_STRING PeakVirtualSizeText;
    PPH_STRING PageFaultsText;
    WCHAR BasePriorityText[PH_INT32_STR_LEN_1];
    WCHAR ThreadsText[PH_INT32_STR_LEN_1 + 3];
    WCHAR HandlesText[PH_INT32_STR_LEN_1 + 3];
    WCHAR GdiHandlesText[PH_INT32_STR_LEN_1 + 3];
    WCHAR UserHandlesText[PH_INT32_STR_LEN_1 + 3];
    PPH_STRING IoRoRateText;
    PPH_STRING IoWRateText;
    WCHAR PagePriorityText[PH_INT32_STR_LEN_1];
    PPH_STRING StartTimeText;
    WCHAR TotalCpuTimeText[PH_TIMESPAN_STR_LEN_1];
    WCHAR KernelCpuTimeText[PH_TIMESPAN_STR_LEN_1];
    WCHAR UserCpuTimeText[PH_TIMESPAN_STR_LEN_1];
    PPH_STRING RelativeStartTimeText;
    PPH_STRING WindowTitleText;
    PPH_STRING CyclesText;
    PPH_STRING CyclesDeltaText;
    PPH_STRING ContextSwitchesText;
    PPH_STRING ContextSwitchesDeltaText;
    PPH_STRING PageFaultsDeltaText;
    PPH_STRING IoGroupText[PHPRTLC_IOGROUP_COUNT];
    PPH_STRING PagedPoolText;
    PPH_STRING PeakPagedPoolText;
    PPH_STRING NonPagedPoolText;
    PPH_STRING PeakNonPagedPoolText;
    PPH_STRING MinimumWorkingSetText;
    PPH_STRING MaximumWorkingSetText;
    PPH_STRING PrivateBytesDeltaText;

    // Graph buffers
    PH_GRAPH_BUFFERS CpuGraphBuffers;
    PH_GRAPH_BUFFERS PrivateGraphBuffers;
    PH_GRAPH_BUFFERS IoGraphBuffers;
// begin_phapppub
} PH_PROCESS_NODE, *PPH_PROCESS_NODE;
// end_phapppub

VOID PhProcessTreeListInitialization(
    VOID
    );

VOID PhInitializeProcessTreeList(
    _In_ HWND hwnd
    );

VOID PhLoadSettingsProcessTreeList(
    VOID
    );

VOID PhSaveSettingsProcessTreeList(
    VOID
    );

VOID PhReloadSettingsProcessTreeList(
    VOID
    );

// begin_phapppub
PHAPPAPI
struct _PH_TN_FILTER_SUPPORT *
NTAPI
PhGetFilterSupportProcessTreeList(
    VOID
    );
// end_phapppub

PPH_PROCESS_NODE PhAddProcessNode(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ ULONG RunId
    );

// begin_phapppub
PHAPPAPI
PPH_PROCESS_NODE
NTAPI
PhFindProcessNode(
    _In_ HANDLE ProcessId
    );
// end_phapppub

VOID PhRemoveProcessNode(
    _In_ PPH_PROCESS_NODE ProcessNode
    );

// begin_phapppub
PHAPPAPI
VOID
NTAPI
PhUpdateProcessNode(
    _In_ PPH_PROCESS_NODE ProcessNode
    );
// end_phapppub

VOID PhTickProcessNodes(
    VOID
    );

// begin_phapppub
PHAPPAPI
PPH_PROCESS_ITEM
NTAPI
PhGetSelectedProcessItem(
    VOID
    );

PHAPPAPI
VOID
NTAPI
PhGetSelectedProcessItems(
    _Out_ PPH_PROCESS_ITEM **Processes,
    _Out_ PULONG NumberOfProcesses
    );

PHAPPAPI
VOID
NTAPI
PhDeselectAllProcessNodes(
    VOID
    );

PHAPPAPI
VOID
NTAPI
PhExpandAllProcessNodes(
    _In_ BOOLEAN Expand
    );

PHAPPAPI
VOID
NTAPI
PhInvalidateAllProcessNodes(
    VOID
    );

PHAPPAPI
VOID
NTAPI
PhSelectAndEnsureVisibleProcessNode(
    _In_ PPH_PROCESS_NODE ProcessNode
    );
// end_phapppub

VOID PhSelectAndEnsureVisibleProcessNodes(
    _In_ PPH_PROCESS_NODE *ProcessNodes,
    _In_ ULONG NumberOfProcessNodes
    );

PPH_LIST PhGetProcessTreeListLines(
    _In_ HWND TreeListHandle,
    _In_ ULONG NumberOfNodes,
    _In_ PPH_LIST RootNodes,
    _In_ ULONG Mode
    );

VOID PhCopyProcessTree(
    VOID
    );

VOID PhWriteProcessTree(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ ULONG Mode
    );

PPH_LIST PhDuplicateProcessNodeList(
    VOID
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
#define PHSVTLC_GROUP 8
#define PHSVTLC_DESCRIPTION 9

#define PHSVTLC_MAXIMUM 10

#define PHSN_CONFIG 0x1
#define PHSN_DESCRIPTION 0x2

// begin_phapppub
typedef struct _PH_SERVICE_NODE
{
    PH_TREENEW_NODE Node;

    PH_SH_STATE ShState;

    PPH_SERVICE_ITEM ServiceItem;
// end_phapppub

    PH_STRINGREF TextCache[PHSVTLC_MAXIMUM];

    ULONG ValidMask;

    WCHAR StartTypeText[12 + 24 + 1];
    // Config
    PPH_STRING BinaryPath;
    PPH_STRING LoadOrderGroup;
    // Description
    PPH_STRING Description;

    PPH_STRING TooltipText;
// begin_phapppub
} PH_SERVICE_NODE, *PPH_SERVICE_NODE;
// end_phapppub

VOID PhServiceTreeListInitialization(
    VOID
    );

VOID PhInitializeServiceTreeList(
    _In_ HWND hwnd
    );

VOID PhLoadSettingsServiceTreeList(
    VOID
    );

VOID PhSaveSettingsServiceTreeList(
    VOID
    );

// begin_phapppub
PHAPPAPI
struct _PH_TN_FILTER_SUPPORT *
NTAPI
PhGetFilterSupportServiceTreeList(
    VOID
    );
// end_phapppub

PPH_SERVICE_NODE PhAddServiceNode(
    _In_ PPH_SERVICE_ITEM ServiceItem,
    _In_ ULONG RunId
    );

// begin_phapppub
PHAPPAPI
PPH_SERVICE_NODE
NTAPI
PhFindServiceNode(
    _In_ PPH_SERVICE_ITEM ServiceItem
    );
// end_phapppub

VOID PhRemoveServiceNode(
    _In_ PPH_SERVICE_NODE ServiceNode
    );

// begin_phapppub
PHAPPAPI
VOID
NTAPI
PhUpdateServiceNode(
    _In_ PPH_SERVICE_NODE ServiceNode
    );
// end_phapppub

VOID PhTickServiceNodes(
    VOID
    );

// begin_phapppub
PHAPPAPI
PPH_SERVICE_ITEM
NTAPI
PhGetSelectedServiceItem(
    VOID
    );

PHAPPAPI
VOID
NTAPI
PhGetSelectedServiceItems(
    _Out_ PPH_SERVICE_ITEM **Services,
    _Out_ PULONG NumberOfServices
    );

PHAPPAPI
VOID
NTAPI
PhDeselectAllServiceNodes(
    VOID
    );

PHAPPAPI
VOID
NTAPI
PhSelectAndEnsureVisibleServiceNode(
    _In_ PPH_SERVICE_NODE ServiceNode
    );
// end_phapppub

VOID PhCopyServiceList(
    VOID
    );

VOID PhWriteServiceList(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ ULONG Mode
    );

// netlist

// Columns

#define PHNETLC_PROCESS 0
#define PHNETLC_LOCALADDRESS 1
#define PHNETLC_LOCALPORT 2
#define PHNETLC_REMOTEADDRESS 3
#define PHNETLC_REMOTEPORT 4
#define PHNETLC_PROTOCOL 5
#define PHNETLC_STATE 6
#define PHNETLC_OWNER 7
#define PHNETLC_TIMESTAMP 8
#define PHNETLC_MAXIMUM 9

// begin_phapppub
typedef struct _PH_NETWORK_NODE
{
    PH_TREENEW_NODE Node;

    PH_SH_STATE ShState;

    PPH_NETWORK_ITEM NetworkItem;
// end_phapppub

    PH_STRINGREF TextCache[PHNETLC_MAXIMUM];

    LONG UniqueId;
    PPH_STRING ProcessNameText;
    PH_STRINGREF LocalAddressText;
    PH_STRINGREF RemoteAddressText;
    PPH_STRING TimeStampText;

    PPH_STRING TooltipText;
// begin_phapppub
} PH_NETWORK_NODE, *PPH_NETWORK_NODE;
// end_phapppub

VOID PhNetworkTreeListInitialization(
    VOID
    );

VOID PhInitializeNetworkTreeList(
    _In_ HWND hwnd
    );

VOID PhLoadSettingsNetworkTreeList(
    VOID
    );

VOID PhSaveSettingsNetworkTreeList(
    VOID
    );

// begin_phapppub
PHAPPAPI
struct _PH_TN_FILTER_SUPPORT *
NTAPI
PhGetFilterSupportNetworkTreeList(
    VOID
    );
// end_phapppub

PPH_NETWORK_NODE PhAddNetworkNode(
    _In_ PPH_NETWORK_ITEM NetworkItem,
    _In_ ULONG RunId
    );

// begin_phapppub
PHAPPAPI
PPH_NETWORK_NODE
NTAPI
PhFindNetworkNode(
    _In_ PPH_NETWORK_ITEM NetworkItem
    );
// end_phapppub

VOID PhRemoveNetworkNode(
    _In_ PPH_NETWORK_NODE NetworkNode
    );

VOID PhUpdateNetworkNode(
    _In_ PPH_NETWORK_NODE NetworkNode
    );

VOID PhTickNetworkNodes(
    VOID
    );

PPH_NETWORK_ITEM PhGetSelectedNetworkItem(
    VOID
    );

VOID PhGetSelectedNetworkItems(
    _Out_ PPH_NETWORK_ITEM **NetworkItems,
    _Out_ PULONG NumberOfNetworkItems
    );

VOID PhDeselectAllNetworkNodes(
    VOID
    );

VOID PhSelectAndEnsureVisibleNetworkNode(
    _In_ PPH_NETWORK_NODE NetworkNode
    );

VOID PhCopyNetworkList(
    VOID
    );

VOID PhWriteNetworkList(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ ULONG Mode
    );

// thrdlist

// Columns

#define PHTHTLC_TID 0
#define PHTHTLC_CPU 1
#define PHTHTLC_CYCLESDELTA 2
#define PHTHTLC_STARTADDRESS 3
#define PHTHTLC_PRIORITY 4
#define PHTHTLC_SERVICE 5

#define PHTHTLC_MAXIMUM 6

// begin_phapppub
typedef struct _PH_THREAD_NODE
{
    PH_TREENEW_NODE Node;

    PH_SH_STATE ShState;

    HANDLE ThreadId;
    PPH_THREAD_ITEM ThreadItem;
// end_phapppub

    PH_STRINGREF TextCache[PHTHTLC_MAXIMUM];

    ULONG ValidMask;

    WCHAR CpuUsageText[PH_INT32_STR_LEN_1];
    PPH_STRING CyclesDeltaText; // used for Context Switches Delta as well
    PPH_STRING StartAddressText;
    PPH_STRING PriorityText;
// begin_phapppub
} PH_THREAD_NODE, *PPH_THREAD_NODE;
// end_phapppub

typedef struct _PH_THREAD_LIST_CONTEXT
{
    HWND ParentWindowHandle;
    HWND TreeNewHandle;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PH_CM_MANAGER Cm;
    BOOLEAN UseCycleTime;
    BOOLEAN HasServices;

    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;

    BOOLEAN EnableStateHighlighting;
    PPH_POINTER_LIST NodeStateList;
} PH_THREAD_LIST_CONTEXT, *PPH_THREAD_LIST_CONTEXT;

VOID PhInitializeThreadList(
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle,
    _Out_ PPH_THREAD_LIST_CONTEXT Context
    );

VOID PhDeleteThreadList(
    _In_ PPH_THREAD_LIST_CONTEXT Context
    );

VOID PhLoadSettingsThreadList(
    _Inout_ PPH_THREAD_LIST_CONTEXT Context
    );

VOID PhSaveSettingsThreadList(
    _Inout_ PPH_THREAD_LIST_CONTEXT Context
    );

PPH_THREAD_NODE PhAddThreadNode(
    _Inout_ PPH_THREAD_LIST_CONTEXT Context,
    _In_ PPH_THREAD_ITEM ThreadItem,
    _In_ BOOLEAN FirstRun
    );

PPH_THREAD_NODE PhFindThreadNode(
    _In_ PPH_THREAD_LIST_CONTEXT Context,
    _In_ HANDLE ThreadId
    );

VOID PhRemoveThreadNode(
    _In_ PPH_THREAD_LIST_CONTEXT Context,
    _In_ PPH_THREAD_NODE ThreadNode
    );

VOID PhUpdateThreadNode(
    _In_ PPH_THREAD_LIST_CONTEXT Context,
    _In_ PPH_THREAD_NODE ThreadNode
    );

VOID PhTickThreadNodes(
    _In_ PPH_THREAD_LIST_CONTEXT Context
    );

PPH_THREAD_ITEM PhGetSelectedThreadItem(
    _In_ PPH_THREAD_LIST_CONTEXT Context
    );

VOID PhGetSelectedThreadItems(
    _In_ PPH_THREAD_LIST_CONTEXT Context,
    _Out_ PPH_THREAD_ITEM **Threads,
    _Out_ PULONG NumberOfThreads
    );

VOID PhDeselectAllThreadNodes(
    _In_ PPH_THREAD_LIST_CONTEXT Context
    );

// modlist

// Columns

#define PHMOTLC_NAME 0
#define PHMOTLC_BASEADDRESS 1
#define PHMOTLC_SIZE 2
#define PHMOTLC_DESCRIPTION 3

#define PHMOTLC_COMPANYNAME 4
#define PHMOTLC_VERSION 5
#define PHMOTLC_FILENAME 6

#define PHMOTLC_TYPE 7
#define PHMOTLC_LOADCOUNT 8
#define PHMOTLC_VERIFICATIONSTATUS 9
#define PHMOTLC_VERIFIEDSIGNER 10
#define PHMOTLC_ASLR 11
#define PHMOTLC_TIMESTAMP 12
#define PHMOTLC_CFGUARD 13
#define PHMOTLC_LOADTIME 14
#define PHMOTLC_LOADREASON 15

#define PHMOTLC_MAXIMUM 16

// begin_phapppub
typedef struct _PH_MODULE_NODE
{
    PH_TREENEW_NODE Node;

    PH_SH_STATE ShState;

    PPH_MODULE_ITEM ModuleItem;
// end_phapppub

    PH_STRINGREF TextCache[PHMOTLC_MAXIMUM];

    ULONG ValidMask;

    PPH_STRING TooltipText;

    PPH_STRING SizeText;
    WCHAR LoadCountText[PH_INT32_STR_LEN_1];
    PPH_STRING TimeStampText;
    PPH_STRING LoadTimeText;
// begin_phapppub
} PH_MODULE_NODE, *PPH_MODULE_NODE;
// end_phapppub

typedef struct _PH_MODULE_LIST_CONTEXT
{
    HWND ParentWindowHandle;
    HWND TreeNewHandle;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PH_CM_MANAGER Cm;

    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;

    BOOLEAN EnableStateHighlighting;
    PPH_POINTER_LIST NodeStateList;

    HFONT BoldFont;
} PH_MODULE_LIST_CONTEXT, *PPH_MODULE_LIST_CONTEXT;

VOID PhInitializeModuleList(
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle,
    _Out_ PPH_MODULE_LIST_CONTEXT Context
    );

VOID PhDeleteModuleList(
    _In_ PPH_MODULE_LIST_CONTEXT Context
    );

VOID PhLoadSettingsModuleList(
    _Inout_ PPH_MODULE_LIST_CONTEXT Context
    );

VOID PhSaveSettingsModuleList(
    _Inout_ PPH_MODULE_LIST_CONTEXT Context
    );

PPH_MODULE_NODE PhAddModuleNode(
    _Inout_ PPH_MODULE_LIST_CONTEXT Context,
    _In_ PPH_MODULE_ITEM ModuleItem,
    _In_ ULONG RunId
    );

PPH_MODULE_NODE PhFindModuleNode(
    _In_ PPH_MODULE_LIST_CONTEXT Context,
    _In_ PPH_MODULE_ITEM ModuleItem
    );

VOID PhRemoveModuleNode(
    _In_ PPH_MODULE_LIST_CONTEXT Context,
    _In_ PPH_MODULE_NODE ModuleNode
    );

VOID PhUpdateModuleNode(
    _In_ PPH_MODULE_LIST_CONTEXT Context,
    _In_ PPH_MODULE_NODE ModuleNode
    );

VOID PhTickModuleNodes(
    _In_ PPH_MODULE_LIST_CONTEXT Context
    );

PPH_MODULE_ITEM PhGetSelectedModuleItem(
    _In_ PPH_MODULE_LIST_CONTEXT Context
    );

VOID PhGetSelectedModuleItems(
    _In_ PPH_MODULE_LIST_CONTEXT Context,
    _Out_ PPH_MODULE_ITEM **Modules,
    _Out_ PULONG NumberOfModules
    );

VOID PhDeselectAllModuleNodes(
    _In_ PPH_MODULE_LIST_CONTEXT Context
    );

// hndllist

// Columns

#define PHHNTLC_TYPE 0
#define PHHNTLC_NAME 1
#define PHHNTLC_HANDLE 2

#define PHHNTLC_OBJECTADDRESS 3
#define PHHNTLC_ATTRIBUTES 4
#define PHHNTLC_GRANTEDACCESS 5
#define PHHNTLC_GRANTEDACCESSSYMBOLIC 6
#define PHHNTLC_ORIGINALNAME 7
#define PHHNTLC_FILESHAREACCESS 8

#define PHHNTLC_MAXIMUM 9

// begin_phapppub
typedef struct _PH_HANDLE_NODE
{
    PH_TREENEW_NODE Node;

    PH_SH_STATE ShState;

    HANDLE Handle;
    PPH_HANDLE_ITEM HandleItem;
// end_phapppub

    PH_STRINGREF TextCache[PHHNTLC_MAXIMUM];

    PPH_STRING GrantedAccessSymbolicText;
    WCHAR FileShareAccessText[4];
// begin_phapppub
} PH_HANDLE_NODE, *PPH_HANDLE_NODE;
// end_phapppub

typedef struct _PH_HANDLE_LIST_CONTEXT
{
    HWND ParentWindowHandle;
    HWND TreeNewHandle;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PH_CM_MANAGER Cm;
    BOOLEAN HideUnnamedHandles;

    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;

    BOOLEAN EnableStateHighlighting;
    PPH_POINTER_LIST NodeStateList;
} PH_HANDLE_LIST_CONTEXT, *PPH_HANDLE_LIST_CONTEXT;

VOID PhInitializeHandleList(
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle,
    _Out_ PPH_HANDLE_LIST_CONTEXT Context
    );

VOID PhDeleteHandleList(
    _In_ PPH_HANDLE_LIST_CONTEXT Context
    );

VOID PhLoadSettingsHandleList(
    _Inout_ PPH_HANDLE_LIST_CONTEXT Context
    );

VOID PhSaveSettingsHandleList(
    _Inout_ PPH_HANDLE_LIST_CONTEXT Context
    );

VOID PhSetOptionsHandleList(
    _Inout_ PPH_HANDLE_LIST_CONTEXT Context,
    _In_ BOOLEAN HideUnnamedHandles
    );

PPH_HANDLE_NODE PhAddHandleNode(
    _Inout_ PPH_HANDLE_LIST_CONTEXT Context,
    _In_ PPH_HANDLE_ITEM HandleItem,
    _In_ ULONG RunId
    );

PPH_HANDLE_NODE PhFindHandleNode(
    _In_ PPH_HANDLE_LIST_CONTEXT Context,
    _In_ HANDLE Handle
    );

VOID PhRemoveHandleNode(
    _In_ PPH_HANDLE_LIST_CONTEXT Context,
    _In_ PPH_HANDLE_NODE HandleNode
    );

VOID PhUpdateHandleNode(
    _In_ PPH_HANDLE_LIST_CONTEXT Context,
    _In_ PPH_HANDLE_NODE HandleNode
    );

VOID PhTickHandleNodes(
    _In_ PPH_HANDLE_LIST_CONTEXT Context
    );

PPH_HANDLE_ITEM PhGetSelectedHandleItem(
    _In_ PPH_HANDLE_LIST_CONTEXT Context
    );

VOID PhGetSelectedHandleItems(
    _In_ PPH_HANDLE_LIST_CONTEXT Context,
    _Out_ PPH_HANDLE_ITEM **Handles,
    _Out_ PULONG NumberOfHandles
    );

VOID PhDeselectAllHandleNodes(
    _In_ PPH_HANDLE_LIST_CONTEXT Context
    );

// memlist

// Columns

#define PHMMTLC_BASEADDRESS 0
#define PHMMTLC_TYPE 1
#define PHMMTLC_SIZE 2
#define PHMMTLC_PROTECTION 3
#define PHMMTLC_USE 4
#define PHMMTLC_TOTALWS 5
#define PHMMTLC_PRIVATEWS 6
#define PHMMTLC_SHAREABLEWS 7
#define PHMMTLC_SHAREDWS 8
#define PHMMTLC_LOCKEDWS 9
#define PHMMTLC_COMMITTED 10
#define PHMMTLC_PRIVATE 11

#define PHMMTLC_MAXIMUM 12

// begin_phapppub
typedef struct _PH_MEMORY_NODE
{
    PH_TREENEW_NODE Node;

    BOOLEAN IsAllocationBase;
    BOOLEAN Reserved1;
    USHORT Reserved2;
    PPH_MEMORY_ITEM MemoryItem;

    struct _PH_MEMORY_NODE *Parent;
    PPH_LIST Children;
// end_phapppub

    PH_STRINGREF TextCache[PHMMTLC_MAXIMUM];

    WCHAR BaseAddressText[PH_PTR_STR_LEN_1];
    WCHAR TypeText[30];
    PPH_STRING SizeText;
    WCHAR ProtectionText[17];
    PPH_STRING UseText;
    PPH_STRING TotalWsText;
    PPH_STRING PrivateWsText;
    PPH_STRING ShareableWsText;
    PPH_STRING SharedWsText;
    PPH_STRING LockedWsText;
    PPH_STRING CommittedText;
    PPH_STRING PrivateText;
// begin_phapppub
} PH_MEMORY_NODE, *PPH_MEMORY_NODE;
// end_phapppub

typedef struct _PH_MEMORY_LIST_CONTEXT
{
    HWND ParentWindowHandle;
    HWND TreeNewHandle;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PH_CM_MANAGER Cm;
    BOOLEAN HideFreeRegions;

    PPH_LIST AllocationBaseNodeList; // Allocation base nodes (list should always be sorted by base address)
    PPH_LIST RegionNodeList; // Memory region nodes
} PH_MEMORY_LIST_CONTEXT, *PPH_MEMORY_LIST_CONTEXT;

VOID PhInitializeMemoryList(
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle,
    _Out_ PPH_MEMORY_LIST_CONTEXT Context
    );

VOID PhDeleteMemoryList(
    _In_ PPH_MEMORY_LIST_CONTEXT Context
    );

VOID PhLoadSettingsMemoryList(
    _Inout_ PPH_MEMORY_LIST_CONTEXT Context
    );

VOID PhSaveSettingsMemoryList(
    _Inout_ PPH_MEMORY_LIST_CONTEXT Context
    );

VOID PhSetOptionsMemoryList(
    _Inout_ PPH_MEMORY_LIST_CONTEXT Context,
    _In_ BOOLEAN HideFreeRegions
    );

VOID PhReplaceMemoryList(
    _Inout_ PPH_MEMORY_LIST_CONTEXT Context,
    _In_ PPH_MEMORY_ITEM_LIST List
    );

VOID PhUpdateMemoryNode(
    _In_ PPH_MEMORY_LIST_CONTEXT Context,
    _In_ PPH_MEMORY_NODE MemoryNode
    );

PPH_MEMORY_NODE PhGetSelectedMemoryNode(
    _In_ PPH_MEMORY_LIST_CONTEXT Context
    );

VOID PhGetSelectedMemoryNodes(
    _In_ PPH_MEMORY_LIST_CONTEXT Context,
    _Out_ PPH_MEMORY_NODE **MemoryNodes,
    _Out_ PULONG NumberOfMemoryNodes
    );

VOID PhDeselectAllMemoryNodes(
    _In_ PPH_MEMORY_LIST_CONTEXT Context
    );

#endif
