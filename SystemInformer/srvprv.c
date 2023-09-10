/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2015
 *     dmex    2017-2023
 *
 */

#include <phapp.h>
#include <phplug.h>
#include <srvprv.h>
#include <svcsup.h>
#include <workqueue.h>
#include <extmgri.h>
#include <procprv.h>
#include <phsettings.h>
#include <apiimport.h>
#include <mapldr.h>

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
    PPH_STRING ServiceName;
    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN IsServiceManager : 1;
            BOOLEAN NonPollDisabled : 1;
            BOOLEAN Spare : 6;
        };
    };
    PHP_SERVICE_NOTIFY_STATE State;
    SERVICE_NOTIFY Buffer;
} PHP_SERVICE_NOTIFY_CONTEXT, *PPHP_SERVICE_NOTIFY_CONTEXT;

typedef struct _PH_SERVICE_QUERY_DATA
{
    SLIST_ENTRY ListEntry;
    ULONG Stage;
    PPH_SERVICE_ITEM ServiceItem;
    PPH_STRING FileName;
} PH_SERVICE_QUERY_DATA, *PPH_SERVICE_QUERY_DATA;

typedef struct _PH_SERVICE_QUERY_S1_DATA
{
    PH_SERVICE_QUERY_DATA Header;

    PPH_IMAGELIST_ITEM IconEntry;
} PH_SERVICE_QUERY_S1_DATA, *PPH_SERVICE_QUERY_S1_DATA;

typedef struct _PH_SERVICE_QUERY_S2_DATA
{
    PH_SERVICE_QUERY_DATA Header;

    VERIFY_RESULT VerifyResult;
    PPH_STRING VerifySignerName;
} PH_SERVICE_QUERY_S2_DATA, *PPH_SERVICE_QUERY_S2_DATA;

VOID NTAPI PhpServiceItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

BOOLEAN NTAPI PhpServiceHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

ULONG NTAPI PhpServiceHashtableHashFunction(
    _In_ PVOID Entry
    );

VOID PhAddProcessItemService(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_SERVICE_ITEM ServiceItem
    );

VOID PhRemoveProcessItemService(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_SERVICE_ITEM ServiceItem
    );

VOID PhSubscribeServiceChangeNotifications(
    _In_ PPH_SERVICE_ITEM ServiceItem
    );

VOID PhInitializeServiceNonPoll(
    VOID
    );

VOID PhCreateServiceNotifyCallback(
    _In_ PPH_SERVICE_ITEM ServiceItem,
    _In_ SC_HANDLE ServiceHandle
    );
VOID PhDestroyServiceNotifyCallback(
    _In_ PPH_SERVICE_ITEM ServiceItem
    );

PPH_OBJECT_TYPE PhServiceItemType = NULL;
PPH_HASHTABLE PhServiceHashtable = NULL;
PH_QUEUED_LOCK PhServiceHashtableLock = PH_QUEUED_LOCK_INIT;

BOOLEAN PhEnableServiceNonPoll = FALSE;
BOOLEAN PhEnableServiceNonPollNotify = FALSE;
ULONG PhServiceNonPollFlushInterval = 10;
static BOOLEAN PhNonPollActive = FALSE;
static volatile LONG PhNonPollGate = 0;
static HANDLE PhNonPollEventHandle = NULL;
static LIST_ENTRY PhpNonPollServiceListHead = { &PhpNonPollServiceListHead, &PhpNonPollServiceListHead };
static LIST_ENTRY PhpNonPollServicePendingListHead = { &PhpNonPollServicePendingListHead, &PhpNonPollServicePendingListHead };
static SLIST_HEADER PhpServiceQueryDataListHead;
static _NotifyServiceStatusChangeW NotifyServiceStatusChange_I = NULL;
static _SubscribeServiceChangeNotifications SubscribeServiceChangeNotifications_I = NULL;
static _UnsubscribeServiceChangeNotifications UnsubscribeServiceChangeNotifications_I = NULL;

