/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2026
 *
 */

#include <kph.h>
#include <informer.h>
#include <comms.h>
#include <kphmsgdyn.h>

#include <trace.h>

typedef enum _KPH_DBG_PRINT_SLOT_STATE
{
    KphpDbgPrintSlotFree,
    KphpDbgPrintSlotClaimed,
    KphpDbgPrintSlotReady,
} KPH_DBG_PRINT_SLOT_STATE;

typedef struct _KPH_DBG_PRINT_SLOT
{
    KDPC Dpc;
    KPH_DBG_PRINT_SLOT_STATE State;
    LARGE_INTEGER TimeStamp;
    LARGE_INTEGER UnbiasedInterruptTime;
    KPHM_CONTEXT Context;
    ULONG ComponentId;
    ULONG Level;
    USHORT Length;
    CHAR Buffer[FIELD_SIZE(KPH_MESSAGE, _Dyn.Buffer)];
} KPH_DBG_PRINT_SLOT, *PKPH_DBG_PRINT_SLOT;

static BOOLEAN KphpDbgPrintInitialized = FALSE;
static ULONG KphpDbgPrintSlotCount = 0;
static PKPH_DBG_PRINT_SLOT KphpDbgPrintSlots = NULL;

FORCEINLINE
KPH_DBG_PRINT_SLOT_STATE
KphpGetDbgPrintSlotState(
    _In_ PKPH_DBG_PRINT_SLOT Slot
    )
{
    return ReadAcquire((PLONG)&Slot->State);
}

FORCEINLINE
VOID
KphpSetDbgPrintSlotState(
    _In_ PKPH_DBG_PRINT_SLOT Slot,
    _In_ KPH_DBG_PRINT_SLOT_STATE State
    )
{
    WriteRelease((PLONG)&Slot->State, State);
}

FORCEINLINE
_Must_inspect_result_
BOOLEAN
KphpTryClaimDbgPrintSlot(
    _In_ PKPH_DBG_PRINT_SLOT Slot
    )
{
    KPH_DBG_PRINT_SLOT_STATE state;

    state = InterlockedCompareExchange((PLONG)&Slot->State,
                                       KphpDbgPrintSlotClaimed,
                                       KphpDbgPrintSlotFree);

    return (state == KphpDbgPrintSlotFree);
}

/**
 * \brief Sends a ready debug print slot to the communication port.
 *
 * \param[in] Slot The ready slot to send.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphpDebugPrintSend(
    _In_ PKPH_DBG_PRINT_SLOT Slot
    )
{
    NTSTATUS status;
    USHORT remaining;
    ANSI_STRING output;
    PKPH_MESSAGE msg;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    NT_ASSERT(KphpGetDbgPrintSlotState(Slot) == KphpDbgPrintSlotReady);

    msg = KphAllocateNPagedMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Failed to allocate message");
        return;
    }

    KphMsgInit(msg, KphMsgDebugPrint);
    msg->Header.TimeStamp.QuadPart = Slot->TimeStamp.QuadPart;
    msg->Header.UnbiasedInterruptTime.QuadPart = Slot->UnbiasedInterruptTime.QuadPart;
    msg->Kernel.DebugPrint.Context = Slot->Context;
    msg->Kernel.DebugPrint.ComponentId = Slot->ComponentId;
    msg->Kernel.DebugPrint.Level = Slot->Level;

    remaining = KphMsgDynRemaining(msg);

    if (Slot->Length > remaining)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Truncated debug print line");
    }

    output.Length = min(Slot->Length, remaining);
    output.MaximumLength = Slot->Length;
    output.Buffer = Slot->Buffer;

    status = KphMsgDynAddAnsiString(msg, KphMsgFieldOutput, &output);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphMsgDynAddAnsiString failed: %!STATUS!",
                      status);
    }

    KphCommsSendNPagedMessageAsync(msg);
}

/**
 * \brief DPC routine for debug informer.
 *
 * \param Dpc Unused
 * \param DeferredContext The slot to send.
 * \param SystemArgument1 Unused
 * \param SystemArgument2 Unused
 */
_Function_class_(KDEFERRED_ROUTINE)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID KphpDebugPrintDpc(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
    )
{
    PKPH_DBG_PRINT_SLOT slot;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    NT_ASSERT(DeferredContext);

    slot = DeferredContext;

    KphpDebugPrintSend(slot);

    KphpSetDbgPrintSlotState(slot, KphpDbgPrintSlotFree);
}

/**
 * \brief Debug print callback.
 *
 * \param[in] Output Debug print string output.
 * \param[in] ComponentId Component ID printing to the debug output.
 * \param[in] Level Debug print level.
 */
