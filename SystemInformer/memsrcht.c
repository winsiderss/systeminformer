/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2024
 *
 */

#include <phapp.h>
#include <emenu.h>
#include <mainwnd.h>
#include <memsrch.h>
#include <memprv.h>
#include <procprv.h>
#include <settings.h>
#include <phsettings.h>
#include <strsrch.h>
#include <colmgr.h>
#include <cpysave.h>

#define WM_PH_MEMSEARCH_SHOWMENU    (WM_USER + 3000)
#define WM_PH_MEMSEARCH_FINISHED    (WM_USER + 3001)

#define PH_MEMSEARCH_STATE_STOPPED   0
#define PH_MEMSEARCH_STATE_SEARCHING 1
#define PH_MEMSEARCH_STATE_FINISHED  2

static CONST PH_STRINGREF EmptyStringsText = PH_STRINGREF_INIT(L"There are no strings to display.");
static CONST PH_STRINGREF LoadingStringsText = PH_STRINGREF_INIT(L"Loading strings...");

typedef struct _PH_MEMSTRINGS_SETTINGS
{
    ULONG MinimumLength;

    union
    {
        struct
        {
            ULONG Ansi : 1;
            ULONG Unicode : 1;
            ULONG ExtendedCharSet : 1;
            ULONG Private : 1;
            ULONG Image : 1;
            ULONG Mapped : 1;
            ULONG ZeroPadAddresses : 1;
            ULONG Spare : 25;
        };

        ULONG Flags;
    };
} PH_MEMSTRINGS_SETTINGS, *PPH_MEMSTRINGS_SETTINGS;

typedef struct _PH_MEMSTRINGS_CONTEXT
{
    PPH_PROCESS_ITEM ProcessItem;
    HANDLE ProcessHandle;
    BOOLEAN UseClone;
    HANDLE CloneHandle;

    HWND WindowHandle;
    HWND TreeNewHandle;
    HWND MessageHandle;
    HWND SearchHandle;
    HWND FilterHandle;
    RECT MinimumSize;
    ULONG_PTR SearchMatchHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PH_TN_FILTER_SUPPORT FilterSupport;
    PH_CM_MANAGER Cm;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    ULONG StringsCount;
    PPH_LIST NodeList;
    BOOLEAN BackOffActive;
    ULONG BackOffSearchMatchRequests;
    ULONG BackOffSearchMatchChecked;

    ULONG State;
    BOOLEAN StopSearch;
    HANDLE SearchThreadHandle;
    ULONG SearchResultsAddIndex;
    PPH_LIST SearchResults;
    PH_QUEUED_LOCK SearchResultsLock;
    PPH_LIST PrevNodeList;

    PH_MEMSTRINGS_SETTINGS Settings;

    ULONG ThreadCount;              // 0 = auto (CPU count), 1..N = fixed
    struct _PH_MEMSTRINGS_NODE_SLAB* SlabHead;
    PH_QUEUED_LOCK SlabLock;
    PPH_LIST StringArenaList;
    PH_QUEUED_LOCK StringArenaLock;
} PH_MEMSTRINGS_CONTEXT, *PPH_MEMSTRINGS_CONTEXT;

typedef enum _PH_MEMSTRINGS_TREE_COLUMN_ITEM
{
    PH_MEMSTRINGS_TREE_COLUMN_ITEM_INDEX,
    PH_MEMSTRINGS_TREE_COLUMN_ITEM_BASE_ADDRESS,
    PH_MEMSTRINGS_TREE_COLUMN_ITEM_ADDRESS,
    PH_MEMSTRINGS_TREE_COLUMN_ITEM_TYPE,
    PH_MEMSTRINGS_TREE_COLUMN_ITEM_LENGTH,
    PH_MEMSTRINGS_TREE_COLUMN_ITEM_STRING,
    PH_MEMSTRINGS_TREE_COLUMN_ITEM_PROTECTION,
    PH_MEMSTRINGS_TREE_COLUMN_ITEM_MEMORY_TYPE,
    PH_MEMSTRINGS_TREE_COLUMN_ITEM_MAXIMUM
} PH_MEMSTRINGS_TREE_COLUMN_ITEM;

typedef struct _PH_MEMSTRINGS_NODE
{
    PH_TREENEW_NODE Node;

    ULONG Index;
    BOOLEAN Unicode;
    PVOID BaseAddress;
    PVOID Address;
    PH_STRINGREF StringRef;
    ULONG Protection;
    ULONG MemoryType;

    PPH_STRING IndexText;
    PPH_STRING BaseAddressText;
    PPH_STRING AddressText;
    PPH_STRING LengthText;
    PPH_STRING ProtectionText;
    // No TextCache array: PPH_STRING fields serve as the per-node cache.
    // GetCellText does not set TN_CACHE so TreeNew calls us per visible cell,
    // but each call is a pointer lookup once the string is allocated.
} PH_MEMSTRINGS_NODE, *PPH_MEMSTRINGS_NODE;

#define MEMSTRINGS_SLAB_CAPACITY 256

typedef struct _PH_MEMSTRINGS_NODE_SLAB
{
    struct _PH_MEMSTRINGS_NODE_SLAB* Next;
    ULONG Used;
    PH_MEMSTRINGS_NODE Nodes[MEMSTRINGS_SLAB_CAPACITY];
} PH_MEMSTRINGS_NODE_SLAB, *PPH_MEMSTRINGS_NODE_SLAB;

typedef struct _PH_MEMSTRINGS_SEARCH_CONTEXT
{
    PPH_MEMSTRINGS_CONTEXT TreeContext;
    ULONG TypeMask;
    MEMORY_BASIC_INFORMATION BasicInfo;
    PVOID CurrentReadAddress;
    PVOID NextReadAddress;
    SIZE_T ReadRemaning;
    PBYTE Buffer;
    SIZE_T BufferSize;
    PMEMORY_BASIC_INFORMATION RegionArray;
    ULONG RegionStart;
    ULONG RegionEnd;
    ULONG CurrentRegionIndex;
    PPH_MEMSTRINGS_NODE LocalBatch[1024];
    ULONG LocalBatchCount;
    PPH_MEMSTRINGS_NODE_SLAB CurrentSlab;
    PWCHAR StringBuffer;
    SIZE_T StringBufferUsed;
    SIZE_T StringBufferCapacity;
} PH_MEMSTRINGS_SEARCH_CONTEXT, *PPH_MEMSTRINGS_SEARCH_CONTEXT;

typedef struct _PH_MEMSTRINGS_WORKER_CONTEXT
{
    PPH_MEMSTRINGS_CONTEXT TreeContext;
    ULONG TypeMask;
    PMEMORY_BASIC_INFORMATION RegionArray;
    ULONG RegionStart;
    ULONG RegionEnd;
} PH_MEMSTRINGS_WORKER_CONTEXT, *PPH_MEMSTRINGS_WORKER_CONTEXT;

BOOLEAN PhpShowMemoryStringTreeDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_opt_ PPH_LIST PrevNodeList,
    _In_ BOOLEAN UseClone
    );

VOID PhpShowMemoryEditor(
    PPH_MEMSTRINGS_CONTEXT context,
    PPH_MEMSTRINGS_NODE node
    );