BOOLEAN PhServiceProviderInitialization(
    VOID
    )
{
    PhServiceItemType = PhCreateObjectType(L"ServiceItem", 0, PhpServiceItemDeleteProcedure);
    PhServiceHashtable = PhCreateHashtable(
        sizeof(PPH_SERVICE_ITEM),
        PhpServiceHashtableEqualFunction,
        PhpServiceHashtableHashFunction,
        40
        );

    RtlInitializeSListHead(&PhpServiceQueryDataListHead);

    if (PhEnableServiceNonPoll)
    {
        PhInitializeServiceNonPoll();
    }

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

        if (PH_IS_REAL_PROCESS_ID(serviceItem->ProcessId))
        {
            PhPrintUInt32(serviceItem->ProcessIdString, HandleToUlong(serviceItem->ProcessId));
        }
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
    if (serviceItem->FileName) PhDereferenceObject(serviceItem->FileName);
    if (serviceItem->VerifySignerName) PhDereferenceObject(serviceItem->VerifySignerName);
    if (serviceItem->IconEntry) PhDereferenceObject(serviceItem->IconEntry);
    //PhDeleteImageVersionInfo(&serviceItem->VersionInfo);
}

BOOLEAN PhpServiceHashtableEqualFunction(
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

    return PhHashStringRefEx(&serviceItem->Key, TRUE, PH_STRING_HASH_X65599);
}

PPH_SERVICE_ITEM PhpLookupServiceItem(
    _In_ PPH_STRINGREF Name
    )
{
    PH_SERVICE_ITEM lookupServiceItem;
    PPH_SERVICE_ITEM lookupServiceItemPtr = &lookupServiceItem;
    PPH_SERVICE_ITEM *serviceItem;

    // Construct a temporary service item for the lookup.
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

    PhInitializeStringRefLongHint(&key, Name);

    PhAcquireQueuedLockShared(&PhServiceHashtableLock);

    serviceItem = PhpLookupServiceItem(&key);

    if (serviceItem)
        PhReferenceObject(serviceItem);

    PhReleaseQueuedLockShared(&PhServiceHashtableLock);

    return serviceItem;
}

VOID PhpResetServiceNonPollGate(
    VOID
    )
{
    if (PhEnableServiceNonPoll)
    {
        InterlockedExchange(&PhNonPollGate, TRUE);
    }
}

VOID PhMarkNeedsConfigUpdateServiceItem(
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    ServiceItem->NeedsConfigUpdate = TRUE;

    PhpResetServiceNonPollGate();
}

VOID PhpRemoveServiceItem(
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    PhRemoveEntryHashtable(PhServiceHashtable, &ServiceItem);

    PhDestroyServiceNotifyCallback(ServiceItem);

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
        Data->ServiceItem->State == SERVICE_RUNNING
        )
    {
        return ServiceStarted;
    }

    if (
        (
        Data->OldService.State == SERVICE_PAUSED ||
        Data->OldService.State == SERVICE_CONTINUE_PENDING
        ) &&
        Data->ServiceItem->State == SERVICE_RUNNING
        )
    {
        return ServiceContinued;
    }

    if (
        (
        Data->OldService.State == SERVICE_RUNNING ||
        Data->OldService.State == SERVICE_PAUSE_PENDING
        ) &&
        Data->ServiceItem->State == SERVICE_PAUSED
        )
    {
        return ServicePaused;
    }

    if (
        (
        Data->OldService.State == SERVICE_RUNNING ||
        Data->OldService.State == SERVICE_STOP_PENDING
        ) &&
        Data->ServiceItem->State == SERVICE_STOPPED
        )
    {
        return ServiceStopped;
    }

    // TODO: ServiceModified (dmex)

    return ULONG_MAX;
}

VOID PhUpdateProcessItemServices(
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PPH_SERVICE_ITEM *serviceItem;

    // We don't need to lock as long as the service provider
    // never runs concurrently with the process provider. This
    // is currently true. (wj32)

    // Starting 2022 the service provider runs concurrently with
    // the process provider and now requires locking. If the service
    // provider is moved back to the primary provider thread then
    // remove these locks per the above comments (dmex)

    PhAcquireQueuedLockShared(&PhServiceHashtableLock);

    PhBeginEnumHashtable(PhServiceHashtable, &enumContext);

    while (serviceItem = PhNextEnumHashtable(&enumContext))
    {
        if (
            (*serviceItem)->PendingProcess &&
            (*serviceItem)->ProcessId == ProcessItem->ProcessId
            )
        {
            PhAddProcessItemService(ProcessItem, *serviceItem);
        }
    }

    PhReleaseQueuedLockShared(&PhServiceHashtableLock);
}

VOID PhAddProcessItemService(
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
    InterlockedExchange(&ProcessItem->JustProcessed, TRUE);
}

