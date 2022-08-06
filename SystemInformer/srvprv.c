/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2015
 *     dmex    2017-2021
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

typedef ULONG (WINAPI *_SubscribeServiceChangeNotifications)(
    _In_ SC_HANDLE hService,
    _In_ SC_EVENT_TYPE eEventType,
    _In_ PSC_NOTIFICATION_CALLBACK pCallback,
    _In_opt_ PVOID pCallbackContext,
    _Out_ PSC_NOTIFICATION_REGISTRATION* pSubscription
    );

typedef VOID (WINAPI *_UnsubscribeServiceChangeNotifications)(
    _In_ PSC_NOTIFICATION_REGISTRATION pSubscription
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
    PPH_STRING ServiceName;
    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN IsServiceManager : 1;
            BOOLEAN JustAddedNotifyRegistration : 1;
            BOOLEAN NonPollDisabled : 1;
            BOOLEAN Spare : 5;
        };
    };
    PHP_SERVICE_NOTIFY_STATE State;
    SERVICE_NOTIFY Buffer;
    PSC_NOTIFICATION_REGISTRATION NotifyRegistration;
} PHP_SERVICE_NOTIFY_CONTEXT, *PPHP_SERVICE_NOTIFY_CONTEXT;

typedef struct _PH_SERVICE_QUERY_DATA
{
    SLIST_ENTRY ListEntry;
    ULONG Stage;
    SC_HANDLE ServiceManagerHandle;
    PPH_SERVICE_ITEM ServiceItem;
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

VOID PhpAddProcessItemService(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_SERVICE_ITEM ServiceItem
    );

VOID PhpRemoveProcessItemService(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_SERVICE_ITEM ServiceItem
    );

VOID PhpSubscribeServiceChangeNotifications(
    _In_ SC_HANDLE ScManagerHandle,
    _In_ PPH_SERVICE_ITEM ServiceItem
    );

VOID PhpInitializeServiceNonPoll(
    VOID
    );

VOID PhpWorkaroundWindows10ServiceTypeBug(
    _Inout_ LPENUM_SERVICE_STATUS_PROCESS ServiceEntry
    );

PPH_OBJECT_TYPE PhServiceItemType = NULL;
PPH_HASHTABLE PhServiceHashtable = NULL;
PH_QUEUED_LOCK PhServiceHashtableLock = PH_QUEUED_LOCK_INIT;

BOOLEAN PhEnableServiceNonPoll = FALSE;
static BOOLEAN PhpNonPollInitialized = FALSE;
static BOOLEAN PhpNonPollActive = FALSE;
static ULONG PhpNonPollGate = 0;
static HANDLE PhpNonPollEventHandle = NULL;
static LIST_ENTRY PhpNonPollServiceListHead = { &PhpNonPollServiceListHead, &PhpNonPollServiceListHead };
static LIST_ENTRY PhpNonPollServicePendingListHead = { &PhpNonPollServicePendingListHead, &PhpNonPollServicePendingListHead };
static SLIST_HEADER PhpServiceQueryDataListHead;
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

    if (WindowsVersion >= WINDOWS_8)
    {
        PVOID sechostHandle;

        if (sechostHandle = PhLoadLibrary(L"sechost.dll"))
        {
            SubscribeServiceChangeNotifications_I = PhGetDllBaseProcedureAddress(sechostHandle, "SubscribeServiceChangeNotifications", 0);
            UnsubscribeServiceChangeNotifications_I = PhGetDllBaseProcedureAddress(sechostHandle, "UnsubscribeServiceChangeNotifications", 0);
        }
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

    PhInitializeStringRef(&key, Name);

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
        InterlockedExchange(&PhpNonPollGate, 1);
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
            PhpAddProcessItemService(ProcessItem, *serviceItem);
        }
    }

    PhReleaseQueuedLockShared(&PhServiceHashtableLock);
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

        if (config = PhGetServiceConfig(serviceHandle))
        {
            PPH_STRING fileName = NULL;

            ServiceItem->StartType = config->dwStartType;
            ServiceItem->ErrorControl = config->dwErrorControl;

            PhGetServiceDllParameter(config->dwServiceType, &ServiceItem->Name->sr, &fileName);

            if (!fileName)
            {
                PPH_STRING commandLine;

                if (config->lpBinaryPathName[0])
                {
                    commandLine = PhCreateString(config->lpBinaryPathName);

                    if (config->dwServiceType & SERVICE_WIN32)
                    {
                        PH_STRINGREF dummyFileName;
                        PH_STRINGREF dummyArguments;

                        PhParseCommandLineFuzzy(&commandLine->sr, &dummyFileName, &dummyArguments, &fileName);

                        if (!fileName)
                            PhSwapReference(&fileName, commandLine);
                    }
                    else
                    {
                        fileName = PhGetFileName(commandLine);
                    }

                    PhDereferenceObject(commandLine);
                }
            }

            ServiceItem->FileName = fileName;

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
    return PhHashStringRefEx(&Value->Name, TRUE, PH_STRING_HASH_X65599);
}

