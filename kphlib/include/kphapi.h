/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *
 */

#pragma once

#ifdef _KERNEL_MODE
#define PHNT_MODE PHNT_MODE_KERNEL
#endif
#pragma warning(push)
#pragma warning(disable : 4201)

// Process

typedef ULONG KPH_PROCESS_STATE;
typedef KPH_PROCESS_STATE* PKPH_PROCESS_STATE;

#define KPH_PROCESS_SECURELY_CREATED                     0x00000001ul
#define KPH_PROCESS_VERIFIED_PROCESS                     0x00000002ul
#define KPH_PROCESS_PROTECTED_PROCESS                    0x00000004ul
#define KPH_PROCESS_NO_UNTRUSTED_IMAGES                  0x00000008ul
#define KPH_PROCESS_HAS_FILE_OBJECT                      0x00000010ul
#define KPH_PROCESS_HAS_SECTION_OBJECT_POINTERS          0x00000020ul
#define KPH_PROCESS_NO_USER_WRITABLE_REFERENCES          0x00000040ul
#define KPH_PROCESS_NO_FILE_TRANSACTION                  0x00000080ul
#define KPH_PROCESS_NOT_BEING_DEBUGGED                   0x00000100ul
#define KPH_PROCESS_NO_WRITABLE_FILE_OBJECT              0x00000200ul

#define KPH_PROCESS_STATE_MAXIMUM (KPH_PROCESS_SECURELY_CREATED               |\
                                   KPH_PROCESS_VERIFIED_PROCESS               |\
                                   KPH_PROCESS_PROTECTED_PROCESS              |\
                                   KPH_PROCESS_NO_UNTRUSTED_IMAGES            |\
                                   KPH_PROCESS_HAS_FILE_OBJECT                |\
                                   KPH_PROCESS_HAS_SECTION_OBJECT_POINTERS    |\
                                   KPH_PROCESS_NO_USER_WRITABLE_REFERENCES    |\
                                   KPH_PROCESS_NO_FILE_TRANSACTION            |\
                                   KPH_PROCESS_NOT_BEING_DEBUGGED             |\
                                   KPH_PROCESS_NO_WRITABLE_FILE_OBJECT)

#define KPH_PROCESS_STATE_HIGH    (KPH_PROCESS_VERIFIED_PROCESS               |\
                                   KPH_PROCESS_PROTECTED_PROCESS              |\
                                   KPH_PROCESS_NO_UNTRUSTED_IMAGES            |\
                                   KPH_PROCESS_HAS_FILE_OBJECT                |\
                                   KPH_PROCESS_HAS_SECTION_OBJECT_POINTERS    |\
                                   KPH_PROCESS_NO_USER_WRITABLE_REFERENCES    |\
                                   KPH_PROCESS_NO_FILE_TRANSACTION            |\
                                   KPH_PROCESS_NOT_BEING_DEBUGGED             |\
                                   KPH_PROCESS_NO_WRITABLE_FILE_OBJECT)

#define KPH_PROCESS_STATE_MEDIUM  (KPH_PROCESS_VERIFIED_PROCESS               |\
                                   KPH_PROCESS_PROTECTED_PROCESS              |\
                                   KPH_PROCESS_HAS_FILE_OBJECT                |\
                                   KPH_PROCESS_HAS_SECTION_OBJECT_POINTERS    |\
                                   KPH_PROCESS_NO_USER_WRITABLE_REFERENCES    |\
                                   KPH_PROCESS_NO_FILE_TRANSACTION            |\
                                   KPH_PROCESS_NO_WRITABLE_FILE_OBJECT)

#define KPH_PROCESS_STATE_LOW     (KPH_PROCESS_VERIFIED_PROCESS               |\
                                   KPH_PROCESS_HAS_FILE_OBJECT                |\
                                   KPH_PROCESS_HAS_SECTION_OBJECT_POINTERS    |\
                                   KPH_PROCESS_NO_USER_WRITABLE_REFERENCES    |\
                                   KPH_PROCESS_NO_FILE_TRANSACTION            |\
                                   KPH_PROCESS_NO_WRITABLE_FILE_OBJECT)

