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

#include <phdk.h>
#include <SetupAPI.h>

typedef enum _DEVNODE_PROP_KEY
{
    DevKeyName,
    DevKeyManufacturer,
    DevKeyService,
    DevKeyClass,
    DevKeyEnumeratorName,
    DevKeyInstallDate,

    DevKeyFirstInstallDate,
    DevKeyLastArrivalDate,
    DevKeyLastRemovalDate,
    DevKeyDeviceDesc,
    DevKeyFriendlyName,
    DevKeyInstanceId,
    DevKeyParentInstanceId,
    DevKeyPDOName,
    DevKeyLocationInfo,
    DevKeyClassGuid,
    DevKeyDriver,
    DevKeyDriverVersion,
    DevKeyDriverDate,
    DevKeyFirmwareDate,
    DevKeyFirmwareVersion,
    DevKeyFirmwareRevision,
    DevKeyHasProblem,
    DevKeyProblemCode,
    DevKeyProblemStatus,
    DevKeyDevNodeStatus,
    DevKeyDevCapabilities,
    DevKeyUpperFilters,
    DevKeyLowerFilters,
    DevKeyHardwareIds,
    DevKeyCompatibleIds,
    DevKeyConfigFlags,
    DevKeyUINumber,
    DevKeyBusTypeGuid,
    DevKeyLegacyBusType,
    DevKeyBusNumber,
    DevKeySecuritySDS,
    DevKeyDevType,
    DevKeyExclusive,
    DevKeyCharacteristics,
    DevKeyAddress,
    DevKeyRemovalPolicy,
    DevKeyRemovalPolicyDefault,
    DevKeyRemovalPolicyOverride,
    DevKeyInstallState,
    DevKeyLocationPaths,
    DevKeyBaseContainerId,
    DevKeyEjectionRelations,
    DevKeyRemovalRelations,
    DevKeyPowerRelations,
    DevKeyBusRelations,
    DevKeyChildren,
    DevKeySiblings,
    DevKeyTransportRelations,
    DevKeyReported,
    DevKeyLegacy,
    DevKeyContainerId,
    DevKeyInLocalMachineContainer,
    DevKeyModel,
    DevKeyModelId,
    DevKeyFriendlyNameAttributes,
    DevKeyManufacturerAttributes,
    DevKeyPresenceNotForDevice,
    DevKeySignalStrength,
    DevKeyIsAssociateableByUserAction,
    DevKeyShowInUninstallUI,
    DevKeyNumaProximityDomain,
    DevKeyDHPRebalancePolicy,
    DevKeyNumaNode,
    DevKeyBusReportedDeviceDesc,
    DevKeyIsPresent,
    DevKeyConfigurationId,
    DevKeyReportedDeviceIdsHash,
    DevKeyBiosDeviceName,
    DevKeyDriverProblemDesc,
    DevKeyDebuggerSafe,
    DevKeyPostInstallInProgress,
    DevKeyStack,
    DevKeyExtendedConfigurationIds,
    DevKeyIsRebootRequired,
    DevKeyDependencyProviders,
    DevKeyDependencyDependents,
    DevKeySoftRestartSupported,
    DevKeyExtendedAddress,
    DevKeyAssignedToGuest,
    DevKeyCreatorProcessId,
    DevKeyFirmwareVendor,
    DevKeySessionId,
    DevKeyDriverDesc,
    DevKeyDriverInfPath,
    DevKeyDriverInfSection,
    DevKeyDriverInfSectionExt,
    DevKeyMatchingDeviceId,
    DevKeyDriverProvider,
    DevKeyDriverPropPageProvider,
    DevKeyDriverCoInstallers,
    DevKeyResourcePickerTags,
    DevKeyResourcePickerExceptions,
    DevKeyDriverRank,
    DevKeyDriverLogoLevel,
    DevKeyNoConnectSound,
    DevKeyGenericDriverInstalled,
    DevKeyAdditionalSoftwareRequested,
    DevKeySafeRemovalRequired,
    DevKeySafeRemovalRequiredOverride,

    DevKeyPkgModel,
    DevKeyPkgVendorWebSite,
    DevKeyPkgDetailedDescription,
    DevKeyPkgDocumentationLink,
    DevKeyPkgIcon,
    DevKeyPkgBrandingIcon,

    DevKeyClassUpperFilters,
    DevKeyClassLowerFilters,
    DevKeyClassSecuritySDS,
    DevKeyClassDevType,
    DevKeyClassExclusive,
    DevKeyClassCharacteristics,
    DevKeyClassName,
    DevKeyClassClassName,
    DevKeyClassIcon,
    DevKeyClassClassInstaller,
    DevKeyClassPropPageProvider,
    DevKeyClassNoInstallClass,
    DevKeyClassNoDisplayClass,
    DevKeyClassSilentInstall,
    DevKeyClassNoUseClass,
    DevKeyClassDefaultService,
    DevKeyClassIconPath,
    DevKeyClassDHPRebalanceOptOut,
    DevKeyClassClassCoInstallers,

    DevKeyInterfaceFriendlyName,
    DevKeyInterfaceEnabled,
    DevKeyInterfaceClassGuid,
    DevKeyInterfaceReferenceString,
    DevKeyInterfaceRestricted,
    DevKeyInterfaceUnrestrictedAppCapabilities,
    DevKeyInterfaceSchematicName,

    DevKeyInterfaceClassDefaultInterface,
    DevKeyInterfaceClassName,

    DevKeyContainerAddress,
    DevKeyContainerDiscoveryMethod,
    DevKeyContainerIsEncrypted,
    DevKeyContainerIsAuthenticated,
    DevKeyContainerIsConnected,
    DevKeyContainerIsPaired,
    DevKeyContainerIcon,
    DevKeyContainerVersion,
    DevKeyContainerLastSeen,
    DevKeyContainerLastConnected,
    DevKeyContainerIsShowInDisconnectedState,
    DevKeyContainerIsLocalMachine,
    DevKeyContainerMetadataPath,
    DevKeyContainerIsMetadataSearchInProgress,
    DevKeyContainerIsNotInterestingForDisplay,
    DevKeyContainerLaunchDeviceStageOnDeviceConnect,
    DevKeyContainerLaunchDeviceStageFromExplorer,
    DevKeyContainerBaselineExperienceId,
    DevKeyContainerIsDeviceUniquelyIdentifiable,
    DevKeyContainerAssociationArray,
    DevKeyContainerDeviceDescription1,
    DevKeyContainerDeviceDescription2,
    DevKeyContainerHasProblem,
    DevKeyContainerIsSharedDevice,
    DevKeyContainerIsNetworkDevice,
    DevKeyContainerIsDefaultDevice,
    DevKeyContainerMetadataCabinet,
    DevKeyContainerRequiresPairingElevation,
    DevKeyContainerExperienceId,
    DevKeyContainerCategory,
    DevKeyContainerCategoryDescSingular,
    DevKeyContainerCategoryDescPlural,
    DevKeyContainerCategoryIcon,
    DevKeyContainerCategoryGroupDesc,
    DevKeyContainerCategoryGroupIcon,
    DevKeyContainerPrimaryCategory,
    DevKeyContainerUnpairUninstall,
    DevKeyContainerRequiresUninstallElevation,
    DevKeyContainerDeviceFunctionSubRank,
    DevKeyContainerAlwaysShowDeviceAsConnected,
    DevKeyContainerConfigFlags,
    DevKeyContainerPrivilegedPackageFamilyNames,
    DevKeyContainerCustomPrivilegedPackageFamilyNames,
    DevKeyContainerIsRebootRequired,
    DevKeyContainerFriendlyName,
    DevKeyContainerManufacturer,
    DevKeyContainerModelName,
    DevKeyContainerModelNumber,
    DevKeyContainerInstallInProgress,

    DevKeyObjectType,

    DevKeyPciInterruptSupport,
    DevKeyPciExpressCapabilityControl,
    DevKeyPciNativeExpressControl,
    DevKeyPciSystemMsiSupport,

    DevKeyStoragePortable,
    DevKeyStorageRemovableMedia,
    DevKeyStorageSystemCritical,
    DevKeyStorageDiskNumber,
    DevKeyStoragePartitionNumber,

    MaxDevKey
} DEVNODE_PROP_KEY, *PDEVNODE_PROP_KEY;

