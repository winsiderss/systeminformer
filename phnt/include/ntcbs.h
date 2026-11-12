/*
 * Component-Based Servicing (CBS) private interfaces and core exports.
 *
 * This file is part of System Informer.
 */

#ifndef _NTCBS_H
#define _NTCBS_H

#include <limits.h>
#include <objbase.h>
#include <objidl.h>

// Based on the KNSoft.NDK analysis of Windows build 26100.9156 (x86 and x64).
// These private interfaces use explicit C vtables in both C and C++.
// Unknown slots and values preserve the limits of that analysis.

#ifdef __cplusplus
extern "C" {
#endif

#define CBS_E_ARRAY_MISSING_INDEX _HRESULT_TYPEDEF_(0x800F0809L)
#define CBS_E_UNKNOWN_UPDATE _HRESULT_TYPEDEF_(0x800F080CL)

typedef enum _CBS_APPLICABILITY
{
    CbsApplicabilityInvalid = -1,
    CbsApplicabilityAll = 0,
    CbsApplicabilityNotApplicable = 1,
    CbsApplicabilityNeedsParent = 2,
    CbsApplicabilityApplicable = 4
} CBS_APPLICABILITY, *PCBS_APPLICABILITY;

typedef enum _CBS_SELECTABILITY
{
    CbsSelectabilityInvalid = -1,
    CbsSelectabilityAll = 0,
    CbsSelectabilitySon = 1,
    CbsSelectabilityRoot = 2
} CBS_SELECTABILITY, *PCBS_SELECTABILITY;

typedef enum _CBS_INSTALL_STATE
{
    CbsInstallStatePartiallyInstalled = -19,
    CbsInstallStateCancel = -18,
    CbsInstallStateSuperseded = -17,
    CbsInstallStateDefault = -16,
    CbsInstallStateInvalidPermanent = -8,
    CbsInstallStateInvalidInstalled = -7,
    CbsInstallStateInvalidStaged = -4,
    CbsInstallStateInvalidResolved = -2,
    CbsInstallStateUnknown = -1,
    CbsInstallStateAbsent = 0,
    CbsInstallStateResolving = 1,
    CbsInstallStateResolved = 2,
    CbsInstallStateStaging = 3,
    CbsInstallStateStaged = 4,
    CbsInstallStateUninstallRequested = 5,
    CbsInstallStateInstallRequested = 6,
    CbsInstallStateInstalled = 7,
    CbsInstallStatePermanent = 8,
    CbsInstallStateInvalid = INT_MAX
} CBS_INSTALL_STATE, *PCBS_INSTALL_STATE;

typedef enum _CBS_REQUIRED_ACTION
{
    CbsRequiredActionNone = 0,
    CbsRequiredActionReboot = 1
} CBS_REQUIRED_ACTION, *PCBS_REQUIRED_ACTION;

typedef enum _CBS_PACKAGE_TYPE
{
    CbsPackageTypeUnknown = -2,
    CbsPackageTypeExisting = -1,
    CbsPackageTypeCabinet = 0,
    CbsPackageTypeExpanded = 1,
    CbsPackageTypeManifest = 2,
    CbsPackageTypeXmlString = 3,
    CbsPackageTypeExpandedWithMum = 4
} CBS_PACKAGE_TYPE, *PCBS_PACKAGE_TYPE;

typedef enum _CBS_PACKAGE_PROPERTY
{
    CbsPackagePropertyIdentityString = 1,
    CbsPackagePropertyDisplayName = 2,
    CbsPackagePropertyDescription = 3,
    CbsPackagePropertyRestart = 4,
    CbsPackagePropertyInstallGroup = 5,
    CbsPackagePropertyHidden = 6,
    CbsPackagePropertyKeyword = 7,
    CbsPackagePropertyReleaseType = 8,
    CbsPackagePropertyProductName = 9,
    CbsPackagePropertyProductVersion = 10,
    CbsPackagePropertyPermanence = 11,
    CbsPackagePropertyCompany = 12,
    CbsPackagePropertyCopyright = 13,
    CbsPackagePropertySupportInformation = 14,
    CbsPackagePropertyCreationTimeStamp = 15,
    CbsPackagePropertyLastUpdateTimeStamp = 16,
    CbsPackagePropertyInstallTimeStamp = 17,
    CbsPackagePropertyInstallPackageName = 18,
    CbsPackagePropertyInstallLocation = 19,
    CbsPackagePropertyInstallClient = 20,
    CbsPackagePropertyInstallUserName = 21,
    CbsPackagePropertyExtendedError = 22,
    CbsPackagePropertyPended = 23,
    CbsPackagePropertyHotpatch = 24,
    CbsPackagePropertyExclusive = 25,
    CbsPackagePropertyAllowedOffline = 26,
    CbsPackagePropertyCompletelyOfflineCapable = 27,
    CbsPackagePropertyScavengeSequence = 28,
    CbsPackagePropertyPackageSize = 29,
    CbsPackagePropertySupersededTime = 30,
    CbsPackagePropertyStackUpdate = 31,
    CbsPackagePropertyReleaseQuality = 32,
    CbsPackagePropertyTargetPartition = 33,
    CbsPackagePropertyBinaryPartition = 34,
    CbsPackagePropertyCapabilityIdentity = 35,
    CbsPackagePropertyUnknown36 = 36,
    CbsPackagePropertyUnknown37 = 37,
    CbsPackagePropertyRequireSatelliteLanguage = 38,
    CbsPackagePropertyRequireSatelliteArch = 39,
    CbsPackagePropertyUnknown40 = 40,
    CbsPackagePropertyFeatureFmid = 41,
    CbsPackagePropertyFeatureGroup = 42,
    CbsPackagePropertyPermanentUntilReset = 43,
    CbsPackagePropertySatelliteType = 44,
    CbsPackagePropertyDeclareSatelliteLanguage = 45,
    CbsPackagePropertyDeclareSatelliteArch = 46,
    CbsPackagePropertyRequiresPriorReboot = 47,
    CbsPackagePropertyUnknown48 = 48,
    CbsPackagePropertyIsMumServicingLcuPackage = 49,
    CbsPackagePropertyIsInstalled = 50,
    CbsPackagePropertyRemoveOnReset = 51,
    CbsPackagePropertyApplicablePartitions = 52,
    CbsPackagePropertyOsUpgradePackage = 53,
    CbsPackagePropertyIsLanguageSatellitePackage = 54,
    CbsPackagePropertyIsArchSatellitePackage = 55,
    CbsPackagePropertyBaseline = 56,
    CbsPackagePropertyIsSelfOwned = 57
} CBS_PACKAGE_PROPERTY, *PCBS_PACKAGE_PROPERTY;

typedef enum _CBS_UPDATE_PROPERTY
{
    CbsUpdatePropertyName = 1,
    CbsUpdatePropertyDisplayName = 2,
    CbsUpdatePropertyDescription = 3,
    CbsUpdatePropertyUnknown4 = 4,
    CbsUpdatePropertyRestart = 5,
    CbsUpdatePropertyUnknown6 = 6,
    CbsUpdatePropertyDownloadSize = 7
} CBS_UPDATE_PROPERTY, *PCBS_UPDATE_PROPERTY;

typedef enum _CBS_RESOURCE_TYPE
{
    CbsResourceTypeUnknown = -1,
    CbsResourceTypeFilesInUse = 1,
    CbsResourceTypeServicesToStop = 2,
    // Returns E_NOTIMPL in the examined build.
    CbsResourceTypeProcess = 3
} CBS_RESOURCE_TYPE, *PCBS_RESOURCE_TYPE;

typedef enum _CBS_UI_RESPONSE
{
    CbsUIResponseContinue = 1,
    CbsUIResponseCancel = 2,
    CbsUIResponseAbort = 3,
    CbsUIResponseUnknown4 = 4,
    CbsUIResponseUnknown5 = 5,
    CbsUIResponseNormalPriority = 0x1000,
    CbsUIResponseUserPresentPriority = 0x1001,
    CbsUIResponseUserAwayPriority = 0x1002
} CBS_UI_RESPONSE, *PCBS_UI_RESPONSE;

typedef enum _CBS_CARDINALITY
{
    CbsCardinalityUnknown = -1
} CBS_CARDINALITY, *PCBS_CARDINALITY;

typedef enum _CBS_PACKAGE_ENCRYPTION
{
    CbsPackageEncryptionUnknown = -1,
    CbsPackageEncryptionNone = 0
} CBS_PACKAGE_ENCRYPTION, *PCBS_PACKAGE_ENCRYPTION;

#define CBS_PACKAGE_CHANGE_OPTION_UNKNOWN1 0x1
#define CBS_PACKAGE_CHANGE_OPTION_UNKNOWN2 0x2
#define CBS_PACKAGE_CHANGE_OPTION_UNKNOWN4 0x4
#define CBS_PACKAGE_CHANGE_OPTION_UNKNOWN8 0x8
#define CBS_PACKAGE_CHANGE_OPTION_UNKNOWN10 0x10
#define CBS_PACKAGE_CHANGE_OPTION_UNKNOWN20 0x20
#define CBS_PACKAGE_CHANGE_OPTION_UNKNOWN40 0x40
#define CBS_PACKAGE_CHANGE_OPTION_UNKNOWN100 0x100
// Internal action-list option; excluded from the public package-change mask.
#define CBS_PACKAGE_CHANGE_OPTION_STAGE_PARTIAL 0x200
#define CBS_PACKAGE_CHANGE_OPTION_UNKNOWN4000 0x4000
#define CBS_PACKAGE_CHANGE_OPTION_TREAT_PACKAGE_AS_PSFX 0x8000
#define CBS_PACKAGE_CHANGE_OPTION_UNKNOWN10000 0x10000
#define CBS_PACKAGE_CHANGE_OPTION_UNKNOWN40000 0x40000
#define CBS_PACKAGE_CHANGE_OPTION_UNKNOWN80000 0x80000
#define CBS_PACKAGE_CHANGE_OPTION_INSTALL_NON_BASELINE_FOD_OR_LANGUAGE_PACK 0x100000
#define CBS_PACKAGE_CHANGE_OPTION_PUBLIC_VALID_MASK 0x1DC17F
#define CBS_CAPABILITY_CHANGE_OPTION_PUBLIC_VALID_MASK 0x1007F

#define CBS_SESSION_FINALIZE_OPTION_CANCEL_PENDING 0x800

#define CBS_SERVICING_PROCESSOR_OPTION_UNKNOWN1 0x1
#define CBS_SERVICING_PROCESSOR_OPTION_UNKNOWN2 0x2
#define CBS_SERVICING_PROCESSOR_OPTION_UNKNOWN4 0x4
#define CBS_SERVICING_PROCESSOR_OPTION_UNKNOWN8 0x8
#define CBS_SERVICING_PROCESSOR_OPTION_UNKNOWN10 0x10
#define CBS_SERVICING_PROCESSOR_OPTION_LATE_ACQUISITION 0x20
#define CBS_SERVICING_PROCESSOR_OPTION_MULTIPLE_PACKAGES 0x40
#define CBS_SERVICING_PROCESSOR_OPTION_UNKNOWN80 0x80
#define CBS_SERVICING_PROCESSOR_OPTION_USER_PRESENT_PRIORITY 0x100
#define CBS_SERVICING_PROCESSOR_OPTION_USER_AWAY_PRIORITY 0x200
#define CBS_SERVICING_PROCESSOR_OPTION_STAGE_ONLY 0x400
#define CBS_SERVICING_PROCESSOR_OPTION_BASELINE_MFL_ONLY 0x800
#define CBS_SERVICING_PROCESSOR_OPTION_HYDRATE_ONLY 0x1000
#define CBS_SERVICING_PROCESSOR_OPTION_DISABLE_PARALLEL_HYDRATION 0x2000
#define CBS_SERVICING_PROCESSOR_OPTION_SSU_ONLY 0x4000
#define CBS_SERVICING_PROCESSOR_PROCESS_VALID_OPTION_MASK 0x773F
#define CBS_SERVICING_PROCESSOR_WRITE_FILE_LIST_VALID_OPTION_MASK 0xBC0

typedef enum _CBS_SESSION_OPTION
{
    CbsSessionOptionNone = 0,
    CbsSessionOptionUnknown00000040 = 0x00000040,
    CbsSessionOptionLoadPersisted = 0x00000080,
    CbsSessionOptionDoScavenge = 0x00000400,
    CbsSessionOptionCancelAllPendedTransactions = 0x00000800,
    CbsSessionOptionEnableCompression = 0x00002000,
    CbsSessionOptionDisableCompression = 0x00004000,
    CbsSessionOptionDetectAndRepairStoreCorruption = 0x0000C000,
    CbsSessionOptionNoPend = 0x00010000,
    CbsSessionOptionUnknown00020000 = 0x00020000,
    CbsSessionOptionUnknown00040000 = 0x00040000,
    CbsSessionOptionReportStackInfo = 0x00100000,
    CbsSessionOptionDoSynchronousCleanup = 0x00400000,
    CbsSessionOptionUnknown00800000 = 0x00800000,
    CbsSessionOptionUnknown02000000 = 0x02000000,
    CbsSessionOptionAnalyzeComponentStore = 0x08000000,
    CbsSessionOptionCancelOnlySmartPendedTransactions = 0x80000000
} CBS_SESSION_OPTION, *PCBS_SESSION_OPTION;

typedef enum _CBS_SESSION_STATE
{
    CbsSessionStateUnknown = 0,
    CbsSessionStateReady = 16,
    CbsSessionStateQueued = 32,
    CbsSessionStateStarted = 48,
    CbsSessionStatePlanned = 64,
    CbsSessionStateResolved = 80,
    CbsSessionStateStaged = 96,
    CbsSessionStateExecutionDelayed = 101,
    CbsSessionStateInstalled = 112,
    CbsSessionStatePended = 128,
    CbsSessionStateShutdownStart = 144,
    CbsSessionStateShutdownFinish = 160,
    CbsSessionStateStartup = 176,
    CbsSessionStateStartupFinish = 192,
    CbsSessionStateComplete = 208,
    CbsSessionStateInterrupted = 224,
    CbsSessionStateCorrupted = 240,
    CbsSessionStateMarkedForRetry = 256
} CBS_SESSION_STATE, *PCBS_SESSION_STATE;

typedef enum _CBS_OPERATION_STAGE
{
    CbsOperationStageWaiting = 1,
    CbsOperationStagePlanning = 5,
    CbsOperationStageDownloading = 15,
    CbsOperationStageExtracting = 20,
    CbsOperationStageResolving = 25,
    CbsOperationStageStaging = 30,
    CbsOperationStageDownloadingLcu = 40,
    CbsOperationStageInstalling = 50,
    CbsOperationStageReservicingLcu = 60
} CBS_OPERATION_STAGE, *PCBS_OPERATION_STAGE;

typedef enum _CBS_OPERATION_TYPE
{
    CbsOperationTypeNone = 0,
    CbsOperationTypeExportRepository = 1,
    CbsOperationTypeUpdateImage = 2,
    CbsOperationTypePrepareServicing = 3,
    CbsOperationTypeLateAcquisition = 4,
    // The following three values are accepted without execution branches in the examined build.
    CbsOperationTypeUnknown5 = 5,
    CbsOperationTypeUnknown6 = 6,
    CbsOperationTypeUnknown7 = 7,
    CbsOperationTypeInitializeCsiStore = 8,
    CbsOperationTypeSetReservesRebootPending = 9,
    CbsOperationTypeOfflineFinalizeInstallation = 10,
    CbsOperationTypeOfflineRollbackInstallation = 11,
    CbsOperationTypeGenerateRepairFileList = 12,
    CbsOperationTypePerformRepairWithExternalContent = 13
} CBS_OPERATION_TYPE, *PCBS_OPERATION_TYPE;

typedef enum _CBS_SESSION_PROPERTY
{
    CbsSessionPropertyRebootRequired = 1,
    CbsSessionPropertyErrorDetail = 2,
    CbsSessionPropertyServiceable = 3,
    CbsSessionPropertyCompressionEnabled = 4,
    CbsSessionPropertyReport = 5,
    CbsSessionPropertyCorruptionFlag = 6,
    CbsSessionPropertyInRepairOperation = 7,
    CbsSessionPropertyVolatileSize = 8,
    CbsSessionPropertyNonVolatileSize = 9,
    CbsSessionPropertySharedWithWindowsSize = 10,
    CbsSessionPropertyAccordingToExplorerSize = 11,
    CbsSessionPropertyLastScavengeDateTime = 12,
    CbsSessionPropertySupersededPackageCount = 13,
    CbsSessionPropertyCleanupRecommended = 14,
    CbsSessionPropertySessionCompletionDateTime = 15,
    CbsSessionPropertyUnknown16 = 16,
    CbsSessionPropertyUnknown17 = 17,
    CbsSessionPropertyFeaturesToRetry = 19,
    CbsSessionPropertyFodRetry = 20,
    CbsSessionPropertyLcuReoffer = 21,
    CbsSessionPropertyRepairNeeded = 22,
    CbsSessionPropertyShutdownTime = 23,
    CbsSessionPropertyRebootTime = 24,
    CbsSessionPropertyPostRebootTime = 25,
    CbsSessionPropertyUnknown26 = 26,
    CbsSessionPropertyLcuPending = 27,
    CbsSessionPropertyDeepReoffer = 28,
    CbsSessionPropertyUpdateAgentPath = 29,
    CbsSessionPropertyFodMetadataPaths = 30,
    CbsSessionPropertyComponentDatabasePaths = 31,
    CbsSessionPropertyRepairMflPath = 32,
    CbsSessionPropertyRepairMflGenerationComplete = 33
} CBS_SESSION_PROPERTY, *PCBS_SESSION_PROPERTY;

typedef enum _CBS_SESSION_CONFIGURABLE_PROPERTY
{
    CbsSessionConfigurablePropertyLocalUupRepository = 1,
    CbsSessionConfigurablePropertyUnknown2 = 2,
    CbsSessionConfigurablePropertyUnknown3 = 3,
    CbsSessionConfigurablePropertyUnknown4 = 4,
    CbsSessionConfigurablePropertyUnknown5 = 5,
    CbsSessionConfigurablePropertyUnknown6 = 6,
    CbsSessionConfigurablePropertyUnknown7 = 7,
    CbsSessionConfigurablePropertyUnknown8 = 8,
    CbsSessionConfigurablePropertyUnknown9 = 9,
    CbsSessionConfigurablePropertyActiveContainerTransactionId = 10,
    CbsSessionConfigurablePropertyUnknown11 = 11,
    CbsSessionConfigurablePropertyUnknown12 = 12,
    CbsSessionConfigurablePropertyUnknown13 = 13,
    CbsSessionConfigurablePropertyUnknown14 = 14,
    CbsSessionConfigurablePropertyUnknown15 = 15,
    CbsSessionConfigurablePropertyUnknown16 = 16,
    CbsSessionConfigurablePropertyUnknown17 = 17,
    CbsSessionConfigurablePropertyUnknown18 = 18,
    CbsSessionConfigurablePropertyUnknown19 = 19
} CBS_SESSION_CONFIGURABLE_PROPERTY, *PCBS_SESSION_CONFIGURABLE_PROPERTY;

typedef enum _CBS_ACTIVITY_TYPE
{
    CbsActivityTypePackage = 1,
    CbsActivityTypeUpdate = 2,
    CbsActivityTypeCapability = 3
} CBS_ACTIVITY_TYPE, *PCBS_ACTIVITY_TYPE;

typedef struct _CBS_PACKAGE_DECRYPTION_DATA
{
    ULONG DataSize;
    PBYTE Data;
} CBS_PACKAGE_DECRYPTION_DATA, *PCBS_PACKAGE_DECRYPTION_DATA;
typedef const CBS_PACKAGE_DECRYPTION_DATA *PCCBS_PACKAGE_DECRYPTION_DATA;

typedef struct _CBS_ACTIVITY
{
    PWSTR Identity;
    CBS_ACTIVITY_TYPE Type;
    CBS_INSTALL_STATE State;
    PWSTR Unknown1;
    PWSTR Unknown2;
} CBS_ACTIVITY, *PCBS_ACTIVITY;

typedef struct _CBS_CUSTOM_PROPERTY
{
    PWSTR Namespace;
    PWSTR Name;
    PWSTR Value;
} CBS_CUSTOM_PROPERTY, *PCBS_CUSTOM_PROPERTY;

typedef struct ICbsCapability ICbsCapability;
typedef struct ICbsCapability2 ICbsCapability2;
typedef struct ICbsCustomInformation ICbsCustomInformation;
typedef struct ICbsFeaturePackage ICbsFeaturePackage;
typedef struct ICbsIdentity ICbsIdentity;
typedef struct ICbsPackage ICbsPackage;
typedef struct ICbsRegBackupHelper ICbsRegBackupHelper;
typedef struct ICbsServicingProcessor ICbsServicingProcessor;
typedef struct ICbsSession ICbsSession;
typedef struct ICbsSession7 ICbsSession7;
typedef struct ICbsSession8 ICbsSession8;
typedef struct ICbsSession9 ICbsSession9;
typedef struct ICbsSession10 ICbsSession10;
typedef struct ICbsSessionObserverListener ICbsSessionObserverListener;
typedef struct ICbsUIHandler ICbsUIHandler;
typedef struct ICbsUIHandler8 ICbsUIHandler8;
typedef struct ICbsUpdate ICbsUpdate;
typedef struct ICbsWorker ICbsWorker;
typedef struct ICbsWorker2 ICbsWorker2;
typedef struct ICSIExternalTransformerExecutor ICSIExternalTransformerExecutor;
typedef struct IEnumCbsActivity IEnumCbsActivity;
typedef struct IEnumCbsCapability IEnumCbsCapability;
typedef struct IEnumCbsCustomInformation IEnumCbsCustomInformation;
typedef struct IEnumCbsCustomProperty IEnumCbsCustomProperty;
typedef struct IEnumCbsFeaturePackage IEnumCbsFeaturePackage;
typedef struct IEnumCbsIdentity IEnumCbsIdentity;
typedef struct IEnumCbsSession IEnumCbsSession;
typedef struct IEnumCbsUpdate IEnumCbsUpdate;
typedef struct IFodHelperComObject IFodHelperComObject;
typedef struct ITrustedInstallerService ITrustedInstallerService;
typedef struct IWimFileFetcherSandbox IWimFileFetcherSandbox;

#define CBS_IUNKNOWN_METHODS(Interface) \
    HRESULT (STDMETHODCALLTYPE *QueryInterface)( \
        _In_ Interface *This, \
        _In_ REFIID InterfaceId, \
        _COM_Outptr_ PVOID *Object); \
    ULONG (STDMETHODCALLTYPE *AddRef)( \
        _In_ Interface *This); \
    ULONG (STDMETHODCALLTYPE *Release)( \
        _In_ Interface *This)

#define CBS_PACKAGE_METHODS(Interface) \
    HRESULT (STDMETHODCALLTYPE *GetIdentity)( \
        _In_ Interface *This, \
        _Outptr_ ICbsIdentity **Identity); \
    HRESULT (STDMETHODCALLTYPE *GetProperty)( \
        _In_ Interface *This, \
        _In_ CBS_PACKAGE_PROPERTY Property, \
        _Outptr_ PWSTR *Value); \
    HRESULT (STDMETHODCALLTYPE *EnumerateUpdates)( \
        _In_ Interface *This, \
        _In_ CBS_APPLICABILITY Applicability, \
        _In_ CBS_SELECTABILITY Selectability, \
        _Outptr_ IEnumCbsUpdate **Enumerator); \
    HRESULT (STDMETHODCALLTYPE *GetUpdate)( \
        _In_ Interface *This, \
        _In_ PCWSTR Name, \
        _Outptr_ ICbsUpdate **Update); \
    HRESULT (STDMETHODCALLTYPE *AddSource)( \
        _In_ Interface *This, \
        _In_ PCWSTR BasePath); \
    HRESULT (STDMETHODCALLTYPE *RemoveSource)( \
        _In_ Interface *This, \
        _In_ PCWSTR BasePath); \
    HRESULT (STDMETHODCALLTYPE *EnumerateSources)( \
        _In_ Interface *This, \
        _Outptr_ IEnumString **Enumerator); \
    HRESULT (STDMETHODCALLTYPE *EvaluateApplicability)( \
        _In_ Interface *This, \
        _In_ UINT Options, \
        _Out_ CBS_INSTALL_STATE *ApplicabilityState, \
        _Out_ CBS_INSTALL_STATE *CurrentState); \
    HRESULT (STDMETHODCALLTYPE *InitiateChanges)( \
        _In_ Interface *This, \
        _In_ UINT Options, \
        _In_ CBS_INSTALL_STATE State, \
        _In_opt_ IUnknown *Progress); \
    HRESULT (STDMETHODCALLTYPE *Status)( \
        _In_ Interface *This, \
        _Out_ CBS_INSTALL_STATE *ProgressState, \
        _Out_ HRESULT *LastError); \
    HRESULT (STDMETHODCALLTYPE *ResourcesToCheck)( \
        _In_ Interface *This, \
        _In_ CBS_RESOURCE_TYPE ResourceType, \
        _Outptr_ IEnumString **Enumerator)

#define CBS_CAPABILITY_METHODS(Interface) \
    HRESULT (STDMETHODCALLTYPE *GetCapability)( \
        _In_ Interface *This, \
        _Outptr_ PWSTR *Namespace, \
        _Outptr_ PWSTR *Language, \
        _Outptr_ PWSTR *Architecture, \
        _Out_ PULONG MajorVersion, \
        _Out_ PULONG MinorVersion); \
    HRESULT (STDMETHODCALLTYPE *GetDependencies)( \
        _In_ Interface *This, \
        _Outptr_ IEnumCbsCapability **Enumerator); \
    HRESULT (STDMETHODCALLTYPE *GetSources)( \
        _In_ Interface *This, \
        _Out_ PUINT Sources); \
    HRESULT (STDMETHODCALLTYPE *GetDownloadSize)( \
        _In_ Interface *This, \
        _Out_ PULONG Size); \
    HRESULT (STDMETHODCALLTYPE *GetInstallSize)( \
        _In_ Interface *This, \
        _Out_ PULONG Size); \
    HRESULT (STDMETHODCALLTYPE *GetInstallState)( \
        _In_ Interface *This, \
        _Out_ CBS_INSTALL_STATE *State); \
    HRESULT (STDMETHODCALLTYPE *GetOwnerInformation)( \
        _In_ Interface *This, \
        _In_ UINT Options, \
        _Out_ PBOOL IsSelfOwned, \
        _Out_ PUINT OwnerCount, \
        _Outptr_ PWSTR *OwnerCapabilities)

#define CBS_UI_HANDLER_METHODS(Interface) \
    HRESULT (STDMETHODCALLTYPE *Initiate)( \
        _In_ Interface *This, \
        _In_ IEnumCbsUpdate *Updates, \
        _Out_ CBS_UI_RESPONSE *Response); \
    HRESULT (STDMETHODCALLTYPE *Terminate)( \
        _In_ Interface *This); \
    HRESULT (STDMETHODCALLTYPE *Error)( \
        _In_ Interface *This, \
        _In_ HRESULT Error, \
        _In_ PCWSTR Message, \
        _Out_ CBS_UI_RESPONSE *Response); \
    HRESULT (STDMETHODCALLTYPE *ResolveSource)( \
        _In_ Interface *This, \
        _In_ PCWSTR Source, \
        _In_ ICbsIdentity *Identity, \
        _In_ PCWSTR RelativePath, \
        _Outptr_ PWSTR *ResolvedSource, \
        _Out_ CBS_UI_RESPONSE *Response); \
    HRESULT (STDMETHODCALLTYPE *Progress)( \
        _In_ Interface *This, \
        _In_ CBS_INSTALL_STATE State, \
        _In_ UINT Current, \
        _In_ UINT Total, \
        _Out_ CBS_UI_RESPONSE *Response)

#define CBS_SESSION_METHODS(Interface) \
    HRESULT (STDMETHODCALLTYPE *Initialize)( \
        _In_ Interface *This, \
        _In_ CBS_SESSION_OPTION Options, \
        _In_ PCWSTR ClientId, \
        _In_opt_ PCWSTR BootDrive, \
        _In_opt_ PCWSTR WindowsDirectory); \
    HRESULT (STDMETHODCALLTYPE *Finalize)( \
        _In_ Interface *This, \
        _Out_ CBS_REQUIRED_ACTION *RequiredAction); \
    HRESULT (STDMETHODCALLTYPE *CreatePackage)( \
        _In_ Interface *This, \
        _In_ UINT Options, \
        _In_ CBS_PACKAGE_TYPE PackageType, \
        _In_ PCWSTR PackagePath, \
        _In_opt_ PCWSTR SandboxPath, \
        _COM_Outptr_ IUnknown **Package); \
    HRESULT (STDMETHODCALLTYPE *OpenPackage)( \
        _In_ Interface *This, \
        _In_ UINT Options, \
        _In_ ICbsIdentity *Identity, \
        _In_opt_ PCWSTR PackagePath, \
        _COM_Outptr_ IUnknown **Package); \
    HRESULT (STDMETHODCALLTYPE *EnumeratePackages)( \
        _In_ Interface *This, \
        _In_ UINT Options, \
        _Outptr_ IEnumCbsIdentity **Enumerator); \
    HRESULT (STDMETHODCALLTYPE *CreateCbsIdentity)( \
        _In_ Interface *This, \
        _Outptr_ ICbsIdentity **Identity)

#define CBS_SESSION7_METHODS(Interface) \
    HRESULT (STDMETHODCALLTYPE *GetStatus)( \
        _In_ Interface *This, \
        _Out_ PUINT CurrentPhase, \
        _Out_ CBS_SESSION_STATE *LastSuccessfulState, \
        _Out_ PBOOL Completed, \
        _Out_ HRESULT *Status); \
    HRESULT (STDMETHODCALLTYPE *Resume)( \
        _In_ Interface *This, \
        _In_opt_ ICbsUIHandler *Progress); \
    HRESULT (STDMETHODCALLTYPE *GetSessionId)( \
        _In_ Interface *This, \
        _Outptr_ PWSTR *SessionId); \
    HRESULT (STDMETHODCALLTYPE *GetProperty)( \
        _In_ Interface *This, \
        _In_ CBS_SESSION_PROPERTY Property, \
        _Outptr_ PWSTR *Value); \
    /* Returns E_NOTIMPL in the examined build. */ \
    HRESULT (STDMETHODCALLTYPE *AddPhaseBreak)( \
        _In_ Interface *This); \
    HRESULT (STDMETHODCALLTYPE *FinalizeEx)( \
        _In_ Interface *This, \
        _In_ UINT Options, \
        _Out_ CBS_REQUIRED_ACTION *RequiredAction)

#define CBS_SESSION8_METHODS(Interface) \
    HRESULT (STDMETHODCALLTYPE *AddSource)( \
        _In_ Interface *This, \
        _In_ UINT Options, \
        _In_ PCWSTR BasePath); \
    HRESULT (STDMETHODCALLTYPE *RegisterCbsUIHandler)( \
        _In_ Interface *This, \
        _In_ ICbsUIHandler *Progress)

#define CBS_SESSION9_METHODS(Interface) \
    HRESULT (STDMETHODCALLTYPE *CreateWindowsUpdatePackage)( \
        _In_ Interface *This, \
        _In_ UINT Options, \
        _In_opt_ PCWSTR WindowsUpdateApplicationId, \
        _In_ GUID UpdateId, \
        _In_ UINT RevisionNumber, \
        _In_ CBS_PACKAGE_TYPE PackageType, \
        _In_ PCWSTR PackagePath, \
        _In_opt_ PCWSTR SandboxPath, \
        _In_ UINT DecryptionDataCount, \
        _In_reads_opt_(DecryptionDataCount) PCCBS_PACKAGE_DECRYPTION_DATA DecryptionData, \
        _In_ CBS_PACKAGE_ENCRYPTION Encryption, \
        _COM_Outptr_ IUnknown **Package); \
    HRESULT (STDMETHODCALLTYPE *EnumerateCapabilities)( \
        _In_ Interface *This, \
        _In_ UINT SourceFilter, \
        _In_opt_ PCWSTR Namespace, \
        _In_opt_ PCWSTR Language, \
        _In_opt_ PCWSTR Architecture, \
        _In_ ULONG MajorVersion, \
        _In_ ULONG MinorVersion, \
        _Outptr_ IEnumCbsCapability **Enumerator); \
    HRESULT (STDMETHODCALLTYPE *InitializeEx)( \
        _In_ Interface *This, \
        _In_ UINT Options, \
        _In_ PCWSTR ClientId, \
        _In_opt_ PCWSTR BootDrive, \
        _In_opt_ PCWSTR WindowsDirectory, \
        _In_opt_ PCWSTR ExternalDirectory); \
    HRESULT (STDMETHODCALLTYPE *CreateExternalTransformerExecutor)( \
        _In_ Interface *This, \
        _Outptr_ ICSIExternalTransformerExecutor **Executor); \
    HRESULT (STDMETHODCALLTYPE *ObserveSessions)( \
        _In_ Interface *This, \
        _In_ UINT Options, \
        _In_opt_ ICbsSessionObserverListener *Listener, \
        _Outptr_ IEnumCbsSession **Enumerator); \
    HRESULT (STDMETHODCALLTYPE *GetActivities)( \
        _In_ Interface *This, \
        _In_ UINT Options, \
        _Outptr_ IEnumCbsActivity **Enumerator); \
    HRESULT (STDMETHODCALLTYPE *SetEnhancedOptions)( \
        _In_ Interface *This, \
        _In_ UINT Options)

#define CBS_ENUM_METHODS(Interface, Element) \
    HRESULT (STDMETHODCALLTYPE *Next)( \
        _In_ Interface *This, \
        _In_ ULONG Count, \
        _Out_writes_to_(Count, *Fetched) Element *Values, \
        _Out_ PULONG Fetched); \
    HRESULT (STDMETHODCALLTYPE *Skip)( \
        _In_ Interface *This, \
        _In_ ULONG Count); \
    HRESULT (STDMETHODCALLTYPE *Reset)( \
        _In_ Interface *This); \
    HRESULT (STDMETHODCALLTYPE *Clone)( \
        _In_ Interface *This, \
        _Outptr_ Interface **Enumerator)

typedef struct ICbsIdentityVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(ICbsIdentity);
    HRESULT (STDMETHODCALLTYPE *Clear)(
        _In_ ICbsIdentity *This);
    HRESULT (STDMETHODCALLTYPE *IsNull)(
        _In_ ICbsIdentity *This,
        _Out_ PBOOL IsNull);
    HRESULT (STDMETHODCALLTYPE *IsEqual)(
        _In_ ICbsIdentity *This,
        _In_ ICbsIdentity *Identity,
        _Out_ PBOOL IsEqual);
    HRESULT (STDMETHODCALLTYPE *LoadFromAttributes)(
        _In_ ICbsIdentity *This,
        _In_ PCWSTR Name,
        _In_opt_ PCWSTR PublicKeyToken,
        _In_opt_ PCWSTR ProcessorArchitecture,
        _In_opt_ PCWSTR Language,
        _In_opt_ PCWSTR Version);
    HRESULT (STDMETHODCALLTYPE *LoadFromStringId)(
        _In_ ICbsIdentity *This,
        _In_ PCWSTR StringId);
    HRESULT (STDMETHODCALLTYPE *SaveAsStringId)(
        _In_ ICbsIdentity *This,
        _Outptr_ PWSTR *StringId);
    END_INTERFACE
} ICbsIdentityVtbl;

