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
#define SystemPowerPolicyAc 0                           // in: SYSTEM_POWER_POLICY, out: SYSTEM_POWER_POLICY // GET: InputBuffer NULL. SET: InputBuffer not NULL.
#define SystemPowerPolicyDc 1                           // in: SYSTEM_POWER_POLICY, out: SYSTEM_POWER_POLICY
#define VerifySystemPolicyAc 2                          // in: SYSTEM_POWER_POLICY, out: SYSTEM_POWER_POLICY
#define VerifySystemPolicyDc 3                          // in: SYSTEM_POWER_POLICY, out: SYSTEM_POWER_POLICY
#define SystemPowerCapabilities 4                       // out: SYSTEM_POWER_CAPABILITIES
#define SystemBatteryState 5                            // out: SYSTEM_BATTERY_STATE
#define SystemPowerStateHandler 6                       // in: POWER_STATE_HANDLER // (kernel-mode only)
#define ProcessorStateHandler 7                         // in: PROCESSOR_STATE_HANDLER // (kernel-mode only)
#define SystemPowerPolicyCurrent 8                      // in: SYSTEM_POWER_POLICY
#define AdministratorPowerPolicy 9                      // in: ADMINISTRATOR_POWER_POLICY
#define SystemReserveHiberFile 10                       // in: BOOLEAN // (requires SeCreatePagefilePrivilege) // TRUE: hibernation file created. FALSE: hibernation file deleted.
#define ProcessorInformation 11                         // out: PROCESSOR_POWER_INFORMATION
#define SystemPowerInformation 12                       // out: SYSTEM_POWER_INFORMATION
#define ProcessorStateHandler2 13                       // in: PROCESSOR_STATE_HANDLER2 // not implemented
#define LastWakeTime 14                                 // out: ULONGLONG // InterruptTime
#define LastSleepTime 15                                // out: ULONGLONG // InterruptTime
#define SystemExecutionState 16                         // out: EXECUTION_STATE // NtSetThreadExecutionState
#define SystemPowerStateNotifyHandler 17                // in: POWER_STATE_NOTIFY_HANDLER // (kernel-mode only)
#define ProcessorPowerPolicyAc 18                       // in: PROCESSOR_POWER_POLICY // not implemented
#define ProcessorPowerPolicyDc 19                       // in: PROCESSOR_POWER_POLICY // not implemented
#define VerifyProcessorPowerPolicyAc 20                 // in: PROCESSOR_POWER_POLICY // not implemented
#define VerifyProcessorPowerPolicyDc 21                 // in: PROCESSOR_POWER_POLICY // not implemented
#define ProcessorPowerPolicyCurrent 22                  // in: PROCESSOR_POWER_POLICY // not implemented
#define SystemPowerStateLogging 23                      // in: SYSTEM_POWER_STATE_DISABLE_REASON[]
#define SystemPowerLoggingEntry 24                      // in: SYSTEM_POWER_LOGGING_ENTRY[] // (kernel-mode only)
#define SetPowerSettingValue 25                         // in: (kernel-mode only)
#define NotifyUserPowerSetting 26                       // not implemented
#define PowerInformationLevelUnused0 27                 // not implemented
#define SystemMonitorHiberBootPowerOff 28               // in: NULL (PowerMonitorOff)
#define SystemVideoState 29                             // out: MONITOR_DISPLAY_STATE
#define TraceApplicationPowerMessage 30                 // in: (kernel-mode only)
#define TraceApplicationPowerMessageEnd 31              // in: (kernel-mode only)
#define ProcessorPerfStates 32                          // in: (kernel-mode only)
#define ProcessorIdleStates 33                          // out: PROCESSOR_IDLE_STATES // (kernel-mode only)
#define ProcessorCap 34                                 // out: PROCESSOR_CAP // (kernel-mode only)
#define SystemWakeSource 35                             // out: POWER_WAKE_SOURCE_INFO
#define SystemHiberFileInformation 36                   // out: SYSTEM_HIBERFILE_INFORMATION
#define TraceServicePowerMessage 37
#define ProcessorLoad 38                                // in: PROCESSOR_LOAD (sets), in: PPROCESSOR_NUMBER (clears)
#define PowerShutdownNotification 39                    // in: (kernel-mode only)
#define MonitorCapabilities 40                          // in: (kernel-mode only)
#define SessionPowerInit 41                             // in: (kernel-mode only)
#define SessionDisplayState 42                          // in: (kernel-mode only)
#define PowerRequestCreate 43                           // in: COUNTED_REASON_CONTEXT, out: HANDLE
#define PowerRequestAction 44                           // in: POWER_REQUEST_ACTION
#define GetPowerRequestList 45                          // out: POWER_REQUEST_LIST
#define ProcessorInformationEx 46                       // in: USHORT ProcessorGroup, out: PROCESSOR_POWER_INFORMATION
#define NotifyUserModeLegacyPowerEvent 47               // in: (kernel-mode only)
#define GroupPark 48                                    // in: (debug-mode boot only)
#define ProcessorIdleDomains 49                         // in: (kernel-mode only)
#define WakeTimerList 50                                // out: WAKE_TIMER_INFO[]
#define SystemHiberFileSize 51                          // out: ULONG
#define ProcessorIdleStatesHv 52                        // in: (kernel-mode only)
#define ProcessorPerfStatesHv 53                        // in: (kernel-mode only)
#define ProcessorPerfCapHv 54                           // int: PROCESSOR_PERF_CAP_HV // (kernel-mode only)
#define ProcessorSetIdle 55                             // in: (debug-mode boot only)
#define LogicalProcessorIdling 56                       // in: (kernel-mode only)
#define UserPresence 57                                 // out: POWER_USER_PRESENCE // not implemented
#define PowerSettingNotificationName 58                 // in: ? (optional) // out: PWNF_STATE_NAME (RtlSubscribeWnfStateChangeNotification)
#define GetPowerSettingValue 59                         // in: GUID
#define IdleResiliency 60                               // out: POWER_IDLE_RESILIENCY
#define SessionRITState 61                              // out: POWER_SESSION_RIT_STATE
#define SessionConnectNotification 62                   // out: POWER_SESSION_WINLOGON
#define SessionPowerCleanup 63
#define SessionLockState 64                             // out: POWER_SESSION_WINLOGON
#define SystemHiberbootState 65                         // out: BOOLEAN // fast startup supported
#define PlatformInformation 66                          // out: BOOLEAN // connected standby supported
#define PdcInvocation 67                                // in: (kernel-mode only)
#define MonitorInvocation 68                            // in: (kernel-mode only)
#define FirmwareTableInformationRegistered 69           // in: (kernel-mode only)
#define SetShutdownSelectedTime 70                      // in: NULL
#define SuspendResumeInvocation 71                      // in: (kernel-mode only) // not implemented
#define PlmPowerRequestCreate 72                        // in: COUNTED_REASON_CONTEXT, out: HANDLE
#define ScreenOff 73                                    // in: NULL (PowerMonitorOff)
#define CsDeviceNotification 74                         // in: (kernel-mode only)
#define PlatformRole 75                                 // out: POWER_PLATFORM_ROLE
#define LastResumePerformance 76                        // out: RESUME_PERFORMANCE
#define DisplayBurst 77                                 // in: NULL (PowerMonitorOn)
#define ExitLatencySamplingPercentage 78                // in: NULL (ClearExitLatencySamplingPercentage), in: ULONG (SetExitLatencySamplingPercentage) (max 100)
#define RegisterSpmPowerSettings 79                     // in: (kernel-mode only)
#define PlatformIdleStates 80                           // in: (kernel-mode only)
#define ProcessorIdleVeto 81                            // in: (kernel-mode only) // deprecated
#define PlatformIdleVeto 82                             // in: (kernel-mode only) // deprecated
#define SystemBatteryStatePrecise 83                    // out: SYSTEM_BATTERY_STATE
#define ThermalEvent 84                                 // in: THERMAL_EVENT // PowerReportThermalEvent
#define PowerRequestActionInternal 85                   // in: POWER_REQUEST_ACTION_INTERNAL
#define BatteryDeviceState 86
#define PowerInformationInternal 87                     // in: POWER_INFORMATION_LEVEL_INTERNAL // PopPowerInformationInternal
#define ThermalStandby 88                               // in: NULL // shutdown with thermal standby as reason.
#define SystemHiberFileType 89                          // in: ULONG // zero ? reduced : full // powercfg.exe /h /type
#define PhysicalPowerButtonPress 90                     // in: BOOLEAN
#define QueryPotentialDripsConstraint 91                // in: (kernel-mode only)
#define EnergyTrackerCreate 92                          // in: POWER_INFORMATION_ENERGY_TRACKER_CREATE_INPUT, out: POWER_INFORMATION_ENERGY_TRACKER_CREATE_OUTPUT
#define EnergyTrackerQuery 93                           // in: POWER_INFORMATION_ENERGY_TRACKER_QUERY_INPUT, out: POWER_INFORMATION_ENERGY_TRACKER_QUERY_OUTPUT
#define UpdateBlackBoxRecorder 94                       // in: POWER_INFORMATION_BBR_UPDATE_REQUEST_INPUT
#define SessionAllowExternalDmaDevices 95               // in: POWER_SESSION_ALLOW_EXTERNAL_DMA_DEVICES
#define SendSuspendResumeNotification 96                // in: since WIN11
#define BlackBoxRecorderDirectAccessBuffer 97           // in: POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_INPUT, out: POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_OUTPUT // since WIN11
#define SystemPowerSourceState 98                       // in: since 25H2
#define PowerInformationLevelMaximum 99
#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

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

