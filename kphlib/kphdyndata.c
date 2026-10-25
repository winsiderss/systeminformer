/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2023-2026
 *
 */

#include <kphlibbase.h>
#include <kphdyndata.h>

/**
 * \brief Searches for a dynamic data configuration.
 *
 * \param[in] Config The dynamic data configuration.
 * \param[in] Length The length of the dynamic data configuration.
 * \param[in] Class The class to search for.
 * \param[in] Machine The machine to search for.
 * \param[in] TimeDateStamp The time date stamp to search for.
 * \param[in] SizeOfImage The size of image to search for.
 * \param[out] Data Receives a pointer into the dynamic data configuration if
 * the requested data is found.
 * \param[in] FieldsLength The expected length, in bytes, of the fields
 * structure for the requested class.
 * \param[out] Fields Receives a pointer into the dynamic data configuration for
 * the related fields if the requested data is found.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphDynDataLookup(
    _In_reads_bytes_(Length) PKPH_DYN_CONFIG Config,
    _In_ ULONG Length,
    _In_ USHORT Class,
    _In_ USHORT Machine,
    _In_ ULONG TimeDateStamp,
    _In_ ULONG SizeOfImage,
    _Outptr_opt_ PKPH_DYN_DATA* Data,
    _In_ ULONG FieldsLength,
    _Outptr_opt_result_buffer_(FieldsLength) PVOID* Fields
    )
{
    NTSTATUS status;
    ULONG headerLength;
    ULONG dataLength;
    ULONG length;

    if (Data)
    {
        *Data = NULL;
    }

    if (Fields)
    {
        *Fields = NULL;
    }

    headerLength = RTL_SIZEOF_THROUGH_FIELD(KPH_DYN_CONFIG, HardwareLength);

    status = RtlULongSub(Length, headerLength, &length);
    if (!NT_SUCCESS(status))
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    if (Config->Version != KPH_DYN_CONFIGURATION_VERSION)
    {
        return STATUS_SI_DYNDATA_VERSION_MISMATCH;
    }

    status = RtlULongSub(length, Config->HardwareLength, &length);
    if (!NT_SUCCESS(status))
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    status = RtlULongMult(sizeof(KPH_DYN_DATA), Config->Count, &dataLength);
    if (!NT_SUCCESS(status))
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    status = RtlULongSub(length, dataLength, &length);
    if (!NT_SUCCESS(status))
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    for (ULONG i = 0; i < Config->Count; i++)
    {
        PKPH_DYN_DATA data;

        data = &Config->Data[i];

        if ((data->Class != Class) ||
            (data->Machine != Machine) ||
            (data->TimeDateStamp != TimeDateStamp) ||
            (data->SizeOfImage != SizeOfImage))
        {
            continue;
        }

        if (Fields)
        {
            PVOID start;
            ULONG end;

            status = RtlULongAdd(data->Offset, FieldsLength, &end);
            if (!NT_SUCCESS(status) || (end > length))
            {
                return STATUS_SI_DYNDATA_INVALID_LENGTH;
            }

            start = &Config->Data[Config->Count];

            *Fields = Add2Ptr(start, data->Offset);
        }

        if (Data)
        {
            *Data = data;
        }

        return STATUS_SUCCESS;
    }

    return STATUS_SI_DYNDATA_UNSUPPORTED_KERNEL;
}

/**
 * \brief Retrieves the length of the identity of an identity kind.
 *
 * \param[in] IdentityKind The kind of identity (KPH_DYN_HW_IDENTITY_*).
 *
 * \return The length of the identity, zero for an unknown kind.
 */
ULONG KphpDynHwIdentityLength(
    _In_ UCHAR IdentityKind
    )
{
    switch (IdentityKind)
    {
        case KPH_DYN_HW_IDENTITY_CPU:
        {
            return sizeof(KPH_DYN_HW_CPU_IDENTITY);
        }
        case KPH_DYN_HW_IDENTITY_PCI:
        {
            return sizeof(KPH_DYN_HW_PCI_IDENTITY);
        }
        case KPH_DYN_HW_IDENTITY_CPU_AND_PCI:
        {
            return sizeof(KPH_DYN_HW_CPU_AND_PCI_IDENTITY);
        }
        case KPH_DYN_HW_IDENTITY_CPU_AND_PCI_CLASS:
        {
            return sizeof(KPH_DYN_HW_CPU_AND_PCI_CLASS_IDENTITY);
        }
        case KPH_DYN_HW_IDENTITY_SUPERIO:
        {
            return sizeof(KPH_DYN_HW_SUPERIO_IDENTITY);
        }
        case KPH_DYN_HW_IDENTITY_BOARD:
        {
            return sizeof(KPH_DYN_HW_BOARD_IDENTITY);
        }
        case KPH_DYN_HW_IDENTITY_ACPI_HID:
        {
            return sizeof(KPH_DYN_HW_ACPI_HID_IDENTITY);
        }
        case KPH_DYN_HW_IDENTITY_ARM_CPU:
        {
            return sizeof(KPH_DYN_HW_ARM_CPU_IDENTITY);
        }
        default:
        {
            return 0;
        }
    }
}

/**
 * \brief Checks that an ACPI_HID identity is well formed.
 *
 * \param[in] Id The identity characters.
 *
 * \return TRUE if the first 7 or 8 characters are [A-Z0-9_] and the rest zero.
 */
