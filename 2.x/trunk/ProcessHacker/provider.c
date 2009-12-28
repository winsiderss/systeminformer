#include <ph.h>

VOID PhInitializeProviderThread(
    __out PPH_PROVIDER_THREAD ProviderThread,
    __in ULONG Interval
    )
{
    ProviderThread->ThreadHandle = NULL;
    ProviderThread->TimerHandle = NULL;
    ProviderThread->Interval = Interval;
    PhInitializeCallback(&ProviderThread->RunEvent);
    PhInitializeFastLock(&ProviderThread->RunEventLock);
    ProviderThread->State = ProviderThreadStopped;
}

NTSTATUS NTAPI PhpProviderThreadStart(
    __in PVOID Parameter
    )
{
    PPH_PROVIDER_THREAD providerThread = (PPH_PROVIDER_THREAD)Parameter;

    while (providerThread->State != ProviderThreadStopping)
    {
        PH_CALLBACK_LIST CallbackList;

        PhAcquireFastLockShared(&providerThread->RunEventLock);
        PhCreateCallbackList(&providerThread->RunEvent, &CallbackList);
        PhReleaseFastLockShared(&providerThread->RunEventLock);

        PhInvokeCallbackList(&CallbackList, NULL);
        PhDeleteCallbackList(&CallbackList);

        WaitForSingleObject(providerThread->TimerHandle, INFINITE);
    }
}

VOID PhStartProviderThread(
    __inout PPH_PROVIDER_THREAD ProviderThread
    )
{
    if (ProviderThread->State != ProviderThreadStopped)
        return;

    // Create and set the timer.
    ProviderThread->TimerHandle = CreateWaitableTimer(NULL, FALSE, NULL);
    PhSetProviderThreadInterval(ProviderThread, ProviderThread->Interval);

    // Create and start the thread.
    ProviderThread->ThreadHandle = CreateThread(
        NULL,
        0,
        PhpProviderThreadStart,
        ProviderThread,
        0,
        NULL
        );

    ProviderThread->State = ProviderThreadRunning;
}

VOID PhStopProviderThread(
    __inout PPH_PROVIDER_THREAD ProviderThread
    )
{
    if (ProviderThread->State != ProviderThreadRunning)
        return;

    // Signal to the thread that we are shutting down, and 
    // wait for it to exit.
    ProviderThread->State = ProviderThreadStopping;
    WaitForSingleObject(ProviderThread->ThreadHandle, INFINITE);

    // Free resources.
    CloseHandle(ProviderThread->ThreadHandle);
    CloseHandle(ProviderThread->TimerHandle);
    ProviderThread->ThreadHandle = NULL;
    ProviderThread->TimerHandle = NULL;
    ProviderThread->State = ProviderThreadStopped;
}

VOID PhSetProviderThreadInterval(
    __inout PPH_PROVIDER_THREAD ProviderThread,
    __in ULONG Interval
    )
{
    ProviderThread->Interval = Interval;

    if (ProviderThread->TimerHandle)
    {
        LARGE_INTEGER interval;

        interval.QuadPart = Interval * PH_TIMEOUT_TO_MS;

        SetWaitableTimer(
            ProviderThread->TimerHandle,
            &interval,
            Interval,
            NULL,
            NULL,
            FALSE
            );
    }
}

VOID PhRegisterProvider(
    __inout PPH_PROVIDER_THREAD ProviderThread,
    __in PPH_CALLBACK_FUNCTION Function,
    __out PPH_CALLBACK_REGISTRATION Registration
    )
{
    PhAcquireFastLockExclusive(&ProviderThread->RunEventLock);
    PhRegisterCallback(&ProviderThread->RunEvent, Function, NULL, Registration);
    PhReleaseFastLockExclusive(&ProviderThread->RunEventLock);
}

VOID PhUnregisterProvider(
    __inout PPH_PROVIDER_THREAD ProviderThread,
    __inout PPH_CALLBACK_REGISTRATION Registration
    )
{
    PhAcquireFastLockExclusive(&ProviderThread->RunEventLock);
    PhUnregisterCallback(&ProviderThread->RunEvent, Registration);
    PhReleaseFastLockExclusive(&ProviderThread->RunEventLock);
}