typedef struct _SYSTEM_HIBERFILE_INFORMATION
{
    ULONG NumberOfMcbPairs;
    LARGE_INTEGER Mcb[1];
} SYSTEM_HIBERFILE_INFORMATION, *PSYSTEM_HIBERFILE_INFORMATION;

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

//typedef struct _POWER_SESSION_WINLOGON
//{
//    ULONG SessionId; // the Win32k session identifier
//    BOOLEAN Console; // TRUE - for console session, FALSE - for remote session
//    BOOLEAN Locked; // TRUE - lock, FALSE - unlock
//} POWER_SESSION_WINLOGON, *PPOWER_SESSION_WINLOGON;

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

//typedef struct _NOTIFY_USER_POWER_SETTING
//{
//    GUID Guid;
//} NOTIFY_USER_POWER_SETTING, *PNOTIFY_USER_POWER_SETTING;

#define POWER_PERF_SCALE    100
#define PERF_LEVEL_TO_PERCENT(_x_) ((_x_ * 1000) / (POWER_PERF_SCALE * 10))
#define PERCENT_TO_PERF_LEVEL(_x_) ((_x_ * POWER_PERF_SCALE * 10) / 1000)
#define PO_REASON_STATE_STANDBY (PO_REASON_STATE_S1 | \
                                 PO_REASON_STATE_S2 | \
                                 PO_REASON_STATE_S3)

