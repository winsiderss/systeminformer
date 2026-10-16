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

#define KPH_HW_MAILBOX_FIRMWARE_ENTRIES 16
#define KPH_HW_MAILBOX_SPIN_US          50
#define KPH_HW_MAILBOX_SPIN_COUNT       20

#define KPH_HW_MAILBOX_RSP_OK           0x01
#define KPH_HW_MAILBOX_RSP_BUSY         0xFC
#define KPH_HW_MAILBOX_RSP_PREREQ       0xFD
#define KPH_HW_MAILBOX_RSP_UNKNOWN      0xFE

typedef struct _KPH_HW_MAILBOX_FIRMWARE
{
    KPH_HW_PCI_LOCATOR Locator;
    ANSI_STRING Path;
    KPH_DYN_HW_MAILBOX_TRANSACT Mailbox;
    ULONG Version;
    ULONG64 LastUse;
} KPH_HW_MAILBOX_FIRMWARE, *PKPH_HW_MAILBOX_FIRMWARE;

#if defined(_M_X64)
typedef enum _KPH_HW_MAILBOX_REGISTER
{
    KphpHwMailboxMsg,
    KphpHwMailboxRsp,
    KphpHwMailboxArg
} KPH_HW_MAILBOX_REGISTER;

typedef struct _KPH_HW_MAILBOX_SESSION
{
    UCHAR Transport; // KPH_DYN_HW_MAILBOX_TRANSPORT_*
    UCHAR ArgCount;
    ULONG Msg;
    ULONG Rsp;
    ULONG Arg0;
    union
    {
        KPH_HW_SMN_SESSION Smn;
        KPH_HW_MMIO_SESSION Mmio;
    };
} KPH_HW_MAILBOX_SESSION, *PKPH_HW_MAILBOX_SESSION;
#endif

static KPH_RWLOCK KphpHwMailboxLock;
static KPH_HW_MAILBOX_FIRMWARE
KphpHwMailboxFirmware[KPH_HW_MAILBOX_FIRMWARE_ENTRIES] = { 0 };
static ULONG64 KphpHwMailboxClock = 0;

KPH_PAGED_FILE();

