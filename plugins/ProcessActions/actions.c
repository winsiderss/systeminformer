#include <phdk.h>
#include "procdb.h"

NTSTATUS PaMatchProcess(
    __in PPH_PROCESS_ITEM ProcessItem,
    __in PPA_PROCDB_SELECTOR Selector,
    __out PBOOLEAN Match
    )
{

}

NTSTATUS PaRunAction(
    __in PPH_PROCESS_ITEM ProcessItem,
    __in PPA_PROCDB_ACTION Action
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    switch (Action->Type)
    {
    case ActionSetPriorityClass:
        if (NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            PROCESS_SET_INFORMATION,
            ProcessItem->ProcessId
            )))
        {
            if (!SetPriorityClass(processHandle, Action->u.SetPriorityClass.PriorityClassWin32))
                status = NTSTATUS_FROM_WIN32(GetLastError());

            NtClose(processHandle);
        }

        break;

    case ActionSetAffinityMask:
        if (NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            PROCESS_SET_INFORMATION,
            ProcessItem->ProcessId
            )))
        {
            status = PhSetProcessAffinityMask(processHandle, Action->u.SetAffinityMask.AffinityMask);
            NtClose(processHandle);
        }

        break;

    case ActionTerminate:
        if (NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            PROCESS_TERMINATE,
            ProcessItem->ProcessId
            )))
        {
            status = PhTerminateProcess(processHandle, STATUS_SUCCESS);
            NtClose(processHandle);
        }

        break;

    default:
        assert(FALSE);
        break;
    }

    return status;
}
