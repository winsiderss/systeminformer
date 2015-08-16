#ifndef PH_PROVIDERS_H
#define PH_PROVIDERS_H

// procprv

#define PH_RECORD_MAX_USAGE
#define PH_ENABLE_VERIFY_CACHE

extern PPH_OBJECT_TYPE PhProcessItemType;

PHAPPAPI extern PH_CALLBACK PhProcessAddedEvent; // phapppub
PHAPPAPI extern PH_CALLBACK PhProcessModifiedEvent; // phapppub
PHAPPAPI extern PH_CALLBACK PhProcessRemovedEvent; // phapppub
PHAPPAPI extern PH_CALLBACK PhProcessesUpdatedEvent; // phapppub

extern PPH_LIST PhProcessRecordList;
extern PH_QUEUED_LOCK PhProcessRecordListLock;

extern ULONG PhStatisticsSampleCount;
extern BOOLEAN PhEnableProcessQueryStage2;
extern BOOLEAN PhEnablePurgeProcessRecords;
extern BOOLEAN PhEnableCycleCpuUsage;

extern PVOID PhProcessInformation; // only can be used if running on same thread as process provider
extern ULONG PhProcessInformationSequenceNumber;
extern SYSTEM_PERFORMANCE_INFORMATION PhPerfInformation;
extern PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PhCpuInformation;
extern SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PhCpuTotals;
extern ULONG PhTotalProcesses;
extern ULONG PhTotalThreads;
extern ULONG PhTotalHandles;

extern ULONG64 PhCpuTotalCycleDelta;
extern PLARGE_INTEGER PhCpuIdleCycleTime; // cycle time for Idle
extern PLARGE_INTEGER PhCpuSystemCycleTime; // cycle time for DPCs and Interrupts
extern PH_UINT64_DELTA PhCpuIdleCycleDelta;
extern PH_UINT64_DELTA PhCpuSystemCycleDelta;

extern FLOAT PhCpuKernelUsage;
extern FLOAT PhCpuUserUsage;
extern PFLOAT PhCpusKernelUsage;
extern PFLOAT PhCpusUserUsage;

extern PH_UINT64_DELTA PhCpuKernelDelta;
extern PH_UINT64_DELTA PhCpuUserDelta;
extern PH_UINT64_DELTA PhCpuIdleDelta;

extern PPH_UINT64_DELTA PhCpusKernelDelta;
extern PPH_UINT64_DELTA PhCpusUserDelta;
extern PPH_UINT64_DELTA PhCpusIdleDelta;

extern PH_UINT64_DELTA PhIoReadDelta;
extern PH_UINT64_DELTA PhIoWriteDelta;
extern PH_UINT64_DELTA PhIoOtherDelta;

extern PH_CIRCULAR_BUFFER_FLOAT PhCpuKernelHistory;
extern PH_CIRCULAR_BUFFER_FLOAT PhCpuUserHistory;
//extern PH_CIRCULAR_BUFFER_FLOAT PhCpuOtherHistory;

extern PPH_CIRCULAR_BUFFER_FLOAT PhCpusKernelHistory;
extern PPH_CIRCULAR_BUFFER_FLOAT PhCpusUserHistory;
//extern PPH_CIRCULAR_BUFFER_FLOAT PhCpusOtherHistory;

extern PH_CIRCULAR_BUFFER_ULONG64 PhIoReadHistory;
extern PH_CIRCULAR_BUFFER_ULONG64 PhIoWriteHistory;
extern PH_CIRCULAR_BUFFER_ULONG64 PhIoOtherHistory;

extern PH_CIRCULAR_BUFFER_ULONG PhCommitHistory;
extern PH_CIRCULAR_BUFFER_ULONG PhPhysicalHistory;

extern PH_CIRCULAR_BUFFER_ULONG PhMaxCpuHistory;
extern PH_CIRCULAR_BUFFER_ULONG PhMaxIoHistory;
#ifdef PH_RECORD_MAX_USAGE
extern PH_CIRCULAR_BUFFER_FLOAT PhMaxCpuUsageHistory;
extern PH_CIRCULAR_BUFFER_ULONG64 PhMaxIoReadOtherHistory;
extern PH_CIRCULAR_BUFFER_ULONG64 PhMaxIoWriteHistory;
#endif

