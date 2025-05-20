/*
 * Loader support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTLDR_H
#define _NTLDR_H

typedef struct _ACTIVATION_CONTEXT *PACTIVATION_CONTEXT;

//
// DLLs
//

// private
typedef _Function_class_(DLL_INIT_ROUTINE)
BOOLEAN NTAPI DLL_INIT_ROUTINE(
    _In_ PVOID DllHandle,
    _In_ ULONG Reason,
    _In_opt_ PVOID Context // PCONTEXT
    );
typedef DLL_INIT_ROUTINE* PDLL_INIT_ROUTINE;

// private
typedef struct _LDR_SERVICE_TAG_RECORD
{
    struct _LDR_SERVICE_TAG_RECORD* Next;
    ULONG ServiceTag;
} LDR_SERVICE_TAG_RECORD, *PLDR_SERVICE_TAG_RECORD;

// private
typedef struct _LDRP_CSLIST
{
    PSINGLE_LIST_ENTRY Tail;
} LDRP_CSLIST, *PLDRP_CSLIST;

// private
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

// private
typedef struct _LDR_DDAG_NODE
{
    LIST_ENTRY Modules;
    PLDR_SERVICE_TAG_RECORD ServiceTagList;
    ULONG LoadCount;
    ULONG LoadWhileUnloadingCount; // ReferenceCount before THRESHOLD
    ULONG LowestLink; // DependencyCount before THRESHOLD
    union
    {
        LDRP_CSLIST Dependencies;
        SINGLE_LIST_ENTRY RemovalLink; // before THRESHOLD
    };
    LDRP_CSLIST IncomingDependencies;
    LDR_DDAG_STATE State;
    SINGLE_LIST_ENTRY CondenseLink;
    ULONG PreorderNumber;
} LDR_DDAG_NODE, *PLDR_DDAG_NODE;

// private
typedef struct _LDRP_DEPENDENCY
{
    SINGLE_LIST_ENTRY Link;
    PLDR_DDAG_NODE ChildNode;
    SINGLE_LIST_ENTRY BackLink;
    union
    {
        PLDR_DDAG_NODE ParentNode;
        struct
        {
            ULONG ForwarderLink : 1;
            ULONG SpareFlags : 2;
        };
    };
} LDRP_DEPENDENCY, *PLDRP_DEPENDENCY;

// private
typedef enum _LDR_DLL_LOAD_REASON
{
    LoadReasonUnknown = -1,
    LoadReasonStaticDependency = 0,
    LoadReasonStaticForwarderDependency = 1,
    LoadReasonDynamicForwarderDependency = 2,
    LoadReasonDelayloadDependency = 3,
    LoadReasonDynamicLoad = 4,
    LoadReasonAsImageLoad = 5,
    LoadReasonAsDataLoad = 6,
    LoadReasonEnclavePrimary = 7, // since REDSTONE3
    LoadReasonEnclaveDependency = 8,
    LoadReasonPatchImage = 9, // since WIN11
} LDR_DLL_LOAD_REASON, *PLDR_DLL_LOAD_REASON;

// private
typedef enum _RTLP_SEARCH_PATH_ELEMENT
{
    RtlpSearchPathDllDir = 0,
    RtlpSearchPathAppDir = 1,
    RtlpSearchPathSystemDirs = 2,
    RtlpSearchPathEnvPath = 3,
    RtlpSearchPathCurDir = 4,
    RtlpSearchPathDllLoadDir = 5,
    RtlpSearchPathUserDirs = 6,
    RtlpSearchPathSystem32 = 7,
    RtlpSearchPathPackageDirs = 8,
    MaxBasepSearchPath = 9,
} RTLP_SEARCH_PATH_ELEMENT;

// private
typedef struct _LDRP_PATH_ELEMENTS
{
    RTLP_SEARCH_PATH_ELEMENT ElementType[6];
    PWSTR ElementPointers[6];
    USHORT ElementCount;
} LDRP_PATH_ELEMENTS, *PLDRP_PATH_ELEMENTS;

// private
typedef struct _LDRP_DLL_PATH
{
    PWSTR DllPath;
    PWSTR DllPathCurDirCheckSafe;
    PWSTR PackageDirectories;
    ULONG DllPathOptions;
    PWSTR RootDllName;
    LDRP_PATH_ELEMENTS PathElements;
    ULONG DllPathSize;
    BOOLEAN NeedsToBeReleased;
} LDRP_DLL_PATH, *PLDRP_DLL_PATH;

// private
typedef struct _LDRP_LOAD_CONTEXT
{
    UNICODE_STRING ModuleName;
    PLDRP_DLL_PATH DllPath;
    ULONG LoadFlags;
    LDR_DLL_LOAD_REASON LoadReason;
    PNTSTATUS LoadStatus;
    struct _LDR_DATA_TABLE_ENTRY* ParentModule;
    struct _LDR_DATA_TABLE_ENTRY* Module;
    LIST_ENTRY WorkLink;
    struct _LDR_DATA_TABLE_ENTRY* PendingModule;
    struct _LDR_DATA_TABLE_ENTRY** ImportArray;
    ULONG ImportCount;
    ULONG UnmappedChildCount;
    PVOID IATBase;
    SIZE_T IATSize;
    ULONG ImportIndex;
    ULONG ThunkIndex;
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptors;
    ULONG IATProtection;
    PVOID GuardCheckICall;
    PVOID* GuardCheckICallFptr;
} LDRP_LOAD_CONTEXT, *PLDRP_LOAD_CONTEXT;

// private
typedef enum _LDR_HOT_PATCH_STATE
{
    LdrHotPatchBaseImage = 0,
    LdrHotPatchNotApplied = 1,
    LdrHotPatchAppliedReverse = 2,
    LdrHotPatchAppliedForward = 3,
    LdrHotPatchFailedToPatch = 4,
    LdrHotPatchStateMax = 5,
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
#define LDRP_VERIFIER_PROVIDER 0x00000400 // reserved before WIN11 24H2
#define LDRP_SHIM_ENGINE_CALLOUT_SENT 0x00000800 // reserved before WIN11 24H2
#define LDRP_LOAD_IN_PROGRESS 0x00001000
#define LDRP_LOAD_CONFIG_PROCESSED 0x00002000 // reserved before THRESHOLD
#define LDRP_ENTRY_PROCESSED 0x00004000
#define LDRP_PROTECT_DELAY_LOAD 0x00008000 // reserved before WINBLUE
#define LDRP_AUX_IAT_COPY_PRIVATE 0x00010000 // reserved before WIN11 24H2
#define LDRP_DONT_CALL_FOR_THREADS 0x00040000
#define LDRP_PROCESS_ATTACH_CALLED 0x00080000
#define LDRP_PROCESS_ATTACH_FAILED 0x00100000
#define LDRP_SCP_IN_EXCEPTION_TABLE 0x00200000 // LDRP_COR_DEFERRED_VALIDATE before WIN11 24H2
#define LDRP_COR_IMAGE 0x00400000
#define LDRP_DONT_RELOCATE 0x00800000
#define LDRP_COR_IL_ONLY 0x01000000
#define LDRP_CHPE_IMAGE 0x02000000 // reserved before REDSTONE4
#define LDRP_CHPE_EMULATOR_IMAGE 0x04000000 // reserved before WIN11
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
    PDLL_INIT_ROUTINE EntryPoint;
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
            ULONG VerifierProvider : 1; // since WIN11 24H2
            ULONG ShimEngineCalloutSent : 1; // since WIN11 24H2
            ULONG LoadInProgress : 1;
            ULONG LoadConfigProcessed : 1; // since THRESHOLD
            ULONG EntryProcessed : 1;
            ULONG ProtectDelayLoad : 1; // since WINBLUE
            ULONG AuxIatCopyPrivate : 1; // since WIN11 24H2
            ULONG ReservedFlags3 : 1;
            ULONG DontCallForThreads : 1;
            ULONG ProcessAttachCalled : 1;
            ULONG ProcessAttachFailed : 1;
            ULONG ScpInExceptionTable : 1; // CorDeferredValidate before WIN11 24H2
            ULONG CorImage : 1;
            ULONG DontRelocate : 1;
            ULONG CorILOnly : 1;
            ULONG ChpeImage : 1; // since REDSTONE4
            ULONG ChpeEmulatorImage : 1; // since WIN11
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
    LDR_DLL_LOAD_REASON LoadReason;
    ULONG ImplicitPathOptions; // since WINBLUE
    ULONG ReferenceCount; // since THRESHOLD
    ULONG DependentLoadFlags; // since REDSTONE
    UCHAR SigningLevel; // since REDSTONE2
    ULONG CheckSum; // since WIN11
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

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrLoadDll(
    _In_opt_ PCWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PCUNICODE_STRING DllName,
    _Out_ PVOID* DllHandle
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrUnloadDll(
    _In_ PVOID DllHandle
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllHandle(
    _In_opt_ PCWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PCUNICODE_STRING DllName,
    _Out_ PVOID* DllHandle
    );

// private
#define LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT 0x00000001
#define LDR_GET_DLL_HANDLE_EX_PIN 0x00000002

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllHandleEx(
    _In_ ULONG Flags, // LDR_GET_DLL_HANDLE_EX_*
    _In_opt_ PCWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PCUNICODE_STRING DllName,
    _Out_ PVOID* DllHandle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_7)

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllHandleByMapping(
    _In_ PVOID MappedBase,
    _Out_ PVOID* DllHandle
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllHandleByName(
    _In_opt_ PCUNICODE_STRING BaseDllName,
    _In_opt_ PCUNICODE_STRING FullDllName,
    _Out_ PVOID* DllHandle
    );

#endif // (PHNT_VERSION >= PHNT_WINDOWS_7)

#if (PHNT_VERSION >= PHNT_WINDOWS_8)

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllFullName(
    _In_opt_ PVOID DllHandle,
    _Out_ PUNICODE_STRING FileName
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllPath(
    _In_  PCWSTR RootDllName,
    _In_  ULONG  DllSearchOptions, // LOAD_LIBRARY_SEARCH_*
    _Out_ PWSTR* DllPath,
    _Out_ PWSTR* PackageDirectories
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllDirectory(
    _Out_ PUNICODE_STRING Path
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrSetDllDirectory(
    _In_ PCUNICODE_STRING Path
    );

#endif // (PHNT_VERSION >= PHNT_WINDOWS_8)

// private
#define LDR_ADDREF_DLL_PIN 0x00000001

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrAddRefDll(
    _In_ ULONG Flags, // LDR_ADDREF_DLL_*
    _In_ PVOID DllHandle
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrGetProcedureAddress(
    _In_ PVOID DllHandle,
    _In_opt_ PCANSI_STRING ProcedureName,
    _In_opt_ ULONG ProcedureNumber,
    _Out_ PVOID* ProcedureAddress
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
    _Out_ PVOID* ProcedureAddress,
    _In_ ULONG Flags // LDR_GET_PROCEDURE_ADDRESS_*
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrGetKnownDllSectionHandle(
    _In_ PCWSTR DllName,
    _In_ BOOLEAN SearchKnownDlls32,
    _Out_ PHANDLE SectionHandle
    );

#endif // (PHNT_VERSION >= PHNT_WINDOWS_VISTA)

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// private
NTSYSAPI
NTSTATUS
NTAPI
LdrGetProcedureAddressForCaller(
    _In_ PVOID DllHandle,
    _In_opt_ PCANSI_STRING ProcedureName,
    _In_opt_ ULONG ProcedureNumber,
    _Out_ PVOID* ProcedureAddress,
    _In_ ULONG Flags, // LDR_GET_PROCEDURE_ADDRESS_*
    _In_ PVOID CallerAddress
    );
#endif

// private
#define LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS 0x00000001
#define LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY 0x00000002

// private
#define LDR_LOCK_LOADER_LOCK_DISPOSITION_INVALID 0
#define LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_ACQUIRED 1
#define LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_NOT_ACQUIRED 2

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrLockLoaderLock(
    _In_ ULONG Flags, // LDR_LOCK_LOADER_LOCK_FLAG_*
    _Out_opt_ PULONG Disposition,
    _Out_ PVOID* Cookie
    );

// private
#define LDR_UNLOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS 0x00000001

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrUnlockLoaderLock(
    _In_ ULONG Flags, // LDR_UNLOCK_LOADER_LOCK_FLAG_*
    _In_ PVOID CookieIn
    );

// private
_Must_inspect_result_
_Maybenull_
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
// private
_Must_inspect_result_
_Maybenull_
NTSYSAPI
PIMAGE_BASE_RELOCATION
NTAPI
LdrProcessRelocationBlockEx(
    _In_ USHORT ImageMachine, // IMAGE_FILE_MACHINE_AMD64|IMAGE_FILE_MACHINE_ARM|IMAGE_FILE_MACHINE_THUMB|IMAGE_FILE_MACHINE_ARMNT
    _In_ ULONG_PTR VA,
    _In_ ULONG SizeOfBlock,
    _In_ PUSHORT RelocationPtr,
    _In_ LONG_PTR Diff
    );
#endif

// private
typedef _Function_class_(LDR_IMPORT_MODULE_CALLBACK)
VOID NTAPI LDR_IMPORT_MODULE_CALLBACK(
    _In_ PVOID Parameter,
    _In_ PCSTR ModuleName
    );
typedef LDR_IMPORT_MODULE_CALLBACK* PLDR_IMPORT_MODULE_CALLBACK;

// private
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

// rev
#define LDR_VERIFY_IMAGE_FLAG_USE_CALLBACK 0x01
#define LDR_VERIFY_IMAGE_FLAG_USE_SECTION_INFO 0x02
#define LDR_VERIFY_IMAGE_FLAG_RETURN_IMAGE_CHARACTERISTICS 0x04

// private
typedef struct _LDR_VERIFY_IMAGE_INFO
{
    ULONG Size;
    ULONG Flags; // LDR_VERIFY_IMAGE_FLAG_* 
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
    _Out_writes_to_(*BufferSize, *BufferSize) PULONG ServiceTagBuffer,
    _Inout_ PULONG BufferSize
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_VISTA)

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
    _Out_ PVOID* Cookie
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
#endif // (PHNT_VERSION >= PHNT_WINDOWS_VISTA)

// end_msdn

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// private
NTSYSAPI
VOID
NTAPI
LdrStandardizeSystemPath(
    _In_ PCUNICODE_STRING DllFullPath
    );
#endif

// private
typedef struct _LDR_FAILURE_DATA
{
    NTSTATUS Status;
    WCHAR DllName[0x20];
    WCHAR AdditionalInfo[0x20];
} LDR_FAILURE_DATA, *PLDR_FAILURE_DATA;

#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)
// private
NTSYSAPI
PLDR_FAILURE_DATA
NTAPI
LdrGetFailureData(
    VOID
    );
#endif

// WIN8 to REDSTONE
typedef struct _PS_MITIGATION_OPTIONS_MAP_V1
{
    ULONG64 Map[1];
} PS_MITIGATION_OPTIONS_MAP_V1, *PPS_MITIGATION_OPTIONS_MAP_V1;

// private // REDSTONE2 to 19H2
typedef struct _PS_MITIGATION_OPTIONS_MAP_V2
{
    ULONG64 Map[2];
} PS_MITIGATION_OPTIONS_MAP_V2, *PPS_MITIGATION_OPTIONS_MAP_V2;

// private // since 20H1
typedef struct _PS_MITIGATION_OPTIONS_MAP_V3
{
    ULONG64 Map[3];
} PS_MITIGATION_OPTIONS_MAP_V3, *PPS_MITIGATION_OPTIONS_MAP_V3;

typedef PS_MITIGATION_OPTIONS_MAP_V3
    PS_MITIGATION_OPTIONS_MAP, *PPS_MITIGATION_OPTIONS_MAP;

// private // REDSTONE3 to 19H2
typedef struct _PS_MITIGATION_AUDIT_OPTIONS_MAP_V2
{
    ULONG64 Map[2];
} PS_MITIGATION_AUDIT_OPTIONS_MAP_V2, *PPS_MITIGATION_AUDIT_OPTIONS_MAP_V2;

// private // since 20H1
typedef struct _PS_MITIGATION_AUDIT_OPTIONS_MAP_V3
{
    ULONG64 Map[3];
} PS_MITIGATION_AUDIT_OPTIONS_MAP_V3, *PPS_MITIGATION_AUDIT_OPTIONS_MAP_V3,
    PS_MITIGATION_AUDIT_OPTIONS_MAP, *PPS_MITIGATION_AUDIT_OPTIONS_MAP;

// private // WIN8 to REDSTONE
typedef struct _PS_SYSTEM_DLL_INIT_BLOCK_V1
{
    ULONG Size;
    ULONG SystemDllWowRelocation;
    ULONG64 SystemDllNativeRelocation;
    ULONG Wow64SharedInformation[16]; // use WOW64_SHARED_INFORMATION as index
    ULONG RngData;
    union
    {
        ULONG Flags;
        struct
        {
            ULONG CfgOverride : 1; // since REDSTONE
            ULONG Reserved : 31;
        };
    };
    ULONG64 MitigationOptions;
    ULONG64 CfgBitMap; // since WINBLUE
    ULONG64 CfgBitMapSize;
    ULONG64 Wow64CfgBitMap; // since THRESHOLD
    ULONG64 Wow64CfgBitMapSize;
} PS_SYSTEM_DLL_INIT_BLOCK_V1, *PPS_SYSTEM_DLL_INIT_BLOCK_V1;

// RS2 - 19H2
typedef struct _PS_SYSTEM_DLL_INIT_BLOCK_V2
{
    ULONG Size;
    ULONG64 SystemDllWowRelocation;
    ULONG64 SystemDllNativeRelocation;
    ULONG64 Wow64SharedInformation[16]; // use WOW64_SHARED_INFORMATION as index
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
    PS_MITIGATION_OPTIONS_MAP_V2 MitigationOptionsMap;
    ULONG64 CfgBitMap;
    ULONG64 CfgBitMapSize;
    ULONG64 Wow64CfgBitMap;
    ULONG64 Wow64CfgBitMapSize;
    PS_MITIGATION_AUDIT_OPTIONS_MAP_V2 MitigationAuditOptionsMap; // since REDSTONE3
} PS_SYSTEM_DLL_INIT_BLOCK_V2, *PPS_SYSTEM_DLL_INIT_BLOCK_V2;

// private // since 20H1
typedef struct _PS_SYSTEM_DLL_INIT_BLOCK_V3
{
    ULONG Size;
    ULONG64 SystemDllWowRelocation; // effectively since WIN8
    ULONG64 SystemDllNativeRelocation;
    ULONG64 Wow64SharedInformation[16]; // use WOW64_SHARED_INFORMATION as index
    ULONG RngData;
    union
    {
        ULONG Flags;
        struct
        {
            ULONG CfgOverride : 1; // effectively since REDSTONE
            ULONG Reserved : 31;
        };
    };
    PS_MITIGATION_OPTIONS_MAP_V3 MitigationOptionsMap;
    ULONG64 CfgBitMap; // effectively since WINBLUE
    ULONG64 CfgBitMapSize;
    ULONG64 Wow64CfgBitMap; // effectively since THRESHOLD
    ULONG64 Wow64CfgBitMapSize;
    PS_MITIGATION_AUDIT_OPTIONS_MAP_V3 MitigationAuditOptionsMap; // effectively since REDSTONE3
    ULONG64 ScpCfgCheckFunction; // since 24H2
    ULONG64 ScpCfgCheckESFunction;
    ULONG64 ScpCfgDispatchFunction;
    ULONG64 ScpCfgDispatchESFunction;
    ULONG64 ScpArm64EcCallCheck;
    ULONG64 ScpArm64EcCfgCheckFunction;
    ULONG64 ScpArm64EcCfgCheckESFunction;
} PS_SYSTEM_DLL_INIT_BLOCK_V3, *PPS_SYSTEM_DLL_INIT_BLOCK_V3,
    PS_SYSTEM_DLL_INIT_BLOCK, *PPS_SYSTEM_DLL_INIT_BLOCK;

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// private
NTSYSAPI PS_SYSTEM_DLL_INIT_BLOCK LdrSystemDllInitBlock;
#endif

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
    _In_opt_ PCWSTR FilePath,
    _In_ SIZE_T Size,
    _In_ HANDLE Handle,
    _In_opt_ PACTIVATION_CONTEXT AssociatedActCtx
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrRemoveLoadAsDataTable(
    _In_ PVOID InitModule,
    _Out_ PVOID* BaseModule,
    _Out_opt_ PSIZE_T Size,
    _In_ ULONG Flags
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrGetFileNameFromLoadAsDataTable(
    _In_ PVOID Module,
    _Out_ PWSTR* FilePath
    );

#endif // (PHNT_VERSION >= PHNT_WINDOWS_VISTA)

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrDisableThreadCalloutsForDll(
    _In_ PVOID DllHandle
    );

//
// Resources
//

// private
/**
 * The LdrAccessResource function returns a pointer to the first byte of the specified resource in memory.
 *
 * @param DllHandle A handle to the DLL.
 * @param ResourceDataEntry The resource information block.
 * @param Address The pointer to the specified resource in memory.
 * @param Size The size, in bytes, of the specified resource.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadresource
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrAccessResource(
    _In_ PVOID DllHandle,
    _In_ PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry,
    _Out_opt_ PVOID* Address,
    _Out_opt_ PULONG Size
    );

// N.B. Internally, resource-searching functions like LdrFindResource_U
// use an unnamed array of ULONG_PTR values to identify a resource,
// where elements form a path and have different meaning at each index.
// We introduce a union to help interpreting them. (diversenok)

// rev // LdrpMUIEtwOutput
typedef enum _LDR_RESOURCE_INFO_INDEX
{
    LdrResourceIdType = 0,
    LdrResourceIdName = 1,
    LdrResourceIdLanguage = 2,
    LdrResourceIdItem = 3,
    LdrResourceIdCount = 4
} LDR_RESOURCE_INFO_INDEX;

// rev // The number of elements required when using a given field, for ResourceIdPathLength
#define LDR_RESOURCE_INFO_LENGTH_THROUGH_TYPE (LdrResourceIdType + 1)
#define LDR_RESOURCE_INFO_LENGTH_THROUGH_NAME (LdrResourceIdName + 1)
#define LDR_RESOURCE_INFO_LENGTH_THROUGH_LANGUAGE (LdrResourceIdLanguage + 1)
#define LDR_RESOURCE_INFO_LENGTH_THROUGH_ITEM (LdrResourceIdItem + 1)

// rev // A union for unpacking the ResourceIdPath array
typedef union _LDR_RESOURCE_INFO
{
    ULONG_PTR ResourceIdPath[LdrResourceIdCount];
    struct
    {
        PCWSTR Type; // RT_*
        PCWSTR Name; // string or MAKEINTRESOURCE
        PCWSTR Language; // string or LANGID
        ULONG_PTR Item; // e.g., MessageId
    };
} LDR_RESOURCE_INFO, *PLDR_RESOURCE_INFO;

// private
/**
 * The LdrFindResource_U function determines the location of a resource in a DLL.
 *
 * @param DllHandle A handle to the DLL.
 * @param ResourceIdPath The path to the specified resource.
 * @param ResourceIdPathLength The number of elements in the path.
 * @param ResourceDataEntry The resource information block.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-findresourceexw
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrFindResource_U(
    _In_ PVOID DllHandle,
    _In_reads_(ResourceIdPathLength) PULONG_PTR ResourceIdPath, // PLDR_RESOURCE_INFO
    _In_ ULONG ResourceIdPathLength,
    _Out_ PIMAGE_RESOURCE_DATA_ENTRY* ResourceDataEntry
    );

// private
#define LDR_FIND_RESOURCE_DATA 0x00000000
#define LDR_FIND_RESOURCE_DIRECTORY 0x00000002
#define LDR_FIND_RESOURCE_LANGUAGE_CAN_FALLBACK 0x00000000
#define LDR_FIND_RESOURCE_LANGUAGE_EXACT 0x00000004
#define LDR_FIND_RESOURCE_LANGUAGE_REDIRECT_VERSION 0x00000008

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrFindResourceEx_U(
    _In_ ULONG Flags, // LDR_FIND_RESOURCE_*
    _In_ PVOID DllHandle,
    _In_reads_(ResourceIdPathLength) PULONG_PTR ResourceIdPath, // PLDR_RESOURCE_INFO
    _In_ ULONG ResourceIdPathLength,
    _Out_ PIMAGE_RESOURCE_DATA_ENTRY* ResourceDataEntry
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrFindResourceDirectory_U(
    _In_ PVOID DllHandle,
    _In_reads_(ResourceIdPathLength) PULONG_PTR ResourceIdPath, // PLDR_RESOURCE_INFO
    _In_ ULONG ResourceIdPathLength,
    _Out_ PIMAGE_RESOURCE_DIRECTORY* ResourceDirectory
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)

// private
/**
 * The LdrResFindResource function finds a resource in a DLL.
 *
 * @param Module A handle to the DLL.
 * @param ResourceType The type of the resource.
 * @param ResourceName The name of the resource.
 * @param Language The language of the resource.
 * @param Resource An optional pointer to receive the resource buffer.
 * @param Size An optional pointer to receive the resource length.
 * @param FoundLanguage An optional buffer to receive the culture name.
 * @param FoundLanguageLength An optional pointer to receive the length of the culture name.
 * @param Flags Flags for the resource search.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrResFindResource(
    _In_ PVOID Module,
    _In_ PCWSTR ResourceType,
    _In_ PCWSTR ResourceName,
    _In_ PCWSTR Language,
    _Out_opt_ PVOID* Resource,
    _Out_opt_ PULONG Size,
    _Out_writes_bytes_opt_(*FoundLanguageLength) PWCHAR FoundLanguage, // WCHAR buffer[6]
    _Out_opt_ PULONG FoundLanguageLength,
    _In_ ULONG Flags
    );

// private
/**
 * The LdrResFindResourceDirectory function finds a resource directory in a DLL.
 *
 * @param DllHandle A handle to the DLL.
 * @param ResourceType The type of the resource.
 * @param ResourceName The name of the resource.
 * @param ResourceDirectory An optional pointer to receive the resource directory.
 * @param FoundLanguage An optional buffer to receive the culture name.
 * @param FoundLanguageLength An optional pointer to receive the length of the culture name.
 * @param Flags Flags for the resource search.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrResFindResourceDirectory(
    _In_ PVOID Module,
    _In_ PCWSTR ResourceType,
    _In_ PCWSTR ResourceName,
    _Out_opt_ PIMAGE_RESOURCE_DIRECTORY* ResourceDirectory,
    _Out_writes_bytes_opt_(*FoundLanguageLength) PWCHAR FoundLanguage, // WCHAR buffer[6]
    _Out_opt_ PULONG FoundLanguageLength,
    _In_ ULONG Flags
    );

#endif // (PHNT_VERSION >= PHNT_WINDOWS_VISTA)

// rev
#define LDR_GET_RESOURCE_DIRECTORY_RANGE_CHECKS 0x1000

#if (PHNT_VERSION >= PHNT_WINDOWS_7)
// private
NTSYSAPI
NTSTATUS
NTAPI
LdrpResGetResourceDirectory(
    _In_ PVOID ModuleBase,
    _In_ SIZE_T MappingSize,
    _In_ ULONG Flags, // LDR_GET_RESOURCE_DIRECTORY_*
    _Out_ PIMAGE_RESOURCE_DIRECTORY* TopResourceDirectory,
    _Out_ PIMAGE_NT_HEADERS* NtHeaders
    );
#endif

#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)
// private
/**
* The LdrResSearchResource function searches for a resource in a DLL.
*
* @param File A handle to the DLL.
* @param InitResIds A pointer to the resource information.
* @param InitResIdCount The level of the resource.
* @param Flags Flags for the resource search.
* @param Resource An optional pointer to receive the resource buffer.
* @param Size An optional pointer to receive the resource length.
* @param FoundLanguage An optional buffer to receive the culture name.
* @param FoundLanguageLength An optional pointer to receive the length of the culture name.
* @return NTSTATUS Successful or errant status.
*/
NTSYSAPI
NTSTATUS
NTAPI
LdrResSearchResource(
    _In_ PVOID File,
    _In_reads_(InitResIdCount) PULONG_PTR InitResIds, // PLDR_RESOURCE_INFO
    _In_ ULONG InitResIdCount,
    _In_ ULONG Flags,
    _Out_opt_ PVOID* Resource,
    _Out_opt_ PSIZE_T Size,
    _Out_writes_bytes_opt_(*FoundLanguageLength) PWCHAR FoundLanguage, // WCHAR buffer[6]
    _Out_opt_ PULONG FoundLanguageLength
    );
