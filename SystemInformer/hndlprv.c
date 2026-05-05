/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2017-2023
 *
 */

#include <phapp.h>
#include <hndlprv.h>

#include <hndlinfo.h>
#include <kphuser.h>
#include <settings.h>
#include <phsettings.h>
#include <workqueue.h>

#include <extmgri.h>

typedef struct _PH_HANDLE_QUERY_DATA
{
    SLIST_ENTRY ListEntry;
    PPH_HANDLE_PROVIDER HandleProvider;
    PPH_HANDLE_ITEM HandleItem;
} PH_HANDLE_QUERY_DATA, *PPH_HANDLE_QUERY_DATA;

typedef struct _PH_HANDLE_QUERY_S1_DATA
{
    PH_HANDLE_QUERY_DATA Header;

    OBJECT_BASIC_INFORMATION HandleBasicInformation;
    NTSTATUS Status;
    PPH_STRING TypeName;
    PPH_STRING ObjectName;
    PPH_STRING BestObjectName;
    ULONG FileFlags;
} PH_HANDLE_QUERY_S1_DATA, *PPH_HANDLE_QUERY_S1_DATA;

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI PhpHandleProviderDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI PhpHandleItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

PPH_HANDLE_ITEM PhpLookupHandleItem(
    _In_ PPH_HANDLE_PROVIDER HandleProvider,
    _In_ HANDLE Handle
    );

VOID PhpAddHandleItem(
    _In_ PPH_HANDLE_PROVIDER HandleProvider,
    _In_ _Assume_refs_(1) PPH_HANDLE_ITEM HandleItem
    );

BOOLEAN PhpClearHandleItemName(
    _Inout_ PPH_HANDLE_ITEM HandleItem
    );

VOID PhpQueueHandleQueryStage1(
    _In_ PPH_HANDLE_PROVIDER HandleProvider,
    _In_ PPH_HANDLE_ITEM HandleItem
    );

VOID PhpFlushHandleQueryData(
    _Inout_ PPH_HANDLE_PROVIDER HandleProvider
    );

VOID PhpClearHandleQueryData(
    _Inout_ PPH_HANDLE_PROVIDER HandleProvider
    );

PPH_OBJECT_TYPE PhHandleProviderType = NULL;
PPH_OBJECT_TYPE PhHandleItemType = NULL;

PPH_HANDLE_PROVIDER PhCreateHandleProvider(
    _In_ HANDLE ProcessId
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    PPH_HANDLE_PROVIDER handleProvider;

    if (PhBeginInitOnce(&initOnce))
    {
        PhHandleProviderType = PhCreateObjectType(L"HandleProvider", 0, PhpHandleProviderDeleteProcedure);
        PhEndInitOnce(&initOnce);
    }

    handleProvider = PhCreateObject(
        PhEmGetObjectSize(EmHandleProviderType, sizeof(PH_HANDLE_PROVIDER)),
        PhHandleProviderType
        );
    memset(handleProvider, 0, sizeof(PH_HANDLE_PROVIDER));

    handleProvider->HandleHashSetSize = 128;
    handleProvider->HandleHashSet = PhCreateHashSet(handleProvider->HandleHashSetSize);
    handleProvider->HandleHashSetCount = 0;
    PhInitializeQueuedLock(&handleProvider->HandleHashSetLock);

    PhInitializeCallback(&handleProvider->HandleAddedEvent);
    PhInitializeCallback(&handleProvider->HandleModifiedEvent);
    PhInitializeCallback(&handleProvider->HandleRemovedEvent);
    PhInitializeCallback(&handleProvider->HandleUpdatedEvent);

    handleProvider->ProcessId = ProcessId;
    handleProvider->ProcessHandle = NULL;
    PhInitializeSListHead(&handleProvider->QueryListHead);

    handleProvider->RunStatus = PhOpenProcess(
        &handleProvider->ProcessHandle,
        PROCESS_QUERY_INFORMATION | PROCESS_DUP_HANDLE,
        ProcessId
        );
    if (!NT_SUCCESS(handleProvider->RunStatus))
    {
        handleProvider->RunStatus = PhOpenProcess(
            &handleProvider->ProcessHandle,
            PROCESS_QUERY_INFORMATION,
            ProcessId
            );
    }

    handleProvider->TempListHashtable = PhCreateSimpleHashtable(512);

    PhEmCallObjectOperation(EmHandleProviderType, handleProvider, EmObjectCreate);

    return handleProvider;
}

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID PhpHandleProviderDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_HANDLE_PROVIDER handleProvider = (PPH_HANDLE_PROVIDER)Object;

    PhEmCallObjectOperation(EmHandleProviderType, handleProvider, EmObjectDelete);

    // Dereference all handle items (we referenced them
    // when we added them to the hashtable).
    PhDereferenceAllHandleItems(handleProvider);
    PhpClearHandleQueryData(handleProvider);

    PhFree(handleProvider->HandleHashSet);
    PhDeleteCallback(&handleProvider->HandleAddedEvent);
    PhDeleteCallback(&handleProvider->HandleModifiedEvent);
    PhDeleteCallback(&handleProvider->HandleRemovedEvent);

    if (handleProvider->ProcessHandle) NtClose(handleProvider->ProcessHandle);

    PhDereferenceObject(handleProvider->TempListHashtable);
}