// begin_phapppub
#define DPCS_PROCESS_ID ((HANDLE)(LONG_PTR)-2)
#define INTERRUPTS_PROCESS_ID ((HANDLE)(LONG_PTR)-3)

// DPCs, Interrupts and System Idle Process are not real.
// Non-"real" processes can never be opened.
#define PH_IS_REAL_PROCESS_ID(ProcessId) ((LONG_PTR)(ProcessId) > 0)

// DPCs and Interrupts are fake, but System Idle Process is not.
#define PH_IS_FAKE_PROCESS_ID(ProcessId) ((LONG_PTR)(ProcessId) < 0)

// The process item has been removed.
#define PH_PROCESS_ITEM_REMOVED 0x1
// end_phapppub

#define PH_INTEGRITY_STR_LEN 10
#define PH_INTEGRITY_STR_LEN_1 (PH_INTEGRITY_STR_LEN + 1)

// begin_phapppub
typedef enum _VERIFY_RESULT VERIFY_RESULT;
typedef struct _PH_PROCESS_RECORD *PPH_PROCESS_RECORD;

typedef struct _PH_PROCESS_ITEM
{
    PH_HASH_ENTRY HashEntry;
    ULONG State;
    PPH_PROCESS_RECORD Record;

    // Basic

    HANDLE ProcessId;
    HANDLE ParentProcessId;
    PPH_STRING ProcessName;
    ULONG SessionId;

    LARGE_INTEGER CreateTime;

    // Handles

    HANDLE QueryHandle;

    // Parameters

    PPH_STRING FileName;
    PPH_STRING CommandLine;

    // File

    HICON SmallIcon;
    HICON LargeIcon;
    PH_IMAGE_VERSION_INFO VersionInfo;

    // Security

    PPH_STRING UserName;
    TOKEN_ELEVATION_TYPE ElevationType;
    MANDATORY_LEVEL IntegrityLevel;
    PWSTR IntegrityString;

    // Other

    PPH_STRING JobName;
    HANDLE ConsoleHostProcessId;

    // Signature, Packed

    VERIFY_RESULT VerifyResult;
    PPH_STRING VerifySignerName;
    ULONG ImportFunctions;
    ULONG ImportModules;

    // Flags

    union
    {
        ULONG Flags;
        struct
        {
            ULONG UpdateIsDotNet : 1;
            ULONG IsBeingDebugged : 1;
            ULONG IsDotNet : 1;
            ULONG IsElevated : 1;
            ULONG IsInJob : 1;
            ULONG IsInSignificantJob : 1;
            ULONG IsPacked : 1;
            ULONG IsPosix : 1;
            ULONG IsSuspended : 1;
            ULONG IsWow64 : 1;
            ULONG IsImmersive : 1;
            ULONG IsWow64Valid : 1;
            ULONG IsPartiallySuspended : 1;
            ULONG AddedEventSent : 1;
            ULONG Spare : 18;
        };
    };

    // Misc.

    ULONG JustProcessed;
    PH_EVENT Stage1Event;

    PPH_POINTER_LIST ServiceList;
    PH_QUEUED_LOCK ServiceListLock;

    WCHAR ProcessIdString[PH_INT32_STR_LEN_1];
    WCHAR ParentProcessIdString[PH_INT32_STR_LEN_1];
    WCHAR SessionIdString[PH_INT32_STR_LEN_1];

    // Dynamic

    KPRIORITY BasePriority;
    ULONG PriorityClass;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    ULONG NumberOfHandles;
    ULONG NumberOfThreads;

    FLOAT CpuUsage; // Below Windows 7, sum of kernel and user CPU usage; above Windows 7, cycle-based CPU usage.
    FLOAT CpuKernelUsage;
    FLOAT CpuUserUsage;

    PH_UINT64_DELTA CpuKernelDelta;
    PH_UINT64_DELTA CpuUserDelta;
    PH_UINT64_DELTA IoReadDelta;
    PH_UINT64_DELTA IoWriteDelta;
    PH_UINT64_DELTA IoOtherDelta;
    PH_UINT64_DELTA IoReadCountDelta;
    PH_UINT64_DELTA IoWriteCountDelta;
    PH_UINT64_DELTA IoOtherCountDelta;
    PH_UINT32_DELTA ContextSwitchesDelta;
    PH_UINT32_DELTA PageFaultsDelta;
    PH_UINT64_DELTA CycleTimeDelta; // since WIN7

    VM_COUNTERS_EX VmCounters;
    IO_COUNTERS IoCounters;
    SIZE_T WorkingSetPrivateSize; // since VISTA
    ULONG PeakNumberOfThreads; // since WIN7
    ULONG HardFaultCount; // since WIN7

    ULONG SequenceNumber;
    PH_CIRCULAR_BUFFER_FLOAT CpuKernelHistory;
    PH_CIRCULAR_BUFFER_FLOAT CpuUserHistory;
    PH_CIRCULAR_BUFFER_ULONG64 IoReadHistory;
    PH_CIRCULAR_BUFFER_ULONG64 IoWriteHistory;
    PH_CIRCULAR_BUFFER_ULONG64 IoOtherHistory;
    PH_CIRCULAR_BUFFER_SIZE_T PrivateBytesHistory;
    //PH_CIRCULAR_BUFFER_SIZE_T WorkingSetHistory;

    // New fields
    PH_UINTPTR_DELTA PrivateBytesDelta;
    PPH_STRING PackageFullName;

    PH_QUEUED_LOCK RemoveLock;
} PH_PROCESS_ITEM, *PPH_PROCESS_ITEM;
// end_phapppub

