/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *
 */

#undef DEVICE_DEVPROPKEY
#include "devices.h"
#include <toolstatusintf.h>

#include <cfgmgr32.h>
#include <devguid.h>
#include <wdmguid.h>
#include <SetupAPI.h>

#undef DEFINE_GUID
#include <devpropdef.h>
#include <pciprop.h>
#include <ntddstor.h>

#include <guiddef.h>

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

#if !defined(NTDDI_WIN10_NI) || (NTDDI_VERSION < NTDDI_WIN10_NI)
// Note: This propkey is required for building with 22H1 and older Windows SDK (dmex)
DEFINE_DEVPROPKEY(DEVPKEY_Device_FirmwareVendor, 0x540b947e, 0x8b40, 0x45bc, 0xa8, 0xa2, 0x6a, 0x0b, 0x89, 0x4c, 0xbd, 0xa2, 26);   // DEVPROP_TYPE_STRING
#endif

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

#define DEVPROP_FILL_FLAG_CLASS_INTERFACE 0x00000001
#define DEVPROP_FILL_FLAG_CLASS_INSTALLER 0x00000002

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

static PH_STRINGREF RootInstanceId = PH_STRINGREF_INIT(L"HTREE\\ROOT\\0");
static PPH_OBJECT_TYPE DeviceNodeType = NULL;
static PPH_OBJECT_TYPE DeviceTreeType = NULL;

static BOOLEAN AutoRefreshDeviceTree = TRUE;
static BOOLEAN ShowDisconnected = TRUE;
static BOOLEAN ShowSoftwareComponents = TRUE;
static BOOLEAN HighlightUpperFiltered = TRUE;
static BOOLEAN HighlightLowerFiltered = TRUE;
static ULONG DeviceProblemColor = 0;
static ULONG DeviceDisabledColor = 0;
static ULONG DeviceDisconnectedColor = 0;
static ULONG DeviceHighlightColor = 0;

static BOOLEAN DeviceTreeCreated = FALSE;
static HWND DeviceTreeHandle = NULL;
static PDEVICE_TREE DeviceTree = NULL;
static HIMAGELIST DeviceImageList = NULL;
static PH_INTEGER_PAIR DeviceIconSize = { 16, 16 };
static PH_LIST DeviceFilterList = { 0 };
static PPH_MAIN_TAB_PAGE DevicesAddedTabPage = NULL;
static PTOOLSTATUS_INTERFACE ToolStatusInterface = NULL;
static BOOLEAN DeviceTabSelected = FALSE;
static ULONG DeviceTreeSortColumn = 0;
static PH_SORT_ORDER DeviceTreeSortOrder = NoSortOrder;
static PH_TN_FILTER_SUPPORT DeviceTreeFilterSupport = { 0 };
static PPH_TN_FILTER_ENTRY DeviceTreeFilterEntry = NULL;
static PH_CALLBACK_REGISTRATION SearchChangedRegistration = { 0 };
static HCMNOTIFICATION DeviceNotification = NULL;
static HCMNOTIFICATION DeviceInterfaceNotification = NULL;

BOOLEAN GetDevicePropertyGuid(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PGUID Guid
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(GUID);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Guid,
        sizeof(GUID),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_GUID))
    {
        return TRUE;
    }

    RtlZeroMemory(Guid, sizeof(GUID));

    return FALSE;
}

BOOLEAN GetClassPropertyGuid(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PGUID Guid
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(GUID);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Guid,
        sizeof(GUID),
        &requiredLength,
        Flags
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_GUID))
    {
        return TRUE;
    }

    RtlZeroMemory(Guid, sizeof(GUID));

    return FALSE;
}

BOOLEAN GetDevicePropertyUInt64(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PULONG64 Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(ULONG64);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(ULONG64),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_UINT64))
    {
        return TRUE;
    }

    *Value = 0;

    return FALSE;
}

BOOLEAN GetClassPropertyUInt64(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PULONG64 Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(ULONG64);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(ULONG64),
        &requiredLength,
        Flags
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_UINT64))
    {
        return TRUE;
    }

    *Value = 0;

    return FALSE;
}

BOOLEAN GetDevicePropertyUInt32(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PULONG Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(ULONG);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(ULONG),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_UINT32))
    {
        return TRUE;
    }

    *Value = 0;

    return FALSE;
}

BOOLEAN GetClassPropertyUInt32(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PULONG Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(ULONG);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(ULONG),
        &requiredLength,
        Flags
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_UINT32))
    {
        return TRUE;
    }

    *Value = 0;

    return FALSE;
}

BOOLEAN GetDevicePropertyInt32(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PLONG Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(LONG);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(LONG),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_INT32))
    {
        return TRUE;
    }

    *Value = 0;

    return FALSE;
}

BOOLEAN GetClassPropertyInt32(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PLONG Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(LONG);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(LONG),
        &requiredLength,
        Flags
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_INT32))
    {
        return TRUE;
    }

    *Value = 0;

    return FALSE;
}

BOOLEAN GetDevicePropertyNTSTATUS(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PNTSTATUS Status
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(NTSTATUS);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Status,
        sizeof(NTSTATUS),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_NTSTATUS))
    {
        return TRUE;
    }

    *Status = 0;

    return FALSE;
}

BOOLEAN GetClassPropertyNTSTATUS(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PNTSTATUS Status
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(NTSTATUS);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Status,
        sizeof(NTSTATUS),
        &requiredLength,
        Flags
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_NTSTATUS))
    {
        return TRUE;
    }

    *Status = 0;

    return FALSE;
}

BOOLEAN GetDevicePropertyBoolean(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PBOOLEAN Boolean
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    DEVPROP_BOOLEAN boolean;
    ULONG requiredLength = sizeof(DEVPROP_BOOLEAN);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)&boolean,
        sizeof(DEVPROP_BOOLEAN),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_BOOLEAN))
    {
        *Boolean = !!boolean;

        return TRUE;
    }

    *Boolean = FALSE;

    return FALSE;
}

BOOLEAN GetClassPropertyBoolean(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PBOOLEAN Boolean
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    DEVPROP_BOOLEAN boolean;
    ULONG requiredLength = sizeof(DEVPROP_BOOLEAN);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)&boolean,
        sizeof(DEVPROP_BOOLEAN),
        &requiredLength,
        Flags
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_BOOLEAN))
    {
        *Boolean = !!boolean;

        return TRUE;
    }

    *Boolean = FALSE;

    return FALSE;
}

BOOLEAN GetDevicePropertyTimeStamp(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PLARGE_INTEGER TimeStamp
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    FILETIME fileTime;
    ULONG requiredLength = sizeof(FILETIME);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)&fileTime,
        sizeof(FILETIME),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_FILETIME))
    {
        TimeStamp->HighPart = fileTime.dwHighDateTime;
        TimeStamp->LowPart = fileTime.dwLowDateTime;

        return TRUE;
    }

    TimeStamp->QuadPart = 0;

    return FALSE;
}

BOOLEAN GetClassPropertyTimeStamp(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PLARGE_INTEGER TimeStamp
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    FILETIME fileTime;
    ULONG requiredLength = sizeof(FILETIME);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)&fileTime,
        sizeof(FILETIME),
        &requiredLength,
        Flags
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_FILETIME))
    {
        TimeStamp->HighPart = fileTime.dwHighDateTime;
        TimeStamp->LowPart = fileTime.dwLowDateTime;

        return TRUE;
    }

    TimeStamp->QuadPart = 0;

    return FALSE;
}

BOOLEAN GetDevicePropertyString(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PPH_STRING* String
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = 0;
    PVOID buffer = NULL;

    *String = NULL;

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        NULL,
        0,
        &requiredLength,
        0
        );
    if (result ||
        (requiredLength == 0) ||
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER) ||
        ((devicePropertyType != DEVPROP_TYPE_STRING) &&
         (devicePropertyType != DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING)))
    {
        goto Exit;
    }

    buffer = PhAllocate(requiredLength);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        buffer,
        requiredLength,
        &requiredLength,
        0
        );
    if (!result)
    {
        goto Exit;
    }

    *String = PhCreateString(buffer);

Exit:

    if (buffer)
        PhFree(buffer);

    return result;
}

BOOLEAN GetClassPropertyString(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PPH_STRING* String
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = 0;
    PVOID buffer = NULL;

    *String = NULL;

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        NULL,
        0,
        &requiredLength,
        Flags
        );
    if (result ||
        (requiredLength == 0) ||
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER) ||
        ((devicePropertyType != DEVPROP_TYPE_STRING) &&
         (devicePropertyType != DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING)))
    {
        goto Exit;
    }

    buffer = PhAllocate(requiredLength);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        buffer,
        requiredLength,
        &requiredLength,
        Flags
        );
    if (!result)
    {
        goto Exit;
    }

    *String = PhCreateString(buffer);

Exit:

    if (buffer)
        PhFree(buffer);

    return result;
}

BOOLEAN GetDevicePropertyStringList(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PPH_LIST* StringList
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = 0;
    PVOID buffer = NULL;
    PPH_LIST stringList;

    *StringList = NULL;

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        NULL,
        0,
        &requiredLength,
        0
        );
    if (result ||
        (requiredLength == 0) ||
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER) ||
        (devicePropertyType != DEVPROP_TYPE_STRING_LIST))
    {
        goto Exit;
    }

    buffer = PhAllocate(requiredLength);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        buffer,
        requiredLength,
        &requiredLength,
        0
        );
    if (!result)
    {
        goto Exit;
    }

    stringList = PhCreateList(1);

    for (PZZWSTR item = buffer;;)
    {
        UNICODE_STRING string;

        RtlInitUnicodeString(&string, item);

        if (string.Length == 0)
        {
            break;
        }

        PhAddItemList(stringList, PhCreateStringFromUnicodeString(&string));

        item = PTR_ADD_OFFSET(item, string.MaximumLength);
    }

    *StringList = stringList;

Exit:

    if (buffer)
        PhFree(buffer);

    return result;
}

BOOLEAN GetClassPropertyStringList(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PPH_LIST* StringList
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = 0;
    PVOID buffer = NULL;
    PPH_LIST stringList;

    *StringList = NULL;

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        NULL,
        0,
        &requiredLength,
        Flags
        );
    if (result ||
        (requiredLength == 0) ||
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER) ||
        (devicePropertyType != DEVPROP_TYPE_STRING_LIST))
    {
        goto Exit;
    }

    buffer = PhAllocate(requiredLength);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        buffer,
        requiredLength,
        &requiredLength,
        Flags
        );
    if (!result)
    {
        goto Exit;
    }

    stringList = PhCreateList(1);

    for (PZZWSTR item = buffer;;)
    {
        UNICODE_STRING string;

        RtlInitUnicodeString(&string, item);

        if (string.Length == 0)
        {
            break;
        }

        PhAddItemList(stringList, PhCreateStringFromUnicodeString(&string));

        item = PTR_ADD_OFFSET(item, string.MaximumLength);
    }

    *StringList = stringList;

Exit:

    if (buffer)
        PhFree(buffer);

    return result;
}

