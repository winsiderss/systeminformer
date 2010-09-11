#ifndef PROVIDERS_H
#define PROVIDERS_H

// procprv

#define PH_RECORD_MAX_USAGE
//#define PH_ENABLE_VERIFY_CACHE

#ifndef PH_PROCPRV_PRIVATE
extern PPH_OBJECT_TYPE PhProcessItemType;

PHAPPAPI extern PH_CALLBACK PhProcessAddedEvent;
PHAPPAPI extern PH_CALLBACK PhProcessModifiedEvent;
PHAPPAPI extern PH_CALLBACK PhProcessRemovedEvent;
PHAPPAPI extern PH_CALLBACK PhProcessesUpdatedEvent;

extern PPH_LIST PhProcessRecordList;
extern PH_QUEUED_LOCK PhProcessRecordListLock;

extern ULONG PhStatisticsSampleCount;
extern BOOLEAN PhEnableProcessQueryStage2;
extern BOOLEAN PhEnablePurgeProcessRecords;

extern SYSTEM_PERFORMANCE_INFORMATION PhPerfInformation;
extern PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PhCpuInformation;
extern SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PhCpuTotals;
extern ULONG PhTotalProcesses;
extern ULONG PhTotalThreads;
extern ULONG PhTotalHandles;

extern FLOAT PhCpuKernelUsage;
extern FLOAT PhCpuUserUsage;
extern PFLOAT PhCpusKernelUsage;
extern PFLOAT PhCpusUserUsage;

extern PH_UINT64_DELTA PhCpuKernelDelta;
extern PH_UINT64_DELTA PhCpuUserDelta;
extern PH_UINT64_DELTA PhCpuOtherDelta;

extern PPH_UINT64_DELTA PhCpusKernelDelta;
extern PPH_UINT64_DELTA PhCpusUserDelta;
extern PPH_UINT64_DELTA PhCpusOtherDelta;

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
#endif

#define DPCS_PROCESS_ID ((HANDLE)(LONG_PTR)-2)
#define INTERRUPTS_PROCESS_ID ((HANDLE)(LONG_PTR)-3)

// DPCs, Interrupts and System Idle Process are not real.
// Non-"real" processes can never be opened.
#define PH_IS_REAL_PROCESS_ID(ProcessId) ((LONG_PTR)(ProcessId) > 0)

// DPCs and Interrupts are fake, but System Idle Process is not.
#define PH_IS_FAKE_PROCESS_ID(ProcessId) ((LONG_PTR)(ProcessId) < 0)

// The process item has been removed.
#define PH_PROCESS_ITEM_REMOVED 0x1
// An extra reference has been added to the process record for the statistics system.
#define PH_PROCESS_ITEM_RECORD_STAT_REF 0x2

#define PH_INTEGRITY_STR_LEN 10
#define PH_INTEGRITY_STR_LEN_1 (PH_INTEGRITY_STR_LEN + 1)

typedef struct _PH_PROCESS_RECORD *PPH_PROCESS_RECORD;