#define PO_REASON_STATE_ALL     (PO_REASON_STATE_STANDBY | \
                                 PO_REASON_STATE_S4 | \
                                 PO_REASON_STATE_S4FIRM)

typedef struct _SYSTEM_POWER_LOGGING_ENTRY
{
    ULONG Reason;
    ULONG States;
} SYSTEM_POWER_LOGGING_ENTRY, *PSYSTEM_POWER_LOGGING_ENTRY;

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
    LARGE_INTEGER DueTime;
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
//
//#define PROCESSOR_IDLESTATE_POLICY_COUNT 0x3
//
//typedef struct
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
    PowerInternalS0LowPowerIdleInfo,                            // out: POWER_S0_LOW_POWER_IDLE_INFO
    PowerInternalReapplyBrightnessSettings,
    PowerInternalUserAbsencePrediction,                         // out: POWER_USER_ABSENCE_PREDICTION
    PowerInternalUserAbsencePredictionCapability,               // out: POWER_USER_ABSENCE_PREDICTION_CAPABILITY
    PowerInternalPoProcessorLatencyHint,                        // out: POWER_PROCESSOR_LATENCY_HINT
    PowerInternalStandbyNetworkRequest,                         // out: POWER_STANDBY_NETWORK_REQUEST (requires PopNetBIServiceSid)
    PowerInternalDirtyTransitionInformation,                    // out: BOOLEAN
    PowerInternalSetBackgroundTaskState,                        // out: POWER_SET_BACKGROUND_TASK_STATE
    PowerInternalTtmOpenTerminal,                               // in: (requires SeShutdownPrivilege and terminalPowerManagement capability)
    PowerInternalTtmCreateTerminal,                             // in: (requires SeShutdownPrivilege and terminalPowerManagement capability) // 10
    PowerInternalTtmEvacuateDevices,                            // in: (requires SeShutdownPrivilege and terminalPowerManagement capability)
    PowerInternalTtmCreateTerminalEventQueue,                   // in: (requires SeShutdownPrivilege and terminalPowerManagement capability)
    PowerInternalTtmGetTerminalEvent,                           // in: (requires SeShutdownPrivilege and terminalPowerManagement capability)
    PowerInternalTtmSetDefaultDeviceAssignment,                 // in: (requires SeShutdownPrivilege and terminalPowerManagement capability)
    PowerInternalTtmAssignDevice,                               // in: (requires SeShutdownPrivilege and terminalPowerManagement capability)
    PowerInternalTtmSetDisplayState,                            // in: (requires SeShutdownPrivilege and terminalPowerManagement capability)
    PowerInternalTtmSetDisplayTimeouts,                         // in: (requires SeShutdownPrivilege and terminalPowerManagement capability)
    PowerInternalBootSessionStandbyActivationInformation,       // out: POWER_BOOT_SESSION_STANDBY_ACTIVATION_INFO
    PowerInternalSessionPowerState,                             // in: POWER_SESSION_POWER_STATE
    PowerInternalSessionTerminalInput,                          // in: POWER_INTERNAL_TERMINAL_CORE_WINDOW_INPUT // 20
    PowerInternalSetWatchdog,
    PowerInternalPhysicalPowerButtonPressInfoAtBoot,            // in: POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_INPUT, out: POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_OUTPUT
    PowerInternalExternalMonitorConnected,                      // in: POWER_INTERNAL_EXTERNAL_MONITOR_CONNECTED_INPUT
    PowerInternalHighPrecisionBrightnessSettings,               // in: POWER_INTERNAL_HIGH_PRECISION_BRIGHTNESS_SETTINGS_INPUT
    PowerInternalWinrtScreenToggle,                             // in: POWER_INTERNAL_WINRT_SCREEN_TOGGLE_INPUT
    PowerInternalPpmQosDisable,                                 // in: POWER_INTERNAL_PPM_QOS_DISABLE_INPUT
    PowerInternalTransitionCheckpoint,                          // in: POWER_INTERNAL_TRANSITION_CHECKPOINT_INPUT
    PowerInternalInputControllerState,
    PowerInternalFirmwareResetReason,                           // in: POWER_INTERNAL_FIRMWARE_RESET_REASON_INPUT, out: POWER_INTERNAL_FIRMWARE_RESET_REASON_OUTPUT
    PowerInternalPpmSchedulerQosSupport,                        // out: POWER_INTERNAL_PROCESSOR_QOS_SUPPORT // 30
    PowerInternalBootStatGet,                                   // in: POWER_INTERNAL_BOOTSTAT_GET_INPUT, out: (optional) POWER_INTERNAL_BOOTSTAT_GET_OUTPUT[EntryCount] or ULONG[EntryCount]
    PowerInternalBootStatSet,
    PowerInternalCallHasNotReturnedWatchdog,
    PowerInternalBootStatCheckIntegrity,                        // in: POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_INPUT, out: POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_OUTPUT
    PowerInternalBootStatRestoreDefaults,                       // in: void
    PowerInternalHostEsStateUpdate,                             // in: POWER_INTERNAL_HOST_ENERGY_SAVER_STATE
    PowerInternalGetPowerActionState,                           // out: ULONG
    PowerInternalBootStatUnlock,
    PowerInternalWakeOnVoiceState,                              // in: POWER_INTERNAL_WAKE_ON_VOICE_STATE_INPUT
    PowerInternalDeepSleepBlock,                                // in: POWER_INTERNAL_DEEP_SLEEP_BLOCK_INPUT // 40
    PowerInternalIsPoFxDevice,
    PowerInternalPowerTransitionExtensionAtBoot,
    PowerInternalProcessorBrandedFrequency,                     // in: POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT, out: POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT
    PowerInternalTimeBrokerExpirationReason,
    PowerInternalNotifyUserShutdownStatus,                      // in: POWER_INTERNAL_NOTIFY_USER_SHUTDOWN_STATUS_INPUT
    PowerInternalPowerRequestTerminalCoreWindow,
    PowerInternalProcessorIdleVeto,                             // out: PROCESSOR_IDLE_VETO
    PowerInternalPlatformIdleVeto,                              // out: PLATFORM_IDLE_VETO
    PowerInternalIsLongPowerButtonBugcheckEnabled,              // out: BOOLEAN
    PowerInternalAutoChkCausedReboot,                           // in: POWER_INTERNAL_AUTOCHK_CAUASED_REBOOT_INPUT, out: POWER_INTERNAL_AUTOCHK_CAUASED_REBOOT_OUTPUT // 50 
    PowerInternalSetWakeAlarmOverride,

    PowerInternalDirectedFxAddTestDevice = 53,
    PowerInternalDirectedFxRemoveTestDevice,

    PowerInternalDirectedFxSetMode = 56,
    PowerInternalRegisterPowerPlane,
    PowerInternalSetDirectedDripsFlags,
    PowerInternalClearDirectedDripsFlags,
    PowerInternalRetrieveHiberFileResumeContext,                // in: // 60
    PowerInternalReadHiberFilePage,                             // in: POWER_INTERNAL_READ_HIBERFILE_PAGE_INPUT, out: POWER_INTERNAL_READ_HIBERFILE_PAGE_OUTPUT
    PowerInternalLastBootSucceeded,                             // out: BOOLEAN
    PowerInternalQuerySleepStudyHelperRoutineBlock,
    PowerInternalDirectedDripsQueryCapabilities,
    PowerInternalClearConstraints,
    PowerInternalSoftParkVelocityEnabled,
    PowerInternalQueryIntelPepCapabilities,                     // in: POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_INPUT, out: POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_OUTPUT
    PowerInternalGetSystemIdleLoopEnablement,                   // in: POWER_INTERNAL_SYSTEM_IDLE_LOOP_ENABLEMENT_INPUT, out: POWER_INTERNAL_SYSTEM_IDLE_LOOP_ENABLEMENT_OUTPUT // since WIN11
    PowerInternalGetVmPerfControlSupport,                       // in: POWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_INPUT, out: POWER_INTERNAL_VM_PERF_CONTROL_SUPPORT_OUTPUT
    PowerInternalGetVmPerfControlConfig,                        // in: // 70
    PowerInternalSleepDetailedDiagUpdate,                       // in: POWER_INTERNAL_SLEEP_DETAILED_DIAG_UPDATE_INPUT
    PowerInternalProcessorClassFrequencyBandsStats,             // in: POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_INPUT, out: POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_OUTPUT[] * NumberOfProcessors
    PowerInternalHostGlobalUserPresenceStateUpdate,             // in: POWER_INTERNAL_HOST_GLOBAL_USER_PRESENCE_STATE_UPDATE_INPUT
    PowerInternalCpuNodeIdleIntervalStats,
    PowerInternalClassIdleIntervalStats,
    PowerInternalCpuNodeConcurrencyStats,
    PowerInternalClassConcurrencyStats,
    PowerInternalQueryProcMeasurementCapabilities,              // in: PROCESSOR_INTERNAL_QUERY_MEASUREMENT_CAPABILITIES
    PowerInternalQueryProcMeasurementValues,                    // in: PROCESSOR_INTERNAL_QUERY_MEASUREMENT_VALUES
    PowerInternalPrepareForSystemInitiatedReboot,               // in: // 80
    PowerInternalGetAdaptiveSessionState,                       // in: POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_INPUT, out: POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_OUTPUT
    PowerInternalSetConsoleLockedState,                         // in: POWER_INTERNAL_SET_CONSOLE_LOCKED_STATE_INPUT
    PowerInternalOverrideSystemInitiatedRebootState,
    PowerInternalFanImpactStats,
    PowerInternalFanRpmBuckets,
    PowerInternalPowerBootAppDiagInfo,                          // out: POWER_INTERNAL_BOOTAPP_DIAGNOSTIC
    PowerInternalUnregisterShutdownNotification,                // in: // since 22H1
    PowerInternalManageTransitionStateRecord,
    PowerInternalGetAcpiTimeAndAlarmCapabilities,               // in: POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_INPUT, out: POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_OUTPUT // since 22H2
    PowerInternalSuspendResumeRequest,                          // in: // 90
    PowerInternalEnergyEstimationInfo,                          // in: since 23H2
    PowerInternalProvSocIdentifierOperation,                    // in: since 24H2
    PowerInternalGetVmPerfPrioritySupport,                      // in: POWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_INPUT, out: POWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_OUTPUT
    PowerInternalGetVmPerfPriorityConfig,                       // in: POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_INPUT, out: POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_OUTPUT
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
typedef struct _POWER_INTERNAL_TERMINAL_CORE_WINDOW_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG SessionId;
    ULONG TerminalId;
    UCHAR InputType;
} POWER_INTERNAL_TERMINAL_CORE_WINDOW_INPUT, *PPOWER_INTERNAL_TERMINAL_CORE_WINDOW_INPUT;

