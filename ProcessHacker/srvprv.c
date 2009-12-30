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

    // Check the length. Should never be above 256, but we have 
    // to make sure.
    if (serviceItem->Name->Length > 256)
        return 0;

    // Copy the name and convert it to lowercase.
    memcpy(lowerName, serviceItem->Name->Buffer, serviceItem->Name->Length);
    lowerName[serviceItem->Name->Length / sizeof(WCHAR)] = 0; // null terminator
    _wcslwr(lowerName);

    // Hash the string.
    return PhHashBytes((PUCHAR)lowerName, serviceItem->Name->Length);
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
    __in PWSTR Name
    )
{
    PH_SERVICE_ITEM lookupServiceItem;
    PPH_SERVICE_ITEM lookupServiceItemPtr = &lookupServiceItem;
    PPH_SERVICE_ITEM *serviceItemPtr;

    lookupServiceItem.Name = PhCreateString(Name);

    serviceItemPtr = PhGetHashtableEntry(PhServiceHashtable, &lookupServiceItemPtr);

    if (serviceItemPtr)
    {
        PhRemoveHashtableEntry(PhServiceHashtable, &lookupServiceItemPtr);
        PhDereferenceObject(*serviceItemPtr);
    }

    PhDereferenceObject(lookupServiceItem.Name);
}

PVOID PhEnumerateServices(
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

VOID PhUpdateServices()
{
    
}
