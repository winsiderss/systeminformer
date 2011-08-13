/*
 * Process Hacker - 
 *   service provider
 * 
 * Copyright (C) 2009-2011 wj32
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

#define PH_SRVPRV_PRIVATE
#include <phapp.h>
#include <winevt.h>
#include <extmgri.h>

typedef DWORD (WINAPI *_NotifyServiceStatusChangeW)(
    __in SC_HANDLE hService,
    __in DWORD dwNotifyMask,
    __in PSERVICE_NOTIFYW pNotifyBuffer
    );

typedef BOOL (WINAPI *_EvtClose)(
    __in EVT_HANDLE Object
    );

typedef EVT_HANDLE (WINAPI *_EvtSubscribe)(
    __in EVT_HANDLE Session,
    __in HANDLE SignalEvent,
    __in LPCWSTR ChannelPath,
    __in LPCWSTR Query,
    __in EVT_HANDLE Bookmark,
    __in PVOID context,
    __in EVT_SUBSCRIBE_CALLBACK Callback,
    __in DWORD Flags
    );

typedef struct _PHP_SERVICE_NAME_ENTRY
{
    PH_HASH_ENTRY HashEntry;
    PH_STRINGREF Name;
    ENUM_SERVICE_STATUS_PROCESS *ServiceEntry;
} PHP_SERVICE_NAME_ENTRY, *PPHP_SERVICE_NAME_ENTRY;

VOID NTAPI PhpServiceItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

BOOLEAN NTAPI PhpServiceHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    );

ULONG NTAPI PhpServiceHashtableHashFunction(
    __in PVOID Entry
    );

VOID PhpAddProcessItemService(
    __in PPH_PROCESS_ITEM ProcessItem,
    __in PPH_SERVICE_ITEM ServiceItem
    );

VOID PhpRemoveProcessItemService(
    __in PPH_PROCESS_ITEM ProcessItem,
    __in PPH_SERVICE_ITEM ServiceItem
    );

VOID PhpInitializeServiceNonPoll();

PPH_OBJECT_TYPE PhServiceItemType;

PPH_HASHTABLE PhServiceHashtable;
PH_QUEUED_LOCK PhServiceHashtableLock = PH_QUEUED_LOCK_INIT;

PHAPPAPI PH_CALLBACK_DECLARE(PhServiceAddedEvent);
PHAPPAPI PH_CALLBACK_DECLARE(PhServiceModifiedEvent);
PHAPPAPI PH_CALLBACK_DECLARE(PhServiceRemovedEvent);
PHAPPAPI PH_CALLBACK_DECLARE(PhServicesUpdatedEvent);

BOOLEAN PhEnableServiceNonPoll = FALSE;
static BOOLEAN PhpNonPollInitialized = FALSE;
static BOOLEAN PhpNonPollActive = FALSE;
static HANDLE PhpNonPollThreadHandle;
static ULONG PhpNonPollGate;
static _NotifyServiceStatusChangeW NotifyServiceStatusChangeW_I;
static _EvtClose EvtClose_I;
static _EvtSubscribe EvtSubscribe_I;

BOOLEAN PhServiceProviderInitialization()
{
    if (!NT_SUCCESS(PhCreateObjectType(
        &PhServiceItemType,
        L"ServiceItem",
        0,
        PhpServiceItemDeleteProcedure
        )))
        return FALSE;

    PhServiceHashtable = PhCreateHashtable(
        sizeof(PPH_SERVICE_ITEM),
        PhpServiceHashtableCompareFunction,
        PhpServiceHashtableHashFunction,
        40
        );

    return TRUE;
}

PPH_SERVICE_ITEM PhCreateServiceItem(
    __in_opt LPENUM_SERVICE_STATUS_PROCESS Information
    )
{
    PPH_SERVICE_ITEM serviceItem;

    if (!NT_SUCCESS(PhCreateObject(
        &serviceItem,
        PhEmGetObjectSize(EmServiceItemType, sizeof(PH_SERVICE_ITEM)),
        0,
        PhServiceItemType
        )))
        return NULL;

    memset(serviceItem, 0, sizeof(PH_SERVICE_ITEM));

    if (Information)
    {
        serviceItem->Name = PhCreateString(Information->lpServiceName);
        serviceItem->Key = serviceItem->Name->sr; 
        serviceItem->DisplayName = PhCreateString(Information->lpDisplayName);
        serviceItem->Type = Information->ServiceStatusProcess.dwServiceType;
        serviceItem->State = Information->ServiceStatusProcess.dwCurrentState;
        serviceItem->ControlsAccepted = Information->ServiceStatusProcess.dwControlsAccepted;
        serviceItem->Flags = Information->ServiceStatusProcess.dwServiceFlags;
        serviceItem->ProcessId = (HANDLE)Information->ServiceStatusProcess.dwProcessId;

        if (serviceItem->ProcessId)
            PhPrintUInt32(serviceItem->ProcessIdString, (ULONG)serviceItem->ProcessId);
    }

    PhEmCallObjectOperation(EmServiceItemType, serviceItem, EmObjectCreate);

    return serviceItem;
}

VOID PhpServiceItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)Object;

    PhEmCallObjectOperation(EmServiceItemType, serviceItem, EmObjectDelete);

    if (serviceItem->Name) PhDereferenceObject(serviceItem->Name);
    if (serviceItem->DisplayName) PhDereferenceObject(serviceItem->DisplayName);
}

/**
 * Generates a hash code for a string, case-insensitive.
 *
 * \param String The string.
 * \param Count The number of characters to hash.
 */
