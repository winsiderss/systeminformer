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
#define SystemPowerPolicyAc 0 // SYSTEM_POWER_POLICY // GET: InputBuffer NULL. SET: InputBuffer not NULL.
#define SystemPowerPolicyDc 1 // SYSTEM_POWER_POLICY
#define VerifySystemPolicyAc 2 // SYSTEM_POWER_POLICY
#define VerifySystemPolicyDc 3 // SYSTEM_POWER_POLICY
#define SystemPowerCapabilities 4 // SYSTEM_POWER_CAPABILITIES
#define SystemBatteryState 5 // SYSTEM_BATTERY_STATE
#define SystemPowerStateHandler 6 // POWER_STATE_HANDLER // (kernel-mode only)
#define ProcessorStateHandler 7 // PROCESSOR_STATE_HANDLER // (kernel-mode only)
#define SystemPowerPolicyCurrent 8 // SYSTEM_POWER_POLICY
#define AdministratorPowerPolicy 9 // ADMINISTRATOR_POWER_POLICY
#define SystemReserveHiberFile 10 // BOOLEAN // (requires SeCreatePagefilePrivilege) // TRUE: hibernation file created. FALSE: hibernation file deleted.
#define ProcessorInformation 11 // PROCESSOR_POWER_INFORMATION
#define SystemPowerInformation 12 // SYSTEM_POWER_INFORMATION
#define ProcessorStateHandler2 13 // PROCESSOR_STATE_HANDLER2 // not implemented
#define LastWakeTime 14 // ULONGLONG // InterruptTime
#define LastSleepTime 15 // ULONGLONG // InterruptTime
#define SystemExecutionState 16 // EXECUTION_STATE // NtSetThreadExecutionState
#define SystemPowerStateNotifyHandler 17 // POWER_STATE_NOTIFY_HANDLER // (kernel-mode only)
#define ProcessorPowerPolicyAc 18 // PROCESSOR_POWER_POLICY // not implemented
#define ProcessorPowerPolicyDc 19 // PROCESSOR_POWER_POLICY // not implemented
#define VerifyProcessorPowerPolicyAc 20 // PROCESSOR_POWER_POLICY // not implemented
#define VerifyProcessorPowerPolicyDc 21 // PROCESSOR_POWER_POLICY // not implemented
#define ProcessorPowerPolicyCurrent 22 // PROCESSOR_POWER_POLICY // not implemented
#define SystemPowerStateLogging 23 // SYSTEM_POWER_STATE_DISABLE_REASON[]
#define SystemPowerLoggingEntry 24 // SYSTEM_POWER_LOGGING_ENTRY[] // (kernel-mode only)
#define SetPowerSettingValue 25 // (kernel-mode only)
#define NotifyUserPowerSetting 26 // not implemented
#define PowerInformationLevelUnused0 27 // not implemented
#define SystemMonitorHiberBootPowerOff 28 // NULL (PowerMonitorOff)
#define SystemVideoState 29 // MONITOR_DISPLAY_STATE
#define TraceApplicationPowerMessage 30 // (kernel-mode only)
#define TraceApplicationPowerMessageEnd 31 // (kernel-mode only)
#define ProcessorPerfStates 32 // (kernel-mode only)
#define ProcessorIdleStates 33 // PROCESSOR_IDLE_STATES // (kernel-mode only)
#define ProcessorCap 34 // PROCESSOR_CAP // (kernel-mode only)
#define SystemWakeSource 35 // out: POWER_WAKE_SOURCE_INFO
#define SystemHiberFileInformation 36 // out: SYSTEM_HIBERFILE_INFORMATION
#define TraceServicePowerMessage 37
#define ProcessorLoad 38 // in: PROCESSOR_LOAD (sets), in: PPROCESSOR_NUMBER (clears)
#define PowerShutdownNotification 39 // (kernel-mode only)
#define MonitorCapabilities 40 // (kernel-mode only)
#define SessionPowerInit 41 // (kernel-mode only)
#define SessionDisplayState 42 // (kernel-mode only)
#define PowerRequestCreate 43 // in: COUNTED_REASON_CONTEXT, out: HANDLE
#define PowerRequestAction 44 // in: POWER_REQUEST_ACTION
#define GetPowerRequestList 45 // out: POWER_REQUEST_LIST
#define ProcessorInformationEx 46 // in: USHORT ProcessorGroup, out: PROCESSOR_POWER_INFORMATION
#define NotifyUserModeLegacyPowerEvent 47 // (kernel-mode only)
#define GroupPark 48 // (debug-mode boot only)
#define ProcessorIdleDomains 49 // (kernel-mode only)
#define WakeTimerList 50 // out: WAKE_TIMER_INFO[]
#define SystemHiberFileSize 51 // ULONG
#define ProcessorIdleStatesHv 52 // (kernel-mode only)
#define ProcessorPerfStatesHv 53 // (kernel-mode only)
#define ProcessorPerfCapHv 54 // PROCESSOR_PERF_CAP_HV // (kernel-mode only)
#define ProcessorSetIdle 55 // (debug-mode boot only)
#define LogicalProcessorIdling 56 // (kernel-mode only)
#define UserPresence 57 // POWER_USER_PRESENCE // not implemented
#define PowerSettingNotificationName 58
#define GetPowerSettingValue 59 // GUID
#define IdleResiliency 60 // POWER_IDLE_RESILIENCY
#define SessionRITState 61 // POWER_SESSION_RIT_STATE
#define SessionConnectNotification 62 // POWER_SESSION_WINLOGON
#define SessionPowerCleanup 63
#define SessionLockState 64 // POWER_SESSION_WINLOGON
#define SystemHiberbootState 65 // BOOLEAN // fast startup supported
#define PlatformInformation 66 // BOOLEAN // connected standby supported
#define PdcInvocation 67 // (kernel-mode only)
#define MonitorInvocation 68 // (kernel-mode only)
#define FirmwareTableInformationRegistered 69 // (kernel-mode only)
#define SetShutdownSelectedTime 70 // NULL
#define SuspendResumeInvocation 71 // (kernel-mode only)
#define PlmPowerRequestCreate 72 // in: COUNTED_REASON_CONTEXT, out: HANDLE
#define ScreenOff 73 // NULL (PowerMonitorOff)
#define CsDeviceNotification 74 // (kernel-mode only)
#define PlatformRole 75 // POWER_PLATFORM_ROLE
#define LastResumePerformance 76 // RESUME_PERFORMANCE
#define DisplayBurst 77 // NULL (PowerMonitorOn)
#define ExitLatencySamplingPercentage 78
#define RegisterSpmPowerSettings 79 // (kernel-mode only)
#define PlatformIdleStates 80 // (kernel-mode only)
#define ProcessorIdleVeto 81 // (kernel-mode only) // deprecated
#define PlatformIdleVeto 82 // (kernel-mode only) // deprecated
#define SystemBatteryStatePrecise 83 // SYSTEM_BATTERY_STATE
#define ThermalEvent 84  // THERMAL_EVENT // PowerReportThermalEvent
#define PowerRequestActionInternal 85 // POWER_REQUEST_ACTION_INTERNAL
#define BatteryDeviceState 86
#define PowerInformationInternal 87 // POWER_INFORMATION_LEVEL_INTERNAL // PopPowerInformationInternal
#define ThermalStandby 88 // NULL // shutdown with thermal standby as reason.
#define SystemHiberFileType 89 // ULONG // zero ? reduced : full // powercfg.exe /h /type
#define PhysicalPowerButtonPress 90 // BOOLEAN
#define QueryPotentialDripsConstraint 91 // (kernel-mode only)
#define EnergyTrackerCreate 92
#define EnergyTrackerQuery 93
#define UpdateBlackBoxRecorder 94
#define SessionAllowExternalDmaDevices 95
#define SendSuspendResumeNotification 96 // since WIN11
#define BlackBoxRecorderDirectAccessBuffer 97
#define PowerInformationLevelMaximum 98
#endif

