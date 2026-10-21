/*
 * Power Management support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTPOAPI_H
#define _NTPOAPI_H

#if (PHNT_MODE != PHNT_MODE_KERNEL)
// POWER_INFORMATION_LEVEL
// Note: We don't use an enum for these values to minimize conflicts with the Windows SDK. (dmex)
#define POWER_INFORMATION_LEVEL ULONG
#define SystemPowerPolicyAc 0                           // in: SYSTEM_POWER_POLICY, out: SYSTEM_POWER_POLICY // GET: InputBuffer == NULL, SET: InputBuffer != NULL // SET path calls PopApplyPolicy and returns the current policy
#define SystemPowerPolicyDc 1                           // in: SYSTEM_POWER_POLICY, out: SYSTEM_POWER_POLICY // GET: InputBuffer == NULL, SET: InputBuffer != NULL // SET path calls PopApplyPolicy and returns the current policy
#define VerifySystemPolicyAc 2                          // in: SYSTEM_POWER_POLICY, out: SYSTEM_POWER_POLICY // Requires both input and output buffers // PopVerifySystemPowerPolicy
#define VerifySystemPolicyDc 3                          // in: SYSTEM_POWER_POLICY, out: SYSTEM_POWER_POLICY // Requires both input and output buffers // PopVerifySystemPowerPolicy
#define SystemPowerCapabilities 4                       // out: SYSTEM_POWER_CAPABILITIES
#define SystemBatteryState 5                            // out: SYSTEM_BATTERY_STATE
#define SystemPowerStateHandler 6                       // in: POWER_STATE_HANDLER // (kernel-mode only)
#define ProcessorStateHandler 7                         // in: PROCESSOR_STATE_HANDLER // (kernel-mode only)
#define SystemPowerPolicyCurrent 8                      // out: SYSTEM_POWER_POLICY
#define AdministratorPowerPolicy 9                      // in: SYSTEM_POWER_POLICY // (requires SeShutdownPrivilege)
#define SystemReserveHiberFile 10                       // in: BOOLEAN // (requires SeCreatePagefilePrivilege) // TRUE: hibernation file created. FALSE: hibernation file deleted.
#define ProcessorInformation 11                         // out: PROCESSOR_POWER_INFORMATION
#define SystemPowerInformation 12                       // out: SYSTEM_POWER_INFORMATION
#define ProcessorStateHandler2 13                       // in: PROCESSOR_STATE_HANDLER2 // not implemented
#define LastWakeTime 14                                 // out: ULONGLONG // InterruptTime
#define LastSleepTime 15                                // out: ULONGLONG // InterruptTime
#define SystemExecutionState 16                         // out: EXECUTION_STATE // NtSetThreadExecutionState
#define SystemPowerStateNotifyHandler 17                // in: POWER_STATE_NOTIFY_HANDLER
#define ProcessorPowerPolicyAc 18                       // in: PROCESSOR_POWER_POLICY // not implemented
#define ProcessorPowerPolicyDc 19                       // in: PROCESSOR_POWER_POLICY // not implemented
#define VerifyProcessorPowerPolicyAc 20                 // in: PROCESSOR_POWER_POLICY // not implemented
#define VerifyProcessorPowerPolicyDc 21                 // in: PROCESSOR_POWER_POLICY // not implemented
#define ProcessorPowerPolicyCurrent 22                  // in: PROCESSOR_POWER_POLICY // not implemented
#define SystemPowerStateLogging 23                      // out: variable-length power logging information
#define SystemPowerLoggingEntry 24                      // in: SYSTEM_POWER_LOGGING_ENTRY
#define SetPowerSettingValue 25                         // in: SET_POWER_SETTING_VALUE_INPUT
#define NotifyUserPowerSetting 26                       // in: not implemented // NOTIFY_USER_POWER_SETTING
#define PowerInformationLevelUnused0 27                 // in: not implemented
#define SystemMonitorHiberBootPowerOff 28               // in: NULL (PowerMonitorOff)
#define SystemVideoState 29                             // out: MONITOR_DISPLAY_STATE
#define TraceApplicationPowerMessage 30                 // in: SYSTEM_POWER_TRACE_APPLICATION_POWER_MESSAGE // POP_ETW_EVENT_SUSPENDAPP // SeAuditProcessCreationInfo.ImageFileName
#define TraceApplicationPowerMessageEnd 31              // in: SYSTEM_POWER_TRACE_APPLICATION_POWER_MESSAGE_END // POP_ETW_EVENT_SUSPENDAPP
#define ProcessorPerfStates 32                          // out: not implemented
#define ProcessorIdleStates 33                          // out: not implemented
#define ProcessorCap 34                                 // out: not implemented
#define SystemWakeSource 35                             // out: POWER_WAKE_SOURCE_INFO
#define SystemHiberFileInformation 36                   // out: SYSTEM_HIBERFILE_INFORMATION
#define TraceServicePowerMessage 37                     // in: SYSTEM_SERVICE_POWER_MESSAGE // (kernel-mode only)
#define ProcessorLoad 38                                // in: PROCESSOR_LOAD (sets), in: PPROCESSOR_NUMBER (clears)
#define PowerShutdownNotification 39                    // in: POWER_SHUTDOWN_NOTIFICATION
#define MonitorCapabilities 40                          // in: POWER_MONITOR_CAPABILITIES
#define SessionPowerInit 41                             // out: POWER_SESSION_POWER_INIT
#define SessionDisplayState 42                          // in: POWER_SESSION_DISPLAY_STATE
#define PowerRequestCreate 43                           // in: COUNTED_REASON_CONTEXT, out: HANDLE
#define PowerRequestAction 44                           // in: POWER_REQUEST_ACTION
#define GetPowerRequestList 45                          // out: POWER_REQUEST_LIST
#define ProcessorInformationEx 46                       // in: USHORT ProcessorGroup, out: PROCESSOR_POWER_INFORMATION
#define NotifyUserModeLegacyPowerEvent 47               // in: SYSTEM_NOTIFY_USER_MODE_LEGACY_POWER_EVENT
#define GroupPark 48                                    // in: PROCESSOR_GROUP_SELECTOR (clear), PROCESSOR_GROUP_PARK_MASK (set), PROCESSOR_GROUP_PARK_MASK_EX (set) // (KdDebuggerEnabled only)
#define ProcessorIdleDomains 49                         // not implemented
#define WakeTimerList 50                                // out: WAKE_TIMER_INFO[]
#define SystemHiberFileSize 51                          // out: ULONG
#define ProcessorIdleStatesHv 52                        // in: not implemented
#define ProcessorPerfStatesHv 53                        // in: not implemented
#define ProcessorPerfCapHv 54                           // in: not implemented
#define ProcessorSetIdle 55                             // in: PROCESSOR_SET_IDLE_STATE (sets), in: PROCESSOR_NUMBER (clears) // (KdDebuggerEnabled only)
#define LogicalProcessorIdling 56                       // in: LOGICAL_PROCESSOR_IDLING_INPUT, out: LOGICAL_PROCESSOR_IDLING_OUTPUT
#define UserPresence 57                                 // out: POWER_USER_PRESENCE // not implemented
#define PowerSettingNotificationName 58                 // in: POWER_SETTING_NOTIFICATION_NAME_INPUT (optional), out: WNF_STATE_NAME // (RtlSubscribeWnfStateChangeNotification)
#define GetPowerSettingValue 59                         // in: GET_POWER_SETTING_VALUE_INPUT, out: GET_POWER_SETTING_VALUE_OUTPUT // packed GET_POWER_SETTING_VALUE_ENTRY[]
#define IdleResiliency 60                               // in: POWER_IDLE_RESILIENCY
#define SessionRITState 61                              // in: 0x10-byte input, out: 0x8-byte output // not supported
#define SessionConnectNotification 62                   // in: POWER_SESSION_WINLOGON
#define SessionPowerCleanup 63                          // in: NULL, out: NULL // calls PopFreeSessionState(SessionId)
#define SessionLockState 64                             // in: POWER_SESSION_WINLOGON
#define SystemHiberbootState 65                         // out: BOOLEAN // effective HiberbootEnabled state
#define PlatformInformation 66                          // out: BOOLEAN // platform AoAc support
#define PdcInvocation 67                                // in: PDC_INVOCATION, out: PDC_INVOCATION_CALLBACKS (optional) // op 0 register, op 1 invoke
#define MonitorInvocation 68                            // in: MONITOR_INVOCATION_INPUT
#define FirmwareTableInformationRegistered 69           // in: NULL, out: NULL // PopInitPlatformSettings: reads the ACPI FADT ('ACPI'/'FACP') to (re)initialize the platform role (Preferred_PM_Profile) and Modern Standby/AoAc capability (Low-Power S0 Idle); applies role/AoAc overrides
#define SetShutdownSelectedTime 70                      // in: NULL, out: NULL
#define SuspendResumeInvocation 71                      // in: not supported
#define PlmPowerRequestCreate 72                        // in: COUNTED_REASON_CONTEXT, out: HANDLE
#define ScreenOff 73                                    // in: NULL (PowerMonitorOff)
#define CsDeviceNotification 74                         // in: POWER_CS_DEVICE_NOTIFICATION // (kernel-mode only)
#define PlatformRole 75                                 // out: POWER_PLATFORM_ROLE
#define LastResumePerformance 76                        // out: RESUME_PERFORMANCE
#define DisplayBurst 77                                 // in: NULL (PowerMonitorOn)
#define ExitLatencySamplingPercentage 78                // in: NULL (ClearExitLatencySamplingPercentage), in: ULONG (SetExitLatencySamplingPercentage) (max 100)
#define RegisterSpmPowerSettings 79                     // in: POWER_CS_DEVICE_NOTIFICATION // (kernel-mode only)
#define PlatformIdleStates 80                           // in: (kernel-mode only)
#define ProcessorIdleVeto 81                            // in: (kernel-mode only) // deprecated
#define PlatformIdleVeto 82                             // in: (kernel-mode only) // deprecated
#define SystemBatteryStatePrecise 83                    // out: SYSTEM_BATTERY_STATE
#define ThermalEvent 84                                 // in: THERMAL_EVENT // PowerReportThermalEvent
#define PowerRequestActionInternal 85                   // in: POWER_REQUEST_ACTION_INTERNAL
#define BatteryDeviceState 86                           // in: PCWSTR BatteryDevicePath, out: BATTERY_DEVICE_STATE
#define PowerInformationInternal 87                     // in: POWER_INFORMATION_LEVEL_INTERNAL // PopPowerInformationInternal
#define ThermalStandby 88                               // in: NULL // shutdown with thermal standby as reason
#define SystemHiberFileType 89                          // in: ULONG // 0 = reduced, nonzero = full
#define PhysicalPowerButtonPress 90                     // in: BOOLEAN
#define QueryPotentialDripsConstraint 91                // in: DEVICE_OBJECT (InputBufferLength == sizeof(DEVICE_OBJECT) == 0x150), out: BOOLEAN (1 byte) // PopFxIsDevicePotentialDripsConstraint // (kernel-mode only, AoAc only)
#define EnergyTrackerCreate 92                          // in: POWER_INFORMATION_ENERGY_TRACKER_CREATE_INPUT, out: POWER_INFORMATION_ENERGY_TRACKER_CREATE_OUTPUT
#define EnergyTrackerQuery 93                           // in: POWER_INFORMATION_ENERGY_TRACKER_QUERY_INPUT, out: POWER_INFORMATION_ENERGY_TRACKER_QUERY_OUTPUT
#define UpdateBlackBoxRecorder 94                       // in: POWER_INFORMATION_BBR_UPDATE_REQUEST_INPUT (InputBufferLength must be 0x20), out: NULL // PopBlackBoxUpdate: writes into the power black-box recorder entry selected by Index (0..24); Flags bit0 selects append-at-Offset vs replace snapshot (up to 4096 bytes); protected entries require a WinTcb caller
#define SessionAllowExternalDmaDevices 95               // in: POWER_SESSION_ALLOW_EXTERNAL_DMA_DEVICES
#define SendSuspendResumeNotification 96                // in: BOOLEAN // since WIN11
#define BlackBoxRecorderDirectAccessBuffer 97           // in: POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_INPUT (>= 0x20), out: POWER_INFORMATION_BBR_DIRECT_ACCESS_RESPONSE_OUTPUT (>= 0x10) // PopBlackBoxDirectAccess // since WIN11
#define SystemPowerSourceState 98                       // out: SYSTEM_POWER_SOURCE_STATE // since 25H2
#define PowerInformationLevelMaximum 99
#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

/**
 * The SYSTEM_POWER_POLICY structure contains information about the current system power policy.
 */
typedef struct _SYSTEM_POWER_POLICY_ACDC // SYSTEM_POWER_POLICY
{
    ULONG Revision;
    // Events
    POWER_ACTION_POLICY PowerButton;
    POWER_ACTION_POLICY SleepButton;
    POWER_ACTION_POLICY LidClose;
    SYSTEM_POWER_STATE LidOpenWake;
    ULONG Reserved;
    // "system idle" detection
    POWER_ACTION_POLICY Idle;
    ULONG IdleTimeout;
    UCHAR IdleSensitivity;
    UCHAR DynamicThrottle;
    UCHAR Spare2[2];
    // meaning of power action "sleep"
    SYSTEM_POWER_STATE MinSleep;
    SYSTEM_POWER_STATE MaxSleep;
    SYSTEM_POWER_STATE ReducedLatencySleep;
    ULONG WinLogonFlags;
    ULONG Spare3;
    // parameters for dozing
    ULONG DozeS4Timeout;
    // Battery policies
    ULONG BroadcastCapacityResolution;
    SYSTEM_POWER_LEVEL DischargePolicy[NUM_DISCHARGE_POLICIES];
    // Video policies
    ULONG VideoTimeout;
    BOOLEAN VideoDimDisplay;
    ULONG VideoReserved[3];
    // Hard disk policies
    ULONG SpindownTimeout;
    // Processor policies
    BOOLEAN OptimizeForPower;
    UCHAR FanThrottleTolerance;
    UCHAR ForcedThrottle;
    UCHAR MinThrottle;
    POWER_ACTION_POLICY OverThrottled;
} SYSTEM_POWER_POLICY_ACDC, *PSYSTEM_POWER_POLICY_ACDC;

//
// PopInitializePowerPolicySimulate initializes the power-manager simulation
//
// - HKLM\System\CurrentControlSet\Control\Session Manager\ DWORD PowerPolicySimulate = 1
// - later NtPowerInformation handlers check bits in PowerPolicySimulate
// - those bits unlock test-only behavior such as accepting synthetic capability/policy input
//
//  POWER_INFORMATION_LEVEL
//
//  - SystemPowerCapabilities (4)
//      - if InputBuffer != NULL, the set/simulate path is only allowed when PowerPolicySimulate & 1
//      - that lets the caller inject simulated SYSTEM_POWER_CAPABILITIES
//  - SystemPowerStateHandler (6)
//      - registration itself does not require PowerPolicySimulate
//      - but the handler registration path checks PowerPolicySimulate bits to decide whether to publish simulated capabilities via PopChangeCapability
//      - bits used here: 0x08, 0x20, 0x40, 0x2000
//
//  POWER_INFORMATION_LEVEL_INTERNAL
//
//  - PowerInternalUserAbsencePrediction (3)
//      - the update path only works when PowerPolicySimulate & 1
//      - otherwise it returns STATUS_INVALID_PARAMETER
//      - on success it calls PopUpdateSmartUserPresencePredictions(...)
//

/**
 * The SYSTEM_POWER_CAPABILITIES_POLICY structure describes the power capabilities of the system.
 * This is the internal name for the SYSTEM_POWER_CAPABILITIES structure.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-system_power_capabilities
 */
typedef struct _SYSTEM_POWER_CAPABILITIES_POLICY // SYSTEM_POWER_CAPABILITIES
{
    // Misc supported system features
    BOOLEAN PowerButtonPresent;
    BOOLEAN SleepButtonPresent;
    BOOLEAN LidPresent;
    BOOLEAN SystemS1;
    BOOLEAN SystemS2;
    BOOLEAN SystemS3;
    BOOLEAN SystemS4; // hibernate
    BOOLEAN SystemS5; // off
    BOOLEAN HiberFilePresent;
    BOOLEAN FullWake;
    BOOLEAN VideoDimPresent;
    BOOLEAN ApmPresent;
    BOOLEAN UpsPresent;
    // Processors
    BOOLEAN ThermalControl;
    BOOLEAN ProcessorThrottle;
    UCHAR ProcessorMinThrottle;
    UCHAR ProcessorMaxThrottle;
    BOOLEAN FastSystemS4;
    BOOLEAN Hiberboot;
    BOOLEAN WakeAlarmPresent;
    BOOLEAN AoAc;
    // Disk
    BOOLEAN DiskSpinDown;
    // HiberFile
    BYTE HiberFileType;
    BOOLEAN AoAcConnectivitySupported;
    BYTE spare3[6];
    // System Battery
    BOOLEAN SystemBatteriesPresent;
    BOOLEAN BatteriesAreShortTerm;
    BATTERY_REPORTING_SCALE BatteryScale[3];
    // Wake
    SYSTEM_POWER_STATE AcOnLineWake;
    SYSTEM_POWER_STATE SoftLidWake;
    SYSTEM_POWER_STATE RtcWake;
    SYSTEM_POWER_STATE MinDeviceWakeState; // note this may change on driver load
    SYSTEM_POWER_STATE DefaultLowLatencyWake;
} SYSTEM_POWER_CAPABILITIES_POLICY, *PSYSTEM_POWER_CAPABILITIES_POLICY;

/**
 * The SYSTEM_POWER_BATTERY_STATE structure describes the current state of the system battery.
 * This is the internal name for the SYSTEM_BATTERY_STATE structure.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-system_battery_state
 */
typedef struct _SYSTEM_POWER_BATTERY_STATE // SYSTEM_BATTERY_STATE
{
    BOOLEAN AcOnLine;
    BOOLEAN BatteryPresent;
    BOOLEAN Charging;
    BOOLEAN Discharging;
    BOOLEAN Spare1[3];
    UCHAR Tag;
    ULONG MaxCapacity;
    ULONG RemainingCapacity;
    ULONG Rate;
    ULONG EstimatedTime;
    ULONG DefaultAlert1;
    ULONG DefaultAlert2;
} SYSTEM_POWER_BATTERY_STATE, *PSYSTEM_POWER_BATTERY_STATE;

// Administrator power policy overrides
/**
 * The SYSTEM_POWER_ADMINISTRATOR_POLICY structure describes administrator-defined overrides applied on top of the active system power policy.
 * This is the internal name for the ADMINISTRATOR_POWER_POLICY structure.
 */
typedef struct _SYSTEM_POWER_ADMINISTRATOR_POLICY // ADMINISTRATOR_POWER_POLICY
{
    // meaning of power action "sleep"
    SYSTEM_POWER_STATE MinSleep;
    SYSTEM_POWER_STATE MaxSleep;
    // video policies
    ULONG MinVideoTimeout;
    ULONG MaxVideoTimeout;
    // disk policies
    ULONG MinSpindownTimeout;
    ULONG MaxSpindownTimeout;
} SYSTEM_POWER_ADMINISTRATOR_POLICY, *PSYSTEM_POWER_ADMINISTRATOR_POLICY;

/**
 * The PROCESSOR_POWER_INFORMATION structure contains information about the power characteristics of a processor.
 * \sa https://learn.microsoft.com/en-us/windows/win32/power/processor-power-information-str
 */
typedef struct _PROCESSOR_POWER_INFORMATION
{
    ULONG Number;
    ULONG MaxMhz;
    ULONG CurrentMhz;
    ULONG MhzLimit;
    ULONG MaxIdleState;
    ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

// SYSTEM_NOTIFY_USER_MODE_LEGACY_POWER_EVENT EventType values
#define POWER_USER_MODE_LEGACY_EVENT_SYSTEM 0
#define POWER_USER_MODE_LEGACY_EVENT_SERVICE 3

// SYSTEM_NOTIFY_USER_MODE_LEGACY_POWER_EVENT EventCode values
#define POWER_USER_MODE_LEGACY_EVENT_CODE_TRANSITION 0x4
#define POWER_USER_MODE_LEGACY_EVENT_CODE_RESUME_COMPLETE 0x7
#define POWER_USER_MODE_LEGACY_EVENT_CODE_SUSPEND 0x12

// SYSTEM_NOTIFY_USER_MODE_LEGACY_POWER_EVENT NotifyPfIo values
#define POWER_USER_MODE_LEGACY_EVENT_NO_PFIO_NOTIFY 0x0
#define POWER_USER_MODE_LEGACY_EVENT_PFIO_NOTIFY 0x1

// SYSTEM_NOTIFY_USER_MODE_LEGACY_POWER_EVENT SendFlags values
#define POWER_USER_MODE_LEGACY_EVENT_SEND_SYNC 0x1
#define POWER_USER_MODE_LEGACY_EVENT_SEND_ASYNC 0x100

// rev
/**
 * The SYSTEM_NOTIFY_USER_MODE_LEGACY_POWER_EVENT structure describes a legacy user-mode power event delivered to registered listeners.
 */
typedef struct _SYSTEM_NOTIFY_USER_MODE_LEGACY_POWER_EVENT
{
    ULONG EventType;
    ULONG EventCode;
    ULONG SessionId;
    UCHAR NotifyPfIo;
    UCHAR SendFlags;
    USHORT Reserved;
} SYSTEM_NOTIFY_USER_MODE_LEGACY_POWER_EVENT, *PSYSTEM_NOTIFY_USER_MODE_LEGACY_POWER_EVENT;

// rev
/**
 * The PROCESSOR_GROUP_SELECTOR structure describes processor group infoclass.
 */
typedef struct _PROCESSOR_GROUP_SELECTOR
{
    USHORT Group; // Processor group index to clear.
} PROCESSOR_GROUP_SELECTOR, *PPROCESSOR_GROUP_SELECTOR;

// rev
/**
 * The PROCESSOR_GROUP_PARK_MASK structure describes processor group park mask.
 */
typedef struct _PROCESSOR_GROUP_PARK_MASK
{
    ULONGLONG ForceMask; // Forced parked logical-processor mask for Group.
    USHORT Group; // Processor group index.
    USHORT Reserved1; // Must be zero.
    USHORT Reserved2; // Must be zero.
    USHORT Reserved3; // Must be zero.
} PROCESSOR_GROUP_PARK_MASK, *PPROCESSOR_GROUP_PARK_MASK;

// rev
/**
 * The PROCESSOR_GROUP_PARK_MASK_EX structure describes processor group park mask ex.
 */
typedef struct _PROCESSOR_GROUP_PARK_MASK_EX
{
    ULONGLONG ForceMask; // Forced parked logical-processor mask for Group.
    USHORT Group; // Processor group index.
    USHORT Reserved1; // Must be zero.
    USHORT Reserved2; // Must be zero.
    USHORT Reserved3; // Must be zero.
    ULONGLONG SecondaryMask; // Secondary forced mask; must be a subset of ForceMask.
} PROCESSOR_GROUP_PARK_MASK_EX, *PPROCESSOR_GROUP_PARK_MASK_EX;

// rev
/**
 * The PROCESSOR_SET_IDLE_STATE structure describes processor set idle state.
 */
typedef struct _PROCESSOR_SET_IDLE_STATE
{
    ULONG StateIndex;
    PROCESSOR_NUMBER ProcessorNumber;
} PROCESSOR_SET_IDLE_STATE, *PPROCESSOR_SET_IDLE_STATE;

// rev
/**
 * The LOGICAL_PROCESSOR_IDLING_INPUT structure contains the input parameters for the logical processor idling operation.
 */
typedef struct _LOGICAL_PROCESSOR_IDLING_INPUT
{
    ULONG LpiCap;
    ULONG ThermalCap;
} LOGICAL_PROCESSOR_IDLING_INPUT, *PLOGICAL_PROCESSOR_IDLING_INPUT;

// rev
/**
 * The LOGICAL_PROCESSOR_IDLING_OUTPUT structure contains the output data returned by the logical processor idling operation.
 */
typedef struct _LOGICAL_PROCESSOR_IDLING_OUTPUT
{
    ULONG EffectiveLpiCap;
} LOGICAL_PROCESSOR_IDLING_OUTPUT, *PLOGICAL_PROCESSOR_IDLING_OUTPUT;

// rev
/**
 * The POWER_SETTING_NOTIFICATION_NAME_INPUT structure contains the input parameters for the setting notification name operation.
 */
typedef struct _POWER_SETTING_NOTIFICATION_NAME_INPUT
{
    GUID SettingGuid;
} POWER_SETTING_NOTIFICATION_NAME_INPUT, *PPOWER_SETTING_NOTIFICATION_NAME_INPUT;

//typedef WNF_STATE_NAME POWER_SETTING_NOTIFICATION_NAME_OUTPUT;
//typedef WNF_STATE_NAME *PPOWER_SETTING_NOTIFICATION_NAME_OUTPUT;

// rev
/**
 * The GET_POWER_SETTING_VALUE_INPUT structure contains the input parameters for the get power setting value operation.
 */
typedef struct _GET_POWER_SETTING_VALUE_INPUT
{
    GUID SettingGuid;
} GET_POWER_SETTING_VALUE_INPUT, *PGET_POWER_SETTING_VALUE_INPUT;

// rev
/**
 * The GET_POWER_SETTING_VALUE_ENTRY structure describes get power setting value entry.
 */
typedef struct _GET_POWER_SETTING_VALUE_ENTRY
{
    ULONG ChangeStamp;
    ULONG ValueLength;
    UCHAR Value[1];
} GET_POWER_SETTING_VALUE_ENTRY, *PGET_POWER_SETTING_VALUE_ENTRY;

// rev
/**
 * The GET_POWER_SETTING_VALUE_OUTPUT structure contains the output data returned by the get power setting value operation.
 */
typedef struct _GET_POWER_SETTING_VALUE_OUTPUT
{
    ULONG TotalLength;
    UCHAR Entries[1];
} GET_POWER_SETTING_VALUE_OUTPUT, *PGET_POWER_SETTING_VALUE_OUTPUT;

// rev
/**
 * The MONITOR_INVOCATION_INPUT structure contains the input parameters for the monitor invocation operation.
 */
typedef struct _MONITOR_INVOCATION_INPUT
{
    BOOLEAN Invoke;
    UCHAR Reserved[3];
    ULONG SessionId;
} MONITOR_INVOCATION_INPUT, *PMONITOR_INVOCATION_INPUT;

// rev
/**
 * The QUERY_POTENTIAL_DRIPS_CONSTRAINT_INPUT structure is the input for the QueryPotentialDripsConstraint (91) information level;
 * it is a full DEVICE_OBJECT (InputBufferLength must equal sizeof(DEVICE_OBJECT), 0x150). The level returns a BOOLEAN indicating
 * whether the device is a potential DRIPS (deepest AoAc idle) constraint.
 * \remarks Kernel-mode only; requires an AoAc (Modern Standby) platform. Reversed from PopFxIsDevicePotentialDripsConstraint.
 */
typedef struct _DEVICE_OBJECT QUERY_POTENTIAL_DRIPS_CONSTRAINT_INPUT, *PQUERY_POTENTIAL_DRIPS_CONSTRAINT_INPUT;

// rev
/**
 * The PDC_INVOCATION structure describes pdc invocation.
 */
typedef struct _PDC_INVOCATION
{
    ULONG Operation;
    ULONG Reserved;
    PVOID Arguments[27];
} PDC_INVOCATION, *PPDC_INVOCATION;

// rev
/**
 * The PDC_INVOCATION_CALLBACKS structure describes pdc invocation callbacks.
 */
typedef struct _PDC_INVOCATION_CALLBACKS
{
    PVOID Callbacks[21];
} PDC_INVOCATION_CALLBACKS, *PPDC_INVOCATION_CALLBACKS;

/**
 * The POWER_CS_DEVICE_NOTIFICATION structure describes power cs device notification.
 */
typedef struct _POWER_CS_DEVICE_NOTIFICATION
{
    ULONGLONG DeviceId;
    ULONG Compliance;
    UCHAR Add;
    UCHAR Flags;
    USHORT Reserved;
} POWER_CS_DEVICE_NOTIFICATION, *PPOWER_CS_DEVICE_NOTIFICATION;

// The GroupPark infoclass supports debug-only input for forcing or clearing CPU parking masks.
//
//    NtPowerInformation behavior:
//   - requires KdDebuggerEnabled != 0
//       - otherwise returns STATUS_ACCESS_DENIED
//   - requires InputBuffer != NULL
//   - requires OutputBuffer == NULL
//
//   Accepted input shapes:
//   - InputBufferLength == 0x10
//       - calls PpmParkApplyForcedMask(InputBuffer, NULL)
//   - InputBufferLength == 0x18
//       - calls PpmParkApplyForcedMask(InputBuffer, InputBuffer + 0x10)
//   - InputBufferLength == 0x2
//       - calls PpmParkClearForcedMask(InputBuffer)
//
//   Recovered layouts:
//   typedef struct _PROCESSOR_GROUP_PARK_MASK
//   {
//       ULONGLONG ForceMask;
//       USHORT Group;
//       USHORT Reserved1;
//       USHORT Reserved2;
//       USHORT Reserved3;
//   } PROCESSOR_GROUP_PARK_MASK;
//
//   For the 0x18 form:
//   typedef struct _PROCESSOR_GROUP_PARK_MASK_EX
//   {
//       ULONGLONG ForceMask;
//       USHORT Group;
//       USHORT Reserved1;
//       USHORT Reserved2;
//       USHORT Reserved3;
//       ULONGLONG SecondaryMask;
//   } PROCESSOR_GROUP_PARK_MASK_EX;
//
//   For the 0x2 clear form:
//   typedef struct _PROCESSOR_GROUP_SELECTOR
//   {
//       USHORT Group;
//   } PROCESSOR_GROUP_SELECTOR;
//
//   Validation and meaning:
//   - Group must be < 0x20
//   - all reserved words in the 0x10/0x18 form must be zero
//   - in the 0x18 form, SecondaryMask must be a subset of ForceMask
//   - on success it updates per-node forced parking state and reapplies park policy
//
//   So the practical meaning of GroupPark is:
//   - 0x10: force a park mask for one processor group
//   - 0x18: force a primary and secondary mask for one processor group
//   - 0x02: clear the forced park mask for a processor group

// CoolingMode flags
#define PO_TZ_ACTIVE 0 // The system is currently in Active cooling mode.
#define PO_TZ_PASSIVE 1 // The system does not support CPU throttling, or there is no thermal zone defined in the system.
#define PO_TZ_INVALID_MODE 2 //The system is currently in Passive cooling mode.

/**
 * The SYSTEM_POWER_INFORMATION structure contains information about the idleness of the system.
 * \sa https://learn.microsoft.com/en-us/windows/win32/power/system-power-information-str
 */
typedef struct _SYSTEM_POWER_INFORMATION
{
    ULONG MaxIdlenessAllowed;
    ULONG Idleness;
    ULONG TimeRemaining;
    UCHAR CoolingMode;
} SYSTEM_POWER_INFORMATION, *PSYSTEM_POWER_INFORMATION;

/**
 * The SYSTEM_HIBERFILE_INFORMATION structure describes system hiberfile information.
 */
typedef struct _SYSTEM_HIBERFILE_INFORMATION
{
    ULONG NumberOfMcbPairs;
    LARGE_INTEGER Mcb[1];
} SYSTEM_HIBERFILE_INFORMATION, *PSYSTEM_HIBERFILE_INFORMATION;

/**
 * The SYSTEM_SERVICE_POWER_MESSAGE structure describes system service power message.
 */
typedef struct _SYSTEM_SERVICE_POWER_MESSAGE
{
    ULONG MessageId;
    ULONG SessionId;
    ULONG Flags;
} SYSTEM_SERVICE_POWER_MESSAGE, *PSYSTEM_SERVICE_POWER_MESSAGE;

//typedef enum POWER_USER_PRESENCE_TYPE
//{
//    UserNotPresent = 0,
//    UserPresent = 1,
//    UserUnknown = 0xff
//} POWER_USER_PRESENCE_TYPE, *PPOWER_USER_PRESENCE_TYPE;

//typedef struct _POWER_USER_PRESENCE
//{
//    POWER_USER_PRESENCE_TYPE PowerUserPresence;
//} POWER_USER_PRESENCE, *PPOWER_USER_PRESENCE;

//typedef struct _POWER_SESSION_CONNECT
//{
//    BOOLEAN Connected;  // TRUE - connected, FALSE - disconnected
//    BOOLEAN Console;    // TRUE - console, FALSE - TS (not used for Connected = FALSE)
//} POWER_SESSION_CONNECT, *PPOWER_SESSION_CONNECT;

//typedef struct _POWER_SESSION_TIMEOUTS
//{
//    ULONG InputTimeout;
//    ULONG DisplayTimeout;
//} POWER_SESSION_TIMEOUTS, *PPOWER_SESSION_TIMEOUTS;

//typedef struct _POWER_SESSION_RIT_STATE
//{
//    BOOLEAN Active;  // TRUE - RIT input received, FALSE - RIT timeout
//    ULONG64 LastInputTime; // last input time held for this session
//} POWER_SESSION_RIT_STATE, *PPOWER_SESSION_RIT_STATE;

// typedef struct _POWER_SESSION_WINLOGON
// {
//     BOOLEAN Console;
//     BOOLEAN Locked;
//     BYTE Reserved[6];
// } POWER_SESSION_WINLOGON, *PPOWER_SESSION_WINLOGON;

//typedef struct _POWER_SESSION_ALLOW_EXTERNAL_DMA_DEVICES
//{
//    BOOLEAN IsAllowed;
//} POWER_SESSION_ALLOW_EXTERNAL_DMA_DEVICES, *PPOWER_SESSION_ALLOW_EXTERNAL_DMA_DEVICES;
//
//typedef struct _POWER_IDLE_RESILIENCY
//{
//    ULONG CoalescingTimeout;
//    ULONG IdleResiliencyPeriod;
//} POWER_IDLE_RESILIENCY, *PPOWER_IDLE_RESILIENCY;

//typedef struct _RESUME_PERFORMANCE
//{
//    ULONG PostTimeMs;
//    ULONGLONG TotalResumeTimeMs;
//    ULONGLONG ResumeCompleteTimestamp;
//} RESUME_PERFORMANCE, *PRESUME_PERFORMANCE;

#define POWER_PERF_SCALE    100
#define PERF_LEVEL_TO_PERCENT(_x_) ((_x_ * 1000) / (POWER_PERF_SCALE * 10))
#define PERCENT_TO_PERF_LEVEL(_x_) ((_x_ * POWER_PERF_SCALE * 10) / 1000)
#define PO_REASON_STATE_STANDBY (PO_REASON_STATE_S1 | \
                                 PO_REASON_STATE_S2 | \
                                 PO_REASON_STATE_S3)

#define PO_REASON_STATE_ALL     (PO_REASON_STATE_STANDBY | \
                                 PO_REASON_STATE_S4 | \
                                 PO_REASON_STATE_S4FIRM)

/**
 * The SYSTEM_POWER_LOGGING_ENTRY structure describes a system power logging entry.
 */
typedef struct _SYSTEM_POWER_LOGGING_ENTRY
{
    ULONG Reason;
    ULONG States;
} SYSTEM_POWER_LOGGING_ENTRY, *PSYSTEM_POWER_LOGGING_ENTRY;

/**
 * The SET_POWER_SETTING_VALUE_INPUT structure contains the input parameters used to set a power setting value.
 */
typedef struct _SET_POWER_SETTING_VALUE_INPUT
{
    ULONG Version;
    GUID SettingGuid;
    ULONG Type;
    ULONG ValueLength;
    UCHAR Value[1];
} SET_POWER_SETTING_VALUE_INPUT, *PSET_POWER_SETTING_VALUE_INPUT;

typedef SET_POWER_SETTING_VALUE_INPUT SYSTEM_POWER_SETTING_VALUE, *PSYSTEM_POWER_SETTING_VALUE;

// typedef struct _NOTIFY_USER_POWER_SETTING
// {
//     GUID Guid;
// } NOTIFY_USER_POWER_SETTING, *PNOTIFY_USER_POWER_SETTING;

/**
 * The SYSTEM_POWER_TRACE_APPLICATION_POWER_MESSAGE structure describes system power trace application power message.
 */
typedef struct _SYSTEM_POWER_TRACE_APPLICATION_POWER_MESSAGE
{
    HANDLE ProcessId;
} SYSTEM_POWER_TRACE_APPLICATION_POWER_MESSAGE, *PSYSTEM_POWER_TRACE_APPLICATION_POWER_MESSAGE;

/**
 * The SYSTEM_POWER_TRACE_APPLICATION_POWER_MESSAGE_END structure describes system power trace application power message end.
 */
typedef struct _SYSTEM_POWER_TRACE_APPLICATION_POWER_MESSAGE_END
{
    ULONG ProcessId;
} SYSTEM_POWER_TRACE_APPLICATION_POWER_MESSAGE_END, *PSYSTEM_POWER_TRACE_APPLICATION_POWER_MESSAGE_END;

/**
 * The POWER_STATE_DISABLED_TYPE enumeration defines the sleep states that can be individually disabled.
 */
typedef enum _POWER_STATE_DISABLED_TYPE
{
    PoDisabledStateSleeping1 = 0,
    PoDisabledStateSleeping2 = 1,
    PoDisabledStateSleeping3 = 2,
    PoDisabledStateSleeping4 = 3,
    PoDisabledStateSleeping0Idle = 4,
    PoDisabledStateReserved5 = 5,
    PoDisabledStateSleeping4Firmware = 6,
    PoDisabledStateMaximum = 7
} POWER_STATE_DISABLED_TYPE, *PPOWER_STATE_DISABLED_TYPE;

#define POWER_STATE_DISABLED_TYPE_MAX  8

/**
 * The SYSTEM_POWER_STATE_DISABLE_REASON structure describes which sleep states are disabled and the reason they are unavailable.
 */
_Struct_size_bytes_(sizeof(SYSTEM_POWER_STATE_DISABLE_REASON) + PowerReasonLength)
typedef struct _SYSTEM_POWER_STATE_DISABLE_REASON
{
    BOOLEAN AffectedState[POWER_STATE_DISABLED_TYPE_MAX];
    ULONG PowerReasonCode;
    ULONG PowerReasonLength;
    //UCHAR PowerReasonInfo[ANYSIZE_ARRAY];
} SYSTEM_POWER_STATE_DISABLE_REASON, *PSYSTEM_POWER_STATE_DISABLE_REASON;

// Reason Context
#define POWER_REQUEST_CONTEXT_NOT_SPECIFIED DIAGNOSTIC_REASON_NOT_SPECIFIED

// wdm
/**
 * The COUNTED_REASON_CONTEXT structure describes the reason a power request was created, either as a localized resource reference or a simple string.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ns-wdm-_counted_reason_context
 */
typedef struct _COUNTED_REASON_CONTEXT
{
    ULONG Version;
    ULONG Flags;
    union
    {
        struct
        {
            UNICODE_STRING ResourceFileName;
            USHORT ResourceReasonId;
            ULONG StringCount;
            _Field_size_(StringCount) PUNICODE_STRING ReasonStrings;
        };
        UNICODE_STRING SimpleString;
    };
} COUNTED_REASON_CONTEXT, *PCOUNTED_REASON_CONTEXT;

/**
 * The POWER_REQUEST_TYPE_INTERNAL enumeration defines the internal power request types used by the power manager.
 * This is the internal name for the POWER_REQUEST_TYPE enumeration.
 */
typedef enum _POWER_REQUEST_TYPE_INTERNAL // POWER_REQUEST_TYPE
{
    PowerRequestDisplayRequiredInternal,
    PowerRequestSystemRequiredInternal,
    PowerRequestAwayModeRequiredInternal,
    PowerRequestExecutionRequiredInternal, // Windows 8+
    PowerRequestPerfBoostRequiredInternal, // Windows 8+
    PowerRequestActiveLockScreenInternal, // Windows 10 RS1+ (reserved on Windows 8)
    // Values 6 and 7 are reserved for Windows 8 only
    PowerRequestInternalInvalid,
    PowerRequestInternalUnknown,
    PowerRequestFullScreenVideoRequired  // Windows 8 only
} POWER_REQUEST_TYPE_INTERNAL;

/**
 * The POWER_REQUEST_ACTION structure describes a set or clear operation performed on a power availability request.
 */
typedef struct _POWER_REQUEST_ACTION
{
    HANDLE PowerRequestHandle;
    POWER_REQUEST_TYPE_INTERNAL RequestType;
    BOOLEAN SetAction;
    HANDLE ProcessHandle; // Windows 8+ and only for requests created via PlmPowerRequestCreate
} POWER_REQUEST_ACTION, *PPOWER_REQUEST_ACTION;

/**
 * The POWER_STATE union specifies a system power state or a device power state.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ns-wdm-_power_state
 */
typedef union _POWER_STATE
{
    SYSTEM_POWER_STATE SystemState;
    DEVICE_POWER_STATE DeviceState;
} POWER_STATE, *PPOWER_STATE;

/**
 * The POWER_STATE_TYPE enumeration indicates whether a POWER_STATE value refers to a system power state or a device power state.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ne-wdm-_power_state_type
 */
typedef enum _POWER_STATE_TYPE
{
    SystemPowerState = 0,
    DevicePowerState
} POWER_STATE_TYPE, *PPOWER_STATE_TYPE;

// wdm
/**
 * The SYSTEM_POWER_STATE_CONTEXT structure encodes the current, effective, and target system power states of a power transition.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ns-wdm-_system_power_state_context
 */
typedef struct _SYSTEM_POWER_STATE_CONTEXT
{
    union
    {
        struct
        {
            ULONG Reserved1 : 8;
            ULONG TargetSystemState : 4;
            ULONG EffectiveSystemState : 4;
            ULONG CurrentSystemState : 4;
            ULONG IgnoreHibernationPath : 1;
            ULONG PseudoTransition : 1;
            ULONG KernelSoftReboot : 1;
            ULONG DirectedDripsTransition : 1;
            ULONG Reserved2 : 8;
        };
        ULONG ContextAsUlong;
    };
} SYSTEM_POWER_STATE_CONTEXT, *PSYSTEM_POWER_STATE_CONTEXT;

/**
 * The REQUESTER_TYPE enumeration identifies the kind of caller that created a power request.
 */
typedef enum _REQUESTER_TYPE
{
    KernelRequester = 0,
    UserProcessRequester = 1,
    UserSharedServiceRequester = 2
} REQUESTER_TYPE;

/**
 * The DUMMYSTRUCTNAME structure describes counted reason context relative.
 */
typedef struct _COUNTED_REASON_CONTEXT_RELATIVE
{
    ULONG Flags;
    union
    {
        struct
        {
            SIZE_T ResourceFileNameOffset;
            USHORT ResourceReasonId;
            ULONG StringCount;
            SIZE_T SubstitutionStringsOffset;
        } DUMMYSTRUCTNAME;
        SIZE_T SimpleStringOffset;
    } DUMMYUNIONNAME;
} COUNTED_REASON_CONTEXT_RELATIVE, *PCOUNTED_REASON_CONTEXT_RELATIVE;

/**
 * The DUMMYSTRUCTNAME structure describes the diagnostic reason context associated with a power request.
 */
typedef struct _DIAGNOSTIC_BUFFER
{
    SIZE_T Size;
    REQUESTER_TYPE CallerType;
    union
    {
        struct
        {
            SIZE_T ProcessImageNameOffset; // PWSTR
            ULONG ProcessId;
            ULONG ServiceTag;
        } DUMMYSTRUCTNAME;
        struct
        {
            SIZE_T DeviceDescriptionOffset; // PWSTR
            SIZE_T DevicePathOffset; // PWSTR
        } DUMMYSTRUCTNAME2;
    } DUMMYUNIONNAME;
    SIZE_T ReasonOffset; // PCOUNTED_REASON_CONTEXT_RELATIVE
} DIAGNOSTIC_BUFFER, *PDIAGNOSTIC_BUFFER;

/**
 * The WAKE_TIMER_INFO structure describes a pending wake timer entry returned by the WakeTimerList information level.
 */
typedef struct _WAKE_TIMER_INFO
{
    SIZE_T OffsetToNext;
    LARGE_INTEGER DueTime;
    ULONG Period;
    DIAGNOSTIC_BUFFER ReasonContext;
} WAKE_TIMER_INFO, *PWAKE_TIMER_INFO;

// rev
/**
 * The PROCESSOR_PERF_CAP_HV structure describes processor perf cap hv.
 */
typedef struct _PROCESSOR_PERF_CAP_HV
{
    ULONG Version;
    ULONG InitialApicId;
    ULONG Ppc;
    ULONG Tpc;
    ULONG ThermalCap;
} PROCESSOR_PERF_CAP_HV, *PPROCESSOR_PERF_CAP_HV;

// rev
/**
 * The PROCESSOR_IDLE_TIMES structure describes processor idle times.
 */
typedef struct PROCESSOR_IDLE_TIMES
{
    ULONG64 StartTime;
    ULONG64 EndTime;
    ULONG Reserved[4];
} PROCESSOR_IDLE_TIMES, *PPROCESSOR_IDLE_TIMES;

typedef _Function_class_(PROCESSOR_IDLE_HANDLER)
NTSTATUS FASTCALL PROCESSOR_IDLE_HANDLER(
    _In_ ULONG_PTR Context,
    _Inout_ PPROCESSOR_IDLE_TIMES IdleTimes
    );
typedef PROCESSOR_IDLE_HANDLER *PPROCESSOR_IDLE_HANDLER;

#define PROCESSOR_STATE_TYPE_PERFORMANCE    0x1
#define PROCESSOR_STATE_TYPE_THROTTLE       0x2

#define IDLE_STATE_FLAGS_C1_HLT     0x01        // describes C1 only
#define IDLE_STATE_FLAGS_C1_IO_HLT  0x02        // describes C1 only
#define IDLE_STATE_FLAGS_IO         0x04        // describes C2 and C3 only
#define IDLE_STATE_FLAGS_MWAIT      0x08        // describes C1, C2, C3, C4, ...

/**
 * The PROCESSOR_IDLE_STATE structure describes processor idle state.
 */
typedef struct _PROCESSOR_IDLE_STATE
{
    UCHAR StateType;
    ULONG StateFlags;
    ULONG HardwareLatency;
    ULONG Power;
    ULONG_PTR Context;
    PPROCESSOR_IDLE_HANDLER Handler;
} PROCESSOR_IDLE_STATE, *PPROCESSOR_IDLE_STATE;

/**
 * The PROCESSOR_IDLE_STATES structure describes processor idle states.
 */
typedef struct _PROCESSOR_IDLE_STATES
{
    ULONG Size;
    ULONG Revision;
    ULONG Count;
    ULONG Type;
    KAFFINITY TargetProcessors;
    PROCESSOR_IDLE_STATE State[ANYSIZE_ARRAY];
} PROCESSOR_IDLE_STATES, *PPROCESSOR_IDLE_STATES;

//
//#define PROCESSOR_IDLESTATE_POLICY_COUNT 0x3
//
//typedef struct _PROCESSOR_IDLESTATE_INFO
//{
//    ULONG TimeCheck;
//    UCHAR DemotePercent;
//    UCHAR PromotePercent;
//    UCHAR Spare[2];
//} PROCESSOR_IDLESTATE_INFO, *PPROCESSOR_IDLESTATE_INFO;
//
//typedef struct
//{
//    USHORT Revision;
//    union
//    {
//        USHORT AsUSHORT;
//        struct
//        {
//            USHORT AllowScaling : 1;
//            USHORT Disabled : 1;
//            USHORT Reserved : 14;
//        } DUMMYSTRUCTNAME;
//    } Flags;
//
//    ULONG PolicyCount;
//    PROCESSOR_IDLESTATE_INFO Policy[PROCESSOR_IDLESTATE_POLICY_COUNT];
//} PROCESSOR_IDLESTATE_POLICY, *PPROCESSOR_IDLESTATE_POLICY;

// rev
/**
 * The PROCESSOR_LOAD structure describes processor load.
 */
typedef struct _PROCESSOR_LOAD
{
    PROCESSOR_NUMBER ProcessorNumber;
    UCHAR BusyPercentage;
    UCHAR FrequencyPercentage;
    USHORT Padding;
} PROCESSOR_LOAD, *PPROCESSOR_LOAD;

// rev
/**
 * The POWER_SHUTDOWN_NOTIFICATION structure describes power shutdown notification.
 */
typedef struct _POWER_SHUTDOWN_NOTIFICATION
{
    PVOID CallbackRoutine;
    PVOID Context;
} POWER_SHUTDOWN_NOTIFICATION, *PPOWER_SHUTDOWN_NOTIFICATION;

// rev
/**
 * The POWER_MONITOR_CAPABILITIES structure describes power monitor capabilities.
 */
typedef struct _POWER_MONITOR_CAPABILITIES
{
    ULONG State; // BOOLEAN brightness-capable state
} POWER_MONITOR_CAPABILITIES, *PPOWER_MONITOR_CAPABILITIES;

// rev
/**
 * The POWER_SESSION_POWER_INIT structure describes power session power init.
 */
typedef struct _POWER_SESSION_POWER_INIT
{
    PBOOLEAN NoMoreInput;
    PBOOLEAN HiberBootForceMonitorOff;

    ULONG Reserved0;
    ULONG VideoDimTimeout;

    ULONG AwayModeEnabled;
    ULONG AwayModePolicy;

    ULONG VideoBrightnessPercent;
    ULONG VideoDimBrightnessPercent;

    ULONG Unk28; // initialized to 100, no strong named xref
    ULONG AdaptiveDisplayBrightness;

    BOOLEAN EnergySaverActive; // (PopEsState == 1)
    BOOLEAN LidOpened;
    USHORT Reserved1;

    ULONG EnergySaverBrightnessPercent;

    BOOLEAN TtmEnabled;
    BOOLEAN LidStateReliable;
    UCHAR Reserved2[6];
} POWER_SESSION_POWER_INIT, *PPOWER_SESSION_POWER_INIT;

// rev
/**
 * The POWER_SESSION_DISPLAY_STATE structure describes power session display state.
 */
typedef struct _POWER_SESSION_DISPLAY_STATE
{
    ULONG SessionId;
    ULONG DisplayState; // e.g. on, off, dimmed
} POWER_SESSION_DISPLAY_STATE, *PPOWER_SESSION_DISPLAY_STATE;

// rev
/**
 * The PROCESSOR_CAP structure describes processor cap.
 */
typedef struct _PROCESSOR_CAP
{
    ULONG Version;
    PROCESSOR_NUMBER ProcessorNumber;
    ULONG PlatformCap;
    ULONG ThermalCap;
    ULONG LimitReasons;
} PROCESSOR_CAP, *PPROCESSOR_CAP;

/**
 * The PO_WAKE_SOURCE_INFO structure describes po wake source info.
 */
typedef struct _PO_WAKE_SOURCE_INFO
{
    ULONG Count;
    ULONG Offsets[ANYSIZE_ARRAY]; // POWER_WAKE_SOURCE_HEADER, POWER_WAKE_SOURCE_INTERNAL, POWER_WAKE_SOURCE_TIMER, POWER_WAKE_SOURCE_FIXED
} PO_WAKE_SOURCE_INFO, *PPO_WAKE_SOURCE_INFO;

/**
 * The PO_WAKE_SOURCE_HISTORY structure describes po wake source history.
 */
typedef struct _PO_WAKE_SOURCE_HISTORY
{
    ULONG Count;
    ULONG Offsets[ANYSIZE_ARRAY]; // POWER_WAKE_SOURCE_HEADER, POWER_WAKE_SOURCE_INTERNAL, POWER_WAKE_SOURCE_TIMER, POWER_WAKE_SOURCE_FIXED
} PO_WAKE_SOURCE_HISTORY, *PPO_WAKE_SOURCE_HISTORY;

/**
 * The PO_WAKE_SOURCE_TYPE enumeration defines po wake source type values.
 */
typedef enum _PO_WAKE_SOURCE_TYPE
{
    DeviceWakeSourceType = 0,
    FixedWakeSourceType = 1,
    TimerWakeSourceType = 2,
    TimerPresumedWakeSourceType = 3,
    InternalWakeSourceType = 4
} PO_WAKE_SOURCE_TYPE, *PPO_WAKE_SOURCE_TYPE;

/**
 * The PO_INTERNAL_WAKE_SOURCE_TYPE enumeration defines po internal wake source type values.
 */
typedef enum _PO_INTERNAL_WAKE_SOURCE_TYPE
{
    InternalWakeSourceDozeToHibernate = 0,
    InternalWakeSourcePredictedUserPresence = 1
} PO_INTERNAL_WAKE_SOURCE_TYPE;

/**
 * The PO_FIXED_WAKE_SOURCE_TYPE enumeration defines po fixed wake source type values.
 */
typedef enum _PO_FIXED_WAKE_SOURCE_TYPE
{
    FixedWakeSourcePowerButton = 0,
    FixedWakeSourceSleepButton = 1,
    FixedWakeSourceRtc = 2,
    FixedWakeSourceDozeToHibernate = 3
} PO_FIXED_WAKE_SOURCE_TYPE, *PPO_FIXED_WAKE_SOURCE_TYPE;

/**
 * The PO_WAKE_SOURCE_HEADER structure describes po wake source header.
 */
typedef struct _PO_WAKE_SOURCE_HEADER
{
    PO_WAKE_SOURCE_TYPE Type;
    ULONG Size;
} PO_WAKE_SOURCE_HEADER, *PPO_WAKE_SOURCE_HEADER;

/**
 * The PO_WAKE_SOURCE_DEVICE structure describes po wake source device.
 */
typedef struct _PO_WAKE_SOURCE_DEVICE
{
    PO_WAKE_SOURCE_HEADER Header;
    WCHAR InstancePath[ANYSIZE_ARRAY];
} PO_WAKE_SOURCE_DEVICE, *PPO_WAKE_SOURCE_DEVICE;

/**
 * The PO_WAKE_SOURCE_FIXED structure describes po wake source fixed.
 */
typedef struct _PO_WAKE_SOURCE_FIXED
{
    PO_WAKE_SOURCE_HEADER Header;
    PO_FIXED_WAKE_SOURCE_TYPE FixedWakeSourceType;
} PO_WAKE_SOURCE_FIXED, *PPO_WAKE_SOURCE_FIXED;

/**
 * The PO_WAKE_SOURCE_INTERNAL structure describes po wake source internal.
 */
typedef struct _PO_WAKE_SOURCE_INTERNAL
{
    PO_WAKE_SOURCE_HEADER Header;
    PO_INTERNAL_WAKE_SOURCE_TYPE InternalWakeSourceType;
} PO_WAKE_SOURCE_INTERNAL, *PPO_WAKE_SOURCE_INTERNAL;

/**
 * The PO_WAKE_SOURCE_TIMER structure describes po wake source timer.
 */
typedef struct _PO_WAKE_SOURCE_TIMER
{
    PO_WAKE_SOURCE_HEADER Header;
    DIAGNOSTIC_BUFFER Reason;
} PO_WAKE_SOURCE_TIMER, *PPO_WAKE_SOURCE_TIMER;

// The number of supported request types per version
#define POWER_REQUEST_SUPPORTED_TYPES_V1 3 // Windows 7
#define POWER_REQUEST_SUPPORTED_TYPES_V2 9 // Windows 8
#define POWER_REQUEST_SUPPORTED_TYPES_V3 5 // Windows 8.1 and Windows 10 TH1-TH2
#define POWER_REQUEST_SUPPORTED_TYPES_V4 6 // Windows 10 RS1+

/**
 * The V1 structure describes power request.
 */
typedef struct _POWER_REQUEST
{
    union
    {
        struct
        {
            ULONG SupportedRequestMask;
            ULONG PowerRequestCount[POWER_REQUEST_SUPPORTED_TYPES_V1];
            DIAGNOSTIC_BUFFER DiagnosticBuffer;
        } V1;
#if (PHNT_VERSION >= PHNT_WINDOWS_8)
        struct
        {
            ULONG SupportedRequestMask;
            ULONG PowerRequestCount[POWER_REQUEST_SUPPORTED_TYPES_V2];
            DIAGNOSTIC_BUFFER DiagnosticBuffer;
        } V2;
#endif // (PHNT_VERSION >= PHNT_WINDOWS_8)
#if (PHNT_VERSION >= PHNT_WINDOWS_8_1)
        struct
        {
            ULONG SupportedRequestMask;
            ULONG PowerRequestCount[POWER_REQUEST_SUPPORTED_TYPES_V3];
            DIAGNOSTIC_BUFFER DiagnosticBuffer;
        } V3;
#endif // (PHNT_VERSION >= PHNT_WINDOWS_8_1)
#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
        struct
        {
            ULONG SupportedRequestMask;
            ULONG PowerRequestCount[POWER_REQUEST_SUPPORTED_TYPES_V4];
            DIAGNOSTIC_BUFFER DiagnosticBuffer;
        } V4;
#endif // (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
    };
} POWER_REQUEST, *PPOWER_REQUEST;

/**
 * The POWER_REQUEST_LIST structure describes power request list.
 */
typedef struct _POWER_REQUEST_LIST
{
    ULONG_PTR Count;
    ULONG_PTR PowerRequestOffsets[ANYSIZE_ARRAY]; // PPOWER_REQUEST
} POWER_REQUEST_LIST, *PPOWER_REQUEST_LIST;

/**
 * The POWER_STATE_HANDLER_TYPE enumeration defines power state handler type values.
 */
typedef enum _POWER_STATE_HANDLER_TYPE
{
    PowerStateSleeping1 = 0,
    PowerStateSleeping2 = 1,
    PowerStateSleeping3 = 2,
    PowerStateSleeping4 = 3,
    PowerStateShutdownOff = 4,
    PowerStateShutdownReset = 5,
    PowerStateSleeping4Firmware = 6,
    PowerStateMaximum = 7
} POWER_STATE_HANDLER_TYPE, *PPOWER_STATE_HANDLER_TYPE;

typedef _Function_class_(ENTER_STATE_SYSTEM_HANDLER)
NTSTATUS NTAPI ENTER_STATE_SYSTEM_HANDLER(
    _In_ PVOID SystemContext
    );
typedef ENTER_STATE_SYSTEM_HANDLER* PENTER_STATE_SYSTEM_HANDLER;

typedef _Function_class_(ENTER_STATE_HANDLER)
NTSTATUS NTAPI ENTER_STATE_HANDLER(
    _In_ PVOID Context,
    _In_opt_ PENTER_STATE_SYSTEM_HANDLER SystemHandler,
    _In_ PVOID SystemContext,
    _In_ LONG NumberProcessors,
    _In_ LONG volatile* Number
    );
typedef ENTER_STATE_HANDLER* PENTER_STATE_HANDLER;

/**
 * The POWER_STATE_HANDLER structure describes power state handler.
 */
typedef struct _POWER_STATE_HANDLER
{
    POWER_STATE_HANDLER_TYPE Type;
    BOOLEAN RtcWake;
    UCHAR Spare[3];
    PENTER_STATE_HANDLER Handler;
    PVOID Context;
} POWER_STATE_HANDLER, *PPOWER_STATE_HANDLER;

typedef _Function_class_(ENTER_STATE_NOTIFY_HANDLER)
NTSTATUS NTAPI ENTER_STATE_NOTIFY_HANDLER(
    _In_ POWER_STATE_HANDLER_TYPE State,
    _In_ PVOID Context,
    _In_ BOOLEAN Entering
    );
typedef ENTER_STATE_NOTIFY_HANDLER* PENTER_STATE_NOTIFY_HANDLER;

/**
 * The POWER_STATE_NOTIFY_HANDLER structure describes power state notify handler.
 */
typedef struct _POWER_STATE_NOTIFY_HANDLER
{
    PENTER_STATE_NOTIFY_HANDLER Handler;
    PVOID Context;
} POWER_STATE_NOTIFY_HANDLER, *PPOWER_STATE_NOTIFY_HANDLER;

/**
 * The POWER_REQUEST_ACTION_INTERNAL structure describes power request action internal.
 */
typedef struct _POWER_REQUEST_ACTION_INTERNAL
{
    PVOID PowerRequestPointer;
    POWER_REQUEST_TYPE_INTERNAL RequestType;
    BOOLEAN SetAction;
} POWER_REQUEST_ACTION_INTERNAL, *PPOWER_REQUEST_ACTION_INTERNAL;

/**
 * The POWER_INFORMATION_LEVEL_INTERNAL enum selects an internal power-manager query, notification, or registration operation.
 * \remarks Pass PowerInformationInternal as the outer NtPowerInformation information level;
 * place the internal infoclass in POWER_INFORMATION_INTERNAL_HEADER.InternalType at the start
 * of the input buffer. These are operation infoclasses, not flags or system power states.
 * Even a query without additional input fields needs the complete 8-byte header.
 *
 * Use the operation-specific structure to distinguish diagnostic queries from requests
 * that change power-manager state. For example, PowerInternalS0LowPowerIdleInfo diagnoses
 * standby connectivity, PowerInternalDirtyTransitionInformation retrieves a boot snapshot,
 * and PowerInternalUserAbsencePrediction supplies a prediction rather than querying one.
 * PowerInternalReapplyBrightnessSettings is a kernel initialization notification : it publishes PopVideoInitialized = TRUE, not a brightness value.
 *
 * Selectors 0..99 use operation-specific internal power-management contracts.
 * Caller mode, access checks, accepted versions, and buffer sizes are operation-specific.
 * Internal contracts can differ on other builds; preserving an enum name does not imply
 * that its legacy numeric infoclass is implemented. In particular, legacy terminal-open
 * infoclass 9 is rejected in this implementation; its current counterpart is infoclass 4096.
 *
 * \sa POWER_INFORMATION_INTERNAL_HEADER
 * \sa PowerInternalTtmOpenTerminalV2
 */
typedef enum _POWER_INFORMATION_LEVEL_INTERNAL
{
    PowerInternalAcpiInterfaceRegister,                       // in: POWER_INTERNAL_ACPI_INTERFACE_REGISTER_INPUT (Version 1), out: POWER_INTERNAL_ACPI_INTERFACE_REGISTER_OUTPUT // kernel-mode only
    PowerInternalS0LowPowerIdleInfo,                          // in: POWER_INFORMATION_INTERNAL_HEADER, out: POWER_S0_LOW_POWER_IDLE_INFO // AoAc only
    PowerInternalReapplyBrightnessSettings,                   // in: POWER_INFORMATION_INTERNAL_HEADER, out: NULL // kernel-mode only; publishes PopVideoInitialized = TRUE
    PowerInternalUserAbsencePrediction,                       // in: POWER_USER_ABSENCE_PREDICTION, out: NULL // requires (PopSimulate & 1)
    PowerInternalUserAbsencePredictionCapability,             // in: POWER_INFORMATION_INTERNAL_HEADER, out: POWER_USER_ABSENCE_PREDICTION_CAPABILITY
    PowerInternalPoProcessorLatencyHint,                      // in: POWER_PROCESSOR_LATENCY_HINT, out: NULL // PopPowerInformationInternal (inline): PoLatencySensitivityHint(Type); user-mode callers only (kernel-mode returns STATUS_NOT_SUPPORTED)
    PowerInternalStandbyNetworkRequest,                       // in: POWER_STANDBY_NETWORK_REQUEST, out: NULL // requires membership in PopNetBIServiceSid
    PowerInternalDirtyTransitionInformation,                  // in: POWER_INFORMATION_INTERNAL_HEADER, out: POWER_INTERNAL_DIRTY_TRANSITION_INFORMATION // user-mode only
    PowerInternalSetBackgroundTaskState,                      // in: POWER_SET_BACKGROUND_TASK_STATE, out: NULL // kernel-mode only
    PowerInternalTtmOpenTerminal,                             // in: STATUS_NOT_SUPPORTED; New infoclass: PowerInternalTtmOpenTerminalV2
    PowerInternalTtmCreateTerminal,                           // in: STATUS_NOT_SUPPORTED; New infoclass: PowerInternalTtmCreateTerminalV2
    PowerInternalTtmEvacuateDevices,                          // in: STATUS_NOT_SUPPORTED; New infoclass: PowerInternalTtmEvacuateDevicesV2
    PowerInternalTtmCreateTerminalEventQueue,                 // in: STATUS_NOT_SUPPORTED; New infoclass: PowerInternalTtmCreateTerminalEventQueueV2
    PowerInternalTtmGetTerminalEvent,                         // in: STATUS_NOT_SUPPORTED; New infoclass: PowerInternalTtmGetTerminalEventV2
    PowerInternalTtmSetDefaultDeviceAssignment,               // in: STATUS_NOT_SUPPORTED; New infoclass: PowerInternalTtmSetDefaultDeviceAssignmentV2
    PowerInternalTtmAssignDevice,                             // in: STATUS_NOT_SUPPORTED; New infoclass: PowerInternalTtmAssignDeviceV2
    PowerInternalTtmSetDisplayState,                          // in: STATUS_NOT_SUPPORTED; New infoclass: PowerInternalTtmSetDisplayStateV2
    PowerInternalTtmSetDisplayTimeouts,                       // in: STATUS_NOT_SUPPORTED; New infoclass: PowerInternalTtmSetDisplayTimeoutsV2
    PowerInternalBootSessionStandbyActivationInformation,     // in: POWER_INFORMATION_INTERNAL_HEADER, out: POWER_INTERNAL_BOOT_SESSION_STANDBY_ACTIVATION_INFO // AoAc; provider-dependent payload
    PowerInternalSessionPowerState,                           // in: POWER_SESSION_POWER_STATE, out: NULL // kernel-mode only; session display notification
    PowerInternalSessionTerminalInput,                        // in: POWER_INTERNAL_TERMINAL_CORE_WINDOW_INPUT, out: NULL // kernel-mode only; terminal activity notification // 20
    PowerInternalSetWatchdog,                                 // in: POWER_INTERNAL_SET_WATCHDOG, out: optional POWER_INTERNAL_SET_WATCHDOG_OUTPUT // kernel-mode only; opaque pointer, NOT NtClose handle
    PowerInternalPhysicalPowerButtonPressInfoAtBoot,          // in: POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_INPUT, out: POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_OUTPUT
    PowerInternalExternalMonitorConnected,                    // in: POWER_INTERNAL_EXTERNAL_MONITOR_CONNECTED_INPUT, out: none // kernel-mode only; supplied output is ignored
    PowerInternalHighPrecisionBrightnessSettings,             // in: POWER_INTERNAL_HIGH_PRECISION_BRIGHTNESS_SETTINGS_INPUT, out: NULL // user-mode LocalSystem only
    PowerInternalWinrtScreenToggle,                           // in: POWER_INTERNAL_WINRT_SCREEN_TOGGLE_INPUT, out: NULL // user mode: systemManagement capability on single-session SKU; kernel also accepted
    PowerInternalPpmQosDisable,                               // in: POWER_INTERNAL_PPM_QOS_DISABLE_INPUT, out: NULL // kernel-only QoS-disable reference count; see structure for underflow/overflow behavior
    PowerInternalTransitionCheckpoint,                        // in: POWER_INTERNAL_TRANSITION_CHECKPOINT_INPUT, out: NULL // host-silo transition diagnostic checkpoint
    PowerInternalInputControllerState,                        // in: POWER_INTERNAL_INPUT_CONTROLLER_STATE, out: none (ignored) // kernel-only; updates AC and DC setting
    PowerInternalFirmwareResetReason,                         // in: POWER_INTERNAL_FIRMWARE_RESET_REASON_INPUT, out: POWER_INTERNAL_FIRMWARE_RESET_REASON_OUTPUT // user mode requires administrator context
    PowerInternalPpmSchedulerQosSupport,                      // in: POWER_INFORMATION_INTERNAL_HEADER, out: POWER_INTERNAL_PROCESSOR_QOS_SUPPORT // 30
    PowerInternalBootStatGet,                                 // in: POWER_INTERNAL_BOOTSTAT_GET_INPUT; data: Entries[].DataBuffer; optional out: ULONG[EntryCount] transfer lengths
    PowerInternalBootStatSet,                                 // in: POWER_INTERNAL_BOOTSTAT_SET_INPUT; data: Entries[].DataBuffer; optional out: ULONG[EntryCount] transfer lengths
    PowerInternalCallHasNotReturnedWatchdog,                  // in: STATUS_NOT_SUPPORTED
    PowerInternalBootStatCheckIntegrity,                      // in: POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_INPUT; result: BOOLEAN at Entries[0].DataBuffer; use NULL/0 direct output
    PowerInternalBootStatRestoreDefaults,                     // in: POWER_INTERNAL_BOOTSTAT_RESTORE_DEFAULTS_INPUT, out: NULL recommended // destructive; user mode LocalSystem
    PowerInternalHostEsStateUpdate,                           // in: POWER_INTERNAL_HOST_ENERGY_SAVER_STATE, out: NULL // user-mode LocalSystem only
    PowerInternalGetPowerActionState,                         // in: POWER_INTERNAL_GET_POWER_ACTION_STATE_INPUT, out: POWER_INTERNAL_GET_POWER_ACTION_STATE_OUTPUT (POWER_ACTION, not internal State byte)
    PowerInternalBootStatUnlock,                              // in: POWER_INTERNAL_BOOTSTAT_UNLOCK_INPUT, out: NULL recommended // releases cached boot-status file state
    PowerInternalWakeOnVoiceState,                            // in: POWER_INTERNAL_WAKE_ON_VOICE_STATE_INPUT, out: none (ignored) // user-mode only; publishes WNF state
    PowerInternalDeepSleepBlock,                              // in: POWER_INTERNAL_DEEP_SLEEP_BLOCK_INPUT, out: none (ignored) // kernel-only; Reserved+8 must be0, Block+12 // 40
    PowerInternalIsPoFxDevice,                                // in: POWER_INTERNAL_IS_POFX_DEVICE_INPUT, out: BOOLEAN // kernel-only; tests FxDevice.Plugin
    PowerInternalPowerTransitionExtensionAtBoot,              // in: POWER_INTERNAL_POWER_TRANSITION_EXTENSION_AT_BOOT_INPUT, out: POWER_INTERNAL_POWER_TRANSITION_EXTENSION_AT_BOOT_OUTPUT
    PowerInternalProcessorBrandedFrequency,                   // in: POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT_BASIC or POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT, out: POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT
    PowerInternalTimeBrokerExpirationReason,                  // in: POWER_INTERNAL_TIME_BROKER_EXPIRATION_REASON_INPUT, out: NULL // user-mode TimeBroker service SID; AoAc
    PowerInternalNotifyUserShutdownStatus,                    // in: POWER_INTERNAL_NOTIFY_USER_SHUTDOWN_STATUS_INPUT, out: none (ignored) // user LocalSystem OR testsigning+administrator
    PowerInternalPowerRequestTerminalCoreWindow,              // in: POWER_INTERNAL_POWER_REQUEST_TERMINAL_CORE_WINDOW_INPUT, out: NULL // user-mode; request handle+8, core window+16, process ID+24
    PowerInternalProcessorIdleVeto,                           // in: POWER_INTERNAL_PROCESSOR_IDLE_VETO_INPUT, out: NULL // user administrator; global processor index; sets/clears veto bit
    PowerInternalPlatformIdleVeto,                            // in: POWER_INTERNAL_PLATFORM_IDLE_VETO_INPUT, out: NULL // user administrator; sets/clears veto bit
    PowerInternalIsLongPowerButtonBugcheckEnabled,            // in: POWER_INTERNAL_LONG_POWER_BUTTON_BUGCHECK_INPUT, out: POWER_INTERNAL_LONG_POWER_BUTTON_BUGCHECK_OUTPUT
    PowerInternalAutoChkCausedReboot,                         // in: POWER_INTERNAL_AUTOCHK_CAUSED_REBOOT_INPUT, out: POWER_INTERNAL_AUTOCHK_CAUSED_REBOOT_OUTPUT // 50
    PowerInternalSetWakeAlarmOverride,                        // in: POWER_INTERNAL_SET_WAKE_ALARM_OVERRIDE_INPUT_V0 or POWER_INTERNAL_SET_WAKE_ALARM_OVERRIDE_INPUT (Version1), out: none (ignored) // user administrator
    PowerInternalReservedInformation52,                       // in: STATUS_INVALID_PARAMETER
    PowerInternalDirectedFxAddTestDevice,                     // in: POWER_INTERNAL_DIRECTED_FX_ADD_TEST_DEVICE_INPUT, out: none // user administrator; counted WCHAR device-instance path
    PowerInternalDirectedFxRemoveTestDevice,                  // in: POWER_INTERNAL_DIRECTED_FX_REMOVE_TEST_DEVICE_INPUT, out: none // user administrator; count excludes terminating NUL
    PowerInternalReservedInformation55,                       // in: STATUS_INVALID_PARAMETER
    PowerInternalDirectedFxSetMode,                           // in: POWER_INTERNAL_DIRECTED_FX_SET_MODE_INPUT, out: none (ignored) // user administrator; directed-DRIPS state bit 0 required
    PowerInternalRegisterPowerPlane,                          // in: POWER_INTERNAL_REGISTER_POWER_PLANE_INPUT, out: NULL // kernel-only; Version 0; callback at+16, NOT a size
    PowerInternalSetDirectedDripsFlags,                       // in: POWER_INTERNAL_DIRECTED_DRIPS_DEVICE_FLAGS_INPUT, out: none (ignored) // kernel-only; Flags subset of 0x3F
    PowerInternalClearDirectedDripsFlags,                     // in: POWER_INTERNAL_DIRECTED_DRIPS_DEVICE_FLAGS_INPUT, out: none (ignored) // kernel-only; Flags subset of 0x3F
    PowerInternalRetrieveHiberFileResumeContext,              // in: POWER_INTERNAL_RETRIEVE_HIBERFILE_RESUME_CONTEXT_INPUT, out: POWER_INTERNAL_RETRIEVE_HIBERFILE_RESUME_CONTEXT_OUTPUT // user administrator // 60
    PowerInternalReadHiberFilePage,                           // in: POWER_INTERNAL_READ_HIBERFILE_PAGE_INPUT, out: POWER_INTERNAL_READ_HIBERFILE_PAGE_OUTPUT // user requires testsigning+administrator; kernel also accepted
    PowerInternalLastBootSucceeded,                           // in: POWER_INTERNAL_LAST_BOOT_SUCCEEDED_INPUT, out: POWER_INTERNAL_LAST_BOOT_SUCCEEDED_OUTPUT
    PowerInternalQuerySleepStudyHelperRoutineBlock,           // in: POWER_INTERNAL_QUERY_SLEEPSTUDY_HELPER_ROUTINE_BLOCK_INPUT, out: POWER_INTERNAL_QUERY_SLEEPSTUDY_HELPER_ROUTINE_BLOCK_OUTPUT // kernel-only borrowed pointer
    PowerInternalDirectedDripsQueryCapabilities,              // in: POWER_INTERNAL_DIRECTED_DRIPS_QUERY_CAPABILITIES_INPUT, out: POWER_INTERNAL_DIRECTED_DRIPS_QUERY_CAPABILITIES_OUTPUT // user administrator; mitigation bits, not test-API availability
    PowerInternalClearConstraints,                            // in: POWER_INTERNAL_CLEAR_CONSTRAINTS_INPUT, out: none (ignored) // kernel-only; non-NULL POP_FX_DEVICE pointer, not DEVICE_OBJECT
    PowerInternalSoftParkVelocityEnabled,                     // in: STATUS_NOT_SUPPORTED
    PowerInternalQueryIntelPepCapabilities,                   // in: POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_INPUT, out: POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_OUTPUT // kernel-only; no input Version check
    PowerInternalGetSystemIdleLoopEnablement,                 // in: POWER_INTERNAL_SYSTEM_IDLE_LOOP_ENABLEMENT_INPUT, out: none (ignored) // STATUS_SUCCESS if PopIdleScanInterval!=0, else STATUS_NOT_SUPPORTED
    PowerInternalGetVmPerfControlSupport,                     // in: POWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_INPUT, out: BOOLEAN or POWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_OUTPUT
    PowerInternalGetVmPerfControlConfig,                      // in: POWER_INTERNAL_VM_PERF_CONTROL_CONFIG_INPUT_V1 (Version 0/1) or POWER_INTERNAL_VM_PERF_CONTROL_CONFIG_INPUT (Version 2), out: POWER_INTERNAL_VM_PERF_CONTROL_CONFIG_OUTPUT // 70
    PowerInternalSleepDetailedDiagUpdate,                     // in: POWER_INTERNAL_SLEEP_DETAILED_DIAG_UPDATE_INPUT, out: none (ignored) // user LocalSystem; Version ignored
    PowerInternalProcessorClassFrequencyBandsStats,           // in: POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_INPUT, out: POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_OUTPUT // cycle counters
    PowerInternalHostGlobalUserPresenceStateUpdate,           // in: POWER_INTERNAL_HOST_GLOBAL_USER_PRESENCE_STATE_UPDATE_INPUT, out: NULL/0 // both modes; raw presence DWORD; Version ignored
    PowerInternalCpuNodeIdleIntervalStats,                    // in: POWER_INTERNAL_CPU_NODE_IDLE_INTERVAL_STATS_INPUT, out: POWER_INTERNAL_IDLE_INTERVAL_PACKAGE // durations in 100-ns units
    PowerInternalClassIdleIntervalStats,                      // in: POWER_INTERNAL_CLASS_IDLE_INTERVAL_STATS_INPUT, out: POWER_INTERNAL_IDLE_INTERVAL_STATS_OUTPUT // 100-ns units
    PowerInternalCpuNodeConcurrencyStats,                     // in: POWER_INTERNAL_CPU_NODE_CONCURRENCY_STATS_INPUT, out: POWER_INTERNAL_CPU_NODE_CONCURRENCY_STATS_OUTPUT // raw QPC ticks
    PowerInternalClassConcurrencyStats,                       // in: POWER_INTERNAL_CLASS_CONCURRENCY_STATS_INPUT, out: POWER_INTERNAL_CLASS_CONCURRENCY_STATS_OUTPUT // raw QPC ticks; packed present banks, not fixed arrays
    PowerInternalQueryProcMeasurementCapabilities,            // in: POWER_INTERNAL_QUERY_MEASUREMENT_CAPABILITIES, out: POWER_INTERNAL_QUERY_MEASUREMENT_CAPABILITIES_OUTPUT // first available provider, no provider -> mask0
    PowerInternalQueryProcMeasurementValues,                  // in: POWER_INTERNAL_QUERY_MEASUREMENT_VALUES, out: POWER_INTERNAL_QUERY_MEASUREMENT_VALUES_COUNT or POWER_INTERNAL_QUERY_MEASUREMENT_VALUES_OUTPUT // nonzero mask
    PowerInternalPrepareForSystemInitiatedReboot,             // in: POWER_INTERNAL_PREPARE_FOR_SYSTEM_INITIATED_REBOOT_INPUT, out: NULL/0 // user SeShutdownPrivilege; captures state, does not reboot // 80
    PowerInternalGetAdaptiveSessionState,                     // in: POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_INPUT, out: POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_OUTPUT // kernel-only; current process session; not side-effect-free
    PowerInternalSetConsoleLockedState,                       // in: POWER_INTERNAL_SET_CONSOLE_LOCKED_STATE_INPUT, out: NULL/0 // user testsigning; power-state notification, not desktop lock
    PowerInternalOverrideSystemInitiatedRebootState,          // in: POWER_INTERNAL_OVERRIDE_SYSTEM_INITIATED_REBOOT_STATE_INPUT, out: NULL/0 // user testsigning; flag+8, target code+12 (0..6)
    PowerInternalFanImpactStats,                              // in: POWER_INTERNAL_FAN_IMPACT_STATS_INPUT, out: POWER_INTERNAL_FAN_IMPACT_STATS_OUTPUT // exactly one eligible fan; 100-ns units
    PowerInternalFanRpmBuckets,                               // in: POWER_INTERNAL_FAN_RPM_BUCKETS_INPUT, out: POWER_INTERNAL_FAN_RPM_OUTPUT // RPM bounds and zone thresholds; exactly one eligible fan // 85
    PowerInternalPowerBootAppDiagInfo,                        // in: POWER_INTERNAL_BOOTAPP_DIAGNOSTIC_INPUT, out: POWER_INTERNAL_BOOTAPP_DIAGNOSTIC // captured globals; Version ignored
    PowerInternalUnregisterShutdownNotification,              // in: POWER_INTERNAL_UNREGISTER_SHUTDOWN_NOTIFICATION_INPUT // kernel-only; unresolved dispatcher/callee buffer-size mismatch on AMD64; out NULL
    PowerInternalManageTransitionStateRecord,                 // in: POWER_INTERNAL_MANAGE_TRANSITION_STATE_RECORD_INPUT, out: none (ignored) // user LocalSystem or kernel; push/pop attribution
    PowerInternalGetAcpiTimeAndAlarmCapabilities,             // in: POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_INPUT_BASIC, out: POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_OUTPUT
    PowerInternalSuspendResumeRequest,                        // in: POWER_INTERNAL_SUSPEND_RESUME_REQUEST_INPUT, out: NULL/0 // user LocalSystem or kernel; callback-defined payload // 90
    PowerInternalEnergyEstimationInfo,                        // in: POWER_INTERNAL_ENERGY_ESTIMATION_INFO_INPUT, out: POWER_INTERNAL_ENERGY_ESTIMATION_INFO_OUTPUT // not measured energy
    PowerInternalProvSocIdentifierOperation,                  // in: POWER_INTERNAL_SOC_IDENTIFIER_OPERATION_INPUT, out: POWER_INTERNAL_SOC_IDENTIFIER_SIZE_OUTPUT (Action0) or UTF-16 bytes (Action1)
    PowerInternalGetVmPerfPrioritySupport,                    // in: POWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_INPUT, out: POWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_OUTPUT // current domain
    PowerInternalGetVmPerfPriorityConfig,                     // in: POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_INPUT, out: POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_OUTPUT // provider-defined; Version ignored
    PowerInternalNotifyWin32kPowerRequestQueued,              // in: POWER_INTERNAL_NOTIFY_WIN32K_POWER_REQUEST_INPUT, out: NULL/0 // kernel-only; index0..31; increment
    PowerInternalNotifyWin32kPowerRequestCompleted,           // in: POWER_INTERNAL_NOTIFY_WIN32K_POWER_REQUEST_INPUT, out: NULL/0 // kernel-only; index0..31; decrement
    PowerInternalQueryVelocityFeatureEnablement,              // in: POWER_INTERNAL_QUERY_VELOCITY_FEATURE_ENABLEMENT_INPUT, out: POWER_INTERNAL_QUERY_VELOCITY_FEATURE_ENABLEMENT_OUTPUT // feature enabled AND key59996711
    PowerInternalNotifyWin32kSessionConnect,                  // in: POWER_INTERNAL_NOTIFY_WIN32K_SESSION_CONNECT_INPUT, out: POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_OUTPUT // feature-gated kernel path; partial output writes
    PowerInternalSetBatteryTestExempt = 99,                   // in: POWER_INTERNAL_BATTERY_TEST_EXEMPT_INPUT; out: NULL; feature-gated, changes battery test policy
    PowerInformationInternalMaximum = 100
} POWER_INFORMATION_LEVEL_INTERNAL;

/**
 * The PowerInternalBatteryTestExempt constant selects the feature-gated battery test-exemption policy update.
 * \remarks Numeric infoclass 99 identifies the battery test-exemption operation.
 * This descriptive name is not a recovered private enum symbol. Preserve existing enum
 * values and use this explicit constant to identify the operation.
 * Requires >=12 input bytes and NULL/0 output. It changes persistent battery policy and
 * device monitoring, not a query; do not invoke merely to detect support.
 * \sa POWER_INTERNAL_BATTERY_TEST_EXEMPT_INPUT
 */
#define PowerInternalBatteryTestExempt ((POWER_INFORMATION_LEVEL_INTERNAL)99)

/**
 * The POWER_INFORMATION_INTERNAL_CONTIGUOUS_LIMIT_26100 constant is the exclusive end of the contiguous internal infoclass range accepted by the native entry.
 * \remarks The internal dispatch accepts infoclasses 0..99 and separately
 * 4096..4106 through the parent dispatch. Acceptance does not imply implementation of
 * every operation. This implementation-specific limit does not establish a universal
 * range or justify renumbering existing infoclasses.
 */
#define POWER_INFORMATION_INTERNAL_CONTIGUOUS_LIMIT_26100 100UL

// rev (AMD64)
// The TTM namespace is separate from legacy infoclasses 9..17. Do not renumber those
// entries: that would also change every subsequent implicit enumeration value.
// V2 is a descriptive alias, not a recovered symbol name. The caller needs
// SeShutdownPrivilege OR the terminalPowerManagement capability, not both.
#define PowerInternalTtmOpenTerminalV2 ((POWER_INFORMATION_LEVEL_INTERNAL)0x1000)
#define PowerInternalTtmCreateTerminalV2 ((POWER_INFORMATION_LEVEL_INTERNAL)0x1001)
#define PowerInternalTtmEvacuateDevicesV2 ((POWER_INFORMATION_LEVEL_INTERNAL)0x1002)
#define PowerInternalTtmCreateTerminalEventQueueV2 ((POWER_INFORMATION_LEVEL_INTERNAL)0x1003)
#define PowerInternalTtmGetTerminalEventV2 ((POWER_INFORMATION_LEVEL_INTERNAL)0x1004)
#define PowerInternalTtmSetDefaultDeviceAssignmentV2 ((POWER_INFORMATION_LEVEL_INTERNAL)0x1005)
#define PowerInternalTtmAssignDeviceV2 ((POWER_INFORMATION_LEVEL_INTERNAL)0x1006)
#define PowerInternalTtmSetDisplayStateV2 ((POWER_INFORMATION_LEVEL_INTERNAL)0x1007)
#define PowerInternalTtmSetDisplayTimeoutsV2 ((POWER_INFORMATION_LEVEL_INTERNAL)0x1008)

/**
 * The POWER_INTERNAL_TTM_TERMINAL_OPERATION_ACCESS constant defines the access required when a TTM operation resolves an existing terminal handle.
 * \remarks The current create/queue/default-assignment routes require the default
 * terminal (ID 0); evacuation requires a nondefault terminal. Assignment/display
 * operations accept either. These descriptive constants do not bypass object access
 * checks, TTM enablement, or the SeShutdownPrivilege OR terminalPowerManagement gate.
 */
#define POWER_INTERNAL_TTM_TERMINAL_OPERATION_ACCESS 0x00000002UL
// Access checked by the terminal-event dequeue route (standard rights required).
#define POWER_INTERNAL_TTM_EVENT_QUEUE_READ_ACCESS 0x000F0000UL

/**
 * The POWER_S0_DISCONNECTED_REASON enum identifies the reported reason for disconnected networking during S0 low-power idle.
 * \remarks Interpret this value together with the compliance and policy fields of
 * POWER_S0_LOW_POWER_IDLE_INFO when diagnosing why standby networking is disconnected.
 * It is a diagnostic classification returned by the power manager, not a value to set
 * network policy or a mask to combine with other reasons.
 *
 * \sa POWER_S0_LOW_POWER_IDLE_INFO
 */
typedef enum _POWER_S0_DISCONNECTED_REASON
{
    PoS0DisconnectedReasonNone,
    PoS0DisconnectedReasonNonCompliantNic,
    PoS0DisconnectedReasonSettingPolicy,
    PoS0DisconnectedReasonEnforceDsPolicy,
    PoS0DisconnectedReasonCsChecksFailed,
    PoS0DisconnectedReasonSmartStandby,
    PoS0DisconnectedReasonMaximum
} POWER_S0_DISCONNECTED_REASON;

/**
 * The POWER_S0_LOW_POWER_IDLE_INFO struct reports standby-networking disconnection, device compliance, and effective policy.
 * \remarks Use this diagnostic result to distinguish the reported disconnection reason
 * from the device-compliance and policy bits when investigating S0 low-power-idle networking.
 * Send an 8-byte POWER_INFORMATION_INTERNAL_HEADER with InternalType set to
 * PowerInternalS0LowPowerIdleInfo and supply this structure as the 8-byte output buffer.
 * The query does not change standby policy. The routine returns STATUS_NOT_SUPPORTED
 * when the platform does not support Always On/Always Connected (AoAc).
 *
 * DisconnectedReason is the 4-byte classification at offset 0. CsDeviceCompliance is the
 * byte at offset 4, with Storage, WiFi, Mbn, and Ethernet at bits 0, 1, 2, and 3.
 * Policy is the byte at offset 5, with DisconnectInStandby and EnforceDs at bits 0 and 1.
 * Each group exposes the same byte as AsUCHAR; its named fields are distinct bits.
 * Read the result only after a successful query.
 *
 * \sa POWER_S0_DISCONNECTED_REASON
 * \sa POWER_INFORMATION_INTERNAL_HEADER
 */
typedef struct _POWER_S0_LOW_POWER_IDLE_INFO
{
    POWER_S0_DISCONNECTED_REASON DisconnectedReason;
    union
    {
        struct
        {
            BOOLEAN Storage : 1;
            BOOLEAN WiFi : 1;
            BOOLEAN Mbn : 1;
            BOOLEAN Ethernet : 1;
            BOOLEAN Reserved : 4;
        };
        UCHAR AsUCHAR;
    } CsDeviceCompliance;
    union
    {
        struct
        {
            BOOLEAN DisconnectInStandby : 1;
            BOOLEAN EnforceDs : 1;
            BOOLEAN Reserved : 6;
        };
        UCHAR AsUCHAR;
    } Policy;
} POWER_S0_LOW_POWER_IDLE_INFO, *PPOWER_S0_LOW_POWER_IDLE_INFO;

/**
 * The POWER_INFORMATION_INTERNAL_HEADER struct identifies and versions the input payload for an internal power operation.
 * \remarks This 8-byte prefix is required when calling NtPowerInformation with outer
 * information level PowerInformationInternal. Set InternalType to the requested internal
 * operation, append any operation-specific fields, and include the prefix in InputBufferLength.
 * For header-only queries, pass sizeof(POWER_INFORMATION_INTERNAL_HEADER), not a NULL input.
 * A query result does not automatically contain this prefix; follow its output declaration.
 *
 * Version is interpreted by the selected operation, not a universal size or version check.
 * Use the documented version for that operation: ACPI interface registration requires 1
 * A request that does not check Version does not establish the
 * accepted version contract for other infoclasses or Windows builds.
 *
 * \sa POWER_INFORMATION_LEVEL_INTERNAL
 * \sa POWER_INTERNAL_ACPI_INTERFACE_REGISTER_INPUT
 */
typedef struct _POWER_INFORMATION_INTERNAL_HEADER
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INFORMATION_INTERNAL_HEADER, *PPOWER_INFORMATION_INTERNAL_HEADER;

// rev (AMD64)
/**
 * The POWER_INTERNAL_PLATFORM_REQUEST_HANDLER callback type defines the one-request-pointer callback used by the platform power interface.
 *
 * \param[in,out] Request The operation-specific request block exchanged by the kernel
 * and registered platform component. Its payload layout is not established here.
 * \return The operation's NTSTATUS result; interpretation depends on the request contract.
 * \remarks Use this typedef for the registration input callback and returned kernel
 * handler, not as a declaration of a user-mode callable export. The AMD64 call sites
 * establish one pointer argument and an NTSTATUS return. Verify the request payload
 * separately before invoking the handler; this typedef alone does not describe it.
 *
 * \sa POWER_INTERNAL_ACPI_INTERFACE_REGISTER_INPUT
 * \sa POWER_INTERNAL_ACPI_INTERFACE_REGISTER_OUTPUT
 */
typedef _Function_class_(POWER_INTERNAL_PLATFORM_REQUEST_HANDLER)
NTSTATUS NTAPI POWER_INTERNAL_PLATFORM_REQUEST_HANDLER(
    _Inout_ PVOID Request
    );
typedef POWER_INTERNAL_PLATFORM_REQUEST_HANDLER *PPOWER_INTERNAL_PLATFORM_REQUEST_HANDLER;

#define POWER_INTERNAL_ACPI_INTERFACE_REGISTER_VERSION 1

// rev
/**
 * The POWER_INTERNAL_ACPI_INTERFACE_REGISTER_INPUT struct registers the kernel platform request interface used for power-management communication.
 * \remarks This is a kernel-component interface-registration payload, not an application
 * power setting or a request to create a user-mode handle. Select PowerInternalAcpiInterfaceRegister,
 * set Header.Version to POWER_INTERNAL_ACPI_INTERFACE_REGISTER_VERSION (1), provide at least
 * 0x20 input bytes, and provide a 0x10-byte POWER_INTERNAL_ACPI_INTERFACE_REGISTER_OUTPUT.
 * The dispatcher denies user-mode callers with STATUS_ACCESS_DENIED.
 *
 * RequestHandler receives a request-block pointer; Context is forwarded in platform requests.
 * RequestHandler and ReferenceObject must be non-NULL. The kernel references ReferenceObject;
 * it is a kernel object pointer, not an object-manager handle. Its subtype is not established.
 * Registration is single-instance: a duplicate returns STATUS_UNSUCCESSFUL, while an unsupported
 * version returns STATUS_REVISION_MISMATCH. Do not infer an unregister or lifetime contract
 * from this registration path alone.
 *
 * The AMD64 typed overlay places the callback at +0x08, context at +0x10, and referenced
 * object at +0x18. Data retains the original 24-byte raw payload view. Request-block payloads
 * remain opaque; do not interpret them without an operation-specific layout.
 *
 * \sa POWER_INTERNAL_ACPI_INTERFACE_REGISTER_OUTPUT
 * \sa POWER_INTERNAL_PLATFORM_REQUEST_HANDLER
 */
typedef struct _POWER_INTERNAL_ACPI_INTERFACE_REGISTER_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    union
    {
        UCHAR Data[24];
#ifdef _WIN64
        struct
        {
            PPOWER_INTERNAL_PLATFORM_REQUEST_HANDLER RequestHandler;
            PVOID Context; // forwarded in platform requests
            PVOID ReferenceObject; // referenced with tag 'MXFP'
        };
#endif
    };
} POWER_INTERNAL_ACPI_INTERFACE_REGISTER_INPUT, *PPOWER_INTERNAL_ACPI_INTERFACE_REGISTER_INPUT;

// rev
/**
 * The POWER_INTERNAL_ACPI_INTERFACE_REGISTER_OUTPUT struct returns the kernel's platform request handler after successful interface registration.
 * \remarks A registering kernel component receives PoFxPlatformRequestHandler and a NULL
 * Context in this 0x10-byte AMD64 result. The handler is an interface entry point, not a
 * registration handle. Do not pass its address to NtClose or treat it as an object handle.
 * Use the result only after a successful PowerInternalAcpiInterfaceRegister request and only
 * with a separately established request-block contract.
 *
 * \sa POWER_INTERNAL_ACPI_INTERFACE_REGISTER_INPUT
 * \sa POWER_INTERNAL_PLATFORM_REQUEST_HANDLER
 */
typedef struct _POWER_INTERNAL_ACPI_INTERFACE_REGISTER_OUTPUT
{
    PPOWER_INTERNAL_PLATFORM_REQUEST_HANDLER RequestHandler;
    PVOID Context;
} POWER_INTERNAL_ACPI_INTERFACE_REGISTER_OUTPUT, *PPOWER_INTERNAL_ACPI_INTERFACE_REGISTER_OUTPUT;

#ifdef _WIN64
#endif

/**
 * The POWER_USER_ABSENCE_PREDICTION struct supplies a predicted user-return time to the power manager's presence-prediction logic.
 * \remarks Use this input for the simulation-enabled prediction path, not to retrieve a
 * prediction or to schedule an application timer. Set Header.InternalType to
 * PowerInternalUserAbsencePrediction, pass at least 0x10 input bytes, and use NULL/0 for output.
 * The 8-byte ReturnTime field at offset 8 is forwarded to PopUpdateSmartUserPresencePredictions.
 * Forwarding ReturnTime does not establish public time units or a scheduling guarantee.
 *
 * The routine requires (PopSimulate & 1); otherwise it returns STATUS_INVALID_PARAMETER.
 * An output buffer is also rejected. This is not an unrestricted production prediction setter.
 *
 * \sa POWER_USER_ABSENCE_PREDICTION_CAPABILITY
 */
typedef struct _POWER_USER_ABSENCE_PREDICTION
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    LARGE_INTEGER ReturnTime;
} POWER_USER_ABSENCE_PREDICTION, *PPOWER_USER_ABSENCE_PREDICTION;

/**
 * The POWER_USER_ABSENCE_PREDICTION_CAPABILITY struct reports the platform/SKU condition used by the absence-prediction capability query.
 * \remarks This query returns the capability condition for the internal presence-prediction
 * path. Send an 8-byte POWER_INFORMATION_INTERNAL_HEADER
 * with InternalType set to PowerInternalUserAbsencePredictionCapability and provide this
 * 1-byte structure as output. Read AbsencePredictionCapability only on success.
 *
 * The capability is TRUE only when both SSHSupportIsPlatformAoAc and RtlIsMultiSessionSku
 * return TRUE. A TRUE result is not permission to inject a prediction: the separate
 * PowerInternalUserAbsencePrediction request still requires its simulation condition.
 *
 * \sa POWER_USER_ABSENCE_PREDICTION
 */
typedef struct _POWER_USER_ABSENCE_PREDICTION_CAPABILITY
{
    BOOLEAN AbsencePredictionCapability;
} POWER_USER_ABSENCE_PREDICTION_CAPABILITY, *PPOWER_USER_ABSENCE_PREDICTION_CAPABILITY;

// rev
/**
 * The POWER_PROCESSOR_LATENCY_HINT struct supplies the processor latency-sensitivity hint consumed by PoLatencySensitivityHint.
 * \remarks Use this payload when communicating a latency-sensitivity hint through the
 * internal power interface. It is a hint, not a measured latency result or an explicit
 * processor-frequency limit. Set PowerInformationInternalHeader.InternalType to
 * PowerInternalPoProcessorLatencyHint, pass at least 0x0C input bytes, and use NULL/0 for output.
 * The ULONG Type at offset 8 is forwarded to PoLatencySensitivityHint.
 *
 * The route accepts user-mode callers and rejects kernel-mode callers with
 * STATUS_NOT_SUPPORTED. This declaration does not define an unverified enumeration or
 * promise a particular scheduler/performance outcome for Type values.
 *
 * \sa POWER_INFORMATION_INTERNAL_HEADER
 */
typedef struct _POWER_PROCESSOR_LATENCY_HINT
{
    POWER_INFORMATION_INTERNAL_HEADER PowerInformationInternalHeader;
    ULONG Type;
} POWER_PROCESSOR_LATENCY_HINT, *PPOWER_PROCESSOR_LATENCY_HINT;

// rev
/**
 * The POWER_STANDBY_NETWORK_REQUEST struct updates the service-authorized standby-network request's active state.
 * \remarks Use this input to communicate an active/inactive request to the standby-network
 * power-management path. It is not an output status query or a general-purpose adapter
 * configuration interface. Set PowerInformationInternalHeader.InternalType to
 * PowerInternalStandbyNetworkRequest, provide at least 0x0C input bytes, and use NULL/0 for output.
 * The handler consumes the Active byte at offset 8.
 *
 * PopNetUpdateStandbyRequest requires token membership in PopNetBIServiceSid. A nonmember
 * receives STATUS_ACCESS_DENIED; an unavailable service SID yields STATUS_UNSUCCESSFUL.
 * Repeating the current active/inactive state returns STATUS_INVALID_PARAMETER, so this
 * request must not be assumed to be an idempotent setter.
 *
 * \sa POWER_INFORMATION_INTERNAL_HEADER
 */
typedef struct _POWER_STANDBY_NETWORK_REQUEST
{
    POWER_INFORMATION_INTERNAL_HEADER PowerInformationInternalHeader;
    BOOLEAN Active;
} POWER_STANDBY_NETWORK_REQUEST, *PPOWER_STANDBY_NETWORK_REQUEST;

// rev (AMD64)
/**
 * The POWER_INTERNAL_DIRTY_TRANSITION_INFORMATION struct captures the power-transition-at-boot record for post-boot diagnostics.
 * \remarks PowerInternalDirtyTransitionInformation reports whether the captured record
 * marks a dirty transition during a previous power transition.
 * Send an 8-byte POWER_INFORMATION_INTERNAL_HEADER and provide the full 0x20-byte output.
 * The dispatcher supports this route for user-mode callers and returns STATUS_NOT_SUPPORTED
 * for kernel-mode callers. This query reads a snapshot; it does not clear or acknowledge it.
 *
 * Data contains all 0x20 bytes copied from PopBsdPowerTransitionAtBoot. This is NOT a BOOLEAN
 * result: the flag is bit 0 at byte offset 8, not the first byte of the buffer.
 * Use POWER_INTERNAL_DIRTY_TRANSITION_IS_DIRTY only after a successful complete query.
 * Other field names and semantics remain unestablished; retain the raw bytes rather than
 * inferring a shutdown cause or decoding an undocumented layout.
 *
 * \sa POWER_INTERNAL_DIRTY_TRANSITION_IS_DIRTY
 */
typedef struct _POWER_INTERNAL_DIRTY_TRANSITION_INFORMATION
{
    UCHAR Data[0x20];
} POWER_INTERNAL_DIRTY_TRANSITION_INFORMATION, *PPOWER_INTERNAL_DIRTY_TRANSITION_INFORMATION;

/**
 * The POWER_INTERNAL_DIRTY_TRANSITION_IS_DIRTY macro tests the dirty-transition flag in a captured boot record.
 *
 * \param[in] Information Non-NULL pointer to a complete, successfully queried
 * POWER_INTERNAL_DIRTY_TRANSITION_INFORMATION result. Evaluated once.
 * \return Nonzero if bit 0 of byte 8 is set; zero if it is clear.
 * \remarks This helper only decodes the captured bytes. It does not issue a query,
 * validate the buffer, clear the record, or establish the cause of the transition.
 *
 * \sa POWER_INTERNAL_DIRTY_TRANSITION_INFORMATION
 */
#define POWER_INTERNAL_DIRTY_TRANSITION_IS_DIRTY(Information) \
    ((((Information)->Data[8]) & 0x01) != 0)

// rev
/**
 * The POWER_SET_BACKGROUND_TASK_STATE struct notifies the kernel's background-cooling status path whether background work is engaged.
 * \remarks Use this kernel-mode notification to publish background engagement to the
 * power-management cooling-status path. It does not create, suspend, or enumerate scheduled
 * tasks and does not return background-task status. Set
 * PowerInformationInternalHeader.InternalType to PowerInternalSetBackgroundTaskState,
 * supply sizeof(POWER_SET_BACKGROUND_TASK_STATE) (0x0C), and use NULL/0 for output.
 * User-mode callers receive STATUS_ACCESS_DENIED.
 *
 * The handler reads the Engaged byte at offset 8 and PopUpdateBackgroundCoolingStatus
 * publishes its nonzero/zero value as a 4-byte BOOL. That BOOL is notification data, not
 * NtPowerInformation output. Although the entry checks only for 8 input bytes, it still
 * reads Engaged; an 8-byte header alone is not a complete input.
 *
 * \sa POWER_INFORMATION_INTERNAL_HEADER
 */
typedef struct _POWER_SET_BACKGROUND_TASK_STATE
{
    POWER_INFORMATION_INTERNAL_HEADER PowerInformationInternalHeader;
    BOOLEAN Engaged;
} POWER_SET_BACKGROUND_TASK_STATE, *PPOWER_SET_BACKGROUND_TASK_STATE;

// rev
/**
 * The POWER_INTERNAL_BOOT_SESSION_STANDBY_ACTIVATION_INFO struct captures boot-session standby activation diagnostics in the complete returned buffer.
 * \remarks Use infoclass 18 with an 8-byte POWER_INFORMATION_INTERNAL_HEADER and at
 * least 0x400 output bytes. The kernel returns STATUS_NOT_SUPPORTED without
 * AoAc and STATUS_NOT_IMPLEMENTED when its activation-information provider is absent.
 * A previous 404-byte ULONG/98-entry declaration did not match this 1024-byte result.
 *
 * The kernel passes provider destinations at offsets 0, 0x10, and 0x18, then writes
 * an 8-byte PpmQueryPlatformStateResidency result at +8. That residency query can return
 * 0xFFFFFFFFFFFFFFFF when unavailable. Provider-owned field widths, units, and client
 * count are not established by the kernel-side callback invocation; preserve those bytes
 * rather than treating them as a array of 98 or 125 durations. DripsTotalTime
 * retains its descriptive name, not a promise of a particular unit.
 * \sa POWER_INFORMATION_INTERNAL_HEADER
 */
typedef struct _POWER_INTERNAL_BOOT_SESSION_STANDBY_ACTIVATION_INFO
{
    union
    {
        UCHAR Buffer[0x400];
        struct
        {
            UCHAR ProviderData0[8]; // provider destination at +0x00; layout not established
            ULONGLONG DripsTotalTime; // PpmQueryPlatformStateResidency result, units provider-dependent
            UCHAR ProviderData1[0x3F0]; // provider destinations at +0x10 and +0x18
        };
    };
} POWER_INTERNAL_BOOT_SESSION_STANDBY_ACTIVATION_INFO, *PPOWER_INTERNAL_BOOT_SESSION_STANDBY_ACTIVATION_INFO;

// rev
/**
 * The POWER_SESSION_POWER_STATE struct notifies the power manager of a session's display power state.
 * \remarks Kernel-mode infoclass 19 requires exactly 0x14 input bytes and NULL/0 output.
 * Use SessionId at +8, On at +0x0C, IsConsole at +0x0D, and RequestReason at +0x10.
 * The handler notifies TTM; when IsConsole is nonzero it also updates console/display
 * status and reports display-powered-on for On != 0. RequestReason is included in the
 * diagnostic trace. This is an incoming notification, not a query or a user-mode display
 * switch. User-mode callers receive STATUS_ACCESS_DENIED.
 * \sa POWER_INTERNAL_TTM_SET_DISPLAY_STATE_INPUT
 */
typedef struct _POWER_SESSION_POWER_STATE
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONG SessionId;
    BOOLEAN On;
    BOOLEAN IsConsole;
    POWER_MONITOR_REQUEST_REASON RequestReason;
} POWER_SESSION_POWER_STATE, *PPOWER_SESSION_POWER_STATE;

// rev (AMD64)
/**
 * The POWER_INTERNAL_WATCHDOG_CALLBACK callback type receives a kernel power-watchdog expiration instead of the default bugcheck action.
 * \param[in] Context Opaque context supplied in the watchdog parameters.
 * \param[in] BugCheckCode Diagnostic code supplied in the watchdog parameters.
 * \param[in] Parameter1 First resolved diagnostic value.
 * \param[in] Parameter2 Second resolved diagnostic value.
 * \param[in] Parameter3 Third resolved diagnostic value.
 * \param[in] Parameter4 Fourth resolved diagnostic value.
 * \remarks The expiration worker passes these six arguments and ignores the return
 * register; this typedef models that as VOID. Kernel-only; not a user-mode callback.
 */
typedef _Function_class_(POWER_INTERNAL_WATCHDOG_CALLBACK)
VOID NTAPI POWER_INTERNAL_WATCHDOG_CALLBACK(
    _In_opt_ PVOID Context,
    _In_ ULONG BugCheckCode,
    _In_ ULONG_PTR Parameter1,
    _In_ ULONG_PTR Parameter2,
    _In_ ULONG_PTR Parameter3,
    _In_ ULONG_PTR Parameter4
    );
typedef POWER_INTERNAL_WATCHDOG_CALLBACK *PPOWER_INTERNAL_WATCHDOG_CALLBACK;

#ifdef _WIN64
/**
 * The POWER_INTERNAL_WATCHDOG_PARAMETERS struct describes a kernel power watchdog's timeout and expiration diagnostics.
 * \remarks Embedded at offset 0x10 of POWER_INTERNAL_SET_WATCHDOG on AMD64.
 * TimeoutMilliseconds is multiplied by 10000 to obtain 100-ns timer units; zero
 * cancels/disarms an existing watchdog. BugCheckParameters are values unless the
 * corresponding DereferenceParameter byte is nonzero, in which case the worker
 * reads one pointer-sized value at that kernel address. Such storage must remain
 * valid until expiration/cancellation. A NULL Callback selects KeBugCheckEx.
 * Unknown04, Unknown0C, and Unknown34 are not established flags; initialize to zero.
 * This describes the kernel ABI for auditing, not an application watchdog service.
 */
typedef struct _POWER_INTERNAL_WATCHDOG_PARAMETERS
{
    ULONG TimeoutMilliseconds;
    ULONG Unknown04;
    ULONG BugCheckCode;
    ULONG Unknown0C;
    ULONG_PTR BugCheckParameters[4];
    BOOLEAN DereferenceParameter[4];
    ULONG Unknown34;
    PPOWER_INTERNAL_WATCHDOG_CALLBACK Callback;
    PVOID Context;
} POWER_INTERNAL_WATCHDOG_PARAMETERS, *PPOWER_INTERNAL_WATCHDOG_PARAMETERS;

#endif

// rev
/**
 * The POWER_INTERNAL_SET_WATCHDOG struct defines the creates, updates, disarms, or deletes a kernel power-transition watchdog.
 * \remarks Kernel-mode infoclass 21 takes at least 0x60 input bytes on AMD64. The opaque
 * WatchdogHandle is a kernel allocation pointer, not an object-manager HANDLE. A NULL
 * pointer requests creation and requires an output buffer; an existing pointer permits
 * NULL/0 output. If output is requested provide at least 8 bytes and check the returned
 * pointer even when NTSTATUS succeeds, since allocation failure can yield NULL.
 *
 * The 0x48-byte parameter block starts at +0x10. DeleteWatchdog at +0x58 deletes the
 * watchdog and yields NULL; a zero timeout disarms it without deletion. Never use NtClose
 * on this pointer. Expiration can invoke a kernel callback or bugcheck the machine; this
 * is for kernel power-transition supervision, not an application timer. User mode is denied.
 * The raw Parameters view is preserved; the typed overlay describes the AMD64 layout.
 * \sa POWER_INTERNAL_WATCHDOG_PARAMETERS
 * \sa POWER_INTERNAL_SET_WATCHDOG_OUTPUT
 */
typedef struct _POWER_INTERNAL_SET_WATCHDOG
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    PVOID WatchdogHandle; // opaque kernel allocation, NOT an object-manager handle
    union
    {
        UCHAR Parameters[0x48];
#ifdef _WIN64
        POWER_INTERNAL_WATCHDOG_PARAMETERS WatchdogParameters;
#endif
    };
    BOOLEAN DeleteWatchdog;
    UCHAR Reserved[7];
} POWER_INTERNAL_SET_WATCHDOG, *PPOWER_INTERNAL_SET_WATCHDOG;

#ifdef _WIN64
#endif

/**
 * The POWER_INTERNAL_SET_WATCHDOG_OUTPUT struct receives the opaque kernel watchdog pointer after create/update/delete.
 * \remarks The AMD64 route returns 8 bytes when output is requested. This is not
 * an object-manager handle and must not be passed to NtClose. Reuse the pointer
 * only with PowerInternalSetWatchdog; deletion returns NULL. A NULL pointer can
 * also indicate allocation failure even when the outer request returns success.
 * \sa POWER_INTERNAL_SET_WATCHDOG
 */
typedef struct _POWER_INTERNAL_SET_WATCHDOG_OUTPUT
{
    PVOID WatchdogHandle;
} POWER_INTERNAL_SET_WATCHDOG_OUTPUT, *PPOWER_INTERNAL_SET_WATCHDOG_OUTPUT;

#ifdef _WIN64
#endif

// rev
/**
 * The POWER_INTERNAL_TERMINAL_CORE_WINDOW_INPUT struct reports input activity to reset a session terminal's inactivity timers.
 * \remarks Kernel-mode infoclass 20 requires exactly 0x14 input bytes and NULL/0 output.
 * SessionId at +8 and TerminalId at +0x0C locate the terminal; Wake at +0x10 is consumed
 * as a Boolean by TtmiResetTerminalTimeouts, not an enum of input-device types. Nonzero
 * allows its wake path; the exact action also depends on the terminal's current state.
 * The dispatcher returns success after the
 * notification call, even if the downstream session/terminal lookup finds no target.
 * This is not input injection and does not carry a window handle. User mode is denied.
 * \sa POWER_SESSION_POWER_STATE
 */
typedef struct _POWER_INTERNAL_TERMINAL_CORE_WINDOW_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG SessionId;
    ULONG TerminalId;
    BOOLEAN Wake; // nonzero permits the wake path while resetting terminal idle timers
} POWER_INTERNAL_TERMINAL_CORE_WINDOW_INPUT, *PPOWER_INTERNAL_TERMINAL_CORE_WINDOW_INPUT;

// rev
/**
 * The POWER_INTERNAL_TTM_OPEN_TERMINAL_INPUT struct requests a handle to the current session's default power-management terminal.
 * \remarks Use this request to obtain the terminal object for session-specific terminal
 * power-management operations; it is not a console or terminal-emulator handle. To open it,
 * set Header.InternalType to PowerInternalTtmOpenTerminalV2 (4096 / 0x1000), pass at least
 * 0x0C input bytes, and provide POWER_INTERNAL_TTM_TERMINAL_HANDLE_OUTPUT with at least
 * 8 output bytes on AMD64. DesiredAccess at offset 8 controls the requested object access.
 *
 * Legacy infoclass PowerInternalTtmOpenTerminal (9) returns STATUS_INVALID_PARAMETER in this
 * build. The replacement route requires SeShutdownPrivilege OR terminalPowerManagement
 * capability, plus TTM enablement; disabled TTM returns STATUS_NOT_SUPPORTED.
 * Header.Version is not checked by the open route. Read the returned handle only
 * on success and close it with NtClose when finished.
 *
 * \sa PowerInternalTtmOpenTerminalV2
 * \sa POWER_INTERNAL_TTM_TERMINAL_HANDLE_OUTPUT
 */
typedef struct _POWER_INTERNAL_TTM_OPEN_TERMINAL_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ACCESS_MASK DesiredAccess;
} POWER_INTERNAL_TTM_OPEN_TERMINAL_INPUT, *PPOWER_INTERNAL_TTM_OPEN_TERMINAL_INPUT;

// rev
/**
 * The POWER_INTERNAL_TTM_TERMINAL_HANDLE_OUTPUT struct receives the object-manager handle opened for the current session's default terminal.
 * \remarks The terminal-open route requires at least 8 output bytes on AMD64. On success,
 * TerminalHandle identifies the power-management terminal opened with the requested
 * DesiredAccess; it is not a console handle or a pointer to a terminal structure.
 * The caller owns the returned handle and must call NtClose when it is no longer needed.
 * After a failed request, this field does not provide a new handle to use or close.
 *
 * \sa POWER_INTERNAL_TTM_OPEN_TERMINAL_INPUT
 */
typedef struct _POWER_INTERNAL_TTM_TERMINAL_HANDLE_OUTPUT
{
    HANDLE TerminalHandle;
} POWER_INTERNAL_TTM_TERMINAL_HANDLE_OUTPUT, *PPOWER_INTERNAL_TTM_TERMINAL_HANDLE_OUTPUT;

#ifdef _WIN64
#endif

// rev
/**
 * The POWER_INTERNAL_TTM_CREATE_TERMINAL_INPUT struct creates an additional power-management terminal in an existing session.
 * \remarks Select PowerInternalTtmCreateTerminalV2 (4097), not legacy infoclass 10.
 * Supply at least 0x18 input bytes and 0x10 output bytes. ParentTerminalHandle must name
 * that session's default terminal (ID 0), with access 0x00000002. It selects the session;
 * this path does not establish a parent/child terminal hierarchy. DesiredAccess at +8
 * is used to create the returned handle.
 * Initialize Reserved to zero. Close the returned TerminalHandle with NtClose.
 * Use outer PowerInformationInternal; the V2 name is a descriptive alias, not a recovered
 * symbol. The route requires SeShutdownPrivilege OR terminalPowerManagement capability
 * and TTM enabled. Header.Version is not checked by the dispatch/handler path.
 * \sa POWER_INTERNAL_TTM_CREATE_TERMINAL_OUTPUT
 */
typedef struct _POWER_INTERNAL_TTM_CREATE_TERMINAL_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    union
    {
        ACCESS_MASK DesiredAccess; // passed to ObInsertObjectEx
    };
    ULONG Reserved;
    HANDLE ParentTerminalHandle; // handle to the default terminal (ID 0) of the target session
} POWER_INTERNAL_TTM_CREATE_TERMINAL_INPUT, *PPOWER_INTERNAL_TTM_CREATE_TERMINAL_INPUT;

#ifdef _WIN64
#endif

// rev
/**
 * The POWER_INTERNAL_TTM_CREATE_TERMINAL_OUTPUT struct receives the new terminal's handle and session-local numeric identifier.
 * \remarks Selector 4097 returns 0x10 bytes on AMD64: handle at +0, TerminalId at +8,
 * and a zero-initialized trailing DWORD. Use the ID when correlating terminal events;
 * use the owned handle for operations and release it with NtClose. Do not use either
 * field as a new result on failure. TerminalId is not a process or Windows session ID.
 * \sa POWER_INTERNAL_TTM_CREATE_TERMINAL_INPUT
 */
typedef struct _POWER_INTERNAL_TTM_CREATE_TERMINAL_OUTPUT
{
    HANDLE TerminalHandle;
    ULONG TerminalId;
    ULONG Reserved;
} POWER_INTERNAL_TTM_CREATE_TERMINAL_OUTPUT, *PPOWER_INTERNAL_TTM_CREATE_TERMINAL_OUTPUT;

#ifdef _WIN64
#endif

// rev
/**
 * The POWER_INTERNAL_TTM_EVACUATE_DEVICES_INPUT struct removes device assignments from a nondefault power-management terminal.
 * \remarks Select PowerInternalTtmEvacuateDevicesV2 (4098), not legacy infoclass 11.
 * Supply at least 0x10 input bytes and NULL/0 output. TerminalHandle must have access
 * 0x00000002 and identify a nondefault terminal. Matching assignments are marked
 * unassigned and work is scheduled if anything changed; success does not establish
 * that asynchronous device processing is already complete. This does not close the handle.
 * Use outer PowerInformationInternal; the V2 name is a descriptive alias, not a recovered
 * symbol. The route requires SeShutdownPrivilege OR terminalPowerManagement capability
 * and TTM enabled. Header.Version is not checked by the dispatch/handler path.
 * \sa POWER_INTERNAL_TTM_ASSIGN_DEVICE_INPUT
 */
typedef struct _POWER_INTERNAL_TTM_EVACUATE_DEVICES_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    HANDLE TerminalHandle;
} POWER_INTERNAL_TTM_EVACUATE_DEVICES_INPUT, *PPOWER_INTERNAL_TTM_EVACUATE_DEVICES_INPUT;

#ifdef _WIN64
#endif

// rev
/**
 * The POWER_INTERNAL_TTM_CREATE_EVENT_QUEUE_INPUT struct creates a session's terminal-event queue and seeds its initial enumeration.
 * \remarks Select PowerInternalTtmCreateTerminalEventQueueV2 (4099), not legacy infoclass 12.
 * Supply at least 0x10 input bytes and 8 output bytes. TerminalHandle must name the
 * session's default terminal (ID 0), with access 0x00000002. The queue receives device
 * and display-required-request enumeration followed by an enumeration-complete event.
 * Use its returned handle to dequeue events; close it with NtClose when finished.
 * Use outer PowerInformationInternal; the V2 name is a descriptive alias, not a recovered
 * symbol. The route requires SeShutdownPrivilege OR terminalPowerManagement capability
 * and TTM enabled. Header.Version is not checked by the dispatch/handler path.
 * \sa POWER_INTERNAL_TTM_EVENT_QUEUE_HANDLE_OUTPUT
 * \sa POWER_INTERNAL_TTM_GET_TERMINAL_EVENT_INPUT
 */
typedef struct _POWER_INTERNAL_TTM_CREATE_EVENT_QUEUE_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    HANDLE TerminalHandle;
} POWER_INTERNAL_TTM_CREATE_EVENT_QUEUE_INPUT, *PPOWER_INTERNAL_TTM_CREATE_EVENT_QUEUE_INPUT;

#ifdef _WIN64
#endif

// rev
/**
 * The POWER_INTERNAL_TTM_EVENT_QUEUE_HANDLE_OUTPUT struct receives the object-manager handle for a newly created terminal-event queue.
 * \remarks The AMD64 create-queue route returns 8 bytes. The caller owns EventQueueHandle
 * and must close it with NtClose. It is a queue handle, not a terminal handle; retain it
 * for PowerInternalTtmGetTerminalEventV2 rather than passing it to terminal setters.
 * Read it only after successful creation.
 * \sa POWER_INTERNAL_TTM_CREATE_EVENT_QUEUE_INPUT
 */
typedef struct _POWER_INTERNAL_TTM_EVENT_QUEUE_HANDLE_OUTPUT
{
    HANDLE EventQueueHandle;
} POWER_INTERNAL_TTM_EVENT_QUEUE_HANDLE_OUTPUT, *PPOWER_INTERNAL_TTM_EVENT_QUEUE_HANDLE_OUTPUT;

#ifdef _WIN64
#endif

// rev
/**
 * The POWER_INTERNAL_TTM_GET_TERMINAL_EVENT_INPUT struct removes the next queued terminal event for session/device-state tracking.
 * \remarks Select PowerInternalTtmGetTerminalEventV2 (4100), not legacy infoclass 13.
 * Supply at least 0x10 input bytes and 0x220 output bytes. EventQueueHandle is checked
 * against the queue object type with access 0x000F0000. This is a nonblocking dequeue:
 * an empty queue returns STATUS_NO_MORE_ENTRIES; an inactive queue returns 0xC0000700.
 * Successful retrieval consumes one event; an empty result is not a zero-filled event.
 * Use outer PowerInformationInternal; the V2 name is a descriptive alias, not a recovered
 * symbol. The route requires SeShutdownPrivilege OR terminalPowerManagement capability
 * and TTM enabled. Header.Version is not checked by the dispatch/handler path.
 * \sa POWER_INTERNAL_TTM_TERMINAL_EVENT
 */
typedef struct _POWER_INTERNAL_TTM_GET_TERMINAL_EVENT_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    HANDLE EventQueueHandle;
} POWER_INTERNAL_TTM_GET_TERMINAL_EVENT_INPUT, *PPOWER_INTERNAL_TTM_GET_TERMINAL_EVENT_INPUT;

#ifdef _WIN64
#endif

#define POWER_INTERNAL_TTM_TERMINAL_EVENT_TYPE_DEVICE_ENUMERATED 0
#define POWER_INTERNAL_TTM_TERMINAL_EVENT_TYPE_ENUMERATION_COMPLETE 1
#define POWER_INTERNAL_TTM_TERMINAL_EVENT_TYPE_DISPLAY_STATE_CHANGED 2
#define POWER_INTERNAL_TTM_TERMINAL_EVENT_TYPE_DEVICE_ARRIVED 3
#define POWER_INTERNAL_TTM_TERMINAL_EVENT_TYPE_DEVICE_ASSIGNED 4
#define POWER_INTERNAL_TTM_TERMINAL_EVENT_TYPE_DEVICE_DEPARTED 5
#define POWER_INTERNAL_TTM_TERMINAL_EVENT_TYPE_DISPLAY_REQUIRED_POWER_REQUEST_UPDATED 6

// rev (producer layout and TraceLogging field names, AMD64)
/**
 * The POWER_INTERNAL_TTM_DEVICE_EVENT_DATA struct describes a device in an enumeration (type 0) or arrival (type 3) event.
 * \remarks Use DeviceId for later assignment; AssignedTerminalId identifies its
 * current terminal. Provider and ProviderSpecificType are raw provider identifiers,
 * not documented enumeration values. Identity is a 260-WCHAR copied string.
 * This 0x218-byte payload starts at offset 8 in the full terminal event.
 */
typedef struct _POWER_INTERNAL_TTM_DEVICE_EVENT_DATA
{
    ULONG DeviceId;
    ULONG AssignedTerminalId;
    ULONG Provider;
    ULONG ProviderSpecificType;
    WCHAR Identity[260];
} POWER_INTERNAL_TTM_DEVICE_EVENT_DATA, *PPOWER_INTERNAL_TTM_DEVICE_EVENT_DATA;

/**
 * The POWER_INTERNAL_TTM_DEVICE_ASSIGNED_EVENT_DATA struct reports terminal assignment for an existing device (event type 4).
 * \remarks Correlate DeviceId with previously enumerated/arrived devices; this
 * 8-byte payload is a notification, not an assignment request.
 */
typedef struct _POWER_INTERNAL_TTM_DEVICE_ASSIGNED_EVENT_DATA
{
    ULONG DeviceId;
    ULONG AssignedTerminalId;
} POWER_INTERNAL_TTM_DEVICE_ASSIGNED_EVENT_DATA, *PPOWER_INTERNAL_TTM_DEVICE_ASSIGNED_EVENT_DATA;

/**
 * The POWER_INTERNAL_TTM_DEVICE_DEPARTED_EVENT_DATA struct identifies a departed device (event type 5).
 * \remarks Remove the identified DeviceId from the consumer's device view. Only
 * this 4-byte identifier is populated for the departure event.
 */
typedef struct _POWER_INTERNAL_TTM_DEVICE_DEPARTED_EVENT_DATA
{
    ULONG DeviceId;
} POWER_INTERNAL_TTM_DEVICE_DEPARTED_EVENT_DATA, *PPOWER_INTERNAL_TTM_DEVICE_DEPARTED_EVENT_DATA;

/**
 * The POWER_INTERNAL_TTM_DISPLAY_STATE_EVENT_DATA struct reports a terminal display-state change (event type 2).
 * \remarks Use the 12-byte notification to correlate a terminal with its reported
 * DisplayState and Reason. Field names are corroborated by TraceLogging metadata;
 * their numeric domains are not asserted to match a public display-state enum.
 */
typedef struct _POWER_INTERNAL_TTM_DISPLAY_STATE_EVENT_DATA
{
    ULONG TerminalId;
    ULONG DisplayState;
    ULONG Reason;
} POWER_INTERNAL_TTM_DISPLAY_STATE_EVENT_DATA, *PPOWER_INTERNAL_TTM_DISPLAY_STATE_EVENT_DATA;

// rev
/**
 * The POWER_INTERNAL_TTM_TERMINAL_EVENT struct carries one terminal/device notification consumed from a TTM event queue.
 * \remarks Selector 4100 returns the complete 0x220-byte record. EventType at +0 selects the payload at +8;
 * the intervening DWORD is zero in producers. Types 0/3 use Device, type 4 uses DeviceAssigned, type 5 uses
 * DeviceDeparted, and type 2 uses DisplayStateChanged. Type 1 marks enumeration complete. Type 6 reports an
 * updated display-required power request; its raw Payload is retained because its identifier/context
 * semantics were not fully recovered. Do not decode every event as Device. Consume the result only on
 * successful dequeue; it contains no owned handles.
 * \sa POWER_INTERNAL_TTM_GET_TERMINAL_EVENT_INPUT
 */
typedef struct _POWER_INTERNAL_TTM_TERMINAL_EVENT
{
    ULONG EventType;
    ULONG Reserved;
    union
    {
        UCHAR Payload[0x218]; // raw view; also retained for type 6 and unknown future types
        POWER_INTERNAL_TTM_DEVICE_EVENT_DATA Device;
        POWER_INTERNAL_TTM_DEVICE_ASSIGNED_EVENT_DATA DeviceAssigned;
        POWER_INTERNAL_TTM_DEVICE_DEPARTED_EVENT_DATA DeviceDeparted;
        POWER_INTERNAL_TTM_DISPLAY_STATE_EVENT_DATA DisplayStateChanged;
    };
} POWER_INTERNAL_TTM_TERMINAL_EVENT, *PPOWER_INTERNAL_TTM_TERMINAL_EVENT;

// rev
/**
 * The POWER_INTERNAL_TTM_SET_DEFAULT_DEVICE_ASSIGNMENT_INPUT struct updates the session's default-device-assignment policy through its default terminal.
 * \remarks Select PowerInternalTtmSetDefaultDeviceAssignmentV2 (4101), not legacy infoclass 14.
 * Supply at least 0x18 input bytes and NULL/0 output. TerminalHandle must have access
 * 0x00000002 and identify terminal ID 0. The nonzero/zero byte at +0x10 sets/clears
 * session policy bit 11. This is a session policy update, not a per-device assignment
 * and not a query. Reserved bytes should be zero.
 * Use outer PowerInformationInternal; the V2 name is a descriptive alias, not a recovered
 * symbol. The route requires SeShutdownPrivilege OR terminalPowerManagement capability
 * and TTM enabled. Header.Version is not checked by the dispatch/handler path.
 * \sa POWER_INTERNAL_TTM_ASSIGN_DEVICE_INPUT
 */
typedef struct _POWER_INTERNAL_TTM_SET_DEFAULT_DEVICE_ASSIGNMENT_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    HANDLE TerminalHandle;
    BOOLEAN DefaultAssignmentEnabled;
    UCHAR Reserved[7];
} POWER_INTERNAL_TTM_SET_DEFAULT_DEVICE_ASSIGNMENT_INPUT, *PPOWER_INTERNAL_TTM_SET_DEFAULT_DEVICE_ASSIGNMENT_INPUT;

#ifdef _WIN64
#endif

// rev
/**
 * The POWER_INTERNAL_TTM_ASSIGN_DEVICE_INPUT struct defines the assigns an enumerated device to a terminal within its session.
 * \remarks Select PowerInternalTtmAssignDeviceV2 (4102), not legacy infoclass 15.
 * Supply at least 0x18 input bytes and NULL/0 output. TerminalHandle requires access
 * 0x00000002; default and nondefault terminals are accepted. DeviceId at +0x10 is
 * looked up in that session, not interpreted as a device handle. An unknown ID returns
 * STATUS_NOT_FOUND. Success publishes an assignment event and schedules further work.
 * Use outer PowerInformationInternal; the V2 name is a descriptive alias, not a recovered
 * symbol. The route requires SeShutdownPrivilege OR terminalPowerManagement capability
 * and TTM enabled. Header.Version is not checked by the dispatch/handler path.
 * \sa POWER_INTERNAL_TTM_DEVICE_EVENT_DATA
 */
typedef struct _POWER_INTERNAL_TTM_ASSIGN_DEVICE_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    HANDLE TerminalHandle;
    ULONG DeviceId;
    ULONG Reserved;
} POWER_INTERNAL_TTM_ASSIGN_DEVICE_INPUT, *PPOWER_INTERNAL_TTM_ASSIGN_DEVICE_INPUT;

#ifdef _WIN64
#endif

// rev
/**
 * The POWER_INTERNAL_TTM_SET_DISPLAY_STATE_INPUT struct defines the queues an on/off display request for a power-management terminal.
 * \remarks Select PowerInternalTtmSetDisplayStateV2 (4103), not legacy infoclass 16.
 * Supply at least 0x18 input bytes and NULL/0 output. TerminalHandle requires access
 * 0x00000002; either default or nondefault terminal is accepted. DisplayOn at +0x10
 * and the DWORD RequestReason at +0x14 are forwarded to TtmiSetPendingOnOffRequest.
 * Success means the request was submitted, not that the physical display has changed.
 * Observe display-state-change events for subsequent state reporting.
 * Use outer PowerInformationInternal; the V2 name is a descriptive alias, not a recovered
 * symbol. The route requires SeShutdownPrivilege OR terminalPowerManagement capability
 * and TTM enabled. Header.Version is not checked by the dispatch/handler path.
 * \sa POWER_INTERNAL_TTM_DISPLAY_STATE_EVENT_DATA
 */
typedef struct _POWER_INTERNAL_TTM_SET_DISPLAY_STATE_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    HANDLE TerminalHandle;
    BOOLEAN DisplayOn;
    UCHAR Reserved[3];
    ULONG RequestReason;
} POWER_INTERNAL_TTM_SET_DISPLAY_STATE_INPUT, *PPOWER_INTERNAL_TTM_SET_DISPLAY_STATE_INPUT;

#ifdef _WIN64
#endif

// rev
/**
 * The POWER_INTERNAL_TTM_SET_DISPLAY_TIMEOUTS_INPUT struct updates terminal dim/off inactivity timeouts and schedules reevaluation.
 * \remarks Select PowerInternalTtmSetDisplayTimeoutsV2 (4104), not legacy infoclass 17.
 * Supply at least 0x18 input bytes and NULL/0 output. TerminalHandle requires access
 * 0x00000002; either default or nondefault terminal is accepted. The DWORD fields at
 * +0x10 and +0x14 are seconds, multiplied by 10000000 into 100-ns durations internally.
 * If both are nonzero and DimTimeoutSeconds >= OffTimeoutSeconds, the internal dim
 * timeout becomes zero. No maximum beyond the ULONG input width is checked here.
 * Use outer PowerInformationInternal; the V2 name is a descriptive alias, not a recovered
 * symbol. The route requires SeShutdownPrivilege OR terminalPowerManagement capability
 * and TTM enabled. Header.Version is not checked by the dispatch/handler path.
 * \sa POWER_INTERNAL_TTM_SET_DISPLAY_STATE_INPUT
 */
typedef struct _POWER_INTERNAL_TTM_SET_DISPLAY_TIMEOUTS_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    HANDLE TerminalHandle;
    ULONG DimTimeoutSeconds;
    ULONG OffTimeoutSeconds;
} POWER_INTERNAL_TTM_SET_DISPLAY_TIMEOUTS_INPUT, *PPOWER_INTERNAL_TTM_SET_DISPLAY_TIMEOUTS_INPUT;

#ifdef _WIN64
#endif

// rev
/**
 * The POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_INPUT struct selects the captured physical-power-button boot diagnostic record.
 * \remarks Set InternalType to infoclass 22 and pass at least this 8-byte header with
 * at least 0x40 output bytes. The handler copies the boot snapshot, not a live button
 * reading, and does not acknowledge or clear it. Version is not on this route.
 * Use this query for post-boot power-button diagnostics without issuing a power action.
 * \sa POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_OUTPUT
 */
typedef struct _POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_INPUT, *PPOWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_INPUT;

// rev
/**
 * The POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_OUTPUT struct receives the complete physical-power-button snapshot captured at boot.
 * \remarks Selector 22 copies 0x40 bytes from PopBsdPhysicalPowerButtonInfoAtBoot.
 * Retain all bytes for diagnostics; no stable typed layout or timestamp unit is established
 * by this copy path. Read Buffer only after a successful query and do not interpret it
 * as a BOOLEAN or a current hardware button state. No owned handles are returned.
 * \sa POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_INPUT
 */
typedef struct _POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_OUTPUT
{
    UCHAR Buffer[64];
} POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_OUTPUT, *PPOWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_EXTERNAL_MONITOR_CONNECTED_INPUT struct notifies the kernel power manager whether an external console display is connected.
 * \remarks Kernel-mode infoclass 23 requires exactly 0x0C input bytes. Connected at +8
 * updates PopConsoleExternalDisplayConnected under the policy lock, traces the change,
 * and queues external-monitor update work. This reports state; it does not enumerate,
 * attach, or detach monitors. Use NULL/0 output: supplied output is ignored on this path,
 * not populated or explicitly rejected. User mode receives STATUS_ACCESS_DENIED.
 * \sa POWER_SESSION_POWER_STATE
 */
typedef struct _POWER_INTERNAL_EXTERNAL_MONITOR_CONNECTED_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    BOOLEAN Connected; // 1 = connected, 0 = disconnected
} POWER_INTERNAL_EXTERNAL_MONITOR_CONNECTED_INPUT, *PPOWER_INTERNAL_EXTERNAL_MONITOR_CONNECTED_INPUT;

// rev
/**
 * The POWER_INTERNAL_HIGH_PRECISION_BRIGHTNESS_SETTINGS_INPUT struct defines the broadcasts a high-precision brightness-engine payload to the session subsystem.
 * \remarks Selector 24 accepts only user-mode LocalSystem; kernel-mode
 * callers receive STATUS_NOT_SUPPORTED. Supply at least 0x28 input bytes and NULL/0 output.
 * The 32 bytes at +8 are forwarded unchanged via PopBroadcastSessionInfo, information
 * class 4. A DWORD at +0x0C is compared with 0xFFFFFFFF: a different value enables the
 * high-precision path and publishes basic-brightness-engine-disabled if not already enabled;
 * 0xFFFFFFFF is rejected with STATUS_NOT_SUPPORTED once the high-precision path is enabled.
 *
 * Use Data to preserve the complete payload; field meanings and units are not established.
 * Do not treat this as a general-purpose brightness-percentage setter; the receiving
 * session subsystem's full payload contract is private.
 * \sa POWER_INFORMATION_INTERNAL_HEADER
 */
typedef struct _POWER_INTERNAL_HIGH_PRECISION_BRIGHTNESS_SETTINGS_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    UCHAR Data[0x20]; // forwarded unchanged through PopBroadcastSessionInfo
} POWER_INTERNAL_HIGH_PRECISION_BRIGHTNESS_SETTINGS_INPUT, *PPOWER_INTERNAL_HIGH_PRECISION_BRIGHTNESS_SETTINGS_INPUT;

// rev
/**
 * The POWER_INTERNAL_WINRT_SCREEN_TOGGLE_INPUT struct requests an explicit screen-on or screen-off action through the internal WinRT power path.
 * \remarks Selector 25 accepts at least 12 input bytes and requires NULL/0 output.
 * ScreenOn at offset 8 is a desired Boolean state, not an instruction to invert
 * current state. Nonzero calls PopScreenOn; zero
 * calls PopScreenOff with reason 30. Success indicates dispatch, not hardware completion.
 *
 * For user mode, RtlCapabilityCheckForSingleSessionSku checks systemManagement and must
 * return both successful status and a true capability result. Multi-session SKUs produce
 * false in that helper and are denied; the kernel-mode route does not perform that capability
 * check. Version is not checked. This changes display state; do not use it as a query.
 * \sa POWER_INFORMATION_INTERNAL_HEADER
 */
typedef struct _POWER_INTERNAL_WINRT_SCREEN_TOGGLE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    union
    {
        BOOLEAN ScreenOn; // nonzero selects on; zero selects off (not an inversion)
    };
} POWER_INTERNAL_WINRT_SCREEN_TOGGLE_INPUT, *PPOWER_INTERNAL_WINRT_SCREEN_TOGGLE_INPUT;

// rev
/**
 * The POWER_INTERNAL_PPM_QOS_DISABLE_INPUT struct defines the acquires or releases a reference that disables processor QoS policy.
 * \remarks Kernel-only infoclass 26 takes at least 12 input bytes and NULL/0 output.
 * Disable at offset 8 is a reference-count operation,
 * not an idempotent Boolean setter. Nonzero increments PpmPerfQosDisableRefcount; zero
 * decrements it. Increment at 0xFFFFFFFF returns STATUS_INTEGER_OVERFLOW; release at zero
 * returns STATUS_NOT_SUPPORTED. Balance successful acquisitions/releases rather than
 * blindly repeating requests. User mode receives STATUS_ACCESS_DENIED.
 *
 * The handler updates domain policy when an increment leaves the count at 1 or a release
 * leaves it at 0. The failed release-at-zero path also calls the policy update;
 * the behavior is not limited strictly to successful 0/1 transitions. Version is not checked.
 * \sa POWER_INTERNAL_PROCESSOR_QOS_SUPPORT
 */
typedef struct _POWER_INTERNAL_PPM_QOS_DISABLE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    union
    {
        BOOLEAN Disable; // nonzero acquires a QoS-disable reference; zero releases one
    };
} POWER_INTERNAL_PPM_QOS_DISABLE_INPUT, *PPOWER_INTERNAL_PPM_QOS_DISABLE_INPUT;

// rev
/**
 * The POWER_INTERNAL_TRANSITION_CHECKPOINT_INPUT struct records a diagnostic checkpoint during a host power transition.
 * \remarks Selector 27 takes at least 16 input bytes and NULL/0 output. CheckpointId and
 * CheckpointType at offsets 8 and 12 are forwarded to PopTransitionCheckpoint. In the host
 * silo, an existing checkpoint list or CheckpointId=1 permits recording/updating the latest
 * checkpoint metadata and sequence number. Non-host silos are ignored. This is progress
 * telemetry, not a request to enter the power state encoded by an ID.
 *
 * The dispatcher returns success after the call even if no record is made or allocation
 * fails; it is not a durable log-write acknowledgement. The numeric ID/type domains are
 * not recovered here, and Version is not checked.
 * \sa POWER_INFORMATION_INTERNAL_HEADER
 */
typedef struct _POWER_INTERNAL_TRANSITION_CHECKPOINT_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG CheckpointId;
    ULONG CheckpointType;
} POWER_INTERNAL_TRANSITION_CHECKPOINT_INPUT, *PPOWER_INTERNAL_TRANSITION_CHECKPOINT_INPUT;

// rev
/**
 * The POWER_INTERNAL_INPUT_CONTROLLER_STATE struct notifies the kernel power policy of the adaptive input-controller state.
 * \remarks Kernel-only infoclass 28 requires exactly 12 input bytes. The DWORD at offset 8
 * is written through PopSetPowerSettingValueAcDc to GUID_ADAPTIVE_INPUT_CONTROLLER_STATE
 * for both AC and DC. It is a setting update, not a controller enumeration or state query.
 * Use NULL/0 output: a supplied output is ignored, not filled or explicitly rejected.
 *
 * The dispatcher discards the setting helper's status and returns success after invocation;
 * it does not prove both setting updates succeeded. State values are opaque. Version is not checked; user mode receives STATUS_ACCESS_DENIED.
 * \sa POWER_INFORMATION_INTERNAL_HEADER
 */
typedef struct _POWER_INTERNAL_INPUT_CONTROLLER_STATE
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONG InputControllerState;
} POWER_INTERNAL_INPUT_CONTROLLER_STATE, *PPOWER_INTERNAL_INPUT_CONTROLLER_STATE;

// rev
/**
 * The POWER_INTERNAL_FIRMWARE_RESET_REASON_INPUT struct selects the firmware reset information captured from the boot loader.
 * \remarks For infoclass 29 provide at least this 8-byte header and at least 48 output bytes.
 * Use this read-only snapshot for reset diagnostics, not to query current hardware or clear
 * the stored record. User-mode callers require administrator context; kernel mode is accepted.
 * Version is not checked by this route. Query success alone does not imply reset data was
 * supplied: check the result's Supplied byte before interpreting Basic/AdditionalInfo.
 * \sa POWER_INTERNAL_FIRMWARE_RESET_REASON_OUTPUT
 */
typedef struct _POWER_INTERNAL_FIRMWARE_RESET_REASON_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_FIRMWARE_RESET_REASON_INPUT, *PPOWER_INTERNAL_FIRMWARE_RESET_REASON_INPUT;

// symbols, rev (_LOADER_RESET_REASON.Basic, AMD64)
/**
 * The POWER_INTERNAL_FIRMWARE_RESET_REASON_BASIC union holds the loader-provided basic reset classification by platform component.
 * \remarks The 8-byte value is exposed both raw and as the symbol-defined Pch and
 * EmbeddedController bytes followed by 6 reserved bytes. The numeric classifications
 * are not defined here. Interpret it only when the enclosing Supplied
 * flag is nonzero; it is not an object pointer or a timestamp.
 * \sa POWER_INTERNAL_FIRMWARE_RESET_REASON_OUTPUT
 */
typedef union _POWER_INTERNAL_FIRMWARE_RESET_REASON_BASIC
{
    struct
    {
        UCHAR Pch;
        UCHAR EmbeddedController;
        UCHAR Reserved[6];
    } Component;
    ULONGLONG AsULONG64;
    UCHAR AsBytes[8];
} POWER_INTERNAL_FIRMWARE_RESET_REASON_BASIC, *PPOWER_INTERNAL_FIRMWARE_RESET_REASON_BASIC;

// rev
/**
 * The POWER_INTERNAL_FIRMWARE_RESET_REASON_OUTPUT struct returns the loader's supplied flag, basic component reasons, and additional reset data.
 * \remarks Selector 29 copies 48 bytes from PopFirmwareResetReason. Symbols identify
 * _LOADER_RESET_REASON: BOOLEAN Supplied at 0, an 8-byte Basic value at 8, and eight ULONG
 * AdditionalInfo values at 16. PopRecordFirmwareResetReason copies Basic/AdditionalInfo
 * only when the loader's Supplied flag is nonzero. Reserved0 is alignment space.
 *
 * Use this snapshot to correlate boot/reset diagnostics. Component codes and AdditionalInfo
 * meanings are not established here. Buffer preserves the full record. The typed record
 * has 8-byte alignment.
 * \sa POWER_INTERNAL_FIRMWARE_RESET_REASON_BASIC
 * \sa POWER_INTERNAL_FIRMWARE_RESET_REASON_INPUT
 */
typedef struct _POWER_INTERNAL_FIRMWARE_RESET_REASON_OUTPUT
{
    union
    {
        UCHAR Buffer[0x30];
        struct
        {
            BOOLEAN Supplied; // loader supplied reset information
            UCHAR Reserved0[7];
            POWER_INTERNAL_FIRMWARE_RESET_REASON_BASIC Basic;
            ULONG AdditionalInfo[8]; // meanings remain platform-specific
        };
    };
} POWER_INTERNAL_FIRMWARE_RESET_REASON_OUTPUT, *PPOWER_INTERNAL_FIRMWARE_RESET_REASON_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_PROCESSOR_QOS_SUPPORT struct reports processor QoS configuration, scheduler-directed performance support, and policy disablement.
 * \remarks Query infoclass 30 with an 8-byte POWER_INFORMATION_INTERNAL_HEADER and at
 * least 3 output bytes. The three bytes are independent fields, not a bitmask or ULONG.
 * QosSupportedAndConfigured and SchedulerDirectedPerfStatesSupported copy the corresponding
 * kernel Boolean globals; QosGroupPolicyDisable normalizes its DWORD global to Boolean.
 *
 * Use this diagnostic result to distinguish hardware/configuration support from policy
 * blocking when investigating QoS behavior. It does not expose the disable reference count
 * used by infoclass 26 and does not enable any feature. Version is not checked.
 * \sa POWER_INTERNAL_PPM_QOS_DISABLE_INPUT
 */
typedef struct _POWER_INTERNAL_PROCESSOR_QOS_SUPPORT
{
    BOOLEAN QosSupportedAndConfigured;
    BOOLEAN SchedulerDirectedPerfStatesSupported;
    BOOLEAN QosGroupPolicyDisable;
} POWER_INTERNAL_PROCESSOR_QOS_SUPPORT, *PPOWER_INTERNAL_PROCESSOR_QOS_SUPPORT;

/**
 * The POWER_INTERNAL_PROCESSOR_QOS_SUPPORT_INPUT type selects the three-byte processor-QoS diagnostic query (infoclass 30).
 * \remarks Pass at least this 8-byte header and a 3-byte output buffer. No payload
 * beyond InternalType/Version is consumed; Version is not checked.
 * \sa POWER_INTERNAL_PROCESSOR_QOS_SUPPORT
 */
typedef POWER_INFORMATION_INTERNAL_HEADER POWER_INTERNAL_PROCESSOR_QOS_SUPPORT_INPUT, *PPOWER_INTERNAL_PROCESSOR_QOS_SUPPORT_INPUT;

typedef struct _RTL_BSD_ITEM RTL_BSD_ITEM, *PRTL_BSD_ITEM;

// rev
/**
 * The POWER_INTERNAL_BOOTSTAT_GET_INPUT struct describes a batch of boot-status items to retrieve into separately supplied buffers.
 * \remarks Selector 31 requires exactly 24 outer input bytes on AMD64, with nonzero
 * EntryCount at +8 and Entries pointer at +16. The 8-byte-aligned Entries array contains
 * 24-byte RTL_BSD_ITEM records: Type at +0, DataBuffer pointer at +8, DataLength at +16.
 * These descriptors and their buffers must remain valid for the call. DataLength is the
 * buffer capacity in bytes, not a pointer to a length. User-mode destinations are probed
 * for writing, and file read-data access (mask 1) is checked against the boot-status file.
 *
 * Item data is written to Entries[i].DataBuffer, NOT the direct NtPowerInformation output.
 * Optional direct output is exactly one ULONG transferred-byte count per item, 4*EntryCount
 * bytes without a header. Omit it with NULL/0 if not needed. With output, EntryCount above
 * 0x3FFFFFFF fails the size calculation; this bound is not a practical allocation guarantee.
 * Type values 0..18 are accepted by the item lookup. Version is not checked.
 *
 * This diagnostic batch is not an all-or-nothing transaction: item buffers may be modified
 * before a later failure or the outer output-length check. The loop can continue after an
 * item I/O failure, so final NTSTATUS and length slots are not a per-item status array or
 * a guarantee that every earlier item succeeded. A failed transfer can leave a length slot
 * holding a previous transferred length; do not interpret those slots as required sizes.
 * \sa POWER_INTERNAL_BOOTSTAT_GET_OUTPUT
 * \sa POWER_INTERNAL_BOOTSTAT_SET_INPUT
 */
typedef struct _POWER_INTERNAL_BOOTSTAT_GET_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG EntryCount;
    ULONG Reserved;
    PRTL_BSD_ITEM Entries;
} POWER_INTERNAL_BOOTSTAT_GET_INPUT, *PPOWER_INTERNAL_BOOTSTAT_GET_INPUT;

#ifdef _WIN64
#endif

/**
 * The POWER_INTERNAL_BOOTSTAT_SET_INPUT type supplies item buffers to update persistent boot-status data (infoclass 32).
 * \remarks The AMD64 outer request is exactly 24 bytes, with nonzero EntryCount
 * and an 8-byte-aligned array of 24-byte RTL_BSD_ITEM records at Entries. Each record
 * supplies Type, DataBuffer, and DataLength; the data are not embedded in this header.
 * User-mode calls require write-data access (mask 2) to the boot-status file's security
 * descriptor. Version is not checked here. Item Type must be 0..18; checksum item 15
 * cannot be written through RtlGetSetBootStatusData. DataLength must cover the selected
 * field's actual size; do not infer that size from an unverified payload declaration.
 * Optional direct output is one ULONG byte count per item, not item data or NTSTATUSs.
 * This is a persistent write, not an atomic transaction: earlier writes are not rolled
 * back on later failures. Do not automatically retry based only on the outer status.
 * \sa POWER_INTERNAL_BOOTSTAT_GET_INPUT
 * \sa POWER_INTERNAL_BOOTSTAT_SET_OUTPUT
 */
typedef POWER_INTERNAL_BOOTSTAT_GET_INPUT POWER_INTERNAL_BOOTSTAT_SET_INPUT, *PPOWER_INTERNAL_BOOTSTAT_SET_INPUT;

/**
 * The POWER_INTERNAL_BOOTSTAT_RESTORE_DEFAULTS_INPUT type selects restoration of the system boot-status file's default contents.
 * \remarks Selector 35 is destructive, not a header-only query. AMD64 requires exactly
 * 24 input bytes even though its helper does not consume Entries. Set EntryCount=0,
 * Reserved=0, Entries=NULL and use NULL/0 direct output. User mode additionally requires
 * LocalSystem and the helper's file read-data access check (mask 1); kernel mode is accepted.
 * Version is not checked. A non-NULL direct output instead invokes the shared count/size
 * path: it requires EntryCount>0 and returns 4*EntryCount zero-initialized bytes on success,
 * not restored data. The restore helper writes a 200-byte default record at file offset 0
 * and updates the matching cache. Prefer no output; never use this as a harmless capability probe.
 * \sa POWER_INTERNAL_BOOTSTAT_GET_INPUT
 */
typedef POWER_INTERNAL_BOOTSTAT_GET_INPUT POWER_INTERNAL_BOOTSTAT_RESTORE_DEFAULTS_INPUT, *PPOWER_INTERNAL_BOOTSTAT_RESTORE_DEFAULTS_INPUT;

/**
 * The POWER_INTERNAL_BOOTSTAT_UNLOCK_INPUT type releases the kernel's cached boot-status file state, not a caller-owned handle.
 * \remarks Selector 38 requires exactly 24 input bytes on AMD64. Use EntryCount=0,
 * Reserved=0, Entries=NULL and NULL/0 output; the helper does not read Entries and
 * calls RtlUnlockBootStatusData(NULL) under PopBootStatLock. It can reset reference/cache
 * state and close the cached file. Do not use this to release arbitrary handles or as
 * a read-only query. No infoclass-specific user privilege check is present.
 * A non-NULL direct output takes the shared count path and returns 4*EntryCount zero bytes
 * on success (EntryCount must be nonzero); this is not useful diagnostic data.
 * \sa POWER_INTERNAL_BOOTSTAT_RESTORE_DEFAULTS_INPUT
 */
typedef POWER_INTERNAL_BOOTSTAT_GET_INPUT POWER_INTERNAL_BOOTSTAT_UNLOCK_INPUT, *PPOWER_INTERNAL_BOOTSTAT_UNLOCK_INPUT;

// rev
/**
 * The POWER_INTERNAL_BOOTSTAT_GET_OUTPUT struct contains the optional headerless array of boot-status transfer byte counts.
 * \remarks Allocate 4*EntryCount bytes for infoclass 31 or 32; Sizes[ANYSIZE_ARRAY] is a
 * trailing-array declaration, not a fixed single-item result or an array of wrapper structs.
 * No count/header precedes Sizes. Actual item data travels through Entries[].DataBuffer.
 * These slots are byte counts, not NTSTATUSs or guaranteed required-buffer sizes.
 *
 * The direct buffer is copied only when the dispatcher succeeds and its final size check
 * passes, but nested buffers or persistent writes may already have changed. The underlying
 * loops do not reliably expose each item's failure independently; stale lengths are possible
 * when a failed item follows another transfer. Do not use a size slot as proof of item success.
 * \sa POWER_INTERNAL_BOOTSTAT_GET_INPUT
 * \sa POWER_INTERNAL_BOOTSTAT_OUTPUT_SIZE
 */
typedef struct _POWER_INTERNAL_BOOTSTAT_GET_OUTPUT
{
    // If present, it receives the actual sizes of the data copied into each DataBuffer.
    ULONG Sizes[ANYSIZE_ARRAY]; // Array of sizes, one per entry
} POWER_INTERNAL_BOOTSTAT_GET_OUTPUT, *PPOWER_INTERNAL_BOOTSTAT_GET_OUTPUT;

/**
 * The POWER_INTERNAL_BOOTSTAT_SET_OUTPUT type describes optional per-item byte counts returned by boot-status writes.
 * \remarks This aliases the same headerless ULONG[EntryCount] output used by Get.
 * It is not an array of NTSTATUS or item data. Direct output may be omitted; writes
 * and their side effects can occur before the outer direct-output length is checked.
 * \sa POWER_INTERNAL_BOOTSTAT_SET_INPUT
 */
typedef POWER_INTERNAL_BOOTSTAT_GET_OUTPUT POWER_INTERNAL_BOOTSTAT_SET_OUTPUT, *PPOWER_INTERNAL_BOOTSTAT_SET_OUTPUT;

/**
 * The POWER_INTERNAL_BOOTSTAT_MAX_OUTPUT_ENTRIES constant is the largest entry count whose optional direct-output byte length fits ULONG.
 * \remarks This is an arithmetic limit (0x3FFFFFFF), not an allocation or success guarantee.
 * It is not a limit on the numeric RTL_BSD_ITEM_TYPE value, whose domain is separate.
 */
#define POWER_INTERNAL_BOOTSTAT_MAX_OUTPUT_ENTRIES 0x3FFFFFFFUL

/**
 * The POWER_INTERNAL_BOOTSTAT_OUTPUT_SIZE macro computes the boot-status transfer-length array size without 32-bit multiplication overflow.
 * \param EntryCount A ULONG count, evaluated once.
 * \return A ULONGLONG byte count equal to 4*EntryCount, with no leading header.
 * \remarks Before narrowing to ULONG or allocating, require a nonzero count no larger
 * than POWER_INTERNAL_BOOTSTAT_MAX_OUTPUT_ENTRIES and apply practical allocation limits.
 * This does not size or validate Entries or their pointed-to data buffers.
 */
#define POWER_INTERNAL_BOOTSTAT_OUTPUT_SIZE(EntryCount) ((ULONGLONG)(EntryCount) * sizeof(ULONG))

// rev
/**
 * The POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_INPUT struct supplies a destination for the boot-status file's integrity-verification result.
 * \remarks Selector 34 requires exactly 24 input bytes on AMD64 and EntryCount>0. Prefer
 * EntryCount=1 with one 8-byte-aligned RTL_BSD_ITEM: DataBuffer points to a writable
 * POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_OUTPUT and DataLength is at least 1. Its Type
 * is not consumed by this helper. All descriptors' buffers are probed for user-mode writes,
 * but only the first descriptor receives the Boolean result. File read-data access (mask 1)
 * is checked for user mode. Version is not checked.
 *
 * Use NULL/0 for the direct NtPowerInformation output. The Boolean is written through
 * Entries[0].DataBuffer, not there. If direct output is supplied, the shared dispatcher
 * instead allocates 4*EntryCount zero-initialized bytes, untouched by the integrity helper;
 * those zeros are not integrity results. NTSTATUS success is separate from IntegrityOk:
 * a file can be read successfully yet fail integrity verification.
 * \sa POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_OUTPUT
 */
typedef struct _POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG EntryCount;
    ULONG Reserved;
    PRTL_BSD_ITEM Entries;
} POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_INPUT, *PPOWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_INPUT;

#ifdef _WIN64
#endif

// rev
/**
 * The POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_OUTPUT struct receives whether the boot-status data's structural bounds and checksum verified.
 * \remarks This 1-byte object belongs at Entries[0].DataBuffer for infoclass 34, with
 * DataLength>=1. It is NOT the direct NtPowerInformation output. Read IntegrityOk only
 * after successful status; FALSE may still accompany success when integrity is invalid.
 * The helper accepts a reported file length from 51 through 2048 bytes,
 * requires a complete read, and checks an 8-bit byte-sum checksum equal to zero. It does not repair the data; restoration is a separate destructive
 * operation. Other entries do not receive repeated copies of this Boolean.
 * \sa POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_INPUT
 */
typedef struct _POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_OUTPUT
{
    BOOLEAN IntegrityOk;
} POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_OUTPUT, *PPOWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_HOST_ENERGY_SAVER_STATE struct reports the host's energy-saver enabled state to the power manager.
 * \remarks Selector 36 accepts user-mode LocalSystem only, at least 12 input bytes,
 * and NULL/0 output. EsEnabledOnHost at +8 is stored under PopEsLock and an asynchronous
 * energy-saver reevaluation is queued. Use this to propagate host state, not to read current
 * policy or assume a synchronous change to a local power plan. Kernel-mode calls return
 * STATUS_NOT_SUPPORTED; other user contexts are denied. Version is not checked.
 * \sa POWER_INFORMATION_INTERNAL_HEADER
 */
typedef struct _POWER_INTERNAL_HOST_ENERGY_SAVER_STATE
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    BOOLEAN EsEnabledOnHost;
} POWER_INTERNAL_HOST_ENERGY_SAVER_STATE, *PPOWER_INTERNAL_HOST_ENERGY_SAVER_STATE;

/**
 * The POWER_INTERNAL_GET_POWER_ACTION_STATE_INPUT type selects a snapshot of the power manager's current POWER_ACTION value.
 * \remarks Set InternalType=PowerInternalGetPowerActionState (37), pass at least this
 * 8-byte header, and provide 4 output bytes. Version is not checked by this route.
 * Use this diagnostic query to identify an action in progress without initiating one.
 * \sa POWER_INTERNAL_GET_POWER_ACTION_STATE_OUTPUT
 */
typedef POWER_INFORMATION_INTERNAL_HEADER POWER_INTERNAL_GET_POWER_ACTION_STATE_INPUT, *PPOWER_INTERNAL_GET_POWER_ACTION_STATE_INPUT;

/**
 * The POWER_INTERNAL_GET_POWER_ACTION_STATE_OUTPUT struct reports the action selected by the power manager, not its internal phase/state byte.
 * \remarks Selector 37 copies the 4-byte _POP_POWER_ACTION.Action field at offset 4.
 * The result is a POWER_ACTION (also wire-compatible with ULONG), not SYSTEM_POWER_STATE
 * and not the byte named State at offset 1. Read it only after query success; it is a
 * snapshot and does not promise that the action will complete or remain unchanged.
 * \sa POWER_INTERNAL_GET_POWER_ACTION_STATE_INPUT
 */
typedef struct _POWER_INTERNAL_GET_POWER_ACTION_STATE_OUTPUT
{
    POWER_ACTION Action;
} POWER_INTERNAL_GET_POWER_ACTION_STATE_OUTPUT, *PPOWER_INTERNAL_GET_POWER_ACTION_STATE_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_IS_POFX_DEVICE_INPUT struct queries whether a kernel device has a non-NULL PoFx plugin association.
 * \remarks Selector 41 requires exactly 16 input bytes on AMD64 and one Boolean output.
 * DeviceObject at offset 8 is a live kernel DEVICE_OBJECT pointer, NOT a user handle.
 * The kernel follows DeviceObjectExtension->DeviceNode->FxDevice->Plugin. A NULL device,
 * node, FxDevice, or Plugin yields FALSE; an arbitrary pointer is not validated as a handle
 * and is unsafe. The device extension is dereferenced without a NULL check on that hop.
 *
 * Use this for kernel power-framework diagnostics with a valid device lifetime. No handle
 * or reference ownership is transferred. User mode receives STATUS_ACCESS_DENIED; Version
 * is not checked. FALSE does not establish that a device has no other power-management support.
 * \sa POWER_INFORMATION_INTERNAL_HEADER
 */
typedef struct _POWER_INTERNAL_IS_POFX_DEVICE_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    PVOID DeviceObject;
} POWER_INTERNAL_IS_POFX_DEVICE_INPUT, *PPOWER_INTERNAL_IS_POFX_DEVICE_INPUT;

#ifdef _WIN64
#endif

// rev
/**
 * The POWER_INTERNAL_POWER_TRANSITION_EXTENSION_AT_BOOT_OUTPUT struct preserves the boot-captured power-transition extension for diagnostics.
 * \remarks Selector 42 accepts an 8-byte input header and copies this complete 32-byte record from
 * PopBsdPowerTransitionExtensionAtBoot. It is not a live transition query, acknowledgement, or clear
 * operation. The snapshot contains no caller-owned handle. Its complete field and bit meanings are private;
 * retain Data rather than interpreting it as a guessed enumeration or an unrelated legacy BSD type.
 * \sa POWER_INTERNAL_POWER_TRANSITION_EXTENSION_AT_BOOT_INPUT
 */
typedef struct _POWER_INTERNAL_POWER_TRANSITION_EXTENSION_AT_BOOT_OUTPUT
{
    UCHAR Data[32];
} POWER_INTERNAL_POWER_TRANSITION_EXTENSION_AT_BOOT_OUTPUT, *PPOWER_INTERNAL_POWER_TRANSITION_EXTENSION_AT_BOOT_OUTPUT;

/**
 * The POWER_INTERNAL_POWER_TRANSITION_EXTENSION_AT_BOOT_INPUT type selects the boot-captured power-transition extension diagnostic snapshot.
 * \remarks Selector 42 needs at least this 8-byte header and 32 output bytes.
 * No additional query payload or Version check is present on the route.
 * \sa POWER_INTERNAL_POWER_TRANSITION_EXTENSION_AT_BOOT_OUTPUT
 */
typedef POWER_INFORMATION_INTERNAL_HEADER POWER_INTERNAL_POWER_TRANSITION_EXTENSION_AT_BOOT_INPUT, *PPOWER_INTERNAL_POWER_TRANSITION_EXTENSION_AT_BOOT_INPUT;

// rev
/**
 * The POWER_INTERNAL_NOTIFY_USER_SHUTDOWN_STATUS_INPUT struct notifies the power manager that the user initiated or cancelled the shutdown scenario.
 * \remarks Selector 45 requires exactly 12 bytes. ShutdownInitiated at +8 selects
 * PoUserShutdownInitiated or PoUserShutdownCancelled; this reports scenario state rather
 * than directly shutting down the computer. The helpers ignore server-silo callers; the
 * dispatcher returns success after notification and does not promise an actual transition.
 *
 * User mode is required, with LocalSystem OR (testsigning enabled AND administrator context).
 * Kernel mode returns STATUS_NOT_SUPPORTED. Version is not checked. Use NULL/0 output;
 * a supplied output is ignored. No shutdown privilege bypass is provided by this declaration.
 * \sa POWER_INFORMATION_INTERNAL_HEADER
 */
typedef struct _POWER_INTERNAL_NOTIFY_USER_SHUTDOWN_STATUS_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    BOOLEAN ShutdownInitiated; //  1 = initiated, 0 = cancelled
} POWER_INTERNAL_NOTIFY_USER_SHUTDOWN_STATUS_INPUT, *PPOWER_INTERNAL_NOTIFY_USER_SHUTDOWN_STATUS_INPUT;

// rev
/**
 * The POWER_INTERNAL_AUTOCHK_CAUSED_REBOOT_INPUT struct selects the AutoChk-caused-reboot flag captured during boot diagnostics.
 * \remarks Set InternalType=PowerInternalAutoChkCausedReboot (50), supply at least this
 * 8-byte header and one output byte. Version is not checked. This queries the captured
 * flag; it does not run AutoChk, test disk integrity, or trigger a reboot.
 * \sa POWER_INTERNAL_AUTOCHK_CAUSED_REBOOT_OUTPUT
 */
typedef struct _POWER_INTERNAL_AUTOCHK_CAUSED_REBOOT_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_AUTOCHK_CAUSED_REBOOT_INPUT, *PPOWER_INTERNAL_AUTOCHK_CAUSED_REBOOT_INPUT;

// rev
/**
 * The POWER_INTERNAL_READ_HIBERFILE_PAGE_INPUT struct selects one raw 4096-byte file page from the current hibernation file.
 * \remarks Selector 61 takes at least 12 bytes with PageNumber at 8. It reads file offset
 * PageNumber*4096, not a physical memory page, virtual address, or decompressed hibernation
 * image page. Provide 4096 output bytes. User mode requires testsigning enabled AND
 * administrator context; kernel mode is accepted. Version is not checked.
 *
 * The file must already be available in the kernel's hibernation state; unsupported state
 * returns STATUS_NOT_SUPPORTED and a page extending past the file yields 0xC0000011.
 * The operation serializes on the transition lock and can perform disk I/O.
 * \sa POWER_INTERNAL_READ_HIBERFILE_PAGE_OUTPUT
 * \sa POWER_INTERNAL_HIBERFILE_PAGE_OFFSET
 */
typedef struct _POWER_INTERNAL_READ_HIBERFILE_PAGE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG PageNumber;
} POWER_INTERNAL_READ_HIBERFILE_PAGE_INPUT, *PPOWER_INTERNAL_READ_HIBERFILE_PAGE_INPUT;

/**
 * The POWER_INTERNAL_HIBERFILE_PAGE_OFFSET macro computes the byte offset of a raw page in the hibernation file.
 * \param PageNumber A ULONG file-page index, evaluated once.
 * \return A ULONGLONG equal to PageNumber * 4096, without 32-bit multiplication overflow.
 * \remarks This is a file offset, not a physical page frame number or virtual address.
 * The kernel separately checks file availability and the end of the requested page.
 */
#define POWER_INTERNAL_HIBERFILE_PAGE_OFFSET(PageNumber) ((ULONGLONG)(PageNumber) * 4096)

// rev
/**
 * The POWER_INTERNAL_READ_HIBERFILE_PAGE_OUTPUT struct receives exactly one raw hibernation-file page on the AMD64 path.
 * \remarks Selector 61 copies 4096 bytes after a successful read. PageData is caller-owned
 * output, not an MDL, mapped view, or decrypted/decompressed memory image. Read only on
 * success. The declaration uses PAGE_SIZE; the AMD64 value and wire size are4096.
 * \sa POWER_INTERNAL_READ_HIBERFILE_PAGE_INPUT
 */
typedef struct _POWER_INTERNAL_READ_HIBERFILE_PAGE_OUTPUT
{
    UCHAR PageData[PAGE_SIZE];
} POWER_INTERNAL_READ_HIBERFILE_PAGE_OUTPUT, *PPOWER_INTERNAL_READ_HIBERFILE_PAGE_OUTPUT;

#ifdef _WIN64
#endif

// rev
// Internal power query layouts are implementation-dependent.
// NtPowerInformation(PowerInformationInternal) requires the 8-byte internal header,
// including for header-only queries. Use Version 0 unless documented otherwise.
/**
 * The POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_INPUT struct selects the kernel inbox-PEP constraint override diagnostic query.
 * \remarks Selector 67 uses at least this 8-byte header and a 4-byte output. Initialize
 * Version to0 by convention; the route does NOT validate it. User mode receives
 * STATUS_ACCESS_DENIED. The returned DWORD is an override value, not a feature-bit list.
 * No direct output is technically required by the branch, but omit it only when no result
 * is wanted; successful status alone does not reveal the value.
 * \sa POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_OUTPUT
 */
typedef struct _POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_INPUT, *PPOWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_INPUT;

// rev
/**
 * The POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_OUTPUT struct reports the raw inbox-PEP generated-constraint override value.
 * \remarks Kernel infoclass 67 returns4 bytes copied from
 * PopDisableInboxPepGeneratedConstraintsOverride. Use it to compare override state while
 * investigating constraints. No hardware capability-mask or Boolean interpretation is
 * established, and this query does not change the override. Read only on successful query.
 * \sa POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_INPUT
 */
typedef struct _POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_OUTPUT
{
    ULONG DisableInboxPepGeneratedConstraintsOverride;
} POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_OUTPUT, *PPOWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_AUTOCHK_CAUSED_REBOOT_OUTPUT struct reports the captured boot-status indication that AutoChk caused a reboot.
 * \remarks Selector 50 returns one byte from PopAutoChkCausedReboot, initialized from
 * bit 0x10 at byte 14 of the boot power-transition record. Read CausedReboot only after
 * successful query. FALSE is not a general guarantee that the previous shutdown was clean.
 * No handles or buffers are owned by this result.
 * \sa POWER_INTERNAL_AUTOCHK_CAUSED_REBOOT_INPUT
 */
typedef struct _POWER_INTERNAL_AUTOCHK_CAUSED_REBOOT_OUTPUT
{
    BOOLEAN CausedReboot;
} POWER_INTERNAL_AUTOCHK_CAUSED_REBOOT_OUTPUT, *PPOWER_INTERNAL_AUTOCHK_CAUSED_REBOOT_OUTPUT;

/**
 * The POWER_INTERNAL_LONG_POWER_BUTTON_BUGCHECK_INPUT type selects the long-power-button bugcheck configuration query (infoclass 49).
 * \remarks Supply at least this 8-byte header and one output byte. Version is not
 * checked. The query does not press a button or trigger a bugcheck.
 * \sa POWER_INTERNAL_LONG_POWER_BUTTON_BUGCHECK_OUTPUT
 */
typedef POWER_INFORMATION_INTERNAL_HEADER POWER_INTERNAL_LONG_POWER_BUTTON_BUGCHECK_INPUT, *PPOWER_INTERNAL_LONG_POWER_BUTTON_BUGCHECK_INPUT;

/**
 * The POWER_INTERNAL_LONG_POWER_BUTTON_BUGCHECK_OUTPUT struct reports whether long-power-button bugcheck configuration equals 2.
 * \remarks Selector 49 returns one Boolean. The helper refreshes its cached enabled
 * flag and normalizes cached live-dump timeout configuration while querying; this is
 * not a completely side-effect-free read, but it does not execute a bugcheck.
 * It returns no timeout or live-dump payload. Read Enabled only on successful status.
 * \sa POWER_INTERNAL_LONG_POWER_BUTTON_BUGCHECK_INPUT
 */
typedef struct _POWER_INTERNAL_LONG_POWER_BUTTON_BUGCHECK_OUTPUT
{
    BOOLEAN Enabled;
} POWER_INTERNAL_LONG_POWER_BUTTON_BUGCHECK_OUTPUT, *PPOWER_INTERNAL_LONG_POWER_BUTTON_BUGCHECK_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_TIME_BROKER_EXPIRATION_REASON_INPUT struct records a TimeBroker wake-expiration reason and its due time for wake-source attribution.
 * \remarks Selector 44 takes at least 144 bytes and NULL/0 output. User mode must have
 * membership in PopTimeBrokerServiceSid, and the platform must support AoAc; kernel mode
 * returns STATUS_NOT_SUPPORTED. Reason at +8 has room for 64 WCHARs, including a required
 * terminating NUL (at most 63 non-NUL characters). DueTime is at +136. An empty reason
 * requires DueTime=0, and a nonempty reason requires a nonzero DueTime; mismatches are invalid.
 *
 * The handler copies the reason/time under the policy lock; it does not create a timer.
 * The wake-attribution consumer compares DueTime on the KUSER_SHARED_DATA.SystemTime clock,
 * accepting a matching window from DueTime through DueTime+20000000 clock units, otherwise
 * using Unknown. Do not pass an interrupt-time or relative-duration value. Version is not
 * checked. A clear request uses an empty reason and zero due time.
 * \sa POWER_INFORMATION_INTERNAL_HEADER
 */
typedef struct _POWER_INTERNAL_TIME_BROKER_EXPIRATION_REASON_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    WCHAR Reason[64];
    ULONGLONG DueTime;
} POWER_INTERNAL_TIME_BROKER_EXPIRATION_REASON_INPUT, *PPOWER_INTERNAL_TIME_BROKER_EXPIRATION_REASON_INPUT;

// rev
/**
 * The POWER_INTERNAL_POWER_REQUEST_TERMINAL_CORE_WINDOW_INPUT struct attributes an existing process power-request handle to a terminal core-window value.
 * \remarks User-mode infoclass 46 requires exactly 32 input bytes on AMD64 and NULL/0 output.
 * PowerRequestHandle at +8 and ProcessId at +24 identify a matching request in the global
 * request list. CoreWindowHandle at +16 becomes that request's window attribute, with TTM
 * notification when enabled. The kernel matches values; it does not interpret +8 as a process
 * handle or +16 as a power-request object pointer. TraceLogging metadata corroborates the
 * PowerRequestHandle, CoreWindowHandle, and ProcessId semantics.
 *
 * No handle is created, referenced by this lookup, or transferred to the caller; do not close
 * a core-window value with NtClose. Missing matches are silent and the dispatcher still
 * returns success. Version is not checked. Kernel mode returns STATUS_NOT_SUPPORTED.
 * \sa POWER_INFORMATION_INTERNAL_HEADER
 */
typedef struct _POWER_INTERNAL_POWER_REQUEST_TERMINAL_CORE_WINDOW_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    union
    {
        HANDLE PowerRequestHandle; // lookup key together with ProcessId
    };
    union
    {
        HANDLE CoreWindowHandle; // attributed window value, not a kernel power-request pointer
    };
    union
    {
        ULONG ProcessId; // identifies the process owning PowerRequestHandle
    };
    ULONG Reserved;
} POWER_INTERNAL_POWER_REQUEST_TERMINAL_CORE_WINDOW_INPUT, *PPOWER_INTERNAL_POWER_REQUEST_TERMINAL_CORE_WINDOW_INPUT;

#ifdef _WIN64
#endif

// rev
/**
 * The POWER_INTERNAL_WAKE_ON_VOICE_STATE_INPUT struct publishes the reported wake-on-voice state to its WNF notification channel.
 * \remarks User-mode infoclass 39 requires exactly 12 input bytes and publishes the raw Enabled byte at +8 as
 * a 1-byte WNF_PO_WAKE_ON_VOICE_STATE update. Use FALSE/TRUE to report disabled/enabled. This is not an audio
 * capture request or proof that hardware wake routing has been armed. The WNF update status is propagated to
 * the caller.
 *
 * Use NULL/0 output: a supplied output is ignored by this route. Kernel-mode calls return
 * STATUS_NOT_SUPPORTED, and Version is not checked. No infoclass-specific user privilege or capability check
 * is present in dispatcher for this operation.
 * \sa POWER_INFORMATION_INTERNAL_HEADER
 */
typedef struct _POWER_INTERNAL_WAKE_ON_VOICE_STATE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    BOOLEAN Enabled; // 1 = enable Wake on Voice, 0 = disable
} POWER_INTERNAL_WAKE_ON_VOICE_STATE_INPUT, *PPOWER_INTERNAL_WAKE_ON_VOICE_STATE_INPUT;

// rev
/**
 * The POWER_INTERNAL_DEEP_SLEEP_BLOCK_INPUT struct sets or clears the kernel's fixed deep-sleep disengagement reason 7.
 * \remarks Selector 40 requires exactly 16 input bytes, not the previous 12-byte shape.
 * Reserved at offset 8 must be zero and Block at offset 12 selects set (nonzero) or clear
 * (zero). This is a bit in a shared constraint mask, not a reference-counted per-caller
 * request. Updating it can queue deep-sleep reevaluation; it does not suspend the system.
 *
 * Kernel mode only; user mode receives STATUS_ACCESS_DENIED. Version is not checked.
 * Use NULL/0 output: a supplied output is ignored rather than filled or rejected. Do not
 * put Block immediately after the 8-byte header, where the required-zero DWORD belongs.
 * \sa POWER_INFORMATION_INTERNAL_HEADER
 */
typedef struct _POWER_INTERNAL_DEEP_SLEEP_BLOCK_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG Reserved; // must be zero 
    BOOLEAN Block; // nonzero sets reason7, zero clears it
    UCHAR Reserved1[3];
} POWER_INTERNAL_DEEP_SLEEP_BLOCK_INPUT, *PPOWER_INTERNAL_DEEP_SLEEP_BLOCK_INPUT;

// rev
/**
 * The POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT struct selects a specific processor for a nominal/branded frequency query.
 * \remarks Selector 43 accepts exactly 12 bytes for this form or exactly 8 bytes for
 * the header-only aggregate form. The 12-byte form uses ProcessorNumber at offset 8;
 * Reserved in PROCESSOR_NUMBER must be zero and the group/number must resolve to a valid
 * processor index. Invalid selection returns STATUS_INVALID_PARAMETER. Provide 8 output
 * bytes. Input Version is not checked; it is not the result Version.
 *
 * The header-only form returns the maximum value across processors in PpmCheckRegistered,
 * not necessarily the current CPU or every configured processor. This is nominal frequency
 * information, not a measurement of current clock, turbo behavior, or workload throughput.
 * \sa POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT_BASIC
 * \sa POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT
 */
typedef struct _POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    PROCESSOR_NUMBER ProcessorNumber; // Optional: provide only when InputBufferLength == sizeof(POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT) (0x0C). Reserved must be 0.
} POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT, *PPOWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT;

/**
 * The POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT_BASIC type selects the maximum branded frequency across registered processors.
 * \remarks Selector 43 with exactly this 8-byte input uses the header-only query.
 * The 12-byte form instead includes one PROCESSOR_NUMBER. This is not a current-CPU
 * query. Input Version is not checked; output uses its own Version field.
 * \sa POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT
 */
typedef POWER_INFORMATION_INTERNAL_HEADER POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT_BASIC, *PPOWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT_BASIC;

#define POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_VERSION 1

// rev
/**
 * The POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT struct returns the nominal/branded frequency selected or accumulated by the processor query.
 * \remarks The 8-byte result holds Version at 0 and NominalFrequency at 4, in MHz.
 * Each accumulated processor contributes its performance-domain NominalFrequency, or
 * KPRCB.MHz when no domain exists; the aggregate result is the maximum, not an average.
 * An accumulated processor sets Version=1. If the header-only path enumerates no registered
 * processors, the zero-initialized Version and frequency can remain 0 despite success.
 * Read both fields after a successful query rather than assuming Version is always 1.
 * \sa POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT
 */
typedef struct _POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT
{
    ULONG Version; // POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_VERSION
    ULONG NominalFrequency; // if (Domain) Prcb->PowerState.CheckContext.Domain.NominalFrequency else Prcb->MHz
} POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT, *PPOWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_SET_WAKE_ALARM_OVERRIDE_INPUT struct supplies separate AC/DC wake-alarm override values for a subsequent power transition.
 * \remarks Selector 51 with Header.Version=1 requires at least 24 bytes and reads the
 * 64-bit AC value at +8 and DC value at +16. Version 0 instead needs only one value in
 * 16 bytes and applies it to both sources; sending this larger structure with Version=0
 * still ignores its DC field. Other versions return STATUS_INVALID_PARAMETER.
 *
 * User-mode administrator context is required; kernel mode returns STATUS_NOT_SUPPORTED.
 * Use NULL/0 output; supplied output is ignored. The handler stores the values, with no
 * immediate hardware programming or time-range validation. Transition code copies them
 * into WakeAlarm[].RequestedTime and clears them after processing the override wake case;
 * zero disables an override. Storage of these values does not establish alarm time units
 * or accepted ranges for subsequent platform programming.
 * \sa POWER_INTERNAL_SET_WAKE_ALARM_OVERRIDE_INPUT_V0
 */
typedef struct _POWER_INTERNAL_SET_WAKE_ALARM_OVERRIDE_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONGLONG WakeAlarmOverrideAc;
    ULONGLONG WakeAlarmOverrideDc;
} POWER_INTERNAL_SET_WAKE_ALARM_OVERRIDE_INPUT, *PPOWER_INTERNAL_SET_WAKE_ALARM_OVERRIDE_INPUT;

#define POWER_INTERNAL_WAKE_ALARM_OVERRIDE_VERSION_0 0
#define POWER_INTERNAL_WAKE_ALARM_OVERRIDE_VERSION_1 1

/**
 * The POWER_INTERNAL_SET_WAKE_ALARM_OVERRIDE_INPUT_V0 struct supplies the same wake-alarm override for AC and DC power sources.
 * \remarks Selector 51 with Header.Version=0 requires at least 16 input bytes and
 * copies WakeAlarmOverride to both source overrides. The 24-byte Version1 form permits
 * distinct AC/DC values; other versions return STATUS_INVALID_PARAMETER.
 * User-mode administrator context is required and kernel mode returns STATUS_NOT_SUPPORTED.
 * Use NULL/0 output; any supplied output is ignored. This updates a later transition's
 * requested alarm time, not an immediate alarm-programming acknowledgement.
 * \sa POWER_INTERNAL_SET_WAKE_ALARM_OVERRIDE_INPUT
 */
typedef struct _POWER_INTERNAL_SET_WAKE_ALARM_OVERRIDE_INPUT_V0
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONGLONG WakeAlarmOverride;
} POWER_INTERNAL_SET_WAKE_ALARM_OVERRIDE_INPUT_V0, *PPOWER_INTERNAL_SET_WAKE_ALARM_OVERRIDE_INPUT_V0;

// rev
/**
 * The PROCESSOR_IDLE_VETO struct retains the legacy processor-idle veto declaration for source compatibility.
 * \remarks This structure is NOT the input or output for PowerInternalProcessorIdleVeto (infoclass 47): that
 * route requires an 8-byte internal header, a global processor index, a state index, and a Boolean veto. This
 * legacy declaration's ProcessorNumber, VetoReason, and Increment fields do not describe the internal wire
 * format. Its independent legacy contract is unspecified; do not wrap it in another header or submit it based
 * only on the similar name.
 * \sa POWER_INTERNAL_PROCESSOR_IDLE_VETO_INPUT
 */
typedef struct _PROCESSOR_IDLE_VETO
{
    ULONG Version;
    PROCESSOR_NUMBER ProcessorNumber;
    ULONG StateIndex;
    ULONG VetoReason;
    UCHAR Increment;
} PROCESSOR_IDLE_VETO, *PPROCESSOR_IDLE_VETO;

// rev
/**
 * The PLATFORM_IDLE_VETO struct retains the legacy platform-idle veto declaration for source compatibility.
 * \remarks This is NOT the input or output for PowerInternalPlatformIdleVeto (infoclass 48).
 * Use its dedicated internal request with header, state index, and Boolean veto instead.
 * The legacy VetoReason/Increment fields do not implement the internal route's set/clear
 * bit semantics. The independent legacy contract is not defined here.
 * \sa POWER_INTERNAL_PLATFORM_IDLE_VETO_INPUT
 */
typedef struct _PLATFORM_IDLE_VETO
{
    ULONG Version;
    ULONG StateIndex;
    ULONG VetoReason;
    UCHAR Increment;
} PLATFORM_IDLE_VETO, *PPLATFORM_IDLE_VETO;

// rev
#define POWER_INTERNAL_IDLE_VETO_VERSION 0

/**
 * The POWER_INTERNAL_PROCESSOR_IDLE_VETO_INPUT struct sets or clears the diagnostic veto bit for one processor idle state.
 * \remarks Selector 47 takes at least 20 bytes, Header.Version=0, and NULL/0 output.
 * It accepts user-mode administrator callers; kernel mode returns STATUS_NOT_SUPPORTED.
 * ProcessorIndex is a global processor index passed to KeGetPrcb, NOT PROCESSOR_NUMBER.
 * StateIndex selects an entry in that processor's idle-state table. Nonzero Veto sets
 * bit31 (0x80000000) and zero clears it; this is not an increment/decrement counter.
 * Invalid processor/state indexes return 0xC00000EF/0xC00000F0 respectively; an absent
 * idle-state table returns 0xC00000AB. No reason-code field is consumed by this route.
 * Use only for controlled idle-policy diagnostics, not to query residency.
 * \sa POWER_INTERNAL_PLATFORM_IDLE_VETO_INPUT
 */
typedef struct _POWER_INTERNAL_PROCESSOR_IDLE_VETO_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONG ProcessorIndex;
    ULONG StateIndex;
    BOOLEAN Veto;
    UCHAR Reserved[3];
} POWER_INTERNAL_PROCESSOR_IDLE_VETO_INPUT, *PPOWER_INTERNAL_PROCESSOR_IDLE_VETO_INPUT;

/**
 * The POWER_INTERNAL_PLATFORM_IDLE_VETO_INPUT struct sets or clears the diagnostic veto bit for a platform idle state.
 * \remarks Selector 48 takes at least 16 bytes, Header.Version=0, and NULL/0 output.
 * It accepts user-mode administrator callers; kernel mode returns STATUS_NOT_SUPPORTED.
 * StateIndex must be within the platform state table. Nonzero Veto sets bit31
 * (0x80000000), zero clears it; no reference count or reason code is updated.
 * An absent table returns 0xC00000AB, an invalid index 0xC00000EF, and nonzero Version
 * STATUS_REVISION_MISMATCH. This changes idle policy rather than reading state.
 * \sa POWER_INTERNAL_PROCESSOR_IDLE_VETO_INPUT
 */
typedef struct _POWER_INTERNAL_PLATFORM_IDLE_VETO_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONG StateIndex;
    BOOLEAN Veto;
    UCHAR Reserved[3];
} POWER_INTERNAL_PLATFORM_IDLE_VETO_INPUT, *PPOWER_INTERNAL_PLATFORM_IDLE_VETO_INPUT;

// rev
/**
 * The POWER_INTERNAL_SYSTEM_IDLE_LOOP_ENABLEMENT_INPUT struct queries whether the kernel's idle-scan interval is enabled using NTSTATUS alone.
 * \remarks Selector 68 needs at least this 8-byte header. PopIdleScanInterval!=0 returns
 * STATUS_SUCCESS; zero returns STATUS_NOT_SUPPORTED. This does not enable the loop or
 * return the numeric interval. Use NULL/0 output; supplied output is ignored, not rejected
 * or filled. Version is not checked and no infoclass-specific caller-mode restriction is
 * present. Use status for diagnostics, not uninitialized output bytes.
 * \sa POWER_INFORMATION_INTERNAL_HEADER
 */
typedef struct _POWER_INTERNAL_SYSTEM_IDLE_LOOP_ENABLEMENT_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_SYSTEM_IDLE_LOOP_ENABLEMENT_INPUT, *PPOWER_INTERNAL_SYSTEM_IDLE_LOOP_ENABLEMENT_INPUT;

// rev
/**
 * The POWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_INPUT struct selects support and frequency-range information for the current processor performance domain.
 * \remarks Selector 69 accepts at least this 8-byte header; Version is not checked. Supply
 * one output byte for support only or at least 20 bytes for the full domain report. Output
 * lengths 1..19 copy only the Boolean; this is not a truncated prefix of the full structure.
 * The query does not enable VM performance control or report every processor domain.
 * \sa POWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_OUTPUT
 */
typedef struct _POWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_INPUT, *PPOWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_INPUT;

// rev
/**
 * The POWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_OUTPUT struct reports VM performance selection support and the current domain's frequency limits.
 * \remarks Selector 69 with output length>=20 copies this 20-byte record: Supported at 0,
 * reserved bytes1..3, and four MHz DWORDs at 4,8,12,16. A length from 1 through 19 copies only
 * Supported and leaves remaining caller bytes unchanged. No-output requests return
 * STATUS_INVALID_PARAMETER. Do not read frequency fields after a short-form call.
 *
 * The helper zero-initializes all 20 bytes under PpmPerfPolicyLock. If VM performance
 * selection is unsupported it returns successful status with an all-zero result; otherwise
 * it sets Supported and copies the current PRCB domain's frequency fields. These are limits
 * and granularity, not percentages or an instantaneous frequency sample. A thread may query
 * a different domain after migration; no system-wide aggregation is performed.
 * \sa POWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_INPUT
 */
typedef struct _POWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_OUTPUT
{
    BOOLEAN Supported; // Only this byte is returned for output lengths 1..19.
    UCHAR Reserved[3];
    ULONG HighestFrequencyMhz;
    ULONG NominalFrequencyMhz;
    ULONG LowestFrequencyMhz;
    ULONG FrequencyStepMhz;
} POWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_OUTPUT, *PPOWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_SLEEP_DETAILED_DIAG_UPDATE_INPUT structure requests a change to the detailed
 * sleep-reliability diagnostic setting.
 * \remarks Set Enable to TRUE or FALSE with PowerInternalSleepDetailedDiagUpdate when an authorized diagnostic
 * component needs to control detailed sleep instrumentation. This is a state-changing request, not a query for
 * sleep statistics; it returns no report and should not be issued merely to detect support.
 *
 * Exact 12-byte request; only byte +8 controls detailed sleep diagnostics.
 * Use FALSE or TRUE. No output payload; pass NULL/0 for output.
 * The dispatcher permits the user-mode LocalSystem path and rejects kernel PreviousMode.
 * \remarks Selector 71 requires exactly 12 input bytes, user-mode LocalSystem context,
 * and stores the raw Enable byte at +8 under PopSleepReliabilityDiagLock, tracing changes.
 * It does not initiate sleep or return diagnostics. Version and reserved bytes are ignored;
 * use FALSE/TRUE convention. Use NULL/0 output: supplied output is ignored, NOT rejected.
 * Kernel mode returns STATUS_NOT_SUPPORTED; other user contexts are denied.
 */
typedef struct _POWER_INTERNAL_SLEEP_DETAILED_DIAG_UPDATE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    BOOLEAN Enable;
    UCHAR Reserved[3];
} POWER_INTERNAL_SLEEP_DETAILED_DIAG_UPDATE_INPUT, *PPOWER_INTERNAL_SLEEP_DETAILED_DIAG_UPDATE_INPUT;

// rev
/**
 * The POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_INPUT structure selects accumulated frequency-band statistics for
 * processor efficiency-class banks.
 * \remarks Use this header with PowerInternalProcessorClassFrequencyBandsStats and a full
 * POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_OUTPUT buffer to query the distribution of accounted cycles across
 * frequency bands and QoS categories. The query aggregates registered processors rather than selecting an
 * individual CPU.
 * \remarks Selector 72 requires at least the 8-byte header and 2304 output bytes.
 * Both caller modes are accepted; Version is ignored. PpmCheckRegistered processors with
 * available performance-check storage contribute under PpmPerfPolicyLock. No new hardware
 * sample is forced; absent contributors can yield a successful all-zero result.
 */
typedef struct _POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_INPUT, *PPOWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_INPUT;

// rev
/**
 * The POWER_INTERNAL_HOST_GLOBAL_USER_PRESENCE_STATE_UPDATE_INPUT structure supplies the host global user-presence
 * state to the adaptive power-management path.
 * \remarks An authorized host integration component can report present, not-present, or inactive state when
 * synchronizing host presence with power policy. Set UserPresenceState to the appropriate USER_ACTIVITY_PRESENCE
 * value and pass no output buffer. This changes reported state; it is not a presence-detection or statistics
 * query.
 *
 * Updates host global user presence with a DWORD state, not a BOOLEAN.
 * Input length must be >= 12; output must be absent. No range check is performed.
 * \remarks Selector 73 needs input >=12 bytes and NULL/0 output; a supplied output
 * is rejected. Both caller modes are accepted with no additional infoclass-specific privilege
 * gate. Version is ignored and the raw DWORD at +8 is not range-checked. Under PopAdpmLock,
 * a changed value updates host presence and reevaluates global user status. This reports
 * host state; it is not input injection, a lock/unlock operation or a presence query.
 */
typedef struct _POWER_INTERNAL_HOST_GLOBAL_USER_PRESENCE_STATE_UPDATE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG UserPresenceState; // USER_ACTIVITY_PRESENCE: present=0, not present=1, inactive=2, invalid=3
} POWER_INTERNAL_HOST_GLOBAL_USER_PRESENCE_STATE_UPDATE_INPUT, *PPOWER_INTERNAL_HOST_GLOBAL_USER_PRESENCE_STATE_UPDATE_INPUT;

// rev
/**
 * The POWER_INTERNAL_IDLE_INTERVAL_STATS_INPUT structure identifies a processor parking node for an idle-interval
 * or concurrency-statistics query.
 * \remarks Select PowerInternalCpuNodeIdleIntervalStats, PowerInternalClassIdleIntervalStats,
 * PowerInternalCpuNodeConcurrencyStats, or PowerInternalClassConcurrencyStats in InternalType and set Node to the
 * requested parking-node infoclass. Choose the matching output layout: idle-interval durations are in 100-ns units,
 * whereas concurrency durations are raw QPC ticks. Do not interpret Node as a logical processor number.
 *
 * Exact 12-byte request shared by internal idle-interval and concurrency queries.
 * Only the USHORT at offset +8 is consumed.
 * \remarks Selectors 74..77 all require exactly 12 bytes and non-NULL output. Both
 * caller modes are accepted; Version and Reserved are ignored. The USHORT Node at +8 is
 * passed with secondary infoclass 0 to PpmParkGetParkNode. It is not a processor index or
 * a group field. Missing parking-node/accounting state yields STATUS_NOT_SUPPORTED.
 * Selector 74 returns 296 bytes, 75 returns 592 bytes; 76/77 return variable-size concurrency
 * records. These are snapshots without a forced new accounting sample.
 */
typedef struct _POWER_INTERNAL_IDLE_INTERVAL_STATS_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    USHORT Node; // Passed to PpmParkGetParkNode; not a logical processor index.
    USHORT Reserved; // The high WORD of the original DWORD infoclass is ignored.
} POWER_INTERNAL_IDLE_INTERVAL_STATS_INPUT, *PPOWER_INTERNAL_IDLE_INTERVAL_STATS_INPUT;

/**
 * The POWER_INTERNAL_CPU_NODE_IDLE_INTERVAL_STATS_INPUT type names the shared request for selecting one node idle-duration histogram.
 * \remarks Selector 74: exactly 12 input bytes, Node at +8; Version ignored.
 * Use the matching result type and units, not a generic interchangeable statistics buffer.
 * \sa POWER_INTERNAL_IDLE_INTERVAL_STATS_INPUT
 */
typedef POWER_INTERNAL_IDLE_INTERVAL_STATS_INPUT POWER_INTERNAL_CPU_NODE_IDLE_INTERVAL_STATS_INPUT, *PPOWER_INTERNAL_CPU_NODE_IDLE_INTERVAL_STATS_INPUT;

/**
 * The POWER_INTERNAL_CLASS_IDLE_INTERVAL_STATS_INPUT type names the shared request for selecting two fixed class idle-duration banks.
 * \remarks Selector 75: exactly 12 input bytes, Node at +8; Version ignored.
 * Use the matching result type and units, not a generic interchangeable statistics buffer.
 * \sa POWER_INTERNAL_IDLE_INTERVAL_STATS_INPUT
 */
typedef POWER_INTERNAL_IDLE_INTERVAL_STATS_INPUT POWER_INTERNAL_CLASS_IDLE_INTERVAL_STATS_INPUT, *PPOWER_INTERNAL_CLASS_IDLE_INTERVAL_STATS_INPUT;

/**
 * The POWER_INTERNAL_CPU_NODE_CONCURRENCY_STATS_INPUT type names the shared request for selecting a node active-processor-count histogram.
 * \remarks Selector 76: exactly 12 input bytes, Node at +8; Version ignored.
 * Use the matching result type and units, not a generic interchangeable statistics buffer.
 * \sa POWER_INTERNAL_IDLE_INTERVAL_STATS_INPUT
 */
typedef POWER_INTERNAL_IDLE_INTERVAL_STATS_INPUT POWER_INTERNAL_CPU_NODE_CONCURRENCY_STATS_INPUT, *PPOWER_INTERNAL_CPU_NODE_CONCURRENCY_STATS_INPUT;

/**
 * The POWER_INTERNAL_CLASS_CONCURRENCY_STATS_INPUT type names the shared request for selecting packed present-class concurrency histograms.
 * \remarks Selector 77: exactly 12 input bytes, Node at +8; Version ignored.
 * Use the matching result type and units, not a generic interchangeable statistics buffer.
 * \sa POWER_INTERNAL_IDLE_INTERVAL_STATS_INPUT
 */
typedef POWER_INTERNAL_IDLE_INTERVAL_STATS_INPUT POWER_INTERNAL_CLASS_CONCURRENCY_STATS_INPUT, *PPOWER_INTERNAL_CLASS_CONCURRENCY_STATS_INPUT;

// PpmPackageIdleIntervalLimits, expressed in 100-ns units.
#define PPM_IDLE_INTERVAL_BUCKET_COUNT 37
#define PPM_IDLE_INTERVAL_BOUNDARY_COUNT (PPM_IDLE_INTERVAL_BUCKET_COUNT - 1)
#define PPM_IDLE_INTERVAL_CLASS_COUNT 2
#define PPM_IDLE_INTERVAL_UPPER_BOUNDS_100NS { \
    10000ULL, 20000ULL, 30000ULL, 40000ULL, 50000ULL, 60000ULL, 70000ULL, 80000ULL, 90000ULL, 100000ULL, \
    110000ULL, 120000ULL, 130000ULL, 140000ULL, 150000ULL, 160000ULL, 170000ULL, 180000ULL, 190000ULL, 200000ULL, \
    320000ULL, 640000ULL, 1280000ULL, 2560000ULL, 5120000ULL, 10000000ULL, 20000000ULL, 40000000ULL, \
    80000000ULL, 160000000ULL, 320000000ULL, 600000000ULL, 1200000000ULL, 2400000000ULL, 4800000000ULL, 9600000000ULL \
}

// rev
/**
 * The POWER_INTERNAL_IDLE_INTERVAL_PACKAGE structure contains the total idle duration accumulated in each
 * idle-interval-length bucket for a node or class bank.
 * \remarks Use the histogram to distinguish short idle opportunities from time accumulated in longer idle
 * intervals when investigating power-saving opportunities. Compare matched snapshots to study a monitoring
 * interval, subject to unchanged counter lifetime and topology. The buckets sum duration, so they cannot directly
 * report the number of wakeups or idle transitions.
 *
 * Histogram of accumulated idle duration, not a count of idle intervals.
 * Buckets are ordered from shortest to longest. Equality moves to the next bucket.
 * The 36 exclusive upper bounds are in PPM_IDLE_INTERVAL_UPPER_BOUNDS_100NS;
 * bucket 36 covers durations >= 9600000000 100-ns units (960 seconds).
 * \remarks Selector 74 copies 37 QWORDs (296 bytes), converting each from accounting
 * time to 100-ns units. No count/version header precedes the array. Unlike infoclass 76,
 * these are interval-length buckets, not bins indexed by active-processor count.
 */
typedef struct _POWER_INTERNAL_IDLE_INTERVAL_PACKAGE
{
    ULONGLONG IdleIntervals[PPM_IDLE_INTERVAL_BUCKET_COUNT]; // Summed idle duration in 100-ns units.
} POWER_INTERNAL_IDLE_INTERVAL_PACKAGE, *PPOWER_INTERNAL_IDLE_INTERVAL_PACKAGE;

// rev
/**
 * The POWER_INTERNAL_IDLE_INTERVAL_STATS_OUTPUT structure contains the two class-specific idle-interval histograms
 * returned for the selected parking node.
 * \remarks Use the class banks to compare short- and long-idle residency patterns across tracked classes when
 * diagnosing asymmetric activity or idle opportunities. A zero bank can reflect absent class storage rather than
 * proof of a class that never idles; retain the topology and query status when interpreting the result.
 *
 * Two class banks, each containing 37 ULONGLONG durations in 100-ns units.
 * Missing class storage stays zero; no usable class statistics yields STATUS_NOT_SUPPORTED.
 * \remarks Selector 75 always returns 592 bytes: two fixed 296-byte bank positions.
 * Unlike packed class concurrency output, a missing class leaves its bank zero without
 * shifting the following bank. Only the first two class slots are copied; no usable class
 * accounting yields STATUS_NOT_SUPPORTED. Do not treat zero as proof that a class exists.
 */
typedef struct _POWER_INTERNAL_IDLE_INTERVAL_STATS_OUTPUT
{
    POWER_INTERNAL_IDLE_INTERVAL_PACKAGE Class[PPM_IDLE_INTERVAL_CLASS_COUNT];
} POWER_INTERNAL_IDLE_INTERVAL_STATS_OUTPUT, *PPOWER_INTERNAL_IDLE_INTERVAL_STATS_OUTPUT;

// rev
#define PPM_PERF_BANKS_COUNT 2
#define PPM_PERF_BANDS_COUNT 48
#define PPM_PERF_METRICS_COUNT 3
#define PPM_PERF_BANDS_SIZE (PPM_PERF_METRICS_COUNT * PPM_PERF_BANDS_COUNT * sizeof(ULONGLONG)) // One 0x480-byte class bank
#define PPM_PERF_STATS_SIZE (PPM_PERF_BANKS_COUNT * PPM_PERF_BANDS_SIZE) // Full 0x900-byte response
#define PPM_PERF_DELTA_OFFSET 0xF8 // _PROC_PERF_CHECK.CyclesByFreqBand; internal offset, not wire offset
#define PPM_PERF_FREQUENCY_BAND_MHZ 100

typedef enum _POWER_INTERNAL_FREQUENCY_QOS_BUCKET
{
    PowerInternalFrequencyQosHigh = 0,
    PowerInternalFrequencyQosLow = 1,
    PowerInternalFrequencyQosTotal = 2
} POWER_INTERNAL_FREQUENCY_QOS_BUCKET;

// rev
/**
 * The POWER_INTERNAL_PPM_PERF_FREQUENCY_BAND_STATS_BANK structure represents one efficiency-class bank of
 * accumulated cycles indexed by QoS category and frequency band.
 * \remarks CyclesByFreqBand records execution cycles by frequency band for the High, Low, and Total QoS indices. Each bank has 3 rows of 48 counters. This layout can support frequency-distribution diagnostics,
 * but its counters are not wall-clock residency times and its Total row must not be added to its component rows as
 * independent work.
 */
typedef struct _POWER_INTERNAL_PPM_PERF_FREQUENCY_BAND_STATS_BANK
{
    ULONGLONG CyclesByFreqBand[PPM_PERF_METRICS_COUNT][PPM_PERF_BANDS_COUNT];
} POWER_INTERNAL_PPM_PERF_FREQUENCY_BAND_STATS_BANK, *PPOWER_INTERNAL_PPM_PERF_FREQUENCY_BAND_STATS_BANK;

// rev
/**
 * The POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS structure contains one class bank of processor-cycle accounting
 * grouped by QoS category and frequency band.
 * \remarks Use the bank to compare the distribution of accounted cycles between frequency ranges or QoS
 * categories. Matched snapshot differences can characterize an observation interval, provided counters have not
 * reset and the contributing processor set is unchanged. The data does not directly measure energy consumption or
 * instantaneous frequency.
 *
 * One 0x480-byte class bank: QoS rows High, Low, Total; 48 frequency bands per row.
 * Band index is min(FrequencyMhz / 100, 47); band 47 includes >= 4700 MHz.
 * Values are cycle accumulations, not durations or sample counts.
 * \remarks The stride is 1152 bytes per class bank: 3 rows * 48 QWORD bands.
 * Within a row the band stride is 8 bytes, and row stride is 384 bytes. Use the row/bank
 * constants rather than transposing to a [48][3] layout. This snapshot does not reset counters.
 */
typedef struct _POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS
{
    ULONGLONG CyclesByFreqBand[PPM_PERF_METRICS_COUNT][PPM_PERF_BANDS_COUNT];
} POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS, *PPOWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS;

// rev
/**
 * The POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_OUTPUT structure contains aggregated frequency-band cycle
 * statistics for zero and nonzero processor efficiency classes.
 * \remarks Use the two banks to investigate differences in frequency and QoS accounting between efficiency-class
 * groups. Keep the class-bank, QoS-row, and frequency-band dimensions separate when plotting or comparing
 * counters; the nonzero bank may combine more than one nonzero efficiency-class value.
 *
 * The 0x900-byte frequency-band response aggregates two efficiency-class banks.
 * Bank 0 is EfficiencyClass == 0; bank 1 is EfficiencyClass != 0.
 * Wire order is [class bank][QoS row][frequency band], not [band][metric].
 * \remarks Selector 72 returns 2304 bytes with no header/count/version prefix. The
 * second bank begins at 1152. EfficiencyClass zero/nonzero is a grouping, not a guarantee
 * of exactly two physical CPU kinds. Match topology and counter lifetime before taking deltas.
 */
typedef struct _POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_OUTPUT
{
    POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS Class[PPM_PERF_BANKS_COUNT];
} POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_OUTPUT, *PPOWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_ADAPTIVE_SESSION_STATE_REQUEST struct supplies the timing and activity context used to evaluate adaptive session power timeouts.
 * \remarks A 32-byte inline record, consumed at offset 8 of infoclass 81's 40-byte request. LastInputTimestamp
 * at 0 is the last-input tick time in milliseconds, compared to the shared tick clock; OverrideResetTimestamp
 * at 8 is the timestamp cutoff for clearing older console overrides; LastDisplayOffTimestamp at 16 updates
 * the inactive-reference time and can clear older console overrides. DisplayState at 24 corresponds to the
 * traced SessionDisplayState field and represents the session display state, not an input-active Boolean.
 * LastInputTimestamp, LastDisplayOffTimestamp and SessionDisplayState are corroborated by Kernel-Power event
 * 576, version 1. OverrideResetTimestamp is a descriptive, consumer-derived name. Reserved at 28 is not
 * consumed by helpers; initialize it to zero.
 *
 * The session identity comes from the current process, not this record. Use only coherent same-clock
 * timestamps from the session integration path; this is not a QPC or FILETIME request. Consumer time
 * differences are divided by 1000 to produce seconds. This record is also embedded in a later
 * session-connection request; its wire shape is unchanged here.
 * \sa POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_INPUT
 */
typedef struct _POWER_INTERNAL_ADAPTIVE_SESSION_STATE_REQUEST
{
    ULONGLONG LastInputTimestamp;
    ULONGLONG OverrideResetTimestamp;
    ULONGLONG LastDisplayOffTimestamp;
    MONITOR_DISPLAY_STATE DisplayState;
    ULONG Reserved;
} POWER_INTERNAL_ADAPTIVE_SESSION_STATE_REQUEST, *PPOWER_INTERNAL_ADAPTIVE_SESSION_STATE_REQUEST;

// rev
/**
 * The POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_INPUT struct evaluates adaptive display, dimming, and input timeouts for the current process session.
 * \remarks Selector 81 is kernel-only: user-mode calls return STATUS_ACCESS_DENIED. Input
 * must be exactly 40 bytes and direct output exactly 16 bytes. Version is not checked.
 * The 32-byte SessionStateRequest begins at +8 and requires 8-byte alignment.
 *
 * This is not a side-effect-free query: under the adaptive lock, console/remote consumers
 * update session user status, and console evaluation can clear stale overrides, update cached
 * timestamps and publish console video-timeout settings. Use it for session power integration,
 * not an arbitrary-session monitoring query. No handles or nested pointer ownership are involved.
 * \sa POWER_INTERNAL_ADAPTIVE_SESSION_STATE_REQUEST
 * \sa POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_OUTPUT
 */
typedef struct _POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    POWER_INTERNAL_ADAPTIVE_SESSION_STATE_REQUEST SessionStateRequest;
} POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_INPUT, *PPOWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_INPUT;

// rev
/**
 * The POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_OUTPUT struct receives adaptive session timeout decisions in seconds plus a one-byte suppression flag.
 * \remarks Selector 81 requires exactly 16 output bytes. DisplayTimeout, DimTimeout, and
 * InputTimeout are DWORDs at 0, 4, and 8. Only the Boolean at +12 is written for the final
 * field: bytes +13..15 remain caller contents. Initialize the output before calling and read
 * InputTimeoutSuppressed.
 *
 * Remote evaluation sets display/dim timeouts to zero; input timeout becomes zero when inactive
 * or expired. Console decisions incorporate display/lock/override policy. The flag is also set
 * by some console overrides, so it is not a universal test for InputTimeout==0 or a persistent
 * configuration Boolean. Results are decisions, not elapsed-time counters or an instruction to
 * change security/session locking. No returned allocation is owned by the caller.
 * \sa POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_INPUT
 */
typedef struct _POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_OUTPUT
{
    ULONG DisplayTimeout; // seconds
    ULONG DimTimeout; // seconds
    ULONG InputTimeout; // seconds
    struct
    {
        BOOLEAN InputTimeoutSuppressed; // only this byte at +12 is written
        UCHAR Reserved[3]; // untouched by the callees
    };
} POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_OUTPUT, *PPOWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_SET_CONSOLE_LOCKED_STATE_INPUT structure contains the input parameters for the set console locked state operation.
 * \remarks Selector 82 requires exactly 12 bytes and NULL/0 output. User mode requires
 * testsigning; kernel mode returns STATUS_NOT_SUPPORTED. There is no extra administrator,
 * LocalSystem or shutdown-privilege gate in this infoclass path. Version is ignored. The byte
 * at +8 is sent as a console Winlogon notification for the current process session, normalized
 * by the consumer, and published as GUID_CONSOLE_LOCKED. This updates power-manager accounting;
 * it does not lock/unlock the secure desktop. Downstream setting status is discarded.
 */
typedef struct _POWER_INTERNAL_SET_CONSOLE_LOCKED_STATE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    BOOLEAN Locked; // 1 if console is locked, 0 if unlocked
} POWER_INTERNAL_SET_CONSOLE_LOCKED_STATE_INPUT, *PPOWER_INTERNAL_SET_CONSOLE_LOCKED_STATE_INPUT;

#define PPM_FAN_RPM_BUCKET_COUNT 17
#define PPM_FAN_IMPACT_BUCKET_COUNT (PPM_FAN_RPM_BUCKET_COUNT + 2)
#define PPM_FAN_NOISE_ZONE_COUNT 4

typedef enum _POWER_INTERNAL_FAN_NOISE_ZONE
{
    PowerInternalFanNoiseLow = 0,
    PowerInternalFanNoiseMedium = 1,
    PowerInternalFanNoiseMediumHigh = 2,
    PowerInternalFanNoiseHigh = 3
} POWER_INTERNAL_FAN_NOISE_ZONE;

// rev
/**
 * The POWER_INTERNAL_FAN_IMPACT_STATS_INPUT structure selects the duration histogram for the eligible fan tracked
 * by the kernel fan-noise path.
 * \remarks Use PowerInternalFanImpactStats with a full POWER_INTERNAL_FAN_IMPACT_STATS_OUTPUT buffer to query
 * time accumulated in stopped and RPM-range buckets. Query POWER_INTERNAL_FAN_RPM_OUTPUT separately to obtain the
 * corresponding RPM boundaries; this request does not set fan speed.
 * \remarks Selector 84 takes at least this 8-byte header and at least 160 output bytes.
 * Both caller modes are accepted; Version is ignored. Exactly one eligible noise-accounting
 * fan must exist; zero or multiple eligible fans returns STATUS_UNSUCCESSFUL. This is not a
 * fan-indexed query. It updates pending duration accounting before copying the first eligible
 * fan's counters, even if a later fan makes the operation fail. It does not set fan speed.
 */
typedef struct _POWER_INTERNAL_FAN_IMPACT_STATS_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_FAN_IMPACT_STATS_INPUT, *PPOWER_INTERNAL_FAN_IMPACT_STATS_INPUT;

// rev
/**
 * The POWER_INTERNAL_FAN_IMPACT_STATS_OUTPUT structure contains accumulated fan-speed-bucket durations updated by
 * the kernel fan-statistics path.
 * \remarks Combine the duration slots with POWER_INTERNAL_FAN_RPM_OUTPUT boundaries to describe how long the fan
 * was stopped or operated in each RPM range. Differences between matched snapshots can help investigate sustained
 * fan activity, provided bucket definitions and the fan instance remain unchanged. The result does not directly
 * measure acoustic loudness or identify the cause of fan activity.
 *
 * Fixed 0xA0-byte result from PopFanReadFanNoiseInfo, refreshed before copying.
 * Bucket 0 is stopped; 1..N are RPM ranges; N+1 is above the last RPM bound.
 * Exactly one eligible fan is required, otherwise STATUS_UNSUCCESSFUL is returned.
 * The entries are durations, not impact samples, dB values, or speed-change counts.
 * \remarks Selector 84 copies the fixed 160-byte result: count DWORD at 0, zero padding
 * at 4, and room for 19 QWORD durations at 8. BucketCountPlusTwo is the number of active entries,
 * not a count to which the caller should add another 2. Ignore zero-filled unused slots.
 * Bin 0 accounts RPM 0, bins 1..N use the fan's inclusive RPM upper bounds, and bin N+1
 * accounts RPM above the last bound; N is at most 17. The query advances statistics using
 * shared interrupt time in 100-ns units, not acoustic decibels, energy, samples or QPC ticks.
 * Read only after success and preserve fan identity/counter lifetime when comparing deltas.
 */
typedef struct _POWER_INTERNAL_FAN_IMPACT_STATS_OUTPUT
{
    _Field_range_(0, PPM_FAN_IMPACT_BUCKET_COUNT) ULONG BucketCountPlusTwo;
    ULONG Reserved;
    _Field_size_part_(PPM_FAN_IMPACT_BUCKET_COUNT, BucketCountPlusTwo)
    ULONGLONG Buckets[PPM_FAN_IMPACT_BUCKET_COUNT]; // Accumulated duration in 100-ns units.
} POWER_INTERNAL_FAN_IMPACT_STATS_OUTPUT, *PPOWER_INTERNAL_FAN_IMPACT_STATS_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_FAN_RPM_BUCKETS_INPUT struct selects the RPM boundaries used to interpret the fan-duration histogram.
 * \remarks Selector 85 accepts at least this 8-byte header and at least 88 output bytes.
 * Both caller modes are accepted; Version is ignored. Exactly one eligible noise-accounting
 * fan must exist, otherwise STATUS_UNSUCCESSFUL. No fan index is accepted. This query copies
 * boundaries without changing speed or refreshing duration counters; pair with infoclass 84
 * only when the same fan and definitions remain in use.
 * \sa POWER_INTERNAL_FAN_RPM_OUTPUT
 */
typedef struct _POWER_INTERNAL_FAN_RPM_BUCKETS_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_FAN_RPM_BUCKETS_INPUT, *PPOWER_INTERNAL_FAN_RPM_BUCKETS_INPUT;

// rev
/**
 * The POWER_INTERNAL_FAN_RPM_OUTPUT struct reports fan-speed histogram boundaries and four noise-zone RPM thresholds.
 * \remarks The 88-byte result has NumberOfFanRpmBuckets at 0, 17 ULONG BucketMaxRpm slots
 * at 4, and 4 ULONG NoiseZoneMaxRpm slots at 72. Only the first NumberOfFanRpmBuckets bounds
 * are meaningful; do not assume unused copied slots are zero. The four zone positions are
 * Low, Medium, MediumHigh and High. All are RPM, not decibels or accumulated durations.
 *
 * For infoclass 84's histogram, bin 0 is RPM 0; bins 1..N have inclusive upper bounds here,
 * and bin N+1 is above the final bound. N must fit the 17-slot capacity. A failed query must
 * not be used as a partial inventory when zero or multiple eligible fans exist.
 * \sa POWER_INTERNAL_FAN_IMPACT_STATS_OUTPUT
 */
typedef struct _POWER_INTERNAL_FAN_RPM_OUTPUT
{
    _Field_range_(0, PPM_FAN_RPM_BUCKET_COUNT) ULONG NumberOfFanRpmBuckets;
    _Field_size_part_(PPM_FAN_RPM_BUCKET_COUNT, NumberOfFanRpmBuckets)
    ULONG BucketMaxRpm[PPM_FAN_RPM_BUCKET_COUNT];
    ULONG NoiseZoneMaxRpm[PPM_FAN_NOISE_ZONE_COUNT]; // Low, Medium, MediumHigh, High.
} POWER_INTERNAL_FAN_RPM_OUTPUT, *PPOWER_INTERNAL_FAN_RPM_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_BOOTAPP_DIAGNOSTIC struct reports the captured boot application's diagnostic code and failure status.
 * \remarks Selector 86 accepts an 8-byte header and returns 8 bytes in either caller mode;
 * Version is ignored. DWORDs at 0/4 come from ExBootAppErrorDiagCode/ExBootAppFailureStatus.
 * The latter is an NTSTATUS bit pattern. These globals describe captured boot diagnostics,
 * not a fresh boot-integrity check or a request to run bcdedit. No handles or owned allocations
 * are returned. Interpret only after successful query; zero is not proof of overall health.
 * \sa POWER_INTERNAL_BOOTAPP_DIAGNOSTIC_INPUT
 */
typedef struct _POWER_INTERNAL_BOOTAPP_DIAGNOSTIC
{
    ULONG BootAppErrorDiagCode; // ExBootAppErrorDiagCode (bcdedit last boot error diagnostic code)
    ULONG BootAppFailureStatus; // ExBootAppFailureStatus (bcdedit last boot failure NTSTATUS)
} POWER_INTERNAL_BOOTAPP_DIAGNOSTIC, *PPOWER_INTERNAL_BOOTAPP_DIAGNOSTIC;

/**
 * The POWER_INTERNAL_BOOTAPP_DIAGNOSTIC_INPUT type names the common 8-byte input for captured boot-application diagnostics.
 * \remarks Selector 86, Version ignored, output 8 bytes; both caller modes accepted.
 * \sa POWER_INTERNAL_BOOTAPP_DIAGNOSTIC
 */
typedef POWER_INFORMATION_INTERNAL_HEADER POWER_INTERNAL_BOOTAPP_DIAGNOSTIC_INPUT, *PPOWER_INTERNAL_BOOTAPP_DIAGNOSTIC_INPUT;

// rev
/**
 * The POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_INPUT struct selects the cached ACPI time-and-alarm device capability query.
 * \remarks Selector 89 needs only the 8-byte common header, not the full historical
 * 20-byte declaration. The retained Reserved[12] tail is unused; a BASIC alias exposes the
 * minimum form. Both caller modes are accepted and Version is ignored. Output must be
 * exactly 20 bytes (even a larger output is rejected with STATUS_BUFFER_TOO_SMALL).
 * The device-capability availability gate can return STATUS_NOT_SUPPORTED.
 * \sa POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_INPUT_BASIC
 * \sa POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_OUTPUT
 */
typedef struct _POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    UCHAR Reserved[12];
} POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_INPUT, *PPOWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_INPUT;

/**
 * The POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_INPUT_BASIC type is the minimum request form for the cached ACPI time-and-alarm capabilities query.
 * \remarks Selector 89 needs only this 8-byte header, with exactly 20 output bytes.
 * The longer historical input structure remains accepted; its tail is ignored.
 */
typedef POWER_INFORMATION_INTERNAL_HEADER POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_INPUT_BASIC, *PPOWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_INPUT_BASIC;

// rev
/**
 * The POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_OUTPUT struct reports cached AC/DC wake and real-time-clock capabilities from the ACPI alarm device.
 * \remarks Selector 89 returns exactly 20 bytes under the policy lock. The producer sends
 * IOCTL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES (0x29421C) with a 20-byte result, matching the SDK
 * ACPI_TIME_AND_ALARM_CAPABILITIES layout: seven Boolean bytes at 0..6, deepest wake state
 * DWORD at 8, real-time-feature Boolean at 12 and resolution DWORD at 16. Resolution 0 means
 * milliseconds and 1 means seconds. Reserved padding is not a capability bitset.
 *
 * Use these flags to distinguish supported wake sources/states and clock resolution, not to
 * arm an alarm or infer current wake policy. This is a cached device report: a producer I/O
 * failure can leave zero capability fields even while the availability gate is set.
 * No pointer or handle ownership is transferred.
 */
typedef struct _POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_OUTPUT
{
    struct
    {
        BOOLEAN AcWakeSupported;
        BOOLEAN DcWakeSupported;
        BOOLEAN S4AcWakeSupported;
        BOOLEAN S4DcWakeSupported;
        BOOLEAN S5AcWakeSupported;
        BOOLEAN S5DcWakeSupported;
        BOOLEAN S4S5WakeStatusSupported;
        UCHAR Reserved0;
        ULONG DeepestWakeSystemState;
        BOOLEAN RealTimeFeaturesSupported;
        UCHAR Reserved1[3];
        ULONG RealTimeResolution; // 0 milliseconds, 1 seconds (ACPI_TIME_RESOLUTION)
    };
} POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_OUTPUT, *PPOWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_OUTPUT;

typedef enum _POWER_INTERNAL_SOC_IDENTIFIER_ACTION
{
    PowerInternalSocIdentifierQuerySize = 0,
    PowerInternalSocIdentifierQueryString = 1
} POWER_INTERNAL_SOC_IDENTIFIER_ACTION;

// rev
/**
 * The POWER_INTERNAL_SOC_IDENTIFIER_OPERATION_INPUT struct selects the required capacity or contents of the registered provisioned SoC identifier.
 * \remarks Selector 92 requires at least 24 bytes in either caller mode. Action at 8
 * accepts 0 (size) or 1 (string); other actions are invalid. Version and bytes 12..23 are
 * ignored. An empty/unregistered identifier returns STATUS_NOT_SUPPORTED. This retrieves a
 * provisioned label; it is not proof of hardware identity or an authentication credential.
 *
 * Action 0 requires at least 2 output bytes; Action 1 requires the returned MaximumLengthBytes.
 * NULL/0 is not a size probe. Parent dispatch copies from borrowed global storage, without
 * freeing it or returning a kernel pointer. The caller owns only its output buffer.
 * \sa POWER_INTERNAL_SOC_IDENTIFIER_SIZE_OUTPUT
 * \sa POWER_INTERNAL_SOC_IDENTIFIER_OPERATION_OUTPUT
 */
typedef struct _POWER_INTERNAL_SOC_IDENTIFIER_OPERATION_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG Action; // POWER_INTERNAL_SOC_IDENTIFIER_ACTION
    ULONG Reserved[3]; // not consumed by this handler.
} POWER_INTERNAL_SOC_IDENTIFIER_OPERATION_INPUT, *PPOWER_INTERNAL_SOC_IDENTIFIER_OPERATION_INPUT;

/**
 * The POWER_INTERNAL_SOC_IDENTIFIER_SIZE_OUTPUT struct reports the byte capacity of the registered UTF-16 SoC identifier including NUL storage.
 * \remarks Selector 92 Action 0 returns exactly one USHORT (2 bytes). MaximumLengthBytes
 * is copied from the global UNICODE_STRING.MaximumLength, not Length or a WCHAR count.
 * Registration duplicates the source with NUL termination. Allocate this many bytes for
 * Action 1 and check successful status before treating them as text. No extra prefix belongs
 * in that allocation, and this result does not transfer ownership of the global string.
 */
typedef struct _POWER_INTERNAL_SOC_IDENTIFIER_SIZE_OUTPUT
{
    USHORT MaximumLengthBytes; // Action 0: byte size, not WCHAR count; includes NUL storage.
} POWER_INTERNAL_SOC_IDENTIFIER_SIZE_OUTPUT, *PPOWER_INTERNAL_SOC_IDENTIFIER_SIZE_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_SOC_IDENTIFIER_OPERATION_OUTPUT struct receives the copied, NUL-terminated provisioned SoC identifier without a length prefix.
 * \remarks Selector 92 Action 1 returns MaximumLengthBytes bytes obtained from Action 0.
 * Identifier is inline UTF-16, not UNICODE_STRING or an owned kernel pointer. Use bounded
 * string parsing within the reported byte capacity. sizeof this ANYSIZE_ARRAY declaration
 * covers one WCHAR only and is not generally a complete output allocation. No normalization,
 * uniqueness, hardware-serial or security guarantee is established by the retrieval path.
 */
typedef struct _POWER_INTERNAL_SOC_IDENTIFIER_OPERATION_OUTPUT
{
    WCHAR Identifier[ANYSIZE_ARRAY];
} POWER_INTERNAL_SOC_IDENTIFIER_OPERATION_OUTPUT, *PPOWER_INTERNAL_SOC_IDENTIFIER_OPERATION_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_INPUT struct selects VM throttle-priority availability in the current processor performance domain.
 * \remarks Selector 93 takes at least this 8-byte header and at least 8 output bytes in
 * either caller mode. Version must be 0; others return STATUS_NOT_SUPPORTED. The support
 * result can be all zero on success when VM selection is unavailable. This is not a query
 * of every domain and does not enable priority selection.
 * \sa POWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_OUTPUT
 */
typedef struct _POWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version; // Must be 0; other versions return STATUS_NOT_SUPPORTED.
} POWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_INPUT, *PPOWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_INPUT;

// rev
/**
 * The POWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_OUTPUT struct reports current-domain VM priority support and its advertised priority count.
 * \remarks The 8-byte result is zero-initialized under PpmPerfPolicyLock. When VM perf
 * selection is enabled, VmThrottlePriorityCount at 4 comes from the current PRCB domain and
 * the Boolean at 0 is TRUE exactly when that count is nonzero. Bytes 1..3 are padding.
 * The count is not a Windows thread-priority range or VM count; provider contracts determine
 * which infoclasses are meaningful. CPU migration can change the queried domain.
 * \sa POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_INPUT
 */
typedef struct _POWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_OUTPUT
{
    BOOLEAN VmThrottleSupportedAndConfigured;
    UCHAR Reserved[3];
    ULONG VmThrottlePriorityCount;
} POWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_OUTPUT, *PPOWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_INPUT struct requests an opaque VM-priority configuration from the current domain provider.
 * \remarks Selector 94 requires at least 12 input bytes and at least 8 output bytes;
 * both modes are accepted and Version is ignored. Priority at 8 is passed unchanged to the
 * provider, with no common range check against the advertised count. Do not equate it with
 * a Windows scheduler priority. Missing VM selection support or provider returns
 * STATUS_NOT_SUPPORTED. The helper generates an encoding; it does not apply it to hardware.
 * \sa POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_OUTPUT
 */
typedef struct _POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG Priority;
} POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_INPUT, *PPOWER_INTERNAL_VMPERF_PRIORITY_CONFIG_INPUT;

// rev
/**
 * The POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_OUTPUT struct receives one provider-defined 64-bit VM throttle-priority configuration.
 * \remarks Selector 94 initializes the QWORD to zero, calls PerfPriorityHandler with
 * current-domain context, Priority and the output pointer, propagates negative provider
 * status and normalizes nonnegative provider status to STATUS_SUCCESS. Read Configuration
 * only on success. No universal bit layout, frequency unit, or returned handle exists here;
 * retrieval alone does not establish that any processor/VM configuration was applied.
 */
typedef struct _POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_OUTPUT
{
    ULONGLONG Configuration; // Provider-generated encoding; no universal bit layout.
} POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_OUTPUT, *PPOWER_INTERNAL_VMPERF_PRIORITY_CONFIG_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_DIRECTED_FX_ADD_TEST_DEVICE_INPUT struct adds a counted device-instance path to the directed-power test-device list.
 * \remarks Selector 53 accepts a user-mode administrator and requires directed-DRIPS
 * state bit 0 to be enabled; otherwise STATUS_NOT_SUPPORTED. This adds an AVL-list entry,
 * not a new hardware device or a caller-owned device handle. Version and the DWORD at +8
 * are not consumed here; Reserved/Flags is not an established flag mask. Initialize both
 * to zero for this unversioned path and use NULL/0 output; supplied output is ignored.
 *
 * DeviceNameLength at +12 is a WCHAR count, not bytes. Input must cover 16+2*count bytes;
 * checked arithmetic rejects overflows or a truncated name. A supplied trailing NUL is
 * removed from the stored comparison length; if absent, the helper appends one internally.
 * Lookup is case-insensitive. Duplicates return 0xC0000718; no additional reference is acquired.
 * Use the same normalized path when removing. An accepted count does not guarantee memory
 * allocation. The sizing helper returns 64-bit arithmetic; check before narrowing to ULONG.
 * \sa POWER_INTERNAL_DIRECTED_FX_REMOVE_TEST_DEVICE_INPUT
 * \sa POWER_INTERNAL_DIRECTED_FX_ADD_TEST_DEVICE_INPUT_SIZE
 */
typedef struct _POWER_INTERNAL_DIRECTED_FX_ADD_TEST_DEVICE_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    union
    {
        ULONG Reserved; // not consumed by this handler; initialize to zero
    };
    ULONG DeviceNameLength; // WCHAR count, not bytes
    WCHAR DeviceName[ANYSIZE_ARRAY]; // counted device-instance path
} POWER_INTERNAL_DIRECTED_FX_ADD_TEST_DEVICE_INPUT, *PPOWER_INTERNAL_DIRECTED_FX_ADD_TEST_DEVICE_INPUT;

// rev
/**
 * The POWER_INTERNAL_DIRECTED_FX_REMOVE_TEST_DEVICE_INPUT struct removes a counted device-instance path from the directed-power test-device list.
 * \remarks Selector 54 accepts a user-mode administrator and requires directed-DRIPS
 * state bit 0. DeviceNameLength at +8 is a WCHAR count; DeviceName begins at +12. Input
 * must cover 12+2*count bytes with no ULONG overflow. Unlike add, remove does not strip a
 * terminating NUL from the comparison length: exclude it to match the normalized stored path.
 * The lookup is case-insensitive; a missing entry returns STATUS_NOT_FOUND.
 *
 * This deletes a test-list entry and decrements its count, not a hardware device or caller
 * handle. Version is not checked; use zero and NULL/0 output. Supplied output is ignored.
 * Use the 64-bit size helper rather than sizeof plus a name length, since the trailing-array
 * structure includes compiler padding not present in the minimum counted wire length.
 * \sa POWER_INTERNAL_DIRECTED_FX_ADD_TEST_DEVICE_INPUT
 * \sa POWER_INTERNAL_DIRECTED_FX_REMOVE_TEST_DEVICE_INPUT_SIZE
 */
typedef struct _POWER_INTERNAL_DIRECTED_FX_REMOVE_TEST_DEVICE_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONG DeviceNameLength;
    WCHAR DeviceName[ANYSIZE_ARRAY];
} POWER_INTERNAL_DIRECTED_FX_REMOVE_TEST_DEVICE_INPUT, *PPOWER_INTERNAL_DIRECTED_FX_REMOVE_TEST_DEVICE_INPUT;

/**
 * The POWER_INTERNAL_DIRECTED_FX_ADD_TEST_DEVICE_INPUT_SIZE macro computes the counted add-test-device request's minimum wire size.
 * \param NameLength A ULONG WCHAR count, evaluated once.
 * \return A ULONGLONG equal to 16 + 2*NameLength bytes; no structure tail padding.
 * \remarks Check against ULONG_MAX and practical allocation limits before narrowing
 * or allocating. This sizes the input only, not the internal AVL entry or an output.
 * \sa POWER_INTERNAL_DIRECTED_FX_ADD_TEST_DEVICE_INPUT
 */
#define POWER_INTERNAL_DIRECTED_FX_ADD_TEST_DEVICE_INPUT_SIZE(NameLength) \
    ((ULONGLONG)FIELD_OFFSET(POWER_INTERNAL_DIRECTED_FX_ADD_TEST_DEVICE_INPUT, DeviceName) + (ULONGLONG)(NameLength) * sizeof(WCHAR))

/**
 * The POWER_INTERNAL_DIRECTED_FX_REMOVE_TEST_DEVICE_INPUT_SIZE macro computes the counted remove-test-device request's minimum wire size.
 * \param NameLength A ULONG WCHAR count, evaluated once; exclude the terminating NUL.
 * \return A ULONGLONG equal to 12 + 2*NameLength bytes; no structure tail padding.
 * \remarks Check against ULONG_MAX and practical allocation limits before narrowing
 * or allocating. The kernel uses the exact count in its case-insensitive lookup.
 * \sa POWER_INTERNAL_DIRECTED_FX_REMOVE_TEST_DEVICE_INPUT
 */
#define POWER_INTERNAL_DIRECTED_FX_REMOVE_TEST_DEVICE_INPUT_SIZE(NameLength) \
    ((ULONGLONG)FIELD_OFFSET(POWER_INTERNAL_DIRECTED_FX_REMOVE_TEST_DEVICE_INPUT, DeviceName) + (ULONGLONG)(NameLength) * sizeof(WCHAR))

// rev
/**
 * The POWER_INTERNAL_DIRECTED_FX_SET_MODE_INPUT struct updates the permissive-mode setting used by directed-power test infrastructure.
 * \remarks Selector 56 requires at least 12 bytes. PermissiveMode at offset 8 is normalized
 * from nonzero/zero and stored under PopDirectedDripsUmLock. User-mode administrator and
 * initialized directed-DRIPS state bit 0 are required; kernel mode is unsupported. Header.Version
 * is not checked. Use NULL/0 output; supplied output is ignored and no result is returned.
 * This modifies test policy, not a query of hardware capability or a general power-plan setting.
 * \sa POWER_INTERNAL_DIRECTED_DRIPS_QUERY_CAPABILITIES_OUTPUT
 */
typedef struct _POWER_INTERNAL_DIRECTED_FX_SET_MODE_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    BOOLEAN PermissiveMode;
} POWER_INTERNAL_DIRECTED_FX_SET_MODE_INPUT, *PPOWER_INTERNAL_DIRECTED_FX_SET_MODE_INPUT;

// rev
/**
 * The POWER_INTERNAL_DIRECTED_DRIPS_QUERY_CAPABILITIES_OUTPUT struct reports two normalized bits from the directed-DRIPS mitigation capability state.
 * \remarks Selector 64 returns2 bytes: MitigationBit0/1 correspond to masks1/2 of the
 * first mask returned by PopDirectedDripsQueryMitigationStatus. The full semantic names
 * of those mitigation bits are not established. The code does not test the
 * existence of those APIs, list membership, or permissive mode when filling the bytes.
 *
 * Use the result to record/compare capability state, not to infer test-device or mode-control
 * permissions. User administrator and directed-state bit 0 are required; Header.Version is
 * not checked. Provide 2 output bytes. The helper allocates a result even if direct output is
 * omitted; callers should request it rather than treating a status-only call as a bit query.
 * \sa POWER_INTERNAL_DIRECTED_DRIPS_QUERY_CAPABILITIES_INPUT
 */
typedef struct _POWER_INTERNAL_DIRECTED_DRIPS_QUERY_CAPABILITIES_OUTPUT
{
    BOOLEAN MitigationBit0;
    BOOLEAN MitigationBit1;
} POWER_INTERNAL_DIRECTED_DRIPS_QUERY_CAPABILITIES_OUTPUT, *PPOWER_INTERNAL_DIRECTED_DRIPS_QUERY_CAPABILITIES_OUTPUT;

/**
 * The POWER_INTERNAL_DIRECTED_DRIPS_QUERY_CAPABILITIES_INPUT type selects the two-byte directed-DRIPS mitigation-status capability snapshot.
 * \remarks Selector 64 accepts user-mode administrators while directed-DRIPS state
 * bit 0 is enabled; otherwise STATUS_NOT_SUPPORTED. Pass this 8-byte header and 2 output
 * bytes. Kernel mode is rejected; no input Version check is present.
 * \sa POWER_INTERNAL_DIRECTED_DRIPS_QUERY_CAPABILITIES_OUTPUT
 */
typedef POWER_INFORMATION_INTERNAL_HEADER POWER_INTERNAL_DIRECTED_DRIPS_QUERY_CAPABILITIES_INPUT, *PPOWER_INTERNAL_DIRECTED_DRIPS_QUERY_CAPABILITIES_INPUT;

// symbols, rev (AMD64)
/**
 * The POWER_INTERNAL_POWER_PLANE_NOTIFY callback type reports aggregate device power draw to the registering power-plane client.
 * \param[in] PmaxHandle Opaque client value supplied at request offset 8.
 * \param[in] DevicePowerMw Aggregate device power draw in milliwatts.
 * \remarks Stored by registration and called on power-draw changes. This is a kernel
 * callback, not a user-mode entry point. PDB _POP_POWER_PLANE corroborates its signature.
 * Registration does not establish an unregister/lifetime contract; keep the callback
 * and client state valid for the registered lifetime.
 */
typedef _Function_class_(POWER_INTERNAL_POWER_PLANE_NOTIFY)
VOID NTAPI POWER_INTERNAL_POWER_PLANE_NOTIFY(
    _In_opt_ PVOID PmaxHandle,
    _In_ ULONG DevicePowerMw
    );
typedef POWER_INTERNAL_POWER_PLANE_NOTIFY *PPOWER_INTERNAL_POWER_PLANE_NOTIFY;

/**
 * The POWER_INTERNAL_POWER_PLANE_PROFILE struct supplies the exclusive and peak power profile for a device/component state.
 * \remarks An 8-byte pair copied into _PO_POWER_PLANE_PROFILE. Both values are
 * milliwatts. Used by the offset-based plane registration data; not a measured sample.
 * \sa POWER_INTERNAL_POWER_PLANE_DEVICE
 */
typedef struct _POWER_INTERNAL_POWER_PLANE_PROFILE
{
    ULONG ExclusivePowerMw;
    ULONG PeakPowerMw;
} POWER_INTERNAL_POWER_PLANE_PROFILE, *PPOWER_INTERNAL_POWER_PLANE_PROFILE;

/**
 * The POWER_INTERNAL_POWER_PLANE_COMPONENT struct describes one component and the offset of its F-state power-profile array.
 * \remarks The serialized component stride is 24 bytes. FxPowerOffset is a byte
 * offset from the entire registration request, not this component or the plane prefix.
 * Each of FxCount profiles is 8 bytes. Keep all referenced data in InputBufferLength.
 * The component's GUID is copied unchanged; the caller owns the original input buffer.
 * \sa POWER_INTERNAL_REGISTER_POWER_PLANE_INPUT
 */
typedef struct _POWER_INTERNAL_POWER_PLANE_COMPONENT
{
    GUID ComponentGuid;
    ULONG FxCount;
    ULONG FxPowerOffset;
} POWER_INTERNAL_POWER_PLANE_COMPONENT, *PPOWER_INTERNAL_POWER_PLANE_COMPONENT;

/**
 * The POWER_INTERNAL_POWER_PLANE_DEVICE struct describes a power-plane device, four D-state profiles, and component records.
 * \remarks The serialized device stride is 44 bytes, with no native pointers.
 * DeviceIdOffset identifies a NUL-terminated WCHAR string; ComponentsOffset identifies
 * ComponentCount records of 24 bytes. Both offsets are bytes from the outer request.
 * DxPower contains four 8-byte profiles. Input strings and arrays must remain entirely
 * within the supplied buffer; do not rely solely on the kernel's partial bounds checks.
 * \sa POWER_INTERNAL_POWER_PLANE_COMPONENT
 */
typedef struct _POWER_INTERNAL_POWER_PLANE_DEVICE
{
    ULONG DeviceIdOffset;
    POWER_INTERNAL_POWER_PLANE_PROFILE DxPower[4];
    ULONG ComponentCount;
    ULONG ComponentsOffset;
} POWER_INTERNAL_POWER_PLANE_DEVICE, *PPOWER_INTERNAL_POWER_PLANE_DEVICE;

/**
 * The POWER_INTERNAL_POWER_PLANE_DATA struct starts the serialized power-plane description following the registration prefix.
 * \remarks On AMD64 this record begins at outer offset 24. PowerPlaneIdOffset is a
 * byte offset from the outer request to a NUL-terminated WCHAR string. DeviceCount must
 * be nonzero; Devices starts 8 bytes into this record with 44-byte stride. The helper
 * checks at least 52 bytes here, so a request needs at least 76 bytes before any extra
 * strings/component/profile arrays. sizeof with ANYSIZE_ARRAY covers only one device.
 * \sa POWER_INTERNAL_REGISTER_POWER_PLANE_INPUT
 */
typedef struct _POWER_INTERNAL_POWER_PLANE_DATA
{
    ULONG PowerPlaneIdOffset;
    ULONG DeviceCount;
    POWER_INTERNAL_POWER_PLANE_DEVICE Devices[ANYSIZE_ARRAY];
} POWER_INTERNAL_POWER_PLANE_DATA, *PPOWER_INTERNAL_POWER_PLANE_DATA;

#define POWER_INTERNAL_REGISTER_POWER_PLANE_VERSION 0

// rev
/**
 * The POWER_INTERNAL_REGISTER_POWER_PLANE_INPUT struct registers a kernel power plane, its device/component profiles, and power-draw callback.
 * \remarks Selector 57 is kernel-only and requires Header.Version=0 and NULL/0 output.
 * The AMD64 fixed prefix is24 bytes: opaque PmaxHandle at 8 and non-NULL callback at 16.
 * InputBufferLength supplies the complete buffer size. Data at 24 contains the offset-based
 * plane description; at least one 44-byte device and the 8-byte plane header are required
 * (minimum 76 total bytes), plus all referenced strings/components/F-state profiles.
 *
 * Offsets in the data are bytes from the entire outer request, never native pointers or
 * relative to Data. Strings must be NUL-terminated wholly within the input; the string helper
 * rejects lengths above 32767  WCHARs. Do not rely on its partial bounds checking as a buffer
 * validator. The header-only sizeof value is not a valid complete registration.
 * The pointer-bearing prefix is native-width; the x86 declaration is source-compatible
 * only and is not evidence of an x86 or WOW64 registration ABI.
 *
 * Registration is single-instance (existing plane: 0xC0000038); wrong Version yields
 * STATUS_REVISION_MISMATCH, NULL callback: STATUS_INVALID_PARAMETER. It stores callback/client
 * state, registers matching devices, and can publish power-draw notifications. No handle is
 * returned. Callback lifetime/unregister semantics are not established by this path. Typed
 * fields name ABI roles, not a public promise of a stable cross-version contract.
 * \sa POWER_INTERNAL_POWER_PLANE_DATA
 * \sa POWER_INTERNAL_POWER_PLANE_NOTIFY
 */
typedef struct _POWER_INTERNAL_REGISTER_POWER_PLANE_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    PVOID PmaxHandle; // opaque client value, passed to NotifyDevicePowerDraw
    PPOWER_INTERNAL_POWER_PLANE_NOTIFY NotifyDevicePowerDraw;
    UCHAR Data[ANYSIZE_ARRAY]; // AMD64 +24: POWER_INTERNAL_POWER_PLANE_DATA followed by referenced data
} POWER_INTERNAL_REGISTER_POWER_PLANE_INPUT, *PPOWER_INTERNAL_REGISTER_POWER_PLANE_INPUT;

#ifdef _WIN64
#endif

/**
 * The POWER_INTERNAL_REGISTER_POWER_PLANE_BASE_SIZE64 macro computes the fixed AMD64 registration extent through the device array.
 * \param DeviceCount A ULONG count, evaluated once; must be nonzero for registration.
 * \return A ULONGLONG equal to32+44*DeviceCount bytes, excluding all referenced tails.
 * \remarks This calculation is specific to the 64-bit callback/client prefix.
 * Append and validate strings/components/profiles, check ULONG limits and alignment,
 * and do not submit the value as a complete length unless it covers all referenced data.
 */
#define POWER_INTERNAL_REGISTER_POWER_PLANE_BASE_SIZE64(DeviceCount) (32ULL + (ULONGLONG)(DeviceCount) * 44ULL)

// rev
/**
 * The POWER_INTERNAL_DIRECTED_DRIPS_DEVICE_FLAGS_INPUT struct sets or clears selected directed-DRIPS flags on a kernel device node.
 * \remarks Selectors 58/59 require at least 24 bytes on AMD64. DeviceObject at 8 is a live
 * DEVICE_OBJECT pointer, not a user handle; Flags at 16 must be a subset of 0x3F. Set ORs
 * those bits into DeviceNode.DirectedDripsState.Flags, clear ANDs their complement. Zero
 * mask is accepted. Individual bit meanings remain unestablished; this is not ref-counting.
 *
 * Kernel only; user mode is denied. A NULL device or missing device node is invalid; the
 * DeviceObjectExtension hop is dereferenced without a NULL check. Caller must ensure a valid
 * object lifetime. Header.Version and Reserved are ignored. Use NULL/0 output: a supplied
 * output is ignored. No ownership/reference is transferred by this request.
 * \sa POWER_INTERNAL_DIRECTED_DRIPS_DEVICE_FLAGS_VALID_MASK
 */
typedef struct _POWER_INTERNAL_DIRECTED_DRIPS_DEVICE_FLAGS_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    PVOID DeviceObject;
    ULONG Flags;
    ULONG Reserved;
} POWER_INTERNAL_DIRECTED_DRIPS_DEVICE_FLAGS_INPUT, *PPOWER_INTERNAL_DIRECTED_DRIPS_DEVICE_FLAGS_INPUT;

#ifdef _WIN64
#endif

/**
 * The POWER_INTERNAL_DIRECTED_DRIPS_DEVICE_FLAGS_VALID_MASK constant is the mask accepted by the kernel device-node directed-DRIPS flag setters.
 * \remarks Only bits 0..5 (0x3F) are accepted; higher bits return STATUS_INVALID_PARAMETER.
 * Individual flag meanings are not defined here. This is not a capability mask.
 */
#define POWER_INTERNAL_DIRECTED_DRIPS_DEVICE_FLAGS_VALID_MASK 0x0000003FUL

// rev
/**
 * The POWER_INTERNAL_RETRIEVE_HIBERFILE_RESUME_CONTEXT_OUTPUT struct preserves the variable-length resume-context blob read from the existing hibernation file.
 * \remarks Selector 60 requires user-mode administrator context; input is an 8-byte header,
 * output must hold the entire returned DataSize. DataSize at+4 is the TOTAL byte count,
 * including the prefix, not sizeof(Data) or a tail allocation size. The first three DWORDs
 * are Version(+0),DataSize(+4),EntryCount(+8). The reader requires Version=1 and
 * checks 48*EntryCount<=DataSize with ULONG overflow rejection, plus DataSize within the
 * page-backed context extent. These checks alone do not establish an entry array starting
 * at offset 12 or a safe iterator; trailing Data remains opaque.
 *
 * The source context starts at file page 2 and its page count comes from the file header.
 * No required-size result is returned separately on a short caller buffer, so this is not
 * a conventional size-probe API. Use known/captured capacity and validate at least 12 readable
 * bytes before interpreting the prefix; do not trust the kernel's weak metadata checks as
 * an application parser. Existing file/hibernation state must be available; the request does
 * not create a resume context or return an owned kernel pointer to user mode.
 * \sa POWER_INTERNAL_HIBERFILE_RESUME_CONTEXT_HEADER
 * \sa POWER_INTERNAL_RETRIEVE_HIBERFILE_RESUME_CONTEXT_INPUT
 */
typedef struct _POWER_INTERNAL_RETRIEVE_HIBERFILE_RESUME_CONTEXT_OUTPUT
{
    ULONG Version;
    ULONG DataSize;
    ULONG EntryCount;
    UCHAR Data[ANYSIZE_ARRAY];
} POWER_INTERNAL_RETRIEVE_HIBERFILE_RESUME_CONTEXT_OUTPUT, *PPOWER_INTERNAL_RETRIEVE_HIBERFILE_RESUME_CONTEXT_OUTPUT;

/**
 * The POWER_INTERNAL_HIBERFILE_RESUME_CONTEXT_HEADER struct is the fixed 12-byte prefix of a variable-length hibernation resume-context result.
 * \remarks This prefix provides the bounds for parsing the opaque trailing bytes.
 * Version must be 1 for the format. TotalSize counts the complete returned
 * buffer, not just the tail; EntryCount participates in a 48*count validation but the
 * entry schema and placement are not established here. Never cast without first checking
 * available length >=12 and TotalSize within the supplied/captured output capacity.
 * \sa POWER_INTERNAL_RETRIEVE_HIBERFILE_RESUME_CONTEXT_OUTPUT
 */
typedef struct _POWER_INTERNAL_HIBERFILE_RESUME_CONTEXT_HEADER
{
    ULONG Version;
    ULONG TotalSize;
    ULONG EntryCount;
} POWER_INTERNAL_HIBERFILE_RESUME_CONTEXT_HEADER, *PPOWER_INTERNAL_HIBERFILE_RESUME_CONTEXT_HEADER;

/**
 * The POWER_INTERNAL_RETRIEVE_HIBERFILE_RESUME_CONTEXT_INPUT type selects retrieval of the existing hibernation-file resume-context blob.
 * \remarks Selector 60 takes at least this 8-byte header and a variable-size output.
 * User-mode administrator required; kernel mode returns STATUS_NOT_SUPPORTED.
 * The operation reads existing file metadata; it does not initiate hibernation/resume.
 * \sa POWER_INTERNAL_RETRIEVE_HIBERFILE_RESUME_CONTEXT_OUTPUT
 */
typedef POWER_INFORMATION_INTERNAL_HEADER POWER_INTERNAL_RETRIEVE_HIBERFILE_RESUME_CONTEXT_INPUT, *PPOWER_INTERNAL_RETRIEVE_HIBERFILE_RESUME_CONTEXT_INPUT;

#define POWER_INTERNAL_HIBERFILE_RESUME_CONTEXT_VERSION 1

// rev
/**
 * The POWER_INTERNAL_QUERY_SLEEPSTUDY_HELPER_ROUTINE_BLOCK_OUTPUT struct receives a borrowed kernel SleepStudy helper-routine-block address.
 * \remarks Selector 63 returns8 bytes on AMD64 when SleepstudyHelperAccountingEnabled is
 * nonzero, otherwise STATUS_NOT_SUPPORTED. Kernel mode only. HelperRoutineBlock points to
 * static SshpRoutineBlock; it is not a copied routine table or an object-manager handle.
 * Do not free it or pass it to NtClose. Routine-table layout and callback signatures are
 * not established here; an opaque pointer must not be used as an arbitrary callable function.
 * This query is for kernel library integration/diagnostics, not a user-mode callable interface.
 * \sa POWER_INTERNAL_QUERY_SLEEPSTUDY_HELPER_ROUTINE_BLOCK_INPUT
 */
typedef struct _POWER_INTERNAL_QUERY_SLEEPSTUDY_HELPER_ROUTINE_BLOCK_OUTPUT
{
    PVOID HelperRoutineBlock;
} POWER_INTERNAL_QUERY_SLEEPSTUDY_HELPER_ROUTINE_BLOCK_OUTPUT, *PPOWER_INTERNAL_QUERY_SLEEPSTUDY_HELPER_ROUTINE_BLOCK_OUTPUT;

#ifdef _WIN64
#endif

/**
 * The POWER_INTERNAL_QUERY_SLEEPSTUDY_HELPER_ROUTINE_BLOCK_INPUT type selects the kernel SleepStudy helper-library interface pointer query.
 * \remarks Selector 63 requires this 8-byte input header and 8 output bytes on AMD64.
 * The library must be enabled. No operation-specific Version validation is present.
 * \sa POWER_INTERNAL_QUERY_SLEEPSTUDY_HELPER_ROUTINE_BLOCK_OUTPUT
 */
typedef POWER_INFORMATION_INTERNAL_HEADER POWER_INTERNAL_QUERY_SLEEPSTUDY_HELPER_ROUTINE_BLOCK_INPUT, *PPOWER_INTERNAL_QUERY_SLEEPSTUDY_HELPER_ROUTINE_BLOCK_INPUT;

/**
 * The POWER_INTERNAL_LAST_BOOT_SUCCEEDED_INPUT type selects the previous-boot success flag captured from the loader.
 * \remarks Selector 62 needs at least this 8-byte header and 1 output byte. Version
 * is not checked. This query does not verify disk contents or clear diagnostic state.
 * \sa POWER_INTERNAL_LAST_BOOT_SUCCEEDED_OUTPUT
 */
typedef POWER_INFORMATION_INTERNAL_HEADER POWER_INTERNAL_LAST_BOOT_SUCCEEDED_INPUT, *PPOWER_INTERNAL_LAST_BOOT_SUCCEEDED_INPUT;

/**
 * The POWER_INTERNAL_LAST_BOOT_SUCCEEDED_OUTPUT struct reports the loader's captured LastBootSucceeded Boolean.
 * \remarks Selector 62 returns byte zero of PopDirtyTransitionDiagInfo, initialized
 * from _LOADER_PARAMETER_EXTENSION.LastBootSucceeded (bit 0 at byte 132). Its global
 * symbol name does not invert the result. Read only after successful query; this is
 * not a current boot-progress guarantee or a newly performed integrity test.
 * \sa POWER_INTERNAL_LAST_BOOT_SUCCEEDED_INPUT
 */
typedef struct _POWER_INTERNAL_LAST_BOOT_SUCCEEDED_OUTPUT
{
    BOOLEAN LastBootSucceeded;
} POWER_INTERNAL_LAST_BOOT_SUCCEEDED_OUTPUT, *PPOWER_INTERNAL_LAST_BOOT_SUCCEEDED_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_CLEAR_CONSTRAINTS_INPUT struct clears eligible PoFx device DRIPS constraints and resets related accounting.
 * \remarks Kernel-only infoclass 65 requires exactly 16 input bytes on AMD64 and non-NULL
 * FxDevice at 8. This is an internal POP_FX_DEVICE pointer, NOT the DEVICE_OBJECT expected
 * by infoclasses 58/59.
 * The helper references/dereferences that PoFx device internally, clears an eligible PEP
 * DRIPS veto mask, and can reset device/component accounting and publish a constraints-updated
 * WNF notification. No caller-owned handle is returned or transferred.
 *
 * The deep-sleep platform state and device/plugin state must support the operation; a
 * non-NULL pointer alone does not ensure success. Invalid pointers are not safely validated
 * as handles. Header.Version is not checked. Use NULL/0 output; supplied output is ignored.
 * This changes constraint state, not a query or generic reset of all system power constraints.
 * \sa POWER_INTERNAL_DIRECTED_DRIPS_DEVICE_FLAGS_INPUT
 */
typedef struct _POWER_INTERNAL_CLEAR_CONSTRAINTS_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    union
    {
        PVOID FxDevice; // internal POP_FX_DEVICE pointer; NOT a DEVICE_OBJECT or object-manager handle
    };
} POWER_INTERNAL_CLEAR_CONSTRAINTS_INPUT, *PPOWER_INTERNAL_CLEAR_CONSTRAINTS_INPUT;

#ifdef _WIN64
#endif

#define PPM_VM_PERF_CONFIG_VERSION_MAX 2UL
#define PPM_VM_PERF_CONFIG_UNSPECIFIED 0xFFFFFFFFUL
#define PPM_VM_PERF_CONFIG_EPP_MAX 255UL

/**
 * The POWER_INTERNAL_VM_PERF_CONTROL_CONFIG_INPUT_V1 structure supplies a frequency request for the version 0 or
 * version 1 VM performance-configuration path.
 * \remarks Set Header.InternalType to PowerInternalGetVmPerfControlConfig, Header.Version to 0 or 1, and
 * FrequencyMhz to the requested frequency. Supply at least 12 input bytes and an 8-byte configuration output. Use
 * this form to investigate policy-clamped frequency configuration generation without the additional CPPC controls
 * of version 2; a zero frequency requests zero configuration.
 * \remarks Selector 70 accepts both caller modes with input >=12 bytes for
 * Version 0/1 and output >=8 bytes. Unsupported VM selection returns STATUS_NOT_SUPPORTED,
 * even for a zero frequency. The current PRCB domain supplies policy/provider data under
 * PpmPerfPolicyLock. Zero frequency returns configuration zero only when support is enabled.
 * The provider constructs an encoding; this path does not itself apply a hardware frequency.
 */
typedef struct _POWER_INTERNAL_VM_PERF_CONTROL_CONFIG_INPUT_V1
{
    POWER_INFORMATION_INTERNAL_HEADER Header; // Version 0 or 1; input length >= 0x0C.
    ULONG FrequencyMhz; // Zero requests zero configuration.
} POWER_INTERNAL_VM_PERF_CONTROL_CONFIG_INPUT_V1, *PPOWER_INTERNAL_VM_PERF_CONTROL_CONFIG_INPUT_V1;

// rev
/**
 * The POWER_INTERNAL_VM_PERF_CONTROL_CONFIG_INPUT structure supplies the version 2 frequency bounds, desired
 * frequency, activity window, and energy-performance preference for VM performance-configuration generation.
 * \remarks This request returns how frequency bounds and autonomous-performance guidance are
 * translated by the current domain provider. Check support first, initialize unspecified values according to the
 * documented sentinel rules, and do not treat IgnorePolicyMaximum as permission to exceed hardware constraints.
 * The query returns an encoding rather than a live frequency or proof that hardware state changed.
 *
 * Version 2 request used by PpmPerfGetVmCppcConfig. Versions above 2 are unsupported.
 * EPP is scaled to min(100, Epp * 100 / 255) before the provider call.
 * The activity-window unit/encoding is provider-dependent.
 * Versions 0/1 use POWER_INTERNAL_VM_PERF_CONTROL_CONFIG_INPUT_V1.
 * \remarks Selector 70 requires input >=32 bytes for Version 2 and output >=8 bytes;
 * Version >2 returns STATUS_NOT_SUPPORTED. Both caller modes are accepted. Minimum/maximum
 * 0 are normalized to 0xFFFFFFFF. If both are unspecified and desired frequency, activity
 * window and EPP all equal 0xFFFFFFFF, the supported helper returns configuration zero.
 *
 * For ordinary EPP inputs 0..255, scaling is floor(EPP*100/255), capped at 100. The machine
 * code multiplies in 32 bits: do not rely on saturation for arbitrary out-of-range ULONG
 * inputs that overflow that product. 0xFFFFFFFF selects policy EPP instead. IgnorePolicyMaximum
 * skips a policy cap, not the domain hardware bound. No generic activity-window unit or
 * provider encoding is established. This is a configuration-generation call, not a setter.
 */
typedef struct _POWER_INTERNAL_VM_PERF_CONTROL_CONFIG_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header; // Version 2; input length >= 0x20.
    ULONG MaximumFrequencyMhz; // 0 and 0xFFFFFFFF select policy defaults.
    ULONG MinimumFrequencyMhz; // 0 and 0xFFFFFFFF select policy defaults.
    ULONG DesiredFrequencyMhz; // 0xFFFFFFFF is unspecified.
    ULONG AutonomousActivityWindow; // provider encoding, 0xFFFFFFFF uses policy.
    ULONG EnergyPerformancePreference; // normally 0..255; 0xFFFFFFFF uses policy.
    BOOLEAN IgnorePolicyMaximum; // does not bypass hardware constraints.
    UCHAR Reserved[3];
} POWER_INTERNAL_VM_PERF_CONTROL_CONFIG_INPUT, *PPOWER_INTERNAL_VM_PERF_CONTROL_CONFIG_INPUT;

// rev
/**
 * The POWER_INTERNAL_VM_PERF_CONTROL_CONFIG_OUTPUT structure contains the opaque configuration generated from a VM
 * frequency or CPPC-style request.
 * \remarks Use Configuration to compare policy-adjusted requests during VM performance diagnostics or with a
 * consumer that knows the processor-provider encoding. The value is not a pair of independently meaningful
 * frequencies, and its bits cannot be decoded portably from the kernel interface alone.
 *
 * The 8-byte output from PpmPerfGetVmPerfConfig or PpmPerfGetVmCppcConfig.
 * This is one provider-defined configuration, not two independently identified ULONGs.
 * \remarks Selector 70 copies exactly 8 bytes on success; read Configuration only after
 * success. Provider return value is not propagated by the helpers; they report
 * support/dispatch status. The opaque encoding may be zero for the documented sentinel paths.
 * Do not split this QWORD into guessed minimum/maximum MHz fields or assume it owns a handle.
 */
typedef struct _POWER_INTERNAL_VM_PERF_CONTROL_CONFIG_OUTPUT
{
    ULONGLONG Configuration; // Provider-generated encoding; no universal bit layout.
} POWER_INTERNAL_VM_PERF_CONTROL_CONFIG_OUTPUT, *PPOWER_INTERNAL_VM_PERF_CONTROL_CONFIG_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_CONCURRENCY_STATS_OUTPUT structure provides the legacy generic view of accumulated node or
 * class concurrency histograms.
 * \remarks Use the infoclass-specific structures for new code so that processor counts and QPC-tick arrays are not
 * mistaken for idle-interval time buckets. These histograms can show how accounted time is distributed across
 * active-processor counts. Convert QPC ticks with the appropriate counter frequency before comparing them with
 * 100-ns-based statistics.
 *
 * Legacy generic view of node/class concurrency output.
 * Node output is 8 + 8 * (Count[0] + 1) bytes. Class output concatenates one
 * Count[n] + 1 QWORD histogram for each present class, at most two classes.
 * Missing classes contribute no payload; a zero count alone cannot prove presence.
 * Class allocation is 8 + 8 * (nodeProcessorCount + max(nodeClassCount, 2)).
 * Counters are copied without conversion or a forced new sample.
 * \remarks Selectors 76/77 require the shared exact 12-byte node input. Do not size
 * class output solely from the two returned counts: allocation can contain zero trailing
 * capacity and a missing class consumes no histogram. Prefer the dedicated node/class types.
 * No interval-bin conversion, refresh, counter reset or topology list is supplied.
 */
typedef struct _POWER_INTERNAL_CONCURRENCY_STATS_OUTPUT
{
    ULONG Count[2]; // Processor counts (not bucket counts); Count[1] reserved for node query.
    ULONGLONG Data[ANYSIZE_ARRAY]; // Raw QPC ticks, not 100-ns units.
} POWER_INTERNAL_CONCURRENCY_STATS_OUTPUT, *PPOWER_INTERNAL_CONCURRENCY_STATS_OUTPUT;

/**
 * The POWER_INTERNAL_CPU_NODE_CONCURRENCY_STATS_OUTPUT structure contains a selected parking-node histogram of
 * accumulated time at each active tracked-processor count.
 * \remarks TimeQpcTicks[k] records time with k active tracked processors, including k=0 for no
 * active processors. The response has an 8-byte prefix and ProcessorCount+1 QWORD counters, for 8 + 8 *
 * (ProcessorCount + 1) bytes. Values are raw QPC ticks copied without forcing a fresh sample; matched deltas can
 * help diagnose sustained versus bursty concurrency without implying which application caused it.
 * \remarks Selector 76 returns 8+8*(ProcessorCount+1) bytes, with the unused DWORD
 * at +4 zero from allocation. Bin 0 is the idle active-count state, not an idle-duration
 * range. POWER_INTERNAL_CPU_NODE_CONCURRENCY_STATS_SIZE uses 64-bit arithmetic for sizing;
 * validate total capacity and ULONG limits before allocating or walking the array.
 */
typedef struct _POWER_INTERNAL_CPU_NODE_CONCURRENCY_STATS_OUTPUT
{
    ULONG ProcessorCount;
    ULONG Reserved;
    _Field_size_((SIZE_T)ProcessorCount + 1)
    ULONGLONG TimeQpcTicks[ANYSIZE_ARRAY]; // Index k: duration with k active tracked processors.
} POWER_INTERNAL_CPU_NODE_CONCURRENCY_STATS_OUTPUT, *PPOWER_INTERNAL_CPU_NODE_CONCURRENCY_STATS_OUTPUT;

/**
 * The POWER_INTERNAL_CPU_NODE_CONCURRENCY_STATS_SIZE macro computes the variable node-concurrency result extent without ULONG overflow.
 * \param ProcessorCount A ULONG processor count, evaluated once.
 * \return ULONGLONG byte count 8+8*(ProcessorCount+1), including the idle bin.
 * \remarks Validate against available bytes and ULONG_MAX before allocation/use. This
 * applies only to infoclass 76; it does not parse class presence for infoclass 77.
 */
#define POWER_INTERNAL_CPU_NODE_CONCURRENCY_STATS_SIZE(ProcessorCount) \
    (8ULL + 8ULL * ((ULONGLONG)(ProcessorCount) + 1ULL))

/**
 * The POWER_INTERNAL_CLASS_CONCURRENCY_STATS_OUTPUT structure contains per-class processor counts followed by
 * packed concurrency histograms for present class banks.
 * \remarks Use this response to compare active-processor-count distributions across classes in the selected
 * parking node. Each present class contributes ProcessorCount[n]+1 raw-QPC-tick counters, with at most two classes
 * copied. Missing classes contribute no array, so topology/class-presence information is required for ambiguous
 * zero-count cases; do not assume two fixed-size arrays or derive the entire allocation size solely from the two
 * counts.
 * \remarks Selector 77's allocation is 8+8*(nodeProcessorCount+max(nodeClassCount,2))
 * bytes, while only at most two present class histograms are copied in increasing slot order.
 * Missing class storage consumes no array; counts alone cannot disambiguate a missing class
 * from a present zero-processor class. No safe automatic class iterator is defined here.
 * Missing node/accounting or no usable class histogram returns STATUS_NOT_SUPPORTED.
 */
typedef struct _POWER_INTERNAL_CLASS_CONCURRENCY_STATS_OUTPUT
{
    ULONG ProcessorCount[2];
    ULONGLONG TimeQpcTicks[ANYSIZE_ARRAY]; // Concatenated PRESENT-class histograms; see generic view above.
} POWER_INTERNAL_CLASS_CONCURRENCY_STATS_OUTPUT, *PPOWER_INTERNAL_CLASS_CONCURRENCY_STATS_OUTPUT;

// Processor measurement capability and request bits. These are provider masks,
// not the measurement IDs returned in the value entries.
#define PROCESSOR_MEASUREMENT_PACKAGE_IDLE 0x00000001UL
#define PROCESSOR_MEASUREMENT_PACKAGE_THERMAL 0x00000002UL
#define PROCESSOR_MEASUREMENT_PACKAGE_MASK (PROCESSOR_MEASUREMENT_PACKAGE_IDLE | PROCESSOR_MEASUREMENT_PACKAGE_THERMAL)

typedef enum _PROCESSOR_MEASUREMENT_ID
{
    ProcessorMeasurementPackageC2Residency = 0x00200200,
    ProcessorMeasurementPackageC3Residency = 0x00200300,
    ProcessorMeasurementPackageC4Residency = 0x00200400,
    ProcessorMeasurementPackageC6Residency = 0x00200600,
    ProcessorMeasurementPackageC7Residency = 0x00200700,
    ProcessorMeasurementPackageC8Residency = 0x00200800,
    ProcessorMeasurementPackageC9Residency = 0x00200900,
    ProcessorMeasurementPackageC10Residency = 0x00200A00,
    ProcessorMeasurementThermalStatus = 0x00010000,
    ProcessorMeasurementProchot = 0x00010001,
    ProcessorMeasurementCriticalTemperature = 0x00010002,
    ProcessorMeasurementThermalThreshold1 = 0x00010003,
    ProcessorMeasurementThermalThreshold2 = 0x00010004,
    ProcessorMeasurementPowerLimitation = 0x00010005,
    ProcessorMeasurementPackageDigitalReadout = 0x02110006
} PROCESSOR_MEASUREMENT_ID;

// rev
/**
 * The POWER_INTERNAL_QUERY_MEASUREMENT_CAPABILITIES structure selects the processor-measurement
 * provider-capability query.
 * \remarks Use the 8-byte header with PowerInternalQueryProcMeasurementCapabilities before constructing a
 * measurement mask. The header is required through NtPowerInformation even though there is no additional payload.
 * The response reports the first domain with an available capability callback, not a union of every domain
 * capability.
 *
 * The POWER_INTERNAL_QUERY_MEASUREMENT_CAPABILITIES structure contains the 8-byte internal request header.
 * \remarks Selector 78 accepts both caller modes, at least 8 input bytes and at least
 * 4 output bytes. Version is ignored. The dispatcher uses the first domain with a non-NULL
 * capability callback, not a union of every provider. No callback yields successful mask 0.
 * A capability mask is not a record count or a list of measurement IDs.
 */
typedef struct _POWER_INTERNAL_QUERY_MEASUREMENT_CAPABILITIES
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
} POWER_INTERNAL_QUERY_MEASUREMENT_CAPABILITIES, *PPOWER_INTERNAL_QUERY_MEASUREMENT_CAPABILITIES;

// rev
/**
 * The POWER_INTERNAL_QUERY_MEASUREMENT_CAPABILITIES_OUTPUT structure contains the provider-selection mask returned
 * by the processor-measurement capability callback.
 * \remarks Use Capabilities to select available measurement providers before querying values. Known Intel provider
 * bits select package idle residency (0x1) and package thermal measurements (0x2); these bits are not the
 * measurement IDs carried by individual records. A zero mask can mean no callback is available, and support alone
 * does not guarantee a later values query will succeed.
 *
 * The POWER_INTERNAL_QUERY_MEASUREMENT_CAPABILITIES_OUTPUT structure contains the available provider mask.
 * \remarks The infoclass 78 wire result is exactly one DWORD. Capability-bit meaning
 * belongs to the selected provider; the kernel dispatcher does not impose a universal mask.
 * Existing Intel package masks are provider-specific, not portable to every processor driver.
 */
typedef struct _POWER_INTERNAL_QUERY_MEASUREMENT_CAPABILITIES_OUTPUT
{
    ULONG Capabilities;
} POWER_INTERNAL_QUERY_MEASUREMENT_CAPABILITIES_OUTPUT, *PPOWER_INTERNAL_QUERY_MEASUREMENT_CAPABILITIES_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_QUERY_MEASUREMENT_VALUES structure requests records from the processor-measurement providers
 * selected by MeasurementMask.
 * \remarks Use a nonzero supported provider mask with PowerInternalQueryProcMeasurementValues and exactly 12 input
 * bytes. First request a 4-byte count-only result, then use the count to size the full response. This request
 * selects measurement categories, not a processor index; mask 0x3 requests both known Intel categories when
 * available.
 *
 * The POWER_INTERNAL_QUERY_MEASUREMENT_VALUES structure requests the nonzero provider mask in a 12-byte input.
 * \remarks Selector 79 requires exactly 12 input bytes in either caller mode; Version
 * is ignored. MeasurementMask 0 is invalid. The first domain with a measurement-values callback
 * is used; no provider returns STATUS_NOT_IMPLEMENTED. A 4-byte output requests count only;
 * otherwise the full 8+24*EntryCount bytes are required. Use infoclass 78's supported provider
 * mask. The kernel dispatcher does not enforce the Intel-only PACKAGE_MASK as a universal mask.
 */
typedef struct _POWER_INTERNAL_QUERY_MEASUREMENT_VALUES
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONG MeasurementMask; // PROCESSOR_MEASUREMENT_PACKAGE_MASK
} POWER_INTERNAL_QUERY_MEASUREMENT_VALUES, *PPOWER_INTERNAL_QUERY_MEASUREMENT_VALUES;

// rev
/**
 * The POWER_INTERNAL_QUERY_MEASUREMENT_VALUES_ENTRY structure contains a measurement identifier, accumulated
 * elapsed refresh time, and an unsigned 64-bit provider-defined value.
 * \remarks Use MeasurementId to choose the correct interpretation before displaying Value. Matched idle-record
 * differences describe residency-time changes; matched Boolean-thermal differences describe sampled assertion
 * time, not event counts. For the digital readout, Delta(Value) / Delta(ElapsedTime100ns) gives sampled average
 * degrees below the package TCC reference, not absolute temperature. Comparisons require the same provider, record
 * identity, and uninterrupted counter lifetime; reject zero elapsed differences and unexplained regressions.
 *
 * The POWER_INTERNAL_QUERY_MEASUREMENT_VALUES_ENTRY structure contains one 24-byte provider value.
 * Idle values are converted residency counters in 100-ns units. Boolean thermal
 * values are accumulated sampled assertion times in 100-ns units. The package
 * digital readout is accumulated thermal headroom in Celsius difference times
 * 100-ns units, not an instantaneous or absolute temperature.
 * \remarks Selector 79 reserves a 24-byte stride for provider records. The common
 * kernel route does not write or normalize the record fields; value units and supported IDs
 * are provider contracts. The above Intel interpretations are not evidence that every provider
 * has those semantics. Ignore padding and do not infer an absolute temperature or timestamp.
 */
typedef struct _POWER_INTERNAL_QUERY_MEASUREMENT_VALUES_ENTRY
{
    ULONG MeasurementId; // PROCESSOR_MEASUREMENT_ID
    ULONG Reserved; // padding; provider writers do not initialize this field
    ULONGLONG ElapsedTime100ns; // accumulated elapsed refresh time, not an absolute timestamp
    ULONGLONG Value; // integer quantity with MeasurementId-specific units
} POWER_INTERNAL_QUERY_MEASUREMENT_VALUES_ENTRY, *PPOWER_INTERNAL_QUERY_MEASUREMENT_VALUES_ENTRY;

// rev
/**
 * The POWER_INTERNAL_QUERY_MEASUREMENT_VALUES_COUNT structure contains the number of records reported by the
 * count-only processor-measurement query.
 * \remarks Use an output length of 4 bytes to obtain EntryCount, then compute the full response length as 8 + 24 *
 * EntryCount with overflow checks. This is a sizing response, not a measurement record or the byte length itself;
 * handle a subsequent failure if availability changes between queries.
 *
 * The POWER_INTERNAL_QUERY_MEASUREMENT_VALUES_COUNT structure is the 4-byte count-only response.
 * \remarks Exactly 4 output bytes selects this form; NULL/0 is not the count probe.
 * The mask must be nonzero and provider status must succeed before using EntryCount.
 * A later full request repeats the provider count query; count-only and full calls are not
 * an atomic snapshot. Check the 64-bit size helper against ULONG_MAX before allocating.
 */
typedef struct _POWER_INTERNAL_QUERY_MEASUREMENT_VALUES_COUNT
{
    ULONG EntryCount;
} POWER_INTERNAL_QUERY_MEASUREMENT_VALUES_COUNT, *PPOWER_INTERNAL_QUERY_MEASUREMENT_VALUES_COUNT;

// rev
/**
 * The POWER_INTERNAL_QUERY_MEASUREMENT_VALUES_OUTPUT structure contains the full variable-length set of records
 * returned by the selected measurement providers.
 * \remarks Use EntryCount to bound traversal of Entries and decode each record by MeasurementId rather than
 * assuming a fixed order or that all known IDs are present. Allocate using the 8-byte prefix plus 24 bytes per
 * entry, not sizeof(structure) plus every entry. Preserve provider and topology context when comparing snapshots,
 * and ignore the reserved padding.
 *
 * The POWER_INTERNAL_QUERY_MEASUREMENT_VALUES_OUTPUT structure contains 8 + 24 * EntryCount bytes.
 * \remarks The dispatcher first gets a count, allocates 8+24*count zeroed bytes,
 * writes EntryCount, then invokes the provider again with the entry region. The header count
 * is the first count; a provider-updated count from the second invocation is not copied back.
 * Read records only on success and within captured capacity; availability may change.
 * The common route proves the stride, not provider-specific meanings or a fixed record order.
 */
typedef struct _POWER_INTERNAL_QUERY_MEASUREMENT_VALUES_OUTPUT
{
    ULONG EntryCount;
    ULONG Reserved;
    _Field_size_(EntryCount) POWER_INTERNAL_QUERY_MEASUREMENT_VALUES_ENTRY Entries[ANYSIZE_ARRAY];
} POWER_INTERNAL_QUERY_MEASUREMENT_VALUES_OUTPUT, *PPOWER_INTERNAL_QUERY_MEASUREMENT_VALUES_OUTPUT;

/**
 * The POWER_INTERNAL_QUERY_MEASUREMENT_VALUES_OUTPUT_SIZE macro computes the full processor-measurement result size from a count-only response.
 * \param EntryCount A ULONG record count, evaluated once.
 * \return ULONGLONG byte count 8+24*EntryCount; zero entries still require the 8-byte prefix.
 * \remarks Check ULONG_MAX and allocation limits. The helper is not a promise that a
 * subsequent provider query has the same count, nor a validator of measurement semantics.
 */
#define POWER_INTERNAL_QUERY_MEASUREMENT_VALUES_OUTPUT_SIZE(EntryCount) \
    (8ULL + 24ULL * (ULONGLONG)(EntryCount))

// rev
/**
 * The POWER_INTERNAL_PREPARE_FOR_SYSTEM_INITIATED_REBOOT_INPUT structure contains the input parameters for the prepare for system initiated reboot operation.
 * \remarks Selector 80 requires exactly this 8-byte header, NULL/0 output, and user-mode
 * SeShutdownPrivilege (otherwise STATUS_PRIVILEGE_NOT_HELD). Kernel mode is unsupported.
 * Version is ignored. The route captures the adaptive reboot target in a global and sets a
 * prepared flag; it does NOT initiate a reboot or return that state. Use only as part of an
 * intended reboot lifecycle, not as a status query. No handle or buffer ownership is transferred.
 */
typedef struct _POWER_INTERNAL_PREPARE_FOR_SYSTEM_INITIATED_REBOOT_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
} POWER_INTERNAL_PREPARE_FOR_SYSTEM_INITIATED_REBOOT_INPUT, *PPOWER_INTERNAL_PREPARE_FOR_SYSTEM_INITIATED_REBOOT_INPUT;

// rev
/**
 * The POWER_INTERNAL_OVERRIDE_SYSTEM_INITIATED_REBOOT_STATE_INPUT struct overrides the adaptive target state captured for a future system-initiated reboot.
 * \remarks Selector 83 accepts user mode only with testsigning enabled; kernel mode returns
 * STATUS_NOT_SUPPORTED. No additional administrator, LocalSystem or shutdown-privilege check is present in
 * infoclass path. Input must be exactly 16 bytes, output NULL/0; Header.Version is ignored. This sets an
 * internal override; it does not execute a reboot.
 *
 * TargetState at +12 accepts codes 0..6. TargetStateFlag at +8 is normalized into internal bit3. State 0 with
 * a nonzero flag is invalid; state 0 with flag FALSE clears the low state and flag bits. The precise named
 * meaning of bit3 and all target codes is not established; do not substitute SYSTEM_POWER_STATE for the
 * target-state code.
 * \sa POWER_INTERNAL_PREPARE_FOR_SYSTEM_INITIATED_REBOOT_INPUT
 */
typedef struct _POWER_INTERNAL_OVERRIDE_SYSTEM_INITIATED_REBOOT_STATE_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    struct
    {
        BOOLEAN TargetStateFlag; // normalized into bit3 of internal target state
        UCHAR Reserved0[3]; // ignored; initialize to zero
    };
    ULONG TargetState; // internal boot-state code 0..6, NOT SYSTEM_POWER_STATE
} POWER_INTERNAL_OVERRIDE_SYSTEM_INITIATED_REBOOT_STATE_INPUT, *PPOWER_INTERNAL_OVERRIDE_SYSTEM_INITIATED_REBOOT_STATE_INPUT;

/**
 * The POWER_INTERNAL_REBOOT_TARGET_STATE_MAX constant is the largest accepted internal reboot target-state code for infoclass 83.
 * \remarks Valid codes are 0..6, not SYSTEM_POWER_STATE. Code 0 requires flag FALSE.
 * This constant validates the numeric range only; it does not define code semantics.
 */
#define POWER_INTERNAL_REBOOT_TARGET_STATE_MAX 6UL

// rev
/**
 * The POWER_INTERNAL_UNREGISTER_SHUTDOWN_NOTIFICATION_INPUT struct identifies the legacy infoclass-87 shutdown-listener removal prefix.
 * \remarks Kernel-only, NULL/0 output, Version ignored. On AMD64 the dispatcher requires
 * exactly 24 input bytes, but PopShutdownListenerRemoveCallback reads 32 bytes and matches
 * QWORDs at 8, 16 and 24 against the original registration. This is a kernel-side
 * length/read mismatch: a 32-byte reported length is rejected, and a 24-byte object does not
 * cover the third key. Do not treat this declaration as a safe complete invocation contract.
 *
 * The first key is opaque registration data, the second the callback address, and the third
 * (outside this prefix) its context. No PID/TID lookup occurs. Successful removal unlinks
 * and frees the kernel's list entry;
 * no matching entry returns STATUS_NOT_FOUND. There is no NtClose handle. The inconsistent input-length requirements
 * do not define a supported caller workaround.
 * \sa POWER_INTERNAL_UNREGISTER_SHUTDOWN_NOTIFICATION_READ_LAYOUT64
 */
typedef struct _POWER_INTERNAL_UNREGISTER_SHUTDOWN_NOTIFICATION_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    PVOID RegistrationKey; // on AMD64: first QWORD of the original registration
    PVOID CallbackRoutine; // on AMD64: callback address used for matching
} POWER_INTERNAL_UNREGISTER_SHUTDOWN_NOTIFICATION_INPUT, *PPOWER_INTERNAL_UNREGISTER_SHUTDOWN_NOTIFICATION_INPUT;

#ifdef _WIN64
#endif

/**
 * The POWER_INTERNAL_UNREGISTER_SHUTDOWN_NOTIFICATION_READ_LAYOUT64 struct describes the 32-byte extent read by the AMD64 unregister callee.
 * \remarks This is NOT a valid full-size syscall request: infoclass 87 rejects input
 * lengths other than 24 even though the callee copies 32 and matches the QWORD at +24.
 * The three address-sized keys correspond to the original registration's 24-byte payload.
 * Address fields use fixed-width types; this layout does not define a callable signature.
 * Do not submit this structure or shorten its reported length as a workaround. The mismatch
 * is unresolved, and no safe invocation/buffer-construction helper is provided.
 * \sa POWER_INTERNAL_UNREGISTER_SHUTDOWN_NOTIFICATION_INPUT
 */
typedef struct _POWER_INTERNAL_UNREGISTER_SHUTDOWN_NOTIFICATION_READ_LAYOUT64
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONGLONG RegistrationKey;
    ULONGLONG CallbackAddress;
    ULONGLONG ContextAddress;
} POWER_INTERNAL_UNREGISTER_SHUTDOWN_NOTIFICATION_READ_LAYOUT64, *PPOWER_INTERNAL_UNREGISTER_SHUTDOWN_NOTIFICATION_READ_LAYOUT64;

// rev
/**
 * The POWER_INTERNAL_MANAGE_TRANSITION_STATE_RECORD_INPUT struct pushes or pops a diagnostic record attributing a blocked power-state transition.
 * \remarks Selector 88 requires at least 56 input bytes on AMD64, correcting the former
 * 48-byte layout. Action at 8 is 0 (push) or 1 (pop). Process/thread values are at 16/24,
 * the opaque ReasonValue QWORD at 32, CallbackType DWORD at 40 and CallbackContext at 48.
 * The old Reason member remains the low 32-bit view; callback members now have correct offsets.
 * Header.Version is ignored. Use NULL/0 output; supplied output is ignored rather than filled.
 *
 * User mode requires LocalSystem. When both IDs are nonzero, the helper resolves them with
 * PsLookupProcessByProcessId/PsLookupThreadByThreadId; otherwise it uses the current process
 * and thread. They are identifiers, not open handles. Kernel mode supplies actual process/
 * thread object pointers. A push references both objects; pop dereferences the global top.
 * The active transition-state gate accepts internal values 1/2; otherwise status 0xC00002D3.
 *
 * Only kernel CallbackType 1/2 with non-NULL CallbackContext enables associated callback data;
 * other cases and user mode omit it. Callable signatures are not established, so the value
 * remains opaque. Action 1 does not look up a supplied identity: it pops the shared stack.
 * Do not issue unmatched pops or use this as an arbitrary object-lifetime API. Pointer-bearing
 * x86 declarations are source compatibility only, not an x86/WOW64 syscall ABI.
 */
typedef struct _POWER_INTERNAL_MANAGE_TRANSITION_STATE_RECORD_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONG Action; // 0 pushes a record; 1 pops the global top record
    ULONG Reserved0;
    union
    {
        HANDLE ProcessId; // user mode: identifier, not an open handle
        PVOID ProcessObject; // kernel mode: referenced process object supplied by caller
    };
    union
    {
        HANDLE ThreadId; // user mode: identifier, not an open handle
        PVOID ThreadObject; // kernel mode: thread object supplied by caller
    };
    ULONGLONG ReasonValue; // on AMD64: copied as one opaque QWORD
    ULONG CallbackType; // on AMD64: 1/2 with non-NULL callback data, kernel mode only
    ULONG Reserved1;
    PVOID CallbackContext; // on AMD64: opaque callback-associated data; signature unresolved
} POWER_INTERNAL_MANAGE_TRANSITION_STATE_RECORD_INPUT, *PPOWER_INTERNAL_MANAGE_TRANSITION_STATE_RECORD_INPUT;

#ifdef _WIN64
#endif

/**
 * The POWER_INTERNAL_TRANSITION_RECORD_ACTION enum selects transition attribution push/pop without inventing a per-caller handle.
 * \remarks Selector 88 Action 0 pushes, 1 pops the global top. Both need >=56 AMD64
 * input bytes and eligible transition state. Pop is not an identity-based removal.
 */
typedef enum _POWER_INTERNAL_TRANSITION_RECORD_ACTION
{
    PowerInternalTransitionRecordPush = 0,
    PowerInternalTransitionRecordPop = 1
} POWER_INTERNAL_TRANSITION_RECORD_ACTION;

// rev
/**
 * The POWER_INTERNAL_SUSPEND_RESUME_REQUEST_INPUT struct forwards a suspend/resume integration request to a registered kernel subsystem callback.
 * \remarks Selector 90 requires at least 20 input bytes and NULL/0 output. User mode
 * requires LocalSystem; kernel mode is also accepted. The ntoskrnl route forwards the entire
 * request pointer; it does not decode Data[12] or validate Header.Version. With no registered
 * callback it returns success without performing the operation. Otherwise callback status
 * is propagated. Success therefore does not prove the system suspended or resumed.
 *
 * The callback-specific payload schema and valid suboperations are not established by this
 * wrapper. Data remains opaque; do not assign guessed Action/Flags fields or construct a
 * state-changing request merely to test support. No live invocation was performed.
 */
typedef struct _POWER_INTERNAL_SUSPEND_RESUME_REQUEST_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    UCHAR Data[12];
} POWER_INTERNAL_SUSPEND_RESUME_REQUEST_INPUT, *PPOWER_INTERNAL_SUSPEND_RESUME_REQUEST_INPUT;

#define PPM_ENERGY_ESTIMATION_CLASS_COUNT 2
#define PPM_ENERGY_ESTIMATION_BUCKET_COUNT 4

/**
 * The POWER_INTERNAL_ENERGY_ESTIMATION_INFO_INPUT struct selects the kernel's processor energy-model bucket weights for diagnostics.
 * \remarks Selector 91 accepts at least an 8-byte header and at least 36 output bytes,
 * in either caller mode. Version is ignored. Missing PpmComputeEnergyData returns
 * STATUS_UNSUCCESSFUL; a model that does not fit the fixed result returns
 * STATUS_BUFFER_TOO_SMALL. It does not measure energy consumption or reset counters.
 * \sa POWER_INTERNAL_ENERGY_ESTIMATION_INFO_OUTPUT
 */
typedef struct _POWER_INTERNAL_ENERGY_ESTIMATION_INFO_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_ENERGY_ESTIMATION_INFO_INPUT, *PPOWER_INTERNAL_ENERGY_ESTIMATION_INFO_INPUT;

// rev
/**
 * The POWER_INTERNAL_ENERGY_ESTIMATION_ENTRY struct holds four relative processor energy-model bucket weights.
 * \remarks Each entry is 16 bytes containing four ULONG values copied unchanged from
 * one model class. They are not joules, watts, residency durations or guaranteed percentages.
 * Do not require the weights to sum to 100, be monotonic, or end at 100. Use them only with
 * the corresponding model's bucket definitions; processor topology is not returned here.
 * \sa POWER_INTERNAL_ENERGY_ESTIMATION_INFO_OUTPUT
 */
typedef struct _POWER_INTERNAL_ENERGY_ESTIMATION_ENTRY
{
    ULONG RelativeBucketWeight[PPM_ENERGY_ESTIMATION_BUCKET_COUNT];
} POWER_INTERNAL_ENERGY_ESTIMATION_ENTRY, *PPOWER_INTERNAL_ENERGY_ESTIMATION_ENTRY;

// rev
/**
 * The POWER_INTERNAL_ENERGY_ESTIMATION_INFO_OUTPUT struct receives the fixed two-entry capacity of processor energy-estimation model weights.
 * \remarks Selector 91 returns 36 bytes: EntryCount DWORD at 0 and two 16-byte entries
 * at 4 and 20, each with four ULONG weights. Only EntryCount entries are valid, up to the
 * 2-entry capacity; unused result storage is zeroed by allocation. Validate the count before
 * indexing captured data. This is model configuration, not accumulated energy or a live power
 * sample. The query does not identify class IDs or specify which physical processors belong
 * in each entry. No returned kernel allocation is exposed to the caller.
 */
typedef struct _POWER_INTERNAL_ENERGY_ESTIMATION_INFO_OUTPUT
{
    _Field_range_(0, PPM_ENERGY_ESTIMATION_CLASS_COUNT) ULONG EntryCount;
    _Field_size_part_(PPM_ENERGY_ESTIMATION_CLASS_COUNT, EntryCount)
    POWER_INTERNAL_ENERGY_ESTIMATION_ENTRY Entries[PPM_ENERGY_ESTIMATION_CLASS_COUNT];
} POWER_INTERNAL_ENERGY_ESTIMATION_INFO_OUTPUT, *PPOWER_INTERNAL_ENERGY_ESTIMATION_INFO_OUTPUT;

// Internal power request and response sizes for the x64 layouts above.

// rev
/**
 * The POWER_INTERNAL_NOTIFY_WIN32K_POWER_REQUEST_INPUT struct balances queued/completed notifications for one Win32k power-request category.
 * \remarks Kernel-only infoclasses 95/96 require exactly 12 input bytes, Version 0,
 * NULL/0 output and RequestIndex in 0..31. Queued increments a per-index DWORD counter;
 * completed decrements it. Transitions to 1 on queue or to 0 on completion can queue a worker
 * when its consumer is registered. No underflow/overflow guard is visible in this path.
 *
 * These are paired lifetime notifications, not idempotent setters or count queries. Do not
 * send unmatched completions or assume RequestIndex is a PID, handle or bit mask. User mode
 * returns STATUS_ACCESS_DENIED. No request object is returned or transferred.
 */
typedef struct _POWER_INTERNAL_NOTIFY_WIN32K_POWER_REQUEST_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONG RequestIndex;
} POWER_INTERNAL_NOTIFY_WIN32K_POWER_REQUEST_INPUT, *PPOWER_INTERNAL_NOTIFY_WIN32K_POWER_REQUEST_INPUT;

/** The POWER_INTERNAL_WIN32K_POWER_REQUEST_COUNT constant is the exclusive RequestIndex limit for infoclasses 95/96: 32 categories, indices 0..31. */
#define POWER_INTERNAL_WIN32K_POWER_REQUEST_COUNT 32UL

// rev
/**
 * The POWER_INTERNAL_QUERY_VELOCITY_FEATURE_ENABLEMENT_INPUT struct queries recognition of the PDC-agent-session feature key, not session contents.
 * \remarks Selector 97 requires exactly 12 input bytes and exactly 1 output byte in
 * either caller mode; Version is ignored. Output is first FALSE, then TRUE only when
 * Servicing_PDCAgentSessions is enabled and QueryKey equals 59996711 (0x03937A27).
 * A disabled feature or other key returns successful FALSE, not STATUS_NOT_SUPPORTED.
 * The result is not a session ID, session count, or permission to invoke infoclass 98.
 * \sa POWER_INTERNAL_QUERY_VELOCITY_FEATURE_ENABLEMENT_OUTPUT
 */
typedef struct _POWER_INTERNAL_QUERY_VELOCITY_FEATURE_ENABLEMENT_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONG QueryKey;
} POWER_INTERNAL_QUERY_VELOCITY_FEATURE_ENABLEMENT_INPUT, *PPOWER_INTERNAL_QUERY_VELOCITY_FEATURE_ENABLEMENT_INPUT;

/**
 * The POWER_INTERNAL_QUERY_VELOCITY_FEATURE_ENABLEMENT_OUTPUT struct reports whether infoclass 97 recognizes the requested PDC session feature key.
 * \remarks Exactly one Boolean byte; FALSE can mean disabled feature or unknown key.
 * This is a capability probe, not a session-state record or authorization decision.
 */
typedef struct _POWER_INTERNAL_QUERY_VELOCITY_FEATURE_ENABLEMENT_OUTPUT
{
    BOOLEAN Supported;
} POWER_INTERNAL_QUERY_VELOCITY_FEATURE_ENABLEMENT_OUTPUT, *PPOWER_INTERNAL_QUERY_VELOCITY_FEATURE_ENABLEMENT_OUTPUT;

/** The POWER_INTERNAL_QUERY_VELOCITY_FEATURE_ENABLEMENT_KEY constant defines the recognized infoclass-97 key; feature enablement is still required. */
#define POWER_INTERNAL_QUERY_VELOCITY_FEATURE_ENABLEMENT_KEY 59996711UL

// rev
/**
 * The POWER_INTERNAL_SESSION_CONNECTION_INFO_V2 struct describes connection state and console/remote classification for session power accounting.
 * \remarks The 8-byte inline record is at offset 8 of infoclass 98: Connected Boolean at 0 and ConnectionType
 * DWORD at 4; three padding bytes are not consumed. Zero connection type selects console, nonzero selects
 * remote in consumer; no finer enum is established. The current process supplies the session ID, not this
 * record. This notifies the power manager and does not establish a network connection or authenticate a user.
 */
typedef struct _POWER_INTERNAL_SESSION_CONNECTION_INFO_V2
{
    BOOLEAN Connected;
    UCHAR Reserved0[3];
    ULONG ConnectionType;
} POWER_INTERNAL_SESSION_CONNECTION_INFO_V2, *PPOWER_INTERNAL_SESSION_CONNECTION_INFO_V2;

// rev
/**
 * The POWER_INTERNAL_NOTIFY_WIN32K_SESSION_CONNECT_INPUT struct updates session connection power state and obtains adaptive timeout decisions.
 * \remarks Feature-gated infoclass 98 requires Version 0, exactly 48 input bytes and
 * exactly 16 direct output bytes. When the feature is enabled, user mode is denied by the
 * dispatcher; disabled feature returns STATUS_INVALID_PARAMETER. Kernel mode still undergoes
 * SeIsAppContainerOrIdentifyLevelContext, rejecting a flagged context with ACCESS_DENIED.
 * ConnectionInfo is at 8 and the 32-byte adaptive request at 16; session ID is current-process.
 *
 * This changes session/display/user status under the adaptive lock and can invoke a registered
 * session callback. Connected requests use adaptive timeout evaluation; disconnected requests
 * write only output DisplayTimeout(+0), InputTimeout(+8), and Boolean(+12) as zero. DimTimeout
 * at +4 and bytes 13..15 are untouched on disconnect. Initialize the full output and do not
 * interpret untouched fields as a fresh result. Connected calls also leave bytes 13..15 alone.
 * Downstream callback status is discarded by the dispatcher. No handles are returned.
 * \sa POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_OUTPUT
 */
typedef struct _POWER_INTERNAL_NOTIFY_WIN32K_SESSION_CONNECT_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    POWER_INTERNAL_SESSION_CONNECTION_INFO_V2 ConnectionInfo;
    POWER_INTERNAL_ADAPTIVE_SESSION_STATE_REQUEST SessionStateRequest;
} POWER_INTERNAL_NOTIFY_WIN32K_SESSION_CONNECT_INPUT, *PPOWER_INTERNAL_NOTIFY_WIN32K_SESSION_CONNECT_INPUT;

/**
 * The POWER_INTERNAL_BATTERY_TEST_EXEMPT_INPUT struct updates the feature-gated battery test-exemption policy (numeric infoclass 99).
 * \remarks Requires at least 12 input bytes and NULL/0 output. Both caller modes are accepted by route with
 * no extra infoclass-specific privilege check; Version is ignored. Feature Servicing_BatteryTestExempt must be
 * enabled, otherwise INVALID_PARAMETER. The raw byte at +8 is stored as a policy value; use FALSE/TRUE
 * convention.
 *
 * A change persists policy through the registry helper and can cancel/wait for battery monitoring I/O,
 * remove/restart monitoring, reinitialize adapters and update capability notifications. This is not a passive
 * support probe or a general battery health setting. No payload or handle is returned.
 * \sa PowerInternalBatteryTestExempt
 */
typedef struct _POWER_INTERNAL_BATTERY_TEST_EXEMPT_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    BOOLEAN Exempt;
    UCHAR Reserved[3];
} POWER_INTERNAL_BATTERY_TEST_EXEMPT_INPUT, *PPOWER_INTERNAL_BATTERY_TEST_EXEMPT_INPUT;

// rev
// POWER_INFORMATION_ENERGY_TRACKER_CREATE_INPUT Flags values
#define POWER_INFORMATION_ENERGY_TRACKER_PROCESS_MIN                    ULONG_C(0x00000001)
#define POWER_INFORMATION_ENERGY_TRACKER_PROCESS_MAX                    ULONG_C(0x00040000)

#define POWER_INFORMATION_ENERGY_TRACKER_CREATE_FLAGS_NONE              ULONG_C(0x00000000)
#define POWER_INFORMATION_ENERGY_TRACKER_CREATE_FLAGS_MODE_PID          ULONG_C(0x00000001) // enables per-process aggregation with process ID and sequence number.
#define POWER_INFORMATION_ENERGY_TRACKER_CREATE_FLAGS_MODE_EXTENDED_V1  ULONG_C(0x10000000) // PROCESS_EXTENDED_ENERGY_VALUES_V1 (0x1B8 bytes)
#define POWER_INFORMATION_ENERGY_TRACKER_CREATE_FLAGS_MODE_MASK         ULONG_C(0xF0000000)

// rev
/**
 * The POWER_INFORMATION_ENERGY_TRACKER_CREATE_INPUT structure contains the input parameters for the energy tracker create operation.
 *
 * Despite its name, the low 16 bits of Reserved are used as a process energy-state aggregation mask
 * in the kernel (PROCESS_ENERGY_STATE_*). Each process entry contains the captured state
 * AND this mask in StateFlags. A zero mask produces zero StateFlags.
 */
typedef struct _POWER_INFORMATION_ENERGY_TRACKER_CREATE_INPUT
{
    ULONG MaxTrackedProcesses; // POWER_INFORMATION_ENERGY_TRACKER_PROCESS_MAX
    ULONG Reserved;            // Low 16 bits: PROCESS_ENERGY_STATE_* aggregation mask.
    ULONG Flags;               // POWER_INFORMATION_ENERGY_TRACKER_CREATE_FLAGS
} POWER_INFORMATION_ENERGY_TRACKER_CREATE_INPUT, *PPOWER_INFORMATION_ENERGY_TRACKER_CREATE_INPUT;

// rev
/**
 * The POWER_INFORMATION_ENERGY_TRACKER_CREATE_OUTPUT structure contains the output data returned by the energy tracker create operation.
 */
typedef struct _POWER_INFORMATION_ENERGY_TRACKER_CREATE_OUTPUT
{
    HANDLE QueryHandle;
} POWER_INFORMATION_ENERGY_TRACKER_CREATE_OUTPUT, *PPOWER_INFORMATION_ENERGY_TRACKER_CREATE_OUTPUT;

// rev
/**
 * The POWER_INFORMATION_ENERGY_TRACKER_QUERY_INPUT structure contains the input parameters for the energy tracker query operation.
 */
typedef struct _POWER_INFORMATION_ENERGY_TRACKER_QUERY_INPUT
{
    HANDLE QueryHandle;
} POWER_INFORMATION_ENERGY_TRACKER_QUERY_INPUT, *PPOWER_INFORMATION_ENERGY_TRACKER_QUERY_INPUT;

#define POWER_INFORMATION_ENERGY_TRACKER_SIGNATURE 0x00200013

// rev
/**
 * The POWER_INFORMATION_ENERGY_TRACKER_STATISTICS structure contains tracker diagnostic counters.
 *
 * StatisticsOffset and CumulativeStatisticsOffset in POWER_INFORMATION_ENERGY_TRACKER_QUERY_HEADER
 * and POWER_INFORMATION_ENERGY_TRACKER_QUERY_OUTPUT both locate this 0x10-byte structure.
 * StatisticsOffset reports counters accumulated since creation or the previous successful query.
 * CumulativeStatisticsOffset reports the previous cumulative totals plus the current-query counters.
 * A successful query clears the current counters and retains the updated cumulative totals.
 * These are event counts, not energy values or counts of distinct processes.
 *
 * AggregateFallbackCount includes failures to allocate or insert an aggregate and attempts blocked
 * by MaxTrackedProcesses. AggregateLimitHitCount counts the latter attempts as well, so these
 * counters overlap and must not be added together as independent failure totals.
 * ProcessSnapshotMissCount increments when no snapshot is found outside tracker initialization,
 * the snapshot reason is not 1 (initial enumeration), and the process energy-state sequence is greater than 1.
 * ProcessSnapshotFailureCount counts failures to allocate a process snapshot or its initial hash table.
 */
typedef struct _POWER_INFORMATION_ENERGY_TRACKER_STATISTICS
{
    ULONG AggregateFallbackCount;            // Updates directed to the built-in fallback aggregate.
    ULONG ProcessSnapshotMissCount;          // Missing snapshots satisfying the conditions above.
    ULONG ProcessSnapshotFailureCount;       // Process snapshot creation failures.
    ULONG AggregateLimitHitCount;            // Aggregate creation attempts blocked by MaxTrackedProcesses.
} POWER_INFORMATION_ENERGY_TRACKER_STATISTICS, *PPOWER_INFORMATION_ENERGY_TRACKER_STATISTICS;

// rev
/**
 * The POWER_INFORMATION_ENERGY_TRACKER_QUERY_HEADER structure describes the fixed header of an energy tracker query.
 *
 * All offsets are byte offsets from the start of the output buffer. ProcessRecordOffset locates
 * ProcessRecordCount entries, each ProcessRecordSize bytes long. StatisticsOffset and
 * CumulativeStatisticsOffset locate the current-query and cumulative tracker statistics, respectively;
 * each block is a POWER_INFORMATION_ENERGY_TRACKER_STATISTICS structure (0x10 bytes), not energy values.
 * TrackerConfigurationOffset locates the effective POWER_INFORMATION_ENERGY_TRACKER_CREATE_INPUT (0x0C bytes).
 * The tick-count fields contain milliseconds derived from TickCount and TickCountMultiplier, not QPC ticks.
 * TickCountDelta and UnbiasedInterruptTimeMsDelta are measured since creation or the previous successful query.
 */
typedef struct _POWER_INFORMATION_ENERGY_TRACKER_QUERY_HEADER
{
    ULONG Signature;                         // POWER_INFORMATION_ENERGY_TRACKER_SIGNATURE
    ULONG HeaderSize;                        // Size of the fixed header, in bytes; zero on STATUS_BUFFER_TOO_SMALL.
    ULONG TotalSize;                         // Required output buffer size, in bytes.
    ULONG Sequence;                          // Starts at 1; advances after each successful query.
    ULONG TickCountDelta;                    // Elapsed tick-count milliseconds.
    ULONG UnbiasedInterruptTimeMsDelta;      // Elapsed unbiased interrupt-time milliseconds.
    ULONG SnapshotTickCount;                 // Tick count before serializing the records.
    ULONG CurrentTickCount;                  // Tick count after serializing the records.
    ULONG CurrentTimelineBitmapTime;         // KeQueryTimelineBitmapTime.
    ULONG ProcessRecordOffset;               // Offset to POWER_INFORMATION_ENERGY_TRACKER_PROCESS_ENTRY[ProcessRecordCount]; 0x48.
    ULONG ProcessRecordCount;                // Number of aggregate process records.
    ULONG StatisticsOffset;                  // Offset to POWER_INFORMATION_ENERGY_TRACKER_STATISTICS (0x10 bytes); current-query counters.
    ULONG CumulativeStatisticsOffset;        // Offset to POWER_INFORMATION_ENERGY_TRACKER_STATISTICS (0x10 bytes); cumulative counters, including this query.
    ULONG TrackerConfigurationOffset;        // Offset to POWER_INFORMATION_ENERGY_TRACKER_CREATE_INPUT (0x0C bytes); effective creation parameters.
    USHORT ProcessRecordSize;                // Size of each aggregate process record, in bytes.
    USHORT StatisticsSize;                   // Size of each tracker statistics block, in bytes.
    USHORT TrackerConfigurationSize;         // Size of the effective tracker configuration, in bytes.
    USHORT Reserved;                         // Reserved.
    ULONG CurrentSystemTimeLow;              // Low 32 bits of the system time.
    ULONG CurrentSystemTimeHigh;             // High 32 bits of the system time.
} POWER_INFORMATION_ENERGY_TRACKER_QUERY_HEADER, *PPOWER_INFORMATION_ENERGY_TRACKER_QUERY_HEADER;

// rev
/**
 * The POWER_INFORMATION_ENERGY_TRACKER_QUERY_OUTPUT structure contains the fixed prefix returned by EnergyTrackerQuery.
 *
 * This prefix has the same layout as POWER_INFORMATION_ENERGY_TRACKER_QUERY_HEADER.
 * Variable-length process records, energy values, tracker configuration, statistics, SIDs and strings
 * follow it and are located by their buffer-relative offsets; TotalSize includes all of these sections.
 * On STATUS_BUFFER_TOO_SMALL, a buffer of at least 0x0C bytes receives Signature, HeaderSize = 0,
 * and the required TotalSize. The remaining fields are valid only after a successful query.
 */
typedef struct _POWER_INFORMATION_ENERGY_TRACKER_QUERY_OUTPUT
{
    ULONG Signature;                         // POWER_INFORMATION_ENERGY_TRACKER_SIGNATURE
    ULONG HeaderSize;                        // Size of the fixed header, in bytes; zero on STATUS_BUFFER_TOO_SMALL.
    ULONG TotalSize;                         // Required output buffer size, in bytes.
    ULONG Sequence;                          // Starts at 1; advances after each successful query.
    ULONG TickCountDelta;                    // Elapsed tick-count milliseconds.
    ULONG UnbiasedInterruptTimeMsDelta;      // Elapsed unbiased interrupt-time milliseconds.
    ULONG SnapshotTickCount;                 // Tick count before serializing the records.
    ULONG CurrentTickCount;                  // Tick count after serializing the records.
    ULONG CurrentTimelineBitmapTime;         // KeQueryTimelineBitmapTime.
    ULONG ProcessRecordOffset;               // Offset to POWER_INFORMATION_ENERGY_TRACKER_PROCESS_ENTRY[ProcessRecordCount]; 0x48.
    ULONG ProcessRecordCount;                // Number of aggregate process records.
    ULONG StatisticsOffset;                  // Offset to POWER_INFORMATION_ENERGY_TRACKER_STATISTICS (0x10 bytes); current-query counters.
    ULONG CumulativeStatisticsOffset;        // Offset to POWER_INFORMATION_ENERGY_TRACKER_STATISTICS (0x10 bytes); cumulative counters, including this query.
    ULONG TrackerConfigurationOffset;        // Offset to POWER_INFORMATION_ENERGY_TRACKER_CREATE_INPUT (0x0C bytes); effective creation parameters.
    USHORT ProcessRecordSize;                // Size of each aggregate process record, in bytes.
    USHORT StatisticsSize;                   // Size of each tracker statistics block, in bytes.
    USHORT TrackerConfigurationSize;         // Size of the effective tracker configuration, in bytes.
    USHORT Reserved;                         // Reserved.
    ULONG CurrentSystemTimeLow;              // Low 32 bits of the system time.
    ULONG CurrentSystemTimeHigh;             // High 32 bits of the system time.
} POWER_INFORMATION_ENERGY_TRACKER_QUERY_OUTPUT, *PPOWER_INFORMATION_ENERGY_TRACKER_QUERY_OUTPUT;

// rev
// POWER_INFORMATION_ENERGY_TRACKER_PROCESS_ENTRY ActivityFlags values.
#define POWER_INFORMATION_ENERGY_TRACKER_ACTIVITY_FOREGROUND       ULONG_C(0x00000001)
#define POWER_INFORMATION_ENERGY_TRACKER_ACTIVITY_DESKTOP_VISIBLE  ULONG_C(0x00000002)

// rev
/**
 * The POWER_INFORMATION_ENERGY_TRACKER_PROCESS_ENTRY structure describes an aggregate process record.
 *
 * All offsets are byte offsets from the start of the query output buffer, not from this entry. String counts
 * are WCHAR counts, exclude a terminating null, and do not imply null termination. UserSidLength is a byte
 * count. ProcessKey and ProcessId are zero unless POWER_INFORMATION_ENERGY_TRACKER_CREATE_FLAGS_MODE_PID is
 * enabled; ProcessKey is the process sequence number. The six trailing ULONG values count nonempty
 * energy-delta updates and contributing processes, not energy.
 *
 * StateFlags contains PROCESS_ENERGY_STATE_* bits captured in the aggregation key, masked by the low 16 bits
 * of POWER_INFORMATION_ENERGY_TRACKER_CREATE_INPUT.Reserved. It is not a duration-history indicator or
 * necessarily the process state at query completion. A zero creation mask produces zero StateFlags. Bits are
 * 0x1 (foreground), 0x2 (desktop-visible), 0x4 (PSM foreground), and 0x8 (PSM background).
 *
 * ActivityFlags contains POWER_INFORMATION_ENERGY_TRACKER_ACTIVITY_* bits ORed during nonempty energy-delta
 * aggregation: 0x1 for nonzero ForegroundDuration and 0x2 for nonzero DesktopVisibleDuration in a
 * contributing input snapshot. The kernel tests accumulated Duration values (excluding IsInState), not
 * calculated interval deltas. These flags therefore do not prove activity occurred in the current query
 * interval.
 * \remarks Snapshot reasons are internal discriminators passed as the fourth argument to
 * PopEtEnergyTrackerUpdateAggregate, not output flags or energy units:
 * - Reason 2 (explicit query): PopEtEnergyTrackerQuery enumerates processes through
 * PopEtProcessEnumSnapshotCallback to capture their current energy values; increments QuerySnapshotCount.
 * - Reason 3 (tracking-state change): PopEtEnergyContextProcessStateUpdate captures the process energy
 * values and notifies the energy trackers; increments StateChangeSnapshotCount.
 * - Reason 4 (process cleanup): PoEnergyContextCleanup captures the final state durations and notifies
 * the energy trackers before cleaning up the process energy context; increments TerminatedProcessCount.
 *
 * A reason-specific counter increments only when the calculated energy delta is nonempty. Every such update
 * also increments SnapshotCount. QuerySnapshotCount is therefore not simply the number of query API calls,
 * and TerminatedProcessCount does not count every process termination.
 *
 * SectionOffset locates the aggregated energy deltas for the query interval, aligned to 8 bytes. SectionSize
 * is 0x1B0 for PROCESS_EXTENDED_ENERGY_VALUES (Flags high nibble 0), or 0x1B8 for
 * PROCESS_EXTENDED_ENERGY_VALUES_V1 (Flags high nibble 1), in the kernel.
 */
typedef struct _POWER_INFORMATION_ENERGY_TRACKER_PROCESS_ENTRY
{
    ULONGLONG ProcessKey;                     // Process sequence number in per-process mode.
    ULONG ProcessId;                          // Process identifier in per-process mode; zero otherwise.
    USHORT StateFlags;                        // PROCESS_ENERGY_STATE_*; masked aggregation-key state.
    USHORT Reserved0E;                        // Reserved.
    ULONG FileNameOffset;                     // Process image path, or the fallback name.
    ULONG ImageCheckSum;                      // PE OptionalHeader.CheckSum.
    ULONG ImageTimeDateStamp;                 // PE FileHeader.TimeDateStamp.
    ULONG PackageFullNameOffset;              // Buffer-relative offset to the package full name.
    ULONG ApplicationIdOffset;                // Buffer-relative offset to the application identifier (RtlQueryPackageIdentity).
    ULONG SubProcessTagNameOffset;            // Buffer-relative offset to the subprocess tag name.
    ULONG UserSidOffset;                      // Buffer-relative offset to the token user SID.
    ULONG FileVersionLow;                     // VS_FIXEDFILEINFO.dwFileVersionLS.
    ULONG FileVersionHigh;                    // VS_FIXEDFILEINFO.dwFileVersionMS.
    USHORT FileNameCount;                     // Image path or fallback name length, in WCHARs, excluding a terminating null.
    USHORT PackageFullNameCount;              // Package full name length, in WCHARs, excluding a terminating null.
    USHORT ApplicationIdCount;                // Application identifier length, in WCHARs, excluding a terminating null.
    USHORT SubProcessTagNameCount;            // Subprocess tag name length, in WCHARs, excluding a terminating null.
    USHORT UserSidLength;                     // Size of the binary user SID, in bytes.
    USHORT Reserved;                          // Reserved.
    ULONG SectionOffset;                      // PROCESS_EXTENDED_ENERGY_VALUES // PROCESS_EXTENDED_ENERGY_VALUES_V1
    ULONG SectionSize;                        // Size of the aggregated energy-delta section, in bytes.
    ULONG ActiveTimeMs;                       // Accumulated unbiased elapsed time, capped to the query interval.
    ULONG ActivityFlags;                      // POWER_INFORMATION_ENERGY_TRACKER_ACTIVITY_*; accumulated-duration indicators.
    ULONG SnapshotCount;                      // All nonempty energy-delta updates.
    ULONG QuerySnapshotCount;                 // Updates during explicit queries (reason 2).
    ULONG StateChangeSnapshotCount;           // Updates during energy-tracking state changes (reason 3).
    ULONG ProcessCount;                       // Distinct processes contributing a nonempty delta in this query.
    ULONG NewProcessCount;                    // Processes contributing their first nonempty delta to this tracker.
    ULONG TerminatedProcessCount;             // Nonempty updates during process energy-context cleanup (reason 4).
} POWER_INFORMATION_ENERGY_TRACKER_PROCESS_ENTRY, *PPOWER_INFORMATION_ENERGY_TRACKER_PROCESS_ENTRY;

// rev
DEFINE_GUID(PopBlackBoxScmGuid, 0x45F9D5A3, 0xE1D0, 0x8891, 0x07, 0x26, 0xFB, 0x1D, 0x71, 0xAD, 0x11, 0xB8);
DEFINE_GUID(PopBlackBoxBsdGuid, 0x4E01CC45, 0xF573, 0x08DF, 0x0E, 0xC1, 0x0B, 0x0E, 0xBA, 0x42, 0x97, 0x6A);
DEFINE_GUID(PopBlackBoxPnpGuid, 0x4CD6532A, 0xB763, 0x1941, 0x57, 0x45, 0x7D, 0x91, 0xB5, 0xED, 0xB1, 0xB1);
DEFINE_GUID(PopBlackBoxAcpiGuid, 0x429FF755, 0x3B2E, 0xA98B, 0x8C, 0x52, 0x06, 0x81, 0xA1, 0x31, 0xC1, 0x80);
DEFINE_GUID(PopBlackBoxPoIrpGuid, 0x4A654DDB, 0x2523, 0xDB46, 0x0C, 0x65, 0xC9, 0x83, 0xF0, 0xE9, 0x13, 0x9A);
DEFINE_GUID(PopBlackBoxWinLogonNotifyGuid, 0x4E3EAA07, 0x6B2D, 0x3E93, 0x3B, 0xC6, 0x3C, 0x0E, 0x6D, 0x91, 0x1A, 0xA4);
DEFINE_GUID(PopBlackBoxPdcLockGuid, 0x4E912A6E, 0x33DB, 0xDDBB, 0x84, 0x8A, 0x7B, 0x99, 0xE1, 0x5D, 0x42, 0x9E);
DEFINE_GUID(PopBlackBoxPoPepWorkOrderGuid, 0x42750E88, 0xE0E8, 0x5A55, 0x0D, 0x03, 0x45, 0xAF, 0xB3, 0xF1, 0x33, 0xF9);
DEFINE_GUID(PopBlackBoxPoPowerWatchdogGuid, 0x44675326, 0x5545, 0xF79E, 0x55, 0x31, 0xE3, 0x3A, 0x63, 0x81, 0x69, 0xAE);
DEFINE_GUID(PopBlackBoxPnpEventWorkerGuid, 0x4131386C, 0x8BEF, 0xF310, 0x4A, 0x68, 0x1E, 0xFA, 0x44, 0x0A, 0xB1, 0xB1);
DEFINE_GUID(PopBlackBoxPnpDeviceCompletionQueueGuid, 0x452E8590, 0xC129, 0x4D5E, 0x68, 0xCA, 0x00, 0xF7, 0x45, 0x7F, 0x71, 0xBC);
DEFINE_GUID(PopBlackBoxPnpDelayedRemoveWorkerGuid, 0x4D9CFF3A, 0x7392, 0xA43B, 0x08, 0xED, 0xCD, 0x65, 0xAA, 0x50, 0x31, 0xBA);
DEFINE_GUID(PopBlackBoxDxgDisplayGuid, 0x44D6ED00, 0xB3BE, 0xB7EE, 0x1C, 0x0F, 0xF8, 0x2D, 0xA9, 0xC1, 0x60, 0xAD);
DEFINE_GUID(PopBlackBoxCrashedProcessGuid, 0x4367A550, 0xBE84, 0xD651, 0x94, 0x1F, 0x63, 0x2B, 0x79, 0xAB, 0x63, 0x8E);
DEFINE_GUID(PopBlackBoxUsoCommitGuid, 0x4CDA57F3, 0x6DE4, 0xC85A, 0x0E, 0x4F, 0x85, 0x85, 0xA8, 0x5C, 0x8F, 0xE8);
DEFINE_GUID(PopBlackBoxWheaGuid, 0x457D912A, 0x32D3, 0xEA49, 0x0F, 0xC5, 0x3E, 0xF2, 0x92, 0x9D, 0xDE, 0x6B);
DEFINE_GUID(PopBlackBoxNtfsGuid, 0x4213940D, 0x00AF, 0xE9C4, 0x20, 0xBC, 0xB5, 0x19, 0x37, 0xCD, 0x16, 0x80);
DEFINE_GUID(PopBlackBoxWinLogonGuid, 0x4AF1A719, 0x80CC, 0x79CF, 0x0C, 0x1E, 0xB7, 0x6F, 0xF2, 0x9F, 0xE9, 0x7B);
DEFINE_GUID(PopBlackBoxExplorerLogonTasksGuid, 0x4D93B9AC, 0xAA9A, 0x6517, 0x0A, 0x03, 0x93, 0x6E, 0x66, 0x62, 0x6D, 0x08);
DEFINE_GUID(PopBlackBoxExplorerCoreStartupGuid, 0x4E121623, 0xF5A6, 0xB2E1, 0x0F, 0xA8, 0x08, 0x29, 0xBA, 0x84, 0x83, 0x98);
DEFINE_GUID(PopBlackBoxUserModeLKDReasonGuid, 0x44BEB1A5, 0xC1B9, 0x41DF, 0x22, 0x5E, 0xBC, 0x66, 0xF1, 0xDA, 0x5C, 0x9D);
DEFINE_GUID(PopBlackBoxCodeIntegrityGuid, 0x44A03CF4, 0x4EE7, 0x6BD8, 0x0A, 0x33, 0x73, 0xE6, 0x43, 0x73, 0x9A, 0x0C);
DEFINE_GUID(PoBlackBoxIdCsrGuid, 0x470BC061, 0x42C1, 0xADD0, 0x0C, 0xB1, 0x8E, 0x99, 0xF2, 0xEF, 0x68, 0xFB);
DEFINE_GUID(PoBlackBoxIdSmGuid, 0x42D2AC4A, 0xD368, 0xF58F, 0x25, 0xA7, 0x6A, 0xC2, 0xDB, 0x76, 0x97, 0x8F);

// rev
/**
 * The POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_CATEGORY structure describes power information bbr direct access request category.
 */
typedef struct _POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_CATEGORY
{
    ULONG Index;
    PCSTR Name;
    GUID Guid;
} POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_CATEGORY, *PPOWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_CATEGORY;

// rev
//CONST POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_CATEGORY BlackBoxCategories[24] =
//{
//    { 0, "SCM", PopBlackBoxScmGuid },
//    { 1, "BSD", PopBlackBoxBsdGuid },
//    { 2, "PNP", PopBlackBoxPnpGuid },
//    { 3, "ACPI", PopBlackBoxAcpiGuid },
//    { 4, "POIRP", PopBlackBoxPoIrpGuid },
//    { 5, "WINLOGON-NOTIFY", PopBlackBoxWinLogonNotifyGuid },
//    { 6, "PDCLOCK", PopBlackBoxPdcLockGuid },
//    { 7, "PEPWORKORDER", PopBlackBoxPoPepWorkOrderGuid },
//    { 8, "POWERWATCHDOG", PopBlackBoxPoPowerWatchdogGuid },
//    { 9, "PNPEVENTWORKER", PopBlackBoxPnpEventWorkerGuid },
//    { 10, "DEVICECOMPLETIONQUEUE", PopBlackBoxPnpDeviceCompletionQueueGuid },
//    { 11, "PNPDELAYEDREMOVEWORKER", PopBlackBoxPnpDelayedRemoveWorkerGuid },
//    { 12, "DXG-DISPLAY", PopBlackBoxDxgDisplayGuid },
//    { 13, "CrashedProcess", PopBlackBoxCrashedProcessGuid },
//    { 14, "UsoCommit", PopBlackBoxUsoCommitGuid },
//    { 15, "WHEA", PopBlackBoxWheaGuid },
//    { 16, "NTFS", PopBlackBoxNtfsGuid },
//    { 17, "Winlogon", PopBlackBoxWinLogonGuid },
//    { 18, "Explorer logon tasks", PopBlackBoxExplorerLogonTasksGuid },
//    { 19, "Explorer core startup", PopBlackBoxExplorerCoreStartupGuid },
//    { 20, "User mode LKD API caller data", PopBlackBoxUserModeLKDReasonGuid },
//    { 21, "CI", PopBlackBoxCodeIntegrityGuid },
//    { 22, "CSR", PoBlackBoxIdCsrGuid },
//    { 23, "SM", PoBlackBoxIdSmGuid },
//};

// rev
/**
 * The POWER_INFORMATION_BBR_UPDATE_REQUEST_INPUT structure contains the input parameters for the UpdateBlackBoxRecorder (94) information level,
 * used to write a payload into the power black-box recorder entry selected by Index.
 * \remarks InputBufferLength must be sizeof(POWER_INFORMATION_BBR_UPDATE_REQUEST_INPUT) (0x20). Layout reversed from PopBlackBoxUpdate.
 */
typedef struct _POWER_INFORMATION_BBR_UPDATE_REQUEST_INPUT
{
    PVOID Buffer;   // Source data copied into the recorder entry.
    SIZE_T Length;  // Length, in bytes, of the data at Buffer.
    SIZE_T Offset;  // Destination offset within the entry (used when Flags & 1).
    ULONG Index;    // Recorder entry/category index (valid range 0..24).
    ULONG Flags;    // Bit 0: 1 = write Length bytes at Offset, 0 = replace snapshot (up to 4096 bytes).
} POWER_INFORMATION_BBR_UPDATE_REQUEST_INPUT, *PPOWER_INFORMATION_BBR_UPDATE_REQUEST_INPUT;

// rev
// note: In current builds PopBlackBoxDirectAccess does not evaluate these Category/Modifiers
// flags; the level-97 handler only reads the plain Index field (see REQUEST_INPUT remarks below).
#define POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_MAX_CATEGORY 24
#define POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_CATEGORY_SHIFT 0
#define POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_CATEGORY_MASK_RAW  0x0000FFFF
#define POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_CATEGORY_MASK_DIRECT 0x0000001F
#define POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_MODIFIERS_MASK 0xFFFF0000

// rev
#define POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_FLAGS(category, modifiers) \
    ((ULONG)(((category) & POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_CATEGORY_MASK_RAW << POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_CATEGORY_SHIFT) | \
    ((ULONG)(modifiers) & POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_MODIFIERS_MASK)))

// rev
/**
 * The POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_INPUT structure contains the input parameters for the BlackBoxRecorderDirectAccessBuffer (97) information level,
 * used to obtain a direct-access mapping (kernel address + size) to the power black-box recorder entry selected by Index.
 * \remarks InputBufferLength must be at least sizeof(POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_INPUT) (0x20). Layout reversed from PopBlackBoxDirectAccess.
 * The handler requires Reserved0..Reserved3 to be zero and only honors Index; the Version/Flags/Category/Modifiers/Offset/Length
 * form and the POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_* macros above are not evaluated by the level-97 handler.
 */
typedef struct _POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_INPUT
{
    ULONGLONG Reserved0;    // Must be 0.
    ULONGLONG Reserved1;    // Must be 0 (Offset is not honored).
    ULONGLONG Reserved2;    // Must be 0 (Length is not honored).
    ULONG Index;            // Recorder category index (valid range 0..24); selects PopBlackBoxEntries[Index].
    ULONG Reserved3;        // Must be 0.
} POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_INPUT, *PPOWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_INPUT;

// rev
/**
 * The POWER_INFORMATION_BBR_DIRECT_ACCESS_RESPONSE_OUTPUT structure contains the output data returned by the bbr direct access response operation.
 */
typedef struct _POWER_INFORMATION_BBR_DIRECT_ACCESS_RESPONSE_OUTPUT
{
    PVOID UserMappingBase;
    SIZE_T UserMappingSize;
} POWER_INFORMATION_BBR_DIRECT_ACCESS_RESPONSE_OUTPUT, *PPOWER_INFORMATION_BBR_DIRECT_ACCESS_RESPONSE_OUTPUT;

#if (PHNT_MODE != PHNT_MODE_KERNEL)
/**
 * The NtPowerInformation routine sets or retrieves system power information.
 *
 * \param InformationLevel Specifies the requested information level, which indicates the specific power information to be set or retrieved.
 * \param InputBuffer Optional pointer to a caller-allocated input buffer.
 * \param InputBufferLength Size, in bytes, of the buffer at InputBuffer.
 * \param OutputBuffer Optional pointer to an output buffer. The type depends on the InformationLevel requested.
 * \param OutputBufferLength Size, in bytes, of the output buffer.
 * \return Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-ntpowerinformation
 * \remarks This is a class dispatcher, not a uniformly read-only query. For example,
 * SystemBatteryState is an output query, while other infoclasses may alter power policy. Validate each infoclass
 * and its input/output lengths separately; a successful battery query does not validate sleep or shutdown paths.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPowerInformation(
    _In_ POWER_INFORMATION_LEVEL InformationLevel,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength
    );

/**
 * The NtSetThreadExecutionState routine informs the system of execution requirements,
 *
 * in order to prevent the system from entering sleep or turning off the display while the application is running.
 * \param NewFlags New execution state flags.
 * \param PreviousFlags Pointer to receive the previous execution state flags.
 * \return Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate
 * \remarks Controls the calling thread's legacy power request rather than global sleep
 * capability. The validator admits bits within 0x80000043 and requires bit 0x80000000 when 0x40
 * is present. Preserve and restore the prior execution-state policy when using a temporary request.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetThreadExecutionState(
    _In_ EXECUTION_STATE NewFlags, // ES_* flags
    _Out_ EXECUTION_STATE *PreviousFlags
    );
#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

#if (PHNT_VERSION < PHNT_WINDOWS_7)
/**
 * The NtRequestWakeupLatency routine requests the system resume latency.
 *
 * \param latency The desired latency time.
 * \return Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRequestWakeupLatency(
    _In_ LATENCY_TIME latency
    );
#endif // (PHNT_VERSION < PHNT_WINDOWS_7)

/**
 * The NtInitiatePowerAction routine initiates a shutdown and optional restart of the specified computer.
 *
 * \param SystemAction The system power action.
 * \param LightestSystemState The lightest system power state.
 * \param Flags Flags for the power action.
 * \param Asynchronous Whether the action is asynchronous.
 * \return Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-initiatesystemshutdownw
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtInitiatePowerAction(
    _In_ POWER_ACTION SystemAction,
    _In_ SYSTEM_POWER_STATE LightestSystemState,
    _In_ ULONG Flags, // POWER_ACTION_* flags
    _In_ BOOLEAN Asynchronous
    );

/**
 * The NtSetSystemPowerState routine initiates a suspension and optional forced shutdown of the specified computer.
 *
 * \param SystemAction The system power action.
 * \param LightestSystemState The lightest system power state.
 * \param Flags Flags for the power action.
 * \return Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setsystempowerstate
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSystemPowerState(
    _In_ POWER_ACTION SystemAction,
    _In_ SYSTEM_POWER_STATE LightestSystemState,
    _In_ ULONG Flags // POWER_ACTION_* flags
    );

/**
 * The NtGetDevicePowerState routine retrieves the current power state of the specified device.
 *
 * \param Device A handle to an object on the device, such as a file or socket, or a handle to the device itself.
 * \param State A pointer to the variable that receives the power state.
 * \return Successful or errant status.
 * \remarks An application can use NtGetDevicePowerState to determine whether a device is in the working state or a low-power state.
 * If the device is in a low-power state, accessing the device may cause it to either queue or fail any I/O requests, or transition the device into the working state.
 * The exact behavior depends on the implementation of the device.
 * \remarks Queries power state through a file object and its related target device.
 * The handle is not a process or thread handle; the entry references IoFileObjectType before device lookup.
 * A failed lookup does not establish that a device is powered off.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetDevicePowerState(
    _In_ HANDLE Device,
    _Out_ PDEVICE_POWER_STATE State
    );

/**
 * The NtIsSystemResumeAutomatic routine checks if the system resume is automatic.
 *
 * \return BOOLEAN TRUE if the system resume is automatic, FALSE otherwise.
 * \remarks Returns BOOLEAN, unlike most Nt entries. The routine returns
 * whether bits 0 and 1 of PopFullWake are both clear. It reports a wake-state observation, not a request to
 * wake, suspend or change system power policy.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOLEAN
NTAPI
NtIsSystemResumeAutomatic(
    VOID
    );

#endif // _NTPOAPI_H