#endif

// rev
#define RC_CONFIG_SIGNATURE 0xFECDFECD

// private
typedef struct _RC_CONFIG
{
    ULONG Signature;
    ULONG Length;
    ULONG RCConfigVersion;
    ULONG FilePathType;
    ULONG FileType;
    ULONG SystemAttributes;
    ULONG UltimateFallbackLocation;
    ULONG ServiceCheckSum[4];
    ULONG Checksum[4];
    ULONG Reserved1;
    ULONG Reserved2;
    ULONG MUIFileNameOffset; // to WCHAR[]
    ULONG MUIFileNameLength;
    ULONG MUIFilePathOffset; // to WCHAR[]
    ULONG MUIFilePathLength;
    ULONG MainResNameTypesOffset; // to WCHAR[]
    ULONG MainResNameTypesLength;
    ULONG MainResIDTypesOffset; // to ULONG[]
    ULONG MainResIDTypesLength;
    ULONG MUIResNameTypesOffset; // to WCHAR[]
    ULONG MUIResNameTypesLength;
    ULONG MUIResIDTypesOffset; // to ULONG[]
    ULONG MUIResIDTypesLength;
    ULONG LanguageOffset; // to WCHAR[]
    ULONG LanguageLength;
    ULONG UltimateFallbackLanguageOffset; // to WCHAR[]
    ULONG UltimateFallbackLanguageLength;
} RC_CONFIG, *PRC_CONFIG;