typedef struct _PROCESSOR_POWER_INFORMATION
{
    ULONG Number;
    ULONG MaxMhz;
    ULONG CurrentMhz;
    ULONG MhzLimit;
    ULONG MaxIdleState;
    ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

typedef struct _SYSTEM_POWER_INFORMATION
{
    ULONG MaxIdlenessAllowed;
    ULONG Idleness;
    ULONG TimeRemaining;
    UCHAR CoolingMode;
} SYSTEM_POWER_INFORMATION, *PSYSTEM_POWER_INFORMATION;

typedef struct _SYSTEM_HIBERFILE_INFORMATION
{
    ULONG NumberOfMcbPairs;
    LARGE_INTEGER Mcb[1];
} SYSTEM_HIBERFILE_INFORMATION, *PSYSTEM_HIBERFILE_INFORMATION;

#define POWER_REQUEST_CONTEXT_NOT_SPECIFIED DIAGNOSTIC_REASON_NOT_SPECIFIED

// wdm
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

typedef struct _POWER_REQUEST_ACTION
{
    HANDLE PowerRequestHandle;
    POWER_REQUEST_TYPE_INTERNAL RequestType;
    BOOLEAN SetAction;
    HANDLE ProcessHandle; // Windows 8+ and only for requests created via PlmPowerRequestCreate
} POWER_REQUEST_ACTION, *PPOWER_REQUEST_ACTION;

typedef union _POWER_STATE
{
    SYSTEM_POWER_STATE SystemState;
    DEVICE_POWER_STATE DeviceState;
} POWER_STATE, *PPOWER_STATE;

typedef enum _POWER_STATE_TYPE
{
    SystemPowerState = 0,
    DevicePowerState
} POWER_STATE_TYPE, *PPOWER_STATE_TYPE;

// wdm
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

typedef enum _REQUESTER_TYPE
{
    KernelRequester = 0,
    UserProcessRequester = 1,
    UserSharedServiceRequester = 2
} REQUESTER_TYPE;

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
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    SIZE_T ReasonOffset; // PCOUNTED_REASON_CONTEXT_RELATIVE
} DIAGNOSTIC_BUFFER, *PDIAGNOSTIC_BUFFER;

