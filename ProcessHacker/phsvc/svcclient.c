/*
 * Process Hacker -
 *   server client
 *
 * Copyright (C) 2011-2015 wj32
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
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

PPH_OBJECT_TYPE PhSvcClientType;
LIST_ENTRY PhSvcClientListHead;
PH_QUEUED_LOCK PhSvcClientListLock = PH_QUEUED_LOCK_INIT;

NTSTATUS PhSvcClientInitialization(
    VOID
    )
{
    PhSvcClientType = PhCreateObjectType(L"Client", 0, PhSvcpClientDeleteProcedure);
    InitializeListHead(&PhSvcClientListHead);

    return STATUS_SUCCESS;
}

PPHSVC_CLIENT PhSvcCreateClient(
    _In_opt_ PCLIENT_ID ClientId
    )
{
    PPHSVC_CLIENT client;

    client = PhCreateObject(sizeof(PHSVC_CLIENT), PhSvcClientType);
    memset(client, 0, sizeof(PHSVC_CLIENT));
    PhInitializeEvent(&client->ReadyEvent);

    if (ClientId)
        client->ClientId = *ClientId;

    PhAcquireQueuedLockExclusive(&PhSvcClientListLock);
    InsertTailList(&PhSvcClientListHead, &client->ListEntry);
    PhReleaseQueuedLockExclusive(&PhSvcClientListLock);

    return client;
}

VOID NTAPI PhSvcpClientDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPHSVC_CLIENT client = Object;

    PhAcquireQueuedLockExclusive(&PhSvcClientListLock);
    RemoveEntryList(&client->ListEntry);
    PhReleaseQueuedLockExclusive(&PhSvcClientListLock);

    if (client->PortHandle)
        NtClose(client->PortHandle);
}

PPHSVC_CLIENT PhSvcReferenceClientByClientId(
    _In_ PCLIENT_ID ClientId
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

PPHSVC_CLIENT PhSvcGetCurrentClient(
    VOID
    )
{
    return PhSvcGetCurrentThreadContext()->CurrentClient;
}

BOOLEAN PhSvcAttachClient(
    _In_ PPHSVC_CLIENT Client
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
    _In_ PPHSVC_CLIENT Client
    )
{
    PPHSVC_THREAD_CONTEXT threadContext = PhSvcGetCurrentThreadContext();

    PhDereferenceObject(threadContext->CurrentClient);
    threadContext->CurrentClient = threadContext->OldClient;
    threadContext->OldClient = NULL;
}
