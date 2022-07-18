#ifndef PH_PROCGRP_H
#define PH_PROCGRP_H

// begin_phapppub
typedef struct _PH_PROCESS_GROUP
{
    PPH_PROCESS_ITEM Representative; // An element of Processes (no extra reference added)
    PPH_LIST Processes; // List of PPH_PROCESS_ITEM
    HWND WindowHandle; // Window handle of representative
} PH_PROCESS_GROUP, *PPH_PROCESS_GROUP;
// end_phapppub

typedef VOID (NTAPI *PPH_SORT_LIST_FUNCTION)(
    _In_ PPH_LIST List,
    _In_opt_ PVOID Context
    );

#define PH_GROUP_PROCESSES_DONT_GROUP 0x1
#define PH_GROUP_PROCESSES_FILE_PATH 0x2

PPH_LIST PhCreateProcessGroupList(
    _In_opt_ PPH_SORT_LIST_FUNCTION SortListFunction, // Sort a list of PPH_PROCESS_NODE
    _In_opt_ PVOID Context,
    _In_ ULONG MaximumGroups,
    _In_ ULONG Flags
    );

VOID PhFreeProcessGroupList(
    _In_ PPH_LIST List
    );

#endif
