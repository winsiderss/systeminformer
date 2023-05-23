/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     dmex    2016-2023
 *
 */

#ifndef PH_PROCPRV_H
#define PH_PROCPRV_H

#define PH_RECORD_MAX_USAGE

extern PPH_OBJECT_TYPE PhProcessItemType;
extern PPH_LIST PhProcessRecordList;
extern PH_QUEUED_LOCK PhProcessRecordListLock;

extern ULONG PhStatisticsSampleCount;
extern BOOLEAN PhEnablePurgeProcessRecords;
extern BOOLEAN PhEnableCycleCpuUsage;
extern BOOLEAN PhEnablePackageIconSupport;

typedef enum _PH_PROCESS_PROVIDER_FLAG
{
    PH_PROCESS_PROVIDER_FLAG_WSCOUNTERS = 0x1,
    PH_PROCESS_PROVIDER_FLAG_GDIUSERHANDLES = 0x2,
    PH_PROCESS_PROVIDER_FLAG_IOPAGEPRIORITY = 0x4,
    PH_PROCESS_PROVIDER_FLAG_WINDOW = 0x8,
    PH_PROCESS_PROVIDER_FLAG_DEPSTATUS = 0x10,
    PH_PROCESS_PROVIDER_FLAG_TOKEN = 0x20,
    PH_PROCESS_PROVIDER_FLAG_OSCONTEXT = 0x40,
    PH_PROCESS_PROVIDER_FLAG_QUOTALIMITS = 0x80,
    PH_PROCESS_PROVIDER_FLAG_IMAGE = 0x100,
    PH_PROCESS_PROVIDER_FLAG_APPID = 0x200,
    PH_PROCESS_PROVIDER_FLAG_DPIAWARENESS = 0x400,
    PH_PROCESS_PROVIDER_FLAG_FILEATTRIBUTES = 0x800,
    PH_PROCESS_PROVIDER_FLAG_DESKTOPINFO = 0x1000,
    PH_PROCESS_PROVIDER_FLAG_USERNAME = 0x2000,
    PH_PROCESS_PROVIDER_FLAG_CRITICAL = 0x4000,
    PH_PROCESS_PROVIDER_FLAG_ERRORMODE = 0x8000,
    PH_PROCESS_PROVIDER_FLAG_CODEPAGE = 0x10000,
    PH_PROCESS_PROVIDER_FLAG_POWERTHROTTLING = 0x20000,
    PH_PROCESS_PROVIDER_FLAG_PRIORITYBOOST = 0x40000,
    PH_PROCESS_PROVIDER_FLAG_CFGUARD = 0x80000,
    PH_PROCESS_PROVIDER_FLAG_CET = 0x100000,
    PH_PROCESS_PROVIDER_FLAG_AVERAGE = 0x200000,
} PH_PROCESS_PROVIDER_FLAG;

extern ULONG PhProcessProviderFlagsMask;

extern PVOID PhProcessInformation; // only can be used if running on same thread as process provider
extern ULONG PhProcessInformationSequenceNumber;
extern SYSTEM_PERFORMANCE_INFORMATION PhPerfInformation;
extern PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PhCpuInformation;
extern SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PhCpuTotals;
extern ULONG PhTotalProcesses;
extern ULONG PhTotalThreads;
extern ULONG PhTotalHandles;
extern ULONG PhTotalCpuQueueLength;

extern ULONG64 PhCpuTotalCycleDelta;
extern PSYSTEM_PROCESSOR_IDLE_CYCLE_TIME_INFORMATION PhCpuIdleCycleTime; // cycle time for Idle
extern PSYSTEM_PROCESSOR_CYCLE_TIME_INFORMATION PhCpuSystemCycleTime; // cycle time for DPCs and Interrupts
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

