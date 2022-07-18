/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2015
 *
 */

#include <phapp.h>
#include <phsvc.h>

VOID NTAPI PhSvcpClientDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

PPH_OBJECT_TYPE PhSvcClientType = NULL;
LIST_ENTRY PhSvcClientListHead = { &PhSvcClientListHead, &PhSvcClientListHead };
PH_QUEUED_LOCK PhSvcClientListLock = PH_QUEUED_LOCK_INIT;

PPHSVC_CLIENT PhSvcCreateClient(
    _In_opt_ PCLIENT_ID ClientId
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    PPHSVC_CLIENT client;

    if (PhBeginInitOnce(&initOnce))
    {
        PhSvcClientType = PhCreateObjectType(L"PhSvcClient", 0, PhSvcpClientDeleteProcedure);
        PhEndInitOnce(&initOnce);
    }

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
