/*
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2024
 *
 */

#pragma once

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

typedef struct _EX_FAST_REF
{
    union
    {
        PVOID Object;
        ULONG_PTR RefCnt : 4;
        ULONG_PTR Value;
    };
} EX_FAST_REF, *PEX_FAST_REF;

typedef struct _EX_PUSH_LOCK_WAIT_BLOCK *PEX_PUSH_LOCK_WAIT_BLOCK;

NTKERNELAPI
VOID
FASTCALL
ExfUnblockPushLock(
    _Inout_ PEX_PUSH_LOCK PushLock,
    _Inout_opt_ PEX_PUSH_LOCK_WAIT_BLOCK WaitBlock
    );

/*
0:000> dt ntoskrnl!_HANDLE_TABLE_ENTRY
   +0x000 VolatileLowValue : Int8B
   +0x000 LowValue         : Int8B
   +0x000 InfoTable        : Ptr64 _HANDLE_TABLE_ENTRY_INFO
   +0x008 HighValue        : Int8B
   +0x008 NextFreeHandleEntry : Ptr64 _HANDLE_TABLE_ENTRY
   +0x008 LeafHandleValue  : _EXHANDLE
   +0x000 RefCountField    : Int8B
   +0x000 Unlocked         : Pos 0, 1 Bit
   +0x000 RefCnt           : Pos 1, 16 Bits
   +0x000 Attributes       : Pos 17, 3 Bits
   +0x000 ObjectPointerBits : Pos 20, 44 Bits
   +0x008 GrantedAccessBits : Pos 0, 25 Bits
   +0x008 NoRightsUpgrade  : Pos 25, 1 Bit
   +0x008 Spare1           : Pos 26, 6 Bits
   +0x00c Spare2           : Uint4B

0:000> dt ntkrla57!_HANDLE_TABLE_ENTRY
   +0x000 VolatileLowValue : Int8B
   +0x000 LowValue         : Int8B
   +0x000 InfoTable        : Ptr64 _HANDLE_TABLE_ENTRY_INFO
   +0x008 HighValue        : Int8B
   +0x008 NextFreeHandleEntry : Ptr64 _HANDLE_TABLE_ENTRY
   +0x008 LeafHandleValue  : _EXHANDLE
   +0x000 RefCountField    : Int8B
   +0x000 Unlocked         : Pos 0, 1 Bit
   +0x000 RefCnt           : Pos 1, 7 Bits
   +0x000 Attributes       : Pos 8, 3 Bits
   +0x000 ObjectPointerBits : Pos 11, 53 Bits
   +0x008 GrantedAccessBits : Pos 0, 25 Bits
   +0x008 NoRightsUpgrade  : Pos 25, 1 Bit
   +0x008 Spare1           : Pos 26, 6 Bits
   +0x00c Spare2           : Uint4B
*/
//
// N.B. We define HANDLE_TABLE_ENTRY this way to allow for dynamic data to
// support different kernels (see above). The number of ObjectPointerBits are
// different for LA57.
//
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
KeTestAlertThread(
    _In_ KPROCESSOR_MODE Mode
    );

typedef
_Function_class_(KE_REMOVE_QUEUE_APC)
BOOLEAN
NTAPI
KE_REMOVE_QUEUE_APC(
    _Inout_ PKAPC Apc
    );
typedef KE_REMOVE_QUEUE_APC* PKE_REMOVE_QUEUE_APC;

// OB

// These definitions are no longer correct, but they produce correct results.

#define OBJ_PROTECT_CLOSE 0x00000001
#define OBJ_HANDLE_ATTRIBUTES (OBJ_PROTECT_CLOSE | OBJ_INHERIT | OBJ_AUDIT_OBJECT_CLOSE)

// This attribute is now stored in the GrantedAccess field.
#define ObpAccessProtectCloseBit 0x2000000

// GrantedAccess in the table entry is the low 25 bits
#define OBJ_GRANTED_ACCESS_MASK 0x01ffffff

#define ObpDecodeGrantedAccess(Access) ((Access) & OBJ_GRANTED_ACCESS_MASK)

FORCEINLINE
VOID
ObpSetGrantedAccess(
    _Inout_ PACCESS_MASK GrantedAccess,
    _In_ ACCESS_MASK Access
    )
{
    //
    // Preserve the high bits and only set the low 25 bits.
    //
    *GrantedAccess = (Access | (*GrantedAccess & ~OBJ_GRANTED_ACCESS_MASK));
}

typedef struct _OBJECT_CREATE_INFORMATION OBJECT_CREATE_INFORMATION, *POBJECT_CREATE_INFORMATION;

