/*
 * Process Hacker - 
 *   server client
 * 
 * Copyright (C) 2011 wj32
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
#include <phsvc.h>

VOID NTAPI PhSvcpClientDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

PPH_OBJECT_TYPE PhSvcClientType;
LIST_ENTRY PhSvcClientListHead;
PH_QUEUED_LOCK PhSvcClientListLock = PH_QUEUED_LOCK_INIT;

NTSTATUS PhSvcClientInitialization()
{
    NTSTATUS status;

    if (!NT_SUCCESS(status = PhCreateObjectType(
        &PhSvcClientType,
        L"Client",
        0,
        PhSvcpClientDeleteProcedure
        )))
        return status;

    InitializeListHead(&PhSvcClientListHead);

    return STATUS_SUCCESS;
}

PPHSVC_CLIENT PhSvcCreateClient(
    __in_opt PCLIENT_ID ClientId
    )
{
    PPHSVC_CLIENT client;
    PPH_HANDLE_TABLE handleTable;

    __try
    {
        handleTable = PhCreateHandleTable();
    }
    __except (SIMPLE_EXCEPTION_FILTER(GetExceptionCode() == STATUS_NO_MEMORY))
    {
        return NULL;
    }

    if (!NT_SUCCESS(PhCreateObject(
        &client,
        sizeof(PHSVC_CLIENT),
        0,
        PhSvcClientType
        )))
    {
        PhDestroyHandleTable(handleTable);
        return NULL;
    }

    memset(client, 0, sizeof(PHSVC_CLIENT));

    if (ClientId)
        client->ClientId = *ClientId;

    client->HandleTable = handleTable;

    PhAcquireQueuedLockExclusive(&PhSvcClientListLock);
    InsertTailList(&PhSvcClientListHead, &client->ListEntry);
    PhReleaseQueuedLockExclusive(&PhSvcClientListLock);

    return client;
}

VOID NTAPI PhSvcpClientDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPHSVC_CLIENT client = (PPHSVC_CLIENT)Object;

    PhAcquireQueuedLockExclusive(&PhSvcClientListLock);
    RemoveEntryList(&client->ListEntry);
    PhReleaseQueuedLockExclusive(&PhSvcClientListLock);

    PhDestroyHandleTable(client->HandleTable);

    if (client->PortHandle)
        NtClose(client->PortHandle);
}

PPHSVC_CLIENT PhSvcReferenceClientByClientId(
    __in PCLIENT_ID ClientId
    )
{
    PLIST_ENTRY listEntry;
    PPHSVC_CLIENT client = NULL;

    PhAcquireQueuedLockShared(&PhSvcClientListLock);

    listEntry = PhSvcClientListHead.Flink;

    while (listEntry != &PhSvcClientListHead)
    {
        client = CONTAINING_RECORD(listEntry, PHSVC_CLIENT, ListEntry);

        if (ClientId->UniqueThread)
        {
            if (
                client->ClientId.UniqueProcess == ClientId->UniqueProcess &&
                client->ClientId.UniqueThread == ClientId->UniqueThread
                )
            {
                break;
            }
        }
        else
        {
            if (client->ClientId.UniqueProcess == ClientId->UniqueProcess)
                break;
        }

        client = NULL;

        listEntry = listEntry->Flink;
    }

    if (client)
    {
        if (!PhReferenceObjectSafe(client))
            client = NULL;
    }

    PhReleaseQueuedLockShared(&PhSvcClientListLock);

    return client;
}

PPHSVC_CLIENT PhSvcGetCurrentClient()
{
    return PhSvcGetCurrentThreadContext()->CurrentClient;
}

BOOLEAN PhSvcAttachClient(
    __in PPHSVC_CLIENT Client
    )
{
    PPHSVC_THREAD_CONTEXT threadContext = PhSvcGetCurrentThreadContext();

    if (threadContext->OldClient)
        return FALSE;

    PhReferenceObject(Client);
    threadContext->OldClient = threadContext->CurrentClient;
    threadContext->CurrentClient = Client;

    return TRUE;
}

VOID PhSvcDetachClient(
    __in PPHSVC_CLIENT Client
    )
{
    PPHSVC_THREAD_CONTEXT threadContext = PhSvcGetCurrentThreadContext();

    PhDereferenceObject(threadContext->CurrentClient);
    threadContext->CurrentClient = threadContext->OldClient;
    threadContext->OldClient = NULL;
}

NTSTATUS PhSvcCreateHandle(
    __out PHANDLE Handle,
    __in PVOID Object,
    __in ACCESS_MASK GrantedAccess
    )
{
    PPHSVC_CLIENT client = PhSvcGetCurrentClient();
    HANDLE handle;
    PH_HANDLE_TABLE_ENTRY entry;

    entry.Object = Object;
    entry.GrantedAccess = GrantedAccess;

    __try
    {
        handle = PhCreateHandle(client->HandleTable, &entry);
    }
    __except (SIMPLE_EXCEPTION_FILTER(GetExceptionCode() == STATUS_NO_MEMORY))
    {
        return STATUS_NO_MEMORY;
    }

    if (!handle)
        return STATUS_NO_MEMORY;

    PhReferenceObject(Object);

    *Handle = handle;

    return STATUS_SUCCESS;
}

NTSTATUS PhSvcCloseHandle(
    __in HANDLE Handle
    )
{
    PPHSVC_CLIENT client = PhSvcGetCurrentClient();
    PPH_HANDLE_TABLE_ENTRY entry;

    entry = PhLookupHandleTableEntry(client->HandleTable, Handle);

    if (!entry)
        return STATUS_INVALID_HANDLE;

    PhDereferenceObject(entry->Object);
    PhDestroyHandle(client->HandleTable, Handle, entry);

    return STATUS_SUCCESS;
}

NTSTATUS PhSvcReferenceObjectByHandle(
    __in HANDLE Handle,
    __in_opt PPH_OBJECT_TYPE ObjectType,
    __in_opt ACCESS_MASK DesiredAccess,
    __out PVOID *Object
    )
{
    PPHSVC_CLIENT client = PhSvcGetCurrentClient();
    PPH_HANDLE_TABLE_ENTRY entry;

    entry = PhLookupHandleTableEntry(client->HandleTable, Handle);

    if (!entry)
        return STATUS_INVALID_HANDLE;

    if (ObjectType && PhGetObjectType(entry->Object) != ObjectType)
    {
        PhUnlockHandleTableEntry(client->HandleTable, entry);
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    PhReferenceObject(entry->Object);
    *Object = entry->Object;

    PhUnlockHandleTableEntry(client->HandleTable, entry);

    return STATUS_SUCCESS;
}
