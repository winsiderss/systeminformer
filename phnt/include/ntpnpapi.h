/*
 * Plug and Play support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTPNPAPI_H
#define _NTPNPAPI_H

typedef enum _PLUGPLAY_EVENT_CATEGORY
{
    HardwareProfileChangeEvent,
    TargetDeviceChangeEvent,
    DeviceClassChangeEvent,
    CustomDeviceEvent,
    DeviceInstallEvent,
    DeviceArrivalEvent,
    PowerEvent,
    VetoEvent,
    BlockedDriverEvent,
    InvalidIDEvent,
    MaxPlugEventCategory
} PLUGPLAY_EVENT_CATEGORY, *PPLUGPLAY_EVENT_CATEGORY;

typedef struct _PLUGPLAY_EVENT_BLOCK
{
    GUID EventGuid;
    PLUGPLAY_EVENT_CATEGORY EventCategory;
    PULONG Result;
    ULONG Flags;
    ULONG TotalSize;
    PVOID DeviceObject;

    union
    {
        struct
        {
            GUID ClassGuid;
            WCHAR SymbolicLinkName[1];
        } DeviceClass;
        struct
        {
            WCHAR DeviceIds[1];
        } TargetDevice;
        struct
        {
            WCHAR DeviceId[1];
        } InstallDevice;
        struct
        {
            PVOID NotificationStructure;
            WCHAR DeviceIds[1];
        } CustomNotification;
        struct
        {
            PVOID Notification;
        } ProfileNotification;
        struct
        {
            ULONG NotificationCode;
            ULONG NotificationData;
        } PowerNotification;
        struct
        {
            PNP_VETO_TYPE VetoType;
            WCHAR DeviceIdVetoNameBuffer[1]; // DeviceId<null>VetoName<null><null>
        } VetoNotification;
        struct
        {
            GUID BlockedDriverGuid;
        } BlockedDriverNotification;
        struct
        {
            WCHAR ParentId[1];
        } InvalidIDNotification;
    } u;
} PLUGPLAY_EVENT_BLOCK, *PPLUGPLAY_EVENT_BLOCK;

typedef enum _PLUGPLAY_CONTROL_CLASS
{
    PlugPlayControlEnumerateDevice, // PLUGPLAY_CONTROL_ENUMERATE_DEVICE_DATA
    PlugPlayControlRegisterNewDevice, // PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA
    PlugPlayControlDeregisterDevice, // PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA
    PlugPlayControlInitializeDevice, // PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA
    PlugPlayControlStartDevice, // PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA
    PlugPlayControlUnlockDevice, // PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA
    PlugPlayControlQueryAndRemoveDevice, // PLUGPLAY_CONTROL_QUERY_AND_REMOVE_DATA
    PlugPlayControlUserResponse, // PLUGPLAY_CONTROL_USER_RESPONSE_DATA
    PlugPlayControlGenerateLegacyDevice, // PLUGPLAY_CONTROL_LEGACY_DEVGEN_DATA
    PlugPlayControlGetInterfaceDeviceList, // PLUGPLAY_CONTROL_INTERFACE_LIST_DATA
    PlugPlayControlProperty, // PLUGPLAY_CONTROL_PROPERTY_DATA
    PlugPlayControlDeviceClassAssociation, // PLUGPLAY_CONTROL_CLASS_ASSOCIATION_DATA
    PlugPlayControlGetRelatedDevice, // PLUGPLAY_CONTROL_RELATED_DEVICE_DATA
    PlugPlayControlGetInterfaceDeviceAlias, // PLUGPLAY_CONTROL_INTERFACE_ALIAS_DATA
    PlugPlayControlDeviceStatus, // PLUGPLAY_CONTROL_STATUS_DATA
    PlugPlayControlGetDeviceDepth, // PLUGPLAY_CONTROL_DEPTH_DATA
    PlugPlayControlQueryDeviceRelations, // PLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA
    PlugPlayControlTargetDeviceRelation, // PLUGPLAY_CONTROL_TARGET_RELATION_DATA
    PlugPlayControlQueryConflictList, // PLUGPLAY_CONTROL_CONFLICT_LIST
    PlugPlayControlRetrieveDock, // PLUGPLAY_CONTROL_RETRIEVE_DOCK_DATA
    PlugPlayControlResetDevice, // PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA
    PlugPlayControlHaltDevice, // PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA
    PlugPlayControlGetBlockedDriverList, // PLUGPLAY_CONTROL_BLOCKED_DRIVER_DATA
    PlugPlayControlGetDeviceInterfaceEnabled, // PLUGPLAY_CONTROL_DEVICE_INTERFACE_ENABLED
    MaxPlugPlayControl
} PLUGPLAY_CONTROL_CLASS, *PPLUGPLAY_CONTROL_CLASS;

// pub
typedef enum _DEVICE_RELATION_TYPE
{
    BusRelations,
    EjectionRelations,
    PowerRelations,
    RemovalRelations,
    TargetDeviceRelation,
    SingleBusRelations,
    TransportRelations
} DEVICE_RELATION_TYPE, *PDEVICE_RELATION_TYPE;

// pub
typedef enum _BUS_QUERY_ID_TYPE
{
    BusQueryDeviceID = 0,           // <Enumerator>\<Enumerator-specific device id>
    BusQueryHardwareIDs = 1,        // Hardware ids
    BusQueryCompatibleIDs = 2,      // compatible device ids
    BusQueryInstanceID = 3,         // persistent id for this instance of the device
    BusQueryDeviceSerialNumber = 4, // serial number for this device
    BusQueryContainerID = 5         // unique id of the device's physical container
} BUS_QUERY_ID_TYPE, *PBUS_QUERY_ID_TYPE;

// pub
typedef enum _DEVICE_TEXT_TYPE
{
    DeviceTextDescription = 0,        // DeviceDesc property
    DeviceTextLocationInformation = 1 // DeviceLocation property
} DEVICE_TEXT_TYPE, *PDEVICE_TEXT_TYPE;

// pub
typedef enum _DEVICE_USAGE_NOTIFICATION_TYPE
{
    DeviceUsageTypeUndefined,
    DeviceUsageTypePaging,
    DeviceUsageTypeHibernation,
    DeviceUsageTypeDumpFile,
    DeviceUsageTypeBoot,
    DeviceUsageTypePostDisplay,
    DeviceUsageTypeGuestAssigned
} DEVICE_USAGE_NOTIFICATION_TYPE, *PDEVICE_USAGE_NOTIFICATION_TYPE;

#if (PHNT_VERSION < PHNT_WIN8)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetPlugPlayEvent(
    _In_ HANDLE EventHandle,
    _In_opt_ PVOID Context,
    _Out_writes_bytes_(EventBufferSize) PPLUGPLAY_EVENT_BLOCK EventBlock,
    _In_ ULONG EventBufferSize
    );
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPlugPlayControl(
    _In_ PLUGPLAY_CONTROL_CLASS PnPControlClass,
    _Inout_updates_bytes_(PnPControlDataLength) PVOID PnPControlData,
    _In_ ULONG PnPControlDataLength
    );

#if (PHNT_VERSION >= PHNT_WIN7)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSerializeBoot(
    VOID
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtEnableLastKnownGood(
    VOID
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDisableLastKnownGood(
    VOID
    );

#endif

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplacePartitionUnit(
    _In_ PUNICODE_STRING TargetInstancePath,
    _In_ PUNICODE_STRING SpareInstancePath,
    _In_ ULONG Flags
    );
#endif

#endif
