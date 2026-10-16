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

#define KPH_HW_SMBUS_EXTENT                 0x20
#define KPH_HW_SMBUS_LEGACY_IO_END          0x100

#define KPH_HW_SMBUS_HSTSTS                 0x00
#define KPH_HW_SMBUS_HSTCNT                 0x02
#define KPH_HW_SMBUS_HSTCMD                 0x03
#define KPH_HW_SMBUS_HSTADD                 0x04
#define KPH_HW_SMBUS_HSTDAT0                0x05
#define KPH_HW_SMBUS_HSTDAT1                0x06
#define KPH_HW_SMBUS_PIIX4_SLVCNT           0x08

#define KPH_HW_SMBUS_STS_HOST_BUSY          0x01
#define KPH_HW_SMBUS_STS_INTR               0x02
#define KPH_HW_SMBUS_STS_DEV_ERR            0x04
#define KPH_HW_SMBUS_STS_BUS_ERR            0x08
#define KPH_HW_SMBUS_STS_FAILED             0x10
#define KPH_HW_SMBUS_STS_I801_INUSE         0x40
#define KPH_HW_SMBUS_STS_I801_BYTE_DONE     0x80
#define KPH_HW_SMBUS_STS_ERRORS             (KPH_HW_SMBUS_STS_DEV_ERR |        \
                                             KPH_HW_SMBUS_STS_BUS_ERR |        \
                                             KPH_HW_SMBUS_STS_FAILED)

#define KPH_HW_SMBUS_STS_I801_CLEAR         (KPH_HW_SMBUS_STS_I801_BYTE_DONE | \
                                             KPH_HW_SMBUS_STS_INTR |           \
                                             KPH_HW_SMBUS_STS_ERRORS)
#define KPH_HW_SMBUS_STS_PIIX4_CLEAR        (KPH_HW_SMBUS_STS_INTR |           \
                                             KPH_HW_SMBUS_STS_ERRORS)

#define KPH_HW_SMBUS_CNT_KILL               0x02
#define KPH_HW_SMBUS_CNT_BYTE_DATA          0x08
#define KPH_HW_SMBUS_CNT_WORD_DATA          0x0C
#define KPH_HW_SMBUS_CNT_START              0x40
#define KPH_HW_SMBUS_CNT_COMMAND_MASK       0x1C

#define KPH_HW_SMBUS_PIIX4_SEMAPHORE        0x10
#define KPH_HW_SMBUS_PIIX4_SEMAPHORE_CLEAR  0x20
#define KPH_HW_SMBUS_PIIX4_SEMAPHORE_TRIES  5

#define KPH_HW_SMBUS_I801_HOSTC_HST_EN      0x01
#define KPH_HW_SMBUS_I801_HOSTC_SMI_EN      0x02
#define KPH_HW_SMBUS_I801_HOSTC_I2C_EN      0x04
#define KPH_HW_SMBUS_I801_BAR_IO            0x00000001
#define KPH_HW_SMBUS_I801_BAR_MASK          0x0000FFE0

#define KPH_HW_SMBUS_SPIN_US                25
#define KPH_HW_SMBUS_SPIN_COUNT             80
#define KPH_HW_SMBUS_PIIX4_START_US         250

typedef struct _KPH_HW_SMBUS_SESSION
{
    UCHAR HostKind;
    UCHAR Port;
    USHORT Base;
    PKPH_DYN_HW_SMBUS_HOST Layout;
    PUCHAR Pm;
    BOOLEAN PmOpen;
    USHORT PmBaseValue;
    UCHAR PmSelect;
    UCHAR OriginalPort;
    BOOLEAN PortChanged;
    BOOLEAN Owned;
    BOOLEAN Started;
    NTSTATUS Stopped;
    ULONG64 Deadline;
} KPH_HW_SMBUS_SESSION, *PKPH_HW_SMBUS_SESSION;

static KPH_RWLOCK KphpHwSmbusLock;

KPH_PAGED_FILE();