_Must_inspect_result_
BOOLEAN KphDynHwAcpiIdValid(
    _In_reads_(KPH_DYN_HW_ACPI_ID_LENGTH) PCCH Id
    )
{
    ULONG length;

    for (length = 0; length < KPH_DYN_HW_ACPI_ID_LENGTH; length++)
    {
        CHAR character;

        character = Id[length];

        if (character == ANSI_NULL)
        {
            break;
        }

        if (((character < 'A') || (character > 'Z')) &&
            ((character < '0') || (character > '9')) &&
            (character != '_'))
        {
            return FALSE;
        }
    }

    if ((length != 7) && (length != 8))
    {
        return FALSE;
    }

    for (ULONG i = length; i < KPH_DYN_HW_ACPI_ID_LENGTH; i++)
    {
        if (Id[i] != ANSI_NULL)
        {
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * \brief Retrieves the length of the fixed part of the body of an op kind.
 *
 * \param[in] Kind The kind of op (KPH_DYN_HW_KIND_*).
 *
 * \return The length of the fixed part of the body, zero for an unknown kind.
 */
ULONG KphpDynHwOpFixedLength(
    _In_ USHORT Kind
    )
{
    switch (Kind)
    {
        case KPH_DYN_HW_KIND_MSR_READ:
        {
            return sizeof(KPH_DYN_HW_MSR_READ);
        }
        case KPH_DYN_HW_KIND_PCI_READ:
        {
            return FIELD_OFFSET(KPH_DYN_HW_PCI_READ, Ranges);
        }
        case KPH_DYN_HW_KIND_PCI_INDIRECT_READ:
        {
            return sizeof(KPH_DYN_HW_PCI_INDIRECT_READ);
        }
        case KPH_DYN_HW_KIND_WINDOW_READ:
        {
            return sizeof(KPH_DYN_HW_WINDOW_READ);
        }
        case KPH_DYN_HW_KIND_SUPERIO_READ:
        {
            return sizeof(KPH_DYN_HW_SUPERIO_READ);
        }
        case KPH_DYN_HW_KIND_SMBUS_READ:
        {
            return sizeof(KPH_DYN_HW_SMBUS_READ);
        }
        case KPH_DYN_HW_KIND_VSEC_READ:
        {
            return sizeof(KPH_DYN_HW_VSEC_READ);
        }
        case KPH_DYN_HW_KIND_MAILBOX_TRANSACT:
        {
            return sizeof(KPH_DYN_HW_MAILBOX_TRANSACT);
        }
        case KPH_DYN_HW_KIND_SMU_TABLE_READ:
        {
            return sizeof(KPH_DYN_HW_SMU_TABLE_READ);
        }
        case KPH_DYN_HW_KIND_ACPI_EVAL:
        {
            return sizeof(KPH_DYN_HW_ACPI_EVAL);
        }
        case KPH_DYN_HW_KIND_SYSREG_READ:
        {
            return sizeof(KPH_DYN_HW_SYSREG_READ);
        }
        case KPH_DYN_HW_KIND_PORT_INDIRECT_READ:
        {
            return sizeof(KPH_DYN_HW_PORT_INDIRECT_READ);
        }
        case KPH_DYN_HW_KIND_SMBUS_HOST:
        {
            return sizeof(KPH_DYN_HW_SMBUS_HOST);
        }
        default:
        {
            return 0;
        }
    }
}

/**
 * \brief Computes the length of an array in the body of an op.
 *
 * \param[in] Count The number of elements.
 * \param[in] ElementSize The size of an element.
 *
 * \return The length of the array.
 */
ULONG KphpDynHwArrayLength(
    _In_ UCHAR Count,
    _In_ ULONG ElementSize
    )
{
    ULONG length;

    NT_VERIFY(NT_SUCCESS(RtlULongMult(Count, ElementSize, &length)));

    return length;
}

/**
 * \brief Checks that a span ends within a limit.
 *
 * \param[in] Offset The offset of the span.
 * \param[in] Length The length of the span.
 * \param[in] Limit The limit the span must end at or below.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphpDynHwValidateSpan(
    _In_ ULONG Offset,
    _In_ ULONG Length,
    _In_ ULONG Limit
    )
{
    NTSTATUS status;
    ULONG end;

    status = RtlULongAdd(Offset, Length, &end);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (end > Limit)
    {
        return STATUS_SI_DYNDATA_HW_INVALID;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Validates the structure of a dynamic data hardware op.
 *
 * \param[in] Op The op to validate.
 * \param[in] Length The number of bytes remaining in the op table at the op.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphpDynHwValidateOp(
    _In_reads_bytes_(Length) PKPH_DYN_HW_OP Op,
    _In_ ULONG Length
    )
{
    NTSTATUS status;
    ULONG bodyLength;
    ULONG fixedLength;
    ULONG arrayLength;

    if ((Length < (ULONG)FIELD_OFFSET(KPH_DYN_HW_OP, Body)) ||
        (Op->Length > Length))
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    status = RtlULongSub(Op->Length,
                         FIELD_OFFSET(KPH_DYN_HW_OP, Body),
                         &bodyLength);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    fixedLength = KphpDynHwOpFixedLength(Op->Kind);
    if (fixedLength == 0)
    {
        return STATUS_SI_DYNDATA_HW_INVALID;
    }

    if (bodyLength < fixedLength)
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    switch (Op->Kind)
    {
        case KPH_DYN_HW_KIND_PCI_READ:
        {
            PKPH_DYN_HW_PCI_READ read;

            read = (PKPH_DYN_HW_PCI_READ)Op->Body;
            arrayLength = KphpDynHwArrayLength(read->RangeCount,
                                               sizeof(KPH_DYN_HW_RANGE));
            break;
        }
        case KPH_DYN_HW_KIND_PCI_INDIRECT_READ:
        {
            PKPH_DYN_HW_PCI_INDIRECT_READ indirect;
            ULONG valuesLength;

            indirect = (PKPH_DYN_HW_PCI_INDIRECT_READ)Op->Body;
            arrayLength = KphpDynHwArrayLength(indirect->RangeCount,
                                               sizeof(KPH_DYN_HW_TARGET_RANGE));
            valuesLength = KphpDynHwArrayLength(indirect->ValueCount,
                                                sizeof(ULONG));
            NT_VERIFY(NT_SUCCESS(RtlULongAdd(arrayLength,
                                             valuesLength,
                                             &arrayLength)));
            break;
        }
        case KPH_DYN_HW_KIND_WINDOW_READ:
        {
            PKPH_DYN_HW_WINDOW_READ window;
            ULONG auxLength;

            window = (PKPH_DYN_HW_WINDOW_READ)Op->Body;
            arrayLength = KphpDynHwArrayLength(window->RangeCount,
                                               sizeof(KPH_DYN_HW_WINDOW_RANGE));
            auxLength = KphpDynHwArrayLength(window->AuxCount, sizeof(ULONG));
            NT_VERIFY(NT_SUCCESS(RtlULongAdd(arrayLength,
                                             auxLength,
                                             &arrayLength)));
            break;
        }
        case KPH_DYN_HW_KIND_SUPERIO_READ:
        {
            PKPH_DYN_HW_SUPERIO_READ superIo;

            superIo = (PKPH_DYN_HW_SUPERIO_READ)Op->Body;
            arrayLength = KphpDynHwArrayLength(superIo->RangeCount,
                                               sizeof(KPH_DYN_HW_RANGE));
            break;
        }
        case KPH_DYN_HW_KIND_SMBUS_READ:
        {
            PKPH_DYN_HW_SMBUS_READ smbus;

            smbus = (PKPH_DYN_HW_SMBUS_READ)Op->Body;
            arrayLength = KphpDynHwArrayLength(smbus->RangeCount,
                                               sizeof(KPH_DYN_HW_RANGE));
            break;
        }
        case KPH_DYN_HW_KIND_VSEC_READ:
        {
            PKPH_DYN_HW_VSEC_READ vsec;

            vsec = (PKPH_DYN_HW_VSEC_READ)Op->Body;
            arrayLength = KphpDynHwArrayLength(vsec->RangeCount,
                                               sizeof(KPH_DYN_HW_WINDOW_RANGE));
            break;
        }
        case KPH_DYN_HW_KIND_MAILBOX_TRANSACT:
        {
            PKPH_DYN_HW_MAILBOX_TRANSACT mailbox;
            ULONG commandSize;

            mailbox = (PKPH_DYN_HW_MAILBOX_TRANSACT)Op->Body;
            commandSize = sizeof(KPH_DYN_HW_MAILBOX_COMMAND);
            arrayLength = KphpDynHwArrayLength(mailbox->CommandCount,
                                               commandSize);
            break;
        }
        case KPH_DYN_HW_KIND_SMU_TABLE_READ:
        {
            PKPH_DYN_HW_SMU_TABLE_READ table;
            ULONG versionSize;

            table = (PKPH_DYN_HW_SMU_TABLE_READ)Op->Body;
            versionSize = sizeof(KPH_DYN_HW_SMU_TABLE_VERSION);
            arrayLength = KphpDynHwArrayLength(table->VersionCount,
                                               versionSize);
            break;
        }
        case KPH_DYN_HW_KIND_ACPI_EVAL:
        {
            PKPH_DYN_HW_ACPI_EVAL eval;

            eval = (PKPH_DYN_HW_ACPI_EVAL)Op->Body;
            arrayLength = KphpDynHwArrayLength(eval->ArgCount,
                                               sizeof(KPH_DYN_HW_ACPI_ARG));
            break;
        }
        case KPH_DYN_HW_KIND_PORT_INDIRECT_READ:
        {
            PKPH_DYN_HW_PORT_INDIRECT_READ port;

            port = (PKPH_DYN_HW_PORT_INDIRECT_READ)Op->Body;
            arrayLength = KphpDynHwArrayLength(port->RangeCount,
                                               sizeof(KPH_DYN_HW_RANGE));
            break;
        }
        default:
        {
            arrayLength = 0;
            break;
        }
    }

    if (arrayLength > (bodyLength - fixedLength))
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Validates the structure of a dynamic data hardware op table.
 *
 * \param[in] Table The op table to validate.
 * \param[in] Length The length of the op table.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphpDynHwValidateTable(
    _In_reads_bytes_(Length) PKPH_DYN_HW_TABLE Table,
    _In_ ULONG Length
    )
{
    NTSTATUS status;
    PKPH_DYN_HW_OP op;
    ULONG remaining;

    status = RtlULongSub(Length, sizeof(KPH_DYN_HW_TABLE), &remaining);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    op = Add2Ptr(Table, sizeof(KPH_DYN_HW_TABLE));

    for (ULONG i = 0; i < Table->Count; i++)
    {
        status = KphpDynHwValidateOp(op, remaining);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        NT_VERIFY(NT_SUCCESS(RtlULongSub(remaining, op->Length, &remaining)));

        op = Add2Ptr(op, op->Length);
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Validates the structure of a dynamic data hardware section.
 *
 * \param[in] Section The hardware section to validate.
 * \param[in] Length The length of the hardware section.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphpDynHwValidateSection(
    _In_reads_bytes_(Length) PKPH_DYN_HW_SECTION Section,
    _In_ ULONG Length
    )
{
    NTSTATUS status;
    PKPH_DYN_HW_INDEX index;
    PVOID body;
    ULONG bodyOffset;
    ULONG bodyLength;

    if (Length < sizeof(KPH_DYN_HW_SECTION))
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    if (Section->Magic != KPH_DYN_HW_SECTION_MAGIC)
    {
        return STATUS_SI_DYNDATA_HW_INVALID;
    }

    if (Section->Length != Length)
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    status = RtlULongMult(sizeof(KPH_DYN_HW_INDEX),
                          Section->Count,
                          &bodyOffset);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = RtlULongAdd(sizeof(KPH_DYN_HW_SECTION), bodyOffset, &bodyOffset);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = RtlULongSub(Length, bodyOffset, &bodyLength);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    index = Add2Ptr(Section, sizeof(KPH_DYN_HW_SECTION));
    body = Add2Ptr(Section, bodyOffset);

    for (ULONG i = 0; i < Section->Count; i++)
    {
        PKPH_DYN_HW_TABLE table;

        status = KphpDynHwValidateSpan(index[i].Offset,
                                       index[i].Length,
                                       bodyLength);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        table = Add2Ptr(body, index[i].Offset);

        status = KphpDynHwValidateTable(table, index[i].Length);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Locates and validates the hardware section of a configuration.
 *
 * \param[in] Config The dynamic data configuration.
 * \param[in] Length The length of the dynamic data configuration.
 * \param[out] Section Receives a pointer into the dynamic data configuration
 * for the hardware section.
 * \param[out] SectionLength Receives the length of the hardware section.
 *
 * \return Successful or errant status, STATUS_NOT_FOUND when the
 * configuration has no hardware section.
 */
_Must_inspect_result_
NTSTATUS KphDynDataLookupHardware(
    _In_reads_bytes_(Length) PKPH_DYN_CONFIG Config,
    _In_ ULONG Length,
    _Outptr_result_maybenull_ PKPH_DYN_HW_SECTION* Section,
    _Out_ PULONG SectionLength
    )
{
    NTSTATUS status;
    PKPH_DYN_HW_SECTION section;
    ULONG dataEnd;
    ULONG end;

    *Section = NULL;
    *SectionLength = 0;

    if (Length < RTL_SIZEOF_THROUGH_FIELD(KPH_DYN_CONFIG, HardwareLength))
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    if (Config->Version != KPH_DYN_CONFIGURATION_VERSION)
    {
        return STATUS_SI_DYNDATA_VERSION_MISMATCH;
    }

    if ((Config->HardwareOffset == 0) && (Config->HardwareLength == 0))
    {
        return STATUS_NOT_FOUND;
    }

    if ((Config->HardwareOffset == 0) || (Config->HardwareLength == 0))
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    status = RtlULongMult(sizeof(KPH_DYN_DATA), Config->Count, &dataEnd);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = RtlULongAdd(FIELD_OFFSET(KPH_DYN_CONFIG, Data), dataEnd, &dataEnd);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (Config->HardwareOffset < dataEnd)
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    status = RtlULongAdd(Config->HardwareOffset, Config->HardwareLength, &end);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (end != Length)
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    section = Add2Ptr(Config, Config->HardwareOffset);

    status = KphpDynHwValidateSection(section, Config->HardwareLength);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    *Section = section;
    *SectionLength = Config->HardwareLength;

    return STATUS_SUCCESS;
}

/**
 * \brief Finds the op table for a hardware identity.
 *
 * \param[in] Section The hardware section.
 * \param[in] IdentityKind The kind of identity to find (KPH_DYN_HW_IDENTITY_*).
 * \param[in] Identity The identity to find, with its Reserved bytes zeroed.
 * \param[out] Table Receives a pointer into the section for the op table.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphDynHwFindTable(
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ UCHAR IdentityKind,
    _In_ PKPH_DYN_HW_IDENTITY Identity,
    _Outptr_ PKPH_DYN_HW_TABLE* Table
    )
{
    PKPH_DYN_HW_INDEX index;
    ULONG identityLength;
    ULONG low;
    ULONG high;

    *Table = NULL;

    identityLength = KphpDynHwIdentityLength(IdentityKind);
    if (identityLength == 0)
    {
        NT_ASSERT(FALSE);
        return STATUS_INVALID_PARAMETER;
    }

    index = Add2Ptr(Section, sizeof(KPH_DYN_HW_SECTION));

    //
    // Rows are sorted by identity kind, then identity bytes.
    //
    low = 0;
    high = Section->Count;

    while (low < high)
    {
        ULONG middle;
        LONG order;

        middle = (low + ((high - low) / 2));

        if (index[middle].IdentityKind != IdentityKind)
        {
            order = ((index[middle].IdentityKind < IdentityKind) ? -1 : 1);
        }
        else
        {
            order = memcmp(&index[middle].Identity, Identity, identityLength);
        }

        if (order == 0)
        {
            *Table = Add2Ptr(&index[Section->Count], index[middle].Offset);
            return STATUS_SUCCESS;
        }

        if (order < 0)
        {
            low = (middle + 1);
        }
        else
        {
            high = middle;
        }
    }

    return STATUS_NOT_FOUND;
}

/**
 * \brief Finds the first msr_read op for a register.
 *
 * \param[in] Table The op table.
 * \param[in] Register The model-specific register.
 * \param[out] Op Receives a pointer into the table for the op.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphDynHwFindMsrOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ ULONG Register,
    _Outptr_ PKPH_DYN_HW_OP* Op
    )
{
    PKPH_DYN_HW_OP op;

    *Op = NULL;

    op = Add2Ptr(Table, sizeof(KPH_DYN_HW_TABLE));

    for (ULONG i = 0; i < Table->Count; i++)
    {
        if ((op->Kind == KPH_DYN_HW_KIND_MSR_READ) &&
            (((PKPH_DYN_HW_MSR_READ)op->Body)->Register == Register))
        {
            *Op = op;
            return STATUS_SUCCESS;
        }

        op = Add2Ptr(op, op->Length);
    }

    return STATUS_NOT_FOUND;
}

/**
 * \brief Finds the first sysreg_read op for a register.
 *
 * \param[in] Table The op table.
 * \param[in] Register The system register (KPH_DYN_HW_SYSREG_*).
 * \param[out] Op Receives a pointer into the table for the op.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphDynHwFindSysregOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ UCHAR Register,
    _Outptr_ PKPH_DYN_HW_OP* Op
    )
{
    PKPH_DYN_HW_OP op;

    *Op = NULL;

    op = Add2Ptr(Table, sizeof(KPH_DYN_HW_TABLE));

    for (ULONG i = 0; i < Table->Count; i++)
    {
        if ((op->Kind == KPH_DYN_HW_KIND_SYSREG_READ) &&
            (((PKPH_DYN_HW_SYSREG_READ)op->Body)->SysregEnum == Register))
        {
            *Op = op;
            return STATUS_SUCCESS;
        }

        op = Add2Ptr(op, op->Length);
    }

    return STATUS_NOT_FOUND;
}

/**
 * \brief Finds the first pci_read op with a range containing a request.
 *
 * \param[in] Table The op table.
 * \param[in] Offset The configuration space offset of the request.
 * \param[in] Length The non-zero length of the request.
 * \param[out] Op Receives a pointer into the table for the op.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphDynHwFindPciReadOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ ULONG Offset,
    _In_ ULONG Length,
    _Outptr_ PKPH_DYN_HW_OP* Op
    )
{
    PKPH_DYN_HW_OP op;

    NT_ASSERT(Length != 0);

    *Op = NULL;

    op = Add2Ptr(Table, sizeof(KPH_DYN_HW_TABLE));

    for (ULONG i = 0; i < Table->Count; i++)
    {
        if (op->Kind == KPH_DYN_HW_KIND_PCI_READ)
        {
            PKPH_DYN_HW_PCI_READ read;
            PKPH_DYN_HW_RANGE ranges;

            read = (PKPH_DYN_HW_PCI_READ)op->Body;
            ranges = Add2Ptr(read, FIELD_OFFSET(KPH_DYN_HW_PCI_READ, Ranges));

            for (ULONG j = 0; j < read->RangeCount; j++)
            {
                PKPH_DYN_HW_RANGE range;
                ULONG delta;

                range = &ranges[j];

                if (Offset < range->Offset)
                {
                    continue;
                }

                delta = (Offset - range->Offset);

                if ((delta < range->Length) &&
                    (Length <= (range->Length - delta)))
                {
                    *Op = op;
                    return STATUS_SUCCESS;
                }
            }
        }

        op = Add2Ptr(op, op->Length);
    }

    return STATUS_NOT_FOUND;
}

/**
 * \brief Finds an op by its index among the ops of one kind of a table.
 *
 * \param[in] Table The op table.
 * \param[in] Kind The op kind.
 * \param[in] Index The index of the op among the ops of the kind.
 * \param[out] Op Receives a pointer into the table for the op.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphpDynHwFindOpOfKind(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ USHORT Kind,
    _In_ ULONG Index,
    _Outptr_ PKPH_DYN_HW_OP* Op
    )
{
    PKPH_DYN_HW_OP op;
    ULONG remaining;

    *Op = NULL;

    op = Add2Ptr(Table, sizeof(KPH_DYN_HW_TABLE));
    remaining = Index;

    for (ULONG i = 0; i < Table->Count; i++)
    {
        if (op->Kind == Kind)
        {
            if (remaining == 0)
            {
                *Op = op;
                return STATUS_SUCCESS;
            }

            remaining--;
        }

        op = Add2Ptr(op, op->Length);
    }

    return STATUS_NOT_FOUND;
}

/**
 * \brief Finds the first pci_indirect_read op.
 *
 * \param[in] Table The op table.
 * \param[out] Op Receives a pointer into the table for the op.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphDynHwFindPciIndirectOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _Outptr_ PKPH_DYN_HW_OP* Op
    )
{
    return KphpDynHwFindOpOfKind(Table,
                                 KPH_DYN_HW_KIND_PCI_INDIRECT_READ,
                                 0,
                                 Op);
}

/**
 * \brief Checks whether a pci_indirect_read op allows a target.
 *
 * \param[in] Op The pci_indirect_read op.
 * \param[in] Target The indirect target.
 *
 * \return TRUE if the target is allowed, FALSE otherwise.
 */
_Must_inspect_result_
BOOLEAN KphDynHwIndirectTargetAllowed(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG Target
    )
{
    PKPH_DYN_HW_PCI_INDIRECT_READ indirect;
    PKPH_DYN_HW_TARGET_RANGE ranges;
    ULONG UNALIGNED* values;

    NT_ASSERT(Op->Kind == KPH_DYN_HW_KIND_PCI_INDIRECT_READ);

    if ((Target & 3) != 0)
    {
        return FALSE;
    }

    indirect = (PKPH_DYN_HW_PCI_INDIRECT_READ)Op->Body;
    ranges = Add2Ptr(indirect, sizeof(KPH_DYN_HW_PCI_INDIRECT_READ));

    for (ULONG i = 0; i < indirect->RangeCount; i++)
    {
        PKPH_DYN_HW_TARGET_RANGE range;
        ULONG step;

        range = &ranges[i];

        if (Target < range->Begin)
        {
            continue;
        }

        //
        // Of the strided copies, the last one beginning at or below the target
        // is the only one that can contain it.
        //
        step = 0;

        if ((range->StrideCount > 1) && range->Stride)
        {
            step = min((Target - range->Begin) / range->Stride,
                       (ULONG)range->StrideCount - 1);
        }

        if ((Target - (step * range->Stride)) < range->End)
        {
            return TRUE;
        }
    }

    values = (ULONG UNALIGNED*)&ranges[indirect->RangeCount];

    for (ULONG i = 0; i < indirect->ValueCount; i++)
    {
        if (values[i] == Target)
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * \brief Finds the window_read op at an index.
 *
 * \param[in] Table The op table.
 * \param[in] Index The index of the op among the window_read ops.
 * \param[out] Op Receives a pointer into the table for the op.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphDynHwFindWindowOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ ULONG Index,
    _Outptr_ PKPH_DYN_HW_OP* Op
    )
{
    return KphpDynHwFindOpOfKind(Table, KPH_DYN_HW_KIND_WINDOW_READ, Index, Op);
}

/**
 * \brief Retrieves an aux word of a window_read op.
 *
 * \param[in] Op The window_read op.
 * \param[in] Index The index of the word, below the op's AuxCount.
 *
 * \return The aux word.
 */
ULONG KphDynHwWindowAux(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG Index
    )
{
    PKPH_DYN_HW_WINDOW_READ window;
    PKPH_DYN_HW_WINDOW_RANGE ranges;
    ULONG UNALIGNED* aux;

    NT_ASSERT(Op->Kind == KPH_DYN_HW_KIND_WINDOW_READ);

    window = (PKPH_DYN_HW_WINDOW_READ)Op->Body;

    NT_ASSERT(Index < window->AuxCount);

    ranges = Add2Ptr(window, sizeof(KPH_DYN_HW_WINDOW_READ));
    aux = (ULONG UNALIGNED*)&ranges[window->RangeCount];

    return aux[Index];
}

/**
 * \brief Checks whether a window_read op allows a read.
 *
 * \param[in] Op The window_read op.
 * \param[in] Offset The window offset of the read, aligned to its width.
 * \param[in] Width The non-zero access width of the read.
 *
 * \return TRUE if the read is allowed, FALSE otherwise.
 */
_Must_inspect_result_
BOOLEAN KphDynHwWindowReadAllowed(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG Offset,
    _In_ UCHAR Width
    )
{
    PKPH_DYN_HW_WINDOW_READ window;
    PKPH_DYN_HW_WINDOW_RANGE ranges;

    NT_ASSERT(Op->Kind == KPH_DYN_HW_KIND_WINDOW_READ);
    NT_ASSERT(Width &&
              ((Offset % Width) == 0));

    window = (PKPH_DYN_HW_WINDOW_READ)Op->Body;
    ranges = Add2Ptr(window, sizeof(KPH_DYN_HW_WINDOW_READ));

    for (ULONG i = 0; i < window->RangeCount; i++)
    {
        PKPH_DYN_HW_WINDOW_RANGE range;

        range = &ranges[i];

        if ((range->Width == Width) &&
            (Offset >= range->Offset) &&
            ((Offset - range->Offset) < range->Length))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * \brief Checks whether the gate of a superio_read op passes.
 *
 * \param[in] Op The superio_read op.
 * \param[in] ChassisType The SMBIOS chassis type, zero when absent.
 *
 * \return TRUE if the op has no gate or the chassis type is in its mask.
 */
_Must_inspect_result_
BOOLEAN KphDynHwSuperIoGatePasses(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ UCHAR ChassisType
    )
{
    ULONG64 mask;

    NT_ASSERT(Op->Kind == KPH_DYN_HW_KIND_SUPERIO_READ);

    if (Op->Gate.Kind == KPH_DYN_HW_GATE_NONE)
    {
        return TRUE;
    }

    NT_ASSERT(Op->Gate.Kind == KPH_DYN_HW_GATE_CHASSIS);

    mask = (((ULONG64)Op->Gate.ValueMax << 32) | Op->Gate.Value);

    return ((ChassisType < 64) && ((mask >> ChassisType) & 1));
}

/**
 * \brief Checks whether a system register gate's field lies in its range.
 *
 * \param[in] Gate The SYSREG_FIELD gate.
 * \param[in] Value The value of the gate's register.
 *
 * \return TRUE if bits [Shift, Shift + Width) of the value lie in
 * [Value, ValueMax], FALSE otherwise.
 */
_Must_inspect_result_
BOOLEAN KphDynHwSysregFieldInRange(
    _In_ PKPH_DYN_HW_GATE Gate,
    _In_ ULONG64 Value
    )
{
    ULONG64 field;

    NT_ASSERT(Gate->Kind == KPH_DYN_HW_GATE_SYSREG_FIELD);
    NT_ASSERT((Gate->Width >= 1) &&
              (Gate->Width <= 32));
    NT_ASSERT(Gate->Shift <= (64 - Gate->Width));

    field = ((Value >> Gate->Shift) & ((1ull << Gate->Width) - 1));

    return ((field >= Gate->Value) && (field <= Gate->ValueMax));
}

/**
 * \brief Checks whether a PCI configuration gate's field lies in its range.
 *
 * \param[in] Gate The PCI_CONFIG gate.
 * \param[in] Value The configuration register at the gate's offset.
 *
 * \return TRUE if bits [Shift, Shift + Width) of the value lie in
 * [Value, ValueMax], FALSE otherwise.
 */
_Must_inspect_result_
BOOLEAN KphDynHwPciConfigFieldInRange(
    _In_ PKPH_DYN_HW_GATE Gate,
    _In_ ULONG Value
    )
{
    ULONG64 field;

    NT_ASSERT(Gate->Kind == KPH_DYN_HW_GATE_PCI_CONFIG);
    NT_ASSERT((Gate->Width >= 1) &&
              (Gate->Width <= 32));
    NT_ASSERT(Gate->Shift <= (32 - Gate->Width));

    field = (((ULONG64)Value >> Gate->Shift) & ((1ull << Gate->Width) - 1));

    return ((field >= Gate->Value) && (field <= Gate->ValueMax));
}

/**
 * \brief Checks whether any superio_read op of a table passes its gate.
 *
 * \param[in] Table The op table of a SUPERIO index row.
 * \param[in] ChassisType The SMBIOS chassis type, zero when absent.
 *
 * \return TRUE if an op of the table passes its gate.
 */
_Must_inspect_result_
BOOLEAN KphpDynHwSuperIoTablePasses(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ UCHAR ChassisType
    )
{
    PKPH_DYN_HW_OP op;

    op = Add2Ptr(Table, sizeof(KPH_DYN_HW_TABLE));

    for (ULONG i = 0; i < Table->Count; i++)
    {
        if (KphDynHwSuperIoGatePasses(op, ChassisType))
        {
            return TRUE;
        }

        op = Add2Ptr(op, op->Length);
    }

    return FALSE;
}

/**
 * \brief Checks whether a passing superio_read op of a table allows a
 * hardware monitor base.
 *
 * \param[in] Table The op table of a SUPERIO index row.
 * \param[in] ChassisType The SMBIOS chassis type, zero when absent.
 * \param[in] Base The hardware monitor base port read from the chip.
 * \param[in] Extent The number of ports accessed from the base.
 *
 * \return TRUE if an op of the table passes its gate and its base range
 * contains [Base, Base + Extent).
 */
_Must_inspect_result_
BOOLEAN KphpDynHwSuperIoTableBaseAllowed(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ UCHAR ChassisType,
    _In_ USHORT Base,
    _In_ USHORT Extent
    )
{
    PKPH_DYN_HW_OP op;

    NT_ASSERT(Extent >= 1);

    op = Add2Ptr(Table, sizeof(KPH_DYN_HW_TABLE));

    for (ULONG i = 0; i < Table->Count; i++)
    {
        PKPH_DYN_HW_SUPERIO_READ superIo;

        superIo = (PKPH_DYN_HW_SUPERIO_READ)op->Body;

        if (KphDynHwSuperIoGatePasses(op, ChassisType) &&
            (Base >= superIo->BaseMin) &&
            (((ULONG)Base + Extent - 1) <= superIo->BaseMax))
        {
            return TRUE;
        }

        op = Add2Ptr(op, op->Length);
    }

    return FALSE;
}

/**
 * \brief Retrieves the SuperIO families with a passing row for a slot.
 *
 * \param[in] Section The hardware section.
 * \param[in] Slot The configuration port slot.
 * \param[in] ChassisType The SMBIOS chassis type, zero when absent.
 *
 * \return Bit n set for each KPH_DYN_HW_SUPERIO_FAMILY n with such a row.
 */
_Must_inspect_result_
ULONG KphDynHwSuperIoFamilies(
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ UCHAR Slot,
    _In_ UCHAR ChassisType
    )
{
    PKPH_DYN_HW_INDEX index;
    PVOID body;
    ULONG families;

    index = Add2Ptr(Section, sizeof(KPH_DYN_HW_SECTION));
    body = &index[Section->Count];
    families = 0;

    for (ULONG i = 0; i < Section->Count; i++)
    {
        PKPH_DYN_HW_SUPERIO_IDENTITY superIo;
        ULONG family;

        if (index[i].IdentityKind != KPH_DYN_HW_IDENTITY_SUPERIO)
        {
            continue;
        }

        superIo = &index[i].Identity.SuperIo;
        family = (1ul << superIo->VendorFamily);

        if ((superIo->Slot == Slot) &&
            !(families & family) &&
            KphpDynHwSuperIoTablePasses(Add2Ptr(body, index[i].Offset),
                                        ChassisType))
        {
            families |= family;
        }
    }

    return families;
}

/**
 * \brief Checks whether a SuperIO chip has a passing row for any customer ID.
 *
 * \param[in] Section The hardware section.
 * \param[in] Slot The configuration port slot.
 * \param[in] Family The SuperIO family (KPH_DYN_HW_SUPERIO_FAMILY_*).
 * \param[in] ChipId The chip ID read from the chip.
 * \param[in] ChassisType The SMBIOS chassis type, zero when absent.
 *
 * \return TRUE if such a row has the slot, family and chip ID.
 */
_Must_inspect_result_
BOOLEAN KphDynHwSuperIoChipKnown(
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ UCHAR Slot,
    _In_ UCHAR Family,
    _In_ USHORT ChipId,
    _In_ UCHAR ChassisType
    )
{
    PKPH_DYN_HW_INDEX index;
    PVOID body;

    index = Add2Ptr(Section, sizeof(KPH_DYN_HW_SECTION));
    body = &index[Section->Count];

    for (ULONG i = 0; i < Section->Count; i++)
    {
        PKPH_DYN_HW_SUPERIO_IDENTITY superIo;

        if (index[i].IdentityKind != KPH_DYN_HW_IDENTITY_SUPERIO)
        {
            continue;
        }

        superIo = &index[i].Identity.SuperIo;

        if ((superIo->Slot == Slot) &&
            (superIo->VendorFamily == Family) &&
            (superIo->ChipId == ChipId) &&
            KphpDynHwSuperIoTablePasses(Add2Ptr(body, index[i].Offset),
                                        ChassisType))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * \brief Checks whether a SuperIO chip's hardware monitor base lies in the
 * range a passing row allows.
 *
 * \param[in] Section The hardware section.
 * \param[in] Slot The configuration port slot.
 * \param[in] Family The SuperIO family (KPH_DYN_HW_SUPERIO_FAMILY_*).
 * \param[in] ChipId The chip ID read from the chip.
 * \param[in] ChassisType The SMBIOS chassis type, zero when absent.
 * \param[in] Base The hardware monitor base port read from the chip.
 * \param[in] Extent The number of ports accessed from the base.
 *
 * \return TRUE if a passing row with the slot, family and chip ID has a
 * superio_read op whose base range contains [Base, Base + Extent).
 */
_Must_inspect_result_
BOOLEAN KphDynHwSuperIoBaseAllowed(
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ UCHAR Slot,
    _In_ UCHAR Family,
    _In_ USHORT ChipId,
    _In_ UCHAR ChassisType,
    _In_ USHORT Base,
    _In_ USHORT Extent
    )
{
    PKPH_DYN_HW_INDEX index;
    PVOID body;

    index = Add2Ptr(Section, sizeof(KPH_DYN_HW_SECTION));
    body = &index[Section->Count];

    for (ULONG i = 0; i < Section->Count; i++)
    {
        PKPH_DYN_HW_SUPERIO_IDENTITY superIo;

        if (index[i].IdentityKind != KPH_DYN_HW_IDENTITY_SUPERIO)
        {
            continue;
        }

        superIo = &index[i].Identity.SuperIo;

        if ((superIo->Slot == Slot) &&
            (superIo->VendorFamily == Family) &&
            (superIo->ChipId == ChipId) &&
            KphpDynHwSuperIoTableBaseAllowed(Add2Ptr(body, index[i].Offset),
                                             ChassisType,
                                             Base,
                                             Extent))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * \brief Finds the superio_read op at an index.
 *
 * \param[in] Table The op table.
 * \param[in] Index The index of the op among the superio_read ops.
 * \param[out] Op Receives a pointer into the table for the op.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphDynHwFindSuperIoOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ ULONG Index,
    _Outptr_ PKPH_DYN_HW_OP* Op
    )
{
    return KphpDynHwFindOpOfKind(Table,
                                 KPH_DYN_HW_KIND_SUPERIO_READ,
                                 Index,
                                 Op);
}

/**
 * \brief Checks whether a superio_read op allows a register.
 *
 * \param[in] Op The superio_read op.
 * \param[in] Register The register, (bank or page << 8) | index.
 *
 * \return TRUE if a range of the op contains the register.
 */
_Must_inspect_result_
BOOLEAN KphDynHwSuperIoReadAllowed(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ USHORT Register
    )
{
    PKPH_DYN_HW_SUPERIO_READ superIo;
    PKPH_DYN_HW_RANGE ranges;

    NT_ASSERT(Op->Kind == KPH_DYN_HW_KIND_SUPERIO_READ);

    superIo = (PKPH_DYN_HW_SUPERIO_READ)Op->Body;
    ranges = Add2Ptr(superIo, sizeof(KPH_DYN_HW_SUPERIO_READ));

    for (ULONG i = 0; i < superIo->RangeCount; i++)
    {
        if ((Register >= ranges[i].Offset) &&
            ((ULONG)(Register - ranges[i].Offset) < ranges[i].Length))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * \brief Finds the smbus_read op at an index.
 *
 * \param[in] Table The op table.
 * \param[in] Index The index of the op among the smbus_read ops.
 * \param[out] Op Receives a pointer into the table for the op.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphDynHwFindSmbusOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ ULONG Index,
    _Outptr_ PKPH_DYN_HW_OP* Op
    )
{
    return KphpDynHwFindOpOfKind(Table, KPH_DYN_HW_KIND_SMBUS_READ, Index, Op);
}

/**
 * \brief Finds the smbus_host op at an index.
 *
 * \param[in] Table The op table.
 * \param[in] Index The index of the op among the smbus_host ops.
 * \param[out] Op Receives a pointer into the table for the op.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphDynHwFindSmbusHostOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ ULONG Index,
    _Outptr_ PKPH_DYN_HW_OP* Op
    )
{
    return KphpDynHwFindOpOfKind(Table, KPH_DYN_HW_KIND_SMBUS_HOST, Index, Op);
}

/**
 * \brief Checks whether an smbus_read op allows a transaction.
 *
 * \param[in] Op The smbus_read op.
 * \param[in] Address The 7-bit slave address.
 * \param[in] Command The command byte.
 * \param[in] Protocol The protocol, BYTE_DATA or WORD_DATA.
 *
 * \return TRUE if the transaction is allowed.
 */
_Must_inspect_result_
BOOLEAN KphDynHwSmbusReadAllowed(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ UCHAR Address,
    _In_ UCHAR Command,
    _In_ UCHAR Protocol
    )
{
    PKPH_DYN_HW_SMBUS_READ smbus;
    PKPH_DYN_HW_RANGE ranges;

    NT_ASSERT(Op->Kind == KPH_DYN_HW_KIND_SMBUS_READ);
    NT_ASSERT((Protocol == KPH_DYN_HW_SMBUS_PROTOCOL_BYTE_DATA) ||
              (Protocol == KPH_DYN_HW_SMBUS_PROTOCOL_WORD_DATA));

    smbus = (PKPH_DYN_HW_SMBUS_READ)Op->Body;

    NT_ASSERT(Address < (sizeof(smbus->Slaves) * 8));

    if (!(smbus->Protocols & Protocol) ||
        !(smbus->Slaves[Address >> 3] & (1 << (Address & 7))))
    {
        return FALSE;
    }

    ranges = Add2Ptr(smbus, sizeof(KPH_DYN_HW_SMBUS_READ));

    for (ULONG i = 0; i < smbus->RangeCount; i++)
    {
        if ((Command >= ranges[i].Offset) &&
            ((ULONG)(Command - ranges[i].Offset) < ranges[i].Length))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * \brief Finds the vsec_read op at an index.
 *
 * \param[in] Table The op table.
 * \param[in] Index The index of the op among the vsec_read ops.
 * \param[out] Op Receives a pointer into the table for the op.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphDynHwFindVsecOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ ULONG Index,
    _Outptr_ PKPH_DYN_HW_OP* Op
    )
{
    return KphpDynHwFindOpOfKind(Table, KPH_DYN_HW_KIND_VSEC_READ, Index, Op);
}

/**
 * \brief Checks whether a vsec_read op allows a read of a region.
 *
 * \param[in] Op The vsec_read op.
 * \param[in] Offset The region offset of the read, aligned to its width.
 * \param[in] Width The non-zero access width of the read.
 *
 * \return TRUE if the read is allowed, FALSE otherwise.
 */
_Must_inspect_result_
BOOLEAN KphDynHwVsecReadAllowed(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG Offset,
    _In_ UCHAR Width
    )
{
    PKPH_DYN_HW_VSEC_READ vsec;
    PKPH_DYN_HW_WINDOW_RANGE ranges;

    NT_ASSERT(Op->Kind == KPH_DYN_HW_KIND_VSEC_READ);
    NT_ASSERT(Width &&
              ((Offset % Width) == 0));

    vsec = (PKPH_DYN_HW_VSEC_READ)Op->Body;
    ranges = Add2Ptr(vsec, sizeof(KPH_DYN_HW_VSEC_READ));

    for (ULONG i = 0; i < vsec->RangeCount; i++)
    {
        PKPH_DYN_HW_WINDOW_RANGE range;

        range = &ranges[i];

        if ((range->Width == Width) &&
            (Offset >= range->Offset) &&
            ((Offset - range->Offset) < range->Length))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * \brief Finds the mailbox_transact op at an index.
 *
 * \param[in] Table The op table.
 * \param[in] Index The index of the op among the mailbox_transact ops.
 * \param[out] Op Receives a pointer into the table for the op.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphDynHwFindMailboxOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ ULONG Index,
    _Outptr_ PKPH_DYN_HW_OP* Op
    )
{
    return KphpDynHwFindOpOfKind(Table,
                                 KPH_DYN_HW_KIND_MAILBOX_TRANSACT,
                                 Index,
                                 Op);
}

/**
 * \brief Finds the smu_table_read op at an index.
 *
 * \param[in] Table The op table.
 * \param[in] Index The index of the op among the smu_table_read ops.
 * \param[out] Op Receives a pointer into the table for the op.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphDynHwFindSmuTableOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ ULONG Index,
    _Outptr_ PKPH_DYN_HW_OP* Op
    )
{
    return KphpDynHwFindOpOfKind(Table,
                                 KPH_DYN_HW_KIND_SMU_TABLE_READ,
                                 Index,
                                 Op);
}

/**
 * \brief Finds the port_indirect_read op at an index.
 *
 * \param[in] Table The op table.
 * \param[in] Index The index of the op among the port_indirect_read ops.
 * \param[out] Op Receives a pointer into the table for the op.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphDynHwFindPortIndirectOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ ULONG Index,
    _Outptr_ PKPH_DYN_HW_OP* Op
    )
{
    return KphpDynHwFindOpOfKind(Table,
                                 KPH_DYN_HW_KIND_PORT_INDIRECT_READ,
                                 Index,
                                 Op);
}

/**
 * \brief Checks whether a port_indirect_read op allows an index.
 *
 * \param[in] Op The port_indirect_read op.
 * \param[in] Index The index register value.
 *
 * \return TRUE if a range of the op contains the index.
 */
_Must_inspect_result_
BOOLEAN KphDynHwPortIndirectReadAllowed(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ UCHAR Index
    )
{
    PKPH_DYN_HW_PORT_INDIRECT_READ port;
    PKPH_DYN_HW_RANGE ranges;

    NT_ASSERT(Op->Kind == KPH_DYN_HW_KIND_PORT_INDIRECT_READ);

    port = (PKPH_DYN_HW_PORT_INDIRECT_READ)Op->Body;
    ranges = Add2Ptr(port, sizeof(KPH_DYN_HW_PORT_INDIRECT_READ));

    for (ULONG i = 0; i < port->RangeCount; i++)
    {
        if ((Index >= ranges[i].Offset) &&
            ((ULONG)(Index - ranges[i].Offset) < ranges[i].Length))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * \brief Finds the mailbox command for an opcode and firmware version.
 *
 * \param[in] Op The mailbox_transact op.
 * \param[in] Opcode The opcode.
 * \param[out] Command Receives a pointer into the op for the command.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphDynHwFindMailboxCommand(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG Opcode,
    _Outptr_ PKPH_DYN_HW_MAILBOX_COMMAND* Command
    )
{
    PKPH_DYN_HW_MAILBOX_TRANSACT mailbox;
    PKPH_DYN_HW_MAILBOX_COMMAND commands;

    NT_ASSERT(Op->Kind == KPH_DYN_HW_KIND_MAILBOX_TRANSACT);

    *Command = NULL;

    mailbox = (PKPH_DYN_HW_MAILBOX_TRANSACT)Op->Body;
    commands = Add2Ptr(mailbox, sizeof(KPH_DYN_HW_MAILBOX_TRANSACT));

    for (ULONG i = 0; i < mailbox->CommandCount; i++)
    {
        if (commands[i].Opcode == Opcode)
        {
            *Command = &commands[i];
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}

/**
 * \brief Finds the acpi_eval op at an index.
 *
 * \param[in] Table The op table.
 * \param[in] Index The index of the op among the acpi_eval ops.
 * \param[out] Op Receives a pointer into the table for the op.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphDynHwFindAcpiEvalOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ ULONG Index,
    _Outptr_ PKPH_DYN_HW_OP* Op
    )
{
    return KphpDynHwFindOpOfKind(Table, KPH_DYN_HW_KIND_ACPI_EVAL, Index, Op);
}

/**
 * \brief Composes the integer arguments of an acpi_eval op.
 *
 * \param[in] Op The acpi_eval op.
 * \param[in] Count The number of caller values, the op's argument count.
 * \param[in] Values The caller values.
 * \param[out] Args Receives the arguments, untouched on failure.
 *
 * \return TRUE if every value is allowed, FALSE otherwise.
 */
_Must_inspect_result_
_Success_(return)
BOOLEAN KphDynHwAcpiComposeArgs(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG Count,
    _In_reads_(Count) PULONG Values,
    _Out_writes_(Count) PULONG Args
    )
{
    PKPH_DYN_HW_ACPI_EVAL eval;
    PKPH_DYN_HW_ACPI_ARG args;

    NT_ASSERT(Op->Kind == KPH_DYN_HW_KIND_ACPI_EVAL);

    eval = (PKPH_DYN_HW_ACPI_EVAL)Op->Body;
    args = Add2Ptr(eval, sizeof(KPH_DYN_HW_ACPI_EVAL));

    NT_ASSERT(Count == eval->ArgCount);

    for (ULONG i = 0; i < Count; i++)
    {
        if ((Values[i] & ~args[i].Mask) || (Values[i] > args[i].Max))
        {
            return FALSE;
        }
    }

    for (ULONG i = 0; i < Count; i++)
    {
        Args[i] = (Values[i] | args[i].Fixed);
    }

    return TRUE;
}