typedef struct _OBJECT_HEADER
{
    SSIZE_T PointerCount;
    union
    {
        SSIZE_T HandleCount;
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
#ifdef _WIN64
    ULONG Reserved;
#endif
    union
    {
        POBJECT_CREATE_INFORMATION ObjectCreateInfo;
        PVOID QuotaBlockCharged;
    };
    PVOID SecurityDescriptor;
    QUAD Body;
} OBJECT_HEADER, *POBJECT_HEADER;

#if defined(_M_X64) || defined(_M_ARM64)
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

NTKERNELAPI
NTSTATUS
NTAPI
ObDuplicateObject(
    _In_ PEPROCESS SourceProcess,
    _In_ HANDLE SourceHandle,
    _In_opt_ PEPROCESS TargetProcess,
    _Out_opt_ PHANDLE TargetHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG HandleAttributes,
    _In_ ULONG Options,
    _In_ KPROCESSOR_MODE PreviousMode
    );

// CM

#if (NTDDI_VERSION < NTDDI_WIN10_FE)
typedef struct _REG_SAVE_MERGED_KEY_INFORMATION
{
    PVOID Object;
    HANDLE FileHandle;
    PVOID HighKeyObject;
    PVOID LowKeyObject;
    PVOID CallContext;
    PVOID ObjectContext;
    PVOID Reserved;
} REG_SAVE_MERGED_KEY_INFORMATION, *PREG_SAVE_MERGED_KEY_INFORMATION;
#endif

// LDR

#define IS_INTRESOURCE(_r) ((((ULONG_PTR)(_r)) >> 16) == 0)
#define MAKEINTRESOURCEA(i) ((LPSTR)((ULONG_PTR)((WORD)(i))))
#define MAKEINTRESOURCEW(i) ((LPWSTR)((ULONG_PTR)((WORD)(i))))
#ifdef UNICODE
#define MAKEINTRESOURCE  MAKEINTRESOURCEW
#else
#define MAKEINTRESOURCE  MAKEINTRESOURCEA
#endif

#define RT_VERSION      MAKEINTRESOURCE(16)

#define VS_FILE_INFO            RT_VERSION
#define VS_VERSION_INFO         1

#define VS_FFI_SIGNATURE        0xFEEF04BDL

typedef struct _IMAGE_RESOURCE_DATA_ENTRY IMAGE_RESOURCE_DATA_ENTRY, *PIMAGE_RESOURCE_DATA_ENTRY;

NTKERNELAPI
NTSTATUS
NTAPI
LdrAccessResource(
    _In_ PVOID DllHandle,
    _In_ PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry,
    _Out_opt_ PVOID *ResourceBuffer,
    _Out_opt_ ULONG *ResourceLength
    );

typedef struct _LDR_RESOURCE_INFO
{
    ULONG_PTR Type;
    ULONG_PTR Name;
    ULONG_PTR Language;
} LDR_RESOURCE_INFO, *PLDR_RESOURCE_INFO;

#define RESOURCE_TYPE_LEVEL 0
#define RESOURCE_NAME_LEVEL 1
#define RESOURCE_LANGUAGE_LEVEL 2
#define RESOURCE_DATA_LEVEL 3

NTKERNELAPI
NTSTATUS
NTAPI
LdrFindResource_U(
    _In_ PVOID DllHandle,
    _In_ PLDR_RESOURCE_INFO ResourceInfo,
    _In_ ULONG Level,
    _Out_ PIMAGE_RESOURCE_DATA_ENTRY *ResourceDataEntry
    );

typedef struct _VS_VERSION_INFO_STRUCT
{
    USHORT Length;
    USHORT ValueLength;
    USHORT Type;
    WCHAR Key[1];
} VS_VERSION_INFO_STRUCT, *PVS_VERSION_INFO_STRUCT;

typedef struct _FIXEDFILEINFO
{
    DWORD   dwSignature;
    DWORD   dwStrucVersion;
    DWORD   dwFileVersionMS;
    DWORD   dwFileVersionLS;
    DWORD   dwProductVersionMS;
    DWORD   dwProductVersionLS;
    DWORD   dwFileFlagsMask;
    DWORD   dwFileFlags;
    DWORD   dwFileOS;
    DWORD   dwFileType;
    DWORD   dwFileSubtype;
    DWORD   dwFileDateMS;
    DWORD   dwFileDateLS;
} VS_FIXEDFILEINFO, *PVS_FIXEDFILEINFO;

typedef struct _NON_PAGED_DEBUG_INFO NON_PAGED_DEBUG_INFO, *PNON_PAGED_DEBUG_INFO;

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

// PS

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

#if (NTDDI_VERSION >= NTDDI_WIN10)
NTKERNELAPI
PS_PROTECTION
NTAPI
PsGetProcessProtection(
    _In_ PEPROCESS Process
    );
#endif

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

#ifdef _WIN64

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
PVOID
NTAPI
PsGetThreadTeb(
    _In_ PETHREAD Thread
    );

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
#define ThreadNameInformation          38
#define ThreadExplicitCaseSensitivity  43

#if (NTDDI_VERSION >= NTDDI_WIN10)
extern PLIST_ENTRY PsLoadedModuleList;
extern PERESOURCE PsLoadedModuleResource;
#endif

typedef struct _PROCESS_MITIGATION_POLICY_INFORMATION
{
    PROCESS_MITIGATION_POLICY Policy;
    union
    {
        PROCESS_MITIGATION_ASLR_POLICY ASLRPolicy;
        PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY StrictHandleCheckPolicy;
        PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY SystemCallDisablePolicy;
        PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY ExtensionPointDisablePolicy;
        PROCESS_MITIGATION_DYNAMIC_CODE_POLICY DynamicCodePolicy;
        PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY ControlFlowGuardPolicy;
        PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY SignaturePolicy;
        PROCESS_MITIGATION_FONT_DISABLE_POLICY FontDisablePolicy;
        PROCESS_MITIGATION_IMAGE_LOAD_POLICY ImageLoadPolicy;
        PROCESS_MITIGATION_SYSTEM_CALL_FILTER_POLICY SystemCallFilterPolicy;
        PROCESS_MITIGATION_PAYLOAD_RESTRICTION_POLICY PayloadRestrictionPolicy;
        PROCESS_MITIGATION_CHILD_PROCESS_POLICY ChildProcessPolicy;
        PROCESS_MITIGATION_SIDE_CHANNEL_ISOLATION_POLICY SideChannelIsolationPolicy;
        PROCESS_MITIGATION_USER_SHADOW_STACK_POLICY UserShadowStackPolicy;
        PROCESS_MITIGATION_REDIRECTION_TRUST_POLICY RedirectionTrustPolicy;
        PROCESS_MITIGATION_USER_POINTER_AUTH_POLICY UserPointerAuthPolicy;
        PROCESS_MITIGATION_SEHOP_POLICY SEHOPPolicy;
    };
} PROCESS_MITIGATION_POLICY_INFORMATION, *PPROCESS_MITIGATION_POLICY_INFORMATION;

NTKERNELAPI
PUCHAR
NTAPI
PsGetProcessImageFileName(
    _In_ PEPROCESS Process
    );

typedef
_Function_class_(PS_GET_PROCESS_SEQUENCE_NUMBER)
_IRQL_requires_max_(DISPATCH_LEVEL)
ULONGLONG
PS_GET_PROCESS_SEQUENCE_NUMBER(
    _In_ PEPROCESS Process
    );
typedef PS_GET_PROCESS_SEQUENCE_NUMBER* PPS_GET_PROCESS_SEQUENCE_NUMBER;

typedef
_Function_class_(PS_GET_PROCESS_START_KEY)
_IRQL_requires_max_(DISPATCH_LEVEL)
ULONGLONG
PS_GET_PROCESS_START_KEY(
    _In_ PEPROCESS Process
    );
typedef PS_GET_PROCESS_START_KEY* PPS_GET_PROCESS_START_KEY;

typedef struct _PROCESS_TELEMETRY_ID_INFORMATION
{
    ULONG HeaderSize;
    ULONG ProcessId;
    ULONGLONG ProcessStartKey;
    ULONGLONG CreateTime;
    ULONGLONG CreateInterruptTime;
    ULONGLONG CreateUnbiasedInterruptTime;
    ULONGLONG ProcessSequenceNumber;
    ULONGLONG SessionCreateTime;
    ULONG SessionId;
    ULONG BootId;
    ULONG ImageChecksum;
    ULONG ImageTimeDateStamp;
    ULONG UserSidOffset;
    ULONG ImagePathOffset;
    ULONG PackageNameOffset;
    ULONG RelativeAppNameOffset;
    ULONG CommandLineOffset;
} PROCESS_TELEMETRY_ID_INFORMATION, *PPROCESS_TELEMETRY_ID_INFORMATION;

// RTL

#ifndef RTL_MAX_DRIVE_LETTERS
#define RTL_MAX_DRIVE_LETTERS 32
#endif

#define RTL_WALK_USER_MODE_STACK 0x00000001
#define RTL_WALK_VALID_FLAGS     0x00000001

#define RTL_STACK_WALKING_MODE_FRAMES_TO_SKIP_SHIFT 8

NTKERNELAPI
PIMAGE_NT_HEADERS
NTAPI
RtlImageNtHeader(
    _In_ PVOID Base
    );

#define RTL_IMAGE_NT_HEADER_EX_FLAG_NO_RANGE_CHECK 0x00000001

NTKERNELAPI
NTSTATUS
NTAPI
RtlImageNtHeaderEx(
    _In_ ULONG Flags,
    _In_ PVOID Base,
    _In_ ULONG64 Size,
    _Out_ PIMAGE_NT_HEADERS* OutHeaders
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

#if (NTDDI_VERSION >= NTDDI_WIN10)
NTKERNELAPI
PVOID
NTAPI
RtlFindExportedRoutineByName(
    _In_ PVOID BaseAddress,
    _In_z_ PCSTR RoutineName
    );
#endif

// MM

#define SEC_DRIVER_IMAGE 0x00100000

extern POBJECT_TYPE *MmSectionObjectType;

typedef
_Function_class_(MM_PROTECT_DRIVER_SECTION)
NTSTATUS
MM_PROTECT_DRIVER_SECTION(
    _In_ PVOID AddressWithinSection,
    _In_ SIZE_T Size,
    _In_ ULONG Flags
    );
typedef MM_PROTECT_DRIVER_SECTION* PMM_PROTECT_DRIVER_SECTION;

#define MM_PROTECT_DRIVER_SECTION_ALLOW_UNLOAD          (0x1)

#define MM_PROTECT_DRIVER_SECTION_VALID_FLAGS \
    (MM_PROTECT_DRIVER_SECTION_ALLOW_UNLOAD)

typedef struct _MMVAD_FLAGS
{
    ULONG Lock : 1;
    ULONG LockContended : 1;
    ULONG DeleteInProgress : 1;
    ULONG NoChange : 1;
    ULONG VadType : 3;
    ULONG Protection : 5;
    ULONG PreferredNode : 7;
    ULONG PageSize : 2;
    ULONG PrivateMemory : 1;
} MMVAD_FLAGS, *PMMVAD_FLAGS;

typedef struct _MM_PRIVATE_VAD_FLAGS
{
    ULONG Lock : 1;
    ULONG LockContended : 1;
    ULONG DeleteInProgress : 1;
    ULONG NoChange : 1;
    ULONG VadType : 3;
    ULONG Protection : 5;
    ULONG PreferredNode : 7;
    ULONG PageSize : 2;
    ULONG PrivateMemoryAlwaysSet : 1;
    ULONG WriteWatch : 1;
    ULONG FixedLargePageSize : 1;
    ULONG ZeroFillPagesOptional : 1;
    ULONG Graphics : 1;
    ULONG Enclave : 1;
    ULONG ShadowStack : 1;
    ULONG PhysicalMemoryPfnsReferenced : 1;
} MM_PRIVATE_VAD_FLAGS, *PMM_PRIVATE_VAD_FLAGS;

typedef struct _MM_GRAPHICS_VAD_FLAGS
{
    ULONG Lock : 1;
    ULONG LockContended : 1;
    ULONG DeleteInProgress : 1;
    ULONG NoChange : 1;
    ULONG VadType : 3;
    ULONG Protection : 5;
    ULONG PreferredNode : 7;
    ULONG PageSize : 2;
    ULONG PrivateMemoryAlwaysSet : 1;
    ULONG WriteWatch : 1;
    ULONG FixedLargePageSize : 1;
    ULONG ZeroFillPagesOptional : 1;
    ULONG GraphicsAlwaysSet : 1;
    ULONG GraphicsUseCoherentBus : 1;
    ULONG GraphicsNoCache : 1;
    ULONG GraphicsPageProtection : 3;
} MM_GRAPHICS_VAD_FLAGS, *PMM_GRAPHICS_VAD_FLAGS;

typedef struct _MM_SHARED_VAD_FLAGS
{
    ULONG Lock : 1;
    ULONG LockContended : 1;
    ULONG DeleteInProgress : 1;
    ULONG NoChange : 1;
    ULONG VadType : 3;
    ULONG Protection : 5;
    ULONG PreferredNode : 7;
    ULONG PageSize : 2;
    ULONG PrivateMemoryAlwaysClear : 1;
    ULONG PrivateFixup : 1;
    ULONG HotPatchState : 2;
} MM_SHARED_VAD_FLAGS, *PMM_SHARED_VAD_FLAGS;

typedef struct _MMVAD_FLAGS1
{
    ULONG CommitCharge : 31;
    ULONG MemCommit : 1;
}MMVAD_FLAGS1, *PMMVAD_FLAGS1;

typedef struct _MMVAD_SHORT
{
    union
    {
        struct
        {
            struct _MMVAD_SHORT* NextVad;
            PVOID ExtraCreateInfo;
        };
        RTL_BALANCED_NODE VadNode;
    };
    ULONG StartingVpn;
    ULONG EndingVpn;
#ifdef _WIN64
    UCHAR StartingVpnHigh;
    UCHAR EndingVpnHigh;
    UCHAR CommitChargeHigh;
    union
    {
        UCHAR SpareNT64VadUChar;
        struct // LA57
        {
            UCHAR EndingVpnHigher : 4;
            UCHAR CommitChargeHigher : 4;
        };
    };
#endif
    LONG ReferenceCount;
    EX_PUSH_LOCK PushLock;
    union
    {
        ULONG LongFlags;
        MMVAD_FLAGS VadFlags;
        MM_PRIVATE_VAD_FLAGS PrivateVadFlags;
        MM_GRAPHICS_VAD_FLAGS GraphicsVadFlags;
        MM_SHARED_VAD_FLAGS SharedVadFlags;
        volatile ULONG VolatileVadLong;
    } u;
    union
    {
        ULONG LongFlags1;
        MMVAD_FLAGS1 VadFlags1;
    } u1;
#ifdef _WIN64
    union
    {
        ULONG_PTR EventListULongPtr;
        UCHAR StartingVpnHigher : 4; // LA57
    } u5;
#else
    PVOID EventList; // PMI_VAD_EVENT_BLOCK
#endif
} MMVAD_SHORT, *PMMVAD_SHORT;

FORCEINLINE
PVOID
MiGetVadShortStartAddress(
    _In_ PMMVAD_SHORT Vad
    )
{
#ifdef _WIN64
    ULONG_PTR higher = Vad->u5.StartingVpnHigher;
    ULONG_PTR high = Vad->StartingVpnHigh;
    ULONG_PTR low = Vad->StartingVpn;
    return (PVOID)((low | ((high | (higher << 8)) << 32)) << PAGE_SHIFT);
#else
    return (PVOID)((ULONG_PTR)Vad->StartingVpn << PAGE_SHIFT);
#endif
}

FORCEINLINE
PVOID
MiGetVadShortEndAddress(
    _In_ PMMVAD_SHORT Vad
    )
{
#ifdef _WIN64
    ULONG_PTR higher = Vad->EndingVpnHigher;
    ULONG_PTR high = Vad->EndingVpnHigh;
    ULONG_PTR low = Vad->EndingVpn;
    return (PVOID)(((low + 1) | ((high | (higher << 8)) << 32)) << PAGE_SHIFT);
#else
    return (PVOID)(((ULONG_PTR)Vad->StartingVpn + 1) << PAGE_SHIFT);
#endif
}

typedef struct _MMVAD_FLAGS2
{
    ULONG FileOffset : 24;
    ULONG Large : 1;
    ULONG TrimBehind : 1;
    ULONG Inherit : 1;
    ULONG NoValidationNeeded : 1;
    ULONG PrivateDemandZero : 1;
    ULONG Spare : 3;
} MMVAD_FLAGS2, *PMMVAD_FLAGS2;

typedef struct _MI_VAD_SEQUENTIAL_INFO
{
    ULONGLONG Length : 12;
    ULONGLONG Vpn : 52;
} MI_VAD_SEQUENTIAL_INFO, *PMI_VAD_SEQUENTIAL_INFO;

typedef struct _MMEXTEND_INFO
{
    ULONGLONG CommittedSize;
    ULONG ReferenceCount;
} MMEXTEND_INFO, *PMMEXTEND_INFO;

typedef struct _MMPTE_HIGHLOW
{
    ULONG LowPart;
    ULONG HighPart;
} MMPTE_HIGHLOW, *PMMPTE_HIGHLOW;

typedef struct _MMPTE_HARDWARE
{
    ULONGLONG Valid : 1;
    ULONGLONG Dirty1 : 1;
    ULONGLONG Owner : 1;
    ULONGLONG WriteThrough : 1;
    ULONGLONG CacheDisable : 1;
    ULONGLONG Accessed : 1;
    ULONGLONG Dirty : 1;
    ULONGLONG LargePage : 1;
    ULONGLONG Global : 1;
    ULONGLONG CopyOnWrite : 1;
    ULONGLONG Unused : 1;
    ULONGLONG Write : 1;
    ULONGLONG PageFrameNumber : 40;
    ULONGLONG ReservedForSoftware : 4;
    ULONGLONG WsleAge : 4;
    ULONGLONG WsleProtection : 3;
    ULONGLONG NoExecute : 1;
} MMPTE_HARDWARE, *PMMPTE_HARDWARE;

typedef struct _MMPTE_PROTOTYPE
{
    ULONGLONG Valid : 1;
    ULONGLONG DemandFillProto : 1;
    ULONGLONG HiberVerifyConverted : 1;
    ULONGLONG ReadOnly : 1;
    ULONGLONG SwizzleBit : 1;
    ULONGLONG Protection : 5;
    ULONGLONG Prototype : 1;
    ULONGLONG Combined : 1;
    ULONGLONG Unused1 : 4;
    LONGLONG ProtoAddress : 48;
} MMPTE_PROTOTYPE, * PMMPTE_PROTOTYPE;

typedef struct _MMPTE_SOFTWARE
{
    ULONGLONG Valid : 1;
    ULONGLONG PageFileReserved : 1;
    ULONGLONG PageFileAllocated : 1;
    ULONGLONG ColdPage : 1;
    ULONGLONG SwizzleBit : 1;
    ULONGLONG Protection : 5;
    ULONGLONG Prototype : 1;
    ULONGLONG Transition : 1;
    ULONGLONG PageFileLow : 4;
    ULONGLONG UsedPageTableEntries : 10;
    ULONGLONG ShadowStack : 1;
    ULONGLONG OnStandbyLookaside : 1;
    ULONGLONG Unused : 4;
    ULONGLONG PageFileHigh : 32;
} MMPTE_SOFTWARE, * PMMPTE_SOFTWARE;

typedef struct _MMPTE_TIMESTAMP
{
    ULONGLONG MustBeZero : 1;
    ULONGLONG Unused : 3;
    ULONGLONG SwizzleBit : 1;
    ULONGLONG Protection : 5;
    ULONGLONG Prototype : 1;
    ULONGLONG Transition : 1;
    ULONGLONG PageFileLow : 4;
    ULONGLONG Reserved : 16;
    ULONGLONG GlobalTimeStamp : 32;
} MMPTE_TIMESTAMP, *PMMPTE_TIMESTAMP;

typedef struct _MMPTE_TRANSITION
{
    ULONGLONG Valid : 1;
    ULONGLONG Write : 1;
    ULONGLONG OnStandbyLookaside : 1;
    ULONGLONG IoTracker : 1;
    ULONGLONG SwizzleBit : 1;
    ULONGLONG Protection : 5;
    ULONGLONG Prototype : 1;
    ULONGLONG Transition : 1;
    ULONGLONG PageFrameNumber : 40;
    ULONGLONG Unused : 12;
} MMPTE_TRANSITION, *PMMPTE_TRANSITION;

typedef struct _MMPTE_SUBSECTION
{
    ULONGLONG Valid : 1;
    ULONGLONG Unused0 : 2;
    ULONGLONG OnStandbyLookaside : 1;
    ULONGLONG SwizzleBit : 1;
    ULONGLONG Protection : 5;
    ULONGLONG Prototype : 1;
    ULONGLONG ColdPage : 1;
    ULONGLONG Unused2 : 3;
    ULONGLONG ExecutePrivilege : 1;
    LONGLONG SubsectionAddress : 48;
} MMPTE_SUBSECTION, * PMMPTE_SUBSECTION;

typedef struct _MMPTE_LIST
{
    ULONGLONG Valid : 1;
    ULONGLONG OneEntry : 1;
    ULONGLONG filler0 : 2;
    ULONGLONG SwizzleBit : 1;
    ULONGLONG Protection : 5;
    ULONGLONG Prototype : 1;
    ULONGLONG Transition : 1;
    ULONGLONG filler1 : 16;
    ULONGLONG NextEntry : 36;
} MMPTE_LIST, *PMMPTE_LIST;

typedef struct _MMPTE
{
    union
    {
        ULONGLONG Long;
        volatile ULONGLONG VolatileLong;
#ifndef _WIN64
        MMPTE_HIGHLOW HighLow;
#endif
        MMPTE_HARDWARE Hard;
        MMPTE_PROTOTYPE Proto;
        MMPTE_SOFTWARE Soft;
        MMPTE_TIMESTAMP TimeStamp;
        MMPTE_TRANSITION Trans;
        MMPTE_SUBSECTION Subsect;
        MMPTE_LIST List;
    } u;
} MMPTE, *PMMPTE;

typedef struct _MMVAD
{
    struct _MMVAD_SHORT Core;
    union
    {
        ULONG LongFlags2;
        MMVAD_FLAGS2 VadFlags2;
    } u2;
    PVOID Subsection; // PSUBSECTION
    PMMPTE FirstPrototypePte;
    PMMPTE LastContiguousPte;
    LIST_ENTRY ViewLinks;
    union
    {
        PEPROCESS VadsProcess;
        UCHAR ViewMapType : 3; // VIEW_MAP_TYPE_*
    };
    union
    {
        MI_VAD_SEQUENTIAL_INFO SequentialVa;
        PMMEXTEND_INFO ExtendedInfo;
    } u4;
    PFILE_OBJECT FileObject; // since WIN10
} MMVAD, *PMMVAD;

FORCEINLINE
PVOID
MiGetVadStartAddress(
    _In_ PMMVAD Vad
    )
{
    return MiGetVadShortStartAddress(&Vad->Core);
}

FORCEINLINE
PVOID
MiGetVadEndAddress(
    _In_ PMMVAD Vad
    )
{
    return MiGetVadShortEndAddress(&Vad->Core);
}

NTKERNELAPI
NTSTATUS
MmCreateSection(
    _Out_ PVOID* SectionObject,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ PLARGE_INTEGER MaximumSize,
    _In_ ULONG SectionPageProtection,
    _In_ ULONG AllocationAttributes,
    _In_opt_ HANDLE FileHandle,
    _In_opt_ PFILE_OBJECT FileObject
    );

NTKERNELAPI
NTSTATUS
NTAPI
MmCopyVirtualMemory(
    _In_ PEPROCESS SourceProcess,
    _In_reads_bytes_(BufferSize) PVOID SourceAddress,
    _In_ PEPROCESS TargetProcess,
    _Out_writes_bytes_(BufferSize) PVOID TargetAddress,
    _In_ SIZE_T BufferSize,
    _In_ KPROCESSOR_MODE PreviousMode,
    _Out_ PSIZE_T ReturnSize
    );

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

// rev
// CiFreePolicyInfo
typedef
_Function_class_(CI_FREE_POLICY_INFO)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
NTAPI
CI_FREE_POLICY_INFO(
    _Inout_ PMINCRYPT_POLICY_INFO PolicyInfo
    );
typedef CI_FREE_POLICY_INFO* PCI_FREE_POLICY_INFO;

// rev
// CiCheckSignedFile (pre 6.1.7601.18519)
typedef
_Function_class_(CI_CHECK_SIGNED_FILE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
NTAPI
CI_CHECK_SIGNED_FILE(
    _In_bytecount_(MINCRYPT_SHA1_HASH_LEN) PBYTE Hash,
    _In_bytecount_(SignatureSize) PBYTE Signature,
    _In_ ULONG SignatureSize,
    _Inout_opt_ PMINCRYPT_POLICY_INFO PolicyInfo,
    _Out_opt_ PLARGE_INTEGER SigningTime,
    _Inout_opt_ PMINCRYPT_POLICY_INFO TimeStampPolicyInfo
    );
typedef CI_CHECK_SIGNED_FILE* PCI_CHECK_SIGNED_FILE;

// rev
// CiCheckSignedFile
typedef
_Function_class_(CI_CHECK_SIGNED_FILE_EX)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
NTAPI
CI_CHECK_SIGNED_FILE_EX(
    _In_bytecount_(HashSize) PBYTE Hash,
    _In_ ULONG HashSize,
    _In_ ALG_ID AlgorithmId,
    _In_bytecount_(SignatureSize) PBYTE Signature,
    _In_ ULONG SignatureSize,
    _Inout_opt_ PMINCRYPT_POLICY_INFO PolicyInfo,
    _Out_opt_ PLARGE_INTEGER SigningTime,
    _Inout_opt_ PMINCRYPT_POLICY_INFO TimeStampPolicyInfo
    );
typedef CI_CHECK_SIGNED_FILE_EX* PCI_CHECK_SIGNED_FILE_EX;

// rev
// CiVerifyHashInCatalog (pre 6.1.7601.18519)
typedef
_Function_class_(CI_VERIFY_HASH_IN_CATALOG)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
NTAPI
CI_VERIFY_HASH_IN_CATALOG(
    _In_bytecount_(MINCRYPT_SHA1_HASH_LEN) PBYTE Hash,
    _In_ ULONG ReloadCatalogs,
    _In_ ULONG SecureProcess,
    _In_ ULONG AcceptRoots,
    _Inout_opt_ PMINCRYPT_POLICY_INFO PolicyInfo,
    _Out_opt_ PUNICODE_STRING CatalogName, // RtlFreeUnicodeString
    _Out_opt_ PLARGE_INTEGER SigningTime,
    _Inout_opt_ PMINCRYPT_POLICY_INFO TimeStampPolicyInfo
    );
typedef CI_VERIFY_HASH_IN_CATALOG* PCI_VERIFY_HASH_IN_CATALOG;

// rev
// CiVerifyHashInCatalog
typedef
_Function_class_(CI_VERIFY_HASH_IN_CATALOG_EX)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
NTAPI
CI_VERIFY_HASH_IN_CATALOG_EX(
    _In_bytecount_(HashSize) PBYTE Hash,
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

// rev
#define CI_POLICY_VALID_FLAGS                        0x1BE00078ul
#define CI_POLICY_DEFAULT                            0x00000000ul
#define CI_POLICY_REQUIRE_MICROSOFT                  0x00000001ul // ? legacy
#define CI_POLICY_REQUIRE_SIGNED                     0x00000002ul // ? legacy
#define CI_POLICY_ALLOW_UNSIGNED                     0x00000004ul // ? legacy
#define CI_POLICY_REJECT_UNSIGNED                    0x80000000ul // ? legacy
#define CI_POLICY_CHECK_PROTECTED_PROCESS_EKU        0x00000008ul
#define CI_POLICY_FORCE_PROTECTED_PROCESS_POLICY     0x00000010ul
#define CI_POLICY_ACCEPT_ANY_ROOT_CERTIFICATE        0x00000020ul
#define CI_POLICY_ALLOW_REVOKED_CERTIFICATE          0x00800000ul
#define CI_POLICY_ALLOW_EXPIRED_REVOKED_CERTIFICATE  0x08000000ul

// rev
// CiValidateFileObject
typedef
_Function_class_(CI_VALIDATE_FILE_OBJECT)
NTSTATUS
NTAPI
CI_VALIDATE_FILE_OBJECT(
    _In_ PFILE_OBJECT FileObject,
    _In_ ULONG PolicyFlags,
    _In_ SE_SIGNING_LEVEL LevelCheck,
    _Inout_ PMINCRYPT_POLICY_INFO PolicyInfo,
    _Inout_ PMINCRYPT_POLICY_INFO TimeStampPolicyInfo,
    _Out_ PLARGE_INTEGER SigningTime,
    _Out_writes_bytes_to_(*ThumbprintSize, *ThumbprintSize) PUCHAR Thumbprint,
    _Inout_ PULONG ThumbprintSize,
    _Out_ PULONG ThumbprintAlgorithm
    );
typedef CI_VALIDATE_FILE_OBJECT* PCI_VALIDATE_FILE_OBJECT;

// rev
typedef _Function_class_(CI_ALLOCATE_ROUTINE)
PVOID
NTAPI
CI_ALLOCATE_ROUTINE(
    _In_ ULONG NumberOfBytes
    );
typedef CI_ALLOCATE_ROUTINE* PCI_ALLOCATE_ROUTINE;

// rev
// CiGetCertPublisherName
typedef
_Function_class_(CI_GET_CERT_PUBLISHER_NAME)
NTSTATUS
NTAPI
CI_GET_CERT_PUBLISHER_NAME(
    _In_ PCRYPT_DER_BLOB Certificate, // MINCRYPT_POLICY_INFO.ChainInfo.ChainElements.Certificate
    _In_ PCI_ALLOCATE_ROUTINE AllocateRoutine,
    _Out_ PUNICODE_STRING PublisherName
    );
typedef CI_GET_CERT_PUBLISHER_NAME* PCI_GET_CERT_PUBLISHER_NAME;

// alpc

extern POBJECT_TYPE *LpcPortObjectType;
#define AlpcPortObjectType LpcPortObjectType

#define PORT_CONNECT 0x0001
#define PORT_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | PORT_CONNECT)

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
    _Inout_opt_ PSIZE_T BufferLength,
    _Inout_opt_ PALPC_MESSAGE_ATTRIBUTES OutMessageAttributes,
    _Inout_opt_ PALPC_MESSAGE_ATTRIBUTES InMessageAttributes,
    _In_opt_ PLARGE_INTEGER Timeout
    );

// LXCORE

typedef
_Function_class_(LXP_PROCESS_GET_CURRENT)
_Must_inspect_result_
BOOLEAN
NTAPI
LXP_PROCESS_GET_CURRENT(
    _Out_ PVOID* Thread
    );
typedef LXP_PROCESS_GET_CURRENT* PLXP_PROCESS_GET_CURRENT;

typedef
_Function_class_(LXP_THREAD_GET_CURRENT)
_Must_inspect_result_
BOOLEAN
NTAPI
LXP_THREAD_GET_CURRENT(
    _Out_ PVOID* Thread
    );
typedef LXP_THREAD_GET_CURRENT* PLXP_THREAD_GET_CURRENT;

// CFG

//
// Define flags for setting process CFG valid call target entries.
//

//
// Call target should be made valid.  If not set, the call target is made
// invalid.  Input flag.
//

#define CFG_CALL_TARGET_VALID                               (0x00000001)

//
// Call target has been successfully processed.  Used to report to the caller
// how much progress has been made.  Output flag.
//

#define CFG_CALL_TARGET_PROCESSED                           (0x00000002)

//
// Call target should be made valid only if it is suppressed export.
// What this flag means is that it can *only* be used on a cell which is
// currently in the CFG export suppressed state (only considered for export
// suppressed processes and not legacy CFG processes!), and it is also
// allowed to be used even if the process is a restricted (i.e. no ACG) process.
//

#define CFG_CALL_TARGET_CONVERT_EXPORT_SUPPRESSED_TO_VALID  (0x00000004)

//
// Call target should be made into an XFG call target.
//

#define CFG_CALL_TARGET_VALID_XFG                           (0x00000008)

//
// Call target should be made valid only if it is already an XFG target
// in a process which has XFG audit mode enabled.
//

#define CFG_CALL_TARGET_CONVERT_XFG_TO_CFG                  (0x00000010)

typedef struct _CFG_CALL_TARGET_INFO {
    ULONG_PTR Offset;
    ULONG_PTR Flags;
} CFG_CALL_TARGET_INFO, *PCFG_CALL_TARGET_INFO;

typedef struct _CFG_CALL_TARGET_LIST_INFORMATION
{
    ULONG NumberOfEntries;
    ULONG Reserved;
    PULONG NumberOfEntriesProcessed;
    PCFG_CALL_TARGET_INFO CallTargetInfo;
    PVOID Section; // since REDSTONE5
    ULONGLONG FileOffset;
} CFG_CALL_TARGET_LIST_INFORMATION, *PCFG_CALL_TARGET_LIST_INFORMATION;

// SE

#define SeDebugPrivilege RtlConvertUlongToLuid(SE_DEBUG_PRIVILEGE)
#define SeCreateTokenPrivilege RtlConvertUlongToLuid(SE_CREATE_TOKEN_PRIVILEGE)

NTKERNELAPI
NTSTATUS
NTAPI
SeCaptureSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR OriginalSecurityDescriptor,
    _In_ KPROCESSOR_MODE CurrentMode,
    _In_ POOL_TYPE PoolType,
    _In_ BOOLEAN CaptureIfKernel,
    _Out_ PSECURITY_DESCRIPTOR *CapturedSecurityDescriptor
    );

NTKERNELAPI
NTSTATUS
NTAPI
SeReleaseSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR CapturedSecurityDescriptor,
    _In_ KPROCESSOR_MODE CurrentMode,
    _In_ BOOLEAN CaptureIfKernelMode
    );

#if (NTDDI_VERSION >= NTDDI_WINBLUE)
NTKERNELAPI
NTSTATUS
NTAPI
SeGetCachedSigningLevel(
    _In_ PFILE_OBJECT FileObject,
    _Out_ PULONG Flags,
    _Out_ PSE_SIGNING_LEVEL SigningLevel,
    _Out_writes_bytes_to_opt_(*ThumbprintSize, *ThumbprintSize) PUCHAR Thumbprint,
    _Inout_opt_ PULONG ThumbprintSize,
    _Out_opt_ PULONG ThumbprintAlgorithm
    );
#endif

typedef
_IRQL_requires_max_(PASSIVE_LEVEL)
_Function_class_(SE_REGISTER_IMAGE_VERIFICATION_CALLBACK)
NTKERNELAPI
NTSTATUS
SE_REGISTER_IMAGE_VERIFICATION_CALLBACK(
    _In_ SE_IMAGE_TYPE ImageType,
    _In_ SE_IMAGE_VERIFICATION_CALLBACK_TYPE CallbackType,
    _In_ PSE_IMAGE_VERIFICATION_CALLBACK_FUNCTION CallbackFunction,
    _In_opt_ PVOID CallbackContext,
    _Reserved_ SE_IMAGE_VERIFICATION_CALLBACK_TOKEN Token,
    _Out_ PVOID* CallbackHandle
    );
typedef SE_REGISTER_IMAGE_VERIFICATION_CALLBACK* PSE_REGISTER_IMAGE_VERIFICATION_CALLBACK;

typedef
_IRQL_requires_max_(PASSIVE_LEVEL)
_Function_class_(SE_UNREGISTER_IMAGE_VERIFICATION_CALLBACK)
NTKERNELAPI
VOID
SE_UNREGISTER_IMAGE_VERIFICATION_CALLBACK(
    _In_ PVOID CallbackHandle
    );
typedef SE_UNREGISTER_IMAGE_VERIFICATION_CALLBACK* PSE_UNREGISTER_IMAGE_VERIFICATION_CALLBACK;

// schannel.h

#define UNISP_NAME_A    "Microsoft Unified Security Protocol Provider"
#define UNISP_NAME_W    L"Microsoft Unified Security Protocol Provider"

#define SSL2SP_NAME_A    "Microsoft SSL 2.0"
#define SSL2SP_NAME_W    L"Microsoft SSL 2.0"

#define SSL3SP_NAME_A    "Microsoft SSL 3.0"
#define SSL3SP_NAME_W    L"Microsoft SSL 3.0"

#define TLS1SP_NAME_A    "Microsoft TLS 1.0"
#define TLS1SP_NAME_W    L"Microsoft TLS 1.0"

#define PCT1SP_NAME_A    "Microsoft PCT 1.0"
#define PCT1SP_NAME_W    L"Microsoft PCT 1.0"

#define SCHANNEL_NAME_A  "Schannel"
#define SCHANNEL_NAME_W  L"Schannel"

#define DEFAULT_TLS_SSP_NAME_A  "Default TLS SSP"
#define DEFAULT_TLS_SSP_NAME_W  L"Default TLS SSP"

#ifdef UNICODE

#define UNISP_NAME  UNISP_NAME_W
#define PCT1SP_NAME  PCT1SP_NAME_W
#define SSL2SP_NAME  SSL2SP_NAME_W
#define SSL3SP_NAME  SSL3SP_NAME_W
#define TLS1SP_NAME  TLS1SP_NAME_W
#define SCHANNEL_NAME  SCHANNEL_NAME_W
#define DEFAULT_TLS_SSP_NAME  DEFAULT_TLS_SSP_NAME_W

#else

#define UNISP_NAME  UNISP_NAME_A
#define PCT1SP_NAME  PCT1SP_NAME_A
#define SSL2SP_NAME  SSL2SP_NAME_A
#define SSL3SP_NAME  SSL3SP_NAME_A
#define TLS1SP_NAME  TLS1SP_NAME_A
#define SCHANNEL_NAME  SCHANNEL_NAME_A
#define DEFAULT_TLS_SSP_NAME  DEFAULT_TLS_SSP_NAME_A

#endif

#define SP_PROT_PCT1_SERVER             0x00000001
#define SP_PROT_PCT1_CLIENT             0x00000002
#define SP_PROT_PCT1                    (SP_PROT_PCT1_SERVER | SP_PROT_PCT1_CLIENT)

#define SP_PROT_SSL2_SERVER             0x00000004
#define SP_PROT_SSL2_CLIENT             0x00000008
#define SP_PROT_SSL2                    (SP_PROT_SSL2_SERVER | SP_PROT_SSL2_CLIENT)

#define SP_PROT_SSL3_SERVER             0x00000010
#define SP_PROT_SSL3_CLIENT             0x00000020
#define SP_PROT_SSL3                    (SP_PROT_SSL3_SERVER | SP_PROT_SSL3_CLIENT)

#define SP_PROT_TLS1_SERVER             0x00000040
#define SP_PROT_TLS1_CLIENT             0x00000080
#define SP_PROT_TLS1                    (SP_PROT_TLS1_SERVER | SP_PROT_TLS1_CLIENT)

#define SP_PROT_SSL3TLS1_CLIENTS        (SP_PROT_TLS1_CLIENT | SP_PROT_SSL3_CLIENT)
#define SP_PROT_SSL3TLS1_SERVERS        (SP_PROT_TLS1_SERVER | SP_PROT_SSL3_SERVER)
#define SP_PROT_SSL3TLS1                (SP_PROT_SSL3 | SP_PROT_TLS1)

#define SP_PROT_UNI_SERVER              0x40000000
#define SP_PROT_UNI_CLIENT              0x80000000
#define SP_PROT_UNI                     (SP_PROT_UNI_SERVER | SP_PROT_UNI_CLIENT)

#define SP_PROT_ALL                     0xffffffff
#define SP_PROT_NONE                    0
#define SP_PROT_CLIENTS                 (SP_PROT_PCT1_CLIENT | SP_PROT_SSL2_CLIENT | SP_PROT_SSL3_CLIENT | SP_PROT_UNI_CLIENT | SP_PROT_TLS1_CLIENT)
#define SP_PROT_SERVERS                 (SP_PROT_PCT1_SERVER | SP_PROT_SSL2_SERVER | SP_PROT_SSL3_SERVER | SP_PROT_UNI_SERVER | SP_PROT_TLS1_SERVER)


#define SP_PROT_TLS1_0_SERVER           SP_PROT_TLS1_SERVER
#define SP_PROT_TLS1_0_CLIENT           SP_PROT_TLS1_CLIENT
#define SP_PROT_TLS1_0                  (SP_PROT_TLS1_0_SERVER | \
                                         SP_PROT_TLS1_0_CLIENT)

