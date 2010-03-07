/*
 * Process Hacker - 
 *   provider system
 * 
 * Copyright (C) 2009-2010 wj32
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Provider objects allow a function to be executed periodically. 
 * This is managed by a synchronization timer object which is 
 * signaled periodically. The use of a timer object as opposed to 
 * a simple sleep call means that the length of time a provider 
 * function takes to execute has no effect on the interval between 
 * runs.
 *
 * In contrast to callback objects, the context passed to provider 
 * functions must be reference-counted objects. This means that 
 * it is not guaranteed that the function will not be in execution 
 * after the unregister operation is complete. However, the 
 * since the context object is reference-counted, there are no 
 * safety issues.
 *
 * Providers can be boosted, which causes them to be run immediately 
 * ignoring the interval. This is separate to the periodic runs, 
 * and does not cause the next periodic run to be missed. There are 
 * also no limits on how many times a provider can be boosted - 
 * if a provider is boosted 10 times in succession, it will be run 10 
 * times in addition to the periodic run.
 *
 * Register/unregister/boost operations are completely thread-safe. 
 * Start/stop operations are not thread-safe.
 */

#define PROVIDER_PRIVATE
#include <ph.h>

#ifdef DEBUG
LIST_ENTRY PhDbgProviderListHead;
PH_FAST_LOCK PhDbgProviderListLock;
#endif

/**
 * Initializes a provider thread.
 *
 * \param ProviderThread A pointer to a provider thread object.
 * \param Interval The interval between each run, in milliseconds.
 */
VOID PhInitializeProviderThread(
    __out PPH_PROVIDER_THREAD ProviderThread,
    __in ULONG Interval
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
    PhAcquireFastLockExclusive(&PhDbgProviderListLock);
    InsertTailList(&PhDbgProviderListHead, &ProviderThread->DbgListEntry);
    PhReleaseFastLockExclusive(&PhDbgProviderListLock);
#endif
}

/**
 * Frees resources used by a provider thread.
 *
 * \param ProviderThread A pointer to a provider thread object.
 */
VOID PhDeleteProviderThread(
    __inout PPH_PROVIDER_THREAD ProviderThread
    )
{
    // Nothing

#ifdef DEBUG
    PhAcquireFastLockExclusive(&PhDbgProviderListLock);
    RemoveEntryList(&ProviderThread->DbgListEntry);
    PhReleaseFastLockExclusive(&PhDbgProviderListLock);
#endif
}

NTSTATUS NTAPI PhpProviderThreadStart(
    __in PVOID Parameter
    )
{
    PPH_PROVIDER_THREAD providerThread = (PPH_PROVIDER_THREAD)Parameter;
    NTSTATUS status = STATUS_SUCCESS;
    PLIST_ENTRY listEntry;
    PPH_PROVIDER_REGISTRATION registration;
    PPH_PROVIDER_FUNCTION providerFunction;
    PVOID object;
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

        PhAcquireQueuedLockExclusiveFast(&providerThread->Lock);

        // Main loop.

        // We check the status variable for STATUS_ALERTED, which 
        // means that someone is requesting that a provider be 
        // boosted. Note that if they alert this thread while we 
        // are not waiting on the timer, when we do perform the 
        // wait it will return immediately with STATUS_ALERTED.

        while (TRUE)
        {
            if (status == STATUS_ALERTED)
            {
                // Check if we have any more providers to boost.
                // Note that this always works because boosted 
                // providers are always in front of normal providers. 
                // Therefore we will never mistakenly boost normal 
                // providers.

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
                // If we're boosting providers, we don't care if they 
                // are enabled or not. However, we have to make sure 
                // any providers which are being unregistered get a 
                // chance to fix the boost count.

                if (registration->Unregistering)
                {
                    PhReleaseQueuedLockExclusiveFast(&providerThread->Lock);
                    PhAcquireQueuedLockExclusiveFast(&providerThread->Lock);

                    continue;
                }
            }

            if (status == STATUS_ALERTED)
            {
                registration->Boosting = FALSE;
                providerThread->BoostCount--;
            }

            providerFunction = registration->Function;
            object = registration->Object;

            if (object)
                PhReferenceObject(object);

            PhReleaseQueuedLockExclusiveFast(&providerThread->Lock);
            providerFunction(object);
            PhAcquireQueuedLockExclusiveFast(&providerThread->Lock);

            if (object)
                PhDereferenceObject(object);
        }

        // Re-add the items in the temp list to the main list.

        while ((listEntry = RemoveHeadList(&tempListHead)) != &tempListHead)
            InsertTailList(&providerThread->ListHead, listEntry);

        PhReleaseQueuedLockExclusiveFast(&providerThread->Lock);

        // Perform an alertable wait so we can be woken up by 
        // someone telling us to boost providers, or to terminate.
        status = NtWaitForSingleObject(
            providerThread->TimerHandle,
            TRUE,
            NULL
            );
    }

    return STATUS_SUCCESS;
}