typedef struct _PH_PROCESS_ITEM
{
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
    PH_INTEGRITY IntegrityLevel;

    // Other

    PPH_STRING JobName;
    HANDLE ConsoleHostProcessId;

    // Signature, Packed

    VERIFY_RESULT VerifyResult;
    PPH_STRING VerifySignerName;
    ULONG ImportFunctions;
    ULONG ImportModules;

    // Flags

    ULONG HasParent : 1;
    ULONG IsBeingDebugged : 1;
    ULONG IsDotNet : 1;
    ULONG IsElevated : 1;
    ULONG IsInJob : 1;
    ULONG IsInSignificantJob : 1;
    ULONG IsPacked : 1;
    ULONG IsPosix : 1;
    ULONG IsSuspended : 1;
    ULONG IsWow64 : 1;

    // Misc.

    BOOLEAN JustProcessed;
    PH_EVENT Stage1Event;

    PPH_POINTER_LIST ServiceList;
    PH_QUEUED_LOCK ServiceListLock;

    WCHAR ProcessIdString[PH_INT32_STR_LEN_1];
    WCHAR ParentProcessIdString[PH_INT32_STR_LEN_1];
    WCHAR SessionIdString[PH_INT32_STR_LEN_1];
    WCHAR IntegrityString[PH_INTEGRITY_STR_LEN_1];

    // Dynamic

    KPRIORITY BasePriority;
    ULONG PriorityClassWin32;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    ULONG NumberOfHandles;
    ULONG NumberOfThreads;

    FLOAT CpuKernelUsage;
    FLOAT CpuUserUsage;
    FLOAT CpuUsage; // from 0 to 1

    PH_UINT64_DELTA CpuKernelDelta; // volatile
    PH_UINT64_DELTA CpuUserDelta; // volatile
    PH_UINT64_DELTA IoReadDelta; // volatile
    PH_UINT64_DELTA IoWriteDelta; // volatile
    PH_UINT64_DELTA IoOtherDelta; // volatile

    VM_COUNTERS_EX VmCounters; // volatile
    IO_COUNTERS IoCounters; // volatile

    ULONG SequenceNumber;
    PH_CIRCULAR_BUFFER_FLOAT CpuKernelHistory;
    PH_CIRCULAR_BUFFER_FLOAT CpuUserHistory;
    PH_CIRCULAR_BUFFER_ULONG64 IoReadHistory;
    PH_CIRCULAR_BUFFER_ULONG64 IoWriteHistory;
    PH_CIRCULAR_BUFFER_ULONG64 IoOtherHistory;
    PH_CIRCULAR_BUFFER_SIZE_T PrivateBytesHistory;
    //PH_CIRCULAR_BUFFER_SIZE_T WorkingSetHistory;
} PH_PROCESS_ITEM, *PPH_PROCESS_ITEM;

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

BOOLEAN PhProcessProviderInitialization();

PHAPPAPI
PPH_STRING PhGetClientIdName(
    __in PCLIENT_ID ClientId
    );

PHAPPAPI
PPH_STRING PhGetClientIdNameEx(
    __in PCLIENT_ID ClientId,
    __in_opt PPH_STRING ProcessName
    );

PHAPPAPI
PWSTR PhGetProcessPriorityClassWin32String(
    __in ULONG PriorityClassWin32
    );

PPH_PROCESS_ITEM PhCreateProcessItem(
    __in HANDLE ProcessId
    );

PHAPPAPI
PPH_PROCESS_ITEM PhReferenceProcessItem(
    __in HANDLE ProcessId
    );

PHAPPAPI
VOID PhEnumProcessItems(
    __out_opt PPH_PROCESS_ITEM **ProcessItems,
    __out PULONG NumberOfProcessItems
    );

PHAPPAPI
BOOLEAN PhGetStatisticsTime(
    __in_opt PPH_PROCESS_ITEM ProcessItem,
    __in ULONG Index,
    __out PLARGE_INTEGER Time
    );

PHAPPAPI
PPH_STRING PhGetStatisticsTimeString(
    __in_opt PPH_PROCESS_ITEM ProcessItem,
    __in ULONG Index
    );

VOID PhProcessProviderUpdate(
    __in PVOID Object
    );

PHAPPAPI
VOID PhReferenceProcessRecord(
    __in PPH_PROCESS_RECORD ProcessRecord
    );

PHAPPAPI
BOOLEAN PhReferenceProcessRecordSafe(
    __in PPH_PROCESS_RECORD ProcessRecord
    );

PHAPPAPI
VOID PhDereferenceProcessRecord(
    __in PPH_PROCESS_RECORD ProcessRecord
    );

PHAPPAPI
PPH_PROCESS_RECORD PhFindProcessRecord(
    __in_opt HANDLE ProcessId,
    __in PLARGE_INTEGER Time
    );

VOID PhPurgeProcessRecords();

PHAPPAPI
PPH_PROCESS_ITEM PhReferenceProcessItemForParent(
    __in HANDLE ParentProcessId,
    __in HANDLE ProcessId,
    __in PLARGE_INTEGER CreateTime
    );

