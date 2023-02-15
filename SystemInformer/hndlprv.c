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
#include <workqueue.h>

#include <extmgri.h>

typedef struct _PHP_CREATE_HANDLE_ITEM_CONTEXT
{
    PPH_HANDLE_PROVIDER Provider;
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Handle;
} PHP_CREATE_HANDLE_ITEM_CONTEXT, *PPHP_CREATE_HANDLE_ITEM_CONTEXT;

VOID NTAPI PhpHandleProviderDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

VOID NTAPI PhpHandleItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
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
        handleItem->Handle = (HANDLE)Handle->HandleValue;
        handleItem->Object = Handle->Object;
        handleItem->Attributes = Handle->HandleAttributes;
        handleItem->GrantedAccess = (ACCESS_MASK)Handle->GrantedAccess;
        handleItem->TypeIndex = Handle->ObjectTypeIndex;

        PhPrintPointer(handleItem->HandleString, (PVOID)handleItem->Handle);
        PhPrintPointer(handleItem->GrantedAccessString, UlongToPtr(handleItem->GrantedAccess));

        if (handleItem->Object)
            PhPrintPointer(handleItem->ObjectString, handleItem->Object);
    }

    PhEmCallObjectOperation(EmHandleItemType, handleItem, EmObjectCreate);

    return handleItem;
}

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

VOID PhpRemoveHandleItem(
    _In_ PPH_HANDLE_PROVIDER HandleProvider,
    _In_ PPH_HANDLE_ITEM HandleItem
    )
{
    PhRemoveEntryHashSet(HandleProvider->HandleHashSet, HandleProvider->HandleHashSetSize, &HandleItem->HashEntry);
    HandleProvider->HandleHashSetCount--;
    PhDereferenceObject(HandleItem);
}

/**
 * Enumerates all handles in a process.
 *
 * \param ProcessId The ID of the process.
 * \param ProcessHandle A handle to the process.
 * \param Handles A variable which receives a pointer to a buffer containing
 * information about the handles.
 * \param FilterNeeded A variable which receives a boolean indicating
 * whether the handle information needs to be filtered by process ID.
 */
NTSTATUS PhEnumHandlesGeneric(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ProcessHandle,
    _Out_ PSYSTEM_HANDLE_INFORMATION_EX *Handles,
    _Out_ PBOOLEAN FilterNeeded
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // There are three ways of enumerating handles:
    // * On Windows 8 and later, NtQueryInformationProcess with ProcessHandleInformation is the most efficient method.
    // * On Windows XP and later, NtQuerySystemInformation with SystemExtendedHandleInformation.
    // * Otherwise, NtQuerySystemInformation with SystemHandleInformation can be used.

    if ((KphLevel() >= KphLevelMed) && ProcessHandle)
    {
        PKPH_PROCESS_HANDLE_INFORMATION handles;
        PSYSTEM_HANDLE_INFORMATION_EX convertedHandles;
        ULONG i;

        // Enumerate handles using KSystemInformer. Unlike with NtQuerySystemInformation,
        // this only enumerates handles for a single process and saves a lot of processing.

        if (NT_SUCCESS(status = KphEnumerateProcessHandles2(ProcessHandle, &handles)))
        {
            convertedHandles = PhAllocate(UFIELD_OFFSET(SYSTEM_HANDLE_INFORMATION_EX, Handles[handles->HandleCount]));
            convertedHandles->NumberOfHandles = handles->HandleCount;

            for (i = 0; i < handles->HandleCount; i++)
            {
                convertedHandles->Handles[i].Object = handles->Handles[i].Object;
                convertedHandles->Handles[i].UniqueProcessId = (ULONG_PTR)ProcessId;
                convertedHandles->Handles[i].HandleValue = (ULONG_PTR)handles->Handles[i].Handle;
                convertedHandles->Handles[i].GrantedAccess = (ULONG)handles->Handles[i].GrantedAccess;
                convertedHandles->Handles[i].CreatorBackTraceIndex = 0;
                convertedHandles->Handles[i].ObjectTypeIndex = handles->Handles[i].ObjectTypeIndex;
                convertedHandles->Handles[i].HandleAttributes = handles->Handles[i].HandleAttributes;
            }

            PhFree(handles);

            *Handles = convertedHandles;
            *FilterNeeded = FALSE;
        }
    }

    if (!NT_SUCCESS(status) && WindowsVersion >= WINDOWS_8 && ProcessHandle && PhGetIntegerSetting(L"EnableHandleSnapshot"))
    {
        PPROCESS_HANDLE_SNAPSHOT_INFORMATION handles;
        PSYSTEM_HANDLE_INFORMATION_EX convertedHandles;
        ULONG i;

        if (NT_SUCCESS(status = PhEnumHandlesEx2(ProcessHandle, &handles)))
        {
            convertedHandles = PhAllocate(UFIELD_OFFSET(SYSTEM_HANDLE_INFORMATION_EX, Handles[handles->NumberOfHandles]));
            convertedHandles->NumberOfHandles = handles->NumberOfHandles;

            for (i = 0; i < handles->NumberOfHandles; i++)
            {
                convertedHandles->Handles[i].Object = 0;
                convertedHandles->Handles[i].UniqueProcessId = (ULONG_PTR)ProcessId;
                convertedHandles->Handles[i].HandleValue = (ULONG_PTR)handles->Handles[i].HandleValue;
                convertedHandles->Handles[i].GrantedAccess = handles->Handles[i].GrantedAccess;
                convertedHandles->Handles[i].CreatorBackTraceIndex = 0;
                convertedHandles->Handles[i].ObjectTypeIndex = (USHORT)handles->Handles[i].ObjectTypeIndex;
                convertedHandles->Handles[i].HandleAttributes = handles->Handles[i].HandleAttributes;
            }

            PhFree(handles);

            *Handles = convertedHandles;
            *FilterNeeded = FALSE;
        }
    }

    if (!NT_SUCCESS(status))
    {
        PSYSTEM_HANDLE_INFORMATION_EX handles;

        if (!NT_SUCCESS(status = PhEnumHandlesEx(&handles)))
            return status;

        *Handles = handles;
        *FilterNeeded = TRUE;
    }

    return status;
}