// rev
typedef struct _POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_INPUT, *PPOWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_INPUT;

// rev
typedef struct _POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_OUTPUT
{
    UCHAR Buffer[64];
} POWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_OUTPUT, *PPOWER_INTERNAL_PHYSICAL_POWER_BUTTON_AT_BOOT_OUTPUT;

// rev
typedef struct _POWER_INTERNAL_EXTERNAL_MONITOR_CONNECTED_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    BOOLEAN Connected; // 1 = connected, 0 = disconnected
} POWER_INTERNAL_EXTERNAL_MONITOR_CONNECTED_INPUT, *PPOWER_INTERNAL_EXTERNAL_MONITOR_CONNECTED_INPUT;

// rev
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
typedef struct _POWER_INTERNAL_WINRT_SCREEN_TOGGLE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    BOOLEAN Toggle; // 1 = turn screen on, 0 = turn screen off
} POWER_INTERNAL_WINRT_SCREEN_TOGGLE_INPUT, *PPOWER_INTERNAL_WINRT_SCREEN_TOGGLE_INPUT;

// rev
typedef struct _POWER_INTERNAL_PPM_QOS_DISABLE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    BOOLEAN EnableDisable; // Non-zero to enable QoS disable, zero to disable
} POWER_INTERNAL_PPM_QOS_DISABLE_INPUT, *PPOWER_INTERNAL_PPM_QOS_DISABLE_INPUT;