PPH_HANDLE_ITEM PhCreateHandleItem(
    _In_opt_ PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Handle
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    PPH_HANDLE_ITEM handleItem;

    if (PhBeginInitOnce(&initOnce))
    {
        PhHandleItemType = PhCreateObjectType(L"HandleItem", 0, PhpHandleItemDeleteProcedure);
        PhEndInitOnce(&initOnce);
    }

    handleItem = PhCreateObject(
        PhEmGetObjectSize(EmHandleItemType, sizeof(PH_HANDLE_ITEM)),
        PhHandleItemType
        );
    memset(handleItem, 0, sizeof(PH_HANDLE_ITEM));

    if (Handle)
    {
        handleItem->Object = Handle->Object;
        handleItem->ProcessId = Handle->UniqueProcessId;
        handleItem->Handle = Handle->HandleValue;
        handleItem->Attributes = Handle->HandleAttributes;
        handleItem->GrantedAccess = Handle->GrantedAccess;
        handleItem->TypeIndex = Handle->ObjectTypeIndex;
        handleItem->TypeName = PhGetObjectTypeNameEx(Handle->ObjectTypeIndex);

        PhPrintPointer(handleItem->HandleString, (PVOID)handleItem->Handle);
        PhPrintPointer(handleItem->GrantedAccessString, UlongToPtr(handleItem->GrantedAccess));

        if (handleItem->Object)
            PhPrintPointer(handleItem->ObjectString, handleItem->Object);
    }

    PhEmCallObjectOperation(EmHandleItemType, handleItem, EmObjectCreate);

    return handleItem;
}

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID PhpHandleItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_HANDLE_ITEM handleItem = (PPH_HANDLE_ITEM)Object;

    PhEmCallObjectOperation(EmHandleItemType, handleItem, EmObjectDelete);

    if (handleItem->TypeName) PhDereferenceObject(handleItem->TypeName);
    if (handleItem->ObjectName) PhDereferenceObject(handleItem->ObjectName);
    if (handleItem->BestObjectName) PhDereferenceObject(handleItem->BestObjectName);
}

FORCEINLINE BOOLEAN PhCompareHandleItem(
    _In_ PPH_HANDLE_ITEM Value1,
    _In_ PPH_HANDLE_ITEM Value2
    )
{
    return Value1->Handle == Value2->Handle;
}

FORCEINLINE ULONG PhHashHandleItem(
    _In_ PPH_HANDLE_ITEM Value
    )
{
    return HandleToUlong(Value->Handle) / 4;
}

