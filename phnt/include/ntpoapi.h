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
 * The PROCESSOR_GROUP_SELECTOR structure describes processor group selector.
 */
typedef struct _PROCESSOR_GROUP_SELECTOR
{
    USHORT Group; // +0x00 Processor group index to clear.
} PROCESSOR_GROUP_SELECTOR, *PPROCESSOR_GROUP_SELECTOR;

// rev
/**
 * The PROCESSOR_GROUP_PARK_MASK structure describes processor group park mask.
 */
typedef struct _PROCESSOR_GROUP_PARK_MASK
{
    ULONGLONG ForceMask; // +0x00 Forced parked logical-processor mask for Group.
    USHORT Group; // +0x08 Processor group index.
    USHORT Reserved1; // +0x0A Must be zero.
    USHORT Reserved2; // +0x0C Must be zero.
    USHORT Reserved3; // +0x0E Must be zero.
} PROCESSOR_GROUP_PARK_MASK, *PPROCESSOR_GROUP_PARK_MASK;

// rev
/**
 * The PROCESSOR_GROUP_PARK_MASK_EX structure describes processor group park mask ex.
 */
typedef struct _PROCESSOR_GROUP_PARK_MASK_EX
{
    ULONGLONG ForceMask; // +0x00 Forced parked logical-processor mask for Group.
    USHORT Group; // +0x08 Processor group index.
    USHORT Reserved1; // +0x0A Must be zero.
    USHORT Reserved2; // +0x0C Must be zero.
    USHORT Reserved3; // +0x0E Must be zero.
    ULONGLONG SecondaryMask; // +0x10 Secondary forced mask; must be a subset of ForceMask.
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
//       ULONGLONG ForceMask;   // +0x00
//       USHORT Group;          // +0x08
//       USHORT Reserved1;      // +0x0A
//       USHORT Reserved2;      // +0x0C
//       USHORT Reserved3;      // +0x0E
//   } PROCESSOR_GROUP_PARK_MASK;
//
//   For the 0x18 form:
//   typedef struct _PROCESSOR_GROUP_PARK_MASK_EX
//   {
//       ULONGLONG ForceMask;       // +0x00
//       USHORT Group;              // +0x08
//       USHORT Reserved1;          // +0x0A
//       USHORT Reserved2;          // +0x0C
//       USHORT Reserved3;          // +0x0E
//       ULONGLONG SecondaryMask;   // +0x10
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
 * The POWER_INFORMATION_LEVEL_INTERNAL enumeration defines power information level internal values.
 */
typedef enum _POWER_INFORMATION_LEVEL_INTERNAL
{
    PowerInternalAcpiInterfaceRegister,                         // in: POWER_INTERNAL_ACPI_INTERFACE_REGISTER_INPUT, out: POWER_INTERNAL_ACPI_INTERFACE_REGISTER_OUTPUT
    PowerInternalS0LowPowerIdleInfo,                            // out: POWER_S0_LOW_POWER_IDLE_INFO
    PowerInternalReapplyBrightnessSettings,                     // in: void
    PowerInternalUserAbsencePrediction,                         // out: POWER_USER_ABSENCE_PREDICTION
    PowerInternalUserAbsencePredictionCapability,               // out: POWER_USER_ABSENCE_PREDICTION_CAPABILITY
    PowerInternalPoProcessorLatencyHint,                        // in: POWER_PROCESSOR_LATENCY_HINT (InputBufferLength >= 0x0C), out: NULL // PopPowerInformationInternal (inline): PoLatencySensitivityHint(Type); user-mode callers only (kernel-mode returns STATUS_NOT_SUPPORTED)
    PowerInternalStandbyNetworkRequest,                         // out: POWER_STANDBY_NETWORK_REQUEST (requires PopNetBIServiceSid)
    PowerInternalDirtyTransitionInformation,                    // out: BOOLEAN
    PowerInternalSetBackgroundTaskState,                        // out: POWER_SET_BACKGROUND_TASK_STATE
    PowerInternalTtmOpenTerminal,                               // in: POWER_INTERNAL_TTM_OPEN_TERMINAL_INPUT, out: POWER_INTERNAL_TTM_TERMINAL_HANDLE_OUTPUT // requires SeShutdownPrivilege and terminalPowerManagement capability
    PowerInternalTtmCreateTerminal,                             // in: POWER_INTERNAL_TTM_CREATE_TERMINAL_INPUT, out: POWER_INTERNAL_TTM_CREATE_TERMINAL_OUTPUT // requires SeShutdownPrivilege and terminalPowerManagement capability
    PowerInternalTtmEvacuateDevices,                            // in: POWER_INTERNAL_TTM_EVACUATE_DEVICES_INPUT // requires SeShutdownPrivilege and terminalPowerManagement capability
    PowerInternalTtmCreateTerminalEventQueue,                   // in: POWER_INTERNAL_TTM_CREATE_EVENT_QUEUE_INPUT, out: POWER_INTERNAL_TTM_EVENT_QUEUE_HANDLE_OUTPUT // requires SeShutdownPrivilege and terminalPowerManagement capability
    PowerInternalTtmGetTerminalEvent,                           // in: POWER_INTERNAL_TTM_GET_TERMINAL_EVENT_INPUT, out: POWER_INTERNAL_TTM_TERMINAL_EVENT // requires SeShutdownPrivilege and terminalPowerManagement capability
    PowerInternalTtmSetDefaultDeviceAssignment,                 // in: POWER_INTERNAL_TTM_SET_DEFAULT_DEVICE_ASSIGNMENT_INPUT // requires SeShutdownPrivilege and terminalPowerManagement capability
    PowerInternalTtmAssignDevice,                               // in: POWER_INTERNAL_TTM_ASSIGN_DEVICE_INPUT // requires SeShutdownPrivilege and terminalPowerManagement capability
    PowerInternalTtmSetDisplayState,                            // in: POWER_INTERNAL_TTM_SET_DISPLAY_STATE_INPUT // requires SeShutdownPrivilege and terminalPowerManagement capability
    PowerInternalTtmSetDisplayTimeouts,                         // in: POWER_INTERNAL_TTM_SET_DISPLAY_TIMEOUTS_INPUT // requires SeShutdownPrivilege and terminalPowerManagement capability
    PowerInternalBootSessionStandbyActivationInformation,       // out: POWER_INTERNAL_BOOT_SESSION_STANDBY_ACTIVATION_INFO
    PowerInternalSessionPowerState,                             // in: POWER_SESSION_POWER_STATE
    PowerInternalSessionTerminalInput,                          // in: POWER_INTERNAL_TERMINAL_CORE_WINDOW_INPUT // 20
    PowerInternalSetWatchdog,                                   // in: POWER_INTERNAL_SET_WATCHDOG, out: (optional) HANDLE
    PowerInternalPhysicalPowerButtonPressInfoAtBoot,            // in: POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_INPUT, out: POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_OUTPUT
    PowerInternalExternalMonitorConnected,                      // in: POWER_INTERNAL_EXTERNAL_MONITOR_CONNECTED_INPUT
    PowerInternalHighPrecisionBrightnessSettings,               // in: POWER_INTERNAL_HIGH_PRECISION_BRIGHTNESS_SETTINGS_INPUT
    PowerInternalWinrtScreenToggle,                             // in: POWER_INTERNAL_WINRT_SCREEN_TOGGLE_INPUT
    PowerInternalPpmQosDisable,                                 // in: POWER_INTERNAL_PPM_QOS_DISABLE_INPUT (InputBufferLength >= 0x0C), out: NULL // PopPowerInformationInternal (inline): ref-counts PpmPerfQosDisableRefcount (enable increments; STATUS_INTEGER_OVERFLOW at UINT_MAX; disable decrements, STATUS_NOT_SUPPORTED if already 0); calls PpmPerfUpdateDomainPolicy on the 0<->1 edge
    PowerInternalTransitionCheckpoint,                          // in: POWER_INTERNAL_TRANSITION_CHECKPOINT_INPUT
    PowerInternalInputControllerState,                          // in: POWER_INTERNAL_INPUT_CONTROLLER_STATE
    PowerInternalFirmwareResetReason,                           // in: POWER_INTERNAL_FIRMWARE_RESET_REASON_INPUT, out: POWER_INTERNAL_FIRMWARE_RESET_REASON_OUTPUT
    PowerInternalPpmSchedulerQosSupport,                        // in: header only (InputBufferLength >= 8), out: POWER_INTERNAL_PROCESSOR_QOS_SUPPORT (3 bytes) // PopPowerInformationInternal (inline): returns PpmPerfQosSupportedAndConfigured, PpmPerfSchedulerDirectedPerfStatesSupported, PpmPerfQosGroupPolicyDisable // 30
    PowerInternalBootStatGet,                                   // in: POWER_INTERNAL_BOOTSTAT_GET_INPUT, out: (optional) POWER_INTERNAL_BOOTSTAT_GET_OUTPUT[EntryCount] or ULONG[EntryCount]
    PowerInternalBootStatSet,                                   // in: POWER_INTERNAL_BOOTSTAT_GET_INPUT
    PowerInternalCallHasNotReturnedWatchdog,                    // in: not implemented
    PowerInternalBootStatCheckIntegrity,                        // in: POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_INPUT, out: POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_OUTPUT
    PowerInternalBootStatRestoreDefaults,                       // in: void
    PowerInternalHostEsStateUpdate,                             // in: POWER_INTERNAL_HOST_ENERGY_SAVER_STATE
    PowerInternalGetPowerActionState,                           // out: ULONG
    PowerInternalBootStatUnlock,                                // in: POWER_INTERNAL_BOOTSTAT_GET_INPUT
    PowerInternalWakeOnVoiceState,                              // in: POWER_INTERNAL_WAKE_ON_VOICE_STATE_INPUT
    PowerInternalDeepSleepBlock,                                // in: POWER_INTERNAL_DEEP_SLEEP_BLOCK_INPUT // 40
    PowerInternalIsPoFxDevice,                                  // in: POWER_INTERNAL_IS_POFX_DEVICE_INPUT, out: BOOLEAN
    PowerInternalPowerTransitionExtensionAtBoot,                // out: POWER_INTERNAL_POWER_TRANSITION_EXTENSION_AT_BOOT_OUTPUT
    PowerInternalProcessorBrandedFrequency,                     // in: POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT, out: POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT
    PowerInternalTimeBrokerExpirationReason,                    // in: POWER_INTERNAL_TIME_BROKER_EXPIRATION_REASON_INPUT
    PowerInternalNotifyUserShutdownStatus,                      // in: POWER_INTERNAL_NOTIFY_USER_SHUTDOWN_STATUS_INPUT
    PowerInternalPowerRequestTerminalCoreWindow,                // in: POWER_INTERNAL_POWER_REQUEST_TERMINAL_CORE_WINDOW_INPUT
    PowerInternalProcessorIdleVeto,                             // out: PROCESSOR_IDLE_VETO
    PowerInternalPlatformIdleVeto,                              // out: PLATFORM_IDLE_VETO
    PowerInternalIsLongPowerButtonBugcheckEnabled,              // out: BOOLEAN
    PowerInternalAutoChkCausedReboot,                           // in: POWER_INTERNAL_AUTOCHK_CAUASED_REBOOT_INPUT, out: POWER_INTERNAL_AUTOCHK_CAUASED_REBOOT_OUTPUT // 50
    PowerInternalSetWakeAlarmOverride,                          // in: POWER_INTERNAL_SET_WAKE_ALARM_OVERRIDE_INPUT
    PowerInternalReservedInformation52,                         // in: not implemented
    PowerInternalDirectedFxAddTestDevice,                       // in: POWER_INTERNAL_DIRECTED_FX_ADD_TEST_DEVICE_INPUT
    PowerInternalDirectedFxRemoveTestDevice,                    // in: POWER_INTERNAL_DIRECTED_FX_REMOVE_TEST_DEVICE_INPUT
    PowerInternalReservedInformation55,                         // in: not implemented
    PowerInternalDirectedFxSetMode,                             // in: POWER_INTERNAL_DIRECTED_FX_SET_MODE_INPUT
    PowerInternalRegisterPowerPlane,                            // in: POWER_INTERNAL_REGISTER_POWER_PLANE_INPUT
    PowerInternalSetDirectedDripsFlags,                         // in: POWER_INTERNAL_DIRECTED_DRIPS_DEVICE_FLAGS_INPUT
    PowerInternalClearDirectedDripsFlags,                       // in: POWER_INTERNAL_DIRECTED_DRIPS_DEVICE_FLAGS_INPUT
    PowerInternalRetrieveHiberFileResumeContext,                // out: POWER_INTERNAL_RETRIEVE_HIBERFILE_RESUME_CONTEXT_OUTPUT // 60
    PowerInternalReadHiberFilePage,                             // in: POWER_INTERNAL_READ_HIBERFILE_PAGE_INPUT, out: POWER_INTERNAL_READ_HIBERFILE_PAGE_OUTPUT
    PowerInternalLastBootSucceeded,                             // out: BOOLEAN
    PowerInternalQuerySleepStudyHelperRoutineBlock,             // out: POWER_INTERNAL_QUERY_SLEEPSTUDY_HELPER_ROUTINE_BLOCK_OUTPUT
    PowerInternalDirectedDripsQueryCapabilities,                // out: POWER_INTERNAL_DIRECTED_DRIPS_QUERY_CAPABILITIES_OUTPUT
    PowerInternalClearConstraints,                              // in: POWER_INTERNAL_CLEAR_CONSTRAINTS_INPUT
    PowerInternalSoftParkVelocityEnabled,                       // in: not implemented
    PowerInternalQueryIntelPepCapabilities,                     // in: POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_INPUT, out: POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_OUTPUT
    PowerInternalGetSystemIdleLoopEnablement,                   // in: POWER_INTERNAL_SYSTEM_IDLE_LOOP_ENABLEMENT_INPUT, out: POWER_INTERNAL_SYSTEM_IDLE_LOOP_ENABLEMENT_OUTPUT // since WIN11
    PowerInternalGetVmPerfControlSupport,                       // in: POWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_INPUT, out: POWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_OUTPUT (0x14 bytes; only 1 byte if OutputBufferLength < 0x14) // PpmPerfGetVmPerfControlSupport
    PowerInternalGetVmPerfControlConfig,                        // in: POWER_INTERNAL_VM_PERF_CONTROL_CONFIG_INPUT (InputBufferLength >= 0x0C; Version <= 2, Version 2 requires >= 0x20), out: POWER_INTERNAL_VM_PERF_CONTROL_CONFIG_OUTPUT (8 bytes) // PpmPerfGetVmPerfConfig, or PpmPerfGetVmCppcConfig when Version == 2 // 70
    PowerInternalSleepDetailedDiagUpdate,                       // in: POWER_INTERNAL_SLEEP_DETAILED_DIAG_UPDATE_INPUT (InputBufferLength == 0x0C), out: NULL // PopPowerInformationInternal (inline): toggles PopSleepReliabilityDetailedDiagEnabled under PopSleepReliabilityDiagLock, traces PopDiagTraceSleepReliabilityDiagConfigUpdate on change
    PowerInternalProcessorClassFrequencyBandsStats,             // in: POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_INPUT, out: POWER_INTERNAL_PPM_PERF_FREQUENCY_BAND_STATS_OUT (fixed 0x900 bytes: Bank[2] x Metric[3] x Band[48]) // PpmPerfGetFrequencyBandStats: accumulates per-processor band counters into two efficiency-class banks
    PowerInternalHostGlobalUserPresenceStateUpdate,             // in: POWER_INTERNAL_HOST_GLOBAL_USER_PRESENCE_STATE_UPDATE_INPUT (InputBufferLength >= 0x0C), out: NULL // PopUserPresenceHostStateChange(UserPresent)
    PowerInternalCpuNodeIdleIntervalStats,                      // in: POWER_INTERNAL_IDLE_INTERVAL_STATS_INPUT (InputBufferLength == 0x0C; Node selects the CPU node), out: POWER_INTERNAL_IDLE_INTERVAL_PACKAGE (0x128 bytes) // PpmIdleGetPackageIdleIntervalStats
    PowerInternalClassIdleIntervalStats,                        // in: POWER_INTERNAL_IDLE_INTERVAL_STATS_INPUT (InputBufferLength == 0x0C), out: POWER_INTERNAL_IDLE_INTERVAL_STATS_OUTPUT (0x250 bytes) // PpmIdleGetPackageIdleIntervalStats
    PowerInternalCpuNodeConcurrencyStats,                       // in: POWER_INTERNAL_IDLE_INTERVAL_STATS_INPUT (InputBufferLength == 0x0C), out: POWER_INTERNAL_CPU_NODE_CONCURRENCY_STATS_OUTPUT (variable length, kernel-allocated) // PpmIdleGetConcurrencyStats
    PowerInternalClassConcurrencyStats,                         // in: POWER_INTERNAL_IDLE_INTERVAL_STATS_INPUT (InputBufferLength == 0x0C), out: POWER_INTERNAL_CLASS_CONCURRENCY_STATS_OUTPUT (variable length, kernel-allocated) // PpmIdleGetConcurrencyStats
    PowerInternalQueryProcMeasurementCapabilities,              // in: POWER_INTERNAL_QUERY_MEASUREMENT_CAPABILITIES (optional), out: POWER_INTERNAL_QUERY_MEASUREMENT_CAPABILITIES_OUTPUT (4 bytes) // PopPowerInformationInternal (inline): returns the first PpmPerfDomainHead domain's measurement-capability bitmask
    PowerInternalQueryProcMeasurementValues,                    // in: PROCESSOR_INTERNAL_QUERY_MEASUREMENT_VALUES (InputBufferLength == 0x0C), out: PROCESSOR_INTERNAL_QUERY_MEASUREMENT_VALUES_OUTPUT (8 + 0x18 * EntryCount bytes; OutputBufferLength == 4 returns EntryCount only) // PpmPerfQueryProcMeasurementValues
    PowerInternalPrepareForSystemInitiatedReboot,               // in: POWER_INTERNAL_PREPARE_FOR_SYSTEM_INITIATED_REBOOT_INPUT // 80
    PowerInternalGetAdaptiveSessionState,                       // in: POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_INPUT, out: POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_OUTPUT
    PowerInternalSetConsoleLockedState,                         // in: POWER_INTERNAL_SET_CONSOLE_LOCKED_STATE_INPUT
    PowerInternalOverrideSystemInitiatedRebootState,            // in: POWER_INTERNAL_OVERRIDE_SYSTEM_INITIATED_REBOOT_STATE_INPUT
    PowerInternalFanImpactStats,                                // in: POWER_INTERNAL_FAN_IMPACT_STATS_INPUT, out: POWER_INTERNAL_FAN_IMPACT_STATS_OUTPUT // PopFanReadFanNoiseInfo
    PowerInternalFanRpmBuckets,                                 // in: POWER_INTERNAL_FAN_RPM_BUCKETS_INPUT, out: POWER_INTERNAL_FAN_RPM_OUTPUT // PopFanReadFanNoiseInfo
    PowerInternalPowerBootAppDiagInfo,                          // out: POWER_INTERNAL_BOOTAPP_DIAGNOSTIC // PopPowerInformationInternal
    PowerInternalUnregisterShutdownNotification,                // in: POWER_INTERNAL_UNREGISTER_SHUTDOWN_NOTIFICATION_INPUT // since 22H1
    PowerInternalManageTransitionStateRecord,                   // in: POWER_INTERNAL_MANAGE_TRANSITION_STATE_RECORD_INPUT
    PowerInternalGetAcpiTimeAndAlarmCapabilities,               // in: POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_INPUT, out: POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_OUTPUT // since 22H2
    PowerInternalSuspendResumeRequest,                          // in: POWER_INTERNAL_SUSPEND_RESUME_REQUEST_INPUT // 90
    PowerInternalEnergyEstimationInfo,                          // out: POWER_INTERNAL_ENERGY_ESTIMATION_INFO_OUTPUT // since 23H2
    PowerInternalProvSocIdentifierOperation,                    // in: POWER_INTERNAL_SOC_IDENTIFIER_OPERATION_INPUT, out: POWER_INTERNAL_SOC_IDENTIFIER_OPERATION_OUTPUT // since 24H2
    PowerInternalGetVmPerfPrioritySupport,                      // in: POWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_INPUT, out: POWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_OUTPUT // PpmPerfGetVmPerfPrioritySupport
    PowerInternalGetVmPerfPriorityConfig,                       // in: POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_INPUT, out: POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_OUTPUT // PpmPerfGetVmPerfPriorityConfig
    PowerInternalNotifyWin32kPowerRequestQueued,                // in: POWER_INTERNAL_NOTIFY_WIN32K_POWER_REQUEST_INPUT
    PowerInternalNotifyWin32kPowerRequestCompleted,             // in: POWER_INTERNAL_NOTIFY_WIN32K_POWER_REQUEST_INPUT
    PowerInternalPdcAgentSessionQuery,                          // in: POWER_INTERNAL_PDC_AGENT_SESSION_QUERY_INPUT, out: BOOLEAN // (feature-gated)
    PowerInternalSessionConnectionChangeV2,                     // in: POWER_INTERNAL_SESSION_CONNECTION_CHANGE_V2_INPUT, out: POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_OUTPUT // (feature-gated)
    PowerInformationInternalMaximum
} POWER_INFORMATION_LEVEL_INTERNAL;

/**
 * The POWER_S0_DISCONNECTED_REASON enumeration defines power s0 disconnected reason values.
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
 * The CsDeviceCompliance structure describes power s0 low power idle info.
 */
typedef struct _POWER_S0_LOW_POWER_IDLE_INFO
{
    POWER_S0_DISCONNECTED_REASON DisconnectedReason;
    union
    {
        BOOLEAN Storage : 1;
        BOOLEAN WiFi : 1;
        BOOLEAN Mbn : 1;
        BOOLEAN Ethernet : 1;
        BOOLEAN Reserved : 4;
        UCHAR AsUCHAR;
    } CsDeviceCompliance;
    union
    {
        BOOLEAN DisconnectInStandby : 1;
        BOOLEAN EnforceDs : 1;
        BOOLEAN Reserved : 6;
        UCHAR AsUCHAR;
    } Policy;
} POWER_S0_LOW_POWER_IDLE_INFO, *PPOWER_S0_LOW_POWER_IDLE_INFO;

/**
 * The POWER_INFORMATION_INTERNAL_HEADER structure describes power information internal header.
 */
typedef struct _POWER_INFORMATION_INTERNAL_HEADER
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INFORMATION_INTERNAL_HEADER, *PPOWER_INFORMATION_INTERNAL_HEADER;

// rev
/**
 * The POWER_INTERNAL_ACPI_INTERFACE_REGISTER_INPUT structure contains the input parameters for the acpi interface register operation.
 */
typedef struct _POWER_INTERNAL_ACPI_INTERFACE_REGISTER_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    UCHAR Data[24];
} POWER_INTERNAL_ACPI_INTERFACE_REGISTER_INPUT, *PPOWER_INTERNAL_ACPI_INTERFACE_REGISTER_INPUT;