#define KPH_PROCESS_STATE_MINIMUM (KPH_PROCESS_HAS_FILE_OBJECT                |\
                                   KPH_PROCESS_HAS_SECTION_OBJECT_POINTERS    |\
                                   KPH_PROCESS_NO_USER_WRITABLE_REFERENCES    |\
                                   KPH_PROCESS_NO_FILE_TRANSACTION            |\
                                   KPH_PROCESS_NO_WRITABLE_FILE_OBJECT)

typedef enum _KPH_PROCESS_INFORMATION_CLASS
{
    KphProcessBasicInformation,      // q: KPH_PROCESS_BASIC_INFORMATION
    KphProcessStateInformation,      // q: KPH_PROCESS_STATE
    KphProcessQuotaLimits,           // s: QUOTA_LIMITS
    KphProcessBasePriority,          // s: KPRIORITY
    KphProcessRaisePriority,         // s: ULONG
    KphProcessPriorityClass,         // s: PROCESS_PRIORITY_CLASS
    KphProcessAffinityMask,          // s: KAFFINITY/GROUP_AFFINITY
    KphProcessPriorityBoost,         // s: ULONG
    KphProcessIoPriority,            // s: IO_PRIORITY_HINT
    KphProcessPagePriority,          // s: PAGE_PRIORITY_INFORMATION
    KphProcessPowerThrottlingState,  // s: POWER_THROTTLING_PROCESS_STATE
    KphProcessPriorityClassEx,       // s: PROCESS_PRIORITY_CLASS_EX
    KphProcessEmptyWorkingSet,       // s
    KphProcessWSLProcessId,          // q: ULONG
    KphProcessSequenceNumber,        // q: ULONG64
    KphProcessStartKey,              // q: ULONG64
    KphProcessImageSection,          // q: HANDLE
} KPH_PROCESS_INFORMATION_CLASS;

typedef enum _KPH_THREAD_INFORMATION_CLASS
{
    KphThreadPriority,               // s: KPRIORITY
    KphThreadBasePriority,           // s: KPRIORITY
    KphThreadAffinityMask,           // s: KAFFINITY
    KphThreadIdealProcessor,         // s: ULONG
    KphThreadPriorityBoost,          // s: ULONG
    KphThreadIoPriority,             // s: IO_PRIORITY_HINT
    KphThreadPagePriority,           // s: PAGE_PRIORITY_INFORMATION
    KphThreadActualBasePriority,     // s: LONG
    KphThreadGroupInformation,       // s: GROUP_AFFINITY
    KphThreadIdealProcessorEx,       // s: PROCESSOR_NUMBER
    KphThreadActualGroupAffinity,    // s: GROUP_AFFINITY
    KphThreadPowerThrottlingState,   // s: POWER_THROTTLING_THREAD_STATE
    KphThreadIoCounters,             // q: IO_COUNTERS
    KphThreadWSLThreadId,            // q: ULONG
    KphThreadExplicitCaseSensitivity,// s: ULONG; s: 0 disables, otherwise enables
    KphThreadKernelStackInformation, // q: KPH_KERNEL_STACK_INFORMATION
} KPH_THREAD_INFORMATION_CLASS;

typedef struct _KPH_PROCESS_BASIC_INFORMATION
{
    KPH_PROCESS_STATE ProcessState;

    ULONG64 ProcessStartKey;
    CLIENT_ID CreatorClientId;

    ULONG UserWritableReferences;

    SIZE_T NumberOfImageLoads;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG CreateNotification : 1;
            ULONG ExitNotification : 1;
            ULONG VerifiedProcess : 1;
            ULONG SecurelyCreated : 1;
            ULONG Protected : 1;
            ULONG IsWow64 : 1;
            ULONG IsSubsystemProcess : 1;
            ULONG AllocatedImageName : 1;
            ULONG Reserved : 24;
        };
    };

    SIZE_T NumberOfThreads;

    //
    // Only valid if Protected flag is set.
    //
    ACCESS_MASK ProcessAllowedMask;
    ACCESS_MASK ThreadAllowedMask;

    //
    // These are only tracked for verified processes.
    //
    SIZE_T NumberOfMicrosoftImageLoads;
    SIZE_T NumberOfAntimalwareImageLoads;
    SIZE_T NumberOfVerifiedImageLoads;
    SIZE_T NumberOfUntrustedImageLoads;
} KPH_PROCESS_BASIC_INFORMATION, *PKPH_PROCESS_BASIC_INFORMATION;

