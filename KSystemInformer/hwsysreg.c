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

#if defined(_M_ARM64)

#define KPH_ARM64_PMUVER_IMPDEF         0xF

typedef union _KPH_ARM64_MIDR
{
    struct
    {
        ULONG Revision : 4;
        ULONG PartNum : 12;
        ULONG Architecture : 4;
        ULONG Variant : 4;
        ULONG Implementer : 8;
    };

    ULONG Value;
} KPH_ARM64_MIDR, *PKPH_ARM64_MIDR;
C_ASSERT(sizeof(KPH_ARM64_MIDR) == sizeof(ULONG));

typedef union _KPH_ARM64_ID_AA64PFR0
{
    struct
    {
        ULONG64 Reserved1 : 44;
        ULONG64 Amu : 4;
        ULONG64 Reserved2 : 16;
    };

    ULONG64 Value;
} KPH_ARM64_ID_AA64PFR0, *PKPH_ARM64_ID_AA64PFR0;
C_ASSERT(sizeof(KPH_ARM64_ID_AA64PFR0) == sizeof(ULONG64));

typedef union _KPH_ARM64_ID_AA64DFR0
{
    struct
    {
        ULONG64 Reserved1 : 8;
        ULONG64 PmuVer : 4;
        ULONG64 Reserved2 : 52;
    };

    ULONG64 Value;
} KPH_ARM64_ID_AA64DFR0, *PKPH_ARM64_ID_AA64DFR0;
C_ASSERT(sizeof(KPH_ARM64_ID_AA64DFR0) == sizeof(ULONG64));

typedef struct _KPH_HW_SYSREG_CORE
{
    NTSTATUS Status;
    ULONG Midr;
    PKPH_DYN_HW_TABLE Table;
    BOOLEAN AmuImplemented;
    BOOLEAN PmuImplemented;
} KPH_HW_SYSREG_CORE, *PKPH_HW_SYSREG_CORE;