VOID PhpServiceQueryStage1(
    _Inout_ PPH_SERVICE_QUERY_S1_DATA Data
    )
{
    PPH_SERVICE_ITEM serviceItem = Data->Header.ServiceItem;
    //SC_HANDLE serviceManagerHandle = Data->Header.ServiceManagerHandle;

    if (serviceItem->FileName)
    {
        if (!(serviceItem->Type & SERVICE_DRIVER)) // Skip icons for driver services (dmex)
        {
            Data->IconEntry = PhImageListExtractIcon(serviceItem->FileName, FALSE);
        }

        // Version info.
        //PhInitializeImageVersionInfo(&Data->VersionInfo, Data->FileName->Buffer);
    }

    PhpResetServiceNonPollGate(); // HACK (dmex)
}

VOID PhpServiceQueryStage2(
    _Inout_ PPH_SERVICE_QUERY_S2_DATA Data
    )
{
    PPH_SERVICE_ITEM serviceItem = Data->Header.ServiceItem;

    if (!PhIsNullOrEmptyString(serviceItem->FileName))
    {
        Data->VerifyResult = PhVerifyFileCached(
            serviceItem->FileName,
            NULL,
            &Data->VerifySignerName,
            FALSE,
            FALSE
            );
    }

    PhpResetServiceNonPollGate(); // HACK (dmex)
}

NTSTATUS PhpServiceQueryStage1Worker(
    _In_ PVOID Parameter
    )
{
    PPH_SERVICE_QUERY_S1_DATA data = Parameter;

    PhpServiceQueryStage1(data);

    RtlInterlockedPushEntrySList(&PhpServiceQueryDataListHead, &data->Header.ListEntry);

    return STATUS_SUCCESS;
}

NTSTATUS PhpServiceQueryStage2Worker(
    _In_ PVOID Parameter
    )
{
    PPH_SERVICE_QUERY_S2_DATA data;
    PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)Parameter;

    data = PhAllocateZero(sizeof(PH_SERVICE_QUERY_S2_DATA));
    data->Header.Stage = 2;
    data->Header.ServiceItem = serviceItem;

    PhpServiceQueryStage2(data);

    RtlInterlockedPushEntrySList(&PhpServiceQueryDataListHead, &data->Header.ListEntry);

    return STATUS_SUCCESS;
}

VOID PhpQueueServiceQueryStage1(
    _Inout_ PPH_SERVICE_QUERY_S1_DATA Data
    )
{
    PPH_SERVICE_ITEM serviceItem = Data->Header.ServiceItem;
    PH_WORK_QUEUE_ENVIRONMENT environment;

    PhInitializeWorkQueueEnvironment(&environment);
    environment.BasePriority = THREAD_PRIORITY_BELOW_NORMAL;
    environment.IoPriority = IoPriorityLow;
    environment.PagePriority = MEMORY_PRIORITY_LOW;

    PhReferenceObject(serviceItem);

    PhQueueItemWorkQueueEx(PhGetGlobalWorkQueue(), PhpServiceQueryStage1Worker, Data, NULL, &environment);
}

VOID PhQueueServiceQueryStage2(
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    PH_WORK_QUEUE_ENVIRONMENT environment;

    if (!PhEnableServiceQueryStage2)
        return;

    PhReferenceObject(ServiceItem);

    PhInitializeWorkQueueEnvironment(&environment);
    environment.BasePriority = THREAD_PRIORITY_BELOW_NORMAL;
    environment.IoPriority = IoPriorityVeryLow;
    environment.PagePriority = MEMORY_PRIORITY_VERY_LOW;

    PhQueueItemWorkQueueEx(PhGetGlobalWorkQueue(), PhpServiceQueryStage2Worker, ServiceItem, NULL, &environment);
}

