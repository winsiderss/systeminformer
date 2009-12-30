#define SRVPRV_PRIVATE
#include <ph.h>

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

PPH_OBJECT_TYPE PhServiceItemType;

PPH_HASHTABLE PhServiceHashtable;
PH_FAST_LOCK PhServiceHashtableLock;

PH_CALLBACK PhServiceAddedEvent;
PH_CALLBACK PhServiceModifiedEvent;
PH_CALLBACK PhServiceRemovedEvent;

BOOLEAN PhInitializeServiceProvider()
{
    if (!NT_SUCCESS(PhCreateObjectType(
        &PhServiceItemType,
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
    PhInitializeFastLock(&PhServiceHashtableLock);

    PhInitializeCallback(&PhServiceAddedEvent);
    PhInitializeCallback(&PhServiceModifiedEvent);
    PhInitializeCallback(&PhServiceRemovedEvent);

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
        PhServiceItemType,
        0
        )))
        return NULL;

    memset(serviceItem, 0, sizeof(PH_SERVICE_ITEM));

    if (Information)
    {
        serviceItem->Name = PhCreateString(Information->lpServiceName);
        serviceItem->DisplayName = PhCreateString(Information->lpDisplayName);
        serviceItem->Type = Information->ServiceStatusProcess.dwServiceType;
        serviceItem->State = Information->ServiceStatusProcess.dwCurrentState;
        serviceItem->ControlsAccepted = Information->ServiceStatusProcess.dwControlsAccepted;
        serviceItem->ProcessId = Information->ServiceStatusProcess.dwProcessId;
        PhPrintInteger(serviceItem->ProcessIdString, serviceItem->ProcessId);
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

    return PhStringEquals(serviceItem1->Name, serviceItem2->Name, TRUE);
}

ULONG PhpServiceHashtableHashFunction(
    __in PVOID Entry
    )
{
    PPH_SERVICE_ITEM serviceItem = *(PPH_SERVICE_ITEM *)Entry;
    WCHAR lowerName[257];

    // Service names are case-insensitive, so we'll lowercase 
    // the given service name and then hash it.

    // Check the length. Should never be above 256, but we have 
    // to make sure.
    if (serviceItem->Name->Length > 256)
        return 0;

    // Copy the name and convert it to lowercase.
    memcpy(lowerName, serviceItem->Name->Buffer, serviceItem->Name->Length);
    lowerName[serviceItem->Name->Length / sizeof(WCHAR)] = 0; // null terminator
    _wcslwr(lowerName);

    // Hash the string.
    return PhHashBytesSdbm((PUCHAR)lowerName, serviceItem->Name->Length);
}

PPH_SERVICE_ITEM PhReferenceServiceItem(
    __in PWSTR Name
    )
{
    PH_SERVICE_ITEM lookupServiceItem;
    PPH_SERVICE_ITEM lookupServiceItemPtr = &lookupServiceItem;
    PPH_SERVICE_ITEM *serviceItemPtr;
    PPH_SERVICE_ITEM serviceItem;

    // Construct a temporary service item for the lookup.
    lookupServiceItem.Name = PhCreateString(Name);

    PhAcquireFastLockShared(&PhServiceHashtableLock);

    serviceItemPtr = (PPH_SERVICE_ITEM *)PhGetHashtableEntry(
        PhServiceHashtable,
        &lookupServiceItemPtr
        );

    if (serviceItemPtr)
    {
        serviceItem = *serviceItemPtr;
        PhReferenceObject(serviceItem);
    }
    else
    {
        serviceItem = NULL;
    }

    PhReleaseFastLockShared(&PhServiceHashtableLock);

    PhDereferenceObject(lookupServiceItem.Name);

    return serviceItem;
}

VOID PhpRemoveServiceItem(
    __in PPH_SERVICE_ITEM ServiceItem
    )
{
    PhRemoveHashtableEntry(PhServiceHashtable, &ServiceItem);
    PhDereferenceObject(ServiceItem);
}

PVOID PhEnumServices(
    __in SC_HANDLE ScManagerHandle,
    __in_opt ULONG Type,
    __in_opt ULONG State,
    __out PULONG Count
    )
{
    PVOID buffer;
    static ULONG bufferSize = 0x8000;
    ULONG returnLength;
    ULONG servicesReturned;

    if (!Type)
        Type = SERVICE_DRIVER | SERVICE_WIN32;
    if (!State)
        State = SERVICE_STATE_ALL;

    buffer = PhAllocate(bufferSize);

    if (!EnumServicesStatusEx(
        ScManagerHandle,
        SC_ENUM_PROCESS_INFO,
        Type,
        State,
        buffer,
        bufferSize,
        &returnLength,
        &servicesReturned,
        NULL,
        NULL
        ))
    {
        if (GetLastError() == ERROR_MORE_DATA)
        {
            PhFree(buffer);
            bufferSize += returnLength;
            buffer = PhAllocate(bufferSize);

            if (!EnumServicesStatusEx(
                ScManagerHandle,
                SC_ENUM_PROCESS_INFO,
                Type,
                State,
                buffer,
                bufferSize,
                &returnLength,
                &servicesReturned,
                NULL,
                NULL
                ))
            {
                PhFree(buffer);
                return NULL;
            }
        }
    }

    *Count = servicesReturned;

    return buffer;
}

PVOID PhQueryServiceConfig(
    __in SC_HANDLE ServiceHandle
    )
{
    PVOID buffer;
    ULONG bufferSize = 0x100;

    buffer = PhAllocate(bufferSize);

    if (!QueryServiceConfig(ServiceHandle, buffer, bufferSize, &bufferSize))
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        if (!QueryServiceConfig(ServiceHandle, buffer, bufferSize, &bufferSize))
        {
            PhFree(buffer);
            return NULL;
        }
    }

    return buffer;
}

