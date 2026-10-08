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

typedef _Enum_is_bitflag_ enum _AHC_INFO_CLASS
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

/**
 * The AHC_SERVICE_CLASS enumeration identifies the operation class for an NtApphelpCacheControl request.
 */
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

/**
 * The AHC_SERVICE_LOOKUP structure contains the parameters for an application compatibility (AppHelp) cache lookup request.
 */
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

/**
 * The AHC_SERVICE_REMOVE structure contains the parameters for an application compatibility (AppHelp) cache remove request.
 */
typedef struct _AHC_SERVICE_REMOVE
{
    AHC_INFO_CLASS InfoClass;
    UNICODE_STRING PackageAlias;
    HANDLE FileHandle;
    UNICODE_STRING ExeSignature;
} AHC_SERVICE_REMOVE, *PAHC_SERVICE_REMOVE;

/**
 * The AHC_SERVICE_UPDATE structure contains the parameters for an application compatibility (AppHelp) cache update request.
 */
typedef struct _AHC_SERVICE_UPDATE
{
    AHC_INFO_CLASS InfoClass;
    UNICODE_STRING PackageAlias;
    HANDLE FileHandle;
    UNICODE_STRING ExeSignature;
    PVOID Data;
    ULONG DataSize;
} AHC_SERVICE_UPDATE, *PAHC_SERVICE_UPDATE;

/**
 * The AHC_SERVICE_CLEAR structure contains the parameters for an application compatibility (AppHelp) cache clear request.
 */
typedef struct _AHC_SERVICE_CLEAR
{
    AHC_INFO_CLASS InfoClass;
} AHC_SERVICE_CLEAR, *PAHC_SERVICE_CLEAR;

/**
 * The AHC_SERVICE_LOOKUP_CDB structure contains the compatibility database (CDB) parameters for an application compatibility cache lookup.
 */
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

/**
 * The AHC_MAIN_STATISTICS structure contains the main application compatibility (AppHelp) cache statistics.
 */
typedef struct _AHC_MAIN_STATISTICS
{
    ULONG Lookup;                               // Count of lookup calls.
    ULONG Remove;                               // Count of remove calls.
    ULONG Update;                               // Count of update calls.
    ULONG Clear;                                // Count of clear calls.
    ULONG SnapStatistics;                       // Count of snap statistics calls.
    ULONG SnapCache;                            // Count of snap store calls.
} AHC_MAIN_STATISTICS, *PAHC_MAIN_STATISTICS;

/**
 * The AHC_STORE_STATISTICS structure contains the application compatibility (AppHelp) cache store statistics.
 */
typedef struct _AHC_STORE_STATISTICS
{
    ULONG LookupHits;                           // Count of lookup hits.
    ULONG LookupMisses;                         // Count of lookup misses.
    ULONG Inserted;                             // Count of inserted.
    ULONG Replaced;                             // Count of replaced.
    ULONG Updated;                              // Count of updates.
} AHC_STORE_STATISTICS, *PAHC_STORE_STATISTICS;

/**
 * The AHC_STATISTICS structure contains the combined application compatibility (AppHelp) cache statistics.
 */
typedef struct _AHC_STATISTICS
{
    ULONG Size;                                 // Size of the structure.
    AHC_MAIN_STATISTICS Main;                   // Main statistics.
    AHC_STORE_STATISTICS Store;                 // Store statistics.
} AHC_STATISTICS, *PAHC_STATISTICS;

/**
 * The AHC_SERVICE_DATAQUERY structure contains the parameters for an application compatibility cache data query.
 */
typedef struct _AHC_SERVICE_DATAQUERY
{
    AHC_STATISTICS Stats;                       // Statistics.
    ULONG DataSize;                             // Size of data.
    PBYTE Data;                                 // Data.
} AHC_SERVICE_DATAQUERY, *PAHC_SERVICE_DATAQUERY;

/**
 * The AHC_SERVICE_DATACACHE structure contains the cached data for an application compatibility cache entry.
 */
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

/**
 * The AHC_SERVICE_HWID_QUERY structure contains the parameters for an application compatibility hardware ID (HWID) query.
 */
typedef struct _AHC_SERVICE_HWID_QUERY
{
    BOOLEAN QueryResult;                        // Query result
    UNICODE_STRING HwId;                        // Query HwId; can contain wildcards
} AHC_SERVICE_HWID_QUERY, *PAHC_SERVICE_HWID_QUERY;

/**
 * The AHC_SERVICE_DATA structure contains the service data passed to NtApphelpCacheControl.
 */
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

/**
 * The NtApphelpCacheControl routine performs an operation on the application compatibility (AppHelp) shim cache.
 *
 * \param[in] ServiceClass The application compatibility cache operation to perform.
 * \param[in, out, optional] ServiceContext Pointer to the operation-specific data buffer (AHC_SERVICE_DATA).
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
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

/**
 * The VDMSERVICECLASS enumeration identifies the Virtual DOS Machine (VDM) service requested through NtVdmControl.
 */
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

/**
 * The NtVdmControl routine performs a control operation for the Virtual DOS Machine (VDM) subsystem.
 *
 * \param[in] Service The VDM service operation to perform.
 * \param[in, out] ServiceData Pointer to the service-specific data buffer.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
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

/**
 * The IO_SESSION_EVENT enumeration identifies the type of an I/O session event.
 */
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

/**
 * The IO_SESSION_STATE enumeration identifies the state of an I/O session.
 */
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