PPH_HANDLE_ITEM PhReferenceHandleItem(
    _In_ PPH_HANDLE_PROVIDER HandleProvider,
    _In_ HANDLE Handle
    )
{
    PPH_HANDLE_ITEM handleItem;

    PhAcquireQueuedLockShared(&HandleProvider->HandleHashSetLock);

    handleItem = PhpLookupHandleItem(HandleProvider, Handle);

    if (handleItem)
        PhReferenceObject(handleItem);

    PhReleaseQueuedLockShared(&HandleProvider->HandleHashSetLock);

    return handleItem;
}

PPH_HANDLE_ITEM PhpLookupHandleItem(
    _In_ PPH_HANDLE_PROVIDER HandleProvider,
    _In_ HANDLE Handle
    )
{
    PH_HANDLE_ITEM lookupHandleItem;
    PPH_HASH_ENTRY entry;
    PPH_HANDLE_ITEM handleItem;

    lookupHandleItem.Handle = Handle;
    entry = PhFindEntryHashSet(
        HandleProvider->HandleHashSet,
        HandleProvider->HandleHashSetSize,
        PhHashHandleItem(&lookupHandleItem)
        );

    for (; entry; entry = entry->Next)
    {
        handleItem = CONTAINING_RECORD(entry, PH_HANDLE_ITEM, HashEntry);

        if (PhCompareHandleItem(&lookupHandleItem, handleItem))
            return handleItem;
    }

    return NULL;
}

VOID PhpRemoveHandleItem(
    _In_ PPH_HANDLE_PROVIDER HandleProvider,
    _In_ PPH_HANDLE_ITEM HandleItem
    )
{
    PhRemoveEntryHashSet(HandleProvider->HandleHashSet, HandleProvider->HandleHashSetSize, &HandleItem->HashEntry);
    HandleProvider->HandleHashSetCount--;
    PhDereferenceObject(HandleItem);
}

VOID PhpAddHandleItem(
    _In_ PPH_HANDLE_PROVIDER HandleProvider,
    _In_ _Assume_refs_(1) PPH_HANDLE_ITEM HandleItem
    )
{
    if (HandleProvider->HandleHashSetSize < HandleProvider->HandleHashSetCount + 1)
    {
        PhResizeHashSet(
            &HandleProvider->HandleHashSet,
            &HandleProvider->HandleHashSetSize,
            HandleProvider->HandleHashSetSize * 2
            );
    }

    PhAddEntryHashSet(
        HandleProvider->HandleHashSet,
        HandleProvider->HandleHashSetSize,
        &HandleItem->HashEntry,
        PhHashHandleItem(HandleItem)
        );
    HandleProvider->HandleHashSetCount++;
}

VOID PhDereferenceAllHandleItems(
    _In_ PPH_HANDLE_PROVIDER HandleProvider
    )
{
    ULONG i;
    PPH_HASH_ENTRY entry;
    PPH_HANDLE_ITEM handleItem;

    PhAcquireQueuedLockExclusive(&HandleProvider->HandleHashSetLock);

    for (i = 0; i < HandleProvider->HandleHashSetSize; i++)
    {
        entry = HandleProvider->HandleHashSet[i];

        while (entry)
        {
            handleItem = CONTAINING_RECORD(entry, PH_HANDLE_ITEM, HashEntry);
            entry = entry->Next;
            PhDereferenceObject(handleItem);
        }
    }

    PhReleaseQueuedLockExclusive(&HandleProvider->HandleHashSetLock);
}

