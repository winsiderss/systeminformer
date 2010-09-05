#ifndef PA_PROCDB_H
#define PA_PROCDB_H

typedef enum _PA_PROCDB_CONTROL_TYPE
{
    ControlInvalid = 0,
    ControlDisabled = 1,
    ControlRunOnce = 2,
    ControlRunPeriod = 3
} PA_PROCDB_CONTROL_TYPE;

typedef struct _PA_PROCDB_CONTROL
{
    PA_PROCDB_CONTROL_TYPE Type;

    union
    {
        struct
        {
            ULONG Period; // in seconds
        } RunPeriod;
    } u;
} PA_PROCDB_CONTROL, *PPA_PROCDB_CONTROL;

typedef enum _PA_PROCDB_SELECTOR_TYPE
{
    SelectorInvalid = 0,
    SelectorProcessName = 1,
    SelectorFileName = 2
} PA_PROCDB_SELECTOR_TYPE;

typedef struct _PA_PROCDB_SELECTOR
{
    PA_PROCDB_SELECTOR_TYPE Type;

    union
    {
        struct
        {
            PPH_STRING Name;
        } ProcessName;
        struct
        {
            PPH_STRING Name;
        } FileName;
    } u;
} PA_PROCDB_SELECTOR, *PPA_PROCDB_SELECTOR;

typedef enum _PA_PROCDB_ACTION_TYPE
{
    ActionInvalid = 0,
    ActionSetPriorityClass = 1,
    ActionSetAffinityMask = 2,
    ActionTerminate = 3
} PA_PROCDB_ACTION_TYPE;

typedef struct _PA_PROCDB_ACTION
{
    PA_PROCDB_ACTION_TYPE Type;

    union
    {
        struct
        {
            ULONG PriorityClassWin32;
        } SetPriorityClass;
        struct
        {
            ULONG_PTR AffinityMask;
        } SetAffinityMask;
    } u;
} PA_PROCDB_ACTION, *PPA_PROCDB_ACTION;

typedef struct _PA_PROCDB_ENTRY
{
    PA_PROCDB_CONTROL Control;
    PA_PROCDB_SELECTOR Selector;

    PPH_LIST Actions;
} PA_PROCDB_ENTRY, *PPA_PROCDB_ENTRY;

#ifndef PA_PROCDB_PRIVATE
extern PPH_LIST PaProcDbList;
extern PPH_STRING PaProcDbFileName;
#endif

VOID PaProcDbInitialization();

PPA_PROCDB_ENTRY PaAllocateProcDbEntry();

VOID PaFreeProcDbEntry(
    __inout PPA_PROCDB_ENTRY Entry
    );

VOID PaDeleteProcDbSelector(
    __inout PPA_PROCDB_SELECTOR Selector
    );

PPA_PROCDB_ACTION PaAllocateProcDbAction(
    __in_opt PPA_PROCDB_ACTION Action
    );

VOID PaFreeProcDbAction(
    __inout PPA_PROCDB_ACTION Action
    );

VOID PaDeleteProcDbAction(
    __inout PPA_PROCDB_ACTION Action
    );

PPH_STRING PaFormatProcDbEntry(
    __in PPA_PROCDB_ENTRY Entry
    );

VOID PaClearProcDb();

NTSTATUS PaLoadProcDb(
    __in PWSTR FileName
    );

NTSTATUS PaSaveProcDb(
    __in PWSTR FileName
    );

#endif