_Function_class_(DEVPROP_FILL_CALLBACK)
VOID NTAPI DevPropFillString(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PDEVNODE_PROP Property,
    _In_ ULONG Flags
    )
{
    Property->Type = DevPropTypeString;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = GetDevicePropertyString(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->String
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = GetClassPropertyString(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->String
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = GetClassPropertyString(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->String
            );
    }

    if (Property->Valid)
    {
        Property->AsString = Property->String;
        PhReferenceObject(Property->AsString);
    }
}

_Function_class_(DEVPROP_FILL_CALLBACK)
VOID NTAPI DevPropFillUInt64(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PDEVNODE_PROP Property,
    _In_ ULONG Flags
    )
{
    Property->Type = DevPropTypeUInt64;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = GetDevicePropertyUInt64(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->UInt64
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = GetClassPropertyUInt64(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->UInt64
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = GetClassPropertyUInt64(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->UInt64
            );
    }

    if (Property->Valid)
    {
        PH_FORMAT format[1];

        PhInitFormatI64U(&format[0], Property->UInt64);

        Property->AsString = PhFormat(format, ARRAYSIZE(format), 1);
    }
}

_Function_class_(DEVPROP_FILL_CALLBACK)
VOID NTAPI DevPropFillUInt64Hex(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PDEVNODE_PROP Property,
    _In_ ULONG Flags
    )
{
    Property->Type = DevPropTypeUInt64;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = GetDevicePropertyUInt64(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->UInt64
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = GetClassPropertyUInt64(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->UInt64
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = GetClassPropertyUInt64(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->UInt64
            );
    }

    if (Property->Valid)
    {
        PH_FORMAT format[2];

        PhInitFormatI64X(&format[1], Property->UInt64);

        Property->AsString = PhFormat(format, ARRAYSIZE(format), 10);
    }
}


_Function_class_(DEVPROP_FILL_CALLBACK)
VOID NTAPI DevPropFillUInt32(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PDEVNODE_PROP Property,
    _In_ ULONG Flags
    )
{
    Property->Type = DevPropTypeUInt32;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = GetDevicePropertyUInt32(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->UInt32
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = GetClassPropertyUInt32(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->UInt32
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = GetClassPropertyUInt32(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->UInt32
            );
    }

    if (Property->Valid)
    {
        PH_FORMAT format[1];

        PhInitFormatU(&format[0], Property->UInt32);

        Property->AsString = PhFormat(format, ARRAYSIZE(format), 1);
    }
}

_Function_class_(DEVPROP_FILL_CALLBACK)
VOID NTAPI DevPropFillUInt32Hex(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PDEVNODE_PROP Property,
    _In_ ULONG Flags
    )
{
    Property->Type = DevPropTypeUInt32;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = GetDevicePropertyUInt32(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->UInt32
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = GetClassPropertyUInt32(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->UInt32
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = GetClassPropertyUInt32(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->UInt32
            );
    }

    if (Property->Valid)
    {
        PH_FORMAT format[2];

        PhInitFormatS(&format[0], L"0x");
        PhInitFormatIX(&format[1], Property->UInt32);

        Property->AsString = PhFormat(format, ARRAYSIZE(format), 10);
    }
}

_Function_class_(DEVPROP_FILL_CALLBACK)
VOID NTAPI DevPropFillInt32(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PDEVNODE_PROP Property,
    _In_ ULONG Flags
    )
{
    Property->Type = DevPropTypeUInt32;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = GetDevicePropertyInt32(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->Int32
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = GetClassPropertyInt32(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->Int32
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = GetClassPropertyInt32(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->Int32
            );
    }

    if (Property->Valid)
    {
        PH_FORMAT format[1];

        PhInitFormatD(&format[0], Property->Int32);

        Property->AsString = PhFormat(format, ARRAYSIZE(format), 1);
    }
}

_Function_class_(DEVPROP_FILL_CALLBACK)
VOID NTAPI DevPropFillNTSTATUS(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PDEVNODE_PROP Property,
    _In_ ULONG Flags
    )
{
    Property->Type = DevPropTypeNTSTATUS;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = GetDevicePropertyNTSTATUS(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->Status
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = GetClassPropertyNTSTATUS(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->Status
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = GetClassPropertyNTSTATUS(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->Status
            );
    }

    if (Property->Valid && Property->Status != STATUS_SUCCESS)
    {
        Property->AsString = PhGetStatusMessage(Property->Status, 0);
    }
}

_Function_class_(DEVPROP_FILL_CALLBACK)
VOID NTAPI DevPropFillGuid(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PDEVNODE_PROP Property,
    _In_ ULONG Flags
    )
{
    Property->Type = DevPropTypeGUID;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = GetDevicePropertyGuid(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->Guid
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = GetClassPropertyGuid(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->Guid
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = GetClassPropertyGuid(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->Guid
            );
    }

    if (Property->Valid)
    {
        Property->AsString = PhFormatGuid(&Property->Guid);
    }
}

PPH_STRING DevPropBusTypeGuidToString(
    _In_ PGUID BusTypeGuid
    )
{
    if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_INTERNAL))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_INTERNAL");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_PCMCIA))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_PCMCIA");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_PCI))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_PCI");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_ISAPNP))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_ISAPNP");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_EISA))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_EISA");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_MCA))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_MCA");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_SERENUM))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_SERENUM");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_USB))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_USB");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_LPTENUM))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_LPTENUM");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_USBPRINT))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_USBPRINT");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_DOT4PRT))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_DOT4PRT");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_1394))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_1394");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_HID))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_HID");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_AVC))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_AVC");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_IRDA))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_IRDA");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_SD))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_SD");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_ACPI))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_ACPI");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_SW_DEVICE))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_SW_DEVICE");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_SCM))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_SCM");
        return PhCreateString2(&string);
    }

    return NULL;
}

_Function_class_(DEVPROP_FILL_CALLBACK)
VOID NTAPI DevPropFillBusTypeGuid(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PDEVNODE_PROP Property,
    _In_ ULONG Flags
    )
{
    Property->Type = DevPropTypeGUID;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = GetDevicePropertyGuid(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->Guid
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = GetClassPropertyGuid(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->Guid
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = GetClassPropertyGuid(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->Guid
            );
    }

    if (Property->Valid)
    {
        Property->AsString = DevPropBusTypeGuidToString(&Property->Guid);

        if (!Property->AsString)
        {
            Property->AsString = PhFormatGuid(&Property->Guid);
        }
    }
}

PPH_STRING DevPropPciDeviceInterruptSupportToString(
    _In_ ULONG Flags
    )
{
    PH_STRING_BUILDER stringBuilder;
    WCHAR pointer[PH_PTR_STR_LEN_1];

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (BooleanFlagOn(Flags, DevProp_PciDevice_InterruptType_LineBased))
        PhAppendStringBuilder2(&stringBuilder, L"Line based, ");
    if (BooleanFlagOn(Flags, DevProp_PciDevice_InterruptType_Msi))
        PhAppendStringBuilder2(&stringBuilder, L"Msi, ");
    if (BooleanFlagOn(Flags, DevProp_PciDevice_InterruptType_MsiX))
        PhAppendStringBuilder2(&stringBuilder, L"MsiX, ");

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhPrintPointer(pointer, UlongToPtr(Flags));
    PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);

    return PhFinalStringBuilderString(&stringBuilder);
}