PHAPPAPI
PPH_PROCESS_ITEM PhReferenceProcessItemForRecord(
    __in PPH_PROCESS_RECORD Record
    );

// srvprv

#ifndef PH_SRVPRV_PRIVATE
extern PPH_OBJECT_TYPE PhServiceItemType;

PHAPPAPI extern PH_CALLBACK PhServiceAddedEvent;
PHAPPAPI extern PH_CALLBACK PhServiceModifiedEvent;
PHAPPAPI extern PH_CALLBACK PhServiceRemovedEvent;
PHAPPAPI extern PH_CALLBACK PhServicesUpdatedEvent;
#endif

typedef struct _PH_SERVICE_ITEM
{
    PH_STRINGREF Key; // points to Name
    PPH_STRING Name;
    PPH_STRING DisplayName;

    // State
    ULONG Type;
    ULONG State;
    ULONG ControlsAccepted;
    HANDLE ProcessId;

    // Config
    ULONG StartType;
    ULONG ErrorControl;

    BOOLEAN PendingProcess;
    BOOLEAN NeedsConfigUpdate;

    WCHAR ProcessIdString[PH_INT32_STR_LEN_1];
} PH_SERVICE_ITEM, *PPH_SERVICE_ITEM;

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

BOOLEAN PhServiceProviderInitialization();

PPH_SERVICE_ITEM PhCreateServiceItem(
    __in_opt LPENUM_SERVICE_STATUS_PROCESS Information
    );

PHAPPAPI
PPH_SERVICE_ITEM PhReferenceServiceItem(
    __in PWSTR Name
    );

VOID PhMarkNeedsConfigUpdateServiceItem(
    __in PPH_SERVICE_ITEM ServiceItem
    );

PHAPPAPI
PH_SERVICE_CHANGE PhGetServiceChange(
    __in PPH_SERVICE_MODIFIED_DATA Data
    );

VOID PhUpdateProcessItemServices(
    __in PPH_PROCESS_ITEM ProcessItem
    );

VOID PhServiceProviderUpdate(
    __in PVOID Object
    );

// netprv

#ifndef PH_NETPRV_PRIVATE
extern PPH_OBJECT_TYPE PhNetworkItemType;
extern PH_CALLBACK PhNetworkItemAddedEvent;
extern PH_CALLBACK PhNetworkItemModifiedEvent;
extern PH_CALLBACK PhNetworkItemRemovedEvent;
extern PH_CALLBACK PhNetworkItemsUpdatedEvent;
#endif

#include <phnet.h>

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

    ULONGLONG OwnerInfo[PH_NETWORK_OWNER_INFO_SIZE];
} PH_NETWORK_ITEM, *PPH_NETWORK_ITEM;

BOOLEAN PhNetworkProviderInitialization();

PPH_NETWORK_ITEM PhCreateNetworkItem();

PHAPPAPI
PPH_NETWORK_ITEM PhReferenceNetworkItem(
    __in ULONG ProtocolType,
    __in PPH_IP_ENDPOINT LocalEndpoint,
    __in PPH_IP_ENDPOINT RemoteEndpoint,
    __in HANDLE ProcessId
    );

PPH_STRING PhGetHostNameFromAddress(
    __in PPH_IP_ADDRESS Address
    );

VOID PhNetworkProviderUpdate(
    __in PVOID Object
    );

PHAPPAPI
PWSTR PhGetProtocolTypeName(
    __in ULONG ProtocolType
    );

PHAPPAPI
PWSTR PhGetTcpStateName(
    __in ULONG State
    );

// modprv

#ifndef PH_MODPRV_PRIVATE
extern PPH_OBJECT_TYPE PhModuleProviderType;
extern PPH_OBJECT_TYPE PhModuleItemType;
#endif

typedef struct _PH_MODULE_ITEM
{
    PVOID BaseAddress;
    ULONG Size;
    ULONG Flags;
    ULONG Type;
    PPH_STRING Name;
    PPH_STRING FileName;
    PH_IMAGE_VERSION_INFO VersionInfo;

    PPH_STRING SizeString;

    WCHAR BaseAddressString[PH_PTR_STR_LEN_1];

    BOOLEAN IsFirst;
} PH_MODULE_ITEM, *PPH_MODULE_ITEM;