_Function_class_(PH_STRING_SEARCH_NEXT_BUFFER)
_Must_inspect_result_
NTSTATUS NTAPI PhpMemoryStringSearchTreeNextBuffer(
    _Inout_bytecount_(*Length) PVOID* Buffer,
    _Out_ PSIZE_T Length,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    HANDLE handle;
    PPH_MEMSTRINGS_SEARCH_CONTEXT context;

    assert(Context);

    context = Context;

    if (context->TreeContext->UseClone)
        handle = context->TreeContext->CloneHandle;
    else
        handle = context->TreeContext->ProcessHandle;

    *Buffer = NULL;
    *Length = 0;

    if (context->ReadRemaning)
        goto ReadMemory;

    while (NT_SUCCESS(status = NtQueryVirtualMemory(
        handle,
        PTR_ADD_OFFSET(context->BasicInfo.BaseAddress, context->BasicInfo.RegionSize),
        MemoryBasicInformation,
        &context->BasicInfo,
        sizeof(MEMORY_BASIC_INFORMATION),
        NULL
        )))
    {
        SIZE_T length;

        if (context->TreeContext->StopSearch)
            break;
        if (context->BasicInfo.State != MEM_COMMIT)
            continue;
        if (FlagOn(context->BasicInfo.Protect, PAGE_NOACCESS | PAGE_GUARD))
            continue;
        if (!FlagOn(context->BasicInfo.Type, context->TypeMask))
            continue;

        context->NextReadAddress = context->BasicInfo.BaseAddress;
        context->ReadRemaning = context->BasicInfo.RegionSize;

        // Don't allocate a huge buffer (16 MiB max)
        length = min(context->BasicInfo.RegionSize, 16 * 1024 * 1024);

        if (length > context->BufferSize)
        {
            context->Buffer = PhReAllocate(context->Buffer, length);
            context->BufferSize = length;
        }

        if (context->ReadRemaning)
        {
ReadMemory:
            context->CurrentReadAddress = context->NextReadAddress;
            length = min(context->ReadRemaning, context->BufferSize);

            assert(context->Buffer);

            if (NT_SUCCESS(status = PhReadVirtualMemory(
                handle,
                context->CurrentReadAddress,
                context->Buffer,
                length,
                &length
                )))
            {
                *Buffer = context->Buffer;
                *Length = length;
                context->ReadRemaning -= length;
                context->NextReadAddress = PTR_ADD_OFFSET(context->NextReadAddress, length);
                break;
            }
        }

        context->NextReadAddress = NULL;
        context->ReadRemaning = 0;
    }

    return status;
}

// Forward declarations for helpers defined below
static PWCHAR PhpAllocateStringData(_In_ PPH_MEMSTRINGS_SEARCH_CONTEXT Context, _In_ PWCHAR Source, _In_ SIZE_T CharCount);
static PPH_MEMSTRINGS_NODE PhpAllocateStringNode(_In_ PPH_MEMSTRINGS_SEARCH_CONTEXT Context);
static VOID PhpFlushLocalStringsBatch(_In_ PPH_MEMSTRINGS_SEARCH_CONTEXT Context);


_Function_class_(PH_STRING_SEARCH_CALLBACK)
BOOLEAN NTAPI PhpMemoryStringSearchTreeCallback(
    _In_ PPH_STRING_SEARCH_RESULT Result,
    _In_opt_ PVOID Context
    )
{
    PPH_MEMSTRINGS_SEARCH_CONTEXT context;
    PPH_MEMSTRINGS_CONTEXT treeContext;
    PPH_MEMSTRINGS_NODE node;

    assert(Context);

    context = Context;
    treeContext = context->TreeContext;

    InterlockedIncrement((volatile LONG*)&treeContext->StringsCount);

    node = PhpAllocateStringNode(context);

    node->Index = treeContext->StringsCount;
    node->Unicode = Result->Unicode;
    node->BaseAddress = context->BasicInfo.BaseAddress;
    node->Address = PTR_ADD_OFFSET(context->CurrentReadAddress, PTR_SUB_OFFSET(Result->Address, context->Buffer));
    node->Protection = context->BasicInfo.Protect;
    node->MemoryType = context->BasicInfo.Type;

    {
        SIZE_T charCount = Result->String.Length / sizeof(WCHAR);
        PWCHAR dest = PhpAllocateStringData(context, Result->String.Buffer, charCount);
        PhInitializeStringRefLongHint(&node->StringRef, dest);
    }

    context->LocalBatch[context->LocalBatchCount++] = node;
    if (context->LocalBatchCount >= RTL_NUMBER_OF(context->LocalBatch))
        PhpFlushLocalStringsBatch(context);

    return !!treeContext->StopSearch;
}

#define MEMSTRINGS_STRING_BUFFER_CHARS (8 * 1024 * 1024)

static PWCHAR PhpAllocateStringData(
    _In_ PPH_MEMSTRINGS_SEARCH_CONTEXT Context,
    _In_ PWCHAR Source,
    _In_ SIZE_T CharCount
    )
{
    PPH_MEMSTRINGS_CONTEXT treeContext = Context->TreeContext;
    PWCHAR dest;

    if (!Context->StringBuffer || Context->StringBufferUsed + CharCount + 1 > Context->StringBufferCapacity)
    {
        PWCHAR newBuffer = PhAllocate(MEMSTRINGS_STRING_BUFFER_CHARS * sizeof(WCHAR));

        PhAcquireQueuedLockExclusive(&treeContext->StringArenaLock);
        PhAddItemList(treeContext->StringArenaList, newBuffer);
        PhReleaseQueuedLockExclusive(&treeContext->StringArenaLock);

        Context->StringBuffer = newBuffer;
        Context->StringBufferUsed = 0;
        Context->StringBufferCapacity = MEMSTRINGS_STRING_BUFFER_CHARS;
    }

    dest = Context->StringBuffer + Context->StringBufferUsed;
    memcpy(dest, Source, CharCount * sizeof(WCHAR));
    dest[CharCount] = 0;
    Context->StringBufferUsed += CharCount + 1;

    return dest;
}

static PPH_MEMSTRINGS_NODE PhpAllocateStringNode(
    _In_ PPH_MEMSTRINGS_SEARCH_CONTEXT Context
    )
{
    PPH_MEMSTRINGS_NODE_SLAB slab = Context->CurrentSlab;

    if (!slab || slab->Used >= MEMSTRINGS_SLAB_CAPACITY)
    {
        slab = PhAllocateZero(sizeof(PH_MEMSTRINGS_NODE_SLAB));

        PhAcquireQueuedLockExclusive(&Context->TreeContext->SlabLock);
        slab->Next = Context->TreeContext->SlabHead;
        Context->TreeContext->SlabHead = slab;
        PhReleaseQueuedLockExclusive(&Context->TreeContext->SlabLock);

        Context->CurrentSlab = slab;
    }

    return &slab->Nodes[slab->Used++];
}

static VOID PhpFlushLocalStringsBatch(
    _In_ PPH_MEMSTRINGS_SEARCH_CONTEXT Context
    )
{
    PPH_MEMSTRINGS_CONTEXT treeContext = Context->TreeContext;

    PhAcquireQueuedLockExclusive(&treeContext->SearchResultsLock);
    for (ULONG i = 0; i < Context->LocalBatchCount; i++)
        PhAddItemList(treeContext->SearchResults, Context->LocalBatch[i]);
    PhReleaseQueuedLockExclusive(&treeContext->SearchResultsLock);

    Context->LocalBatchCount = 0;
}

static VOID PhpFreeMemoryStringsSlabs(
    _In_ PPH_MEMSTRINGS_CONTEXT Context
    )
{
    PPH_MEMSTRINGS_NODE_SLAB slab = Context->SlabHead;
    while (slab)
    {
        PPH_MEMSTRINGS_NODE_SLAB next = slab->Next;
        PhFree(slab);
        slab = next;
    }
    Context->SlabHead = NULL;
}

static VOID PhpFreeMemoryStringsArenas(
    _In_ PPH_MEMSTRINGS_CONTEXT Context
    )
{
    if (!Context->StringArenaList)
        return;

    for (ULONG i = 0; i < Context->StringArenaList->Count; i++)
        PhFree(Context->StringArenaList->Items[i]);

    PhClearList(Context->StringArenaList);
}