C_ASSERT(sizeof(POWER_INTERNAL_ACPI_INTERFACE_REGISTER_INPUT) == 0x20);

// rev
/**
 * The POWER_INTERNAL_ACPI_INTERFACE_REGISTER_OUTPUT structure contains the output data returned by the acpi interface register operation.
 */
typedef struct _POWER_INTERNAL_ACPI_INTERFACE_REGISTER_OUTPUT
{
    ULONG_PTR RegistrationHandle;
    ULONG_PTR Reserved;
} POWER_INTERNAL_ACPI_INTERFACE_REGISTER_OUTPUT, *PPOWER_INTERNAL_ACPI_INTERFACE_REGISTER_OUTPUT;

/**
 * The POWER_USER_ABSENCE_PREDICTION structure describes power user absence prediction.
 */
typedef struct _POWER_USER_ABSENCE_PREDICTION
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    LARGE_INTEGER ReturnTime;
} POWER_USER_ABSENCE_PREDICTION, *PPOWER_USER_ABSENCE_PREDICTION;

/**
 * The POWER_USER_ABSENCE_PREDICTION_CAPABILITY structure describes power user absence prediction capability.
 */
typedef struct _POWER_USER_ABSENCE_PREDICTION_CAPABILITY
{
    BOOLEAN AbsencePredictionCapability;
} POWER_USER_ABSENCE_PREDICTION_CAPABILITY, *PPOWER_USER_ABSENCE_PREDICTION_CAPABILITY;