typedef struct _KPH_KERNEL_STACK_INFORMATION
{
    PVOID InitialStack;
    PVOID StackLimit;
    PVOID StackBase;
    PVOID KernelStack;
} KPH_KERNEL_STACK_INFORMATION, *PKPH_KERNEL_STACK_INFORMATION;

// Process handle information

typedef struct _KPH_PROCESS_HANDLE
{
    HANDLE Handle;
    PVOID Object;
    ACCESS_MASK GrantedAccess;
    USHORT ObjectTypeIndex;
    USHORT Reserved1;
    ULONG HandleAttributes;
    ULONG Reserved2;
} KPH_PROCESS_HANDLE, *PKPH_PROCESS_HANDLE;

typedef struct _KPH_PROCESS_HANDLE_INFORMATION
{
    ULONG HandleCount;
    _Field_size_(HandleCount) KPH_PROCESS_HANDLE Handles[1];
} KPH_PROCESS_HANDLE_INFORMATION, *PKPH_PROCESS_HANDLE_INFORMATION;

// Object information

typedef enum _KPH_OBJECT_INFORMATION_CLASS
{
    KphObjectBasicInformation,                // q: OBJECT_BASIC_INFORMATION
    KphObjectNameInformation,                 // q: OBJECT_NAME_INFORMATION
    KphObjectTypeInformation,                 // q: OBJECT_TYPE_INFORMATION
    KphObjectHandleFlagInformation,           // qs: OBJECT_HANDLE_FLAG_INFORMATION
    KphObjectProcessBasicInformation,         // q: PROCESS_BASIC_INFORMATION
    KphObjectThreadBasicInformation,          // q: THREAD_BASIC_INFORMATION
    KphObjectEtwRegBasicInformation,          // q: ETWREG_BASIC_INFORMATION
    KphObjectFileObjectInformation,           // q: KPH_FILE_OBJECT_INFORMATION
    KphObjectFileObjectDriver,                // q: KPH_FILE_OBJECT_DRIVER
    KphObjectProcessTimes,                    // q: KERNEL_USER_TIMES
    KphObjectThreadTimes,                     // q: KERNEL_USER_TIMES
    KphObjectProcessImageFileName,            // q: UNICODE_STRING
    KphObjectThreadNameInformation,           // q: THREAD_NAME_INFORMATION
    KphObjectThreadIsTerminated,              // q: ULONG
    KphObjectSectionBasicInformation,         // q: SECTION_BASIC_INFORMATION
    KphObjectSectionFileName,                 // q: UNICODE_STRING
    KphObjectSectionImageInformation,         // q; SECTION_IMAGE_INFORMATION
    KphObjectSectionRelocationInformation,    // q; PVOID RelocationAddress
    KphObjectSectionOriginalBaseInformation,  // q: PVOID BaseAddress
    KphObjectSectionInternalImageInformation, // q: SECTION_INTERNAL_IMAGE_INFORMATION
    KphObjectSectionMappingsInformation,      // q: KPH_SECTION_MAPPINGS_INFORMATION
    KphObjectAttributesInformation,           // q: KPH_OBJECT_ATTRIBUTES_INFORMATION
    MaxKphObjectInfoClass
} KPH_OBJECT_INFORMATION_CLASS;

typedef struct _KPH_VPB
{
    CSHORT Type;
    CSHORT Size;
    USHORT Flags;
    USHORT VolumeLabelLength;
    ULONG SerialNumber;
    ULONG ReferenceCount;
    WCHAR VolumeLabel[32];
} KPH_VPB, *PKPH_VPB;

typedef struct _KPH_DEVICE_INFO
{
    DEVICE_TYPE Type;
    ULONG Characteristics;
    ULONG Flags;
    KPH_VPB Vpb;
} KPH_DEVICE_INFO, *PKPH_DEVICE_INFO;

typedef struct _KPH_FILE_OBJECT_INFORMATION
{
    BOOLEAN LockOperation;
    BOOLEAN DeletePending;
    BOOLEAN ReadAccess;
    BOOLEAN WriteAccess;
    BOOLEAN DeleteAccess;
    BOOLEAN SharedRead;
    BOOLEAN SharedWrite;
    BOOLEAN SharedDelete;
    LARGE_INTEGER CurrentByteOffset;
    ULONG Flags;
    ULONG UserWritableReferences;
    BOOLEAN HasActiveTransaction;
    BOOLEAN IsIgnoringSharing;
    LONG Waiters;
    LONG Busy;
    KPH_VPB Vpb;
    KPH_DEVICE_INFO Device;
    KPH_DEVICE_INFO AttachedDevice;
    KPH_DEVICE_INFO RelatedDevice;
} KPH_FILE_OBJECT_INFORMATION, *PKPH_FILE_OBJECT_INFORMATION;