typedef struct _WAKE_TIMER_INFO
{
    SIZE_T OffsetToNext;
    ULARGE_INTEGER DueTime;
    ULONG Period;
    DIAGNOSTIC_BUFFER ReasonContext;
} WAKE_TIMER_INFO, *PWAKE_TIMER_INFO;

// rev
typedef struct _PROCESSOR_PERF_CAP_HV
{
    ULONG Version;
    ULONG InitialApicId;
    ULONG Ppc;
    ULONG Tpc;
    ULONG ThermalCap;
} PROCESSOR_PERF_CAP_HV, *PPROCESSOR_PERF_CAP_HV;

typedef struct PROCESSOR_IDLE_TIMES
{
    ULONG64 StartTime;
    ULONG64 EndTime;
    ULONG Reserved[4];
} PROCESSOR_IDLE_TIMES, *PPROCESSOR_IDLE_TIMES;

_Function_class_(PROCESSOR_IDLE_HANDLER)
typedef NTSTATUS (FASTCALL PROCESSOR_IDLE_HANDLER)(
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

typedef struct _PROCESSOR_IDLE_STATE
{
    UCHAR StateType;
    ULONG StateFlags;
    ULONG HardwareLatency;
    ULONG Power;
    ULONG_PTR Context;
    PPROCESSOR_IDLE_HANDLER Handler;
} PROCESSOR_IDLE_STATE, *PPROCESSOR_IDLE_STATE;

typedef struct _PROCESSOR_IDLE_STATES
{
    ULONG Size;
    ULONG Revision;
    ULONG Count;
    ULONG Type;
    KAFFINITY TargetProcessors;
    PROCESSOR_IDLE_STATE State[ANYSIZE_ARRAY];
} PROCESSOR_IDLE_STATES, *PPROCESSOR_IDLE_STATES;

// rev
typedef struct _PROCESSOR_LOAD
{
    PROCESSOR_NUMBER ProcessorNumber;
    UCHAR BusyPercentage;
    UCHAR FrequencyPercentage;
    USHORT Padding;
} PROCESSOR_LOAD, *PPROCESSOR_LOAD;

// rev
typedef struct _PROCESSOR_CAP
{
    ULONG Version;
    PROCESSOR_NUMBER ProcessorNumber;
    ULONG PlatformCap;
    ULONG ThermalCap;
    ULONG LimitReasons;
} PROCESSOR_CAP, *PPROCESSOR_CAP;

typedef struct _PO_WAKE_SOURCE_INFO
{
    ULONG Count;
    ULONG Offsets[ANYSIZE_ARRAY]; // POWER_WAKE_SOURCE_HEADER, POWER_WAKE_SOURCE_INTERNAL, POWER_WAKE_SOURCE_TIMER, POWER_WAKE_SOURCE_FIXED
} PO_WAKE_SOURCE_INFO, *PPO_WAKE_SOURCE_INFO;

typedef struct _PO_WAKE_SOURCE_HISTORY
{
    ULONG Count;
    ULONG Offsets[ANYSIZE_ARRAY]; // POWER_WAKE_SOURCE_HEADER, POWER_WAKE_SOURCE_INTERNAL, POWER_WAKE_SOURCE_TIMER, POWER_WAKE_SOURCE_FIXED
} PO_WAKE_SOURCE_HISTORY, *PPO_WAKE_SOURCE_HISTORY;

typedef enum _PO_WAKE_SOURCE_TYPE
{
    DeviceWakeSourceType = 0,
    FixedWakeSourceType = 1,
    TimerWakeSourceType = 2,
    TimerPresumedWakeSourceType = 3,
    InternalWakeSourceType = 4
} PO_WAKE_SOURCE_TYPE, *PPO_WAKE_SOURCE_TYPE;

typedef enum _PO_INTERNAL_WAKE_SOURCE_TYPE
{
    InternalWakeSourceDozeToHibernate = 0,
    InternalWakeSourcePredictedUserPresence = 1
} PO_INTERNAL_WAKE_SOURCE_TYPE;

typedef enum _PO_FIXED_WAKE_SOURCE_TYPE
{
    FixedWakeSourcePowerButton = 0,
    FixedWakeSourceSleepButton = 1,
    FixedWakeSourceRtc = 2,
    FixedWakeSourceDozeToHibernate = 3
} PO_FIXED_WAKE_SOURCE_TYPE, *PPO_FIXED_WAKE_SOURCE_TYPE;

typedef struct _PO_WAKE_SOURCE_HEADER
{
    PO_WAKE_SOURCE_TYPE Type;
    ULONG Size;
} PO_WAKE_SOURCE_HEADER, *PPO_WAKE_SOURCE_HEADER;

typedef struct _PO_WAKE_SOURCE_DEVICE
{
    PO_WAKE_SOURCE_HEADER Header;
    WCHAR InstancePath[ANYSIZE_ARRAY];
} PO_WAKE_SOURCE_DEVICE, *PPO_WAKE_SOURCE_DEVICE;

typedef struct _PO_WAKE_SOURCE_FIXED
{
    PO_WAKE_SOURCE_HEADER Header;
    PO_FIXED_WAKE_SOURCE_TYPE FixedWakeSourceType;
} PO_WAKE_SOURCE_FIXED, *PPO_WAKE_SOURCE_FIXED;

typedef struct _PO_WAKE_SOURCE_INTERNAL
{
    PO_WAKE_SOURCE_HEADER Header;
    PO_INTERNAL_WAKE_SOURCE_TYPE InternalWakeSourceType;
} PO_WAKE_SOURCE_INTERNAL, *PPO_WAKE_SOURCE_INTERNAL;

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
#if (PHNT_VERSION >= PHNT_WIN8)
        struct
        {
            ULONG SupportedRequestMask;
            ULONG PowerRequestCount[POWER_REQUEST_SUPPORTED_TYPES_V2];
            DIAGNOSTIC_BUFFER DiagnosticBuffer;
        } V2;
#endif
#if (PHNT_VERSION >= PHNT_WINBLUE)
        struct
        {
            ULONG SupportedRequestMask;
            ULONG PowerRequestCount[POWER_REQUEST_SUPPORTED_TYPES_V3];
            DIAGNOSTIC_BUFFER DiagnosticBuffer;
        } V3;
#endif
#if (PHNT_VERSION >= PHNT_REDSTONE)
        struct
        {
            ULONG SupportedRequestMask;
            ULONG PowerRequestCount[POWER_REQUEST_SUPPORTED_TYPES_V4];
            DIAGNOSTIC_BUFFER DiagnosticBuffer;
        } V4;
#endif
    };
} POWER_REQUEST, *PPOWER_REQUEST;