struct ICbsIdentity
{
    const ICbsIdentityVtbl *lpVtbl;
};

typedef struct ICbsPackageVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(ICbsPackage);
    CBS_PACKAGE_METHODS(ICbsPackage);
    END_INTERFACE
} ICbsPackageVtbl;

struct ICbsPackage
{
    const ICbsPackageVtbl *lpVtbl;
};

typedef struct ICbsUpdateVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(ICbsUpdate);
    HRESULT (STDMETHODCALLTYPE *GetProperty)(
        _In_ ICbsUpdate *This,
        _In_ CBS_UPDATE_PROPERTY Property,
        _Outptr_ PWSTR *Value);
    HRESULT (STDMETHODCALLTYPE *GetPackage)(
        _In_ ICbsUpdate *This,
        _Outptr_ ICbsPackage **Package);
    HRESULT (STDMETHODCALLTYPE *GetParentUpdate)(
        _In_ ICbsUpdate *This,
        _In_ UINT Index,
        _Outptr_ PWSTR *ParentName,
        _Outptr_result_maybenull_ PWSTR *ParentSet);
    HRESULT (STDMETHODCALLTYPE *GetCapability)(
        _In_ ICbsUpdate *This,
        _Out_ CBS_APPLICABILITY *Applicability,
        _Out_ CBS_SELECTABILITY *Selectability);
    // Returns E_NOTIMPL in the examined build.
    HRESULT (STDMETHODCALLTYPE *GetDeclaredSet)(
        _In_ ICbsUpdate *This,
        _In_ UINT Index,
        _Outptr_ PWSTR *DeclaredSet,
        _Out_ CBS_CARDINALITY *Cardinality);
    HRESULT (STDMETHODCALLTYPE *GetInstallState)(
        _In_ ICbsUpdate *This,
        _Out_ CBS_INSTALL_STATE *CurrentState,
        _Out_ CBS_INSTALL_STATE *IntendedState,
        _Out_ CBS_INSTALL_STATE *RequestedState);
    HRESULT (STDMETHODCALLTYPE *SetInstallState)(
        _In_ ICbsUpdate *This,
        _In_ UINT Options,
        _In_ CBS_INSTALL_STATE State);
    END_INTERFACE
} ICbsUpdateVtbl;