/**
 * The NtOpenSession routine opens a handle to a session object.
 *
 * \param[out] SessionHandle Pointer to a variable that receives a handle to the session object.
 * \param[in] DesiredAccess The access mask that specifies the requested access to the session object.
 * \param[in] ObjectAttributes Pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenSession(
    _Out_ PHANDLE SessionHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    );

/**
 * The NtNotifyChangeSession routine notifies registered clients of a change in the state of a session.
 *
 * \param[in] SessionHandle Handle to the session object.
 * \param[in] ChangeSequenceNumber The sequence number associated with the state change.
 * \param[in] ChangeTimeStamp Pointer to the time stamp of the state change.
 * \param[in] Event The session event that occurred.
 * \param[in] NewState The new session state.
 * \param[in] PreviousState The previous session state.
 * \param[in, optional] Payload Pointer to an optional event-specific payload buffer.
 * \param[in] PayloadSize The size, in bytes, of the payload buffer.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
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

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The ApiSetGetImplementationHost routine resolves the implementation host module of an API set.
 *
 * \param[in] ApiSetName The name of the API set to resolve.
 * \param[out] Resolved Pointer to a variable that receives TRUE if the API set was resolved.
 * \param[out] HostName Pointer to a UNICODE_STRING that receives the host module name.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
ApiSetGetImplementationHost(
    _In_ PCSTR ApiSetName,
    _Out_ PBOOLEAN Resolved,
    _Out_ PUNICODE_STRING HostName
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_11)

/**
 * The ApiSetQueryApiSetPresence routine determines whether an API set is present on the system.
 *
 * \param[in] Namespace The API set namespace name to query.
 * \param[out] Present Pointer to a variable that receives TRUE if the API set is present.
 * \return LOGICAL TRUE if the operation succeeded; otherwise FALSE.
 */
NTSYSAPI
LOGICAL
NTAPI
ApiSetQueryApiSetPresence(
    _In_ PCUNICODE_STRING Namespace,
    _Out_ PBOOLEAN Present
    );

/**
 * The ApiSetQueryApiSetPresenceEx routine determines whether an API set is present and defined in the API set schema.
 *
 * \param[in] Namespace The API set namespace name to query.
 * \param[out] IsInSchema Pointer to a variable that receives TRUE if the API set is defined in the schema.
 * \param[out] Present Pointer to a variable that receives TRUE if the API set is present.
 * \return LOGICAL TRUE if the operation succeeded; otherwise FALSE.
 */
NTSYSAPI
LOGICAL
NTAPI
ApiSetQueryApiSetPresenceEx(
    _In_ PCUNICODE_STRING Namespace,
    _Out_ PBOOLEAN IsInSchema,
    _Out_ PBOOLEAN Present
    );

/**
 * The SECURE_SETTING_VALUE_TYPE enumeration identifies the data type of a secure setting value.
 */
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
/**
 * The NtQuerySecurityPolicy routine queries a value from the system security policy.
 *
 * \param[in] Policy The name of the security policy to query.
 * \param[in] KeyName The name of the policy key.
 * \param[in] ValueName The name of the value to query.
 * \param[in] ValueType The type of the value to retrieve.
 * \param[out, optional] Value Pointer to a buffer that receives the value data.
 * \param[in, out] ValueSize On input, the size, in bytes, of the buffer; on output, the size of the data returned.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
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
/**
 * The NtCreateCrossVmEvent routine creates a cross-VM event object for communication between virtual machine partitions.
 *
 * \param[out] CrossVmEvent Pointer to a variable that receives a handle to the cross-VM event object.
 * \param[in] DesiredAccess The requested access to the event object.
 * \param[in, optional] ObjectAttributes Pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * \param[in] CrossVmEventFlags Flags that control the cross-VM event creation.
 * \param[in] VMID Pointer to the GUID that identifies the target virtual machine.
 * \param[in] ServiceID Pointer to the GUID that identifies the service.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
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
/**
 * The NtCreateCrossVmMutant routine creates a cross-VM mutant object for synchronization between virtual machine partitions.
 *
 * \param[out] EventHandle Pointer to a variable that receives a handle to the cross-VM mutant object.
 * \param[in] DesiredAccess The requested access to the mutant object.
 * \param[in, optional] ObjectAttributes Pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * \param[in] CrossVmEventFlags Flags that control the cross-VM mutant creation.
 * \param[in] VMID Pointer to the GUID that identifies the target virtual machine.
 * \param[in] ServiceID Pointer to the GUID that identifies the service.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
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
/**
 * The NtAcquireCrossVmMutant routine acquires (waits on) a cross-VM mutant object.
 *
 * \param[in] CrossVmMutant Handle to the cross-VM mutant object.
 * \param[in] Timeout Pointer to the timeout value for the acquire operation.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
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
/**
 * The NtDirectGraphicsCall routine issues a direct call to the graphics subsystem.
 *
 * \param[in] InputBufferLength The size, in bytes, of the input buffer.
 * \param[in, optional] InputBuffer Pointer to the input buffer.
 * \param[in] OutputBufferLength The size, in bytes, of the output buffer.
 * \param[out, optional] OutputBuffer Pointer to the output buffer.
 * \param[out] ReturnLength Pointer to a variable that receives the number of bytes returned.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
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
/**
 * The NtOpenCpuPartition routine opens a handle to an existing CPU partition object.
 *
 * \param[out] CpuPartitionHandle Pointer to a variable that receives a handle to the CPU partition object.
 * \param[in] DesiredAccess The requested access to the CPU partition object.
 * \param[in, optional] ObjectAttributes Pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenCpuPartition(
    _Out_ PHANDLE CpuPartitionHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes
    );

// rev
/**
 * The NtCreateCpuPartition routine creates a CPU partition object.
 *
 * \param[out] CpuPartitionHandle Pointer to a variable that receives a handle to the CPU partition object.
 * \param[in] DesiredAccess The requested access to the CPU partition object.
 * \param[in, optional] ObjectAttributes Pointer to an OBJECT_ATTRIBUTES structure that specifies the object attributes.
 * \param[in, optional] ExtendedParameters Pointer to an array of extended parameters.
 * \param[in] ExtendedParameterCount Number of extended parameters.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateCpuPartition(
    _Out_ PHANDLE CpuPartitionHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_reads_opt_(ExtendedParameterCount) PVOID ExtendedParameters,
    _In_ ULONG ExtendedParameterCount
    );

// rev
/**
 * The NtSetInformationCpuPartition routine sets information for a CPU partition object.
 *
 * \param[in] CpuPartitionHandle Handle to the CPU partition object.
 * \param[in] CpuPartitionInformationClass The type of information to set.
 * \param[in] CpuPartitionInformation Pointer to a buffer that contains the information to set.
 * \param[in] CpuPartitionInformationLength The size, in bytes, of the information buffer.
 * \remarks The remaining parameters are reserved.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
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
/**
 * The NtQueryInformationCpuPartition routine queries information about a CPU partition object.
 *
 * \param[in] CpuPartitionHandle Handle to the CPU partition object.
 * \param[in] CpuPartitionInformationClass The type of information to query.
 * \param[out, optional] CpuPartitionInformation Pointer to a buffer that receives the information.
 * \param[in] CpuPartitionInformationLength The size, in bytes, of the information buffer.
 * \param[out, optional] ReturnLength Pointer to a variable that receives the number of bytes returned.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
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

/**
 * The PROCESS_ACTIVITY_TYPE enumeration identifies the type of process activity for NtAcquireProcessActivityReference.
 */