FORCEINLINE ULONG PhpHashStringIgnoreCase(
    __in PWSTR String,
    __in ULONG Count
    )
{
    ULONG hash = Count;

    if (Count == 0)
        return 0;

    do
    {
        hash = RtlUpcaseUnicodeChar(*String) + (hash << 6) + (hash << 16) - hash;
        String++;
    } while (--Count != 0);

    return hash;
}

BOOLEAN PhpServiceHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    PPH_SERVICE_ITEM serviceItem1 = *(PPH_SERVICE_ITEM *)Entry1;
    PPH_SERVICE_ITEM serviceItem2 = *(PPH_SERVICE_ITEM *)Entry2;

    return PhEqualStringRef(&serviceItem1->Key, &serviceItem2->Key, TRUE);
}

ULONG PhpServiceHashtableHashFunction(
    __in PVOID Entry
    )
{
    PPH_SERVICE_ITEM serviceItem = *(PPH_SERVICE_ITEM *)Entry;

    return PhpHashStringIgnoreCase(serviceItem->Key.Buffer, serviceItem->Key.Length / sizeof(WCHAR));
}

__assumeLocked PPH_SERVICE_ITEM PhpLookupServiceItem(
    __in PPH_STRINGREF Name
    )
{
    PH_SERVICE_ITEM lookupServiceItem;
    PPH_SERVICE_ITEM lookupServiceItemPtr = &lookupServiceItem;
    PPH_SERVICE_ITEM *serviceItem;

    lookupServiceItem.Key = *Name;

    serviceItem = (PPH_SERVICE_ITEM *)PhFindEntryHashtable(
        PhServiceHashtable,
        &lookupServiceItemPtr
        );

    if (serviceItem)
        return *serviceItem;
    else
        return NULL;
}

PPH_SERVICE_ITEM PhReferenceServiceItem(
    __in PWSTR Name
    )
{
    PPH_SERVICE_ITEM serviceItem;
    PH_STRINGREF key;

    // Construct a temporary service item for the lookup.
    PhInitializeStringRef(&key, Name);

    PhAcquireQueuedLockShared(&PhServiceHashtableLock);

    serviceItem = PhpLookupServiceItem(&key);

    if (serviceItem)
        PhReferenceObject(serviceItem);

    PhReleaseQueuedLockShared(&PhServiceHashtableLock);

    return serviceItem;
}

VOID PhMarkNeedsConfigUpdateServiceItem(
    __in PPH_SERVICE_ITEM ServiceItem
    )
{
    ServiceItem->NeedsConfigUpdate = TRUE;
}

__assumeLocked VOID PhpRemoveServiceItem(
    __in PPH_SERVICE_ITEM ServiceItem
    )
{
    PhRemoveEntryHashtable(PhServiceHashtable, &ServiceItem);
    PhDereferenceObject(ServiceItem);
}