typedef struct _POWER_REQUEST_LIST
{
    ULONG_PTR Count;
    ULONG_PTR PowerRequestOffsets[ANYSIZE_ARRAY]; // PPOWER_REQUEST
} POWER_REQUEST_LIST, *PPOWER_REQUEST_LIST;

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

typedef NTSTATUS (NTAPI *PENTER_STATE_SYSTEM_HANDLER)(
    _In_ PVOID SystemContext
    );

typedef NTSTATUS (NTAPI *PENTER_STATE_HANDLER)(
    _In_ PVOID Context,
    _In_opt_ PENTER_STATE_SYSTEM_HANDLER SystemHandler,
    _In_ PVOID SystemContext,
    _In_ LONG NumberProcessors,
    _In_ LONG volatile *Number
    );

typedef struct _POWER_STATE_HANDLER
{
    POWER_STATE_HANDLER_TYPE Type;
    BOOLEAN RtcWake;
    UCHAR Spare[3];
    PENTER_STATE_HANDLER Handler;
    PVOID Context;
} POWER_STATE_HANDLER, *PPOWER_STATE_HANDLER;

typedef NTSTATUS (NTAPI *PENTER_STATE_NOTIFY_HANDLER)(
    _In_ POWER_STATE_HANDLER_TYPE State,
    _In_ PVOID Context,
    _In_ BOOLEAN Entering
    );