typedef enum _DEVNODE_PROP_TYPE
{
    DevPropTypeString,
    DevPropTypeUInt64,
    DevPropTypeUInt32,
    DevPropTypeInt32,
    DevPropTypeNTSTATUS,
    DevPropTypeGUID,
    DevPropTypeBoolean,
    DevPropTypeTimeStamp,
    DevPropTypeStringList,

    MaxDevType
} DEVNODE_PROP_TYPE, PDEVNODE_PROP_TYPE;
C_ASSERT(MaxDevType <= MAXSHORT);


typedef struct _DEVNODE_PROP
{
    union
    {
        struct
        {
            ULONG Type : 16; // DEVNODE_PROP_TYPE
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
    };

    PPH_STRING AsString;

} DEVNODE_PROP, *PDEVNODE_PROP;

typedef struct _DEVICE_NODE
{
    PH_TREENEW_NODE Node;
    PPH_LIST Children;

    struct _DEVICE_NODE* Parent;
    struct _DEVICE_NODE* Sibling;
    struct _DEVICE_NODE* Child;

    HDEVINFO DeviceInfoHandle;
    SP_DEVINFO_DATA DeviceInfoData;
    ULONG_PTR IconIndex;

    PPH_STRING InstanceId;
    PPH_STRING ParentInstanceId;
    ULONG ProblemCode;
    ULONG DevNodeStatus;
    ULONG Capabilities;

    union
    {
        struct
        {
            ULONG HasUpperFilters : 1;
            ULONG HasLowerFilters : 1;
            ULONG Spare : 30;
        };
        ULONG Flags;
    };

    DEVNODE_PROP Properties[MaxDevKey];
    PH_STRINGREF TextCache[MaxDevKey];

} DEVICE_NODE, *PDEVICE_NODE;

typedef struct _DEVICE_TREE
{
    HDEVINFO DeviceInfoHandle;
    PPH_LIST DeviceRootList;
    PPH_LIST DeviceList;

} DEVICE_TREE, *PDEVICE_TREE;

_Function_class_(PH_TN_FILTER_FUNCTION)
typedef
VOID
NTAPI
DEVPROP_FILL_CALLBACK(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PDEVNODE_PROP Property,
    _In_ ULONG Flags
    );
typedef DEVPROP_FILL_CALLBACK* PDEVPROP_FILL_CALLBACK;

typedef struct _DEVNODE_PROP_TABLE_ENTRY
{
    PWSTR ColumnName;
    BOOLEAN ColumnVisible;
    ULONG ColumnWidth;
    ULONG ColumnTextFlags;

    DEVNODE_PROP_KEY NodeKey;
    const DEVPROPKEY* PropKey;
    PDEVPROP_FILL_CALLBACK Callback;
    ULONG CallbackFlags;

} DEVNODE_PROP_TABLE_ENTRY, *PDEVNODE_PROP_TABLE_ENTRY;

extern DEVNODE_PROP_TABLE_ENTRY DevNodePropTable[];
extern ULONG DevNodePropTableCount;

PDEVNODE_PROP
GetDeviceNodeProperty(
    _In_ PDEVICE_NODE Node,
    _In_ DEVNODE_PROP_KEY Key
    );

typedef struct _DEVICE_TREE_OPTIONS
{
    union
    {
        struct
        {
            ULONG IncludeSoftwareComponents : 1;
            ULONG SortChildrenByName : 1;
            ULONG Reserved : 30;
        };

        ULONG Flags;
    };

    HIMAGELIST ImageList;
    PH_INTEGER_PAIR IconSize;
    PPH_TN_FILTER_SUPPORT FilterSupport;

} DEVICE_TREE_OPTIONS, *PDEVICE_TREE_OPTIONS;

PDEVICE_TREE
CreateDeviceTree(
    _In_ PDEVICE_TREE_OPTIONS Options
    );

VOID InitializeDeviceProvider(
    VOID
    );

#endif
