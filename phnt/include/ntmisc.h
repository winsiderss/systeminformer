/*
 * Trace Control support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTMISC_H
#define _NTMISC_H

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

#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenSession(
    _Out_ PHANDLE SessionHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_VISTA)

#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

#if (PHNT_VERSION >= PHNT_WINDOWS_7)
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
#endif // (PHNT_VERSION >= PHNT_WINDOWS_7)

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
#endif

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
#endif

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
    _Reserved_ PROCESS_ACTIVITY_TYPE Reserved
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
    _Out_ PACKAGE_APPLICATION_CONTEXT_REFERENCE *PackageResourcesContext
    );

// rev
WINBASEAPI
LONG
WINAPI
GetPackageApplicationResourcesContext(
    _In_ PVOID PackageInfoReference, // PACKAGE_INFO_REFERENCE
    _In_ ULONG Index,
    _Reserved_ ULONG_PTR Unused,
    _Out_ PACKAGE_APPLICATION_CONTEXT_REFERENCE *PackageResourcesContext
    );

// rev
WINBASEAPI
LONG
WINAPI
GetPackageResourcesProperty(
    _In_ PACKAGE_APPLICATION_CONTEXT_REFERENCE PackageResourcesContext,
    _In_ PackageResourcesProperty PropertyId,
    _Inout_ PULONG BufferSize,
    _Out_writes_bytes_(BufferSize) PVOID Buffer
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
    _In_ ULONG flags,
    _In_ ULONG packagePathType, // PackagePathType
    _Inout_ PULONG bufferLength,
    _Out_writes_bytes_opt_(*bufferLength) PVOID buffer,
    _Out_opt_ PULONG count
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

#endif