// rev
#define LDR_GET_RC_CONFIG_DONT_QUERY_MAPPING_SIZE 0x2000

#if (PHNT_VERSION >= PHNT_WINDOWS_7)

// private
/**
 * The LdrResGetRCConfig function retrieves the RC configuration for a DLL.
 *
 * @param ModuleBase A handle to the DLL.
 * @param InitMappingSize The size of the mapping, when known.
 * @param RcConfig A variable that receives a pointer to the configuration.
 * @param Flags Flags for the operation.
 * @param IsCaching Indicates if an alternate resource should be loaded.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrResGetRCConfig(
    _In_ PVOID ModuleBase,
    _In_opt_ SIZE_T InitMappingSize,
    _Out_opt_ PRC_CONFIG* RcConfig,
    _In_ ULONG Flags, // LDR_GET_RC_CONFIG_*
    _In_ BOOLEAN IsCaching // LdrLoadAlternateResourceModule
    );

// rev
#define LDR_RC_CONFIG_NOT_IN_MUI 0x20000
#define LDR_RC_CONFIG_NOT_IN_MAIN 0x40000

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrRscIsTypeExist(
    _In_ PRC_CONFIG Config,
    _In_ PCWSTR ResType,
    _Reserved_ ULONG Flags,
    _Inout_ PULONG RetFlags // LDR_RC_CONFIG_*
    );

#endif // (PHNT_VERSION >= PHNT_WINDOWS_7)

#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)
// private
/**
 * The LdrResRelease function releases a resource in a DLL.
 *
 * @param File A handle to the DLL.
 * @param Language An optional culture name or ID.
 * @param Flags Flags for the operation.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrResRelease(
    _In_ PVOID File,
    _In_opt_ PWSTR Language, // MAKEINTRESOURCE
    _In_ ULONG Flags
    );
#endif

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

FORCEINLINE
ULONG_PTR
NTAPI
LdrNameOrIdFromResourceEntry(
    _In_ PIMAGE_RESOURCE_DIRECTORY ResourceDirectory,
    _In_ PIMAGE_RESOURCE_DIRECTORY_ENTRY Entry)
{
    if (Entry->NameIsString)
        return (ULONG_PTR)((ULONG_PTR)ResourceDirectory + (ULONG_PTR)Entry->NameOffset);
    else
        return (ULONG_PTR)Entry->Id;
}

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrEnumResources(
    _In_ PVOID DllHandle,
    _In_reads_(ResourceIdPathLength) PULONG_PTR ResourceIdPath, // PLDR_RESOURCE_INFO
    _In_ ULONG ResourceIdPathLength,
    _Inout_ PULONG NumberOfResources,
    _Out_writes_to_opt_(*NumberOfResources, *NumberOfResources) PLDR_ENUM_RESOURCE_ENTRY Resources
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrFindEntryForAddress(
    _In_ PVOID Address,
    _Out_ PLDR_DATA_TABLE_ENTRY* TableEntry
    );

// private
/**
 * Returns a handle to the language-specific dynamic-link library (DLL) resource module associated with a DLL that is already loaded for the calling process.
 *
 * \param Module A handle to the DLL module to search for a MUI resource. If the language-specific DLL for the MUI is available, loads the specified module into the address space of the calling process and returns a handle to the module.
 * \param ReturnAlternateModule The base address of the mapped view.
 * \param AlternateViewSize The size of the mapped view.
 * \param Flags Flags for the operation.
 * \return Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrLoadAlternateResourceModule(
    _In_ PVOID Module,
    _Out_ PVOID* ReturnAlternateModule,
    _Out_opt_ PSIZE_T AlternateViewSize,
    _In_ ULONG Flags
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)
// private
NTSYSAPI
NTSTATUS
NTAPI
LdrLoadAlternateResourceModuleEx(
    _In_ PVOID Module,
    _In_ LANGID LangId,
    _Out_ PVOID* ReturnAlternateModule,
    _Out_opt_ PSIZE_T AlternateViewSize,
    _In_ ULONG Flags
    );
#endif

// private
/**
 * Frees the language-specific dynamic-link library (DLL) resource module previously loaded by LdrLoadAlternateResourceModule function.
 *
 * @param Module The base address of the mapped view.
 * @return Successful or errant status.
 */
