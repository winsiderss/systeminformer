#include <ph.h>

NTSTATUS PhEnumProcesses(
    __in PPH_ENUM_PROCESSES_CALLBACK Callback,
    __out_opt PBOOLEAN Found
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 2048;
    PSYSTEM_PROCESS_INFORMATION procInfo;

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
            return status;
        }
    }

    procInfo = (PSYSTEM_PROCESS_INFORMATION)buffer;

    while (TRUE)
    {
        if (Callback(procInfo))
        {
            if (Found)
                *Found = TRUE;

            return STATUS_SUCCESS;
        }

        if (procInfo->NextEntryOffset == 0)
            break;

        procInfo = (PSYSTEM_PROCESS_INFORMATION)((PCHAR)procInfo + procInfo->NextEntryOffset);
    }

    PhFree(buffer);

    if (Found)
        *Found = FALSE;

    return STATUS_SUCCESS;
}