PH_SERVICE_CHANGE PhGetServiceChange(
    __in PPH_SERVICE_MODIFIED_DATA Data
    )
{
    if (
        (
        Data->OldService.State == SERVICE_STOPPED ||
        Data->OldService.State == SERVICE_START_PENDING
        ) &&
        Data->Service->State == SERVICE_RUNNING
        )
    {
        return ServiceStarted;
    }

    if (
        (
        Data->OldService.State == SERVICE_PAUSED ||
        Data->OldService.State == SERVICE_CONTINUE_PENDING
        ) &&
        Data->Service->State == SERVICE_RUNNING
        )
    {
        return ServiceContinued;
    }

    if (
        (
        Data->OldService.State == SERVICE_RUNNING ||
        Data->OldService.State == SERVICE_PAUSE_PENDING
        ) &&
        Data->Service->State == SERVICE_PAUSED
        )
    {
        return ServicePaused;
    }

    if (
        (
        Data->OldService.State == SERVICE_RUNNING ||
        Data->OldService.State == SERVICE_STOP_PENDING
        ) &&
        Data->Service->State == SERVICE_STOPPED
        )
    {
        return ServiceStopped;
    }

    return -1;
}

VOID PhUpdateProcessItemServices(
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PPH_SERVICE_ITEM *serviceItem;

    // We don't need to lock as long as the service provider 
    // never runs concurrently with the process provider. This 
    // is currently true.

    PhBeginEnumHashtable(PhServiceHashtable, &enumContext);

    while (serviceItem = PhNextEnumHashtable(&enumContext))
    {
        if (
            (*serviceItem)->PendingProcess &&
            (*serviceItem)->ProcessId == ProcessItem->ProcessId
            )
        {
            PhpAddProcessItemService(ProcessItem, *serviceItem);
        }
    }
}

VOID PhpAddProcessItemService(
    __in PPH_PROCESS_ITEM ProcessItem,
    __in PPH_SERVICE_ITEM ServiceItem
    )
{
    PhAcquireQueuedLockExclusive(&ProcessItem->ServiceListLock);

    if (!ProcessItem->ServiceList)
        ProcessItem->ServiceList = PhCreatePointerList(2);

    if (!PhFindItemPointerList(ProcessItem->ServiceList, ServiceItem))
    {
        PhReferenceObject(ServiceItem);
        PhAddItemPointerList(ProcessItem->ServiceList, ServiceItem);
    }

    PhReleaseQueuedLockExclusive(&ProcessItem->ServiceListLock);

    ServiceItem->PendingProcess = FALSE;
    ProcessItem->JustProcessed = TRUE;
}

VOID PhpRemoveProcessItemService(
    __in PPH_PROCESS_ITEM ProcessItem,
    __in PPH_SERVICE_ITEM ServiceItem
    )
{
    HANDLE pointerHandle;

    if (!ProcessItem->ServiceList)
        return;

    PhAcquireQueuedLockExclusive(&ProcessItem->ServiceListLock);

    if (pointerHandle = PhFindItemPointerList(ProcessItem->ServiceList, ServiceItem))
    {
        PhRemoveItemPointerList(ProcessItem->ServiceList, pointerHandle);
        PhDereferenceObject(ServiceItem);
    }

    PhReleaseQueuedLockExclusive(&ProcessItem->ServiceListLock);

    ProcessItem->JustProcessed = TRUE;
}

VOID PhpUpdateServiceItemConfig(
    __in SC_HANDLE ScManagerHandle,
    __in PPH_SERVICE_ITEM ServiceItem
    )
{
    SC_HANDLE serviceHandle;

    serviceHandle = OpenService(ScManagerHandle, ServiceItem->Name->Buffer, SERVICE_QUERY_CONFIG);

    if (serviceHandle)
    {
        LPQUERY_SERVICE_CONFIG config;

        config = PhGetServiceConfig(serviceHandle);

        if (config)
        {
            ServiceItem->StartType = config->dwStartType;
            ServiceItem->ErrorControl = config->dwErrorControl;

            PhFree(config);
        }

        CloseServiceHandle(serviceHandle);
    }
}

static BOOLEAN PhpCompareServiceNameEntry(
    __in PPHP_SERVICE_NAME_ENTRY Value1,
    __in PPHP_SERVICE_NAME_ENTRY Value2
    )
{
    return PhEqualStringRef(&Value1->Name, &Value2->Name, TRUE);
}

