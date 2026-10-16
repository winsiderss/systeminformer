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

#pragma once

#include <kphdyn.h>

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
    );

_Must_inspect_result_
NTSTATUS KphDynDataLookupHardware(
    _In_reads_bytes_(Length) PKPH_DYN_CONFIG Config,
    _In_ ULONG Length,
    _Outptr_result_maybenull_ PKPH_DYN_HW_SECTION* Section,
    _Out_ PULONG SectionLength
    );

_Must_inspect_result_
NTSTATUS KphDynHwFindTable(
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ UCHAR IdentityKind,
    _In_ PKPH_DYN_HW_IDENTITY Identity,
    _Outptr_ PKPH_DYN_HW_TABLE* Table
    );

_Must_inspect_result_
NTSTATUS KphDynHwFindMsrOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ ULONG Register,
    _Outptr_ PKPH_DYN_HW_OP* Op
    );

_Must_inspect_result_
NTSTATUS KphDynHwFindSysregOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ UCHAR Register,
    _Outptr_ PKPH_DYN_HW_OP* Op
    );

_Must_inspect_result_
NTSTATUS KphDynHwFindPciReadOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ ULONG Offset,
    _In_ ULONG Length,
    _Outptr_ PKPH_DYN_HW_OP* Op
    );

_Must_inspect_result_
NTSTATUS KphDynHwFindPciIndirectOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _Outptr_ PKPH_DYN_HW_OP* Op
    );

_Must_inspect_result_
BOOLEAN KphDynHwIndirectTargetAllowed(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG Target
    );

_Must_inspect_result_
NTSTATUS KphDynHwFindWindowOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ ULONG Index,
    _Outptr_ PKPH_DYN_HW_OP* Op
    );

ULONG KphDynHwWindowAux(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG Index
    );

_Must_inspect_result_
BOOLEAN KphDynHwWindowReadAllowed(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG Offset,
    _In_ UCHAR Width
    );

_Must_inspect_result_
BOOLEAN KphDynHwSuperIoGatePasses(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ UCHAR ChassisType
    );

_Must_inspect_result_
BOOLEAN KphDynHwSysregFieldInRange(
    _In_ PKPH_DYN_HW_GATE Gate,
    _In_ ULONG64 Value
    );

_Must_inspect_result_
BOOLEAN KphDynHwPciConfigFieldInRange(
    _In_ PKPH_DYN_HW_GATE Gate,
    _In_ ULONG Value
    );

_Must_inspect_result_
ULONG KphDynHwSuperIoFamilies(
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ UCHAR Slot,
    _In_ UCHAR ChassisType
    );

_Must_inspect_result_
BOOLEAN KphDynHwSuperIoChipKnown(
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ UCHAR Slot,
    _In_ UCHAR Family,
    _In_ USHORT ChipId,
    _In_ UCHAR ChassisType
    );

_Must_inspect_result_
BOOLEAN KphDynHwSuperIoBaseAllowed(
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ UCHAR Slot,
    _In_ UCHAR Family,
    _In_ USHORT ChipId,
    _In_ UCHAR ChassisType,
    _In_ USHORT Base,
    _In_ USHORT Extent
    );

_Must_inspect_result_
NTSTATUS KphDynHwFindSuperIoOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ ULONG Index,
    _Outptr_ PKPH_DYN_HW_OP* Op
    );

_Must_inspect_result_
BOOLEAN KphDynHwSuperIoReadAllowed(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ USHORT Register
    );

_Must_inspect_result_
NTSTATUS KphDynHwFindSmbusOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ ULONG Index,
    _Outptr_ PKPH_DYN_HW_OP* Op
    );

_Must_inspect_result_
NTSTATUS KphDynHwFindSmbusHostOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ ULONG Index,
    _Outptr_ PKPH_DYN_HW_OP* Op
    );

_Must_inspect_result_
NTSTATUS KphDynHwFindVsecOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ ULONG Index,
    _Outptr_ PKPH_DYN_HW_OP* Op
    );

_Must_inspect_result_
BOOLEAN KphDynHwVsecReadAllowed(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG Offset,
    _In_ UCHAR Width
    );

_Must_inspect_result_
NTSTATUS KphDynHwFindMailboxOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ ULONG Index,
    _Outptr_ PKPH_DYN_HW_OP* Op
    );

_Must_inspect_result_
NTSTATUS KphDynHwFindSmuTableOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ ULONG Index,
    _Outptr_ PKPH_DYN_HW_OP* Op
    );

_Must_inspect_result_
NTSTATUS KphDynHwFindMailboxCommand(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG Opcode,
    _Outptr_ PKPH_DYN_HW_MAILBOX_COMMAND* Command
    );

_Must_inspect_result_
BOOLEAN KphDynHwSmbusReadAllowed(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ UCHAR Address,
    _In_ UCHAR Command,
    _In_ UCHAR Protocol
    );

_Must_inspect_result_
NTSTATUS KphDynHwFindPortIndirectOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ ULONG Index,
    _Outptr_ PKPH_DYN_HW_OP* Op
    );

_Must_inspect_result_
BOOLEAN KphDynHwPortIndirectReadAllowed(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ UCHAR Index
    );

_Must_inspect_result_
BOOLEAN KphDynHwAcpiIdValid(
    _In_reads_(KPH_DYN_HW_ACPI_ID_LENGTH) PCCH Id
    );

_Must_inspect_result_
NTSTATUS KphDynHwFindAcpiEvalOp(
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ ULONG Index,
    _Outptr_ PKPH_DYN_HW_OP* Op
    );

_Must_inspect_result_
_Success_(return)
BOOLEAN KphDynHwAcpiComposeArgs(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG Count,
    _In_reads_(Count) PULONG Values,
    _Out_writes_(Count) PULONG Args
    );