// rev
/**
 * The POWER_PROCESSOR_LATENCY_HINT structure is the input for PowerInternalPoProcessorLatencyHint;
 * its Type field is forwarded to PoLatencySensitivityHint to set the processor latency-sensitivity hint.
 */
typedef struct _POWER_PROCESSOR_LATENCY_HINT
{
    POWER_INFORMATION_INTERNAL_HEADER PowerInformationInternalHeader;
    ULONG Type;
} POWER_PROCESSOR_LATENCY_HINT, *PPOWER_PROCESSOR_LATENCY_HINT;

// rev
/**
 * The POWER_STANDBY_NETWORK_REQUEST structure describes power standby network request.
 */
typedef struct _POWER_STANDBY_NETWORK_REQUEST
{
    POWER_INFORMATION_INTERNAL_HEADER PowerInformationInternalHeader;
    BOOLEAN Active;
} POWER_STANDBY_NETWORK_REQUEST, *PPOWER_STANDBY_NETWORK_REQUEST;

// rev
/**
 * The POWER_SET_BACKGROUND_TASK_STATE structure describes power set background task state.
 */
typedef struct _POWER_SET_BACKGROUND_TASK_STATE
{
    POWER_INFORMATION_INTERNAL_HEADER PowerInformationInternalHeader;
    BOOLEAN Engaged;
} POWER_SET_BACKGROUND_TASK_STATE, *PPOWER_SET_BACKGROUND_TASK_STATE;

// rev
/**
 * The POWER_INTERNAL_BOOT_SESSION_STANDBY_ACTIVATION_INFO structure describes power internal boot session standby activation info.
 */
typedef struct _POWER_INTERNAL_BOOT_SESSION_STANDBY_ACTIVATION_INFO
{
    ULONG StandbyTotalTime;
    ULONG DripsTotalTime;
    ULONG ActivatorClientTotalActiveTime;
    ULONG PerActivatorClientTotalActiveTime[98];
} POWER_INTERNAL_BOOT_SESSION_STANDBY_ACTIVATION_INFO, *PPOWER_INTERNAL_BOOT_SESSION_STANDBY_ACTIVATION_INFO;

// rev
/**
 * The POWER_SESSION_POWER_STATE structure describes power session power state.
 */
typedef struct _POWER_SESSION_POWER_STATE
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONG SessionId;
    BOOLEAN On;
    BOOLEAN IsConsole;
    POWER_MONITOR_REQUEST_REASON RequestReason;
} POWER_SESSION_POWER_STATE, *PPOWER_SESSION_POWER_STATE;

// rev
/**
 * The POWER_INTERNAL_SET_WATCHDOG structure describes power internal set watchdog.
 */
typedef struct _POWER_INTERNAL_SET_WATCHDOG
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    PVOID WatchdogHandle;
    UCHAR Parameters[0x48];
    BOOLEAN DeleteWatchdog;
    UCHAR Reserved[7];
} POWER_INTERNAL_SET_WATCHDOG, *PPOWER_INTERNAL_SET_WATCHDOG;

//C_ASSERT(sizeof(POWER_INTERNAL_SET_WATCHDOG) == 0x60);

// rev
/**
 * The POWER_INTERNAL_TERMINAL_CORE_WINDOW_INPUT structure contains the input parameters for the terminal core window operation.
 */
typedef struct _POWER_INTERNAL_TERMINAL_CORE_WINDOW_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG SessionId;
    ULONG TerminalId;
    UCHAR InputType;
} POWER_INTERNAL_TERMINAL_CORE_WINDOW_INPUT, *PPOWER_INTERNAL_TERMINAL_CORE_WINDOW_INPUT;

// rev
/**
 * The POWER_INTERNAL_TTM_OPEN_TERMINAL_INPUT structure contains the input parameters for the ttm open terminal operation.
 */
typedef struct _POWER_INTERNAL_TTM_OPEN_TERMINAL_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ACCESS_MASK DesiredAccess;
} POWER_INTERNAL_TTM_OPEN_TERMINAL_INPUT, *PPOWER_INTERNAL_TTM_OPEN_TERMINAL_INPUT;

// rev
/**
 * The POWER_INTERNAL_TTM_TERMINAL_HANDLE_OUTPUT structure contains the output data returned by the ttm terminal handle operation.
 */
typedef struct _POWER_INTERNAL_TTM_TERMINAL_HANDLE_OUTPUT
{
    HANDLE TerminalHandle;
} POWER_INTERNAL_TTM_TERMINAL_HANDLE_OUTPUT, *PPOWER_INTERNAL_TTM_TERMINAL_HANDLE_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_TTM_CREATE_TERMINAL_INPUT structure contains the input parameters for the ttm create terminal operation.
 */
typedef struct _POWER_INTERNAL_TTM_CREATE_TERMINAL_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONG CreateFlags;
    ULONG Reserved;
    HANDLE ParentTerminalHandle;
} POWER_INTERNAL_TTM_CREATE_TERMINAL_INPUT, *PPOWER_INTERNAL_TTM_CREATE_TERMINAL_INPUT;

// rev
/**
 * The POWER_INTERNAL_TTM_CREATE_TERMINAL_OUTPUT structure contains the output data returned by the ttm create terminal operation.
 */
typedef struct _POWER_INTERNAL_TTM_CREATE_TERMINAL_OUTPUT
{
    HANDLE TerminalHandle;
    ULONG TerminalId;
    ULONG Reserved;
} POWER_INTERNAL_TTM_CREATE_TERMINAL_OUTPUT, *PPOWER_INTERNAL_TTM_CREATE_TERMINAL_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_TTM_EVACUATE_DEVICES_INPUT structure contains the input parameters for the ttm evacuate devices operation.
 */
typedef struct _POWER_INTERNAL_TTM_EVACUATE_DEVICES_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    HANDLE TerminalHandle;
} POWER_INTERNAL_TTM_EVACUATE_DEVICES_INPUT, *PPOWER_INTERNAL_TTM_EVACUATE_DEVICES_INPUT;

// rev
/**
 * The POWER_INTERNAL_TTM_CREATE_EVENT_QUEUE_INPUT structure contains the input parameters for the ttm create event queue operation.
 */
typedef struct _POWER_INTERNAL_TTM_CREATE_EVENT_QUEUE_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    HANDLE TerminalHandle;
} POWER_INTERNAL_TTM_CREATE_EVENT_QUEUE_INPUT, *PPOWER_INTERNAL_TTM_CREATE_EVENT_QUEUE_INPUT;

// rev
/**
 * The POWER_INTERNAL_TTM_EVENT_QUEUE_HANDLE_OUTPUT structure contains the output data returned by the ttm event queue handle operation.
 */
typedef struct _POWER_INTERNAL_TTM_EVENT_QUEUE_HANDLE_OUTPUT
{
    HANDLE EventQueueHandle;
} POWER_INTERNAL_TTM_EVENT_QUEUE_HANDLE_OUTPUT, *PPOWER_INTERNAL_TTM_EVENT_QUEUE_HANDLE_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_TTM_GET_TERMINAL_EVENT_INPUT structure contains the input parameters for the ttm get terminal event operation.
 */
typedef struct _POWER_INTERNAL_TTM_GET_TERMINAL_EVENT_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    HANDLE EventQueueHandle;
} POWER_INTERNAL_TTM_GET_TERMINAL_EVENT_INPUT, *PPOWER_INTERNAL_TTM_GET_TERMINAL_EVENT_INPUT;

#define POWER_INTERNAL_TTM_TERMINAL_EVENT_TYPE_DEVICE_ENUMERATED 0
#define POWER_INTERNAL_TTM_TERMINAL_EVENT_TYPE_ENUMERATION_COMPLETE 1
#define POWER_INTERNAL_TTM_TERMINAL_EVENT_TYPE_DEVICE_ARRIVED 3
#define POWER_INTERNAL_TTM_TERMINAL_EVENT_TYPE_DEVICE_ASSIGNED 4
#define POWER_INTERNAL_TTM_TERMINAL_EVENT_TYPE_DEVICE_DEPARTED 5
#define POWER_INTERNAL_TTM_TERMINAL_EVENT_TYPE_DISPLAY_REQUIRED_POWER_REQUEST_UPDATED 6

// rev
/**
 * The POWER_INTERNAL_TTM_TERMINAL_EVENT structure describes power internal ttm terminal event.
 */
typedef struct _POWER_INTERNAL_TTM_TERMINAL_EVENT
{
    ULONG EventType;
    ULONG Reserved;
    UCHAR Payload[0x218];
} POWER_INTERNAL_TTM_TERMINAL_EVENT, *PPOWER_INTERNAL_TTM_TERMINAL_EVENT;

// rev
/**
 * The POWER_INTERNAL_TTM_SET_DEFAULT_DEVICE_ASSIGNMENT_INPUT structure contains the input parameters for the ttm set default device assignment operation.
 */
typedef struct _POWER_INTERNAL_TTM_SET_DEFAULT_DEVICE_ASSIGNMENT_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    HANDLE TerminalHandle;
    BOOLEAN DefaultAssignmentEnabled;
    UCHAR Reserved[7];
} POWER_INTERNAL_TTM_SET_DEFAULT_DEVICE_ASSIGNMENT_INPUT, *PPOWER_INTERNAL_TTM_SET_DEFAULT_DEVICE_ASSIGNMENT_INPUT;

