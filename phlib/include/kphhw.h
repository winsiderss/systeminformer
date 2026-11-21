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

#ifndef _PH_KPHHW_H
#define _PH_KPHHW_H

#include <kphuser.h>

EXTERN_C_START

typedef struct _KPH_HW_CPU_IDENTITY
{
    CHAR Vendor[12];
    ULONG Family;
    ULONG Model;
    ULONG Stepping;
    ULONG Signature;
    ULONG Hybrid;
} KPH_HW_CPU_IDENTITY, *PKPH_HW_CPU_IDENTITY;

typedef struct _KPH_HW_CPU_ITEM
{
    USHORT Group;
    USHORT Number;
    ULONG Msr;
} KPH_HW_CPU_ITEM, *PKPH_HW_CPU_ITEM;

typedef const KPH_HW_CPU_ITEM* PCKPH_HW_CPU_ITEM;

typedef struct _KPH_HW_VALUE64
{
    NTSTATUS Status;
    ULONG Reserved;
    ULONG64 Value;
} KPH_HW_VALUE64, *PKPH_HW_VALUE64;

typedef struct _KPH_HW_SUPERIO_CHIP
{
    ULONG Family;
    ULONG Chip;
    ULONG Customer;
} KPH_HW_SUPERIO_CHIP, *PKPH_HW_SUPERIO_CHIP;

typedef struct _KPH_HW_VALUE32
{
    NTSTATUS Status;
    ULONG Value;
} KPH_HW_VALUE32, *PKPH_HW_VALUE32;

typedef struct _KPH_HW_SMN_IDENTITY
{
    USHORT Vendor;
    USHORT Device;
    ULONG Family;
    ULONG Model;
    ULONG Stepping;
} KPH_HW_SMN_IDENTITY, *PKPH_HW_SMN_IDENTITY;

typedef struct _KPH_HW_SMU_IDENTITY
{
    ULONG FirmwareVersion;
    ULONG TableVersion;
    ULONG TableLength;
    ULONG Reserved;
} KPH_HW_SMU_IDENTITY, *PKPH_HW_SMU_IDENTITY;

typedef struct _KPH_HW_SMU_TABLE
{
    ULONG Version;
    ULONG Length;
} KPH_HW_SMU_TABLE, *PKPH_HW_SMU_TABLE;

typedef struct _KPH_HW_SMU_COMMAND
{
    ULONG Opcode;
    ULONG Arguments[6];
} KPH_HW_SMU_COMMAND, *PKPH_HW_SMU_COMMAND;

typedef const KPH_HW_SMU_COMMAND* PCKPH_HW_SMU_COMMAND;

typedef struct _KPH_HW_SMU_RESULT
{
    NTSTATUS Status;
    ULONG Response;
    ULONG Results[6];
} KPH_HW_SMU_RESULT, *PKPH_HW_SMU_RESULT;

typedef struct _KPH_HW_HSMP_IDENTITY
{
    ULONG Version;
    ULONG Reserved;
} KPH_HW_HSMP_IDENTITY, *PKPH_HW_HSMP_IDENTITY;

typedef struct _KPH_HW_HSMP_COMMAND
{
    ULONG Opcode;
    ULONG Arguments[8];
} KPH_HW_HSMP_COMMAND, *PKPH_HW_HSMP_COMMAND;

typedef const KPH_HW_HSMP_COMMAND* PCKPH_HW_HSMP_COMMAND;

typedef struct _KPH_HW_HSMP_RESULT
{
    NTSTATUS Status;
    ULONG Response;
    ULONG Results[8];
} KPH_HW_HSMP_RESULT, *PKPH_HW_HSMP_RESULT;

typedef struct _KPH_HW_ACPI_DEVICE
{
    ULONG Row;
    ULONG Ordinal;
    CHAR Id[8];
} KPH_HW_ACPI_DEVICE, *PKPH_HW_ACPI_DEVICE;