// rev
typedef struct _POWER_INTERNAL_TRANSITION_CHECKPOINT_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG CheckpointId;
    ULONG CheckpointType;
} POWER_INTERNAL_TRANSITION_CHECKPOINT_INPUT, *PPOWER_INTERNAL_TRANSITION_CHECKPOINT_INPUT;

// rev
typedef struct _POWER_INTERNAL_FIRMWARE_RESET_REASON_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_FIRMWARE_RESET_REASON_INPUT, *PPOWER_INTERNAL_FIRMWARE_RESET_REASON_INPUT;

// rev
typedef struct _POWER_INTERNAL_FIRMWARE_RESET_REASON_OUTPUT
{
    ULONG ResetReasonCode;
    UCHAR DiagnosticData1[16];
    UCHAR DiagnosticData2[16];
    UCHAR Reserved[12];
} POWER_INTERNAL_FIRMWARE_RESET_REASON_OUTPUT, *PPOWER_INTERNAL_FIRMWARE_RESET_REASON_OUTPUT;

// rev
typedef struct _POWER_INTERNAL_PROCESSOR_QOS_SUPPORT
{
    BOOLEAN QosSupportedAndConfigured;
    BOOLEAN SchedulerDirectedPerfStatesSupported;
    BOOLEAN QosGroupPolicyDisable;
} POWER_INTERNAL_PROCESSOR_QOS_SUPPORT, *PPOWER_INTERNAL_PROCESSOR_QOS_SUPPORT;

typedef struct _RTL_BSD_ITEM RTL_BSD_ITEM, *PRTL_BSD_ITEM;

// rev
typedef struct _POWER_INTERNAL_BOOTSTAT_GET_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG EntryCount;
    ULONG Reserved;
    PRTL_BSD_ITEM Entries;
} POWER_INTERNAL_BOOTSTAT_GET_INPUT, *PPOWER_INTERNAL_BOOTSTAT_GET_INPUT;

// rev
typedef struct _POWER_INTERNAL_BOOTSTAT_GET_OUTPUT
{
    // If present, it receives the actual sizes of the data copied into each DataBuffer.
    ULONG Sizes[ANYSIZE_ARRAY]; // Array of sizes, one per entry
} POWER_INTERNAL_BOOTSTAT_GET_OUTPUT, *PPOWER_INTERNAL_BOOTSTAT_GET_OUTPUT;