_Function_class_(DEVPROP_FILL_CALLBACK)
VOID NTAPI DevPropFillPciDeviceInterruptSupport(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PDEVNODE_PROP Property,
    _In_ ULONG Flags
    )
{
    Property->Type = DevPropTypeUInt32;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = GetDevicePropertyUInt32(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->UInt32
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = GetClassPropertyUInt32(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->UInt32
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = GetClassPropertyUInt32(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->UInt32
            );
    }

    if (Property->Valid)
    {
        Property->AsString = DevPropPciDeviceInterruptSupportToString(Property->UInt32);
    }
}

_Function_class_(DEVPROP_FILL_CALLBACK)
VOID NTAPI DevPropFillBoolean(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PDEVNODE_PROP Property,
    _In_ ULONG Flags
    )
{
    Property->Type = DevPropTypeBoolean;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = GetDevicePropertyBoolean(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->Boolean
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = GetClassPropertyBoolean(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->Boolean
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = GetClassPropertyBoolean(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->Boolean
            );
    }

    if (Property->Valid)
    {
        if (Property->Boolean)
            Property->AsString = PhCreateString(L"true");
        else
            Property->AsString = PhCreateString(L"false");
    }
}

_Function_class_(DEVPROP_FILL_CALLBACK)
VOID
NTAPI
DevPropFillTimeStamp(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PDEVNODE_PROP Property,
    _In_ ULONG Flags
    )
{
    Property->Type = DevPropTypeTimeStamp;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = GetDevicePropertyTimeStamp(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->TimeStamp
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = GetClassPropertyTimeStamp(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->TimeStamp
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = GetClassPropertyTimeStamp(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->TimeStamp
            );
    }

    if (Property->Valid)
    {
        SYSTEMTIME systemTime;

        PhLargeIntegerToLocalSystemTime(&systemTime, &Property->TimeStamp);

        Property->AsString = PhFormatDateTime(&systemTime);
    }
}

_Function_class_(DEVPROP_FILL_CALLBACK)
VOID
NTAPI
DevPropFillStringList(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PDEVNODE_PROP Property,
    _In_ ULONG Flags
    )
{
    Property->Type = DevPropTypeStringList;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = GetDevicePropertyStringList(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->StringList
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = GetClassPropertyStringList(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->StringList
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = GetClassPropertyStringList(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->StringList
            );
    }

    if (Property->Valid && Property->StringList->Count > 0)
    {
        PH_STRING_BUILDER stringBuilder;
        PPH_STRING string;

        PhInitializeStringBuilder(&stringBuilder, Property->StringList->Count);

        for (ULONG i = 0; i < Property->StringList->Count - 1; i++)
        {
            string = Property->StringList->Items[i];

            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L", ");
        }

        string = Property->StringList->Items[Property->StringList->Count - 1];

        PhAppendStringBuilder(&stringBuilder, &string->sr);

        Property->AsString = PhFinalStringBuilderString(&stringBuilder);
        PhReferenceObject(Property->AsString);

        PhDeleteStringBuilder(&stringBuilder);
    }
}

_Function_class_(DEVPROP_FILL_CALLBACK)
VOID
NTAPI
DevPropFillStringOrStringList(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PDEVNODE_PROP Property,
    _In_ ULONG Flags
    )
{
    DevPropFillString(
        DeviceInfoSet,
        DeviceInfoData,
        PropertyKey,
        Property,
        Flags
        );
    if (!Property->Valid)
    {
        DevPropFillStringList(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            Property,
            Flags
            );
    }
}

static DEVNODE_PROP_TABLE_ENTRY DevNodePropTable[] =
{
    { L"Name", TRUE, 400, 0, DevKeyName, &DEVPKEY_NAME, DevPropFillString, 0 },
    { L"Manufacturer", TRUE, 180, 0, DevKeyManufacturer, &DEVPKEY_Device_Manufacturer, DevPropFillString, 0 },
    { L"Service", TRUE, 120, 0, DevKeyService, &DEVPKEY_Device_Service, DevPropFillString, 0 },
    { L"Class", TRUE, 120, 0, DevKeyClass, &DEVPKEY_Device_Class, DevPropFillString, 0 },
    { L"Enumerator", TRUE, 80, 0, DevKeyEnumeratorName, &DEVPKEY_Device_EnumeratorName, DevPropFillString, 0 },
    { L"Installed", TRUE, 160, 0, DevKeyInstallDate, &DEVPKEY_Device_InstallDate, DevPropFillTimeStamp, 0 },

    { L"First Installed", FALSE, 160, 0, DevKeyFirstInstallDate, &DEVPKEY_Device_FirstInstallDate, DevPropFillTimeStamp, 0 },
    { L"Last Arrival", FALSE, 160, 0, DevKeyLastArrivalDate, &DEVPKEY_Device_LastArrivalDate, DevPropFillTimeStamp, 0 },
    { L"Last Removal", FALSE, 160, 0, DevKeyLastRemovalDate, &DEVPKEY_Device_LastRemovalDate, DevPropFillTimeStamp, 0 },
    { L"Description", FALSE, 280, 0, DevKeyDeviceDesc, &DEVPKEY_Device_DeviceDesc, DevPropFillString, 0 },
    { L"Friendly Name", FALSE, 220, 0, DevKeyFriendlyName, &DEVPKEY_Device_FriendlyName, DevPropFillString, 0 },
    { L"Instance ID", FALSE, 240, DT_PATH_ELLIPSIS, DevKeyInstanceId, &DEVPKEY_Device_InstanceId, DevPropFillString, 0 },
    { L"Parent Instance ID", FALSE, 240, DT_PATH_ELLIPSIS, DevKeyParentInstanceId, &DEVPKEY_Device_Parent, DevPropFillString, 0 },
    { L"PDO Name", FALSE, 180, DT_PATH_ELLIPSIS, DevKeyPDOName, &DEVPKEY_Device_PDOName, DevPropFillString, 0 },
    { L"Location Info", FALSE, 180, DT_PATH_ELLIPSIS, DevKeyLocationInfo, &DEVPKEY_Device_LocationInfo, DevPropFillString, 0 },
    { L"Class GUID", FALSE, 80, 0, DevKeyClassGuid, &DEVPKEY_Device_ClassGuid, DevPropFillGuid, 0 },
    { L"Driver", FALSE, 180, DT_PATH_ELLIPSIS, DevKeyDriver, &DEVPKEY_Device_Driver, DevPropFillString, 0 },
    { L"Driver Version", FALSE, 80, 0, DevKeyDriverVersion, &DEVPKEY_Device_DriverVersion, DevPropFillString, 0 },
    { L"Driver Date", FALSE, 80, 0, DevKeyDriverDate, &DEVPKEY_Device_DriverDate, DevPropFillTimeStamp, 0 },
    { L"Firmware Date", FALSE, 80, 0, DevKeyFirmwareDate, &DEVPKEY_Device_FirmwareDate, DevPropFillTimeStamp, 0 },
    { L"Firmware Version", FALSE, 80, 0, DevKeyFirmwareVersion, &DEVPKEY_Device_FirmwareVersion, DevPropFillString, 0 },
    { L"Firmware Revision", FALSE, 80, 0, DevKeyFirmwareRevision, &DEVPKEY_Device_FirmwareRevision, DevPropFillString, 0 },
    { L"Has Problem", FALSE, 80, 0, DevKeyHasProblem, &DEVPKEY_Device_HasProblem, DevPropFillBoolean, 0 },
    { L"Problem Code", FALSE, 80, 0, DevKeyProblemCode, &DEVPKEY_Device_ProblemCode, DevPropFillUInt32, 0 },
    { L"Problem Status", FALSE, 80, 0, DevKeyProblemStatus, &DEVPKEY_Device_ProblemStatus, DevPropFillNTSTATUS, 0 },
    { L"Node Status Flags", FALSE, 80, 0, DevKeyDevNodeStatus, &DEVPKEY_Device_DevNodeStatus, DevPropFillUInt32Hex, 0 },
    { L"Capabilities", FALSE, 80, 0, DevKeyDevCapabilities, &DEVPKEY_Device_Capabilities, DevPropFillUInt32Hex, 0 },
    { L"Upper Filters", FALSE, 80, 0, DevKeyUpperFilters, &DEVPKEY_Device_UpperFilters, DevPropFillStringList, 0 },
    { L"Lower Filters", FALSE, 80, 0, DevKeyLowerFilters, &DEVPKEY_Device_LowerFilters, DevPropFillStringList, 0 },
    { L"Hardware IDs ", FALSE, 80, 0, DevKeyHardwareIds, &DEVPKEY_Device_HardwareIds, DevPropFillStringList, 0 },
    { L"Compatible IDs", FALSE, 80, 0, DevKeyCompatibleIds, &DEVPKEY_Device_CompatibleIds, DevPropFillStringList, 0 },
    { L"Configuration Flags", FALSE, 80, 0, DevKeyConfigFlags, &DEVPKEY_Device_ConfigFlags, DevPropFillUInt32Hex, 0 },
    { L"Number", FALSE, 80, 0, DevKeyUINumber, &DEVPKEY_Device_UINumber, DevPropFillUInt32, 0 },
    { L"Bus Type GUID", FALSE, 80, 0, DevKeyBusTypeGuid, &DEVPKEY_Device_BusTypeGuid, DevPropFillBusTypeGuid, 0 },
    { L"Legacy Bus Type", FALSE, 80, 0, DevKeyLegacyBusType, &DEVPKEY_Device_LegacyBusType, DevPropFillUInt32, 0 },
    { L"Bus Number", FALSE, 80, 0, DevKeyBusNumber, &DEVPKEY_Device_BusNumber, DevPropFillUInt32, 0 },
    //{ L"", FALSE, 80, 0, DevKey, &DEVPKEY_Device_Security, NULL, 0 },               // DEVPROP_TYPE_SECURITY_DESCRIPTOR
    { L"Security Descriptor", FALSE, 80, 0, DevKeySecuritySDS, &DEVPKEY_Device_SecuritySDS, DevPropFillString, 0 },
    { L"Type", FALSE, 80, 0, DevKeyDevType, &DEVPKEY_Device_DevType, DevPropFillUInt32, 0 },
    { L"Exclusive", FALSE, 80, 0, DevKeyExclusive, &DEVPKEY_Device_Exclusive, DevPropFillBoolean, 0 },
    { L"Characteristics", FALSE, 80, 0, DevKeyCharacteristics, &DEVPKEY_Device_Characteristics, DevPropFillUInt32Hex, 0 },
    { L"Address", FALSE, 80, 0, DevKeyAddress, &DEVPKEY_Device_Address, DevPropFillUInt32Hex, 0 },
    //{ L"", FALSE, 80, 0, DevKey, &DEVPKEY_Device_PowerData, NULL, 0 },              // DEVPROP_TYPE_BINARY
    { L"Removal Policy", FALSE, 80, 0, DevKeyRemovalPolicy, &DEVPKEY_Device_RemovalPolicy, DevPropFillUInt32, 0 },
    { L"Removal Policy Default", FALSE, 80, 0, DevKeyRemovalPolicyDefault, &DEVPKEY_Device_RemovalPolicyDefault, DevPropFillUInt32, 0 },
    { L"Removal Policy Override", FALSE, 80, 0, DevKeyRemovalPolicyOverride, &DEVPKEY_Device_RemovalPolicyOverride, DevPropFillUInt32, 0 },
    { L"Install State", FALSE, 80, 0, DevKeyInstallState, &DEVPKEY_Device_InstallState, DevPropFillUInt32, 0 },
    { L"Location Paths", FALSE, 80, 0, DevKeyLocationPaths, &DEVPKEY_Device_LocationPaths, DevPropFillStringList, 0 },
    { L"Base Container ID", FALSE, 80, 0, DevKeyBaseContainerId, &DEVPKEY_Device_BaseContainerId, DevPropFillGuid, 0 },
    { L"Ejection Relations", FALSE, 80, 0, DevKeyEjectionRelations, &DEVPKEY_Device_EjectionRelations, DevPropFillStringList, 0 },
    { L"Removal Relations", FALSE, 80, 0, DevKeyRemovalRelations, &DEVPKEY_Device_RemovalRelations, DevPropFillStringList, 0 },
    { L"Power Relations", FALSE, 80, 0, DevKeyPowerRelations, &DEVPKEY_Device_PowerRelations, DevPropFillStringList, 0 },
    { L"Bus Relations", FALSE, 80, 0, DevKeyBusRelations, &DEVPKEY_Device_BusRelations, DevPropFillStringList, 0 },
    { L"Children", FALSE, 80, 0, DevKeyChildren, &DEVPKEY_Device_Children, DevPropFillStringList, 0 },
    { L"Siblings", FALSE, 80, 0, DevKeySiblings, &DEVPKEY_Device_Siblings, DevPropFillStringList, 0 },
    { L"Transport Relations", FALSE, 80, 0, DevKeyTransportRelations, &DEVPKEY_Device_TransportRelations, DevPropFillStringList, 0 },
    { L"Reported", FALSE, 80, 0, DevKeyReported, &DEVPKEY_Device_Reported, DevPropFillBoolean, 0 },
    { L"Legacy", FALSE, 80, 0, DevKeyLegacy, &DEVPKEY_Device_Legacy, DevPropFillBoolean, 0 },
    { L"Container ID", FALSE, 80, 0, DevKeyContainerId, &DEVPKEY_Device_ContainerId, DevPropFillGuid, 0 },
    { L"Local Machine Container", FALSE, 80, 0, DevKeyInLocalMachineContainer, &DEVPKEY_Device_InLocalMachineContainer, DevPropFillBoolean, 0 },
    { L"Model", FALSE, 80, 0, DevKeyModel, &DEVPKEY_Device_Model, DevPropFillString, 0 },
    { L"Model ID", FALSE, 80, 0, DevKeyModelId, &DEVPKEY_Device_ModelId, DevPropFillGuid, 0 },
    { L"Friendly Name Attributes", FALSE, 80, 0, DevKeyFriendlyNameAttributes, &DEVPKEY_Device_FriendlyNameAttributes, DevPropFillUInt32Hex, 0 },
    { L"Manufacture Attributes", FALSE, 80, 0, DevKeyManufacturerAttributes, &DEVPKEY_Device_ManufacturerAttributes, DevPropFillUInt32Hex, 0 },
    { L"Presence Not for Device", FALSE, 80, 0, DevKeyPresenceNotForDevice, &DEVPKEY_Device_PresenceNotForDevice, DevPropFillBoolean, 0 },
    { L"Signal Strength", FALSE, 80, 0, DevKeySignalStrength, &DEVPKEY_Device_SignalStrength, DevPropFillInt32, 0 },
    { L"Associateable by User Action", FALSE, 80, 0, DevKeyIsAssociateableByUserAction, &DEVPKEY_Device_IsAssociateableByUserAction, DevPropFillBoolean, 0 },
    { L"Show Uninstall UI", FALSE, 80, 0, DevKeyShowInUninstallUI, &DEVPKEY_Device_ShowInUninstallUI, DevPropFillBoolean, 0 },
    { L"Numa Proximity Default", FALSE, 80, 0, DevKeyNumaProximityDomain, &DEVPKEY_Device_Numa_Proximity_Domain, DevPropFillUInt32, 0 },
    { L"DHP Rebalance Policy", FALSE, 80, 0, DevKeyDHPRebalancePolicy, &DEVPKEY_Device_DHP_Rebalance_Policy, DevPropFillUInt32, 0 },
    { L"Numa Node", FALSE, 80, 0, DevKeyNumaNode, &DEVPKEY_Device_Numa_Node, DevPropFillUInt32, 0 },
    { L"Bus Reported Description", FALSE, 80, 0, DevKeyBusReportedDeviceDesc, &DEVPKEY_Device_BusReportedDeviceDesc, DevPropFillString, 0 },
    { L"Present", FALSE, 80, 0, DevKeyIsPresent, &DEVPKEY_Device_IsPresent, DevPropFillBoolean, 0 },
    { L"Configuration ID", FALSE, 80, 0, DevKeyConfigurationId, &DEVPKEY_Device_ConfigurationId, DevPropFillString, 0 },
    { L"Reported IDs Hash", FALSE, 80, 0, DevKeyReportedDeviceIdsHash, &DEVPKEY_Device_ReportedDeviceIdsHash, DevPropFillUInt32, 0 },
    //{ L"", FALSE, 80, 0, DevKey, &DEVPKEY_Device_PhysicalDeviceLocation, NULL, 0 },    // DEVPROP_TYPE_BINARY
    { L"BIOS Name", FALSE, 80, 0, DevKeyBiosDeviceName, &DEVPKEY_Device_BiosDeviceName, DevPropFillString, 0 },
    { L"Problem Description", FALSE, 80, 0, DevKeyDriverProblemDesc, &DEVPKEY_Device_DriverProblemDesc, DevPropFillString, 0 },
    { L"Debugger Safe", FALSE, 80, 0, DevKeyDebuggerSafe, &DEVPKEY_Device_DebuggerSafe, DevPropFillUInt32, 0 },
    { L"Post Install in Progress", FALSE, 80, 0, DevKeyPostInstallInProgress, &DEVPKEY_Device_PostInstallInProgress, DevPropFillBoolean, 0 },
    { L"Stack", FALSE, 80, 0, DevKeyStack, &DEVPKEY_Device_Stack, DevPropFillStringList, 0 },
    { L"Extended Configuration IDs", FALSE, 80, 0, DevKeyExtendedConfigurationIds, &DEVPKEY_Device_ExtendedConfigurationIds, DevPropFillStringList, 0 },
    { L"Reboot Required", FALSE, 80, 0, DevKeyIsRebootRequired, &DEVPKEY_Device_IsRebootRequired, DevPropFillBoolean, 0 },
    { L"Dependency Providers", FALSE, 80, 0, DevKeyDependencyProviders, &DEVPKEY_Device_DependencyProviders, DevPropFillStringList, 0 },
    { L"Dependency Dependents", FALSE, 80, 0, DevKeyDependencyDependents, &DEVPKEY_Device_DependencyDependents, DevPropFillStringList, 0 },
    { L"Soft Restart Supported", FALSE, 80, 0, DevKeySoftRestartSupported, &DEVPKEY_Device_SoftRestartSupported, DevPropFillBoolean, 0 },
    { L"Extended Address", FALSE, 80, 0, DevKeyExtendedAddress, &DEVPKEY_Device_ExtendedAddress, DevPropFillUInt64Hex, 0 },
    { L"Assigned to Guest", FALSE, 80, 0, DevKeyAssignedToGuest, &DEVPKEY_Device_AssignedToGuest, DevPropFillBoolean, 0 },
    { L"Creator Process ID", FALSE, 80, 0, DevKeyCreatorProcessId, &DEVPKEY_Device_CreatorProcessId, DevPropFillUInt32, 0 },
    { L"Firmware Vendor", FALSE, 80, 0, DevKeyFirmwareVendor, &DEVPKEY_Device_FirmwareVendor, DevPropFillString, 0 },
    { L"Session ID", FALSE, 80, 0, DevKeySessionId, &DEVPKEY_Device_SessionId, DevPropFillUInt32, 0 },
    { L"Driver Description", FALSE, 80, 0, DevKeyDriverDesc, &DEVPKEY_Device_DriverDesc, DevPropFillString, 0 },
    { L"Driver INF Path", FALSE, 80, 0, DevKeyDriverInfPath, &DEVPKEY_Device_DriverInfPath, DevPropFillString, 0 },
    { L"Driver INF Section", FALSE, 80, 0, DevKeyDriverInfSection, &DEVPKEY_Device_DriverInfSection, DevPropFillString, 0 },
    { L"Driver INF Section Extended", FALSE, 80, 0, DevKeyDriverInfSectionExt, &DEVPKEY_Device_DriverInfSectionExt, DevPropFillString, 0 },
    { L"Matching ID", FALSE, 80, 0, DevKeyMatchingDeviceId, &DEVPKEY_Device_MatchingDeviceId, DevPropFillString, 0 },
    { L"Driver Provider", FALSE, 80, 0, DevKeyDriverProvider, &DEVPKEY_Device_DriverProvider, DevPropFillString, 0 },
    { L"Driver Property Page Provider", FALSE, 80, 0, DevKeyDriverPropPageProvider, &DEVPKEY_Device_DriverPropPageProvider, DevPropFillString, 0 },
    { L"Driver Co-installers", FALSE, 80, 0, DevKeyDriverCoInstallers, &DEVPKEY_Device_DriverCoInstallers, DevPropFillStringList, 0 },
    { L"Resource Picker Tags", FALSE, 80, 0, DevKeyResourcePickerTags, &DEVPKEY_Device_ResourcePickerTags, DevPropFillString, 0 },
    { L"Resource Picker Exceptions", FALSE, 80, 0, DevKeyResourcePickerExceptions, &DEVPKEY_Device_ResourcePickerExceptions, DevPropFillString, 0 },
    { L"Driver Rank", FALSE, 80, 0, DevKeyDriverRank, &DEVPKEY_Device_DriverRank, DevPropFillUInt32, 0 },
    { L"Driver LOGO Level", FALSE, 80, 0, DevKeyDriverLogoLevel, &DEVPKEY_Device_DriverLogoLevel, DevPropFillUInt32, 0 },
    { L"No Connect Sound", FALSE, 80, 0, DevKeyNoConnectSound, &DEVPKEY_Device_NoConnectSound, DevPropFillBoolean, 0 },
    { L"Generic Driver Installed", FALSE, 80, 0, DevKeyGenericDriverInstalled, &DEVPKEY_Device_GenericDriverInstalled, DevPropFillBoolean, 0 },
    { L"Additional Software Requested", FALSE, 80, 0, DevKeyAdditionalSoftwareRequested, &DEVPKEY_Device_AdditionalSoftwareRequested, DevPropFillBoolean, 0 },
    { L"Safe Removal Required", FALSE, 80, 0, DevKeySafeRemovalRequired, &DEVPKEY_Device_SafeRemovalRequired, DevPropFillBoolean, 0 },
    { L"Save Removal Required Override", FALSE, 80, 0, DevKeySafeRemovalRequiredOverride, &DEVPKEY_Device_SafeRemovalRequiredOverride, DevPropFillBoolean, 0 },

    { L"Package Model", FALSE, 80, 0, DevKeyPkgModel, &DEVPKEY_DrvPkg_Model, DevPropFillString, 0 },
    { L"Package Vendor Website", FALSE, 80, 0, DevKeyPkgVendorWebSite, &DEVPKEY_DrvPkg_VendorWebSite, DevPropFillString, 0 },
    { L"Package Description", FALSE, 80, 0, DevKeyPkgDetailedDescription, &DEVPKEY_DrvPkg_DetailedDescription, DevPropFillString, 0 },
    { L"Package Documentation", FALSE, 80, 0, DevKeyPkgDocumentationLink, &DEVPKEY_DrvPkg_DocumentationLink, DevPropFillString, 0 },
    { L"Package Icon", FALSE, 80, 0, DevKeyPkgIcon, &DEVPKEY_DrvPkg_Icon, DevPropFillStringList, 0 },
    { L"Package Branding Icon", FALSE, 80, 0, DevKeyPkgBrandingIcon, &DEVPKEY_DrvPkg_BrandingIcon, DevPropFillStringList, 0 },

    { L"Class Upper Filters", FALSE, 80, 0, DevKeyClassUpperFilters, &DEVPKEY_DeviceClass_UpperFilters, DevPropFillStringList, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { L"Class Lower Filters", FALSE, 80, 0, DevKeyClassLowerFilters, &DEVPKEY_DeviceClass_LowerFilters, DevPropFillStringList, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    //{ L"", FALSE, 80, 0, DevKey, &DEVPKEY_DeviceClass_Security, NULL, 0 },    // DEVPROP_TYPE_SECURITY_DESCRIPTOR
    { L"Class Security Descriptor", FALSE, 80, 0, DevKeyClassSecuritySDS, &DEVPKEY_DeviceClass_SecuritySDS, DevPropFillString, 0 },
    { L"Class Type", FALSE, 80, 0, DevKeyClassDevType, &DEVPKEY_DeviceClass_DevType, DevPropFillUInt32, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { L"Class Exclusive", FALSE, 80, 0, DevKeyClassExclusive, &DEVPKEY_DeviceClass_Exclusive, DevPropFillBoolean, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { L"Class Characteristics", FALSE, 80, 0, DevKeyClassCharacteristics, &DEVPKEY_DeviceClass_Characteristics, DevPropFillUInt32Hex, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { L"Class Device Name", FALSE, 80, 0, DevKeyClassName, &DEVPKEY_DeviceClass_Name, DevPropFillString, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { L"Class Name", FALSE, 80, 0, DevKeyClassClassName, &DEVPKEY_DeviceClass_ClassName, DevPropFillString, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { L"Class Icon", FALSE, 80, 0, DevKeyClassIcon, &DEVPKEY_DeviceClass_Icon, DevPropFillString, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { L"Class Installer", FALSE, 80, 0, DevKeyClassClassInstaller, &DEVPKEY_DeviceClass_ClassInstaller, DevPropFillString, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { L"Class Property Page Provider", FALSE, 80, 0, DevKeyClassPropPageProvider, &DEVPKEY_DeviceClass_PropPageProvider, DevPropFillString, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { L"Class No Install", FALSE, 80, 0, DevKeyClassNoInstallClass, &DEVPKEY_DeviceClass_NoInstallClass, DevPropFillBoolean, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { L"Class No Display", FALSE, 80, 0, DevKeyClassNoDisplayClass, &DEVPKEY_DeviceClass_NoDisplayClass, DevPropFillBoolean, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { L"Class Silent Install", FALSE, 80, 0, DevKeyClassSilentInstall, &DEVPKEY_DeviceClass_SilentInstall, DevPropFillBoolean, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { L"Class No Use Class", FALSE, 80, 0, DevKeyClassNoUseClass, &DEVPKEY_DeviceClass_NoUseClass, DevPropFillBoolean, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { L"Class Default Service", FALSE, 80, 0, DevKeyClassDefaultService, &DEVPKEY_DeviceClass_DefaultService, DevPropFillString, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { L"Class Icon Path", FALSE, 80, 0, DevKeyClassIconPath, &DEVPKEY_DeviceClass_IconPath, DevPropFillStringList, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { L"Class DHP Rebalance Opt-out", FALSE, 80, 0, DevKeyClassDHPRebalanceOptOut, &DEVPKEY_DeviceClass_DHPRebalanceOptOut, DevPropFillBoolean, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { L"Class Co-installers", FALSE, 80, 0, DevKeyClassClassCoInstallers, &DEVPKEY_DeviceClass_ClassCoInstallers, DevPropFillStringList, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },

    { L"Interface Friendly Name", FALSE, 80, 0, DevKeyInterfaceFriendlyName, &DEVPKEY_DeviceInterface_FriendlyName, DevPropFillString, 0 },
    { L"Interface Enabled", FALSE, 80, 0, DevKeyInterfaceEnabled, &DEVPKEY_DeviceInterface_Enabled, DevPropFillBoolean, 0 },
    { L"Interface Class GUID", FALSE, 80, 0, DevKeyInterfaceClassGuid, &DEVPKEY_DeviceInterface_ClassGuid, DevPropFillGuid, 0 },
    { L"Interface Reference", FALSE, 80, 0, DevKeyInterfaceReferenceString, &DEVPKEY_DeviceInterface_ReferenceString, DevPropFillString, 0 },
    { L"Interface Restricted", FALSE, 80, 0, DevKeyInterfaceRestricted, &DEVPKEY_DeviceInterface_Restricted, DevPropFillBoolean, 0 },
    { L"Interface Unrestricted Application Capabilities", FALSE, 80, 0, DevKeyInterfaceUnrestrictedAppCapabilities, &DEVPKEY_DeviceInterface_UnrestrictedAppCapabilities, DevPropFillStringList, 0 },
    { L"Interface Schematic Name", FALSE, 80, 0, DevKeyInterfaceSchematicName, &DEVPKEY_DeviceInterface_SchematicName, DevPropFillString, 0 },

    { L"Interface Class Default Interface", FALSE, 80, 0, DevKeyInterfaceClassDefaultInterface, &DEVPKEY_DeviceInterfaceClass_DefaultInterface, DevPropFillString, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { L"Interface Class Name", FALSE, 80, 0, DevKeyInterfaceClassName, &DEVPKEY_DeviceInterfaceClass_Name, DevPropFillString, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },

    { L"Container Address", FALSE, 80, 0, DevKeyContainerAddress, &DEVPKEY_DeviceContainer_Address, DevPropFillStringOrStringList, 0 },
    { L"Container Discovery Method", FALSE, 80, 0, DevKeyContainerDiscoveryMethod, &DEVPKEY_DeviceContainer_DiscoveryMethod, DevPropFillStringList, 0 },
    { L"Container Encrypted", FALSE, 80, 0, DevKeyContainerIsEncrypted, &DEVPKEY_DeviceContainer_IsEncrypted, DevPropFillBoolean, 0 },
    { L"Container Authenticated", FALSE, 80, 0, DevKeyContainerIsAuthenticated, &DEVPKEY_DeviceContainer_IsAuthenticated, DevPropFillBoolean, 0 },
    { L"Container Connected", FALSE, 80, 0, DevKeyContainerIsConnected, &DEVPKEY_DeviceContainer_IsConnected, DevPropFillBoolean, 0 },
    { L"Container Paired", FALSE, 80, 0, DevKeyContainerIsPaired, &DEVPKEY_DeviceContainer_IsPaired, DevPropFillBoolean, 0 },
    { L"Container Icon", FALSE, 80, 0, DevKeyContainerIcon, &DEVPKEY_DeviceContainer_Icon, DevPropFillString, 0 },
    { L"Container Version", FALSE, 80, 0, DevKeyContainerVersion, &DEVPKEY_DeviceContainer_Version, DevPropFillString, 0 },
    { L"Container Last Seen", FALSE, 80, 0, DevKeyContainerLastSeen, &DEVPKEY_DeviceContainer_Last_Seen, DevPropFillTimeStamp, 0 },
    { L"Container Last Connected", FALSE, 80, 0, DevKeyContainerLastConnected, &DEVPKEY_DeviceContainer_Last_Connected, DevPropFillTimeStamp, 0 },
    { L"Container Show in Disconnected State", FALSE, 80, 0, DevKeyContainerIsShowInDisconnectedState, &DEVPKEY_DeviceContainer_IsShowInDisconnectedState, DevPropFillBoolean, 0 },
    { L"Container Local Machine", FALSE, 80, 0, DevKeyContainerIsLocalMachine, &DEVPKEY_DeviceContainer_IsLocalMachine, DevPropFillBoolean, 0 },
    { L"Container Metadata Path", FALSE, 80, 0, DevKeyContainerMetadataPath, &DEVPKEY_DeviceContainer_MetadataPath, DevPropFillString, 0 },
    { L"Container Metadata Search in Progress", FALSE, 80, 0, DevKeyContainerIsMetadataSearchInProgress, &DEVPKEY_DeviceContainer_IsMetadataSearchInProgress, DevPropFillBoolean, 0 },
    //{ L"", FALSE, 80, 0, DevKey, &DEVPKEY_DeviceContainer_MetadataChecksum, NULL, 0 },            // DEVPROP_TYPE_BINARY
    { L"Container Not Interesting For Display", FALSE, 80, 0, DevKeyContainerIsNotInterestingForDisplay, &DEVPKEY_DeviceContainer_IsNotInterestingForDisplay, DevPropFillBoolean, 0 },
    { L"Container Launch on Connect", FALSE, 80, 0, DevKeyContainerLaunchDeviceStageOnDeviceConnect, &DEVPKEY_DeviceContainer_LaunchDeviceStageOnDeviceConnect, DevPropFillBoolean, 0 },
    { L"Container Launch from Explorer", FALSE, 80, 0, DevKeyContainerLaunchDeviceStageFromExplorer, &DEVPKEY_DeviceContainer_LaunchDeviceStageFromExplorer, DevPropFillBoolean, 0 },
    { L"Container Baseline Experience ID", FALSE, 80, 0, DevKeyContainerBaselineExperienceId, &DEVPKEY_DeviceContainer_BaselineExperienceId, DevPropFillGuid, 0 },
    { L"Container Uniquely Identifiable", FALSE, 80, 0, DevKeyContainerIsDeviceUniquelyIdentifiable, &DEVPKEY_DeviceContainer_IsDeviceUniquelyIdentifiable, DevPropFillBoolean, 0 },
    { L"Container Association", FALSE, 80, 0, DevKeyContainerAssociationArray, &DEVPKEY_DeviceContainer_AssociationArray, DevPropFillStringList, 0 },
    { L"Container Description", FALSE, 80, 0, DevKeyContainerDeviceDescription1, &DEVPKEY_DeviceContainer_DeviceDescription1, DevPropFillString, 0 },
    { L"Container Description Other", FALSE, 80, 0, DevKeyContainerDeviceDescription2, &DEVPKEY_DeviceContainer_DeviceDescription2, DevPropFillString, 0 },
    { L"Container Has Problem", FALSE, 80, 0, DevKeyContainerHasProblem, &DEVPKEY_DeviceContainer_HasProblem, DevPropFillBoolean, 0 },
    { L"Container Shared Device", FALSE, 80, 0, DevKeyContainerIsSharedDevice, &DEVPKEY_DeviceContainer_IsSharedDevice, DevPropFillBoolean, 0 },
    { L"Container Network Device", FALSE, 80, 0, DevKeyContainerIsNetworkDevice, &DEVPKEY_DeviceContainer_IsNetworkDevice, DevPropFillBoolean, 0 },
    { L"Container Default Device", FALSE, 80, 0, DevKeyContainerIsDefaultDevice, &DEVPKEY_DeviceContainer_IsDefaultDevice, DevPropFillBoolean, 0 },
    { L"Container Metadata Cabinet", FALSE, 80, 0, DevKeyContainerMetadataCabinet, &DEVPKEY_DeviceContainer_MetadataCabinet, DevPropFillString, 0 },
    { L"Container Requires Pairing Elevation", FALSE, 80, 0, DevKeyContainerRequiresPairingElevation, &DEVPKEY_DeviceContainer_RequiresPairingElevation, DevPropFillBoolean, 0 },
    { L"Container Experience ID", FALSE, 80, 0, DevKeyContainerExperienceId, &DEVPKEY_DeviceContainer_ExperienceId, DevPropFillGuid, 0 },
    { L"Container Category", FALSE, 80, 0, DevKeyContainerCategory, &DEVPKEY_DeviceContainer_Category, DevPropFillStringList, 0 },
    { L"Container Category Description", FALSE, 80, 0, DevKeyContainerCategoryDescSingular, &DEVPKEY_DeviceContainer_Category_Desc_Singular, DevPropFillStringList, 0 },
    { L"Container Category Description Plural", FALSE, 80, 0, DevKeyContainerCategoryDescPlural, &DEVPKEY_DeviceContainer_Category_Desc_Plural, DevPropFillStringList, 0 },
    { L"Container Category Icon", FALSE, 80, 0, DevKeyContainerCategoryIcon, &DEVPKEY_DeviceContainer_Category_Icon, DevPropFillString, 0 },
    { L"Container Category Group Description", FALSE, 80, 0, DevKeyContainerCategoryGroupDesc, &DEVPKEY_DeviceContainer_CategoryGroup_Desc, DevPropFillStringList, 0 },
    { L"Container Category Group Icon", FALSE, 80, 0, DevKeyContainerCategoryGroupIcon, &DEVPKEY_DeviceContainer_CategoryGroup_Icon, DevPropFillString, 0 },
    { L"Container Primary Category", FALSE, 80, 0, DevKeyContainerPrimaryCategory, &DEVPKEY_DeviceContainer_PrimaryCategory, DevPropFillString, 0 },
    { L"Container Unpair Uninstall", FALSE, 80, 0, DevKeyContainerUnpairUninstall, &DEVPKEY_DeviceContainer_UnpairUninstall, DevPropFillBoolean, 0 },
    { L"Container Requires Uninstall Elevation", FALSE, 80, 0, DevKeyContainerRequiresUninstallElevation, &DEVPKEY_DeviceContainer_RequiresUninstallElevation, DevPropFillBoolean, 0 },
    { L"Container Function Sub-rank", FALSE, 80, 0, DevKeyContainerDeviceFunctionSubRank, &DEVPKEY_DeviceContainer_DeviceFunctionSubRank, DevPropFillUInt32, 0 },
    { L"Container Always Show Connected ", FALSE, 80, 0, DevKeyContainerAlwaysShowDeviceAsConnected, &DEVPKEY_DeviceContainer_AlwaysShowDeviceAsConnected, DevPropFillBoolean, 0 },
    { L"Container Control Flags", FALSE, 80, 0, DevKeyContainerConfigFlags, &DEVPKEY_DeviceContainer_ConfigFlags, DevPropFillUInt32Hex, 0 },
    { L"Container Privileged Package Family Names", FALSE, 80, 0, DevKeyContainerPrivilegedPackageFamilyNames, &DEVPKEY_DeviceContainer_PrivilegedPackageFamilyNames, DevPropFillStringList, 0 },
    { L"Container Custom Privileged Package Family Names", FALSE, 80, 0, DevKeyContainerCustomPrivilegedPackageFamilyNames, &DEVPKEY_DeviceContainer_CustomPrivilegedPackageFamilyNames, DevPropFillStringList, 0 },
    { L"Container Reboot Required", FALSE, 80, 0, DevKeyContainerIsRebootRequired, &DEVPKEY_DeviceContainer_IsRebootRequired, DevPropFillBoolean, 0 },
    { L"Container Friendly Name", FALSE, 80, 0, DevKeyContainerFriendlyName, &DEVPKEY_DeviceContainer_FriendlyName, DevPropFillString, 0 },
    { L"Container Manufacture", FALSE, 80, 0, DevKeyContainerManufacturer, &DEVPKEY_DeviceContainer_Manufacturer, DevPropFillString, 0 },
    { L"Container Model Name", FALSE, 80, 0, DevKeyContainerModelName, &DEVPKEY_DeviceContainer_ModelName, DevPropFillString, 0 },
    { L"Container Model Number", FALSE, 80, 0, DevKeyContainerModelNumber, &DEVPKEY_DeviceContainer_ModelNumber, DevPropFillString, 0 },
    { L"Container Install in Progress", FALSE, 80, 0, DevKeyContainerInstallInProgress, &DEVPKEY_DeviceContainer_InstallInProgress, DevPropFillBoolean, 0 },

    { L"Object Type", FALSE, 80, 0, DevKeyObjectType, &DEVPKEY_DevQuery_ObjectType, DevPropFillUInt32, 0 },

    { L"PCI Interrupt Support", FALSE, 80, 0, DevKeyPciInterruptSupport, &DEVPKEY_PciDevice_InterruptSupport, DevPropFillPciDeviceInterruptSupport, 0 },
    { L"PCI Express Capability Control", FALSE, 80, 0, DevKeyPciExpressCapabilityControl, &DEVPKEY_PciRootBus_PCIExpressCapabilityControl, DevPropFillBoolean, 0 },
    { L"PCI Native Express Control", FALSE, 80, 0, DevKeyPciNativeExpressControl, &DEVPKEY_PciRootBus_NativePciExpressControl, DevPropFillBoolean, 0 },
    { L"PCI System MSI Support", FALSE, 80, 0, DevKeyPciSystemMsiSupport, &DEVPKEY_PciRootBus_SystemMsiSupport, DevPropFillBoolean, 0 },

    { L"Storage Portable", FALSE, 80, 0, DevKeyStoragePortable, &DEVPKEY_Storage_Portable, DevPropFillBoolean, 0 },
    { L"Storage Removable Media", FALSE, 80, 0, DevKeyStorageRemovableMedia, &DEVPKEY_Storage_Removable_Media, DevPropFillBoolean, 0 },
    { L"Storage System Critical", FALSE, 80, 0, DevKeyStorageSystemCritical, &DEVPKEY_Storage_System_Critical, DevPropFillBoolean, 0 },
    { L"Storage Disk Number", FALSE, 80, 0, DevKeyStorageDiskNumber, &DEVPKEY_Storage_Disk_Number, DevPropFillUInt32, 0 },
    { L"Storage Disk Partition Number", FALSE, 80, 0, DevKeyStoragePartitionNumber, &DEVPKEY_Storage_Partition_Number, DevPropFillUInt32, 0 },
};

#if DEBUG
static BOOLEAN BreakOnDeviceProperty = FALSE;
static DEVNODE_PROP_KEY BreakOnDevPropKey = 0;
#endif

PDEVNODE_PROP GetDeviceNodeProperty(
    _In_ PDEVICE_NODE Node,
    _In_ DEVNODE_PROP_KEY Key
    )
{
    PDEVNODE_PROP prop;

    prop = &Node->Properties[Key];
    if (!prop->Initialized)
    {
        PDEVNODE_PROP_TABLE_ENTRY entry;

        entry = &DevNodePropTable[Key];

#if DEBUG
        if (BreakOnDeviceProperty && (BreakOnDevPropKey == Key))
            __debugbreak();
#endif

        entry->Callback(
            Node->DeviceInfoHandle,
            &Node->DeviceInfoData,
            entry->PropKey,
            prop,
            entry->CallbackFlags
            );

        prop->Initialized = TRUE;
    }

    return prop;
}

PDEVICE_NODE NTAPI AddDeviceNode(
    _In_ PDEVICE_TREE Tree,
    _In_ PSP_DEVINFO_DATA DeviceInfoData
    )
{
    PDEVICE_NODE node;

    node = PhCreateObjectZero(sizeof(DEVICE_NODE), DeviceNodeType);

    node->DeviceInfoHandle = Tree->DeviceInfoHandle;
    RtlCopyMemory(&node->DeviceInfoData, DeviceInfoData, sizeof(SP_DEVINFO_DATA));

    PhInitializeTreeNewNode(&node->Node);
    RtlZeroMemory(node->TextCache, sizeof(PH_STRINGREF) * ARRAYSIZE(node->TextCache));
    node->Node.TextCache = node->TextCache;
    node->Node.TextCacheSize = ARRAYSIZE(node->TextCache);

    node->Children = PhCreateList(1);
    node->IconIndex = ULONG_MAX;

    //
    // Only get the minimum here, the rest will be retrieved later if necessary.
    // For convenience later, other frequently referenced items are gotten here too.
    //

    node->InstanceId = GetDeviceNodeProperty(node, DevKeyInstanceId)->AsString;
    if (node->InstanceId)
    {
        //
        // If this is the root enumerator override some properties.
        //
        if (PhEqualStringRef(&node->InstanceId->sr, &RootInstanceId, TRUE))
        {
            WCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1];
            ULONG length = MAX_COMPUTERNAME_LENGTH + 1;
            PDEVNODE_PROP prop;

            prop = &node->Properties[DevKeyName];
            assert(!prop->Initialized);

            if (GetComputerNameW(computerName, &length))
            {
                prop->AsString = PhCreateStringEx(computerName, length * sizeof(WCHAR));
            }

            prop->Initialized = TRUE;
            node->IconIndex = 0; // application icon
        }

        PhReferenceObject(node->InstanceId);
    }

    node->ParentInstanceId = GetDeviceNodeProperty(node, DevKeyParentInstanceId)->AsString;
    if (node->ParentInstanceId)
        PhReferenceObject(node->ParentInstanceId);

    GetDeviceNodeProperty(node, DevKeyProblemCode);
    if (node->Properties[DevKeyProblemCode].Valid)
        node->ProblemCode = node->Properties[DevKeyProblemCode].UInt32;
    else
        node->ProblemCode = CM_PROB_PHANTOM;

    GetDeviceNodeProperty(node, DevKeyDevNodeStatus);
    if (node->Properties[DevKeyDevNodeStatus].Valid)
        node->DevNodeStatus = node->Properties[DevKeyDevNodeStatus].UInt32;
    else
        node->DevNodeStatus = 0;

    GetDeviceNodeProperty(node, DevKeyDevCapabilities);
    if (node->Properties[DevKeyDevCapabilities].Valid)
        node->Capabilities = node->Properties[DevKeyDevCapabilities].UInt32;
    else
        node->Capabilities = 0;

    GetDeviceNodeProperty(node, DevKeyUpperFilters);
    GetDeviceNodeProperty(node, DevKeyClassUpperFilters);
    if ((node->Properties[DevKeyUpperFilters].Valid &&
         (node->Properties[DevKeyUpperFilters].StringList->Count > 0)) ||
        (node->Properties[DevKeyClassUpperFilters].Valid &&
         (node->Properties[DevKeyClassUpperFilters].StringList->Count > 0)))
    {
        node->HasUpperFilters = TRUE;
    }

    GetDeviceNodeProperty(node, DevKeyLowerFilters);
    GetDeviceNodeProperty(node, DevKeyClassLowerFilters);
    if ((node->Properties[DevKeyLowerFilters].Valid &&
         (node->Properties[DevKeyLowerFilters].StringList->Count > 0)) ||
        (node->Properties[DevKeyClassLowerFilters].Valid &&
         (node->Properties[DevKeyClassLowerFilters].StringList->Count > 0)))
    {
        node->HasLowerFilters = TRUE;
    }

    if (node->IconIndex == ULONG_MAX)
    {
        HICON iconHandle;

        if (SetupDiLoadDeviceIcon(
            node->DeviceInfoHandle,
            DeviceInfoData,
            DeviceIconSize.X,
            DeviceIconSize.X,
            0,
            &iconHandle
            ))
        {
            node->IconIndex = PhImageListAddIcon(DeviceImageList, iconHandle);
            DestroyIcon(iconHandle);
        }
        else
        {
            node->IconIndex = 0; // Must be set to zero (dmex)
        }
    }

    if (DeviceTreeFilterSupport.NodeList)
        node->Node.Visible = PhApplyTreeNewFiltersToNode(&DeviceTreeFilterSupport, &node->Node);
    else
        node->Node.Visible = TRUE;

    PhAddItemList(Tree->DeviceList, node);

    return node;
}

VOID DeviceNodeDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PDEVICE_NODE node = Object;

    PhDereferenceObject(node->Children);

    for (ULONG i = 0; i < ARRAYSIZE(node->Properties); i++)
    {
        PDEVNODE_PROP prop;

        prop = &node->Properties[i];

        if (prop->Valid)
        {
            if (prop->Type == DevPropTypeString)
            {
                PhDereferenceObject(prop->String);
            }
            else if (prop->Type == DevPropTypeStringList)
            {
                for (ULONG j = 0; j < prop->StringList->Count; j++)
                {
                    PhDereferenceObject(prop->StringList->Items[j]);
                }

                PhDereferenceObject(prop->StringList);
            }
        }

        if (prop->AsString)
            PhDereferenceObject(prop->AsString);
    }

    if (node->InstanceId)
        PhDereferenceObject(node->InstanceId);

    if (node->ParentInstanceId)
        PhDereferenceObject(node->ParentInstanceId);
}

VOID DeviceTreeDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PDEVICE_TREE tree = Object;

    for (ULONG i = 0; i < tree->DeviceList->Count; i++)
    {
        PDEVICE_NODE node;

        node = tree->DeviceList->Items[i];

        PhDereferenceObject(node);
    }

    PhDereferenceObject(tree->DeviceList);
    PhDereferenceObject(tree->DeviceRootList);

    if (tree->DeviceInfoHandle != INVALID_HANDLE_VALUE)
    {
        SetupDiDestroyDeviceInfoList(tree->DeviceInfoHandle);
        tree->DeviceInfoHandle = INVALID_HANDLE_VALUE;
    }
}

static int __cdecl DeviceListSortByNameFunction(
    const void* Left,
    const void* Right
    )
{
    PDEVICE_NODE lhsNode;
    PDEVICE_NODE rhsNode;
    PDEVNODE_PROP lhs;
    PDEVNODE_PROP rhs;

    lhsNode = *(PDEVICE_NODE*)Left;
    rhsNode = *(PDEVICE_NODE*)Right;
    lhs = GetDeviceNodeProperty(lhsNode, DevKeyName);
    rhs = GetDeviceNodeProperty(rhsNode, DevKeyName);

    return PhCompareStringWithNull(lhs->AsString, rhs->AsString, TRUE);
}

PDEVICE_TREE CreateDeviceTree(
    VOID
    )
{
    PDEVICE_TREE tree;
    PDEVICE_NODE root;

    tree = PhCreateObjectZero(sizeof(DEVICE_TREE), DeviceTreeType);

    tree->DeviceRootList = PhCreateList(1);
    tree->DeviceList = PhCreateList(40);

    tree->DeviceInfoHandle = SetupDiGetClassDevsW(
        NULL,
        NULL,
        NULL,
        DIGCF_ALLCLASSES
        );
    if (tree->DeviceInfoHandle == INVALID_HANDLE_VALUE)
    {
        return tree;
    }

    for (ULONG i = 0;; i++)
    {
        SP_DEVINFO_DATA deviceInfoData;

        memset(&deviceInfoData, 0, sizeof(SP_DEVINFO_DATA));
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        if (!SetupDiEnumDeviceInfo(tree->DeviceInfoHandle, i, &deviceInfoData))
            break;

        if (!ShowSoftwareComponents)
        {
            if (IsEqualGUID(&deviceInfoData.ClassGuid, &GUID_DEVCLASS_SOFTWARECOMPONENT))
                continue;
        }

        AddDeviceNode(tree, &deviceInfoData);
    }

    //
    // Link the device relations.
    //
    root = NULL;

    for (ULONG i = 0; i < tree->DeviceList->Count; i++)
    {
        BOOLEAN found;
        PDEVICE_NODE node;
        PDEVICE_NODE other;

        found = FALSE;

        node = tree->DeviceList->Items[i];

        for (ULONG j = 0; j < tree->DeviceList->Count; j++)
        {
            other = tree->DeviceList->Items[j];

            if (node == other)
            {
                continue;
            }

            if (!other->InstanceId || !node->ParentInstanceId)
            {
                continue;
            }

            if (!PhEqualString(other->InstanceId, node->ParentInstanceId, TRUE))
            {
                continue;
            }

            found = TRUE;

            node->Parent = other;

            if (!other->Child)
            {
                other->Child = node;
                break;
            }

            other = other->Child;
            while (other->Sibling)
            {
                other = other->Sibling;
            }

            other->Sibling = node;
            break;
        }

        if (found)
        {
            continue;
        }

        other = root;
        if (!other)
        {
            root = node;
            continue;
        }

        while (other->Sibling)
        {
            other = other->Sibling;
        }

        other->Sibling = node;
    }

    for (ULONG i = 0; i < tree->DeviceList->Count; i++)
    {
        PDEVICE_NODE node;
        PDEVICE_NODE child;

        node = tree->DeviceList->Items[i];

        child = node->Child;
        while (child)
        {
            PhAddItemList(node->Children, child);
            child = child->Sibling;
        }

        if (PhGetIntegerSetting(SETTING_NAME_DEVICE_SORT_CHILDREN_BY_NAME))
        {
            qsort(
                node->Children->Items,
                node->Children->Count,
                sizeof(PVOID),
                DeviceListSortByNameFunction
                );
        }
    }

    while (root)
    {
        PhAddItemList(tree->DeviceRootList, root);
        root = root->Sibling;
    }

    return tree;
}

VOID NTAPI DeviceTreePublish(
    _In_ PDEVICE_TREE Tree
    )
{
    PDEVICE_TREE oldDeviceTree;

    TreeNew_SetRedraw(DeviceTreeHandle, FALSE);

    oldDeviceTree = DeviceTree;
    DeviceTree = Tree;

    DeviceFilterList.AllocatedCount = DeviceTree->DeviceList->AllocatedCount;
    DeviceFilterList.Count = DeviceTree->DeviceList->Count;
    DeviceFilterList.Items = DeviceTree->DeviceList->Items;

    TreeNew_SetRedraw(DeviceTreeHandle, TRUE);

    TreeNew_NodesStructured(DeviceTreeHandle);

    if (DeviceTreeFilterSupport.FilterList)
        PhApplyTreeNewFilters(&DeviceTreeFilterSupport);

    if (oldDeviceTree)
        PhDereferenceObject(oldDeviceTree);
}

NTSTATUS NTAPI DeviceTreePublishThread(
    _In_ PVOID Parameter
    )
{
    ProcessHacker_Invoke(DeviceTreePublish, CreateDeviceTree());

    return STATUS_SUCCESS;
}

VOID DeviceTreePublicAsync(
    VOID
    )
{
    PhCreateThread2(DeviceTreePublishThread, NULL);
}

_Function_class_(PCM_NOTIFY_CALLBACK)
ULONG CALLBACK CmNotifyCallback(
    _In_ HCMNOTIFICATION hNotify,
    _In_opt_ PVOID Context,
    _In_ CM_NOTIFY_ACTION Action,
    _In_reads_bytes_(EventDataSize) PCM_NOTIFY_EVENT_DATA EventData,
    _In_ ULONG EventDataSize
    )
{
    if (DeviceTabSelected && AutoRefreshDeviceTree)
        ProcessHacker_Invoke(DeviceTreePublish, CreateDeviceTree());

    return ERROR_SUCCESS;
}

VOID InvalidateDeviceTree(
    _In_ PDEVICE_TREE Tree
    )
{
    for (ULONG i = 0; i < Tree->DeviceList->Count; i++)
    {
        PDEVICE_NODE node;

        node = Tree->DeviceList->Items[i];

        PhInvalidateTreeNewNode(&node->Node, TN_CACHE_COLOR);
        TreeNew_InvalidateNode(DeviceTreeHandle, &node->Node);
    }
}

BOOLEAN DeviceTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PDEVICE_NODE node = (PDEVICE_NODE)Node;

    if (!ShowDisconnected && (node->ProblemCode == CM_PROB_PHANTOM))
        return FALSE;

    if (PhIsNullOrEmptyString(ToolStatusInterface->GetSearchboxText()))
        return TRUE;

    for (ULONG i = 0; i < ARRAYSIZE(node->Properties); i++)
    {
        PDEVNODE_PROP prop;

        prop = GetDeviceNodeProperty(node, i);

        if (PhIsNullOrEmptyString(prop->AsString))
            continue;

        if (ToolStatusInterface->WordMatch(&prop->AsString->sr))
            return TRUE;
    }

    return FALSE;
}

VOID NTAPI DeviceTreeSearchChangedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (DeviceTabSelected)
        PhApplyTreeNewFilters(&DeviceTreeFilterSupport);
}

static int __cdecl DeviceTreeSortFunction(
    const void* Left,
    const void* Right
    )
{
    int sortResult;
    PDEVICE_NODE lhsNode;
    PDEVICE_NODE rhsNode;
    PDEVNODE_PROP lhs;
    PDEVNODE_PROP rhs;
    PH_STRINGREF srl;
    PH_STRINGREF srr;

    sortResult = 0;
    lhsNode = *(PDEVICE_NODE*)Left;
    rhsNode = *(PDEVICE_NODE*)Right;
    lhs = GetDeviceNodeProperty(lhsNode, DeviceTreeSortColumn);
    rhs = GetDeviceNodeProperty(rhsNode, DeviceTreeSortColumn);

    assert(lhs->Type == rhs->Type);

    if (!lhs->Valid && !rhs->Valid)
    {
        sortResult = 0;
    }
    else if (lhs->Valid && !rhs->Valid)
    {
        sortResult = 1;
    }
    else if (!lhs->Valid && rhs->Valid)
    {
        sortResult = -1;
    }
    else
    {
        switch (lhs->Type)
        {
        case DevPropTypeString:
            sortResult = PhCompareString(lhs->String, rhs->String, TRUE);
            break;
        case DevPropTypeUInt64:
            sortResult = uint64cmp(lhs->UInt64, rhs->UInt64);
            break;
        case DevPropTypeUInt32:
            sortResult = uint64cmp(lhs->UInt32, rhs->UInt32);
            break;
        case DevPropTypeInt32:
        case DevPropTypeNTSTATUS:
            sortResult = int64cmp(lhs->Int32, rhs->Int32);
            break;
        case DevPropTypeGUID:
            sortResult = memcmp(&lhs->Guid, &rhs->Guid, sizeof(GUID));
            break;
        case DevPropTypeBoolean:
            {
                if (lhs->Boolean && !rhs->Boolean)
                    sortResult = 1;
                else if (!lhs->Boolean && rhs->Boolean)
                    sortResult = -1;
                else
                    sortResult = 0;
            }
            break;
        case DevPropTypeTimeStamp:
            sortResult = int64cmp(lhs->TimeStamp.QuadPart, rhs->TimeStamp.QuadPart);
            break;
        case DevPropTypeStringList:
        {
            srl = PhGetStringRef(lhs->AsString);
            srr = PhGetStringRef(rhs->AsString);
            sortResult = PhCompareStringRef(&srl, &srr, TRUE);
            break;
        }
        default:
            assert(FALSE);
        }
    }

    if (sortResult == 0)
    {
        srl = PhGetStringRef(lhsNode->Properties[DevKeyName].AsString);
        srr = PhGetStringRef(rhsNode->Properties[DevKeyName].AsString);
        sortResult = PhCompareStringRef(&srl, &srr, TRUE);
    }

    return PhModifySort(sortResult, DeviceTreeSortOrder);
}

BOOLEAN NTAPI DeviceTreeCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PDEVICE_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            if (!DeviceTree)
            {
                getChildren->Children = NULL;
                getChildren->NumberOfChildren = 0;
            }
            else
            {
                node = (PDEVICE_NODE)getChildren->Node;

                if (DeviceTreeSortOrder == NoSortOrder)
                {
                    if (!node)
                    {
                        if (PhGetIntegerSetting(SETTING_NAME_DEVICE_SHOW_ROOT))
                        {
                            getChildren->Children = (PPH_TREENEW_NODE*)DeviceTree->DeviceRootList->Items;
                            getChildren->NumberOfChildren = DeviceTree->DeviceRootList->Count;
                        }
                        else if (DeviceTree->DeviceRootList->Count)
                        {
                            PDEVICE_NODE root;

                            root = DeviceTree->DeviceRootList->Items[0];

                            getChildren->Children = (PPH_TREENEW_NODE*)root->Children->Items;
                            getChildren->NumberOfChildren = root->Children->Count;
                        }
                    }
                    else
                    {
                        getChildren->Children = (PPH_TREENEW_NODE*)node->Children->Items;
                        getChildren->NumberOfChildren = node->Children->Count;
                    }
                }
                else
                {
                    if (!node)
                    {
                        if (DeviceTreeSortColumn < MaxDevKey)
                        {
                            qsort(
                                DeviceTree->DeviceList->Items,
                                DeviceTree->DeviceList->Count,
                                sizeof(PVOID),
                                DeviceTreeSortFunction
                                );
                        }
                    }

                    getChildren->Children = (PPH_TREENEW_NODE*)DeviceTree->DeviceList->Items;
                    getChildren->NumberOfChildren = DeviceTree->DeviceList->Count;
                }
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;
            node = (PDEVICE_NODE)isLeaf->Node;

            if (DeviceTreeSortOrder == NoSortOrder)
                isLeaf->IsLeaf = node->Children->Count == 0;
            else
                isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;
            node = (PDEVICE_NODE)getCellText->Node;

            PPH_STRING text = GetDeviceNodeProperty(node, getCellText->Id)->AsString;

            getCellText->Text = PhGetStringRef(text);
            getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = Parameter1;
            node = (PDEVICE_NODE)getNodeColor->Node;

            getNodeColor->Flags = TN_CACHE | TN_AUTO_FORECOLOR;

            if (node->DevNodeStatus & DN_HAS_PROBLEM && (node->ProblemCode != CM_PROB_DISABLED))
            {
                getNodeColor->BackColor = DeviceProblemColor;
            }
            else if ((node->Capabilities & CM_DEVCAP_HARDWAREDISABLED) || (node->ProblemCode == CM_PROB_DISABLED))
            {
                getNodeColor->BackColor = DeviceDisabledColor;
            }
            else if (node->ProblemCode == CM_PROB_PHANTOM)
            {
                getNodeColor->ForeColor = DeviceDisconnectedColor;
                ClearFlag(getNodeColor->Flags, TN_AUTO_FORECOLOR);
            }
            else if ((HighlightUpperFiltered && node->HasUpperFilters) || (HighlightLowerFiltered && node->HasLowerFilters))
            {
                getNodeColor->BackColor = DeviceHighlightColor;
            }
        }
        return TRUE;
    case TreeNewGetNodeIcon:
        {
            PPH_TREENEW_GET_NODE_ICON getNodeIcon = Parameter1;

            node = (PDEVICE_NODE)getNodeIcon->Node;
            getNodeIcon->Icon = (HICON)(ULONG_PTR)node->IconIndex;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            TreeNew_GetSort(hwnd, &DeviceTreeSortColumn, &DeviceTreeSortOrder);
            TreeNew_NodesStructured(hwnd);
            if (DeviceTreeFilterSupport.FilterList)
                PhApplyTreeNewFilters(&DeviceTreeFilterSupport);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;
            PPH_EMENU menu;
            PPH_EMENU subMenu;
            PPH_EMENU_ITEM selectedItem;
            PPH_EMENU_ITEM autoRefresh;
            PPH_EMENU_ITEM showDisconnectedDevices;
            PPH_EMENU_ITEM showSoftwareDevices;
            PPH_EMENU_ITEM highlightUpperFiltered;
            PPH_EMENU_ITEM highlightLowerFiltered;
            PPH_EMENU_ITEM gotoServiceItem;
            PPH_EMENU_ITEM enable;
            PPH_EMENU_ITEM disable;
            PPH_EMENU_ITEM restart;
            PPH_EMENU_ITEM uninstall;
            PPH_EMENU_ITEM properties;
            BOOLEAN republish;
            BOOLEAN invalidate;

            node = (PDEVICE_NODE)contextMenuEvent->Node;

            menu = PhCreateEMenu();
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 100, L"Refresh", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, autoRefresh = PhCreateEMenuItem(0, 101, L"Refresh automatically", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
            PhInsertEMenuItem(menu, showDisconnectedDevices = PhCreateEMenuItem(0, 102, L"Show disconnected devices", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, showSoftwareDevices = PhCreateEMenuItem(0, 103, L"Show software components", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, highlightUpperFiltered = PhCreateEMenuItem(0, 104, L"Highlight upper filtered", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, highlightLowerFiltered = PhCreateEMenuItem(0, 105, L"Highlight lower filtered", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
            PhInsertEMenuItem(menu, gotoServiceItem = PhCreateEMenuItem(0, 106, L"Go to service...", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
            PhInsertEMenuItem(menu, enable = PhCreateEMenuItem(0, 0, L"Enable", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, disable = PhCreateEMenuItem(0, 1, L"Disable", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, restart = PhCreateEMenuItem(0, 2, L"Restart", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, uninstall = PhCreateEMenuItem(0, 3, L"Uninstall", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
            subMenu = PhCreateEMenuItem(0, 0, L"Open key", NULL, NULL);
            PhInsertEMenuItem(subMenu, PhCreateEMenuItem(0, HW_KEY_INDEX_HARDWARE, L"Hardware", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(subMenu, PhCreateEMenuItem(0, HW_KEY_INDEX_SOFTWARE, L"Software", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(subMenu, PhCreateEMenuItem(0, HW_KEY_INDEX_USER, L"User", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(subMenu, PhCreateEMenuItem(0, HW_KEY_INDEX_CONFIG, L"Config", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, subMenu, ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
            PhInsertEMenuItem(menu, properties = PhCreateEMenuItem(0, 10, L"Properties", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 11, L"Copy", NULL, NULL), ULONG_MAX);
            PhInsertCopyCellEMenuItem(menu, 11, DeviceTreeHandle, contextMenuEvent->Column);
            PhSetFlagsEMenuItem(menu, 10, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

            if (AutoRefreshDeviceTree)
                autoRefresh->Flags |= PH_EMENU_CHECKED;
            if (ShowDisconnected)
                showDisconnectedDevices->Flags |= PH_EMENU_CHECKED;
            if (ShowSoftwareComponents)
                showSoftwareDevices->Flags |= PH_EMENU_CHECKED;
            if (HighlightUpperFiltered)
                highlightUpperFiltered->Flags |= PH_EMENU_CHECKED;
            if (HighlightLowerFiltered)
                highlightLowerFiltered->Flags |= PH_EMENU_CHECKED;

            if (!node)
            {
                PhSetDisabledEMenuItem(gotoServiceItem);
                PhSetDisabledEMenuItem(subMenu);
                PhSetDisabledEMenuItem(properties);
            }

            if (gotoServiceItem && node)
            {
                PPH_STRING serviceName = GetDeviceNodeProperty(node, DevKeyService)->AsString;

                if (PhIsNullOrEmptyString(serviceName))
                {
                    PhSetDisabledEMenuItem(gotoServiceItem);
                }
            }

            if (!PhGetOwnTokenAttributes().Elevated)
            {
                enable->Flags |= PH_EMENU_DISABLED;
                disable->Flags |= PH_EMENU_DISABLED;
                restart->Flags |= PH_EMENU_DISABLED;
                uninstall->Flags |= PH_EMENU_DISABLED;
            }

            selectedItem = PhShowEMenu(
                menu,
                PhMainWndHandle,
                PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP,
                contextMenuEvent->Location.x,
                contextMenuEvent->Location.y
                );

            republish = FALSE;
            invalidate = FALSE;

            if (selectedItem && selectedItem->Id != ULONG_MAX)
            {
                if (!PhHandleCopyCellEMenuItem(selectedItem))
                {
                    switch (selectedItem->Id)
                    {
                    case 0:
                    case 1:
                        if (node->InstanceId)
                            republish = HardwareDeviceEnableDisable(hwnd, node->InstanceId, selectedItem->Id == 0);
                        break;
                    case 2:
                        if (node->InstanceId)
                            republish = HardwareDeviceRestart(hwnd, node->InstanceId);
                        break;
                    case 3:
                        if (node->InstanceId)
                            republish = HardwareDeviceUninstall(hwnd, node->InstanceId);
                        break;
                    case HW_KEY_INDEX_HARDWARE:
                    case HW_KEY_INDEX_SOFTWARE:
                    case HW_KEY_INDEX_USER:
                    case HW_KEY_INDEX_CONFIG:
                        if (node->InstanceId)
                            HardwareDeviceOpenKey(hwnd, node->InstanceId, selectedItem->Id);
                        break;
                    case 10:
                        if (node->InstanceId)
                            HardwareDeviceShowProperties(hwnd, node->InstanceId);
                        break;
                    case 11:
                        {
                            PPH_STRING text;

                            text = PhGetTreeNewText(DeviceTreeHandle, 0);
                            PhSetClipboardString(DeviceTreeHandle, &text->sr);
                            PhDereferenceObject(text);
                        }
                        break;
                    case 100:
                        republish = TRUE;
                        break;
                    case 101:
                        AutoRefreshDeviceTree = !AutoRefreshDeviceTree;
                        PhSetIntegerSetting(SETTING_NAME_DEVICE_TREE_AUTO_REFRESH, ShowDisconnected);
                        break;
                    case 102:
                        ShowDisconnected = !ShowDisconnected;
                        PhSetIntegerSetting(SETTING_NAME_DEVICE_TREE_SHOW_DISCONNECTED, ShowDisconnected);
                        invalidate = TRUE;
                        break;
                    case 103:
                        ShowSoftwareComponents = !ShowSoftwareComponents;
                        PhSetIntegerSetting(SETTING_NAME_DEVICE_SHOW_SOFTWARE_COMPONENTS, ShowSoftwareComponents);
                        republish = TRUE;
                        break;
                    case 104:
                        HighlightUpperFiltered = !HighlightUpperFiltered;
                        PhSetIntegerSetting(SETTING_NAME_DEVICE_TREE_HIGHLIGHT_UPPER_FILTERED, HighlightUpperFiltered);
                        invalidate = TRUE;
                        break;
                    case 105:
                        HighlightLowerFiltered = !HighlightLowerFiltered;
                        PhSetIntegerSetting(SETTING_NAME_DEVICE_TREE_HIGHLIGHT_LOWER_FILTERED, HighlightLowerFiltered);
                        invalidate = TRUE;
                        break;
                    case 106:
                        {
                            PPH_STRING serviceName = GetDeviceNodeProperty(node, DevKeyService)->AsString;
                            PPH_SERVICE_ITEM serviceItem;

                            if (!PhIsNullOrEmptyString(serviceName))
                            {
                                if (serviceItem = PhReferenceServiceItem(PhGetString(serviceName)))
                                {
                                    ProcessHacker_SelectTabPage(1);
                                    ProcessHacker_SelectServiceItem(serviceItem);
                                    PhDereferenceObject(serviceItem);
                                }
                            }
                        }
                        break;
                    }
                }
            }

            PhDestroyEMenu(menu);

            if (republish)
            {
                DeviceTreePublicAsync();
            }
            else if (invalidate)
            {
                InvalidateDeviceTree(DeviceTree);
                if (DeviceTreeFilterSupport.FilterList)
                    PhApplyTreeNewFilters(&DeviceTreeFilterSupport);
            }
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            PPH_TREENEW_MOUSE_EVENT mouseEvent = Parameter1;
            node = (PDEVICE_NODE)mouseEvent->Node;

            if (node->InstanceId)
                HardwareDeviceShowProperties(hwnd, node->InstanceId);
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = hwnd;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = 0;
            data.DefaultSortOrder = NoSortOrder;
            PhInitializeTreeNewColumnMenuEx(&data, PH_TN_COLUMN_MENU_SHOW_RESET_SORT);

            data.Selection = PhShowEMenu(
                data.Menu,
                hwnd,
                PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP,
                data.MouseEvent->ScreenLocation.x,
                data.MouseEvent->ScreenLocation.y
                );
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    }

    return FALSE;
}

VOID DevicesTreeLoadSettings(
    _In_ HWND TreeNewHandle
    )
{
    PPH_STRING settings;
    PH_INTEGER_PAIR sortSettings;

    settings = PhGetStringSetting(SETTING_NAME_DEVICE_TREE_COLUMNS);
    sortSettings = PhGetIntegerPairSetting(SETTING_NAME_DEVICE_TREE_SORT);
    PhCmLoadSettings(TreeNewHandle, &settings->sr);
    TreeNew_SetSort(TreeNewHandle, (ULONG)sortSettings.X, (ULONG)sortSettings.Y);
    PhDereferenceObject(settings);
}

VOID DevicesTreeSaveSettings(
    VOID
    )
{
    PPH_STRING settings;
    PH_INTEGER_PAIR sortSettings;
    ULONG sortColumn;
    ULONG sortOrder;

    if (!DeviceTreeCreated)
        return;

    settings = PhCmSaveSettings(DeviceTreeHandle);
    TreeNew_GetSort(DeviceTreeHandle, &sortColumn, &sortOrder);
    sortSettings.X = sortColumn;
    sortSettings.Y = sortOrder;
    PhSetStringSetting2(SETTING_NAME_DEVICE_TREE_COLUMNS, &settings->sr);
    PhSetIntegerPairSetting(SETTING_NAME_DEVICE_TREE_SORT, sortSettings);
    PhDereferenceObject(settings);
}

VOID DevicesTreeImageListInitialize(
    _In_ HWND TreeNewHandle
    )
{
    LONG dpi;

    dpi = PhGetWindowDpi(TreeNewHandle);
    DeviceIconSize.X = PhGetSystemMetrics(SM_CXSMICON, dpi);
    DeviceIconSize.Y = PhGetSystemMetrics(SM_CYSMICON, dpi);

    if (DeviceImageList)
    {
        PhImageListSetIconSize(DeviceImageList, DeviceIconSize.X, DeviceIconSize.Y);
    }
    else
    {
        DeviceImageList = PhImageListCreate(
            DeviceIconSize.X,
            DeviceIconSize.Y,
            ILC_MASK | ILC_COLOR32,
            200,
            100
            );
    }

    PhImageListAddIcon(DeviceImageList, PhGetApplicationIcon(TRUE));

    TreeNew_SetImageList(DeviceTreeHandle, DeviceImageList);
}

VOID DevicesTreeInitialize(
    _In_ HWND TreeNewHandle
    )
{
    CM_NOTIFY_FILTER cmFilter;

    DeviceTreeHandle = TreeNewHandle;

    PhSetControlTheme(DeviceTreeHandle, L"explorer");
    TreeNew_SetCallback(DeviceTreeHandle, DeviceTreeCallback, NULL);
    TreeNew_SetExtendedFlags(DeviceTreeHandle, TN_FLAG_ITEM_DRAG_SELECT, TN_FLAG_ITEM_DRAG_SELECT);
    SendMessage(TreeNew_GetTooltips(DeviceTreeHandle), TTM_SETDELAYTIME, TTDT_AUTOPOP, MAXSHORT);

    DevicesTreeImageListInitialize(DeviceTreeHandle);

    TreeNew_SetRedraw(DeviceTreeHandle, FALSE);

    for (ULONG i = 0; i < ARRAYSIZE(DevNodePropTable); i++)
    {
        ULONG displayIndex;
        PDEVNODE_PROP_TABLE_ENTRY entry;

        entry = &DevNodePropTable[i];

        assert(i == entry->NodeKey);

        if (entry->NodeKey == DevKeyName)
        {
            assert(i == 0);
            displayIndex = -2;
        }
        else
        {
            assert(i > 0);
            displayIndex = i - 1;
        }

        PhAddTreeNewColumn(
            DeviceTreeHandle,
            entry->NodeKey,
            entry->ColumnVisible,
            entry->ColumnName,
            entry->ColumnWidth,
            PH_ALIGN_LEFT,
            displayIndex,
            entry->ColumnTextFlags
            );
    }

    DevicesTreeLoadSettings(DeviceTreeHandle);

    TreeNew_SetRedraw(DeviceTreeHandle, TRUE);

    TreeNew_SetTriState(DeviceTreeHandle, TRUE);

    if (PhGetIntegerSetting(L"TreeListCustomRowSize"))
    {
        ULONG treelistCustomRowSize = PhGetIntegerSetting(L"TreeListCustomRowSize");

        if (treelistCustomRowSize < 15)
            treelistCustomRowSize = 15;

        TreeNew_SetRowHeight(DeviceTreeHandle, treelistCustomRowSize);
    }

    PhInitializeTreeNewFilterSupport(&DeviceTreeFilterSupport, DeviceTreeHandle, &DeviceFilterList);
    if (ToolStatusInterface)
    {
        PhRegisterCallback(
            ToolStatusInterface->SearchChangedEvent,
            DeviceTreeSearchChangedHandler,
            NULL,
            &SearchChangedRegistration);
        PhAddTreeNewFilter(&DeviceTreeFilterSupport, DeviceTreeFilterCallback, NULL);
    }

    if (PhGetIntegerSetting(L"EnableThemeSupport"))
    {
        PhInitializeWindowTheme(DeviceTreeHandle, TRUE);
        TreeNew_ThemeSupport(DeviceTreeHandle, TRUE);
    }

    RtlZeroMemory(&cmFilter, sizeof(CM_NOTIFY_FILTER));
    cmFilter.cbSize = sizeof(CM_NOTIFY_FILTER);

    cmFilter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINSTANCE;
    cmFilter.Flags = CM_NOTIFY_FILTER_FLAG_ALL_DEVICE_INSTANCES;
    if (CM_Register_Notification(
        &cmFilter,
        NULL,
        CmNotifyCallback,
        &DeviceNotification
        ) != CR_SUCCESS)
    {
        DeviceNotification = NULL;
    }

    cmFilter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
    cmFilter.Flags = CM_NOTIFY_FILTER_FLAG_ALL_INTERFACE_CLASSES;
    if (CM_Register_Notification(
        &cmFilter,
        NULL,
        CmNotifyCallback,
        &DeviceInterfaceNotification
        ) != CR_SUCCESS)
    {
        DeviceInterfaceNotification = NULL;
    }
}

VOID DevicesTreeDestroy(
    VOID
    )
{
    if (DeviceInterfaceNotification)
    {
        CM_Unregister_Notification(DeviceInterfaceNotification);
        DeviceInterfaceNotification = NULL;
    }

    if (DeviceInterfaceNotification)
    {
        CM_Unregister_Notification(DeviceInterfaceNotification);
        DeviceInterfaceNotification = NULL;
    }

    PhRemoveTreeNewFilter(&DeviceTreeFilterSupport, DeviceTreeFilterEntry);
    PhDeleteTreeNewFilterSupport(&DeviceTreeFilterSupport);

    if (DeviceTree)
    {
        PhDereferenceObject(DeviceTree);
        DeviceTree = NULL;
    }
}

BOOLEAN DevicesTabPageCallback(
    _In_ struct _PH_MAIN_TAB_PAGE* Page,
    _In_ PH_MAIN_TAB_PAGE_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    switch (Message)
    {
    case MainTabPageCreateWindow:
        {
            HWND hwnd;
            ULONG thinRows;
            ULONG treelistBorder;
            ULONG treelistCustomColors;
            PH_TREENEW_CREATEPARAMS treelistCreateParams = { 0 };

            thinRows = PhGetIntegerSetting(L"ThinRows") ? TN_STYLE_THIN_ROWS : 0;
            treelistBorder = (PhGetIntegerSetting(L"TreeListBorderEnable") && !PhGetIntegerSetting(L"EnableThemeSupport")) ? WS_BORDER : 0;
            treelistCustomColors = PhGetIntegerSetting(L"TreeListCustomColorsEnable") ? TN_STYLE_CUSTOM_COLORS : 0;

            if (treelistCustomColors)
            {
                treelistCreateParams.TextColor = PhGetIntegerSetting(L"TreeListCustomColorText");
                treelistCreateParams.FocusColor = PhGetIntegerSetting(L"TreeListCustomColorFocus");
                treelistCreateParams.SelectionColor = PhGetIntegerSetting(L"TreeListCustomColorSelection");
            }

            hwnd = CreateWindow(
                PH_TREENEW_CLASSNAME,
                NULL,
                WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TN_STYLE_ICONS | TN_STYLE_DOUBLE_BUFFERED | TN_STYLE_ANIMATE_DIVIDER | thinRows | treelistBorder | treelistCustomColors,
                0,
                0,
                3,
                3,
                PhMainWndHandle,
                NULL,
                PluginInstance->DllBase,
                &treelistCreateParams
                );

            if (!hwnd)
                return FALSE;

            DeviceTreeCreated = TRUE;

            DevicesTreeInitialize(hwnd);

            if (Parameter1)
            {
                *(HWND*)Parameter1 = hwnd;
            }
        }
        return TRUE;
    case MainTabPageLoadSettings:
        {
            NOTHING;
        }
        return TRUE;
    case MainTabPageSaveSettings:
        {
            DevicesTreeSaveSettings();
        }
        return TRUE;
    case MainTabPageSelected:
        {
            DeviceTabSelected = (BOOLEAN)Parameter1;
            if (DeviceTabSelected)
                DeviceTreePublicAsync();
        }
        break;
    case MainTabPageFontChanged:
        {
            HFONT font = (HFONT)Parameter1;

            if (DeviceTreeHandle)
                SendMessage(DeviceTreeHandle, WM_SETFONT, (WPARAM)Parameter1, TRUE);
        }
        break;
    case MainTabPageDpiChanged:
        {
            if (DeviceImageList)
            {
                DevicesTreeImageListInitialize(DeviceTreeHandle);

                if (DeviceTree && DeviceTree->DeviceList)
                {
                    for (ULONG i = 0; i < DeviceTree->DeviceList->Count; i++)
                    {
                        PDEVICE_NODE node = DeviceTree->DeviceList->Items[i];
                        HICON iconHandle;

                        if (SetupDiLoadDeviceIcon(
                            node->DeviceInfoHandle,
                            &node->DeviceInfoData,
                            DeviceIconSize.X,
                            DeviceIconSize.X,
                            0,
                            &iconHandle
                            ))
                        {
                            node->IconIndex = PhImageListAddIcon(DeviceImageList, iconHandle);
                            DestroyIcon(iconHandle);
                        }
                        else
                        {
                            node->IconIndex = 0; // Must be reset (dmex)
                        }
                    }
                }
            }
        }
        break;
    }

    return FALSE;
}

VOID NTAPI ToolStatusActivateContent(
    _In_ BOOLEAN Select
    )
{
    SetFocus(DeviceTreeHandle);

    if (Select)
    {
        if (TreeNew_GetFlatNodeCount(DeviceTreeHandle) > 0)
        {
            PDEVICE_NODE node;

            TreeNew_DeselectRange(DeviceTreeHandle, 0, -1);

            node = (PDEVICE_NODE)TreeNew_GetFlatNode(DeviceTreeHandle, 0);

            if (!node->Node.Visible)
            {
                TreeNew_SetFocusNode(DeviceTreeHandle, &node->Node);
                TreeNew_SetMarkNode(DeviceTreeHandle, &node->Node);
                TreeNew_SelectRange(DeviceTreeHandle, node->Node.Index, node->Node.Index);
                TreeNew_EnsureVisible(DeviceTreeHandle, &node->Node);
            }
        }
    }
}

HWND NTAPI ToolStatusGetTreeNewHandle(
    VOID
    )
{
    return DeviceTreeHandle;
}

VOID InitializeDevicesTab(
    VOID
    )
{
    PH_MAIN_TAB_PAGE page;
    PPH_PLUGIN toolStatusPlugin;

    DeviceNodeType = PhCreateObjectType(L"DeviceNode", 0, DeviceNodeDeleteProcedure);
    DeviceTreeType = PhCreateObjectType(L"DeviceTree", 0, DeviceTreeDeleteProcedure);
    AutoRefreshDeviceTree = !!PhGetIntegerSetting(SETTING_NAME_DEVICE_TREE_AUTO_REFRESH);
    ShowDisconnected = !!PhGetIntegerSetting(SETTING_NAME_DEVICE_TREE_SHOW_DISCONNECTED);
    ShowSoftwareComponents = !!PhGetIntegerSetting(SETTING_NAME_DEVICE_SHOW_SOFTWARE_COMPONENTS);
    HighlightUpperFiltered = !!PhGetIntegerSetting(SETTING_NAME_DEVICE_TREE_HIGHLIGHT_UPPER_FILTERED);
    HighlightLowerFiltered = !!PhGetIntegerSetting(SETTING_NAME_DEVICE_TREE_HIGHLIGHT_LOWER_FILTERED);
    DeviceProblemColor = PhGetIntegerSetting(SETTING_NAME_DEVICE_PROBLEM_COLOR);
    DeviceDisabledColor = PhGetIntegerSetting(SETTING_NAME_DEVICE_DISABLED_COLOR);
    DeviceDisconnectedColor = PhGetIntegerSetting(SETTING_NAME_DEVICE_DISCONNECTED_COLOR);
    DeviceHighlightColor = PhGetIntegerSetting(SETTING_NAME_DEVICE_HIGHLIGHT_COLOR);

    RtlZeroMemory(&page, sizeof(PH_MAIN_TAB_PAGE));
    PhInitializeStringRef(&page.Name, L"Devices");
    page.Callback = DevicesTabPageCallback;

    DevicesAddedTabPage = PhPluginCreateTabPage(&page);

    if (toolStatusPlugin = PhFindPlugin(TOOLSTATUS_PLUGIN_NAME))
    {
        ToolStatusInterface = PhGetPluginInformation(toolStatusPlugin)->Interface;

        if (ToolStatusInterface->Version <= TOOLSTATUS_INTERFACE_VERSION)
        {
            PTOOLSTATUS_TAB_INFO tabInfo;

            tabInfo = ToolStatusInterface->RegisterTabInfo(DevicesAddedTabPage->Index);
            tabInfo->BannerText = L"Search Devices";
            tabInfo->ActivateContent = ToolStatusActivateContent;
            tabInfo->GetTreeNewHandle = ToolStatusGetTreeNewHandle;
        }
    }
}