/**
 * \brief Initializes the SMBus infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeHardwareSmbus(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphInitializeRWLock(&KphpHwSmbusLock);
}

/**
 * \brief Cleans up the SMBus infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupHardwareSmbus(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphDeleteRWLock(&KphpHwSmbusLock);
}

#if defined(_M_X64)

/**
 * \brief Waits at PASSIVE_LEVEL.
 *
 * \param[in] Milliseconds The time to wait.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwSmbusDelay(
    _In_ ULONG Milliseconds
    )
{
    LARGE_INTEGER interval;

    KPH_PAGED_CODE_PASSIVE();

    interval.QuadPart = (-10000ll * Milliseconds);

    KeDelayExecutionThread(KernelMode, FALSE, &interval);
}

/**
 * \brief Reads a host register.
 *
 * \param[in] Session The session.
 * \param[in] Register The register, relative to the base.
 *
 * \return The register value.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
UCHAR KphpHwSmbusIn(
    _In_ PKPH_HW_SMBUS_SESSION Session,
    _In_ UCHAR Register
    )
{
    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(Register < KPH_HW_SMBUS_EXTENT);

    return READ_PORT_UCHAR((PUCHAR)((ULONG_PTR)Session->Base + Register));
}

/**
 * \brief Writes a host register.
 *
 * \param[in] Session The session.
 * \param[in] Register The register, relative to the base.
 * \param[in] Value The value.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwSmbusOut(
    _In_ PKPH_HW_SMBUS_SESSION Session,
    _In_ UCHAR Register,
    _In_ UCHAR Value
    )
{
    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(Register < KPH_HW_SMBUS_EXTENT);

    WRITE_PORT_UCHAR((PUCHAR)((ULONG_PTR)Session->Base + Register), Value);
}

/**
 * \brief Accesses a PM register through the port pair of the layout. Nothing
 * arbitrates the pair, so the index is read back around the data access and
 * put back only while it is still ours.
 *
 * \param[in] Session The session, PM open through a port pair.
 * \param[in] Register The PM register.
 * \param[in] Write TRUE to write *Value, FALSE to read into it.
 * \param[in,out] Value The value to write, or receives the value read.
 *
 * \return TRUE on success, FALSE if the index moved.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpHwSmbusPmPort(
    _In_ PKPH_HW_SMBUS_SESSION Session,
    _In_ UCHAR Register,
    _In_ BOOLEAN Write,
    _Inout_ PUCHAR Value
    )
{
    BOOLEAN result;
    USHORT indexPort;
    USHORT dataPort;
    UCHAR original;
    UCHAR value;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(Session->Layout->PmAccess == KPH_DYN_HW_SMBUS_PM_PORT_PAIR);

    result = FALSE;

    KphHwPortPairPorts(Session->Layout->PortPair, &indexPort, &dataPort);

    KphHwAcquireIsaLock();

    original = READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)indexPort);

    WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)indexPort, Register);

    if (READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)indexPort) != Register)
    {
        goto Exit;
    }

    if (Write)
    {
        WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)dataPort, *Value);
        value = *Value;
    }
    else
    {
        value = READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)dataPort);
    }

    if (READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)indexPort) != Register)
    {
        goto Exit;
    }

    WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)indexPort, original);

    *Value = value;
    result = TRUE;

Exit:

    KphHwReleaseIsaLock();

    if (!result)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMBus PM index 0x%02lx moved",
                      (ULONG)Register);
    }

    return result;
}

/**
 * \brief Reads a PM register.
 *
 * \param[in] Session The session, PM open.
 * \param[in] Register The register, relative to the PM base.
 * \param[out] Value Receives the register value.
 *
 * \return TRUE on success, FALSE if the pair's index moved.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
_Success_(return)
BOOLEAN KphpHwSmbusPmIn(
    _In_ PKPH_HW_SMBUS_SESSION Session,
    _In_ UCHAR Register,
    _Out_ PUCHAR Value
    )
{
    UCHAR value;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(Session->PmOpen);

    if (Session->Pm)
    {
        NT_ASSERT(Register < Session->Layout->PmLength);

        *Value = READ_REGISTER_UCHAR(&Session->Pm[Register]);
        return TRUE;
    }

    value = 0;

    if (!KphpHwSmbusPmPort(Session, Register, FALSE, &value))
    {
        return FALSE;
    }

    *Value = value;
    return TRUE;
}

/**
 * \brief Writes a PM register.
 *
 * \param[in] Session The session, PM open.
 * \param[in] Register The register, relative to the PM base.
 * \param[in] Value The value.
 *
 * \return TRUE on success, FALSE if the pair's index moved.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpHwSmbusPmOut(
    _In_ PKPH_HW_SMBUS_SESSION Session,
    _In_ UCHAR Register,
    _In_ UCHAR Value
    )
{
    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(Session->PmOpen);

    if (Session->Pm)
    {
        NT_ASSERT(Register < Session->Layout->PmLength);

        WRITE_REGISTER_UCHAR(&Session->Pm[Register], Value);
        return TRUE;
    }

    return KphpHwSmbusPmPort(Session, Register, TRUE, &Value);
}

/**
 * \brief Reads the SMBus enable and base from the PM decode registers of the
 * layout; the pair is read twice and must agree.
 *
 * \param[in] Session The session, PM open.
 * \param[out] Base Receives the base, zero when not assigned.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwSmbusPmDecode(
    _In_ PKPH_HW_SMBUS_SESSION Session,
    _Out_ PULONG Base
    )
{
    UCHAR lowRegister;
    UCHAR highRegister;
    UCHAR low;
    UCHAR high;
    UCHAR lowCheck;
    UCHAR highCheck;

    KPH_PAGED_CODE_PASSIVE();

    *Base = 0;

    lowRegister = Session->Layout->DecodeLow;
    highRegister = Session->Layout->DecodeHigh;

    if (!KphpHwSmbusPmIn(Session, lowRegister, &low) ||
        !KphpHwSmbusPmIn(Session, highRegister, &high) ||
        !KphpHwSmbusPmIn(Session, lowRegister, &lowCheck) ||
        !KphpHwSmbusPmIn(Session, highRegister, &highCheck))
    {
        return STATUS_RETRY;
    }

    if ((low != lowCheck) || (high != highCheck))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMBus PM decode changed between reads");

        return STATUS_RETRY;
    }

    if ((low == MAXUCHAR) && (high == MAXUCHAR))
    {
        return STATUS_DEVICE_NOT_READY;
    }

    *Base = ((((ULONG)high << RTL_BITS_OF(UCHAR)) | low) &
             Session->Layout->BaseMask);

    if (!BooleanFlagOn(low, Session->Layout->EnableMask))
    {
        return STATUS_DEVICE_NOT_READY;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Selects the SMBus port; the select register is read back.
 *
 * \param[in] Session The session, PM open.
 * \param[in] Port The port to select.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwSmbusSelectPort(
    _In_ PKPH_HW_SMBUS_SESSION Session,
    _In_ UCHAR Port
    )
{
    UCHAR select;
    UCHAR check;

    KPH_PAGED_CODE_PASSIVE();

    if (!KphpHwSmbusPmIn(Session, Session->PmSelect, &select))
    {
        return STATUS_RETRY;
    }

    select = (UCHAR)((select & ~Session->Layout->SelectMask) |
                     ((Port << Session->Layout->SelectShift) &
                      Session->Layout->SelectMask));

    if (!KphpHwSmbusPmOut(Session, Session->PmSelect, select) ||
        !KphpHwSmbusPmIn(Session, Session->PmSelect, &check))
    {
        return STATUS_RETRY;
    }

    if (((check ^ select) & Session->Layout->SelectMask) != 0)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMBus PM 0x%02lx read back 0x%02lx, wrote 0x%02lx",
                      (ULONG)Session->PmSelect,
                      (ULONG)check,
                      (ULONG)select);

        return STATUS_RETRY;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Derives and validates the base of a host controller.
 *
 * \param[in,out] Session The session, with HostKind set.
 * \param[in] Host The configuration registers of the host controller.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwSmbusOpenHost(
    _Inout_ PKPH_HW_SMBUS_SESSION Session,
    _In_ PKPH_HW_SMBUS_HOST Host
    )
{
    NTSTATUS status;
    ULONG base;

    KPH_PAGED_CODE_PASSIVE();

    if (Session->HostKind == KPH_DYN_HW_SMBUS_HOST_I801)
    {
        if (!BooleanFlagOn(Host->Command, PCI_ENABLE_IO_SPACE) ||
            !BooleanFlagOn(Host->Hostc, KPH_HW_SMBUS_I801_HOSTC_HST_EN))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SMBus I801 host disabled: command 0x%04lx "
                          "HOSTC 0x%02lx",
                          (ULONG)Host->Command,
                          (ULONG)Host->Hostc);

            return STATUS_DEVICE_NOT_READY;
        }

        if (BooleanFlagOn(Host->Hostc, KPH_HW_SMBUS_I801_HOSTC_I2C_EN) ||
            !BooleanFlagOn(Host->Bar, KPH_HW_SMBUS_I801_BAR_IO) ||
            (Host->Bar & ~(KPH_HW_SMBUS_I801_BAR_MASK |
                           KPH_HW_SMBUS_I801_BAR_IO)))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SMBus I801 host misconfigured: HOSTC 0x%02lx "
                          "BAR 0x%08lx",
                          (ULONG)Host->Hostc,
                          Host->Bar);

            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }

        base = (Host->Bar & KPH_HW_SMBUS_I801_BAR_MASK);

        if (BooleanFlagOn(Host->Hostc, KPH_HW_SMBUS_I801_HOSTC_SMI_EN))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SMBus 0x%04lx HOSTC 0x%02lx: SMB_SMI_EN set",
                          base,
                          (ULONG)Host->Hostc);
        }
    }
    else
    {
        PKPH_DYN_HW_SMBUS_HOST layout;

        NT_ASSERT(Session->HostKind == KPH_DYN_HW_SMBUS_HOST_PIIX4);
        NT_ASSERT(Host->HostOp);

        layout = (PKPH_DYN_HW_SMBUS_HOST)Host->HostOp->Body;
        Session->Layout = layout;

        if (layout->PmAccess == KPH_DYN_HW_SMBUS_PM_MMIO)
        {
            PHYSICAL_ADDRESS physical;
            ULONG64 pmEnd;
            ULONG64 checkBegin;
            ULONG64 checkEnd;

            status = RtlULongLongAdd(layout->PmBase,
                                     layout->PmLength,
                                     &pmEnd);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              HARDWARE,
                              "RtlULongLongAdd failed: %!STATUS!",
                              status);

                return status;
            }

            if (pmEnd > (MAXULONG64 - PAGE_SIZE))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              HARDWARE,
                              "PM end 0x%llx cannot be page rounded",
                              pmEnd);

                return STATUS_DEVICE_CONFIGURATION_ERROR;
            }

            checkBegin = (ULONG64)PAGE_ALIGN(layout->PmBase);
            checkEnd = ROUND_TO_PAGES(pmEnd);

            status = KphHwWindowCheckRange(checkBegin, checkEnd);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              HARDWARE,
                              "KphHwWindowCheckRange failed: %!STATUS!",
                              status);

                return status;
            }

            physical.QuadPart = (LONGLONG)layout->PmBase;

            Session->Pm = MmMapIoSpaceEx(physical,
                                         layout->PmLength,
                                         PAGE_READWRITE | PAGE_NOCACHE);
            if (!Session->Pm)
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              HARDWARE,
                              "MmMapIoSpaceEx failed");

                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else
        {
            NT_ASSERT(layout->PmAccess == KPH_DYN_HW_SMBUS_PM_PORT_PAIR);
        }

        Session->PmOpen = TRUE;
        Session->PmSelect = layout->Select;

        if (layout->SelectChooseMask)
        {
            UCHAR choose;

            if (!KphpHwSmbusPmIn(Session, layout->SelectChoose, &choose))
            {
                return STATUS_RETRY;
            }

            if (BooleanFlagOn(choose, layout->SelectChooseMask))
            {
                Session->PmSelect = layout->SelectAlternate;
            }
        }

        status = KphpHwSmbusPmDecode(Session, &base);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SMBus PM decode failed: base 0x%04lx %!STATUS!",
                          base,
                          status);

            return status;
        }

        Session->PmBaseValue = (USHORT)base;
    }

    if (base == 0)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMBus host base not assigned");

        return STATUS_DEVICE_NOT_READY;
    }

    if (base < KPH_HW_SMBUS_LEGACY_IO_END)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMBus host base 0x%04lx is in the legacy ports",
                      base);

        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    Session->Base = (USHORT)base;

    return STATUS_SUCCESS;
}

/**
 * \brief Undoes KphpHwSmbusOpenHost.
 *
 * \param[in,out] Session The session.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwSmbusCloseHost(
    _Inout_ PKPH_HW_SMBUS_SESSION Session
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (Session->Pm)
    {
        MmUnmapIoSpace(Session->Pm, Session->Layout->PmLength);
        Session->Pm = NULL;
    }

    Session->PmOpen = FALSE;
}

/**
 * \brief Checks that the PM registers still enable the session's base.
 *
 * \param[in] Session The session, host open.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwSmbusPmCheck(
    _In_ PKPH_HW_SMBUS_SESSION Session
    )
{
    NTSTATUS status;
    ULONG base;

    KPH_PAGED_CODE_PASSIVE();

    if (!Session->PmOpen)
    {
        return STATUS_SUCCESS;
    }

    status = KphpHwSmbusPmDecode(Session, &base);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (base != Session->PmBaseValue)
    {
        return STATUS_DEVICE_NOT_READY;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Takes the host for the request.
 *
 * \param[in,out] Session The session, host open.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwSmbusAcquire(
    _Inout_ PKPH_HW_SMBUS_SESSION Session
    )
{
    UCHAR hststs;

    KPH_PAGED_CODE_PASSIVE();

    if (Session->HostKind == KPH_DYN_HW_SMBUS_HOST_I801)
    {
        hststs = KphpHwSmbusIn(Session, KPH_HW_SMBUS_HSTSTS);

        if (BooleanFlagOn(hststs, KPH_HW_SMBUS_STS_I801_INUSE))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SMBus 0x%04lx in use",
                          (ULONG)Session->Base);

            return STATUS_RETRY;
        }

        Session->Owned = TRUE;
    }
    else
    {
        UCHAR control;
        UCHAR select;

        control = KphpHwSmbusIn(Session, KPH_HW_SMBUS_PIIX4_SLVCNT);

        if (BooleanFlagOn(control, KPH_HW_SMBUS_PIIX4_SEMAPHORE))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SMBus 0x%04lx semaphore held",
                          (ULONG)Session->Base);

            return STATUS_RETRY;
        }

        for (ULONG i = 0; i < KPH_HW_SMBUS_PIIX4_SEMAPHORE_TRIES; i++)
        {
            KphpHwSmbusOut(Session,
                           KPH_HW_SMBUS_PIIX4_SLVCNT,
                           control | KPH_HW_SMBUS_PIIX4_SEMAPHORE);

            control = KphpHwSmbusIn(Session, KPH_HW_SMBUS_PIIX4_SLVCNT);

            if (BooleanFlagOn(control, KPH_HW_SMBUS_PIIX4_SEMAPHORE))
            {
                Session->Owned = TRUE;
                break;
            }

            KphpHwSmbusDelay(1);
        }

        if (!Session->Owned)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SMBus 0x%04lx semaphore not granted",
                          (ULONG)Session->Base);

            return STATUS_RETRY;
        }

        hststs = KphpHwSmbusIn(Session, KPH_HW_SMBUS_HSTSTS);

        if (BooleanFlagOn(hststs, KPH_HW_SMBUS_STS_HOST_BUSY))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SMBus 0x%04lx busy: status 0x%02lx",
                          (ULONG)Session->Base,
                          (ULONG)hststs);

            return STATUS_RETRY;
        }

        if (!KphpHwSmbusPmIn(Session, Session->PmSelect, &select))
        {
            return STATUS_RETRY;
        }

        select &= Session->Layout->SelectMask;

        Session->OriginalPort = (select >> Session->Layout->SelectShift);

        if (Session->OriginalPort != Session->Port)
        {
            NTSTATUS status;

            Session->PortChanged = TRUE;

            status = KphpHwSmbusSelectPort(Session, Session->Port);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              HARDWARE,
                              "KphpHwSmbusSelectPort failed: %!STATUS!",
                              status);

                return status;
            }
        }
    }

    hststs = KphpHwSmbusIn(Session, KPH_HW_SMBUS_HSTSTS);

    if (BooleanFlagOn(hststs, KPH_HW_SMBUS_STS_HOST_BUSY))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMBus 0x%04lx busy: status 0x%02lx",
                      (ULONG)Session->Base,
                      (ULONG)hststs);

        return STATUS_RETRY;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Restores the PM port select and releases the host.
 *
 * \param[in,out] Session The session, host owned.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwSmbusRelease(
    _Inout_ PKPH_HW_SMBUS_SESSION Session
    )
{
    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(Session->Owned);

    if (Session->PortChanged)
    {
        NTSTATUS status;

        status = KphpHwSmbusSelectPort(Session, Session->OriginalPort);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SMBus port 0x%02lx not restored: %!STATUS!",
                          (ULONG)Session->OriginalPort,
                          status);
        }

        Session->PortChanged = FALSE;
    }

    if (Session->HostKind == KPH_DYN_HW_SMBUS_HOST_I801)
    {
        UCHAR clear;

        clear = (Session->Started ? KPH_HW_SMBUS_STS_I801_CLEAR : 0);

        KphpHwSmbusOut(Session,
                       KPH_HW_SMBUS_HSTSTS,
                       (UCHAR)(KPH_HW_SMBUS_STS_I801_INUSE | clear));
    }
    else
    {
        UCHAR control;

        control = KphpHwSmbusIn(Session, KPH_HW_SMBUS_PIIX4_SLVCNT);
        control = (UCHAR)(control | KPH_HW_SMBUS_PIIX4_SEMAPHORE_CLEAR);

        KphpHwSmbusOut(Session, KPH_HW_SMBUS_PIIX4_SLVCNT, control);
    }

    Session->Owned = FALSE;
}

/**
 * \brief Kills an incomplete transaction and clears its completion flags.
 *
 * \param[in,out] Session The session, host acquired.
 * \param[in] Clear The completion flags of the host kind.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwSmbusKill(
    _Inout_ PKPH_HW_SMBUS_SESSION Session,
    _In_ UCHAR Clear
    )
{
    UCHAR control;
    UCHAR hststs;

    KPH_PAGED_CODE_PASSIVE();

    control = KphpHwSmbusIn(Session, KPH_HW_SMBUS_HSTCNT);
    control = (UCHAR)((control & ~KPH_HW_SMBUS_CNT_START) |
                      KPH_HW_SMBUS_CNT_KILL);

    KphpHwSmbusOut(Session, KPH_HW_SMBUS_HSTCNT, control);
    KphpHwSmbusDelay(1);

    control = KphpHwSmbusIn(Session, KPH_HW_SMBUS_HSTCNT);
    control = (UCHAR)(control & ~(KPH_HW_SMBUS_CNT_KILL |
                                  KPH_HW_SMBUS_CNT_START));

    KphpHwSmbusOut(Session, KPH_HW_SMBUS_HSTCNT, control);

    hststs = KphpHwSmbusIn(Session, KPH_HW_SMBUS_HSTSTS);

    if (BooleanFlagOn(hststs, KPH_HW_SMBUS_STS_HOST_BUSY) ||
        !BooleanFlagOn(hststs, KPH_HW_SMBUS_STS_FAILED))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMBus 0x%04lx failed terminating the transaction: "
                      "status 0x%02lx",
                      (ULONG)Session->Base,
                      (ULONG)hststs);
    }

    if (hststs & Clear)
    {
        KphpHwSmbusOut(Session, KPH_HW_SMBUS_HSTSTS, hststs & Clear);
    }
}

/**
 * \brief Runs one read transaction.
 *
 * \param[in,out] Session The session, host acquired.
 * \param[in] Address The 7-bit slave address.
 * \param[in] Command The command byte.
 * \param[in] Protocol KPH_DYN_HW_SMBUS_PROTOCOL_BYTE_DATA or _WORD_DATA.
 * \param[out] Value Receives the data; zero unless STATUS_SUCCESS.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwSmbusTransact(
    _Inout_ PKPH_HW_SMBUS_SESSION Session,
    _In_ UCHAR Address,
    _In_ UCHAR Command,
    _In_ UCHAR Protocol,
    _Out_ PUSHORT Value
    )
{
    UCHAR clear;
    UCHAR transaction;
    UCHAR hstadd;
    UCHAR hststs;
    UCHAR addressAfter;
    UCHAR commandAfter;
    UCHAR controlAfter;
    USHORT value;
    BOOLEAN i801;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(Session->Owned);
    NT_ASSERT(Address < 0x80);

    *Value = 0;

    i801 = (Session->HostKind == KPH_DYN_HW_SMBUS_HOST_I801);
    clear = (i801 ? KPH_HW_SMBUS_STS_I801_CLEAR : KPH_HW_SMBUS_STS_PIIX4_CLEAR);
    transaction = ((Protocol == KPH_DYN_HW_SMBUS_PROTOCOL_WORD_DATA) ?
                   KPH_HW_SMBUS_CNT_WORD_DATA : KPH_HW_SMBUS_CNT_BYTE_DATA);

    hstadd = (UCHAR)((Address << 1) | 1);

    hststs = KphpHwSmbusIn(Session, KPH_HW_SMBUS_HSTSTS);

    if (BooleanFlagOn(hststs, KPH_HW_SMBUS_STS_HOST_BUSY))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMBus 0x%04lx host busy: status 0x%02lx",
                      (ULONG)Session->Base,
                      (ULONG)hststs);

        Session->Stopped = STATUS_RETRY;
        return STATUS_RETRY;
    }

    if (hststs & clear)
    {
        KphpHwSmbusOut(Session, KPH_HW_SMBUS_HSTSTS, hststs & clear);

        hststs = KphpHwSmbusIn(Session, KPH_HW_SMBUS_HSTSTS);

        if (hststs & clear)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SMBus 0x%04lx not ready: status 0x%02lx",
                          (ULONG)Session->Base,
                          (ULONG)hststs);

            Session->Stopped = STATUS_DEVICE_NOT_READY;
            return STATUS_DEVICE_NOT_READY;
        }
    }

    Session->Started = TRUE;

    KphpHwSmbusOut(Session, KPH_HW_SMBUS_HSTADD, hstadd);
    KphpHwSmbusOut(Session, KPH_HW_SMBUS_HSTCMD, Command);

    if (i801)
    {
        KphpHwSmbusOut(Session,
                       KPH_HW_SMBUS_HSTCNT,
                       transaction | KPH_HW_SMBUS_CNT_START);
    }
    else
    {
        KphpHwSmbusOut(Session, KPH_HW_SMBUS_HSTCNT, transaction);
        KphpHwSmbusOut(Session,
                       KPH_HW_SMBUS_HSTCNT,
                       transaction | KPH_HW_SMBUS_CNT_START);
    }

    KeStallExecutionProcessor(i801 ?
                              KPH_HW_SMBUS_SPIN_US :
                              KPH_HW_SMBUS_PIIX4_START_US);

    for (ULONG i = 0; ; i++)
    {
        hststs = KphpHwSmbusIn(Session, KPH_HW_SMBUS_HSTSTS);

        if (!BooleanFlagOn(hststs, KPH_HW_SMBUS_STS_HOST_BUSY) &&
            (hststs & (KPH_HW_SMBUS_STS_INTR | KPH_HW_SMBUS_STS_ERRORS)))
        {
            break;
        }

        if (KphHwDeadlinePassed(Session->Deadline))
        {
            KphpHwSmbusKill(Session, clear);

            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SMBus 0x%04lx slave 0x%02lx command 0x%02lx "
                          "timed out, killed: status 0x%02lx",
                          (ULONG)Session->Base,
                          (ULONG)Address,
                          (ULONG)Command,
                          (ULONG)hststs);

            Session->Stopped = STATUS_IO_TIMEOUT;
            return STATUS_IO_TIMEOUT;
        }

        if (i < KPH_HW_SMBUS_SPIN_COUNT)
        {
            KeStallExecutionProcessor(KPH_HW_SMBUS_SPIN_US);
        }
        else
        {
            KphpHwSmbusDelay(1);
        }
    }

    value = KphpHwSmbusIn(Session, KPH_HW_SMBUS_HSTDAT0);

    if (Protocol == KPH_DYN_HW_SMBUS_PROTOCOL_WORD_DATA)
    {
        value |= ((USHORT)KphpHwSmbusIn(Session, KPH_HW_SMBUS_HSTDAT1) <<
                  RTL_BITS_OF(UCHAR));
    }

    addressAfter = KphpHwSmbusIn(Session, KPH_HW_SMBUS_HSTADD);
    commandAfter = KphpHwSmbusIn(Session, KPH_HW_SMBUS_HSTCMD);
    controlAfter = KphpHwSmbusIn(Session, KPH_HW_SMBUS_HSTCNT);
    controlAfter &= KPH_HW_SMBUS_CNT_COMMAND_MASK;

    if ((addressAfter != hstadd) ||
        (commandAfter != Command) ||
        (controlAfter != transaction))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMBus 0x%04lx slave 0x%02lx command 0x%02lx collided",
                      (ULONG)Session->Base,
                      (ULONG)Address,
                      (ULONG)Command);

        Session->Stopped = STATUS_RETRY;
        return STATUS_RETRY;
    }

    if (hststs & clear)
    {
        KphpHwSmbusOut(Session, KPH_HW_SMBUS_HSTSTS, hststs & clear);
    }

    if (hststs & (KPH_HW_SMBUS_STS_BUS_ERR | KPH_HW_SMBUS_STS_FAILED))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMBus 0x%04lx slave 0x%02lx command 0x%02lx "
                      "bus error: status 0x%02lx",
                      (ULONG)Session->Base,
                      (ULONG)Address,
                      (ULONG)Command,
                      (ULONG)hststs);

        Session->Stopped = STATUS_RETRY;
        return STATUS_RETRY;
    }

    if (BooleanFlagOn(hststs, KPH_HW_SMBUS_STS_DEV_ERR))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMBus 0x%04lx slave 0x%02lx command 0x%02lx "
                      "device error",
                      (ULONG)Session->Base,
                      (ULONG)Address,
                      (ULONG)Command);

        return STATUS_NO_SUCH_DEVICE;
    }

    *Value = value;

    return STATUS_SUCCESS;
}

/**
 * \brief Validates batch entries against an smbus_read op.
 *
 * \param[in] Op The smbus_read op.
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The batch entries, in system memory.
 *
 * \return TRUE if any entry is to be read, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpHwSmbusValidateEntries(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_SMBUS_BATCH_ENTRY Entries
    )
{
    BOOLEAN anyAllowed;

    KPH_PAGED_CODE_PASSIVE();

    anyAllowed = FALSE;

    for (ULONG i = 0; i < Count; i++)
    {
        PKPH_HW_SMBUS_BATCH_ENTRY entry;

        entry = &Entries[i];

        entry->Value = 0;

        if ((entry->Address >= 0x80) ||
            ((entry->Protocol != KPH_DYN_HW_SMBUS_PROTOCOL_BYTE_DATA) &&
             (entry->Protocol != KPH_DYN_HW_SMBUS_PROTOCOL_WORD_DATA)))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SMBus address 0x%02lx protocol %lu is invalid",
                          (ULONG)entry->Address,
                          (ULONG)entry->Protocol);

            entry->Status = STATUS_INVALID_PARAMETER;
            continue;
        }

        if (!KphDynHwSmbusReadAllowed(Op,
                                      entry->Address,
                                      entry->Command,
                                      entry->Protocol))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SMBus address 0x%02lx command 0x%02lx refused",
                          (ULONG)entry->Address,
                          (ULONG)entry->Command);

            entry->Status = STATUS_ACCESS_DENIED;
            continue;
        }

        entry->Status = STATUS_PENDING;
        anyAllowed = TRUE;
    }

    return anyAllowed;
}

/**
 * \brief Checks that a host controller still matches the one opened.
 *
 * \param[in] Locator The locator of the host controller.
 * \param[in] Section The hardware section.
 * \param[in] OpIndex The index of the smbus_read op of the controller.
 * \param[in] Op The op the host was opened with.
 * \param[in] Host The configuration registers the host was opened with.
 *
 * \return TRUE if unchanged, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpHwSmbusHostUnchanged(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ ULONG OpIndex,
    _In_ PKPH_DYN_HW_OP Op,
    _In_ PKPH_HW_SMBUS_HOST Host
    )
{
    NTSTATUS status;
    PKPH_DYN_HW_OP op;
    KPH_HW_SMBUS_HOST host;

    KPH_PAGED_CODE_PASSIVE();

    status = KphHwPciReadSmbusHost(Locator, Section, OpIndex, &op, &host);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwPciReadSmbusHost failed: %!STATUS!",
                      status);

        return FALSE;
    }

    return ((op == Op) &&
            (host.HostOp == Host->HostOp) &&
            (host.Revision == Host->Revision) &&
            (host.Hostc == Host->Hostc) &&
            (host.Command == Host->Command) &&
            (host.Bar == Host->Bar));
}

/**
 * \brief Runs a batch of read transactions on an SMBus host controller.
 *
 * \param[in] Locator The locator of the host controller.
 * \param[in] Section The hardware section.
 * \param[in] OpIndex The index of the smbus_read op of the controller.
 * \param[in] Deadline The deadline of the request.
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The batch entries, in system memory.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwSmbusLock)
_Must_inspect_result_
NTSTATUS KphpHwSmbusRead(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ ULONG OpIndex,
    _In_ ULONG64 Deadline,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_SMBUS_BATCH_ENTRY Entries
    )
{
    NTSTATUS status;
    PKPH_DYN_HW_OP op;
    KPH_HW_SMBUS_HOST host;
    KPH_HW_SMBUS_SESSION session;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(&session, sizeof(session));

    status = KphHwPciReadSmbusHost(Locator, Section, OpIndex, &op, &host);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwPciReadSmbusHost failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (!KphpHwSmbusValidateEntries(op, Count, Entries))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "No SMBus entry is allowed");

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    session.HostKind = host.HostKind;
    session.Port = ((PKPH_DYN_HW_SMBUS_READ)op->Body)->Port;
    session.Stopped = STATUS_SUCCESS;
    session.Deadline = Deadline;

    status = KphpHwSmbusOpenHost(&session, &host);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMBus %04lx:%02lx:%02lx.%lx host kind %lu "
                      "refused: %!STATUS!",
                      (ULONG)Locator->Segment,
                      (ULONG)Locator->Bus,
                      (ULONG)Locator->Device,
                      (ULONG)Locator->Function,
                      (ULONG)session.HostKind,
                      status);

        goto Exit;
    }

    status = KphpHwSmbusAcquire(&session);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwSmbusAcquire failed: %!STATUS!",
                      status);

        goto Exit;
    }

    for (ULONG i = 0; i < Count; i++)
    {
        PKPH_HW_SMBUS_BATCH_ENTRY entry;

        entry = &Entries[i];

        if (entry->Status != STATUS_PENDING)
        {
            continue;
        }

        if (!NT_SUCCESS(session.Stopped))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SMBus entry %lu not started: %!STATUS!",
                          i,
                          session.Stopped);

            entry->Status = STATUS_RETRY;
            continue;
        }

        if (KphHwDeadlinePassed(session.Deadline))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SMBus entry %lu not started: deadline passed",
                          i);

            entry->Status = STATUS_RETRY;
            continue;
        }

        entry->Status = KphpHwSmbusTransact(&session,
                                            entry->Address,
                                            entry->Command,
                                            entry->Protocol,
                                            &entry->Value);
    }

    status = KphpHwSmbusPmCheck(&session);

    if (NT_SUCCESS(status) &&
        !KphpHwSmbusHostUnchanged(Locator, Section, OpIndex, op, &host))
    {
        status = STATUS_DEVICE_NOT_READY;
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMBus host not confirmed after the batch: %!STATUS!",
                      status);

        for (ULONG i = 0; i < Count; i++)
        {
            if (Entries[i].Status == STATUS_SUCCESS)
            {
                Entries[i].Value = 0;
                Entries[i].Status = status;
            }
        }
    }

    status = STATUS_SUCCESS;

Exit:

    if (session.Owned)
    {
        KphpHwSmbusRelease(&session);
    }

    KphpHwSmbusCloseHost(&session);

    return status;
}

#endif

/**
 * \brief Reads a batch of allow-listed SMBus registers through a host
 * controller.
 *
 * \details User mode holds Global\Access_SMBUS.HTP.Method around the request,
 * and Global\Access_ISABUS.HTP.Method for PIIX4 hosts.
 *
 * \param[in] Locator The locator of the host controller.
 * \param[in] OpIndex The index of the smbus_read op of the controller.
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The batch entries.
 * \param[in] Timeout Optional timeout for the request; NULL is infinite.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadSmbusBatch(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ ULONG OpIndex,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_SMBUS_BATCH_ENTRY Entries,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
#if defined(_M_X64)
    NTSTATUS status;
    KPH_HW_PCI_LOCATOR locator;
    ULONG entriesLength;
    PKPH_HW_SMBUS_BATCH_ENTRY entries;
    PKPH_DYN dyn;
    ULONG64 deadline;

    KPH_PAGED_CODE_PASSIVE();

    entries = NULL;
    dyn = NULL;

    locator = *Locator;

    if (Count == 0)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMBus batch is empty");

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
                          sizeof(KPH_HW_SMBUS_BATCH_ENTRY),
                          &entriesLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongMult failed: %!STATUS!",
                      status);

        goto Exit;
    }

    entries = KphAllocatePaged(entriesLength, KPH_TAG_HW_SMBUS_BATCH);
    if (!entries)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate SMBus batch buffer");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphCopyFromMode(entries, Entries, entriesLength, AccessMode);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    KphAcquireRWLockExclusive(&KphpHwSmbusLock);

    status = KphpHwSmbusRead(&locator,
                             dyn->Hardware.Section,
                             OpIndex,
                             deadline,
                             Count,
                             entries);

    KphReleaseRWLock(&KphpHwSmbusLock);

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwSmbusRead failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphCopyToMode(Entries, entries, entriesLength, AccessMode);

Exit:

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    if (entries)
    {
        KphFree(entries, KPH_TAG_HW_SMBUS_BATCH);
    }

    return status;
#else
    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Locator);
    UNREFERENCED_PARAMETER(OpIndex);
    UNREFERENCED_PARAMETER(Count);
    UNREFERENCED_PARAMETER(Entries);
    UNREFERENCED_PARAMETER(Timeout);
    UNREFERENCED_PARAMETER(AccessMode);

    return STATUS_NOT_SUPPORTED;
#endif
}