// rev
typedef struct _POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    HANDLE BootStatHandle;
} POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_INPUT, *PPOWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_INPUT;

// rev
typedef struct _POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_OUTPUT
{
    BOOLEAN IntegrityOk;
} POWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_OUTPUT, *PPOWER_INTERNAL_BOOTSTAT_CHECK_INTEGRITY_OUTPUT;

// rev
typedef struct _POWER_INTERNAL_HOST_ENERGY_SAVER_STATE
{
    POWER_INFORMATION_INTERNAL_HEADER Header;
    BOOLEAN EsEnabledOnHost;
} POWER_INTERNAL_HOST_ENERGY_SAVER_STATE, *PPOWER_INTERNAL_HOST_ENERGY_SAVER_STATE;

// rev
typedef struct _POWER_INTERNAL_NOTIFY_USER_SHUTDOWN_STATUS_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    BOOLEAN ShutdownInitiated; //  1 = initiated, 0 = cancelled
} POWER_INTERNAL_NOTIFY_USER_SHUTDOWN_STATUS_INPUT, *PPOWER_INTERNAL_NOTIFY_USER_SHUTDOWN_STATUS_INPUT;

// rev
typedef struct _POWER_INTERNAL_AUTOCHK_CAUASED_REBOOT_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_AUTOCHK_CAUASED_REBOOT_INPUT, *PPOWER_INTERNAL_AUTOCHK_CAUASED_REBOOT_INPUT;

// rev
typedef struct _POWER_INTERNAL_READ_HIBERFILE_PAGE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG PageNumber;
} POWER_INTERNAL_READ_HIBERFILE_PAGE_INPUT, *PPOWER_INTERNAL_READ_HIBERFILE_PAGE_INPUT;

// rev
typedef struct _POWER_INTERNAL_READ_HIBERFILE_PAGE_OUTPUT
{
    UCHAR PageData[PAGE_SIZE];
} POWER_INTERNAL_READ_HIBERFILE_PAGE_OUTPUT, *PPOWER_INTERNAL_READ_HIBERFILE_PAGE_OUTPUT;

// rev
typedef struct _POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_INPUT, *PPOWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_INPUT;

// rev
typedef struct _POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_OUTPUT
{
    ULONG Capabilities[4];
} POWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_OUTPUT, *PPOWER_INTERNAL_QUERY_INTEL_PEP_CAPABILITIES_OUTPUT;

// rev
typedef struct _POWER_INTERNAL_AUTOCHK_CAUASED_REBOOT_OUTPUT
{
    BOOLEAN CausedReboot;
} POWER_INTERNAL_AUTOCHK_CAUASED_REBOOT_OUTPUT, *PPOWER_INTERNAL_AUTOCHK_CAUASED_REBOOT_OUTPUT;

// rev
typedef struct _POWER_INTERNAL_WAKE_ON_VOICE_STATE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    BOOLEAN Enabled; // 1 = enable Wake on Voice, 0 = disable
} POWER_INTERNAL_WAKE_ON_VOICE_STATE_INPUT, *PPOWER_INTERNAL_WAKE_ON_VOICE_STATE_INPUT;

// rev
typedef struct _POWER_INTERNAL_DEEP_SLEEP_BLOCK_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    BOOLEAN Block; // 1 = block deep sleep, 0 = unblock
} POWER_INTERNAL_DEEP_SLEEP_BLOCK_INPUT, *PPOWER_INTERNAL_DEEP_SLEEP_BLOCK_INPUT;

// rev
typedef struct _POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    PROCESSOR_NUMBER ProcessorNumber; // ULONG_MAX
} POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT, *PPOWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT;

#define POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_VERSION 1

// rev
typedef struct _POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT
{
    ULONG Version;
    ULONG NominalFrequency; // if (Domain) Prcb->PowerState.CheckContext.Domain.NominalFrequency else Prcb->MHz
} POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT, *PPOWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT;

// rev
typedef struct _PROCESSOR_IDLE_VETO
{
    ULONG Version;
    PROCESSOR_NUMBER ProcessorNumber;
    ULONG StateIndex;
    ULONG VetoReason;
    UCHAR Increment;
} PROCESSOR_IDLE_VETO, *PPROCESSOR_IDLE_VETO;

// rev
typedef struct _PLATFORM_IDLE_VETO
{
    ULONG Version;
    ULONG StateIndex;
    ULONG VetoReason;
    UCHAR Increment;
} PLATFORM_IDLE_VETO, *PPLATFORM_IDLE_VETO;

// rev
typedef struct _POWER_INTERNAL_SYSTEM_IDLE_LOOP_ENABLEMENT_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
} POWER_INTERNAL_SYSTEM_IDLE_LOOP_ENABLEMENT_INPUT, *PPOWER_INTERNAL_SYSTEM_IDLE_LOOP_ENABLEMENT_INPUT;

