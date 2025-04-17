/*
 * Loader support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTLDR_H
#define _NTLDR_H

typedef struct _ACTIVATION_CONTEXT *PACTIVATION_CONTEXT;
typedef struct _LDRP_LOAD_CONTEXT *PLDRP_LOAD_CONTEXT;

//
// DLLs
//

typedef _Function_class_(LDR_INIT_ROUTINE)
BOOLEAN NTAPI LDR_INIT_ROUTINE(
    _In_ PVOID DllHandle,
    _In_ ULONG Reason,
    _In_opt_ PVOID Context
    );
typedef LDR_INIT_ROUTINE* PLDR_INIT_ROUTINE;

typedef struct _LDR_SERVICE_TAG_RECORD
{
    struct _LDR_SERVICE_TAG_RECORD *Next;
    ULONG ServiceTag;
} LDR_SERVICE_TAG_RECORD, *PLDR_SERVICE_TAG_RECORD;

typedef struct _LDRP_CSLIST
{
    PSINGLE_LIST_ENTRY Tail;
} LDRP_CSLIST, *PLDRP_CSLIST;

typedef enum _LDR_DDAG_STATE
{
    LdrModulesMerged = -5,
    LdrModulesInitError = -4,
    LdrModulesSnapError = -3,
    LdrModulesUnloaded = -2,
    LdrModulesUnloading = -1,
    LdrModulesPlaceHolder = 0,
    LdrModulesMapping = 1,
    LdrModulesMapped = 2,
    LdrModulesWaitingForDependencies = 3,
    LdrModulesSnapping = 4,
    LdrModulesSnapped = 5,
    LdrModulesCondensed = 6,
    LdrModulesReadyToInit = 7,
    LdrModulesInitializing = 8,
    LdrModulesReadyToRun = 9
} LDR_DDAG_STATE;

typedef struct _LDR_DDAG_NODE
{
    LIST_ENTRY Modules;
    PLDR_SERVICE_TAG_RECORD ServiceTagList;
    ULONG LoadCount;
    ULONG LoadWhileUnloadingCount;
    ULONG LowestLink;
    union
    {
        LDRP_CSLIST Dependencies;
        SINGLE_LIST_ENTRY RemovalLink;
    };
    LDRP_CSLIST IncomingDependencies;
    LDR_DDAG_STATE State;
    SINGLE_LIST_ENTRY CondenseLink;
    ULONG PreorderNumber;
} LDR_DDAG_NODE, *PLDR_DDAG_NODE;

// rev
typedef struct _LDR_DEPENDENCY_RECORD
{
    SINGLE_LIST_ENTRY DependencyLink;
    PLDR_DDAG_NODE DependencyNode;
    SINGLE_LIST_ENTRY IncomingDependencyLink;
    PLDR_DDAG_NODE IncomingDependencyNode;
} LDR_DEPENDENCY_RECORD, *PLDR_DEPENDENCY_RECORD;

typedef enum _LDR_DLL_LOAD_REASON
{
    LoadReasonStaticDependency,
    LoadReasonStaticForwarderDependency,
    LoadReasonDynamicForwarderDependency,
    LoadReasonDelayloadDependency,
    LoadReasonDynamicLoad,
    LoadReasonAsImageLoad,
    LoadReasonAsDataLoad,
    LoadReasonEnclavePrimary, // since REDSTONE3
    LoadReasonEnclaveDependency,
    LoadReasonPatchImage, // since WIN11
    LoadReasonUnknown = -1
} LDR_DLL_LOAD_REASON, *PLDR_DLL_LOAD_REASON;

typedef enum _LDR_HOT_PATCH_STATE
{
    LdrHotPatchBaseImage,
    LdrHotPatchNotApplied,
    LdrHotPatchAppliedReverse,
    LdrHotPatchAppliedForward,
    LdrHotPatchFailedToPatch,
    LdrHotPatchStateMax,
} LDR_HOT_PATCH_STATE, *PLDR_HOT_PATCH_STATE;

// LDR_DATA_TABLE_ENTRY->Flags
#define LDRP_PACKAGED_BINARY 0x00000001
#define LDRP_MARKED_FOR_REMOVAL 0x00000002
#define LDRP_IMAGE_DLL 0x00000004
#define LDRP_LOAD_NOTIFICATIONS_SENT 0x00000008
#define LDRP_TELEMETRY_ENTRY_PROCESSED 0x00000010
#define LDRP_PROCESS_STATIC_IMPORT 0x00000020
#define LDRP_IN_LEGACY_LISTS 0x00000040
#define LDRP_IN_INDEXES 0x00000080
#define LDRP_SHIM_DLL 0x00000100
#define LDRP_IN_EXCEPTION_TABLE 0x00000200
#define LDRP_LOAD_IN_PROGRESS 0x00001000
#define LDRP_LOAD_CONFIG_PROCESSED 0x00002000
#define LDRP_ENTRY_PROCESSED 0x00004000
#define LDRP_PROTECT_DELAY_LOAD 0x00008000
#define LDRP_DONT_CALL_FOR_THREADS 0x00040000
#define LDRP_PROCESS_ATTACH_CALLED 0x00080000
#define LDRP_PROCESS_ATTACH_FAILED 0x00100000
#define LDRP_COR_DEFERRED_VALIDATE 0x00200000
#define LDRP_COR_IMAGE 0x00400000
#define LDRP_DONT_RELOCATE 0x00800000
#define LDRP_COR_IL_ONLY 0x01000000
#define LDRP_CHPE_IMAGE 0x02000000
#define LDRP_CHPE_EMULATOR_IMAGE 0x04000000
#define LDRP_REDIRECTED 0x10000000
#define LDRP_COMPAT_DATABASE_PROCESSED 0x80000000

#define LDR_DATA_TABLE_ENTRY_SIZE_WINXP FIELD_OFFSET(LDR_DATA_TABLE_ENTRY, DdagNode)
#define LDR_DATA_TABLE_ENTRY_SIZE_WIN7 FIELD_OFFSET(LDR_DATA_TABLE_ENTRY, BaseNameHashValue)
#define LDR_DATA_TABLE_ENTRY_SIZE_WIN8 FIELD_OFFSET(LDR_DATA_TABLE_ENTRY, ImplicitPathOptions)
#define LDR_DATA_TABLE_ENTRY_SIZE_WIN10 FIELD_OFFSET(LDR_DATA_TABLE_ENTRY, SigningLevel)
#define LDR_DATA_TABLE_ENTRY_SIZE_WIN11 sizeof(LDR_DATA_TABLE_ENTRY)

// symbols
typedef struct _LDR_DATA_TABLE_ENTRY
{
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PLDR_INIT_ROUTINE EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    union
    {
        UCHAR FlagGroup[4];
        ULONG Flags;
        struct
        {
            ULONG PackagedBinary : 1;
            ULONG MarkedForRemoval : 1;
            ULONG ImageDll : 1;
            ULONG LoadNotificationsSent : 1;
            ULONG TelemetryEntryProcessed : 1;
            ULONG ProcessStaticImport : 1;
            ULONG InLegacyLists : 1;
            ULONG InIndexes : 1;
            ULONG ShimDll : 1;
            ULONG InExceptionTable : 1;
            ULONG ReservedFlags1 : 2;
            ULONG LoadInProgress : 1;
            ULONG LoadConfigProcessed : 1;
            ULONG EntryProcessed : 1;
            ULONG ProtectDelayLoad : 1;
            ULONG ReservedFlags3 : 2;
            ULONG DontCallForThreads : 1;
            ULONG ProcessAttachCalled : 1;
            ULONG ProcessAttachFailed : 1;
            ULONG CorDeferredValidate : 1;
            ULONG CorImage : 1;
            ULONG DontRelocate : 1;
            ULONG CorILOnly : 1;
            ULONG ChpeImage : 1;
            ULONG ChpeEmulatorImage : 1;
            ULONG ReservedFlags5 : 1;
            ULONG Redirected : 1;
            ULONG ReservedFlags6 : 2;
            ULONG CompatDatabaseProcessed : 1;
        };
    };
    USHORT ObsoleteLoadCount;
    USHORT TlsIndex;
    LIST_ENTRY HashLinks;
    ULONG TimeDateStamp;
    PACTIVATION_CONTEXT EntryPointActivationContext;
    PVOID Lock; // RtlAcquireSRWLockExclusive
    PLDR_DDAG_NODE DdagNode;
    LIST_ENTRY NodeModuleLink;
    PLDRP_LOAD_CONTEXT LoadContext;
    PVOID ParentDllBase;
    PVOID SwitchBackContext;
    RTL_BALANCED_NODE BaseAddressIndexNode;
    RTL_BALANCED_NODE MappingInfoIndexNode;
    PVOID OriginalBase;
    LARGE_INTEGER LoadTime;
    ULONG BaseNameHashValue;
    LDR_DLL_LOAD_REASON LoadReason; // since WIN8
    ULONG ImplicitPathOptions;
    ULONG ReferenceCount; // since WIN10
    ULONG DependentLoadFlags;
    UCHAR SigningLevel; // since REDSTONE2
    ULONG CheckSum; // since 22H1
    PVOID ActivePatchImageBase;
    LDR_HOT_PATCH_STATE HotPatchState;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

#define LDR_IS_DATAFILE(DllHandle) (((ULONG_PTR)(DllHandle)) & (ULONG_PTR)1)
#define LDR_IS_IMAGEMAPPING(DllHandle) (((ULONG_PTR)(DllHandle)) & (ULONG_PTR)2)
#define LDR_IS_RESOURCE(DllHandle) (LDR_IS_IMAGEMAPPING(DllHandle) || LDR_IS_DATAFILE(DllHandle))
#define LDR_MAPPEDVIEW_TO_DATAFILE(BaseAddress) ((PVOID)(((ULONG_PTR)(BaseAddress)) | (ULONG_PTR)1))
#define LDR_MAPPEDVIEW_TO_IMAGEMAPPING(BaseAddress) ((PVOID)(((ULONG_PTR)(BaseAddress)) | (ULONG_PTR)2))
#define LDR_DATAFILE_TO_MAPPEDVIEW(DllHandle) ((PVOID)(((ULONG_PTR)(DllHandle)) & ~(ULONG_PTR)1))
#define LDR_IMAGEMAPPING_TO_MAPPEDVIEW(DllHandle) ((PVOID)(((ULONG_PTR)(DllHandle)) & ~(ULONG_PTR)2))

#if (PHNT_MODE != PHNT_MODE_KERNEL)

NTSYSAPI
NTSTATUS
NTAPI
LdrLoadDll(
    _In_opt_ PCWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PCUNICODE_STRING DllName,
    _Out_ PVOID *DllHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrUnloadDll(
    _In_ PVOID DllHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllHandle(
    _In_opt_ PCWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PCUNICODE_STRING DllName,
    _Out_ PVOID *DllHandle
    );

#define LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT 0x00000001
#define LDR_GET_DLL_HANDLE_EX_PIN 0x00000002

NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllHandleEx(
    _In_ ULONG Flags,
    _In_opt_ PCWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PCUNICODE_STRING DllName,
    _Out_ PVOID *DllHandle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_7)
// rev
NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllHandleByMapping(
    _In_ PVOID BaseAddress,
    _Out_ PVOID *DllHandle
    );
#endif

#if (PHNT_VERSION >= PHNT_WINDOWS_7)
// rev
NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllHandleByName(
    _In_opt_ PCUNICODE_STRING BaseDllName,
    _In_opt_ PCUNICODE_STRING FullDllName,
    _Out_ PVOID *DllHandle
    );
#endif

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// rev
NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllFullName(
    _In_ PVOID DllHandle,
    _Out_ PUNICODE_STRING FullDllName
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllPath(
    _In_  PCWSTR DllName,
    _In_  ULONG  Flags, // LOAD_LIBRARY_SEARCH_*
    _Out_ PWSTR* DllPath,
    _Out_ PWSTR* SearchPaths
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllDirectory(
    _Out_ PUNICODE_STRING DllDirectory
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
LdrSetDllDirectory(
    _In_ PCUNICODE_STRING DllDirectory
    );
#endif

#define LDR_ADDREF_DLL_PIN 0x00000001

NTSYSAPI
NTSTATUS
NTAPI
LdrAddRefDll(
    _In_ ULONG Flags,
    _In_ PVOID DllHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrGetProcedureAddress(
    _In_ PVOID DllHandle,
    _In_opt_ PCANSI_STRING ProcedureName,
    _In_opt_ ULONG ProcedureNumber,
    _Out_ PVOID *ProcedureAddress
    );

// rev
#define LDR_GET_PROCEDURE_ADDRESS_DONT_RECORD_FORWARDER 0x00000001

#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)
// private
NTSYSAPI
NTSTATUS
NTAPI
LdrGetProcedureAddressEx(
    _In_ PVOID DllHandle,
    _In_opt_ PCANSI_STRING ProcedureName,
    _In_opt_ ULONG ProcedureNumber,
    _Out_ PVOID *ProcedureAddress,
    _In_ ULONG Flags
    );
#endif

NTSYSAPI
NTSTATUS
NTAPI
LdrGetKnownDllSectionHandle(
    _In_ PCWSTR DllName,
    _In_ BOOLEAN KnownDlls32,
    _Out_ PHANDLE Section
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
// rev
NTSYSAPI
NTSTATUS
NTAPI
LdrGetProcedureAddressForCaller(
    _In_ PVOID DllHandle,
    _In_opt_ PCANSI_STRING ProcedureName,
    _In_opt_ ULONG ProcedureNumber,
    _Out_ PVOID *ProcedureAddress,
    _In_ ULONG Flags,
    _In_ PVOID *Callback
    );
#endif

#define LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS 0x00000001
#define LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY 0x00000002

#define LDR_LOCK_LOADER_LOCK_DISPOSITION_INVALID 0
#define LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_ACQUIRED 1
#define LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_NOT_ACQUIRED 2

NTSYSAPI
NTSTATUS
NTAPI
LdrLockLoaderLock(
    _In_ ULONG Flags,
    _Out_opt_ ULONG *Disposition,
    _Out_opt_ PVOID *Cookie
    );

#define LDR_UNLOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS 0x00000001

NTSYSAPI
NTSTATUS
NTAPI
LdrUnlockLoaderLock(
    _In_ ULONG Flags,
    _In_opt_ PVOID Cookie
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrRelocateImage(
    _In_ PVOID NewBase,
    _In_opt_ PSTR LoaderName,
    _In_ NTSTATUS Success,
    _In_ NTSTATUS Conflict,
    _In_ NTSTATUS Invalid
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrRelocateImageWithBias(
    _In_ PVOID NewBase,
    _In_opt_ LONGLONG Bias,
    _In_opt_ PSTR LoaderName,
    _In_ NTSTATUS Success,
    _In_ NTSTATUS Conflict,
    _In_ NTSTATUS Invalid
    );

NTSYSAPI
PIMAGE_BASE_RELOCATION
NTAPI
LdrProcessRelocationBlock(
    _In_ ULONG_PTR VA,
    _In_ ULONG SizeOfBlock,
    _In_ PUSHORT NextOffset,
    _In_ LONG_PTR Diff
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
NTSYSAPI
PIMAGE_BASE_RELOCATION
NTAPI
LdrProcessRelocationBlockEx(
    _In_ ULONG Machine, // IMAGE_FILE_MACHINE_AMD64|IMAGE_FILE_MACHINE_ARM|IMAGE_FILE_MACHINE_THUMB|IMAGE_FILE_MACHINE_ARMNT
    _In_ ULONG_PTR VA,
    _In_ ULONG SizeOfBlock,
    _In_ PUSHORT NextOffset,
    _In_ LONG_PTR Diff
    );
#endif

NTSYSAPI
BOOLEAN
NTAPI
LdrVerifyMappedImageMatchesChecksum(
    _In_ PVOID BaseAddress,
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG FileLength
    );

typedef _Function_class_(LDR_IMPORT_MODULE_CALLBACK)
VOID NTAPI LDR_IMPORT_MODULE_CALLBACK(
    _In_ PVOID Parameter,
    _In_ PSTR ModuleName
    );
typedef LDR_IMPORT_MODULE_CALLBACK* PLDR_IMPORT_MODULE_CALLBACK;

NTSYSAPI
NTSTATUS
NTAPI
LdrVerifyImageMatchesChecksum(
    _In_ HANDLE ImageFileHandle,
    _In_opt_ PLDR_IMPORT_MODULE_CALLBACK ImportCallbackRoutine,
    _In_ PVOID ImportCallbackParameter,
    _Out_opt_ PUSHORT ImageCharacteristics
    );

// private
typedef struct _LDR_IMPORT_CALLBACK_INFO
{
    PLDR_IMPORT_MODULE_CALLBACK ImportCallbackRoutine;
    PVOID ImportCallbackParameter;
} LDR_IMPORT_CALLBACK_INFO, *PLDR_IMPORT_CALLBACK_INFO;

// private
typedef struct _LDR_SECTION_INFO
{
    HANDLE SectionHandle;
    ACCESS_MASK DesiredAccess;
    POBJECT_ATTRIBUTES ObjA;
    ULONG SectionPageProtection;
    ULONG AllocationAttributes;
} LDR_SECTION_INFO, *PLDR_SECTION_INFO;

// private
typedef struct _LDR_VERIFY_IMAGE_INFO
{
    ULONG Size;
    ULONG Flags;
    LDR_IMPORT_CALLBACK_INFO CallbackInfo;
    LDR_SECTION_INFO SectionInfo;
    USHORT ImageCharacteristics;
} LDR_VERIFY_IMAGE_INFO, *PLDR_VERIFY_IMAGE_INFO;

#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)
// private
NTSYSAPI
NTSTATUS
NTAPI
LdrVerifyImageMatchesChecksumEx(
    _In_ HANDLE ImageFileHandle,
    _Inout_ PLDR_VERIFY_IMAGE_INFO VerifyInfo
    );
#endif

#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)
// private
NTSYSAPI
NTSTATUS
NTAPI
LdrQueryModuleServiceTags(
    _In_ PVOID DllHandle,
    _Out_writes_(*BufferSize) PULONG ServiceTagBuffer,
    _Inout_ PULONG BufferSize
    );
#endif

// begin_msdn:"DLL Load Notification"

#define LDR_DLL_NOTIFICATION_REASON_LOADED 1
#define LDR_DLL_NOTIFICATION_REASON_UNLOADED 2

typedef struct _LDR_DLL_LOADED_NOTIFICATION_DATA
{
    ULONG Flags;
    PUNICODE_STRING FullDllName;
    PUNICODE_STRING BaseDllName;
    PVOID DllBase;
    ULONG SizeOfImage;
} LDR_DLL_LOADED_NOTIFICATION_DATA, *PLDR_DLL_LOADED_NOTIFICATION_DATA;

typedef struct _LDR_DLL_UNLOADED_NOTIFICATION_DATA
{
    ULONG Flags;
    PCUNICODE_STRING FullDllName;
    PCUNICODE_STRING BaseDllName;
    PVOID DllBase;
    ULONG SizeOfImage;
} LDR_DLL_UNLOADED_NOTIFICATION_DATA, *PLDR_DLL_UNLOADED_NOTIFICATION_DATA;

typedef union _LDR_DLL_NOTIFICATION_DATA
{
    LDR_DLL_LOADED_NOTIFICATION_DATA Loaded;
    LDR_DLL_UNLOADED_NOTIFICATION_DATA Unloaded;
} LDR_DLL_NOTIFICATION_DATA, *PLDR_DLL_NOTIFICATION_DATA;

typedef const LDR_DLL_NOTIFICATION_DATA *PCLDR_DLL_NOTIFICATION_DATA;

typedef _Function_class_(LDR_DLL_NOTIFICATION_FUNCTION)
VOID NTAPI LDR_DLL_NOTIFICATION_FUNCTION(
    _In_ ULONG NotificationReason,
    _In_ PCLDR_DLL_NOTIFICATION_DATA NotificationData,
    _In_opt_ PVOID Context
    );
typedef LDR_DLL_NOTIFICATION_FUNCTION* PLDR_DLL_NOTIFICATION_FUNCTION;

#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)
/**
 * Registers for notification when a DLL is first loaded. This notification occurs before dynamic linking takes place.
 *
 * @param Flags This parameter must be zero.
 * @param NotificationFunction A pointer to an LdrDllNotification notification callback function to call when the DLL is loaded.
 * @param Context A pointer to context data for the callback function.
 * @param Cookie A pointer to a variable to receive an identifier for the callback function. This identifier is used to unregister the notification callback function.
 * @return NTSTATUS Successful or errant status.
 * @remarks https://learn.microsoft.com/en-us/windows/win32/devnotes/ldrregisterdllnotification
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrRegisterDllNotification(
    _In_ ULONG Flags,
    _In_ PLDR_DLL_NOTIFICATION_FUNCTION NotificationFunction,
    _In_opt_ PVOID Context,
    _Out_ PVOID *Cookie
    );

/**
 * Cancels DLL load notification previously registered by calling the LdrRegisterDllNotification function.
 *
 * @param Cookie A pointer to the callback identifier received from the LdrRegisterDllNotification call that registered for notification.
 * @return NTSTATUS Successful or errant status.
 * @remarks https://learn.microsoft.com/en-us/windows/win32/devnotes/ldrunregisterdllnotification
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrUnregisterDllNotification(
    _In_ PVOID Cookie
    );
#endif

// end_msdn

// rev
NTSYSAPI
PUNICODE_STRING
NTAPI
LdrStandardizeSystemPath(
    _In_ PCUNICODE_STRING SystemPath
    );

typedef struct _LDR_FAILURE_DATA
{
    NTSTATUS Status;
    WCHAR DllName[0x20];
    WCHAR AdditionalInfo[0x20];
} LDR_FAILURE_DATA, *PLDR_FAILURE_DATA;

#if (PHNT_VERSION >= PHNT_WINDOWS_8_1)
NTSYSAPI
PLDR_FAILURE_DATA
NTAPI
LdrGetFailureData(
    VOID
    );
#endif

// private
typedef struct _PS_MITIGATION_OPTIONS_MAP
{
    ULONG_PTR Map[3]; // 2 < 20H1
} PS_MITIGATION_OPTIONS_MAP, *PPS_MITIGATION_OPTIONS_MAP;

// private
typedef struct _PS_MITIGATION_AUDIT_OPTIONS_MAP
{
    ULONG_PTR Map[3]; // 2 < 20H1
} PS_MITIGATION_AUDIT_OPTIONS_MAP, *PPS_MITIGATION_AUDIT_OPTIONS_MAP;

// private
typedef struct _PS_SYSTEM_DLL_INIT_BLOCK
{
    ULONG Size;
    ULONG_PTR SystemDllWowRelocation;
    ULONG_PTR SystemDllNativeRelocation;
    ULONG_PTR Wow64SharedInformation[16]; // use WOW64_SHARED_INFORMATION as index
    ULONG RngData;
    union
    {
        ULONG Flags;
        struct
        {
            ULONG CfgOverride : 1;
            ULONG Reserved : 31;
        };
    };
    PS_MITIGATION_OPTIONS_MAP MitigationOptionsMap;
    ULONG_PTR CfgBitMap;
    ULONG_PTR CfgBitMapSize;
    ULONG_PTR Wow64CfgBitMap;
    ULONG_PTR Wow64CfgBitMapSize;
    PS_MITIGATION_AUDIT_OPTIONS_MAP MitigationAuditOptionsMap; // REDSTONE3
    ULONG_PTR ScpCfgCheckFunction; // since 24H2
    ULONG_PTR ScpCfgCheckESFunction;
    ULONG_PTR ScpCfgDispatchFunction;
    ULONG_PTR ScpCfgDispatchESFunction;
    ULONG_PTR ScpArm64EcCallCheck;
    ULONG_PTR ScpArm64EcCfgCheckFunction;
    ULONG_PTR ScpArm64EcCfgCheckESFunction;
} PS_SYSTEM_DLL_INIT_BLOCK, *PPS_SYSTEM_DLL_INIT_BLOCK;

// rev
#if (PHNT_VERSION >= PHNT_WINDOWS_10)
NTSYSAPI PS_SYSTEM_DLL_INIT_BLOCK LdrSystemDllInitBlock;
#endif

#define PS_SYSTEM_DLL_INIT_BLOCK_SIZE_V1 \
    RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK, MitigationAuditOptionsMap)
#define PS_SYSTEM_DLL_INIT_BLOCK_SIZE_V2 \
    RTL_SIZEOF_THROUGH_FIELD(PS_SYSTEM_DLL_INIT_BLOCK, ScpArm64EcCfgCheckESFunction)

//static_assert(PS_SYSTEM_DLL_INIT_BLOCK_SIZE_V1 == 240, "PS_SYSTEM_DLL_INIT_BLOCK_SIZE_V1 must equal 240");
//static_assert(PS_SYSTEM_DLL_INIT_BLOCK_SIZE_V2 == 296, "PS_SYSTEM_DLL_INIT_BLOCK_SIZE_V2 must equal 296");

// rev see also MEMORY_IMAGE_EXTENSION_INFORMATION
typedef struct _RTL_SCPCFG_NTDLL_EXPORTS
{
    PVOID ScpCfgHeader_Nop;
    PVOID ScpCfgEnd_Nop;
    PVOID ScpCfgHeader;
    PVOID ScpCfgEnd;
    PVOID ScpCfgHeader_ES;
    PVOID ScpCfgEnd_ES;
    PVOID ScpCfgHeader_Fptr;
    PVOID ScpCfgEnd_Fptr;
    PVOID LdrpGuardDispatchIcallNoESFptr;
    PVOID __guard_dispatch_icall_fptr;
    PVOID LdrpGuardCheckIcallNoESFptr;
    PVOID __guard_check_icall_fptr;
    PVOID LdrpHandleInvalidUserCallTarget;
    struct
    {
        PVOID NtOpenFile;
        PVOID NtCreateSection;
        PVOID NtQueryAttributesFile;
        PVOID NtOpenSection;
        PVOID NtMapViewOfSection;
    } LdrpCriticalLoaderFunctions;
} RTL_SCPCFG_NTDLL_EXPORTS, *PRTL_SCPCFG_NTDLL_EXPORTS;

// rev
#if (PHNT_VERSION >= PHNT_WINDOWS_11_24H2)
NTSYSAPI RTL_SCPCFG_NTDLL_EXPORTS RtlpScpCfgNtdllExports;
#endif

//
// Load as data table
//

#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrAddLoadAsDataTable(
    _In_ PVOID Module,
    _In_ PCWSTR FilePath,
    _In_ SIZE_T Size,
    _In_ HANDLE Handle,
    _In_opt_ PACTIVATION_CONTEXT ActCtx
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrRemoveLoadAsDataTable(
    _In_ PVOID InitModule,
    _Out_opt_ PVOID *BaseModule,
    _Out_opt_ PSIZE_T Size,
    _In_ ULONG Flags
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrGetFileNameFromLoadAsDataTable(
    _In_ PVOID Module,
    _Out_ PVOID *pFileNamePrt
    );

#endif // (PHNT_VERSION >= PHNT_WINDOWS_VISTA)

NTSYSAPI
NTSTATUS
NTAPI
LdrDisableThreadCalloutsForDll(
    _In_ PVOID DllImageBase
    );

//
// Resources
//

/**
 * The LdrAccessResource function returns a pointer to the first byte of the specified resource in memory.
 *
 * @param DllHandle A handle to the DLL.
 * @param ResourceDataEntry The resource information block.
 * @param ResourceBuffer The pointer to the specified resource in memory.
 * @param ResourceLength The size, in bytes, of the specified resource.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadresource
 */