_Function_class_(PH_STRING_SEARCH_NEXT_BUFFER)
_Must_inspect_result_
NTSTATUS NTAPI PhpMemoryStringSearchTreeNextBufferParallel(
    _Inout_bytecount_(*Length) PVOID* Buffer,
    _Out_ PSIZE_T Length,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    HANDLE handle;
    PPH_MEMSTRINGS_SEARCH_CONTEXT context;

    assert(Context);

    context = Context;

    if (context->TreeContext->UseClone)
        handle = context->TreeContext->CloneHandle;
    else
        handle = context->TreeContext->ProcessHandle;

    *Buffer = NULL;
    *Length = 0;

    for (;;)
    {
        if (context->ReadRemaning == 0)
        {
            if (context->CurrentRegionIndex >= context->RegionEnd)
                return STATUS_NO_MORE_ENTRIES;

            if (context->TreeContext->StopSearch)
                return STATUS_CANCELLED;

            context->BasicInfo = context->RegionArray[context->CurrentRegionIndex++];
            context->NextReadAddress = context->BasicInfo.BaseAddress;
            context->ReadRemaning = context->BasicInfo.RegionSize;

            if (context->ReadRemaning == 0)
                continue;

            {
                SIZE_T bufLength = min(context->ReadRemaning, 32 * 1024 * 1024);
                if (bufLength > context->BufferSize)
                {
                    context->Buffer = PhReAllocate(context->Buffer, bufLength);
                    context->BufferSize = bufLength;
                }
            }
        }

        context->CurrentReadAddress = context->NextReadAddress;
        {
            SIZE_T length = min(context->ReadRemaning, context->BufferSize);

            if (NT_SUCCESS(status = PhReadVirtualMemory(
                handle,
                context->CurrentReadAddress,
                context->Buffer,
                length,
                &length
                )))
            {
                *Buffer = context->Buffer;
                *Length = length;
                context->ReadRemaning -= length;
                context->NextReadAddress = PTR_ADD_OFFSET(context->NextReadAddress, length);
                return STATUS_SUCCESS;
            }
        }

        context->ReadRemaning = 0;
    }
}

_Function_class_(USER_THREAD_START_ROUTINE)
static NTSTATUS PhpMemoryStringsWorkerThread(
    _In_ PVOID Parameter
    )
{
    PPH_MEMSTRINGS_WORKER_CONTEXT worker = Parameter;
    PH_MEMSTRINGS_SEARCH_CONTEXT context;

    memset(&context, 0, sizeof(context));
    context.TreeContext = worker->TreeContext;
    context.TypeMask = worker->TypeMask;
    context.RegionArray = worker->RegionArray;
    context.RegionStart = worker->RegionStart;
    context.RegionEnd = worker->RegionEnd;
    context.CurrentRegionIndex = worker->RegionStart;

    PhSearchStrings(
        worker->TreeContext->Settings.MinimumLength,
        !!worker->TreeContext->Settings.ExtendedCharSet,
        PhpMemoryStringSearchTreeNextBufferParallel,
        PhpMemoryStringSearchTreeCallback,
        &context
        );

    if (context.LocalBatchCount > 0)
        PhpFlushLocalStringsBatch(&context);

    if (context.Buffer)
        PhFree(context.Buffer);

    PhFree(worker);
    return STATUS_SUCCESS;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS PhpMemorySearchStringsThread(
    _In_ PPH_MEMSTRINGS_CONTEXT Context
    )
{
    HANDLE handle;
    ULONG typeMask;
    PMEMORY_BASIC_INFORMATION regionArray;
    ULONG regionCount;
    ULONG regionCapacity;


    handle = Context->UseClone ? Context->CloneHandle : Context->ProcessHandle;

    typeMask = 0;
    if (Context->Settings.Private)
        SetFlag(typeMask, MEM_PRIVATE);
    if (Context->Settings.Image)
        SetFlag(typeMask, MEM_IMAGE);
    if (Context->Settings.Mapped)
        SetFlag(typeMask, MEM_MAPPED);

    if (!typeMask)
        goto Finished;

    regionCapacity = 256;
    regionCount = 0;
    regionArray = PhAllocate(regionCapacity * sizeof(MEMORY_BASIC_INFORMATION));

    {
        PVOID address = NULL;
        MEMORY_BASIC_INFORMATION basicInfo;

        while (NT_SUCCESS(NtQueryVirtualMemory(
            handle,
            address,
            MemoryBasicInformation,
            &basicInfo,
            sizeof(MEMORY_BASIC_INFORMATION),
            NULL
            )))
        {
            if (Context->StopSearch)
                break;

            if (basicInfo.State == MEM_COMMIT &&
                !FlagOn(basicInfo.Protect, PAGE_NOACCESS | PAGE_GUARD) &&
                FlagOn(basicInfo.Type, typeMask))
            {
                if (regionCount >= regionCapacity)
                {
                    regionCapacity *= 2;
                    regionArray = PhReAllocate(regionArray, regionCapacity * sizeof(MEMORY_BASIC_INFORMATION));
                }
                regionArray[regionCount++] = basicInfo;
            }

            address = PTR_ADD_OFFSET(basicInfo.BaseAddress, basicInfo.RegionSize);
        }
    }

    if (regionCount > 0 && !Context->StopSearch)
    {
        ULONG threadCount;
        PHANDLE threadHandles;

        if (Context->ThreadCount > 0)
            threadCount = Context->ThreadCount;
        else
            threadCount = PhSystemBasicInformation.NumberOfProcessors;
        if (threadCount < 1) threadCount = 1;
        if (threadCount > 64) threadCount = 64;
        if (threadCount > regionCount) threadCount = regionCount;

        threadHandles = PhAllocate(threadCount * sizeof(HANDLE));

        for (ULONG t = 0; t < threadCount; t++)
        {
            PPH_MEMSTRINGS_WORKER_CONTEXT worker;

            worker = PhAllocateZero(sizeof(PH_MEMSTRINGS_WORKER_CONTEXT));
            worker->TreeContext = Context;
            worker->TypeMask = typeMask;
            worker->RegionArray = regionArray;
            worker->RegionStart = t * regionCount / threadCount;
            worker->RegionEnd = (t + 1) * regionCount / threadCount;

            threadHandles[t] = PhCreateThread(0, PhpMemoryStringsWorkerThread, worker);
        }

        NtWaitForMultipleObjects(threadCount, threadHandles, WaitAll, FALSE, NULL);

        for (ULONG t = 0; t < threadCount; t++)
            NtClose(threadHandles[t]);

        PhFree(threadHandles);
    }

    PhFree(regionArray);

Finished:
    if (!Context->StopSearch)
        PostMessage(Context->WindowHandle, WM_PH_MEMSEARCH_FINISHED, 0, 0);

    return STATUS_SUCCESS;
}

VOID PhpMemoryStringsCheckBackOff(
    _In_ PPH_MEMSTRINGS_CONTEXT Context
    )
{
    // Once we reach a very large number of strings back off the updates to the
    // list and searching else the user input and interface can lag.
    //
    // N.B. The updated timer effectively induces a (timer value * 2) on search
    // filtering updates. Logic elsewhere will check on the delay for the user
    // to stop typing. Then the next timer trigger the search will be updated
    // based on the current user input.
    if (Context->NodeList->Count >= 150000)
    {
        Context->BackOffActive = TRUE;
        PhSetTimer(Context->WindowHandle, PH_WINDOW_TIMER_DEFAULT, 250, NULL);
    }
    else if (Context->NodeList->Count >= 500000)
    {
        Context->BackOffActive = TRUE;
        PhSetTimer(Context->WindowHandle, PH_WINDOW_TIMER_DEFAULT, 400, NULL);
    }
}

VOID PhpAddPendingMemoryStringsNodes(
    _In_ PPH_MEMSTRINGS_CONTEXT Context
    )
{
    ULONG pendingStart;
    ULONG pendingEnd;
    ULONG pendingCount;
    BOOLEAN hasFilter;

    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    PhAcquireQueuedLockExclusive(&Context->SearchResultsLock);

    pendingStart = Context->SearchResultsAddIndex;
    pendingEnd = Context->SearchResults->Count;
    pendingCount = pendingEnd - pendingStart;

    if (pendingCount > 0)
    {
        PhResizeList(Context->NodeList, Context->NodeList->Count + pendingCount);

        hasFilter = Context->FilterSupport.NodeList && Context->SearchMatchHandle;

        for (ULONG i = pendingStart; i < pendingEnd; i++)
        {
            PPH_MEMSTRINGS_NODE entry = (PPH_MEMSTRINGS_NODE)Context->SearchResults->Items[i];

            entry->Node.Visible = TRUE;
            entry->Node.Expanded = TRUE;

            if (hasFilter)
                entry->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->FilterSupport, &entry->Node);
        }

        PhAddItemsList(
            Context->NodeList,
            &Context->SearchResults->Items[pendingStart],
            pendingCount
            );

        Context->SearchResultsAddIndex = pendingEnd;
    }

    PhReleaseQueuedLockExclusive(&Context->SearchResultsLock);

    PhpMemoryStringsCheckBackOff(Context);

    if (pendingCount > 0)
        TreeNew_NodesStructured(Context->TreeNewHandle);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);
}