typedef enum _PROCESS_ACTIVITY_TYPE
{
    ProcessActivityTypeAudio = 0,
    ProcessActivityTypeMax = 1
} PROCESS_ACTIVITY_TYPE;

// rev
/**
 * The NtAcquireProcessActivityReference routine acquires an activity reference on a process to keep it active.
 *
 * \param[out] ActivityReferenceHandle Pointer to a variable that receives a handle to the activity reference.
 * \param[in] ParentProcessHandle Handle to the process on which to acquire the activity reference.
 * \param[in] ProcessActivityType The type of process activity (PROCESS_ACTIVITY_TYPE).
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
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
/**
 * The PACKAGE_CONTEXT_REFERENCE structure represents an opaque reference to an application package context.
 */
typedef struct _PACKAGE_CONTEXT_REFERENCE
{
    PVOID reserved;
} *PACKAGE_CONTEXT_REFERENCE;

// private
/**
 * The PackageProperty enumeration identifies a property of an application package.
 */
typedef enum _PackageProperty
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
/**
 * The PACKAGE_APPLICATION_CONTEXT_REFERENCE structure represents an opaque reference to a package application context.
 */
typedef struct _PACKAGE_APPLICATION_CONTEXT_REFERENCE
{
    PVOID reserved;
} *PACKAGE_APPLICATION_CONTEXT_REFERENCE;

// private
/**
 * The PackageApplicationProperty enumeration identifies a property of an application within a package.
 */
typedef enum _PackageApplicationProperty
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
/**
 * The PACKAGE_RESOURCES_CONTEXT_REFERENCE structure represents an opaque reference to a package resources context.
 */
typedef struct _PACKAGE_RESOURCES_CONTEXT_REFERENCE
{
    PVOID reserved;
} *PACKAGE_RESOURCES_CONTEXT_REFERENCE;

// private
/**
 * The PackageResourcesProperty enumeration identifies a resources property of an application package.
 */
typedef enum _PackageResourcesProperty
{
    PackageResourcesProperty_DisplayName = 1,
    PackageResourcesProperty_PublisherDisplayName = 2,
    PackageResourcesProperty_Description = 3,
    PackageResourcesProperty_Logo = 4,
    PackageResourcesProperty_SmallLogo = 5,
    PackageResourcesProperty_StartPage = 6
} PackageResourcesProperty;

// private
/**
 * The PACKAGE_SECURITY_CONTEXT_REFERENCE structure represents an opaque reference to a package security context.
 */
typedef struct _PACKAGE_SECURITY_CONTEXT_REFERENCE
{
    PVOID reserved;
} *PACKAGE_SECURITY_CONTEXT_REFERENCE;

// private
/**
 * The PackageSecurityProperty enumeration identifies a security property of an application package.
 */
typedef enum _PackageSecurityProperty
{
    PackageSecurityProperty_SecurityFlags = 1,     // q: ULONG
    PackageSecurityProperty_AppContainerSID = 2,   // q: Sid
    PackageSecurityProperty_CapabilitiesCount = 3, // q: ULONG
    PackageSecurityProperty_Capabilities = 4       // q: Sid[]
} PackageSecurityProperty;

// private
/**
 * The TARGET_PLATFORM_CONTEXT_REFERENCE structure represents an opaque reference to a target platform context.
 */
typedef struct _TARGET_PLATFORM_CONTEXT_REFERENCE
{
    PVOID reserved;
} *TARGET_PLATFORM_CONTEXT_REFERENCE;

// private
/**
 * The TargetPlatformProperty enumeration identifies a target platform property of an application package.
 */
typedef enum _TargetPlatformProperty
{
    TargetPlatformProperty_Platform = 1,   // q: ULONG
    TargetPlatformProperty_MinVersion = 2, // q: PACKAGE_VERSION
    TargetPlatformProperty_MaxVersion = 3  // q: PACKAGE_VERSION
} TargetPlatformProperty;

// private
/**
 * The PACKAGE_GLOBALIZATION_CONTEXT_REFERENCE structure represents an opaque reference to a package globalization context.
 */
typedef struct _PACKAGE_GLOBALIZATION_CONTEXT_REFERENCE
{
    PVOID reserved;
} *PACKAGE_GLOBALIZATION_CONTEXT_REFERENCE;

// private
/**
 * The PackageGlobalizationProperty enumeration identifies a globalization property of an application package.
 */
typedef enum _PackageGlobalizationProperty
{
    PackageGlobalizationProperty_ForceUtf8 = 1,                // q: ULONG
    PackageGlobalizationProperty_UseWindowsDisplayLanguage = 2 // q: ULONG
} PackageGlobalizationProperty;

#if (PHNT_VERSION >= PHNT_WINDOWS_8_1)