/**
 * Starts a provider thread.
 *
 * \param ProviderThread A pointer to a provider thread object.
 */
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
    ProviderThread->ThreadHandle = PhCreateThread(
        0,
        PhpProviderThreadStart,
        ProviderThread
        );

    ProviderThread->State = ProviderThreadRunning;
}

/**
 * Stops a provider thread.
 *
 * \param ProviderThread A pointer to a provider thread object.
 */
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
VOID PhSetProviderThreadInterval(
    __inout PPH_PROVIDER_THREAD ProviderThread,
    __in ULONG Interval
    )
{
    ProviderThread->Interval = Interval;

    if (ProviderThread->TimerHandle)
    {
        LARGE_INTEGER interval;

        interval.QuadPart = -(LONGLONG)Interval * PH_TIMEOUT_MS;

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

/**
 * Causes a provider to be queued for immediate execution.
 *
 * \param ProviderThread A pointer to a provider thread object.
 * \param Registration A pointer to the registration object for 
 * a provider.
 *
 * \return TRUE if the operation was successful; FALSE if 
 * the provider is being unregistered, or the provider thread 
 * is not running.
 *
 * \remarks Boosted providers will be run immediately, ignoring 
 * the run interval. Boosting will not however affect the normal 
 * runs; if a provider is boosted \a n times, it will be run 
 * \a n times in addition to the normal periodic run.
 */
BOOLEAN PhBoostProvider(
    __inout PPH_PROVIDER_THREAD ProviderThread,
    __inout PPH_PROVIDER_REGISTRATION Registration
    )
{
    if (Registration->Unregistering)
        return FALSE;

    // Simply move to the provider to the front of the list. 
    // This works even if the provider is currently in the temp list.

    PhAcquireQueuedLockExclusiveFast(&ProviderThread->Lock);

    // Abort if the provider is stopping/stopped.
    if (ProviderThread->State != ProviderThreadRunning)
    {
        PhReleaseQueuedLockExclusive(&ProviderThread->Lock);
        return FALSE;
    }

    RemoveEntryList(&Registration->ListEntry);
    InsertHeadList(&ProviderThread->ListHead, &Registration->ListEntry);

    Registration->Boosting = TRUE;
    ProviderThread->BoostCount++;

    PhReleaseQueuedLockExclusiveFast(&ProviderThread->Lock);

    // Wake up the thread.
    NtAlertThread(ProviderThread->ThreadHandle);

    return TRUE;
}

/**
 * Sets whether a provider is enabled.
 *
 * \param Registration A pointer to the registration object for 
 * a provider.
 * \param Enabled TRUE if the provider is enabled, otherwise 
 * FALSE.
 */
VOID PhSetProviderEnabled(
    __in PPH_PROVIDER_REGISTRATION Registration,
    __in BOOLEAN Enabled
    )
{
    Registration->Enabled = Enabled;
}

/**
 * Registers a provider with a provider thread.
 *
 * \param ProviderThread A pointer to a provider thread object.
 * \param Function The provider function.
 * \param Object A pointer to an object to pass to the provider 
 * function. The object must be managed by the reference-counting 
 * system.
 * \param Registration A variable which receives registration 
 * information for the provider.
 *
 * \remarks The provider is initially disabled. Call 
 * PhSetProviderEnabled() to enable it.
 */
VOID PhRegisterProvider(
    __inout PPH_PROVIDER_THREAD ProviderThread,
    __in PPH_PROVIDER_FUNCTION Function,
    __in PVOID Object,
    __out PPH_PROVIDER_REGISTRATION Registration
    )
{
    Registration->Function = Function;
    Registration->Object = Object;
    Registration->Enabled = FALSE;
    Registration->Unregistering = FALSE;
    Registration->Boosting = FALSE;

    if (Object)
        PhReferenceObject(Object);

    PhAcquireQueuedLockExclusiveFast(&ProviderThread->Lock);
    InsertTailList(&ProviderThread->ListHead, &Registration->ListEntry);
    PhReleaseQueuedLockExclusiveFast(&ProviderThread->Lock);
}

/**
 * Unregisters a provider.
 *
 * \param ProviderThread A pointer to a provider thread object.
 * \param Registration A pointer to the registration object for 
 * a provider.
 *
 * \remarks The provider function may still be in execution 
 * once this function returns.
 */
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

    PhAcquireQueuedLockExclusiveFast(&ProviderThread->Lock);

    RemoveEntryList(&Registration->ListEntry);

    // Fix the boost count.
    if (Registration->Boosting)
        ProviderThread->BoostCount--;

    // The user-supplied object must be dereferenced 
    // while the mutex is held.
    if (Registration->Object)
        PhDereferenceObject(Registration->Object);

    PhReleaseQueuedLockExclusiveFast(&ProviderThread->Lock);
}
