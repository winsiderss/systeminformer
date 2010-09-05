#ifndef PA_ACTIONS_H
#define PA_ACTIONS_H

VOID PaActionsInitialization();

NTSTATUS PaMatchProcess(
    __in PPH_PROCESS_ITEM ProcessItem,
    __in PPA_PROCDB_SELECTOR Selector,
    __out PBOOLEAN Match
    );

NTSTATUS PaRunAction(
    __in PPH_PROCESS_ITEM ProcessItem,
    __in PPA_PROCDB_ACTION Action
    );

#endif