typedef struct _KPH_FILE_OBJECT_DRIVER
{
    HANDLE DriverHandle;
} KPH_FILE_OBJECT_DRIVER, *PKPH_FILE_OBJECT_DRIVER;

typedef struct _KPH_OBJECT_ATTRIBUTES_INFORMATION
{
    union
    {
        UCHAR Flags;
        struct
        {
            UCHAR NewObject : 1;
            UCHAR KernelObject : 1;
            UCHAR KernelOnlyAccess : 1;
            UCHAR ExclusiveObject : 1;
            UCHAR PermanentObject : 1;
            UCHAR DefaultSecurityQuota : 1;
            UCHAR SingleHandleEntry : 1;
            UCHAR DeletedInline : 1;
        };
    };
} KPH_OBJECT_ATTRIBUTES_INFORMATION, *PKPH_OBJECT_ATTRIBUTES_INFORMATION;

// Driver information

typedef enum _KPH_DRIVER_INFORMATION_CLASS
{
    KphDriverBasicInformation,          // q: DRIVER_BASIC_INFORMATION
    KphDriverNameInformation,           // q: UNICODE_STRING
    KphDriverServiceKeyNameInformation, // q: UNICODE_STRING
    KphDriverImageFileNameInformation,  // q: UNICODE_STRING
    MaxKphDriverInfoClass
} KPH_DRIVER_INFORMATION_CLASS;

typedef struct _KPH_DRIVER_BASIC_INFORMATION
{
    ULONG Flags;
    PVOID DriverStart;
    ULONG DriverSize;
} KPH_DRIVER_BASIC_INFORMATION, *PKPH_DRIVER_BASIC_INFORMATION;

typedef struct _KPH_DRIVER_NAME_INFORMATION
{
    UNICODE_STRING DriverName;
} KPH_DRIVER_NAME_INFORMATION, *PKPH_DRIVER_NAME_INFORMATION;

typedef struct _KPH_DRIVER_SERVICE_KEY_NAME_INFORMATION
{
    UNICODE_STRING ServiceKeyName;
} KPH_DRIVER_SERVICE_KEY_NAME_INFORMATION, *PKPH_DRIVER_SERVICE_KEY_NAME_INFORMATION;

// ETW registration object information

typedef struct _KPH_ETWREG_BASIC_INFORMATION
{
    GUID Guid;
    ULONG_PTR SessionId;
} KPH_ETWREG_BASIC_INFORMATION, *PKPH_ETWREG_BASIC_INFORMATION;

// ALPC object information

typedef enum _KPH_ALPC_INFORMATION_CLASS
{
    KphAlpcBasicInformation,               // q: KPH_ALPC_BASIC_INFORMATION
    KphAlpcCommunicationInformation,       // q: KPH_ALPC_COMMUNICATION_INFORMATION
    KphAlpcCommunicationNamesInformation,  // q: KPH_ALPC_COMMUNICATION_NAMES_INFORMATION
} KPH_ALPC_INFORMATION_CLASS;

typedef struct _KPH_ALPC_BASIC_INFORMATION
{
    HANDLE OwnerProcessId;
    ULONG Flags;
    LONG SequenceNo;
    PVOID PortContext;
    union
    {
        ULONG State;
        struct
        {
            ULONG Initialized : 1;
            ULONG Type : 2;
            ULONG ConnectionPending : 1;
            ULONG ConnectionRefused : 1;
            ULONG Disconnected : 1;
            ULONG Closed : 1;
            ULONG NoFlushOnClose : 1;
            ULONG ReturnExtendedInfo : 1;
            ULONG Waitable : 1;
            ULONG DynamicSecurity : 1;
            ULONG Wow64CompletionList : 1;
            ULONG Lpc : 1;
            ULONG LpcToLpc : 1;
            ULONG HasCompletionList : 1;
            ULONG HadCompletionList : 1;
            ULONG EnableCompletionList : 1;
        };
    };
} KPH_ALPC_BASIC_INFORMATION, *PKPH_ALPC_BASIC_INFORMATION;