VOID PhRemoveProcessItemService(
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

    InterlockedExchange(&ProcessItem->JustProcessed, TRUE);
}

static BOOLEAN PhCompareServiceNameEntry(
    _In_ PPHP_SERVICE_NAME_ENTRY Value1,
    _In_ PPHP_SERVICE_NAME_ENTRY Value2
    )
{
    return PhEqualStringRef(&Value1->Name, &Value2->Name, TRUE);
}

static ULONG PhHashServiceNameEntry(
    _In_ PPHP_SERVICE_NAME_ENTRY Value
    )
{
    return PhHashStringRefEx(&Value->Name, TRUE, PH_STRING_HASH_X65599);
}

VOID PhServiceQueryStage1(
    _Inout_ PPH_SERVICE_QUERY_S1_DATA Data
    )
{
    PPH_SERVICE_ITEM serviceItem = Data->Header.ServiceItem;
    PPH_STRING fileName = Data->Header.FileName;

    if (fileName)
    {
        if (!FlagOn(serviceItem->Type, SERVICE_DRIVER)) // Skip icons for kernel drivers (dmex)
        {
            Data->IconEntry = PhImageListExtractIcon(fileName, FALSE, 0, NULL, PhSystemDpi);
        }

        // Version info.
        //PhInitializeImageVersionInfo(&Data->VersionInfo, fileName->Buffer);
    }
}

VOID PhServiceQueryStage2(
    _Inout_ PPH_SERVICE_QUERY_S2_DATA Data
    )
{
    PPH_SERVICE_ITEM serviceItem = Data->Header.ServiceItem;
    PPH_STRING fileName = Data->Header.FileName;

    if (fileName)
    {
        Data->VerifyResult = PhVerifyFileCached(
            fileName,
            NULL,
            &Data->VerifySignerName,
            FALSE,
            FALSE
            );
    }
}

NTSTATUS PhpServiceQueryStage1Worker(
    _In_ PVOID Parameter
    )
{
    PPH_SERVICE_QUERY_S1_DATA data = Parameter;

    PhServiceQueryStage1(data);

    RtlInterlockedPushEntrySList(&PhpServiceQueryDataListHead, &data->Header.ListEntry);

    return STATUS_SUCCESS;
}

NTSTATUS PhpServiceQueryStage2Worker(
    _In_ PVOID Parameter
    )
{
    PPH_SERVICE_QUERY_S2_DATA data = Parameter;

    PhServiceQueryStage2(data);

    RtlInterlockedPushEntrySList(&PhpServiceQueryDataListHead, &data->Header.ListEntry);

    return STATUS_SUCCESS;
}

VOID PhQueueServiceQueryStage1(
    _Inout_ PPH_SERVICE_QUERY_S1_DATA Data
    )
{
    PH_WORK_QUEUE_ENVIRONMENT environment;

    PhInitializeWorkQueueEnvironment(&environment);
    environment.BasePriority = THREAD_PRIORITY_BELOW_NORMAL;
    environment.IoPriority = IoPriorityLow;
    environment.PagePriority = MEMORY_PRIORITY_LOW;

    PhQueueItemWorkQueueEx(PhGetGlobalWorkQueue(), PhpServiceQueryStage1Worker, Data, NULL, &environment);
}

VOID PhQueueServiceQueryStage2(
    _Inout_ PPH_SERVICE_QUERY_S2_DATA Data
    )
{
    PH_WORK_QUEUE_ENVIRONMENT environment;

    PhInitializeWorkQueueEnvironment(&environment);
    environment.BasePriority = THREAD_PRIORITY_BELOW_NORMAL;
    environment.IoPriority = IoPriorityVeryLow;
    environment.PagePriority = MEMORY_PRIORITY_VERY_LOW;

    PhQueueItemWorkQueueEx(PhGetGlobalWorkQueue(), PhpServiceQueryStage2Worker, Data, NULL, &environment);
}

VOID PhCreateQueueServiceQueryStage2(
    _In_ PPH_SERVICE_ITEM ServiceItem,
    _In_ PPH_STRING FileName
    )
{
    PPH_SERVICE_QUERY_S2_DATA data;

    ServiceItem->VerifyResult = 0;
    PhClearReference(&ServiceItem->VerifySignerName);

    data = PhAllocateZero(sizeof(PH_SERVICE_QUERY_S2_DATA));
    data->Header.Stage = 2;
    PhSetReference(&data->Header.ServiceItem, ServiceItem);
    PhSetReference(&data->Header.FileName, FileName);

    PhQueueServiceQueryStage2(data);
}

