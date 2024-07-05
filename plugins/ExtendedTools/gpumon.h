/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2015
 *     dmex    2016-2023
 *
 */

#ifndef GPUMON_H
#define GPUMON_H

// Macros

#define BYTES_NEEDED_FOR_BITS(Bits) ((((Bits) + sizeof(ULONG) * 8 - 1) / 8) & ~(SIZE_T)(sizeof(ULONG) - 1)) // divide round up

// Functions

BOOLEAN EtpGpuInitializeD3DStatistics(
    VOID
    );

PETP_GPU_ADAPTER EtpAllocateGpuAdapter(
    _In_ ULONG NumberOfSegments
    );

VOID NTAPI EtGpuProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

#endif