struct ICbsUpdate
{
    const ICbsUpdateVtbl *lpVtbl;
};

typedef struct ICbsCapabilityVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(ICbsCapability);
    CBS_PACKAGE_METHODS(ICbsCapability);
    CBS_CAPABILITY_METHODS(ICbsCapability);
    END_INTERFACE
} ICbsCapabilityVtbl;

struct ICbsCapability
{
    const ICbsCapabilityVtbl *lpVtbl;
};

typedef struct ICbsCapability2Vtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(ICbsCapability2);
    CBS_PACKAGE_METHODS(ICbsCapability2);
    CBS_CAPABILITY_METHODS(ICbsCapability2);
    // Returns E_NOTIMPL in the examined build.
    HRESULT (STDMETHODCALLTYPE *EnumerateFeaturePackages)(
        _In_ ICbsCapability2 *This,
        _Outptr_ IEnumCbsFeaturePackage **Enumerator);
    END_INTERFACE
} ICbsCapability2Vtbl;

struct ICbsCapability2
{
    const ICbsCapability2Vtbl *lpVtbl;
};

typedef struct ICbsFeaturePackageVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(ICbsFeaturePackage);
    HRESULT (STDMETHODCALLTYPE *GetProperty)(
        _In_ ICbsFeaturePackage *This,
        _In_ UINT Property,
        _Outptr_ PWSTR *Value);
    END_INTERFACE
} ICbsFeaturePackageVtbl;