#define SP_PROT_TLS1_1_SERVER           0x00000100
#define SP_PROT_TLS1_1_CLIENT           0x00000200
#define SP_PROT_TLS1_1                  (SP_PROT_TLS1_1_SERVER | \
                                         SP_PROT_TLS1_1_CLIENT)

#define SP_PROT_TLS1_2_SERVER           0x00000400
#define SP_PROT_TLS1_2_CLIENT           0x00000800
#define SP_PROT_TLS1_2                  (SP_PROT_TLS1_2_SERVER | \
                                         SP_PROT_TLS1_2_CLIENT)

#define SP_PROT_TLS1_3_SERVER           0x00001000
#define SP_PROT_TLS1_3_CLIENT           0x00002000
#define SP_PROT_TLS1_3                  (SP_PROT_TLS1_3_SERVER | \
                                         SP_PROT_TLS1_3_CLIENT)

#define SP_PROT_DTLS_SERVER             0x00010000
#define SP_PROT_DTLS_CLIENT             0x00020000
#define SP_PROT_DTLS                    (SP_PROT_DTLS_SERVER | \
                                         SP_PROT_DTLS_CLIENT)

#define SP_PROT_DTLS1_0_SERVER          SP_PROT_DTLS_SERVER
#define SP_PROT_DTLS1_0_CLIENT          SP_PROT_DTLS_CLIENT
#define SP_PROT_DTLS1_0                 (SP_PROT_DTLS1_0_SERVER | SP_PROT_DTLS1_0_CLIENT)

