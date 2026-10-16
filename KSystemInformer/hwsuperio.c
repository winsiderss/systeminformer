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

#define KPH_HW_SUPERIO_PORT_SLOT0           0x2E
#define KPH_HW_SUPERIO_PORT_SLOT1           0x4E
#define KPH_HW_SUPERIO_DATA_PORT_OFFSET     1
#define KPH_HW_SUPERIO_KEY_ENTER            0x87
#define KPH_HW_SUPERIO_KEY_ITE_ENTER1       0x01
#define KPH_HW_SUPERIO_KEY_ITE_ENTER2       0x55
#define KPH_HW_SUPERIO_KEY_ITE_SLOT0        0x55
#define KPH_HW_SUPERIO_KEY_ITE_SLOT1        0xAA
#define KPH_HW_SUPERIO_KEY_EXIT             0xAA
#define KPH_HW_SUPERIO_CR_CONFIG_CONTROL    0x02
#define KPH_HW_SUPERIO_CONFIG_CONTROL_EXIT  0x02
#define KPH_HW_SUPERIO_CR_LDN               0x07
#define KPH_HW_SUPERIO_CR_CHIP_ID           0x20
#define KPH_HW_SUPERIO_CR_MANUFACTURER      0x23
#define KPH_HW_SUPERIO_CR_ACTIVATE          0x30
#define KPH_HW_SUPERIO_CR_BASE              0x60
#define KPH_HW_SUPERIO_LDN_ACTIVE           0x01
#define KPH_HW_SUPERIO_FINTEK_MANUFACTURER  0x1934

#define KPH_HW_SUPERIO_HWM_ALIGN_MASK       0x0007ul
#define KPH_HW_SUPERIO_HWM_PAGE             4
#define KPH_HW_SUPERIO_HWM_INDEX            5
#define KPH_HW_SUPERIO_HWM_DATA             6
#define KPH_HW_SUPERIO_NUVOTON_BANK         0x4E
#define KPH_HW_SUPERIO_NUVOTON_VENDOR_INDEX 0x4F
#define KPH_HW_SUPERIO_NUVOTON_VENDOR_HIGH  0x8000
#define KPH_HW_SUPERIO_NUVOTON_ID_HIGH      0x5C
#define KPH_HW_SUPERIO_NUVOTON_ID_LOW       0xA3
#define KPH_HW_SUPERIO_EC_PAGE_FREE         0xFF
#define KPH_HW_SUPERIO_EC_CUSTOMER          0x0602
#define KPH_HW_SUPERIO_ITE_VENDOR           0x58
#define KPH_HW_SUPERIO_ITE_ID               0x90
#define KPH_HW_SUPERIO_ITE_ID_ALT           0x7F
#define KPH_HW_SUPERIO_KEY_SMSC_ENTER       0x55
#define KPH_HW_SUPERIO_SMSC_INDEX           0x70
#define KPH_HW_SUPERIO_SMSC_DATA            0x71
#define KPH_HW_SUPERIO_SMSC_VENDOR          0x3E
#define KPH_HW_SUPERIO_SMSC_ID              0x5C
#define KPH_HW_SUPERIO_SMSC_DEVICE          0x3D
#define KPH_HW_SUPERIO_SMSC_DEVICE_SCH311X  0x8C
#define KPH_HW_SUPERIO_SMSC_DEVICE_SCH5127  0x8E
#define KPH_HW_SUPERIO_SMSC_CHIP_SCH5127    0x86
#define KPH_HW_SUPERIO_SMSC_ALIGN_MASK      0x007Ful

typedef struct _KPH_HW_SUPERIO_SESSION
{
    PKPH_DYN_HW_SECTION Section;
    UCHAR Slot;
    UCHAR Family; // KPH_DYN_HW_SUPERIO_FAMILY_*
    USHORT Port;
    BOOLEAN Entered;
    USHORT ChipId;
    USHORT CustomerId;
    USHORT Base;
    PKPH_DYN_HW_OP Op;
    BOOLEAN BankRead;
    UCHAR OriginalBank;
    UCHAR Bank;
    ULONG64 Deadline;
    UCHAR ChassisType;
} KPH_HW_SUPERIO_SESSION, *PKPH_HW_SUPERIO_SESSION;

static KPH_RWLOCK KphpHwIsaLock;
static UCHAR KphpHwSuperIoHint[KPH_DYN_HW_SUPERIO_SLOTS] = { 0 };

KPH_PAGED_FILE();