struct ICbsFeaturePackage
{
    const ICbsFeaturePackageVtbl *lpVtbl;
};

typedef struct ICbsUIHandlerVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(ICbsUIHandler);
    CBS_UI_HANDLER_METHODS(ICbsUIHandler);
    END_INTERFACE
} ICbsUIHandlerVtbl;

struct ICbsUIHandler
{
    const ICbsUIHandlerVtbl *lpVtbl;
};

typedef struct ICbsUIHandler8Vtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(ICbsUIHandler8);
    CBS_UI_HANDLER_METHODS(ICbsUIHandler8);
    HRESULT (STDMETHODCALLTYPE *EnteringStage)(
        _In_ ICbsUIHandler8 *This,
        _In_ UINT Options,
        _In_ CBS_OPERATION_STAGE Stage,
        _In_ INT Current,
        _In_ INT Total);
    HRESULT (STDMETHODCALLTYPE *ProgressEx)(
        _In_ ICbsUIHandler8 *This,
        _In_ CBS_INSTALL_STATE State,
        _In_ UINT Current,
        _In_ UINT Total,
        _In_ UINT Options,
        _Out_ CBS_UI_RESPONSE *Response);
    END_INTERFACE
} ICbsUIHandler8Vtbl;

struct ICbsUIHandler8
{
    const ICbsUIHandler8Vtbl *lpVtbl;
};