VOID PhpLoadSettingsMemoryStrings(
    _In_ PPH_MEMSTRINGS_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhGetStringSetting(SETTING_MEM_STRINGS_TREE_LIST_COLUMNS);
    sortSettings = PhGetStringSetting(SETTING_MEM_STRINGS_TREE_LIST_SORT);
    Context->Settings.Flags = PhGetIntegerSetting(SETTING_MEM_STRINGS_TREE_LIST_FLAGS);
    Context->Settings.MinimumLength = PhGetIntegerSetting(SETTING_MEM_STRINGS_MINIMUM_LENGTH);

    PhCmLoadSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &settings->sr, &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PhpSaveSettingsMemoryStrings(
    _In_ PPH_MEMSTRINGS_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhCmSaveSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &sortSettings);

    PhSetIntegerSetting(SETTING_MEM_STRINGS_MINIMUM_LENGTH, Context->Settings.MinimumLength);
    PhSetIntegerSetting(SETTING_MEM_STRINGS_TREE_LIST_FLAGS, Context->Settings.Flags);
    PhSetStringSetting2(SETTING_MEM_STRINGS_TREE_LIST_COLUMNS, &settings->sr);
    PhSetStringSetting2(SETTING_MEM_STRINGS_TREE_LIST_SORT, &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PhpInvalidateMemoryStringsAddresses(
    _In_ PPH_MEMSTRINGS_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_MEMSTRINGS_NODE node = (PPH_MEMSTRINGS_NODE)Context->NodeList->Items[i];

        PhClearReference(&node->BaseAddressText);
        PhClearReference(&node->AddressText);
    }

    TreeNew_NodesStructured(Context->TreeNewHandle);
}

BOOLEAN PhpGetSelectedMemoryStringsNodes(
    _In_ PPH_MEMSTRINGS_CONTEXT Context,
    _Out_ PPH_MEMSTRINGS_NODE** Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_MEMSTRINGS_NODE node = (PPH_MEMSTRINGS_NODE)Context->NodeList->Items[i];

        if (node->Node.Selected)
            PhAddItemList(list, node);
    }

    if (list->Count)
    {
        *Nodes = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
        *NumberOfNodes = list->Count;

        PhDereferenceObject(list);
        return TRUE;
    }

    *Nodes = NULL;
    *NumberOfNodes = 0;

    PhDereferenceObject(list);
    return FALSE;
}

VOID PhpCopyFilteredMemoryStringsNodes(
    _In_ PPH_MEMSTRINGS_CONTEXT Context,
    _Out_ PPH_LIST* NodeList
    )
{
    PPH_LIST list = PhCreateList(10);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_MEMSTRINGS_NODE node = (PPH_MEMSTRINGS_NODE)Context->NodeList->Items[i];

        if (node->Node.Visible)
        {
            PPH_MEMSTRINGS_NODE cloned;

            cloned = PhAllocateCopy(node, sizeof(PH_MEMSTRINGS_NODE));
            // StringRef points into arena - no reference counting needed
            PhAddItemList(list, cloned);
        }
    }

    *NodeList = list;
}

_Function_class_(PH_TN_FILTER_FUNCTION)
BOOLEAN PhpMemoryStringsTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_MEMSTRINGS_CONTEXT context = Context;
    PPH_MEMSTRINGS_NODE node = (PPH_MEMSTRINGS_NODE)Node;

    assert(Context);

    if (!context->Settings.Ansi && !node->Unicode)
        return FALSE;
    if (!context->Settings.Unicode && node->Unicode)
        return FALSE;

    if (!context->SearchMatchHandle)
        return TRUE;

    return PhSearchControlMatch(context->SearchMatchHandle, &node->StringRef);
}

_Function_class_(PH_SEARCHCONTROL_CALLBACK)
VOID NTAPI PvpStringsSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PPH_MEMSTRINGS_CONTEXT context = Context;

    assert(context);

    context->SearchMatchHandle = MatchHandle;

    if (context->BackOffActive)
        context->BackOffSearchMatchRequests++;
    else
        PhApplyTreeNewFilters(&context->FilterSupport);
}

VOID PhpDeleteMemoryStringsNodeList(
    _In_ PPH_LIST NodeList
    )
{
    // Free lazily-allocated display strings (non-NULL only for visible nodes).
    // Node memory itself is freed by PhpFreeMemoryStringsSlabs.
    for (ULONG i = 0; i < NodeList->Count; i++)
    {
        PPH_MEMSTRINGS_NODE node = (PPH_MEMSTRINGS_NODE)NodeList->Items[i];
        if (node->IndexText)       PhDereferenceObject(node->IndexText);
        if (node->BaseAddressText) PhDereferenceObject(node->BaseAddressText);
        if (node->AddressText)     PhDereferenceObject(node->AddressText);
        if (node->LengthText)      PhDereferenceObject(node->LengthText);
        if (node->ProtectionText)  PhDereferenceObject(node->ProtectionText);
    }
}

VOID PhpDeleteMemoryStringsTree(
    _In_ PPH_MEMSTRINGS_CONTEXT Context
    )
{
    Context->StopSearch = TRUE;
    if (Context->SearchThreadHandle)
    {
        NtWaitForSingleObject(Context->SearchThreadHandle, FALSE, NULL);
        NtClose(Context->SearchThreadHandle);
        Context->SearchThreadHandle = NULL;
    }
    Context->StopSearch = FALSE;

    if (Context->CloneHandle)
    {
        NtTerminateProcess(Context->CloneHandle, STATUS_PROCESS_CLONED);
        NtClose(Context->CloneHandle);
        Context->CloneHandle = NULL;
    }

    PhpAddPendingMemoryStringsNodes(Context);

    PhpDeleteMemoryStringsNodeList(Context->NodeList);
    PhpFreeMemoryStringsSlabs(Context);
    PhpFreeMemoryStringsArenas(Context);
    PhClearReference(&Context->StringArenaList);

    PhDeleteTreeNewFilterSupport(&Context->FilterSupport);

    PhClearReference(&Context->NodeList);
    PhClearReference(&Context->SearchResults);
    Context->SearchResultsAddIndex = 0;
    Context->StringsCount = 0;
}

VOID PhpSearchMemoryStrings(
    _In_ PPH_MEMSTRINGS_CONTEXT Context
    )
{
    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    PhpDeleteMemoryStringsTree(Context);

    Context->NodeList = PhCreateList(100);
    Context->SearchResults = PhCreateList(100);

    PhInitializeTreeNewFilterSupport(&Context->FilterSupport, Context->TreeNewHandle, Context->NodeList);
    PhAddTreeNewFilter(&Context->FilterSupport, PhpMemoryStringsTreeFilterCallback, Context);

    TreeNew_SetEmptyText(Context->TreeNewHandle, &LoadingStringsText, 0);
    TreeNew_NodesStructured(Context->TreeNewHandle);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);

    if (Context->UseClone)
    {
        NTSTATUS status;

        if (!NT_SUCCESS(status = PhCreateProcessClone(
            &Context->CloneHandle,
            Context->ProcessItem->ProcessId
            )))
        {
            PhShowStatus(Context->WindowHandle, L"Unable to clone the process", status, 0);
            return;
        }
    }

    Context->State = PH_MEMSEARCH_STATE_SEARCHING;
    EnableWindow(Context->FilterHandle, FALSE);

    PhCreateThreadEx(&Context->SearchThreadHandle, PhpMemorySearchStringsThread, Context);
}