/**
 * \brief Initializes the SuperIO infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeHardwareSuperIo(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphInitializeRWLock(&KphpHwIsaLock);
}

/**
 * \brief Cleans up the SuperIO infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupHardwareSuperIo(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphDeleteRWLock(&KphpHwIsaLock);
}

/**
 * \brief Acquires the ISA lock exclusive.
 *
 * \details Every legacy port request runs under it; no other lock is taken
 * under it.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Acquires_lock_(_Global_critical_region_)
VOID KphHwAcquireIsaLock(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphAcquireRWLockExclusive(&KphpHwIsaLock);
}

/**
 * \brief Releases the ISA lock acquired by KphHwAcquireIsaLock.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Releases_lock_(_Global_critical_region_)
VOID KphHwReleaseIsaLock(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphReleaseRWLock(&KphpHwIsaLock);
}

#if defined(_M_X64)

/**
 * \brief Waits at PASSIVE_LEVEL.
 *
 * \param[in] Milliseconds The time to wait.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwSuperIoDelay(
    _In_ ULONG Milliseconds
    )
{
    LARGE_INTEGER interval;

    KPH_PAGED_CODE_PASSIVE();

    interval.QuadPart = -10000ll * Milliseconds;

    KeDelayExecutionThread(KernelMode, FALSE, &interval);
}

/**
 * \brief Writes a byte to a port.
 *
 * \param[in] Port The port.
 * \param[in] Value The byte.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwSuperIoOut(
    _In_ ULONG Port,
    _In_ UCHAR Value
    )
{
    KPH_PAGED_CODE_PASSIVE();

    WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)Port, Value);
}

/**
 * \brief Reads a byte from a port.
 *
 * \param[in] Port The port.
 *
 * \return The byte.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
UCHAR KphpHwSuperIoIn(
    _In_ ULONG Port
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)Port);
}

/**
 * \brief Reads a configuration register.
 *
 * \param[in] Session The session.
 * \param[in] Register The configuration register.
 *
 * \return The register value.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
UCHAR KphpHwSuperIoReadConfig(
    _In_ PKPH_HW_SUPERIO_SESSION Session,
    _In_ UCHAR Register
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphpHwSuperIoOut(Session->Port, Register);

    return KphpHwSuperIoIn(Session->Port + KPH_HW_SUPERIO_DATA_PORT_OFFSET);
}

/**
 * \brief Writes a configuration register.
 *
 * \param[in] Session The session.
 * \param[in] Register The configuration register.
 * \param[in] Value The register value.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwSuperIoWriteConfig(
    _In_ PKPH_HW_SUPERIO_SESSION Session,
    _In_ UCHAR Register,
    _In_ UCHAR Value
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphpHwSuperIoOut(Session->Port, Register);
    KphpHwSuperIoOut(Session->Port + KPH_HW_SUPERIO_DATA_PORT_OFFSET, Value);
}

/**
 * \brief Reads a big-endian pair of configuration registers.
 *
 * \param[in] Session The session.
 * \param[in] Register The configuration register of the high byte; the low
 * byte follows it.
 *
 * \return The register pair value.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
USHORT KphpHwSuperIoReadConfigWord(
    _In_ PKPH_HW_SUPERIO_SESSION Session,
    _In_ UCHAR Register
    )
{
    USHORT value;

    KPH_PAGED_CODE_PASSIVE();

    value = ((USHORT)KphpHwSuperIoReadConfig(Session, Register) <<
             RTL_BITS_OF(UCHAR));
    value |= KphpHwSuperIoReadConfig(Session, Register + 1);

    return value;
}

/**
 * \brief Enters configuration mode with the key of the session's family.
 *
 * \param[in,out] Session The session.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwSuperIoEnter(
    _Inout_ PKPH_HW_SUPERIO_SESSION Session
    )
{
    UCHAR slotKey;

    KPH_PAGED_CODE_PASSIVE();

    if (Session->Family == KPH_DYN_HW_SUPERIO_FAMILY_ITE)
    {
        if (Session->Slot)
        {
            slotKey = KPH_HW_SUPERIO_KEY_ITE_SLOT1;
        }
        else
        {
            slotKey = KPH_HW_SUPERIO_KEY_ITE_SLOT0;
        }

        KphpHwSuperIoOut(Session->Port, KPH_HW_SUPERIO_KEY_ENTER);
        KphpHwSuperIoOut(Session->Port, KPH_HW_SUPERIO_KEY_ITE_ENTER1);
        KphpHwSuperIoOut(Session->Port, KPH_HW_SUPERIO_KEY_ITE_ENTER2);
        KphpHwSuperIoOut(Session->Port, slotKey);
    }
    else if (Session->Family == KPH_DYN_HW_SUPERIO_FAMILY_SMSC)
    {
        KphpHwSuperIoOut(Session->Port, KPH_HW_SUPERIO_KEY_SMSC_ENTER);
    }
    else
    {
        KphpHwSuperIoOut(Session->Port, KPH_HW_SUPERIO_KEY_ENTER);
        KphpHwSuperIoOut(Session->Port, KPH_HW_SUPERIO_KEY_ENTER);
    }

    Session->Entered = TRUE;
}

/**
 * \brief Exits configuration mode, if this session entered it.
 *
 * \param[in,out] Session The session.
 * \param[in] Exit How to exit (KPH_DYN_HW_SUPERIO_EXIT_*).
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwSuperIoExit(
    _Inout_ PKPH_HW_SUPERIO_SESSION Session,
    _In_ UCHAR Exit
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (!Session->Entered)
    {
        return;
    }

    switch (Exit)
    {
        case KPH_DYN_HW_SUPERIO_EXIT_KEY_AA:
        {
            KphpHwSuperIoOut(Session->Port, KPH_HW_SUPERIO_KEY_EXIT);
            break;
        }
        case KPH_DYN_HW_SUPERIO_EXIT_CONFIG_02:
        {
            KphpHwSuperIoWriteConfig(Session,
                                     KPH_HW_SUPERIO_CR_CONFIG_CONTROL,
                                     KPH_HW_SUPERIO_CONFIG_CONTROL_EXIT);
            break;
        }
        default:
        {
            NT_ASSERT(Exit == KPH_DYN_HW_SUPERIO_EXIT_NONE);
            break;
        }
    }

    Session->Entered = FALSE;
}

/**
 * \brief Exits configuration mode for a chip that was not identified.
 *
 * \param[in,out] Session The session.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwSuperIoAbandon(
    _Inout_ PKPH_HW_SUPERIO_SESSION Session
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (!Session->Entered)
    {
        return;
    }

    KphpHwSuperIoOut(Session->Port, KPH_HW_SUPERIO_KEY_EXIT);

    if (Session->Family != KPH_DYN_HW_SUPERIO_FAMILY_SMSC)
    {
        KphpHwSuperIoWriteConfig(Session,
                                 KPH_HW_SUPERIO_CR_CONFIG_CONTROL,
                                 KPH_HW_SUPERIO_CONFIG_CONTROL_EXIT);
    }

    Session->Entered = FALSE;
}

/**
 * \brief Validates a hardware monitor base read from the chip.
 *
 * \param[in] Family The SuperIO family.
 * \param[in] Raw The base registers as read.
 * \param[out] Base Receives the validated base.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwSuperIoValidateBase(
    _In_ UCHAR Family,
    _In_ USHORT Raw,
    _Out_ PUSHORT Base
    )
{
    USHORT base;

    KPH_PAGED_CODE_PASSIVE();

    *Base = 0;

    if ((Raw == 0) || (Raw == MAXUSHORT))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SuperIO monitor base 0x%04lx is not set",
                      (ULONG)Raw);

        return STATUS_DEVICE_NOT_READY;
    }

    base = Raw;

    if ((Family == KPH_DYN_HW_SUPERIO_FAMILY_FINTEK) &&
        ((base & KPH_HW_SUPERIO_HWM_ALIGN_MASK) == KPH_HW_SUPERIO_HWM_INDEX))
    {
        base &= ~KPH_HW_SUPERIO_HWM_ALIGN_MASK;
    }

    //
    // SMSC uses the HWM pair at +70h/+71h of its 128-byte runtime block.
    //
    if ((base & KPH_HW_SUPERIO_HWM_ALIGN_MASK) ||
        ((Family == KPH_DYN_HW_SUPERIO_FAMILY_SMSC) &&
         (base & KPH_HW_SUPERIO_SMSC_ALIGN_MASK)))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SuperIO monitor base 0x%04lx is misaligned",
                      (ULONG)base);

        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    *Base = base;

    return STATUS_SUCCESS;
}

/**
 * \brief Identifies the chip at the session's slot and derives its monitor
 * base.
 *
 * \param[in,out] Session The session, with Section, Slot, Family and Port set.
 * \param[in] OpIndex The index of the superio_read op among those of the chip.
 * \param[out] ChipMatched Receives TRUE when the chip ID matched a row.
 * \param[out] SlotBusy Receives TRUE when no family may probe the slot further.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwSuperIoConfigure(
    _Inout_ PKPH_HW_SUPERIO_SESSION Session,
    _In_ ULONG OpIndex,
    _Out_ PBOOLEAN ChipMatched,
    _Out_ PBOOLEAN SlotBusy
    )
{
    NTSTATUS status;
    KPH_DYN_HW_IDENTITY identity;
    PKPH_DYN_HW_TABLE table;
    PKPH_DYN_HW_OP first;
    PKPH_DYN_HW_SUPERIO_READ read;
    USHORT chipId;
    BOOLEAN known;
    USHORT vendorId;
    BOOLEAN identified;
    UCHAR exitKind;
    BOOLEAN ldnSelected;
    UCHAR ldn;
    UCHAR originalLdn;
    UCHAR activate;
    BOOLEAN requireActive;
    USHORT base;
    USHORT extent;

    KPH_PAGED_CODE_PASSIVE();

    *ChipMatched = FALSE;
    *SlotBusy = FALSE;

    identified = FALSE;
    exitKind = KPH_DYN_HW_SUPERIO_EXIT_NONE;
    ldnSelected = FALSE;
    ldn = 0;
    originalLdn = 0;

    if (Session->Family == KPH_DYN_HW_SUPERIO_FAMILY_ITE)
    {
        chipId = KphpHwSuperIoReadConfigWord(Session,
                                             KPH_HW_SUPERIO_CR_CHIP_ID);

        known = KphDynHwSuperIoChipKnown(Session->Section,
                                         Session->Slot,
                                         Session->Family,
                                         chipId,
                                         Session->ChassisType);
        if (!known)
        {
            if ((chipId != 0) && (chipId != MAXUSHORT))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              HARDWARE,
                              "SuperIO slot %lu holds unknown chip 0x%04lx",
                              (ULONG)Session->Slot,
                              (ULONG)chipId);

                *SlotBusy = TRUE;
                status = STATUS_NOT_FOUND;
                goto Exit;
            }

            KphpHwSuperIoEnter(Session);

            chipId = KphpHwSuperIoReadConfigWord(Session,
                                                 KPH_HW_SUPERIO_CR_CHIP_ID);
        }
    }
    else if (Session->Family == KPH_DYN_HW_SUPERIO_FAMILY_SMSC)
    {
        chipId = KphpHwSuperIoReadConfig(Session, KPH_HW_SUPERIO_CR_CHIP_ID);

        if ((chipId != 0) && (chipId != MAXUCHAR))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SuperIO slot %lu answers 0x%02lx before the SMSC "
                          "key",
                          (ULONG)Session->Slot,
                          (ULONG)chipId);

            *SlotBusy = TRUE;
            status = STATUS_NOT_FOUND;
            goto Exit;
        }

        KphpHwSuperIoEnter(Session);

        chipId = KphpHwSuperIoReadConfig(Session, KPH_HW_SUPERIO_CR_CHIP_ID);
    }
    else
    {
        KphpHwSuperIoEnter(Session);

        chipId = KphpHwSuperIoReadConfigWord(Session,
                                             KPH_HW_SUPERIO_CR_CHIP_ID);
    }

    known = KphDynHwSuperIoChipKnown(Session->Section,
                                     Session->Slot,
                                     Session->Family,
                                     chipId,
                                     Session->ChassisType);
    if (!known)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SuperIO slot %lu family %lu chip 0x%04lx is not known",
                      (ULONG)Session->Slot,
                      (ULONG)Session->Family,
                      (ULONG)chipId);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    if (Session->Family == KPH_DYN_HW_SUPERIO_FAMILY_FINTEK)
    {
        vendorId = KphpHwSuperIoReadConfigWord(Session,
                                               KPH_HW_SUPERIO_CR_MANUFACTURER);
        if (vendorId != KPH_HW_SUPERIO_FINTEK_MANUFACTURER)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SuperIO slot %lu manufacturer 0x%04lx is not "
                          "Fintek",
                          (ULONG)Session->Slot,
                          (ULONG)vendorId);

            status = STATUS_NOT_FOUND;
            goto Exit;
        }
    }

    *ChipMatched = TRUE;
    Session->ChipId = chipId;

    if (Session->Family == KPH_DYN_HW_SUPERIO_FAMILY_NUVOTON_EC)
    {
        identified = TRUE;
        exitKind = KPH_DYN_HW_SUPERIO_EXIT_KEY_AA;
        ldn = KPH_DYN_HW_SUPERIO_EC_LDN;
    }
    else
    {
        RtlZeroMemory(&identity, sizeof(identity));
        identity.SuperIo.Slot = Session->Slot;
        identity.SuperIo.VendorFamily = Session->Family;
        identity.SuperIo.ChipId = chipId;

        status = KphDynHwFindTable(Session->Section,
                                   KPH_DYN_HW_IDENTITY_SUPERIO,
                                   &identity,
                                   &table);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphDynHwFindTable failed: %!STATUS!",
                          status);

            goto Exit;
        }

        status = KphDynHwFindSuperIoOp(table, 0, &first);
        if (NT_SUCCESS(status))
        {
            read = (PKPH_DYN_HW_SUPERIO_READ)first->Body;

            identified = TRUE;
            exitKind = read->Exit;
        }

        status = KphDynHwFindSuperIoOp(table, OpIndex, &Session->Op);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphDynHwFindSuperIoOp failed: %!STATUS!",
                          status);

            goto Exit;
        }

        if (!KphDynHwSuperIoGatePasses(Session->Op, Session->ChassisType))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SuperIO op %lu is gated off for chassis type "
                          "0x%02lx",
                          OpIndex,
                          (ULONG)Session->ChassisType);

            Session->Op = NULL;
            status = STATUS_NOT_FOUND;
            goto Exit;
        }

        read = (PKPH_DYN_HW_SUPERIO_READ)Session->Op->Body;

        identified = TRUE;
        exitKind = read->Exit;
        ldn = read->Ldn;
    }

    originalLdn = KphpHwSuperIoReadConfig(Session, KPH_HW_SUPERIO_CR_LDN);

    KphpHwSuperIoWriteConfig(Session, KPH_HW_SUPERIO_CR_LDN, ldn);
    ldnSelected = TRUE;

    requireActive = TRUE;

    if (Session->Family == KPH_DYN_HW_SUPERIO_FAMILY_SMSC)
    {
        requireActive = FALSE;
    }

    activate = KphpHwSuperIoReadConfig(Session, KPH_HW_SUPERIO_CR_ACTIVATE);
    if (requireActive && !BooleanFlagOn(activate, KPH_HW_SUPERIO_LDN_ACTIVE))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SuperIO LDN 0x%02lx is not active",
                      (ULONG)ldn);

        status = STATUS_DEVICE_NOT_READY;
        goto Exit;
    }

    base = KphpHwSuperIoReadConfigWord(Session, KPH_HW_SUPERIO_CR_BASE);

    KphpHwSuperIoDelay(1);

    if ((KphpHwSuperIoReadConfig(Session, KPH_HW_SUPERIO_CR_LDN) != ldn) ||
        (KphpHwSuperIoReadConfigWord(Session, KPH_HW_SUPERIO_CR_BASE) != base))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SuperIO LDN 0x%02lx base 0x%04lx changed while read",
                      (ULONG)ldn,
                      (ULONG)base);

        status = STATUS_RETRY;
        goto Exit;
    }

    status = KphpHwSuperIoValidateBase(Session->Family, base, &Session->Base);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwSuperIoValidateBase failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (Session->Family == KPH_DYN_HW_SUPERIO_FAMILY_SMSC)
    {
        extent = (KPH_HW_SUPERIO_SMSC_ALIGN_MASK + 1);
    }
    else
    {
        extent = (KPH_HW_SUPERIO_HWM_ALIGN_MASK + 1);
    }

    if (!KphDynHwSuperIoBaseAllowed(Session->Section,
                                    Session->Slot,
                                    Session->Family,
                                    Session->ChipId,
                                    Session->ChassisType,
                                    Session->Base,
                                    extent))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SuperIO monitor base 0x%04lx outside the allowed range",
                      (ULONG)Session->Base);

        Session->Base = 0;
        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Exit;
    }

    status = STATUS_SUCCESS;

Exit:

    if (ldnSelected && !Session->Entered && (originalLdn != ldn))
    {
        KphpHwSuperIoWriteConfig(Session, KPH_HW_SUPERIO_CR_LDN, originalLdn);
    }

    if (identified)
    {
        KphpHwSuperIoExit(Session, exitKind);
    }
    else
    {
        KphpHwSuperIoAbandon(Session);
    }

    return status;
}

/**
 * \brief Ends hardware monitor access, restoring the original bank.
 *
 * \param[in,out] Session The session.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwSuperIoEndMonitor(
    _Inout_ PKPH_HW_SUPERIO_SESSION Session
    )
{
    ULONG index;
    ULONG data;

    KPH_PAGED_CODE_PASSIVE();

    if (Session->BankRead && (Session->Bank != Session->OriginalBank))
    {
        index = ((ULONG)Session->Base + KPH_HW_SUPERIO_HWM_INDEX);
        data = ((ULONG)Session->Base + KPH_HW_SUPERIO_HWM_DATA);

        KphpHwSuperIoOut(index, KPH_HW_SUPERIO_NUVOTON_BANK);
        KphpHwSuperIoOut(data, Session->OriginalBank);
        Session->Bank = Session->OriginalBank;
    }
}

/**
 * \brief Reads a hardware monitor register.
 *
 * \param[in,out] Session The session, between begin and end of monitor access.
 * \param[in] Register The register, (bank or page << 8) | index.
 * \param[out] Value Receives the register value.
 *
 * \return Successful or errant status; STATUS_IO_TIMEOUT when the EC page
 * stayed taken past the deadline.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwSuperIoReadMonitor(
    _Inout_ PKPH_HW_SUPERIO_SESSION Session,
    _In_ USHORT Register,
    _Out_ PUCHAR Value
    )
{
    UCHAR bank;
    ULONG index;
    ULONG data;
    ULONG page;

    KPH_PAGED_CODE_PASSIVE();

    *Value = 0;

    bank = (UCHAR)(Register >> 8);
    index = ((ULONG)Session->Base + KPH_HW_SUPERIO_HWM_INDEX);
    data = ((ULONG)Session->Base + KPH_HW_SUPERIO_HWM_DATA);
    page = ((ULONG)Session->Base + KPH_HW_SUPERIO_HWM_PAGE);

    switch (Session->Family)
    {
        case KPH_DYN_HW_SUPERIO_FAMILY_NUVOTON:
        {
            if (!Session->BankRead)
            {
                KphpHwSuperIoOut(index, KPH_HW_SUPERIO_NUVOTON_BANK);
                Session->OriginalBank = KphpHwSuperIoIn(data);
                Session->Bank = Session->OriginalBank;
                Session->BankRead = TRUE;
            }

            if (bank != Session->Bank)
            {
                KphpHwSuperIoOut(index, KPH_HW_SUPERIO_NUVOTON_BANK);
                KphpHwSuperIoOut(data, bank);
                Session->Bank = bank;
            }

            break;
        }
        case KPH_DYN_HW_SUPERIO_FAMILY_NUVOTON_EC:
        {
            while (KphpHwSuperIoIn(page) != KPH_HW_SUPERIO_EC_PAGE_FREE)
            {
                if (KphHwDeadlinePassed(Session->Deadline))
                {
                    KphTracePrint(TRACE_LEVEL_VERBOSE,
                                  HARDWARE,
                                  "SuperIO EC page busy, register 0x%04lx",
                                  (ULONG)Register);

                    return STATUS_IO_TIMEOUT;
                }

                KphpHwSuperIoDelay(1);
            }

            KphpHwSuperIoOut(page, KPH_HW_SUPERIO_EC_PAGE_FREE);
            KphpHwSuperIoOut(page, bank);
            break;
        }
        case KPH_DYN_HW_SUPERIO_FAMILY_SMSC:
        {
            NT_ASSERT(bank == 0);

            index = ((ULONG)Session->Base + KPH_HW_SUPERIO_SMSC_INDEX);
            data = ((ULONG)Session->Base + KPH_HW_SUPERIO_SMSC_DATA);
            break;
        }
        default:
        {
            NT_ASSERT(bank == 0);
            break;
        }
    }

    KphpHwSuperIoOut(index, (UCHAR)Register);
    *Value = KphpHwSuperIoIn(data);

    if (Session->Family == KPH_DYN_HW_SUPERIO_FAMILY_NUVOTON_EC)
    {
        KphpHwSuperIoOut(page, KPH_HW_SUPERIO_EC_PAGE_FREE);
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Checks the hardware monitor vendor ID of the session's family.
 *
 * \param[in,out] Session The session, between begin and end of monitor access.
 *
 * \return TRUE if the vendor ID reads back or the family has none, FALSE
 * otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpHwSuperIoCheckVendor(
    _Inout_ PKPH_HW_SUPERIO_SESSION Session
    )
{
    NTSTATUS status;
    UCHAR high;
    UCHAR low;

    KPH_PAGED_CODE_PASSIVE();

    if ((Session->Family != KPH_DYN_HW_SUPERIO_FAMILY_NUVOTON) &&
        (Session->Family != KPH_DYN_HW_SUPERIO_FAMILY_ITE) &&
        (Session->Family != KPH_DYN_HW_SUPERIO_FAMILY_SMSC))
    {
        return TRUE;
    }

    if (Session->Family == KPH_DYN_HW_SUPERIO_FAMILY_SMSC)
    {
        UCHAR device;

        status = KphpHwSuperIoReadMonitor(Session,
                                          KPH_HW_SUPERIO_SMSC_VENDOR,
                                          &low);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpHwSuperIoReadMonitor failed: %!STATUS!",
                          status);

            return FALSE;
        }

        status = KphpHwSuperIoReadMonitor(Session,
                                          KPH_HW_SUPERIO_SMSC_DEVICE,
                                          &high);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpHwSuperIoReadMonitor failed: %!STATUS!",
                          status);

            return FALSE;
        }

        if (Session->ChipId == KPH_HW_SUPERIO_SMSC_CHIP_SCH5127)
        {
            device = KPH_HW_SUPERIO_SMSC_DEVICE_SCH5127;
        }
        else
        {
            device = KPH_HW_SUPERIO_SMSC_DEVICE_SCH311X;
        }

        return ((low == KPH_HW_SUPERIO_SMSC_ID) && (high == device));
    }

    if (Session->Family == KPH_DYN_HW_SUPERIO_FAMILY_NUVOTON)
    {
        PKPH_DYN_HW_SUPERIO_READ read;
        UCHAR index;

        read = (PKPH_DYN_HW_SUPERIO_READ)Session->Op->Body;
        index = (read->VendorIndex ? read->VendorIndex :
                 (UCHAR)KPH_HW_SUPERIO_NUVOTON_VENDOR_INDEX);

        status = KphpHwSuperIoReadMonitor(
            Session,
            (USHORT)(KPH_HW_SUPERIO_NUVOTON_VENDOR_HIGH | index),
            &high);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpHwSuperIoReadMonitor failed: %!STATUS!",
                          status);

            return FALSE;
        }

        status = KphpHwSuperIoReadMonitor(Session, index, &low);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpHwSuperIoReadMonitor failed: %!STATUS!",
                          status);

            return FALSE;
        }

        return ((high == KPH_HW_SUPERIO_NUVOTON_ID_HIGH) &&
                (low == KPH_HW_SUPERIO_NUVOTON_ID_LOW));
    }

    NT_ASSERT(Session->Family == KPH_DYN_HW_SUPERIO_FAMILY_ITE);

    status = KphpHwSuperIoReadMonitor(Session,
                                      KPH_HW_SUPERIO_ITE_VENDOR,
                                      &low);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwSuperIoReadMonitor failed: %!STATUS!",
                      status);

        return FALSE;
    }

    return ((low == KPH_HW_SUPERIO_ITE_ID) ||
            (low == KPH_HW_SUPERIO_ITE_ID_ALT));
}

/**
 * \brief Resolves the op of an NCT668x chip by its customer ID.
 *
 * \param[in,out] Session The session, between begin and end of monitor access.
 * \param[in] OpIndex The index of the superio_read op among those of the chip.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwSuperIoResolveEc(
    _Inout_ PKPH_HW_SUPERIO_SESSION Session,
    _In_ ULONG OpIndex
    )
{
    NTSTATUS status;
    KPH_DYN_HW_IDENTITY identity;
    PKPH_DYN_HW_TABLE table;
    UCHAR high;
    UCHAR low;

    KPH_PAGED_CODE_PASSIVE();

    status = KphpHwSuperIoReadMonitor(Session,
                                      KPH_HW_SUPERIO_EC_CUSTOMER,
                                      &high);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwSuperIoReadMonitor failed: %!STATUS!",
                      status);

        return status;
    }

    status = KphpHwSuperIoReadMonitor(Session,
                                      KPH_HW_SUPERIO_EC_CUSTOMER + 1,
                                      &low);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwSuperIoReadMonitor failed: %!STATUS!",
                      status);

        return status;
    }

    Session->CustomerId = (((USHORT)high << RTL_BITS_OF(UCHAR)) | low);

    RtlZeroMemory(&identity, sizeof(identity));
    identity.SuperIo.Slot = Session->Slot;
    identity.SuperIo.VendorFamily = Session->Family;
    identity.SuperIo.ChipId = Session->ChipId;
    identity.SuperIo.CustomerId = Session->CustomerId;

    status = KphDynHwFindTable(Session->Section,
                               KPH_DYN_HW_IDENTITY_SUPERIO,
                               &identity,
                               &table);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphDynHwFindTable failed: %!STATUS!",
                      status);

        return status;
    }

    status = KphDynHwFindSuperIoOp(table, OpIndex, &Session->Op);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphDynHwFindSuperIoOp failed: %!STATUS!",
                      status);

        return status;
    }

    if (!KphDynHwSuperIoGatePasses(Session->Op, Session->ChassisType))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SuperIO op %lu is gated off for chassis type 0x%02lx",
                      OpIndex,
                      (ULONG)Session->ChassisType);

        Session->Op = NULL;
        return STATUS_NOT_FOUND;
    }

    NT_ASSERT(((PKPH_DYN_HW_SUPERIO_READ)Session->Op->Body)->Ldn ==
              KPH_DYN_HW_SUPERIO_EC_LDN);

    return STATUS_SUCCESS;
}

/**
 * \brief Validates batch entries against a superio_read op.
 *
 * \param[in] Op The superio_read op.
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The batch entries, in system memory.
 *
 * \return TRUE if any entry is to be read, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpHwSuperIoValidateEntries(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_SUPERIO_BATCH_ENTRY Entries
    )
{
    BOOLEAN anyAllowed;

    KPH_PAGED_CODE_PASSIVE();

    anyAllowed = FALSE;

    for (ULONG i = 0; i < Count; i++)
    {
        Entries[i].Value = 0;

        if (!KphDynHwSuperIoReadAllowed(Op, Entries[i].Register))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SuperIO register 0x%04lx refused",
                          (ULONG)Entries[i].Register);

            Entries[i].Status = STATUS_ACCESS_DENIED;
            continue;
        }

        Entries[i].Status = STATUS_PENDING;
        anyAllowed = TRUE;
    }

    return anyAllowed;
}

/**
 * \brief Identifies the chip at a slot and reads a batch of its registers.
 *
 * \param[in] Section The hardware section.
 * \param[in] Slot The configuration port slot.
 * \param[in] OpIndex The index of the superio_read op among those of the chip.
 * \param[in] Deadline The deadline of the request.
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The batch entries, in system memory.
 * \param[out] Chip Receives the identified chip.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwIsaLock)
_Must_inspect_result_
NTSTATUS KphpHwSuperIoRead(
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_range_(0, KPH_DYN_HW_SUPERIO_SLOTS - 1) UCHAR Slot,
    _In_ ULONG OpIndex,
    _In_ ULONG64 Deadline,
    _In_ ULONG Count,
    _Inout_updates_opt_(Count) PKPH_HW_SUPERIO_BATCH_ENTRY Entries,
    _Out_ PKPH_HW_SUPERIO_CHIP Chip
    )
{
    NTSTATUS status;
    KPH_HW_SUPERIO_SESSION session;
    UCHAR order[6];
    ULONG families;
    ULONG tried;
    BOOLEAN matched;
    BOOLEAN slotBusy;
    UCHAR chassisType;
    USHORT port;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(Slot < KPH_DYN_HW_SUPERIO_SLOTS);

    RtlZeroMemory(Chip, sizeof(KPH_HW_SUPERIO_CHIP));
    RtlZeroMemory(&session, sizeof(session));

    chassisType = KphGetHardwareChassisType();
    port = (Slot ? KPH_HW_SUPERIO_PORT_SLOT1 : KPH_HW_SUPERIO_PORT_SLOT0);

    families = KphDynHwSuperIoFamilies(Section, Slot, chassisType);
    if (!families)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SuperIO slot %lu has no family for chassis type 0x%02lx",
                      (ULONG)Slot,
                      (ULONG)chassisType);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    order[0] = KphpHwSuperIoHint[Slot];
    order[1] = KPH_DYN_HW_SUPERIO_FAMILY_ITE;
    order[2] = KPH_DYN_HW_SUPERIO_FAMILY_NUVOTON;
    order[3] = KPH_DYN_HW_SUPERIO_FAMILY_NUVOTON_EC;
    order[4] = KPH_DYN_HW_SUPERIO_FAMILY_FINTEK;
    order[5] = KPH_DYN_HW_SUPERIO_FAMILY_SMSC;

    status = STATUS_NOT_FOUND;
    tried = 0;
    matched = FALSE;
    slotBusy = FALSE;

    for (ULONG i = 0; i < ARRAYSIZE(order); i++)
    {
        if (!(families & (1ul << order[i])) || (tried & (1ul << order[i])))
        {
            continue;
        }

        tried |= (1ul << order[i]);

        RtlZeroMemory(&session, sizeof(session));
        session.Section = Section;
        session.Slot = Slot;
        session.Family = order[i];
        session.Port = port;
        session.ChassisType = chassisType;

        status = KphpHwSuperIoConfigure(&session, OpIndex, &matched, &slotBusy);
        if (matched || slotBusy)
        {
            break;
        }
    }

    KphpHwSuperIoHint[Slot] = (matched ? session.Family : 0);

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SuperIO slot %lu family %lu chip 0x%04lx: %!STATUS!",
                      (ULONG)Slot,
                      (ULONG)(matched ? session.Family : 0),
                      (ULONG)(matched ? session.ChipId : 0),
                      status);

        goto Exit;
    }

    session.Deadline = Deadline;

    if (session.Family == KPH_DYN_HW_SUPERIO_FAMILY_NUVOTON_EC)
    {
        status = KphpHwSuperIoResolveEc(&session, OpIndex);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpHwSuperIoResolveEc failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }

    Chip->Slot = Slot;
    Chip->VendorFamily = session.Family;
    Chip->ChipId = session.ChipId;
    Chip->CustomerId = session.CustomerId;

    if (Count == 0)
    {
        status = STATUS_SUCCESS;
        goto Exit;
    }

    NT_ASSERT(Entries);

    if (!KphpHwSuperIoValidateEntries(session.Op, Count, Entries))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "No SuperIO entry is allowed");

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (!KphpHwSuperIoCheckVendor(&session))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SuperIO slot %lu chip 0x%04lx failed the vendor "
                      "check",
                      (ULONG)Slot,
                      (ULONG)session.ChipId);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    for (ULONG i = 0; i < Count; i++)
    {
        if (Entries[i].Status != STATUS_PENDING)
        {
            continue;
        }

        Entries[i].Status = KphpHwSuperIoReadMonitor(&session,
                                                     Entries[i].Register,
                                                     &Entries[i].Value);
    }

    status = STATUS_SUCCESS;

Exit:

    KphpHwSuperIoEndMonitor(&session);

    return status;
}

#endif

/**
 * \brief Reads a batch of allow-listed SuperIO hardware monitor registers.
 *
 * \param[in] Slot The configuration port slot: 0 for 2Eh/2Fh, 1 for 4Eh/4Fh.
 * \param[in] OpIndex The index of the superio_read op among those of the chip.
 * \param[in] Count The number of entries, zero to only identify the chip.
 * \param[in,out] Entries The batch entries.
 * \param[out] Chip Optionally receives the identified chip.
 * \param[in] Timeout Optional timeout for the request; NULL is infinite.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadSuperIoBatch(
    _In_ UCHAR Slot,
    _In_ ULONG OpIndex,
    _In_ ULONG Count,
    _Inout_updates_opt_(Count) PKPH_HW_SUPERIO_BATCH_ENTRY Entries,
    _Out_opt_ PKPH_HW_SUPERIO_CHIP Chip,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
#if defined(_M_X64)
    NTSTATUS status;
    ULONG entriesLength;
    PKPH_HW_SUPERIO_BATCH_ENTRY entries;
    KPH_HW_SUPERIO_CHIP chip;
    PKPH_DYN dyn;
    ULONG64 deadline;

    KPH_PAGED_CODE_PASSIVE();

    entries = NULL;
    dyn = NULL;

    if ((Count && !Entries) || (Slot >= KPH_DYN_HW_SUPERIO_SLOTS))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SuperIO batch count %lu slot %lu is invalid",
                      Count,
                      (ULONG)Slot);

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    status = KphHwCaptureDeadline(Timeout, &deadline, AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwCaptureDeadline failed: %!STATUS!",
                      status);

        goto Exit;
    }

    dyn = KphReferenceDynData();
    if (!dyn || !dyn->Hardware.Section)
    {
        status = STATUS_NOINTERFACE;
        goto Exit;
    }

    status = RtlULongMult(Count,
                          sizeof(KPH_HW_SUPERIO_BATCH_ENTRY),
                          &entriesLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongMult failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (Count)
    {
        entries = KphAllocatePaged(entriesLength, KPH_TAG_HW_SUPERIO_BATCH);
        if (!entries)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Failed to allocate SuperIO batch buffer");

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        status = KphCopyFromMode(entries, Entries, entriesLength, AccessMode);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    KphAcquireRWLockExclusive(&KphpHwIsaLock);

    status = KphpHwSuperIoRead(dyn->Hardware.Section,
                               Slot,
                               OpIndex,
                               deadline,
                               Count,
                               entries,
                               &chip);

    KphReleaseRWLock(&KphpHwIsaLock);

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwSuperIoRead failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (Count)
    {
        status = KphCopyToMode(Entries, entries, entriesLength, AccessMode);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    if (Chip)
    {
        status = KphCopyToMode(Chip,
                               &chip,
                               sizeof(KPH_HW_SUPERIO_CHIP),
                               AccessMode);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    status = STATUS_SUCCESS;

Exit:

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    if (entries)
    {
        KphFree(entries, KPH_TAG_HW_SUPERIO_BATCH);
    }

    return status;
#else
    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Slot);
    UNREFERENCED_PARAMETER(OpIndex);
    UNREFERENCED_PARAMETER(Count);
    UNREFERENCED_PARAMETER(Entries);
    UNREFERENCED_PARAMETER(Chip);
    UNREFERENCED_PARAMETER(Timeout);
    UNREFERENCED_PARAMETER(AccessMode);

    return STATUS_NOT_SUPPORTED;
#endif
}