typedef struct ICbsCustomInformationVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(ICbsCustomInformation);
    HRESULT (STDMETHODCALLTYPE *GetProperty)(
        _In_ ICbsCustomInformation *This,
        _In_ UINT Options,
        _In_opt_ PCWSTR Namespace,
        _In_ PCWSTR Name,
        _Outptr_ PWSTR *Value);
    // Returns E_NOTIMPL in the examined build.
    HRESULT (STDMETHODCALLTYPE *SetProperty)(
        _In_ ICbsCustomInformation *This,
        _In_ UINT Options,
        _In_opt_ PCWSTR Namespace,
        _In_ PCWSTR Name,
        _In_ PCWSTR Value);
    HRESULT (STDMETHODCALLTYPE *GetSelfInformation)(
        _In_ ICbsCustomInformation *This,
        _Outptr_ PWSTR *Namespace,
        _Outptr_ PWSTR *Name);
    HRESULT (STDMETHODCALLTYPE *EnumerateCustomProperties)(
        _In_ ICbsCustomInformation *This,
        _Outptr_ IEnumCbsCustomProperty **Enumerator);
    HRESULT (STDMETHODCALLTYPE *EnumerateChildElements)(
        _In_ ICbsCustomInformation *This,
        _Outptr_ IEnumCbsCustomInformation **Enumerator);
    HRESULT (STDMETHODCALLTYPE *GetChildElement)(
        _In_ ICbsCustomInformation *This,
        _In_ PCWSTR Name,
        _Outptr_ ICbsCustomInformation **Element);
    END_INTERFACE
} ICbsCustomInformationVtbl;

struct ICbsCustomInformation
{
    const ICbsCustomInformationVtbl *lpVtbl;
};

typedef struct ICbsSessionVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(ICbsSession);
    CBS_SESSION_METHODS(ICbsSession);
    END_INTERFACE
} ICbsSessionVtbl;

struct ICbsSession
{
    const ICbsSessionVtbl *lpVtbl;
};

typedef struct ICbsSession7Vtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(ICbsSession7);
    CBS_SESSION_METHODS(ICbsSession7);
    CBS_SESSION7_METHODS(ICbsSession7);
    END_INTERFACE
} ICbsSession7Vtbl;

struct ICbsSession7
{
    const ICbsSession7Vtbl *lpVtbl;
};

typedef struct ICbsSession8Vtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(ICbsSession8);
    CBS_SESSION_METHODS(ICbsSession8);
    CBS_SESSION7_METHODS(ICbsSession8);
    CBS_SESSION8_METHODS(ICbsSession8);
    END_INTERFACE
} ICbsSession8Vtbl;

struct ICbsSession8
{
    const ICbsSession8Vtbl *lpVtbl;
};

typedef struct ICbsSession9Vtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(ICbsSession9);
    CBS_SESSION_METHODS(ICbsSession9);
    CBS_SESSION7_METHODS(ICbsSession9);
    CBS_SESSION8_METHODS(ICbsSession9);
    CBS_SESSION9_METHODS(ICbsSession9);
    END_INTERFACE
} ICbsSession9Vtbl;

struct ICbsSession9
{
    const ICbsSession9Vtbl *lpVtbl;
};

typedef struct ICbsSession10Vtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(ICbsSession10);
    CBS_SESSION_METHODS(ICbsSession10);
    CBS_SESSION7_METHODS(ICbsSession10);
    CBS_SESSION8_METHODS(ICbsSession10);
    CBS_SESSION9_METHODS(ICbsSession10);
    HRESULT (STDMETHODCALLTYPE *SetProperty)(
        _In_ ICbsSession10 *This,
        _In_ CBS_SESSION_CONFIGURABLE_PROPERTY Property,
        _In_ PCWSTR Value);
    HRESULT (STDMETHODCALLTYPE *PerformOperation)(
        _In_ ICbsSession10 *This,
        _In_ UINT Options,
        _In_ CBS_OPERATION_TYPE Operation);
    END_INTERFACE
} ICbsSession10Vtbl;

struct ICbsSession10
{
    const ICbsSession10Vtbl *lpVtbl;
};

typedef struct ICbsSessionObserverListenerVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(ICbsSessionObserverListener);
    HRESULT (STDMETHODCALLTYPE *OnSessionStateChanged)(
        _In_ ICbsSessionObserverListener *This,
        _In_ ICbsSession9 *Session,
        _In_ CBS_SESSION_STATE State);
    END_INTERFACE
} ICbsSessionObserverListenerVtbl;

struct ICbsSessionObserverListener
{
    const ICbsSessionObserverListenerVtbl *lpVtbl;
};

typedef struct ICbsRegBackupHelperVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(ICbsRegBackupHelper);
    HRESULT (STDMETHODCALLTYPE *Backup)(
        _In_ ICbsRegBackupHelper *This,
        _In_ PCWSTR RootDirectory,
        _In_ PCWSTR WindowsDirectory);
    HRESULT (STDMETHODCALLTYPE *Restore)(
        _In_ ICbsRegBackupHelper *This,
        _In_ PCWSTR RootDirectory,
        _In_ PCWSTR WindowsDirectory);
    HRESULT (STDMETHODCALLTYPE *Delete)(
        _In_ ICbsRegBackupHelper *This,
        _In_ PCWSTR WindowsDirectory);
    HRESULT (STDMETHODCALLTYPE *BackupExists)(
        _In_ ICbsRegBackupHelper *This,
        _In_ PCWSTR WindowsDirectory,
        _Out_ PBOOL Exists);
    END_INTERFACE
} ICbsRegBackupHelperVtbl;

struct ICbsRegBackupHelper
{
    const ICbsRegBackupHelperVtbl *lpVtbl;
};

typedef struct ICbsServicingProcessorVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(ICbsServicingProcessor);
    HRESULT (STDMETHODCALLTYPE *Process)(
        _In_ ICbsServicingProcessor *This,
        _In_ UINT Options,
        _In_ PCWSTR ActionListPath,
        _In_ PCWSTR SandboxPath,
        _In_ PCWSTR ClientId,
        _In_opt_ PCWSTR WindowsDirectory,
        _In_opt_ ICbsUIHandler *UIHandler,
        _Out_ CBS_REQUIRED_ACTION *RequiredAction,
        _Outptr_ PWSTR *SessionId);
    HRESULT (STDMETHODCALLTYPE *QuerySessionStatus)(
        _In_ ICbsServicingProcessor *This,
        _In_ PCWSTR SessionId,
        _Out_ HRESULT *Status);
    HRESULT (STDMETHODCALLTYPE *WritePackageFileList)(
        _In_ ICbsServicingProcessor *This,
        _In_ UINT Options,
        _In_ PCWSTR ActionListPath,
        _In_ PCWSTR SandboxPath,
        _In_ PCWSTR ClientId,
        _In_opt_ PCWSTR WindowsDirectory,
        _In_ PCWSTR PackagePath,
        _In_ PCWSTR FileListPath);
    END_INTERFACE
} ICbsServicingProcessorVtbl;

struct ICbsServicingProcessor
{
    const ICbsServicingProcessorVtbl *lpVtbl;
};

typedef struct ICSIExternalTransformerExecutorVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(ICSIExternalTransformerExecutor);
    HRESULT (STDMETHODCALLTYPE *Initialize)(
        _In_ ICSIExternalTransformerExecutor *This,
        _In_ ULONG Options,
        _In_ ULONGLONG Version,
        _In_ PCWSTR WindowsDirectory,
        _In_opt_ PCWSTR PseudoWindowsDirectory,
        _In_opt_ PCWSTR Unknown5);
    HRESULT (STDMETHODCALLTYPE *Install)(
        _In_ ICSIExternalTransformerExecutor *This,
        _In_ PCWSTR ManifestPath,
        _In_opt_ PCWSTR TransformId);
    HRESULT (STDMETHODCALLTYPE *Uninstall)(
        _In_ ICSIExternalTransformerExecutor *This,
        _In_ PCWSTR ManifestPath,
        _In_opt_ PCWSTR TransformId);
    HRESULT (STDMETHODCALLTYPE *Commit)(
        _In_ ICSIExternalTransformerExecutor *This,
        _In_opt_ PCWSTR UserSid,
        _In_opt_ PCWSTR Unknown2,
        _In_opt_ PCWSTR Unknown3,
        _In_opt_ PCWSTR Unknown4);
    END_INTERFACE
} ICSIExternalTransformerExecutorVtbl;

struct ICSIExternalTransformerExecutor
{
    const ICSIExternalTransformerExecutorVtbl *lpVtbl;
};

typedef struct IEnumCbsUpdateVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(IEnumCbsUpdate);
    CBS_ENUM_METHODS(IEnumCbsUpdate, ICbsUpdate*);
    END_INTERFACE
} IEnumCbsUpdateVtbl;

struct IEnumCbsUpdate
{
    const IEnumCbsUpdateVtbl *lpVtbl;
};

typedef struct IEnumCbsIdentityVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(IEnumCbsIdentity);
    CBS_ENUM_METHODS(IEnumCbsIdentity, ICbsIdentity*);
    END_INTERFACE
} IEnumCbsIdentityVtbl;

struct IEnumCbsIdentity
{
    const IEnumCbsIdentityVtbl *lpVtbl;
};

typedef struct IEnumCbsCapabilityVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(IEnumCbsCapability);
    CBS_ENUM_METHODS(IEnumCbsCapability, ICbsCapability*);
    END_INTERFACE
} IEnumCbsCapabilityVtbl;

struct IEnumCbsCapability
{
    const IEnumCbsCapabilityVtbl *lpVtbl;
};

typedef struct IEnumCbsFeaturePackageVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(IEnumCbsFeaturePackage);
    HRESULT (STDMETHODCALLTYPE *Next)(
        _In_ IEnumCbsFeaturePackage *This,
        _In_ ULONG Count,
        _Out_writes_to_(Count, *Fetched) ICbsFeaturePackage **Values,
        _Out_ PULONG Fetched);
    HRESULT (STDMETHODCALLTYPE *Skip)(
        _In_ IEnumCbsFeaturePackage *This,
        _In_ ULONG Count);
    HRESULT (STDMETHODCALLTYPE *Reset)(
        _In_ IEnumCbsFeaturePackage *This);
    END_INTERFACE
} IEnumCbsFeaturePackageVtbl;

struct IEnumCbsFeaturePackage
{
    const IEnumCbsFeaturePackageVtbl *lpVtbl;
};

typedef struct IEnumCbsSessionVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(IEnumCbsSession);
    CBS_ENUM_METHODS(IEnumCbsSession, ICbsSession9*);
    END_INTERFACE
} IEnumCbsSessionVtbl;

struct IEnumCbsSession
{
    const IEnumCbsSessionVtbl *lpVtbl;
};

typedef struct IEnumCbsActivityVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(IEnumCbsActivity);
    CBS_ENUM_METHODS(IEnumCbsActivity, CBS_ACTIVITY);
    END_INTERFACE
} IEnumCbsActivityVtbl;

struct IEnumCbsActivity
{
    const IEnumCbsActivityVtbl *lpVtbl;
};

typedef struct IEnumCbsCustomPropertyVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(IEnumCbsCustomProperty);
    CBS_ENUM_METHODS(IEnumCbsCustomProperty, CBS_CUSTOM_PROPERTY);
    END_INTERFACE
} IEnumCbsCustomPropertyVtbl;

struct IEnumCbsCustomProperty
{
    const IEnumCbsCustomPropertyVtbl *lpVtbl;
};

typedef struct IEnumCbsCustomInformationVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(IEnumCbsCustomInformation);
    CBS_ENUM_METHODS(IEnumCbsCustomInformation, ICbsCustomInformation*);
    END_INTERFACE
} IEnumCbsCustomInformationVtbl;

struct IEnumCbsCustomInformation
{
    const IEnumCbsCustomInformationVtbl *lpVtbl;
};

// Unnamed slots are ABI-only; named slots are verified from CbsCore callers.
typedef struct ITrustedInstallerServiceVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(ITrustedInstallerService);
    HRESULT (STDMETHODCALLTYPE *Unknown1)(
        _In_ ITrustedInstallerService *This);
    HRESULT (STDMETHODCALLTYPE *Unknown2)(
        _In_ ITrustedInstallerService *This,
        _Out_ PULONG Value);
    HRESULT (STDMETHODCALLTYPE *Unknown3)(
        _In_ ITrustedInstallerService *This);
    HRESULT (STDMETHODCALLTYPE *Unknown4)(
        _In_ ITrustedInstallerService *This);
    HRESULT (STDMETHODCALLTYPE *Unknown5)(
        _In_ ITrustedInstallerService *This);
    HRESULT (STDMETHODCALLTYPE *Unknown6)(
        _In_ ITrustedInstallerService *This);
    HRESULT (STDMETHODCALLTYPE *Unknown7)(
        _In_ ITrustedInstallerService *This);
    HRESULT (STDMETHODCALLTYPE *Unknown8)(
        _In_ ITrustedInstallerService *This);
    HRESULT (STDMETHODCALLTYPE *RequestAdditionalTime)(
        _In_ ITrustedInstallerService *This,
        _In_ ULONG TimeoutMilliseconds);
    HRESULT (STDMETHODCALLTYPE *Unknown10)(
        _In_ ITrustedInstallerService *This);
    HRESULT (STDMETHODCALLTYPE *Unknown11)(
        _In_ ITrustedInstallerService *This);
    HRESULT (STDMETHODCALLTYPE *Unknown12)(
        _In_ ITrustedInstallerService *This);
    HRESULT (STDMETHODCALLTYPE *Unknown13)(
        _In_ ITrustedInstallerService *This,
        _In_ ULONG Value);
    HRESULT (STDMETHODCALLTYPE *GetLastSleepTickCount)(
        _In_ ITrustedInstallerService *This,
        _Out_ PULONGLONG TickCount);
    END_INTERFACE
} ITrustedInstallerServiceVtbl;

struct ITrustedInstallerService
{
    const ITrustedInstallerServiceVtbl *lpVtbl;
};

#define CBS_WORKER_METHODS(Interface) \
    HRESULT (STDMETHODCALLTYPE *Unknown1)( \
        _In_ Interface *This, \
        _In_ ITrustedInstallerService *Service, \
        _In_ ULONG Options); \
    HRESULT (STDMETHODCALLTYPE *Unknown2)( \
        _In_ Interface *This, \
        _In_ ULONG Unknown1, \
        _In_ ULONG Unknown2, \
        _In_ ULONGLONG Unknown3, \
        _Outptr_ ICbsSession8 **Session); \
    HRESULT (STDMETHODCALLTYPE *Unknown3)( \
        _In_ Interface *This, \
        _In_ ULONG Unknown1, \
        _Outptr_ IClassFactory **ClassFactory); \
    HRESULT (STDMETHODCALLTYPE *Unknown4)( \
        _In_ Interface *This, \
        _In_ ULONG Value); \
    HRESULT (STDMETHODCALLTYPE *Unknown5)( \
        _In_ Interface *This); \
    HRESULT (STDMETHODCALLTYPE *Unknown6)( \
        _In_ Interface *This); \
    HRESULT (STDMETHODCALLTYPE *Unknown7)( \
        _In_ Interface *This); \
    HRESULT (STDMETHODCALLTYPE *Unknown8)( \
        _In_ Interface *This); \
    HRESULT (STDMETHODCALLTYPE *Unknown9)( \
        _In_ Interface *This); \
    HRESULT (STDMETHODCALLTYPE *Unknown10)( \
        _In_ Interface *This, \
        _In_ ULONG Unknown1, \
        _Out_ PULONG Unknown2); \
    HRESULT (STDMETHODCALLTYPE *Unknown11)( \
        _In_ Interface *This); \
    HRESULT (STDMETHODCALLTYPE *Unknown12)( \
        _In_ Interface *This); \
    HRESULT (STDMETHODCALLTYPE *Unknown13)( \
        _In_ Interface *This, \
        _In_ ULONG Unknown1, \
        _Out_ PULONG Unknown2, \
        _Outptr_result_maybenull_ PWSTR *Unknown3); \
    HRESULT (STDMETHODCALLTYPE *Unknown14)( \
        _In_ Interface *This); \
    HRESULT (STDMETHODCALLTYPE *Unknown15)( \
        _In_ Interface *This)

typedef struct ICbsWorkerVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(ICbsWorker);
    CBS_WORKER_METHODS(ICbsWorker);
    END_INTERFACE
} ICbsWorkerVtbl;

struct ICbsWorker
{
    const ICbsWorkerVtbl *lpVtbl;
};

typedef struct ICbsWorker2Vtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(ICbsWorker2);
    CBS_WORKER_METHODS(ICbsWorker2);
    HRESULT (STDMETHODCALLTYPE *Unknown16)(
        _In_ ICbsWorker2 *This,
        _In_ ULONG Value);
    END_INTERFACE
} ICbsWorker2Vtbl;

struct ICbsWorker2
{
    const ICbsWorker2Vtbl *lpVtbl;
};

typedef struct IWimFileFetcherSandboxVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(IWimFileFetcherSandbox);
    HRESULT (STDMETHODCALLTYPE *Unknown1)(
        _In_ IWimFileFetcherSandbox *This,
        _In_ PCWSTR Value);
    HRESULT (STDMETHODCALLTYPE *Unknown2)(
        _In_ IWimFileFetcherSandbox *This,
        _Out_ PULONG Value);
    HRESULT (STDMETHODCALLTYPE *Unknown3)(
        _In_ IWimFileFetcherSandbox *This,
        _In_ PCWSTR Unknown1,
        _In_ ULONG_PTR Unknown2,
        _In_ ULONG Unknown3);
    HRESULT (STDMETHODCALLTYPE *Unknown4)(
        _In_ IWimFileFetcherSandbox *This,
        _In_ PCWSTR Value);
    HRESULT (STDMETHODCALLTYPE *Unknown5)(
        _In_ IWimFileFetcherSandbox *This,
        _In_ PCWSTR Value);
    END_INTERFACE
} IWimFileFetcherSandboxVtbl;

struct IWimFileFetcherSandbox
{
    const IWimFileFetcherSandboxVtbl *lpVtbl;
};

typedef struct IFodHelperComObjectVtbl
{
    BEGIN_INTERFACE
    CBS_IUNKNOWN_METHODS(IFodHelperComObject);
    HRESULT (STDMETHODCALLTYPE *Unknown1)(
        _In_ IFodHelperComObject *This,
        _In_ PCWSTR Unknown1,
        _In_ ULONG Unknown2,
        _In_ PCWSTR Unknown3,
        _In_ ULONG Unknown4,
        _Outptr_ PWSTR *Unknown5);
    HRESULT (STDMETHODCALLTYPE *Unknown2)(
        _In_ IFodHelperComObject *This,
        _In_ PCWSTR Unknown1,
        _In_ ULONG Unknown2,
        _In_ PCWSTR Unknown3,
        _In_ ULONG Unknown4,
        _In_opt_ ICbsUIHandler *Progress,
        _Outptr_ PWSTR *Unknown6);
    HRESULT (STDMETHODCALLTYPE *Unknown3)(
        _In_ IFodHelperComObject *This,
        _In_ PCWSTR Unknown1,
        _In_ ULONG Unknown2,
        _In_ PCWSTR Unknown3,
        _In_ ULONG Unknown4,
        _In_opt_ ICbsUIHandler *Progress,
        _Out_ CBS_REQUIRED_ACTION *RequiredAction,
        _Outptr_ PWSTR *Unknown7);
    HRESULT (STDMETHODCALLTYPE *Unknown4)(
        _In_ IFodHelperComObject *This,
        _In_ PCWSTR Unknown1,
        _In_ ULONG Unknown2,
        _In_ PCWSTR Unknown3,
        _In_ ULONG Unknown4,
        _In_opt_ ICbsUIHandler *Progress,
        _In_ ULONG Unknown6,
        _Outptr_ PWSTR *Unknown7);
    HRESULT (STDMETHODCALLTYPE *Unknown5)(
        _In_ IFodHelperComObject *This,
        _In_ PCWSTR Unknown1,
        _In_ ULONG Unknown2,
        _In_ PCWSTR Unknown3,
        _In_ ULONG Unknown4,
        _In_opt_ ICbsUIHandler *Progress,
        _In_ ULONG Unknown6,
        _Out_ CBS_REQUIRED_ACTION *RequiredAction,
        _Outptr_ PWSTR *Unknown8);
    HRESULT (STDMETHODCALLTYPE *Unknown6)(
        _In_ IFodHelperComObject *This,
        _In_ PCWSTR Unknown1,
        _In_ ULONG Unknown2,
        _In_ PCWSTR Unknown3,
        _In_ INT Unknown4,
        _In_opt_ ICbsUIHandler *Progress,
        _In_ ULONG Unknown6,
        _Out_ CBS_REQUIRED_ACTION *RequiredAction,
        _Outptr_ PWSTR *Unknown8);
    END_INTERFACE
} IFodHelperComObjectVtbl;

struct IFodHelperComObject
{
    const IFodHelperComObjectVtbl *lpVtbl;
};

#undef CBS_WORKER_METHODS
#undef CBS_ENUM_METHODS
#undef CBS_SESSION9_METHODS
#undef CBS_SESSION8_METHODS
#undef CBS_SESSION7_METHODS
#undef CBS_SESSION_METHODS
#undef CBS_UI_HANDLER_METHODS
#undef CBS_CAPABILITY_METHODS
#undef CBS_PACKAGE_METHODS
#undef CBS_IUNKNOWN_METHODS

#define CBS_DEFINE_GUID(Name, Data1, Data2, Data3, Data40, Data41, Data42, Data43, Data44, Data45, Data46, Data47) \
    EXTERN_C DECLSPEC_SELECTANY CONST GUID Name = \
        { Data1, Data2, Data3, { Data40, Data41, Data42, Data43, Data44, Data45, Data46, Data47 } }

CBS_DEFINE_GUID(
    CLSID_CbsSession,
    0x752073A1, 0x23F2, 0x4396, 0x85, 0xF0, 0x8F, 0xDB, 0x87, 0x9E, 0xD0, 0xED);
CBS_DEFINE_GUID(
    CLSID_CbsSessionRetry,
    0x4729DC2B, 0x36FF, 0x405F, 0xBD, 0x36, 0xF4, 0x51, 0x13, 0xAD, 0xB0, 0x52);
CBS_DEFINE_GUID(
    CLSID_CbsWorker,
    0x0823B6F8, 0xF499, 0x4D5E, 0xB8, 0x85, 0xEA, 0x9C, 0xB4, 0xF4, 0x3B, 0x24);
CBS_DEFINE_GUID(
    CLSID_FodHelperComObject,
    0xC6B167EA, 0xDB3E, 0x4659, 0xBA, 0xDC, 0xD1, 0xCC, 0xC0, 0x0E, 0xFE, 0x9C);
CBS_DEFINE_GUID(
    IID_ICbsCapability,
    0x96DB6347, 0x01BB, 0x42D8, 0xB4, 0xA0, 0x3D, 0x2E, 0xBA, 0x26, 0xFA, 0x56);
CBS_DEFINE_GUID(
    IID_ICbsCapability2,
    0x9EEAE39F, 0xE52B, 0x4F18, 0x9C, 0x14, 0xF8, 0x27, 0xBB, 0x3B, 0xAF, 0x0F);
CBS_DEFINE_GUID(
    IID_ICbsCustomInformation,
    0x80255A37, 0x11ED, 0x4B44, 0xA3, 0x88, 0x87, 0xFE, 0x71, 0xDE, 0x63, 0x06);
CBS_DEFINE_GUID(
    IID_ICbsFeaturePackage,
    0xB531E34E, 0xA150, 0x4B01, 0x8D, 0x77, 0x85, 0x8E, 0x2F, 0x6C, 0xFE, 0x75);
CBS_DEFINE_GUID(
    IID_ICbsIdentity,
    0x75207396, 0x23F2, 0x4396, 0x85, 0xF0, 0x8F, 0xDB, 0x87, 0x9E, 0xD0, 0xED);
CBS_DEFINE_GUID(
    IID_ICbsPackage,
    0x75207393, 0x23F2, 0x4396, 0x85, 0xF0, 0x8F, 0xDB, 0x87, 0x9E, 0xD0, 0xED);
CBS_DEFINE_GUID(
    IID_ICbsRegBackupHelper,
    0x5785850C, 0xB868, 0x41D8, 0xBE, 0x3E, 0x06, 0xDD, 0xF4, 0x2D, 0x7A, 0xC1);
CBS_DEFINE_GUID(
    IID_ICbsServicingProcessor,
    0xBE996087, 0x7C81, 0x465A, 0x9B, 0xB1, 0x39, 0xCE, 0x73, 0x50, 0xAD, 0xCD);
CBS_DEFINE_GUID(
    IID_ICbsSession,
    0x75207391, 0x23F2, 0x4396, 0x85, 0xF0, 0x8F, 0xDB, 0x87, 0x9E, 0xD0, 0xED);
CBS_DEFINE_GUID(
    IID_ICbsSession7,
    0xDC95A094, 0xEE0E, 0x4974, 0x96, 0x00, 0x02, 0x7D, 0x23, 0x21, 0xC2, 0xD4);
CBS_DEFINE_GUID(
    IID_ICbsSession8,
    0xF568C899, 0xAF4F, 0x4EAA, 0xB1, 0x2A, 0xB8, 0xE5, 0xF1, 0xB2, 0x19, 0xDE);
CBS_DEFINE_GUID(
    IID_ICbsSession9,
    0x9C7E3CF3, 0x4C97, 0x4D36, 0xBD, 0xEB, 0xE3, 0x09, 0x3C, 0x22, 0x8C, 0x22);
CBS_DEFINE_GUID(
    IID_ICbsSession10,
    0xF112757A, 0x565B, 0x4260, 0xBD, 0x05, 0x9F, 0xA3, 0x44, 0x17, 0x34, 0x9A);
CBS_DEFINE_GUID(
    IID_ICbsSessionObserverListener,
    0xE3AFD5FD, 0x3B03, 0x453D, 0x91, 0xCE, 0xEB, 0xBD, 0xA9, 0xB8, 0xBE, 0xA1);
CBS_DEFINE_GUID(
    IID_ICbsUIHandler,
    0x75207392, 0x23F2, 0x4396, 0x85, 0xF0, 0x8F, 0xDB, 0x87, 0x9E, 0xD0, 0xED);
CBS_DEFINE_GUID(
    IID_ICbsUIHandler8,
    0xA69C1E5A, 0x5E02, 0x4F6C, 0xBC, 0x7D, 0x2C, 0x47, 0x40, 0x7F, 0xF6, 0x17);