// rev
/**
 * The POWER_INTERNAL_TTM_ASSIGN_DEVICE_INPUT structure contains the input parameters for the ttm assign device operation.
 */
typedef struct _POWER_INTERNAL_TTM_ASSIGN_DEVICE_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    HANDLE TerminalHandle;
    ULONG DeviceId;
    ULONG Reserved;
} POWER_INTERNAL_TTM_ASSIGN_DEVICE_INPUT, *PPOWER_INTERNAL_TTM_ASSIGN_DEVICE_INPUT;

// rev
/**
 * The POWER_INTERNAL_TTM_SET_DISPLAY_STATE_INPUT structure contains the input parameters for the ttm set display state operation.
 */
typedef struct _POWER_INTERNAL_TTM_SET_DISPLAY_STATE_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    HANDLE TerminalHandle;
    BOOLEAN DisplayOn;
    UCHAR Reserved[3];
    ULONG RequestReason;
} POWER_INTERNAL_TTM_SET_DISPLAY_STATE_INPUT, *PPOWER_INTERNAL_TTM_SET_DISPLAY_STATE_INPUT;

// rev
/**
 * The POWER_INTERNAL_TTM_SET_DISPLAY_TIMEOUTS_INPUT structure contains the input parameters for the ttm set display timeouts operation.
 */
typedef struct _POWER_INTERNAL_TTM_SET_DISPLAY_TIMEOUTS_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    HANDLE TerminalHandle;
    ULONG DimTimeoutSeconds;
    ULONG OffTimeoutSeconds;
} POWER_INTERNAL_TTM_SET_DISPLAY_TIMEOUTS_INPUT, *PPOWER_INTERNAL_TTM_SET_DISPLAY_TIMEOUTS_INPUT;

// rev
/**
 * The POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_INPUT structure contains the input parameters for the physical power button at boot operation.
 */
typedef struct _POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_INPUT, *PPOWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_INPUT;

// rev
/**
 * The POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_OUTPUT structure contains the output data returned by the physical power button at boot operation.
 */
typedef struct _POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_OUTPUT
{
    UCHAR Buffer[64];
} POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_OUTPUT, *PPOWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_EXTERNAL_MONITOR_CONNECTED_INPUT structure contains the input parameters for the external monitor connected operation.
 */
typedef struct _POWER_INTERNAL_EXTERNAL_MONITOR_CONNECTED_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    BOOLEAN Connected; // 1 = connected, 0 = disconnected
} POWER_INTERNAL_EXTERNAL_MONITOR_CONNECTED_INPUT, *PPOWER_INTERNAL_EXTERNAL_MONITOR_CONNECTED_INPUT;

// rev
/**
 * The POWER_INTERNAL_HIGH_PRECISION_BRIGHTNESS_SETTINGS_INPUT structure contains the input parameters for the high precision brightness settings operation.
 */
typedef struct _POWER_INTERNAL_HIGH_PRECISION_BRIGHTNESS_SETTINGS_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG SessionId;
    ULONG BrightnessLevel;
    ULONG Flags;
    ULONG Reserved[5];
} POWER_INTERNAL_HIGH_PRECISION_BRIGHTNESS_SETTINGS_INPUT, *PPOWER_INTERNAL_HIGH_PRECISION_BRIGHTNESS_SETTINGS_INPUT;

// rev
/**
 * The POWER_INTERNAL_WINRT_SCREEN_TOGGLE_INPUT structure contains the input parameters for the winrt screen toggle operation.
 */
typedef struct _POWER_INTERNAL_WINRT_SCREEN_TOGGLE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    BOOLEAN Toggle; // 1 = turn screen on, 0 = turn screen off
} POWER_INTERNAL_WINRT_SCREEN_TOGGLE_INPUT, *PPOWER_INTERNAL_WINRT_SCREEN_TOGGLE_INPUT;

// rev
/**
 * The POWER_INTERNAL_PPM_QOS_DISABLE_INPUT structure contains the input parameters for the ppm qos disable operation.
 */
typedef struct _POWER_INTERNAL_PPM_QOS_DISABLE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    BOOLEAN EnableDisable; // Non-zero to enable QoS disable, zero to disable
} POWER_INTERNAL_PPM_QOS_DISABLE_INPUT, *PPOWER_INTERNAL_PPM_QOS_DISABLE_INPUT;

// rev
/**
 * The POWER_INTERNAL_TRANSITION_CHECKPOINT_INPUT structure contains the input parameters for the transition checkpoint operation.
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
 * The POWER_INTERNAL_INPUT_CONTROLLER_STATE structure describes power internal input controller state.
 */
typedef struct _POWER_INTERNAL_INPUT_CONTROLLER_STATE
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONG InputControllerState;
} POWER_INTERNAL_INPUT_CONTROLLER_STATE, *PPOWER_INTERNAL_INPUT_CONTROLLER_STATE;

// rev
/**
 * The POWER_INTERNAL_FIRMWARE_RESET_REASON_INPUT structure contains the input parameters for the firmware reset reason operation.
 */
typedef struct _POWER_INTERNAL_FIRMWARE_RESET_REASON_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_FIRMWARE_RESET_REASON_INPUT, *PPOWER_INTERNAL_FIRMWARE_RESET_REASON_INPUT;

// rev
/**
 * The POWER_INTERNAL_FIRMWARE_RESET_REASON_OUTPUT structure contains the output data returned by the firmware reset reason operation.
 */
typedef struct _POWER_INTERNAL_FIRMWARE_RESET_REASON_OUTPUT
{
    ULONG ResetReasonCode;
    UCHAR DiagnosticData1[16];
    UCHAR DiagnosticData2[16];
    UCHAR Reserved[12];
} POWER_INTERNAL_FIRMWARE_RESET_REASON_OUTPUT, *PPOWER_INTERNAL_FIRMWARE_RESET_REASON_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_PROCESSOR_QOS_SUPPORT structure describes power internal processor qos support.
 */
typedef struct _POWER_INTERNAL_PROCESSOR_QOS_SUPPORT
{
    BOOLEAN QosSupportedAndConfigured;
    BOOLEAN SchedulerDirectedPerfStatesSupported;
    BOOLEAN QosGroupPolicyDisable;
} POWER_INTERNAL_PROCESSOR_QOS_SUPPORT, *PPOWER_INTERNAL_PROCESSOR_QOS_SUPPORT;

typedef struct _RTL_BSD_ITEM RTL_BSD_ITEM, *PRTL_BSD_ITEM;

// rev
/**
 * The POWER_INTERNAL_BOOTSTAT_GET_INPUT structure contains the input parameters for the bootstat get operation.
 */
typedef struct _POWER_INTERNAL_BOOTSTAT_GET_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG EntryCount;
    ULONG Reserved;
    PRTL_BSD_ITEM Entries;
} POWER_INTERNAL_BOOTSTAT_GET_INPUT, *PPOWER_INTERNAL_BOOTSTAT_GET_INPUT;

// rev
/**
 * The POWER_INTERNAL_BOOTSTAT_GET_OUTPUT structure contains the output data returned by the bootstat get operation.
 */
typedef struct _POWER_INTERNAL_BOOTSTAT_GET_OUTPUT
{
    // If present, it receives the actual sizes of the data copied into each DataBuffer.
    ULONG Sizes[ANYSIZE_ARRAY]; // Array of sizes, one per entry
} POWER_INTERNAL_BOOTSTAT_GET_OUTPUT, *PPOWER_INTERNAL_BOOTSTAT_GET_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_INPUT structure contains the input parameters for the bootstat check integrity operation.
 */
typedef struct _POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG EntryCount;
    ULONG Reserved;
    PRTL_BSD_ITEM Entries;
} POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_INPUT, *PPOWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_INPUT;

C_ASSERT(sizeof(POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_INPUT) == sizeof(POWER_INTERNAL_BOOTSTAT_GET_INPUT));

// rev
/**
 * The POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_OUTPUT structure contains the output data returned by the bootstat check integrity operation.
 */
typedef struct _POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_OUTPUT
{
    BOOLEAN IntegrityOk;
} POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_OUTPUT, *PPOWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_HOST_ENERGY_SAVER_STATE structure describes power internal host energy saver state.
 */
typedef struct _POWER_INTERNAL_HOST_ENERGY_SAVER_STATE
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    BOOLEAN EsEnabledOnHost;
} POWER_INTERNAL_HOST_ENERGY_SAVER_STATE, *PPOWER_INTERNAL_HOST_ENERGY_SAVER_STATE;

// rev
/**
 * The POWER_INTERNAL_IS_POFX_DEVICE_INPUT structure contains the input parameters for the is pofx device operation.
 */
typedef struct _POWER_INTERNAL_IS_POFX_DEVICE_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    PVOID DeviceObject;
} POWER_INTERNAL_IS_POFX_DEVICE_INPUT, *PPOWER_INTERNAL_IS_POFX_DEVICE_INPUT;

// rev
/**
 * The POWER_INTERNAL_POWER_TRANSITION_EXTENSION_AT_BOOT_OUTPUT structure contains the output data returned by the power transition extension at boot operation.
 */
typedef struct _POWER_INTERNAL_POWER_TRANSITION_EXTENSION_AT_BOOT_OUTPUT
{
    UCHAR Data[32];
} POWER_INTERNAL_POWER_TRANSITION_EXTENSION_AT_BOOT_OUTPUT, *PPOWER_INTERNAL_POWER_TRANSITION_EXTENSION_AT_BOOT_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_NOTIFY_USER_SHUTDOWN_STATUS_INPUT structure contains the input parameters for the notify user shutdown status operation.
 */
typedef struct _POWER_INTERNAL_NOTIFY_USER_SHUTDOWN_STATUS_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    BOOLEAN ShutdownInitiated; //  1 = initiated, 0 = cancelled
} POWER_INTERNAL_NOTIFY_USER_SHUTDOWN_STATUS_INPUT, *PPOWER_INTERNAL_NOTIFY_USER_SHUTDOWN_STATUS_INPUT;

// rev
/**
 * The POWER_INTERNAL_AUTOCHK_CAUASED_REBOOT_INPUT structure contains the input parameters for the autochk cauased reboot operation.
 */
typedef struct _POWER_INTERNAL_AUTOCHK_CAUASED_REBOOT_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_AUTOCHK_CAUASED_REBOOT_INPUT, *PPOWER_INTERNAL_AUTOCHK_CAUASED_REBOOT_INPUT;

// rev
/**
 * The POWER_INTERNAL_READ_HIBERFILE_PAGE_INPUT structure contains the input parameters for the read hiberfile page operation.
 */
typedef struct _POWER_INTERNAL_READ_HIBERFILE_PAGE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG PageNumber;
} POWER_INTERNAL_READ_HIBERFILE_PAGE_INPUT, *PPOWER_INTERNAL_READ_HIBERFILE_PAGE_INPUT;

// rev
/**
 * The POWER_INTERNAL_READ_HIBERFILE_PAGE_OUTPUT structure contains the output data returned by the read hiberfile page operation.
 */