NTSTATUS PhpCreateHandleItemFunction(
    _In_ PVOID Parameter
    )
{
    PPHP_CREATE_HANDLE_ITEM_CONTEXT context = Parameter;
    PPH_HANDLE_ITEM handleItem;

    handleItem = PhCreateHandleItem(context->Handle);

    PhGetHandleInformationEx(
        context->Provider->ProcessHandle,
        handleItem->Handle,
        context->Handle->ObjectTypeIndex,
        0,
        NULL,
        NULL,
        &handleItem->TypeName,
        &handleItem->ObjectName,
        &handleItem->BestObjectName,
        NULL
        );

    // HACK: Some security products block NtQueryObject with ObjectTypeInformation and return an invalid type
    // so we need to lookup the TypeName using the TypeIndex. We should improve PhGetHandleInformationEx for this case
    // but for now we'll preserve backwards compat by doing the lookup here. (dmex)
    if (PhIsNullOrEmptyString(handleItem->TypeName))
    {
        PhMoveReference(&handleItem->TypeName, PhGetObjectTypeName(handleItem->TypeIndex));
    }

    if (handleItem->TypeName)
    {
        // Add the handle item to the hashtable.
        PhAcquireQueuedLockExclusive(&context->Provider->HandleHashSetLock);
        PhpAddHandleItem(context->Provider, handleItem);
        PhReleaseQueuedLockExclusive(&context->Provider->HandleHashSetLock);

        // Raise the handle added event.
        PhInvokeCallback(&context->Provider->HandleAddedEvent, handleItem);
    }
    else
    {
        PhDereferenceObject(handleItem);
    }

    PhFree(context);

    return STATUS_SUCCESS;
}

