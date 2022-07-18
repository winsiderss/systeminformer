/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *
 */

/*
 * Provider objects allow a function to be executed periodically. This is managed by a
 * synchronization timer object which is signaled periodically. The use of a timer object as opposed
 * to a simple sleep call means that the length of time a provider function takes to execute has no
 * effect on the interval between runs.
 *
 * In contrast to callback objects, the context passed to provider functions must be
 * reference-counted objects. This means that it is not guaranteed that the function will not be in
 * execution after the unregister operation is complete. However, the since the context object is
 * reference-counted, there are no safety issues.
 *
 * Providers can be boosted, which causes them to be run immediately ignoring the interval. This is
 * separate to the periodic runs, and does not cause the next periodic run to be missed. Providers,
 * even when boosted, always run on the same provider thread. The other option would be to have the
 * boosting thread run the provider function directly, which would involve unnecessary blocking and
 * synchronization.
 */

#include <ph.h>
#include <provider.h>

#ifdef DEBUG
PPH_LIST PhDbgProviderList;
PH_QUEUED_LOCK PhDbgProviderListLock = PH_QUEUED_LOCK_INIT;
#endif

/**
 * Initializes a provider thread.
 *
 * \param ProviderThread A pointer to a provider thread object.
 * \param Interval The interval between each run, in milliseconds.
 */
VOID PhInitializeProviderThread(
    _Out_ PPH_PROVIDER_THREAD ProviderThread,
    _In_ ULONG Interval
    )
{
    ProviderThread->ThreadHandle = NULL;
    ProviderThread->TimerHandle = NULL;
    ProviderThread->Interval = Interval;
    ProviderThread->State = ProviderThreadStopped;

    PhInitializeQueuedLock(&ProviderThread->Lock);
    InitializeListHead(&ProviderThread->ListHead);
    ProviderThread->BoostCount = 0;

#ifdef DEBUG
    PhAcquireQueuedLockExclusive(&PhDbgProviderListLock);
    if (!PhDbgProviderList)
        PhDbgProviderList = PhCreateList(4);
    PhAddItemList(PhDbgProviderList, ProviderThread);
    PhReleaseQueuedLockExclusive(&PhDbgProviderListLock);
#endif
}

/**
 * Frees resources used by a provider thread.
 *
 * \param ProviderThread A pointer to a provider thread object.
 */
VOID PhDeleteProviderThread(
    _Inout_ PPH_PROVIDER_THREAD ProviderThread
    )
{
#ifdef DEBUG
    ULONG index;
#endif
    // Nothing

#ifdef DEBUG
    PhAcquireQueuedLockExclusive(&PhDbgProviderListLock);
    if ((index = PhFindItemList(PhDbgProviderList, ProviderThread)) != -1)
        PhRemoveItemList(PhDbgProviderList, index);
    PhReleaseQueuedLockExclusive(&PhDbgProviderListLock);
#endif
}

