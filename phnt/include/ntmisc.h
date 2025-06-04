/*
 * Trace Control support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTMISC_H
#define _NTMISC_H

//
// Apphelp
//

typedef enum _AHC_INFO_CLASS
{
    AhcInfoClassSdbQueryResult          = 0x00000001,
    AhcInfoClassSdbSxsOverrideManifest  = 0x00000002,
    AhcInfoClassSdbRunlevelFlags        = 0x00000004,
    AhcInfoClassSdbFusionFlags          = 0x00000008,
    AhcInfoClassSdbInstallerFlags       = 0x00000010,
    AhcInfoClassFusionFlags             = 0x00000020,
    AhcInfoClassTelemetryFlags          = 0x00000040,
    AhcInfoClassInstallDetect           = 0x00000080,
    AhcInfoClassRacEventSent            = 0x00000100,
    AhcInfoClassIsSystemFile            = 0x00000200,
    AhcInfoClassMonitoringFlags         = 0x00000400,
    AhcInfoClassExeType                 = 0x00000800,
} AHC_INFO_CLASS, *PAHC_INFO_CLASS;

#define AHC_INFO_CLASS_FILTER_ON_FILETIME_CHANGE            \
    (AHC_INFO_CLASS)(AhcInfoClassSdbQueryResult |           \
                     AhcInfoClassSdbSxsOverrideManifest |   \
                     AhcInfoClassSdbRunlevelFlags |         \
                     AhcInfoClassSdbFusionFlags |           \
                     AhcInfoClassSdbInstallerFlags |        \
                     AhcInfoClassFusionFlags |              \
                     AhcInfoClassRacEventSent)

#define AHC_INFO_CLASS_FILTER_ON_SDB_CHANGE                 \
    (AHC_INFO_CLASS)(AhcInfoClassSdbQueryResult |           \
                     AhcInfoClassSdbSxsOverrideManifest |   \
                     AhcInfoClassSdbRunlevelFlags |         \
                     AhcInfoClassSdbFusionFlags |           \
                     AhcInfoClassSdbInstallerFlags |        \
                     AhcInfoClassInstallDetect)

#define AHC_INFO_CLASS_ALL                                  \
    (AHC_INFO_CLASS)(AhcInfoClassSdbQueryResult |           \
                     AhcInfoClassSdbSxsOverrideManifest |   \
                     AhcInfoClassSdbRunlevelFlags |         \
                     AhcInfoClassSdbFusionFlags |           \
                     AhcInfoClassSdbInstallerFlags |        \
                     AhcInfoClassFusionFlags |              \
                     AhcInfoClassTelemetryFlags |           \
                     AhcInfoClassInstallDetect |            \
                     AhcInfoClassRacEventSent |             \
                     AhcInfoClassIsSystemFile |             \
                     AhcInfoClassMonitoringFlags |          \
                     AhcInfoClassExeType)

#define AHC_INFO_CLASS_INTERNALLY_COMPUTED                  \
    (AHC_INFO_CLASS)(AhcInfoClassSdbQueryResult |           \
                     AhcInfoClassSdbSxsOverrideManifest |   \
                     AhcInfoClassSdbRunlevelFlags |         \
                     AhcInfoClassSdbFusionFlags |           \
                     AhcInfoClassSdbInstallerFlags |        \
                     AhcInfoClassTelemetryFlags |           \
                     AhcInfoClassIsSystemFile |             \
                     AhcInfoClassMonitoringFlags |          \
                     AhcInfoClassExeType)

#define AHC_INFO_CLASS_SAFE_FOR_UNPRIVILEGED_UPDATE         \
    (AHC_INFO_CLASS)(AhcInfoClassInstallDetect |            \
                     AhcInfoClassRacEventSent |             \
                     AhcInfoClassTelemetryFlags |           \
                     AhcInfoClassMonitoringFlags)

//
// Cache structures and APIs.
//

typedef enum _AHC_SERVICE_CLASS
{
    ApphelpCacheServiceLookup = 0,
    ApphelpCacheServiceRemove = 1,
    ApphelpCacheServiceUpdate = 2,
    ApphelpCacheServiceClear = 3,
    ApphelpCacheServiceSnapStatistics = 4,
    ApphelpCacheServiceSnapCache = 5,
    ApphelpCacheServiceLookupCdb = 6,
    ApphelpCacheServiceRefreshCdb = 7,
    ApphelpCacheServiceMapQuirks = 8,
    ApphelpCacheServiceHwIdQuery = 9,
    ApphelpCacheServiceInitProcessData = 10,
    ApphelpCacheServiceLookupAndWriteToProcess = 11,
    ApphelpCacheServiceMax
} AHC_SERVICE_CLASS;

typedef struct _AHC_SERVICE_LOOKUP
{
    AHC_INFO_CLASS InfoClass;                   // Information to lookup.
    UINT HintFlags;                             // Hint flags about cache query.
    UNICODE_STRING PackageAlias;                // Aliased package moniker in a packed string.
    HANDLE FileHandle;                          // User space handle to file.
    HANDLE ProcessHandle;                       // User space process handle.
    USHORT ExeType;                             // Executable bitness.
    USHORT Padding;                             // Padding to even USHORTs.
    UNICODE_STRING ExeSignature;                // Executable file signature.
    PCZZWSTR Environment;                       // Environment block.
    UINT EnvironmentSize;                       // Size of environment block in bytes.
} AHC_SERVICE_LOOKUP, *PAHC_SERVICE_LOOKUP;

typedef struct _AHC_SERVICE_REMOVE
{
    AHC_INFO_CLASS InfoClass;
    UNICODE_STRING PackageAlias;
    HANDLE FileHandle;
    UNICODE_STRING ExeSignature;
} AHC_SERVICE_REMOVE, *PAHC_SERVICE_REMOVE;

typedef struct _AHC_SERVICE_UPDATE
{
    AHC_INFO_CLASS InfoClass;
    UNICODE_STRING PackageAlias;
    HANDLE FileHandle;
    UNICODE_STRING ExeSignature;
    PVOID Data;
    ULONG DataSize;
} AHC_SERVICE_UPDATE, *PAHC_SERVICE_UPDATE;

typedef struct _AHC_SERVICE_CLEAR
{
    AHC_INFO_CLASS InfoClass;
} AHC_SERVICE_CLEAR, *PAHC_SERVICE_CLEAR;

typedef struct _AHC_SERVICE_LOOKUP_CDB
{
    UNICODE_STRING Name;
} AHC_SERVICE_LOOKUP_CDB, *PAHC_SERVICE_LOOKUP_CDB;

//
// AHC_HINT_* flags are used in the HintFlags variable.
//

#define AHC_HINT_FORCE_BYPASS                           0x00000001
#define AHC_HINT_REMOVABLE_MEDIA                        0x00000002
#define AHC_HINT_TEMPORARY_DIRECTORY                    0x00000004
#define AHC_HINT_USER_PERM_LAYER                        0x00000008
#define AHC_HINT_CREATE_PROCESS                         0x00000010
#define AHC_HINT_NATIVE_EXE                             0x00000020

#define SHIM_CACHE_MAIN_DATABASE_PATH32                 L"\\AppPatch\\sysmain.sdb"
#define SHIM_CACHE_MAIN_DATABASE_PATH64                 L"\\AppPatch\\AppPatch64\\sysmain.sdb"

//
// Flag definitions for various flag-type information in cache.
//

#define AHC_CACHE_FLAG_MONITORING_IS_CANDIDATE          0x00000001 // Candidate for monitoring.
#define AHC_CACHE_FLAG_MONITORING_IS_COMPLETE           0x00000002 // Monitoring has completed.
#define AHC_CACHE_FLAG_MONITORING_VALID_MASK            (AHC_CACHE_FLAG_MONITORING_IS_CANDIDATE | \
                                                         AHC_CACHE_FLAG_MONITORING_IS_COMPLETE)

#define AHC_CACHE_FLAG_TELEMETRY_IS_CANDIDATE           0x00000001 // Candidate for telemetry.
#define AHC_CACHE_FLAG_TELEMETRY_HAS_SAMPLED            0x00000002 // Telemetry has run.
#define AHC_CACHE_FLAG_TELEMETRY_VALID_MASK             (AHC_CACHE_FLAG_TELEMETRY_IS_CANDIDATE | \
                                                         AHC_CACHE_FLAG_TELEMETRY_HAS_SAMPLED)

#define AHC_CACHE_FLAG_FUSION_HASDOTLOCAL               0x00000001 // Dot local file exists.
#define AHC_CACHE_FLAG_FUSION_HASMANIFESTFILE           0x00000002 // Fusion manifest exists.
#define AHC_CACHE_FLAG_FUSION_HASMANIFESTRESOURCE       0x00000004 // Fusion manifest resource exists.
#define AHC_CACHE_FLAG_FUSION_VALID_MASK                (AHC_CACHE_FLAG_FUSION_HASDOTLOCAL | \
                                                         AHC_CACHE_FLAG_FUSION_HASMANIFESTFILE | \
                                                         AHC_CACHE_FLAG_FUSION_HASMANIFESTRESOURCE)

#define AHC_CACHE_FLAG_RAC_EVENTSENT                    0x00000001 // Rac event has been sent.
#define AHC_CACHE_FLAG_RAC_VALID_MASK                   (AHC_CACHE_FLAG_RAC_EVENTSENT)

#define AHC_CACHE_FLAG_INSTALLDETECT_CLAIMED            0x00000001 // InstallDetect claimed.
#define AHC_CACHE_FLAG_INSTALLDETECT_VALID_MASK         (AHC_CACHE_FLAG_RAC_EVENTSENT)

//
// Statistics.
//

typedef struct _AHC_MAIN_STATISTICS
{
    ULONG Lookup;                               // Count of lookup calls.
    ULONG Remove;                               // Count of remove calls.
    ULONG Update;                               // Count of update calls.
    ULONG Clear;                                // Count of clear calls.
    ULONG SnapStatistics;                       // Count of snap statistics calls.
    ULONG SnapCache;                            // Count of snap store calls.
} AHC_MAIN_STATISTICS, *PAHC_MAIN_STATISTICS;

typedef struct _AHC_STORE_STATISTICS
{
    ULONG LookupHits;                           // Count of lookup hits.
    ULONG LookupMisses;                         // Count of lookup misses.
    ULONG Inserted;                             // Count of inserted.
    ULONG Replaced;                             // Count of replaced.
    ULONG Updated;                              // Count of updates.
} AHC_STORE_STATISTICS, *PAHC_STORE_STATISTICS;

typedef struct _AHC_STATISTICS
{
    ULONG Size;                                 // Size of the structure.
    AHC_MAIN_STATISTICS Main;                   // Main statistics.
    AHC_STORE_STATISTICS Store;                 // Store statistics.
} AHC_STATISTICS, *PAHC_STATISTICS;

typedef struct _AHC_SERVICE_DATAQUERY
{
    AHC_STATISTICS Stats;                       // Statistics.
    ULONG DataSize;                             // Size of data.
    PBYTE Data;                                 // Data.
} AHC_SERVICE_DATAQUERY, *PAHC_SERVICE_DATAQUERY;

typedef struct _AHC_SERVICE_DATACACHE
{
    HANDLE FileHandle;                          // User space handle to file.
    USHORT ExeType;                             // Executable bitness.
    USHORT Padding;                             // Padding to even USHORTs.
    UINT HintFlags;                             // Metadata flags about cache query.
    HANDLE ProcessHandle;                       // User space process handle.
    UNICODE_STRING FileName;                    // Executable file name.
    UNICODE_STRING Environment;                 // Environment block.
    UNICODE_STRING PackageAlias;                // Aliased package moniker in a packed string.
    ULONG CustomDataSize;                       // Size of the custom data to cache.
    PBYTE CustomData;                           // Pointer to the custom data.
} AHC_SERVICE_DATACACHE, *PAHC_SERVICE_DATACACHE;

typedef struct _AHC_SERVICE_HWID_QUERY
{
    BOOLEAN QueryResult;                        // Query result
    UNICODE_STRING HwId;                        // Query HwId; can contain wildcards
} AHC_SERVICE_HWID_QUERY, *PAHC_SERVICE_HWID_QUERY;

typedef struct _AHC_SERVICE_DATA
{
    AHC_SERVICE_LOOKUP Lookup;                  // Lookup EXE/Package.
    AHC_SERVICE_UPDATE Update;                  // Updating flags for a given exe/package.
    AHC_SERVICE_DATACACHE Cache;                // For cache operations.
    AHC_SERVICE_LOOKUP_CDB LookupCdb;           // Lookup cdb.
    AHC_SERVICE_CLEAR Clear;                    // Clear flags for all exes/packages.
    AHC_SERVICE_REMOVE Remove;                  // Remove EXE/Package.
    AHC_SERVICE_HWID_QUERY HwIdQuery;           // For HWID cache queries.
    NTSTATUS DriverStatus;                      // Receive the status from the cache driver. Set error code in IoStatus block causes driver verifier violation.
    PVOID ParamsOut;                            // Parameters out data.
    ULONG ParamsOutSize;                        // Parameters out size.
} AHC_SERVICE_DATA, *PAHC_SERVICE_DATA;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtApphelpCacheControl(
    _In_ AHC_SERVICE_CLASS ServiceClass,
    _Inout_opt_ PVOID ServiceContext // AHC_SERVICE_DATA
    );

//
// VDM
//

typedef enum _VDMSERVICECLASS
{
    VdmStartExecution,
    VdmQueueInterrupt,
    VdmDelayInterrupt,
    VdmInitialize,
    VdmFeatures,
    VdmSetInt21Handler,
    VdmQueryDir,
    VdmPrinterDirectIoOpen,
    VdmPrinterDirectIoClose,
    VdmPrinterInitialize,
    VdmSetLdtEntries,
    VdmSetProcessLdtInfo,
    VdmAdlibEmulation,
    VdmPMCliControl,
    VdmQueryVdmProcess,
    VdmPreInitialize
} VDMSERVICECLASS, *PVDMSERVICECLASS;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtVdmControl(
    _In_ VDMSERVICECLASS Service,
    _Inout_ PVOID ServiceData
    );

//
// Sessions
//

typedef enum _IO_SESSION_EVENT
{
    IoSessionEventIgnore,
    IoSessionEventCreated,
    IoSessionEventTerminated,
    IoSessionEventConnected,
    IoSessionEventDisconnected,
    IoSessionEventLogon,
    IoSessionEventLogoff,
    IoSessionEventMax
} IO_SESSION_EVENT;

typedef enum _IO_SESSION_STATE
{
    IoSessionStateCreated = 1,
    IoSessionStateInitialized = 2,
    IoSessionStateConnected = 3,
    IoSessionStateDisconnected = 4,
    IoSessionStateDisconnectedLoggedOn = 5,
    IoSessionStateLoggedOn = 6,
    IoSessionStateLoggedOff = 7,
    IoSessionStateTerminated = 8,
    IoSessionStateMax
} IO_SESSION_STATE;

#if (PHNT_MODE != PHNT_MODE_KERNEL)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenSession(
    _Out_ PHANDLE SessionHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtNotifyChangeSession(
    _In_ HANDLE SessionHandle,
    _In_ ULONG ChangeSequenceNumber,
    _In_ PLARGE_INTEGER ChangeTimeStamp,
    _In_ IO_SESSION_EVENT Event,
    _In_ IO_SESSION_STATE NewState,
    _In_ IO_SESSION_STATE PreviousState,
    _In_reads_bytes_opt_(PayloadSize) PVOID Payload,
    _In_ ULONG PayloadSize
    );

#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

//
// ApiSet
//

NTSYSAPI
BOOL
NTAPI
ApiSetQueryApiSetPresence(
    _In_ PCUNICODE_STRING Namespace,
    _Out_ PBOOLEAN Present
    );

NTSYSAPI
BOOL
NTAPI
ApiSetQueryApiSetPresenceEx(
    _In_ PCUNICODE_STRING Namespace,
    _Out_ PBOOLEAN IsInSchema,
    _Out_ PBOOLEAN Present
    );

typedef enum _SECURE_SETTING_VALUE_TYPE
{
    SecureSettingValueTypeBoolean = 0,
    SecureSettingValueTypeUlong = 1,
    SecureSettingValueTypeBinary = 2,
    SecureSettingValueTypeString = 3,
    SecureSettingValueTypeUnknown = 4
} SECURE_SETTING_VALUE_TYPE, *PSECURE_SETTING_VALUE_TYPE;

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
// rev
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySecurityPolicy(
    _In_ PCUNICODE_STRING Policy,
    _In_ PCUNICODE_STRING KeyName,
    _In_ PCUNICODE_STRING ValueName,
    _In_ SECURE_SETTING_VALUE_TYPE ValueType,
    _Out_writes_bytes_opt_(*ValueSize) PVOID Value,
    _Inout_ PULONG ValueSize
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)

#if (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)
// rev
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateCrossVmEvent(
    _Out_ PHANDLE CrossVmEvent,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG CrossVmEventFlags,
    _In_ LPCGUID VMID,
    _In_ LPCGUID ServiceID
    );

// rev
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateCrossVmMutant(
    _Out_ PHANDLE EventHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG CrossVmEventFlags,
    _In_ LPCGUID VMID,
    _In_ LPCGUID ServiceID
    );

// rev
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAcquireCrossVmMutant(
    _In_ HANDLE CrossVmMutant,
    _In_ PLARGE_INTEGER Timeout
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)

#if (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)
// rev
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDirectGraphicsCall(
    _In_ ULONG InputBufferLength,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _Out_ PULONG ReturnLength
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)

#if (PHNT_VERSION >= PHNT_WINDOWS_11_22H2)
// rev
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenCpuPartition(
    _Out_ PHANDLE CpuPartitionHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes
    );

// rev
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateCpuPartition(
    _Out_ PHANDLE CpuPartitionHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes
    );

// rev
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationCpuPartition(
    _In_ HANDLE CpuPartitionHandle,
    _In_ ULONG CpuPartitionInformationClass,
    _In_reads_bytes_(CpuPartitionInformationLength) PVOID CpuPartitionInformation,
    _In_ ULONG CpuPartitionInformationLength,
    _Reserved_ PVOID,
    _Reserved_ ULONG,
    _Reserved_ ULONG
    );

// rev
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationCpuPartition(
    _In_ HANDLE CpuPartitionHandle,
    _In_ ULONG CpuPartitionInformationClass,
    _Out_writes_bytes_opt_(CpuPartitionInformationLength) PVOID CpuPartitionInformation,
    _In_ ULONG CpuPartitionInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

#endif // (PHNT_VERSION >= PHNT_WINDOWS_11_22H2)

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS2)
//
// Process KeepAlive (also WakeCounter)
//

typedef enum _PROCESS_ACTIVITY_TYPE
{
    ProcessActivityTypeAudio = 0,
    ProcessActivityTypeMax = 1
} PROCESS_ACTIVITY_TYPE;

// rev
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAcquireProcessActivityReference(
    _Out_ PHANDLE ActivityReferenceHandle,
    _In_ HANDLE ParentProcessHandle,
    _In_ ULONG ProcessActivityType // PROCESS_ACTIVITY_TYPE
    );

#endif // (PHNT_VERSION >= PHNT_WINDOWS_10_RS2)

//
// Appx/Msix Packages
//

// private
typedef struct _PACKAGE_CONTEXT_REFERENCE
{
    PVOID reserved;
} *PACKAGE_CONTEXT_REFERENCE;

// private
typedef enum PackageProperty
{
    PackageProperty_Name = 1,                  // q: WCHAR[]
    PackageProperty_Version = 2,               // q: WCHAR[]
    PackageProperty_Architecture = 3,          // q: ULONG (PROCESSOR_ARCHITECTURE_*)
    PackageProperty_ResourceId = 4,            // q: WCHAR[]
    PackageProperty_Publisher = 5,             // q: WCHAR[]
    PackageProperty_PublisherId = 6,           // q: WCHAR[]
    PackageProperty_FamilyName = 7,            // q: WCHAR[]
    PackageProperty_FullName = 8,              // q: WCHAR[]
    PackageProperty_Flags = 9,                 // q: ULONG
    PackageProperty_InstalledLocation = 10,    // q: WCHAR[]
    PackageProperty_DisplayName = 11,          // q: WCHAR[]
    PackageProperty_PublisherDisplayName = 12, // q: WCHAR[]
    PackageProperty_Description = 13,          // q: WCHAR[]
    PackageProperty_Logo = 14,                 // q: WCHAR[]
    PackageProperty_PackageOrigin = 15         // q: PackageOrigin
} PackageProperty;

// private
typedef struct _PACKAGE_APPLICATION_CONTEXT_REFERENCE
{
    PVOID reserved;
} *PACKAGE_APPLICATION_CONTEXT_REFERENCE;

// private
typedef enum PackageApplicationProperty
{
    PackageApplicationProperty_Aumid = 1,                        // q: WCHAR[]
    PackageApplicationProperty_Praid = 2,                        // q: WCHAR[]
    PackageApplicationProperty_DisplayName = 3,                  // q: WCHAR[]
    PackageApplicationProperty_Description = 4,                  // q: WCHAR[]
    PackageApplicationProperty_Logo = 5,                         // q: WCHAR[]
    PackageApplicationProperty_SmallLogo = 6,                    // q: WCHAR[]
    PackageApplicationProperty_ForegroundText = 7,               // q: ULONG
    PackageApplicationProperty_ForegroundTextString = 8,         // q: WCHAR[]
    PackageApplicationProperty_BackgroundColor = 9,              // q: ULONG
    PackageApplicationProperty_StartPage = 10,                   // q: WCHAR[]
    PackageApplicationProperty_ContentURIRulesCount = 11,        // q: ULONG
    PackageApplicationProperty_ContentURIRules = 12,             // q: WCHAR[] (multi-sz)
    PackageApplicationProperty_StaticContentURIRulesCount = 13,  // q: ULONG
    PackageApplicationProperty_StaticContentURIRules = 14,       // q: WCHAR[] (multi-sz)
    PackageApplicationProperty_DynamicContentURIRulesCount = 15, // q: ULONG
    PackageApplicationProperty_DynamicContentURIRules = 16       // q: WCHAR[] (multi-sz)
} PackageApplicationProperty;

// private
typedef struct _PACKAGE_RESOURCES_CONTEXT_REFERENCE
{
    PVOID reserved;
} *PACKAGE_RESOURCES_CONTEXT_REFERENCE;

// private
typedef enum PackageResourcesProperty
{
    PackageResourcesProperty_DisplayName = 1,
    PackageResourcesProperty_PublisherDisplayName = 2,
    PackageResourcesProperty_Description = 3,
    PackageResourcesProperty_Logo = 4,
    PackageResourcesProperty_SmallLogo = 5,
    PackageResourcesProperty_StartPage = 6
} PackageResourcesProperty;

// private
typedef struct _PACKAGE_SECURITY_CONTEXT_REFERENCE
{
    PVOID reserved;
} *PACKAGE_SECURITY_CONTEXT_REFERENCE;

// private
typedef enum PackageSecurityProperty
{
    PackageSecurityProperty_SecurityFlags = 1,     // q: ULONG
    PackageSecurityProperty_AppContainerSID = 2,   // q: Sid
    PackageSecurityProperty_CapabilitiesCount = 3, // q: ULONG
    PackageSecurityProperty_Capabilities = 4       // q: Sid[]
} PackageSecurityProperty;

// private
typedef struct _TARGET_PLATFORM_CONTEXT_REFERENCE
{
    PVOID reserved;
} *TARGET_PLATFORM_CONTEXT_REFERENCE;

// private
typedef enum TargetPlatformProperty
{
    TargetPlatformProperty_Platform = 1,   // q: ULONG
    TargetPlatformProperty_MinVersion = 2, // q: PACKAGE_VERSION
    TargetPlatformProperty_MaxVersion = 3  // q: PACKAGE_VERSION
} TargetPlatformProperty;

// private
typedef struct _PACKAGE_GLOBALIZATION_CONTEXT_REFERENCE
{
    PVOID reserved;
} *PACKAGE_GLOBALIZATION_CONTEXT_REFERENCE;

// private
typedef enum PackageGlobalizationProperty
{
    PackageGlobalizationProperty_ForceUtf8 = 1,                // q: ULONG
    PackageGlobalizationProperty_UseWindowsDisplayLanguage = 2 // q: ULONG
} PackageGlobalizationProperty;

#if (PHNT_VERSION >= PHNT_WINDOWS_8_1)

// rev
WINBASEAPI
ULONG
WINAPI
GetCurrentPackageContext(
    _In_ ULONG Index,
    _Reserved_ ULONG_PTR Unused,
    _Out_ PACKAGE_CONTEXT_REFERENCE *PackageContext
    );

// rev
WINBASEAPI
ULONG
WINAPI
GetPackageContext(
    _In_ PVOID PackageInfoReference, // PACKAGE_INFO_REFERENCE
    _In_ ULONG Index,
    _Reserved_ ULONG_PTR Unused,
    _Out_ PACKAGE_CONTEXT_REFERENCE *PackageContext
    );

// rev
WINBASEAPI
ULONG
WINAPI
GetPackageProperty(
    _In_ PACKAGE_CONTEXT_REFERENCE PackageContext,
    _In_ PackageProperty PropertyId,
    _Inout_ PULONG BufferSize,
    _Out_writes_bytes_(BufferSize) PVOID Buffer
    );

// rev
WINBASEAPI
ULONG
WINAPI
GetPackagePropertyString(
    _In_ PACKAGE_CONTEXT_REFERENCE PackageContext,
    _In_ PackageProperty PropertyId,
    _Inout_ PULONG BufferLength,
    _Out_writes_(BufferLength) PWSTR Buffer
    );

// rev
WINBASEAPI
ULONG
WINAPI
GetPackageOSMaxVersionTested(
    _In_ PACKAGE_CONTEXT_REFERENCE PackageContext,
    _Out_ ULONGLONG *OSMaxVersionTested // PACKAGE_VERSION
    );

//
// Package Application Properties
//

// rev
WINBASEAPI
ULONG
WINAPI
GetCurrentPackageApplicationContext(
    _In_ ULONG Index,
    _Reserved_ ULONG_PTR Unused,
    _Out_ PACKAGE_APPLICATION_CONTEXT_REFERENCE *PackageApplicationContext
    );

// rev
WINBASEAPI
ULONG
WINAPI
GetPackageApplicationContext(
    _In_ PVOID PackageInfoReference, // PACKAGE_INFO_REFERENCE
    _In_ ULONG Index,
    _Reserved_ ULONG_PTR Unused,
    _Out_ PACKAGE_APPLICATION_CONTEXT_REFERENCE *PackageApplicationContext
    );

// rev
WINBASEAPI
ULONG
WINAPI
GetPackageApplicationProperty(
    _In_ PACKAGE_APPLICATION_CONTEXT_REFERENCE PackageApplicationContext,
    _In_ PackageApplicationProperty PropertyId,
    _Inout_ PULONG BufferSize,
    _Out_writes_bytes_(BufferSize) PVOID Buffer
    );

// rev
WINBASEAPI
ULONG
WINAPI
GetPackageApplicationPropertyString(
    _In_ PACKAGE_APPLICATION_CONTEXT_REFERENCE PackageApplicationContext,
    _In_ PackageApplicationProperty PropertyId,
    _Inout_ PULONG BufferLength,
    _Out_writes_(BufferLength) PWSTR Buffer
    );

//
// Package Resource Properties
//

// rev
WINBASEAPI
ULONG
WINAPI
GetCurrentPackageResourcesContext(
    _In_ ULONG Index,
    _Reserved_ ULONG_PTR Unused,
    _Out_ PACKAGE_RESOURCES_CONTEXT_REFERENCE *PackageResourcesContext
    );

// rev
WINBASEAPI
ULONG
WINAPI
GetPackageResourcesContext(
    _In_ PVOID PackageInfoReference, // PACKAGE_INFO_REFERENCE
    _In_ ULONG Index,
    _Reserved_ ULONG_PTR Unused,
    _Out_ PACKAGE_RESOURCES_CONTEXT_REFERENCE *PackageResourcesContext
    );

// rev
WINBASEAPI
ULONG
WINAPI
GetCurrentPackageApplicationResourcesContext(
    _In_ ULONG Index,
    _Reserved_ ULONG_PTR Unused,
    _Out_ PACKAGE_RESOURCES_CONTEXT_REFERENCE *PackageResourcesContext
    );

// rev
WINBASEAPI
LONG
WINAPI
GetPackageApplicationResourcesContext(
    _In_ PVOID PackageInfoReference, // PACKAGE_INFO_REFERENCE
    _In_ ULONG Index,
    _Reserved_ ULONG_PTR Unused,
    _Out_ PACKAGE_RESOURCES_CONTEXT_REFERENCE *PackageResourcesContext
    );

// rev
WINBASEAPI
LONG
WINAPI
GetPackageResourcesProperty(
    _In_ PACKAGE_RESOURCES_CONTEXT_REFERENCE PackageResourcesContext,
    _In_ PackageResourcesProperty PropertyId,
    _Inout_ PULONG BufferSize,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _Out_opt_ PULONG Flags
    );

//
// Package Security Properties
//

// rev
WINBASEAPI
LONG
WINAPI
GetCurrentPackageSecurityContext(
    _Reserved_ ULONG_PTR Unused,
    _Out_ PACKAGE_SECURITY_CONTEXT_REFERENCE *PackageSecurityContext
    );

// rev
WINBASEAPI
LONG
WINAPI
GetPackageSecurityContext(
    _In_ PVOID PackageInfoReference, // PACKAGE_INFO_REFERENCE
    _Reserved_ ULONG_PTR Unused,
    _Out_ PACKAGE_SECURITY_CONTEXT_REFERENCE *PackageSecurityContext
    );

// rev
WINBASEAPI
LONG
WINAPI
GetPackageSecurityProperty(
    _In_ PACKAGE_SECURITY_CONTEXT_REFERENCE PackageSecurityContext,
    _In_ PackageSecurityProperty PropertyId,
    _Inout_ PULONG BufferSize,
    _Out_writes_bytes_(BufferSize) PVOID Buffer
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_8_1

#if (PHNT_VERSION >= PHNT_WINDOWS_10)

//
// Target Platform Properties
//

// rev
WINBASEAPI
LONG
WINAPI
GetCurrentTargetPlatformContext(
    _Reserved_ ULONG_PTR Unused,
    _Out_ TARGET_PLATFORM_CONTEXT_REFERENCE *TargetPlatformContext
    );

WINBASEAPI
LONG
WINAPI
GetTargetPlatformContext(
    _In_ PVOID PackageInfoReference, // PACKAGE_INFO_REFERENCE
    _Reserved_ ULONG_PTR Unused,
    _Out_ TARGET_PLATFORM_CONTEXT_REFERENCE *TargetPlatformContext
    );

// rev
WINBASEAPI
LONG
WINAPI
GetPackageTargetPlatformProperty(
    _In_ TARGET_PLATFORM_CONTEXT_REFERENCE TargetPlatformContext,
    _In_ TargetPlatformProperty PropertyId,
    _Inout_ PULONG BufferSize,
    _Out_writes_bytes_(BufferSize) PVOID Buffer
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_10

#if (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)

// rev
WINBASEAPI
HRESULT
WINAPI
GetCurrentPackageInfo3(
    _In_ ULONG Flags,
    _In_ ULONG PackagePathType, // PackagePathType
    _Inout_ PULONG BufferLength,
    _Out_writes_bytes_opt_(*BufferLength) PVOID Buffer,
    _Out_opt_ PULONG ReturnLength
    );

//
// Package Globalization Properties
//

// rev
WINBASEAPI
LONG
WINAPI
GetCurrentPackageGlobalizationContext(
    _In_ ULONG Index,
    _Reserved_ ULONG_PTR Unused,
    _Out_ PACKAGE_GLOBALIZATION_CONTEXT_REFERENCE *PackageGlobalizationContext
    );

// rev
WINBASEAPI
LONG
WINAPI
GetPackageGlobalizationContext(
    _In_ PVOID PackageInfoReference, // PACKAGE_INFO_REFERENCE
    _In_ ULONG Index,
    _Reserved_ ULONG_PTR Unused,
    _Out_ PACKAGE_GLOBALIZATION_CONTEXT_REFERENCE *PackageGlobalizationContext
    );

// rev
WINBASEAPI
LONG
WINAPI
GetPackageGlobalizationProperty(
    _In_ PACKAGE_GLOBALIZATION_CONTEXT_REFERENCE PackageGlobalizationContext,
    _In_ PackageGlobalizationProperty PropertyId,
    _Inout_ PULONG BufferSize,
    _Out_writes_bytes_(BufferSize) PVOID Buffer
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_10_20H1

//
// COM
//

// private
typedef enum _MTA_HOST_USAGE_FLAGS
{
    MTA_HOST_USAGE_NONE = 0x0,
    MTA_HOST_USAGE_MTAINITIALIZED = 0x1,
    MTA_HOST_USAGE_ACTIVATORINITIALIZED = 0x2,
    MTA_HOST_USAGE_UNLOADCALLED = 0x4,
} MTA_HOST_USAGE_FLAGS, *PMTA_HOST_USAGE_FLAGS;

// private
typedef struct _MTA_USAGE_GLOBALS
{
    _Reserved_ ULONG StackCapture;
    PULONG MTAInits; // A pointer to the total number of MTA inits
    PULONG MTAIncInits; // A pointer to the number of MTA inits from CoIncrementMTAUsage
    PULONG MTAWaiters; // A pointer to the number of callers waiting inside CoWaitMTACompletion
    PULONG MTAIncrementorSize; // A pointer to the size of the cookie returned by CoIncrementMTAUsage
    ULONG CompletionTimeOut; // A timeout for CoWaitMTACompletion in milliseconds
    _Reserved_ PLIST_ENTRY ListEntryHeadMTAUsageIncrementor;
    _Reserved_ PULONG MTAIncrementorCompleted;
    _Reserved_ PVOID* MTAUsageCompletedIncrementorHead;
    PMTA_HOST_USAGE_FLAGS MTAHostUsageFlags; // A pointer to the MTA usage flags // since THRESHOLD
} MTA_USAGE_GLOBALS, *PMTA_USAGE_GLOBALS;

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// private // combase.dll, ordinal 70
_Success_(return != 0)
_Must_inspect_result_
WINBASEAPI
PMTA_USAGE_GLOBALS
WINAPI
CoGetMTAUsageInfo(
    VOID
    );
#endif

//
// COM/OLE
//

// OLETLSFLAGS
#define OLETLS_LOCALTID 0x01 // This TID is in the current process.
#define OLETLS_UUIDINITIALIZED 0x02 // This Logical thread is init'd.
#define OLETLS_INTHREADDETACH 0x04 // This is in thread detach.
#define OLETLS_CHANNELTHREADINITIALZED 0x08// This channel has been init'd
#define OLETLS_WOWTHREAD 0x10 // This thread is a 16-bit WOW thread.
#define OLETLS_THREADUNINITIALIZING 0x20 // This thread is in CoUninitialize.
#define OLETLS_DISABLE_OLE1DDE 0x40 // This thread can't use a DDE window.
#define OLETLS_APARTMENTTHREADED 0x80 // This is an STA apartment thread
#define OLETLS_MULTITHREADED 0x100 // This is an MTA apartment thread
#define OLETLS_IMPERSONATING 0x200 // This thread is impersonating
#define OLETLS_DISABLE_EVENTLOGGER 0x400 // Prevent recursion in event logger
#define OLETLS_INNEUTRALAPT 0x800 // This thread is in the NTA
#define OLETLS_DISPATCHTHREAD 0x1000 // This is a dispatch thread
#define OLETLS_HOSTTHREAD 0x2000 // This is a host thread
#define OLETLS_ALLOWCOINIT 0x4000 // This thread allows inits
#define OLETLS_PENDINGUNINIT 0x8000 // This thread has pending uninit
#define OLETLS_FIRSTMTAINIT 0x10000// First thread to attempt an MTA init
#define OLETLS_FIRSTNTAINIT 0x20000// First thread to attempt an NTA init
#define OLETLS_APTINITIALIZING 0x40000 // Apartment Object is initializing
#define OLETLS_UIMSGSINMODALLOOP 0x80000
#define OLETLS_MARSHALING_ERROR_OBJECT 0x100000 // since WIN8
#define OLETLS_WINRT_INITIALIZE 0x200000 // This thread called RoInitialize
#define OLETLS_APPLICATION_STA 0x400000
#define OLETLS_IN_SHUTDOWN_CALLBACKS 0x800000
#define OLETLS_POINTER_INPUT_BLOCKED 0x1000000
#define OLETLS_IN_ACTIVATION_FILTER 0x2000000 // since WINBLUE
#define OLETLS_ASTATOASTAEXEMPT_QUIRK 0x4000000
#define OLETLS_ASTATOASTAEXEMPT_PROXY 0x8000000
#define OLETLS_ASTATOASTAEXEMPT_INDOUBT 0x10000000
#define OLETLS_DETECTED_USER_INITIALIZED 0x20000000 // since RS3
#define OLETLS_BRIDGE_STA 0x40000000 // since RS5
#define OLETLS_NAINITIALIZING 0x80000000UL // since 19H1

// private
typedef struct tagSOleTlsData
{
    PVOID ThreadBase;
    PVOID SmAllocator;
    ULONG ApartmentID;
    ULONG Flags; // OLETLSFLAGS
    LONG TlsMapIndex;
    PVOID *TlsSlot;
    ULONG ComInits;
    ULONG OleInits;
    ULONG Calls;
    PVOID ServerCall; // previously CallInfo (before TH1)
    PVOID CallObjectCache; // previously FreeAsyncCall (before TH1)
    PVOID ContextStack; // previously FreeClientCall (before TH1)
    PVOID ObjServer;
    ULONG TIDCaller;
    // ... (other fields are version-dependant)
} SOleTlsData, *PSOleTlsData;

//
// AppCompat
//

typedef struct tagSDBQUERYRESULT
{
    ULONG Exes[16];
    ULONG ExeFlags[16];
    ULONG Layers[8];
    ULONG LayerFlags;
    ULONG AppHelp;
    ULONG ExeCount;
    ULONG LayerCount;
    GUID ID;
    ULONG ExtraFlags;
    ULONG CustomSDBMap;
    GUID DB[16];
} SDBQUERYRESULT, *PSDBQUERYRESULT;

static_assert(sizeof(SDBQUERYRESULT) == 0x1c8, "SDBQUERYRESULT size mismatch");

typedef struct tagSWITCH_CONTEXT_ATTRIBUTE
{
    ULONG_PTR ContextUpdateCounter;
    BOOL AllowContextUpdate;
    BOOL EnableTrace;
    HANDLE EtwHandle;
} SWITCH_CONTEXT_ATTRIBUTE, *PSWITCH_CONTEXT_ATTRIBUTE;

#ifdef _WIN64
static_assert(sizeof(SWITCH_CONTEXT_ATTRIBUTE) == 0x18, "SWITCH_CONTEXT_ATTRIBUTE size mismatch");
#else
static_assert(sizeof(SWITCH_CONTEXT_ATTRIBUTE) == 0x10, "SWITCH_CONTEXT_ATTRIBUTE size mismatch");
#endif

typedef struct tagSWITCH_CONTEXT_DATA
{
    ULONGLONG OsMaxVersionTested;
    ULONG TargetPlatform;
    ULONGLONG ContextMinimum;
    GUID Platform;
    GUID MinPlatform;
    ULONG ContextSource;
    ULONG ElementCount;
    GUID Elements[48];
} SWITCH_CONTEXT_DATA, * PSWITCH_CONTEXT_DATA;

static_assert(sizeof(SWITCH_CONTEXT_DATA) == 0x340, "SWITCH_CONTEXT_DATA size mismatch");

typedef struct tagSWITCH_CONTEXT
{
    SWITCH_CONTEXT_ATTRIBUTE Attribute;
    SWITCH_CONTEXT_DATA Data;
} SWITCH_CONTEXT, *PSWITCH_CONTEXT;

#ifdef _WIN64
static_assert(sizeof(SWITCH_CONTEXT) == 0x358, "SWITCH_CONTEXT size mismatch");
#else
static_assert(sizeof(SWITCH_CONTEXT) == 0x350, "SWITCH_CONTEXT size mismatch");
#endif

typedef struct _SDB_CSTRUCT_COBALT_PROCFLAG
{
    KAFFINITY AffinityMask;
    ULONG CPUIDEcxOverride;
    ULONG CPUIDEdxOverride;
    USHORT ProcessorGroup;
    USHORT FastSelfModThreshold;
    USHORT Reserved1;
    UCHAR Reserved2;
    UCHAR BackgroundWork : 5;
    UCHAR CPUIDBrand : 4;
    UCHAR Reserved3 : 4;
    UCHAR RdtscScaling : 3;
    UCHAR Reserved4 : 2;
    UCHAR UnalignedAtomicApproach : 2;
    UCHAR Win11Atomics : 2;
    UCHAR RunOnSingleCore : 1;
    UCHAR X64CPUID : 1;
    UCHAR PatchUnaligned : 1;
    UCHAR InterpreterOrJitter : 1;
    UCHAR ForceSegmentHeap : 1;
    UCHAR Reserved5 : 1;
    UCHAR Reserved6 : 1;
    union
    {
        ULONGLONG Group1AsUINT64;
        struct _SDB_CSTRUCT_COBALT_PROCFLAG* Specified;
    };
} SDB_CSTRUCT_COBALT_PROCFLAG, *PSDB_CSTRUCT_COBALT_PROCFLAG;

#ifdef _WIN64
static_assert(sizeof(SDB_CSTRUCT_COBALT_PROCFLAG) == 0x28, "SDB_CSTRUCT_COBALT_PROCFLAG size mismatch");
#else
static_assert(sizeof(SDB_CSTRUCT_COBALT_PROCFLAG) == 0x20, "SDB_CSTRUCT_COBALT_PROCFLAG size mismatch");
#endif

typedef struct _APPCOMPAT_EXE_DATA
{
    ULONG_PTR Reserved[65];
    ULONG Size;
    ULONG Magic;
    BOOL LoadShimEngine;
    USHORT ExeType;
    SDBQUERYRESULT SdbQueryResult;
    ULONG_PTR DbgLogChannels[128];
    SWITCH_CONTEXT SwitchContext;
    ULONG ParentProcessId;
    WCHAR ParentImageName[260];
    WCHAR ParentCompatLayers[256];
    WCHAR ActiveCompatLayers[256];
    ULONG ImageFileSize;
    ULONG ImageCheckSum;
    BOOL LatestOs;
    BOOL PackageId;
    BOOL SwitchBackManifest;
    BOOL UacManifest;
    BOOL LegacyInstaller;
    ULONG RunLevel;
    ULONG_PTR WinRTFlags;
    PVOID HookCOM;
    PVOID ComponentOnDemandEvent;
    PVOID Quirks;
    ULONG QuirksSize;
    SDB_CSTRUCT_COBALT_PROCFLAG CobaltProcFlags;
    ULONG FullMatchDbSizeCb;
    ULONG FullMatchDbOffset;
} APPCOMPAT_EXE_DATA;

#ifdef _WIN64
static_assert(sizeof(APPCOMPAT_EXE_DATA) == 0x11C0, "APPCOMPAT_EXE_DATA size mismatch");
#else
static_assert(sizeof(APPCOMPAT_EXE_DATA) == 0xE98, "APPCOMPAT_EXE_DATA size mismatch");
#endif

#endif // _NTMISC_H