// rev
/**
 * The GetCurrentPackageContext routine retrieves a package context reference for the current package.
 *
 * \param Index The zero-based index of the package in the current package graph.
 * \param Unused Reserved; must be zero.
 * \param PackageContext Receives the package context reference.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
WINBASEAPI
ULONG
WINAPI
GetCurrentPackageContext(
    _In_ ULONG Index,
    _Reserved_ ULONG_PTR Unused,
    _Out_ PACKAGE_CONTEXT_REFERENCE *PackageContext
    );

// rev
/**
 * The GetPackageContext routine retrieves a package context reference from a package information reference.
 *
 * \param PackageInfoReference A reference to the package information object.
 * \param Index The zero-based index of the package.
 * \param Unused Reserved; must be zero.
 * \param PackageContext Receives the package context reference.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
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
/**
 * The GetPackageProperty routine retrieves a property value from a package context.
 *
 * \param PackageContext The package context reference.
 * \param PropertyId The package property identifier to query.
 * \param BufferSize A pointer to a variable that specifies the buffer size and receives the required or returned size in bytes.
 * \param Buffer A pointer to the buffer receiving the property value.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
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
/**
 * The GetPackagePropertyString routine retrieves a string property value from a package context.
 *
 * \param PackageContext The package context reference.
 * \param PropertyId The package property identifier to query.
 * \param BufferLength A pointer to a variable that specifies the buffer length and receives the required or returned length in characters.
 * \param Buffer A pointer to the buffer receiving the string property.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
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
/**
 * The GetPackageOSMaxVersionTested routine retrieves the maximum operating system version against which the package was tested.
 *
 * \param PackageContext The package context reference.
 * \param OSMaxVersionTested Receives the tested maximum OS version encoded as an integer.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
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
/**
 * The GetCurrentPackageApplicationContext routine retrieves an application context reference for the current package.
 *
 * \param Index The zero-based index of the application in the package.
 * \param Unused Reserved; must be zero.
 * \param PackageApplicationContext Receives the package application context reference.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
WINBASEAPI
ULONG
WINAPI
GetCurrentPackageApplicationContext(
    _In_ ULONG Index,
    _Reserved_ ULONG_PTR Unused,
    _Out_ PACKAGE_APPLICATION_CONTEXT_REFERENCE *PackageApplicationContext
    );

// rev
/**
 * The GetPackageApplicationContext routine retrieves an application context reference from a package information reference.
 *
 * \param PackageInfoReference A reference to the package information object.
 * \param Index The zero-based index of the application.
 * \param Unused Reserved; must be zero.
 * \param PackageApplicationContext Receives the package application context reference.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
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
/**
 * The GetPackageApplicationProperty routine retrieves a property value from a package application context.
 *
 * \param PackageApplicationContext The package application context reference.
 * \param PropertyId The package application property identifier to query.
 * \param BufferSize A pointer to a variable that specifies the buffer size and receives the required or returned size in bytes.
 * \param Buffer A pointer to the buffer receiving the property value.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
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
/**
 * The GetPackageApplicationPropertyString routine retrieves a string property value from a package application context.
 *
 * \param PackageApplicationContext The package application context reference.
 * \param PropertyId The package application property identifier to query.
 * \param BufferLength A pointer to a variable that specifies the buffer length and receives the required or returned length in characters.
 * \param Buffer A pointer to the buffer receiving the string property.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
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
/**
 * The GetCurrentPackageResourcesContext routine retrieves a resources context reference for the current package.
 *
 * \param Index The zero-based index of the resource context.
 * \param Unused Reserved; must be zero.
 * \param PackageResourcesContext Receives the package resources context reference.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
WINBASEAPI
ULONG
WINAPI
GetCurrentPackageResourcesContext(
    _In_ ULONG Index,
    _Reserved_ ULONG_PTR Unused,
    _Out_ PACKAGE_RESOURCES_CONTEXT_REFERENCE *PackageResourcesContext
    );

// rev
/**
 * The GetPackageResourcesContext routine retrieves a resources context reference from a package information reference.
 *
 * \param PackageInfoReference A reference to the package information object.
 * \param Index The zero-based index of the resource context.
 * \param Unused Reserved; must be zero.
 * \param PackageResourcesContext Receives the package resources context reference.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
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
/**
 * The GetCurrentPackageApplicationResourcesContext routine retrieves an application resources context reference for the current package.
 *
 * \param Index The zero-based index of the application resource context.
 * \param Unused Reserved; must be zero.
 * \param PackageResourcesContext Receives the package resources context reference.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
WINBASEAPI
ULONG
WINAPI
GetCurrentPackageApplicationResourcesContext(
    _In_ ULONG Index,
    _Reserved_ ULONG_PTR Unused,
    _Out_ PACKAGE_RESOURCES_CONTEXT_REFERENCE *PackageResourcesContext
    );

// rev
/**
 * The GetPackageApplicationResourcesContext routine retrieves an application resources context reference from a package information reference.
 *
 * \param PackageInfoReference A reference to the package information object.
 * \param Index The zero-based index of the application resource context.
 * \param Unused Reserved; must be zero.
 * \param PackageResourcesContext Receives the package resources context reference.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
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
/**
 * The GetPackageResourcesProperty routine retrieves a property value from a package resources context.
 *
 * \param PackageResourcesContext The package resources context reference.
 * \param PropertyId The package resources property identifier to query.
 * \param BufferSize A pointer to a variable specifying the buffer size and receiving the required or returned size in bytes.
 * \param Buffer A pointer to the buffer receiving the resources property value.
 * \param Flags An optional pointer receiving property flags.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
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
/**
 * The GetCurrentPackageSecurityContext routine retrieves a security context reference for the current package.
 *
 * \param Unused Reserved; must be zero.
 * \param PackageSecurityContext Receives the package security context reference.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
WINBASEAPI
LONG
WINAPI
GetCurrentPackageSecurityContext(
    _Reserved_ ULONG_PTR Unused,
    _Out_ PACKAGE_SECURITY_CONTEXT_REFERENCE *PackageSecurityContext
    );

// rev
/**
 * The GetPackageSecurityContext routine retrieves a security context reference from a package information reference.
 *
 * \param PackageInfoReference A reference to the package information object.
 * \param Unused Reserved; must be zero.
 * \param PackageSecurityContext Receives the package security context reference.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
WINBASEAPI
LONG
WINAPI
GetPackageSecurityContext(
    _In_ PVOID PackageInfoReference, // PACKAGE_INFO_REFERENCE
    _Reserved_ ULONG_PTR Unused,
    _Out_ PACKAGE_SECURITY_CONTEXT_REFERENCE *PackageSecurityContext
    );

// rev
/**
 * The GetPackageSecurityProperty routine retrieves a property value from a package security context.
 *
 * \param PackageSecurityContext The package security context reference.
 * \param PropertyId The package security property identifier to query.
 * \param BufferSize A pointer to a variable specifying the buffer size and receiving the required or returned size in bytes.
 * \param Buffer A pointer to the buffer receiving the security property value.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
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
/**
 * The GetCurrentTargetPlatformContext routine retrieves a target platform context reference for the current package.
 *
 * \param Unused Reserved; must be zero.
 * \param TargetPlatformContext Receives the target platform context reference.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
WINBASEAPI
LONG
WINAPI
GetCurrentTargetPlatformContext(
    _Reserved_ ULONG_PTR Unused,
    _Out_ TARGET_PLATFORM_CONTEXT_REFERENCE *TargetPlatformContext
    );

// rev
/**
 * The GetTargetPlatformContext routine retrieves a target platform context reference from a package information reference.
 *
 * \param PackageInfoReference A reference to the package information object.
 * \param Unused Reserved; must be zero.
 * \param TargetPlatformContext Receives the target platform context reference.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
WINBASEAPI
LONG
WINAPI
GetTargetPlatformContext(
    _In_ PVOID PackageInfoReference, // PACKAGE_INFO_REFERENCE
    _Reserved_ ULONG_PTR Unused,
    _Out_ TARGET_PLATFORM_CONTEXT_REFERENCE *TargetPlatformContext
    );

// rev
/**
 * The GetPackageTargetPlatformProperty routine retrieves a property value from a target platform context.
 *
 * \param TargetPlatformContext The target platform context reference.
 * \param PropertyId The target platform property identifier to query.
 * \param BufferSize A pointer to a variable specifying the buffer size and receiving the required or returned size in bytes.
 * \param Buffer A pointer to the buffer receiving the property value.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
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
/**
 * The GetCurrentPackageInfo3 routine retrieves package information for the current process, specifying the package path type.
 *
 * \param Flags Package information flags specifying what information to retrieve.
 * \param PackagePathType The type of folder path to retrieve for the package.
 * \param BufferLength A pointer to a variable specifying the buffer size and receiving the required or returned size in bytes.
 * \param Buffer A pointer to the buffer receiving the package information.
 * \param ReturnLength Optional pointer receiving the number of elements written to the buffer.
 * \return HRESULT Successful or error status.
 */
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
/**
 * The GetCurrentPackageGlobalizationContext routine retrieves a globalization context reference for the current package.
 *
 * \param Index An index specifying which globalization context to retrieve.
 * \param Unused Reserved; must be zero.
 * \param PackageGlobalizationContext Receives the package globalization context reference.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
WINBASEAPI
LONG
WINAPI
GetCurrentPackageGlobalizationContext(
    _In_ ULONG Index,
    _Reserved_ ULONG_PTR Unused,
    _Out_ PACKAGE_GLOBALIZATION_CONTEXT_REFERENCE *PackageGlobalizationContext
    );

// rev
/**
 * The GetPackageGlobalizationContext routine retrieves a globalization context reference from a package information reference.
 *
 * \param PackageInfoReference A reference to the package information object.
 * \param Index An index specifying which globalization context to retrieve.
 * \param Unused Reserved; must be zero.
 * \param PackageGlobalizationContext Receives the package globalization context reference.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
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
/**
 * The GetPackageGlobalizationProperty routine retrieves a property value from a package globalization context.
 *
 * \param PackageGlobalizationContext The package globalization context reference.
 * \param PropertyId The package globalization property identifier to query.
 * \param BufferSize A pointer to a variable specifying the buffer size and receiving the required or returned size in bytes.
 * \param Buffer A pointer to the buffer receiving the property value.
 * \return A Win32 error code. ERROR_SUCCESS on success.
 */
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
typedef _Enum_is_bitflag_ enum _MTA_HOST_USAGE_FLAGS
{
    MTA_HOST_USAGE_NONE = 0x0,
    MTA_HOST_USAGE_MTAINITIALIZED = 0x1,
    MTA_HOST_USAGE_ACTIVATORINITIALIZED = 0x2,
    MTA_HOST_USAGE_UNLOADCALLED = 0x4,
} MTA_HOST_USAGE_FLAGS, *PMTA_HOST_USAGE_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(MTA_HOST_USAGE_FLAGS);

