#ifndef PH_PHUISUP_H
#define PH_PHUISUP_H

// begin_phapppub
// Common state highlighting support

typedef struct _PH_SH_STATE
{
    PH_ITEM_STATE State;
    HANDLE StateListHandle;
    ULONG TickCount;
} PH_SH_STATE, *PPH_SH_STATE;
// end_phapppub

FORCEINLINE VOID PhChangeShStateTn(
    _Inout_ PPH_TREENEW_NODE Node,
    _Inout_ PPH_SH_STATE ShState,
    _Inout_ PPH_POINTER_LIST *StateList,
    _In_ PH_ITEM_STATE NewState,
    _In_ COLORREF NewTempBackColor,
    _In_opt_ HWND TreeNewHandleForUpdate
    )
{
    if (!*StateList)
        *StateList = PhCreatePointerList(4);

    if (ShState->State == NormalItemState)
        ShState->StateListHandle = PhAddItemPointerList(*StateList, Node);

    ShState->TickCount = GetTickCount();
    ShState->State = NewState;

    Node->UseTempBackColor = TRUE;
    Node->TempBackColor = NewTempBackColor;

    if (TreeNewHandleForUpdate)
        TreeNew_InvalidateNode(TreeNewHandleForUpdate, Node);
}

#define PH_TICK_SH_STATE_TN(NodeType, ShStateFieldName, StateList, RemoveFunction, HighlightingDuration, TreeNewHandleForUpdate, Invalidate, FullyInvalidated, ...) \
    do { \
        NodeType *node; \
        ULONG enumerationKey = 0; \
        ULONG tickCount; \
        BOOLEAN preferFullInvalidate; \
        HANDLE stateListHandle; \
        BOOLEAN redrawDisabled = FALSE; \
        BOOLEAN needsFullInvalidate = FALSE; \
\
        if (!StateList || StateList->Count == 0) \
            break; \
\
        tickCount = GetTickCount(); \
        preferFullInvalidate = StateList->Count > 8; \
\
        while (PhEnumPointerList(StateList, &enumerationKey, &node)) \
        { \
            if (PhRoundNumber(tickCount - node->ShStateFieldName.TickCount, 100) < (HighlightingDuration)) \
                continue; \
\
            stateListHandle = node->ShStateFieldName.StateListHandle; \
\
            if (node->ShStateFieldName.State == NewItemState) \
            { \
                node->ShStateFieldName.State = NormalItemState; \
                ((PPH_TREENEW_NODE)node)->UseTempBackColor = FALSE; \
                if (Invalidate) \
                { \
                    if (preferFullInvalidate) \
                    { \
                        needsFullInvalidate = TRUE; \
                    } \
                    else \
                    { \
                        TreeNew_InvalidateNode(TreeNewHandleForUpdate, node); \
                    } \
                } \
            } \
            else if (node->ShStateFieldName.State == RemovingItemState) \
            { \
                if (TreeNewHandleForUpdate) \
                { \
                    if (!redrawDisabled) \
                    { \
                        TreeNew_SetRedraw((TreeNewHandleForUpdate), FALSE); \
                        redrawDisabled = TRUE; \
                    } \
                } \
\
                RemoveFunction(node, __VA_ARGS__); \
                needsFullInvalidate = TRUE; \
            } \
\
            PhRemoveItemPointerList(StateList, stateListHandle); \
        } \
\
        if (TreeNewHandleForUpdate) \
        { \
            if (redrawDisabled) \
                TreeNew_SetRedraw((TreeNewHandleForUpdate), TRUE); \
            if (needsFullInvalidate) \
            { \
                InvalidateRect((TreeNewHandleForUpdate), NULL, FALSE); \
                if (FullyInvalidated) \
                    *((PBOOLEAN)FullyInvalidated) = TRUE; \
            } \
        } \
    } while (0)

// Provider event queues

typedef enum _PH_PROVIDER_EVENT_TYPE
{
    ProviderAddedEvent = 1,
    ProviderModifiedEvent = 2,
    ProviderRemovedEvent = 3
} PH_PROVIDER_EVENT_TYPE;