typedef struct _POWER_INTERNAL_READ_HIBERFILE_PAGE_OUTPUT
{
    UCHAR PageData[PAGE_SIZE];
} POWER_INTERNAL_READ_HIBERFILE_PAGE_OUTPUT, *PPOWER_INTERNAL_READ_HIBERFILE_PAGE_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_INPUT structure contains the input parameters for the query intel pep capabilities operation.
 */
typedef struct _POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_INPUT, *PPOWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_INPUT;

// rev
/**
 * The POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_OUTPUT structure contains the output data returned by the query intel pep capabilities operation.
 */
typedef struct _POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_OUTPUT
{
    ULONG Capabilities[4];
} POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_OUTPUT, *PPOWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_AUTOCHK_CAUASED_REBOOT_OUTPUT structure contains the output data returned by the autochk cauased reboot operation.
 */
typedef struct _POWER_INTERNAL_AUTOCHK_CAUASED_REBOOT_OUTPUT
{
    BOOLEAN CausedReboot;
} POWER_INTERNAL_AUTOCHK_CAUASED_REBOOT_OUTPUT, *PPOWER_INTERNAL_AUTOCHK_CAUASED_REBOOT_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_TIME_BROKER_EXPIRATION_REASON_INPUT structure contains the input parameters for the time broker expiration reason operation.
 */
typedef struct _POWER_INTERNAL_TIME_BROKER_EXPIRATION_REASON_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    WCHAR Reason[64];
    ULONGLONG DueTime;
} POWER_INTERNAL_TIME_BROKER_EXPIRATION_REASON_INPUT, *PPOWER_INTERNAL_TIME_BROKER_EXPIRATION_REASON_INPUT;

C_ASSERT(sizeof(POWER_INTERNAL_TIME_BROKER_EXPIRATION_REASON_INPUT) == 0x90);

// rev
/**
 * The POWER_INTERNAL_POWER_REQUEST_TERMINAL_CORE_WINDOW_INPUT structure contains the input parameters for the power request terminal core window operation.
 */
typedef struct _POWER_INTERNAL_POWER_REQUEST_TERMINAL_CORE_WINDOW_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    PVOID ProcessHandle;
    PVOID PowerRequestPointer;
    ULONG ReasonCode;
    ULONG Reserved;
} POWER_INTERNAL_POWER_REQUEST_TERMINAL_CORE_WINDOW_INPUT, *PPOWER_INTERNAL_POWER_REQUEST_TERMINAL_CORE_WINDOW_INPUT;

// rev
/**
 * The POWER_INTERNAL_WAKE_ON_VOICE_STATE_INPUT structure contains the input parameters for the wake on voice state operation.
 */
typedef struct _POWER_INTERNAL_WAKE_ON_VOICE_STATE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    BOOLEAN Enabled; // 1 = enable Wake on Voice, 0 = disable
} POWER_INTERNAL_WAKE_ON_VOICE_STATE_INPUT, *PPOWER_INTERNAL_WAKE_ON_VOICE_STATE_INPUT;

// rev
/**
 * The POWER_INTERNAL_DEEP_SLEEP_BLOCK_INPUT structure contains the input parameters for the deep sleep block operation.
 */
typedef struct _POWER_INTERNAL_DEEP_SLEEP_BLOCK_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    BOOLEAN Block; // 1 = block deep sleep, 0 = unblock
} POWER_INTERNAL_DEEP_SLEEP_BLOCK_INPUT, *PPOWER_INTERNAL_DEEP_SLEEP_BLOCK_INPUT;

// rev
/**
 * The POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT structure contains the input parameters for the processor branded frequency operation.
 */
typedef struct _POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    PROCESSOR_NUMBER ProcessorNumber; // Optional: provide only when InputBufferLength == sizeof(POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT) (0x0C). Reserved must be 0.
} POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT, *PPOWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT;

#define POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_VERSION 1

C_ASSERT(sizeof(POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT) == 0x0C);

// rev
/**
 * The POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT structure contains the output data returned by the processor branded frequency operation.
 */
typedef struct _POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT
{
    ULONG Version; // POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_VERSION
    ULONG NominalFrequency; // if (Domain) Prcb->PowerState.CheckContext.Domain.NominalFrequency else Prcb->MHz
} POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT, *PPOWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT;

C_ASSERT(sizeof(POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT) == 0x08);

// rev
/**
 * The POWER_INTERNAL_SET_WAKE_ALARM_OVERRIDE_INPUT structure contains the input parameters for the set wake alarm override operation.
 */
typedef struct _POWER_INTERNAL_SET_WAKE_ALARM_OVERRIDE_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONGLONG WakeAlarmOverrideAc;
    ULONGLONG WakeAlarmOverrideDc;
} POWER_INTERNAL_SET_WAKE_ALARM_OVERRIDE_INPUT, *PPOWER_INTERNAL_SET_WAKE_ALARM_OVERRIDE_INPUT;

// rev
/**
 * The PROCESSOR_IDLE_VETO structure describes processor idle veto.
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
 * The PLATFORM_IDLE_VETO structure describes platform idle veto.
 */
typedef struct _PLATFORM_IDLE_VETO
{
    ULONG Version;
    ULONG StateIndex;
    ULONG VetoReason;
    UCHAR Increment;
} PLATFORM_IDLE_VETO, *PPLATFORM_IDLE_VETO;

// rev
/**
 * The POWER_INTERNAL_SYSTEM_IDLE_LOOP_ENABLEMENT_INPUT structure contains the input parameters for the system idle loop enablement operation.
 */
typedef struct _POWER_INTERNAL_SYSTEM_IDLE_LOOP_ENABLEMENT_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_SYSTEM_IDLE_LOOP_ENABLEMENT_INPUT, *PPOWER_INTERNAL_SYSTEM_IDLE_LOOP_ENABLEMENT_INPUT;

// rev
/**
 * The POWER_INTERNAL_SYSTEM_IDLE_LOOP_ENABLEMENT_OUTPUT structure contains the output data returned by the system idle loop enablement operation.
 */
typedef struct _POWER_INTERNAL_SYSTEM_IDLE_LOOP_ENABLEMENT_OUTPUT
{
    ULONG IdleLoopEnabled;
} POWER_INTERNAL_SYSTEM_IDLE_LOOP_ENABLEMENT_OUTPUT, *PPOWER_INTERNAL_SYSTEM_IDLE_LOOP_ENABLEMENT_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_INPUT structure contains the input parameters for the vm perf control support operation.
 */
typedef struct _POWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG Reserved1;
} POWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_INPUT, *PPOWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_INPUT;

// rev
#define PPM_VMPCS_SUPPORTS_PERF_SET        0x00000001 // Can set explicit performance levels
#define PPM_VMPCS_SUPPORTS_AUTONOMOUS      0x00000002 // Supports autonomous (hardware-managed) mode
#define PPM_VMPCS_SUPPORTS_EPP             0x00000004 // Supports Energy Performance Preference (EPP)
#define PPM_VMPCS_SUPPORTS_BOOST           0x00000008 // Supports boost performance modes
#define PPM_VMPCS_SUPPORTS_TIME_WINDOW     0x00000010 // Supports time-window based control

// rev
/**
 * The POWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_OUTPUT structure contains the output data returned by the vm perf control support operation.
 */
typedef struct _POWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_OUTPUT
{
    // If OutputBuffer only 1 byte, just this flag returned for "VM perf-control supported".
    UCHAR Supported;
    // Reserved values (returned when OutputBuffer > 1 bytes).
    UCHAR Reserved0;
    UCHAR Reserved1;
    UCHAR Reserved2;
    // Extended details (returned when OutputBuffer >= 20 bytes).
    ULONG MinPerfPercent; // Minimum performance percentage (0..100)
    ULONG MaxPerfPercent; // Maximum performance percentage (0..100)
    ULONG StepPerfPercent; // Step size for performance percentage (>=1)
    ULONG Capabilities; // Bitmask of PPM_VMPCS_SUPPORTS_* flags
} POWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_OUTPUT, *PPOWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_SLEEP_DETAILED_DIAG_UPDATE_INPUT structure contains the input parameters for the sleep detailed diag update operation.
 */
typedef struct _POWER_INTERNAL_SLEEP_DETAILED_DIAG_UPDATE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    BOOLEAN Enable;
} POWER_INTERNAL_SLEEP_DETAILED_DIAG_UPDATE_INPUT, *PPOWER_INTERNAL_SLEEP_DETAILED_DIAG_UPDATE_INPUT;

// rev
/**
 * The POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_INPUT structure contains the input parameters for the processor class band stats operation.
 */
typedef struct _POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_INPUT, *PPOWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_INPUT;

// rev
/**
 * The POWER_INTERNAL_HOST_GLOBAL_USER_PRESENCE_STATE_UPDATE_INPUT structure contains the input parameters for the host global user presence state update operation.
 */
typedef struct _POWER_INTERNAL_HOST_GLOBAL_USER_PRESENCE_STATE_UPDATE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    BOOLEAN UserPresent; // 1 if user is present, 0 otherwise
} POWER_INTERNAL_HOST_GLOBAL_USER_PRESENCE_STATE_UPDATE_INPUT, *PPOWER_INTERNAL_HOST_GLOBAL_USER_PRESENCE_STATE_UPDATE_INPUT;

// rev
/**
 * The POWER_INTERNAL_IDLE_INTERVAL_STATS_INPUT structure is passed to internal power management routines
 * to request idle interval statistics for a given processor package or node.
 */
typedef struct _POWER_INTERNAL_IDLE_INTERVAL_STATS_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG Node;
} POWER_INTERNAL_IDLE_INTERVAL_STATS_INPUT, *PPOWER_INTERNAL_IDLE_INTERVAL_STATS_INPUT;

// rev
/**
 * The POWER_INTERNAL_IDLE_INTERVAL_PACKAGE structure contains a histogram of idle intervals,
 * each entry representing the total time spent in a given duration bucket.
 * The 37 buckets are logarithmically spaced to capture idle durations from ms up to seconds.
 * Approximate bucket ranges:
 * - Indices [0-2] = Very long idle (10-50 seconds)
 * - Indices [3-5] = Long idle (2-10 seconds)
 * - Indices [6-10] = Medium idle (100 ms-1 second)
 * - Indices [11-15] = Short idle (10-100 ms)
 * - Indices [16-36] = Very short idle (<10 ms, down to microseconds)
 */
typedef struct _POWER_INTERNAL_IDLE_INTERVAL_PACKAGE
{
    /**
     * Idle interval histogram buckets.
     * Each entry is a ULONGLONG value in 100-nanosecond units.
     * There are 37 buckets, covering idle durations from microseconds
     * up to tens of seconds.
     */
    ULONGLONG IdleIntervals[37];
} POWER_INTERNAL_IDLE_INTERVAL_PACKAGE, *PPOWER_INTERNAL_IDLE_INTERVAL_PACKAGE;

// rev
/**
 * The POWER_INTERNAL_IDLE_INTERVAL_STATS_OUTPUT structure contains the idle interval statistics.
 */
typedef struct _POWER_INTERNAL_IDLE_INTERVAL_STATS_OUTPUT
{
    POWER_INTERNAL_IDLE_INTERVAL_PACKAGE Package[2];
} POWER_INTERNAL_IDLE_INTERVAL_STATS_OUTPUT, *PPOWER_INTERNAL_IDLE_INTERVAL_STATS_OUTPUT;