NTSYSAPI
BOOLEAN
NTAPI
LdrUnloadAlternateResourceModule(
    _In_ PVOID Module
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)
// private
NTSYSAPI
BOOLEAN
NTAPI
LdrUnloadAlternateResourceModuleEx(
    _In_ PVOID Module,
    _In_ LANGID LangId
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrSetMUICacheType(
    _In_ ULONG Flags
    );
#endif

#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

//
// Module information
//

// private
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

// private
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

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrQueryProcessModuleInformation(
    _Out_writes_bytes_to_(ModuleInformationLength, *ReturnLength) PRTL_PROCESS_MODULES ModuleInformation,
    _In_ ULONG ModuleInformationLength,
    _Out_ PULONG ReturnLength
    );

// private
typedef _Function_class_(LDR_LOADED_MODULE_ENUMERATION_CALLBACK_FUNCTION)
VOID NTAPI LDR_LOADED_MODULE_ENUMERATION_CALLBACK_FUNCTION(
    _In_ PLDR_DATA_TABLE_ENTRY DataTableEntry,
    _In_ PVOID Context,
    _Inout_ PBOOLEAN StopEnumeration
    );
typedef LDR_LOADED_MODULE_ENUMERATION_CALLBACK_FUNCTION* PLDR_LOADED_MODULE_ENUMERATION_CALLBACK_FUNCTION;

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrEnumerateLoadedModules(
    _Reserved_ ULONG Flags,
    _In_ PLDR_LOADED_MODULE_ENUMERATION_CALLBACK_FUNCTION CallbackFunction,
    _In_ PVOID Context
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)
// private
NTSYSAPI
NTSTATUS
NTAPI
LdrOpenImageFileOptionsKey(
    _In_ PCUNICODE_STRING ImagePathName,
    _In_ BOOLEAN Wow64Path,
    _Out_ PHANDLE KeyHandle
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrQueryImageFileKeyOption(
    _In_ HANDLE KeyHandle,
    _In_ PCWSTR OptionName,
    _In_ ULONG Type,
    _Out_writes_bytes_to_(BufferSize, *ResultSize) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG ResultSize
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_VISTA)

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrQueryImageFileExecutionOptions(
    _In_ PCUNICODE_STRING ImagePathName,
    _In_ PCWSTR OptionName,
    _In_ ULONG Type,
    _Out_writes_bytes_to_(BufferSize, *ResultSize) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG ResultSize
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)
// private
NTSYSAPI
NTSTATUS
NTAPI
LdrQueryImageFileExecutionOptionsEx(
    _In_ PCUNICODE_STRING ImagePathName,
    _In_ PCWSTR OptionName,
    _In_ ULONG Type,
    _Out_writes_bytes_to_(BufferSize, *ResultSize) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG ResultSize,
    _In_ BOOLEAN Wow64Path
    );
#endif

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

// private
typedef _Function_class_(DELAYLOAD_FAILURE_SYSTEM_ROUTINE)
PVOID NTAPI DELAYLOAD_FAILURE_SYSTEM_ROUTINE(
    _In_ PCSTR DllName,
    _In_ PCSTR ProcedureName
    );
typedef DELAYLOAD_FAILURE_SYSTEM_ROUTINE* PDELAYLOAD_FAILURE_SYSTEM_ROUTINE;

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// private
/**
 * Determines whether the specified function in a delay-loaded DLL is available on the system.
 *
 * @param ImportModuleBase A handle to the calling module. (NtCurrentImageBase)
 * @param ExportDllName The file name of the delay-loaded DLL that exports the specified function. This parameter is case-insensitive.
 * @param ExportProcName The address of a delay-load failure callback function for the specified DLL and process.
 * @param Flags Reserved; must be 0.
 * @return NTSTATUS Successful or errant status.
 * @remarks https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi2/nf-libloaderapi2-queryoptionaldelayloadedapi
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrQueryOptionalDelayLoadedAPI(
    _In_ PVOID ImportModuleBase,
    _In_ PCSTR ExportDllName,
    _In_ PCSTR ExportProcName,
    _Reserved_ ULONG Flags
    );

// private
/**
 * Locates the target function of the specified import and replaces the function pointer in the import thunk with the target of the function implementation.
 *
 * @param ImportModuleBase The address of the base of the module importing a delay-loaded function. (NtCurrentImageBase)
 * @param DelayloadDescriptor The address of the image delay import directory for the module to be loaded.
 * @param FailureDllHook The address of a delay-load failure callback function for the specified DLL and process.
 * @param FailureSystemHook The address of a delay-load failure callback function for the specified DLL and process.
 * @param ThunkAddress The thunk data for the target function. Used to find the specific name table entry of the function.
 * @param Flags Flags for the operation.
 * @return The address of the import, or the failure stub for it.
 * @remarks https://learn.microsoft.com/en-us/windows/win32/devnotes/resolvedelayloadedapi
 */
NTSYSAPI
PVOID
NTAPI
LdrResolveDelayLoadedAPI(
    _In_ PVOID ImportModuleBase,
    _In_ PCIMAGE_DELAYLOAD_DESCRIPTOR DelayloadDescriptor,
    _In_opt_ PDELAYLOAD_FAILURE_DLL_CALLBACK FailureDllHook,
    _In_opt_ PDELAYLOAD_FAILURE_SYSTEM_ROUTINE FailureSystemHook, // kernel32.DelayLoadFailureHook
    _Inout_ PIMAGE_THUNK_DATA ThunkAddress,
    _In_ ULONG Flags
    );

// private
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
    _In_ PVOID ImportModuleBase,
    _In_ PCSTR ExportDllName,
    _Reserved_ ULONG Flags
    );

// private
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
    _In_ ULONG DirectoryFlags // LOAD_LIBRARY_SEARCH_*
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_8)

