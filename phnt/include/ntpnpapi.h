/*
 * Plug and Play support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTPNPAPI_H
#define _NTPNPAPI_H

#include <cfg.h>

typedef enum _PLUGPLAY_EVENT_CATEGORY
{
    HardwareProfileChangeEvent,
    TargetDeviceChangeEvent,
    DeviceClassChangeEvent,
    CustomDeviceEvent,
    DeviceInstallEvent,
    DeviceArrivalEvent,
    VetoEvent,
    BlockedDriverEvent,
    InvalidIDEvent,
    DevicePropertyChangeEvent,
    DeviceInstanceRemovalEvent,
    DeviceInstanceStartedEvent,
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
#if (PHNT_VERSION >= PHNT_WINDOWS_11_24H2)
        // symbols: retained in the current union, even though PowerEvent is no longer a category.
        struct
        {
            GUID PowerSettingGuid;
            ULONG Flags;
            ULONG SessionId;
            ULONG DataLength;
            UCHAR Data[1];
        } PowerSettingNotification;
        struct
        {
            WCHAR DeviceId[1];
        } PropertyChangeNotification;
        struct
        {
            WCHAR DeviceId[1];
        } DeviceInstanceNotification;
#endif
    } u;
} PLUGPLAY_EVENT_BLOCK, *PPLUGPLAY_EVENT_BLOCK;

typedef enum _PLUGPLAY_CONTROL_CLASS
{
    PlugPlayControlEnumerateDevice              = 0, // PLUGPLAY_CONTROL_ENUMERATE_DEVICE_DATA
    PlugPlayControlRegisterNewDevice            = 1, // PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA
    PlugPlayControlDeregisterDevice             = 2, // PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA
    PlugPlayControlInitializeDevice             = 3, // PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA
    PlugPlayControlStartDevice                  = 4, // PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA
    PlugPlayControlUnlockDevice                 = 5, // PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA
    PlugPlayControlQueryAndRemoveDevice         = 6, // PLUGPLAY_CONTROL_QUERY_AND_REMOVE_DATA
    PlugPlayControlUserResponse                 = 7, // PLUGPLAY_CONTROL_USER_RESPONSE_DATA
    PlugPlayControlGenerateLegacyDevice         = 8, // PLUGPLAY_CONTROL_LEGACY_DEVGEN_DATA
    PlugPlayControlGetInterfaceDeviceList       = 9, // PLUGPLAY_CONTROL_INTERFACE_LIST_DATA
    PlugPlayControlProperty                     = 10, // PLUGPLAY_CONTROL_PROPERTY_DATA
    PlugPlayControlDeviceClassAssociation       = 11, // PLUGPLAY_CONTROL_CLASS_ASSOCIATION_DATA
    PlugPlayControlGetRelatedDevice             = 12, // PLUGPLAY_CONTROL_RELATED_DEVICE_DATA
    PlugPlayControlGetInterfaceDeviceAlias      = 13, // PLUGPLAY_CONTROL_INTERFACE_ALIAS_DATA
    PlugPlayControlDeviceStatus                 = 14, // PLUGPLAY_CONTROL_STATUS_DATA
    PlugPlayControlGetDeviceDepth               = 15, // PLUGPLAY_CONTROL_DEPTH_DATA
    PlugPlayControlQueryDeviceRelations         = 16, // PLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA
    PlugPlayControlTargetDeviceRelation         = 17, // PLUGPLAY_CONTROL_TARGET_RELATION_DATA
    PlugPlayControlQueryConflictList            = 18, // PLUGPLAY_CONTROL_CONFLICT_LIST
    PlugPlayControlRetrieveDock                 = 19, // PLUGPLAY_CONTROL_RETRIEVE_DOCK_DATA
    PlugPlayControlResetDevice                  = 20, // PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA
    PlugPlayControlHaltDevice                   = 21, // PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA
    PlugPlayControlGetBlockedDriverList         = 22, // PLUGPLAY_CONTROL_BLOCKED_DRIVER_DATA
    PlugPlayControlGetDeviceInterfaceEnabled    = 23, // PLUGPLAY_CONTROL_DEVICE_INTERFACE_ENABLED
    MaxPlugPlayControl = 24
} PLUGPLAY_CONTROL_CLASS, *PPLUGPLAY_CONTROL_CLASS;

// rev
// Payloads for the six non-NULL entries in PlugPlayHandlerTable on
// ntoskrnl 10.0.26100.9457 (AMD64). The remaining control classes are retained
// for compatibility, but return STATUS_NOT_IMPLEMENTED on this build.
// DeviceInstance.Length must be even and in the range 2..400 bytes; its
// MaximumLength is not used by these handlers. Device interface names use
// the separate limit documented with PLUGPLAY_CONTROL_DEVICE_INTERFACE_ENABLED.

// rev: selectors for PlugPlayControlProperty, not DEVICE_REGISTRY_PROPERTY or CM_DRP_*.
// Values 0, 9 and 12 are not accepted by the audited handler.
typedef enum _PLUGPLAY_CONTROL_PROPERTY_TYPE
{
    PlugPlayPropertyPhysicalDeviceObjectName        = 1, // NUL-terminated WCHAR string
    PlugPlayPropertyBusTypeGuid                     = 2, // GUID (16 bytes)
    PlugPlayPropertyLegacyBusType                   = 3, // INTERFACE_TYPE (4 bytes)
    PlugPlayPropertyBusNumber                       = 4, // ULONG (4 bytes)
    PlugPlayPropertyPowerData                       = 5, // CM_POWER_DATA (56 bytes)
    PlugPlayPropertyRemovalPolicy                   = 6, // ULONG (4 bytes)
    PlugPlayPropertyRemovalPolicyOverride           = 7, // ULONG (4 bytes)
    PlugPlayPropertyAddress                         = 8, // ULONG (4 bytes)
    PlugPlayPropertyRemovalPolicyHwDefault          = 10, // ULONG (4 bytes)
    PlugPlayPropertyInstallState                    = 11, // ULONG (4 bytes)
    PlugPlayPropertyDeviceIdsHash                   = 13, // ULONG (4 bytes)
    PlugPlayPropertyDeviceStack                     = 14, // MULTI_SZ of driver object names
    PlugPlayPropertyDependencyProviders             = 15, // MULTI_SZ of device instance paths
    PlugPlayPropertyDependencyDependents            = 16 // MULTI_SZ of device instance paths
} PLUGPLAY_CONTROL_PROPERTY_TYPE, *PPLUGPLAY_CONTROL_PROPERTY_TYPE;

// rev (sizeof = 0x28 on AMD64)
typedef struct _PLUGPLAY_CONTROL_PROPERTY_DATA
{
    UNICODE_STRING DeviceInstance;
    ULONG Property; // PLUGPLAY_CONTROL_PROPERTY_TYPE
    PVOID Buffer;
    ULONG BufferSize; // in: capacity in bytes; out: required size in bytes
} PLUGPLAY_CONTROL_PROPERTY_DATA, *PPLUGPLAY_CONTROL_PROPERTY_DATA;

// rev
typedef enum _PLUGPLAY_CONTROL_RELATED_DEVICE_TYPE
{
    PlugPlayRelatedDeviceParent = 1,
    PlugPlayRelatedDeviceChild = 2,
    PlugPlayRelatedDeviceSibling = 3
} PLUGPLAY_CONTROL_RELATED_DEVICE_TYPE, *PPLUGPLAY_CONTROL_RELATED_DEVICE_TYPE;

// rev (sizeof = 0x28 on AMD64)
typedef struct _PLUGPLAY_CONTROL_RELATED_DEVICE_DATA
{
    UNICODE_STRING DeviceInstance;
    ULONG Relation; // PLUGPLAY_CONTROL_RELATED_DEVICE_TYPE
    PWSTR RelatedDeviceInstance;
    // In WCHARs: input capacity includes the terminator; success returns the
    // string length excluding the terminator; STATUS_BUFFER_TOO_SMALL returns
    // the required capacity including the terminator.
    ULONG RelatedDeviceInstanceLength;
} PLUGPLAY_CONTROL_RELATED_DEVICE_DATA, *PPLUGPLAY_CONTROL_RELATED_DEVICE_DATA;

// rev
typedef enum _PLUGPLAY_CONTROL_STATUS_OPERATION
{
    PlugPlayGetDeviceStatus = 0,
    PlugPlaySetDeviceStatus = 1,
    PlugPlayClearDeviceProblem = 2
} PLUGPLAY_CONTROL_STATUS_OPERATION, *PPLUGPLAY_CONTROL_STATUS_OPERATION;

// rev: queue PlugPlaySetDeviceStatus without waiting for completion.
#define PLUGPLAY_CONTROL_STATUS_ASYNC 0x00000001

// symbols (sizeof = 0x28 on AMD64; current layout including Flags and ProblemStatus)
typedef struct _PLUGPLAY_CONTROL_STATUS_DATA
{
    UNICODE_STRING DeviceInstance;
    ULONG Operation; // PLUGPLAY_CONTROL_STATUS_OPERATION
    ULONG DeviceStatus; // DN_* flags
    ULONG DeviceProblem; // CM_PROB_* code
    ULONG Flags; // PLUGPLAY_CONTROL_STATUS_ASYNC
    NTSTATUS ProblemStatus;
} PLUGPLAY_CONTROL_STATUS_DATA, *PPLUGPLAY_CONTROL_STATUS_DATA;

// rev (sizeof = 0x18 on AMD64)
typedef struct _PLUGPLAY_CONTROL_DEPTH_DATA
{
    UNICODE_STRING DeviceInstance;
    ULONG Depth;
} PLUGPLAY_CONTROL_DEPTH_DATA, *PPLUGPLAY_CONTROL_DEPTH_DATA;

// rev: these values are translated to DEVICE_RELATION_TYPE by PiQueryDeviceRelations.
// They are NOT the values of DEVICE_RELATION_TYPE.
typedef enum _PLUGPLAY_CONTROL_DEVICE_RELATION_TYPE
{
    PlugPlayDeviceEjectionRelations = 0, // EjectionRelations (1)
    PlugPlayDeviceRemovalRelations = 1, // RemovalRelations (3)
    PlugPlayDevicePowerRelations = 2, // PowerRelations (2)
    PlugPlayDeviceBusRelations = 3, // BusRelations (0)
    PlugPlayDeviceTransportRelations = 4 // TransportRelations (6)
} PLUGPLAY_CONTROL_DEVICE_RELATION_TYPE, *PPLUGPLAY_CONTROL_DEVICE_RELATION_TYPE;

// rev (sizeof = 0x20 on AMD64)
typedef struct _PLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA
{
    UNICODE_STRING DeviceInstance;
    ULONG Relations; // PLUGPLAY_CONTROL_DEVICE_RELATION_TYPE
    ULONG BufferSize; // in: capacity in WCHARs; out: required WCHARs, including MULTI_SZ terminators
    PWSTR Buffer; // MULTI_SZ of related device instance paths
} PLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA, *PPLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA;

// rev (sizeof = 0x18 on AMD64)
typedef struct _PLUGPLAY_CONTROL_DEVICE_INTERFACE_ENABLED
{
    UNICODE_STRING SymbolicLinkName; // Length must be even and in the range 2..1008 bytes
    ULONG Flags; // Must be zero
    BOOLEAN Enabled; // out (one byte, not BOOL)
} PLUGPLAY_CONTROL_DEVICE_INTERFACE_ENABLED, *PPLUGPLAY_CONTROL_DEVICE_INTERFACE_ENABLED;

// private
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

// private
typedef enum _BUS_QUERY_ID_TYPE
{
    BusQueryDeviceID = 0,           // <Enumerator>\<Enumerator-specific device id>
    BusQueryHardwareIDs = 1,        // Hardware ids
    BusQueryCompatibleIDs = 2,      // compatible device ids
    BusQueryInstanceID = 3,         // persistent id for this instance of the device
    BusQueryDeviceSerialNumber = 4, // serial number for this device
    BusQueryContainerID = 5         // unique id of the device's physical container
} BUS_QUERY_ID_TYPE, *PBUS_QUERY_ID_TYPE;

// private
typedef enum _DEVICE_TEXT_TYPE
{
    DeviceTextDescription = 0,        // DeviceDesc property
    DeviceTextLocationInformation = 1 // DeviceLocation property
} DEVICE_TEXT_TYPE, *PDEVICE_TEXT_TYPE;

// private
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

#if (PHNT_VERSION < PHNT_WINDOWS_8)
/**
 * The NtGetPlugPlayEvent routine retrieves a Plug and Play event notification from the system event queue.
 *
 * \param EventHandle A handle to the synchronization event signaled when a PnP event is queued.
 * \param Context An optional context pointer passed to the routine.
 * \param EventBlock A pointer to a buffer that receives the PnP event block.
 * \param EventBufferSize The size, in bytes, of the buffer pointed to by EventBlock.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetPlugPlayEvent(
    _In_ HANDLE EventHandle,
    _In_opt_ PVOID Context,
    _Out_writes_bytes_(EventBufferSize) PPLUGPLAY_EVENT_BLOCK EventBlock,
    _In_ ULONG EventBufferSize
    );
#endif // (PHNT_VERSION < PHNT_WINDOWS_8)

/**
 * The NtPlugPlayControl routine performs a Plug and Play control operation.
 *
 * \param PnPControlClass The control class specifying the operation to perform.
 * \param PnPControlData A pointer to the control data buffer whose layout depends on PnPControlClass.
 * \param PnPControlDataLength The size, in bytes, of the control data structure (not its pointed-to buffers).
 * The length must exactly match the selected class; a mismatch returns STATUS_INVALID_PARAMETER_MIX.
 * \remarks User-mode callers require SeTcbPrivilege. The control buffer must be 4-byte aligned;
 * pointed-to WCHAR buffers must be 2-byte aligned. On ntoskrnl 10.0.26100.9457, only Property,
 * GetRelatedDevice, DeviceStatus, GetDeviceDepth, QueryDeviceRelations and GetDeviceInterfaceEnabled
 * are implemented. GetDeviceDepth and QueryDeviceRelations are not permitted in a server silo.
 * STATUS_BUFFER_TOO_SMALL still copies the control structure back, allowing size-query calls.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPlugPlayControl(
    _In_ PLUGPLAY_CONTROL_CLASS PnPControlClass,
    _Inout_updates_bytes_(PnPControlDataLength) PVOID PnPControlData,
    _In_ ULONG PnPControlDataLength
    );

/**
 * The NtSerializeBoot routine serializes the boot sequence with respect to Plug and Play device enumeration.
 *
 * \remarks Requires a user-mode caller with SeTcbPrivilege.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSerializeBoot(
    VOID
    );

/**
 * The NtEnableLastKnownGood routine enables the Last Known Good (LKG) configuration boot option.
 *
 * \remarks Requires a user-mode caller with SeTcbPrivilege outside a server silo.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtEnableLastKnownGood(
    VOID
    );

/**
 * The NtDisableLastKnownGood routine disables the Last Known Good (LKG) configuration boot option.
 *
 * \remarks Requires a user-mode caller with SeTcbPrivilege outside a server silo.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDisableLastKnownGood(
    VOID
    );

/**
 * The NtReplacePartitionUnit routine replaces a hardware partition unit with a spare partition unit.
 *
 * \param TargetInstancePath A pointer to the path of the target partition unit to be replaced.
 * \param SpareInstancePath A pointer to the path of the spare partition unit to use as replacement.
 * \param Flags Zero to replace the target with the spare, or 0x80000000 to test device quiesce/wake.
 * In test mode, both instance-path pointers are ignored and may be NULL. Other values return
 * STATUS_INVALID_PARAMETER_3.
 * \remarks Requires a user-mode caller with SeShutdownPrivilege. Test mode is not a read-only query;
 * it quiesces and wakes devices.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplacePartitionUnit(
    _When_(Flags == 0, _In_) _When_(Flags == 0x80000000, _In_opt_) PCUNICODE_STRING TargetInstancePath,
    _When_(Flags == 0, _In_) _When_(Flags == 0x80000000, _In_opt_) PCUNICODE_STRING SpareInstancePath,
    _In_ ULONG Flags
    );

#endif // _NTPNPAPI_H