// rev
typedef struct _POWER_INTERNAL_SYSTEM_IDLE_LOOP_ENABLEMENT_OUTPUT
{
    ULONG IdleLoopEnabled;
} POWER_INTERNAL_SYSTEM_IDLE_LOOP_ENABLEMENT_OUTPUT, *PPOWER_INTERNAL_SYSTEM_IDLE_LOOP_ENABLEMENT_OUTPUT;

// rev
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
typedef struct _POWER_INTERNAL_SLEEP_DETAILED_DIAG_UPDATE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    BOOLEAN Enable;
} POWER_INTERNAL_SLEEP_DETAILED_DIAG_UPDATE_INPUT, *PPOWER_INTERNAL_SLEEP_DETAILED_DIAG_UPDATE_INPUT;

// rev
typedef struct _POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
} POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_INPUT, *PPOWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_INPUT;

// rev
typedef struct _POWER_INTERNAL_HOST_GLOBAL_USER_PRESENCE_STATE_UPDATE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    BOOLEAN UserPresent; // 1 if user is present, 0 otherwise
} POWER_INTERNAL_HOST_GLOBAL_USER_PRESENCE_STATE_UPDATE_INPUT, *PPOWER_INTERNAL_HOST_GLOBAL_USER_PRESENCE_STATE_UPDATE_INPUT;

#define PPM_PERF_BANKS_COUNT 2
#define PPM_PERF_BANDS_COUNT 48
#define PPM_PERF_METRICS_COUNT 3
#define PPM_PERF_BANDS_SIZE sizeof(PPM_PERF_BAND_ENTRY)
#define PPM_PERF_STATS_SIZE (PPM_PERF_BANDS_COUNT * PPM_PERF_BANDS_SIZE)
#define PPM_PERF_DELTA_OFFSET 0xF8 // 248 bytes

// rev
typedef struct _POWER_INTERNAL_PPM_PERF_FREQUENCY_BAND_STATS_BANK
{
    // Metric[0][0..47], Metric[1][0..47], Metric[2][0..47]
    ULONGLONG Metric[PPM_PERF_METRICS_COUNT][PPM_PERF_BANDS_COUNT];
} POWER_INTERNAL_PPM_PERF_FREQUENCY_BAND_STATS_BANK, PPOWER_INTERNAL_PM_PERF_FREQUENCY_BAND_STATS_BANK;

// rev
typedef struct _POWER_INTERNAL_PPM_PERF_FREQUENCY_BAND_STATS_OUT
{
    POWER_INTERNAL_PPM_PERF_FREQUENCY_BAND_STATS_BANK Bank[PPM_PERF_BANKS_COUNT];
} POWER_INTERNAL_PPM_PERF_FREQUENCY_BAND_STATS_OUT;

// rev
typedef struct _POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS
{
    ULONGLONG Counter[PPM_PERF_METRICS_COUNT];
} POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS, *PPOWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS;

// rev
typedef struct _POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_OUTPUT
{
    POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS Band[PPM_PERF_BANDS_COUNT];
} POWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_OUTPUT, *PPOWER_INTERNAL_PROCESSOR_CLASS_BAND_STATS_OUTPUT;

// rev
typedef struct _POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    ULONG SessionStateId;
    UCHAR Reserved[28];
} POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_INPUT, *PPOWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_INPUT;

// rev
typedef struct _POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_OUTPUT
{
    UCHAR Reserved[16];
} POWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_OUTPUT, *PPOWER_INTERNAL_GET_ADAPTIVE_SESSION_STATE_OUTPUT;

// rev
typedef struct _POWER_INTERNAL_SET_CONSOLE_LOCKED_STATE_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    BOOLEAN Locked; // 1 if console is locked, 0 if unlocked
} POWER_INTERNAL_SET_CONSOLE_LOCKED_STATE_INPUT, *PPOWER_INTERNAL_SET_CONSOLE_LOCKED_STATE_INPUT;

// rev
typedef struct _POWER_INTERNAL_BOOTAPP_DIAGNOSTIC
{
    ULONG BootAppErrorDiagCode; // bcdedit last status
    ULONG BootAppFailureStatus; // bcdedit last status
} POWER_INTERNAL_BOOTAPP_DIAGNOSTIC, *PPOWER_INTERNAL_BOOTAPP_DIAGNOSTIC;

// rev
typedef struct _POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Version;
    UCHAR Reserved[12];
} POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_INPUT, *PPOWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_INPUT;

// rev
typedef struct  _POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_OUTPUT
{
    UCHAR Capabilities[20];
} POWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_OUTPUT, *PPOWER_INTERNAL_GET_ACPI_TIME_AND_ALARM_CAPABILITIES_OUTPUT;

// rev
typedef struct _POWER_INTERNAL_SOC_IDENTIFIER_OPERATION_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Action;
    ULONG Domain;
} POWER_INTERNAL_SOC_IDENTIFIER_OPERATION_INPUT, *PPOWER_INTERNAL_SOC_IDENTIFIER_OPERATION_INPUT;