#define SP_PROT_DTLS1_2_SERVER          0x00040000
#define SP_PROT_DTLS1_2_CLIENT          0x00080000
#define SP_PROT_DTLS1_2                 (SP_PROT_DTLS1_2_SERVER | SP_PROT_DTLS1_2_CLIENT)

#define SP_PROT_DTLS1_X_SERVER          (SP_PROT_DTLS1_0_SERVER | \
                                         SP_PROT_DTLS1_2_SERVER)

#define SP_PROT_DTLS1_X_CLIENT          (SP_PROT_DTLS1_0_CLIENT | \
                                         SP_PROT_DTLS1_2_CLIENT)

#define SP_PROT_DTLS1_X                 (SP_PROT_DTLS1_X_SERVER | \
                                         SP_PROT_DTLS1_X_CLIENT)

#define SP_PROT_TLS1_1PLUS_SERVER       (SP_PROT_TLS1_1_SERVER | \
                                         SP_PROT_TLS1_2_SERVER | \
                                         SP_PROT_TLS1_3_SERVER)
#define SP_PROT_TLS1_1PLUS_CLIENT       (SP_PROT_TLS1_1_CLIENT | \
                                         SP_PROT_TLS1_2_CLIENT | \
                                         SP_PROT_TLS1_3_CLIENT)

