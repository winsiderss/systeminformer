/*
 * Process Hacker -
 *   service provider
 *
 * Copyright (C) 2009-2015 wj32
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

#include <phapp.h>
#include <winevt.h>
#include <extmgri.h>

typedef DWORD (WINAPI *_NotifyServiceStatusChangeW)(
    _In_ SC_HANDLE hService,
    _In_ DWORD dwNotifyMask,
    _In_ PSERVICE_NOTIFYW pNotifyBuffer
    );

typedef struct _PHP_SERVICE_NAME_ENTRY
{
    PH_HASH_ENTRY HashEntry;
    PH_STRINGREF Name;
    ENUM_SERVICE_STATUS_PROCESS *ServiceEntry;
} PHP_SERVICE_NAME_ENTRY, *PPHP_SERVICE_NAME_ENTRY;

typedef enum _PHP_SERVICE_NOTIFY_STATE
{
    SnNone,
    SnAdding,
    SnRemoving,
    SnNotify
} PHP_SERVICE_NOTIFY_STATE;

typedef struct _PHP_SERVICE_NOTIFY_CONTEXT
{
    LIST_ENTRY ListEntry;
    SC_HANDLE ServiceHandle;
    PPH_STRING ServiceName; // Valid only when adding
    BOOLEAN IsServiceManager;
    PHP_SERVICE_NOTIFY_STATE State;
    SERVICE_NOTIFY Buffer;
} PHP_SERVICE_NOTIFY_CONTEXT, *PPHP_SERVICE_NOTIFY_CONTEXT;

VOID NTAPI PhpServiceItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

BOOLEAN NTAPI PhpServiceHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

ULONG NTAPI PhpServiceHashtableHashFunction(
    _In_ PVOID Entry
    );

VOID PhpAddProcessItemService(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_SERVICE_ITEM ServiceItem
    );

VOID PhpRemoveProcessItemService(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_SERVICE_ITEM ServiceItem
    );

VOID PhpInitializeServiceNonPoll(
    VOID
    );

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
static HANDLE PhpNonPollEventHandle;
static PH_QUEUED_LOCK PhpNonPollServiceListLock = PH_QUEUED_LOCK_INIT;
static LIST_ENTRY PhpNonPollServiceListHead;
static LIST_ENTRY PhpNonPollServicePendingListHead;

BOOLEAN PhServiceProviderInitialization(
    VOID
    )
{
    PhServiceItemType = PhCreateObjectType(L"ServiceItem", 0, PhpServiceItemDeleteProcedure);
    PhServiceHashtable = PhCreateHashtable(
        sizeof(PPH_SERVICE_ITEM),
        PhpServiceHashtableCompareFunction,
        PhpServiceHashtableHashFunction,
        40
        );

    return TRUE;
}

PPH_SERVICE_ITEM PhCreateServiceItem(
    _In_opt_ LPENUM_SERVICE_STATUS_PROCESS Information
    )
{
    PPH_SERVICE_ITEM serviceItem;

    serviceItem = PhCreateObject(
        PhEmGetObjectSize(EmServiceItemType, sizeof(PH_SERVICE_ITEM)),
        PhServiceItemType
        );
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
        serviceItem->ProcessId = UlongToHandle(Information->ServiceStatusProcess.dwProcessId);

        if (serviceItem->ProcessId)
            PhPrintUInt32(serviceItem->ProcessIdString, HandleToUlong(serviceItem->ProcessId));
    }

    PhEmCallObjectOperation(EmServiceItemType, serviceItem, EmObjectCreate);

    return serviceItem;
}

VOID PhpServiceItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)Object;

    PhEmCallObjectOperation(EmServiceItemType, serviceItem, EmObjectDelete);

    if (serviceItem->Name) PhDereferenceObject(serviceItem->Name);
    if (serviceItem->DisplayName) PhDereferenceObject(serviceItem->DisplayName);
}

BOOLEAN PhpServiceHashtableCompareFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_SERVICE_ITEM serviceItem1 = *(PPH_SERVICE_ITEM *)Entry1;
    PPH_SERVICE_ITEM serviceItem2 = *(PPH_SERVICE_ITEM *)Entry2;

    return PhEqualStringRef(&serviceItem1->Key, &serviceItem2->Key, TRUE);
}

ULONG PhpServiceHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PPH_SERVICE_ITEM serviceItem = *(PPH_SERVICE_ITEM *)Entry;

    return PhHashStringRef(&serviceItem->Key, TRUE);
}

PPH_SERVICE_ITEM PhpLookupServiceItem(
    _In_ PPH_STRINGREF Name
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
    _In_ PWSTR Name
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
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    ServiceItem->NeedsConfigUpdate = TRUE;
}

VOID PhpRemoveServiceItem(
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    PhRemoveEntryHashtable(PhServiceHashtable, &ServiceItem);
    PhDereferenceObject(ServiceItem);
}

PH_SERVICE_CHANGE PhGetServiceChange(
    _In_ PPH_SERVICE_MODIFIED_DATA Data
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
    _In_ PPH_PROCESS_ITEM ProcessItem
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
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_SERVICE_ITEM ServiceItem
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
    ProcessItem->JustProcessed = 1;
}

VOID PhpRemoveProcessItemService(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_SERVICE_ITEM ServiceItem
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

    ProcessItem->JustProcessed = 1;
}

VOID PhpUpdateServiceItemConfig(
    _In_ SC_HANDLE ScManagerHandle,
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    SC_HANDLE serviceHandle;

    serviceHandle = OpenService(ScManagerHandle, ServiceItem->Name->Buffer, SERVICE_QUERY_CONFIG);

    if (serviceHandle)
    {
        LPQUERY_SERVICE_CONFIG config;
        SERVICE_DELAYED_AUTO_START_INFO delayedAutoStartInfo;
        ULONG returnLength;
        PSERVICE_TRIGGER_INFO triggerInfo;

        config = PhGetServiceConfig(serviceHandle);

        if (config)
        {
            ServiceItem->StartType = config->dwStartType;
            ServiceItem->ErrorControl = config->dwErrorControl;

            PhFree(config);
        }

        if (QueryServiceConfig2(
            serviceHandle,
            SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
            (BYTE *)&delayedAutoStartInfo,
            sizeof(SERVICE_DELAYED_AUTO_START_INFO),
            &returnLength
            ))
        {
            ServiceItem->DelayedStart = delayedAutoStartInfo.fDelayedAutostart;
        }
        else
        {
            ServiceItem->DelayedStart = FALSE;
        }

        if (triggerInfo = PhQueryServiceVariableSize(serviceHandle, SERVICE_CONFIG_TRIGGER_INFO))
        {
            ServiceItem->HasTriggers = triggerInfo->cTriggers != 0;
            PhFree(triggerInfo);
        }
        else
        {
            ServiceItem->HasTriggers = FALSE;
        }

        CloseServiceHandle(serviceHandle);
    }
}

static BOOLEAN PhpCompareServiceNameEntry(
    _In_ PPHP_SERVICE_NAME_ENTRY Value1,
    _In_ PPHP_SERVICE_NAME_ENTRY Value2
    )
{
    return PhEqualStringRef(&Value1->Name, &Value2->Name, TRUE);
}

static ULONG PhpHashServiceNameEntry(
    _In_ PPHP_SERVICE_NAME_ENTRY Value
    )
{
    return PhHashStringRef(&Value->Name, TRUE);
}

VOID PhServiceProviderUpdate(
    _In_ PVOID Object
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
        PhInitializeStringRefLongHint(&entry->Name, services[i].lpServiceName);
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
                    serviceItem->ProcessId != UlongToHandle(serviceEntry->ServiceStatusProcess.dwProcessId) ||
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
                    serviceItem->ProcessId = UlongToHandle(serviceEntry->ServiceStatusProcess.dwProcessId);

                    if (serviceItem->ProcessId)
                        PhPrintUInt32(serviceItem->ProcessIdString, HandleToUlong(serviceItem->ProcessId));
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

VOID CALLBACK PhpServiceNonPollScNotifyCallback(
    _In_ PVOID pParameter
    )
{
    PSERVICE_NOTIFYW notifyBuffer = pParameter;
    PPHP_SERVICE_NOTIFY_CONTEXT notifyContext = notifyBuffer->pContext;

    if (notifyBuffer->dwNotificationStatus == ERROR_SUCCESS)
    {
        if ((notifyBuffer->dwNotificationTriggered & (SERVICE_NOTIFY_CREATED | SERVICE_NOTIFY_DELETED)) &&
            notifyBuffer->pszServiceNames)
        {
            PWSTR name;
            SIZE_T nameLength;

            name = notifyBuffer->pszServiceNames;

            while (TRUE)
            {
                nameLength = PhCountStringZ(name);

                if (nameLength == 0)
                    break;

                if (name[0] == '/')
                {
                    PPHP_SERVICE_NOTIFY_CONTEXT newNotifyContext;

                    // Service creation
                    newNotifyContext = PhAllocate(sizeof(PHP_SERVICE_NOTIFY_CONTEXT));
                    memset(newNotifyContext, 0, sizeof(PHP_SERVICE_NOTIFY_CONTEXT));
                    newNotifyContext->State = SnAdding;
                    newNotifyContext->ServiceName = PhCreateString(name + 1);
                    InsertTailList(&PhpNonPollServicePendingListHead, &newNotifyContext->ListEntry);
                }

                name += nameLength + 1;
            }

            LocalFree(notifyBuffer->pszServiceNames);
        }

        notifyContext->State = SnNotify;
        RemoveEntryList(&notifyContext->ListEntry);
        InsertTailList(&PhpNonPollServicePendingListHead, &notifyContext->ListEntry);
    }
    else if (notifyBuffer->dwNotificationStatus == ERROR_SERVICE_MARKED_FOR_DELETE)
    {
        if (!notifyContext->IsServiceManager)
        {
            notifyContext->State = SnRemoving;
            RemoveEntryList(&notifyContext->ListEntry);
            InsertTailList(&PhpNonPollServicePendingListHead, &notifyContext->ListEntry);
        }
    }
    else
    {
        notifyContext->State = SnNotify;
        RemoveEntryList(&notifyContext->ListEntry);
        InsertTailList(&PhpNonPollServicePendingListHead, &notifyContext->ListEntry);
    }

    PhpNonPollGate = 1;
    NtSetEvent(PhpNonPollEventHandle, NULL);
}

VOID PhpDestroyServiceNotifyContext(
    _In_ PPHP_SERVICE_NOTIFY_CONTEXT NotifyContext
    )
{
    if (NotifyContext->Buffer.pszServiceNames)
        LocalFree(NotifyContext->Buffer.pszServiceNames);

    CloseServiceHandle(NotifyContext->ServiceHandle);
    PhClearReference(&NotifyContext->ServiceName);
    PhFree(NotifyContext);
}

NTSTATUS PhpServiceNonPollThreadStart(
    _In_ PVOID Parameter
    )
{
    ULONG result;
    SC_HANDLE scManagerHandle;
    LPENUM_SERVICE_STATUS_PROCESS services;
    ULONG numberOfServices;
    ULONG i;
    PLIST_ENTRY listEntry;
    PPHP_SERVICE_NOTIFY_CONTEXT notifyContext;

    if (!NT_SUCCESS(NtCreateEvent(&PhpNonPollEventHandle, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE)))
    {
        PhpNonPollActive = FALSE;
        PhpNonPollGate = 1;
        return STATUS_UNSUCCESSFUL;
    }

    while (TRUE)
    {
        scManagerHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);

        if (!scManagerHandle)
            goto ErrorExit;

        InitializeListHead(&PhpNonPollServiceListHead);
        InitializeListHead(&PhpNonPollServicePendingListHead);

        if (!(services = PhEnumServices(scManagerHandle, 0, 0, &numberOfServices)))
            goto ErrorExit;

        for (i = 0; i < numberOfServices; i++)
        {
            SC_HANDLE serviceHandle;

            if (serviceHandle = OpenService(scManagerHandle, services[i].lpServiceName, SERVICE_QUERY_STATUS))
            {
                notifyContext = PhAllocate(sizeof(PHP_SERVICE_NOTIFY_CONTEXT));
                memset(notifyContext, 0, sizeof(PHP_SERVICE_NOTIFY_CONTEXT));
                notifyContext->ServiceHandle = serviceHandle;
                notifyContext->State = SnNotify;
                InsertTailList(&PhpNonPollServicePendingListHead, &notifyContext->ListEntry);
            }
        }

        PhFree(services);

        notifyContext = PhAllocate(sizeof(PHP_SERVICE_NOTIFY_CONTEXT));
        memset(notifyContext, 0, sizeof(PHP_SERVICE_NOTIFY_CONTEXT));
        notifyContext->ServiceHandle = scManagerHandle;
        notifyContext->IsServiceManager = TRUE;
        notifyContext->State = SnNotify;
        InsertTailList(&PhpNonPollServicePendingListHead, &notifyContext->ListEntry);

        while (TRUE)
        {
            BOOLEAN lagging = FALSE;

            listEntry = PhpNonPollServicePendingListHead.Flink;

            while (listEntry != &PhpNonPollServicePendingListHead)
            {
                notifyContext = CONTAINING_RECORD(listEntry, PHP_SERVICE_NOTIFY_CONTEXT, ListEntry);
                listEntry = listEntry->Flink;

                switch (notifyContext->State)
                {
                case SnNone:
                    break;
                case SnAdding:
                    notifyContext->ServiceHandle =
                        OpenService(scManagerHandle, notifyContext->ServiceName->Buffer, SERVICE_QUERY_STATUS);

                    if (!notifyContext->ServiceHandle)
                    {
                        RemoveEntryList(&notifyContext->ListEntry);
                        PhpDestroyServiceNotifyContext(notifyContext);
                        continue;
                    }

                    PhClearReference(&notifyContext->ServiceName);
                    notifyContext->State = SnNotify;
                    goto NotifyCase;
                case SnRemoving:
                    RemoveEntryList(&notifyContext->ListEntry);
                    PhpDestroyServiceNotifyContext(notifyContext);
                    break;
                case SnNotify:
NotifyCase:
                    memset(&notifyContext->Buffer, 0, sizeof(SERVICE_NOTIFY));
                    notifyContext->Buffer.dwVersion = SERVICE_NOTIFY_STATUS_CHANGE;
                    notifyContext->Buffer.pfnNotifyCallback = PhpServiceNonPollScNotifyCallback;
                    notifyContext->Buffer.pContext = notifyContext;
                    result = NotifyServiceStatusChangeW_I(
                        notifyContext->ServiceHandle,
                        notifyContext->IsServiceManager
                        ? (SERVICE_NOTIFY_CREATED | SERVICE_NOTIFY_DELETED)
                        : (SERVICE_NOTIFY_STOPPED | SERVICE_NOTIFY_START_PENDING | SERVICE_NOTIFY_STOP_PENDING |
                        SERVICE_NOTIFY_RUNNING | SERVICE_NOTIFY_CONTINUE_PENDING | SERVICE_NOTIFY_PAUSE_PENDING |
                        SERVICE_NOTIFY_PAUSED | SERVICE_NOTIFY_DELETE_PENDING),
                        &notifyContext->Buffer
                        );

                    switch (result)
                    {
                    case ERROR_SUCCESS:
                        notifyContext->State = SnNone;
                        RemoveEntryList(&notifyContext->ListEntry);
                        InsertTailList(&PhpNonPollServiceListHead, &notifyContext->ListEntry);
                        break;
                    case ERROR_SERVICE_NOTIFY_CLIENT_LAGGING:
                        // We are lagging behind. Re-open the handle to the SCM as soon as possible.
                        lagging = TRUE;
                        break;
                    case ERROR_SERVICE_MARKED_FOR_DELETE:
                    default:
                        RemoveEntryList(&notifyContext->ListEntry);
                        PhpDestroyServiceNotifyContext(notifyContext);
                        break;
                    }

                    break;
                }
            }

            while (NtWaitForSingleObject(PhpNonPollEventHandle, TRUE, NULL) != STATUS_WAIT_0)
                NOTHING;

            if (lagging)
                break;
        }

        // Execute all pending callbacks.
        NtTestAlert();

        listEntry = PhpNonPollServiceListHead.Flink;

        while (listEntry != &PhpNonPollServiceListHead)
        {
            notifyContext = CONTAINING_RECORD(listEntry, PHP_SERVICE_NOTIFY_CONTEXT, ListEntry);
            listEntry = listEntry->Flink;
            PhpDestroyServiceNotifyContext(notifyContext);
        }

        listEntry = PhpNonPollServicePendingListHead.Flink;

        while (listEntry != &PhpNonPollServicePendingListHead)
        {
            notifyContext = CONTAINING_RECORD(listEntry, PHP_SERVICE_NOTIFY_CONTEXT, ListEntry);
            listEntry = listEntry->Flink;
            PhpDestroyServiceNotifyContext(notifyContext);
        }

        CloseServiceHandle(scManagerHandle);
    }

    NtClose(PhpNonPollEventHandle);

    return STATUS_SUCCESS;

ErrorExit:
    PhpNonPollActive = FALSE;
    PhpNonPollGate = 1;
    NtClose(PhpNonPollEventHandle);
    return STATUS_UNSUCCESSFUL;
}

VOID PhpInitializeServiceNonPoll(
    VOID
    )
{
    // Dynamically import the required functions.

    NotifyServiceStatusChangeW_I = PhGetModuleProcAddress(L"advapi32.dll", "NotifyServiceStatusChangeW");

    if (!NotifyServiceStatusChangeW_I)
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
