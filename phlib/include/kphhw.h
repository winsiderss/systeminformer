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

C_ASSERT(sizeof(KPH_HW_CPU_IDENTITY) == 28);
C_ASSERT(sizeof(KPH_HW_CPU_ITEM) == 8);
C_ASSERT(sizeof(KPH_HW_VALUE64) == 16);
C_ASSERT(sizeof(KPH_HW_SUPERIO_CHIP) == 12);
C_ASSERT(sizeof(KPH_HW_VALUE32) == 8);

PHLIBAPI
NTSTATUS
NTAPI
KphHwCpuIdentify(
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

EXTERN_C_END

#endif
