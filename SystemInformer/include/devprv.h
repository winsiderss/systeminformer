/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2023
 *
 */

#ifndef _DEVICEPRV_H_
#define _DEVICEPRV_H_

#include <SetupAPI.h>

// begin_phapppub
extern PPH_OBJECT_TYPE PhDeviceTreeType;
extern PPH_OBJECT_TYPE PhDeviceItemType;
extern PPH_OBJECT_TYPE PhDeviceNotifyType;

typedef enum _PH_DEVICE_PROPERTY_CLASS
{
    PhDevicePropertyName,
    PhDevicePropertyManufacturer,
    PhDevicePropertyService,
    PhDevicePropertyClass,
    PhDevicePropertyEnumeratorName,
    PhDevicePropertyInstallDate,

    PhDevicePropertyFirstInstallDate,
    PhDevicePropertyLastArrivalDate,
    PhDevicePropertyLastRemovalDate,
    PhDevicePropertyDeviceDesc,
    PhDevicePropertyFriendlyName,
    PhDevicePropertyInstanceId,
    PhDevicePropertyParentInstanceId,
    PhDevicePropertyPDOName,
    PhDevicePropertyLocationInfo,
    PhDevicePropertyClassGuid,
    PhDevicePropertyDriver,
    PhDevicePropertyDriverVersion,
    PhDevicePropertyDriverDate,
    PhDevicePropertyFirmwareDate,
    PhDevicePropertyFirmwareVersion,
    PhDevicePropertyFirmwareRevision,
    PhDevicePropertyHasProblem,
    PhDevicePropertyProblemCode,
    PhDevicePropertyProblemStatus,
    PhDevicePropertyDevNodeStatus,
    PhDevicePropertyDevCapabilities,
    PhDevicePropertyUpperFilters,
    PhDevicePropertyLowerFilters,
    PhDevicePropertyHardwareIds,
    PhDevicePropertyCompatibleIds,
    PhDevicePropertyConfigFlags,
    PhDevicePropertyUINumber,
    PhDevicePropertyBusTypeGuid,
    PhDevicePropertyLegacyBusType,
    PhDevicePropertyBusNumber,
    PhDevicePropertySecurity,
    PhDevicePropertySecuritySDS,
    PhDevicePropertyDevType,
    PhDevicePropertyExclusive,
    PhDevicePropertyCharacteristics,
    PhDevicePropertyAddress,
    PhDevicePropertyPowerData,
    PhDevicePropertyRemovalPolicy,
    PhDevicePropertyRemovalPolicyDefault,
    PhDevicePropertyRemovalPolicyOverride,
    PhDevicePropertyInstallState,
    PhDevicePropertyLocationPaths,
    PhDevicePropertyBaseContainerId,
    PhDevicePropertyEjectionRelations,
    PhDevicePropertyRemovalRelations,
    PhDevicePropertyPowerRelations,
    PhDevicePropertyBusRelations,
    PhDevicePropertyChildren,
    PhDevicePropertySiblings,
    PhDevicePropertyTransportRelations,
    PhDevicePropertyReported,
    PhDevicePropertyLegacy,
    PhDevicePropertyContainerId,
    PhDevicePropertyInLocalMachineContainer,
    PhDevicePropertyModel,
    PhDevicePropertyModelId,
    PhDevicePropertyFriendlyNameAttributes,
    PhDevicePropertyManufacturerAttributes,
    PhDevicePropertyPresenceNotForDevice,
    PhDevicePropertySignalStrength,
    PhDevicePropertyIsAssociateableByUserAction,
    PhDevicePropertyShowInUninstallUI,
    PhDevicePropertyNumaProximityDomain,
    PhDevicePropertyDHPRebalancePolicy,
    PhDevicePropertyNumaNode,
    PhDevicePropertyBusReportedDeviceDesc,
    PhDevicePropertyIsPresent,
    PhDevicePropertyConfigurationId,
    PhDevicePropertyReportedDeviceIdsHash,
    PhDevicePropertyPhysicalDeviceLocation,
    PhDevicePropertyBiosDeviceName,
    PhDevicePropertyDriverProblemDesc,
    PhDevicePropertyDebuggerSafe,
    PhDevicePropertyPostInstallInProgress,
    PhDevicePropertyStack,
    PhDevicePropertyExtendedConfigurationIds,
    PhDevicePropertyIsRebootRequired,
    PhDevicePropertyDependencyProviders,
    PhDevicePropertyDependencyDependents,
    PhDevicePropertySoftRestartSupported,
    PhDevicePropertyExtendedAddress,
    PhDevicePropertyAssignedToGuest,
    PhDevicePropertyCreatorProcessId,
    PhDevicePropertyFirmwareVendor,
    PhDevicePropertySessionId,
    PhDevicePropertyDriverDesc,
    PhDevicePropertyDriverInfPath,
    PhDevicePropertyDriverInfSection,
    PhDevicePropertyDriverInfSectionExt,
    PhDevicePropertyMatchingDeviceId,
    PhDevicePropertyDriverProvider,
    PhDevicePropertyDriverPropPageProvider,
    PhDevicePropertyDriverCoInstallers,
    PhDevicePropertyResourcePickerTags,
    PhDevicePropertyResourcePickerExceptions,
    PhDevicePropertyDriverRank,
    PhDevicePropertyDriverLogoLevel,
    PhDevicePropertyNoConnectSound,
    PhDevicePropertyGenericDriverInstalled,
    PhDevicePropertyAdditionalSoftwareRequested,
    PhDevicePropertySafeRemovalRequired,
    PhDevicePropertySafeRemovalRequiredOverride,

    PhDevicePropertyPkgModel,
    PhDevicePropertyPkgVendorWebSite,
    PhDevicePropertyPkgDetailedDescription,
    PhDevicePropertyPkgDocumentationLink,
    PhDevicePropertyPkgIcon,
    PhDevicePropertyPkgBrandingIcon,

    PhDevicePropertyClassUpperFilters,
    PhDevicePropertyClassLowerFilters,
    PhDevicePropertyClassSecurity,
    PhDevicePropertyClassSecuritySDS,
    PhDevicePropertyClassDevType,
    PhDevicePropertyClassExclusive,
    PhDevicePropertyClassCharacteristics,
    PhDevicePropertyClassName,
    PhDevicePropertyClassClassName,
    PhDevicePropertyClassIcon,
    PhDevicePropertyClassClassInstaller,
    PhDevicePropertyClassPropPageProvider,
    PhDevicePropertyClassNoInstallClass,
    PhDevicePropertyClassNoDisplayClass,
    PhDevicePropertyClassSilentInstall,
    PhDevicePropertyClassNoUseClass,
    PhDevicePropertyClassDefaultService,
    PhDevicePropertyClassIconPath,
    PhDevicePropertyClassDHPRebalanceOptOut,
    PhDevicePropertyClassClassCoInstallers,

    PhDevicePropertyInterfaceFriendlyName,
    PhDevicePropertyInterfaceEnabled,
    PhDevicePropertyInterfaceClassGuid,
    PhDevicePropertyInterfaceReferenceString,
    PhDevicePropertyInterfaceRestricted,
    PhDevicePropertyInterfaceUnrestrictedAppCapabilities,
    PhDevicePropertyInterfaceSchematicName,

    PhDevicePropertyInterfaceClassDefaultInterface,
    PhDevicePropertyInterfaceClassName,

    PhDevicePropertyContainerAddress,
    PhDevicePropertyContainerDiscoveryMethod,
    PhDevicePropertyContainerIsEncrypted,
    PhDevicePropertyContainerIsAuthenticated,
    PhDevicePropertyContainerIsConnected,
    PhDevicePropertyContainerIsPaired,
    PhDevicePropertyContainerIcon,
    PhDevicePropertyContainerVersion,
    PhDevicePropertyContainerLastSeen,
    PhDevicePropertyContainerLastConnected,
    PhDevicePropertyContainerIsShowInDisconnectedState,
    PhDevicePropertyContainerIsLocalMachine,
    PhDevicePropertyContainerMetadataPath,
    PhDevicePropertyContainerIsMetadataSearchInProgress,
    PhDevicePropertyContainerIsMetadataChecksum,
    PhDevicePropertyContainerIsNotInterestingForDisplay,
    PhDevicePropertyContainerLaunchDeviceStageOnDeviceConnect,
    PhDevicePropertyContainerLaunchDeviceStageFromExplorer,
    PhDevicePropertyContainerBaselineExperienceId,
    PhDevicePropertyContainerIsDeviceUniquelyIdentifiable,
    PhDevicePropertyContainerAssociationArray,
    PhDevicePropertyContainerDeviceDescription1,
    PhDevicePropertyContainerDeviceDescription2,
    PhDevicePropertyContainerHasProblem,
    PhDevicePropertyContainerIsSharedDevice,
    PhDevicePropertyContainerIsNetworkDevice,
    PhDevicePropertyContainerIsDefaultDevice,
    PhDevicePropertyContainerMetadataCabinet,
    PhDevicePropertyContainerRequiresPairingElevation,
    PhDevicePropertyContainerExperienceId,
    PhDevicePropertyContainerCategory,
    PhDevicePropertyContainerCategoryDescSingular,
    PhDevicePropertyContainerCategoryDescPlural,
    PhDevicePropertyContainerCategoryIcon,
    PhDevicePropertyContainerCategoryGroupDesc,
    PhDevicePropertyContainerCategoryGroupIcon,
    PhDevicePropertyContainerPrimaryCategory,
    PhDevicePropertyContainerUnpairUninstall,
    PhDevicePropertyContainerRequiresUninstallElevation,
    PhDevicePropertyContainerDeviceFunctionSubRank,
    PhDevicePropertyContainerAlwaysShowDeviceAsConnected,
    PhDevicePropertyContainerConfigFlags,
    PhDevicePropertyContainerPrivilegedPackageFamilyNames,
    PhDevicePropertyContainerCustomPrivilegedPackageFamilyNames,
    PhDevicePropertyContainerIsRebootRequired,
    PhDevicePropertyContainerFriendlyName,
    PhDevicePropertyContainerManufacturer,
    PhDevicePropertyContainerModelName,
    PhDevicePropertyContainerModelNumber,
    PhDevicePropertyContainerInstallInProgress,

    PhDevicePropertyObjectType,

    PhDevicePropertyPciInterruptSupport,
    PhDevicePropertyPciExpressCapabilityControl,
    PhDevicePropertyPciNativeExpressControl,
    PhDevicePropertyPciSystemMsiSupport,

    PhDevicePropertyStoragePortable,
    PhDevicePropertyStorageRemovableMedia,
    PhDevicePropertyStorageSystemCritical,
    PhDevicePropertyStorageDiskNumber,
    PhDevicePropertyStoragePartitionNumber,

    PhMaxDeviceProperty
} PH_DEVICE_PROPERTY_CLASS, *PPH_DEVICE_PROPERTY_CLASS;