typedef struct _KPH_ALPC_COMMUNICATION_INFORMATION
{
    KPH_ALPC_BASIC_INFORMATION ConnectionPort;
    KPH_ALPC_BASIC_INFORMATION ServerCommunicationPort;
    KPH_ALPC_BASIC_INFORMATION ClientCommunicationPort;
} KPH_ALPC_COMMUNICATION_INFORMATION, *PKPH_ALPC_COMMUNICATION_INFORMATION;

typedef struct _KPH_ALPC_COMMUNICATION_NAMES_INFORMATION
{
    UNICODE_STRING ConnectionPort;
    UNICODE_STRING ServerCommunicationPort;
    UNICODE_STRING ClientCommunicationPort;
} KPH_ALPC_COMMUNICATION_NAMES_INFORMATION, *PKPH_ALPC_COMMUNICATION_NAMES_INFORMATION;

// System control

typedef enum _KPH_SYSTEM_CONTROL_CLASS
{
    KphSystemControlEmptyCompressionStore
} KPH_SYSTEM_CONTROL_CLASS;

// Section

typedef enum _KPH_SECTION_INFORMATION_CLASS
{
    KphSectionMappingsInformation, // q: KPH_SECTION_MAPPINGS_INFORMATION
} KPH_SECTION_INFORMATION_CLASS;

#define VIEW_MAP_TYPE_PROCESS         1
#define VIEW_MAP_TYPE_SESSION         2
#define VIEW_MAP_TYPE_SYSTEM_CACHE    3

typedef struct _KPH_SECTION_MAP_ENTRY
{
    UCHAR ViewMapType;
    HANDLE ProcessId;
    PVOID StartVa;
    PVOID EndVa;
} KPH_SECTION_MAP_ENTRY, *PKPH_SECTION_MAP_ENTRY;

typedef struct _KPH_SECTION_MAPPINGS_INFORMATION
{
    ULONG NumberOfMappings;
    KPH_SECTION_MAP_ENTRY Mappings[ANYSIZE_ARRAY];
} KPH_SECTION_MAPPINGS_INFORMATION, *PKPH_SECTION_MAPPINGS_INFORMATION;

// Virtual memory

typedef enum _KPH_MEMORY_INFORMATION_CLASS
{
    KphMemoryImageSection,          // q: HANDLE
    KphMemoryDataSection,           // q: KPH_MEMORY_DATA_SECTION
    KphMemoryMappedInformation,     // q: KPH_MEMORY_MAPPED_INFORMATION
} KPH_MEMORY_INFORMATION_CLASS, *PKPH_MEMORY_INFORMATION_CLASS;

typedef struct _KPH_MEMORY_DATA_SECTION
{
    HANDLE SectionHandle;
    LARGE_INTEGER SectionFileSize;
} KPH_MEMORY_DATA_SECTION, *PKPH_MEMORY_DATA_SECTION;

typedef struct _KPH_MEMORY_MAPPED_INFORMATION
{
    PVOID FileObject;
    PVOID SectionObjectPointers;
    PVOID DataControlArea;
    PVOID SharedCacheMap;
    PVOID ImageControlArea;
    ULONG UserWritableReferences;
} KPH_MEMORY_MAPPED_INFORMATION, *PKPH_MEMORY_MAPPED_INFORMATION;

// File

typedef enum _KPH_HASH_ALGORITHM
{
    KphHashAlgorithmMd5,
    KphHashAlgorithmSha1,
    KphHashAlgorithmSha1Authenticode,
    KphHashAlgorithmSha256,
    KphHashAlgorithmSha256Authenticode,
    KphHashAlgorithmSha384,
    KphHashAlgorithmSha512,
    MaxKphHashAlgorithm,
} KPH_HASH_ALGORITHM, *PKPH_HASH_ALGORITHM;

#define KPH_HASH_ALGORITHM_MAX_LENGTH (512 / 8)

typedef struct _KPH_HASH_INFORMATION
{
    KPH_HASH_ALGORITHM Algorithm;
    ULONG Length;
    BYTE Hash[KPH_HASH_ALGORITHM_MAX_LENGTH];
} KPH_HASH_INFORMATION, *PKPH_HASH_INFORMATION;

// Verification