#define SORT_FUNCTION(Column) PvpStringsTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PvpStringsTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPH_MEMSTRINGS_NODE node1 = *(PPH_MEMSTRINGS_NODE *)_elem1; \
    PPH_MEMSTRINGS_NODE node2 = *(PPH_MEMSTRINGS_NODE *)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintcmp(node1->Index, node2->Index); \
    \
    return PhModifySort(sortResult, ((PPH_MEMSTRINGS_CONTEXT)_context)->TreeNewSortOrder); \
}

_Function_class_(PH_CM_POST_SORT_FUNCTION)
LONG PhpMemoryStringsTreeNewPostSortFunction(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    )
{
    if (Result == 0)
        Result = uintptrcmp((ULONG_PTR)((PPH_MEMSTRINGS_NODE)Node1)->Index, (ULONG_PTR)((PPH_MEMSTRINGS_NODE)Node2)->Index);

    return PhModifySort(Result, SortOrder);
}

BEGIN_SORT_FUNCTION(Index)
{
    NOTHING;
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(BaseAddress)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->BaseAddress, (ULONG_PTR)node2->BaseAddress);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Address)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->Address, (ULONG_PTR)node2->Address);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Type)
{
    sortResult = uintcmp(node1->Unicode, node2->Unicode);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Length)
{
    sortResult = uintptrcmp(node1->StringRef.Length, node2->StringRef.Length);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(String)
{
    sortResult = PhCompareStringRef(&node1->StringRef, &node2->StringRef, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Protection)
{
    sortResult = uintcmp(node1->Protection, node2->Protection);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(MemoryType)
{
    sortResult = uintcmp(node1->MemoryType, node2->MemoryType);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PhpMemoryStringsTreeNewCallback(
    _In_ HWND WindowHandle,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PPH_MEMSTRINGS_CONTEXT context = Context;
    PPH_MEMSTRINGS_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PPH_MEMSTRINGS_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static CONST _CoreCrtSecureSearchSortCompareFunction sortFunctions[] =
                {
                    SORT_FUNCTION(Index),
                    SORT_FUNCTION(BaseAddress),
                    SORT_FUNCTION(Address),
                    SORT_FUNCTION(Type),
                    SORT_FUNCTION(Length),
                    SORT_FUNCTION(String),
                    SORT_FUNCTION(Protection),
                    SORT_FUNCTION(MemoryType),
                };
                _CoreCrtSecureSearchSortCompareFunction sortFunction;

                static_assert(RTL_NUMBER_OF(sortFunctions) == PH_MEMSTRINGS_TREE_COLUMN_ITEM_MAXIMUM, "SortFunctions must equal maximum.");

                if (context->TreeNewSortColumn < PH_MEMSTRINGS_TREE_COLUMN_ITEM_MAXIMUM)
                    sortFunction = sortFunctions[context->TreeNewSortColumn];
                else
                    sortFunction = NULL;

                // Nodes are added in index order; skip the sort when column is Index ascending.
                BOOLEAN skipSort =
                    sortFunction &&
                    context->TreeNewSortColumn == PH_MEMSTRINGS_TREE_COLUMN_ITEM_INDEX &&
                    context->TreeNewSortOrder != DescendingSortOrder;

                if (sortFunction && !skipSort)
                {
                    qsort_s(context->NodeList->Items, context->NodeList->Count, sizeof(PVOID), sortFunction, context);
                }

                getChildren->Children = (PPH_TREENEW_NODE *)context->NodeList->Items;
                getChildren->NumberOfChildren = context->NodeList->Count;
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = (PPH_TREENEW_IS_LEAF)Parameter1;
            node = (PPH_MEMSTRINGS_NODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;
            node = (PPH_MEMSTRINGS_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PH_MEMSTRINGS_TREE_COLUMN_ITEM_INDEX:
                if (!node->IndexText)
                {
                    PH_FORMAT format[1];
                    PhInitFormatU(&format[0], node->Index);
                    node->IndexText = PhFormat(format, 1, 16);
                }
                getCellText->Text = node->IndexText->sr;
                break;
            case PH_MEMSTRINGS_TREE_COLUMN_ITEM_BASE_ADDRESS:
                if (!node->BaseAddressText)
                {
                    WCHAR buf[PH_PTR_STR_LEN_1];
                    if (context->Settings.ZeroPadAddresses)
                        PhPrintPointerPadZeros(buf, node->BaseAddress);
                    else
                        PhPrintPointer(buf, node->BaseAddress);
                    node->BaseAddressText = PhCreateString(buf);
                }
                getCellText->Text = node->BaseAddressText->sr;
                break;
            case PH_MEMSTRINGS_TREE_COLUMN_ITEM_ADDRESS:
                if (!node->AddressText)
                {
                    WCHAR buf[PH_PTR_STR_LEN_1];
                    if (context->Settings.ZeroPadAddresses)
                        PhPrintPointerPadZeros(buf, node->Address);
                    else
                        PhPrintPointer(buf, node->Address);
                    node->AddressText = PhCreateString(buf);
                }
                getCellText->Text = node->AddressText->sr;
                break;
            case PH_MEMSTRINGS_TREE_COLUMN_ITEM_PROTECTION:
                if (!node->ProtectionText)
                {
                    WCHAR buf[17];
                    PhGetMemoryProtectionString(node->Protection, buf);
                    node->ProtectionText = PhCreateString(buf);
                }
                getCellText->Text = node->ProtectionText->sr;
                break;
            case PH_MEMSTRINGS_TREE_COLUMN_ITEM_MEMORY_TYPE:
                getCellText->Text = *PhGetMemoryTypeString(node->MemoryType);
                break;
            case PH_MEMSTRINGS_TREE_COLUMN_ITEM_TYPE:
                PhInitializeStringRef(&getCellText->Text, node->Unicode ? L"Unicode" : L"ANSI");
                break;
            case PH_MEMSTRINGS_TREE_COLUMN_ITEM_LENGTH:
                if (!node->LengthText)
                {
                    PH_FORMAT format[1];
                    PhInitFormatU(&format[0], (ULONG)(node->StringRef.Length / sizeof(WCHAR)));
                    node->LengthText = PhFormat(format, 1, 16);
                }
                getCellText->Text = node->LengthText->sr;
                break;
            case PH_MEMSTRINGS_TREE_COLUMN_ITEM_STRING:
                getCellText->Text = node->StringRef;
                break;
            default:
                return FALSE;
            }

            getCellText->Flags = 0;
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = (PPH_TREENEW_GET_NODE_COLOR)Parameter1;
            node = (PPH_MEMSTRINGS_NODE)getNodeColor->Node;

            getNodeColor->Flags = TN_AUTO_FORECOLOR;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            PPH_TREENEW_SORT_CHANGED_EVENT sorting = Parameter1;

            context->TreeNewSortColumn = sorting->SortColumn;
            context->TreeNewSortOrder = sorting->SortOrder;

            TreeNew_NodesStructured(WindowHandle);
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                {
                    if (GetKeyState(VK_CONTROL) < 0)
                    {
                        PPH_STRING text;

                        text = PhGetTreeNewText(WindowHandle, 0);
                        PhSetClipboardString(WindowHandle, &text->sr);
                        PhDereferenceObject(text);
                    }
                }
                break;
            case 'A':
                if (GetKeyState(VK_CONTROL) < 0)
                    TreeNew_SelectRange(context->TreeNewHandle, 0, -1);
                break;
            }
        }
        return TRUE;
    case TreeNewNodeExpanding:
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            PPH_TREENEW_MOUSE_EVENT mouseEvent = Parameter1;
            node = (PPH_MEMSTRINGS_NODE)mouseEvent->Node;

            PhpShowMemoryEditor(context, node);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenu = Parameter1;

            SendMessage(context->WindowHandle, WM_PH_MEMSEARCH_SHOWMENU, 0, (LPARAM)contextMenu);
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = WindowHandle;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = 0;
            data.DefaultSortOrder = AscendingSortOrder;
            PhInitializeTreeNewColumnMenuEx(&data, PH_TN_COLUMN_MENU_SHOW_RESET_SORT);

            data.Selection = PhShowEMenu(data.Menu, WindowHandle, PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    }

    return FALSE;
}

VOID PhpInitializeMemoryStringsTree(
    _In_ PPH_MEMSTRINGS_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HWND TreeNewHandle
    )
{
    BOOLEAN enableMonospaceFont = !!PhGetIntegerSetting(SETTING_ENABLE_MONOSPACE_FONT);

    Context->WindowHandle = WindowHandle;
    Context->TreeNewHandle = TreeNewHandle;

    Context->NodeList = PhCreateList(1);
    Context->SearchResults = PhCreateList(1);

    PhSetControlTheme(TreeNewHandle, L"explorer");

    TreeNew_SetCallback(TreeNewHandle, PhpMemoryStringsTreeNewCallback, Context);
    TreeNew_SetRedraw(TreeNewHandle, FALSE);

    PhAddTreeNewColumnEx2(TreeNewHandle, PH_MEMSTRINGS_TREE_COLUMN_ITEM_INDEX, TRUE, L"#", 40, PH_ALIGN_LEFT, PH_MEMSTRINGS_TREE_COLUMN_ITEM_INDEX, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PH_MEMSTRINGS_TREE_COLUMN_ITEM_BASE_ADDRESS, TRUE, L"Base address", 80, PH_ALIGN_LEFT | (enableMonospaceFont ? PH_ALIGN_MONOSPACE_FONT : 0), PH_MEMSTRINGS_TREE_COLUMN_ITEM_BASE_ADDRESS, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PH_MEMSTRINGS_TREE_COLUMN_ITEM_ADDRESS, TRUE, L"Address", 80, PH_ALIGN_LEFT | (enableMonospaceFont ? PH_ALIGN_MONOSPACE_FONT : 0), PH_MEMSTRINGS_TREE_COLUMN_ITEM_ADDRESS, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PH_MEMSTRINGS_TREE_COLUMN_ITEM_TYPE, TRUE, L"Type", 80, PH_ALIGN_LEFT, PH_MEMSTRINGS_TREE_COLUMN_ITEM_TYPE, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PH_MEMSTRINGS_TREE_COLUMN_ITEM_LENGTH, TRUE, L"Length", 80, PH_ALIGN_LEFT, PH_MEMSTRINGS_TREE_COLUMN_ITEM_LENGTH, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PH_MEMSTRINGS_TREE_COLUMN_ITEM_STRING, TRUE, L"String", 600, PH_ALIGN_LEFT, PH_MEMSTRINGS_TREE_COLUMN_ITEM_STRING, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PH_MEMSTRINGS_TREE_COLUMN_ITEM_PROTECTION, FALSE, L"Protection", 80, PH_ALIGN_LEFT, PH_MEMSTRINGS_TREE_COLUMN_ITEM_PROTECTION, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PH_MEMSTRINGS_TREE_COLUMN_ITEM_MEMORY_TYPE, FALSE, L"Memory type", 80, PH_ALIGN_LEFT, PH_MEMSTRINGS_TREE_COLUMN_ITEM_MEMORY_TYPE, 0, 0);

    TreeNew_SetRedraw(TreeNewHandle, TRUE);
    TreeNew_SetSort(TreeNewHandle, PH_MEMSTRINGS_TREE_COLUMN_ITEM_INDEX, AscendingSortOrder);

    PhCmInitializeManager(&Context->Cm, TreeNewHandle, PH_MEMSTRINGS_TREE_COLUMN_ITEM_MAXIMUM, PhpMemoryStringsTreeNewPostSortFunction);

    Context->StringArenaList = PhCreateList(16);

    PhpLoadSettingsMemoryStrings(Context);
}

INT_PTR CALLBACK PhpMemoryStringsMinimumLengthDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    PULONG length;

    if (uMsg == WM_INITDIALOG)
    {
        length = (PULONG)lParam;

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, length);
    }
    else
    {
        length = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!length)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            WCHAR lengthString[PH_INT32_STR_LEN_1];

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhPrintUInt32(lengthString, *length);

            PhSetDialogItemText(hwndDlg, IDC_MINIMUMLENGTH, lengthString);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    ULONG64 minimumLength;

                    PhStringToInteger64(&PhaGetDlgItemText(hwndDlg, IDC_MINIMUMLENGTH)->sr, 0, &minimumLength);

                    if (!minimumLength || minimumLength > MAXULONG32)
                    {
                        PhShowError2(hwndDlg, L"Unable to update the length.", L"%s", L"The minimum length is invalid.");
                        break;
                    }

                    *length = (ULONG)minimumLength;
                    EndDialog(hwndDlg, IDOK);
                }
                break;
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

INT_PTR CALLBACK PhpMemoryStringsThreadCountDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PULONG threadCount;

    if (uMsg == WM_INITDIALOG)
    {
        threadCount = (PULONG)lParam;
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, threadCount);
    }
    else
    {
        threadCount = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!threadCount)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            WCHAR countString[PH_INT32_STR_LEN_1];
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));
            SetWindowText(hwndDlg, L"Value (0 = auto / unlimited)");
            PhPrintUInt32(countString, *threadCount);
            PhSetDialogItemText(hwndDlg, IDC_MINIMUMLENGTH, countString);
            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        break;
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDCANCEL:
            EndDialog(hwndDlg, IDCANCEL);
            break;
        case IDOK:
            {
                ULONG64 count;
                PhStringToInteger64(&PhaGetDlgItemText(hwndDlg, IDC_MINIMUMLENGTH)->sr, 0, &count);
                if (count > 64)
                {
                    PhShowError2(hwndDlg, L"Invalid value.", L"%s", L"Enter a value between 0 and 64.");
                    break;
                }
                *threadCount = (ULONG)count;
                EndDialog(hwndDlg, IDOK);
            }
            break;
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }
    return FALSE;
}

