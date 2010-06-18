#ifndef NTPOAPI_H
#define NTPOAPI_H

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

#if (PHNT_VERSION >= PHNT_VISTA)
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
            ULONG Reserved2 : 10;
        };
        ULONG ContextAsUlong;
    };
} SYSTEM_POWER_STATE_CONTEXT, *PSYSTEM_POWER_STATE_CONTEXT;
#endif

#if (PHNT_VERSION >= PHNT_WIN7)
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
            PUNICODE_STRING __field_ecount(StringCount) ReasonStrings;
        };
        UNICODE_STRING SimpleString;
    };
} COUNTED_REASON_CONTEXT, *PCOUNTED_REASON_CONTEXT;
#endif

typedef enum
{
    PowerStateSleeping1 = 0,
    PowerStateSleeping2 = 1,
    PowerStateSleeping3 = 2,
    PowerStateSleeping4 = 3,
    PowerStateSleeping4Firmware = 4,
    PowerStateShutdownReset = 5,
    PowerStateShutdownOff = 6,
    PowerStateMaximum = 7
} POWER_STATE_HANDLER_TYPE, *PPOWER_STATE_HANDLER_TYPE;

typedef NTSTATUS (NTAPI *PENTER_STATE_SYSTEM_HANDLER)(
    __in PVOID SystemContext
    );

typedef NTSTATUS (NTAPI *PENTER_STATE_HANDLER)(
    __in PVOID Context,
    __in_opt PENTER_STATE_SYSTEM_HANDLER SystemHandler,
    __in PVOID SystemContext,
    __in LONG NumberProcessors,
    __in volatile PLONG Number
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
    __in POWER_STATE_HANDLER_TYPE State,
    __in PVOID Context,
    __in BOOLEAN Entering
    );

typedef struct _POWER_STATE_NOTIFY_HANDLER
{
    PENTER_STATE_NOTIFY_HANDLER Handler;
    PVOID Context;
} POWER_STATE_NOTIFY_HANDLER, *PPOWER_STATE_NOTIFY_HANDLER;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPowerInformation(
    __in POWER_INFORMATION_LEVEL InformationLevel,
    __in_bcount_opt(InputBufferLength) PVOID InputBuffer,
    __in ULONG InputBufferLength,
    __out_bcount_opt(OutputBufferLength) PVOID OutputBuffer,
    __in ULONG OutputBufferLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetThreadExecutionState(
    __in EXECUTION_STATE esFlags, // ES_* flags
    __out EXECUTION_STATE *PreviousFlags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRequestWakeupLatency(
    __in LATENCY_TIME latency
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtInitiatePowerAction(
    __in POWER_ACTION SystemAction,
    __in SYSTEM_POWER_STATE MinSystemState,
    __in ULONG Flags, // POWER_ACTION_* flags
    __in BOOLEAN Asynchronous
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSystemPowerState(
    __in POWER_ACTION SystemAction,
    __in SYSTEM_POWER_STATE MinSystemState,
    __in ULONG Flags // POWER_ACTION_* flags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetDevicePowerState(
    __in HANDLE Device,
    __out DEVICE_POWER_STATE *State
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRequestDeviceWakeup(
    __in HANDLE Device
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCancelDeviceWakeupRequest(
    __in HANDLE Device
    );

NTSYSCALLAPI
BOOLEAN
NTAPI
NtIsSystemResumeAutomatic();

#endif