VOID PhpFillServiceItemStage1(
    _In_ PPH_SERVICE_QUERY_S1_DATA Data
    )
{
    PPH_SERVICE_ITEM serviceItem = Data->Header.ServiceItem;
    PPH_STRING fileName = Data->Header.FileName;

    serviceItem->IconEntry = Data->IconEntry;
    //memcpy(&processItem->VersionInfo, &Data->VersionInfo, sizeof(PH_IMAGE_VERSION_INFO));

    // Note: Queue stage 2 processing after filling stage1 process data.

    if (PhEnableServiceQueryStage2)
    {
        PhCreateQueueServiceQueryStage2(serviceItem, fileName);
    }
}

VOID PhpFillServiceItemStage2(
    _In_ PPH_SERVICE_QUERY_S2_DATA Data
    )
{
    PPH_SERVICE_ITEM serviceItem = Data->Header.ServiceItem;

    serviceItem->VerifyResult = Data->VerifyResult;
    PhMoveReference(&serviceItem->VerifySignerName, Data->VerifySignerName);
}

VOID PhFlushServiceQueryData(
    VOID
    )
{
    PSLIST_ENTRY entry;
    PPH_SERVICE_QUERY_DATA data;

    //if (!RtlFirstEntrySList(&PhpServiceQueryDataListHead))
    //    return;

    entry = RtlInterlockedFlushSList(&PhpServiceQueryDataListHead);

    while (entry)
    {
        data = CONTAINING_RECORD(entry, PH_SERVICE_QUERY_DATA, ListEntry);
        entry = entry->Next;

        if (data->Stage == 1)
        {
            PhpFillServiceItemStage1((PPH_SERVICE_QUERY_S1_DATA)data);
        }
        else if (data->Stage == 2)
        {
            PhpFillServiceItemStage2((PPH_SERVICE_QUERY_S2_DATA)data);
        }

        InterlockedExchange(&data->ServiceItem->JustProcessed, TRUE);

        if (data->FileName) PhDereferenceObject(data->FileName);
        PhDereferenceObject(data->ServiceItem);
        PhFree(data);
    }
}

VOID PhUpdateServiceItemConfig(
    _In_ PPH_SERVICE_ITEM ServiceItem,
    _In_ BOOLEAN CreateServiceNotifyCallback
    )
{
    NTSTATUS status;
    SC_HANDLE serviceHandle;
    LPQUERY_SERVICE_CONFIG config;
    BOOLEAN delayedAutoStartInfo;

    status = PhOpenService(
        &serviceHandle,
        SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG,
        PhGetString(ServiceItem->Name)
        );

    if (!NT_SUCCESS(status))
    {
        status = PhOpenService(
            &serviceHandle,
            SERVICE_QUERY_CONFIG,
            PhGetString(ServiceItem->Name)
            );
    }

    if (!NT_SUCCESS(status))
        return;

    if (NT_SUCCESS(PhGetServiceConfig(serviceHandle, &config)))
    {
        ServiceItem->StartType = config->dwStartType;
        ServiceItem->ErrorControl = config->dwErrorControl;

        {
            PPH_STRING fileName;

            fileName = PhGetServiceConfigFileName(
                config->dwServiceType,
                config->lpBinaryPathName,
                &ServiceItem->Name->sr
                );

            if (fileName && ServiceItem->FileName && !PhEqualStringRef(&fileName->sr, &ServiceItem->FileName->sr, TRUE))
            {
                if (PhEnableServiceQueryStage2)
                {
                    PhCreateQueueServiceQueryStage2(ServiceItem, fileName);
                }
            }

            PhMoveReference(&ServiceItem->FileName, fileName);
        }

        PhFree(config);
    }

    if (PhGetServiceDelayedAutoStart(serviceHandle, &delayedAutoStartInfo))
        ServiceItem->DelayedStart = delayedAutoStartInfo;
    else
        ServiceItem->DelayedStart = FALSE;

    if (PhGetServiceTriggerInfo(serviceHandle, NULL))
        ServiceItem->HasTriggers = TRUE;
    else
        ServiceItem->HasTriggers = FALSE;

    if (CreateServiceNotifyCallback)
    {
        PhCreateServiceNotifyCallback(ServiceItem, serviceHandle);
    }

    PhCloseServiceHandle(serviceHandle);
}