ULONG PhpMemoryStringsValueDialog(
    _In_ HWND WindowHandle,
    _In_ ULONG Current
    )
{
    ULONG value = Current;
    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_MEMSTRINGSMINLEN),
        WindowHandle,
        PhpMemoryStringsThreadCountDlgProc,
        &value
        );
    return value;
}

ULONG PhpMemoryStringsMinimumLengthDialog(
    _In_ HWND WindowHandle,
    _In_ ULONG CurrentMinimumLength
)
{
    ULONG length;

    length = CurrentMinimumLength;

    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_MEMSTRINGSMINLEN),
        WindowHandle,
        PhpMemoryStringsMinimumLengthDlgProc,
        &length
        );

    return length;
}

VOID PhpMemoryStringsSetWindowTitle(
    _In_ PPH_MEMSTRINGS_CONTEXT Context
    )
{
    HICON icon;
    PPH_STRING title;
    CLIENT_ID clientId;

    if (Context->ProcessItem->IconEntry && Context->ProcessItem->IconEntry->SmallIconIndex)
    {
        icon = PhGetImageListIcon(Context->ProcessItem->IconEntry->SmallIconIndex, FALSE);
        if (icon)
            PhSetWindowIcon(Context->WindowHandle, icon, NULL, TRUE);
    }

    clientId.UniqueProcess = Context->ProcessItem->ProcessId;
    clientId.UniqueThread = NULL;
    title = PhaConcatStrings2(PhGetStringOrEmpty(PH_AUTO(PhGetClientIdName(&clientId))), L" Strings");
    SetWindowTextW(Context->WindowHandle, title->Buffer);
}

