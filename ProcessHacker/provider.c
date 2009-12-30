#include <ph.h>

VOID PhInitializeProviderThread(
    __out PPH_PROVIDER_THREAD ProviderThread,
    __in ULONG Interval
    )
{
    ProviderThread->ThreadHandle = NULL;
    ProviderThread->TimerHandle = NULL;
    ProviderThread->Interval = Interval;
    ProviderThread->State = ProviderThreadStopped;

    PhInitializeMutex(&ProviderThread->Mutex);
    InitializeListHead(&ProviderThread->ListHead);
}

VOID PhDeleteProviderThread(
    __inout PPH_PROVIDER_THREAD ProviderThread
    )
{
    PhDeleteMutex(&ProviderThread->Mutex);
}

NTSTATUS NTAPI PhpProviderThreadStart(
    __in PVOID Parameter
    )
{
    PPH_PROVIDER_THREAD providerThread = (PPH_PROVIDER_THREAD)Parameter;
    PLIST_ENTRY listEntry;
    PPH_PROVIDER_REGISTRATION registration;
    LIST_ENTRY tempListHead;

    while (providerThread->State != ProviderThreadStopping)
    {
        // Keep removing and executing providers from the list 
        // until there are no more. Each removed provider will 
        // be placed on the temporary list.
        // After this is done, all providers on the temporary 
        // list will be re-added to the list again.
        //
        // The key to this working safely with the other functions 
        // (boost, register, unregister) is that at all times 
        // when the mutex is not acquired every single provider 
        // must be in a list (main list or the temp list).

        InitializeListHead(&tempListHead);

        PhAcquireMutex(&providerThread->Mutex);

        // Main loop.

        while (
            (listEntry = RemoveHeadList(&providerThread->ListHead)) !=
            &providerThread->ListHead
            )
        {
            registration = CONTAINING_RECORD(listEntry, PH_PROVIDER_REGISTRATION, ListEntry);

            // Add the provider to the temp list.
            InsertTailList(&tempListHead, listEntry);

            if (registration->Unregistering)
                continue;

            PhReleaseMutex(&providerThread->Mutex);
            registration->Function();
            PhAcquireMutex(&providerThread->Mutex);
        }

        // Re-add the items in the temp list to the main list.

        while ((listEntry = RemoveHeadList(&tempListHead)) != &tempListHead)
            InsertTailList(&providerThread->ListHead, listEntry);

        PhReleaseMutex(&providerThread->Mutex);

        // Perform an alertable wait so we can be woken up by 
        // a provider boost or someone telling us to terminate.
        WaitForSingleObject(providerThread->TimerHandle, INFINITE);
    }

    return STATUS_SUCCESS;
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
    NtAlertThread(ProviderThread->ThreadHandle); // wake it up
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

        interval.QuadPart = -(LONGLONG)Interval * PH_TIMEOUT_TO_MS;

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

VOID PhBoostProvider(
    __inout PPH_PROVIDER_THREAD ProviderThread,
    __inout PPH_PROVIDER_REGISTRATION Registration
    )
{
    if (Registration->Unregistering)
        return;

    // Simply move to the provider to the front of the list. 
    // This works even if the provider is currently in the temp list.
    PhAcquireMutex(&ProviderThread->Mutex);
    RemoveEntryList(&Registration->ListEntry);
    InsertHeadList(&ProviderThread->ListHead, &Registration->ListEntry);
    PhReleaseMutex(&ProviderThread->Mutex);

    // Wake up the thread.
    NtAlertThread(ProviderThread->ThreadHandle);
}

VOID PhRegisterProvider(
    __inout PPH_PROVIDER_THREAD ProviderThread,
    __in PPH_PROVIDER_FUNCTION Function,
    __out PPH_PROVIDER_REGISTRATION Registration
    )
{
    Registration->Function = Function;
    Registration->Unregistering = FALSE;

    PhAcquireMutex(&ProviderThread->Mutex);
    InsertTailList(&ProviderThread->ListHead, &Registration->ListEntry);
    PhReleaseMutex(&ProviderThread->Mutex);
}

VOID PhUnregisterProvider(
    __inout PPH_PROVIDER_THREAD ProviderThread,
    __inout PPH_PROVIDER_REGISTRATION Registration
    )
{
    Registration->Unregistering = TRUE;

    // There are two possibilities for removal:
    // 1. The provider is removed while the thread is 
    //    not in the main loop. This is the normal case.
    // 2. The provider is removed while the thread is 
    //    in the main loop. In that case the provider 
    //    will be removed from the temp list and so 
    //    it won't be re-added to the main list.

    PhAcquireMutex(&ProviderThread->Mutex);
    RemoveEntryList(&Registration->ListEntry);
    PhReleaseMutex(&ProviderThread->Mutex);
}