// private
typedef struct _RTLP_DLL_DIRECTORY
{
    LIST_ENTRY Link;
    USHORT Length;
    _Field_size_bytes_(Length) WCHAR Path[ANYSIZE_ARRAY];
} RTLP_DLL_DIRECTORY, *PRTLP_DLL_DIRECTORY;

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// private
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
    _Out_ PRTLP_DLL_DIRECTORY* Cookie
    );

// private
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
    _In_ PRTLP_DLL_DIRECTORY Cookie
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_8)

// private
_Analysis_noreturn_
DECLSPEC_NORETURN
NTSYSAPI
VOID
NTAPI
LdrShutdownProcess(
    VOID
    );

// private
_Analysis_noreturn_
DECLSPEC_NORETURN
NTSYSAPI
VOID
NTAPI
LdrShutdownThread(
    VOID
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8_1)
// private
NTSYSAPI
NTSTATUS
NTAPI
LdrSetImplicitPathOptions(
    _In_ PVOID ModuleBase,
    _In_ ULONG SearchOptions
    );
#endif

#if (PHNT_VERSION >= PHNT_WINDOWS_10RS3)
// private
/**
 * The LdrControlFlowGuardEnforced function checks if Control Flow Guard is enforced.
 *
 * @return TRUE if Control Flow Guard is enforced, FALSE otherwise.
 */