// rev
#define PPM_PERF_BANKS_COUNT 2
#define PPM_PERF_BANDS_COUNT 48
#define PPM_PERF_METRICS_COUNT 3
#define PPM_PERF_BANDS_SIZE sizeof(PPM_PERF_BAND_ENTRY)
#define PPM_PERF_STATS_SIZE (PPM_PERF_BANDS_COUNT * PPM_PERF_BANDS_SIZE)
#define PPM_PERF_DELTA_OFFSET 0xF8 // 248 bytes

// rev
/**
 * The PPM_WMI_PERFSTATES_DATA structure is the fixed 0x50-byte payload returned for
 * PPM_PERFSTATES_DATA_GUID by PpmWmiGetAllData.
 *
 * Only a subset of fields could be named confidently from the kernel path:
 * - StateCount at offset 0x04
 * - PercentFrequency at offset 0x1C
 * - Type at offset 0x1D
 * - Control at offset 0x28
 * - HitCount at offset 0x40
 *
 * The remaining fields are structurally verified but semantically opaque on this build.
 */
typedef struct _PPM_WMI_PERFSTATES_DATA
{
    ULONG Reserved0;
    ULONG StateCount;
    ULONG Reserved1;
    ULONG Reserved2;
    ULONGLONG Reserved3;
    UCHAR PercentFrequency;
    UCHAR Type;
    USHORT Reserved4;
    ULONG Reserved5;
    ULONG Reserved6;
    ULONGLONG Control;
    ULONGLONG Reserved7;
    ULONGLONG Reserved8;
    ULONG HitCount;
    ULONG Reserved9;
    ULONGLONG Reserved10;
} PPM_WMI_PERFSTATES_DATA, *PPPM_WMI_PERFSTATES_DATA;

// rev
/**
 * The POWER_INTERNAL_PPM_PERF_FREQUENCY_BAND_STATS_BANK structure describes power internal ppm perf frequency band stats bank.
 */
typedef struct _POWER_INTERNAL_PPM_PERF_FREQUENCY_BAND_STATS_BANK
{
    // Metric[0][0..47], Metric[1][0..47], Metric[2][0..47]
    ULONGLONG Metric[PPM_PERF_METRICS_COUNT][PPM_PERF_BANDS_COUNT];
} POWER_INTERNAL_PPM_PERF_FREQUENCY_BAND_STATS_BANK, PPOWER_INTERNAL_PM_PERF_FREQUENCY_BAND_STATS_BANK;

// rev
/**
 * The POWER_INTERNAL_PPM_PERF_FREQUENCY_BAND_STATS_OUT structure describes power internal ppm perf frequency band stats out.
 */
typedef struct _POWER_INTERNAL_PPM_PERF_FREQUENCY_BAND_STATS_OUT
{
    POWER_INTERNAL_PPM_PERF_FREQUENCY_BAND_STATS_BANK Bank[PPM_PERF_BANKS_COUNT];
} POWER_INTERNAL_PPM_PERF_FREQUENCY_BAND_STATS_OUT;

// rev
/**
 * The POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS structure describes power internal processor class band stats.
 */
typedef struct _POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS
{
    ULONGLONG Counter[PPM_PERF_METRICS_COUNT];
} POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS, *PPOWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS;

// rev
/**
 * The POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_OUTPUT structure contains the output data returned by the processor class band stats operation.
 */
typedef struct _POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_OUTPUT
{
    POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS Band[PPM_PERF_BANDS_COUNT];
} POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_OUTPUT, *PPOWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_INPUT structure contains the input parameters for the get adaptive session state operation.
 */
typedef struct _POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG SessionStateId;
    UCHAR Reserved[28];
} POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_INPUT, *PPOWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_INPUT;

// rev
/**
 * The POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_OUTPUT structure contains the output data returned by the get adaptive session state operation.
 */
typedef struct _POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_OUTPUT
{
    ULONG DisplayTimeout;
    ULONG DimTimeout;
    ULONG InputTimeout;
    ULONG InputTimeoutDisabled;
} POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_OUTPUT, *PPOWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_SET_CONSOLE_LOCKED_STATE_INPUT structure contains the input parameters for the set console locked state operation.
 */
typedef struct _POWER_INTERNAL_SET_CONSOLE_LOCKED_STATE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    BOOLEAN Locked; // 1 if console is locked, 0 if unlocked
} POWER_INTERNAL_SET_CONSOLE_LOCKED_STATE_INPUT, *PPOWER_INTERNAL_SET_CONSOLE_LOCKED_STATE_INPUT;

// rev
/**
 * The POWER_INTERNAL_FAN_IMPACT_STATS_INPUT structure contains the input parameters for the fan impact stats operation.
 */
typedef struct _POWER_INTERNAL_FAN_IMPACT_STATS_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_FAN_IMPACT_STATS_INPUT, *PPOWER_INTERNAL_FAN_IMPACT_STATS_INPUT;

// rev
/**
 * The POWER_INTERNAL_FAN_IMPACT_STATS_OUTPUT structure contains the output data returned by the PowerInternalFanImpactStats operation.
 * \remarks Reversed from PopFanReadFanNoiseInfo (selector 84). BucketCountPlusTwo is the enabled fan's bucket count plus two; Buckets holds that
 * many ULONGLONG impact samples copied (after PopFanUpdateStatistics) from the fan device state. The level requires exactly one enabled fan,
 * otherwise STATUS_UNSUCCESSFUL (0xC0000001) is returned; the fixed output length is 160 bytes.
 */
typedef struct _POWER_INTERNAL_FAN_IMPACT_STATS_OUTPUT
{
    ULONG BucketCountPlusTwo;
    ULONGLONG Buckets[19];
} POWER_INTERNAL_FAN_IMPACT_STATS_OUTPUT, *PPOWER_INTERNAL_FAN_IMPACT_STATS_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_FAN_RPM_BUCKETS_INPUT structure contains the input parameters for the fan rpm buckets operation.
 */
typedef struct _POWER_INTERNAL_FAN_RPM_BUCKETS_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_FAN_RPM_BUCKETS_INPUT, *PPOWER_INTERNAL_FAN_RPM_BUCKETS_INPUT;

// rev
/**
 * The POWER_INTERNAL_FAN_RPM_OUTPUT structure contains the output data returned by the PowerInternalFanRpmBuckets operation.
 * \remarks Reversed from PopFanReadFanNoiseInfo (selector 85). NumberOfFanRpmBuckets is the enabled fan's bucket count; BucketMaxRpm and
 * NoiseZoneMaxRpm are copied from the fan device state. The level requires exactly one enabled fan, otherwise STATUS_UNSUCCESSFUL (0xC0000001)
 * is returned; the fixed output length is 88 bytes (see the C_ASSERT below).
 */
typedef struct _POWER_INTERNAL_FAN_RPM_OUTPUT
{
    ULONG NumberOfFanRpmBuckets;
    ULONG BucketMaxRpm[17];
    ULONG NoiseZoneMaxRpm[4];
} POWER_INTERNAL_FAN_RPM_OUTPUT, *PPOWER_INTERNAL_FAN_RPM_OUTPUT;

C_ASSERT(sizeof(POWER_INTERNAL_FAN_RPM_OUTPUT) == 0x58);

// rev
/**
 * The POWER_INTERNAL_BOOTAPP_DIAGNOSTIC structure contains the last boot application (bootmgr/winload) diagnostic captured from the persisted shutdown/BCD marker.
 * \remarks Reversed from PopPowerInformationInternal level 86. The fields are copied from the kernel globals ExBootAppErrorDiagCode and
 * ExBootAppFailureStatus (populated by PopCheckShutdownMarker; also emitted to ETW by BapdWriteEtwEvents). The level requires an output buffer
 * of at least 8 bytes, otherwise STATUS_BUFFER_TOO_SMALL (0xC0000023) is returned.
 */
typedef struct _POWER_INTERNAL_BOOTAPP_DIAGNOSTIC
{
    ULONG BootAppErrorDiagCode; // ExBootAppErrorDiagCode (bcdedit last boot error diagnostic code)
    ULONG BootAppFailureStatus; // ExBootAppFailureStatus (bcdedit last boot failure NTSTATUS)
} POWER_INTERNAL_BOOTAPP_DIAGNOSTIC, *PPOWER_INTERNAL_BOOTAPP_DIAGNOSTIC;

// rev
/**
 * The POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_INPUT structure contains the input parameters for the get acpi time and alarm capabilities operation.
 */
typedef struct _POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    UCHAR Reserved[12];
} POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_INPUT, *PPOWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_INPUT;

// rev
/**
 * The POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_OUTPUT structure contains the output data returned by the get acpi time and alarm capabilities operation.
 */
typedef struct  _POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_OUTPUT
{
    UCHAR Capabilities[20];
} POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_OUTPUT, *PPOWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_SOC_IDENTIFIER_OPERATION_INPUT structure contains the input parameters for the soc identifier operation operation.
 */
typedef struct _POWER_INTERNAL_SOC_IDENTIFIER_OPERATION_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG Action;
    ULONG Domain;
    ULONG Reserved[2];
} POWER_INTERNAL_SOC_IDENTIFIER_OPERATION_INPUT, *PPOWER_INTERNAL_SOC_IDENTIFIER_OPERATION_INPUT;

// rev
/**
 * The POWER_INTERNAL_SOC_IDENTIFIER_OPERATION_OUTPUT structure contains the output data returned by the soc identifier operation operation.
 */
typedef struct _POWER_INTERNAL_SOC_IDENTIFIER_OPERATION_OUTPUT
{
    // Action 0 returns a USHORT maximum-length value.
    // Action 1 returns a UTF-16 identifier blob.
    UCHAR Data[ANYSIZE_ARRAY];
} POWER_INTERNAL_SOC_IDENTIFIER_OPERATION_OUTPUT, *PPOWER_INTERNAL_SOC_IDENTIFIER_OPERATION_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_INPUT structure contains the input parameters for the vmperf priority support operation.
 */
typedef struct _POWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_INPUT, *PPOWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_INPUT;

// rev
/**
 * The POWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_OUTPUT structure contains the output data returned by the PowerInternalGetVmPerfPrioritySupport operation.
 * \remarks Reversed from PpmPerfGetVmPerfPrioritySupport. Values reflect the current PRCB perf domain's VmThrottlePriorityCount and are only
 * populated when PpmPerfVmPerfSelectionSupported is set; VmThrottleSupportedAndConfigured is TRUE when VmThrottlePriorityCount != 0.
 */
typedef struct _POWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_OUTPUT
{
    BOOLEAN VmThrottleSupportedAndConfigured;
    ULONG VmThrottlePriorityCount;
} POWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_OUTPUT, *PPOWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_INPUT structure contains the input parameters for the PowerInternalGetVmPerfPriorityConfig operation.
 * \remarks Reversed from PpmPerfGetVmPerfPriorityConfig. Priority is the selector forwarded to the perf domain's PerfPriorityHandler callback.
 */
typedef struct _POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG Priority;
} POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_INPUT, *PPOWER_INTERNAL_VMPERF_PRIORITY_CONFIG_INPUT;

