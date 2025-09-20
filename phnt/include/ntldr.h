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

typedef _Function_class_(DLL_INIT_ROUTINE)
BOOLEAN NTAPI DLL_INIT_ROUTINE(
    _In_ PVOID DllHandle,
    _In_ ULONG Reason,
    _In_opt_ PVOID Context
    );
typedef DLL_INIT_ROUTINE* PDLL_INIT_ROUTINE;

// private
typedef struct _LDR_SERVICE_TAG_RECORD
{
    struct _LDR_SERVICE_TAG_RECORD *Next;
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
    ULONG LoadWhileUnloadingCount; // ReferenceCount before WIN10
    ULONG LowestLink; // DependencyCount before WIN10
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

// LoadReason
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

// HotPatchState
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
#define LDRP_VERIFIER_PROVIDER 0x00000400 // reserved before WIN11 24H2
#define LDRP_SHIM_ENGINE_CALLOUT_SENT 0x00000800 // reserved before WIN11 24H2
#define LDRP_LOAD_IN_PROGRESS 0x00001000
#define LDRP_LOAD_CONFIG_PROCESSED 0x00002000 // reserved before WIN10
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
    PVOID EntryPoint; // PDLL_INIT_ROUTINE
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
            ULONG VerifierProvider : 1; // 24H2
            ULONG ShimEngineCalloutSent : 1; // 24H2
            ULONG LoadInProgress : 1;
            ULONG LoadConfigProcessed : 1; // WIN10
            ULONG EntryProcessed : 1;
            ULONG ProtectDelayLoad : 1; // WINBLUE
            ULONG AuxIatCopyPrivate : 1; // 24H2
            ULONG ReservedFlags3 : 1;
            ULONG DontCallForThreads : 1;
            ULONG ProcessAttachCalled : 1;
            ULONG ProcessAttachFailed : 1;
            ULONG ScpInExceptionTable : 1; // CorDeferredValidate before 24H2
            ULONG CorImage : 1;
            ULONG DontRelocate : 1;
            ULONG CorILOnly : 1;
            ULONG ChpeImage : 1; // RS4
            ULONG ChpeEmulatorImage : 1; // WIN11
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
    ULONG ReferenceCount; // since WIN10
    ULONG DependentLoadFlags; // since RS1
    UCHAR SigningLevel; // since RS2
    ULONG CheckSum; // since WIN11
    PVOID ActivePatchImageBase;
    LDR_HOT_PATCH_STATE HotPatchState;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef const LDR_DATA_TABLE_ENTRY* PCLDR_DATA_TABLE_ENTRY;

#define LDR_IS_DATAFILE(DllHandle) (((ULONG_PTR)(DllHandle)) & (ULONG_PTR)1)
#define LDR_IS_IMAGEMAPPING(DllHandle) (((ULONG_PTR)(DllHandle)) & (ULONG_PTR)2)
#define LDR_IS_RESOURCE(DllHandle) (LDR_IS_IMAGEMAPPING(DllHandle) || LDR_IS_DATAFILE(DllHandle))
#define LDR_MAPPEDVIEW_TO_DATAFILE(BaseAddress) ((PVOID)(((ULONG_PTR)(BaseAddress)) | (ULONG_PTR)1))
#define LDR_MAPPEDVIEW_TO_IMAGEMAPPING(BaseAddress) ((PVOID)(((ULONG_PTR)(BaseAddress)) | (ULONG_PTR)2))
#define LDR_DATAFILE_TO_MAPPEDVIEW(DllHandle) ((PVOID)(((ULONG_PTR)(DllHandle)) & ~(ULONG_PTR)1))
#define LDR_IMAGEMAPPING_TO_MAPPEDVIEW(DllHandle) ((PVOID)(((ULONG_PTR)(DllHandle)) & ~(ULONG_PTR)2))

#if (PHNT_MODE != PHNT_MODE_KERNEL)

// rev LdrLoadDll DllCharacteristics
#define LDR_DONT_RESOLVE_DLL_REFERENCES       0x00000002 // IMAGE_FILE_EXECUTABLE_IMAGE maps to DONT_RESOLVE_DLL_REFERENCES
#define LDR_PACKAGED_LIBRARY                  0x00000004 // LOAD_PACKAGED_LIBRARY
#define LDR_REQUIRE_SIGNED_TARGET             0x00800000 // maps to LOAD_LIBRARY_REQUIRE_SIGNED_TARGET (requires /INTEGRITYCHECK)
#define LDR_OS_INTEGRITY_CONTINUITY           0x80000000 // maps to LOAD_LIBRARY_OS_INTEGRITY_CONTINUITY // since REDSTONE2
// rev LdrLoadDll DllPath
#define LDR_PATH_IS_FLAGS                     0x00000001
#define LDR_PATH_VALID_FLAGS                  0x00007F08
#define LDR_PATH_WITH_ALTERED_SEARCH_PATH     0x00000008 // LOAD_WITH_ALTERED_SEARCH_PATH
#define LDR_PATH_SEARCH_DLL_LOAD_DIR          0x00000100 // LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR
#define LDR_PATH_SEARCH_APPLICATION_DIR       0x00000200 // LOAD_LIBRARY_SEARCH_APPLICATION_DIR
#define LDR_PATH_SEARCH_USER_DIRS             0x00000400 // LOAD_LIBRARY_SEARCH_USER_DIRS
#define LDR_PATH_SEARCH_SYSTEM32              0x00000800 // LOAD_LIBRARY_SEARCH_SYSTEM32
#define LDR_PATH_SEARCH_DEFAULT_DIRS          0x00001000 // LOAD_LIBRARY_SEARCH_DEFAULT_DIRS
#define LDR_PATH_SAFE_CURRENT_DIRS            0x00002000 // LOAD_LIBRARY_SAFE_CURRENT_DIRS // since REDSTONE1
#define LDR_PATH_SEARCH_SYSTEM32_NO_FORWARDER 0x00004000 // LOAD_LIBRARY_SEARCH_SYSTEM32_NO_FORWARDER // since REDSTONE1

/**
 * The LdrLoadDll routine loads the specified DLL into the address space of the calling process.
 *
 * \param DllPath A pointer to a Unicode string specifying the search path for the DLL or a combination of LDR_PATH_* flags. If NULL, the default search order is used.
 * \param DllCharacteristics A pointer to a variable specifying DLL characteristics.
 * \param DllName A pointer to a UNICODE_STRING structure containing the name of the DLL to load.
 * \param DllHandle A pointer that receives the handle to module on success.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryexw
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrLoadDll(
    _In_opt_ PCWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PCUNICODE_STRING DllName,
    _Out_ PVOID *DllHandle
    );

/**
 * The LdrUnloadDll routine unloads the specified DLL from the address space of the calling process.
 *
 * \param DllHandle A handle to the DLL module to unload, as returned by LdrLoadDll.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-freelibrary
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrUnloadDll(
    _In_ PVOID DllHandle
    );

/**
 * The LdrGetDllHandle routine retrieves a handle to a module that is already loaded in the calling process.
 *
 * \param DllPath A pointer to a Unicode string specifying the search path for the DLL or a combination of LDR_PATH_* flags. If NULL, the default search order is used.
 * \param DllCharacteristics A pointer to a variable specifying DLL characteristics. Can be NULL.
 * \param DllName A pointer to a UNICODE_STRING structure containing the name of the DLL to find.
 * \param DllHandle A pointer that receives the handle to the loaded DLL module on success.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandleexw
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllHandle(
    _In_opt_ PCWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PCUNICODE_STRING DllName,
    _Out_ PVOID *DllHandle
    );

// LdrGetDllHandleEx Flags
#define LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT 0x00000001
#define LDR_GET_DLL_HANDLE_EX_PIN 0x00000002

/**
 * The LdrGetDllHandleEx routine retrieves a handle to a module that is already loaded in the calling process, with extended control over reference counting.
 *
 * \param Flags A combination of flags that control behavior:
 *  - LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT: Do not modify the module's reference count.
 *  - LDR_GET_DLL_HANDLE_EX_PIN: Pin the module so it cannot be unloaded for the lifetime of the process.
 * \param DllPath An optional semicolon-separated search path used to resolve DllName if needed. If NULL, the default module lookup is used.
 * \param DllCharacteristics Optional pointer to the DLL characteristics (same values accepted by LdrLoadDll). Typically NULL for lookups.
 * \param DllName The Unicode name of the module to find. Can be a base name (e.g., "ntdll.dll") or a fully-qualified path.
 * \param DllHandle Receives the module handle on success.
 * \return NTSTATUS Successful or errant status.
 */
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

// rev
/**
 * The LdrGetDllHandleByMapping routine retrieves a module handle for an image that is already loaded in the calling process, identified by base address.
 *
 * \param BaseAddress The base address of a mapped image (image or datafile view).
 * \param DllHandle Receives the module handle corresponding to the base address.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllHandleByMapping(
    _In_ PVOID BaseAddress,
    _Out_ PVOID *DllHandle
    );

// rev
/**
 * The LdrGetDllHandleByName routine retrieves a module handle by base name and/or full path for a DLL already loaded in the calling process.
 *
 * \param BaseDllName Optional base file name (e.g., "kernel32.dll"). Note: Matching is case-insensitive.
 * \param FullDllName Optional fully-qualified path of the module. Note: Matching is case-insensitive.
 * \param DllHandle Receives the module handle on success.
 * \return NTSTATUS Successful or errant status.
 * \remarks At least one of BaseDllName or FullDllName must be supplied. If both are supplied, they must refer to the same module.
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllHandleByName(
    _In_opt_ PCUNICODE_STRING BaseDllName,
    _In_opt_ PCUNICODE_STRING FullDllName,
    _Out_ PVOID *DllHandle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// rev
NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllFullName(
    _In_opt_ PVOID DllHandle,
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
/**
 * The LdrGetDllDirectory routine retrieves the application-specific portion of the search path used to locate DLLs for the application.
 *
 * \param PathName A pointer to a buffer that receives the application-specific portion of the search path.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getdlldirectoryw
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllDirectory(
    _Out_ PUNICODE_STRING PathName
    );

// rev
/**
 * The LdrSetDllDirectory routine adds a directory to the search path used to locate DLLs for the application.
 *
 * \param PathName The directory to be added to the search path. If this parameter is NULL, the function restores the default search order.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setdlldirectoryw
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrSetDllDirectory(
    _In_ PCUNICODE_STRING PathName
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_8)

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

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrGetProcedureAddressEx(
    _In_ PVOID DllHandle,
    _In_opt_ PCANSI_STRING ProcedureName,
    _In_opt_ ULONG ProcedureNumber,
    _Out_ PVOID *ProcedureAddress,
    _In_ ULONG Flags // LDR_GET_PROCEDURE_ADDRESS_*
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrGetKnownDllSectionHandle(
    _In_ PCWSTR DllName,
    _In_ BOOLEAN KnownDlls32,
    _Out_ PHANDLE SectionHandle
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
    _In_ ULONG Flags, // LDR_GET_PROCEDURE_ADDRESS_*
    _In_ PVOID CallerAddress
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_10)

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
    _Out_opt_ PULONG Disposition,
    _Out_ PVOID *Cookie
    );

#define LDR_UNLOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS 0x00000001

NTSYSAPI
NTSTATUS
NTAPI
LdrUnlockLoaderLock(
    _In_ ULONG Flags,
    _In_ PVOID Cookie
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
    _In_ ULONG Machine, // IMAGE_FILE_MACHINE_AMD64|IMAGE_FILE_MACHINE_ARM|IMAGE_FILE_MACHINE_THUMB|IMAGE_FILE_MACHINE_ARMNT
    _In_ ULONG_PTR VA,
    _In_ ULONG SizeOfBlock,
    _In_ PUSHORT NextOffset,
    _In_ LONG_PTR Diff
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_8)

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

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrVerifyImageMatchesChecksumEx(
    _In_ HANDLE ImageFileHandle,
    _Inout_ PLDR_VERIFY_IMAGE_INFO VerifyInfo
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrQueryModuleServiceTags(
    _In_ PVOID DllHandle,
    _Out_writes_(*BufferSize) PULONG ServiceTagBuffer,
    _Inout_ PULONG BufferSize
    );

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

/**
 * Registers for notification when a DLL is first loaded. This notification occurs before dynamic linking takes place.
 *
 * \param Flags This parameter must be zero.
 * \param NotificationFunction A pointer to an LdrDllNotification notification callback function to call when the DLL is loaded.
 * \param Context A pointer to context data for the callback function.
 * \param Cookie A pointer to a variable to receive an identifier for the callback function. This identifier is used to unregister the notification callback function.
 * \return NTSTATUS Successful or errant status.
 * \remarks https://learn.microsoft.com/en-us/windows/win32/devnotes/ldrregisterdllnotification
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
 * \param Cookie A pointer to the callback identifier received from the LdrRegisterDllNotification call that registered for notification.
 * \return NTSTATUS Successful or errant status.
 * \remarks https://learn.microsoft.com/en-us/windows/win32/devnotes/ldrunregisterdllnotification
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrUnregisterDllNotification(
    _In_ PVOID Cookie
    );

// end_msdn

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
// deprecated
NTSYSAPI
PUNICODE_STRING
NTAPI
LdrStandardizeSystemPath(
    _In_ PCUNICODE_STRING SystemPath
    );
#endif

typedef struct _LDR_FAILURE_DATA
{
    NTSTATUS Status;
    WCHAR DllName[0x20];
    WCHAR AdditionalInfo[0x20];
} LDR_FAILURE_DATA, *PLDR_FAILURE_DATA;

#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)
NTSYSAPI
PLDR_FAILURE_DATA
NTAPI
LdrGetFailureData(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_VISTA)

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

// private
#if (PHNT_VERSION >= PHNT_WINDOWS_8)
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

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrAddLoadAsDataTable(
    _In_ PVOID DllHandle,
    _In_opt_ PCWSTR FilePath,
    _In_ SIZE_T FileSize,
    _In_ HANDLE FileHandle,
    _In_opt_ PACTIVATION_CONTEXT ActCtx
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrRemoveLoadAsDataTable(
    _In_ PVOID DllHandle,
    _Out_ PVOID *BaseModule,
    _Out_opt_ PSIZE_T FileSize,
    _In_ ULONG Flags
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
LdrGetFileNameFromLoadAsDataTable(
    _In_ PVOID DllHandle,
    _Out_ PWSTR *FileName
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrDisableThreadCalloutsForDll(
    _In_ PVOID DllHandle
    );

//
// Resources
//

// NtCurrentTeb()->ResourceRetValue
// LdrFindResource* and LdrAccessResource
typedef struct _LDR_RESLOADER_RET
{
    PVOID Module;
    PVOID DataEntry;
    PVOID TargetModule;
} LDR_RESLOADER_RET, *PLDR_RESLOADER_RET;

/**
 * The LdrAccessResource function returns a pointer to the first byte of the specified resource in memory.
 *
 * \param DllHandle A handle to the DLL.
 * \param ResourceDataEntry The resource information block.
 * \param ResourceBuffer The pointer to the specified resource in memory.
 * \param ResourceLength The size, in bytes, of the specified resource.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadresource
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

/**
 * The LdrFindResource_U function determines the location of a resource in a DLL.
 *
 * \param DllHandle A handle to the DLL.
 * \param ResourcePath A pointer to an array of Type/Name/Language/(optional)AlternateType.
 * \param Count The number of elements in the ResourcePath array.
 * \param ResourceDataEntry The resource information block.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-findresourceexw
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrFindResource_U(
    _In_ PVOID DllHandle,
    _In_reads_(Count) PULONG_PTR ResourcePath,
    _In_ ULONG Count,
    _Out_ PIMAGE_RESOURCE_DATA_ENTRY *ResourceDataEntry
    );

/**
 * The LdrFindResourceEx_U function determines the location of a resource in a DLL.
 *
 * \param Flags A handle to the DLL.
 * \param DllHandle A handle to the DLL.
 * \param ResourcePath A pointer to an array of Type/Name/Language/(optional)AlternateType.
 * \param Count The number of elements in the ResourcePath array.
 * \param ResourceDataEntry The resource information block.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-findresourceexw
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrFindResourceEx_U(
    _In_ ULONG Flags,
    _In_ PVOID DllHandle,
    _In_reads_(Count) PULONG_PTR ResourcePath,
    _In_ ULONG Count,
    _Out_ PIMAGE_RESOURCE_DATA_ENTRY *ResourceDataEntry
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrFindResourceDirectory_U(
    _In_ PVOID DllHandle,
    _In_reads_(Count) PULONG_PTR ResourcePath,
    _In_ ULONG Count,
    _Out_ PIMAGE_RESOURCE_DIRECTORY *ResourceDirectory
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)

// rev // Flags for LdrResFindResource, LdrpResGetResourceDirectory, LdrResSearchResource
#define LDR_RES_REQUIRE_FOUR_KEYS_A      0x00000001u  // Enables 4-key mode (variant A) (requires Count==4)
#define LDR_RES_ALLOW_ANY                0x00000002u  // Permit Count < 3 (else Count must be 3 or 4)
#define LDR_RES_OPTIMIZE_SMALL_A         0x00000008u  // Cannot combine with LDR_RES_OPTIMIZE_SMALL_B
#define LDR_RES_OPTIMIZE_SMALL_B         0x00000010u  // Required when using LDR_RES_SPECIAL_DEPENDENCY with LDR_RES_MODE_D_SEARCH
#define LDR_RES_REQUIRE_FOUR_KEYS_B      0x00000040u  // Enables 4-key mode (Enable alternate module message) (requires Count==4)

// Search mode flags (if not specified, LDR_RES_MODE_A_SEARCH is the default)
#define LDR_RES_MODE_A_SEARCH            0x00000100u  // Default mode for typical resource lookup. // Exclusive with B/C/D // LdrResRelease
#define LDR_RES_MODE_B_SEARCH            0x00000200u  // When the resource is loaded as a datafile // LDR_IS_DATAFILE(DllHandle) // Exclusive with A/C/D // LdrResRelease
#define LDR_RES_MODE_C_SEARCH            0x00000400u  // When precise control over mapping size is needed. // Exclusive with A/B/D // LdrResRelease
#define LDR_RES_MODE_D_SEARCH            0x00000800u  // When dependency resolution or alternate resources are needed. // Used with LDR_RES_SPECIAL_DEPENDENCY // Exclusive with A/B/C // LdrResRelease

// Mapping behavior flags (only valid with LDR_RES_MODE_C or LDR_RES_MODE_D)
#define LDR_RES_MAPPING_STRICT           0x00001000u  // Default; Fail if mapping size query fails // LdrResRelease
#define LDR_RES_MAPPING_LENIENT          0x00002000u  // Allow fallback if mapping size query fails // LdrResRelease
#define LDR_RES_MAPPING_ALT_RESOURCE     0x00004000u  // When the primary resource search fails, try load and search the alternate resource // LdrResRelease

// Small/fast lookup optimizations (only valid with LDR_RES_MODE_A or LDR_RES_MODE_B)
#define LDR_RES_SPECIAL_DEPENDENCY       0x00008000u  // Only valid with (LDR_RES_MODE_D_SEARCH | LDR_RES_OPTIMIZE_SMALL_B)

#define LDR_RES_SIZE_FROM_LENGTH_C       0x00020000u  // Use *ResourceLength as mapping size; requires LDR_RES_MODE_C
#define LDR_RES_SIZE_FROM_LENGTH_AB      0x00080000u  // Use *ResourceLength as mapping size; requires LDR_RES_MODE_A or LDR_RES_MODE_B

// Internal-only (set by loader on alternate resource retry; callers must not set)
#define LDR_RES_INTERNAL_ALT_RETRY       0x01000000u

// Group masks
#define LDR_RES_MODE_MASK                0x00000F00u  // LDR_RES_MODE_A|LDR_RES_MODE_B|LDR_RES_MODE_C|LDR_RES_MODE_D
#define LDR_RES_BEHAVIOR_MASK            0x00003000u  // LDR_RES_MAPPING_STRICT/LDR_RES_MAPPING_LENIENT
#define LDR_RES_SIZEOVERRIDE_MASK        0x000A0000u  // LDR_RES_SIZE_FROM_LENGTH_* (0x20000|0x80000)
#define LDR_RES_KEY4_MASK                (LDR_RES_REQUIRE_FOUR_KEYS_A | LDR_RES_REQUIRE_FOUR_KEYS_B)

// Public/caller-visible bit mask (high bits must be zero for callers)
#define LDR_RES_PUBLIC_MASK              0x000FFFFFu

// Common invalid combinations (useful for validation)
#define LDR_RES_INVALID_SMALL_OPT_PAIR           0x00000018u  // LDR_RES_OPTIMIZE_SMALL_A|LDR_RES_OPTIMIZE_SMALL_B
#define LDR_RES_INVALID_MAPPING_BEHAVIOR_PAIR    0x00003000u  // LDR_RES_MAPPING_STRICT|LDR_RES_MAPPING_LENIENT?

// rev
/**
 * The LdrResFindResource function finds a resource in a DLL.
 *
 * \param DllHandle A handle to the DLL.
 * \param Type The type of the resource. This parameter can also be MAKEINTRESOURCE(ID), where ID is the integer identifier of the resource.
 * \param Name The name of the resource. This parameter can also be MAKEINTRESOURCE(ID), where ID is the integer identifier of the resource.
 * \param Language The language of the resource. This parameter can also be MAKEINTRESOURCE(ID), where ID is the integer identifier of the resource.
 * \param ResourceBuffer An optional pointer to receive the resource buffer.
 * \param ResourceLength An optional pointer to receive the resource length.
 * \param CultureName An optional buffer to receive the culture name.
 * \param CultureNameLength An optional pointer to receive the length of the culture name.
 * \param Flags Flags to modify the resource search.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrResFindResource(
    _In_ PVOID DllHandle,
    _In_ PCWSTR Type,
    _In_ PCWSTR Name,
    _In_ PCWSTR Language,
    _Out_opt_ PVOID* ResourceBuffer,
    _Out_opt_ PULONG ResourceLength,
    _Out_writes_bytes_opt_(CultureNameLength) PVOID CultureName, // WCHAR buffer[6]
    _Out_opt_ PULONG CultureNameLength,
    _In_opt_ ULONG Flags
    );

// rev
/**
 * The LdrResFindResourceDirectory function finds the resource directory containing the specified resource.
 *
 * \param DllHandle A handle to the DLL.
 * \param Type The type of the resource. This parameter can also be MAKEINTRESOURCE(ID), where ID is the integer identifier of the resource.
 * \param Name The name of the resource. This parameter can also be MAKEINTRESOURCE(ID), where ID is the integer identifier of the resource.
 * \param ResourceDirectory An optional pointer to receive the resource directory.
 * \param CultureName An optional buffer to receive the culture name.
 * \param CultureNameLength An optional pointer to receive the length of the culture name.
 * \param Flags Flags for the resource search.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrResFindResourceDirectory(
    _In_ PVOID DllHandle,
    _In_ PCWSTR Type,
    _In_ PCWSTR Name,
    _Out_opt_ PIMAGE_RESOURCE_DIRECTORY* ResourceDirectory,
    _Out_writes_bytes_opt_(CultureNameLength) PVOID CultureName, // WCHAR buffer[6]
    _Out_opt_ PULONG CultureNameLength,
    _In_opt_ ULONG Flags
    );

// rev
/**
 * The LdrpResGetResourceDirectory function returns the resource directory for a DLL.
 *
 * \param DllHandle A handle to the DLL.
 * \param Type The type of the resource. This parameter can also be MAKEINTRESOURCE(ID), where ID is the integer identifier of the resource.
 * \param Name The name of the resource. This parameter can also be MAKEINTRESOURCE(ID), where ID is the integer identifier of the resource.
 * \param ResourceDirectory An optional pointer to receive the resource directory.
 * \param CultureName An optional buffer to receive the culture name.
 * \param CultureNameLength An optional pointer to receive the length of the culture name.
 * \param Flags Flags for the resource search.
 * \return NTSTATUS Successful or errant status.
 */
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

// rev
/**
* The LdrResSearchResource function searches for a resource in a DLL.
*
* \param DllHandle A handle to the DLL.
* \param ResourcePath A pointer to an array of Type/Name/Language/(optional)AlternateType.
* \param Count The number of elements in the ResourcePath array.
* \param Flags Flags for the resource search.
* \param ResourceBuffer An optional pointer to receive the resource buffer.
* \param ResourceLength An optional pointer to receive the resource length.
* \param CultureName An optional buffer to receive the culture name.
* \param CultureNameLength An optional pointer to receive the length of the culture name.
* \return NTSTATUS Successful or errant status.
*/
NTSYSAPI
NTSTATUS
NTAPI
LdrResSearchResource(
    _In_ PVOID DllHandle,
    _In_reads_(Count) PULONG_PTR ResourcePath,
    _In_ ULONG Count,
    _In_ ULONG Flags,
    _Out_opt_ PVOID* ResourceBuffer,
    _Out_opt_ PSIZE_T ResourceLength,
    _Out_writes_bytes_opt_(*CultureNameLength) PVOID CultureName, // WCHAR buffer[6]
    _Out_opt_ PULONG CultureNameLength
    );

// rev
typedef struct _MUI_RC_CONFIG
{
    ULONG Signature;          // Magic signature 0xFEEDFACE (-20054323 signed)
    ULONG Size;               // Total size of this structure
    ULONG Version;            // Version (0x10000 = 1.0)
    ULONG Flags1;             // Primary flags field (validated with & 0xFFFFFFF8)
    ULONG Flags2;             // Secondary flags field (validated with & 0xFFFFFFCC)
    ULONG ValidationField;    // Additional validation field
    ULONG Flags3;             // Tertiary flags field (validated with & 0xFFFFFFFC)
    ULONG Reserved1;          // Reserved field
    ULONG Reserved2;          // Reserved field
    ULONG Reserved3;          // Reserved field
    ULONG Reserved4;          // Reserved field
    ULONG Reserved5;          // Reserved field
    ULONG Reserved6;          // Reserved field
    ULONG Reserved7;          // Reserved field
    ULONG Reserved8;          // Reserved field
    ULONG Reserved9;          // Reserved field
    ULONG Reserved10;         // Reserved field

    // Data section offset/size pairs (validated for bounds checking)
    ULONG Section1Offset;     // First data section offset
    ULONG Section1Size;       // First data section size
    ULONG Section2Offset;     // Second data section offset
    ULONG Section2Size;       // Second data section size
    ULONG Section3Offset;     // Third data section offset
    ULONG Section3Size;       // Third data section size
    ULONG Section4Offset;     // Fourth data section offset
    ULONG Section4Size;       // Fourth data section size
    ULONG Section5Offset;     // Fifth data section offset
    ULONG Section5Size;       // Fifth data section size
    ULONG Section6Offset;     // Sixth data section offset
    ULONG Section6Size;       // Sixth data section size
    ULONG Section7Offset;     // Seventh data section offset
    ULONG Section7Size;       // Seventh data section size
    ULONG Section8Offset;     // Eighth data section offset
    ULONG Section8Size;       // Eighth data section size
    // Variable length data follows...
    // The actual data sections referenced by the offset/size pairs above
} MUI_RC_CONFIG, *PMUI_RC_CONFIG;

// Magic signature constant
#define MUI_RC_CONFIG_SIGNATURE 0xFEEDFACE
#define MUI_RC_CONFIG_VERSION_1_0 0x10000
// Flag validation masks
#define MUI_FLAGS1_VALID_MASK 0xFFFFFFF8  // Only lower 3 bits allowed
#define MUI_FLAGS2_VALID_MASK 0xFFFFFFCC  // Specific bit pattern
#define MUI_FLAGS3_VALID_MASK 0xFFFFFFFC  // Only lower 2 bits allowed

/**
 * The LdrResGetRCConfig function retrieves the MUI configuration (resource type 3) for a DLL.
 *
 * \param DllHandle A handle to the DLL.
 * \param Length The length of the configuration buffer.
 * \param Config A buffer to receive the configuration.
 * \param Flags Flags for the operation.
 * \param AlternateResource Indicates if an alternate resource should be loaded.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrResGetRCConfig(
    _In_ PVOID DllHandle,
    _In_opt_ SIZE_T Length,
    _Out_writes_bytes_opt_(Length) PMUI_RC_CONFIG* Config,
    _In_ ULONG Flags,
    _In_ BOOLEAN AlternateResource // LdrLoadAlternateResourceModule
    );

/**
 * The LdrResRelease function releases the alternate resource module or section of an associated DLL.
 *
 * \param DllHandle A handle to the DLL.
 * \param CultureNameOrId An optional culture name or ID.
 * \param Flags Flags for the operation.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrResRelease(
    _In_ PVOID DllHandle,
    _In_opt_ PCWSTR CultureNameOrId, // MAKEINTRESOURCE
    _In_ ULONG Flags
    );

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

/**
 * The LdrEnumResources routine enumerates resources of a specified DLL module.
 *
 * \param DllHandle Handle to the loaded DLL module whose resources are to be enumerated.
 * \param ResourceId A pointer to an array of Type/Name/Language/(optional)AlternateType.
 * \param Count Specifies the number of elements in the ResourceId array.
 * \param ResourceCount On input, specifies the maximum number of resources to enumerate. On output, receives the actual number of resources enumerated.
 * \param Resources Pointer to a buffer that receives an array of LDR_ENUM_RESOURCE_ENTRY structures describing the resources.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrEnumResources(
    _In_ PVOID DllHandle,
    _In_reads_(Count) PULONG_PTR ResourceId,
    _In_ ULONG Count,
    _Inout_ ULONG *ResourceCount,
    _Out_writes_to_opt_(*ResourceCount, *ResourceCount) PLDR_ENUM_RESOURCE_ENTRY Resources
    );

/**
 * The LdrFindEntryForAddress routine retrieves the loader data table entry for a given address within a loaded module.
 *
 * \param DllHandle A pointer to an address within the loaded module (such as the base address of the DLL or any address inside the module).
 * \param Entry On success, receives a pointer to the LDR_DATA_TABLE_ENTRY structure corresponding to the module containing the specified address.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrFindEntryForAddress(
    _In_ PVOID DllHandle,
    _Out_ PLDR_DATA_TABLE_ENTRY *Entry
    );

// rev
/**
 * The LdrLoadAlternateResourceModule routine returns a handle to the language-specific dynamic-link library (DLL)
 * resource module associated with a DLL that is already loaded for the calling process.
 *
 * \param DllHandle A handle to the DLL module to search for a MUI resource. If the language-specific DLL for the MUI is available,
 * loads the specified module into the address space of the calling process and returns a handle to the module.
 * \param BaseAddress The base address of the mapped view.
 * \param Size The size of the mapped view.
 * \param Flags Reserved
 * \return NTSTATUS Successful or errant status.
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

// Flags for LdrLoadAlternateResourceModuleEx
#define LDR_LOAD_ALT_RESOURCE_MUN_MODE 0x01000000u // Use .mun files instead of .mui files

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
    _Out_opt_ PULONG ReturnedSize
    );

typedef _Function_class_(LDR_ENUM_CALLBACK)
VOID NTAPI LDR_ENUM_CALLBACK(
    _In_ PLDR_DATA_TABLE_ENTRY ModuleInformation,
    _In_ PVOID Parameter,
    _Out_ PBOOLEAN Stop
    );
typedef LDR_ENUM_CALLBACK* PLDR_ENUM_CALLBACK;

typedef _Function_class_(LDR_LOADED_MODULE_ENUMERATION_CALLBACK_FUNCTION)
VOID NTAPI LDR_LOADED_MODULE_ENUMERATION_CALLBACK_FUNCTION(
    _In_ PCLDR_DATA_TABLE_ENTRY DataTableEntry,
    _In_opt_ PVOID Context,
    _Inout_ BOOLEAN *StopEnumeration
    );
typedef LDR_LOADED_MODULE_ENUMERATION_CALLBACK_FUNCTION* PLDR_LOADED_MODULE_ENUMERATION_CALLBACK_FUNCTION;

NTSYSAPI
NTSTATUS
NTAPI
LdrEnumerateLoadedModules(
    _In_ BOOLEAN ReservedFlag,
    _In_ PLDR_LOADED_MODULE_ENUMERATION_CALLBACK_FUNCTION EnumProc,
    _In_opt_ PVOID Context
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
 * \param ParentModuleBase A handle to the calling module. (NtCurrentImageBase)
 * \param DllName The file name of the delay-loaded DLL that exports the specified function. This parameter is case-insensitive.
 * \param ProcedureName The address of a delay-load failure callback function for the specified DLL and process.
 * \param Flags Reserved; must be 0.
 * \return NTSTATUS Successful or errant status.
 * \remarks https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi2/nf-libloaderapi2-queryoptionaldelayloadedapi
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
#endif // (PHNT_VERSION >= PHNT_WINDOWS_10)

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// rev from ResolveDelayLoadedAPI
/**
 * Locates the target function of the specified import and replaces the function pointer in the import thunk with the target of the function implementation.
 *
 * \param ParentModuleBase The address of the base of the module importing a delay-loaded function. (NtCurrentImageBase)
 * \param DelayloadDescriptor The address of the image delay import directory for the module to be loaded.
 * \param FailureDllHook The address of a delay-load failure callback function for the specified DLL and process.
 * \param FailureSystemHook The address of a delay-load failure callback function for the specified DLL and process.
 * \param ThunkAddress The thunk data for the target function. Used to find the specific name table entry of the function.
 * \param Flags Reserved; must be 0.
 * \return The address of the import, or the failure stub for it.
 * \remarks https://learn.microsoft.com/en-us/windows/win32/devnotes/resolvedelayloadedapi
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
 * \param [in] ParentModuleBase The base address of the module that delay loads another binary.
 * \param [in] TargetDllName The name of the target DLL.
 * \param [in] Flags Reserved; must be 0.
 * \return NTSTATUS Successful or errant status.
 * \remarks https://learn.microsoft.com/en-us/windows/win32/devnotes/resolvedelayloadsfromdll
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
 * \param [in] DirectoryFlags The directories to search.
 * \return NTSTATUS Successful or errant status.
 * \remarks https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-setdefaultdlldirectories
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
 * \param [in] NewDirectory An absolute path to the directory to add to the search path. For example, to add the directory Dir2 to the process DLL search path, specify \Dir2.
 * \param [out] Cookie An opaque pointer that can be passed to RemoveDllDirectory to remove the DLL from the process DLL search path.
 * \return NTSTATUS Successful or errant status.
 * \remarks https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-adddlldirectory
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
 * \param [in] Cookie The cookie returned by LdrAddDllDirectory when the directory was added to the search path.
 * \return NTSTATUS Successful or errant status.
 * \remarks https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-removedlldirectory
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrRemoveDllDirectory(
    _In_ DLL_DIRECTORY_COOKIE Cookie
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_8)

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

#if (PHNT_VERSION >= PHNT_WINDOWS_10RS3)
// private
/**
 * The LdrControlFlowGuardEnforced function checks if Control Flow Guard is enforced.
 *
 * \return TRUE if Control Flow Guard is enforced, FALSE otherwise.
 */
NTSYSAPI
ULONG
NTAPI
LdrControlFlowGuardEnforced(
    VOID
    );
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
 * \param ProcessHandle A handle to the process for which you want to create an enclave.
 * \param BaseAddress The preferred base address of the enclave. Specify NULL to have the operating system assign the base address.
 * \param Reserved Reserved.
 * \param Size The size of the enclave that you want to create, including the size of the code that you will load into the enclave, in bytes.
 * \param InitialCommitment The amount of memory to commit for the enclave, in bytes. This parameter is not used for virtualization-based security (VBS) enclaves.
 * \param EnclaveType The architecture type of the enclave that you want to create. To verify that an enclave type is supported, call IsEnclaveTypeSupported.
 * \param EnclaveInformation A pointer to the architecture-specific information to use to create the enclave.
 * \param EnclaveInformationLength The length of the structure that the EnclaveInformation parameter points to, in bytes.
 * For the ENCLAVE_TYPE_SGX and ENCLAVE_TYPE_SGX2 enclave types, this value must be 4096. For the ENCLAVE_TYPE_VBS enclave type, this value must be sizeof(ENCLAVE_CREATE_INFO_VBS), which is 36 bytes.
 * \param EnclaveError An optional pointer to a variable that receives an enclave error code that is architecture-specific.
 * \return NTSTATUS Successful or errant status.
 * \remarks https://learn.microsoft.com/en-us/windows/win32/api/enclaveapi/nf-enclaveapi-createenclave
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
 * \param ProcessHandle A handle to the process for which the enclave was created.
 * \param BaseAddress Any address within the enclave.
 * \param EnclaveInformation A pointer to the architecture-specific information to use to initialize the enclave.
 * \param EnclaveInformationLength The length of the structure that the EnclaveInformation parameter points to, in bytes.
 * For the ENCLAVE_TYPE_SGX and ENCLAVE_TYPE_SGX2 enclave types, this value must be 4096. For the ENCLAVE_TYPE_VBS enclave type, this value must be sizeof(ENCLAVE_CREATE_INFO_VBS), which is 36 bytes.
 * \param EnclaveError An optional pointer to a variable that receives an enclave error code that is architecture-specific.
 * \return NTSTATUS Successful or errant status.
 * \remarks https://learn.microsoft.com/en-us/windows/win32/api/enclaveapi/nf-enclaveapi-initializeenclave
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
 * \param BaseAddress The base address of the enclave that you want to delete.
 * \return NTSTATUS Successful or errant status.
 * \remarks https://learn.microsoft.com/en-us/windows/win32/api/enclaveapi/nf-enclaveapi-deleteenclave
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
 * \param Routine The address of the function that you want to call.
 * \param Flags The flags to modify the call function.
 * \param RoutineParamReturn The parameter than you want to pass to the function.
 * \return NTSTATUS Successful or errant status.
 * \remarks https://learn.microsoft.com/en-us/windows/win32/api/enclaveapi/nf-enclaveapi-callenclave
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
 * \param BaseAddress The base address of the image into which to load the image.
 * \param DllPath A NULL-terminated string that contains the path of the image to load.
 * \param DllName A NULL-terminated string that contains the name of the image to load.
 * \return NTSTATUS Successful or errant status.
 * \remarks https://learn.microsoft.com/en-us/windows/win32/api/enclaveapi/nf-enclaveapi-loadenclaveimagew
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
 * \remarks This routine does not catch all potential deadlock cases; it is possible for a thread inside a loader callout
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
NTSTATUS
NTAPI
LdrAppxHandleIntegrityFailure(
    _In_ NTSTATUS Status
    );

#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

// Note: Keep the static asserts below at the end of the file to ensure the structure is correct.

#if defined(_WIN64)
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks) == 0x10, "LDR_DATA_TABLE_ENTRY.InMemoryOrderLinks offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks) == 0x20, "LDR_DATA_TABLE_ENTRY.InInitializationOrderLinks offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, DllBase) == 0x30, "LDR_DATA_TABLE_ENTRY.DllBase offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, EntryPoint) == 0x38, "LDR_DATA_TABLE_ENTRY.EntryPoint offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, SizeOfImage) == 0x40, "LDR_DATA_TABLE_ENTRY.SizeOfImage offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, ObsoleteLoadCount) == 0x6c, "LDR_DATA_TABLE_ENTRY.ObsoleteLoadCount offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, TimeDateStamp) == 0x80, "LDR_DATA_TABLE_ENTRY.TimeDateStamp offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, DdagNode) == 0x98, "LDR_DATA_TABLE_ENTRY.DdagNode offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, ParentDllBase) == 0xb8, "LDR_DATA_TABLE_ENTRY.ParentDllBase offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, OriginalBase) == 0xf8, "LDR_DATA_TABLE_ENTRY.OriginalBase offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, BaseNameHashValue) == 0x108, "LDR_DATA_TABLE_ENTRY.BaseNameHashValue offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, LoadReason) == 0x10c, "LDR_DATA_TABLE_ENTRY.LoadReason offset incorrect");
static_assert(sizeof(LDR_DATA_TABLE_ENTRY) == 0x138, "LDR_DATA_TABLE_ENTRY incorrect size");
#else
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks) == 0x8, "LDR_DATA_TABLE_ENTRY.InMemoryOrderLinks offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks) == 0x10, "LDR_DATA_TABLE_ENTRY.InInitializationOrderLinks offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, DllBase) == 0x18, "LDR_DATA_TABLE_ENTRY.DllBase offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, EntryPoint) == 0x1c, "LDR_DATA_TABLE_ENTRY.EntryPoint offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, SizeOfImage) == 0x20, "LDR_DATA_TABLE_ENTRY.SizeOfImage offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, ObsoleteLoadCount) == 0x38, "LDR_DATA_TABLE_ENTRY.ObsoleteLoadCount offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, TimeDateStamp) == 0x44, "LDR_DATA_TABLE_ENTRY.TimeDateStamp offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, DdagNode) == 0x50, "LDR_DATA_TABLE_ENTRY.DdagNode offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, ParentDllBase) == 0x60, "LDR_DATA_TABLE_ENTRY.ParentDllBase offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, OriginalBase) == 0x80, "LDR_DATA_TABLE_ENTRY.OriginalBase offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, BaseNameHashValue) == 0x90, "LDR_DATA_TABLE_ENTRY.BaseNameHashValue offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY, LoadReason) == 0x94, "LDR_DATA_TABLE_ENTRY.LoadReason offset incorrect");
static_assert(sizeof(LDR_DATA_TABLE_ENTRY) == 0xB8, "LDR_DATA_TABLE_ENTRY incorrect size");
#endif

#endif