VOID PhpFillServiceItemStage1(
    _In_ PPH_SERVICE_QUERY_S1_DATA Data
    )
{
    PPH_SERVICE_ITEM serviceItem = Data->Header.ServiceItem;

    serviceItem->IconEntry = Data->IconEntry;
    //memcpy(&processItem->VersionInfo, &Data->VersionInfo, sizeof(PH_IMAGE_VERSION_INFO));

    // Note: Queue stage 2 processing after filling stage1 process data.
    PhQueueServiceQueryStage2(serviceItem);
}

VOID PhpFillServiceItemStage2(
    _In_ PPH_SERVICE_QUERY_S2_DATA Data
    )
{
    PPH_SERVICE_ITEM serviceItem = Data->Header.ServiceItem;

    serviceItem->VerifyResult = Data->VerifyResult;
    serviceItem->VerifySignerName = Data->VerifySignerName;
}

VOID PhFlushServiceQueryData(
    VOID
    )
{
    PSLIST_ENTRY entry;
    PPH_SERVICE_QUERY_DATA data;

    if (!RtlFirstEntrySList(&PhpServiceQueryDataListHead))
        return;

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

        data->ServiceItem->JustProcessed = TRUE;

        PhDereferenceObject(data->ServiceItem);
        PhFree(data);
    }
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
            PhpInitializeServiceNonPoll();
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
            PhpWorkaroundWindows10ServiceTypeBug(entry->ServiceEntry);

        PhAddEntryHashSet(
            nameHashSet,
            PH_HASH_SET_SIZE(nameHashSet),
            &entry->HashEntry,
            PhpHashServiceNameEntry(entry)
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

                    processItem = PhReferenceProcessItem((*serviceItem)->ProcessId);

                    if (processItem)
                    {
                        PhpRemoveProcessItemService(processItem, *serviceItem);
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
                PhpUpdateServiceItemConfig(scManagerHandle, serviceItem);

                if (!PhEnableServiceNonPoll)
                {
                    PhpSubscribeServiceChangeNotifications(scManagerHandle, serviceItem);
                }

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
                    data.Header.ServiceManagerHandle = scManagerHandle;
                    data.Header.ServiceItem = serviceItem;
                    PhpServiceQueryStage1(&data);
                    PhpFillServiceItemStage1(&data);
                }
                else
                {
                    PPH_SERVICE_QUERY_S1_DATA data;

                    data = PhAllocateZero(sizeof(PH_SERVICE_QUERY_S1_DATA));
                    data->Header.Stage = 1;
                    data->Header.ServiceManagerHandle = scManagerHandle;
                    data->Header.ServiceItem = serviceItem;

                    PhpQueueServiceQueryStage1(data);
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
                        // is in the process ID. (wj32)

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

                    if (serviceItem->JustProcessed) // HACK (dmex)
                        serviceItem->JustProcessed = FALSE;

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

VOID CALLBACK PhpServiceNonPollScNotifyCallback(
    _In_ PVOID pParameter
    )
{
    PSERVICE_NOTIFY notifyBuffer = pParameter;
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

                if (name[0] == OBJ_NAME_ALTPATH_SEPARATOR)
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

    PhpNonPollGate = 1;
    NtSetEvent(PhpNonPollEventHandle, NULL);
}

VOID PhpDestroyServiceNotifyContext(
    _In_ PPHP_SERVICE_NOTIFY_CONTEXT NotifyContext
    )
{
    if (UnsubscribeServiceChangeNotifications_I && NotifyContext->NotifyRegistration)
        UnsubscribeServiceChangeNotifications_I(NotifyContext->NotifyRegistration);

    if (NotifyContext->Buffer.pszServiceNames)
        LocalFree(NotifyContext->Buffer.pszServiceNames);

    if (NotifyContext->ServiceHandle)
        CloseServiceHandle(NotifyContext->ServiceHandle);

    PhClearReference(&NotifyContext->ServiceName);
    PhFree(NotifyContext);
}

VOID CALLBACK PhpServicePropertyChangeNotifyCallback(
    _In_ ULONG ServiceNotifyFlags,
    _In_opt_ PVOID Context
    )
{
    PPHP_SERVICE_NOTIFY_CONTEXT notifyContext = Context;
    PPH_SERVICE_ITEM serviceItem;

    // Note: Ignore deleted nofications since NonPoll handles these elsewhere and our
    // notify context gets destroyed before services.exe invokes this callback. (dmex)
    if (ServiceNotifyFlags == SERVICE_NOTIFY_DELETED)
    {
        // Note: We can handle delete notifications only when NonPoll is disabled. (dmex)
        if (notifyContext && notifyContext->NonPollDisabled)
        {
            PhpDestroyServiceNotifyContext(notifyContext);
        }
        return;
    }

    if (!notifyContext)
        return;

    if (notifyContext->JustAddedNotifyRegistration)
    {
        notifyContext->JustAddedNotifyRegistration = FALSE;
        return;
    }

    if (!PhIsNullOrEmptyString(notifyContext->ServiceName))
    {
        if (serviceItem = PhReferenceServiceItem(notifyContext->ServiceName->Buffer))
        {
            PhMarkNeedsConfigUpdateServiceItem(serviceItem);

            PhDereferenceObject(serviceItem);
        }
    }
}

VOID PhpSubscribeServiceChangeNotifications(
    _In_ SC_HANDLE ScManagerHandle,
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    PSC_NOTIFICATION_REGISTRATION serviceNotifyRegistration;
    PPHP_SERVICE_NOTIFY_CONTEXT notifyContext;
    SC_HANDLE serviceHandle;

    if (!SubscribeServiceChangeNotifications_I)
        return;

    serviceHandle = OpenService(
        ScManagerHandle,
        PhGetStringOrEmpty(ServiceItem->Name),
        SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG
        );

    if (!serviceHandle)
        return;

    notifyContext = PhAllocateZero(sizeof(PHP_SERVICE_NOTIFY_CONTEXT));
    notifyContext->ServiceName = PhReferenceObject(ServiceItem->Name);
    notifyContext->JustAddedNotifyRegistration = TRUE;
    notifyContext->NonPollDisabled = TRUE;

    if (SubscribeServiceChangeNotifications_I(
        serviceHandle,
        SC_EVENT_PROPERTY_CHANGE,
        PhpServicePropertyChangeNotifyCallback,
        notifyContext,
        &serviceNotifyRegistration
        ) == ERROR_SUCCESS)
    {
        notifyContext->NotifyRegistration = serviceNotifyRegistration;
    }
    else
    {
        PhDereferenceObject(notifyContext->ServiceName);
        PhFree(notifyContext);
    }

    CloseServiceHandle(serviceHandle);
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

    if (!(scManagerHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE)))
    {
        PhpNonPollActive = FALSE;
        PhpNonPollGate = 1;
        return STATUS_UNSUCCESSFUL;
    }

    while (TRUE)
    {
        if (!(services = PhEnumServices(scManagerHandle, 0, 0, &numberOfServices)))
            goto ErrorExit;

        for (i = 0; i < numberOfServices; i++)
        {
            SC_HANDLE serviceHandle;

            if (!(serviceHandle = OpenService(scManagerHandle, services[i].lpServiceName, SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG)))
                serviceHandle = OpenService(scManagerHandle, services[i].lpServiceName, SERVICE_QUERY_STATUS);

            if (serviceHandle)
            {
                notifyContext = PhAllocateZero(sizeof(PHP_SERVICE_NOTIFY_CONTEXT));
                notifyContext->ServiceHandle = serviceHandle;
                notifyContext->State = SnNotify;
                notifyContext->ServiceName = PhCreateString(services[i].lpServiceName);
                InsertTailList(&PhpNonPollServicePendingListHead, &notifyContext->ListEntry);
            }
        }

        PhFree(services);

        notifyContext = PhAllocateZero(sizeof(PHP_SERVICE_NOTIFY_CONTEXT));
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
                case SnRemoving:
                    RemoveEntryList(&notifyContext->ListEntry);
                    PhpDestroyServiceNotifyContext(notifyContext);
                    break;
                case SnAdding:
                    {
                        notifyContext->ServiceHandle = OpenService(
                            scManagerHandle,
                            notifyContext->ServiceName->Buffer,
                            SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG
                            );

                        if (!notifyContext->ServiceHandle)
                        {
                            notifyContext->ServiceHandle = OpenService(
                                scManagerHandle,
                                notifyContext->ServiceName->Buffer,
                                SERVICE_QUERY_STATUS
                                );
                        }

                        if (!notifyContext->ServiceHandle)
                        {
                            RemoveEntryList(&notifyContext->ListEntry);
                            PhpDestroyServiceNotifyContext(notifyContext);
                            continue;
                        }

                        notifyContext->State = SnNotify;
                    }
                    __fallthrough;
                case SnNotify:
                    {
                        if (notifyContext->NotifyRegistration)
                        {
                            if (UnsubscribeServiceChangeNotifications_I)
                                UnsubscribeServiceChangeNotifications_I(notifyContext->NotifyRegistration);

                            notifyContext->JustAddedNotifyRegistration = FALSE;
                            notifyContext->NotifyRegistration = NULL;
                        }

                        if (SubscribeServiceChangeNotifications_I && !notifyContext->IsServiceManager)
                        {
                            PSC_NOTIFICATION_REGISTRATION serviceNotifyRegistration;

                            if (SubscribeServiceChangeNotifications_I(
                                notifyContext->ServiceHandle,
                                SC_EVENT_PROPERTY_CHANGE, // TODO: SC_EVENT_STATUS_CHANGE (dmex)
                                PhpServicePropertyChangeNotifyCallback,
                                notifyContext,
                                &serviceNotifyRegistration
                                ) == ERROR_SUCCESS)
                            {
                                notifyContext->JustAddedNotifyRegistration = TRUE;
                                notifyContext->NotifyRegistration = serviceNotifyRegistration;
                            }
                        }

                        memset(&notifyContext->Buffer, 0, sizeof(SERVICE_NOTIFY));
                        notifyContext->Buffer.dwVersion = SERVICE_NOTIFY_STATUS_CHANGE;
                        notifyContext->Buffer.pfnNotifyCallback = PhpServiceNonPollScNotifyCallback;
                        notifyContext->Buffer.pContext = notifyContext;

                        result = NotifyServiceStatusChange(
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
                            // We are lagging behind. Re-open the handle to the SCM as soon as possible. (wj32)
                            lagging = TRUE;
                            break;
                        case ERROR_SERVICE_MARKED_FOR_DELETE:
                        default:
                            RemoveEntryList(&notifyContext->ListEntry);
                            PhpDestroyServiceNotifyContext(notifyContext);
                            break;
                        }
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
    }

    NtClose(PhpNonPollEventHandle);

    CloseServiceHandle(scManagerHandle);

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
    PhpNonPollActive = TRUE;
    PhpNonPollGate = 1; // initially the gate should be open since we only just initialized everything (wj32)

    if (WindowsVersion >= WINDOWS_8)
    {
        PVOID sechostHandle;

        if (sechostHandle = PhLoadLibrary(L"sechost.dll"))
        {
            SubscribeServiceChangeNotifications_I = PhGetDllBaseProcedureAddress(sechostHandle, "SubscribeServiceChangeNotifications", 0);
            UnsubscribeServiceChangeNotifications_I = PhGetDllBaseProcedureAddress(sechostHandle, "UnsubscribeServiceChangeNotifications", 0);
        }
    }

    PhCreateThread2(PhpServiceNonPollThreadStart, NULL);
}

VOID PhpWorkaroundWindows10ServiceTypeBug(
    _Inout_ LPENUM_SERVICE_STATUS_PROCESS ServiceEntry
    )
{
    // https://github.com/processhacker2/processhacker/issues/120 (dmex)
    if (ServiceEntry->ServiceStatusProcess.dwServiceType == SERVICE_WIN32)
        ServiceEntry->ServiceStatusProcess.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    if (ServiceEntry->ServiceStatusProcess.dwServiceType == (SERVICE_WIN32 | SERVICE_USER_SHARE_PROCESS | SERVICE_USERSERVICE_INSTANCE))
        ServiceEntry->ServiceStatusProcess.dwServiceType = SERVICE_USER_SHARE_PROCESS | SERVICE_USERSERVICE_INSTANCE;
}
