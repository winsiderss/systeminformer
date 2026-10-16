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

#define KPH_HW_PORT_FCH_PM_INDEX            0xCD6
#define KPH_HW_PORT_FCH_PM_DATA             0xCD7

typedef union _KPH_HW_PORT_SIGNATURE
{
    struct
    {
        ULONG Offset : 16;
        ULONG Length : 16;
    };

    ULONG Value;
} KPH_HW_PORT_SIGNATURE, *PKPH_HW_PORT_SIGNATURE;
C_ASSERT(sizeof(KPH_HW_PORT_SIGNATURE) == sizeof(ULONG));

KPH_PAGED_FILE();

#if defined(_M_X64)

/**
 * \brief Retrieves the ports of a compiled port pair.
 *
 * \param[in] PortPair The pair (KPH_DYN_HW_PORT_PAIR_*).
 * \param[out] IndexPort Receives the index port.
 * \param[out] DataPort Receives the data port.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphHwPortPairPorts(
    _In_ UCHAR PortPair,
    _Out_ PUSHORT IndexPort,
    _Out_ PUSHORT DataPort
    )
{
    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(PortPair == KPH_DYN_HW_PORT_PAIR_FCH_PM);
    UNREFERENCED_PARAMETER(PortPair);

    *IndexPort = KPH_HW_PORT_FCH_PM_INDEX;
    *DataPort = KPH_HW_PORT_FCH_PM_DATA;
}

/**
 * \brief Validates batch entries against a port_indirect_read op.
 *
 * \param[in] Op The port_indirect_read op.
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The batch entries, in system memory.
 *
 * \return TRUE if any entry is to be read, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpHwPortValidateEntries(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_PORT_INDIRECT_BATCH_ENTRY Entries
    )
{
    BOOLEAN anyAllowed;

    KPH_PAGED_CODE_PASSIVE();

    anyAllowed = FALSE;

    for (ULONG i = 0; i < Count; i++)
    {
        PKPH_HW_PORT_INDIRECT_BATCH_ENTRY entry;

        entry = &Entries[i];

        entry->Value = 0;

        if (!KphDynHwPortIndirectReadAllowed(Op, entry->Index))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Port index 0x%02lx refused",
                          (ULONG)entry->Index);

            entry->Status = STATUS_ACCESS_DENIED;
        }
        else
        {
            entry->Status = STATUS_PENDING;
            anyAllowed = TRUE;
        }
    }

    return anyAllowed;
}

/**
 * \brief Reads the pending entries through a port pair.
 *
 * \param[in] IndexPort The index port.
 * \param[in] DataPort The data port.
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The prepared batch entries.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(_Global_critical_region_)
VOID KphpHwPortReadIndirect(
    _In_ USHORT IndexPort,
    _In_ USHORT DataPort,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_PORT_INDIRECT_BATCH_ENTRY Entries
    )
{
    UCHAR original;
    UCHAR written;
    BOOLEAN moved;

    KPH_PAGED_CODE_PASSIVE();

    original = READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)IndexPort);
    written = original;
    moved = FALSE;

    for (ULONG i = 0; i < Count; i++)
    {
        PKPH_HW_PORT_INDIRECT_BATCH_ENTRY entry;

        entry = &Entries[i];

        if (entry->Status != STATUS_PENDING)
        {
            continue;
        }

        entry->Status = STATUS_RETRY;

        if (moved)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Port index 0x%02lx not started: the index moved",
                          (ULONG)entry->Index);

            continue;
        }

        for (ULONG attempt = 0; attempt < 2; attempt++)
        {
            UCHAR value;

            WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)IndexPort, entry->Index);
            written = entry->Index;

            value = READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)DataPort);

            if (READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)IndexPort) == entry->Index)
            {
                entry->Value = value;
                entry->Status = STATUS_SUCCESS;
                break;
            }
        }

        if (entry->Status != STATUS_SUCCESS)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Port index 0x%02lx moved during the read",
                          (ULONG)entry->Index);

            moved = TRUE;
        }
    }

    if ((written != original) &&
        (READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)IndexPort) == written))
    {
        WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)IndexPort, original);
    }
}

/**
 * \brief Checks the signature of a fixed port window. Caller holds the ISA
 * lock.
 *
 * \param[in] Port The first port of the signature.
 * \param[in] Length The signature length, in bytes.
 * \param[in] Expected The signature bytes, little endian.
 *
 * \return TRUE if the ports read the signature, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(_Global_critical_region_)
_Must_inspect_result_
BOOLEAN KphpHwPortSignatureMatches(
    _In_ ULONG Port,
    _In_ ULONG Length,
    _In_ ULONG Expected
    )
{
    ULONG value;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT((Length >= 1) && (Length <= KPH_DYN_HW_FIXED_IO_MAX_SIGNATURE));

    value = 0;

    for (ULONG i = 0; i < Length; i++)
    {
        ULONG port;
        UCHAR byte;

        NT_VERIFY(NT_SUCCESS(RtlULongAdd(Port, i, &port)));

        byte = READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)port);
        value |= ((ULONG)byte << (i * RTL_BITS_OF(UCHAR)));
    }

    return (value == Expected);
}

#endif

/**
 * \brief Reads a batch of allow-listed registers through a compiled port pair.
 *
 * \param[in] Locator The locator of the function proving the pair.
 * \param[in] OpIndex The index among the function's port_indirect_read ops.
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The batch entries.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadPortIndirectBatch(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ ULONG OpIndex,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_PORT_INDIRECT_BATCH_ENTRY Entries,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
#if defined(_M_X64)
    NTSTATUS status;
    KPH_HW_PCI_LOCATOR locator;
    ULONG entriesLength;
    PKPH_HW_PORT_INDIRECT_BATCH_ENTRY entries;
    PKPH_DYN dyn;
    PKPH_DYN_HW_OP op;
    USHORT indexPort;
    USHORT dataPort;

    KPH_PAGED_CODE_PASSIVE();

    entries = NULL;
    dyn = NULL;

    locator = *Locator;

    if (Count == 0)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "Port batch is empty");

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
                          sizeof(KPH_HW_PORT_INDIRECT_BATCH_ENTRY),
                          &entriesLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongMult failed: %!STATUS!",
                      status);

        goto Exit;
    }

    entries = KphAllocatePaged(entriesLength, KPH_TAG_HW_PORT_BATCH);
    if (!entries)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate port batch buffer");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphCopyFromMode(entries, Entries, entriesLength, AccessMode);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    status = KphHwPciResolvePortIndirect(&locator,
                                         dyn->Hardware.Section,
                                         OpIndex,
                                         &op);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwPciResolvePortIndirect failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (!KphpHwPortValidateEntries(op, Count, entries))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "No port entry is allowed");

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    KphHwPortPairPorts(((PKPH_DYN_HW_PORT_INDIRECT_READ)op->Body)->PortPair,
                       &indexPort,
                       &dataPort);

    KphHwAcquireIsaLock();

    KphpHwPortReadIndirect(indexPort, dataPort, Count, entries);

    KphHwReleaseIsaLock();

    status = KphCopyToMode(Entries, entries, entriesLength, AccessMode);

Exit:

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    if (entries)
    {
        KphFree(entries, KPH_TAG_HW_PORT_BATCH);
    }

    return status;
#else
    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Locator);
    UNREFERENCED_PARAMETER(OpIndex);
    UNREFERENCED_PARAMETER(Count);
    UNREFERENCED_PARAMETER(Entries);
    UNREFERENCED_PARAMETER(AccessMode);

    return STATUS_NOT_SUPPORTED;
#endif
}

/**
 * \brief Reads a batch of a FIXED_IO window of this board's row.
 *
 * \param[in] Op The window_read op, base kind FIXED_IO.
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The batch entries, in system memory.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadFixedIoWindow(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries
    )
{
#if defined(_M_X64)
    NTSTATUS status;
    PKPH_DYN_HW_WINDOW_READ window;
    ULONG base;
    KPH_HW_PORT_SIGNATURE signature;
    ULONG signaturePort;
    ULONG expected;
    ULONG spanBegin;
    ULONG spanEnd;
    BOOLEAN locked;

    KPH_PAGED_CODE_PASSIVE();

    locked = FALSE;

    window = (PKPH_DYN_HW_WINDOW_READ)Op->Body;

    NT_ASSERT(window->BaseKind == KPH_DYN_HW_WINDOW_BASE_FIXED_IO);

    base = (ULONG)window->FixedBase;

    if (!KphHwWindowValidateEntries(Op, Count, Entries, &spanBegin, &spanEnd))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "No window entry is allowed");

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    signature.Value = KphDynHwWindowAux(Op, 0);
    expected = KphDynHwWindowAux(Op, 1);

    NT_VERIFY(NT_SUCCESS(RtlULongAdd(base,
                                     signature.Offset,
                                     &signaturePort)));

    KphHwAcquireIsaLock();
    locked = TRUE;

    if (!KphpHwPortSignatureMatches(signaturePort,
                                    signature.Length,
                                    expected))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Port window %03lx+%lx refused: signature mismatch",
                      base,
                      window->WindowSize);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    for (ULONG i = 0; i < Count; i++)
    {
        ULONG port;

        if (Entries[i].Status != STATUS_PENDING)
        {
            continue;
        }

        NT_ASSERT(Entries[i].Width == 1);

        NT_VERIFY(NT_SUCCESS(RtlULongAdd(base, Entries[i].Offset, &port)));

        Entries[i].Value = READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)port);
        Entries[i].Status = STATUS_SUCCESS;
    }

    if (!KphpHwPortSignatureMatches(signaturePort,
                                    signature.Length,
                                    expected))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Port window %03lx+%lx signature changed",
                      base,
                      window->WindowSize);

        for (ULONG i = 0; i < Count; i++)
        {
            if (Entries[i].Status == STATUS_SUCCESS)
            {
                Entries[i].Value = 0;
                Entries[i].Status = STATUS_DEVICE_NOT_READY;
            }
        }
    }

    status = STATUS_SUCCESS;

Exit:

    if (locked)
    {
        KphHwReleaseIsaLock();
    }

    return status;
#else
    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Op);
    UNREFERENCED_PARAMETER(Count);
    UNREFERENCED_PARAMETER(Entries);

    return STATUS_NOT_SUPPORTED;
#endif
}