typedef struct _KPH_HW_SMBUS_HOST
{
    USHORT Vendor;
    USHORT Device;
    ULONG Revision;
    ULONG Kind;
    ULONG Base;
} KPH_HW_SMBUS_HOST, *PKPH_HW_SMBUS_HOST;

typedef struct _KPH_HW_SMBUS_ITEM
{
    UCHAR Address;
    UCHAR Command;
    UCHAR Protocol;
    UCHAR Reserved;
} KPH_HW_SMBUS_ITEM, *PKPH_HW_SMBUS_ITEM;

typedef const KPH_HW_SMBUS_ITEM* PCKPH_HW_SMBUS_ITEM;

typedef struct _KPH_HW_PCI_MMIO
{
    USHORT Vendor;
    USHORT Device;
    ULONG Windows;
} KPH_HW_PCI_MMIO, *PKPH_HW_PCI_MMIO;

typedef struct _KPH_HW_WINDOW_ITEM
{
    ULONG Offset;
    ULONG Width;
} KPH_HW_WINDOW_ITEM, *PKPH_HW_WINDOW_ITEM;

typedef const KPH_HW_WINDOW_ITEM* PCKPH_HW_WINDOW_ITEM;

typedef struct _KPH_HW_PCI_CONFIG
{
    USHORT Vendor;
    USHORT Device;
    ULONG Ranges;
} KPH_HW_PCI_CONFIG, *PKPH_HW_PCI_CONFIG;

#define KPH_HW_WINDOW_MCHBAR 1
#define KPH_HW_WINDOW_BAR0 2

#define KPH_HW_PLATFORM_LPIT 1
#define KPH_HW_PLATFORM_PWRM 2

#define KPH_HW_ACPI_ROW_PNP0C0B 1
#define KPH_HW_ACPI_ROW_INT3403 2
#define KPH_HW_ACPI_ROW_ATK0110 3
#define KPH_HW_ACPI_ROW_LEN0068 4
#define KPH_HW_ACPI_ROW_PNP0C09 5
#define KPH_HW_ACPI_ROW_ACPI000D 6
#define KPH_HW_ACPI_ROW_INT3407 7

C_ASSERT(sizeof(KPH_HW_CPU_IDENTITY) == 32);
C_ASSERT(sizeof(KPH_HW_CPU_ITEM) == 8);
C_ASSERT(sizeof(KPH_HW_VALUE64) == 16);
C_ASSERT(sizeof(KPH_HW_SUPERIO_CHIP) == 12);
C_ASSERT(sizeof(KPH_HW_VALUE32) == 8);
C_ASSERT(sizeof(KPH_HW_SMN_IDENTITY) == 16);
C_ASSERT(sizeof(KPH_HW_SMU_IDENTITY) == 16);
C_ASSERT(sizeof(KPH_HW_SMU_TABLE) == 8);
C_ASSERT(sizeof(KPH_HW_SMU_COMMAND) == 28);
C_ASSERT(sizeof(KPH_HW_SMU_RESULT) == 32);
C_ASSERT(sizeof(KPH_HW_HSMP_IDENTITY) == 8);
C_ASSERT(sizeof(KPH_HW_HSMP_COMMAND) == 36);
C_ASSERT(sizeof(KPH_HW_HSMP_RESULT) == 40);
C_ASSERT(sizeof(KPH_HW_ACPI_DEVICE) == 16);
C_ASSERT(sizeof(KPH_HW_SMBUS_HOST) == 16);
C_ASSERT(sizeof(KPH_HW_SMBUS_ITEM) == 4);
C_ASSERT(sizeof(KPH_HW_PCI_MMIO) == 8);
C_ASSERT(sizeof(KPH_HW_WINDOW_ITEM) == 8);
C_ASSERT(sizeof(KPH_HW_PCI_CONFIG) == 8);

