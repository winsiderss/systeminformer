/*
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *
 */

#pragma once
extern ULONG KphDynNtVersion;
extern ULONG KphDynObDecodeShift;
extern ULONG KphDynObAttributesShift;

typedef struct _CLIENT_ID32
{
    ULONG UniqueProcess;
    ULONG UniqueThread;
} CLIENT_ID32, *PCLIENT_ID32;

typedef struct _CLIENT_ID64
{
    ULONGLONG UniqueProcess;
    ULONGLONG UniqueThread;
} CLIENT_ID64, *PCLIENT_ID64;

// EX

typedef struct _EX_PUSH_LOCK_WAIT_BLOCK *PEX_PUSH_LOCK_WAIT_BLOCK;

NTKERNELAPI
VOID
FASTCALL
ExfUnblockPushLock(
    _Inout_ PEX_PUSH_LOCK PushLock,
    _Inout_opt_ PEX_PUSH_LOCK_WAIT_BLOCK WaitBlock
    );

typedef struct _HANDLE_TABLE_ENTRY
{
    union
    {
        PVOID Object;
        ULONG ObAttributes;
        ULONG_PTR Value;
    };
    union
    {
        ACCESS_MASK GrantedAccess;
        LONG NextFreeTableEntry;
    };
} HANDLE_TABLE_ENTRY, *PHANDLE_TABLE_ENTRY;

typedef struct _HANDLE_TABLE HANDLE_TABLE, *PHANDLE_TABLE;

typedef
_Function_class_(EX_ENUM_HANDLE_CALLBACK_61)
_Must_inspect_result_
BOOLEAN
NTAPI
EX_ENUM_HANDLE_CALLBACK_61(
    _Inout_ PHANDLE_TABLE_ENTRY HandleTableEntry,
    _In_ HANDLE Handle,
    _In_opt_ PVOID Context
    );
typedef EX_ENUM_HANDLE_CALLBACK_61* PEX_ENUM_HANDLE_CALLBACK_61;

// since WIN8
typedef
_Function_class_(EX_ENUM_HANDLE_CALLBACK)
_Must_inspect_result_
BOOLEAN
NTAPI
EX_ENUM_HANDLE_CALLBACK(
    _In_ PHANDLE_TABLE HandleTable,
    _Inout_ PHANDLE_TABLE_ENTRY HandleTableEntry,
    _In_ HANDLE Handle,
    _In_opt_ PVOID Context
    );
typedef EX_ENUM_HANDLE_CALLBACK* PEX_ENUM_HANDLE_CALLBACK;