typedef struct _PH_MODULE_PROVIDER
{
    PPH_HASHTABLE ModuleHashtable;
    PH_FAST_LOCK ModuleHashtableLock;
    PH_CALLBACK ModuleAddedEvent;
    PH_CALLBACK ModuleRemovedEvent;
    PH_CALLBACK UpdatedEvent;

    HANDLE ProcessId;
    HANDLE ProcessHandle;
} PH_MODULE_PROVIDER, *PPH_MODULE_PROVIDER;

BOOLEAN PhModuleProviderInitialization();

PPH_MODULE_PROVIDER PhCreateModuleProvider(
    __in HANDLE ProcessId
    );

PPH_MODULE_ITEM PhCreateModuleItem();

PPH_MODULE_ITEM PhReferenceModuleItem(
    __in PPH_MODULE_PROVIDER ModuleProvider,
    __in PVOID BaseAddress
    );

VOID PhDereferenceAllModuleItems(
    __in PPH_MODULE_PROVIDER ModuleProvider
    );

VOID PhModuleProviderUpdate(
    __in PVOID Object
    );

// thrdprv

#ifndef PH_THRDPRV_PRIVATE
extern PPH_OBJECT_TYPE PhThreadProviderType;
extern PPH_OBJECT_TYPE PhThreadItemType;
#endif

typedef struct _PH_THREAD_ITEM
{
    HANDLE ThreadId;

    LARGE_INTEGER CreateTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;

    PH_UINT32_DELTA ContextSwitchesDelta;
    PH_UINT64_DELTA CyclesDelta;
    LONG Priority;
    LONG BasePriority;
    ULONG64 StartAddress;
    PPH_STRING StartAddressString;
    PPH_STRING StartAddressFileName;
    PH_SYMBOL_RESOLVE_LEVEL StartAddressResolveLevel;
    KTHREAD_STATE State;
    KWAIT_REASON WaitReason;
    LONG PriorityWin32;
    PPH_STRING PriorityWin32String;
    PPH_STRING ServiceName;

    HANDLE ThreadHandle;

    BOOLEAN IsGuiThread;
    BOOLEAN JustResolved;

    PPH_STRING ContextSwitchesDeltaString;
    PPH_STRING CyclesDeltaString;

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
    PPH_SYMBOL_PROVIDER SymbolProvider;
    PH_EVENT SymbolsLoadedEvent;
    LONG SymbolsLoading;
    SLIST_HEADER QueryListHead;
} PH_THREAD_PROVIDER, *PPH_THREAD_PROVIDER;

BOOLEAN PhThreadProviderInitialization();

PPH_THREAD_PROVIDER PhCreateThreadProvider(
    __in HANDLE ProcessId
    );

PPH_THREAD_ITEM PhCreateThreadItem(
    __in HANDLE ThreadId
    );

PPH_THREAD_ITEM PhReferenceThreadItem(
    __in PPH_THREAD_PROVIDER ThreadProvider,
    __in HANDLE ThreadId
    );

VOID PhDereferenceAllThreadItems(
    __in PPH_THREAD_PROVIDER ThreadProvider
    );

PHAPPAPI
PPH_STRING PhGetThreadPriorityWin32String(
    __in LONG PriorityWin32
    );

VOID PhThreadProviderUpdate(
    __in PVOID Object
    );

// hndlprv

#ifndef PH_HNDLPRV_PRIVATE
extern PPH_OBJECT_TYPE PhHandleProviderType;
extern PPH_OBJECT_TYPE PhHandleItemType;
#endif

