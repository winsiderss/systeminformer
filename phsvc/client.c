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

    if (!NT_SUCCESS(PhCreateObject(
        &client,
        sizeof(PHSVC_CLIENT),
        0,
        PhSvcClientType,
        0
        )))
        return NULL;

    if (ClientId)
        client->ClientId = *ClientId;

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
}
