/*
 * Process Hacker - 
 *   service provider
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

#define PH_SRVPRV_PRIVATE
#include <phapp.h>

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

PPH_OBJECT_TYPE PhServiceItemType;

PPH_HASHTABLE PhServiceHashtable;
PH_QUEUED_LOCK PhServiceHashtableLock = PH_QUEUED_LOCK_INIT;

PHAPPAPI PH_CALLBACK_DECLARE(PhServiceAddedEvent);
PHAPPAPI PH_CALLBACK_DECLARE(PhServiceModifiedEvent);
PHAPPAPI PH_CALLBACK_DECLARE(PhServiceRemovedEvent);
PHAPPAPI PH_CALLBACK_DECLARE(PhServicesUpdatedEvent);

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
        sizeof(PH_SERVICE_ITEM),
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

    return serviceItem;
}

VOID PhpServiceItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)Object;

    if (serviceItem->Name) PhDereferenceObject(serviceItem->Name);
    if (serviceItem->DisplayName) PhDereferenceObject(serviceItem->DisplayName);
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
    WCHAR upperName[257];

    // Service names are case-insensitive, so we'll uppercase 
    // the given service name and then hash it.

    // Check the length. Should never be above 256, but we have 
    // to make sure.
    if (serviceItem->Key.Length > 256 * sizeof(WCHAR))
        return 0;

    // Copy the name and convert it to uppercase.
    memcpy(upperName, serviceItem->Key.Buffer, serviceItem->Key.Length);
    upperName[serviceItem->Key.Length / sizeof(WCHAR)] = 0; // null terminator
    _wcsupr(upperName);

    // Hash the string.
    return PhHashBytes((PUCHAR)upperName, serviceItem->Key.Length);
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
    ULONG enumerationKey = 0;
    PPH_SERVICE_ITEM *serviceItem;

    // We don't need to lock as long as the service provider 
    // never runs concurrently with the process provider. This 
    // is currently true.

    while (PhEnumHashtable(PhServiceHashtable, (PPVOID)&serviceItem, &enumerationKey))
    {
        if (
            (*serviceItem)->PendingProcess &&
            (*serviceItem)->ProcessId == ProcessItem->ProcessId
            )
        {
            PhpAddProcessItemService(ProcessItem, *serviceItem);
            (*serviceItem)->PendingProcess = FALSE;
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
        ULONG enumerationKey = 0;
        PPH_SERVICE_ITEM *serviceItem;

        while (PhEnumHashtable(PhServiceHashtable, (PPVOID)&serviceItem, &enumerationKey))
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
                            serviceItem->PendingProcess = FALSE;
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

    PhInvokeCallback(&PhServicesUpdatedEvent, NULL);
    runCount++;
}