// begin_phapppub
// The process itself is dead.
#define PH_PROCESS_RECORD_DEAD 0x1
// An extra reference has been added to the process record for the statistics system.
#define PH_PROCESS_RECORD_STAT_REF 0x2

typedef struct _PH_PROCESS_RECORD
{
    LIST_ENTRY ListEntry;
    LONG RefCount;
    ULONG Flags;

    HANDLE ProcessId;
    HANDLE ParentProcessId;
    ULONG SessionId;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER ExitTime;

    PPH_STRING ProcessName;
    PPH_STRING FileName;
    PPH_STRING CommandLine;
    /*PPH_STRING UserName;*/
} PH_PROCESS_RECORD, *PPH_PROCESS_RECORD;
// end_phapppub

BOOLEAN PhProcessProviderInitialization(
    VOID
    );

// begin_phapppub
PHAPPAPI
PPH_STRING
NTAPI
PhGetClientIdName(
    _In_ PCLIENT_ID ClientId
    );

PHAPPAPI
PPH_STRING
NTAPI
PhGetClientIdNameEx(
    _In_ PCLIENT_ID ClientId,
    _In_opt_ PPH_STRING ProcessName
    );

PHAPPAPI
PWSTR
NTAPI
PhGetProcessPriorityClassString(
    _In_ ULONG PriorityClass
    );
// end_phapppub

PPH_PROCESS_ITEM PhCreateProcessItem(
    _In_ HANDLE ProcessId
    );

// begin_phapppub
PHAPPAPI
PPH_PROCESS_ITEM
NTAPI
PhReferenceProcessItem(
    _In_ HANDLE ProcessId
    );

PHAPPAPI
VOID
NTAPI
PhEnumProcessItems(
    _Out_opt_ PPH_PROCESS_ITEM **ProcessItems,
    _Out_ PULONG NumberOfProcessItems
    );
// end_phapppub

typedef struct _PH_VERIFY_FILE_INFO *PPH_VERIFY_FILE_INFO;

VERIFY_RESULT PhVerifyFileWithAdditionalCatalog(
    _In_ PPH_VERIFY_FILE_INFO Information,
    _In_opt_ PWSTR PackageFullName,
    _Out_opt_ PPH_STRING *SignerName
    );

VERIFY_RESULT PhVerifyFileCached(
    _In_ PPH_STRING FileName,
    _In_opt_ PWSTR PackageFullName,
    _Out_opt_ PPH_STRING *SignerName,
    _In_ BOOLEAN CachedOnly
    );

