/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2023-2025
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
    _Out_opt_ PKPH_DYN_DATA* Data,
    _Out_opt_ PVOID* Fields
    );
