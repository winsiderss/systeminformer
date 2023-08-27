/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *
 */

#include <kph.h>
#include <dyndata.h>
#include <comms.h>
#include <kphmsgdyn.h>

#include <trace.h>

typedef struct _KPH_DBG_PRINT_SLOT
{
    KDPC Dpc;
    LARGE_INTEGER TimeStamp;
    CLIENT_ID ContextClientId;
    ULONG ComponentId;
    ULONG Level;
    USHORT Length;
    CHAR Buffer[3 * 1024];
} KPH_DBG_PRINT_SLOT, *PKPH_DBG_PRINT_SLOT;

static BOOLEAN KphpDbgPrintInitialized = FALSE;
static KSPIN_LOCK KphpDbgPrintLock;
static ULONG KphpDbgPrintSlotNext = 0;
static ULONG KphpDbgPrintSlotCount = 0;
static PKPH_DBG_PRINT_SLOT KphpDbgPrintSlots;

/**
 * \brief Flushes the debug print slots to the communication port.
 */
VOID KphpDebugPrintFlush(
    VOID
    )
{
    for (ULONG i = 0; i < KphpDbgPrintSlotNext; i++)
    {
        NTSTATUS status;
        ANSI_STRING output;
        PKPH_MESSAGE msg;
        PKPH_DBG_PRINT_SLOT slot;

        slot = &KphpDbgPrintSlots[i];

        msg = KphAllocateNPagedMessage();
        if (!msg)
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          INFORMER,
                          "Failed to allocate message");
            continue;
        }

        KphMsgInit(msg, KphMsgDebugPrint);
        msg->Header.TimeStamp.QuadPart = slot->TimeStamp.QuadPart;
        msg->Kernel.DebugPrint.ContextClientId.UniqueProcess = slot->ContextClientId.UniqueProcess;
        msg->Kernel.DebugPrint.ContextClientId.UniqueThread = slot->ContextClientId.UniqueThread;
        msg->Kernel.DebugPrint.ComponentId = slot->ComponentId;
        msg->Kernel.DebugPrint.Level = slot->Level;

        output.Length = slot->Length;
        output.MaximumLength = slot->Length;
        output.Buffer = slot->Buffer;

        status = KphMsgDynAddAnsiString(msg, KphMsgFieldOutput, &output);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_WARNING,
                          INFORMER,
                          "KphMsgDynAddAnsiString failed: %!STATUS!",
                          status);
        }

        KphCommsSendNPagedMessageAsync(msg);
    }

    KphpDbgPrintSlotNext = 0;
}

/**
 * \brief DPC routine for debug informer.
 *
 * \param Dpc Unused
 * \param DeferredContext Unused
 * \param SystemArgument1 Unused
 * \param SystemArgument2 Unused
 */
_IRQL_requires_same_
VOID KphpDebugPrintDpc(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
    )
{
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    //
    // We use threaded DPCs so we could be invoked at passive or dispatch.
    //

    KeAcquireSpinLock(&KphpDbgPrintLock, &oldIrql);
    KphpDebugPrintFlush();
    KeReleaseSpinLock(&KphpDbgPrintLock, oldIrql);
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
    PKPH_DBG_PRINT_SLOT slot;

    NPAGED_CODE_DISPATCH_MIN();

    if (!KphInformerSettings.DebugPrint)
    {
        return;
    }

    slot = NULL;

    if (!Output->Buffer || (Output->Length == 0))
    {
        return;
    }

    KeAcquireSpinLockAtDpcLevel(&KphpDbgPrintLock);

    if (KphpDbgPrintSlotNext >= KphpDbgPrintSlotCount)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "Out of debug print slots!");
        goto Exit;
    }

    slot = &KphpDbgPrintSlots[KphpDbgPrintSlotNext++];

    KeQuerySystemTime(&slot->TimeStamp);
    slot->ContextClientId.UniqueProcess = PsGetCurrentProcessId();
    slot->ContextClientId.UniqueThread = PsGetCurrentThreadId();
    slot->ComponentId = ComponentId;
    slot->Level = Level;
    slot->Length = min(Output->Length, ARRAYSIZE(slot->Buffer));
    RtlCopyMemory(slot->Buffer, Output->Buffer, slot->Length);

    if (slot->Length != Output->Length)
    {
        KphTracePrint(TRACE_LEVEL_WARNING,
                      INFORMER,
                      "Truncated debug print line");
    }

Exit:

    KeReleaseSpinLockFromDpcLevel(&KphpDbgPrintLock);

    if (slot)
    {
        KeInsertQueueDpc(&slot->Dpc, NULL, NULL);
    }
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

    NPAGED_CODE_PASSIVE();

    NT_ASSERT(KphpDbgPrintSlotNext == 0);
    NT_ASSERT(!KphpDbgPrintInitialized);

    KeInitializeSpinLock(&KphpDbgPrintLock);
    count = (KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS) * 2);

    KphpDbgPrintSlots = KphAllocateNPaged((sizeof(KPH_DBG_PRINT_SLOT) * count),
                                          KPH_TAG_DBG_SLOTS);
    if (!KphpDbgPrintSlots)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
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
                                NULL);
    }

    status = DbgSetDebugPrintCallback(KphpDebugPrintCallback, TRUE);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
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
    KIRQL oldIrql;

    NPAGED_CODE_PASSIVE();

    if (!KphpDbgPrintInitialized)
    {
        return;
    }

    DbgSetDebugPrintCallback(KphpDebugPrintCallback, FALSE);

    for (ULONG i = 0; i < KphpDbgPrintSlotCount; i++)
    {
        KeRemoveQueueDpcEx(&KphpDbgPrintSlots[i].Dpc, TRUE);
    }

    KeAcquireSpinLock(&KphpDbgPrintLock, &oldIrql);

    KphpDebugPrintFlush();

    KphpDbgPrintSlotCount = 0;
    KphFree(KphpDbgPrintSlots, KPH_TAG_DBG_SLOTS);
    KphpDbgPrintSlots = NULL;

    KeReleaseSpinLock(&KphpDbgPrintLock, oldIrql);

    KphpDbgPrintInitialized = FALSE;
}