// begin_phapppub
PHAPPAPI
BOOLEAN
NTAPI
PhGetStatisticsTime(
    _In_opt_ PPH_PROCESS_ITEM ProcessItem,
    _In_ ULONG Index,
    _Out_ PLARGE_INTEGER Time
    );

PHAPPAPI
PPH_STRING
NTAPI
PhGetStatisticsTimeString(
    _In_opt_ PPH_PROCESS_ITEM ProcessItem,
    _In_ ULONG Index
    );
// end_phapppub

VOID PhFlushProcessQueryData(
    _In_ BOOLEAN SendModifiedEvent
    );

VOID PhProcessProviderUpdate(
    _In_ PVOID Object
    );

// begin_phapppub
PHAPPAPI
VOID
NTAPI
PhReferenceProcessRecord(
    _In_ PPH_PROCESS_RECORD ProcessRecord
    );

PHAPPAPI
BOOLEAN
NTAPI
PhReferenceProcessRecordSafe(
    _In_ PPH_PROCESS_RECORD ProcessRecord
    );

PHAPPAPI
VOID
NTAPI
PhReferenceProcessRecordForStatistics(
    _In_ PPH_PROCESS_RECORD ProcessRecord
    );

PHAPPAPI
VOID
NTAPI
PhDereferenceProcessRecord(
    _In_ PPH_PROCESS_RECORD ProcessRecord
    );

PHAPPAPI
PPH_PROCESS_RECORD
NTAPI
PhFindProcessRecord(
    _In_opt_ HANDLE ProcessId,
    _In_ PLARGE_INTEGER Time
    );
// end_phapppub

VOID PhPurgeProcessRecords(
    VOID
    );

// begin_phapppub
PHAPPAPI
PPH_PROCESS_ITEM
NTAPI
PhReferenceProcessItemForParent(
    _In_ HANDLE ParentProcessId,
    _In_ HANDLE ProcessId,
    _In_ PLARGE_INTEGER CreateTime
    );

PHAPPAPI
PPH_PROCESS_ITEM
NTAPI
PhReferenceProcessItemForRecord(
    _In_ PPH_PROCESS_RECORD Record
    );
// end_phapppub

// srvprv

extern PPH_OBJECT_TYPE PhServiceItemType;

PHAPPAPI extern PH_CALLBACK PhServiceAddedEvent; // phapppub
PHAPPAPI extern PH_CALLBACK PhServiceModifiedEvent; // phapppub
PHAPPAPI extern PH_CALLBACK PhServiceRemovedEvent; // phapppub
PHAPPAPI extern PH_CALLBACK PhServicesUpdatedEvent; // phapppub

extern BOOLEAN PhEnableServiceNonPoll;

// begin_phapppub
typedef struct _PH_SERVICE_ITEM
{
    PH_STRINGREF Key; // points to Name
    PPH_STRING Name;
    PPH_STRING DisplayName;

    // State
    ULONG Type;
    ULONG State;
    ULONG ControlsAccepted;
    ULONG Flags; // e.g. SERVICE_RUNS_IN_SYSTEM_PROCESS
    HANDLE ProcessId;

    // Config
    ULONG StartType;
    ULONG ErrorControl;
// end_phapppub
    BOOLEAN DelayedStart;
    BOOLEAN HasTriggers;

    BOOLEAN PendingProcess;
    BOOLEAN NeedsConfigUpdate;

    WCHAR ProcessIdString[PH_INT32_STR_LEN_1];
// begin_phapppub
} PH_SERVICE_ITEM, *PPH_SERVICE_ITEM;
// end_phapppub

// begin_phapppub
typedef struct _PH_SERVICE_MODIFIED_DATA
{
    PPH_SERVICE_ITEM Service;
    PH_SERVICE_ITEM OldService;
} PH_SERVICE_MODIFIED_DATA, *PPH_SERVICE_MODIFIED_DATA;

typedef enum _PH_SERVICE_CHANGE
{
    ServiceStarted,
    ServiceContinued,
    ServicePaused,
    ServiceStopped
} PH_SERVICE_CHANGE, *PPH_SERVICE_CHANGE;
// end_phapppub

BOOLEAN PhServiceProviderInitialization(
    VOID
    );