typedef struct _POWER_STATE_NOTIFY_HANDLER
{
    PENTER_STATE_NOTIFY_HANDLER Handler;
    PVOID Context;
} POWER_STATE_NOTIFY_HANDLER, *PPOWER_STATE_NOTIFY_HANDLER;

typedef struct _POWER_REQUEST_ACTION_INTERNAL
{
    PVOID PowerRequestPointer;
    POWER_REQUEST_TYPE_INTERNAL RequestType;
    BOOLEAN SetAction;
} POWER_REQUEST_ACTION_INTERNAL, *PPOWER_REQUEST_ACTION_INTERNAL;

typedef enum _POWER_INFORMATION_LEVEL_INTERNAL
{
    PowerInternalAcpiInterfaceRegister,
    PowerInternalS0LowPowerIdleInfo, // POWER_S0_LOW_POWER_IDLE_INFO
    PowerInternalReapplyBrightnessSettings,
    PowerInternalUserAbsencePrediction, // POWER_USER_ABSENCE_PREDICTION
    PowerInternalUserAbsencePredictionCapability, // POWER_USER_ABSENCE_PREDICTION_CAPABILITY
    PowerInternalPoProcessorLatencyHint, // POWER_PROCESSOR_LATENCY_HINT
    PowerInternalStandbyNetworkRequest, // POWER_STANDBY_NETWORK_REQUEST (requires PopNetBIServiceSid)
    PowerInternalDirtyTransitionInformation, // out: BOOLEAN
    PowerInternalSetBackgroundTaskState, // POWER_SET_BACKGROUND_TASK_STATE
    PowerInternalTtmOpenTerminal,
    PowerInternalTtmCreateTerminal, // 10
    PowerInternalTtmEvacuateDevices,
    PowerInternalTtmCreateTerminalEventQueue,
    PowerInternalTtmGetTerminalEvent,
    PowerInternalTtmSetDefaultDeviceAssignment,
    PowerInternalTtmAssignDevice,
    PowerInternalTtmSetDisplayState,
    PowerInternalTtmSetDisplayTimeouts,
    PowerInternalBootSessionStandbyActivationInformation, // out: POWER_BOOT_SESSION_STANDBY_ACTIVATION_INFO
    PowerInternalSessionPowerState, // in: POWER_SESSION_POWER_STATE
    PowerInternalSessionTerminalInput, // 20
    PowerInternalSetWatchdog,
    PowerInternalPhysicalPowerButtonPressInfoAtBoot,
    PowerInternalExternalMonitorConnected,
    PowerInternalHighPrecisionBrightnessSettings,
    PowerInternalWinrtScreenToggle,
    PowerInternalPpmQosDisable,
    PowerInternalTransitionCheckpoint,
    PowerInternalInputControllerState,
    PowerInternalFirmwareResetReason,
    PowerInternalPpmSchedulerQosSupport, // out: POWER_INTERNAL_PROCESSOR_QOS_SUPPORT // 30
    PowerInternalBootStatGet,
    PowerInternalBootStatSet,
    PowerInternalCallHasNotReturnedWatchdog,
    PowerInternalBootStatCheckIntegrity,
    PowerInternalBootStatRestoreDefaults, // in: void
    PowerInternalHostEsStateUpdate, // in: POWER_INTERNAL_HOST_ENERGY_SAVER_STATE
    PowerInternalGetPowerActionState, // out: ULONG
    PowerInternalBootStatUnlock,
    PowerInternalWakeOnVoiceState,
    PowerInternalDeepSleepBlock, // 40
    PowerInternalIsPoFxDevice,
    PowerInternalPowerTransitionExtensionAtBoot,
    PowerInternalProcessorBrandedFrequency, // in: POWER_INTERNAL_PROCESSOR_BRANDED_FREQENCY_INPUT, out: POWER_INTERNAL_PROCESSOR_BRANDED_FREQENCY_OUTPUT
    PowerInternalTimeBrokerExpirationReason,
    PowerInternalNotifyUserShutdownStatus,
    PowerInternalPowerRequestTerminalCoreWindow,
    PowerInternalProcessorIdleVeto,
    PowerInternalPlatformIdleVeto,
    PowerInternalIsLongPowerButtonBugcheckEnabled,
    PowerInternalAutoChkCausedReboot, // 50
    PowerInternalSetWakeAlarmOverride,

    PowerInternalDirectedFxAddTestDevice = 53,
    PowerInternalDirectedFxRemoveTestDevice,

    PowerInternalDirectedFxSetMode = 56,
    PowerInternalRegisterPowerPlane,
    PowerInternalSetDirectedDripsFlags,
    PowerInternalClearDirectedDripsFlags,
    PowerInternalRetrieveHiberFileResumeContext, // 60
    PowerInternalReadHiberFilePage,
    PowerInternalLastBootSucceeded, // out: BOOLEAN
    PowerInternalQuerySleepStudyHelperRoutineBlock,
    PowerInternalDirectedDripsQueryCapabilities,
    PowerInternalClearConstraints,
    PowerInternalSoftParkVelocityEnabled,
    PowerInternalQueryIntelPepCapabilities,
    PowerInternalGetSystemIdleLoopEnablement, // since WIN11
    PowerInternalGetVmPerfControlSupport,
    PowerInternalGetVmPerfControlConfig, // 70
    PowerInternalSleepDetailedDiagUpdate,
    PowerInternalProcessorClassFrequencyBandsStats,
    PowerInternalHostGlobalUserPresenceStateUpdate,
    PowerInternalCpuNodeIdleIntervalStats,
    PowerInternalClassIdleIntervalStats,
    PowerInternalCpuNodeConcurrencyStats,
    PowerInternalClassConcurrencyStats,
    PowerInternalQueryProcMeasurementCapabilities,
    PowerInternalQueryProcMeasurementValues,
    PowerInternalPrepareForSystemInitiatedReboot, // 80
    PowerInternalGetAdaptiveSessionState,
    PowerInternalSetConsoleLockedState,
    PowerInternalOverrideSystemInitiatedRebootState,
    PowerInternalFanImpactStats,
    PowerInternalFanRpmBuckets,
    PowerInternalPowerBootAppDiagInfo, // out: POWER_INTERNAL_BOOTAPP_DIAGNOSTIC
    PowerInternalUnregisterShutdownNotification, // since 22H1
    PowerInternalManageTransitionStateRecord,
    PowerInternalGetAcpiTimeAndAlarmCapabilities, // since 22H2
    PowerInternalSuspendResumeRequest,
    PowerInternalEnergyEstimationInfo, // since 23H2
    PowerInternalProvSocIdentifierOperation, // since 24H2
    PowerInternalGetVmPerfPrioritySupport,
    PowerInternalGetVmPerfPriorityConfig,
    PowerInternalNotifyWin32kPowerRequestQueued,
    PowerInternalNotifyWin32kPowerRequestCompleted,
    PowerInformationInternalMaximum
} POWER_INFORMATION_LEVEL_INTERNAL;

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