// rev
typedef struct _POWER_INTERNAL_SOC_IDENTIFIER_OPERATION_OUTPUT
{
    BOOLEAN VmThrottleSupportedAndConfigured;
    ULONG VmThrottlePriorityCount;
} POWER_INTERNAL_SOC_IDENTIFIER_OPERATION_OUTPUT, *PPOWER_INTERNAL_SOC_IDENTIFIER_OPERATION_OUTPUT;

// rev
typedef struct _POWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
} POWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_INPUT, *PPOWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_INPUT;

// rev
typedef struct _POWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_OUTPUT
{
    BOOLEAN VmThrottleSupportedAndConfigured;
    ULONG VmThrottlePriorityCount;
} POWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_OUTPUT, *PPOWER_INTERNAL_VMPERF_PRIORITY_SUPPORT_OUTPUT;

// rev
typedef struct _POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_INPUT
{
    POWER_INFORMATION_LEVEL_INTERNAL InternalType;
    ULONG Action;
    ULONG Domain;
} POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_INPUT, *PPOWER_INTERNAL_VMPERF_PRIORITY_CONFIG_INPUT;

// rev
typedef struct _POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_OUTPUT
{
    BOOLEAN VmThrottleSupportedAndConfigured;
    ULONG VmThrottlePriorityCount;
} POWER_INTERNAL_VMPERF_PRIORITY_CONFIG_OUTPUT, *PPOWER_INTERNAL_VMPERF_PRIORITY_CONFIG_OUTPUT;

// rev
typedef struct _POWER_INFORMATION_ENERGY_TRACKER_CREATE_INPUT
{
    ULONG Version;
    ULONG Flags;
    ULONG Reserved;
} POWER_INFORMATION_ENERGY_TRACKER_CREATE_INPUT, *PPOWER_INFORMATION_ENERGY_TRACKER_CREATE_INPUT;

// rev
typedef struct _POWER_INFORMATION_ENERGY_TRACKER_CREATE_OUTPUT
{
    HANDLE QueryHandle;
} POWER_INFORMATION_ENERGY_TRACKER_CREATE_OUTPUT, *PPOWER_INFORMATION_ENERGY_TRACKER_CREATE_OUTPUT;

// rev
typedef struct _POWER_INFORMATION_ENERGY_TRACKER_QUERY_INPUT
{
    HANDLE QueryHandle;
} POWER_INFORMATION_ENERGY_TRACKER_QUERY_INPUT, *PPOWER_INFORMATION_ENERGY_TRACKER_QUERY_INPUT;

// rev
typedef struct _POWER_INFORMATION_ENERGY_TRACKER_QUERY_OUTPUT
{
    ULONG Version;
    ULONG DataType;
    ULONG DataSize;
    // BYTE Data[...];  // optional payload follows
} POWER_INFORMATION_ENERGY_TRACKER_QUERY_OUTPUT, *PPOWER_INFORMATION_ENERGY_TRACKER_QUERY_OUTPUT;

// rev
typedef struct _POWER_INFORMATION_BBR_UPDATE_REQUEST_INPUT
{
    ULONG Version;
    ULONG Flags;
    ULONGLONG Reserved0; // must be zero
    ULONGLONG Reserved1; // must be zero
    ULONGLONG Reserved2; // must be zero
} POWER_INFORMATION_BBR_UPDATE_REQUEST_INPUT, *PPOWER_INFORMATION_BBR_UPDATE_REQUEST_INPUT;

// rev
typedef struct _POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_INPUT
{
    ULONG Version;
    ULONG Flags;
    ULONGLONG Offset;
    ULONGLONG Length;
} POWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_INPUT, *PPOWER_INFORMATION_BBR_DIRECT_ACCESS_REQUEST_INPUT;

// rev
typedef struct _POWER_INFORMATION_BBR_DIRECT_ACCESS_RESPONSE_OUTPUT
{
    PVOID UserMappingBase;
    ULONGLONG MappingSize;
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
 * The NtSetThreadExecutionState routine Requests the system resume latency.
 *
 * \param latency The desired latency time.
 * \return Successful or errant status.
 */
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
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSystemPowerState(
    _In_ POWER_ACTION SystemAction,
    _In_ SYSTEM_POWER_STATE LightestSystemState,
    _In_ ULONG Flags // POWER_ACTION_* flags
    );

/**
 * The NtSetSystemPowerState routine retrieves the current power state of the specified device.
 *
 * \param Device A handle to an object on the device, such as a file or socket, or a handle to the device itself.
 * \param State A pointer to the variable that receives the power state.
 * \return Successful or errant status.
 * \remarks An application can use NtGetDevicePowerState to determine whether a device is in the working state or a low-power state.
 * If the device is in a low-power state, accessing the device may cause it to either queue or fail any I/O requests, or transition the device into the working state.
 * The exact behavior depends on the implementation of the device.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetDevicePowerState(
    _In_ HANDLE Device,
    _Out_ PDEVICE_POWER_STATE State
    );

/**
 * The NtIsSystemResumeAutomatic routine Checks if the system resume is automatic.

 * \return BOOLEAN TRUE if the system resume is automatic, FALSE otherwise.
 */
NTSYSCALLAPI
BOOLEAN
NTAPI
NtIsSystemResumeAutomatic(
    VOID
    );

#endif // _NTPOAPI_H