NTSYSAPI
ULONG
NTAPI
LdrControlFlowGuardEnforced(
    VOID
    );
#endif

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS5)
// rev
NTSYSAPI
BOOLEAN
NTAPI
LdrIsModuleSxsRedirected(
    _In_ PVOID DllHandle
    );
#endif

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS2)
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

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS3)

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

#endif // (PHNT_VERSION >= PHNT_WINDOWS_10_RS3)

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
// private
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
#endif

// private
NTSYSAPI
BOOLEAN
NTAPI
LdrFlushAlternateResourceModules(
    VOID
    );

// private
typedef _Function_class_(LDR_MANIFEST_PROBER_ROUTINE)
NTSTATUS NTAPI LDR_MANIFEST_PROBER_ROUTINE(
    _In_ PVOID DllHandle,
    _In_ PCWSTR FullDllName,
    _Out_ PACTIVATION_CONTEXT* ActCtx
);
typedef LDR_MANIFEST_PROBER_ROUTINE* PLDR_MANIFEST_PROBER_ROUTINE;

// private
typedef _Function_class_(LDR_CREATE_ACT_CTX_LANGUAGE)
NTSTATUS NTAPI LDR_CREATE_ACT_CTX_LANGUAGE(
    _In_ PACTIVATION_CONTEXT ActCtxIn,
    _In_ LANGID LangId,
    _Out_ PACTIVATION_CONTEXT* ActCtxOut
    );