#define KPH_PROCESS_READ_ACCESS   (STANDARD_RIGHTS_READ                       |\
                                   SYNCHRONIZE                                |\
                                   PROCESS_QUERY_INFORMATION                  |\
                                   PROCESS_QUERY_LIMITED_INFORMATION          |\
                                   PROCESS_VM_READ)

#define KPH_THREAD_READ_ACCESS    (STANDARD_RIGHTS_READ                       |\
                                   SYNCHRONIZE                                |\
                                   THREAD_QUERY_INFORMATION                   |\
                                   THREAD_QUERY_LIMITED_INFORMATION           |\
                                   THREAD_GET_CONTEXT)

#define KPH_TOKEN_READ_ACCESS     (STANDARD_RIGHTS_READ                       |\
                                   SYNCHRONIZE                                |\
                                   TOKEN_QUERY                                |\
                                   TOKEN_QUERY_SOURCE)

#define KPH_JOB_READ_ACCESS       (STANDARD_RIGHTS_READ                       |\
                                   SYNCHRONIZE                                |\
                                   JOB_OBJECT_QUERY)

#define KPH_FILE_READ_ACCESS      (STANDARD_RIGHTS_READ                       |\
                                   SYNCHRONIZE                                |\
                                   FILE_READ_DATA                             |\
                                   FILE_READ_ATTRIBUTES                       |\
                                   FILE_READ_EA)

#define KPH_FILE_READ_DISPOSITION (FILE_OPEN)

#define KPH_SECTION_READ_ACCESS   (SECTION_MAP_READ | SECTION_QUERY)

// Informer

