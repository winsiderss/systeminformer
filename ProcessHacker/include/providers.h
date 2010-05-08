#ifndef PROVIDERS_H
#define PROVIDERS_H

// procprv

#ifndef PROCPRV_PRIVATE
extern PPH_OBJECT_TYPE PhProcessItemType;

extern PH_CALLBACK PhProcessAddedEvent;
extern PH_CALLBACK PhProcessModifiedEvent;
extern PH_CALLBACK PhProcessRemovedEvent;
extern PH_CALLBACK PhProcessesUpdatedEvent;
#endif

#define PH_INTEGRITY_STR_LEN 10
#define PH_INTEGRITY_STR_LEN_1 (PH_INTEGRITY_STR_LEN + 1)

typedef struct _PH_PROCESS_ITEM
{
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

    // Signature

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
    WCHAR CpuUsageString[PH_INT32_STR_LEN_1]; // volatile

    // Statistics

    FLOAT CpuUsage; // from 0 to 1

    PH_UINT64_DELTA CpuKernelDelta; // volatile
    PH_UINT64_DELTA CpuUserDelta; // volatile
    PH_UINT64_DELTA IoReadDelta; // volatile
    PH_UINT64_DELTA IoWriteDelta; // volatile
    PH_UINT64_DELTA IoOtherDelta; // volatile

    VM_COUNTERS_EX VmCounters; // volatile
} PH_PROCESS_ITEM, *PPH_PROCESS_ITEM;

BOOLEAN PhInitializeProcessProvider();

PPH_PROCESS_ITEM PhCreateProcessItem(
    __in HANDLE ProcessId
    );

PPH_PROCESS_ITEM PhReferenceProcessItem(
    __in HANDLE ProcessId
    );

PPH_STRING PhGetClientIdName(
    __in PCLIENT_ID ClientId
    );

PWSTR PhGetProcessPriorityClassWin32String(
    __in ULONG PriorityClassWin32
    );

VOID PhProcessProviderUpdate(
    __in PVOID Object
    );

// srvprv

#ifndef SRVPRV_PRIVATE
extern PPH_OBJECT_TYPE PhServiceItemType;

extern PH_CALLBACK PhServiceAddedEvent;
extern PH_CALLBACK PhServiceModifiedEvent;
extern PH_CALLBACK PhServiceRemovedEvent;
extern PH_CALLBACK PhServicesUpdatedEvent;
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

BOOLEAN PhInitializeServiceProvider();

PPH_SERVICE_ITEM PhCreateServiceItem(
    __in_opt LPENUM_SERVICE_STATUS_PROCESS Information
    );

PPH_SERVICE_ITEM PhReferenceServiceItem(
    __in PWSTR Name
    );

VOID PhMarkNeedsConfigUpdateServiceItem(
    __in PPH_SERVICE_ITEM ServiceItem
    );

PH_SERVICE_CHANGE PhGetServiceChange(
    __in PPH_SERVICE_MODIFIED_DATA Data
    );

VOID PhUpdateProcessItemServices(
    __in PPH_PROCESS_ITEM ProcessItem
    );

VOID PhServiceProviderUpdate(
    __in PVOID Object
    );

// modprv

#ifndef MODPRV_PRIVATE
extern PPH_OBJECT_TYPE PhModuleProviderType;
extern PPH_OBJECT_TYPE PhModuleItemType;
#endif

typedef struct _PH_MODULE_ITEM
{
    PVOID BaseAddress;
    ULONG Size;
    ULONG Flags;
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

BOOLEAN PhInitializeModuleProvider();

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

#ifndef THRDPRV_PRIVATE
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
    PPH_QUEUE QueryQueue;
    PH_MUTEX QueryQueueLock;
} PH_THREAD_PROVIDER, *PPH_THREAD_PROVIDER;

BOOLEAN PhInitializeThreadProvider();

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

PPH_STRING PhGetThreadPriorityWin32String(
    __in LONG PriorityWin32
    );

VOID PhThreadProviderUpdate(
    __in PVOID Object
    );

// hndlprv

#ifndef HNDLPRV_PRIVATE
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

BOOLEAN PhInitializeHandleProvider();

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

VOID PhHandleProviderUpdate(
    __in PVOID Object
    );

#endif
