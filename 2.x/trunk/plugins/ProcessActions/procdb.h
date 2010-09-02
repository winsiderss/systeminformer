#ifndef PA_PROCDB_H
#define PA_PROCDB_H

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
    PA_PROCDB_SELECTOR Selector;
    PA_PROCDB_ACTION Action;
} PA_PROCDB_ENTRY, *PPA_PROCDB_ENTRY;

#ifndef PA_PROCDB_PRIVATE
extern PPH_LIST PaProcDbList;
#endif

PPA_PROCDB_ENTRY PaAllocateProcDbEntry();

VOID PaFreeProcDbEntry(
    __inout PPA_PROCDB_ENTRY Entry
    );

#endif