/**
 * \brief Reads a system register on the current processor.
 *
 * \param[in] Register The register (KPH_DYN_HW_SYSREG_*).
 * \param[out] Value Receives the value.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwReadSysregRaw(
    _In_ UCHAR Register,
    _Out_ PULONG64 Value
    )
{
    NTSTATUS status;
    ULONG64 value;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT((Register >= KPH_DYN_HW_SYSREG_MIDR_EL1) &&
              (Register <= KPH_DYN_HW_SYSREG_MAX));

    status = STATUS_SUCCESS;
    value = 0;

    //
    // ISB orders a counter read after the preceding instructions.
    //
    __try
    {
        switch (Register)
        {
            case KPH_DYN_HW_SYSREG_MIDR_EL1:
            {
                value = (ULONG64)_ReadStatusReg(ARM64_MIDR_EL1);
                break;
            }
            case KPH_DYN_HW_SYSREG_MPIDR_EL1:
            {
                value = (ULONG64)_ReadStatusReg(ARM64_MPIDR_EL1);
                break;
            }
            case KPH_DYN_HW_SYSREG_REVIDR_EL1:
            {
                value = (ULONG64)_ReadStatusReg(ARM64_REVIDR_EL1);
                break;
            }
            case KPH_DYN_HW_SYSREG_ID_AA64PFR0_EL1:
            {
                value = (ULONG64)_ReadStatusReg(ARM64_ID_AA64PFR0_EL1);
                break;
            }
            case KPH_DYN_HW_SYSREG_ID_AA64PFR1_EL1:
            {
                value = (ULONG64)_ReadStatusReg(ARM64_ID_AA64PFR1_EL1);
                break;
            }
            case KPH_DYN_HW_SYSREG_ID_AA64DFR0_EL1:
            {
                value = (ULONG64)_ReadStatusReg(ARM64_ID_AA64DFR0_EL1);
                break;
            }
            case KPH_DYN_HW_SYSREG_CNTFRQ_EL0:
            {
                value = (ULONG64)_ReadStatusReg(ARM64_CNTFRQ_EL0);
                break;
            }
            case KPH_DYN_HW_SYSREG_CNTVCT_EL0:
            {
                __isb(_ARM64_BARRIER_SY);
                value = (ULONG64)_ReadStatusReg(ARM64_CNTVCT_EL0);
                break;
            }
            case KPH_DYN_HW_SYSREG_PMCCNTR_EL0:
            {
                __isb(_ARM64_BARRIER_SY);
                value = (ULONG64)_ReadStatusReg(ARM64_PMCCNTR_EL0);
                break;
            }
            case KPH_DYN_HW_SYSREG_AMCFGR_EL0:
            {
                __isb(_ARM64_BARRIER_SY);
                value = (ULONG64)_ReadStatusReg(ARM64_AMCFGR_EL0);
                break;
            }
            case KPH_DYN_HW_SYSREG_AMCGCR_EL0:
            {
                __isb(_ARM64_BARRIER_SY);
                value = (ULONG64)_ReadStatusReg(ARM64_AMCGCR_EL0);
                break;
            }
            case KPH_DYN_HW_SYSREG_AMCNTENSET0_EL0:
            {
                __isb(_ARM64_BARRIER_SY);
                value = (ULONG64)_ReadStatusReg(ARM64_AMCNTENSET0_EL0);
                break;
            }
            case KPH_DYN_HW_SYSREG_AMEVCNTR00_EL0:
            {
                __isb(_ARM64_BARRIER_SY);
                value = (ULONG64)_ReadStatusReg(ARM64_AMEVCNTR00_EL0);
                break;
            }
            case KPH_DYN_HW_SYSREG_AMEVCNTR01_EL0:
            {
                __isb(_ARM64_BARRIER_SY);
                value = (ULONG64)_ReadStatusReg(ARM64_AMEVCNTR01_EL0);
                break;
            }
            case KPH_DYN_HW_SYSREG_AMEVCNTR02_EL0:
            {
                __isb(_ARM64_BARRIER_SY);
                value = (ULONG64)_ReadStatusReg(ARM64_AMEVCNTR02_EL0);
                break;
            }
            case KPH_DYN_HW_SYSREG_AMEVCNTR03_EL0:
            {
                __isb(_ARM64_BARRIER_SY);
                value = (ULONG64)_ReadStatusReg(ARM64_AMEVCNTR03_EL0);
                break;
            }
            default:
            {
                status = STATUS_NOT_SUPPORTED;
                break;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = STATUS_NOT_SUPPORTED;
    }

    if (NT_SUCCESS(status))
    {
        *Value = value;
    }

    return status;
}

/**
 * \brief Checks that the current processor implements a register's feature.
 *
 * \param[in] Core The processor the batch is pinned to.
 * \param[in] Register The register (KPH_DYN_HW_SYSREG_*).
 *
 * \return TRUE if the register may be read, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpHwSysregPrerequisite(
    _In_ PKPH_HW_SYSREG_CORE Core,
    _In_ UCHAR Register
    )
{
    KPH_PAGED_CODE_PASSIVE();

    switch (Register)
    {
        case KPH_DYN_HW_SYSREG_PMCCNTR_EL0:
        {
            return Core->PmuImplemented;
        }
        case KPH_DYN_HW_SYSREG_AMCFGR_EL0:
        case KPH_DYN_HW_SYSREG_AMCGCR_EL0:
        case KPH_DYN_HW_SYSREG_AMCNTENSET0_EL0:
        case KPH_DYN_HW_SYSREG_AMEVCNTR00_EL0:
        case KPH_DYN_HW_SYSREG_AMEVCNTR01_EL0:
        case KPH_DYN_HW_SYSREG_AMEVCNTR02_EL0:
        case KPH_DYN_HW_SYSREG_AMEVCNTR03_EL0:
        {
            return Core->AmuImplemented;
        }
        default:
        {
            NT_ASSERT((Register >= KPH_DYN_HW_SYSREG_MIDR_EL1) &&
                      (Register <= KPH_DYN_HW_SYSREG_CNTVCT_EL0));
            return TRUE;
        }
    }
}

/**
 * \brief Reads the current processor's identity and features and resolves.
 *
 * \param[in] Section The hardware section.
 * \param[out] Core Receives the processor's identity, features and row.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwInitializeSysregCore(
    _In_ PKPH_DYN_HW_SECTION Section,
    _Out_ PKPH_HW_SYSREG_CORE Core
    )
{
    NTSTATUS status;
    KPH_DYN_HW_IDENTITY identity;
    ULONG64 value;
    KPH_ARM64_MIDR midr;
    KPH_ARM64_ID_AA64PFR0 pfr0;
    KPH_ARM64_ID_AA64DFR0 dfr0;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(Core, sizeof(KPH_HW_SYSREG_CORE));

    status = KphpHwReadSysregRaw(KPH_DYN_HW_SYSREG_MIDR_EL1, &value);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwReadSysregRaw failed: %!STATUS!",
                      status);

        Core->Status = status;
        return;
    }

    midr.Value = (ULONG)value;
    Core->Midr = midr.Value;

    RtlZeroMemory(&identity, sizeof(identity));
    identity.ArmCpu.Implementer = (UCHAR)midr.Implementer;
    identity.ArmCpu.Variant = (UCHAR)midr.Variant;
    identity.ArmCpu.PartNum = (USHORT)midr.PartNum;
    identity.ArmCpu.Revision = (UCHAR)midr.Revision;

    status = KphDynHwFindTable(Section,
                               KPH_DYN_HW_IDENTITY_ARM_CPU,
                               &identity,
                               &Core->Table);
    NT_VERIFY(NT_SUCCESS(status) || (status == STATUS_NOT_FOUND));

    status = KphpHwReadSysregRaw(KPH_DYN_HW_SYSREG_ID_AA64PFR0_EL1, &value);
    if (NT_SUCCESS(status))
    {
        pfr0.Value = value;
        Core->AmuImplemented = (pfr0.Amu >= 1);
    }
    else
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwReadSysregRaw failed: %!STATUS!",
                      status);
    }

    status = KphpHwReadSysregRaw(KPH_DYN_HW_SYSREG_ID_AA64DFR0_EL1, &value);
    if (NT_SUCCESS(status))
    {
        dfr0.Value = value;
        Core->PmuImplemented = ((dfr0.PmuVer >= 1) &&
                                (dfr0.PmuVer < KPH_ARM64_PMUVER_IMPDEF));
    }
    else
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwReadSysregRaw failed: %!STATUS!",
                      status);
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "Sysreg batch MIDR 0x%08lx row %lu AMU %lu PMU %lu",
                  Core->Midr,
                  (ULONG)!!Core->Table,
                  (ULONG)Core->AmuImplemented,
                  (ULONG)Core->PmuImplemented);
}

/**
 * \brief Reads one allow-listed system register on the current processor.
 *
 * \param[in] Core The processor the batch is pinned to.
 * \param[in,out] Entry The batch entry.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwReadSysregEntry(
    _In_ PKPH_HW_SYSREG_CORE Core,
    _Inout_ PKPH_HW_SYSREG_BATCH_ENTRY Entry
    )
{
    NTSTATUS status;
    PKPH_DYN_HW_OP op;
    PKPH_DYN_HW_SYSREG_READ sysreg;
    ULONG64 value;

    KPH_PAGED_CODE_PASSIVE();

    Entry->Midr = Core->Midr;

    if (!NT_SUCCESS(Core->Status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Sysreg %lu refused: no processor identity: %!STATUS!",
                      (ULONG)Entry->Register,
                      Core->Status);

        Entry->Status = Core->Status;
        return;
    }

    if (!Core->Table)
    {
        Entry->Status = STATUS_NOT_FOUND;
        return;
    }

    status = KphDynHwFindSysregOp(Core->Table, Entry->Register, &op);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphDynHwFindSysregOp failed: %!STATUS!",
                      status);

        Entry->Status = status;
        return;
    }

    sysreg = (PKPH_DYN_HW_SYSREG_READ)op->Body;

    if (op->Gate.Kind == KPH_DYN_HW_GATE_SYSREG_FIELD)
    {
        NT_ASSERT((op->Gate.Register >= KPH_DYN_HW_SYSREG_MIDR_EL1) &&
                  (op->Gate.Register <= KPH_DYN_HW_SYSREG_ID_LAST));

        status = KphpHwReadSysregRaw(op->Gate.Register, &value);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpHwReadSysregRaw failed: %!STATUS!",
                          status);

            Entry->Status = STATUS_NOT_FOUND;
            return;
        }

        if (!KphDynHwSysregFieldInRange(&op->Gate, value))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Sysreg %lu refused: gate value 0x%016llx",
                          (ULONG)Entry->Register,
                          value);

            Entry->Status = STATUS_NOT_FOUND;
            return;
        }
    }
    else
    {
        NT_ASSERT(op->Gate.Kind == KPH_DYN_HW_GATE_NONE);
    }

    if (!KphpHwSysregPrerequisite(Core, Entry->Register))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Sysreg %lu refused: feature not implemented",
                      (ULONG)Entry->Register);

        Entry->Status = STATUS_NOT_SUPPORTED;
        return;
    }

    status = KphpHwReadSysregRaw(Entry->Register, &value);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwReadSysregRaw failed: %!STATUS!",
                      status);

        Entry->Status = status;
        return;
    }

    Entry->Value = (value & sysreg->ReadMask);
    Entry->Status = STATUS_SUCCESS;
}

#endif // defined(_M_ARM64)

/**
 * \brief Reads a batch of allow-listed ARM64 system registers.
 *
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The batch entries.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadSysregBatch(
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_SYSREG_BATCH_ENTRY Entries,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
#if defined(_M_ARM64)
    NTSTATUS status;
    ULONG entriesLength;
    ULONG orderLength;
    ULONG bufferLength;
    PVOID buffer;
    PKPH_HW_SYSREG_BATCH_ENTRY entries;
    PULONG order;
    ULONG orderCount;
    PKPH_DYN dyn;
    BOOLEAN pinned;
    PROCESSOR_NUMBER processor;
    GROUP_AFFINITY previous;
    KPH_HW_SYSREG_CORE core;

    KPH_PAGED_CODE_PASSIVE();

    buffer = NULL;
    dyn = NULL;

    if (Count == 0)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "Sysreg batch is empty");

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
                          sizeof(KPH_HW_SYSREG_BATCH_ENTRY),
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

    buffer = KphAllocatePaged(bufferLength, KPH_TAG_HW_SYSREG_BATCH);
    if (!buffer)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate sysreg batch buffer");

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

    orderCount = 0;

    for (ULONG i = 0; i < Count; i++)
    {
        PKPH_HW_SYSREG_BATCH_ENTRY entry;
        ULONG j;

        entry = &entries[i];

        entry->Value = 0;
        entry->Midr = 0;

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

        for (j = orderCount; j > 0; j--)
        {
            PKPH_HW_SYSREG_BATCH_ENTRY prev;

            prev = &entries[order[j - 1]];

            if ((prev->Processor.Group < entry->Processor.Group) ||
                ((prev->Processor.Group == entry->Processor.Group) &&
                 (prev->Processor.Number <= entry->Processor.Number)))
            {
                break;
            }

            order[j] = order[j - 1];
        }

        order[j] = i;
        orderCount++;
    }

    RtlZeroMemory(&processor, sizeof(processor));
    RtlZeroMemory(&core, sizeof(core));
    pinned = FALSE;

    for (ULONG i = 0; i < orderCount; i++)
    {
        PKPH_HW_SYSREG_BATCH_ENTRY entry;

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

            KphpHwInitializeSysregCore(dyn->Hardware.Section, &core);
        }

        KphpHwReadSysregEntry(&core, entry);
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
        KphFree(buffer, KPH_TAG_HW_SYSREG_BATCH);
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