// private
/**
 * The MTA_USAGE_GLOBALS structure contains the global state used to track COM multithreaded apartment (MTA) usage.
 */
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
/**
 * The CoGetMTAUsageInfo routine retrieves the global multithreaded apartment (MTA) usage tracking structure.
 *
 * \return A pointer to the MTA_USAGE_GLOBALS structure if MTA usage information is available; otherwise, NULL.
 */
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

// private // ole32.dll
/**
 * The UpdateDCOMSettings routine updates the system Distributed COM (DCOM) settings and security configuration.
 */
WINBASEAPI
VOID
WINAPI
UpdateDCOMSettings(
    VOID
    );

//
// AppCompat
//

/**
 * The SDBQUERYRESULT structure contains the result of an application compatibility database (SDB) query.
 */
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

/**
 * The SWITCH_CONTEXT_ATTRIBUTE structure describes a single attribute of an application compatibility switch context.
 */
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

/**
 * The SWITCH_CONTEXT_DATA structure contains the data for an application compatibility switch context.
 */
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

/**
 * The SWITCH_CONTEXT structure contains an application compatibility switch context.
 */
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

/**
 * The SDB_CSTRUCT_COBALT_PROCFLAG structure contains the Cobalt process flags from the application compatibility database (SDB).
 */
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
    } DUMMYUNIONNAME;
} SDB_CSTRUCT_COBALT_PROCFLAG, *PSDB_CSTRUCT_COBALT_PROCFLAG;

#ifdef _WIN64
static_assert(sizeof(SDB_CSTRUCT_COBALT_PROCFLAG) == 0x28, "SDB_CSTRUCT_COBALT_PROCFLAG size mismatch");
#else
static_assert(sizeof(SDB_CSTRUCT_COBALT_PROCFLAG) == 0x20, "SDB_CSTRUCT_COBALT_PROCFLAG size mismatch");
#endif

/**
 * The APPCOMPAT_EXE_DATA structure contains the application compatibility data for an executable image.
 */
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

//
// Performance Counters for Windows
//

/**
 * PCW Handle Types  
 */
DECLARE_HANDLE(HPCW_REGISTRATION);  
DECLARE_HANDLE(HPCW_QUERY);
DECLARE_HANDLE(HPCW_NOTIFIER);

/**
 * PCW Callback Types  
 */  