VOID PhServiceProviderUpdate(
    _In_ PVOID Object
    )
{
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
        if (PhNonPollActive)
        {
            if (InterlockedExchange(&PhNonPollGate, 0) == 0)
            {
                // The SCM doesn't generate notifications for drivers. So flush service
                // information once in a while so we can detect driver events. (dmex)
                if (runCount % PhServiceNonPollFlushInterval == 0)
                    goto UpdateStart;

                // Go through the queued services query data.
                PhFlushServiceQueryData();

                // Non-poll gate is closed; skip all processing.
                goto UpdateEnd;
            }
        }
    }

UpdateStart:

    if (!NT_SUCCESS(PhEnumServices(&services, &numberOfServices)))
        return;

    // Build a hash set containing the service names.

    // This has caused a massive decrease in background CPU usage, and
    // is certainly much better than the quadratic-time string comparisons
    // we were doing before (in the "Look for dead services" section). (wj32)

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

        if (WindowsVersion >= WINDOWS_10_RS2)
        {
            PhServiceWorkaroundWindowsServiceTypeBug(entry->ServiceEntry);
        }

        PhAddEntryHashSet(
            nameHashSet,
            PH_HASH_SET_SIZE(nameHashSet),
            &entry->HashEntry,
            PhHashServiceNameEntry(entry)
            );
    }

    // Go through the queued services query data.
    PhFlushServiceQueryData();

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
                PhHashServiceNameEntry(&lookupNameEntry)
                );

            for (; hashEntry; hashEntry = hashEntry->Next)
            {
                PPHP_SERVICE_NAME_ENTRY nameEntry;

                nameEntry = CONTAINING_RECORD(hashEntry, PHP_SERVICE_NAME_ENTRY, HashEntry);

                if (PhCompareServiceNameEntry(&lookupNameEntry, nameEntry))
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

                    processItem = PhReferenceProcessItem((*serviceItem)->ProcessId);

                    if (processItem)
                    {
                        PhRemoveProcessItemService(processItem, *serviceItem);
                        PhDereferenceObject(processItem);
                    }
                }

                // Raise the service removed event.
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackServiceProviderRemovedEvent), *serviceItem);

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
                PhUpdateServiceItemConfig(serviceItem, TRUE);

                // Add the service to its process, if appropriate.
                if (serviceItem->ProcessId)
                {
                    PPH_PROCESS_ITEM processItem;

                    if (processItem = PhReferenceProcessItem(serviceItem->ProcessId))
                    {
                        PhAddProcessItemService(processItem, serviceItem);
                        PhDereferenceObject(processItem);
                    }
                    else
                    {
                        // The process doesn't exist yet (to us). Set the pending
                        // flag and when the process is added this will be
                        // fixed. (wj32)
                        serviceItem->PendingProcess = TRUE;
                    }
                }

                // If this is the first run of the provider, queue the
                // process query tasks. Otherwise, perform stage 1
                // processing now and queue stage 2 processing. (wj32)
                if (runCount > 0)
                {
                    PH_SERVICE_QUERY_S1_DATA data;

                    memset(&data, 0, sizeof(PH_SERVICE_QUERY_S1_DATA));
                    data.Header.Stage = 1;
                    data.Header.ServiceItem = serviceItem;
                    data.Header.FileName = serviceItem->FileName;

                    PhServiceQueryStage1(&data);
                    PhpFillServiceItemStage1(&data);
                }
                else
                {
                    PPH_SERVICE_QUERY_S1_DATA data;

                    data = PhAllocateZero(sizeof(PH_SERVICE_QUERY_S1_DATA));
                    data->Header.Stage = 1;
                    PhSetReference(&data->Header.ServiceItem, serviceItem);
                    PhSetReference(&data->Header.FileName, serviceItem->FileName);

                    PhQueueServiceQueryStage1(data);
                }

                // Add the service item to the hashtable.
                PhAcquireQueuedLockExclusive(&PhServiceHashtableLock);
                PhAddEntryHashtable(PhServiceHashtable, &serviceItem);
                PhReleaseQueuedLockExclusive(&PhServiceHashtableLock);

                // Raise the service added event.
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackServiceProviderAddedEvent), serviceItem);
            }
            else
            {
                if (
                    serviceItem->Type != serviceEntry->ServiceStatusProcess.dwServiceType ||
                    serviceItem->State != serviceEntry->ServiceStatusProcess.dwCurrentState ||
                    serviceItem->ControlsAccepted != serviceEntry->ServiceStatusProcess.dwControlsAccepted ||
                    serviceItem->ProcessId != UlongToHandle(serviceEntry->ServiceStatusProcess.dwProcessId) ||
                    serviceItem->NeedsConfigUpdate ||
                    serviceItem->JustProcessed
                    )
                {
                    PH_SERVICE_MODIFIED_DATA serviceModifiedData;
                    PH_SERVICE_CHANGE serviceChange;

                    // The service has been "modified".

                    serviceModifiedData.ServiceItem = serviceItem;
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

                    if (PH_IS_REAL_PROCESS_ID(serviceItem->ProcessId))
                        PhPrintUInt32(serviceItem->ProcessIdString, HandleToUlong(serviceItem->ProcessId));
                    else
                        serviceItem->ProcessIdString[0] = UNICODE_NULL;

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
                                PhAddProcessItemService(processItem, serviceItem);
                            else
                                PhRemoveProcessItemService(processItem, serviceItem);

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
                        // is in the process ID. (wj32)

                        if (processItem = PhReferenceProcessItem(serviceModifiedData.OldService.ProcessId))
                        {
                            PhRemoveProcessItemService(processItem, serviceItem);
                            PhDereferenceObject(processItem);
                        }

                        if (processItem = PhReferenceProcessItem(serviceItem->ProcessId))
                        {
                            PhAddProcessItemService(processItem, serviceItem);
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
                        PhUpdateServiceItemConfig(serviceItem, FALSE);
                        serviceItem->NeedsConfigUpdate = FALSE;
                    }

                    InterlockedExchange(&serviceItem->JustProcessed, FALSE);

                    // Raise the service modified event.
                    PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackServiceProviderModifiedEvent), &serviceModifiedData);
                }
            }
        }
    }

    PhFree(services);