NTSYSAPI
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

/**
 * The LdrFindResource_U function determines the location of a resource in a DLL.
 *
 * @param DllHandle A handle to the DLL.
 * @param ResourceInfo The type and name of the resource.
 * @param Level The level of resource information.
 * @param ResourceDataEntry The resource information block.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-findresourceexw
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrFindResource_U(
    _In_ PVOID DllHandle,
    _In_ PLDR_RESOURCE_INFO ResourceInfo,
    _In_ ULONG Level,
    _Out_ PIMAGE_RESOURCE_DATA_ENTRY *ResourceDataEntry
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrFindResourceEx_U(
    _In_ ULONG Flags,
    _In_ PVOID DllHandle,
    _In_ PLDR_RESOURCE_INFO ResourceInfo,
    _In_ ULONG Level,
    _Out_ PIMAGE_RESOURCE_DATA_ENTRY *ResourceDataEntry
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrFindResourceDirectory_U(
    _In_ PVOID DllHandle,
    _In_ PLDR_RESOURCE_INFO ResourceInfo,
    _In_ ULONG Level,
    _Out_ PIMAGE_RESOURCE_DIRECTORY *ResourceDirectory
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
/**
 * The LdrResFindResource function finds a resource in a DLL.
 *
 * @param DllHandle A handle to the DLL.
 * @param Type The type of the resource.
 * @param Name The name of the resource.
 * @param Language The language of the resource.
 * @param ResourceBuffer An optional pointer to receive the resource buffer.
 * @param ResourceLength An optional pointer to receive the resource length.
 * @param CultureName An optional buffer to receive the culture name.
 * @param CultureNameLength An optional pointer to receive the length of the culture name.
 * @param Flags Flags for the resource search.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrResFindResource(
    _In_ PVOID DllHandle,
    _In_ ULONG_PTR Type,
    _In_ ULONG_PTR Name,
    _In_ ULONG_PTR Language,
    _Out_opt_ PVOID* ResourceBuffer,
    _Out_opt_ PULONG ResourceLength,
    _Out_writes_bytes_opt_(CultureNameLength) PVOID CultureName, // WCHAR buffer[6]
    _Out_opt_ PULONG CultureNameLength,
    _In_ ULONG Flags
    );

/**
 * The LdrResFindResourceDirectory function finds a resource directory in a DLL.
 *
 * @param DllHandle A handle to the DLL.
 * @param Type The type of the resource.
 * @param Name The name of the resource.
 * @param ResourceDirectory An optional pointer to receive the resource directory.
 * @param CultureName An optional buffer to receive the culture name.
 * @param CultureNameLength An optional pointer to receive the length of the culture name.
 * @param Flags Flags for the resource search.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrResFindResourceDirectory(
    _In_ PVOID DllHandle,
    _In_ ULONG_PTR Type,
    _In_ ULONG_PTR Name,
    _Out_opt_ PIMAGE_RESOURCE_DIRECTORY* ResourceDirectory,
    _Out_writes_bytes_opt_(CultureNameLength) PVOID CultureName, // WCHAR buffer[6]
    _Out_opt_ PULONG CultureNameLength,
    _In_ ULONG Flags
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrpResGetResourceDirectory(
    _In_ PVOID DllHandle,
    _In_ SIZE_T Size,
    _In_ ULONG Flags,
    _Out_opt_ PIMAGE_RESOURCE_DIRECTORY* ResourceDirectory,
    _Out_ PIMAGE_NT_HEADERS* OutHeaders
    );

/**
* The LdrResSearchResource function searches for a resource in a DLL.
*
* @param DllHandle A handle to the DLL.
* @param ResourceInfo A pointer to the resource information.
* @param Level The level of the resource.
* @param Flags Flags for the resource search.
* @param ResourceBuffer An optional pointer to receive the resource buffer.
* @param ResourceLength An optional pointer to receive the resource length.
* @param CultureName An optional buffer to receive the culture name.
* @param CultureNameLength An optional pointer to receive the length of the culture name.
* @return NTSTATUS Successful or errant status.
*/
NTSYSAPI
NTSTATUS
NTAPI
LdrResSearchResource(
    _In_ PVOID DllHandle,
    _In_ PLDR_RESOURCE_INFO ResourceInfo,
    _In_ ULONG Level,
    _In_ ULONG Flags,
    _Out_opt_ PVOID* ResourceBuffer,
    _Out_opt_ PSIZE_T ResourceLength,
    _Out_writes_bytes_opt_(CultureNameLength) PVOID CultureName, // WCHAR buffer[6]
    _Out_opt_ PULONG CultureNameLength
    );