typedef enum _PH_DEVICE_PROPERTY_TYPE
{
    PhDevicePropertyTypeString,
    PhDevicePropertyTypeUInt64,
    PhDevicePropertyTypeUInt32,
    PhDevicePropertyTypeInt32,
    PhDevicePropertyTypeNTSTATUS,
    PhDevicePropertyTypeGUID,
    PhDevicePropertyTypeBoolean,
    PhDevicePropertyTypeTimeStamp,
    PhDevicePropertyTypeStringList,
    PhDevicePropertyTypeBinary,

    PhMaxDevicePropertyType
} PH_DEVICE_PROPERTY_TYPE, PPH_DEVICE_PROPERTY_TYPE;
C_ASSERT(PhMaxDevicePropertyType <= MAXSHORT);

typedef struct _PH_DEVICE_PROPERTY
{
    union
    {
        struct
        {
            ULONG Type : 16; // PH_DEVICE_PROPERTY_TYPE
            ULONG Spare : 14;
            ULONG Initialized : 1;
            ULONG Valid : 1;
        };
        ULONG State;
    };

    union
    {
        PPH_STRING String;
        ULONG64 UInt64;
        ULONG UInt32;
        LONG Int32;
        NTSTATUS Status;
        GUID Guid;
        BOOLEAN Boolean;
        LARGE_INTEGER TimeStamp;
        PPH_LIST StringList;
        struct
        {
            ULONG Size;
            PBYTE Buffer;
        } Binary;
    };

    PPH_STRING AsString;
} PH_DEVICE_PROPERTY, *PPH_DEVICE_PROPERTY;

