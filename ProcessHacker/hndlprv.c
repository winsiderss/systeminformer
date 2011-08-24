/*
 * Process Hacker - 
 *   handle provider
 * 
 * Copyright (C) 2010-2011 wj32
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

#define PH_HNDLPRV_PRIVATE
#include <phapp.h>
#include <kphuser.h>
#include <extmgri.h>

VOID NTAPI PhpHandleProviderDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

VOID NTAPI PhpHandleItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

PPH_OBJECT_TYPE PhHandleProviderType;
PPH_OBJECT_TYPE PhHandleItemType;

BOOLEAN PhHandleProviderInitialization(
    VOID
    )
{
    if (!NT_SUCCESS(PhCreateObjectType(
        &PhHandleProviderType,
        L"HandleProvider",
        0,
        PhpHandleProviderDeleteProcedure
        )))
        return FALSE;

    if (!NT_SUCCESS(PhCreateObjectType(
        &PhHandleItemType,
        L"HandleItem",
        0,
        PhpHandleItemDeleteProcedure
        )))
        return FALSE;

    return TRUE;
}

PPH_HANDLE_PROVIDER PhCreateHandleProvider(
    __in HANDLE ProcessId
    )
{
    PPH_HANDLE_PROVIDER handleProvider;

    if (!NT_SUCCESS(PhCreateObject(
        &handleProvider,
        sizeof(PH_HANDLE_PROVIDER),
        0,
        PhHandleProviderType
        )))
        return NULL;

    handleProvider->HandleHashSetSize = 128;
    handleProvider->HandleHashSet = PhCreateHashSet(handleProvider->HandleHashSetSize);
    handleProvider->HandleHashSetCount = 0;
    PhInitializeQueuedLock(&handleProvider->HandleHashSetLock);

    PhInitializeCallback(&handleProvider->HandleAddedEvent);
    PhInitializeCallback(&handleProvider->HandleModifiedEvent);
    PhInitializeCallback(&handleProvider->HandleRemovedEvent);
    PhInitializeCallback(&handleProvider->UpdatedEvent);

    handleProvider->ProcessId = ProcessId;
    handleProvider->ProcessHandle = NULL;

    PhOpenProcess(
        &handleProvider->ProcessHandle,
        PROCESS_DUP_HANDLE,
        ProcessId
        );

    handleProvider->TempListHashtable = PhCreateSimpleHashtable(20);

    return handleProvider;
}

VOID PhpHandleProviderDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_HANDLE_PROVIDER handleProvider = (PPH_HANDLE_PROVIDER)Object;

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
    __in_opt PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Handle
    )
{
    PPH_HANDLE_ITEM handleItem;

    if (!NT_SUCCESS(PhCreateObject(
        &handleItem,
        PhEmGetObjectSize(EmHandleItemType, sizeof(PH_HANDLE_ITEM)),
        0,
        PhHandleItemType
        )))
        return NULL;

    memset(handleItem, 0, sizeof(PH_HANDLE_ITEM));

    if (Handle)
    {
        handleItem->Handle = (HANDLE)Handle->HandleValue;
        PhPrintPointer(handleItem->HandleString, (PVOID)handleItem->Handle);
        handleItem->Object = Handle->Object;
        PhPrintPointer(handleItem->ObjectString, handleItem->Object);
        handleItem->Attributes = Handle->HandleAttributes;
        handleItem->GrantedAccess = (ACCESS_MASK)Handle->GrantedAccess;
        PhPrintPointer(handleItem->GrantedAccessString, (PVOID)handleItem->GrantedAccess);
    }

    PhEmCallObjectOperation(EmHandleItemType, handleItem, EmObjectCreate);

    return handleItem;
}

VOID PhpHandleItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_HANDLE_ITEM handleItem = (PPH_HANDLE_ITEM)Object;

    PhEmCallObjectOperation(EmHandleItemType, handleItem, EmObjectDelete);

    if (handleItem->TypeName) PhDereferenceObject(handleItem->TypeName);
    if (handleItem->ObjectName) PhDereferenceObject(handleItem->ObjectName);
    if (handleItem->BestObjectName) PhDereferenceObject(handleItem->BestObjectName);
}

FORCEINLINE BOOLEAN PhCompareHandleItem(
    __in PPH_HANDLE_ITEM Value1,
    __in PPH_HANDLE_ITEM Value2
    )
{
    return Value1->Handle == Value2->Handle;
}

FORCEINLINE ULONG PhHashHandleItem(
    __in PPH_HANDLE_ITEM Value
    )
{
    return (ULONG)Value->Handle / 4;
}

PPH_HANDLE_ITEM PhpLookupHandleItem(
    __in __assumeLocked PPH_HANDLE_PROVIDER HandleProvider,
    __in HANDLE Handle
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
    __in PPH_HANDLE_PROVIDER HandleProvider,
    __in HANDLE Handle
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
    __in PPH_HANDLE_PROVIDER HandleProvider
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

__assumeLocked VOID PhpAddHandleItem(
    __in PPH_HANDLE_PROVIDER HandleProvider,
    __in __assumeRefs(1) PPH_HANDLE_ITEM HandleItem
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

__assumeLocked VOID PhpRemoveHandleItem(
    __in PPH_HANDLE_PROVIDER HandleProvider,
    __in PPH_HANDLE_ITEM HandleItem
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
    __in HANDLE ProcessId,
    __in HANDLE ProcessHandle,
    __out PSYSTEM_HANDLE_INFORMATION_EX *Handles,
    __out PBOOLEAN FilterNeeded
    )
{
    NTSTATUS status;

    // There are three ways of enumerating handles:
    // * When KProcessHacker is available, using KphEnumerateProcessHandles 
    //   is the most efficient method.
    // * On Windows XP and later, NtQuerySystemInformation with 
    //   SystemExtendedHandleInformation can be used.
    // * Otherwise, NtQuerySystemInformation with SystemHandleInformation 
    //   can be used.

    if (KphIsConnected())
    {
        PKPH_PROCESS_HANDLE_INFORMATION handles;
        PSYSTEM_HANDLE_INFORMATION_EX convertedHandles;
        ULONG i;

        // Enumerate handles using KProcessHacker. Unlike with NtQuerySystemInformation, 
        // this only enumerates handles for a single process and saves a lot of processing.

        if (NT_SUCCESS(status = PhEnumProcessHandles(ProcessHandle, &handles)))
        {
            convertedHandles = PhAllocate(
                FIELD_OFFSET(SYSTEM_HANDLE_INFORMATION_EX, Handles) +
                sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX) * handles->HandleCount
                );

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

            return status;
        }
    }

    if (WindowsVersion >= WINDOWS_XP)
    {
        PSYSTEM_HANDLE_INFORMATION_EX handles;

        // Enumerate handles using the new method; no conversion 
        // necessary.

        if (!NT_SUCCESS(status = PhEnumHandlesEx(&handles)))
            return status;

        *Handles = handles;
        *FilterNeeded = TRUE;
    }
    else
    {
        PSYSTEM_HANDLE_INFORMATION handles;
        PSYSTEM_HANDLE_INFORMATION_EX convertedHandles;
        ULONG count;
        ULONG allocatedCount;
        ULONG i;

        // Enumerate handles using the old info class and convert 
        // the relevant entries to the new format.

        if (!NT_SUCCESS(status = PhEnumHandles(&handles)))
            return status;

        count = 0;
        allocatedCount = 100;

        convertedHandles = PhAllocate(
            FIELD_OFFSET(SYSTEM_HANDLE_INFORMATION_EX, Handles) +
            sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX) * allocatedCount
            );

        for (i = 0; i < handles->NumberOfHandles; i++)
        {
            if ((HANDLE)handles->Handles[i].UniqueProcessId != ProcessId)
                continue;

            if (count == allocatedCount)
            {
                allocatedCount *= 2;
                convertedHandles = PhReAllocate(
                    convertedHandles,
                    FIELD_OFFSET(SYSTEM_HANDLE_INFORMATION_EX, Handles) +
                    sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX) * allocatedCount
                    );
            }

            convertedHandles->Handles[count].Object = handles->Handles[i].Object;
            convertedHandles->Handles[count].UniqueProcessId = (ULONG_PTR)handles->Handles[i].UniqueProcessId;
            convertedHandles->Handles[count].HandleValue = (ULONG_PTR)handles->Handles[i].HandleValue;
            convertedHandles->Handles[count].GrantedAccess = handles->Handles[i].GrantedAccess;
            convertedHandles->Handles[count].CreatorBackTraceIndex = handles->Handles[i].CreatorBackTraceIndex;
            convertedHandles->Handles[count].ObjectTypeIndex = handles->Handles[i].ObjectTypeIndex;
            convertedHandles->Handles[count].HandleAttributes = (ULONG)handles->Handles[i].HandleAttributes;

            count++;
        }

        convertedHandles->NumberOfHandles = count;

        PhFree(handles);

        *Handles = convertedHandles;
        *FilterNeeded = FALSE;
    }

    return STATUS_SUCCESS;
}

VOID PhHandleProviderUpdate(
    __in PVOID Object
    )
{
    PPH_HANDLE_PROVIDER handleProvider = (PPH_HANDLE_PROVIDER)Object;
    PSYSTEM_HANDLE_INFORMATION_EX handleInfo;
    BOOLEAN filterNeeded;
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handles;
    ULONG numberOfHandles;
    ULONG i;
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PPH_KEY_VALUE_PAIR handlePair;

    if (!handleProvider->ProcessHandle)
        return;

    if (!NT_SUCCESS(PhEnumHandlesGeneric(
        handleProvider->ProcessId,
        handleProvider->ProcessHandle,
        &handleInfo,
        &filterNeeded
        )))
        return;

    handles = handleInfo->Handles;
    numberOfHandles = (ULONG)handleInfo->NumberOfHandles;

    // Make a list of the relevant handles.
    if (filterNeeded)
    {
        for (i = 0; i < (ULONG)numberOfHandles; i++)
        {
            PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handle = &handles[i];

            if (handle->UniqueProcessId == (USHORT)handleProvider->ProcessId)
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
        for (i = 0; i < (ULONG)numberOfHandles; i++)
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
                    if (handleItem->Object == (*tempHashtableValue)->Object)
                    {
                        found = TRUE;
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

            // We need at least a type name to continue.
            if (!handleItem->TypeName)
            {
                PhDereferenceObject(handleItem);
                continue;
            }

            if (PhEqualString2(handleItem->TypeName, L"File", TRUE) && KphIsConnected())
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

    PhInvokeCallback(&handleProvider->UpdatedEvent, NULL);
}