typedef enum _PCW_CALLBACK_TYPE
{  
    PcwCallbackAddCounter = 0,  
    PcwCallbackRemoveCounter = 1,  
    PcwCallbackEnumerateEvents = 2,  
    PcwCallbackCollectData = 3,  
} PCW_CALLBACK_TYPE;

/**
 * The PCW_CALLBACK_INFORMATION structure contains the information passed to a Performance Counters for Windows (PCW) callback.
 */
typedef struct _PCW_CALLBACK_INFORMATION
{  
    PCW_CALLBACK_TYPE Type;  
    union
    {
        struct
        {
            PUNICODE_STRING InstanceName;
            PVOID InstanceData;
            ULONG InstanceId;
        } AddCounter;
        struct
        {
            PVOID InstanceContext;
        } RemoveCounter;
        struct
        {
            PVOID CancelEvent;
        } CollectData;  
    } DUMMYUNIONNAME;
} PCW_CALLBACK_INFORMATION, *PPCW_CALLBACK_INFORMATION;

typedef _Function_class_(PCW_CALLBACK)
NTSTATUS NTAPI PCW_CALLBACK(
    _In_ PCW_CALLBACK_TYPE Type,
    _In_ PPCW_CALLBACK_INFORMATION Info,
    _In_opt_ PVOID Context
    );
typedef PCW_CALLBACK* PPCW_CALLBACK;

/**
 * Creates a new Performance Counters for Windows (PCW) query object.
 *
 * \param[out] QueryHandle Receives the created `HPCW_QUERY` handle on success.
 * \param[in,opt] CancelEventHandle Optional event handle that the kernel monitors to detect
 * cancellation of long-running operations (for example, during `PcwCollectData`).
 * The kernel only reads this handle; it may be `NULL` if cancellation support is not required.
 * \return NTSTATUS Returns `STATUS_SUCCESS` on success; otherwise an appropriate NTSTATUS error code.
 * \remark Usermode requests cancellation by signaling `CancelEventHandle` from another thread.
 */
NTSYSAPI
NTSTATUS
NTAPI
PcwCreateQuery(
    _Out_ HPCW_QUERY* QueryHandle,
    _In_opt_ HANDLE CancelEventHandle
    );

/**
 * The PCW_ADD_QUERY_ITEM_FLAGS enumeration defines the flags used when adding a query item to a PCW query.
 */
typedef enum _PCW_ADD_QUERY_ITEM_FLAGS
{
    PCW_ADD_QUERY_ITEM_NONE = 0x0,
    PCW_ADD_QUERY_ITEM_INSTANCE_WILDCARD = 0x1,
} PCW_ADD_QUERY_ITEM_FLAGS;