// rev
/**
 * The POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_OUTPUT structure contains the output data returned by the PowerInternalGetVmPerfPriorityConfig operation.
 * \remarks Reversed from PpmPerfGetVmPerfPriorityConfig. The 8-byte Data field is populated by the perf domain's PerfPriorityHandler callback;
 * the level returns 0xC000003B when PpmPerfVmPerfSelectionSupported is clear or no handler is registered.
 */
typedef struct _POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_OUTPUT
{
    ULONGLONG Data;
} POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_OUTPUT, *PPOWER_INTERNAL_VMPERF_PRIORITY_CONFIG_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_DIRECTED_FX_ADD_TEST_DEVICE_INPUT structure contains the input parameters for the directed fx add test device operation.
 */
typedef struct _POWER_INTERNAL_DIRECTED_FX_ADD_TEST_DEVICE_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONG Flags;
    ULONG DeviceNameLength;
    WCHAR DeviceName[ANYSIZE_ARRAY];
} POWER_INTERNAL_DIRECTED_FX_ADD_TEST_DEVICE_INPUT, *PPOWER_INTERNAL_DIRECTED_FX_ADD_TEST_DEVICE_INPUT;

// rev
/**
 * The POWER_INTERNAL_DIRECTED_FX_REMOVE_TEST_DEVICE_INPUT structure contains the input parameters for the directed fx remove test device operation.
 */
typedef struct _POWER_INTERNAL_DIRECTED_FX_REMOVE_TEST_DEVICE_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONG DeviceNameLength;
    WCHAR DeviceName[ANYSIZE_ARRAY];
} POWER_INTERNAL_DIRECTED_FX_REMOVE_TEST_DEVICE_INPUT, *PPOWER_INTERNAL_DIRECTED_FX_REMOVE_TEST_DEVICE_INPUT;

// rev
/**
 * The POWER_INTERNAL_DIRECTED_FX_SET_MODE_INPUT structure contains the input parameters for the directed fx set mode operation.
 */
typedef struct _POWER_INTERNAL_DIRECTED_FX_SET_MODE_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    BOOLEAN PermissiveMode;
} POWER_INTERNAL_DIRECTED_FX_SET_MODE_INPUT, *PPOWER_INTERNAL_DIRECTED_FX_SET_MODE_INPUT;

// rev
/**
 * The POWER_INTERNAL_DIRECTED_DRIPS_QUERY_CAPABILITIES_OUTPUT structure contains the output data returned by the directed drips query capabilities operation.
 */
typedef struct _POWER_INTERNAL_DIRECTED_DRIPS_QUERY_CAPABILITIES_OUTPUT
{
    BOOLEAN SupportsDirectedFxTestDevice;
    BOOLEAN SupportsDirectedFxModeControl;
} POWER_INTERNAL_DIRECTED_DRIPS_QUERY_CAPABILITIES_OUTPUT, *PPOWER_INTERNAL_DIRECTED_DRIPS_QUERY_CAPABILITIES_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_REGISTER_POWER_PLANE_INPUT structure contains the input parameters for the register power plane operation.
 */
typedef struct _POWER_INTERNAL_REGISTER_POWER_PLANE_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONG_PTR ClientHandle;
    ULONG_PTR DataSize;
    UCHAR Data[ANYSIZE_ARRAY];
} POWER_INTERNAL_REGISTER_POWER_PLANE_INPUT, *PPOWER_INTERNAL_REGISTER_POWER_PLANE_INPUT;

// rev
/**
 * The POWER_INTERNAL_DIRECTED_DRIPS_DEVICE_FLAGS_INPUT structure contains the input parameters for the directed drips device flags operation.
 */
typedef struct _POWER_INTERNAL_DIRECTED_DRIPS_DEVICE_FLAGS_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    PVOID DeviceObject;
    ULONG Flags;
    ULONG Reserved;
} POWER_INTERNAL_DIRECTED_DRIPS_DEVICE_FLAGS_INPUT, *PPOWER_INTERNAL_DIRECTED_DRIPS_DEVICE_FLAGS_INPUT;

// rev
/**
 * The POWER_INTERNAL_RETRIEVE_HIBERFILE_RESUME_CONTEXT_OUTPUT structure contains the output data returned by the retrieve hiberfile resume context operation.
 */
typedef struct _POWER_INTERNAL_RETRIEVE_HIBERFILE_RESUME_CONTEXT_OUTPUT
{
    ULONG Version;
    ULONG DataSize;
    ULONG EntryCount;
    UCHAR Data[ANYSIZE_ARRAY];
} POWER_INTERNAL_RETRIEVE_HIBERFILE_RESUME_CONTEXT_OUTPUT, *PPOWER_INTERNAL_RETRIEVE_HIBERFILE_RESUME_CONTEXT_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_QUERY_SLEEPSTUDY_HELPER_ROUTINE_BLOCK_OUTPUT structure contains the output data returned by the query sleepstudy helper routine block operation.
 */
typedef struct _POWER_INTERNAL_QUERY_SLEEPSTUDY_HELPER_ROUTINE_BLOCK_OUTPUT
{
    PVOID HelperRoutineBlock;
} POWER_INTERNAL_QUERY_SLEEPSTUDY_HELPER_ROUTINE_BLOCK_OUTPUT, *PPOWER_INTERNAL_QUERY_SLEEPSTUDY_HELPER_ROUTINE_BLOCK_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_CLEAR_CONSTRAINTS_INPUT structure contains the input parameters for the clear constraints operation.
 */
typedef struct _POWER_INTERNAL_CLEAR_CONSTRAINTS_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    PVOID DeviceObject;
} POWER_INTERNAL_CLEAR_CONSTRAINTS_INPUT, *PPOWER_INTERNAL_CLEAR_CONSTRAINTS_INPUT;

// rev
/**
 * The POWER_INTERNAL_VM_PERF_CONTROL_CONFIG_INPUT structure contains the input parameters for the vm perf control config operation.
 */
typedef struct _POWER_INTERNAL_VM_PERF_CONTROL_CONFIG_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONG Parameter0;
    ULONG Parameter1;
    ULONG Parameter2;
    ULONG Parameter3;
    ULONG Parameter4;
    BOOLEAN Parameter5;
    UCHAR Reserved[3];
} POWER_INTERNAL_VM_PERF_CONTROL_CONFIG_INPUT, *PPOWER_INTERNAL_VM_PERF_CONTROL_CONFIG_INPUT;

// rev
/**
 * The POWER_INTERNAL_VM_PERF_CONTROL_CONFIG_OUTPUT structure contains the output data returned by the vm perf control config operation.
 */
typedef struct _POWER_INTERNAL_VM_PERF_CONTROL_CONFIG_OUTPUT
{
    ULONG Value0;
    ULONG Value1;
} POWER_INTERNAL_VM_PERF_CONTROL_CONFIG_OUTPUT, *PPOWER_INTERNAL_VM_PERF_CONTROL_CONFIG_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_CONCURRENCY_STATS_OUTPUT structure is the variable-length wire format
 * returned by the internal concurrency-statistics queries.
 *
 * For PowerInternalCpuNodeConcurrencyStats:
 * - Count[0] is the bucket count.
 * - Count[1] is reserved.
 * - Data contains Count[0] + 1 ULONGLONG values.
 *
 * For PowerInternalClassConcurrencyStats:
 * - Count[0] and Count[1] are the per-class bucket counts.
 * - Data contains the class 0 ULONGLONG sequence followed by the class 1 sequence.
 * - Each present class contributes Count[n] + 1 ULONGLONG values.
 */
typedef struct _POWER_INTERNAL_CONCURRENCY_STATS_OUTPUT
{
    ULONG Count[2];
    ULONGLONG Data[ANYSIZE_ARRAY];
} POWER_INTERNAL_CONCURRENCY_STATS_OUTPUT, *PPOWER_INTERNAL_CONCURRENCY_STATS_OUTPUT;

typedef POWER_INTERNAL_CONCURRENCY_STATS_OUTPUT POWER_INTERNAL_CPU_NODE_CONCURRENCY_STATS_OUTPUT;
typedef POWER_INTERNAL_CONCURRENCY_STATS_OUTPUT *PPOWER_INTERNAL_CPU_NODE_CONCURRENCY_STATS_OUTPUT;

typedef POWER_INTERNAL_CONCURRENCY_STATS_OUTPUT POWER_INTERNAL_CLASS_CONCURRENCY_STATS_OUTPUT;
typedef POWER_INTERNAL_CONCURRENCY_STATS_OUTPUT *PPOWER_INTERNAL_CLASS_CONCURRENCY_STATS_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_QUERY_MEASUREMENT_CAPABILITIES structure describes power internal query measurement capabilities.
 */
typedef struct _POWER_INTERNAL_QUERY_MEASUREMENT_CAPABILITIES
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
} POWER_INTERNAL_QUERY_MEASUREMENT_CAPABILITIES, *PPOWER_INTERNAL_QUERY_MEASUREMENT_CAPABILITIES;

// rev
/**
 * The POWER_INTERNAL_QUERY_MEASUREMENT_CAPABILITIES_OUTPUT structure contains the output data returned by the query measurement capabilities operation.
 */
typedef struct _POWER_INTERNAL_QUERY_MEASUREMENT_CAPABILITIES_OUTPUT
{
    ULONG Capabilities;
} POWER_INTERNAL_QUERY_MEASUREMENT_CAPABILITIES_OUTPUT, *PPOWER_INTERNAL_QUERY_MEASUREMENT_CAPABILITIES_OUTPUT;

// rev
/**
 * The PROCESSOR_INTERNAL_QUERY_MEASUREMENT_VALUES structure describes processor internal query measurement values.
 */
typedef struct _PROCESSOR_INTERNAL_QUERY_MEASUREMENT_VALUES
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONG Processor;
} PROCESSOR_INTERNAL_QUERY_MEASUREMENT_VALUES, *PPROCESSOR_INTERNAL_QUERY_MEASUREMENT_VALUES;

// rev
/**
 * The PROCESSOR_INTERNAL_QUERY_MEASUREMENT_VALUE_ENTRY structure describes processor internal query measurement value entry.
 */
typedef struct _PROCESSOR_INTERNAL_QUERY_MEASUREMENT_VALUE_ENTRY
{
    ULONGLONG Value[3];
} PROCESSOR_INTERNAL_QUERY_MEASUREMENT_VALUE_ENTRY, *PPROCESSOR_INTERNAL_QUERY_MEASUREMENT_VALUE_ENTRY;

// rev
/**
 * The PROCESSOR_INTERNAL_QUERY_MEASUREMENT_VALUES_OUTPUT structure contains the output data returned by the query measurement values operation.
 */
typedef struct _PROCESSOR_INTERNAL_QUERY_MEASUREMENT_VALUES_OUTPUT
{
    ULONG EntryCount;
    ULONG Reserved;
    PROCESSOR_INTERNAL_QUERY_MEASUREMENT_VALUE_ENTRY Entries[ANYSIZE_ARRAY];
} PROCESSOR_INTERNAL_QUERY_MEASUREMENT_VALUES_OUTPUT, *PPROCESSOR_INTERNAL_QUERY_MEASUREMENT_VALUES_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_PREPARE_FOR_SYSTEM_INITIATED_REBOOT_INPUT structure contains the input parameters for the prepare for system initiated reboot operation.
 */