PPH_SERVICE_ITEM PhCreateServiceItem(
    _In_opt_ LPENUM_SERVICE_STATUS_PROCESS Information
    );

// begin_phapppub
PHAPPAPI
PPH_SERVICE_ITEM
NTAPI
PhReferenceServiceItem(
    _In_ PWSTR Name
    );
// end_phapppub

VOID PhMarkNeedsConfigUpdateServiceItem(
    _In_ PPH_SERVICE_ITEM ServiceItem
    );

// begin_phapppub
PHAPPAPI
PH_SERVICE_CHANGE
NTAPI
PhGetServiceChange(
    _In_ PPH_SERVICE_MODIFIED_DATA Data
    );
// end_phapppub

VOID PhUpdateProcessItemServices(
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

VOID PhServiceProviderUpdate(
    _In_ PVOID Object
    );

// netprv

extern PPH_OBJECT_TYPE PhNetworkItemType;
PHAPPAPI extern PH_CALLBACK PhNetworkItemAddedEvent; // phapppub
PHAPPAPI extern PH_CALLBACK PhNetworkItemModifiedEvent; // phapppub
PHAPPAPI extern PH_CALLBACK PhNetworkItemRemovedEvent; // phapppub
PHAPPAPI extern PH_CALLBACK PhNetworkItemsUpdatedEvent; // phapppub

extern BOOLEAN PhEnableNetworkProviderResolve;

// begin_phapppub
#define PH_NETWORK_OWNER_INFO_SIZE 16

typedef struct _PH_NETWORK_ITEM
{
    ULONG ProtocolType;
    PH_IP_ENDPOINT LocalEndpoint;
    PH_IP_ENDPOINT RemoteEndpoint;
    ULONG State;
    HANDLE ProcessId;

    PPH_STRING ProcessName;
    HICON ProcessIcon;
    BOOLEAN ProcessIconValid;
    PPH_STRING OwnerName;

    BOOLEAN JustResolved;

    WCHAR LocalAddressString[65];
    WCHAR LocalPortString[PH_INT32_STR_LEN_1];
    WCHAR RemoteAddressString[65];
    WCHAR RemotePortString[PH_INT32_STR_LEN_1];
    PPH_STRING LocalHostString;
    PPH_STRING RemoteHostString;

    LARGE_INTEGER CreateTime;
    ULONGLONG OwnerInfo[PH_NETWORK_OWNER_INFO_SIZE];
} PH_NETWORK_ITEM, *PPH_NETWORK_ITEM;
// end_phapppub

BOOLEAN PhNetworkProviderInitialization(
    VOID
    );

PPH_NETWORK_ITEM PhCreateNetworkItem(
    VOID
    );

// begin_phapppub
PHAPPAPI
PPH_NETWORK_ITEM
NTAPI
PhReferenceNetworkItem(
    _In_ ULONG ProtocolType,
    _In_ PPH_IP_ENDPOINT LocalEndpoint,
    _In_ PPH_IP_ENDPOINT RemoteEndpoint,
    _In_ HANDLE ProcessId
    );
// end_phapppub

PPH_STRING PhGetHostNameFromAddress(
    _In_ PPH_IP_ADDRESS Address
    );

VOID PhNetworkProviderUpdate(
    _In_ PVOID Object
    );

// begin_phapppub
PHAPPAPI
PWSTR
NTAPI
PhGetProtocolTypeName(
    _In_ ULONG ProtocolType
    );

PHAPPAPI
PWSTR
NTAPI
PhGetTcpStateName(
    _In_ ULONG State
    );
// end_phapppub

// modprv

extern PPH_OBJECT_TYPE PhModuleProviderType;
extern PPH_OBJECT_TYPE PhModuleItemType;

// begin_phapppub
typedef struct _PH_MODULE_ITEM
{
    PVOID BaseAddress;
    ULONG Size;
    ULONG Flags;
    ULONG Type;
    USHORT LoadReason;
    USHORT LoadCount;
    PPH_STRING Name;
    PPH_STRING FileName;
    PH_IMAGE_VERSION_INFO VersionInfo;

    WCHAR BaseAddressString[PH_PTR_STR_LEN_1];

    BOOLEAN IsFirst;
    BOOLEAN JustProcessed;

    VERIFY_RESULT VerifyResult;
    PPH_STRING VerifySignerName;

    ULONG ImageTimeDateStamp;
    USHORT ImageCharacteristics;
    USHORT ImageDllCharacteristics;

    LARGE_INTEGER LoadTime;
} PH_MODULE_ITEM, *PPH_MODULE_ITEM;

typedef struct _PH_MODULE_PROVIDER
{
    PPH_HASHTABLE ModuleHashtable;
    PH_FAST_LOCK ModuleHashtableLock;
    PH_CALLBACK ModuleAddedEvent;
    PH_CALLBACK ModuleModifiedEvent;
    PH_CALLBACK ModuleRemovedEvent;
    PH_CALLBACK UpdatedEvent;

    HANDLE ProcessId;
    HANDLE ProcessHandle;
    PPH_STRING PackageFullName;
    SLIST_HEADER QueryListHead;
    NTSTATUS RunStatus;
} PH_MODULE_PROVIDER, *PPH_MODULE_PROVIDER;
// end_phapppub

BOOLEAN PhModuleProviderInitialization(
    VOID
    );

PPH_MODULE_PROVIDER PhCreateModuleProvider(
    _In_ HANDLE ProcessId
    );

PPH_MODULE_ITEM PhCreateModuleItem(
    VOID
    );

PPH_MODULE_ITEM PhReferenceModuleItem(
    _In_ PPH_MODULE_PROVIDER ModuleProvider,
    _In_ PVOID BaseAddress
    );

VOID PhDereferenceAllModuleItems(
    _In_ PPH_MODULE_PROVIDER ModuleProvider
    );

VOID PhModuleProviderUpdate(
    _In_ PVOID Object
    );

// thrdprv

extern PPH_OBJECT_TYPE PhThreadProviderType;
extern PPH_OBJECT_TYPE PhThreadItemType;

// begin_phapppub
typedef struct _PH_THREAD_ITEM
{
    HANDLE ThreadId;

    LARGE_INTEGER CreateTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;

    FLOAT CpuUsage;
    PH_UINT64_DELTA CpuKernelDelta;
    PH_UINT64_DELTA CpuUserDelta;

    PH_UINT32_DELTA ContextSwitchesDelta;
    PH_UINT64_DELTA CyclesDelta;
    LONG Priority;
    LONG BasePriority;
    ULONG64 StartAddress;
    PPH_STRING StartAddressString;
    PPH_STRING StartAddressFileName;
    enum _PH_SYMBOL_RESOLVE_LEVEL StartAddressResolveLevel;
    KTHREAD_STATE State;
    KWAIT_REASON WaitReason;
    LONG PriorityWin32;
    PPH_STRING ServiceName;

    HANDLE ThreadHandle;

    BOOLEAN IsGuiThread;
    BOOLEAN JustResolved;

    WCHAR ThreadIdString[PH_INT32_STR_LEN_1];
} PH_THREAD_ITEM, *PPH_THREAD_ITEM;

typedef enum _PH_KNOWN_PROCESS_TYPE PH_KNOWN_PROCESS_TYPE;

typedef struct _PH_THREAD_PROVIDER
{
    PPH_HASHTABLE ThreadHashtable;
    PH_FAST_LOCK ThreadHashtableLock;
    PH_CALLBACK ThreadAddedEvent;
    PH_CALLBACK ThreadModifiedEvent;
    PH_CALLBACK ThreadRemovedEvent;
    PH_CALLBACK UpdatedEvent;
    PH_CALLBACK LoadingStateChangedEvent;

    HANDLE ProcessId;
    HANDLE ProcessHandle;
    BOOLEAN HasServices;
    BOOLEAN HasServicesKnown;
    BOOLEAN Terminating;
    struct _PH_SYMBOL_PROVIDER *SymbolProvider;

    SLIST_HEADER QueryListHead;
    PH_QUEUED_LOCK LoadSymbolsLock;
    LONG SymbolsLoading;
    ULONG64 RunId;
    ULONG64 SymbolsLoadedRunId;
} PH_THREAD_PROVIDER, *PPH_THREAD_PROVIDER;
// end_phapppub

BOOLEAN PhThreadProviderInitialization(
    VOID
    );

PPH_THREAD_PROVIDER PhCreateThreadProvider(
    _In_ HANDLE ProcessId
    );

VOID PhRegisterThreadProvider(
    _In_ PPH_THREAD_PROVIDER ThreadProvider,
    _Out_ PPH_CALLBACK_REGISTRATION CallbackRegistration
    );

VOID PhUnregisterThreadProvider(
    _In_ PPH_THREAD_PROVIDER ThreadProvider,
    _In_ PPH_CALLBACK_REGISTRATION CallbackRegistration
    );

VOID PhSetTerminatingThreadProvider(
    _Inout_ PPH_THREAD_PROVIDER ThreadProvider
    );

VOID PhLoadSymbolsThreadProvider(
    _In_ PPH_THREAD_PROVIDER ThreadProvider
    );

PPH_THREAD_ITEM PhCreateThreadItem(
    _In_ HANDLE ThreadId
    );

PPH_THREAD_ITEM PhReferenceThreadItem(
    _In_ PPH_THREAD_PROVIDER ThreadProvider,
    _In_ HANDLE ThreadId
    );

VOID PhDereferenceAllThreadItems(
    _In_ PPH_THREAD_PROVIDER ThreadProvider
    );

// begin_phapppub
PHAPPAPI
PPH_STRING
NTAPI
PhGetThreadPriorityWin32String(
    _In_ LONG PriorityWin32
    );
// end_phapppub

VOID PhThreadProviderInitialUpdate(
    _In_ PPH_THREAD_PROVIDER ThreadProvider
    );

// hndlprv

extern PPH_OBJECT_TYPE PhHandleProviderType;
extern PPH_OBJECT_TYPE PhHandleItemType;

// begin_phapppub
#define PH_HANDLE_FILE_SHARED_READ 0x1
#define PH_HANDLE_FILE_SHARED_WRITE 0x2
#define PH_HANDLE_FILE_SHARED_DELETE 0x4
#define PH_HANDLE_FILE_SHARED_MASK 0x7

typedef struct _PH_HANDLE_ITEM
{
    PH_HASH_ENTRY HashEntry;

    HANDLE Handle;
    PVOID Object;
    ULONG Attributes;
    ACCESS_MASK GrantedAccess;
    ULONG FileFlags;

    PPH_STRING TypeName;
    PPH_STRING ObjectName;
    PPH_STRING BestObjectName;

    WCHAR HandleString[PH_PTR_STR_LEN_1];
    WCHAR ObjectString[PH_PTR_STR_LEN_1];
    WCHAR GrantedAccessString[PH_PTR_STR_LEN_1];
} PH_HANDLE_ITEM, *PPH_HANDLE_ITEM;

typedef struct _PH_HANDLE_PROVIDER
{
    PPH_HASH_ENTRY *HandleHashSet;
    ULONG HandleHashSetSize;
    ULONG HandleHashSetCount;
    PH_QUEUED_LOCK HandleHashSetLock;

    PH_CALLBACK HandleAddedEvent;
    PH_CALLBACK HandleModifiedEvent;
    PH_CALLBACK HandleRemovedEvent;
    PH_CALLBACK UpdatedEvent;

    HANDLE ProcessId;
    HANDLE ProcessHandle;

    PPH_HASHTABLE TempListHashtable;
    NTSTATUS RunStatus;
} PH_HANDLE_PROVIDER, *PPH_HANDLE_PROVIDER;
// end_phapppub

BOOLEAN PhHandleProviderInitialization(
    VOID
    );

PPH_HANDLE_PROVIDER PhCreateHandleProvider(
    _In_ HANDLE ProcessId
    );

PPH_HANDLE_ITEM PhCreateHandleItem(
    _In_opt_ PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Handle
    );

PPH_HANDLE_ITEM PhReferenceHandleItem(
    _In_ PPH_HANDLE_PROVIDER HandleProvider,
    _In_ HANDLE Handle
    );

VOID PhDereferenceAllHandleItems(
    _In_ PPH_HANDLE_PROVIDER HandleProvider
    );

NTSTATUS PhEnumHandlesGeneric(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ProcessHandle,
    _Out_ PSYSTEM_HANDLE_INFORMATION_EX *Handles,
    _Out_ PBOOLEAN FilterNeeded
    );

VOID PhHandleProviderUpdate(
    _In_ PVOID Object
    );

// memprv

extern PPH_OBJECT_TYPE PhMemoryItemType;

// begin_phapppub
typedef enum _PH_MEMORY_REGION_TYPE
{
    UnknownRegion,
    CustomRegion,
    UnusableRegion,
    MappedFileRegion,
    UserSharedDataRegion,
    PebRegion,
    Peb32Region,
    TebRegion,
    Teb32Region, // Not used
    StackRegion,
    Stack32Region,
    HeapRegion,
    Heap32Region,
    HeapSegmentRegion,
    HeapSegment32Region
} PH_MEMORY_REGION_TYPE;

typedef struct _PH_MEMORY_ITEM
{
    LIST_ENTRY ListEntry;
    PH_AVL_LINKS Links;

    union
    {
        struct
        {
            PVOID BaseAddress;
            PVOID AllocationBase;
            ULONG AllocationProtect;
            SIZE_T RegionSize;
            ULONG State;
            ULONG Protect;
            ULONG Type;
        };
        MEMORY_BASIC_INFORMATION BasicInfo;
    };

    struct _PH_MEMORY_ITEM *AllocationBaseItem;

    SIZE_T CommittedSize;
    SIZE_T PrivateSize;

    SIZE_T TotalWorkingSetPages;
    SIZE_T PrivateWorkingSetPages;
    SIZE_T SharedWorkingSetPages;
    SIZE_T ShareableWorkingSetPages;
    SIZE_T LockedWorkingSetPages;

    PH_MEMORY_REGION_TYPE RegionType;

    union
    {
        struct
        {
            PPH_STRING Text;
            BOOLEAN PropertyOfAllocationBase;
        } Custom;
        struct
        {
            PPH_STRING FileName;
        } MappedFile;
        struct
        {
            HANDLE ThreadId;
        } Teb;
        struct
        {
            HANDLE ThreadId;
        } Stack;
        struct
        {
            ULONG Index;
        } Heap;
        struct
        {
            struct _PH_MEMORY_ITEM *HeapItem;
        } HeapSegment;
    } u;
} PH_MEMORY_ITEM, *PPH_MEMORY_ITEM;

typedef struct _PH_MEMORY_ITEM_LIST
{
    HANDLE ProcessId;
    PH_AVL_TREE Set;
    LIST_ENTRY ListHead;
} PH_MEMORY_ITEM_LIST, *PPH_MEMORY_ITEM_LIST;
// end_phapppub

BOOLEAN PhMemoryProviderInitialization(
    VOID
    );

VOID PhGetMemoryProtectionString(
    _In_ ULONG Protection,
    _Out_writes_(17) PWSTR String
    );

PWSTR PhGetMemoryStateString(
    _In_ ULONG State
    );

PWSTR PhGetMemoryTypeString(
    _In_ ULONG Type
    );

PPH_MEMORY_ITEM PhCreateMemoryItem(
    VOID
    );

// begin_phapppub
PHAPPAPI
VOID
NTAPI
PhDeleteMemoryItemList(
    _In_ PPH_MEMORY_ITEM_LIST List
    );

PHAPPAPI
PPH_MEMORY_ITEM
NTAPI
PhLookupMemoryItemList(
    _In_ PPH_MEMORY_ITEM_LIST List,
    _In_ PVOID Address
    );

#define PH_QUERY_MEMORY_IGNORE_FREE 0x1
#define PH_QUERY_MEMORY_REGION_TYPE 0x2
#define PH_QUERY_MEMORY_WS_COUNTERS 0x4

PHAPPAPI
NTSTATUS
NTAPI
PhQueryMemoryItemList(
    _In_ HANDLE ProcessId,
    _In_ ULONG Flags,
    _Out_ PPH_MEMORY_ITEM_LIST List
    );
// end_phapppub

#endif
