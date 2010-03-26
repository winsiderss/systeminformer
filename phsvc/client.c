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
    __except (SIMPLE_EXCEPTION_FILTER(GetExceptionCode() == STATUS_INSUFFICIENT_RESOURCES))
    {
        return NULL;
    }

    if (!NT_SUCCESS(PhCreateObject(
        &client,
        sizeof(PHSVC_CLIENT),
        0,
        PhSvcClientType,
        0
        )))
    {
        PhDestroyHandleTable(handleTable);
        return NULL;
    }

    if (ClientId)
        client->ClientId = *ClientId;

    client->HandleTable = handleTable;

    PhAcquireQueuedLockExclusiveFast(&PhSvcClientListLock);
    InsertTailList(&PhSvcClientListHead, &client->ListEntry);
    PhReleaseQueuedLockExclusiveFast(&PhSvcClientListLock);

    return client;
}

VOID NTAPI PhSvcpClientDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPHSVC_CLIENT client = (PPHSVC_CLIENT)Object;

    PhAcquireQueuedLockExclusiveFast(&PhSvcClientListLock);
    RemoveEntryList(&client->ListEntry);
    PhReleaseQueuedLockExclusiveFast(&PhSvcClientListLock);

    PhDestroyHandleTable(client->HandleTable);
}

PPHSVC_CLIENT PhSvcReferenceClientByClientId(
    __in PCLIENT_ID ClientId
    )
{
    PLIST_ENTRY listEntry;
    PPHSVC_CLIENT client = NULL;

    PhAcquireQueuedLockSharedFast(&PhSvcClientListLock);

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

    PhReleaseQueuedLockSharedFast(&PhSvcClientListLock);

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

    handle = PhCreateHandle(client->HandleTable, &entry);

    if (!handle)
        return STATUS_INTERNAL_ERROR;

    *Handle = handle;

    return STATUS_SUCCESS;
}

NTSTATUS PhSvcCloseHandle(
    __in HANDLE Handle
    )
{
    PPHSVC_CLIENT client = PhSvcGetCurrentClient();
    PPH_HANDLE_TABLE_ENTRY entry;

    entry = PhGetHandleTableEntry(client->HandleTable, Handle);

    if (!entry)
        return STATUS_INVALID_HANDLE;

    PhDereferenceObject(entry->Object);
    PhDestroyHandle(client->HandleTable, Handle, entry);

    return STATUS_SUCCESS;
}

NTSTATUS PhSvcReferenceObjectByHandle(
    __out PPVOID Object,
    __in HANDLE Handle,
    __in_opt PPH_OBJECT_TYPE ObjectType,
    __in_opt ACCESS_MASK DesiredAccess,
    __in PHSVC_ACCESS_MODE AccessMode
    )
{
    PPHSVC_CLIENT client = PhSvcGetCurrentClient();
    PPH_HANDLE_TABLE_ENTRY entry;

    entry = PhGetHandleTableEntry(client->HandleTable, Handle);

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
