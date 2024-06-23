/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2015
 *     dmex    2016-2023
 *     jxy-s   2024
 *
 */

#ifndef NPUMON_H
#define NPUMON_H

// Macros

#define BYTES_NEEDED_FOR_BITS(Bits) ((((Bits) + sizeof(ULONG) * 8 - 1) / 8) & ~(SIZE_T)(sizeof(ULONG) - 1)) // divide round up

// Functions

BOOLEAN EtpNpuInitializeD3DStatistics(
    VOID
    );

PETP_NPU_ADAPTER EtpAllocateNpuAdapter(
    _In_ ULONG NumberOfSegments
    );

VOID NTAPI EtNpuProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

#endif