VOID PhpShowMemoryEditor(
    PPH_MEMSTRINGS_CONTEXT context,
    PPH_MEMSTRINGS_NODE node
    )
{
    NTSTATUS status;
    MEMORY_BASIC_INFORMATION basicInfo;
    PPH_SHOW_MEMORY_EDITOR showMemoryEditor;
    PVOID address;
    SIZE_T length;

    address = node->Address;
    if (node->Unicode)
        length = node->StringRef.Length;
    else
        length = node->StringRef.Length / 2;

    if (NT_SUCCESS(status = NtQueryVirtualMemory(
        context->ProcessHandle,
        address,
        MemoryBasicInformation,
        &basicInfo,
        sizeof(basicInfo),
        NULL
        )))
    {
        showMemoryEditor = PhAllocateZero(sizeof(PH_SHOW_MEMORY_EDITOR));
        showMemoryEditor->ProcessId = context->ProcessItem->ProcessId;
        showMemoryEditor->BaseAddress = basicInfo.BaseAddress;
        showMemoryEditor->RegionSize = basicInfo.RegionSize;
        showMemoryEditor->SelectOffset = (ULONG)((ULONG_PTR)address - (ULONG_PTR)basicInfo.BaseAddress);
        showMemoryEditor->SelectLength = (ULONG)length;

        SystemInformer_ShowMemoryEditor(showMemoryEditor);
    }
    else
    {
        PhShowStatus(context->WindowHandle, L"Unable to edit memory", status, 0);
    }
}