static ULONG PhpHashServiceNameEntry(
    __in PPHP_SERVICE_NAME_ENTRY Value
    )
{
    return PhHashBytes((PUCHAR)Value->Name.Buffer, Value->Name.Length);
}

VOID PhServiceProviderUpdate(
    __in PVOID Object
    )
{
    static SC_HANDLE scManagerHandle = NULL;
    static ULONG runCount = 0;

    static PPH_HASH_ENTRY nameHashSet[256];
    static PPHP_SERVICE_NAME_ENTRY nameEntries = NULL;
    static ULONG nameEntriesCount;
    static ULONG nameEntriesAllocated = 0;

    LPENUM_SERVICE_STATUS_PROCESS services;
    ULONG numberOfServices;
    ULONG i;
    PPH_HASH_ENTRY hashEntry;

    // We always execute the first run, and we only initialize non-polling after the first run.
    if (PhEnableServiceNonPoll && runCount != 0)
    {
        if (!PhpNonPollInitialized)
        {
            if (WindowsVersion >= WINDOWS_VISTA)
            {
                PhpInitializeServiceNonPoll();
            }

            PhpNonPollInitialized = TRUE;
        }

        if (PhpNonPollActive)
        {
            if (InterlockedExchange(&PhpNonPollGate, 0) == 0)
            {
                // Non-poll gate is closed; skip all processing.
                goto UpdateEnd;
            }
        }
    }

    if (!scManagerHandle)
    {
        scManagerHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);

        if (!scManagerHandle)
            return;
    }

    services = PhEnumServices(scManagerHandle, 0, 0, &numberOfServices);

    if (!services)
        return;

    // Build a hash set containing the service names.

    // This has caused a massive decrease in background CPU usage, and 
    // is certainly much better than the quadratic-time string comparisons 
    // we were doing before (in the "Look for dead services" section).

    nameEntriesCount = 0;

    if (nameEntriesAllocated < numberOfServices)
    {
        nameEntriesAllocated = numberOfServices + 32;

        if (nameEntries) PhFree(nameEntries);
        nameEntries = PhAllocate(sizeof(PHP_SERVICE_NAME_ENTRY) * nameEntriesAllocated);
    }

    PhInitializeHashSet(nameHashSet, PH_HASH_SET_SIZE(nameHashSet));

    for (i = 0; i < numberOfServices; i++)
    {
        PPHP_SERVICE_NAME_ENTRY entry;

        entry = &nameEntries[nameEntriesCount++];
        PhInitializeStringRef(&entry->Name, services[i].lpServiceName);
        entry->ServiceEntry = &services[i];
        PhAddEntryHashSet(
            nameHashSet,
            PH_HASH_SET_SIZE(nameHashSet),
            &entry->HashEntry,
            PhpHashServiceNameEntry(entry)
            );
    }

    // Look for dead services.
    {
        PPH_LIST servicesToRemove = NULL;
        PH_HASHTABLE_ENUM_CONTEXT enumContext;
        PPH_SERVICE_ITEM *serviceItem;

        PhBeginEnumHashtable(PhServiceHashtable, &enumContext);

        while (serviceItem = PhNextEnumHashtable(&enumContext))
        {
            BOOLEAN found = FALSE;
            PHP_SERVICE_NAME_ENTRY lookupNameEntry;

            // Check if the service still exists.

            lookupNameEntry.Name = (*serviceItem)->Name->sr;
            hashEntry = PhFindEntryHashSet(
                nameHashSet,
                PH_HASH_SET_SIZE(nameHashSet),
                PhpHashServiceNameEntry(&lookupNameEntry)
                );

            for (; hashEntry; hashEntry = hashEntry->Next)
            {
                PPHP_SERVICE_NAME_ENTRY nameEntry;

                nameEntry = CONTAINING_RECORD(hashEntry, PHP_SERVICE_NAME_ENTRY, HashEntry);

                if (PhpCompareServiceNameEntry(&lookupNameEntry, nameEntry))
                {
                    found = TRUE;
                    break;
                }
            }

            if (!found)
            {
                // Remove the service from its process.
                if ((*serviceItem)->ProcessId)
                {
                    PPH_PROCESS_ITEM processItem;

                    processItem = PhReferenceProcessItem((HANDLE)(*serviceItem)->ProcessId);

                    if (processItem)
                    {
                        PhpRemoveProcessItemService(processItem, *serviceItem);
                        PhDereferenceObject(processItem);
                    }
                }

                // Raise the service removed event.
                PhInvokeCallback(&PhServiceRemovedEvent, *serviceItem);

                if (!servicesToRemove)
                    servicesToRemove = PhCreateList(2);

                PhAddItemList(servicesToRemove, *serviceItem);
            }
        }

        if (servicesToRemove)
        {
            PhAcquireQueuedLockExclusive(&PhServiceHashtableLock);

            for (i = 0; i < servicesToRemove->Count; i++)
            {
                PhpRemoveServiceItem((PPH_SERVICE_ITEM)servicesToRemove->Items[i]);
            }

            PhReleaseQueuedLockExclusive(&PhServiceHashtableLock);
            PhDereferenceObject(servicesToRemove);
        }
    }

    // Look for new services and update existing ones.
    for (i = 0; i < PH_HASH_SET_SIZE(nameHashSet); i++)
    {
        for (hashEntry = nameHashSet[i]; hashEntry; hashEntry = hashEntry->Next)
        {
            PPH_SERVICE_ITEM serviceItem;
            PPHP_SERVICE_NAME_ENTRY nameEntry;
            ENUM_SERVICE_STATUS_PROCESS *serviceEntry;

            nameEntry = CONTAINING_RECORD(hashEntry, PHP_SERVICE_NAME_ENTRY, HashEntry);
            serviceEntry = nameEntry->ServiceEntry;
            serviceItem = PhpLookupServiceItem(&nameEntry->Name);

            if (!serviceItem)
            {
                // Create the service item and fill in basic information.

                serviceItem = PhCreateServiceItem(serviceEntry);

                PhpUpdateServiceItemConfig(scManagerHandle, serviceItem);

                // Add the service to its process, if appropriate.
                if (
                    (
                    serviceItem->State == SERVICE_RUNNING ||
                    serviceItem->State == SERVICE_PAUSED
                    ) &&
                    serviceItem->ProcessId
                    )
                {
                    PPH_PROCESS_ITEM processItem;

                    if (processItem = PhReferenceProcessItem(serviceItem->ProcessId))
                    {
                        PhpAddProcessItemService(processItem, serviceItem);
                        PhDereferenceObject(processItem);
                    }
                    else
                    {
                        // The process doesn't exist yet (to us). Set the pending 
                        // flag and when the process is added this will be 
                        // fixed.
                        serviceItem->PendingProcess = TRUE;
                    }
                }

                // Add the service item to the hashtable.
                PhAcquireQueuedLockExclusive(&PhServiceHashtableLock);
                PhAddEntryHashtable(PhServiceHashtable, &serviceItem);
                PhReleaseQueuedLockExclusive(&PhServiceHashtableLock);

                // Raise the service added event.
                PhInvokeCallback(&PhServiceAddedEvent, serviceItem);
            }
            else
            {
                if (
                    serviceItem->Type != serviceEntry->ServiceStatusProcess.dwServiceType || 
                    serviceItem->State != serviceEntry->ServiceStatusProcess.dwCurrentState ||
                    serviceItem->ControlsAccepted != serviceEntry->ServiceStatusProcess.dwControlsAccepted ||
                    serviceItem->ProcessId != (HANDLE)serviceEntry->ServiceStatusProcess.dwProcessId ||
                    serviceItem->NeedsConfigUpdate
                    )
                {
                    PH_SERVICE_MODIFIED_DATA serviceModifiedData;
                    PH_SERVICE_CHANGE serviceChange;

                    // The service has been "modified".

                    serviceModifiedData.Service = serviceItem;
                    memset(&serviceModifiedData.OldService, 0, sizeof(PH_SERVICE_ITEM));
                    serviceModifiedData.OldService.Type = serviceItem->Type;
                    serviceModifiedData.OldService.State = serviceItem->State;
                    serviceModifiedData.OldService.ControlsAccepted = serviceItem->ControlsAccepted;
                    serviceModifiedData.OldService.ProcessId = serviceItem->ProcessId;

                    // Update the service item.
                    serviceItem->Type = serviceEntry->ServiceStatusProcess.dwServiceType;
                    serviceItem->State = serviceEntry->ServiceStatusProcess.dwCurrentState;
                    serviceItem->ControlsAccepted = serviceEntry->ServiceStatusProcess.dwControlsAccepted;
                    serviceItem->ProcessId = (HANDLE)serviceEntry->ServiceStatusProcess.dwProcessId;

                    if (serviceItem->ProcessId)
                        PhPrintUInt32(serviceItem->ProcessIdString, (ULONG)serviceItem->ProcessId);
                    else
                        serviceItem->ProcessIdString[0] = 0;

                    // Add/remove the service from its process.

                    serviceChange = PhGetServiceChange(&serviceModifiedData);

                    if (
                        (serviceChange == ServiceStarted && serviceItem->ProcessId) ||
                        (serviceChange == ServiceStopped && serviceModifiedData.OldService.ProcessId)
                        )
                    {
                        PPH_PROCESS_ITEM processItem;

                        if (serviceChange == ServiceStarted)
                            processItem = PhReferenceProcessItem(serviceItem->ProcessId);
                        else
                            processItem = PhReferenceProcessItem(serviceModifiedData.OldService.ProcessId);

                        if (processItem)
                        {
                            if (serviceChange == ServiceStarted)
                                PhpAddProcessItemService(processItem, serviceItem);
                            else
                                PhpRemoveProcessItemService(processItem, serviceItem);

                            PhDereferenceObject(processItem);
                        }
                        else
                        {
                            if (serviceChange == ServiceStarted)
                                serviceItem->PendingProcess = TRUE;
                            else
                                serviceItem->PendingProcess = FALSE;
                        }
                    }
                    else if (
                        serviceItem->State == SERVICE_RUNNING &&
                        serviceItem->ProcessId != serviceModifiedData.OldService.ProcessId &&
                        serviceItem->ProcessId
                        )
                    {
                        PPH_PROCESS_ITEM processItem;

                        // The service stopped and started, and the only change we have detected 
                        // is in the process ID.

                        if (processItem = PhReferenceProcessItem(serviceModifiedData.OldService.ProcessId))
                        {
                            PhpRemoveProcessItemService(processItem, serviceItem);
                            PhDereferenceObject(processItem);
                        }

                        if (processItem = PhReferenceProcessItem(serviceItem->ProcessId))
                        {
                            PhpAddProcessItemService(processItem, serviceItem);
                            PhDereferenceObject(processItem);
                        }
                        else
                        {
                            serviceItem->PendingProcess = TRUE;
                        }
                    }

                    // Do a config update if necessary.
                    if (serviceItem->NeedsConfigUpdate)
                    {
                        PhpUpdateServiceItemConfig(scManagerHandle, serviceItem);
                        serviceItem->NeedsConfigUpdate = FALSE;
                    }

                    // Raise the service modified event.
                    PhInvokeCallback(&PhServiceModifiedEvent, &serviceModifiedData);
                }
            }
        }
    }

    PhFree(services);