VOID PhHandleProviderUpdate(
    _In_ PVOID Object
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static ULONG fileObjectTypeIndex = ULONG_MAX;

    PPH_HANDLE_PROVIDER handleProvider = (PPH_HANDLE_PROVIDER)Object;
    PSYSTEM_HANDLE_INFORMATION_EX handleInfo;
    BOOLEAN filterNeeded;
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handles;
    ULONG numberOfHandles;
    ULONG i;
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PPH_KEY_VALUE_PAIR handlePair;
    BOOLEAN useWorkQueue = FALSE;
    PH_WORK_QUEUE workQueue;
    KPH_LEVEL level;

    if (!NT_SUCCESS(handleProvider->RunStatus = PhEnumHandlesGeneric(
        handleProvider->ProcessId,
        handleProvider->ProcessHandle,
        &handleInfo,
        &filterNeeded
        )))
        goto UpdateExit;

    level = KphLevel();

    if ((level >= KphLevelMed))
    {
        useWorkQueue = TRUE;
        PhInitializeWorkQueue(&workQueue, 1, 20, 1000);

        if (PhBeginInitOnce(&initOnce))
        {
            fileObjectTypeIndex = PhGetObjectTypeNumberZ(L"File");
            PhEndInitOnce(&initOnce);
        }
    }

    handles = handleInfo->Handles;
    numberOfHandles = (ULONG)handleInfo->NumberOfHandles;

    // Make a list of the relevant handles.
    if (filterNeeded)
    {
        for (i = 0; i < numberOfHandles; i++)
        {
            PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handle = &handles[i];

            if (handle->UniqueProcessId == (ULONG_PTR)handleProvider->ProcessId)
            {
                PhAddItemSimpleHashtable(
                    handleProvider->TempListHashtable,
                    (PVOID)handle->HandleValue,
                    handle
                    );
            }
        }
    }
    else
    {
        for (i = 0; i < numberOfHandles; i++)
        {
            PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handle = &handles[i];

            PhAddItemSimpleHashtable(
                handleProvider->TempListHashtable,
                (PVOID)handle->HandleValue,
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
                    (PVOID)(handleItem->Handle)
                    );

                if (tempHashtableValue)
                {
                    // Also compare the object pointers to make sure a
                    // different object wasn't re-opened with the same
                    // handle value. This isn't 100% accurate as pool
                    // addresses may be re-used, but it works well.
                    if (handleItem->Object && handleItem->Object == (*tempHashtableValue)->Object)
                    {
                        found = TRUE;
                    }
                    else
                    {
                        if (
                            handleItem->Handle == (HANDLE)(*tempHashtableValue)->HandleValue &&
                            handleItem->GrantedAccess == (*tempHashtableValue)->GrantedAccess &&
                            handleItem->TypeIndex == (*tempHashtableValue)->ObjectTypeIndex
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
                PhpRemoveHandleItem(
                    handleProvider,
                    (PPH_HANDLE_ITEM)handlesToRemove->Items[i]
                    );
            }

            PhReleaseQueuedLockExclusive(&handleProvider->HandleHashSetLock);
            PhDereferenceObject(handlesToRemove);
        }
    }

    // Look for new handles and update existing ones.

    PhBeginEnumHashtable(handleProvider->TempListHashtable, &enumContext);

    while (handlePair = PhNextEnumHashtable(&enumContext))
    {
        PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handle = handlePair->Value;
        PPH_HANDLE_ITEM handleItem;

        handleItem = PhpLookupHandleItem(handleProvider, (HANDLE)handle->HandleValue);

        if (!handleItem)
        {
            // When we don't have KPH, query handle information in parallel to take full advantage of the
            // PhCallWithTimeout functionality.
            if (useWorkQueue && handle->ObjectTypeIndex == fileObjectTypeIndex)
            {
                PPHP_CREATE_HANDLE_ITEM_CONTEXT context;

                context = PhAllocate(sizeof(PHP_CREATE_HANDLE_ITEM_CONTEXT));
                context->Provider = handleProvider;
                context->Handle = handle;
                PhQueueItemWorkQueue(&workQueue, PhpCreateHandleItemFunction, context);
                continue;
            }

            handleItem = PhCreateHandleItem(handle);

            PhGetHandleInformationEx(
                handleProvider->ProcessHandle,
                handleItem->Handle,
                handle->ObjectTypeIndex,
                0,
                NULL,
                NULL,
                &handleItem->TypeName,
                &handleItem->ObjectName,
                &handleItem->BestObjectName,
                NULL
                );

            // HACK: Some security products block NtQueryObject with ObjectTypeInformation and return an invalid type
            // so we need to lookup the TypeName using the TypeIndex. We should improve PhGetHandleInformationEx for this case
            // but for now we'll preserve backwards compat by doing the lookup here. (dmex)
            if (PhIsNullOrEmptyString(handleItem->TypeName))
            {
                PPH_STRING typeName;

                if (typeName = PhGetObjectTypeName(handleItem->TypeIndex))
                {
                    PhMoveReference(&handleItem->TypeName, typeName);
                }
            }

            if (handleItem->TypeName && PhEqualString2(handleItem->TypeName, L"File", TRUE) && (level >= KphLevelMed))
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
                        handleItem->FileFlags |= PH_HANDLE_FILE_SHARED_READ;
                    if (objectInfo.SharedWrite)
                        handleItem->FileFlags |= PH_HANDLE_FILE_SHARED_WRITE;
                    if (objectInfo.SharedDelete)
                        handleItem->FileFlags |= PH_HANDLE_FILE_SHARED_DELETE;

                    // TODO add extra info from file objects here (jxy-s)
                    // objectInfo.HasActiveTransaction;
                    // objectInfo.UserWritableReferences;
                    // objectInfo.IsIgnoringSharing;
                    // ... more
                }
            }

            // Add the handle item to the hashtable.
            PhAcquireQueuedLockExclusive(&handleProvider->HandleHashSetLock);
            PhpAddHandleItem(handleProvider, handleItem);
            PhReleaseQueuedLockExclusive(&handleProvider->HandleHashSetLock);

            // Raise the handle added event.
            PhInvokeCallback(&handleProvider->HandleAddedEvent, handleItem);
        }
        else
        {
            BOOLEAN modified = FALSE;

            if (handleItem->Attributes != handle->HandleAttributes)
            {
                handleItem->Attributes = handle->HandleAttributes;
                modified = TRUE;
            }

            if (modified)
            {
                // Raise the handle modified event.
                PhInvokeCallback(&handleProvider->HandleModifiedEvent, handleItem);
            }
        }
    }

    if (useWorkQueue)
    {
        PhWaitForWorkQueue(&workQueue);
        PhDeleteWorkQueue(&workQueue);
    }

    PhFree(handleInfo);

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

UpdateExit:
    PhInvokeCallback(&handleProvider->HandleUpdatedEvent, NULL);
}