NTSTATUS NTAPI PhpProviderThreadStart(
    _In_ PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    PPH_PROVIDER_THREAD providerThread = (PPH_PROVIDER_THREAD)Parameter;
    NTSTATUS status = STATUS_SUCCESS;
    PLIST_ENTRY listEntry;
    PPH_PROVIDER_REGISTRATION registration;
    PPH_PROVIDER_FUNCTION providerFunction;
    PVOID object;
    LIST_ENTRY tempListHead;

    PhInitializeAutoPool(&autoPool);

    while (providerThread->State != ProviderThreadStopping)
    {
        // Keep removing and executing providers from the list until there are no more. Each removed
        // provider will be placed on the temporary list. After this is done, all providers on the
        // temporary list will be re-added to the list again.
        //
        // The key to this working safely with the other functions (boost, register, unregister) is
        // that at all times when the mutex is not acquired every single provider must be in a list
        // (main list or the temp list).

        InitializeListHead(&tempListHead);

        PhAcquireQueuedLockExclusive(&providerThread->Lock);

        // Main loop.

        // We check the status variable for STATUS_ALERTED, which means that someone is requesting
        // that a provider be boosted. Note that if they alert this thread while we are not waiting
        // on the timer, when we do perform the wait it will return immediately with STATUS_ALERTED.

        while (TRUE)
        {
            if (status == STATUS_ALERTED)
            {
                // Check if we have any more providers to boost. Note that this always works because
                // boosted providers are always in front of normal providers. Therefore we will
                // never mistakenly boost normal providers.

                if (providerThread->BoostCount == 0)
                    break;
            }

            listEntry = RemoveHeadList(&providerThread->ListHead);

            if (listEntry == &providerThread->ListHead)
                break;

            registration = CONTAINING_RECORD(listEntry, PH_PROVIDER_REGISTRATION, ListEntry);

            // Add the provider to the temp list.
            InsertTailList(&tempListHead, listEntry);

            if (status != STATUS_ALERTED)
            {
                if (!registration->Enabled || registration->Unregistering)
                    continue;
            }
            else
            {
                // If we're boosting providers, we don't care if they are enabled or not. However,
                // we have to make sure any providers which are being unregistered get a chance to
                // fix the boost count.

                if (registration->Unregistering)
                {
                    PhReleaseQueuedLockExclusive(&providerThread->Lock);
                    PhAcquireQueuedLockExclusive(&providerThread->Lock);

                    continue;
                }
            }

            if (status == STATUS_ALERTED)
            {
                assert(registration->Boosting);
                registration->Boosting = FALSE;
                providerThread->BoostCount--;
            }

            providerFunction = registration->Function;
            object = registration->Object;

            if (object)
                PhReferenceObject(object);

            registration->RunId++;

            PhReleaseQueuedLockExclusive(&providerThread->Lock);
            providerFunction(object);
            PhDrainAutoPool(&autoPool);
            PhAcquireQueuedLockExclusive(&providerThread->Lock);

            if (object)
                PhDereferenceObject(object);
        }

        // Re-add the items in the temp list to the main list.

        while ((listEntry = RemoveHeadList(&tempListHead)) != &tempListHead)
        {
            registration = CONTAINING_RECORD(listEntry, PH_PROVIDER_REGISTRATION, ListEntry);

            // We must insert boosted providers at the front of the list in order to maintain the
            // condition that boosted providers are always in front of normal providers. This occurs
            // when the timer is signaled just before a boosting provider alerts our thread.
            if (!registration->Boosting)
                InsertTailList(&providerThread->ListHead, listEntry);
            else
                InsertHeadList(&providerThread->ListHead, listEntry);
        }

        PhReleaseQueuedLockExclusive(&providerThread->Lock);

        // Perform an alertable wait so we can be woken up by someone telling us to boost providers,
        // or to terminate.
        status = NtWaitForSingleObject(
            providerThread->TimerHandle,
            TRUE,
            NULL
            );
    }

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

/**
 * Starts a provider thread.
 *
 * \param ProviderThread A pointer to a provider thread object.
 */
VOID PhStartProviderThread(
    _Inout_ PPH_PROVIDER_THREAD ProviderThread
    )
{
    if (ProviderThread->State != ProviderThreadStopped)
        return;

    // Create and set the timer.
    NtCreateTimer(&ProviderThread->TimerHandle, TIMER_ALL_ACCESS, NULL, SynchronizationTimer);
    PhSetIntervalProviderThread(ProviderThread, ProviderThread->Interval);

    // Create and start the thread.
    PhCreateThreadEx(&ProviderThread->ThreadHandle, PhpProviderThreadStart, ProviderThread);
    ProviderThread->State = ProviderThreadRunning;
}

/**
 * Stops a provider thread.
 *
 * \param ProviderThread A pointer to a provider thread object.
 */
VOID PhStopProviderThread(
    _Inout_ PPH_PROVIDER_THREAD ProviderThread
    )
{
    if (ProviderThread->State != ProviderThreadRunning)
        return;

    // Signal to the thread that we are shutting down, and wait for it to exit.
    ProviderThread->State = ProviderThreadStopping;
    NtAlertThread(ProviderThread->ThreadHandle); // wake it up
    NtWaitForSingleObject(ProviderThread->ThreadHandle, FALSE, NULL);

    // Free resources.
    NtClose(ProviderThread->ThreadHandle);
    NtClose(ProviderThread->TimerHandle);
    ProviderThread->ThreadHandle = NULL;
    ProviderThread->TimerHandle = NULL;

    ProviderThread->State = ProviderThreadStopped;
}

/**
 * Sets the run interval for a provider thread.
 *
 * \param ProviderThread A pointer to a provider thread object.
 * \param Interval The interval between each run, in milliseconds.
 */
VOID PhSetIntervalProviderThread(
    _Inout_ PPH_PROVIDER_THREAD ProviderThread,
    _In_ ULONG Interval
    )
{
    ProviderThread->Interval = Interval;

    if (ProviderThread->TimerHandle)
    {
        LARGE_INTEGER interval;

        interval.QuadPart = -(LONGLONG)UInt32x32To64(Interval, PH_TIMEOUT_MS);
        NtSetTimer(ProviderThread->TimerHandle, &interval, NULL, NULL, FALSE, Interval, NULL);
    }
}

/**
 * Registers a provider with a provider thread.
 *
 * \param ProviderThread A pointer to a provider thread object.
 * \param Function The provider function.
 * \param Object A pointer to an object to pass to the provider function. The object must be managed
 * by the reference-counting system.
 * \param Registration A variable which receives registration information for the provider.
 *
 * \remarks The provider is initially disabled. Call PhSetEnabledProvider() to enable it.
 */
VOID PhRegisterProvider(
    _Inout_ PPH_PROVIDER_THREAD ProviderThread,
    _In_ PPH_PROVIDER_FUNCTION Function,
    _In_opt_ PVOID Object,
    _Out_ PPH_PROVIDER_REGISTRATION Registration
    )
{
    Registration->ProviderThread = ProviderThread;
    Registration->Function = Function;
    Registration->Object = Object;
    Registration->RunId = 0;
    Registration->Enabled = FALSE;
    Registration->Unregistering = FALSE;
    Registration->Boosting = FALSE;

    if (Object)
        PhReferenceObject(Object);

    PhAcquireQueuedLockExclusive(&ProviderThread->Lock);
    InsertTailList(&ProviderThread->ListHead, &Registration->ListEntry);
    PhReleaseQueuedLockExclusive(&ProviderThread->Lock);
}

/**
 * Unregisters a provider.
 *
 * \param Registration A pointer to the registration object for a provider.
 *
 * \remarks The provider function may still be in execution once this function returns.
 */
VOID PhUnregisterProvider(
    _Inout_ PPH_PROVIDER_REGISTRATION Registration
    )
{
    PPH_PROVIDER_THREAD providerThread;

    providerThread = Registration->ProviderThread;

    Registration->Unregistering = TRUE;

    // There are two possibilities for removal:
    // 1. The provider is removed while the thread is not in the main loop. This is the normal case.
    // 2. The provider is removed while the thread is in the main loop. In that case the provider
    //    will be removed from the temp list and so it won't be re-added to the main list.

    PhAcquireQueuedLockExclusive(&providerThread->Lock);

    RemoveEntryList(&Registration->ListEntry);

    // Fix the boost count.
    if (Registration->Boosting)
        providerThread->BoostCount--;

    // The user-supplied object must be dereferenced
    // while the mutex is held.
    if (Registration->Object)
        PhDereferenceObject(Registration->Object);

    PhReleaseQueuedLockExclusive(&providerThread->Lock);
}

/**
 * Causes a provider to be queued for immediate execution.
 *
 * \param Registration A pointer to the registration object for a provider.
 * \param FutureRunId A variable which receives the run ID of the future run.
 *
 * \return TRUE if the operation was successful; FALSE if the provider is being unregistered, the
 * provider is already being boosted, or the provider thread is not running.
 *
 * \remarks Boosted providers will be run immediately, ignoring the run interval. Boosting will not
 * however affect the normal runs.
 */
BOOLEAN PhBoostProvider(
    _Inout_ PPH_PROVIDER_REGISTRATION Registration,
    _Out_opt_ PULONG FutureRunId
    )
{
    PPH_PROVIDER_THREAD providerThread;
    ULONG futureRunId;

    if (Registration->Unregistering)
        return FALSE;

    providerThread = Registration->ProviderThread;

    // Simply move to the provider to the front of the list. This works even if the provider is
    // currently in the temp list.

    PhAcquireQueuedLockExclusive(&providerThread->Lock);

    // Abort if the provider is already being boosted or the provider thread is stopping/stopped.
    if (Registration->Boosting || providerThread->State != ProviderThreadRunning)
    {
        PhReleaseQueuedLockExclusive(&providerThread->Lock);
        return FALSE;
    }

    RemoveEntryList(&Registration->ListEntry);
    InsertHeadList(&providerThread->ListHead, &Registration->ListEntry);

    Registration->Boosting = TRUE;
    providerThread->BoostCount++;

    futureRunId = Registration->RunId + 1;

    PhReleaseQueuedLockExclusive(&providerThread->Lock);

    // Wake up the thread.
    NtAlertThread(providerThread->ThreadHandle);

    if (FutureRunId)
        *FutureRunId = futureRunId;

    return TRUE;
}

/**
 * Gets the current run ID of a provider.
 *
 * \param Registration A pointer to the registration object for a provider.
 */
ULONG PhGetRunIdProvider(
    _In_ PPH_PROVIDER_REGISTRATION Registration
    )
{
    return Registration->RunId;
}

/**
 * Gets whether a provider is enabled.
 *
 * \param Registration A pointer to the registration object for a provider.
 */
BOOLEAN PhGetEnabledProvider(
    _In_ PPH_PROVIDER_REGISTRATION Registration
    )
{
    return Registration->Enabled;
}

/**
 * Sets whether a provider is enabled.
 *
 * \param Registration A pointer to the registration object for a provider.
 * \param Enabled TRUE if the provider is enabled, otherwise FALSE.
 */
VOID PhSetEnabledProvider(
    _Inout_ PPH_PROVIDER_REGISTRATION Registration,
    _In_ BOOLEAN Enabled
    )
{
    Registration->Enabled = Enabled;
}
