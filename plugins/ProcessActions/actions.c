#include <phdk.h>
#include "procdb.h"
#include "actions.h"

VOID NTAPI ProcessAddedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

PH_CALLBACK_REGISTRATION ProcessAddedCallbackRegistration;

VOID PaActionsInitialization()
{
    PhRegisterCallback(
        &PhProcessAddedEvent,
        ProcessAddedCallback,
        NULL,
        &ProcessAddedCallbackRegistration
        );
}

NTSTATUS PaMatchProcess(
    __in PPH_PROCESS_ITEM ProcessItem,
    __in PPA_PROCDB_SELECTOR Selector,
    __out PBOOLEAN Match
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    switch (Selector->Type)
    {
    case SelectorProcessName:
        *Match = PhMatchWildcards(
            Selector->u.ProcessName.Name->Buffer,
            ProcessItem->ProcessName->Buffer,
            TRUE
            );

        break;

    case SelectorFileName:
        if (ProcessItem->FileName)
        {
            *Match = PhMatchWildcards(
                Selector->u.FileName.Name->Buffer,
                ProcessItem->FileName->Buffer,
                TRUE
                );
        }
        else
        {
            return STATUS_UNSUCCESSFUL;
        }

        break;

    default:
        assert(FALSE);
        break;
    }

    return status;
}

NTSTATUS PaRunAction(
    __in PPH_PROCESS_ITEM ProcessItem,
    __in PPA_PROCDB_ACTION Action
    )
{
    NTSTATUS status = STATUS_SUCCESS;
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

VOID NTAPI ProcessAddedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    NTSTATUS status;
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;
    ULONG i;

    for (i = 0; i < PaProcDbList->Count; i++)
    {
        PPA_PROCDB_ENTRY entry = PaProcDbList->Items[i];
        BOOLEAN match;

        if (entry->Control.Type == ControlDisabled)
            continue;

        if (NT_SUCCESS(status = PaMatchProcess(processItem, &entry->Selector, &match)))
        {
            if (match)
            {
                ULONG j;

                for (j = 0; j < entry->Actions->Count; j++)
                    PaRunAction(processItem, entry->Actions->Items[j]);
            }
        }
    }
}