_IRQL_always_function_min_(DISPATCH_LEVEL)
VOID KphpDebugPrintCallback(
    _In_ PSTRING Output,
    _In_ ULONG ComponentId,
    _In_ ULONG Level
    )
{
    BOOLEAN enabled;
    PKPH_DBG_PRINT_SLOT slot;

    KPH_NPAGED_CODE_DISPATCH_MIN();

    if (!Output->Buffer || (Output->Length == 0))
    {
        return;
    }

    KPH_INFORMER_CONTEXT_ENTER();
    enabled = KphInformerEnabled(DebugPrint);
    KPH_INFORMER_CONTEXT_EXIT();
    if (!enabled)
    {
        return;
    }

    slot = NULL;

    for (ULONG i = 0; i < KphpDbgPrintSlotCount; i++)
    {
        if (KphpTryClaimDbgPrintSlot(&KphpDbgPrintSlots[i]))
        {
            slot = &KphpDbgPrintSlots[i];
            break;
        }
    }

    if (!slot)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Out of debug print slots!");
        return;
    }

    KeQuerySystemTime(&slot->TimeStamp);
    slot->UnbiasedInterruptTime.QuadPart = KeQueryUnbiasedInterruptTime();
    KphCaptureCurrentContextEx(&slot->Context, TRUE);
    slot->ComponentId = ComponentId;
    slot->Level = Level;
    slot->Length = min(Output->Length, ARRAYSIZE(slot->Buffer));
    RtlCopyMemory(slot->Buffer, Output->Buffer, slot->Length);

    if (Output->Length > ARRAYSIZE(slot->Buffer))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Truncated debug print line");
    }

    KphpSetDbgPrintSlotState(slot, KphpDbgPrintSlotReady);

    NT_VERIFY(KeInsertQueueDpc(&slot->Dpc, NULL, NULL));
}

/**
 * \brief Starts the debug informer.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphDebugInformerStart(
    VOID
    )
{
    NTSTATUS status;
    ULONG count;

    KPH_NPAGED_CODE_PASSIVE();

    NT_ASSERT(!KphpDbgPrintInitialized);

    count = (KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS) * 2);

    KphpDbgPrintSlots = KphAllocateNPaged((sizeof(KPH_DBG_PRINT_SLOT) * count),
                                          KPH_TAG_DBG_SLOTS);
    if (!KphpDbgPrintSlots)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Failed to allocate slots for debug print callback");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    RtlZeroMemory(KphpDbgPrintSlots, sizeof(KPH_DBG_PRINT_SLOT) * count);

    KphpDbgPrintSlotCount = count;

    for (ULONG i = 0; i < KphpDbgPrintSlotCount; i++)
    {
        KeInitializeThreadedDpc(&KphpDbgPrintSlots[i].Dpc,
                                KphpDebugPrintDpc,
                                &KphpDbgPrintSlots[i]);
    }

    status = DbgSetDebugPrintCallback(KphpDebugPrintCallback, TRUE);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Failed to register debug print callback: %!STATUS!",
                      status);

        goto Exit;
    }

    status = STATUS_SUCCESS;
    KphpDbgPrintInitialized = TRUE;

Exit:

    if (!NT_SUCCESS(status))
    {
        NT_ASSERT(!KphpDbgPrintInitialized);

        KphpDbgPrintSlotCount = 0;

        if (KphpDbgPrintSlots)
        {
            KphFree(KphpDbgPrintSlots, KPH_TAG_DBG_SLOTS);
            KphpDbgPrintSlots = NULL;
        }
    }

    return status;
}

/**
 * \brief Stops the debug informer.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphDebugInformerStop(
    VOID
    )
{
    KPH_NPAGED_CODE_PASSIVE();

    if (!KphpDbgPrintInitialized)
    {
        return;
    }

    DbgSetDebugPrintCallback(KphpDebugPrintCallback, FALSE);

    for (ULONG i = 0; i < KphpDbgPrintSlotCount; i++)
    {
        PKPH_DBG_PRINT_SLOT slot;

        slot = &KphpDbgPrintSlots[i];

        KeRemoveQueueDpcEx(&slot->Dpc, TRUE);

        if (KphpGetDbgPrintSlotState(slot) == KphpDbgPrintSlotReady)
        {
            KphpDebugPrintSend(slot);
        }
    }

    KphpDbgPrintSlotCount = 0;
    KphFree(KphpDbgPrintSlots, KPH_TAG_DBG_SLOTS);
    KphpDbgPrintSlots = NULL;

    KphpDbgPrintInitialized = FALSE;
}