PHLIBAPI
NTSTATUS
NTAPI
KphHwCpuIdentify(
    _In_ USHORT Group,
    _In_ USHORT Number,
    _Out_ PKPH_HW_CPU_IDENTITY Identity
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwCpuRead(
    _In_reads_(Count) PCKPH_HW_CPU_ITEM Items,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE64 Values
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwSuperIoIdentify(
    _In_ ULONG Slot,
    _Out_ PKPH_HW_SUPERIO_CHIP Chip
    );

PHLIBAPI
VOID
NTAPI
KphHwSetScriptMode(
    _In_ ULONG Flags
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwSuperIoRead(
    _In_ ULONG Slot,
    _In_reads_(Count) PCULONG Registers,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwSmnIdentify(
    _In_ ULONG Locator,
    _Out_ PKPH_HW_SMN_IDENTITY Identity
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwSmnRead(
    _In_ ULONG Locator,
    _In_reads_(Count) PCULONG Addresses,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwSmuIdentify(
    _In_ ULONG Locator,
    _Out_ PKPH_HW_SMU_IDENTITY Identity
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwSmuReadTable(
    _In_ ULONG Locator,
    _Out_ PKPH_HW_SMU_TABLE Header,
    _Out_writes_bytes_to_(Length, *ReturnLength) PVOID Table,
    _In_ ULONG Length,
    _Out_ PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwSmuCommand(
    _In_ ULONG Locator,
    _In_reads_(Count) PCKPH_HW_SMU_COMMAND Commands,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_SMU_RESULT Results
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwHsmpIdentify(
    _In_ ULONG Locator,
    _Out_ PKPH_HW_HSMP_IDENTITY Identity
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwHsmpCommand(
    _In_ ULONG Locator,
    _In_reads_(Count) PCKPH_HW_HSMP_COMMAND Commands,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_HSMP_RESULT Results
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwSmbusIdentify(
    _In_ ULONG Locator,
    _Out_ PKPH_HW_SMBUS_HOST Host
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwSmbusRead(
    _In_ ULONG Locator,
    _In_ ULONG Port,
    _In_reads_(Count) PCKPH_HW_SMBUS_ITEM Items,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwPciMmioIdentify(
    _In_ ULONG Locator,
    _Out_ PKPH_HW_PCI_MMIO Device
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwPciMmioRead(
    _In_ ULONG Locator,
    _In_ ULONG Window,
    _In_reads_(Count) PCKPH_HW_WINDOW_ITEM Items,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE64 Values
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwPciConfigIdentify(
    _In_ ULONG Locator,
    _Out_ PKPH_HW_PCI_CONFIG Device
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwPciConfigRead(
    _In_ ULONG Locator,
    _In_reads_(Count) PCKPH_HW_WINDOW_ITEM Items,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwPciConfigReadBlock(
    _In_ ULONG Locator,
    _In_ ULONG Offset,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwPlatformRead(
    _In_ ULONG Window,
    _In_reads_(Count) PCKPH_HW_WINDOW_ITEM Items,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE64 Values
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwAcpiIdentify(
    _Out_writes_to_opt_(Count, *ReturnCount) PKPH_HW_ACPI_DEVICE Devices,
    _In_ ULONG Count,
    _Out_ PULONG ReturnCount
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwAcpiEval(
    _In_ ULONG Milliseconds,
    _In_reads_bytes_(EvalsLength) PCVOID Evals,
    _In_ ULONG EvalsLength,
    _Out_writes_bytes_to_(Length, *ReturnLength) PVOID Output,
    _In_ ULONG Length,
    _Out_ PULONG ReturnLength
    );

PHLIBAPI
VOID
NTAPI
KphHwAcpiAppendEval(
    _Inout_ PPH_BYTES_BUILDER Evals,
    _In_ ULONG Row,
    _In_ ULONG Ordinal,
    _In_ PCPH_BYTESREF Name,
    _In_opt_ PCPH_BYTESREF Arguments
    );

EXTERN_C_END

#endif