UpdateEnd:
    PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackServiceProviderUpdatedEvent), NULL);
    runCount++;
}
VOID CALLBACK PhServiceNotifyNonPollCallback(
    _In_ PVOID pParameter
    )
{
    PSERVICE_NOTIFYW notifyBuffer = pParameter;
    PPHP_SERVICE_NOTIFY_CONTEXT notifyContext = notifyBuffer->pContext;

    if (notifyBuffer->dwNotificationStatus == ERROR_SUCCESS)
    {
        if (FlagOn(notifyBuffer->dwNotificationTriggered, SERVICE_NOTIFY_CREATED | SERVICE_NOTIFY_DELETED) && notifyBuffer->pszServiceNames)
        {
            PWSTR name;
            SIZE_T nameLength;

            name = notifyBuffer->pszServiceNames;

            while (TRUE)
            {
                nameLength = PhCountStringZ(name);

                if (nameLength == 0)
                    break;

                if (name[0] == L'/')
                {
                    PPHP_SERVICE_NOTIFY_CONTEXT newNotifyContext;

                    // Service creation
                    newNotifyContext = PhAllocateZero(sizeof(PHP_SERVICE_NOTIFY_CONTEXT));
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

    PhpResetServiceNonPollGate();
    NtSetEvent(PhNonPollEventHandle, NULL);
}

VOID PhDestroyServiceNotifyContext(
    _In_ PPHP_SERVICE_NOTIFY_CONTEXT NotifyContext
    )
{
    if (NotifyContext->Buffer.pszServiceNames)
        LocalFree(NotifyContext->Buffer.pszServiceNames);

    PhCloseServiceHandle(NotifyContext->ServiceHandle);
    PhClearReference(&NotifyContext->ServiceName);
    PhFree(NotifyContext);
}

NTSTATUS PhServiceNonPollThreadStart(
    _In_ PVOID Parameter
    )
{
    ULONG result;
    LPENUM_SERVICE_STATUS_PROCESS services;
    ULONG numberOfServices;
    ULONG i;
    PLIST_ENTRY listEntry;
    PPHP_SERVICE_NOTIFY_CONTEXT notifyContext;

    if (!NT_SUCCESS(NtCreateEvent(&PhNonPollEventHandle, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE)))
    {
        PhNonPollActive = FALSE;
        PhpResetServiceNonPollGate();
        return STATUS_UNSUCCESSFUL;
    }

    while (TRUE)
    {
        InitializeListHead(&PhpNonPollServiceListHead);
        InitializeListHead(&PhpNonPollServicePendingListHead);

        if (!NT_SUCCESS(PhEnumServices(&services, &numberOfServices)))
            goto ErrorExit;

        for (i = 0; i < numberOfServices; i++)
        {
            SC_HANDLE serviceHandle;

            if (NT_SUCCESS(PhOpenService(&serviceHandle, SERVICE_QUERY_STATUS, services[i].lpServiceName)))
            {
                notifyContext = PhAllocateZero(sizeof(PHP_SERVICE_NOTIFY_CONTEXT));
                notifyContext->ServiceHandle = serviceHandle;
                notifyContext->State = SnNotify;
                InsertTailList(&PhpNonPollServicePendingListHead, &notifyContext->ListEntry);
            }
        }

        PhFree(services);

        notifyContext = PhAllocateZero(sizeof(PHP_SERVICE_NOTIFY_CONTEXT));
        notifyContext->ServiceHandle = PhGetServiceManagerHandle();
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
                case SnAdding:
                    if (!NT_SUCCESS(PhOpenService(&notifyContext->ServiceHandle, SERVICE_QUERY_STATUS, PhGetString(notifyContext->ServiceName))))
                    {
                        RemoveEntryList(&notifyContext->ListEntry);
                        PhDestroyServiceNotifyContext(notifyContext);
                        continue;
                    }

                    PhClearReference(&notifyContext->ServiceName);
                    notifyContext->State = SnNotify;
                    goto NotifyCase;
                case SnRemoving:
                    RemoveEntryList(&notifyContext->ListEntry);
                    PhDestroyServiceNotifyContext(notifyContext);
                    break;
                case SnNotify:
NotifyCase:
                    memset(&notifyContext->Buffer, 0, sizeof(SERVICE_NOTIFY));
                    notifyContext->Buffer.dwVersion = SERVICE_NOTIFY_STATUS_CHANGE;
                    notifyContext->Buffer.pfnNotifyCallback = PhServiceNotifyNonPollCallback;
                    notifyContext->Buffer.pContext = notifyContext;
                    result = NotifyServiceStatusChange_I(
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
                        PhDestroyServiceNotifyContext(notifyContext);
                        break;
                    }

                    break;
                }
            }

            while (NtWaitForSingleObject(PhNonPollEventHandle, TRUE, NULL) != STATUS_WAIT_0)
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
            PhDestroyServiceNotifyContext(notifyContext);
        }

        listEntry = PhpNonPollServicePendingListHead.Flink;

        while (listEntry != &PhpNonPollServicePendingListHead)
        {
            notifyContext = CONTAINING_RECORD(listEntry, PHP_SERVICE_NOTIFY_CONTEXT, ListEntry);
            listEntry = listEntry->Flink;
            PhDestroyServiceNotifyContext(notifyContext);
        }
    }

    PhNonPollActive = FALSE;
    PhpResetServiceNonPollGate();
    NtClose(PhNonPollEventHandle);
    return STATUS_SUCCESS;

ErrorExit:
    PhNonPollActive = FALSE;
    PhpResetServiceNonPollGate();
    NtClose(PhNonPollEventHandle);
    return STATUS_UNSUCCESSFUL;
}

VOID CALLBACK PhServiceNotifyPropertyChangeCallback(
    _In_ ULONG ServiceNotifyFlags,
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    if (ServiceItem->NotifyCreatedPropertyRegistration)
    {
        ServiceItem->NotifyCreatedPropertyRegistration = FALSE;
        return;
    }

    PhMarkNeedsConfigUpdateServiceItem(ServiceItem);
}

VOID CALLBACK PhServiceNotifyStatusChangeCallback(
    _In_ ULONG ServiceNotifyFlags,
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    if (ServiceItem->NotifyCreatedStatusRegistration)
    {
        ServiceItem->NotifyCreatedStatusRegistration = FALSE;
        return;
    }

    PhMarkNeedsConfigUpdateServiceItem(ServiceItem);
}

VOID CALLBACK PhServiceNotifyDatabaseChangeCallback(
    _In_ ULONG ServiceNotifyFlags,
    _In_ PVOID Context
    )
{
    static BOOLEAN NotifyCreatedDatabaseRegistration = TRUE;

    if (NotifyCreatedDatabaseRegistration)
    {
        NotifyCreatedDatabaseRegistration = FALSE;
        return;
    }

    PhpResetServiceNonPollGate();
}

VOID PhCreateServiceNotifyCallback(
    _In_ PPH_SERVICE_ITEM ServiceItem,
    _In_ SC_HANDLE ServiceHandle
    )
{
    PSC_NOTIFICATION_REGISTRATION servicePropertyRegistration;
    PSC_NOTIFICATION_REGISTRATION serviceStatusRegistration;

    if (!SubscribeServiceChangeNotifications_I)
        return;

    //if (FlagOn(ServiceItem->Type, SERVICE_DRIVER))
    //    return;

    if (!ServiceItem->NotifyCreatedPropertyRegistration)
    {
        ServiceItem->NotifyCreatedPropertyRegistration = TRUE;

        PhReferenceObject(ServiceItem);

        if (SubscribeServiceChangeNotifications_I(
            ServiceHandle,
            SC_EVENT_PROPERTY_CHANGE,
            PhServiceNotifyPropertyChangeCallback,
            ServiceItem,
            &servicePropertyRegistration
            ) == ERROR_SUCCESS)
        {
            ServiceItem->NotifyPropertyRegistration = servicePropertyRegistration;
        }
        else
        {
            PhDereferenceObject(ServiceItem);
        }
    }

    if (!ServiceItem->NotifyCreatedStatusRegistration)
    {
        ServiceItem->NotifyCreatedStatusRegistration = TRUE;

        PhReferenceObject(ServiceItem);

        if (SubscribeServiceChangeNotifications_I(
            ServiceHandle,
            SC_EVENT_STATUS_CHANGE,
            PhServiceNotifyStatusChangeCallback,
            ServiceItem,
            &serviceStatusRegistration
            ) == ERROR_SUCCESS)
        {
            ServiceItem->NotifyStatusRegistration = serviceStatusRegistration;
        }
        else
        {
            PhDereferenceObject(ServiceItem);
        }
    }
}

VOID PhDestroyServiceNotifyCallback(
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    if (!UnsubscribeServiceChangeNotifications_I)
        return;

    if (ServiceItem->NotifyStatusRegistration)
    {
        UnsubscribeServiceChangeNotifications_I(ServiceItem->NotifyStatusRegistration);
        ServiceItem->NotifyStatusRegistration = NULL;
        PhDereferenceObject(ServiceItem);
    }

    if (ServiceItem->NotifyPropertyRegistration)
    {
        UnsubscribeServiceChangeNotifications_I(ServiceItem->NotifyPropertyRegistration);
        ServiceItem->NotifyPropertyRegistration = NULL;
        PhDereferenceObject(ServiceItem);
    }
}

VOID PhCreateServiceDatabaseNotifyCallback(
    VOID
    )
{
    PSC_NOTIFICATION_REGISTRATION serviceNotifyRegistration;

    if (!SubscribeServiceChangeNotifications_I)
        return;

    SubscribeServiceChangeNotifications_I(
        PhGetServiceManagerHandle(),
        SC_EVENT_DATABASE_CHANGE,
        PhServiceNotifyDatabaseChangeCallback,
        NULL,
        &serviceNotifyRegistration
        );
}

VOID PhInitializeServiceNonPoll(
    VOID
    )
{
    PVOID baseAddress;

    if (baseAddress = PhLoadLibrary(L"sechost.dll"))
    {
        if (PhEnableServiceNonPollNotify && WindowsVersion >= WINDOWS_8)
        {
            SubscribeServiceChangeNotifications_I = PhGetDllBaseProcedureAddress(baseAddress, "SubscribeServiceChangeNotifications", 0);
            UnsubscribeServiceChangeNotifications_I = PhGetDllBaseProcedureAddress(baseAddress, "UnsubscribeServiceChangeNotifications", 0);
        }
        else
        {
            NotifyServiceStatusChange_I = PhGetDllBaseProcedureAddress(baseAddress, "NotifyServiceStatusChangeW", 0);
        }
    }

    PhNonPollActive = TRUE;
    InterlockedExchange(&PhNonPollGate, 1); // initially the gate should be open since we only just initialized everything (wj32)

    if (NotifyServiceStatusChange_I)
    {
        PhCreateThread2(PhServiceNonPollThreadStart, NULL);
    }

    PhCreateServiceDatabaseNotifyCallback();
}