typedef struct _KPH_INFORMER_SETTINGS
{
    union
    {
        volatile ULONG64 Flags;
        struct
        {
            ULONG64 ProcessCreate : 1;
            ULONG64 ProcessExit : 1;
            ULONG64 ThreadCreate : 1;
            ULONG64 ThreadExecute : 1;
            ULONG64 ThreadExit : 1;
            ULONG64 ImageLoad : 1;
            ULONG64 DebugPrint : 1;
            ULONG64 HandlePreCreateProcess : 1;
            ULONG64 HandlePostCreateProcess : 1;
            ULONG64 HandlePreDuplicateProcess : 1;
            ULONG64 HandlePostDuplicateProcess : 1;
            ULONG64 HandlePreCreateThread : 1;
            ULONG64 HandlePostCreateThread : 1;
            ULONG64 HandlePreDuplicateThread : 1;
            ULONG64 HandlePostDuplicateThread : 1;
            ULONG64 HandlePreCreateDesktop : 1;
            ULONG64 HandlePostCreateDesktop : 1;
            ULONG64 HandlePreDuplicateDesktop : 1;
            ULONG64 HandlePostDuplicateDesktop : 1;
            ULONG64 EnableStackTraces : 1;
            ULONG64 EnableProcessCreateReply : 1;
            ULONG64 FileEnablePreCreateReply : 1;
            ULONG64 FileEnablePostCreateReply : 1;
            ULONG64 FileEnablePostFileNames : 1;
            ULONG64 FileEnablePagingIo : 1;
            ULONG64 FileEnableSyncPagingIo : 1;
            ULONG64 FileEnableIoControlBuffers : 1;
            ULONG64 FileEnableFsControlBuffers : 1;
            ULONG64 FileEnableDirControlBuffers : 1;
            ULONG64 FilePreCreate : 1;
            ULONG64 FilePostCreate : 1;
            ULONG64 FilePreCreateNamedPipe : 1;
            ULONG64 FilePostCreateNamedPipe : 1;
            ULONG64 FilePreClose : 1;
            ULONG64 FilePostClose : 1;
            ULONG64 FilePreRead : 1;
            ULONG64 FilePostRead : 1;
            ULONG64 FilePreWrite : 1;
            ULONG64 FilePostWrite : 1;
            ULONG64 FilePreQueryInformation : 1;
            ULONG64 FilePostQueryInformation : 1;
            ULONG64 FilePreSetInformation : 1;
            ULONG64 FilePostSetInformation : 1;
            ULONG64 FilePreQueryEa : 1;
            ULONG64 FilePostQueryEa : 1;
            ULONG64 FilePreSetEa : 1;
            ULONG64 FilePostSetEa : 1;
            ULONG64 FilePreFlushBuffers : 1;
            ULONG64 FilePostFlushBuffers : 1;
            ULONG64 FilePreQueryVolumeInformation : 1;
            ULONG64 FilePostQueryVolumeInformation : 1;
            ULONG64 FilePreSetVolumeInformation : 1;
            ULONG64 FilePostSetVolumeInformation : 1;
            ULONG64 FilePreDirectoryControl : 1;
            ULONG64 FilePostDirectoryControl : 1;
            ULONG64 FilePreFileSystemControl : 1;
            ULONG64 FilePostFileSystemControl : 1;
            ULONG64 FilePreDeviceControl : 1;
            ULONG64 FilePostDeviceControl : 1;
            ULONG64 FilePreInternalDeviceControl : 1;
            ULONG64 FilePostInternalDeviceControl : 1;
            ULONG64 FilePreShutdown : 1;
            ULONG64 FilePostShutdown : 1;
            ULONG64 FilePreLockControl : 1;
        };
    };
    union
    {
        volatile ULONG64 Flags2;
        struct
        {
            ULONG64 FilePostLockControl : 1;
            ULONG64 FilePreCleanup : 1;
            ULONG64 FilePostCleanup : 1;
            ULONG64 FilePreCreateMailslot : 1;
            ULONG64 FilePostCreateMailslot : 1;
            ULONG64 FilePreQuerySecurity : 1;
            ULONG64 FilePostQuerySecurity : 1;
            ULONG64 FilePreSetSecurity : 1;
            ULONG64 FilePostSetSecurity : 1;
            ULONG64 FilePrePower : 1;
            ULONG64 FilePostPower : 1;
            ULONG64 FilePreSystemControl : 1;
            ULONG64 FilePostSystemControl : 1;
            ULONG64 FilePreDeviceChange : 1;
            ULONG64 FilePostDeviceChange : 1;
            ULONG64 FilePreQueryQuota : 1;
            ULONG64 FilePostQueryQuota : 1;
            ULONG64 FilePreSetQuota : 1;
            ULONG64 FilePostSetQuota : 1;
            ULONG64 FilePrePnp : 1;
            ULONG64 FilePostPnp : 1;
            ULONG64 FilePreAcquireForSectionSync : 1;
            ULONG64 FilePostAcquireForSectionSync : 1;
            ULONG64 FilePreReleaseForSectionSync : 1;
            ULONG64 FilePostReleaseForSectionSync : 1;
            ULONG64 FilePreAcquireForModWrite : 1;
            ULONG64 FilePostAcquireForModWrite : 1;
            ULONG64 FilePreReleaseForModWrite : 1;
            ULONG64 FilePostReleaseForModWrite : 1;
            ULONG64 FilePreAcquireForCcFlush : 1;
            ULONG64 FilePostAcquireForCcFlush : 1;
            ULONG64 FilePreReleaseForCcFlush : 1;
            ULONG64 FilePostReleaseForCcFlush : 1;
            ULONG64 FilePreQueryOpen : 1;
            ULONG64 FilePostQueryOpen : 1;
            ULONG64 FilePreFastIoCheckIfPossible : 1;
            ULONG64 FilePostFastIoCheckIfPossible : 1;
            ULONG64 FilePreNetworkQueryOpen : 1;
            ULONG64 FilePostNetworkQueryOpen : 1;
            ULONG64 FilePreMdlRead : 1;
            ULONG64 FilePostMdlRead : 1;
            ULONG64 FilePreMdlReadComplete : 1;
            ULONG64 FilePostMdlReadComplete : 1;
            ULONG64 FilePrePrepareMdlWrite : 1;
            ULONG64 FilePostPrepareMdlWrite : 1;
            ULONG64 FilePreMdlWriteComplete : 1;
            ULONG64 FilePostMdlWriteComplete : 1;
            ULONG64 FilePreVolumeMount : 1;
            ULONG64 FilePostVolumeMount : 1;
            ULONG64 FilePreVolumeDismount : 1;
            ULONG64 FilePostVolumeDismount : 1;
            ULONG64 RegEnablePostObjectNames : 1;
            ULONG64 RegEnablePostValueNames : 1;
            ULONG64 RegEnableValueBuffers : 1;
            ULONG64 RegPreDeleteKey : 1;
            ULONG64 RegPostDeleteKey : 1;
            ULONG64 RegPreSetValueKey : 1;
            ULONG64 RegPostSetValueKey : 1;
            ULONG64 RegPreDeleteValueKey : 1;
            ULONG64 RegPostDeleteValueKey : 1;
            ULONG64 RegPreSetInformationKey : 1;
            ULONG64 RegPostSetInformationKey : 1;
            ULONG64 RegPreRenameKey : 1;
            ULONG64 RegPostRenameKey : 1;
        };
    };
    union
    {
        volatile ULONG64 Flags3;
        struct
        {
            ULONG64 RegPreEnumerateKey : 1;
            ULONG64 RegPostEnumerateKey : 1;
            ULONG64 RegPreEnumerateValueKey : 1;
            ULONG64 RegPostEnumerateValueKey : 1;
            ULONG64 RegPreQueryKey : 1;
            ULONG64 RegPostQueryKey : 1;
            ULONG64 RegPreQueryValueKey : 1;
            ULONG64 RegPostQueryValueKey : 1;
            ULONG64 RegPreQueryMultipleValueKey : 1;
            ULONG64 RegPostQueryMultipleValueKey : 1;
            ULONG64 RegPreKeyHandleClose : 1;
            ULONG64 RegPostKeyHandleClose : 1;
            ULONG64 RegPreCreateKey : 1;
            ULONG64 RegPostCreateKey : 1;
            ULONG64 RegPreOpenKey : 1;
            ULONG64 RegPostOpenKey : 1;
            ULONG64 RegPreFlushKey : 1;
            ULONG64 RegPostFlushKey : 1;
            ULONG64 RegPreLoadKey : 1;
            ULONG64 RegPostLoadKey : 1;
            ULONG64 RegPreUnLoadKey : 1;
            ULONG64 RegPostUnLoadKey : 1;
            ULONG64 RegPreQueryKeySecurity : 1;
            ULONG64 RegPostQueryKeySecurity : 1;
            ULONG64 RegPreSetKeySecurity : 1;
            ULONG64 RegPostSetKeySecurity : 1;
            ULONG64 RegPreRestoreKey : 1;
            ULONG64 RegPostRestoreKey : 1;
            ULONG64 RegPreSaveKey : 1;
            ULONG64 RegPostSaveKey : 1;
            ULONG64 RegPreReplaceKey : 1;
            ULONG64 RegPostReplaceKey : 1;
            ULONG64 RegPreQueryKeyName : 1;
            ULONG64 RegPostQueryKeyName : 1;
            ULONG64 RegPreSaveMergedKey : 1;
            ULONG64 RegPostSaveMergedKey : 1;
            ULONG64 ImageVerify : 1;
            ULONG64 Reserved : 27;
        };
    };
} KPH_INFORMER_SETTINGS, *PKPH_INFORMER_SETTINGS;
typedef const KPH_INFORMER_SETTINGS* PCKPH_INFORMER_SETTINGS;