// end_phapppub
typedef struct _PH_DEVINFO
{
    HDEVINFO Handle;
} PH_DEVINFO, *PPH_DEVINFO;

typedef struct _PH_DEVINFO_DATA
{
    BOOLEAN Interface;
    union
    {
        SP_DEVINFO_DATA DeviceData;
        SP_DEVICE_INTERFACE_DATA InterfaceData;
    };
} PH_DEVINFO_DATA, *PPH_DEVINFO_DATA;
// begin_phapppub

typedef struct _PH_DEVICE_ITEM
{
    struct _PH_DEVICE_TREE* Tree;
    struct _PH_DEVICE_ITEM* Parent;
    struct _PH_DEVICE_ITEM* Sibling;
    struct _PH_DEVICE_ITEM* Child;

    GUID ClassGuid;
    ULONG InstanceIdHash;
    PPH_STRING InstanceId;
    PPH_STRING ParentInstanceId;
    ULONG ProblemCode;
    ULONG DevNodeStatus;
    ULONG Capabilities;
    ULONG ChildrenCount;
    ULONG InterfaceCount;

    union
    {
        struct
        {
            ULONG HasUpperFilters : 1;
            ULONG HasLowerFilters : 1;
            ULONG DeviceInterface : 1;
            ULONG Spare : 29;
        };
        ULONG Flags;
    };

    PH_DEVICE_PROPERTY Properties[PhMaxDeviceProperty];

// end_phapppub
    PPH_DEVINFO DeviceInfo;
    PH_DEVINFO_DATA DeviceInfoData;
// begin_phapppub
} PH_DEVICE_ITEM, *PPH_DEVICE_ITEM;