typedef struct _POWER_INFORMATION_INTERNAL_HEADER
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INFORMATION_INTERNAL_HEADER, *PPOWER_INFORMATION_INTERNAL_HEADER;

typedef struct _POWER_USER_ABSENCE_PREDICTION
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    LARGE_INTEGER ReturnTime;
} POWER_USER_ABSENCE_PREDICTION, *PPOWER_USER_ABSENCE_PREDICTION;

typedef struct _POWER_USER_ABSENCE_PREDICTION_CAPABILITY
{
    BOOLEAN AbsencePredictionCapability;
} POWER_USER_ABSENCE_PREDICTION_CAPABILITY, *PPOWER_USER_ABSENCE_PREDICTION_CAPABILITY;

// rev
typedef struct _POWER_PROCESSOR_LATENCY_HINT
{
    POWER_INFORMATION_INTERNAL_HEADER PowerInformationInternalHeader;
    ULONG Type;
} POWER_PROCESSOR_LATENCY_HINT, *PPOWER_PROCESSOR_LATENCY_HINT;

// rev
typedef struct _POWER_STANDBY_NETWORK_REQUEST
{
    POWER_INFORMATION_INTERNAL_HEADER PowerInformationInternalHeader;
    BOOLEAN Active;
} POWER_STANDBY_NETWORK_REQUEST, *PPOWER_STANDBY_NETWORK_REQUEST;