#define SP_PROT_TLS1_1PLUS              (SP_PROT_TLS1_1PLUS_SERVER | \
                                         SP_PROT_TLS1_1PLUS_CLIENT)

#define SP_PROT_TLS1_3PLUS_SERVER       SP_PROT_TLS1_3_SERVER
#define SP_PROT_TLS1_3PLUS_CLIENT       SP_PROT_TLS1_3_CLIENT
#define SP_PROT_TLS1_3PLUS              (SP_PROT_TLS1_3PLUS_SERVER | \
                                         SP_PROT_TLS1_3PLUS_CLIENT)

#define SP_PROT_TLS1_X_SERVER           (SP_PROT_TLS1_0_SERVER | \
                                         SP_PROT_TLS1_1_SERVER | \
                                         SP_PROT_TLS1_2_SERVER | \
                                         SP_PROT_TLS1_3_SERVER)
#define SP_PROT_TLS1_X_CLIENT           (SP_PROT_TLS1_0_CLIENT | \
                                         SP_PROT_TLS1_1_CLIENT | \
                                         SP_PROT_TLS1_2_CLIENT | \
                                         SP_PROT_TLS1_3_CLIENT)
#define SP_PROT_TLS1_X                  (SP_PROT_TLS1_X_SERVER | \
                                         SP_PROT_TLS1_X_CLIENT)

