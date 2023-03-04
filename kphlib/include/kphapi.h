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

#define KPH_PROTECTION_SUPPRESSED 0

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

#if KPH_PROTECTION_SUPPRESSED

#define KPH_PROCESS_STATE_MAXIMUM (KPH_PROCESS_VERIFIED_PROCESS             |\
                                   KPH_PROCESS_NO_UNTRUSTED_IMAGES          |\
                                   KPH_PROCESS_HAS_FILE_OBJECT              |\
                                   KPH_PROCESS_HAS_SECTION_OBJECT_POINTERS  |\
                                   KPH_PROCESS_NO_USER_WRITABLE_REFERENCES  |\
                                   KPH_PROCESS_NO_FILE_TRANSACTION)

#define KPH_PROCESS_STATE_HIGH    (KPH_PROCESS_VERIFIED_PROCESS             |\
                                   KPH_PROCESS_NO_UNTRUSTED_IMAGES          |\
                                   KPH_PROCESS_HAS_FILE_OBJECT              |\
                                   KPH_PROCESS_HAS_SECTION_OBJECT_POINTERS  |\
                                   KPH_PROCESS_NO_USER_WRITABLE_REFERENCES  |\
                                   KPH_PROCESS_NO_FILE_TRANSACTION)

#define KPH_PROCESS_STATE_MEDIUM  (KPH_PROCESS_VERIFIED_PROCESS             |\
                                   KPH_PROCESS_HAS_FILE_OBJECT              |\
                                   KPH_PROCESS_HAS_SECTION_OBJECT_POINTERS  |\
                                   KPH_PROCESS_NO_USER_WRITABLE_REFERENCES  |\
                                   KPH_PROCESS_NO_FILE_TRANSACTION)

#else

#define KPH_PROCESS_STATE_MAXIMUM (KPH_PROCESS_SECURELY_CREATED             |\
                                   KPH_PROCESS_VERIFIED_PROCESS             |\
                                   KPH_PROCESS_PROTECTED_PROCESS            |\
                                   KPH_PROCESS_NO_UNTRUSTED_IMAGES          |\
                                   KPH_PROCESS_HAS_FILE_OBJECT              |\
                                   KPH_PROCESS_HAS_SECTION_OBJECT_POINTERS  |\
                                   KPH_PROCESS_NO_USER_WRITABLE_REFERENCES  |\
                                   KPH_PROCESS_NO_FILE_TRANSACTION          |\
                                   KPH_PROCESS_NOT_BEING_DEBUGGED)

#define KPH_PROCESS_STATE_HIGH    (KPH_PROCESS_VERIFIED_PROCESS             |\
                                   KPH_PROCESS_PROTECTED_PROCESS            |\
                                   KPH_PROCESS_NO_UNTRUSTED_IMAGES          |\
                                   KPH_PROCESS_HAS_FILE_OBJECT              |\
                                   KPH_PROCESS_HAS_SECTION_OBJECT_POINTERS  |\
                                   KPH_PROCESS_NO_USER_WRITABLE_REFERENCES  |\
                                   KPH_PROCESS_NO_FILE_TRANSACTION          |\
                                   KPH_PROCESS_NOT_BEING_DEBUGGED)

#define KPH_PROCESS_STATE_MEDIUM  (KPH_PROCESS_VERIFIED_PROCESS             |\
                                   KPH_PROCESS_PROTECTED_PROCESS            |\
                                   KPH_PROCESS_HAS_FILE_OBJECT              |\
                                   KPH_PROCESS_HAS_SECTION_OBJECT_POINTERS  |\
                                   KPH_PROCESS_NO_USER_WRITABLE_REFERENCES  |\
                                   KPH_PROCESS_NO_FILE_TRANSACTION)

#endif

#define KPH_PROCESS_STATE_LOW     (KPH_PROCESS_VERIFIED_PROCESS             |\
                                   KPH_PROCESS_HAS_FILE_OBJECT              |\
                                   KPH_PROCESS_HAS_SECTION_OBJECT_POINTERS  |\
                                   KPH_PROCESS_NO_USER_WRITABLE_REFERENCES  |\
                                   KPH_PROCESS_NO_FILE_TRANSACTION)

#define KPH_PROCESS_STATE_MINIMUM (KPH_PROCESS_HAS_FILE_OBJECT              |\
                                   KPH_PROCESS_HAS_SECTION_OBJECT_POINTERS  |\
                                   KPH_PROCESS_NO_USER_WRITABLE_REFERENCES  |\
                                   KPH_PROCESS_NO_FILE_TRANSACTION)

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

} KPH_THREAD_INFORMATION_CLASS;

