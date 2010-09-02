#include <phdk.h>
#define PA_PROCDB_PRIVATE
#include "procdb.h"
#include <mxml.h>

VOID PaDeleteProcDbSelector(
    __inout PPA_PROCDB_SELECTOR Selector
    );

VOID PaDeleteProcDbAction(
    __inout PPA_PROCDB_ACTION Action
    );

PPH_LIST PaProcDbList;

static WCHAR *PaSelectorNames[] =
{
    L"Invalid",
    L"ProcessName",
    L"FileName"
};

static WCHAR *PaActionNames[] =
{
    L"Invalid",
    L"SetPriorityClass",
    L"SetAffinityMask",
    L"Terminate"
};

VOID PaProcDbInitialization()
{
    PaProcDbList = PhCreateList(10);
}

PPA_PROCDB_ENTRY PaAllocateProcDbEntry()
{
    PPA_PROCDB_ENTRY entry;

    entry = PhAllocate(sizeof(PA_PROCDB_ENTRY));
    memset(entry, 0, sizeof(PA_PROCDB_ENTRY));

    return entry;
}

VOID PaFreeProcDbEntry(
    __inout PPA_PROCDB_ENTRY Entry
    )
{
    PaDeleteProcDbSelector(&Entry->Selector);
    PaDeleteProcDbAction(&Entry->Action);
}

VOID PaDeleteProcDbSelector(
    __inout PPA_PROCDB_SELECTOR Selector
    )
{
    switch (Selector->Type)
    {
    case SelectorProcessName:
        PhDereferenceObject(Selector->u.ProcessName.Name);
        break;
    case SelectorFileName:
        PhDereferenceObject(Selector->u.FileName.Name);
        break;
    default:
        assert(FALSE);
        break;
    }
}

VOID PaDeleteProcDbAction(
    __inout PPA_PROCDB_ACTION Action
    )
{
    switch (Action->Type)
    {
    case ActionSetPriorityClass:
        break;
    case ActionSetAffinityMask:
        break;
    case ActionTerminate:
        break;
    default:
        assert(FALSE);
        break;
    }
}