typedef struct _POWER_INTERNAL_PREPARE_FOR_SYSTEM_INITIATED_REBOOT_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
} POWER_INTERNAL_PREPARE_FOR_SYSTEM_INITIATED_REBOOT_INPUT, *PPOWER_INTERNAL_PREPARE_FOR_SYSTEM_INITIATED_REBOOT_INPUT;

// rev
/**
 * The POWER_INTERNAL_OVERRIDE_SYSTEM_INITIATED_REBOOT_STATE_INPUT structure contains the input parameters for the override system initiated reboot state operation.
 */
typedef struct _POWER_INTERNAL_OVERRIDE_SYSTEM_INITIATED_REBOOT_STATE_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONG State;
    ULONG Reserved;
} POWER_INTERNAL_OVERRIDE_SYSTEM_INITIATED_REBOOT_STATE_INPUT, *PPOWER_INTERNAL_OVERRIDE_SYSTEM_INITIATED_REBOOT_STATE_INPUT;

// rev
/**
 * The POWER_INTERNAL_UNREGISTER_SHUTDOWN_NOTIFICATION_INPUT structure contains the input parameters for the unregister shutdown notification operation.
 */
typedef struct _POWER_INTERNAL_UNREGISTER_SHUTDOWN_NOTIFICATION_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    HANDLE ProcessId;
    HANDLE ThreadId;
} POWER_INTERNAL_UNREGISTER_SHUTDOWN_NOTIFICATION_INPUT, *PPOWER_INTERNAL_UNREGISTER_SHUTDOWN_NOTIFICATION_INPUT;

// rev
/**
 * The POWER_INTERNAL_MANAGE_TRANSITION_STATE_RECORD_INPUT structure contains the input parameters for the manage transition state record operation.
 */
typedef struct _POWER_INTERNAL_MANAGE_TRANSITION_STATE_RECORD_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONG Action;
    ULONG Reserved0;
    HANDLE ProcessId;
    HANDLE ThreadId;
    ULONG Reason;
    ULONG CallbackType;
    PVOID CallbackContext;
} POWER_INTERNAL_MANAGE_TRANSITION_STATE_RECORD_INPUT, *PPOWER_INTERNAL_MANAGE_TRANSITION_STATE_RECORD_INPUT;

// rev
/**
 * The POWER_INTERNAL_SUSPEND_RESUME_REQUEST_INPUT structure contains the input parameters for the suspend resume request operation.
 */
typedef struct _POWER_INTERNAL_SUSPEND_RESUME_REQUEST_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    UCHAR Data[12];
} POWER_INTERNAL_SUSPEND_RESUME_REQUEST_INPUT, *PPOWER_INTERNAL_SUSPEND_RESUME_REQUEST_INPUT;

// rev
/**
 * The POWER_INTERNAL_ENERGY_ESTIMATION_ENTRY structure describes power internal energy estimation entry.
 */
typedef struct _POWER_INTERNAL_ENERGY_ESTIMATION_ENTRY
{
    ULONG Data[4];
} POWER_INTERNAL_ENERGY_ESTIMATION_ENTRY, *PPOWER_INTERNAL_ENERGY_ESTIMATION_ENTRY;

// rev
/**
 * The POWER_INTERNAL_ENERGY_ESTIMATION_INFO_OUTPUT structure contains the output data returned by the energy estimation info operation.
 */
typedef struct _POWER_INTERNAL_ENERGY_ESTIMATION_INFO_OUTPUT
{
    ULONG EntryCount;
    POWER_INTERNAL_ENERGY_ESTIMATION_ENTRY Entries[2];
} POWER_INTERNAL_ENERGY_ESTIMATION_INFO_OUTPUT, *PPOWER_INTERNAL_ENERGY_ESTIMATION_INFO_OUTPUT;

// rev
/**
 * The POWER_INTERNAL_NOTIFY_WIN32K_POWER_REQUEST_INPUT structure contains the input parameters for the notify win32k power request operation.
 */
typedef struct _POWER_INTERNAL_NOTIFY_WIN32K_POWER_REQUEST_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONG RequestIndex;
} POWER_INTERNAL_NOTIFY_WIN32K_POWER_REQUEST_INPUT, *PPOWER_INTERNAL_NOTIFY_WIN32K_POWER_REQUEST_INPUT;

C_ASSERT(sizeof(POWER_INTERNAL_NOTIFY_WIN32K_POWER_REQUEST_INPUT) == 0x0C);

// rev
/**
 * The POWER_INTERNAL_PDC_AGENT_SESSION_QUERY_INPUT structure contains the input parameters for the pdc agent session query operation.
 */
typedef struct _POWER_INTERNAL_PDC_AGENT_SESSION_QUERY_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONG QueryKey;
} POWER_INTERNAL_PDC_AGENT_SESSION_QUERY_INPUT, *PPOWER_INTERNAL_PDC_AGENT_SESSION_QUERY_INPUT;

C_ASSERT(sizeof(POWER_INTERNAL_PDC_AGENT_SESSION_QUERY_INPUT) == 0x0C);

// rev
/**
 * The POWER_INTERNAL_SESSION_CONNECTION_INFO_V2 structure describes power internal session connection info v2.
 */
typedef struct _POWER_INTERNAL_SESSION_CONNECTION_INFO_V2
{
    BOOLEAN Connected;
    UCHAR Reserved0[3];
    ULONG ConnectionType;
} POWER_INTERNAL_SESSION_CONNECTION_INFO_V2, *PPOWER_INTERNAL_SESSION_CONNECTION_INFO_V2;

// rev
/**
 * The POWER_INTERNAL_ADAPTIVE_SESSION_STATE_REQUEST structure describes power internal adaptive session state request.
 */
typedef struct _POWER_INTERNAL_ADAPTIVE_SESSION_STATE_REQUEST
{
    ULONGLONG Field0;
    ULONGLONG Field1;
    ULONGLONG Field2;
    ULONG Field3;
    ULONG Reserved;
} POWER_INTERNAL_ADAPTIVE_SESSION_STATE_REQUEST, *PPOWER_INTERNAL_ADAPTIVE_SESSION_STATE_REQUEST;

// rev
/**
 * The POWER_INTERNAL_SESSION_CONNECTION_CHANGE_V2_INPUT structure contains the input parameters for the session connection change v2 operation.
 */
typedef struct _POWER_INTERNAL_SESSION_CONNECTION_CHANGE_V2_INPUT
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    POWER_INTERNAL_SESSION_CONNECTION_INFO_V2 ConnectionInfo;
    POWER_INTERNAL_ADAPTIVE_SESSION_STATE_REQUEST SessionStateRequest;
} POWER_INTERNAL_SESSION_CONNECTION_CHANGE_V2_INPUT, *PPOWER_INTERNAL_SESSION_CONNECTION_CHANGE_V2_INPUT;

C_ASSERT(sizeof(POWER_INTERNAL_SESSION_CONNECTION_CHANGE_V2_INPUT) == 0x30);

// rev
// POWER_INFORMATION_ENERGY_TRACKER_CREATE_INPUT Flags values
#define POWER_INFORMATION_ENERGY_TRACKER_PROCESS_MIN                    ULONG_C(0x00000001)
#define POWER_INFORMATION_ENERGY_TRACKER_PROCESS_MAX                    ULONG_C(0x00040000)

#define POWER_INFORMATION_ENERGY_TRACKER_CREATE_FLAGS_NONE              ULONG_C(0x00000000)
#define POWER_INFORMATION_ENERGY_TRACKER_CREATE_FLAGS_MODE_PID          ULONG_C(0x00000001)
#define POWER_INFORMATION_ENERGY_TRACKER_CREATE_FLAGS_MODE_UNKNOWN      ULONG_C(0x10000000)
#define POWER_INFORMATION_ENERGY_TRACKER_CREATE_FLAGS_MODE_MASK         ULONG_C(0xF0000000)

// rev
/**
 * The POWER_INFORMATION_ENERGY_TRACKER_CREATE_INPUT structure contains the input parameters for the energy tracker create operation.
 */
typedef struct _POWER_INFORMATION_ENERGY_TRACKER_CREATE_INPUT
{
    ULONG MaxTrackedProcesses; // POWER_INFORMATION_ENERGY_TRACKER_PROCESS_MAX
    ULONG Reserved;
    ULONG Flags;
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
 * The POWER_INFORMATION_ENERGY_TRACKER_QUERY_OUTPUT structure contains the output data returned by the energy tracker query operation.
 */
typedef struct _POWER_INFORMATION_ENERGY_TRACKER_QUERY_OUTPUT
{
    ULONG Signature;
    ULONG HeaderSize;
    ULONG TotalSize;
    ULONG Sequence;
    ULONG DeltaPerformanceTime; // Elapsed Ticks
    ULONG DeltaInterruptTime;   // Elapsed Ms
    ULONG CurrentPerformanceTime;
    ULONG CurrentTimelineTime;
    ULONG CurrentTimelineBitmapTime;
    ULONG ProcessRecordOffset;
    ULONG ProcessRecordCount;
    ULONG EnergyDeltaOffset;
    ULONG EnergyCumulativeOffset;
    ULONG Offset;
    ULONG Type;
    USHORT Size;
    USHORT Reserved;
    ULONG CurrentSystemTimeLow;
    ULONG CurrentSystemTimeHigh;
} POWER_INFORMATION_ENERGY_TRACKER_QUERY_OUTPUT, *PPOWER_INFORMATION_ENERGY_TRACKER_QUERY_OUTPUT;

// rev
/**
 * The POWER_INFORMATION_ENERGY_TRACKER_ENTRY structure describes power information energy tracker entry.
 */
typedef struct _POWER_INFORMATION_ENERGY_TRACKER_ENTRY
{
    ULONGLONG ProcessKey;       // requires POWER_INFORMATION_ENERGY_TRACKER_CREATE_FLAGS_MODE_PID
    ULONG ProcessId;
    ULONG EntryStateLow16;
    ULONG FileNameOffset;
    ULONG MetricA0;
    ULONG MetricA1;
    ULONG PackageNameOffset;
    ULONG InstallLocationOffset;
    ULONG SubProcessTagOffset;
    ULONG NameOffset;
    ULONG MetricA3;
    ULONG MetricA4;
    USHORT FileNameCount;
    USHORT PackageNameCount;
    USHORT InstallLocationCount;
    USHORT SubProcessTagCount;
    USHORT BaseNameLength;
    USHORT Reserved;
    ULONG DetailBlobOffset;
    ULONG DetailBlobSize;
    ULONG TailStat0;
    ULONG TailStat1;
    UCHAR ExtraTelemetry[24];
} POWER_INFORMATION_ENERGY_TRACKER_ENTRY, *PPOWER_INFORMATION_ENERGY_TRACKER_ENTRY;

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
 * In this build the handler requires Reserved0..Reserved3 to be zero and only honors Index; the Version/Flags/Category/Modifiers/Offset/Length
 * form and the POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_* macros above are not evaluated by the level-97 handler.
 */
typedef struct _POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_INPUT
{
    ULONGLONG Reserved0;    // Must be 0.
    ULONGLONG Reserved1;    // Must be 0 (Offset is not honored in this build).
    ULONGLONG Reserved2;    // Must be 0 (Length is not honored in this build).
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
 */
_Kernel_entry_
NTSYSCALLAPI
BOOLEAN
NTAPI
NtIsSystemResumeAutomatic(
    VOID
    );

#endif // _NTPOAPI_H
