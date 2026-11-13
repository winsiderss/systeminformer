/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include <kph.h>
#include <kphdyndata.h>
#include <kphhw.h>

#include <trace.h>

KPH_PAGED_FILE();

#if defined(_M_X64)

/**
 * \brief Evaluates an op's CPUID gate on the current processor.
 *
 * \param[in] Cpu The cached CPU identity.
 * \param[in] Op The MSR op.
 *
 * \return TRUE if the op applies to the current processor, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpHwMsrOpApplies(
    _In_ PCKPH_HW_CPU_IDENTITY Cpu,
    _In_ PCKPH_DYN_HW_OP Op
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return KphHwCpuidGatePasses(Cpu, &Op->Gate);
}

/**
 * \brief Reads one allow-listed MSR on the current processor.
 *
 * \details Caller is pinned to the entry's processor.
 *
 * \param[in] Cpu The cached CPU identity.
 * \param[in] Table The op table of this CPU, NULL when there is none.
 * \param[in,out] Entry The batch entry.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwReadMsrEntry(
    _In_ PCKPH_HW_CPU_IDENTITY Cpu,
    _In_opt_ PKPH_DYN_HW_TABLE Table,
    _Inout_ PKPH_HW_MSR_BATCH_ENTRY Entry
    )
{
    NTSTATUS status;
    PKPH_DYN_HW_OP op;
    PCKPH_DYN_HW_MSR_READ msr;
    ULONG64 value;

    KPH_PAGED_CODE_PASSIVE();

    if (!Table)
    {
        Entry->Status = STATUS_NOT_FOUND;
        return;
    }

    status = KphDynHwFindMsrOp(Table, Entry->Register, &op);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphDynHwFindMsrOp failed: %!STATUS!",
                      status);

        Entry->Status = status;
        return;
    }

    if (!KphpHwMsrOpApplies(Cpu, op))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "MSR 0x%08lx refused: the op does not apply",
                      Entry->Register);

        Entry->Status = STATUS_NOT_FOUND;
        return;
    }

    msr = (PCKPH_DYN_HW_MSR_READ)op->Body;

    __try
    {
        value = __readmsr(Entry->Register);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "MSR 0x%08lx read faulted",
                      Entry->Register);

        Entry->Status = STATUS_NOT_SUPPORTED;
        return;
    }

    Entry->Value = (value & msr->ReadMask);
    Entry->Status = STATUS_SUCCESS;
}

/**
 * \brief Orders MSR batch entries by processor, then by position.
 *
 * \param[in] Context The batch entries.
 * \param[in] LeftElement The index of the first entry.
 * \param[in] RightElement The index of the second entry.
 *
 * \return Negative, zero, or positive ordering of the entries.
 */
_Function_class_(KPH_QUICK_SORT_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
INT KSIAPI KphpHwMsrSortOrder(
    _In_opt_ PVOID Context,
    _In_ PCVOID LeftElement,
    _In_ PCVOID RightElement
    )
{
    PKPH_HW_MSR_BATCH_ENTRY entries;
    ULONG left;
    ULONG right;
    PPROCESSOR_NUMBER leftProcessor;
    PPROCESSOR_NUMBER rightProcessor;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    entries = Context;
    left = *(const ULONG*)LeftElement;
    right = *(const ULONG*)RightElement;
    leftProcessor = &entries[left].Processor;
    rightProcessor = &entries[right].Processor;

    if (leftProcessor->Group != rightProcessor->Group)
    {
        return ((leftProcessor->Group < rightProcessor->Group) ? -1 : 1);
    }

    if (leftProcessor->Number != rightProcessor->Number)
    {
        return ((leftProcessor->Number < rightProcessor->Number) ? -1 : 1);
    }

    if (left != right)
    {
        return ((left < right) ? -1 : 1);
    }

    return 0;
}

#endif // defined(_M_X64)

/**
 * \brief Reads a batch of allow-listed model-specific registers.
 *
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The batch entries.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadMsrBatch(
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_MSR_BATCH_ENTRY Entries,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
#if defined(_M_X64)
    NTSTATUS status;
    ULONG entriesLength;
    ULONG orderLength;
    ULONG bufferLength;
    PVOID buffer;
    PKPH_HW_MSR_BATCH_ENTRY entries;
    PULONG order;
    ULONG orderCount;
    PKPH_DYN dyn;
    PCKPH_HW_CPU_IDENTITY cpu;
    BOOLEAN pinned;
    PROCESSOR_NUMBER processor;
    GROUP_AFFINITY previous;

    KPH_PAGED_CODE_PASSIVE();

    buffer = NULL;
    dyn = NULL;

    if (Count == 0)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "MSR batch is empty");

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    dyn = KphReferenceDynData();
    if (!dyn || !dyn->Hardware.Section)
    {
        status = STATUS_NOINTERFACE;
        goto Exit;
    }

    status = RtlULongMult(Count,
                          sizeof(KPH_HW_MSR_BATCH_ENTRY),
                          &entriesLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongMult failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = RtlULongMult(Count, sizeof(ULONG), &orderLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongMult failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = RtlULongAdd(entriesLength, orderLength, &bufferLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongAdd failed: %!STATUS!",
                      status);

        goto Exit;
    }

    buffer = KphAllocatePaged(bufferLength, KPH_TAG_HW_MSR_BATCH);
    if (!buffer)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate MSR batch buffer");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    entries = buffer;
    order = Add2Ptr(buffer, entriesLength);

    status = KphCopyFromMode(entries, Entries, entriesLength, AccessMode);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    cpu = KphGetHardwareCpuIdentity();

    orderCount = 0;

    for (ULONG i = 0; i < Count; i++)
    {
        PKPH_HW_MSR_BATCH_ENTRY entry;

        entry = &entries[i];

        entry->Value = 0;

        if (!KphHwIsActiveProcessor(&entry->Processor))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Processor %lu:%lu is not active",
                          (ULONG)entry->Processor.Group,
                          (ULONG)entry->Processor.Number);

            entry->Status = STATUS_INVALID_PARAMETER;
            continue;
        }

        order[orderCount++] = i;
    }

    KphQuickSort(order,
                 orderCount,
                 sizeof(ULONG),
                 KphpHwMsrSortOrder,
                 entries);

    RtlZeroMemory(&processor, sizeof(processor));
    pinned = FALSE;

    for (ULONG i = 0; i < orderCount; i++)
    {
        PKPH_HW_MSR_BATCH_ENTRY entry;

        entry = &entries[order[i]];

        if (pinned &&
            ((entry->Processor.Group != processor.Group) ||
             (entry->Processor.Number != processor.Number)))
        {
            KphHwUnpinProcessor(&previous);
            pinned = FALSE;
        }

        if (!pinned)
        {
            processor = entry->Processor;
            KphHwPinProcessor(&processor, &previous);
            pinned = TRUE;
        }

        KphpHwReadMsrEntry(cpu, dyn->Hardware.CpuTable, entry);
    }

    if (pinned)
    {
        KphHwUnpinProcessor(&previous);
    }

    status = KphCopyToMode(Entries, entries, entriesLength, AccessMode);

Exit:

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    if (buffer)
    {
        KphFree(buffer, KPH_TAG_HW_MSR_BATCH);
    }

    return status;
#else
    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Count);
    UNREFERENCED_PARAMETER(Entries);
    UNREFERENCED_PARAMETER(AccessMode);

    return STATUS_NOT_SUPPORTED;
#endif
}