typedef struct _PH_PROVIDER_EVENT
{
    ULONG_PTR TypeAndObject;
    ULONG RunId;
} PH_PROVIDER_EVENT, *PPH_PROVIDER_EVENT;

#define PH_PROVIDER_EVENT_TYPE_MASK 0x3
#define PH_PROVIDER_EVENT_OBJECT_MASK (~(ULONG_PTR)0x3)
#define PH_PROVIDER_EVENT_TYPE(Event) ((ULONG)(Event).TypeAndObject & PH_PROVIDER_EVENT_TYPE_MASK)
#define PH_PROVIDER_EVENT_OBJECT(Event) ((PVOID)((Event).TypeAndObject & PH_PROVIDER_EVENT_OBJECT_MASK))

typedef struct _PH_PROVIDER_EVENT_QUEUE
{
    PH_ARRAY Array;
    PH_QUEUED_LOCK Lock;
} PH_PROVIDER_EVENT_QUEUE, *PPH_PROVIDER_EVENT_QUEUE;

FORCEINLINE VOID PhInitializeProviderEventQueue(
    _Out_ PPH_PROVIDER_EVENT_QUEUE EventQueue,
    _In_ SIZE_T InitialCapacity
    )
{
    PhInitializeArray(&EventQueue->Array, sizeof(PH_PROVIDER_EVENT), InitialCapacity);
    PhInitializeQueuedLock(&EventQueue->Lock);
}

FORCEINLINE VOID PhDeleteProviderEventQueue(
    _Inout_ PPH_PROVIDER_EVENT_QUEUE EventQueue
    )
{
    PPH_PROVIDER_EVENT events;
    SIZE_T i;

    events = EventQueue->Array.Items;

    for (i = 0; i < EventQueue->Array.Count; i++)
    {
        if (PH_PROVIDER_EVENT_TYPE(events[i]) == ProviderAddedEvent)
            PhDereferenceObject(PH_PROVIDER_EVENT_OBJECT(events[i]));
    }

    PhDeleteArray(&EventQueue->Array);
}

FORCEINLINE VOID PhPushProviderEventQueue(
    _Inout_ PPH_PROVIDER_EVENT_QUEUE EventQueue,
    _In_ PH_PROVIDER_EVENT_TYPE Type,
    _In_opt_ PVOID Object,
    _In_ ULONG RunId
    )
{
    PH_PROVIDER_EVENT event;

    assert(!(PtrToUlong(Object) & PH_PROVIDER_EVENT_TYPE_MASK));
    event.TypeAndObject = (ULONG_PTR)Object | Type;
    event.RunId = RunId;

    PhAcquireQueuedLockExclusive(&EventQueue->Lock);
    PhAddItemArray(&EventQueue->Array, &event);
    PhReleaseQueuedLockExclusive(&EventQueue->Lock);
}

FORCEINLINE PPH_PROVIDER_EVENT PhFlushProviderEventQueue(
    _Inout_ PPH_PROVIDER_EVENT_QUEUE EventQueue,
    _In_ ULONG UpToRunId,
    _Out_ PULONG Count
    )
{
    PPH_PROVIDER_EVENT availableEvents;
    PPH_PROVIDER_EVENT events = NULL;
    SIZE_T count;

    PhAcquireQueuedLockExclusive(&EventQueue->Lock);
    availableEvents = EventQueue->Array.Items;

    for (count = 0; count < EventQueue->Array.Count; count++)
    {
        if ((LONG)(UpToRunId - availableEvents[count].RunId) < 0)
            break;
    }

    if (count != 0)
    {
        events = PhAllocateCopy(availableEvents, count * sizeof(PH_PROVIDER_EVENT));
        PhRemoveItemsArray(&EventQueue->Array, 0, count);
    }

    PhReleaseQueuedLockExclusive(&EventQueue->Lock);

    *Count = (ULONG)count;

    return events;
}

#endif