INT_PTR CALLBACK PhpMemoryStringsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_MEMSTRINGS_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_MEMSTRINGS_CONTEXT)lParam;

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->MessageHandle = GetDlgItem(hwndDlg, IDC_MESSAGE);
            context->SearchHandle = GetDlgItem(hwndDlg, IDC_SEARCH);
            context->FilterHandle = GetDlgItem(hwndDlg, IDC_FILTER);

            PhpMemoryStringsSetWindowTitle(context);
            PhRegisterDialog(hwndDlg);

            PhCreateSearchControl(
                hwndDlg,
                context->SearchHandle,
                L"Search Strings (Ctrl+K)",
                PvpStringsSearchControlCallback,
                context
                );

            PhpInitializeMemoryStringsTree(context, hwndDlg, GetDlgItem(hwndDlg, IDC_TREELIST));

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->SearchHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, context->MessageHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            context->MinimumSize.left = 0;
            context->MinimumSize.top = 0;
            context->MinimumSize.right = 300;
            context->MinimumSize.bottom = 100;
            MapDialogRect(hwndDlg, &context->MinimumSize);

            if (PhValidWindowPlacementFromSetting(SETTING_MEM_STRINGS_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_MEM_STRINGS_WINDOW_POSITION, SETTING_MEM_STRINGS_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, PhMainWndHandle);

            PhSetTimer(hwndDlg, PH_WINDOW_TIMER_DEFAULT, 200, NULL);

            if (context->PrevNodeList)
            {
                PhInitializeTreeNewFilterSupport(&context->FilterSupport, context->TreeNewHandle, context->NodeList);
                PhAddTreeNewFilter(&context->FilterSupport, PhpMemoryStringsTreeFilterCallback, context);

                PhMoveReference(&context->SearchResults, context->PrevNodeList);
                context->StringsCount = context->PrevNodeList->Count;
                context->State = PH_MEMSEARCH_STATE_FINISHED;
                EnableWindow(context->FilterHandle, FALSE);

                PhpMemoryStringsCheckBackOff(context);
            }
            else
            {
                PhpSearchMemoryStrings(context);
            }

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhpSaveSettingsMemoryStrings(context);
            PhpDeleteMemoryStringsTree(context);

            PhSaveWindowPlacementToSetting(SETTING_MEM_STRINGS_WINDOW_POSITION, SETTING_MEM_STRINGS_WINDOW_SIZE, hwndDlg);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            PhDereferenceObject(context->ProcessItem);
            NtClose(context->ProcessHandle);
            PhFree(context);

            PostQuitMessage(0);
        }
        break;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, context->MinimumSize.right, context->MinimumSize.bottom);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)context->TreeNewHandle);
                return TRUE;
            }
        }
        break;
    case WM_PH_MEMSEARCH_FINISHED:
        {
            PhpAddPendingMemoryStringsNodes(context);

            context->State = PH_MEMSEARCH_STATE_FINISHED;

            TreeNew_SetEmptyText(context->TreeNewHandle, &EmptyStringsText, 0);
            TreeNew_NodesStructured(context->TreeNewHandle);
        }
        break;
    case WM_TIMER:
        {
            if (context->State != PH_MEMSEARCH_STATE_STOPPED)
            {
                PPH_STRING message;
                PH_FORMAT format[3];
                ULONG count = 0;

                PhpAddPendingMemoryStringsNodes(context);

                if (context->State == PH_MEMSEARCH_STATE_SEARCHING)
                    PhInitFormatS(&format[count++], L"Searching... ");

                PhInitFormatU(&format[count++], context->StringsCount);
                PhInitFormatS(&format[count++], L" strings");

                if (context->State == PH_MEMSEARCH_STATE_FINISHED)
                {
                    context->State = PH_MEMSEARCH_STATE_STOPPED;
                    EnableWindow(context->FilterHandle, TRUE);
                }

                message = PhFormat(format, count, 80);
                SetWindowText(context->MessageHandle, message->Buffer);
                PhDereferenceObject(message);
            }

            if (context->BackOffActive && context->BackOffSearchMatchRequests)
            {
                if (context->BackOffSearchMatchChecked == context->BackOffSearchMatchRequests)
                {
                    context->BackOffSearchMatchRequests = 0;
                    context->BackOffSearchMatchChecked = 0;
                    PhApplyTreeNewFilters(&context->FilterSupport);
                }
                else
                {
                    // User still typing...
                    context->BackOffSearchMatchChecked = context->BackOffSearchMatchRequests;
                }
            }
        }
        break;
    case WM_PH_MEMSEARCH_SHOWMENU:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
            PPH_EMENU menu;
            PPH_EMENU_ITEM selectedItem;
            PPH_MEMSTRINGS_NODE* stringsNodes = NULL;
            ULONG numberOfNodes = 0;

            if (!PhpGetSelectedMemoryStringsNodes(context, &stringsNodes, &numberOfNodes))
                break;

            if (numberOfNodes != 0)
            {
                PPH_EMENU_ITEM readWrite;

                menu = PhCreateEMenu();

                readWrite = PhCreateEMenuItem(0, IDC_SHOW, L"Read/Write memory", NULL, NULL);
                readWrite->Flags |= PH_EMENU_DEFAULT;
                PhInsertEMenuItem(menu, readWrite, ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"Copy", NULL, NULL), ULONG_MAX);
                PhInsertCopyCellEMenuItem(menu, IDC_COPY, context->TreeNewHandle, contextMenuEvent->Column);

                if (numberOfNodes != 1)
                    readWrite->Flags |= PH_EMENU_DISABLED;

                selectedItem = PhShowEMenu(
                    menu,
                    hwndDlg,
                    PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                    PH_ALIGN_LEFT | PH_ALIGN_TOP,
                    contextMenuEvent->Location.x,
                    contextMenuEvent->Location.y
                    );

                if (selectedItem && selectedItem->Id != ULONG_MAX && !PhHandleCopyCellEMenuItem(selectedItem))
                {
                    switch (selectedItem->Id)
                    {
                    case IDC_SHOW:
                        {
                            assert(numberOfNodes == 1);

                            PhpShowMemoryEditor(context, stringsNodes[0]);
                        }
                        break;
                    case IDC_COPY:
                        {
                            PPH_STRING text;

                            text = PhGetTreeNewText(context->TreeNewHandle, 0);
                            PhSetClipboardString(context->TreeNewHandle, &text->sr);
                            PhDereferenceObject(text);
                        }
                        break;
                    }
                }

                PhDestroyEMenu(menu);
                PhFree(stringsNodes);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                DestroyWindow(hwndDlg);
                break;
            case IDC_SETTINGS:
                {
                    RECT rect;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM selectedItem;
                    PPH_EMENU_ITEM ansi;
                    PPH_EMENU_ITEM unicode;
                    PPH_EMENU_ITEM extendedUnicode;
                    PPH_EMENU_ITEM private;
                    PPH_EMENU_ITEM image;
                    PPH_EMENU_ITEM mapped;
                    PPH_EMENU_ITEM minimumLength;
                    PPH_EMENU_ITEM threadCount;
                    PPH_EMENU_ITEM zeroPad;
                    PPH_EMENU_ITEM refresh;
                    WCHAR threadCountLabel[64];

                    if (!PhGetWindowRect(GetDlgItem(hwndDlg, IDC_SETTINGS), &rect))
                        break;

                    ansi = PhCreateEMenuItem(0, 1, L"ANSI", NULL, NULL);
                    unicode = PhCreateEMenuItem(0, 2, L"Unicode", NULL, NULL);
                    extendedUnicode = PhCreateEMenuItem(0, 3, L"Extended character set", NULL, NULL);
                    private = PhCreateEMenuItem(0, 4, L"Private", NULL, NULL);
                    image = PhCreateEMenuItem(0, 5, L"Image", NULL, NULL);
                    mapped = PhCreateEMenuItem(0, 6, L"Mapped", NULL, NULL);
                    minimumLength = PhCreateEMenuItem(0, 7, L"Minimum length...", NULL, NULL);
                    zeroPad = PhCreateEMenuItem(0, 8, L"Zero pad addresses", NULL, NULL);
                    refresh = PhCreateEMenuItem(0, 9, L"Refresh\bF5", NULL, NULL);
                    if (context->ThreadCount == 0)
                        swprintf_s(threadCountLabel, RTL_NUMBER_OF(threadCountLabel), L"Thread count... (auto)");
                    else
                        swprintf_s(threadCountLabel, RTL_NUMBER_OF(threadCountLabel), L"Thread count... (%lu)", context->ThreadCount);
                    threadCount = PhCreateEMenuItem(0, 10, threadCountLabel, NULL, NULL);

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, ansi, ULONG_MAX);
                    PhInsertEMenuItem(menu, unicode, ULONG_MAX);
                    PhInsertEMenuItem(menu, extendedUnicode, ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, private, ULONG_MAX);
                    PhInsertEMenuItem(menu, image, ULONG_MAX);
                    PhInsertEMenuItem(menu, mapped, ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, minimumLength, ULONG_MAX);
                    PhInsertEMenuItem(menu, threadCount, ULONG_MAX);
                    PhInsertEMenuItem(menu, zeroPad, ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, refresh, ULONG_MAX);

                    if (context->Settings.Ansi)
                        ansi->Flags |= PH_EMENU_CHECKED;
                    if (context->Settings.Unicode)
                        unicode->Flags |= PH_EMENU_CHECKED;
                    if (context->Settings.ExtendedCharSet)
                        extendedUnicode->Flags |= PH_EMENU_CHECKED;
                    if (context->Settings.Private)
                        private->Flags |= PH_EMENU_CHECKED;
                    if (context->Settings.Image)
                        image->Flags |= PH_EMENU_CHECKED;
                    if (context->Settings.Mapped)
                        mapped->Flags |= PH_EMENU_CHECKED;
                    if (context->Settings.ZeroPadAddresses)
                        zeroPad->Flags |= PH_EMENU_CHECKED;

                    selectedItem = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        rect.left,
                        rect.bottom
                        );

                    if (selectedItem && selectedItem->Id != ULONG_MAX)
                    {
                        if (selectedItem == ansi)
                        {
                            context->Settings.Ansi = !context->Settings.Ansi;
                            PhpSaveSettingsMemoryStrings(context);
                            PhApplyTreeNewFilters(&context->FilterSupport);
                        }
                        else if (selectedItem == unicode)
                        {
                            context->Settings.Unicode = !context->Settings.Unicode;
                            PhpSaveSettingsMemoryStrings(context);
                            PhApplyTreeNewFilters(&context->FilterSupport);
                        }
                        else if (selectedItem == extendedUnicode)
                        {
                            context->Settings.ExtendedCharSet = !context->Settings.ExtendedCharSet;
                            PhpSaveSettingsMemoryStrings(context);
                            PhpSearchMemoryStrings(context);
                        }
                        else if (selectedItem == private)
                        {
                            context->Settings.Private = !context->Settings.Private;
                            PhpSaveSettingsMemoryStrings(context);
                            PhpSearchMemoryStrings(context);
                        }
                        else if (selectedItem == image)
                        {
                            context->Settings.Image = !context->Settings.Image;
                            PhpSaveSettingsMemoryStrings(context);
                            PhpSearchMemoryStrings(context);
                        }
                        else if (selectedItem == mapped)
                        {
                            context->Settings.Mapped = !context->Settings.Mapped;
                            PhpSaveSettingsMemoryStrings(context);
                            PhpSearchMemoryStrings(context);
                        }
                        else if (selectedItem == minimumLength)
                        {
                            ULONG length = PhpMemoryStringsMinimumLengthDialog(hwndDlg, context->Settings.MinimumLength);
                            if (length != context->Settings.MinimumLength)
                            {
                                context->Settings.MinimumLength = length;
                                PhpSaveSettingsMemoryStrings(context);
                                PhpSearchMemoryStrings(context);
                            }
                        }
                        else if (selectedItem == threadCount)
                        {
                            context->ThreadCount = PhpMemoryStringsValueDialog(hwndDlg, context->ThreadCount);
                        }
                        else if (selectedItem == zeroPad)
                        {
                            context->Settings.ZeroPadAddresses = !context->Settings.ZeroPadAddresses;
                            PhpSaveSettingsMemoryStrings(context);
                            PhpInvalidateMemoryStringsAddresses(context);
                        }
                        else if (selectedItem == refresh)
                        {
                            PhpSearchMemoryStrings(context);
                        }
                    }

                    PhDestroyEMenu(menu);
                }
                break;
            case IDC_FILTER:
                {
                    PPH_LIST nodeList;

                    PhpCopyFilteredMemoryStringsNodes(context, &nodeList);

                    if (!PhpShowMemoryStringTreeDialog(
                        hwndDlg,
                        context->ProcessItem,
                        nodeList,
                        context->UseClone
                        ))
                    {
                        PhpDeleteMemoryStringsNodeList(nodeList);
                        PhDereferenceObject(nodeList);
                    }
                }
                break;
            }
        }
        break;
    case WM_KEYDOWN:
        {
            switch (wParam)
            {
            case VK_F5:
                PhpSearchMemoryStrings(context);
                return TRUE;
            }
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetTextColor((HDC)wParam, RGB(0, 0, 0));
            SetDCBrushColor((HDC)wParam, RGB(255, 255, 255));
            return (INT_PTR)GetStockBrush(DC_BRUSH);
        }
        break;
    }

    return FALSE;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS NTAPI PhpShowMemoryStringDialogThreadStart(
    _In_ PVOID Parameter
    )
{
    HWND windowHandle;
    BOOL result;
    MSG message;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    windowHandle = PhCreateDialog(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_MEMSTRINGS),
        NULL,
        PhpMemoryStringsDlgProc,
        Parameter
        );

    ShowWindow(windowHandle, SW_SHOW);
    SetForegroundWindow(windowHandle);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == INT_ERROR)
            break;

        if (!IsDialogMessage(windowHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

BOOLEAN PhpShowMemoryStringTreeDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_opt_ PPH_LIST PrevNodeList,
    _In_ BOOLEAN UseClone
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    PPH_MEMSTRINGS_CONTEXT context;

    if (!NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        ProcessItem->ProcessId
        )))
    {
        PhShowStatus(ParentWindowHandle, L"Unable to open the process", status, 0);
        return FALSE;
    }

    context = PhAllocateZero(sizeof(PH_MEMSTRINGS_CONTEXT));
    context->ProcessItem = PhReferenceObject(ProcessItem);
    context->ProcessHandle = processHandle;
    context->UseClone = UseClone;
    context->PrevNodeList = PrevNodeList;

    if (!NT_SUCCESS(status = PhCreateThread2(PhpShowMemoryStringDialogThreadStart, context)))
    {
        PhDereferenceObject(context->ProcessItem);
        NtClose(context->ProcessHandle);
        PhFree(context);

        PhShowStatus(ParentWindowHandle, L"Unable to create the window.", status, 0);
        return FALSE;
    }

    return TRUE;
}

VOID PhShowMemoryStringTreeDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PhpShowMemoryStringTreeDialog(
        ParentWindowHandle,
        ProcessItem,
        NULL,
        ProcessItem->ProcessId == NtCurrentProcessId()
        );
}