typedef struct _PH_IMAGELIST_ITEM
{
    PPH_STRING FileName;
    ULONG LargeIconIndex;
    ULONG SmallIconIndex;
} PH_IMAGELIST_ITEM, *PPH_IMAGELIST_ITEM;

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

    PPH_STRING FileNameWin32;
    PPH_STRING FileName;
    PPH_STRING CommandLine;

    // File

    ULONG_PTR SmallIconIndex;
    ULONG_PTR LargeIconIndex;
    PH_IMAGE_VERSION_INFO VersionInfo;

    // Security

    PSID Sid;
    TOKEN_ELEVATION_TYPE ElevationType;
    MANDATORY_LEVEL IntegrityLevel;
    PWSTR IntegrityString;

    // Other

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
            ULONG IsHandleValid : 1;
            ULONG IsSuspended : 1;
            ULONG IsWow64 : 1;
            ULONG IsImmersive : 1;
            ULONG IsPartiallySuspended : 1;
            ULONG IsProtectedHandle : 1;
            ULONG IsProtectedProcess : 1;
            ULONG IsSecureProcess : 1;
            ULONG IsSubsystemProcess : 1;
            ULONG IsPackagedProcess : 1;
            ULONG IsUIAccessEnabled : 1;
            ULONG IsControlFlowGuardEnabled : 1;
            ULONG IsCetEnabled : 1;
            ULONG IsXfgEnabled : 1;
            ULONG IsXfgAuditEnabled : 1;
            ULONG Spare : 10;
        };
    };

    // Misc.

    volatile LONG JustProcessed;
    PH_EVENT Stage1Event;

    PPH_POINTER_LIST ServiceList;
    PH_QUEUED_LOCK ServiceListLock;

    WCHAR ProcessIdString[PH_INT32_STR_LEN_1];
    //WCHAR ParentProcessIdString[PH_INT32_STR_LEN_1];
    //WCHAR SessionIdString[PH_INT32_STR_LEN_1];
    PPH_STRING AlternateProcessIdString;

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
    FLOAT CpuAverageUsage;

    PH_UINT64_DELTA CpuKernelDelta;
    PH_UINT64_DELTA CpuUserDelta;
    PH_UINT64_DELTA IoReadDelta;
    PH_UINT64_DELTA IoWriteDelta;
    PH_UINT64_DELTA IoOtherDelta;
    PH_UINT64_DELTA IoReadCountDelta;
    PH_UINT64_DELTA IoWriteCountDelta;
    PH_UINT64_DELTA IoOtherCountDelta;
    PH_UINT64_DELTA ContextSwitchesDelta;
    PH_UINT32_DELTA PageFaultsDelta;
    PH_UINT32_DELTA HardFaultsDelta;
    PH_UINT64_DELTA CycleTimeDelta; // since WIN7

    VM_COUNTERS_EX VmCounters;
    IO_COUNTERS IoCounters;
    SIZE_T WorkingSetPrivateSize; // since VISTA
    ULONG PeakNumberOfThreads; // since WIN7
    ULONG HardFaultCount; // since WIN7

    ULONG TimeSequenceNumber;
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
    PPH_STRING UserName;

    PROCESS_DISK_COUNTERS DiskCounters;
    ULONGLONG ContextSwitches;

    ULONGLONG ProcessSequenceNumber;
    PH_KNOWN_PROCESS_TYPE KnownProcessType;
    PS_PROTECTION Protection;
    ULONG JobObjectId;
    SIZE_T SharedCommitCharge;

    PPH_IMAGELIST_ITEM IconEntry;

    NTSTATUS ImageCoherencyStatus;
    FLOAT ImageCoherency;

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
    ULONGLONG ProcessSequenceNumber;
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
// end_phapppub

PPH_PROCESS_ITEM PhCreateProcessItem(
    _In_ HANDLE ProcessId
    );

// begin_phapppub
PHAPPAPI
PPH_PROCESS_ITEM
NTAPI
PhReferenceProcessItem(
    _In_opt_ HANDLE ProcessId
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
    _In_opt_ PPH_STRING PackageFullName,
    _Out_opt_ PPH_STRING *SignerName
    );

VERIFY_RESULT PhVerifyFileCached(
    _In_ PPH_STRING FileName,
    _In_opt_ PPH_STRING PackageFullName,
    _Out_opt_ PPH_STRING *SignerName,
    _In_ BOOLEAN NativeFileName,
    _In_ BOOLEAN CachedOnly
    );

VOID PhFlushVerifyCache(
    VOID
    );

// begin_phapppub
PHAPPAPI
_Success_(return)
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
    VOID
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
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

PHAPPAPI
PPH_PROCESS_ITEM
NTAPI
PhReferenceProcessItemForRecord(
    _In_ PPH_PROCESS_RECORD Record
    );
// end_phapppub

extern PPH_OBJECT_TYPE PhImageListItemType;
extern HIMAGELIST PhProcessLargeImageList;
extern HIMAGELIST PhProcessSmallImageList;

// begin_phapppub
PHAPPAPI
VOID
NTAPI
PhProcessImageListInitialization(
    _In_ HWND WindowHandle,
    _In_ LONG WindowDpi
    );

PHAPPAPI
PPH_IMAGELIST_ITEM
NTAPI
PhImageListExtractIcon(
    _In_ PPH_STRING FileName,
    _In_ BOOLEAN NativeFileName,
    _In_opt_ HANDLE ProcessId,
    _In_opt_ PPH_STRING PackageFullName,
    _In_ LONG SystemDpi
    );

PHAPPAPI
VOID
NTAPI
PhImageListFlushCache(
    VOID
    );

PHAPPAPI
HICON
NTAPI
PhGetImageListIcon(
    _In_ ULONG_PTR Index,
    _In_ BOOLEAN Large
    );

PHAPPAPI
HIMAGELIST
NTAPI
PhGetProcessSmallImageList(
    VOID
    );
// end_phapppub

#endif