typedef struct _KPH_PROCESS_BASIC_INFORMATION
{
    KPH_PROCESS_STATE ProcessState;

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
            ULONG IsLsass : 1;
            ULONG IsWow64 : 1;
            ULONG Reserved : 25;
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

// Thread information

#define KPH_STACK_TRACE_CAPTURE_USER_STACK 0x00000001

// Object information

typedef enum _KPH_OBJECT_INFORMATION_CLASS
{
    KphObjectBasicInformation,         // q: OBJECT_BASIC_INFORMATION
    KphObjectNameInformation,          // q: OBJECT_NAME_INFORMATION
    KphObjectTypeInformation,          // q: OBJECT_TYPE_INFORMATION
    KphObjectHandleFlagInformation,    // qs: OBJECT_HANDLE_FLAG_INFORMATION
    KphObjectProcessBasicInformation,  // q: PROCESS_BASIC_INFORMATION
    KphObjectThreadBasicInformation,   // q: THREAD_BASIC_INFORMATION
    KphObjectEtwRegBasicInformation,   // q: ETWREG_BASIC_INFORMATION
    KphObjectFileObjectInformation,    // q: KPH_FILE_OBJECT_INFORMATION
    KphObjectFileObjectDriver,         // q: KPH_FILE_OBJECT_DRIVER
    KphObjectProcessTimes,             // q: KERNEL_USER_TIMES
    KphObjectThreadTimes,              // q: KERNEL_USER_TIMES
    KphObjectProcessImageFileName,     // q: UNICODE_STRING
    KphObjectThreadNameInformation,    // q: THREAD_NAME_INFORMATION
    KphObjectThreadIsTerminated,       // q: ULONG
    KphObjectSectionBasicInformation,  // q: SECTION_BASIC_INFORMATION
    KphObjectSectionFileName,          // q: UNICODE_STRING
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

// Driver information

typedef enum _DRIVER_INFORMATION_CLASS
{
    DriverBasicInformation,             // q: DRIVER_BASIC_INFORMATION
    DriverNameInformation,              // q: UNICODE_STRING
    DriverServiceKeyNameInformation,    // q: UNICODE_STRING
    DriverImageFileNameInformation,     // q: UNICODE_STRING
    MaxDriverInfoClass
} DRIVER_INFORMATION_CLASS;

typedef struct _DRIVER_BASIC_INFORMATION
{
    ULONG Flags;
    PVOID DriverStart;
    ULONG DriverSize;
} DRIVER_BASIC_INFORMATION, *PDRIVER_BASIC_INFORMATION;

typedef struct _DRIVER_NAME_INFORMATION
{
    UNICODE_STRING DriverName;
} DRIVER_NAME_INFORMATION, *PDRIVER_NAME_INFORMATION;

typedef struct _DRIVER_SERVICE_KEY_NAME_INFORMATION
{
    UNICODE_STRING ServiceKeyName;
} DRIVER_SERVICE_KEY_NAME_INFORMATION, *PDRIVER_SERVICE_KEY_NAME_INFORMATION;

// ETW registration object information

typedef struct _ETWREG_BASIC_INFORMATION
{
    GUID Guid;
    ULONG_PTR SessionId;
} ETWREG_BASIC_INFORMATION, *PETWREG_BASIC_INFORMATION;

// ALPC ojbect information

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

// Verification

#define KPH_PROCESS_READ_ACCESS   (STANDARD_RIGHTS_READ                 |\
                                   SYNCHRONIZE                          |\
                                   PROCESS_QUERY_INFORMATION            |\
                                   PROCESS_QUERY_LIMITED_INFORMATION    |\
                                   PROCESS_VM_READ)

#define KPH_THREAD_READ_ACCESS    (STANDARD_RIGHTS_READ                 |\
                                   SYNCHRONIZE                          |\
                                   THREAD_QUERY_INFORMATION             |\
                                   THREAD_QUERY_LIMITED_INFORMATION     |\
                                   THREAD_GET_CONTEXT)

#define KPH_TOKEN_READ_ACCESS     (STANDARD_RIGHTS_READ                 |\
                                   SYNCHRONIZE                          |\
                                   TOKEN_QUERY                          |\
                                   TOKEN_QUERY_SOURCE)

#define KPH_JOB_READ_ACCESS       (STANDARD_RIGHTS_READ                 |\
                                   SYNCHRONIZE                          |\
                                   JOB_OBJECT_QUERY)

#define KPH_FILE_READ_ACCESS      (STANDARD_RIGHTS_READ                 |\
                                   SYNCHRONIZE                          |\
                                   FILE_READ_DATA                       |\
                                   FILE_READ_ATTRIBUTES                 |\
                                   FILE_READ_EA)

#define KPH_FILE_READ_DISPOSITION (FILE_OPEN)

// Informer

typedef struct _KPH_INFORMER_SETTINGS
{
    union
    {
        volatile ULONGLONG Flags;
        struct
        {
            ULONGLONG ProcessCreate : 1;
            ULONGLONG ProcessExit : 1;
            ULONGLONG ThreadCreate : 1;
            ULONGLONG ThreadExecute : 1;
            ULONGLONG ThreadExit : 1;
            ULONGLONG ImageLoad : 1;
            ULONGLONG DebugPrint : 1;
            ULONGLONG ProcessHandlePreCreate : 1;
            ULONGLONG ProcessHandlePostCreate : 1;
            ULONGLONG ProcessHandlePreDuplicate : 1;
            ULONGLONG ProcessHandlePostDuplicate : 1;
            ULONGLONG ThreadHandlePreCreate : 1;
            ULONGLONG ThreadHandlePostCreate : 1;
            ULONGLONG ThreadHandlePreDuplicate : 1;
            ULONGLONG ThreadHandlePostDuplicate : 1;
            ULONGLONG DesktopHandlePreCreate : 1;
            ULONGLONG DesktopHandlePostCreate : 1;
            ULONGLONG DesktopHandlePreDuplicate : 1;
            ULONGLONG DesktopHandlePostDuplicate : 1;
            ULONGLONG EnableStackTraces : 1;
            ULONGLONG Reserved : 44;
        };
    };

} KPH_INFORMER_SETTINGS, *PKPH_INFORMER_SETTINGS;

#pragma warning(pop)