CBS_DEFINE_GUID(
    IID_ICbsUpdate,
    0x75207394, 0x23F2, 0x4396, 0x85, 0xF0, 0x8F, 0xDB, 0x87, 0x9E, 0xD0, 0xED);
CBS_DEFINE_GUID(
    IID_ICbsWorker,
    0xA70DBECC, 0x3734, 0x4B22, 0xB2, 0xD1, 0x64, 0x8C, 0x0E, 0x43, 0xE1, 0x77);
CBS_DEFINE_GUID(
    IID_ICbsWorker2,
    0xEAEA3E99, 0x6B9A, 0x424F, 0x9E, 0xAC, 0x06, 0xE3, 0xAF, 0x7A, 0x3E, 0x09);
CBS_DEFINE_GUID(
    IID_ICSIExternalTransformerExecutor,
    0x1C8ADB85, 0x982E, 0x47F9, 0x99, 0x9F, 0xB0, 0xC3, 0xBF, 0x9D, 0x04, 0x49);
CBS_DEFINE_GUID(
    IID_IEnumCbsActivity,
    0xA454308D, 0x1FC4, 0x48E4, 0xB6, 0xC0, 0x2A, 0x95, 0x80, 0x80, 0x46, 0x86);
CBS_DEFINE_GUID(
    IID_IEnumCbsCapability,
    0x25F05277, 0xE733, 0x4455, 0x80, 0xB7, 0xFA, 0x9C, 0x2D, 0xC9, 0xE6, 0x78);
CBS_DEFINE_GUID(
    IID_IEnumCbsCustomInformation,
    0xAE17E230, 0x3909, 0x4805, 0xAE, 0x1D, 0x8A, 0xEC, 0xDE, 0x53, 0xBD, 0xE6);
CBS_DEFINE_GUID(
    IID_IEnumCbsCustomProperty,
    0x49DCC394, 0x8EDD, 0x415D, 0xA5, 0x5F, 0x85, 0xE5, 0x7A, 0xF9, 0x73, 0x34);
CBS_DEFINE_GUID(
    IID_IEnumCbsFeaturePackage,
    0xD2F9F360, 0x5BC7, 0x4B1E, 0xAA, 0xD5, 0xDD, 0x15, 0x17, 0x33, 0xC7, 0xC8);
CBS_DEFINE_GUID(
    IID_IEnumCbsIdentity,
    0x75207397, 0x23F2, 0x4396, 0x85, 0xF0, 0x8F, 0xDB, 0x87, 0x9E, 0xD0, 0xED);
CBS_DEFINE_GUID(
    IID_IEnumCbsSession,
    0x6943A742, 0x8AE2, 0x4EB1, 0x8A, 0x07, 0x74, 0x7C, 0x5C, 0xFD, 0x3B, 0x9E);
CBS_DEFINE_GUID(
    IID_IEnumCbsUpdate,
    0x75207395, 0x23F2, 0x4396, 0x85, 0xF0, 0x8F, 0xDB, 0x87, 0x9E, 0xD0, 0xED);
CBS_DEFINE_GUID(
    IID_IFodHelperComObject,
    0xFE3CCCCC, 0xF5B9, 0x4B61, 0x86, 0xB8, 0x69, 0x79, 0x08, 0xAD, 0x69, 0x0F);
CBS_DEFINE_GUID(
    IID_ITrustedInstallerService,
    0x365DE52E, 0xEE7E, 0x4975, 0xAE, 0xC8, 0x06, 0x58, 0x82, 0x34, 0xBB, 0x3C);
CBS_DEFINE_GUID(
    IID_IWimFileFetcherSandbox,
    0xE020793F, 0xE3CC, 0x4B36, 0x91, 0x7D, 0x81, 0xC5, 0x99, 0xE9, 0x3F, 0x4B);

#undef CBS_DEFINE_GUID

// CbsCore.dll exports and callbacks.

#define CBS_CORE_STARTUP_OPTION_UNKNOWN1 0x1
#define CBS_CORE_STARTUP_OPTION_RECOVERY 0x2
#define CBS_CORE_STARTUP_OPTION_SAFE_MODE 0x4

typedef enum _CBS_CORE_STATE
{
    CbsCoreStateRequireRebootCallback = 0,
    CbsCoreStateRevokeShutdownProcessing = 1,
    CbsCoreStateUnknown2 = 2,
    CbsCoreStateIsRebootRequiredCallback = 3,
    CbsCoreStateAnticipateShutdownProcessingNeeded = 4,
    CbsCoreStateRegisterWinlogonNotification = 5,
    CbsCoreStateUnregisterWinlogonNotification = 6,
    CbsCoreStateFinalizeExecutionEngine = 7,
    CbsCoreStateSetOnline = 8,
    CbsCoreStateTrustedInstallerNotifyAllPendedOperationsCanceled = 9,
    CbsCoreStateWaitForOutstandingSessions = 10
} CBS_CORE_STATE, *PCBS_CORE_STATE;

typedef enum _CBS_SESSION_NOTIFICATION
{
    CbsSessionNotificationAllowLogon = 1,
    CbsSessionNotificationWait = 2,
    CbsSessionNotificationWaitWithMessage = 3,
    CbsSessionNotificationRestart = 4
} CBS_SESSION_NOTIFICATION, *PCBS_SESSION_NOTIFICATION;

typedef
HRESULT
(WINAPI *PCBS_CORE_LOCK_PROCESS_CALLBACK)(
    _In_ ULONG Options);

typedef
VOID
(WINAPI *PCBS_CORE_CALLBACK)(
    VOID);

typedef
BOOL
(WINAPI *PCBS_CORE_IS_REBOOT_REQUIRED_CALLBACK)(
    VOID);

typedef
HRESULT
(WINAPI *PCBS_CORE_ANTICIPATE_SHUTDOWN_PROCESSING_CALLBACK)(
    VOID);

typedef
VOID
(WINAPI *PCBS_CORE_UNREGISTER_WINLOGON_NOTIFICATION_CALLBACK)(
    _In_ ULONG Value);

typedef
VOID
(WINAPI *PCBS_CORE_LOG_CALLBACK)(
    _In_ ULONG Level,
    _In_ PCSTR Message);

NTSYSAPI
HRESULT
NTAPI
CbsCoreEnsureNoStartupProcessing(
    _In_ BOOL SafeMode);

NTSYSAPI
HRESULT
NTAPI
CbsCoreFinalizeShutdownProcessing(
    VOID);

NTSYSAPI
HRESULT
NTAPI
CbsCorePrepareShutdownProcessing(
    _In_ ITrustedInstallerService *Service,
    _In_ ULONG Options);

NTSYSAPI
HRESULT
NTAPI
CbsCoreServiceIdleProcessing(
    _In_ BOOL Enable,
    _Out_ PBOOL Completed);

NTSYSAPI
HRESULT
NTAPI
CbsCoreShutdownProcessing(
    _In_ ITrustedInstallerService *Service,
    _In_ BOOL SafeMode);

NTSYSAPI
HRESULT
NTAPI
CbsCreateSessionNotify(
    _In_ ITrustedInstallerService *Service,
    _In_ ULONG Options,
    _Out_ CBS_SESSION_NOTIFICATION *Notification,
    _Outptr_result_maybenull_ PWSTR *ProgressMessage);

NTSYSAPI
HRESULT
NTAPI
CbsCreateSessionNotifyFinalize(
    VOID);

NTSYSAPI
HRESULT
NTAPI
CbsCreateSessionNotifyInitialize(
    VOID);

NTSYSAPI
HRESULT
NTAPI
CbsCoreFinalize(
    VOID);

NTSYSAPI
HRESULT
NTAPI
CbsCoreGetActiveOfflineSession(
    _Outptr_ ICbsSession **Session);

NTSYSAPI
HRESULT
NTAPI
CbsCoreInitialize(
    _In_ IMalloc *Allocator,
    _In_ PCBS_CORE_LOCK_PROCESS_CALLBACK LockProcess,
    _In_ PCBS_CORE_CALLBACK UnlockProcess,
    _In_ PCBS_CORE_CALLBACK InstanceCreated,
    _In_ PCBS_CORE_CALLBACK InstanceDestroyed,
    _In_opt_ PCBS_CORE_CALLBACK RequireShutdownNow,
    _In_ PCBS_CORE_CALLBACK RequireShutdownProcessing,
    _Outptr_ IClassFactory **ClassFactory);

NTSYSAPI
BOOL
NTAPI
CbsCoreIsExecutionEngineIdle(
    VOID);

NTSYSAPI
HRESULT
NTAPI
CbsCoreLoadComponentStore(
    VOID);

NTSYSAPI
HRESULT
NTAPI
CbsCoreNotifyAllowingUserLogin(
    VOID);

NTSYSAPI
VOID
NTAPI
CbsCoreSetCustomLogging(
    _In_opt_ PCBS_CORE_LOG_CALLBACK Callback);

NTSYSAPI
HRESULT
NTAPI
CbsCoreSetState(
    _In_ CBS_CORE_STATE State,
    _In_ ULONG_PTR Value);

NTSYSAPI
HRESULT
NTAPI
CbsCoreStartupProcessing(
    _In_ BOOL SafeMode);

NTSYSAPI
HRESULT
NTAPI
CbsCoreStartupProcessingEx(
    _In_ UINT Options);

NTSYSAPI
HRESULT
NTAPI
CbsCoreStopIdleProcessing(
    VOID);

NTSYSAPI
HRESULT
NTAPI
CreateCbsRegBackupHelper(
    _Outptr_ ICbsRegBackupHelper **Helper);

NTSYSAPI
HRESULT
NTAPI
SetRebootInProgressFlag(
    VOID);

NTSYSAPI
VOID
NTAPI
SetTestMode(
    _In_ BOOL Enabled);

#ifdef __cplusplus
}
#endif

#endif // _NTCBS_H