BOOLEAN PhpClearHandleItemName(
    _Inout_ PPH_HANDLE_ITEM HandleItem
    )
{
    BOOLEAN modified = FALSE;

    if (!HandleItem->NameResolved)
    {
        HandleItem->NameResolved = TRUE;
        modified = TRUE;
    }

    if (HandleItem->ObjectName)
    {
        PhClearReference(&HandleItem->ObjectName);
        modified = TRUE;
    }

    if (HandleItem->BestObjectName)
    {
        PhClearReference(&HandleItem->BestObjectName);
        modified = TRUE;
    }

    return modified;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS PhpHandleQueryStage1Worker(
    _In_ PVOID Parameter
    )
{
    PPH_HANDLE_QUERY_S1_DATA data = Parameter;
    PPH_HANDLE_PROVIDER handleProvider = data->Header.HandleProvider;
    PPH_HANDLE_ITEM handleItem = data->Header.HandleItem;

    data->Status = PhGetHandleInformationEx(
        handleProvider->ProcessHandle,
        handleItem->Handle,
        handleItem->TypeIndex,
        0,
        NULL,
        &data->HandleBasicInformation,
        &data->TypeName,
        &data->ObjectName,
        &data->BestObjectName,
        NULL
        );

    if (PhIsNullOrEmptyString(data->TypeName))
    {
        PhMoveReference(&data->TypeName, PhGetObjectTypeIndexName(handleItem->TypeIndex));
    }

    if (
        PhIsObjectTypeIndex(handleItem->TypeIndex, PhHandleObjectTypeFile) &&
        KsiLevel() >= KphLevelMed
        )
    {
        KPH_FILE_OBJECT_INFORMATION objectInfo;

        if (NT_SUCCESS(KphQueryInformationObject(
            handleProvider->ProcessHandle,
            handleItem->Handle,
            KphObjectFileObjectInformation,
            &objectInfo,
            sizeof(KPH_FILE_OBJECT_INFORMATION),
            NULL
            )))
        {
            if (objectInfo.SharedRead)
                data->FileFlags |= PH_HANDLE_FILE_SHARED_READ;
            if (objectInfo.SharedWrite)
                data->FileFlags |= PH_HANDLE_FILE_SHARED_WRITE;
            if (objectInfo.SharedDelete)
                data->FileFlags |= PH_HANDLE_FILE_SHARED_DELETE;
        }
    }

    RtlInterlockedPushEntrySList(&handleProvider->QueryListHead, &data->Header.ListEntry);
    PhDereferenceObject(handleProvider);

    return STATUS_SUCCESS;
}

VOID PhpQueueHandleQueryStage1(
    _In_ PPH_HANDLE_PROVIDER HandleProvider,
    _In_ PPH_HANDLE_ITEM HandleItem
    )
{
    PH_WORK_QUEUE_ENVIRONMENT environment;
    PPH_HANDLE_QUERY_S1_DATA data;

    data = PhAllocateZero(sizeof(PH_HANDLE_QUERY_S1_DATA));
    data->Header.HandleProvider = HandleProvider;
    data->Header.HandleItem = HandleItem;

    // Provider ref: dereferenced by the worker after it queues the result.
    // Handle item ref: dereferenced when the provider update function removes the item from the queue.
    PhReferenceObject(HandleProvider);
    PhReferenceObject(HandleItem);

    PhInitializeWorkQueueEnvironment(&environment);
    environment.BasePriority = THREAD_PRIORITY_BELOW_NORMAL;
    environment.IoPriority = IoPriorityLow;
    environment.PagePriority = MEMORY_PRIORITY_LOW;

    PhQueueItemWorkQueueEx(PhGetGlobalWorkQueue(), PhpHandleQueryStage1Worker, data, NULL, &environment);
}

BOOLEAN PhpFillHandleItemStage1(
    _In_ PPH_HANDLE_QUERY_S1_DATA Data
    )
{
    PPH_HANDLE_ITEM handleItem = Data->Header.HandleItem;
    BOOLEAN modified = FALSE;

    if (!NT_SUCCESS(Data->Status))
    {
        modified = PhpClearHandleItemName(handleItem);
    }

    if (handleItem->HandleCount != Data->HandleBasicInformation.HandleCount)
    {
        handleItem->HandleCount = Data->HandleBasicInformation.HandleCount;
        modified = TRUE;
    }

    if (handleItem->PointerCount != Data->HandleBasicInformation.PointerCount)
    {
        handleItem->PointerCount = Data->HandleBasicInformation.PointerCount;
        modified = TRUE;
    }

    if (handleItem->PagedPoolCharge != Data->HandleBasicInformation.PagedPoolCharge)
    {
        handleItem->PagedPoolCharge = Data->HandleBasicInformation.PagedPoolCharge;
        modified = TRUE;
    }

    if (handleItem->NonPagedPoolCharge != Data->HandleBasicInformation.NonPagedPoolCharge)
    {
        handleItem->NonPagedPoolCharge = Data->HandleBasicInformation.NonPagedPoolCharge;
        modified = TRUE;
    }

    if (PhCompareStringWithNull(handleItem->TypeName, Data->TypeName, FALSE) != 0)
    {
        PhMoveReference(&handleItem->TypeName, Data->TypeName);
        Data->TypeName = NULL;
        modified = TRUE;
    }

    if (PhCompareStringWithNull(handleItem->ObjectName, Data->ObjectName, FALSE) != 0)
    {
        PhMoveReference(&handleItem->ObjectName, Data->ObjectName);
        Data->ObjectName = NULL;
        modified = TRUE;
    }

    if (PhCompareStringWithNull(handleItem->BestObjectName, Data->BestObjectName, FALSE) != 0)
    {
        PhMoveReference(&handleItem->BestObjectName, Data->BestObjectName);
        Data->BestObjectName = NULL;
        modified = TRUE;
    }

    if ((handleItem->FileFlags & PH_HANDLE_FILE_SHARED_MASK) != Data->FileFlags)
    {
        handleItem->FileFlags &= ~PH_HANDLE_FILE_SHARED_MASK;
        handleItem->FileFlags |= Data->FileFlags;
        modified = TRUE;
    }

    return modified;
}

VOID PhpFreeHandleQueryData(
    _In_ PPH_HANDLE_QUERY_DATA Data
    )
{
    PPH_HANDLE_QUERY_S1_DATA stage1Data = (PPH_HANDLE_QUERY_S1_DATA)Data;

    if (stage1Data->TypeName) PhDereferenceObject(stage1Data->TypeName);
    if (stage1Data->ObjectName) PhDereferenceObject(stage1Data->ObjectName);
    if (stage1Data->BestObjectName) PhDereferenceObject(stage1Data->BestObjectName);

    PhDereferenceObject(Data->HandleItem);
    PhFree(Data);
}

VOID PhpFlushHandleQueryData(
    _Inout_ PPH_HANDLE_PROVIDER HandleProvider
    )
{
    PSLIST_ENTRY entry;

    entry = RtlInterlockedFlushSList(&HandleProvider->QueryListHead);

    while (entry)
    {
        PPH_HANDLE_QUERY_DATA data;
        PPH_HANDLE_ITEM handleItem;

        data = CONTAINING_RECORD(entry, PH_HANDLE_QUERY_DATA, ListEntry);
        entry = entry->Next;
        handleItem = data->HandleItem;

        if (handleItem)
        {
            PhAcquireQueuedLockShared(&HandleProvider->HandleHashSetLock);

            if (PhpLookupHandleItem(HandleProvider, handleItem->Handle) == handleItem)
            {
                BOOLEAN modified = FALSE;
                BOOLEAN nameResolved;

                modified = PhpFillHandleItemStage1((PPH_HANDLE_QUERY_S1_DATA)data);
                nameResolved = TRUE;

                if (handleItem->NameResolved != nameResolved)
                {
                    handleItem->NameResolved = nameResolved;
                    modified = TRUE;
                }

                PhReleaseQueuedLockShared(&HandleProvider->HandleHashSetLock);

                if (modified)
                    InterlockedExchange(&handleItem->JustProcessed, TRUE);
            }
            else
            {
                PhReleaseQueuedLockShared(&HandleProvider->HandleHashSetLock);
            }
        }

        PhpFreeHandleQueryData(data);
    }
}

VOID PhpClearHandleQueryData(
    _Inout_ PPH_HANDLE_PROVIDER HandleProvider
    )
{
    PSLIST_ENTRY entry;

    entry = RtlInterlockedFlushSList(&HandleProvider->QueryListHead);

    while (entry)
    {
        PPH_HANDLE_QUERY_DATA data;

        data = CONTAINING_RECORD(entry, PH_HANDLE_QUERY_DATA, ListEntry);
        entry = entry->Next;

        PhpFreeHandleQueryData(data);
    }
}

_Function_class_(PH_PROVIDER_FUNCTION)
VOID PhHandleProviderUpdate(
    _In_ PVOID Object
    )
{
    PPH_HANDLE_PROVIDER handleProvider = (PPH_HANDLE_PROVIDER)Object;
    PSYSTEM_HANDLE_INFORMATION_EX handleInfo = NULL;
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handles = NULL;
    ULONG_PTR numberOfHandles;
    ULONG i;
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PPH_KEY_VALUE_PAIR handlePair;
    KPH_LEVEL level;

    // Note about locking:
    //
    // Since this is the only function that is allowed to modify the handle hashtable, locking is
    // not needed for shared accesses. However, exclusive accesses need locking.

    handleProvider->RunStatus = PhEnumHandlesGeneric(
        handleProvider->ProcessId,
        handleProvider->ProcessHandle,
        !!PhCsEnableHandleSnapshot,
        &handleInfo
        );

    level = KsiLevel();

    if (NT_SUCCESS(handleProvider->RunStatus))
    {
        handles = handleInfo->Handles;
        numberOfHandles = handleInfo->NumberOfHandles;

        for (i = 0; i < numberOfHandles; i++)
        {
            PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handle = &handles[i];

            PhAddItemSimpleHashtable(
                handleProvider->TempListHashtable,
                handle->HandleValue,
                handle
                );
        }
    }

    // Look for closed handles.
    {
        PPH_LIST handlesToRemove = NULL;
        PPH_HASH_ENTRY entry;
        PPH_HANDLE_ITEM handleItem;
        PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX *tempHashtableValue;

        for (i = 0; i < handleProvider->HandleHashSetSize; i++)
        {
            for (entry = handleProvider->HandleHashSet[i]; entry; entry = entry->Next)
            {
                BOOLEAN found = FALSE;

                handleItem = CONTAINING_RECORD(entry, PH_HANDLE_ITEM, HashEntry);

                // Check if the handle still exists.

                tempHashtableValue = (PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX *)PhFindItemSimpleHashtable(
                    handleProvider->TempListHashtable,
                    handleItem->Handle
                    );

                if (tempHashtableValue)
                {
                    // Also compare the object pointers to make sure a different object wasn't
                    // re-opened with the same handle value.
                    if (level >= KphLevelMed && handleProvider->ProcessHandle)
                    {
                        found = NT_SUCCESS(KphCompareObjects(
                            handleProvider->ProcessHandle,
                            handleItem->Handle,
                            (*tempHashtableValue)->HandleValue
                            ));
                    }
                    // This isn't 100% accurate as pool addresses may be re-used, but it works well.
                    else if (handleItem->Object && handleItem->Object == (*tempHashtableValue)->Object)
                    {
                        found = TRUE;
                    }
                    else

                    {
                        if (
                            handleItem->Handle == (*tempHashtableValue)->HandleValue &&
                            handleItem->GrantedAccess == (*tempHashtableValue)->GrantedAccess &&
                            handleItem->TypeIndex == (*tempHashtableValue)->ObjectTypeIndex &&
                            handleItem->ProcessId == (*tempHashtableValue)->UniqueProcessId
                            )
                        {
                            found = TRUE;
                        }
                    }
                }

                if (!found)
                {
                    // Raise the handle removed event.
                    PhInvokeCallback(&handleProvider->HandleRemovedEvent, handleItem);

                    if (!handlesToRemove)
                        handlesToRemove = PhCreateList(2);

                    PhAddItemList(handlesToRemove, handleItem);
                }
            }
        }

        if (handlesToRemove)
        {
            PhAcquireQueuedLockExclusive(&handleProvider->HandleHashSetLock);

            for (i = 0; i < handlesToRemove->Count; i++)
            {
                PhpRemoveHandleItem(handleProvider, handlesToRemove->Items[i]);
            }

            PhReleaseQueuedLockExclusive(&handleProvider->HandleHashSetLock);
            PhDereferenceObject(handlesToRemove);
        }
    }

    PhpFlushHandleQueryData(handleProvider);

    // Look for new handles and update existing ones.

    PhBeginEnumHashtable(handleProvider->TempListHashtable, &enumContext);

    while (handlePair = PhNextEnumHashtable(&enumContext))
    {
        PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handle = handlePair->Value;
        PPH_HANDLE_ITEM handleItem;

        handleItem = PhpLookupHandleItem(handleProvider, handle->HandleValue);

        if (!handleItem)
        {
            handleItem = PhCreateHandleItem(handle);

            // Add the handle item to the hashtable.
            PhAcquireQueuedLockExclusive(&handleProvider->HandleHashSetLock);
            PhpAddHandleItem(handleProvider, handleItem);
            PhReleaseQueuedLockExclusive(&handleProvider->HandleHashSetLock);

            // Raise the handle added event.
            PhInvokeCallback(&handleProvider->HandleAddedEvent, handleItem);

            PhpQueueHandleQueryStage1(handleProvider, handleItem);
        }
        else
        {
            BOOLEAN modified = FALSE;
            OBJECT_BASIC_INFORMATION handleBasicInformation = { 0 };

            if (NT_SUCCESS(PhGetHandleInformationEx(
                handleProvider->ProcessHandle,
                handleItem->Handle,
                handle->ObjectTypeIndex,
                0,
                NULL,
                &handleBasicInformation,
                NULL,
                NULL,
                NULL,
                NULL
                )))
            {
                if (handleItem->HandleCount != handleBasicInformation.HandleCount)
                {
                    handleItem->HandleCount = handleBasicInformation.HandleCount;
                    modified = TRUE;
                }

                if (handleItem->PointerCount != handleBasicInformation.PointerCount)
                {
                    handleItem->PointerCount = handleBasicInformation.PointerCount;
                    modified = TRUE;
                }

                if (handleItem->PagedPoolCharge != handleBasicInformation.PagedPoolCharge)
                {
                    handleItem->PagedPoolCharge = handleBasicInformation.PagedPoolCharge;
                    modified = TRUE;
                }

                if (handleItem->NonPagedPoolCharge != handleBasicInformation.NonPagedPoolCharge)
                {
                    handleItem->NonPagedPoolCharge = handleBasicInformation.NonPagedPoolCharge;
                    modified = TRUE;
                }
            }
            else
            {
                modified = PhpClearHandleItemName(handleItem);
            }

            if (handleItem->Attributes != handle->HandleAttributes)
            {
                handleItem->Attributes = handle->HandleAttributes;
                modified = TRUE;
            }

            if (InterlockedExchange(&handleItem->JustProcessed, 0) != 0)
                modified = TRUE;

            if (modified)
            {
                // Raise the handle modified event.
                PhInvokeCallback(&handleProvider->HandleModifiedEvent, handleItem);
            }
        }
    }

    if (NT_SUCCESS(handleProvider->RunStatus))
    {
        PhFree(handleInfo);
    }

    // Re-create the temporary hashtable if it got too big.
    if (handleProvider->TempListHashtable->AllocatedEntries > 8192)
    {
        PhDereferenceObject(handleProvider->TempListHashtable);
        handleProvider->TempListHashtable = PhCreateSimpleHashtable(512);
    }
    else
    {
        PhClearHashtable(handleProvider->TempListHashtable);
    }

    PhInvokeCallback(&handleProvider->HandleUpdatedEvent, NULL);
}