/**
 * The PcwAddQueryItem routine adds a counter query item to a Performance Counters for Windows (PCW) query.
 *
 * \param[out] ItemId Pointer to a variable that receives the identifier assigned to the query item.
 * \param[in] QueryHandle Handle to the PCW query.
 * \param[in] Flags Flags that control the query item.
 * \param[in] CounterSetPath The path of the counter set to query.
 * \param[in] InstanceName The name of the instance to query.
 * \param[in] InstanceId The identifier of the instance to query.
 * \param[in] CounterMask A bitmask that selects the counters to include.
 * \param[in, optional] UserData Caller-defined data to associate with the query item.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
PcwAddQueryItem(
    _Out_ PULONG ItemId,
    _In_ HPCW_QUERY QueryHandle,
    _In_ PCW_ADD_QUERY_ITEM_FLAGS Flags,
    _In_ PCUNICODE_STRING CounterSetPath,
    _In_ PCUNICODE_STRING InstanceName,
    _In_ ULONG InstanceId,
    _In_ ULONG64 CounterMask,
    _In_opt_ PVOID UserData
    );

/**
 * The PcwCollectData routine collects counter data for a Performance Counters for Windows (PCW) query.
 *
 * \param[in] QueryHandle Handle to the PCW query.
 * \param[out] Buffer Pointer to a buffer that receives the collected counter data.
 * \param[in] BufferSize The size, in bytes, of the buffer.
 * \param[out] BytesReturned Pointer to a variable that receives the number of bytes written.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
PcwCollectData(
    _In_ HPCW_QUERY QueryHandle,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_ PULONG BytesReturned
    );

/**
 * The PcwRemoveQueryItem routine removes a counter query item from a Performance Counters for Windows (PCW) query.
 *
 * \param[in] QueryHandle Handle to the PCW query.
 * \param[in] ItemId The identifier of the query item to remove.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
PcwRemoveQueryItem(
    _In_ HPCW_QUERY QueryHandle,
    _In_ ULONG ItemId
    );

/**
 * The PcwSetQueryItemUserData routine sets the caller-defined data associated with a PCW query item.
 *
 * \param[in] QueryHandle Handle to the PCW query.
 * \param[in] ItemId The identifier of the query item.
 * \param[in] UserData The caller-defined data to associate with the query item.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
PcwSetQueryItemUserData(
    _In_ HPCW_QUERY QueryHandle,
    _In_ ULONG ItemId,
    _In_ PVOID UserData
    );

// HPCW_NOTIFIER APIs

/**
 * The PcwCreateNotifier routine creates a Performance Counters for Windows (PCW) notifier.
 *
 * \param[out] NotifierHandle Pointer to a variable that receives the notifier handle.
 * \param[in] Name The name of the notifier.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
PcwCreateNotifier(
    _Out_ HPCW_NOTIFIER* NotifierHandle,
    _In_ PCUNICODE_STRING Name
    );

/**
 * The PcwIsNotifierAlive routine determines whether a Performance Counters for Windows (PCW) notifier is still alive.
 *
 * \param[out] Alive Pointer to a boolean receiving whether the notifier is alive.
 * \param[in] Registration Handle to the PCW registration.
 * \param[in] NotificationId Pointer to the notification ID.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
PcwIsNotifierAlive(
    _Out_ PBOOLEAN Alive,
    _In_ HANDLE Registration,
    _In_ const ULONGLONG *NotificationId
    );

/**
 * The PcwReadNotificationData routine reads pending notification data from a Performance Counters for Windows (PCW) notifier.
 *
 * \param[in] NotifierHandle Handle to the PCW notifier.
 * \param[out] Buffer Pointer to a buffer that receives the notification data.
 * \param[in] BufferSize The size, in bytes, of the buffer.
 * \param[out] BytesReturned Pointer to a variable that receives the number of bytes written.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
PcwReadNotificationData(
    _In_ HPCW_NOTIFIER NotifierHandle,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_ PULONG BytesReturned
    );

/**
 * The PcwCompleteNotification routine completes a pending Performance Counters for Windows (PCW) notification.
 *
 * \param[out] Result Pointer to a boolean receiving the result.
 * \param[in] Notifier Handle to the PCW notifier.
 * \param[in] Status The completion status of the notification.
 * \param[in, optional] UserData Caller-defined data associated with the completion.
 * \param[in] UserDataSize The size, in bytes, of the user data.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
PcwCompleteNotification(
    _Out_ PBOOLEAN Result,
    _In_ HANDLE Notifier,
    _In_ NTSTATUS Status,
    _In_opt_ PVOID UserData,
    _In_ ULONG UserDataSize
    );

/**
 * The PcwRegisterCounterSet routine registers a Performance Counters for Windows (PCW) counter set.
 *
 * \param[out] Registration Pointer to a variable that receives the registration handle.
 * \param[in] Name The name of the counter set.
 * \param[in, optional] NotificationFile Optional handle to the notification file.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
PcwRegisterCounterSet(
    _Out_ PHANDLE Registration,
    _In_ PCUNICODE_STRING Name,
    _In_opt_ HANDLE NotificationFile
    );

/**
 * The PcwDisconnectCounterSet routine disconnects a registered Performance Counters for Windows (PCW) counter set.
 *
 * \param[out] Result Pointer to a boolean receiving the result.
 * \param[in] Registration Handle to the PCW counter set registration.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
PcwDisconnectCounterSet(
    _Out_ PBOOLEAN Result,
    _In_ HANDLE Registration
    );

/**
 * The PcwEnumerateInstances routine enumerates the instances of a registered Performance Counters for Windows (PCW) counter set.
 *
 * \param[in] RegistrationHandle Handle to the PCW counter set registration.
 * \param[in] CounterSetPath The path of the counter set.
 * \param[in] InstanceName The name of the instance to enumerate.
 * \param[out] Buffer Pointer to a buffer that receives the enumerated instance data.
 * \param[in] BufferSize The size, in bytes, of the buffer.
 * \param[out] BytesReturned Pointer to a variable that receives the number of bytes written.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
PcwEnumerateInstances(
    _In_ HPCW_REGISTRATION RegistrationHandle,
    _In_ PCUNICODE_STRING CounterSetPath,
    _In_ PCUNICODE_STRING InstanceName,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_ PULONG BytesReturned
    );

/**
 * The PcwQueryCounterSetSecurity routine queries the security descriptor of a Performance Counters for Windows (PCW) counter set.
 *
 * \param[in] Name The name of the counter set.
 * \param[in] SecurityInformation The security information to query.
 * \param[out, optional] SecurityDescriptor Pointer to a buffer that receives the security descriptor.
 * \param[in] BufferSize The size, in bytes, of the buffer.
 * \param[out] BytesReturned Pointer to a variable that receives the number of bytes required or written.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
PcwQueryCounterSetSecurity(
    _In_ PCUNICODE_STRING Name,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_writes_bytes_opt_(BufferSize) PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ ULONG BufferSize,
    _Out_ PULONG BytesReturned
    );

/**
 * The PcwSendNotification routine sends a notification to a registered Performance Counters for Windows (PCW) counter set.
 *
 * \param[in] RegistrationHandle Handle to the PCW counter set registration.
 * \param[in] NotificationType The type of notification to send.
 * \param[in, optional] NotificationData Pointer to the notification data.
 * \param[in, optional] InstanceName The name of the instance associated with the notification.
 * \param[in] InstanceId The identifier of the instance.
 * \param[out, optional] OutputBuffer Pointer to a buffer that receives the notification response.
 * \param[in] OutputBufferSize The size, in bytes, of the output buffer.
 * \param[out, optional] BytesReturned Pointer to a variable that receives the number of bytes written.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
PcwSendNotification(
    _In_ HPCW_REGISTRATION RegistrationHandle,
    _In_ ULONG NotificationType,
    _In_opt_ PVOID NotificationData,
    _In_opt_ PCUNICODE_STRING InstanceName,
    _In_ ULONG InstanceId,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ ULONG OutputBufferSize,
    _Out_opt_ PULONG BytesReturned
    );

/**
 * The PcwSendStatelessNotification routine sends a stateless notification to a Performance Counters for Windows (PCW) counter set.
 *
 * \param[in] CounterSetName The name of the counter set.
 * \param[in] NotificationType The type of notification to send.
 * \param[in, optional] NotificationData Pointer to the notification data.
 * \param[in, optional] InstanceName The name of the instance associated with the notification.
 * \param[in] InstanceId The identifier of the instance.
 * \param[out, optional] OutputBuffer Pointer to a buffer that receives the notification response.
 * \param[in] OutputBufferSize The size, in bytes, of the output buffer.
 * \param[out, optional] BytesReturned Pointer to a variable that receives the number of bytes written.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
PcwSendStatelessNotification(
    _In_ PCUNICODE_STRING CounterSetName,
    _In_ ULONG NotificationType,
    _In_opt_ PVOID NotificationData,
    _In_opt_ PCUNICODE_STRING InstanceName,
    _In_ ULONG InstanceId,
    _Out_writes_bytes_opt_(OutputBufferSize) PVOID OutputBuffer,
    _In_ ULONG OutputBufferSize,
    _Out_opt_ PULONG BytesReturned
    );

/**
 * The PcwSetCounterSetSecurity routine sets the security descriptor of a Performance Counters for Windows (PCW) counter set.
 *
 * \param[in] Name The name of the counter set.
 * \param[in] SecurityInformation The security information to set.
 * \param[in] SecurityDescriptor Pointer to the security descriptor to apply.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
PcwSetCounterSetSecurity(
    _In_ PCUNICODE_STRING Name,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    );

/**
 * The PcwClearCounterSetSecurity routine clears the security descriptor of a Performance Counters for Windows (PCW) counter set, restoring the default.
 *
 * \param[in] Name The name of the counter set.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
PcwClearCounterSetSecurity(
    _In_ PCUNICODE_STRING Name
    );

//
// PCW Buffer Structures and Parsing Helpers
//

/**
 * PCW query snapshot - represents a data collection snapshot
 */