#define SP_PROT_SSL3TLS1_X_CLIENTS      (SP_PROT_TLS1_X_CLIENT | \
                                         SP_PROT_SSL3_CLIENT)
#define SP_PROT_SSL3TLS1_X_SERVERS      (SP_PROT_TLS1_X_SERVER | \
                                         SP_PROT_SSL3_SERVER)
#define SP_PROT_SSL3TLS1_X              (SP_PROT_SSL3 | SP_PROT_TLS1_X)

#define SP_PROT_X_CLIENTS               (SP_PROT_CLIENTS | \
                                         SP_PROT_TLS1_X_CLIENT | \
                                         SP_PROT_DTLS1_X_CLIENT)
#define SP_PROT_X_SERVERS               (SP_PROT_SERVERS | \
                                         SP_PROT_TLS1_X_SERVER | \
                                         SP_PROT_DTLS1_X_SERVER)

#define SCH_CRED_NO_SYSTEM_MAPPER                    0x00000002
#define SCH_CRED_NO_SERVERNAME_CHECK                 0x00000004
#define SCH_CRED_MANUAL_CRED_VALIDATION              0x00000008
#define SCH_CRED_NO_DEFAULT_CREDS                    0x00000010
#define SCH_CRED_AUTO_CRED_VALIDATION                0x00000020
#define SCH_CRED_USE_DEFAULT_CREDS                   0x00000040
#define SCH_CRED_DISABLE_RECONNECTS                  0x00000080