typedef struct _PH_DEVICE_TREE
{
    PPH_DEVICE_ITEM Root;
    PPH_LIST DeviceList;
    PPH_LIST DeviceInterfaceList;
// end_phapppub
    PPH_DEVINFO DeviceInfo;
// begin_phapppub
} PH_DEVICE_TREE, *PPH_DEVICE_TREE;

PHAPPAPI
PPH_DEVICE_PROPERTY
NTAPI
PhGetDeviceProperty(
    _In_ PPH_DEVICE_ITEM Item,
    _In_ PH_DEVICE_PROPERTY_CLASS Class 
    );

PHAPPAPI
BOOLEAN
NTAPI
PhLookupDevicePropertyClass(
    _In_ const DEVPROPKEY* Key,
    _Out_ PPH_DEVICE_PROPERTY_CLASS Class
    );

PHAPPAPI
HICON
NTAPI
PhGetDeviceIcon(
    _In_ PPH_DEVICE_ITEM Item,
    _In_ PPH_INTEGER_PAIR IconSize
    );

PHAPPAPI
PPH_DEVICE_TREE
NTAPI
PhReferenceDeviceTree(
    VOID
    );

PHAPPAPI
PPH_DEVICE_TREE
NTAPI
PhReferenceDeviceTreeEx(
    _In_ BOOLEAN ForceRefresh
    );

PHAPPAPI
_Success_(return != NULL)
_Must_inspect_result_
PPH_DEVICE_ITEM
NTAPI
PhLookupDeviceItem(
    _In_ PPH_DEVICE_TREE Tree,
    _In_ PPH_STRINGREF InstanceId
    );

PHAPPAPI
_Success_(return != NULL)
_Must_inspect_result_
PPH_DEVICE_ITEM
NTAPI
PhReferenceDeviceItem(
    _In_ PPH_DEVICE_TREE Tree,
    _In_ PPH_STRINGREF InstanceId
    );

PHAPPAPI
_Success_(return != NULL)
_Must_inspect_result_
PPH_DEVICE_ITEM
NTAPI
PhReferenceDeviceItem2(
    _In_ PPH_STRINGREF InstanceId
    );

typedef enum _PH_DEVICE_NOTIFY_ACTION
{
    PhDeviceNotifyInterfaceArrival,
    PhDeviceNotifyInterfaceRemoval,
    PhDeviceNotifyInstanceEnumerated,
    PhDeviceNotifyInstanceStarted,
    PhDeviceNotifyInstanceRemoved,
} PH_DEVICE_NOTIFY_ACTION, *PPH_DEVICE_NOTIFY_ACTION;

typedef struct _PH_DEVICE_NOTIFY
{
    PH_DEVICE_NOTIFY_ACTION Action;

    union
    {
        struct
        {
            GUID ClassGuid;
        } DeviceInterface; // PhDeviceNotifyInterface...

        struct
        {
            PPH_STRING InstanceId;
        } DeviceInstance; // PhDeviceNotifyInstance...
    };

// end_phapppub
    LIST_ENTRY ListEntry;
// begin_phapppub
} PH_DEVICE_NOTIFY, *PPH_DEVICE_NOTIFY;

PHAPPAPI
BOOLEAN
NTAPI
PhDeviceProviderInitialization(
    VOID
    );

// end_phapppub

#endif