// rev
typedef struct _POWER_SET_BACKGROUND_TASK_STATE
{
    POWER_INFORMATION_INTERNAL_HEADER PowerInformationInternalHeader;
    BOOLEAN Engaged;
} POWER_SET_BACKGROUND_TASK_STATE, *PPOWER_SET_BACKGROUND_TASK_STATE;

// rev
typedef struct _POWER_BOOT_SESSION_STANDBY_ACTIVATION_INFO
{
    ULONG StandbyTotalTime;
    ULONG DripsTotalTime;
    ULONG ActivatorClientTotalActiveTime;
    ULONG PerActivatorClientTotalActiveTime[98];
} POWER_BOOT_SESSION_STANDBY_ACTIVATION_INFO, *PPOWER_BOOT_SESSION_STANDBY_ACTIVATION_INFO;

// rev
typedef struct _POWER_SESSION_POWER_STATE
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    ULONG SessionId;
    BOOLEAN On;
    BOOLEAN IsConsole;
    POWER_MONITOR_REQUEST_REASON RequestReason;
} POWER_SESSION_POWER_STATE, *PPOWER_SESSION_POWER_STATE;

// rev
typedef struct _POWER_INTERNAL_PROCESSOR_QOS_SUPPORT
{
    BOOLEAN QosSupportedAndConfigured;
    BOOLEAN SchedulerDirectedPerfStatesSupported;
    BOOLEAN QosGroupPolicyDisable;
} POWER_INTERNAL_PROCESSOR_QOS_SUPPORT, *PPOWER_INTERNAL_PROCESSOR_QOS_SUPPORT;