#define SCH_CRED_REVOCATION_CHECK_END_CERT           0x00000100
#define SCH_CRED_REVOCATION_CHECK_CHAIN              0x00000200
#define SCH_CRED_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT 0x00000400
#define SCH_CRED_IGNORE_NO_REVOCATION_CHECK          0x00000800
#define SCH_CRED_IGNORE_REVOCATION_OFFLINE           0x00001000

#define SCH_CRED_RESTRICTED_ROOTS                    0x00002000
#define SCH_CRED_REVOCATION_CHECK_CACHE_ONLY         0x00004000
#define SCH_CRED_CACHE_ONLY_URL_RETRIEVAL            0x00008000

#define SCH_CRED_MEMORY_STORE_CERT                   0x00010000

#define SCH_CRED_CACHE_ONLY_URL_RETRIEVAL_ON_CREATE  0x00020000

#define SCH_SEND_ROOT_CERT                           0x00040000
#define SCH_CRED_SNI_CREDENTIAL                      0x00080000
#define SCH_CRED_SNI_ENABLE_OCSP                     0x00100000
#define SCH_SEND_AUX_RECORD                          0x00200000
#define SCH_USE_STRONG_CRYPTO                        0x00400000
#define SCH_USE_PRESHAREDKEY_ONLY                    0x00800000
#define SCH_USE_DTLS_ONLY                            0x01000000
#define SCH_ALLOW_NULL_ENCRYPTION                    0x02000000

#define SCHANNEL_RENEGOTIATE    0   // renegotiate a connection
#define SCHANNEL_SHUTDOWN       1   // gracefully close down a connection
#define SCHANNEL_ALERT          2   // build an error message
#define SCHANNEL_SESSION        3   // session control

#define SCH_CRED_V1              0x00000001
#define SCH_CRED_V2              0x00000002  // for legacy code
#define SCH_CRED_VERSION         0x00000002  // for legacy code
#define SCH_CRED_V3              0x00000003  // for legacy code
#define SCHANNEL_CRED_VERSION    0x00000004  // for legacy code
#define SCH_CREDENTIALS_VERSION  0x00000005

typedef struct _SCHANNEL_CRED
{
    DWORD           dwVersion;      // always SCHANNEL_CRED_VERSION
    DWORD           cCreds;
    //PCCERT_CONTEXT *paCred;
    PVOID           paCred;
    //HCERTSTORE      hRootStore;
    PVOID           hRootStore;

    DWORD           cMappers;
    struct _HMAPPER **aphMappers;

    DWORD           cSupportedAlgs;
    ALG_ID *        palgSupportedAlgs;

    DWORD           grbitEnabledProtocols;
    DWORD           dwMinimumCipherStrength;
    DWORD           dwMaximumCipherStrength;
    DWORD           dwSessionLifespan;
    DWORD           dwFlags;
    DWORD           dwCredFormat;
} SCHANNEL_CRED, *PSCHANNEL_CRED;

typedef enum _eTlsAlgorithmUsage
{
    TlsParametersCngAlgUsageKeyExchange,          // Key exchange algorithm. RSA, ECHDE, DHE, etc.
    TlsParametersCngAlgUsageSignature,            // Signature algorithm. RSA, DSA, ECDSA, etc.
    TlsParametersCngAlgUsageCipher,               // Encryption algorithm. AES, DES, RC4, etc.
    TlsParametersCngAlgUsageDigest,               // Digest of cipher suite. SHA1, SHA256, SHA384, etc.
    TlsParametersCngAlgUsageCertSig               // Signature and/or hash used to sign certificate. RSA, DSA, ECDSA, SHA1, SHA256, etc.
} eTlsAlgorithmUsage;

//
// SCH_CREDENTIALS structures
//
typedef struct _CRYPTO_SETTINGS
{
    eTlsAlgorithmUsage  eAlgorithmUsage;         // How this algorithm is being used.
    UNICODE_STRING      strCngAlgId;             // CNG algorithm identifier.
    DWORD               cChainingModes;          // Set to 0 if CNG algorithm does not have a chaining mode.
    PUNICODE_STRING     rgstrChainingModes;      // Set to NULL if CNG algorithm does not have a chaining mode.
    DWORD               dwMinBitLength;          // Blacklist key sizes less than this. Set to 0 if not defined or CNG algorithm implies bit length.
    DWORD               dwMaxBitLength;          // Blacklist key sizes greater than this. Set to 0 if not defined or CNG algorithm implies bit length.
} CRYPTO_SETTINGS, *PCRYPTO_SETTINGS;

typedef struct _TLS_PARAMETERS
{
    DWORD               cAlpnIds;                // Valid for server applications only. Must be zero otherwise. Number of ALPN IDs in rgstrAlpnIds; set to 0 if applies to all.
    PUNICODE_STRING     rgstrAlpnIds;            // Valid for server applications only. Must be NULL otherwise. Array of ALPN IDs that the following settings apply to; set to NULL if applies to all.
    DWORD               grbitDisabledProtocols;  // List protocols you DO NOT want negotiated.
    DWORD               cDisabledCrypto;         // Number of CRYPTO_SETTINGS structures; set to 0 if there are none.
    PCRYPTO_SETTINGS    pDisabledCrypto;         // Array of CRYPTO_SETTINGS structures; set to NULL if there are none;
    DWORD               dwFlags;                 // Optional flags to pass; set to 0 if there are none.
} TLS_PARAMETERS, *PTLS_PARAMETERS;

#define TLS_PARAMS_OPTIONAL 0x00000001           // Valid for server applications only. Must be zero otherwise.
                                                 // TLS_PARAMETERS that will only be honored if they do not cause this server to terminate the handshake.