/**
 * \brief Releases a firmware cache entry.
 *
 * \param[in,out] Entry The entry, zeroed on return.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwMailboxFreeFirmware(
    _Inout_ PKPH_HW_MAILBOX_FIRMWARE Entry
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (Entry->Path.Buffer)
    {
        KphFree(Entry->Path.Buffer, KPH_TAG_HW_MAILBOX_PATH);
    }

    RtlZeroMemory(Entry, sizeof(KPH_HW_MAILBOX_FIRMWARE));
}

/**
 * \brief Initializes the mailbox infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeHardwareMailbox(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphInitializeRWLock(&KphpHwMailboxLock);
}

/**
 * \brief Cleans up the mailbox infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupHardwareMailbox(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphAcquireRWLockExclusive(&KphpHwMailboxLock);

    for (ULONG i = 0; i < ARRAYSIZE(KphpHwMailboxFirmware); i++)
    {
        KphpHwMailboxFreeFirmware(&KphpHwMailboxFirmware[i]);
    }

    KphReleaseRWLock(&KphpHwMailboxLock);

    KphDeleteRWLock(&KphpHwMailboxLock);
}

#if defined(_M_X64)

/**
 * \brief Waits at PASSIVE_LEVEL.
 *
 * \param[in] Milliseconds The time to wait.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwMailboxDelay(
    _In_ ULONG Milliseconds
    )
{
    ULONG64 ticks;
    LARGE_INTEGER interval;

    KPH_PAGED_CODE_PASSIVE();

    NT_VERIFY(NT_SUCCESS(RtlULongLongMult(Milliseconds, 10000, &ticks)));
    interval.QuadPart = -(LONG64)ticks;

    KeDelayExecutionThread(KernelMode, FALSE, &interval);
}

/**
 * \brief Opens the SMN mailbox of a root complex for a mailbox op.
 *
 * \param[in] Locator The locator of the root complex.
 * \param[in] Section The hardware section.
 * \param[in] Kind KPH_DYN_HW_KIND_MAILBOX_TRANSACT or
 * KPH_DYN_HW_KIND_SMU_TABLE_READ.
 * \param[in] OpIndex The index of the op of the kind of the root.
 * \param[out] Op Receives the op, valid while the section is referenced.
 * \param[out] Mailbox Receives the mailbox_transact op.
 * \param[out] Session Receives the session, closed with KphpHwMailboxClose.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwMailboxLock)
_Must_inspect_result_
NTSTATUS KphpHwMailboxOpenSmn(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ USHORT Kind,
    _In_ ULONG OpIndex,
    _Outptr_ PKPH_DYN_HW_OP* Op,
    _Outptr_ PKPH_DYN_HW_OP* Mailbox,
    _Out_ PKPH_HW_MAILBOX_SESSION Session
    )
{
    NTSTATUS status;
    PKPH_DYN_HW_MAILBOX_TRANSACT mailbox;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(Session, sizeof(KPH_HW_MAILBOX_SESSION));

    status = KphHwPciOpenSmn(Locator,
                             Section,
                             Kind,
                             OpIndex,
                             Op,
                             Mailbox,
                             &Session->Smn);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwPciOpenSmn failed: %!STATUS!",
                      status);

        return status;
    }

    mailbox = (PKPH_DYN_HW_MAILBOX_TRANSACT)(*Mailbox)->Body;

    NT_ASSERT(mailbox->Transport == KPH_DYN_HW_MAILBOX_TRANSPORT_SMN_PAIR);

    Session->Transport = KPH_DYN_HW_MAILBOX_TRANSPORT_SMN_PAIR;
    Session->ArgCount = mailbox->ArgCount;
    Session->Msg = mailbox->Address.Smn.Msg;
    Session->Rsp = mailbox->Address.Smn.Rsp;
    Session->Arg0 = mailbox->Address.Smn.Arg0;

    return STATUS_SUCCESS;
}

/**
 * \brief Opens the MMIO mailbox of an ACPI device for a mailbox op.
 *
 * \param[in] Path The normalized namespace path of the device; it must
 * outlive the session.
 * \param[in] Section The hardware section.
 * \param[in] OpIndex The index of the mailbox_transact op of the device.
 * \param[out] Op Receives the op, valid while the section is referenced.
 * \param[out] Session Receives the session, closed with KphpHwMailboxClose.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwMailboxLock)
_Must_inspect_result_
NTSTATUS KphpHwMailboxOpenMmio(
    _In_ PCANSI_STRING Path,
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ ULONG OpIndex,
    _Outptr_ PKPH_DYN_HW_OP* Op,
    _Out_ PKPH_HW_MAILBOX_SESSION Session
    )
{
    NTSTATUS status;
    PKPH_DYN_HW_MAILBOX_TRANSACT mailbox;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(Session, sizeof(KPH_HW_MAILBOX_SESSION));

    status = KphHwAcpiOpenMailbox(Path, Section, OpIndex, Op, &Session->Mmio);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwAcpiOpenMailbox failed: %!STATUS!",
                      status);

        return status;
    }

    mailbox = (PKPH_DYN_HW_MAILBOX_TRANSACT)(*Op)->Body;

    NT_ASSERT(mailbox->Transport == KPH_DYN_HW_MAILBOX_TRANSPORT_MMIO_WINDOW);

    Session->Transport = KPH_DYN_HW_MAILBOX_TRANSPORT_MMIO_WINDOW;
    Session->ArgCount = mailbox->ArgCount;
    Session->Msg = mailbox->Address.Window.Msg;
    Session->Rsp = mailbox->Address.Window.Rsp;
    Session->Arg0 = mailbox->Address.Window.Arg0;

    return STATUS_SUCCESS;
}

/**
 * \brief Closes a mailbox session.
 *
 * \param[in,out] Session The session.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwMailboxClose(
    _Inout_ PKPH_HW_MAILBOX_SESSION Session
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (Session->Transport == KPH_DYN_HW_MAILBOX_TRANSPORT_MMIO_WINDOW)
    {
        KphHwAcpiCloseMailbox(&Session->Mmio);
    }
    else
    {
        NT_ASSERT(Session->Transport == KPH_DYN_HW_MAILBOX_TRANSPORT_SMN_PAIR);

        KphHwPciCloseSmn(&Session->Smn);
    }

    RtlZeroMemory(Session, sizeof(KPH_HW_MAILBOX_SESSION));
}

/**
 * \brief Retrieves the address of a mailbox register.
 *
 * \param[in] Session The mailbox session.
 * \param[in] Register The register.
 * \param[in] Index The argument register, below ArgCount; zero otherwise.
 *
 * \return The SMN address or window offset of the register.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG KphpHwMailboxAddress(
    _In_ PKPH_HW_MAILBOX_SESSION Session,
    _In_ KPH_HW_MAILBOX_REGISTER Register,
    _In_ ULONG Index
    )
{
    KPH_PAGED_CODE_PASSIVE();

    switch (Register)
    {
        case KphpHwMailboxMsg:
        {
            NT_ASSERT(Index == 0);
            return Session->Msg;
        }
        case KphpHwMailboxRsp:
        {
            NT_ASSERT(Index == 0);
            return Session->Rsp;
        }
        default:
        {
            NT_ASSERT(Register == KphpHwMailboxArg);
            NT_ASSERT(Index < Session->ArgCount);
            return (Session->Arg0 + (Index * sizeof(ULONG)));
        }
    }
}

/**
 * \brief Retrieves the mapped address of a register of an MMIO mailbox.
 *
 * \param[in] Session The MMIO mailbox session.
 * \param[in] Offset The window offset of the register.
 *
 * \return The mapped register.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PULONG KphpHwMailboxMapped(
    _In_ PKPH_HW_MAILBOX_SESSION Session,
    _In_ ULONG Offset
    )
{
    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(Session->Transport == KPH_DYN_HW_MAILBOX_TRANSPORT_MMIO_WINDOW);
    NT_ASSERT(Session->Mmio.Mapping);
    NT_ASSERT((Offset >= Session->Mmio.MapOffset) &&
              ((Offset - Session->Mmio.MapOffset) <=
               (Session->Mmio.MapLength - sizeof(ULONG))));

    return Add2Ptr(Session->Mmio.Mapping, Offset - Session->Mmio.MapOffset);
}

/**
 * \brief Reads RSP or an argument register of a mailbox.
 *
 * \param[in] Session The mailbox session.
 * \param[in] Register KphpHwMailboxRsp or KphpHwMailboxArg.
 * \param[in] Index The argument register, below ArgCount; zero for RSP.
 * \param[out] Value Receives the register value.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwMailboxLock)
_Must_inspect_result_
NTSTATUS KphpHwMailboxRead(
    _In_ PKPH_HW_MAILBOX_SESSION Session,
    _In_ KPH_HW_MAILBOX_REGISTER Register,
    _In_ ULONG Index,
    _Out_ PULONG Value
    )
{
    ULONG address;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(Register != KphpHwMailboxMsg);

    address = KphpHwMailboxAddress(Session, Register, Index);

    if (Session->Transport == KPH_DYN_HW_MAILBOX_TRANSPORT_SMN_PAIR)
    {
        return KphHwPciSmnRead(&Session->Smn, address, Value);
    }

    *Value = READ_REGISTER_ULONG(KphpHwMailboxMapped(Session, address));

    return STATUS_SUCCESS;
}

/**
 * \brief Writes MSG, RSP or an argument register of a mailbox.
 *
 * \param[in] Session The mailbox session.
 * \param[in] Register The register.
 * \param[in] Index The argument register, below ArgCount; zero otherwise.
 * \param[in] Value The value to write.
 *
 * \return Successful or errant status; STATUS_RETRY when the SMN index moved.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwMailboxLock)
_Must_inspect_result_
NTSTATUS KphpHwMailboxWrite(
    _In_ PKPH_HW_MAILBOX_SESSION Session,
    _In_ KPH_HW_MAILBOX_REGISTER Register,
    _In_ ULONG Index,
    _In_ ULONG Value
    )
{
    ULONG address;

    KPH_PAGED_CODE_PASSIVE();

    address = KphpHwMailboxAddress(Session, Register, Index);

    if (Session->Transport == KPH_DYN_HW_MAILBOX_TRANSPORT_SMN_PAIR)
    {
        return KphHwPciSmnWrite(&Session->Smn, address, Value);
    }

    WRITE_REGISTER_ULONG(KphpHwMailboxMapped(Session, address), Value);

    return STATUS_SUCCESS;
}

/**
 * \brief Polls the RSP register of a mailbox until it is non-zero.
 *
 * \param[in] Session The mailbox session.
 * \param[in] Deadline The deadline of the request.
 * \param[out] Response Receives the non-zero RSP value.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwMailboxLock)
_Must_inspect_result_
NTSTATUS KphpHwMailboxWait(
    _In_ PKPH_HW_MAILBOX_SESSION Session,
    _In_ ULONG64 Deadline,
    _Out_ PULONG Response
    )
{
    NTSTATUS status;
    ULONG value;

    KPH_PAGED_CODE_PASSIVE();

    *Response = 0;

    for (ULONG i = 0; ; i++)
    {
        BOOLEAN expired;

        expired = KphHwDeadlinePassed(Deadline);

        status = KphpHwMailboxRead(Session, KphpHwMailboxRsp, 0, &value);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpHwMailboxRead failed: %!STATUS!",
                          status);

            return status;
        }

        if (value == MAXULONG)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Mailbox 0x%08lx RSP reads all ones",
                          Session->Msg);

            return STATUS_DEVICE_NOT_READY;
        }

        if (value)
        {
            *Response = value;
            return STATUS_SUCCESS;
        }

        if (expired)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Mailbox 0x%08lx RSP poll timed out",
                          Session->Msg);

            return STATUS_IO_TIMEOUT;
        }

        if (i < KPH_HW_MAILBOX_SPIN_COUNT)
        {
            KeStallExecutionProcessor(KPH_HW_MAILBOX_SPIN_US);
        }
        else
        {
            KphpHwMailboxDelay(1);
        }
    }
}

/**
 * \brief Restores the idle RSP value after a handshake failed before MSG.
 *
 * \param[in] Session The mailbox session.
 * \param[in] Idle The non-zero RSP value the idle poll read.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwMailboxLock)
VOID KphpHwMailboxRestoreIdle(
    _In_ PKPH_HW_MAILBOX_SESSION Session,
    _In_ ULONG Idle
    )
{
    NTSTATUS status;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(Idle && (Idle != MAXULONG));

    status = KphpHwMailboxWrite(Session, KphpHwMailboxRsp, 0, Idle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Mailbox 0x%08lx RSP not restored: %!STATUS!",
                      Session->Msg,
                      status);
    }
}

/**
 * \brief Runs one mailbox command.
 *
 * \details Another agent's command in flight (RSP zero) is never written over.
 *
 * \param[in] Session The mailbox session.
 * \param[in] Op The mailbox_transact op.
 * \param[in] Deadline The deadline of the request.
 * \param[in] Opcode The opcode.
 * \param[in] Args The argument register values, ArgCount of them.
 * \param[in] ResponseWords The argument registers read back on success.
 * \param[out] Results Receives the response words.
 * \param[in] ClientCommand TRUE to retry a torn MSG write; kernel-issued
 * commands are never posted twice.
 * \param[out] Completed Receives FALSE when the handshake failed and no
 * further command may run this request.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwMailboxLock)
_Must_inspect_result_
NTSTATUS KphpHwMailboxTransact(
    _In_ PKPH_HW_MAILBOX_SESSION Session,
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG64 Deadline,
    _In_ ULONG Opcode,
    _In_reads_(KPH_HW_MAILBOX_ARGS) PCULONG Args,
    _In_ UCHAR ResponseWords,
    _In_ BOOLEAN ClientCommand,
    _Out_writes_(KPH_HW_MAILBOX_ARGS) PULONG Results,
    _Out_ PBOOLEAN Completed
    )
{
    NTSTATUS status;
    PKPH_DYN_HW_MAILBOX_TRANSACT mailbox;
    ULONG response;
    BOOLEAN restoreIdle;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(Op->Kind == KPH_DYN_HW_KIND_MAILBOX_TRANSACT);

    RtlZeroMemory(Results, sizeof(ULONG) * KPH_HW_MAILBOX_ARGS);
    *Completed = FALSE;

    restoreIdle = FALSE;

    mailbox = (PKPH_DYN_HW_MAILBOX_TRANSACT)Op->Body;

    NT_ASSERT(mailbox->ArgCount <= KPH_HW_MAILBOX_ARGS);
    NT_ASSERT(mailbox->ArgCount == Session->ArgCount);
    NT_ASSERT(ResponseWords <= mailbox->ArgCount);

    status = KphpHwMailboxWait(Session, Deadline, &response);
    if (status == STATUS_IO_TIMEOUT)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Mailbox 0x%08lx busy before opcode 0x%02lx",
                      Session->Msg,
                      Opcode);

        status = STATUS_DEVICE_BUSY;
        goto Exit;
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwMailboxWait failed: %!STATUS!",
                      status);

        goto Exit;
    }

    restoreIdle = TRUE;

    status = KphpHwMailboxWrite(Session, KphpHwMailboxRsp, 0, 0);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwMailboxWrite failed: %!STATUS!",
                      status);

        goto Exit;
    }

    for (ULONG i = 0; i < mailbox->ArgCount; i++)
    {
        status = KphpHwMailboxWrite(Session, KphpHwMailboxArg, i, Args[i]);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpHwMailboxWrite failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }

    //
    // RSP is never restored once MSG may have been written.
    //
    restoreIdle = FALSE;

    status = KphpHwMailboxWrite(Session, KphpHwMailboxMsg, 0, Opcode);
    if ((status == STATUS_RETRY) && ClientCommand)
    {
        status = KphpHwMailboxWrite(Session, KphpHwMailboxMsg, 0, Opcode);
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwMailboxWrite failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpHwMailboxWait(Session, Deadline, &response);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Mailbox 0x%08lx opcode 0x%02lx: %!STATUS!",
                      Session->Msg,
                      Opcode,
                      status);

        goto Exit;
    }

    *Completed = TRUE;

    switch (response)
    {
        case KPH_HW_MAILBOX_RSP_OK:
        {
            break;
        }
        case KPH_HW_MAILBOX_RSP_BUSY:
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Mailbox 0x%08lx opcode 0x%02lx busy",
                          Session->Msg,
                          Opcode);

            status = STATUS_DEVICE_BUSY;
            goto Exit;
        }
        case KPH_HW_MAILBOX_RSP_PREREQ:
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Mailbox 0x%08lx opcode 0x%02lx prerequisite failed",
                          Session->Msg,
                          Opcode);

            status = STATUS_INTERNAL_ERROR;
            goto Exit;
        }
        case KPH_HW_MAILBOX_RSP_UNKNOWN:
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Mailbox 0x%08lx opcode 0x%02lx unknown",
                          Session->Msg,
                          Opcode);

            status = STATUS_NOT_IMPLEMENTED;
            goto Exit;
        }
        default:
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Mailbox 0x%08lx opcode 0x%02lx response 0x%08lx",
                          Session->Msg,
                          Opcode,
                          response);

            status = STATUS_UNSUCCESSFUL;
            goto Exit;
        }
    }

    for (ULONG i = 0; i < ResponseWords; i++)
    {
        status = KphpHwMailboxRead(Session, KphpHwMailboxArg, i, &Results[i]);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpHwMailboxRead failed: %!STATUS!",
                          status);

            RtlZeroMemory(Results, sizeof(ULONG) * KPH_HW_MAILBOX_ARGS);
            *Completed = FALSE;
            goto Exit;
        }
    }

    status = STATUS_SUCCESS;

Exit:

    if (restoreIdle)
    {
        KphpHwMailboxRestoreIdle(Session, response);
    }

    return status;
}

/**
 * \brief Composes the argument registers of a command.
 *
 * \param[in] Command The command row.
 * \param[in] Caller The caller's arguments; zero for a kernel-issued command.
 * \param[out] Args Receives (caller & mask) | fixed per argument.
 *
 * \return TRUE if every caller argument lies in the row's mask and maximum,
 * FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpHwMailboxComposeArgs(
    _In_ PKPH_DYN_HW_MAILBOX_COMMAND Command,
    _In_reads_(KPH_HW_MAILBOX_ARGS) PCULONG Caller,
    _Out_writes_(KPH_HW_MAILBOX_ARGS) PULONG Args
    )
{
    KPH_PAGED_CODE_PASSIVE();

    C_ASSERT(KPH_HW_MAILBOX_ARGS == KPH_DYN_HW_MAILBOX_MAX_ARGS);

    RtlZeroMemory(Args, sizeof(ULONG) * KPH_HW_MAILBOX_ARGS);

    for (ULONG i = 0; i < KPH_HW_MAILBOX_ARGS; i++)
    {
        if ((Caller[i] & ~Command->ArgMask[i]) ||
            (Caller[i] > Command->ArgMax[i]))
        {
            return FALSE;
        }

        Args[i] = (Caller[i] | Command->ArgFixed[i]);
    }

    return TRUE;
}

/**
 * \brief Finds the cached firmware version of a mailbox.
 *
 * \param[in] Session The mailbox session.
 * \param[in] Op The mailbox_transact op.
 *
 * \return The cache entry, NULL if none.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwMailboxLock)
_Must_inspect_result_
PKPH_HW_MAILBOX_FIRMWARE KphpHwMailboxFindFirmware(
    _In_ PKPH_HW_MAILBOX_SESSION Session,
    _In_ PKPH_DYN_HW_OP Op
    )
{
    PKPH_DYN_HW_MAILBOX_TRANSACT key;

    KPH_PAGED_CODE_PASSIVE();

    key = (PKPH_DYN_HW_MAILBOX_TRANSACT)Op->Body;

    NT_ASSERT(key->Transport == Session->Transport);

    for (ULONG i = 0; i < ARRAYSIZE(KphpHwMailboxFirmware); i++)
    {
        PKPH_HW_MAILBOX_FIRMWARE entry;
        PKPH_DYN_HW_MAILBOX_SMN smn;
        PKPH_DYN_HW_MAILBOX_SMN entrySmn;

        entry = &KphpHwMailboxFirmware[i];

        if (!entry->LastUse || (entry->Mailbox.Transport != key->Transport))
        {
            continue;
        }

        if (key->Transport == KPH_DYN_HW_MAILBOX_TRANSPORT_MMIO_WINDOW)
        {
            PKPH_DYN_HW_MAILBOX_WINDOW window;
            PKPH_DYN_HW_MAILBOX_WINDOW entryWindow;

            window = &key->Address.Window;
            entryWindow = &entry->Mailbox.Address.Window;

            if (RtlEqualString(&entry->Path, &Session->Mmio.Path, FALSE) &&
                (entry->Mailbox.ArgCount == key->ArgCount) &&
                (entry->Mailbox.VersionOpcode == key->VersionOpcode) &&
                (entry->Mailbox.VersionArg0 == key->VersionArg0) &&
                (entryWindow->BaseKind == window->BaseKind) &&
                (entryWindow->BarIndex == window->BarIndex) &&
                (entryWindow->WindowSize == window->WindowSize) &&
                (entryWindow->Msg == window->Msg) &&
                (entryWindow->Rsp == window->Rsp) &&
                (entryWindow->Arg0 == window->Arg0))
            {
                return entry;
            }

            continue;
        }

        smn = &key->Address.Smn;
        entrySmn = &entry->Mailbox.Address.Smn;

        if ((entry->Locator.Segment == Session->Smn.Locator.Segment) &&
            (entry->Locator.Bus == Session->Smn.Locator.Bus) &&
            (entry->Locator.Device == Session->Smn.Locator.Device) &&
            (entry->Locator.Function == Session->Smn.Locator.Function) &&
            (entry->Mailbox.ArgCount == key->ArgCount) &&
            (entry->Mailbox.VersionOpcode == key->VersionOpcode) &&
            (entry->Mailbox.VersionArg0 == key->VersionArg0) &&
            (entrySmn->IndexOffset == smn->IndexOffset) &&
            (entrySmn->DataOffset == smn->DataOffset) &&
            (entrySmn->Msg == smn->Msg) &&
            (entrySmn->Rsp == smn->Rsp) &&
            (entrySmn->Arg0 == smn->Arg0))
        {
            return entry;
        }
    }

    return NULL;
}

/**
 * \brief Retrieves the firmware version of a mailbox.
 *
 * \details Two agreeing reads are cached for the boot. Must run before any
 * other command.
 *
 * \param[in] Session The mailbox session.
 * \param[in] Op The mailbox_transact op.
 * \param[in] Deadline The deadline of the request.
 * \param[out] Version Receives the firmware version.
 *
 * \return Successful or errant status; STATUS_RETRY when the two reads differ.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwMailboxLock)
_Must_inspect_result_
NTSTATUS KphpHwMailboxQueryFirmware(
    _In_ PKPH_HW_MAILBOX_SESSION Session,
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG64 Deadline,
    _Out_ PULONG Version
    )
{
    NTSTATUS status;
    PKPH_DYN_HW_MAILBOX_TRANSACT mailbox;
    PKPH_HW_MAILBOX_FIRMWARE entry;
    ULONG args[KPH_HW_MAILBOX_ARGS];
    ULONG results[KPH_HW_MAILBOX_ARGS];
    ULONG versions[2];
    BOOLEAN completed;
    PCHAR path;

    KPH_PAGED_CODE_PASSIVE();

    *Version = 0;

    mailbox = (PKPH_DYN_HW_MAILBOX_TRANSACT)Op->Body;

    entry = KphpHwMailboxFindFirmware(Session, Op);
    if (entry)
    {
        entry->LastUse = ++KphpHwMailboxClock;
        *Version = entry->Version;
        return STATUS_SUCCESS;
    }

    RtlZeroMemory(args, sizeof(args));
    args[0] = mailbox->VersionArg0;

    for (ULONG i = 0; i < ARRAYSIZE(versions); i++)
    {
        status = KphpHwMailboxTransact(Session,
                                       Op,
                                       Deadline,
                                       mailbox->VersionOpcode,
                                       args,
                                       1,
                                       FALSE,
                                       results,
                                       &completed);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Mailbox 0x%08lx version command 0x%02lx: %!STATUS!",
                          Session->Msg,
                          mailbox->VersionOpcode,
                          status);

            return status;
        }

        if ((results[0] == 0) || (results[0] == MAXULONG))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Mailbox 0x%08lx firmware version 0x%08lx invalid",
                          Session->Msg,
                          results[0]);

            return STATUS_DEVICE_NOT_READY;
        }

        versions[i] = results[0];
    }

    if (versions[0] != versions[1])
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Mailbox 0x%08lx firmware reads 0x%08lx 0x%08lx differ",
                      Session->Msg,
                      versions[0],
                      versions[1]);

        return STATUS_RETRY;
    }

    *Version = versions[0];

    path = NULL;

    if (Session->Transport == KPH_DYN_HW_MAILBOX_TRANSPORT_MMIO_WINDOW)
    {
        path = KphAllocatePaged(Session->Mmio.Path.Length,
                                KPH_TAG_HW_MAILBOX_PATH);
        if (!path)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Failed to allocate mailbox firmware cache path");

            return STATUS_SUCCESS;
        }
    }

    entry = &KphpHwMailboxFirmware[0];

    for (ULONG i = 0; i < ARRAYSIZE(KphpHwMailboxFirmware); i++)
    {
        if (!KphpHwMailboxFirmware[i].LastUse)
        {
            entry = &KphpHwMailboxFirmware[i];
            break;
        }

        if (KphpHwMailboxFirmware[i].LastUse < entry->LastUse)
        {
            entry = &KphpHwMailboxFirmware[i];
        }
    }

    KphpHwMailboxFreeFirmware(entry);
    RtlCopyMemory(&entry->Mailbox,
                  mailbox,
                  sizeof(KPH_DYN_HW_MAILBOX_TRANSACT));
    entry->Mailbox.CommandCount = 0;
    entry->Version = versions[0];
    entry->LastUse = ++KphpHwMailboxClock;

    if (Session->Transport == KPH_DYN_HW_MAILBOX_TRANSPORT_MMIO_WINDOW)
    {
        RtlCopyMemory(path,
                      Session->Mmio.Path.Buffer,
                      Session->Mmio.Path.Length);
        entry->Path.Buffer = path;
        entry->Path.Length = Session->Mmio.Path.Length;
        entry->Path.MaximumLength = Session->Mmio.Path.Length;

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Mailbox 0x%08lx at %Z firmware 0x%08lx",
                      Session->Msg,
                      &Session->Mmio.Path,
                      versions[0]);
    }
    else
    {
        entry->Locator = Session->Smn.Locator;

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Mailbox 0x%08lx at %04lx:%02lx:%02lx.%lx "
                      "firmware 0x%08lx",
                      Session->Msg,
                      (ULONG)Session->Smn.Locator.Segment,
                      (ULONG)Session->Smn.Locator.Bus,
                      (ULONG)Session->Smn.Locator.Device,
                      (ULONG)Session->Smn.Locator.Function,
                      versions[0]);
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Prepares a client command of a batch.
 *
 * \param[in] Op The mailbox_transact op.
 * \param[in] Entry The entry.
 * \param[out] Command Receives the row the command runs through.
 * \param[out] Args Receives the argument register values.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwMailboxPrepareEntry(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ PKPH_HW_MAILBOX_ENTRY Entry,
    _Outptr_ PKPH_DYN_HW_MAILBOX_COMMAND* Command,
    _Out_writes_(KPH_HW_MAILBOX_ARGS) PULONG Args
    )
{
    NTSTATUS status;
    PKPH_DYN_HW_MAILBOX_COMMAND command;

    KPH_PAGED_CODE_PASSIVE();

    *Command = NULL;
    RtlZeroMemory(Args, sizeof(ULONG) * KPH_HW_MAILBOX_ARGS);

    if ((Entry->Opcode < 1) || (Entry->Opcode > KPH_DYN_HW_MAILBOX_MAX_OPCODE))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Mailbox opcode 0x%lx is out of range",
                      Entry->Opcode);

        return STATUS_INVALID_PARAMETER;
    }

    status = KphDynHwFindMailboxCommand(Op, Entry->Opcode, &command);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Mailbox opcode 0x%lx refused: %!STATUS!",
                      Entry->Opcode,
                      status);

        return STATUS_ACCESS_DENIED;
    }

    if (BooleanFlagOn(command->Flags, KPH_DYN_HW_MAILBOX_COMMAND_INTERNAL))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Mailbox opcode 0x%lx refused: internal",
                      Entry->Opcode);

        return STATUS_ACCESS_DENIED;
    }

    if (!KphpHwMailboxComposeArgs(command, Entry->Args, Args))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Mailbox opcode 0x%lx refused: arguments not allowed",
                      Entry->Opcode);

        return STATUS_ACCESS_DENIED;
    }

    *Command = command;

    return STATUS_SUCCESS;
}

/**
 * \brief Runs a batch of mailbox commands under the mailbox lock.
 *
 * \param[in] Session The open mailbox session.
 * \param[in] Op The mailbox_transact op.
 * \param[in] Deadline The deadline of the request.
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The captured entries.
 * \param[out] FirmwareVersion Receives the firmware version.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwMailboxLock)
_Must_inspect_result_
NTSTATUS KphpHwMailboxRunBatch(
    _In_ PKPH_HW_MAILBOX_SESSION Session,
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG64 Deadline,
    _In_ ULONG Count,
    _Inout_updates_opt_(Count) PKPH_HW_MAILBOX_ENTRY Entries,
    _Out_ PULONG FirmwareVersion
    )
{
    NTSTATUS status;
    ULONG version;
    NTSTATUS stopped;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(Entries || (Count == 0));

    *FirmwareVersion = 0;

    status = KphpHwMailboxQueryFirmware(Session, Op, Deadline, &version);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwMailboxQueryFirmware failed: %!STATUS!",
                      status);

        return status;
    }

    *FirmwareVersion = version;

    stopped = STATUS_SUCCESS;

    for (ULONG i = 0; i < Count; i++)
    {
        PKPH_HW_MAILBOX_ENTRY entry;
        PKPH_DYN_HW_MAILBOX_COMMAND command;
        ULONG args[KPH_HW_MAILBOX_ARGS];
        BOOLEAN completed;

        entry = &Entries[i];

        status = KphpHwMailboxPrepareEntry(Op, entry, &command, args);
        if (NT_SUCCESS(status))
        {
            if (NT_SUCCESS(stopped) && KphHwDeadlinePassed(Deadline))
            {
                stopped = STATUS_RETRY;
            }

            status = stopped;
        }

        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Mailbox entry %lu not run: %!STATUS!",
                          i,
                          status);

            entry->Status = status;
            RtlZeroMemory(entry->Args, sizeof(entry->Args));
            continue;
        }

        status = KphpHwMailboxTransact(Session,
                                       Op,
                                       Deadline,
                                       entry->Opcode,
                                       args,
                                       command->ResponseWords,
                                       TRUE,
                                       entry->Args,
                                       &completed);

        entry->Status = status;

        if (!completed)
        {
            stopped = STATUS_RETRY;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Runs a kernel-issued command of an SMU table read.
 *
 * \param[in] Session The mailbox session.
 * \param[in] Op The mailbox_transact op.
 * \param[in] Command The row of the command.
 * \param[in] Opcode The opcode.
 * \param[in] Deadline The deadline of the request.
 * \param[out] Results Receives the response words.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwMailboxLock)
_Must_inspect_result_
NTSTATUS KphpHwMailboxTableCommand(
    _In_ PKPH_HW_MAILBOX_SESSION Session,
    _In_ PKPH_DYN_HW_OP Op,
    _In_ PKPH_DYN_HW_MAILBOX_COMMAND Command,
    _In_ ULONG Opcode,
    _In_ ULONG64 Deadline,
    _Out_writes_(KPH_HW_MAILBOX_ARGS) PULONG Results
    )
{
    NTSTATUS status;
    ULONG zero[KPH_HW_MAILBOX_ARGS];
    ULONG args[KPH_HW_MAILBOX_ARGS];
    BOOLEAN completed;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(zero, sizeof(zero));

    NT_VERIFY(KphpHwMailboxComposeArgs(Command, zero, args));

    status = KphpHwMailboxTransact(Session,
                                   Op,
                                   Deadline,
                                   Opcode,
                                   args,
                                   Command->ResponseWords,
                                   FALSE,
                                   Results,
                                   &completed);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Mailbox 0x%08lx opcode 0x%02lx: %!STATUS!",
                      Session->Msg,
                      Opcode,
                      status);

        return status;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Reads the base of an SMU-owned table.
 *
 * \param[in] Session The mailbox session.
 * \param[in] Op The mailbox_transact op.
 * \param[in] Table The smu_table_read op body.
 * \param[in] SetupCommand The setup command (TWO_STEP), else NULL.
 * \param[in] BaseCommand The base command.
 * \param[in] Deadline The deadline of the request.
 * \param[out] Results Receives the base command's response words.
 * \param[out] Base Receives the base.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwMailboxLock)
_Must_inspect_result_
NTSTATUS KphpHwMailboxTableBase(
    _In_ PKPH_HW_MAILBOX_SESSION Session,
    _In_ PKPH_DYN_HW_OP Op,
    _In_ PKPH_DYN_HW_SMU_TABLE_READ Table,
    _In_opt_ PKPH_DYN_HW_MAILBOX_COMMAND SetupCommand,
    _In_ PKPH_DYN_HW_MAILBOX_COMMAND BaseCommand,
    _In_ ULONG64 Deadline,
    _Out_writes_(KPH_HW_MAILBOX_ARGS) PULONG Results,
    _Out_ PULONG64 Base
    )
{
    NTSTATUS status;

    KPH_PAGED_CODE_PASSIVE();

    *Base = 0;

    NT_ASSERT((SetupCommand != NULL) ==
              (Table->BaseShape == KPH_DYN_HW_SMU_TABLE_BASE_TWO_STEP));

    if (SetupCommand)
    {
        status = KphpHwMailboxTableCommand(Session,
                                           Op,
                                           SetupCommand,
                                           Table->SetupOpcode,
                                           Deadline,
                                           Results);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    status = KphpHwMailboxTableCommand(Session,
                                       Op,
                                       BaseCommand,
                                       Table->BaseOpcode,
                                       Deadline,
                                       Results);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (Table->BaseShape == KPH_DYN_HW_SMU_TABLE_BASE_TWO_STEP)
    {
        *Base = Results[0];
    }
    else
    {
        *Base = (Results[0] | ((ULONG64)Results[1] << 32));
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Reads an SMU-owned table under the mailbox lock.
 *
 * \param[in] Locator The locator of the root complex.
 * \param[in] Section The hardware section.
 * \param[in] OpIndex The index of the smu_table_read op of the root.
 * \param[in] Deadline The deadline of the request.
 * \param[in] Length The capacity of the caller's buffer.
 * \param[out] Buffer Receives the table copy, freed with KPH_TAG_HW_SMU_TABLE.
 * \param[out] Version Receives the table version, zero until read.
 * \param[out] Size Receives the table size, zero until known.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwMailboxLock)
_Must_inspect_result_
NTSTATUS KphpHwMailboxCopyTable(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ ULONG OpIndex,
    _In_ ULONG64 Deadline,
    _In_ ULONG Length,
    _Outptr_ PVOID* Buffer,
    _Out_ PULONG Version,
    _Out_ PULONG Size
    )
{
    NTSTATUS status;
    KPH_HW_MAILBOX_SESSION session;
    PKPH_DYN_HW_OP op;
    PKPH_DYN_HW_OP mailboxOp;
    PKPH_DYN_HW_SMU_TABLE_READ table;
    PKPH_DYN_HW_SMU_TABLE_VERSION versions;
    PKPH_DYN_HW_MAILBOX_COMMAND versionCommand;
    PKPH_DYN_HW_MAILBOX_COMMAND transferCommand;
    PKPH_DYN_HW_MAILBOX_COMMAND setupCommand;
    PKPH_DYN_HW_MAILBOX_COMMAND baseCommand;
    ULONG results[KPH_HW_MAILBOX_ARGS];
    ULONG check[KPH_HW_MAILBOX_ARGS];
    ULONG firmware;
    ULONG size;
    ULONG64 base;
    ULONG64 checkBase;
    ULONG64 end;
    ULONG64 rangeEnd;
    PHYSICAL_ADDRESS physical;
    PULONG mapping;
    PULONG copy;
    BOOLEAN changed;

    KPH_PAGED_CODE_PASSIVE();

    *Buffer = NULL;
    *Version = 0;
    *Size = 0;

    copy = NULL;
    transferCommand = NULL;
    setupCommand = NULL;

    status = KphpHwMailboxOpenSmn(Locator,
                                  Section,
                                  KPH_DYN_HW_KIND_SMU_TABLE_READ,
                                  OpIndex,
                                  &op,
                                  &mailboxOp,
                                  &session);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwMailboxOpenSmn failed: %!STATUS!",
                      status);

        return status;
    }

    table = (PKPH_DYN_HW_SMU_TABLE_READ)op->Body;
    versions = Add2Ptr(table, sizeof(KPH_DYN_HW_SMU_TABLE_READ));

    status = KphpHwMailboxQueryFirmware(&session,
                                        mailboxOp,
                                        Deadline,
                                        &firmware);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwMailboxQueryFirmware failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphDynHwFindMailboxCommand(mailboxOp,
                                        table->VersionOpcode,
                                        &versionCommand);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphDynHwFindMailboxCommand failed: %!STATUS!",
                      status);

        status = STATUS_ACCESS_DENIED;
        goto Exit;
    }

    status = KphDynHwFindMailboxCommand(mailboxOp,
                                        table->BaseOpcode,
                                        &baseCommand);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphDynHwFindMailboxCommand failed: %!STATUS!",
                      status);

        status = STATUS_ACCESS_DENIED;
        goto Exit;
    }

    if (table->TransferOpcode)
    {
        status = KphDynHwFindMailboxCommand(mailboxOp,
                                            table->TransferOpcode,
                                            &transferCommand);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphDynHwFindMailboxCommand failed: %!STATUS!",
                          status);

            status = STATUS_ACCESS_DENIED;
            goto Exit;
        }
    }

    if (table->BaseShape == KPH_DYN_HW_SMU_TABLE_BASE_TWO_STEP)
    {
        status = KphDynHwFindMailboxCommand(mailboxOp,
                                            table->SetupOpcode,
                                            &setupCommand);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphDynHwFindMailboxCommand failed: %!STATUS!",
                          status);

            status = STATUS_ACCESS_DENIED;
            goto Exit;
        }

        NT_ASSERT(BooleanFlagOn(setupCommand->Flags,
                                KPH_DYN_HW_MAILBOX_COMMAND_INTERNAL));
    }

    NT_ASSERT(versionCommand->ResponseWords >= 1);
    NT_ASSERT(baseCommand->ResponseWords >=
              ((table->BaseShape == KPH_DYN_HW_SMU_TABLE_BASE_HSMP) ? 3 :
               (table->BaseShape == KPH_DYN_HW_SMU_TABLE_BASE_TWO_STEP) ? 1 :
               2));
    NT_ASSERT(BooleanFlagOn(baseCommand->Flags,
                            KPH_DYN_HW_MAILBOX_COMMAND_INTERNAL));
    NT_ASSERT(!transferCommand ||
              BooleanFlagOn(transferCommand->Flags,
                            KPH_DYN_HW_MAILBOX_COMMAND_INTERNAL));

    status = KphpHwMailboxTableCommand(&session,
                                       mailboxOp,
                                       versionCommand,
                                       table->VersionOpcode,
                                       Deadline,
                                       results);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwMailboxTableCommand failed: %!STATUS!",
                      status);

        goto Exit;
    }

    *Version = results[0];

    size = 0;

    for (ULONG i = 0; i < table->VersionCount; i++)
    {
        if (versions[i].Version == results[0])
        {
            size = versions[i].Size;
            break;
        }
    }

    if (!size)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMU table version 0x%08lx not supported",
                      results[0]);

        status = STATUS_NOT_SUPPORTED;
        goto Exit;
    }

    *Size = size;

    if (Length < size)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMU table size %lu exceeds buffer length %lu",
                      size,
                      Length);

        status = STATUS_BUFFER_TOO_SMALL;
        goto Exit;
    }

    copy = KphAllocatePaged(size, KPH_TAG_HW_SMU_TABLE);
    if (!copy)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate SMU table buffer");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    if (setupCommand && transferCommand)
    {
        status = KphpHwMailboxTableCommand(&session,
                                           mailboxOp,
                                           setupCommand,
                                           table->SetupOpcode,
                                           Deadline,
                                           results);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpHwMailboxTableCommand failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }

    if (transferCommand)
    {
        status = KphpHwMailboxTableCommand(&session,
                                           mailboxOp,
                                           transferCommand,
                                           table->TransferOpcode,
                                           Deadline,
                                           results);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpHwMailboxTableCommand failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }

    status = KphpHwMailboxTableBase(&session,
                                    mailboxOp,
                                    table,
                                    setupCommand,
                                    baseCommand,
                                    Deadline,
                                    results,
                                    &base);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwMailboxTableBase failed: %!STATUS!",
                      status);

        goto Exit;
    }

    //
    // Nothing is mapped before two base reads agree.
    //
    status = KphpHwMailboxTableBase(&session,
                                    mailboxOp,
                                    table,
                                    setupCommand,
                                    baseCommand,
                                    Deadline,
                                    check,
                                    &checkBase);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwMailboxTableBase failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (checkBase != base)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMU table base 0x%llx moved",
                      base);

        status = STATUS_RETRY;
        goto Exit;
    }

    //
    // HSMP reports the table size; never read past it.
    //
    if ((table->BaseShape == KPH_DYN_HW_SMU_TABLE_BASE_HSMP) &&
        results[2] &&
        (results[2] < size))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMU table reported size %lu below %lu",
                      results[2],
                      size);

        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Exit;
    }

    if (!base)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "SMU table base is zero");

        status = STATUS_DEVICE_NOT_READY;
        goto Exit;
    }

    status = RtlULongLongAdd(base, size, &end);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongLongAdd failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (BYTE_OFFSET(base) || (end > (MAXULONG64 - PAGE_SIZE)))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMU table range 0x%llx-0x%llx invalid",
                      base,
                      end);

        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Exit;
    }

    rangeEnd = ROUND_TO_PAGES(end);

    status = KphHwWindowCheckRange(base, rangeEnd);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMU table version 0x%08lx refused: %!STATUS!",
                      *Version,
                      status);

        goto Exit;
    }

    physical.QuadPart = (LONGLONG)base;

    mapping = MmMapIoSpaceEx(physical, size, PAGE_READONLY | PAGE_NOCACHE);
    if (!mapping)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "MmMapIoSpaceEx failed");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    for (ULONG i = 0; i < (size / sizeof(ULONG)); i++)
    {
        copy[i] = READ_REGISTER_ULONG(&mapping[i]);
    }

    //
    // A concurrent transfer changes the first dword (best effort).
    //
    changed = (READ_REGISTER_ULONG(&mapping[0]) != copy[0]);

    MmUnmapIoSpace(mapping, size);

    if (changed)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMU table changed during the copy");

        status = STATUS_RETRY;
        goto Exit;
    }

    status = KphpHwMailboxTableBase(&session,
                                    mailboxOp,
                                    table,
                                    setupCommand,
                                    baseCommand,
                                    Deadline,
                                    check,
                                    &checkBase);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwMailboxTableBase failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (checkBase != base)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMU table base 0x%llx moved",
                      base);

        status = STATUS_RETRY;
        goto Exit;
    }

    *Buffer = copy;
    copy = NULL;

    status = STATUS_SUCCESS;

Exit:

    KphpHwMailboxClose(&session);

    if (copy)
    {
        KphFree(copy, KPH_TAG_HW_SMU_TABLE);
    }

    return status;
}

#endif

/**
 * \brief Runs a batch of read-type commands on a firmware mailbox.
 *
 * \param[in] Locator The locator of the root complex.
 * \param[in] OpIndex The index of the mailbox_transact op of the root.
 * \param[in] Count The number of entries; zero reads the firmware version
 * only.
 * \param[in,out] Entries The batch entries.
 * \param[out] FirmwareVersion Optionally receives the firmware version.
 * \param[in] Timeout Optional timeout for the request; NULL is infinite.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwMailboxBatch(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ ULONG OpIndex,
    _In_ ULONG Count,
    _Inout_updates_opt_(Count) PKPH_HW_MAILBOX_ENTRY Entries,
    _Out_opt_ PULONG FirmwareVersion,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
#if defined(_M_X64)
    NTSTATUS status;
    KPH_HW_PCI_LOCATOR locator;
    ULONG entriesLength;
    PKPH_HW_MAILBOX_ENTRY entries;
    PKPH_DYN dyn;
    KPH_HW_MAILBOX_SESSION session;
    PKPH_DYN_HW_OP op;
    PKPH_DYN_HW_OP mailboxOp;
    ULONG version;
    ULONG64 deadline;
    BOOLEAN locked;
    BOOLEAN sessionOpen;

    KPH_PAGED_CODE_PASSIVE();

    entries = NULL;
    dyn = NULL;
    version = 0;
    locked = FALSE;
    sessionOpen = FALSE;

    locator = *Locator;

    if (Count && !Entries)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Mailbox entries missing for count %lu",
                      Count);

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

    status = RtlULongMult(Count, sizeof(KPH_HW_MAILBOX_ENTRY), &entriesLength);
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
        entries = KphAllocatePaged(entriesLength, KPH_TAG_HW_MAILBOX_BATCH);
        if (!entries)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Failed to allocate mailbox batch buffer");

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        status = KphCopyFromMode(entries, Entries, entriesLength, AccessMode);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    KphAcquireRWLockExclusive(&KphpHwMailboxLock);
    locked = TRUE;

    status = KphpHwMailboxOpenSmn(&locator,
                                  dyn->Hardware.Section,
                                  KPH_DYN_HW_KIND_MAILBOX_TRANSACT,
                                  OpIndex,
                                  &op,
                                  &mailboxOp,
                                  &session);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwMailboxOpenSmn failed: %!STATUS!",
                      status);

        goto Exit;
    }

    sessionOpen = TRUE;

    NT_ASSERT(op == mailboxOp);

    status = KphpHwMailboxRunBatch(&session,
                                   op,
                                   deadline,
                                   Count,
                                   entries,
                                   &version);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwMailboxRunBatch failed: %!STATUS!",
                      status);

        goto Exit;
    }

    KphpHwMailboxClose(&session);
    sessionOpen = FALSE;

    KphReleaseRWLock(&KphpHwMailboxLock);
    locked = FALSE;

    if (Count)
    {
        status = KphCopyToMode(Entries, entries, entriesLength, AccessMode);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    if (FirmwareVersion)
    {
        status = KphWriteULongToMode(FirmwareVersion, version, AccessMode);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    status = STATUS_SUCCESS;

Exit:

    if (sessionOpen)
    {
        KphpHwMailboxClose(&session);
    }

    if (locked)
    {
        KphReleaseRWLock(&KphpHwMailboxLock);
    }

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    if (entries)
    {
        KphFree(entries, KPH_TAG_HW_MAILBOX_BATCH);
    }

    return status;
#else
    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Locator);
    UNREFERENCED_PARAMETER(OpIndex);
    UNREFERENCED_PARAMETER(Count);
    UNREFERENCED_PARAMETER(Entries);
    UNREFERENCED_PARAMETER(FirmwareVersion);
    UNREFERENCED_PARAMETER(Timeout);
    UNREFERENCED_PARAMETER(AccessMode);

    return STATUS_NOT_SUPPORTED;
#endif
}

/**
 * \brief Runs a batch of read-type commands on the MMIO mailbox of an ACPI
 * device.
 *
 * \param[in] Path The namespace path of the device.
 * \param[in] OpIndex The index of the mailbox_transact op of the device.
 * \param[in] Count The number of entries; zero reads the firmware version
 * only.
 * \param[in,out] Entries The batch entries.
 * \param[out] FirmwareVersion Optionally receives the firmware version.
 * \param[in] Timeout Optional timeout for the request; NULL is infinite.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwMailboxBatchAcpi(
    _In_ PANSI_STRING Path,
    _In_ ULONG OpIndex,
    _In_ ULONG Count,
    _Inout_updates_opt_(Count) PKPH_HW_MAILBOX_ENTRY Entries,
    _Out_opt_ PULONG FirmwareVersion,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
#if defined(_M_X64)
    NTSTATUS status;
    PANSI_STRING path;
    ULONG entriesLength;
    PKPH_HW_MAILBOX_ENTRY entries;
    PKPH_DYN dyn;
    KPH_HW_MAILBOX_SESSION session;
    PKPH_DYN_HW_OP op;
    ULONG version;
    ULONG64 deadline;
    BOOLEAN locked;
    BOOLEAN sessionOpen;

    KPH_PAGED_CODE_PASSIVE();

    path = NULL;
    entries = NULL;
    dyn = NULL;
    version = 0;
    locked = FALSE;
    sessionOpen = FALSE;

    if (Count && !Entries)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Mailbox entries missing for count %lu",
                      Count);

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

    status = KphHwAcpiCapturePath(Path, &path, AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwAcpiCapturePath failed: %!STATUS!",
                      status);

        goto Exit;
    }

    dyn = KphReferenceDynData();
    if (!dyn || !dyn->Hardware.Section)
    {
        status = STATUS_NOINTERFACE;
        goto Exit;
    }

    status = RtlULongMult(Count, sizeof(KPH_HW_MAILBOX_ENTRY), &entriesLength);
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
        entries = KphAllocatePaged(entriesLength, KPH_TAG_HW_MAILBOX_BATCH);
        if (!entries)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Failed to allocate mailbox batch buffer");

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        status = KphCopyFromMode(entries, Entries, entriesLength, AccessMode);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    KphAcquireRWLockExclusive(&KphpHwMailboxLock);
    locked = TRUE;

    status = KphpHwMailboxOpenMmio(path,
                                   dyn->Hardware.Section,
                                   OpIndex,
                                   &op,
                                   &session);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwMailboxOpenMmio failed: %!STATUS!",
                      status);

        goto Exit;
    }

    sessionOpen = TRUE;

    status = KphpHwMailboxRunBatch(&session,
                                   op,
                                   deadline,
                                   Count,
                                   entries,
                                   &version);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwMailboxRunBatch failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (KphHwAcpiMailboxMoved(&session.Mmio, op))
    {
        PKPH_HW_MAILBOX_FIRMWARE firmware;

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Mailbox 0x%08lx at %Z moved during the request",
                      session.Msg,
                      &session.Mmio.Path);

        firmware = KphpHwMailboxFindFirmware(&session, op);
        if (firmware)
        {
            KphpHwMailboxFreeFirmware(firmware);
        }

        status = STATUS_DEVICE_NOT_READY;
        goto Exit;
    }

    KphpHwMailboxClose(&session);
    sessionOpen = FALSE;

    KphReleaseRWLock(&KphpHwMailboxLock);
    locked = FALSE;

    if (Count)
    {
        status = KphCopyToMode(Entries, entries, entriesLength, AccessMode);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    if (FirmwareVersion)
    {
        status = KphWriteULongToMode(FirmwareVersion, version, AccessMode);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    status = STATUS_SUCCESS;

Exit:

    if (sessionOpen)
    {
        KphpHwMailboxClose(&session);
    }

    if (locked)
    {
        KphReleaseRWLock(&KphpHwMailboxLock);
    }

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    if (entries)
    {
        KphFree(entries, KPH_TAG_HW_MAILBOX_BATCH);
    }

    if (path)
    {
        KphReleaseAnsiString(path);
    }

    return status;
#else
    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Path);
    UNREFERENCED_PARAMETER(OpIndex);
    UNREFERENCED_PARAMETER(Count);
    UNREFERENCED_PARAMETER(Entries);
    UNREFERENCED_PARAMETER(FirmwareVersion);
    UNREFERENCED_PARAMETER(Timeout);
    UNREFERENCED_PARAMETER(AccessMode);

    return STATUS_NOT_SUPPORTED;
#endif
}

/**
 * \brief Copies out an SMU-owned table.
 *
 * \param[in] Locator The locator of the root complex.
 * \param[in] OpIndex The index of the smu_table_read op of the root.
 * \param[in] Length The capacity of Buffer.
 * \param[out] Buffer Receives the table.
 * \param[out] Version Optionally receives the table version, zero until read.
 * \param[out] ReturnLength Optionally receives the table size, zero until
 * known.
 * \param[in] Timeout Optional timeout for the request; NULL is infinite.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status; STATUS_BUFFER_TOO_SMALL when the table
 * does not fit, STATUS_NOT_SUPPORTED when its version is not signed.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadSmuTable(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ ULONG OpIndex,
    _In_ ULONG Length,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _Out_opt_ PULONG Version,
    _Out_opt_ PULONG ReturnLength,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
#if defined(_M_X64)
    NTSTATUS status;
    NTSTATUS copyStatus;
    KPH_HW_PCI_LOCATOR locator;
    PKPH_DYN dyn;
    PVOID copy;
    ULONG version;
    ULONG size;
    ULONG64 deadline;

    KPH_PAGED_CODE_PASSIVE();

    dyn = NULL;
    copy = NULL;
    version = 0;
    size = 0;

    locator = *Locator;

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

    KphAcquireRWLockExclusive(&KphpHwMailboxLock);

    copyStatus = KphpHwMailboxCopyTable(&locator,
                                        dyn->Hardware.Section,
                                        OpIndex,
                                        deadline,
                                        Length,
                                        &copy,
                                        &version,
                                        &size);

    KphReleaseRWLock(&KphpHwMailboxLock);

    if (NT_SUCCESS(copyStatus))
    {
        NT_ASSERT(copy && (size <= Length));

        status = KphCopyToMode(Buffer, copy, size, AccessMode);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    if (Version)
    {
        status = KphWriteULongToMode(Version, version, AccessMode);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    if (ReturnLength)
    {
        status = KphWriteULongToMode(ReturnLength, size, AccessMode);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    if (!NT_SUCCESS(copyStatus))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwMailboxCopyTable failed: %!STATUS!",
                      copyStatus);

        status = copyStatus;
        goto Exit;
    }

    status = STATUS_SUCCESS;

Exit:

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    if (copy)
    {
        KphFree(copy, KPH_TAG_HW_SMU_TABLE);
    }

    return status;
#else
    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Locator);
    UNREFERENCED_PARAMETER(OpIndex);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(Version);
    UNREFERENCED_PARAMETER(ReturnLength);
    UNREFERENCED_PARAMETER(Timeout);
    UNREFERENCED_PARAMETER(AccessMode);

    return STATUS_NOT_SUPPORTED;
#endif
}
