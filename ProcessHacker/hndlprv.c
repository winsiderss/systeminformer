/*
 * Process Hacker - 
 *   handle provider
 * 
 * Copyright (C) 2010 wj32
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

#define HNDLPRV_PRIVATE
#include <phapp.h>

VOID NTAPI PhpHandleProviderDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

VOID NTAPI PhpHandleItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

BOOLEAN NTAPI PhpHandleHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    );

ULONG NTAPI PhpHandleHashtableHashFunction(
    __in PVOID Entry
    );

PPH_OBJECT_TYPE PhHandleProviderType;
PPH_OBJECT_TYPE PhHandleItemType;

BOOLEAN PhInitializeHandleProvider()
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
        PhHandleProviderType,
        0
        )))
        return NULL;

    handleProvider->HandleHashtable = PhCreateHashtable(
        sizeof(PPH_HANDLE_ITEM),
        PhpHandleHashtableCompareFunction,
        PhpHandleHashtableHashFunction,
        20
        );
    PhInitializeQueuedLock(&handleProvider->HandleHashtableLock);

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

    PhDereferenceObject(handleProvider->HandleHashtable);
    PhDeleteCallback(&handleProvider->HandleAddedEvent);
    PhDeleteCallback(&handleProvider->HandleModifiedEvent);
    PhDeleteCallback(&handleProvider->HandleRemovedEvent);

    if (handleProvider->ProcessHandle) NtClose(handleProvider->ProcessHandle);

    PhDereferenceObject(handleProvider->TempListHashtable);
}

PPH_HANDLE_ITEM PhCreateHandleItem(
    __in_opt PSYSTEM_HANDLE_TABLE_ENTRY_INFO Handle
    )
{
    PPH_HANDLE_ITEM handleItem;

    if (!NT_SUCCESS(PhCreateObject(
        &handleItem,
        sizeof(PH_HANDLE_ITEM),
        0,
        PhHandleItemType,
        0
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

    return handleItem;
}

VOID PhpHandleItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_HANDLE_ITEM handleItem = (PPH_HANDLE_ITEM)Object;

    if (handleItem->TypeName) PhDereferenceObject(handleItem->TypeName);
    if (handleItem->ObjectName) PhDereferenceObject(handleItem->ObjectName);
    if (handleItem->BestObjectName) PhDereferenceObject(handleItem->BestObjectName);
}

BOOLEAN PhpHandleHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    return
        (*(PPH_HANDLE_ITEM *)Entry1)->Handle ==
        (*(PPH_HANDLE_ITEM *)Entry2)->Handle;
}

ULONG PhpHandleHashtableHashFunction(
    __in PVOID Entry
    )
{
    return (ULONG)(*(PPH_HANDLE_ITEM *)Entry)->Handle / 4;
}

PPH_HANDLE_ITEM PhpLookupHandleItem(
    __in __assumeLocked PPH_HANDLE_PROVIDER HandleProvider,
    __in HANDLE Handle
    )
{
    PH_HANDLE_ITEM lookupHandleItem;
    PPH_HANDLE_ITEM lookupHandleItemPtr = &lookupHandleItem;
    PPH_HANDLE_ITEM *handleItemPtr;

    lookupHandleItem.Handle = Handle;

    handleItemPtr = (PPH_HANDLE_ITEM *)PhGetHashtableEntry(
        HandleProvider->HandleHashtable,
        &lookupHandleItemPtr
        );

    if (handleItemPtr)
        return *handleItemPtr;
    else
        return NULL;
}

PPH_HANDLE_ITEM PhReferenceHandleItem(
    __in PPH_HANDLE_PROVIDER HandleProvider,
    __in HANDLE Handle
    )
{
    PPH_HANDLE_ITEM handleItem;

    PhAcquireQueuedLockSharedFast(&HandleProvider->HandleHashtableLock);

    handleItem = PhpLookupHandleItem(HandleProvider, Handle);

    if (handleItem)
        PhReferenceObject(handleItem);

    PhReleaseQueuedLockSharedFast(&HandleProvider->HandleHashtableLock);

    return handleItem;
}

VOID PhDereferenceAllHandleItems(
    __in PPH_HANDLE_PROVIDER HandleProvider
    )
{
    ULONG enumerationKey = 0;
    PPH_HANDLE_ITEM *handleItem;

    PhAcquireQueuedLockExclusiveFast(&HandleProvider->HandleHashtableLock);

    while (PhEnumHashtable(HandleProvider->HandleHashtable, (PPVOID)&handleItem, &enumerationKey))
    {
        PhDereferenceObject(*handleItem);
    }

    PhReleaseQueuedLockExclusiveFast(&HandleProvider->HandleHashtableLock);
}

__assumeLocked VOID PhpRemoveHandleItem(
    __in PPH_HANDLE_PROVIDER HandleProvider,
    __in PPH_HANDLE_ITEM HandleItem
    )
{
    PhRemoveHashtableEntry(HandleProvider->HandleHashtable, &HandleItem);
    PhDereferenceObject(HandleItem);
}

VOID PhHandleProviderUpdate(
    __in PVOID Object
    )
{
    PPH_HANDLE_PROVIDER handleProvider = (PPH_HANDLE_PROVIDER)Object;
    PSYSTEM_HANDLE_INFORMATION handleInfo;
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO handles;
    ULONG numberOfHandles;
    ULONG i;
    PPH_KEY_VALUE_PAIR handlePair;

    if (!handleProvider->ProcessHandle)
        return;

    if (!NT_SUCCESS(PhEnumHandles(&handleInfo)))
        return;

    handles = handleInfo->Handles;
    numberOfHandles = handleInfo->NumberOfHandles;

    // Make a list of the relevant handles.
    for (i = 0; i < numberOfHandles; i++)
    {
        PSYSTEM_HANDLE_TABLE_ENTRY_INFO handle = &handles[i];

        if (handle->UniqueProcessId == (USHORT)handleProvider->ProcessId)
        {
            PhAddSimpleHashtableItem(
                handleProvider->TempListHashtable,
                (PVOID)handle->HandleValue,
                handle
                );
        }
    }

    // Look for closed handles.
    {
        PPH_LIST handlesToRemove = NULL;
        ULONG enumerationKey = 0;
        PPH_HANDLE_ITEM *handleItem;
        PSYSTEM_HANDLE_TABLE_ENTRY_INFO *tempHashtableValue;

        while (PhEnumHashtable(handleProvider->HandleHashtable, (PPVOID)&handleItem, &enumerationKey))
        {
            BOOLEAN found = FALSE;

            // Check if the handle still exists.

            tempHashtableValue = (PSYSTEM_HANDLE_TABLE_ENTRY_INFO *)PhGetSimpleHashtableItem(
                handleProvider->TempListHashtable,
                (PVOID)((*handleItem)->Handle)
                );

            if (tempHashtableValue)
            {
                // Also compare the object pointers to make sure a 
                // different object wasn't re-opened with the same 
                // handle value. This isn't 100% accurate as pool 
                // addresses may be re-used, but it works well.
                if ((*handleItem)->Object == (*tempHashtableValue)->Object)
                {
                    found = TRUE;
                }
            }

            if (!found)
            {
                // Raise the handle removed event.
                PhInvokeCallback(&handleProvider->HandleRemovedEvent, *handleItem);

                if (!handlesToRemove)
                    handlesToRemove = PhCreateList(2);

                PhAddListItem(handlesToRemove, *handleItem);
            }
        }

        if (handlesToRemove)
        {
            PhAcquireQueuedLockExclusiveFast(&handleProvider->HandleHashtableLock);

            for (i = 0; i < handlesToRemove->Count; i++)
            {
                PhpRemoveHandleItem(
                    handleProvider,
                    (PPH_HANDLE_ITEM)handlesToRemove->Items[i]
                    );
            }

            PhReleaseQueuedLockExclusiveFast(&handleProvider->HandleHashtableLock);
            PhDereferenceObject(handlesToRemove);
        }
    }

    // Look for new handles and update existing ones.

    i = 0;

    while (PhEnumHashtable(handleProvider->TempListHashtable, &handlePair, &i))
    {
        PSYSTEM_HANDLE_TABLE_ENTRY_INFO handle = handlePair->Value;
        PPH_HANDLE_ITEM handleItem;

        handleItem = PhpLookupHandleItem(handleProvider, (HANDLE)handle->HandleValue);

        if (!handleItem)
        {
            handleItem = PhCreateHandleItem(handle);

            PhGetHandleInformation(
                handleProvider->ProcessHandle,
                handleItem->Handle,
                handle->ObjectTypeIndex,
                NULL,
                &handleItem->TypeName,
                &handleItem->ObjectName,
                &handleItem->BestObjectName
                );

            // We need at least a type name to continue.
            if (!handleItem->TypeName)
            {
                PhDereferenceObject(handleItem);
                continue;
            }

            // Add the handle item to the hashtable.
            PhAcquireQueuedLockExclusiveFast(&handleProvider->HandleHashtableLock);
            PhAddHashtableEntry(handleProvider->HandleHashtable, &handleItem);
            PhReleaseQueuedLockExclusiveFast(&handleProvider->HandleHashtableLock);

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