typedef struct _PH_HANDLE_ITEM
{
    HANDLE Handle;
    PVOID Object;
    ULONG Attributes;
    ACCESS_MASK GrantedAccess;

    PPH_STRING TypeName;
    PPH_STRING ObjectName;
    PPH_STRING BestObjectName;

    WCHAR HandleString[PH_PTR_STR_LEN_1];
    WCHAR ObjectString[PH_PTR_STR_LEN_1];
    WCHAR GrantedAccessString[PH_PTR_STR_LEN_1];
} PH_HANDLE_ITEM, *PPH_HANDLE_ITEM;

typedef struct _PH_HANDLE_PROVIDER
{
    PPH_HASHTABLE HandleHashtable;
    PH_QUEUED_LOCK HandleHashtableLock;
    PH_CALLBACK HandleAddedEvent;
    PH_CALLBACK HandleModifiedEvent;
    PH_CALLBACK HandleRemovedEvent;
    PH_CALLBACK UpdatedEvent;

    HANDLE ProcessId;
    HANDLE ProcessHandle;

    PPH_HASHTABLE TempListHashtable;
} PH_HANDLE_PROVIDER, *PPH_HANDLE_PROVIDER;

BOOLEAN PhHandleProviderInitialization();

PPH_HANDLE_PROVIDER PhCreateHandleProvider(
    __in HANDLE ProcessId
    );

PPH_HANDLE_ITEM PhCreateHandleItem(
    __in_opt PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Handle
    );

PPH_HANDLE_ITEM PhReferenceHandleItem(
    __in PPH_HANDLE_PROVIDER HandleProvider,
    __in HANDLE Handle
    );

VOID PhDereferenceAllHandleItems(
    __in PPH_HANDLE_PROVIDER HandleProvider
    );

NTSTATUS PhEnumHandlesGeneric(
    __in HANDLE ProcessId,
    __in HANDLE ProcessHandle,
    __out PSYSTEM_HANDLE_INFORMATION_EX *Handles,
    __out PBOOLEAN FilterNeeded
    );

VOID PhHandleProviderUpdate(
    __in PVOID Object
    );

// memprv

#ifndef PH_MEMPRV_PRIVATE
extern PPH_OBJECT_TYPE PhMemoryItemType;
#endif

typedef struct _PH_MEMORY_ITEM
{
    PVOID BaseAddress;
    ULONG_PTR Size;
    ULONG Flags;
    ULONG Protection;

    WCHAR BaseAddressString[PH_PTR_STR_LEN_1];
    PPH_STRING Name;
} PH_MEMORY_ITEM, *PPH_MEMORY_ITEM;

typedef struct _PH_MEMORY_PROVIDER *PPH_MEMORY_PROVIDER;

typedef BOOLEAN (NTAPI *PPH_MEMORY_PROVIDER_CALLBACK)(
    __in PPH_MEMORY_PROVIDER Provider,
    __in __assumeRefs(1) PPH_MEMORY_ITEM MemoryItem
    );

typedef struct _PH_MEMORY_PROVIDER
{
    PPH_MEMORY_PROVIDER_CALLBACK Callback;
    PVOID Context;

    HANDLE ProcessId;
    HANDLE ProcessHandle;

    BOOLEAN IgnoreFreeRegions;
} PH_MEMORY_PROVIDER, *PPH_MEMORY_PROVIDER;

BOOLEAN PhMemoryProviderInitialization();

VOID PhInitializeMemoryProvider(
    __out PPH_MEMORY_PROVIDER Provider,
    __in HANDLE ProcessId,
    __in PPH_MEMORY_PROVIDER_CALLBACK Callback,
    __in_opt PVOID Context
    );

VOID PhDeleteMemoryProvider(
    __inout PPH_MEMORY_PROVIDER Provider
    );

PPH_MEMORY_ITEM PhCreateMemoryItem();

PHAPPAPI
VOID PhGetMemoryProtectionString(
    __in ULONG Protection,
    __out_ecount(17) PWSTR String
    );

PHAPPAPI
PWSTR PhGetMemoryStateString(
    __in ULONG State
    );

PHAPPAPI
PWSTR PhGetMemoryTypeString(
    __in ULONG Type
    );

VOID PhMemoryProviderUpdate(
    __in PPH_MEMORY_PROVIDER Provider
    );

#endif
