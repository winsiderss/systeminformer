#ifndef _PH_PROVIDER_H
#define _PH_PROVIDER_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(DEBUG)
extern PPH_LIST PhDbgProviderList;
extern PH_QUEUED_LOCK PhDbgProviderListLock;
#endif

typedef enum _PH_PROVIDER_THREAD_STATE
{
    ProviderThreadRunning,
    ProviderThreadStopped,
    ProviderThreadStopping
} PH_PROVIDER_THREAD_STATE;

typedef VOID (NTAPI *PPH_PROVIDER_FUNCTION)(
    _In_ PVOID Object
    );

struct _PH_PROVIDER_THREAD;
typedef struct _PH_PROVIDER_THREAD *PPH_PROVIDER_THREAD;

typedef struct _PH_PROVIDER_REGISTRATION
{
    LIST_ENTRY ListEntry;
    PPH_PROVIDER_THREAD ProviderThread;
    PPH_PROVIDER_FUNCTION Function;
    PVOID Object;
    ULONG RunId;
    BOOLEAN Enabled;
    BOOLEAN Unregistering;
    BOOLEAN Boosting;
} PH_PROVIDER_REGISTRATION, *PPH_PROVIDER_REGISTRATION;

typedef struct _PH_PROVIDER_THREAD
{
    HANDLE ThreadHandle;
    HANDLE TimerHandle;
    ULONG Interval;
    PH_PROVIDER_THREAD_STATE State;

    PH_QUEUED_LOCK Lock;
    LIST_ENTRY ListHead;
    ULONG BoostCount;
} PH_PROVIDER_THREAD, *PPH_PROVIDER_THREAD;

PHLIBAPI
VOID
NTAPI
PhInitializeProviderThread(
    _Out_ PPH_PROVIDER_THREAD ProviderThread,
    _In_ ULONG Interval
    );

PHLIBAPI
VOID
NTAPI
PhDeleteProviderThread(
    _Inout_ PPH_PROVIDER_THREAD ProviderThread
    );

PHLIBAPI
VOID
NTAPI
PhStartProviderThread(
    _Inout_ PPH_PROVIDER_THREAD ProviderThread
    );

PHLIBAPI
VOID
NTAPI
PhStopProviderThread(
    _Inout_ PPH_PROVIDER_THREAD ProviderThread
    );

PHLIBAPI
VOID
NTAPI
PhSetIntervalProviderThread(
    _Inout_ PPH_PROVIDER_THREAD ProviderThread,
    _In_ ULONG Interval
    );

PHLIBAPI
VOID
NTAPI
PhRegisterProvider(
    _Inout_ PPH_PROVIDER_THREAD ProviderThread,
    _In_ PPH_PROVIDER_FUNCTION Function,
    _In_opt_ PVOID Object,
    _Out_ PPH_PROVIDER_REGISTRATION Registration
    );

PHLIBAPI
VOID
NTAPI
PhUnregisterProvider(
    _Inout_ PPH_PROVIDER_REGISTRATION Registration
    );

PHLIBAPI
BOOLEAN
NTAPI
PhBoostProvider(
    _Inout_ PPH_PROVIDER_REGISTRATION Registration,
    _Out_opt_ PULONG FutureRunId
    );

PHLIBAPI
ULONG
NTAPI
PhGetRunIdProvider(
    _In_ PPH_PROVIDER_REGISTRATION Registration
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetEnabledProvider(
    _In_ PPH_PROVIDER_REGISTRATION Registration
    );

PHLIBAPI
VOID
NTAPI
PhSetEnabledProvider(
    _Inout_ PPH_PROVIDER_REGISTRATION Registration,
    _In_ BOOLEAN Enabled
    );

#ifdef __cplusplus
}
#endif

#endif