/**
 * The LdrResGetRCConfig function retrieves the RC configuration for a DLL.
 *
 * @param DllHandle A handle to the DLL.
 * @param Length The length of the configuration buffer.
 * @param Config A buffer to receive the configuration.
 * @param Flags Flags for the operation.
 * @param AlternateResource Indicates if an alternate resource should be loaded.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrResGetRCConfig(
    _In_ PVOID DllHandle,
    _In_opt_ SIZE_T Length,
    _Out_writes_bytes_opt_(Length) PVOID Config,
    _In_ ULONG Flags,
    _In_ BOOLEAN AlternateResource // LdrLoadAlternateResourceModule
    );

/**
 * The LdrResRelease function releases a resource in a DLL.
 *
 * @param DllHandle A handle to the DLL.
 * @param CultureNameOrId An optional culture name or ID.
 * @param Flags Flags for the operation.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrResRelease(
    _In_ PVOID DllHandle,
    _In_opt_ ULONG_PTR CultureNameOrId, // MAKEINTRESOURCE
    _In_ ULONG Flags
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_8)

// private
typedef struct _LDR_ENUM_RESOURCE_ENTRY
{
    union
    {
        ULONG_PTR NameOrId;
        PIMAGE_RESOURCE_DIRECTORY_STRING Name;
        struct
        {
            USHORT Id;
            USHORT NameIsPresent;
        };
    } Path[3];
    PVOID Data;
    ULONG Size;
    ULONG Reserved;
} LDR_ENUM_RESOURCE_ENTRY, *PLDR_ENUM_RESOURCE_ENTRY;

#define NAME_FROM_RESOURCE_ENTRY(RootDirectory, Entry) \
    ((Entry)->NameIsString ? (ULONG_PTR)((ULONG_PTR)(RootDirectory) + (ULONG_PTR)((Entry)->NameOffset)) : (Entry)->Id)

NTSYSAPI
NTSTATUS
NTAPI
LdrEnumResources(
    _In_ PVOID DllHandle,
    _In_ PLDR_RESOURCE_INFO ResourceInfo,
    _In_ ULONG Level,
    _Inout_ ULONG *ResourceCount,
    _Out_writes_to_opt_(*ResourceCount, *ResourceCount) PLDR_ENUM_RESOURCE_ENTRY Resources
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrFindEntryForAddress(
    _In_ PVOID DllHandle,
    _Out_ PLDR_DATA_TABLE_ENTRY *Entry
    );

// rev
/**
 * Returns a handle to the language-specific dynamic-link library (DLL) resource module associated with a DLL that is already loaded for the calling process.
 *
 * \param DllHandle A handle to the DLL module to search for a MUI resource. If the language-specific DLL for the MUI is available, loads the specified module into the address space of the calling process and returns a handle to the module.
 * \param BaseAddress The base address of the mapped view.
 * \param Size The size of the mapped view.
 * \param Flags Reserved
 * \return Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrLoadAlternateResourceModule(
    _In_ PVOID DllHandle,
    _Out_ PVOID *BaseAddress,
    _Out_opt_ SIZE_T *Size,
    _In_ ULONG Flags
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
LdrLoadAlternateResourceModuleEx(
    _In_ PVOID DllHandle,
    _In_ LANGID LanguageId,
    _Out_ PVOID *BaseAddress,
    _Out_opt_ SIZE_T *Size,
    _In_ ULONG Flags
    );

// rev
/**
 * Frees the language-specific dynamic-link library (DLL) resource module previously loaded by LdrLoadAlternateResourceModule function.
 *
 * @param DllHandle The base address of the mapped view.
 * @return Successful or errant status.
 */