typedef struct _KPH_MESSAGE_TIMEOUTS
{
    LARGE_INTEGER AsyncTimeout;
    LARGE_INTEGER DefaultTimeout;
    LARGE_INTEGER ProcessCreateTimeout;
    LARGE_INTEGER FilePreCreateTimeout;
    LARGE_INTEGER FilePostCreateTimeout;
} KPH_MESSAGE_TIMEOUTS, *PKPH_MESSAGE_TIMEOUTS;

// Parameters

typedef union _KPH_PARAMETER_FLAGS
{
    ULONG Flags;
    struct
    {
        ULONG DisableImageLoadProtection : 1;
        ULONG RandomizedPoolTag : 1;
        ULONG DynDataNoEmbedded : 1;
        ULONG Reserved : 29;
    };
} KPH_PARAMETER_FLAGS, *PKPH_PARAMETER_FLAGS;

// Session Token

#define KPH_TOKEN_PRIVILEGE_TERMINATE 0x00000001ul
#define KPH_TOKEN_VALID_PRIVILEGES    (KPH_TOKEN_PRIVILEGE_TERMINATE)

#include <pshpack1.h>
typedef struct _KPH_SESSION_ACCESS_TOKEN
{
    LARGE_INTEGER Expiry;
    ULONG Privileges;
    LONG Uses;
    GUID Identifier;
    BYTE Material[32];
} KPH_SESSION_ACCESS_TOKEN, *PKPH_SESSION_ACCESS_TOKEN;
C_ASSERT(sizeof(KPH_SESSION_ACCESS_TOKEN) == 64);
#include <poppack.h>

#pragma warning(pop)