typedef struct _PCW_QUERY_SNAPSHOT
{
    PVOID RawBuffer;
    ULONG64 Reserved08;
    ULONG64 TimestampA;
    ULONG64 TimestampB;
} PCW_QUERY_SNAPSHOT, *PPCW_QUERY_SNAPSHOT;

/**
 * Root header for PCW result data
 */
typedef struct _PCW_RESULT_ROOT
{
    ULONG HeaderSize;
    ULONG HeaderOffset;
} PCW_RESULT_ROOT, *PPCW_RESULT_ROOT;

/**
 * Header describing PCW counter set results
 */
typedef struct _PCW_RESULT_HEADER
{
    ULONG Size;
    ULONG InstanceListOffset;
    ULONG InstanceCount;
} PCW_RESULT_HEADER, *PPCW_RESULT_HEADER;

/**
 * PCW instance buffer - contains instance data and counter values
 */
typedef struct _PCW_INSTANCE_BUFFER
{
    ULONG Size;
    ULONG Index;
    ULONG CounterDataOffset;
    ULONG Reserved0C;
    WCHAR InstanceName[ANYSIZE_ARRAY];
} PCW_INSTANCE_BUFFER, *PPCW_INSTANCE_BUFFER;

/**
 * PCW counter record - single counter value
 */
typedef struct _PCW_COUNTER_RECORD
{
    ULONG Size;
    union
    {
        ULONG CounterIdAndValueSize;
        struct
        {
            USHORT CounterId;
            USHORT ValueSize;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    ULONG64 Value;
} PCW_COUNTER_RECORD, *PPCW_COUNTER_RECORD;

/**
 * PCW counter trailer - metadata following counter data
 */
typedef struct _PCW_COUNTER_TRAILER
{
    ULONG Size;
    union
    {
        ULONG CounterIdAndValueSize;
        struct
        {
            USHORT CounterId;
            USHORT ValueSize;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    ULONG TickCount;
} PCW_COUNTER_TRAILER, *PPCW_COUNTER_TRAILER;

/**
 * PCW enumerate instances header
 */
typedef struct _PCW_ENUMERATE_INSTANCES_HEADER
{
    ULONG InstanceCount;
    ULONG FirstInstanceOffset;
} PCW_ENUMERATE_INSTANCES_HEADER, *PPCW_ENUMERATE_INSTANCES_HEADER;

/**
 * PCW enumerated instance
 */
typedef struct _PCW_ENUMERATE_INSTANCE
{
    ULONG Size;
    ULONG InstanceId;
    WCHAR InstanceName[ANYSIZE_ARRAY];
} PCW_ENUMERATE_INSTANCE, *PPCW_ENUMERATE_INSTANCE;

//
// PCW Buffer Parsing Helper Functions
//

/**
 * Retrieves the header from a PCW query snapshot buffer.
 */
FORCEINLINE
PPCW_RESULT_HEADER
NTAPI
PcwQueryGetHeader(
    _In_ PPCW_QUERY_SNAPSHOT Snapshot
    )
{
    PPCW_RESULT_ROOT root;

    root = (PPCW_RESULT_ROOT)Snapshot->RawBuffer;
    return (PPCW_RESULT_HEADER)RTL_PTR_ADD(Snapshot->RawBuffer, root->HeaderOffset);
}

/**
 * Retrieves the first instance from a PCW result header.
 */
FORCEINLINE
PPCW_INSTANCE_BUFFER
NTAPI
PcwHeaderGetFirstInstance(
    _In_ PPCW_RESULT_HEADER Header
    )
{
    return (PPCW_INSTANCE_BUFFER)RTL_PTR_ADD(Header, Header->InstanceListOffset);
}

/**
 * Retrieves the next instance in a PCW instance sequence.
 */
FORCEINLINE
PPCW_INSTANCE_BUFFER
NTAPI
PcwInstanceGetNext(
    _In_ PPCW_INSTANCE_BUFFER Instance
    )
{
    return (PPCW_INSTANCE_BUFFER)RTL_PTR_ADD(Instance, Instance->Size);
}

/**
 * Retrieves the first counter record from a PCW instance.
 */
FORCEINLINE
PPCW_COUNTER_RECORD
NTAPI
PcwInstanceGetFirstCounter(
    _In_ PPCW_INSTANCE_BUFFER Instance
    )
{
    return (PPCW_COUNTER_RECORD)RTL_PTR_ADD(Instance, Instance->CounterDataOffset);
}

/**
 * Retrieves the next counter record in a PCW counter sequence.
 */
FORCEINLINE
PPCW_COUNTER_RECORD
NTAPI
PcwCounterGetNext(
    _In_ PPCW_COUNTER_RECORD Counter
    )
{
    return (PPCW_COUNTER_RECORD)RTL_PTR_ADD(Counter, Counter->Size);
}

/**
 * Retrieves the trailer following a PCW counter record.
 */
FORCEINLINE
PPCW_COUNTER_TRAILER
NTAPI
PcwCounterGetTrailer(
    _In_ PPCW_COUNTER_RECORD Counter
    )
{
    return (PPCW_COUNTER_TRAILER)RTL_PTR_ADD(Counter, Counter->Size);
}

/**
 * Retrieves the first enumerated instance from a PCW enumerate header.
 */
FORCEINLINE
PPCW_ENUMERATE_INSTANCE
NTAPI
PcwEnumerateHeaderGetFirstInstance(
    _In_ PPCW_ENUMERATE_INSTANCES_HEADER Header
    )
{
    return (PPCW_ENUMERATE_INSTANCE)RTL_PTR_ADD(Header, Header->FirstInstanceOffset);
}

/**
 * Retrieves the next enumerated instance.
 */
FORCEINLINE
PPCW_ENUMERATE_INSTANCE
NTAPI
PcwEnumerateInstanceGetNext(
    _In_ PPCW_ENUMERATE_INSTANCE Instance
    )
{
    return (PPCW_ENUMERATE_INSTANCE)RTL_PTR_ADD(Instance, Instance->Size);
}

#endif // _NTMISC_H