NTSYSAPI
BOOLEAN
NTAPI
LdrUnloadAlternateResourceModule(
    _In_ PVOID DllHandle
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
LdrUnloadAlternateResourceModuleEx(
    _In_ PVOID DllHandle,
    _In_ ULONG Flags
    );

#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

//
// Module information
//

typedef struct _RTL_PROCESS_MODULE_INFORMATION
{
    PVOID Section;
    PVOID MappedBase;
    PVOID ImageBase;
    ULONG ImageSize;
    ULONG Flags;
    USHORT LoadOrderIndex;
    USHORT InitOrderIndex;
    USHORT LoadCount;
    USHORT OffsetToFileName;
    UCHAR FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES
{
    ULONG NumberOfModules;
    _Field_size_(NumberOfModules) RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;

// private
typedef struct _RTL_PROCESS_MODULE_INFORMATION_EX
{
    USHORT NextOffset;
    union
    {
        RTL_PROCESS_MODULE_INFORMATION BaseInfo;
        struct
        {
            PVOID Section;
            PVOID MappedBase;
            PVOID ImageBase;
            ULONG ImageSize;
            ULONG Flags;
            USHORT LoadOrderIndex;
            USHORT InitOrderIndex;
            USHORT LoadCount;
            USHORT OffsetToFileName;
            UCHAR FullPathName[256];
        };
    };
    ULONG ImageChecksum;
    ULONG TimeDateStamp;
    PVOID DefaultBase;
} RTL_PROCESS_MODULE_INFORMATION_EX, *PRTL_PROCESS_MODULE_INFORMATION_EX;

#if (PHNT_MODE != PHNT_MODE_KERNEL)

NTSYSAPI
NTSTATUS
NTAPI
LdrQueryProcessModuleInformation(
    _In_opt_ PRTL_PROCESS_MODULES ModuleInformation,
    _In_opt_ ULONG Size,
    _Out_ PULONG ReturnedSize
    );

typedef _Function_class_(LDR_ENUM_CALLBACK)
VOID NTAPI LDR_ENUM_CALLBACK(
    _In_ PLDR_DATA_TABLE_ENTRY ModuleInformation,
    _In_ PVOID Parameter,
    _Out_ BOOLEAN* Stop
    );
typedef LDR_ENUM_CALLBACK* PLDR_ENUM_CALLBACK;

NTSYSAPI
NTSTATUS
NTAPI
LdrEnumerateLoadedModules(
    _In_ BOOLEAN ReservedFlag,
    _In_ PLDR_ENUM_CALLBACK EnumProc,
    _In_ PVOID Context
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrOpenImageFileOptionsKey(
    _In_ PCUNICODE_STRING SubKey,
    _In_ BOOLEAN Wow64,
    _Out_ PHANDLE NewKeyHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrQueryImageFileKeyOption(
    _In_ HANDLE KeyHandle,
    _In_ PCWSTR ValueName,
    _In_ ULONG Type,
    _Out_ PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG ReturnedLength
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrQueryImageFileExecutionOptions(
    _In_ PCUNICODE_STRING SubKey,
    _In_ PCWSTR ValueName,
    _In_ ULONG ValueSize,
    _Out_ PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG ReturnedLength
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrQueryImageFileExecutionOptionsEx(
    _In_ PCUNICODE_STRING SubKey,
    _In_ PCWSTR ValueName,
    _In_ ULONG Type,
    _Out_ PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG ReturnedLength,
    _In_ BOOLEAN Wow64
    );

// private
typedef struct _DELAYLOAD_PROC_DESCRIPTOR
{
    ULONG ImportDescribedByName;
    union
    {
        PCSTR Name;
        ULONG Ordinal;
    } Description;
} DELAYLOAD_PROC_DESCRIPTOR, *PDELAYLOAD_PROC_DESCRIPTOR;

// private
typedef struct _DELAYLOAD_INFO
{
    ULONG Size;
    PCIMAGE_DELAYLOAD_DESCRIPTOR DelayloadDescriptor;
    PIMAGE_THUNK_DATA ThunkAddress;
    PCSTR TargetDllName;
    DELAYLOAD_PROC_DESCRIPTOR TargetApiDescriptor;
    PVOID TargetModuleBase;
    PVOID Unused;
    ULONG LastError;
} DELAYLOAD_INFO, *PDELAYLOAD_INFO;

// private
typedef _Function_class_(DELAYLOAD_FAILURE_DLL_CALLBACK)
PVOID NTAPI DELAYLOAD_FAILURE_DLL_CALLBACK(
    _In_ ULONG NotificationReason,
    _In_ PDELAYLOAD_INFO DelayloadInfo
    );
typedef DELAYLOAD_FAILURE_DLL_CALLBACK* PDELAYLOAD_FAILURE_DLL_CALLBACK;

// rev
typedef _Function_class_(DELAYLOAD_FAILURE_SYSTEM_ROUTINE)
PVOID NTAPI DELAYLOAD_FAILURE_SYSTEM_ROUTINE(
    _In_ PCSTR DllName,
    _In_ PCSTR ProcedureName
    );
typedef DELAYLOAD_FAILURE_SYSTEM_ROUTINE* PDELAYLOAD_FAILURE_SYSTEM_ROUTINE;

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
// rev from QueryOptionalDelayLoadedAPI
/**
 * Determines whether the specified function in a delay-loaded DLL is available on the system.
 *
 * @param ParentModuleBase A handle to the calling module. (NtCurrentImageBase)
 * @param DllName The file name of the delay-loaded DLL that exports the specified function. This parameter is case-insensitive.
 * @param ProcedureName The address of a delay-load failure callback function for the specified DLL and process.
 * @param Flags Reserved; must be 0.
 * @return NTSTATUS Successful or errant status.
 * @remarks https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi2/nf-libloaderapi2-queryoptionaldelayloadedapi
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrQueryOptionalDelayLoadedAPI(
    _In_ PVOID ParentModuleBase,
    _In_ PCSTR DllName,
    _In_ PCSTR ProcedureName,
    _Reserved_ ULONG Flags
    );
#endif

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// rev from ResolveDelayLoadedAPI
/**
 * Locates the target function of the specified import and replaces the function pointer in the import thunk with the target of the function implementation.
 *
 * @param ParentModuleBase The address of the base of the module importing a delay-loaded function. (NtCurrentImageBase)
 * @param DelayloadDescriptor The address of the image delay import directory for the module to be loaded.
 * @param FailureDllHook The address of a delay-load failure callback function for the specified DLL and process.
 * @param FailureSystemHook The address of a delay-load failure callback function for the specified DLL and process.
 * @param ThunkAddress The thunk data for the target function. Used to find the specific name table entry of the function.
 * @param Flags Reserved; must be 0.
 * @return The address of the import, or the failure stub for it.
 * @remarks https://learn.microsoft.com/en-us/windows/win32/devnotes/resolvedelayloadedapi
 */
NTSYSAPI
PVOID
NTAPI
LdrResolveDelayLoadedAPI(
    _In_ PVOID ParentModuleBase,
    _In_ PCIMAGE_DELAYLOAD_DESCRIPTOR DelayloadDescriptor,
    _In_opt_ PDELAYLOAD_FAILURE_DLL_CALLBACK FailureDllHook,
    _In_opt_ PDELAYLOAD_FAILURE_SYSTEM_ROUTINE FailureSystemHook, // kernel32.DelayLoadFailureHook
    _Out_ PIMAGE_THUNK_DATA ThunkAddress,
    _Reserved_ ULONG Flags
    );

// rev from ResolveDelayLoadsFromDll
/**
 * Forwards the work in resolving delay-loaded imports from the parent binary to a target binary.
 *
 * @param [in] ParentModuleBase The base address of the module that delay loads another binary.
 * @param [in] TargetDllName The name of the target DLL.
 * @param [in] Flags Reserved; must be 0.
 * @return NTSTATUS Successful or errant status.
 * @remarks https://learn.microsoft.com/en-us/windows/win32/devnotes/resolvedelayloadsfromdll
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrResolveDelayLoadsFromDll(
    _In_ PVOID ParentModuleBase,
    _In_ PCSTR TargetDllName,
    _Reserved_ ULONG Flags
    );

// rev from SetDefaultDllDirectories
/**
 * Specifies a default set of directories to search when the calling process loads a DLL.
 *
 * @param [in] DirectoryFlags The directories to search.
 * @return NTSTATUS Successful or errant status.
 * @remarks https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-setdefaultdlldirectories
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrSetDefaultDllDirectories(
    _In_ ULONG DirectoryFlags
    );

// rev from AddDllDirectory
/**
 * Adds a directory to the process DLL search path.
 *
 * @param [in] NewDirectory An absolute path to the directory to add to the search path. For example, to add the directory Dir2 to the process DLL search path, specify \Dir2.
 * @param [out] Cookie An opaque pointer that can be passed to RemoveDllDirectory to remove the DLL from the process DLL search path.
 * @return NTSTATUS Successful or errant status.
 * @remarks https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-adddlldirectory
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrAddDllDirectory(
    _In_ PCUNICODE_STRING NewDirectory,
    _Out_ PDLL_DIRECTORY_COOKIE Cookie
    );

// rev from RemoveDllDirectory
/**
 * Removes a directory that was added to the process DLL search path by using LdrAddDllDirectory.
 *
 * @param [in] Cookie The cookie returned by LdrAddDllDirectory when the directory was added to the search path.
 * @return NTSTATUS Successful or errant status.
 * @remarks https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-removedlldirectory
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrRemoveDllDirectory(
    _In_ DLL_DIRECTORY_COOKIE Cookie
    );
#endif

// rev
_Analysis_noreturn_
DECLSPEC_NORETURN
NTSYSAPI
VOID
NTAPI
LdrShutdownProcess(
    VOID
    );

// rev
_Analysis_noreturn_
DECLSPEC_NORETURN
NTSYSAPI
VOID
NTAPI
LdrShutdownThread(
    VOID
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8_1)
// rev
NTSYSAPI
NTSTATUS
NTAPI
LdrSetImplicitPathOptions(
    _In_ ULONG ImplicitPathOptions
    );
#endif

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
#ifdef PHNT_INLINE_TYPEDEFS
/**
 * The LdrControlFlowGuardEnforced function checks if Control Flow Guard is enforced.
 *
 * @return BOOLEAN TRUE if Control Flow Guard is enforced, FALSE otherwise.
 */
FORCEINLINE
BOOLEAN
NTAPI
LdrControlFlowGuardEnforced(
    VOID
    )
{
    return LdrSystemDllInitBlock.CfgBitMap && (LdrSystemDllInitBlock.Flags & 1) == 0;
}
#else
// rev
/**
 * The LdrControlFlowGuardEnforced function checks if Control Flow Guard is enforced.
 *
 * @return BOOLEAN TRUE if Control Flow Guard is enforced, FALSE otherwise.
 */
NTSYSAPI
BOOLEAN
NTAPI
LdrControlFlowGuardEnforced(
    VOID
    );
#endif
#endif

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
/**
 * The LdrControlFlowGuardEnforcedWithExportSuppression function checks if Control Flow Guard is
 * enforced with export suppression.
 *
 * @return BOOLEAN TRUE if Control Flow Guard is enforced, FALSE otherwise.
 */
FORCEINLINE
BOOLEAN
NTAPI
LdrControlFlowGuardEnforcedWithExportSuppression(
    VOID
    )
{
    return LdrSystemDllInitBlock.CfgBitMap
        && (LdrSystemDllInitBlock.Flags & 1) == 0
        && (LdrSystemDllInitBlock.MitigationOptionsMap.Map[0] & 3) == 3; // PROCESS_CREATION_MITIGATION_POLICY_CONTROL_FLOW_GUARD_EXPORT_SUPPRESSION
}
#endif

#if (PHNT_VERSION >= PHNT_WINDOWS_10_19H1)
// rev
NTSYSAPI
BOOLEAN
NTAPI
LdrIsModuleSxsRedirected(
    _In_ PVOID DllHandle
    );
#endif

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
// rev
NTSYSAPI
NTSTATUS
NTAPI
LdrUpdatePackageSearchPath(
    _In_ PCWSTR SearchPath
    );
#endif

// rev
#define ENCLAVE_STATE_CREATED         0x00000000ul // LdrpCreateSoftwareEnclave initial state
#define ENCLAVE_STATE_INITIALIZED     0x00000001ul // ZwInitializeEnclave successful (LdrInitializeEnclave)
#define ENCLAVE_STATE_INITIALIZED_VBS 0x00000002ul // only for ENCLAVE_TYPE_VBS (LdrInitializeEnclave)

// rev
typedef struct _LDR_SOFTWARE_ENCLAVE
{
    LIST_ENTRY Links; // ntdll!LdrpEnclaveList
    RTL_CRITICAL_SECTION CriticalSection;
    ULONG EnclaveType; // ENCLAVE_TYPE_*
    LONG ReferenceCount;
    ULONG EnclaveState; // ENCLAVE_STATE_*
    PVOID BaseAddress;
    SIZE_T Size;
    PVOID PreviousBaseAddress;
    LIST_ENTRY Modules; // LDR_DATA_TABLE_ENTRY.InLoadOrderLinks
    PLDR_DATA_TABLE_ENTRY PrimaryModule;
    PLDR_DATA_TABLE_ENTRY BCryptModule;
    PLDR_DATA_TABLE_ENTRY BCryptPrimitivesModule;
} LDR_SOFTWARE_ENCLAVE, *PLDR_SOFTWARE_ENCLAVE;

#if (PHNT_VERSION >= PHNT_WINDOWS_10)

// rev from CreateEnclave
/**
 * Creates a new uninitialized enclave. An enclave is an isolated region of code and data within the address space for an application. Only code that runs within the enclave can access data within the same enclave.
 *
 * @param ProcessHandle A handle to the process for which you want to create an enclave.
 * @param BaseAddress The preferred base address of the enclave. Specify NULL to have the operating system assign the base address.
 * @param Reserved Reserved.
 * @param Size The size of the enclave that you want to create, including the size of the code that you will load into the enclave, in bytes.
 * @param InitialCommitment The amount of memory to commit for the enclave, in bytes. This parameter is not used for virtualization-based security (VBS) enclaves.
 * @param EnclaveType The architecture type of the enclave that you want to create. To verify that an enclave type is supported, call IsEnclaveTypeSupported.
 * @param EnclaveInformation A pointer to the architecture-specific information to use to create the enclave.
 * @param EnclaveInformationLength The length of the structure that the EnclaveInformation parameter points to, in bytes.
 * For the ENCLAVE_TYPE_SGX and ENCLAVE_TYPE_SGX2 enclave types, this value must be 4096. For the ENCLAVE_TYPE_VBS enclave type, this value must be sizeof(ENCLAVE_CREATE_INFO_VBS), which is 36 bytes.
 * @param EnclaveError An optional pointer to a variable that receives an enclave error code that is architecture-specific.
 * @return NTSTATUS Successful or errant status.
 * @remarks https://learn.microsoft.com/en-us/windows/win32/api/enclaveapi/nf-enclaveapi-createenclave
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrCreateEnclave(
    _In_ HANDLE ProcessHandle,
    _Inout_ PVOID* BaseAddress,
    _In_ ULONG Reserved,
    _In_ SIZE_T Size,
    _In_ SIZE_T InitialCommitment,
    _In_ ULONG EnclaveType,
    _In_reads_bytes_(EnclaveInformationLength) PVOID EnclaveInformation,
    _In_ ULONG EnclaveInformationLength,
    _Out_ PULONG EnclaveError
    );

// rev from InitializeEnclave
/**
 * Initializes an enclave that you created and loaded with data.
 *
 * @param ProcessHandle A handle to the process for which the enclave was created.
 * @param BaseAddress Any address within the enclave.
 * @param EnclaveInformation A pointer to the architecture-specific information to use to initialize the enclave.
 * @param EnclaveInformationLength The length of the structure that the EnclaveInformation parameter points to, in bytes.
 * For the ENCLAVE_TYPE_SGX and ENCLAVE_TYPE_SGX2 enclave types, this value must be 4096. For the ENCLAVE_TYPE_VBS enclave type, this value must be sizeof(ENCLAVE_CREATE_INFO_VBS), which is 36 bytes.
 * @param EnclaveError An optional pointer to a variable that receives an enclave error code that is architecture-specific.
 * @return NTSTATUS Successful or errant status.
 * @remarks https://learn.microsoft.com/en-us/windows/win32/api/enclaveapi/nf-enclaveapi-initializeenclave
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrInitializeEnclave(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_reads_bytes_(EnclaveInformationLength) PVOID EnclaveInformation,
    _In_ ULONG EnclaveInformationLength,
    _Out_ PULONG EnclaveError
    );

// rev from DeleteEnclave
/**
 * Deletes the specified enclave.
 *
 * @param BaseAddress The base address of the enclave that you want to delete.
 * @return NTSTATUS Successful or errant status.
 * @remarks https://learn.microsoft.com/en-us/windows/win32/api/enclaveapi/nf-enclaveapi-deleteenclave
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrDeleteEnclave(
    _In_ PVOID BaseAddress
    );

// rev from CallEnclave
/**
 * Calls a function within an enclave. LdrCallEnclave can also be called within an enclave to call a function outside of the enclave.
 *
 * @param Routine The address of the function that you want to call.
 * @param Flags The flags to modify the call function.
 * @param RoutineParamReturn The parameter than you want to pass to the function.
 * @return NTSTATUS Successful or errant status.
 * @remarks https://learn.microsoft.com/en-us/windows/win32/api/enclaveapi/nf-enclaveapi-callenclave
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrCallEnclave(
    _In_ PENCLAVE_ROUTINE Routine,
    _In_ ULONG Flags, // ENCLAVE_CALL_FLAG_*
    _Inout_ PVOID* RoutineParamReturn
    );

// rev from LoadEnclaveImage
/**
 * Loads an image and all of its imports into an enclave.
 *
 * @param BaseAddress The base address of the image into which to load the image.
 * @param DllPath A NULL-terminated string that contains the path of the image to load.
 * @param DllName A NULL-terminated string that contains the name of the image to load.
 * @return NTSTATUS Successful or errant status.
 * @remarks https://learn.microsoft.com/en-us/windows/win32/api/enclaveapi/nf-enclaveapi-loadenclaveimagew
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrLoadEnclaveModule(
    _In_ PVOID BaseAddress,
    _In_opt_ PCWSTR DllPath,
    _In_ PCUNICODE_STRING DllName
    );

#endif // (PHNT_VERSION >= PHNT_WINDOWS_10)

/**
 * This function forcefully terminates the calling program if it is invoked inside a loader callout. Otherwise, it has no effect.
 *
 * @remarks This routine does not catch all potential deadlock cases; it is possible for a thread inside a loader callout
 * to acquire a lock while some thread outside a loader callout holds the same lock and makes a call into the loader.
 * In other words, there can be a lock order inversion between the loader lock and a client lock.
 * https://learn.microsoft.com/en-us/windows/win32/devnotes/ldrfastfailinloadercallout
 */
NTSYSAPI
VOID
NTAPI
LdrFastFailInLoaderCallout(
    VOID
    );

NTSYSAPI
BOOLEAN
NTAPI
LdrFlushAlternateResourceModules(
    VOID
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
LdrDllRedirectionCallback(
    _In_ ULONG Flags,
    _In_ PCWSTR DllName,
    _In_opt_ PCWSTR DllPath,
    _Inout_opt_ PULONG DllCharacteristics,
    _In_ PVOID CallbackData,
    _Out_ PCWSTR *EffectiveDllPath
    );

// rev
NTSYSAPI
VOID
NTAPI
LdrSetDllManifestProber(
    _In_ PVOID Routine
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
NTSYSAPI BOOLEAN LdrpChildNtdll; // DATA export
#endif

// rev
NTSYSAPI
VOID
NTAPI
LdrpResGetMappingSize(
    _In_ PVOID BaseAddress,
    _Out_ PSIZE_T Size,
    _In_ ULONG Flags,
    _In_ BOOLEAN GetFileSizeFromLoadAsDataTable
    );

#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

#endif