UpdateEnd:
    PhInvokeCallback(&PhServicesUpdatedEvent, NULL);
    runCount++;
}

DWORD WINAPI PhpServiceNonPollSubscribeCallback(
    __in EVT_SUBSCRIBE_NOTIFY_ACTION Action,
    __in PVOID UserContext,
    __in EVT_HANDLE Event
    )
{
    PhpNonPollGate = 1;
    return 0;
}

VOID CALLBACK PhpServiceNonPollScNotifyCallback(
    __in PVOID pParameter 
    )
{
    PSERVICE_NOTIFYW notifyBuffer = pParameter;

    if (notifyBuffer->dwNotificationStatus == ERROR_SUCCESS)
    {
        if (notifyBuffer->dwNotificationTriggered & (SERVICE_NOTIFY_CREATED | SERVICE_NOTIFY_DELETED))
        {
            LocalFree(notifyBuffer->pszServiceNames);
        }
    }

    PhpNonPollGate = 1;

    NtSetEvent((HANDLE)notifyBuffer->pContext, NULL);
}

NTSTATUS PhpServiceNonPollThreadStart(
    __in PVOID Parameter
    )
{
    EVT_HANDLE subscriptionHandle;
    SC_HANDLE scManagerHandle;
    HANDLE notifyEventHandle;
    SERVICE_NOTIFYW notifyBuffer;
    ULONG result;

    // The non-polling method involves two functions:
    // * NotifyServiceStatusChange provides us with service creation and deletion events.
    // * EvtSubscribe provides us with service state change events (but not pending states).
    // Currently there are two major problems with non-polling:
    // * Pending state changes are not visible.
    // * Driver events (start, stop, delete) are not visible. This is because the SCM must 
    //   explicitly check if drivers are loaded - it doesn't get notifications for them either.

    subscriptionHandle = EvtSubscribe_I(
        NULL,
        NULL,
        L"System",
        L"*[System[Provider[@Name='Service Control Manager']]]",
        NULL,
        NULL,
        PhpServiceNonPollSubscribeCallback,
        EvtSubscribeToFutureEvents
        );

    if (!subscriptionHandle)
    {
        // Subscription unsuccessful; cancel non-polling.
        PhpNonPollActive = FALSE;
        PhpNonPollGate = 1;
        return STATUS_UNSUCCESSFUL;
    }

    if (!NT_SUCCESS(NtCreateEvent(&notifyEventHandle, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE)))
    {
        EvtClose_I(subscriptionHandle);
        PhpNonPollActive = FALSE;
        PhpNonPollGate = 1;
        return STATUS_UNSUCCESSFUL;
    }

    while (TRUE)
    {
        scManagerHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);

        if (!scManagerHandle)
        {
            PhpNonPollActive = FALSE;
            PhpNonPollGate = 1;
            return STATUS_UNSUCCESSFUL;
        }

        while (TRUE)
        {
            memset(&notifyBuffer, 0, sizeof(SERVICE_NOTIFYW));
            notifyBuffer.dwVersion = SERVICE_NOTIFY_STATUS_CHANGE;
            notifyBuffer.pfnNotifyCallback = PhpServiceNonPollScNotifyCallback;
            notifyBuffer.pContext = notifyEventHandle;

            result = NotifyServiceStatusChangeW_I(scManagerHandle, SERVICE_NOTIFY_CREATED | SERVICE_NOTIFY_DELETED, &notifyBuffer);

            if (result == ERROR_SUCCESS)
            {
                // Wait for the callback function to be called and complete.

                while (NtWaitForSingleObject(notifyEventHandle, TRUE, NULL) != STATUS_WAIT_0)
                    NOTHING;
            }
            else if (result == ERROR_SERVICE_NOTIFY_CLIENT_LAGGING)
            {
                // We are lagging behind. Re-open the handle to the SCM.
                break;
            }
            else
            {
                LARGE_INTEGER interval;

                // Sleep for a bit and try again.
                interval.QuadPart = -100 * PH_TIMEOUT_MS;
                NtDelayExecution(FALSE, &interval);
            }
        }

        CloseServiceHandle(scManagerHandle);
    }

    NtClose(notifyEventHandle);

    EvtClose_I(subscriptionHandle);

    return STATUS_SUCCESS;
}

VOID PhpInitializeServiceNonPoll()
{
    HMODULE wevtapiHandle;

    // Dynamically import the required functions.

    NotifyServiceStatusChangeW_I = PhGetProcAddress(L"advapi32.dll", "NotifyServiceStatusChangeW");

    if (!NotifyServiceStatusChangeW_I)
        return;

    wevtapiHandle = LoadLibrary(L"wevtapi.dll");

    if (!wevtapiHandle)
        return;

    EvtClose_I = (PVOID)GetProcAddress(wevtapiHandle, "EvtClose");

    if (!EvtClose_I)
        return;

    EvtSubscribe_I = (PVOID)GetProcAddress(wevtapiHandle, "EvtSubscribe");

    if (!EvtSubscribe_I)
        return;

    PhpNonPollActive = TRUE;
    PhpNonPollGate = 1; // initially the gate should be open since we only just initialized everything

    PhpNonPollThreadHandle = PhCreateThread(0, PhpServiceNonPollThreadStart, NULL);

    if (!PhpNonPollThreadHandle)
    {
        PhpNonPollActive = FALSE;
        return;
    }
}