VOID PhServiceProviderUpdate(
    __in PVOID Context
    )
{
    static SC_HANDLE scManagerHandle = NULL;
    static ULONG runCount = 0;

    LPENUM_SERVICE_STATUS_PROCESS services;
    ULONG numberOfServices;
    ULONG i;

    if (scManagerHandle == NULL)
    {
        scManagerHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);
    }

    services = PhEnumServices(scManagerHandle, 0, 0, &numberOfServices);

    if (!services)
        return;

    // Look for dead services.
    {
        PPH_LIST servicesToRemove = NULL;
        ULONG enumerationKey = 0;
        PPH_SERVICE_ITEM *serviceItem;

        while (PhEnumHashtable(PhServiceHashtable, (PPVOID)&serviceItem, &enumerationKey))
        {
            BOOLEAN found = FALSE;

            // Check if the service still exists.
            for (i = 0; i < numberOfServices; i++)
            {
                if (PhStringEquals2((*serviceItem)->Name, services[i].lpServiceName, TRUE))
                {
                    found = TRUE;
                    break;
                }
            }

            if (!found)
            {
                // Raise the service removed event.
                PhInvokeCallback(&PhServiceRemovedEvent, *serviceItem);

                if (!servicesToRemove)
                    servicesToRemove = PhCreateList(2);

                PhAddListItem(servicesToRemove, *serviceItem);
            }
        }

        if (servicesToRemove)
        {
            PhAcquireFastLockExclusive(&PhServiceHashtableLock);

            for (i = 0; i < servicesToRemove->Count; i++)
            {
                PhpRemoveServiceItem((PPH_SERVICE_ITEM)servicesToRemove->Items[i]);
            }

            PhReleaseFastLockExclusive(&PhServiceHashtableLock);
            PhDereferenceObject(servicesToRemove);
        }
    }

    // Look for new services and update existing ones.
    for (i = 0; i < numberOfServices; i++)
    {
        PPH_SERVICE_ITEM serviceItem;

        serviceItem = PhReferenceServiceItem(services[i].lpServiceName);

        if (!serviceItem)
        {
            // Create the service item and fill in basic information.
            serviceItem = PhCreateServiceItem(&services[i]);

            {
                SC_HANDLE serviceHandle;

                serviceHandle = OpenService(scManagerHandle, serviceItem->Name->Buffer, SERVICE_QUERY_CONFIG);

                if (serviceHandle)
                {
                    LPQUERY_SERVICE_CONFIG config;

                    config = PhQueryServiceConfig(serviceHandle);

                    if (config)
                    {
                        serviceItem->StartType = config->dwStartType;
                        serviceItem->ErrorControl = config->dwErrorControl;

                        PhFree(config);
                    }

                    CloseServiceHandle(serviceHandle);
                }
            }

            // Add the service item to the hashtable.
            PhAcquireFastLockExclusive(&PhServiceHashtableLock);
            PhAddHashtableEntry(PhServiceHashtable, &serviceItem);
            PhReleaseFastLockExclusive(&PhServiceHashtableLock);

            // Raise the service added event.
            PhInvokeCallback(&PhServiceAddedEvent, serviceItem);
        }
        else
        {
            if (
                serviceItem->Type != services[i].ServiceStatusProcess.dwServiceType || 
                serviceItem->State != services[i].ServiceStatusProcess.dwCurrentState ||
                serviceItem->ControlsAccepted != services[i].ServiceStatusProcess.dwControlsAccepted ||
                serviceItem->ProcessId != services[i].ServiceStatusProcess.dwProcessId
                )
            {
                PH_SERVICE_MODIFIED_DATA serviceModifiedData;

                // The service has been "modified".

                serviceModifiedData.Service = serviceItem;
                memset(&serviceModifiedData.OldService, 0, sizeof(PH_SERVICE_ITEM));
                serviceModifiedData.OldService.Type = serviceItem->Type;
                serviceModifiedData.OldService.State = serviceItem->State;
                serviceModifiedData.OldService.ControlsAccepted = serviceItem->ControlsAccepted;
                serviceModifiedData.OldService.ProcessId = serviceItem->ProcessId;

                // Update the service item.
                serviceItem->Type = services[i].ServiceStatusProcess.dwServiceType;
                serviceItem->State = services[i].ServiceStatusProcess.dwCurrentState;
                serviceItem->ControlsAccepted = services[i].ServiceStatusProcess.dwControlsAccepted;
                serviceItem->ProcessId = services[i].ServiceStatusProcess.dwProcessId;
                PhPrintInteger(serviceItem->ProcessIdString, serviceItem->ProcessId);

                // Raise the service modified event.
                PhInvokeCallback(&PhServiceModifiedEvent, &serviceModifiedData);
            }

            PhDereferenceObject(serviceItem);
        }
    }

    PhFree(services);

    runCount++;
}