// rev
typedef struct _POWER_INTERNAL_HOST_ENERGY_SAVER_STATE
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    BOOLEAN EsEnabledOnHost;
} POWER_INTERNAL_HOST_ENERGY_SAVER_STATE, *PPOWER_INTERNAL_HOST_ENERGY_SAVER_STATE;

// rev
typedef struct _POWER_INTERNAL_PROCESSOR_BRANDED_FREQENCY_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    PROCESSOR_NUMBER ProcessorNumber; // ULONG_MAX
} POWER_INTERNAL_PROCESSOR_BRANDED_FREQENCY_INPUT, *PPOWER_INTERNAL_PROCESSOR_BRANDED_FREQENCY_INPUT;

// rev
typedef struct _POWER_INTERNAL_PROCESSOR_BRANDED_FREQENCY_OUTPUT
{
    ULONG Version;
    ULONG NominalFrequency; // if (Domain) Prcb->PowerState.CheckContext.Domain.NominalFrequency else Prcb->MHz
} POWER_INTERNAL_PROCESSOR_BRANDED_FREQENCY_OUTPUT, *PPOWER_INTERNAL_PROCESSOR_BRANDED_FREQENCY_OUTPUT;

// rev
typedef struct _POWER_INTERNAL_BOOTAPP_DIAGNOSTIC
{
    ULONG BootAppErrorDiagCode; // bcdedit last status
    ULONG BootAppFailureStatus; // bcdedit last status
} POWER_INTERNAL_BOOTAPP_DIAGNOSTIC, *PPOWER_INTERNAL_BOOTAPP_DIAGNOSTIC;

#if (PHNT_MODE != PHNT_MODE_KERNEL)

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

#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetThreadExecutionState(
    _In_ EXECUTION_STATE NewFlags, // ES_* flags
    _Out_ EXECUTION_STATE *PreviousFlags
    );

#if (PHNT_VERSION < PHNT_WIN7)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRequestWakeupLatency(
    _In_ LATENCY_TIME latency
    );
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtInitiatePowerAction(
    _In_ POWER_ACTION SystemAction,
    _In_ SYSTEM_POWER_STATE LightestSystemState,
    _In_ ULONG Flags, // POWER_ACTION_* flags
    _In_ BOOLEAN Asynchronous
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSystemPowerState(
    _In_ POWER_ACTION SystemAction,
    _In_ SYSTEM_POWER_STATE LightestSystemState,
    _In_ ULONG Flags // POWER_ACTION_* flags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetDevicePowerState(
    _In_ HANDLE Device,
    _Out_ PDEVICE_POWER_STATE State
    );

NTSYSCALLAPI
BOOLEAN
NTAPI
NtIsSystemResumeAutomatic(
    VOID
    );

#endif