typedef struct _SCH_CREDENTIALS
{
    DWORD               dwVersion;               // Always SCH_CREDENTIALS_VERSION.
    DWORD               dwCredFormat;
    DWORD               cCreds;
    //PCCERT_CONTEXT     *paCred;
    PVOID               *paCred;
    //HCERTSTORE          hRootStore;
    PVOID               hRootStore;

    DWORD               cMappers;
    struct _HMAPPER   **aphMappers;

    DWORD               dwSessionLifespan;
    DWORD               dwFlags;
    DWORD               cTlsParameters;
    PTLS_PARAMETERS     pTlsParameters;
} SCH_CREDENTIALS, *PSCH_CREDENTIALS;

#define SCH_CRED_MAX_SUPPORTED_PARAMETERS 16
#define SCH_CRED_MAX_SUPPORTED_ALPN_IDS 16
#define SCH_CRED_MAX_SUPPORTED_CRYPTO_SETTINGS 16
#define SCH_CRED_MAX_SUPPORTED_CHAINING_MODES 16

#define SECPKG_ATTR_ISSUER_LIST          0x50   // (OBSOLETE) returns SecPkgContext_IssuerListInfo
#define SECPKG_ATTR_REMOTE_CRED          0x51   // (OBSOLETE) returns SecPkgContext_RemoteCredentialInfo
#define SECPKG_ATTR_LOCAL_CRED           0x52   // (OBSOLETE) returns SecPkgContext_LocalCredentialInfo
#define SECPKG_ATTR_REMOTE_CERT_CONTEXT  0x53   // returns PCCERT_CONTEXT
#define SECPKG_ATTR_LOCAL_CERT_CONTEXT   0x54   // returns PCCERT_CONTEXT
#define SECPKG_ATTR_ROOT_STORE           0x55   // returns HCERTCONTEXT to the root store
#define SECPKG_ATTR_SUPPORTED_ALGS       0x56   // returns SecPkgCred_SupportedAlgs
#define SECPKG_ATTR_CIPHER_STRENGTHS     0x57   // returns SecPkgCred_CipherStrengths
#define SECPKG_ATTR_SUPPORTED_PROTOCOLS  0x58   // returns SecPkgCred_SupportedProtocols
#define SECPKG_ATTR_ISSUER_LIST_EX       0x59   // returns SecPkgContext_IssuerListInfoEx
#define SECPKG_ATTR_CONNECTION_INFO      0x5a   // returns SecPkgContext_ConnectionInfo
#define SECPKG_ATTR_EAP_KEY_BLOCK        0x5b   // returns SecPkgContext_EapKeyBlock
#define SECPKG_ATTR_MAPPED_CRED_ATTR     0x5c   // returns SecPkgContext_MappedCredAttr
#define SECPKG_ATTR_SESSION_INFO         0x5d   // returns SecPkgContext_SessionInfo
#define SECPKG_ATTR_APP_DATA             0x5e   // sets/returns SecPkgContext_SessionAppData
#define SECPKG_ATTR_REMOTE_CERTIFICATES  0x5F   // returns SecPkgContext_Certificates
#define SECPKG_ATTR_CLIENT_CERT_POLICY   0x60   // sets    SecPkgCred_ClientCertCtlPolicy
#define SECPKG_ATTR_CC_POLICY_RESULT     0x61   // returns SecPkgContext_ClientCertPolicyResult
#define SECPKG_ATTR_USE_NCRYPT           0x62   // Sets the CRED_FLAG_USE_NCRYPT_PROVIDER FLAG on cred group
#define SECPKG_ATTR_LOCAL_CERT_INFO      0x63   // returns SecPkgContext_CertInfo
#define SECPKG_ATTR_CIPHER_INFO          0x64   // returns new CNG SecPkgContext_CipherInfo
#define SECPKG_ATTR_EAP_PRF_INFO         0x65   // sets    SecPkgContext_EapPrfInfo
#define SECPKG_ATTR_SUPPORTED_SIGNATURES 0x66   // returns SecPkgContext_SupportedSignatures
#define SECPKG_ATTR_REMOTE_CERT_CHAIN    0x67   // returns PCCERT_CONTEXT
#define SECPKG_ATTR_UI_INFO              0x68   // sets SEcPkgContext_UiInfo
#define SECPKG_ATTR_EARLY_START          0x69   // sets SecPkgContext_EarlyStart
#define SECPKG_ATTR_KEYING_MATERIAL_INFO 0x6a   // sets SecPkgContext_KeyingMaterialInfo
#define SECPKG_ATTR_KEYING_MATERIAL      0x6b   // returns SecPkgContext_KeyingMaterial
#define SECPKG_ATTR_SRTP_PARAMETERS      0x6c   // returns negotiated SRTP parameters
#define SECPKG_ATTR_TOKEN_BINDING        0x6d   // returns SecPkgContext_TokenBinding
#define SECPKG_ATTR_CONNECTION_INFO_EX   0x6e   // returns SecPkgContext_ConnectionInfoEx
#define SECPKG_ATTR_KEYING_MATERIAL_TOKEN_BINDING 0x6f // returns SecPkgContext_KeyingMaterial specific to Token Binding
#define SECPKG_ATTR_KEYING_MATERIAL_INPROC        0x70 // returns SecPkgContext_KeyingMaterial_Inproc
#define SECPKG_ATTR_CERT_CHECK_RESULT        0x71 // returns SecPkgContext_CertificateValidationResult, use during and after SSPI handshake loop
#define SECPKG_ATTR_CERT_CHECK_RESULT_INPROC 0x72 // returns SecPkgContext_CertificateValidationResult, use only after SSPI handshake loop
#define SECPKG_ATTR_SESSION_TICKET_KEYS      0x73 // sets    SecPkgCred_SessionTicketKeys
#define SECPKG_ATTR_SERIALIZED_REMOTE_CERT_CONTEXT_INPROC 0x74 // returns CERT_BLOB, use only after SSPI handshake loop
#define SECPKG_ATTR_SERIALIZED_REMOTE_CERT_CONTEXT 0x75 // returns CERT_BLOB, use during and after SSPI handshake loop

typedef struct _SecPkgContext_ConnectionInfo
{
    DWORD   dwProtocol;
    ALG_ID  aiCipher;
    DWORD   dwCipherStrength;
    ALG_ID  aiHash;
    DWORD   dwHashStrength;
    ALG_ID  aiExch;
    DWORD   dwExchStrength;
} SecPkgContext_ConnectionInfo, *PSecPkgContext_ConnectionInfo;

#define SZ_ALG_MAX_SIZE 64

#define SECPKGCONTEXT_CONNECTION_INFO_EX_V1   1

typedef struct _SecPkgContext_ConnectionInfoEx
{
    DWORD   dwVersion;
    DWORD   dwProtocol;
    WCHAR   szCipher[SZ_ALG_MAX_SIZE];
    DWORD   dwCipherStrength;
    WCHAR   szHash[SZ_ALG_MAX_SIZE];
    DWORD   dwHashStrength;
    WCHAR   szExchange[SZ_ALG_MAX_SIZE];
    DWORD   dwExchStrength;
} SecPkgContext_ConnectionInfoEx, *PSecPkgContext_ConnectionInfoEx;

#define SECPKGCONTEXT_CIPHERINFO_V1 1

typedef struct _SecPkgContext_CipherInfo
{

    DWORD dwVersion;
    DWORD dwProtocol;
    DWORD dwCipherSuite;
    DWORD dwBaseCipherSuite;
    WCHAR szCipherSuite[SZ_ALG_MAX_SIZE];
    WCHAR szCipher[SZ_ALG_MAX_SIZE];
    DWORD dwCipherLen;
    DWORD dwCipherBlockLen;    // in bytes
    WCHAR szHash[SZ_ALG_MAX_SIZE];
    DWORD dwHashLen;
    WCHAR szExchange[SZ_ALG_MAX_SIZE];
    DWORD dwMinExchangeLen;
    DWORD dwMaxExchangeLen;
    WCHAR szCertificate[SZ_ALG_MAX_SIZE];
    DWORD dwKeyType;
} SecPkgContext_CipherInfo, *PSecPkgContext_CipherInfo;