typedef LDR_CREATE_ACT_CTX_LANGUAGE* PLDR_CREATE_ACT_CTX_LANGUAGE;

// private
typedef _Function_class_(LDR_RELEASE_ACT_CTX)
NTSTATUS NTAPI LDR_RELEASE_ACT_CTX(
    _In_ PACTIVATION_CONTEXT ActCtx
    );
typedef LDR_RELEASE_ACT_CTX* PLDR_RELEASE_ACT_CTX;

// private
NTSYSAPI
VOID
NTAPI
LdrSetDllManifestProber(
    _In_ PLDR_MANIFEST_PROBER_ROUTINE ManifestProberRoutine,
    _In_ PLDR_CREATE_ACT_CTX_LANGUAGE CreateActCtxLanguage,
    _In_ PLDR_RELEASE_ACT_CTX ReleaseActCtx
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
// private
NTSYSAPI BOOLEAN LdrpChildNtdll; // DATA export
#endif

// rev
#define LDR_GET_MAPPING_SIZE_VALIDATE_ONLY 0x20000 // Check if the provided size is larger than the real mapping size

#if (PHNT_VERSION >= PHNT_WINDOWS_7)
// private
NTSYSAPI
VOID
NTAPI
LdrpResGetMappingSize(
    _In_ PVOID ModuleBase,
    _When_(Flags & LDR_GET_MAPPING_SIZE_VALIDATE_ONLY, _In_)
    _When_(!(Flags & LDR_GET_MAPPING_SIZE_VALIDATE_ONLY), _Out_)
        PSIZE_T MappingSize,
    _In_ ULONG Flags, // LDR_GET_MAPPING_SIZE_*
    _In_ BOOLEAN IsMUI
    );
#endif

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrInitShimEngineDynamic(
    _In_ PVOID ShimEngineModule,
    _In_ PCUNICODE_STRING ShimDllList
    );

#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

#endif
