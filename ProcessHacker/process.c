#define PROCESS_PRIVATE
#include <ph.h>

VOID PhpProcessItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

PPH_OBJECT_TYPE PhProcessItemType;

BOOLEAN PhInitializeProcessItem()
{
    return NT_SUCCESS(PhCreateObjectType(
        &PhProcessItemType,
        0,
        PhpProcessItemDeleteProcedure
        ));
}

PPH_PROCESS_ITEM PhCreateProcessItem(
    __in HANDLE ProcessId
    )
{
    PPH_PROCESS_ITEM processItem;

    if (!NT_SUCCESS(PhCreateObject(
        &processItem,
        sizeof(PH_PROCESS_ITEM),
        0,
        PhProcessItemType,
        0
        )))
        return NULL;

    memset(processItem, 0, sizeof(PH_PROCESS_ITEM));
    processItem->ProcessId = ProcessId;

    return processItem;
}

VOID PhpProcessItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Object;

    if (processItem->ProcessName)
        PhDereferenceObject(processItem->ProcessName);
}

NTSTATUS PhEnumProcesses(
    __out PPVOID Processes
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 2048;

    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        if (!buffer)
            return STATUS_INSUFFICIENT_RESOURCES;

        status = NtQuerySystemInformation(
            SystemProcessInformation,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (NT_SUCCESS(status))
            break;

        if (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INFO_LENGTH_MISMATCH)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);
        }
        else
        {
            PhFree(buffer);
            return status;
        }
    }

    *Processes = buffer;

    return STATUS_SUCCESS;
}