NTKERNELAPI
BOOLEAN
NTAPI
ExEnumHandleTable(
    _In_ PHANDLE_TABLE HandleTable,
    _In_ PEX_ENUM_HANDLE_CALLBACK EnumHandleProcedure,
    _Inout_ PVOID Context,
    _Out_opt_ PHANDLE Handle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQuerySystemInformation(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _Out_writes_bytes_opt_(SystemInformationLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQuerySection(
    _In_ HANDLE SectionHandle,
    _In_ SECTION_INFORMATION_CLASS SectionInformationClass,
    _Out_writes_bytes_(SectionInformationLength) PVOID SectionInformation,
    _In_ SIZE_T SectionInformationLength,
    _Out_opt_ PSIZE_T ReturnLength
    );

// IO

extern POBJECT_TYPE *IoDriverObjectType;
extern POBJECT_TYPE *IoDeviceObjectType;

// KE

typedef enum _KAPC_ENVIRONMENT
{
    OriginalApcEnvironment,
    AttachedApcEnvironment,
    CurrentApcEnvironment,
    InsertApcEnvironment

} KAPC_ENVIRONMENT, *PKAPC_ENVIRONMENT;

typedef
_Function_class_(KNORMAL_ROUTINE)
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
NTAPI
KNORMAL_ROUTINE(
    _In_opt_ PVOID NormalContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
    );
typedef KNORMAL_ROUTINE *PKNORMAL_ROUTINE;

typedef
_Function_class_(KRUNDOWN_ROUTINE)
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
NTAPI
KRUNDOWN_ROUTINE(
    _In_ PRKAPC Apc
    );
typedef KRUNDOWN_ROUTINE *PKRUNDOWN_ROUTINE;

typedef
_Function_class_(KKERNEL_ROUTINE)
_IRQL_requires_(APC_LEVEL)
_IRQL_requires_same_
VOID
NTAPI
KKERNEL_ROUTINE(
    _In_ PRKAPC Apc,
    _Inout_ _Deref_pre_maybenull_ PKNORMAL_ROUTINE *NormalRoutine,
    _Inout_ _Deref_pre_maybenull_ PVOID* NormalContext,
    _Inout_ _Deref_pre_maybenull_ PVOID* SystemArgument1,
    _Inout_ _Deref_pre_maybenull_ PVOID* SystemArgument2
    );
typedef KKERNEL_ROUTINE *PKKERNEL_ROUTINE;

NTKERNELAPI
VOID
NTAPI
KeInitializeApc(
    _Out_ PRKAPC Apc,
    _In_ PRKTHREAD Thread,
    _In_ KAPC_ENVIRONMENT Environment,
    _In_ PKKERNEL_ROUTINE KernelRoutine,
    _In_opt_ PKRUNDOWN_ROUTINE RundownRoutine,
    _In_opt_ PKNORMAL_ROUTINE NormalRoutine,
    _In_ KPROCESSOR_MODE Mode,
    _In_opt_ PVOID NormalContext
    );

NTKERNELAPI
BOOLEAN
NTAPI
KeInsertQueueApc(
    _Inout_ PRKAPC Apc,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2,
    _In_ KPRIORITY Increment
    );

NTKERNELAPI
BOOLEAN
NTAPI
KeTestAlertThread (
    _In_ KPROCESSOR_MODE Mode
    );

// OB

// These definitions are no longer correct, but they produce correct results.

#define OBJ_PROTECT_CLOSE 0x00000001
#define OBJ_HANDLE_ATTRIBUTES (OBJ_PROTECT_CLOSE | OBJ_INHERIT | OBJ_AUDIT_OBJECT_CLOSE)

// This attribute is now stored in the GrantedAccess field.
#define ObpAccessProtectCloseBit 0x2000000

// GrantedAccess in the table entry is the low 25 bits
#define OBJ_GRANTED_ACCESS_MASK 0x01ffffff

#define ObpDecodeGrantedAccess(Access) \
    ((Access) & OBJ_GRANTED_ACCESS_MASK)

FORCEINLINE
VOID ObpSetGrantedAccess(
    _Inout_ PACCESS_MASK GrantedAccess,
    _In_ ACCESS_MASK Access
    )
{
    //
    // Preserve the high bits and only set the low 25 bits.
    //
    *GrantedAccess = (Access | (*GrantedAccess & ~OBJ_GRANTED_ACCESS_MASK));
}

FORCEINLINE
PVOID ObpDecodeObject(
    _In_ PVOID Object
    )
{
#if (defined _M_X64) || (defined _M_ARM64)
    if (KphDynNtVersion >= PHNT_WIN8)
    {
        if (KphDynObDecodeShift != ULONG_MAX)
        {
            return (PVOID)(((LONG_PTR)Object >> KphDynObDecodeShift) & ~(ULONG_PTR)0xf);
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        return (PVOID)((ULONG_PTR)Object & ~OBJ_HANDLE_ATTRIBUTES);
    }
#else
    return (PVOID)((ULONG_PTR)Object & ~OBJ_HANDLE_ATTRIBUTES);
#endif
}

FORCEINLINE
ULONG ObpGetHandleAttributes(
    _In_ PHANDLE_TABLE_ENTRY HandleTableEntry
    )
{
#if (defined _M_X64) || (defined _M_ARM64)
    if (KphDynNtVersion >= PHNT_WIN8)
    {
        if (KphDynObAttributesShift != ULONG_MAX)
        {
            return (ULONG)(HandleTableEntry->Value >> KphDynObAttributesShift) & 0x3;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return (HandleTableEntry->ObAttributes & (OBJ_INHERIT | OBJ_AUDIT_OBJECT_CLOSE)) |
            ((HandleTableEntry->GrantedAccess & ObpAccessProtectCloseBit) ? OBJ_PROTECT_CLOSE : 0);
    }
#else
    return (HandleTableEntry->ObAttributes & (OBJ_INHERIT | OBJ_AUDIT_OBJECT_CLOSE)) |
        ((HandleTableEntry->GrantedAccess & ObpAccessProtectCloseBit) ? OBJ_PROTECT_CLOSE : 0);
#endif
}

typedef struct _OBJECT_CREATE_INFORMATION OBJECT_CREATE_INFORMATION, *POBJECT_CREATE_INFORMATION;

// This structure is not correct on Windows 7, but the offsets we need are still correct.
typedef struct _OBJECT_HEADER
{
    LONG PointerCount;
    union
    {
        LONG HandleCount;
        PVOID NextToFree;
    };
    EX_PUSH_LOCK Lock;
    UCHAR TypeIndex;
    union
    {
        UCHAR TraceFlags;
        struct
        {
            UCHAR DbgRefTrace : 1;
            UCHAR DbgTracePermanent : 1;
            UCHAR Reserved : 6;
        };
    };
    UCHAR InfoMask;
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
    union
    {
        POBJECT_CREATE_INFORMATION ObjectCreateInfo;
        PVOID QuotaBlockCharged;
    };
    PVOID SecurityDescriptor;
    QUAD Body;
} OBJECT_HEADER, *POBJECT_HEADER;

#if (defined _M_X64) || (defined _M_ARM64)
C_ASSERT(FIELD_OFFSET(OBJECT_HEADER, Body) == 0x030);
C_ASSERT(sizeof(OBJECT_HEADER) == 0x038);
#else
C_ASSERT(FIELD_OFFSET(OBJECT_HEADER, Body) == 0x018);
C_ASSERT(sizeof(OBJECT_HEADER) == 0x020);
#endif

#define OBJECT_TO_OBJECT_HEADER(Object) CONTAINING_RECORD((Object), OBJECT_HEADER, Body)

NTKERNELAPI
POBJECT_TYPE
NTAPI
ObGetObjectType(
    _In_ PVOID Object
    );

NTKERNELAPI
NTSTATUS
NTAPI
ObOpenObjectByName(
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ POBJECT_TYPE ObjectType,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_opt_ PACCESS_STATE AccessState,
    _In_opt_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID ParseContext,
    _Out_ PHANDLE Handle
    );

NTKERNELAPI
NTSTATUS
NTAPI
ObSetHandleAttributes(
    _In_ HANDLE Handle,
    _In_ POBJECT_HANDLE_FLAG_INFORMATION HandleFlags,
    _In_ KPROCESSOR_MODE AccessMode
    );

// PS

typedef struct _KLDR_DATA_TABLE_ENTRY
{
    LIST_ENTRY InLoadOrderLinks;
    PVOID ExceptionTable;
    ULONG ExceptionTableSize;
    PVOID GpValue;
    PNON_PAGED_DEBUG_INFO NonPagedDebugInfo;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    union
    {
        USHORT SignatureLevel : 4;
        USHORT SignatureType : 3;
        USHORT Frozen : 2;
        USHORT HotPatch : 1;
        USHORT Unused : 6;
        USHORT EntireField;
    } u1;
    PVOID SectionPointer;
    ULONG CheckSum;
    ULONG CoverageSectionSize;
    PVOID CoverageSection;
    PVOID LoadedImports;
    union
    {
        PVOID Spare;
        struct _KLDR_DATA_TABLE_ENTRY* NtDataTableEntry; // win11
    };
    ULONG SizeOfImageNotRounded;
    ULONG TimeDateStamp;

} KLDR_DATA_TABLE_ENTRY, *PKLDR_DATA_TABLE_ENTRY;

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _Out_writes_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwQueryInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ THREADINFOCLASS ThreadInformationClass,
    _Out_writes_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

NTKERNELAPI
NTSTATUS
NTAPI
PsLookupProcessThreadByCid(
    _In_ PCLIENT_ID ClientId,
    _Out_opt_ PEPROCESS *Process,
    _Out_ PETHREAD *Thread
    );

typedef struct _EJOB *PEJOB;

extern POBJECT_TYPE *PsJobType;

NTKERNELAPI
PEJOB
NTAPI
PsGetProcessJob(
    _In_ PEPROCESS Process
    );

NTKERNELAPI
NTSTATUS
NTAPI
PsAcquireProcessExitSynchronization(
    _In_ PEPROCESS Process
    );

NTKERNELAPI
VOID
NTAPI
PsReleaseProcessExitSynchronization(
    _In_ PEPROCESS Process
    );

NTKERNELAPI
PVOID
NTAPI
PsGetProcessSectionBaseAddress(
    _In_ PEPROCESS Process
    );

NTKERNELAPI
NTSTATUS
NTAPI
PsSuspendProcess(
    _In_ PEPROCESS Process
    );

NTKERNELAPI
NTSTATUS
NTAPI
PsResumeProcess(
    _In_ PEPROCESS Process
    );

typedef
_Function_class_(PS_GET_THREAD_EXIT_STATUS)
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NTAPI
PS_GET_THREAD_EXIT_STATUS(
    _In_ PETHREAD Thread
    );
typedef PS_GET_THREAD_EXIT_STATUS* PPS_GET_THREAD_EXIT_STATUS;

typedef struct _PS_PROTECTION
{
    union
    {
        UCHAR Level;
        struct
        {
            UCHAR Type   : 3;
            UCHAR Audit  : 1;                  // Reserved
            UCHAR Signer : 4;
        };
    };

} PS_PROTECTION, *PPS_PROTECTION;

typedef enum _PS_PROTECTED_TYPE
{
    PsProtectedTypeNone = 0,
    PsProtectedTypeProtectedLight = 1,
    PsProtectedTypeProtected = 2

} PS_PROTECTED_TYPE, *PPS_PROTECTED_TYPE;

typedef enum _PS_PROTECTED_SIGNER
{
    PsProtectedSignerNone = 0,
    PsProtectedSignerAuthenticode,
    PsProtectedSignerCodeGen,
    PsProtectedSignerAntimalware,
    PsProtectedSignerLsa,
    PsProtectedSignerWindows,
    PsProtectedSignerWinTcb,
    PsProtectedSignerWinSystem,
    PsProtectedSignerApp,
    PsProtectedSignerMax

} PS_PROTECTED_SIGNER, *PPS_PROTECTED_SIGNER;

typedef
_Function_class_(PS_GET_PROCESS_PROTECTION)
PS_PROTECTION
NTAPI
PS_GET_PROCESS_PROTECTION(
    _In_ PEPROCESS Process
    );
typedef PS_GET_PROCESS_PROTECTION* PPS_GET_PROCESS_PROTECTION;

typedef
_Function_class_(PS_SET_LOAD_IMAGE_NOTIFY_ROUTINE_EX)
NTSTATUS
NTAPI
PS_SET_LOAD_IMAGE_NOTIFY_ROUTINE_EX(
    _In_ PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine,
    _In_ ULONG_PTR Flags
    );
typedef PS_SET_LOAD_IMAGE_NOTIFY_ROUTINE_EX* PPS_SET_LOAD_IMAGE_NOTIFY_ROUTINE_EX;

#if (NTDDI_VERSION < NTDDI_WIN10_RS2)
typedef enum _PSCREATEPROCESSNOTIFYTYPE
{
    PsCreateProcessNotifySubsystems = 0

} PSCREATEPROCESSNOTIFYTYPE;
#endif

typedef
_Function_class_(PS_SET_CREATE_PROCESS_NOTIFY_ROUTINE_EX2)
NTSTATUS
NTAPI
PS_SET_CREATE_PROCESS_NOTIFY_ROUTINE_EX2(
    _In_ PSCREATEPROCESSNOTIFYTYPE NotifyType,
    _In_ PVOID NotifyInformation,
    _In_ BOOLEAN Remove
    );
typedef PS_SET_CREATE_PROCESS_NOTIFY_ROUTINE_EX2* PPS_SET_CREATE_PROCESS_NOTIFY_ROUTINE_EX2;

#if (NTDDI_VERSION < NTDDI_WINTHRESHOLD)
typedef enum _PSCREATETHREADNOTIFYTYPE
{
    PsCreateThreadNotifyNonSystem = 0,
    PsCreateThreadNotifySubsystems = 1

} PSCREATETHREADNOTIFYTYPE;
#endif

typedef
_Function_class_(PS_SET_CREATE_THREAD_NOTIFY_ROUTINE_EX)
NTSTATUS
NTAPI
PS_SET_CREATE_THREAD_NOTIFY_ROUTINE_EX(
    _In_ PSCREATETHREADNOTIFYTYPE NotifyType,
    _In_ PVOID NotifyInformation
    );
typedef PS_SET_CREATE_THREAD_NOTIFY_ROUTINE_EX* PPS_SET_CREATE_THREAD_NOTIFY_ROUTINE_EX;

#ifndef PS_IMAGE_NOTIFY_CONFLICTING_ARCHITECTURE
#define PS_IMAGE_NOTIFY_CONFLICTING_ARCHITECTURE            0x1
#endif

NTKERNELAPI
_Must_inspect_result_
NTSTATUS
NTAPI
PsReferenceProcessFilePointer(
    _In_ PEPROCESS Process,
    _Out_ PFILE_OBJECT* FileObject
    );

NTKERNELAPI
HANDLE
NTAPI
PsGetProcessInheritedFromUniqueProcessId(
    _In_ PEPROCESS Process
    );

#if defined(_WIN64)

NTKERNELAPI
PVOID
NTAPI
PsGetProcessWow64Process(
    _In_ PEPROCESS Process
    );

NTKERNELAPI
PVOID
NTAPI
PsGetCurrentProcessWow64Process(
    VOID
    );

#endif

NTKERNELAPI
BOOLEAN
NTAPI
PsIsProcessBeingDebugged(
    _In_ PEPROCESS Process
    );

NTKERNELAPI
NTSTATUS
NTAPI
ZwSetInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _In_reads_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength
    );

#define ProcessPowerThrottlingState    77
#define ProcessPriorityClassEx        108

#define ThreadPowerThrottlingState     49

// RTL

#ifndef RTL_MAX_DRIVE_LETTERS
#define RTL_MAX_DRIVE_LETTERS 32
#endif

// Sensible limit that may or may not correspond to the actual Windows value.
#define MAX_STACK_DEPTH 256

#define RTL_WALK_USER_MODE_STACK 0x00000001
#define RTL_WALK_VALID_FLAGS 0x00000001

NTKERNELAPI
PIMAGE_NT_HEADERS
NTAPI
RtlImageNtHeader(
    _In_ PVOID Base
    );

NTKERNELAPI
PVOID
NTAPI
RtlImageDirectoryEntryToData(
    _In_ PVOID BaseAddress,
    _In_ BOOLEAN MappedAsImage,
    _In_ USHORT Directory,
    _Out_ PULONG Size
    );

typedef
_Function_class_(RTL_FIND_EXPORTED_ROUTINE_BY_NAME)
PVOID
NTAPI
RTL_FIND_EXPORTED_ROUTINE_BY_NAME(
    _In_ PVOID BaseAddress,
    _In_z_ PCSTR RoutineName
    );
typedef RTL_FIND_EXPORTED_ROUTINE_BY_NAME* PRTL_FIND_EXPORTED_ROUTINE_BY_NAME;

typedef
_Function_class_(RTL_IMAGE_NT_HEADER_EX)
NTSTATUS
NTAPI
RTL_IMAGE_NT_HEADER_EX(
    _In_ ULONG Flags,
    _In_ PVOID Base,
    _In_ ULONG64 Size,
    _Out_ PIMAGE_NT_HEADERS* OutHeaders
    );
typedef RTL_IMAGE_NT_HEADER_EX* PRTL_IMAGE_NT_HEADER_EX;

typedef
_Function_class_(KE_REMOVE_QUEUE_DPC_EX)
_IRQL_requires_max_(HIGH_LEVEL)
BOOLEAN
NTAPI
KE_REMOVE_QUEUE_DPC_EX(
    _Inout_ PRKDPC Dpc,
    _In_ BOOLEAN WaitIfActive
    );
typedef KE_REMOVE_QUEUE_DPC_EX* PKE_REMOVE_QUEUE_DPC_EX;

// MM

extern POBJECT_TYPE *MmSectionObjectType;

// SE

#define SeDebugPrivilege RtlConvertUlongToLuid(SE_DEBUG_PRIVILEGE)
#define SeCreateTokenPrivilege RtlConvertUlongToLuid(SE_CREATE_TOKEN_PRIVILEGE)

// CI

#ifndef ALGIDDEF
#define ALGIDDEF
typedef unsigned int ALG_ID;
#endif

#ifndef ALG_SID_MD5
#define ALG_SID_MD5                     3
#endif ALG_SID_MD5
#ifndef ALG_SID_SHA1
#define ALG_SID_SHA1                    4
#endif
#ifndef ALG_SID_SHA_256
#define ALG_SID_SHA_256                 12
#endif

#ifndef ALG_CLASS_HASH
#define ALG_CLASS_HASH                  (4 << 13)
#endif
#ifndef ALG_TYPE_ANY
#define ALG_TYPE_ANY                    (0)
#endif

#ifndef CALG_MD5
#define CALG_MD5                (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_MD5)
#endif
#ifndef CALG_SHA1
#define CALG_SHA1               (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_SHA1)
#endif
#ifndef CALG_SHA_256
#define CALG_SHA_256            (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_SHA_256)
#endif

#ifndef CRYPTO_BLOBS_DEFINED
#define CRYPTO_BLOBS_DEFINED
typedef struct _CRYPTOAPI_BLOB
{
    ULONG cbData;
    _Field_size_bytes_(cbData) UCHAR *pbData;
}
CRYPT_INTEGER_BLOB, *PCRYPT_INTEGER_BLOB,
CRYPT_UINT_BLOB, *PCRYPT_UINT_BLOB,
CRYPT_OBJID_BLOB, *PCRYPT_OBJID_BLOB,
CERT_NAME_BLOB, *PCERT_NAME_BLOB,
CERT_RDN_VALUE_BLOB, *PCERT_RDN_VALUE_BLOB,
CERT_BLOB, *PCERT_BLOB,
CRL_BLOB, *PCRL_BLOB,
DATA_BLOB, *PDATA_BLOB,
CRYPT_DATA_BLOB, *PCRYPT_DATA_BLOB,
CRYPT_HASH_BLOB, *PCRYPT_HASH_BLOB,
CRYPT_DIGEST_BLOB, *PCRYPT_DIGEST_BLOB,
CRYPT_DER_BLOB, *PCRYPT_DER_BLOB,
CRYPT_ATTR_BLOB, *PCRYPT_ATTR_BLOB;
#endif

#ifndef MINCRYPT_MAX_HASH_LEN
#define MINCRYPT_MAX_HASH_LEN 64
#endif

#ifndef MINCRYPT_SHA1_HASH_LEN
#define MINCRYPT_SHA1_HASH_LEN (160 / 8)
#endif

#ifndef MINCRYPT_SHA256_HASH_LEN
#define MINCRYPT_SHA256_HASH_LEN (256 / 8)
#endif

//
// Self-signed
//
#define MINCRYPT_POLICY_NO_ROOT           0x1
//
// Microsoft Authenticode Root Authority
// Microsoft Root Authority
// Microsoft Root Certificate Authority
//
#define MINCRYPT_POLICY_MICROSOFT_ROOT    0x2
//
// Microsoft Test Root Authority
//
#define MINCRYPT_POLICY_TEST_ROOT         0x4
//
// Microsoft Code Verification Root
//
#define MINCRYPT_POLICY_CODE_ROOT         0x8
//
// Win 10 RS5 started using Unknown Root
//
#define MINCRYPT_POLICY_UNKNOWN_ROOT      0x10
//
// Microsoft Digital Media Authority 2005
// Microsoft Digital Media Authority 2005 for preview releases
//
#define MINCRYPT_POLICY_DMD_ROOT          0x20
//
// MS Protected Media Test Root
//
#define MINCRYPT_POLICY_DMD_TEST_ROOT     0x40
//
// Win 8.1/10 flags
//
#define MINCRYPT_POLICY_3RD_PARTY_ROOT    0x80
#define MINCRYPT_POLICY_TRUSTED_BOOT_ROOT 0x100
#define MINCRYPT_POLICY_UEFI_ROOT         0x200
#define MINCRYPT_POLICY_FLIGHT_ROOT       0x400
#define MINCRYPT_POLICY_PRS_WIN81_ROOT    0x800
#define MINCRYPT_POLICY_TEST_WIN81_ROOT   0x1000
#define MINCRYPT_POLICY_OTHER_ROOT        0x2000
//
// Undefined bits
//
#define MINCRYPT_POLICY_INVALID_BITS      0xC000

//
// This flag masks out errors and only keeps policies
//
#define MINCRYPT_POLICY_ROOT_FLAG         0xFFFF

//
// All these are errors instead
//
#define MINCRYPT_POLICY_NO_SIGNATURE      0x00010000
#define MINCRYPT_POLICY_BAD_CHAIN         0x00020000
#define MINCRYPT_POLICY_BAD_SIGNATURE     0x00040000
#define MINCRYPT_POLICY_NO_CODE_EKU       0x00080000
#define MINCRYPT_POLICY_NO_PAGE_HASHES    0x00100000
#define MINCRYPT_POLICY_CERT_REVOKED      0x00200000
#define MINCRYPT_POLICY_CERT_EXPIRED      0x00400000
#define MINCRYPT_POLICY_UNKNOWN_ERROR     0x10000000
#define MINCRYPT_POLICY_ERROR_BIT_SHIFT   16
#define MINCRYPT_POLICY_ERROR_FLAGS       0xFFFF0000

//
// This set of flags ignores an invalid/unknown root authority and too long
// signing chain.
//
#define MINCRYPT_POLICY_BAD_ERROR_FLAGS (MINCRYPT_POLICY_ERROR_FLAGS & ~MINCRYPT_POLICY_BAD_CHAIN)

// https://docs.microsoft.com/en-us/windows/security/threat-protection/windows-defender-application-control/event-tag-explanations#microsoft-root-cas-trusted-by-windows
typedef enum _MINCRYPT_KNOWN_ROOT
{
    MincryptRootNone,
    MincryptRootUnknown,
    MincryptRootSelfSigned,
    MincryptRootAuthenticode,
    MincryptRootMicrosoftProduct1997,
    MincryptRootMicrosoftProduct2001,
    MincryptRootMicrosoftProduct2010,
    MincryptRootMicrosoftStandard2011,
    MincryptRootMicrosoftCodeVerification2006,
    MincryptRootMicrosoftTest1999,
    MincryptRootMicrosoftTest2010,
    MincryptRootMicrosoftDMDTest2005,
    MincryptRootMicrosoftDMD2005,
    MincryptRootMicrosoftDMDPreview2005,
    MincryptRootMicrosoftFlight2014,
    MincryptRootMicrosoftThirdPartyMarketplace,
    MincryptRootMicrosoftECCTestingRoot2017,
    MincryptRootMicrosoftECCDevelopmentRoot2018,
    MincryptRootMicrosoftECCProduct2018,
    MincryptRootMicrosoftECCProduct2017

} MINCRYPT_KNOWN_ROOT;

typedef struct _MINCRYPT_STRING
{
    PCHAR Buffer;
    USHORT Length;
    UCHAR Asn1EncodingTag;
    UCHAR Spare[1];

} MINCRYPT_STRING, *PMINCRYPT_STRING;

typedef struct _MINCRYPT_CHAIN_ELEMENT
{
    ALG_ID AlgorithmId;
    ULONG HashSize;
    UCHAR Hash[MINCRYPT_MAX_HASH_LEN];
    MINCRYPT_STRING Subject;
    MINCRYPT_STRING Issuer;
    CRYPT_DER_BLOB Certificate;

} MINCRYPT_CHAIN_ELEMENT, *PMINCRYPT_CHAIN_ELEMENT;

typedef struct _MINCRYPT_CHAIN_INFO
{
    ULONG Size;
    PCRYPT_DER_BLOB PublicKeys;
    ULONG NumberOfPublicKeys;
    PCRYPT_OBJID_BLOB ExtendedKeyUses;
    ULONG NumberOfExtendedKeyUses;

    // win10+

    PMINCRYPT_CHAIN_ELEMENT ChainElements;
    ULONG NumberOfChainElements;
    MINCRYPT_KNOWN_ROOT KnownRoot;
    CRYPT_ATTR_BLOB AuthenticodeAttributes;
    UCHAR PlatformManifest[MINCRYPT_SHA256_HASH_LEN];

} MINCRYPT_CHAIN_INFO, *PMINCRYPT_CHAIN_INFO;

typedef struct _MINCRYPT_POLICY_INFO
{
    ULONG Size;
    NTSTATUS VerificationStatus;
    ULONG PolicyBits;
    PMINCRYPT_CHAIN_INFO ChainInfo;

    // win8+

    LARGE_INTEGER RevocationTime;

    // win10+

    LARGE_INTEGER ValidFromTime;
    LARGE_INTEGER ValidToTime;

} MINCRYPT_POLICY_INFO, *PMINCRYPT_POLICY_INFO;

typedef
_Function_class_(CI_FREE_POLICY_INFO)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
NTAPI
CI_FREE_POLICY_INFO(
    _Inout_ PMINCRYPT_POLICY_INFO PolicyInfo
    );
typedef CI_FREE_POLICY_INFO* PCI_FREE_POLICY_INFO;

typedef
_Function_class_(CI_CHECK_SIGNED_FILE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
NTAPI
CI_CHECK_SIGNED_FILE(
    _In_bytecount_(MINCRYPT_SHA1_HASH_LEN) PUCHAR Hash,
    _In_bytecount_(SignatureSize) PUCHAR Signature,
    _In_ ULONG SignatureSize,
    _Inout_opt_ PMINCRYPT_POLICY_INFO PolicyInfo,
    _Out_opt_ PLARGE_INTEGER SigningTime,
    _Inout_opt_ PMINCRYPT_POLICY_INFO TimeStampPolicyInfo
    );
typedef CI_CHECK_SIGNED_FILE* PCI_CHECK_SIGNED_FILE;

typedef
_Function_class_(CI_CHECK_SIGNED_FILE_EX)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
NTAPI
CI_CHECK_SIGNED_FILE_EX(
    _In_bytecount_(HashSize) PUCHAR Hash,
    _In_ ULONG HashSize,
    _In_ ALG_ID AlgorithmId,
    _In_bytecount_(SignatureSize) PUCHAR Signature,
    _In_ ULONG SignatureSize,
    _Inout_opt_ PMINCRYPT_POLICY_INFO PolicyInfo,
    _Out_opt_ PLARGE_INTEGER SigningTime,
    _Inout_opt_ PMINCRYPT_POLICY_INFO TimeStampPolicyInfo
    );
typedef CI_CHECK_SIGNED_FILE_EX* PCI_CHECK_SIGNED_FILE_EX;

typedef
_Function_class_(CI_VERIFY_HASH_IN_CATALOG)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
NTAPI
CI_VERIFY_HASH_IN_CATALOG(
    _In_bytecount_(MINCRYPT_SHA1_HASH_LEN) PUCHAR Hash,
    _In_ ULONG ReloadCatalogs,
    _In_ ULONG SecureProcess,
    _In_ ULONG AcceptRoots,
    _Inout_opt_ PMINCRYPT_POLICY_INFO PolicyInfo,
    _Out_opt_ PUNICODE_STRING CatalogName, // RtlFreeUnicodeString
    _Out_opt_ PLARGE_INTEGER SigningTime,
    _Inout_opt_ PMINCRYPT_POLICY_INFO TimeStampPolicyInfo
    );
typedef CI_VERIFY_HASH_IN_CATALOG* PCI_VERIFY_HASH_IN_CATALOG;

typedef
_Function_class_(CI_VERIFY_HASH_IN_CATALOG_EX)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
NTAPI
CI_VERIFY_HASH_IN_CATALOG_EX(
    _In_bytecount_(HashSize) PUCHAR Hash,
    _In_ ULONG HashSize,
    _In_ ALG_ID AlgorithmId,
    _In_ ULONG ReloadCatalogs,
    _In_ ULONG SecureProcess,
    _In_ ULONG AcceptRoots,
    _Inout_opt_ PMINCRYPT_POLICY_INFO PolicyInfo,
    _Out_opt_ PUNICODE_STRING CatalogName, // RtlFreeUnicodeString
    _Out_opt_ PLARGE_INTEGER SigningTime,
    _Inout_opt_ PMINCRYPT_POLICY_INFO TimeStampPolicyInfo
    );
typedef CI_VERIFY_HASH_IN_CATALOG_EX* PCI_VERIFY_HASH_IN_CATALOG_EX;

// alpc

extern POBJECT_TYPE *LpcPortObjectType;
#define AlpcPortObjectType LpcPortObjectType

#define PORT_CONNECT 0x0001
#define PORT_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1)

typedef struct _PORT_MESSAGE
{
    union
    {
        struct
        {
            CSHORT DataLength;
            CSHORT TotalLength;
        } s1;
        ULONG Length;
    } u1;
    union
    {
        struct
        {
            CSHORT Type;
            CSHORT DataInfoOffset;
        } s2;
        ULONG ZeroInit;
    } u2;
    union
    {
        CLIENT_ID ClientId;
        double DoNotUseThisField;
    };
    ULONG MessageId;
    union
    {
        SIZE_T ClientViewSize; // only valid for LPC_CONNECTION_REQUEST messages
        ULONG CallbackId; // only valid for LPC_REQUEST messages
    };
} PORT_MESSAGE, *PPORT_MESSAGE;

typedef HANDLE ALPC_HANDLE, *PALPC_HANDLE;

#define ALPC_PORFLG_ALLOW_LPC_REQUESTS 0x20000 // rev
#define ALPC_PORFLG_WAITABLE_PORT 0x40000 // dbg
#define ALPC_PORFLG_SYSTEM_PROCESS 0x100000 // dbg

typedef struct _ALPC_PORT_ATTRIBUTES
{
    ULONG Flags;
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    SIZE_T MaxMessageLength;
    SIZE_T MemoryBandwidth;
    SIZE_T MaxPoolUsage;
    SIZE_T MaxSectionSize;
    SIZE_T MaxViewSize;
    SIZE_T MaxTotalSectionSize;
    ULONG DupObjectTypes;
#ifdef _WIN64
    ULONG Reserved;
#endif
} ALPC_PORT_ATTRIBUTES, *PALPC_PORT_ATTRIBUTES;

#define ALPC_MESSAGE_SECURITY_ATTRIBUTE 0x80000000
#define ALPC_MESSAGE_VIEW_ATTRIBUTE 0x40000000
#define ALPC_MESSAGE_CONTEXT_ATTRIBUTE 0x20000000
#define ALPC_MESSAGE_HANDLE_ATTRIBUTE 0x10000000

typedef struct _ALPC_MESSAGE_ATTRIBUTES
{
    ULONG AllocatedAttributes;
    ULONG ValidAttributes;
} ALPC_MESSAGE_ATTRIBUTES, *PALPC_MESSAGE_ATTRIBUTES;

#define ALPC_MSGFLG_REPLY_MESSAGE 0x1
#define ALPC_MSGFLG_LPC_MODE 0x2 // ?
#define ALPC_MSGFLG_RELEASE_MESSAGE 0x10000 // dbg
#define ALPC_MSGFLG_SYNC_REQUEST 0x20000 // dbg
#define ALPC_MSGFLG_WAIT_USER_MODE 0x100000
#define ALPC_MSGFLG_WAIT_ALERTABLE 0x200000
#define ALPC_MSGFLG_WOW64_CALL 0x80000000 // dbg

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcConnectPort(
    _Out_ PHANDLE PortHandle,
    _In_ PUNICODE_STRING PortName,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes,
    _In_ ULONG Flags,
    _In_opt_ PSID RequiredServerSid,
    _Inout_updates_bytes_to_opt_(*BufferLength, *BufferLength) PPORT_MESSAGE ConnectionMessage,
    _Inout_opt_ PULONG BufferLength,
    _Inout_opt_ PALPC_MESSAGE_ATTRIBUTES OutMessageAttributes,
    _Inout_opt_ PALPC_MESSAGE_ATTRIBUTES InMessageAttributes,
    _In_opt_ PLARGE_INTEGER Timeout
    );
